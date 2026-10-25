/*******************************************************************************
* djinterp [parse]                                         arg_validator_tests.c
*
* The suite of the argument parser's validators, declared in
* arg_validator_tests.h.
*   The transcripts are CLI11 2.7.2's: each of its default validators, and
* their composites, run over the same values with CLI11's rule for leading
* zeros, written as a description line and a line for each value -- whether
* it passed, as what, or why it was refused. A control character in a value
* is written as its escape, so that every line of a transcript is one line.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_validator_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_validator_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free
#include <string.h>   // strcmp
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the integer rule
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // descriptors
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // validators
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // d_tests_arg_holds, d_tests_arg_kind_of


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// NUMBERS
//   constant: the values the numeric validators are run over, as CLI11's
// were.
static const char* const NUMBERS[] =
{
    "5", "0", "10", "11", "-1", "abc", "", "5.5", " 5", "0x5", "010", "1e0",
    "2.5", "2.6", "nan", "inf", "-0", "1e-320", "-1e-300", "c", "99", "z",
    "3", "7", "12", "30", "\"5\"", "1_000"
};

// NUMBER_COUNT
//   constant: how many values NUMBERS holds.
static const size_t NUMBER_COUNT = sizeof(NUMBERS) / sizeof(NUMBERS[0]);

// ESCAPES
//   constant: the values EscapedString is run over, as CLI11's was: quotes
// of each kind, each escape, binary strings, and malformed ones.
static const char* const ESCAPES[] =
{
    "plain", "\"quoted\"", "\"a\\tb\"", "'single'", "`back`", "a\\nb",
    "\"bad\\q\"", "bad\\", "\\u0041", "\\u00", "\\U0001F600",
    "B\"(\\x41\\x42)\"", "'B\"(\\x41)\"'", "B\"(AB)\"", "\"\"", "\"",
    "\\0x", "x\\\"y", "'B\"(no)\"'", "\\uD800", "\\U00110000",
    "\"\\u00e9\""
};

// ESCAPE_COUNT
//   constant: how many values ESCAPES holds.
static const size_t ESCAPE_COUNT = sizeof(ESCAPES) / sizeof(ESCAPES[0]);

// RANGE_INT
//   constant: CLI11's Range(0, 10) over `int`.
static const char RANGE_INT[] =
    "desc <INT in [0 - 10]>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> fail <Value 11 not in range [0 - 10]>\n"
    "[-1] -> fail <Value -1 not in range [0 - 10]>\n"
    "[abc] -> fail <Value abc not in range [0 - 10]>\n"
    "[] -> fail <Value  not in range [0 - 10]>\n"
    "[5.5] -> fail <Value 5.5 not in range [0 - 10]>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[1e0] -> fail <Value 1e0 not in range [0 - 10]>\n"
    "[2.5] -> fail <Value 2.5 not in range [0 - 10]>\n"
    "[2.6] -> fail <Value 2.6 not in range [0 - 10]>\n"
    "[nan] -> fail <Value nan not in range [0 - 10]>\n"
    "[inf] -> fail <Value inf not in range [0 - 10]>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <Value 1e-320 not in range [0 - 10]>\n"
    "[-1e-300] -> fail <Value -1e-300 not in range [0 - 10]>\n"
    "[c] -> fail <Value c not in range [0 - 10]>\n"
    "[99] -> fail <Value 99 not in range [0 - 10]>\n"
    "[z] -> fail <Value z not in range [0 - 10]>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> fail <Value 12 not in range [0 - 10]>\n"
    "[30] -> fail <Value 30 not in range [0 - 10]>\n"
    "[\"5\"] -> fail <Value \"5\" not in range [0 - 10]>\n"
    "[1_000] -> fail <Value 1_000 not in range [0 - 10]>\n";

// RANGE_UINT
//   constant: CLI11's Range(0u, 10u) over `unsigned int`.
static const char RANGE_UINT[] =
    "desc <UINT in [0 - 10]>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> fail <Value 11 not in range [0 - 10]>\n"
    "[-1] -> fail <Value -1 not in range [0 - 10]>\n"
    "[abc] -> fail <Value abc not in range [0 - 10]>\n"
    "[] -> fail <Value  not in range [0 - 10]>\n"
    "[5.5] -> fail <Value 5.5 not in range [0 - 10]>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[1e0] -> fail <Value 1e0 not in range [0 - 10]>\n"
    "[2.5] -> fail <Value 2.5 not in range [0 - 10]>\n"
    "[2.6] -> fail <Value 2.6 not in range [0 - 10]>\n"
    "[nan] -> fail <Value nan not in range [0 - 10]>\n"
    "[inf] -> fail <Value inf not in range [0 - 10]>\n"
    "[-0] -> fail <Value -0 not in range [0 - 10]>\n"
    "[1e-320] -> fail <Value 1e-320 not in range [0 - 10]>\n"
    "[-1e-300] -> fail <Value -1e-300 not in range [0 - 10]>\n"
    "[c] -> fail <Value c not in range [0 - 10]>\n"
    "[99] -> fail <Value 99 not in range [0 - 10]>\n"
    "[z] -> fail <Value z not in range [0 - 10]>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> fail <Value 12 not in range [0 - 10]>\n"
    "[30] -> fail <Value 30 not in range [0 - 10]>\n"
    "[\"5\"] -> fail <Value \"5\" not in range [0 - 10]>\n"
    "[1_000] -> fail <Value 1_000 not in range [0 - 10]>\n";

// RANGE_DOUBLE
//   constant: CLI11's Range(0.5, 2.5) over `double`.
static const char RANGE_DOUBLE[] =
    "desc <FLOAT in [0.5 - 2.5]>\n"
    "[5] -> fail <Value 5 not in range [0.5 - 2.5]>\n"
    "[0] -> fail <Value 0 not in range [0.5 - 2.5]>\n"
    "[10] -> fail <Value 10 not in range [0.5 - 2.5]>\n"
    "[11] -> fail <Value 11 not in range [0.5 - 2.5]>\n"
    "[-1] -> fail <Value -1 not in range [0.5 - 2.5]>\n"
    "[abc] -> fail <Value abc not in range [0.5 - 2.5]>\n"
    "[] -> fail <Value  not in range [0.5 - 2.5]>\n"
    "[5.5] -> fail <Value 5.5 not in range [0.5 - 2.5]>\n"
    "[ 5] -> fail <Value  5 not in range [0.5 - 2.5]>\n"
    "[0x5] -> fail <Value 0x5 not in range [0.5 - 2.5]>\n"
    "[010] -> fail <Value 010 not in range [0.5 - 2.5]>\n"
    "[1e0] -> pass <1e0>\n"
    "[2.5] -> pass <2.5>\n"
    "[2.6] -> fail <Value 2.6 not in range [0.5 - 2.5]>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> fail <Value inf not in range [0.5 - 2.5]>\n"
    "[-0] -> fail <Value -0 not in range [0.5 - 2.5]>\n"
    "[1e-320] -> fail <Value 1e-320 not in range [0.5 - 2.5]>\n"
    "[-1e-300] -> fail <Value -1e-300 not in range [0.5 - 2.5]>\n"
    "[c] -> fail <Value c not in range [0.5 - 2.5]>\n"
    "[99] -> fail <Value 99 not in range [0.5 - 2.5]>\n"
    "[z] -> fail <Value z not in range [0.5 - 2.5]>\n"
    "[3] -> fail <Value 3 not in range [0.5 - 2.5]>\n"
    "[7] -> fail <Value 7 not in range [0.5 - 2.5]>\n"
    "[12] -> fail <Value 12 not in range [0.5 - 2.5]>\n"
    "[30] -> fail <Value 30 not in range [0.5 - 2.5]>\n"
    "[\"5\"] -> fail <Value \"5\" not in range [0.5 - 2.5]>\n"
    "[1_000] -> fail <Value 1_000 not in range [0.5 - 2.5]>\n";

// RANGE_CHAR
//   constant: CLI11's Range('a', 'f') over `char`.
static const char RANGE_CHAR[] =
    "desc <CHAR in [a - f]>\n"
    "[5] -> fail <Value 5 not in range [a - f]>\n"
    "[0] -> fail <Value 0 not in range [a - f]>\n"
    "[10] -> fail <Value 10 not in range [a - f]>\n"
    "[11] -> fail <Value 11 not in range [a - f]>\n"
    "[-1] -> fail <Value -1 not in range [a - f]>\n"
    "[abc] -> fail <Value abc not in range [a - f]>\n"
    "[] -> fail <Value  not in range [a - f]>\n"
    "[5.5] -> fail <Value 5.5 not in range [a - f]>\n"
    "[ 5] -> fail <Value  5 not in range [a - f]>\n"
    "[0x5] -> fail <Value 0x5 not in range [a - f]>\n"
    "[010] -> fail <Value 010 not in range [a - f]>\n"
    "[1e0] -> fail <Value 1e0 not in range [a - f]>\n"
    "[2.5] -> fail <Value 2.5 not in range [a - f]>\n"
    "[2.6] -> fail <Value 2.6 not in range [a - f]>\n"
    "[nan] -> fail <Value nan not in range [a - f]>\n"
    "[inf] -> fail <Value inf not in range [a - f]>\n"
    "[-0] -> fail <Value -0 not in range [a - f]>\n"
    "[1e-320] -> fail <Value 1e-320 not in range [a - f]>\n"
    "[-1e-300] -> fail <Value -1e-300 not in range [a - f]>\n"
    "[c] -> pass <c>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Value z not in range [a - f]>\n"
    "[3] -> fail <Value 3 not in range [a - f]>\n"
    "[7] -> fail <Value 7 not in range [a - f]>\n"
    "[12] -> fail <Value 12 not in range [a - f]>\n"
    "[30] -> fail <Value 30 not in range [a - f]>\n"
    "[\"5\"] -> fail <Value \"5\" not in range [a - f]>\n"
    "[1_000] -> fail <Value 1_000 not in range [a - f]>\n";

// POSITIVE
//   constant: CLI11's PositiveNumber.
static const char POSITIVE[] =
    "desc <POSITIVE>\n"
    "[5] -> pass <5>\n"
    "[0] -> fail <Value 0 not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> fail <Value -1 not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[abc] -> fail <Value abc not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[] -> fail <Value  not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[5.5] -> pass <5.5>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[1e0] -> pass <1e0>\n"
    "[2.5] -> pass <2.5>\n"
    "[2.6] -> pass <2.6>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> fail <Value inf not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[-0] -> fail <Value -0 not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[1e-320] -> pass <1e-320>\n"
    "[-1e-300] -> fail <Value -1e-300 not in range [4.94066e-324 - 1.79769e+308"
    "]>\n"
    "[c] -> fail <Value c not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Value z not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Value \"5\" not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[1_000] -> pass <1_000>\n";

// NON_NEGATIVE
//   constant: CLI11's NonNegativeNumber.
static const char NON_NEGATIVE[] =
    "desc <NONNEGATIVE>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> fail <Value -1 not in range [0 - 1.79769e+308]>\n"
    "[abc] -> fail <Value abc not in range [0 - 1.79769e+308]>\n"
    "[] -> fail <Value  not in range [0 - 1.79769e+308]>\n"
    "[5.5] -> pass <5.5>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[1e0] -> pass <1e0>\n"
    "[2.5] -> pass <2.5>\n"
    "[2.6] -> pass <2.6>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> fail <Value inf not in range [0 - 1.79769e+308]>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> pass <1e-320>\n"
    "[-1e-300] -> fail <Value -1e-300 not in range [0 - 1.79769e+308]>\n"
    "[c] -> fail <Value c not in range [0 - 1.79769e+308]>\n"
    "[99] -> pass <99>\n"
    "[z] -> fail <Value z not in range [0 - 1.79769e+308]>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> fail <Value \"5\" not in range [0 - 1.79769e+308]>\n"
    "[1_000] -> pass <1_000>\n";

// ESCAPED
//   constant: CLI11's EscapedString.
static const char ESCAPED[] =
    "desc <>\n"
    "[plain] -> pass <plain>\n"
    "[\"quoted\"] -> pass <quoted>\n"
    "[\"a\\tb\"] -> pass <a\\tb>\n"
    "['single'] -> pass <single>\n"
    "[`back`] -> pass <back>\n"
    "[a\\nb] -> pass <a\\nb>\n"
    "[\"bad\\q\"] -> fail <unrecognized escape sequence \\q in bad\\q>\n"
    "[bad\\] -> fail <invalid escape sequence bad\\>\n"
    "[\\u0041] -> pass <A>\n"
    "[\\u00] -> fail <unicode sequence must have 4 hex codes \\u00>\n"
    "[\\U0001F600] -> pass <\xf0\x9f\x98\x80>\n"
    "[B\"(\\x41\\x42)\"] -> pass <AB>\n"
    "['B\"(\\x41)\"'] -> pass <A>\n"
    "[B\"(AB)\"] -> pass <B\"(AB)\">\n"
    "[\"\"] -> pass <>\n"
    "[\"] -> pass <\">\n"
    "[\\0x] -> pass <\\x00x>\n"
    "[x\\\"y] -> pass <x\"y>\n"
    "['B\"(no)\"'] -> pass <no>\n"
    "[\\uD800] -> fail <[0xD800, 0xDFFF] are not valid code points.>\n"
    "[\\U00110000] -> fail <values above 0x10FFFF are not valid code points.>\n"
    "[\"\\u00e9\"] -> pass <\xc3\xa9>\n";

// NON_MODIFYING
//   constant: CLI11's EscapedString made non_modifying().
static const char NON_MODIFYING[] =
    "desc <>\n"
    "[plain] -> pass <plain>\n"
    "[\"quoted\"] -> pass <\"quoted\">\n"
    "[\"a\\tb\"] -> pass <\"a\\tb\">\n"
    "['single'] -> pass <'single'>\n"
    "[`back`] -> pass <`back`>\n"
    "[a\\nb] -> pass <a\\nb>\n"
    "[\"bad\\q\"] -> fail <unrecognized escape sequence \\q in bad\\q>\n"
    "[bad\\] -> fail <invalid escape sequence bad\\>\n"
    "[\\u0041] -> pass <\\u0041>\n"
    "[\\u00] -> fail <unicode sequence must have 4 hex codes \\u00>\n"
    "[\\U0001F600] -> pass <\\U0001F600>\n"
    "[B\"(\\x41\\x42)\"] -> pass <B\"(\\x41\\x42)\">\n"
    "['B\"(\\x41)\"'] -> pass <'B\"(\\x41)\"'>\n"
    "[B\"(AB)\"] -> pass <B\"(AB)\">\n"
    "[\"\"] -> pass <\"\">\n"
    "[\"] -> pass <\">\n"
    "[\\0x] -> pass <\\0x>\n"
    "[x\\\"y] -> pass <x\\\"y>\n"
    "['B\"(no)\"'] -> pass <'B\"(no)\"'>\n"
    "[\\uD800] -> fail <[0xD800, 0xDFFF] are not valid code points.>\n"
    "[\\U00110000] -> fail <values above 0x10FFFF are not valid code points.>\n"
    "[\"\\u00e9\"] -> pass <\"\\u00e9\">\n";

// AND
//   constant: CLI11's Range(0, 10) & Range(5, 20).
static const char AND[] =
    "desc <(INT in [0 - 10]) AND (INT in [5 - 20])>\n"
    "[5] -> pass <5>\n"
    "[0] -> fail <Value 0 not in range [5 - 20]>\n"
    "[10] -> pass <10>\n"
    "[11] -> fail <Value 11 not in range [0 - 10]>\n"
    "[-1] -> fail <(Value -1 not in range [0 - 10]) AND (Value -1 not in range "
    "[5 - 20])>\n"
    "[abc] -> fail <(Value abc not in range [0 - 10]) AND (Value abc not in ran"
    "ge [5 - 20])>\n"
    "[] -> fail <(Value  not in range [0 - 10]) AND (Value  not in range [5 - 2"
    "0])>\n"
    "[5.5] -> fail <(Value 5.5 not in range [0 - 10]) AND (Value 5.5 not in ran"
    "ge [5 - 20])>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[1e0] -> fail <(Value 1e0 not in range [0 - 10]) AND (Value 1e0 not in ran"
    "ge [5 - 20])>\n"
    "[2.5] -> fail <(Value 2.5 not in range [0 - 10]) AND (Value 2.5 not in ran"
    "ge [5 - 20])>\n"
    "[2.6] -> fail <(Value 2.6 not in range [0 - 10]) AND (Value 2.6 not in ran"
    "ge [5 - 20])>\n"
    "[nan] -> fail <(Value nan not in range [0 - 10]) AND (Value nan not in ran"
    "ge [5 - 20])>\n"
    "[inf] -> fail <(Value inf not in range [0 - 10]) AND (Value inf not in ran"
    "ge [5 - 20])>\n"
    "[-0] -> fail <Value -0 not in range [5 - 20]>\n"
    "[1e-320] -> fail <(Value 1e-320 not in range [0 - 10]) AND (Value 1e-320 n"
    "ot in range [5 - 20])>\n"
    "[-1e-300] -> fail <(Value -1e-300 not in range [0 - 10]) AND (Value -1e-30"
    "0 not in range [5 - 20])>\n"
    "[c] -> fail <(Value c not in range [0 - 10]) AND (Value c not in range [5 "
    "- 20])>\n"
    "[99] -> fail <(Value 99 not in range [0 - 10]) AND (Value 99 not in range "
    "[5 - 20])>\n"
    "[z] -> fail <(Value z not in range [0 - 10]) AND (Value z not in range [5 "
    "- 20])>\n"
    "[3] -> fail <Value 3 not in range [5 - 20]>\n"
    "[7] -> pass <7>\n"
    "[12] -> fail <Value 12 not in range [0 - 10]>\n"
    "[30] -> fail <(Value 30 not in range [0 - 10]) AND (Value 30 not in range "
    "[5 - 20])>\n"
    "[\"5\"] -> fail <(Value \"5\" not in range [0 - 10]) AND (Value \"5\" not "
    "in range [5 - 20])>\n"
    "[1_000] -> fail <(Value 1_000 not in range [0 - 10]) AND (Value 1_000 not "
    "in range [5 - 20])>\n";

// OR
//   constant: CLI11's Range(0, 5) | Range(10, 15).
static const char OR[] =
    "desc <(INT in [0 - 5]) OR (INT in [10 - 15])>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> fail <(Value -1 not in range [0 - 5]) OR (Value -1 not in range [1"
    "0 - 15])>\n"
    "[abc] -> fail <(Value abc not in range [0 - 5]) OR (Value abc not in range"
    " [10 - 15])>\n"
    "[] -> fail <(Value  not in range [0 - 5]) OR (Value  not in range [10 - 15"
    "])>\n"
    "[5.5] -> fail <(Value 5.5 not in range [0 - 5]) OR (Value 5.5 not in range"
    " [10 - 15])>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> fail <(Value 010 not in range [0 - 5]) OR (Value 010 not in range"
    " [10 - 15])>\n"
    "[1e0] -> fail <(Value 1e0 not in range [0 - 5]) OR (Value 1e0 not in range"
    " [10 - 15])>\n"
    "[2.5] -> fail <(Value 2.5 not in range [0 - 5]) OR (Value 2.5 not in range"
    " [10 - 15])>\n"
    "[2.6] -> fail <(Value 2.6 not in range [0 - 5]) OR (Value 2.6 not in range"
    " [10 - 15])>\n"
    "[nan] -> fail <(Value nan not in range [0 - 5]) OR (Value nan not in range"
    " [10 - 15])>\n"
    "[inf] -> fail <(Value inf not in range [0 - 5]) OR (Value inf not in range"
    " [10 - 15])>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <(Value 1e-320 not in range [0 - 5]) OR (Value 1e-320 not"
    " in range [10 - 15])>\n"
    "[-1e-300] -> fail <(Value -1e-300 not in range [0 - 5]) OR (Value -1e-300 "
    "not in range [10 - 15])>\n"
    "[c] -> fail <(Value c not in range [0 - 5]) OR (Value c not in range [10 -"
    " 15])>\n"
    "[99] -> fail <(Value 99 not in range [0 - 5]) OR (Value 99 not in range [1"
    "0 - 15])>\n"
    "[z] -> fail <(Value z not in range [0 - 5]) OR (Value z not in range [10 -"
    " 15])>\n"
    "[3] -> pass <3>\n"
    "[7] -> fail <(Value 7 not in range [0 - 5]) OR (Value 7 not in range [10 -"
    " 15])>\n"
    "[12] -> pass <12>\n"
    "[30] -> fail <(Value 30 not in range [0 - 5]) OR (Value 30 not in range [1"
    "0 - 15])>\n"
    "[\"5\"] -> fail <(Value \"5\" not in range [0 - 5]) OR (Value \"5\" not in"
    " range [10 - 15])>\n"
    "[1_000] -> fail <(Value 1_000 not in range [0 - 5]) OR (Value 1_000 not in"
    " range [10 - 15])>\n";

// NOT
//   constant: CLI11's !Range(0, 5).
static const char NOT[] =
    "desc <NOT INT in [0 - 5]>\n"
    "[5] -> fail <check INT in [0 - 5] succeeded improperly>\n"
    "[0] -> fail <check INT in [0 - 5] succeeded improperly>\n"
    "[10] -> pass <10>\n"
    "[11] -> pass <11>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> pass <abc>\n"
    "[] -> pass <>\n"
    "[5.5] -> pass <5.5>\n"
    "[ 5] -> fail <check INT in [0 - 5] succeeded improperly>\n"
    "[0x5] -> fail <check INT in [0 - 5] succeeded improperly>\n"
    "[010] -> pass <010>\n"
    "[1e0] -> pass <1e0>\n"
    "[2.5] -> pass <2.5>\n"
    "[2.6] -> pass <2.6>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> pass <inf>\n"
    "[-0] -> fail <check INT in [0 - 5] succeeded improperly>\n"
    "[1e-320] -> pass <1e-320>\n"
    "[-1e-300] -> pass <-1e-300>\n"
    "[c] -> pass <c>\n"
    "[99] -> pass <99>\n"
    "[z] -> pass <z>\n"
    "[3] -> fail <check INT in [0 - 5] succeeded improperly>\n"
    "[7] -> pass <7>\n"
    "[12] -> pass <12>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> pass <\"5\">\n"
    "[1_000] -> pass <1_000>\n";

// NOT_OR
//   constant: CLI11's !(Range(0, 5) | Range(10, 15)).
static const char NOT_OR[] =
    "desc <NOT (INT in [0 - 5]) OR (INT in [10 - 15])>\n"
    "[5] -> fail <check (INT in [0 - 5]) OR (INT in [10 - 15]) succeeded improp"
    "erly>\n"
    "[0] -> fail <check (INT in [0 - 5]) OR (INT in [10 - 15]) succeeded improp"
    "erly>\n"
    "[10] -> fail <check (INT in [0 - 5]) OR (INT in [10 - 15]) succeeded impro"
    "perly>\n"
    "[11] -> fail <check (INT in [0 - 5]) OR (INT in [10 - 15]) succeeded impro"
    "perly>\n"
    "[-1] -> pass <-1>\n"
    "[abc] -> pass <abc>\n"
    "[] -> pass <>\n"
    "[5.5] -> pass <5.5>\n"
    "[ 5] -> fail <check (INT in [0 - 5]) OR (INT in [10 - 15]) succeeded impro"
    "perly>\n"
    "[0x5] -> fail <check (INT in [0 - 5]) OR (INT in [10 - 15]) succeeded impr"
    "operly>\n"
    "[010] -> pass <010>\n"
    "[1e0] -> pass <1e0>\n"
    "[2.5] -> pass <2.5>\n"
    "[2.6] -> pass <2.6>\n"
    "[nan] -> pass <nan>\n"
    "[inf] -> pass <inf>\n"
    "[-0] -> fail <check (INT in [0 - 5]) OR (INT in [10 - 15]) succeeded impro"
    "perly>\n"
    "[1e-320] -> pass <1e-320>\n"
    "[-1e-300] -> pass <-1e-300>\n"
    "[c] -> pass <c>\n"
    "[99] -> pass <99>\n"
    "[z] -> pass <z>\n"
    "[3] -> fail <check (INT in [0 - 5]) OR (INT in [10 - 15]) succeeded improp"
    "erly>\n"
    "[7] -> pass <7>\n"
    "[12] -> fail <check (INT in [0 - 5]) OR (INT in [10 - 15]) succeeded impro"
    "perly>\n"
    "[30] -> pass <30>\n"
    "[\"5\"] -> pass <\"5\">\n"
    "[1_000] -> pass <1_000>\n";

// ESCAPED_AND
//   constant: CLI11's EscapedString & Range(0, 10).
static const char ESCAPED_AND[] =
    "desc <INT in [0 - 10]>\n"
    "[5] -> pass <5>\n"
    "[0] -> pass <0>\n"
    "[10] -> pass <10>\n"
    "[11] -> fail <Value 11 not in range [0 - 10]>\n"
    "[-1] -> fail <Value -1 not in range [0 - 10]>\n"
    "[abc] -> fail <Value abc not in range [0 - 10]>\n"
    "[] -> fail <Value  not in range [0 - 10]>\n"
    "[5.5] -> fail <Value 5.5 not in range [0 - 10]>\n"
    "[ 5] -> pass < 5>\n"
    "[0x5] -> pass <0x5>\n"
    "[010] -> pass <010>\n"
    "[1e0] -> fail <Value 1e0 not in range [0 - 10]>\n"
    "[2.5] -> fail <Value 2.5 not in range [0 - 10]>\n"
    "[2.6] -> fail <Value 2.6 not in range [0 - 10]>\n"
    "[nan] -> fail <Value nan not in range [0 - 10]>\n"
    "[inf] -> fail <Value inf not in range [0 - 10]>\n"
    "[-0] -> pass <-0>\n"
    "[1e-320] -> fail <Value 1e-320 not in range [0 - 10]>\n"
    "[-1e-300] -> fail <Value -1e-300 not in range [0 - 10]>\n"
    "[c] -> fail <Value c not in range [0 - 10]>\n"
    "[99] -> fail <Value 99 not in range [0 - 10]>\n"
    "[z] -> fail <Value z not in range [0 - 10]>\n"
    "[3] -> pass <3>\n"
    "[7] -> pass <7>\n"
    "[12] -> fail <Value 12 not in range [0 - 10]>\n"
    "[30] -> fail <Value 30 not in range [0 - 10]>\n"
    "[\"5\"] -> pass <5>\n"
    "[1_000] -> fail <Value 1_000 not in range [0 - 10]>\n";

// INACTIVE
//   constant: CLI11's Range(0, 5).active(false).
static const char INACTIVE[] =
    "desc <>\n"
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
    "[1_000] -> pass <1_000>\n";

// HALF_ACTIVE
//   constant: CLI11's Range(0, 5).active(false) & Range(0, 10).
static const char HALF_ACTIVE[] =
    "desc <>\n"
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
    "[1_000] -> pass <1_000>\n";

// DESCRIBED
//   constant: CLI11's Range(1, 3, "MYRANGE").
static const char DESCRIBED[] =
    "desc <MYRANGE>\n"
    "[5] -> fail <Value 5 not in range [1 - 3]>\n"
    "[0] -> fail <Value 0 not in range [1 - 3]>\n"
    "[10] -> fail <Value 10 not in range [1 - 3]>\n"
    "[11] -> fail <Value 11 not in range [1 - 3]>\n"
    "[-1] -> fail <Value -1 not in range [1 - 3]>\n"
    "[abc] -> fail <Value abc not in range [1 - 3]>\n"
    "[] -> fail <Value  not in range [1 - 3]>\n"
    "[5.5] -> fail <Value 5.5 not in range [1 - 3]>\n"
    "[ 5] -> fail <Value  5 not in range [1 - 3]>\n"
    "[0x5] -> fail <Value 0x5 not in range [1 - 3]>\n"
    "[010] -> fail <Value 010 not in range [1 - 3]>\n"
    "[1e0] -> fail <Value 1e0 not in range [1 - 3]>\n"
    "[2.5] -> fail <Value 2.5 not in range [1 - 3]>\n"
    "[2.6] -> fail <Value 2.6 not in range [1 - 3]>\n"
    "[nan] -> fail <Value nan not in range [1 - 3]>\n"
    "[inf] -> fail <Value inf not in range [1 - 3]>\n"
    "[-0] -> fail <Value -0 not in range [1 - 3]>\n"
    "[1e-320] -> fail <Value 1e-320 not in range [1 - 3]>\n"
    "[-1e-300] -> fail <Value -1e-300 not in range [1 - 3]>\n"
    "[c] -> fail <Value c not in range [1 - 3]>\n"
    "[99] -> fail <Value 99 not in range [1 - 3]>\n"
    "[z] -> fail <Value z not in range [1 - 3]>\n"
    "[3] -> pass <3>\n"
    "[7] -> fail <Value 7 not in range [1 - 3]>\n"
    "[12] -> fail <Value 12 not in range [1 - 3]>\n"
    "[30] -> fail <Value 30 not in range [1 - 3]>\n"
    "[\"5\"] -> fail <Value \"5\" not in range [1 - 3]>\n"
    "[1_000] -> fail <Value 1_000 not in range [1 - 3]>\n";

// d_tests_arg_validator_counts
//   struct: what an owned context has been through: how many copies were
// made of it, and how many copies or originals were freed.
struct d_tests_arg_validator_counts
{
    int copies;
    int releases;
};

// d_tests_arg_validator_context
//   struct: a context a validator owns, which counts what is done to it in
// the counts it shares with its copies.
struct d_tests_arg_validator_context
{
    struct d_tests_arg_validator_counts* counts;
};

/**
 * @brief Makes CLI11's Range over `int`, as the composites use it.
 *
 * @param[in] _low   the least value.
 * @param[in] _high  the greatest value.
 * @return the check.
 *
 * CLI11's Range over `int`, as the composites use it.
 */
