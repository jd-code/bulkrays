##
## Bulkrays Copyright (C) 2012-2020 Jean-Daniel Pauget
## A whole set of building utilities
##
## jdbulkrayed@disjunkt.com  -  http://bulkrays.disjunkt.com/
##
## This file is part of Bulkrays.
##
## Loopsoids is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## Loopsoids is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Loopsoids; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##
## you can also try the web at http://www.gnu.org/
##



ACLOCAL_AMFLAGS = -I m4

vimtest: bulkrays_static
	## ddd -args ./bulkrays --bind=127.0.0.1:10080 --user=$$USER --access_log=access_log --earlylog --console --p tagazou zonzon=2 p=3 BulkRays::ownsetoftests &
	## exit 0
	## ./hcpp2cpp testbb.hcpp -o testbb.cpp
	./bulkrays_static --bind=0.0.0.0:10080 --user=$$USER		\
	    --access_log=access_log --earlylog --console	\
	    --p tagazou zonzon=2 p=3				\
	    BulkRays::ownsetoftests				\
	    2>&1 | unbuffer -p tr ':' '='
##	    --debugparsereq \


pkginclude_HEADERS = include/bulkrays/bulkrays.h

bin_PROGRAMS = hcpp2cpp bulkrays_static bulkrays

hcpp2cpp_SOURCES = hcpp2cpp.cpp

bulkrays_static_SOURCES = testsite.cpp bootstrap.cpp simplefmap.cc bulkrays.cc somehash.cpp digest.cpp
bulkrays_static_CXXFLAGS = @LOCALINCPATH@ @LIBMHASH_CFLAGS@
bulkrays_static_LDADD = @QICONNLDLIB@
bulkrays_static_LDFLAGS = @QICONNLDFLAGS@  @LIBMHASH_LDFLAGS@

bulkrays_SOURCES = testsite.cpp bootstrap.cpp simplefmap.cc somehash.cpp digest.cpp
bulkrays_CXXFLAGS = @LOCALINCPATH@ @LIBMHASH_CFLAGS@
bulkrays_LDADD = @QICONNLDLIB@ @LIBMHASH_LDFLAGS@ libbulkrays.a
bulkrays_LDFLAGS = @QICONNLDFLAGS@

lib_LTLIBRARIES = libbulkrays.la
noinst_LIBRARIES = libbulkrays.a

libbulkrays_la_SOURCES = bulkrays.cc somehash.cpp digest.cpp
libbulkrays_la_CPPFLAGS = @LOCALINCPATH@ $(AM_CPPFLAGS)
libbulkrays_la_LDFLAGS = @QICONNLDFLAGS@

libbulkrays_a_SOURCES = bulkrays.cc somehash.cpp digest.cpp
libbulkrays_a_CPPFLAGS = @LOCALINCPATH@ $(AM_CPPFLAGS)
libbulkrays_a_LDFLAGS = @QICONNLDFLAGS@

		   
%.cc: %.hcpp hcpp2cpp
	./hcpp2cpp $< -o $@

doc: include/bulkrays/*.h *.cpp bulkrays.dox
	doxygen bulkrays.dox

CLEANFILES = bulkrays.cc simplefmap.cc access_log

clean-local:
	rm -rf bulkrays-doc

distclean-local:
	rm m4/*.m4


