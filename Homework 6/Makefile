all: ant_algorithm.c
	mpiicc -std=c99 -Wall -g -openmp -o ant_algorithm ant_algorithm.c -lm

clean:
	rm ant_algorithm
