/*******************************************************************************
* djinterp [re_std]                                                     config.h
*
* The part of re_std's configuration that C can read as well as C++: which
* language and level is being compiled, by which compiler, whether the C
* library has its fixed-width-integer headers, whether `long long` exists, and
* how wide a pointer is.
*   config.hpp includes it and adds what only C++ needs; dstdint.h, which C
* code includes too, includes it alone. Like config.hpp it takes nothing from
* djinterp, spells everything RE_STD_, and lets every value be pre-defined.
* Each value answers exactly as its djinterp env counterpart did (noted at
* each), so a build that read both before sees no difference.
*
* path:      /inc/re_std/config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.01
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOB
    ----
    1.  Knob values
         1.  RE_STD_INTERNAL_CFG_IS_LITERAL
    2.  Knob
         1.  RE_STD_CFG_ISO_STRICT
2.  LANGUAGE
    --------
    1.  Language
    2.  Language level
3.  COMPILER
    --------
    1.  Compiler identity
    2.  Compiler version
4.  LIBRARY AND TYPES
    -----------------
    1.  Fixed-width-integer headers
    2.  long long
    3.  Pointer width
*/

#ifndef RE_STD_CONFIG_H
#define RE_STD_CONFIG_H 1


//==============================================================================
// 1.  KNOB
//==============================================================================


// 1.1    Knob values
//------------------------------------------------------------------------------
// 1.1.1
// RE_STD_INTERNAL_CFG_IS_LITERAL
//   macro (internal): 1 when `_x` is, or expands to, exactly one of the
// decimal literals 0, 1 or 2, the values re_std's knobs take; 0 for
// anything else. #if only. A knob is validated with it because arithmetic
// cannot tell a misspelled value from 0: #if reads an identifier nobody
// defined as 0, so `-DRE_STD_CFG_ISO_STRICT=yes` would pass a test against
// 0 and 1 and quietly mean "off", and a mistyped backend name would mean
// AUTO. The value is pasted onto a prefix instead: the three literals land
// on a macro defined as 1, and anything else on nothing. As djinterp's
// D_CFG_IS_INT_LITERAL, of which re_std may read nothing.
#define RE_STD_INTERNAL_CFG_LITERAL_0  1
#define RE_STD_INTERNAL_CFG_LITERAL_1  1
#define RE_STD_INTERNAL_CFG_LITERAL_2  1
#define RE_STD_INTERNAL_CFG_PASTE(_a, _b)                                      \
    _a ## _b
#define RE_STD_INTERNAL_CFG_PROBE(_value)                                      \
    RE_STD_INTERNAL_CFG_PASTE(RE_STD_INTERNAL_CFG_LITERAL_, _value)
#define RE_STD_INTERNAL_CFG_IS_LITERAL(_x)                                     \
    (RE_STD_INTERNAL_CFG_PROBE(_x) + 0)


// 1.2    Knob
//------------------------------------------------------------------------------
// 1.2.1
// RE_STD_CFG_ISO_STRICT
//   knob: 1 when the build wants ISO C or C++ only -- no compiler extension
// used, so below C99 and C++11 `long long` does not exist
// (RE_STD_HAS_LONG_LONG is 0, and what needs it is absent) and neither do
// the C library's <stdint.h> and <inttypes.h>. 0 by default; the literal 0
// or 1.
#ifndef RE_STD_CFG_ISO_STRICT
    #define RE_STD_CFG_ISO_STRICT 0
#endif  // RE_STD_CFG_ISO_STRICT

#if !RE_STD_INTERNAL_CFG_IS_LITERAL(RE_STD_CFG_ISO_STRICT)
    #error "RE_STD_CFG_ISO_STRICT must be 0 or 1"
#elif ( (RE_STD_CFG_ISO_STRICT != 0) &&                                        \
        (RE_STD_CFG_ISO_STRICT != 1) )
    #error "RE_STD_CFG_ISO_STRICT must be 0 or 1"
#endif


//==============================================================================
// 2.  LANGUAGE
//==============================================================================


// 2.1    Language
//------------------------------------------------------------------------------
// 2.1.1
// RE_STD_LANG_IS_CPP
//   macro: 1 when compiling C++, 0 when compiling C.
#ifdef __cplusplus
    #define RE_STD_LANG_IS_CPP 1
#else
    #define RE_STD_LANG_IS_CPP 0
