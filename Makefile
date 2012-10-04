
#DEBUG=
DEBUG=-g
PREFIX=/usr/local
SHELL=/bin/sh
VERSION=0.0.3

default: all

all: bulkrays

allstrip: all
	strip bulkrays


vimtest: all
	./bulkrays --bind=127.0.0.1:10080 --user=$$USER
	### ddd --args ./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 -nofork &

bulkrays: bulkrays.o qiconn.o testsite.o bootstrap.o testsite.o simplefmap.o
	g++ ${DEBUG} -Wall -o  bulkrays bulkrays.o qiconn.o bootstrap.o testsite.o simplefmap.o

bulkrays.o: bulkrays.cpp qiconn.h bulkrays.h
	g++ ${DEBUG} -DBULKRAYSVERSION="\"${VERSION}\"" -Wall -c bulkrays.cpp


qiconn.o: qiconn.cpp qiconn.h
	g++ ${DEBUG} -Wall -c qiconn.cpp


clean:
	rm -f *.o bulkrays

distclean: clean

doc: *.h *.cpp bulkrays.dox
	doxygen bulkrays.dox

.PHONY: clean

.PHONY: distclean


bootstrap.o: bootstrap.cpp bulkrays.h qiconn.h
	g++ ${DEBUG} -DBULKRAYSVERSION="\"${VERSION}\"" -Wall -c bootstrap.cpp

testsite.o: testsite.cpp bulkrays.h qiconn.h
	g++ ${DEBUG} -DBULKRAYSVERSION="\"${VERSION}\"" -Wall -c testsite.cpp

simplefmap.o: simplefmap.cpp bulkrays.h qiconn.h
	g++ ${DEBUG} -DBULKRAYSVERSION="\"${VERSION}\"" -Wall -c simplefmap.cpp

