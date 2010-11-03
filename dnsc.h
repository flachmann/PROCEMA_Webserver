/*----------------------------------------------------------------------------
 Copyright:      Michael Kleiber
 Author:         Michael Kleiber
 Remarks:        
 known Problems: none
 Version:        25.04.2008
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
#if USE_DNS
#ifndef _DNSCLIENT_H
	#define _DNSCLIENT_H

	//#define DNS_DEBUG usart_write
	#define DNS_DEBUG(...)

	#define DNS_CLIENT_PORT		1078
	#define DNS_SERVER_PORT		53

	#define DNS_IP_EEPROM_STORE 	42

	extern unsigned char dns_server_ip[4];
    extern unsigned char dns_resolved_ip[4];
	volatile unsigned int dns_timer;
	
	extern void dns_init(void);
    extern unsigned char dns_resolve (char *name);
    extern void dns_query(char *name);
	extern void dns_get(void);
	
	
#endif //_DNSCLIENT_H
#endif //USE_DNS

