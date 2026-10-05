/*******************************************************************************
* djinterp [re_std]                                             output_range.hpp
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
* path:      /inc/re_std/ranges/output_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_OUTPUT_RANGE_HPP
#define RE_STD_RANGES_OUTPUT_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./range.hpp"
#include "./iterator_t.hpp"


namespace re_std
{


// ===========================================================================
// I.   INDIRECTLY_WRITABLE-LIKE DETECTOR (internal)
// ===========================================================================

namespace internal
{

// output_assignable
//   trait: detects whether *declval<I&>() = declval<T&&>() is a
// valid expression. Mirrors a simplified form of C++20's
// indirectly_writable concept.
template<typename I, typename T, typename = void>
struct output_assignable : false_type
{};

template<typename I, typename T>
struct output_assignable<
    I, T,
    void_t<
        decltype(*declval<I&>() = declval<T&&>())
    >
> : true_type
{};

}  // internal


// ===========================================================================
// II.  OUTPUT_RANGE
// ===========================================================================

// output_range<R, T>
//   trait: true iff R is a range AND its iterator type supports
// the assignment *it = T-value.
template<typename R, typename T, typename = void>
struct output_range : false_type
{};

template<typename R, typename T>
struct output_range<
    R, T,
    typename enable_if<
        range<R>::value
        && internal::output_assignable<
               iterator_t<R>,
               T
           >::value,
        void
    >::type
> : true_type
{};


#if RE_STD_LANG_IS_CPP14_OR_HIGHER
template<typename R, typename T>
RE_STD_CONSTEXPR bool output_range_v = output_range<R, T>::value;
#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_OUTPUT_RANGE_HPP
