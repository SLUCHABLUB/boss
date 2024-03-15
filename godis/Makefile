CFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS=$(CFLAGS)
LDFLAGS=-lsfml-graphics -lsfml-window -lsfml-system
RGB_INCDIR=../include
CXX:=g++

all: godis

godis: sfml_matrix.o ../lib/graphics.o ../lib/bdf-font.o ../examples-api-use/$(ARGS).o
	$(CXX) -I$(RGB_INCDIR) $^ $(CXXFLAGS) -o $@ $(LDFLAGS)

%.o: %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

clean:
	rm -f godis *.o

.PHONY: all