/* shared.hpp: rules both guides state in the same words, read as C++ */
#ifndef D_SHARED_FIXTURE
#define D_SHARED_FIXTURE 1

/*
TABLE OF CONTENTS
=================
I.   CHECKS

II.  NAMES
              a. first
*/

#include <cstdint>
#include "../djinterp.hpp"  // NS_*, feature macros


// =============================================================================
// I.   CHECKS
// =============================================================================

static_assert(sizeof(int) >= 2, "int holds 16 bits");

#ifdef D_SHARED_FIXTURE_EXTRA
// i.    extras
int d_extra(int _a);
#endif

#endif  // D_SHARED_FIXTURE
