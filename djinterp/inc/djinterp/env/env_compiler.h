/*******************************************************************************
* djinterp [env]                                                  env_compiler.h
*
* djinterp compiler detection and preprocessor limits.
*   Compiler identification and version (the D_ENV_COMPILER_* interface),
* __VA_OPT__ availability, and the preprocessor translation-limit interface
* (D_ENV_PP_*). The limits block consults the D_ENV_PLATFORM_* flags from the
* OS section, so this header must be included after env_os.h, which the
* umbrella env.h arranges. Compiler identity itself is independent of
* architecture and OS, and must precede env_c_lib.h.
*   Requires cfg_env.h, env_lang.h (for D_ENV_LANG_*), and env_os.h (for
* D_ENV_PLATFORM_WINDOWS, used by the limits block). This header is an
* internal component of env.h and is #included by it; do not #include it
* directly.
*
* path:      /inc/djinterp/env/env_compiler.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.03.27
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  COMPILER DETECTION
    ------------------
    1.  Automatic detection
         1.  Compiler identity
              1.  Clang
              2.  GCC
              3.  Microsoft Visual C++
              4.  Intel C++
              5.  Borland / Turbo C++
              6.  Unknown compiler
    2.  Predefined detection
         1.  D_ENV_DETECTED_COMPILER_* overrides
         2.  Default version information
    3.  Toolchain family
         1.  D_ENV_COMPILER_MSVC_FAMILY
    4.  Version checks
         1.  D_ENV_COMPILER_VERSION_AT_LEAST
         2.  D_ENV_COMPILER_VERSION_AT_MOST
2.  PREPROCESSOR FEATURES
    ---------------------
    1.  __VA_OPT__
         1.  D_ENV_PP_HAS_VA_OPT
         2.  D_ENV_PP_HAS_VA_OPT_ENABLED
3.  PREPROCESSOR LIMITS
    -------------------
    1.  Standard translation limits
         1.  C89 minimum limits
              1.  D_ENV_PP_LIMIT_C89_MACRO_ARGS
              2.  D_ENV_PP_LIMIT_C89_NESTING_DEPTH
              3.  D_ENV_PP_LIMIT_C89_MACRO_IDS
              4.  D_ENV_PP_LIMIT_C89_PARAMS
              5.  D_ENV_PP_LIMIT_C89_STRING_LENGTH
         2.  C99 and later minimum limits
              1.  D_ENV_PP_LIMIT_C99_MACRO_ARGS
              2.  D_ENV_PP_LIMIT_C99_NESTING_DEPTH
              3.  D_ENV_PP_LIMIT_C99_MACRO_IDS
              4.  D_ENV_PP_LIMIT_C99_PARAMS
              5.  D_ENV_PP_LIMIT_C99_STRING_LENGTH
         3.  C++ minimum limits
              1.  D_ENV_PP_LIMIT_CPP_MACRO_ARGS
              2.  D_ENV_PP_LIMIT_CPP_NESTING_DEPTH
              3.  D_ENV_PP_LIMIT_CPP_MACRO_IDS
              4.  D_ENV_PP_LIMIT_CPP_PARAMS
              5.  D_ENV_PP_LIMIT_CPP_STRING_LENGTH
    2.  Standard-based minimums
         1.  D_ENV_PP_MIN_*
    3.  Compiler-specific practical limits
         1.  D_ENV_PP_MAX_* and D_ENV_PP_LIMIT_SOURCE
    4.  Limit utilities
         1.  D_ENV_PP_ARGS_WITHIN_LIMIT
         2.  D_ENV_PP_ARGS_WITHIN_STANDARD
         3.  D_ENV_PP_IS_UNLIMITED
         4.  D_ENV_PP_EFFECTIVE_LIMIT
*/

#ifndef DJINTERP_ENV_ENV_COMPILER_H
#define DJINTERP_ENV_ENV_COMPILER_H 1


//==============================================================================
// 1.  COMPILER DETECTION
//==============================================================================
// Identifies the compiler and its version, from its predefined macros or,
// when detection is disabled, from the D_ENV_DETECTED_COMPILER_* overrides.


