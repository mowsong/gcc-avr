/*****************************************************************************
* Ported to GCC by ngms
* 2018-09-18
*
* Target is the Arduino Uno, ATmega328p @ 16MHz
*****************************************************************************/
 
/*****************************************************************************
*
* Atmel Corporation
*
* File              : main.c
* Compiler          : IAR EWAAVR 2.28a/3.10c
* Revision          : $Revision: 1.3 $
* Date              : $Date: 17. mars 2004 14:47:06 $
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
// Platform         : Developed on STK500. Can be used stand-alone.
// Description      : 
// This program provides a gateway between the Mega8 Master SPI port and an 
// RS232C ASCII terminal, for example a terminal program running on a PC. The 
// terminal can then be used to configure the port and send/receive data via
// SPI, allowing interactive communication with, and debugging of, SPI slave
// devices.
****************************************************************************/

#include "main.h"

#define __DEBUG_EEPROM__  0

void printhex(char c);

// string literals
const char crlf[] PROGMEM = "\r\n\007";
const char backspace[] PROGMEM = "\b \b";
const char exchange[] PROGMEM = " sent,  received=";
const char prompt[] PROGMEM = "\r\n[H]-help >";
const char nullprompt[] PROGMEM = "\r\n          ";
const char error1[] PROGMEM = " [illegal character entered]";
const char newstat[] PROGMEM = "\r\nNew";
const char bit0[] PROGMEM = " (MSB first)";
const char bit1[] PROGMEM = " (LSB first)";
const char auto0[] PROGMEM = " (pulse low)";
const char auto1[] PROGMEM = " (pulse high)";
const char auto2[] PROGMEM = " (off)";
const char gap[] PROGMEM = "   ";
const char cfgwrite[] PROGMEM = " configuration saved";
const char config[] PROGMEM = "Configuration loaded -";
const char noconfig[] PROGMEM = "No configuration stored, defaults used\007";

const char  status[][MAXSTATSTRLEN] PROGMEM = 
{ "Status: ",
  "SCK=",
  "SS=",
  "Auto=",
  "Bit Order=",
  "Mode="
};

const char  help[][MAXHELPSTRLEN] PROGMEM = 
{ "\r\n\r\n",
  "****** SPI Gateway - USART to SPI Master Bridge ******\r\n",
  "****** V1.1   Andy Gayne 2004    GD Technik, UK ******\r\n\r\n",
  "Enter hex encoded byte to send via SPI, or commands:\r\n",
  "[?] - Show current status\r\n",
  "[X] - Extended hex sequence ( maximum 16 bytes )\r\n",
  "[Q] - SCK freq ( ----SPI2X=0----    ----SPI2X=1---- )\r\n",
  "               ( 0=F/4      4MHz    4=F/2      8MHz )\r\n",
  "               ( 1=F/16     1MHz    5=F/8      2MHz )\r\n",
  "               ( 2=F/64   250kHz    6=F/32   500kHz )\r\n",
  "               ( 3=F/128  125kHz    7=F/64   250kHz )\r\n",
  "[S] - Slave Select pin level (0=low, 1=high)\r\n",
  "[P] - Auto-toggle SS control (0=low, 1=high, 2=off)\r\n",
  "[T] - Bit Transmission Order (0=MSB first, 1=LSB first)\r\n",
  "[M] - SPI Clock Mode         (0 : 1 : 2 : 3)\r\n",
  "      Mode  Leading-Edge     Trailing-Edge   CPOL CPHA\r\n",
  "       0    Sample(Rising)   Setup (Falling)   0    0\r\n",
  "       1    Setup (Rising)   Sample(Falling)   0    1\r\n",
  "       2    Sample(Falling)  Setup (Rising)    1    0\r\n",
  "       3    Setup (Falling)  Sample(Rising)    1    1\r\n",
  "[W] - Write (save) configuration (auto loaded at reset)\r\n\r\n"
// 1234567890123456789012345678901234567890123456789012345678901234  64 char
};
const char  freq[][MAXFREQSTRLEN] PROGMEM = 
{ "  (4MHz  Fosc/4 - 16MHz HSRC)",
  "  (1MHz  Fosc/16 - 16MHz HSRC)",
  "  (250kHz  Fosc/64 - 16MHz HSRC)",
  "  (125kHz  Fosc/128 - 16MHz HSRC)",
  "  (8MHz  Fosc/2 - 16MHz HSRC)",
  "  (2MHz  Fosc/8 - 16MHz HSRC)",
  "  (500kHz  Fosc/32 - 16MMHz HSRC)",
  "  (250kHz  Fosc/64 - 16MHz HSRC)"
};

