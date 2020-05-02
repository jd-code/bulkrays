
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

