/******************************************************************************
* djinterp [c]                                                      djinterp.h
*
* djinterp C framework root header
*   Targets a C99 language floor and supplies the common facilities assumed by
* other modules: cross-platform scalar types, the qualifier kit, shared
* function-pointer typedefs, the global keyword vocabulary, and negative-
* indexing types and macros.
*
*
* path:      /inc/djinterp/c/djinterp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.11.12
*                                                          revised: 2026.09.09
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  GLOBAL KEYWORDS
    ---------------
    1.  Framework keywords
         1.  D_KEYWORD_FRAMEWORK_NAME
         2.  D_KEYWORD_BINARY
         3.  D_KEYWORD_CLI
         3.  D_KEYWORD_CONTAINER
         5.  D_KEYWORD_DATABASE
         6.  D_KEYWORD_ERROR
         7   D_KEYWORD_EXCEPTION
         8.  D_KEYWORD_FUNCTIONAL
         9.  D_KEYWORD_INTERNAL
         10. D_KEYWORD_MATH
         11. D_KEYWORD_MESSAGE
         12. D_KEYWORD_TEST
         13. D_KEYWORD_TEXT
         14. D_KEYWORD_TESTING
         15. D_KEYWORD_USER_INTERFACE
         16. D_KEYWORD_WARNING

2.  FUNDAMENTAL TYPES
    -----------------
    1.  Cross-platform compatibility
         1.  bool
         2.  D_RESTRICT
         3.  ssize_t
    2.  Static assertion
         1.  D_STATIC_ASSERT
    3.  Qualifier kit
         1.  D_STATIC
         2.  D_INLINE
         3.  D_STATIC_INLINE
         4.  D_EXTERN_C
         5.  D_EXTERN_C_BEGIN / D_EXTERN_C_END
         6.  D_NOINLINE
         7.  D_NODISCARD
    4.  Function pointers
         1.  fn_apply
         2.  fn_apply_context
         3.  fn_callback
         4.  fn_comparator
         5.  fn_free
         6.  fn_print
         7.  fn_to_string
         8.  fn_write

3.  DEFINED CONSTANTS
    -----------------
    1.  Framework constants
         1.  D_SUCCESS
         2.  D_FAILURE
         3.  D_ENABLED
         4.  D_DISABLED
         5.  D_INDENT

4.  NEGATIVE INDEXING
    -----------------
    1.  Index type
         1.  d_index
    2.  Conversion and validation
         1.  d_index_convert_fast / d_index_convert_safe
         2.  d_index_is_valid

5.  INDEXING MACROS AND UTILITIES
    -----------------------------
    1.  Array size macros
         1.  D_ARRAY_STATIC_SIZE
         2.  D_ARRAY_TOTAL_SIZE
    2.  Index clamping and bounds tests
         1.  D_CLAMP_INDEX
         2.  D_INDEX_IN_BOUNDS
         3.  D_IS_VALID_INDEX
         4.  D_IS_VALID_INDEX_N
    3.  Negative-index access macros
         1.  D_SAFE_ARR_IDX
         2.  D_NEG_IDX
         3.  D_ARR_IDX
*/

#ifndef DJINTERP_C_
#define DJINTERP_C_ 1

// std
#include <limits.h>                         // LONG_MAX, LLONG_MAX
#include <stddef.h>                         // size_t
#include <stdint.h>                         // fixed-width integer types
// djinterp
#include "../env/env.h"                     // language/compiler/OS detection
#include "../env/c/env_attributes.h"        // standard attribute detection
#include "../env/c/env_vendor_attributes.h" // vendor attribute detection
#include "../config/cfg_qualifiers.h"       // qualifier configuration
#include "./dmacro.h"                       // macro utilities


//==============================================================================
// 1.  GLOBAL KEYWORDS
//==============================================================================

// 1.1    Framework keywords
//------------------------------------------------------------------------------

// 1.1.1   D_KEYWORD_FRAMEWORK_NAME
//   constant: keyword corresponding to the name of this framework.
#define D_KEYWORD_FRAMEWORK_NAME    djinterp

