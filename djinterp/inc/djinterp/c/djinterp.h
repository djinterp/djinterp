/******************************************************************************
* djinterp [c]                                                      djinterp.h
*
* djinterp C framework root header
*   Targets a C99 language floor and supplies the common facilities assumed by
* other modules: cross-platform scalar types, the qualifier kit, shared
* function-pointer typedefs, the global keyword vocabulary, and negative-
* indexing types and macros.
*   It declares no subsystem of its own. Everything here is either a type a
* dependent module needs in its own signatures, or a spelling the framework
* must agree on before any of it can compile.
*
*
* path:      /inc/djinterp/c/djinterp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.11.12
*                                                          revised: 2026.09.07
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    FUNDAMENTAL TYPES
      -----------------
      1.    Cross-platform compatibility
            a. bool
            b. D_RESTRICT
            c. ssize_t
      2.    Static assertion
            a. D_STATIC_ASSERT
      3.    Qualifier kit
            a. D_STATIC
            b. D_INLINE
            c. D_STATIC_INLINE
            d. D_EXTERN_C              (+ _BEGIN / _END)
            e. D_NOINLINE
            f. D_NODISCARD
      4.    Function pointers
            a. fn_apply
            b. fn_apply_context
            c. fn_callback
            d. fn_comparator
            e. fn_free
            f. fn_print
            g. fn_to_string
            h. fn_write

II.   GLOBAL KEYWORDS
      ---------------
      1.    D_KEYWORD_FRAMEWORK_NAME
      2.    D_KEYWORD_CLI
      3.    D_KEYWORD_DATABASE
      4.    D_KEYWORD_ERROR
      5.    D_KEYWORD_EXCEPTION
      6.    D_KEYWORD_FUNCTIONAL
      7.    D_KEYWORD_INTERNAL
      8.    D_KEYWORD_MATH
      9.    D_KEYWORD_MESSAGE
      10.   D_KEYWORD_TEST
      11.   D_KEYWORD_TESTING
      12.   D_KEYWORD_USER_INTERFACE
      13.   D_KEYWORD_WARNING

III.  DEFINED CONSTANTS
      -----------------
      1.    D_SUCCESS / D_FAILURE
      2.    D_ENABLED / D_DISABLED
      3.    D_INDENT

IV.   NEGATIVE INDEXING
      -----------------
      1.    d_index
      2.    Conversion and validation functions

V.    INDEXING MACROS & UTILITIES
      ---------------------------
      1.    Array size macros
      2.    Index clamping and bounds tests
      3.    Negative-index access macros
*/

#ifndef DJINTERP_C_
#define DJINTERP_C_ 1

// std
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "../env/env.h"
#include "../env/c/env_attributes.h"
#include "../env/c/env_vendor_attributes.h"
#include "../config/cfg_qualifiers.h"
#include "./dmacro.h"


//==============================================================================
//                             I. FUNDAMENTAL TYPES                            
//==============================================================================


// I.1   Cross-platform compatibility

// bool
//   type: portable boolean type for the framework's supported C/C++ modes.
#if ( defined(__bool_true_false_are_defined) ||                               \
      defined(bool)                          ||                               \
      defined(true)                          ||                               \
      defined(false) )
    // `stdbool.h` has already been included, do nothing

#elif D_ENV_LANG_IS_C23_OR_HIGHER
    // C23 or newer - bool is a built-in keyword
    // nothing to do, C23 has bool, true, false as keywords

#elif D_ENV_LANG_IS_C99_OR_HIGHER
    // C99 or newer - use the standard header
    // std
    #include <stdbool.h>
#elif defined(__cplusplus)
    // C++ has built-in bool
    // nothing to do, C++ already has bool, true, false
#else
    // C99 is the floor (see 0. LANGUAGE FLOOR), so <stdbool.h> is guaranteed
    // and the hand-rolled typedef fallbacks that used to live here are
    // unreachable. Keeping them would be keeping code no build can enter.
    // std
    #include <stdbool.h>
#endif

// D_RESTRICT
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

// b.
// ssize_t
//   type: signed-size_t;
// b.
// ssize_t
//   type: signed-size_t;
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

// I.2   Static assertion

// D_INTERNAL_STATIC_ASSERT_UID
//   macro (internal): unique suffix for fallback static-assert declarations.
#if defined(__COUNTER__)
    #define D_INTERNAL_STATIC_ASSERT_UID __COUNTER__
#else
    #define D_INTERNAL_STATIC_ASSERT_UID __LINE__
#endif

// D_STATIC_ASSERT
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
#endif

// I.3   Qualifier kit

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
#endif

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

