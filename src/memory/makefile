CXX = g++
CXXFLAGS = -O2 -std=c++11 -Wall -Werror -Wextra -DLIKWID_PERFMON -g
CXXFLAGS += -Wno-unused-variable -Wno-unused-function

EXES = tlbtest htable list deque sort mmap
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
	$(CXX) $^ -o $@ -L/usr/local/lib -llikwid -lm

addr:	addr.o
	$(CXX) $(CXXFLAGS) addr.o -o $@

clean:
	rm -f *.o $(EXES)
