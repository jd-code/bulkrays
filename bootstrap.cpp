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
