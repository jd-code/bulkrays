
#include <errno.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

bool debug = false;


bool atbeginingosstring = true;

void escape (ostream & out, char c) {
    switch (c) {
	case 10:
	case 13:
		    if (!atbeginingosstring)
			out << "\\015\\012\"";
		    out << c;
		    atbeginingosstring = true;
		    break;

	default:
	    if (atbeginingosstring) {
		out << '"';
		atbeginingosstring = false;
	    }
	    switch (c) {
		case '"':   out << "\\\"";  break;
		case '\\':  out << "\\\\";  break;
		default:    out << c;	    break;
	    }
    }
}

bool parse (istream &in, ostream &out) {
    string  matchbegin ("{{"),
	    matchend ("}}");
#define     MATCHLEN 2
    string const * match = &matchbegin;
    bool incpp = true;
    char c;
    size_t pos = 0;

    while (in && in.get(c)) {
	if (c == (*match)[pos]) {
	    pos ++;
	    if (pos == MATCHLEN) {
		if (incpp) {
		    incpp = false;
		    match = &matchend;
		    atbeginingosstring = true;
		    out << " ";
		} else {
		    incpp = true;
		    match = &matchbegin;
		    if (!atbeginingosstring)
			out << '"';
		    out << " ";
		}
		pos = 0;
	    }
	} else {
	    if (pos != 0) {
		for (size_t i=0 ; i<pos ; i++) {
		    if (incpp)
			out << (*match)[i];
		    else
			escape(out, (*match)[i]);
		}
		pos = 0;
	    }
	    if (incpp)
		out << c;
	    else
		escape(out, c);
	}
    }
    return true;
}

int main (int nb, char ** cmde) {

    string outputfname;

    int i;
    for (i=1 ; i<nb ; i++) {	// let's seek for the output name
	if (strncmp (cmde[i], "-o", 2) == 0) {
	    if (cmde[i][2] != 0) {
		outputfname += (cmde[i] + 2);
	    } else if (i+1 < nb) {
		outputfname += cmde[i+1];
	    }
	    break;
	}
    }

    if (outputfname.empty()) {
	cerr << "hcpp2cpp  error, missing filename output" << endl;
	return 1;
    }

    ofstream out(outputfname.c_str());

    if (!out) {
	int e = errno;
	cerr << "hcpp2cpp  error, cannot open " << outputfname << " , "
	     << strerror(e) << endl;
	return 1;
    }

    for (i=1 ; i<nb ; i++) {
	if (strncmp (cmde[i], "-o", 2) == 0) {
	    if (cmde[i][2] == 0) {
		i++;
	    }
	    continue;
	}

	ifstream in(cmde[i]);
	if (!in) {
	    int e = errno;
	    cerr << "hcpp2cpp  error, cannot open " << cmde[i] << " , "
		 << strerror(e) << endl;
	    return 1;
	}
if (debug)
cerr << "parsing " << cmde[i] << " ..." << endl;
	out << "# 1 \"" << cmde[i] << "\"" << endl;
	if (!parse (in, out))
	    break;
    }

    return 0;
}
