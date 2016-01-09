all:	numatest tlbtest tlbtest.lw

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O3

numatest:	numatest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tlbtest:	tlbtest.cc
	$(CXX) $(CXXFLAGS) -c $^
	$(CXX) -o $@ tlbtest.o $(LDFLAGS)

tlbtest.lw:	tlbtest.cc
	$(CXX) $(CXXFLAGS) -DLIKWID_PERFMON -c $^
	$(CXX) -o $@ tlbtest.o $(LDFLAGS) -pthread -L/usr/local/lib -llikwid -lm

clean:
	rm -f *.o numatest tlbtest tlbtest.lw
