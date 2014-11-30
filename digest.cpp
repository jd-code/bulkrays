#include "config.h"

#include <fstream>

#include <errno.h>

#include <bulkrays/bulkrays.h>

namespace bulkrays {

    int64_t random64fromrandom (void) {
	ifstream ifrand ("/dev/random");
	if (!ifrand) {
	    int e = errno;
	    cerr << "randomfromrandom : could not open /dev/random : " << strerror (e) << endl;
	    return RANDFRMRANDERR;
	}
	int64_t v;
	ifrand.read ((char *)&v, sizeof(v));
	if (ifrand.gcount() != sizeof(v))
	    return RANDFRMRANDERR;
	return v;
    }


    int64_t hex2i64 (const string &s, size_t p /* =0 */) {
	int64_t r = 0;
	size_t l = s.size();
	int i = 0;
	while ((p<l) && (i<16)) {
	    r <<= 4;
	    switch (s[p]) {
		case '0':	r+=0;	break;
		case '1':	r+=1;	break;
		case '2':	r+=2;	break;
		case '3':	r+=3;	break;
		case '4':	r+=4;	break;
		case '5':	r+=5;	break;
		case '6':	r+=6;	break;
		case '7':	r+=7;	break;
		case '8':	r+=8;	break;
		case '9':	r+=9;	break;
		case 'a':	r+=10;	break;
		case 'b':	r+=11;	break;
		case 'c':	r+=12;	break;
		case 'd':	r+=13;	break;
		case 'e':	r+=14;	break;
		case 'f':	r+=15;	break;
		default: return 0;
	    }
	    p++, i++;
	}
	if (i!=16) return 0;
	return r;
    }

    ParsedMimeEntry::ParsedMimeEntry (string const & s) : sorig(s) {
	size_t p=0, l=s.size();
	// find main name
	while ((p<l) && (isspace(s[p]))) { p++; }        if (p==l) return;
	while ((p<l) && (isalnum(s[p]))) { mainname += s[p]; p++; } if (p==l) return;

	// loop for ident=["]data["][,]
	while (p<l) {
	    string ident;
	    size_t start, len;
	    while ((p<l) && (isspace(s[p]))) { p++; }		     if (p==l) return;
	    while ((p<l) && (isalnum(s[p]))) { ident += s[p]; p++; } if (p==l) { m[ident]=pair<int,int>(0,0); return; }
	    while ((p<l) && (isspace(s[p]))) { p++; }		     if (p==l) { m[ident]=pair<int,int>(0,0); return; }
	    if (s[p] != '=') { m[ident]=pair<int,int>(0,0); return; }	// we bail out parsing : missing "equal"
	    p++;
	    while ((p<l) && (isspace(s[p]))) { p++; }		     if (p==l) { m[ident]=pair<int,int>(0,0); return; }
	    if (s[p] == '"') {
		p++;
		if (p>=l) { m[ident]=pair<int,int>(0,0); return; }
		start = p, len = 0;
		while ((p<l) && (s[p] != '"')) { p++, len++; };
		m[ident]=pair<int,int>(start,len);
		if (p==l) return;
		p++; // we skip the '"'
	    } else {
		start = p, len = 0;
		while ((p<l) && (s[p] != ',') && (!isspace(s[p]))) { p++, len++; }
		m[ident]=pair<int,int>(start,len);
		if (p==l) return;
	    }
	    if (s[p] == ',') p++;
	}
    }

    void ParsedMimeEntry::dump (ostream & cout) {
	map <string, pair<int,int> >::const_iterator mi;
	cout << "[" << mainname << "]" << endl;
	for (mi=m.begin() ; mi!=m.end() ; mi++) {
	    cout << "     [" << mi->first << "] = \"" << sorig.substr (mi->second.first, mi->second.second) << "\"" << endl;
	}
    }

    DigestAuthTag::DigestAuthTag (string const &nonce, time_t allowance /*= 15*60*/) : nonce(nonce) {
	expiration = time(NULL) + allowance;
	mdigest[nonce] = this;
    }

    void DigestAuthTag::addallowance (time_t allowance /* = 15*60 */) {
	expiration = time(NULL) + allowance;
    }

    DigestAuthTag* DigestAuthTag::finddtag (string &nonce) {
	map<string, DigestAuthTag*>::iterator mi = mdigest.find (nonce);
	if (mi == mdigest.end())
	    return NULL;
	if (mi->second->expiration > time(NULL)) {
cerr << "erasing nonce = " << nonce << endl;
	    delete (mi->second);			    // JDJDJD there's a serious potential race here, we should have a lock
	    return NULL;
	}
	return (mi->second);
    }

    void DigestAuthTag::gennonce (string &s) {
	nonceseed ++;
	int64_t v = nonceseed;
	int64_t t = time(NULL);
static const char *hex = "0123456789abcdef";
	for (int i=0 ; i<16 ; i++) {
	    s += hex[(t & 0xf000000000000000)>>60];
	    t <<= 4;
	}
	for (int i=0 ; i<16 ; i++) {
	    s += hex[(v & 0xf000000000000000)>>60];
	    v <<= 4;
	}
    }

    DigestAuthTag::~DigestAuthTag () {
	map<string, DigestAuthTag*>::iterator mi = mdigest.find(nonce);
	if (mi == mdigest.end()) {
	    cerr << "~DigestAuthTag : could not find nonce " << nonce << " at destruction ???" << endl;
	    return;
	}
	mdigest.erase (mi);
    }

