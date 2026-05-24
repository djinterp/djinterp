/******************************************************************************
* djinterp [core]                                              env_attributes.h
*
*   Portable wrappers for standard [[…]] attributes (C++11–C++23, C23).
*
*   This header defines D_* macros that expand to the correct attribute
*   spelling for the detected language standard, compiler, and version,
*   falling back to compiler extensions or empty expansions so the macros
*   are always safe to use.
*
*   Requires:  env.h  (must be #included first for the D_ENV_* detection
*              macros used throughout this file).
*
*   Attributes defined herein:
*     D_NORETURN              [[noreturn]]             C++11 / C23 / C11
*     D_CARRIES_DEPENDENCY    [[carries_dependency]]   C++11
*     D_DEPRECATED            [[deprecated]]           C++14 / C23
*     D_DEPRECATED_MSG(msg)   [[deprecated("…")]]      C++14 / C23
*     D_FALLTHROUGH           [[fallthrough]]          C++17 / C23
*     D_NODISCARD             [[nodiscard]]            C++17 / C23
*     D_NODISCARD_MSG(msg)    [[nodiscard("…")]]       C++20 / C23
*     D_MAYBE_UNUSED          [[maybe_unused]]         C++17 / C23
*     D_NO_UNIQUE_ADDRESS     [[no_unique_address]]    C++20
*     D_LIKELY                [[likely]]               C++20
*     D_UNLIKELY              [[unlikely]]             C++20
*     D_ASSUME(expr)          [[assume(…)]]            C++23
*
*   Every macro is pre-definable: #define it before including this header
*   to override the detected value.
*
* path:      /inc/c/core/config/env_attributes.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2023.11.12
******************************************************************************/

#ifndef DJINTERP_ENV_ATTRIBUTES_
#define DJINTERP_ENV_ATTRIBUTES_ 1


// ===========================================================================
// I.   C++ ATTRIBUTES
// ===========================================================================

#if defined(__cplusplus)

// D_DELETE
//   macro: resolves to "= delete" on C++11+, where deleted
// functions are part of the language baseline.  On C++03,
// expands to nothing; the function should be left declared
// in the private section to achieve the same effect.
#ifndef D_DELETE
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_DELETE        = delete
    #else
        #define D_DELETE
    #endif
#endif  // D_DELETE

// -----------------------------------------------------------------------------
// D_NORETURN                                                     (C++11, §7.6.8)
//   Indicates that a function does not return to its caller.  Enables dead-
//   code elimination and improved diagnostics.
//
//   Resolution order:
//     1. C++11 - [[noreturn]] is standard.
//     2. __has_cpp_attribute(noreturn) - early support probe.
//     3. GCC / Clang - __attribute__((noreturn)).
//     4. MSVC - __declspec(noreturn).
//     5. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_NORETURN
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_NORETURN [[noreturn]]
    #elif defined(__has_cpp_attribute)
        #if __has_cpp_attribute(noreturn)
            #define D_NORETURN [[noreturn]]
        #endif
    #endif

    #ifndef D_NORETURN
        #if ( defined(D_ENV_COMPILER_GCC) ||  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_NORETURN __attribute__((noreturn))
        #elif defined(D_ENV_COMPILER_MSVC)
            #define D_NORETURN __declspec(noreturn)
        #else
            #define D_NORETURN
        #endif
    #endif  // D_NORETURN (fallback)
#endif  // D_NORETURN (outer guard)


// -----------------------------------------------------------------------------
// D_CARRIES_DEPENDENCY                                           (C++11, §7.6.4)
//   Indicates that a function parameter or return value carries a dependency
//   chain into or out of the function, allowing the implementation to skip
//   unnecessary memory-fence instructions.
//
//   Resolution order:
//     1. C++11 - [[carries_dependency]] is standard.
//     2. __has_cpp_attribute probe.
//     3. No-op fallback (the attribute is purely advisory).
// -----------------------------------------------------------------------------
#ifndef D_CARRIES_DEPENDENCY
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_CARRIES_DEPENDENCY [[carries_dependency]]
    #elif defined(__has_cpp_attribute)
        #if __has_cpp_attribute(carries_dependency)
            #define D_CARRIES_DEPENDENCY [[carries_dependency]]
        #endif
    #endif

    #ifndef D_CARRIES_DEPENDENCY
        #define D_CARRIES_DEPENDENCY
    #endif  // D_CARRIES_DEPENDENCY (fallback)
