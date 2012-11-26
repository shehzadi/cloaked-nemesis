# Makefile 
#*********************************************************
CC = gcc
CFLAGS = -g 

PSRC=Makefile readckt.c 

POBJ    = readckt.o

TARGET  = readckt

#*********************************************************


$(TARGET) : $(POBJ)
	gcc $(CFLAGS) $(POBJ) -o $(TARGET) -lm