// 1.1.2   D_KEYWORD_BINARY
//   keyword: resolves to `binary`.
// used to specify that a unit of code pertains to binary encoding and/or
// decoding.
#define D_KEYWORD_BINARY            binary

// 1.1.3   D_KEYWORD_CLI
//   keyword: resolves to `cli`.
// used to specify that a unit of code pertains to a command-line interface.
#define D_KEYWORD_CLI               cli

// 1.1.4   D_KEYWORD_CONTAINER
//   keyword: resolves to `container`.
// used to specify that a unit of code pertains to a command-line interface.
#define D_KEYWORD_CONTAINER         container

// 1.1.5   D_KEYWORD_DATABASE
//   keyword: resolves to `database`.
// corresponds to code dealing with databases and database systems.
#define D_KEYWORD_DATABASE          database

// 1.1.6   D_KEYWORD_ERROR
//   keyword: resolves to `error`.
// used for an invalid state; from Latin `errare` -- to wander.
#define D_KEYWORD_ERROR             error

// 1.1.7   D_KEYWORD_EXCEPTION
//   keyword: resolves to `exception`.
// used to indicate a severe deviation from a valid state.
#define D_KEYWORD_EXCEPTION         exception

// 1.1.8   D_KEYWORD_FUNCTIONAL
//   keyword: resolves to `functional`.
// used to specify that a unit of code pertains to functional programming.
#define D_KEYWORD_FUNCTIONAL        functional

// 1.1.9   D_KEYWORD_INTERNAL
//   keyword: resolves to `internal`.
// used to specify that a unit of code is part of the `internal` namespace,
// hiding the details of implementation from regular use.
#define D_KEYWORD_INTERNAL          internal

// 1.1.10  D_KEYWORD_MATH
//   keyword: resolves to `math`.
// used for variables, macros, namespaces, etc. that pertain to the `math`
// submodule.
#define D_KEYWORD_MATH              math

// 1.1.10  D_KEYWORD_MESSAGE
//   keyword: resolves to `message`.
// used for variables, macros, namespaces, etc. that convey (usually string-
// based) human-readable information that is conveyed to the user, often
// (but not limited to) debugging and error-handling.
#define D_KEYWORD_MESSAGE           message

// 1.1.11  D_KEYWORD_TEST
//   keyword: resolves to `test`.
// used to specify that a unit of code is part of unit testing.
#define D_KEYWORD_TEST              test

// 1.1.12  D_KEYWORD_TESTING
//   keyword: resolves to `testing`; used to signify that a code segment
// pertains to unit testing.
#define D_KEYWORD_TESTING           testing

// 1.1.13  D_KEYWORD_TEXT
//   keyword: resolves to `text`.
// used to specify that a unit of code has to do with textual functionality,
// including (but not limited to): converting, encoding, processing, 
// and rendering.
#define D_KEYWORD_TEXT              text

// 1.1.14  D_KEYWORD_USER_INTERFACE
//   keyword: resolves to `ui`.
// used to specify that a unit of code is part of the user interface.
#define D_KEYWORD_USER_INTERFACE    ui

// 1.1.15  D_KEYWORD_WARNING
//   keyword: resolves to `warning`.
// used to specify that the program has an anomalous state that is not
// necessarily the end of the world.
#define D_KEYWORD_WARNING           warning


//==============================================================================
// 2.  FUNDAMENTAL TYPES
//==============================================================================


// 2.1    Cross-platform compatibility
//------------------------------------------------------------------------------

// 2.1.1  bool
//   type: portable boolean type for the framework's supported C/C++ modes.
#if ( defined(__bool_true_false_are_defined)  ||                              \
      defined(bool)                           ||                              \
      defined(true)                           ||                              \
      defined(false) )
    // `stdbool.h` has already been included, do nothing

#elif D_ENV_LANG_IS_C23_OR_HIGHER
    // C23 or newer - bool is a built-in keyword
    // nothing to do, C23 has bool, true, false as keywords

#elif D_ENV_LANG_IS_C99_OR_HIGHER
    // C99 or newer - use the standard header
    // std
    #include <stdbool.h>  // bool, true, false
