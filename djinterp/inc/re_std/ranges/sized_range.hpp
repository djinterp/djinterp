/*******************************************************************************
* djinterp [re_std]                                              sized_range.hpp
*
* sized_range concept-trait header:
*   Provides the C++20 sized_range concept as a SFINAE-detection
* trait. sized_range<T>::value is true iff range<T> AND re_std::size
* is well-formed when applied to an lvalue of type T.
*
*   PORTABILITY:
*   C++11+ (via range + decltype detection on re_std::size).
* C++14+ variable spelling sized_range_v<T>.
*
*
* path:      /inc/re_std/ranges/sized_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_SIZED_RANGE_HPP
#define RE_STD_RANGES_SIZED_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "../utility/declval.hpp"
#include "../iterator/size.hpp"
#include "./range.hpp"


namespace re_std
{


namespace internal
{

// has_size
//   trait: SFINAE on re_std::size(declval<T&>()).
template<typename Type,
         typename = void>
struct has_size
    : false_type
{};

template<typename Type>
struct has_size<Type,
                void_t<decltype(re_std::size(declval<Type&>()))> >
    : true_type
{};

}  // internal


// ===========================================================================
// I.   SIZED_RANGE
// ===========================================================================

// sized_range
//   trait: range whose ranges::size is well-formed. Matches the
// C++20 ranges::sized_range concept.
template<typename Type>
struct sized_range
    : integral_constant<bool,
                        range<Type>::value &&
                        internal::has_size<Type>::value>
{};


// ===========================================================================
// II.  SIZED_RANGE_V
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool sized_range_v = sized_range<Type>::value;

#endif


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_SIZED_RANGE_HPP
