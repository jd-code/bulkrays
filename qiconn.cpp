#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h> /* clok times */

#include <iomanip>

#include "qiconn.h"

namespace qiconn
{
    using namespace std;

    /*
     *  ---------------------------- simple ostream operators for hostent and sockaddr_in --------------------
     */

    ostream& operator<< (ostream& out, const struct hostent &he) {
	cout << he.h_name << " :" << endl;
	int i=0;
	if (he.h_addrtype == AF_INET) {
	    while (he.h_addr_list[i] != NULL) {
		out << (int)((unsigned char)he.h_addr_list[i][0]) << "."
		    << (int)((unsigned char)he.h_addr_list[i][1]) << "."
		    << (int)((unsigned char)he.h_addr_list[i][2]) << "."
		    << (int)((unsigned char)he.h_addr_list[i][3]) << endl;
		i++;
	    }
	} else {
	    cout << " h_addrtype not known" << endl ;
	}
	return out;
    }

    ostream& operator<< (ostream& out, struct sockaddr_in const &a) {
	const unsigned char* p = (const unsigned char*) &a.sin_addr;
	out << (int)p[0] << '.' << (int)p[1] << '.' << (int)p[2] << '.' << (int)p[3];
	return out;
    }

    /*
     *  ---------------------------- server_pool : opens a socket for listing a port at a whole --------------
     */

    #define MAX_QUEUDED_CONNECTIONS 5

    int server_pool (int port, const char *addr /* = NULL */, int type /*= AF_INET*/) {
	struct sockaddr_in serv_addr;

	memset (&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons (port);
	if (addr == NULL) {
	    serv_addr.sin_addr.s_addr = INADDR_ANY;
	} else {
	    if (inet_aton(addr, &serv_addr.sin_addr) == (int)INADDR_NONE) {
		int e = errno;
		cerr << "gethostbyaddr (" << addr << " failed : " << strerror (e) << endl;
		return -1;
	    }
	}

	int s;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
	    int e = errno;
	    cerr << "could not create socket (for listenning connections :" << port << ") : " << strerror (e) << endl ;
	    return -1;
	}

	{	int yes = 1;
	    if (setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) != 0) {
		int e = errno;
		cerr << "could not setsockopt (for listenning connections :" << port << ") : " << strerror (e) << endl ;
		return -1;
	    }
	}

