/*******************************************************************************
* djinterp [re_std]                                                   config.hpp
*
* re_std's own configuration: everything it needs to know about the language,
* the compiler and the runtime, and the qualifier kit it spells its code with.
*   re_std stands on its own -- nothing under inc/re_std/ includes anything
* outside it -- so this header replaces what it used to take from djinterp's
* root and env layer. Every macro is spelled RE_STD_, never D_, so this kit and
* djinterp's can never collide, and each answers exactly as its djinterp
* counterpart did, so a build that included both before sees no difference.
*   The language-neutral half -- the level tests, the compiler, the
* fixed-width-integer headers, the long long gate, and the ISO strict knob --
* is config.h, which C reads too (dstdint.h does); this header includes it
* and adds what only C++ needs. Every value is pre-definable: #define it
* before the first re_std include to override detection. The two knobs
* (RE_STD_CFG_*) are what a build may set on purpose.
*
* path:      /inc/re_std/config.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.01
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOB
    ----
    1.  RE_STD_CFG_TESTING
2.  LANGUAGE
    --------
    1.  Language features
3.  RUNTIME AND LIBRARY
    -------------------
    1.  Exceptions and RTTI
    2.  Hosted headers
4.  QUALIFIERS
    ----------
    1.  constexpr
    2.  inline
    3.  Exception specifications and null pointers
    4.  Class members
    5.  Attributes
    6.  Static assertion
*/

#ifndef RE_STD_CONFIG_HPP
#define RE_STD_CONFIG_HPP 1

#ifndef __cplusplus
    #error "re_std is C++: config.hpp included in a C translation unit"
#endif  // __cplusplus

// re_std
#include "./config.h"  // RE_STD_CFG_ISO_STRICT, RE_STD_LANG_IS_*,
                       // RE_STD_COMPILER_*, RE_STD_HAS_HEADER_STDINT,
                       // RE_STD_HAS_LONG_LONG, RE_STD_LONG_LONG_DIAG_*,
                       // RE_STD_INTERNAL_CFG_IS_LITERAL


//==============================================================================
// 1.  KNOB
//==============================================================================
// The ISO strict knob is config.h's; this is the one only C++ reads.


// 1.1    Knob
//------------------------------------------------------------------------------
// 1.1.1
// RE_STD_CFG_TESTING
//   knob: 1 in a test build, where RE_STD_INLINE asks for inlining instead of
// forcing it, so every function keeps a body a debugger and a coverage tool
// can see. 0 by default; the literal 0 or 1. djinterp sets it from its own
// D_CFG_TESTING.
#ifndef RE_STD_CFG_TESTING
    #define RE_STD_CFG_TESTING 0
#endif  // RE_STD_CFG_TESTING

#if !RE_STD_INTERNAL_CFG_IS_LITERAL(RE_STD_CFG_TESTING)
    #error "RE_STD_CFG_TESTING must be 0 or 1"
#elif ( (RE_STD_CFG_TESTING != 0) &&                                           \
        (RE_STD_CFG_TESTING != 1) )
    #error "RE_STD_CFG_TESTING must be 0 or 1"
#endif


//==============================================================================
// 2.  LANGUAGE
//==============================================================================
// The level tests are config.h's.


// 2.1    Language features
//------------------------------------------------------------------------------
// 2.1.1
// RE_STD_LANG_HAS_RVALUE_REFERENCES, ..._VARIADIC_TEMPLATES,
// ..._ALIAS_TEMPLATES, ..._VARIABLE_TEMPLATES, ..._INLINE_VARIABLES
//   macro: 1 when the compiler reports the feature through its standard
// feature-test macro (__cpp_rvalue_references, ...), else 0.
#ifndef RE_STD_LANG_HAS_RVALUE_REFERENCES
    #ifdef __cpp_rvalue_references
        #define RE_STD_LANG_HAS_RVALUE_REFERENCES 1
    #else
        #define RE_STD_LANG_HAS_RVALUE_REFERENCES 0
    #endif  // __cpp_rvalue_references
#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES

#ifndef RE_STD_LANG_HAS_VARIADIC_TEMPLATES
    #ifdef __cpp_variadic_templates
        #define RE_STD_LANG_HAS_VARIADIC_TEMPLATES 1
    #else
        #define RE_STD_LANG_HAS_VARIADIC_TEMPLATES 0
    #endif  // __cpp_variadic_templates
