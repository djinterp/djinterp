/***********************************************************************
* restd                                                      addressof.hpp
*
* obtain a true pointer to an object, bypassing operator& overloads:
*   restd::addressof(_x) returns the address of _x as a _Type*, ignoring
* any user-defined operator& on _Type. Three implementation tiers, picked
* by capability detection:
*
*   1. C++17+ with __builtin_addressof: constexpr, single-statement.
*   2. C++11+:                          non-constexpr, reinterpret_cast
*                                       through volatile char& (the
*                                       canonical N4150 idiom).
*   3. C++98/03:                        same body as tier 2; lacks the
*                                       deleted rvalue overload.
*
* The deleted rvalue overload (`addressof(const _Type&&) = delete`) is
* present only on C++11+. On C++98/03 there are no rvalue references to
* delete, so the misuse is impossible to express in the first place.
*
*
* path:      /inc/djinterp/re_std/memory/addressof.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.01
***********************************************************************/

#ifndef RESTD_MEMORY_ADDRESSOF_
#define RESTD_MEMORY_ADDRESSOF_ 1

#include "djinterp.hpp"


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// D_RESTD_HAS_BUILTIN_ADDRESSOF
//   constant: 1 if __builtin_addressof is available. Required for the
//   constexpr path; the reinterpret_cast fallback is never constexpr
//   because it crosses the implicitly-volatile boundary.
#ifndef D_RESTD_HAS_BUILTIN_ADDRESSOF
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_addressof)
            #define D_RESTD_HAS_BUILTIN_ADDRESSOF  1
        #else
            #define D_RESTD_HAS_BUILTIN_ADDRESSOF  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC) &&                                    \
            D_ENV_COMPILER_VERSION_AT_LEAST(7, 0, 0) )
        #define D_RESTD_HAS_BUILTIN_ADDRESSOF  1
    #elif ( defined(D_ENV_COMPILER_MSVC) &&                                   \
            D_ENV_COMPILER_VERSION_AT_LEAST(19, 0, 0) )
        // MSVC 2015+ (_MSC_VER 1900+) supplies it under the same name.
        #define D_RESTD_HAS_BUILTIN_ADDRESSOF  1
    #else
        #define D_RESTD_HAS_BUILTIN_ADDRESSOF  0
    #endif
#endif


namespace restd
{

// =============================================================================
// addressof
// =============================================================================

#if D_RESTD_HAS_BUILTIN_ADDRESSOF

    // addressof
    //   function: returns the actual address of _v, ignoring any
    //             operator& overload on _Type. constexpr.
    template<typename _Type>
    D_CONSTEXPR _Type* addressof(_Type& _v) D_NOEXCEPT
    {
        return __builtin_addressof(_v);
    }

#else  // !D_RESTD_HAS_BUILTIN_ADDRESSOF

    // addressof
    //   function: portable fallback. Casts through char& to defeat any
    //             user operator&, then back to _Type*. Not constexpr
    //             (the reinterpret_cast forbids it).
    template<typename _Type>
    _Type* addressof(_Type& _v) D_NOEXCEPT
    {
        return reinterpret_cast<_Type*>
        (
            &const_cast<char&>
            (
                reinterpret_cast<const volatile char&>(_v)
            )
        );
    }

#endif  // D_RESTD_HAS_BUILTIN_ADDRESSOF


// =============================================================================
// addressof  -  rvalue overload deletion
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    // addressof(const _Type&&)
    //   function: deleted. Catches addressof(temporary) at compile time.
    template<typename _Type>
    const _Type* addressof(const _Type&&) = delete;

#endif


}  // namespace restd

#endif  // RESTD_MEMORY_ADDRESSOF_
