#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include <errno.h>

#define QICONN_H_GLOBINST
#define BULKRAYS_H_GLOBINST
#include "bulkrays/bulkrays.h"

namespace bulkrays {
    using namespace std;
    using namespace qiconn;

    inline int hexfromchar (char c) {
	switch (c) {
	    case '0': return 0;
	    case '1': return 1;
	    case '2': return 2;
	    case '3': return 3;
	    case '4': return 4;
	    case '5': return 5;
	    case '6': return 6;
	    case '7': return 7;
	    case '8': return 8;
	    case '9': return 9;
	    case 'a': case 'A': return 10;
	    case 'b': case 'B': return 11;
	    case 'c': case 'C': return 12;
	    case 'd': case 'D': return 13;
	    case 'e': case 'E': return 14;
	    case 'f': case 'F': return 15;
	    default: return -1;
	}
    }

    int percentdecode (const string &src, string &result) {
	size_t i, l = src.length();
	int nberror = 0;

	for (i=0 ; i<l ; ) {
	    if (src[i] == '%') {
		if (i+2 > l) {
		    nberror++;
		    break;
		}
		int d = hexfromchar(src[i+1]);
		int u = hexfromchar(src[i+2]);
		if ((d<0) || (u<<0)) {
		    nberror++;
		    result += src[i++];
		    result += src[i++];
		    result += src[i++];
		} else {
		    result += (char)(d*16+u);
		    i += 3;
		}
	    } else {
		result += src[i++];
	    }
	}
// cerr << "percentdecode (" << src << ")=" << result << endl;
	return nberror;
    }

    int percentdecodeform (const string &src, string &result) {
	size_t i, l = src.length();
	int nberror = 0;

	for (i=0 ; i<l ; ) {
	    if (src[i] == '%') {
		if (i+2 > l) {
		    nberror++;
		    break;
		}
		int d = hexfromchar(src[i+1]);
		int u = hexfromchar(src[i+2]);
		if ((d<0) || (u<0)) {
		    nberror++;
		    result += src[i++];
		    result += src[i++];
		    result += src[i++];
		} else {
		    result += (char)(d*16+u);
		    i += 3;
		}
	    } else if (src[i] == '+') {
		result += " ", i++;
	    } else {
		result += src[i++];
	    }
	}
// cerr << "percentdecodeform (" << src << ")=" << result << endl;
	return nberror;
    }

    void FieldsMapR::import (FieldsMap const &m) {
	FieldsMap::const_iterator mi;

	for (mi=m.begin() ; mi!=m.end() ; mi++)
	    this->insert ( pair<string, const string&> (mi->first, mi->second) );
    }

    ostream & operator<< (ostream& cout, FieldsMapR const &m ) {
	FieldsMapR::const_iterator mi;
	if (m.empty()) {
	    cout << "      " << m.name << "[]={empty}" << endl;
	    return cout;
	}
	for (mi=m.begin() ; mi!=m.end() ; mi++)
	    cout << "      " << m.name << "[" << mi->first << "]=\"" << mi->second << '"' << endl;
	return cout;
    }

    ostream & operator<< (ostream& cout, map <string, BodySubEntry> const &m ) {
	map <string, BodySubEntry>::const_iterator mi;
	if (m.empty()) {
	    cout << "      content_fields[]={empty}" << endl;
	    return cout;
	}
	for (mi=m.begin() ; mi!=m.end() ; mi++) {
	    cout << "      content_fields[" << mi->first << "]=\"" << mi->second.contenttype << '"' << endl;
//	    cout << "                            {" << mi->second.body.substr(mi->second.begin, mi->second.length) << "}" << endl;
	}
	return cout;
    }


    int populate_reqfields_from_urlencodebody (const string& body, FieldsMap &reqfields, size_t p /* =0 */) {
	int nberror = 0;

	size_t q = p,
	       l = body.size();
	while (q < l) {
// cerr << "----(" << body.substr(q) << ")------------------" << endl;
	    q = body.find('=', q);
	    if (q == string::npos) {
		nberror ++;
		break;
	    }
	    string ident = body.substr (p, q-p);
// cerr << "------------" << ident << endl;
	    FieldsMap::iterator mi = reqfields.find (ident);
	    if (mi != reqfields.end()) {
		cerr << "populate_reqfields_from_urlencodebody : field[" << ident << "] repopulated !" << endl;
		nberror ++;
	    }
	    p = q+1;
	    q = body.find('&', q);
	    string value;
	    if (q == string::npos) {
		nberror += percentdecodeform (body.substr (p), value);
		reqfields[ident] = value;
		break;
	    }
	    nberror += percentdecodeform (body.substr (p, q-p), value);
	    reqfields[ident] = value;
	    p = q+1;
	}

	return nberror;
    }

