/******************************************************************************
* djinterp [restd]                                  range_const_reference_t.hpp
*
* range_const_reference_t header:
*   Provides the C++23 range_const_reference_t<R> alias —
* the const-projected reference type of a range's elements as
* produced by basic_const_iterator over its iterators. Trivially
* iter_const_reference_t<iterator_t<R>>.
*
*   PORTABILITY:
*   - C++11+; depends on iterator_t (Phase R1) and
*     iter_const_reference_t (Phase R22).
*
*
* path:      /inc/djinterp/restd/ranges/range_const_reference_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_RANGE_CONST_REFERENCE_T_
#define DJINTERP_RESTD_RANGES_RANGE_CONST_REFERENCE_T_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/basic_const_iterator.hpp"
#include "./iterator_t.hpp"


NS_RESTD


// range_const_reference<_R>
//   trait: iter_const_reference<iterator_t<_R>>.
template<typename _R>
struct range_const_reference
{
    typedef typename iter_const_reference<iterator_t<_R> >::type type;
};

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename _R>
using range_const_reference_t = typename range_const_reference<_R>::type;
#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_RANGE_CONST_REFERENCE_T_