#elif defined(__cplusplus)
    // C++ has built-in bool
    // nothing to do, C++ already has bool, true, false
#else
    // C99 is the framework language floor, so <stdbool.h> is guaranteed
    // and the hand-rolled typedef fallbacks that used to live here are
    // unreachable. Keeping them would be keeping code no build can enter.
    // std
    #include <stdbool.h>  // bool, true, false
#endif

// 2.1.2  D_RESTRICT
//   qualifier: portable restrict/no-alias spelling for C and C++.
// `restrict` is a C keyword but not a C++ keyword. C uses the standard keyword;
// C++ uses the compiler extension when one is available.
#if !defined(__cplusplus)
    #define D_RESTRICT restrict
#elif ( defined(D_ENV_COMPILER_GCC) ||                                        \
        defined(D_ENV_COMPILER_CLANG) )
    #define D_RESTRICT __restrict__
#elif defined(D_ENV_COMPILER_MSVC)
    #define D_RESTRICT __restrict
#else
    // unknown C++ compiler: no alias qualifier; correct but not optimal.
    #define D_RESTRICT
#endif  // !defined(__cplusplus)

// 2.1.3  ssize_t
//   type: signed integer type corresponding to `size_t`.
#ifndef _SSIZE_T_DEFINED
    #ifndef _SSIZE_T
        #ifndef __ssize_t_defined
            #ifndef _SSIZE_T_
                #ifndef ssize_t
                    #if D_ENV_OS_USING_WINDOWS64
                        typedef long long ssize_t;
                        #define SSIZE_MAX LLONG_MAX
                    #else
                        typedef long ssize_t;
                        #define SSIZE_MAX LONG_MAX
                    #endif  // defined(D_ENV_OS_USING_WINDOWS64)

                    #define _SSIZE_T_
                    #define _SSIZE_T_DEFINED
                    #define __ssize_t_defined
                    #define _SSIZE_T
                #endif  // ssize_t
            #endif  // _SSIZE_T_
        #endif  // __ssize_t_defined
    #endif  // _SSIZE_T
#endif  // _SSIZE_T_DEFINED

// 2.2    Static assertion
//------------------------------------------------------------------------------

// D_INTERNAL_STATIC_ASSERT_UID
//   macro (internal): unique suffix for fallback static-assert declarations.
#if defined(__COUNTER__)
    #define D_INTERNAL_STATIC_ASSERT_UID __COUNTER__
#else
    #define D_INTERNAL_STATIC_ASSERT_UID __LINE__
#endif

// 2.2.1  D_STATIC_ASSERT
//   macro: portable compile-time assertion across supported C and C++ modes.
#ifndef D_STATIC_ASSERT
    #if defined(D_ENV_LANG_DETECTED_CPP)
        // >=C++11 has the static_assert keyword.
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            #define D_STATIC_ASSERT(cond, msg) static_assert((cond), msg)
        #else
            #define D_STATIC_ASSERT(cond, msg)                                \
                typedef char D_CONCAT(static_assertion_failed_,               \
                                      D_INTERNAL_STATIC_ASSERT_UID)[(cond)    \
                                          ? 1                                 \
                                          : -1]
        #endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
    #else
        // C11+ has `_Static_assert`.
        #if D_ENV_LANG_IS_C11_OR_HIGHER
            #define D_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
        #else
            #define D_STATIC_ASSERT(cond, msg)                                \
                typedef char D_CONCAT(static_assertion_failed_,               \
                                      D_INTERNAL_STATIC_ASSERT_UID)[(cond)    \
                                          ? 1                                 \
                                          : -1]
        #endif
    #endif
#endif  // D_STATIC_ASSERT

