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

//! implementation of rfc2617/rfc7616(partial only) : HTTP Digest Access Authentication
// JDJDJDJD missing :  periodic cleanup of map for old nonces


#include "config.h"

#include <fstream>

#include <stdlib.h>
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

    int64_t random64 (void) {
	int64_t v = 0;
#if RAND_MAX > 65536
	for (int i=0 ; i<4 ; i++) {
	    v += (random() & 0xffff);
	    v <<= 16;
	}
#else
	for (int i=0 ; i<8 ; i++) {
	    v += (random() & 0xff);
	    v <<= 8;
	}
#endif
	return v;
    }

    string & append_i64 (string &s, int64_t v) {
static const char *hex = "0123456789abcdef";
	for (int i=0 ; i<16 ; i++) {
	    s += hex[(v & 0xf000000000000000)>>60];
	    v <<= 4;
	}
	return s;
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
	if (mi->second->expiration < time(NULL)) {
cerr << "erasing nonce = " << nonce << endl
     << "        expiration = " << mi->second->expiration << endl
     << "              time = " << time(NULL) << endl;;
	    delete (mi->second);			    // JDJDJD there's a serious potential race here, we should have a lock
	    return NULL;
	}
	return (mi->second);
    }

    void DigestAuthTag::gennonce (string &s) {
	nonceseed ++;
	append_i64 (s, nonceseed);
	append_i64 (s, (int64_t)time(NULL));
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

    int DigestAuth::importdigestfile (const string &fname) {

// the digest-negociation needs the clear password somewhere ....
// file-format :
// <login>:<some realm>:<clearpassword>
// johndoe:treasure part:obafgkm42
// kent:secret files:verytrongpassword__

	ifstream input (fname.c_str());
	if (!input) {
	    int e = errno;
	    cerr << "DigestAuth::importdigestfile could not open file " << fname << " : " << strerror(e) << endl;
	    return 0;
	}
	int nline = 0;
	int nbuseradded = 0;
	while ((bool)input) {
	    string line;
	    char c;
	    while (input && (c = input.get()) && (!input.eof()) && (c!=10) && (c!=13)) line += c;
	    nline ++;
	    if (line.empty()) continue;
	    size_t p, q;
	    p = line.find (':');
	    if (p == string::npos) {
		cerr << "DigestAuth::importdigestfile malformed line " << fname << " at line " << nline << endl;
		continue;
	    }
	    string user = line.substr (0, p);
	    q = line.find (':', p+1);
	    if (p == string::npos) {
		cerr << "DigestAuth::importdigestfile malformed line " << fname << " at line " << nline << endl;
		continue;
	    }
	    string lrealm = line.substr (p+1, q-(p+1));
	    if (lrealm != realm) continue;
	    string hash = line.substr (q+1);
	    if (adduserhash (user, hash))
		nbuseradded ++;
	}

	return nbuseradded;
    }

    bool DigestAuth::adduserhash (string user, string hash) {
	map <string, string>::iterator mi = users.find (user);
	if (mi != users.end()) return false;	// a user is already here !
	users [user] = hash;
	return true;
    }

    bool DigestAuth::updateuserhash (string user, string hash) {
	map <string, string>::iterator mi = users.find (user);
	if (mi == users.end()) return false;	// no user to be updated !
	users [user] = hash;
	return true;
    }

    bool DigestAuth::adduser (string user, string password) {
	map <string, string>::iterator mi = users.find (user);
	if (mi != users.end()) return false;	// a user is already here !
	string ha1s = user + ':' + realm + ':' + password;
	string ha1;
	md5 (ha1s, ha1);
	users [user] = ha1;
	return true;
    }

    bool DigestAuth::updateuser (string user, string password) {
	map <string, string>::iterator mi = users.find (user);
	if (mi == users.end()) return false;	// no user to be updated !
	string ha1s = user + ':' + realm + ':' + password;
	string ha1;
	md5 (ha1s, ha1);
	users [user] = ha1;
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
// au.dump (cerr);
	    string nonce (au["nonce"]);
	    DigestAuthTag* dtag = DigestAuthTag::finddtag (nonce);
	    if (dtag == NULL) {	// not a validated nonce
		if (nonce.size() < 32) {
// cerr << "nonce=" << nonce << endl;
// cerr << "nonce is too short for time validation" << endl;
		    needauthentication = true;
		} else {
		    int64_t propt = hex2i64 (nonce, 16);
// cerr << "nonce=" << nonce << endl
//      << "    t=" << setbase(16) << setfill('0') << setw(16) << propt << setbase(10) << endl;
		    if ( time(NULL) - propt > expiration ) { // the unvalidated nonce is too old		    JDJDJDJD be consistent here with further allowance 
// cerr << "le nonce a expire si on peut dire ca... diff=" << time(NULL) - propt << " > " << expiration << endl;
			needauthentication = true;
		    } else {
			goodenoughforvalid = true;
		    }
		}
	    } else { // the nonce was a valid one ... JDJDJDJD maybe we should still check for time validation and make them expire !!
		goodenoughforvalid = true;
	    }

	    if (goodenoughforvalid) {

		// lets look for the proper user
		map<string,string>::const_iterator mi_user = users.find (au["username"]);
		if (mi_user != users.end()) {	// the user is in the db
		    string  a1 (mi_user->first);
			    a1 += ':';
			    // ha1s += au["realm"];
			    a1 += realm;
			    a1 += ":";
			    a1 += mi_user->second;	// pass
		    string  a2 (req.method);
			    a2 += ":";
			    // ha2s += req.req_uri;
			    a2 += au["uri"];		// we use the provided uri because some bad client give complete url instead of uri (android web-client !)
		    string ha1, ha2;
		    md5 (a1, ha1);
		    md5 (a2, ha2);
		    string digest;
		    md5 (ha1+':'+au["nonce"]+':'+au["nc"]+':'+au["cnonce"]+':'+au["qop"]+':'+ha2, digest);
// cerr << "source digest calculated : [" << 
//		        (ha1+':'+au["nonce"]+':'+au["nc"]+':'+au["cnonce"]+':'+au["qop"]+':'+ha2)
//		     << "]" << endl;
// cerr << "digest calculated : " << digest << endl;

		    if (au["response"] == digest) {
			needauthentication = false;
			if (dtag == NULL) {
			    dtag = new DigestAuthTag(nonce, expiration);
			    if (dtag == NULL) {
				cerr << "FMap_TreatRequest::DigestAuthTag could not create a DigestAuthTag" << endl;
				return error (cout, req, 500, "could not create a DigestAuthTag");
			    }
			} else {
			    a2 = "::";
			    a2 += req.req_uri;
			    md5 (a2, ha2);
			    md5 (mi_user->second+':'+au["nonce"]+':'+au["nc"]+':'+au["cnonce"]+':'+au["qop"]+':'+ha2, digest);
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
		"realm=\"";
	    www_authenticate += realm;
	    www_authenticate += "\", "
		"qop=\"auth\", "
		"opaque=\"";
	    append_i64 (www_authenticate, random64());
	    www_authenticate += "\", "
		"nonce=\"";
	    DigestAuthTag::gennonce (www_authenticate);
	    www_authenticate += "\", "
		"domain=\"";
	    www_authenticate += "/";
	    www_authenticate += '"';
	    
	    return error (cout, req, 401, "authentication required");
	}
	    
	return TRPending;
    }

    

} // namespace bulkrays
