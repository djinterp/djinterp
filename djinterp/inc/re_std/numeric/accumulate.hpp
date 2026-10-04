/***********************************************************************
* re_std                                                       accumulate.hpp
*
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
* path:      /inc/djinterp/re_std/numeric/accumulate.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.09
***********************************************************************/

#ifndef DJINTERP_RE_STD_NUMERIC_ACCUMULATE_
#define DJINTERP_RE_STD_NUMERIC_ACCUMULATE_ 1

#include "djinterp.hpp"


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

// Default-op (operator+) overload.
template<typename _InputIt, typename _T>
D_CONSTEXPR_CPP14 _T accumulate
(
    _InputIt _first,
    _InputIt _last,
    _T       _init
)
{
    for (; _first != _last; ++_first)
    {
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            // Move the running total through each step so user-defined
            // _T types with non-trivial copy can move-fold.
            _init = re_std::move(_init) + *_first;
        #else
            _init = _init + *_first;
        #endif
    }
    return _init;
}

// Custom-op overload.
template<typename _InputIt, typename _T, typename _BinOp>
D_CONSTEXPR_CPP14 _T accumulate
(
    _InputIt _first,
    _InputIt _last,
    _T       _init,
    _BinOp   _op
)
{
    for (; _first != _last; ++_first)
    {
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            _init = _op(re_std::move(_init), *_first);
        #else
            _init = _op(_init, *_first);
        #endif
    }
    return _init;
}


}  // namespace re_std

#endif  // DJINTERP_RE_STD_NUMERIC_ACCUMULATE_
