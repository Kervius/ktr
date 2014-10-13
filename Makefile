

ktor: ktor.cc Makefile
	clang++ -Wall -g ktor.cc -o ktor

tags: ktor.cc
	ctags $^
