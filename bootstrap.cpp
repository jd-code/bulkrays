#include <iostream>
namespace bulkrays { int testsite_global (void); }		// testsite.cpp
namespace simplefmap { int simplefmap_global (void); }		// simplefmap.cpp

namespace bulkrays {
    using namespace std;

    int bootstrap_global (void) {
	int i = 0;

	if (bulkrays::testsite_global() != 0) i++;	    // testsite.cpp
	if (simplefmap::simplefmap_global() != 0) i++;	    // simplefmap.cpp

	if (i != 0) {
	    cerr << "bootstrap_global : " << i << "errors reported" << endl;
	    return -1;
	} else
	    return 0;
    }

} // namespace bulkrays