static struct d_arg_validator
d_tests_arg_validator_int_range(
    int _low,
    int _high
)
{
    return d_arg_validator_range(d_arg_type_builtin(D_ARG_BUILTIN_INT),
                                 &_low,
                                 &_high);
}

/**
 * @brief Raises a value's letters to upper case.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the validation, whose value is replaced or
 *                             refused.
 * @return D_ARG_OK, with the value replaced or refused; otherwise the status of
 *         a replacement or refusal that could not be recorded.
 *
 * A custom validation: the value in upper case, or a refusal of the empty
 * value.
 */
static int
d_tests_arg_validator_upper(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    char   upper[32] = { 0 };
    size_t length    = 0u;

    (void)_validator;

    // the empty value has no upper case
    if (_validation->value.length == 0u)
    {
        return d_arg_validation_reject(_validation,
                                       "nothing to raise");
    }

    // each letter raised, as far as the room goes
    while ( (length < _validation->value.length) &&
            (length < sizeof(upper) - 1u) )
    {
        const char letter = _validation->value.data[length];

        upper[length] = ( (letter >= 'a') &&
                          (letter <= 'z') )
                        ? (char)(letter - 'a' + 'A')
                        : letter;
        ++length;
    }

    return d_arg_validation_replace(_validation,
                                    d_pack_text_from_span(upper,
                                                          length));
}

