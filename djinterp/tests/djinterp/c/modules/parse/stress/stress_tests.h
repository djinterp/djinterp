/*******************************************************************************
* djinterp [c][parse]                                             stress_tests.h
*
* Expression core v2's stress suite in DTest's C face: seeded round trips
* and seeded random bytes, the same cases on every compiler and every run.
*   A round trip renders a random term and reads the text back by the same
* signature, which must give the same term. Two signatures between them hold
* every form the core has: math's (prefix, infix of each association,
* postfix, chains of relations, and mixfix lists, rows and bars) and an
* awk-like one's (a flattening juxtaposition, a call that must touch its
* name, a prefix and a postfix operator on one token, and mixfix operators
* with a tail, `?:` and `if ... then ... else`).
*   Random bytes go through the default scanner, alone and into the parser,
* which must end, keep every place inside the text, and take steps linear
* in its length. A libFuzzer harness over the same path is built beside the
* suite where the compiler offers one (stress_fuzz.c); the same file, built
* with D_TESTS_PARSE_STRESS_REPLAY, replays the harness's corpus as a test
* on every compiler.
*   Two faces, chosen by DTEST_SPEC_MODE as in every C suite: the section
* units see expression core v2's suite fixtures; the runner sees the tests
* and the spec provider, d_tests_parse_stress_spec().
*
*   knobs:  D_TESTS_PARSE_STRESS_TERMS  the terms each round trip draws
*                                       (100000)
*           D_TESTS_PARSE_STRESS_TEXTS  the random texts the bytes tests
*                                       draw (20000)
*
*
* path:      /tests/djinterp/c/modules/parse/stress/stress_tests.h
* link(s):   /docs/djinterp/c/modules/parse/expr/expression_core_v2.html
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_PARSE_STRESS_TESTS_H
#define DJINTERP_TESTS_C_PARSE_STRESS_TESTS_H 1

// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    #include "../expr/expr_tests.h"  // the core, its languages, the generator
#endif  // DTEST_SPEC_MODE


// D_TESTS_PARSE_STRESS_TERMS
//   constant: the terms each round trip draws.
#ifndef D_TESTS_PARSE_STRESS_TERMS
    #define D_TESTS_PARSE_STRESS_TERMS 100000u
#endif

// D_TESTS_PARSE_STRESS_TEXTS
//   constant: the random texts each bytes test draws.
#ifndef D_TESTS_PARSE_STRESS_TEXTS
    #define D_TESTS_PARSE_STRESS_TEXTS 20000u
#endif


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================
bool d_tests_stress_math_roundtrip(void);
bool d_tests_stress_awk_roundtrip(void);
bool d_tests_stress_scan_bytes(void);
bool d_tests_stress_parse_bytes(void);

#ifndef DTEST_SPEC_MODE

// d_tests_stress_fuzz_one
//   function: one input through the scanner and the parser, as the libFuzzer
// harness and the corpus replay run it: true where the parse ended within
// its bounds and left a term that verifies.
bool d_tests_stress_fuzz_one(const uint8_t* _data,
                             size_t         _size);

#endif  // DTEST_SPEC_MODE


//==============================================================================
// 2.  SPEC
//==============================================================================
#ifdef DTEST_SPEC_MODE

// d_tests_parse_stress_roundtrip_rows
//   constant: the round trips' rows.
static const struct dj_test_case_spec d_tests_parse_stress_roundtrip_rows[] =
{
    { "d_tests_stress_math_roundtrip",
      "math's random terms, rendered and read back",
      d_tests_stress_math_roundtrip },
    { "d_tests_stress_awk_roundtrip",
      "an awk-like language's random terms, rendered and read back",
      d_tests_stress_awk_roundtrip }
};

// d_tests_parse_stress_bytes_rows
//   constant: the random bytes' rows.
static const struct dj_test_case_spec d_tests_parse_stress_bytes_rows[] =
{
    { "d_tests_stress_scan_bytes",
      "random bytes through the scanner, in linear steps",
      d_tests_stress_scan_bytes },
    { "d_tests_stress_parse_bytes",
      "random bytes scanned and parsed, in linear steps",
      d_tests_stress_parse_bytes }
};

// d_tests_parse_stress_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_parse_stress_blocks[] =
{
    { "roundtrip",
      "seeded random terms, rendered and read back",
      d_tests_parse_stress_roundtrip_rows,
      sizeof(d_tests_parse_stress_roundtrip_rows) /
          sizeof(d_tests_parse_stress_roundtrip_rows[0]) },
    { "bytes",
      "seeded random bytes, scanned and parsed",
      d_tests_parse_stress_bytes_rows,
      sizeof(d_tests_parse_stress_bytes_rows) /
          sizeof(d_tests_parse_stress_bytes_rows[0]) }
};

// d_tests_parse_stress_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_parse_stress_module =
{
    "parse_stress",
    "parse/expr: seeded round trips and random bytes",
    d_tests_parse_stress_blocks,
    sizeof(d_tests_parse_stress_blocks) /
        sizeof(d_tests_parse_stress_blocks[0])
};

// d_tests_parse_stress_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_parse_stress_spec(void)
{
    return &d_tests_parse_stress_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_PARSE_STRESS_TESTS_H
