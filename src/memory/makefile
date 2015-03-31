CXX = clang++
CXXFLAGS = -O2 -std=c++11 -Wall -Werror -Wextra
CXXFLAGS += -Wno-unused-variable -Wno-unused-function

EXES = tlbtest cpuid htable list deque sort mmap
all:	$(EXES)

mmap:	mmap.o
	$(CXX) $(CXXFLAGS) $^ -o $@

sort:	sort.o
	$(CXX) $(CXXFLAGS) $^ -o $@

deque:	deque.o
	$(CXX) $(CXXFLAGS) $^ -o $@

list:	list.o
	$(CXX) $(CXXFLAGS) $^ -o $@

htable:	htable.o
	$(CXX) $(CXXFLAGS) $^ -o $@

tlbtest:	tlbtest.o
	$(CXX) $(CXXFLAGS) $^ -o $@ -L/usr/local/lib -llikwid -lm

cpuid:	cpuid.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o $(EXES)
