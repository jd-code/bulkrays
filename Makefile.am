

ACLOCAL_AMFLAGS = -I m4

vimtest: all
	## ddd -args ./bulkrays --bind=127.0.0.1:10080 --user=$$USER --access_log=access_log --earlylog --console --p tagazou zonzon=2 p=3 BulkRays::ownsetoftests &
	## exit 0
	## ./hcpp2cpp testbb.hcpp -o testbb.cpp
	./bulkrays --bind=127.0.0.1:10080 --user=$$USER		\
	    --access_log=access_log --earlylog --console	\
	    --p tagazou zonzon=2 p=3				\
	    BulkRays::ownsetoftests				\
	    2>&1 | unbuffer -p tr ':' '='

bin_PROGRAMS = hcpp2cpp bulkrays

hcpp2cpp_SOURCES = hcpp2cpp.cpp

bulkrays_SOURCES = testsite.cpp bootstrap.cpp simplefmap.cc bulkrays.cc
bulkrays_LDADD = @QICONNSOURCE@/qiconn.o

		   
%.cc: %.hcpp hcpp2cpp
	./hcpp2cpp $< -o $@

doc: include/bulkrays/*.h *.cpp bulkrays.dox
	doxygen bulkrays.dox

CLEANFILES = bulkrays.cc simplefmap.cc

clean-local:
	rm -rf bulkrays-doc

distclean-local:
	rm m4/*.m4


@QICONNSOURCE@/qiconn.o:
	cd @QICONNSOURCE@ && make qiconn.o

