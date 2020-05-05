/*
 * Bulkrays Copyright (C) 2012-2020 Jean-Daniel Pauget
 * A whole set of building utilities
 *
 * jdbulkrayed@disjunkt.com  -  http://bulkrays.disjunkt.com/
 *
 * This file is part of Bulkrays.
 *
 * Loopsoids is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Loopsoids is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Loopsoids; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * you can also try the web at http://www.gnu.org/
 */

#include <bulkrays/bulkrays.h>

namespace bulkrays {
    using namespace std;

    class TestSite_SimpleParrot : virtual public TreatRequest {
      public:
	TestSite_SimpleParrot () : TreatRequest () {}
	~TestSite_SimpleParrot () {}
	virtual TReqResult output (ostream &cout, HTTPRequest &req) {
	    stringstream head;
	    // head << req.version << " 200 OK" << endl
	    head << "HTTP/1.1" << " 200 OK" << endl
		 << "Server: BulkRays/" << BULKRAYSVERSION << endl
		 << "Content-Type: text/html;charset=UTF-8" << endl
		 << "Connection: keep-alive" << endl
    //             << "Accept-Ranges: bytes" << endl
		;
	    stringstream s;
	    s << "<!DOCTYPE html " << endl
	      << "     PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"" << endl
	      << "     \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << endl
	      << "<html xmlns='http://www.w3.org/1999/xhtml'>" << endl;

	    s << "<head><title>" << req.req_uri << "</title></head>" << endl;
	    s << "<body>" << endl;
	    s << "<div>" << endl;
	    s << "<tt>" << endl;
	    s << "<div>" << req.method << "</div>" << endl
	      << "<div>" << req.req_uri << "</div>" << endl
	      << "<div>" << req.version << "</div>" << endl;
	    s << "</tt>" << endl;
	    s << "</div>" << endl;
	    
	    s << "<div>" << endl;
	    MimeHeader::iterator mi;
	    for (mi=req.mime.begin() ; mi!=req.mime.end() ; mi++)
		s << "<div><i>mime</i>[<b><tt>" << mi->first << "</tt></b>] = <tt><b>" << mi->second << "</b></tt></div>" << endl;
	    s << "</div>" << endl;

	    s << "</body>" << endl;
	    s << "</html>" << endl;

	    head << "Content-Length: " << s.str().size() << endl;
	    head << endl;
	    cout << head.str() << s.str();
	    
	    return TRCompleted;
	}
    };

    class TestSite_SimpleURIMapper : virtual public URIMapper {
	public:
	    TestSite_SimpleParrot simpleparrot;

	    TestSite_SimpleURIMapper() : URIMapper() {}
	    ~TestSite_SimpleURIMapper() {}
	    virtual TreatRequest* treatrequest (HTTPRequest &req) {
		return &simpleparrot;
	    }
    };

    TestSite_SimpleURIMapper *mapper = NULL;

    int testsite_global (BSOperation op) {
	switch (op) {
	  case StartUp:
	    mapper = new TestSite_SimpleURIMapper();
	    if (mapper == NULL) {
		cerr << "bulkrays::testsite_global could not allocate TestSite_SimpleURIMapper" << endl;
		return -1;
	    }
	    hostmapper["bulkrays.zz"] = mapper;
	    hostmapper["bulkrays.zz:10080"] = mapper;
	    return 0;

	  case ShutDown:
	    if (mapper == NULL) return -1;
	    hostmapper.deregisterall (mapper);
	    delete (mapper);
	    return 0;
	}
	return -1;  // simply to quiet warnings ...
    }
}