// globals
unsigned char EEMEM config1;
unsigned char EEMEM config2;
unsigned char EEMEM config3;
unsigned char EEMEM checksum;

union
{
  unsigned char flags;
  struct
  {
    unsigned char   ss_level  :1,
                    ss_auto   :2,
                    unused    :5;
  };
} spi_ss;
                  

//***************************************************************************
// MAIN
//***************************************************************************
int main(void)
{
  char tmp;
  
  USART_Init(57600);     // Baudrate 
  SPI_MasterInit();

  sspinadjust(1);       // A good starting level for most applications
  spi_ss.ss_auto = 2;          // Default the auto toggle mode to 'off'

  sei();
  send_str(crlf);       // Send the welcome message
  send_str(crlf);
  send_str(help[1]);
  send_str(help[2]);
  send_str(crlf);
  
  loadconfig();         // load a stored configuration (if present)
  
  for( ; ; )            // loop forever
  {
    send_str(prompt);
    tmp = getnextchar();
    
    if (isxdigit(tmp)) spi_tx(tmp); // if a hex character received
    else switch (tmp)
    {
      case 'H':     // help request
        showhelp(); 
        break;
      case '?':     // show current status
        showstatus(STATLIST);
        break;
      case 'X':     // extended hex sequence
        hexsequence();
        break;
      case 'Q':     // adjust SPI SCK frequency
        setfreq();
        break;
      case 'S':     // adjust SS pin
        setsspin();
        break;
      case 'P':     // adjust SS pin auto toggle mode
        setautomode();
        break;
      case 'T':     // adjust bit transmission order
        setorder();
        break;
      case 'M':     // adjust SPI SCK mode
        setmode();
        break;
      case 'W':     // write configuration to EEPROM
        writeconfig();
        break;
      default:
        if(isgraph(tmp)) send_str(backspace); // erase invalid characters
    }// end switch
  }// end 'forever' loop
}

//***************************************************************************
// send_str - send string literal from flash to USART
//***************************************************************************
void send_str (const char  * str)
{
  char c = pgm_read_byte(str++);

  while (c) // send string characters until null terminator found
  {
    USART_Transmit(c);
    c = pgm_read_byte(str++);
  }
}

//***************************************************************************
// getnextchar - fetch next keyboard character, echo if displayable
//***************************************************************************
char getnextchar(void)
{
  char tmp;
  
  tmp = USART_Receive();                  // get next character
  tmp = toupper(tmp);                     // force to upper case
  if (isgraph(tmp)) USART_Transmit(tmp);  // Echo the received character if
                                          // printable and not whitespace.
  return(tmp); 
}

//***************************************************************************
// htoa - convert single hex digit to ASCII equivalent
//***************************************************************************
char htoa (char ch)
{
  ch += 0x30;            // add offset to ASCII '0'
  if (ch > '9') ch += 7; // if alphabetic add offset to ASCII 'A'
  return ch;
}

