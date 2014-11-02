
all: tags ktor

CXXFLAGS=-O0 -std=c++11 -Wall -g
CXX=clang++

#ktor: ktor.cc maink.cc k.hh utilk.cc utilk.hh Makefile
#	clang++ -O0 -std=c++11 -Wall -g ktor.cc maink.cc utilk.cc -o ktor

%.o: %.cc k.hh utilk.hh Makefile
	$(CXX) $(CXXFLAGS) $< -c -o $@

ktor: ktor.o maink.o utilk.o
	$(CXX) $(CXXFLAGS) $^ -o $@

tags: ktor.cc k.hh maink.cc utilk.cc utilk.hh
	ctags $^
