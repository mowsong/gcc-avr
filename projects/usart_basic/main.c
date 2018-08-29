#include "avr/io.h"
#include "util/delay.h"
#include "avr/pgmspace.h"

#define LED_Bit     5
#define LED_Port    PORTB
#define LED_DDR     DDRB

#define GPIO_Set(port, bit)  (port) |= _BV((bit))
#define GPIO_Clr(port, bit)  (port) &= ~_BV((bit))
#define GPIO_Tgl(port, bit)  (port) ^= _BV((bit))

void Serial_Init(unsigned long baud, uint8_t config)
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

void Serial_Transmit(uint8_t data)
{
    while ( !(UCSR0A & (1<<UDRE0)) );

    UDR0 = data;
}


void Serial_PrintString(uint8_t *msg)
{
    while (*msg) {
        Serial_Transmit(*msg++);
    }
}

const uint8_t str_hello[] = "Hello, AVR\r\n";
const uint8_t str_test[] = "This is a test\r\n";

int main(void)
{
    uint8_t *p;

    LED_DDR |= _BV(LED_Bit);

    Serial_Init(9600, 0);

    while (1) {
        GPIO_Set(LED_Port, LED_Bit);
        Serial_PrintString(str_hello);
        _delay_ms(100);
        GPIO_Clr(LED_Port, LED_Bit);
        Serial_PrintString(str_test);
        _delay_ms(500);
    }
}


