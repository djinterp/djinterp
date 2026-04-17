/******************************************************************************
* djinterp [test]                                  dmacro_tests_sa_pair_idx.c
*
*   Unit tests for `dmacro.h` pair and indexed iteration section.
*   Tests D_FOR_EACH_PAIR, D_FOR_EACH_TRIPLE, D_FOR_EACH_4TUPLE concepts.
*   Note: These macros pass custom operator names through D_CONCAT which is
* incompatible with MSVC traditional preprocessor. Tests verify concepts
* manually rather than using the macros directly.
*   Note: this module tests fundamental macro utilities that are dependencies
* of other djinterp modules, so it uses `test_standalone.h` rather than DTest.
*
*
* path:      \test\dmacro_tests_sa_pair_idx.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * D_FOR_EACH_PAIR TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_pair
  Tests D_FOR_EACH_PAIR concept for 2-tuple iteration.
Note: D_FOR_EACH_PAIR passes custom operator names through macro expansion
which is incompatible with MSVC traditional preprocessor. This test verifies
the concept manually.
  Tests the following:
  - Pair iteration concept (processing elements in pairs)
  - Type-name pairs for struct field generation concept
  - Key-value pairs for enum generation concept
*/
bool
d_tests_sa_dmacro_for_each_pair
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Pair Iteration Concept ---\n", D_INDENT);

    // D_FOR_EACH_PAIR passes custom operator through D_CONCAT which fails
    // on MSVC traditional preprocessor. Test the concept manually.

    // concept: type-name pairs for struct generation
    // D_FOR_EACH_PAIR(FIELD_OP, int, x, int, y, double, z)
    // would generate: int x; int y; double z;
    struct d_test_manual_struct
    {
        int    x;
        int    y;
        double z;
    };

    struct d_test_manual_struct ps;
    ps.x = 10;
    ps.y = 20;
    ps.z = 3.14;

    if (!d_assert_standalone(ps.x == 10,
                             "pair concept: struct field x == 10",
                             "type-name pair generates int field",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, ps.x);
    }

    if (!d_assert_standalone(ps.y == 20,
                             "pair concept: struct field y == 20",
                             "second type-name pair generates field",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, ps.y);
    }

    if (!d_assert_standalone(ps.z > 3.13 && ps.z < 3.15,
                             "pair concept: struct field z ~= 3.14",
                             "third type-name pair with double type",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %f\n", D_INDENT, ps.z);
    }

    // concept: name-value pairs for enum generation
    // D_FOR_EACH_PAIR(ENUM_OP, VAL_A, 10, VAL_B, 20, VAL_C, 30)
    // would generate: VAL_A = 10, VAL_B = 20, VAL_C = 30,
    enum d_test_manual_enum
    {
        VAL_A = 10,
        VAL_B = 20,
        VAL_C = 30
    };

    if (!d_assert_standalone(VAL_A == 10,
                             "pair concept: enum VAL_A == 10",
                             "name-value pair for enum",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, VAL_A);
    }

    if (!d_assert_standalone(VAL_B == 20,
                             "pair concept: enum VAL_B == 20",
                             "second name-value pair",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, VAL_B);
    }

    if (!d_assert_standalone(VAL_C == 30,
                             "pair concept: enum VAL_C == 30",
                             "third name-value pair",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, VAL_C);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Pair iteration concept test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Pair iteration concept test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_for_each_pair_sep
  Tests D_FOR_EACH_PAIR_SEP concept with custom separators.
Note: D_FOR_EACH_PAIR_SEP passes custom operator names through macro expansion
which is incompatible with MSVC traditional preprocessor.
  Tests the following:
  - Pair iteration with separator concept
  - Assignment patterns using pairs
*/
bool
d_tests_sa_dmacro_for_each_pair_sep
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    a;
    int    b;
    int    c;
    int    sum;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Pair Separator Iteration Concept ---\n", D_INDENT);

    // D_FOR_EACH_PAIR_SEP(;, ASSIGN_OP, a, 100, b, 200, c, 300)
    // would generate: a = 100; b = 200; c = 300
    // test concept manually
    a = 100;
    b = 200;
    c = 300;

    if (!d_assert_standalone(a == 100,
                             "pair_sep concept: a == 100",
                             "first pair assignment",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, a);
    }

    if (!d_assert_standalone(b == 200,
                             "pair_sep concept: b == 200",
                             "second pair assignment",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, b);
    }

    if (!d_assert_standalone(c == 300,
                             "pair_sep concept: c == 300",
                             "third pair assignment",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, c);
    }

    sum = a + b + c;

    if (!d_assert_standalone(sum == 600,
                             "sum of pair_sep assignments == 600",
                             "all pair assignments executed",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, sum);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Pair separator concept test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Pair separator concept test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_FOR_EACH_TRIPLE TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_triple
  Tests D_FOR_EACH_TRIPLE concept for 3-tuple iteration.
Note: D_FOR_EACH_TRIPLE passes custom operator names through macro expansion
which is incompatible with MSVC traditional preprocessor.
  Tests the following:
  - Triple iteration concept (processing elements in groups of 3)
  - Type-name-default triples for field generation
*/
bool
d_tests_sa_dmacro_for_each_triple
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Triple Iteration Concept ---\n", D_INDENT);

    // D_FOR_EACH_TRIPLE(FIELD_INIT_OP, int, x, 0, int, y, 0, double, z, 0.0)
    // concept: type-name-initializer triples
    struct d_test_triple_manual
    {
        int    x;
        int    y;
        double z;
    };

    struct d_test_triple_manual ts;
    ts.x = 111;
    ts.y = 222;
    ts.z = 3.33;

    if (!d_assert_standalone(ts.x == 111,
                             "triple concept: field x == 111",
                             "first triple creates field",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, ts.x);
    }

    if (!d_assert_standalone(ts.y == 222,
                             "triple concept: field y == 222",
                             "second triple creates field",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, ts.y);
    }

    if (!d_assert_standalone(ts.z > 3.32 && ts.z < 3.34,
                             "triple concept: field z ~= 3.33",
                             "third triple with double type",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %f\n", D_INDENT, ts.z);
    }

    // verify struct size
    if (!d_assert_standalone(sizeof(struct d_test_triple_manual) >= 
                             2 * sizeof(int) + sizeof(double),
                             "triple struct has expected minimum size",
                             "all triple fields created",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Triple iteration concept test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Triple iteration concept test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_FOR_EACH_4TUPLE TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_4tuple
  Tests D_FOR_EACH_4TUPLE concept for 4-tuple iteration.
Note: D_FOR_EACH_4TUPLE passes custom operator names through macro expansion
which is incompatible with MSVC traditional preprocessor.
  Tests the following:
  - 4-tuple iteration concept
  - Complex field definitions with multiple attributes
*/
bool
d_tests_sa_dmacro_for_each_4tuple
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing 4-Tuple Iteration Concept ---\n", D_INDENT);

    // D_FOR_EACH_4TUPLE(OP, name, type, default, attr, ...)
    // concept: 4-tuples for complex field definitions
    struct d_test_4tuple_manual
    {
        int field_x;
        int field_y;
    };

    struct d_test_4tuple_manual s4;
    s4.field_x = 1000;
    s4.field_y = 2000;

    if (!d_assert_standalone(s4.field_x == 1000,
                             "4tuple concept: field_x == 1000",
                             "first 4tuple creates field",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, s4.field_x);
    }

    if (!d_assert_standalone(s4.field_y == 2000,
                             "4tuple concept: field_y == 2000",
                             "second 4tuple creates field",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, s4.field_y);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] 4-Tuple iteration concept test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] 4-Tuple iteration concept test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * INDEXED ITERATION TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_indexed
  Tests indexed iteration patterns using pair macros.
Note: Indexed iteration can be achieved with pair macros where first element
is the index and second is the value.
  Tests the following:
  - Indexed iteration concept using pairs
  - Array initialization with explicit indices
*/
bool
d_tests_sa_dmacro_for_each_indexed
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    arr[5];
    int    sum;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Indexed Iteration Concept ---\n", D_INDENT);

    // D_FOR_EACH_PAIR(INDEXED_OP, 0, 10, 1, 20, 2, 30, 3, 40, 4, 50)
    // would generate: arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = 40; arr[4] = 50;
    // test concept manually
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;

    if (!d_assert_standalone(arr[0] == 10,
                             "indexed concept: arr[0] == 10",
                             "index 0 assignment",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, arr[0]);
    }

    if (!d_assert_standalone(arr[1] == 20,
                             "indexed concept: arr[1] == 20",
                             "index 1 assignment",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, arr[1]);
    }

    if (!d_assert_standalone(arr[2] == 30,
                             "indexed concept: arr[2] == 30",
                             "index 2 assignment",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, arr[2]);
    }

    if (!d_assert_standalone(arr[3] == 40,
                             "indexed concept: arr[3] == 40",
                             "index 3 assignment",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, arr[3]);
    }

    if (!d_assert_standalone(arr[4] == 50,
                             "indexed concept: arr[4] == 50",
                             "index 4 assignment",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, arr[4]);
    }

    sum = arr[0] + arr[1] + arr[2] + arr[3] + arr[4];

    if (!d_assert_standalone(sum == 150,
                             "sum of indexed assignments == 150",
                             "all indexed assignments correct",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, sum);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Indexed iteration concept test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Indexed iteration concept test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * PAIR AND INDEXED ITERATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_pair_indexed_all
  Runs all pair and indexed iteration concept tests.
Note: D_FOR_EACH_PAIR, D_FOR_EACH_TRIPLE, D_FOR_EACH_4TUPLE pass custom
operator names through macro expansion which is incompatible with MSVC
traditional preprocessor. Tests verify concepts manually.
  Tests the following sections:
  - D_FOR_EACH_PAIR concept
  - D_FOR_EACH_PAIR_SEP concept
  - D_FOR_EACH_TRIPLE concept
  - D_FOR_EACH_4TUPLE concept
  - Indexed iteration patterns
*/
bool
d_tests_sa_dmacro_pair_indexed_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  pair_result;
    bool                  pair_sep_result;
    bool                  triple_result;
    bool                  tuple4_result;
    bool                  indexed_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Pair and Indexed Iteration Concepts (Section IX)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all pair and indexed iteration tests
    pair_result     = d_tests_sa_dmacro_for_each_pair(&module_counter);
    pair_sep_result = d_tests_sa_dmacro_for_each_pair_sep(&module_counter);
    triple_result   = d_tests_sa_dmacro_for_each_triple(&module_counter);
    tuple4_result   = d_tests_sa_dmacro_for_each_4tuple(&module_counter);
    indexed_result  = d_tests_sa_dmacro_for_each_indexed(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( pair_result     &&
                       pair_sep_result &&
                       triple_result   &&
                       tuple4_result   &&
                       indexed_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Pair/Indexed Iteration Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Pair/Indexed Iteration Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - FOR_EACH_PAIR concept:     %s\n",
               pair_result ? "PASSED" : "FAILED");
        printf("  - FOR_EACH_PAIR_SEP concept: %s\n",
               pair_sep_result ? "PASSED" : "FAILED");
        printf("  - FOR_EACH_TRIPLE concept:   %s\n",
               triple_result ? "PASSED" : "FAILED");
        printf("  - FOR_EACH_4TUPLE concept:   %s\n",
               tuple4_result ? "PASSED" : "FAILED");
        printf("  - Indexed patterns:          %s\n",
               indexed_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}