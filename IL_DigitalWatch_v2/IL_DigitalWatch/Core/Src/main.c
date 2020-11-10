 /*
 * IL_DigitalWatch.c
 *
 * Created: 10/22/2020 1:26:29 PM
 * Author : gints.dreifogels
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>


#include "atmega328p_gpio.h"


int main(void)
{
	Init_Ports();
	Init_TC0();
	Init_TC1();
	Init_USART();

	sei ();
    
    while (1) 
    {
    }
}


