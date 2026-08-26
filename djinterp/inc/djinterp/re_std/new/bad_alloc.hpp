/******************************************************************************
* djinterp [re_std]                                                 bad_alloc.hpp
*
* bad_alloc re-export header:
*   re_std::bad_alloc is a using-alias for std::bad_alloc — the
* exception thrown by the global operator new when memory allocation
* fails. The standard library's runtime supplies the actual
* implementation; re_std's role here is namespace-consistency so
* other re_std modules can write
*
*     throw re_std::bad_alloc();
*
* rather than crossing namespaces.
*
*   PORTABILITY:
*   std::bad_alloc has been in <new> since C++98 (was previously
* xalloc in pre-standard libraries). No back-port needed.
*
*
* path:      /inc/djinterp/re_std/new/bad_alloc.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RE_STD_BAD_ALLOC_
#define DJINTERP_RE_STD_BAD_ALLOC_ 1

#include "../../core/djinterp.hpp"
#include <new>


NS_RESTD


// ===========================================================================
// I.   BAD_ALLOC
// ===========================================================================

// using-declaration; works across all standards. Drags std::bad_alloc
// into the re_std namespace by name without copying or aliasing — the
// type identity is preserved (catch on std::bad_alloc& still catches
// re_std::bad_alloc and vice versa).
using std::bad_alloc;


NS_END  // re_std


#endif  // DJINTERP_RE_STD_BAD_ALLOC_
