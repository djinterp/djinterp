/*******************************************************************************
* djinterp [re_std]                                 range_rvalue_reference_t.hpp
*
* range_rvalue_reference_t header:
*   Provides the C++20 range_rvalue_reference_t<R> alias —
* the rvalue-reference projection of a range's element type as
* produced by re_std::iter_move on its iterators. Trivially
* iter_rvalue_reference_t<iterator_t<R>>.
*
*   PORTABILITY:
*   - C++11+; depends on iterator_t (Phase R1) and
*     iter_rvalue_reference_t (Phase R22).
*
*
* path:      /inc/re_std/ranges/range_rvalue_reference_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_RANGE_RVALUE_REFERENCE_T_HPP
#define RE_STD_RANGES_RANGE_RVALUE_REFERENCE_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/iter_move.hpp"
#include "./iterator_t.hpp"


namespace re_std
{


// range_rvalue_reference<R>
//   trait: iter_rvalue_reference<iterator_t<R>>.
template<typename R>
struct range_rvalue_reference
{
    typedef typename iter_rvalue_reference<iterator_t<R> >::type type;
};

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
template<typename R>
using range_rvalue_reference_t = typename range_rvalue_reference<R>::type;
#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_RANGE_RVALUE_REFERENCE_T_HPP
