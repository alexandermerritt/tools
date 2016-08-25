CXX := clang++
CXXFLAGS := -O2 -Wall -Wextra -Werror -std=c++11
LDFLAGS := 
LIBS := 

all:	cpuid

cpuid:	cpuid.o
	$(CXX) $< -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o cpuid
