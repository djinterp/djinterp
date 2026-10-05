/*******************************************************************************
* djinterp [re_std]                                                    dstdint.h
*
* Portable fixed-width integer types: the <stdint.h> interface, with the format
* macros of <inttypes.h>, at every language level re_std supports.
*   Where the platform has those headers this one includes them, which is always
* in C, whose framework floor is C99, from C++11 in C++, and in C++98 wherever
* the compiler ships them. It then adds only what the platform leaves out: the
* format macros of a freestanding build, which need have only <stdint.h>, and
* the macros an older C++ library withholds. Otherwise -- a C++98 toolchain
* without the headers, or a C++98 build that asked for ISO strictness
* (RE_STD_CFG_ISO_STRICT), to which they do not belong -- it defines the same
* names itself; RE_STD_CFG_STDINT_BACKEND (section 1) can pin either backend.
* Either way the names are the standard ones: include this header in place of
* <stdint.h> or <inttypes.h>, and change nothing else.
*   C NAMES ONLY. Like <stdint.h>, this header declares its names in the global
* namespace and nowhere else, at every level, and includes no C++ header, so a
* C++ build without a C++ library (-nostdinc++, an embedded g++) can use it.
* C++ code that wants the names in a namespace includes cstdint.hpp, beside
* this file, which re-exports each of them as re_std::.
*   IDENTITY. Every type defined here is the exact type the platform's own
* <stdint.h> uses, taken from the compiler's predefined macros (__INT32_TYPE__
* and its family, which Clang and GCC from 4.5 provide in every language mode).
* The two paths therefore agree on overloads, name mangling and printf
* formats, and a translation unit may include the system header as well: its
* typedefs then redeclare identical types. Where a platform is known to
* declare a type the compiler does not predefine, the platform's is taken
* (sections 2.2.2 and 4.2 record each case, and what it was measured with).
*   ABSENT, NOT APPROXIMATED. A type that the platform spells `long long` exists
* only where `long long` does (RE_STD_HAS_LONG_LONG). Under ISO strict C++98 a
* 32-bit target therefore has no 64-bit types, 64-bit Windows has no intptr_t
* either, and macOS has no int64_t, which Apple spells `long long` although
* `long` is 64 bits there. No other type of the same width stands in, because
* that would break identity. A type is present exactly when its limit macro is
* defined, which is the test the C standard prescribes: `#ifdef INT64_MAX`.
*   The int_fastN_t types are the exception to that: no ABI fixes them, and a
* compiler's predefined macros can disagree with the C library (Clang
* predefines `short` for int_fast16_t on Linux; glibc declares `long`). The
* self-defined path declares them only where it knows which <stdint.h> the
* platform uses -- the compiler's own, or a C library whose choice section 2.4
* records -- and leaves them out elsewhere, with everything of theirs: a
* family is present or absent as a whole. Below C99 and C++11 the 8-bit scanf
* formats are left out too, since their `hh` modifier came with those, and so
* they are where the runtime's scanf has none (MinGW on Microsoft's
* msvcrt.dll, section 6.2.1).
*   A compiler with neither the headers nor the predefined macros gets types
* derived from <limits.h>. Their widths are exact, but where two standard types
* share a width the lower-ranked one is chosen (`int` over `long`, `long` over
* `long long`), which the platform may not do; `long long` is taken for a
* 64-bit type only where its width is proved, and no type stands in where it is
* not; intptr_t has the width of a pointer, and is left out where nothing
* states that width; intmax_t is left out where the platform has a wider
* `long long` than the build can use; and SIZE_MAX, PTRDIFF_MIN / _MAX,
* SIG_ATOMIC_*, WCHAR_* and WINT_* are not defined.
*   It is re_std's, and so stands on its own: it reads only config.h, which C
* reads too, and includes nothing from djinterp. It came from djinterp's
* c/re_std/, with the backend selection of cfg_dstdint.h folded into section
* 1 and every djinterp name spelled RE_STD_.
*
*
* path:      /inc/re_std/cstdint/dstdint.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BACKEND
    -------
    1.  Backend selection
         1.  RE_STD_CFG_STDINT_BACKEND_AUTO, _PLATFORM, _OWN
         2.  RE_STD_CFG_STDINT_BACKEND
         3.  RE_STD_INTERNAL_STDINT_*: the resolved backend
    2.  Backend in use
         1.  RE_STD_STDINT_BACKEND
         2.  RE_STD_STDINT_BACKEND_NAME
    3.  Platform headers
2.  TYPE DESCRIPTIONS
    -----------------
    1.  Spelling classes
         1.  RE_STD_INTERNAL_STDINT_CLASS
         2.  RE_STD_INTERNAL_STDINT_USABLE
    2.  Descriptions from the compiler
         1.  RE_STD_INTERNAL_STDINT_PREDEFINED
         2.  Family descriptions, RE_STD_INTERNAL_STDINT_PTR_OVERRULED
    3.  Descriptions from <limits.h>
         1.  Family descriptions
    4.  Descriptions of the fastest minimum-width families
         1.  RE_STD_INTERNAL_STDINT_FAST_HEADER
         2.  RE_STD_INTERNAL_STDINT_FAST_FROM, RE_STD_INTERNAL_STDINT_FASTN_IS
         3.  Fast family descriptions
    5.  Family availability
         1.  RE_STD_INTERNAL_STDINT_*_OK
3.  TYPES
    -----
    1.  Integer types
         1.  Exact-width integer types
         2.  Minimum-width integer types
         3.  Integer types capable of holding object pointers
         4.  Greatest-width integer types
         5.  Fastest minimum-width integer types
4.  LIMITS
    ------
    1.  Limits of the integer types
         1.  Exact-width limits
         2.  Minimum-width limits
         3.  Pointer-holding and greatest-width limits
         4.  Fastest minimum-width limits
    2.  Limits of other integer types
         1.  PTRDIFF_MIN / PTRDIFF_MAX
         2.  SIZE_MAX
         3.  SIG_ATOMIC_MIN / SIG_ATOMIC_MAX
         4.  WCHAR_MIN / WCHAR_MAX
         5.  WINT_MIN / WINT_MAX
5.  INTEGER CONSTANT MACROS
    -----------------------
    1.  Constants
         1.  RE_STD_INTERNAL_STDINT_C
         2.  Minimum-width constants
         3.  Greatest-width constants
6.  FORMAT MACROS
    -------------
    1.  Formatted families
         1.  RE_STD_INTERNAL_STDINT_*_FMT
    2.  Length modifiers
         1.  RE_STD_INTERNAL_STDINT_MS_STDIO
         2.  RE_STD_INTERNAL_STDINT_LEN
         3.  RE_STD_INTERNAL_STDINT_PRILEN_*
         4.  RE_STD_INTERNAL_STDINT_HAS_HH
         5.  RE_STD_INTERNAL_STDINT_IS_SHORT
         6.  RE_STD_INTERNAL_STDINT_SCNLEN_*
    3.  printf formats
         1.  PRI formats
    4.  scanf formats
         1.  SCN formats
*/

#ifndef RE_STD_CSTDINT_DSTDINT_H
#define RE_STD_CSTDINT_DSTDINT_H 1

// re_std
#include "../config.h"  // RE_STD_CFG_ISO_STRICT, RE_STD_HAS_HEADER_STDINT,
                        // RE_STD_HAS_HEADER_INTTYPES, RE_STD_HAS_LONG_LONG,
                        // RE_STD_LANG_*, RE_STD_COMPILER_*,
                        // RE_STD_POINTER_BITS,
                        // RE_STD_INTERNAL_CFG_IS_LITERAL,
                        // RE_STD_INTERNAL_HEADER_INTTYPES_STATED


//==============================================================================
// 1.  BACKEND
//==============================================================================
//   The platform's headers where config.h finds them, this header's own
// definitions where it does not, unless RE_STD_CFG_STDINT_BACKEND pins one.
// <inttypes.h> is preferred to <stdint.h> because it also carries the format
// macros, and it includes <stdint.h> itself.


// 1.1    Backend selection
//------------------------------------------------------------------------------
// 1.1.1
// RE_STD_CFG_STDINT_BACKEND_AUTO, _PLATFORM, _OWN
//   constant: the knob's three values. AUTO detects, preferring the
// platform's headers; PLATFORM is the platform's <inttypes.h>, or its
// <stdint.h> where that is all it has, completed here where it falls short
// of C99; OWN is this header's own definitions, taken from the compiler's
// predefined macros or derived from <limits.h> -- available everywhere, and
// pinning it where the platform has the headers is how the definitions are
// tested against them.
#define RE_STD_CFG_STDINT_BACKEND_AUTO      0
#define RE_STD_CFG_STDINT_BACKEND_PLATFORM  1
#define RE_STD_CFG_STDINT_BACKEND_OWN       2

// 1.1.2
// RE_STD_CFG_STDINT_BACKEND
//   knob: one of the three values above; AUTO by default. Pinning a backend
// the platform cannot provide is an error, not a reason to fall back, and
// so is a value that is none of the three: a misspelled name must not read
// as 0 and so as AUTO.
#ifndef RE_STD_CFG_STDINT_BACKEND
    #define RE_STD_CFG_STDINT_BACKEND RE_STD_CFG_STDINT_BACKEND_AUTO
#endif  // RE_STD_CFG_STDINT_BACKEND

#if !RE_STD_INTERNAL_CFG_IS_LITERAL(RE_STD_CFG_STDINT_BACKEND)
    #error "RE_STD_CFG_STDINT_BACKEND: not a RE_STD_CFG_STDINT_BACKEND_* value"
#endif

// 1.1.3
// RE_STD_INTERNAL_STDINT_PLATFORM_USABLE, _BACKEND, _USE_PLATFORM, _USE_OWN
//   constant (internal): the platform is usable where <inttypes.h> is, or
// <stdint.h> alone on a compiler whose predefined macros (__INT32_TYPE__ and
// its family) let this header supply the format macros <inttypes.h> would
// have; the backend is the knob's, or the platform's where usable, or this
// header's own.
#if RE_STD_HAS_HEADER_INTTYPES
    #define RE_STD_INTERNAL_STDINT_PLATFORM_USABLE 1
#elif ( (RE_STD_HAS_HEADER_STDINT) &&                                          \
        (defined(__INT32_TYPE__)) )
    #define RE_STD_INTERNAL_STDINT_PLATFORM_USABLE 1
#else
    #define RE_STD_INTERNAL_STDINT_PLATFORM_USABLE 0
#endif

#if (RE_STD_CFG_STDINT_BACKEND != RE_STD_CFG_STDINT_BACKEND_AUTO)
    #define RE_STD_INTERNAL_STDINT_BACKEND RE_STD_CFG_STDINT_BACKEND
#elif RE_STD_INTERNAL_STDINT_PLATFORM_USABLE
    #define RE_STD_INTERNAL_STDINT_BACKEND RE_STD_CFG_STDINT_BACKEND_PLATFORM
#else
    #define RE_STD_INTERNAL_STDINT_BACKEND RE_STD_CFG_STDINT_BACKEND_OWN
#endif

#if (RE_STD_INTERNAL_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_PLATFORM)
    #define RE_STD_INTERNAL_STDINT_USE_PLATFORM 1
    #define RE_STD_INTERNAL_STDINT_USE_OWN      0
#else
    #define RE_STD_INTERNAL_STDINT_USE_PLATFORM 0
    #define RE_STD_INTERNAL_STDINT_USE_OWN      1
#endif

#if ( (RE_STD_INTERNAL_STDINT_USE_PLATFORM) &&                                 \
      (!RE_STD_INTERNAL_STDINT_PLATFORM_USABLE) )
    #error "RE_STD_CFG_STDINT_BACKEND: the platform's <stdint.h> is unusable"
#endif

// 1.2    Backend in use
//------------------------------------------------------------------------------
// 1.2.1
// RE_STD_STDINT_BACKEND
//   constant: the backend in use, RE_STD_CFG_STDINT_BACKEND_PLATFORM or
// RE_STD_CFG_STDINT_BACKEND_OWN. Read-only; set RE_STD_CFG_STDINT_BACKEND
// to choose.
#define RE_STD_STDINT_BACKEND RE_STD_INTERNAL_STDINT_BACKEND

// 1.2.2
// RE_STD_STDINT_BACKEND_NAME
//   constant: the backend in use, as a string literal for diagnostics.
#if (RE_STD_INTERNAL_STDINT_USE_PLATFORM == 1)
    #define RE_STD_STDINT_BACKEND_NAME "platform"
#else
    #define RE_STD_STDINT_BACKEND_NAME "own"
#endif


// 1.3    Platform headers
//------------------------------------------------------------------------------
//   A C++ library older than C++11 may declare the limit, constant and format
// macros only for a program that asks for them first, as footnotes to C99
// recommended, so this header asks before including. Each request is defined
// empty, the form programs usually give it, and only if it is not defined
// already; and each one this header made it withdraws after the includes, as
// Clang's own <stdint.h> does with its own, so that a program which defines
// one later (`#define __STDC_FORMAT_MACROS 1`) redefines nothing. A platform
// header included before this one keeps what it withheld; sections 4 to 6
// supply it.
//   A build with <stdint.h> alone gets its formats from section 6, which
// must know the types for that. It knows the fastest minimum-width ones only
// for the compiler's own <stdint.h> (2.4.1). Any other is a C library's, put
// ahead of the compiler's on the include path, and that library's
// <inttypes.h>, where __has_include finds one, has the formats for its own
// types: it is included here, so that nothing is guessed.
//   Clang has an <inttypes.h> of its own, which __has_include finds whether
// or not a C library's stands behind it, and which is an error where none
// does (it only forwards to the next one on the path). Nothing a header can
// test tells the two cases apart, so a build with a <stdint.h> of its own
// and no <inttypes.h> at all must say so: RE_STD_HAS_HEADER_INTTYPES=0,
// which a build may also define as 1 (config.h, 4.1.2). Stated either way,
// nothing is looked for here.
#if (RE_STD_INTERNAL_STDINT_USE_PLATFORM == 1)
    #if (RE_STD_LANG_IS_CPP == 1)
        #ifndef __STDC_LIMIT_MACROS
            #define __STDC_LIMIT_MACROS
            #define RE_STD_INTERNAL_STDINT_ASKED_LIMIT_MACROS    1
        #endif  // __STDC_LIMIT_MACROS

        #ifndef __STDC_CONSTANT_MACROS
            #define __STDC_CONSTANT_MACROS
            #define RE_STD_INTERNAL_STDINT_ASKED_CONSTANT_MACROS 1
        #endif  // __STDC_CONSTANT_MACROS

        #ifndef __STDC_FORMAT_MACROS
            #define __STDC_FORMAT_MACROS
            #define RE_STD_INTERNAL_STDINT_ASKED_FORMAT_MACROS   1
        #endif  // __STDC_FORMAT_MACROS
    #endif  // RE_STD_LANG_IS_CPP == 1

    #if ( (defined(RE_STD_HAS_HEADER_INTTYPES)) &&                             \
          (RE_STD_HAS_HEADER_INTTYPES == 1) )
        // std
        #include <inttypes.h>  // intN_t, INTN_MAX, INTN_C, PRIdN, SCNdN, ...
    #else
        // std
        #include <stdint.h>    // intN_t, INTN_MAX, INTN_C, ...

        #if ( (!defined(_GCC_STDINT_H))                      &&                \
              (!defined(__CLANG_STDINT_H))                   &&                \
              (RE_STD_INTERNAL_HEADER_INTTYPES_STATED == 0)  &&                \
              (defined(__has_include)) )
            #if __has_include(<inttypes.h>)
                // std; "file not found" from inside Clang's own
                // <inttypes.h> means this build has none: see above
                #include <inttypes.h>  // PRIdN, SCNdN, ... of that library
            #endif
        #endif
    #endif

    #ifdef RE_STD_INTERNAL_STDINT_ASKED_LIMIT_MACROS
        #undef __STDC_LIMIT_MACROS
        #undef RE_STD_INTERNAL_STDINT_ASKED_LIMIT_MACROS
    #endif  // RE_STD_INTERNAL_STDINT_ASKED_LIMIT_MACROS

    #ifdef RE_STD_INTERNAL_STDINT_ASKED_CONSTANT_MACROS
        #undef __STDC_CONSTANT_MACROS
        #undef RE_STD_INTERNAL_STDINT_ASKED_CONSTANT_MACROS
    #endif  // RE_STD_INTERNAL_STDINT_ASKED_CONSTANT_MACROS

    #ifdef RE_STD_INTERNAL_STDINT_ASKED_FORMAT_MACROS
        #undef __STDC_FORMAT_MACROS
        #undef RE_STD_INTERNAL_STDINT_ASKED_FORMAT_MACROS
    #endif  // RE_STD_INTERNAL_STDINT_ASKED_FORMAT_MACROS
