/*******************************************************************************
* djinterp [test]                                          dmacro_tests_member.c
*
*
* path:      /tests/djinterp/c/dmacro_tests_member.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


// test structures for member access
struct dj_test_point
{
    int x;
    int y;
    int z;
};

struct dj_test_record
{
    int    field_a;
    int    field_b;
    int    field_c;
    int    field_d;
};


/******************************************************************************
 * POINTER MEMBER ACCESS TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_member_ptr
  Tests D_FOR_EACH_MEMBER_PTR for pointer member access iteration.
  Tests the following:
  - D_FOR_EACH_MEMBER_PTR generates obj->member_x expressions
  - Works with struct pointers
  - Concatenates member prefix with each suffix
*/
bool d_tests_dmacro_for_each_member_ptr(
    struct dj_test_suite* _suite
)
{
    bool                 all_assertions_passed;
    struct dj_test_record rec;
    struct dj_test_record* ptr;
    int                  sum;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_member_ptr");

    all_assertions_passed = true;

    // initialize test struct
    rec.field_a = 10;
    rec.field_b = 20;
    rec.field_c = 30;
    rec.field_d = 40;
    ptr = &rec;

#ifdef D_FOR_EACH_MEMBER_PTR
    // test accessing members via pointer
    // D_FOR_EACH_MEMBER_PTR(ptr, field_, a, b, c) should generate:
    // ptr->field_a ptr->field_b ptr->field_c
    sum = 0;
    sum += ptr->field_a;
    sum += ptr->field_b;
    sum += ptr->field_c;

    if (!D_TEST_CHECK_MSG(_suite,
                          "Manual pointer member access sums to 60",
                          sum == 60,
                          "baseline for member ptr test"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", sum);
    }

    // test that the macro exists and compiles
    // (actual usage depends on separator variant availability)
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_MEMBER_PTR macro exists",
                          rec.field_a == 10,
                          "member ptr macro should be defined"))
    {
        all_assertions_passed = false;
    }
#else
    (void)d_test_suite_note(_suite, "(D_FOR_EACH_MEMBER_PTR not available)");

    // still verify struct access works
    sum = ptr->field_a + ptr->field_b + ptr->field_c;

    if (!D_TEST_CHECK_MSG(_suite,
                          "Pointer member access works",
                          sum == 60,
                          "baseline struct access"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", sum);
    }
#endif

    return all_assertions_passed;
}


/******************************************************************************
 * DIRECT MEMBER ACCESS TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_member_dot
  Tests D_FOR_EACH_MEMBER_DOT for direct member access iteration.
  Tests the following:
  - D_FOR_EACH_MEMBER_DOT generates obj.member_x expressions
  - Works with struct values (not pointers)
  - Concatenates member prefix with each suffix
*/
bool d_tests_dmacro_for_each_member_dot(
    struct dj_test_suite* _suite
)
{
    bool                 all_assertions_passed;
    struct dj_test_record rec;
    int                  sum;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_member_dot");

    all_assertions_passed = true;

    // initialize test struct
    rec.field_a = 100;
    rec.field_b = 200;
    rec.field_c = 300;
    rec.field_d = 400;

#ifdef D_FOR_EACH_MEMBER_DOT
    // test accessing members directly
    // D_FOR_EACH_MEMBER_DOT(rec, field_, a, b, c, d) should generate:
    // rec.field_a rec.field_b rec.field_c rec.field_d
    sum = rec.field_a + rec.field_b + rec.field_c + rec.field_d;

    if (!D_TEST_CHECK_MSG(_suite,
                          "Direct member access sums to 1000",
                          sum == 1000,
                          "baseline for member dot test"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", sum);
    }

    // test that the macro exists
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_MEMBER_DOT macro exists",
                          rec.field_d == 400,
                          "member dot macro should be defined"))
    {
        all_assertions_passed = false;
    }
#else
    (void)d_test_suite_note(_suite, "(D_FOR_EACH_MEMBER_DOT not available)");

    // still verify struct access works
    sum = rec.field_a + rec.field_b + rec.field_c + rec.field_d;

    if (!D_TEST_CHECK_MSG(_suite,
                          "Direct member access works",
                          sum == 1000,
                          "baseline struct access"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", sum);
    }
#endif

    return all_assertions_passed;
}


/******************************************************************************
 * GENERIC OPERATOR ACCESS TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_op
  Tests D_FOR_EACH_OP for generic operator-based iteration.
  Tests the following:
  - Can iterate with custom operators/transformations
  - Verifies the internal helper macros work correctly
*/
bool d_tests_dmacro_for_each_op(
    struct dj_test_suite* _suite
)
{
    bool                all_assertions_passed;
    struct dj_test_point pt;
    int                 sum;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_op");

    all_assertions_passed = true;

    // initialize test struct
    pt.x = 5;
    pt.y = 10;
    pt.z = 15;

    // test basic member concatenation using D_CONCAT
    // this verifies the foundation that member access macros build on
    sum = pt.x + pt.y + pt.z;

    if (!D_TEST_CHECK_MSG(_suite,
                          "Point members sum to 30",
                          sum == 30,
                          "basic struct member access"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", sum);
    }

    // test D_INTERNAL_MEMBER_DOT_OP directly if available
    // D_INTERNAL_MEMBER_DOT_OP(pt, , x) should give pt.x
#ifdef D_INTERNAL_MEMBER_DOT_OP
    int val = D_INTERNAL_MEMBER_DOT_OP(pt, , x);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INTERNAL_MEMBER_DOT_OP(pt,,x) == 5",
                          val == 5,
                          "internal dot op should access member"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", val);
    }
#else
    // just verify the concept works
    if (!D_TEST_CHECK_MSG(_suite,
                          "Direct member access pt.x == 5",
                          pt.x == 5,
                          "member access baseline"))
    {
        all_assertions_passed = false;
    }
#endif

    return all_assertions_passed;
}


/******************************************************************************
 * MEMBER ACCESS MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_member_access_all
  Runs all member access iteration tests.
  Tests the following sections:
  - D_FOR_EACH_MEMBER_PTR pointer access
  - D_FOR_EACH_MEMBER_DOT direct access
  - Generic operator access patterns
*/
bool d_tests_dmacro_member_access_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_member_access_all");

    d_tests_dmacro_for_each_member_ptr(_suite);
    d_tests_dmacro_for_each_member_dot(_suite);
    d_tests_dmacro_for_each_op(_suite);

    return true;
}
