/*******************************************************************************
* djinterp [functional]                                pipeline_tests_creation.c
*
*
* path:      /tests/djinterp/c/functional/pipeline_tests_creation.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./pipeline_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST HELPER: int transformer (doubles each element)
 *****************************************************************************/
static
bool test_helper_double_int(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    if ( (!_input) ||
         (!_output) )
    {
        return false;
    }

    *(int*)_output = (*(const int*)_input) * 2;

    return true;
}


/**
 * @brief Part 1 of d_tests_pipeline_begin.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - valid parameters
 *   - single element
 *   - NULL data
 *   - zero count: an empty pipeline, which is valid
 *   - zero element_size
 */
static bool
d_internal_tests_pipeline_begin_part1(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    int                           data[] = { 10, 20, 30, 40, 50 };
    bool                          all_passed;

    all_passed = true;

    // ---- valid parameters ----
    pipe = d_functional_pipeline_begin(data,
                                       5,
                                       sizeof(int));

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: valid params produce error_code 0",
                                   pipe.error_code == 0,
                                   "error_code should be 0 for valid input");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: data pointer matches input",
                                   pipe.data == data,
                                   "pipeline should wrap the original pointer");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: count matches input",
                                   pipe.count == 5,
                                   "count should be 5");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: element_size matches input",
                                   pipe.element_size == sizeof(int),
                                   "element_size should be sizeof(int)");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: pipeline does NOT own the data",
                                   pipe.owns_data == false,
                                   "owns_data should be false for begin (no copy)");

    // ---- single element ----
    pipe = d_functional_pipeline_begin(data,
                                       1,
                                       sizeof(int));

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: single element is valid",
                                   pipe.error_code == 0 && pipe.count == 1,
                                   "a 1-element pipeline should succeed");

    // ---- NULL data ----
    pipe = d_functional_pipeline_begin(NULL,
                                       5,
                                       sizeof(int));

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: NULL data returns error",
                                   pipe.error_code == EINVAL,
                                   "error_code should be EINVAL for NULL data");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: NULL data error has NULL data ptr",
                                   pipe.data == NULL,
                                   "data should be NULL on error");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: NULL data error does not own data",
                                   pipe.owns_data == false,
                                   "owns_data should be false on error");

    // ---- zero count: an empty pipeline, which is valid ----
    pipe = d_functional_pipeline_begin(data,
                                       0,
                                       sizeof(int));

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: zero count is an empty pipeline",
                                   pipe.error_code == 0,
                                   "an empty pipeline is not an error");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: zero count has count 0",
                                   pipe.count == 0,
                                   "count should be 0");

    // ---- zero element_size ----
    pipe = d_functional_pipeline_begin(data,
                                       5,
                                       0);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: zero element_size returns error",
                                   pipe.error_code == EINVAL,
                                   "error_code should be EINVAL for size 0");

    return all_passed;
}


/**
 * @brief Part 2 of d_tests_pipeline_begin.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - all invalid simultaneously (a count with no array and no size)
 */
static bool
d_internal_tests_pipeline_begin_part2(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    bool                          all_passed;

    all_passed = true;

    // ---- all invalid simultaneously (a count with no array and no size) ----
    pipe = d_functional_pipeline_begin(NULL,
                                       5,
                                       0);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin: all params invalid returns error",
                                   pipe.error_code == EINVAL,
                                   "error_code should be EINVAL");

    return all_passed;
}


/*
d_tests_pipeline_begin
  Tests d_functional_pipeline_begin for wrapping mutable data.
  Tests the following:
  - valid data, count, and element_size produces a valid pipeline
  - pipeline does NOT own the data (owns_data == false)
  - pipeline stores correct data pointer, count, and element_size
  - error_code is 0 on success
  - NULL data returns error pipeline (error_code == EINVAL)
  - zero count is an empty, valid pipeline
  - zero element_size returns error pipeline
  - error pipeline has NULL data, zero count, owns_data == false
*/
bool d_tests_pipeline_begin(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    d_test_suite_test(_suite, "pipeline_begin");

    all_passed = true;
    all_passed &= d_internal_tests_pipeline_begin_part1(_suite);
    all_passed &= d_internal_tests_pipeline_begin_part2(_suite);

    return all_passed;
}


