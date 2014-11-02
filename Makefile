
all: ktor tags

ktor: ktor.cc maink.cc k.hh utilk.cc utilk.hh Makefile
	clang++ -std=c++11 -Wall -g ktor.cc maink.cc utilk.cc -o ktor

tags: ktor.cc k.hh maink.cc utilk.cc utilk.hh
	ctags $^
