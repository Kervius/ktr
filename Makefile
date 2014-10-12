

xtor: ktor.cc Makefile
	clang++ -Wall -g ktor.cc -o xtor

tags: ktor.cc
	ctags $^
