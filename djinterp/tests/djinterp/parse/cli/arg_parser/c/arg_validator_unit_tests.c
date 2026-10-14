/*******************************************************************************
* djinterp [parse]                                    arg_validator_unit_tests.c
*
* The suite of CLI11's numbers with units, declared in
* arg_validator_unit_tests.h.
*   The transcripts are CLI11 2.7.2's, written as the core suite writes them
* (arg_tests_support.h): each validator run over the values CLI11's was run
* over, under CLI11's rule for leading zeros. Every refusal of these two is
* a throw in CLI11, so a refused value's line says `status`, with CLI11's
* message. Two transcripts depart from CLI11, and their constants say why:
* a `float` product, which CLI11 writes to every digit the type holds, and
* the results a size that fails leaves, which CLI11 cuts short. Which units
* cannot be made is CLI11's where CLI11 refuses them, and this module's
* where C can pass what C++ cannot.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_validator_unit_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./arg_validator_unit_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <string.h>   // strcmp
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knob
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // descriptors
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // running, copies, composites
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_extra.h"  // the unit validators
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // transcripts, parses, kinds
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// UNITS
//   constant: the values the numbers with units are run over, as CLI11's
// were: numbers with and without units, units alone, spaces around and
// between, every spelling of a number the types read, overflow, and units
// no validator knows.
static const char* const UNITS[] =
{
    "10", "10 kb", "10KB", "5 mb", "kb", "  7  ", "", "   ", "abc", "10x",
    "10 xb", "-2 kb", "1.5 kb", "2000000 mb", "0x10 kb", "+3kb", "1e3 kb",
    "10 k b", "7 b", "3MB", "08 kb", "010", " 12 KB ", "1.5 min", "250ms",
    "2", "1 s", "3 S", "1e308 min", "-1e308 min", "inf min", "0 min",
    "x min", "4 hours"
};

// UNIT_COUNT
//   constant: how many values UNITS holds.
static const size_t UNIT_COUNT = sizeof(UNITS) / sizeof(UNITS[0]);

// SIZES
//   constant: the values the sizes are run over, as CLI11's were: every
// prefix in both cases, with and without `b`, `i` and `ib`, the largest
// 64-bit size and one past it, and units no size has.
static const char* const SIZES[] =
{
    "1", "1b", "1kb", "1KB", "1kib", "1 KiB", "2 MB", "2 Mi", "1eib",
    "16 eib", "15 eib", "1xb", "kb", "", "-1", "1.5k", "0x10k", "1 k",
    "3 GiB", "1 tb", "2 PB", "5 E", "1 bb", "1 ki b", "18446744073709551615",
    "18446744073709551616", "1 B", "17 EB", "16 EB", "2 Eb", "  8 kB  ",
    "9 i", "1ib"
};

// SIZE_COUNT
//   constant: how many values SIZES holds.
static const size_t SIZE_COUNT = sizeof(SIZES) / sizeof(SIZES[0]);

// CASES
//   constant: the values the units that differ only in case are run over,
// as CLI11's were.
static const char* const CASES[] =
{
    "1 kb", "1 KB", "1 Kb", "kb", "KB", "1"
};

// CASE_COUNT
//   constant: how many values CASES holds.
static const size_t CASE_COUNT = sizeof(CASES) / sizeof(CASES[0]);

// TWICE
//   constant: the values a unit given twice is run over: each unit, one
// alone, one no unit is, one in another case, and a number alone.
static const char* const TWICE[] =
{
    "1 z", "2 b", "z", "1 q", "3Z", "4"
};

// TWICE_COUNT
//   constant: how many values TWICE holds.
static const size_t TWICE_COUNT = sizeof(TWICE) / sizeof(TWICE[0]);

// FLOAT_UNITS
//   constant: the values the `float` unit that departs from CLI11 is run
// over.
static const char* const FLOAT_UNITS[] =
{
    "0.1 x", "1 x", "x"
};

// BYTES
//   constant: CLI11's std::map<std::string, int> {{"b", 1}, {"kb", 1024},
// {"MB", 1048576}}.
static const struct d_arg_mapping BYTES[] =
{
    { "b",  "1"       },
    { "kb", "1024"    },
    { "MB", "1048576" }
};

// TIMES
//   constant: CLI11's std::map<std::string, double> {{"s", 1.0},
// {"ms", 0.5}, {"min", 60.0}}.
static const struct d_arg_mapping TIMES[] =
{
    { "s",   "1"   },
    { "ms",  "0.5" },
    { "min", "60"  }
};

// SCALES
//   constant: CLI11's std::map<std::string, unsigned> {{"x", 3u},
// {"k", 1000u}}, given out of the map's order.
static const struct d_arg_mapping SCALES[] =
{
    { "x", "3"    },
    { "k", "1000" }
};

// SHORT_SCALES
//   constant: CLI11's std::map<std::string, short> {{"k", 1000},
// {"h", 100}, {"min", -1}}.
static const struct d_arg_mapping SHORT_SCALES[] =
{
    { "k",   "1000" },
    { "h",   "100"  },
    { "min", "-1"   }
};

// WIDE_SCALES
//   constant: CLI11's std::map<std::string, int64_t> {{"kb", 1024},
// {"x", -2}, {"eb", 1152921504606846976}}.
static const struct d_arg_mapping WIDE_SCALES[] =
{
    { "kb", "1024"                },
    { "x",  "-2"                  },
    { "eb", "1152921504606846976" }
};

// CASED
//   constant: CLI11's std::map<std::string, int> {{"kb", 1}, {"KB", 2}}:
// two units that differ only in case.
static const struct d_arg_mapping CASED[] =
{
    { "kb", "1" },
    { "KB", "2" }
};

// U_INT
//   constant: CLI11's AsNumberWithUnit of BYTES, DEFAULT.
static const char U_INT[] =
    "desc <INT [UNIT]>\n"
    "[10] -> pass <10>\n"
    "[10 kb] -> pass <10240>\n"
    "[10KB] -> pass <10240>\n"
    "[5 mb] -> pass <5242880>\n"
    "[kb] -> pass <1024>\n"
    "[  7  ] -> pass <7>\n"
    "[] -> status <Input is empty>\n"
    "[   ] -> status <Input is empty>\n"
    "[abc] -> status <abc unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[10x] -> status <x unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[10 xb] -> status <xb unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[-2 kb] -> pass <-2048>\n"
    "[1.5 kb] -> status <Value 1.5 could not be converted to INT>\n"
    "[2000000 mb] -> status <2000000 multiplied by mb factor would cause "
    "number overflow. Use smaller value.>\n"
    "[0x10 kb] -> pass <16384>\n"
    "[+3kb] -> pass <3072>\n"
    "[1e3 kb] -> status <Value 1e3 could not be converted to INT>\n"
    "[10 k b] -> status <Value 10 k could not be converted to INT>\n"
    "[7 b] -> pass <7>\n"
    "[3MB] -> pass <3145728>\n"
    "[08 kb] -> status <Value 08 could not be converted to INT>\n"
    "[010] -> pass <010>\n"
    "[ 12 KB ] -> pass <12288>\n"
    "[1.5 min] -> status <min unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[250ms] -> status <ms unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[2] -> pass <2>\n"
    "[1 s] -> status <s unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[3 S] -> status <s unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[1e308 min] -> status <min unit not recognized. Allowed values: "
    "{b,kb,mb}>\n"
    "[-1e308 min] -> status <min unit not recognized. Allowed values: "
    "{b,kb,mb}>\n"
    "[inf min] -> status <min unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[0 min] -> status <min unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[x min] -> status <min unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[4 hours] -> status <hours unit not recognized. Allowed values: "
    "{b,kb,mb}>\n";

// U_REQ
//   constant: CLI11's AsNumberWithUnit of BYTES, CASE_SENSITIVE |
// UNIT_REQUIRED, named "SIZE".
static const char U_REQ[] =
    "desc <INT SIZE>\n"
    "[10] -> status <Missing mandatory unit>\n"
    "[10 kb] -> pass <10240>\n"
    "[10KB] -> status <KB unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[5 mb] -> status <mb unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[kb] -> pass <1024>\n"
    "[  7  ] -> status <Missing mandatory unit>\n"
    "[] -> status <Input is empty>\n"
    "[   ] -> status <Input is empty>\n"
    "[abc] -> status <abc unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[10x] -> status <x unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[10 xb] -> status <xb unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[-2 kb] -> pass <-2048>\n"
    "[1.5 kb] -> status <Value 1.5 could not be converted to INT>\n"
    "[2000000 mb] -> status <mb unit not recognized. Allowed values: "
    "{MB,b,kb}>\n"
    "[0x10 kb] -> pass <16384>\n"
    "[+3kb] -> pass <3072>\n"
    "[1e3 kb] -> status <Value 1e3 could not be converted to INT>\n"
    "[10 k b] -> status <Value 10 k could not be converted to INT>\n"
    "[7 b] -> pass <7>\n"
    "[3MB] -> pass <3145728>\n"
    "[08 kb] -> status <Value 08 could not be converted to INT>\n"
    "[010] -> status <Missing mandatory unit>\n"
    "[ 12 KB ] -> status <KB unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[1.5 min] -> status <min unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[250ms] -> status <ms unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[2] -> status <Missing mandatory unit>\n"
    "[1 s] -> status <s unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[3 S] -> status <S unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[1e308 min] -> status <min unit not recognized. Allowed values: "
    "{MB,b,kb}>\n"
    "[-1e308 min] -> status <min unit not recognized. Allowed values: "
    "{MB,b,kb}>\n"
    "[inf min] -> status <min unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[0 min] -> status <min unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[x min] -> status <min unit not recognized. Allowed values: {MB,b,kb}>\n"
    "[4 hours] -> status <hours unit not recognized. Allowed values: "
    "{MB,b,kb}>\n";

// U_UINT
//   constant: CLI11's AsNumberWithUnit of SCALES, CASE_INSENSITIVE, named
// "N".
static const char U_UINT[] =
    "desc <UINT [N]>\n"
    "[10] -> pass <10>\n"
    "[10 kb] -> status <kb unit not recognized. Allowed values: {k,x}>\n"
    "[10KB] -> status <kb unit not recognized. Allowed values: {k,x}>\n"
    "[5 mb] -> status <mb unit not recognized. Allowed values: {k,x}>\n"
    "[kb] -> status <kb unit not recognized. Allowed values: {k,x}>\n"
    "[  7  ] -> pass <7>\n"
    "[] -> status <Input is empty>\n"
    "[   ] -> status <Input is empty>\n"
    "[abc] -> status <abc unit not recognized. Allowed values: {k,x}>\n"
    "[10x] -> pass <30>\n"
    "[10 xb] -> status <xb unit not recognized. Allowed values: {k,x}>\n"
    "[-2 kb] -> status <kb unit not recognized. Allowed values: {k,x}>\n"
    "[1.5 kb] -> status <kb unit not recognized. Allowed values: {k,x}>\n"
    "[2000000 mb] -> status <mb unit not recognized. Allowed values: {k,x}>\n"
    "[0x10 kb] -> status <kb unit not recognized. Allowed values: {k,x}>\n"
    "[+3kb] -> status <kb unit not recognized. Allowed values: {k,x}>\n"
    "[1e3 kb] -> status <kb unit not recognized. Allowed values: {k,x}>\n"
    "[10 k b] -> status <b unit not recognized. Allowed values: {k,x}>\n"
    "[7 b] -> status <b unit not recognized. Allowed values: {k,x}>\n"
    "[3MB] -> status <mb unit not recognized. Allowed values: {k,x}>\n"
    "[08 kb] -> status <kb unit not recognized. Allowed values: {k,x}>\n"
    "[010] -> pass <010>\n"
    "[ 12 KB ] -> status <kb unit not recognized. Allowed values: {k,x}>\n"
    "[1.5 min] -> status <min unit not recognized. Allowed values: {k,x}>\n"
    "[250ms] -> status <ms unit not recognized. Allowed values: {k,x}>\n"
    "[2] -> pass <2>\n"
    "[1 s] -> status <s unit not recognized. Allowed values: {k,x}>\n"
    "[3 S] -> status <s unit not recognized. Allowed values: {k,x}>\n"
    "[1e308 min] -> status <min unit not recognized. Allowed values: {k,x}>\n"
    "[-1e308 min] -> status <min unit not recognized. Allowed values: {k,x}>\n"
    "[inf min] -> status <min unit not recognized. Allowed values: {k,x}>\n"
    "[0 min] -> status <min unit not recognized. Allowed values: {k,x}>\n"
    "[x min] -> status <min unit not recognized. Allowed values: {k,x}>\n"
    "[4 hours] -> status <hours unit not recognized. Allowed values: {k,x}>\n";

// U_DBL
//   constant: CLI11's AsNumberWithUnit of TIMES, DEFAULT.
static const char U_DBL[] =
    "desc <FLOAT [UNIT]>\n"
    "[10] -> pass <10>\n"
    "[10 kb] -> status <kb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[10KB] -> status <kb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[5 mb] -> status <mb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[kb] -> status <kb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[  7  ] -> pass <7>\n"
    "[] -> status <Input is empty>\n"
    "[   ] -> status <Input is empty>\n"
    "[abc] -> status <abc unit not recognized. Allowed values: {min,ms,s}>\n"
    "[10x] -> status <x unit not recognized. Allowed values: {min,ms,s}>\n"
    "[10 xb] -> status <xb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[-2 kb] -> status <kb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[1.5 kb] -> status <kb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[2000000 mb] -> status <mb unit not recognized. Allowed values: "
    "{min,ms,s}>\n"
    "[0x10 kb] -> status <kb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[+3kb] -> status <kb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[1e3 kb] -> status <kb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[10 k b] -> status <b unit not recognized. Allowed values: {min,ms,s}>\n"
    "[7 b] -> status <b unit not recognized. Allowed values: {min,ms,s}>\n"
    "[3MB] -> status <mb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[08 kb] -> status <kb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[010] -> pass <010>\n"
    "[ 12 KB ] -> status <kb unit not recognized. Allowed values: {min,ms,s}>\n"
    "[1.5 min] -> pass <90>\n"
    "[250ms] -> pass <125>\n"
    "[2] -> pass <2>\n"
    "[1 s] -> pass <1>\n"
    "[3 S] -> pass <3>\n"
    "[1e308 min] -> status <1e+308 multiplied by min factor would cause number "
    "overflow. Use smaller value.>\n"
    "[-1e308 min] -> status <-1e+308 multiplied by min factor would cause "
    "number overflow. Use smaller value.>\n"
    "[inf min] -> pass <inf>\n"
    "[0 min] -> pass <0>\n"
    "[x min] -> status <Value x could not be converted to FLOAT>\n"
    "[4 hours] -> status <hours unit not recognized. Allowed values: "
    "{min,ms,s}>\n";

// U_SHORT
//   constant: CLI11's AsNumberWithUnit of SHORT_SCALES, DEFAULT.
static const char U_SHORT[] =
    "desc <INT [UNIT]>\n"
    "[10] -> pass <10>\n"
    "[10 kb] -> status <kb unit not recognized. Allowed values: {h,k,min}>\n"
    "[10KB] -> status <kb unit not recognized. Allowed values: {h,k,min}>\n"
    "[5 mb] -> status <mb unit not recognized. Allowed values: {h,k,min}>\n"
    "[kb] -> status <kb unit not recognized. Allowed values: {h,k,min}>\n"
    "[  7  ] -> pass <7>\n"
    "[] -> status <Input is empty>\n"
    "[   ] -> status <Input is empty>\n"
    "[abc] -> status <abc unit not recognized. Allowed values: {h,k,min}>\n"
    "[10x] -> status <x unit not recognized. Allowed values: {h,k,min}>\n"
    "[10 xb] -> status <xb unit not recognized. Allowed values: {h,k,min}>\n"
    "[-2 kb] -> status <kb unit not recognized. Allowed values: {h,k,min}>\n"
    "[1.5 kb] -> status <kb unit not recognized. Allowed values: {h,k,min}>\n"
    "[2000000 mb] -> status <mb unit not recognized. Allowed values: "
    "{h,k,min}>\n"
    "[0x10 kb] -> status <kb unit not recognized. Allowed values: {h,k,min}>\n"
    "[+3kb] -> status <kb unit not recognized. Allowed values: {h,k,min}>\n"
    "[1e3 kb] -> status <kb unit not recognized. Allowed values: {h,k,min}>\n"
    "[10 k b] -> status <b unit not recognized. Allowed values: {h,k,min}>\n"
    "[7 b] -> status <b unit not recognized. Allowed values: {h,k,min}>\n"
    "[3MB] -> status <mb unit not recognized. Allowed values: {h,k,min}>\n"
    "[08 kb] -> status <kb unit not recognized. Allowed values: {h,k,min}>\n"
    "[010] -> pass <010>\n"
    "[ 12 KB ] -> status <kb unit not recognized. Allowed values: {h,k,min}>\n"
    "[1.5 min] -> status <Value 1.5 could not be converted to INT>\n"
    "[250ms] -> status <ms unit not recognized. Allowed values: {h,k,min}>\n"
    "[2] -> pass <2>\n"
    "[1 s] -> status <s unit not recognized. Allowed values: {h,k,min}>\n"
    "[3 S] -> status <s unit not recognized. Allowed values: {h,k,min}>\n"
    "[1e308 min] -> status <Value 1e308 could not be converted to INT>\n"
    "[-1e308 min] -> status <Value -1e308 could not be converted to INT>\n"
    "[inf min] -> status <Value inf could not be converted to INT>\n"
    "[0 min] -> pass <0>\n"
    "[x min] -> status <Value x could not be converted to INT>\n"
    "[4 hours] -> status <hours unit not recognized. Allowed values: "
    "{h,k,min}>\n";

// U_I64
//   constant: CLI11's AsNumberWithUnit of WIDE_SCALES, DEFAULT.
static const char U_I64[] =
    "desc <INT [UNIT]>\n"
    "[10] -> pass <10>\n"
    "[10 kb] -> pass <10240>\n"
    "[10KB] -> pass <10240>\n"
    "[5 mb] -> status <mb unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[kb] -> pass <1024>\n"
    "[  7  ] -> pass <7>\n"
    "[] -> status <Input is empty>\n"
    "[   ] -> status <Input is empty>\n"
    "[abc] -> status <abc unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[10x] -> pass <-20>\n"
    "[10 xb] -> status <xb unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[-2 kb] -> pass <-2048>\n"
    "[1.5 kb] -> status <Value 1.5 could not be converted to INT>\n"
    "[2000000 mb] -> status <mb unit not recognized. Allowed values: "
    "{eb,kb,x}>\n"
    "[0x10 kb] -> pass <16384>\n"
    "[+3kb] -> pass <3072>\n"
    "[1e3 kb] -> status <Value 1e3 could not be converted to INT>\n"
    "[10 k b] -> status <b unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[7 b] -> status <b unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[3MB] -> status <mb unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[08 kb] -> status <Value 08 could not be converted to INT>\n"
    "[010] -> pass <010>\n"
    "[ 12 KB ] -> pass <12288>\n"
    "[1.5 min] -> status <min unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[250ms] -> status <ms unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[2] -> pass <2>\n"
    "[1 s] -> status <s unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[3 S] -> status <s unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[1e308 min] -> status <min unit not recognized. Allowed values: "
    "{eb,kb,x}>\n"
    "[-1e308 min] -> status <min unit not recognized. Allowed values: "
    "{eb,kb,x}>\n"
    "[inf min] -> status <min unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[0 min] -> status <min unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[x min] -> status <min unit not recognized. Allowed values: {eb,kb,x}>\n"
    "[4 hours] -> status <hours unit not recognized. Allowed values: "
    "{eb,kb,x}>\n";

// U_CS
//   constant: CLI11's AsNumberWithUnit of CASED, CASE_SENSITIVE, run over
// CASES.
static const char U_CS[] =
    "desc <INT [UNIT]>\n"
    "[1 kb] -> pass <1>\n"
    "[1 KB] -> pass <2>\n"
    "[1 Kb] -> status <Kb unit not recognized. Allowed values: {KB,kb}>\n"
    "[kb] -> pass <1>\n"
    "[KB] -> pass <2>\n"
    "[1] -> pass <1>\n";

// U_NONE
//   constant: CLI11's AsNumberWithUnit of an empty std::map<std::string,
// int>, DEFAULT.
static const char U_NONE[] =
    "desc <INT [UNIT]>\n"
    "[10] -> pass <10>\n"
    "[10 kb] -> status <kb unit not recognized. Allowed values: {}>\n"
    "[10KB] -> status <kb unit not recognized. Allowed values: {}>\n"
    "[5 mb] -> status <mb unit not recognized. Allowed values: {}>\n"
    "[kb] -> status <kb unit not recognized. Allowed values: {}>\n"
    "[  7  ] -> pass <7>\n"
    "[] -> status <Input is empty>\n"
    "[   ] -> status <Input is empty>\n"
    "[abc] -> status <abc unit not recognized. Allowed values: {}>\n"
    "[10x] -> status <x unit not recognized. Allowed values: {}>\n"
    "[10 xb] -> status <xb unit not recognized. Allowed values: {}>\n"
    "[-2 kb] -> status <kb unit not recognized. Allowed values: {}>\n"
    "[1.5 kb] -> status <kb unit not recognized. Allowed values: {}>\n"
    "[2000000 mb] -> status <mb unit not recognized. Allowed values: {}>\n"
    "[0x10 kb] -> status <kb unit not recognized. Allowed values: {}>\n"
    "[+3kb] -> status <kb unit not recognized. Allowed values: {}>\n"
    "[1e3 kb] -> status <kb unit not recognized. Allowed values: {}>\n"
    "[10 k b] -> status <b unit not recognized. Allowed values: {}>\n"
    "[7 b] -> status <b unit not recognized. Allowed values: {}>\n"
    "[3MB] -> status <mb unit not recognized. Allowed values: {}>\n"
    "[08 kb] -> status <kb unit not recognized. Allowed values: {}>\n"
    "[010] -> pass <010>\n"
    "[ 12 KB ] -> status <kb unit not recognized. Allowed values: {}>\n"
    "[1.5 min] -> status <min unit not recognized. Allowed values: {}>\n"
    "[250ms] -> status <ms unit not recognized. Allowed values: {}>\n"
    "[2] -> pass <2>\n"
    "[1 s] -> status <s unit not recognized. Allowed values: {}>\n"
    "[3 S] -> status <s unit not recognized. Allowed values: {}>\n"
    "[1e308 min] -> status <min unit not recognized. Allowed values: {}>\n"
    "[-1e308 min] -> status <min unit not recognized. Allowed values: {}>\n"
    "[inf min] -> status <min unit not recognized. Allowed values: {}>\n"
    "[0 min] -> status <min unit not recognized. Allowed values: {}>\n"
    "[x min] -> status <min unit not recognized. Allowed values: {}>\n"
    "[4 hours] -> status <hours unit not recognized. Allowed values: {}>\n";

// U_TWICE
//   constant: CLI11's AsNumberWithUnit of std::map<std::string, int>
// {{"z", 1}, {"b", 2}, {"z", 3}}, DEFAULT, run over TWICE: the map keeps
// the first `z`.
static const char U_TWICE[] =
    "desc <INT [UNIT]>\n"
    "[1 z] -> pass <1>\n"
    "[2 b] -> pass <4>\n"
    "[z] -> pass <1>\n"
    "[1 q] -> status <q unit not recognized. Allowed values: {b,z}>\n"
    "[3Z] -> pass <3>\n"
    "[4] -> pass <4>\n";

// U_KB
//   constant: CLI11's AsNumberWithUnit of std::map<std::string, int>
// {{"Kb", 1}}, UNIT_REQUIRED, run over CASES.
static const char U_KB[] =
    "desc <INT UNIT>\n"
    "[1 kb] -> status <kb unit not recognized. Allowed values: {Kb}>\n"
    "[1 KB] -> status <KB unit not recognized. Allowed values: {Kb}>\n"
    "[1 Kb] -> pass <1>\n"
    "[kb] -> status <kb unit not recognized. Allowed values: {Kb}>\n"
    "[KB] -> status <KB unit not recognized. Allowed values: {Kb}>\n"
    "[1] -> status <Missing mandatory unit>\n";

// S_1000
//   constant: CLI11's AsSizeValue(true).
static const char S_1000[] =
    "desc <SIZE [b, kb(=1000b), kib(=1024b), ...]>\n"
    "[1] -> pass <1>\n"
    "[1b] -> pass <1>\n"
    "[1kb] -> pass <1000>\n"
    "[1KB] -> pass <1000>\n"
    "[1kib] -> pass <1024>\n"
    "[1 KiB] -> pass <1024>\n"
    "[2 MB] -> pass <2000000>\n"
    "[2 Mi] -> pass <2097152>\n"
    "[1eib] -> pass <1152921504606846976>\n"
    "[16 eib] -> status <16 multiplied by eib factor would cause number "
    "overflow. Use smaller value.>\n"
    "[15 eib] -> pass <17293822569102704640>\n"
    "[1xb] -> status <xb unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n"
    "[kb] -> pass <1000>\n"
    "[] -> status <Input is empty>\n"
    "[-1] -> status <Value -1 could not be converted to UINT>\n"
    "[1.5k] -> status <Value 1.5 could not be converted to UINT>\n"
    "[0x10k] -> pass <16000>\n"
    "[1 k] -> pass <1000>\n"
    "[3 GiB] -> pass <3221225472>\n"
    "[1 tb] -> pass <1000000000000>\n"
    "[2 PB] -> pass <2000000000000000>\n"
    "[5 E] -> pass <5000000000000000000>\n"
    "[1 bb] -> status <bb unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n"
    "[1 ki b] -> status <Value 1 ki could not be converted to UINT>\n"
    "[18446744073709551615] -> pass <18446744073709551615>\n"
    "[18446744073709551616] -> status <Value 18446744073709551616 could not be "
    "converted to UINT>\n"
    "[1 B] -> pass <1>\n"
    "[17 EB] -> pass <17000000000000000000>\n"
    "[16 EB] -> pass <16000000000000000000>\n"
    "[2 Eb] -> pass <2000000000000000000>\n"
    "[  8 kB  ] -> pass <8000>\n"
    "[9 i] -> status <i unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n"
    "[1ib] -> status <ib unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n";

// S_1024
//   constant: CLI11's AsSizeValue(false).
static const char S_1024[] =
    "desc <SIZE [b, kb(=1024b), ...]>\n"
    "[1] -> pass <1>\n"
    "[1b] -> pass <1>\n"
    "[1kb] -> pass <1024>\n"
    "[1KB] -> pass <1024>\n"
    "[1kib] -> pass <1024>\n"
    "[1 KiB] -> pass <1024>\n"
    "[2 MB] -> pass <2097152>\n"
    "[2 Mi] -> pass <2097152>\n"
    "[1eib] -> pass <1152921504606846976>\n"
    "[16 eib] -> status <16 multiplied by eib factor would cause number "
    "overflow. Use smaller value.>\n"
    "[15 eib] -> pass <17293822569102704640>\n"
    "[1xb] -> status <xb unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n"
    "[kb] -> pass <1024>\n"
    "[] -> status <Input is empty>\n"
    "[-1] -> status <Value -1 could not be converted to UINT>\n"
    "[1.5k] -> status <Value 1.5 could not be converted to UINT>\n"
    "[0x10k] -> pass <16384>\n"
    "[1 k] -> pass <1024>\n"
    "[3 GiB] -> pass <3221225472>\n"
    "[1 tb] -> pass <1099511627776>\n"
    "[2 PB] -> pass <2251799813685248>\n"
    "[5 E] -> pass <5764607523034234880>\n"
    "[1 bb] -> status <bb unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n"
    "[1 ki b] -> status <Value 1 ki could not be converted to UINT>\n"
    "[18446744073709551615] -> pass <18446744073709551615>\n"
    "[18446744073709551616] -> status <Value 18446744073709551616 could not be "
    "converted to UINT>\n"
    "[1 B] -> pass <1>\n"
    "[17 EB] -> status <17 multiplied by eb factor would cause number "
    "overflow. Use smaller value.>\n"
    "[16 EB] -> status <16 multiplied by eb factor would cause number "
    "overflow. Use smaller value.>\n"
    "[2 Eb] -> pass <2305843009213693952>\n"
    "[  8 kB  ] -> pass <8192>\n"
    "[9 i] -> status <i unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n"
    "[1ib] -> status <ib unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n";

// K_OR
//   constant: CLI11's AsSizeValue(false) | Range(0, 5), run over SIZES.
static const char K_OR[] =
    "desc <(SIZE [b, kb(=1024b), ...]) OR (INT in [0 - 5])>\n"
    "[1] -> pass <1>\n"
    "[1b] -> pass <1>\n"
    "[1kb] -> pass <1024>\n"
    "[1KB] -> pass <1024>\n"
    "[1kib] -> pass <1024>\n"
    "[1 KiB] -> pass <1024>\n"
    "[2 MB] -> pass <2097152>\n"
    "[2 Mi] -> pass <2097152>\n"
    "[1eib] -> pass <1152921504606846976>\n"
    "[16 eib] -> status <16 multiplied by eib factor would cause number "
    "overflow. Use smaller value.>\n"
    "[15 eib] -> pass <17293822569102704640>\n"
    "[1xb] -> status <xb unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n"
    "[kb] -> pass <1024>\n"
    "[] -> status <Input is empty>\n"
    "[-1] -> status <Value -1 could not be converted to UINT>\n"
    "[1.5k] -> status <Value 1.5 could not be converted to UINT>\n"
    "[0x10k] -> pass <16384>\n"
    "[1 k] -> pass <1024>\n"
    "[3 GiB] -> pass <3221225472>\n"
    "[1 tb] -> pass <1099511627776>\n"
    "[2 PB] -> pass <2251799813685248>\n"
    "[5 E] -> pass <5764607523034234880>\n"
    "[1 bb] -> status <bb unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n"
    "[1 ki b] -> status <Value 1 ki could not be converted to UINT>\n"
    "[18446744073709551615] -> pass <18446744073709551615>\n"
    "[18446744073709551616] -> status <Value 18446744073709551616 could not be "
    "converted to UINT>\n"
    "[1 B] -> pass <1>\n"
    "[17 EB] -> status <17 multiplied by eb factor would cause number "
    "overflow. Use smaller value.>\n"
    "[16 EB] -> status <16 multiplied by eb factor would cause number "
    "overflow. Use smaller value.>\n"
    "[2 Eb] -> pass <2305843009213693952>\n"
    "[  8 kB  ] -> pass <8192>\n"
    "[9 i] -> status <i unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n"
    "[1ib] -> status <ib unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n";

// K_UNIT_OR
//   constant: CLI11's AsNumberWithUnit of BYTES | IsMember({"alpha",
// "Beta", "gamma_ray"}), run over UNITS.
static const char K_UNIT_OR[] =
    "desc <(INT [UNIT]) OR ({alpha,Beta,gamma_ray})>\n"
    "[10] -> pass <10>\n"
    "[10 kb] -> pass <10240>\n"
    "[10KB] -> pass <10240>\n"
    "[5 mb] -> pass <5242880>\n"
    "[kb] -> pass <1024>\n"
    "[  7  ] -> pass <7>\n"
    "[] -> status <Input is empty>\n"
    "[   ] -> status <Input is empty>\n"
    "[abc] -> status <abc unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[10x] -> status <x unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[10 xb] -> status <xb unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[-2 kb] -> pass <-2048>\n"
    "[1.5 kb] -> status <Value 1.5 could not be converted to INT>\n"
    "[2000000 mb] -> status <2000000 multiplied by mb factor would cause "
    "number overflow. Use smaller value.>\n"
    "[0x10 kb] -> pass <16384>\n"
    "[+3kb] -> pass <3072>\n"
    "[1e3 kb] -> status <Value 1e3 could not be converted to INT>\n"
    "[10 k b] -> status <Value 10 k could not be converted to INT>\n"
    "[7 b] -> pass <7>\n"
    "[3MB] -> pass <3145728>\n"
    "[08 kb] -> status <Value 08 could not be converted to INT>\n"
    "[010] -> pass <010>\n"
    "[ 12 KB ] -> pass <12288>\n"
    "[1.5 min] -> status <min unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[250ms] -> status <ms unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[2] -> pass <2>\n"
    "[1 s] -> status <s unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[3 S] -> status <s unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[1e308 min] -> status <min unit not recognized. Allowed values: "
    "{b,kb,mb}>\n"
    "[-1e308 min] -> status <min unit not recognized. Allowed values: "
    "{b,kb,mb}>\n"
    "[inf min] -> status <min unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[0 min] -> status <min unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[x min] -> status <min unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[4 hours] -> status <hours unit not recognized. Allowed values: "
    "{b,kb,mb}>\n";

// D_UFLT
//   constant: CLI11's AsNumberWithUnit of std::map<std::string, float>
// {{"x", 3.0f}}, run over FLOAT_UNITS. The product of `0.1 x` is written
// as the `float` writes values, the shortest text that reads back as the
// same number, `0.3`, where CLI11 writes every digit a `float` holds,
// `0.300000012`. The two texts read back as the same `float`.
static const char D_UFLT[] =
    "desc <FLOAT [UNIT]>\n"
    "[0.1 x] -> pass <0.3>\n"
    "[1 x] -> pass <3>\n"
    "[x] -> pass <3>\n";

// P_SIZE
//   constant: CLI11's `--size 10kb`, the option bound to text and
// transformed by AsSizeValue(false).
static const char P_SIZE[] =
    "cmd app n=1 miss=[] subs=[] order=[--size]\n"
    "opt --size n=1 res=[10240]\n"
    "value [10240]\n";

// P_SIZE_EMPTY
//   constant: CLI11's `--size ""`, to the same option.
static const char P_SIZE_EMPTY[] =
    "ERR ValidationError 105: --size: Input is empty\n"
    "cmd app n=1 miss=[] subs=[] order=[--size]\n"
    "opt --size n=1 res=[]\n"
    "value []\n";

// P_SIZE_UNKNOWN
//   constant: CLI11's `--size 1zb`, to the same option. The parse fails as
// CLI11's does, with its message; the option's results keep the value as
// it was given, `1zb`, where CLI11 leaves `1`: its validator cuts the unit
// off the value before it looks the unit up, and the cut stays when the
// lookup fails. Here a validator changes a value only when it passes it.
static const char P_SIZE_UNKNOWN[] =
    "ERR ValidationError 105: --size: zb unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}\n"
    "cmd app n=1 miss=[] subs=[] order=[--size]\n"
    "opt --size n=1 res=[1zb]\n"
    "value []\n";

// SIZE_OPTION
//   constant: CLI11's `add_option("--size", text)`, transformed.
static const struct d_tests_arg_checked SIZE_OPTION =
{
    "--size",
    D_ARG_BUILTIN_STRING,
    true
};

/**
 * @brief CLI11's AsNumberWithUnit over a built-in type.
 *
 * @param[in] _builtin  the type, a d_arg_builtin value.
 * @param[in] _units    the units.
 * @param[in] _count    how many.
 * @param[in] _options  D_ARG_UNIT_* flags.
 * @param[in] _name     the unit's name in help; NULL for "UNIT".
 * @return the validator, or a failed one, as
 *         d_arg_validator_as_number_with_unit's.
 */
