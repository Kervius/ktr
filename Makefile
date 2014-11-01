
all: ktor tags

ktor: ktor.cc maink.cc k.hh Makefile
	clang++ -std=c++11 -Wall -g ktor.cc maink.cc -o ktor

tags: ktor.cc
	ctags $^
