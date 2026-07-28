/******************************************************************************
* djinterp [restd]                                     borrowed_iterator_t.hpp
*
* borrowed_iterator_t alias template header:
*   Yields iterator_t<_Range> when _Range is a borrowed_range (i.e.
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
* path:      /inc/djinterp/restd/ranges/borrowed_iterator_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_BORROWED_ITERATOR_T_
#define DJINTERP_RESTD_RANGES_BORROWED_ITERATOR_T_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./iterator_t.hpp"
#include "./dangling.hpp"
#include "./enable_borrowed_range.hpp"


NS_RESTD


// ===========================================================================
// I.   BORROWED_ITERATOR_T
// ===========================================================================

// borrowed_iterator_t
//   alias: iterator_t<_Range> when _Range is an lvalue reference
// OR enable_borrowed_range is specialised true for the
// (cv-stripped, ref-stripped) value type; otherwise dangling.
// note: the lvalue-reference branch is detected via is_reference.
// The C++20 standard expresses this via the borrowed_range concept;
// restd unfolds it manually.
template<typename _Range>
using borrowed_iterator_t =
    typename conditional<
        is_reference<_Range>::value
            || enable_borrowed_range<
                   typename remove_cv<
                       typename remove_reference<_Range>::type
                   >::type
               >::value,
        iterator_t<typename remove_reference<_Range>::type>,
        dangling
    >::type;


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_BORROWED_ITERATOR_T_
