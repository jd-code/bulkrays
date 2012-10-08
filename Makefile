
#DEBUG=
DEBUG=-g
INCLUDES=-I./include
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
	g++ ${DEBUG} ${INCLUDES} -Wall -o  bulkrays bulkrays.o qiconn.o bootstrap.o testsite.o simplefmap.o

bulkrays.o: bulkrays.cpp include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${INCLUDES} -DBULKRAYSVERSION="\"${VERSION}\"" -Wall -c bulkrays.cpp


qiconn.o: qiconn.cpp include/qiconn/qiconn.h
	g++ ${DEBUG} ${INCLUDES} -Wall -c qiconn.cpp


include/bulkrays/bulkrays.h: include/qiconn/qiconn.h

clean:
	rm -f *.o bulkrays

distclean: clean

doc: *.h *.cpp bulkrays.dox
	doxygen bulkrays.dox

.PHONY: clean

.PHONY: distclean


bootstrap.o: bootstrap.cpp include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${INCLUDES} -Wall -c bootstrap.cpp

testsite.o: testsite.cpp include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${INCLUDES} -DBULKRAYSVERSION="\"${VERSION}\"" -Wall -c testsite.cpp

simplefmap.o: simplefmap.cpp include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${INCLUDES} -Wall -c simplefmap.cpp

