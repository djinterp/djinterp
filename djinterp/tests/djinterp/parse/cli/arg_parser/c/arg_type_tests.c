/*******************************************************************************
* djinterp [parse]                                              arg_type_tests.c
*
* The suite of the argument parser's value types, declared in
* arg_type_tests.h.
*   The conversion tables are CLI11 2.7.2's results, read under the C rule
* for leading zeros, which is CLI11's, for the same texts: CLI11's
* lexical_conversion into each type, its to_flag_value, and its
* sum_string_vector. A row the module reads otherwise says so beside it; the
* departures are three: a comma, which CLI11 removes as the classic
* locale's thousands separator; a prefix with no digits after it, or after
* a sign, which CLI11 reads or refuses by accident of the order it tries
* things in; and a floating-point value starting with a separator. The
* written texts are the module's own -- the shortest text that reads back
* -- where CLI11 writes every digit a type has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_type_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./arg_type_tests.h"  // corresponding header
// std
#include <float.h>    // FLT_MAX, DBL_MAX
#include <limits.h>   // ULONG_MAX, LONG_MAX, LLONG_MIN, LLONG_MAX
#include <locale.h>   // setlocale, localeconv, LC_NUMERIC
#include <math.h>     // isinf, isnan, signbit
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, ptrdiff_t, NULL
#include <stdio.h>    // printf
#include <string.h>   // memcmp, memcpy, memset, strcmp, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // the readers, descriptors
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // d_tests_arg_same
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // exact widths, intmax_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// CONTEXT_C
//   constant: a conversion under the C rule for leading zeros, CLI11's.
static const struct d_arg_convert_context CONTEXT_C =
{
    D_ARG_INTEGER_C,
    NULL,
    NULL
};

// d_tests_arg_type_long_row
//   struct: a text, and what a `long` reader makes of it: whether it
// converts, and to what.
struct d_tests_arg_type_long_row
{
    const char* text;
    bool        converts;
    long        value;
};

// LONG_C_ROWS
//   constant: d_arg_parse_long under the C rule. Every row is CLI11's but
// the last five, where the comment says what CLI11 does.
static const struct d_tests_arg_type_long_row LONG_C_ROWS[] =
{
    { "0",           true,  0L },
    { "1",           true,  1L },
    { "-1",          true,  -1L },
    { "+1",          true,  1L },
    { "42",          true,  42L },
    { " 42",         true,  42L },
    { "42 ",         true,  42L },
    { " 42 ",        true,  42L },
    { "\t42",        true,  42L },
    { "42\n",        true,  42L },
    { "010",         true,  8L },
    { "08",          false, 0L },
    { "0x1F",        true,  31L },
    { "0X1f",        true,  31L },
    { "-0x1F",       true,  -31L },
    { "0x",          false, 0L },
    { "0o17",        true,  15L },
    { "0O17",        true,  15L },
    { "0o8",         false, 0L },
    { "0b101",       true,  5L },
    { "0B11",        true,  3L },
    { "-0b11",       true,  -3L },
    { "0b2",         false, 0L },
    { "1_000",       true,  1000L },
    { "1'000",       true,  1000L },
    { "1__0",        true,  10L },
    { "_5",          true,  5L },
    { "5_",          true,  5L },
    { "-_5",         true,  -5L },
    { "0x1_F",       true,  31L },
    { "0_x1F",       true,  31L },
    { "true",        true,  1L },
    { "-0",          true,  0L },
    { "2147483647",  true,  2147483647L },
    { "-2147483648", true,  -2147483647L - 1L },
    { "",            false, 0L },
    { " ",           false, 0L },
    { "1.000",       false, 0L },
    { "1e3",         false, 0L },
    { "1.5",         false, 0L },
    { "false",       false, 0L },
    { "TRUE",        false, 0L },
    { "on",          false, 0L },
    { "+",           false, 0L },
    { "-",           false, 0L },
    { "x7",          false, 0L },
    { "7x",          false, 0L },
    { "--1",         false, 0L },
    { "- 5",         false, 0L },
    { "+-1",         false, 0L },
    { "1 2",         false, 0L },
    { "inf",         false, 0L },
    { "1,000",       false, 0L },   // CLI11: 1000, the comma removed
    { "0b",          false, 0L },   // CLI11: 0
    { "0o",          false, 0L },   // CLI11: 0
    { "-0o17",       true,  -15L }, // CLI11: refused
    { "+0o17",       true,  15L }   // CLI11: refused
};

// LONG_DECIMAL_ROWS
//   constant: d_arg_parse_long under the decimal rule, the module's default:
// leading zeros are decimal, and the prefixes name the other bases.
static const struct d_tests_arg_type_long_row LONG_DECIMAL_ROWS[] =
{
    { "010",   true,  10L },
    { "08",    true,  8L },
    { "-010",  true,  -10L },
    { "007",   true,  7L },
    { "00",    true,  0L },
    { "0",     true,  0L },
    { "0_10",  true,  10L },
    { "0x10",  true,  16L },
    { "0o10",  true,  8L },
    { "0b10",  true,  2L },
    { "1_000", true,  1000L },
    { "0x",    false, 0L },
    { "0o8",   false, 0L }
};

// d_tests_arg_type_ulong_row
//   struct: a text, and what d_arg_parse_ulong makes of it.
struct d_tests_arg_type_ulong_row
{
    const char*   text;
    bool          converts;
    unsigned long value;
};

// ULONG_ROWS
//   constant: d_arg_parse_ulong under the C rule, every row CLI11's: a minus
// sign anywhere before the digits is refused, and so is `true`.
static const struct d_tests_arg_type_ulong_row ULONG_ROWS[] =
{
    { "0",          true,  0UL },
    { "42",         true,  42UL },
    { "+1",         true,  1UL },
    { " 42 ",       true,  42UL },
    { "010",        true,  8UL },
    { "0x1F",       true,  31UL },
    { "0b101",      true,  5UL },
    { "1_000",      true,  1000UL },
    { "_5",         true,  5UL },
    { "4294967295", true,  4294967295UL },
    { "-1",         false, 0UL },
    { "-0",         false, 0UL },
    { "-0x1F",      false, 0UL },
    { "-_5",        false, 0UL },
    { "true",       false, 0UL },
    { "+",          false, 0UL },
    { "x7",         false, 0UL },
    { "1.5",        false, 0UL },
    { "",           false, 0UL },
    { " ",          false, 0UL }
};

// d_tests_arg_type_value
//   union: room for a value of any built-in type.
union d_tests_arg_type_value
{
    bool               boolean;
    char               character;
    signed char        signed_char;
    unsigned char      unsigned_char;
    short              short_int;
    unsigned short     unsigned_short;
    int                integer;
    unsigned int       unsigned_int;
    long               long_int;
    unsigned long      unsigned_long;
    long long          long_long;
    unsigned long long unsigned_long_long;
    size_t             size;
    ptrdiff_t          difference;
    int8_t             int8;
    int16_t            int16;
    int32_t            int32;
    int64_t            int64;
    uint8_t            uint8;
    uint16_t           uint16;
    uint32_t           uint32;
    uint64_t           uint64;
    intmax_t           intmax;
    uintmax_t          uintmax;
    float              single;
    double             real;
    long double        extended;
    const char*        string;
    struct d_pack_text text;
};

// d_tests_arg_type_signed_row
//   struct: a text, a signed built-in type, and what converting the one into
// the other comes to.
struct d_tests_arg_type_signed_row
{
    int         builtin;
    const char* text;
    bool        converts;
    long long   value;
};

// SIGNED_ROWS
//   constant: each signed integer type at its limits, CLI11's results. A
// type whose width differs by platform -- `long`, ptrdiff_t -- is held to
// the 32-bit limits every platform has.
static const struct d_tests_arg_type_signed_row SIGNED_ROWS[] =
{
    { D_ARG_BUILTIN_SIGNED_CHAR, "127",                   true,  127LL },
    { D_ARG_BUILTIN_SIGNED_CHAR, "128",                   false, 0LL },
    { D_ARG_BUILTIN_SIGNED_CHAR, "-128",                  true,  -128LL },
    { D_ARG_BUILTIN_SIGNED_CHAR, "-129",                  false, 0LL },
    { D_ARG_BUILTIN_SIGNED_CHAR, "a",                     false, 0LL },
    { D_ARG_BUILTIN_SHORT,       "32767",                 true,  32767LL },
    { D_ARG_BUILTIN_SHORT,       "32768",                 false, 0LL },
    { D_ARG_BUILTIN_SHORT,       "-32768",                true,  -32768LL },
    { D_ARG_BUILTIN_SHORT,       "-32769",                false, 0LL },
    { D_ARG_BUILTIN_INT,         "2147483647",            true,
      2147483647LL },
    { D_ARG_BUILTIN_INT,         "2147483648",            false, 0LL },
    { D_ARG_BUILTIN_INT,         "-2147483648",           true,
      -2147483648LL },
    { D_ARG_BUILTIN_INT,         "-2147483649",           false, 0LL },
    { D_ARG_BUILTIN_LONG,        "2147483647",            true,
      2147483647LL },
    { D_ARG_BUILTIN_LONG,        "-2147483648",           true,
      -2147483648LL },
    { D_ARG_BUILTIN_LONG_LONG,   "9223372036854775807",   true,
      9223372036854775807LL },
    { D_ARG_BUILTIN_LONG_LONG,   "9223372036854775808",   false, 0LL },
    { D_ARG_BUILTIN_LONG_LONG,   "-9223372036854775808",  true,
      -9223372036854775807LL - 1LL },
    { D_ARG_BUILTIN_LONG_LONG,   "-9223372036854775809",  false, 0LL },
    { D_ARG_BUILTIN_LONG_LONG,   "0x7fffffffffffffff",    true,
      9223372036854775807LL },
    { D_ARG_BUILTIN_PTRDIFF,     "2147483647",            true,
      2147483647LL },
    { D_ARG_BUILTIN_PTRDIFF,     "-2147483648",           true,
      -2147483648LL },
    { D_ARG_BUILTIN_INT8,        "127",                   true,  127LL },
    { D_ARG_BUILTIN_INT8,        "-129",                  false, 0LL },
    { D_ARG_BUILTIN_INT16,       "-32768",                true,  -32768LL },
    { D_ARG_BUILTIN_INT16,       "32768",                 false, 0LL },
    { D_ARG_BUILTIN_INT32,       "-2147483648",           true,
      -2147483648LL },
    { D_ARG_BUILTIN_INT32,       "2147483648",            false, 0LL },
    { D_ARG_BUILTIN_INT64,       "-9223372036854775808",  true,
      -9223372036854775807LL - 1LL },
    { D_ARG_BUILTIN_INT64,       "9223372036854775808",   false, 0LL },
    { D_ARG_BUILTIN_INTMAX,      "-9223372036854775808",  true,
      -9223372036854775807LL - 1LL }
};

// d_tests_arg_type_unsigned_row
//   struct: a text, an unsigned built-in type, and what converting the one
// into the other comes to.
struct d_tests_arg_type_unsigned_row
{
    int                builtin;
    const char*        text;
    bool               converts;
    unsigned long long value;
};

// UNSIGNED_ROWS
//   constant: each unsigned integer type at its limits, CLI11's results,
// with the same care for the types whose width differs by platform.
static const struct d_tests_arg_type_unsigned_row UNSIGNED_ROWS[] =
{
    { D_ARG_BUILTIN_UNSIGNED_CHAR,      "255",          true,  255ULL },
    { D_ARG_BUILTIN_UNSIGNED_CHAR,      "256",          false, 0ULL },
    { D_ARG_BUILTIN_UNSIGNED_CHAR,      "-1",           false, 0ULL },
    { D_ARG_BUILTIN_UNSIGNED_SHORT,     "65535",        true,  65535ULL },
    { D_ARG_BUILTIN_UNSIGNED_SHORT,     "65536",        false, 0ULL },
    { D_ARG_BUILTIN_UNSIGNED_INT,       "4294967295",   true,
      4294967295ULL },
    { D_ARG_BUILTIN_UNSIGNED_INT,       "4294967296",   false, 0ULL },
    { D_ARG_BUILTIN_UNSIGNED_LONG,      "4294967295",   true,
      4294967295ULL },
    { D_ARG_BUILTIN_UNSIGNED_LONG,      "-1",           false, 0ULL },
    { D_ARG_BUILTIN_UNSIGNED_LONG_LONG, "18446744073709551615", true,
      18446744073709551615ULL },
    { D_ARG_BUILTIN_UNSIGNED_LONG_LONG, "18446744073709551616", false,
      0ULL },
    { D_ARG_BUILTIN_UNSIGNED_LONG_LONG, "0xffffffffffffffff",   true,
      18446744073709551615ULL },
    { D_ARG_BUILTIN_UNSIGNED_LONG_LONG, "-1",           false, 0ULL },
    { D_ARG_BUILTIN_SIZE,               "4294967295",   true,
      4294967295ULL },
    { D_ARG_BUILTIN_SIZE,               "-1",           false, 0ULL },
    { D_ARG_BUILTIN_UINT8,              "200",          true,  200ULL },
    { D_ARG_BUILTIN_UINT8,              "256",          false, 0ULL },
    { D_ARG_BUILTIN_UINT16,             "65535",        true,  65535ULL },
    { D_ARG_BUILTIN_UINT32,             "4294967296",   false, 0ULL },
    { D_ARG_BUILTIN_UINT64,             "18446744073709551615", true,
      18446744073709551615ULL },
    { D_ARG_BUILTIN_UINT64,             "99999999999999999999", false,
      0ULL },
    { D_ARG_BUILTIN_UINTMAX,            "18446744073709551615", true,
      18446744073709551615ULL }
};

// d_tests_arg_type_char_row
//   struct: a text, a character type, and the value converting it gives.
struct d_tests_arg_type_char_row
{
    int         builtin;
    const char* text;
    bool        converts;
    int         value;
};

// CHAR_ROWS
//   constant: the character types, CLI11's results: `char` takes one
// character as itself and longer text as a small integer; `signed char` and
// `unsigned char` are integers only.
static const struct d_tests_arg_type_char_row CHAR_ROWS[] =
{
    { D_ARG_BUILTIN_CHAR,          " ",     true,  32 },
    { D_ARG_BUILTIN_CHAR,          "a",     true,  97 },
    { D_ARG_BUILTIN_CHAR,          "7",     true,  55 },
    { D_ARG_BUILTIN_CHAR,          "+",     true,  43 },
    { D_ARG_BUILTIN_CHAR,          "65",    true,  65 },
    { D_ARG_BUILTIN_CHAR,          "-65",   true,  -65 },
    { D_ARG_BUILTIN_CHAR,          "127",   true,  127 },
    { D_ARG_BUILTIN_CHAR,          "128",   false, 0 },
    { D_ARG_BUILTIN_CHAR,          "-128",  true,  -128 },
    { D_ARG_BUILTIN_CHAR,          "0x1F",  true,  31 },
    { D_ARG_BUILTIN_CHAR,          "true",  true,  1 },
    { D_ARG_BUILTIN_CHAR,          "ab",    false, 0 },
    { D_ARG_BUILTIN_CHAR,          "200",   false, 0 },
    { D_ARG_BUILTIN_SIGNED_CHAR,   " ",     false, 0 },
    { D_ARG_BUILTIN_SIGNED_CHAR,   "t",     false, 0 },
    { D_ARG_BUILTIN_SIGNED_CHAR,   "65",    true,  65 },
    { D_ARG_BUILTIN_UNSIGNED_CHAR, "a",     false, 0 },
    { D_ARG_BUILTIN_UNSIGNED_CHAR, "200",   true,  200 },
    { D_ARG_BUILTIN_UNSIGNED_CHAR, "-1",    false, 0 }
};

// d_tests_arg_type_double_row
//   struct: a text, and what d_arg_parse_double makes of it.
struct d_tests_arg_type_double_row
{
    const char* text;
    bool        converts;
    double      value;
};

// DOUBLE_ROWS
//   constant: d_arg_parse_double, CLI11's results but for the last four,
// where the comment says what CLI11 does. Leading zeros never make octal.
static const struct d_tests_arg_type_double_row DOUBLE_ROWS[] =
{
    { "0",       true,  0.0 },
    { "1.5",     true,  1.5 },
    { "-1.5",    true,  -1.5 },
    { "+1",      true,  1.0 },
    { ".5",      true,  0.5 },
    { "-.5",     true,  -0.5 },
    { "5.",      true,  5.0 },
    { "1e3",     true,  1000.0 },
    { "1E3",     true,  1000.0 },
    { "1e-3",    true,  0.001 },
    { "1.5e2",   true,  150.0 },
    { " 42 ",    true,  42.0 },
    { "\t42",    true,  42.0 },
    { "42\n",    true,  42.0 },
    { "010",     true,  10.0 },
    { "08",      true,  8.0 },
    { "0x1F",    true,  31.0 },
    { "-0x1F",   true,  -31.0 },
    { "0x1p3",   true,  8.0 },
    { "1_000",   true,  1000.0 },
    { "1'000",   true,  1000.0 },
    { "1__0",    true,  10.0 },
    { "5_",      true,  5.0 },
    { "0x1_F",   true,  31.0 },
    { "0_x1F",   true,  31.0 },
    { "1.000",   true,  1.0 },
    { "1e-400",  true,  0.0 },
    { "",        false, 0.0 },
    { " ",       false, 0.0 },
    { "0x",      false, 0.0 },
    { "0o17",    false, 0.0 },
    { "0b101",   false, 0.0 },
    { "true",    false, 0.0 },
    { "+",       false, 0.0 },
    { "x7",      false, 0.0 },
    { "7x",      false, 0.0 },
    { "--1",     false, 0.0 },
    { "- 5",     false, 0.0 },
    { "1 2",     false, 0.0 },
    { "_5",      true,  5.0 },  // CLI11: refused
    { "-_5",     true,  -5.0 }, // CLI11: refused
    { "1,000",   false, 0.0 },  // CLI11: 1000, the comma removed
    { "1,5",     false, 0.0 }   // CLI11: 15
};

// d_tests_arg_type_bool_row
//   struct: a text, and what d_arg_parse_bool makes of it.
struct d_tests_arg_type_bool_row
{
    const char* text;
    bool        converts;
    bool        value;
};

// BOOL_ROWS
//   constant: d_arg_parse_bool under the C rule, every row CLI11's: words in
// any case, numbers by their sign, numbers too large for any type by their
// sign too, whitespace only before a number.
static const struct d_tests_arg_type_bool_row BOOL_ROWS[] =
{
    { "",                      true,  false },
    { "0",                     true,  false },
    { "1",                     true,  true },
    { "-1",                    true,  false },
    { "+1",                    true,  true },
    { "7",                     true,  true },
    { " 42",                   true,  true },
    { "\t42",                  true,  true },
    { "010",                   true,  true },
    { "0x1F",                  true,  true },
    { "-0x1F",                 true,  false },
    { "0b101",                 true,  true },
    { "-0b11",                 true,  false },
    { "-0",                    true,  false },
    { "true",                  true,  true },
    { "TRUE",                  true,  true },
    { "True",                  true,  true },
    { "false",                 true,  false },
    { "on",                    true,  true },
    { "off",                   true,  false },
    { "yes",                   true,  true },
    { "no",                    true,  false },
    { "enable",                true,  true },
    { "disable",               true,  false },
    { "t",                     true,  true },
    { "f",                     true,  false },
    { "y",                     true,  true },
    { "n",                     true,  false },
    { "+",                     true,  true },
    { "-",                     true,  false },
    { "9223372036854775808",   true,  true },
    { "-9223372036854775809",  true,  false },
    { "99999999999999999999",  true,  true },
    { "0xffffffffffffffff",    true,  true },
    { " ",                     false, false },
    { "42 ",                   false, false },
    { "42\n",                  false, false },
    { "08",                    false, false },
    { "0x",                    false, false },
    { "0o17",                  false, false },
    { "0b",                    false, false },
    { "1_000",                 false, false },
    { "_5",                    false, false },
    { "1,000",                 false, false },
    { "1.5",                   false, false },
    { "0.0",                   false, false },
    { "inf",                   false, false },
    { "maybe",                 false, false },
    { "--1",                   false, false },
    { "1 2",                   false, false }
};

// FLAG_ROWS
//   constant: d_arg_flag_value under the C rule, every row CLI11's
// to_flag_value: a true word is 1, a false word or `0` is -1, any other
// number is itself -- `-0` being 0 -- and the empty text is 0.
static const struct d_tests_arg_type_long_row FLAG_ROWS[] =
{
    { "",            true,  0L },
    { "0",           true,  -1L },
    { "-0",          true,  0L },
    { "1",           true,  1L },
    { "-1",          true,  -1L },
    { "+1",          true,  1L },
    { "7",           true,  7L },
    { "-7",          true,  -7L },
    { " 42",         true,  42L },
    { "010",         true,  8L },
    { "0x1F",        true,  31L },
    { "-0x1F",       true,  -31L },
    { "0b101",       true,  5L },
    { "-0b11",       true,  -3L },
    { "true",        true,  1L },
    { "TRUE",        true,  1L },
    { "false",       true,  -1L },
    { "on",          true,  1L },
    { "off",         true,  -1L },
    { "yes",         true,  1L },
    { "no",          true,  -1L },
    { "enable",      true,  1L },
    { "disable",     true,  -1L },
    { "t",           true,  1L },
    { "f",           true,  -1L },
    { "y",           true,  1L },
    { "n",           true,  -1L },
    { "+",           true,  1L },
    { "-",           true,  -1L },
    { "2147483647",  true,  2147483647L },
    { "-2147483648", true,  -2147483647L - 1L },
    { " ",           false, 0L },
    { "42 ",         false, 0L },
    { "08",          false, 0L },
    { "0x",          false, 0L },
    { "0o17",        false, 0L },
    { "0b",          false, 0L },
    { "1_000",       false, 0L },
    { "_5",          false, 0L },
    { "1,000",       false, 0L },
    { "1.5",         false, 0L },
    { "maybe",       false, 0L }
};

// d_tests_arg_type_wide_row
//   struct: a text, and what a reader as wide as `long long` makes of it:
// whether it converts, and to what.
struct d_tests_arg_type_wide_row
{
    const char* text;
    bool        converts;
    long long   value;
};

// FLAG_WIDE_ROWS
//   constant: d_arg_flag_value_intmax under the C rule, every row CLI11's
// to_flag_value over the 64 bits it reads into: the limits, decimal and
// hexadecimal, one past each, numbers past 32 bits, and the readings
// d_arg_flag_value shares.
static const struct d_tests_arg_type_wide_row FLAG_WIDE_ROWS[] =
{
    { "9223372036854775807",   true,  LLONG_MAX },
    { "-9223372036854775808",  true,  LLONG_MIN },
    { "0x7fffffffffffffff",    true,  LLONG_MAX },
    { "-0x8000000000000000",   true,  LLONG_MIN },
    { "4294967296",            true,  4294967296LL },
    { "-2147483649",           true,  -2147483649LL },
    { " 0x100000000",          true,  4294967296LL },
    { "9223372036854775808",   false, 0LL },
    { "-9223372036854775809",  false, 0LL },
    { "0x8000000000000000",    false, 0LL },
    { "",                      true,  0LL },
    { "0",                     true,  -1LL },
    { "7",                     true,  7LL },
    { "yes",                   true,  1LL },
    { "Disable",               true,  -1LL },
    { "4294967296 ",           false, 0LL },
    { "4_294_967_296",         false, 0LL }
};

// d_tests_arg_type_sum_row
//   struct: up to four values, and the text the sum policy makes of them.
struct d_tests_arg_type_sum_row
{
    const char* values[4];
    size_t      count;
    const char* sum;
};

// SUM_ROWS
//   constant: d_arg_sum_values under the C rule, every row CLI11's
// sum_string_vector: exact for integers, sixteen significant digits for
// floating point, flag words as their values, text concatenated.
static const struct d_tests_arg_type_sum_row SUM_ROWS[] =
{
    { { "1", "2" },                         2u, "3" },
    { { "0.1", "0.2" },                     2u, "0.3" },
    { { "1", "true", "3" },                 3u, "5" },
    { { "a", "b" },                         2u, "ab" },
    { { "1.5", "yes" },                     2u, "2.5" },
    { { NULL },                             0u, "0" },
    { { "7" },                              1u, "7" },
    { { "9223372036854775807", "1" },       2u, "9.223372036854776e+18" },
    { { "-9223372036854775808", "-1" },     2u, "-9.223372036854776e+18" },
    { { "1e3", "1" },                       2u, "1001" },
    { { "0x10", "1" },                      2u, "17" },
    { { "010", "1" },                       2u, "9" },
    { { "1", "off" },                       2u, "0" },
    { { "-", "+" },                         2u, "0" },
    { { "2", "x" },                         2u, "2x" },
    { { "1_000", "1" },                     2u, "1001" },
    { { "1.0", "2.0" },                     2u, "3" },
    { { "1e300", "1e300" },                 2u, "2e+300" },
    { { "0.1", "0.2", "0.3" },              3u, "0.6000000000000001" },
    { { "123456789012345678", "1" },        2u, "123456789012345679" },
    { { "0.000001", "0" },                  2u, "1e-06" },
    { { "1", "-1" },                        2u, "0" },
    { { "true", "true" },                   2u, "2" },
    { { "", "1" },                          2u, "1" }
};

// d_tests_arg_type_color
//   enum: the values of the caller's own type d_tests_arg_type_custom_
// descriptor converts into.
enum d_tests_arg_type_color
{
    COLOR_RED   = 0,
    COLOR_GREEN = 1,
    COLOR_BLUE  = 2
};

// COLOR_NAMES
//   constant: each color's name, in the enumeration's order.
static const char* const COLOR_NAMES[] =
{
    "red",
    "green",
    "blue"
};

/*
d_tests_arg_type_convert_color
  A converter of the caller's: a color's name into an `int`, and a reason
for anything else.
*/
static int
d_tests_arg_type_convert_color(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    (void)_type;

    // no value is no color
    if (_count == 0u)
    {
        return D_ARG_ERROR_CONVERSION;
    }

    // a name among the colors' names
    for (int color = 0; color < 3; ++color)
    {
        if (d_pack_text_equal(_values[0],
                              d_pack_text_from_cstr(COLOR_NAMES[color])))
        {
            *(int*)_target = color;

            return D_ARG_OK;
        }
    }

    // the reason, for a caller who asks for it
    if ( (_context != NULL) &&
         (_context->message != NULL) )
    {
        (void)d_arg_buffer_printf(_context->message,
                                  "not a color: %.*s",
                                  (int)_values[0].length,
                                  _values[0].data);
    }

    return D_ARG_ERROR_CONVERSION;
}

