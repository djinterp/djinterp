/*******************************************************************************
* djinterp [env]                                                      env_lang.h
*
* djinterp language-standard detection.
*   Compile-time detection of the C and C++ language standard, exposing the
* unified D_ENV_LANG_* interface (version constants, *_STANDARD / *_NAME,
* USING_C / USING_CPP, and the IS_*_OR_HIGHER comparison helpers), the
* D_DELETE helper, and core integral-type availability (long long).
*   Requires cfg_env.h (for the D_CFG_ENV_* switches). This header is an
* internal component of env.h and is #included by it; do not #include it
* directly.
*
* path:      /inc/djinterp/env/env_lang.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.03.27
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LANGUAGE STANDARD CONSTANTS
    ---------------------------
    1.  C standards
         1.  C standard identifiers
              1.  D_ENV_LANG_C_STANDARD_C95
              2.  D_ENV_LANG_C_STANDARD_C99
              3.  D_ENV_LANG_C_STANDARD_C11
              4.  D_ENV_LANG_C_STANDARD_C17
              5.  D_ENV_LANG_C_STANDARD_C23
    2.  C++ standards
         1.  C++ standard identifiers
              1.  D_ENV_LANG_CPP_STANDARD_CPP98
              2.  D_ENV_LANG_CPP_STANDARD_CPP11
              3.  D_ENV_LANG_CPP_STANDARD_CPP14
              4.  D_ENV_LANG_CPP_STANDARD_CPP17
              5.  D_ENV_LANG_CPP_STANDARD_CPP20
              6.  D_ENV_LANG_CPP_STANDARD_CPP23
2.  LANGUAGE DETECTION
    ------------------
    1.  Automatic detection
         1.  D_INTERNAL_ENV_LANG_CPLUSPLUS
         2.  D_ENV_LANG_CPP_STANDARD
         3.  D_ENV_LANG_C_STANDARD
    2.  Predefined detection
         1.  D_ENV_DETECTED_CPP* overrides
         2.  D_ENV_DETECTED_C* overrides
3.  LANGUAGE PREDICATES
    -------------------
    1.  C++
         1.  D_ENV_LANG_USING_CPP
         2.  D_ENV_LANG_IS_CPP98_OR_HIGHER
         3.  D_ENV_LANG_IS_CPP11_OR_HIGHER
         4.  D_ENV_LANG_IS_CPP14_OR_HIGHER
         5.  D_ENV_LANG_IS_CPP17_OR_HIGHER
         6.  D_ENV_LANG_IS_CPP20_OR_HIGHER
         7.  D_ENV_LANG_IS_CPP23_OR_HIGHER
    2.  C
         1.  D_ENV_LANG_USING_C
         2.  D_ENV_LANG_IS_C95_OR_HIGHER
         3.  D_ENV_LANG_IS_C99_OR_HIGHER
         4.  D_ENV_LANG_IS_C11_OR_HIGHER
         5.  D_ENV_LANG_IS_C17_OR_HIGHER
         6.  D_ENV_LANG_IS_C23_OR_HIGHER
4.  LANGUAGE HELPERS
    ----------------
    1.  C++ helpers
         1.  D_DELETE
    2.  Integral-type availability
*/

#ifndef DJINTERP_ENV_ENV_LANG_H
#define DJINTERP_ENV_ENV_LANG_H 1


//==============================================================================
// 1.  LANGUAGE STANDARD CONSTANTS
//==============================================================================
// Version numbers as __STDC_VERSION__ and __cplusplus report them, so a
// detected standard compares directly against these.


// 1.1    C standards
//------------------------------------------------------------------------------
// 1.1.1
// C standard identifiers

// 1.1.1.1
// D_ENV_LANG_C_STANDARD_C95
//   constant: __STDC_VERSION__ value for C95 (ISO C, Amendment 1).
#define D_ENV_LANG_C_STANDARD_C95       199409L

// 1.1.1.2
// D_ENV_LANG_C_STANDARD_C99
//   constant: __STDC_VERSION__ value for C99.
#define D_ENV_LANG_C_STANDARD_C99       199901L

