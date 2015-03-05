CXX = clang++
CXXFLAGS = -O2 -std=c++11 -Wall -Werror -Wextra
CXXFLAGS += -Wno-unused-variable -Wno-unused-function

all:	tlbtest cpuid

tlbtest:	tlbtest.cc
	$(CXX) $(CXXFLAGS) $^ -o $@

cpuid:	cpuid.cc
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f tlbtest cpuid