/*
d_tests_arg_type_format_color
  A color's name, written as the converter reads it.
*/
static int
d_tests_arg_type_format_color(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    const int color = *(const int*)_value;

    (void)_type;

    return d_arg_sink_cstr(_sink,
                           ( (color >= 0) &&
                             (color < 3) ) ? COLOR_NAMES[color] : "?");
}

// COLOR
//   constant: the descriptor of the caller's color type, as a program would
// define one: static, so that it outlives every option bound through it.
static const struct d_arg_type COLOR =
{
    "COLOR",
    d_tests_arg_type_convert_color,
    d_tests_arg_type_format_color,
    NULL,
    sizeof(int),
    1,
    1,
    1,
    D_ARG_TYPE_FLAG_ENUM
};

// NO_FORMAT
//   constant: a descriptor with no way to write its values.
static const struct d_arg_type NO_FORMAT =
{
    "OPAQUE",
    d_tests_arg_type_convert_color,
    NULL,
    NULL,
    sizeof(int),
    1,
    1,
    1,
    0u
};

// d_tests_arg_type_samples
//   struct: one value of each kind d_tests_arg_type_format writes.
struct d_tests_arg_type_samples
{
    double             tenth;
    double             third;
    double             large;
    double             smallest;
    double             negative_zero;
    double             hundred;
    double             little;
    double             wide;
    double             fixed;
    float              single;
    long double        extended;
    int                negative;
    int8_t             lowest;
    uint64_t           highest;
    unsigned char      byte;
    char               letter;
    bool               yes;
    bool               no;
    const char*        string;
    const char*        nothing;
    struct d_pack_text span;
};

