/******************************************************************************
* djinterp [restd]                                                   copy_if.hpp
*
* copy_if algorithm header:
*   Copies elements from [_first, _last) for which _pred returns true
* into the output range starting at _d_first. Returns the iterator one
* past the last copied element.
*
*   PORTABILITY:
*   - std::copy_if is C++11; restd back-ports to C++98 (no language
*     blocker — just predicate + conditional assign).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/restd/algorithm/copy_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_COPY_IF_
#define DJINTERP_RESTD_ALGORITHM_COPY_IF_ 1

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
// I.   COPY_IF
// ===========================================================================

// copy_if
//   function: copies every element of [_first, _last) for which
// _pred(*it) holds into the output range starting at _d_first. Returns
// the iterator one past the last element written.
template<typename _InputIt,
         typename _OutputIt,
         typename _Pred>
D_CONSTEXPR_CPP14 _OutputIt
copy_if(
    _InputIt  _first,
    _InputIt  _last,
    _OutputIt _d_first,
    _Pred     _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            *_d_first = *_first;
            ++_d_first;
        }
    }

    return _d_first;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_COPY_IF_
