/******************************************************************************
* djinterp [re_std]                                                bad_cast.hpp
*
* bad_cast exception header:
*   Surfaces re_std::bad_cast as a using-declaration for std::bad_cast —
* the exception thrown when a reference dynamic_cast fails. It is
* runtime-provided (the dynamic_cast machinery throws it directly), so
* re_std re-exports rather than reimplements. Type identity is preserved:
* re_std::bad_cast IS std::bad_cast, so a catch on either spelling catches
* a throw of the other, and the exception thrown by a language-level
* dynamic_cast is caught by catch (const re_std::bad_cast&).
*
*   This is the canonical base that re_std's own bad_*_access exceptions
* (bad_any_cast, bad_expected_access, bad_variant_access) inherit from on
* their Tier-1 (typeinfo-available) path.
*
*   PORTABILITY:
*   Gated on D_ENV_CPP98_HAS_TYPEINFO. C++98 baseline; nothing to
* back-port (std::bad_cast has existed since C++98).
*
*
* path:      /inc/djinterp/re_std/typeinfo/bad_cast.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.04
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPEINFO_BAD_CAST_
#define DJINTERP_RE_STD_TYPEINFO_BAD_CAST_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_CPP98_HAS_TYPEINFO


// std (runtime-provided RTTI types)
#include <typeinfo>


NS_RESTD

// bad_cast
//   class: re-export of std::bad_cast (derives from std::exception).
// Thrown by a failed reference dynamic_cast; what() returns an
// implementation-defined message.
using ::std::bad_cast;

NS_END  // re_std


#endif  // D_ENV_CPP98_HAS_TYPEINFO


#endif  // DJINTERP_RE_STD_TYPEINFO_BAD_CAST_
