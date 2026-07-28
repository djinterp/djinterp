/******************************************************************************
* djinterp [restd]                                                shift_left.hpp
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
*   - std::shift_left is C++20; restd back-ports to C++98 (no language
*     blocker — the algorithm is just a forward walk with assignment).
*   - C++11+ uses move assignment for the shifted elements; C++98 uses
*     copy assignment (gated on D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES).
*   - constexpr in std from C++20; restd lifts to C++14.
*   - Non-positive _n is a no-op that returns _last.
*
*
* path:      /inc/djinterp/restd/algorithm/shift_left.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_SHIFT_LEFT_
#define DJINTERP_RESTD_ALGORITHM_SHIFT_LEFT_ 1

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
// I.   SHIFT_LEFT
// ===========================================================================

// shift_left
//   function: shifts [_first, _last) leftward by _n. Returns the
// iterator one past the last element of the resulting (shorter) valid
// range. Returns _last when _n <= 0, _first when _n >= the range
// length.
template<typename _ForwardIt>
D_CONSTEXPR_CPP14 _ForwardIt
shift_left(
    _ForwardIt _first,
    _ForwardIt _last,
    typename iterator_traits<_ForwardIt>::difference_type _n
)
{
    if (_n <= 0)
    {
        return _last;
    }

    // advance the source pointer by _n, watching for premature end
    _ForwardIt _source = _first;
    typename iterator_traits<_ForwardIt>::difference_type _i = 0;
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
    _ForwardIt _dest = _first;
    while (_source != _last)
    {
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
        *_dest = restd::move(*_source);
#else
        *_dest = *_source;
#endif
        ++_dest;
        ++_source;
    }

    return _dest;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_SHIFT_LEFT_
