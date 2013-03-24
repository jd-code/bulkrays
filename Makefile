
#DEBUG=
DEBUG=-g
CPPFLAGS=-Wnon-virtual-dtor
INCLUDES=-I./include -I./qiconn/include
PREFIX=/usr/local
SHELL=/bin/sh
VERSION=0.0.5

default: all

all: bulkrays

allstrip: all
	strip bulkrays


vimtest: all
	# ddd -args ./bulkrays --bind=127.0.0.1:10080 --user=$$USER --access_log=access_log --earlylog --console --p tagazou zonzon=2 p=3 BulkRays::ownsetoftests &
	./bulkrays --bind=127.0.0.1:10080 --user=$$USER		\
	    --access_log=access_log --earlylog --console	\
	    --p tagazou zonzon=2 p=3				\
	    BulkRays::ownsetoftests				\
	    2>&1 | unbuffer -p tr ':' '='

bulkrays: bulkrays.o qiconn/qiconn.o testsite.o bootstrap.o testsite.o simplefmap.o include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${CPPFLAGS} ${INCLUDES} -Wall -o  bulkrays bulkrays.o qiconn/qiconn.o bootstrap.o testsite.o simplefmap.o

bulkrays.o: bulkrays.cpp include/bulkrays/bulkrays.h qiconn/include/qiconn/qiconn.h
	g++ ${DEBUG} ${CPPFLAGS} ${INCLUDES} -DBULKRAYSVERSION="\"${VERSION}\"" -Wall -c bulkrays.cpp


qiconn/qiconn.o: qiconn/qiconn.cpp qiconn/include/qiconn/qiconn.h
	( cd qiconn ; make qiconn.o )


include/bulkrays/bulkrays.h: qiconn/include/qiconn/qiconn.h

clean:
	rm -f *.o bulkrays
	rm -rf bulkrays-doc
	( cd qiconn ; make clean )

distclean: clean

doc: include/bulkrays/*.h *.cpp bulkrays.dox
	doxygen bulkrays.dox

.PHONY: clean

.PHONY: distclean


bootstrap.o: bootstrap.cpp include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${CPPFLAGS} ${INCLUDES} -Wall -c bootstrap.cpp

testsite.o: testsite.cpp include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${CPPFLAGS} ${INCLUDES} -DBULKRAYSVERSION="\"${VERSION}\"" -Wall -c testsite.cpp

simplefmap.o: simplefmap.cpp include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${CPPFLAGS} ${INCLUDES} -Wall -c simplefmap.cpp

