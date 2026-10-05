/*******************************************************************************
* djinterp [re_std]                                              shift_right.hpp
*
* shift_right algorithm header:
*   Shifts the elements of [_first, _last) rightward by _n positions.
* The last _n positions are overwritten; the result is the original
* prefix [_first, _last - _n) packed at the end. Returns the iterator
* to the new beginning of the valid range — i.e. _first + _n, or _last
* if _n >= the range length.
*
*   PORTABILITY:
*   - std::shift_right is C++20 and requires only ForwardIterator. re_std
*     requires a BIDIRECTIONAL iterator (DEVIATION FROM STD) because the
*     forward-only algorithm is essentially a hidden rotate; the
*     additional complexity is judged not worth the niche use case here.
*     Forward-only callers will get a hard compile error on --_last.
*   - C++11+ uses move assignment; C++98 uses copy
*     (gated on RE_STD_LANG_HAS_RVALUE_REFERENCES).
*   - constexpr in std from C++20; re_std lifts to C++14.
*   - Non-positive _n is a no-op that returns _first.
*
*
* path:      /inc/re_std/algorithm/shift_right.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_SHIFT_RIGHT_HPP
#define RE_STD_ALGORITHM_SHIFT_RIGHT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "../iterator/iterator_traits.hpp"
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "../utility/move.hpp"
#endif


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   SHIFT_RIGHT
// ===========================================================================

// shift_right
//   function: shifts [_first, _last) rightward by _n. Returns the
// iterator to the new beginning of the valid range. Returns _first
// when _n <= 0, _last when _n >= the range length.
// requires: BidirectionalIterator (see header comment).
template<typename BidirIt>
RE_STD_CONSTEXPR_CPP14 BidirIt
shift_right(
    BidirIt _first,
    BidirIt _last,
    typename iterator_traits<BidirIt>::difference_type _n
)
{
    if (_n <= 0)
    {
        return _first;
    }

    // walk a "source end" pointer backward _n steps from _last;
    // it then equals (_last - _n) and bounds the source range.
    BidirIt _source_end = _last;
    typename iterator_traits<BidirIt>::difference_type _i = 0;
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
    BidirIt _src = _source_end;
    BidirIt _dst = _last;
    while (_src != _first)
    {
        --_src;
        --_dst;
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
        *_dst = re_std::move(*_src);
#else
        *_dst = *_src;
#endif
    }

    return _dst;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_SHIFT_RIGHT_HPP
