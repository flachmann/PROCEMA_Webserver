/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:
 known Problems: none
 Version:        24.10.2007
 Description:    Timer Routinen

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
#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart.h"
#include "stack.h"
#include "ntp.h"
#include "cmd.h"
#include "dhcpc.h"
#include "timer.h"

volatile unsigned long time;
volatile unsigned long time_watchdog = 0;

//----------------------------------------------------------------------------
//Diese Routine startet und inizialisiert den Timer
void timer_init (void)
{
	#if EXTCLOCK==1
		#if defined (__AVR_ATmega644__)
			//Asynchroner Modus ein, Oszillator an TOSC1 und TOSC2 aktiv
			ASSR |= (1<<AS2);
			TCCR2B = 0x05;
			while(ASSR & 0x11);
			//Capture/Compare-Interrupt aktiv
			TIMSK2 |= (1<<OCIE2A);
		#else
			//Asynchroner Modus ein, Oszillator an TOSC1 und TOSC2 aktiv
			ASSR  = (1<<AS2);
			//CTC-Modus an (Clear Timer on Compare Match)
			TCCR2 = (1<<WGM21);			
			//dieser Wert ergibt eine Sekunde Periodendauer
			OCR2  = 31;	
			//l�sche Prescaler 2				
			SFIE  = (1<<PSR2);			
			//Starte Timer 2 mit Prescaler gleich 1/1024
			TCCR2 |= (1<<CS22)|(1<<CS21)|(1<<CS20); 
			while(ASSR & 0x07);
			//Capture/Compare-Interrupt aktiv
			TIMSK = (1<<OCIE2);			
		#endif
	#else
			TCCR1B |= (1<<WGM12) | (1<<CS10 | 0<<CS11 | 1<<CS12);
			TCNT1 = 0;
			OCR1A = (F_CPU / 1024) - 1;
			TIMSK |= (1 << OCIE1A);
	#endif
return;
};

//----------------------------------------------------------------------------
//Timer Interrupt
#if EXTCLOCK==1
	#if defined (__AVR_ATmega644__)
    ISR (TIMER2_COMPA_vect)
	#else
    ISR (TIMER2_COMP_vect)
	#endif
#else
	ISR (TIMER1_COMPA_vect)
#endif
{
	//tick 1 second
	time++;
    if((stack_watchdog++) > WTT)  //emergency reset of the stack
    {
        RESET();
	}
    eth.timer = 1;
	#if USE_NTP
	ntp_timer--;
	#endif //USE_NTP
	#if USE_DHCP
	if ( dhcp_lease > 0 ) dhcp_lease--;
    if ( gp_timer   > 0 ) gp_timer--;
    #endif //USE_DHCP
}
