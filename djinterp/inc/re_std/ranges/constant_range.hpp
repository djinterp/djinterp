/*******************************************************************************
* djinterp [re_std]                                           constant_range.hpp
*
* constant_range header:
*   Provides the C++23 constant_range concept as a SFINAE trait.
* A constant_range is an input_range whose reference type is the
* same as its const-projected reference type — i.e. its elements
* are already exposed as const, so as_const_view over it is a
* no-op semantically.
*
*   C++23 spec: constant_range<R> = input_range<R> AND
*               same_as<range_reference_t<R>, range_const_reference_t<R>>.
*
*   PORTABILITY:
*   - C++11+; depends on input_range (Phase R2), range_reference_t
*     (Phase R1), range_const_reference_t (Phase R23).
*
*
* path:      /inc/re_std/ranges/constant_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_CONSTANT_RANGE_HPP
#define RE_STD_RANGES_CONSTANT_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./input_range.hpp"
#include "./range_reference_t.hpp"
#include "./range_const_reference_t.hpp"


namespace re_std
{


// constant_range<R>
//   trait: true iff R is an input_range AND its reference type
// already equals its const-projected reference type.
template<typename R, typename = void>
struct constant_range : false_type
{};

template<typename R>
struct constant_range<
    R,
    typename enable_if<
        input_range<R>::value
        && is_same<
               range_reference_t<R>,
               range_const_reference_t<R>
           >::value,
        void
    >::type
> : true_type
{};


#if RE_STD_LANG_IS_CPP14_OR_HIGHER
template<typename R>
RE_STD_CONSTEXPR bool constant_range_v = constant_range<R>::value;
#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_CONSTANT_RANGE_HPP