/**
 * @brief Stops a validation with an error.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the validation, which records a reason.
 * @return D_ARG_ERROR_FILE.
 *
 * A custom validation that cannot reach a verdict: it stops with an error,
 * as a CLI11 validator that throws.
 */
static int
d_tests_arg_validator_stop(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    (void)_validator;
    (void)d_arg_validation_reject(_validation,
                                  "cannot tell");

    return D_ARG_ERROR_FILE;
}

/**
 * @brief Passes every value, counting it.
 *
 * @param[in] _validator   the validator, whose data points at the `int` that
 *                         counts.
 * @param[in] _validation  the validation; unused.
 * @return D_ARG_OK.
 *
 * A custom validation that passes every value, counting the values in the
 * `int` its context points at.
 */
static int
d_tests_arg_validator_tally(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    (void)_validation;

    ++*(int*)_validator->data;

    return D_ARG_OK;
}

/**
 * @brief Copies an owned context, counting the copy.
 *
 * @param[in] _context  the d_tests_arg_validator_context to copy.
 * @return the copy, which d_tests_arg_validator_release_context frees; NULL
 *         when it could not be allocated.
 *
 * Copies an owned context, counting the copy.
 */
static void*
d_tests_arg_validator_copy_context(
    const void* _context
)
{
    const struct d_tests_arg_validator_context* const source = _context;
    struct d_tests_arg_validator_context* const       copy   =
        malloc(sizeof(*copy));

    // a copy made counts
    if (copy != NULL)
    {
        copy->counts = source->counts;
        ++copy->counts->copies;
    }

    return copy;
}

