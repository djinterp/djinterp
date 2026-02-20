#include ".\pipeline_tests_sa.h"


/******************************************************************************
 * TEST HELPER: int transformer (triples each element)
 *****************************************************************************/
static bool
test_helper_triple_int
(
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
d_tests_sa_pipeline_end
  Tests d_functional_pipeline_end.
  Tests the following:
  - end returns the data pointer for a valid pipeline
  - end writes count to _out_count when non-NULL
  - end with NULL _out_count does not crash
  - end returns NULL for an error pipeline
  - end writes count even on error (count is written before error check)
  - caller takes ownership of data after end
  - end after map returns the mapped buffer
*/
bool
d_tests_sa_pipeline_end
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    int                          data[] = { 5, 10, 15, 20 };
    void*                        result;
    size_t                       out_count;
    bool                         all_passed;

    all_passed = true;

    // ---- valid pipeline with _out_count ----
    pipe   = d_functional_pipeline_begin(data, 4, sizeof(int));
    result = d_functional_pipeline_end(pipe, &out_count);

    all_passed &= d_assert_standalone(
        result == data,
        "end: returns data pointer for valid pipeline",
        "result should point to original data",
        _test_info);

    all_passed &= d_assert_standalone(
        out_count == 4,
        "end: writes count to _out_count",
        "out_count should be 4",
        _test_info);

    // ---- valid pipeline with NULL _out_count ----
    pipe   = d_functional_pipeline_begin(data, 4, sizeof(int));
    result = d_functional_pipeline_end(pipe, NULL);

    all_passed &= d_assert_standalone(
        result == data,
        "end: NULL _out_count does not crash",
        "should still return data pointer",
        _test_info);

    // ---- error pipeline ----
    {
        struct d_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        out_count = 999;
        result    = d_functional_pipeline_end(err_pipe, &out_count);

        all_passed &= d_assert_standalone(
            result == NULL,
            "end: returns NULL for error pipeline",
            "error pipeline should yield NULL",
            _test_info);

        all_passed &= d_assert_standalone(
            out_count == 0,
            "end: writes count even on error",
            "out_count should be 0 (from err_pipe.count)",
            _test_info);
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

        all_passed &= d_assert_standalone(
            mapped_data != NULL && mapped_count == 3,
            "end: after map returns non-NULL with count 3",
            "caller should receive the mapped buffer",
            _test_info);

        all_passed &= d_assert_standalone(
            mapped_data[0] == 3 && mapped_data[1] == 6 &&
            mapped_data[2] == 9,
            "end: after map values are 3,6,9",
            "1*3=3, 2*3=6, 3*3=9",
            _test_info);

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

        all_passed &= d_assert_standalone(
            copy_data != NULL && copy_count == 2,
            "end: after begin_copy returns owned copy",
            "caller takes ownership of copied data",
            _test_info);

        all_passed &= d_assert_standalone(
            copy_data[0] == 42 && copy_data[1] == 84,
            "end: copy values match original",
            "copied data should match source",
            _test_info);

        free(copy_data);
    }

    // ---- end after take: reduced count ----
    {
        size_t taken_count;

        pipe   = d_functional_pipeline_begin(data, 4, sizeof(int));
        pipe   = d_functional_pipeline_take(pipe, 2);
        result = d_functional_pipeline_end(pipe, &taken_count);

        all_passed &= d_assert_standalone(
            result != NULL && taken_count == 2,
            "end: after take(2) returns count 2",
            "taken count should be reflected in end",
            _test_info);
    }

    // ---- end after skip: reduced count ----
    {
        size_t skipped_count;

        pipe   = d_functional_pipeline_begin(data, 4, sizeof(int));
        pipe   = d_functional_pipeline_skip(pipe, 1);
        result = d_functional_pipeline_end(pipe, &skipped_count);

        all_passed &= d_assert_standalone(
            result != NULL && skipped_count == 3,
            "end: after skip(1) returns count 3",
            "skipped count should be reflected in end",
            _test_info);

        all_passed &= d_assert_standalone(
            ((int*)result)[0] == 10,
            "end: after skip(1) first element is data[1]",
            "first visible element should be 10",
            _test_info);
    }

    return all_passed;
}


