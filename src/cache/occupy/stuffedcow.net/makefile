all:	lat lat2 lat3

%.o:	%.cc
	g++ -O2 -c $< -o $@

lat:	lat.o
	g++ $< -o $@

lat2:	lat2.o
	g++ $< -o $@

lat3:	lat3.o
	g++ $< -o $@

clean:
	rm -f *.o lat lat2 lat3
