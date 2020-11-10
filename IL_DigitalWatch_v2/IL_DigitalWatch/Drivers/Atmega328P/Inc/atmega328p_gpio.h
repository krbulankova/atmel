/*
 * atmega328p_gpio.h
 *
 * Created: 10.11.2020 12:43:52
 *  Author: My pc
 */ 


#ifndef ATMEGA328P_GPIO_H_
#define ATMEGA328P_GPIO_H_

#include <avr/io.h>

void Init_Ports();
void Init_TC0();			// priekš taimera/skait?t?ja 0 (8 bitu TC)
void Init_TC1();			// priekš taimera/skait?t?ja 1 (16 bitu TC)
void Init_USART();
void commandCheck ();
void TC1_Stop ();
void TC1_Start ();


#endif /* ATMEGA328P_GPIO_H_ */