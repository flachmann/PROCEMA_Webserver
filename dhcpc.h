/*----------------------------------------------------------------------------
 Copyright:      Michael Kleiber
 Author:         Michael Kleiber
 Remarks:        
 known Problems: none
 Version:        29.04.2008
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

#if USE_DHCP
#ifndef _DHCPCLIENT_H
	#define _DHCPCLIENT_H

	//#define DHCP_DEBUG usart_write
	#define DHCP_DEBUG(...)

	#define DHCP_CLIENT_PORT		  68
	#define DHCP_SERVER_PORT		  67


volatile unsigned long dhcp_lease;
volatile unsigned char dhcp_timer;

extern void dhcp_init     (void);
extern void dhcp_message  (unsigned char type);
extern void dhcp_get      (void);
extern unsigned char dhcp (void);

#endif //_DHCPCLIENT_H
#endif //USE_DHCP

