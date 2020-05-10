/*
 * Bulkrays Copyright (C) 2012-2020 Jean-Daniel Pauget
 * ... light http server C++ library ...
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

void usage (void) {
    cout << "usage : hcpp2cpp [ ... input.hcpp ... ] -o[ ]output.cc" << endl << endl;
}

int main (int nb, char ** cmde) {

    if (nb == 1) {
	usage();
	return 1;
    }

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
	usage();
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
