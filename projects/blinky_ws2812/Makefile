TARGET=$(notdir $(CURDIR))
ELF=$(TARGET).elf
IHEX=$(TARGET).hex
SOURCES=main.c light_ws2812.c
DEPS=
COBJ=$(SOURCES:.c=.o)

CC=avr-gcc
OBJC=avr-objcopy
MCU=atmega328p

WS2812_LIB=../../common/light_ws2812_AVR

vpath %.c $(WS2812_LIB)

CFLAGS =-Wall -Os -DF_CPU=16000000UL 

CFLAGS+=-I. -I$(WS2812_LIB) 

LDFLAGS=

include ../../common/Makefile.common
