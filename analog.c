/*------------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        31.12.2007
 Description:    Analogeing�nge Abfragen
 
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
#include "analog.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "httpd.h" //f�r Variablen Array

#if USE_ADC
volatile unsigned char channel = 0;

//------------------------------------------------------------------------------
//
void ADC_Init(void)
{ 
	ADMUX = (1<<REFS0);
	//Free Running Mode, Division Factor 128, Interrupt on
	ADCSRA=(1<<ADEN)|(1<<ADSC)|(1<<ADATE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADIE);
}

//------------------------------------------------------------------------------
//
ISR (ADC_vect)
{
    ANALOG_OFF; //ADC OFF
	var_array[channel++] = ADC;
	//usart_write("Kanal(%i)=%i\n\r",(channel-1),var_array[(channel-1)]);
	if (channel > 7) channel = 0;
    ADMUX =(1<<REFS0) + channel;
    //ANALOG_ON;//ADC ON
}

#endif //USE_ADC


