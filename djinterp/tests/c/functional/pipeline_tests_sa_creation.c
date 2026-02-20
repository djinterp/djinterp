#include ".\pipeline_tests_sa.h"


/******************************************************************************
 * TEST HELPER: int transformer (doubles each element)
 *****************************************************************************/
static bool
test_helper_double_int
(
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


/*
d_tests_sa_pipeline_begin
  Tests d_functional_pipeline_begin for wrapping mutable data.
  Tests the following:
  - valid data, count, and element_size produces a valid pipeline
  - pipeline does NOT own the data (owns_data == false)
  - pipeline stores correct data pointer, count, and element_size
  - error_code is 0 on success
  - NULL data returns error pipeline (error_code == -1)
  - zero count returns error pipeline
  - zero element_size returns error pipeline
  - error pipeline has NULL data, zero count, owns_data == false
*/
bool
d_tests_sa_pipeline_begin
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    int                          data[] = { 10, 20, 30, 40, 50 };
    bool                         all_passed;

    all_passed = true;

    // ---- valid parameters ----
    pipe = d_functional_pipeline_begin(data,
                                       5,
                                       sizeof(int));

    all_passed &= d_assert_standalone(
        pipe.error_code == 0,
        "begin: valid params produce error_code 0",
        "error_code should be 0 for valid input",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.data == data,
        "begin: data pointer matches input",
        "pipeline should wrap the original pointer",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.count == 5,
        "begin: count matches input",
        "count should be 5",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.element_size == sizeof(int),
        "begin: element_size matches input",
        "element_size should be sizeof(int)",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.owns_data == false,
        "begin: pipeline does NOT own the data",
        "owns_data should be false for begin (no copy)",
        _test_info);

    // ---- single element ----
    pipe = d_functional_pipeline_begin(data,
                                       1,
                                       sizeof(int));

    all_passed &= d_assert_standalone(
        pipe.error_code == 0 && pipe.count == 1,
        "begin: single element is valid",
        "a 1-element pipeline should succeed",
        _test_info);

    // ---- NULL data ----
    pipe = d_functional_pipeline_begin(NULL,
                                       5,
                                       sizeof(int));

    all_passed &= d_assert_standalone(
        pipe.error_code == -1,
        "begin: NULL data returns error",
        "error_code should be -1 for NULL data",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.data == NULL,
        "begin: NULL data error has NULL data ptr",
        "data should be NULL on error",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.owns_data == false,
        "begin: NULL data error does not own data",
        "owns_data should be false on error",
        _test_info);

    // ---- zero count ----
    pipe = d_functional_pipeline_begin(data,
                                       0,
                                       sizeof(int));

    all_passed &= d_assert_standalone(
        pipe.error_code == -1,
        "begin: zero count returns error",
        "error_code should be -1 for zero count",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.count == 0,
        "begin: zero count error has count 0",
        "count should be 0 on error",
        _test_info);

    // ---- zero element_size ----
    pipe = d_functional_pipeline_begin(data,
                                       5,
                                       0);

    all_passed &= d_assert_standalone(
        pipe.error_code == -1,
        "begin: zero element_size returns error",
        "error_code should be -1 for zero element_size",
        _test_info);

    // ---- all invalid simultaneously ----
    pipe = d_functional_pipeline_begin(NULL,
                                       0,
                                       0);

    all_passed &= d_assert_standalone(
        pipe.error_code == -1,
        "begin: all params invalid returns error",
        "error_code should be -1 when all params invalid",
        _test_info);

    return all_passed;
}


