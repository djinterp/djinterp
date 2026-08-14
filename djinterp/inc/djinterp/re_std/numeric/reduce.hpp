/***********************************************************************
* restd                                                            reduce.hpp
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
*   restd's implementation is currently a SERIAL left-to-right fold,
*   identical in result to accumulate(). The interface still requires
*   the assoc + commut contract from the user, so when restd grows
*   parallel-execution support, calling code does not need to change.
*
* added in std C++17.
*
*
* path:      /inc/djinterp/re_std/numeric/reduce.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.09
***********************************************************************/

#ifndef RESTD_NUMERIC_REDUCE_
#define RESTD_NUMERIC_REDUCE_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/iterator/iterator_traits.hpp"
    #include "restd/utility/move.hpp"


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace restd
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
        _init = _op(restd::move(_init), *_first);
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
        _init = restd::move(_init) + *_first;
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
    return restd::reduce(_first, _last, _T());
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_NUMERIC_REDUCE_