#if D_CFG_ENV_COMPILER_ENABLED

// 1.1    Automatic detection
//------------------------------------------------------------------------------
// 1.1.1
// Compiler identity
//   constant: exactly one D_ENV_COMPILER_<compiler> flag, defined to 1, plus
// D_ENV_COMPILER_NAME, _FULL_NAME, _MAJOR, _MINOR, _PATCHLEVEL, and
// _VERSION_STRING. Clang is tested first because it also defines __GNUC__, and
// clang-cl is therefore classified as Clang (see D_ENV_COMPILER_MSVC_FAMILY).

    // 1.1.1.1
    // Clang
    #if defined(__clang__)
        #define D_ENV_COMPILER_CLANG 1

        #ifdef __apple_build_version__
            #define D_ENV_COMPILER_APPLE_CLANG  1
            #define D_ENV_COMPILER_NAME         "Apple Clang"
            #define D_ENV_COMPILER_FULL_NAME    "Apple Clang/LLVM"
        #else
            #define D_ENV_COMPILER_NAME         "Clang"
            #define D_ENV_COMPILER_FULL_NAME    "Clang/LLVM"
        #endif  // __apple_build_version__

        #define D_ENV_COMPILER_MAJOR            __clang_major__
        #define D_ENV_COMPILER_MINOR            __clang_minor__
        #define D_ENV_COMPILER_PATCHLEVEL       __clang_patchlevel__
        #define D_ENV_COMPILER_VERSION_STRING   __clang_version__

    // 1.1.1.2
    // GCC
    #elif defined(__GNUC__)
        #define D_ENV_COMPILER_GCC 1
        #define D_ENV_COMPILER_NAME             "GCC"
        #define D_ENV_COMPILER_FULL_NAME        "GNU Compiler Collection"
        #define D_ENV_COMPILER_MAJOR            __GNUC__
        #define D_ENV_COMPILER_MINOR            __GNUC_MINOR__

        #ifdef __GNUC_PATCHLEVEL__
            #define D_ENV_COMPILER_PATCHLEVEL   __GNUC_PATCHLEVEL__
        #else
            #define D_ENV_COMPILER_PATCHLEVEL   0
        #endif  // __GNUC_PATCHLEVEL__

        #ifdef __VERSION__
            #define D_ENV_COMPILER_VERSION_STRING __VERSION__
        #else
            #define D_ENV_COMPILER_VERSION_STRING "GCC (version unknown)"
        #endif  // __VERSION__

    // 1.1.1.3
    // Microsoft Visual C++
    #elif defined(_MSC_VER)
        #define D_ENV_COMPILER_MSVC             1
        #define D_ENV_COMPILER_NAME             "MSVC"
        #define D_ENV_COMPILER_FULL_NAME        "Microsoft Visual C++"

        // version mapping for MSVC
        #if _MSC_VER >= 1930
            #define D_ENV_COMPILER_MAJOR 17
        #elif _MSC_VER >= 1920
            #define D_ENV_COMPILER_MAJOR 16
        #elif _MSC_VER >= 1910
            #define D_ENV_COMPILER_MAJOR 15
        #elif _MSC_VER >= 1900
            #define D_ENV_COMPILER_MAJOR 14
        #elif _MSC_VER >= 1800
            #define D_ENV_COMPILER_MAJOR 12
        #elif _MSC_VER >= 1700
            #define D_ENV_COMPILER_MAJOR 11
        #elif _MSC_VER >= 1600
            #define D_ENV_COMPILER_MAJOR 10
        #else
            #define D_ENV_COMPILER_MAJOR 9
        #endif

        #define D_ENV_COMPILER_MINOR            ( (_MSC_VER % 100) / 10)
        #define D_ENV_COMPILER_PATCHLEVEL         (_MSC_VER % 10)
        #define D_ENV_COMPILER_VERSION_STRING   "MSVC"

    // 1.1.1.4
    // Intel C++
    #elif ( defined(__INTEL_COMPILER) ||  \
            defined(__ICL)            ||  \
            defined(__ICC) )
        #define D_ENV_COMPILER_INTEL 1
        #define D_ENV_COMPILER_NAME            "Intel C++"
        #define D_ENV_COMPILER_FULL_NAME       "Intel C++ Compiler"

        #ifdef __INTEL_COMPILER
            #define D_ENV_COMPILER_MAJOR        (__INTEL_COMPILER / 100)
            #define D_ENV_COMPILER_MINOR        ((__INTEL_COMPILER % 100) / 10)
            #define D_ENV_COMPILER_PATCHLEVEL   (__INTEL_COMPILER % 10)
        #else
            #define D_ENV_COMPILER_MAJOR        0
            #define D_ENV_COMPILER_MINOR        0
            #define D_ENV_COMPILER_PATCHLEVEL   0
        #endif  // __INTEL_COMPILER
        #define D_ENV_COMPILER_VERSION_STRING   "Intel C++"

    // 1.1.1.5
    // Borland / Turbo C++
    #elif ( defined(__BORLANDC__) ||    \
            defined(__TURBOC__) )
        #define D_ENV_COMPILER_BORLAND          1
        #define D_ENV_COMPILER_NAME             "Borland C++"
        #define D_ENV_COMPILER_FULL_NAME        "Borland C++ Compiler"

        #ifdef __BORLANDC__
            #define D_ENV_COMPILER_MAJOR        (__BORLANDC__ >> 8)
            #define D_ENV_COMPILER_MINOR        ((__BORLANDC__ & 0xFF) >> 4)
            #define D_ENV_COMPILER_PATCHLEVEL   (__BORLANDC__ & 0x0F)
        #else
            #define D_ENV_COMPILER_MAJOR        0
            #define D_ENV_COMPILER_MINOR        0
            #define D_ENV_COMPILER_PATCHLEVEL   0
        #endif  // __BORLANDC__

        #define D_ENV_COMPILER_VERSION_STRING   "Borland C++"

    // 1.1.1.6
    // Unknown compiler
    #else
        #define D_ENV_COMPILER_UNKNOWN          1
        #define D_ENV_COMPILER_NAME             "unknown"
        #define D_ENV_COMPILER_FULL_NAME        "unknown Compiler"
        #define D_ENV_COMPILER_MAJOR            0
        #define D_ENV_COMPILER_MINOR            0
        #define D_ENV_COMPILER_PATCHLEVEL       0
        #define D_ENV_COMPILER_VERSION_STRING   "unknown"
    #endif