/**
 * @brief Frees an owned context, counting the release.
 *
 * @param[in] _context  the d_tests_arg_validator_context, freed.
 *
 * Frees an owned context, counting the release.
 */
static void
d_tests_arg_validator_release_context(
    void* _context
)
{
    struct d_tests_arg_validator_context* const context = _context;

    ++context->counts->releases;
    free(context);

    return;
}

/**
 * @brief Lists the letters of a validator's description as its choices.
 *
 * @param[in] _validator  the validator.
 * @param[in] _each       receives each choice.
 * @param[in] _context    `_each`'s context.
 * @return D_ARG_OK, or the first status other than D_ARG_OK that `_each`
 *         returned.
 *
 * A d_arg_choices_fn of the caller's: the letters of the validator's
 * description, each a choice.
 */
static int
d_tests_arg_validator_list(
    const struct d_arg_validator* _validator,
    d_arg_choice_fn               _each,
    void*                         _context
)
{
    int status = D_ARG_OK;

    // each letter of the description
    for (const char* letter = _validator->description;
         (status == D_ARG_OK) && (letter != NULL) && (*letter != '\0');
         ++letter)
    {
        status = _each(d_pack_text_from_span(letter,
                                             1u),
                       _context);
    }

    return status;
}

/**
 * @brief Appends a choice and a `|` to a buffer.
 *
 * @param[in] _choice   the choice.
 * @param[in] _context  the d_arg_buffer.
 * @return D_ARG_OK, or the buffer's status.
 *
 * A d_arg_choice_fn that appends each choice and a `|` to a buffer.
 */
