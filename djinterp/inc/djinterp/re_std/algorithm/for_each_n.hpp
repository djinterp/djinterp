/******************************************************************************
* djinterp [restd]                                                for_each_n.hpp
*
* for_each_n algorithm header:
*   Counted variant of for_each. Invokes _f on the first _n elements
* starting at _first and returns the iterator one past the last visited
* element (i.e. _first + _n).
*
*   PORTABILITY:
*   - std::for_each_n is C++17; restd back-ports to C++98 (just a
*     counted loop; no language blocker).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - The (void) cast on the iterator-increment guards against
*     operator-comma overloads on weird proxy iterators (matches
*     libstdc++ idiom).
*
*
* path:      /inc/djinterp/restd/algorithm/for_each_n.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_FOR_EACH_N_
#define DJINTERP_RESTD_ALGORITHM_FOR_EACH_N_ 1

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
// I.   FOR_EACH_N
// ===========================================================================

// for_each_n
//   function: invokes _f(*it) for it in [_first, _first + _n). Returns
// the iterator one past the last visited element. Non-positive _n is a
// no-op that returns _first unchanged.
template<typename _InputIt,
         typename _Size,
         typename _Func>
D_CONSTEXPR_CPP14 _InputIt
for_each_n(
    _InputIt _first,
    _Size    _n,
    _Func    _f
)
{
    for (; _n > 0; --_n, (void)++_first)
    {
        _f(*_first);
    }

    return _first;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_FOR_EACH_N_
