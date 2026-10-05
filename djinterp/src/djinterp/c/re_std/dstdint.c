/*******************************************************************************
* djinterp [c]                                                         dstdint.c
*
* Compile-time verification of dstdint.h.
*   The header declares only types and macros, which need no definitions, so
* this unit holds the header's claims instead, for the compiler to check
* wherever the unit is built: each exact-width type has exactly N bits and no
* padding; every type has the signedness its name gives it; each limit is the
* limit of its type; every limit and constant works in #if; intptr_t holds a
* pointer and intmax_t is the widest type; a family is present or absent as a
* whole; and, for the header's own definitions, each limit and constant has
* exactly its type after promotion (where the language can compare types: C++
* and C11) and each type is the one the compiler predefines.
* A family the header leaves out is skipped, as its missing limit macro says
* to.
*   It also holds the two libraries to one answer about the pointer's width,
* which re_std states for itself (RE_STD_POINTER_BITS) and djinterp's env
* states for everything else (D_ENV_ARCH_POINTER_BITS): each is the width the
* compiler gives `void*`, wherever it claims to know.
*   The unit compiles as C99 and later and as C++98 and later. C builds use the
* platform's headers, so they check the values the platform gives, on which
* the rest of the framework relies; the header's own definitions are checked
* in full by the C++ builds that take them (ISO strict C++98, or a library
* without <stdint.h>) and by any build that pins RE_STD_CFG_STDINT_BACKEND to
* RE_STD_CFG_STDINT_BACKEND_OWN.
*   Three headers are included at the end and not at the top, each beside the
* one check that needs it: djinterp's env, <signal.h> and <wchar.h>. Any of
* them may bring the platform's <stdint.h> with it (env reads <unistd.h>,
* which does on WASI), and that header's macros then stand in place of this
* one's, whichever backend is in use. Everything before them is about
* dstdint.h's definitions alone.
*
*   Check (from the repo root):
*     cc  -std=c99   -pedantic-errors -fsyntax-only                           \
*         src/djinterp/c/re_std/dstdint.c
*     c++ -std=c++98 -pedantic-errors -fsyntax-only -DRE_STD_CFG_ISO_STRICT=1  \
*         -x c++ src/djinterp/c/re_std/dstdint.c
*
*
* path:      /src/djinterp/c/re_std/dstdint.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.02
*******************************************************************************/
#include "../../../../inc/re_std/cstdint/dstdint.h"  // the header verified
// re_std
#include "../../../../inc/re_std/config.h"  // RE_STD_LANG_IS_CPP,
                                            // RE_STD_LANG_IS_CPP11_OR_HIGHER,
                                            // RE_STD_COMPILER_GCC,
                                            // RE_STD_COMPILER_VERSION_AT_LEAST,
                                            // RE_STD_POINTER_BITS
// std
#include <limits.h>  // CHAR_BIT, LONG_MAX, ULONG_MAX
#include <stddef.h>  // size_t, ptrdiff_t, wchar_t

// sig_atomic_t and wint_t come from <signal.h> and <wchar.h>, which a
// freestanding build need not have, and which some hosted C libraries leave
// out or hollow as well: avr-libc has neither type, though it has a
// <signal.h>, an obsolete name for its interrupt header; and wasi-libc's
// <signal.h> is an #error unless the build asks for its emulation. The two
// headers are included at the end of this unit, where their types are
// needed, and not here (see "The C library's types")
#if ( (defined(__STDC_HOSTED__)) &&                                           \
      (__STDC_HOSTED__ == 0) )
    #define D_INTERNAL_STDINT_VERIFY_SIGNAL 0
    #define D_INTERNAL_STDINT_VERIFY_WCHAR  0
#elif defined(__has_include)
    #if ( (__has_include(<signal.h>))  &&                                     \
          (!defined(__AVR__))          &&                                     \
          ( (!defined(__wasi__)) ||                                           \
            (defined(_WASI_EMULATED_SIGNAL)) ) )
        #define D_INTERNAL_STDINT_VERIFY_SIGNAL 1
    #else
        #define D_INTERNAL_STDINT_VERIFY_SIGNAL 0
    #endif

    #if __has_include(<wchar.h>)
        #define D_INTERNAL_STDINT_VERIFY_WCHAR 1
    #else
        #define D_INTERNAL_STDINT_VERIFY_WCHAR 0
    #endif
#else
    #define D_INTERNAL_STDINT_VERIFY_SIGNAL 1
    #define D_INTERNAL_STDINT_VERIFY_WCHAR  1
#endif


/*
D_INTERNAL_STDINT_VERIFY
  A compile-time assertion at file scope that works in every language mode the
unit compiles in: a false `_cond` declares an array of negative size, named for
the check that failed. djinterp.h's D_STATIC_ASSERT would serve, but would make
the unit depend on more than the header it verifies.
*/
#define D_INTERNAL_STDINT_VERIFY(_name, _cond)                                \
    typedef char d_internal_stdint_verify_##_name[(_cond) ? 1 : -1]

/*
D_INTERNAL_STDINT_VERIFY_OWN
  As D_INTERNAL_STDINT_VERIFY, for a check that the header's own definitions
must pass and a platform's need not, which on the platform path always holds.
That a limit or constant has its type after promotion is such a check: Clang's
freestanding <stdint.h>, for one, gives UINT8_MAX and UINT8_C, and their 16-bit
counterparts, the type `unsigned int` where C requires `int`, and that is no
reason for djinterp not to build. (It is WG14 DR 209, which Clang's own
conformance page lists as partial for this very reason: measured in Clang
through 20.1, and fixed in 21.1. There `-1 < UINT8_MAX` is false, in #if as
in code. The header's own definitions, which every build can pin, have the
required types.)
*/
#if (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_OWN)
    #define D_INTERNAL_STDINT_VERIFY_OWN(_name, _cond)                        \
        D_INTERNAL_STDINT_VERIFY(_name, _cond)
#else
    #define D_INTERNAL_STDINT_VERIFY_OWN(_name, _cond)                        \
        D_INTERNAL_STDINT_VERIFY(_name, 1)
#endif

/*
D_INTERNAL_STDINT_UNSIGNED
  1 when the expression `_e` has an unsigned type after the integer
promotions, 0 when it has a signed one: only an unsigned type makes one less
than zero positive.
*/
#define D_INTERNAL_STDINT_UNSIGNED(_e)                                        \
    ( ((_e) * 0 - 1) > 0 )

/*
D_INTERNAL_STDINT_HAS_GENERIC
  1 in a C build that has C11's generic selection: C11 or later, with one
exception that is still installed. GCC accepted -std=c11 two releases before
it implemented _Generic, in 4.9 (4.8.5 is the system compiler of CentOS 7);
a C11 build there checks what a C99 build does.
*/
#if ( (RE_STD_LANG_IS_CPP == 1)       ||                                      \
      (!defined(__STDC_VERSION__)) )
    #define D_INTERNAL_STDINT_HAS_GENERIC 0
#elif (__STDC_VERSION__ < 201112L)
    #define D_INTERNAL_STDINT_HAS_GENERIC 0
#elif ( (defined(RE_STD_COMPILER_GCC)) &&                                     \
        (!RE_STD_COMPILER_VERSION_AT_LEAST(4, 9, 0)) )
    #define D_INTERNAL_STDINT_HAS_GENERIC 0
#else
    #define D_INTERNAL_STDINT_HAS_GENERIC 1
#endif

/*
D_INTERNAL_STDINT_SAME_TYPE
  1 when the expressions `_a` and `_b` have one type. C++ tells by overload
resolution, at every level: of the two function templates below, the one that
takes both arguments as a single type is the more specialized, and is chosen
exactly when they have one. C11 tells by generic selection over the six types
an integer expression can have after the promotions. C99 can tell neither
way, and compares size and signedness, which cannot separate `long` from
`long long` where they have one width.
*/
#if (RE_STD_LANG_IS_CPP == 1)
    template<typename Type,
             typename Other>
    char (&d_internal_stdint_types(const Type&, const Other&))[1];

    template<typename Type>
    char (&d_internal_stdint_types(const Type&, const Type&))[2];

    #define D_INTERNAL_STDINT_SAME_TYPE(_a, _b)                               \
        (sizeof(d_internal_stdint_types((_a), (_b))) == 2)
#elif (D_INTERNAL_STDINT_HAS_GENERIC == 1)
    #define D_INTERNAL_STDINT_TYPE_ID(_e)                                     \
        _Generic((_e),                                                        \
                 int:                1,                                       \
                 unsigned int:       2,                                       \
                 long:               3,                                       \
                 unsigned long:      4,                                       \
                 long long:          5,                                       \
                 unsigned long long: 6,                                       \
                 default:            0)

    #define D_INTERNAL_STDINT_SAME_TYPE(_a, _b)                               \
        ( (D_INTERNAL_STDINT_TYPE_ID(_a) != 0) &&                             \
          (D_INTERNAL_STDINT_TYPE_ID(_a) == D_INTERNAL_STDINT_TYPE_ID(_b)) )
#else
    #define D_INTERNAL_STDINT_SAME_TYPE(_a, _b)                               \
        ( (sizeof(_a) == sizeof(_b)) &&                                       \
          (D_INTERNAL_STDINT_UNSIGNED(_a) == D_INTERNAL_STDINT_UNSIGNED(_b)) )
#endif

/*
D_INTERNAL_STDINT_HAS_TYPE
  1 when the expression `_e` has the type an object of type `_type` has after
the integer promotions, which is the type C requires of each limit and each
constant macro: the very type, so that a constant spelled `ULL` where the
type is `unsigned long` fails, though the two have one size.
*/
#define D_INTERNAL_STDINT_HAS_TYPE(_e, _type)                                 \
    D_INTERNAL_STDINT_SAME_TYPE((_e), +(_type)0)