/*
d_tests_sa_pipeline_begin_copy
  Tests d_functional_pipeline_begin_copy for copying immutable data.
  Tests the following:
  - valid data, count, element_size produces a valid pipeline
  - pipeline OWNS the data (owns_data == true)
  - copied data matches original contents
  - copied data is at a DIFFERENT address than the original
  - error_code is 0 on success
  - NULL data returns error pipeline
  - zero count returns error pipeline
  - zero element_size returns error pipeline
  - error pipeline fields are properly zeroed
  - modifications to the copy do not affect the original
*/
bool
d_tests_sa_pipeline_begin_copy
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    const int                    data[] = { 100, 200, 300 };
    int*                         copied;
    bool                         all_passed;
    bool                         contents_match;
    size_t                       i;

    all_passed = true;

    // ---- valid parameters ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            3,
                                            sizeof(int));

    all_passed &= d_assert_standalone(
        pipe.error_code == 0,
        "begin_copy: valid params produce error_code 0",
        "error_code should be 0 for valid input",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.owns_data == true,
        "begin_copy: pipeline OWNS the data",
        "owns_data should be true (data was copied)",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.data != NULL,
        "begin_copy: data is non-NULL",
        "copied data pointer should not be NULL",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.data != (void*)data,
        "begin_copy: data is at a DIFFERENT address",
        "copy should not alias the original",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.count == 3,
        "begin_copy: count matches input",
        "count should be 3",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.element_size == sizeof(int),
        "begin_copy: element_size matches input",
        "element_size should be sizeof(int)",
        _test_info);

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

    all_passed &= d_assert_standalone(
        contents_match,
        "begin_copy: copied contents match original",
        "each element should be identical to the source",
        _test_info);

    // verify independence: modify copy, original unchanged
    copied[0] = 999;

    all_passed &= d_assert_standalone(
        data[0] == 100,
        "begin_copy: modifying copy does not affect original",
        "original data[0] should still be 100",
        _test_info);

    // clean up owned data
    d_functional_pipeline_free(&pipe);

    // ---- single element copy ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            1,
                                            sizeof(int));

    all_passed &= d_assert_standalone(
        pipe.error_code == 0 && pipe.count == 1,
        "begin_copy: single element copy succeeds",
        "a 1-element copied pipeline should succeed",
        _test_info);

    d_functional_pipeline_free(&pipe);

    // ---- NULL data ----
    pipe = d_functional_pipeline_begin_copy(NULL,
                                            3,
                                            sizeof(int));

    all_passed &= d_assert_standalone(
        pipe.error_code == -1,
        "begin_copy: NULL data returns error",
        "error_code should be -1 for NULL data",
        _test_info);

    all_passed &= d_assert_standalone(
        pipe.data == NULL && pipe.owns_data == false,
        "begin_copy: NULL data error fields are clean",
        "data should be NULL and owns_data false on error",
        _test_info);

    // ---- zero count ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            0,
                                            sizeof(int));

    all_passed &= d_assert_standalone(
        pipe.error_code == -1,
        "begin_copy: zero count returns error",
        "error_code should be -1 for zero count",
        _test_info);

    // ---- zero element_size ----
    pipe = d_functional_pipeline_begin_copy(data,
                                            3,
                                            0);

    all_passed &= d_assert_standalone(
        pipe.error_code == -1,
        "begin_copy: zero element_size returns error",
        "error_code should be -1 for zero element_size",
        _test_info);

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

        all_passed &= d_assert_standalone(
            pipe.error_code == 0,
            "begin_copy: large struct elements succeed",
            "copying structs should work",
            _test_info);

        pts_copy = (struct test_large*)pipe.data;

        all_passed &= d_assert_standalone(
            pts_copy[0].x == 1.0 && pts_copy[1].z == 6.0,
            "begin_copy: large struct contents correct",
            "struct field values should match originals",
            _test_info);

        d_functional_pipeline_free(&pipe);
    }

    return all_passed;
}


/*
d_tests_sa_pipeline_creation_all
  Runs all pipeline creation tests.
  Tests the following:
  - d_functional_pipeline_begin
  - d_functional_pipeline_begin_copy
*/
bool
d_tests_sa_pipeline_creation_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_pipeline_begin(_test_info);
    all_passed &= d_tests_sa_pipeline_begin_copy(_test_info);

    return all_passed;
}