// 2.3    Qualifier kit
//------------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Storage / linkage qualifiers
//    public: D_STATIC, D_INLINE, D_STATIC_INLINE, D_EXTERN_C,
//            D_EXTERN_C_BEGIN / D_EXTERN_C_END (+ C constexpr fallbacks).
//    requires qual_cfg.h to have been included first (it supplies the
//  D_INTERNAL_CFG_* gates and, via the config layer, D_CFG_TESTING).
//
//    why `inline` needs care: in C++ / on MSVC an `inline` function is merged
//  across TUs (header-safe alone); in standard C a bare header `inline` emits
//  no symbol, so `static inline` is the only header-only-safe spelling (a bare
//  one gives an undefined-reference LINKER error at -O0 or when its address is
//  taken). So D_INLINE carries `static` in C but not in C++/MSVC -- which is
//  why the compounds are composed from an inline specifier that never contains
//  `static`, with exactly one `static` prepended where required (no naive
//  `static` + `static inline` = "multiple storage classes" in C). The
//  testing-vs-release difference is exactly the force-inline hint: present
//  outside testing, dropped in testing so functions stay real / breakpoint-
//  able / coverable, never at the cost of linker safety.
// ----------------------------------------------------------------------------

// --- internal inline-specifier resolution (compiler -> language -> mode) ---
//   D_INTERNAL_INLINE_QUAL          inline keyword (+force outside testing);
//                                   NEVER contains `static`.
//   D_INTERNAL_INLINE_NEEDS_STATIC  1 when a header-defined function needs
//                                   `static` for linker safety (C's inline).

// D_INTERNAL_QUAL_TESTING
//   macro (internal): 0/1 testing signal. Respects a value already supplied by
// qual_cfg.h; otherwise uses the config layer's canonical D_CFG_TESTING, and
// failing that normalizes the raw D_TESTING build flag (empty-safe).
#ifndef D_INTERNAL_QUAL_TESTING
    #if defined(D_CFG_TESTING)
        #define D_INTERNAL_QUAL_TESTING     (D_CFG_TESTING == 1)
    #else
        #define D_INTERNAL_QUAL_TESTING     ((D_TESTING + 0) == 1)
    #endif
#endif  // D_INTERNAL_QUAL_TESTING

#if defined(D_ENV_COMPILER_MSVC)
    // MSVC inline (C or C++) is COMDAT-merged -> linker-safe, no `static`.
    #if D_INTERNAL_QUAL_TESTING
        #define D_INTERNAL_INLINE_QUAL      __inline
    #else
        #define D_INTERNAL_INLINE_QUAL      __forceinline
    #endif
    #define D_INTERNAL_INLINE_NEEDS_STATIC  0

#elif ( defined(D_ENV_COMPILER_GCC) ||                                        \
        defined(D_ENV_COMPILER_CLANG) )
    // covers GCC, Clang, Apple Clang, and both Intel front-ends.
    #if defined(__cplusplus)
        // C++ inline is one merged definition -> linker-safe, no `static`.
        #if D_INTERNAL_QUAL_TESTING
            #define D_INTERNAL_INLINE_QUAL  inline
        #else
            #define D_INTERNAL_INLINE_QUAL                                     \
                inline __attribute__((always_inline))
        #endif
        #define D_INTERNAL_INLINE_NEEDS_STATIC 0
    #else
        // C: `__inline__` is accepted in every C mode (incl. -std=c89
        // -pedantic) with no diagnostic, unlike bare `inline`.
        #if D_INTERNAL_QUAL_TESTING
            #define D_INTERNAL_INLINE_QUAL  __inline__
        #else
            #define D_INTERNAL_INLINE_QUAL                                     \
                __inline__ __attribute__((always_inline))
        #endif
        #define D_INTERNAL_INLINE_NEEDS_STATIC 1
    #endif

#else
    // unknown compiler: hint-only (no portable force), still fully correct.
    // `inline` exists in both languages at the floor, so there is no case
    // left in which it has to be spelled as nothing.
    #define D_INTERNAL_INLINE_QUAL          inline
    #if defined(__cplusplus)
        #define D_INTERNAL_INLINE_NEEDS_STATIC 0
    #else
        #define D_INTERNAL_INLINE_NEEDS_STATIC 1
    #endif
#endif

