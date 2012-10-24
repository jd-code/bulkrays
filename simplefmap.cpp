#include <sys/types.h>	    // stat opendir
#include <sys/stat.h>	    // stat
#include <unistd.h>	    // stat
#include <fcntl.h>	    // open

#include <string.h>	    // strerror
#include <errno.h>	    // errno
#include <limits.h>	    // realpath
#include <stdlib.h>	    // realpath

#include <dirent.h>	    // opendir
#include <unistd.h>	    // readdir_r pathconf

#include <sys/mman.h>	    // mmap

#include <bulkrays/bulkrays.h>

namespace simplefmap {
    using namespace std;
    using namespace bulkrays;

    // some goodies in order to quickly output a full date : cout << os_time_t (mytime_t_var) << ....

    struct ostream_time_t { time_t f_param; };

    inline ostream_time_t
    os_time_t(time_t param)
    { 
	ostream_time_t funct_struct; 
	funct_struct.f_param = param; 
	return funct_struct; 
    }

    template<typename _CharT, typename _Traits>
    inline basic_ostream<_CharT,_Traits>& 
    operator<<(basic_ostream<_CharT,_Traits>& out, ostream_time_t f_struct)
    { 
	time_t t = f_struct.f_param;
	struct tm tt;
	localtime_r (&t, &tt);

	return out << setfill('0')
	    << setw(4) << tt.tm_year+1900 << "/"
	    << setw(2) << tt.tm_mon+1 << "/"
	    << setw(2) << tt.tm_mday << " "
	    << setw(2) << tt.tm_hour << ":"
	    << setw(2) << tt.tm_min << ":"
	    << setw(2) << tt.tm_sec;
    }


//    ostream& operator<< (ostream& out, const time_t &t) {
//	struct tm tt;
//	localtime_r (&t, &tt);
//
//	return out << setfill('0')
//	    << setw(4) << tt.tm_year+1900 << "-"
//	    << setw(2) << tt.tm_mon+1 << "-"
//	    << setw(2) << tt.tm_mday << " "
//	    << setw(2) << tt.tm_hour << ":"
//	    << setw(2) << tt.tm_min << ":"
//	    << setw(2) << tt.tm_sec;
//    }

    class direntry {
	public:
	    string name;
	    bool isvalid;
	    off_t size;
	    time_t mtime, ctime;
	    bool isdir;
	    direntry (const string &curdir, struct dirent const& d) : name (d.d_name) {
		struct stat bstat;
		string fname(curdir);
		fname += "/";
		fname += name;
		if (stat (fname.c_str(), &bstat) != 0) {
		    isvalid = false;
		    size = 0, mtime = 0, ctime = 0;
		    isdir = false;
		    return;
		}
		isvalid = true;
		size = bstat.st_size;
		mtime = bstat.st_mtime;
		ctime = bstat.st_ctime;
		isdir = (d.d_type == DT_DIR);
	    }
    };

    ostream& operator<< (ostream& out, const direntry &de) {	// JDJDJDJD some css tagging is missing here
	out << "<a href=\"" << de.name;
	if (de.isdir)
	    out << '/';
	out << "\">" << de.name << "</a> " << (int)de.size << " " << os_time_t (de.mtime) << " " << os_time_t (de.ctime);
	return out;
    }


    class MMapBuffer : virtual public DummyBuffer {
	private:
	    int fd;
	public:
	    MMapBuffer (int fd, char * start, ssize_t length) : DummyBuffer (start, length), fd(fd) {}
	    ~MMapBuffer () {
		if (munmap (start, length) != 0) {
		    int e = errno;
		    cerr << "~MMapBuffer : munmap failed : " << e << " " << strerror(e) << endl;
		}
		close (fd);
	    }
    };

    class FMap_TreatRequest : virtual public TreatRequest {
	string rootdir;
	size_t rootdirlength;
      public:
	FMap_TreatRequest (string proposedrootdir) : TreatRequest(), rootdir(proposedrootdir) {
		if (rootdir.empty()) {
		    rootdir = "/";
		} else if (rootdir[rootdir.size()-1] != '/') {
		    rootdir += '/';
		}
		rootdirlength = rootdir.size();
	    }

	~FMap_TreatRequest () {}