#endif  // RE_STD_INTERNAL_STDINT_USE_PLATFORM == 1


//==============================================================================
// 2.  TYPE DESCRIPTIONS
//==============================================================================
//   Each family of types -- 8, 16, 32 and 64 exact-width, LEAST8 to LEAST64,
// PTR and MAX -- is described by four internal macros: its signed and unsigned
// types (RE_STD_INTERNAL_STDINT_<F>_S / _U) and their maxima (_SMAX / _UMAX).
// The maxima are the compiler's own literals, and a literal's suffix names the
// type it has, which is how this header learns, without inspecting a type,
// whether the platform spells it `long long`, what an INTN_C constant needs,
// and which printf length modifier matches.


// 2.1    Spelling classes
//------------------------------------------------------------------------------
// 2.1.1
// RE_STD_INTERNAL_STDINT_CLASS
//   macro (internal): the spelling class of an integer maximum, from its
// suffix: 1 none (int, or a type that promotes to int), 2 U, 3 L, 4 UL, 5 LL,
// 6 ULL, and 0 for a spelling not in the table below. The maximum is pasted
// onto a prefix to form an identifier, so the literal is never evaluated and
// a `long long` literal draws no diagnostic.
#define RE_STD_INTERNAL_STDINT_CAT_(_a, _b)  _a##_b
#define RE_STD_INTERNAL_STDINT_CAT(_a, _b)   RE_STD_INTERNAL_STDINT_CAT_(_a, _b)
#define RE_STD_INTERNAL_STDINT_CLASS(_max)                                     \
    RE_STD_INTERNAL_STDINT_CAT(RE_STD_INTERNAL_STDINT_SFX_, _max)

// the maxima GCC and Clang predefine, in both of their spellings
#define RE_STD_INTERNAL_STDINT_SFX_127                     1
#define RE_STD_INTERNAL_STDINT_SFX_0x7f                    1
#define RE_STD_INTERNAL_STDINT_SFX_255                     1
#define RE_STD_INTERNAL_STDINT_SFX_0xff                    1
#define RE_STD_INTERNAL_STDINT_SFX_32767                   1
#define RE_STD_INTERNAL_STDINT_SFX_0x7fff                  1
#define RE_STD_INTERNAL_STDINT_SFX_65535                   1
#define RE_STD_INTERNAL_STDINT_SFX_0xffff                  1
#define RE_STD_INTERNAL_STDINT_SFX_2147483647              1
#define RE_STD_INTERNAL_STDINT_SFX_0x7fffffff              1
#define RE_STD_INTERNAL_STDINT_SFX_65535U                  2
#define RE_STD_INTERNAL_STDINT_SFX_0xffffU                 2
#define RE_STD_INTERNAL_STDINT_SFX_4294967295U             2
#define RE_STD_INTERNAL_STDINT_SFX_0xffffffffU             2
#define RE_STD_INTERNAL_STDINT_SFX_2147483647L             3
#define RE_STD_INTERNAL_STDINT_SFX_0x7fffffffL             3
#define RE_STD_INTERNAL_STDINT_SFX_9223372036854775807L    3
#define RE_STD_INTERNAL_STDINT_SFX_0x7fffffffffffffffL     3
#define RE_STD_INTERNAL_STDINT_SFX_4294967295UL            4
#define RE_STD_INTERNAL_STDINT_SFX_0xffffffffUL            4
#define RE_STD_INTERNAL_STDINT_SFX_18446744073709551615UL  4
#define RE_STD_INTERNAL_STDINT_SFX_0xffffffffffffffffUL    4
#define RE_STD_INTERNAL_STDINT_SFX_9223372036854775807LL   5
#define RE_STD_INTERNAL_STDINT_SFX_0x7fffffffffffffffLL    5
#define RE_STD_INTERNAL_STDINT_SFX_18446744073709551615ULL 6
#define RE_STD_INTERNAL_STDINT_SFX_0xffffffffffffffffULL   6

// 2.1.2
// RE_STD_INTERNAL_STDINT_USABLE
//   macro (internal): 1 when a type whose maximum is `_max` can be spelled in
// this build: its class is known, and it is not `long long`, or `long long`
// is available. #if only.
#define RE_STD_INTERNAL_STDINT_USABLE(_max)                                    \
    ( (RE_STD_INTERNAL_STDINT_CLASS(_max) >= 1) &&                             \
      ( (RE_STD_INTERNAL_STDINT_CLASS(_max) <= 4) ||                           \
        (RE_STD_HAS_LONG_LONG == 1) ) )


// 2.2    Descriptions from the compiler
//------------------------------------------------------------------------------
// 2.2.1
// RE_STD_INTERNAL_STDINT_PREDEFINED
//   constant (internal): 1 when the compiler predefines the <stdint.h> types
// (GCC 4.5 and later, Clang, and compilers that emulate them). Pre-defining
// RE_STD_INTERNAL_STDINT_USE_LIMITS to 1 forces the <limits.h> derivation
// instead, so that tests can exercise it on those compilers.
#if ( (defined(__INT32_TYPE__)) &&                                            \
      ( (!defined(RE_STD_INTERNAL_STDINT_USE_LIMITS)) ||                       \
        (RE_STD_INTERNAL_STDINT_USE_LIMITS != 1) ) )
    #define RE_STD_INTERNAL_STDINT_PREDEFINED 1
#else
    #define RE_STD_INTERNAL_STDINT_PREDEFINED 0
#endif

