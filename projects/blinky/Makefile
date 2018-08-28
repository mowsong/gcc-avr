TARGET=$(notdir $(CURDIR))
ELF=$(TARGET).elf
IHEX=$(TARGET).hex
SOURCES=main.c
DEPS=
COBJ=$(SOURCES:.c=.o)

CC=avr-gcc
OBJC=avr-objcopy
MCU=atmega328p
CFLAGS=-I. -Wall -Os -DF_CPU=16000000UL 
LDFLAGS=

include ../../common/Makefile.common