// D_INTERNAL_INLINE_STATIC_PREFIX
//   macro (internal): the `static` (or nothing) prepended to a bare D_INLINE.
#if D_INTERNAL_INLINE_NEEDS_STATIC
    #define D_INTERNAL_INLINE_STATIC_PREFIX static
#else
    #define D_INTERNAL_INLINE_STATIC_PREFIX
#endif


// --- public qualifiers (gated by qual_cfg.h; each honors a user override) ---

// 2.3.1  D_STATIC
//   qualifier: internal linkage (`static`); identical spelling in C and C++.
#if ( !defined(D_STATIC) &&                                                   \
      (D_INTERNAL_CFG_STATIC == 1) )
    #define D_STATIC                        static
#endif

// 2.3.2  D_INLINE
//   qualifier: header-safe inline. `inline` (+force outside testing) in C++ and
// on MSVC; `static inline` (+force outside testing) in standard C -- the only
// header-only spelling that never triggers a linker error. Safe on a header-
// defined function standalone in either language.
#if ( !defined(D_INLINE) && (                                                 \
      D_INTERNAL_CFG_INLINE == 1) )
    #define D_INLINE    D_INTERNAL_INLINE_STATIC_PREFIX D_INTERNAL_INLINE_QUAL
#endif

// 2.3.3  D_STATIC_INLINE
//   qualifier: internal linkage + inline, composed as exactly one `static`
// plus the inline specifier. Because the specifier never contains `static`,
// this is always a single well-formed `static inline` (no double-`static`).
// in standard C it coincides with D_INLINE, which is correct: a C header inline
// is always a static inline.
#if ( !defined(D_STATIC_INLINE) &&                                            \
      (D_INTERNAL_CFG_INLINE == 1) )
    #define D_STATIC_INLINE                 static D_INTERNAL_INLINE_QUAL
#endif

// 2.3.4  D_EXTERN_C
//   qualifier: C linkage for a SINGLE declaration.  `extern "C"` under C++,
// nothing under C, so one spelling serves both languages:
//     D_EXTERN_C int d_foo(void);
#if ( !defined(D_EXTERN_C) &&                                                 \
      (D_INTERNAL_CFG_EXTERN_C == 1) )
    #if D_INTERNAL_QUAL_EXTERN_C_LINKAGE
        #define D_EXTERN_C                  extern "C"
    #else
        #define D_EXTERN_C
    #endif
#endif

// 2.3.5  D_EXTERN_C_BEGIN / D_EXTERN_C_END
//   macro: C linkage for a BLOCK of declarations -- the form a header wants,
// since it costs one line at each end rather than a qualifier on every line.
// both expand to nothing under C, so a C-only build sees no trace of them and
// the header needs no `#ifdef __cplusplus` of its own.
//     D_EXTERN_C_BEGIN
//     int d_foo(void);
//     D_EXTERN_C_END
//   note the asymmetry with D_EXTERN_C: `extern "C" { ... }` is a linkage
// block, `extern "C" decl;` is a linkage specification on one declaration.
// they nest and agree, so mixing the two forms in one header is well-formed.
#if ( !defined(D_EXTERN_C_BEGIN) &&                                           \
      (D_INTERNAL_CFG_EXTERN_C == 1) )
    #if D_INTERNAL_QUAL_EXTERN_C_LINKAGE
        #define D_EXTERN_C_BEGIN            extern "C" {
    #else
        #define D_EXTERN_C_BEGIN
    #endif
#endif

#if ( !defined(D_EXTERN_C_END) &&                                             \
      (D_INTERNAL_CFG_EXTERN_C == 1) )
    #if D_INTERNAL_QUAL_EXTERN_C_LINKAGE
        #define D_EXTERN_C_END              }
    #else
        #define D_EXTERN_C_END
    #endif
#endif