/*
d_tests_pipeline_begin_copy
  Tests d_functional_pipeline_begin_copy for copying immutable data.
  Tests the following:
  - valid data, count, element_size produces a valid pipeline
  - pipeline OWNS the data (owns_data == true)
  - copied data matches original contents
  - copied data is at a DIFFERENT address than the original
  - error_code is 0 on success
  - NULL data returns error pipeline
  - zero count is an empty, valid pipeline
  - zero element_size returns error pipeline
  - error pipeline fields are properly zeroed
  - modifications to the copy do not affect the original
*/
bool d_tests_pipeline_begin_copy(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    const int                    data[] = { 100, 200, 300 };
    int*                         copied;
    bool                         all_passed;
    bool                         contents_match;
    size_t                       i;

    d_test_suite_test(_suite, "pipeline_begin_copy");

    all_passed = true;

    // ---- valid parameters ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            3,
                                            sizeof(int));

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: valid params produce error_code 0",
                                   pipe.error_code == 0,
                                   "error_code should be 0 for valid input");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: pipeline OWNS the data",
                                   pipe.owns_data == true,
                                   "owns_data should be true (data was copied)");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: data is non-NULL",
                                   pipe.data != NULL,
                                   "copied data pointer should not be NULL");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: data is at a DIFFERENT address",
                                   pipe.data != (void*)data,
                                   "copy should not alias the original");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: count matches input",
                                   pipe.count == 3,
                                   "count should be 3");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: element_size matches input",
                                   pipe.element_size == sizeof(int),
                                   "element_size should be sizeof(int)");

    // verify contents match
    copied          = (int*)pipe.data;
    contents_match  = true;

    for (i = 0; i < 3; i++)
    {
        if (copied[i] != data[i])
        {
            contents_match = false;
        }
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: copied contents match original",
                                   contents_match,
                                   "each element should be identical to the source");

    // verify independence: modify copy, original unchanged
    copied[0] = 999;

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: modifying copy does not affect original",
                                   data[0] == 100,
                                   "original data[0] should still be 100");

    // clean up owned data
    d_functional_pipeline_free(&pipe);

    // ---- single element copy ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            1,
                                            sizeof(int));

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: single element copy succeeds",
                                   pipe.error_code == 0 && pipe.count == 1,
                                   "a 1-element copied pipeline should succeed");

    d_functional_pipeline_free(&pipe);

    // ---- NULL data ----
    pipe = d_functional_pipeline_begin_copy(NULL,
                                            3,
                                            sizeof(int));

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: NULL data returns error",
                                   pipe.error_code == EINVAL,
                                   "error_code should be EINVAL for NULL data");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: NULL data error fields are clean",
                                   pipe.data == NULL && pipe.owns_data == false,
                                   "data should be NULL and owns_data false on error");

    // ---- zero count: an empty pipeline, with nothing to copy ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            0,
                                            sizeof(int));

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: zero count is an empty pipeline",
                                   (pipe.error_code == 0) &&
                                   (pipe.count == 0) &&
                                   (pipe.data == NULL),
                                   "nothing copied, nothing owned");

    // ---- zero element_size ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            3,
                                            0);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "begin_copy: zero element_size returns error",
                                   pipe.error_code == EINVAL,
                                   "error_code should be EINVAL for size 0");

    // ---- large element type (struct) ----
    {
        struct test_large
        {
            double x;
            double y;
            double z;
        };
        struct test_large pts[] = { {1.0, 2.0, 3.0}, {4.0, 5.0, 6.0} };
        struct test_large* pts_copy;

        pipe = d_functional_pipeline_begin_copy(pts,
                                                2,
                                                sizeof(struct test_large));

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "begin_copy: large struct elements succeed",
                                       pipe.error_code == 0,
                                       "copying structs should work");

        pts_copy = (struct test_large*)pipe.data;

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "begin_copy: large struct contents correct",
                                       pts_copy[0].x == 1.0 && pts_copy[1].z == 6.0,
                                       "struct field values should match originals");

        d_functional_pipeline_free(&pipe);
    }

    return all_passed;
}


/*
d_tests_pipeline_creation_all
  Runs all pipeline creation tests.
  Tests the following:
  - d_functional_pipeline_begin
  - d_functional_pipeline_begin_copy
*/
bool d_tests_pipeline_creation_all(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    d_test_suite_block(_suite, "pipeline_creation_all");

    all_passed = true;

    all_passed &= d_tests_pipeline_begin(_suite);
    all_passed &= d_tests_pipeline_begin_copy(_suite);

    return all_passed;
}
