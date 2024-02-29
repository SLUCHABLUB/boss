CFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS=$(CFLAGS)
LDFLAGS+=-lsfml-graphics -lsfml-window -lsfml-system
CXX:=g++

all: test_bench

test_bench: boss_test_bench.o
	$(CXX) $^ $(CXXFLAGS) -o $@ $(LDFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: all