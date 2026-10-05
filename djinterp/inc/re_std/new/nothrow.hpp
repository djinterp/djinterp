/*******************************************************************************
* djinterp [re_std]                                                  nothrow.hpp
*
* nothrow tag + constant header:
*   re_std::nothrow_t (tag type) and re_std::nothrow (constant) are
* using-declarations from std::. The non-throwing operator-new
* overload `operator new(size_t, nothrow_t const&)` is the only
* common use — the rest of the standard's nothrow infrastructure
* lives in the runtime.
*
*   PORTABILITY:
*   Both have been in <new> since C++98. No back-port needed.
*
*
* path:      /inc/re_std/new/nothrow.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_NEW_NOTHROW_HPP
#define RE_STD_NEW_NOTHROW_HPP 1

// std
#include <new>
// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// ===========================================================================
// I.   NOTHROW
// ===========================================================================

using std::nothrow_t;
using std::nothrow;


}  // re_std


#endif  // RE_STD_NEW_NOTHROW_HPP
