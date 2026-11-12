/*******************************************************************************
* djinterp [c][test]                                test_counter_machine_tests.h
*
* DTest's suite for its C counters' opt-in abstract machine, section 4 of
* test_counter.h: the option_env schema of the counters' settings, and the
* reader that turns a frame into tally options.
*   The section compiles only with D_CFG_TEST_COUNTER_MACHINE, so the suite
* has a leaf of its own that sets it; built without it, each of its tests
* skips.
*
*
* path:      /tests/djinterp/c/modules/test/test_counter_machine_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TESTS
    -----
    1.  The machine
2.  SPEC
    ----
    1.  The provider
*/

#ifndef DJINTERP_TESTS_C_TEST_COUNTER_MACHINE_TESTS_H
#define DJINTERP_TESTS_C_TEST_COUNTER_MACHINE_TESTS_H 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"  // sec. 4


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    The machine
//------------------------------------------------------------------------------
bool
d_tests_test_counter_machine_schema(void);

bool
d_tests_test_counter_machine_defaults(void);

bool
d_tests_test_counter_machine_bound(void);

bool
d_tests_test_counter_machine_refused(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_test_counter_machine_rows
//   constant: the machine block's rows.
static const struct dj_test_case_spec d_tests_test_counter_machine_rows[] =
{
    { "d_tests_test_counter_machine_schema",
      "four variables, each declared once, with the knobs as defaults",
      d_tests_test_counter_machine_schema },
    { "d_tests_test_counter_machine_defaults",
      "a frame that binds nothing, or none at all, reads the knobs",
      d_tests_test_counter_machine_defaults },
    { "d_tests_test_counter_machine_bound",
      "bindings made by text become the tally options",
      d_tests_test_counter_machine_bound },
    { "d_tests_test_counter_machine_refused",
      "a value out of range or unknown is refused, and a step of 0 kept out",
      d_tests_test_counter_machine_refused }
};

// d_tests_test_counter_machine_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_test_counter_machine_blocks[] =
{
    { "machine",
      "the counters' settings as an option_env schema",
      d_tests_test_counter_machine_rows,
      sizeof(d_tests_test_counter_machine_rows) /
          sizeof(d_tests_test_counter_machine_rows[0]) }
};

// d_tests_test_counter_machine_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_test_counter_machine_module =
{
    "test_counter_machine",
    "c/modules/test: the test counters' opt-in abstract machine",
    d_tests_test_counter_machine_blocks,
    sizeof(d_tests_test_counter_machine_blocks) /
        sizeof(d_tests_test_counter_machine_blocks[0])
};

// d_tests_test_counter_machine_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_test_counter_machine_spec(void)
{
    return &d_tests_test_counter_machine_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEST_COUNTER_MACHINE_TESTS_H
