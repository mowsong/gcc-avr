#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>

static volatile uint16_t TICK = 0;

#define PORT_SPI    PORTB
#define DDR_SPI     DDRB

#define P_MOSI  PORTB3
#define P_SCK   PORTB5
#define P_SS    PORTB2

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

void SPI_MasterInit(void)
{
  // Set MOSI, SCK and SS output, all others input
  DDR_SPI = (1 << P_MOSI) | (1 << P_SCK) | (1 << P_SS);
  // make SS output high
  PORT_SPI |= (1 << P_SS); 
  // Enable SPI, Master, set clock rate fck/16
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

char SPI_MasterTransmit(char cData)
{
  // Start transmission
  SPDR = cData;
  // Wait for transmission complete
  while(!(SPSR & (1 << SPIF)));
  return (SPDR);
}

int main(void)
{
    USART_Init(9600, 0);
    SPI_MasterInit();

    while(1) {
        SPI_MasterTransmit(0x55);
        USART_PrintString("Hello\r\n");
        _delay_ms(500); 
        USART_PrintString("AVR\r\n");
        _delay_ms(500); 
    }
}


