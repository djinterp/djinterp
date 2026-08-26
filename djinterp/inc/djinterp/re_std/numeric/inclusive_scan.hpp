/***********************************************************************
* re_std                                                   inclusive_scan.hpp
*
* inclusive_scan is the parallel-friendly prefix-fold:
*   d[0] = src[0]
*   d[i] = op(d[i-1], src[i])
*
* "Inclusive" means each output position includes the corresponding
* input position. Result is identical to partial_sum() in serial,
* but the contract requires op to be associative (allowing reordering
* in a parallel implementation).
*
* overloads:
*   inclusive_scan(f, l, d)
*   inclusive_scan(f, l, d, op)
*   inclusive_scan(f, l, d, op, init)
*
* return value: iterator past the last destination written.
*
*
* path:      /inc/djinterp/re_std/numeric/inclusive_scan.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.09
***********************************************************************/

#ifndef DJINTERP_RE_STD_NUMERIC_INCLUSIVE_SCAN_
#define DJINTERP_RE_STD_NUMERIC_INCLUSIVE_SCAN_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/iterator_traits.hpp"
    #include "re_std/utility/move.hpp"


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace re_std
{

// Default-op (operator+).
template<typename _InputIt, typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt inclusive_scan
(
    _InputIt   _first,
    _InputIt   _last,
    _OutputIt  _d_first
)
{
    if (_first == _last) return _d_first;
    typedef typename iterator_traits<_InputIt>::value_type _T;
    _T _acc = *_first;
    *_d_first = _acc;
    for (++_first, (void)++_d_first; _first != _last;
         ++_first, (void)++_d_first)
    {
        _acc = re_std::move(_acc) + *_first;
        *_d_first = _acc;
    }
    return _d_first;
}

// Custom-op without explicit init.
template<typename _InputIt, typename _OutputIt, typename _BinOp>
D_CONSTEXPR_CPP14 _OutputIt inclusive_scan
(
    _InputIt   _first,
    _InputIt   _last,
    _OutputIt  _d_first,
    _BinOp     _op
)
{
    if (_first == _last) return _d_first;
    typedef typename iterator_traits<_InputIt>::value_type _T;
    _T _acc = *_first;
    *_d_first = _acc;
    for (++_first, (void)++_d_first; _first != _last;
         ++_first, (void)++_d_first)
    {
        _acc = _op(re_std::move(_acc), *_first);
        *_d_first = _acc;
    }
    return _d_first;
}

// Custom-op with explicit init.
//   d[0] = op(init, src[0])
//   d[i] = op(d[i-1], src[i])
template<typename _InputIt, typename _OutputIt, typename _BinOp, typename _T>
D_CONSTEXPR_CPP14 _OutputIt inclusive_scan
(
    _InputIt   _first,
    _InputIt   _last,
    _OutputIt  _d_first,
    _BinOp     _op,
    _T         _init
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        _init = _op(re_std::move(_init), *_first);
        *_d_first = _init;
    }
    return _d_first;
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_NUMERIC_INCLUSIVE_SCAN_
