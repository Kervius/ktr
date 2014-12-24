
all: tags ktr
MAKEFLAGS=-j2

CXXFLAGS=-O0 -std=c++11 -Wall -Wno-reorder -g
CXX=clang++

o/%.o: %.cc Makefile
	$(CXX) $(CXXFLAGS) $< -c -o $@

ktr: o/maink.o o/utilk.o o/filek.o o/ktor.o o/cksum.o
	$(CXX) $(CXXFLAGS) $^ -o $@

tags: ktor.cc k.hh maink.cc utilk.cc filek.cc cksum.cc utilk.hh cksum.hh Makefile
	-ctags $^

.PHONY: clean deps

clean:
	rm -f o/*.o ktr

deps:
	perl -pe '/^# --- deps ---/ && exit' < Makefile > tmp1.mk
	echo '# --- deps ---' >> tmp1.mk
	grep -n '^#include.*hh' *.cc | perl -pe 's!.cc:\d+:#include "!.o: !; s!^!o/!; s!"$$!!' >> tmp1.mk
	grep -q ^all  < tmp1.mk
	grep -q ^ktr  < tmp1.mk
	mv tmp1.mk Makefile

# --- deps ---
o/cksum.o: cksum.hh
o/cksum.o: utilk.hh
o/filek.o: k.hh
o/filek.o: utilk.hh
o/ktor.o: k.hh
o/ktor.o: utilk.hh
o/maink.o: k.hh
o/utilk.o: utilk.hh