static struct d_arg_validator
d_tests_arg_unit_of(
    int                         _builtin,
    const struct d_arg_mapping* _units,
    size_t                      _count,
    uint32_t                    _options,
    const char*                 _name
)
{
    return d_arg_validator_as_number_with_unit(d_arg_type_builtin(_builtin),
                                               _units,
                                               _count,
                                               _options,
                                               _name);
}

/**
 * @brief Runs a validator over UNITS and compares its transcript with the
 *        one expected.
 *
 * @param[in] _what       what the transcript is, for a printed mismatch.
 * @param[in] _validator  the validator, consumed.
 * @param[in] _expected   the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_unit_units(
    const char*            _what,
    struct d_arg_validator _validator,
    const char*            _expected
)
{
    return d_tests_arg_holds(_what,
                             _validator,
                             UNITS,
                             UNIT_COUNT,
                             _expected);
}

/**
 * @brief Runs a validator over SIZES and compares its transcript with the
 *        one expected.
 *
 * @param[in] _what       what the transcript is, for a printed mismatch.
 * @param[in] _validator  the validator, consumed.
 * @param[in] _expected   the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_unit_sized(
    const char*            _what,
    struct d_arg_validator _validator,
    const char*            _expected
)
{
    return d_tests_arg_holds(_what,
                             _validator,
                             SIZES,
                             SIZE_COUNT,
                             _expected);
}

/**
 * @brief Whether running a validator over a value stops with the status
 *        expected.
 *
 * @param[in] _validator  the validator, consumed.
 * @param[in] _value      the value.
 * @param[in] _status     the status expected.
 * @return whether d_arg_validator_run returned it.
 */
