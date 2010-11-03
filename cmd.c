/*------------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        24.10.2007
 Description:    Commando Interpreter

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
#include "networkcard/enc28j60.h"
#include "networkcard/rtl8019.h"
#include "cmd.h"
#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include "usart.h"
#include "stack.h"
#include "httpd.h"
#include "ntp.h"
#include "wol.h"
#include "sendmail.h"
#include "timer.h"
#include "dnsc.h"
	
volatile unsigned int variable[MAX_VAR];

COMMAND_STRUCTUR COMMAND_TABELLE[] = // Befehls-Tabelle
{
	{"RESET",command_reset},
	{"ARP",command_arp},
	{"TCP",command_tcp},
	{"IP",command_ip},
	{"NET",command_net},
	{"ROUTER",command_router},
	{"NTP",command_ntp},
	{"MAC",command_mac},
	{"VER",command_ver},
	{"SV",command_setvar},
	{"TIME",command_time},
	{"NTPR",command_ntp_refresh},	
	#if USE_WOL
	{"WOL",command_wol},
	#endif //USE_WOL
	{"PING", command_ping},
	#if USE_MAIL
    {"mail", command_mail},
    #endif //USE_MAIL
	#if HELPTEXT
	{"HELP",command_help},
	{"?",command_help},
	#endif //HELPTEXT
	{NULL,NULL} 
};

#if HELPTEXT
	PROGMEM char helptext[] = {
		"RESET  - reset the AVR - Controller\r\n"
		"ARP    - list the ARP table\r\n"
		"TCP    - list the tcp table\r\n"
		"IP     - list/change ip\r\n"
		"NET    - list/change netmask\r\n"
		"ROUTER - list/change router ip\r\n"
		"NTP    - list/change NTP\r\n"
		"NTPR   - NTP Refresh\r\n"
		"MAC    - list MAC-address\r\n"
		"VER    - list enc version number\r\n"
		"SV     - set variable\r\n"
        "PING   - send Ping\r\n"
        #if USE_MAIL
        "MAIL   - send E_MAIL\r\n"
        #endif //USE_MAIL
		#if USE_WOL
		"WOL    - send WOL / set MAC / set MAC and IP\r\n"
		#endif //USE_WOL
		"TIME   - get time\r\n"
		"HELP   - print Helptext\r\n"
		"?      - print Helptext\r\n"
	};
#endif

//------------------------------------------------------------------------------
//Commando auswerten
unsigned char extract_cmd (char *string_pointer)
{
	//Stringzeiger;
	char *string_pointer_tmp;
	unsigned char cmd_index = 0;
 
    string_pointer_tmp = strsep(&string_pointer," "); 

	//Kommando in Tabelle suchen
	while(strcasecmp(COMMAND_TABELLE[cmd_index].cmd,string_pointer_tmp))
    {
        //Abruch Whileschleife und Unterprogramm verlassen 
        if (COMMAND_TABELLE[++cmd_index].cmd == 0) return(0);
    }
    
    //Variablen finden und auswerten
	for (unsigned char a = 0; a<MAX_VAR;a++)
	{ 
        string_pointer_tmp = strsep(&string_pointer,"., ");  
		variable[a] = strtol(string_pointer_tmp,NULL,0);
	}

    //Kommando ausführen
	COMMAND_TABELLE[cmd_index].fp();
	return(1); 
}

//------------------------------------------------------------------------------
//Reset ausführen
void command_reset (void)
{
	RESET();
}

//------------------------------------------------------------------------------
//print/edit own IP
void command_ip (void)
{
	write_eeprom_ip(IP_EEPROM_STORE);
	(*((unsigned long*)&myip[0])) = get_eeprom_value(IP_EEPROM_STORE,MYIP);
	usart_write("My IP: %1i.%1i.%1i.%1i\r\n",myip[0],myip[1],myip[2],myip[3]);
}

//------------------------------------------------------------------------------
//
void write_eeprom_ip (unsigned int eeprom_adresse)
{
	if (*((unsigned int*)&variable[0]) != 0x00000000)
	{	
		//value ins EEPROM schreiben
		for (unsigned char count = 0; count<4;count++)
		{
			eeprom_busy_wait ();
			eeprom_write_byte((unsigned char *)(eeprom_adresse + count),variable[count]);
		}
	}
}

//------------------------------------------------------------------------------
//print/edit NTP Server IP
void command_ntp (void)
{
	#if USE_NTP
	write_eeprom_ip(NTP_IP_EEPROM_STORE);
	(*((unsigned long*)&ntp_server_ip[0])) = get_eeprom_value(NTP_IP_EEPROM_STORE,NTP_IP);
	usart_write("NTP_Server: %1i.%1i.%1i.%1i\r\n",ntp_server_ip[0],ntp_server_ip[1],ntp_server_ip[2],ntp_server_ip[3]);
	#endif //USE_NTP
}

//------------------------------------------------------------------------------
//print/edit Netmask
void command_net (void)
{
	write_eeprom_ip(NETMASK_EEPROM_STORE);
	(*((unsigned long*)&netmask[0])) = get_eeprom_value(NETMASK_EEPROM_STORE,NETMASK);
	usart_write("NETMASK: %1i.%1i.%1i.%1i\r\n",netmask[0],netmask[1],netmask[2],netmask[3]);
}

//------------------------------------------------------------------------------
//print/edit Router IP
void command_router (void)
{
	write_eeprom_ip(ROUTER_IP_EEPROM_STORE);
	(*((unsigned long*)&router_ip[0])) = get_eeprom_value(ROUTER_IP_EEPROM_STORE,ROUTER_IP);
	usart_write("Router IP: %1i.%1i.%1i.%1i\r\n",router_ip[0],router_ip[1],router_ip[2],router_ip[3]);
}

//------------------------------------------------------------------------------
//print own mac
void command_mac (void)
{
	usart_write("My MAC: %2x:%2x:%2x:%2x:%2x:%2x\r\n",mymac[0],mymac[1],mymac[2],mymac[3],mymac[4],mymac[5]);
}

//------------------------------------------------------------------------------
//print enc28j60 chip version
void command_ver (void)
{
#if USE_ENC28J60
	usart_write("ENC28J60-Version: %1x\r\n", enc28j60_revision);
#endif

#if USE_RTL8019
	usart_write("RTL8019 Ethernetcard\r\n");
#endif
}

//------------------------------------------------------------------------------
//print ARP table
void command_arp (void)
{
	for (unsigned char index = 0;index<MAX_ARP_ENTRY;index++)
	{
		usart_write("%2i  MAC:%2x",index,arp_entry[index].arp_t_mac[0]);
		usart_write(".%2x",arp_entry[index].arp_t_mac[1]);
		usart_write(".%2x",arp_entry[index].arp_t_mac[2]);
		usart_write(".%2x",arp_entry[index].arp_t_mac[3]);
		usart_write(".%2x",arp_entry[index].arp_t_mac[4]);
		usart_write(".%2x",arp_entry[index].arp_t_mac[5]);
		
		usart_write("  IP:%3i",(arp_entry[index].arp_t_ip&0x000000FF));
		usart_write(".%3i",((arp_entry[index].arp_t_ip&0x0000FF00)>>8));
		usart_write(".%3i",((arp_entry[index].arp_t_ip&0x00FF0000)>>16));
		usart_write(".%3i",((arp_entry[index].arp_t_ip&0xFF000000)>>24));
			
		usart_write("  Time:%4i\r\n",arp_entry[index].arp_t_time);
	}
}

//------------------------------------------------------------------------------
//print ARP table
void command_tcp (void)
{
	for (unsigned char index = 0;index<MAX_TCP_ENTRY;index++)
	{
		usart_write("%2i",index);
		usart_write("  IP:%3i",(tcp_entry[index].ip&0x000000FF));
		usart_write(".%3i",((tcp_entry[index].ip&0x0000FF00)>>8));
		usart_write(".%3i",((tcp_entry[index].ip&0x00FF0000)>>16));
		usart_write(".%3i",((tcp_entry[index].ip&0xFF000000)>>24));
		usart_write("  PORT:%4i",HTONS(tcp_entry[index].src_port));
		usart_write("  Time:%4i\r\n",tcp_entry[index].time);
	}
}

//------------------------------------------------------------------------------
//ändern einer Variable
void command_setvar (void)
{
	var_array[variable[0]] = variable[1];
	usart_write("Inhalt der Variable[%2i] = %2i\r\n",variable[0],var_array[variable[0]]);
}

//------------------------------------------------------------------------------
//print Time
void command_time (void)
{
	unsigned char hh = (time/3600)%24;
	unsigned char mm = (time/60)%60;
	unsigned char ss = time %60;
	usart_write ("\n\rTIME: %2i:%2i:%2i\r\n",hh,mm,ss);
}

//------------------------------------------------------------------------------
//Time Refresh via NTP-Server
void command_ntp_refresh (void)
{
	#if USE_NTP
	ntp_request();
	#endif //USE_NTP
}

//------------------------------------------------------------------------------
//Sendet eine fertige E-MAIL
#if USE_MAIL
void command_mail (void)
{
	mail_enable = 1;
}
#endif //USE_MAIL

//------------------------------------------------------------------------------
//
void command_wol (void)
{
	#if USE_WOL
	// EEPROM beschreiben, falls Parameter angegeben wurden
	if ((*((unsigned int*)&variable[0]) != 0x00000000) || (*((unsigned int*)&variable[1]) != 0x00000000) || (*((unsigned int*)&variable[2]) != 0x00000000))
	{	
		//schreiben der MAC
		for (unsigned char count = 0; count<6; count++)
		{
			eeprom_busy_wait ();
			eeprom_write_byte((unsigned char *)(WOL_MAC_EEPROM_STORE + count),variable[count]);
		}
		//zusätzlich schreiben der Broadcast-Adresse, falls vorhandenden
		for (unsigned char count = 0; count<4 && (*((unsigned int*)&variable[6]) != 0x00000000);count++)
		{
			eeprom_busy_wait ();
			eeprom_write_byte((unsigned char*)(WOL_BCAST_EEPROM_STORE + count),variable[count+6]);
		}
		//init
		wol_init();
	}else{
		//MagicPacket senden
		wol_request();
	}
	#endif //USE_WOL	
}

//------------------------------------------------------------------------------
// Sende "Ping" an angegebene Adresse
void command_ping (void)
{
	if (*((unsigned int*)&variable[0]) != 0x00000000)
	{
		(*((unsigned long*)&ping.ip1[0])) = ((variable[0])+((unsigned long)(variable[1])<<8)+((unsigned long)(variable[2])<<16)+((unsigned long)(variable[3])<<24));
		//ARP Request senden
		arp_request ( (*(unsigned long*)&ping.ip1[0]));
		
		//ICMP-Nachricht Type=8 Code=0: Echo-Anfrage
		//TODO: Sequenznummer, Identifier 
		icmp_send( (*(unsigned long*)&ping.ip1[0]),0x08,0x00,1,1);
	}
}

//------------------------------------------------------------------------------
//print helptext
void command_help (void)
{
#if HELPTEXT
	unsigned char data;
	PGM_P helptest_pointer = helptext;
	
	do
	{
		data = pgm_read_byte(helptest_pointer++);
		usart_write("%c",data);
	}while(data != 0);
#endif
}

//------------------------------------------------------------------------------
//save all ip-datas
void save_ip_addresses(void)
{
  for (unsigned char count = 0; count<4; count++)
  {
    eeprom_busy_wait ();
    eeprom_write_byte((unsigned char *)(IP_EEPROM_STORE + count),myip[count]);
  }
  for (unsigned char count = 0; count<4; count++)
  {
    eeprom_busy_wait ();
    eeprom_write_byte((unsigned char *)(NETMASK_EEPROM_STORE + count),netmask[count]);
  }
  for (unsigned char count = 0; count<4; count++)
  {
    eeprom_busy_wait ();
    eeprom_write_byte((unsigned char *)(ROUTER_IP_EEPROM_STORE + count),router_ip[count]);
  }
  #if USE_DNS
  for (unsigned char count = 0; count<4; count++)
  {
    eeprom_busy_wait ();
    eeprom_write_byte((unsigned char *)(DNS_IP_EEPROM_STORE + count),dns_server_ip[count]);
  }
  #endif //USE_DNS
}

//------------------------------------------------------------------------------
//Read all IP-Datas
void read_ip_addresses (void)
{
  (*((unsigned long*)&myip[0]))      = get_eeprom_value(IP_EEPROM_STORE,MYIP);
  (*((unsigned long*)&netmask[0]))   = get_eeprom_value(NETMASK_EEPROM_STORE,NETMASK);
  (*((unsigned long*)&router_ip[0])) = get_eeprom_value(ROUTER_IP_EEPROM_STORE,ROUTER_IP);
	
	#if USE_DNS
	//DNS-Server IP aus EEPROM auslesen
	(*((unsigned long*)&dns_server_ip[0])) = get_eeprom_value(DNS_IP_EEPROM_STORE,DNS_IP);
	#endif
}









