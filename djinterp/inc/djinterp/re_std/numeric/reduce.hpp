/***********************************************************************
* re_std                                                           reduce.hpp
*
* reduce([first, last [, init [, op]]]) is a generalised fold over a
* range. Unlike accumulate, the binary operation is REQUIRED to be
* associative AND commutative — the implementation is allowed to
* reorder evaluation, including in parallel.
*
* overloads:
*   reduce(first, last)              -> default init = T(), default op = +
*   reduce(first, last, init)        -> default op = +
*   reduce(first, last, init, op)
*
* IMPLEMENTATION NOTE:
*   re_std's implementation is currently a SERIAL left-to-right fold,
*   identical in result to accumulate(). The interface still requires
*   the assoc + commut contract from the user, so when re_std grows
*   parallel-execution support, calling code does not need to change.
*
* added in std C++17.
*
*
* path:      /inc/djinterp/re_std/numeric/reduce.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.09
***********************************************************************/

#ifndef DJINTERP_RE_STD_NUMERIC_REDUCE_
#define DJINTERP_RE_STD_NUMERIC_REDUCE_ 1

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

// Most-general overload: explicit init + custom op.
template<typename _InputIt, typename _T, typename _BinOp>
D_CONSTEXPR_CPP14 _T reduce
(
    _InputIt _first,
    _InputIt _last,
    _T       _init,
    _BinOp   _op
)
{
    for (; _first != _last; ++_first)
    {
        _init = _op(re_std::move(_init), *_first);
    }
    return _init;
}

// Default-op overload: explicit init, op = operator+.
template<typename _InputIt, typename _T>
D_CONSTEXPR_CPP14 _T reduce
(
    _InputIt _first,
    _InputIt _last,
    _T       _init
)
{
    for (; _first != _last; ++_first)
    {
        _init = re_std::move(_init) + *_first;
    }
    return _init;
}

// Default-init / default-op overload.
//   The standard says: T = iterator_traits<It>::value_type, init = T().
template<typename _InputIt>
D_CONSTEXPR_CPP14 typename iterator_traits<_InputIt>::value_type
reduce
(
    _InputIt _first,
    _InputIt _last
)
{
    typedef typename iterator_traits<_InputIt>::value_type _T;
    return re_std::reduce(_first, _last, _T());
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_NUMERIC_REDUCE_