#else

// 1.2    Predefined detection
//------------------------------------------------------------------------------
    // 1.2.1
    // D_ENV_DETECTED_COMPILER_* overrides
    //   the first D_ENV_DETECTED_COMPILER_* defined selects the compiler.
    #ifdef D_ENV_DETECTED_COMPILER_APPLE_CLANG
        #define D_ENV_COMPILER_APPLE_CLANG 1
        #define D_ENV_COMPILER_CLANG     1
        #define D_ENV_COMPILER_NAME      "Apple Clang"
        #define D_ENV_COMPILER_FULL_NAME "Apple Clang/LLVM"
    #elif defined(D_ENV_DETECTED_COMPILER_CLANG)
        #define D_ENV_COMPILER_CLANG     1
        #define D_ENV_COMPILER_NAME      "Clang"
        #define D_ENV_COMPILER_FULL_NAME "Clang/LLVM"
    #elif defined(D_ENV_DETECTED_COMPILER_GCC)
        #define D_ENV_COMPILER_GCC       1
        #define D_ENV_COMPILER_NAME      "GCC"
        #define D_ENV_COMPILER_FULL_NAME "GNU Compiler Collection"
    #elif defined(D_ENV_DETECTED_COMPILER_MSVC)
        #define D_ENV_COMPILER_MSVC      1
        #define D_ENV_COMPILER_NAME      "MSVC"
        #define D_ENV_COMPILER_FULL_NAME "Microsoft Visual C++"
    #elif defined(D_ENV_DETECTED_COMPILER_INTEL)
        #define D_ENV_COMPILER_INTEL     1
        #define D_ENV_COMPILER_NAME      "Intel C++"
        #define D_ENV_COMPILER_FULL_NAME "Intel C++ Compiler"
    #elif defined(D_ENV_DETECTED_COMPILER_BORLAND)
        #define D_ENV_COMPILER_BORLAND    1
        #define D_ENV_COMPILER_NAME      "Borland C++"
        #define D_ENV_COMPILER_FULL_NAME "Borland C++ Compiler"
    #elif defined(D_ENV_DETECTED_COMPILER_UNKNOWN)
        #define D_ENV_COMPILER_UNKNOWN   1
        #define D_ENV_COMPILER_NAME      "Unknown"
        #define D_ENV_COMPILER_FULL_NAME "Unknown Compiler"
    #endif  // D_ENV_DETECTED_COMPILER_APPLE_CLANG

    // 1.2.2
    // Default version information
    //   a simulated compiler reports version 0.0.0, "simulated".
    #ifndef D_ENV_COMPILER_MAJOR
        #define D_ENV_COMPILER_MAJOR          0
        #define D_ENV_COMPILER_MINOR          0
        #define D_ENV_COMPILER_PATCHLEVEL     0
        #define D_ENV_COMPILER_VERSION_STRING "simulated"
    #endif  // D_ENV_COMPILER_MAJOR
