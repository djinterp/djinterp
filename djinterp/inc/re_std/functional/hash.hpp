/*******************************************************************************
* djinterp [re_std]                                                     hash.hpp
*
* class: customisation point for hashing values.
*   Provides the primary `re_std::hash<Type>` template plus
* specialisations for every scalar-like type the standard requires:
* every arithmetic type, every pointer type, and `nullptr_t`.
*
*   Specialisations whose key types live in other modules
* (`hash<basic_string<...>>`, `hash<unique_ptr<...>>`,
* `hash<optional<T>>`, etc.) ship alongside those modules.
*
*   The standard says the hash for an integer is "implementation
* defined". re_std uses identity casts to `size_t` for integral types --
* this is the same choice libstdc++/libc++ make for short keys -- and
* a `reinterpret_cast`-based cast for pointers. For floating-point,
* the bytes are read into a `size_t` so that distinct bit patterns
* yield distinct hashes; `+0.0` and `-0.0` both normalise to a zero
* hash so equal-comparing values hash equally.
*
*
* path:      /inc/re_std/functional/hash.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_HASH_HPP
#define RE_STD_FUNCTIONAL_HASH_HPP 1

// std
#include <cstddef>   // size_t, nullptr_t (gated below)
#include <cstring>   // memcpy for fp hashing
// re_std
#include "../config.hpp"  // RE_STD_* configuration

namespace re_std
{

// hash
//   class: primary template -- left empty (no operator()), so attempts
// to hash an unsupported key type are ill-formed at instantiation.
template<typename Type>
struct hash
{};

namespace internal
{

    // hash_integer_cast
    //   function: identity-cast hash for integers. Branchless and
    // trivially constexpr.
    template<typename Type>
    RE_STD_CONSTEXPR std::size_t
    hash_integer_cast(
        Type _v
    )
    {
        return static_cast<std::size_t>(_v);
    }

    // hash_floating
    //   function: bytewise hash for floating-point values. Normalises
    // -0.0 to +0.0 so that equal values hash equally. Not constexpr
    // (memcpy is not constexpr until C++20).
    template<typename Type>
    inline std::size_t
    hash_floating(
        Type _v
    )
    {
        // normalise signed zero
        if (_v == static_cast<Type>(0))
        {
            return 0;
        }

        std::size_t _result;

        if (sizeof(Type) <= sizeof(std::size_t))
        {
            _result = 0;
            std::memcpy(&_result, &_v, sizeof(Type));
        }
        else
        {
            // fold high bits into low for wide fp (e.g. long double)
            unsigned char _bytes[sizeof(Type)];
            std::memcpy(_bytes, &_v, sizeof(Type));
            _result = 0;
            for (std::size_t _i = 0; _i < sizeof(Type); ++_i)
            {
                _result = (_result * 131u) + _bytes[_i];
            }
        }

        return _result;
    }

}  // internal

// =============================================================================
// integer specialisations
// =============================================================================

#define RE_STD_HASH_INTEGER_SPEC(T)                                          \
    template<>                                                                \
    struct hash< T >                                                          \
    {                                                                         \
        RE_STD_CONSTEXPR std::size_t                                               \
        operator()(                                                           \
            T _v                                                              \
        ) const                                                               \
        {                                                                     \
            return internal::hash_integer_cast(_v);                           \
        }                                                                     \
    }

RE_STD_HASH_INTEGER_SPEC(bool);
RE_STD_HASH_INTEGER_SPEC(char);
RE_STD_HASH_INTEGER_SPEC(signed char);
RE_STD_HASH_INTEGER_SPEC(unsigned char);
RE_STD_HASH_INTEGER_SPEC(wchar_t);
RE_STD_HASH_INTEGER_SPEC(short);
RE_STD_HASH_INTEGER_SPEC(unsigned short);
RE_STD_HASH_INTEGER_SPEC(int);
RE_STD_HASH_INTEGER_SPEC(unsigned int);
RE_STD_HASH_INTEGER_SPEC(long);
RE_STD_HASH_INTEGER_SPEC(unsigned long);

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
RE_STD_HASH_INTEGER_SPEC(long long);
RE_STD_HASH_INTEGER_SPEC(unsigned long long);
RE_STD_HASH_INTEGER_SPEC(char16_t);
RE_STD_HASH_INTEGER_SPEC(char32_t);
#endif

#if RE_STD_LANG_IS_CPP20_OR_HIGHER
RE_STD_HASH_INTEGER_SPEC(char8_t);
#endif

#undef RE_STD_HASH_INTEGER_SPEC

// =============================================================================
// floating-point specialisations
// =============================================================================

#define RE_STD_HASH_FLOAT_SPEC(T)                                            \
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

RE_STD_HASH_FLOAT_SPEC(float);
RE_STD_HASH_FLOAT_SPEC(double);
RE_STD_HASH_FLOAT_SPEC(long double);

#undef RE_STD_HASH_FLOAT_SPEC

// =============================================================================
// pointer specialisation
// =============================================================================

// hash<T*>
//   class: hashes any object or function pointer. Non-constexpr because
// `reinterpret_cast` is non-constexpr at every standard tier.
template<typename Type>
struct hash<Type*>
{
    std::size_t
    operator()(
        Type* _p
    ) const
    {
        return reinterpret_cast<std::size_t>(_p);
    }
};

// =============================================================================
// nullptr_t specialisation (C++11+)
// =============================================================================

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// hash<nullptr_t>
//   class: nullptr_t has only one value; hash is constant zero.
template<>
struct hash<std::nullptr_t>
{
    RE_STD_CONSTEXPR std::size_t
    operator()(
        std::nullptr_t
    ) const
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
        noexcept
#endif
    {
        return 0;
    }
};

#endif // RE_STD_LANG_IS_CPP11_OR_HIGHER

}  // re_std
#endif  // RE_STD_FUNCTIONAL_HASH_HPP
