/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich, Michael Kleiber
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
#include "config.h"
#include "ntp.h"
#include <avr/io.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include "stack.h"
#include "usart.h"	
#include "timer.h"
#include "httpd.h"
	
#if USE_NTP
volatile unsigned int ntp_timer = NTP_REFRESH;

//----------------------------------------------------------------------------
//
PROGMEM char NTP_Request[] = {
    0xd9,0x00,0x0a,0xfa,0x00,0x00,0x00,0x00,
    0x00,0x01,0x04,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc7,0xd6,0xac,0x72,0x08,0x00,0x00,0x00
};

PROGMEM char wday_str[] = "So\0Mo\0Di\0Mi\0Do\0Fr\0Sa";

unsigned char ntp_server_ip[4];
unsigned char ntp_state;

//----------------------------------------------------------------------------
//Enter NTP Port for receive
void ntp_init (void)
{
    //Port in Anwendungstabelle eintragen für eingehende NTP Daten!
    add_udp_app (NTP_CLIENT_PORT, (void(*)(unsigned char))ntp_get);
      
    //NTP IP aus EEPROM auslesen
    (*((unsigned long*)&ntp_server_ip[0])) = get_eeprom_value(NTP_IP_EEPROM_STORE,NTP_IP);
      
    return;
}

//----------------------------------------------------------------------------
//Handle NTP request and answer
//return 0 = o.k. 1 = Err 
unsigned char ntp (void)
{
    ntp_state = NTP_STATE_IDLE;
  
    ntp_request();

    if (ntp_state != NTP_STATE_REQ_SENT)
    {
        return(1);
    }
	gp_timer = 5; //is decremented in timer.c

    do
    {
        eth_get_data();
    }
    while ( (ntp_state == NTP_STATE_REQ_SENT) && (gp_timer > 0));
					
    if ((ntp_state != NTP_STATE_FINISHED) || (gp_timer == 0 )) return(1);
	return (0);
}


//----------------------------------------------------------------------------
//Anforderung der aktuellen Zeitinformationen von einem NTP Server
void ntp_request (void)
{
    //oeffnet eine Verbindung zu einem NTP Server
    unsigned int byte_count;
    
    //ARP Request senden
    unsigned long tmp_ip = (*(unsigned long*)&ntp_server_ip[0]);

    if (arp_request(tmp_ip) == 1)
    {  
        //Interrupt deaktivieren da Buffer zum Senden gebraucht wird!
        PGM_P ntp_data_pointer = NTP_Request;

        if ( MTU_SIZE < sizeof( NTP_Request ) )
        {
          return;
        }
        for (byte_count = 0; byte_count<sizeof( NTP_Request ); byte_count++)
        {
          unsigned char b;
          b = pgm_read_byte(ntp_data_pointer++);
          eth_buffer[UDP_DATA_START + byte_count] = b;
        }
    
        create_new_udp_packet(byte_count,NTP_CLIENT_PORT,NTP_SERVER_PORT,tmp_ip);
        NTP_DEBUG("** NTP Request gesendet! **\r\n");
		ntp_state = NTP_STATE_REQ_SENT;
        return;
    }
	ntp_state = NTP_STATE_REQ_ERR;
    NTP_DEBUG("Kein NTP Server gefunden!!\r\n");
    return;
}

//----------------------------------------------------------------------------
//Empfang der Zeitinformationen von einem NTP Server
void ntp_get (unsigned char index)
{
    struct NTP_GET_Header *ntp;
    ntp_struct dt;

    NTP_DEBUG("** NTP DATA GET! **\r\n");
        
    ntp = (struct NTP_GET_Header *)&eth_buffer[UDP_DATA_START];

    ntp->rx_timestamp  = htons32(ntp->rx_timestamp);
    ntp->rx_timestamp += GMT_TIME_CORRECTION; //  UTC +1h
    time = ntp->rx_timestamp;
      
    decode_time(time, &dt);
    NTP_DEBUG("%s, %s\r\n", dt.datestr, dt.timestr);
    if (dstr[0] == 'N') //the first time
    {
        sprintf_P(dstr, PSTR("%s, %s"), dt.datestr, dt.timestr);
    }
    ntp_state = NTP_STATE_FINISHED;
}

/************************************************************************
Beschreibung:    monthlen
Inputs: isleapyear = 0-1, month=0-11
Return:  Number of days per month
*************************************************************************/
unsigned char monthlen(unsigned char isleapyear,unsigned char month)
{
    if (month == 1)
    {
        return (28+isleapyear);
    }

    if (month > 6)
    {
        month--;
    }

    if (month %2 == 1)
    {
        return (30);
    }

    return (31);
}


/************************************************************************
Beschreibung:    decode_time
decodes the time into the datetime_t struct
Return:        
*************************************************************************/
void decode_time(unsigned long ntp_time, ntp_struct *dt)
{
    unsigned long dayclock;
    unsigned int  dayno;
    unsigned char summertime;

    dt->year = EPOCH_YR; //=1970
    dayclock = (ntp_time - GETTIMEOFDAY_TO_NTP_OFFSET) % SECS_DAY;
    dayno    = (ntp_time - GETTIMEOFDAY_TO_NTP_OFFSET) / SECS_DAY;

    dt->second = dayclock % 60UL;
    dt->minute = (dayclock % 3600UL) / 60;
    dt->hour   = dayclock / 3600UL;
    dt->wday   = (dayno + 4) % 7;      // day 0 was a thursday

    while (dayno >= YEARSIZE(dt->year))
    {
        dayno -= YEARSIZE(dt->year);
        dt->year++;
    }

    dt->month = 0;
    while (dayno >= monthlen(LEAPYEAR(dt->year), dt->month))
    {
        dayno -= monthlen(LEAPYEAR(dt->year), dt->month);
        dt->month++;
    }
    dt->month ++;
    dt->day  = dayno+1;

    // Summertime
    summertime = 1;
    if (dt->month < 3 || dt->month > 10)     // month 1, 2, 11, 12
    {
        summertime = 0;                          // -> Winter
    }

    if ((dt->day - dt->wday >= 25) && (dt->wday || dt->hour >= 2))
    {                              // after last Sunday 2:00
        if (dt->month == 10)        // October -> Winter
        {
            summertime = 0;
        }
    }
    else
    {                              // before last Sunday 2:00
        if (dt->month == 3)        // March -> Winter
        {
            summertime = 0;
        }
    }

    if (summertime)
    {
        dt->hour++;              // add one hour
        if (dt->hour == 24)
        {                        // next day
            dt->hour = 0;
            dt->wday++;            // next weekday
            if (dt->wday == 7 )
            {
                dt->wday = 0;
            }
            if (dt->day == monthlen(LEAPYEAR(dt->year), dt->month))
            {                // next month
                dt->day = 0;
                dt->month++;
            }
            dt->day++;
        }
    }
    
    strcpy_P(dt->datestr, &wday_str[dt->wday*3]);
    sprintf_P(dt->datestr+2, PSTR(" %u.%02u.%04u"), dt->day, dt->month, dt->year);
    sprintf_P(dt->timestr, PSTR("%2u:%02u:%02u"), dt->hour, dt->minute, dt->second);
}

#endif //USE_NTP






