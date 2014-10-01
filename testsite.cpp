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
	    return 0;

	  case ShutDown:
	    if (mapper == NULL) return -1;
	    hostmapper.deregisterall (mapper);
	    delete (mapper);
	    return 0;
	}
	return -1;
    }
}