#endif  // !D_CFG_ENV_COMPILER_ENABLED

// 1.3    Toolchain family
//------------------------------------------------------------------------------
// 1.3.1
// D_ENV_COMPILER_MSVC_FAMILY
//   macro: 1 when the compiler uses the Microsoft toolchain's headers and C
// runtime -- cl itself, and clang-cl and Intel on Windows, which define
// _MSC_VER as well but are classified above under their own names -- and 0
// otherwise. Questions about the runtime rather than the compiler, such as
// whether <sys/types.h> declares ssize_t, key on this macro rather than on
// D_ENV_COMPILER_MSVC.
#if ( defined(_MSC_VER) ||                                                    \
      defined(D_ENV_COMPILER_MSVC) )
    #define D_ENV_COMPILER_MSVC_FAMILY 1
#else
    #define D_ENV_COMPILER_MSVC_FAMILY 0
#endif

// 1.4    Version checks
//------------------------------------------------------------------------------
// 1.4.1
// D_ENV_COMPILER_VERSION_AT_LEAST
//   macro: utility macro for version checking, ensuring that the compiler
// version is greater than, or equal to, the version specified.
#define D_ENV_COMPILER_VERSION_AT_LEAST(major, minor, patch) \
    ( (D_ENV_COMPILER_MAJOR > (major))     ||  \
     ( (D_ENV_COMPILER_MAJOR == (major)) &&    \
       (D_ENV_COMPILER_MINOR > (minor)) )  ||  \
     ( (D_ENV_COMPILER_MAJOR == (major)) &&    \
       (D_ENV_COMPILER_MINOR == (minor)) &&    \
       (D_ENV_COMPILER_PATCHLEVEL >= (patch)) ) )

// 1.4.2
// D_ENV_COMPILER_VERSION_AT_MOST
//   macro: utility macro for version checking, ensuring that the compiler
// version is less than, or equal to, the version specified.
#define D_ENV_COMPILER_VERSION_AT_MOST(major, minor, patch) \
    ( (D_ENV_COMPILER_MAJOR < (major))     ||  \
     ( (D_ENV_COMPILER_MAJOR == (major)) &&    \
       (D_ENV_COMPILER_MINOR < (minor)) )  ||  \
     ( (D_ENV_COMPILER_MAJOR == (major)) &&    \
       (D_ENV_COMPILER_MINOR == (minor)) &&    \
       (D_ENV_COMPILER_PATCHLEVEL <= (patch)) ) )


//==============================================================================
// 2.  PREPROCESSOR FEATURES
//==============================================================================


