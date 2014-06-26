#include <iostream>
#include <bulkrays/bulkrays.h>

namespace bulkrays { int testsite_global (bulkrays::BSOperation op); }		// testsite.cpp
namespace simplefmap { int simplefmap_global (bulkrays::BSOperation op); }		// simplefmap.cpp

namespace bulkrays {
    using namespace std;

    int bootstrap_global (bulkrays::BSOperation op) {
	int i = 0;

	if (bulkrays::testsite_global(op) != 0) i++;	    // testsite.cpp
	if (simplefmap::simplefmap_global(op) != 0) i++;    // simplefmap.cpp

	if (i != 0) {
	    cerr << "bootstrap_global(" << op << ") : " << i << "errors reported" << endl;
	    return -1;
	} else
	    return 0;
    }

} // namespace bulkrays

int main (int nb, char ** cmde) {
    return bulkrays::main(nb, cmde);
}
