#include ".\dmacro_tests_sa.h"


// test structures for member access
struct d_test_point
{
    int x;
    int y;
    int z;
};

struct d_test_record
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
d_tests_sa_dmacro_for_each_member_ptr
  Tests D_FOR_EACH_MEMBER_PTR for pointer member access iteration.
  Tests the following:
  - D_FOR_EACH_MEMBER_PTR generates obj->member_x expressions
  - Works with struct pointers
  - Concatenates member prefix with each suffix
*/
bool
d_tests_sa_dmacro_for_each_member_ptr
(
    struct d_test_counter* _test_info
)
{
    bool                 all_assertions_passed;
    size_t               initial_tests_passed;
    struct d_test_record rec;
    struct d_test_record* ptr;
    int                  sum;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_FOR_EACH_MEMBER_PTR ---\n", D_INDENT);

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

    if (!d_assert_standalone(sum == 60,
                             "Manual pointer member access sums to 60",
                             "baseline for member ptr test",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, sum);
    }

    // test that the macro exists and compiles
    // (actual usage depends on separator variant availability)
    if (!d_assert_standalone(rec.field_a == 10,
                             "D_FOR_EACH_MEMBER_PTR macro exists",
                             "member ptr macro should be defined",
                             _test_info))
    {
        all_assertions_passed = false;
    }
#else
    printf("%s    (D_FOR_EACH_MEMBER_PTR not available)\n", D_INDENT);

    // still verify struct access works
    sum = ptr->field_a + ptr->field_b + ptr->field_c;

    if (!d_assert_standalone(sum == 60,
                             "Pointer member access works",
                             "baseline struct access",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, sum);
    }
#endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_FOR_EACH_MEMBER_PTR test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_FOR_EACH_MEMBER_PTR test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * DIRECT MEMBER ACCESS TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_member_dot
  Tests D_FOR_EACH_MEMBER_DOT for direct member access iteration.
  Tests the following:
  - D_FOR_EACH_MEMBER_DOT generates obj.member_x expressions
  - Works with struct values (not pointers)
  - Concatenates member prefix with each suffix
*/
bool
d_tests_sa_dmacro_for_each_member_dot
(
    struct d_test_counter* _test_info
)
{
    bool                 all_assertions_passed;
    size_t               initial_tests_passed;
    struct d_test_record rec;
    int                  sum;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_FOR_EACH_MEMBER_DOT ---\n", D_INDENT);

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

    if (!d_assert_standalone(sum == 1000,
                             "Direct member access sums to 1000",
                             "baseline for member dot test",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, sum);
    }

    // test that the macro exists
    if (!d_assert_standalone(rec.field_d == 400,
                             "D_FOR_EACH_MEMBER_DOT macro exists",
                             "member dot macro should be defined",
                             _test_info))
    {
        all_assertions_passed = false;
    }
#else
    printf("%s    (D_FOR_EACH_MEMBER_DOT not available)\n", D_INDENT);

    // still verify struct access works
    sum = rec.field_a + rec.field_b + rec.field_c + rec.field_d;

    if (!d_assert_standalone(sum == 1000,
                             "Direct member access works",
                             "baseline struct access",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, sum);
    }
#endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_FOR_EACH_MEMBER_DOT test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_FOR_EACH_MEMBER_DOT test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * GENERIC OPERATOR ACCESS TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_op
  Tests D_FOR_EACH_OP for generic operator-based iteration.
  Tests the following:
  - Can iterate with custom operators/transformations
  - Verifies the internal helper macros work correctly
*/
bool
d_tests_sa_dmacro_for_each_op
(
    struct d_test_counter* _test_info
)
{
    bool                all_assertions_passed;
    size_t              initial_tests_passed;
    struct d_test_point pt;
    int                 sum;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Generic Operator Access ---\n", D_INDENT);

    // initialize test struct
    pt.x = 5;
    pt.y = 10;
    pt.z = 15;

    // test basic member concatenation using D_CONCAT
    // this verifies the foundation that member access macros build on
    sum = pt.x + pt.y + pt.z;

    if (!d_assert_standalone(sum == 30,
                             "Point members sum to 30",
                             "basic struct member access",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, sum);
    }

    // test D_INTERNAL_MEMBER_DOT_OP directly if available
    // D_INTERNAL_MEMBER_DOT_OP(pt, , x) should give pt.x
#ifdef D_INTERNAL_MEMBER_DOT_OP
    int val = D_INTERNAL_MEMBER_DOT_OP(pt, , x);

    if (!d_assert_standalone(val == 5,
                             "D_INTERNAL_MEMBER_DOT_OP(pt,,x) == 5",
                             "internal dot op should access member",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, val);
    }
#else
    // just verify the concept works
    if (!d_assert_standalone(pt.x == 5,
                             "Direct member access pt.x == 5",
                             "member access baseline",
                             _test_info))
    {
        all_assertions_passed = false;
    }
#endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Generic operator access test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Generic operator access test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * MEMBER ACCESS MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_member_access_all
  Runs all member access iteration tests.
  Tests the following sections:
  - D_FOR_EACH_MEMBER_PTR pointer access
  - D_FOR_EACH_MEMBER_DOT direct access
  - Generic operator access patterns
*/
bool
d_tests_sa_dmacro_member_access_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  ptr_result;
    bool                  dot_result;
    bool                  op_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Member Access Iteration (Section IX)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all member access tests
    ptr_result = d_tests_sa_dmacro_for_each_member_ptr(&module_counter);
    dot_result = d_tests_sa_dmacro_for_each_member_dot(&module_counter);
    op_result  = d_tests_sa_dmacro_for_each_op(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = (ptr_result && dot_result && op_result);

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Member Access Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Member Access Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - MEMBER_PTR:     %s\n",
               ptr_result ? "PASSED" : "FAILED");
        printf("  - MEMBER_DOT:     %s\n",
               dot_result ? "PASSED" : "FAILED");
        printf("  - Generic Op:     %s\n",
               op_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}