/*
D_INTERNAL_STDINT_VERIFY_FAMILY
  The checks every family passes, given its name `_f`, its types `_s` and `_u`
and its limits `_smin`, `_smax` and `_umax`: the signed type is signed and the
unsigned type unsigned, and both have one size; the unsigned maximum is the
unsigned type's largest value, the signed maximum half of it, and the minimum
the two's complement one; and each limit the header defines has its type after
promotion, as C requires.
*/
#define D_INTERNAL_STDINT_VERIFY_FAMILY(_f, _s, _u, _smin, _smax, _umax)      \
    D_INTERNAL_STDINT_VERIFY(_f##_signed,   ((_s)-1) < 0);                    \
    D_INTERNAL_STDINT_VERIFY(_f##_unsigned, ((_u)-1) > 0);                    \
    D_INTERNAL_STDINT_VERIFY(_f##_size,     sizeof(_s) == sizeof(_u));        \
    D_INTERNAL_STDINT_VERIFY(_f##_umax,     (_umax) == (_u)-1);               \
    D_INTERNAL_STDINT_VERIFY(_f##_smax,     (_smax) == (_s)((_umax) >> 1));   \
    D_INTERNAL_STDINT_VERIFY(_f##_smin,     (_smin) == -(_smax) - 1);         \
    D_INTERNAL_STDINT_VERIFY_OWN(_f##_smin_type,                              \
        D_INTERNAL_STDINT_HAS_TYPE(_smin, _s));                               \
    D_INTERNAL_STDINT_VERIFY_OWN(_f##_smax_type,                              \
        D_INTERNAL_STDINT_HAS_TYPE(_smax, _s));                               \
    D_INTERNAL_STDINT_VERIFY_OWN(_f##_umax_type,                              \
        D_INTERNAL_STDINT_HAS_TYPE(_umax, _u))

/*
D_INTERNAL_STDINT_VERIFY_EXACT
  The exact-width checks, given the family's name `_f`, width `_n`, signed
type `_s` and unsigned maximum `_umax`: the type is `_n` bits in size, and all
of them hold value, since the unsigned maximum has exactly `_n` value bits.
*/
#define D_INTERNAL_STDINT_VERIFY_EXACT(_f, _n, _s, _umax)                     \
    D_INTERNAL_STDINT_VERIFY(_f##_bits,       sizeof(_s) * CHAR_BIT == (_n)); \
    D_INTERNAL_STDINT_VERIFY(_f##_no_padding, ((_umax) >> ((_n) - 1)) == 1)

/*
D_INTERNAL_STDINT_VERIFY_LEAST
  The minimum-width check, given the family's name `_f`, width `_n` and
unsigned maximum `_umax`: the unsigned maximum has at least `_n` value bits.
*/
#define D_INTERNAL_STDINT_VERIFY_LEAST(_f, _n, _umax)                         \
    D_INTERNAL_STDINT_VERIFY(_f##_bits, ((_umax) >> ((_n) - 1)) >= 1)

/*
D_INTERNAL_STDINT_VERIFY_CONSTANTS
  The constant-macro checks, given the family's name `_f`, its types `_s` and
`_u`, and a constant made by each of its macros, `_sc` and `_uc`: each
constant the header defines has its type after promotion.
*/
#define D_INTERNAL_STDINT_VERIFY_CONSTANTS(_f, _s, _u, _sc, _uc)              \
    D_INTERNAL_STDINT_VERIFY_OWN(_f##_sc_type,                                \
        D_INTERNAL_STDINT_HAS_TYPE(_sc, _s));                                 \
    D_INTERNAL_STDINT_VERIFY_OWN(_f##_uc_type,                                \
        D_INTERNAL_STDINT_HAS_TYPE(_uc, _u))

/*
d_internal_stdint_wide
  The widest unsigned type the build has, through which the limits of the
types below are computed: uintmax_t, or `unsigned long` where the header
leaves that out.
*/
#ifdef UINTMAX_MAX
    typedef uintmax_t     d_internal_stdint_wide;
#else
    typedef unsigned long d_internal_stdint_wide;
#endif  // UINTMAX_MAX

/*
D_INTERNAL_STDINT_VERIFY_OTHER
  The checks for a type this header only gives limits to, given its name
`_f`, the type `_t`, the type `_under` it is made of, and the limits `_min`
and `_max`. The limits are exactly the extremes of the type, computed from
its width and signedness, so that a maximum one short or a minimum one off
fails: the maximum is all of an unsigned type's bits, or all but the sign of
a signed one's; the minimum is zero, or the value that added to the maximum
makes -1. (The sums are taken in the wide unsigned type, where they are
defined whatever the signs.) And both limits, where the header defines them,
have the type of `_under` after promotion. That is the type itself in C, and
in C++ for all but wchar_t, which is a type of its own there and promotes by
its range, not by its underlying type: where that is a 32-bit `long`, an
object of it promotes to `int`, while every platform's WCHAR_MAX, and the
compiler's own, is a `long`.
*/
#define D_INTERNAL_STDINT_VERIFY_OTHER(_f, _t, _under, _min, _max)            \
    D_INTERNAL_STDINT_VERIFY(_f##_fits_wide,                                  \
        sizeof(_t) <= sizeof(d_internal_stdint_wide));                        \
    D_INTERNAL_STDINT_VERIFY(_f##_max_exact,                                  \
        (d_internal_stdint_wide)(_max) ==                                     \
        ( (((_t)-1) > ((_t)0))                                                \
          ? (d_internal_stdint_wide)((_t)-1)                                  \
          : (d_internal_stdint_wide)                                          \
            ( ( ((d_internal_stdint_wide)1) <<                                \
                ( ( (sizeof(_t) <= sizeof(d_internal_stdint_wide))            \
                    ? sizeof(_t)                                              \
                    : 1 ) * CHAR_BIT - 1 ) ) - 1 ) ));                        \
    D_INTERNAL_STDINT_VERIFY(_f##_min_exact,                                  \
        (((_t)-1) > ((_t)0))                                                  \
        ? ( (d_internal_stdint_wide)(_min) == 0 )                             \
        : ( (d_internal_stdint_wide)                                          \
            ( (d_internal_stdint_wide)(_min) +                                \
              (d_internal_stdint_wide)(_max) ) ==                             \
            (d_internal_stdint_wide)-1 ));                                    \
    D_INTERNAL_STDINT_VERIFY_OWN(_f##_min_type,                               \
        D_INTERNAL_STDINT_HAS_TYPE(_min, _under));                            \
    D_INTERNAL_STDINT_VERIFY_OWN(_f##_max_type,                               \
        D_INTERNAL_STDINT_HAS_TYPE(_max, _under))


// the backend is one of the two the configuration names; this check also
// keeps the unit from being empty on a platform with no families at all
D_INTERNAL_STDINT_VERIFY(backend,
                         (RE_STD_STDINT_BACKEND ==
                          RE_STD_CFG_STDINT_BACKEND_PLATFORM) ||
                         (RE_STD_STDINT_BACKEND ==
                          RE_STD_CFG_STDINT_BACKEND_OWN));


/*
Fast types without formats
  The one state the header cannot mend, said by name before the rule below
calls it "incomplete": a C library's <stdint.h> declared the fastest
minimum-width types, in a build with no <inttypes.h> to give their formats,
and the library is not one whose choice of types the header records
(dstdint.h, 2.4.2). The formats would have to be guessed. The build has three
ways out: put the library's <inttypes.h> on the include path, or define
RE_STD_HAS_HEADER_INTTYPES as 1 where it is there and was not found; add the
library's row to dstdint.h, 2.4.2; or pin the header's own backend
(RE_STD_CFG_STDINT_BACKEND), which declares the compiler's types.
*/
#if ( (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_PLATFORM) &&        \
      (RE_STD_INTERNAL_STDINT_FAST_FROM == 0)                       &&        \
      ( ( (defined(INT_FAST8_MAX))  && (!defined(PRIdFAST8)) )  ||            \
        ( (defined(INT_FAST16_MAX)) && (!defined(PRIdFAST16)) ) ||            \
        ( (defined(INT_FAST32_MAX)) && (!defined(PRIdFAST32)) ) ||            \
        ( (defined(INT_FAST64_MAX)) && (!defined(PRIdFAST64)) ) ) )
    #error "dstdint.h: the int_fastN_t types of this <stdint.h> have no formats"
#endif


/*
Presence
  A family is present or absent as a whole: its limit macro is defined exactly
when all of its names are, which is what makes `#ifdef INT64_MAX` a sufficient
test. An exact-width family also implies the minimum-width family of its
width. The 8-bit families' scanf formats are required only where the language
has their `hh` modifier.
*/
#if ( (defined(INT8_MAX))                                   &&                \
      ( (!defined(INT8_MIN))   || (!defined(UINT8_MAX))     ||                \
        (!defined(PRId8))      || (!defined(PRIi8))         ||                \
        (!defined(PRIo8))      || (!defined(PRIu8))         ||                \
        (!defined(PRIx8))      || (!defined(PRIX8))         ||                \
        (!defined(INT_LEAST8_MAX)) ) )
    #error "dstdint.h: the int8_t family is incomplete"
#endif

#if ( (defined(INT16_MAX))                                  &&                \
      ( (!defined(INT16_MIN))  || (!defined(UINT16_MAX))    ||                \
        (!defined(PRId16))     || (!defined(PRIi16))        ||                \
        (!defined(PRIo16))     || (!defined(PRIu16))        ||                \
        (!defined(PRIx16))     || (!defined(PRIX16))        ||                \
        (!defined(SCNd16))     || (!defined(SCNi16))        ||                \
        (!defined(SCNo16))     || (!defined(SCNu16))        ||                \
        (!defined(SCNx16))     || (!defined(INT_LEAST16_MAX)) ) )
    #error "dstdint.h: the int16_t family is incomplete"
#endif

#if ( (defined(INT32_MAX))                                  &&                \
      ( (!defined(INT32_MIN))  || (!defined(UINT32_MAX))    ||                \
        (!defined(PRId32))     || (!defined(PRIi32))        ||                \
        (!defined(PRIo32))     || (!defined(PRIu32))        ||                \
        (!defined(PRIx32))     || (!defined(PRIX32))        ||                \
        (!defined(SCNd32))     || (!defined(SCNi32))        ||                \
        (!defined(SCNo32))     || (!defined(SCNu32))        ||                \
        (!defined(SCNx32))     || (!defined(INT_LEAST32_MAX)) ) )
    #error "dstdint.h: the int32_t family is incomplete"
#endif

#if ( (defined(INT64_MAX))                                  &&                \
      ( (!defined(INT64_MIN))  || (!defined(UINT64_MAX))    ||                \
        (!defined(PRId64))     || (!defined(PRIi64))        ||                \
        (!defined(PRIo64))     || (!defined(PRIu64))        ||                \
        (!defined(PRIx64))     || (!defined(PRIX64))        ||                \
        (!defined(SCNd64))     || (!defined(SCNi64))        ||                \
        (!defined(SCNo64))     || (!defined(SCNu64))        ||                \
        (!defined(SCNx64))     || (!defined(INT_LEAST64_MAX)) ) )
    #error "dstdint.h: the int64_t family is incomplete"
#endif

#if ( (defined(INT_LEAST8_MAX))                             &&                \
      ( (!defined(INT_LEAST8_MIN))  || (!defined(UINT_LEAST8_MAX))  ||        \
        (!defined(INT8_C))          || (!defined(UINT8_C))          ||        \
        (!defined(PRIdLEAST8))      || (!defined(PRIiLEAST8))       ||        \
        (!defined(PRIoLEAST8))      || (!defined(PRIuLEAST8))       ||        \
        (!defined(PRIxLEAST8))      || (!defined(PRIXLEAST8)) ) )
    #error "dstdint.h: the int_least8_t family is incomplete"
#endif

#if ( (defined(INT_LEAST16_MAX))                            &&                \
      ( (!defined(INT_LEAST16_MIN)) || (!defined(UINT_LEAST16_MAX)) ||        \
        (!defined(INT16_C))         || (!defined(UINT16_C))         ||        \
        (!defined(PRIdLEAST16))     || (!defined(PRIiLEAST16))      ||        \
        (!defined(PRIoLEAST16))     || (!defined(PRIuLEAST16))      ||        \
        (!defined(PRIxLEAST16))     || (!defined(PRIXLEAST16))      ||        \
        (!defined(SCNdLEAST16))     || (!defined(SCNiLEAST16))      ||        \
        (!defined(SCNoLEAST16))     || (!defined(SCNuLEAST16))      ||        \
        (!defined(SCNxLEAST16)) ) )
    #error "dstdint.h: the int_least16_t family is incomplete"
#endif

#if ( (defined(INT_LEAST32_MAX))                            &&                \
      ( (!defined(INT_LEAST32_MIN)) || (!defined(UINT_LEAST32_MAX)) ||        \
        (!defined(INT32_C))         || (!defined(UINT32_C))         ||        \
        (!defined(PRIdLEAST32))     || (!defined(PRIiLEAST32))      ||        \
        (!defined(PRIoLEAST32))     || (!defined(PRIuLEAST32))      ||        \
        (!defined(PRIxLEAST32))     || (!defined(PRIXLEAST32))      ||        \
        (!defined(SCNdLEAST32))     || (!defined(SCNiLEAST32))      ||        \
        (!defined(SCNoLEAST32))     || (!defined(SCNuLEAST32))      ||        \
        (!defined(SCNxLEAST32)) ) )
    #error "dstdint.h: the int_least32_t family is incomplete"
#endif

#if ( (defined(INT_LEAST64_MAX))                            &&                \
      ( (!defined(INT_LEAST64_MIN)) || (!defined(UINT_LEAST64_MAX)) ||        \
        (!defined(INT64_C))         || (!defined(UINT64_C))         ||        \
        (!defined(PRIdLEAST64))     || (!defined(PRIiLEAST64))      ||        \
        (!defined(PRIoLEAST64))     || (!defined(PRIuLEAST64))      ||        \
        (!defined(PRIxLEAST64))     || (!defined(PRIXLEAST64))      ||        \
        (!defined(SCNdLEAST64))     || (!defined(SCNiLEAST64))      ||        \
        (!defined(SCNoLEAST64))     || (!defined(SCNuLEAST64))      ||        \
        (!defined(SCNxLEAST64)) ) )
    #error "dstdint.h: the int_least64_t family is incomplete"
