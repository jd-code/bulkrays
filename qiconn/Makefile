
DEBUG=
#DEBUG=-g
INCLUDES=-I./include
PICTARGET=-fpic
PREFIX=/usr/local
SHELL=/bin/sh
VERSION=1.0.0

default: all

all: qiconn.o libqiconn.a libqiconn.so


vimtest: all
	### ./bulkrays --bind=127.0.0.1:10080 --user=$$USER
	### ddd --args ./qigong    -pidfile=/tmp/qigongbuild.pid -logfile=testqigong.log -debugout -port 1364 -nofork &


qiconn_dyn.o: qiconn.cpp include/qiconn/qiconn.h
	g++ ${DEBUG} ${INCLUDES} ${PICTARGET} -Wall -c -o qiconn_dyn.o qiconn.cpp

qiconn.o: qiconn.cpp include/qiconn/qiconn.h
	g++ ${DEBUG} ${INCLUDES} -Wall -c qiconn.cpp

libqiconn.a: qiconn.o
	ar  rcs libqiconn.a qiconn.o

libqiconn.so: qiconn_dyn.o
	g++ -shared -o libqiconn.so qiconn_dyn.o

clean:
	rm -f *.o *.a *.so

distclean: clean

doc: *.h *.cpp bulkrays.dox
	doxygen qiconn.dox

.PHONY: clean

.PHONY: distclean

