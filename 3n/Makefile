all: p pow

p: p.cc Makefile
	g++ -o p p.cc -Wall -W -O3 -march=native -mtune=native -funroll-loops -fno-peephole2 -lgmp -static

pow: p.cc Makefile
	g++ -o pow pow.cc -Wall -W -lgmp -static

clean:
	rm -f p pow
