#ifndef INCL_BULKRAYS_H
#define INCL_BULKRAYS_H

#include <iomanip>
#include <vector>

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
    BULKRAYS_H_SCOPE bool debugparsebody = false;
    BULKRAYS_H_SCOPE bool debugearlylog = false;
#else
    BULKRAYS_H_SCOPE bool debugparsereq;
    BULKRAYS_H_SCOPE bool debugparsebody;
    BULKRAYS_H_SCOPE bool debugearlylog;
#endif

    class Property {
	private:
	    bool b;
	    int i;
	    string s;
	public:
	    Property () : b(false), i(0) {}
	    Property (const string &s);
	    inline operator bool(void) { return b; }
	    inline operator int(void) { return i; }
	    inline operator const string & (void) { return s; }
    };

    class PropertyMap : public map <string, Property> {
	public:
    };

#ifdef BULKRAYS_H_GLOBINST
    BULKRAYS_H_SCOPE PropertyMap properties;
#else
    BULKRAYS_H_SCOPE PropertyMap properties;
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

    // typedef map<string, string> FieldsMap;
    class FieldsMap : public map<string, string> {
	public:
	    bool match (const string &k, const string &v) const;
	    bool notempty (const string &k) const;
	    bool notempty (const string &k, FieldsMap::iterator &mi);
	    bool verif (const string &, FieldsMap::iterator& mi);
    };


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
	    HttppConn *httppconn;
	    int statuscode;
	    const char* errormsg;
	    const char* suberrormsg;
	    MimeHeader outmime;
	    bool expires_set;
	    bool headerpublished;

	    string method;		//  GET PUT POST etc ....
	    string host;		//  fqdn|ip[:port] 
	    string req_uri;		//  /tagazon/file.htm?truc=a&thingy=b ....
	    string document_uri;	//  /tagazon/file.htmm
	    string version;		//  HTTP/1.1  ~  HTTP/1.0
	    MimeHeader mime;


	    size_t reqbodylen;
	    size_t readbodybytes;
	    string req_body;

	    FieldsMapR	req_fields;
	    FieldsMap	uri_fields,
			body_fields;
	    map <string, BodySubEntry> content_fields;
	    FieldsMap	cookies;

	    bool cookiescooked, cookiescookedresult;