#endif  // D_CARRIES_DEPENDENCY (outer guard)


// -----------------------------------------------------------------------------
// D_DEPRECATED / D_DEPRECATED_MSG(msg)                           (C++14, §7.6.5)
//   Marks an entity as deprecated.  The _MSG variant includes a human-
//   readable reason string shown in the compiler diagnostic.
//
//   Resolution order:
//     1. C++14 - [[deprecated]] / [[deprecated("…")]] is standard.
//     2. __has_cpp_attribute probe.
//     3. GCC / Clang - __attribute__((deprecated)) / ("…").
//     4. MSVC - __declspec(deprecated) / ("…").
//     5. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_DEPRECATED
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_DEPRECATED              [[deprecated]]
        #define D_DEPRECATED_MSG(msg)     [[deprecated(msg)]]
    #elif defined(__has_cpp_attribute)
        #if __has_cpp_attribute(deprecated)
            #define D_DEPRECATED          [[deprecated]]
            #define D_DEPRECATED_MSG(msg) [[deprecated(msg)]]
        #endif
    #endif

    #ifndef D_DEPRECATED
        #if ( defined(D_ENV_COMPILER_GCC) ||  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_DEPRECATED              __attribute__((deprecated))
            #define D_DEPRECATED_MSG(msg)     __attribute__((deprecated(msg)))
        #elif defined(D_ENV_COMPILER_MSVC)
            #define D_DEPRECATED              __declspec(deprecated)
            #define D_DEPRECATED_MSG(msg)     __declspec(deprecated(msg))
        #else
            #define D_DEPRECATED
            #define D_DEPRECATED_MSG(msg)
        #endif
    #endif  // D_DEPRECATED (fallback)
#endif  // D_DEPRECATED (outer guard)

#ifndef D_DEPRECATED_MSG
    #define D_DEPRECATED_MSG(msg) D_DEPRECATED
#endif  // D_DEPRECATED_MSG (safety net)


// -----------------------------------------------------------------------------
// D_FALLTHROUGH                                                  (C++17, §7.6.6)
//   Placed in a case body before a fall-through to the next label, silencing
//   the compiler's implicit-fallthrough warning.
//
//   Resolution order:
//     1. C++17 - [[fallthrough]] is standard.
//     2. __has_cpp_attribute probe.
//     3. GCC / Clang - __attribute__((fallthrough)).
//     4. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_FALLTHROUGH
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_FALLTHROUGH [[fallthrough]]
    #elif defined(__has_cpp_attribute)
        #if __has_cpp_attribute(fallthrough)
            #define D_FALLTHROUGH [[fallthrough]]
        #endif
    #endif

    #ifndef D_FALLTHROUGH
        #if ( defined(D_ENV_COMPILER_GCC) ||  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_FALLTHROUGH __attribute__((fallthrough))
        #else
            #define D_FALLTHROUGH
        #endif
    #endif  // D_FALLTHROUGH (fallback)
#endif  // D_FALLTHROUGH (outer guard)


// -----------------------------------------------------------------------------
// D_NODISCARD / D_NODISCARD_MSG(msg)                    (C++17 / C++20, §7.6.7)
//   Indicates that a function's return value should not be silently
//   discarded.  The _MSG variant (C++20) includes a reason string.
//
//   Resolution order:
//     1. C++17 - [[nodiscard]] is standard.
//     2. C++20 - [[nodiscard("…")]] adds the message form.
//     3. __has_cpp_attribute probe.
//     4. GCC / Clang - __attribute__((warn_unused_result)).
//     5. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_NODISCARD
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_NODISCARD [[nodiscard]]
    #elif defined(__has_cpp_attribute)
        #if __has_cpp_attribute(nodiscard)
            #define D_NODISCARD [[nodiscard]]
        #endif
    #endif

    #ifndef D_NODISCARD
        #if ( defined(D_ENV_COMPILER_GCC) ||  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_NODISCARD __attribute__((warn_unused_result))
        #else
            #define D_NODISCARD
        #endif
    #endif  // D_NODISCARD (fallback)
