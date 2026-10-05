/*******************************************************************************
* djinterp [re_std]                                           inclusive_scan.hpp
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
* path:      /inc/re_std/numeric/inclusive_scan.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_INCLUSIVE_SCAN_HPP
#define RE_STD_NUMERIC_INCLUSIVE_SCAN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/iterator_traits.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

// Default-op (operator+).
template<typename InputIt, typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt inclusive_scan
(
    InputIt   _first,
    InputIt   _last,
    OutputIt  _d_first
)
{
    if (_first == _last) return _d_first;
    typedef typename iterator_traits<InputIt>::value_type T;
    T _acc = *_first;
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
template<typename InputIt, typename OutputIt, typename BinOp>
RE_STD_CONSTEXPR_CPP14 OutputIt inclusive_scan
(
    InputIt   _first,
    InputIt   _last,
    OutputIt  _d_first,
    BinOp     _op
)
{
    if (_first == _last) return _d_first;
    typedef typename iterator_traits<InputIt>::value_type T;
    T _acc = *_first;
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
template<typename InputIt, typename OutputIt, typename BinOp, typename T>
RE_STD_CONSTEXPR_CPP14 OutputIt inclusive_scan
(
    InputIt   _first,
    InputIt   _last,
    OutputIt  _d_first,
    BinOp     _op,
    T         _init
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        _init = _op(re_std::move(_init), *_first);
        *_d_first = _init;
    }
    return _d_first;
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_NUMERIC_INCLUSIVE_SCAN_HPP
