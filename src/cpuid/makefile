CXX := clang++
CC := clang
CXXFLAGS := -O2 -Wall -Wextra -Werror -std=c++11
CFLAGS := -O2 -Wall -Wextra -Werror -std=gnu11
LDFLAGS := 
LIBS := 

all:	cpuid timing

cpuid:	cpuid.o
	$(CXX) $< -o $@ $(LDFLAGS) $(LIBS)

timing:	timing.o
	$(CC) $< -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o cpuid timing
