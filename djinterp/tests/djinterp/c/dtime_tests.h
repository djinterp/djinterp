/*******************************************************************************
* djinterp [test]                                                  dtime_tests.h
*
* dtime.h's suite in DTest's C face (c/modules/test/test_suite.h).
*   The first block holds the register's decisions on dtime (#14, #15, #25 to
* #29 of docs/djinterp_decisions.md, Part B), each with a regression test.
* The standalone suite (dtime_tests_sa*.c) moves in as further blocks.
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
* path:      /tests/djinterp/c/dtime_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DTIME_TESTS_H
#define DJINTERP_TESTS_C_DTIME_TESTS_H 1

// djinterp
#include "../../../inc/djinterp/c/modules/test/test_suite.h"
#ifndef DTEST_SPEC_MODE
    #include "../../../inc/djinterp/c/dtime.h"
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    The register's decisions
//------------------------------------------------------------------------------
bool d_tests_dtime_timespec_to_ns(void);
bool d_tests_dtime_nanosleep(void);
bool d_tests_dtime_timegm_normalizes(void);
bool d_tests_dtime_clocks(void);
bool d_tests_dtime_strftime_s(void);
bool d_tests_dtime_timespec_cmp_null(void);
bool d_tests_dtime_timespec_type(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_dtime_register_rows
//   constant: the register block's rows.
static const struct dj_test_case_spec d_tests_dtime_register_rows[] =
{
    { "d_tests_dtime_timespec_to_ns",
      "#14: whole and negative values convert within the precondition",
      d_tests_dtime_timespec_to_ns },
    { "d_tests_dtime_nanosleep",
      "#15: a short sleep succeeds; a NULL or out-of-range request is EINVAL",
      d_tests_dtime_nanosleep },
    { "d_tests_dtime_timegm_normalizes",
      "#25: timegm, or its fallback, normalizes the broken-down time",
      d_tests_dtime_timegm_normalizes },
    { "d_tests_dtime_clocks",
      "#26: the monotonic clock never goes back; CPU time advances",
      d_tests_dtime_clocks },
    { "d_tests_dtime_strftime_s",
      "#27: strftime_s formats, and empties its buffer on a bad argument",
      d_tests_dtime_strftime_s },
    { "d_tests_dtime_timespec_cmp_null",
      "#28: NULL orders first on both sides",
      d_tests_dtime_timespec_cmp_null },
    { "d_tests_dtime_timespec_type",
      "#29: struct timespec is the platform's, with no shim of its own",
      d_tests_dtime_timespec_type }
};

// d_tests_dtime_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_dtime_blocks[] =
{
    { "register",
      "the register's decisions on dtime, each with its regression test",
      d_tests_dtime_register_rows,
      sizeof(d_tests_dtime_register_rows) /
          sizeof(d_tests_dtime_register_rows[0]) }
};

// d_tests_dtime_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_dtime_module =
{
    "dtime",
    "dtime.h: portable clocks, sleeping, conversion and formatting",
    d_tests_dtime_blocks,
    sizeof(d_tests_dtime_blocks) / sizeof(d_tests_dtime_blocks[0])
};

// d_tests_dtime_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_dtime_spec(void)
{
    return &d_tests_dtime_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_DTIME_TESTS_H