// SAMPLES
//   constant: the values d_tests_arg_type_format writes.
static const struct d_tests_arg_type_samples SAMPLES =
{
    0.1,
    1.0 / 3.0,
    1e21,
    5e-324,
    -0.0,
    100.0,
    1e-7,
    3e16,
    1.5e3,
    1.0f / 3.0f,
    2.5L,
    -42,
    -128,
    18446744073709551615ULL,
    200u,
    'a',
    true,
    false,
    "text",
    NULL,
    { "span!", 4u }
};

// d_tests_arg_type_format_row
//   struct: one of the samples, and the text its type writes for it.
struct d_tests_arg_type_format_row
{
    int         builtin;
    const void* value;
    const char* text;
};

// FORMAT_ROWS
//   constant: what each built-in type writes: floating point as the
// shortest text that reads back, fixed while the type's digits allow it,
// with a two-digit exponent otherwise; integers, characters, truth values
// and text as themselves, a NULL string as nothing.
static const struct d_tests_arg_type_format_row FORMAT_ROWS[] =
{
    { D_ARG_BUILTIN_DOUBLE,        &SAMPLES.tenth,         "0.1" },
    { D_ARG_BUILTIN_DOUBLE,        &SAMPLES.third,
      "0.3333333333333333" },
    { D_ARG_BUILTIN_DOUBLE,        &SAMPLES.large,         "1e+21" },
    { D_ARG_BUILTIN_DOUBLE,        &SAMPLES.smallest,      "5e-324" },
    { D_ARG_BUILTIN_DOUBLE,        &SAMPLES.negative_zero, "-0" },
    { D_ARG_BUILTIN_DOUBLE,        &SAMPLES.hundred,       "100" },
    { D_ARG_BUILTIN_DOUBLE,        &SAMPLES.little,        "1e-07" },
    { D_ARG_BUILTIN_DOUBLE,        &SAMPLES.wide,
      "30000000000000000" },
    { D_ARG_BUILTIN_DOUBLE,        &SAMPLES.fixed,         "1500" },
    { D_ARG_BUILTIN_FLOAT,         &SAMPLES.single,        "0.33333334" },
    { D_ARG_BUILTIN_LONG_DOUBLE,   &SAMPLES.extended,      "2.5" },
    { D_ARG_BUILTIN_INT,           &SAMPLES.negative,      "-42" },
    { D_ARG_BUILTIN_INT8,          &SAMPLES.lowest,        "-128" },
    { D_ARG_BUILTIN_UINT64,        &SAMPLES.highest,
      "18446744073709551615" },
    { D_ARG_BUILTIN_UNSIGNED_CHAR, &SAMPLES.byte,          "200" },
    { D_ARG_BUILTIN_CHAR,          &SAMPLES.letter,        "a" },
    { D_ARG_BUILTIN_BOOL,          &SAMPLES.yes,           "true" },
    { D_ARG_BUILTIN_BOOL,          &SAMPLES.no,            "false" },
    { D_ARG_BUILTIN_STRING,        &SAMPLES.string,        "text" },
    { D_ARG_BUILTIN_STRING,        &SAMPLES.nothing,       "" },
    { D_ARG_BUILTIN_TEXT,          &SAMPLES.span,          "span" }
};