#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES

#ifndef RE_STD_LANG_HAS_ALIAS_TEMPLATES
    #ifdef __cpp_alias_templates
        #define RE_STD_LANG_HAS_ALIAS_TEMPLATES 1
    #else
        #define RE_STD_LANG_HAS_ALIAS_TEMPLATES 0
    #endif  // __cpp_alias_templates
#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES

#ifndef RE_STD_LANG_HAS_VARIABLE_TEMPLATES
    #ifdef __cpp_variable_templates
        #define RE_STD_LANG_HAS_VARIABLE_TEMPLATES 1
    #else
        #define RE_STD_LANG_HAS_VARIABLE_TEMPLATES 0
    #endif  // __cpp_variable_templates
#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES

#ifndef RE_STD_LANG_HAS_INLINE_VARIABLES
    #ifdef __cpp_inline_variables
        #define RE_STD_LANG_HAS_INLINE_VARIABLES 1
    #else
        #define RE_STD_LANG_HAS_INLINE_VARIABLES 0
    #endif  // __cpp_inline_variables
#endif  // RE_STD_LANG_HAS_INLINE_VARIABLES


//==============================================================================
// 3.  RUNTIME AND LIBRARY
//==============================================================================


// 3.1    Exceptions and RTTI
//------------------------------------------------------------------------------
// 3.1.1
// RE_STD_HAS_EXCEPTIONS
//   macro: 1 when exceptions are enabled (so throw, try and <stdexcept>'s
// types may be used), 0 under -fno-exceptions and its kin.
#ifndef RE_STD_HAS_EXCEPTIONS
    #if ( (defined(__cpp_exceptions)) ||                                       \
          (defined(__EXCEPTIONS))     ||                                       \
          (defined(_CPPUNWIND)) )
        #define RE_STD_HAS_EXCEPTIONS 1
    #else
        #define RE_STD_HAS_EXCEPTIONS 0
    #endif
#endif  // RE_STD_HAS_EXCEPTIONS

// 3.1.2
// RE_STD_HAS_RTTI
//   macro: 1 when run-time type information is enabled (typeid, <typeinfo>,
// dynamic_cast to a non-base), 0 under -fno-rtti and its kin.
#ifndef RE_STD_HAS_RTTI
    #if ( (defined(__GXX_RTTI))       ||                                       \
          (defined(_CPPRTTI))         ||                                       \
          (defined(__INTEL_RTTI__)) )
        #define RE_STD_HAS_RTTI 1
    #else
        #define RE_STD_HAS_RTTI 0
    #endif
#endif  // RE_STD_HAS_RTTI

// 3.2    Hosted headers
//------------------------------------------------------------------------------
// 3.2.1
// RE_STD_HAS_HEADER_NEW, RE_STD_HAS_HEADER_UTILITY
//   macro: 1 when the implementation supplies <new> (<utility>). Every hosted
// C++ implementation does; define one to 0 for a freestanding one without it.
#ifndef RE_STD_HAS_HEADER_NEW
    #define RE_STD_HAS_HEADER_NEW 1
#endif  // RE_STD_HAS_HEADER_NEW

#ifndef RE_STD_HAS_HEADER_UTILITY
    #define RE_STD_HAS_HEADER_UTILITY 1
#endif  // RE_STD_HAS_HEADER_UTILITY


//==============================================================================
// 4.  QUALIFIERS
//==============================================================================
// Each expands to the keyword where the language has it and to nothing (or
// the C++98 equivalent) where it does not, so one spelling serves every level.


// 4.1    constexpr
//------------------------------------------------------------------------------
// 4.1.1
// RE_STD_CONSTEXPR
//   qualifier: `constexpr` from C++11, empty below.
#ifndef RE_STD_CONSTEXPR
    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        #define RE_STD_CONSTEXPR            constexpr
    #else
        #define RE_STD_CONSTEXPR
    #endif
#endif  // RE_STD_CONSTEXPR

