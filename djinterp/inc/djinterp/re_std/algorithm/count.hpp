/******************************************************************************
* djinterp [restd]                                                     count.hpp
*
* count algorithm header:
*   Returns the number of elements in [_first, _last) that compare equal
* to _value via operator==. Well-defined and returns 0 for an empty
* range.
*
*   PORTABILITY:
*   - std::count is C++98.
*   - Return type is iterator_traits<It>::difference_type (signed).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14 (the
*     mutable accumulator requires relaxed constexpr).
*
*
* path:      /inc/djinterp/restd/algorithm/count.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_COUNT_
#define DJINTERP_RESTD_ALGORITHM_COUNT_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
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
// I.   COUNT
// ===========================================================================

// count
//   function: returns the number of elements in [_first, _last) that
// compare equal (operator==) to _value.
template<typename _InputIt,
         typename _Type>
D_CONSTEXPR_CPP14 typename iterator_traits<_InputIt>::difference_type
count(
    _InputIt     _first,
    _InputIt     _last,
    const _Type& _value
)
{
    typename iterator_traits<_InputIt>::difference_type _result = 0;

    for (; _first != _last; ++_first)
    {
        if (*_first == _value)
        {
            ++_result;
        }
    }

    return _result;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_COUNT_