#endif  // D_NODISCARD (outer guard)

#ifndef D_NODISCARD_MSG
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_NODISCARD_MSG(msg) [[nodiscard(msg)]]
    #elif defined(__has_cpp_attribute)
        // __has_cpp_attribute(nodiscard) >= 201907L means the message
        // form is supported (P1301R4).
        #if __has_cpp_attribute(nodiscard) >= 201907L
            #define D_NODISCARD_MSG(msg) [[nodiscard(msg)]]
        #endif
    #endif

    #ifndef D_NODISCARD_MSG
        #define D_NODISCARD_MSG(msg) D_NODISCARD
    #endif  // D_NODISCARD_MSG (fallback)
#endif  // D_NODISCARD_MSG (outer guard)


// -----------------------------------------------------------------------------
// D_MAYBE_UNUSED                                                 (C++17, §7.6.7)
//   Suppresses warnings about entities that are intentionally unused (e.g.
//   variables, functions, parameters retained for API stability).
//
//   Resolution order:
//     1. C++17 - [[maybe_unused]] is standard.
//     2. __has_cpp_attribute probe.
//     3. GCC / Clang - __attribute__((unused)).
//     4. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_MAYBE_UNUSED
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_MAYBE_UNUSED [[maybe_unused]]
    #elif defined(__has_cpp_attribute)
        #if __has_cpp_attribute(maybe_unused)
            #define D_MAYBE_UNUSED [[maybe_unused]]
        #endif
    #endif

    #ifndef D_MAYBE_UNUSED
        #if ( defined(D_ENV_COMPILER_GCC) ||  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_MAYBE_UNUSED __attribute__((unused))
        #else
            #define D_MAYBE_UNUSED
        #endif
    #endif  // D_MAYBE_UNUSED (fallback)
#endif  // D_MAYBE_UNUSED (outer guard)


// -----------------------------------------------------------------------------
// D_NO_UNIQUE_ADDRESS                                            (C++20, §7.6.9)
//   Indicates that a non-static data member need not have an address
//   distinct from all other non-static data members of its class.
//   Allows the compiler to optimise empty members to occupy no space,
//   which is particularly useful for storing stateless allocators,
//   comparators, and policy objects.
//
//   Resolution order:
//     1. C++20 - [[no_unique_address]] is standard.
//     2. __has_cpp_attribute(no_unique_address) - early support probe.
//     3. MSVC - [[msvc::no_unique_address]] vendor-prefixed form; MSVC
//        accepted this before recognising the standard spelling.
//     4. No-op fallback (member occupies at least one byte).
// -----------------------------------------------------------------------------
#ifndef D_NO_UNIQUE_ADDRESS
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_NO_UNIQUE_ADDRESS [[no_unique_address]]
    #elif defined(__has_cpp_attribute)
        #if __has_cpp_attribute(no_unique_address)
            #define D_NO_UNIQUE_ADDRESS [[no_unique_address]]
        #endif
    #endif

    // ---- MSVC vendor-prefixed fallback ----
    #ifndef D_NO_UNIQUE_ADDRESS
        #if defined(D_ENV_COMPILER_MSVC)
            #if defined(__has_cpp_attribute)
                #if __has_cpp_attribute(msvc::no_unique_address)
                    #define D_NO_UNIQUE_ADDRESS \
                        [[msvc::no_unique_address]]
                #endif
            #endif
        #endif
    #endif  // D_NO_UNIQUE_ADDRESS (MSVC fallback)

    #ifndef D_NO_UNIQUE_ADDRESS
        #define D_NO_UNIQUE_ADDRESS
    #endif  // D_NO_UNIQUE_ADDRESS (final fallback)
#endif  // D_NO_UNIQUE_ADDRESS (outer guard)