// 2.2.2
// Family descriptions, RE_STD_INTERNAL_STDINT_PTR_OVERRULED
//   each family is described only if the compiler predefines all four of its
// macros, or, for a minimum-width family, the exact-width family of its width
// is described; the family is then exactly what the platform's <stdint.h>
// names. RE_STD_INTERNAL_STDINT_PTR_OVERRULED is 1 in the one case where it
// would not be, and the platform's choice is taken over the compiler's
// (below, at the pointer-holding family).
#if (RE_STD_INTERNAL_STDINT_PREDEFINED == 1)
    #if ( (defined(__INT8_TYPE__))  &&                                        \
          (defined(__UINT8_TYPE__)) &&                                        \
          (defined(__INT8_MAX__))   &&                                        \
          (defined(__UINT8_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_8_S           __INT8_TYPE__
        #define RE_STD_INTERNAL_STDINT_8_U           __UINT8_TYPE__
        #define RE_STD_INTERNAL_STDINT_8_SMAX        __INT8_MAX__
        #define RE_STD_INTERNAL_STDINT_8_UMAX        __UINT8_MAX__
    #endif

    #if ( (defined(__INT16_TYPE__))  &&                                       \
          (defined(__UINT16_TYPE__)) &&                                       \
          (defined(__INT16_MAX__))   &&                                       \
          (defined(__UINT16_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_16_S          __INT16_TYPE__
        #define RE_STD_INTERNAL_STDINT_16_U          __UINT16_TYPE__
        #define RE_STD_INTERNAL_STDINT_16_SMAX       __INT16_MAX__
        #define RE_STD_INTERNAL_STDINT_16_UMAX       __UINT16_MAX__
    #endif

    #if ( (defined(__INT32_TYPE__))  &&                                       \
          (defined(__UINT32_TYPE__)) &&                                       \
          (defined(__INT32_MAX__))   &&                                       \
          (defined(__UINT32_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_32_S          __INT32_TYPE__
        #define RE_STD_INTERNAL_STDINT_32_U          __UINT32_TYPE__
        #define RE_STD_INTERNAL_STDINT_32_SMAX       __INT32_MAX__
        #define RE_STD_INTERNAL_STDINT_32_UMAX       __UINT32_MAX__
    #endif

    #if ( (defined(__INT64_TYPE__))  &&                                       \
          (defined(__UINT64_TYPE__)) &&                                       \
          (defined(__INT64_MAX__))   &&                                       \
          (defined(__UINT64_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_64_S          __INT64_TYPE__
        #define RE_STD_INTERNAL_STDINT_64_U          __UINT64_TYPE__
        #define RE_STD_INTERNAL_STDINT_64_SMAX       __INT64_MAX__
        #define RE_STD_INTERNAL_STDINT_64_UMAX       __UINT64_MAX__
    #endif

    // the minimum-width types are the exact-width ones wherever those exist,
    // as C libraries and the compilers' own headers make them; Clang
    // predefines `long` for int_least64_t on 64-bit OpenBSD, for one, where
    // int64_t, and the C library's int_least64_t, are `long long`
    #if defined(RE_STD_INTERNAL_STDINT_8_S)
        #define RE_STD_INTERNAL_STDINT_LEAST8_S      RE_STD_INTERNAL_STDINT_8_S
        #define RE_STD_INTERNAL_STDINT_LEAST8_U      RE_STD_INTERNAL_STDINT_8_U
        #define RE_STD_INTERNAL_STDINT_LEAST8_SMAX                             \
            RE_STD_INTERNAL_STDINT_8_SMAX
        #define RE_STD_INTERNAL_STDINT_LEAST8_UMAX                             \
            RE_STD_INTERNAL_STDINT_8_UMAX
    #elif ( (defined(__INT_LEAST8_TYPE__))  &&                                \
            (defined(__UINT_LEAST8_TYPE__)) &&                                \
            (defined(__INT_LEAST8_MAX__))   &&                                \
            (defined(__UINT_LEAST8_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_LEAST8_S      __INT_LEAST8_TYPE__
        #define RE_STD_INTERNAL_STDINT_LEAST8_U      __UINT_LEAST8_TYPE__
        #define RE_STD_INTERNAL_STDINT_LEAST8_SMAX   __INT_LEAST8_MAX__
        #define RE_STD_INTERNAL_STDINT_LEAST8_UMAX   __UINT_LEAST8_MAX__
    #endif

    #if defined(RE_STD_INTERNAL_STDINT_16_S)
        #define RE_STD_INTERNAL_STDINT_LEAST16_S     RE_STD_INTERNAL_STDINT_16_S
        #define RE_STD_INTERNAL_STDINT_LEAST16_U     RE_STD_INTERNAL_STDINT_16_U
        #define RE_STD_INTERNAL_STDINT_LEAST16_SMAX                            \
            RE_STD_INTERNAL_STDINT_16_SMAX
        #define RE_STD_INTERNAL_STDINT_LEAST16_UMAX                            \
            RE_STD_INTERNAL_STDINT_16_UMAX
    #elif ( (defined(__INT_LEAST16_TYPE__))  &&                               \
            (defined(__UINT_LEAST16_TYPE__)) &&                               \
            (defined(__INT_LEAST16_MAX__))   &&                               \
            (defined(__UINT_LEAST16_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_LEAST16_S     __INT_LEAST16_TYPE__
        #define RE_STD_INTERNAL_STDINT_LEAST16_U     __UINT_LEAST16_TYPE__
        #define RE_STD_INTERNAL_STDINT_LEAST16_SMAX  __INT_LEAST16_MAX__
        #define RE_STD_INTERNAL_STDINT_LEAST16_UMAX  __UINT_LEAST16_MAX__
    #endif

    #if defined(RE_STD_INTERNAL_STDINT_32_S)
        #define RE_STD_INTERNAL_STDINT_LEAST32_S     RE_STD_INTERNAL_STDINT_32_S
        #define RE_STD_INTERNAL_STDINT_LEAST32_U     RE_STD_INTERNAL_STDINT_32_U
        #define RE_STD_INTERNAL_STDINT_LEAST32_SMAX                            \
            RE_STD_INTERNAL_STDINT_32_SMAX
        #define RE_STD_INTERNAL_STDINT_LEAST32_UMAX                            \
            RE_STD_INTERNAL_STDINT_32_UMAX
    #elif ( (defined(__INT_LEAST32_TYPE__))  &&                               \
            (defined(__UINT_LEAST32_TYPE__)) &&                               \
            (defined(__INT_LEAST32_MAX__))   &&                               \
            (defined(__UINT_LEAST32_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_LEAST32_S     __INT_LEAST32_TYPE__
        #define RE_STD_INTERNAL_STDINT_LEAST32_U     __UINT_LEAST32_TYPE__
        #define RE_STD_INTERNAL_STDINT_LEAST32_SMAX  __INT_LEAST32_MAX__
        #define RE_STD_INTERNAL_STDINT_LEAST32_UMAX  __UINT_LEAST32_MAX__
    #endif

    #if defined(RE_STD_INTERNAL_STDINT_64_S)
        #define RE_STD_INTERNAL_STDINT_LEAST64_S     RE_STD_INTERNAL_STDINT_64_S
        #define RE_STD_INTERNAL_STDINT_LEAST64_U     RE_STD_INTERNAL_STDINT_64_U
        #define RE_STD_INTERNAL_STDINT_LEAST64_SMAX                            \
            RE_STD_INTERNAL_STDINT_64_SMAX
        #define RE_STD_INTERNAL_STDINT_LEAST64_UMAX                            \
            RE_STD_INTERNAL_STDINT_64_UMAX
    #elif ( (defined(__INT_LEAST64_TYPE__))  &&                               \
            (defined(__UINT_LEAST64_TYPE__)) &&                               \
            (defined(__INT_LEAST64_MAX__))   &&                               \
            (defined(__UINT_LEAST64_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_LEAST64_S     __INT_LEAST64_TYPE__
        #define RE_STD_INTERNAL_STDINT_LEAST64_U     __UINT_LEAST64_TYPE__
        #define RE_STD_INTERNAL_STDINT_LEAST64_SMAX  __INT_LEAST64_MAX__
        #define RE_STD_INTERNAL_STDINT_LEAST64_UMAX  __UINT_LEAST64_MAX__
    #endif

    //   one compiler is known to predefine a type the platform does not
    // use. On 32-bit MIPS Linux (the o32 and n32 ABIs) glibc and musl
    // declare intptr_t as `int`, which GCC predefines; Clang predefined
    // `long` there until version 21, and its own <stdint.h>, which a
    // freestanding build takes, declares what it predefines. So in a
    // hosted build for such a target, where the compiler says `long` the
    // family is the 32-bit exact-width one, which is `int`
    #if ( (defined(__linux__))                      &&                        \
          (defined(__mips__))                       &&                        \
          (RE_STD_POINTER_BITS == 32)               &&                        \
          (defined(__INTPTR_MAX__))                 &&                        \
          (defined(RE_STD_INTERNAL_STDINT_32_S)) )
        #if ( (defined(__STDC_HOSTED__)) &&                                   \
              (__STDC_HOSTED__ == 0) )
            //   the compiler's own header: its word stands
        #elif (RE_STD_INTERNAL_STDINT_CLASS(__INTPTR_MAX__) == 3)
            #define RE_STD_INTERNAL_STDINT_PTR_OVERRULED 1
            #define RE_STD_INTERNAL_STDINT_PTR_S                              \
                RE_STD_INTERNAL_STDINT_32_S
            #define RE_STD_INTERNAL_STDINT_PTR_U                              \
                RE_STD_INTERNAL_STDINT_32_U
            #define RE_STD_INTERNAL_STDINT_PTR_SMAX                           \
                RE_STD_INTERNAL_STDINT_32_SMAX
            #define RE_STD_INTERNAL_STDINT_PTR_UMAX                           \
                RE_STD_INTERNAL_STDINT_32_UMAX
        #endif
    #endif

    #if ( (!defined(RE_STD_INTERNAL_STDINT_PTR_S)) &&                         \
          (defined(__INTPTR_TYPE__))               &&                         \
          (defined(__UINTPTR_TYPE__))              &&                         \
          (defined(__INTPTR_MAX__))                &&                         \
          (defined(__UINTPTR_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_PTR_S         __INTPTR_TYPE__
        #define RE_STD_INTERNAL_STDINT_PTR_U         __UINTPTR_TYPE__
        #define RE_STD_INTERNAL_STDINT_PTR_SMAX      __INTPTR_MAX__
        #define RE_STD_INTERNAL_STDINT_PTR_UMAX      __UINTPTR_MAX__
    #endif

    #if ( (defined(__INTMAX_TYPE__))  &&                                      \
          (defined(__UINTMAX_TYPE__)) &&                                      \
          (defined(__INTMAX_MAX__))   &&                                      \
          (defined(__UINTMAX_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_MAX_S         __INTMAX_TYPE__
        #define RE_STD_INTERNAL_STDINT_MAX_U         __UINTMAX_TYPE__
        #define RE_STD_INTERNAL_STDINT_MAX_SMAX      __INTMAX_MAX__
        #define RE_STD_INTERNAL_STDINT_MAX_UMAX      __UINTMAX_MAX__
    #endif

    // the largest `int`, for telling `short` from `int` in scanf formats
    #define RE_STD_INTERNAL_STDINT_INT_MAX           __INT_MAX__
#endif  // RE_STD_INTERNAL_STDINT_PREDEFINED == 1

#ifndef RE_STD_INTERNAL_STDINT_PTR_OVERRULED
    #define RE_STD_INTERNAL_STDINT_PTR_OVERRULED 0
#endif  // RE_STD_INTERNAL_STDINT_PTR_OVERRULED


// 2.3    Descriptions from <limits.h>
//------------------------------------------------------------------------------
// 2.3.1
// Family descriptions
//   used only on the self-defined path of a compiler without the predefined
// macros. Every comparison stays within the range of `long`, which is all a
// C++98 preprocessor promises. A platform's own <stdint.h>, where there is
// one, is never second-guessed from <limits.h>.
#if ( (RE_STD_INTERNAL_STDINT_PREDEFINED == 0) &&                              \
      (RE_STD_INTERNAL_STDINT_USE_OWN == 1) )
    // std
    #include <limits.h>  // SCHAR_MAX, SHRT_MAX, INT_MAX, LONG_MAX, ...

    #if ( (SCHAR_MAX == 127) &&                                               \
          (UCHAR_MAX == 255) )
        #define RE_STD_INTERNAL_STDINT_8_S           signed char
        #define RE_STD_INTERNAL_STDINT_8_U           unsigned char
        #define RE_STD_INTERNAL_STDINT_8_SMAX        127
        #define RE_STD_INTERNAL_STDINT_8_UMAX        255
    #endif

    #if (SHRT_MAX == 32767)
        #define RE_STD_INTERNAL_STDINT_16_S          short
        #define RE_STD_INTERNAL_STDINT_16_U          unsigned short
        #define RE_STD_INTERNAL_STDINT_16_SMAX       32767
        #if (INT_MAX > 32767)
            #define RE_STD_INTERNAL_STDINT_16_UMAX   65535
        #else
            #define RE_STD_INTERNAL_STDINT_16_UMAX   65535U
        #endif
    #elif (INT_MAX == 32767)
        #define RE_STD_INTERNAL_STDINT_16_S          int
        #define RE_STD_INTERNAL_STDINT_16_U          unsigned int
        #define RE_STD_INTERNAL_STDINT_16_SMAX       32767
        #define RE_STD_INTERNAL_STDINT_16_UMAX       65535U
    #endif

    #if (INT_MAX == 2147483647)
        #define RE_STD_INTERNAL_STDINT_32_S          int
        #define RE_STD_INTERNAL_STDINT_32_U          unsigned int
        #define RE_STD_INTERNAL_STDINT_32_SMAX       2147483647
        #define RE_STD_INTERNAL_STDINT_32_UMAX       4294967295U
    #elif (LONG_MAX == 2147483647)
        #define RE_STD_INTERNAL_STDINT_32_S          long
        #define RE_STD_INTERNAL_STDINT_32_U          unsigned long
        #define RE_STD_INTERNAL_STDINT_32_SMAX       2147483647L
        #define RE_STD_INTERNAL_STDINT_32_UMAX       4294967295UL
    #endif

    // the width of `long long`, where the build has the type and something
    // proves the width: the compiler's own size macro, or <limits.h>'s
    // maximum where the preprocessor is sure to hold it (from C99 and C++11
    // its arithmetic is intmax_t's; below them `long` is all it promises), or
    // the compiler being MSVC, which documents 64 bits. 0 where nothing
    // does, and then no type here is `long long`: it need not be 64 bits
    // wide (Clang's tce target has a 32-bit one)
    #if (RE_STD_HAS_LONG_LONG != 1)
        #define RE_STD_INTERNAL_STDINT_LLONG_BITS 0
    #elif ( (defined(__SIZEOF_LONG_LONG__)) &&                                \
            (defined(__CHAR_BIT__)) )
        #define RE_STD_INTERNAL_STDINT_LLONG_BITS                             \
            (__SIZEOF_LONG_LONG__ * __CHAR_BIT__)
    #elif ( ( (RE_STD_LANG_IS_C99_OR_HIGHER) ||                                \
              (RE_STD_LANG_IS_CPP11_OR_HIGHER) ) &&                            \
            (defined(ULLONG_MAX)) )
        #if ( ((ULLONG_MAX >> 31) >> 31) == 3 )
            #define RE_STD_INTERNAL_STDINT_LLONG_BITS 64
        #else
            #define RE_STD_INTERNAL_STDINT_LLONG_BITS 0
        #endif
    #elif defined(_MSC_VER)
        #define RE_STD_INTERNAL_STDINT_LLONG_BITS 64
    #else
        #define RE_STD_INTERNAL_STDINT_LLONG_BITS 0
    #endif

    // a 64-bit `long` is recognised by shifting, never by a 64-bit literal
    #if ( ((LONG_MAX >> 31) >> 31) == 1 )
        #define RE_STD_INTERNAL_STDINT_64_S          long
        #define RE_STD_INTERNAL_STDINT_64_U          unsigned long
        #define RE_STD_INTERNAL_STDINT_64_SMAX       9223372036854775807L
        #define RE_STD_INTERNAL_STDINT_64_UMAX       18446744073709551615UL
    #elif (RE_STD_INTERNAL_STDINT_LLONG_BITS == 64)
        #define RE_STD_INTERNAL_STDINT_64_S          long long
        #define RE_STD_INTERNAL_STDINT_64_U          unsigned long long
        #define RE_STD_INTERNAL_STDINT_64_SMAX       9223372036854775807LL
        #define RE_STD_INTERNAL_STDINT_64_UMAX       18446744073709551615ULL
    #endif

    // the minimum-width types are the exact-width ones
    #ifdef RE_STD_INTERNAL_STDINT_8_S
        #define RE_STD_INTERNAL_STDINT_LEAST8_S      RE_STD_INTERNAL_STDINT_8_S
        #define RE_STD_INTERNAL_STDINT_LEAST8_U      RE_STD_INTERNAL_STDINT_8_U
        #define RE_STD_INTERNAL_STDINT_LEAST8_SMAX                             \
            RE_STD_INTERNAL_STDINT_8_SMAX
        #define RE_STD_INTERNAL_STDINT_LEAST8_UMAX                             \
            RE_STD_INTERNAL_STDINT_8_UMAX
    #endif  // RE_STD_INTERNAL_STDINT_8_S

    #ifdef RE_STD_INTERNAL_STDINT_16_S
        #define RE_STD_INTERNAL_STDINT_LEAST16_S     RE_STD_INTERNAL_STDINT_16_S
        #define RE_STD_INTERNAL_STDINT_LEAST16_U     RE_STD_INTERNAL_STDINT_16_U
        #define RE_STD_INTERNAL_STDINT_LEAST16_SMAX                            \
            RE_STD_INTERNAL_STDINT_16_SMAX
        #define RE_STD_INTERNAL_STDINT_LEAST16_UMAX                            \
            RE_STD_INTERNAL_STDINT_16_UMAX
    #endif  // RE_STD_INTERNAL_STDINT_16_S

    #ifdef RE_STD_INTERNAL_STDINT_32_S
        #define RE_STD_INTERNAL_STDINT_LEAST32_S     RE_STD_INTERNAL_STDINT_32_S
        #define RE_STD_INTERNAL_STDINT_LEAST32_U     RE_STD_INTERNAL_STDINT_32_U
        #define RE_STD_INTERNAL_STDINT_LEAST32_SMAX                            \
            RE_STD_INTERNAL_STDINT_32_SMAX
        #define RE_STD_INTERNAL_STDINT_LEAST32_UMAX                            \
            RE_STD_INTERNAL_STDINT_32_UMAX
    #endif  // RE_STD_INTERNAL_STDINT_32_S

    #ifdef RE_STD_INTERNAL_STDINT_64_S
        #define RE_STD_INTERNAL_STDINT_LEAST64_S     RE_STD_INTERNAL_STDINT_64_S
        #define RE_STD_INTERNAL_STDINT_LEAST64_U     RE_STD_INTERNAL_STDINT_64_U
        #define RE_STD_INTERNAL_STDINT_LEAST64_SMAX                            \
            RE_STD_INTERNAL_STDINT_64_SMAX
        #define RE_STD_INTERNAL_STDINT_LEAST64_UMAX                            \
            RE_STD_INTERNAL_STDINT_64_UMAX
    #endif  // RE_STD_INTERNAL_STDINT_64_S

    // a pointer fits the type as wide as itself, and its width is the
    // ABI's (config.h's RE_STD_POINTER_BITS), not the architecture's: a
    // 64-bit architecture has 32-bit pointers on x32, MIPS n32 and arm64_32.
    // 64 bits: `long`, else a proved `long long` (64-bit Windows). 32 bits:
    // `int`, as glibc and MSVC choose (n32 and arm64_32 choose `long`), else
    // `long` where `int` has 16 bits. 16 bits: `int`. Where the width is
    // unknown (0) or none of these, no type is chosen to hold a pointer
    #if ( (RE_STD_POINTER_BITS == 64) &&                                       \
          ( ((LONG_MAX >> 31) >> 31) == 1 ) )
        #define RE_STD_INTERNAL_STDINT_PTR_S         long
        #define RE_STD_INTERNAL_STDINT_PTR_U         unsigned long
        #define RE_STD_INTERNAL_STDINT_PTR_SMAX      9223372036854775807L
        #define RE_STD_INTERNAL_STDINT_PTR_UMAX      18446744073709551615UL
    #elif ( (RE_STD_POINTER_BITS == 64) &&                                     \
            (RE_STD_INTERNAL_STDINT_LLONG_BITS == 64) )
        #define RE_STD_INTERNAL_STDINT_PTR_S         long long
        #define RE_STD_INTERNAL_STDINT_PTR_U         unsigned long long
        #define RE_STD_INTERNAL_STDINT_PTR_SMAX      9223372036854775807LL
        #define RE_STD_INTERNAL_STDINT_PTR_UMAX      18446744073709551615ULL
    #elif ( (RE_STD_POINTER_BITS == 32) &&                                     \
            (INT_MAX == 2147483647) )
        #define RE_STD_INTERNAL_STDINT_PTR_S         int
        #define RE_STD_INTERNAL_STDINT_PTR_U         unsigned int
        #define RE_STD_INTERNAL_STDINT_PTR_SMAX      2147483647
        #define RE_STD_INTERNAL_STDINT_PTR_UMAX      4294967295U
    #elif ( (RE_STD_POINTER_BITS == 32) &&                                     \
            (LONG_MAX == 2147483647) )
        #define RE_STD_INTERNAL_STDINT_PTR_S         long
        #define RE_STD_INTERNAL_STDINT_PTR_U         unsigned long
        #define RE_STD_INTERNAL_STDINT_PTR_SMAX      2147483647L
        #define RE_STD_INTERNAL_STDINT_PTR_UMAX      4294967295UL
    #elif ( (RE_STD_POINTER_BITS == 16) &&                                     \
            (INT_MAX == 32767) )
        #define RE_STD_INTERNAL_STDINT_PTR_S         int
        #define RE_STD_INTERNAL_STDINT_PTR_U         unsigned int
        #define RE_STD_INTERNAL_STDINT_PTR_SMAX      32767
        #define RE_STD_INTERNAL_STDINT_PTR_UMAX      65535U
    #endif

    // the greatest-width types are the widest exact-width ones. Without a
    // 64-bit type they are left out wherever the platform may have a wider
    // `long long` that this build cannot use, whose own intmax_t would then
    // be wider than any type here: in an ISO strict build, where <limits.h>
    // need not show one, and wherever the compiler or <limits.h> shows one
    // that is not proved to be 32 bits or fewer
    #if defined(RE_STD_INTERNAL_STDINT_64_S)
        #define RE_STD_INTERNAL_STDINT_MAX_S         RE_STD_INTERNAL_STDINT_64_S
        #define RE_STD_INTERNAL_STDINT_MAX_U         RE_STD_INTERNAL_STDINT_64_U
        #define RE_STD_INTERNAL_STDINT_MAX_SMAX                                \
            RE_STD_INTERNAL_STDINT_64_SMAX
        #define RE_STD_INTERNAL_STDINT_MAX_UMAX                                \
            RE_STD_INTERNAL_STDINT_64_UMAX
    #elif ( (defined(RE_STD_INTERNAL_STDINT_32_S))  &&                         \
            (RE_STD_CFG_ISO_STRICT == 0)        &&                             \
            ( ( (RE_STD_INTERNAL_STDINT_LLONG_BITS >= 1) &&                    \
                (RE_STD_INTERNAL_STDINT_LLONG_BITS <= 32) ) ||                 \
              ( (!defined(LLONG_MAX))            &&                           \
                (!defined(__LONG_LONG_MAX__))    &&                           \
                (!defined(__SIZEOF_LONG_LONG__)) &&                           \
                (!defined(_I64_MAX)) ) ) )
        #define RE_STD_INTERNAL_STDINT_MAX_S         RE_STD_INTERNAL_STDINT_32_S
        #define RE_STD_INTERNAL_STDINT_MAX_U         RE_STD_INTERNAL_STDINT_32_U
        #define RE_STD_INTERNAL_STDINT_MAX_SMAX                                \
            RE_STD_INTERNAL_STDINT_32_SMAX
        #define RE_STD_INTERNAL_STDINT_MAX_UMAX                                \
            RE_STD_INTERNAL_STDINT_32_UMAX
    #endif

    #define RE_STD_INTERNAL_STDINT_INT_MAX           INT_MAX
#endif  // RE_STD_INTERNAL_STDINT_PREDEFINED == 0 on the self-defined path


// 2.4    Descriptions of the fastest minimum-width families
//------------------------------------------------------------------------------
//   These types are not the ABI's to fix, as the other families' are: each
// <stdint.h> chooses them, and a compiler's predefined macros speak for the
// compiler's own header only (Clang predefines `short` for int_fast16_t on
// Linux, where glibc declares `long`). So they are described only where it
// is known which <stdint.h> declares them here -- the one the platform
// backend included, or the one it would include -- and are absent where it
// is not: no type of the same width stands in, since a function that takes
// an int_fast16_t would then have two ABIs.

// 2.4.1
// RE_STD_INTERNAL_STDINT_FAST_HEADER
//   constant (internal): whose <stdint.h> declares the fast types: 1 GCC's
// own (stdint-gcc.h), 2 Clang's own definitions, 3 a C library's, 0 unknown.
// A header that was included is known by the include guard it leaves behind,
// never by which compiler this is: _GCC_STDINT_H, and __CLANG_STDINT_H in a
// freestanding build, where Clang's header takes its own definitions; any
// other is a C library's. Where none was included, on the own backend, it is
// the header the platform backend would include: the compiler's own in a
// freestanding build, a C library's in a hosted one.
#if ( (defined(__STDC_HOSTED__)) &&                                            \
      (__STDC_HOSTED__ == 0) )
    #define RE_STD_INTERNAL_STDINT_FREESTANDING 1
#else
    #define RE_STD_INTERNAL_STDINT_FREESTANDING 0
#endif

#if (RE_STD_INTERNAL_STDINT_USE_PLATFORM == 1)
    #if defined(_GCC_STDINT_H)
        #define RE_STD_INTERNAL_STDINT_FAST_HEADER 1
    #elif ( (defined(__CLANG_STDINT_H)) &&                                     \
            (RE_STD_INTERNAL_STDINT_FREESTANDING == 1) )
        #define RE_STD_INTERNAL_STDINT_FAST_HEADER 2
    #else
        #define RE_STD_INTERNAL_STDINT_FAST_HEADER 3
    #endif
#elif (RE_STD_INTERNAL_STDINT_FREESTANDING == 1)
    #if defined(__clang__)
        #define RE_STD_INTERNAL_STDINT_FAST_HEADER 2
    #elif defined(__GNUC__)
        #define RE_STD_INTERNAL_STDINT_FAST_HEADER 1
    #else
        #define RE_STD_INTERNAL_STDINT_FAST_HEADER 0
    #endif
#else
    #define RE_STD_INTERNAL_STDINT_FAST_HEADER 3

    //   a C library names itself in its own headers; <limits.h> is one that
    // every build may include, and the <limits.h> derivation already has
    #if (RE_STD_INTERNAL_STDINT_PREDEFINED == 1)
        // std
        #include <limits.h>  // __GLIBC__, __WORDSIZE, __BIONIC__, __NEWLIB__
    #endif
#endif

// 2.4.2
// RE_STD_INTERNAL_STDINT_FAST_FROM, RE_STD_INTERNAL_STDINT_FASTN_IS
//   constant (internal): what describes the fast families: 0 nothing, and
// they are absent; 1 the compiler's predefined __INT_FASTn_TYPE__ macros; 2
// the minimum-width families; 3 the exact-width families whose widths
// RE_STD_INTERNAL_STDINT_FAST8_IS, _FAST16_IS, _FAST32_IS and _FAST64_IS
// name.
//   GCC's own header declares the predefined types (1) and Clang's the
// minimum-width ones (2). A C library is taken at its word, each row below
// read from that library's own <stdint.h>:
//     glibc, uClibc   signed char; then `long` for all three where
//                     __WORDSIZE is 64, else int, int, long long        (3)
//     bionic          as glibc, by __LP64__                             (3)
//     Apple, MinGW, WASI
//                     the exact-width types of 8, 16, 32 and 64 bits    (3)
//     FreeBSD, OpenBSD, DragonFly
//                     the 32-bit type for the first three, then the
//                     64-bit one                                        (3)
//     MSVC's runtime  signed char, int, int, long long                  (3)
//     NetBSD, newlib, picolibc, Cygwin, Emscripten
//                     whatever the compiler predefines: their headers
//                     take __INT_FASTn_TYPE__ where it exists           (1)
// A library that names itself nowhere (musl, for one) is known only to GCC,
// which is configured for its target's C library, so that its predefined
// types are that library's (1); to any other compiler it is unknown (0).
// One GCC is not: a GCC configured for glibc in front of another library,
// as Debian's and Ubuntu's musl-gcc is (the system's GCC behind a specs
// file), predefines glibc's types. GCC defines __gnu_linux__ only when
// configured for glibc (gcc/config/linux.h, under OPTION_GLIBC), so a real
// GCC with __gnu_linux__ and without __GLIBC__ or __UCLIBC__ is that case,
// and there the types are absent (0).
#if (RE_STD_INTERNAL_STDINT_FAST_HEADER == 1)
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 1
#elif (RE_STD_INTERNAL_STDINT_FAST_HEADER == 2)
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 2
#elif (RE_STD_INTERNAL_STDINT_FAST_HEADER != 3)
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 0
#elif ( ( (defined(__GLIBC__)) ||                                              \
          (defined(__UCLIBC__)) ) &&                                           \
        (defined(__WORDSIZE)) )
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 3
    #if (__WORDSIZE == 64)
        #define RE_STD_INTERNAL_STDINT_FAST8_IS  8
        #define RE_STD_INTERNAL_STDINT_FAST16_IS 64
        #define RE_STD_INTERNAL_STDINT_FAST32_IS 64
        #define RE_STD_INTERNAL_STDINT_FAST64_IS 64
    #else
        #define RE_STD_INTERNAL_STDINT_FAST8_IS  8
        #define RE_STD_INTERNAL_STDINT_FAST16_IS 32
        #define RE_STD_INTERNAL_STDINT_FAST32_IS 32
        #define RE_STD_INTERNAL_STDINT_FAST64_IS 64
    #endif
#elif defined(__BIONIC__)
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 3
    #if defined(__LP64__)
        #define RE_STD_INTERNAL_STDINT_FAST8_IS  8
        #define RE_STD_INTERNAL_STDINT_FAST16_IS 64
        #define RE_STD_INTERNAL_STDINT_FAST32_IS 64
        #define RE_STD_INTERNAL_STDINT_FAST64_IS 64
    #else
        #define RE_STD_INTERNAL_STDINT_FAST8_IS  8
        #define RE_STD_INTERNAL_STDINT_FAST16_IS 32
        #define RE_STD_INTERNAL_STDINT_FAST32_IS 32
        #define RE_STD_INTERNAL_STDINT_FAST64_IS 64
    #endif  // __LP64__
#elif ( (defined(__APPLE__))   ||                                              \
        (defined(__MINGW32__)) ||                                              \
        (defined(__wasi__)) )
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 3
    #define RE_STD_INTERNAL_STDINT_FAST8_IS  8
    #define RE_STD_INTERNAL_STDINT_FAST16_IS 16
    #define RE_STD_INTERNAL_STDINT_FAST32_IS 32
    #define RE_STD_INTERNAL_STDINT_FAST64_IS 64
#elif ( (defined(__FreeBSD__)) ||                                              \
        (defined(__OpenBSD__)) ||                                              \
        (defined(__DragonFly__)) )
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 3
    #define RE_STD_INTERNAL_STDINT_FAST8_IS  32
    #define RE_STD_INTERNAL_STDINT_FAST16_IS 32
    #define RE_STD_INTERNAL_STDINT_FAST32_IS 32
    #define RE_STD_INTERNAL_STDINT_FAST64_IS 64
#elif defined(_MSC_VER)
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 3
    #define RE_STD_INTERNAL_STDINT_FAST8_IS  8
    #define RE_STD_INTERNAL_STDINT_FAST16_IS 32
    #define RE_STD_INTERNAL_STDINT_FAST32_IS 32
    #define RE_STD_INTERNAL_STDINT_FAST64_IS 64
#elif ( (defined(__NetBSD__)) ||                                               \
        (defined(__NEWLIB__)) ||                                               \
        (defined(__EMSCRIPTEN__)) )
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 1
#elif ( (defined(__GNUC__))         &&                                         \
        (!defined(__clang__))       &&                                         \
        (!defined(__INTEL_COMPILER)) &&                                        \
        (defined(__gnu_linux__))    &&                                         \
        (!defined(__GLIBC__))       &&                                         \
        (!defined(__UCLIBC__)) )
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 0
#elif ( (defined(__GNUC__))   &&                                               \
        (!defined(__clang__)) &&                                               \
        (!defined(__INTEL_COMPILER)) )
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 1
#else
    #define RE_STD_INTERNAL_STDINT_FAST_FROM 0
