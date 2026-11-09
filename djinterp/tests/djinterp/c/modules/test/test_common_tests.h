/*******************************************************************************
* djinterp [test]                                            test_common_tests.h
*
* test_common.h's suite in DTest's C face (c/modules/test/test_suite.h).
*   test_common.h is the DTest vocabulary: the status value set, the three
* identifier types and the five operations on a status. The first block holds
* what a build fixes at compile time; the second, the operations, among them
* the regression for their argument width (the arguments are int64_t, so no
* configured width narrows a value before it is checked).
*
*
* path:      /tests/djinterp/c/modules/test/test_common_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.06
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TESTS
    -----
    1.  Values and identifiers
    2.  Operations
2.  SPEC
    ----
    1.  The provider
*/

#ifndef DJINTERP_TESTS_C_MODULES_TEST_TEST_COMMON_TESTS_H
#define DJINTERP_TESTS_C_MODULES_TEST_TEST_COMMON_TESTS_H 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"   // D_TEST_*
#include "../../../../../inc/djinterp/c/modules/test/test_common.h"  // tested


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    Values and identifiers
//------------------------------------------------------------------------------
bool d_tests_test_common_status_values(void);
bool d_tests_test_common_identifier_types(void);

// 1.2    Operations
//------------------------------------------------------------------------------
bool d_tests_test_common_is_valid(void);
bool d_tests_test_common_or_error(void);
bool d_tests_test_common_is_terminal(void);
bool d_tests_test_common_name(void);
bool d_tests_test_common_worse_of(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_test_common_value_rows
//   constant: the values block's rows.
static const struct dj_test_case_spec d_tests_test_common_value_rows[] =
{
    { "d_tests_test_common_status_values",
      "the five statuses are 0 to 4, dense, and both notations agree",
      d_tests_test_common_status_values },
    { "d_tests_test_common_identifier_types",
      "the three types have the signedness and widths their knobs require",
      d_tests_test_common_identifier_types }
};

// d_tests_test_common_operation_rows
//   constant: the operations block's rows.
static const struct dj_test_case_spec d_tests_test_common_operation_rows[] =
{
    { "d_tests_test_common_is_valid",
      "only 0 to 4 are statuses, at any width the caller holds",
      d_tests_test_common_is_valid },
    { "d_tests_test_common_or_error",
      "a status passes through, anything else becomes error",
      d_tests_test_common_or_error },
    { "d_tests_test_common_is_terminal",
      "every status but pending is an outcome",
      d_tests_test_common_is_terminal },
    { "d_tests_test_common_name",
      "each status has its own name, anything else is unknown",
      d_tests_test_common_name },
    { "d_tests_test_common_worse_of",
      "error > failed > skipped > pending > passed, and invalid is error",
      d_tests_test_common_worse_of }
};

// d_tests_test_common_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_test_common_blocks[] =
{
    { "values",
      "what a build fixes at compile time",
      d_tests_test_common_value_rows,
      sizeof(d_tests_test_common_value_rows) /
          sizeof(d_tests_test_common_value_rows[0]) },
    { "operations",
      "the five operations on a status",
      d_tests_test_common_operation_rows,
      sizeof(d_tests_test_common_operation_rows) /
          sizeof(d_tests_test_common_operation_rows[0]) }
};

// d_tests_test_common_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_test_common_module =
{
    "test_common",
    "c/modules/test/test_common.h: the DTest vocabulary",
    d_tests_test_common_blocks,
    sizeof(d_tests_test_common_blocks) / sizeof(d_tests_test_common_blocks[0])
};

// d_tests_test_common_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_test_common_spec(void)
{
    return &d_tests_test_common_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_MODULES_TEST_TEST_COMMON_TESTS_H
