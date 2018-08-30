#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/* 
* https://sites.google.com/site/qeewiki/books/avr-guide/timers-on-the-atmega328
*/
static volatile uint16_t TICK = 0;

ISR(TIMER0_COMPA_vect)
{
    if (TICK++ >= 500) {
        TICK = 0;
        PORTB ^= (1 << PORTB5);
    }
}

int main(void)
{
    DDRB |= (1 << DDB5);
    PORTB |= (1 << PORTB5);

    cli();

    TCCR0A |= (1 << WGM01);
    OCR0A = 0xf9;   // 249
    TIMSK0 = (1 << OCIE0A);

    sei();

    TCCR0B = (1<<CS01) | (1<<CS00); // 1/64 prescaling, 16 MHz/64 == 250
    
    while(1);
}


