/*******************************************************************************
* djinterp [re_std]                                  range_const_reference_t.hpp
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
* path:      /inc/re_std/ranges/range_const_reference_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_RANGE_CONST_REFERENCE_T_HPP
#define RE_STD_RANGES_RANGE_CONST_REFERENCE_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/basic_const_iterator.hpp"
#include "./iterator_t.hpp"


namespace re_std
{


// range_const_reference<R>
//   trait: iter_const_reference<iterator_t<R>>.
template<typename R>
struct range_const_reference
{
    typedef typename iter_const_reference<iterator_t<R> >::type type;
};

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
template<typename R>
using range_const_reference_t = typename range_const_reference<R>::type;
#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_RANGE_CONST_REFERENCE_T_HPP