static bool
d_tests_arg_unit_runs_as(
    struct d_arg_validator _validator,
    const char*            _value,
    int                    _status
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
                          d_pack_text_from_cstr(_value));

    const int status = d_arg_validator_run(&_validator,
                                           &validation);

    d_arg_validation_release(&validation);
    d_arg_validator_release(&_validator);

    return (status == _status);
}

/**
 * @brief Whether a unit validator could not be made, for the reason
 *        expected.
 *
 * @param[in] _validator  the validator, consumed.
 * @param[in] _status     the failure expected.
 * @param[in] _reason     the reason the validator keeps -- CLI11's message
 *                        for a failure CLI11 throws -- or NULL for none.
 * @return whether the validator holds the failure and the reason, and runs
 *         as the failure.
 */
static bool
d_tests_arg_unit_refused(
    struct d_arg_validator _validator,
    int                    _status,
    const char*            _reason
)
{
    const bool reason = (_reason != NULL)
                        ? ( (_validator.description != NULL) &&
                            (strcmp(_validator.description,
                                    _reason) == 0) )
                        : (_validator.description == NULL);
    const bool held   = (_validator.status == _status);

    return ( (d_tests_arg_unit_runs_as(_validator,
                                       "1 kb",
                                       _status)) &&
             (held)                              &&
             (reason) );
}

