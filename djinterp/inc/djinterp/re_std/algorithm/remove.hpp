/******************************************************************************
* djinterp [restd]                                                    remove.hpp
*
* remove algorithm header:
*   In-place compaction. Walks [_first, _last) and pulls every element
* not equal to _value forward to overwrite the removed positions.
* Returns the iterator one past the last KEPT element; elements in
* [returned, _last) are in valid-but-unspecified state.
*
*   PORTABILITY:
*   - std::remove is C++98. C++11 strengthened the kept-element transfer
*     from copy assignment to move assignment. restd honours the same
*     evolution: copy on C++98/03, move on C++11+ (gated on
*     D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Implementation forwards through find for the skip-prefix scan, so
*     this header includes find.hpp.
*
*
* path:      /inc/djinterp/re_std/algorithm/remove.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_REMOVE_
#define DJINTERP_RESTD_ALGORITHM_REMOVE_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./find.hpp"
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "../utility/move.hpp"
#endif


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
// I.   REMOVE
// ===========================================================================

// remove
//   function: in-place compaction by value. Returns the iterator one
// past the last kept element. Kept elements retain their relative
// order. The unspecified tail [returned, _last) must be erased by the
// caller if a true size reduction is desired (the "erase-remove"
// idiom).
template<typename _ForwardIt,
         typename _Type>
D_CONSTEXPR_CPP14 _ForwardIt
remove(
    _ForwardIt   _first,
    _ForwardIt   _last,
    const _Type& _value
)
{
    // skip the matchless prefix
    _first = restd::find(_first, _last, _value);
    if (_first == _last)
    {
        return _first;
    }

    // _first now points at the first removable element; pull subsequent
    // non-matching elements forward over it
    _ForwardIt _it = _first;
    ++_it;

    for (; _it != _last; ++_it)
    {
        if (!(*_it == _value))
        {
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
            *_first = restd::move(*_it);
#else
            *_first = *_it;
#endif
            ++_first;
        }
    }

    return _first;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_REMOVE_
