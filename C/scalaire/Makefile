CC = gcc
CFLAGS = -Wall
GTK_CFLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LIBS = `pkg-config --libs gtk+-3.0`
LIBS = -lws2_32
GTK_LIBS_FULL = -lws2_32 $(GTK_LIBS)

all: prog1 prog2 prog3 prog4

prog1: prog1.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o prog1 prog1.c $(GTK_LIBS_FULL)

prog2: prog2.c
	$(CC) $(CFLAGS) -o prog2 prog2.c $(LIBS)

prog3: prog3.c
	$(CC) $(CFLAGS) -o prog3 prog3.c $(LIBS)

prog4: prog4.c
	$(CC) $(CFLAGS) -o prog4 prog4.c $(LIBS)

clean:
	rm -f prog1 prog2 prog3 prog4