/**
 * @brief Whether AsNumberWithUnit over `int`, DEFAULT, refuses units as
 *        CLI11 refuses them.
 *
 * @param[in] _units    the units.
 * @param[in] _count    how many.
 * @param[in] _message  CLI11's message.
 * @return whether the validator failed, holding D_ARG_ERROR_VALIDATION and
 *         the message, and runs as that failure.
 */
static bool
d_tests_arg_unit_thrown(
    const struct d_arg_mapping* _units,
    size_t                      _count,
    const char*                 _message
)
{
    return d_tests_arg_unit_refused(
               d_tests_arg_unit_of(D_ARG_BUILTIN_INT,
                                   _units,
                                   _count,
                                   D_ARG_UNIT_DEFAULT,
                                   NULL),
               D_ARG_ERROR_VALIDATION,
               _message);
}

/**
 * @brief The units CLI11 refuses to make, each refused here with CLI11's
 *        message.
 *
 * @return whether each is a failed validator holding
 *         D_ARG_ERROR_VALIDATION and CLI11's message.
 */
static bool
d_tests_arg_unit_unmade_cli11(void)
{
    static const struct d_arg_mapping EMPTY[]   = { { "", "1" } };
    static const struct d_arg_mapping DIGIT[]   = { { "k1", "1" } };
    static const struct d_arg_mapping BOTH[]    = { { "k1", "1" },
                                                    { "",   "2" } };
    static const struct d_arg_mapping LEADING[] = { { "1k", "1" },
                                                    { "",   "2" } };
    static const struct d_arg_mapping SPACED[]  = { { "k b", "1" } };
    static const struct d_arg_mapping FOLDED[]  = { { "Ab", "1" },
                                                    { "aB", "2" },
                                                    { "c",  "3" },
                                                    { "C",  "4" } };

    return ( (d_tests_arg_unit_thrown(EMPTY,
                                      1u,
                                      "Unit must not be empty."))         &&
             (d_tests_arg_unit_thrown(DIGIT,
                                      1u,
                                      "Unit must contain only letters.")) &&
             (d_tests_arg_unit_thrown(BOTH,
                                      2u,
                                      "Unit must not be empty."))         &&
             (d_tests_arg_unit_thrown(LEADING,
                                      2u,
                                      "Unit must not be empty."))         &&
             (d_tests_arg_unit_thrown(SPACED,
                                      1u,
                                      "Unit must contain only letters.")) &&
             (d_tests_arg_unit_thrown(CASED,
                                      2u,
                                      "Several matching lowercase unit "
                                      "representations are found: kb"))  &&
             (d_tests_arg_unit_thrown(FOLDED,
                                      4u,
                                      "Several matching lowercase unit "
                                      "representations are found: ab")) );
}