static ostream * clog;

	    void logger (void);

	    ostream& errlog (void);

	    void set_relative_expires (time_t seconds);
	    void set_relative_expires_jitter (size_t seconds, float jitter = 7.0);
	    void set_contentlength (size_t l);
	    void publish_header (void);

	    bool cookcookies (void);

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
		     cookies.clear();
	    }
	    HTTPRequest (HttppConn &dc) :
		httppconn(&dc),
		statuscode(0),
		errormsg(NULL),
		suberrormsg(NULL),
		expires_set(false),
		headerpublished(false),
		req_fields("req_fields"),
		cookiescooked(false),
		cookiescookedresult(false)
	    {
		initoutmime ();
	    }
	    ~HTTPRequest ();

	    ostream& dump (ostream& out) const;
    };


    inline ostream& operator<< (ostream& out, HTTPRequest const &op) {
	return op.dump (out);
    }

    class BulkRaysCPool : public ConnectionPool {
	protected:
	    virtual void treat_signal (void);
	public:
	    BulkRaysCPool (void) : ConnectionPool () {}
	    virtual ~BulkRaysCPool () {}
	    virtual int select_poll (struct timeval *timeout) {
		return ConnectionPool::select_poll (timeout);
	    }
	    void askforexit (const char * reason);
    };

    void sillyconsolelineread (BufConnection &bcin, BufConnection &bcout, BulkRaysCPool *bcp);

    class SillyConsole : public BufConnection {
	protected:
	    BulkRaysCPool *bcp;
	    string name;
	public:
	    virtual ~SillyConsole (void);
	    SillyConsole (int fd, BulkRaysCPool *bcp);
	    virtual void lineread (void) {
		sillyconsolelineread (*this, *this, bcp);
	    }
	    inline virtual string getname (void) {
		return name;
	    }
    };

    class SillyConsoleOut : public BufConnection {
	public:
	    virtual ~SillyConsoleOut (void) {};
	    SillyConsoleOut (int fd) : BufConnection(fd) {};
	    virtual void lineread (void) {}
	    inline virtual string getname (void) {
		return "SillyConsoleOut";
	    }
	    virtual const char * gettype (void) { return "SillyConsoleOut"; }
    };

    class SillyConsoleIn : public BufConnection {
	protected:
	    SillyConsoleOut *sco;
	    BulkRaysCPool *bcp;
	    string name;
	public:
	    virtual ~SillyConsoleIn (void);
	    SillyConsoleIn (int fd, SillyConsoleOut *sco, BulkRaysCPool *bcp);
	    virtual void lineread (void) {
		sillyconsolelineread (*this, *sco, bcp);
	    }
	    inline virtual string getname (void) {
		return name;
	    }
	    virtual const char * gettype (void) { return "SillyConsoleIn"; }
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

    typedef enum {
	TRPending,
	TRCompleted
    } TReqResult;

    class TreatRequest {
	public:
	    TreatRequest () {}
	    virtual ~TreatRequest () {}
	    virtual TReqResult output (ostream &cout, HTTPRequest &req) = 0;
	    TReqResult error (ostream &cout, HTTPRequest &req, int statuscode, const char* message=NULL, const char* submessage=NULL, bool closeconnection=false);
	    inline int errorandclose (ostream &cout, HTTPRequest &req, int statuscode, const char* message=NULL, const char* submessage=NULL) {
		return error (cout, req, statuscode, message, submessage, true);
	    }
    };

    class ReturnError : virtual public TreatRequest {
	    bool closeconnection;
	public:
	    ReturnError () : TreatRequest (), closeconnection(true) {}
	    virtual ~ReturnError () {}
	    
	    virtual TReqResult output (ostream &cout, HTTPRequest &req);
	    int shortcuterror (ostream &cout, HTTPRequest &req, int statuscode, const char* message=NULL, const char* submessage=NULL, bool closeconnection=true);
    };

    BULKRAYS_H_SCOPE ReturnError returnerror;

    class URIMapper {
	public:
	    URIMapper () {}
	    virtual ~URIMapper () {}
	    virtual TreatRequest* treatrequest (HTTPRequest &req) = 0;
    };

    // each host has it's own URIMapper

    // typedef map <string, URIMapper*> THostMapper;
    class THostMapper : public map <string, URIMapper*> {
	public:
	    bool deregisterall (URIMapper* up);
    };




    BULKRAYS_H_SCOPE THostMapper hostmapper;
#ifdef BULKRAYS_H_GLOBINST
    BULKRAYS_H_SCOPE bool wehaveadefaulthost = false;
    BULKRAYS_H_SCOPE THostMapper::iterator mi_defaulthost = hostmapper.end();
#else
    BULKRAYS_H_SCOPE bool wehaveadefaulthost;
    BULKRAYS_H_SCOPE THostMapper::iterator mi_defaulthost;
#endif


    bool set_default_host (string const &hostname);
    void unset_default_host (void);

    typedef enum {
	StartUp,
	ShutDown
    } BSOperation;

    int bootstrap_global (BSOperation op);

    inline ostream& operator<< (ostream& out, BSOperation const &op) {
	switch (op) {
	    case StartUp:
		return out << "Startup";
	    case ShutDown:
		return out << "ShutDown";
	}
    }

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
		TreatPending,		// we're waiting a call-back from treatment that will finish pushing the output
		WaitingEOW,		// the treatment is finished we're waiting for the end of normal transmission
		BuggyConn		// the connection is considered buggy no further reading should be performed and close asap
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
	    virtual const char * gettype (void) { return "HttppConn"; }

	    void finishtreatment (void);
    };
#ifdef BULKRAYS_H_GLOBINST
int HttppConn::idnum = 0;
    BULKRAYS_H_SCOPE ofstream cnull("/dev/null");
    BULKRAYS_H_SCOPE HttppConn httpconnnull (-1, sockaddr_in());
    BULKRAYS_H_SCOPE HTTPRequest reqnull(httpconnnull);
