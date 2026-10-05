/*******************************************************************************
* djinterp [re_std]                                               accumulate.hpp
*
* accumulate algorithm header:
* accumulate(_first, _last, _init [, _op]) folds [_first, _last) into
* the accumulator _init via _op (default: operator+). Strict left-fold
* semantics: the operations are applied in iteration order.
*
*   accumulate({ 1, 2, 3, 4 }, 0)       == 10
*   accumulate({ 1, 2, 3, 4 }, 1, *)    == 24    // factorial-like
*
* contrast with reduce(): accumulate is GUARANTEED to be a left-fold
* in iteration order. reduce() may reorder evaluations and so requires
* an associative-and-commutative op.
*
* added in std C++98; constexpr in C++20. re_std back-ports the
* constexpr to C++14+ on every tier.
*
*
* path:      /inc/re_std/numeric/accumulate.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_ACCUMULATE_HPP
#define RE_STD_NUMERIC_ACCUMULATE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    #include "re_std/utility/move.hpp"
#endif


namespace re_std
{

// Default-op (operator+) overload.
template<typename InputIt, typename T>
RE_STD_CONSTEXPR_CPP14 T accumulate
(
    InputIt _first,
    InputIt _last,
    T       _init
)
{
    for (; _first != _last; ++_first)
    {
        #if RE_STD_LANG_IS_CPP11_OR_HIGHER
            // Move the running total through each step so user-defined
            // T types with non-trivial copy can move-fold.
            _init = re_std::move(_init) + *_first;
        #else
            _init = _init + *_first;
        #endif
    }
    return _init;
}

// Custom-op overload.
template<typename InputIt, typename T, typename BinOp>
RE_STD_CONSTEXPR_CPP14 T accumulate
(
    InputIt _first,
    InputIt _last,
    T       _init,
    BinOp   _op
)
{
    for (; _first != _last; ++_first)
    {
        #if RE_STD_LANG_IS_CPP11_OR_HIGHER
            _init = _op(re_std::move(_init), *_first);
        #else
            _init = _op(_init, *_first);
        #endif
    }
    return _init;
}


}  // re_std
#endif  // RE_STD_NUMERIC_ACCUMULATE_HPP