    DigestAuth::DigestAuth (string realm, string domain, time_t expiration) :
	realm (realm),
	domain (domain),
	expiration (expiration)
	{}

    DigestAuth::~DigestAuth () {}

    bool DigestAuth::adduser (string user, string password) {
	map <string, string>::iterator mi = users.find (user);
	if (mi != users.end()) return false;	// a user is already here !
	users [user] = password;	    // a saffer way would be to store only the hash
	return true;
    }

    bool DigestAuth::updateuser (string user, string password) {
	map <string, string>::iterator mi = users.find (user);
	if (mi == users.end()) return false;	// no user to be updated !
	users [user] = password;	    // a saffer way would be to store only the hash
	return true;
    }

    bool DigestAuth::deluser (string user) {
	map <string, string>::iterator mi = users.find (user);
	if (mi == users.end()) return false;	// no user to be deleted !
	users.erase (mi);		    // JDJDJDJDJD we should find a way to invalidate all current nonce ?!
	return true;
    }

    TReqResult DigestAuth::output (ostream &cout, HTTPRequest &req) {
	return check (cout, req);
    }

    TReqResult DigestAuth::check (ostream &cout, HTTPRequest &req) {
	bool needauthentication = true;
	bool goodenoughforvalid = false;
	FieldsMap::iterator mi;

	if (req.mime.verif("Authorization", mi)) {
	    ParsedMimeEntry au(mi->second);
au.dump (cerr);
	    string nonce (au["nonce"]);
	    DigestAuthTag* dtag = DigestAuthTag::finddtag (nonce);
	    if (dtag == NULL) {	// not a validated nonce
		int64_t propt = hex2i64 (nonce);
cerr << "nonce=" << nonce << endl
     << "    t=" << setbase(16) << setfill('0') << setw(16) << propt << setbase(10) << endl;
		if ( time(NULL) - propt > expiration ) // the unvalidated nonce is too old		    JDJDJDJD be consistent here with further allowance 
		    needauthentication = true;
		else
		    goodenoughforvalid = true;
	    } else
		goodenoughforvalid = true;

	    if (goodenoughforvalid) {

		// lets look for the proper user
		map<string,string>::const_iterator mi_user = users.find (au["username"]);
		if (mi_user != users.end()) {	// the user is in the db
		    string  ha1s (mi_user->first);
			    ha1s += ':';
			    ha1s += au["realm"];
			    ha1s += ":";
			    ha1s += mi_user->second;	// pass
		    string  ha2s (req.method);
			    ha2s += ":";
			    // ha2s += req.req_uri;
			    ha2s += au["uri"];		// we use the provided uri because some bad client give complete url instead of uri (android web-client !)
		    string ha1, ha2;
		    md5 (ha1s, ha1);
		    md5 (ha2s, ha2);
		    string digest;
		    md5 (ha1+':'+au["nonce"]+':'+au["nc"]+':'+au["cnonce"]+':'+au["qop"]+':'+ha2, digest);
cerr << "digest calculated : " << digest << endl;

		    if (au["response"] == digest) {
			needauthentication = false;
			if (dtag == NULL) {
			    dtag = new DigestAuthTag(nonce, expiration);
			    if (dtag == NULL) {
				cerr << "FMap_TreatRequest::DigestAuthTag could not create a DigestAuthTag" << endl;
				return error (cout, req, 500, "could not create a DigestAuthTag");
			    }
			} else {
			    ha2 = "::";
			    ha2s += req.req_uri;
			    md5 (ha2s, ha2);
			    md5 (ha1+':'+au["nonce"]+':'+au["nc"]+':'+au["cnonce"]+':'+au["qop"]+':'+ha2, digest);
			    string &authinfo = req.outmime["Authentication-Info"];
			    authinfo = "rspauth=\"";
			    authinfo += digest;
			    authinfo += "\", "
				"cnonce=\"";
			    authinfo += au["cnonce"];
			    authinfo += "\", "
				"nc=\"";
			    authinfo += au["nc"];
			    authinfo += "\", "
				"qop=\"auth\"";

			    dtag->addallowance (expiration);
			}
		    }
		}
	    }
	}

	if (needauthentication) {
	    req.statuscode = 401;
	    // a new nonce is created
//		DigestAuthTag *dtag = new DigestAuthTag;
//		if (dtag == NULL) {
//		    cerr << "FMap_TreatRequest::DigestAuthTag could not create a DigestAuthTag" << endl;
//		    return error (cout, req, 500, "could not create a DigestAuthTag");
//		}

	    string &www_authenticate = req.outmime["WWW-Authenticate"];

	    www_authenticate = "Digest "
		"realm=\"tagazon of new york\", "
		"qop=\"auth\", "
		"opaque=\"5ccc069c403ebaf9f0171e9517f40e41\", "
		"nonce=\"";
	    DigestAuthTag::gennonce (www_authenticate);
	    www_authenticate += "\", "
		"domain=\"";
	    www_authenticate += "/";
	    www_authenticate += '"';
	    

	    req.outmime["Content-Type"] = "text/ascii";
	    stringstream s;
	    s << "yes you must authenticate" << endl;
	    req.set_contentlength (s.str().size());
	    req.publish_header();
	    cout << s.str();
	    return TRCompleted;
	}
	    
	return TRPending;
    }

    

} // namespace bulkrays
