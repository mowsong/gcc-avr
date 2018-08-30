#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    DDRD = 0xFF;
    
    while (1) {
        PORTD = 0x0;
        _delay_ms(50);
        PORTD = 0xff;
        _delay_ms(950);
    }
}

