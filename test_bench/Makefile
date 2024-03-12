CFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS=$(CFLAGS)
LDFLAGS=-lsfml-graphics -lsfml-window -lsfml-system
RGB_INCDIR=../include
CXX:=g++

all: test_bench

test_bench: boss_test_bench.o sfml_matrix.o ../examples-api-use/choochoo.o
	$(CXX) -I$(RGB_INCDIR) $^ $(CXXFLAGS)  -o $@ $(LDFLAGS)

%.o: %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

clean:
	rm -f test_bench *.o

.PHONY: all