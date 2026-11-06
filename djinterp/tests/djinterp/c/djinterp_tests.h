/*******************************************************************************
* djinterp [test]                                               djinterp_tests.h
*
* djinterp.h's suite in DTest's C face (c/modules/test/test_suite.h).
*   The first block holds the register's decision #21
* (docs/djinterp_decisions.md, Part B): the index macros go through
* d_index_is_valid.
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
* path:      /tests/djinterp/c/djinterp_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DJINTERP_TESTS_H
#define DJINTERP_TESTS_C_DJINTERP_TESTS_H 1

// djinterp
#include "../../../inc/djinterp/c/modules/test/test_suite.h"
#ifndef DTEST_SPEC_MODE
    #include "../../../inc/djinterp/c/djinterp.h"
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    The register's decisions
//------------------------------------------------------------------------------
bool d_tests_djinterp_index_macros(void);
bool d_tests_djinterp_index_most_negative(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_djinterp_register_rows
//   constant: the register block's rows.
static const struct dj_test_case_spec d_tests_djinterp_register_rows[] =
{
    { "d_tests_djinterp_index_macros",
      "#21: the index macros agree with d_index_is_valid at the limits",
      d_tests_djinterp_index_macros },
    { "d_tests_djinterp_index_most_negative",
      "#21: the most negative index is invalid, and nothing negates it",
      d_tests_djinterp_index_most_negative }
};

// d_tests_djinterp_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_djinterp_blocks[] =
{
    { "register",
      "the register's decisions on the root, each with its regression test",
      d_tests_djinterp_register_rows,
      sizeof(d_tests_djinterp_register_rows) /
          sizeof(d_tests_djinterp_register_rows[0]) }
};

// d_tests_djinterp_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_djinterp_module =
{
    "djinterp",
    "djinterp.h: the framework root",
    d_tests_djinterp_blocks,
    sizeof(d_tests_djinterp_blocks) / sizeof(d_tests_djinterp_blocks[0])
};

// d_tests_djinterp_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_djinterp_spec(void)
{
    return &d_tests_djinterp_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_DJINTERP_TESTS_H
