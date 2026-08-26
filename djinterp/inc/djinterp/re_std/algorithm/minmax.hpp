/******************************************************************************
* djinterp [re_std]                                                   minmax.hpp
*
* minmax algorithm header:
*   Two families:
*
*     minmax(a, b)            -> pair<const T&, const T&>
*     minmax(init_list)       -> pair<T, T>        (C++11+ only)
*
*   each with a comparator overload. The two-argument form returns
* pair(b, a) when b compares less than a, and pair(a, b) otherwise --
* so on a TIE the arguments come back in the order given.
*
*   PORTABILITY:
*   - std::minmax is C++11; re_std back-ports the two-argument forms
*     to C++98. The initializer_list forms cannot be back-ported:
*     the language cannot form the type before C++11, so they are
*     gated on D_ENV_LANG_IS_CPP11_OR_HIGHER.
*   - constexpr in std from C++14; re_std matches at C++14.
*   - The initializer_list forms delegate to minmax_element, which is
*     what gives them the standard-mandated asymmetric tie-breaking
*     (leftmost minimum, rightmost maximum).
*
*   THE RETURN TYPE IS TIERED, AND THIS IS DELIBERATE:
*
*     C++11+ :  pair<const T&, const T&>   (matches std exactly)
*     C++98   :  pair<T, T>                (by value)
*
*   re_std::pair cannot hold reference members before C++11. Its
* constructor takes `const _T1&`, and with _T1 = const T& that is a
* reference to a reference -- ill-formed under the C++98 text, and
* rejected in practice (GCC 13 -std=c++98: "forming reference to
* reference type"). Reference collapsing only became guaranteed in
* C++11.
*
*   The alternatives were to drop the two-argument forms below C++11
* or to return by value there. Returning by value keeps the symbol at
* the C++98 floor at the cost of one copy of each argument, and costs
* no conformance: std::minmax does not exist before C++11, so there is
* no C++98 contract to deviate from.
*
*   Code that must compile on both tiers should receive the result by
* value -- `pair<T, T> p = minmax(a, b);` -- which is well-formed on
* C++11+ too via the converting constructor.
*
*
* path:      /inc/djinterp/re_std/algorithm/minmax.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.24
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_MINMAX_
#define DJINTERP_RE_STD_ALGORITHM_MINMAX_ 1

// djinterp
#include "../../core/djinterp.hpp"
// re_std
#include "./minmax_element.hpp"
#include "../utility/pair.hpp"
#include "../initializer_list/initializer_list.hpp"


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
// 0b.  MINMAX_RESULT  (tiered return type -- see the header note)
// ===========================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #define D_RE_STD_MINMAX_RESULT(T)  pair<const T&, const T&>
#else
    // re_std::pair cannot hold reference members on C++98: its ctor
    // parameter `const _T1&` becomes a reference to a reference.
    #define D_RE_STD_MINMAX_RESULT(T)  pair<T, T>
#endif


// ===========================================================================
// I.   MINMAX (TWO ARGUMENTS, DEFAULT operator<)
// ===========================================================================

// minmax
//   function: pair(smaller, larger). On a tie returns pair(_a, _b), so
// the caller's argument order is preserved.
template<typename _Type>
D_CONSTEXPR_CPP14 D_RE_STD_MINMAX_RESULT(_Type)
minmax(
    const _Type& _a,
    const _Type& _b
)
{
    return (_b < _a)
        ? D_RE_STD_MINMAX_RESULT(_Type)(_b, _a)
        : D_RE_STD_MINMAX_RESULT(_Type)(_a, _b);
}


// ===========================================================================
// II.  MINMAX (TWO ARGUMENTS, COMPARATOR)
// ===========================================================================

// minmax (comparator)
//   function: as above but ordering is decided by _comp.
template<typename _Type,
         typename _Compare>
D_CONSTEXPR_CPP14 D_RE_STD_MINMAX_RESULT(_Type)
minmax(
    const _Type& _a,
    const _Type& _b,
    _Compare     _comp
)
{
    return _comp(_b, _a)
        ? D_RE_STD_MINMAX_RESULT(_Type)(_b, _a)
        : D_RE_STD_MINMAX_RESULT(_Type)(_a, _b);
}


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// ===========================================================================
// III. MINMAX (INITIALIZER_LIST, DEFAULT operator<)
// ===========================================================================

// minmax (initializer_list)
//   function: pair(smallest, largest) BY VALUE. Delegates to
// minmax_element, so the minimum is the leftmost and the maximum the
// rightmost of any equal run. The list must not be empty.
template<typename _Type>
D_CONSTEXPR_CPP14 pair<_Type, _Type>
minmax(
    initializer_list<_Type> _list
)
{
    pair<const _Type*, const _Type*> _p =
        re_std::minmax_element(_list.begin(), _list.end());
    return pair<_Type, _Type>(*_p.first, *_p.second);
}


// ===========================================================================
// IV.  MINMAX (INITIALIZER_LIST, COMPARATOR)
// ===========================================================================

// minmax (initializer_list, comparator)
//   function: as above but ordering is decided by _comp.
template<typename _Type,
         typename _Compare>
D_CONSTEXPR_CPP14 pair<_Type, _Type>
minmax(
    initializer_list<_Type> _list,
    _Compare                _comp
)
{
    pair<const _Type*, const _Type*> _p =
        re_std::minmax_element(_list.begin(), _list.end(), _comp);
    return pair<_Type, _Type>(*_p.first, *_p.second);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_MINMAX_
