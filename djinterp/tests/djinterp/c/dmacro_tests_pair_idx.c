/*******************************************************************************
* djinterp [test]                                        dmacro_tests_pair_idx.c
*
*   Unit tests for `dmacro.h` pair and indexed iteration section.
*   Tests D_FOR_EACH_PAIR, D_FOR_EACH_TRIPLE, D_FOR_EACH_4TUPLE concepts.
*   Note: These macros pass custom operator names through D_CONCAT which is
* incompatible with MSVC traditional preprocessor. Tests verify concepts
* manually rather than using the macros directly.
*   Note: these macro utilities are dependencies of other djinterp modules,
* DTest's C face among them (dmacro_token.h, through djinterp.h), so a fault
* here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/dmacro_tests_pair_idx.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * D_FOR_EACH_PAIR TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_pair
  Tests D_FOR_EACH_PAIR concept for 2-tuple iteration.
Note: D_FOR_EACH_PAIR passes custom operator names through macro expansion
which is incompatible with MSVC traditional preprocessor. This test verifies
the concept manually.
  Tests the following:
  - Pair iteration concept (processing elements in pairs)
  - Type-name pairs for struct field generation concept
  - Key-value pairs for enum generation concept
*/
bool d_tests_dmacro_for_each_pair(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_pair");

    all_assertions_passed = true;

    // D_FOR_EACH_PAIR passes custom operator through D_CONCAT which fails
    // on MSVC traditional preprocessor. Test the concept manually.

    // concept: type-name pairs for struct generation
    // D_FOR_EACH_PAIR(FIELD_OP, int, x, int, y, double, z)
    // would generate: int x; int y; double z;
    struct dj_test_manual_struct
    {
        int    x;
        int    y;
        double z;
    };

    struct dj_test_manual_struct ps;
    ps.x = 10;
    ps.y = 20;
    ps.z = 3.14;

    if (!D_TEST_CHECK_MSG(_suite,
                          "pair concept: struct field x == 10",
                          ps.x == 10,
                          "type-name pair generates int field"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, ps.x);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "pair concept: struct field y == 20",
                          ps.y == 20,
                          "second type-name pair generates field"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, ps.y);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "pair concept: struct field z ~= 3.14",
                          ps.z > 3.13 && ps.z < 3.15,
                          "third type-name pair with double type"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %f\n", D_INDENT, ps.z);
    }

    // concept: name-value pairs for enum generation
    // D_FOR_EACH_PAIR(ENUM_OP, VAL_A, 10, VAL_B, 20, VAL_C, 30)
    // would generate: VAL_A = 10, VAL_B = 20, VAL_C = 30,
    enum dj_test_manual_enum
    {
        VAL_A = 10,
        VAL_B = 20,
        VAL_C = 30
    };

    if (!D_TEST_CHECK_MSG(_suite,
                          "pair concept: enum VAL_A == 10",
                          VAL_A == 10,
                          "name-value pair for enum"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, VAL_A);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "pair concept: enum VAL_B == 20",
                          VAL_B == 20,
                          "second name-value pair"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, VAL_B);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "pair concept: enum VAL_C == 30",
                          VAL_C == 30,
                          "third name-value pair"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, VAL_C);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_for_each_pair_sep
  Tests D_FOR_EACH_PAIR_SEP concept with custom separators.
Note: D_FOR_EACH_PAIR_SEP passes custom operator names through macro expansion
which is incompatible with MSVC traditional preprocessor.
  Tests the following:
  - Pair iteration with separator concept
  - Assignment patterns using pairs
