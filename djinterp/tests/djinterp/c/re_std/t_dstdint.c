/*******************************************************************************
* djinterp [c]                                                       t_dstdint.c
*
*   Conformance harness for dstdint.h. Builds as C (C99 and later) and as C++
* (C++98 and later), on either backend -- the platform's headers or the
* header's own definitions -- and checks what the compiler cannot check
* alone: that every printf format prints its family's limits as the digits
* the family's width calls for, that every scanf format reads those digits
* back, and that a pointer survives a round trip through intptr_t and
* uintptr_t. It reports the backend and which families are present.
*   Build with -Wformat=2 -Werror as well: the compiler then checks every
* format against the type of its argument, which catches a format macro that
* names the wrong length modifier even where the digits come out right.
*   The type, limit and constant checks are dstdint.c's, compiled alongside.
* Defining T_DSTDINT_WITH_SYSTEM adds one more, at the end of this file: the
* platform's own <stdint.h> and <inttypes.h> are included after the header,
* and each typedef they make must redeclare one of the header's as the same
* type. That needs C11 or C++, since C99 rejects even an identical typedef.
*   Defining T_DSTDINT_FREESTANDING instead builds a compile-only check for a
* target without a C library, as for a cross compiler: every format is passed
* with an argument of its type to functions declared with GCC's format
* attribute, so -Wformat -Werror still checks each one against that target's
* types.
*
*   Build (from the repo root), at any language level from C99 or C++98:
*     cc  -std=c99   -Wall -Wextra -Wformat=2 -Werror                         \
*         src/djinterp/c/re_std/dstdint.c tests/djinterp/c/re_std/t_dstdint.c \
*         -o t_dstdint
*     c++ -std=c++98 -pedantic-errors -DRE_STD_CFG_ISO_STRICT=1                \
*         -Wall -Wextra -Wformat=2 -Werror -x c++                             \
*         src/djinterp/c/re_std/dstdint.c tests/djinterp/c/re_std/t_dstdint.c \
*         -o t_dstdint
*   and, for the header's own definitions in C:
*     cc  -std=c11 -DRE_STD_CFG_STDINT_BACKEND=RE_STD_CFG_STDINT_BACKEND_OWN \
*         ...
*   and, for a cross target:
*     clang --target=arm-none-eabi -ffreestanding -nostdlibinc -fsyntax-only  \
*           -Wformat -Werror -DT_DSTDINT_FREESTANDING                         \
*           tests/djinterp/c/re_std/t_dstdint.c
*   ci/check_dstdint.sh runs the whole matrix.
*
*
* path:      /tests/djinterp/c/re_std/t_dstdint.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.02
*******************************************************************************/
// std
#include <limits.h>  // CHAR_BIT
#include <stddef.h>  // size_t, NULL
#ifndef T_DSTDINT_FREESTANDING
    #include <stdio.h>   // printf, sprintf, sscanf
    #include <string.h>  // strcmp
#endif  // T_DSTDINT_FREESTANDING
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // module under test
// djinterp
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_USING_CPP,
                                               // D_ENV_LANG_C_STANDARD


// <stdio.h> and <string.h> come before the header. Where they bring the
// platform's <stdint.h> with them, as wasi-libc's do, an ISO strict C++98
// build is handed families whose types it cannot spell, and every use of one
// below would be an error the header could not have prevented. Said once
#if ( (!defined(T_DSTDINT_FREESTANDING))                            &&        \
      (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_OWN)      &&        \
      ( ( (defined(INT64_MAX))       &&                                       \
          (RE_STD_INTERNAL_STDINT_64_OK == 0) )      ||                       \
        ( (defined(INT_LEAST64_MAX)) &&                                       \
          (RE_STD_INTERNAL_STDINT_LEAST64_OK == 0) ) ||                       \
        ( (defined(INTPTR_MAX))      &&                                       \
          (RE_STD_INTERNAL_STDINT_PTR_OK == 0) )     ||                       \
        ( (defined(INTMAX_MAX))      &&                                       \
          (RE_STD_INTERNAL_STDINT_MAX_OK == 0) ) ) )
    #error "t_dstdint.c: <stdio.h> declared types this build cannot spell"
