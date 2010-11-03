/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        12.11.2007
 Description:    HTTP-Client (empfang einer Webseite)

 Dieses Programm ist freie Software. Sie k�nnen es unter den Bedingungen der 
 GNU General Public License, wie von der Free Software Foundation ver�ffentlicht, 
 weitergeben und/oder modifizieren, entweder gem�� Version 2 der Lizenz oder 
 (nach Ihrer Option) jeder sp�teren Version. 

 Die Ver�ffentlichung dieses Programms erfolgt in der Hoffnung, 
 da� es Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, 
 sogar ohne die implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT 
 F�R EINEN BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License. 

 Sie sollten eine Kopie der GNU General Public License zusammen mit diesem 
 Programm erhalten haben. 
 Falls nicht, schreiben Sie an die Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA. 
------------------------------------------------------------------------------*/
#include "config.h"
#include "http_get.h" 
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "stack.h"
#include "usart.h"
#include "timer.h"
	
PROGMEM char WEATHER_GET_STRING[] = {"GET /globalweather.asmx/GetWeather?CityName=Paderborn&CountryName=Germany HTTP/1.1\r\n"
                                     //"GET / HTTP/1.1\r\n"
                                     "Host: www.webservicex.net\r\n"
                                     //"Host: www.ulrichradig.de\r\n"
                                     "Keep-Alive: 300\r\n"
                                     "Connection: Keep-Alive\r\n\r\n"};

volatile unsigned int http_get_state = 0;

//----------------------------------------------------------------------------
//Daten kommen von einem Webserver an!!
void test (unsigned char index)
{
	for (int a = TCP_DATA_START_VAR;a < TCP_DATA_END_VAR;a++)
	{
		HTTPC_DEBUG("%c",eth_buffer[a]);
	}
	tcp_entry[index].time = TCP_TIME_OFF;
	tcp_entry[index].status = ACK_FLAG;
	create_new_tcp_packet(0,index);
}

//----------------------------------------------------------------------------
//HTTP Request an einen Webserver stelle
void http_request (void)
{
    unsigned long index = MAX_TCP_ENTRY;
    
    if (http_get_state > 1 && http_get_state < 20) http_get_state++;

    if (http_get_state == 0)
    {
        //offnet eine Verbindung zu meinem Webserver
        HTTPC_DEBUG("ARP Request\n\r");
        unsigned int my_http_cp = 2354;
        add_tcp_app (my_http_cp, (void(*)(unsigned char))test);
        
        //ARP Request senden
        if(arp_request (WEATHER_SERVER_IP))
        {
            for(unsigned long a=0;a<2000000;a++){asm("nop");};
            
            tcp_port_open (WEATHER_SERVER_IP,HTONS(80),HTONS(my_http_cp));
               
            unsigned char tmp_counter = 0;
            while((index >= MAX_ARP_ENTRY) && (tcp_entry[index].app_status != 1))
            {
                index = tcp_entry_search (WEATHER_SERVER_IP,HTONS(80));
                if (tmp_counter++ > 30)
                {
                    HTTPC_DEBUG("TCP Eintrag nicht gefunden (HTTP_CLIENT)!\r\n");
                    return;
                }
            }
             
            HTTPC_DEBUG("TCP Eintrag gefunden (HTTP_CLIENT)!\r\n");
            tcp_entry[index].first_ack = 1;
            http_get_state = 2;
        }
        else
        {
            http_get_state = 1;
        }
    }
    
    //if (http_get_state == 10)  
    if (http_get_state > 10 && http_get_state < 20)
    {
        HTTPC_DEBUG("\r\n\r\n\r\nDaten Anfordern\r\n");
        index = tcp_entry_search (WEATHER_SERVER_IP,HTONS(80));
        memcpy_P(&eth_buffer[TCP_DATA_START],WEATHER_GET_STRING,(sizeof(WEATHER_GET_STRING)-1));
        tcp_entry[index].status =  ACK_FLAG | PSH_FLAG;
        create_new_tcp_packet((sizeof(WEATHER_GET_STRING)-1),index);
    }
}




