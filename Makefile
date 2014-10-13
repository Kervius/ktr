

ktor: ktor.cc Makefile
	clang++ -std=c++11 -Wall -g ktor.cc -o ktor

tags: ktor.cc
	ctags $^