#endif

// the 8-bit families' scanf formats use `hh`, which C++ before C++11 lacks
#if ( (D_ENV_LANG_USING_CPP == 0) ||                                          \
      (D_ENV_LANG_IS_CPP11_OR_HIGHER) )
    #define T_SCANF_8 1
#else
    #define T_SCANF_8 0
#endif


#ifndef T_DSTDINT_FREESTANDING

static int fails  = 0;
static int checks = 0;


/*
t_check
  Counts one check, and reports it by `_what` if `_ok` is 0.
*/
static void
t_check(
    const char* _what,
    int         _ok
)
{
    ++checks;

    if (!_ok)
    {
        printf("  FAIL %s\n", _what);
        ++fails;
    }

    return;
}

/*
t_check_text
  Counts one check, and reports it by `_what` if the text `_got` is not the
text `_want`.
*/
static void
t_check_text(
    const char* _what,
    const char* _got,
    const char* _want
)
{
    ++checks;

    if (strcmp(_got, _want) != 0)
    {
        printf("  FAIL %s: got \"%s\", want \"%s\"\n", _what, _got, _want);
        ++fails;
    }

    return;
}


/*
t_digits
  The limits of a family of one width, as its formats must print them: the
signed minimum and maximum in decimal, and the unsigned maximum in decimal,
octal and both cases of hexadecimal.
*/
typedef struct t_digits
{
    const char* smin;
    const char* smax;
    const char* umax;
    const char* omax;
    const char* xmax;
    const char* xmax_upper;
} t_digits;

/*
t_digits_for
  The digits for a family `_bits` wide, or NULL for a width with no entry.
*/
static const t_digits*
t_digits_for(
    size_t _bits
)
{
    static const t_digits digits_8 =
    {
        "-128",
        "127",
        "255",
        "377",
        "ff",
        "FF"
    };
    static const t_digits digits_16 =
    {
        "-32768",
        "32767",
        "65535",
        "177777",
        "ffff",
        "FFFF"
    };
    static const t_digits digits_32 =
    {
        "-2147483648",
        "2147483647",
        "4294967295",
        "37777777777",
        "ffffffff",
        "FFFFFFFF"
    };
    static const t_digits digits_64 =
    {
        "-9223372036854775808",
        "9223372036854775807",
        "18446744073709551615",
        "1777777777777777777777",
        "ffffffffffffffff",
        "FFFFFFFFFFFFFFFF"
    };

    switch (_bits)
    {
        case 8:
            return &digits_8;

        case 16:
            return &digits_16;

        case 32:
            return &digits_32;

        case 64:
            return &digits_64;

        default:
            break;
    }

    return NULL;
}


