/******************************************************************************
* djinterp [restd]                                               shift_right.hpp
*
* shift_right algorithm header:
*   Shifts the elements of [_first, _last) rightward by _n positions.
* The last _n positions are overwritten; the result is the original
* prefix [_first, _last - _n) packed at the end. Returns the iterator
* to the new beginning of the valid range — i.e. _first + _n, or _last
* if _n >= the range length.
*
*   PORTABILITY:
*   - std::shift_right is C++20 and requires only ForwardIterator. restd
*     requires a BIDIRECTIONAL iterator (DEVIATION FROM STD) because the
*     forward-only algorithm is essentially a hidden rotate; the
*     additional complexity is judged not worth the niche use case here.
*     Forward-only callers will get a hard compile error on --_last.
*   - C++11+ uses move assignment; C++98 uses copy
*     (gated on D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES).
*   - constexpr in std from C++20; restd lifts to C++14.
*   - Non-positive _n is a no-op that returns _first.
*
*
* path:      /inc/djinterp/re_std/algorithm/shift_right.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_SHIFT_RIGHT_
#define DJINTERP_RESTD_ALGORITHM_SHIFT_RIGHT_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "../iterator/iterator_traits.hpp"
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
// I.   SHIFT_RIGHT
// ===========================================================================

// shift_right
//   function: shifts [_first, _last) rightward by _n. Returns the
// iterator to the new beginning of the valid range. Returns _first
// when _n <= 0, _last when _n >= the range length.
// requires: BidirectionalIterator (see header comment).
template<typename _BidirIt>
D_CONSTEXPR_CPP14 _BidirIt
shift_right(
    _BidirIt _first,
    _BidirIt _last,
    typename iterator_traits<_BidirIt>::difference_type _n
)
{
    if (_n <= 0)
    {
        return _first;
    }

    // walk a "source end" pointer backward _n steps from _last;
    // it then equals (_last - _n) and bounds the source range.
    _BidirIt _source_end = _last;
    typename iterator_traits<_BidirIt>::difference_type _i = 0;
    while ( (_i < _n) &&
            (_source_end != _first) )
    {
        --_source_end;
        ++_i;
    }
    if (_source_end == _first && _i < _n)
    {
        // never reached this branch; safety guard
        return _last;
    }
    if (_i < _n)
    {
        // _n >= range length
        return _last;
    }

    // move [_first, _source_end) into [_first + _n, _last), walking
    // backward to avoid overwriting unread source elements
    _BidirIt _src = _source_end;
    _BidirIt _dst = _last;
    while (_src != _first)
    {
        --_src;
        --_dst;
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
        *_dst = restd::move(*_src);
#else
        *_dst = *_src;
#endif
    }

    return _dst;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_SHIFT_RIGHT_