#endif

#if ( (defined(INTPTR_MAX))                                 &&                \
      ( (!defined(INTPTR_MIN)) || (!defined(UINTPTR_MAX))   ||                \
        (!defined(PRIdPTR))    || (!defined(PRIiPTR))       ||                \
        (!defined(PRIoPTR))    || (!defined(PRIuPTR))       ||                \
        (!defined(PRIxPTR))    || (!defined(PRIXPTR))       ||                \
        (!defined(SCNdPTR))    || (!defined(SCNiPTR))       ||                \
        (!defined(SCNoPTR))    || (!defined(SCNuPTR))       ||                \
        (!defined(SCNxPTR)) ) )
    #error "dstdint.h: the intptr_t family is incomplete"
#endif

#if ( (defined(INTMAX_MAX))                                 &&                \
      ( (!defined(INTMAX_MIN)) || (!defined(UINTMAX_MAX))   ||                \
        (!defined(INTMAX_C))   || (!defined(UINTMAX_C))     ||                \
        (!defined(PRIdMAX))    || (!defined(PRIiMAX))       ||                \
        (!defined(PRIoMAX))    || (!defined(PRIuMAX))       ||                \
        (!defined(PRIxMAX))    || (!defined(PRIXMAX))       ||                \
        (!defined(SCNdMAX))    || (!defined(SCNiMAX))       ||                \
        (!defined(SCNoMAX))    || (!defined(SCNuMAX))       ||                \
        (!defined(SCNxMAX)) ) )
    #error "dstdint.h: the intmax_t family is incomplete"
#endif

#if (RE_STD_INTERNAL_STDINT_HAS_HH == 1)
    #if ( (defined(INT8_MAX))           &&                                    \
          ( (!defined(SCNd8)) || (!defined(SCNi8)) ||                         \
            (!defined(SCNo8)) || (!defined(SCNu8)) ||                         \
            (!defined(SCNx8)) ) )
        #error "dstdint.h: the int8_t family lacks its scanf formats"
    #endif

    #if ( (defined(INT_LEAST8_MAX))     &&                                    \
          ( (!defined(SCNdLEAST8)) || (!defined(SCNiLEAST8)) ||               \
            (!defined(SCNoLEAST8)) || (!defined(SCNuLEAST8)) ||               \
            (!defined(SCNxLEAST8)) ) )
        #error "dstdint.h: the int_least8_t family lacks its scanf formats"
    #endif

    #if ( (defined(INT_FAST8_MAX))      &&                                    \
          ( (!defined(SCNdFAST8)) || (!defined(SCNiFAST8)) ||                 \
            (!defined(SCNoFAST8)) || (!defined(SCNuFAST8)) ||                 \
            (!defined(SCNxFAST8)) ) )
        #error "dstdint.h: the int_fast8_t family lacks its scanf formats"
    #endif
#endif  // RE_STD_INTERNAL_STDINT_HAS_HH == 1

#if ( (defined(INT_FAST8_MAX))                            &&                  \
      ( (!defined(INT_FAST8_MIN)) || (!defined(UINT_FAST8_MAX)) ||            \
        (!defined(PRIdFAST8))      || (!defined(PRIiFAST8))       ||          \
        (!defined(PRIoFAST8))      || (!defined(PRIuFAST8))       ||          \
        (!defined(PRIxFAST8))      || (!defined(PRIXFAST8)) ) )
    #error "dstdint.h: the int_fast8_t family is incomplete"
#endif

#if ( (defined(INT_FAST16_MAX))                           &&                  \
      ( (!defined(INT_FAST16_MIN)) || (!defined(UINT_FAST16_MAX)) ||          \
        (!defined(PRIdFAST16))      || (!defined(PRIiFAST16))       ||        \
        (!defined(PRIoFAST16))      || (!defined(PRIuFAST16))       ||        \
        (!defined(PRIxFAST16))      || (!defined(PRIXFAST16))       ||        \
        (!defined(SCNdFAST16))      || (!defined(SCNiFAST16))       ||        \
        (!defined(SCNoFAST16))      || (!defined(SCNuFAST16))       ||        \
        (!defined(SCNxFAST16)) ) )
    #error "dstdint.h: the int_fast16_t family is incomplete"
#endif

#if ( (defined(INT_FAST32_MAX))                           &&                  \
      ( (!defined(INT_FAST32_MIN)) || (!defined(UINT_FAST32_MAX)) ||          \
        (!defined(PRIdFAST32))      || (!defined(PRIiFAST32))       ||        \
        (!defined(PRIoFAST32))      || (!defined(PRIuFAST32))       ||        \
        (!defined(PRIxFAST32))      || (!defined(PRIXFAST32))       ||        \
        (!defined(SCNdFAST32))      || (!defined(SCNiFAST32))       ||        \
        (!defined(SCNoFAST32))      || (!defined(SCNuFAST32))       ||        \
        (!defined(SCNxFAST32)) ) )
    #error "dstdint.h: the int_fast32_t family is incomplete"
#endif

#if ( (defined(INT_FAST64_MAX))                           &&                  \
      ( (!defined(INT_FAST64_MIN)) || (!defined(UINT_FAST64_MAX)) ||          \
        (!defined(PRIdFAST64))      || (!defined(PRIiFAST64))       ||        \
        (!defined(PRIoFAST64))      || (!defined(PRIuFAST64))       ||        \
        (!defined(PRIxFAST64))      || (!defined(PRIXFAST64))       ||        \
        (!defined(SCNdFAST64))      || (!defined(SCNiFAST64))       ||        \
        (!defined(SCNoFAST64))      || (!defined(SCNuFAST64))       ||        \
        (!defined(SCNxFAST64)) ) )
    #error "dstdint.h: the int_fast64_t family is incomplete"
#endif


/*
Presence, the other way
  And nothing of a family is defined without it: where its limit macro is
absent, so is every other name it has. A format or a limit left standing for
a type the header does not declare -- the fastest minimum-width formats once
were, on the own backend -- would make a program that tests the limit macro
right and one that tests any other name wrong.
*/
#if ( (!defined(INT8_MAX))                                  &&                \
      ( (defined(INT8_MIN))  || (defined(UINT8_MAX)) ||                       \
        (defined(PRId8))     || (defined(PRIi8))     ||                       \
        (defined(PRIo8))     || (defined(PRIu8))     ||                       \
        (defined(PRIx8))     || (defined(PRIX8))     ||                       \
        (defined(SCNd8))     || (defined(SCNi8))     ||                       \
        (defined(SCNo8))     || (defined(SCNu8))     ||                       \
        (defined(SCNx8)) ) )
    #error "dstdint.h: the int8_t family is defined in part"
#endif

#if ( (!defined(INT16_MAX))                                 &&                \
      ( (defined(INT16_MIN))  || (defined(UINT16_MAX)) ||                     \
        (defined(PRId16))     || (defined(PRIi16))     ||                     \
        (defined(PRIo16))     || (defined(PRIu16))     ||                     \
        (defined(PRIx16))     || (defined(PRIX16))     ||                     \
        (defined(SCNd16))     || (defined(SCNi16))     ||                     \
        (defined(SCNo16))     || (defined(SCNu16))     ||                     \
        (defined(SCNx16)) ) )
    #error "dstdint.h: the int16_t family is defined in part"
#endif

#if ( (!defined(INT32_MAX))                                 &&                \
      ( (defined(INT32_MIN))  || (defined(UINT32_MAX)) ||                     \
        (defined(PRId32))     || (defined(PRIi32))     ||                     \
        (defined(PRIo32))     || (defined(PRIu32))     ||                     \
        (defined(PRIx32))     || (defined(PRIX32))     ||                     \
        (defined(SCNd32))     || (defined(SCNi32))     ||                     \
        (defined(SCNo32))     || (defined(SCNu32))     ||                     \
        (defined(SCNx32)) ) )
    #error "dstdint.h: the int32_t family is defined in part"
#endif

#if ( (!defined(INT64_MAX))                                 &&                \
      ( (defined(INT64_MIN))  || (defined(UINT64_MAX)) ||                     \
        (defined(PRId64))     || (defined(PRIi64))     ||                     \
        (defined(PRIo64))     || (defined(PRIu64))     ||                     \
        (defined(PRIx64))     || (defined(PRIX64))     ||                     \
        (defined(SCNd64))     || (defined(SCNi64))     ||                     \
        (defined(SCNo64))     || (defined(SCNu64))     ||                     \
        (defined(SCNx64)) ) )
    #error "dstdint.h: the int64_t family is defined in part"