// -----------------------------------------------------------------------------
// D_LIKELY / D_UNLIKELY                                         (C++20, §7.6.7)
//   Hints to the compiler which branch of an if/else or switch is the
//   expected hot path, enabling better code layout and branch prediction.
//
//   NOTE: These are statement attributes, not expression-level hints.
//   For the expression form, see D_EXPECT_TRUE / D_EXPECT_FALSE (if
//   provided elsewhere) which wrap __builtin_expect.
//
//   Resolution order:
//     1. C++20 - [[likely]] / [[unlikely]] are standard.
//     2. __has_cpp_attribute probe.
//     3. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_LIKELY
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_LIKELY   [[likely]]
        #define D_UNLIKELY [[unlikely]]
    #elif defined(__has_cpp_attribute)
        #if __has_cpp_attribute(likely)
            #define D_LIKELY   [[likely]]
            #define D_UNLIKELY [[unlikely]]
        #endif
    #endif

    #ifndef D_LIKELY
        #define D_LIKELY
        #define D_UNLIKELY
    #endif  // D_LIKELY (fallback)
#endif  // D_LIKELY (outer guard)

#ifndef D_UNLIKELY
    #define D_UNLIKELY
#endif  // D_UNLIKELY (safety net)


// -----------------------------------------------------------------------------
// D_ASSUME(expr)                                                (C++23, §7.6.10)
//   Tells the compiler that `expr` is guaranteed to be true at the point
//   of the annotation, enabling optimisations that exploit the invariant.
//   Undefined behaviour if `expr` is false at runtime.
//
//   Resolution order:
//     1. C++23 - [[assume(…)]] is standard.
//     2. __has_cpp_attribute probe.
//     3. MSVC - __assume(…).
//     4. GCC 13+ - __attribute__((assume(…))).
//     5. Clang - __builtin_assume(…).
//     6. No-op fallback (void-cast to suppress unused warnings).
// -----------------------------------------------------------------------------
#ifndef D_ASSUME
    #if D_ENV_LANG_IS_CPP23_OR_HIGHER
        #define D_ASSUME(expr) [[assume(expr)]]
    #elif defined(__has_cpp_attribute)
        #if __has_cpp_attribute(assume)
            #define D_ASSUME(expr) [[assume(expr)]]
        #endif
    #endif

    #ifndef D_ASSUME
        #if defined(D_ENV_COMPILER_MSVC)
            #define D_ASSUME(expr) __assume(expr)
        #elif defined(D_ENV_COMPILER_GCC) && (__GNUC__ >= 13)
            #define D_ASSUME(expr) __attribute__((assume(expr)))
        #elif defined(D_ENV_COMPILER_CLANG)
            #define D_ASSUME(expr) __builtin_assume(expr)
        #else
            #define D_ASSUME(expr) ((void)(expr))
        #endif
    #endif  // D_ASSUME (fallback)
#endif  // D_ASSUME (outer guard)


// ===========================================================================
// II.  C ATTRIBUTES
// ===========================================================================

#else  // !defined(__cplusplus) - compiling as C


// -----------------------------------------------------------------------------
// D_NORETURN                                                     (C11 / C23)
//   C11 introduced _Noreturn (and <stdnoreturn.h>).  C23 added the
//   standard [[noreturn]] attribute and deprecated _Noreturn.
//
//   Resolution order:
//     1. C23 - [[noreturn]] is standard.
//     2. __has_c_attribute probe.
//     3. C11 - _Noreturn keyword.
//     4. GCC / Clang - __attribute__((noreturn)).
//     5. MSVC - __declspec(noreturn).
//     6. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_NORETURN
    #if D_ENV_LANG_IS_C23_OR_HIGHER
        #define D_NORETURN [[noreturn]]
    #elif defined(__has_c_attribute)
        #if __has_c_attribute(noreturn)
            #define D_NORETURN [[noreturn]]
        #endif
    #endif

    #ifndef D_NORETURN
        #if D_ENV_LANG_IS_C11_OR_HIGHER
            #define D_NORETURN _Noreturn
        #elif ( defined(D_ENV_COMPILER_GCC) ||  \
                defined(D_ENV_COMPILER_CLANG) )
            #define D_NORETURN __attribute__((noreturn))
        #elif defined(D_ENV_COMPILER_MSVC)
            #define D_NORETURN __declspec(noreturn)
        #else
            #define D_NORETURN
        #endif
    #endif  // D_NORETURN (fallback)
