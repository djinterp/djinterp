/******************************************************************************
* djinterp [restd]                                                variant_size.hpp
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
* path:      /inc/djinterp/restd/variant/variant_size.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_VARIANT_SIZE_
#define DJINTERP_RESTD_VARIANT_SIZE_ 1

#include <cstddef>
#include "../../core/djinterp.hpp"
#include "../type_traits/integral_constant.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_RESTD


// Forward-declare variant so variant_size can specialise on it
// without including the (much larger) variant.hpp.
template<typename... _Types> class variant;


// ===========================================================================
// I.   VARIANT_SIZE — primary template
// ===========================================================================

// Primary template: not defined. Specialisations below handle the
// supported cases (variant and its cv-qualified forms). Missing
// specialisations produce a compile error at instantiation —
// matches std.
template<typename _V>
struct variant_size;


// ===========================================================================
// II.  SPECIALISATION FOR VARIANT
// ===========================================================================

template<typename... _Types>
struct variant_size<variant<_Types...> >
    : restd::integral_constant<std::size_t, sizeof...(_Types)>
{};


// ===========================================================================
// III. CV-QUALIFIED PASSTHROUGH (LWG-style)
// ===========================================================================

template<typename _V>
struct variant_size<_V const>
    : restd::integral_constant<std::size_t, variant_size<_V>::value>
{};

template<typename _V>
struct variant_size<_V volatile>
    : restd::integral_constant<std::size_t, variant_size<_V>::value>
{};

template<typename _V>
struct variant_size<_V const volatile>
    : restd::integral_constant<std::size_t, variant_size<_V>::value>
{};


// ===========================================================================
// IV.  VARIANT_SIZE_V (C++14+)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _V>
D_CONSTEXPR std::size_t variant_size_v = variant_size<_V>::value;

#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_VARIANT_SIZE_
