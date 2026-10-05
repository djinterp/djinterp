/*******************************************************************************
* djinterp [re_std]                                               for_each_n.hpp
*
* for_each_n algorithm header:
*   Counted variant of for_each. Invokes _f on the first _n elements
* starting at _first and returns the iterator one past the last visited
* element (i.e. _first + _n).
*
*   PORTABILITY:
*   - std::for_each_n is C++17; re_std back-ports to C++98 (just a
*     counted loop; no language blocker).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - The (void) cast on the iterator-increment guards against
*     operator-comma overloads on weird proxy iterators (matches
*     libstdc++ idiom).
*
*
* path:      /inc/re_std/algorithm/for_each_n.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_FOR_EACH_N_HPP
#define RE_STD_ALGORITHM_FOR_EACH_N_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   FOR_EACH_N
// ===========================================================================

// for_each_n
//   function: invokes _f(*it) for it in [_first, _first + _n). Returns
// the iterator one past the last visited element. Non-positive _n is a
// no-op that returns _first unchanged.
template<typename InputIt,
         typename Size,
         typename Func>
RE_STD_CONSTEXPR_CPP14 InputIt
for_each_n(
    InputIt _first,
    Size    _n,
    Func    _f
)
{
    for (; _n > 0; --_n, (void)++_first)
    {
        _f(*_first);
    }

    return _first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_FOR_EACH_N_HPP
