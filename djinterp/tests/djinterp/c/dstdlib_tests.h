/*******************************************************************************
* djinterp [c]                                                   dstdlib_tests.h
*
* dstdlib.h's suite in DTest's C face (c/modules/test/test_suite.h).
*   Each test sets and removes variables of its own, named DJINTERP_DSTDLIB_*,
* and leaves none set.
*
*
* TABLE OF CONTENTS
* =================
* 1.  TESTS
*     -----
*     1.  Reading a variable
*     2.  Setting and removing a variable
* 2.  SPEC
*     ----
*     1.  The provider
*
*
* path:      /tests/djinterp/c/dstdlib_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.26
*                                                            revised: 2026.11.26
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DSTDLIB_TESTS_H
#define DJINTERP_TESTS_C_DSTDLIB_TESTS_H 1

// djinterp
#include "../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    // std
    #include <errno.h>   // EINVAL, ENOENT, ERANGE
    #include <stdlib.h>  // free
    #include <string.h>  // memset, strcmp
    // djinterp
    #include "../../../inc/djinterp/c/dstdlib.h"  // d_getenv_copy and kin
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    Reading a variable
//------------------------------------------------------------------------------
bool d_tests_dstdlib_copy_set(void);

bool d_tests_dstdlib_copy_bounds(void);

bool d_tests_dstdlib_copy_query(void);

bool d_tests_dstdlib_copy_unset(void);

bool d_tests_dstdlib_dup(void);

bool d_tests_dstdlib_names(void);

// 1.2    Setting and removing a variable
//------------------------------------------------------------------------------
bool d_tests_dstdlib_set_replace(void);

bool d_tests_dstdlib_set_empty(void);

bool d_tests_dstdlib_unset_absent(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_dstdlib_read_rows
//   constant: the reading block's rows.
static const struct dj_test_case_spec d_tests_dstdlib_read_rows[] =
{
    { "d_tests_dstdlib_copy_set",
      "a set variable is copied whole, with its length",
      d_tests_dstdlib_copy_set },
    { "d_tests_dstdlib_copy_bounds",
      "a value that fills the buffer is ERANGE; one byte more fits",
      d_tests_dstdlib_copy_bounds },
    { "d_tests_dstdlib_copy_query",
      "a NULL buffer of size 0 asks whether it is set, and how long",
      d_tests_dstdlib_copy_query },
    { "d_tests_dstdlib_copy_unset",
      "a variable not set is ENOENT, the buffer emptied",
      d_tests_dstdlib_copy_unset },
    { "d_tests_dstdlib_dup",
      "the copy the caller frees, or ENOENT and NULL",
      d_tests_dstdlib_dup },
    { "d_tests_dstdlib_names",
      "a NULL or empty name, or one with '=', is EINVAL everywhere",
      d_tests_dstdlib_names }
};

// d_tests_dstdlib_write_rows
//   constant: the setting block's rows.
static const struct dj_test_case_spec d_tests_dstdlib_write_rows[] =
{
    { "d_tests_dstdlib_set_replace",
      "setting a variable again replaces its value",
      d_tests_dstdlib_set_replace },
    { "d_tests_dstdlib_set_empty",
      "a value of \"\" removes the variable, on every platform",
      d_tests_dstdlib_set_empty },
    { "d_tests_dstdlib_unset_absent",
      "removing a variable not set succeeds",
      d_tests_dstdlib_unset_absent }
};

// d_tests_dstdlib_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_dstdlib_blocks[] =
{
    { "read",
      "d_getenv_copy and d_getenv_dup",
      d_tests_dstdlib_read_rows,
      sizeof(d_tests_dstdlib_read_rows) /
          sizeof(d_tests_dstdlib_read_rows[0]) },
    { "write",
      "d_setenv and d_unsetenv",
      d_tests_dstdlib_write_rows,
      sizeof(d_tests_dstdlib_write_rows) /
          sizeof(d_tests_dstdlib_write_rows[0]) }
};

// d_tests_dstdlib_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_dstdlib_module =
{
    "dstdlib",
    "dstdlib.h: the environment, the same on every platform",
    d_tests_dstdlib_blocks,
    sizeof(d_tests_dstdlib_blocks) / sizeof(d_tests_dstdlib_blocks[0])
};

// d_tests_dstdlib_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_dstdlib_spec(void)
{
    return &d_tests_dstdlib_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_DSTDLIB_TESTS_H