/**
 * @brief Whether AsNumberWithUnit refuses arguments CLI11's types could not
 *        express, as a bad argument.
 *
 * @param[in] _builtin  the type, a d_arg_builtin value; -1 for none.
 * @param[in] _units    the units.
 * @param[in] _count    how many.
 * @return whether the validator failed, holding D_ARG_ERROR_BAD_ARGUMENT
 *         and no reason, and runs as that failure.
 */
static bool
d_tests_arg_unit_bad(
    int                         _builtin,
    const struct d_arg_mapping* _units,
    size_t                      _count
)
{
    const struct d_arg_type* const type = (_builtin >= 0)
                                          ? d_arg_type_builtin(_builtin)
                                          : NULL;

    return d_tests_arg_unit_refused(
               d_arg_validator_as_number_with_unit(type,
                                                   _units,
                                                   _count,
                                                   D_ARG_UNIT_DEFAULT,
                                                   NULL),
               D_ARG_ERROR_BAD_ARGUMENT,
               NULL);
}

/**
 * @brief The units C can pass that CLI11's types cannot express, each
 *        refused as a bad argument.
 *
 * @return whether each is a failed validator holding
 *         D_ARG_ERROR_BAD_ARGUMENT and no reason.
 */
static bool
d_tests_arg_unit_unmade_c(void)
{
    static const struct d_arg_mapping NO_FROM[] = { { NULL, "1" } };
    static const struct d_arg_mapping NO_TO[]   = { { "k", NULL } };
    static const struct d_arg_mapping UNREAD[]  = { { "k", "x" } };
    static const struct d_arg_mapping TOO_BIG[] = { { "k", "70000" } };

    return ( (d_tests_arg_unit_bad(-1,
                                   BYTES,
                                   3u))                   &&
             (d_tests_arg_unit_bad(D_ARG_BUILTIN_STRING,
                                   BYTES,
                                   3u))                   &&
             (d_tests_arg_unit_bad(D_ARG_BUILTIN_INT,
                                   NULL,
                                   1u))                   &&
             (d_tests_arg_unit_bad(D_ARG_BUILTIN_INT,
                                   NO_FROM,
                                   1u))                   &&
             (d_tests_arg_unit_bad(D_ARG_BUILTIN_INT,
                                   NO_TO,
                                   1u))                   &&
             (d_tests_arg_unit_bad(D_ARG_BUILTIN_INT,
                                   UNREAD,
                                   1u))                   &&
             (d_tests_arg_unit_bad(D_ARG_BUILTIN_SHORT,
                                   TOO_BIG,
                                   1u)) );
}

