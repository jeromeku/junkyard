CXXFLAGS=-I/usr/include/opencv -Wall -W -O2
LDFLAGS=-lcv -lhighgui

all: psnrtool resize

psnrtool: psnrtool.cc
resize: resize.cc simple.cc

psnrtool.cc: psnr.h
resize.cc: resize.h psnr.h simple.h
simple.cc: simple.h resize.h

clean:
	rm -f psnrtool resize
