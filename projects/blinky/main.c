#include <avr/io.h>
#include <util/delay.h>

#define LED_Bit     5
#define LED_Port    PORTB
#define LED_DDR     DDRB

#define GPIO_Set(port, bit)  (port) |= _BV((bit))
#define GPIO_Clr(port, bit)  (port) &= ~_BV((bit))
#define GPIO_Tgl(port, bit)  (port) ^= _BV((bit))

int main(void)
{
    LED_DDR |= _BV(LED_Bit);

    while (1) {
        GPIO_Set(LED_Port, LED_Bit);
        _delay_ms(500);
        GPIO_Clr(LED_Port, LED_Bit);
        _delay_ms(500);
    }
}


