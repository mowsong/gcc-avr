#include "avr/io.h"
#include "util/delay.h"
#include "avr/interrupt.h"
#include "avr/pgmspace.h"

#define LED_Bit     5
#define LED_Port    PORTB
#define LED_DDR     DDRB

#define GPIO_Set(port, bit)  (port) |= _BV((bit))
#define GPIO_Clr(port, bit)  (port) &= ~_BV((bit))
#define GPIO_Tgl(port, bit)  (port) ^= _BV((bit))

static volatile uint16_t TICK = 0;

// string in flash
const uint8_t pgm_hello[] PROGMEM = "Hello, AVR\r\n";
const uint8_t pgm_test[] PROGMEM = "This is a test\r\n";

// string in sram
const uint8_t hello[] = "Hello, AVR in SRAM\r\n";
const uint8_t test[] = "This is a test in SRAM\r\n";

ISR(TIMER0_COMPA_vect)
{
    if (TICK++ >= 500) {
        TICK = 0;
        GPIO_Tgl(LED_Port, LED_Bit);
    }
}

void TIMER0_Init(uint8_t match_value)
{
    TCCR0A = (1<<WGM01);    // CTC mode
    OCR0A  = match_value;
    TCNT0  = 0;
    TIMSK0 = (1<<OCIE0A);
}

void TIMER0_Start(void)
{
    TCCR0B = (1<<CS01) | (1<<CS00);
}

void USART_Init(unsigned long baud, uint8_t config)
{
    uint16_t baud_setting = (F_CPU / 4 / baud - 1) / 2;
    UCSR0A = 1 << U2X0;

    if (((F_CPU == 16000000UL) && (baud == 57600)) || (baud_setting > 4095)) {
        UCSR0A = 0;
        baud_setting = (F_CPU / 8 / baud - 1) / 2;
    }
    UBRR0H = baud_setting >> 8;
    UBRR0L = baud_setting;  

    UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
    UCSR0B = (1<<RXEN0) | (1<<TXEN0);
}

void USART_Transmit(uint8_t data)
{
    while ( !(UCSR0A & (1<<UDRE0)) );

    UDR0 = data;
}

void USART_PrintString(uint8_t *msg)
{
    while (*msg) {
        USART_Transmit(*msg++);
    }
}

void USART_PrintString_P(const uint8_t *msg)
{
    uint8_t c;

    c = pgm_read_byte(msg++);
    while (c) {
        USART_Transmit(c);
        c = pgm_read_byte(msg++);
    }
}


int main(void)
{
    uint8_t *p;

    cli();

    LED_DDR |= _BV(LED_Bit);

    USART_Init(9600, 0);

    TIMER0_Init(249);
    TIMER0_Start();

    sei();

    while (1) { 
        USART_PrintString_P(pgm_hello);
        _delay_ms(100);
        USART_PrintString(hello);
        _delay_ms(100);
        USART_PrintString_P(pgm_test);
        _delay_ms(100);
        USART_PrintString(test);
        _delay_ms(100);
    }
}