// D_STATIC
//   qualifier: internal linkage (`static`); identical spelling in C and C++.
#if !defined(D_STATIC) && (D_INTERNAL_CFG_STATIC == 1)
    #define D_STATIC                        static
#endif

// D_INLINE
//   qualifier: header-safe inline. `inline` (+force outside testing) in C++ and
// on MSVC; `static inline` (+force outside testing) in standard C -- the only
// header-only spelling that never triggers a linker error. Safe on a header-
// defined function standalone in either language.
#if !defined(D_INLINE) && (D_INTERNAL_CFG_INLINE == 1)
    #define D_INLINE                                                          \
        D_INTERNAL_INLINE_STATIC_PREFIX D_INTERNAL_INLINE_QUAL
#endif

// D_STATIC_INLINE
//   qualifier: internal linkage + inline, composed as exactly one `static`
// plus the inline specifier. Because the specifier never contains `static`,
// this is always a single well-formed `static inline` (no double-`static`).
// in standard C it coincides with D_INLINE, which is correct: a C header inline
// is always a static inline.
#if !defined(D_STATIC_INLINE) && (D_INTERNAL_CFG_INLINE == 1)
    #define D_STATIC_INLINE                 static D_INTERNAL_INLINE_QUAL
#endif

// D_EXTERN_C
//   qualifier: C linkage for a SINGLE declaration.  `extern "C"` under C++,
// nothing under C, so one spelling serves both languages:
//     D_EXTERN_C int d_foo(void);
#if !defined(D_EXTERN_C) && (D_INTERNAL_CFG_EXTERN_C == 1)
    #if D_INTERNAL_QUAL_EXTERN_C_LINKAGE
        #define D_EXTERN_C                  extern "C"
    #else
        #define D_EXTERN_C
    #endif
#endif

// D_EXTERN_C_BEGIN / D_EXTERN_C_END
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
#if !defined(D_EXTERN_C_BEGIN) && (D_INTERNAL_CFG_EXTERN_C == 1)
    #if D_INTERNAL_QUAL_EXTERN_C_LINKAGE
        #define D_EXTERN_C_BEGIN            extern "C" {
    #else
        #define D_EXTERN_C_BEGIN
    #endif
#endif

#if !defined(D_EXTERN_C_END) && (D_INTERNAL_CFG_EXTERN_C == 1)
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
#if !defined(__cplusplus) && (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_CONSTEXPR
        #if D_ENV_LANG_IS_C23_OR_HIGHER
            #define D_CONSTEXPR             constexpr   // C23: objects only
        #else
            #define D_CONSTEXPR                         // no constexpr in C
        #endif
    #endif
    #ifndef D_STATIC_CONSTEXPR
        #define D_STATIC_CONSTEXPR          static D_CONSTEXPR
    #endif
    #ifndef D_CONSTEXPR_INLINE
        #define D_CONSTEXPR_INLINE          D_INLINE      // C: == static inline
    #endif
    #ifndef D_STATIC_CONSTEXPR_INLINE
        #define D_STATIC_CONSTEXPR_INLINE   D_STATIC_INLINE
    #endif
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
    #if !defined(D_INLINE_VAR) && (D_INTERNAL_CFG_INLINE == 1)
        // no inline variables in C
        #define D_INLINE_VAR
    #endif
    #if !defined(D_CONSTEXPR_INLINE_VAR) &&                                   \
        (D_INTERNAL_CFG_CONSTEXPR == 1) && (D_INTERNAL_CFG_INLINE == 1)
        #define D_CONSTEXPR_INLINE_VAR      D_CONSTEXPR
    #endif
#endif

// D_NOINLINE
//   qualifier: prevents inlining for debugging and profiling.
#if defined(D_ENV_COMPILER_MSVC)
    #define D_NOINLINE          __declspec(noinline)
#elif ( defined(D_ENV_COMPILER_GCC) ||  \
        defined(D_ENV_COMPILER_CLANG) )
    #define D_NOINLINE          __attribute__((noinline))
#else
    #define D_NOINLINE
#endif

// D_NODISCARD
//   qualifier: indicates that a function return value should not be silently
// discarded. The compiler will emit a warning (or error, depending
// on settings) if the caller ignores the return value.
//
//   resolution order:
//     1. C++17  / C23  - [[nodiscard]] is standard.
//     2. __has_cpp_attribute / __has_c_attribute - catches compilers
//        that support the attribute before the standard mandates it.
//     3. GCC / Clang - __attribute__((warn_unused_result)) in both
//        C and C++ modes, all the way back to GCC 3.4 / Clang 3.0.
//     4. Everything else - empty (no diagnostic, but no breakage).
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
        #if ( defined(D_ENV_COMPILER_GCC) ||  \
              defined(D_ENV_COMPILER_CLANG) )
            #define D_NODISCARD __attribute__((warn_unused_result))
        #else
            #define D_NODISCARD
        #endif
    #endif  // D_NODISCARD (fallback)
