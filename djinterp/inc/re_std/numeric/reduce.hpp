/*******************************************************************************
* djinterp [re_std]                                                   reduce.hpp
*
* reduce algorithm header:
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
* path:      /inc/re_std/numeric/reduce.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_REDUCE_HPP
#define RE_STD_NUMERIC_REDUCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/iterator_traits.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

// Most-general overload: explicit init + custom op.
template<typename InputIt, typename T, typename BinOp>
RE_STD_CONSTEXPR_CPP14 T reduce
(
    InputIt _first,
    InputIt _last,
    T       _init,
    BinOp   _op
)
{
    for (; _first != _last; ++_first)
    {
        _init = _op(re_std::move(_init), *_first);
    }
    return _init;
}

// Default-op overload: explicit init, op = operator+.
template<typename InputIt, typename T>
RE_STD_CONSTEXPR_CPP14 T reduce
(
    InputIt _first,
    InputIt _last,
    T       _init
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
template<typename InputIt>
RE_STD_CONSTEXPR_CPP14 typename iterator_traits<InputIt>::value_type
reduce
(
    InputIt _first,
    InputIt _last
)
{
    typedef typename iterator_traits<InputIt>::value_type T;
    return re_std::reduce(_first, _last, T());
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_NUMERIC_REDUCE_HPP