/*
T_PRINTF
  Prints the limits of the family whose formats end in `_sfx` -- its types `_s`
and `_u`, its limits `_smin`, `_smax` and `_umax` -- with each of its printf
formats, and compares each result with the digits for the family's width.
*/
#define T_PRINTF(_sfx, _s, _u, _smin, _smax, _umax)                           \
    do                                                                        \
    {                                                                         \
        const t_digits* digits = t_digits_for(sizeof(_s) * CHAR_BIT);         \
        char            text[64];                                             \
                                                                              \
        t_check("PRI*" #_sfx ": digits for the width", digits != NULL);       \
                                                                              \
        if (digits != NULL)                                                   \
        {                                                                     \
            sprintf(text, "%" PRId##_sfx, (_s)(_smin));                       \
            t_check_text("PRId" #_sfx " " #_smin, text, digits->smin);        \
            sprintf(text, "%" PRId##_sfx, (_s)(_smax));                       \
            t_check_text("PRId" #_sfx " " #_smax, text, digits->smax);        \
            sprintf(text, "%" PRIi##_sfx, (_s)(_smin));                       \
            t_check_text("PRIi" #_sfx " " #_smin, text, digits->smin);        \
            sprintf(text, "%" PRIu##_sfx, (_u)(_umax));                       \
            t_check_text("PRIu" #_sfx " " #_umax, text, digits->umax);        \
            sprintf(text, "%" PRIo##_sfx, (_u)(_umax));                       \
            t_check_text("PRIo" #_sfx " " #_umax, text, digits->omax);        \
            sprintf(text, "%" PRIx##_sfx, (_u)(_umax));                       \
            t_check_text("PRIx" #_sfx " " #_umax, text, digits->xmax);        \
            sprintf(text, "%" PRIX##_sfx, (_u)(_umax));                       \
            t_check_text("PRIX" #_sfx " " #_umax, text,                       \
                         digits->xmax_upper);                                 \
        }                                                                     \
    } while (0)

/*
T_SCANF
  Reads the digits for the width of the family whose formats end in `_sfx`
back with each of its scanf formats, and compares each result with the limit
the digits spell.
*/
#define T_SCANF(_sfx, _s, _u, _smin, _smax, _umax)                            \
    do                                                                        \
    {                                                                         \
        const t_digits* digits = t_digits_for(sizeof(_s) * CHAR_BIT);         \
        _s              value  = 0;                                           \
        _u              uvalue = 0;                                           \
                                                                              \
        if (digits != NULL)                                                   \
        {                                                                     \
            t_check("SCNd" #_sfx " " #_smin,                                  \
                    (sscanf(digits->smin, "%" SCNd##_sfx, &value) == 1) &&    \
                    (value == (_s)(_smin)));                                  \
            t_check("SCNi" #_sfx " " #_smax,                                  \
                    (sscanf(digits->smax, "%" SCNi##_sfx, &value) == 1) &&    \
                    (value == (_s)(_smax)));                                  \
            t_check("SCNu" #_sfx " " #_umax,                                  \
                    (sscanf(digits->umax, "%" SCNu##_sfx, &uvalue) == 1) &&   \
                    (uvalue == (_u)(_umax)));                                 \
            t_check("SCNo" #_sfx " " #_umax,                                  \
                    (sscanf(digits->omax, "%" SCNo##_sfx, &uvalue) == 1) &&   \
                    (uvalue == (_u)(_umax)));                                 \
            t_check("SCNx" #_sfx " " #_umax,                                  \
                    (sscanf(digits->xmax, "%" SCNx##_sfx, &uvalue) == 1) &&   \
                    (uvalue == (_u)(_umax)));                                 \
        }                                                                     \
    } while (0)


/*
T_OTHER_*
  1 when a limit of another integer type is to be reported as present where
it is defined: on the platform backend always. On the header's own, if the
header defined it; and SIZE_MAX and PTRDIFF_MAX also where a platform header
did and the build can read the type the compiler names. mingw-w64's
<limits.h> defines SIZE_MAX, which this build has on either backend; but on
64-bit Windows it is a `long long` literal, which an ISO strict C++98 build
cannot read and the header rightly did not define, and there it is absent.
*/
#if (RE_STD_STDINT_BACKEND == RE_STD_CFG_STDINT_BACKEND_PLATFORM)
    #define T_OTHER_SIZE        1
    #define T_OTHER_PTRDIFF     1
    #define T_OTHER_WCHAR       1
    #define T_OTHER_WINT        1
    #define T_OTHER_SIG_ATOMIC  1
#else
    #if (RE_STD_INTERNAL_STDINT_OWN_SIZE == 1)
        #define T_OTHER_SIZE    1
    #elif defined(__SIZE_MAX__)
        #if RE_STD_INTERNAL_STDINT_USABLE(__SIZE_MAX__)
            #define T_OTHER_SIZE 1
        #else
            #define T_OTHER_SIZE 0
        #endif
    #else
        #define T_OTHER_SIZE    0
    #endif

    #if (RE_STD_INTERNAL_STDINT_OWN_PTRDIFF == 1)
        #define T_OTHER_PTRDIFF 1
    #elif defined(__PTRDIFF_MAX__)
        #if RE_STD_INTERNAL_STDINT_USABLE(__PTRDIFF_MAX__)
            #define T_OTHER_PTRDIFF 1
        #else
            #define T_OTHER_PTRDIFF 0
        #endif
    #else
        #define T_OTHER_PTRDIFF 0
    #endif

    #define T_OTHER_WCHAR       RE_STD_INTERNAL_STDINT_OWN_WCHAR
    #define T_OTHER_WINT        RE_STD_INTERNAL_STDINT_OWN_WINT
    #define T_OTHER_SIG_ATOMIC  RE_STD_INTERNAL_STDINT_OWN_SIG_ATOMIC
#endif

/*
t_families
  The present families, named in a string the preprocessor builds from their
limit macros.
*/
static const char t_families[] =
#ifdef INT8_MAX
    " int8"
#endif
#ifdef INT16_MAX
    " int16"
#endif
#ifdef INT32_MAX
    " int32"
#endif
#ifdef INT64_MAX
    " int64"
#endif
#ifdef INT_LEAST8_MAX
    " least8"
#endif
#ifdef INT_LEAST16_MAX
    " least16"
#endif
#ifdef INT_LEAST32_MAX
    " least32"
#endif
#ifdef INT_LEAST64_MAX
    " least64"
#endif
#ifdef INTPTR_MAX
    " intptr"
#endif
#ifdef INTMAX_MAX
    " intmax"
#endif
#ifdef INT_FAST8_MAX
    " fast8"
#endif
#ifdef INT_FAST16_MAX
    " fast16"
#endif
#ifdef INT_FAST32_MAX
    " fast32"
#endif
#ifdef INT_FAST64_MAX
    " fast64"
#endif
#if ( (defined(SIZE_MAX)) &&                                                  \
      (T_OTHER_SIZE == 1) )
    " SIZE_MAX"
#endif
#if ( (defined(PTRDIFF_MAX)) &&                                               \
      (T_OTHER_PTRDIFF == 1) )
    " PTRDIFF_*"
#endif
#if ( (defined(WCHAR_MAX)) &&                                                 \
      (T_OTHER_WCHAR == 1) )
    " WCHAR_*"
#endif
#if ( (defined(WINT_MAX)) &&                                                  \
      (T_OTHER_WINT == 1) )
    " WINT_*"
#endif
#if ( (defined(SIG_ATOMIC_MAX)) &&                                            \
      (T_OTHER_SIG_ATOMIC == 1) )
    " SIG_ATOMIC_*"
#endif
    "";


/*
t_exact
  The exact-width families.
*/
static void
t_exact(void)
{
#ifdef INT8_MAX
    T_PRINTF(8, int8_t, uint8_t, INT8_MIN, INT8_MAX, UINT8_MAX);

    #if ( (T_SCANF_8 == 1) &&                                                 \
          (defined(SCNd8)) )
        T_SCANF(8, int8_t, uint8_t, INT8_MIN, INT8_MAX, UINT8_MAX);
    #endif
#endif  // INT8_MAX

#ifdef INT16_MAX
    T_PRINTF(16, int16_t, uint16_t, INT16_MIN, INT16_MAX, UINT16_MAX);
    T_SCANF(16, int16_t, uint16_t, INT16_MIN, INT16_MAX, UINT16_MAX);
#endif  // INT16_MAX

#ifdef INT32_MAX
    T_PRINTF(32, int32_t, uint32_t, INT32_MIN, INT32_MAX, UINT32_MAX);
    T_SCANF(32, int32_t, uint32_t, INT32_MIN, INT32_MAX, UINT32_MAX);
#endif  // INT32_MAX

#ifdef INT64_MAX
    T_PRINTF(64, int64_t, uint64_t, INT64_MIN, INT64_MAX, UINT64_MAX);
    T_SCANF(64, int64_t, uint64_t, INT64_MIN, INT64_MAX, UINT64_MAX);
#endif  // INT64_MAX

    return;
}

/*
t_least
  The minimum-width families.
*/
static void
t_least(void)
{
#ifdef INT_LEAST8_MAX
    T_PRINTF(LEAST8, int_least8_t, uint_least8_t,
             INT_LEAST8_MIN, INT_LEAST8_MAX, UINT_LEAST8_MAX);

    #if ( (T_SCANF_8 == 1) &&                                                 \
          (defined(SCNdLEAST8)) )
        T_SCANF(LEAST8, int_least8_t, uint_least8_t,
                INT_LEAST8_MIN, INT_LEAST8_MAX, UINT_LEAST8_MAX);
    #endif
#endif  // INT_LEAST8_MAX

#ifdef INT_LEAST16_MAX
    T_PRINTF(LEAST16, int_least16_t, uint_least16_t,
             INT_LEAST16_MIN, INT_LEAST16_MAX, UINT_LEAST16_MAX);
    T_SCANF(LEAST16, int_least16_t, uint_least16_t,
            INT_LEAST16_MIN, INT_LEAST16_MAX, UINT_LEAST16_MAX);
#endif  // INT_LEAST16_MAX

#ifdef INT_LEAST32_MAX
    T_PRINTF(LEAST32, int_least32_t, uint_least32_t,
             INT_LEAST32_MIN, INT_LEAST32_MAX, UINT_LEAST32_MAX);
    T_SCANF(LEAST32, int_least32_t, uint_least32_t,
            INT_LEAST32_MIN, INT_LEAST32_MAX, UINT_LEAST32_MAX);
#endif  // INT_LEAST32_MAX

#ifdef INT_LEAST64_MAX
    T_PRINTF(LEAST64, int_least64_t, uint_least64_t,
             INT_LEAST64_MIN, INT_LEAST64_MAX, UINT_LEAST64_MAX);
    T_SCANF(LEAST64, int_least64_t, uint_least64_t,
            INT_LEAST64_MIN, INT_LEAST64_MAX, UINT_LEAST64_MAX);
#endif  // INT_LEAST64_MAX

    return;
}

/*
t_pointer_and_max
  The pointer-holding and greatest-width families, and a pointer's round trip
through each pointer-holding type.
*/
static void
t_pointer_and_max(void)
{
#ifdef INTPTR_MAX
    int   object  = 0;
    void* pointer = &object;

    T_PRINTF(PTR, intptr_t, uintptr_t, INTPTR_MIN, INTPTR_MAX, UINTPTR_MAX);
    T_SCANF(PTR, intptr_t, uintptr_t, INTPTR_MIN, INTPTR_MAX, UINTPTR_MAX);

    t_check("void* -> intptr_t -> void*",
            (void*)(intptr_t)pointer == pointer);
    t_check("void* -> uintptr_t -> void*",
            (void*)(uintptr_t)pointer == pointer);
#endif  // INTPTR_MAX

#ifdef INTMAX_MAX
    T_PRINTF(MAX, intmax_t, uintmax_t, INTMAX_MIN, INTMAX_MAX, UINTMAX_MAX);
    T_SCANF(MAX, intmax_t, uintmax_t, INTMAX_MIN, INTMAX_MAX, UINTMAX_MAX);
#endif  // INTMAX_MAX

    return;
}


/*
t_fast
  The fastest minimum-width families, which only the platform declares.
*/
static void
t_fast(void)
{
#ifdef INT_FAST8_MAX
    T_PRINTF(FAST8, int_fast8_t, uint_fast8_t,
             INT_FAST8_MIN, INT_FAST8_MAX, UINT_FAST8_MAX);

    #if ( (T_SCANF_8 == 1) &&                                                 \
          (defined(SCNdFAST8)) )
        T_SCANF(FAST8, int_fast8_t, uint_fast8_t,
                INT_FAST8_MIN, INT_FAST8_MAX, UINT_FAST8_MAX);
    #endif
#endif  // INT_FAST8_MAX

#ifdef INT_FAST16_MAX
    T_PRINTF(FAST16, int_fast16_t, uint_fast16_t,
             INT_FAST16_MIN, INT_FAST16_MAX, UINT_FAST16_MAX);
    T_SCANF(FAST16, int_fast16_t, uint_fast16_t,
            INT_FAST16_MIN, INT_FAST16_MAX, UINT_FAST16_MAX);
#endif  // INT_FAST16_MAX

#ifdef INT_FAST32_MAX
    T_PRINTF(FAST32, int_fast32_t, uint_fast32_t,
             INT_FAST32_MIN, INT_FAST32_MAX, UINT_FAST32_MAX);
    T_SCANF(FAST32, int_fast32_t, uint_fast32_t,
            INT_FAST32_MIN, INT_FAST32_MAX, UINT_FAST32_MAX);
#endif  // INT_FAST32_MAX

#ifdef INT_FAST64_MAX
    T_PRINTF(FAST64, int_fast64_t, uint_fast64_t,
             INT_FAST64_MIN, INT_FAST64_MAX, UINT_FAST64_MAX);
    T_SCANF(FAST64, int_fast64_t, uint_fast64_t,
            INT_FAST64_MIN, INT_FAST64_MAX, UINT_FAST64_MAX);
#endif  // INT_FAST64_MAX

    return;
}


int
main(void)
{
#if (D_ENV_LANG_USING_CPP == 1)
    printf("t_dstdint: C++ %ld, ", (long)__cplusplus);
#else
    printf("t_dstdint: C %ld, ", (long)D_ENV_LANG_C_STANDARD);
#endif

#if RE_STD_CFG_ISO_STRICT
    printf("ISO strict, ");
#endif

    printf("%s backend\n", RE_STD_STDINT_BACKEND_NAME);
    printf("  present:%s\n", t_families);

    t_exact();
    t_least();
    t_pointer_and_max();
    t_fast();

    printf("  %d checks, %d failed\n", checks, fails);

    return (fails == 0) ? 0 : 1;
}

#else  // T_DSTDINT_FREESTANDING

/*
t_format_printf, t_format_scanf
  Declared and never defined: the format attribute makes the compiler check
each call's format against its arguments, which is all a build without a C
library can check.
  For a MinGW target GCC reads the attribute's `printf` and `scanf` as the
dialect of Microsoft's runtime, which has no `hh`. Where the header's formats
are C's, as in a freestanding build (dstdint.h, 6.2.1), they are held to C's
dialect, which GCC names gnu_printf and gnu_scanf.
*/
#if ( (defined(__MINGW32__))                   &&                             \
      (defined(__GNUC__))                      &&                             \
      (!defined(__clang__))                    &&                             \
      (RE_STD_INTERNAL_STDINT_MS_STDIO == 0) )
    #define T_FORMAT_PRINTF gnu_printf
    #define T_FORMAT_SCANF  gnu_scanf
#else
    #define T_FORMAT_PRINTF printf
    #define T_FORMAT_SCANF  scanf
#endif

int t_format_printf(const char* _format, ...)
    __attribute__((format(T_FORMAT_PRINTF, 1, 2)));
int t_format_scanf(const char* _format, ...)
    __attribute__((format(T_FORMAT_SCANF, 1, 2)));

/*
T_FORMATS
  Passes a value of each type of the family whose formats end in `_sfx` to
each of its printf formats, and a pointer to one to each of its scanf formats.
*/
#define T_FORMATS(_sfx, _s, _u)                                               \
    do                                                                        \
    {                                                                         \
        _s value  = 0;                                                        \
        _u uvalue = 0;                                                        \
                                                                              \
        t_format_printf("%" PRId##_sfx " %" PRIi##_sfx, value, value);        \
        t_format_printf("%" PRIo##_sfx " %" PRIu##_sfx                        \
                        " %" PRIx##_sfx " %" PRIX##_sfx,                      \
                        uvalue, uvalue, uvalue, uvalue);                      \
        t_format_scanf("%" SCNd##_sfx " %" SCNi##_sfx, &value, &value);       \
        t_format_scanf("%" SCNo##_sfx " %" SCNu##_sfx " %" SCNx##_sfx,        \
                       &uvalue, &uvalue, &uvalue);                            \
    } while (0)

/*
T_PRINTF_FORMATS
  As T_FORMATS, for printf formats alone: the 8-bit families, below C99 and
C++11, where they have no scanf formats.
*/
#define T_PRINTF_FORMATS(_sfx, _s, _u)                                        \
    do                                                                        \
    {                                                                         \
        _s value  = 0;                                                        \
        _u uvalue = 0;                                                        \
                                                                              \
        t_format_printf("%" PRId##_sfx " %" PRIi##_sfx, value, value);        \
        t_format_printf("%" PRIo##_sfx " %" PRIu##_sfx                        \
                        " %" PRIx##_sfx " %" PRIX##_sfx,                      \
                        uvalue, uvalue, uvalue, uvalue);                      \
    } while (0)

void t_formats(void);

/*
t_formats
  Every format of every present family, once.
*/
void
t_formats(void)
{
#ifdef INT8_MAX
    #if ( (T_SCANF_8 == 1) &&                                                 \
          (defined(SCNd8)) )
        T_FORMATS(8, int8_t, uint8_t);
    #else
        T_PRINTF_FORMATS(8, int8_t, uint8_t);
    #endif
#endif  // INT8_MAX

#ifdef INT16_MAX
    T_FORMATS(16, int16_t, uint16_t);
#endif  // INT16_MAX

#ifdef INT32_MAX
    T_FORMATS(32, int32_t, uint32_t);
#endif  // INT32_MAX

#ifdef INT64_MAX
    T_FORMATS(64, int64_t, uint64_t);
#endif  // INT64_MAX

#ifdef INT_LEAST8_MAX
    #if ( (T_SCANF_8 == 1) &&                                                 \
          (defined(SCNdLEAST8)) )
        T_FORMATS(LEAST8, int_least8_t, uint_least8_t);
    #else
        T_PRINTF_FORMATS(LEAST8, int_least8_t, uint_least8_t);
    #endif
#endif  // INT_LEAST8_MAX

#ifdef INT_LEAST16_MAX
    T_FORMATS(LEAST16, int_least16_t, uint_least16_t);
#endif  // INT_LEAST16_MAX

#ifdef INT_LEAST32_MAX
    T_FORMATS(LEAST32, int_least32_t, uint_least32_t);
#endif  // INT_LEAST32_MAX

#ifdef INT_LEAST64_MAX
    T_FORMATS(LEAST64, int_least64_t, uint_least64_t);
#endif  // INT_LEAST64_MAX

#ifdef INTPTR_MAX
    T_FORMATS(PTR, intptr_t, uintptr_t);
#endif  // INTPTR_MAX

#ifdef INTMAX_MAX
    T_FORMATS(MAX, intmax_t, uintmax_t);
#endif  // INTMAX_MAX

#ifdef INT_FAST8_MAX
    #if ( (T_SCANF_8 == 1) &&                                                 \
          (defined(SCNdFAST8)) )
        T_FORMATS(FAST8, int_fast8_t, uint_fast8_t);
    #else
        T_PRINTF_FORMATS(FAST8, int_fast8_t, uint_fast8_t);
    #endif
#endif  // INT_FAST8_MAX

#ifdef INT_FAST16_MAX
    T_FORMATS(FAST16, int_fast16_t, uint_fast16_t);
#endif  // INT_FAST16_MAX

#ifdef INT_FAST32_MAX
    T_FORMATS(FAST32, int_fast32_t, uint_fast32_t);
#endif  // INT_FAST32_MAX

#ifdef INT_FAST64_MAX
    T_FORMATS(FAST64, int_fast64_t, uint_fast64_t);
#endif  // INT_FAST64_MAX

    return;
}

#endif  // T_DSTDINT_FREESTANDING


/*
Identity with the platform
  Each typedef the platform's headers make here redeclares one of dstdint.h's,
which C11 and C++ allow only for the same type. Everything above has used the
header's macros; the platform's may replace them from here on.
*/
#ifdef T_DSTDINT_WITH_SYSTEM
    // std
    #ifndef T_DSTDINT_FREESTANDING
        #include <inttypes.h>  // the platform's intN_t, ..., and formats
    #endif  // T_DSTDINT_FREESTANDING
    #include <stdint.h>        // the platform's intN_t, ...
#endif  // T_DSTDINT_WITH_SYSTEM
