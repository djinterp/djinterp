/*******************************************************************************
* djinterp [functional]                            pipeline_tests_finalization.c
*
*
* path:      /tests/djinterp/c/functional/pipeline_tests_finalization.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./pipeline_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST HELPER: int transformer (triples each element)
 *****************************************************************************/
static
bool test_helper_triple_int(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = (*(const int*)_input) * 3;

    return true;
}


/*
d_tests_pipeline_end
  Tests d_functional_pipeline_end.
  Tests the following:
  - end hands over a block of the caller's own (a view is copied)
  - end writes count to _out_count when non-NULL
  - end with NULL _out_count does not crash
  - end returns NULL for an error pipeline
  - end writes count even on error (count is written before error check)
  - caller takes ownership of data after end
  - end after map returns the mapped buffer
*/
bool d_tests_pipeline_end(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    int                          data[] = { 5, 10, 15, 20 };
    void*                        result;
    size_t                       out_count;
    bool                         all_passed;

    d_test_suite_test(_suite, "pipeline_end");

    all_passed = true;

    // ---- valid pipeline with _out_count ----
    pipe   = d_functional_pipeline_begin(data, 4, sizeof(int));
    result = d_functional_pipeline_end(pipe, &out_count);

    //   end always hands over a block of the caller's own: a view, as here,
    // is copied
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "end: returns a copy of a view",
                                   (result != NULL) &&
                                   (result != (void*)data) &&
                                   (((int*)result)[0] == 5) &&
                                   (((int*)result)[3] == 20),
                                   "a view's elements, in a block of its own");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "end: writes count to _out_count",
                                   out_count == 4,
                                   "out_count should be 4");

    free(result);

    // ---- valid pipeline with NULL _out_count ----
    pipe   = d_functional_pipeline_begin(data, 4, sizeof(int));
    result = d_functional_pipeline_end(pipe, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "end: NULL _out_count does not crash",
                                   result != NULL,
                                   "should still return the copy");

    free(result);

    // ---- error pipeline ----
    {
        struct dj_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        out_count = 999;
        result    = d_functional_pipeline_end(err_pipe, &out_count);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "end: returns NULL for error pipeline",
                                       result == NULL,
                                       "error pipeline should yield NULL");

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "end: writes count even on error",
                                       out_count == 0,
                                       "out_count should be 0 (from err_pipe.count)");
    }

    // ---- end after map: caller owns the mapped buffer ----
    {
        int    src[] = { 1, 2, 3 };
        int*   mapped_data;
        size_t mapped_count;

        pipe        = d_functional_pipeline_begin(src,
                                                  3,
                                                  sizeof(int));
        pipe        = d_functional_pipeline_map(pipe,
                                                test_helper_triple_int,
                                                NULL);
        mapped_data = (int*)d_functional_pipeline_end(pipe,
                                                      &mapped_count);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "end: after map returns non-NULL with count 3",
                                       mapped_data != NULL && mapped_count == 3,
                                       "caller should receive the mapped buffer");

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "end: after map values are 3,6,9",
                                       mapped_data[0] == 3 && mapped_data[1] == 6 && mapped_data[2] == 9,
                                       "1*3=3, 2*3=6, 3*3=9");

        // caller is responsible for freeing
        free(mapped_data);
    }

    // ---- end with begin_copy: caller owns the copy ----
    {
        const int src[] = { 42, 84 };
        int*      copy_data;
        size_t    copy_count;

        pipe      = d_functional_pipeline_begin_copy(src,
                                                     2,
                                                     sizeof(int));
        copy_data = (int*)d_functional_pipeline_end(pipe,
                                                    &copy_count);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "end: after begin_copy returns owned copy",
                                       copy_data != NULL && copy_count == 2,
                                       "caller takes ownership of copied data");

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "end: copy values match original",
                                       copy_data[0] == 42 && copy_data[1] == 84,
                                       "copied data should match source");

        free(copy_data);
    }

    // ---- end after take: reduced count ----
    {
        size_t taken_count;

        pipe   = d_functional_pipeline_begin(data, 4, sizeof(int));
        pipe   = d_functional_pipeline_take(pipe, 2);
        result = d_functional_pipeline_end(pipe, &taken_count);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "end: after take(2) returns count 2",
                                       result != NULL && taken_count == 2,
                                       "taken count should be reflected in end");

        free(result);
    }

    // ---- end after skip: reduced count ----
    {
        size_t skipped_count;

        pipe   = d_functional_pipeline_begin(data, 4, sizeof(int));
        pipe   = d_functional_pipeline_skip(pipe, 1);
        result = d_functional_pipeline_end(pipe, &skipped_count);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "end: after skip(1) returns count 3",
                                       result != NULL && skipped_count == 3,
                                       "skipped count should be reflected in end");

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "end: after skip(1) first element is data[1]",
                                       ((int*)result)[0] == 10,
                                       "first visible element should be 10");

        free(result);
    }

    return all_passed;
}