// --- C-language fallback for the constexpr family ------------------------
//   in C++ these are defined in djinterp.hpp. In C the concept does not exist
// before C23 (and C23 constexpr is objects-only), so provide header-safe
// fallbacks for shared C/C++ headers. Defined directly (not by naive
// composition) to avoid a double-`static` in C.
#if ( !defined(__cplusplus) &&                                                \
      (D_INTERNAL_CFG_CONSTEXPR == 1) )
    #ifndef D_CONSTEXPR
        #if D_ENV_LANG_IS_C23_OR_HIGHER
            #define D_CONSTEXPR             constexpr   // C23: objects only
        #else
            #define D_CONSTEXPR                         // no constexpr in C
        #endif
    #endif  // D_CONSTEXPR
    #ifndef D_STATIC_CONSTEXPR
        #define D_STATIC_CONSTEXPR          static D_CONSTEXPR
    #endif  // D_STATIC_CONSTEXPR
    #ifndef D_CONSTEXPR_INLINE
        #define D_CONSTEXPR_INLINE          D_INLINE      // C: == static inline
    #endif  // D_CONSTEXPR_INLINE
    #ifndef D_STATIC_CONSTEXPR_INLINE
        #define D_STATIC_CONSTEXPR_INLINE   D_STATIC_INLINE
    #endif  // D_STATIC_CONSTEXPR_INLINE
#endif

// --- C-language spelling of the inline-VARIABLE qualifiers ---------------
//   C has no inline variable at any standard level -- C's `inline` is a
// function specifier only, and even C23's constexpr is an object qualifier
// rather than a linkage one. So both spellings are EMPTY here, for the same
// reason D_EXTERN_C is: a shared C/C++ header can then write them
// unconditionally and a C-only build sees no trace of them. The C++
// definitions, which are the ones that do something, live in djinterp.hpp.
//   guarded on !__cplusplus because djinterp.hpp includes THIS header first;
// an unguarded definition here would win its #ifndef and silently disable the
// C++17 spelling.
#if !defined(__cplusplus)
    #if ( !defined(D_INLINE_VAR) &&                                           \
          (D_INTERNAL_CFG_INLINE == 1) )
        // no inline variables in C
        #define D_INLINE_VAR
    #endif
    #if ( !defined(D_CONSTEXPR_INLINE_VAR) &&                                 \
          (D_INTERNAL_CFG_CONSTEXPR == 1)  &&                                 \
          (D_INTERNAL_CFG_INLINE == 1) )
        #define D_CONSTEXPR_INLINE_VAR      D_CONSTEXPR
    #endif
#endif

// 2.3.6  D_NOINLINE
//   qualifier: prevents inlining for debugging and profiling.
#if defined(D_ENV_COMPILER_MSVC)
    #define D_NOINLINE          __declspec(noinline)
#elif ( defined(D_ENV_COMPILER_GCC) ||                                        \
        defined(D_ENV_COMPILER_CLANG) )
    #define D_NOINLINE          __attribute__((noinline))
#else
    #define D_NOINLINE
#endif

// 2.3.7  D_NODISCARD
//   qualifier: indicates that a function return value should not be silently
// discarded. The compiler will emit a warning (or error, depending
// on settings) if the caller ignores the return value.
//
//   resolution order:
//     1.  C++17  / C23  - [[nodiscard]] is standard.
//     2.  __has_cpp_attribute / __has_c_attribute - catches compilers
//        that support the attribute before the standard mandates it.
//     3.  GCC / Clang - __attribute__((warn_unused_result)) in both
//        C and C++ modes, all the way back to GCC 3.4 / Clang 3.0.
//     4.  Everything else - empty (no diagnostic, but no breakage).
//
//   pre-definable: users may #define D_NODISCARD before including
// this header to override the detected value.
#ifndef D_NODISCARD
    // ---- standard attribute form ----
    #if defined(__cplusplus)
        #if D_ENV_LANG_IS_CPP17_OR_HIGHER
            #define D_NODISCARD [[nodiscard]]
        #elif defined(__has_cpp_attribute)
            #if __has_cpp_attribute(nodiscard)
                #define D_NODISCARD [[nodiscard]]
            #endif
        #endif
    #else
        #if D_ENV_LANG_IS_C23_OR_HIGHER
            #define D_NODISCARD [[nodiscard]]
        #elif defined(__has_c_attribute)
            #if __has_c_attribute(nodiscard)
                #define D_NODISCARD [[nodiscard]]
            #endif
        #endif
    #endif  // __cplusplus

    // ---- compiler-specific fallback ----
    #ifndef D_NODISCARD
        #if ( defined(D_ENV_COMPILER_GCC) ||                                  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_NODISCARD __attribute__((warn_unused_result))
        #else
            #define D_NODISCARD
        #endif
    #endif  // D_NODISCARD (fallback)
