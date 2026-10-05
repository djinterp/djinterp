/*******************************************************************************
* djinterp [re_std]                                            inner_product.hpp
*
* inner_product(_first1, _last1, _first2, _init [, _op1, _op2])
* computes the generalized inner product of two ranges:
*
*   for each i in [0, last1 - first1):
*     _init = _op1(_init, _op2(*_first1, *_first2));
*     advance both inputs;
*
* default: _op1 = operator+, _op2 = operator*  (standard dot product).
*
* added in std C++98; constexpr in C++20. re_std back-ports constexpr
* to C++14+ where the loop body is permitted.
*
*
* path:      /inc/re_std/numeric/inner_product.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_INNER_PRODUCT_HPP
#define RE_STD_NUMERIC_INNER_PRODUCT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    #include "re_std/utility/move.hpp"
#endif


namespace re_std
{

// Default-op overload (operator+ / operator*).
template<typename InputIt1, typename InputIt2, typename T>
RE_STD_CONSTEXPR_CPP14 T inner_product
(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    T        _init
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        #if RE_STD_LANG_IS_CPP11_OR_HIGHER
            _init = re_std::move(_init) + (*_first1 * *_first2);
        #else
            _init = _init + (*_first1 * *_first2);
        #endif
    }
    return _init;
}

// Custom-op overload.
template<typename InputIt1, typename InputIt2, typename T,
         typename BinOp1, typename BinOp2>
RE_STD_CONSTEXPR_CPP14 T inner_product
(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    T        _init,
    BinOp1   _op1,
    BinOp2   _op2
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        #if RE_STD_LANG_IS_CPP11_OR_HIGHER
            _init = _op1(re_std::move(_init), _op2(*_first1, *_first2));
        #else
            _init = _op1(_init, _op2(*_first1, *_first2));
        #endif
    }
    return _init;
}


}  // re_std
#endif  // RE_STD_NUMERIC_INNER_PRODUCT_HPP
