/***********************************************************************
* re_std                                                      partial_sum.hpp
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
* path:      /inc/djinterp/re_std/numeric/partial_sum.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.09
***********************************************************************/

#ifndef DJINTERP_RE_STD_NUMERIC_PARTIAL_SUM_
#define DJINTERP_RE_STD_NUMERIC_PARTIAL_SUM_ 1

#include "djinterp.hpp"

#include "re_std/iterator/iterator_traits.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include "re_std/utility/move.hpp"
#endif


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace re_std
{

// Default-op overload (operator+).
template<typename _InputIt, typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt partial_sum
(
    _InputIt   _first,
    _InputIt   _last,
    _OutputIt  _d_first
)
{
    if (_first == _last) return _d_first;

    typename iterator_traits<_InputIt>::value_type _acc = *_first;
    *_d_first = _acc;

    for (++_first, (void)++_d_first; _first != _last; ++_first, (void)++_d_first)
    {
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            _acc = re_std::move(_acc) + *_first;
        #else
            _acc = _acc + *_first;
        #endif
        *_d_first = _acc;
    }
    return _d_first;
}

// Custom-op overload.
template<typename _InputIt, typename _OutputIt, typename _BinOp>
D_CONSTEXPR_CPP14 _OutputIt partial_sum
(
    _InputIt   _first,
    _InputIt   _last,
    _OutputIt  _d_first,
    _BinOp     _op
)
{
    if (_first == _last) return _d_first;

    typename iterator_traits<_InputIt>::value_type _acc = *_first;
    *_d_first = _acc;

    for (++_first, (void)++_d_first; _first != _last; ++_first, (void)++_d_first)
    {
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            _acc = _op(re_std::move(_acc), *_first);
        #else
            _acc = _op(_acc, *_first);
        #endif
        *_d_first = _acc;
    }
    return _d_first;
}


}  // namespace re_std

#endif  // DJINTERP_RE_STD_NUMERIC_PARTIAL_SUM_