/**
 * @brief A d_arg_choice_fn that counts the choices it is given.
 *
 * @param[in] _choice   the choice; unused.
 * @param[in] _context  the `size_t` count.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_unit_count_choice(
    struct d_pack_text _choice,
    void*              _context
)
{
    (void)_choice;
    ++*(size_t*)_context;

    return D_ARG_OK;
}

/**
 * @brief Whether a validator completes as a number and lists no choices.
 *
 * @param[in] _validator  the validator, consumed.
 * @return whether its only completion flag is D_ARG_VALIDATOR_NUMBER and it
 *         lists nothing.
 */
static bool
d_tests_arg_unit_numeric(
    struct d_arg_validator _validator
)
{
    size_t    listed = 0u;
    const int status =
        d_arg_validator_each_choice(&_validator,
                                    d_tests_arg_unit_count_choice,
                                    &listed);

    return ( (status == D_ARG_OK)                                          &&
             (listed == 0u)                                                &&
             (d_tests_arg_kind_of(_validator) == D_ARG_VALIDATOR_NUMBER) );
}

/**
 * @brief Whether a copy of a validator judges as the original, once the
 *        original is gone.
 *
 * @param[in] _what       what the transcript is, for a printed mismatch.
 * @param[in] _validator  the original, consumed: released before the copy
 *                        runs.
 * @param[in] _inputs     the values.
 * @param[in] _count      how many.
 * @param[in] _expected   the original's transcript.
 * @return whether the copy was made and its transcript was as expected.
 */