#endif

// 2.4.3
// Fast family descriptions
//   as the other families' (2.2.2), from the source 2.4.2 names. On the own
// backend they describe the types section 3 declares. On the platform
// backend they serve what the header that declared the types left out: the
// formats, in a build with <stdint.h> alone, and the limits and formats an
// older C++ library withheld (sections 4 and 6).
//   RE_STD_INTERNAL_STDINT_OF(_n, _part) is the description macro `_part`
// (_S, _U, _SMAX, _UMAX) of the exact-width family `_n` bits wide, and
// RE_STD_INTERNAL_STDINT_N_DESCRIBED is 1 where that family is described;
// with them a fast family takes the description of the exact-width family
// the library's row names.
#define RE_STD_INTERNAL_STDINT_OF_(_n, _part)                                  \
    RE_STD_INTERNAL_STDINT_##_n##_part
#define RE_STD_INTERNAL_STDINT_OF(_n, _part)                                   \
    RE_STD_INTERNAL_STDINT_OF_(_n, _part)

#if defined(RE_STD_INTERNAL_STDINT_8_S)
    #define RE_STD_INTERNAL_STDINT_8_DESCRIBED  1
#else
    #define RE_STD_INTERNAL_STDINT_8_DESCRIBED  0
#endif  // RE_STD_INTERNAL_STDINT_8_S

#if defined(RE_STD_INTERNAL_STDINT_16_S)
    #define RE_STD_INTERNAL_STDINT_16_DESCRIBED 1
#else
    #define RE_STD_INTERNAL_STDINT_16_DESCRIBED 0
#endif  // RE_STD_INTERNAL_STDINT_16_S

#if defined(RE_STD_INTERNAL_STDINT_32_S)
    #define RE_STD_INTERNAL_STDINT_32_DESCRIBED 1
#else
    #define RE_STD_INTERNAL_STDINT_32_DESCRIBED 0
#endif  // RE_STD_INTERNAL_STDINT_32_S

#if defined(RE_STD_INTERNAL_STDINT_64_S)
    #define RE_STD_INTERNAL_STDINT_64_DESCRIBED 1
#else
    #define RE_STD_INTERNAL_STDINT_64_DESCRIBED 0
#endif  // RE_STD_INTERNAL_STDINT_64_S

#if (RE_STD_INTERNAL_STDINT_FAST_FROM == 1)
    #if ( (defined(__INT_FAST8_TYPE__))  &&                                    \
          (defined(__UINT_FAST8_TYPE__)) &&                                    \
          (defined(__INT_FAST8_MAX__))   &&                                    \
          (defined(__UINT_FAST8_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_FAST8_S     __INT_FAST8_TYPE__
        #define RE_STD_INTERNAL_STDINT_FAST8_U     __UINT_FAST8_TYPE__
        #define RE_STD_INTERNAL_STDINT_FAST8_SMAX  __INT_FAST8_MAX__
        #define RE_STD_INTERNAL_STDINT_FAST8_UMAX  __UINT_FAST8_MAX__
    #endif

    #if ( (defined(__INT_FAST16_TYPE__))  &&                                   \
          (defined(__UINT_FAST16_TYPE__)) &&                                   \
          (defined(__INT_FAST16_MAX__))   &&                                   \
          (defined(__UINT_FAST16_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_FAST16_S     __INT_FAST16_TYPE__
        #define RE_STD_INTERNAL_STDINT_FAST16_U     __UINT_FAST16_TYPE__
        #define RE_STD_INTERNAL_STDINT_FAST16_SMAX  __INT_FAST16_MAX__
        #define RE_STD_INTERNAL_STDINT_FAST16_UMAX  __UINT_FAST16_MAX__
    #endif

    #if ( (defined(__INT_FAST32_TYPE__))  &&                                   \
          (defined(__UINT_FAST32_TYPE__)) &&                                   \
          (defined(__INT_FAST32_MAX__))   &&                                   \
          (defined(__UINT_FAST32_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_FAST32_S     __INT_FAST32_TYPE__
        #define RE_STD_INTERNAL_STDINT_FAST32_U     __UINT_FAST32_TYPE__
        #define RE_STD_INTERNAL_STDINT_FAST32_SMAX  __INT_FAST32_MAX__
        #define RE_STD_INTERNAL_STDINT_FAST32_UMAX  __UINT_FAST32_MAX__
    #endif

    #if ( (defined(__INT_FAST64_TYPE__))  &&                                   \
          (defined(__UINT_FAST64_TYPE__)) &&                                   \
          (defined(__INT_FAST64_MAX__))   &&                                   \
          (defined(__UINT_FAST64_MAX__)) )
        #define RE_STD_INTERNAL_STDINT_FAST64_S     __INT_FAST64_TYPE__
        #define RE_STD_INTERNAL_STDINT_FAST64_U     __UINT_FAST64_TYPE__
        #define RE_STD_INTERNAL_STDINT_FAST64_SMAX  __INT_FAST64_MAX__
        #define RE_STD_INTERNAL_STDINT_FAST64_UMAX  __UINT_FAST64_MAX__
    #endif
#elif (RE_STD_INTERNAL_STDINT_FAST_FROM == 2)
    #if defined(RE_STD_INTERNAL_STDINT_LEAST8_S)
        #define RE_STD_INTERNAL_STDINT_FAST8_S                                 \
            RE_STD_INTERNAL_STDINT_LEAST8_S
        #define RE_STD_INTERNAL_STDINT_FAST8_U                                 \
            RE_STD_INTERNAL_STDINT_LEAST8_U
        #define RE_STD_INTERNAL_STDINT_FAST8_SMAX                              \
            RE_STD_INTERNAL_STDINT_LEAST8_SMAX
        #define RE_STD_INTERNAL_STDINT_FAST8_UMAX                              \
            RE_STD_INTERNAL_STDINT_LEAST8_UMAX
    #endif  // RE_STD_INTERNAL_STDINT_LEAST8_S

    #if defined(RE_STD_INTERNAL_STDINT_LEAST16_S)
        #define RE_STD_INTERNAL_STDINT_FAST16_S                                \
            RE_STD_INTERNAL_STDINT_LEAST16_S
        #define RE_STD_INTERNAL_STDINT_FAST16_U                                \
            RE_STD_INTERNAL_STDINT_LEAST16_U
        #define RE_STD_INTERNAL_STDINT_FAST16_SMAX                             \
            RE_STD_INTERNAL_STDINT_LEAST16_SMAX
        #define RE_STD_INTERNAL_STDINT_FAST16_UMAX                             \
            RE_STD_INTERNAL_STDINT_LEAST16_UMAX
    #endif  // RE_STD_INTERNAL_STDINT_LEAST16_S

    #if defined(RE_STD_INTERNAL_STDINT_LEAST32_S)
        #define RE_STD_INTERNAL_STDINT_FAST32_S                                \
            RE_STD_INTERNAL_STDINT_LEAST32_S
        #define RE_STD_INTERNAL_STDINT_FAST32_U                                \
            RE_STD_INTERNAL_STDINT_LEAST32_U
        #define RE_STD_INTERNAL_STDINT_FAST32_SMAX                             \
            RE_STD_INTERNAL_STDINT_LEAST32_SMAX
        #define RE_STD_INTERNAL_STDINT_FAST32_UMAX                             \
            RE_STD_INTERNAL_STDINT_LEAST32_UMAX
    #endif  // RE_STD_INTERNAL_STDINT_LEAST32_S

    #if defined(RE_STD_INTERNAL_STDINT_LEAST64_S)
        #define RE_STD_INTERNAL_STDINT_FAST64_S                                \
            RE_STD_INTERNAL_STDINT_LEAST64_S
        #define RE_STD_INTERNAL_STDINT_FAST64_U                                \
            RE_STD_INTERNAL_STDINT_LEAST64_U
        #define RE_STD_INTERNAL_STDINT_FAST64_SMAX                             \
            RE_STD_INTERNAL_STDINT_LEAST64_SMAX
        #define RE_STD_INTERNAL_STDINT_FAST64_UMAX                             \
            RE_STD_INTERNAL_STDINT_LEAST64_UMAX
    #endif  // RE_STD_INTERNAL_STDINT_LEAST64_S
#elif (RE_STD_INTERNAL_STDINT_FAST_FROM == 3)
    #if RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST8_IS, _DESCRIBED)
        #define RE_STD_INTERNAL_STDINT_FAST8_S                                 \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST8_IS, _S)
        #define RE_STD_INTERNAL_STDINT_FAST8_U                                 \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST8_IS, _U)
        #define RE_STD_INTERNAL_STDINT_FAST8_SMAX                              \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST8_IS, _SMAX)
        #define RE_STD_INTERNAL_STDINT_FAST8_UMAX                              \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST8_IS, _UMAX)
    #endif

    #if RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST16_IS, _DESCRIBED)
        #define RE_STD_INTERNAL_STDINT_FAST16_S                                \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST16_IS, _S)
        #define RE_STD_INTERNAL_STDINT_FAST16_U                                \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST16_IS, _U)
        #define RE_STD_INTERNAL_STDINT_FAST16_SMAX                             \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST16_IS, _SMAX)
        #define RE_STD_INTERNAL_STDINT_FAST16_UMAX                             \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST16_IS, _UMAX)
    #endif

    #if RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST32_IS, _DESCRIBED)
        #define RE_STD_INTERNAL_STDINT_FAST32_S                                \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST32_IS, _S)
        #define RE_STD_INTERNAL_STDINT_FAST32_U                                \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST32_IS, _U)
        #define RE_STD_INTERNAL_STDINT_FAST32_SMAX                             \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST32_IS, _SMAX)
        #define RE_STD_INTERNAL_STDINT_FAST32_UMAX                             \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST32_IS, _UMAX)
    #endif

    #if RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST64_IS, _DESCRIBED)
        #define RE_STD_INTERNAL_STDINT_FAST64_S                                \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST64_IS, _S)
        #define RE_STD_INTERNAL_STDINT_FAST64_U                                \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST64_IS, _U)
        #define RE_STD_INTERNAL_STDINT_FAST64_SMAX                             \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST64_IS, _SMAX)
        #define RE_STD_INTERNAL_STDINT_FAST64_UMAX                             \
            RE_STD_INTERNAL_STDINT_OF(RE_STD_INTERNAL_STDINT_FAST64_IS, _UMAX)
    #endif
#endif  // RE_STD_INTERNAL_STDINT_FAST_FROM


// 2.5    Family availability
//------------------------------------------------------------------------------
// 2.5.1
// RE_STD_INTERNAL_STDINT_*_OK
//   constant (internal): 1 when a family is described and both of its types
// can be spelled in this build, 0 otherwise. The types, limits and constants
// this header defines for a family depend on it, and the formats on the
// family being there at all (6.1.1), so a family is present or absent as a
// whole.
#if ( (defined(RE_STD_INTERNAL_STDINT_8_S))                          &&        \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_8_SMAX))      &&   \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_8_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_8_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_8_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_16_S))                         &&        \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_16_SMAX))     &&   \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_16_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_16_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_16_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_32_S))                         &&        \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_32_SMAX))     &&   \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_32_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_32_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_32_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_64_S))                         &&        \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_64_SMAX))     &&   \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_64_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_64_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_64_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_LEAST8_S))                     &&        \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_LEAST8_SMAX)) &&   \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_LEAST8_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_LEAST8_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_LEAST8_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_LEAST16_S))                     &&       \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_LEAST16_SMAX)) &&  \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_LEAST16_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_LEAST16_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_LEAST16_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_LEAST32_S))                     &&       \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_LEAST32_SMAX)) &&  \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_LEAST32_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_LEAST32_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_LEAST32_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_LEAST64_S))                     &&       \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_LEAST64_SMAX)) &&  \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_LEAST64_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_LEAST64_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_LEAST64_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_PTR_S))                        &&        \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_PTR_SMAX))    &&   \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_PTR_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_PTR_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_PTR_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_MAX_S))                        &&        \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_MAX_SMAX))    &&   \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_MAX_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_MAX_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_MAX_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_FAST8_S))                     &&         \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_FAST8_SMAX)) &&    \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_FAST8_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_FAST8_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_FAST8_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_FAST16_S))                     &&        \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_FAST16_SMAX)) &&   \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_FAST16_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_FAST16_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_FAST16_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_FAST32_S))                     &&        \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_FAST32_SMAX)) &&   \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_FAST32_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_FAST32_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_FAST32_OK 0
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_FAST64_S))                     &&        \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_FAST64_SMAX)) &&   \
      (RE_STD_INTERNAL_STDINT_USABLE(RE_STD_INTERNAL_STDINT_FAST64_UMAX)) )
    #define RE_STD_INTERNAL_STDINT_FAST64_OK 1
