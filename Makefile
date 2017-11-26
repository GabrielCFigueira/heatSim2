CFLAGS= -g -Wall -pedantic
CC=gcc

all: heatSim

heatSim: main.o thread.o matrix2d.o util.o matrix2d.h thread.h util.h
	$(CC) $(CFLAGS) -pthread -o heatSim main.o thread.o matrix2d.o util.o

main.o: main.c thread.h matrix2d.h util.h
	$(CC) $(CFLAGS) -c main.c

thread.o: thread.c thread.h matrix2d.h util.h
	$(CC) $(FLAGS) -c thread.c

util.o: util.c util.h
	$(CC) $(FLAGS) -c util.c

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(FLAGS) -c matrix2d.c

clean:
	rm -f *.o heatSim

zip:
	zip exercicio4.zip *.c *.h Makefile

run:
	./heatSim 1 10 10 0 0 5000 1 0 ola.txt 0
