/*******************************************************************************
* djinterp [test]                                              string_fn_tests.h
*
* string_fn.h's suite in DTest's C face (c/modules/test/test_suite.h).
*   The first block holds the register's decisions on string_fn (#11, #12 and
* #30 of docs/djinterp_decisions.md, Part B), each with a regression test.
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
* path:      /tests/djinterp/c/string_fn_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_STRING_FN_TESTS_H
#define DJINTERP_TESTS_C_STRING_FN_TESTS_H 1

// djinterp
#include "../../../inc/djinterp/c/modules/test/test_suite.h"
#ifndef DTEST_SPEC_MODE
    #include "../../../inc/djinterp/c/string_fn.h"
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    The register's decisions
//------------------------------------------------------------------------------
bool d_tests_string_fn_strerror_r(void);
bool d_tests_string_fn_ascii_only(void);
bool d_tests_string_fn_casestr_embedded_nul(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_string_fn_register_rows
//   constant: the register block's rows.
static const struct dj_test_case_spec d_tests_string_fn_register_rows[] =
{
    { "d_tests_string_fn_strerror_r",
      "#11: the platform's description, with EINVAL and ERANGE as documented",
      d_tests_string_fn_strerror_r },
    { "d_tests_string_fn_ascii_only",
      "#12: classes and case folding are ASCII, whatever the locale",
      d_tests_string_fn_ascii_only },
    { "d_tests_string_fn_casestr_embedded_nul",
      "#30: a case-insensitive search reads embedded NULs",
      d_tests_string_fn_casestr_embedded_nul }
};

// d_tests_string_fn_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_string_fn_blocks[] =
{
    { "register",
      "the register's decisions on string_fn, each with its regression test",
      d_tests_string_fn_register_rows,
      sizeof(d_tests_string_fn_register_rows) /
          sizeof(d_tests_string_fn_register_rows[0]) }
};

// d_tests_string_fn_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_string_fn_module =
{
    "string_fn",
    "string_fn.h: raw-buffer string primitives",
    d_tests_string_fn_blocks,
    sizeof(d_tests_string_fn_blocks) / sizeof(d_tests_string_fn_blocks[0])
};

// d_tests_string_fn_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_string_fn_spec(void)
{
    return &d_tests_string_fn_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_STRING_FN_TESTS_H
