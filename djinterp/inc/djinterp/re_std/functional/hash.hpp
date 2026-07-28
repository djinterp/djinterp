/***********************************************************************
* restd                                                         hash.hpp
*
* class: customisation point for hashing values.
*   Provides the primary `restd::hash<_Type>` template plus
* specialisations for every scalar-like type the standard requires:
* every arithmetic type, every pointer type, and `nullptr_t`.
*
*   Specialisations whose key types live in other modules
* (`hash<basic_string<...>>`, `hash<unique_ptr<...>>`,
* `hash<optional<T>>`, etc.) ship alongside those modules.
*
*   The standard says the hash for an integer is "implementation
* defined". restd uses identity casts to `size_t` for integral types --
* this is the same choice libstdc++/libc++ make for short keys -- and
* a `reinterpret_cast`-based cast for pointers. For floating-point,
* the bytes are read into a `size_t` so that distinct bit patterns
* yield distinct hashes; `+0.0` and `-0.0` both normalise to a zero
* hash so equal-comparing values hash equally.
*
*
* path:      /inc/restd/functional/hash.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_HASH_
#define RESTD_FUNCTIONAL_HASH_ 1

#include "djinterp.hpp"

#include <cstddef>   // size_t, nullptr_t (gated below)
#include <cstring>   // memcpy for fp hashing

namespace restd
{

// hash
//   class: primary template -- left empty (no operator()), so attempts
// to hash an unsupported key type are ill-formed at instantiation.
template<typename _Type>
struct hash
{};

NS_INTERNAL

    // hash_integer_cast
    //   function: identity-cast hash for integers. Branchless and
    // trivially constexpr.
    template<typename _Type>
    D_CONSTEXPR std::size_t
    hash_integer_cast(
        _Type _v
    )
    {
        return static_cast<std::size_t>(_v);
    }

    // hash_floating
    //   function: bytewise hash for floating-point values. Normalises
    // -0.0 to +0.0 so that equal values hash equally. Not constexpr
    // (memcpy is not constexpr until C++20).
    template<typename _Type>
    inline std::size_t
    hash_floating(
        _Type _v
    )
    {
        // normalise signed zero
        if (_v == static_cast<_Type>(0))
        {
            return 0;
        }

        std::size_t _result;

        if (sizeof(_Type) <= sizeof(std::size_t))
        {
            _result = 0;
            std::memcpy(&_result, &_v, sizeof(_Type));
        }
        else
        {
            // fold high bits into low for wide fp (e.g. long double)
            unsigned char _bytes[sizeof(_Type)];
            std::memcpy(_bytes, &_v, sizeof(_Type));
            _result = 0;
            for (std::size_t _i = 0; _i < sizeof(_Type); ++_i)
            {
                _result = (_result * 131u) + _bytes[_i];
            }
        }

        return _result;
    }

NS_END  // internal

// =============================================================================
// integer specialisations
// =============================================================================

#define D_RESTD_HASH_INTEGER_SPEC(T)                                          \
    template<>                                                                \
    struct hash< T >                                                          \
    {                                                                         \
        D_CONSTEXPR std::size_t                                               \
        operator()(                                                           \
            T _v                                                              \
        ) const                                                               \
        {                                                                     \
            return internal::hash_integer_cast(_v);                           \
        }                                                                     \
    }

D_RESTD_HASH_INTEGER_SPEC(bool);
D_RESTD_HASH_INTEGER_SPEC(char);
D_RESTD_HASH_INTEGER_SPEC(signed char);
D_RESTD_HASH_INTEGER_SPEC(unsigned char);
D_RESTD_HASH_INTEGER_SPEC(wchar_t);
D_RESTD_HASH_INTEGER_SPEC(short);
D_RESTD_HASH_INTEGER_SPEC(unsigned short);
D_RESTD_HASH_INTEGER_SPEC(int);
D_RESTD_HASH_INTEGER_SPEC(unsigned int);
D_RESTD_HASH_INTEGER_SPEC(long);
D_RESTD_HASH_INTEGER_SPEC(unsigned long);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
D_RESTD_HASH_INTEGER_SPEC(long long);
D_RESTD_HASH_INTEGER_SPEC(unsigned long long);
D_RESTD_HASH_INTEGER_SPEC(char16_t);
D_RESTD_HASH_INTEGER_SPEC(char32_t);
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
D_RESTD_HASH_INTEGER_SPEC(char8_t);
#endif

#undef D_RESTD_HASH_INTEGER_SPEC

// =============================================================================
// floating-point specialisations
// =============================================================================

#define D_RESTD_HASH_FLOAT_SPEC(T)                                            \
    template<>                                                                \
    struct hash< T >                                                          \
    {                                                                         \
        std::size_t                                                           \
        operator()(                                                           \
            T _v                                                              \
        ) const                                                               \
        {                                                                     \
            return internal::hash_floating(_v);                               \
        }                                                                     \
    }

D_RESTD_HASH_FLOAT_SPEC(float);
D_RESTD_HASH_FLOAT_SPEC(double);
D_RESTD_HASH_FLOAT_SPEC(long double);

#undef D_RESTD_HASH_FLOAT_SPEC

// =============================================================================
// pointer specialisation
// =============================================================================

// hash<T*>
//   class: hashes any object or function pointer. Non-constexpr because
// `reinterpret_cast` is non-constexpr at every standard tier.
template<typename _Type>
struct hash<_Type*>
{
    std::size_t
    operator()(
        _Type* _p
    ) const
    {
        return reinterpret_cast<std::size_t>(_p);
    }
};

// =============================================================================
// nullptr_t specialisation (C++11+)
// =============================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// hash<nullptr_t>
//   class: nullptr_t has only one value; hash is constant zero.
template<>
struct hash<std::nullptr_t>
{
    D_CONSTEXPR std::size_t
    operator()(
        std::nullptr_t
    ) const
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        noexcept
#endif
    {
        return 0;
    }
};

#endif // D_ENV_LANG_IS_CPP11_OR_HIGHER

} // namespace restd

#endif // RESTD_FUNCTIONAL_HASH_
