/*******************************************************************************
* djinterp [re_std]                                               shift_left.hpp
*
* shift_left algorithm header:
*   Shifts the elements of [_first, _last) leftward by _n positions.
* Elements in the first _n positions are overwritten; the result is the
* original suffix [_first + _n, _last) packed at the beginning. Returns
* the iterator one past the last element of the new (shorter) valid
* range — i.e. _first + ((_last - _first) - _n), or _first if _n >= the
* range length.
*
*   PORTABILITY:
*   - std::shift_left is C++20; re_std back-ports to C++98 (no language
*     blocker — the algorithm is just a forward walk with assignment).
*   - C++11+ uses move assignment for the shifted elements; C++98 uses
*     copy assignment (gated on RE_STD_LANG_HAS_RVALUE_REFERENCES).
*   - constexpr in std from C++20; re_std lifts to C++14.
*   - Non-positive _n is a no-op that returns _last.
*
*
* path:      /inc/re_std/algorithm/shift_left.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_SHIFT_LEFT_HPP
#define RE_STD_ALGORITHM_SHIFT_LEFT_HPP 1

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
// I.   SHIFT_LEFT
// ===========================================================================

// shift_left
//   function: shifts [_first, _last) leftward by _n. Returns the
// iterator one past the last element of the resulting (shorter) valid
// range. Returns _last when _n <= 0, _first when _n >= the range
// length.
template<typename ForwardIt>
RE_STD_CONSTEXPR_CPP14 ForwardIt
shift_left(
    ForwardIt _first,
    ForwardIt _last,
    typename iterator_traits<ForwardIt>::difference_type _n
)
{
    if (_n <= 0)
    {
        return _last;
    }

    // advance the source pointer by _n, watching for premature end
    ForwardIt _source = _first;
    typename iterator_traits<ForwardIt>::difference_type _i = 0;
    while ( (_i < _n) &&
            (_source != _last) )
    {
        ++_source;
        ++_i;
    }
    if (_source == _last)
    {
        // _n >= range length: the entire range is "shifted out"
        return _first;
    }

    // pull [_source, _last) forward over [_first, ...)
    ForwardIt _dest = _first;
    while (_source != _last)
    {
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
        *_dest = re_std::move(*_source);
#else
        *_dest = *_source;
#endif
        ++_dest;
        ++_source;
    }

    return _dest;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_SHIFT_LEFT_HPP
