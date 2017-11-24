CFLAGS= -g -Wall -pedantic
CC=gcc

all: heatSim

heatSim: main.o thread.o matrix2d.o matrix2d.h thread.h
	$(CC) $(CFLAGS) -pthread -o heatSim main.o thread.o matrix2d.o

main.o: main.c thread.h matrix2d.h
	$(CC) $(CFLAGS) -c main.c

thread.o: thread.c thread.h matrix2d.h
	$(CC) $(FLAGS) -c thread.c

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(FLAGS) -c matrix2d.c

clean:
	rm -f *.o heatSim

run:
	./heatSim 2 10 10 0 0 5 1 0 ola.txt 1
