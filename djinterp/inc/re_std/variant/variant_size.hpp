/*******************************************************************************
* djinterp [re_std]                                             variant_size.hpp
*
* variant_size trait header:
*   variant_size<V> yields the number of alternatives in V as a
* compile-time size_t.
*
*     variant_size<variant<int, double, string>>::value == 3
*
*   Cv-qualified specialisations forward to the unqualified primary.
*
*
* path:      /inc/re_std/variant/variant_size.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_VARIANT_VARIANT_SIZE_HPP
#define RE_STD_VARIANT_VARIANT_SIZE_HPP 1

// std
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "../type_traits/integral_constant.hpp"

#if RE_STD_LANG_IS_CPP11_OR_HIGHER


namespace re_std
{


// Forward-declare variant so variant_size can specialise on it
// without including the (much larger) variant.hpp.
template<typename... Types> class variant;


// ===========================================================================
// I.   VARIANT_SIZE — primary template
// ===========================================================================

// Primary template: not defined. Specialisations below handle the
// supported cases (variant and its cv-qualified forms). Missing
// specialisations produce a compile error at instantiation —
// matches std.
template<typename V>
struct variant_size;


// ===========================================================================
// II.  SPECIALISATION FOR VARIANT
// ===========================================================================

template<typename... Types>
struct variant_size<variant<Types...> >
    : re_std::integral_constant<std::size_t, sizeof...(Types)>
{};


// ===========================================================================
// III. CV-QUALIFIED PASSTHROUGH (LWG-style)
// ===========================================================================

template<typename V>
struct variant_size<V const>
    : re_std::integral_constant<std::size_t, variant_size<V>::value>
{};

template<typename V>
struct variant_size<V volatile>
    : re_std::integral_constant<std::size_t, variant_size<V>::value>
{};

template<typename V>
struct variant_size<V const volatile>
    : re_std::integral_constant<std::size_t, variant_size<V>::value>
{};


// ===========================================================================
// IV.  VARIANT_SIZE_V (C++14+)
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename V>
RE_STD_CONSTEXPR std::size_t variant_size_v = variant_size<V>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_VARIANT_SIZE_HPP
