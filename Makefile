.phony all:
all: mts

mts: mts.c
	gcc -Wall mts.c -pthread -o MTS

.PHONY clean:
clean:
	-rm -rf *.o *.exe MTS
