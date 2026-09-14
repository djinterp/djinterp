/******************************************************************************
* djinterp [core]                                                   env_lang.h
*
* djinterp language-standard detection:
*   Compile-time detection of the C and C++ language standard, exposing the
* unified D_ENV_LANG_* interface (version constants, *_STANDARD / *_NAME,
* USING_C / USING_CPP, and the IS_*_OR_HIGHER comparison helpers), the
* D_DELETE helper, and core integral-type availability (long long).
*
*   Requires:  cfg_env.h (for the D_CFG_ENV_* switches). This header is an
*              internal component of env.h and is #included by it; do NOT
*              #include it directly.
*
*
* path:      /inc/djinterp/env/env_lang.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.03.27
******************************************************************************/

#ifndef DJINTERP_ENV_LANG_
#define DJINTERP_ENV_LANG_ 1


// language standard version constants
#define D_ENV_LANG_C_STANDARD_C95        199409L
#define D_ENV_LANG_C_STANDARD_C99        199901L
#define D_ENV_LANG_C_STANDARD_C11        201112L
#define D_ENV_LANG_C_STANDARD_C17        201710L
#define D_ENV_LANG_C_STANDARD_C23        202311L

#define D_ENV_LANG_CPP_STANDARD_CPP98    199711L
#define D_ENV_LANG_CPP_STANDARD_CPP11    201103L
#define D_ENV_LANG_CPP_STANDARD_CPP14    201402L
#define D_ENV_LANG_CPP_STANDARD_CPP17    201703L
#define D_ENV_LANG_CPP_STANDARD_CPP20    202002L
#define D_ENV_LANG_CPP_STANDARD_CPP23    202302L

// language detection logic
#if D_CFG_ENV_LANG_ENABLED
    #ifdef __cplusplus
        #define D_ENV_LANG_DETECTED_CPP

        // C++ standard detection
        #if __cplusplus >= D_ENV_LANG_CPP_STANDARD_CPP23
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP23
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++23"
        #elif __cplusplus >= D_ENV_LANG_CPP_STANDARD_CPP20
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP20
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++20"
        #elif __cplusplus >= D_ENV_LANG_CPP_STANDARD_CPP17
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP17
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++17"
        #elif __cplusplus >= D_ENV_LANG_CPP_STANDARD_CPP14
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP14
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++14"
        #elif __cplusplus >= D_ENV_LANG_CPP_STANDARD_CPP11
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP11
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++11"
        #else
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP98
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++98"
        #endif
    #endif  // __cplusplus

    // C standard detection
    #ifdef __STDC_VERSION__
        #define D_ENV_LANG_DETECTED_C

        #if (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C23)
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C23"
        #elif (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C17)
            #define D_ENV_LANG_C_STANDARD       __STDC_VERSION__
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
        #endif  // #if __STDC_VERSION__
    #else
        #define D_ENV_LANG_C_STANDARD      199000L
        #define D_ENV_LANG_C_STANDARD_NAME "C90"
    #endif  // __STDC_VERSION__

#else
    // use pre-defined detection variables when language detection is disabled
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
    #endif

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

// define convenience macros based on detected language
#ifdef D_ENV_LANG_CPP_STANDARD
    #define D_ENV_LANG_USING_CPP 1

    // D_ENV_LANG_IS_CPP98_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++98 or later.
    #define D_ENV_LANG_IS_CPP98_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP98)

    // D_ENV_LANG_IS_CPP11_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++11 or later.
    #define D_ENV_LANG_IS_CPP11_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP11)

    // D_ENV_LANG_IS_CPP14_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++14 or later.
    #define D_ENV_LANG_IS_CPP14_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP14)

    // D_ENV_LANG_IS_CPP17_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++17 or later.
    #define D_ENV_LANG_IS_CPP17_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP17)

    // D_ENV_LANG_IS_CPP20_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++20 or later.
    #define D_ENV_LANG_IS_CPP20_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP20)

    // D_ENV_LANG_IS_CPP23_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++23 or later.
    #define D_ENV_LANG_IS_CPP23_OR_HIGHER                                     \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP23)
#else
    #define D_ENV_LANG_USING_CPP 0
#endif  // D_ENV_LANG_CPP_STANDARD

#if D_ENV_LANG_C_STANDARD
    #define D_ENV_LANG_USING_C 1
#else
    #define D_ENV_LANG_USING_C 0
#endif  // D_ENV_LANG_C_STANDARD

// D_ENV_LANG_IS_C95_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C95 or later.
#define D_ENV_LANG_IS_C95_OR_HIGHER                                           \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C95)

// D_ENV_LANG_IS_C99_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C99 or later.
#define D_ENV_LANG_IS_C99_OR_HIGHER                                           \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C99)

// D_ENV_LANG_IS_C11_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C11 or later.
#define D_ENV_LANG_IS_C11_OR_HIGHER                                           \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C11)

// D_ENV_LANG_IS_C17_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C17 or later.
#define D_ENV_LANG_IS_C17_OR_HIGHER                                           \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C17)

// D_ENV_LANG_IS_C23_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C23 or later.
#define D_ENV_LANG_IS_C23_OR_HIGHER                                           \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C23)

// D_DELETE
//   macro: resolves to "= delete" on C++11+, where deleted functions are part
// of the language baseline.  On C++03, expands to nothing; the function should
// be left declared in the private section to achieve the same effect.
#ifndef D_DELETE
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_DELETE = delete
    #else
        #define D_DELETE
    #endif
#endif  // D_DELETE

// ---------------------------------------------------------------------------
//   Core integral-type availability that is derived from the language
// standard detected above (freestanding-safe, C and C++ alike).  Lives in the
// sibling header below; it depends only on the D_ENV_LANG_IS_* macros and the
// intrinsic compiler predefines, so it is included at the end of this section.
// env_long_long.h is an internal component of this header; do not #include it
// directly.

// djinterp
#include "./c/env_long_long.h"


#endif  // DJINTERP_ENV_LANG_