#endif  // __cplusplus

// 2.2    Language level
//------------------------------------------------------------------------------
// 2.2.1
// RE_STD_INTERNAL_CPLUSPLUS
//   macro (internal): the C++ level being compiled, 0 in C. MSVC keeps
// __cplusplus at 199711L unless /Zc:__cplusplus is given and reports the real
// level in _MSVC_LANG (clang-cl sets both), so the larger of the two is
// taken.
#if !RE_STD_LANG_IS_CPP
    #define RE_STD_INTERNAL_CPLUSPLUS 0L
#elif ( (defined(_MSVC_LANG)) &&                                               \
        (_MSVC_LANG > __cplusplus) )
    #define RE_STD_INTERNAL_CPLUSPLUS _MSVC_LANG
#else
    #define RE_STD_INTERNAL_CPLUSPLUS __cplusplus
#endif

// 2.2.2
// RE_STD_LANG_IS_CPP11_OR_HIGHER, ..._CPP14_..., ..._CPP17_..., ..._CPP20_...
//   macro: 1 when compiling C++11 (C++14, C++17, C++20) or later, else 0 --
// in C too. As djinterp's D_ENV_LANG_IS_CPP*_OR_HIGHER.
#ifndef RE_STD_LANG_IS_CPP11_OR_HIGHER
    #define RE_STD_LANG_IS_CPP11_OR_HIGHER                                     \
        (RE_STD_INTERNAL_CPLUSPLUS >= 201103L)
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#ifndef RE_STD_LANG_IS_CPP14_OR_HIGHER
    #define RE_STD_LANG_IS_CPP14_OR_HIGHER                                     \
        (RE_STD_INTERNAL_CPLUSPLUS >= 201402L)
#endif  // RE_STD_LANG_IS_CPP14_OR_HIGHER

#ifndef RE_STD_LANG_IS_CPP17_OR_HIGHER
    #define RE_STD_LANG_IS_CPP17_OR_HIGHER                                     \
        (RE_STD_INTERNAL_CPLUSPLUS >= 201703L)
#endif  // RE_STD_LANG_IS_CPP17_OR_HIGHER

#ifndef RE_STD_LANG_IS_CPP20_OR_HIGHER
    #define RE_STD_LANG_IS_CPP20_OR_HIGHER                                     \
        (RE_STD_INTERNAL_CPLUSPLUS >= 202002L)
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

// 2.2.3
// RE_STD_LANG_IS_C99_OR_HIGHER
//   macro: 1 when compiling C99 or later, else 0 -- in C++ too. As djinterp's
// D_ENV_LANG_IS_C99_OR_HIGHER.
#ifndef RE_STD_LANG_IS_C99_OR_HIGHER
    #if ( (!RE_STD_LANG_IS_CPP)          &&                                    \
          (defined(__STDC_VERSION__))    )
        #if __STDC_VERSION__ >= 199901L
            #define RE_STD_LANG_IS_C99_OR_HIGHER 1
        #endif
    #endif

    #ifndef RE_STD_LANG_IS_C99_OR_HIGHER
        #define RE_STD_LANG_IS_C99_OR_HIGHER 0
    #endif  // RE_STD_LANG_IS_C99_OR_HIGHER
#endif  // RE_STD_LANG_IS_C99_OR_HIGHER


//==============================================================================
// 3.  COMPILER
//==============================================================================


