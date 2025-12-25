# Ako radim cross-kompajliranje za Raspberry Pi,  prvu liniju ispod:
CC = arm-linux-gnueabihf-gcc
# Ako kompajliram direktno na Raspberry Pi, koristim ovo:
#CC = gcc

CFLAGS = -Wall
LIBS = -lwiringPi

TARGET = Conway

all: $(TARGET)

$(TARGET): Conway.o
	$(CC) -o $(TARGET) Conway.o $(LIBS)

Conway.o: Conway.c
	$(CC) -c Conway.c $(CFLAGS)

clean:
	rm -f $(TARGET) Conway.o