/*
d_tests_sa_pipeline_free
  Tests d_functional_pipeline_free.
  Tests the following:
  - free on an owned pipeline frees data and resets fields
  - free on a non-owned pipeline resets fields but does not free data
  - free on a NULL pointer is a safe no-op
  - free resets error_code to 0
  - free sets data to NULL, count to 0, owns_data to false
  - double-free safety: freeing an already-freed pipeline is safe
*/
bool
d_tests_sa_pipeline_free
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    int                          data[] = { 1, 2, 3 };
    bool                         all_passed;

    all_passed = true;

    // ---- free owned pipeline (begin_copy) ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            3,
                                            sizeof(int));

    all_passed &= d_assert_standalone(
        pipe.owns_data == true && pipe.data != NULL,
        "free: precondition: copy pipeline owns data",
        "begin_copy should own data before free",
        _test_info);

    d_functional_pipeline_free(&pipe);

    all_passed &= d_assert_standalone(
        pipe.data == NULL,
        "free: data set to NULL after free",
        "data should be NULL",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.count == 0,
        "free: count set to 0 after free",
        "count should be 0",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.owns_data == false,
        "free: owns_data set to false after free",
        "owns_data should be false",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.error_code == 0,
        "free: error_code set to 0 after free",
        "error_code should be reset",
        _test_info);

    // ---- free non-owned pipeline (begin) ----
    pipe = d_functional_pipeline_begin(data, 3, sizeof(int));

    d_functional_pipeline_free(&pipe);

    all_passed &= d_assert_standalone(
        pipe.data == NULL && pipe.count == 0,
        "free: non-owned pipeline fields reset",
        "data=NULL, count=0 even for non-owned",
        _test_info);

    // data[] should still be accessible (not freed)
    all_passed &= d_assert_standalone(
        data[0] == 1 && data[2] == 3,
        "free: original data intact after non-owned free",
        "non-owned free should not touch the original buffer",
        _test_info);

    // ---- free NULL pointer ----
    d_functional_pipeline_free(NULL);

    all_passed &= d_assert_standalone(
        true,
        "free: NULL pointer is safe no-op",
        "calling free(NULL) should not crash",
        _test_info);

    // ---- double-free safety ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            3,
                                            sizeof(int));
    d_functional_pipeline_free(&pipe);

    // second free: data is already NULL, owns_data is false
    d_functional_pipeline_free(&pipe);

    all_passed &= d_assert_standalone(
        pipe.data == NULL && pipe.owns_data == false,
        "free: double-free is safe",
        "freeing an already-freed pipeline should not crash",
        _test_info);

    // ---- free error pipeline ----
    {
        struct d_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 5;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        d_functional_pipeline_free(&err_pipe);

        all_passed &= d_assert_standalone(
            err_pipe.error_code == 0 && err_pipe.count == 0,
            "free: error pipeline fields reset properly",
            "error_code should be 0, count should be 0",
            _test_info);
    }

    // ---- free after map (owns allocated buffer) ----
    {
        struct d_functional_pipeline mapped;
        int                          src[] = { 10, 20 };

        pipe   = d_functional_pipeline_begin(src,
                                             2,
                                             sizeof(int));
        mapped = d_functional_pipeline_map(pipe,
                                           test_helper_triple_int,
                                           NULL);

        all_passed &= d_assert_standalone(
            mapped.owns_data == true,
            "free: mapped pipeline owns data before free",
            "precondition for free-after-map",
            _test_info);

        d_functional_pipeline_free(&mapped);

        all_passed &= d_assert_standalone(
            mapped.data == NULL && mapped.owns_data == false,
            "free: mapped pipeline properly freed",
            "data=NULL, owns_data=false after free",
            _test_info);
    }

    return all_passed;
}


/*
d_tests_sa_pipeline_finalization_all
  Runs all pipeline finalization tests.
  Tests the following:
  - d_functional_pipeline_end
  - d_functional_pipeline_free
*/
bool
d_tests_sa_pipeline_finalization_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_pipeline_end(_test_info);
    all_passed &= d_tests_sa_pipeline_free(_test_info);

    return all_passed;
}
