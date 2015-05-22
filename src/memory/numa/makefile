all:	numatest tlbtest

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O3 -DLIKWID_PERFMON

numatest:	numatest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tlbtest:	tlbtest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) -pthread -L/usr/local/lib -llikwid -lm

clean:
	rm -f *.o numatest tlbtest
