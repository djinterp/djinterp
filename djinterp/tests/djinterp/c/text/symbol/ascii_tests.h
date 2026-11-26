/*******************************************************************************
* djinterp [c][text]                                               ascii_tests.h
*
* The suite for c/text/symbol/ascii.h, in DTest's C spec form: every class and
* both case conversions against <ctype.h> in the "C" locale, for all 256 byte
* values, which also holds that no byte above 0x7F is in any class or changes
* case.
*
*
* path:      /tests/djinterp/c/text/symbol/ascii_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.11.23
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  TESTS
2.  SPEC
    ----
    1.  The provider
*/
#ifndef DJINTERP_TESTS_C_TEXT_SYMBOL_ASCII_TESTS_H
#define DJINTERP_TESTS_C_TEXT_SYMBOL_ASCII_TESTS_H 1
// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../../inc/djinterp/c/text/symbol/ascii.h"  // d_ascii_*
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


bool d_tests_ascii_classes(void);
bool d_tests_ascii_case(void);
bool d_tests_ascii_fixed(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE
// d_tests_ascii_rows
//   constant: the block's rows.
static const struct dj_test_case_spec d_tests_ascii_rows[] =
{
    D_TEST_CASE(d_tests_ascii_classes,
                "each of the nine classes agrees with <ctype.h> on every byte"),
    D_TEST_CASE(d_tests_ascii_case,
                "to_upper and to_lower agree with toupper and tolower"),
    D_TEST_CASE(d_tests_ascii_fixed,
                "a few fixed points, whatever the reference says")
};

// d_tests_ascii_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_ascii_blocks[] =
{
    { "ascii",
      "classes and case, against the \"C\" locale",
      d_tests_ascii_rows,
      sizeof(d_tests_ascii_rows) / sizeof(d_tests_ascii_rows[0]) }
};

// d_tests_ascii_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_ascii_module =
{
    "ascii",
    "c/text/symbol: ASCII's classes and case, with no locale",
    d_tests_ascii_blocks,
    sizeof(d_tests_ascii_blocks) / sizeof(d_tests_ascii_blocks[0])
};

// d_tests_ascii_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_ascii_spec(void)
{
    return &d_tests_ascii_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_SYMBOL_ASCII_TESTS_H
