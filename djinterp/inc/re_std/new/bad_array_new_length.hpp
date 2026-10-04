/******************************************************************************
* djinterp [re_std]                                     bad_array_new_length.hpp
*
* bad_array_new_length header:
*   Exception thrown by array form of operator new when the total
* size requested would exceed implementation limits or when an
* explicit size is negative. Standardised in C++11.
*
*   STRATEGY:
*   C++11+: using-declaration from std::bad_array_new_length.
*   C++98:  back-port — class deriving from re_std::bad_alloc
*           (which is just std::bad_alloc) with what() override.
*
*   NOTE:
*   The C++98 back-port cannot be thrown automatically by the
* compiler's array-new path (that's runtime-provided), but user
* code that explicitly throws bad_array_new_length will work on
* C++98 the same way it does on C++11+. re_std modules that want
* to surface this error condition just throw the back-port.
*
*
* path:      /inc/djinterp/re_std/new/bad_array_new_length.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RE_STD_BAD_ARRAY_NEW_LENGTH_
#define DJINTERP_RE_STD_BAD_ARRAY_NEW_LENGTH_ 1

#include "../../core/djinterp.hpp"
#include "./bad_alloc.hpp"


NS_RESTD


// ===========================================================================
// I.   BAD_ARRAY_NEW_LENGTH
// ===========================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// C++11+ std::bad_array_new_length is available. Pass-through.
using std::bad_array_new_length;

#else

// C++98 back-port. Derives from std::bad_alloc; what() identifies
// it as bad_array_new_length so catch sites can distinguish.
class bad_array_new_length : public bad_alloc
{
public:
    bad_array_new_length() throw() {}
    virtual ~bad_array_new_length() throw() {}

    virtual const char* what() const throw()
    {
        return "bad_array_new_length";
    }
};

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_BAD_ARRAY_NEW_LENGTH_
