CC = gcc
CFLAGS = -Wall
LIBS = -lws2_32
GUI_LIBS = -lws2_32 -lgdi32 -luser32 -lkernel32

PROGS = prog1 prog2 prog3 prog4 gui

all: $(PROGS)

prog1: prog1.c
	$(CC) $(CFLAGS) -o prog1 prog1.c $(LIBS)

prog2: prog2.c
	$(CC) $(CFLAGS) -o prog2 prog2.c $(LIBS)

prog3: prog3.c
	$(CC) $(CFLAGS) -o prog3 prog3.c $(LIBS)

prog4: prog4.c
	$(CC) $(CFLAGS) -o prog4 prog4.c $(LIBS)

gui: gui.c
	$(CC) $(CFLAGS) -o gui gui.c $(GUI_LIBS)

clean:
	del *.exe