# CXX = clang++
CXX = g++

CXXFLAGS += -Wall -std=c++11
LDFLAGS += -lrt -pthread -std=c++11

## Main application file
GENBITS = genbits
IOGREEDY = iogreedy
VALI_MEASURE = vali_measure
LIB = -Inetsnap/glib -Inetsnap/snap


OBJS_COM = lz4.o lz4io.o Snap.o argsparser.o utils.o
OBJS_GENBITS = bitsgen.o bitsdecode.o $(OBJS_COM)
OBJS_IOGREEDY = bitsdecode.o iogreedy.o $(OBJS_COM)

DEBUG = -g

all: $(GENBITS) $(VALI_MEASURE) $(IOGREEDY)
opt: CXXFLAGS += -O4
opt: LDFLAGS += -O4

# COMPILE

$(GENBITS): main_$(GENBITS).cpp $(OBJS_GENBITS)
	$(CXX) $(DEBUG) $(OBJS_GENBITS) -o $(GENBITS) $(LIB) $(LDFLAGS) $<

$(VALI_MEASURE): main_$(VALI_MEASURE).cpp $(OBJS_GENBITS)
	$(CXX) $(DEBUG) $(OBJS_GENBITS) -o $(VALI_MEASURE) $(LIB) $(LDFLAGS) $<

$(IOGREEDY): main_$(IOGREEDY).cpp $(OBJS_IOGREEDY)
	$(CXX) $(DEBUG) $(OBJS_IOGREEDY) -o $(IOGREEDY) $(LIB) $(LDFLAGS) $<

bitsgen.o: bitsgen.cpp bitsgen.h stdafx.h
	$(CXX) -c $(DEBUG) $(CXXFLAGS) $(LIB) $<

bitsdecode.o: bitsdecode.cpp bitsdecode.h stdafx.h
	$(CXX) -c $(DEBUG) $(CXXFLAGS) $(LIB) $<

iogreedy.o: iogreedy.cpp iogreedy.h stdafx.h
	$(CXX) -c $(DEBUG) $(CXXFLAGS) $(LIB) $<

Snap.o:
	$(CXX) -c $(CXXFLAGS) $(LIB) netsnap/snap/Snap.cpp

argsparser.o: argsparser.cpp argsparser.h
	$(CXX) -c $(CXXFLAGS) $<

utils.o: utils.cpp utils.h
	$(CXX) -c $(CXXFLAGS) $<

lz4io.o: lz4/lz4io.cpp lz4/lz4io.h
	$(CXX) -c $(DEBUG) $(CXXFLAGS) $<

lz4.o: lz4/lib/lz4.c
	$(CXX) -c -x c -Wall $<

clean:
	rm -f *.o *.Err $(GENBITS) $(IOGREEDY) $(VALI_MEASURE)
	rm -rf Debug Release
