/*******************************************************************************
* djinterp [re_std]                                                addressof.hpp
*
* obtain a true pointer to an object, bypassing operator& overloads:
*   re_std::addressof(_x) returns the address of _x as a Type*, ignoring
* any user-defined operator& on Type. Three implementation tiers, picked
* by capability detection:
*
*   1. C++17+ with __builtin_addressof: constexpr, single-statement.
*   2. C++11+:                          non-constexpr, reinterpret_cast
*                                       through volatile char& (the
*                                       canonical N4150 idiom).
*   3. C++98/03:                        same body as tier 2; lacks the
*                                       deleted rvalue overload.
*
* The deleted rvalue overload (`addressof(const Type&&) = delete`) is
* present only on C++11+. On C++98/03 there are no rvalue references to
* delete, so the misuse is impossible to express in the first place.
*
*
* path:      /inc/re_std/memory/addressof.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.01
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_ADDRESSOF_HPP
#define RE_STD_MEMORY_ADDRESSOF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// RE_STD_HAS_BUILTIN_ADDRESSOF
//   constant: 1 if __builtin_addressof is available. Required for the
//   constexpr path; the reinterpret_cast fallback is never constexpr
//   because it crosses the implicitly-volatile boundary.
#ifndef RE_STD_HAS_BUILTIN_ADDRESSOF
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_addressof)
            #define RE_STD_HAS_BUILTIN_ADDRESSOF  1
        #else
            #define RE_STD_HAS_BUILTIN_ADDRESSOF  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC) &&                                    \
            RE_STD_COMPILER_VERSION_AT_LEAST(7, 0, 0) )
        #define RE_STD_HAS_BUILTIN_ADDRESSOF  1
    #elif ( defined(RE_STD_COMPILER_MSVC) &&                                   \
            RE_STD_COMPILER_VERSION_AT_LEAST(19, 0, 0) )
        // MSVC 2015+ (_MSC_VER 1900+) supplies it under the same name.
        #define RE_STD_HAS_BUILTIN_ADDRESSOF  1
    #else
        #define RE_STD_HAS_BUILTIN_ADDRESSOF  0
    #endif
#endif


namespace re_std
{

// =============================================================================
// addressof
// =============================================================================

#if RE_STD_HAS_BUILTIN_ADDRESSOF

    // addressof
    //   function: returns the actual address of _v, ignoring any
    //             operator& overload on Type. constexpr.
    template<typename Type>
    RE_STD_CONSTEXPR Type* addressof(Type& _v) RE_STD_NOEXCEPT
    {
        return __builtin_addressof(_v);
    }

#else  // !RE_STD_HAS_BUILTIN_ADDRESSOF

    // addressof
    //   function: portable fallback. Casts through char& to defeat any
    //             user operator&, then back to Type*. Not constexpr
    //             (the reinterpret_cast forbids it).
    template<typename Type>
    Type* addressof(Type& _v) RE_STD_NOEXCEPT
    {
        return reinterpret_cast<Type*>
        (
            &const_cast<char&>
            (
                reinterpret_cast<const volatile char&>(_v)
            )
        );
    }

#endif  // RE_STD_HAS_BUILTIN_ADDRESSOF


// =============================================================================
// addressof  -  rvalue overload deletion
// =============================================================================

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    // addressof(const Type&&)
    //   function: deleted. Catches addressof(temporary) at compile time.
    template<typename Type>
    const Type* addressof(const Type&&) = delete;

#endif


}  // re_std
#endif  // RE_STD_MEMORY_ADDRESSOF_HPP
