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


// this one is appart because of the burden from mhash includes conflicting
// with our autoconf includes !

#include <iostream>
#include <string>

#include <mhash.h>

namespace bulkrays {
    using namespace std;

    int md5 (const string &s, string &result) {
	MHASH td;
	td = mhash_init(MHASH_MD5);
	if (td == MHASH_FAILED) {
	    cerr << "md5 error : could not init !" << endl;
	    return -1;
	}
	size_t l = s.size();
	mhash (td, s.c_str(), l);
        unsigned char *hash = (unsigned char *)mhash_end(td);
	if (hash == NULL) {
	    cerr << "md5 error : mhash_end returned null !" << endl;
	    return -1;
	}
static const char *hex = "0123456789abcdef";
        for (size_t i = 0; i < mhash_get_block_size(MHASH_MD5); i++) {
	    result += hex[(hash[i] & 0xf0)>>4];
	    result += hex[(hash[i] & 0x0f)];
	}
	
	free (hash);
//	mhash_deinit (td, NULL);
	return 0;
    }
}