// 1.1.1.3
// D_ENV_LANG_C_STANDARD_C11
//   constant: __STDC_VERSION__ value for C11.
#define D_ENV_LANG_C_STANDARD_C11       201112L

// 1.1.1.4
// D_ENV_LANG_C_STANDARD_C17
//   constant: __STDC_VERSION__ value for C17.
#define D_ENV_LANG_C_STANDARD_C17       201710L

// 1.1.1.5
// D_ENV_LANG_C_STANDARD_C23
//   constant: __STDC_VERSION__ value for C23.
#define D_ENV_LANG_C_STANDARD_C23       202311L

// 1.2    C++ standards
//------------------------------------------------------------------------------
// 1.2.1
// C++ standard identifiers

// 1.2.1.1
// D_ENV_LANG_CPP_STANDARD_CPP98
//   constant: __cplusplus value for C++98.
#define D_ENV_LANG_CPP_STANDARD_CPP98   199711L

// 1.2.1.2
// D_ENV_LANG_CPP_STANDARD_CPP11
//   constant: __cplusplus value for C++11.
#define D_ENV_LANG_CPP_STANDARD_CPP11   201103L

// 1.2.1.3
// D_ENV_LANG_CPP_STANDARD_CPP14
//   constant: __cplusplus value for C++14.
#define D_ENV_LANG_CPP_STANDARD_CPP14   201402L

// 1.2.1.4
// D_ENV_LANG_CPP_STANDARD_CPP17
//   constant: __cplusplus value for C++17.
#define D_ENV_LANG_CPP_STANDARD_CPP17   201703L

// 1.2.1.5
// D_ENV_LANG_CPP_STANDARD_CPP20
//   constant: __cplusplus value for C++20.
#define D_ENV_LANG_CPP_STANDARD_CPP20   202002L

// 1.2.1.6
// D_ENV_LANG_CPP_STANDARD_CPP23
//   constant: __cplusplus value for C++23.
#define D_ENV_LANG_CPP_STANDARD_CPP23   202302L


//==============================================================================
// 2.  LANGUAGE DETECTION
//==============================================================================
// Settles D_ENV_LANG_C_STANDARD and, under a C++ compiler,
// D_ENV_LANG_CPP_STANDARD, each with a *_NAME string, from the compiler or,
// when detection is disabled, from the D_ENV_DETECTED_* overrides.


#if D_CFG_ENV_LANG_ENABLED