static bool
d_tests_arg_unit_copied(
    const char*            _what,
    struct d_arg_validator _validator,
    const char* const*     _inputs,
    size_t                 _count,
    const char*            _expected
)
{
    const struct d_arg_validator copy = d_arg_validator_copy(&_validator);
    const bool                   made = (copy.status == D_ARG_OK);

    d_arg_validator_release(&_validator);

    return ( (d_tests_arg_holds(_what,
                                copy,
                                _inputs,
                                _count,
                                _expected)) &&
             (made) );
}

/**
 * @brief CLI11's `--size <value>`, the option transformed by
 *        AsSizeValue(false).
 *
 * @param[in] _value     the value given.
 * @param[in] _expected  the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_unit_size(
    const char* _value,
    const char* _expected
)
{
    return d_tests_arg_validates(&SIZE_OPTION,
                                 d_arg_validator_as_size_value(false),
                                 _value,
                                 _expected);
}

//==============================================================================
// 1.  NUMBERS WITH UNITS
//==============================================================================

/*
d_tests_arg_unit_integers
  CLI11's AsNumberWithUnit over `int` and `unsigned int`, in each of its
options.
  Tests the following:
  - described as "<TYPE> [<name>]", or "<TYPE> <name>" when the unit is
    required, "UNIT" when no name is given
  - a number with a unit becomes the product, written as the type writes
    values; a unit alone stands for its factor; a number alone is kept as
    written, trimmed, unless the unit is required
  - units matched without case, or exactly; the unit read after spaces,
    and a number read as the type reads it, prefixes and signs among it
  - each refusal stops the validation with CLI11's message: an empty
    value, a missing unit, a number that does not convert, a unit not
    recognized among the units listed in the map's order
*/
bool
d_tests_arg_unit_integers(void)
{
    const bool plain    = d_tests_arg_unit_units(
                              "U_INT",
                              d_tests_arg_unit_of(D_ARG_BUILTIN_INT,
                                                  BYTES,
                                                  3u,
                                                  D_ARG_UNIT_DEFAULT,
                                                  NULL),
                              U_INT);
    const bool required = d_tests_arg_unit_units(
                              "U_REQ",
                              d_tests_arg_unit_of(
                                  D_ARG_BUILTIN_INT,
                                  BYTES,
                                  3u,
                                  ( D_ARG_UNIT_CASE_SENSITIVE |
                                    D_ARG_UNIT_REQUIRED ),
                                  "SIZE"),
                              U_REQ);
    const bool natural  = d_tests_arg_unit_units(
                              "U_UINT",
                              d_tests_arg_unit_of(
                                  D_ARG_BUILTIN_UNSIGNED_INT,
                                  SCALES,
                                  2u,
                                  D_ARG_UNIT_CASE_INSENSITIVE,
                                  "N"),
                              U_UINT);

    return ( (plain)    &&
             (required) &&
             (natural) );
}

/*
d_tests_arg_unit_others
  CLI11's AsNumberWithUnit over `double`, `short` and `int64_t`.
  Tests the following:
  - a floating-point product, and a factor that is a fraction or negative
  - the product refused where CLI11's checked_multiply refuses it --
    beyond `short`, beyond 64 bits, infinite -- with "<number> multiplied
    by <unit> factor would cause number overflow. Use smaller value."
*/
bool
d_tests_arg_unit_others(void)
{
    const bool real  = d_tests_arg_unit_units(
                           "U_DBL",
                           d_tests_arg_unit_of(D_ARG_BUILTIN_DOUBLE,
                                               TIMES,
                                               3u,
                                               D_ARG_UNIT_DEFAULT,
                                               NULL),
                           U_DBL);
    const bool small = d_tests_arg_unit_units(
                           "U_SHORT",
                           d_tests_arg_unit_of(D_ARG_BUILTIN_SHORT,
                                               SHORT_SCALES,
                                               3u,
                                               D_ARG_UNIT_DEFAULT,
                                               NULL),
                           U_SHORT);
    const bool wide  = d_tests_arg_unit_units(
                           "U_I64",
                           d_tests_arg_unit_of(D_ARG_BUILTIN_INT64,
                                               WIDE_SCALES,
                                               3u,
                                               D_ARG_UNIT_DEFAULT,
                                               NULL),
                           U_I64);

    return ( (real)  &&
             (small) &&
             (wide) );
}

/*
d_tests_arg_unit_case
  Units that differ only in case.
  Tests the following:
  - matched exactly, two such units are two units
  - a required unit of mixed case is matched as written, and nothing else
    is; a number alone is refused
*/
bool
d_tests_arg_unit_case(void)
{
    static const struct d_arg_mapping MIXED[] = { { "Kb", "1" } };

    const bool exact    = d_tests_arg_holds(
                              "U_CS",
                              d_tests_arg_unit_of(D_ARG_BUILTIN_INT,
                                                  CASED,
                                                  2u,
                                                  D_ARG_UNIT_CASE_SENSITIVE,
                                                  NULL),
                              CASES,
                              CASE_COUNT,
                              U_CS);
    const bool required = d_tests_arg_holds(
                              "U_KB",
                              d_tests_arg_unit_of(D_ARG_BUILTIN_INT,
                                                  MIXED,
                                                  1u,
                                                  D_ARG_UNIT_REQUIRED,
                                                  NULL),
                              CASES,
                              CASE_COUNT,
                              U_KB);

    return ( (exact) &&
             (required) );
}

/*
d_tests_arg_unit_maps
  The units given: none at all, and one given twice.
  Tests the following:
  - with no units, a number alone passes and every unit is refused, among
    "{}"; matched without case, the unit refused is named in lowercase
  - of a unit given twice the first is kept, as CLI11's std::map keeps it
*/
bool
d_tests_arg_unit_maps(void)
{
    static const struct d_arg_mapping REPEATED[] =
    {
        { "z", "1" },
        { "b", "2" },
        { "z", "3" }
    };

    const bool none  = d_tests_arg_unit_units(
                           "U_NONE",
                           d_tests_arg_unit_of(D_ARG_BUILTIN_INT,
                                               NULL,
                                               0u,
                                               D_ARG_UNIT_DEFAULT,
                                               NULL),
                           U_NONE);
    const bool twice = d_tests_arg_holds(
                           "U_TWICE",
                           d_tests_arg_unit_of(D_ARG_BUILTIN_INT,
                                               REPEATED,
                                               3u,
                                               D_ARG_UNIT_DEFAULT,
                                               NULL),
                           TWICE,
                           TWICE_COUNT,
                           U_TWICE);

    return ( (none) &&
             (twice) );
}

