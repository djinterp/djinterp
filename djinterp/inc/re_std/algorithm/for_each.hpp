/*******************************************************************************
* djinterp [re_std]                                                 for_each.hpp
*
* for_each algorithm header:
*   Applies _f to every element in [_first, _last) in sequence. Returns
* _f by value so that stateful functors can recover their accumulated
* state at the call site.
*
*   PORTABILITY:
*   - std::for_each is C++98. C++11 changed the return to std::move(f);
*     observably equivalent for non-throwing functors. re_std returns by
*     value on every tier to keep the C++98 path move-free.
*   - Sequential ordering is guaranteed only for this (no-policy)
*     overload; the C++17 ExecutionPolicy overload (deferred) drops it.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/for_each.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_FOR_EACH_HPP
#define RE_STD_ALGORITHM_FOR_EACH_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   FOR_EACH
// ===========================================================================

// for_each
//   function: invokes _f(*it) for each it in [_first, _last). Returns
// _f. NRVO + copy elision keep this efficient even without explicit
// move.
template<typename InputIt,
         typename Func>
RE_STD_CONSTEXPR_CPP14 Func
for_each(
    InputIt _first,
    InputIt _last,
    Func    _f
)
{
    for (; _first != _last; ++_first)
    {
        _f(*_first);
    }

    return _f;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_FOR_EACH_HPP