	virtual int output (ostream &cout, HTTPRequest &req) {
	    if (req.method != "GET") {
		req.set_relative_expires (60);
		return error (cout, req, 405);	// JDJDJDJD The response MUST include an Allow header containing a list of valid methods for the requested resource. 
	    }

	    bool isrootdir = false;

	    string fname (rootdir);
	    if (req.document_uri.empty())
		req.document_uri = "/";
	    if (req.document_uri == "/")
		isrootdir = true;

// cerr << "req.document_uri = " << req.document_uri << endl;

	    fname += req.document_uri;
	    char * canonfname = realpath (fname.c_str(), NULL);
	    if (canonfname == NULL) {
		int e = errno;
		switch (e) {
		    case EACCES:
			req.set_relative_expires (60);
			return error (cout, req, 403, NULL, "file permission denied (001)");

		    case ENOENT:
			req.set_relative_expires (20);
			return error (cout, req, 404, NULL, "Not Found (001f)");

		    case ENOTDIR:
			req.set_relative_expires (20);
			return error (cout, req, 404, NULL, "Not Found (001d)");

		    case ENAMETOOLONG:
		    case EINVAL:
		    case EIO:
		    default:
			req.errlog() << "simplefmap::output realpath gave error " << e << " " << strerror (e) << endl;
			req.set_relative_expires (60);
			return error (cout, req, 500);
		}
	    }

	    if (!isrootdir) {
		if (strncmp (canonfname, rootdir.c_str(), rootdirlength) != 0) {
		    stringstream err;
		    err << "simplefmap : FMap_TreatRequest::output : file " << canonfname << " fells outside of rootdir : " << rootdir;
		    req.set_relative_expires (60);
		    return error (cout, req, 403, NULL, "file permission denied (001b)");
		}
	    }

	    struct stat statbuf;
	    if (lstat (canonfname, &statbuf) != 0) {
		int e = errno;
req.errlog() << "simplefmap::output lstat(" << canonfname << ") gave error " << e << " " << strerror (e) << endl;  // this is debug JDJDJDJD we should have an uniform way to log such things
                switch (e) {
		    case EACCES:
			req.set_relative_expires (60);
			return error (cout, req, 403, NULL, "file permission denied (002)");

		    case ENOENT:
			req.set_relative_expires (20);
			return error (cout, req, 404, NULL, "Not Found (002f)");

		    case ENOTDIR:
			req.set_relative_expires (20);
			return error (cout, req, 404, NULL, "Not found (002d)");

		    case ENAMETOOLONG:
		    case EINVAL:
		    case EIO:
		    default:
			req.errlog() << "simplefmap::output lstat(" << canonfname << ") gave error " << e << " " << strerror (e) << endl;
			req.set_relative_expires (60);
			return error (cout, req, 500);
		}
	    }
	    if (  !( ((S_IFREG & statbuf.st_mode) != 0) || ((S_IFDIR & statbuf.st_mode) != 0) )   )	{   // we didn't reach a file or a directory (maybe a link though, but we don't follow links)
		req.set_relative_expires (60);
		return error (cout, req, 403, NULL, "file permission denied (003)");
	    }

	    if ((S_IFDIR & statbuf.st_mode) != 0) {
		DIR * fdir = opendir (canonfname);

		if (fdir == NULL) {
		    int e = errno;
req.errlog() << "simplefmap::output opendir(" << canonfname << " ) gave error " << e << " " << strerror (e) << endl;  // this is debug JDJDJDJD we should have an uniform way to log such things
		    switch (e) {
			case EACCES:
			    req.set_relative_expires (60);
			    return error (cout, req, 403, NULL, "file permission denied (003b)");

			case ENOENT:
			    req.set_relative_expires (20);
			    return error (cout, req, 404, NULL, "Not Found (003)");

			default:
			    req.set_relative_expires (60);
			    return error (cout, req, 503, NULL, "weidries occured (001)");
		    }
		}

		size_t dirent_len = offsetof(struct dirent, d_name) + pathconf(canonfname, _PC_NAME_MAX) + 1;
		struct dirent *entryp = (struct dirent *) malloc(dirent_len);
		if (entryp == NULL) {
		    req.errlog() << "simplefmap::output could not alloc struct dirent * (size = " << (int)dirent_len << ")" << endl;
		    req.set_relative_expires (60);
		    return error (cout, req, 503, NULL, "weidries occured (002)");
		}

	list <direntry> thedir;

		while (true) {
		    struct dirent *presult;
		    if (readdir_r(fdir, entryp, &presult) != 0)
			break;

		    if (presult == NULL)
			break;

		    thedir.push_back (direntry(canonfname, *presult));

//		    struct dirent &entry = *presult;
//
//		    if (entry.d_type == DT_DIR) {
//			string name(entry.d_name);
//			name += '/';
//			thedir [name] = "";
//		    } else
//			thedir [entry.d_name] = "";

		}

		free (entryp);

		req.statuscode = 200;
		req.outmime["Content-Type"] = "text/html";

// silly test just to see about auth
// req.statuscode = 401;
// req.outmime["WWW-Authenticate"] = "Basic realm=\"WallyWorld\"";


		stringstream s;
		s << "<!DOCTYPE html " << endl
		  << "     PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"" << endl
		  << "     \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << endl
		  << "<html xmlns='http://www.w3.org/1999/xhtml'>" << endl;

		s << "<head><title>" << req.document_uri << "</title></head>" << endl;
		s << "<body>" << endl;
		s << "<h1>" << canonfname << "</h1>" << endl;
		s << "<h2>" << fname << "</h2>" << endl;
	{   list<direntry>::iterator li;
	    s << "<ul>" << endl;
	    for (li=thedir.begin() ; li!=thedir.end() ; li++) {
		// s << "<li><a href=\"" << mi->first << "\">" << mi->first << "</a></li>" << endl;
		s << "<li>" << *li << "</li>" << endl;
	    }
	    s << "</ul>" << endl;
	}
		s << "<div>" << endl;
		s << "<tt>" << endl;
		s << "<div>" << req.method << "</div>" << endl
		  << "<div>" << req.document_uri << "</div>" << endl
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

		req.set_contentlength (s.str().size());
		req.set_relative_expires (20);	    // JDJDJDJD this should be tunable ?
		req.publish_header();
		cout << s.str();
	    } else {
		int f = open (canonfname, O_RDONLY);
		if (f < 0) {
		    int e = errno;
		    req.errlog() << "simplefmap::output open(" << canonfname << ") failed : " << e << " " << strerror(e) << endl;

		    free (canonfname);
		    req.set_relative_expires (60);
		    return error (cout, req, 403, NULL, "file permission denied (004)");
		}

		char * fmap = (char *) mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, f, 0);
		if (fmap == MAP_FAILED) {
		    int e = errno;
		    req.errlog() << "simplefmap::output mmap(" << canonfname << ") failed : " << e << " " << strerror(e) << endl;

		    close (f);
		    free (canonfname);
		    req.set_relative_expires (60);
		    return error (cout, req, 403, NULL, "file permission denied (005)");
		}

		string mime_type;
		size_t pdot = req.document_uri.rfind('.');
		if (pdot == string::npos) {
		    mime_type = "text/plain";
		} else {
		    string terminaison = req.document_uri.substr(pdot + 1);
		    const string * pmt = mimetypes.getmimefromterminaison (terminaison);
		    if (pmt == NULL)
			mime_type = "text/plain";
		    else
			mime_type = *pmt;
		}

		req.statuscode = 200;
		req.outmime["Content-Type"] = mime_type;
		req.set_contentlength (statbuf.st_size);
		req.set_relative_expires (60);	// JDJDJDJD this should be tuneable, with mime type ?
		req.publish_header();


		MMapBuffer* mmbuf = new MMapBuffer (f, fmap, statbuf.st_size);
		if (mmbuf == NULL) {
		    req.errlog() << "could not allocate MMapBuffer" << endl;
		    munmap (fmap, statbuf.st_size);
		    close (f);
		}
		req.pdummyconnection->pushdummybuffer (mmbuf);

	    }
	    free (canonfname);

	    return 0;
	}
    };

    class fmap_urimapper : virtual public URIMapper {
	public:
	    FMap_TreatRequest fmap_treatrequest;

	    fmap_urimapper(string rootdir) : URIMapper(), fmap_treatrequest(rootdir) {}
	    ~fmap_urimapper() {}
	    virtual TreatRequest* treatrequest (HTTPRequest &req) {
		return &fmap_treatrequest;
	    }
    };

    int simplefmap_global (void) {
	fmap_urimapper *mapper = new fmap_urimapper("/home/");
//	fmap_urimapper *mapper = new fmap_urimapper("/BIG/home/webs/hashttpp.zz/html");
	if (mapper == NULL) {
	    cerr << "bulkrays::testsite_global could not allocate fmap_urimapper" << endl;
	    return -1;
	}
	hostmapper["hashttpp.zz"] = mapper;
	return 0;
    }
}
