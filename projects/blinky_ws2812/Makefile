SOURCES=main.c light_ws2812.c
DEPS=

MCU=atmega328p

WS2812=../../common/light_ws2812_AVR

vpath %.c $(WS2812)

CFLAGS =-Wall -Os -DF_CPU=16000000UL 

CFLAGS+=-I. -I$(WS2812) 

LDFLAGS=

include ../../common/Makefile.common
