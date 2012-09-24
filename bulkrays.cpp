#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#define QICONN_H_GLOBINST
#define BULKRAYS_H_GLOBINST
#include "bulkrays.h"

namespace bulkrays {
    using namespace std;
    using namespace qiconn;


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
	cout << pdummyconnection->getname() << " " << method << " " << host << req_uri << " " << statuscode << " " << msg << endl;
    }

    void HTTPRequest::logger (const char *msg /*=NULL*/) {
	if (msg == NULL)
	    cout << pdummyconnection->getname() << " " << method << " " << host << req_uri << " " << statuscode << endl;
	else
	    cout << pdummyconnection->getname() << " " << method << " " << host << req_uri << " " << statuscode << " " << msg << endl;
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
	
	body << xhtml_header << endl;
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

	stringstream head;
	head << "HTTP/1.1 " << req.statuscode << " " << status_message[req.statuscode] << endl
	     << "Server: BulkRays/" << BULKRAYSVERSION << endl
	     << "Content-Type: text/html" << endl
	     << "Accept-Ranges: bytes" << endl
	     << "Retry-After: 5" << endl	// JDJDJDJD should be tunable
	     << "Connection: close" << endl;
	head << "Content-Length: " << body.str().size() << endl;
	head << endl;
	cout << head.str() << body.str();

	req.logger ("yop");

	return 0;
    }

    int ReturnError::shortcuterror (ostream &cout, HTTPRequest &req, int statuscode, const char* message /*=NULL*/, const char* submessage /*=NULL*/) {
	req.errormsg = message, req.suberrormsg = submessage, req.statuscode = statuscode;
	return output (cout, req);
    }

    int TreatRequest::error (ostream &cout, HTTPRequest &req, int statuscode, const char* message /*=NULL*/, const char* submessage /*=NULL*/) {
	return returnerror.shortcuterror (cout, req, statuscode, message, submessage);
    }

    HttppConn::HttppConn (int fd, struct sockaddr_in const &client_addr) : DummyConnection(fd, client_addr),request(*this) {
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

    void HttppConn::lineread (void) {
	size_t p, q, len;
	MimeHeader::iterator mi;

	switch (state) {
	    case HTTPRequestLine:
		p = bufin.find (' ');
		if (p == string::npos) {
		    request.method = bufin; // JDJDJDJD unused ????
		    cerr << "wrong request line (method only ?): " << bufin << endl;
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong request line (method only ?)");
		    flushandclose();
		    break;
		}
		request.method = bufin.substr (0, p);
cout << "[" << id << "] method = " << request.method << endl;
		q = p+1, p = bufin.find (' ', q);
		if (p == string::npos) {
		    request.req_uri = bufin.substr(q);
		    cerr << "wrong request line (missing version ?): " << bufin << endl;
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong request line (missing http version ?)");
		    flushandclose();
		    break;
		}
		request.req_uri = bufin.substr (q, p-q);
cout << "[" << id << "] req_uri = " << request.req_uri << endl;
		q = p+1, p = bufin.find (' ', q);
		request.version = bufin.substr(q);
cout << "[" << id << "] version = " << request.version << endl;
		state = MIMEHeader;
		break;
    
	    case MIMEHeader:
		if (bufin.empty()) {
		    state = EndOfMIMEHeader;
		    for (mi=request.mime.begin() ; mi!=request.mime.end() ; mi++)
			cout << "[" << id << "]    mime[" << mi->first << "] = <" << mi->second << ">" << endl;
		    break;
		}
		if (!isalnum(bufin[0])) {
		    cerr << "wrong mime header-name (bad starting char ?) : " << bufin << endl;
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
		    state = EndOfMIMEHeader;
		    for (mi=request.mime.begin() ; mi!=request.mime.end() ; mi++)
			cout << "[" << id << "]    mime[" << mi->first << "] = <" << mi->second << ">" << endl;
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
		    returnerror.shortcuterror ((*out), request, 400, NULL, "wrong mime header-name (missing ':' ?)");
		    flushandclose();
		    break;
		}
		if (!isalnum(bufin[0])) {
		    cerr << "wrong mime header-name (bad starting char ?) : " << bufin << endl;
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

	    case EndOfMIMEHeader:
		cout << "[" << id << "] endofmimeheader :  " << bufin << endl;
		break;
	}
	if (state == EndOfMIMEHeader) {
	    MimeHeader::iterator mi_host = request.mime.find ("Host");
	    if (mi_host == request.mime.end()) {
		cerr << "missing Host mime entry" << endl;
		returnerror.shortcuterror ((*out), request, 503, NULL, "missing Host mime entry");  // JDJDJDJD we should have a default host
								    // JDJDJDJD we should be able to tune the error message (Unknown virtual host.)
		flushandclose();
		return;
	    } 
	    request.host = mi_host->second;
	    THostMapper::iterator mi = hostmapper.find (request.host);
	    if (mi == hostmapper.end()) {
		cerr << "unhandled Host :" << request.host << endl;
		returnerror.shortcuterror ((*out), request, 404, "Unkown Virtual Host");  // JDJDJDJD we should have a default host
								    // JDJDJDJD we should be able to tune the error message (Unknown virtual host.)
		flushandclose();
		return;
	    }
	    TreatRequest* treatrequest = mi->second->treatrequest (request);
	    if (treatrequest == NULL) {
		cerr << "NULL treatrequest from hostmapper for Host " << request.host << endl;
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
}

using namespace std;
using namespace qiconn;
using namespace bulkrays;

int main (int nb, char ** cmde) {
    int port = 10080;


    if (nb < 2)
	return 1;
    char * addr = cmde [1];




    int s = server_pool (port, addr);
    if (s < 0) {
	cerr << "could not instanciate connection pool, bailing out !" << endl;
	return -1;
    }
    HttppBinder *ls = new HttppBinder (s, port, addr);
    if (ls == NULL) {
	cerr << "could not instanciate HttppBinder, bailing out !" << endl;
	return -1;
    }

    ConnectionPool cp;

    cp.init_signal ();
    
    cp.push (ls);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    status_message_globalinit ();
    bootstrap_global ();

    cp.select_loop (timeout);

    cerr << "terminating" << endl;

    close (s);
    return 0;
}


