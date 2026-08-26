/******************************************************************************
* djinterp [re_std]                                           output_range.hpp
*
* output_range header:
*   Provides the C++20 output_range concept as a SFINAE trait. The
* full concept is output_range<R, T> = range<R> AND
* output_iterator<iterator_t<R>, T>. Since the C++20 output_iterator
* concept itself decomposes into input_or_output_iterator +
* indirectly_writable, and the latter is "can be assigned via
* *it = t", re_std checks the assignment expression directly.
*
*   The two-parameter form is awkward in the trait-struct style
* compared to the one-parameter range concepts shipped in Phase R2,
* but mechanically identical: a void_t-based partial specialisation
* switches on when both conditions are met. The output_range_v
* variable template is C++14+ as usual.
*
*   PORTABILITY:
*   - C++11+; depends on range<R> (Phase R2) and iterator_t<R>
*     (Phase R1).
*   - The writability check uses void_t<decltype(*it = t)>. Mirrors
*     the indirectly_writable C++20 specification in its simplest
*     form (the spec also requires *it++ = t etc., but the basic
*     form catches the cases that matter).
*
*
* path:      /inc/djinterp/re_std/ranges/output_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_OUTPUT_RANGE_
#define DJINTERP_RE_STD_RANGES_OUTPUT_RANGE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./range.hpp"
#include "./iterator_t.hpp"


NS_RESTD


// ===========================================================================
// I.   INDIRECTLY_WRITABLE-LIKE DETECTOR (internal)
// ===========================================================================

NS_INTERNAL

// output_assignable
//   trait: detects whether *declval<_I&>() = declval<_T&&>() is a
// valid expression. Mirrors a simplified form of C++20's
// indirectly_writable concept.
template<typename _I, typename _T, typename = void>
struct output_assignable : false_type
{};

template<typename _I, typename _T>
struct output_assignable<
    _I, _T,
    typename void_t<
        decltype(*declval<_I&>() = declval<_T&&>())
    >::type
> : true_type
{};

NS_END  // internal


// ===========================================================================
// II.  OUTPUT_RANGE
// ===========================================================================

// output_range<_R, _T>
//   trait: true iff _R is a range AND its iterator type supports
// the assignment *it = _T-value.
template<typename _R, typename _T, typename = void>
struct output_range : false_type
{};

template<typename _R, typename _T>
struct output_range<
    _R, _T,
    typename enable_if<
        range<_R>::value
        && internal::output_assignable<
               iterator_t<_R>,
               _T
           >::value,
        void
    >::type
> : true_type
{};


#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _R, typename _T>
D_CONSTEXPR bool output_range_v = output_range<_R, _T>::value;
#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RANGES_OUTPUT_RANGE_
