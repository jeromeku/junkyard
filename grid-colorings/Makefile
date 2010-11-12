CXXFLAGS=-O3 -march=native -mtune=native -funroll-loops -fno-peephole2 -W -Wall

all: grid-col grid-col-pair

grid-col: grid-col.cc Makefile
	g++ $(CXXFLAGS) -o grid-col grid-col.cc

grid-col-pair: grid-col.cc Makefile
	g++ $(CXXFLAGS) -DCOLLAPSE_PAIR -o grid-col-pair grid-col.cc

check: all
	./check.py

clean:
	rm -f grid-col grid-col-pair
