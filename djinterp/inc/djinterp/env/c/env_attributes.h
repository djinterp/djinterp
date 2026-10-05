/*******************************************************************************
* djinterp [env]                                                env_attributes.h
*
* djinterp portable wrappers for standard attributes.
*   D_* macros that expand to the right spelling of each standard [[...]]
* attribute (C++11 to C++23, and C23) for the detected language standard,
* compiler, and version, falling back to compiler extensions or an empty
* expansion, so they are always safe to use:
*     D_NORETURN              [[noreturn]]            C++11 / C23 / C11
*     D_CARRIES_DEPENDENCY    [[carries_dependency]]  C++11
*     D_DEPRECATED            [[deprecated]]          C++14 / C23
*     D_DEPRECATED_MSG(msg)   [[deprecated(msg)]]     C++14 / C23
*     D_FALLTHROUGH           [[fallthrough]]         C++17 / C23
*     D_NODISCARD             [[nodiscard]]           C++17 / C23
*     D_NODISCARD_MSG(msg)    [[nodiscard(msg)]]      C++20 / C23
*     D_MAYBE_UNUSED          [[maybe_unused]]        C++17 / C23
*     D_NO_UNIQUE_ADDRESS     [[no_unique_address]]   C++20
*     D_LIKELY / D_UNLIKELY   [[likely]]/[[unlikely]] C++20
*     D_ASSUME(expr)          [[assume(expr)]]        C++23
*   In C, a [[...]] spelling is used only at C23, where it is standard;
* __has_c_attribute is not trusted alone, because GCC and Clang answer yes in
* older C modes too, where the syntax is an extension -pedantic rejects. C++
* is held to the same rule: a [[...]] spelling only from the level that
* guarantees it, never because __has_cpp_attribute says yes. The
* one exception is D_NODISCARD under GCC, spelled [[nodiscard]] behind
* __extension__ (section 3.1.1). D_NODISCARD is also the one attribute
* defined outside the language split, once for both languages; the
* framework root takes it from here.
*   D_DELETE, which is not an attribute, is env_lang.h's.
*   Every macro is pre-definable: #define it before including this header to
* override the detected value.
*   It requires env.h, for the D_ENV_LANG_* and D_ENV_COMPILER_* families it
* reads, and includes it itself.
*
*
* path:      /inc/djinterp/env/c/env_attributes.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.11.12
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  C++ ATTRIBUTES
    --------------
    1.  C++11 attributes
         1.  D_NORETURN
         2.  D_CARRIES_DEPENDENCY
    2.  C++14 attributes
         1.  D_DEPRECATED / D_DEPRECATED_MSG
    3.  C++17 attributes
         1.  D_FALLTHROUGH
         2.  D_MAYBE_UNUSED
    4.  C++20 attributes
         1.  D_NO_UNIQUE_ADDRESS
         2.  D_LIKELY / D_UNLIKELY
    5.  C++23 attributes
         1.  D_ASSUME
2.  C ATTRIBUTES
    ------------
    1.  C11 and C23 attributes
         1.  D_NORETURN
         2.  D_DEPRECATED / D_DEPRECATED_MSG
         3.  D_FALLTHROUGH
         4.  D_MAYBE_UNUSED
    2.  C++-only attributes
         1.  D_CARRIES_DEPENDENCY
         2.  D_NO_UNIQUE_ADDRESS
         3.  D_LIKELY / D_UNLIKELY
    3.  Assumptions
         1.  D_ASSUME
3.  ATTRIBUTES OF BOTH LANGUAGES
    -----------------------------
    1.  Discarded results
         1.  D_NODISCARD / D_NODISCARD_MSG
*/

#ifndef DJINTERP_ENV_C_ENV_ATTRIBUTES_H
#define DJINTERP_ENV_C_ENV_ATTRIBUTES_H 1

// djinterp
#include "../env.h"  // D_ENV_LANG_*, D_ENV_COMPILER_*


#if D_ENV_LANG_USING_CPP