#else
    #define RE_STD_INTERNAL_STDINT_FAST64_OK 0
#endif


//==============================================================================
// 3.  TYPES
//==============================================================================
//   Only on the self-defined path; the platform's headers declared them
// otherwise. Where `long long` is an extension (C++98), GCC and Clang would
// flag each typedef that names it under -pedantic, although the build asked
// for the extension, so those typedefs are declared with -Wlong-long off.


#if (RE_STD_INTERNAL_STDINT_USE_OWN == 1)

#if ( (RE_STD_HAS_LONG_LONG == 1)                                   &&         \
      (RE_STD_LANG_IS_CPP == 1)                                  &&         \
      (!RE_STD_LANG_IS_CPP11_OR_HIGHER)                             &&         \
      ( (defined(RE_STD_COMPILER_CLANG)) ||                                    \
        ( (defined(RE_STD_COMPILER_GCC)) &&                                    \
          (RE_STD_COMPILER_VERSION_AT_LEAST(4, 6, 0)) ) ) )
    #define RE_STD_INTERNAL_STDINT_QUIET_LONG_LONG 1
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wlong-long"
#else
    #define RE_STD_INTERNAL_STDINT_QUIET_LONG_LONG 0
#endif


// 3.1    Integer types
//------------------------------------------------------------------------------
// 3.1.1
// Exact-width integer types
//   type: int8_t, int16_t, int32_t, int64_t and their unsigned counterparts;
// exactly N bits, no padding, two's complement.
#if (RE_STD_INTERNAL_STDINT_8_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_8_S         int8_t;
    typedef RE_STD_INTERNAL_STDINT_8_U         uint8_t;
#endif

#if (RE_STD_INTERNAL_STDINT_16_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_16_S        int16_t;
    typedef RE_STD_INTERNAL_STDINT_16_U        uint16_t;
#endif

#if (RE_STD_INTERNAL_STDINT_32_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_32_S        int32_t;
    typedef RE_STD_INTERNAL_STDINT_32_U        uint32_t;
#endif

#if (RE_STD_INTERNAL_STDINT_64_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_64_S        int64_t;
    typedef RE_STD_INTERNAL_STDINT_64_U        uint64_t;
#endif

// 3.1.2
// Minimum-width integer types
//   type: int_least8_t to int_least64_t and their unsigned counterparts; the
// narrowest types of at least N bits.
#if (RE_STD_INTERNAL_STDINT_LEAST8_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_LEAST8_S    int_least8_t;
    typedef RE_STD_INTERNAL_STDINT_LEAST8_U    uint_least8_t;
#endif

#if (RE_STD_INTERNAL_STDINT_LEAST16_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_LEAST16_S   int_least16_t;
    typedef RE_STD_INTERNAL_STDINT_LEAST16_U   uint_least16_t;
#endif

#if (RE_STD_INTERNAL_STDINT_LEAST32_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_LEAST32_S   int_least32_t;
    typedef RE_STD_INTERNAL_STDINT_LEAST32_U   uint_least32_t;
#endif

#if (RE_STD_INTERNAL_STDINT_LEAST64_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_LEAST64_S   int_least64_t;
    typedef RE_STD_INTERNAL_STDINT_LEAST64_U   uint_least64_t;
#endif

// 3.1.3
// Integer types capable of holding object pointers
//   type: intptr_t and uintptr_t; a `void*` converted to either and back
// compares equal to the original.
#if (RE_STD_INTERNAL_STDINT_PTR_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_PTR_S       intptr_t;
    typedef RE_STD_INTERNAL_STDINT_PTR_U       uintptr_t;
#endif

// 3.1.4
// Greatest-width integer types
//   type: intmax_t and uintmax_t; the platform's widest integer types.
#if (RE_STD_INTERNAL_STDINT_MAX_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_MAX_S       intmax_t;
    typedef RE_STD_INTERNAL_STDINT_MAX_U       uintmax_t;
#endif

// 3.1.5
// Fastest minimum-width integer types
//   type: int_fast8_t to int_fast64_t and their unsigned counterparts; the
// types of at least N bits that the platform's <stdint.h> calls its fastest,
// declared only where it is known which those are (2.4).
#if (RE_STD_INTERNAL_STDINT_FAST8_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_FAST8_S     int_fast8_t;
    typedef RE_STD_INTERNAL_STDINT_FAST8_U     uint_fast8_t;
#endif

#if (RE_STD_INTERNAL_STDINT_FAST16_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_FAST16_S    int_fast16_t;
    typedef RE_STD_INTERNAL_STDINT_FAST16_U    uint_fast16_t;
#endif

#if (RE_STD_INTERNAL_STDINT_FAST32_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_FAST32_S    int_fast32_t;
    typedef RE_STD_INTERNAL_STDINT_FAST32_U    uint_fast32_t;
#endif

#if (RE_STD_INTERNAL_STDINT_FAST64_OK == 1)
    typedef RE_STD_INTERNAL_STDINT_FAST64_S    int_fast64_t;
    typedef RE_STD_INTERNAL_STDINT_FAST64_U    uint_fast64_t;
#endif


#if (RE_STD_INTERNAL_STDINT_QUIET_LONG_LONG == 1)
    #pragma GCC diagnostic pop
#endif

#endif  // RE_STD_INTERNAL_STDINT_USE_OWN == 1


//==============================================================================
// 4.  LIMITS
//==============================================================================
//   Defined wherever a family exists and its macros do not: the whole
// self-defined path, and on the platform path the limits an older C++ library
// withheld because it was included, before this header, without the request
// that section 1.3 makes. Every value is the compiler's own literal, so each
// is usable in #if.


// 4.1    Limits of the integer types
//------------------------------------------------------------------------------
// 4.1.1
// Exact-width limits
//   constant: INTN_MIN, INTN_MAX and UINTN_MAX for N = 8, 16, 32, 64.
#if ( (RE_STD_INTERNAL_STDINT_8_OK == 1) &&                                    \
      (!defined(INT8_MAX)) )
    #define INT8_MAX           RE_STD_INTERNAL_STDINT_8_SMAX
    #define INT8_MIN           (-INT8_MAX - 1)
    #define UINT8_MAX          RE_STD_INTERNAL_STDINT_8_UMAX
#endif

#if ( (RE_STD_INTERNAL_STDINT_16_OK == 1) &&                                   \
      (!defined(INT16_MAX)) )
    #define INT16_MAX          RE_STD_INTERNAL_STDINT_16_SMAX
    #define INT16_MIN          (-INT16_MAX - 1)
    #define UINT16_MAX         RE_STD_INTERNAL_STDINT_16_UMAX
#endif

#if ( (RE_STD_INTERNAL_STDINT_32_OK == 1) &&                                   \
      (!defined(INT32_MAX)) )
    #define INT32_MAX          RE_STD_INTERNAL_STDINT_32_SMAX
    #define INT32_MIN          (-INT32_MAX - 1)
    #define UINT32_MAX         RE_STD_INTERNAL_STDINT_32_UMAX
#endif

#if ( (RE_STD_INTERNAL_STDINT_64_OK == 1) &&                                   \
      (!defined(INT64_MAX)) )
    #define INT64_MAX          RE_STD_INTERNAL_STDINT_64_SMAX
    #define INT64_MIN          (-INT64_MAX - 1)
    #define UINT64_MAX         RE_STD_INTERNAL_STDINT_64_UMAX
#endif

// 4.1.2
// Minimum-width limits
//   constant: INT_LEASTN_MIN, INT_LEASTN_MAX and UINT_LEASTN_MAX.
#if ( (RE_STD_INTERNAL_STDINT_LEAST8_OK == 1) &&                               \
      (!defined(INT_LEAST8_MAX)) )
    #define INT_LEAST8_MAX     RE_STD_INTERNAL_STDINT_LEAST8_SMAX
    #define INT_LEAST8_MIN     (-INT_LEAST8_MAX - 1)
    #define UINT_LEAST8_MAX    RE_STD_INTERNAL_STDINT_LEAST8_UMAX
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST16_OK == 1) &&                              \
      (!defined(INT_LEAST16_MAX)) )
    #define INT_LEAST16_MAX    RE_STD_INTERNAL_STDINT_LEAST16_SMAX
    #define INT_LEAST16_MIN    (-INT_LEAST16_MAX - 1)
    #define UINT_LEAST16_MAX   RE_STD_INTERNAL_STDINT_LEAST16_UMAX
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST32_OK == 1) &&                              \
      (!defined(INT_LEAST32_MAX)) )
    #define INT_LEAST32_MAX    RE_STD_INTERNAL_STDINT_LEAST32_SMAX
    #define INT_LEAST32_MIN    (-INT_LEAST32_MAX - 1)
    #define UINT_LEAST32_MAX   RE_STD_INTERNAL_STDINT_LEAST32_UMAX
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST64_OK == 1) &&                              \
      (!defined(INT_LEAST64_MAX)) )
    #define INT_LEAST64_MAX    RE_STD_INTERNAL_STDINT_LEAST64_SMAX
    #define INT_LEAST64_MIN    (-INT_LEAST64_MAX - 1)
    #define UINT_LEAST64_MAX   RE_STD_INTERNAL_STDINT_LEAST64_UMAX
#endif

// 4.1.3
// Pointer-holding and greatest-width limits
//   constant: INTPTR_MIN, INTPTR_MAX, UINTPTR_MAX, INTMAX_MIN, INTMAX_MAX and
// UINTMAX_MAX.
#if ( (RE_STD_INTERNAL_STDINT_PTR_OK == 1) &&                                  \
      (!defined(INTPTR_MAX)) )
    #define INTPTR_MAX         RE_STD_INTERNAL_STDINT_PTR_SMAX
    #define INTPTR_MIN         (-INTPTR_MAX - 1)
    #define UINTPTR_MAX        RE_STD_INTERNAL_STDINT_PTR_UMAX
#endif

#if ( (RE_STD_INTERNAL_STDINT_MAX_OK == 1) &&                                  \
      (!defined(INTMAX_MAX)) )
    #define INTMAX_MAX         RE_STD_INTERNAL_STDINT_MAX_SMAX
    #define INTMAX_MIN         (-INTMAX_MAX - 1)
    #define UINTMAX_MAX        RE_STD_INTERNAL_STDINT_MAX_UMAX
#endif

// 4.1.4
// Fastest minimum-width limits
//   constant: INT_FASTN_MIN, INT_FASTN_MAX and UINT_FASTN_MAX, where the
// family is described (2.4.3): on the own backend, and where a platform
// header that declared the types withheld their limits; without them the
// family's formats, which section 6 supplies on the same terms, would stand
// for a family that tests as absent.
#if ( (RE_STD_INTERNAL_STDINT_FAST8_OK == 1) &&                                \
      (!defined(INT_FAST8_MAX)) )
    #define INT_FAST8_MAX      RE_STD_INTERNAL_STDINT_FAST8_SMAX
    #define INT_FAST8_MIN      (-INT_FAST8_MAX - 1)
    #define UINT_FAST8_MAX     RE_STD_INTERNAL_STDINT_FAST8_UMAX
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST16_OK == 1) &&                               \
      (!defined(INT_FAST16_MAX)) )
    #define INT_FAST16_MAX     RE_STD_INTERNAL_STDINT_FAST16_SMAX
    #define INT_FAST16_MIN     (-INT_FAST16_MAX - 1)
    #define UINT_FAST16_MAX    RE_STD_INTERNAL_STDINT_FAST16_UMAX
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST32_OK == 1) &&                               \
      (!defined(INT_FAST32_MAX)) )
    #define INT_FAST32_MAX     RE_STD_INTERNAL_STDINT_FAST32_SMAX
    #define INT_FAST32_MIN     (-INT_FAST32_MAX - 1)
    #define UINT_FAST32_MAX    RE_STD_INTERNAL_STDINT_FAST32_UMAX
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST64_OK == 1) &&                               \
      (!defined(INT_FAST64_MAX)) )
    #define INT_FAST64_MAX     RE_STD_INTERNAL_STDINT_FAST64_SMAX
    #define INT_FAST64_MIN     (-INT_FAST64_MAX - 1)
    #define UINT_FAST64_MAX    RE_STD_INTERNAL_STDINT_FAST64_UMAX
#endif


