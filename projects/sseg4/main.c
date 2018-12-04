#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


#define LED_Bit     5
#define LED_Port    PORTB
#define LED_DDR     DDRB

#define SCLK        4
#define RCLK        3
#define DIO         2
#define SSEG_Port   PORTB
#define SSEG_DDR    DDRB

#define DIG_DOT   DIG[16]
#define DIG_BAR   DIG[17]
#define DIG_OFF   DIG[18]

unsigned char DIG[] = {
0xC0,0xF9,0xA4,0xB0,	// 0, 1, 2, 3
0x99,0x92,0x82,0xF8,	// 4, 5, 6, 7
0x80,0x90,0x88,0x83,	// 8, 9, A, b
0xC6,0xA1,0x86,0x8E,    // C, d, E, F 
0x7F,					// - (bar)
0xBF,					// . (d0t)
0xff					//   (blank)
};

#define GPIO_Set(port, bit)  (port) |= _BV((bit))
#define GPIO_Clr(port, bit)  (port) &= ~_BV((bit))
#define GPIO_Tgl(port, bit)  (port) ^= _BV((bit))

void SSEG_Show(uint8_t val, uint8_t pos);
void SSEG_Send(uint8_t bits);

uint8_t SSEG_Buffer[4];

uint8_t SSEG_Buffer_Index = 0;

static volatile uint32_t TICK = 0;

ISR(TIMER0_COMPA_vect)
{
	TICK++;
	SSEG_Show(SSEG_Buffer[SSEG_Buffer_Index], 1<<SSEG_Buffer_Index);
	SSEG_Buffer_Index++;
	if (SSEG_Buffer_Index>=4) {
		SSEG_Buffer_Index = 0;
	}
}

uint32_t TICK_Get(void)
{
	volatile uint32_t tmp;
	cli();
	tmp = TICK;
	sei();
	return tmp;
}

void SSEG_Init(void) 
{
	SSEG_Buffer[0] = DIG[0];
	SSEG_Buffer[1] = DIG_OFF;
	SSEG_Buffer[2] = DIG_OFF;
	SSEG_Buffer[3] = DIG_OFF;
}
void SSEG_Show(uint8_t val, uint8_t pos)
{
    SSEG_Send(val);
    SSEG_Send(pos);
    GPIO_Clr(SSEG_Port, RCLK);
    GPIO_Set(SSEG_Port, RCLK);
}

void SSEG_Send(uint8_t bits)
{
    int i;
    for (i=0; i<8; i++) {
        if (bits & 0x80) {
            GPIO_Set(SSEG_Port, DIO);
        }
        else {
            GPIO_Clr(SSEG_Port, DIO);
        }
        bits <<= 0x1;
        GPIO_Clr(SSEG_Port, SCLK);
        GPIO_Set(SSEG_Port, SCLK);
    }
}


int main(void)
{
	uint32_t old_tick = 0;
	uint32_t tick;
	uint8_t n;

	cli();

    GPIO_Clr(SSEG_Port, SCLK);
    GPIO_Clr(SSEG_Port, RCLK);
    GPIO_Clr(SSEG_Port, DIO);
    SSEG_DDR |= _BV(SCLK);
    SSEG_DDR |= _BV(RCLK);
    SSEG_DDR |= _BV(DIO);

	TCCR0A |= (1 << WGM01);
    OCR0A = 0xf9;   // 249
    TIMSK0 = (1 << OCIE0A);

    sei();

    TCCR0B = (1<<CS01) | (1<<CS00); // 1/64 prescaling, 16 MHz/64 == 250

	SSEG_Init();

	n = 0;
    while (1) {
#if 1
		tick = TICK_Get();
		if (tick - old_tick>=200) {
			
			old_tick = tick;

			SSEG_Buffer[0] = DIG[n];
			n++;
			if (n > 19) {
				n = 0;
			}
		}
#endif
	}
}


