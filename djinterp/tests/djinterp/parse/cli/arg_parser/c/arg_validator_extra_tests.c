/*******************************************************************************
* djinterp [parse]                                   arg_validator_extra_tests.c
*
* The suite of CLI11's extra checks, declared in arg_validator_extra_tests.h.
*   The transcripts are CLI11 2.7.2's, written as the core suite writes them
* (arg_tests_support.h): each check run over the values CLI11's was run
* over, under CLI11's rule for leading zeros. Two kinds of line are not
* CLI11's, and only the departures section has them: the numbers a bound
* makes, which CLI11 writes to every digit the type holds or as a character,
* and the word `true` in an address, which CLI11 reads as 1. A `long` is as
* wide as the system makes it, as in CLI11, so 2147483648 is a `long` where
* `long` has 64 bits and is not where it has 32.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_validator_extra_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./arg_validator_extra_tests.h"  // corresponding header
// std
#include <limits.h>   // LONG_MAX
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memcmp, memcpy
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knob
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // descriptors
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // composites
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_extra.h"  // the checks
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // transcripts, parses, kinds


#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// NUMBERS
//   constant: the values the checks of numbers are run over, as CLI11's
// were: integers in and out of every range here, fractions, words, spaces,
// signs, overflow, and the separators and prefixes the readers know.
static const char* const NUMBERS[] =
{
    "5", "0", "10", "11", "-1", "abc", "", "5.5", " 5", "0x5", "010", "08",
    "1e0", "2.5", "2.6", "nan", "inf", "-0", "1e-320", "-1e-300", "c", "99",
    "z", "3", "7", "12", "30", "\"5\"", "1_000", "true", "false", "+5",
    " 7 ", "300", "-129", "2147483648", "18446744073709551616", "a", "ab",
    "0.1", "0.3", "-5", "1'0", "0b1", "0o7", "0B1", "-0b1", "0x1_0"
};

// NUMBER_COUNT
//   constant: how many values NUMBERS holds.
static const size_t NUMBER_COUNT = sizeof(NUMBERS) / sizeof(NUMBERS[0]);

// ADDRESSES
//   constant: the values ValidIPV4 is run over, as CLI11's was.
static const char* const ADDRESSES[] =
{
    "1.2.3.4", "255.255.255.255", "256.1.1.1", "1.2.3", "1.2.3.4.5",
    "a.b.c.d", "1..2.3", ".1.2.3", "-1.2.3.4", "0x7f.0.0.1", "010.1.1.1",
    "08.1.1.1", " 1.2.3.4", "1.2.3.4 ", "1 .2.3.4", "2147483648.1.1.1",
    "1_0.0.0.1", "", "...", "0.0.0.0", "-0.0.0.0", "+1.2.3.4", "0b1.0.0.0",
    "0o7.0.0.0", "1.2.3.-1", "999.0.0.0"
};

// ADDRESS_COUNT
//   constant: how many values ADDRESSES holds.
static const size_t ADDRESS_COUNT = sizeof(ADDRESSES) / sizeof(ADDRESSES[0]);

// TRUE_ADDRESSES
//   constant: addresses with the word `true` for a part, which CLI11's
// ValidIPV4 takes, beside two with a dot too many, which it does not.
static const char* const TRUE_ADDRESSES[] =
{
    "1.2.3.", "true.0.0.1", "true.1.1.1", "1.2.3.4.", "0.0.0.true"
};

// TRUE_ADDRESS_COUNT
//   constant: how many values TRUE_ADDRESSES holds.
static const size_t TRUE_ADDRESS_COUNT =
    sizeof(TRUE_ADDRESSES) / sizeof(TRUE_ADDRESSES[0]);

// CLAMPED
//   constant: the values the bounds that depart from CLI11 are run over:
// below, inside and above bounds from 0.1 to 0.3, and from 65 to 100.
static const char* const CLAMPED[] =
{
    "0.05", "0.5", "0.2", "-1", "120", "50"
};

// CLAMPED_COUNT
//   constant: how many values CLAMPED holds.
static const size_t CLAMPED_COUNT = sizeof(CLAMPED) / sizeof(CLAMPED[0]);

// T_INT
//   constant: CLI11's TypeValidator<int>.
static const char T_INT[] =
    "desc <INT>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> fail <Failed parsing abc as a INT>\n"
    "[] -> fail <Failed parsing  as a INT>\n"
    "[5.5] -> fail <Failed parsing 5.5 as a INT>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> fail <Failed parsing 08 as a INT>\n"
    "[1e0] -> fail <Failed parsing 1e0 as a INT>\n"
    "[2.5] -> fail <Failed parsing 2.5 as a INT>\n"
    "[2.6] -> fail <Failed parsing 2.6 as a INT>\n"
    "[nan] -> fail <Failed parsing nan as a INT>\n"
    "[inf] -> fail <Failed parsing inf as a INT>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <Failed parsing 1e-320 as a INT>\n"
    "[-1e-300] -> fail <Failed parsing -1e-300 as a INT>\n"
    "[c] -> fail <Failed parsing c as a INT>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Failed parsing z as a INT>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Failed parsing \"5\" as a INT>\n"
    "[1_000] -> pass <1_000>\n"
    "[true] -> pass <true>\n"
    "[false] -> fail <Failed parsing false as a INT>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> pass <300>\n"
    "[-129] -> pass <-129>\n"
    "[2147483648] -> fail <Failed parsing 2147483648 as a INT>\n"
    "[18446744073709551616] -> fail <Failed parsing 18446744073709551616 as a "
    "INT>\n"
    "[a] -> fail <Failed parsing a as a INT>\n"
    "[ab] -> fail <Failed parsing ab as a INT>\n"
    "[0.1] -> fail <Failed parsing 0.1 as a INT>\n"
    "[0.3] -> fail <Failed parsing 0.3 as a INT>\n"
    "[-5] -> pass <-5>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <0o7>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> pass <-0b1>\n"
    "[0x1_0] -> pass <0x1_0>\n";

// T_UINT
//   constant: CLI11's TypeValidator<unsigned int>.
static const char T_UINT[] =
    "desc <UINT>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> fail <Failed parsing -1 as a UINT>\n"
    "[abc] -> fail <Failed parsing abc as a UINT>\n"
    "[] -> fail <Failed parsing  as a UINT>\n"
    "[5.5] -> fail <Failed parsing 5.5 as a UINT>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> fail <Failed parsing 08 as a UINT>\n"
    "[1e0] -> fail <Failed parsing 1e0 as a UINT>\n"
    "[2.5] -> fail <Failed parsing 2.5 as a UINT>\n"
    "[2.6] -> fail <Failed parsing 2.6 as a UINT>\n"
    "[nan] -> fail <Failed parsing nan as a UINT>\n"
    "[inf] -> fail <Failed parsing inf as a UINT>\n"
    "[-0] -> fail <Failed parsing -0 as a UINT>\n"
    "[1e-320] -> fail <Failed parsing 1e-320 as a UINT>\n"
    "[-1e-300] -> fail <Failed parsing -1e-300 as a UINT>\n"
    "[c] -> fail <Failed parsing c as a UINT>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Failed parsing z as a UINT>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Failed parsing \"5\" as a UINT>\n"
    "[1_000] -> pass <1_000>\n"
    "[true] -> fail <Failed parsing true as a UINT>\n"
    "[false] -> fail <Failed parsing false as a UINT>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> pass <300>\n"
    "[-129] -> fail <Failed parsing -129 as a UINT>\n"
    "[2147483648] -> pass <2147483648>\n"
    "[18446744073709551616] -> fail <Failed parsing 18446744073709551616 as a "
    "UINT>\n"
    "[a] -> fail <Failed parsing a as a UINT>\n"
    "[ab] -> fail <Failed parsing ab as a UINT>\n"
    "[0.1] -> fail <Failed parsing 0.1 as a UINT>\n"
    "[0.3] -> fail <Failed parsing 0.3 as a UINT>\n"
    "[-5] -> fail <Failed parsing -5 as a UINT>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <0o7>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> fail <Failed parsing -0b1 as a UINT>\n"
    "[0x1_0] -> pass <0x1_0>\n";

// T_I8
//   constant: CLI11's TypeValidator<int8_t>.
static const char T_I8[] =
    "desc <INT>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> fail <Failed parsing abc as a INT>\n"
    "[] -> fail <Failed parsing  as a INT>\n"
    "[5.5] -> fail <Failed parsing 5.5 as a INT>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> fail <Failed parsing 08 as a INT>\n"
    "[1e0] -> fail <Failed parsing 1e0 as a INT>\n"
    "[2.5] -> fail <Failed parsing 2.5 as a INT>\n"
    "[2.6] -> fail <Failed parsing 2.6 as a INT>\n"
    "[nan] -> fail <Failed parsing nan as a INT>\n"
    "[inf] -> fail <Failed parsing inf as a INT>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <Failed parsing 1e-320 as a INT>\n"
    "[-1e-300] -> fail <Failed parsing -1e-300 as a INT>\n"
    "[c] -> fail <Failed parsing c as a INT>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Failed parsing z as a INT>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Failed parsing \"5\" as a INT>\n"
    "[1_000] -> fail <Failed parsing 1_000 as a INT>\n"
    "[true] -> pass <true>\n"
    "[false] -> fail <Failed parsing false as a INT>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> fail <Failed parsing 300 as a INT>\n"
    "[-129] -> fail <Failed parsing -129 as a INT>\n"
    "[2147483648] -> fail <Failed parsing 2147483648 as a INT>\n"
    "[18446744073709551616] -> fail <Failed parsing 18446744073709551616 as a "
    "INT>\n"
    "[a] -> fail <Failed parsing a as a INT>\n"
    "[ab] -> fail <Failed parsing ab as a INT>\n"
    "[0.1] -> fail <Failed parsing 0.1 as a INT>\n"
    "[0.3] -> fail <Failed parsing 0.3 as a INT>\n"
    "[-5] -> pass <-5>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <0o7>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> pass <-0b1>\n"
    "[0x1_0] -> pass <0x1_0>\n";

// T_U64
//   constant: CLI11's TypeValidator<uint64_t>.
static const char T_U64[] =
    "desc <UINT>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> fail <Failed parsing -1 as a UINT>\n"
    "[abc] -> fail <Failed parsing abc as a UINT>\n"
    "[] -> fail <Failed parsing  as a UINT>\n"
    "[5.5] -> fail <Failed parsing 5.5 as a UINT>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> fail <Failed parsing 08 as a UINT>\n"
    "[1e0] -> fail <Failed parsing 1e0 as a UINT>\n"
    "[2.5] -> fail <Failed parsing 2.5 as a UINT>\n"
    "[2.6] -> fail <Failed parsing 2.6 as a UINT>\n"
    "[nan] -> fail <Failed parsing nan as a UINT>\n"
    "[inf] -> fail <Failed parsing inf as a UINT>\n"
    "[-0] -> fail <Failed parsing -0 as a UINT>\n"
    "[1e-320] -> fail <Failed parsing 1e-320 as a UINT>\n"
    "[-1e-300] -> fail <Failed parsing -1e-300 as a UINT>\n"
    "[c] -> fail <Failed parsing c as a UINT>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Failed parsing z as a UINT>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Failed parsing \"5\" as a UINT>\n"
    "[1_000] -> pass <1_000>\n"
    "[true] -> fail <Failed parsing true as a UINT>\n"
    "[false] -> fail <Failed parsing false as a UINT>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> pass <300>\n"
    "[-129] -> fail <Failed parsing -129 as a UINT>\n"
    "[2147483648] -> pass <2147483648>\n"
    "[18446744073709551616] -> fail <Failed parsing 18446744073709551616 as a "
    "UINT>\n"
    "[a] -> fail <Failed parsing a as a UINT>\n"
    "[ab] -> fail <Failed parsing ab as a UINT>\n"
    "[0.1] -> fail <Failed parsing 0.1 as a UINT>\n"
    "[0.3] -> fail <Failed parsing 0.3 as a UINT>\n"
    "[-5] -> fail <Failed parsing -5 as a UINT>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <0o7>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> fail <Failed parsing -0b1 as a UINT>\n"
    "[0x1_0] -> pass <0x1_0>\n";

// T_DBL
//   constant: CLI11's TypeValidator<double>.
static const char T_DBL[] =
    "desc <FLOAT>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> fail <Failed parsing abc as a FLOAT>\n"
    "[] -> fail <Failed parsing  as a FLOAT>\n"
    "[5.5] -> pass <5.5>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> pass <08>\n"
    "[1e0] -> pass <1e0>\n"
    "[2.5] -> pass <2.5>\n"
    "[2.6] -> pass <2.6>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> pass <inf>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> pass <1e-320>\n"
    "[-1e-300] -> pass <-1e-300>\n"
    "[c] -> fail <Failed parsing c as a FLOAT>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Failed parsing z as a FLOAT>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Failed parsing \"5\" as a FLOAT>\n"
    "[1_000] -> pass <1_000>\n"
    "[true] -> fail <Failed parsing true as a FLOAT>\n"
    "[false] -> fail <Failed parsing false as a FLOAT>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> pass <300>\n"
    "[-129] -> pass <-129>\n"
    "[2147483648] -> pass <2147483648>\n"
    "[18446744073709551616] -> pass <18446744073709551616>\n"
    "[a] -> fail <Failed parsing a as a FLOAT>\n"
    "[ab] -> fail <Failed parsing ab as a FLOAT>\n"
    "[0.1] -> pass <0.1>\n"
    "[0.3] -> pass <0.3>\n"
    "[-5] -> pass <-5>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> fail <Failed parsing 0b1 as a FLOAT>\n"
    "[0o7] -> fail <Failed parsing 0o7 as a FLOAT>\n"
    "[0B1] -> fail <Failed parsing 0B1 as a FLOAT>\n"
    "[-0b1] -> fail <Failed parsing -0b1 as a FLOAT>\n"
    "[0x1_0] -> pass <0x1_0>\n";

// T_FLT
//   constant: CLI11's TypeValidator<float>.
static const char T_FLT[] =
    "desc <FLOAT>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> fail <Failed parsing abc as a FLOAT>\n"
    "[] -> fail <Failed parsing  as a FLOAT>\n"
    "[5.5] -> pass <5.5>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> pass <08>\n"
    "[1e0] -> pass <1e0>\n"
    "[2.5] -> pass <2.5>\n"
    "[2.6] -> pass <2.6>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> pass <inf>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> pass <1e-320>\n"
    "[-1e-300] -> pass <-1e-300>\n"
    "[c] -> fail <Failed parsing c as a FLOAT>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Failed parsing z as a FLOAT>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Failed parsing \"5\" as a FLOAT>\n"
    "[1_000] -> pass <1_000>\n"
    "[true] -> fail <Failed parsing true as a FLOAT>\n"
    "[false] -> fail <Failed parsing false as a FLOAT>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> pass <300>\n"
    "[-129] -> pass <-129>\n"
    "[2147483648] -> pass <2147483648>\n"
    "[18446744073709551616] -> pass <18446744073709551616>\n"
    "[a] -> fail <Failed parsing a as a FLOAT>\n"
    "[ab] -> fail <Failed parsing ab as a FLOAT>\n"
    "[0.1] -> pass <0.1>\n"
    "[0.3] -> pass <0.3>\n"
    "[-5] -> pass <-5>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> fail <Failed parsing 0b1 as a FLOAT>\n"
    "[0o7] -> fail <Failed parsing 0o7 as a FLOAT>\n"
    "[0B1] -> fail <Failed parsing 0B1 as a FLOAT>\n"
    "[-0b1] -> fail <Failed parsing -0b1 as a FLOAT>\n"
    "[0x1_0] -> pass <0x1_0>\n";

// T_CHR
//   constant: CLI11's TypeValidator<char>.
static const char T_CHR[] =
    "desc <CHAR>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> fail <Failed parsing abc as a CHAR>\n"
    "[] -> fail <Failed parsing  as a CHAR>\n"
    "[5.5] -> fail <Failed parsing 5.5 as a CHAR>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> fail <Failed parsing 08 as a CHAR>\n"
    "[1e0] -> fail <Failed parsing 1e0 as a CHAR>\n"
    "[2.5] -> fail <Failed parsing 2.5 as a CHAR>\n"
    "[2.6] -> fail <Failed parsing 2.6 as a CHAR>\n"
    "[nan] -> fail <Failed parsing nan as a CHAR>\n"
    "[inf] -> fail <Failed parsing inf as a CHAR>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <Failed parsing 1e-320 as a CHAR>\n"
    "[-1e-300] -> fail <Failed parsing -1e-300 as a CHAR>\n"
    "[c] -> pass <c>\n"
    "[99] -> pass <99>\n"
    "[z] -> pass <z>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Failed parsing \"5\" as a CHAR>\n"
    "[1_000] -> fail <Failed parsing 1_000 as a CHAR>\n"
    "[true] -> pass <true>\n"
    "[false] -> fail <Failed parsing false as a CHAR>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> fail <Failed parsing 300 as a CHAR>\n"
    "[-129] -> fail <Failed parsing -129 as a CHAR>\n"
    "[2147483648] -> fail <Failed parsing 2147483648 as a CHAR>\n"
    "[18446744073709551616] -> fail <Failed parsing 18446744073709551616 as a "
    "CHAR>\n"
    "[a] -> pass <a>\n"
    "[ab] -> fail <Failed parsing ab as a CHAR>\n"
    "[0.1] -> fail <Failed parsing 0.1 as a CHAR>\n"
    "[0.3] -> fail <Failed parsing 0.3 as a CHAR>\n"
    "[-5] -> pass <-5>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <0o7>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> pass <-0b1>\n"
    "[0x1_0] -> pass <0x1_0>\n";

// T_BOOL
//   constant: CLI11's TypeValidator<bool>.
static const char T_BOOL[] =
    "desc <BOOLEAN>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> fail <Failed parsing abc as a BOOLEAN>\n"
    "[] -> pass <>\n"
    "[5.5] -> fail <Failed parsing 5.5 as a BOOLEAN>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> fail <Failed parsing 08 as a BOOLEAN>\n"
    "[1e0] -> fail <Failed parsing 1e0 as a BOOLEAN>\n"
    "[2.5] -> fail <Failed parsing 2.5 as a BOOLEAN>\n"
    "[2.6] -> fail <Failed parsing 2.6 as a BOOLEAN>\n"
    "[nan] -> fail <Failed parsing nan as a BOOLEAN>\n"
    "[inf] -> fail <Failed parsing inf as a BOOLEAN>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <Failed parsing 1e-320 as a BOOLEAN>\n"
    "[-1e-300] -> fail <Failed parsing -1e-300 as a BOOLEAN>\n"
    "[c] -> fail <Failed parsing c as a BOOLEAN>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Failed parsing z as a BOOLEAN>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Failed parsing \"5\" as a BOOLEAN>\n"
    "[1_000] -> fail <Failed parsing 1_000 as a BOOLEAN>\n"
    "[true] -> pass <true>\n"
    "[false] -> pass <false>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> fail <Failed parsing  7  as a BOOLEAN>\n"
    "[300] -> pass <300>\n"
    "[-129] -> pass <-129>\n"
    "[2147483648] -> pass <2147483648>\n"
    "[18446744073709551616] -> pass <18446744073709551616>\n"
    "[a] -> fail <Failed parsing a as a BOOLEAN>\n"
    "[ab] -> fail <Failed parsing ab as a BOOLEAN>\n"
    "[0.1] -> fail <Failed parsing 0.1 as a BOOLEAN>\n"
    "[0.3] -> fail <Failed parsing 0.3 as a BOOLEAN>\n"
    "[-5] -> pass <-5>\n"
    "[1'0] -> fail <Failed parsing 1'0 as a BOOLEAN>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> fail <Failed parsing 0o7 as a BOOLEAN>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> pass <-0b1>\n"
    "[0x1_0] -> fail <Failed parsing 0x1_0 as a BOOLEAN>\n";

// T_STR
//   constant: CLI11's TypeValidator<std::string>.
static const char T_STR[] =
    "desc <TEXT>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> pass <abc>\n"
    "[] -> pass <>\n"
    "[5.5] -> pass <5.5>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> pass <08>\n"
    "[1e0] -> pass <1e0>\n"
    "[2.5] -> pass <2.5>\n"
    "[2.6] -> pass <2.6>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> pass <inf>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> pass <1e-320>\n"
    "[-1e-300] -> pass <-1e-300>\n"
    "[c] -> pass <c>\n"
    "[99] -> pass <99>\n"
    "[z] -> pass <z>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> pass <\"5\">\n"
    "[1_000] -> pass <1_000>\n"
    "[true] -> pass <true>\n"
    "[false] -> pass <false>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> pass <300>\n"
    "[-129] -> pass <-129>\n"
    "[2147483648] -> pass <2147483648>\n"
    "[18446744073709551616] -> pass <18446744073709551616>\n"
    "[a] -> pass <a>\n"
    "[ab] -> pass <ab>\n"
    "[0.1] -> pass <0.1>\n"
    "[0.3] -> pass <0.3>\n"
    "[-5] -> pass <-5>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <0o7>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> pass <-0b1>\n"
    "[0x1_0] -> pass <0x1_0>\n";

// T_NUMBER
//   constant: CLI11's Number.
static const char T_NUMBER[] =
    "desc <NUMBER>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> fail <Failed parsing abc as a FLOAT>\n"
    "[] -> fail <Failed parsing  as a FLOAT>\n"
    "[5.5] -> pass <5.5>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> pass <08>\n"
    "[1e0] -> pass <1e0>\n"
    "[2.5] -> pass <2.5>\n"
    "[2.6] -> pass <2.6>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> pass <inf>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> pass <1e-320>\n"
    "[-1e-300] -> pass <-1e-300>\n"
    "[c] -> fail <Failed parsing c as a FLOAT>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Failed parsing z as a FLOAT>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Failed parsing \"5\" as a FLOAT>\n"
    "[1_000] -> pass <1_000>\n"
    "[true] -> fail <Failed parsing true as a FLOAT>\n"
    "[false] -> fail <Failed parsing false as a FLOAT>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> pass <300>\n"
    "[-129] -> pass <-129>\n"
    "[2147483648] -> pass <2147483648>\n"
    "[18446744073709551616] -> pass <18446744073709551616>\n"
    "[a] -> fail <Failed parsing a as a FLOAT>\n"
    "[ab] -> fail <Failed parsing ab as a FLOAT>\n"
    "[0.1] -> pass <0.1>\n"
    "[0.3] -> pass <0.3>\n"
    "[-5] -> pass <-5>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> fail <Failed parsing 0b1 as a FLOAT>\n"
    "[0o7] -> fail <Failed parsing 0o7 as a FLOAT>\n"
    "[0B1] -> fail <Failed parsing 0B1 as a FLOAT>\n"
    "[-0b1] -> fail <Failed parsing -0b1 as a FLOAT>\n"
    "[0x1_0] -> pass <0x1_0>\n";

// T_WORD
//   constant: the type check of WORD, a type of the caller's: its name
// describes the check and ends its message, and a conversion that runs out
// of memory stops the check with that error.
static const char T_WORD[] =
    "desc <WORD>\n"
    "[abc] -> pass <abc>\n"
    "[ABC] -> fail <Failed parsing ABC as a WORD>\n"
    "[] -> fail <Failed parsing  as a WORD>\n"
    "[a b] -> fail <Failed parsing a b as a WORD>\n"
    "[full] -> status <>\n";

// B_INT
//   constant: CLI11's Bound(0, 10) over `int`.
static const char B_INT[] =
    "desc <INT bounded to [0 - 10]>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <10>\n"
    "[-1] -> pass <0>\n"
    "[abc] -> fail <Value abc could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "[5.5] -> fail <Value 5.5 could not be converted>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> fail <Value 08 could not be converted>\n"
    "[1e0] -> fail <Value 1e0 could not be converted>\n"
    "[2.5] -> fail <Value 2.5 could not be converted>\n"
    "[2.6] -> fail <Value 2.6 could not be converted>\n"
    "[nan] -> fail <Value nan could not be converted>\n"
    "[inf] -> fail <Value inf could not be converted>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <Value 1e-320 could not be converted>\n"
    "[-1e-300] -> fail <Value -1e-300 could not be converted>\n"
    "[c] -> fail <Value c could not be converted>\n"
    "[99] -> pass <10>\n"
    "[z] -> fail <Value z could not be converted>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <10>\n"
    "[30] -> pass <10>\n"
    "[\"5\"] -> fail <Value \"5\" could not be converted>\n"
    "[1_000] -> pass <10>\n"
    "[true] -> pass <true>\n"
    "[false] -> fail <Value false could not be converted>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> pass <10>\n"
    "[-129] -> pass <0>\n"
    "[2147483648] -> fail <Value 2147483648 could not be converted>\n"
    "[18446744073709551616] -> fail <Value 18446744073709551616 could not be "
    "converted>\n"
    "[a] -> fail <Value a could not be converted>\n"
    "[ab] -> fail <Value ab could not be converted>\n"
    "[0.1] -> fail <Value 0.1 could not be converted>\n"
    "[0.3] -> fail <Value 0.3 could not be converted>\n"
    "[-5] -> pass <0>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <0o7>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> pass <0>\n"
    "[0x1_0] -> pass <10>\n";

// B_UINT
//   constant: CLI11's Bound(0u, 10u) over `unsigned int`.
static const char B_UINT[] =
    "desc <UINT bounded to [0 - 10]>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <10>\n"
    "[-1] -> fail <Value -1 could not be converted>\n"
    "[abc] -> fail <Value abc could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "[5.5] -> fail <Value 5.5 could not be converted>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[08] -> fail <Value 08 could not be converted>\n"
    "[1e0] -> fail <Value 1e0 could not be converted>\n"
    "[2.5] -> fail <Value 2.5 could not be converted>\n"
    "[2.6] -> fail <Value 2.6 could not be converted>\n"
    "[nan] -> fail <Value nan could not be converted>\n"
    "[inf] -> fail <Value inf could not be converted>\n"
    "[-0] -> fail <Value -0 could not be converted>\n"
    "[1e-320] -> fail <Value 1e-320 could not be converted>\n"
    "[-1e-300] -> fail <Value -1e-300 could not be converted>\n"
    "[c] -> fail <Value c could not be converted>\n"
    "[99] -> pass <10>\n"
    "[z] -> fail <Value z could not be converted>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <10>\n"
    "[30] -> pass <10>\n"
    "[\"5\"] -> fail <Value \"5\" could not be converted>\n"
    "[1_000] -> pass <10>\n"
    "[true] -> fail <Value true could not be converted>\n"
    "[false] -> fail <Value false could not be converted>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> pass <10>\n"
    "[-129] -> fail <Value -129 could not be converted>\n"
    "[2147483648] -> pass <10>\n"
    "[18446744073709551616] -> fail <Value 18446744073709551616 could not be "
    "converted>\n"
    "[a] -> fail <Value a could not be converted>\n"
    "[ab] -> fail <Value ab could not be converted>\n"
    "[0.1] -> fail <Value 0.1 could not be converted>\n"
    "[0.3] -> fail <Value 0.3 could not be converted>\n"
    "[-5] -> fail <Value -5 could not be converted>\n"
    "[1'0] -> pass <1'0>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <0o7>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> fail <Value -0b1 could not be converted>\n"
    "[0x1_0] -> pass <10>\n";

// B_LONG
//   constant: CLI11's Bound(-5L, 5L) over `long`, where 2147483648 is a
// `long` only when `long` is wider than 32 bits.
static const char B_LONG[] =
    "desc <INT bounded to [-5 - 5]>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <5>\n"
    "[11] -> pass <5>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> fail <Value abc could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "[5.5] -> fail <Value 5.5 could not be converted>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <5>\n"
    "[08] -> fail <Value 08 could not be converted>\n"
    "[1e0] -> fail <Value 1e0 could not be converted>\n"
    "[2.5] -> fail <Value 2.5 could not be converted>\n"
    "[2.6] -> fail <Value 2.6 could not be converted>\n"
    "[nan] -> fail <Value nan could not be converted>\n"
    "[inf] -> fail <Value inf could not be converted>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <Value 1e-320 could not be converted>\n"
    "[-1e-300] -> fail <Value -1e-300 could not be converted>\n"
    "[c] -> fail <Value c could not be converted>\n"
    "[99] -> pass <5>\n"
    "[z] -> fail <Value z could not be converted>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <5>\n"
    "[12] -> pass <5>\n"
    "[30] -> pass <5>\n"
    "[\"5\"] -> fail <Value \"5\" could not be converted>\n"
    "[1_000] -> pass <5>\n"
    "[true] -> pass <true>\n"
    "[false] -> fail <Value false could not be converted>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass <5>\n"
    "[300] -> pass <5>\n"
    "[-129] -> pass <-5>\n"
#if (LONG_MAX > 2147483647L)
    "[2147483648] -> pass <5>\n"
#else
    "[2147483648] -> fail <Value 2147483648 could not be converted>\n"
#endif
    "[18446744073709551616] -> fail <Value 18446744073709551616 could not be "
    "converted>\n"
    "[a] -> fail <Value a could not be converted>\n"
    "[ab] -> fail <Value ab could not be converted>\n"
    "[0.1] -> fail <Value 0.1 could not be converted>\n"
    "[0.3] -> fail <Value 0.3 could not be converted>\n"
    "[-5] -> pass <-5>\n"
    "[1'0] -> pass <5>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <5>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> pass <-0b1>\n"
    "[0x1_0] -> pass <5>\n";

// B_LL
//   constant: CLI11's Bound(-5LL, 5LL) over `long long`.
static const char B_LL[] =
    "desc <INT bounded to [-5 - 5]>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <5>\n"
    "[11] -> pass <5>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> fail <Value abc could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "[5.5] -> fail <Value 5.5 could not be converted>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <5>\n"
    "[08] -> fail <Value 08 could not be converted>\n"
    "[1e0] -> fail <Value 1e0 could not be converted>\n"
    "[2.5] -> fail <Value 2.5 could not be converted>\n"
    "[2.6] -> fail <Value 2.6 could not be converted>\n"
    "[nan] -> fail <Value nan could not be converted>\n"
    "[inf] -> fail <Value inf could not be converted>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <Value 1e-320 could not be converted>\n"
    "[-1e-300] -> fail <Value -1e-300 could not be converted>\n"
    "[c] -> fail <Value c could not be converted>\n"
    "[99] -> pass <5>\n"
    "[z] -> fail <Value z could not be converted>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <5>\n"
    "[12] -> pass <5>\n"
    "[30] -> pass <5>\n"
    "[\"5\"] -> fail <Value \"5\" could not be converted>\n"
    "[1_000] -> pass <5>\n"
    "[true] -> pass <true>\n"
    "[false] -> fail <Value false could not be converted>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass <5>\n"
    "[300] -> pass <5>\n"
    "[-129] -> pass <-5>\n"
    "[2147483648] -> pass <5>\n"
    "[18446744073709551616] -> fail <Value 18446744073709551616 could not be "
    "converted>\n"
    "[a] -> fail <Value a could not be converted>\n"
    "[ab] -> fail <Value ab could not be converted>\n"
    "[0.1] -> fail <Value 0.1 could not be converted>\n"
    "[0.3] -> fail <Value 0.3 could not be converted>\n"
    "[-5] -> pass <-5>\n"
    "[1'0] -> pass <5>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <5>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> pass <-0b1>\n"
    "[0x1_0] -> pass <5>\n";

// B_ONE
//   constant: CLI11's Bound(7), its one-bound form, which bounds an `int`
// from 0.
static const char B_ONE[] =
    "desc <INT bounded to [0 - 7]>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <7>\n"
    "[11] -> pass <7>\n"
    "[-1] -> pass <0>\n"
    "[abc] -> fail <Value abc could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "[5.5] -> fail <Value 5.5 could not be converted>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <7>\n"
    "[08] -> fail <Value 08 could not be converted>\n"
    "[1e0] -> fail <Value 1e0 could not be converted>\n"
    "[2.5] -> fail <Value 2.5 could not be converted>\n"
    "[2.6] -> fail <Value 2.6 could not be converted>\n"
    "[nan] -> fail <Value nan could not be converted>\n"
    "[inf] -> fail <Value inf could not be converted>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <Value 1e-320 could not be converted>\n"
    "[-1e-300] -> fail <Value -1e-300 could not be converted>\n"
    "[c] -> fail <Value c could not be converted>\n"
    "[99] -> pass <7>\n"
    "[z] -> fail <Value z could not be converted>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <7>\n"
    "[30] -> pass <7>\n"
    "[\"5\"] -> fail <Value \"5\" could not be converted>\n"
    "[1_000] -> pass <7>\n"
    "[true] -> pass <true>\n"
    "[false] -> fail <Value false could not be converted>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> pass <7>\n"
    "[-129] -> pass <0>\n"
    "[2147483648] -> fail <Value 2147483648 could not be converted>\n"
    "[18446744073709551616] -> fail <Value 18446744073709551616 could not be "
    "converted>\n"
    "[a] -> fail <Value a could not be converted>\n"
    "[ab] -> fail <Value ab could not be converted>\n"
    "[0.1] -> fail <Value 0.1 could not be converted>\n"
    "[0.3] -> fail <Value 0.3 could not be converted>\n"
    "[-5] -> pass <0>\n"
    "[1'0] -> pass <7>\n"
    "[0b1] -> pass <0b1>\n"
    "[0o7] -> pass <0o7>\n"
    "[0B1] -> pass <0B1>\n"
    "[-0b1] -> pass <0>\n"
    "[0x1_0] -> pass <7>\n";

// B_DBL
//   constant: CLI11's Bound(0.5, 2.5) over `double`.
static const char B_DBL[] =
    "desc <FLOAT bounded to [0.5 - 2.5]>\n"
    "[5] -> pass <2.5>\n"
    "[0] -> pass <0.5>\n"
    "[10] -> pass <2.5>\n"
    "[11] -> pass <2.5>\n"
    "[-1] -> pass <0.5>\n"
    "[abc] -> fail <Value abc could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "[5.5] -> pass <2.5>\n"
    "[ 5] -> pass <2.5>\n"
    "[0x5] -> pass <2.5>\n"
    "[010] -> pass <2.5>\n"
    "[08] -> pass <2.5>\n"
    "[1e0] -> pass <1e0>\n"
    "[2.5] -> pass <2.5>\n"
    "[2.6] -> pass <2.5>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> pass <2.5>\n"
    "[-0] -> pass <0.5>\n"
    "[1e-320] -> pass <0.5>\n"
    "[-1e-300] -> pass <0.5>\n"
    "[c] -> fail <Value c could not be converted>\n"
    "[99] -> pass <2.5>\n"
    "[z] -> fail <Value z could not be converted>\n"
    "[3] -> pass <2.5>\n"
    "[7] -> pass <2.5>\n"
    "[12] -> pass <2.5>\n"
    "[30] -> pass <2.5>\n"
    "[\"5\"] -> fail <Value \"5\" could not be converted>\n"
    "[1_000] -> pass <2.5>\n"
    "[true] -> fail <Value true could not be converted>\n"
    "[false] -> fail <Value false could not be converted>\n"
    "[+5] -> pass <2.5>\n"
    "[ 7 ] -> pass <2.5>\n"
    "[300] -> pass <2.5>\n"
    "[-129] -> pass <0.5>\n"
    "[2147483648] -> pass <2.5>\n"
    "[18446744073709551616] -> pass <2.5>\n"
    "[a] -> fail <Value a could not be converted>\n"
    "[ab] -> fail <Value ab could not be converted>\n"
    "[0.1] -> pass <0.5>\n"
    "[0.3] -> pass <0.5>\n"
    "[-5] -> pass <0.5>\n"
    "[1'0] -> pass <2.5>\n"
    "[0b1] -> fail <Value 0b1 could not be converted>\n"
    "[0o7] -> fail <Value 0o7 could not be converted>\n"
    "[0B1] -> fail <Value 0B1 could not be converted>\n"
    "[-0b1] -> fail <Value -0b1 could not be converted>\n"
    "[0x1_0] -> pass <2.5>\n";

// B_CHR
//   constant: CLI11's Bound('a', 'f') over `char`.
static const char B_CHR[] =
    "desc <CHAR bounded to [a - f]>\n"
    "[5] -> pass <a>\n"
    "[0] -> pass <a>\n"
    "[10] -> pass <a>\n"
    "[11] -> pass <a>\n"
    "[-1] -> pass <a>\n"
    "[abc] -> fail <Value abc could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "[5.5] -> fail <Value 5.5 could not be converted>\n"
    "[ 5] -> pass <a>\n"
    "[0x5] -> pass <a>\n"
    "[010] -> pass <a>\n"
    "[08] -> fail <Value 08 could not be converted>\n"
    "[1e0] -> fail <Value 1e0 could not be converted>\n"
    "[2.5] -> fail <Value 2.5 could not be converted>\n"
    "[2.6] -> fail <Value 2.6 could not be converted>\n"
    "[nan] -> fail <Value nan could not be converted>\n"
    "[inf] -> fail <Value inf could not be converted>\n"
    "[-0] -> pass <a>\n"
    "[1e-320] -> fail <Value 1e-320 could not be converted>\n"
    "[-1e-300] -> fail <Value -1e-300 could not be converted>\n"
    "[c] -> pass <c>\n"
    "[99] -> pass <99>\n"
    "[z] -> pass <f>\n"
    "[3] -> pass <a>\n"
    "[7] -> pass <a>\n"
    "[12] -> pass <a>\n"
    "[30] -> pass <a>\n"
    "[\"5\"] -> fail <Value \"5\" could not be converted>\n"
    "[1_000] -> fail <Value 1_000 could not be converted>\n"
    "[true] -> pass <a>\n"
    "[false] -> fail <Value false could not be converted>\n"
    "[+5] -> pass <a>\n"
    "[ 7 ] -> pass <a>\n"
    "[300] -> fail <Value 300 could not be converted>\n"
    "[-129] -> fail <Value -129 could not be converted>\n"
    "[2147483648] -> fail <Value 2147483648 could not be converted>\n"
    "[18446744073709551616] -> fail <Value 18446744073709551616 could not be "
    "converted>\n"
    "[a] -> pass <a>\n"
    "[ab] -> fail <Value ab could not be converted>\n"
    "[0.1] -> fail <Value 0.1 could not be converted>\n"
    "[0.3] -> fail <Value 0.3 could not be converted>\n"
    "[-5] -> pass <a>\n"
    "[1'0] -> pass <a>\n"
    "[0b1] -> pass <a>\n"
    "[0o7] -> pass <a>\n"
    "[0B1] -> pass <a>\n"
    "[-0b1] -> pass <a>\n"
    "[0x1_0] -> pass <a>\n";

// B_NEG
//   constant: CLI11's Bound(-1.5, -0.5) over `double`.
static const char B_NEG[] =
    "desc <FLOAT bounded to [-1.5 - -0.5]>\n"
    "[5] -> pass <-0.5>\n"
    "[0] -> pass <-0.5>\n"
    "[10] -> pass <-0.5>\n"
    "[11] -> pass <-0.5>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> fail <Value abc could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "[5.5] -> pass <-0.5>\n"
    "[ 5] -> pass <-0.5>\n"
    "[0x5] -> pass <-0.5>\n"
    "[010] -> pass <-0.5>\n"
    "[08] -> pass <-0.5>\n"
    "[1e0] -> pass <-0.5>\n"
    "[2.5] -> pass <-0.5>\n"
    "[2.6] -> pass <-0.5>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> pass <-0.5>\n"
    "[-0] -> pass <-0.5>\n"
    "[1e-320] -> pass <-0.5>\n"
    "[-1e-300] -> pass <-0.5>\n"
    "[c] -> fail <Value c could not be converted>\n"
    "[99] -> pass <-0.5>\n"
    "[z] -> fail <Value z could not be converted>\n"
    "[3] -> pass <-0.5>\n"
    "[7] -> pass <-0.5>\n"
    "[12] -> pass <-0.5>\n"
    "[30] -> pass <-0.5>\n"
    "[\"5\"] -> fail <Value \"5\" could not be converted>\n"
    "[1_000] -> pass <-0.5>\n"
    "[true] -> fail <Value true could not be converted>\n"
    "[false] -> fail <Value false could not be converted>\n"
    "[+5] -> pass <-0.5>\n"
    "[ 7 ] -> pass <-0.5>\n"
    "[300] -> pass <-0.5>\n"
    "[-129] -> pass <-1.5>\n"
    "[2147483648] -> pass <-0.5>\n"
    "[18446744073709551616] -> pass <-0.5>\n"
    "[a] -> fail <Value a could not be converted>\n"
    "[ab] -> fail <Value ab could not be converted>\n"
    "[0.1] -> pass <-0.5>\n"
    "[0.3] -> pass <-0.5>\n"
    "[-5] -> pass <-1.5>\n"
    "[1'0] -> pass <-0.5>\n"
    "[0b1] -> fail <Value 0b1 could not be converted>\n"
    "[0o7] -> fail <Value 0o7 could not be converted>\n"
    "[0B1] -> fail <Value 0B1 could not be converted>\n"
    "[-0b1] -> fail <Value -0b1 could not be converted>\n"
    "[0x1_0] -> pass <-0.5>\n";

// IPV4
//   constant: CLI11's ValidIPV4.
static const char IPV4[] =
    "desc <IPV4>\n"
    "[1.2.3.4] -> pass <1.2.3.4>\n"
    "[255.255.255.255] -> pass <255.255.255.255>\n"
    "[256.1.1.1] -> fail <Each IP number must be between 0 and 255 256>\n"
    "[1.2.3] -> fail <Invalid IPV4 address: must have 3 separators>\n"
    "[1.2.3.4.5] -> fail <Invalid IPV4 address: must have 3 separators>\n"
    "[a.b.c.d] -> fail <Failed parsing number (a)>\n"
    "[1..2.3] -> fail <Failed parsing number ()>\n"
    "[.1.2.3] -> fail <Failed parsing number ()>\n"
    "[-1.2.3.4] -> fail <Each IP number must be between 0 and 255 -1>\n"
    "[0x7f.0.0.1] -> pass <0x7f.0.0.1>\n"
    "[010.1.1.1] -> pass <010.1.1.1>\n"
    "[08.1.1.1] -> fail <Failed parsing number (08)>\n"
    "[ 1.2.3.4] -> pass < 1.2.3.4>\n"
    "[1.2.3.4 ] -> pass <1.2.3.4 >\n"
    "[1 .2.3.4] -> pass <1 .2.3.4>\n"
    "[2147483648.1.1.1] -> fail <Failed parsing number (2147483648)>\n"
    "[1_0.0.0.1] -> pass <1_0.0.0.1>\n"
    "[] -> fail <Invalid IPV4 address: must have 3 separators>\n"
    "[...] -> fail <Failed parsing number ()>\n"
    "[0.0.0.0] -> pass <0.0.0.0>\n"
    "[-0.0.0.0] -> pass <-0.0.0.0>\n"
    "[+1.2.3.4] -> pass <+1.2.3.4>\n"
    "[0b1.0.0.0] -> pass <0b1.0.0.0>\n"
    "[0o7.0.0.0] -> pass <0o7.0.0.0>\n"
    "[1.2.3.-1] -> fail <Each IP number must be between 0 and 255 -1>\n"
    "[999.0.0.0] -> fail <Each IP number must be between 0 and 255 999>\n";

// K_NOT
//   constant: CLI11's !ValidIPV4.
static const char K_NOT[] =
    "desc <NOT IPV4>\n"
    "[1.2.3.4] -> fail <check IPV4 succeeded improperly>\n"
    "[255.255.255.255] -> fail <check IPV4 succeeded improperly>\n"
    "[256.1.1.1] -> pass <256.1.1.1>\n"
    "[1.2.3] -> pass <1.2.3>\n"
    "[1.2.3.4.5] -> pass <1.2.3.4.5>\n"
    "[a.b.c.d] -> pass <a.b.c.d>\n"
    "[1..2.3] -> pass <1..2.3>\n"
    "[.1.2.3] -> pass <.1.2.3>\n"
    "[-1.2.3.4] -> pass <-1.2.3.4>\n"
    "[0x7f.0.0.1] -> fail <check IPV4 succeeded improperly>\n"
    "[010.1.1.1] -> fail <check IPV4 succeeded improperly>\n"
    "[08.1.1.1] -> pass <08.1.1.1>\n"
    "[ 1.2.3.4] -> fail <check IPV4 succeeded improperly>\n"
    "[1.2.3.4 ] -> fail <check IPV4 succeeded improperly>\n"
    "[1 .2.3.4] -> fail <check IPV4 succeeded improperly>\n"
    "[2147483648.1.1.1] -> pass <2147483648.1.1.1>\n"
    "[1_0.0.0.1] -> fail <check IPV4 succeeded improperly>\n"
    "[] -> pass <>\n"
    "[...] -> pass <...>\n"
    "[0.0.0.0] -> fail <check IPV4 succeeded improperly>\n"
    "[-0.0.0.0] -> fail <check IPV4 succeeded improperly>\n"
    "[+1.2.3.4] -> fail <check IPV4 succeeded improperly>\n"
    "[0b1.0.0.0] -> fail <check IPV4 succeeded improperly>\n"
    "[0o7.0.0.0] -> fail <check IPV4 succeeded improperly>\n"
    "[1.2.3.-1] -> pass <1.2.3.-1>\n"
    "[999.0.0.0] -> pass <999.0.0.0>\n";

// K_BOUND
//   constant: CLI11's Bound(0, 10) & Range(5, 7).
static const char K_BOUND[] =
    "desc <(INT bounded to [0 - 10]) AND (INT in [5 - 7])>\n"
    "[5] -> pass <5>\n"
    "[0] -> fail <Value 0 not in range [5 - 7]>\n"
    "[10] -> fail <Value 10 not in range [5 - 7]>\n"
    "[11] -> fail <Value 10 not in range [5 - 7]>\n"
    "[-1] -> fail <Value 0 not in range [5 - 7]>\n"
    "[abc] -> fail <(Value abc could not be converted) AND (Value abc not in "
    "range [5 - 7])>\n"
    "[] -> fail <(Value  could not be converted) AND (Value  not in range [5 - "
    "7])>\n"
    "[5.5] -> fail <(Value 5.5 could not be converted) AND (Value 5.5 not in "
    "range [5 - 7])>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> fail <Value 010 not in range [5 - 7]>\n"
    "[08] -> fail <(Value 08 could not be converted) AND (Value 08 not in "
    "range [5 - 7])>\n"
    "[1e0] -> fail <(Value 1e0 could not be converted) AND (Value 1e0 not in "
    "range [5 - 7])>\n"
    "[2.5] -> fail <(Value 2.5 could not be converted) AND (Value 2.5 not in "
    "range [5 - 7])>\n"
    "[2.6] -> fail <(Value 2.6 could not be converted) AND (Value 2.6 not in "
    "range [5 - 7])>\n"
    "[nan] -> fail <(Value nan could not be converted) AND (Value nan not in "
    "range [5 - 7])>\n"
    "[inf] -> fail <(Value inf could not be converted) AND (Value inf not in "
    "range [5 - 7])>\n"
    "[-0] -> fail <Value -0 not in range [5 - 7]>\n"
    "[1e-320] -> fail <(Value 1e-320 could not be converted) AND (Value 1e-320 "
    "not in range [5 - 7])>\n"
    "[-1e-300] -> fail <(Value -1e-300 could not be converted) AND (Value "
    "-1e-300 not in range [5 - 7])>\n"
    "[c] -> fail <(Value c could not be converted) AND (Value c not in range "
    "[5 - 7])>\n"
    "[99] -> fail <Value 10 not in range [5 - 7]>\n"
    "[z] -> fail <(Value z could not be converted) AND (Value z not in range "
    "[5 - 7])>\n"
    "[3] -> fail <Value 3 not in range [5 - 7]>\n"
    "[7] -> pass <7>\n"
    "[12] -> fail <Value 10 not in range [5 - 7]>\n"
    "[30] -> fail <Value 10 not in range [5 - 7]>\n"
    "[\"5\"] -> fail <(Value \"5\" could not be converted) AND (Value \"5\" "
    "not in range [5 - 7])>\n"
    "[1_000] -> fail <Value 10 not in range [5 - 7]>\n"
    "[true] -> fail <Value true not in range [5 - 7]>\n"
    "[false] -> fail <(Value false could not be converted) AND (Value false "
    "not in range [5 - 7])>\n"
    "[+5] -> pass <+5>\n"
    "[ 7 ] -> pass < 7 >\n"
    "[300] -> fail <Value 10 not in range [5 - 7]>\n"
    "[-129] -> fail <Value 0 not in range [5 - 7]>\n"
    "[2147483648] -> fail <(Value 2147483648 could not be converted) AND "
    "(Value 2147483648 not in range [5 - 7])>\n"
    "[18446744073709551616] -> fail <(Value 18446744073709551616 could not be "
    "converted) AND (Value 18446744073709551616 not in range [5 - 7])>\n"
    "[a] -> fail <(Value a could not be converted) AND (Value a not in range "
    "[5 - 7])>\n"
    "[ab] -> fail <(Value ab could not be converted) AND (Value ab not in "
    "range [5 - 7])>\n"
    "[0.1] -> fail <(Value 0.1 could not be converted) AND (Value 0.1 not in "
    "range [5 - 7])>\n"
    "[0.3] -> fail <(Value 0.3 could not be converted) AND (Value 0.3 not in "
    "range [5 - 7])>\n"
    "[-5] -> fail <Value 0 not in range [5 - 7]>\n"
    "[1'0] -> fail <Value 1'0 not in range [5 - 7]>\n"
    "[0b1] -> fail <Value 0b1 not in range [5 - 7]>\n"
    "[0o7] -> pass <0o7>\n"
    "[0B1] -> fail <Value 0B1 not in range [5 - 7]>\n"
    "[-0b1] -> fail <Value 0 not in range [5 - 7]>\n"
    "[0x1_0] -> fail <Value 10 not in range [5 - 7]>\n";

// DEPARTED_IPV4
//   constant: ValidIPV4 over TRUE_ADDRESSES. CLI11 reads each part with its
// conversion to `int`, which takes `true` as 1 for the sake of flags, so it
// passes `true.0.0.1`, `true.1.1.1` and `0.0.0.true`; here a part is an
// integer or nothing, and the word is refused as a number that did not
// parse. The two lines with a dot too many are CLI11's.
static const char DEPARTED_IPV4[] =
    "desc <IPV4>\n"
    "[1.2.3.] -> fail <Failed parsing number ()>\n"
    "[true.0.0.1] -> fail <Failed parsing number (true)>\n"
    "[true.1.1.1] -> fail <Failed parsing number (true)>\n"
    "[1.2.3.4.] -> fail <Invalid IPV4 address: must have 3 separators>\n"
    "[0.0.0.true] -> fail <Failed parsing number (true)>\n";

// DEPARTED_BDBL
//   constant: CLI11's Bound(0.1, 0.3) over `double`, run over CLAMPED. A
// clamped value is written as the type writes values, the shortest text
// that reads back as the same number -- `0.1`, `0.3` -- where CLI11 writes
// every digit a `double` holds: `0.10000000000000001`,
// `0.29999999999999999`. Each text reads back as the same value.
static const char DEPARTED_BDBL[] =
    "desc <FLOAT bounded to [0.1 - 0.3]>\n"
    "[0.05] -> pass <0.1>\n"
    "[0.5] -> pass <0.3>\n"
    "[0.2] -> pass <0.2>\n"
    "[-1] -> pass <0.1>\n"
    "[120] -> pass <0.3>\n"
    "[50] -> pass <0.3>\n";

// DEPARTED_BFLT
//   constant: CLI11's Bound(0.1f, 0.3f) over `float`, run over CLAMPED: the
// same, where CLI11 writes `0.100000001` and `0.300000012`.
static const char DEPARTED_BFLT[] =
    "desc <FLOAT bounded to [0.1 - 0.3]>\n"
    "[0.05] -> pass <0.1>\n"
    "[0.5] -> pass <0.3>\n"
    "[0.2] -> pass <0.2>\n"
    "[-1] -> pass <0.1>\n"
    "[120] -> pass <0.3>\n"
    "[50] -> pass <0.3>\n";

// DEPARTED_BU8
//   constant: CLI11's Bound over `uint8_t` from 65 to 100, run over CLAMPED.
// The bounds and a clamped value are written in decimal, where CLI11 streams
// an 8-bit integer as a character: its description is `UINT bounded to [A -
// d]`, and it clamps 120 to `d`, which an option of the type then fails to
// convert.
static const char DEPARTED_BU8[] =
    "desc <UINT bounded to [65 - 100]>\n"
    "[0.05] -> fail <Value 0.05 could not be converted>\n"
    "[0.5] -> fail <Value 0.5 could not be converted>\n"
    "[0.2] -> fail <Value 0.2 could not be converted>\n"
    "[-1] -> fail <Value -1 could not be converted>\n"
    "[120] -> pass <100>\n"
    "[50] -> pass <65>\n";

// P_ADDRESS_SHORT
//   constant: CLI11's `--ip 1.2.3`, the option bound to text and checked by
// ValidIPV4.
static const char P_ADDRESS_SHORT[] =
    "ERR ValidationError 105: --ip: Invalid IPV4 address: must have 3 "
    "separators\n"
    "cmd app n=1 miss=[] subs=[] order=[--ip]\n"
    "opt --ip n=1 res=[1.2.3]\n"
    "value []\n";

// P_ADDRESS
//   constant: CLI11's `--ip 1.2.3.4`, to the same option.
static const char P_ADDRESS[] =
    "cmd app n=1 miss=[] subs=[] order=[--ip]\n"
    "opt --ip n=1 res=[1.2.3.4]\n"
    "value [1.2.3.4]\n";

// P_RATIO_WORD
//   constant: CLI11's `--ratio abc`, the option bound to a `double` and
// checked by Number.
static const char P_RATIO_WORD[] =
    "ERR ValidationError 105: --ratio: Failed parsing abc as a FLOAT\n"
    "cmd app n=1 miss=[] subs=[] order=[--ratio]\n"
    "opt --ratio n=1 res=[abc]\n"
    "value [0]\n";

// P_RATIO
//   constant: CLI11's `--ratio 2.5`, to the same option.
static const char P_RATIO[] =
    "cmd app n=1 miss=[] subs=[] order=[--ratio]\n"
    "opt --ratio n=1 res=[2.5]\n"
    "value [2.5]\n";

// P_COUNT_HIGH
//   constant: CLI11's `--count 20`, the option bound to an `int` and
// transformed by Bound(0, 10).
static const char P_COUNT_HIGH[] =
    "cmd app n=1 miss=[] subs=[] order=[--count]\n"
    "opt --count n=1 res=[10]\n"
    "value [10]\n";

// P_COUNT_LOW
//   constant: CLI11's `--count -3`, to the same option.
static const char P_COUNT_LOW[] =
    "cmd app n=1 miss=[] subs=[] order=[--count]\n"
    "opt --count n=1 res=[0]\n"
    "value [0]\n";

// P_COUNT_WORD
//   constant: CLI11's `--count x`, to the same option.
static const char P_COUNT_WORD[] =
    "ERR ValidationError 105: --count: Value x could not be converted\n"
    "cmd app n=1 miss=[] subs=[] order=[--count]\n"
    "opt --count n=1 res=[x]\n"
    "value [0]\n";

// ADDRESS_OPTION
//   constant: CLI11's `add_option("--ip", text)->check(ValidIPV4)`.
static const struct d_tests_arg_checked ADDRESS_OPTION =
{
    "--ip",
    D_ARG_BUILTIN_STRING,
    false
};

// RATIO_OPTION
//   constant: CLI11's `add_option("--ratio", real)->check(Number)`.
static const struct d_tests_arg_checked RATIO_OPTION =
{
    "--ratio",
    D_ARG_BUILTIN_DOUBLE,
    false
};

// COUNT_OPTION
//   constant: CLI11's `add_option("--count", whole)` with a Bound added by
// transform().
static const struct d_tests_arg_checked COUNT_OPTION =
{
    "--count",
    D_ARG_BUILTIN_INT,
    true
};

// WORD_MAX
//   constant: how many letters a WORD holds.
enum
{
    WORD_MAX = 80
};

// d_tests_arg_extra_word
//   struct: an object of WORD, a type of the caller's: a run of lowercase
// letters, in more room than any built-in type takes, so that a check of the
// type converts into storage of its own.
struct d_tests_arg_extra_word
{
    char letters[WORD_MAX + 1];
};

/*
d_tests_arg_extra_word_convert
  WORD's conversion: one value of lowercase letters, at least one and at most
WORD_MAX of them; the word `full` runs out of memory.
*/
static int
d_tests_arg_extra_word_convert(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    struct d_tests_arg_extra_word word = { { 0 } };

    (void)_type;
    (void)_context;

    // one value, of a length a word has
    if ( (_count != 1u)                         ||
         (_values[0].length == 0u)              ||
         (_values[0].length > (size_t)WORD_MAX) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    // the word that cannot be had
    if ( (_values[0].length == 4u) &&
         (memcmp(_values[0].data,
                 "full",
                 4u) == 0) )
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    // each character a lowercase letter
    for (size_t index = 0u; index < _values[0].length; ++index)
    {
        const char letter = _values[0].data[index];

        // anything else is no word
        if ( (letter < 'a') ||
             (letter > 'z') )
        {
            return D_ARG_ERROR_CONVERSION;
        }

        word.letters[index] = letter;
    }

    memcpy(_target,
           &word,
           sizeof(word));

    return D_ARG_OK;
}

// WORD_TYPE
//   constant: WORD's descriptor, which converts and does not format.
static const struct d_arg_type WORD_TYPE =
{
    "WORD",
    d_tests_arg_extra_word_convert,
    NULL,
    NULL,
    sizeof(struct d_tests_arg_extra_word),
    1,
    1,
    1,
    0u
};

/*
d_tests_arg_extra_numbers
  Runs a check over NUMBERS and compares its transcript with the one
expected.
*/
static bool
d_tests_arg_extra_numbers(
    const char*            _what,
    struct d_arg_validator _validator,
    const char*            _expected
)
{
    return d_tests_arg_holds(_what,
                             _validator,
                             NUMBERS,
                             NUMBER_COUNT,
                             _expected);
}

/*
d_tests_arg_extra_typed
  CLI11's TypeValidator of a built-in type, run over NUMBERS.
*/
static bool
d_tests_arg_extra_typed(
    const char* _what,
    int         _builtin,
    const char* _expected
)
{
    return d_tests_arg_extra_numbers(
               _what,
               d_arg_validator_type(d_arg_type_builtin(_builtin)),
               _expected);
}

/*
d_tests_arg_extra_int_bound
  CLI11's Bound over `int`.
*/
static struct d_arg_validator
d_tests_arg_extra_int_bound(
    int _low,
    int _high
)
{
    return d_arg_validator_bound(d_arg_type_builtin(D_ARG_BUILTIN_INT),
                                 &_low,
                                 &_high);
}

/*
d_tests_arg_extra_int_range
  CLI11's Range over `int`.
*/
static struct d_arg_validator
d_tests_arg_extra_int_range(
    int _low,
    int _high
)
{
    return d_arg_validator_range(d_arg_type_builtin(D_ARG_BUILTIN_INT),
                                 &_low,
                                 &_high);
}

/*
d_tests_arg_extra_unmade
  Whether a check could not be made for a bad argument: it holds
D_ARG_ERROR_BAD_ARGUMENT, and runs as that error. The check is released.
*/
static bool
d_tests_arg_extra_unmade(
    struct d_arg_validator _validator
)
{
    struct d_arg_validation validation =
    {
        { NULL, 0u },
        D_ARG_BUFFER_INIT,
        D_ARG_BUFFER_INIT,
        NULL,
        0
    };

    d_arg_validation_init(&validation,
                          d_pack_text_from_cstr("1"));

    const int  status = d_arg_validator_run(&_validator,
                                            &validation);
    const bool unmade =
        ( (_validator.status == D_ARG_ERROR_BAD_ARGUMENT) &&
          (status == D_ARG_ERROR_BAD_ARGUMENT) );

    d_arg_validation_release(&validation);
    d_arg_validator_release(&_validator);

    return unmade;
}

//==============================================================================
// 1.  TYPES
//==============================================================================

/*
d_tests_arg_extra_type_integers
  CLI11's TypeValidator over `int`, `unsigned int`, `int8_t` and `uint64_t`.
  Tests the following:
  - each described as its type's name, and refusing with "Failed parsing
    <value> as a <TYPE>"
  - a value judged as an option of the type converts it -- leading zeros
    octal, the `0x`, `0o` and `0b` prefixes, the separators, `true` as 1,
    each type's limits -- but for the empty text, which no number is
  - a passing value stands as it was given
*/
bool
d_tests_arg_extra_type_integers(void)
{
    const bool whole   = d_tests_arg_extra_typed("T_INT",
                                                 D_ARG_BUILTIN_INT,
                                                 T_INT);
    const bool natural = d_tests_arg_extra_typed("T_UINT",
                                                 D_ARG_BUILTIN_UNSIGNED_INT,
                                                 T_UINT);
    const bool small   = d_tests_arg_extra_typed("T_I8",
                                                 D_ARG_BUILTIN_INT8,
                                                 T_I8);
    const bool wide    = d_tests_arg_extra_typed("T_U64",
                                                 D_ARG_BUILTIN_UINT64,
                                                 T_U64);

    return ( (whole)   &&
             (natural) &&
             (small)   &&
             (wide) );
}

/*
d_tests_arg_extra_type_others
  CLI11's TypeValidator over `double`, `float`, `char`, `bool` and text, and
Number.
  Tests the following:
  - floating point read in full, NaN and infinity among it, and a `float`
    refusing what overflows it
  - a `char` one character, or a small integer
  - a `bool` the truth words and the numbers flags take
  - text taking every value, the empty one too
  - Number described as NUMBER and refusing as a FLOAT
*/
bool
d_tests_arg_extra_type_others(void)
{
    const bool real   = d_tests_arg_extra_typed("T_DBL",
                                                D_ARG_BUILTIN_DOUBLE,
                                                T_DBL);
    const bool single = d_tests_arg_extra_typed("T_FLT",
                                                D_ARG_BUILTIN_FLOAT,
                                                T_FLT);
    const bool letter = d_tests_arg_extra_typed("T_CHR",
                                                D_ARG_BUILTIN_CHAR,
                                                T_CHR);
    const bool truth  = d_tests_arg_extra_typed("T_BOOL",
                                                D_ARG_BUILTIN_BOOL,
                                                T_BOOL);
    const bool text   = d_tests_arg_extra_typed("T_STR",
                                                D_ARG_BUILTIN_STRING,
                                                T_STR);
    const bool number = d_tests_arg_extra_numbers("T_NUMBER",
                                                  d_arg_validator_number(),
                                                  T_NUMBER);

    return ( (real)   &&
             (single) &&
             (letter) &&
             (truth)  &&
             (text)   &&
             (number) );
}

/*
d_tests_arg_extra_type_custom
  The type check of WORD, a type of the caller's whose objects are larger
than any built-in type's.
  Tests the following:
  - the check is described by, and refuses with, the type's name
  - each value is converted in full into storage of its own, and dropped
  - a conversion that stops with an error stops the check with it
*/
bool
d_tests_arg_extra_type_custom(void)
{
    static const char* const WORDS[] =
    {
        "abc", "ABC", "", "a b", "full"
    };

    return d_tests_arg_holds("T_WORD",
                             d_arg_validator_type(&WORD_TYPE),
                             WORDS,
                             sizeof(WORDS) / sizeof(WORDS[0]),
                             T_WORD);
}

//==============================================================================
// 2.  BOUNDS
//==============================================================================

/*
d_tests_arg_extra_bound_integers
  CLI11's Bound over `int`, `unsigned int`, `long` and `long long`, and its
one-bound form.
  Tests the following:
  - described as "<TYPE> bounded to [<low> - <high>]"
  - a value outside the bounds becomes the nearer bound; one inside stands
    as it was given
  - one that does not convert is refused with "Value <value> could not be
    converted", a `-0` for an unsigned type included
*/
bool
d_tests_arg_extra_bound_integers(void)
{
    const unsigned int u_low  = 0u;
    const unsigned int u_high = 10u;
    const long long    w_low  = -5;
    const long long    w_high = 5;
    const struct d_arg_type* const natural_type =
        d_arg_type_builtin(D_ARG_BUILTIN_UNSIGNED_INT);
    const struct d_arg_type* const wide_type    =
        d_arg_type_builtin(D_ARG_BUILTIN_LONG_LONG);
    const bool whole   = d_tests_arg_extra_numbers(
                             "B_INT",
                             d_tests_arg_extra_int_bound(0,
                                                         10),
                             B_INT);
    const bool natural = d_tests_arg_extra_numbers(
                             "B_UINT",
                             d_arg_validator_bound(natural_type,
                                                   &u_low,
                                                   &u_high),
                             B_UINT);
    const bool longer  = d_tests_arg_extra_numbers(
                             "B_LONG",
                             d_arg_validator_bound_long(-5L,
                                                        5L),
                             B_LONG);
    const bool wide    = d_tests_arg_extra_numbers(
                             "B_LL",
                             d_arg_validator_bound(wide_type,
                                                   &w_low,
                                                   &w_high),
                             B_LL);
    const bool one     = d_tests_arg_extra_numbers(
                             "B_ONE",
                             d_tests_arg_extra_int_bound(0,
                                                         7),
                             B_ONE);

    return ( (whole)   &&
             (natural) &&
             (longer)  &&
             (wide)    &&
             (one) );
}

/*
d_tests_arg_extra_bound_others
  CLI11's Bound over `double` and `char`, and over negative bounds.
  Tests the following:
  - floating-point bounds written to six significant digits, and a NaN,
    which no comparison catches, standing
  - character bounds written as characters
*/
bool
d_tests_arg_extra_bound_others(void)
{
    const char  c_low  = 'a';
    const char  c_high = 'f';
    const bool  real   = d_tests_arg_extra_numbers(
                             "B_DBL",
                             d_arg_validator_bound_double(0.5,
                                                          2.5),
                             B_DBL);
    const bool  letter = d_tests_arg_extra_numbers(
                             "B_CHR",
                             d_arg_validator_bound(
                                 d_arg_type_builtin(D_ARG_BUILTIN_CHAR),
                                 &c_low,
                                 &c_high),
                             B_CHR);
    const bool  below  = d_tests_arg_extra_numbers(
                             "B_NEG",
                             d_arg_validator_bound_double(-1.5,
                                                          -0.5),
                             B_NEG);

    return ( (real)   &&
             (letter) &&
             (below) );
}

//==============================================================================
// 3.  ADDRESSES
//==============================================================================

/*
d_tests_arg_extra_ipv4
  CLI11's ValidIPV4.
  Tests the following:
  - three separators, or "Invalid IPV4 address: must have 3 separators"
  - each part an integer, or "Failed parsing number (<part>)": empty parts,
    words, and numbers too large for an `int`
  - each part from 0 to 255, or "Each IP number must be between 0 and 255
    <part>"
  - a part read as an integer is read: prefixes, separators, signs and
    spaces around it
*/
bool
d_tests_arg_extra_ipv4(void)
{
    return d_tests_arg_holds("IPV4",
                             d_arg_validator_valid_ipv4(),
                             ADDRESSES,
                             ADDRESS_COUNT,
                             IPV4);
}

//==============================================================================
// 4.  COMPOSITES
//==============================================================================

/*
d_tests_arg_extra_composites
  The extra checks in CLI11's composites.
  Tests the following:
  - !ValidIPV4 passes what ValidIPV4 refuses, and refuses what it passes
    with "check IPV4 succeeded improperly"
  - Bound(0, 10) & Range(5, 7): the range judges the value the bound made
*/
bool
d_tests_arg_extra_composites(void)
{
    const bool negated = d_tests_arg_holds(
                             "K_NOT",
                             d_arg_validator_not(
                                 d_arg_validator_valid_ipv4()),
                             ADDRESSES,
                             ADDRESS_COUNT,
                             K_NOT);
    const bool bounded = d_tests_arg_extra_numbers(
                             "K_BOUND",
                             d_arg_validator_and(
                                 d_tests_arg_extra_int_bound(0,
                                                             10),
                                 d_tests_arg_extra_int_range(5,
                                                             7)),
                             K_BOUND);

    return ( (negated) &&
             (bounded) );
}

//==============================================================================
// 5.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_extra_departures
  Where the module writes or judges otherwise than CLI11, on purpose: each
transcript's constant says what CLI11 does instead.
  Tests the following:
  - the word `true` is no part of an address
  - a clamped `double` or `float` written as the shortest text that reads
    back as it
  - an 8-bit bound written, and a value it clamps made, in decimal
*/
bool
d_tests_arg_extra_departures(void)
{
    const float         f_low  = 0.1f;
    const float         f_high = 0.3f;
    const unsigned char b_low  = 65u;
    const unsigned char b_high = 100u;
    const bool          address = d_tests_arg_holds(
                                      "DEPARTED_IPV4",
                                      d_arg_validator_valid_ipv4(),
                                      TRUE_ADDRESSES,
                                      TRUE_ADDRESS_COUNT,
                                      DEPARTED_IPV4);
    const bool          real    = d_tests_arg_holds(
                                      "DEPARTED_BDBL",
                                      d_arg_validator_bound_double(0.1,
                                                                   0.3),
                                      CLAMPED,
                                      CLAMPED_COUNT,
                                      DEPARTED_BDBL);
    const bool          single  = d_tests_arg_holds(
                                      "DEPARTED_BFLT",
                                      d_arg_validator_bound(
                                          d_arg_type_builtin(
                                              D_ARG_BUILTIN_FLOAT),
                                          &f_low,
                                          &f_high),
                                      CLAMPED,
                                      CLAMPED_COUNT,
                                      DEPARTED_BFLT);
    const bool          byte    = d_tests_arg_holds(
                                      "DEPARTED_BU8",
                                      d_arg_validator_bound(
                                          d_arg_type_builtin(
                                              D_ARG_BUILTIN_UINT8),
                                          &b_low,
                                          &b_high),
                                      CLAMPED,
                                      CLAMPED_COUNT,
                                      DEPARTED_BU8);

    return ( (address) &&
             (real)    &&
             (single)  &&
             (byte) );
}

//==============================================================================
// 6.  CONSTRUCTION AND COMPLETION
//==============================================================================

/*
d_tests_arg_extra_construction
  Checks that cannot be made.
  Tests the following:
  - a type check of no type, of a type without a conversion, or of a
    container's, whose objects are not plain data, is a failed validator
    holding D_ARG_ERROR_BAD_ARGUMENT, which runs as that error
  - so is a bound of no type, of text, of a type without a conversion or a
    container's, or without either bound
*/
bool
d_tests_arg_extra_construction(void)
{
    const int                      low   = 0;
    const int                      high  = 1;
    const struct d_arg_type* const whole =
        d_arg_type_builtin(D_ARG_BUILTIN_INT);
    const struct d_arg_type* const text  =
        d_arg_type_builtin(D_ARG_BUILTIN_STRING);
    struct d_arg_type              mute  = *whole;
    struct d_arg_type              list  = *whole;

    mute.convert = NULL;
    list.flags  |= D_ARG_TYPE_FLAG_CONTAINER;

    return ( (d_tests_arg_extra_unmade(d_arg_validator_type(NULL)))      &&
             (d_tests_arg_extra_unmade(d_arg_validator_type(&mute)))     &&
             (d_tests_arg_extra_unmade(d_arg_validator_type(&list)))     &&
             (d_tests_arg_extra_unmade(d_arg_validator_bound(NULL,
                                                             &low,
                                                             &high)))    &&
             (d_tests_arg_extra_unmade(d_arg_validator_bound(text,
                                                             &low,
                                                             &high)))    &&
             (d_tests_arg_extra_unmade(d_arg_validator_bound(&mute,
                                                             &low,
                                                             &high)))    &&
             (d_tests_arg_extra_unmade(d_arg_validator_bound(&list,
                                                             &low,
                                                             &high)))    &&
             (d_tests_arg_extra_unmade(d_arg_validator_bound(whole,
                                                             NULL,
                                                             &high)))    &&
             (d_tests_arg_extra_unmade(d_arg_validator_bound(whole,
                                                             &low,
                                                             NULL))) );
}

/*
d_tests_arg_extra_value_kinds
  What the extra checks complete as.
  Tests the following:
  - the type checks of numbers, characters and truth values, Number, the
    bounds and ValidIPV4 complete as numbers
  - the type check of text, and of a type of the caller's that is no
    number, complete as anything
*/
bool
d_tests_arg_extra_value_kinds(void)
{
    const unsigned int number = D_ARG_VALIDATOR_NUMBER;

    return ( (d_tests_arg_kind_of(
                  d_arg_validator_type(
                      d_arg_type_builtin(D_ARG_BUILTIN_INT))) == number)     &&
             (d_tests_arg_kind_of(
                  d_arg_validator_type(
                      d_arg_type_builtin(D_ARG_BUILTIN_DOUBLE))) == number)  &&
             (d_tests_arg_kind_of(
                  d_arg_validator_type(
                      d_arg_type_builtin(D_ARG_BUILTIN_CHAR))) == number)    &&
             (d_tests_arg_kind_of(
                  d_arg_validator_type(
                      d_arg_type_builtin(D_ARG_BUILTIN_BOOL))) == number)    &&
             (d_tests_arg_kind_of(d_arg_validator_number()) == number)       &&
             (d_tests_arg_kind_of(d_arg_validator_bound_long(0L,
                                                             1L)) == number) &&
             (d_tests_arg_kind_of(d_arg_validator_valid_ipv4()) == number)   &&
             (d_tests_arg_kind_of(
                  d_arg_validator_type(
                      d_arg_type_builtin(D_ARG_BUILTIN_STRING))) == 0u)      &&
             (d_tests_arg_kind_of(d_arg_validator_type(&WORD_TYPE)) == 0u) );
}

//==============================================================================
// 7.  IN A PARSE
//==============================================================================

/*
d_tests_arg_extra_in_a_parse
  The extra checks on options, as CLI11 adds them.
  Tests the following:
  - a check that refuses a value fails the parse with ValidationError, its
    message after the option's name, and the variable keeps its zero
  - a value a check passes reaches the variable as given
  - a Bound added by transform() replaces the value with the bound it
    clamps to, in the results and in the variable
*/
bool
d_tests_arg_extra_in_a_parse(void)
{
    const bool bare  = d_tests_arg_validates(&ADDRESS_OPTION,
                                             d_arg_validator_valid_ipv4(),
                                             "1.2.3",
                                             P_ADDRESS_SHORT);
    const bool whole = d_tests_arg_validates(&ADDRESS_OPTION,
                                             d_arg_validator_valid_ipv4(),
                                             "1.2.3.4",
                                             P_ADDRESS);
    const bool word  = d_tests_arg_validates(&RATIO_OPTION,
                                             d_arg_validator_number(),
                                             "abc",
                                             P_RATIO_WORD);
    const bool real  = d_tests_arg_validates(&RATIO_OPTION,
                                             d_arg_validator_number(),
                                             "2.5",
                                             P_RATIO);
    const bool high  = d_tests_arg_validates(&COUNT_OPTION,
                                             d_tests_arg_extra_int_bound(0,
                                                                         10),
                                             "20",
                                             P_COUNT_HIGH);
    const bool low   = d_tests_arg_validates(&COUNT_OPTION,
                                             d_tests_arg_extra_int_bound(0,
                                                                         10),
                                             "-3",
                                             P_COUNT_LOW);
    const bool bad   = d_tests_arg_validates(&COUNT_OPTION,
                                             d_tests_arg_extra_int_bound(0,
                                                                         10),
                                             "x",
                                             P_COUNT_WORD);

    return ( (bare)  &&
             (whole) &&
             (word)  &&
             (real)  &&
             (high)  &&
             (low)   &&
             (bad) );
}

//==============================================================================
// 8.  THE SUITE
//==============================================================================

// d_tests_arg_extra
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_extra[] =
{
    { "arg_extra_type_integers",
      d_tests_arg_extra_type_integers },
    { "arg_extra_type_others",
      d_tests_arg_extra_type_others },
    { "arg_extra_type_custom",
      d_tests_arg_extra_type_custom },
    { "arg_extra_bound_integers",
      d_tests_arg_extra_bound_integers },
    { "arg_extra_bound_others",
      d_tests_arg_extra_bound_others },
    { "arg_extra_ipv4",
      d_tests_arg_extra_ipv4 },
    { "arg_extra_composites",
      d_tests_arg_extra_composites },
    { "arg_extra_departures",
      d_tests_arg_extra_departures },
    { "arg_extra_construction",
      d_tests_arg_extra_construction },
    { "arg_extra_value_kinds",
      d_tests_arg_extra_value_kinds },
    { "arg_extra_in_a_parse",
      d_tests_arg_extra_in_a_parse }
};

// d_tests_arg_extra_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_extra_count =
    sizeof(d_tests_arg_extra) / sizeof(d_tests_arg_extra[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
