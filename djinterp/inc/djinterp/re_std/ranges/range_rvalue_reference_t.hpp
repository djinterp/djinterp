/******************************************************************************
* djinterp [restd]                                  range_rvalue_reference_t.hpp
*
* range_rvalue_reference_t header:
*   Provides the C++20 range_rvalue_reference_t<R> alias —
* the rvalue-reference projection of a range's element type as
* produced by restd::iter_move on its iterators. Trivially
* iter_rvalue_reference_t<iterator_t<R>>.
*
*   PORTABILITY:
*   - C++11+; depends on iterator_t (Phase R1) and
*     iter_rvalue_reference_t (Phase R22).
*
*
* path:      /inc/djinterp/re_std/ranges/range_rvalue_reference_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_RANGE_RVALUE_REFERENCE_T_
#define DJINTERP_RESTD_RANGES_RANGE_RVALUE_REFERENCE_T_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/iter_move.hpp"
#include "./iterator_t.hpp"


NS_RESTD


// range_rvalue_reference<_R>
//   trait: iter_rvalue_reference<iterator_t<_R>>.
template<typename _R>
struct range_rvalue_reference
{
    typedef typename iter_rvalue_reference<iterator_t<_R> >::type type;
};

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename _R>
using range_rvalue_reference_t = typename range_rvalue_reference<_R>::type;
#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_RANGE_RVALUE_REFERENCE_T_