// 2.1    Automatic detection
//------------------------------------------------------------------------------
    #ifdef __cplusplus
        // D_ENV_LANG_DETECTED_CPP
        //   constant: defined, empty, when a C++ compiler was detected.
        #define D_ENV_LANG_DETECTED_CPP

        // 2.1.1
        // D_INTERNAL_ENV_LANG_CPLUSPLUS
        //   macro (internal): the C++ version actually in use. MSVC leaves
        // __cplusplus at 199711L unless /Zc:__cplusplus is given, and reports
        // the real version in _MSVC_LANG (clang-cl sets both), so the larger
        // of the two is taken. Every other compiler has no _MSVC_LANG.
        #if ( defined(_MSVC_LANG) &&                                          \
              (_MSVC_LANG > __cplusplus) )
            #define D_INTERNAL_ENV_LANG_CPLUSPLUS _MSVC_LANG
        #else
            #define D_INTERNAL_ENV_LANG_CPLUSPLUS __cplusplus
        #endif

        // 2.1.2
        // D_ENV_LANG_CPP_STANDARD
        //   constant: the detected C++ standard, one of the 1.2 identifiers,
        // with D_ENV_LANG_CPP_STANDARD_NAME its name.
        #if D_INTERNAL_ENV_LANG_CPLUSPLUS >= D_ENV_LANG_CPP_STANDARD_CPP23
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP23
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++23"
        #elif D_INTERNAL_ENV_LANG_CPLUSPLUS >= D_ENV_LANG_CPP_STANDARD_CPP20
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP20
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++20"
        #elif D_INTERNAL_ENV_LANG_CPLUSPLUS >= D_ENV_LANG_CPP_STANDARD_CPP17
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP17
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++17"
        #elif D_INTERNAL_ENV_LANG_CPLUSPLUS >= D_ENV_LANG_CPP_STANDARD_CPP14
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP14
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++14"
        #elif D_INTERNAL_ENV_LANG_CPLUSPLUS >= D_ENV_LANG_CPP_STANDARD_CPP11
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP11
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++11"
        #else
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP98
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++98"
        #endif
    #endif  // __cplusplus

    // 2.1.3
    // D_ENV_LANG_C_STANDARD
    //   constant: the detected C standard, taken from __STDC_VERSION__, with
    // D_ENV_LANG_C_STANDARD_NAME its name; 199000L ("C90") when the compiler
    // reports no version, which includes every C++ compiler.
    #ifdef __STDC_VERSION__
        // D_ENV_LANG_DETECTED_C
        //   constant: defined, empty, when __STDC_VERSION__ is available.
        #define D_ENV_LANG_DETECTED_C

        #if (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C23)
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C23"
        #elif (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C17)
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C17"
        #elif (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C11)
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C11"
        #elif (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C99)
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C99"
        #elif (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C95)
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C95"
        #else
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C90"
        #endif
    #else
        #define D_ENV_LANG_C_STANDARD      199000L
        #define D_ENV_LANG_C_STANDARD_NAME "C90"
    #endif  // __STDC_VERSION__

#else

// 2.2    Predefined detection
//------------------------------------------------------------------------------
    // 2.2.1
    // D_ENV_DETECTED_CPP* overrides
    //   the newest D_ENV_DETECTED_CPPnn defined selects the C++ standard.
    #ifdef D_ENV_DETECTED_CPP23
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP23
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++23"
    #elif defined(D_ENV_DETECTED_CPP20)
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP20
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++20"
    #elif defined(D_ENV_DETECTED_CPP17)
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP17
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++17"
    #elif defined(D_ENV_DETECTED_CPP14)
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP14
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++14"
    #elif defined(D_ENV_DETECTED_CPP11)
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP11
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++11"
    #elif defined(D_ENV_DETECTED_CPP98)
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP98
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++98"
    #endif  // D_ENV_DETECTED_CPP23

    // 2.2.2
    // D_ENV_DETECTED_C* overrides
    //   the newest D_ENV_DETECTED_Cnn defined selects the C standard.
    #if defined(D_ENV_DETECTED_C23)
        #define D_ENV_LANG_C_STANDARD        D_ENV_LANG_C_STANDARD_C23
        #define D_ENV_LANG_C_STANDARD_NAME   "C23"
    #elif defined(D_ENV_DETECTED_C17)
        #define D_ENV_LANG_C_STANDARD        D_ENV_LANG_C_STANDARD_C17
        #define D_ENV_LANG_C_STANDARD_NAME   "C17"
    #elif defined(D_ENV_DETECTED_C11)
        #define D_ENV_LANG_C_STANDARD        D_ENV_LANG_C_STANDARD_C11
        #define D_ENV_LANG_C_STANDARD_NAME   "C11"
    #elif defined(D_ENV_DETECTED_C99)
        #define D_ENV_LANG_C_STANDARD        D_ENV_LANG_C_STANDARD_C99
        #define D_ENV_LANG_C_STANDARD_NAME   "C99"
    #elif defined(D_ENV_DETECTED_C95)
        #define D_ENV_LANG_C_STANDARD        D_ENV_LANG_C_STANDARD_C95
        #define D_ENV_LANG_C_STANDARD_NAME   "C95"
    #else
        // fallback when no manual C standard is specified
        #define D_ENV_LANG_C_STANDARD        199000L
        #define D_ENV_LANG_C_STANDARD_NAME   "C90"
    #endif

#endif  // D_CFG_ENV_LANG_ENABLED