	if (bind (s, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) != 0) {
	    int e = errno;
	    cerr << "could not bind socket (for listenning connections :" << port << ") : " << strerror (e) << endl ;
	    return -1;
	}
	if (listen (s, MAX_QUEUDED_CONNECTIONS) != 0) {
	    int e = errno;
	    cerr << "could not listen socket (for listenning connections :" << port << ") : " << strerror (e) << endl ;
	    return -1;
	}
	return s;
    }



    /*
     *  ---------------------------- init_connect : makes a telnet over socket, yes yes ----------------------
     */

    // JDJDJDJD a better thing should be to create an intermediate state after needtoconnect
    // like needtoconnectconfirm in order to wait for writable state (?) and poll for connection establishment

    // init_connect open a tcp session from fqdn and port
    // return a connected socket
    // may fail with -1 on a straightforward fail   (connection refused)
    // may fail with -2 on a slow timed-out failure (port filtered, packet dropped, unreachable machine)
    


    int init_connect (const char *fqdn, int port, struct sockaddr_in *ps /* = NULL */ ) {
	struct hostent * he;
	if (debug_connect) cerr << "init_connect -> gethostbyname (" << fqdn << ")" << endl;
	he = gethostbyname (fqdn);
	if (he != NULL) {
	    if (debug_resolver) cout << "gethostbyname(" << fqdn << ") = " << *he;
	} else
	    return -1;		    // A logger et détailler JDJDJDJD

    //    struct servent *se = getservbyport (port, "tcp");
	
	struct sockaddr_in sin;
	bzero ((char *)&sin, sizeof(sin));

	sin.sin_family = AF_INET;
	bcopy (he->h_addr_list[0], (char *)&sin.sin_addr, he->h_length);
	sin.sin_port = htons(port);

	if (ps != NULL)
	    *ps = sin;
	
	struct protoent *pe = getprotobyname ("tcp");
	if (pe == NULL) {
	    int e = errno;
	    cerr << "could not get protocol entry tcp : " << strerror (e) << endl ;
	    return -1;
	}

	if (debug_connect) cerr << "init_connect -> socket (PF_INET, SOCK_STREAM, tcp)" << endl;
	int s = socket(PF_INET, SOCK_STREAM, pe->p_proto);
	if (s == -1) {
	    int e = errno;
	    cerr << "could not create socket (for connection to " << fqdn << ":" << port << ") : " << strerror (e) << endl ;
	    return -1;
	}

	long s_flags = 0;
	if (fcntl (s, F_GETFL, s_flags) == -1) {
	    int e = errno;
	    cerr << "could not get socket flags (for connection to " << fqdn << ":" << port << ") : " << strerror (e) << endl ;
	    close (s);
	    return -1;
	}

	s_flags |= O_NONBLOCK;
	if (fcntl (s, F_SETFL, s_flags)  == -1) {
	    int e = errno;
	    cerr << "could not set socket flags with O_NONBLOCK (for connection to " << fqdn << ":" << port << ") : " << strerror (e) << endl ;
	    close (s);
	    return -1;
	}
	
	if (debug_connect) cerr << "init_connect -> socket (PF_INET, SOCK_STREAM, tcp)" << endl;

//PROFILECONNECT    timeval startc, clok;
//PROFILECONNECT    gettimeofday (&startc, NULL);

	int r = connect (s, (const struct sockaddr *)&sin, sizeof(sin));

//PROFILECONNECT    gettimeofday (&clok, NULL);
	if (r < 0) {
//PROFILECONNECT    cerr << "first connect fail connect=" << r
//PROFILECONNECT         << " took[" << ((clok.tv_sec - startc.tv_sec) * 10000 + (clok.tv_usec - startc.tv_usec)/100) << "]" << endl;
	    int e = errno;
	    if (e == 115 /* EINPROGRESS */) {
		time_t start = time (NULL);
		bool connected =false;
		do { 
		    struct timeval tv;
		    fd_set myset;
		    tv.tv_sec = 0; 
		    tv.tv_usec = 10000;
		    FD_ZERO(&myset); 
		    FD_SET(s, &myset); 
		    if (debug_connect) cerr << "init_connect -> select ()" << endl;
		    int res = select(s+1, NULL, &myset, NULL, &tv); 
//PROFILECONNECT    gettimeofday (&clok, NULL);
//PROFILECONNECT    cerr << "select took[" << ((clok.tv_sec - startc.tv_sec) * 10000 + (clok.tv_usec - startc.tv_usec)/100) << "]" << endl;
		    if ((res<0) && (errno != EINTR)) { 
			int e = errno;
			cerr << "while selecting after connect attempt (for connection to "
			     << fqdn << ":" << port << ") got : "
			     << strerror (e) << " still attempting..." << endl ;
		    } 
		    else if (res > 0) { 
			// Socket selected for write 
			int valopt;
			socklen_t lon = sizeof(valopt); 
			if (debug_connect) cerr << "init_connect -> getsockopt (SOL_SOCKET, SO_ERROR)" << endl;
			if (getsockopt(s, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
			    int e = errno;
			    cerr << "while selecting after connect attempt for (for connection to "
				 << fqdn << ":" << port << ") getsockopt got "
				 << strerror (e) << endl;
//PROFILECONNECT    gettimeofday (&clok, NULL);
//PROFILECONNECT    cerr << "getsockopt (SOL_SOCKET, SO_ERROR) took[" << ((clok.tv_sec - startc.tv_sec) * 10000 + (clok.tv_usec - startc.tv_usec)/100) << "]" << endl;
			    close (s);
			    return -1;
			} 

//PROFILECONNECT    gettimeofday (&clok, NULL);
//PROFILECONNECT    cerr << "getsockopt (SOL_SOCKET, SO_ERROR) took[" << ((clok.tv_sec - startc.tv_sec) * 10000 + (clok.tv_usec - startc.tv_usec)/100) << "]" << endl;
			// Check the value returned... 
			if (valopt) { 
			    cerr << "while selecting after connect attempt for (for connection to "
				 << fqdn << ":" << port << ") getsockopt returned "
				 << strerror(valopt) << endl;
			    close (s);
			    return -1;
			} else {
			    connected = true;
			    break; 
			}
		    }
		} while ((time(NULL) - start) < 2);	// JDJDJDJD ceci est une valeur arbitraire de 2 secondes !
		if (connected) {
//PROFILECONNECT    gettimeofday (&clok, NULL);
//PROFILECONNECT    cerr << "connected finally took[" << ((clok.tv_sec - startc.tv_sec) * 10000 + (clok.tv_usec - startc.tv_usec)/100) << "]" << endl;
		    return s;
		} else {
//PROFILECONNECT    gettimeofday (&clok, NULL);
//PROFILECONNECT    cerr << "not connected timeout took[" << ((clok.tv_sec - startc.tv_sec) * 10000 + (clok.tv_usec - startc.tv_usec)/100) << "]" << endl;
		    cerr << "selecting after connect attempt for (for connection to "
			 << fqdn << ":" << port << ") timed out" << endl;
		    close (s);
		    return -2;
		} 
	    } else if (e != 0) {
//PROFILECONNECT    gettimeofday (&clok, NULL);
//PROFILECONNECT    cerr << "not connected straigh error took[" << ((clok.tv_sec - startc.tv_sec) * 10000 + (clok.tv_usec - startc.tv_usec)/100) << "]" << endl;
		cerr << "could not connect to " << fqdn << ":" << port << " : " << strerror (e) << " errno=" << e << endl ;
		close (s);
		return -1;
	    }
	}
	return s;
    }

    /*
     *  ---------------------------- Connection : handles an incoming connection from fd --------------------
     */

    /*
     *	virtual class in order to pool and watch several fds with select later on.
     *	this skeleton provides
     *	    building from a file descriptor and an incoming internet socket address
     *	    building from a file descriptor only (partly broken)
     *	    read is called whene theres some reading found by select
     *	    write is called when the fd requested to watch for write status and is write-ready
     */

    void Connection::close (void) {
	deregister_from_pool ();
	if (fd >= 0) {
	    if (::close(fd) != 0) {
		int e = errno;
		cerr << "error closing fd[" << fd << "] : " << strerror(e) << endl ;
	    }
	}
    }

    void Connection::register_into_pool (ConnectionPool *cp) {
	if (cp != NULL) {
	    Connection::cp = cp;
	    cp->push (this);
	} else if (Connection::cp != cp)
	    cerr << "error: connection [" << getname() << "] already registered to another cp" << endl;
    }

    void Connection::deregister_from_pool () {
	if (cp != NULL) {
	    cp->pull (this);
	    cp = NULL;
	}
    }

    Connection::~Connection (void) {
	if (debug_fddestr) cerr << "destruction of fd[" << fd << "] (deregistering)" << endl ;
	deregister_from_pool ();
	close ();
    }

    void Connection::schedule_for_destruction (void) {
	if (cp != NULL)
	    cp->schedule_for_destruction (this);
	else
	    cerr << "warning : unable to register fd[" << getname() << "]for destrucion becasue cp=NULL" << endl;
    }


    SocketConnection::SocketConnection (int fd, struct sockaddr_in const &client_addr)
	: Connection (fd)
    {
	setname (client_addr);
    }

    void SocketConnection::setname (struct sockaddr_in const &client_addr) {
	SocketConnection::client_addr = client_addr;
	stringstream s;
	s << client_addr;
	name = s.str();
    }

    /*
     *  ---------------------------- select pooling : need to be all in one class --------- JDJDJDJD ---------
     */

    /*
     *  ---------------------------- rought signal treatments ------------------------------------------------
     */

    int caught_signal;

    void signal_handler (int sig) {
	printf ("got signal %d\n", sig);
	caught_signal ++;
	if ((sig > 0) && (sig<254))
	    pend_signals[sig]++;
	else
	    pend_signals[255]++;
    }

    int ConnectionPool::add_signal_handler (int sig) {
	if (SIG_ERR == signal(sig, signal_handler)) {
	    int e = errno;
//	    if (e != 22) {
		cerr << "could not set signal handler for sig=" << sig << " : [" << e << "]" << strerror (e) << endl;
//	    }
	    return 1;
	} else {
	    cerr << "signal [" << sig << "] handled." << endl;
	    return 0;
	}
    }

    int ConnectionPool::init_signal (void) {
	int i;
	int err = 0;
	for (i=0 ; i<256 ; i++)
	    pend_signals[i] = 0;
	caught_signal = 0;

	for (i=0 ; i<255 ; i++) {
	  if (i == 13)
	      err += add_signal_handler (i);
	}
	return err;
    }

    void ConnectionPool::treat_signal (void) {
	int i;
	for (i=0 ; i<256 ; i++) {
	    if (pend_signals [i] != 0) {
		cerr << "got sig[" << i << "] " << pend_signals [i] << " time" << ((i==1) ? "" : "s") << "." << endl;
		pend_signals [i] = 0;
	    }
	}
	caught_signal = 0;
    }

    ConnectionPool::ConnectionPool (void) {
	biggest_fd = 0;
	exitselect = false;
	scheddest = false;
    }


    void ConnectionPool::build_r_fd (void) {
	MConnections::iterator mi;
	FD_ZERO (&r_fd);
	for (mi=connections.begin() ; mi!=connections.end() ; mi++)
	    if (mi->first >= 0) FD_SET (mi->first, &r_fd);
    }

    int ConnectionPool::set_biggest (void) {
	if (connections.empty())
	    biggest_fd = 0;
	else {
	    biggest_fd = connections.begin()->first + 1;
	    if (biggest_fd < 0)
		biggest_fd = 0;
	}
	return biggest_fd;
    }

    void ConnectionPool::schedule_for_destruction (Connection * c) {
	if (connections.find (c->fd) == connections.end()) {
	    cerr << "warning: we were asked for destroying some unregistered connection[" << c->getname() << "]" << endl;
	    return;
	}
	destroy_schedule.push_back(c);
	scheddest = true;
    }

    int ConnectionPool::select_poll (struct timeval *timeout) {
	if (scheddest) {
	    list<Connection*>::iterator li;
	    for (li=destroy_schedule.begin() ; li!=destroy_schedule.end() ; li++)
		delete (*li);
	    destroy_schedule.erase (destroy_schedule.begin(), destroy_schedule.end());
	    scheddest = false;
	}

	MConnections::iterator mi;
	for (mi=connections.begin() ; mi!=connections.end() ; mi++)
	    mi->second->poll ();
	
	fd_set cr_fd = r_fd,
	       cw_fd = w_fd;
	select (biggest_fd, &cr_fd, &cw_fd, NULL, timeout);

	FD_ZERO (&w_fd);

	if (caught_signal)
	    treat_signal ();
	else {
	    int i;
	    for (i=0 ; i<biggest_fd ; i++) {
		// we test both, because there is a suspicion of
		// erased connexions between cr_fd build and now !
		//		alternative:    if (connections.find(i) != connections.end())
		if ((FD_ISSET(i, &r_fd)) && (FD_ISSET(i, &cr_fd))) {
			connections[i]->read();
		}
	    }
	    for (i=0 ; i<biggest_fd ; i++) {
		// the same should be done here same here JDJDJD
		if (FD_ISSET(i, &cw_fd))
		    connections[i]->write();
	    }
	}
	if (exitselect)
	    return 1;
	else
	    return 0;
    }

    int ConnectionPool::select_loop (const struct timeval & timeout) {
	struct timeval t_out = timeout;
	build_r_fd ();
	FD_ZERO (&w_fd);

	while (select_poll(&t_out) == 0)  {
	    t_out = timeout;
	}
	return 0;
    }

    void ConnectionPool::closeall (void) {
	MConnections::reverse_iterator rmi;
	cerr << "entering ConnectionPool::closeall..." << endl;
	for (rmi=connections.rbegin() ; rmi!=connections.rend() ; rmi++) {
	    rmi->second->close();
	}
	cerr << "...done." << endl;
    }

    void ConnectionPool::destroyall (void) {
	MConnections::reverse_iterator rmi;
	cerr << "entering ConnectionPool::destroyall..." << endl;
	for (rmi=connections.rbegin() ; rmi!=connections.rend() ; rmi++) {
	    rmi->second->schedule_for_destruction();
	}
	cerr << "...done." << endl;
    }

    ConnectionPool::~ConnectionPool (void) {
	closeall ();
    }

    void ConnectionPool::push (Connection *c) {
	if (c->cp == NULL) {
	    c->cp = this;
	} else if (c->cp != this) {
	    cerr << "warning: connection[" << c->fd << ":" << c->getname() << "] already commited to another cp !" << endl;
	    return;
	}
	if (c->fd == -1) {  // we're setting a pending connection...
// cerr << "ici" << endl;
	    if (connections.empty()) {
// cerr << "set -2 because empty" << endl;
		c->fd = -2;
	    } else {
		c->fd = connections.rbegin()->first - 1;    // MConnections is reverse-ordered !! (and needs to be)
// cerr << "not empty so calculuus gives :" << c->fd << endl;
// {   MConnections::iterator mi;
//     for (mi=connections.begin() ; mi!=connections.end() ; mi++)
// 	cerr << "    fd[" << mi->first << "] used" << endl;
// }
		if (c->fd >= 0)
		    c->fd = -2;
// cerr << "not empty final value :" << c->fd << endl;
	    }
	}
	MConnections::iterator mi = connections.find (c->fd);
	if (mi == connections.end()) {
	    connections[c->fd] = c;
	    set_biggest ();
	    build_r_fd ();
	    reqw (c->fd);	/* we ask straightforwardly for write for welcome message */
	} else
	    cerr << "warning: connection[" << c->getname() << ", fd=" << c->fd << "] was already in pool ???" << endl;
    }

    void ConnectionPool::pull (Connection *c) {
	MConnections::iterator mi = connections.find (c->fd);
	if (mi != connections.end()) {
	    connections.erase (mi);
	    set_biggest ();
	    build_r_fd ();
	} else {
	    cerr << "warning: ConnectionPool::pull : cannot pull {" << c->getname() << "} !" << endl;
	}
    }


    /*
     *  ---------------------------- DummyConnection : connection that simply echo in hex what you just typed 
     */

    #define BUFLEN 1024
    DummyConnection::~DummyConnection (void) {
	ldums.erase (me);
    };

    DummyConnection::DummyConnection (int fd, struct sockaddr_in const &client_addr) : SocketConnection (fd, client_addr) {
	raw = false;
	pdummybuffer = NULL;
	givenbuffer = false;
	givenbufferiswaiting = false;
	destroyatendofwrite = false;
	out = new stringstream ();
	wpos = 0;
	ldums.push_back (this);
	me = ldums.end();
	me--;
    }

    void DummyConnection::setrawmode (void) {
	raw = true;
    }

    void DummyConnection::setlinemode (void) {
	raw = false;
    }

    void DummyConnection::read (void) {
	char s[BUFLEN];
	ssize_t n = ::read (fd, (void *)s, BUFLEN);
	
	if (debug_transmit) {
	    int i;
	    clog << "fd=" << fd << "; ";
	    for (i=0 ; i<n ; i++)
		clog << s[i];
	    clog << endl;
	}
if (debug_dummyin)
{   int i;
    cerr << "DummyConnection::read got[";
    for (i=0 ; i<n ; i++)
	cerr << s[i];
    cerr << endl;
}
	int i;
	for (i=0 ; i<n ; i++) {
	    if (!raw) {
		if ((s[i]==10) || (s[i]==13) || s[i]==0) {
		    if (i+1<n) {
			if ( ((s[i]==10) && (s[i+1]==13)) || ((s[i]==13) && (s[i+1]==10)) )
			    i++;
		    }
if (debug_lineread) {
    cerr << "DummyConnection::read->lineread(" << bufin << ")" << endl;
}
		    lineread ();
		    bufin = "";
		} else
		    bufin += s[i];
	    } else {
		bufin += s[i];
	    }
	}
	if (raw) {
if (debug_lineread) {
    cerr << "DummyConnection::read->lineread(" << bufin << ")" << endl;
}
	    lineread ();
	    bufin = "";
	}
	if (n==0) {
	    cerr << "read() returned 0. we may close the fd[" << fd << "] ????" << endl;
	    reconnect_hook();
	}
    }

    void DummyConnection::reconnect_hook (void) {
	    schedule_for_destruction();
    }

    void DummyConnection::lineread (void) {
	string::iterator si;
	for (si=bufin.begin() ; si!=bufin.end() ; si++) {
	    (*out) << setw(2) << setbase(16) << setfill('0') << (int)(*si) << ':' ;
	}
	(*out) << " | " << bufin << endl;
	flush ();

	if (bufin.find("shut") == 0) {
	    if (cp != NULL) {
		cerr << "shutting down on request from fd[" << getname() << "]" << endl;
		cp->tikkle();
		// cp->closeall();
		// exit (0);
	    }
	    else
		cerr << "could not shut down from fd[" << getname() << "] : no cp registered" << endl;
	}

	if (bufin.find("wall ") == 0) {
	    list<DummyConnection *>::iterator li;
	    int i = 0;
	    for (li=ldums.begin() ; li!=ldums.end() ; li++) {
		(*(*li)->out) << i << " : " << bufin.substr(5) << endl;
		(*li)->flush();
		i++;
	    }
	}
    }

    void DummyConnection::flush(void) {
	if (cp != NULL)
	    cp->reqw (fd);
	bufout += out->str();
if (debug_dummyout) {
    cerr << "                                                                                      ->out=" << out->str() << endl ;
}
	delete (out); 
	out = new stringstream ();
    }

    void DummyConnection::flushandclose(void) {
	destroyatendofwrite = true;
	flush();
    }

    int DummyConnection::pushdummybuffer (DummyBuffer* pdb) {
	if (givenbufferiswaiting || givenbuffer) {
	    cerr << "DummyConnection::pushdummybuffer : silly attempt to push a buffer onto another" << endl;
	    cerr << "     givenbufferiswaiting = " << (givenbufferiswaiting ? "true" : "false") << endl;
	    cerr << "     givenbuffer = " << (givenbuffer ? "true" : "false") << endl;
	    // JDJDJDJD ca devrait pouvoir ce faire pourtant !
	    return -1;
	}
	givenbufferiswaiting = true;
	pdummybuffer = pdb;
//	if (cp != NULL)
//	    cp->reqw (fd);
	return 0;
    }

    void DummyConnection::write (void) {
	if (givenbuffer) {
	    ssize_t size = pdummybuffer->length - wpos,
		   nb;
	    if (size == 0) {
		givenbuffer = false;
		if (!bufout.empty()) {	// JDJDJDJD UGGLY !
		    if (cp != NULL)
			cp->reqw (fd);
		}
		delete (pdummybuffer);
		if (bufout.empty() && destroyatendofwrite)
		    schedule_for_destruction ();
		return;
	    }

	    if (size > BUFLEN) {
		if (cp != NULL)
		    cp->reqw (fd);
		nb = BUFLEN;
	    } else {
		nb = size;
	    }
	    ssize_t nt = ::write (fd, pdummybuffer->start+wpos, nb);
	    if (nt != -1) {
		wpos += nt;
		if (nt != nb) {
		    cerr << "some pending chars" << endl;
		    if (cp != NULL)
			cp->reqw (fd);
		}
	    } else {
		int e = errno;

		if (e == EPIPE) {   /* we can assume the connection is shut (and wasn't detected by read) */
		    reconnect_hook();
		} else {
		    cerr << "error writing via givenbuffer to fd[" << fd << ":" << getname() << "] : (" << e << ") " << strerror (e) << endl ;
		}
	    }
	    if (wpos == (size_t)pdummybuffer->length) {
		wpos = 0;

		givenbuffer = false;
		if (!bufout.empty()) {	// JDJDJDJD UGGLY !
		    if (cp != NULL)
			cp->reqw (fd);
		}
		delete (pdummybuffer);
		if (bufout.empty() && destroyatendofwrite)
		    schedule_for_destruction ();
		return;

	    }
// =================================================================================
	} else {
	    ssize_t size = (ssize_t)bufout.size() - wpos,
		   nb;
	    if (size == 0) {
		if (givenbufferiswaiting) {
		    givenbuffer = true;
		    givenbufferiswaiting = false;
		    if (cp != NULL)
			cp->reqw (fd);
		} else if (destroyatendofwrite) {
		    schedule_for_destruction ();
		}
		return;
	    }

	    if (size > BUFLEN) {
		if (cp != NULL)
		    cp->reqw (fd);
		nb = BUFLEN;
	    } else {
		nb = size;
	    }
	    ssize_t nt = ::write (fd, bufout.c_str()+wpos, nb);
	    if (nt != -1) {
		wpos += nt;
		if (nt != nb) {
		    cerr << "some pending chars" << endl;
		    if (cp != NULL)
			cp->reqw (fd);
		}
	    } else {
		int e = errno;

		if (e == EPIPE) {   /* we can assume the connection is shut (and wasn't detected by read) */
		    reconnect_hook();
		} else {
		    cerr << "error writing to fd[" << fd << ":" << getname() << "] : (" << e << ") " << strerror (e) << endl ;
		}
	    }
	    if (wpos == bufout.size()) {
		wpos = 0;
		bufout = "";
		if (givenbufferiswaiting) {
		    givenbuffer = true;
		    givenbufferiswaiting = false;
		    if (cp != NULL)
			cp->reqw (fd);
		} else if (destroyatendofwrite) {
		    schedule_for_destruction ();
		}
	    }
	}
    }

    /*
     *  ---------------------------- ListeningSocket : the fd that watches incoming cnx ----------------------
     */


    int ListeningSocket::addconnect (int socket) {
	struct sockaddr_in client_addr;
	socklen_t size_addr = sizeof(client_addr);
	int f = accept ( socket, (struct sockaddr *) &client_addr, &size_addr );
	if (f < 0) {
	    int e = errno;
	    cerr << "could not accept connection : " << strerror (e) << endl ;
	    return -1;
	}
	cerr << "new connection from fd[" << f << ":" << client_addr << "]" << endl;
	if (cp != NULL) {
	    DummyConnection * pdc = connection_binder (f, client_addr);
	    if (pdc == NULL) {
		cerr << "error: could not add connection : failed to allocate DummyConnection" << endl;
		return -1;
	    }
	    cp->push (pdc);
	} else
	    cerr << "error: could not add connection : no cp registered yet !" << endl;
	// connections[fd] = new DummyConnection (fd, client_addr);
	// cout << "now on, biggest_fd = " << set_biggest () << " / " << connections.size() << endl;
	// build_r_fd ();
	return 0;
    }

    ListeningSocket::~ListeningSocket (void) {}

    DummyConnection * ListeningSocket::connection_binder (int fd, struct sockaddr_in const &client_addr) {
	return new DummyConnection (fd, client_addr);
    }
    
    void ListeningSocket::setname (const string & name) {
	ListeningSocket::name = name;
    }

    ListeningSocket::ListeningSocket (int fd) : Connection(fd) {}

    void ListeningSocket::read (void) {
	addconnect (fd);
    }

    ListeningSocket::ListeningSocket (int fd, const string & name) : Connection(fd) {
	setname (name);
    }

    string ListeningSocket::getname (void) {
	return name;
    }

    void ListeningSocket::write (void) {}

    /*
     *  ---------------------------- all-purpose string (and more) utilities ---------------------------------
     */

    bool getstring (istream & cin, string &s, size_t maxsize /* = 2048 */) {
	char c;
	size_t n = 0;
	while ((n < maxsize) && cin.get(c) && (c != 10) && (c != 13))
	    s += c, n++;
	return (cin);
    }

    size_t seekspace (const string &s, size_t p /* = 0 */) {
	if (p == string::npos)
	    return string::npos;

	size_t l = s.size();
	
	if (p >= l)
	    return string::npos;

	while ((p < l) && isspace (s[p])) p++;

	if (p >= l)
	    return string::npos;

	return p;
    }

    size_t getidentifier (const string &s, string &ident, size_t p /* = 0 */ ) {
	size_t l = s.size();
	ident = "";

	p = seekspace(s, p);
	
	if (p == string::npos)
	    return string::npos;
	
	if (p >= l)
	    return string::npos;

	if (! isalpha(s[p]))
	    return p;

	while (p < l) {
	    if (isalnum (s[p]) || (s[p]=='_') || (s[p]=='-')) {
		ident += s[p];
		p++;
	    } else
		break;
	}

	if (p >= l)
	    return string::npos;

	return p;
    }

    size_t getfqdn (const string &s, string &ident, size_t p /* = 0 */ ) {
	size_t l = s.size();
	ident = "";

	p = seekspace(s, p);
	
	if (p == string::npos)
	    return string::npos;
	
	if (p >= l)
	    return string::npos;

	if (! isalnum(s[p]))
	    return p;

	while (p < l) {
	    if (isalnum (s[p]) || (s[p]=='.') || (s[p]=='-')) {
		ident += s[p];
		p++;
	    } else
		break;
	}

	if (p >= l)
	    return string::npos;

	return p;
    }

    size_t getinteger (const string &s, long long &n, size_t p /* = 0 */ ) {
	size_t l = s.size();
	string buf;

	p = seekspace(s, p);
	
	if (p == string::npos)
	    return string::npos;
	
	if (p >= l)
	    return string::npos;

	if (! isdigit(s[p]))
	    return p;

	while (p < l) {
	    if (isdigit (s[p])) {
		buf += s[p];
		p++;
	    } else
		break;
	}

	n = atoll (buf.c_str());
	
	if (p >= l)
	    return string::npos;

	return p;
    }

    size_t getinteger (const string &s, long &n, size_t p /* = 0 */ ) {
	size_t l = s.size();
	string buf;

	p = seekspace(s, p);
	
	if (p == string::npos)
	    return string::npos;
	
	if (p >= l)
	    return string::npos;

	if (! isdigit(s[p]))
	    return p;

	while (p < l) {
	    if (isdigit (s[p])) {
		buf += s[p];
		p++;
	    } else
		break;
	}

	n = atol (buf.c_str());
	
	if (p >= l)
	    return string::npos;

	return p;
    }

    CharPP::CharPP (string const & s) {
	isgood = false;
	size_t size = s.size();
	size_t p;
	n = 0;
	char *buf = (char *) malloc (size+1);
	if (buf == NULL)
	    return;
	
	list <char *> lp;
	lp.push_back (buf);
	for (p=0 ; p<size ; p++) {
	    if (s[p] == 0)
		lp.push_back (buf + p + 1);
	    buf[p] = s[p];
	}
	n = lp.size() - 1;
	charpp = (char **) malloc ((n+1) * sizeof (char *));
	if (charpp == NULL) {
	    delete (buf);
	    n = 0;
	    return;
	}
	list <char *>::iterator li;
	int i;
	for (li=lp.begin(), i=0 ; (li!=lp.end()) && (i<n) ; li++, i++)
	    charpp[i] = *li;
	charpp[i] = NULL;
	isgood = true;
    }
    char ** CharPP::get_charpp (void) {
	if (isgood)
	    return charpp;
	else
	    return NULL;
    }
    size_t CharPP::size (void) {
	return n;
    }
    CharPP::~CharPP (void) {
	if (charpp != NULL) {
	    if (n>0)
		delete (charpp[0]);
	    delete (charpp);
	}
    }
    ostream& CharPP::dump (ostream& cout) const {
	int i = 0;
	while (charpp[i] != NULL)
	    cout << "    " << charpp[i++] << endl;
	return cout;
    }

    ostream & operator<< (ostream& cout, CharPP const & chpp) {
	return chpp.dump (cout);
    }
    

    ostream & operator<< (ostream& cout, ostreamMap const &m ) {
	if (m.m.empty()) {
	    cout << m.name << "[]={empty}" << endl;
	    return cout;
	}
	map<string,string>::const_iterator mi;
	for (mi=m.m.begin() ; mi!=m.m.end() ; mi++)
	    cout << m.name << "[" << mi->first << "]=\"" << mi->second << '"' << endl;
	
	return cout;
    }
} // namespace qiconn

