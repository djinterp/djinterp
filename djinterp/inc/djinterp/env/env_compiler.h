/******************************************************************************
* djinterp [core]                                               env_compiler.h
*
* djinterp compiler detection + preprocessor limits:
*   Compiler identification and version (the D_ENV_COMPILER_* interface),
* __VA_OPT__ availability detection, and the preprocessor translation-limit
* interface (D_ENV_PP_*). The preprocessor-limits block consults the
* D_ENV_PLATFORM_* flags from the OS section, so this header must be included
* AFTER env_os.h (the umbrella env.h arranges this). Compiler identity itself
* is independent of arch/OS and must precede env_c_lib.h.
*
*   Requires:  cfg_env.h, env_lang.h (for D_ENV_LANG_*), and env_os.h (for
*              D_ENV_PLATFORM_WINDOWS, used by the limits block). This header is
*              an internal component of env.h and is #included by it; do NOT
*              #include it directly.
*
*
* path:      /inc/djinterp/env/env_compiler.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.03.27
******************************************************************************/

#ifndef DJINTERP_ENV_COMPILER_
#define DJINTERP_ENV_COMPILER_ 1


// ===========================================================================
// I.   COMPILER DETECTION
// ===========================================================================

// compiler detection logic
#if D_CFG_ENV_COMPILER_ENABLED
    // clang (check first as it can masquerade as GCC)
    #if defined(__clang__)
        #define D_ENV_COMPILER_CLANG 1

        #ifdef __apple_build_version__
            #define D_ENV_COMPILER_APPLE_CLANG  1
            #define D_ENV_COMPILER_NAME         "Apple Clang"
            #define D_ENV_COMPILER_FULL_NAME    "Apple Clang/LLVM"
        #else
            #define D_ENV_COMPILER_NAME         "Clang"
            #define D_ENV_COMPILER_FULL_NAME    "Clang/LLVM"
        #endif

        #define D_ENV_COMPILER_MAJOR            __clang_major__
        #define D_ENV_COMPILER_MINOR            __clang_minor__
        #define D_ENV_COMPILER_PATCHLEVEL       __clang_patchlevel__
        #define D_ENV_COMPILER_VERSION_STRING   __clang_version__

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
        #endif

        #ifdef __VERSION__
            #define D_ENV_COMPILER_VERSION_STRING __VERSION__
        #else
            #define D_ENV_COMPILER_VERSION_STRING "GCC (version unknown)"
        #endif

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
        #endif
        #define D_ENV_COMPILER_VERSION_STRING   "Intel C++"

    // Borland/Turbo C++
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
        #endif

        #define D_ENV_COMPILER_VERSION_STRING   "Borland C++"

    // unknown compiler
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
// use pre-defined detection variables when compiler detection is disabled
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
    #endif

    // default version info when using detected variables
    #ifndef D_ENV_COMPILER_MAJOR
        #define D_ENV_COMPILER_MAJOR          0
        #define D_ENV_COMPILER_MINOR          0
        #define D_ENV_COMPILER_PATCHLEVEL     0
        #define D_ENV_COMPILER_VERSION_STRING "simulated"
    #endif  // D_ENV_COMPILER_MAJOR
#endif  // !D_CFG_ENV_COMPILER_ENABLED

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

#ifndef D_ENV_PP_HAS_VA_OPT
    #ifdef D_ENV_LANG_USING_CPP
        #if ( defined(__cpp_va_opt) &&  \
              (__cpp_va_opt >= 201803L) )
            #define D_ENV_PP_HAS_VA_OPT 1
        #else
            #define D_ENV_PP_HAS_VA_OPT 0
        #endif  // defined(__cpp_va_opt) && (__cpp_va_opt >= 201803L)
    #else
        // detection method: when __VA_OPT__ is supported and __VA_ARGS__ is non-empty,
        // __VA_OPT__(,) expands to ",", which shifts argument selection.
        // We pass a dummy argument (~) to ensure __VA_ARGS__ is non-empty.
        //
        // If __VA_OPT__ works:   __VA_OPT__(,) -> "," -> PP_THIRD selects arg 2 (1)
        // If __VA_OPT__ literal: no comma inserted   -> PP_THIRD selects arg 3 (0)

        #define D_VA_OPT_THIRD_ARG_(a, b, c, ...) c
        #define D_VA_OPT_THIRD_ARG(...)           D_VA_OPT_THIRD_ARG_(__VA_ARGS__)
        #define D_VA_OPT_PROBE_(...)              D_VA_OPT_THIRD_ARG(__VA_OPT__(,), 1, 0, )
        #define D_ENV_PP_HAS_VA_OPT               D_VA_OPT_PROBE_(~)
    #endif  // D_ENV_LANG_USING_CPP
#endif  // D_ENV_PP_HAS_VA_OPT

// D_ENV_PP_HAS_VA_OPT_ENABLED
//   macro: function-style wrapper for cleaner conditionals
#define D_ENV_PP_HAS_VA_OPT_ENABLED  \
    D_ENV_PP_HAS_VA_OPT