//==============================================================================
// 1.  C++ ATTRIBUTES
//==============================================================================
// Each resolves in order: the standard spelling from the language level that
// guarantees it, then a vendor spelling that behaves the same, then nothing.
// There is no __has_cpp_attribute tier: GCC and Clang answer yes below the
// level that guarantees an attribute, where -pedantic-errors rejects the
// [[...]] spelling -- and at C++98 the syntax itself does not exist -- so a
// probe picks a spelling the build refuses. (D_NODISCARD, defined for both
// languages in section 3, keeps one probe, behind a compiler and a level.)


// 1.1    C++11 attributes
//------------------------------------------------------------------------------
// 1.1.1
// D_NORETURN
//   macro: indicates that a function does not return to its caller. Enables
// dead-code elimination and improved diagnostics. Standard in C++11, §7.6.8.
//
//   resolution order:
//     1. C++11 - [[noreturn]] is standard.
//     2. GCC / Clang - __attribute__((noreturn)).
//     3. MSVC - __declspec(noreturn).
//     4. No-op fallback.
#ifndef D_NORETURN
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_NORETURN [[noreturn]]
    #elif ( (defined(D_ENV_COMPILER_GCC)) ||                                   \
            (defined(D_ENV_COMPILER_CLANG)) )
        #define D_NORETURN __attribute__((noreturn))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_NORETURN __declspec(noreturn)
    #else
        #define D_NORETURN
    #endif
#endif  // D_NORETURN

// 1.1.2
// D_CARRIES_DEPENDENCY
//   macro: indicates that a function parameter or return value carries a
// dependency chain into or out of the function, allowing the implementation to
// skip unnecessary memory-fence instructions. Standard in C++11, §7.6.4.
//
//   resolution order:
//     1. C++11, except GCC - [[carries_dependency]] is standard. GCC does not
//        implement it and warns that it is ignored at every level.
//     2. No-op fallback (the attribute is purely advisory).
#ifndef D_CARRIES_DEPENDENCY
    #if ( (D_ENV_LANG_IS_CPP11_OR_HIGHER) &&                                   \
          (!defined(D_ENV_COMPILER_GCC)) )
        #define D_CARRIES_DEPENDENCY [[carries_dependency]]
    #else
        #define D_CARRIES_DEPENDENCY
    #endif
#endif  // D_CARRIES_DEPENDENCY

// 1.2    C++14 attributes
//------------------------------------------------------------------------------
// 1.2.1
// D_DEPRECATED / D_DEPRECATED_MSG
//   macro: marks an entity as deprecated. The _MSG variant includes a
// human-readable reason string shown in the compiler diagnostic. Standard in
// C++14, §7.6.5.
//
//   resolution order:
//     1. C++14 - [[deprecated]] / [[deprecated("…")]] is standard.
//     2. GCC / Clang - __attribute__((deprecated)) / ("…").
//     3. MSVC - __declspec(deprecated) / ("…").
//     4. No-op fallback.
#ifndef D_DEPRECATED
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_DEPRECATED              [[deprecated]]
        #define D_DEPRECATED_MSG(msg)     [[deprecated(msg)]]
    #elif ( (defined(D_ENV_COMPILER_GCC)) ||                                   \
            (defined(D_ENV_COMPILER_CLANG)) )
        #define D_DEPRECATED              __attribute__((deprecated))
        #define D_DEPRECATED_MSG(msg)     __attribute__((deprecated(msg)))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_DEPRECATED              __declspec(deprecated)
        #define D_DEPRECATED_MSG(msg)     __declspec(deprecated(msg))
    #else
        #define D_DEPRECATED
        #define D_DEPRECATED_MSG(msg)
    #endif
#endif  // D_DEPRECATED

#ifndef D_DEPRECATED_MSG
    #define D_DEPRECATED_MSG(msg) D_DEPRECATED
#endif  // D_DEPRECATED_MSG

// 1.3    C++17 attributes
//------------------------------------------------------------------------------
// 1.3.1
// D_FALLTHROUGH
//   macro: placed in a case body before a fall-through to the next label,
// silencing the compiler's implicit-fallthrough warning. Standard in C++17,
// §7.6.6.
//
//   resolution order:
//     1. C++17 - [[fallthrough]] is standard.
//     2. GCC / Clang - __attribute__((fallthrough)).
//     3. No-op fallback.
#ifndef D_FALLTHROUGH
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_FALLTHROUGH [[fallthrough]]
    #elif ( (defined(D_ENV_COMPILER_GCC)) ||                                   \
            (defined(D_ENV_COMPILER_CLANG)) )
        #define D_FALLTHROUGH __attribute__((fallthrough))
    #else
        #define D_FALLTHROUGH
    #endif
