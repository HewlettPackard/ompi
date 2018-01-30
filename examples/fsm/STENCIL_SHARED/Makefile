CC=mpicc
CFLAGS=-g
.PHONY: clean

main: stencil.o stencil_sub.o
	$(CC) $(CFLAGS) -o main stencil.o stencil_sub.o

stencil.o: stencil.c stencil.h
	$(CC) $(CFLAGS) -c stencil.c 

stencil_sub.o: stencil_sub.c stencil.h
	$(CC) $(CFLAGS) -c stencil_sub.c


clean:
	rm -f *.o *.o 
