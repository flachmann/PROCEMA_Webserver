/*----------------------------------------------------------------------------
 Copyright:      Michael Kleiber
 Author:         Michael Kleiber
 Remarks:        
 known Problems: none
 Version:        29.11.2008
 Description:    DHCP Client

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
#include "dnsc.h"
#include "dhcpc.h"
	
#if USE_DHCP

struct dhcp_msg
{
  unsigned char op;              //1
  unsigned char htype;           //2
  unsigned char hlen;            //3
  unsigned char hops;            //4
  unsigned char xid[4];          //8
  unsigned int  secs;            //12
  unsigned int  flags;           //14
  unsigned char ciaddr[4];       //16
  unsigned char yiaddr[4];       //20
  unsigned char siaddr[4];       //24
  unsigned char giaddr[4];       //28
  unsigned char chaddr[16];      //44
  unsigned char sname[64];       //108
  unsigned char file[128];       //Up to here 236 Bytes
  unsigned char options[312];    // receive up to 312 bytes
};

struct dhcp_cache
{
  unsigned char type;
  unsigned char ovld;
  unsigned char router_ip[4];
  unsigned char dns1_ip  [4];
  unsigned char dns2_ip  [4];
  unsigned char netmask  [4];
  unsigned char lease    [4];
  unsigned char serv_id  [4];
};

//DHCP Message types for Option 53
#define DHCPDISCOVER   1     // client -> server
#define DHCPOFFER      2     // server -> client
#define DHCPREQUEST    3     // client -> server
#define DHCPDECLINE    4     // client -> server
#define DHCPACK        5     // server -> client
#define DHCPNAK        6     // server -> client
#define DHCPRELEASE    7     // client -> server
#define DHCPINFORM     8     // client -> server

unsigned char dhcp_state;
#define DHCP_STATE_IDLE             0
#define DHCP_STATE_DISCOVER_SENT    1
#define DHCP_STATE_OFFER_RCVD       2
#define DHCP_STATE_SEND_REQUEST     3
#define DHCP_STATE_REQUEST_SENT     4
#define DHCP_STATE_ACK_RCVD         5
#define DHCP_STATE_NAK_RCVD         6
#define DHCP_STATE_ERR              7
#define DHCP_STATE_FINISHED         8

struct dhcp_cache cache; 
volatile unsigned long dhcp_lease;
//----------------------------------------------------------------------------
//Init of DHCP client port
void dhcp_init (void)
{
  //Port in Anwendungstabelle eintragen für eingehende DHCP Daten!
  add_udp_app (DHCP_CLIENT_PORT, (void(*)(unsigned char))dhcp_get);
  dhcp_state = DHCP_STATE_IDLE;
  return;
}
//----------------------------------------------------------------------------
// Configure this client by DHCP
unsigned char dhcp (void)
{
  unsigned char timeout_cnt;
  
  timeout_cnt = 0;

	if ( (dhcp_state == DHCP_STATE_FINISHED ) &&
       (dhcp_lease < 600)                         )
  {
      dhcp_state = DHCP_STATE_SEND_REQUEST;
  } 
	
  do
  {
    if ( timeout_cnt > 3 )
    {
      dhcp_state = DHCP_STATE_ERR;
      DHCP_DEBUG("DHCP timeout\r\n");
      return (1);
    }
    switch (dhcp_state)
    {
      case DHCP_STATE_IDLE:
        dhcp_message(DHCPDISCOVER);
        gp_timer = 5;
      break;
      case DHCP_STATE_DISCOVER_SENT:
        if (gp_timer == 0) 
        {
          dhcp_state = DHCP_STATE_IDLE;
          timeout_cnt++;
        }
      break;
      case DHCP_STATE_OFFER_RCVD:
        timeout_cnt = 0;
				dhcp_state = DHCP_STATE_SEND_REQUEST;
      break;
      case DHCP_STATE_SEND_REQUEST:
        gp_timer  = 5;
        dhcp_message(DHCPREQUEST);
      break;
      case DHCP_STATE_REQUEST_SENT:
        if (gp_timer == 0) 
        {
          dhcp_state = DHCP_STATE_SEND_REQUEST;
          timeout_cnt++;
        }
      break;
      case DHCP_STATE_ACK_RCVD:
        DHCP_DEBUG("LEASE %2x%2x%2x%2x\r\n", cache.lease[0],cache.lease[1],cache.lease[2],cache.lease[3]);
		
        dhcp_lease = (unsigned long)cache.lease[0] << 24 | (unsigned long)cache.lease[1] << 16 | (unsigned long)cache.lease[2] <<  8 |(unsigned long)cache.lease[3];

		(*((unsigned long*)&netmask[0]))       = (*((unsigned long*)&cache.netmask[0]));
        (*((unsigned long*)&router_ip[0]))     = (*((unsigned long*)&cache.router_ip[0]));
				#if USE_DNS
				(*((unsigned long*)&dns_server_ip[0])) = (*((unsigned long*)&cache.dns1_ip[0]));
				#endif
        dhcp_state = DHCP_STATE_FINISHED;
      break;
      case DHCP_STATE_NAK_RCVD:
        dhcp_state = DHCP_STATE_IDLE;
      break;
    }
    eth_get_data();
  }
  while ( dhcp_state != DHCP_STATE_FINISHED );
  return(0);
}

//----------------------------------------------------------------------------
//Sendet DHCP messages an Broadcast
void dhcp_message (unsigned char type)
{
  struct dhcp_msg *msg;
  unsigned char   *options;
  
  for (unsigned int i=0; i < sizeof (struct dhcp_msg); i++) //clear eth_buffer to 0
  {
    eth_buffer[UDP_DATA_START+i] = 0;
  }
  
  msg = (struct dhcp_msg *)&eth_buffer[UDP_DATA_START];
  msg->op          = 1; // BOOTREQUEST
  msg->htype       = 1; // Ethernet
  msg->hlen        = 6; // Ethernet MAC
  msg->xid[0]      = MYMAC6; //use the MAC as the ID to be unique in the LAN
  msg->xid[1]      = MYMAC5;
  msg->xid[2]      = MYMAC4;
  msg->xid[3]      = MYMAC3;
  msg->flags       = HTONS(0x8000);
  msg->chaddr[0]   = MYMAC1;
  msg->chaddr[1]   = MYMAC2;
  msg->chaddr[2]   = MYMAC3;
  msg->chaddr[3]   = MYMAC4;
  msg->chaddr[4]   = MYMAC5;
  msg->chaddr[5]   = MYMAC6;
  
  options = &msg->options[0];  //options
  *options++       = 99;       //magic cookie
  *options++       = 130;
  *options++       = 83;
  *options++       = 99;

  *options++       = 53;    // Option 53: DHCP message type DHCP Discover
  *options++       = 1;     // len = 1
  *options++       = type;  // 1 = DHCP Discover
  
  *options++       = 55;    // Option 55: parameter request list
  *options++       = 3;     // len = 3
  *options++       = 1;     // netmask
  *options++       = 3;     // router
  *options++       = 6;     // dns

  *options++       = 50;    // Option 54: requested IP
  *options++       = 4;     // len = 4
  *options++       = myip[0];
  *options++       = myip[1];
  *options++       = myip[2];
  *options++       = myip[3];

  switch (type)
  {
    case DHCPDISCOVER:
      dhcp_state = DHCP_STATE_DISCOVER_SENT;
      DHCP_DEBUG("DISCOVER sent\r\n");
    break;
    case DHCPREQUEST:
      *options++       = 54;    // Option 54: server ID
      *options++       = 4;     // len = 4
      *options++       = cache.serv_id[0];
      *options++       = cache.serv_id[1];
      *options++       = cache.serv_id[2];
      *options++       = cache.serv_id[3];
      dhcp_state = DHCP_STATE_REQUEST_SENT;
      DHCP_DEBUG("REQUEST sent\r\n");
    break;
    default:
      DHCP_DEBUG("Wrong DHCP msg type\r\n");
    break;
  }

  *options++       = 12;    // Option 12: host name
  *options++       = 8;     // len = 8
  *options++       = 'M';
  *options++       = 'i';
  *options++       = 'n';
  *options++       = 'i';
  *options++       = '-';
  *options++       = 'A';
  *options++       = 'V';
  *options++       = 'R';
  
  *options         = 0xff;  //end option

  create_new_udp_packet(sizeof (struct dhcp_msg),DHCP_CLIENT_PORT,DHCP_SERVER_PORT,(unsigned long)0xffffffff);
}
//----------------------------------------------------------------------------
//liest 4 bytes aus einem buffer und speichert in dem anderen
void get4bytes (unsigned char *source, unsigned char *target)
{
  unsigned char i;
  
  for (i=0; i<4; i++)
  {
    *target++ = *source++;
  }
}
//----------------------------------------------------------------------------
//read all the options
//pointer to the variables and size from options to packet end
void dhcp_parse_options (unsigned char *msg, struct dhcp_cache *c, unsigned int size)
{
  unsigned int ix;

  ix = 0;
  do
  {
    switch (msg[ix])
    {
      case 0: //Padding
      ix++;
      break;
      case 1: //Netmask
        ix++;
        if ( msg[ix] == 4 )
        {
          ix++;
          get4bytes(&msg[ix], &c->netmask[0]);
          ix += 4;
        }
        else
        {
          ix += (msg[ix]+1);
        }
      break;
      case 3: //router (gateway IP)
        ix++;
        if ( msg[ix] == 4 )
        {
          ix++;
          get4bytes(&msg[ix], &c->router_ip[0]);
          ix += 4;
        }
        else
        {
          ix += (msg[ix]+1);
        }
      break;
      case 6: //dns len = n * 4
        ix++;
        if ( msg[ix] == 4 )
        {
          ix++;
          get4bytes(&msg[ix], &c->dns1_ip[0]);
          ix += 4;
        }
        else
        if ( msg[ix] == 8 )
        {
          ix++;
          get4bytes(&msg[ix], &c->dns1_ip[0]);
          ix += 4;
          get4bytes(&msg[ix], &c->dns2_ip[0]);
          ix += 4;
        }
        else
        {
          ix += (msg[ix]+1);
        }
      break;
      case 51: //lease time
        ix++;
        if ( msg[ix] == 4 )
        {
          ix++;
          get4bytes(&msg[ix], &c->lease[0]);
          ix += 4;
        }
        else
        {
          ix += msg[ix]+1;
        }
      break;
      case 52: //Options overload 
        ix++;
        if ( msg[ix] == 1 )   //size == 1
        {
          ix++;
          c->ovld   = msg[ix];
          ix++;
        }
        else
        {
          ix += (msg[ix]+1);
        }
      break;
      case 53: //DHCP Type 
        ix++;
        if ( msg[ix] == 1 )   //size == 1
        {
          ix++;
          c->type = msg[ix];
          ix++;
        }
        else
        {
          ix += (msg[ix]+1);
        }
      break;
      case 54: //Server identifier
        ix++;
        if ( msg[ix] == 4 )
        {
          ix++;
          get4bytes(&msg[ix], &c->serv_id[0]);
          ix += 4;
        }
        else
        {
          ix += msg[ix]+1;
        }
      break;
      case 99:   //Magic cookie
        ix += 4;
      break;
      case 0xff: //end option
      break;
      default: 
        DHCP_DEBUG("Unknown Option: %2x\r\n", msg[ix]);
        ix++;
        ix += (msg[ix]+1);
      break;
    }
  }
  while ( (msg[ix] != 0xff) && (ix < size) ); 
}

//----------------------------------------------------------------------------
//Wertet message vom DHCP Server aus
// DHCP packets: 20 Bytes IP Header, 8 Bytes UDP_Header,
// DHCP fixed fields 236 Bytes, min 312 Bytes options -> 576 Bytes min.
void dhcp_get (void)
{
  struct dhcp_msg  *msg;
  struct IP_Header *ip;
  unsigned char *p;
  unsigned int i;

  ip  = (struct IP_Header *)&eth_buffer[IP_OFFSET];
  if ( htons(ip->IP_Pktlen) > MTU_SIZE )
  {
    DHCP_DEBUG("DHCP too big, discarded\r\n");
    return;
  }

  p = &cache.type; //clear the cache
  for (i=0; i<sizeof(cache); i++)
  {
    p[i] = 0;
  }

  // set pointer of DHCP message to beginning of UDP data
  msg = (struct dhcp_msg *)&eth_buffer[UDP_DATA_START];

  //check the id
  if ( (msg->xid[0] != MYMAC6) ||
       (msg->xid[1] != MYMAC5) ||
       (msg->xid[2] != MYMAC4) ||
       (msg->xid[3] != MYMAC3)    )
  {
    DHCP_DEBUG("Wrong DHCP ID, discarded\r\n");
    return;
  }


  dhcp_parse_options(&msg->options[0], &cache, (htons(ip->IP_Pktlen)-264) );
  // check if file field or sname field are overloaded (option 52)
  switch (cache.ovld) 
  {
    case 0:  // no overload, do nothing
    break;
    case 1:  // the file field contains options
      dhcp_parse_options(&msg->file[0], &cache, 128);
    break;
    case 2:  // the sname field contains options
      dhcp_parse_options(&msg->sname[0], &cache, 64);
    break;
    case 3:  // the file and the sname field contain options
      dhcp_parse_options(&msg->file[0], &cache, 128);
      dhcp_parse_options(&msg->sname[0], &cache, 64);
    break;
    default: // must not occur
      DHCP_DEBUG("Option 52 Error\r\n");
    break;
  }

  switch (cache.type)
  {
    case DHCPOFFER:
      // this will be our IP address
      (*((unsigned long*)&myip[0])) = (*((unsigned long*)&msg->yiaddr[0]));
      DHCP_DEBUG("** DHCP OFFER RECVD! **\r\n");
      dhcp_state = DHCP_STATE_OFFER_RCVD;
    break;
    case DHCPACK:
      DHCP_DEBUG("** DHCP ACK RECVD! **\r\n");
      dhcp_state = DHCP_STATE_ACK_RCVD;
    break;
    case DHCPNAK:
      DHCP_DEBUG("** DHCP NAK RECVD! **\r\n");
      dhcp_state = DHCP_STATE_NAK_RCVD;
    break;
  }
}

#endif