#endif  // D_NORETURN (outer guard)


// -----------------------------------------------------------------------------
// D_CARRIES_DEPENDENCY
//   Not part of any C standard.  Always a no-op in C mode.
// -----------------------------------------------------------------------------
#ifndef D_CARRIES_DEPENDENCY
    #define D_CARRIES_DEPENDENCY
#endif  // D_CARRIES_DEPENDENCY


// -----------------------------------------------------------------------------
// D_DEPRECATED / D_DEPRECATED_MSG(msg)                           (C23)
//   C23 added [[deprecated]] / [[deprecated("…")]].  Before C23, GCC and
//   Clang support __attribute__((deprecated)), MSVC has __declspec.
//
//   Resolution order:
//     1. C23 - [[deprecated]] / [[deprecated("…")]] is standard.
//     2. __has_c_attribute probe.
//     3. GCC / Clang - __attribute__((deprecated)) / ("…").
//     4. MSVC - __declspec(deprecated) / ("…").
//     5. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_DEPRECATED
    #if D_ENV_LANG_IS_C23_OR_HIGHER
        #define D_DEPRECATED              [[deprecated]]
        #define D_DEPRECATED_MSG(msg)     [[deprecated(msg)]]
    #elif defined(__has_c_attribute)
        #if __has_c_attribute(deprecated)
            #define D_DEPRECATED          [[deprecated]]
            #define D_DEPRECATED_MSG(msg) [[deprecated(msg)]]
        #endif
    #endif

    #ifndef D_DEPRECATED
        #if ( defined(D_ENV_COMPILER_GCC) ||  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_DEPRECATED              __attribute__((deprecated))
            #define D_DEPRECATED_MSG(msg)     __attribute__((deprecated(msg)))
        #elif defined(D_ENV_COMPILER_MSVC)
            #define D_DEPRECATED              __declspec(deprecated)
            #define D_DEPRECATED_MSG(msg)     __declspec(deprecated(msg))
        #else
            #define D_DEPRECATED
            #define D_DEPRECATED_MSG(msg)
        #endif
    #endif  // D_DEPRECATED (fallback)
#endif  // D_DEPRECATED (outer guard)

#ifndef D_DEPRECATED_MSG
    #define D_DEPRECATED_MSG(msg) D_DEPRECATED
#endif  // D_DEPRECATED_MSG (safety net)


// -----------------------------------------------------------------------------
// D_FALLTHROUGH                                                  (C23)
//   C23 added [[fallthrough]].  Before C23, GCC 7+ and Clang 3.6+
//   accept __attribute__((fallthrough)) in C mode.
//
//   Resolution order:
//     1. C23 - [[fallthrough]] is standard.
//     2. __has_c_attribute probe.
//     3. GCC / Clang - __attribute__((fallthrough)).
//     4. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_FALLTHROUGH
    #if D_ENV_LANG_IS_C23_OR_HIGHER
        #define D_FALLTHROUGH [[fallthrough]]
    #elif defined(__has_c_attribute)
        #if __has_c_attribute(fallthrough)
            #define D_FALLTHROUGH [[fallthrough]]
        #endif
    #endif

    #ifndef D_FALLTHROUGH
        #if ( defined(D_ENV_COMPILER_GCC) ||  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_FALLTHROUGH __attribute__((fallthrough))
        #else
            #define D_FALLTHROUGH
        #endif
    #endif  // D_FALLTHROUGH (fallback)
#endif  // D_FALLTHROUGH (outer guard)


// -----------------------------------------------------------------------------
// D_NODISCARD / D_NODISCARD_MSG(msg)                             (C23)
//   C23 added [[nodiscard]] / [[nodiscard("…")]].  Before C23, GCC and
//   Clang support __attribute__((warn_unused_result)) in C mode.
//
//   Resolution order:
//     1. C23 - [[nodiscard]] / [[nodiscard("…")]] is standard.
//     2. __has_c_attribute probe.
//     3. GCC / Clang - __attribute__((warn_unused_result)).
//     4. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_NODISCARD
    #if D_ENV_LANG_IS_C23_OR_HIGHER
        #define D_NODISCARD [[nodiscard]]
    #elif defined(__has_c_attribute)
        #if __has_c_attribute(nodiscard)
            #define D_NODISCARD [[nodiscard]]
        #endif
    #endif

    #ifndef D_NODISCARD
        #if ( defined(D_ENV_COMPILER_GCC) ||  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_NODISCARD __attribute__((warn_unused_result))
        #else
            #define D_NODISCARD
        #endif
    #endif  // D_NODISCARD (fallback)
