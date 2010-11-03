/*----------------------------------------------------------------------------
 Copyright:      Michael Kleiber
 Author:         Michael Kleiber 
 Remarks:        
 known Problems: none
 Version:        29.11.2008
 Description:    DNS Client

 Dieses Programm ist freie Software. Sie können es unter den Bedingungen der 
 GNU General Public License, wie von der Free Software Foundation veröffentlicht, 
 weitergeben und/oder modifizieren, entweder gemäß Version 2 der Lizenz oder 
 (nach Ihrer Option) jeder späteren Version. 

 Die Veröffentlichung dieses Programms erfolgt in der Hoffnung, 
 daß es Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, 
 sogar ohne die implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT 
 FÜR EINEN BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License. 

 Sie sollten eine Kopie der GNU General Public License zusammen mit diesem 
 Programm erhalten haben. 
 Falls nicht, schreiben Sie an die Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA. 
------------------------------------------------------------------------------*/
#include "config.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "stack.h"
#include "usart.h"
#include "timer.h"
	
#if USE_DNS

#include "dnsc.h"

//----------------------------------------------------------------------------

unsigned char dns_server_ip[4];
unsigned char dns_resolved_ip[4];
unsigned char dns_state;

#define DNS_STATE_IDLE     0
#define DNS_STATE_REQ_SENT 1
#define DNS_STATE_REQ_ERR  2
#define DNS_STATE_FINISHED 3
#define DNS_STATE_REC_ERR  4

//----------------------------------------------------------------------------
//Eintrag des DNS clients im stack
void dns_init (void)
{
    //ADD the DNS port as apllication to the list
    add_udp_app (DNS_CLIENT_PORT, (void(*)(unsigned char))dns_get);
  
    //DNS IP read from EEPROM
    (*((unsigned long*)&dns_server_ip[0])) = get_eeprom_value(DNS_IP_EEPROM_STORE,DNS_IP);

    DNS_DEBUG("DNS  %1i.%1i.%1i.%1i\r\n",dns_server_ip[0],dns_server_ip[1],dns_server_ip[2],dns_server_ip[3]);
    return;
}

//----------------------------------------------------------------------------
//resolve domain name
unsigned char dns_resolve (char *name)
{
    dns_state = DNS_STATE_IDLE;
  
    dns_query(name);

    if (dns_state != DNS_STATE_REQ_SENT)
    {
        return(1);
    }
    gp_timer = 30; //is decremented in timer.c

    do
    {
        eth_get_data();
    }
    while ( (dns_state == DNS_STATE_REQ_SENT) &&
          (gp_timer > 0                   )     );
          
    if ( ( dns_state != DNS_STATE_FINISHED) ||
         ( gp_timer == 0 )                       )
    {
        return(1);
    }
    return (0);
}
//----------------------------------------------------------------------------
//send query to DNS server
void dns_query(char *name)
{
    unsigned char   *nptr;
    unsigned char   n;
    unsigned int    byte_count;
    unsigned long   tmp_ip;

    tmp_ip = (*(unsigned long*)&dns_server_ip[0]);

    if (arp_request(tmp_ip) == 1) // if DNS server is answering to ARP
    {  
        nptr = &eth_buffer[UDP_DATA_START];
        *nptr++ = 0x12; //ID h
        *nptr++ = 0x34; //ID l
        *nptr++ = 0x01; //Flags h  FLAG_RD ->Standard query
        *nptr++ = 0x00; //Flags l
        *nptr++ = 0x00; //QDCOUNT h
        *nptr++ = 0x01; //QDCOUNT l
        *nptr++ = 0x00; //ANCOUNT h
        *nptr++ = 0x00; //ANCOUNT l
        *nptr++ = 0x00; //NSCOUNT h
        *nptr++ = 0x00; //NSCOUNT l
        *nptr++ = 0x00; //ARCOUNT h
        *nptr++ = 0x00; //ARCOUNT l
   
        byte_count = 12;
        --name;
        /* Convert hostname to query format. */
        do 
        {
            ++name;
            nptr = &eth_buffer[UDP_DATA_START+byte_count];   //remember the first byte
            byte_count++;
            for(n = 0; *name != '.' && *name != 0; ++name)  //copy the name
            {
                eth_buffer[UDP_DATA_START+byte_count] = *name;
                byte_count++;
                n++;
            }
            *nptr = n;       //store the length in the first byte
        }
        while(*name != 0); //until name string is finished 
   
        //end of query:
        eth_buffer[UDP_DATA_START+byte_count] = 0; //zero string
        byte_count++;
    
        eth_buffer[UDP_DATA_START+byte_count] = 0; //2 bytes QTYPE 1=a host address
        byte_count++;
        eth_buffer[UDP_DATA_START+byte_count] = 1; //
        byte_count++;
    
        eth_buffer[UDP_DATA_START+byte_count] = 0; //2 bytes QCLASS 1=Internet
        byte_count++;
        eth_buffer[UDP_DATA_START+byte_count] = 1; //
        byte_count++;
   
        create_new_udp_packet(byte_count,DNS_CLIENT_PORT,DNS_SERVER_PORT,tmp_ip);
        dns_state = DNS_STATE_REQ_SENT;
        return;
    }
    dns_state = DNS_STATE_REQ_ERR;
    DNS_DEBUG("No DNS server!!\r\n");
}

