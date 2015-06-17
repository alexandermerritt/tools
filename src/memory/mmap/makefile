CXX = g++
CXXFLAGS = -O2 -std=c++11 -Wall -Werror -Wextra -g
CXXFLAGS += -Wno-unused-variable -Wno-unused-function

EXES = mmap
all:	$(EXES)

mmap:	mmap.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o $(EXES)