// 2.1    __VA_OPT__
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_PP_HAS_VA_OPT
//   constant: 1 when __VA_OPT__ is available, 0 otherwise, decided in order:
//     - __cpp_va_opt, where a C++ compiler defines it (GCC 13 and Clang 18
//       do not, even at C++20), answers directly.
//     - a strict ISO mode (__STRICT_ANSI__) whose standard predates
//       __VA_OPT__ -- C++ before C++20, C before C2x -- reports 0: it is not
//       part of the language there, and GCC's -pedantic rejects it even in
//       a macro that is never expanded.
//     - anything else probes: with __VA_OPT__ support, __VA_OPT__(,) in a
//       non-empty argument list expands to a comma, which shifts which
//       argument the probe selects. The EXPAND step re-scans the forwarded
//       arguments, which MSVC's traditional preprocessor otherwise passes on
//       as one.
#ifndef D_ENV_PP_HAS_VA_OPT
    #if ( defined(__cpp_va_opt) &&                                             \
          (__cpp_va_opt >= 201803L) )
        #define D_ENV_PP_HAS_VA_OPT 1
    #elif ( defined(__STRICT_ANSI__)                                  &&       \
            (D_ENV_LANG_USING_CPP)                                    &&       \
            (D_ENV_LANG_CPP_STANDARD < D_ENV_LANG_CPP_STANDARD_CPP20) )
        #define D_ENV_PP_HAS_VA_OPT 0
    #elif ( defined(__STRICT_ANSI__)                             &&            \
            (!D_ENV_LANG_USING_CPP)                              &&            \
            (D_ENV_LANG_C_STANDARD <= D_ENV_LANG_C_STANDARD_C17) )
        #define D_ENV_PP_HAS_VA_OPT 0
    #else
        //   a dummy argument (~) keeps __VA_ARGS__ non-empty:
        //     __VA_OPT__ works:    __VA_OPT__(,) -> ",", third argument 1
        //     __VA_OPT__ literal:  no comma inserted, third argument 0

        // D_INTERNAL_ENV_VA_OPT_EXPAND / D_INTERNAL_ENV_VA_OPT_THIRD_ARG_ /
        // D_INTERNAL_ENV_VA_OPT_THIRD_ARG / D_INTERNAL_ENV_VA_OPT_PROBE
        //   macro (internal): the probe's argument-selection machinery.
        #define D_INTERNAL_ENV_VA_OPT_EXPAND(x) x
        #define D_INTERNAL_ENV_VA_OPT_THIRD_ARG_(a, b, c, ...) c
        #define D_INTERNAL_ENV_VA_OPT_THIRD_ARG(...)                           \
            D_INTERNAL_ENV_VA_OPT_EXPAND(                                      \
                D_INTERNAL_ENV_VA_OPT_THIRD_ARG_(__VA_ARGS__))
        #define D_INTERNAL_ENV_VA_OPT_PROBE(...)                               \
            D_INTERNAL_ENV_VA_OPT_THIRD_ARG(__VA_OPT__(,), 1, 0, )
        #define D_ENV_PP_HAS_VA_OPT D_INTERNAL_ENV_VA_OPT_PROBE(~)
    #endif
#endif  // D_ENV_PP_HAS_VA_OPT

// 2.1.2
// D_ENV_PP_HAS_VA_OPT_ENABLED
//   macro: alias for D_ENV_PP_HAS_VA_OPT, for cleaner conditionals.
#define D_ENV_PP_HAS_VA_OPT_ENABLED  \
    D_ENV_PP_HAS_VA_OPT


//==============================================================================
// 3.  PREPROCESSOR LIMITS
//==============================================================================
// Translation limits: the minimums the C and C++ standards require every
// conforming implementation to support (D_ENV_PP_MIN_*), and the practical
// maximums documented or measured for each compiler (D_ENV_PP_MAX_*), which
// typically far exceed them. A maximum of 0 means the compiler imposes no
// hard limit.


// 3.1    Standard translation limits
//------------------------------------------------------------------------------
// 3.1.1
// C89 minimum limits

// 3.1.1.1
// D_ENV_PP_LIMIT_C89_MACRO_ARGS
//   constant: C89 / C90 minimum for arguments in one macro invocation.
#define D_ENV_PP_LIMIT_C89_MACRO_ARGS       31