/*
d_tests_pipeline_free
  Tests d_functional_pipeline_free.
  Tests the following:
  - free on an owned pipeline frees data and resets fields
  - free on a non-owned pipeline resets fields but does not free data
  - free on a NULL pointer is a safe no-op
  - free resets error_code to 0
  - free sets data to NULL, count to 0, owns_data to false
  - double-free safety: freeing an already-freed pipeline is safe
*/
bool d_tests_pipeline_free(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    int                          data[] = { 1, 2, 3 };
    bool                         all_passed;

    d_test_suite_test(_suite, "pipeline_free");

    all_passed = true;

    // ---- free owned pipeline (begin_copy) ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            3,
                                            sizeof(int));

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: precondition: copy pipeline owns data",
                                   pipe.owns_data == true && pipe.data != NULL,
                                   "begin_copy should own data before free");

    d_functional_pipeline_free(&pipe);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: data set to NULL after free",
                                   pipe.data == NULL,
                                   "data should be NULL");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: count set to 0 after free",
                                   pipe.count == 0,
                                   "count should be 0");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: owns_data set to false after free",
                                   pipe.owns_data == false,
                                   "owns_data should be false");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: error_code set to 0 after free",
                                   pipe.error_code == 0,
                                   "error_code should be reset");

    // ---- free non-owned pipeline (begin) ----
    pipe = d_functional_pipeline_begin(data, 3, sizeof(int));

    d_functional_pipeline_free(&pipe);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: non-owned pipeline fields reset",
                                   pipe.data == NULL && pipe.count == 0,
                                   "data=NULL, count=0 even for non-owned");

    // data[] should still be accessible (not freed)
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: original data intact after non-owned free",
                                   data[0] == 1 && data[2] == 3,
                                   "non-owned free should not touch the original buffer");

    // ---- free NULL pointer ----
    d_functional_pipeline_free(NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: NULL pointer is safe no-op",
                                   true,
                                   "calling free(NULL) should not crash");

    // ---- double-free safety ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            3,
                                            sizeof(int));
    d_functional_pipeline_free(&pipe);

    // second free: data is already NULL, owns_data is false
    d_functional_pipeline_free(&pipe);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: double-free is safe",
                                   pipe.data == NULL && pipe.owns_data == false,
                                   "freeing an already-freed pipeline should not crash");

    // ---- free error pipeline ----
    {
        struct dj_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 5;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        d_functional_pipeline_free(&err_pipe);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "free: error pipeline fields reset properly",
                                       err_pipe.error_code == 0 && err_pipe.count == 0,
                                       "error_code should be 0, count should be 0");
    }

    // ---- free after map (owns allocated buffer) ----
    {
        struct dj_functional_pipeline mapped;
        int                          src[] = { 10, 20 };

        pipe   = d_functional_pipeline_begin(src,
                                             2,
                                             sizeof(int));
        mapped = d_functional_pipeline_map(pipe,
                                           test_helper_triple_int,
                                           NULL);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "free: mapped pipeline owns data before free",
                                       mapped.owns_data == true,
                                       "precondition for free-after-map");

        d_functional_pipeline_free(&mapped);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "free: mapped pipeline properly freed",
                                       mapped.data == NULL && mapped.owns_data == false,
                                       "data=NULL, owns_data=false after free");
    }

    return all_passed;
}


/*
d_tests_pipeline_finalization_all
  Runs all pipeline finalization tests.
  Tests the following:
  - d_functional_pipeline_end
  - d_functional_pipeline_free
*/
bool d_tests_pipeline_finalization_all(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    d_test_suite_block(_suite, "pipeline_finalization_all");

    all_passed = true;

    all_passed &= d_tests_pipeline_end(_suite);
    all_passed &= d_tests_pipeline_free(_suite);

    return all_passed;
}
