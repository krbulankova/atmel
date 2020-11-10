/*
 * atmega328P_gpio.c
 *
 * Created: 10.11.2020 12:43:02
 *  Author: My pc
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include "atmega328p_gpio.h"

#define d0 0b01111110
#define d1 0b00001100
#define d2 0b10110110
#define d3 0b10011110
#define d4 0b11001100
#define d5 0b11011010
#define d6 0b11111010
#define d7 0b00001110
#define d8 0b11111110
#define d9 0b11011110

#define dt0 0b00000001  // 0x01, 1. tranzistors:	PC0 // dt: digit-transistor
#define dt1 0b00000010  // 0x02, 2. tranzistors:	PC1
#define dt2 0b00000100  // 0x04, 3. tranzistors:	PC2
#define dt3 0b00001000  // 0x08, 4. tranzistors:	PC3



uint8_t Display_mm_ss [4] = {d0, d1, d2, d3};
uint8_t Display_hh_mm [4] = {0, 0, 0, 0};
uint8_t DisplayDigitHolder [10] = {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9}; // priekš displeja ciparu att?lošanas
uint8_t DisplayDigitScan [4] = {dt3, dt2, dt1, dt0}; // priekš displeja izvadu multipleks?šanas
uint8_t DigitUpdate [6];
uint8_t TC0_i = 0;

uint8_t secOnes = 0, secTens = 0, minOnes = 0, minTens = 0, hourOnes = 0, hourTens = 0;
uint8_t modeCheck = 'm';
//uint8_t USARTReadBuffer = 0;
uint8_t bufferSize = 15;
char USARTReadBuffer [15]= "";
uint8_t URB_CNT = 0, i = 0;


void Init_Ports()
{
	DDRD = 0b11111110;		// DDRD - porta D datu virziena re?istrs
							// porta D izvadi PD1 l?dz PD7 konfigur?ti darb?bai uz izvadi
							// priekš displeja segmentiem a l?dz g,
							// kur PD1-a, PD2-b, PD3-c, PD4-d, PD5-e, PD6-g, PD7-g
							// savuk?rt PD0: darb?bai uz ievadi, priekš USART RX (seri?l? komunik?cija)
	
	DDRC = 0x1F;			// DDRD - porta C datu virziena re?istrs,ACTIVIZETI IZVADI:PC0, PC1,PC2, PC3, PC4
							// izvadi PC0 l?dz PC3 konfigur?ti darb?bai uz izvadi
	DDRB = 0x2F;
							//DDRB = 0x20;            // 0b0010_0000, porta B 5. izvads darb?bai uz izvadi - t? ir lietot?ja gaismas diode uz ATmega328P Xplained Mini plates
	PORTB |= (1 << PORTB5);
}

void Init_TC0()
{
	TCNT0 = 0;
	OCR0A = 30;					// lai nodrošin?tu displeja kop?jo atjaunošan?s laiku aptuveni 8 ms
								// ik nepilnas 2 ms iesl?dzot/izsl?dzot katru no displeja cipariem
	
	TIMSK0 |= (1 << OCIE0A);	// timer/counter mask Register 0
								// OCIE0A: output compare interrupt enable 0
			
	TCCR0B |= (1 << CS02) | (1 << CS00);
	TCCR0A |= (1 << WGM01);
}

void Init_TC1()
{
	TCNT1 = 0;
	OCR1A = 15625;		// lai TC0 p?rtraukums izpild?tos katru 1 sekundi
	
	TIMSK1 |= (1 << OCIE1A);
	TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);
}

void Init_USART()
{
	UBRR0 = 103;							 // lai nodrošin?tu baud rate = 9600
	
	UCSR0B = (1 << RXEN0) | (1 << RXCIE0);	 // lai aktiviz?tu MCU perif?rijas USART uztveroš?s (RX) da?as bloku
	UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);  // lai nodrošin?tu datu kadra izm?ru: 8 biti
}


void commandCheck()
{
	if (strcmp (USARTReadBuffer, "#CRST*") == 0)
	{
		secOnes = 0;
		secTens = 0;
		minOnes = 0;
		minTens = 0;
		hourOnes = 0;
		hourTens = 0;
	}
	
	else if (strcmp (USARTReadBuffer, "#CSTOP*") == 0)
	{
		TC1_Stop();
	}
	
	else if (strcmp (USARTReadBuffer, "#CSTART*") == 0)
	{
		TC1_Start();
	}
	
	else if (strcmp (USARTReadBuffer, "#MODE1*") == 0)
	{
		modeCheck = 'm';
	}
	
	else if (strcmp (USARTReadBuffer, "#MODE2*") == 0)
	{
		modeCheck = 'h';
	}
	
	
	else if (USARTReadBuffer [0] == '#' && USARTReadBuffer [1] == 'C' && USARTReadBuffer [2] == 'C' && USARTReadBuffer [3] == 'H' && USARTReadBuffer [4] == 'T' && USARTReadBuffer [5] == '=' && USARTReadBuffer [12] == '*')
	{//#CCHT=121212* Uzst?da vajadz?go laiku
		hourTens = USARTReadBuffer [6] -'0';//no ASCII at?em ASCII 0,kur '0' = 48
		hourOnes = USARTReadBuffer [7] -'0';
		minTens = USARTReadBuffer [8] -'0';
		minOnes = USARTReadBuffer [9] -'0';
		secTens = USARTReadBuffer [10] -'0';
		secOnes = USARTReadBuffer [11] -'0';
		
	}
	
	
	//*USARTReadBuffer = '\0';
}
void TC1_Stop ()
{
	TCCR1B &= ~((1 << CS12) | (1 << CS10));
}
void TC1_Start ()
{
	TCCR1B |= (1 << CS12) | (1 << CS10);
}

// Как запрошено в задании 2.1 вынимаем тело вычислений времени в отдельную функцию
void Clock_Count_Up() {
	// счётчики устроены поциферно [0..9]
	
	secOnes++; // Увеличиваем счётсчик секунд на 1 (прошла же одна секунда?)
	
	// если счётчик единиц для секунд переваливает за 9, это уже десятки.. увеличиваем десяток
	//  на 1, а единицу возвращаем обратно в 0, например
	// seconds 	seccTens	secOnes		'human' seconds
	// ...		...		...		...
	// n		8		8		88
	// n+1		8		8+1=9		89
	// n+2		8		9+1=10		9(10)
	//		|		|		|
	//		v		v		v
	//		8+1=9		0		90
	if (secOnes > 9)
	{
		secOnes = 0;
		secTens++;
	}
	
	// если счётчик десяток для секунд переваливает за 5 (60), то уже +минута
	if (secTens > 5)
	{
		secTens = 0;
		minOnes++;
	}
	
	// анадогично для минут
	if (minOnes > 9)
	{
		minOnes = 0;
		minTens++;
	}
	
	if (minTens > 5)
	{
		minTens = 0;
		hourOnes++;
	}
	
	if (hourOnes > 9)
	{
		hourOnes = 0;
		hourTens++;
	}
	
	// каждые 42 часа счётчик сбрасывается (не у меня спрашивай, зачем)
	if (hourTens >=4 && hourOnes >=2)
	{
		secOnes = 0;
		secTens = 0;
		minOnes = 0;
		minTens = 0;
		hourOnes = 0;
		hourTens = 0;
	}
}

// Тело данной препроцессорной функции вызывается, как тут сказано, раз в секунду
ISR (TIMER1_COMPA_vect)			
{
	// notiks p?reja uz šo p?rtraukuma apstr?des proced?ru, tad kad TCNT1 = OCR1A jeb TCNT1 = 15625, jo š? v?rt?ba ir ierakst?ta
	// ar? re?istr? OCR1A (OCR1A = 15625) ISR izpild?sies ik p?c vienas sekundes izmanto sekunžu skait?šanai
	
	Clock_Count_Up();
	
	DigitUpdate [0] = DisplayDigitHolder [secOnes];
	DigitUpdate [1] = DisplayDigitHolder [secTens];
	DigitUpdate [2] = DisplayDigitHolder [minOnes];
	DigitUpdate [3] = DisplayDigitHolder [minTens];
	DigitUpdate [4] = DisplayDigitHolder [hourOnes];
	DigitUpdate [5] = DisplayDigitHolder [hourTens];
	
	Display_mm_ss [0] = DigitUpdate [0];
	Display_mm_ss [1] = DigitUpdate [1];
	Display_mm_ss [2] = DigitUpdate [2];
	Display_mm_ss [3] = DigitUpdate [3];
	
	Display_hh_mm [0] = DigitUpdate [2];
	Display_hh_mm [1] = DigitUpdate [3];
	Display_hh_mm [2] = DigitUpdate [4];
	Display_hh_mm [3] = DigitUpdate [5];
	
	PORTC ^= ( 1<< PORTC4);//PORTC4 jev pc4
}

ISR (TIMER0_COMPA_vect)
{
	uint8_t temp;
	
// notiks p?reja uz šo p?rtraukuma apstr?des proced?ru, tad kad TCNT0 = OCR0A jeb TCNT0 = 30, jo š? v?rt?ba ir ierakst?ta
// ar? re?istr? OCR1A (OCR1A = 30) ISR izpild?sies ik p?c aptveni 2 ms izmanto displeja kop?go izvadu (kur piesl?gti tranzsitori) multipleks?šanai
	
	if (modeCheck == 'm')
	{
		PORTD = Display_mm_ss [TC0_i];
	}
	
	if (modeCheck == 'h')
	{
		PORTD = Display_hh_mm [TC0_i];
	}
	temp = PINC;//porta las?šanas re?istrs
	temp = temp & 0x10;
		
	
	//PORTB = DisplayDigitScan [TC0_i];
	PORTC = DisplayDigitScan [TC0_i] | temp;
	TC0_i++;
	
	if (TC0_i == 4)
	{
		TC0_i = 0;
	}
}

ISR (USART_RX_vect)
{
								// notiks p?reja uz šo p?rtraukuma apstr?des proced?ru, tad
								// seri?l?s perif?rijas USART datu sa?emšanas re?istr? UDR (USART Data Register)
								// tiks sa?emts baits (piem?ram, nos?tot no datora simbolu, izmantojot klaviat?ru)
	/*USARTReadBuffer = UDR0;
	
	if (USARTReadBuffer == 'm')
	{
		modeCheck = 'm';
	}
	
	if (USARTReadBuffer == 'h')
	{
		modeCheck = 'h';
	}
	*/

	
	USARTReadBuffer [URB_CNT] = UDR0;
	
	if (USARTReadBuffer [0] != '#')
	{
		URB_CNT = 255;
		
		for (uint8_t i = 0; i < bufferSize; i++)
		{
			USARTReadBuffer [i] = 0;
		}
	}
	
	else if (USARTReadBuffer [URB_CNT] == '*')
	{
		USARTReadBuffer [URB_CNT + 1] = '\0';
		URB_CNT = 255;
		commandCheck ();	
		
		/*
		for (uint8_t i = 0; i < bufferSize; i++)
		{
			USARTReadBuffer [i] = 0;
		}
		*/
	
	}
	
	URB_CNT++;

}