// 4.1.2
// RE_STD_CONSTEXPR_CPP14, RE_STD_CONSTEXPR_CPP17, RE_STD_CONSTEXPR_CPP20
//   qualifier: RE_STD_CONSTEXPR from the level whose constexpr rules the body
// needs (C++14: loops and locals; C++17: lambdas, if constexpr; C++20:
// try, virtual, allocation), empty below it.
#ifndef RE_STD_CONSTEXPR_CPP14
    #if RE_STD_LANG_IS_CPP14_OR_HIGHER
        #define RE_STD_CONSTEXPR_CPP14      RE_STD_CONSTEXPR
    #else
        #define RE_STD_CONSTEXPR_CPP14
    #endif
#endif  // RE_STD_CONSTEXPR_CPP14

#ifndef RE_STD_CONSTEXPR_CPP17
    #if RE_STD_LANG_IS_CPP17_OR_HIGHER
        #define RE_STD_CONSTEXPR_CPP17      RE_STD_CONSTEXPR
    #else
        #define RE_STD_CONSTEXPR_CPP17
    #endif
#endif  // RE_STD_CONSTEXPR_CPP17

#ifndef RE_STD_CONSTEXPR_CPP20
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        #define RE_STD_CONSTEXPR_CPP20      RE_STD_CONSTEXPR
    #else
        #define RE_STD_CONSTEXPR_CPP20
    #endif
#endif  // RE_STD_CONSTEXPR_CPP20

// 4.2    inline
//------------------------------------------------------------------------------
// 4.2.1
// RE_STD_INLINE
//   qualifier: `inline`, forced where the compiler can force it (GCC and Clang,
// and Intel's GCC-compatible front end, which identifies as GCC:
// always_inline; MSVC: __forceinline), and only asked for in a test build
// (RE_STD_CFG_TESTING).
#ifndef RE_STD_INLINE
    #if RE_STD_CFG_TESTING
        #define RE_STD_INLINE               inline
    #elif defined(RE_STD_COMPILER_MSVC)
        #define RE_STD_INLINE               __forceinline
    #elif ( (defined(RE_STD_COMPILER_GCC)) ||                                  \
            (defined(RE_STD_COMPILER_CLANG)) )
        #define RE_STD_INLINE                                                  \
            inline __attribute__((always_inline))
    #else
        #define RE_STD_INLINE               inline
    #endif
#endif  // RE_STD_INLINE

// 4.2.2
// RE_STD_CONSTEXPR_INLINE
//   qualifier: RE_STD_CONSTEXPR RE_STD_INLINE, for a header function that is
// constexpr where the language allows.
#ifndef RE_STD_CONSTEXPR_INLINE
    #define RE_STD_CONSTEXPR_INLINE         RE_STD_CONSTEXPR RE_STD_INLINE
#endif  // RE_STD_CONSTEXPR_INLINE

// 4.2.3
// RE_STD_INLINE_VAR
//   qualifier: `inline` on a variable from C++17, which has inline variables,
// empty below.
#ifndef RE_STD_INLINE_VAR
    #if RE_STD_LANG_IS_CPP17_OR_HIGHER
        #define RE_STD_INLINE_VAR           inline
    #else
        #define RE_STD_INLINE_VAR
    #endif
#endif  // RE_STD_INLINE_VAR

// 4.3    Exception specifications and null pointers
//------------------------------------------------------------------------------
// 4.3.1
// RE_STD_NOEXCEPT, RE_STD_NOEXCEPT_IF
//   qualifier: `noexcept` and `noexcept(cond)` from C++11, empty below.
#ifndef RE_STD_NOEXCEPT
    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        #define RE_STD_NOEXCEPT             noexcept
    #else
        #define RE_STD_NOEXCEPT
    #endif
#endif  // RE_STD_NOEXCEPT

#ifndef RE_STD_NOEXCEPT_IF
    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        #define RE_STD_NOEXCEPT_IF(cond)    noexcept(cond)
    #else
        #define RE_STD_NOEXCEPT_IF(cond)
    #endif
#endif  // RE_STD_NOEXCEPT_IF

// 4.3.2
// RE_STD_NULLPTR
//   macro: `nullptr` from C++11, `0` below.
#ifndef RE_STD_NULLPTR
    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        #define RE_STD_NULLPTR              nullptr
    #else
        #define RE_STD_NULLPTR              0
    #endif
#endif  // RE_STD_NULLPTR