#endif  // D_NODISCARD (outer guard)

// 2.4    Function pointers
//------------------------------------------------------------------------------
// 2.4.1  fn_apply
//   typedef: function pointer type for applying an operation to an element.
typedef void (*fn_apply)(void* _element);

// 2.4.2  fn_apply_context
//   typedef: function pointer type for applying an operation to an element
// with additional context.
typedef void (*fn_apply_context)(void* _element, void* _context);

// 2.4.3  fn_callback
//   typedef: generic callback function pointer with optional context.
// `_context` may be NULL.
typedef void (*fn_callback)(void* _context);

// 2.4.4  fn_comparator
//   typedef: function pointer for ordering two values of identical type.
// returns a value less than, equal to, or greater than 0 when the first value
// compares less than, equal to, or greater than the second, respectively.
typedef int (*fn_comparator)(const void* _a, const void* _b);

// 2.4.5  fn_free
//   typedef: function pointer used to free associated memory.
typedef void (*fn_free)(void* _ptr);

// 2.4.6  fn_print
//   typedef: function pointer used to print a value to the desired output.
typedef void (*fn_print)(void* _type, ...);

// 2.4.7  fn_to_string
//   typedef: function pointer returning a string representation.
typedef const char* (*fn_to_string)(void);

// 2.4.8  fn_write
//   typedef: function pointer that writes to a buffer.
typedef size_t (*fn_write)(char* const _buffer, size_t _size);


//==============================================================================
// 3.  DEFINED CONSTANTS
//==============================================================================

// 3.1    Framework constants
//------------------------------------------------------------------------------

// 3.1.1  D_SUCCESS
//   constant: corresponds to a SUCCESSFUL operation; evaluates to `true`.
#define D_SUCCESS  true

// 3.1.2  D_FAILURE
//   constant: corresponds to a FAILED operation; evaluates to `false`.
#define D_FAILURE  false

// 3.1.3  D_ENABLED
//   constant: corresponds to a capability that IS compiled in; evaluates to
// `true`. Not a success value -- see D_SUCCESS for that.
#define D_ENABLED  true

// 3.1.4  D_DISABLED
//   constant: corresponds to a capability that is NOT compiled in; evaluates
// to `false`. Not a failure value -- see D_FAILURE for that.
#define D_DISABLED false

// 3.1.5  D_INDENT
//   constant: string corresponding to one (1) level of indentation. Defaults
// to two single spaces.
#ifndef D_INDENT
#   define D_INDENT "  "
#endif  // D_INDENT


//==============================================================================
// 4.  NEGATIVE INDEXING
//==============================================================================

// 4.1    Index type
//------------------------------------------------------------------------------

// 4.1.1  d_index
//   type: a type corresponding to a vector index that may be negative (in
// addition to the traditional positive or zero vector indices).
//   A negative `d_index` counts from the last element back toward 0, rather
// than forward as a positive index does. An index of -1 is the last element;
// an index of -n, for a count of n, is element 0.
typedef ssize_t d_index;

// 4.2    Conversion and validation
//------------------------------------------------------------------------------
D_EXTERN_C_BEGIN

// 4.2.1  d_index_convert_fast / d_index_convert_safe
size_t d_index_convert_fast(d_index _index,
                            size_t  _count);
bool   d_index_convert_safe(d_index _index,
                            size_t  _count,
                            size_t* _destination);

// 4.2.2  d_index_is_valid
bool   d_index_is_valid(d_index _index,
                        size_t  _count);
D_EXTERN_C_END


//==============================================================================
// 5.  INDEXING MACROS AND UTILITIES
//==============================================================================