// 3.1    Compiler identity
//------------------------------------------------------------------------------
// 3.1.1
// RE_STD_COMPILER_CLANG, RE_STD_COMPILER_GCC, RE_STD_COMPILER_MSVC,
// RE_STD_COMPILER_INTEL
//   macro: exactly one is defined, to 1, for the compiler in use (none for
// another); test them with defined(). Clang is checked first, since it also
// defines __GNUC__ and, as clang-cl, _MSC_VER; and GCC before Intel, which
// defines __GNUC__ on Linux -- the order djinterp's env layer uses.
//   RE_STD_COMPILER_MAJOR, _MINOR, _PATCHLEVEL: its version, 0 when unknown.
// For MSVC that is the compiler's own version, not the Visual Studio
// release's: 19.29 is _MSC_VER 1929 (Visual Studio 2019 16.10), the patch
// level the build number in _MSC_FULL_VER -- the numbers re_std's tests are
// written in.
#if ( (!defined(RE_STD_COMPILER_CLANG)) &&                                     \
      (!defined(RE_STD_COMPILER_GCC))   &&                                     \
      (!defined(RE_STD_COMPILER_MSVC))  &&                                     \
      (!defined(RE_STD_COMPILER_INTEL)) )
    #if defined(__clang__)
        #define RE_STD_COMPILER_CLANG           1
        #define RE_STD_COMPILER_MAJOR           __clang_major__
        #define RE_STD_COMPILER_MINOR           __clang_minor__
        #define RE_STD_COMPILER_PATCHLEVEL      __clang_patchlevel__
    #elif defined(__GNUC__)
        #define RE_STD_COMPILER_GCC             1
        #define RE_STD_COMPILER_MAJOR           __GNUC__
        #define RE_STD_COMPILER_MINOR           __GNUC_MINOR__
        #if defined(__GNUC_PATCHLEVEL__)
            #define RE_STD_COMPILER_PATCHLEVEL  __GNUC_PATCHLEVEL__
        #else
            #define RE_STD_COMPILER_PATCHLEVEL  0
        #endif  // __GNUC_PATCHLEVEL__
    #elif defined(_MSC_VER)
        #define RE_STD_COMPILER_MSVC            1
        #define RE_STD_COMPILER_MAJOR           (_MSC_VER / 100)
        #define RE_STD_COMPILER_MINOR           (_MSC_VER % 100)
        #if defined(_MSC_FULL_VER)
            #define RE_STD_COMPILER_PATCHLEVEL  (_MSC_FULL_VER % 100000)
        #endif  // _MSC_FULL_VER
    #elif ( (defined(__INTEL_COMPILER)) ||                                     \
            (defined(__ICL))            ||                                     \
            (defined(__ICC)) )
        #define RE_STD_COMPILER_INTEL           1
        #ifdef __INTEL_COMPILER
            #define RE_STD_COMPILER_MAJOR       (__INTEL_COMPILER / 100)
            #define RE_STD_COMPILER_MINOR       ((__INTEL_COMPILER % 100) / 10)
            #define RE_STD_COMPILER_PATCHLEVEL  (__INTEL_COMPILER % 10)
        #endif  // __INTEL_COMPILER
    #endif
#endif

#ifndef RE_STD_COMPILER_MAJOR
    #define RE_STD_COMPILER_MAJOR               0
#endif  // RE_STD_COMPILER_MAJOR
#ifndef RE_STD_COMPILER_MINOR
    #define RE_STD_COMPILER_MINOR               0
#endif  // RE_STD_COMPILER_MINOR
#ifndef RE_STD_COMPILER_PATCHLEVEL
    #define RE_STD_COMPILER_PATCHLEVEL          0
#endif  // RE_STD_COMPILER_PATCHLEVEL

// 3.2    Compiler version
//------------------------------------------------------------------------------
// 3.2.1
// RE_STD_COMPILER_VERSION_AT_LEAST
//   macro: 1 when the compiler in use is at least version major.minor.patch;
// meaningful only beside the RE_STD_COMPILER_* test that names which one.
#define RE_STD_COMPILER_VERSION_AT_LEAST(major, minor, patch)                  \
    ( (RE_STD_COMPILER_MAJOR > (major))     ||                                 \
     ( (RE_STD_COMPILER_MAJOR == (major)) &&                                   \
       (RE_STD_COMPILER_MINOR > (minor)) )  ||                                 \
     ( (RE_STD_COMPILER_MAJOR == (major)) &&                                   \
       (RE_STD_COMPILER_MINOR == (minor)) &&                                   \
       (RE_STD_COMPILER_PATCHLEVEL >= (patch)) ) )


//==============================================================================
// 4.  LIBRARY AND TYPES
//==============================================================================


