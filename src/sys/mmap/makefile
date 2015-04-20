all:	map

CXX = clang++36
CXXFLAGS = -Wall -Wextra -O0 -g

map.o:	map.cc
	$(CXX) $(CXXFLAGS) -std=c++11 map.cc -c

map:	map.o
	$(CXX) $(CXXFLAGS) map.o -o map

clean:
	rm -f *.o map
