CXX = clang++
CXXFLAGS = -O2 -std=c++11 -Wall -Werror -Wextra
CXXFLAGS += -Wno-unused-variable -Wno-unused-function

EXES = tlbtest cpuid htable list deque sort mmap
all:	$(EXES)

mmap:	mmap.o
	$(CXX) $(CXXFLAGS) mmap.o -o $@

sort:	sort.o
	$(CXX) $(CXXFLAGS) $^ -o $@

deque:	deque.o
	$(CXX) $(CXXFLAGS) $^ -o $@

list:	list.o
	$(CXX) $(CXXFLAGS) $^ -o $@

htable:	htable.o
	$(CXX) $(CXXFLAGS) $^ -o $@

tlbtest:	tlbtest.o
	$(CXX) $(CXXFLAGS) tlbtest.o -o $@

cpuid:	cpuid.o
	$(CXX) $(CXXFLAGS) cpuid.o -o $@

addr:	addr.o
	$(CXX) $(CXXFLAGS) addr.o -o $@

clean:
	rm -f *.o $(EXES)