// 3.1.1.2
// D_ENV_PP_LIMIT_C89_NESTING_DEPTH
//   constant: C89 / C90 minimum for nesting levels of #included files.
#define D_ENV_PP_LIMIT_C89_NESTING_DEPTH    8

// 3.1.1.3
// D_ENV_PP_LIMIT_C89_MACRO_IDS
//   constant: C89 / C90 minimum for macro identifiers defined at once.
#define D_ENV_PP_LIMIT_C89_MACRO_IDS        1024

// 3.1.1.4
// D_ENV_PP_LIMIT_C89_PARAMS
//   constant: C89 / C90 minimum for parameters in one function definition.
#define D_ENV_PP_LIMIT_C89_PARAMS           31

// 3.1.1.5
// D_ENV_PP_LIMIT_C89_STRING_LENGTH
//   constant: C89 / C90 minimum for characters in one string literal.
#define D_ENV_PP_LIMIT_C89_STRING_LENGTH    509

// 3.1.2
// C99 and later minimum limits

// 3.1.2.1
// D_ENV_PP_LIMIT_C99_MACRO_ARGS
//   constant: C99 and later minimum for arguments in one macro invocation.
#define D_ENV_PP_LIMIT_C99_MACRO_ARGS       127

// 3.1.2.2
// D_ENV_PP_LIMIT_C99_NESTING_DEPTH
//   constant: C99 and later minimum for nesting levels of #included files.
#define D_ENV_PP_LIMIT_C99_NESTING_DEPTH    15

// 3.1.2.3
// D_ENV_PP_LIMIT_C99_MACRO_IDS
//   constant: C99 and later minimum for macro identifiers defined at once.
#define D_ENV_PP_LIMIT_C99_MACRO_IDS        4095

// 3.1.2.4
// D_ENV_PP_LIMIT_C99_PARAMS
//   constant: C99 and later minimum for parameters in one function definition.
#define D_ENV_PP_LIMIT_C99_PARAMS           127

// 3.1.2.5
// D_ENV_PP_LIMIT_C99_STRING_LENGTH
//   constant: C99 and later minimum for characters in one string literal.
#define D_ENV_PP_LIMIT_C99_STRING_LENGTH    4095

// 3.1.3
// C++ minimum limits

// 3.1.3.1
// D_ENV_PP_LIMIT_CPP_MACRO_ARGS
//   constant: C++ minimum for arguments in one macro invocation.
#define D_ENV_PP_LIMIT_CPP_MACRO_ARGS       256

// 3.1.3.2
// D_ENV_PP_LIMIT_CPP_NESTING_DEPTH
//   constant: C++ minimum for nesting levels of #included files.
#define D_ENV_PP_LIMIT_CPP_NESTING_DEPTH    256

// 3.1.3.3
// D_ENV_PP_LIMIT_CPP_MACRO_IDS
//   constant: C++ minimum for macro identifiers defined at once.
#define D_ENV_PP_LIMIT_CPP_MACRO_IDS        65536

// 3.1.3.4
// D_ENV_PP_LIMIT_CPP_PARAMS
//   constant: C++ minimum for parameters in one function definition.
#define D_ENV_PP_LIMIT_CPP_PARAMS           256

// 3.1.3.5
// D_ENV_PP_LIMIT_CPP_STRING_LENGTH
//   constant: C++ minimum for characters in one string literal.
#define D_ENV_PP_LIMIT_CPP_STRING_LENGTH    65536

