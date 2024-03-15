CFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS=$(CFLAGS)
LDFLAGS=-lsfml-graphics -lsfml-window -lsfml-system
RGB_INCDIR=../include
CXX:=g++

AV_LDFLAGS=$(shell pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil)

all: godis video

video: sfml_matrix.o ../lib/graphics.o ../lib/bdf-font.o ../lib/content-streamer.o ../utils/video-viewer.o
	$(CXX) -I$(RGB_INCDIR) $^ $(CXXFLAGS) -o $@ $(LDFLAGS) $(AV_LDFLAGS)

godis: sfml_matrix.o ../lib/graphics.o ../lib/bdf-font.o ../examples-api-use/$(ARGS).o
	$(CXX) -I$(RGB_INCDIR) $^ $(CXXFLAGS) -o $@ $(LDFLAGS)

%.o: %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f godis video *.o

.PHONY: all godis
