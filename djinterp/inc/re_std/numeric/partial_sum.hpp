/*******************************************************************************
* djinterp [re_std]                                              partial_sum.hpp
*
* partial_sum(_first, _last, _d_first [, _op]) writes the running fold
* (default: operator+) of [_first, _last) into _d_first:
*
*   d[0] = src[0]
*   d[1] = d[0] + src[1]
*   d[2] = d[1] + src[2]
*   ...
*
* preserves source order; serial only.
*
* return value:
*   iterator to one past the last destination element written.
*
*
* path:      /inc/re_std/numeric/partial_sum.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_PARTIAL_SUM_HPP
#define RE_STD_NUMERIC_PARTIAL_SUM_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "re_std/iterator/iterator_traits.hpp"

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    #include "re_std/utility/move.hpp"
#endif


namespace re_std
{

// Default-op overload (operator+).
template<typename InputIt, typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt partial_sum
(
    InputIt   _first,
    InputIt   _last,
    OutputIt  _d_first
)
{
    if (_first == _last) return _d_first;

    typename iterator_traits<InputIt>::value_type _acc = *_first;
    *_d_first = _acc;

    for (++_first, (void)++_d_first; _first != _last; ++_first, (void)++_d_first)
    {
        #if RE_STD_LANG_IS_CPP11_OR_HIGHER
            _acc = re_std::move(_acc) + *_first;
        #else
            _acc = _acc + *_first;
        #endif
        *_d_first = _acc;
    }
    return _d_first;
}

// Custom-op overload.
template<typename InputIt, typename OutputIt, typename BinOp>
RE_STD_CONSTEXPR_CPP14 OutputIt partial_sum
(
    InputIt   _first,
    InputIt   _last,
    OutputIt  _d_first,
    BinOp     _op
)
{
    if (_first == _last) return _d_first;

    typename iterator_traits<InputIt>::value_type _acc = *_first;
    *_d_first = _acc;

    for (++_first, (void)++_d_first; _first != _last; ++_first, (void)++_d_first)
    {
        #if RE_STD_LANG_IS_CPP11_OR_HIGHER
            _acc = _op(re_std::move(_acc), *_first);
        #else
            _acc = _op(_acc, *_first);
        #endif
        *_d_first = _acc;
    }
    return _d_first;
}


}  // re_std
#endif  // RE_STD_NUMERIC_PARTIAL_SUM_HPP