// 4.1    Fixed-width-integer headers
//------------------------------------------------------------------------------
// 4.1.1
// RE_STD_HAS_HEADER_STDINT
//   macro: 1 when the C library's <stdint.h> may be included: from C99 and
// C++11; below them never in an ISO strict build, where it is not part of
// the language, and otherwise where __has_include finds it, or the compiler
// is GCC or MSVC from Visual Studio 2010 (_MSC_VER 1600); else 0. As
// djinterp's D_ENV_C_HAS_STDINT_H.
#ifndef RE_STD_HAS_HEADER_STDINT
    #if ( (RE_STD_LANG_IS_C99_OR_HIGHER) ||                                    \
          (RE_STD_LANG_IS_CPP11_OR_HIGHER) )
        #define RE_STD_HAS_HEADER_STDINT 1
    #elif RE_STD_CFG_ISO_STRICT
        #define RE_STD_HAS_HEADER_STDINT 0
    #elif defined(__has_include)
        #if __has_include(<stdint.h>)
            #define RE_STD_HAS_HEADER_STDINT 1
        #else
            #define RE_STD_HAS_HEADER_STDINT 0
        #endif
    #elif ( (defined(RE_STD_COMPILER_GCC))    ||                               \
            ( (defined(RE_STD_COMPILER_MSVC)) &&                               \
              (_MSC_VER >= 1600) ) )
        #define RE_STD_HAS_HEADER_STDINT 1
    #else
        #define RE_STD_HAS_HEADER_STDINT 0
    #endif
#endif  // RE_STD_HAS_HEADER_STDINT

// 4.1.2
// RE_STD_HAS_HEADER_INTTYPES
//   macro: 1 when the C library's <inttypes.h> may be included: never in a
// freestanding build, which need supply only <stdint.h>; from C99 and C++11;
// below them never in an ISO strict build, and otherwise where
// __has_include finds it, or the compiler is GCC or MSVC from Visual Studio
// 2013 (_MSC_VER 1800); else 0. As djinterp's D_ENV_C_HAS_INTTYPES_H.
//   A build that defines it first has the last word: 1 for a freestanding
// build whose C library has the header all the same, 0 for a build without
// one. RE_STD_INTERNAL_HEADER_INTTYPES_STATED is 1 where it did, and
// dstdint.h then looks no further; left to this header, a freestanding
// build still gets the <inttypes.h> of a C library that put its <stdint.h>
// ahead of the compiler's (dstdint.h, 1.3).
#ifdef RE_STD_HAS_HEADER_INTTYPES
    #define RE_STD_INTERNAL_HEADER_INTTYPES_STATED 1
#else
    #define RE_STD_INTERNAL_HEADER_INTTYPES_STATED 0
#endif  // RE_STD_HAS_HEADER_INTTYPES

#ifndef RE_STD_HAS_HEADER_INTTYPES
    #if ( (defined(__STDC_HOSTED__)) &&                                        \
          (__STDC_HOSTED__ == 0) )
        #define RE_STD_HAS_HEADER_INTTYPES 0
    #elif ( (RE_STD_LANG_IS_C99_OR_HIGHER) ||                                  \
            (RE_STD_LANG_IS_CPP11_OR_HIGHER) )
        #define RE_STD_HAS_HEADER_INTTYPES 1
    #elif RE_STD_CFG_ISO_STRICT
        #define RE_STD_HAS_HEADER_INTTYPES 0
    #elif defined(__has_include)
        #if __has_include(<inttypes.h>)
            #define RE_STD_HAS_HEADER_INTTYPES 1
        #else
            #define RE_STD_HAS_HEADER_INTTYPES 0
        #endif
    #elif ( (defined(RE_STD_COMPILER_GCC))    ||                               \
            ( (defined(RE_STD_COMPILER_MSVC)) &&                               \
              (_MSC_VER >= 1800) ) )
        #define RE_STD_HAS_HEADER_INTTYPES 1
    #else
        #define RE_STD_HAS_HEADER_INTTYPES 0
    #endif
#endif  // RE_STD_HAS_HEADER_INTTYPES

// 4.2    long long
//------------------------------------------------------------------------------
// 4.2.1
// RE_STD_HAS_LONG_LONG
//   macro: 1 when `long long` and `unsigned long long` exist: from C99 and
// C++11, and before them as the extension every major compiler offers,
// unless the build is ISO strict (RE_STD_CFG_ISO_STRICT). As djinterp's
// D_ENV_HAS_LONG_LONG.
#ifndef RE_STD_HAS_LONG_LONG
    #if ( (RE_STD_LANG_IS_CPP11_OR_HIGHER) ||                                  \
          (RE_STD_LANG_IS_C99_OR_HIGHER) )
        #define RE_STD_HAS_LONG_LONG 1
    #elif RE_STD_CFG_ISO_STRICT
        #define RE_STD_HAS_LONG_LONG 0
    #elif ( (defined(__GNUC__))         ||                                     \
            (defined(__clang__))        ||                                     \
            (defined(_MSC_VER))         ||                                     \
            (defined(__INTEL_COMPILER)) ||                                     \
            (defined(__IBMCPP__))       ||                                     \
            (defined(__SUNPRO_CC)) )
        #define RE_STD_HAS_LONG_LONG 1
    #else
        #define RE_STD_HAS_LONG_LONG 0
    #endif