// ===========================================================================
// II.  PREPROCESSOR LIMITS
// ===========================================================================
// This section defines preprocessor translation limits based on the C/C++
// standard and compiler-specific implementations. These limits describe the
// maximum capabilities guaranteed or supported by the environment.
//
// The C standard specifies MINIMUM limits that conforming implementations
// must support. Actual implementations typically exceed these minimums.
//
// Key limits defined:
//   D_ENV_PP_MIN_MACRO_ARGS      - Standard-mandated minimum macro arguments
//   D_ENV_PP_MAX_MACRO_ARGS      - Compiler-specific practical maximum
//   D_ENV_PP_MIN_NESTING_DEPTH   - Standard-mandated minimum #include nesting
//   D_ENV_PP_MAX_NESTING_DEPTH   - Compiler-specific practical maximum
//   D_ENV_PP_MIN_MACRO_IDS       - Standard-mandated minimum macro identifiers
//   D_ENV_PP_MIN_PARAMS          - Standard-mandated minimum function parameters
//   D_ENV_PP_MIN_STRING_LENGTH   - Standard-mandated minimum string literal length

// -----------------------------------------------------------------------------
// Standard Translation Limits (from ISO C/C++)
// -----------------------------------------------------------------------------
// C89/C90 (ANSI C) minimum limits:
#define D_ENV_PP_LIMIT_C89_MACRO_ARGS       31
#define D_ENV_PP_LIMIT_C89_NESTING_DEPTH    8
#define D_ENV_PP_LIMIT_C89_MACRO_IDS        1024
#define D_ENV_PP_LIMIT_C89_PARAMS           31
#define D_ENV_PP_LIMIT_C89_STRING_LENGTH    509

// C99/C11/C17/C23 minimum limits:
#define D_ENV_PP_LIMIT_C99_MACRO_ARGS       127
#define D_ENV_PP_LIMIT_C99_NESTING_DEPTH    15
#define D_ENV_PP_LIMIT_C99_MACRO_IDS        4095
#define D_ENV_PP_LIMIT_C99_PARAMS           127
#define D_ENV_PP_LIMIT_C99_STRING_LENGTH    4095

// C++ minimum limits (similar to C99+ for modern standards):
#define D_ENV_PP_LIMIT_CPP_MACRO_ARGS       256
#define D_ENV_PP_LIMIT_CPP_NESTING_DEPTH    256
#define D_ENV_PP_LIMIT_CPP_MACRO_IDS        65536
#define D_ENV_PP_LIMIT_CPP_PARAMS           256
#define D_ENV_PP_LIMIT_CPP_STRING_LENGTH    65536

// -----------------------------------------------------------------------------
// Standard-Based Minimum Limits
// -----------------------------------------------------------------------------
// D_ENV_PP_MIN_MACRO_ARGS
//   The minimum number of arguments in a macro invocation that any conforming
//   implementation must support, based on the detected language standard.
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
#endif

// -----------------------------------------------------------------------------
// Compiler-Specific Practical Limits
// -----------------------------------------------------------------------------
// These values represent practical/documented limits for specific compilers.
// Note: GCC and Clang do not impose hard limits on macro arguments; they are
// constrained only by available memory and recursion depth. The values below
// are conservative practical limits tested to work reliably.

#if defined(D_ENV_COMPILER_GCC)
    // GCC: No hard-coded limit; memory-constrained
    // tested to reliably handle 10000+ arguments
    #define D_ENV_PP_MAX_MACRO_ARGS         10000
    #define D_ENV_PP_MAX_NESTING_DEPTH      200
    #define D_ENV_PP_MAX_MACRO_IDS          0       // 0 = unlimited (memory-bound)
    #define D_ENV_PP_MAX_STRING_LENGTH      0       // 0 = unlimited (memory-bound)
    #define D_ENV_PP_LIMIT_SOURCE           "GCC (practical)"

#elif defined(D_ENV_COMPILER_CLANG)
    // Clang: Similar to GCC, no hard limit
    // default macro recursion depth is 256, adjustable via -fmacro-backtrace-limit
    #define D_ENV_PP_MAX_MACRO_ARGS         10000
    #define D_ENV_PP_MAX_NESTING_DEPTH      256
    #define D_ENV_PP_MAX_MACRO_IDS          0       // 0 = unlimited (memory-bound)
    #define D_ENV_PP_MAX_STRING_LENGTH      0       // 0 = unlimited (memory-bound)
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

// -----------------------------------------------------------------------------
// Preprocessor Limit Utility Macros
// -----------------------------------------------------------------------------

// D_ENV_PP_ARGS_WITHIN_LIMIT
//   macro: evaluates to 1 if the given count is within the practical limit.
// usage:
//   #if D_ENV_PP_ARGS_WITHIN_LIMIT(64)
//       // safe to use 64-argument macro
//   #endif
#define D_ENV_PP_ARGS_WITHIN_LIMIT(count) \
    ((count) <= D_ENV_PP_MAX_MACRO_ARGS)

// D_ENV_PP_ARGS_WITHIN_STANDARD
//   macro: evaluates to 1 if the given count is within the standard minimum.
// usage:
//   #if D_ENV_PP_ARGS_WITHIN_STANDARD(31)
//       // portable across all conforming compilers
//   #endif
#define D_ENV_PP_ARGS_WITHIN_STANDARD(count) \
    ((count) <= D_ENV_PP_MIN_MACRO_ARGS)

// D_ENV_PP_IS_UNLIMITED
//   macro: evaluates to 1 if the compiler has no hard limit (value is 0).
#define D_ENV_PP_IS_UNLIMITED(limit) ((limit) == 0)

// D_ENV_PP_EFFECTIVE_LIMIT
//   macro: returns the effective limit, treating 0 as a large practical value.
#define D_ENV_PP_EFFECTIVE_LIMIT(limit) \
    (D_ENV_PP_IS_UNLIMITED(limit) ? 2147483647L : (limit))


#endif  // DJINTERP_ENV_COMPILER_