/*
d_tests_arg_type_convert
  Converts one text into a value through a built-in descriptor, under the
C rule, and returns the descriptor's status.
*/
static int
d_tests_arg_type_convert(
    int                           _builtin,
    const char*                   _text,
    union d_tests_arg_type_value* _value
)
{
    const struct d_pack_text text = d_pack_text_from_cstr(_text);

    return d_arg_convert(d_arg_type_builtin(_builtin),
                         &text,
                         1u,
                         _value,
                         &CONTEXT_C);
}

/*
d_tests_arg_type_signed_of
  The value a signed built-in type left in the union, widened.
*/
static long long
d_tests_arg_type_signed_of(
    int                                 _builtin,
    const union d_tests_arg_type_value* _value
)
{
    switch (_builtin)
    {
        case D_ARG_BUILTIN_SIGNED_CHAR: return _value->signed_char;
        case D_ARG_BUILTIN_SHORT:       return _value->short_int;
        case D_ARG_BUILTIN_INT:         return _value->integer;
        case D_ARG_BUILTIN_LONG:        return _value->long_int;
        case D_ARG_BUILTIN_LONG_LONG:   return _value->long_long;
        case D_ARG_BUILTIN_PTRDIFF:     return (long long)_value->difference;
        case D_ARG_BUILTIN_INT8:        return _value->int8;
        case D_ARG_BUILTIN_INT16:       return _value->int16;
        case D_ARG_BUILTIN_INT32:       return _value->int32;
        case D_ARG_BUILTIN_INT64:       return (long long)_value->int64;
        case D_ARG_BUILTIN_INTMAX:      return (long long)_value->intmax;
        default:                        return 0LL;
    }
}

/*
d_tests_arg_type_unsigned_of
  The value an unsigned built-in type left in the union, widened.
*/
static unsigned long long
d_tests_arg_type_unsigned_of(
    int                                 _builtin,
    const union d_tests_arg_type_value* _value
)
{
    switch (_builtin)
    {
        case D_ARG_BUILTIN_UNSIGNED_CHAR:      return _value->unsigned_char;
        case D_ARG_BUILTIN_UNSIGNED_SHORT:     return _value->unsigned_short;
        case D_ARG_BUILTIN_UNSIGNED_INT:       return _value->unsigned_int;
        case D_ARG_BUILTIN_UNSIGNED_LONG:      return _value->unsigned_long;
        case D_ARG_BUILTIN_UNSIGNED_LONG_LONG:
            return _value->unsigned_long_long;
        case D_ARG_BUILTIN_SIZE:               return _value->size;
        case D_ARG_BUILTIN_UINT8:              return _value->uint8;
        case D_ARG_BUILTIN_UINT16:             return _value->uint16;
        case D_ARG_BUILTIN_UINT32:             return _value->uint32;
        case D_ARG_BUILTIN_UINT64:
            return (unsigned long long)_value->uint64;
        case D_ARG_BUILTIN_UINTMAX:
            return (unsigned long long)_value->uintmax;
        default:                               return 0ULL;
    }
}

/*
d_tests_arg_type_zero
  Whether a built-in type left its zero in the union: 0 for a number,
`false`, NUL, empty text.
*/
static bool
d_tests_arg_type_zero(
    int                                 _builtin,
    const union d_tests_arg_type_value* _value
)
{
    switch (_builtin)
    {
        case D_ARG_BUILTIN_BOOL:        return !_value->boolean;
        case D_ARG_BUILTIN_CHAR:        return (_value->character == '\0');
        case D_ARG_BUILTIN_FLOAT:       return (_value->single == 0.0f);
        case D_ARG_BUILTIN_DOUBLE:      return (_value->real == 0.0);
        case D_ARG_BUILTIN_LONG_DOUBLE: return (_value->extended == 0.0L);
        case D_ARG_BUILTIN_STRING:
            return ( (_value->string != NULL) &&
                     (_value->string[0] == '\0') );
        case D_ARG_BUILTIN_TEXT:        return (_value->text.length == 0u);
        default:
            return ( (d_tests_arg_type_signed_of(_builtin,
                                                 _value) == 0LL) &&
                     (d_tests_arg_type_unsigned_of(_builtin,
                                                   _value) == 0ULL) );
    }
}

/*
d_tests_arg_type_writes
  Whether a built-in type writes a value as the text expected.
*/
static bool
d_tests_arg_type_writes(
    int         _builtin,
    const void* _value,
    const char* _expected
)
{
    struct d_arg_buffer buffer = D_ARG_BUFFER_INIT;
    const int           status = d_arg_format(d_arg_type_builtin(_builtin),
                                              _value,
                                              d_arg_buffer_sink(&buffer));
    const bool          same   = d_tests_arg_same(_expected,
                                                  d_arg_buffer_text(&buffer),
                                                  _expected);

    d_arg_buffer_release(&buffer);

    return ( (status == D_ARG_OK) &&
             (same) );
}

