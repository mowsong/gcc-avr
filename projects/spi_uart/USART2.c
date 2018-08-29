/*****************************************************************************
*
* Atmel Corporation
*
* File              : USART2.c
* Compiler          : IAR EWAAVR 2.28a/3.10c
* Revision          : $Revision: 1.3 $
* Date              : $Date: 17. mars 2004 14:47:16 $
* Updated by        : $Author: ltwa $
*
* Support mail      : avr@atmel.com
*
* Supported devices : All devices with a SPI and USART module can be used.
*                     The example is written for the ATmega8
*
* AppNote           : AVR303 - SPI-UART Gateway
*
// Author           : Andy Gayne. avr@gayne.co.uk   www.gd-technik.com
// Description      : Routines for interrupt controlled USART
****************************************************************************/

/* Includes */
#include "USART2.h"

/* Static Variables */
static unsigned char USART_RxBuf[USART_RX_BUFFER_SIZE] = {0};
static volatile unsigned char USART_RxHead;
static volatile unsigned char USART_RxTail;
static unsigned char USART_TxBuf[USART_TX_BUFFER_SIZE] = {0};
static volatile unsigned char USART_TxHead;
static volatile unsigned char USART_TxTail;

/* Initialize USART */
void USART_Init( unsigned int baudrate )
{
	unsigned char x;
    uint16_t baud_setting = (F_CPU / 4 / baudrate - 1) / 2;
    
    UCSR0A = 1 << U2X0;

    if (((F_CPU == 16000000UL) && (baudrate == 57600)) || (baud_setting > 4095)) {
        UCSR0A = 0;
        baud_setting = (F_CPU / 8 / baudrate - 1) / 2;
    }   
	/* Set the baud rate */
    UBRR0H = baud_setting >> 8;
    UBRR0L = baud_setting; 

	/* Set frame format: 8 data, no parity, 1 stop */
	UCSR0C = (0<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);
	
    /* Enable UART receiver and transmitter */
	UCSR0B = ( ( 1 << RXCIE0 ) | ( 1 << RXEN0 ) | ( 1 << TXEN0 ) );
	
	/* Flush receive buffer */
	x = 0; 			    

	USART_RxTail = x;
	USART_RxHead = x;
	USART_TxTail = x;
	USART_TxHead = x;
}

/* Interrupt handlers */
ISR(USART_RX_vect)
{
	unsigned char data;
	unsigned char tmphead;

	/* Read the received data */
	data = UDR0;                 
	/* Calculate buffer index */
	tmphead = ( USART_RxHead + 1 ) & USART_RX_BUFFER_MASK;
	USART_RxHead = tmphead;      /* Store new index */

	if ( tmphead == USART_RxTail )
	{
		/* ERROR! Receive buffer overflow */
	}
	
	USART_RxBuf[tmphead] = data; /* Store received data in buffer */
}

ISR(USART_UDRE_vect)
{
	unsigned char tmptail;
    
	/* Check if all data is transmitted */
	if ( USART_TxHead != USART_TxTail )
	{
		/* Calculate buffer index */
		tmptail = ( USART_TxTail + 1 ) & USART_TX_BUFFER_MASK;
		USART_TxTail = tmptail;      /* Store new index */
	
		UDR0 = USART_TxBuf[tmptail];  /* Start transmition */
	}
	else
	{
		UCSR0B &= ~(1<<UDRIE0);         /* Disable UDRE interrupt */
	}
}

/* Read and write functions */
unsigned char USART_Receive( void )
{
	unsigned char tmptail;
	
	while ( USART_RxHead == USART_RxTail );  /* Wait for incomming data */
		
	tmptail = ( USART_RxTail + 1 ) & USART_RX_BUFFER_MASK;/* Calculate buffer index */
	
	USART_RxTail = tmptail;                /* Store new index */
	
	return USART_RxBuf[tmptail];  /* Return data */
}

void USART_Transmit( unsigned char data )
{
	unsigned char tmphead;
	/* Calculate buffer index */
	tmphead = ( USART_TxHead + 1 ) & USART_TX_BUFFER_MASK; /* Wait for free space in buffer */
	while ( tmphead == USART_TxTail );

	USART_TxBuf[tmphead] = data;           /* Store data in buffer */
	USART_TxHead = tmphead;                /* Store new index */

	UCSR0B |= (1<<UDRIE0);                    /* Enable UDRE interrupt */
}

unsigned char DataInReceiveBuffer( void )
{
	return ( USART_RxHead != USART_RxTail ); /* Return 0 (FALSE) if the receive buffer is empty */
}
