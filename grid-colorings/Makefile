CXXFLAGS=-O3 -march=native -mtune=native -Wall -W

all: grid-col grid-col-pair

grid-col:
	g++ $(CXXFLAGS) -o grid-col grid-col.cc

grid-col-pair:
	g++ $(CXXFLAGS) -DCOLLAPSE_PAIR -o grid-col-pair grid-col.cc

check: all
	./check.py

clean:
	rm -f grid-col grid-col-pair