/*
d_tests_arg_type_long_rows
  Runs d_arg_parse_long over a table under a rule, printing each row that
does not come out as the table says.
*/
static bool
d_tests_arg_type_long_rows(
    const struct d_tests_arg_type_long_row* _rows,
    size_t                                  _count,
    int                                     _rule
)
{
    bool held = true;

    // each row
    for (size_t index = 0u; index < _count; ++index)
    {
        long      value  = -999L;
        const int status = d_arg_parse_long(
                               d_pack_text_from_cstr(_rows[index].text),
                               _rule,
                               &value);
        const bool row   = (_rows[index].converts)
                           ? ( (status == D_ARG_OK) &&
                               (value == _rows[index].value) )
                           : ( (status == D_ARG_ERROR_CONVERSION) &&
                               (value == -999L) );

        // a row that did not hold is shown
        if (!row)
        {
            printf("    [%s]: status %d, value %ld\n",
                   _rows[index].text,
                   status,
                   value);
        }

        held = ( (held) &&
                 (row) );
    }

    return held;
}

//==============================================================================
// 1.  TYPE DESCRIPTORS
//==============================================================================

/*
d_tests_arg_type_builtin_descriptors
  What each built-in descriptor says of its type.
  Tests the following:
  - CLI11's help names: BOOLEAN, CHAR, INT, UINT, FLOAT and TEXT
  - each type's size, one value per item and one item per option
  - each converts and writes, and its flags say what kind of type it is
  - a value outside d_arg_builtin has no descriptor
*/
bool
d_tests_arg_type_builtin_descriptors(void)
{
    bool described = ( (d_arg_type_builtin(-1) == NULL)                     &&
                       (d_arg_type_builtin((int)D_ARG_BUILTIN_COUNT)
                        == NULL) );

    // each descriptor
    for (int builtin = 0; builtin < (int)D_ARG_BUILTIN_COUNT; ++builtin)
    {
        const struct d_arg_type* const type = d_arg_type_builtin(builtin);

        described = ( (described)                &&
                      (type != NULL)             &&
                      (type->convert != NULL)    &&
                      (type->format != NULL)     &&
                      (type->type_size_min == 1) &&
                      (type->type_size_max == 1) &&
                      (type->expected == 1) );
    }

    const struct d_arg_type* const boolean = d_arg_type_builtin(
                                                 D_ARG_BUILTIN_BOOL);
    const struct d_arg_type* const number  = d_arg_type_builtin(
                                                 D_ARG_BUILTIN_INT);
    const struct d_arg_type* const count   = d_arg_type_builtin(
                                                 D_ARG_BUILTIN_SIZE);
    const struct d_arg_type* const real    = d_arg_type_builtin(
                                                 D_ARG_BUILTIN_DOUBLE);
    const struct d_arg_type* const text    = d_arg_type_builtin(
                                                 D_ARG_BUILTIN_STRING);

    return ( (described)                                                &&
             (strcmp(boolean->name, "BOOLEAN") == 0)                    &&
             (boolean->flags == D_ARG_TYPE_FLAG_BOOL)                   &&
             (strcmp(number->name, "INT") == 0)                         &&
             (number->flags == (D_ARG_TYPE_FLAG_INTEGER |
                                D_ARG_TYPE_FLAG_SIGNED))                &&
             (number->size == sizeof(int))                              &&
             (strcmp(count->name, "UINT") == 0)                         &&
             (count->flags == D_ARG_TYPE_FLAG_INTEGER)                  &&
             (count->size == sizeof(size_t))                            &&
             (strcmp(real->name, "FLOAT") == 0)                         &&
             ((real->flags & D_ARG_TYPE_FLAG_FLOAT) != 0u)              &&
             (strcmp(text->name, "TEXT") == 0)                          &&
             (text->flags == D_ARG_TYPE_FLAG_STRING)                    &&
             (strcmp(d_arg_type_builtin(D_ARG_BUILTIN_CHAR)->name,
                     "CHAR") == 0) );
}

//==============================================================================
// 2.1  INTEGERS
//==============================================================================

/*
d_tests_arg_type_long_c_rule
  d_arg_parse_long under the C rule, CLI11's.
  Tests the following:
  - signs, whitespace around the number, the four bases, a leading zero as
    octal, `_` and `'` anywhere, `true` as 1
  - what is refused, the output then untouched
  - the module's departures: no comma, no prefix without digits, a sign
    before any prefix
*/
bool
d_tests_arg_type_long_c_rule(void)
{
    return d_tests_arg_type_long_rows(LONG_C_ROWS,
                                      sizeof(LONG_C_ROWS) /
                                      sizeof(LONG_C_ROWS[0]),
                                      D_ARG_INTEGER_C);
}

/*
d_tests_arg_type_long_decimal_rule
  d_arg_parse_long under the decimal rule.
  Tests the following:
  - leading zeros are decimal, so `08` reads and `010` is ten
  - `0x`, `0o` and `0b` still name the other bases
*/
bool
d_tests_arg_type_long_decimal_rule(void)
{
    return d_tests_arg_type_long_rows(LONG_DECIMAL_ROWS,
                                      sizeof(LONG_DECIMAL_ROWS) /
                                      sizeof(LONG_DECIMAL_ROWS[0]),
                                      D_ARG_INTEGER_DECIMAL);
}

/*
d_tests_arg_type_ulong
  d_arg_parse_ulong under the C rule.
  Tests the following:
  - the readings d_arg_parse_long shares, but for `true`
  - a minus sign is refused wherever it stands, before a prefix or a
    separator too
  - the limit of `unsigned long`, whatever its width
*/
bool
d_tests_arg_type_ulong(void)
{
    bool held = true;

    // each row
    for (size_t index = 0u;
         index < sizeof(ULONG_ROWS) / sizeof(ULONG_ROWS[0]);
         ++index)
    {
        unsigned long value  = 999UL;
        const int     status = d_arg_parse_ulong(
                                   d_pack_text_from_cstr(
                                       ULONG_ROWS[index].text),
                                   D_ARG_INTEGER_C,
                                   &value);

        held = ( (held) &&
                 ( (ULONG_ROWS[index].converts)
                   ? ( (status == D_ARG_OK) &&
                       (value == ULONG_ROWS[index].value) )
                   : ( (status == D_ARG_ERROR_CONVERSION) &&
                       (value == 999UL) ) ) );
    }

    unsigned long wide   = 0UL;
    const int     beyond = d_arg_parse_ulong(
                               d_pack_text_from_cstr("4294967296"),
                               D_ARG_INTEGER_C,
                               &wide);

    // past 32 bits, as wide as the platform's `unsigned long` is
    return ( (held) &&
             ( (ULONG_MAX > 4294967295UL)
               ? ( (beyond == D_ARG_OK) &&
                   (wide == (unsigned long)4294967295UL + 1UL) )
               : (beyond == D_ARG_ERROR_CONVERSION) ) );
}

/*
d_tests_arg_type_intmax
  The widest readers, d_arg_parse_intmax and d_arg_parse_uintmax.
  Tests the following:
  - the 64-bit limits, decimal and hexadecimal, and one past each
  - an unsigned reader refuses `-1`
*/
bool
d_tests_arg_type_intmax(void)
{
    intmax_t  low     = 0;
    intmax_t  high    = 0;
    uintmax_t top     = 0u;
    uintmax_t hex     = 0u;
    uintmax_t refused = 7u;

    const bool read  =
        ( (d_arg_parse_intmax(d_pack_text_from_cstr("-9223372036854775808"),
                              D_ARG_INTEGER_C,
                              &low) == D_ARG_OK)                       &&
          (d_arg_parse_intmax(d_pack_text_from_cstr("0x7fffffffffffffff"),
                              D_ARG_INTEGER_C,
                              &high) == D_ARG_OK)                      &&
          (d_arg_parse_uintmax(d_pack_text_from_cstr("18446744073709551615"),
                               D_ARG_INTEGER_C,
                               &top) == D_ARG_OK)                      &&
          (d_arg_parse_uintmax(d_pack_text_from_cstr("0xffffffffffffffff"),
                               D_ARG_INTEGER_C,
                               &hex) == D_ARG_OK)                      &&
          (d_arg_parse_uintmax(d_pack_text_from_cstr("-1"),
                               D_ARG_INTEGER_C,
                               &refused) == D_ARG_ERROR_CONVERSION) );
    const bool wide  = ( (INTMAX_MAX == INT64_MAX)                      &&
                         (low == INT64_MIN)                             &&
                         (high == INT64_MAX)                            &&
                         (top == UINT64_MAX)                            &&
                         (hex == UINT64_MAX)                            &&
                         (refused == 7u) );
    const bool past  =
        ( (d_arg_parse_intmax(d_pack_text_from_cstr("9223372036854775808"),
                              D_ARG_INTEGER_C,
                              &low) == D_ARG_ERROR_CONVERSION)         &&
          (d_arg_parse_uintmax(d_pack_text_from_cstr("18446744073709551616"),
                               D_ARG_INTEGER_C,
                               &top) == D_ARG_ERROR_CONVERSION) );

    return ( (read) &&
             (wide) &&
             (past) );
}