    string fetch_localcr (const string &s, size_t p /* =0 */) {
	size_t l = s.size();
	string lcr;

	while ((p<l) && ((s[p]==' ') || (s[p] == '\t'))) p++; // JDJDJD is this white space walking overkill ?

	if (p > l)
	    return lcr;

	if ((s[p] != 10) && (s[p] != 13)) {
	    cerr << "fetch_localcr bad carriage return at end of boundary ?" << endl;
	    return lcr;
	}
	lcr += s[p];

	p++;
	if (p > l)
	    return lcr;

	if ((s[p] != 10) && (s[p] != 13))
	    return lcr;

	if (s[p] == lcr[0])
	    return lcr;

	lcr += s[p];
	return lcr;
    }

    int read_mimes_in_string (string const &s, FieldsMap &mime, string const &lcr, size_t &p, size_t l /* = string::npos */) {
	int nberror = 0;
	if (l == string::npos)
	    l = s.size();
	size_t lcrs = lcr.size();
	string firstofmatch (":"); firstofmatch += lcr[0];

	while (p<l) {
	    if (s.substr (p,lcrs) == lcr) {  // end of mime header
		p += lcrs;
		break;
	    }

	    if (isspace(s[p])) {	    // we should not start an identifier with a space
		return nberror ++;
	    }
	    
	    size_t q = s.find_first_of(firstofmatch, p);
	    if (q == string::npos) {	    // we didn't find the ':'
		nberror ++;
		break;
	    }
	    if (s[q] != ':') {		    // we should find a : in order to build an identifier
		nberror ++;
		break;
	    }

	    string name = s.substr(p, q-p);
	    p = q+1;

	    while ((p<l) && isspace(s[p])) p++;	    // remove in-between spaces
	    q = s.find (lcr, p);
	    if (q == string::npos) {	    // we didn't find the end of line
		nberror ++;
		break;
	    }

	    mime[name] = s.substr (p, q-p);	    // JDJDJDJD we sgould check duplicate mime entries ?
	    p = q + lcrs;
	    while (p<l) {
		if (s[p] == lcr[0]) break;
		if (!isspace(s[p])) break;
		while ((p<l) && isspace(s[p])) p++;
		q = s.find(lcr, p);
		if (q == string::npos)		    // we could not find the end of line
		    return nberror ++;

		mime[name] += s.substr (p, q-p);
		p = q + lcrs;
	    }
	}
	return nberror;
    }


    int populate_reqfields_from_uri (const string& uri, string &document_uri, FieldsMap &reqfields) {
	size_t p;
	int nberror = 0;

	p = uri.find ('?');

	nberror += percentdecode (uri.substr (0,p), document_uri);

	if (p == string::npos)
	    return nberror;
	else
	    return populate_reqfields_from_urlencodebody (uri, reqfields, p+1);

	p++;
	size_t q = p,
	       l = uri.size();
	while (q < l) {
// cerr << "----(" << uri.substr(q) << ")------------------" << endl;
	    q = uri.find('=', q);
	    if (q == string::npos) {
		nberror ++;
		break;
	    }
	    string ident = uri.substr (p, q-p);
// cerr << "------------" << ident << endl;
	    FieldsMap::iterator mi = reqfields.find (ident);
	    if (mi != reqfields.end()) {
		cerr << "populate_reqfields_from_uri : field[" << ident << "] repopulated !" << endl;
		nberror ++;
	    }
	    p = q+1;
	    q = uri.find('&', q);
	    string value;
	    if (q == string::npos) {
		nberror += percentdecodeform (uri.substr (p), value);
		reqfields[ident] = value;
		break;
	    }
	    nberror += percentdecodeform (uri.substr (p, q-p), value);
	    reqfields[ident] = value;
	    p = q+1;
	}

	return nberror;
    }

    MimeTypes::MimeTypes (const char* fname) {
	ifstream f(fname);
	if (!f) {
	    int e = errno;
	    cerr << "MimeTypes::MimeTypes : could not open " << fname << " : " << e << " " << strerror (e) << endl;
	}
	while (f) {
	    char c;
	    string line;
	    while (f && (f.get(c)) && (c != 10) && (c != 13)) line += c;
	    while (f && (f.get(c)) && ((c == 10) || (c == 13))) ;
	    f.unget();

	    // remove first spaces
	    size_t p = 0;
	    size_t l = line.size();
	    while ((p < l) && (isspace(line[p]))) p++;

	    if ((p<l) && (p == '#'))	// have-we a comment line ?
		continue;

//cerr << line << endl;
	    string mime_type;
	    while ((p<l) && (!isspace(line[p])))
		mime_type += line[p++];

	    // remove the inner spaces
	    while ((p < l) && (isspace(line[p]))) p++;

	    while (p < l) {
		string term;
		while ((p<l) && (!isspace(line[p])))
		    term += tolower (line[p++]);	// terminaisons are lowered case

		while ((p < l) && (isspace(line[p]))) p++;

		if (! (term.empty() || mime_type.empty())) {
		    (*this)[term] = mime_type;
//cerr << term << " = " << mime_type << endl;
		}
	    }
	    
	}
    }

