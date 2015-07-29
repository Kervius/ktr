
all: tags ktr
MAKEFLAGS=-j2

CXXFLAGS=-O0 -std=c++11 -Wall -Wno-reorder -g
CXX=$(firstword $(wildcard /usr/bin/clang++* /usr/bin/g++ /usr/bin/c++))

#ktor: ktor.cc maink.cc k.hh utilk.cc utilk.hh Makefile
#	clang++ -O0 -std=c++11 -Wall -g ktor.cc maink.cc utilk.cc -o ktor

HDR=k.hh utilk.hh datak.hh
SRC=maink.cc utilk.cc filek.cc ktor.cc datak.cc
OBJ=$(patsubst %.cc,o/%.o,$(SRC))

o/%.o: %.cc $(HDR) Makefile
	$(CXX) $(CXXFLAGS) $< -c -o $@

ktr: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

datak_tst:
	$(CXX) $(CXXFLAGS) -DDATAK_SELFTEST utilk.cc datak.cc -o datak_tst

tags: $(SRC) $(HDR) Makefile
	-ctags $^

check:
	-@cppcheck --enable=all  *.cc 2>cppcheck.out
	-@cat cppcheck.out

clean:
	rm -f o/*.o ktr cppcheck.out


.PHONY: clean