#endif  // D_FALLTHROUGH

// 1.3.2
// D_MAYBE_UNUSED
//   macro: suppresses warnings about entities that are intentionally unused
// (e.g. variables, functions, parameters retained for API stability). Standard
// in C++17, §7.6.7.
//
//   resolution order:
//     1. C++17 - [[maybe_unused]] is standard.
//     2. GCC / Clang - __attribute__((unused)).
//     3. No-op fallback.
#ifndef D_MAYBE_UNUSED
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_MAYBE_UNUSED [[maybe_unused]]
    #elif ( (defined(D_ENV_COMPILER_GCC)) ||                                   \
            (defined(D_ENV_COMPILER_CLANG)) )
        #define D_MAYBE_UNUSED __attribute__((unused))
    #else
        #define D_MAYBE_UNUSED
    #endif
#endif  // D_MAYBE_UNUSED

// 1.4    C++20 attributes
//------------------------------------------------------------------------------
// 1.4.1
// D_NO_UNIQUE_ADDRESS
//   macro: indicates that a non-static data member need not have an address
// distinct from all other non-static data members of its class. Allows the
// compiler to optimise empty members to occupy no space, which is particularly
// useful for storing stateless allocators, comparators, and policy objects.
// Standard in C++20, §7.6.9.
//
//   resolution order:
//     1. C++20 - [[no_unique_address]] is standard.
//     2. MSVC - [[msvc::no_unique_address]], the vendor-prefixed form it
//        accepted before the standard spelling. MSVC's C++ modes all have
//        the [[...]] syntax, and a vendor namespace draws no -pedantic
//        diagnostic there.
//     3. No-op fallback (member occupies at least one byte).
#ifndef D_NO_UNIQUE_ADDRESS
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_NO_UNIQUE_ADDRESS [[no_unique_address]]
    #elif ( (D_ENV_LANG_IS_CPP11_OR_HIGHER) &&                                 \
            (defined(D_ENV_COMPILER_MSVC))  &&                                 \
            (defined(__has_cpp_attribute)) )
        #if __has_cpp_attribute(msvc::no_unique_address)
            #define D_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
        #endif
    #endif

    #ifndef D_NO_UNIQUE_ADDRESS
        #define D_NO_UNIQUE_ADDRESS
    #endif  // D_NO_UNIQUE_ADDRESS
#endif  // D_NO_UNIQUE_ADDRESS

// 1.4.2
// D_LIKELY / D_UNLIKELY
//   macro: hints to the compiler which branch of an if/else or switch is the
// expected hot path, enabling better code layout and branch prediction.
// Standard in C++20, §7.6.7.
//
//   note: These are statement attributes, not expression-level hints. For the
// expression form, see D_EXPECT_TRUE / D_EXPECT_FALSE (if provided elsewhere)
// which wrap __builtin_expect.
//
//   resolution order:
//     1. C++20 - [[likely]] / [[unlikely]] are standard.
//     2. No-op fallback: no vendor spelling exists for the statement form.
#ifndef D_LIKELY
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_LIKELY   [[likely]]
        #define D_UNLIKELY [[unlikely]]
    #else
        #define D_LIKELY
        #define D_UNLIKELY
    #endif
#endif  // D_LIKELY

#ifndef D_UNLIKELY
    #define D_UNLIKELY
#endif  // D_UNLIKELY

