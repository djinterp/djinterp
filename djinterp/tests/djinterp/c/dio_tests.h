/*******************************************************************************
* djinterp [test]                                                    dio_tests.h
*
* dio.h's suite in DTest's C face (c/modules/test/test_suite.h).
*   The first block tests the framework's own checked scanf engine (style #38,
* ruled again on 2026.11.04: keep the engine, test it): correct calls, sizes 0
* and 1, %[ sets, a size too small, and the size_t size on 32- and 64-bit.
*
*
* TABLE OF CONTENTS
* =================
* 1.  TESTS
*     -----
*     1.  The checked scanf engine
* 2.  SPEC
*     ----
*     1.  The provider
*
*
* path:      /tests/djinterp/c/dio_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DIO_TESTS_H
#define DJINTERP_TESTS_C_DIO_TESTS_H 1

// djinterp
#include "../../../inc/djinterp/c/modules/test/test_suite.h"
#ifndef DTEST_SPEC_MODE
    #include "../../../inc/djinterp/c/dio.h"
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    The checked scanf engine
//------------------------------------------------------------------------------
bool d_tests_dio_sscanf_s_correct(void);
bool d_tests_dio_sscanf_s_size_zero(void);
bool d_tests_dio_sscanf_s_size_one(void);
bool d_tests_dio_sscanf_s_scanset(void);
bool d_tests_dio_sscanf_s_too_small(void);
bool d_tests_dio_sscanf_s_size_type(void);
bool d_tests_dio_sscanf_s_constraints(void);
bool d_tests_dio_fscanf_s_stream(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_dio_scanf_rows
//   constant: the scanf block's rows.
static const struct dj_test_case_spec d_tests_dio_scanf_rows[] =
{
    { "d_tests_dio_sscanf_s_correct",
      "a correct %s and %d call assigns both fields",
      d_tests_dio_sscanf_s_correct },
    { "d_tests_dio_sscanf_s_size_zero",
      "a size of 0 is a matching failure that writes nothing",
      d_tests_dio_sscanf_s_size_zero },
    { "d_tests_dio_sscanf_s_size_one",
      "a size of 1 holds a %c but not a %s field",
      d_tests_dio_sscanf_s_size_one },
    { "d_tests_dio_sscanf_s_scanset",
      "a %[ set reads its characters and stops at the first other",
      d_tests_dio_sscanf_s_scanset },
    { "d_tests_dio_sscanf_s_too_small",
      "a field longer than its buffer fails and leaves it empty",
      d_tests_dio_sscanf_s_too_small },
    { "d_tests_dio_sscanf_s_size_type",
      "the size is read as a size_t, all of its bits",
      d_tests_dio_sscanf_s_size_type },
    { "d_tests_dio_sscanf_s_constraints",
      "a NULL buffer or format is a constraint violation: EOF",
      d_tests_dio_sscanf_s_constraints },
    { "d_tests_dio_fscanf_s_stream",
      "the stream form caps a field at its buffer as the string form does",
      d_tests_dio_fscanf_s_stream }
};

// d_tests_dio_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_dio_blocks[] =
{
    { "scanf",
      "d_sscanf_s and d_fscanf_s, the engine of decision #38",
      d_tests_dio_scanf_rows,
      sizeof(d_tests_dio_scanf_rows) /
          sizeof(d_tests_dio_scanf_rows[0]) }
};

// d_tests_dio_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_dio_module =
{
    "dio",
    "dio.h: portable formatted and character I/O",
    d_tests_dio_blocks,
    sizeof(d_tests_dio_blocks) / sizeof(d_tests_dio_blocks[0])
};

// d_tests_dio_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_dio_spec(void)
{
    return &d_tests_dio_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_DIO_TESTS_H
