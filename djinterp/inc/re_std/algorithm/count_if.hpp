/*******************************************************************************
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
* path:      /inc/re_std/algorithm/count_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_COUNT_IF_HPP
#define RE_STD_ALGORITHM_COUNT_IF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "../iterator/iterator_traits.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   COUNT_IF
// ===========================================================================

// count_if
//   function: returns the number of elements in [_first, _last) for
// which _pred returns true.
template<typename InputIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 typename iterator_traits<InputIt>::difference_type
count_if(
    InputIt _first,
    InputIt _last,
    Pred    _pred
)
{
    typename iterator_traits<InputIt>::difference_type _result = 0;

    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            ++_result;
        }
    }

    return _result;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_COUNT_IF_HPP