// 3.2    Standard-based minimums
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_PP_MIN_*
//   constant: D_ENV_PP_MIN_MACRO_ARGS, _NESTING_DEPTH, _MACRO_IDS, _PARAMS, and
// _STRING_LENGTH: the 3.1 limits of the detected language standard.
#ifdef D_ENV_LANG_CPP_STANDARD
    #define D_ENV_PP_MIN_MACRO_ARGS     D_ENV_PP_LIMIT_CPP_MACRO_ARGS
    #define D_ENV_PP_MIN_NESTING_DEPTH  D_ENV_PP_LIMIT_CPP_NESTING_DEPTH
    #define D_ENV_PP_MIN_MACRO_IDS      D_ENV_PP_LIMIT_CPP_MACRO_IDS
    #define D_ENV_PP_MIN_PARAMS         D_ENV_PP_LIMIT_CPP_PARAMS
    #define D_ENV_PP_MIN_STRING_LENGTH  D_ENV_PP_LIMIT_CPP_STRING_LENGTH
#elif D_ENV_LANG_IS_C99_OR_HIGHER
    #define D_ENV_PP_MIN_MACRO_ARGS     D_ENV_PP_LIMIT_C99_MACRO_ARGS
    #define D_ENV_PP_MIN_NESTING_DEPTH  D_ENV_PP_LIMIT_C99_NESTING_DEPTH
    #define D_ENV_PP_MIN_MACRO_IDS      D_ENV_PP_LIMIT_C99_MACRO_IDS
    #define D_ENV_PP_MIN_PARAMS         D_ENV_PP_LIMIT_C99_PARAMS
    #define D_ENV_PP_MIN_STRING_LENGTH  D_ENV_PP_LIMIT_C99_STRING_LENGTH
#else
    #define D_ENV_PP_MIN_MACRO_ARGS     D_ENV_PP_LIMIT_C89_MACRO_ARGS
    #define D_ENV_PP_MIN_NESTING_DEPTH  D_ENV_PP_LIMIT_C89_NESTING_DEPTH
    #define D_ENV_PP_MIN_MACRO_IDS      D_ENV_PP_LIMIT_C89_MACRO_IDS
    #define D_ENV_PP_MIN_PARAMS         D_ENV_PP_LIMIT_C89_PARAMS
    #define D_ENV_PP_MIN_STRING_LENGTH  D_ENV_PP_LIMIT_C89_STRING_LENGTH
#endif  // D_ENV_LANG_CPP_STANDARD

// 3.3    Compiler-specific practical limits
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_PP_MAX_* and D_ENV_PP_LIMIT_SOURCE
//   constant: D_ENV_PP_MAX_MACRO_ARGS, _NESTING_DEPTH, _MACRO_IDS, and
// _STRING_LENGTH for the detected compiler, with D_ENV_PP_LIMIT_SOURCE naming
// where the values come from.
// note: GCC and Clang do not impose hard limits on macro arguments; they are
// constrained only by available memory and recursion depth. The values below
// are conservative practical limits tested to work reliably.

#if defined(D_ENV_COMPILER_GCC)
    // GCC: No hard-coded limit; memory-constrained
    // tested to reliably handle 10000+ arguments
    #define D_ENV_PP_MAX_MACRO_ARGS         10000
    #define D_ENV_PP_MAX_NESTING_DEPTH      200
    #define D_ENV_PP_MAX_MACRO_IDS          0       // 0: no hard limit
    #define D_ENV_PP_MAX_STRING_LENGTH      0       // 0: no hard limit
    #define D_ENV_PP_LIMIT_SOURCE           "GCC (practical)"

#elif defined(D_ENV_COMPILER_CLANG)
    // Clang: Similar to GCC, no hard limit
    // default macro recursion depth is 256, adjustable via
    // -fmacro-backtrace-limit
    #define D_ENV_PP_MAX_MACRO_ARGS         10000
    #define D_ENV_PP_MAX_NESTING_DEPTH      256
    #define D_ENV_PP_MAX_MACRO_IDS          0       // 0: no hard limit
    #define D_ENV_PP_MAX_STRING_LENGTH      0       // 0: no hard limit
    #define D_ENV_PP_LIMIT_SOURCE           "Clang (practical)"

#elif defined(D_ENV_COMPILER_MSVC)
    // MSVC: More conservative, historically stricter limits
    // /Za (strict ANSI) mode may impose additional restrictions
    #define D_ENV_PP_MAX_MACRO_ARGS         127
    #define D_ENV_PP_MAX_NESTING_DEPTH      256
    #define D_ENV_PP_MAX_MACRO_IDS          16380
    #define D_ENV_PP_MAX_STRING_LENGTH      16380
    #define D_ENV_PP_LIMIT_SOURCE           "MSVC (documented)"

