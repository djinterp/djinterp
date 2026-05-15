/******************************************************************************
* djinterp [restd]                                                      copy.hpp
*
* copy algorithm header:
*   Copies elements from [_first, _last) to the output range starting
* at _d_first. Returns the iterator one past the last copied element
* (i.e. _d_first + (_last - _first)).
*
*   PORTABILITY:
*   - std::copy is C++98.
*   - The (void) cast on the output increment guards against
*     operator-comma overloads on proxy iterators.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/restd/algorithm/copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_COPY_
#define DJINTERP_RESTD_ALGORITHM_COPY_ 1

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
// I.   COPY
// ===========================================================================

// copy
//   function: copies [_first, _last) into [_d_first, _d_first + N).
// Returns one past the last copied element. Source and destination
// must not overlap unless _d_first is outside [_first, _last).
template<typename _InputIt,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
copy(
    _InputIt  _first,
    _InputIt  _last,
    _OutputIt _d_first
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        *_d_first = *_first;
    }

    return _d_first;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_COPY_