static int
d_tests_arg_validator_collect(
    struct d_pack_text _choice,
    void*              _context
)
{
    struct d_arg_buffer* const out = _context;

    (void)d_arg_buffer_append_text(out,
                                   _choice);

    return d_arg_buffer_append_char(out,
                                    '|');
}

/**
 * @brief Stops a listing at `b`.
 *
 * @param[in] _choice   the choice.
 * @param[in] _context  unused.
 * @return D_ARG_ERROR_HORRIBLE for `b`; D_ARG_OK otherwise.
 *
 * A d_arg_choice_fn that stops the listing at `b`.
 */
static int
d_tests_arg_validator_stop_at_b(
    struct d_pack_text _choice,
    void*              _context
)
{
    (void)_context;

    return ( (_choice.length == 1u) &&
             (_choice.data[0] == 'b') )
           ? D_ARG_ERROR_HORRIBLE
           : D_ARG_OK;
}

/**
 * @brief Makes a validator that lists the letters of its description as its
 *        choices.
 *
 * @param[in] _letters  the description: the letters.
 * @return the validator.
 *
 * A validator that lists the letters of its description as its choices.
 */
static struct d_arg_validator
d_tests_arg_validator_lister(
    const char* _letters
)
{
    struct d_arg_validator validator = d_arg_validator_custom(NULL,
                                                              NULL,
                                                              _letters);

    d_arg_validator_set_choices(&validator,
                                d_tests_arg_validator_list);

    return validator;
}

/**
 * @brief Tells whether a validator lists the choices expected, and releases it.
 *
 * @param[in] _validator  the validator, consumed.
 * @param[in] _expected   the choices expected, as `a|b|...|`.
 * @return `true` when the listing ran to its end and gave the choices expected;
 *         `false` otherwise.
 *
 * Whether a validator lists the choices expected, as `a|b|...|`; consumes
 * the validator.
 */
static bool
d_tests_arg_validator_listed(
    struct d_arg_validator _validator,
    const char*            _expected
)
{
    struct d_arg_buffer listed = D_ARG_BUFFER_INIT;
    const int           status =
        d_arg_validator_each_choice(&_validator,
                                    d_tests_arg_validator_collect,
                                    &listed);
    const bool          same   = d_tests_arg_same("choices",
                                                  d_arg_buffer_text(&listed),
                                                  _expected);

    d_arg_buffer_release(&listed);
    d_arg_validator_release(&_validator);

    return ( (status == D_ARG_OK) &&
             (same) );
}

/**
 * @brief Makes a validator of the caller's that completes as the kind given.
 *
 * @param[in] _kind  the kind: 0, D_ARG_VALIDATOR_PATH,
 *                   D_ARG_VALIDATOR_DIRECTORY, or D_ARG_VALIDATOR_NUMBER.
 * @return the validator.
 *
 * A validator of the caller's that completes as the kind given.
 */
static struct d_arg_validator
d_tests_arg_validator_kinded(
    uint32_t _kind
)
{
    struct d_arg_validator validator = d_arg_validator_custom(NULL,
                                                              NULL,
                                                              NULL);

    d_arg_validator_set_value_kind(&validator,
                                   _kind);

    return validator;
}

//==============================================================================
// 1.  VALIDATION
//==============================================================================

/*
d_tests_arg_validator_validation
  One value on its way through validators.
  Tests the following:
  - a new validation holds the value, has passed, and reads integers by the
    configured rule
  - a replacement is a copy the validation keeps
  - an empty rejection records nothing; a formatted one fails the value
  - NULL is refused, or reads as a value that did not pass
*/
bool
d_tests_arg_validator_validation(void)
{
    char                    text[8]    = "start";
    struct d_arg_validation validation =
    {
        { NULL, 0u },
        D_ARG_BUFFER_INIT,
        D_ARG_BUFFER_INIT,
        NULL,
        0
    };

    d_arg_validation_init(&validation,
                          d_pack_text_from_cstr(text));

    const bool fresh    = ( (d_arg_validation_passed(&validation))        &&
                            (validation.value.data == text)               &&
                            (validation.option == NULL)                   &&
                            (validation.integer_rule ==
                             (int)D_INTERNAL_ARG_PARSER_INTEGER_RULE) );
    const bool replaced = ( (d_arg_validation_replace(
                                 &validation,
                                 d_pack_text_from_cstr("changed"))
                             == D_ARG_OK)                                 &&
                            (validation.value.data != text)               &&
                            (strcmp(validation.value.data,
                                    "changed") == 0) );
    const bool kept     = ( (d_arg_validation_reject(&validation,
                                                     "") == D_ARG_OK)     &&
                            (d_arg_validation_passed(&validation)) );
    const bool rejected = ( (d_arg_validation_rejectf(&validation,
                                                      "bad %d",
                                                      7) == D_ARG_OK)     &&
                            (!d_arg_validation_passed(&validation))       &&
                            (strcmp(validation.message.data,
                                    "bad 7") == 0) );

    d_arg_validation_release(&validation);
    d_arg_validation_release(NULL);

    return ( (fresh)                                                       &&
             (replaced)                                                    &&
             (kept)                                                        &&
             (rejected)                                                    &&
             (!d_arg_validation_passed(NULL))                              &&
             (d_arg_validation_reject(NULL,
                                      "x") == D_ARG_ERROR_BAD_ARGUMENT)    &&
             (d_arg_validation_replace(NULL,
                                       d_pack_text_from_cstr("x"))
              == D_ARG_ERROR_BAD_ARGUMENT) );
}

//==============================================================================
// 2.  VALIDATORS
//==============================================================================

