/******************************************************************************
* djinterp [restd]                                                    nothrow.hpp
*
* nothrow tag + constant header:
*   restd::nothrow_t (tag type) and restd::nothrow (constant) are
* using-declarations from std::. The non-throwing operator-new
* overload `operator new(size_t, nothrow_t const&)` is the only
* common use — the rest of the standard's nothrow infrastructure
* lives in the runtime.
*
*   PORTABILITY:
*   Both have been in <new> since C++98. No back-port needed.
*
*
* path:      /inc/djinterp/re_std/new/nothrow.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_NOTHROW_
#define DJINTERP_RESTD_NOTHROW_ 1

#include "../../core/djinterp.hpp"
#include <new>


NS_RESTD


// ===========================================================================
// I.   NOTHROW
// ===========================================================================

using std::nothrow_t;
using std::nothrow;


NS_END  // restd


#endif  // DJINTERP_RESTD_NOTHROW_
