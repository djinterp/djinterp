/***********************************************************************
* restd                                                  to_underlying.hpp
*
* to_underlying(_e):
*   Returns _e cast to its underlying integer type. Eliminates the
* repeated boilerplate `static_cast<typename underlying_type<E>::type>`
* in code that needs to work with the integer view of an enum.
*
* added in std C++23. restd back-ports unconditionally on C++11+
* (where __underlying_type is available; the trait itself is gated).
*
* dependency: restd::underlying_type, which itself requires the
* __underlying_type intrinsic. If absent (D_RESTD_HAS_UNDERLYING_TYPE
* = 0), this header degrades to a no-op via the same gating that
* protects underlying_type's consumers.
*
*
* path:      /inc/restd/utility/to_underlying.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.05.09
***********************************************************************/

#ifndef RESTD_UTILITY_TO_UNDERLYING_
#define RESTD_UTILITY_TO_UNDERLYING_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER && D_RESTD_HAS_UNDERLYING_TYPE

    #include "../type_traits/underlying_type.hpp"


namespace restd
{

template<typename _Enum>
D_CONSTEXPR typename underlying_type<_Enum>::type
to_underlying(_Enum _e) D_NOEXCEPT
{
    return static_cast<typename underlying_type<_Enum>::type>(_e);
}


}  // namespace restd

#endif  // C++11+ && D_RESTD_HAS_UNDERLYING_TYPE

#endif  // RESTD_UTILITY_TO_UNDERLYING_
