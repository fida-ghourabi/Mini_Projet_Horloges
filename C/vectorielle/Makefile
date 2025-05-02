# Makefile
CC = gcc
CFLAGS = -Wall
LIBS = -lws2_32 -lgdi32

PROGS = prog1_vector prog2_vector prog3_vector prog4_vector gui

all: $(PROGS)

prog1_vector: prog1_vector.c
	$(CC) $(CFLAGS) -o prog1_vector prog1_vector.c $(LIBS)

prog2_vector: prog2_vector.c
	$(CC) $(CFLAGS) -o prog2_vector prog2_vector.c $(LIBS)

prog3_vector: prog3_vector.c
	$(CC) $(CFLAGS) -o prog3_vector prog3_vector.c $(LIBS)

prog4_vector: prog4_vector.c
	$(CC) $(CFLAGS) -o prog4_vector prog4_vector.c $(LIBS)

gui: gui.c
	$(CC) $(CFLAGS) -o gui gui.c $(LIBS)

clean:
	del *.exe