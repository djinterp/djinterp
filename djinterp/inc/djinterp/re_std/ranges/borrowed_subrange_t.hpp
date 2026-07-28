/******************************************************************************
* djinterp [restd]                                     borrowed_subrange_t.hpp
*
* borrowed_subrange_t alias template header:
*   Yields subrange<iterator_t<_Range>> when _Range is a
* borrowed_range, and dangling otherwise. The companion of
* borrowed_iterator_t for algorithms that return a subrange rather
* than a single iterator.
*
*   PORTABILITY:
*   Requires alias templates, decltype, conditional. Available C++11+.
*
*
* path:      /inc/djinterp/restd/ranges/borrowed_subrange_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_BORROWED_SUBRANGE_T_
#define DJINTERP_RESTD_RANGES_BORROWED_SUBRANGE_T_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./iterator_t.hpp"
#include "./dangling.hpp"
#include "./subrange.hpp"
#include "./enable_borrowed_range.hpp"


NS_RESTD


// ===========================================================================
// I.   BORROWED_SUBRANGE_T
// ===========================================================================

// borrowed_subrange_t
//   alias: subrange<iterator_t<_Range>> when _Range is an lvalue
// reference OR enable_borrowed_range is true for the (cv- and
// ref-stripped) value type; dangling otherwise.
template<typename _Range>
using borrowed_subrange_t =
    typename conditional<
        is_reference<_Range>::value
            || enable_borrowed_range<
                   typename remove_cv<
                       typename remove_reference<_Range>::type
                   >::type
               >::value,
        subrange<iterator_t<typename remove_reference<_Range>::type> >,
        dangling
    >::type;


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_BORROWED_SUBRANGE_T_