//***************************************************************************
// spi_tx - build byte value and send via SPI, report received byte
//***************************************************************************
void spi_tx (char u_nib) // function is passed first hex character typed
{
  char spi_ch;
  
  if (u_nib > 0x39) u_nib -= 7;     // A-F shifted to be consecutive from 0-9
  u_nib -= 0x30;                    // Remove ASCII offset
  u_nib = u_nib << 4;               // store value in upper nibble
  
  spi_ch = getnextchar();
  if (isxdigit(spi_ch))             // a valid hexadecimal character
  {
    if (spi_ch > 0x39) spi_ch -= 7; // A-F shifted to be consecutive from 0-9
    spi_ch -= 0x30;                 // Remove ASCII offset
    spi_ch |= u_nib;                // combine with upper nibble
    
    spi_ch = SPI_MasterTransmit(spi_ch);   // SPI byte exchange
    
    send_str(exchange);                    // header for SPI RX byte
    //decode received spi byte
    u_nib = (spi_ch & 0xf0) >> 4;          // extract upper nibble
    USART_Transmit( htoa(u_nib) );         // display upper nibble
    USART_Transmit( htoa(spi_ch & 0x0F) ); // display lower nibble
  }
  else send_str(error1); // abort entry for illegal second character
}

//***************************************************************************
// showhelp - show help screen and current settings
//***************************************************************************
void showhelp(void)
{
  //char ix;
  uint8_t ix;

  // send all the help strings stored in the help array
  for(ix = 0;ix < (sizeof(help)/MAXHELPSTRLEN);ix++) send_str(help[ix]);

  showstatus(STATLONG); // show single line status string
}

//***************************************************************************
// showstatus - show current SPI port settings
//***************************************************************************
void showstatus(char displaymode)
{
  //char tmp, ix=0;
  uint8_t tmp, ix=0;
  
  if(displaymode == STATLIST) USART_Transmit(' '); // some padding

  for(;;) // loop until the return statement is executed
  {
    send_str(status[ix]);
    switch(ix++){
      case 1:   // actual frequency
        tmp = (SPCR & 0x03) | ((SPSR & 1) << 2);              
        USART_Transmit(htoa(tmp));           
        if(displaymode == STATLIST) send_str(freq[tmp]);
        break;
      case 2:   // actual SS status 
        USART_Transmit(htoa((PORT_SPI & (1<<P_SS)) >> P_SS)); 
        break;
      case 3:   // actual auto mode
        USART_Transmit(htoa(spi_ss.ss_auto));         
        if(displaymode == STATLIST)
        {
          if(!spi_ss.ss_auto) send_str(auto0);
          else if(spi_ss.ss_auto == 1) send_str(auto1);
          else send_str(auto2);
        }
        break;
      case 4:   // actual data order
        tmp = (SPCR & (1 << DORD)) >> DORD;                   
        USART_Transmit(htoa(tmp));   
        if((displaymode == STATLIST) && !tmp) send_str(bit0);
        if((displaymode == STATLIST) && tmp) send_str(bit1);
        break;
      case 5:   // actual mode
        USART_Transmit(htoa((SPCR & 0x0C) >> 2));
        return; // nothing more to display
    }// end switch
    if(displaymode == STATLIST) send_str(nullprompt); // next line
    else send_str(gap);                               // or some padding
  }// end for
}

