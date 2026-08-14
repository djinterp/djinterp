/******************************************************************************
* djinterp [restd]                                                  for_each.hpp
*
* for_each algorithm header:
*   Applies _f to every element in [_first, _last) in sequence. Returns
* _f by value so that stateful functors can recover their accumulated
* state at the call site.
*
*   PORTABILITY:
*   - std::for_each is C++98. C++11 changed the return to std::move(f);
*     observably equivalent for non-throwing functors. restd returns by
*     value on every tier to keep the C++98 path move-free.
*   - Sequential ordering is guaranteed only for this (no-policy)
*     overload; the C++17 ExecutionPolicy overload (deferred) drops it.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/for_each.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_FOR_EACH_
#define DJINTERP_RESTD_ALGORITHM_FOR_EACH_ 1

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
// I.   FOR_EACH
// ===========================================================================

// for_each
//   function: invokes _f(*it) for each it in [_first, _last). Returns
// _f. NRVO + copy elision keep this efficient even without explicit
// move.
template<typename _InputIt,
         typename _Func>
D_CONSTEXPR_CPP14 _Func
for_each(
    _InputIt _first,
    _InputIt _last,
    _Func    _f
)
{
    for (; _first != _last; ++_first)
    {
        _f(*_first);
    }

    return _f;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_FOR_EACH_
