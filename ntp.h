/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        12.11.2007
 Description:    NTP Client

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
#if USE_NTP
#ifndef _NTPCLIENT_H
	#define _NTPCLIENT_H

	//#define NTP_DEBUG usart_write
	#define NTP_DEBUG(...)

	#define NTP_CLIENT_PORT		2300
	#define NTP_SERVER_PORT		123

    #define NTP_STATE_IDLE     0
    #define NTP_STATE_REQ_SENT 1
    #define NTP_STATE_REQ_ERR  2
    #define NTP_STATE_FINISHED 3
    #define NTP_STATE_REC_ERR  4

	#define NTP_IP_EEPROM_STORE 	50

    #define	EPOCH_YR	1970
    //(24L * 60L * 60L)
    #define	SECS_DAY	86400UL  
    #define	LEAPYEAR(year)	(!((year) % 4) && (((year) % 100) || !((year) % 400)))
    #define	YEARSIZE(year)	(LEAPYEAR(year) ? 366 : 365)

    // Number of seconds between 1-Jan-1900 and 1-Jan-1970, unix time starts 1970
    // and ntp time starts 1900.
    #define GETTIMEOFDAY_TO_NTP_OFFSET 2208988800UL
  
    typedef struct
    {
        unsigned char	second;
        unsigned char	minute;
        unsigned char	hour;
        unsigned char	day;
        unsigned char	wday;
        unsigned char	month;
        unsigned int	year;
        char	datestr[14];
        char	timestr[9];
    } ntp_struct;

	unsigned char ntp_server_ip[4];
	volatile unsigned int ntp_timer;
    char dstr[24];
    unsigned char ntp_state;
    
	extern void ntp_init(void);
    extern unsigned char ntp (void);
	extern void ntp_request(void); 
	extern void ntp_get(unsigned char);
    extern void decode_time(unsigned long time_sec, ntp_struct *dt);
	
	#define GMT_TIME_CORRECTION 3600	//in Sekunden
	
	struct NTP_GET_Header 
    {
		char dummy[40];
		unsigned long rx_timestamp;
	};
	
#endif
#endif //USE_NTP

