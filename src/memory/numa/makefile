all:	numatest

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

numatest:	numatest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o numatest
