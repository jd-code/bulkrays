#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include <sys/types.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>

#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>

#define QICONN_H_GLOBINST
#define BULKRAYS_H_GLOBINST
#include "bulkrays/bulkrays.h"

namespace bulkrays {
    using namespace std;
    using namespace qiconn;

    Property::Property (const string &s) : s(s) {
	i = atoi (s.c_str());
	if ((s=="true") || (s=="Y") || (s=="y") || (s=="yes") || (s=="YES") || (s=="TRUE"))
	    b = true;
	else
	    b = false;
    }

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
		if ((d<0) || (u<0)) {
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


    string xmlencode (const string &s) {
	string r;

	size_t i, l=s.size();
	for (i=0 ; i<l ; i++) {
	    switch (s[i]) {
		case '"':   r += "&quot;";  break;
		case '&':   r += "&amp;";   break;
		case '\'':  r += "&apos;";  break;
		case '<':   r += "&lt;";    break;
		case '>':   r += "&gt;";    break;
		default:    r += s[i];	    break;
	    }
	}
	return r;
    }

    void xmlencode (ostream & cout, const string &s) {
	size_t i, l=s.size();
	for (i=0 ; i<l ; i++) {
	    switch (s[i]) {
		case '"':   cout << "&quot;";	break;
		case '&':   cout << "&amp;";   	break;
		case '\'':  cout << "&apos;";  	break;
		case '<':   cout << "&lt;";    	break;
		case '>':   cout << "&gt;";    	break;
		default:    cout << s[i];	break;
	    }
	}
    }

    void urlencode (ostream & cout, const string &s) {
static const char* hex="0123456789ABCDEF";
	size_t i, l=s.size();
	for (i=0 ; i<l ; i++) {
	    switch (s[i]) {
		default:
		    if (isascii(s[i]) && (!iscntrl(s[i]))) {
			cout << s[i];
			break;
		    }
		case '=':
		case '+':
		case '&':
		case ' ':
		    cout << '%'
			 << hex[ (((unsigned char)s[i]) & 0xf0) >> 4 ]
			 << hex[ (((unsigned char)s[i]) & 0x0f)      ];
		    break;
	    }
	}
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
	    if (mi != reqfields.end()) {	// JDJDJDJD if it was a member function we could populate the error message
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

    bool FieldsMap::match (const string &k, const string &v) const {
	FieldsMap::const_iterator mi = find (k);
	if (mi == end())
	    return false;
	if (mi->second == v)
	    return true;
	return false;
    }

    bool FieldsMap::notempty (const string &k) const {
	FieldsMap::const_iterator mi = find (k);
	if (mi == end())
	    return false;
	if (mi->second.empty())
	    return false;
	else
	    return true;
    }

    bool FieldsMap::notempty (const string &k, FieldsMap::iterator &mi) {
	mi = find (k);
	if (mi == end())
	    return false;
	if (mi->second.empty())
	    return false;
	else
	    return true;
    }

    bool FieldsMap::verif (const string &k, FieldsMap::iterator& mi) {
	mi = find (k);
	if (mi == end())
	    return false;
	return true;
    }

    void HTTPRequest::logger () {
	time_t t;
	time (&t);
	struct tm tm;
	gmtime_r(&t, &tm);

//	clog << httppconn->getname() << " " << method << " " << host << req_uri << " " << statuscode << " " << msg << endl;
	(*clog) 
	    << millidiff (httppconn->entering_ReadBody,		httppconn->entering_HTTPRequestLine) << " "
	    << millidiff (httppconn->entering_NowTreatRequest,	httppconn->entering_ReadBody) << " "
	    << millidiff (httppconn->entering_WaitingEOW,	httppconn->entering_NowTreatRequest) << " "
	    << millidiff (httppconn->ending_WaitingEOW,		httppconn->entering_WaitingEOW) << " "
	    
	    << httppconn->getname()
	    << " - - "	    // JDJDJDJD here stuff about authentication
	    << "[" << setfill('0')
	       << setw(2) << tm.tm_mday << '/'
	       << setw(2) << tm.tm_mon+1 << '/'
	       << setw(4) << tm.tm_year + 1900 << ':'
	       << setw(2) << tm.tm_hour << ':'
	       << setw(2) << tm.tm_min << ':'
	       << setw(2) << tm.tm_sec << "] \""
	    << method << " " << host << req_uri << " " << version << "\" "
	    << statuscode << " "
	    << (int)(httppconn->gettotw() - httppconn->lastbwindex)
	    << " \"";

	MimeHeader::iterator mi = mime.find("Referer");

	if (mi == mime.end())
	    (*clog) << " - ";
	else
	    (*clog) << mi->second;

	(*clog) << "\" ";

	mi = mime.find("User-Agent");
	if (mi == mime.end())
	    (*clog) << "\" - \"";
	else
	    (*clog) << mi->second;
	(*clog) << endl;
    }

//	    void HTTPRequest::logger (const char *msg /*=NULL*/) {
//		if (msg != NULL)
//		    logger (string(msg));
//		else
//		    logger (string(""));
//	//	if (msg == NULL)
//	//	    cout << httppconn->getname() << " " << method << " " << host << req_uri << " " << statuscode << endl;
//	//	else
//	//	    cout << httppconn->getname() << " " << method << " " << host << req_uri << " " << statuscode << " " << msg << endl;
//	    }

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
	    errlog() << "HTTPRequest::publish_header : header already published !!!" << endl;
	    return;
	}
	char buf[40];
	ostream &cout = *(httppconn->out) ;

	const char * outputerror;
	if (errormsg == NULL) {
	    map <int,const char *>::iterator mi = status_message.find(statuscode);
	    if (mi == status_message.end()) {
		errlog() << "HTTPRequest::publish_header : statuscode [" << statuscode << "] has no associated message" << endl;
		outputerror = "Sorry, No Message";
	    } else {
		outputerror = mi->second;
	    }
	} else {
	    outputerror = errormsg;
	}
	cout << "HTTP/1.1 " << statuscode << ' ' << outputerror << bendl
	     << "Date: " << rfc1123date_offset(buf, 0) << bendl;

	MimeHeader::iterator mi;
	for (mi=outmime.begin() ; mi!=outmime.end() ; mi++)
	    cout << mi->first << ": " << mi->second << bendl;

	cout << bendl;
	headerpublished = true;
    }

    void HTTPRequest::set_contentlength (size_t l) {
	char buf[40];
	snprintf (buf, 39, "%ld", (long) l);
	outmime["Content-Length"] = buf;
    }

    bool HTTPRequest::cookcookies (void) {
	if (cookiescooked)
	    return cookiescookedresult;

	MimeHeader::iterator mi = mime.find("Cookie");
	if (mi == mime.end()) {
	    cookiescooked = true;
	    cookiescookedresult = false;
	    return cookiescookedresult;
	}

	string &c = mi->second;
	size_t p = 0, s= c.size();

	while (p < s) {
	    while ((p<s) && isspace(c[p])) p++;
	    size_t q = c.find('=',p);
	    if (q == string::npos)
		break;
	    string ident = c.substr (p,q-p);
	    p = q+1;

	    q = c.find ("; ", p);
	    if (q == string::npos) {
		cookies[ident] = c.substr (p);
		break;
	    }
	    cookies[ident] = c.substr (p, q-p);
	    p = q+2;
	}
	if (cookies.empty()) {
	    cookiescooked = true;
	    cookiescookedresult = false;
	    return cookiescookedresult;
	}
	cookiescooked = true;
	cookiescookedresult = true;
	return cookiescookedresult;
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

    TReqResult ReturnError::output (ostream &cout, HTTPRequest &req) {
	stringstream body;

	if (req.errormsg == NULL) {

	    map <int,const char *>::iterator mi = status_message.find (req.statuscode);
	    if (mi == status_message.end()) {
		req.errlog() << "ReturnError::output : wrong internal error code : " << req.statuscode << endl;
		req.statuscode = 500;
		mi = status_message.find (req.statuscode);
	    }
	    req.errormsg = mi->second;
	}
	
	body << xhtml_header;
	body << "<html>" << bendl
	     << "<head>" << bendl
	     << " <title>" << req.statuscode << " " << req.errormsg << "</title>" << bendl
	     << " <style>" << bendl
	     << "  .bomb64 { background: url(data:image/gif;base64," << bomb64_gif << ")" << bendl
	     << "            top left no-repeat;" << bendl
	     << "            height: 72px; width: 64px; margin: 0.1em; float: left;" << bendl
	     << "          }" << bendl
	     << " </style>" << bendl
	     << "</head>" << bendl
	     << "<body>" << bendl
	     << " <h1>" << req.statuscode << " " << req.errormsg << "</h1>" << bendl;
	if (req.suberrormsg != NULL)
	     body << "<p>" << req.suberrormsg << "</p>" << bendl;
// << "<img src=\"data:image/gif;base64," << bomb64_gif << "\">" << bendl
	int i;
	body << "<div>";
	for (i=0 ; i<req.statuscode/100 ; i++)
	    body <<" <span class=\"bomb64\"> </span>";
	body << "</div>" << bendl;

	body << " <hr style=\"clear: both;\">" << bendl
	     << "<p>bulkrays http server</p>" << bendl
	     << "</body>";

	req.outmime ["Content-Type"] = "text/html;charset=ASCII";
	req.outmime ["Accept-Ranges"] = "bytes";
//	req.outmime ["Retry-After"] = "5";
	req.set_contentlength (body.str().size());
	if (!req.expires_set)
	    req.set_relative_expires (10);
	if (closeconnection)
	    req.outmime["Connection"] = "close";

	req.publish_header();
	cout << body.str();

	return TRCompleted;
    }

    int ReturnError::shortcuterror (ostream &cout, HTTPRequest &req, int statuscode, const char* message /*=NULL*/, const char* submessage /*=NULL*/, bool closeconnection) {
	ReturnError::closeconnection = closeconnection;
	req.errormsg = message, req.suberrormsg = submessage, req.statuscode = statuscode;
	int e = output (cout, req);
	return e;
    }

    TReqResult TreatRequest::error (ostream &cout, HTTPRequest &req, int statuscode, const char* message /*=NULL*/, const char* submessage /*=NULL*/, bool closeconnection) {
	req.errormsg = message, req.suberrormsg = submessage, req.statuscode = statuscode;
	return returnerror.output (cout, req);
    }

    HttppConn::HttppConn (int fd, struct sockaddr_in const &client_addr) : SocketConnection(fd, client_addr),request(*this) {
	corking = true;
	cork ();
	id = idnum;
	idnum ++;
	state = HTTPRequestLine;
	lastbwindex = gettotw();
if (lastbwindex != 0)
errlog() << "lastbwindex = " << lastbwindex << " shouldn't it be 0 ???" << endl;
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

    ostream& HttppConn::shorterrlog (void) {
	time_t t;
	time (&t);
	struct tm tm;
	gmtime_r(&t, &tm);

	return cerr 
	    << "[" << setfill('0')
	       << setw(2) << tm.tm_mday << '/'
	       << setw(2) << tm.tm_mon+1 << '/'
	       << setw(4) << tm.tm_year + 1900 << ':'
	       << setw(2) << tm.tm_hour << ':'
	       << setw(2) << tm.tm_min << ':'
	       << setw(2) << tm.tm_sec << "] \""
	    << "[" << fd << " : " << id << " : " << name << "] ";
    }

    ostream& HttppConn::errlog (void) {
	return shorterrlog()
	    << request.method << " ("
	    << request.host << ") "
	    << request.document_uri
	    << " ";
    }

    ostream& HTTPRequest::errlog (void) {
	return httppconn->errlog();
    }

    const char * charitnull = "NULL";
    const char * boolittrue = "TRUE";
    const char * boolitfalse = "FALSE";

    inline const char * charit (const char *p) { return (p == NULL) ? charitnull : p; }
    inline const char * boolit (bool b) { return b ? boolittrue : boolitfalse; }

#define BEGIN_TERM_IDENT  "\033[33m"
#define BEGIN_TERM_VALUE  "\033[36m"
#define END_TERM_IDENT  "\033[m"

    ostream& HTTPRequest::dump (ostream& out) const {
	const char  *I = BEGIN_TERM_IDENT,
		    *V = BEGIN_TERM_VALUE,
		    *E = END_TERM_IDENT;
      return out
//	<< I << "httppconn -> "   << E << "= " << E << V << (*httppconn) << E << endl
	<< I << "statuscode "            << E << "= " << E << V << statuscode << E << endl
	<< I << "errormsg "              << E << "= " << E << V << charit(errormsg) << E << endl
	<< I << "suberrormsg "           << E << "= " << E << V << charit(errormsg) << E << endl
	<< I << "expires_set "           << E << "= " << E << V << boolit(expires_set) << E << endl
	<< I << "headerpublished "       << E << "= " << E << V << boolit(headerpublished) << E << endl
	<< I << "method "                << E << "= " << E << V << method << E << endl
	<< I << "host "                  << E << "= " << E << V << host << E << endl
        << I << "req_uri "               << E << "= " << E << V << req_uri << E << endl
        << I << "document_uri "          << E << "= " << E << V << document_uri << E << endl
        << I << "version "               << E << "= " << E << V << version << E << endl
        << I << "reqbodylen "            << E << "= " << E << V << reqbodylen << E << endl
        << I << "readbodybytes "         << E << "= " << E << V << readbodybytes << E << endl
             << "req_body NOTDUMPED" << endl
	     << endl
	     << ostreamMap(outmime,        BEGIN_TERM_IDENT "outmime"         END_TERM_IDENT) << endl
	     << ostreamMap(mime,           BEGIN_TERM_IDENT "mime"            END_TERM_IDENT) << endl
//	     << ostreamMap(req_fields,     BEGIN_TERM_IDENT "req_fields"     END_TERM_IDENT) << endl
	     << ostreamMap(uri_fields,     BEGIN_TERM_IDENT "uri_fields"     END_TERM_IDENT) << endl
	     << ostreamMap(body_fields,    BEGIN_TERM_IDENT "body_fields"    END_TERM_IDENT) << endl
//	     << ostreamMap(content_fields, BEGIN_TERM_IDENT "content_fields" END_TERM_IDENT) << endl
	;
    }

    bool set_default_host (string const &hostname) {
	THostMapper::iterator mi = hostmapper.find (hostname);
	mi_defaulthost = mi;
	if (mi == hostmapper.end()) {
	    wehaveadefaulthost = false;
	    return false;
	} else {
	    wehaveadefaulthost = true;
	    return true;
	}
    }

    void unset_default_host (void) {
	wehaveadefaulthost = false;
    }

    void HttppConn::compute_reqbodylen (void) {
// errlog() << "HttppConn::compute_reqbodylen" << endl;
	MimeHeader::iterator mi = request.mime.find("Content-Length");
	if (mi == request.mime.end()) {
	    if (request.method == "PUT") {
		errlog() << "MessageBody : no Content-Length header " << hexdump(bufin) << endl;
	    }
	    request.reqbodylen = 0;
	    request.readbodybytes = 0;
	    return;
	}
	request.reqbodylen = atoi (mi->second.c_str());
	request.readbodybytes = 0;
    }

    void HTTPResponse::compute_bodylen (void) {
// errlog() << "HttppConn::compute_reqbodylen" << endl;
	MimeHeader::iterator mi = mime.find("Content-Length");
	if (mi == mime.end()) {
	    reqbodylen = -1;	// JDJDJDJD tricky, need to be checked
	    readbodybytes = 0;
	    return;
	}
	reqbodylen = atoi (mi->second.c_str());
	readbodybytes = 0;
    }

    void HttppConn::lineread (void) {
	size_t p, q, len;
	MimeHeader::iterator mi;

	switch (state) {

	    case BuggyConn:
		errlog() << "received some stuff in BuggyConn: " << bufin.size() << " bytes" << endl;
		break;

	    case TreatPending:
	    case WaitingEOW:	// --------------------------------------------------------------------
		errlog() << "received some stuff before end of transmission occured ?!" << endl;
errlog() << "some garbage while waiting eow : {" << hexdump(bufin) << "}" << endl;
		break;

	    case HTTPRequestLine:   // ----------------------------------------------------------------
		cork();
		gettimeofday(&entering_HTTPRequestLine, NULL);
		p = bufin.find (' ');
		if (p == string::npos) {
		    request.method = bufin; // JDJDJDJD unused ????
		    errlog() << "wrong request line (method only ?): " << hexdump(bufin) << endl;
		    request.set_relative_expires (60);
		    gettimeofday(&entering_ReadBody, NULL);
		    gettimeofday(&entering_NowTreatRequest, NULL);
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong request line (method only ?)");
		    gettimeofday(&entering_WaitingEOW, NULL);
		    // state = WaitingEOW;	// JDJDJDJD things are already so buggy that we're not http-speak and should bail out with a special bogussilentmode
		    state = BuggyConn;
		    cp->reqnor (fd);
		    flushandclose();
		    break;
		}
		if (p == 0) {	// empty method ......
		    errlog() << "wrong request line (empty method ?): " << hexdump(bufin) << endl;
		    request.set_relative_expires (60);
		    gettimeofday(&entering_ReadBody, NULL);
		    gettimeofday(&entering_NowTreatRequest, NULL);
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong request line (method only ?)");
		    gettimeofday(&entering_WaitingEOW, NULL);
		    // state = WaitingEOW;	// JDJDJDJD things are already so buggy that we're not http-speak and should bail out with a special bogussilentmode
		    state = BuggyConn;
		    cp->reqnor (fd);
		    flushandclose();
		    break;
		}
		request.method = bufin.substr (0, p);
if (debugparsereq) shorterrlog() << "method = " << request.method << endl;
		q = p+1, p = bufin.find (' ', q);
		if (p == string::npos) {
		    request.req_uri = bufin.substr(q);
		    populate_reqfields_from_uri (request.req_uri, request.document_uri, request.uri_fields);
if (debugparsereq) {
    shorterrlog() << "req_uri = " << request.req_uri << endl;
    shorterrlog() << "  "<< endl
	 << ostreamMap(request.uri_fields, "       uri_fields") << endl;
}
		    errlog() << "wrong request line (missing version ?): " << bufin << endl;
		    request.set_relative_expires (60);
		    gettimeofday(&entering_ReadBody, NULL);
		    gettimeofday(&entering_NowTreatRequest, NULL);
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong request line (missing http version ?)");
		    gettimeofday(&entering_WaitingEOW, NULL);
		    // state = WaitingEOW;	// JDJDJDJD things are already so buggy that we're not http-speak and should bail out with a special bogussilentmode
		    // maybe this one is a little bit tough ... we'll see ....
		    state = BuggyConn;
		    cp->reqnor (fd);
		    flushandclose();
		    break;
		}
		request.req_uri = bufin.substr (q, p-q);
		populate_reqfields_from_uri (request.req_uri, request.document_uri, request.uri_fields);
if (debugparsereq) {
    shorterrlog() << "req_uri = " << request.req_uri << endl;
    errlog() << "  "<< endl
	 << ostreamMap(request.uri_fields, "       uri_fields") << endl;
}
		q = p+1, p = bufin.find (' ', q);
		request.version = bufin.substr(q);
if (debugparsereq) errlog() << "version = " << request.version << endl;

if (debugearlylog) errlog() << endl;
		state = MIMEHeader;
		mimevalue.clear();		// JDJDJDJD this is rather tricky
		mimeheadername.clear();		// JDJDJDJD this is rather tricky
		break;
    
	    case MIMEHeader:	// --------------------------------------------------------------------
		if (bufin.empty()) {
		    compute_reqbodylen ();
		    if (request.reqbodylen > 0) {
			state = ReadBody;
			gettimeofday(&entering_ReadBody, NULL);
			setrawmode();
		    } else {
			gettimeofday(&entering_ReadBody, NULL);
			gettimeofday(&entering_NowTreatRequest, NULL);
			state = NowTreatRequest;
		    }
if (debugparsereq) {
    errlog() << "  "<< endl
	 << ostreamMap(request.mime, "      mime") << endl;
}
		    break;
		}
		if (!isalnum(bufin[0])) {
		    errlog() << "wrong mime header-name (bad starting char ?) : " << hexdump(bufin) << endl;
		    request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
		    gettimeofday(&entering_ReadBody, NULL);
		    gettimeofday(&entering_NowTreatRequest, NULL);
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong mime header-name");
		    gettimeofday(&entering_WaitingEOW, NULL);
		    state = WaitingEOW;
		    flushandclose();
		    break;
		}

	    case NextMIMEHeader:    // ----------------------------------------------------------------
		len = bufin.size();
		if (len == 0) {
		    if (!mimeheadername.empty()) {
			request.mime[mimeheadername] = mimevalue;   // JDJDJDJD should check for pre-existing mime entry
// errlog() << mimeheadername << " = " << mimevalue << endl;
		    }
		    compute_reqbodylen ();
		    if (request.reqbodylen > 0) {
			gettimeofday(&entering_ReadBody, NULL);
			state = ReadBody;
			setrawmode();
		    } else {
			gettimeofday(&entering_ReadBody, NULL);
			gettimeofday(&entering_NowTreatRequest, NULL);
			state = NowTreatRequest;
		    }
if (debugparsereq) {
    errlog() << "  "<< endl
	 << ostreamMap(request.mime, "      mime") << endl;
}
		    break;
		}
		p = 0;
		if ((bufin[p] == ' ') || (bufin[p] == 9)) {	// are we on a continuation of mime value ?
		    while ((p < len) && ((bufin[p] == ' ') || (bufin[p] == 9)))
			p++;
		    while (p < len)
			mimevalue += bufin[p++];
		
		    request.mime[mimeheadername] = mimevalue;	// JDJDJDJD should check for pre-existing mime entry
// errlog() << mimeheadername << " = " << mimevalue << endl;
		    mimevalue.clear();
		    mimeheadername.clear();
		    state = NextMIMEHeader;
		    break;
		}
		if (!mimeheadername.empty()) {
		    request.mime[mimeheadername] = mimevalue;   // JDJDJDJD should check for pre-existing mime entry
// errlog() << mimeheadername << " = " << mimevalue << endl;
		}
		p = bufin.find (':');
		if (p == string::npos) {
		    errlog() << "wrong mime header-name (missing ':' ?) : " << hexdump(bufin) << endl;
		    request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
		    gettimeofday(&entering_ReadBody, NULL);
		    gettimeofday(&entering_NowTreatRequest, NULL);
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong mime header-name (missing ':' ?)");
		    gettimeofday(&entering_WaitingEOW, NULL);
		    state = WaitingEOW;
		    flushandclose();
		    break;
		}
		if (!isalnum(bufin[0])) {
		    errlog() << "wrong mime header-name (bad starting char ?) : " << hexdump(bufin) << endl;
		    request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
		    gettimeofday(&entering_ReadBody, NULL);
		    gettimeofday(&entering_NowTreatRequest, NULL);
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong mime header-name (bad starting char ? [2])");
		    gettimeofday(&entering_WaitingEOW, NULL);
		    state = WaitingEOW;
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

	    case ReadBody:  // ------------------------------------------------------------------------
		request.req_body += bufin;
		request.readbodybytes += bufin.size();
		if (request.readbodybytes >= request.reqbodylen) {
// JDJDJDJD we should not complain if the value matches !
		    errlog() << "ReadBody : " << request.readbodybytes << " read, " << request.reqbodylen << " schedulled.  diff = " << request.readbodybytes-request.reqbodylen << endl;
		    mi = request.mime.find ("Content-Type");
		    if (mi == request.mime.end()) {
			errlog() << "missing Content-Type with non-empty request body" << endl;	// JDJDJDJD this could have been detected earlier, and the connection shut earlier
			request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
			gettimeofday(&entering_NowTreatRequest, NULL);
			returnerror.shortcuterror ((*out), request, 400, NULL, "missing Content-Type for request body");
			gettimeofday(&entering_WaitingEOW, NULL);
			state = WaitingEOW;
			flushandclose();
			break;
		    }
		    if (mi->second.substr(0,20) == "multipart/form-data;") {
// errlog() << "we're about to deal with a multipart/form-data" << endl;

			bool weseekthefirstboundary = true;
			string name;
			string lcr; // the locally supllied in-use crlf string .....
			size_t lcrs = 0; // the size of lcr
			size_t lastboundary = 0, currboundary = 0;
			bool wehaveasmallfield = false;
			bool wehaveencontententry = false;
			FieldsMap mpentrymime;
			string contenttype;

			size_t p = mi->second.find ("boundary=");
			if (p == string::npos) {
			    errlog() << "no boundary given in a multipart/form-data : Content-Type: " << mi->second << endl;
			    request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
			    gettimeofday(&entering_NowTreatRequest, NULL);
			    returnerror.shortcuterror ((*out), request, 400, NULL, "missing boundary definition in multipart/form-data");
			    gettimeofday(&entering_WaitingEOW, NULL);
			    state = WaitingEOW;
			    flushandclose();
			    break;
			}
			string boundary ("--");
			boundary += mi->second.substr(p+9);
// errlog() << "boundary = " << boundary << endl;

			p = 0;
			size_t	q = p,
				l = request.req_body.length();

			while (p < l) {

			    p = request.req_body.find(boundary, q);
			    if (p == string::npos) {
				errlog() << "could not find next boundary" << endl;
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
				errlog() << "last boundary too close to the end of body ???" << endl;
				break;
			    }
			    if ((request.req_body[q] == '-') && (request.req_body[q+1] == '-'))	{
				q += 2;
if (l-q != 2)
errlog() << "reached closing boundary at " << (l - q) << " bytes from end of body" << endl;
				break;
			    }
			    p = q;

			    if (weseekthefirstboundary) {
				weseekthefirstboundary = false;
				lcr = fetch_localcr (request.req_body, p);
				lcrs = lcr.size();
				if (lcrs == 0) {
				    errlog() << "at boundarizing the post req_body : could not determine the crlf in use" << endl;
				    break;
				}
if (debugparsereq) 
{   ostream &out = errlog();				
    out << "crlf in use :";
    {   size_t i;
	for(i=0;i<lcr.size();i++)
	    out << "[" << (int)lcr[i] << "]";
    }
    out << endl;
}
			    }

			    q = request.req_body.find (lcr, p);
			    if (q == string::npos) {
				errlog() << "at boundarizing the post req_body : could not find begining of multipart-entry mime" << endl;
				break;
			    }
			    q += lcrs;
			    p = q;
			    {	mpentrymime.clear();
				contenttype.clear();

				read_mimes_in_string (request.req_body, mpentrymime, lcr, q);
				lastboundary = q;
				FieldsMap::iterator mi = mpentrymime.find ("Content-Disposition");
				if (mi == mpentrymime.end()) {
				    errlog() << "missing Content-Disposition mime-entry (every boundary should have a Content-Disposition entry)" << endl;
				    continue;
				}
				string& cd = mi->second;
				size_t p = cd.find(';');
				if (cd.substr(0, p) != "form-data") {
				    errlog() << "wrong Content-Disposition entry, not \"form-data\" ??" << endl;
				    continue;
				}
				p = cd.find("name=\"", p);
				if (p == string::npos) {
				    errlog() << "wrong Content-Disposition entry, missing \"name=\" ??" << endl;
				    continue;
				}
				p += 6;
				size_t q = cd.find ('"', p);
				if (q == string::npos) {
				    errlog() << "wrong Content-Disposition entry, name=\" is unclosed ??" << endl;
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
if (debugparsereq) errlog() << "we've a form post !" << endl;
			populate_reqfields_from_urlencodebody (request.req_body, request.body_fields);
		    } else {
			errlog() << "unhandled request-body' Content-Type : " << mi->second << endl;	// JDJDJDJD this could have been detected earlier, and the connection shut earlier
			request.set_relative_expires (0);   // this isn't cachable, the uri may be identical and mimes differents
			gettimeofday(&entering_NowTreatRequest, NULL);
			returnerror.shortcuterror ((*out), request, 415, NULL, "the provided Content-type for request body cannot be handled by server");
			gettimeofday(&entering_WaitingEOW, NULL);
			state = WaitingEOW;
			flushandclose();
			break;
		    }
		    gettimeofday(&entering_NowTreatRequest, NULL);
		    state = NowTreatRequest;
		    setlinemode();
		    break;
		}
		break;

	    case NowTreatRequest:   // ----------------------------------------------------------------
		break;
	}
// errlog << "state=" << state << " : request.reqbodylen=" << request.reqbodylen << " request.readbodybytes=" << request.readbodybytes << endl;
	if (state == NowTreatRequest) {
if (debugparsereq) {
    errlog() << "============================" << endl;
    errlog() << endl
	 << ostreamMap(request.body_fields, "      body_fields") << endl;
}

	    request.req_fields.import(request.uri_fields);
	    request.req_fields.import(request.body_fields);
if (debugparsereq) {
    errlog() << endl
	 << request.req_fields << endl
	 << request.content_fields << endl;
}

	    MimeHeader::iterator mi_host = request.mime.find ("Host");
	    if (mi_host == request.mime.end()) {
		errlog() << "missing Host mime entry" << endl;
		request.set_relative_expires (60);
		returnerror.shortcuterror ((*out), request, 503, NULL, "missing Host mime entry");  // JDJDJDJD we should have a default host
								    // JDJDJDJD we should be able to tune the error message (Unknown virtual host.)
		gettimeofday(&entering_WaitingEOW, NULL);
		state = WaitingEOW;
		flushandclose();
		return;
	    } 
	    request.host = mi_host->second;
	    THostMapper::iterator mi = hostmapper.find (request.host);
	    if (mi == hostmapper.end()) {
		if (wehaveadefaulthost) {
		    mi = mi_defaulthost;
		} else { 
		    errlog() << "unhandled Host :" << request.host << endl;
		    request.set_relative_expires (60);
		    returnerror.shortcuterror ((*out), request, 404, "Unkown Virtual Host");  // JDJDJDJD we should have a default host
									// JDJDJDJD we should be able to tune the error message (Unknown virtual host.)
		    gettimeofday(&entering_WaitingEOW, NULL);
		    state = WaitingEOW;
		    flushandclose();
		    return;
		}
	    }
	    TreatRequest* treatrequest = mi->second->treatrequest (request);
	    if (treatrequest == NULL) {
		errlog() << "NULL treatrequest from hostmapper for Host " << request.host << endl;
		request.set_relative_expires (60);
		returnerror.shortcuterror ((*out), request, 404, NULL, "No Handler for URI");  // JDJDJDJD we should be able to tune the err
		gettimeofday(&entering_WaitingEOW, NULL);
		state = WaitingEOW;
		flushandclose();
		return;
	    }
	    switch (treatrequest->output ((*out), request)) {
		case TRCompleted:
		    gettimeofday(&entering_WaitingEOW, NULL);	// ibid finishtreatment (void)
		    state = WaitingEOW;
		    flush();
		    break;
		case TRPending:
		    state = TreatPending;
		    if (cp != NULL)
			cp->reqnow (fd);
		    // JDJDJDJD sonme stuff missing here, probably ...
		    break;
	    }
	}
    }

    void HttppConn::finishtreatment (void) {
	gettimeofday(&entering_WaitingEOW, NULL);
	state = WaitingEOW;
	flush();
    }

    void HttppConn::reconnect_hook (void) {
	// BuggyConn aren't logged twice here ...
	if (state == WaitingEOW) {  // probably a shortened transmission !
	    gettimeofday(&ending_WaitingEOW, NULL);
	    request.statuscode = 999;	    // JDJDJDJD don't really know what to do here 
	    request.logger();
	    request.clear ();
	}
	SocketConnection::reconnect_hook(); // this one schedules for destruction ...
    }

    void HttppConn::eow_hook (void) {
// for now we also log BuggyConn here :
if ((state != WaitingEOW) && (state != HTTPRequestLine))
errlog() << "HttppConn::eow_hook called while not in WaitingEOW or HTTPRequestLine !!  state = " << (int) state << endl;

	// BuggyConn aren't logged twice here ...
	if (state == WaitingEOW) {	// JDJDJDJD logging should go here !!!
	    gettimeofday(&ending_WaitingEOW, NULL);
	    request.logger();
	    request.clear ();
	    state = HTTPRequestLine;
	    lastbwindex = gettotw();
	}
	BufConnection::eow_hook();	// JDJDJDJD not very good, should be called internally
    }

    ostream * HTTPRequest::clog = &cerr;

    

    SillyConsole::SillyConsole (int fd, BulkRaysCPool *bcp)
      : BufConnection (fd),
	bcp(bcp)
    {	stringstream ss;
	ss << "SillyConsole-fd[" << fd << "]";
	name = ss.str();
    }

    SillyConsole::~SillyConsole (void) {
	// nothing to do there yet
    }

    SillyConsoleIn::SillyConsoleIn (int fd, SillyConsoleOut *sco, BulkRaysCPool *bcp)
      : BufConnection (fd),
	sco(sco),
	bcp(bcp)
    {	stringstream ss;
	ss << "SillyConsoleIn-fd[" << fd << "]";
	name = ss.str();
	(*sco->out) << "Bulkrays SillyConsole" << endl;
	sco->flush();
    }

    SillyConsoleIn::~SillyConsoleIn (void) {
	// nothing to do there yet
    }

    void sillyconsolelineread (BufConnection &bcin, BufConnection &bcout, BulkRaysCPool *bcp) {
	size_t p = 0, s = bcin.bufin.size();
	string command;
	while ((p<s) && isspace (bcin.bufin[p])) p++;
	while ((p<s) && isalnum (bcin.bufin[p])) command += bcin.bufin[p++];

	if (command.size() == 0)
	    return;

	if (command == "turnoff") {
	    (*bcout.out) << "sending exitrequest to connection-pool" << endl;
	    bcout.flush();
	    if (bcp != NULL) {
		bcp->askforexit (" exit request from Sillyconsole");
	    } else {
		(*bcout.out) << "failed : bcp == NULL ?" << endl;
		bcout.flush();
	    }
	} else if (command == "help") {
	    (*bcout.out) << "turnoff         closes the server cleanly" << endl
			 << "help             this help" << endl
		;
	    bcout.flush();
	} else {
	    (*bcout.out) << "unrecognised command : \"" << command << '"' << endl;
	    bcout.flush();
	}
	(*bcout.out) << "> ";
	bcout.flush ();
    }

    void BulkRaysCPool::askforexit (const char * reason) {
	exitselect = true;
	time_t t;
	time (&t);
	struct tm tm;
	gmtime_r(&t, &tm);

	cerr 
	    << "[" << setfill('0')
	       << setw(2) << tm.tm_mday << '/'
	       << setw(2) << tm.tm_mon+1 << '/'
	       << setw(4) << tm.tm_year + 1900 << ':'
	       << setw(2) << tm.tm_hour << ':'
	       << setw(2) << tm.tm_min << ':'
	       << setw(2) << tm.tm_sec << "] "
	    << "[CP :-: BulkRaysCPool] "
	    << " asked to exit : "
	    << reason << endl;
    }

    void BulkRaysCPool::treat_signal (void) {
int i;
for (i=0 ; i<256 ; i++) {
    if (pend_signals [i] != 0)
	cerr << "got signal " << i << " " << pend_signals [i] << " times" << endl;
}
	if (pend_signals [SIGQUIT] != 0) {
	    askforexit ("got signal SIGQUIT");
	    pend_signals [SIGQUIT] = 0;
	}
	if (pend_signals [SIGINT] != 0) {
	    askforexit ("got signal SIGINT");
	    pend_signals [SIGINT] = 0;
	}
	ConnectionPool::treat_signal();
    } 


    bool THostMapper::deregisterall (URIMapper* um) {
	if (um == NULL) return false;
	bool found = false;
	THostMapper::iterator mi;
	for (mi=begin() ; mi != end() ;) {
	    THostMapper::iterator mj = mi;
	    mi ++;
	    if (mj->second == um) {
		erase (mj);
		found = true;
	    }
	}
	return found;
    }


    ostream& HTTPClient::shorterrlog (void) {
	time_t t;
	time (&t);
	struct tm tm;
	gmtime_r(&t, &tm);

	return cerr 
	    << "[" << setfill('0')
	       << setw(2) << tm.tm_mday << '/'
	       << setw(2) << tm.tm_mon+1 << '/'
	       << setw(4) << tm.tm_year + 1900 << ':'
	       << setw(2) << tm.tm_hour << ':'
	       << setw(2) << tm.tm_min << ':'
	       << setw(2) << tm.tm_sec << "] \""
	    << "[" << fd << " : : HTTPClient::" << prevhost << "] ";
    }

    ostream& HTTPClient::errlog (void) {
	if (presponse != NULL)  {
	    return shorterrlog()
		<< presponse->method << " () "
		<< (string)curspliturl
		<< " ";
	} else {
	    return shorterrlog() << " undefined_HTTPClient " ;
	}
    }

    HTTPClient::HTTPClient (bool keepalive)
      : BufConnection (-1, true),
	state(creation),
	keepalive(keepalive),
	timeout (0),
	ascb (NULL),
	callbackvalue (-1),
	presponse(NULL)
    {	
    }

    HTTPClient::~HTTPClient (void) {
    }

    void HTTPClient::reconnect_hook (void) {
	switch (state) {
	    case creation:
		errlog() << "HTTPClient::reconnect_hook state=creation ???? wondering how this could happen ?" << endl;
		break;
	    case waitingHTTPline1:
	    case mimeheadering:
	    case nextmimeheadering:
	    case docreceiving:
		errlog() << " reconnect_hook short transmission" << endl;
		setlinemode();
		state = waiting_cbtreat;
		if (ascb != NULL)
		    ascb->callback (callbackvalue);
		break;
	    case waiting_cbtreat:
		errlog() << " reconnect_hook in waiting_cbtreat" << endl;
		break;
	}
    }


// http://user:pass@tagazon.tragacouet.cn:80/thepath/part/ofthe/url?thefieldspart=ofthequery&b=0
//        p        r                        q 
//        p         r                       q 


    SplitUrl::SplitUrl (const string &url) {

//    void urlsplit (const string &url,
//	  bool &userpass,
//	string &user,
//	string &pass,
//	string &host,
//	   int &port,
//	string &uripath
//    ) {
	userpass = false;
	hostport.port = 80;

	size_t p = 0;
	string fqdnpart;
	if (url.substr (0, 7) == "http://")
	    p = 7;

	size_t q = url.find ('/', p);
	size_t r = url.rfind ('@', q);
	if ((r == string::npos) || (r < p)) {	// no userpass
	    r = p;
	} else {    // we have a userpass
	    userpass = true;
	    size_t userend = url.find_first_of (":@", p);
	    if ((userend != string::npos) && (userend > r))
		userend = r;
	    user = url.substr (p, userend-p);
	    if (userend < r)
		pass = url.substr (userend+1, r-userend-1);
	    r++;
	}
	size_t portbegin = url.find_first_of (":/", r);
	if ((portbegin != string::npos) && (portbegin != q)) {
	    hostport.port = atoi (url.substr(portbegin+1, (q==string::npos)? string::npos : portbegin+1 - q).c_str());
	    hostport.host = url.substr (r, portbegin-r);
	} else {
	    hostport.host = url.substr (r, (q==string::npos)? string::npos : q-r);
	}
	
	if (q != string::npos)
	    uripath = url.substr(q);

cerr <<        setfill (' ') << setw(45) << url
     << "|" << setfill (' ') << (userpass ? " # " : "   " )
     << "|" << setfill (' ') << setw(10) << user
     << "|" << setfill (' ') << setw(10) << pass
     << "|" << setfill (' ') << setw(35) << hostport.host
     << "|" << setfill (' ') << setw(3) << hostport.port
     << "|" << uripath << "|" << endl;

    }


    string HTTPClient::getname(void) {
	return string (curspliturl);
//	if (pcurspliturl != NULL)
//	    return string (*pcurspliturl);
//	else
//	    return "[free]";
    }

    string & appenduint16tos (unsigned int i, string &s) {
	int e=1000000000;
	unsigned int r;
	while ((e>0) && ((r = i/e) == 0)) e /= 10;
	if (e <= 0) {
	    s += "0";
	    return s;
	}
	if (r >= 10) {
	    s += "Nan";
	    return s;
	}
	s += ('0' + r); i-= r*e; e /= 10;
//cerr << "i=" << i << " e=" << e << " r=" << e << " s=" << s << endl;
	while (e>0) {
	    r = i/e;
	    s += ('0' + r); i-= r*e; e /= 10;
//cerr << "i=" << i << " e=" << e << " r=" << e << " s=" << s << endl;
	}
	return s;
    }

    SplitUrl::operator string () const {
	string s;
	if (userpass) {
	    s += user;
	    s += ':';
	    s += pass;
	    s += '@';
	}
	s += hostport.host;
	if (hostport.port != 80) {
	    s += ':';
	    appenduint16tos (hostport.port, s);
	}
cerr << endl << "********************************************** " << s << endl;
	s += uripath;
cerr << "********************************************** " << s << endl << endl;;
	return s;
    }

    bool HTTPClient::http_get (const SplitUrl &spurl, HTTPResponse &response, ASyncCallBack *ascb, int callbackvalue) {
	presponse = &response;
	curspliturl = spurl;
//	response.clear();
	HTTPClient::ascb = ascb;
	HTTPClient::callbackvalue = callbackvalue;
//	bool userpass;
//	string user, pass, host, uripath;
//	int port;
//
//	urlsplit (url, userpass, user, pass, host, port, uripath);

	// MISSING : checking keepalive value and if host is the same and if the connection is ok ...

////// if we wanted to make ipv4 / fqdn discrimination (from rfc2396.txt)
//	size_t lastdot = host.rfind('.');
//	// IPV4 discriminator
//	if ((lastdot != npos) && (lastdot < host.size()-1) && (isdigit(host[lastdot+1]))) {
//	    
//	}

	// JDJDJDJD should switch to some non-blocking connect !!!!
	// ideally inherited from a special Connection parent ???
	int propfd = init_connect (spurl.hostport.host.c_str(), spurl.hostport.port);

	if (propfd < 0)		// JDJDJDJD should log bad connections attempt ????
	    return false;

	if (cp != NULL) {	// JDJDJDJD qiconn should provide such things no ?
	    ConnectionPool *ocp = cp;
	    deregister_from_pool ();
	    fd = propfd;
	    register_into_pool (ocp);
	}

	(*out)	<< "GET " << spurl.uripath << " HTTP/1.1" << endl
		<< "Accept: */*" << endl;
	if (spurl.hostport.port != 80)
	    (*out) << "Host: " << spurl.hostport.host << ':' << spurl.hostport.port << endl;
	else
	    (*out) << "Host: " << spurl.hostport.host << endl;
	(*out)  << "Connection: " << "close" << endl	    // JDJDJDJD to be fixed
		<< endl;

	flush();
	state = waitingHTTPline1;

	return true;
    }

    
    bool HTTPClient::http_post_urlencoded (const SplitUrl &spurl, FieldsMap& vals, HTTPResponse &response, ASyncCallBack *ascb, int callbackvalue) {
	presponse = &response;
	curspliturl = spurl;
//	response.clear();
	HTTPClient::ascb = ascb;
	HTTPClient::callbackvalue = callbackvalue;
//	bool userpass;
//	string user, pass, host, uripath;
//	int port;
//
//	urlsplit (url, userpass, user, pass, host, port, uripath);


	// MISSING : checking keepalive value and if host is the same and if the connection is ok ...

////// if we wanted to make ipv4 / fqdn discrimination (from rfc2396.txt)
//	size_t lastdot = host.rfind('.');
//	// IPV4 discriminator
//	if ((lastdot != npos) && (lastdot < host.size()-1) && (isdigit(host[lastdot+1]))) {
//	    
//	}

	// JDJDJDJD should switch to some non-blocking connect !!!!
	// ideally inherited from a special Connection parent ???
	int propfd = init_connect (spurl.hostport.host.c_str(), spurl.hostport.port);

	if (propfd < 0)
	    return false;

	if (cp != NULL) {
cerr << "auto-de-re-enregistration !!!" << endl;
	    ConnectionPool *ocp = cp;
	    deregister_from_pool ();
	    fd = propfd;
	    register_into_pool (ocp);
	}


cerr << "entering" << endl;
	stringstream s;
	FieldsMap::const_iterator mi;
	for (mi=vals.begin() ; mi!=vals.end() ; mi++) {
	    if (mi != vals.begin())
		s << '&';
	    urlencode (s, mi->first);
	    s << '=';
	    urlencode (s, mi->second);
	}
cerr << "leaving : " << s.str() << endl;

	(*out)	<< "POST " << spurl.uripath << " HTTP/1.1" << endl
		<< "Accept: */*" << endl;
	if (spurl.hostport.port != 80)
	    (*out) << "Host: " << spurl.hostport.host << ':' << spurl.hostport.port << endl;
	else
	    (*out) << "Host: " << spurl.hostport.host << endl;
	(*out)  << "Connection: " << "close" << endl	    // JDJDJDJD to be fixed
		<< "Content-Type: application/x-www-form-urlencoded" << endl
		<< "Content-Length: " << s.str().size() << endl
		<< endl
		<< s.str();

	flush();
	state = waitingHTTPline1;

	return true;
    }

    void HTTPClient::lineread (void) {
	if (presponse == NULL) {
	    cerr << "HTTPClient::lineread we have serious troubles with a presponse=NULL here !" << endl;   // JDJDJDJD should close the link and reset
	    /* JDJDJDJD
		some serious stuff missing here
		*/	
	    return;
	}
	HTTPResponse &response = *presponse;
	size_t len, p;
	switch (state) {
	    case creation:
		cerr << "HTTPClient::lineread receiving garbage during creation !" << endl;
		break;
	    case waitingHTTPline1: {
		    size_t p = bufin.find (' ');
		    if (p == string::npos) {
			cerr << "HTTPClient::lineread malformed http answer" << endl;
// JDJDJDJD should close the connection here
			break;
		    }
		    if (bufin.substr (0, p) != "HTTP/1.1") {
			cerr << "HTTPClient::lineread unknown http variant : [" << bufin.substr (0, p) << "]" << endl;
// JDJDJDJD should close the connection here
			break;
		    }
		    if (p+1 > bufin.size()) {
			cerr << "HTTPClient::lineread missing http return code !" << endl;
// JDJDJDJD should close the connection here
			break;
		    }

		    response.statuscode = atoi (bufin.substr(p+1).c_str());

cerr << bufin << endl;
		    state = mimeheadering;
		}
		break;

	    case mimeheadering:
		if (bufin.empty()) {
		    response.compute_bodylen ();
		    if (response.reqbodylen > 0) {
			state = docreceiving;
			setrawmode();
		    } else {
			state = waiting_cbtreat;
		    }
		    break;
		}
		if (!isalnum(bufin[0])) {
		    cerr << "HTTPClient::readline : wrong mime header-name (bad starting char ?) : " << bufin << endl;
// JDJDJDJD should close the connection here
		    break;
		}

	    case nextmimeheadering:
		len = bufin.size();
		if (len == 0) {
		    if (!mimeheadername.empty()) {
			response.mime[mimeheadername] = mimevalue;   // JDJDJDJD should check for pre-existing mime entry
		    }
		    response.compute_bodylen ();
		    if (response.reqbodylen > 0) {
			state = docreceiving;
			setrawmode();
		    } else {
			state = waiting_cbtreat;
		    }
		    break;
		}
		p = 0;
		if ((bufin[p] == ' ') || (bufin[p] == 9)) {	// are we on a continuation of mime value ?
		    while ((p < len) && ((bufin[p] == ' ') || (bufin[p] == 9)))
			p++;
		    while (p < len)
			mimevalue += bufin[p++];
		
		    response.mime[mimeheadername] = mimevalue;	// JDJDJDJD should check for pre-existing mime entry
// errlog() << mimeheadername << " = " << mimevalue << endl;
		    mimevalue.clear();
		    mimeheadername.clear();
		    state = nextmimeheadering;
		    break;
		}
		if (!mimeheadername.empty()) {
		    response.mime[mimeheadername] = mimevalue;   // JDJDJDJD should check for pre-existing mime entry
// errlog() << mimeheadername << " = " << mimevalue << endl;
		}
		p = bufin.find (':');
		if (p == string::npos) {
		    errlog() << "wrong mime header-name (missing ':' ?) : " << hexdump(bufin) << endl;
		    cerr << "HTTPClient::lineread missing http return code !" << endl;
// JDJDJDJD should close the connection here
		    break;
		}
		if (!isalnum(bufin[0])) {
		    errlog() << "wrong mime header-name (bad starting char ?) : " << hexdump(bufin) << endl;
		    cerr << "HTTPClient::lineread missing http return code !" << endl;
// JDJDJDJD should close the connection here
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
		state = nextmimeheadering;
		break;

	    case docreceiving:
		response.req_body += bufin;
		response.readbodybytes += bufin.size();
		if (response.readbodybytes >= response.reqbodylen) {
errlog() << "ReadBody : " << response.readbodybytes << " read, " << response.reqbodylen << " schedulled.  diff = " << response.readbodybytes-response.reqbodylen << endl;
cerr << ostreamMap(response.mime,           BEGIN_TERM_IDENT "mime"            END_TERM_IDENT) << endl;
		    setlinemode();
		    response.fulltransmission = true;
		    state = waiting_cbtreat;
		    if (ascb != NULL)
			ascb->callback (callbackvalue);
		    break;
		}
		break;

	    case waiting_cbtreat:
		cerr << "HTTPClient: receiving some stuff while in waiting_cbtreat state ?!" << endl;
		break;

	    default:
		cerr << "HTTPClient: strange unhandled case occured at lineread ? ?!" << endl;
		break;
	}
	
    }


// ---------------------- TO BE REMOVED ------------------------
// ---------------------- TO BE REMOVED ------------------------

class SillyHttpGet : public ASyncCallBack {
	HTTPClient *hc;
	string theurl;
	HTTPResponse response;
    public:
	SillyHttpGet (string url) :
	    theurl(url)
	{
	    hc = new HTTPClient ();
	    if (hc == NULL) {
cerr << "complain !" << endl;
		return;
	    }
	    hc->register_into_pool (&bulkrayscpool);

cerr << "here" << endl;
	    response.clear();
	    hc->http_get (url, response, this, 42);
cerr << theurl << endl;
cerr << "there" << endl;
	}
	virtual int callback (int v) {
	    if (v == 42) {
		if (hc == NULL) return -1;
		cerr << "--------------------" << endl
		     << theurl << endl
		     << "--------------------" << endl
		     << response.req_body << "<--------" << endl
		     << "--------------------" << endl;
		hc->deregister_from_pool();
cerr << "about to delete hc" << endl;
///////////////		delete hc;
		hc = NULL;
	    }
	    return 0;
	}
	virtual ~SillyHttpGet (void) {
cerr << "destruction" << endl;
	}
};


class SillyHttpPut : public ASyncCallBack {
	HTTPClient *hc;
	string theurl;
	HTTPResponse response;
    public:
	SillyHttpPut (string url, FieldsMap &vals) :
	    theurl(url)
	{
	    hc = new HTTPClient ();
	    if (hc == NULL) {
cerr << "complain !" << endl;
		return;
	    }
	    hc->register_into_pool (&bulkrayscpool);

cerr << "here" << endl;
	    response.clear();
	    hc->http_post_urlencoded (url, vals, response, this, 42);
cerr << theurl << endl;
cerr << "there" << endl;
	}
	virtual int callback (int v) {
	    if (v == 42) {
		if (hc == NULL) return -1;
		cerr << "--------------------" << endl
		     << theurl << endl
		     << "--------------------" << endl
		     << response.req_body << "<--------" << endl
		     << "--------------------" << endl;
		hc->deregister_from_pool();
cerr << "about to delete hc (2)" << endl;
///////////////		delete hc;
		hc = NULL;
	    }
	    return 0;
	}
	virtual ~SillyHttpPut (void) {
cerr << "destruction" << endl;
	}
};


// ---------------------- TO BE REMOVED ------------------------
// ---------------------- TO BE REMOVED ------------------------




    // HTTPClientPool ----------------------------------------------------

    HTTPClientPool::HTTPClientPool (int maxpool) :
	maxpool(maxpool)
    {	vhc.resize(maxpool);
	int size = vhc.size();
	if (size != maxpool) {
	    cerr << "allocating HTTPClientPool::HTTPClientPool : "
		 << size << " pointers allocated instead of "
		 << maxpool << " pointers requested !" << endl;
	}
	maxpool = size;	// JDJDJDJD we're loosing the initial requested size ...

	int i;
	for (i=0 ; i<maxpool ; i++) {
	    HTTPClient *p = new HTTPClient(true);	    // JDJDJDJD keepalive could be choosed ??
	    if (p == NULL) {
		cerr << "allocating HTTPClientPool::HTTPClientPool : "
		     << i << " HTTPClientPool allocated instead of "
		     << maxpool << " HTTPClientPool requested !" << endl;
		break;
	    }
	    vhc[i] = p;
	    available_hc.push_back(vhc[i]);
	}
	maxpool = i;

	if (maxpool == 0) {
	    cerr << "allocating HTTPClientPool::HTTPClientPool : empty pool !!" << endl;
	}
    }

    // end of HTTPClientPool ---------------------------------------------










} // namespace bulkrays

using namespace std;
using namespace qiconn;
using namespace bulkrays;

int main (int nb, char ** cmde) {

    string user ("www-data");
    string group;
    string address ("0.0.0.0");
    int port = 80;
    string flogname("/var/log/bulkrays/access_log");	// JDJDJDJD we should introduce a DEFINEd scheme for such defaults
    bool activatettyconsole = false;


    bool inproperties = false;

    int i;
    for (i=1 ; i<nb ; i++) {
      if (!inproperties) {
	if (strncmp (cmde[i], "--help", 6) == 0) {
	    cout << cmde[0] << "   \\" << endl
			    << "      [--bind=[address][:port]]  \\" << endl
			    << "      [--user=[user][:group]]    \\" << endl
			    << "      [--access_log=filename]    \\" << endl
			    << "      [--earlylog]               \\" << endl
			    << "      [--console]" << endl;
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
	    // cerr << "will try to bind : " << address << ":" << port << endl;
	} else if (strncmp (cmde[i], "--earlylog", 10) == 0) {
	    debugearlylog = true;
	} else if (strncmp (cmde[i], "--debugparsereq", 15) == 0) {
	    debugparsereq = true;
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
	} else if (strncmp (cmde[i], "--console", 9) == 0) {
	    activatettyconsole = true;
	} else if ((strcmp (cmde[i],"-p") == 0) || (strcmp(cmde[i], "--properties"))) {
	    inproperties = true;
	    continue;
	} else {
	    cerr << "unknown option : " << cmde[i] << endl;
	}
      } else {	// here below inproperties is true !
	if (!isalnum (cmde[i][0])) {
	    cerr << "bad property-name : " << cmde[i] << endl;
	    continue;
	}
	size_t j=0;
	while ((cmde[i][j] != 0) && (cmde[i][j]!='=')) j++;
	if (cmde[i][j] != '=') {    // we have a simple property-name !
	    properties[cmde[i]] = string("y");	// we set it true
	    continue;
	}
	properties[string(cmde[i],j)] = string(cmde[i]+j+1);
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


    bulkrayscpool.init_signal ();
    bulkrayscpool.add_signal_handler (SIGQUIT);
    bulkrayscpool.add_signal_handler (SIGINT);
    
    bulkrayscpool.push (ls);

    SillyConsoleOut *psillyconsolestdout = NULL;
    SillyConsoleIn *psillyconsolestdin = NULL;
    if (activatettyconsole) {
	{   int flags;	// JDJDJDJD this part could be avoided if Connection object could be marked "not for reading" in ConnectionPool
	    if ((flags = fcntl (1, F_GETFL, 0)) < 0) {
		cerr << "fcntl F_GETFL failed on stdout" << endl;
	    } else {
		flags |= O_NONBLOCK;
		if (fcntl (1, F_SETFL, flags) < 0) {
		    cerr << "could not set O_NONBLOCK on stdout" << endl;
		}
	    }
	}
	if ((psillyconsolestdout = new SillyConsoleOut (1)) == NULL) {
	    cerr << "could not allocate SillyConsoleOut" << endl;
	} else {
	    psillyconsolestdout->register_into_pool (&bulkrayscpool, false);
	}
	if ((psillyconsolestdin = new SillyConsoleIn (0, psillyconsolestdout, &bulkrayscpool)) == NULL) {
	    cerr << "could not allocate SillyConsoleIn" << endl;
	} else {
	    bulkrayscpool.push (psillyconsolestdin);
	}
    }
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    status_message_globalinit ();
    bootstrap_global (bulkrays::StartUp);

// HTTPClient hc;

//    hc.httpget ("http://www.univ-lyon1.fr");
//    hc.httpget ("http://www.univ-lyon1.fr/");
//    hc.httpget ("http://www.univ-lyon1.fr/blabla");
//    hc.httpget ("http://jd:test@www.univ-lyon1.fr/blabla");
//    hc.httpget ("http://jd:@www.univ-lyon1.fr/blabla");
//    hc.httpget ("http://jd@www.univ-lyon1.fr/blabla");
//    hc.httpget ("http://www.univ-lyon1.fr:82/blabla");
//    hc.httpget ("http://jd@www.univ-lyon1.fr:82/blabla");
//    hc.httpget ("http://jd:test@www.univ-lyon1.fr:82/blabla");
//
//    hc.httpget ("www.univ-lyon1.fr");
//    hc.httpget ("www.univ-lyon1.fr/");
//    hc.httpget ("www.univ-lyon1.fr/blabla");
//    hc.httpget ("jd:test@www.univ-lyon1.fr/blabla");
//    hc.httpget ("jd:@www.univ-lyon1.fr/blabla");
//    hc.httpget ("jd@www.univ-lyon1.fr/blabla");
//    hc.httpget ("www.univ-lyon1.fr:82/blabla");
//    hc.httpget ("jd@www.univ-lyon1.fr:82/blabla");
//    hc.httpget ("jd:test@www.univ-lyon1.fr:82/blabla");


////    hc.httpget ("http://bulkrays2.nkdn.fr/fiches/TODO");
////    hc.register_into_pool (&bulkrayscpool);


// JDJDJDJD this should go away someday !
if (bulkrays::properties["BulkRays::ownsetoftests"]) {

{string s;
s = "" ; cerr << "[" << appenduint16tos (0,s) << "]" << endl;
s = "" ; cerr << "[" << appenduint16tos (1,s) << "]" << endl;
s = "" ; cerr << "[" << appenduint16tos (10,s) << "]" << endl;
s = "" ; cerr << "[" << appenduint16tos (1234,s) << "]" << endl;
s = "" ; cerr << "[" << appenduint16tos (132185434,s) << "]" << endl;;
}

    new SillyHttpGet("http://bulkrays2.nkdn.fr/fiches/TODO");
    FieldsMap vals;
    vals ["login"]    = "jd";
    vals ["srvfam"]   = "*";
    vals ["passhash"] = "yopYOP";
    new SillyHttpPut ("http://127.0.0.1:10082/check", vals);
}

    bulkrayscpool.select_loop (timeout);


    if (psillyconsolestdin != NULL) {
	psillyconsolestdin->deregister_from_pool();	// so that some other stuff can still be done on stdin/out
	delete (psillyconsolestdin);
    }
    if (psillyconsolestdout != NULL) {
	psillyconsolestdout->deregister_from_pool();
	delete (psillyconsolestdout);
    }

    cerr << "terminating" << endl;

    bulkrayscpool.closeall ();

    close (s);

    bootstrap_global (bulkrays::ShutDown);

    return 0;
}