#endif

#if ( (!defined(INT_LEAST8_MAX))                            &&                \
      ( (defined(INT_LEAST8_MIN))  || (defined(UINT_LEAST8_MAX)) ||           \
        (defined(INT8_C))          || (defined(UINT8_C))         ||           \
        (defined(PRIdLEAST8))      || (defined(PRIiLEAST8))      ||           \
        (defined(PRIoLEAST8))      || (defined(PRIuLEAST8))      ||           \
        (defined(PRIxLEAST8))      || (defined(PRIXLEAST8))      ||           \
        (defined(SCNdLEAST8))      || (defined(SCNiLEAST8))      ||           \
        (defined(SCNoLEAST8))      || (defined(SCNuLEAST8))      ||           \
        (defined(SCNxLEAST8)) ) )
    #error "dstdint.h: the int_least8_t family is defined in part"
#endif

#if ( (!defined(INT_LEAST16_MAX))                           &&                \
      ( (defined(INT_LEAST16_MIN))  || (defined(UINT_LEAST16_MAX)) ||         \
        (defined(INT16_C))          || (defined(UINT16_C))         ||         \
        (defined(PRIdLEAST16))      || (defined(PRIiLEAST16))      ||         \
        (defined(PRIoLEAST16))      || (defined(PRIuLEAST16))      ||         \
        (defined(PRIxLEAST16))      || (defined(PRIXLEAST16))      ||         \
        (defined(SCNdLEAST16))      || (defined(SCNiLEAST16))      ||         \
        (defined(SCNoLEAST16))      || (defined(SCNuLEAST16))      ||         \
        (defined(SCNxLEAST16)) ) )
    #error "dstdint.h: the int_least16_t family is defined in part"
#endif

#if ( (!defined(INT_LEAST32_MAX))                           &&                \
      ( (defined(INT_LEAST32_MIN))  || (defined(UINT_LEAST32_MAX)) ||         \
        (defined(INT32_C))          || (defined(UINT32_C))         ||         \
        (defined(PRIdLEAST32))      || (defined(PRIiLEAST32))      ||         \
        (defined(PRIoLEAST32))      || (defined(PRIuLEAST32))      ||         \
        (defined(PRIxLEAST32))      || (defined(PRIXLEAST32))      ||         \
        (defined(SCNdLEAST32))      || (defined(SCNiLEAST32))      ||         \
        (defined(SCNoLEAST32))      || (defined(SCNuLEAST32))      ||         \
        (defined(SCNxLEAST32)) ) )
    #error "dstdint.h: the int_least32_t family is defined in part"
#endif

#if ( (!defined(INT_LEAST64_MAX))                           &&                \
      ( (defined(INT_LEAST64_MIN))  || (defined(UINT_LEAST64_MAX)) ||         \
        (defined(INT64_C))          || (defined(UINT64_C))         ||         \
        (defined(PRIdLEAST64))      || (defined(PRIiLEAST64))      ||         \
        (defined(PRIoLEAST64))      || (defined(PRIuLEAST64))      ||         \
        (defined(PRIxLEAST64))      || (defined(PRIXLEAST64))      ||         \
        (defined(SCNdLEAST64))      || (defined(SCNiLEAST64))      ||         \
        (defined(SCNoLEAST64))      || (defined(SCNuLEAST64))      ||         \
        (defined(SCNxLEAST64)) ) )
    #error "dstdint.h: the int_least64_t family is defined in part"
#endif

#if ( (!defined(INT_FAST8_MAX))                             &&                \
      ( (defined(INT_FAST8_MIN))  || (defined(UINT_FAST8_MAX)) ||             \
        (defined(PRIdFAST8))      || (defined(PRIiFAST8))      ||             \
        (defined(PRIoFAST8))      || (defined(PRIuFAST8))      ||             \
        (defined(PRIxFAST8))      || (defined(PRIXFAST8))      ||             \
        (defined(SCNdFAST8))      || (defined(SCNiFAST8))      ||             \
        (defined(SCNoFAST8))      || (defined(SCNuFAST8))      ||             \
        (defined(SCNxFAST8)) ) )
    #error "dstdint.h: the int_fast8_t family is defined in part"
#endif

#if ( (!defined(INT_FAST16_MAX))                            &&                \
      ( (defined(INT_FAST16_MIN))  || (defined(UINT_FAST16_MAX)) ||           \
        (defined(PRIdFAST16))      || (defined(PRIiFAST16))      ||           \
        (defined(PRIoFAST16))      || (defined(PRIuFAST16))      ||           \
        (defined(PRIxFAST16))      || (defined(PRIXFAST16))      ||           \
        (defined(SCNdFAST16))      || (defined(SCNiFAST16))      ||           \
        (defined(SCNoFAST16))      || (defined(SCNuFAST16))      ||           \
        (defined(SCNxFAST16)) ) )
    #error "dstdint.h: the int_fast16_t family is defined in part"
#endif

#if ( (!defined(INT_FAST32_MAX))                            &&                \
      ( (defined(INT_FAST32_MIN))  || (defined(UINT_FAST32_MAX)) ||           \
        (defined(PRIdFAST32))      || (defined(PRIiFAST32))      ||           \
        (defined(PRIoFAST32))      || (defined(PRIuFAST32))      ||           \
        (defined(PRIxFAST32))      || (defined(PRIXFAST32))      ||           \
        (defined(SCNdFAST32))      || (defined(SCNiFAST32))      ||           \
        (defined(SCNoFAST32))      || (defined(SCNuFAST32))      ||           \
        (defined(SCNxFAST32)) ) )
    #error "dstdint.h: the int_fast32_t family is defined in part"
#endif

#if ( (!defined(INT_FAST64_MAX))                            &&                \
      ( (defined(INT_FAST64_MIN))  || (defined(UINT_FAST64_MAX)) ||           \
        (defined(PRIdFAST64))      || (defined(PRIiFAST64))      ||           \
        (defined(PRIoFAST64))      || (defined(PRIuFAST64))      ||           \
        (defined(PRIxFAST64))      || (defined(PRIXFAST64))      ||           \
        (defined(SCNdFAST64))      || (defined(SCNiFAST64))      ||           \
        (defined(SCNoFAST64))      || (defined(SCNuFAST64))      ||           \
        (defined(SCNxFAST64)) ) )
    #error "dstdint.h: the int_fast64_t family is defined in part"
#endif

#if ( (!defined(INTPTR_MAX))                                &&                \
      ( (defined(INTPTR_MIN))  || (defined(UINTPTR_MAX)) ||                   \
        (defined(PRIdPTR))     || (defined(PRIiPTR))     ||                   \
        (defined(PRIoPTR))     || (defined(PRIuPTR))     ||                   \
        (defined(PRIxPTR))     || (defined(PRIXPTR))     ||                   \
        (defined(SCNdPTR))     || (defined(SCNiPTR))     ||                   \
        (defined(SCNoPTR))     || (defined(SCNuPTR))     ||                   \
        (defined(SCNxPTR)) ) )
    #error "dstdint.h: the intptr_t family is defined in part"
#endif

#if ( (!defined(INTMAX_MAX))                                &&                \
      ( (defined(INTMAX_MIN))  || (defined(UINTMAX_MAX)) ||                   \
        (defined(INTMAX_C))    || (defined(UINTMAX_C))   ||                   \
        (defined(PRIdMAX))     || (defined(PRIiMAX))     ||                   \
        (defined(PRIoMAX))     || (defined(PRIuMAX))     ||                   \
        (defined(PRIxMAX))     || (defined(PRIXMAX))     ||                   \
        (defined(SCNdMAX))     || (defined(SCNiMAX))     ||                   \
        (defined(SCNoMAX))     || (defined(SCNuMAX))     ||                   \
        (defined(SCNxMAX)) ) )
    #error "dstdint.h: the intmax_t family is defined in part"
#endif


/*
Preprocessor arithmetic
  C requires every limit and constant macro to be usable in #if, where it must
evaluate to the same value as in the program. Each present family is checked
there against literal values; a wrong suffix or spelling fails here or does
not preprocess at all.
*/
#ifdef INT8_MAX
    #if ( (INT8_MAX  != 127)      ||                                          \
          (INT8_MIN  != -127 - 1) ||                                          \
          (UINT8_MAX != 255) )
        #error "dstdint.h: an 8-bit limit is wrong in #if"
    #endif
#endif  // INT8_MAX

#ifdef INT16_MAX
    #if ( (INT16_MAX  != 32767)        ||                                     \
          (INT16_MIN  != -32767 - 1)   ||                                     \
          (UINT16_MAX != 65535) )
        #error "dstdint.h: a 16-bit limit is wrong in #if"
    #endif
#endif  // INT16_MAX

#ifdef INT32_MAX
    #if ( (INT32_MAX  != 2147483647)      ||                                  \
          (INT32_MIN  != -2147483647 - 1) ||                                  \
          (UINT32_MAX != 4294967295U) )
        #error "dstdint.h: a 32-bit limit is wrong in #if"
    #endif
#endif  // INT32_MAX

#ifdef INT64_MAX
    #if ( ((INT64_MAX >> 62)  != 1)              ||                           \
          (((INT64_MAX >> 31) >> 31) != 1)        ||                          \
          (INT64_MIN != -INT64_MAX - 1)           ||                          \
          ((UINT64_MAX >> 63) != 1)               ||                          \
          ((UINT64_MAX & 0xffffffffU) != 0xffffffffU) )
        #error "dstdint.h: a 64-bit limit is wrong in #if"
    #endif
#endif  // INT64_MAX

#ifdef INT_LEAST8_MAX
    #if ( (INT8_C(127)   != 127)  ||                                          \
          (UINT8_C(255)  != 255)  ||                                          \
          (INT_LEAST8_MIN != -INT_LEAST8_MAX - 1) )
        #error "dstdint.h: an 8-bit constant is wrong in #if"
    #endif
#endif  // INT_LEAST8_MAX

#ifdef INT_LEAST16_MAX
    #if ( (INT16_C(32767)   != 32767)  ||                                     \
          (UINT16_C(65535)  != 65535)  ||                                     \
          (INT_LEAST16_MIN != -INT_LEAST16_MAX - 1) )
        #error "dstdint.h: a 16-bit constant is wrong in #if"
    #endif
#endif  // INT_LEAST16_MAX

#ifdef INT_LEAST32_MAX
    #if ( (INT32_C(2147483647)  != 2147483647)  ||                            \
          (UINT32_C(4294967295) != 4294967295U) ||                            \
          (INT_LEAST32_MIN != -INT_LEAST32_MAX - 1) )
        #error "dstdint.h: a 32-bit constant is wrong in #if"
    #endif
#endif  // INT_LEAST32_MAX

