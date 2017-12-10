

SRC=$(wildcard *.cc)
OBJ=$(SRC:.cc=.o)

CXXFLAGS=-O0 -std=c++11 -Wall -Wno-reorder -g -D_GLIBCXX_DEBUG
CXX=$(firstword $(wildcard /usr/bin/clang++* /usr/bin/g++ /usr/bin/c++))
#CXX=g++

all: $(OBJ) tags

test1: $(OBJ) Minsk/min.o
	$(CXX) -g $^ -o test1

tags: $(SRC) $(wildcard *.hh) Makefile
	-@ctags $^

clean:
	-rm $(OBJ)


