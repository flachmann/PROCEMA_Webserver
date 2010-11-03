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

#ifndef _CMD_H_
	#define _CMD_H_	
	
	typedef struct
	{
		char* cmd; 				        // Stringzeiger auf Befehlsnamen
		void(*fp)(void);  	// Zeiger auf auszuführende Funktion
	} COMMAND_STRUCTUR;	
	
	#define MAX_VAR	10
	#define HELPTEXT	1
	
	extern unsigned char extract_cmd (char *);
	extern void write_eeprom_ip (unsigned int);
	
    extern void save_ip_addresses(void);
    extern void read_ip_addresses (void);
  
	//reset the unit
	extern void command_reset		(void);
	//Ausgabe der ARP Tabelle
	extern void command_arp 		(void);
	//Ausgabe der TCP Tabelle
	extern void command_tcp 		(void);
	
	//ändern/ausgabe der IP, NETMASK, ROUTER_IP, NTP_IP
	extern void command_ip 			(void);
	extern void command_net 		(void);
	extern void command_router		(void);
	extern void command_ntp 		(void);

	extern void command_mac			(void);
	extern void command_ver			(void);
	extern void command_setvar		(void);
	extern void command_time		(void);
	extern void command_ntp_refresh	(void);
	#if USE_WOL
	extern void command_wol 		(void);
	#endif //USE_WOL
    #if USE_MAIL
	extern void command_mail 		(void);
	#endif //USE_MAIL
	extern void command_ping 		(void);
	extern void command_help 		(void);
	
	#define RESET() {asm("ldi r30,0"); asm("ldi r31,0"); asm("ijmp");}
	
#endif //_CMD_H_