// 4.4    Class members
//------------------------------------------------------------------------------
// 4.4.1
// RE_STD_STATIC_CONSTEXPR
//   qualifier: `static constexpr` from C++11, `static` below.
#ifndef RE_STD_STATIC_CONSTEXPR
    #define RE_STD_STATIC_CONSTEXPR         static RE_STD_CONSTEXPR
#endif  // RE_STD_STATIC_CONSTEXPR

// 4.4.2
// RE_STD_OVERRIDE
//   qualifier: `override` from C++11, empty below.
#ifndef RE_STD_OVERRIDE
    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        #define RE_STD_OVERRIDE             override
    #else
        #define RE_STD_OVERRIDE
    #endif
#endif  // RE_STD_OVERRIDE

// 4.4.3
// RE_STD_DELETED_FN
//   macro: marks a member that must not exist, taking its full declaration:
// `= delete` from C++11; below, the declaration alone, which the class never
// defines. Write it in a private: section at every level, so access is the
// same on every tier. From C++11 any use is a compile error naming the
// deleted function; below, a use from outside the class is an access error,
// and one from inside the class or a friend a link error.
#ifndef RE_STD_DELETED_FN
    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        #define RE_STD_DELETED_FN(decl)     decl = delete;
    #else
        #define RE_STD_DELETED_FN(decl)     decl;
    #endif
#endif  // RE_STD_DELETED_FN

// 4.5    Attributes
//------------------------------------------------------------------------------
// 4.5.1
// RE_STD_NODISCARD
//   attribute: [[nodiscard]] from C++17; Clang's warn_unused_result below it;
// [[nodiscard]] on GCC at C++11 and C++14 where it reports the attribute; else
// empty. __has_cpp_attribute is consulted only below C++17, never alone: GCC
// and Clang answer yes in modes whose grammar lacks the spelling.
#ifndef RE_STD_NODISCARD
    #if RE_STD_LANG_IS_CPP17_OR_HIGHER
        #define RE_STD_NODISCARD [[nodiscard]]
    #elif defined(RE_STD_COMPILER_CLANG)
        #define RE_STD_NODISCARD __attribute__((warn_unused_result))
    #elif ( (RE_STD_LANG_IS_CPP11_OR_HIGHER) &&                                \
            (defined(RE_STD_COMPILER_GCC))   &&                                \
            (defined(__has_cpp_attribute)) )
        #if __has_cpp_attribute(nodiscard)
            #define RE_STD_NODISCARD [[nodiscard]]
        #endif
    #endif

    #ifndef RE_STD_NODISCARD
        #define RE_STD_NODISCARD
    #endif  // RE_STD_NODISCARD
#endif  // RE_STD_NODISCARD

// 4.6    Static assertion
//------------------------------------------------------------------------------
// 4.6.1
// RE_STD_INTERNAL_CONCAT, RE_STD_INTERNAL_STATIC_ASSERT_UID
//   macro (internal): token pasting after expansion, and a unique suffix (the
// counter where there is one, else the line) for the C++98 fallback below.
#define RE_STD_INTERNAL_CONCAT_IMPL(a, b) a##b
#define RE_STD_INTERNAL_CONCAT(a, b)      RE_STD_INTERNAL_CONCAT_IMPL(a, b)

#if defined(__COUNTER__)
    #define RE_STD_INTERNAL_STATIC_ASSERT_UID __COUNTER__
#else
    #define RE_STD_INTERNAL_STATIC_ASSERT_UID __LINE__
#endif

// 4.6.2
// RE_STD_STATIC_ASSERT
//   macro: a compile-time assertion: static_assert from C++11; below, a
// typedef of an array whose size is negative when the condition is false.
#ifndef RE_STD_STATIC_ASSERT
    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        #define RE_STD_STATIC_ASSERT(cond, msg) static_assert((cond), msg)
    #else
        #define RE_STD_STATIC_ASSERT(cond, msg)                                \
            typedef char RE_STD_INTERNAL_CONCAT(                               \
                re_std_static_assertion_failed_,                               \
                RE_STD_INTERNAL_STATIC_ASSERT_UID)[(cond) ? 1 : -1]
    #endif
#endif  // RE_STD_STATIC_ASSERT


#endif  // RE_STD_CONFIG_HPP