/*
d_tests_arg_type_integer_widths
  Every integer type at its limits, through its descriptor.
  Tests the following:
  - each signed type takes its minimum and maximum and refuses one past
    either, and each unsigned type its maximum, refusing one past it and any
    negative value
  - the fixed-width and the platform-width types alike
*/
bool
d_tests_arg_type_integer_widths(void)
{
    bool held = true;

    // each signed row
    for (size_t index = 0u;
         index < sizeof(SIGNED_ROWS) / sizeof(SIGNED_ROWS[0]);
         ++index)
    {
        const struct d_tests_arg_type_signed_row* const row =
            &SIGNED_ROWS[index];
        union d_tests_arg_type_value value  = { false };
        const int                    status =
            d_tests_arg_type_convert(row->builtin,
                                     row->text,
                                     &value);

        held = ( (held) &&
                 ( (row->converts)
                   ? ( (status == D_ARG_OK) &&
                       (d_tests_arg_type_signed_of(row->builtin,
                                                   &value) == row->value) )
                   : (status == D_ARG_ERROR_CONVERSION) ) );
    }

    // each unsigned row
    for (size_t index = 0u;
         index < sizeof(UNSIGNED_ROWS) / sizeof(UNSIGNED_ROWS[0]);
         ++index)
    {
        const struct d_tests_arg_type_unsigned_row* const row =
            &UNSIGNED_ROWS[index];
        union d_tests_arg_type_value value  = { false };
        const int                    status =
            d_tests_arg_type_convert(row->builtin,
                                     row->text,
                                     &value);

        held = ( (held) &&
                 ( (row->converts)
                   ? ( (status == D_ARG_OK) &&
                       (d_tests_arg_type_unsigned_of(row->builtin,
                                                     &value) == row->value) )
                   : (status == D_ARG_ERROR_CONVERSION) ) );
    }

    return held;
}

/*
d_tests_arg_type_characters
  The three character types.
  Tests the following:
  - `char` takes a single character as itself, a space included, and longer
    text as an 8-bit integer, `true` included
  - `signed char` and `unsigned char` take integers only
*/
bool
d_tests_arg_type_characters(void)
{
    bool held = true;

    // each row
    for (size_t index = 0u;
         index < sizeof(CHAR_ROWS) / sizeof(CHAR_ROWS[0]);
         ++index)
    {
        const struct d_tests_arg_type_char_row* const row = &CHAR_ROWS[index];
        union d_tests_arg_type_value value  = { false };
        const int                    status =
            d_tests_arg_type_convert(row->builtin,
                                     row->text,
                                     &value);
        const int                    got    =
            (row->builtin == D_ARG_BUILTIN_CHAR)
            ? (int)(signed char)value.character
            : ( (row->builtin == D_ARG_BUILTIN_SIGNED_CHAR)
                ? (int)value.signed_char
                : (int)value.unsigned_char );

        held = ( (held) &&
                 ( (row->converts)
                   ? ( (status == D_ARG_OK) &&
                       (got == row->value) )
                   : (status == D_ARG_ERROR_CONVERSION) ) );
    }

    return held;
}

//==============================================================================
// 2.2  FLOATING POINT
//==============================================================================

/*
d_tests_arg_type_floating
  d_arg_parse_double, and the narrower and wider types.
  Tests the following:
  - signs, decimal points, exponents, hexadecimal, whitespace around the
    number, separators, leading zeros as decimal
  - what is refused, the output then untouched
  - `float` rounds once, to its own precision, and overflows to infinity;
    `long double` reads as `double` does
*/
bool
d_tests_arg_type_floating(void)
{
    bool held = true;

    // each row
    for (size_t index = 0u;
         index < sizeof(DOUBLE_ROWS) / sizeof(DOUBLE_ROWS[0]);
         ++index)
    {
        double    value  = -999.0;
        const int status = d_arg_parse_double(
                               d_pack_text_from_cstr(DOUBLE_ROWS[index].text),
                               &value);

        held = ( (held) &&
                 ( (DOUBLE_ROWS[index].converts)
                   ? ( (status == D_ARG_OK) &&
                       (value == DOUBLE_ROWS[index].value) )
                   : ( (status == D_ARG_ERROR_CONVERSION) &&
                       (value == -999.0) ) ) );
    }

    union d_tests_arg_type_value tenth = { false };
    union d_tests_arg_type_value most  = { false };
    union d_tests_arg_type_value over  = { false };
    long double                  wide  = 0.0L;

    const bool narrow = ( (d_tests_arg_type_convert(D_ARG_BUILTIN_FLOAT,
                                                    "0.1",
                                                    &tenth) == D_ARG_OK)    &&
                          (tenth.single == 0.1f)                            &&
                          (d_tests_arg_type_convert(D_ARG_BUILTIN_FLOAT,
                                                    "3.4028235e38",
                                                    &most) == D_ARG_OK)     &&
                          (most.single == FLT_MAX)                          &&
                          (d_tests_arg_type_convert(D_ARG_BUILTIN_FLOAT,
                                                    "3.5e38",
                                                    &over) == D_ARG_OK)     &&
                          (isinf(over.single)) );
    const bool extended =
        ( (d_arg_parse_long_double(d_pack_text_from_cstr("2.5"),
                                   &wide) == D_ARG_OK) &&
          (wide == 2.5L) );

    return ( (held)   &&
             (narrow) &&
             (extended) );
}

/*
d_tests_arg_type_floating_special
  Infinities, NaNs and signed zeros.
  Tests the following:
  - `inf`, `infinity` and `nan` in any case, with a sign
  - a value too large is an infinity of its sign, as CLI11 reads it
  - `-0` keeps its sign
*/
bool
d_tests_arg_type_floating_special(void)
{
    double up      = 0.0;
    double down    = 0.0;
    double word    = 0.0;
    double missing = 0.0;
    double huge    = 0.0;
    double tiny    = 0.0;
    double zero    = 0.0;

    const bool read = ( (d_arg_parse_double(d_pack_text_from_cstr("INF"),
                                            &up) == D_ARG_OK)          &&
                        (d_arg_parse_double(d_pack_text_from_cstr("-inf"),
                                            &down) == D_ARG_OK)        &&
                        (d_arg_parse_double(
                             d_pack_text_from_cstr("infinity"),
                             &word) == D_ARG_OK)                       &&
                        (d_arg_parse_double(d_pack_text_from_cstr("NaN"),
                                            &missing) == D_ARG_OK)     &&
                        (d_arg_parse_double(d_pack_text_from_cstr("1e400"),
                                            &huge) == D_ARG_OK)        &&
                        (d_arg_parse_double(d_pack_text_from_cstr("-1e400"),
                                            &tiny) == D_ARG_OK)        &&
                        (d_arg_parse_double(d_pack_text_from_cstr("-0"),
                                            &zero) == D_ARG_OK) );

    return ( (read)                                  &&
             (isinf(up))                             &&
             (up > 0.0)                              &&
             (isinf(down))                           &&
             (down < 0.0)                            &&
             (isinf(word))                           &&
             (isnan(missing))                        &&
             (isinf(huge))                           &&
             (isinf(tiny))                           &&
             (tiny < 0.0)                            &&
             (zero == 0.0)                           &&
             (signbit(zero)) );
}

/*
d_tests_arg_type_floating_locale
  The decimal point, whatever the locale says.
  Tests the following:
  - under a locale whose decimal point is a comma, `1.5` still reads as one
    and a half, `1,5` is still refused, and a value is still written with a
    point
  A system with no such locale is checked under the C locale only, and the
section says so.
*/
bool
d_tests_arg_type_floating_locale(void)
{
    static const char* const NAMES[] =
    {
        "de_DE.UTF-8", "de_DE.utf8", "de_DE", "fr_FR.UTF-8", "fr_FR",
        "German", "French"
    };
    const char* found = NULL;

    // the first locale with a decimal comma
    for (size_t index = 0u;
         (found == NULL) && (index < sizeof(NAMES) / sizeof(NAMES[0]));
         ++index)
    {
        if ( (setlocale(LC_NUMERIC, NAMES[index]) != NULL) &&
             (strcmp(localeconv()->decimal_point, ",") == 0) )
        {
            found = NAMES[index];
        }
    }

    // none here: the C locale is what is checked
    if (!found)
    {
        (void)setlocale(LC_NUMERIC, "C");
        printf("    no locale with a decimal comma here: checked under "
               "the C locale\n");
    }

    double     point = 0.0;
    double     comma = 0.0;
    const bool read  = ( (d_arg_parse_double(d_pack_text_from_cstr("1.5"),
                                             &point) == D_ARG_OK)      &&
                         (point == 1.5)                                &&
                         (d_arg_parse_double(d_pack_text_from_cstr("1,5"),
                                             &comma)
                          == D_ARG_ERROR_CONVERSION) );
    const double quarter = 2.25;
    const bool   written = d_tests_arg_type_writes(D_ARG_BUILTIN_DOUBLE,
                                                   &quarter,
                                                   "2.25");

    (void)setlocale(LC_NUMERIC, "C");

    return ( (read) &&
             (written) );
}

//==============================================================================
// 2.3  TRUTH VALUES
//==============================================================================