#ifdef INT_LEAST64_MAX
    #if ( ((INT64_C(9223372036854775807) >> 62)   != 1) ||                    \
          ((UINT64_C(18446744073709551615) >> 63) != 1) ||                    \
          (INT_LEAST64_MIN != -INT_LEAST64_MAX - 1) )
        #error "dstdint.h: a 64-bit constant is wrong in #if"
    #endif
#endif  // INT_LEAST64_MAX

#ifdef INTPTR_MAX
    #if ( (INTPTR_MIN != -INTPTR_MAX - 1) ||                                  \
          (UINTPTR_MAX < 65535) )
        #error "dstdint.h: a pointer-holding limit is wrong in #if"
    #endif
#endif  // INTPTR_MAX

#ifdef INTMAX_MAX
    #if ( (INTMAX_MIN != -INTMAX_MAX - 1)     ||                              \
          (INTMAX_MAX < LONG_MAX)             ||                              \
          (UINTMAX_MAX < ULONG_MAX)           ||                              \
          (INTMAX_C(2147483647) != 2147483647) ||                             \
          (UINTMAX_C(4294967295) != 4294967295U) )
        #error "dstdint.h: a greatest-width limit or constant is wrong in #if"
    #endif
#endif  // INTMAX_MAX


/*
Exact-width families
*/
#ifdef INT8_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(int8, int8_t, uint8_t,
                                    INT8_MIN, INT8_MAX, UINT8_MAX);
    D_INTERNAL_STDINT_VERIFY_EXACT(int8, 8, int8_t, UINT8_MAX);
#endif  // INT8_MAX

#ifdef INT16_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(int16, int16_t, uint16_t,
                                    INT16_MIN, INT16_MAX, UINT16_MAX);
    D_INTERNAL_STDINT_VERIFY_EXACT(int16, 16, int16_t, UINT16_MAX);
#endif  // INT16_MAX

#ifdef INT32_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(int32, int32_t, uint32_t,
                                    INT32_MIN, INT32_MAX, UINT32_MAX);
    D_INTERNAL_STDINT_VERIFY_EXACT(int32, 32, int32_t, UINT32_MAX);
#endif  // INT32_MAX

#ifdef INT64_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(int64, int64_t, uint64_t,
                                    INT64_MIN, INT64_MAX, UINT64_MAX);
    D_INTERNAL_STDINT_VERIFY_EXACT(int64, 64, int64_t, UINT64_MAX);
#endif  // INT64_MAX


/*
Minimum-width families
  Each is also the narrowest type of its width: where the exact-width type
exists, the minimum-width type is its size.
*/
#ifdef INT_LEAST8_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(least8, int_least8_t, uint_least8_t,
                                    INT_LEAST8_MIN, INT_LEAST8_MAX,
                                    UINT_LEAST8_MAX);
    D_INTERNAL_STDINT_VERIFY_LEAST(least8, 8, UINT_LEAST8_MAX);
    D_INTERNAL_STDINT_VERIFY_CONSTANTS(least8, int_least8_t, uint_least8_t,
                                       INT8_C(1), UINT8_C(1));
    D_INTERNAL_STDINT_VERIFY(least8_c_values,
                             (INT8_C(127) == 127) && (UINT8_C(255) == 255));

    #ifdef INT8_MAX
        D_INTERNAL_STDINT_VERIFY(least8_narrowest,
                                 sizeof(int_least8_t) == sizeof(int8_t));
    #endif  // INT8_MAX
#endif  // INT_LEAST8_MAX

#ifdef INT_LEAST16_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(least16, int_least16_t, uint_least16_t,
                                    INT_LEAST16_MIN, INT_LEAST16_MAX,
                                    UINT_LEAST16_MAX);
    D_INTERNAL_STDINT_VERIFY_LEAST(least16, 16, UINT_LEAST16_MAX);
    D_INTERNAL_STDINT_VERIFY_CONSTANTS(least16, int_least16_t,
                                       uint_least16_t,
                                       INT16_C(1), UINT16_C(1));
    D_INTERNAL_STDINT_VERIFY(least16_c_values,
                             (INT16_C(32767) == 32767) &&
                             (UINT16_C(65535) == 65535));

    #ifdef INT16_MAX
        D_INTERNAL_STDINT_VERIFY(least16_narrowest,
                                 sizeof(int_least16_t) == sizeof(int16_t));
    #endif  // INT16_MAX
#endif  // INT_LEAST16_MAX

#ifdef INT_LEAST32_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(least32, int_least32_t, uint_least32_t,
                                    INT_LEAST32_MIN, INT_LEAST32_MAX,
                                    UINT_LEAST32_MAX);
    D_INTERNAL_STDINT_VERIFY_LEAST(least32, 32, UINT_LEAST32_MAX);
    D_INTERNAL_STDINT_VERIFY_CONSTANTS(least32, int_least32_t,
                                       uint_least32_t,
                                       INT32_C(1), UINT32_C(1));
    D_INTERNAL_STDINT_VERIFY(least32_c_values,
                             (INT32_C(2147483647) == 2147483647) &&
                             (UINT32_C(4294967295) == 4294967295U));

    #ifdef INT32_MAX
        D_INTERNAL_STDINT_VERIFY(least32_narrowest,
                                 sizeof(int_least32_t) == sizeof(int32_t));
    #endif  // INT32_MAX
#endif  // INT_LEAST32_MAX

#ifdef INT_LEAST64_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(least64, int_least64_t, uint_least64_t,
                                    INT_LEAST64_MIN, INT_LEAST64_MAX,
                                    UINT_LEAST64_MAX);
    D_INTERNAL_STDINT_VERIFY_LEAST(least64, 64, UINT_LEAST64_MAX);
    D_INTERNAL_STDINT_VERIFY_CONSTANTS(least64, int_least64_t,
                                       uint_least64_t,
                                       INT64_C(1), UINT64_C(1));
    D_INTERNAL_STDINT_VERIFY(least64_c_values,
                             (INT64_C(9223372036854775807) ==
                              (int_least64_t)(UINT64_C(18446744073709551615)
                                              >> 1)));

    #ifdef INT64_MAX
        D_INTERNAL_STDINT_VERIFY(least64_narrowest,
                                 sizeof(int_least64_t) == sizeof(int64_t));
    #endif  // INT64_MAX
#endif  // INT_LEAST64_MAX


/*
Fastest minimum-width families
  The platform's types, or the header's own where it knows which types the
platform's <stdint.h> would declare; their limits may be the header's on
either backend. Checked on the same terms as the minimum-width families;
that the header's own are the right types is Identity's to check, below, and
the harness's (T_DSTDINT_WITH_SYSTEM).
*/
#ifdef INT_FAST8_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(fast8, int_fast8_t, uint_fast8_t,
                                    INT_FAST8_MIN, INT_FAST8_MAX,
                                    UINT_FAST8_MAX);
    D_INTERNAL_STDINT_VERIFY_LEAST(fast8, 8, UINT_FAST8_MAX);
#endif  // INT_FAST8_MAX

#ifdef INT_FAST16_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(fast16, int_fast16_t, uint_fast16_t,
                                    INT_FAST16_MIN, INT_FAST16_MAX,
                                    UINT_FAST16_MAX);
    D_INTERNAL_STDINT_VERIFY_LEAST(fast16, 16, UINT_FAST16_MAX);
#endif  // INT_FAST16_MAX

#ifdef INT_FAST32_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(fast32, int_fast32_t, uint_fast32_t,
                                    INT_FAST32_MIN, INT_FAST32_MAX,
                                    UINT_FAST32_MAX);
    D_INTERNAL_STDINT_VERIFY_LEAST(fast32, 32, UINT_FAST32_MAX);
#endif  // INT_FAST32_MAX

#ifdef INT_FAST64_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(fast64, int_fast64_t, uint_fast64_t,
                                    INT_FAST64_MIN, INT_FAST64_MAX,
                                    UINT_FAST64_MAX);
    D_INTERNAL_STDINT_VERIFY_LEAST(fast64, 64, UINT_FAST64_MAX);
#endif  // INT_FAST64_MAX


/*
Pointer-holding and greatest-width families
  intptr_t is at least as wide as a pointer, and intmax_t at least as wide as
`long` and as the widest minimum-width type, the widest types the build has.
*/
#ifdef INTPTR_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(intptr, intptr_t, uintptr_t,
                                    INTPTR_MIN, INTPTR_MAX, UINTPTR_MAX);
    D_INTERNAL_STDINT_VERIFY(intptr_holds_pointer,
                             (sizeof(intptr_t) >= sizeof(void*)) &&
                             (sizeof(uintptr_t) >= sizeof(void*)));
#endif  // INTPTR_MAX

// derived from <limits.h>, intptr_t is chosen by the pointer's width, so it
// has exactly that width
#if ( (defined(INTPTR_MAX))                                     &&            \
      (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_OWN)  &&            \
      (RE_STD_INTERNAL_STDINT_PREDEFINED == 0) )
    D_INTERNAL_STDINT_VERIFY(intptr_is_pointer_wide,
                             (sizeof(intptr_t) == sizeof(void*)) &&
                             (sizeof(uintptr_t) == sizeof(void*)));
#endif

#ifdef INTMAX_MAX
    D_INTERNAL_STDINT_VERIFY_FAMILY(intmax, intmax_t, uintmax_t,
                                    INTMAX_MIN, INTMAX_MAX, UINTMAX_MAX);
    D_INTERNAL_STDINT_VERIFY_CONSTANTS(intmax, intmax_t, uintmax_t,
                                       INTMAX_C(1), UINTMAX_C(1));
    D_INTERNAL_STDINT_VERIFY(intmax_widest_long,
                             (sizeof(intmax_t) >= sizeof(long)) &&
                             (sizeof(uintmax_t) >= sizeof(unsigned long)));

    #ifdef INT_LEAST64_MAX
        D_INTERNAL_STDINT_VERIFY(intmax_widest_least64,
                                 (sizeof(intmax_t) >=
                                  sizeof(int_least64_t)) &&
                                 (sizeof(uintmax_t) >=
                                  sizeof(uint_least64_t)));
    #endif  // INT_LEAST64_MAX
#endif  // INTMAX_MAX


/*
Pointer width
  Wherever re_std's config.h states the width of a pointer, it is the width
the compiler gives `void*`. djinterp's env is held to the same at the end of
the unit, so that where both state it, they agree.
*/
#if (RE_STD_POINTER_BITS != 0)
    D_INTERNAL_STDINT_VERIFY(re_std_pointer_bits,
                             RE_STD_POINTER_BITS ==
                             sizeof(void*) * CHAR_BIT);
#endif