//----------------------------------------------------------------------------
//Überspringt den Namen im RR und gibt die Länge zurück
unsigned int skip_name (char *query)
{
    unsigned char n;
    unsigned int  count;

    count = 0;
    do 
    {
        n = *query++;
        count++;
    
        while(n > 0) 
        {
            query++;
            count++;
            n--;
        };
    
    }
    while(*query != 0);

    return count+1;
}

//----------------------------------------------------------------------------
//Empfang der Antwort vom DNS Server
void dns_get(void)
{
    unsigned int  byte_count;
    unsigned int  id;
    unsigned int  flags;
    unsigned int  qdcount;
    unsigned int  answ_cnt;
    unsigned int  antype;
    unsigned int  anclass;
    unsigned int  rdlen;
    unsigned char ip[4];
  
    DNS_DEBUG("** DNS DATA GET! **\r\n");

    id  = eth_buffer[UDP_DATA_START+ 0]*256;
    id += eth_buffer[UDP_DATA_START+ 1];
    DNS_DEBUG("ID %4x\r\n", id);

    flags  = eth_buffer[UDP_DATA_START+ 2]*256;
    flags += eth_buffer[UDP_DATA_START+ 3];
    DNS_DEBUG("Flags %4x\r\n", flags);

    qdcount  = eth_buffer[UDP_DATA_START+ 4]*256;
    qdcount += eth_buffer[UDP_DATA_START+ 5];
    DNS_DEBUG("QDCOUNT %4x\r\n", qdcount);

    answ_cnt  = eth_buffer[UDP_DATA_START+ 6]*256;
    answ_cnt += eth_buffer[UDP_DATA_START+ 7];
    DNS_DEBUG("ANCOUNT %4x\r\n", answ_cnt);
  
    if ( (flags & 0x000f) != 0 ) //RCODE 0=no error
    {
        DNS_DEBUG("Err in flags: %4x\r\n", flags);
        dns_state = DNS_STATE_REC_ERR;
        return;
    }

    // nscount and arcount discarded. Jump to the questions and answers
    byte_count = 12;

    // Skip the question...
    byte_count += skip_name( (char *)&eth_buffer[UDP_DATA_START + byte_count] );
    byte_count += 4; //QTYPE + QCLASS

    // now the answer:
    while(answ_cnt > 0) 
    {
        // first byte of the ARR determines a compressed record or a normal one.
        if( (eth_buffer[UDP_DATA_START + byte_count] & 0xc0) != 0 ) // Compressed name
        {                                                            // the remaining 14 bits 
            byte_count += 2;                                           // of the 2 bytes are the pointer
                                                                 // to the name... but ignored
        } 
        else   // Not compressed...
        {
            byte_count += skip_name( (char *)&eth_buffer[UDP_DATA_START + byte_count] );
        }

        antype  = eth_buffer[UDP_DATA_START + byte_count++]*256;
        antype += eth_buffer[UDP_DATA_START + byte_count++];
        //DNS_DEBUG("TYPE %4x\r\n",antype);
        anclass  = eth_buffer[UDP_DATA_START + byte_count++]*256;
        anclass += eth_buffer[UDP_DATA_START + byte_count++];
        //DNS_DEBUG("CLASS %4x\r\n",anclass);
    
        // skip TTL
        byte_count += 4;
    
        rdlen  = eth_buffer[UDP_DATA_START + byte_count++]*256;
        rdlen += eth_buffer[UDP_DATA_START + byte_count++];
        //DNS_DEBUG("RDLEN %4x\r\n",rdlen);

        // Check for IP address type and Internet class. Others are discarded.
        if(antype == 1 && anclass == 1 && rdlen == 4 ) 
        {
            ip[0] = eth_buffer[UDP_DATA_START + byte_count++];
            ip[1] = eth_buffer[UDP_DATA_START + byte_count++];
            ip[2] = eth_buffer[UDP_DATA_START + byte_count++];
            ip[3] = eth_buffer[UDP_DATA_START + byte_count++];
            DNS_DEBUG("IP address %1i.%1i.%1i.%1i\r\n",ip[0], ip[1], ip[2], ip[3]);
            if ( dns_state != DNS_STATE_FINISHED )
            {
                (*((unsigned long*)&dns_resolved_ip[0])) = (*((unsigned long*)&ip[0]));
                dns_state = DNS_STATE_FINISHED;
            }
        } 
        else 
        {
            byte_count += rdlen;
        }
        answ_cnt -= 1;
    } //while answ_cnt >0
}
/*---------------------------------------------------------------------------*/
#endif //USE_DNS

