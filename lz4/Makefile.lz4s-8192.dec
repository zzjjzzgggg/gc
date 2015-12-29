CXX = gcc

CXXFLAGS += -Wall -std=c++11
LDFLAGS += -lrt -pthread -std=c++11

## Main application file
MAIN = main
OBJS = lz4.o
DEBUG =

all: $(MAIN)
opt: CXXFLAGS += -O4
opt: LDFLAGS += -O4
opt: $(MAIN)

# COMPILE

$(MAIN): $(MAIN).cpp $(OBJS)
	$(CXX) $(DEBUG) $(OBJS) -o $(MAIN) $(LDFLAGS) $<

lz4.o: lib/lz4.c
	$(CXX) -c -x c -Wall $<

clean:
	rm -f *.o $(MAIN) *.Err test lib/*.o
	rm -rf Debug Release
