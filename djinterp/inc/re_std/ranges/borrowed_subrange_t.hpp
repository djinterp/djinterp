/*******************************************************************************
* djinterp [re_std]                                      borrowed_subrange_t.hpp
*
* borrowed_subrange_t alias template header:
*   Yields subrange<iterator_t<Range>> when Range is a
* borrowed_range, and dangling otherwise. The companion of
* borrowed_iterator_t for algorithms that return a subrange rather
* than a single iterator.
*
*   PORTABILITY:
*   Requires alias templates, decltype, conditional. Available C++11+.
*
*
* path:      /inc/re_std/ranges/borrowed_subrange_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_BORROWED_SUBRANGE_T_HPP
#define RE_STD_RANGES_BORROWED_SUBRANGE_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./iterator_t.hpp"
#include "./dangling.hpp"
#include "./subrange.hpp"
#include "./enable_borrowed_range.hpp"


namespace re_std
{


// ===========================================================================
// I.   BORROWED_SUBRANGE_T
// ===========================================================================

// borrowed_subrange_t
//   alias: subrange<iterator_t<Range>> when Range is an lvalue
// reference OR enable_borrowed_range is true for the (cv- and
// ref-stripped) value type; dangling otherwise.
template<typename Range>
using borrowed_subrange_t =
    typename conditional<
        is_reference<Range>::value
            || enable_borrowed_range<
                   typename remove_cv<
                       typename remove_reference<Range>::type
                   >::type
               >::value,
        subrange<iterator_t<typename remove_reference<Range>::type> >,
        dangling
    >::type;


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_BORROWED_SUBRANGE_T_HPP