#endif  // RE_STD_HAS_LONG_LONG

// 4.2.2
// RE_STD_LONG_LONG_DIAG_PUSH, RE_STD_LONG_LONG_DIAG_POP
//   macro: bracket code that spells `long long` so it builds without a
// diagnostic where the type is an extension (GCC and Clang below C99 and
// C++11, under -pedantic); empty everywhere else. Whether the type exists is
// RE_STD_HAS_LONG_LONG's question: gate the code on it too.
#ifndef RE_STD_LONG_LONG_DIAG_PUSH
    #if ( (RE_STD_HAS_LONG_LONG)                 &&                            \
          (!RE_STD_LANG_IS_CPP11_OR_HIGHER)      &&                            \
          (!RE_STD_LANG_IS_C99_OR_HIGHER)        &&                            \
          ( (defined(RE_STD_COMPILER_GCC)) ||                                  \
            (defined(RE_STD_COMPILER_CLANG)) ) )
        #define RE_STD_LONG_LONG_DIAG_PUSH                                     \
            _Pragma("GCC diagnostic push")                                     \
            _Pragma("GCC diagnostic ignored \"-Wlong-long\"")
        #define RE_STD_LONG_LONG_DIAG_POP                                      \
            _Pragma("GCC diagnostic pop")
    #else
        #define RE_STD_LONG_LONG_DIAG_PUSH
        #define RE_STD_LONG_LONG_DIAG_POP
    #endif
#endif  // RE_STD_LONG_LONG_DIAG_PUSH

// 4.3    Pointer width
//------------------------------------------------------------------------------
// 4.3.1
// RE_STD_POINTER_BITS
//   macro: the width of an object pointer (`void*`) in bits, as the target
// itself states it; 0 where it does not. From the compiler's size macro
// (GCC, Clang and the compilers that emulate them), else the data model the
// target names (Win64 and LP64 have 64-bit pointers, Win32 and ILP32 32-bit
// ones), else a 16-bit target the compiler names without stating a size
// (AVR; MSP430 but for its large memory model, whose pointers have 20 bits).
// It is the ABI's width, not the instruction set's: 32 on x32, MIPS n32 and
// arm64_32. As djinterp's D_ENV_ARCH_POINTER_BITS, but for that one's last
// resort, the width of the architecture env detected, which re_std keeps no
// table for.
#ifndef RE_STD_POINTER_BITS
    #if ( (defined(__SIZEOF_POINTER__)) &&                                     \
          (defined(__CHAR_BIT__)) )
        #if ((__SIZEOF_POINTER__ * __CHAR_BIT__) == 64)
            #define RE_STD_POINTER_BITS 64
        #elif ((__SIZEOF_POINTER__ * __CHAR_BIT__) == 32)
            #define RE_STD_POINTER_BITS 32
        #elif ((__SIZEOF_POINTER__ * __CHAR_BIT__) == 16)
            #define RE_STD_POINTER_BITS 16
        #else
            #define RE_STD_POINTER_BITS (__SIZEOF_POINTER__ * __CHAR_BIT__)
        #endif
    #elif ( (defined(_WIN64))   ||                                             \
            (defined(__LP64__)) ||                                             \
            (defined(_LP64)) )
        #define RE_STD_POINTER_BITS 64
    #elif ( (defined(_WIN32))    ||                                            \
            (defined(__ILP32__)) ||                                            \
            (defined(_ILP32)) )
        #define RE_STD_POINTER_BITS 32
    #elif ( (defined(__AVR__)) ||                                              \
            ( (defined(__MSP430__)) &&                                         \
              (!defined(__MSP430X_LARGE__)) ) )
        #define RE_STD_POINTER_BITS 16
    #else
        #define RE_STD_POINTER_BITS 0
    #endif
#endif  // RE_STD_POINTER_BITS


#endif  // RE_STD_CONFIG_H