/*
Limits of other integer types
  size_t's maximum is its largest value, and the limits of ptrdiff_t and
wchar_t are exactly the extremes of those types; each, where the header
defines it, has the type's type after promotion. Those of wint_t and
sig_atomic_t, whose types a C library declares, are at the end of the unit.
  On the platform backend the limits are the platform's, on which the
framework relies, and are held to this. On the header's own, only those it
defined are: one it left out may be defined all the same, by a platform
header it had to include, and need not be readable. mingw-w64's <limits.h>
defines SIZE_MAX, which on 64-bit Windows is a `long long` literal that an
ISO strict C++98 build, having no such type, cannot compile.
*/
#if (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_PLATFORM)
    #define D_INTERNAL_STDINT_HELD_SIZE     1
    #define D_INTERNAL_STDINT_HELD_PTRDIFF  1
    #define D_INTERNAL_STDINT_HELD_WCHAR    1
#else
    #define D_INTERNAL_STDINT_HELD_SIZE     RE_STD_INTERNAL_STDINT_OWN_SIZE
    #define D_INTERNAL_STDINT_HELD_PTRDIFF  RE_STD_INTERNAL_STDINT_OWN_PTRDIFF
    #define D_INTERNAL_STDINT_HELD_WCHAR    RE_STD_INTERNAL_STDINT_OWN_WCHAR
#endif

#if ( (defined(SIZE_MAX)) &&                                                  \
      (D_INTERNAL_STDINT_HELD_SIZE == 1) )
    D_INTERNAL_STDINT_VERIFY(size_max, SIZE_MAX == (size_t)-1);
    D_INTERNAL_STDINT_VERIFY_OWN(size_max_type,
                                 D_INTERNAL_STDINT_HAS_TYPE(SIZE_MAX,
                                                            size_t));
#endif

#if ( (defined(PTRDIFF_MAX)) &&                                               \
      (D_INTERNAL_STDINT_HELD_PTRDIFF == 1) )
    D_INTERNAL_STDINT_VERIFY(ptrdiff_signed, ((ptrdiff_t)-1) < 0);
    D_INTERNAL_STDINT_VERIFY_OTHER(ptrdiff, ptrdiff_t, ptrdiff_t,
                                   PTRDIFF_MIN, PTRDIFF_MAX);
#endif

#if ( (defined(WCHAR_MAX)) &&                                                 \
      (D_INTERNAL_STDINT_HELD_WCHAR == 1) )
    #ifdef __WCHAR_TYPE__
        D_INTERNAL_STDINT_VERIFY_OTHER(wchar, wchar_t, __WCHAR_TYPE__,
                                       WCHAR_MIN, WCHAR_MAX);
    #else
        D_INTERNAL_STDINT_VERIFY_OTHER(wchar, wchar_t, wchar_t,
                                       WCHAR_MIN, WCHAR_MAX);
    #endif  // __WCHAR_TYPE__
#endif

// the limits of sig_atomic_t and wint_t, where the header defined them, are
// the ones it knows by its internal names, which the end of the unit holds
// to the library's types
#if (RE_STD_INTERNAL_STDINT_OWN_SIG_ATOMIC == 1)
    D_INTERNAL_STDINT_VERIFY(sig_atomic_public,
        (SIG_ATOMIC_MAX == RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MAX) &&
        (SIG_ATOMIC_MIN == RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MIN));
#endif

#if (RE_STD_INTERNAL_STDINT_OWN_WINT == 1)
    D_INTERNAL_STDINT_VERIFY(wint_public,
        (WINT_MAX == RE_STD_INTERNAL_STDINT_WINT_MAX) &&
        (WINT_MIN == RE_STD_INTERNAL_STDINT_WINT_MIN));
#endif


/*
Spellings
  On the self-defined path every maximum the compiler predefines must be
spelled in a way the header knows (RE_STD_INTERNAL_STDINT_CLASS), or the
family it describes would go missing without a word.
*/
#if ( (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_OWN) &&             \
      (RE_STD_INTERNAL_STDINT_PREDEFINED == 1) )
    #if ( (defined(__INT8_MAX__))                   &&                        \
          (defined(__UINT8_MAX__))                  &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__INT8_MAX__) == 0)        ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__UINT8_MAX__) == 0) ) )
        #error "dstdint.h: int8_t: a maximum has an unknown spelling"
    #endif

    #if ( (defined(__INT16_MAX__))                  &&                        \
          (defined(__UINT16_MAX__))                 &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__INT16_MAX__) == 0)       ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__UINT16_MAX__) == 0) ) )
        #error "dstdint.h: int16_t: a maximum has an unknown spelling"
    #endif

    #if ( (defined(__INT32_MAX__))                  &&                        \
          (defined(__UINT32_MAX__))                 &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__INT32_MAX__) == 0)       ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__UINT32_MAX__) == 0) ) )
        #error "dstdint.h: int32_t: a maximum has an unknown spelling"
    #endif

    #if ( (defined(__INT64_MAX__))                  &&                        \
          (defined(__UINT64_MAX__))                 &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__INT64_MAX__) == 0)       ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__UINT64_MAX__) == 0) ) )
        #error "dstdint.h: int64_t: a maximum has an unknown spelling"
    #endif

    #if ( (defined(__INT_LEAST8_MAX__))             &&                        \
          (defined(__UINT_LEAST8_MAX__))            &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__INT_LEAST8_MAX__) == 0)  ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__UINT_LEAST8_MAX__) == 0) ) )
        #error "dstdint.h: int_least8_t: a maximum has an unknown spelling"
    #endif

    #if ( (defined(__INT_LEAST16_MAX__))            &&                        \
          (defined(__UINT_LEAST16_MAX__))           &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__INT_LEAST16_MAX__) == 0) ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__UINT_LEAST16_MAX__) == 0) ) )
        #error "dstdint.h: int_least16_t: a maximum has an unknown spelling"
    #endif

    #if ( (defined(__INT_LEAST32_MAX__))            &&                        \
          (defined(__UINT_LEAST32_MAX__))           &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__INT_LEAST32_MAX__) == 0) ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__UINT_LEAST32_MAX__) == 0) ) )
        #error "dstdint.h: int_least32_t: a maximum has an unknown spelling"
    #endif

    #if ( (defined(__INT_LEAST64_MAX__))            &&                        \
          (defined(__UINT_LEAST64_MAX__))           &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__INT_LEAST64_MAX__) == 0) ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__UINT_LEAST64_MAX__) == 0) ) )
        #error "dstdint.h: int_least64_t: a maximum has an unknown spelling"
    #endif

    #if ( (defined(__INTPTR_MAX__))                 &&                        \
          (defined(__UINTPTR_MAX__))                &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__INTPTR_MAX__) == 0)      ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__UINTPTR_MAX__) == 0) ) )
        #error "dstdint.h: intptr_t: a maximum has an unknown spelling"
    #endif

    #if ( (defined(__INTMAX_MAX__))                 &&                        \
          (defined(__UINTMAX_MAX__))                &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__INTMAX_MAX__) == 0)      ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__UINTMAX_MAX__) == 0) ) )
        #error "dstdint.h: intmax_t: a maximum has an unknown spelling"
    #endif

    #if ( (defined(__PTRDIFF_MAX__))                &&                        \
          (defined(__SIZE_MAX__))                   &&                        \
          ( (RE_STD_INTERNAL_STDINT_CLASS(__PTRDIFF_MAX__) == 0)     ||       \
            (RE_STD_INTERNAL_STDINT_CLASS(__SIZE_MAX__) == 0) ) )
        #error "dstdint.h: ptrdiff_t, size_t: a maximum has an unknown spelling"
    #endif
#endif  // self-defined path with predefined types


/*
Identity
  On the self-defined path each type must be the very type the compiler
predefines for it, and a minimum-width type the exact-width type of its width
wherever that exists: not merely one of the same size and signedness, but the
same type, so that overloads, name mangling and printf formats agree with the
platform's own <stdint.h>. That the platform's header agrees in turn is the
harness's to check (T_DSTDINT_WITH_SYSTEM). C++ compares the types with a
class template, and C11 with a generic selection; C99 has no way to, and
relies on the checks above.
*/
#if (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_OWN)
    #if (RE_STD_LANG_IS_CPP == 1)
        template<typename Type,
                 typename Other>
        struct d_internal_stdint_same
        {
            enum { value = 0 };
        };

        template<typename Type>
        struct d_internal_stdint_same<Type, Type>
        {
            enum { value = 1 };
        };

        #define D_INTERNAL_STDINT_SAME(_type, _other)                         \
            (d_internal_stdint_same<_type, _other>::value == 1)
    #elif (D_INTERNAL_STDINT_HAS_GENERIC == 1)
        #define D_INTERNAL_STDINT_SAME(_type, _other)                         \
            _Generic((_type*)0, _other*: 1, default: 0)
    #endif
#endif  // own backend