// 5.1    Array size macros
//------------------------------------------------------------------------------

// 5.1.1  D_ARRAY_STATIC_SIZE
//   macro: the number of ELEMENTS in a stack-allocated array whose size is
// known at compile time. Equal to the quotient of the array's total size and
// the size of one element -- a count, not a byte total.
// note: decays to nonsense if handed a pointer rather than an array.
#define D_ARRAY_STATIC_SIZE(_array)                                           \
    ((size_t)(sizeof(_array) / sizeof((_array)[0])))

// 5.1.2  D_ARRAY_TOTAL_SIZE
//   macro: shorthand for calculating the total memory occupied, in bytes, by
// a vector of elements.
// equal to the product of `_element_size` and `_elements_count`.
#define D_ARRAY_TOTAL_SIZE(_element_size, _elements_count)                    \
    ((size_t)( (_element_size) * (_elements_count) ))

// 5.2    Index clamping and bounds tests
//------------------------------------------------------------------------------

// 5.2.1  D_CLAMP_INDEX
//   macro: clamps an index to the valid range for a given array size.
// returns 0 for negative indices and the last index for oversized indices.
#define D_CLAMP_INDEX(index, arr_size)                                        \
    ( (arr_size) == 0                                                         \
      ? 0                                                                     \
      : ( (index) < 0                                                         \
          ? 0                                                                 \
          : ( (index) >= (ssize_t)(arr_size) )                                \
            ? ( (arr_size) - 1 )                                              \
            : (index) ) )                                                     
                                                                              
// 5.2.2  D_INDEX_IN_BOUNDS                                                   
//   macro: alias for D_IS_VALID_INDEX_N for compatibility.                   
#define D_INDEX_IN_BOUNDS(_index, _arr_size)                                  \
    D_IS_VALID_INDEX_N((_index), (_arr_size))


// 5.2.3  D_IS_VALID_INDEX
//   macro: validates that an `_index` is within bounds for an array of given
// `_count`.
#define D_IS_VALID_INDEX(_index, _count)                                      \
    ( ((_count) > 0) &&                                                       \
      ( ((_index) >= 0 && (_index) < (ssize_t)(_count)) ||                    \
        ((_index) < 0 && (-(_index)) <= (ssize_t)(_count)) ) )                
                                                                              
// 5.2.4  D_IS_VALID_INDEX_N                                                  
//   macro: validates `_index` against the symmetric negative-index range.    
#define D_IS_VALID_INDEX_N(_index, _count)                                    \
    ( (_index) >= -(ssize_t)(_count) && (_index) < (ssize_t)(_count) )


// 5.3    Negative-index access macros
//------------------------------------------------------------------------------

// 5.3.1  D_SAFE_ARR_IDX
//   macro: safe array indexing that returns an element value, not a pointer.
//   note: only to be used on stack-allocated arrays whose size is known at
// compile time.
#define D_SAFE_ARR_IDX(_arr, _index)                                          \
    ( D_IS_VALID_INDEX_N((_index), sizeof(_arr)/sizeof((_arr)[0]) )           \
        ? D_ARR_IDX((_arr), (_index))                                         \
        : (_arr)[0] )

// 5.3.2  D_NEG_IDX
//   macro: given a negative index and the size of the vector (in number of
// elements), returns the non-negative valid index equivalent.
//   Note: this does not check if INDEX corresponds to a valid index within the
// span of the vector; that must be done by the caller to avoid an out-of-bounds
// index.
#define D_NEG_IDX(_index, _count)                                             \
    ( (_index) < 0 ? (_count) + (_index) :                                    \
                    (_index) )

// 5.3.3  D_ARR_IDX
//   macro: given a negative index and array, returns the array element at the
// equivalent positive index.
// note: only to be used on stack-allocated arrays whose size is known at
// compile time.
#define D_ARR_IDX(_array, _index)                                             \
    ( (_array)[(_index) < 0                                                   \
        ? ( (sizeof(_array)/sizeof((_array)[0])) + (_index) )                 \
        : (_index)] )


#endif  // DJINTERP_C_