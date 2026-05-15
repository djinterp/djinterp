/******************************************************************************
* djinterp [restd]                                                 transform.hpp
*
* transform algorithm header:
*   Applies an operation to one or two ranges and writes the results
* to an output range. Two overloads:
*   - unary:  d_first[i] = op(first[i])
*   - binary: d_first[i] = op(first1[i], first2[i])
*
*   PORTABILITY:
*   - std::transform is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/restd/algorithm/transform.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_TRANSFORM_
#define DJINTERP_RESTD_ALGORITHM_TRANSFORM_ 1

#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   TRANSFORM (UNARY)
// ===========================================================================

// transform (unary)
//   function: writes _op(*it) to *d_first for each it in
// [_first, _last). Returns the iterator one past the last element
// written. _op is allowed to be the identity, making this equivalent
// to copy. _d_first may be equal to _first (in-place transform).
template<typename _InputIt,
         typename _OutputIt,
         typename _UnaryOp>
D_CONSTEXPR_CPP14 _OutputIt
transform(
    _InputIt  _first,
    _InputIt  _last,
    _OutputIt _d_first,
    _UnaryOp  _op
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        *_d_first = _op(*_first);
    }

    return _d_first;
}


// ===========================================================================
// II.  TRANSFORM (BINARY)
// ===========================================================================

// transform (binary)
//   function: writes _op(*it1, *it2) to *d_first for parallel
// iterators over [_first1, _last1) and the range starting at _first2.
// The second range is assumed long enough. Returns the iterator one
// past the last element written.
template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt,
         typename _BinaryOp>
D_CONSTEXPR_CPP14 _OutputIt
transform(
    _InputIt1 _first1,
    _InputIt1 _last1,
    _InputIt2 _first2,
    _OutputIt _d_first,
    _BinaryOp _op
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2, (void)++_d_first)
    {
        *_d_first = _op(*_first1, *_first2);
    }

    return _d_first;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_TRANSFORM_
