/*
 *
 * This example is configured for a Atmega32 at 16MHz
 */ 

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "light_ws2812.h"

#define MAXPIX 8

struct cRGB led[MAXPIX];

int main(void)
{
	uint8_t i;

	DDRB|=_BV(ws2812_pin);

    for (i=0; i<MAXPIX; i++){
        led[i].r = 0;
        led[i].g = 0;
        led[i].b = 0;
    }
    
    ws2812_sendarray((uint8_t *)led,MAXPIX*3);
    
    _delay_ms(250);

    led[3].g = 60;
    led[3].b = 70;

    ws2812_sendarray((uint8_t *)led,MAXPIX*3);
		 
	while(1); 
}
