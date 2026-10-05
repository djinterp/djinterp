/*******************************************************************************
* djinterp [re_std]                                                transform.hpp
*
* transform algorithm header:
*   Applies an operation to one or two ranges and writes the results
* to an output range. Two overloads:
*   - unary:  d_first[i] = op(first[i])
*   - binary: d_first[i] = op(first1[i], first2[i])
*
*   PORTABILITY:
*   - std::transform is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/transform.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_TRANSFORM_HPP
#define RE_STD_ALGORITHM_TRANSFORM_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   TRANSFORM (UNARY)
// ===========================================================================

// transform (unary)
//   function: writes _op(*it) to *d_first for each it in
// [_first, _last). Returns the iterator one past the last element
// written. _op is allowed to be the identity, making this equivalent
// to copy. _d_first may be equal to _first (in-place transform).
template<typename InputIt,
         typename OutputIt,
         typename UnaryOp>
RE_STD_CONSTEXPR_CPP14 OutputIt
transform(
    InputIt  _first,
    InputIt  _last,
    OutputIt _d_first,
    UnaryOp  _op
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
template<typename InputIt1,
         typename InputIt2,
         typename OutputIt,
         typename BinaryOp>
RE_STD_CONSTEXPR_CPP14 OutputIt
transform(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    OutputIt _d_first,
    BinaryOp _op
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2, (void)++_d_first)
    {
        *_d_first = _op(*_first1, *_first2);
    }

    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_TRANSFORM_HPP
