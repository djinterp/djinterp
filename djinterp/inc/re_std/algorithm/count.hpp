/*******************************************************************************
* djinterp [re_std]                                                    count.hpp
*
* count algorithm header:
*   Returns the number of elements in [_first, _last) that compare equal
* to _value via operator==. Well-defined and returns 0 for an empty
* range.
*
*   PORTABILITY:
*   - std::count is C++98.
*   - Return type is iterator_traits<It>::difference_type (signed).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14 (the
*     mutable accumulator requires relaxed constexpr).
*
*
* path:      /inc/re_std/algorithm/count.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_COUNT_HPP
#define RE_STD_ALGORITHM_COUNT_HPP 1

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
// I.   COUNT
// ===========================================================================

// count
//   function: returns the number of elements in [_first, _last) that
// compare equal (operator==) to _value.
template<typename InputIt,
         typename Type>
RE_STD_CONSTEXPR_CPP14 typename iterator_traits<InputIt>::difference_type
count(
    InputIt     _first,
    InputIt     _last,
    const Type& _value
)
{
    typename iterator_traits<InputIt>::difference_type _result = 0;

    for (; _first != _last; ++_first)
    {
        if (*_first == _value)
        {
            ++_result;
        }
    }

    return _result;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_COUNT_HPP
