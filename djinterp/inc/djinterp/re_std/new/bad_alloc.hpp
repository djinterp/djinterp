/******************************************************************************
* djinterp [restd]                                                  bad_alloc.hpp
*
* bad_alloc re-export header:
*   restd::bad_alloc is a using-alias for std::bad_alloc — the
* exception thrown by the global operator new when memory allocation
* fails. The standard library's runtime supplies the actual
* implementation; restd's role here is namespace-consistency so
* other restd modules can write
*
*     throw restd::bad_alloc();
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

#ifndef DJINTERP_RESTD_BAD_ALLOC_
#define DJINTERP_RESTD_BAD_ALLOC_ 1

#include "../../core/djinterp.hpp"
#include <new>


NS_RESTD


// ===========================================================================
// I.   BAD_ALLOC
// ===========================================================================

// using-declaration; works across all standards. Drags std::bad_alloc
// into the restd namespace by name without copying or aliasing — the
// type identity is preserved (catch on std::bad_alloc& still catches
// restd::bad_alloc and vice versa).
using std::bad_alloc;


NS_END  // restd


#endif  // DJINTERP_RESTD_BAD_ALLOC_