    const string * MimeTypes::getmimefromterminaison (const string &term) {
	string s;
	size_t p=0, l=term.size();
	while (p<l) s += tolower (term[p++]);
	
	map<string,string>::iterator mi = find (s);
	if (mi == end())
	    return NULL;
	else
	    return &mi->second;
    }

    const string * MimeTypes::getmimefromterminaison (const char *term) {
	if (term == NULL) return NULL;
	string s;
	const char *p = term;
	while (*p != 0) s += tolower (*p++);

	map<string,string>::iterator mi = find (s);
	if (mi == end())
	    return NULL;
	else
	    return &mi->second;
    }


    void HTTPRequest::logger (const string &msg) {
	time_t t;
	time (&t);
	struct tm tm;
	gmtime_r(&t, &tm);

//	clog << pdummyconnection->getname() << " " << method << " " << host << req_uri << " " << statuscode << " " << msg << endl;
	(*clog) << pdummyconnection->getname()
	     << " - - "	    // JDJDJDJD here stuff about authentication
	     << "[" << setfill('0') << setw(2)
		<< tm.tm_mday << '/'
		<< tm.tm_mon+1 << '/'
		<< setw(4) << tm.tm_year + 1900 << ':'
		<< setw(2) << tm.tm_hour << ':'
		<< tm.tm_min << ':'
		<< tm.tm_sec << "] \""
	     << method << " " << host << req_uri << " " << version << "\" "
	     << statuscode << " size \"";

	MimeHeader::iterator mi = mime.find("Referer");

	if (mi == mime.end())
	    (*clog) << " - ";
	else
	    (*clog) << mi->second;

	(*clog) << "\" \""
		<< msg << "\" ";

	mi = mime.find("User-Agent");
	if (mi == mime.end())
	    (*clog) << "\" - \"";
	else
	    (*clog) << mi->second;
	(*clog) << endl;
    }

    void HTTPRequest::logger (const char *msg /*=NULL*/) {
	if (msg != NULL)
	    logger (string(msg));
	else
	    logger (string(""));
//	if (msg == NULL)
//	    cout << pdummyconnection->getname() << " " << method << " " << host << req_uri << " " << statuscode << endl;
//	else
//	    cout << pdummyconnection->getname() << " " << method << " " << host << req_uri << " " << statuscode << " " << msg << endl;
    }

