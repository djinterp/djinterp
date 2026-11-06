/*******************************************************************************
* djinterp [test]                                                dstring_tests.h
*
* dstring.h's suite in DTest's C face (c/modules/test/test_suite.h).
*   Two faces, chosen by DTEST_SPEC_MODE as in every C suite: the section
* translation units see the header under test and the test declarations; the
* runner, which defines DTEST_SPEC_MODE first, sees the declarations and the
* spec provider, d_tests_dstring_spec().
*
*   The first block holds the register's decisions on dstring (#13, #19, #22,
* #23 and #24 of docs/djinterp_decisions.md, Part B): each fix that the
* settlement of 2026.10.04 made, with a test that fails on the code before
* it. The standalone suite (dstring_tests_sa*.c) moves in as further blocks
* as it is converted.
*
*
* TABLE OF CONTENTS
* =================
* 1.  TESTS
*     -----
*     1.  The register's decisions
* 2.  SPEC
*     ----
*     1.  The provider
*
*
* path:      /tests/djinterp/c/dstring_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DSTRING_TESTS_H
#define DJINTERP_TESTS_C_DSTRING_TESTS_H 1

// djinterp
#include "../../../inc/djinterp/c/modules/test/test_suite.h"
#ifndef DTEST_SPEC_MODE
    #include "../../../inc/djinterp/c/dstring.h"  // the header under test
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    The register's decisions
//------------------------------------------------------------------------------
bool d_tests_dstring_tokenize_updates_size(void);
bool d_tests_dstring_split_tokens(void);
bool d_tests_dstring_split_growth(void);
bool d_tests_dstring_split_only_delimiters(void);
bool d_tests_dstring_npos_counts_clamp(void);
bool d_tests_dstring_self_operands(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_dstring_register_rows
//   constant: the register block's rows.
static const struct dj_test_case_spec d_tests_dstring_register_rows[] =
{
    { "d_tests_dstring_tokenize_updates_size",
      "#13: after the first tokenize the size is the text's length",
      d_tests_dstring_tokenize_updates_size },
    { "d_tests_dstring_split_tokens",
      "#19: the split helpers keep split's results, empty input included",
      d_tests_dstring_split_tokens },
    { "d_tests_dstring_split_growth",
      "#19: a thousand tokens grow the array through every doubling",
      d_tests_dstring_split_growth },
    { "d_tests_dstring_split_only_delimiters",
      "#24: delimiters alone give no token and a NULL array",
      d_tests_dstring_split_only_delimiters },
    { "d_tests_dstring_npos_counts_clamp",
      "#22: a SIZE_MAX count clamps in substr, erase and both replaces",
      d_tests_dstring_npos_counts_clamp },
    { "d_tests_dstring_self_operands",
      "#23: append, insert, assign and replace take the string itself",
      d_tests_dstring_self_operands }
};

// d_tests_dstring_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_dstring_blocks[] =
{
    { "register",
      "the register's decisions on dstring, each with its regression test",
      d_tests_dstring_register_rows,
      sizeof(d_tests_dstring_register_rows) /
          sizeof(d_tests_dstring_register_rows[0]) }
};

// d_tests_dstring_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_dstring_module =
{
    "dstring",
    "dstring.h: the dj_string type and its operations",
    d_tests_dstring_blocks,
    sizeof(d_tests_dstring_blocks) / sizeof(d_tests_dstring_blocks[0])
};

// d_tests_dstring_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_dstring_spec(void)
{
    return &d_tests_dstring_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_DSTRING_TESTS_H
