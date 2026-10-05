/*******************************************************************************
* djinterp [re_std]                                            to_underlying.hpp
*
* enum-to-underlying-type cast:
*   Yields the underlying integral value of an enumeration. Equivalent
* to static_cast<underlying_type<E>::type>(e), but spelled out so that
* the cast is unambiguous in generic code.
*
*   STANDARD STATUS:
*   Introduced in C++23 (P1682R3). re_std back-ports to C++11+ where the
* underlying_type intrinsic is available (gated on
* RE_STD_HAS_UNDERLYING_TYPE).
*
*   GATING:
*   When RE_STD_HAS_UNDERLYING_TYPE is 0 (no compiler intrinsic), the
* function is not defined at all. Callers should gate their use on
* the same macro -- this matches the policy used by underlying_type
* itself.
*
*
* path:      /inc/re_std/utility/to_underlying.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_TO_UNDERLYING_HPP
#define RE_STD_UTILITY_TO_UNDERLYING_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/underlying_type.hpp"

#if RE_STD_HAS_UNDERLYING_TYPE

namespace re_std
{

// =============================================================================
// TO_UNDERLYING
// =============================================================================

// to_underlying
//   function: casts an enumeration value to its underlying integral
//   representation. Single-statement; constexpr-eligible from C++11.
template<typename Enum>
RE_STD_CONSTEXPR
typename underlying_type<Enum>::type
to_underlying(Enum _e) noexcept
{
    return static_cast<typename underlying_type<Enum>::type>(_e);
}

}  // re_std

#endif  // RE_STD_HAS_UNDERLYING_TYPE

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_UTILITY_TO_UNDERLYING_HPP