// 1.5    C++23 attributes
//------------------------------------------------------------------------------
// 1.5.1
// D_ASSUME
//   macro: tells the compiler that `expr` is guaranteed to be true at the point
// of the annotation, enabling optimisations that exploit the invariant.
// Undefined behaviour if `expr` is false at runtime. Standard in C++23,
// §7.6.10.
//
//   resolution order:
//     1. C++23, where the compiler implements it - [[assume(…)]]. The level
//        alone is not enough: Clang before 19 accepts -std=c++23 without the
//        attribute, so __has_cpp_attribute(assume) is asked as well.
//     2. MSVC - __assume(…).
//     3. GCC 13+ - __attribute__((assume(…))).
//     4. Clang - __builtin_assume(…).
//     5. No-op fallback (void-cast to suppress unused warnings).
#ifndef D_ASSUME
    #if ( (D_ENV_LANG_IS_CPP23_OR_HIGHER) &&                                   \
          (defined(__has_cpp_attribute)) )
        #if __has_cpp_attribute(assume)
            #define D_ASSUME(expr) [[assume(expr)]]
        #endif
    #endif

    #ifndef D_ASSUME
        #if defined(D_ENV_COMPILER_MSVC)
            #define D_ASSUME(expr) __assume(expr)
        #elif ( (defined(D_ENV_COMPILER_GCC)) &&                               \
                (__GNUC__ >= 13) )
            #define D_ASSUME(expr) __attribute__((assume(expr)))
        #elif defined(D_ENV_COMPILER_CLANG)
            #define D_ASSUME(expr) __builtin_assume(expr)
        #else
            #define D_ASSUME(expr) ((void)(expr))
        #endif
    #endif  // D_ASSUME
#endif  // D_ASSUME


#else  // compiling as C

//==============================================================================
// 2.  C ATTRIBUTES
//==============================================================================
// C23 standardized several of these, and C11 has _Noreturn; before C23 the
// vendor spellings stand in. The C++-only attributes expand to nothing, and
// D_ASSUME uses compiler intrinsics.


// 2.1    C11 and C23 attributes
//------------------------------------------------------------------------------
// 2.1.1
// D_NORETURN
//   macro: C11 introduced _Noreturn (and <stdnoreturn.h>). C23 added the
// standard [[noreturn]] attribute and deprecated _Noreturn. Standard in C11 /
// C23.
//
//   resolution order:
//     1. C23 - [[noreturn]] is standard.
//     2. C11 - _Noreturn keyword.
//     3. GCC / Clang - __attribute__((noreturn)).
//     4. MSVC - __declspec(noreturn).
//     5. No-op fallback.
#ifndef D_NORETURN
    #if D_ENV_LANG_IS_C23_OR_HIGHER
        #define D_NORETURN [[noreturn]]
    #endif

    #ifndef D_NORETURN
        #if D_ENV_LANG_IS_C11_OR_HIGHER
            #define D_NORETURN _Noreturn
        #elif ( (defined(D_ENV_COMPILER_GCC)) ||                               \
                (defined(D_ENV_COMPILER_CLANG)) )
            #define D_NORETURN __attribute__((noreturn))
        #elif defined(D_ENV_COMPILER_MSVC)
            #define D_NORETURN __declspec(noreturn)
        #else
            #define D_NORETURN
        #endif
    #endif  // D_NORETURN
#endif  // D_NORETURN

// 2.1.2
// D_DEPRECATED / D_DEPRECATED_MSG
//   macro: C23 added [[deprecated]] / [[deprecated("…")]]. Before C23, GCC and
// Clang support __attribute__((deprecated)), MSVC has __declspec. Standard in
// C23.
//
//   resolution order:
//     1. C23 - [[deprecated]] / [[deprecated("…")]] is standard.
//     2. GCC / Clang - __attribute__((deprecated)) / ("…").
//     3. MSVC - __declspec(deprecated) / ("…").
//     4. No-op fallback.
#ifndef D_DEPRECATED
    #if D_ENV_LANG_IS_C23_OR_HIGHER
        #define D_DEPRECATED              [[deprecated]]
        #define D_DEPRECATED_MSG(msg)     [[deprecated(msg)]]
    #endif

    #ifndef D_DEPRECATED
        #if ( (defined(D_ENV_COMPILER_GCC)) ||                                 \
              (defined(D_ENV_COMPILER_CLANG)) )
            #define D_DEPRECATED              __attribute__((deprecated))
            #define D_DEPRECATED_MSG(msg)     __attribute__((deprecated(msg)))
        #elif defined(D_ENV_COMPILER_MSVC)
            #define D_DEPRECATED              __declspec(deprecated)
            #define D_DEPRECATED_MSG(msg)     __declspec(deprecated(msg))
        #else
            #define D_DEPRECATED
            #define D_DEPRECATED_MSG(msg)
        #endif
    #endif  // D_DEPRECATED