//==============================================================================
// 2.  SIZES
//==============================================================================

/*
d_tests_arg_unit_sizes
  CLI11's AsSizeValue, `kb` 1000 bytes and 1024.
  Tests the following:
  - described as CLI11 describes each base
  - `b`, and each prefix alone and with `b`, `i` and `ib`, in any case, the
    `i` forms always powers of 1024
  - the result a 64-bit unsigned size: the largest passes, one past it does
    not convert, and a product past it overflows
  - a negative or fractional size does not convert to UINT
*/
bool
d_tests_arg_unit_sizes(void)
{
    const bool decimal = d_tests_arg_unit_sized(
                             "S_1000",
                             d_arg_validator_as_size_value(true),
                             S_1000);
    const bool binary  = d_tests_arg_unit_sized(
                             "S_1024",
                             d_arg_validator_as_size_value(false),
                             S_1024);

    return ( (decimal) &&
             (binary) );
}

//==============================================================================
// 3.  COMPOSITES
//==============================================================================

/*
d_tests_arg_unit_composites
  The units in CLI11's composites.
  Tests the following:
  - AsSizeValue(false) | Range(0, 5): a refusal of the size stops the
    composite, as CLI11's throw passes through `|`
  - AsNumberWithUnit | IsMember: likewise, whatever the set would say
*/
bool
d_tests_arg_unit_composites(void)
{
    static const char* const MEMBERS[] = { "alpha", "Beta", "gamma_ray" };

    const int  low   = 0;
    const int  high  = 5;
    const bool size  = d_tests_arg_unit_sized(
                           "K_OR",
                           d_arg_validator_or(
                               d_arg_validator_as_size_value(false),
                               d_arg_validator_range(
                                   d_arg_type_builtin(D_ARG_BUILTIN_INT),
                                   &low,
                                   &high)),
                           K_OR);
    const bool units = d_tests_arg_unit_units(
                           "K_UNIT_OR",
                           d_arg_validator_or(
                               d_tests_arg_unit_of(D_ARG_BUILTIN_INT,
                                                   BYTES,
                                                   3u,
                                                   D_ARG_UNIT_DEFAULT,
                                                   NULL),
                               d_arg_validator_is_member(NULL,
                                                         MEMBERS,
                                                         3u,
                                                         NULL)),
                           K_UNIT_OR);

    return ( (size) &&
             (units) );
}

//==============================================================================
// 4.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_unit_departures
  Where the module writes otherwise than CLI11, on purpose: the constant
says what CLI11 writes instead.
  Tests the following:
  - a `float` product written as the shortest text that reads back as it
*/
bool
d_tests_arg_unit_departures(void)
{
    static const struct d_arg_mapping THREE[] = { { "x", "3" } };

    return d_tests_arg_holds("D_UFLT",
                             d_tests_arg_unit_of(D_ARG_BUILTIN_FLOAT,
                                                 THREE,
                                                 1u,
                                                 D_ARG_UNIT_DEFAULT,
                                                 NULL),
                             FLOAT_UNITS,
                             sizeof(FLOAT_UNITS) / sizeof(FLOAT_UNITS[0]),
                             D_UFLT);
}

//==============================================================================
// 5.  CONSTRUCTION AND COMPLETION
//==============================================================================

/*
d_tests_arg_unit_construction
  Units that cannot be made.
  Tests the following:
  - those CLI11 refuses, refused with its message, as a failed validator
    holding D_ARG_ERROR_VALIDATION and the message: an empty unit, one of
    more than letters -- in the map's order, so an empty one is found
    first -- and two that match without case
  - those C can pass and CLI11's types cannot express, refused as
    D_ARG_ERROR_BAD_ARGUMENT: no type, a type that is no number, units
    promised and not given, a unit or factor missing, and a factor that
    does not convert to the type, or does not fit it
  - each runs as its failure
*/
bool
d_tests_arg_unit_construction(void)
{
    const bool cli11 = d_tests_arg_unit_unmade_cli11();
    const bool c     = d_tests_arg_unit_unmade_c();

    return ( (cli11) &&
             (c) );
}

/*
d_tests_arg_unit_value_kinds
  What the units complete as, and their copies.
  Tests the following:
  - AsNumberWithUnit and AsSizeValue complete as numbers, listing nothing
  - a copy of each owns what it judges by: it judges as the original did
    once the original is gone
*/
bool
d_tests_arg_unit_value_kinds(void)
{
    const bool units = d_tests_arg_unit_numeric(
                           d_tests_arg_unit_of(D_ARG_BUILTIN_INT,
                                               BYTES,
                                               3u,
                                               D_ARG_UNIT_DEFAULT,
                                               NULL));
    const bool size  = d_tests_arg_unit_numeric(
                           d_arg_validator_as_size_value(true));
    const bool kept  = d_tests_arg_unit_copied(
                           "U_INT",
                           d_tests_arg_unit_of(D_ARG_BUILTIN_INT,
                                               BYTES,
                                               3u,
                                               D_ARG_UNIT_DEFAULT,
                                               NULL),
                           UNITS,
                           UNIT_COUNT,
                           U_INT);
    const bool sized = d_tests_arg_unit_copied(
                           "S_1024",
                           d_arg_validator_as_size_value(false),
                           SIZES,
                           SIZE_COUNT,
                           S_1024);

    return ( (units) &&
             (size)  &&
             (kept)  &&
             (sized) );
}

//==============================================================================
// 6.  IN A PARSE
//==============================================================================

/*
d_tests_arg_unit_in_a_parse
  AsSizeValue on an option, as CLI11 adds it by transform().
  Tests the following:
  - a size becomes its number of bytes, in the results and the variable
  - a refusal fails the parse with ValidationError, CLI11's message after
    the option's name, and the variable keeps its zero
  - the results keep a refused value as it was given
*/
bool
d_tests_arg_unit_in_a_parse(void)
{
    const bool size    = d_tests_arg_unit_size("10kb",
                                               P_SIZE);
    const bool empty   = d_tests_arg_unit_size("",
                                               P_SIZE_EMPTY);
    const bool unknown = d_tests_arg_unit_size("1zb",
                                               P_SIZE_UNKNOWN);

    return ( (size)  &&
             (empty) &&
             (unknown) );
}

//==============================================================================
// 7.  THE SUITE
//==============================================================================

// d_tests_arg_unit
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_unit[] =
{
    { "arg_unit_integers",
      d_tests_arg_unit_integers },
    { "arg_unit_others",
      d_tests_arg_unit_others },
    { "arg_unit_case",
      d_tests_arg_unit_case },
    { "arg_unit_maps",
      d_tests_arg_unit_maps },
    { "arg_unit_sizes",
      d_tests_arg_unit_sizes },
    { "arg_unit_composites",
      d_tests_arg_unit_composites },
    { "arg_unit_departures",
      d_tests_arg_unit_departures },
    { "arg_unit_construction",
      d_tests_arg_unit_construction },
    { "arg_unit_value_kinds",
      d_tests_arg_unit_value_kinds },
    { "arg_unit_in_a_parse",
      d_tests_arg_unit_in_a_parse }
};

// d_tests_arg_unit_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_unit_count =
    sizeof(d_tests_arg_unit) / sizeof(d_tests_arg_unit[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
