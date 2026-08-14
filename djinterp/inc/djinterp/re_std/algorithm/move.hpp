/******************************************************************************
* djinterp [restd]                                                      move.hpp
*
* move algorithm header:
*   Moves elements from [_first, _last) to the output range starting
* at _d_first via restd::move (the utility cast). Returns the iterator
* one past the last element written.
*
*   PORTABILITY:
*   - std::move (algorithm) is C++11. Requires rvalue references; cannot
*     be back-ported with correct semantics to C++98 (a copy fallback
*     would silently change meaning).
*   - Coexists with restd::move (the utility cast in
*     restd/utility/move.hpp) via overload resolution: the cast takes
*     one argument; the algorithm takes three iterator arguments.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/move.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_MOVE_
#define DJINTERP_RESTD_ALGORITHM_MOVE_ 1

#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   GATE: rvalue references required
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// restd
#include "../utility/move.hpp"


// ===========================================================================
// 1.   COMPATIBILITY MACROS
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
// I.   MOVE (ALGORITHM)
// ===========================================================================

// move (algorithm)
//   function: moves [_first, _last) into [_d_first, _d_first + N) via
// restd::move-cast on each element. Returns one past the last element
// written.
template<typename _InputIt,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
move(
    _InputIt  _first,
    _InputIt  _last,
    _OutputIt _d_first
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        *_d_first = restd::move(*_first);
    }

    return _d_first;
}


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


#endif  // DJINTERP_RESTD_ALGORITHM_MOVE_