#endif  // D_NODISCARD (outer guard)

#ifndef D_NODISCARD_MSG
    #if D_ENV_LANG_IS_C23_OR_HIGHER
        #define D_NODISCARD_MSG(msg) [[nodiscard(msg)]]
    #elif defined(__has_c_attribute)
        #if __has_c_attribute(nodiscard)
            #define D_NODISCARD_MSG(msg) [[nodiscard(msg)]]
        #endif
    #endif

    #ifndef D_NODISCARD_MSG
        #define D_NODISCARD_MSG(msg) D_NODISCARD
    #endif  // D_NODISCARD_MSG (fallback)
#endif  // D_NODISCARD_MSG (outer guard)


// -----------------------------------------------------------------------------
// D_MAYBE_UNUSED                                                 (C23)
//   C23 added [[maybe_unused]].  Before C23, GCC and Clang support
//   __attribute__((unused)) in C mode.
//
//   Resolution order:
//     1. C23 - [[maybe_unused]] is standard.
//     2. __has_c_attribute probe.
//     3. GCC / Clang - __attribute__((unused)).
//     4. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_MAYBE_UNUSED
    #if D_ENV_LANG_IS_C23_OR_HIGHER
        #define D_MAYBE_UNUSED [[maybe_unused]]
    #elif defined(__has_c_attribute)
        #if __has_c_attribute(maybe_unused)
            #define D_MAYBE_UNUSED [[maybe_unused]]
        #endif
    #endif

    #ifndef D_MAYBE_UNUSED
        #if ( defined(D_ENV_COMPILER_GCC) ||  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_MAYBE_UNUSED __attribute__((unused))
        #else
            #define D_MAYBE_UNUSED
        #endif
    #endif  // D_MAYBE_UNUSED (fallback)
#endif  // D_MAYBE_UNUSED (outer guard)


// -----------------------------------------------------------------------------
// D_NO_UNIQUE_ADDRESS
//   Not part of any C standard (the concept is meaningless for C structs).
//   Always a no-op in C mode.
// -----------------------------------------------------------------------------
#ifndef D_NO_UNIQUE_ADDRESS
    #define D_NO_UNIQUE_ADDRESS
#endif  // D_NO_UNIQUE_ADDRESS


// -----------------------------------------------------------------------------
// D_LIKELY / D_UNLIKELY
//   Not part of any C standard as statement attributes.  Always a no-op
//   in C mode.  (Use __builtin_expect for expression-level hints.)
// -----------------------------------------------------------------------------
#ifndef D_LIKELY
    #define D_LIKELY
#endif
#ifndef D_UNLIKELY
    #define D_UNLIKELY
#endif


// -----------------------------------------------------------------------------
// D_ASSUME(expr)
//   Not standardised in C.  Compiler intrinsics are still available.
//
//   Resolution order:
//     1. MSVC - __assume(…).
//     2. GCC 13+ - __attribute__((assume(…))).
//     3. Clang - __builtin_assume(…).
//     4. No-op fallback (void-cast to suppress unused warnings).
// -----------------------------------------------------------------------------
#ifndef D_ASSUME
    #if defined(D_ENV_COMPILER_MSVC)
        #define D_ASSUME(expr) __assume(expr)
    #elif defined(D_ENV_COMPILER_GCC) && (__GNUC__ >= 13)
        #define D_ASSUME(expr) __attribute__((assume(expr)))
    #elif defined(D_ENV_COMPILER_CLANG)
        #define D_ASSUME(expr) __builtin_assume(expr)
    #else
        #define D_ASSUME(expr) ((void)(expr))
    #endif
#endif  // D_ASSUME


#endif  // __cplusplus


#endif  // DJINTERP_ENV_ATTRIBUTES_