#if ( (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_OWN) &&             \
      (RE_STD_INTERNAL_STDINT_PREDEFINED == 1) )

    #ifdef D_INTERNAL_STDINT_SAME
        #ifdef INT8_MAX
            D_INTERNAL_STDINT_VERIFY(int8_identity,
                D_INTERNAL_STDINT_SAME(int8_t, __INT8_TYPE__) &&
                D_INTERNAL_STDINT_SAME(uint8_t, __UINT8_TYPE__));
        #endif  // INT8_MAX

        #ifdef INT16_MAX
            D_INTERNAL_STDINT_VERIFY(int16_identity,
                D_INTERNAL_STDINT_SAME(int16_t, __INT16_TYPE__) &&
                D_INTERNAL_STDINT_SAME(uint16_t, __UINT16_TYPE__));
        #endif  // INT16_MAX

        #ifdef INT32_MAX
            D_INTERNAL_STDINT_VERIFY(int32_identity,
                D_INTERNAL_STDINT_SAME(int32_t, __INT32_TYPE__) &&
                D_INTERNAL_STDINT_SAME(uint32_t, __UINT32_TYPE__));
        #endif  // INT32_MAX

        #ifdef INT64_MAX
            D_INTERNAL_STDINT_VERIFY(int64_identity,
                D_INTERNAL_STDINT_SAME(int64_t, __INT64_TYPE__) &&
                D_INTERNAL_STDINT_SAME(uint64_t, __UINT64_TYPE__));
        #endif  // INT64_MAX

        #ifdef INT_LEAST8_MAX
            #ifdef INT8_MAX
                D_INTERNAL_STDINT_VERIFY(least8_identity,
                    D_INTERNAL_STDINT_SAME(int_least8_t, int8_t) &&
                    D_INTERNAL_STDINT_SAME(uint_least8_t, uint8_t));
            #else
                D_INTERNAL_STDINT_VERIFY(least8_identity,
                    D_INTERNAL_STDINT_SAME(int_least8_t,
                                           __INT_LEAST8_TYPE__) &&
                    D_INTERNAL_STDINT_SAME(uint_least8_t,
                                           __UINT_LEAST8_TYPE__));
            #endif  // INT8_MAX
        #endif  // INT_LEAST8_MAX

        #ifdef INT_LEAST16_MAX
            #ifdef INT16_MAX
                D_INTERNAL_STDINT_VERIFY(least16_identity,
                    D_INTERNAL_STDINT_SAME(int_least16_t, int16_t) &&
                    D_INTERNAL_STDINT_SAME(uint_least16_t, uint16_t));
            #else
                D_INTERNAL_STDINT_VERIFY(least16_identity,
                    D_INTERNAL_STDINT_SAME(int_least16_t,
                                           __INT_LEAST16_TYPE__) &&
                    D_INTERNAL_STDINT_SAME(uint_least16_t,
                                           __UINT_LEAST16_TYPE__));
            #endif  // INT16_MAX
        #endif  // INT_LEAST16_MAX

        #ifdef INT_LEAST32_MAX
            #ifdef INT32_MAX
                D_INTERNAL_STDINT_VERIFY(least32_identity,
                    D_INTERNAL_STDINT_SAME(int_least32_t, int32_t) &&
                    D_INTERNAL_STDINT_SAME(uint_least32_t, uint32_t));
            #else
                D_INTERNAL_STDINT_VERIFY(least32_identity,
                    D_INTERNAL_STDINT_SAME(int_least32_t,
                                           __INT_LEAST32_TYPE__) &&
                    D_INTERNAL_STDINT_SAME(uint_least32_t,
                                           __UINT_LEAST32_TYPE__));
            #endif  // INT32_MAX
        #endif  // INT_LEAST32_MAX

        #ifdef INT_LEAST64_MAX
            #ifdef INT64_MAX
                D_INTERNAL_STDINT_VERIFY(least64_identity,
                    D_INTERNAL_STDINT_SAME(int_least64_t, int64_t) &&
                    D_INTERNAL_STDINT_SAME(uint_least64_t, uint64_t));
            #else
                D_INTERNAL_STDINT_VERIFY(least64_identity,
                    D_INTERNAL_STDINT_SAME(int_least64_t,
                                           __INT_LEAST64_TYPE__) &&
                    D_INTERNAL_STDINT_SAME(uint_least64_t,
                                           __UINT_LEAST64_TYPE__));
            #endif  // INT64_MAX
        #endif  // INT_LEAST64_MAX

        //   but where the compiler is overruled by the platform's choice
        // (dstdint.h, 2.2.2): 32-bit MIPS Linux, whose intptr_t is `int`
        #if ( (defined(INTPTR_MAX)) &&                                        \
              (RE_STD_INTERNAL_STDINT_PTR_OVERRULED == 1) )
            D_INTERNAL_STDINT_VERIFY(intptr_identity,
                D_INTERNAL_STDINT_SAME(intptr_t, int) &&
                D_INTERNAL_STDINT_SAME(uintptr_t, unsigned int));
        #elif defined(INTPTR_MAX)
            D_INTERNAL_STDINT_VERIFY(intptr_identity,
                D_INTERNAL_STDINT_SAME(intptr_t, __INTPTR_TYPE__) &&
                D_INTERNAL_STDINT_SAME(uintptr_t, __UINTPTR_TYPE__));
        #endif

        #ifdef INTMAX_MAX
            D_INTERNAL_STDINT_VERIFY(intmax_identity,
                D_INTERNAL_STDINT_SAME(intmax_t, __INTMAX_TYPE__) &&
                D_INTERNAL_STDINT_SAME(uintmax_t, __UINTMAX_TYPE__));
        #endif  // INTMAX_MAX
    #endif  // D_INTERNAL_STDINT_SAME

#endif  // self-defined path with predefined types

/*
Identity of the fastest minimum-width types
  These have no ABI to be checked against, only the <stdint.h> the header
took them from (dstdint.h, 2.4): GCC's own declares the types GCC predefines,
and Clang's own the minimum-width types. Where a C library's row gave them,
the library's header is the judge, which the harness includes
(T_DSTDINT_WITH_SYSTEM); but GCC is configured for its target's C library, so
there its predefined types must be the row's too, and a row that GCC
contradicts on any target this unit is compiled for fails here. (Not where
the types were derived from <limits.h>, which may choose `int` where the
platform has `long`.)
*/
#if ( (defined(__GNUC__))            &&                                       \
      (!defined(__clang__))          &&                                       \
      (!defined(__INTEL_COMPILER))   &&                                       \
      (RE_STD_INTERNAL_STDINT_PREDEFINED == 1) )
    #define D_INTERNAL_STDINT_VERIFY_GCC 1
#else
    #define D_INTERNAL_STDINT_VERIFY_GCC 0
#endif

#if ( (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_OWN) &&             \
      (defined(D_INTERNAL_STDINT_SAME)) )
    #ifdef INT_FAST8_MAX
        #if (RE_STD_INTERNAL_STDINT_FAST_FROM == 2)
            D_INTERNAL_STDINT_VERIFY(fast8_identity,
                D_INTERNAL_STDINT_SAME(int_fast8_t, int_least8_t) &&
                D_INTERNAL_STDINT_SAME(uint_fast8_t, uint_least8_t));
        #elif ( (RE_STD_INTERNAL_STDINT_FAST_FROM == 1) ||                    \
                (D_INTERNAL_STDINT_VERIFY_GCC == 1) )
            D_INTERNAL_STDINT_VERIFY(fast8_identity,
                D_INTERNAL_STDINT_SAME(int_fast8_t,
                                       __INT_FAST8_TYPE__) &&
                D_INTERNAL_STDINT_SAME(uint_fast8_t,
                                       __UINT_FAST8_TYPE__));
        #endif
    #endif  // INT_FAST8_MAX

    #ifdef INT_FAST16_MAX
        #if (RE_STD_INTERNAL_STDINT_FAST_FROM == 2)
            D_INTERNAL_STDINT_VERIFY(fast16_identity,
                D_INTERNAL_STDINT_SAME(int_fast16_t, int_least16_t) &&
                D_INTERNAL_STDINT_SAME(uint_fast16_t, uint_least16_t));
        #elif ( (RE_STD_INTERNAL_STDINT_FAST_FROM == 1) ||                    \
                (D_INTERNAL_STDINT_VERIFY_GCC == 1) )
            D_INTERNAL_STDINT_VERIFY(fast16_identity,
                D_INTERNAL_STDINT_SAME(int_fast16_t,
                                       __INT_FAST16_TYPE__) &&
                D_INTERNAL_STDINT_SAME(uint_fast16_t,
                                       __UINT_FAST16_TYPE__));
        #endif
    #endif  // INT_FAST16_MAX

    #ifdef INT_FAST32_MAX
        #if (RE_STD_INTERNAL_STDINT_FAST_FROM == 2)
            D_INTERNAL_STDINT_VERIFY(fast32_identity,
                D_INTERNAL_STDINT_SAME(int_fast32_t, int_least32_t) &&
                D_INTERNAL_STDINT_SAME(uint_fast32_t, uint_least32_t));
        #elif ( (RE_STD_INTERNAL_STDINT_FAST_FROM == 1) ||                    \
                (D_INTERNAL_STDINT_VERIFY_GCC == 1) )
            D_INTERNAL_STDINT_VERIFY(fast32_identity,
                D_INTERNAL_STDINT_SAME(int_fast32_t,
                                       __INT_FAST32_TYPE__) &&
                D_INTERNAL_STDINT_SAME(uint_fast32_t,
                                       __UINT_FAST32_TYPE__));
        #endif
    #endif  // INT_FAST32_MAX

    #ifdef INT_FAST64_MAX
        #if (RE_STD_INTERNAL_STDINT_FAST_FROM == 2)
            D_INTERNAL_STDINT_VERIFY(fast64_identity,
                D_INTERNAL_STDINT_SAME(int_fast64_t, int_least64_t) &&
                D_INTERNAL_STDINT_SAME(uint_fast64_t, uint_least64_t));
        #elif ( (RE_STD_INTERNAL_STDINT_FAST_FROM == 1) ||                    \
                (D_INTERNAL_STDINT_VERIFY_GCC == 1) )
            D_INTERNAL_STDINT_VERIFY(fast64_identity,
                D_INTERNAL_STDINT_SAME(int_fast64_t,
                                       __INT_FAST64_TYPE__) &&
                D_INTERNAL_STDINT_SAME(uint_fast64_t,
                                       __UINT_FAST64_TYPE__));
        #endif
    #endif  // INT_FAST64_MAX
#endif  // own backend, with a way to compare types