#endif  // D_NODISCARD (outer guard)

// I.4   Function pointers
// fn_apply
//   typedef: function pointer type for applying an operation to an element.
typedef void (*fn_apply)(void* _element);

// fn_apply_context
//   typedef: function pointer type for applying an operation to an element
// with additional context.
typedef void (*fn_apply_context)(void* _element, void* _context);

// fn_callback
//   typedef: generic callback function pointer with optional context.
// `_context` may be NULL.
typedef void (*fn_callback)(void* _context);

// fn_comparator
//   typedef: function pointer for ordering two values of identical type.
// returns a value less than, equal to, or greater than 0 when the first value
// compares less than, equal to, or greater than the second, respectively.
typedef int (*fn_comparator)(const void* _a, const void* _b);

// fn_free
//   typedef: function pointer used to free associated memory.
typedef void (*fn_free)(void* _ptr);

// fn_print
//   typedef: function pointer used to print a value to the desired output.
typedef void (*fn_print)(void* _type, ...);

// fn_to_string
//   typedef: function pointer returning a string representation.
typedef const char* (*fn_to_string)(void);

// fn_write
//   typedef: function pointer that writes to a buffer.
typedef size_t (*fn_write)(char* const _buffer, size_t _size);


//==============================================================================
//                               II. GLOBAL TYPES                              
//==============================================================================

// D_KEYWORD_FRAMEWORK_NAME
//   constant: keyword corresponding to the name of this framework.
#define D_KEYWORD_FRAMEWORK_NAME    djinterp

// D_KEYWORD_CLI
//   keyword: resolves to `cli`.
// used to specify that a unit of code pertains to a command-line interface.
#define D_KEYWORD_CLI               cli

// D_KEYWORD_DATABASE
//   keyword: resolves to `database`.
// corresponds to code dealing with databases and database systems.
#define D_KEYWORD_DATABASE          database

// D_KEYWORD_ERROR
//   keyword: resolves to `error`.
// used for an invalid state; from Latin `errare` -- to wander.
#define D_KEYWORD_ERROR             error

// D_KEYWORD_EXCEPTION
//   keyword: resolves to `exception`.
// used to indicate a severe deviation from a valid state.
#define D_KEYWORD_EXCEPTION         exception

// D_KEYWORD_FUNCTIONAL
//   keyword: resolves to `functional`.
// used to specify that a unit of code pertains to functional programming.
#define D_KEYWORD_FUNCTIONAL        functional

// D_KEYWORD_INTERNAL
//   keyword: resolves to `internal`.
// used to specify that a unit of code is part of the `internal` namespace,
// hiding the details of implementation from regular use.
#define D_KEYWORD_INTERNAL          internal

// D_KEYWORD_MATH
//   keyword: resolves to `math`.
// used for variables, macros, namespaces, etc. that pertain to the `math`
// submodule.
#define D_KEYWORD_MATH              math

// D_KEYWORD_MESSAGE
//   keyword: resolves to `message`.
// used for variables, macros, namespaces, etc. that convey (usually string-
// based) human-readable information that is conveyed to the user, often
// (but not limited to) debugging and error-handling.
#define D_KEYWORD_MESSAGE           message

// D_KEYWORD_TEST
//   keyword: resolves to `test`.
// used to specify that a unit of code is part of unit testing.
#define D_KEYWORD_TEST              test

// D_KEYWORD_TESTING
//   keyword: resolves to `testing`; used to signify that a code segment
// pertains to unit testing.
#define D_KEYWORD_TESTING           testing

// D_KEYWORD_USER_INTERFACE
//   keyword: resolves to `ui`.
// used to specify that a unit of code is part of the user interface.
#define D_KEYWORD_USER_INTERFACE    ui

// D_KEYWORD_WARNING
//   keyword: resolves to `warning`.
// used to specify that the program has an anomalous state that is not
// necessarily the end of the world.
#define D_KEYWORD_WARNING           warning


//==============================================================================
//                            III. DEFINED CONSTANTS                           
//==============================================================================

// D_SUCCESS
//   constant: corresponds to a SUCCESSFUL operation; evaluates to `true`.
#define D_SUCCESS  true

// D_FAILURE
//   constant: corresponds to a FAILED operation; evaluates to `false`.
#define D_FAILURE  false

// D_ENABLED
//   constant: corresponds to a capability that IS compiled in; evaluates to
// `true`. Not a success value -- see D_SUCCESS for that.
#define D_ENABLED  true

// D_DISABLED
//   constant: corresponds to a capability that is NOT compiled in; evaluates
// to `false`. Not a failure value -- see D_FAILURE for that.
#define D_DISABLED false