/*
d_tests_arg_validator_defaults
  CLI11's default Validator.
  Tests the following:
  - active and modifying, applying to every value, unnamed, undescribed,
    and not failed
  - it passes any value, unchanged, and describes nothing
  - releasing it leaves it as d_arg_validator_init makes it
*/
bool
d_tests_arg_validator_defaults(void)
{
    struct d_arg_validator validator = { 0 };

    d_arg_validator_init(&validator);

    const bool made = ( (validator.validate == NULL)                      &&
                        (validator.flags == (D_ARG_VALIDATOR_ACTIVE |
                                             D_ARG_VALIDATOR_MODIFYING))  &&
                        (validator.application_index == -1)               &&
                        (validator.name == NULL)                          &&
                        (validator.description == NULL)                   &&
                        (validator.status == D_ARG_OK) );
    const bool used = d_tests_arg_holds(
                          "default",
                          validator,
                          NUMBERS,
                          3u,
                          "desc <>\n"
                          "[5] -> pass <5>\n"
                          "[0] -> pass <0>\n"
                          "[10] -> pass <10>\n");

    d_arg_validator_release(&validator);
    d_arg_validator_init(NULL);
    d_arg_validator_release(NULL);

    return ( (made)                                       &&
             (used)                                       &&
             (validator.flags == (D_ARG_VALIDATOR_ACTIVE |
                                  D_ARG_VALIDATOR_MODIFYING)) );
}

/*
d_tests_arg_validator_copy
  Copies of validators.
  Tests the following:
  - a copy runs and describes as its source, and owns its own name
  - a copy outlives its source
  - copying NULL, or a validator whose owned context cannot be copied, gives
    a failed validator saying why
*/
bool
d_tests_arg_validator_copy(void)
{
    struct d_arg_validator source = d_tests_arg_validator_int_range(0,
                                                                    10);

    (void)d_arg_validator_set_name(&source,
                                   "bounded");

    struct d_arg_validator copy  = d_arg_validator_copy(&source);
    const bool             named = ( (copy.status == D_ARG_OK)  &&
                                     (copy.name != NULL)        &&
                                     (copy.name != source.name) &&
                                     (strcmp(copy.name,
                                             "bounded") == 0) );

    d_arg_validator_release(&source);

    const bool outlives = d_tests_arg_holds(
                              "copy",
                              copy,
                              NUMBERS,
                              4u,
                              "desc <INT in [0 - 10]>\n"
                              "[5] -> pass <5>\n"
                              "[0] -> pass <0>\n"
                              "[10] -> pass <10>\n"
                              "[11] -> fail <Value 11 not in range "
                              "[0 - 10]>\n");
    struct d_arg_validator none  = d_arg_validator_copy(NULL);
    struct d_arg_validator owner = d_arg_validator_custom(NULL,
                                                          malloc(1u),
                                                          NULL);

    d_arg_validator_own_context(&owner,
                                NULL,
                                free);

    struct d_arg_validator unsupported = d_arg_validator_copy(&owner);
    const bool             refused     =
        ( (none.status == D_ARG_ERROR_BAD_ARGUMENT) &&
          (unsupported.status == D_ARG_ERROR_UNSUPPORTED) );

    d_arg_validator_release(&none);
    d_arg_validator_release(&unsupported);
    d_arg_validator_release(&owner);

    return ( (named)    &&
             (outlives) &&
             (refused) );
}

/*
d_tests_arg_validator_settings
  CLI11's name(), description(), active(), non_modifying() and
application_index().
  Tests the following:
  - a name and a fixed description are copies; the description replaces
    the worked-out one
  - an inactive validator passes every value without running
  - a NULL validator is refused, or ignored
*/
bool
d_tests_arg_validator_settings(void)
{
    struct d_arg_validator validator = d_tests_arg_validator_int_range(0,
                                                                       5);
    char                   name[8]   = "first";

    const bool set = ( (d_arg_validator_set_name(&validator,
                                                 name) == D_ARG_OK)     &&
                       (validator.name != name)                         &&
                       (d_arg_validator_set_description(&validator,
                                                        "FIVE")
                        == D_ARG_OK) );

    d_arg_validator_set_application_index(&validator,
                                          2);
    d_arg_validator_set_modifying(&validator,
                                  false);
    d_arg_validator_set_active(&validator,
                               false);

    const bool flags   = ( (validator.application_index == 2)          &&
                           ((validator.flags &
                             D_ARG_VALIDATOR_MODIFYING) == 0u)        &&
                           ((validator.flags &
                             D_ARG_VALIDATOR_ACTIVE) == 0u) );
    const bool passive = d_tests_arg_holds("inactive",
                                           validator,
                                           NUMBERS + 3u,
                                           1u,
                                           "desc <>\n"
                                           "[11] -> pass <11>\n");

    d_arg_validator_set_active(NULL,
                               true);
    d_arg_validator_set_modifying(NULL,
                                  true);
    d_arg_validator_set_application_index(NULL,
                                          0);

    return ( (set)                                                    &&
             (flags)                                                  &&
             (passive)                                                &&
             (d_arg_validator_set_name(NULL,
                                       "x") == D_ARG_ERROR_BAD_ARGUMENT) );
}

/*
d_tests_arg_validator_choices
  The values a validator lists, for shell completion.
  Tests the following:
  - a validator's own choices, in its order
  - an OR lists both operands', an AND the first operand's that has any
  - a NOT, an inactive validator, and one with no choices list nothing
  - a listing stops at the first status other than D_ARG_OK, and returns it
*/
bool
d_tests_arg_validator_choices(void)
{
    struct d_arg_validator inactive = d_tests_arg_validator_lister("xy");
    struct d_arg_validator stopping = d_tests_arg_validator_lister("abc");

    d_arg_validator_set_active(&inactive,
                               false);

    const int stopped = d_arg_validator_each_choice(
                            &stopping,
                            d_tests_arg_validator_stop_at_b,
                            NULL);

    d_arg_validator_release(&stopping);

    return ( (d_tests_arg_validator_listed(
                  d_tests_arg_validator_lister("ab"),
                  "a|b|"))                                              &&
             (d_tests_arg_validator_listed(
                  d_arg_validator_or(d_tests_arg_validator_lister("ab"),
                                     d_tests_arg_validator_lister("c")),
                  "a|b|c|"))                                            &&
             (d_tests_arg_validator_listed(
                  d_arg_validator_and(d_tests_arg_validator_int_range(0,
                                                                      9),
                                      d_tests_arg_validator_lister("d")),
                  "d|"))                                                &&
             (d_tests_arg_validator_listed(
                  d_arg_validator_not(d_tests_arg_validator_lister("e")),
                  ""))                                                  &&
             (d_tests_arg_validator_listed(inactive,
                                           ""))                         &&
             (d_tests_arg_validator_listed(
                  d_tests_arg_validator_int_range(0,
                                                  9),
                  ""))                                                  &&
             (stopped == D_ARG_ERROR_HORRIBLE) );
}

/*
d_tests_arg_validator_value_kinds
  What shell completion offers for a validator's values.
  Tests the following:
  - a range completes as a number; a directory is also a path
  - an AND completes as a number when either operand does, and else as a
    path when either does
  - an OR completes as a number only when both do, as a path when either
    does, and as a directory when every path operand is one
*/
bool
d_tests_arg_validator_value_kinds(void)
{
    const unsigned int path      = D_ARG_VALIDATOR_PATH;
    const unsigned int directory = D_ARG_VALIDATOR_PATH |
                                   D_ARG_VALIDATOR_DIRECTORY;
    const unsigned int number    = D_ARG_VALIDATOR_NUMBER;

    return ( (d_tests_arg_kind_of(
                  d_tests_arg_validator_int_range(0,
                                                  9)) == number)      &&
             (d_tests_arg_kind_of(
                  d_tests_arg_validator_kinded(
                      D_ARG_VALIDATOR_DIRECTORY)) == directory)       &&
             (d_tests_arg_kind_of(
                  d_arg_validator_and(
                      d_tests_arg_validator_kinded(path),
                      d_tests_arg_validator_kinded(number))) == number) &&
             (d_tests_arg_kind_of(
                  d_arg_validator_or(
                      d_tests_arg_validator_kinded(path),
                      d_tests_arg_validator_kinded(number))) == path)   &&
             (d_tests_arg_kind_of(
                  d_arg_validator_or(
                      d_tests_arg_validator_kinded(directory),
                      d_tests_arg_validator_kinded(directory)))
              == directory)                                             &&
             (d_tests_arg_kind_of(
                  d_arg_validator_or(
                      d_tests_arg_validator_kinded(directory),
                      d_tests_arg_validator_kinded(path))) == path) );
}

//==============================================================================
// 3.  COMPOSITION
//==============================================================================

