#ifndef INCL_BULKRAYS_H
#define INCL_BULKRAYS_H

#include <iomanip>

#ifdef BULKRAYS_H_GLOBINST
#define BULKRAYS_H_SCOPE
#else
#define BULKRAYS_H_SCOPE extern
#endif


#include <qiconn/qiconn.h>

namespace bulkrays {
    using namespace std;
    using namespace qiconn;

#ifdef BULKRAYS_H_GLOBINST
    BULKRAYS_H_SCOPE bool debugparsereq = false;
    BULKRAYS_H_SCOPE bool debugearlylog = false;
#else
    BULKRAYS_H_SCOPE bool debugparsereq;
    BULKRAYS_H_SCOPE bool debugearlylog;
#endif

    int percentdecode (const string &src, string &result);
    int percentdecodeform (const string &src, string &result);
    string xmlencode (const string &s);
    void xmlencode (ostream & cout, const string &s);

    inline ostream& bendl (ostream& out) {
	return out << (char) 13 << (char) 10;
    }

    class MimeTypes : protected map<string, string> {
	public:
	    MimeTypes () {}
	    MimeTypes (const char* fname);
	    ~MimeTypes () {}
	    const string * getmimefromterminaison (const string &term);
	    const string * getmimefromterminaison (const char *term);
    };

    typedef map<string, string> FieldsMap;
    typedef FieldsMap MimeHeader;

    class FieldsMapR : public map<string, const string&> {
	public:
	    const char * name;
	    FieldsMapR (const char * name) : name(name) {}
	    ~FieldsMapR () {}
	    void import (FieldsMap const &m);
    };

    class BodySubEntry {
	public:
	    string const &body, contenttype;
	    size_t begin, length;
	    FieldsMap mime;
	    BodySubEntry (string contenttype, string const &body, size_t begin, size_t length, FieldsMap const &mime) : body(body), contenttype(contenttype), begin(begin), length(length), mime(mime) {};
	    BodySubEntry (BodySubEntry const &b) : body(b.body), contenttype(b.contenttype), begin(b.begin), length(b.length), mime(b.mime) {};
	    ~BodySubEntry () {}
    };


    string fetch_localcr (const string &s, size_t p=0);
    int read_mimes_in_string (string const &s, FieldsMap &mime, string const &lcr, size_t &p, size_t l = string::npos);

    int populate_reqfields_from_urlencodebody (const string& body, FieldsMap &reqfields, size_t p=0);
    int populate_reqfields_from_uri (const string& uri, string &document_uri, FieldsMap &reqfields);

    class HttppConn;

    class HTTPRequest
    {
	public:
	    HttppConn *pdummyconnection;
	    int statuscode;
	    const char* errormsg;
	    const char* suberrormsg;
	    MimeHeader outmime;
	    bool expires_set;
	    bool headerpublished;

	    string method;
	    string host;
	    string req_uri;
	    string document_uri;
	    string version;
	    MimeHeader mime;


	    size_t reqbodylen;
	    size_t readbodybytes;
	    string req_body;

	    FieldsMapR	req_fields;
	    FieldsMap	uri_fields,
			body_fields;
	    map <string, BodySubEntry> content_fields;

static ostream * clog;

	    void logger (void);

	    ostream& errlog (void);

	    void set_relative_expires (time_t seconds);
	    void set_relative_expires_jitter (size_t seconds, float jitter = 7.0);
	    void set_contentlength (size_t l);
	    void publish_header (void);

	    void initoutmime (void);

	    void clear (void) {
		 expires_set = false;
	     headerpublished = false;
		      method.clear();
			host.clear();
		     req_uri.clear();
		document_uri.clear();
		     version.clear();
			mime.clear();
		  statuscode = 0;
		    errormsg = "OK";
		 suberrormsg = "";
		initoutmime ();

		  reqbodylen = 0;
	       readbodybytes = 0;
		    req_body.clear();

		  req_fields.clear();
		  uri_fields.clear();
		 body_fields.clear();
	      content_fields.clear();
	    }
	    HTTPRequest (HttppConn &dc) :
		pdummyconnection(&dc),
		statuscode(0),
		errormsg(NULL),
		suberrormsg(NULL),
		expires_set(false),
		headerpublished(false),
		req_fields("req_fields") {
		initoutmime ();
	    }
	    ~HTTPRequest ();
    };

    class BulkRaysCPool : public ConnectionPool {
	protected:
	    virtual void treat_signal (void);
	public:
	    BulkRaysCPool (void) : ConnectionPool () {}
	    virtual ~BulkRaysCPool () {}
	    virtual int select_poll (struct timeval *timeout) {
		return ConnectionPool::select_poll (timeout);
	    }
    };

    BULKRAYS_H_SCOPE BulkRaysCPool bulkrayscpool;

#ifdef BULKRAYS_H_GLOBINST
    BULKRAYS_H_SCOPE
    const string xhtml_header ("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			       "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
			       "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");

