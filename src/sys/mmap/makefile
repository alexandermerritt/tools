all:	map-madv-color

CXX = clang++36
CXXFLAGS = -Wall -Wextra -O0 -g -Wno-unused-parameter

map-madv-color.o:	map-madv-color.cc
	$(CXX) $(CXXFLAGS) -std=c++11 map-madv-color.cc -c

map-madv-color:	map-madv-color.o
	$(CXX) $(CXXFLAGS) map-madv-color.o -o map-madv-color

clean:
	rm -f *.o map-madv-color
