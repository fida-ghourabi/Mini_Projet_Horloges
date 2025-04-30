CC = gcc
CFLAGS = -Wall
LIBS = -lws2_32

PROGS = prog1_matrix prog2_matrix prog3_matrix prog4_matrix

all: $(PROGS)

prog1_matrix: prog1_matrix.c
	$(CC) $(CFLAGS) -o prog1_matrix prog1_matrix.c $(LIBS)

prog2_matrix: prog2_matrix.c
	$(CC) $(CFLAGS) -o prog2_matrix prog2_matrix.c $(LIBS)

prog3_matrix: prog3_matrix.c
	$(CC) $(CFLAGS) -o prog3_matrix prog3_matrix.c $(LIBS)

prog4_matrix: prog4_matrix.c
	$(CC) $(CFLAGS) -o prog4_matrix prog4_matrix.c $(LIBS)

clean:
	del *.exe
