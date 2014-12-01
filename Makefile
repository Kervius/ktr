
all: tags ktr
MAKEFLAGS=-j2

CXXFLAGS=-O0 -std=c++11 -Wall -g
CXX=clang++

#ktor: ktor.cc maink.cc k.hh utilk.cc utilk.hh Makefile
#	clang++ -O0 -std=c++11 -Wall -g ktor.cc maink.cc utilk.cc -o ktor

o/%.o: %.cc k.hh utilk.hh Makefile
	$(CXX) $(CXXFLAGS) $< -c -o $@

ktr: o/maink.o o/utilk.o o/filek.o o/ktor.o 
	$(CXX) $(CXXFLAGS) $^ -o $@

tags: ktor.cc k.hh maink.cc utilk.cc filek.cc utilk.hh Makefile
	-ctags $^

clean:
	rm -f o/*.o ktor


.PHONY: clean