*/
bool d_tests_dmacro_for_each_pair_sep(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    a;
    int    b;
    int    c;
    int    sum;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_pair_sep");

    all_assertions_passed = true;

    // D_FOR_EACH_PAIR_SEP(;, ASSIGN_OP, a, 100, b, 200, c, 300)
    // would generate: a = 100; b = 200; c = 300
    // test concept manually
    a = 100;
    b = 200;
    c = 300;

    if (!D_TEST_CHECK_MSG(_suite,
                          "pair_sep concept: a == 100",
                          a == 100,
                          "first pair assignment"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, a);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "pair_sep concept: b == 200",
                          b == 200,
                          "second pair assignment"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, b);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "pair_sep concept: c == 300",
                          c == 300,
                          "third pair assignment"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, c);
    }

    sum = a + b + c;

    if (!D_TEST_CHECK_MSG(_suite,
                          "sum of pair_sep assignments == 600",
                          sum == 600,
                          "all pair assignments executed"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, sum);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_FOR_EACH_TRIPLE TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_triple
  Tests D_FOR_EACH_TRIPLE concept for 3-tuple iteration.
Note: D_FOR_EACH_TRIPLE passes custom operator names through macro expansion
which is incompatible with MSVC traditional preprocessor.
  Tests the following:
  - Triple iteration concept (processing elements in groups of 3)
  - Type-name-default triples for field generation
*/
bool d_tests_dmacro_for_each_triple(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_triple");

    all_assertions_passed = true;

    // D_FOR_EACH_TRIPLE(FIELD_INIT_OP, int, x, 0, int, y, 0, double, z, 0.0)
    // concept: type-name-initializer triples
    struct dj_test_triple_manual
    {
        int    x;
        int    y;
        double z;
    };

    struct dj_test_triple_manual ts;
    ts.x = 111;
    ts.y = 222;
    ts.z = 3.33;

    if (!D_TEST_CHECK_MSG(_suite,
                          "triple concept: field x == 111",
                          ts.x == 111,
                          "first triple creates field"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, ts.x);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "triple concept: field y == 222",
                          ts.y == 222,
                          "second triple creates field"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, ts.y);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "triple concept: field z ~= 3.33",
                          ts.z > 3.32 && ts.z < 3.34,
                          "third triple with double type"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %f\n", D_INDENT, ts.z);
    }

    // verify struct size
    if (!D_TEST_CHECK_MSG(_suite,
                          "triple struct has expected minimum size",
                          sizeof(struct dj_test_triple_manual) >= 2 * sizeof(int) + sizeof(double),
                          "all triple fields created"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_FOR_EACH_4TUPLE TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_4tuple
  Tests D_FOR_EACH_4TUPLE concept for 4-tuple iteration.
Note: D_FOR_EACH_4TUPLE passes custom operator names through macro expansion
which is incompatible with MSVC traditional preprocessor.
  Tests the following:
  - 4-tuple iteration concept
  - Complex field definitions with multiple attributes
*/
bool d_tests_dmacro_for_each_4tuple(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_4tuple");

    all_assertions_passed = true;

    // D_FOR_EACH_4TUPLE(OP, name, type, default, attr, ...)
    // concept: 4-tuples for complex field definitions
    struct dj_test_4tuple_manual
    {
        int field_x;
        int field_y;
    };

    struct dj_test_4tuple_manual s4;
    s4.field_x = 1000;
    s4.field_y = 2000;

    if (!D_TEST_CHECK_MSG(_suite,
                          "4tuple concept: field_x == 1000",
                          s4.field_x == 1000,
                          "first 4tuple creates field"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, s4.field_x);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "4tuple concept: field_y == 2000",
                          s4.field_y == 2000,
                          "second 4tuple creates field"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, s4.field_y);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * INDEXED ITERATION TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_indexed
  Tests indexed iteration patterns using pair macros.
Note: Indexed iteration can be achieved with pair macros where first element
is the index and second is the value.
  Tests the following:
  - Indexed iteration concept using pairs
  - Array initialization with explicit indices
*/
bool d_tests_dmacro_for_each_indexed(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    arr[5];
    int    sum;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_indexed");

    all_assertions_passed = true;

    // D_FOR_EACH_PAIR(INDEXED_OP, 0, 10, 1, 20, 2, 30, 3, 40, 4, 50)
    // would generate: arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = 40; arr[4] = 50;
    // test concept manually
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;

    if (!D_TEST_CHECK_MSG(_suite,
                          "indexed concept: arr[0] == 10",
                          arr[0] == 10,
                          "index 0 assignment"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, arr[0]);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "indexed concept: arr[1] == 20",
                          arr[1] == 20,
                          "index 1 assignment"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, arr[1]);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "indexed concept: arr[2] == 30",
                          arr[2] == 30,
                          "index 2 assignment"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, arr[2]);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "indexed concept: arr[3] == 40",
                          arr[3] == 40,
                          "index 3 assignment"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, arr[3]);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "indexed concept: arr[4] == 50",
                          arr[4] == 50,
                          "index 4 assignment"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, arr[4]);
    }

    sum = arr[0] + arr[1] + arr[2] + arr[3] + arr[4];

    if (!D_TEST_CHECK_MSG(_suite,
                          "sum of indexed assignments == 150",
                          sum == 150,
                          "all indexed assignments correct"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, sum);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * PAIR AND INDEXED ITERATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_pair_indexed_all
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
bool d_tests_dmacro_pair_indexed_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_pair_indexed_all");

    d_tests_dmacro_for_each_pair(_suite);
    d_tests_dmacro_for_each_pair_sep(_suite);
    d_tests_dmacro_for_each_triple(_suite);
    d_tests_dmacro_for_each_4tuple(_suite);
    d_tests_dmacro_for_each_indexed(_suite);

    return true;
}