/*
d_tests_arg_validator_and
  CLI11's Range(0, 10) & Range(5, 20).
  Tests the following:
  - a value passes only when both pass; one rejection is the message, two
    are joined with AND, each in parentheses
  - the description joins both descriptions the same way
*/
bool
d_tests_arg_validator_and(void)
{
    return d_tests_arg_holds(
               "AND",
               d_arg_validator_and(d_tests_arg_validator_int_range(0,
                                                                   10),
                                   d_tests_arg_validator_int_range(5,
                                                                   20)),
               NUMBERS,
               NUMBER_COUNT,
               AND);
}

/*
d_tests_arg_validator_or
  CLI11's Range(0, 5) | Range(10, 15).
  Tests the following:
  - a value passes when either passes, and is otherwise refused with both
    messages joined with OR
*/
bool
d_tests_arg_validator_or(void)
{
    return d_tests_arg_holds(
               "OR",
               d_arg_validator_or(d_tests_arg_validator_int_range(0,
                                                                  5),
                                  d_tests_arg_validator_int_range(10,
                                                                  15)),
               NUMBERS,
               NUMBER_COUNT,
               OR);
}

/*
d_tests_arg_validator_not
  CLI11's !Range(0, 5).
  Tests the following:
  - a value passes when the range refuses it, and is otherwise refused as
    a check that "succeeded improperly"
  - the description is NOT and the range's
*/
bool
d_tests_arg_validator_not(void)
{
    return d_tests_arg_holds(
               "NOT",
               d_arg_validator_not(d_tests_arg_validator_int_range(0,
                                                                   5)),
               NUMBERS,
               NUMBER_COUNT,
               NOT);
}

/*
d_tests_arg_validator_not_or
  CLI11's !(Range(0, 5) | Range(10, 15)).
  Tests the following:
  - an inverted composite, described as CLI11 describes it
*/
bool
d_tests_arg_validator_not_or(void)
{
    return d_tests_arg_holds(
               "NOT_OR",
               d_arg_validator_not(
                   d_arg_validator_or(d_tests_arg_validator_int_range(0,
                                                                      5),
                                      d_tests_arg_validator_int_range(10,
                                                                      15))),
               NUMBERS,
               NUMBER_COUNT,
               NOT_OR);
}

/*
d_tests_arg_validator_escaped_and
  CLI11's EscapedString & Range(0, 10).
  Tests the following:
  - the second operand runs on what the first made of the value: `"5"` is
    unquoted, then in range
  - an operand that describes nothing leaves the other's description alone
*/
bool
d_tests_arg_validator_escaped_and(void)
{
    return d_tests_arg_holds(
               "ESCAPED_AND",
               d_arg_validator_and(d_arg_validator_escaped_string(),
                                   d_tests_arg_validator_int_range(0,
                                                                   10)),
               NUMBERS,
               NUMBER_COUNT,
               ESCAPED_AND);
}

/*
d_tests_arg_validator_inactive
  CLI11's Range(0, 5).active(false).
  Tests the following:
  - every value passes, unchanged, and nothing is described
*/
bool
d_tests_arg_validator_inactive(void)
{
    struct d_arg_validator range = d_tests_arg_validator_int_range(0,
                                                                   5);

    d_arg_validator_set_active(&range,
                               false);

    return d_tests_arg_holds("INACTIVE",
                             range,
                             NUMBERS,
                             NUMBER_COUNT,
                             INACTIVE);
}

/*
d_tests_arg_validator_half_active
  CLI11's Range(0, 5).active(false) & Range(0, 10).
  Tests the following:
  - a composite is inactive when an operand is, so it passes everything,
    and an inactive operand still runs inside an active composite
*/
bool
d_tests_arg_validator_half_active(void)
{
    struct d_arg_validator range = d_tests_arg_validator_int_range(0,
                                                                   5);

    d_arg_validator_set_active(&range,
                               false);

    return d_tests_arg_holds(
               "HALF_ACTIVE",
               d_arg_validator_and(range,
                                   d_tests_arg_validator_int_range(0,
                                                                   10)),
               NUMBERS,
               NUMBER_COUNT,
               HALF_ACTIVE);
}

/*
d_tests_arg_validator_error_stops
  An operand that stops with an error, as a CLI11 validator that throws.
  Tests the following:
  - the composite stops at once, with that status: the second operand does
    not run
*/
bool
d_tests_arg_validator_error_stops(void)
{
    int                     tally      = 0;
    struct d_arg_validator  composite  =
        d_arg_validator_and(
            d_arg_validator_custom(d_tests_arg_validator_stop,
                                   NULL,
                                   NULL),
            d_arg_validator_custom(d_tests_arg_validator_tally,
                                   &tally,
                                   NULL));
    struct d_arg_validation validation =
    {
        { NULL, 0u },
        D_ARG_BUFFER_INIT,
        D_ARG_BUFFER_INIT,
        NULL,
        0
    };

    d_arg_validation_init(&validation,
                          d_pack_text_from_cstr("value"));

    const int status = d_arg_validator_run(&composite,
                                           &validation);

    d_arg_validation_release(&validation);
    d_arg_validator_release(&composite);

    return ( (status == D_ARG_ERROR_FILE) &&
             (tally == 0) );
}

/*
d_tests_arg_validator_failed
  A validator whose building failed.
  Tests the following:
  - a range over no type is a failed validator, holding the bad argument
  - it runs as that error, and composes into a failed validator
*/
bool
d_tests_arg_validator_failed(void)
{
    const int               low        = 0;
    const int               high       = 1;
    struct d_arg_validator  failed     = d_arg_validator_range(NULL,
                                                               &low,
                                                               &high);
    struct d_arg_validator  copy       = d_arg_validator_copy(&failed);
    struct d_arg_validation validation =
    {
        { NULL, 0u },
        D_ARG_BUFFER_INIT,
        D_ARG_BUFFER_INIT,
        NULL,
        0
    };

    d_arg_validation_init(&validation,
                          d_pack_text_from_cstr("0"));

    const int              run       = d_arg_validator_run(&failed,
                                                           &validation);
    struct d_arg_validator composite =
        d_arg_validator_or(copy,
                           d_tests_arg_validator_int_range(0,
                                                           9));
    const bool             spread    =
        (composite.status == D_ARG_ERROR_BAD_ARGUMENT);

    d_arg_validation_release(&validation);
    d_arg_validator_release(&composite);
    d_arg_validator_release(&failed);

    return ( (run == D_ARG_ERROR_BAD_ARGUMENT) &&
             (spread) );
}

//==============================================================================
// 4.  CUSTOM VALIDATORS
//==============================================================================

/*
d_tests_arg_validator_custom
  Validators made from functions of the caller's.
  Tests the following:
  - the function judges and rewrites the value, under the description given
  - made non-modifying, its verdict counts and its rewriting does not
  - a validator with no function passes every value
*/
bool
d_tests_arg_validator_custom(void)
{
    static const char* const WORDS[] =
    {
        "abc", ""
    };
    struct d_arg_validator checked = d_arg_validator_custom(
                                         d_tests_arg_validator_upper,
                                         NULL,
                                         "UPPER");

    d_arg_validator_set_modifying(&checked,
                                  false);

    return ( (d_tests_arg_holds(
                  "upper",
                  d_arg_validator_custom(d_tests_arg_validator_upper,
                                         NULL,
                                         "UPPER"),
                  WORDS,
                  2u,
                  "desc <UPPER>\n"
                  "[abc] -> pass <ABC>\n"
                  "[] -> fail <nothing to raise>\n"))              &&
             (d_tests_arg_holds(
                  "checked",
                  checked,
                  WORDS,
                  2u,
                  "desc <UPPER>\n"
                  "[abc] -> pass <abc>\n"
                  "[] -> fail <nothing to raise>\n"))              &&
             (d_tests_arg_holds(
                  "pass-through",
                  d_arg_validator_custom(NULL,
                                         NULL,
                                         NULL),
                  WORDS,
                  2u,
                  "desc <>\n"
                  "[abc] -> pass <abc>\n"
                  "[] -> pass <>\n")) );
}