// 4.2    Limits of other integer types
//------------------------------------------------------------------------------
//   From the compiler's predefined macros only; <limits.h> cannot tell which
// type size_t or ptrdiff_t is. ptrdiff_t, size_t and wchar_t are the
// compiler's own types, and it is to be believed about them. sig_atomic_t
// and wint_t are a C library's, declared in <signal.h> and <wchar.h>: there
// the compiler predefines what its own <stdint.h> assumes, which is the
// library's choice wherever it was held to the library's own headers
// (glibc, musl, Apple's, NetBSD, OpenBSD, DragonFly, mingw-w64, newlib,
// wasi-libc), with the exceptions written out below, both FreeBSD's.
// dstdint.c holds each of the two to the library's declaration wherever the
// limits are this header's, so a library not on that list cannot differ
// unnoticed.
#if (RE_STD_INTERNAL_STDINT_PREDEFINED == 1)

// 4.2.1
// PTRDIFF_MIN / PTRDIFF_MAX, RE_STD_INTERNAL_STDINT_OWN_PTRDIFF
//   constant: the range of ptrdiff_t, and (internal) 1 where this header
// defined it, 0 where the platform had or the build cannot spell the type.
#if ( (defined(__PTRDIFF_MAX__))                   &&                         \
      (RE_STD_INTERNAL_STDINT_USABLE(__PTRDIFF_MAX__))  &&                     \
      (!defined(PTRDIFF_MAX)) )
    #define RE_STD_INTERNAL_STDINT_OWN_PTRDIFF 1
    #define PTRDIFF_MAX        __PTRDIFF_MAX__
    #define PTRDIFF_MIN        (-PTRDIFF_MAX - 1)
#else
    #define RE_STD_INTERNAL_STDINT_OWN_PTRDIFF 0
#endif

// 4.2.2
// SIZE_MAX, RE_STD_INTERNAL_STDINT_OWN_SIZE
//   constant: the largest size_t, and (internal) 1 where this header defined
// it. A platform header may define it where this one will not: mingw-w64's
// <limits.h> does, and on 64-bit Windows an ISO strict C++98 build then has
// a SIZE_MAX it cannot read, a `long long` literal.
#if ( (defined(__SIZE_MAX__))                      &&                         \
      (RE_STD_INTERNAL_STDINT_USABLE(__SIZE_MAX__))     &&                     \
      (!defined(SIZE_MAX)) )
    #define RE_STD_INTERNAL_STDINT_OWN_SIZE 1
    #define SIZE_MAX           __SIZE_MAX__
#else
    #define RE_STD_INTERNAL_STDINT_OWN_SIZE 0
#endif

// 4.2.3
// SIG_ATOMIC_MIN / SIG_ATOMIC_MAX
//   constant: the range of sig_atomic_t. The compiler says `int` on every
// target; FreeBSD makes it `long` on amd64, arm64, riscv64 and 32-bit arm,
// and `int` on its other ports. GCC predefines the minimum; Clang does not,
// and its own <stdint.h> takes the type as signed, as this does.
//   RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MIN and _MAX (internal) are the range
// as this header knows it, and RE_STD_INTERNAL_STDINT_OWN_SIG_ATOMIC is 1
// where the public names are defined here, from them, and 0 where the
// platform had defined them. dstdint.c reads the internal names, which no
// platform header can define anew.
#if ( (defined(__FreeBSD__))       &&                                         \
      (defined(__LONG_MAX__))      &&                                         \
      ( (defined(__x86_64__))  ||                                             \
        (defined(__aarch64__)) ||                                             \
        (defined(__riscv))     ||                                             \
        (defined(__arm__)) ) )
    #define RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MAX  __LONG_MAX__
    #define RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MIN  (-__LONG_MAX__ - 1)
#elif defined(__SIG_ATOMIC_MAX__)
    #define RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MAX  __SIG_ATOMIC_MAX__
    #if defined(__SIG_ATOMIC_MIN__)
        #define RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MIN  __SIG_ATOMIC_MIN__
    #else
        #define RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MIN                         \
            (-__SIG_ATOMIC_MAX__ - 1)
    #endif
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MAX)) &&                     \
      (!defined(SIG_ATOMIC_MAX)) )
    #define RE_STD_INTERNAL_STDINT_OWN_SIG_ATOMIC 1
    #define SIG_ATOMIC_MAX     RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MAX
    #define SIG_ATOMIC_MIN     RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MIN
#else
    #define RE_STD_INTERNAL_STDINT_OWN_SIG_ATOMIC 0
#endif

// 4.2.4
// WCHAR_MIN / WCHAR_MAX, RE_STD_INTERNAL_STDINT_OWN_WCHAR
//   constant: the range of wchar_t, and (internal) 1 where this header
// defined it. The minimum of an unsigned wchar_t is written as a difference
// so that it has the maximum's type.
#if ( (defined(__WCHAR_MAX__)) &&                                             \
      (!defined(WCHAR_MAX)) )
    #define RE_STD_INTERNAL_STDINT_OWN_WCHAR 1
    #define WCHAR_MAX          __WCHAR_MAX__
    #if defined(__WCHAR_MIN__)
        #define WCHAR_MIN      __WCHAR_MIN__
    #elif defined(__WCHAR_UNSIGNED__)
        #define WCHAR_MIN      (WCHAR_MAX - WCHAR_MAX)
    #else
        #define WCHAR_MIN      (-WCHAR_MAX - 1)
    #endif
#else
    #define RE_STD_INTERNAL_STDINT_OWN_WCHAR 0
#endif

// 4.2.5
// WINT_MIN / WINT_MAX
//   constant: the range of wint_t; the minimum of an unsigned wint_t is
// written as WCHAR_MIN's. On FreeBSD wint_t is `int` on every architecture,
// whatever the compiler predefines (Clang says `unsigned int` for riscv64).
//   RE_STD_INTERNAL_STDINT_WINT_MIN and _MAX (internal) and
// RE_STD_INTERNAL_STDINT_OWN_WINT are as their SIG_ATOMIC counterparts.
#if ( (defined(__FreeBSD__)) &&                                               \
      (defined(__INT_MAX__)) )
    #define RE_STD_INTERNAL_STDINT_WINT_MAX  __INT_MAX__
    #define RE_STD_INTERNAL_STDINT_WINT_MIN  (-__INT_MAX__ - 1)
#elif defined(__WINT_MAX__)
    #define RE_STD_INTERNAL_STDINT_WINT_MAX  __WINT_MAX__
    #if defined(__WINT_MIN__)
        #define RE_STD_INTERNAL_STDINT_WINT_MIN  __WINT_MIN__
    #elif defined(__WINT_UNSIGNED__)
        #define RE_STD_INTERNAL_STDINT_WINT_MIN  (__WINT_MAX__ - __WINT_MAX__)
    #else
        #define RE_STD_INTERNAL_STDINT_WINT_MIN  (-__WINT_MAX__ - 1)
    #endif
#endif

#if ( (defined(RE_STD_INTERNAL_STDINT_WINT_MAX)) &&                           \
      (!defined(WINT_MAX)) )
    #define RE_STD_INTERNAL_STDINT_OWN_WINT 1
    #define WINT_MAX           RE_STD_INTERNAL_STDINT_WINT_MAX
    #define WINT_MIN           RE_STD_INTERNAL_STDINT_WINT_MIN
#else
    #define RE_STD_INTERNAL_STDINT_OWN_WINT 0
#endif

#else
    #define RE_STD_INTERNAL_STDINT_OWN_PTRDIFF    0
    #define RE_STD_INTERNAL_STDINT_OWN_SIZE       0
    #define RE_STD_INTERNAL_STDINT_OWN_SIG_ATOMIC 0
    #define RE_STD_INTERNAL_STDINT_OWN_WCHAR      0
    #define RE_STD_INTERNAL_STDINT_OWN_WINT       0
#endif  // RE_STD_INTERNAL_STDINT_PREDEFINED == 1


//==============================================================================
// 5.  INTEGER CONSTANT MACROS
//==============================================================================
//   Defined, like the limits, wherever a family exists and its macros do not.
// Each takes its suffix from the family's description rather than from the
// public limit macro, which a platform header may spell another way.


// 5.1    Constants
//------------------------------------------------------------------------------
// 5.1.1
// RE_STD_INTERNAL_STDINT_C
//   macro (internal): appends to the constant `_c` the suffix its type needs,
// read from the spelling class of the type's maximum `_max`.
#define RE_STD_INTERNAL_STDINT_CAT2_(_a, _b)  _a##_b
#define RE_STD_INTERNAL_STDINT_CAT2(_a, _b)                                    \
    RE_STD_INTERNAL_STDINT_CAT2_(_a, _b)
#define RE_STD_INTERNAL_STDINT_C(_c, _max)                                     \
    RE_STD_INTERNAL_STDINT_CAT2(RE_STD_INTERNAL_STDINT_SUFFIX_,                \
                           RE_STD_INTERNAL_STDINT_CLASS(_max))(_c)
#define RE_STD_INTERNAL_STDINT_SUFFIX_1(_c)  _c
#define RE_STD_INTERNAL_STDINT_SUFFIX_2(_c)  _c##U
#define RE_STD_INTERNAL_STDINT_SUFFIX_3(_c)  _c##L
#define RE_STD_INTERNAL_STDINT_SUFFIX_4(_c)  _c##UL
#define RE_STD_INTERNAL_STDINT_SUFFIX_5(_c)  _c##LL
#define RE_STD_INTERNAL_STDINT_SUFFIX_6(_c)  _c##ULL

// 5.1.2
// Minimum-width constants
//   macro: INTN_C(c) and UINTN_C(c), the constant `c` with the type of
// int_leastN_t / uint_leastN_t after promotion; usable in #if.
#if ( (RE_STD_INTERNAL_STDINT_LEAST8_OK == 1) &&                               \
      (!defined(INT8_C)) )
    #define INT8_C(_c)                                                        \
        RE_STD_INTERNAL_STDINT_C(_c, RE_STD_INTERNAL_STDINT_LEAST8_SMAX)
    #define UINT8_C(_c)                                                       \
        RE_STD_INTERNAL_STDINT_C(_c, RE_STD_INTERNAL_STDINT_LEAST8_UMAX)
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST16_OK == 1) &&                              \
      (!defined(INT16_C)) )
    #define INT16_C(_c)                                                       \
        RE_STD_INTERNAL_STDINT_C(_c, RE_STD_INTERNAL_STDINT_LEAST16_SMAX)
    #define UINT16_C(_c)                                                      \
        RE_STD_INTERNAL_STDINT_C(_c, RE_STD_INTERNAL_STDINT_LEAST16_UMAX)
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST32_OK == 1) &&                              \
      (!defined(INT32_C)) )
    #define INT32_C(_c)                                                       \
        RE_STD_INTERNAL_STDINT_C(_c, RE_STD_INTERNAL_STDINT_LEAST32_SMAX)
    #define UINT32_C(_c)                                                      \
        RE_STD_INTERNAL_STDINT_C(_c, RE_STD_INTERNAL_STDINT_LEAST32_UMAX)
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST64_OK == 1) &&                              \
      (!defined(INT64_C)) )
    #define INT64_C(_c)                                                       \
        RE_STD_INTERNAL_STDINT_C(_c, RE_STD_INTERNAL_STDINT_LEAST64_SMAX)
    #define UINT64_C(_c)                                                      \
        RE_STD_INTERNAL_STDINT_C(_c, RE_STD_INTERNAL_STDINT_LEAST64_UMAX)
#endif

// 5.1.3
// Greatest-width constants
//   macro: INTMAX_C(c) and UINTMAX_C(c), the constant `c` with the type of
// intmax_t / uintmax_t.
#if ( (RE_STD_INTERNAL_STDINT_MAX_OK == 1) &&                                  \
      (!defined(INTMAX_C)) )
    #define INTMAX_C(_c)                                                      \
        RE_STD_INTERNAL_STDINT_C(_c, RE_STD_INTERNAL_STDINT_MAX_SMAX)
    #define UINTMAX_C(_c)                                                     \
        RE_STD_INTERNAL_STDINT_C(_c, RE_STD_INTERNAL_STDINT_MAX_UMAX)
#endif


//==============================================================================
// 6.  FORMAT MACROS
//==============================================================================
//   The fprintf and fscanf conversion specifiers of <inttypes.h>, defined
// wherever a family exists and its macros do not: the self-defined path, a
// freestanding build, which has <stdint.h> alone, and an older C++ library
// that withheld them. Each expands to string literals that concatenate into
// one. The fastest minimum-width families (FASTN) get formats here only
// where they are described (2.4) and the platform gave them none: with its
// <inttypes.h> they have the platform's, and where they are not described
// they do not exist.


// 6.1    Formatted families
//------------------------------------------------------------------------------
// 6.1.1
// RE_STD_INTERNAL_STDINT_*_FMT
//   constant (internal): 1 when a family is to have formats: it exists, its
// limit macro being defined, and it is described (section 2). That is every
// family this header declared, and one more kind: a family this build
// cannot spell, which the platform's <stdint.h> declared before this header
// was included. Under ISO strict C++98 that is int64_t on a 32-bit target,
// once any header has brought <stdint.h> with it. A format is a string, and
// needs no spelling of the type; with it the family is whole, as every
// other is, and `#ifdef INT64_MAX` remains a sufficient test.
#if ( (defined(INT8_MAX))                     &&                              \
      (defined(RE_STD_INTERNAL_STDINT_8_S)) )
    #define RE_STD_INTERNAL_STDINT_8_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_8_FMT 0
#endif

#if ( (defined(INT16_MAX))                    &&                              \
      (defined(RE_STD_INTERNAL_STDINT_16_S)) )
    #define RE_STD_INTERNAL_STDINT_16_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_16_FMT 0
#endif

#if ( (defined(INT32_MAX))                    &&                              \
      (defined(RE_STD_INTERNAL_STDINT_32_S)) )
    #define RE_STD_INTERNAL_STDINT_32_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_32_FMT 0
#endif

#if ( (defined(INT64_MAX))                    &&                              \
      (defined(RE_STD_INTERNAL_STDINT_64_S)) )
    #define RE_STD_INTERNAL_STDINT_64_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_64_FMT 0
#endif

#if ( (defined(INT_LEAST8_MAX))               &&                              \
      (defined(RE_STD_INTERNAL_STDINT_LEAST8_S)) )
    #define RE_STD_INTERNAL_STDINT_LEAST8_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_LEAST8_FMT 0
#endif

#if ( (defined(INT_LEAST16_MAX))              &&                              \
      (defined(RE_STD_INTERNAL_STDINT_LEAST16_S)) )
    #define RE_STD_INTERNAL_STDINT_LEAST16_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_LEAST16_FMT 0
#endif

#if ( (defined(INT_LEAST32_MAX))              &&                              \
      (defined(RE_STD_INTERNAL_STDINT_LEAST32_S)) )
    #define RE_STD_INTERNAL_STDINT_LEAST32_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_LEAST32_FMT 0
#endif

#if ( (defined(INT_LEAST64_MAX))              &&                              \
      (defined(RE_STD_INTERNAL_STDINT_LEAST64_S)) )
    #define RE_STD_INTERNAL_STDINT_LEAST64_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_LEAST64_FMT 0
#endif

#if ( (defined(INTPTR_MAX))                   &&                              \
      (defined(RE_STD_INTERNAL_STDINT_PTR_S)) )
    #define RE_STD_INTERNAL_STDINT_PTR_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_PTR_FMT 0
#endif

#if ( (defined(INTMAX_MAX))                   &&                              \
      (defined(RE_STD_INTERNAL_STDINT_MAX_S)) )
    #define RE_STD_INTERNAL_STDINT_MAX_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_MAX_FMT 0
#endif

#if ( (defined(INT_FAST8_MAX))                &&                              \
      (defined(RE_STD_INTERNAL_STDINT_FAST8_S)) )
    #define RE_STD_INTERNAL_STDINT_FAST8_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_FAST8_FMT 0
#endif

#if ( (defined(INT_FAST16_MAX))               &&                              \
      (defined(RE_STD_INTERNAL_STDINT_FAST16_S)) )
    #define RE_STD_INTERNAL_STDINT_FAST16_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_FAST16_FMT 0
#endif

#if ( (defined(INT_FAST32_MAX))               &&                              \
      (defined(RE_STD_INTERNAL_STDINT_FAST32_S)) )
    #define RE_STD_INTERNAL_STDINT_FAST32_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_FAST32_FMT 0
#endif

