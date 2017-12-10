
CXX=clang++-3.6

all: min.o
min.o: min.cc min.hh Makefile
	$(CXX) -Wall -std=c++11 -D_GLIBCXX_DEBUG -c min.cc

t: test1

test1: min.cc min.hh Makefile
	$(CXX) -Wall -std=c++11 -DSELF_TEST min.cc -o $@
	./$@
