/******************************************************************************
* djinterp [re_std]                                                    clamp.hpp
*
* clamp algorithm header:
*   Returns a reference to _v confined to [_lo, _hi]:
*
*     _v < _lo   -> _lo
*     _hi < _v   -> _hi
*     otherwise  -> _v
*
*   PORTABILITY:
*   - std::clamp is C++17; re_std back-ports it to C++98.
*   - constexpr in std from C++17 (it was constexpr on arrival);
*     re_std matches at C++14.
*   - Two overloads: default operator< and custom comparator.
*
*   PRECONDITION:
*   !(_hi < _lo). As in std, this is the caller s responsibility and
* is NOT checked -- an inverted range gives an unspecified result
* rather than a diagnostic.
*
*   The result is a REFERENCE into one of the three arguments, so the
* usual dangling caution applies: binding it to a named reference is
* only safe while all three arguments are alive.
*
*
* path:      /inc/djinterp/re_std/algorithm/clamp.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.24
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_CLAMP_
#define DJINTERP_RE_STD_ALGORITHM_CLAMP_ 1

// djinterp
#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   CLAMP (DEFAULT operator<)
// ===========================================================================

// clamp
//   function: reference to _v, _lo or _hi, whichever lies in range.
template<typename _Type>
D_CONSTEXPR_CPP14 const _Type&
clamp(
    const _Type& _v,
    const _Type& _lo,
    const _Type& _hi
)
{
    return (_v < _lo)
        ? _lo
        : ( (_hi < _v) ? _hi : _v );
}


// ===========================================================================
// II.  CLAMP (COMPARATOR)
// ===========================================================================

// clamp (comparator)
//   function: as above but ordering is decided by _comp.
template<typename _Type,
         typename _Compare>
D_CONSTEXPR_CPP14 const _Type&
clamp(
    const _Type& _v,
    const _Type& _lo,
    const _Type& _hi,
    _Compare     _comp
)
{
    return _comp(_v, _lo)
        ? _lo
        : ( _comp(_hi, _v) ? _hi : _v );
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_CLAMP_