//==============================================================================
// 3.  LANGUAGE PREDICATES
//==============================================================================
// Usable in #if. The C++ comparisons exist only under a C++ compiler; the C
// comparisons always exist, and under C++ compare against 199000L.


// 3.1    C++
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_LANG_USING_CPP
//   constant: 1 when a C++ standard was detected, 0 otherwise; always defined,
// so test it with #if, not #ifdef.
#ifdef D_ENV_LANG_CPP_STANDARD
    #define D_ENV_LANG_USING_CPP 1

    // 3.1.2
    // D_ENV_LANG_IS_CPP98_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++98 or later.
    #define D_ENV_LANG_IS_CPP98_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP98)

    // 3.1.3
    // D_ENV_LANG_IS_CPP11_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++11 or later.
    #define D_ENV_LANG_IS_CPP11_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP11)

    // 3.1.4
    // D_ENV_LANG_IS_CPP14_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++14 or later.
    #define D_ENV_LANG_IS_CPP14_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP14)

    // 3.1.5
    // D_ENV_LANG_IS_CPP17_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++17 or later.
    #define D_ENV_LANG_IS_CPP17_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP17)

    // 3.1.6
    // D_ENV_LANG_IS_CPP20_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++20 or later.
    #define D_ENV_LANG_IS_CPP20_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP20)

    // 3.1.7
    // D_ENV_LANG_IS_CPP23_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++23 or later.
    #define D_ENV_LANG_IS_CPP23_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP23)
#else
    #define D_ENV_LANG_USING_CPP 0
#endif  // D_ENV_LANG_CPP_STANDARD

// 3.2    C
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_LANG_USING_C
//   constant: 1 when D_ENV_LANG_C_STANDARD is nonzero. It is nonzero under C++
// as well (199000L), so this does not distinguish C from C++; use
// D_ENV_LANG_USING_CPP for that.
#if D_ENV_LANG_C_STANDARD
    #define D_ENV_LANG_USING_C 1
#else
    #define D_ENV_LANG_USING_C 0
#endif  // D_ENV_LANG_C_STANDARD

// 3.2.2
// D_ENV_LANG_IS_C95_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C95 or later.
#define D_ENV_LANG_IS_C95_OR_HIGHER                                           \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C95)

// 3.2.3
// D_ENV_LANG_IS_C99_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C99 or later.
#define D_ENV_LANG_IS_C99_OR_HIGHER                                           \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C99)

// 3.2.4
// D_ENV_LANG_IS_C11_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C11 or later.
#define D_ENV_LANG_IS_C11_OR_HIGHER                                           \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C11)

// 3.2.5
// D_ENV_LANG_IS_C17_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C17 or later.
#define D_ENV_LANG_IS_C17_OR_HIGHER                                           \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C17)

// 3.2.6
// D_ENV_LANG_IS_C23_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C23 or later.
#define D_ENV_LANG_IS_C23_OR_HIGHER                                           \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C23)


//==============================================================================
// 4.  LANGUAGE HELPERS
//==============================================================================


// 4.1    C++ helpers
//------------------------------------------------------------------------------
// 4.1.1
// D_DELETE
//   macro: resolves to "= delete" on C++11+, where deleted functions are part
// of the language baseline. On C++03, expands to nothing; the function should
// be left declared in the private section to achieve the same effect.
#ifndef D_DELETE
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_DELETE = delete
    #else
        #define D_DELETE
    #endif
#endif  // D_DELETE

// 4.2    Integral-type availability
//------------------------------------------------------------------------------
//   core integral-type availability derived from the language standard
// detected above (freestanding-safe, C and C++ alike). It lives in the sibling
// header below, which depends only on the D_ENV_LANG_IS_* macros and the
// intrinsic compiler predefines, so it is included here, at the end.
// env_long_long.h is an internal component of this header; do not #include it
// directly.

// djinterp
#include "./c/env_long_long.h"  // `long long` availability


#endif  // DJINTERP_ENV_ENV_LANG_H