    char * rfc1123date_offset (char *buf, time_t offset) {
	time_t t;
	time (&t);
	t += offset;
	struct tm tm;
	// gmtime_r(&t, &tm);
	localtime_r(&t, &tm);

static const char* dayname[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char* monthname[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
	snprintf (buf, 39, "%3s, %02d %3s %4d %02d:%02d:%02d GMT",
		dayname[tm.tm_wday],
		tm.tm_mday,
		monthname[tm.tm_mon],
		tm.tm_year + 1900,
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec
	    );
	return buf;
    }

    void HTTPRequest::set_relative_expires (time_t seconds) {
	char buf[40];
	outmime["Expires"] = rfc1123date_offset (buf, seconds);
	expires_set = true;
    }

    void set_relative_expires_jitter (size_t seconds, float jitter = 7.0) {
    }

    void HTTPRequest::publish_header (void) {
	if (headerpublished) {
	    cerr << "HTTPRequest::publish_header : header already published !!!" << endl;
	    return;
	}
	char buf[40];
	ostream &cout = *(pdummyconnection->out) ;

	const char * outputerror;
	if (errormsg == NULL) {
	    map <int,const char *>::iterator mi = status_message.find(statuscode);
	    if (mi == status_message.end()) {
cerr << "HTTPRequest::publish_header : statuscode [" << statuscode << "] has no associated message" << endl;	// JDJDJDJD a typical place where host+uri should be welcomed
		outputerror = "Sorry, No Message";
	    } else {
		outputerror = mi->second;
	    }
	} else {
	    outputerror = errormsg;
	}
	cout << "HTTP/1.1 " << statuscode << ' ' 
	     << outputerror << endl
	     << "Date: " << rfc1123date_offset(buf, 0) << endl;

	MimeHeader::iterator mi;
	for (mi=outmime.begin() ; mi!=outmime.end() ; mi++)
	    cout << mi->first << ": " << mi->second << endl;

	cout << endl;
	headerpublished = true;
    }

    void HTTPRequest::set_contentlength (size_t l) {
	char buf[40];
	snprintf (buf, 39, "%ld", (long) l);
	outmime["Content-Length"] = buf;
    }

    void HTTPRequest::initoutmime (void) {
	 outmime.clear();
	 outmime["Server"] = "bulkrays/" BULKRAYSVERSION;
	 outmime["Connection"] = "Keep-Alive";
    }

    HTTPRequest::~HTTPRequest () {
    }

    int status_message_globalinit (void) {
	status_message [100] =   "Continue";
	status_message [101] =   "Switching Protocols";
	status_message [200] =   "OK";
	status_message [201] =   "Created";
	status_message [202] =   "Accepted";
	status_message [203] =   "Non-Authoritative Information";
	status_message [204] =   "No Content";
	status_message [205] =   "Reset Content";
	status_message [206] =   "Partial Content";
	status_message [300] =   "Multiple Choices";
	status_message [301] =   "Moved Permanently";
	status_message [302] =   "Found";
	status_message [303] =   "See Other";
	status_message [304] =   "Not Modified";
	status_message [305] =   "Use Proxy";
	status_message [307] =   "Temporary Redirect";
	status_message [400] =   "Bad Request";
	status_message [401] =   "Unauthorized";
	status_message [402] =   "Payment Required";
	status_message [403] =   "Forbidden";
	status_message [404] =   "Not Found";
	status_message [405] =   "Method Not Allowed";
	status_message [406] =   "Not Acceptable";
	status_message [407] =   "Proxy Authentication Required";
	status_message [408] =   "Request Time-out";
	status_message [409] =   "Conflict";
	status_message [410] =   "Gone";
	status_message [411] =   "Length Required";
	status_message [412] =   "Precondition Failed";
	status_message [413] =   "Request Entity Too Large";
	status_message [414] =   "Request-URI Too Large";
	status_message [415] =   "Unsupported Media Type";
	status_message [416] =   "Requested range not satisfiable";
	status_message [417] =   "Expectation Failed";
	status_message [418] =   "I'm a teapot";
	status_message [500] =   "Internal Server Error";
	status_message [501] =   "Not Implemented";
	status_message [502] =   "Bad Gateway";
	status_message [503] =   "Service Unavailable";
	status_message [504] =   "Gateway Time-out";
	status_message [505] =   "HTTP Version not supported";
	return 0;
    }

    int ReturnError::output (ostream &cout, HTTPRequest &req) {
	stringstream body;

	if (req.errormsg == NULL) {

	    map <int,const char *>::iterator mi = status_message.find (req.statuscode);
	    if (mi == status_message.end()) {
		cerr << "ReturnError::output : wrong internal error code : " << req.statuscode << endl;
		req.statuscode = 500;
		mi = status_message.find (req.statuscode);
	    }
	    req.errormsg = mi->second;
	}
	
	body << xhtml_header;
	body << "<html>" << endl
	     << "<head>" << endl
	     << " <title>" << req.statuscode << " " << req.errormsg << "</title>" << endl
	     << " <style>" << endl
	     << "  .bomb64 { background: url(data:image/gif;base64," << bomb64_gif << ")" << endl
	     << "            top left no-repeat;" << endl
	     << "            height: 72px; width: 64px; margin 0.1em; float: left;" << endl
	     << "          }" << endl
	     << " </style>" << endl
	     << "</head>" << endl
	     << "<body>" << endl
	     << " <h1>" << req.statuscode << " " << req.errormsg << "</h1>" << endl;
	if (req.suberrormsg != NULL)
	     body << "<p>" << req.suberrormsg << "</p>" << endl;
// << "<img src=\"data:image/gif;base64," << bomb64_gif << "\">" << endl
	int i;
	body << "<div>";
	for (i=0 ; i<req.statuscode/100 ; i++)
	    body <<" <span class=\"bomb64\"> </span>";
	body << "</div>" << endl;

	body << " <hr style=\"clear: both;\">" << endl
	     << "<p>bulkrays http server</p>" << endl
	     << "</body>";

	req.outmime ["Content-Type"] = "text/html";
	req.outmime ["Accept-Ranges"] = "bytes";
//	req.outmime ["Retry-After"] = "5";
	req.set_contentlength (body.str().size());
	if (!req.expires_set)
	    req.set_relative_expires (10);
	if (closeconnection)
	    req.outmime["Connection"] = "close";

	req.publish_header();
	cout << body.str();

	return 0;
    }

    int ReturnError::shortcuterror (ostream &cout, HTTPRequest &req, int statuscode, const char* message /*=NULL*/, const char* submessage /*=NULL*/, bool closeconnection) {
	ReturnError::closeconnection = closeconnection;
	req.errormsg = message, req.suberrormsg = submessage, req.statuscode = statuscode;
	int e = output (cout, req);
	req.logger ("yop");
	return e;
    }

    int TreatRequest::error (ostream &cout, HTTPRequest &req, int statuscode, const char* message /*=NULL*/, const char* submessage /*=NULL*/, bool closeconnection) {
	req.errormsg = message, req.suberrormsg = submessage, req.statuscode = statuscode;
	return returnerror.output (cout, req);
    }

    HttppConn::HttppConn (int fd, struct sockaddr_in const &client_addr) : SocketConnection(fd, client_addr),request(*this) {
	id = idnum;
	idnum ++;
	state = HTTPRequestLine;
    }
    HttppConn::~HttppConn (void) {
	// nothing to do there yet
    }

// [2]GET /blouga HTTP/1.1
// [2]Host: bulkrays.zz
// [2]User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:15.0) Gecko/20100101 Firefox/15.0.1
// [2]Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
// [2]Accept-Language: en-us,en;q=0.5
// [2]X-Forwarded-For: 192.168.42.51
// [2]X-Varnish: 834650459
// [2]Accept-Encoding: gzip
// [2]

// [0]GET /tests/nav_logo114.png HTTP/1.1
// [0]Host: apache.zz
// [0]Connection: close
// [0]

    void HttppConn::compute_reqbodylen (void) {
// cout << "HttppConn::compute_reqbodylen" << endl;
	MimeHeader::iterator mi = request.mime.find("Content-Length");
	if (mi == request.mime.end()) {
	    cout << "[" << id << "] MessageBody : no Content-Length header " << bufin << endl;
	    request.reqbodylen = 0;
	    request.readbodybytes = 0;
	    return;
	}
	request.reqbodylen = atoi (mi->second.c_str());
	request.readbodybytes = 0;
    }

    void HttppConn::lineread (void) {
	size_t p, q, len;
	MimeHeader::iterator mi;

	switch (state) {
	    case HTTPRequestLine:
		p = bufin.find (' ');
		if (p == string::npos) {
		    request.method = bufin; // JDJDJDJD unused ????
		    cerr << "wrong request line (method only ?): " << bufin << endl;
		    request.set_relative_expires (60);
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong request line (method only ?)");
		    flushandclose();
		    break;
		}
		request.method = bufin.substr (0, p);
cout << "[" << id << "] method = " << request.method << endl;
		q = p+1, p = bufin.find (' ', q);
		if (p == string::npos) {
		    request.req_uri = bufin.substr(q);
		    populate_reqfields_from_uri (request.req_uri, request.document_uri, request.uri_fields);
cout << "[" << id << "] req_uri = " << request.req_uri << endl;
cout << "[" << id << "]   "<< endl
     << ostreamMap(request.uri_fields, "       uri_fields") << endl;
		    cerr << "wrong request line (missing version ?): " << bufin << endl;
		    request.set_relative_expires (60);
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong request line (missing http version ?)");
		    flushandclose();
		    break;
		}
		request.req_uri = bufin.substr (q, p-q);
		populate_reqfields_from_uri (request.req_uri, request.document_uri, request.uri_fields);
cout << "[" << id << "] req_uri = " << request.req_uri << endl;
cout << "[" << id << "]   "<< endl
     << ostreamMap(request.uri_fields, "       uri_fields") << endl;
		q = p+1, p = bufin.find (' ', q);
		request.version = bufin.substr(q);
cout << "[" << id << "] version = " << request.version << endl;
		state = MIMEHeader;
		break;
    
	    case MIMEHeader:
		if (bufin.empty()) {
		    compute_reqbodylen ();
		    if (request.reqbodylen > 0) {
			state = ReadBody;
			setrawmode();
		    } else {
			state = NowTreatRequest;
		    }
cout << "[" << id << "]   "<< endl
     << ostreamMap(request.mime, "      mime") << endl;
		    break;
		}
		if (!isalnum(bufin[0])) {
		    cerr << "wrong mime header-name (bad starting char ?) : " << bufin << endl;
		    request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong mime header-name");
		    flushandclose();
		    break;
		}
	    case NextMIMEHeader:
		len = bufin.size();
		if (len == 0) {
		    if (!mimeheadername.empty()) {
			request.mime[mimeheadername] = mimevalue;   // JDJDJDJD should check for pre-existing mime entry
// cout << mimeheadername << " = " << mimevalue << endl;
		    }
		    compute_reqbodylen ();
		    if (request.reqbodylen > 0) {
			state = ReadBody;
			setrawmode();
		    } else {
			state = NowTreatRequest;
		    }
cout << "[" << id << "]   "<< endl
     << ostreamMap(request.mime, "      mime") << endl;
		    break;
		}
		p = 0;
		if ((bufin[p] == ' ') || (bufin[p] == 9)) {	// are we on a continuation of mime value ?
		    while ((p < len) && ((bufin[p] == ' ') || (bufin[p] == 9)))
			p++;
		    while (p < len)
			mimevalue += bufin[p++];
		
		    request.mime[mimeheadername] = mimevalue;	// JDJDJDJD should check for pre-existing mime entry
// cout << mimeheadername << " = " << mimevalue << endl;
		    mimevalue.clear();
		    mimeheadername.clear();
		    state = NextMIMEHeader;
		    break;
		}
		if (!mimeheadername.empty()) {
		    request.mime[mimeheadername] = mimevalue;   // JDJDJDJD should check for pre-existing mime entry
// cout << mimeheadername << " = " << mimevalue << endl;
		}
		p = bufin.find (':');
		if (p == string::npos) {
		    cerr << "wrong mime header-name (missing ':' ?) : " << bufin << endl;
		    request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong mime header-name (missing ':' ?)");
		    flushandclose();
		    break;
		}
		if (!isalnum(bufin[0])) {
		    cerr << "wrong mime header-name (bad starting char ?) : " << bufin << endl;
		    request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong mime header-name (bad starting char ? [2])");
		    flushandclose();
		    break;
		}
		mimeheadername = bufin.substr (0, p);
		mimevalue.clear();
		p++;
		while ((p < len) && ((bufin[p] == ' ') || (bufin[p] == 9)))
		    p++;
		while (p < len) {
		    mimevalue += bufin[p++];
		}
		state = NextMIMEHeader;
		break;

	    case ReadBody:
		request.req_body += bufin;
		request.readbodybytes += bufin.size();
		if (request.readbodybytes >= request.reqbodylen) {
		    cout << "[" << id << "] ReadBody : " << request.readbodybytes << " read, " << request.reqbodylen << " schedulled.  diff = " << request.readbodybytes-request.reqbodylen << endl;
		    mi = request.mime.find ("Content-Type");
		    if (mi == request.mime.end()) {
			cerr << "missing Content-Type with non-empty request body" << endl;	// JDJDJDJD this could have been detected earlier, and the connection shut earlier
			request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
			returnerror.shortcuterror ((*out), request, 400, NULL, "missing Content-Type for request body");
			flushandclose();
			break;
		    }
		    if (mi->second.substr(0,20) == "multipart/form-data;") {
// cout << "we're about to deal with a multipart/form-data" << endl;

			bool weseekthefirstboundary = true;
			string name;
			string lcr; // the locally supllied in-use crlf string .....
			size_t lcrs = 0; // the size of lcr
			size_t lastboundary, currboundary = 0;
			bool wehaveasmallfield = false;
			bool wehaveencontententry = false;
			FieldsMap mpentrymime;
			string contenttype;

			size_t p = mi->second.find ("boundary=");
			if (p == string::npos) {
			    cerr << "no boundary given in a multipart/form-data : Content-Type: " << mi->second << endl;
			    request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
			    returnerror.shortcuterror ((*out), request, 400, NULL, "missing boundary definition in multipart/form-data");
			    flushandclose();
			    break;
			}
			string boundary ("--");
			boundary += mi->second.substr(p+9);
// cout << "boundary = " << boundary << endl;

			p = 0;
			size_t	q = p,
				l = request.req_body.length();

			while (p < l) {

			    p = request.req_body.find(boundary, q);
			    if (p == string::npos) {
cout << "could not find next boundary" << endl;
				break;
			    }

			    currboundary = p;
			    if (wehaveasmallfield) {
				if (wehaveencontententry) {
				    BodySubEntry bse(contenttype, request.req_body, lastboundary, currboundary - lastboundary - lcrs, mpentrymime);
				    request.content_fields.insert(pair<string, BodySubEntry>(name,bse));
				    // request.content_fields[name] = bse;
				    wehaveencontententry = false;
				} else {
				    request.body_fields[name] = request.req_body.substr (lastboundary, currboundary - lastboundary - lcrs);
				    wehaveasmallfield = false;
				}
			    }

			    q = p+boundary.length();
			    if (q+1 > l) {
cout << "last boundary too close to the end of body ???" << endl;
				break;
			    }
			    if ((request.req_body[q] == '-') && (request.req_body[q+1] == '-'))	{
				q += 2;
if (l-q != 2)
cout << "reached closing boundary at " << (l - q) << " bytes from end of body" << endl;
				break;
			    }
			    p = q;

			    if (weseekthefirstboundary) {
				weseekthefirstboundary = false;
				lcr = fetch_localcr (request.req_body, p);
				lcrs = lcr.size();
				if (lcrs == 0) {
				    cerr << "at boundarizing the post req_body : could not determine the crlf in use" << endl;
				    break;
				}
				
// cout << "crlf in use :";
// {   size_t i;
//     for(i=0;i<lcr.size();i++)
// 	cout << "[" << (int)lcr[i] << "]";
// }
// cout << endl;
			    }

			    q = request.req_body.find (lcr, p);
			    if (q == string::npos) {
				cerr << "at boundarizing the post req_body : could not find begining of multipart-entry mime" << endl;
				break;
			    }
			    q += lcrs;
			    p = q;
			    {	mpentrymime.clear();
				contenttype.clear();

				read_mimes_in_string (request.req_body, mpentrymime, lcr, q);
				lastboundary = q;
// cout << "[" << id << "]   "<< endl
//      << ostreamMap(mpentrymime, "      pentry[] mime") << endl;
				FieldsMap::iterator mi = mpentrymime.find ("Content-Disposition");
				if (mi == mpentrymime.end()) {
				    cerr << "missing Content-Disposition mime-entry (every boundary should have a Content-Disposition entry)" << endl;
				    continue;
				}
				string& cd = mi->second;
				size_t p = cd.find(';');
				if (cd.substr(0, p) != "form-data") {
				    cerr << "wrong Content-Disposition entry, not \"form-data\" ??" << endl;
				    continue;
				}
				p = cd.find("name=\"", p);
				if (p == string::npos) {
				    cerr << "wrong Content-Disposition entry, missing \"name=\" ??" << endl;
				    continue;
				}
				p += 6;
				size_t q = cd.find ('"', p);
				if (q == string::npos) {
				    cerr << "wrong Content-Disposition entry, name=\" is unclosed ??" << endl;
				    continue;
				}
				name = cd.substr(p, q-p);
				wehaveasmallfield = true;

				mi = mpentrymime.find ("Content-Type");
				if (mi == mpentrymime.end())
				    continue;
				
				wehaveencontententry = true;
				contenttype = mi->second;
			    }


			    
// -----------------------------8595497025756728601942872864
// Content-Disposition: form-data; name="fichierno1"; filename="subneko_16.png"
// Content-Type: image/png


			}


		    } else if (mi->second == "application/x-www-form-urlencoded") {
cout << "we've a form post !" << endl;
			populate_reqfields_from_urlencodebody (request.req_body, request.body_fields);
		    } else {
			cerr << "unhandled request-body' Content-Type : " << mi->second << endl;	// JDJDJDJD this could have been detected earlier, and the connection shut earlier
			request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
			returnerror.shortcuterror ((*out), request, 415, NULL, "the provided Content-type for request body cannot be handled by server");
			flushandclose();
			break;
		    }
		    state = NowTreatRequest;
		    setlinemode();
		    break;
		}
		break;

	    case NowTreatRequest:
		break;
	}
// cout << "state=" << state << " : request.reqbodylen=" << request.reqbodylen << " request.readbodybytes=" << request.readbodybytes << endl;
	if (state == NowTreatRequest) {

cout << "============================" << endl;
cout << "[" << id << "]   "<< endl
     << ostreamMap(request.body_fields, "      body_fields") << endl;

	    request.req_fields.import(request.uri_fields);
	    request.req_fields.import(request.body_fields);
cout << "[" << id << "]   "<< endl
     << request.req_fields << endl
     << request.content_fields << endl;

	    MimeHeader::iterator mi_host = request.mime.find ("Host");
	    if (mi_host == request.mime.end()) {
		cerr << "missing Host mime entry" << endl;
		request.set_relative_expires (60);
		returnerror.shortcuterror ((*out), request, 503, NULL, "missing Host mime entry");  // JDJDJDJD we should have a default host
								    // JDJDJDJD we should be able to tune the error message (Unknown virtual host.)
		flushandclose();
		return;
	    } 
	    request.host = mi_host->second;
	    THostMapper::iterator mi = hostmapper.find (request.host);
	    if (mi == hostmapper.end()) {
		cerr << "unhandled Host :" << request.host << endl;
		request.set_relative_expires (60);
		returnerror.shortcuterror ((*out), request, 404, "Unkown Virtual Host");  // JDJDJDJD we should have a default host
								    // JDJDJDJD we should be able to tune the error message (Unknown virtual host.)
		flushandclose();
		return;
	    }
	    TreatRequest* treatrequest = mi->second->treatrequest (request);
	    if (treatrequest == NULL) {
		cerr << "NULL treatrequest from hostmapper for Host " << request.host << endl;
		request.set_relative_expires (60);
		returnerror.shortcuterror ((*out), request, 404, NULL, "No Handler for URI");  // JDJDJDJD we should be able to tune the err
		flushandclose();
		return;
	    }
	    treatrequest->output ((*out), request);
	    state = HTTPRequestLine;
	    request.logger();
	    request.clear ();
	    flush();
	}
    }

    ostream * HTTPRequest::clog = &cerr;
}

using namespace std;
using namespace qiconn;
using namespace bulkrays;

int main (int nb, char ** cmde) {
    string user ("www-data");
    string group;
    string address ("0.0.0.0");
    int port = 80;
    string flogname("/var/log/bulkrays/access_log");	// JDJDJDJD we should introduce a DEFINEd scheme for such defaults

    int i;
    for (i=1 ; i<nb ; i++) {
	if (strncmp (cmde[i], "--help", 6) == 0) {
	    cout << cmde[0] << "   \\" << endl
			    << "      [--bind=[address][:port]]  \\" << endl
			    << "      [--user=[user][:group]]  \\" << endl
			    << "      [--access_log=filename]" << endl;
	    return 0;
	} else if (strncmp (cmde[i], "--bind=", 7) == 0) {
	    string scheme(cmde[i]+7);
	    size_t p = scheme.find(':');
	    if (p == string::npos) {	// we have only an addr ?
		address = scheme;
	    } else {
		port = atoi (scheme.substr(p+1).c_str());
		if (p>0)
		    address = scheme.substr(0,p);
	    }
	    cerr << "will try to bind : " << address << ":" << port << endl;
	} else if (strncmp (cmde[i], "--access_log=", 13) == 0) {
	    if (*(cmde[i]+13) != 0) {
		flogname = cmde[i]+13;
	    }
	} else if (strncmp (cmde[i], "--user=", 7) == 0) {
	    string scheme(cmde[i]+7);
	    size_t p = scheme.find(':');
	    if (p == string::npos) {	// we have only a user
		user = scheme;
	    } else {
		if (!scheme.substr(p+1).empty())
		    group = scheme.substr(p+1);
		if (p>0)
		    user = scheme.substr(0,p);
	    }
	} else {
	    cerr << "unknown option : " << cmde[i] << endl;
	}
    }


    int s = server_pool (port, address.c_str());
    if (s < 0) {
	cerr << "could not instanciate connection pool, bailing out !" << endl;
	return -1;
    }

    {	struct passwd *pwent = getpwnam (user.c_str());
	if (pwent == NULL) {
	    cerr << "could not retrieve uid for user=" << user << endl;
	    return -1;
	}
	uid_t uid = pwent->pw_uid;
	gid_t gid = pwent->pw_gid;

	if (!group.empty()) {
	    struct group * grent = getgrnam (group.c_str());
	    if (grent == NULL) {
		cerr << "could not retrieve gid for group=" << group << endl;
		return -1;
	    }
	    gid = grent->gr_gid;
	}

	if (setgid (gid) != 0) {
	    int e = errno;
	    cerr << "could not change gid privileges ? " << strerror(e) << endl;
	    return -1;
	}
	if (setuid (uid) != 0) {
	    int e = errno;
	    cerr << "could not change uid privileges ? " << strerror(e) << endl;
	    return -1;
	}
    }


    HttppBinder *ls = new HttppBinder (s, port, address.c_str());
    if (ls == NULL) {
	cerr << "could not instanciate HttppBinder, bailing out !" << endl;
	return -1;
    }

    ofstream cflog (flogname.c_str(), ios::app);
    if (!cflog) {
	int e = errno;
	cerr << "could not open " << flogname << " : " << strerror(e) << endl;
	return -1;
    }
    HTTPRequest::clog = &cflog;



    connectionpool.init_signal ();
    
    connectionpool.push (ls);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    status_message_globalinit ();
    bootstrap_global ();

    connectionpool.select_loop (timeout);

    cerr << "terminating" << endl;

    close (s);
    return 0;
}


