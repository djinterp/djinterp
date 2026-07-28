/***********************************************************************
* restd                                                 to_underlying.hpp
*
* enum-to-underlying-type cast:
*   Yields the underlying integral value of an enumeration. Equivalent
* to static_cast<underlying_type<E>::type>(e), but spelled out so that
* the cast is unambiguous in generic code.
*
*   STANDARD STATUS:
*   Introduced in C++23 (P1682R3). restd back-ports to C++11+ where the
* underlying_type intrinsic is available (gated on
* D_RESTD_HAS_UNDERLYING_TYPE).
*
*   GATING:
*   When D_RESTD_HAS_UNDERLYING_TYPE is 0 (no compiler intrinsic), the
* function is not defined at all. Callers should gate their use on
* the same macro -- this matches the policy used by underlying_type
* itself.
*
*
* path:      /inc/restd/utility/to_underlying.hpp
* link(s):   TBA
* author(s): restd team                                  date: 2026.05.02
***********************************************************************/

#ifndef RESTD_UTILITY_TO_UNDERLYING_
#define RESTD_UTILITY_TO_UNDERLYING_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/underlying_type.hpp"

#if D_RESTD_HAS_UNDERLYING_TYPE

NS_RESTD

// =============================================================================
// TO_UNDERLYING
// =============================================================================

// to_underlying
//   function: casts an enumeration value to its underlying integral
//   representation. Single-statement; constexpr-eligible from C++11.
template<typename _Enum>
D_CONSTEXPR
typename underlying_type<_Enum>::type
to_underlying(_Enum _e) noexcept
{
    return static_cast<typename underlying_type<_Enum>::type>(_e);
}

NS_END  // restd

#endif  // D_RESTD_HAS_UNDERLYING_TYPE

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_UTILITY_TO_UNDERLYING_
