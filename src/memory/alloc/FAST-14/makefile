#CXX := g++
CXX := /opt/rh/devtoolset-4/root/usr/bin/g++
CXXFLAGS := -O3 -Wall -Wextra -std=c++11  -fdiagnostics-color=auto -pedantic\
	-Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
HOME := /home/merrital
LDFLAGS := -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu -L/usr/local/include -L$(HOME)/local/lib
LIBS := -lredox -lev

DEPS := makefile
TARGETS := libctest jetest tctest sutest hoardtest redistest

.PHONY:	all
all:	$(TARGETS)

malloc.o:	malloc.cc $(DEPS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

libctest:	malloc.o
	$(CXX) -o $@ $< $(LDFLAGS) $(LIBS)

jetest:	malloc.o
	$(CXX) -o $@ $< $(LDFLAGS) -ljemalloc $(LIBS)

tctest:	malloc.o
	$(CXX) -o $@ $< $(LDFLAGS) -ltcmalloc_minimal $(LIBS)

sutest:	malloc.o
	$(CXX) -o $@ $< $(LDFLAGS) -lsupermalloc $(LIBS)

hoardtest:	malloc.o
	$(CXX) -o $@ $< $(LDFLAGS) -lhoard $(LIBS)

redistest:	libctest
	ln -sf $< $@

clean:
	rm -f *.o $(TARGETS)

.PHONY:	run
run:
	@./libctest
	@./jetest
	@./tctest
	@./sutest
	@./hoardtest