/*
d_tests_arg_type_bool
  d_arg_parse_bool under the C rule, CLI11's.
  Tests the following:
  - the true and false words in any case, `+` and `-`
  - numbers true above zero, false at or below it, however large
  - what is refused, the output then untouched
*/
bool
d_tests_arg_type_bool(void)
{
    bool held = true;

    // each row
    for (size_t index = 0u;
         index < sizeof(BOOL_ROWS) / sizeof(BOOL_ROWS[0]);
         ++index)
    {
        bool      value  = !BOOL_ROWS[index].value;
        const int status = d_arg_parse_bool(
                               d_pack_text_from_cstr(BOOL_ROWS[index].text),
                               D_ARG_INTEGER_C,
                               &value);
        const bool row   = (BOOL_ROWS[index].converts)
                           ? ( (status == D_ARG_OK) &&
                               (value == BOOL_ROWS[index].value) )
                           : (status == D_ARG_ERROR_CONVERSION);

        // a row that did not hold is shown
        if (!row)
        {
            printf("    [%s]: status %d, value %d\n",
                   BOOL_ROWS[index].text,
                   status,
                   (int)value);
        }

        held = ( (held) &&
                 (row) );
    }

    return held;
}

/*
d_tests_arg_type_flag_value
  d_arg_flag_value under the C rule, CLI11's to_flag_value.
  Tests the following:
  - true words 1 and false words -1, `0` -1 but `-0` 0, numbers themselves
  - what is refused, the output then untouched
*/
bool
d_tests_arg_type_flag_value(void)
{
    bool held = true;

    // each row
    for (size_t index = 0u;
         index < sizeof(FLAG_ROWS) / sizeof(FLAG_ROWS[0]);
         ++index)
    {
        long      value  = 999L;
        const int status = d_arg_flag_value(
                               d_pack_text_from_cstr(FLAG_ROWS[index].text),
                               D_ARG_INTEGER_C,
                               &value);
        const bool row   = (FLAG_ROWS[index].converts)
                           ? ( (status == D_ARG_OK) &&
                               (value == FLAG_ROWS[index].value) )
                           : ( (status == D_ARG_ERROR_CONVERSION) &&
                               (value == 999L) );

        // a row that did not hold is shown
        if (!row)
        {
            printf("    [%s]: status %d, value %ld\n",
                   FLAG_ROWS[index].text,
                   status,
                   value);
        }

        held = ( (held) &&
                 (row) );
    }

    return held;
}

/*
d_tests_arg_type_flag_value_intmax
  d_arg_flag_value_intmax under the C rule, CLI11's to_flag_value over the
64 bits it reads, and the `long` d_arg_flag_value narrows it to.
  Tests the following:
  - the 64-bit limits, decimal and hexadecimal, and one past each, refused
    with the output untouched
  - numbers past 32 bits, which a 32-bit `long` cannot hold
  - the words, digits and spaces both readers read alike
  - d_arg_flag_value reads a number past 32 bits as wide as the platform's
    `long` is: where it is 32 bits, it refuses it
*/
bool
d_tests_arg_type_flag_value_intmax(void)
{
    bool held = true;

    // each row
    for (size_t index = 0u;
         index < sizeof(FLAG_WIDE_ROWS) / sizeof(FLAG_WIDE_ROWS[0]);
         ++index)
    {
        const struct d_tests_arg_type_wide_row* const wide =
            &FLAG_WIDE_ROWS[index];
        intmax_t                                      value  = 999;
        const int                                     status =
            d_arg_flag_value_intmax(d_pack_text_from_cstr(wide->text),
                                    D_ARG_INTEGER_C,
                                    &value);
        const bool                                    row    =
            (wide->converts)
            ? ( (status == D_ARG_OK) &&
                (value == (intmax_t)wide->value) )
            : ( (status == D_ARG_ERROR_CONVERSION) &&
                (value == 999) );

        // a row that did not hold is shown
        if (!row)
        {
            printf("    [%s]: status %d, value %lld\n",
                   wide->text,
                   status,
                   (long long)value);
        }

        held = ( (held) &&
                 (row) );
    }

    long      narrow = 7L;
    const int beyond = d_arg_flag_value(d_pack_text_from_cstr("4294967296"),
                                        D_ARG_INTEGER_C,
                                        &narrow);

    // past 32 bits, as wide as the platform's `long` is
    return ( (held) &&
             ( (LONG_MAX > 2147483647L)
               ? ( (beyond == D_ARG_OK) &&
                   ((long long)narrow == 4294967296LL) )
               : ( (beyond == D_ARG_ERROR_CONVERSION) &&
                   (narrow == 7L) ) ) );
}

//==============================================================================
// 2.4  THROUGH A DESCRIPTOR
//==============================================================================

/*
d_tests_arg_type_empty_is_zero
  An empty value, and none at all, through every built-in descriptor.
  Tests the following:
  - each type takes its zero, as CLI11's lexical_assign value-initializes:
    0, `false`, NUL, empty text
*/
bool
d_tests_arg_type_empty_is_zero(void)
{
    const struct d_pack_text none = { "", 0u };
    bool                     held = true;

    // each type, with one empty value and with no value
    for (int builtin = 0; builtin < (int)D_ARG_BUILTIN_COUNT; ++builtin)
    {
        const struct d_arg_type* const type  = d_arg_type_builtin(builtin);
        union d_tests_arg_type_value   one   = { false };
        union d_tests_arg_type_value   empty = { false };

        memset(&one,
               0x5a,
               sizeof(one));
        memset(&empty,
               0x5a,
               sizeof(empty));

        const int first  = d_arg_convert(type,
                                         &none,
                                         1u,
                                         &one,
                                         NULL);
        const int second = d_arg_convert(type,
                                         NULL,
                                         0u,
                                         &empty,
                                         NULL);

        held = ( (held)                                  &&
                 (first == D_ARG_OK)                     &&
                 (second == D_ARG_OK)                    &&
                 (d_tests_arg_type_zero(builtin,
                                        &one))           &&
                 (d_tests_arg_type_zero(builtin,
                                        &empty)) );
    }

    return held;
}

/*
d_tests_arg_type_failure_keeps_target
  A value no number, truth value or character type can take.
  Tests the following:
  - every one of those types refuses `x7`, and leaves every byte of its
    variable as it was, where CLI11's conversions leave a partial result
*/
bool
d_tests_arg_type_failure_keeps_target(void)
{
    bool held = true;

    // each type but the text types, which take anything
    for (int builtin = 0; builtin < (int)D_ARG_BUILTIN_STRING; ++builtin)
    {
        union d_tests_arg_type_value value  = { false };
        union d_tests_arg_type_value before = { false };

        memset(&value,
               0x5a,
               sizeof(value));
        memcpy(&before,
               &value,
               sizeof(value));

        const int status = d_tests_arg_type_convert(builtin,
                                                    "x7",
                                                    &value);

        held = ( (held)                                &&
                 (status == D_ARG_ERROR_CONVERSION)    &&
                 (memcmp(&value,
                         &before,
                         sizeof(value)) == 0) );
    }

    return held;
}

/*
d_tests_arg_type_text_types
  The two text types, which keep the value rather than copy it.
  Tests the following:
  - D_ARG_BUILTIN_STRING points at the value's own bytes
  - D_ARG_BUILTIN_TEXT is the value's span, its length included
  - with several values, each takes the first
*/
bool
d_tests_arg_type_text_types(void)
{
    const struct d_pack_text values[2] =
    {
        { "first",  5u },
        { "second", 6u }
    };
    union d_tests_arg_type_value string = { false };
    union d_tests_arg_type_value text   = { false };

    const int pointed = d_arg_convert(
                            d_arg_type_builtin(D_ARG_BUILTIN_STRING),
                            values,
                            2u,
                            &string,
                            NULL);
    const int spanned = d_arg_convert(
                            d_arg_type_builtin(D_ARG_BUILTIN_TEXT),
                            values,
                            2u,
                            &text,
                            NULL);

    return ( (pointed == D_ARG_OK)              &&
             (string.string == values[0].data)  &&
             (spanned == D_ARG_OK)              &&
             (text.text.data == values[0].data) &&
             (text.text.length == 5u) );
}

/*
d_tests_arg_type_format
  The text each built-in type writes for a value.
  Tests the following:
  - every row of FORMAT_ROWS
  - the infinities and NaN in one spelling on every system
*/
bool
d_tests_arg_type_format(void)
{
    bool held = true;

    // each row
    for (size_t index = 0u;
         index < sizeof(FORMAT_ROWS) / sizeof(FORMAT_ROWS[0]);
         ++index)
    {
        held = ( (held) &&
                 (d_tests_arg_type_writes(FORMAT_ROWS[index].builtin,
                                          FORMAT_ROWS[index].value,
                                          FORMAT_ROWS[index].text)) );
    }

    const double infinite = HUGE_VAL;
    const double below    = -HUGE_VAL;
    const double missing  = (double)NAN;

    return ( (held)                                               &&
             (d_tests_arg_type_writes(D_ARG_BUILTIN_DOUBLE,
                                      &infinite,
                                      "inf"))                     &&
             (d_tests_arg_type_writes(D_ARG_BUILTIN_DOUBLE,
                                      &below,
                                      "-inf"))                    &&
             (d_tests_arg_type_writes(D_ARG_BUILTIN_DOUBLE,
                                      &missing,
                                      "nan")) );
}

