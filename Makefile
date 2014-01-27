
#DEBUG=
DEBUG=-g
CPPFLAGS=-Wnon-virtual-dtor 
#CPPFLAGS=-Wnon-virtual-dtor -no-integrated-cpp -wrapper ./wrapper.sh
INCLUDES=-I./include -I./qiconn/include
PREFIX=/usr/local
SHELL=/bin/sh
VERSION=0.0.6

default: all

all: hcpp2cpp bulkrays

allstrip: all
	strip bulkrays hcpp2cpp

vimtest: all
	# ddd -args ./bulkrays --bind=127.0.0.1:10080 --user=$$USER --access_log=access_log --earlylog --console --p tagazou zonzon=2 p=3 BulkRays::ownsetoftests &
	# exit 0
	# ./hcpp2cpp testbb.hcpp -o testbb.cpp
	./bulkrays --bind=127.0.0.1:10080 --user=$$USER		\
	    --debugschedest --debugconstructor			\
	    --access_log=access_log --earlylog --console	\
	    --p tagazou zonzon=2 p=3				\
	    BulkRays::ownsetoftests				\
	    2>&1 | unbuffer -p tr ':' '='

hcpp2cpp: hcpp2cpp.cpp
	g++ -Wall -o hcpp2cpp hcpp2cpp.cpp

bulkrays: bulkrays.o qiconn/qiconn.o testsite.o bootstrap.o testsite.o simplefmap.o include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${CPPFLAGS} ${INCLUDES} -Wall -o  bulkrays bulkrays.o qiconn/qiconn.o bootstrap.o testsite.o simplefmap.o

bulkrays.o: bulkrays.cc include/bulkrays/bulkrays.h qiconn/include/qiconn/qiconn.h
	g++ ${DEBUG} ${CPPFLAGS} ${INCLUDES} -DBULKRAYSVERSION="\"${VERSION}\"" -Wall -c bulkrays.cc


qiconn/qiconn.o: qiconn/qiconn.cpp qiconn/include/qiconn/qiconn.h
	( export MAKEDEBUG=${DEBUG} ; cd qiconn ; make qiconn.o )


include/bulkrays/bulkrays.h: qiconn/include/qiconn/qiconn.h

clean:
	rm -f *.cc
	rm -f *.o bulkrays hcpp2cpp
	rm -rf bulkrays-doc
	( export MAKEDEBUG=${DEBUG} ; cd qiconn ; make clean )

distclean: clean

doc: include/bulkrays/*.h *.cpp bulkrays.dox
	doxygen bulkrays.dox

.PHONY: clean

.PHONY: distclean


bootstrap.o: bootstrap.cpp include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${CPPFLAGS} ${INCLUDES} -Wall -c bootstrap.cpp

testsite.o: testsite.cpp include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${CPPFLAGS} ${INCLUDES} -DBULKRAYSVERSION="\"${VERSION}\"" -Wall -c testsite.cpp

simplefmap.o: simplefmap.cc include/bulkrays/bulkrays.h
	g++ ${DEBUG} ${CPPFLAGS} ${INCLUDES} -Wall -c simplefmap.cc

# bulkrays.cpp: bulkrays.hcpp hcpp2cpp

#	./hcpp2cpp bulkrays.hcpp -o bulkrays.cpp


%.cc: %.hcpp hcpp2cpp
	./hcpp2cpp $< -o $@