#endif  // D_DEPRECATED

#ifndef D_DEPRECATED_MSG
    #define D_DEPRECATED_MSG(msg) D_DEPRECATED
#endif  // D_DEPRECATED_MSG

// 2.1.3
// D_FALLTHROUGH
//   macro: C23 added [[fallthrough]]. Before C23, GCC 7+ and Clang 3.6+ accept
// __attribute__((fallthrough)) in C mode. Standard in C23.
//
//   resolution order:
//     1. C23 - [[fallthrough]] is standard.
//     2. GCC / Clang - __attribute__((fallthrough)).
//     3. No-op fallback.
#ifndef D_FALLTHROUGH
    #if D_ENV_LANG_IS_C23_OR_HIGHER
        #define D_FALLTHROUGH [[fallthrough]]
    #endif

    #ifndef D_FALLTHROUGH
        #if ( (defined(D_ENV_COMPILER_GCC)) ||                                 \
              (defined(D_ENV_COMPILER_CLANG)) )
            #define D_FALLTHROUGH __attribute__((fallthrough))
        #else
            #define D_FALLTHROUGH
        #endif
    #endif  // D_FALLTHROUGH
#endif  // D_FALLTHROUGH

// 2.1.4
// D_MAYBE_UNUSED
//   macro: C23 added [[maybe_unused]]. Before C23, GCC and Clang support
// __attribute__((unused)) in C mode. Standard in C23.
//
//   resolution order:
//     1. C23 - [[maybe_unused]] is standard.
//     2. GCC / Clang - __attribute__((unused)).
//     3. No-op fallback.
#ifndef D_MAYBE_UNUSED
    #if D_ENV_LANG_IS_C23_OR_HIGHER
        #define D_MAYBE_UNUSED [[maybe_unused]]
    #endif

    #ifndef D_MAYBE_UNUSED
        #if ( (defined(D_ENV_COMPILER_GCC)) ||                                 \
              (defined(D_ENV_COMPILER_CLANG)) )
            #define D_MAYBE_UNUSED __attribute__((unused))
        #else
            #define D_MAYBE_UNUSED
        #endif
    #endif  // D_MAYBE_UNUSED
#endif  // D_MAYBE_UNUSED

// 2.2    C++-only attributes
//------------------------------------------------------------------------------
// 2.2.1
// D_CARRIES_DEPENDENCY
//   macro: not part of any C standard. Always a no-op in C mode.
#ifndef D_CARRIES_DEPENDENCY
    #define D_CARRIES_DEPENDENCY
#endif  // D_CARRIES_DEPENDENCY

// 2.2.2
// D_NO_UNIQUE_ADDRESS
//   macro: not part of any C standard (the concept is meaningless for C
// structs). Always a no-op in C mode.
#ifndef D_NO_UNIQUE_ADDRESS
    #define D_NO_UNIQUE_ADDRESS
#endif  // D_NO_UNIQUE_ADDRESS

// 2.2.3
// D_LIKELY / D_UNLIKELY
//   macro: not part of any C standard as statement attributes. Always a no-op
// in C mode. (Use __builtin_expect for expression-level hints.)
#ifndef D_LIKELY
    #define D_LIKELY
#endif  // D_LIKELY
#ifndef D_UNLIKELY
    #define D_UNLIKELY
#endif  // D_UNLIKELY

// 2.3    Assumptions
//------------------------------------------------------------------------------
// 2.3.1
// D_ASSUME
//   macro: not standardised in C. Compiler intrinsics are still available.
//
//   resolution order:
//     1. MSVC - __assume(…).
//     2. GCC 13+ - __attribute__((assume(…))).
//     3. Clang - __builtin_assume(…).
//     4. No-op fallback (void-cast to suppress unused warnings).
#ifndef D_ASSUME
    #if defined(D_ENV_COMPILER_MSVC)
        #define D_ASSUME(expr) __assume(expr)
    #elif ( (defined(D_ENV_COMPILER_GCC)) &&                                   \
            (__GNUC__ >= 13) )
        #define D_ASSUME(expr) __attribute__((assume(expr)))
    #elif defined(D_ENV_COMPILER_CLANG)
        #define D_ASSUME(expr) __builtin_assume(expr)
    #else
        #define D_ASSUME(expr) ((void)(expr))
    #endif