/*
d_tests_arg_type_arguments
  The readers given nowhere to put a value.
  Tests the following:
  - each reader refuses a NULL output as a bad argument
*/
bool
d_tests_arg_type_arguments(void)
{
    const struct d_pack_text one = { "1", 1u };

    return ( (d_arg_parse_long(one,
                               D_ARG_INTEGER_C,
                               NULL) == D_ARG_ERROR_BAD_ARGUMENT)   &&
             (d_arg_parse_ulong(one,
                                D_ARG_INTEGER_C,
                                NULL) == D_ARG_ERROR_BAD_ARGUMENT)  &&
             (d_arg_parse_intmax(one,
                                 D_ARG_INTEGER_C,
                                 NULL) == D_ARG_ERROR_BAD_ARGUMENT) &&
             (d_arg_parse_double(one,
                                 NULL) == D_ARG_ERROR_BAD_ARGUMENT) &&
             (d_arg_parse_long_double(one,
                                      NULL)
              == D_ARG_ERROR_BAD_ARGUMENT)                          &&
             (d_arg_parse_bool(one,
                               D_ARG_INTEGER_C,
                               NULL) == D_ARG_ERROR_BAD_ARGUMENT)   &&
             (d_arg_flag_value(one,
                               D_ARG_INTEGER_C,
                               NULL) == D_ARG_ERROR_BAD_ARGUMENT) );
}

/*
d_tests_arg_type_descriptor_arguments
  Conversions, writes and sums given nothing to work with.
  Tests the following:
  - d_arg_convert refuses a NULL descriptor or target, and d_arg_format a
    NULL descriptor or value, as bad arguments
  - a descriptor with no writer is unsupported for d_arg_format
  - d_arg_sum_values refuses a NULL buffer, and NULL values with a count
*/
bool
d_tests_arg_type_descriptor_arguments(void)
{
    const struct d_pack_text one    = { "1", 1u };
    struct d_arg_buffer      buffer = D_ARG_BUFFER_INIT;
    const struct d_pack_sink sink   = d_arg_buffer_sink(&buffer);
    int                      target = 0;

    const bool through = ( (d_arg_convert(NULL,
                                          &one,
                                          1u,
                                          &target,
                                          NULL)
                            == D_ARG_ERROR_BAD_ARGUMENT)   &&
                           (d_arg_convert(&COLOR,
                                          &one,
                                          1u,
                                          NULL,
                                          NULL)
                            == D_ARG_ERROR_BAD_ARGUMENT)   &&
                           (d_arg_format(NULL,
                                         &target,
                                         sink)
                            == D_ARG_ERROR_BAD_ARGUMENT)   &&
                           (d_arg_format(&COLOR,
                                         NULL,
                                         sink)
                            == D_ARG_ERROR_BAD_ARGUMENT)   &&
                           (d_arg_format(&NO_FORMAT,
                                         &target,
                                         sink)
                            == D_ARG_ERROR_UNSUPPORTED) );
    const bool sums    = ( (d_arg_sum_values(&one,
                                             1u,
                                             D_ARG_INTEGER_C,
                                             NULL)
                            == D_ARG_ERROR_BAD_ARGUMENT)   &&
                           (d_arg_sum_values(NULL,
                                             1u,
                                             D_ARG_INTEGER_C,
                                             &buffer)
                            == D_ARG_ERROR_BAD_ARGUMENT) );

    d_arg_buffer_release(&buffer);

    return ( (through) &&
             (sums) );
}

/*
d_tests_arg_type_custom_descriptor
  A descriptor of the caller's own, through d_arg_convert and d_arg_format.
  Tests the following:
  - the caller's converter runs, with the context's rule and buffer
  - a refusal's reason reaches the buffer the caller gave
  - the caller's writer writes the value
*/
bool
d_tests_arg_type_custom_descriptor(void)
{
    const struct d_pack_text           green   = { "green", 5u };
    const struct d_pack_text           purple  = { "purple", 6u };
    struct d_arg_buffer                reason  = D_ARG_BUFFER_INIT;
    struct d_arg_buffer                written = D_ARG_BUFFER_INIT;
    const struct d_arg_convert_context context =
    {
        D_ARG_INTEGER_DECIMAL,
        NULL,
        &reason
    };
    int                                color   = -1;

    const bool read    = ( (d_arg_convert(&COLOR,
                                          &green,
                                          1u,
                                          &color,
                                          &context) == D_ARG_OK)          &&
                           (color == COLOR_GREEN) );
    const bool refused = ( (d_arg_convert(&COLOR,
                                          &purple,
                                          1u,
                                          &color,
                                          &context)
                            == D_ARG_ERROR_CONVERSION)                    &&
                           (color == COLOR_GREEN)                         &&
                           (d_tests_arg_same("reason",
                                             d_arg_buffer_text(&reason),
                                             "not a color: purple")) );
    const bool wrote   = ( (d_arg_format(&COLOR,
                                         &color,
                                         d_arg_buffer_sink(&written))
                            == D_ARG_OK)                                  &&
                           (d_tests_arg_same("written",
                                             d_arg_buffer_text(&written),
                                             "green")) );

    d_arg_buffer_release(&reason);
    d_arg_buffer_release(&written);

    return ( (read)    &&
             (refused) &&
             (wrote) );
}

//==============================================================================
// 2.5  SUMS
//==============================================================================

/*
d_tests_arg_type_sums
  d_arg_sum_values, CLI11's sum_string_vector.
  Tests the following:
  - integers summed exactly, past what a `double` holds exactly
  - a sum that overflows, or any floating-point value, summed as `double`
    and written to sixteen significant digits
  - flag words as their values, text concatenated, no values as 0
  - the decimal rule reads leading zeros as decimal
*/
bool
d_tests_arg_type_sums(void)
{
    struct d_arg_buffer buffer = D_ARG_BUFFER_INIT;
    bool                held   = true;

    // each row
    for (size_t index = 0u;
         index < sizeof(SUM_ROWS) / sizeof(SUM_ROWS[0]);
         ++index)
    {
        struct d_pack_text values[4] = { { NULL, 0u } };

        // the row's values as spans
        for (size_t at = 0u; at < SUM_ROWS[index].count; ++at)
        {
            values[at] = d_pack_text_from_cstr(SUM_ROWS[index].values[at]);
        }

        d_arg_buffer_clear(&buffer);
        held = ( (held)                                                  &&
                 (d_arg_sum_values(values,
                                   SUM_ROWS[index].count,
                                   D_ARG_INTEGER_C,
                                   &buffer) == D_ARG_OK)                 &&
                 (d_tests_arg_same("sum",
                                   d_arg_buffer_text(&buffer),
                                   SUM_ROWS[index].sum)) );
    }

    const struct d_pack_text decimal[2] = { { "010", 3u }, { "1", 1u } };

    d_arg_buffer_clear(&buffer);

    const bool ruled = ( (d_arg_sum_values(decimal,
                                           2u,
                                           D_ARG_INTEGER_DECIMAL,
                                           &buffer) == D_ARG_OK)         &&
                         (d_tests_arg_same("decimal sum",
                                           d_arg_buffer_text(&buffer),
                                           "11")) );

    d_arg_buffer_release(&buffer);

    return ( (held) &&
             (ruled) );
}

//==============================================================================
// 3.  THE SUITE
//==============================================================================

// d_tests_arg_type
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_type[] =
{
    { "arg_type_builtin_descriptors",
      d_tests_arg_type_builtin_descriptors },
    { "arg_type_long_c_rule",
      d_tests_arg_type_long_c_rule },
    { "arg_type_long_decimal_rule",
      d_tests_arg_type_long_decimal_rule },
    { "arg_type_ulong",
      d_tests_arg_type_ulong },
    { "arg_type_intmax",
      d_tests_arg_type_intmax },
    { "arg_type_integer_widths",
      d_tests_arg_type_integer_widths },
    { "arg_type_characters",
      d_tests_arg_type_characters },
    { "arg_type_floating",
      d_tests_arg_type_floating },
    { "arg_type_floating_special",
      d_tests_arg_type_floating_special },
    { "arg_type_floating_locale",
      d_tests_arg_type_floating_locale },
    { "arg_type_bool",
      d_tests_arg_type_bool },
    { "arg_type_flag_value",
      d_tests_arg_type_flag_value },
    { "arg_type_flag_value_intmax",
      d_tests_arg_type_flag_value_intmax },
    { "arg_type_empty_is_zero",
      d_tests_arg_type_empty_is_zero },
    { "arg_type_failure_keeps_target",
      d_tests_arg_type_failure_keeps_target },
    { "arg_type_text_types",
      d_tests_arg_type_text_types },
    { "arg_type_format",
      d_tests_arg_type_format },
    { "arg_type_arguments",
      d_tests_arg_type_arguments },
    { "arg_type_descriptor_arguments",
      d_tests_arg_type_descriptor_arguments },
    { "arg_type_custom_descriptor",
      d_tests_arg_type_custom_descriptor },
    { "arg_type_sums",
      d_tests_arg_type_sums }
};

// d_tests_arg_type_count
//   constant: how many sections the table above holds in this build.
const size_t d_tests_arg_type_count =
    sizeof(d_tests_arg_type) / sizeof(d_tests_arg_type[0]);