#if ( (defined(INT_FAST64_MAX))               &&                              \
      (defined(RE_STD_INTERNAL_STDINT_FAST64_S)) )
    #define RE_STD_INTERNAL_STDINT_FAST64_FMT 1
#else
    #define RE_STD_INTERNAL_STDINT_FAST64_FMT 0
#endif


// 6.2    Length modifiers
//------------------------------------------------------------------------------
// 6.2.1
// RE_STD_INTERNAL_STDINT_MS_STDIO
//   constant (internal): 1 where printf and scanf are those of Microsoft's
// msvcrt.dll, which MinGW calls unless it is built for the UCRT or the build
// takes MinGW's own C99 ones (__USE_MINGW_ANSI_STDIO, which mingw-w64 turns
// on by default from C99 and C++11). Those take `I64` where C says `ll`, and
// have no `hh`: their scanf reads it as `h`, and writes two bytes through a
// pointer to one. mingw-w64's <inttypes.h> makes this very test, defines its
// 64-bit formats with `I64` and leaves the 8-bit scanf formats out, so the
// header's own formats are the platform's there too. A build that has
// included no header of that runtime, a freestanding one, has C's formats.
#if ( (defined(__MINGW32__))            &&                                     \
      (!defined(_UCRT))                 &&                                     \
      (defined(__USE_MINGW_ANSI_STDIO)) )
    #if (__USE_MINGW_ANSI_STDIO == 0)
        #define RE_STD_INTERNAL_STDINT_MS_STDIO 1
    #else
        #define RE_STD_INTERNAL_STDINT_MS_STDIO 0
    #endif
#else
    #define RE_STD_INTERNAL_STDINT_MS_STDIO 0
#endif

// 6.2.2
// RE_STD_INTERNAL_STDINT_LEN
//   macro (internal): the printf length modifier for a type whose maximum is
// `_max`: none for `int` and the types that promote to it, `l` for `long`,
// and for `long long` `ll`, or `I64` where the runtime is Microsoft's
// (6.2.1).
#define RE_STD_INTERNAL_STDINT_LEN(_max)                                       \
    RE_STD_INTERNAL_STDINT_CAT2(RE_STD_INTERNAL_STDINT_LEN_CLASS_,             \
                           RE_STD_INTERNAL_STDINT_CLASS(_max))
#define RE_STD_INTERNAL_STDINT_LEN_CLASS_1  ""
#define RE_STD_INTERNAL_STDINT_LEN_CLASS_2  ""
#define RE_STD_INTERNAL_STDINT_LEN_CLASS_3  "l"
#define RE_STD_INTERNAL_STDINT_LEN_CLASS_4  "l"

#if (RE_STD_INTERNAL_STDINT_MS_STDIO == 1)
    #define RE_STD_INTERNAL_STDINT_LEN_CLASS_5  "I64"
    #define RE_STD_INTERNAL_STDINT_LEN_CLASS_6  "I64"
#else
    #define RE_STD_INTERNAL_STDINT_LEN_CLASS_5  "ll"
    #define RE_STD_INTERNAL_STDINT_LEN_CLASS_6  "ll"
#endif

// 6.2.3
// RE_STD_INTERNAL_STDINT_PRILEN_*
//   constant (internal): the printf length modifier of each family. A family's
// signed and unsigned types share a rank, so its signed maximum decides for
// both.
#define RE_STD_INTERNAL_STDINT_PRILEN_8                                        \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_8_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_16                                       \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_16_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_32                                       \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_32_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_64                                       \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_64_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_LEAST8                                   \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_LEAST8_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_LEAST16                                  \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_LEAST16_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_LEAST32                                  \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_LEAST32_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_LEAST64                                  \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_LEAST64_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_PTR                                      \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_PTR_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_MAX                                      \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_MAX_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_FAST8                                    \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_FAST8_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_FAST16                                   \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_FAST16_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_FAST32                                   \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_FAST32_SMAX)
#define RE_STD_INTERNAL_STDINT_PRILEN_FAST64                                   \
    RE_STD_INTERNAL_STDINT_LEN(RE_STD_INTERNAL_STDINT_FAST64_SMAX)

// 6.2.4
// RE_STD_INTERNAL_STDINT_HAS_HH
//   constant (internal): 1 when scanf has the `hh` length modifier, which
// the scanf formats of the 8-bit families need: the language has it from
// C99 and C++11, and the runtime must as well, which Microsoft's msvcrt.dll
// does not (6.2.1).
#if (RE_STD_INTERNAL_STDINT_MS_STDIO == 1)
    #define RE_STD_INTERNAL_STDINT_HAS_HH 0
#elif (RE_STD_LANG_IS_CPP == 1)
    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        #define RE_STD_INTERNAL_STDINT_HAS_HH 1
    #else
        #define RE_STD_INTERNAL_STDINT_HAS_HH 0
    #endif
#elif RE_STD_LANG_IS_C99_OR_HIGHER
    #define RE_STD_INTERNAL_STDINT_HAS_HH 1
#else
    #define RE_STD_INTERNAL_STDINT_HAS_HH 0
#endif

// 6.2.5
// RE_STD_INTERNAL_STDINT_IS_SHORT
//   macro (internal): 1 when the type `_type` is `short`, spelled `short` or
// `short int` as the compiler or section 2.3 spells it, and 0 when it is
// `int`. The first token is pasted onto a prefix that expands to the answer
// and the opening of a conditional, `1 ? 1 :` or `0 ? 0 :`, so whatever
// follows -- nothing, or the `int` of `short int` -- lands in the arm that is
// never evaluated, where #if takes an identifier without giving it a value
// and without -Wundef's objection. A spelling that begins otherwise
// (`signed short`, which no compiler emits) does not preprocess: a loud
// failure, not a wrong answer. #if only.
#define RE_STD_INTERNAL_STDINT_IS_SHORT(_type)                                 \
    (RE_STD_INTERNAL_STDINT_CAT(RE_STD_INTERNAL_STDINT_SHORT_, _type) + 0)
#define RE_STD_INTERNAL_STDINT_SHORT_short  1 ? 1 :
#define RE_STD_INTERNAL_STDINT_SHORT_int    0 ? 0 :

// 6.2.6
// RE_STD_INTERNAL_STDINT_SCNLEN_*
//   constant (internal): the scanf length modifier of each family, which,
// unlike printf's, must name the type exactly. From `int` up it is printf's.
// Below, a maximum without a suffix tells the type: 127 is a character
// type's, which takes `hh`, and a maximum under `int`'s is `short`'s, which
// takes `h`; where `short` and `int` have one width, as on 16-bit targets,
// only the type's spelling can tell them apart. The families wider than 16
// bits are never narrower than `int`. No maximum that may be `long long` is
// ever evaluated.
#if (RE_STD_INTERNAL_STDINT_8_FMT == 1)
    #if (RE_STD_INTERNAL_STDINT_CLASS(RE_STD_INTERNAL_STDINT_8_SMAX) != 1)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_8                                \
            RE_STD_INTERNAL_STDINT_PRILEN_8
    #elif (RE_STD_INTERNAL_STDINT_8_SMAX == 127)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_8        "hh"
    #elif (RE_STD_INTERNAL_STDINT_8_SMAX < RE_STD_INTERNAL_STDINT_INT_MAX)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_8        "h"
    #elif RE_STD_INTERNAL_STDINT_IS_SHORT(RE_STD_INTERNAL_STDINT_8_S)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_8        "h"
    #else
        #define RE_STD_INTERNAL_STDINT_SCNLEN_8        ""
    #endif
#endif  // RE_STD_INTERNAL_STDINT_8_FMT == 1

#if (RE_STD_INTERNAL_STDINT_LEAST8_FMT == 1)
    #if (RE_STD_INTERNAL_STDINT_CLASS(RE_STD_INTERNAL_STDINT_LEAST8_SMAX) != 1)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST8                           \
            RE_STD_INTERNAL_STDINT_PRILEN_LEAST8
    #elif (RE_STD_INTERNAL_STDINT_LEAST8_SMAX == 127)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST8   "hh"
    #elif (RE_STD_INTERNAL_STDINT_LEAST8_SMAX < RE_STD_INTERNAL_STDINT_INT_MAX)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST8   "h"
    #elif RE_STD_INTERNAL_STDINT_IS_SHORT(RE_STD_INTERNAL_STDINT_LEAST8_S)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST8   "h"
    #else
        #define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST8   ""
    #endif
#endif  // RE_STD_INTERNAL_STDINT_LEAST8_FMT == 1

#if (RE_STD_INTERNAL_STDINT_FAST8_FMT == 1)
    #if (RE_STD_INTERNAL_STDINT_CLASS(RE_STD_INTERNAL_STDINT_FAST8_SMAX) != 1)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_FAST8                            \
            RE_STD_INTERNAL_STDINT_PRILEN_FAST8
    #elif (RE_STD_INTERNAL_STDINT_FAST8_SMAX == 127)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_FAST8    "hh"
    #elif (RE_STD_INTERNAL_STDINT_FAST8_SMAX < RE_STD_INTERNAL_STDINT_INT_MAX)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_FAST8    "h"
    #elif RE_STD_INTERNAL_STDINT_IS_SHORT(RE_STD_INTERNAL_STDINT_FAST8_S)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_FAST8    "h"
    #else
        #define RE_STD_INTERNAL_STDINT_SCNLEN_FAST8    ""
    #endif
#endif  // RE_STD_INTERNAL_STDINT_FAST8_FMT == 1

#if (RE_STD_INTERNAL_STDINT_16_FMT == 1)
    #if (RE_STD_INTERNAL_STDINT_CLASS(RE_STD_INTERNAL_STDINT_16_SMAX) != 1)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_16                               \
            RE_STD_INTERNAL_STDINT_PRILEN_16
    #elif (RE_STD_INTERNAL_STDINT_16_SMAX == 127)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_16       "hh"
    #elif (RE_STD_INTERNAL_STDINT_16_SMAX < RE_STD_INTERNAL_STDINT_INT_MAX)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_16       "h"
    #elif RE_STD_INTERNAL_STDINT_IS_SHORT(RE_STD_INTERNAL_STDINT_16_S)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_16       "h"
    #else
        #define RE_STD_INTERNAL_STDINT_SCNLEN_16       ""
    #endif
#endif  // RE_STD_INTERNAL_STDINT_16_FMT == 1

#if (RE_STD_INTERNAL_STDINT_LEAST16_FMT == 1)
    #if (RE_STD_INTERNAL_STDINT_CLASS(RE_STD_INTERNAL_STDINT_LEAST16_SMAX) != 1)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST16                          \
            RE_STD_INTERNAL_STDINT_PRILEN_LEAST16
    #elif (RE_STD_INTERNAL_STDINT_LEAST16_SMAX == 127)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST16  "hh"
    #elif (RE_STD_INTERNAL_STDINT_LEAST16_SMAX < RE_STD_INTERNAL_STDINT_INT_MAX)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST16  "h"
    #elif RE_STD_INTERNAL_STDINT_IS_SHORT(RE_STD_INTERNAL_STDINT_LEAST16_S)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST16  "h"
    #else
        #define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST16  ""
    #endif
#endif  // RE_STD_INTERNAL_STDINT_LEAST16_FMT == 1

#if (RE_STD_INTERNAL_STDINT_FAST16_FMT == 1)
    #if (RE_STD_INTERNAL_STDINT_CLASS(RE_STD_INTERNAL_STDINT_FAST16_SMAX) != 1)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_FAST16                           \
            RE_STD_INTERNAL_STDINT_PRILEN_FAST16
    #elif (RE_STD_INTERNAL_STDINT_FAST16_SMAX == 127)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_FAST16   "hh"
    #elif (RE_STD_INTERNAL_STDINT_FAST16_SMAX < RE_STD_INTERNAL_STDINT_INT_MAX)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_FAST16   "h"
    #elif RE_STD_INTERNAL_STDINT_IS_SHORT(RE_STD_INTERNAL_STDINT_FAST16_S)
        #define RE_STD_INTERNAL_STDINT_SCNLEN_FAST16   "h"
    #else
        #define RE_STD_INTERNAL_STDINT_SCNLEN_FAST16   ""
    #endif
#endif  // RE_STD_INTERNAL_STDINT_FAST16_FMT == 1

#define RE_STD_INTERNAL_STDINT_SCNLEN_32       RE_STD_INTERNAL_STDINT_PRILEN_32
#define RE_STD_INTERNAL_STDINT_SCNLEN_64       RE_STD_INTERNAL_STDINT_PRILEN_64
#define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST32                                  \
    RE_STD_INTERNAL_STDINT_PRILEN_LEAST32
#define RE_STD_INTERNAL_STDINT_SCNLEN_LEAST64                                  \
    RE_STD_INTERNAL_STDINT_PRILEN_LEAST64
#define RE_STD_INTERNAL_STDINT_SCNLEN_PTR      RE_STD_INTERNAL_STDINT_PRILEN_PTR
#define RE_STD_INTERNAL_STDINT_SCNLEN_MAX      RE_STD_INTERNAL_STDINT_PRILEN_MAX
#define RE_STD_INTERNAL_STDINT_SCNLEN_FAST32                                   \
    RE_STD_INTERNAL_STDINT_PRILEN_FAST32
#define RE_STD_INTERNAL_STDINT_SCNLEN_FAST64                                   \
    RE_STD_INTERNAL_STDINT_PRILEN_FAST64


