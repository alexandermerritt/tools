CXX = g++
CXXFLAGS = -O0 -std=c++11 -Wall -Werror -Wextra -g
CXXFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter

EXES = mmap
all:	$(EXES)

mmap:	mmap.o
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpthread -lrt

clean:
	rm -f *.o $(EXES)
