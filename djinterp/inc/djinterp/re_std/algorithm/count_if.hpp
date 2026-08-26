/******************************************************************************
* djinterp [re_std]                                                 count_if.hpp
*
* count_if algorithm header:
*   Returns the number of elements in [_first, _last) for which the
* unary predicate _pred returns true. Well-defined and returns 0 for an
* empty range.
*
*   PORTABILITY:
*   - std::count_if is C++98.
*   - Return type is iterator_traits<It>::difference_type (signed).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/count_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_COUNT_IF_
#define DJINTERP_RE_STD_ALGORITHM_COUNT_IF_ 1

// djinterp
#include "../../core/djinterp.hpp"
// re_std
#include "../iterator/iterator_traits.hpp"


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
// I.   COUNT_IF
// ===========================================================================

// count_if
//   function: returns the number of elements in [_first, _last) for
// which _pred returns true.
template<typename _InputIt,
         typename _Pred>
D_CONSTEXPR_CPP14 typename iterator_traits<_InputIt>::difference_type
count_if(
    _InputIt _first,
    _InputIt _last,
    _Pred    _pred
)
{
    typename iterator_traits<_InputIt>::difference_type _result = 0;

    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            ++_result;
        }
    }

    return _result;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_COUNT_IF_