#else
    BULKRAYS_H_SCOPE ofstream cnull;
    BULKRAYS_H_SCOPE HTTPRequest reqnull;
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
	    virtual const char * gettype (void) { return "HttppBinder"; }
    };

    class ASyncCallBack {
	public:
	    ASyncCallBack () {}
	    virtual ~ASyncCallBack () {}
	    virtual int callback (int v) = 0;
    };

    class HTTPResponse
    {
	public:
	    // HttppConn *httppconn;
	    bool fulltransmission;
	    int statuscode;
	    const char* errormsg;
	    const char* suberrormsg;
	    // MimeHeader outmime;
	    // bool expires_set;
	    // bool headerpublished;

	    string method;
	    string host;
	    string req_uri;
	    string document_uri;
	    string version;
	    MimeHeader mime;


	    size_t reqbodylen;
	    size_t readbodybytes;
	    string req_body;

//	    FieldsMapR	req_fields;
//	    FieldsMap	uri_fields,
//			body_fields;
//	    map <string, BodySubEntry> content_fields;

static ostream * clog;

	    void logger (void);

	    ostream& errlog (void);

//	    void set_relative_expires (time_t seconds);
//	    void set_relative_expires_jitter (size_t seconds, float jitter = 7.0);
//	    void set_contentlength (size_t l);
//	    void publish_header (void);

//	    void initoutmime (void);

	    void clear (void) {
	    fulltransmission = false;
		  statuscode = 0;
		    errormsg = NULL;
		 suberrormsg = NULL;
		      method.clear();
			host.clear();
		     req_uri.clear();
		document_uri.clear();
		     version.clear();
			mime.clear();

		  reqbodylen = 0;
	       readbodybytes = 0;
		    req_body.clear();
	    }

	    HTTPResponse () :
	    fulltransmission(false),
		statuscode(0),
		errormsg(NULL),
		suberrormsg(NULL)
		{}
	    ~HTTPResponse () {}
	    void compute_bodylen (void);

	    ostream& dump (ostream& out) const;
    };

    class HTTPConn {
	public:
	    string host;
	    int port;

	    inline HTTPConn () : port (0) {}
	    HTTPConn (HTTPConn const &o) {
		host = o.host;
		port = o.port;
	    }

	    HTTPConn& operator= (HTTPConn const &o) {
		host = o.host;
		port = o.port;
		return *this;
	    } 
    };
    
    class SplitUrl {
	public:
	    string user;
	    string pass;
	    
	    HTTPConn hostport;

	    string uripath;
	    bool userpass;  // do we have a user (+pass) ?

	    SplitUrl (void): userpass(false) {}
	    SplitUrl (const string &url);

	    operator string () const;

	    SplitUrl (SplitUrl const &o) {
		user = o.user;
		pass = o.pass;
		hostport = o.hostport;
		uripath = o.uripath;
		userpass = o.userpass;
	    }
	    SplitUrl& operator= (SplitUrl const &o) {
		user = o.user;
		pass = o.pass;
		hostport = o.hostport;
		uripath = o.uripath;
		userpass = o.userpass;
		return *this;
	    }
    };

    class HTTPClient : public BufConnection {

/*
 *  missing here :
 *	- should log bad connections attempt (connect failed)
 *	- when not registered to a collection pool, the query is silently lost !
 *	- refuse query while already querying ...
 *	- keepalive
 *	- cleaning between subsequent query (particularly the presponse pointer ...)
 *	- bug at premeture closure from the other side ...
 *	- bug at closure from the other side ...
 */


	typedef enum {
	    creation,
	    waitingHTTPline1,
	    mimeheadering,
	    nextmimeheadering,
	    docreceiving,
	    waiting_cbtreat
	} HTTPClientStatus;
	protected:
	    HTTPClientStatus state;
	    bool keepalive;
	    time_t timeout;
	    ASyncCallBack *ascb;
	    int callbackvalue;
	    string prevhost;
	    stringstream raw;

	    string mimevalue, mimeheadername;	// JDJDJDJD where is the cleaning between two reqs ?
	    SplitUrl curspliturl;		// JDJDJDJD where is the cleaning between two reqs ?

	public:
	    HTTPResponse * presponse;

	public:
	    HTTPClient (bool keepalive = true);
	    virtual ~HTTPClient (void);
	    bool http_get (const SplitUrl &spurl, HTTPResponse &response, ASyncCallBack *ascb = NULL, int callbackvalue = -1);
	    bool http_post_urlencoded (const SplitUrl &spurl, FieldsMap& vals, HTTPResponse &response, ASyncCallBack *ascb = NULL, int callbackvalue = -1);

	    inline bool http_get (const string &url, HTTPResponse &response, ASyncCallBack *ascb = NULL, int callbackvalue = -1) {
		SplitUrl spurl(url);
		return http_get (spurl, response, ascb, callbackvalue);
	    }
	    inline bool http_post_urlencoded (const string &url, FieldsMap& vals, HTTPResponse &response, ASyncCallBack *ascb = NULL, int callbackvalue = -1) {
		SplitUrl spurl(url);
		return http_post_urlencoded (spurl, vals, response, ascb, callbackvalue);
	    }

	    virtual void lineread (void);
	    virtual string getname (void);
	    virtual const char * gettype (void) { return "HTTPClient"; }
	    ostream& errlog (void);
	    ostream& shorterrlog (void);
	    virtual void reconnect_hook (void);
    };

    class HTTPClientPool {
	protected:
	    vector <HTTPClient*> vhc;
	    list <HTTPClient*> available_hc;
	    map <string, HTTPClient*> mhc;
	    int maxpool;
	public:
	    HTTPClientPool (int maxpool);


    };

}

#endif // INCL_BULKRAYS_H

