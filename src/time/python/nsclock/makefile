all:	nsclock

nsclock:	nsclock.c
	python2.7 setup.py build

install:	all
	sudo /usr/local/bin/python2.7 setup.py install

clean:
	rm -rf build/

