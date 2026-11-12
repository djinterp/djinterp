/*******************************************************************************
* djinterp [c]                                                  djinterp_tests.h
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
*     2.  The sections' entry points
* 3.  SECTIONS
*     --------
*
*
* path:      /tests/djinterp/c/djinterp_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DJINTERP_TESTS_H
#define DJINTERP_TESTS_C_DJINTERP_TESTS_H 1

// djinterp
#include "../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    // std
    #include <limits.h>
    #include <stdlib.h>
    // djinterp
    #include "../../../inc/djinterp/c/djinterp.h"  // framework root
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

// 2.2    The sections' entry points
//------------------------------------------------------------------------------
// The tests built call by call (section 3), which the runner calls after
// adding the spec.
bool d_tests_djinterp_run_all(struct dj_test_suite* _suite);

bool d_tests_djinterp_core_all_extended(struct dj_test_suite* _suite);


#endif  // DTEST_SPEC_MODE


//==============================================================================
// 3.  SECTIONS
//==============================================================================
// The module's tests built call by call (test_suite.h 3.2), outside spec
// mode only, since they read the module's own types.
#ifndef DTEST_SPEC_MODE

/******************************************************************************
 * MODULE 1: dj_index FUNCTION TESTS (djinterp_tests_sa_index.c)
 *****************************************************************************/

// d_index_convert_fast() tests

// d_index_convert_safe() tests

// d_index_is_valid() tests

// Comprehensive dj_index function tests


/******************************************************************************
 * MODULE 2: INDEX MANIPULATION MACRO TESTS (djinterp_tests_sa_index_macros.c)
 *****************************************************************************/

// Individual macro tests
bool d_tests_djinterp_macro_clamp_index(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_macro_index_in_bounds(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_macro_safe_arr_idx(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_macro_is_valid_index(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_macro_is_valid_index_n(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_macro_neg_idx(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_macro_arr_idx(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_macro_function_consistency(
    struct dj_test_suite* _suite
);

// Comprehensive index macro tests
bool d_tests_djinterp_index_macros_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE 3: ARRAY UTILITY MACRO TESTS (djinterp_tests_sa_array.c)
 *****************************************************************************/

// Array macro tests

// Comprehensive array macro tests


/******************************************************************************
 * MODULE 4: BOOLEAN CONSTANT/MACRO TESTS (djinterp_tests_sa_boolean.c)
 *****************************************************************************/

// Boolean constant tests

// Boolean macro tests

// Comprehensive boolean tests


/******************************************************************************
 * MODULE 5: FUNCTION POINTER TESTS (djinterp_tests_sa_function_ptr.c)
 *****************************************************************************/

// Function pointer type tests

// Comprehensive function pointer tests


/******************************************************************************
 * MODULE 6: EDGE CASE TESTS (djinterp_tests_sa_edge.c)
 *****************************************************************************/

// Boundary condition tests

// Special size tests

// Comprehensive edge case tests


/******************************************************************************
 * MASTER TEST RUNNERS (djinterp_tests_sa.c)
 *****************************************************************************/

// Module-level runners
bool d_tests_djinterp_run_index_macros(
    struct dj_test_suite* _suite
);

// Category-level runners
bool d_tests_djinterp_run_all_macros(
    struct dj_test_suite* _suite
);

// Master runner - runs EVERYTHING
bool d_tests_djinterp_run_all(
    struct dj_test_suite* _suite
);

// Utility functions


// `ssize64` creation and access tests
bool d_tests_djinterp_ssize64_creation(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_ssize64_access(
    struct dj_test_suite* _suite
);

// `ssize64` arithmetic operation tests
bool d_tests_djinterp_ssize64_arithmetic_functions(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_ssize64_arithmetic_macros(
    struct dj_test_suite* _suite
);

// `ssize64` comparison tests
bool d_tests_djinterp_ssize64_comparison_ssize64(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_ssize64_comparison_other_types(
    struct dj_test_suite* _suite
);

// `ssize64` conversion and validation tests
bool d_tests_djinterp_ssize64_conversion_functions(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_ssize64_validation_functions(
    struct dj_test_suite* _suite
);

bool d_tests_djinterp_ssize64_edge_cases(
    struct dj_test_suite* _suite
);

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_DJINTERP_TESTS_H