#endif  // D_ASSUME

#endif  // D_ENV_LANG_USING_CPP


//==============================================================================
// 3.  ATTRIBUTES OF BOTH LANGUAGES
//==============================================================================
// An attribute whose spelling differs by language but whose choice is one
// rule is defined once, outside the language split above. The framework root
// (djinterp.h) takes it from here rather than keeping a copy, so include order
// cannot change its value.


// 3.1    Discarded results
//------------------------------------------------------------------------------
// 3.1.1
// D_NODISCARD / D_NODISCARD_MSG
//   macro: asks for a diagnostic when a caller discards the result of a
// function so marked; the _MSG variant adds a reason to it. Standard as
// [[nodiscard]] in C++17 and C23, and as [[nodiscard(msg)]] in C++20 and C23.
// Below those levels a spelling is used only where it behaves exactly as the
// standard attribute does -- a (void) cast silences it -- and -pedantic-errors
// accepts it; elsewhere the macro is empty, which loses the diagnostic but
// never changes what compiles.
//
//   resolution order:
//     1.  C++17 and C23 - [[nodiscard]].
//     2.  Clang below those, in either language -
//         __attribute__((warn_unused_result)): the same diagnostic, silenced
//         by a (void) cast, valid at every level. C++98 included, where the
//         [[...]] syntax does not exist, so [[clang::warn_unused_result]] is
//         not used.
//     3.  GCC at C++11 and C++14 - [[nodiscard]], which g++ accepts early
//         with its standard meaning and no -pedantic diagnostic.
//     4.  GCC 11 and later in C before C23 - [[nodiscard]] behind
//         __extension__, which -pedantic accepts. __extension__ must begin a
//         declaration, so D_NODISCARD leads every declaration it marks.
//     5.  Everything else - empty. GCC's own warn_unused_result is never
//         used: a (void) cast does not silence it, so it would make the lower
//         levels stricter than C++17. GCC at C++98 has no spelling left, since
//         [[...]] is C++11 syntax; nor does MSVC below C++17.
//   __has_cpp_attribute and __has_c_attribute are consulted only as a second
// condition below the standard level, never alone: GCC and Clang answer yes in
// modes whose grammar lacks the spelling.
//
//   pre-definable: #define D_NODISCARD or D_NODISCARD_MSG before including
// this header to override the detected value.
#ifndef D_NODISCARD
    #if ( (D_ENV_LANG_IS_CPP17_OR_HIGHER) ||                                  \
          (D_ENV_LANG_IS_C23_OR_HIGHER) )
        #define D_NODISCARD [[nodiscard]]
    #elif defined(D_ENV_COMPILER_CLANG)
        #define D_NODISCARD __attribute__((warn_unused_result))
    #elif ( (D_ENV_LANG_IS_CPP11_OR_HIGHER) &&                                \
            (defined(D_ENV_COMPILER_GCC))   &&                                \
            (defined(__has_cpp_attribute)) )
        #if __has_cpp_attribute(nodiscard)
            #define D_NODISCARD [[nodiscard]]
        #endif
    #elif ( (!D_ENV_LANG_USING_CPP)       &&                                  \
            (defined(D_ENV_COMPILER_GCC)) &&                                  \
            (defined(__has_c_attribute)) )
        #if __has_c_attribute(nodiscard)
            #define D_NODISCARD __extension__ [[nodiscard]]
        #endif
    #endif

    // 5. above: no spelling that behaves the same
    #ifndef D_NODISCARD
        #define D_NODISCARD
    #endif  // D_NODISCARD
#endif  // D_NODISCARD

#ifndef D_NODISCARD_MSG
    #if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) ||                                  \
          (D_ENV_LANG_IS_C23_OR_HIGHER) )
        #define D_NODISCARD_MSG(msg) [[nodiscard(msg)]]
    #else
        #define D_NODISCARD_MSG(msg) D_NODISCARD
    #endif
#endif  // D_NODISCARD_MSG


#endif  // DJINTERP_ENV_C_ENV_ATTRIBUTES_H