// 6.3    printf formats
//------------------------------------------------------------------------------
// 6.3.1
// PRI formats
//   constant: PRIdN, PRIiN, PRIoN, PRIuN, PRIxN and PRIXN for the exact-width,
// minimum-width (LEASTN), pointer-holding (PTR), greatest-width (MAX) and,
// where they are described, fastest minimum-width (FASTN) families.
#if ( (RE_STD_INTERNAL_STDINT_8_FMT == 1) &&                                   \
      (!defined(PRId8)) )
    #define PRId8        RE_STD_INTERNAL_STDINT_PRILEN_8 "d"
    #define PRIi8        RE_STD_INTERNAL_STDINT_PRILEN_8 "i"
    #define PRIo8        RE_STD_INTERNAL_STDINT_PRILEN_8 "o"
    #define PRIu8        RE_STD_INTERNAL_STDINT_PRILEN_8 "u"
    #define PRIx8        RE_STD_INTERNAL_STDINT_PRILEN_8 "x"
    #define PRIX8        RE_STD_INTERNAL_STDINT_PRILEN_8 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_16_FMT == 1) &&                                  \
      (!defined(PRId16)) )
    #define PRId16       RE_STD_INTERNAL_STDINT_PRILEN_16 "d"
    #define PRIi16       RE_STD_INTERNAL_STDINT_PRILEN_16 "i"
    #define PRIo16       RE_STD_INTERNAL_STDINT_PRILEN_16 "o"
    #define PRIu16       RE_STD_INTERNAL_STDINT_PRILEN_16 "u"
    #define PRIx16       RE_STD_INTERNAL_STDINT_PRILEN_16 "x"
    #define PRIX16       RE_STD_INTERNAL_STDINT_PRILEN_16 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_32_FMT == 1) &&                                  \
      (!defined(PRId32)) )
    #define PRId32       RE_STD_INTERNAL_STDINT_PRILEN_32 "d"
    #define PRIi32       RE_STD_INTERNAL_STDINT_PRILEN_32 "i"
    #define PRIo32       RE_STD_INTERNAL_STDINT_PRILEN_32 "o"
    #define PRIu32       RE_STD_INTERNAL_STDINT_PRILEN_32 "u"
    #define PRIx32       RE_STD_INTERNAL_STDINT_PRILEN_32 "x"
    #define PRIX32       RE_STD_INTERNAL_STDINT_PRILEN_32 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_64_FMT == 1) &&                                  \
      (!defined(PRId64)) )
    #define PRId64       RE_STD_INTERNAL_STDINT_PRILEN_64 "d"
    #define PRIi64       RE_STD_INTERNAL_STDINT_PRILEN_64 "i"
    #define PRIo64       RE_STD_INTERNAL_STDINT_PRILEN_64 "o"
    #define PRIu64       RE_STD_INTERNAL_STDINT_PRILEN_64 "u"
    #define PRIx64       RE_STD_INTERNAL_STDINT_PRILEN_64 "x"
    #define PRIX64       RE_STD_INTERNAL_STDINT_PRILEN_64 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST8_FMT == 1) &&                              \
      (!defined(PRIdLEAST8)) )
    #define PRIdLEAST8   RE_STD_INTERNAL_STDINT_PRILEN_LEAST8 "d"
    #define PRIiLEAST8   RE_STD_INTERNAL_STDINT_PRILEN_LEAST8 "i"
    #define PRIoLEAST8   RE_STD_INTERNAL_STDINT_PRILEN_LEAST8 "o"
    #define PRIuLEAST8   RE_STD_INTERNAL_STDINT_PRILEN_LEAST8 "u"
    #define PRIxLEAST8   RE_STD_INTERNAL_STDINT_PRILEN_LEAST8 "x"
    #define PRIXLEAST8   RE_STD_INTERNAL_STDINT_PRILEN_LEAST8 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST16_FMT == 1) &&                             \
      (!defined(PRIdLEAST16)) )
    #define PRIdLEAST16  RE_STD_INTERNAL_STDINT_PRILEN_LEAST16 "d"
    #define PRIiLEAST16  RE_STD_INTERNAL_STDINT_PRILEN_LEAST16 "i"
    #define PRIoLEAST16  RE_STD_INTERNAL_STDINT_PRILEN_LEAST16 "o"
    #define PRIuLEAST16  RE_STD_INTERNAL_STDINT_PRILEN_LEAST16 "u"
    #define PRIxLEAST16  RE_STD_INTERNAL_STDINT_PRILEN_LEAST16 "x"
    #define PRIXLEAST16  RE_STD_INTERNAL_STDINT_PRILEN_LEAST16 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST32_FMT == 1) &&                             \
      (!defined(PRIdLEAST32)) )
    #define PRIdLEAST32  RE_STD_INTERNAL_STDINT_PRILEN_LEAST32 "d"
    #define PRIiLEAST32  RE_STD_INTERNAL_STDINT_PRILEN_LEAST32 "i"
    #define PRIoLEAST32  RE_STD_INTERNAL_STDINT_PRILEN_LEAST32 "o"
    #define PRIuLEAST32  RE_STD_INTERNAL_STDINT_PRILEN_LEAST32 "u"
    #define PRIxLEAST32  RE_STD_INTERNAL_STDINT_PRILEN_LEAST32 "x"
    #define PRIXLEAST32  RE_STD_INTERNAL_STDINT_PRILEN_LEAST32 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST64_FMT == 1) &&                             \
      (!defined(PRIdLEAST64)) )
    #define PRIdLEAST64  RE_STD_INTERNAL_STDINT_PRILEN_LEAST64 "d"
    #define PRIiLEAST64  RE_STD_INTERNAL_STDINT_PRILEN_LEAST64 "i"
    #define PRIoLEAST64  RE_STD_INTERNAL_STDINT_PRILEN_LEAST64 "o"
    #define PRIuLEAST64  RE_STD_INTERNAL_STDINT_PRILEN_LEAST64 "u"
    #define PRIxLEAST64  RE_STD_INTERNAL_STDINT_PRILEN_LEAST64 "x"
    #define PRIXLEAST64  RE_STD_INTERNAL_STDINT_PRILEN_LEAST64 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_PTR_FMT == 1) &&                                 \
      (!defined(PRIdPTR)) )
    #define PRIdPTR      RE_STD_INTERNAL_STDINT_PRILEN_PTR "d"
    #define PRIiPTR      RE_STD_INTERNAL_STDINT_PRILEN_PTR "i"
    #define PRIoPTR      RE_STD_INTERNAL_STDINT_PRILEN_PTR "o"
    #define PRIuPTR      RE_STD_INTERNAL_STDINT_PRILEN_PTR "u"
    #define PRIxPTR      RE_STD_INTERNAL_STDINT_PRILEN_PTR "x"
    #define PRIXPTR      RE_STD_INTERNAL_STDINT_PRILEN_PTR "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_MAX_FMT == 1) &&                                 \
      (!defined(PRIdMAX)) )
    #define PRIdMAX      RE_STD_INTERNAL_STDINT_PRILEN_MAX "d"
    #define PRIiMAX      RE_STD_INTERNAL_STDINT_PRILEN_MAX "i"
    #define PRIoMAX      RE_STD_INTERNAL_STDINT_PRILEN_MAX "o"
    #define PRIuMAX      RE_STD_INTERNAL_STDINT_PRILEN_MAX "u"
    #define PRIxMAX      RE_STD_INTERNAL_STDINT_PRILEN_MAX "x"
    #define PRIXMAX      RE_STD_INTERNAL_STDINT_PRILEN_MAX "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST8_FMT == 1) &&                               \
      (!defined(PRIdFAST8)) )
    #define PRIdFAST8    RE_STD_INTERNAL_STDINT_PRILEN_FAST8 "d"
    #define PRIiFAST8    RE_STD_INTERNAL_STDINT_PRILEN_FAST8 "i"
    #define PRIoFAST8    RE_STD_INTERNAL_STDINT_PRILEN_FAST8 "o"
    #define PRIuFAST8    RE_STD_INTERNAL_STDINT_PRILEN_FAST8 "u"
    #define PRIxFAST8    RE_STD_INTERNAL_STDINT_PRILEN_FAST8 "x"
    #define PRIXFAST8    RE_STD_INTERNAL_STDINT_PRILEN_FAST8 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST16_FMT == 1) &&                              \
      (!defined(PRIdFAST16)) )
    #define PRIdFAST16   RE_STD_INTERNAL_STDINT_PRILEN_FAST16 "d"
    #define PRIiFAST16   RE_STD_INTERNAL_STDINT_PRILEN_FAST16 "i"
    #define PRIoFAST16   RE_STD_INTERNAL_STDINT_PRILEN_FAST16 "o"
    #define PRIuFAST16   RE_STD_INTERNAL_STDINT_PRILEN_FAST16 "u"
    #define PRIxFAST16   RE_STD_INTERNAL_STDINT_PRILEN_FAST16 "x"
    #define PRIXFAST16   RE_STD_INTERNAL_STDINT_PRILEN_FAST16 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST32_FMT == 1) &&                              \
      (!defined(PRIdFAST32)) )
    #define PRIdFAST32   RE_STD_INTERNAL_STDINT_PRILEN_FAST32 "d"
    #define PRIiFAST32   RE_STD_INTERNAL_STDINT_PRILEN_FAST32 "i"
    #define PRIoFAST32   RE_STD_INTERNAL_STDINT_PRILEN_FAST32 "o"
    #define PRIuFAST32   RE_STD_INTERNAL_STDINT_PRILEN_FAST32 "u"
    #define PRIxFAST32   RE_STD_INTERNAL_STDINT_PRILEN_FAST32 "x"
    #define PRIXFAST32   RE_STD_INTERNAL_STDINT_PRILEN_FAST32 "X"
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST64_FMT == 1) &&                              \
      (!defined(PRIdFAST64)) )
    #define PRIdFAST64   RE_STD_INTERNAL_STDINT_PRILEN_FAST64 "d"
    #define PRIiFAST64   RE_STD_INTERNAL_STDINT_PRILEN_FAST64 "i"
    #define PRIoFAST64   RE_STD_INTERNAL_STDINT_PRILEN_FAST64 "o"
    #define PRIuFAST64   RE_STD_INTERNAL_STDINT_PRILEN_FAST64 "u"
    #define PRIxFAST64   RE_STD_INTERNAL_STDINT_PRILEN_FAST64 "x"
    #define PRIXFAST64   RE_STD_INTERNAL_STDINT_PRILEN_FAST64 "X"
#endif


// 6.4    scanf formats
//------------------------------------------------------------------------------
// 6.4.1
// SCN formats
//   constant: SCNdN, SCNiN, SCNoN, SCNuN and SCNxN for the same families as
// the printf formats. Those of the 8-bit families need `hh`, and are left out
// where scanf has none (6.2.4).
#if ( (RE_STD_INTERNAL_STDINT_8_FMT == 1) &&                                   \
      (RE_STD_INTERNAL_STDINT_HAS_HH == 1) &&                                  \
      (!defined(SCNd8)) )
    #define SCNd8        RE_STD_INTERNAL_STDINT_SCNLEN_8 "d"
    #define SCNi8        RE_STD_INTERNAL_STDINT_SCNLEN_8 "i"
    #define SCNo8        RE_STD_INTERNAL_STDINT_SCNLEN_8 "o"
    #define SCNu8        RE_STD_INTERNAL_STDINT_SCNLEN_8 "u"
    #define SCNx8        RE_STD_INTERNAL_STDINT_SCNLEN_8 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_16_FMT == 1) &&                                  \
      (!defined(SCNd16)) )
    #define SCNd16       RE_STD_INTERNAL_STDINT_SCNLEN_16 "d"
    #define SCNi16       RE_STD_INTERNAL_STDINT_SCNLEN_16 "i"
    #define SCNo16       RE_STD_INTERNAL_STDINT_SCNLEN_16 "o"
    #define SCNu16       RE_STD_INTERNAL_STDINT_SCNLEN_16 "u"
    #define SCNx16       RE_STD_INTERNAL_STDINT_SCNLEN_16 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_32_FMT == 1) &&                                  \
      (!defined(SCNd32)) )
    #define SCNd32       RE_STD_INTERNAL_STDINT_SCNLEN_32 "d"
    #define SCNi32       RE_STD_INTERNAL_STDINT_SCNLEN_32 "i"
    #define SCNo32       RE_STD_INTERNAL_STDINT_SCNLEN_32 "o"
    #define SCNu32       RE_STD_INTERNAL_STDINT_SCNLEN_32 "u"
    #define SCNx32       RE_STD_INTERNAL_STDINT_SCNLEN_32 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_64_FMT == 1) &&                                  \
      (!defined(SCNd64)) )
    #define SCNd64       RE_STD_INTERNAL_STDINT_SCNLEN_64 "d"
    #define SCNi64       RE_STD_INTERNAL_STDINT_SCNLEN_64 "i"
    #define SCNo64       RE_STD_INTERNAL_STDINT_SCNLEN_64 "o"
    #define SCNu64       RE_STD_INTERNAL_STDINT_SCNLEN_64 "u"
    #define SCNx64       RE_STD_INTERNAL_STDINT_SCNLEN_64 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST8_FMT == 1) &&                              \
      (RE_STD_INTERNAL_STDINT_HAS_HH == 1) &&                                  \
      (!defined(SCNdLEAST8)) )
    #define SCNdLEAST8   RE_STD_INTERNAL_STDINT_SCNLEN_LEAST8 "d"
    #define SCNiLEAST8   RE_STD_INTERNAL_STDINT_SCNLEN_LEAST8 "i"
    #define SCNoLEAST8   RE_STD_INTERNAL_STDINT_SCNLEN_LEAST8 "o"
    #define SCNuLEAST8   RE_STD_INTERNAL_STDINT_SCNLEN_LEAST8 "u"
    #define SCNxLEAST8   RE_STD_INTERNAL_STDINT_SCNLEN_LEAST8 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST16_FMT == 1) &&                             \
      (!defined(SCNdLEAST16)) )
    #define SCNdLEAST16  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST16 "d"
    #define SCNiLEAST16  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST16 "i"
    #define SCNoLEAST16  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST16 "o"
    #define SCNuLEAST16  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST16 "u"
    #define SCNxLEAST16  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST16 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST32_FMT == 1) &&                             \
      (!defined(SCNdLEAST32)) )
    #define SCNdLEAST32  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST32 "d"
    #define SCNiLEAST32  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST32 "i"
    #define SCNoLEAST32  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST32 "o"
    #define SCNuLEAST32  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST32 "u"
    #define SCNxLEAST32  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST32 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_LEAST64_FMT == 1) &&                             \
      (!defined(SCNdLEAST64)) )
    #define SCNdLEAST64  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST64 "d"
    #define SCNiLEAST64  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST64 "i"
    #define SCNoLEAST64  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST64 "o"
    #define SCNuLEAST64  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST64 "u"
    #define SCNxLEAST64  RE_STD_INTERNAL_STDINT_SCNLEN_LEAST64 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_PTR_FMT == 1) &&                                 \
      (!defined(SCNdPTR)) )
    #define SCNdPTR      RE_STD_INTERNAL_STDINT_SCNLEN_PTR "d"
    #define SCNiPTR      RE_STD_INTERNAL_STDINT_SCNLEN_PTR "i"
    #define SCNoPTR      RE_STD_INTERNAL_STDINT_SCNLEN_PTR "o"
    #define SCNuPTR      RE_STD_INTERNAL_STDINT_SCNLEN_PTR "u"
    #define SCNxPTR      RE_STD_INTERNAL_STDINT_SCNLEN_PTR "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_MAX_FMT == 1) &&                                 \
      (!defined(SCNdMAX)) )
    #define SCNdMAX      RE_STD_INTERNAL_STDINT_SCNLEN_MAX "d"
    #define SCNiMAX      RE_STD_INTERNAL_STDINT_SCNLEN_MAX "i"
    #define SCNoMAX      RE_STD_INTERNAL_STDINT_SCNLEN_MAX "o"
    #define SCNuMAX      RE_STD_INTERNAL_STDINT_SCNLEN_MAX "u"
    #define SCNxMAX      RE_STD_INTERNAL_STDINT_SCNLEN_MAX "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST8_FMT == 1) &&                               \
      (RE_STD_INTERNAL_STDINT_HAS_HH == 1) &&                                  \
      (!defined(SCNdFAST8)) )
    #define SCNdFAST8    RE_STD_INTERNAL_STDINT_SCNLEN_FAST8 "d"
    #define SCNiFAST8    RE_STD_INTERNAL_STDINT_SCNLEN_FAST8 "i"
    #define SCNoFAST8    RE_STD_INTERNAL_STDINT_SCNLEN_FAST8 "o"
    #define SCNuFAST8    RE_STD_INTERNAL_STDINT_SCNLEN_FAST8 "u"
    #define SCNxFAST8    RE_STD_INTERNAL_STDINT_SCNLEN_FAST8 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST16_FMT == 1) &&                              \
      (!defined(SCNdFAST16)) )
    #define SCNdFAST16   RE_STD_INTERNAL_STDINT_SCNLEN_FAST16 "d"
    #define SCNiFAST16   RE_STD_INTERNAL_STDINT_SCNLEN_FAST16 "i"
    #define SCNoFAST16   RE_STD_INTERNAL_STDINT_SCNLEN_FAST16 "o"
    #define SCNuFAST16   RE_STD_INTERNAL_STDINT_SCNLEN_FAST16 "u"
    #define SCNxFAST16   RE_STD_INTERNAL_STDINT_SCNLEN_FAST16 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST32_FMT == 1) &&                              \
      (!defined(SCNdFAST32)) )
    #define SCNdFAST32   RE_STD_INTERNAL_STDINT_SCNLEN_FAST32 "d"
    #define SCNiFAST32   RE_STD_INTERNAL_STDINT_SCNLEN_FAST32 "i"
    #define SCNoFAST32   RE_STD_INTERNAL_STDINT_SCNLEN_FAST32 "o"
    #define SCNuFAST32   RE_STD_INTERNAL_STDINT_SCNLEN_FAST32 "u"
    #define SCNxFAST32   RE_STD_INTERNAL_STDINT_SCNLEN_FAST32 "x"
#endif

#if ( (RE_STD_INTERNAL_STDINT_FAST64_FMT == 1) &&                              \
      (!defined(SCNdFAST64)) )
    #define SCNdFAST64   RE_STD_INTERNAL_STDINT_SCNLEN_FAST64 "d"
    #define SCNiFAST64   RE_STD_INTERNAL_STDINT_SCNLEN_FAST64 "i"
    #define SCNoFAST64   RE_STD_INTERNAL_STDINT_SCNLEN_FAST64 "o"
    #define SCNuFAST64   RE_STD_INTERNAL_STDINT_SCNLEN_FAST64 "u"
    #define SCNxFAST64   RE_STD_INTERNAL_STDINT_SCNLEN_FAST64 "x"
#endif


#endif  // RE_STD_CSTDINT_DSTDINT_H
