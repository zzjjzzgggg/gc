CXX = clang++

CXXFLAGS += -Wall -std=c++11
LDFLAGS += -lrt -pthread -std=c++11

## Main application file
MAIN = genbits
IOGREEDY = iogreedy
LIBDIR = /home/jzzhao/git_project
LIB = -I$(LIBDIR)/netsnap/glib -I$(LIBDIR)/netsnap/snap


OBJS_COM = lz4.o lz4io.o Snap.o argsparser.o utils.o
OBJS_MAIN = bitsgen.o bitsdecode.o $(OBJS_COM)
OBJS_IOGREEDY = bitsdecode.o iogreedy.o $(OBJS_COM)

DEBUG = 

all: $(MAIN) $(IOGREEDY)
opt: CXXFLAGS += -O4
opt: LDFLAGS += -O4

# COMPILE

$(MAIN): main_genbits.cpp $(OBJS_MAIN)
	$(CXX) $(DEBUG) $(OBJS_MAIN) -o $(MAIN) $(LIB) $(LDFLAGS) $<

$(IOGREEDY): main_exam.cpp $(OBJS_IOGREEDY)
	$(CXX) $(DEBUG) $(OBJS_IOGREEDY) -o $(IOGREEDY) $(LIB) $(LDFLAGS) $<

bitsgen.o: bitsgen.cpp bitsgen.h stdafx.h
	$(CXX) -c $(DEBUG) $(CXXFLAGS) $(LIB) $<

bitsdecode.o: bitsdecode.cpp bitsdecode.h stdafx.h
	$(CXX) -c $(DEBUG) $(CXXFLAGS) $(LIB) $<

iogreedy.o: iogreedy.cpp iogreedy.h stdafx.h
	$(CXX) -c $(DEBUG) $(CXXFLAGS) $(LIB) $<

Snap.o:
	$(CXX) -c $(CXXFLAGS) $(LIB) $(LIBDIR)/netsnap/snap/Snap.cpp

argsparser.o: argsparser.cpp argsparser.h
	$(CXX) -c $(CXXFLAGS) $<

utils.o: utils.cpp utils.h
	$(CXX) -c $(CXXFLAGS) $<

lz4io.o: lz4/lz4io.cpp lz4/lz4io.h
	$(CXX) -c $(DEBUG) $(CXXFLAGS) $<

lz4.o: lz4/lib/lz4.c
	$(CXX) -c -x c -Wall $<

clean:
	rm -f *.o *.Err $(MAIN) $(IOGREEDY)
	rm -rf Debug Release