#elif defined(D_ENV_COMPILER_INTEL)
    // Intel C++: Generally follows GCC/Clang behavior on Linux,
    // MSVC behavior on Windows
    #if defined(D_ENV_PLATFORM_WINDOWS)
        #define D_ENV_PP_MAX_MACRO_ARGS     127
        #define D_ENV_PP_MAX_NESTING_DEPTH  256
        #define D_ENV_PP_MAX_MACRO_IDS      16380
        #define D_ENV_PP_MAX_STRING_LENGTH  16380
        #define D_ENV_PP_LIMIT_SOURCE       "Intel (Windows/MSVC-compat)"
    #else
        #define D_ENV_PP_MAX_MACRO_ARGS     10000
        #define D_ENV_PP_MAX_NESTING_DEPTH  200
        #define D_ENV_PP_MAX_MACRO_IDS      0
        #define D_ENV_PP_MAX_STRING_LENGTH  0
        #define D_ENV_PP_LIMIT_SOURCE       "Intel (Linux/GCC-compat)"
    #endif

#elif defined(D_ENV_COMPILER_BORLAND)
    // Borland/Turbo C++: Legacy compiler with stricter limits
    #define D_ENV_PP_MAX_MACRO_ARGS         32
    #define D_ENV_PP_MAX_NESTING_DEPTH      32
    #define D_ENV_PP_MAX_MACRO_IDS          1024
    #define D_ENV_PP_MAX_STRING_LENGTH      4096
    #define D_ENV_PP_LIMIT_SOURCE           "Borland (estimated)"

#else
    // unknown compiler: fall back to standard minimums
    #define D_ENV_PP_MAX_MACRO_ARGS         D_ENV_PP_MIN_MACRO_ARGS
    #define D_ENV_PP_MAX_NESTING_DEPTH      D_ENV_PP_MIN_NESTING_DEPTH
    #define D_ENV_PP_MAX_MACRO_IDS          D_ENV_PP_MIN_MACRO_IDS
    #define D_ENV_PP_MAX_STRING_LENGTH      D_ENV_PP_MIN_STRING_LENGTH
    #define D_ENV_PP_LIMIT_SOURCE           "Unknown (standard minimum)"

#endif

// 3.4    Limit utilities
//------------------------------------------------------------------------------
// 3.4.1
// D_ENV_PP_ARGS_WITHIN_LIMIT
//   macro: evaluates to 1 if the given count is within the practical limit.
// usage:
//   #if D_ENV_PP_ARGS_WITHIN_LIMIT(64)
//       // safe to use 64-argument macro
//   #endif
#define D_ENV_PP_ARGS_WITHIN_LIMIT(count) \
    ((count) <= D_ENV_PP_MAX_MACRO_ARGS)

// 3.4.2
// D_ENV_PP_ARGS_WITHIN_STANDARD
//   macro: evaluates to 1 if the given count is within the standard minimum.
// usage:
//   #if D_ENV_PP_ARGS_WITHIN_STANDARD(31)
//       // portable across all conforming compilers
//   #endif
#define D_ENV_PP_ARGS_WITHIN_STANDARD(count) \
    ((count) <= D_ENV_PP_MIN_MACRO_ARGS)

// 3.4.3
// D_ENV_PP_IS_UNLIMITED
//   macro: evaluates to 1 if the compiler has no hard limit (value is 0).
#define D_ENV_PP_IS_UNLIMITED(limit) ((limit) == 0)

// 3.4.4
// D_ENV_PP_EFFECTIVE_LIMIT
//   macro: returns the effective limit, treating 0 as a large practical value.
#define D_ENV_PP_EFFECTIVE_LIMIT(limit) \
    (D_ENV_PP_IS_UNLIMITED(limit) ? 2147483647L : (limit))


#endif  // DJINTERP_ENV_ENV_COMPILER_H