/*
d_tests_arg_validator_owned_context
  A context a validator owns.
  Tests the following:
  - copying the validator copies the context, with the caller's function
  - releasing the validator and its copy frees each context once
*/
bool
d_tests_arg_validator_owned_context(void)
{
    struct d_tests_arg_validator_counts         counts  = { 0, 0 };
    struct d_tests_arg_validator_context* const context =
        malloc(sizeof(*context));

    // the context the validator is given
    if (!context)
    {
        return false;
    }

    context->counts = &counts;

    struct d_arg_validator original = d_arg_validator_custom(NULL,
                                                             context,
                                                             NULL);

    d_arg_validator_own_context(&original,
                                d_tests_arg_validator_copy_context,
                                d_tests_arg_validator_release_context);

    struct d_arg_validator copy   = d_arg_validator_copy(&original);
    const bool             copied = ( (copy.status == D_ARG_OK)    &&
                                      (copy.data != original.data) &&
                                      (counts.copies == 1) );

    d_arg_validator_release(&original);
    d_arg_validator_release(&copy);

    return ( (copied) &&
             (counts.releases == 2) );
}

/*
d_tests_arg_validator_described
  CLI11's Range(1, 3, "MYRANGE"): a range with a description of its own.
  Tests the following:
  - the fixed description replaces the range's, and the messages are the
    range's still
*/
bool
d_tests_arg_validator_described(void)
{
    struct d_arg_validator range = d_tests_arg_validator_int_range(1,
                                                                   3);

    (void)d_arg_validator_set_description(&range,
                                          "MYRANGE");

    return d_tests_arg_holds("DESCRIBED",
                             range,
                             NUMBERS,
                             NUMBER_COUNT,
                             DESCRIBED);
}

//==============================================================================
// 5.  DEFAULT VALIDATORS
//==============================================================================

/*
d_tests_arg_validator_range_int
  CLI11's Range(0, 10) over `int`.
  Tests the following:
  - the description, and each value converted as an `int` is -- the empty
    text and fractions out of range -- and refused outside the bounds with
    CLI11's message
*/
bool
d_tests_arg_validator_range_int(void)
{
    return d_tests_arg_holds(
               "RANGE_INT",
               d_tests_arg_validator_int_range(0,
                                               10),
               NUMBERS,
               NUMBER_COUNT,
               RANGE_INT);
}

/*
d_tests_arg_validator_range_uint
  CLI11's Range(0u, 10u) over `unsigned int`.
  Tests the following:
  - a negative value, `-0` included, is out of range for an unsigned type
*/
bool
d_tests_arg_validator_range_uint(void)
{
    const unsigned int low  = 0u;
    const unsigned int high = 10u;

    return d_tests_arg_holds(
               "RANGE_UINT",
               d_arg_validator_range(
                   d_arg_type_builtin(D_ARG_BUILTIN_UNSIGNED_INT),
                   &low,
                   &high),
               NUMBERS,
               NUMBER_COUNT,
               RANGE_UINT);
}

/*
d_tests_arg_validator_range_double
  CLI11's Range(0.5, 2.5) over `double`.
  Tests the following:
  - bounds written to six significant digits; NaN and infinity out of
    range; leading zeros decimal
*/
bool
d_tests_arg_validator_range_double(void)
{
    return d_tests_arg_holds(
               "RANGE_DOUBLE",
               d_arg_validator_range_double(0.5,
                                            2.5),
               NUMBERS,
               NUMBER_COUNT,
               RANGE_DOUBLE);
}

/*
d_tests_arg_validator_range_char
  CLI11's Range('a', 'f') over `char`.
  Tests the following:
  - bounds written as characters; a value one character long is itself, a
    longer one a small integer
*/
bool
d_tests_arg_validator_range_char(void)
{
    const char low  = 'a';
    const char high = 'f';

    return d_tests_arg_holds(
               "RANGE_CHAR",
               d_arg_validator_range(d_arg_type_builtin(D_ARG_BUILTIN_CHAR),
                                     &low,
                                     &high),
               NUMBERS,
               NUMBER_COUNT,
               RANGE_CHAR);
}

/*
d_tests_arg_validator_positive
  CLI11's PositiveNumber.
  Tests the following:
  - described as POSITIVE; zero, `-0` and the negative values refused
*/
bool
d_tests_arg_validator_positive(void)
{
    return d_tests_arg_holds("POSITIVE",
                             d_arg_validator_positive_number(),
                             NUMBERS,
                             NUMBER_COUNT,
                             POSITIVE);
}

/*
d_tests_arg_validator_non_negative
  CLI11's NonNegativeNumber.
  Tests the following:
  - described as NONNEGATIVE; zero and `-0` taken
*/
bool
d_tests_arg_validator_non_negative(void)
{
    return d_tests_arg_holds(
               "NON_NEGATIVE",
               d_arg_validator_non_negative_number(),
               NUMBERS,
               NUMBER_COUNT,
               NON_NEGATIVE);
}

/*
d_tests_arg_validator_escaped
  CLI11's EscapedString.
  Tests the following:
  - quotes of each kind removed, escapes resolved inside double quotes and
    in unquoted text, Unicode escapes written as UTF-8
  - binary strings made of their bytes when CLI11 makes them so
  - CLI11's messages for malformed escapes and invalid code points
*/
bool
d_tests_arg_validator_escaped(void)
{
    return d_tests_arg_holds("ESCAPED",
                             d_arg_validator_escaped_string(),
                             ESCAPES,
                             ESCAPE_COUNT,
                             ESCAPED);
}

/*
d_tests_arg_validator_non_modifying
  CLI11's EscapedString, made non_modifying().
  Tests the following:
  - every value is judged as before and kept as it was given
*/
bool
d_tests_arg_validator_non_modifying(void)
{
    struct d_arg_validator escaped = d_arg_validator_escaped_string();

    d_arg_validator_set_modifying(&escaped,
                                  false);

    return d_tests_arg_holds("NON_MODIFYING",
                             escaped,
                             ESCAPES,
                             ESCAPE_COUNT,
                             NON_MODIFYING);
}

//==============================================================================
// 6.  THE SUITE
//==============================================================================

// d_tests_arg_validator
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_validator[] =
{
    { "arg_validator_validation",
      d_tests_arg_validator_validation },
    { "arg_validator_defaults",
      d_tests_arg_validator_defaults },
    { "arg_validator_copy",
      d_tests_arg_validator_copy },
    { "arg_validator_settings",
      d_tests_arg_validator_settings },
    { "arg_validator_choices",
      d_tests_arg_validator_choices },
    { "arg_validator_value_kinds",
      d_tests_arg_validator_value_kinds },
    { "arg_validator_and",
      d_tests_arg_validator_and },
    { "arg_validator_or",
      d_tests_arg_validator_or },
    { "arg_validator_not",
      d_tests_arg_validator_not },
    { "arg_validator_not_or",
      d_tests_arg_validator_not_or },
    { "arg_validator_escaped_and",
      d_tests_arg_validator_escaped_and },
    { "arg_validator_inactive",
      d_tests_arg_validator_inactive },
    { "arg_validator_half_active",
      d_tests_arg_validator_half_active },
    { "arg_validator_error_stops",
      d_tests_arg_validator_error_stops },
    { "arg_validator_failed",
      d_tests_arg_validator_failed },
    { "arg_validator_custom",
      d_tests_arg_validator_custom },
    { "arg_validator_owned_context",
      d_tests_arg_validator_owned_context },
    { "arg_validator_described",
      d_tests_arg_validator_described },
    { "arg_validator_range_int",
      d_tests_arg_validator_range_int },
    { "arg_validator_range_uint",
      d_tests_arg_validator_range_uint },
    { "arg_validator_range_double",
      d_tests_arg_validator_range_double },
    { "arg_validator_range_char",
      d_tests_arg_validator_range_char },
    { "arg_validator_positive",
      d_tests_arg_validator_positive },
    { "arg_validator_non_negative",
      d_tests_arg_validator_non_negative },
    { "arg_validator_escaped",
      d_tests_arg_validator_escaped },
    { "arg_validator_non_modifying",
      d_tests_arg_validator_non_modifying }
};

// d_tests_arg_validator_count
//   constant: how many sections the table above holds in this build.
const size_t d_tests_arg_validator_count =
    sizeof(d_tests_arg_validator) / sizeof(d_tests_arg_validator[0]);