//***************************************************************************
// hexsequence - fetch, check and transmit a multi-byte sequence
//***************************************************************************
void hexsequence(void)
{
  char tx_str[MAXSEQ*2]; // enough storage for ascii encoded bytes
  char rx_str[MAXSEQ];   // storage for returned characters
  char length;
  int ix;
  
  for (ix = 0;ix < MAXSEQ*2;ix++)       // get character string
  {
    if (!(ix % 2)) USART_Transmit(':'); // separator every 2nd character
    tx_str[ix] = getnextchar();
    
    if (tx_str[ix] == '\r') break;      // detect 'enter' key pressed
    if (!isxdigit(tx_str[ix])) // detect invalid characters entered
    {
      send_str(error1);        // abort entry for illegal character
      return;                  // exit immediately
    }
    if (tx_str[ix] > '9') tx_str[ix] -= 7;
                               // A-F shifted to be consecutive from 0-9
    tx_str[ix] -= 0x30;        // Remove ASCII offset
  }
  length = ix/2;  // store number of full bytes entered

  // ensure SS pin is in idle condition if auto toggle is on
  if(!spi_ss.ss_auto)     sspinadjust(1); // make SS pin high
  if(spi_ss.ss_auto == 1) sspinadjust(0); // make SS pin low
  
  // At this point, tx_str contains length*2 valid half-bytes.
  // The conversion to bytes, SPI exchange and received byte display are
  // now carried out in seperate loops to ensure the SPI transmission takes
  // place in the shortest time period.
  
  for (ix = 0;ix < length;ix++) // convert half-bytes to bytes
    tx_str[ix] = (tx_str[ix*2] << 4) | tx_str[(ix*2)+1]; // nibbles combined
 
  if(spi_ss.ss_auto != 2) sspinadjust(spi_ss.ss_auto);  // Auto toggle SS pin if mode is on

  for (ix = 0;ix < length;ix++) // transmit the sequence
    rx_str[ix] = SPI_MasterTransmit(tx_str[ix]);         // SPI byte exchange
  
  // restore SS pin to idle condition if auto toggle is on
  if(!spi_ss.ss_auto)     sspinadjust(1); // make SS pin high
  if(spi_ss.ss_auto == 1) sspinadjust(0); // make SS pin low

  for (ix = 0;ix < length;ix++) // Display the exchange
  {
    send_str(nullprompt);
    printhex(tx_str[ix]);
    //USART_Transmit( htoa((tx_str[ix] & 0xf0) >> 4) );//display upper nibble
    //USART_Transmit( htoa(tx_str[ix] & 0x0F) );       //display lower nibble
    send_str(exchange);
    printhex(rx_str[ix]);
    //USART_Transmit( htoa((rx_str[ix] & 0xf0) >> 4) );//display upper nibble
    //USART_Transmit( htoa(rx_str[ix] & 0x0F) );       //display lower nibble
  }
}

//***************************************************************************
// setfreq - set the SPI SCK frequency
//***************************************************************************
void setfreq(void)
{
  char tmp;
  
  tmp = getnextchar();
  tmp -= 0x30;  // Remove ASCII offset
  
  if (tmp < 8) // a valid frequency value (unsigned, so no negative values)
  {
    SPCR &= 0xFC;        // mask off current SPR bits (bit 1 & 0)
    SPCR |= tmp & 0x03;  // set new value using two lsb of selection number
    if(tmp & 0x04) SPSR |= (1 << SPI2X); // for selection 4-7 set SPI2X
    else SPSR &= ~(1 << SPI2X);          // otherwise clear SPI2X
    send_str(newstat);
    showstatus(STATLIST);
  }
  else send_str(error1); // abort entry for illegal second character
}

//***************************************************************************
// setsspin - modify the state of SS pin on SPI port
//***************************************************************************
void setsspin(void)
{
  char tmp;
  
  tmp = getnextchar();
  tmp -= 0x30;  // Remove ASCII offset
  
  if (tmp < 2)  // a valid bit value (unsigned, so no negative values)
  {
    sspinadjust(tmp); // change pin level
    send_str(newstat);
    showstatus(STATLIST);
  }
  else send_str(error1);  // abort entry for illegal second character
}

//***************************************************************************
// sspinadjust - modify the physical SS pin on SPI port
//***************************************************************************
void sspinadjust(char level)
{
  if(level) PORT_SPI |= (1 << P_SS);  // make SS pin high
  else PORT_SPI &= ~(1 << P_SS);    // make SS pin low
  spi_ss.ss_level = level;
}

//***************************************************************************
// setautomode - store the required mode for SS pin auto toggle
//***************************************************************************
void setautomode(void)
{
  char tmp;
  
  tmp = getnextchar();
  tmp -= 0x30;  // Remove ASCII offset
  
  if (tmp < 3)  // a valid bit value (unsigned, so no negative values)
  {
    spi_ss.ss_auto = tmp;  // auto mode stored
    send_str(newstat);
    showstatus(STATLIST);
  }
  else send_str(error1);  // abort entry for illegal second character
}

