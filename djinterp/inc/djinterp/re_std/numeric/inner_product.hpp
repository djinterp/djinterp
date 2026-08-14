/***********************************************************************
* restd                                                     inner_product.hpp
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
* added in std C++98; constexpr in C++20. restd back-ports constexpr
* to C++14+ where the loop body is permitted.
*
*
* path:      /inc/djinterp/re_std/numeric/inner_product.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.09
***********************************************************************/

#ifndef RESTD_NUMERIC_INNER_PRODUCT_
#define RESTD_NUMERIC_INNER_PRODUCT_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include "restd/utility/move.hpp"
#endif


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace restd
{

// Default-op overload (operator+ / operator*).
template<typename _InputIt1, typename _InputIt2, typename _T>
D_CONSTEXPR_CPP14 _T inner_product
(
    _InputIt1 _first1,
    _InputIt1 _last1,
    _InputIt2 _first2,
    _T        _init
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            _init = restd::move(_init) + (*_first1 * *_first2);
        #else
            _init = _init + (*_first1 * *_first2);
        #endif
    }
    return _init;
}

// Custom-op overload.
template<typename _InputIt1, typename _InputIt2, typename _T,
         typename _BinOp1, typename _BinOp2>
D_CONSTEXPR_CPP14 _T inner_product
(
    _InputIt1 _first1,
    _InputIt1 _last1,
    _InputIt2 _first2,
    _T        _init,
    _BinOp1   _op1,
    _BinOp2   _op2
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            _init = _op1(restd::move(_init), _op2(*_first1, *_first2));
        #else
            _init = _op1(_init, _op2(*_first1, *_first2));
        #endif
    }
    return _init;
}


}  // namespace restd

#endif  // RESTD_NUMERIC_INNER_PRODUCT_
