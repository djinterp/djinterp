/*******************************************************************************
* djinterp [re_std]                                      borrowed_iterator_t.hpp
*
* borrowed_iterator_t alias template header:
*   Yields iterator_t<Range> when Range is a borrowed_range (i.e.
* an lvalue range OR an rvalue range whose enable_borrowed_range is
* true), and the dangling sentinel type otherwise. Used by range
* algorithms to surface a dangling-iterator at compile time when the
* algorithm's source is an rvalue temporary that would invalidate
* its iterators on return.
*
*   PORTABILITY:
*   Requires alias templates, decltype, conditional. Available C++11+.
*
*
* path:      /inc/re_std/ranges/borrowed_iterator_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_BORROWED_ITERATOR_T_HPP
#define RE_STD_RANGES_BORROWED_ITERATOR_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./iterator_t.hpp"
#include "./dangling.hpp"
#include "./enable_borrowed_range.hpp"


namespace re_std
{


// ===========================================================================
// I.   BORROWED_ITERATOR_T
// ===========================================================================

// borrowed_iterator_t
//   alias: iterator_t<Range> when Range is an lvalue reference
// OR enable_borrowed_range is specialised true for the
// (cv-stripped, ref-stripped) value type; otherwise dangling.
// note: the lvalue-reference branch is detected via is_reference.
// The C++20 standard expresses this via the borrowed_range concept;
// re_std unfolds it manually.
template<typename Range>
using borrowed_iterator_t =
    typename conditional<
        is_reference<Range>::value
            || enable_borrowed_range<
                   typename remove_cv<
                       typename remove_reference<Range>::type
                   >::type
               >::value,
        iterator_t<typename remove_reference<Range>::type>,
        dangling
    >::type;


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_BORROWED_ITERATOR_T_HPP
