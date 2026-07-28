/******************************************************************************
* djinterp [restd]                                               remove_copy.hpp
*
* remove_copy algorithm header:
*   Out-of-place sibling of remove. Copies elements from
* [_first, _last) to the output range starting at _d_first, skipping
* those equal to _value. Returns the iterator one past the last element
* written.
*
*   PORTABILITY:
*   - std::remove_copy is C++98.
*   - Pure copy semantics (writes to a separate output range); no
*     conditional move dance like in-place remove.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/restd/algorithm/remove_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_REMOVE_COPY_
#define DJINTERP_RESTD_ALGORITHM_REMOVE_COPY_ 1

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
// I.   REMOVE_COPY
// ===========================================================================

// remove_copy
//   function: copies every element of [_first, _last) not equal to
// _value into the output range starting at _d_first. Returns the
// iterator one past the last element written.
template<typename _InputIt,
         typename _OutputIt,
         typename _Type>
D_CONSTEXPR_CPP14 _OutputIt
remove_copy(
    _InputIt     _first,
    _InputIt     _last,
    _OutputIt    _d_first,
    const _Type& _value
)
{
    for (; _first != _last; ++_first)
    {
        if (!(*_first == _value))
        {
            *_d_first = *_first;
            ++_d_first;
        }
    }

    return _d_first;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_REMOVE_COPY_