    BULKRAYS_H_SCOPE
    const string bomb64_gif ("R0lGODdhQABAAIABAP///wAAACwAAAAAQABAAAAC6YSPCcHNCqOcNDlXs97mNg6GlheI5vgcGLSe"
			     "YLt8rOxyMIzgdXXTaLkLanTCYixlTB6BSiMx+HxGpKKoj3etWZEZamjL7GaV3iXpHN6V0ey0dvxr"
			     "X5zwnJzUFN/necre0+f3V5Y3SPhmaEiXOLjIuOf4KKcmWWlSaVmF+Xi5ydjpqagZ2jhKCml6Ormh"
			     "ionVyikI+yk7Kzoxa4eKC6t796rqu8rbKtxmQ2oMmOqqcsyc2fH8lezMBiqpjIYdK7370uz9C31r"
			     "Nn1iW0d+GtlblM7VHt3HHmg+X1htP3O9z3rur9aZgEMAiigAADs=");

    BULKRAYS_H_SCOPE MimeTypes mimetypes("/etc/mime.types");
#else
    BULKRAYS_H_SCOPE const string xhtml_header;
    BULKRAYS_H_SCOPE const string bomb64_gif;
    BULKRAYS_H_SCOPE MimeTypes mimetypes;
#endif

    BULKRAYS_H_SCOPE
    map <int,const char *> status_message;

    class TreatRequest {
	public:
	    TreatRequest () {}
	    ~TreatRequest () {}
	    virtual int output (ostream &cout, HTTPRequest &req) = 0;
	    int error (ostream &cout, HTTPRequest &req, int statuscode, const char* message=NULL, const char* submessage=NULL, bool closeconnection=false);
	    inline int errorandclose (ostream &cout, HTTPRequest &req, int statuscode, const char* message=NULL, const char* submessage=NULL) {
		return error (cout, req, statuscode, message, submessage, true);
	    }
    };

    class ReturnError : virtual public TreatRequest {
	    bool closeconnection;
	public:
	    ReturnError () : TreatRequest (), closeconnection(true) {}
	    ~ReturnError () {}
	    
	    virtual int output (ostream &cout, HTTPRequest &req);
	    int shortcuterror (ostream &cout, HTTPRequest &req, int statuscode, const char* message=NULL, const char* submessage=NULL, bool closeconnection=true);
    };

    BULKRAYS_H_SCOPE ReturnError returnerror;

    class URIMapper {
	public:
	    URIMapper () {}
	    ~URIMapper () {}
	    virtual TreatRequest* treatrequest (HTTPRequest &req) = 0;
    };

    // each host has it's own URIMapper
    typedef map <string, URIMapper*> THostMapper;
    BULKRAYS_H_SCOPE THostMapper hostmapper;
#ifdef BULKRAYS_H_GLOBINST
    BULKRAYS_H_SCOPE bool wehaveadefaulthost = false;
    BULKRAYS_H_SCOPE THostMapper::iterator mi_defaulthost = hostmapper.end();
#else
    BULKRAYS_H_SCOPE bool wehaveadefaulthost;
    BULKRAYS_H_SCOPE THostMapper::iterator mi_defaulthost;
#endif

    int bootstrap_global (void);

    class HttppConn : public SocketConnection
    {
	public:
static int idnum;

	    typedef enum {
		HTTPRequestLine,	// waiting for an http Request-Line
		MIMEHeader,		// first mime-entry of mime header
		NextMIMEHeader,		// either continuing a mime value or next mime entry
//		MessageBody,		// starting the datas following mime header (Content-Length troubles)
		ReadBody,		// buffering the body of the message itself
		NowTreatRequest,	// all message suposely read, we treat ...
		WaitingEOW		// the treatment is finished w're waiting for the end of normal transmission
	    } State;
	    State state;

	    timeval entering_HTTPRequestLine,
		    entering_ReadBody,
		    entering_NowTreatRequest,
		    entering_WaitingEOW,
		    ending_WaitingEOW;

	    string mimevalue, mimeheadername;
	    size_t lastbwindex;

	    HTTPRequest request;
	    int id;

	    void compute_reqbodylen (void);
	    ostream& shorterrlog (void);
	    ostream& errlog (void);

	    virtual ~HttppConn (void);
	    HttppConn (int fd, struct sockaddr_in const &client_addr);
	    virtual void lineread (void);
	    virtual void poll (void) {};
	    virtual void eow_hook (void);
	    virtual void reconnect_hook (void);
    };
#ifdef BULKRAYS_H_GLOBINST
int HttppConn::idnum = 0;
#endif


    class HttppBinder : public ListeningSocket
    {
	public:
	    virtual ~HttppBinder (void) {}
	    HttppBinder (int fd, int port, const char * addr = NULL) : ListeningSocket (fd, "httppbinder") {
		stringstream newname;
		if (addr == NULL)
		    newname << "*";
		else
		    newname << addr;
		newname << ":" << port;
		setname (newname.str());
	    }
	    virtual SocketConnection* connection_binder (int fd, struct sockaddr_in const &client_addr) {
		return new HttppConn (fd, client_addr);
	    }
	    virtual void poll (void) {}
    };

}

#endif // INCL_BULKRAYS_H