// D_INDENT
//   constant: string corresponding to one (1) level of indentation. Defaults
// to two single spaces.
#ifndef D_INDENT
#   define D_INDENT "  "
#endif  // D_INDENT


//==============================================================================
//                            IV. NEGATIVE INDEXING                            
//==============================================================================

// IV.1  Index type

// d_index
//   type: a type corresponding to a vector index that may be negative (in
// addition to the traditional positive or zero vector indices).
//   A negative `d_index` counts from the last element back toward 0, rather
// than forward as a positive index does. An index of -1 is the last element;
// an index of -n, for a count of n, is element 0.
typedef ssize_t d_index;

// IV.2  Conversion and validation
D_EXTERN_C_BEGIN
size_t d_index_convert_fast(d_index _index,
                            size_t  _count);
bool   d_index_convert_safe(d_index _index,
                            size_t  _count,
                            size_t* _destination);
bool   d_index_is_valid(d_index _index,
                        size_t  _count);
D_EXTERN_C_END


// V.    Indexing macros and utilities

// D_ARRAY_STATIC_SIZE
//   macro: the number of ELEMENTS in a stack-allocated array whose size is
// known at compile time. Equal to the quotient of the array's total size and
// the size of one element -- a count, not a byte total.
// note: decays to nonsense if handed a pointer rather than an array.
#define D_ARRAY_STATIC_SIZE(_array)                                         \
    ((size_t)(sizeof(_array) / sizeof((_array)[0])))

// D_ARRAY_TOTAL_SIZE
//   macro: shorthand for calculating the total memory occupied, in bytes, by
// a vector of elements.
// equal to the product of `_element_size` and `_elements_count`.
#define D_ARRAY_TOTAL_SIZE(_element_size, _elements_count)                  \
    ((size_t)( (_element_size) * (_elements_count) ))

// D_CLAMP_INDEX
//   macro: clamps an index to the valid range for a given array size.
// returns 0 for negative indices and the last index for oversized indices.
#define D_CLAMP_INDEX(index, arr_size)                                      \
    ( (arr_size) == 0                                                       \
      ? 0                                                                   \
      : ( (index) < 0                                                       \
          ? 0                                                               \
          : ( (index) >= (ssize_t)(arr_size) )                              \
            ? ( (arr_size) - 1 )                                            \
            : (index) ) )

// D_INDEX_IN_BOUNDS
//   macro: alias for D_IS_VALID_INDEX_N for compatibility.
#define D_INDEX_IN_BOUNDS(_index, _arr_size)                                \
    D_IS_VALID_INDEX_N((_index), (_arr_size))

// D_SAFE_ARR_IDX
//   macro: safe array indexing that returns an element value, not a pointer.
//   note: only to be used on stack-allocated arrays whose size is known at
// compile time.
#define D_SAFE_ARR_IDX(_arr, _index)                                        \
    ( D_IS_VALID_INDEX_N((_index), sizeof(_arr)/sizeof((_arr)[0]) )         \
        ? D_ARR_IDX((_arr), (_index))                                       \
        : (_arr)[0] )

// D_IS_VALID_INDEX
//   macro: validates that an `_index` is within bounds for an array of given
// `_count`.
#define D_IS_VALID_INDEX(_index, _count)                                    \
    ( ((_count) > 0) &&                                                     \
      ( ((_index) >= 0 && (_index) < (ssize_t)(_count)) ||                  \
        ((_index) < 0 && (-(_index)) <= (ssize_t)(_count)) ) )

// D_IS_VALID_INDEX_N
//   macro: validates `_index` against the symmetric negative-index range.
#define D_IS_VALID_INDEX_N(_index, _count)                                  \
    ( (_index) >= -(ssize_t)(_count) && (_index) < (ssize_t)(_count) )

// D_NEG_IDX
//   macro: given a negative index and the size of the vector (in number of
// elements), returns the non-negative valid index equivalent.
//   Note: this does not check if INDEX corresponds to a valid index within the
// span of the vector; that must be done by the caller to avoid an out-of-bounds
// index.
#define D_NEG_IDX(_index, _count)                                           \
    ( (_index) < 0 ? (_count) + (_index) :                                  \
                    (_index) )

// D_ARR_IDX
//   macro: given a negative index and array, returns the array element at the
// equivalent positive index.
// note: only to be used on stack-allocated arrays whose size is known at
// compile time.
#define D_ARR_IDX(_array, _index)                                           \
    ( (_array)[(_index) < 0                                                 \
        ? ( (sizeof(_array)/sizeof((_array)[0])) + (_index) )               \
        : (_index)] )


#endif  // DJINTERP_C_