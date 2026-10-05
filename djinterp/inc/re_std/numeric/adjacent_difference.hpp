/*******************************************************************************
* djinterp [re_std]                                      adjacent_difference.hpp
*
* adjacent_difference algorithm header:
* adjacent_difference(_first, _last, _d_first [, _op]) writes the
* sequence
*   d[0] = src[0]
*   d[i] = src[i] - src[i-1]   for i > 0
* into _d_first.
*
* with a custom op, replaces the subtraction. Note _op(b, a) — second
* arg is the EARLIER element, matching std::adjacent_difference.
*
* return value: iterator past the last destination written.
*
*
* path:      /inc/re_std/numeric/adjacent_difference.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_ADJACENT_DIFFERENCE_HPP
#define RE_STD_NUMERIC_ADJACENT_DIFFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "re_std/iterator/iterator_traits.hpp"

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    #include "re_std/utility/move.hpp"
#endif


namespace re_std
{

// Default-op overload (operator-).
template<typename InputIt, typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt adjacent_difference
(
    InputIt   _first,
    InputIt   _last,
    OutputIt  _d_first
)
{
    if (_first == _last) return _d_first;

    typedef typename iterator_traits<InputIt>::value_type _T;

    _T _prev = *_first;
    *_d_first = _prev;

    for (++_first, (void)++_d_first; _first != _last; ++_first, (void)++_d_first)
    {
        _T _cur = *_first;
        // *d = cur - prev
        #if RE_STD_LANG_IS_CPP11_OR_HIGHER
            *_d_first = _cur - re_std::move(_prev);
            _prev = re_std::move(_cur);
        #else
            *_d_first = _cur - _prev;
            _prev = _cur;
        #endif
    }
    return _d_first;
}

// Custom-op overload.
template<typename InputIt, typename OutputIt, typename BinOp>
RE_STD_CONSTEXPR_CPP14 OutputIt adjacent_difference
(
    InputIt   _first,
    InputIt   _last,
    OutputIt  _d_first,
    BinOp     _op
)
{
    if (_first == _last) return _d_first;

    typedef typename iterator_traits<InputIt>::value_type _T;

    _T _prev = *_first;
    *_d_first = _prev;

    for (++_first, (void)++_d_first; _first != _last; ++_first, (void)++_d_first)
    {
        _T _cur = *_first;
        #if RE_STD_LANG_IS_CPP11_OR_HIGHER
            *_d_first = _op(_cur, re_std::move(_prev));
            _prev = re_std::move(_cur);
        #else
            *_d_first = _op(_cur, _prev);
            _prev = _cur;
        #endif
    }
    return _d_first;
}


}  // re_std
#endif  // RE_STD_NUMERIC_ADJACENT_DIFFERENCE_HPP