/*
Formats, against the compiler's own
  Clang predefines the conversion specifier of each <stdint.h> type
(__INT8_FMTd__ is "hhd", __UINT64_FMTX__ "lX" or "llX", ...). Where the format
macros are the header's own -- on its own backend, and on the platform one
where Clang's <stdint.h> came without an <inttypes.h> -- each must be,
character for character, what the compiler says: every scanf format, and
every printf format of a type the integer promotions leave alone. (For a
narrower type the header's printf format names the promoted type, `d` where
Clang says `hhd`; both print it rightly.) The header infers each length
modifier from the spelling of a maximum; this holds the inference to the
compiler's own answer on every target the unit is compiled for. It needs
C++11, to compare strings in a constant expression; GCC predefines no such
macros, and there the harness's -Wformat is the check.
  A minimum-width family is compared with the macros of the exact-width
family of its width wherever that exists, since that is the type the header
gives it, and Clang's own <stdint.h> too: on 64-bit OpenBSD Clang predefines
`long` and "ld" for int_least64_t, and declares it `long long`. The fastest
minimum-width families are compared, on the same terms, only where their
types are Clang's own choice.
*/
#if ( (RE_STD_LANG_IS_CPP == 1)         &&                                    \
      (RE_STD_LANG_IS_CPP11_OR_HIGHER)  &&                                    \
      (defined(__clang__))              &&                                    \
      (defined(__INT32_FMTd__))         &&                                    \
      ( (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_OWN) ||           \
        (RE_STD_INTERNAL_STDINT_FAST_HEADER == 2) ) )

    constexpr bool
    d_internal_stdint_same_text(
        const char* _text,
        const char* _other
    )
    {
        return ( (*_text == *_other) &&
                 ( (*_text == '\0') ||
                   d_internal_stdint_same_text(_text + 1, _other + 1) ) );
    }

    /*
    D_INTERNAL_STDINT_VERIFY_FORMATS
      The format checks of the family named `_f`, whose format macros end in
    `_sfx` and whose signed type is `_s`, against the compiler's macros, which
    begin `_cs` for the signed type and `_cu` for the unsigned one.
    */
    #define D_INTERNAL_STDINT_VERIFY_FORMATS(_f, _sfx, _s, _cs, _cu)          \
        static_assert(                                                        \
            (!D_INTERNAL_STDINT_SAME_TYPE(+(_s)0, (_s)0)) ||                  \
            ( d_internal_stdint_same_text(PRId##_sfx, _cs##FMTd__) &&         \
              d_internal_stdint_same_text(PRIi##_sfx, _cs##FMTi__) &&         \
              d_internal_stdint_same_text(PRIo##_sfx, _cu##FMTo__) &&         \
              d_internal_stdint_same_text(PRIu##_sfx, _cu##FMTu__) &&         \
              d_internal_stdint_same_text(PRIx##_sfx, _cu##FMTx__) &&         \
              d_internal_stdint_same_text(PRIX##_sfx, _cu##FMTX__) ),         \
            "dstdint.h: a printf format of the " #_f " family is not the "    \
            "compiler's");                                                    \
        static_assert(                                                        \
            d_internal_stdint_same_text(SCNd##_sfx, _cs##FMTd__) &&           \
            d_internal_stdint_same_text(SCNi##_sfx, _cs##FMTi__) &&           \
            d_internal_stdint_same_text(SCNo##_sfx, _cu##FMTo__) &&           \
            d_internal_stdint_same_text(SCNu##_sfx, _cu##FMTu__) &&           \
            d_internal_stdint_same_text(SCNx##_sfx, _cu##FMTx__),             \
            "dstdint.h: a scanf format of the " #_f " family is not the "     \
            "compiler's")

    #if ( (defined(INT8_MAX)) &&                                              \
          (defined(__INT8_FMTd__)) )
        D_INTERNAL_STDINT_VERIFY_FORMATS(int8, 8, int8_t,
                                         __INT8_, __UINT8_);
    #endif

    #if ( (defined(INT16_MAX)) &&                                             \
          (defined(__INT16_FMTd__)) )
        D_INTERNAL_STDINT_VERIFY_FORMATS(int16, 16, int16_t,
                                         __INT16_, __UINT16_);
    #endif

    #if ( (defined(INT32_MAX)) &&                                             \
          (defined(__INT32_FMTd__)) )
        D_INTERNAL_STDINT_VERIFY_FORMATS(int32, 32, int32_t,
                                         __INT32_, __UINT32_);
    #endif

    #if ( (defined(INT64_MAX)) &&                                             \
          (defined(__INT64_FMTd__)) )
        D_INTERNAL_STDINT_VERIFY_FORMATS(int64, 64, int64_t,
                                         __INT64_, __UINT64_);
    #endif

    #ifdef INT_LEAST8_MAX
        #if ( (defined(INT8_MAX)) &&                                          \
              (defined(__INT8_FMTd__)) )
            D_INTERNAL_STDINT_VERIFY_FORMATS(least8, LEAST8, int_least8_t,
                                             __INT8_, __UINT8_);
        #elif defined(__INT_LEAST8_FMTd__)
            D_INTERNAL_STDINT_VERIFY_FORMATS(least8, LEAST8, int_least8_t,
                                             __INT_LEAST8_, __UINT_LEAST8_);
        #endif
    #endif  // INT_LEAST8_MAX

    #ifdef INT_LEAST16_MAX
        #if ( (defined(INT16_MAX)) &&                                         \
              (defined(__INT16_FMTd__)) )
            D_INTERNAL_STDINT_VERIFY_FORMATS(least16, LEAST16, int_least16_t,
                                             __INT16_, __UINT16_);
        #elif defined(__INT_LEAST16_FMTd__)
            D_INTERNAL_STDINT_VERIFY_FORMATS(least16, LEAST16, int_least16_t,
                                             __INT_LEAST16_, __UINT_LEAST16_);
        #endif
    #endif  // INT_LEAST16_MAX

    #ifdef INT_LEAST32_MAX
        #if ( (defined(INT32_MAX)) &&                                         \
              (defined(__INT32_FMTd__)) )
            D_INTERNAL_STDINT_VERIFY_FORMATS(least32, LEAST32, int_least32_t,
                                             __INT32_, __UINT32_);
        #elif defined(__INT_LEAST32_FMTd__)
            D_INTERNAL_STDINT_VERIFY_FORMATS(least32, LEAST32, int_least32_t,
                                             __INT_LEAST32_, __UINT_LEAST32_);
        #endif
    #endif  // INT_LEAST32_MAX

    #ifdef INT_LEAST64_MAX
        #if ( (defined(INT64_MAX)) &&                                         \
              (defined(__INT64_FMTd__)) )
            D_INTERNAL_STDINT_VERIFY_FORMATS(least64, LEAST64, int_least64_t,
                                             __INT64_, __UINT64_);
        #elif defined(__INT_LEAST64_FMTd__)
            D_INTERNAL_STDINT_VERIFY_FORMATS(least64, LEAST64, int_least64_t,
                                             __INT_LEAST64_, __UINT_LEAST64_);
        #endif
    #endif  // INT_LEAST64_MAX

    //   not where the compiler's intptr_t is overruled (dstdint.h, 2.2.2):
    // its formats are then for a type the platform does not use
    #if ( (defined(INTPTR_MAX))                         &&                    \
          (defined(__INTPTR_FMTd__))                    &&                    \
          (RE_STD_INTERNAL_STDINT_PTR_OVERRULED == 0) )
        D_INTERNAL_STDINT_VERIFY_FORMATS(intptr, PTR, intptr_t,
                                         __INTPTR_, __UINTPTR_);
    #endif

    #if ( (defined(INTMAX_MAX)) &&                                            \
          (defined(__INTMAX_FMTd__)) )
        D_INTERNAL_STDINT_VERIFY_FORMATS(intmax, MAX, intmax_t,
                                         __INTMAX_, __UINTMAX_);
    #endif

    #if (RE_STD_INTERNAL_STDINT_FAST_FROM == 2)
        #ifdef INT_FAST8_MAX
            #if ( (defined(INT8_MAX)) &&                                      \
                  (defined(__INT8_FMTd__)) )
                D_INTERNAL_STDINT_VERIFY_FORMATS(fast8, FAST8, int_fast8_t,
                                                 __INT8_, __UINT8_);
            #elif defined(__INT_LEAST8_FMTd__)
                D_INTERNAL_STDINT_VERIFY_FORMATS(fast8, FAST8, int_fast8_t,
                                                 __INT_LEAST8_, __UINT_LEAST8_);
            #endif
        #endif  // INT_FAST8_MAX

        #ifdef INT_FAST16_MAX
            #if ( (defined(INT16_MAX)) &&                                     \
                  (defined(__INT16_FMTd__)) )
                D_INTERNAL_STDINT_VERIFY_FORMATS(fast16, FAST16, int_fast16_t,
                                                 __INT16_, __UINT16_);
            #elif defined(__INT_LEAST16_FMTd__)
                D_INTERNAL_STDINT_VERIFY_FORMATS(
                    fast16, FAST16, int_fast16_t,
                    __INT_LEAST16_, __UINT_LEAST16_);
            #endif
        #endif  // INT_FAST16_MAX

        #ifdef INT_FAST32_MAX
            #if ( (defined(INT32_MAX)) &&                                     \
                  (defined(__INT32_FMTd__)) )
                D_INTERNAL_STDINT_VERIFY_FORMATS(fast32, FAST32, int_fast32_t,
                                                 __INT32_, __UINT32_);
            #elif defined(__INT_LEAST32_FMTd__)
                D_INTERNAL_STDINT_VERIFY_FORMATS(
                    fast32, FAST32, int_fast32_t,
                    __INT_LEAST32_, __UINT_LEAST32_);
            #endif
        #endif  // INT_FAST32_MAX

        #ifdef INT_FAST64_MAX
            #if ( (defined(INT64_MAX)) &&                                     \
                  (defined(__INT64_FMTd__)) )
                D_INTERNAL_STDINT_VERIFY_FORMATS(fast64, FAST64, int_fast64_t,
                                                 __INT64_, __UINT64_);
            #elif defined(__INT_LEAST64_FMTd__)
                D_INTERNAL_STDINT_VERIFY_FORMATS(
                    fast64, FAST64, int_fast64_t,
                    __INT_LEAST64_, __UINT_LEAST64_);
            #endif
        #endif  // INT_FAST64_MAX
    #endif  // the fast types are Clang's own choice

#endif  // C++11 and Clang, with the header's own formats


/*
Pointer width, by djinterp's env
  Wherever env states the width of a pointer, it is the width the compiler
gives `void*`, as re_std's is above. It is checked under automatic detection
only: a build that configures the architecture by hand may describe a target
that is not the compiler's.
*/
// djinterp
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_ARCH_POINTER_BITS,
                                               // D_CFG_IS_ON,
                                               // D_CFG_ENV_ARCH_ENABLED

#if ( (D_CFG_IS_ON(D_CFG_ENV_ARCH_ENABLED)) &&                                \
      (D_ENV_ARCH_POINTER_BITS != 0) )
    D_INTERNAL_STDINT_VERIFY(env_pointer_bits,
                             D_ENV_ARCH_POINTER_BITS ==
                             sizeof(void*) * CHAR_BIT);
#endif


/*
The C library's types
  sig_atomic_t and wint_t are a C library's to declare, in <signal.h> and
<wchar.h>. Where their limits are the header's own they are held to those
declarations: exactly the extremes of the type the library declares, with
its type after promotion. That is what finds a library whose choice is not
the compiler's, as FreeBSD's `long` sig_atomic_t was found. The header's
internal names for the limits are read, not the public ones, which the two
headers included here may define anew (wasi-libc's <wchar.h> does).
  Limits that the platform's <stdint.h> defined are the platform's word
about its own types, and are not held to it here. Two that are wrong, each
compiled against the platform's headers: FreeBSD's powerpc64 ports give
SIG_ATOMIC_MAX the value of INT64_MAX for an `int` sig_atomic_t, and
wasi-libc gives WINT_MAX that of UINT32_MAX for an `int` wint_t. Neither is
a reason for djinterp not to build, and no part of the framework reads
either limit.
  The two headers are included here, after every other check, and not at
the top with the rest: a platform's headers may define names of <stdint.h>
themselves, on either backend -- FreeBSD's <wchar.h> defines WCHAR_MIN and
WCHAR_MAX anew, and wasi-libc's headers include its <stdint.h> whole -- and
everything above them is about the header's definitions, not theirs.
*/
#if (D_INTERNAL_STDINT_VERIFY_SIGNAL == 1)
    // std
    #include <signal.h>  // sig_atomic_t

    #if (RE_STD_INTERNAL_STDINT_OWN_SIG_ATOMIC == 1)
        D_INTERNAL_STDINT_VERIFY_OTHER(
            sig_atomic, sig_atomic_t, sig_atomic_t,
            RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MIN,
            RE_STD_INTERNAL_STDINT_SIG_ATOMIC_MAX);
    #endif
#endif  // D_INTERNAL_STDINT_VERIFY_SIGNAL == 1

#if (D_INTERNAL_STDINT_VERIFY_WCHAR == 1)
    // std
    #include <wchar.h>   // wint_t

    #if (RE_STD_INTERNAL_STDINT_OWN_WINT == 1)
        D_INTERNAL_STDINT_VERIFY_OTHER(wint, wint_t, wint_t,
                                       RE_STD_INTERNAL_STDINT_WINT_MIN,
                                       RE_STD_INTERNAL_STDINT_WINT_MAX);
    #endif
#endif  // D_INTERNAL_STDINT_VERIFY_WCHAR == 1