//***************************************************************************
// setorder - set the bit transmission order
//***************************************************************************
void setorder(void)
{
  char tmp;
  
  tmp = getnextchar();
  tmp -= 0x30;  // Remove ASCII offset
  
  if (tmp < 2)  // a valid bit value (unsigned, so no negative values)
  {
    if(tmp) SPCR |= (1 << DORD); // set DORD bit
    else SPCR &= ~(1 << DORD);   // clear DORD bit
    send_str(newstat);
    showstatus(STATLIST);
  }
  else send_str(error1);  // abort entry for illegal second character
}

//***************************************************************************
// setmode - set the SPI communications mode
//***************************************************************************
void setmode(void)
{
  char tmp;
  
  tmp = getnextchar();
  tmp -= 0x30;  // Remove ASCII offset
  
  if (tmp < 4)  // a valid mode value (unsigned, so no negative values)
  {
    SPCR &= 0xF3;         // mask off current CPOL/CPHA bits (bit 3 & 2)
    SPCR |= tmp << 2;     // set new value
    send_str(newstat);
    showstatus(STATLIST);
  }
  else send_str(error1);  // abort entry for illegal second character
}

//***************************************************************************
// writeconfig - save the current configuration in EEPROM
//***************************************************************************
void writeconfig(void)
{
  unsigned char tmp1, tmp2, tmp3, tmp4;

  cli(); // disable interrupts during EEPROM write

#if (__DEBUG_EEPROM__ == 1)
  send_str(crlf);
  printhex(SPCR);
  printhex(SPSR);
  printhex(spi_ss.flags);
  send_str(crlf);
#endif

  eeprom_write_byte(&config1, SPCR);          // store SPCR register 

  eeprom_write_byte(&config2, SPSR);          // store SPSR register

  eeprom_write_byte(&config3, spi_ss.flags);  // store global flag variable
 
  //checksum = 0 - (config1 + config2 + config3);
  tmp1 = eeprom_read_byte(&config1);
  tmp2 = eeprom_read_byte(&config2);
  tmp3 = eeprom_read_byte(&config3);
  tmp4 = 0 - (tmp1 + tmp2 + tmp3);
    
  eeprom_write_byte(&checksum, tmp4); // generate and store checksum
  
#if (__DEBUG_EEPROM__ == 1)
  send_str(crlf);
  printhex(tmp1);
  printhex(tmp2);
  printhex(tmp3);
  printhex(tmp4);
  send_str(crlf);
#endif

  sei(); // Enable interrupts => enable UART interrupts
  
  send_str(cfgwrite);
}

//***************************************************************************
// loadconfig - fetch the configuration from EEPROM
//***************************************************************************
void loadconfig(void)
{ 
  unsigned char tmp1, tmp2, tmp3, tmp4;

  // check for valid config - blank EEPROM will not give zero result
  tmp1 = eeprom_read_byte(&config1);
  tmp2 = eeprom_read_byte(&config2);
  tmp3 = eeprom_read_byte(&config3);
  tmp4 = eeprom_read_byte(&checksum);
  
#if (__DEBUG_EEPROM__ == 1)
  send_str(crlf);
  printhex(tmp1);
  printhex(tmp2);
  printhex(tmp3);
  printhex(tmp4);
  send_str(crlf);
#endif

  if (! ((tmp1 + tmp2 + tmp3 + tmp4) & 0xFF) )
  {
    SPCR = tmp1;        // restore SPCR register
    SPSR = tmp2;        // restore SPSR register
    spi_ss.flags = tmp3;       // restore global flag variable

    sspinadjust(spi_ss.ss_level); // restore default SS pin level    
    
    send_str(config);
    showstatus(STATLIST);
  }
  else send_str(noconfig);
}

//***************************************************************************
// printhex 
//***************************************************************************
void printhex(char c)
{
  USART_Transmit(htoa((c & 0xf0) >> 4));
  USART_Transmit(htoa((c & 0x0f)));
}

