/*******************************************************************************
* djinterp [functional]                            fn_builder_tests_operations.c
*
*
* path:      /tests/djinterp/c/functional/fn_builder_tests_operations.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./fn_builder_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST HELPERS: callbacks for fluent operation tests
 *****************************************************************************/

/*
test_helper_double_int
  Transformer: multiplies each int by 2.
*/
static
bool test_helper_double_int(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = (*(const int*)_input) * 2;

    return true;
}


/*
test_helper_add_ten
  Transformer: adds 10 to each int.
*/
static
bool test_helper_add_ten(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = (*(const int*)_input) + 10;

    return true;
}


/*
test_helper_negate
  Transformer: negates each int.
*/
static
bool test_helper_negate(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = -(*(const int*)_input);

    return true;
}


/*
test_helper_is_even
  Predicate: returns true if the int is even.
*/
static
bool test_helper_is_even(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return (*(const int*)_element) % 2 == 0;
}


/*
test_helper_is_positive
  Predicate: returns true if the int is > 0.
*/
static
bool test_helper_is_positive(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return (*(const int*)_element) > 0;
}


/*
d_tests_fn_builder_map
  Tests d_funtional_builder_map.
  Tests the following:
  - appending a valid transformer increments transform_count
  - return value is the builder itself (for chaining)
  - multiple transforms can be appended sequentially
  - NULL builder returns NULL
  - NULL transform returns NULL
  - both NULL returns NULL
  - NULL parameters do not modify a valid builder's state
*/
bool d_tests_fn_builder_map(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    struct dj_fn_builder* result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_map");

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- append one transformer ----
    result = d_funtional_builder_map(builder,
                                     test_helper_double_int);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: returns builder pointer for chaining",
                                   result == builder,
                                   "return should be the same builder");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: transform_count is 1 after first add",
                                   builder->transform_count == 1,
                                   "one transformer should be registered");

    // ---- append a second transformer ----
    result = d_funtional_builder_map(builder,
                                     test_helper_add_ten);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: transform_count is 2 after second add",
                                   result == builder && builder->transform_count == 2,
                                   "two transformers should be registered");

    // ---- append a third transformer ----
    result = d_funtional_builder_map(builder,
                                     test_helper_negate);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: transform_count is 3 after third add",
                                   result == builder && builder->transform_count == 3,
                                   "three transformers should be registered");

    // ---- NULL builder ----
    result = d_funtional_builder_map(NULL,
                                     test_helper_double_int);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: NULL builder returns NULL",
                                   result == NULL,
                                   "cannot add to a NULL builder");

    // ---- NULL transform ----
    result = d_funtional_builder_map(builder, NULL);

    //   a missing transformer adds nothing and returns the builder: NULL is
    // kept for a builder that is missing or cannot grow
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: NULL transform adds nothing",
                                   result == builder,
                                   "the builder is returned unchanged");

    // verify builder was not modified by the NULL call
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: NULL transform does not modify builder",
                                   builder->transform_count == 3,
                                   "transform_count should remain 3");

    // ---- both NULL ----
    result = d_funtional_builder_map(NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: both NULL returns NULL",
                                   result == NULL,
                                   "all-invalid should return NULL");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_filter
  Tests d_funtional_builder_filter.
  Tests the following:
  - appending a valid predicate increments predicate_count
  - return value is the builder itself (for chaining)
  - multiple predicates can be appended sequentially
  - NULL builder returns NULL
  - NULL predicate returns NULL
  - both NULL returns NULL
  - NULL parameters do not modify a valid builder's state
  - predicate_count is independent of transform_count
*/
bool d_tests_fn_builder_filter(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    struct dj_fn_builder* result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_filter");

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- append one predicate ----
    result = d_funtional_builder_filter(builder,
                                        test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: returns builder pointer for chaining",
                                   result == builder,
                                   "return should be the same builder");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: predicate_count is 1 after first add",
                                   builder->predicate_count == 1,
                                   "one predicate should be registered");

    // ---- append a second predicate ----
    result = d_funtional_builder_filter(builder,
                                        test_helper_is_positive);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: predicate_count is 2 after second add",
                                   result == builder && builder->predicate_count == 2,
                                   "two predicates should be registered");

    // ---- transform_count independent of predicate_count ----
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: transform_count still 0",
                                   builder->transform_count == 0,
                                   "adding predicates should not affect transforms");

    // ---- NULL builder ----
    result = d_funtional_builder_filter(NULL,
                                        test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: NULL builder returns NULL",
                                   result == NULL,
                                   "cannot add to a NULL builder");

    // ---- NULL predicate ----
    result = d_funtional_builder_filter(builder, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: NULL predicate adds nothing",
                                   result == builder,
                                   "the builder is returned unchanged");

    // verify builder was not modified
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: NULL predicate does not modify builder",
                                   builder->predicate_count == 2,
                                   "predicate_count should remain 2");

    // ---- both NULL ----
    result = d_funtional_builder_filter(NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: both NULL returns NULL",
                                   result == NULL,
                                   "all-invalid should return NULL");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_and_then
  Tests d_funtional_builder_and_then (alias for map).
  Tests the following:
  - appending via and_then increments transform_count
  - and_then returns the builder pointer
  - and_then with NULL builder returns NULL
  - and_then with NULL transform returns NULL
  - and_then shares the same array as map
*/
bool d_tests_fn_builder_and_then(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    struct dj_fn_builder* result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_and_then");

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- and_then appends a transformer ----
    result = d_funtional_builder_and_then(builder,
                                          test_helper_double_int);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "and_then: appends transformer, returns builder",
                                   result == builder && builder->transform_count == 1,
                                   "alias should behave identically to map");

    // ---- mixed: map then and_then share the same array ----
    d_funtional_builder_map(builder, test_helper_add_ten);
    d_funtional_builder_and_then(builder, test_helper_negate);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "and_then: shares transform array with map",
                                   builder->transform_count == 3,
                                   "mixed map/and_then should accumulate in same array");

    // ---- NULL builder ----
    result = d_funtional_builder_and_then(NULL,
                                          test_helper_double_int);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "and_then: NULL builder returns NULL",
                                   result == NULL,
                                   "should delegate NULL check to map");

    // ---- NULL transform ----
    result = d_funtional_builder_and_then(builder, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "and_then: NULL transform adds nothing",
                                   result == builder,
                                   "delegates to map: builder unchanged");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_where
  Tests d_funtional_builder_where (alias for filter).
  Tests the following:
  - appending via where increments predicate_count
  - where returns the builder pointer
  - where with NULL builder returns NULL
  - where with NULL predicate returns NULL
  - where shares the same array as filter
*/
bool d_tests_fn_builder_where(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    struct dj_fn_builder* result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_where");

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- where appends a predicate ----
    result = d_funtional_builder_where(builder,
                                       test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "where: appends predicate, returns builder",
                                   result == builder && builder->predicate_count == 1,
                                   "alias should behave identically to filter");

    // ---- mixed: filter then where share the same array ----
    d_funtional_builder_filter(builder, test_helper_is_positive);
    d_funtional_builder_where(builder, test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "where: shares predicate array with filter",
                                   builder->predicate_count == 3,
                                   "mixed filter/where should accumulate in same array");

    // ---- NULL builder ----
    result = d_funtional_builder_where(NULL,
                                       test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "where: NULL builder returns NULL",
                                   result == NULL,
                                   "should delegate NULL check to filter");

    // ---- NULL predicate ----
    result = d_funtional_builder_where(builder, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "where: NULL predicate adds nothing",
                                   result == builder,
                                   "delegates to filter: builder unchanged");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_chaining
  Tests fluent chaining of multiple map/filter/and_then/where calls.
  Tests the following:
  - chained map->filter builds expected counts
  - chained map->and_then->filter->where builds expected counts
  - chain breaks on NULL return (NULL transform mid-chain)
  - chaining using return value directly works
*/
bool d_tests_fn_builder_chaining(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    struct dj_fn_builder* result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_chaining");

    all_passed = true;

    // ---- chain: map -> filter ----
    builder = d_fn_builder_new();
    result  = d_funtional_builder_map(builder,
                                      test_helper_double_int);
    result  = d_funtional_builder_filter(result,
                                         test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: map -> filter returns same builder",
                                   result == builder,
                                   "chaining should pass through the same pointer");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: map -> filter counts correct",
                                   builder->transform_count == 1 && builder->predicate_count == 1,
                                   "1 transform and 1 predicate");

    d_fn_builder_free(builder);

    // ---- chain: map -> and_then -> filter -> where ----
    builder = d_fn_builder_new();
    result  = d_funtional_builder_map(builder,
                                      test_helper_double_int);
    result  = d_funtional_builder_and_then(result,
                                           test_helper_add_ten);
    result  = d_funtional_builder_filter(result,
                                         test_helper_is_even);
    result  = d_funtional_builder_where(result,
                                        test_helper_is_positive);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: map -> and_then -> filter -> where returns builder",
                                   result == builder,
                                   "four-stage chain should yield same pointer");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: four-stage counts correct",
                                   builder->transform_count == 2 && builder->predicate_count == 2,
                                   "2 transforms and 2 predicates");

    d_fn_builder_free(builder);

    // ---- chain breaks on NULL return ----
    builder = d_fn_builder_new();
    result  = d_funtional_builder_map(builder,
                                      test_helper_double_int);

    // a NULL transform mid-chain adds nothing; the chain carries on
    result  = d_funtional_builder_map(result, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: NULL transform mid-chain adds nothing",
                                   result == builder,
                                   "the chain keeps its builder");

    // builder itself should still be valid with 1 transform
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: builder state preserved after chain break",
                                   builder->transform_count == 1,
                                   "the first successful map should persist");

    d_fn_builder_free(builder);

    // ---- direct return-value chaining (idiomatic usage) ----
    builder = d_fn_builder_new();
    result  = d_funtional_builder_filter(
                  d_funtional_builder_map(builder,
                                          test_helper_double_int),
                  test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: nested call chaining works",
                                   result == builder,
                                   "filter(map(b, t), p) should return b");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: nested call counts correct",
                                   builder->transform_count == 1 && builder->predicate_count == 1,
                                   "1 transform and 1 predicate from nested calls");

    d_fn_builder_free(builder);

    return all_passed;
}


/**
 * @brief Part 1 of d_tests_fn_builder_grow.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - fill transforms to initial capacity
 *   - exceed capacity: triggers grow
 *   - keep adding well beyond initial capacity
 *   - predicate array grows independently
 */
static bool
d_internal_tests_fn_builder_grow_part1(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    struct dj_fn_builder* result;
    bool                  all_passed;
    size_t                i;
    size_t                initial_cap;

    all_passed = true;

    // ---- fill transforms to initial capacity ----
    builder     = d_fn_builder_new();
    initial_cap = builder->capacity;

    for (i = 0; i < initial_cap; i++)
    {
        result = d_funtional_builder_map(builder,
                                         test_helper_double_int);

        if (!result)
        {
            break;
        }
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: fill to initial capacity succeeds",
                                   builder->transform_count == initial_cap,
                                   "should fit without reallocation");

    // ---- exceed capacity: triggers grow ----
    result = d_funtional_builder_map(builder,
                                     test_helper_add_ten);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: exceeding capacity succeeds (transforms)",
                                   result == builder,
                                   "grow should reallocate and return builder");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: transform_count correct after grow",
                                   builder->transform_count == initial_cap + 1,
                                   "should be initial_cap + 1");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: capacity increased after grow",
                                   builder->capacity > initial_cap,
                                   "capacity should have doubled");

    // ---- keep adding well beyond initial capacity ----
    for (i = builder->transform_count; i < 20; i++)
    {
        result = d_funtional_builder_map(builder,
                                         test_helper_negate);

        if (!result)
        {
            break;
        }
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: 20 transforms accumulated successfully",
                                   builder->transform_count == 20,
                                   "multiple grow cycles should work");

    d_fn_builder_free(builder);

    // ---- predicate array grows independently ----
    builder = d_fn_builder_new();

    for (i = 0; i < 12; i++)
    {
        result = d_funtional_builder_filter(builder,
                                            test_helper_is_even);

        if (!result)
        {
            break;
        }
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: 12 predicates accumulated (exceeds initial cap)",
                                   builder->predicate_count == 12,
                                   "predicate array should grow as needed");

    d_fn_builder_free(builder);

    return all_passed;
}


/**
 * @brief Part 2 of d_tests_fn_builder_grow.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - mixed: both arrays grow
 */
static bool
d_internal_tests_fn_builder_grow_part2(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    bool                  all_passed;
    size_t                i;

    all_passed = true;

    // ---- mixed: both arrays grow ----
    builder = d_fn_builder_new();

    for (i = 0; i < 10; i++)
    {
        d_funtional_builder_map(builder, test_helper_double_int);
        d_funtional_builder_filter(builder, test_helper_is_even);
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: mixed 10 transforms + 10 predicates",
                                   builder->transform_count == 10 && builder->predicate_count == 10,
                                   "both arrays should grow independently");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_grow
  Tests the internal grow mechanism by exceeding the initial capacity.
  Tests the following:
  - adding up to initial capacity (8) succeeds without grow
  - adding the 9th element triggers grow and succeeds
  - adding many more elements (16, 20) continues to succeed
  - predicate array also grows correctly beyond initial capacity
  - mixed transforms and predicates both grow independently
*/
bool d_tests_fn_builder_grow(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    d_test_suite_test(_suite, "fn_builder_grow");

    all_passed = true;
    all_passed &= d_internal_tests_fn_builder_grow_part1(_suite);
    all_passed &= d_internal_tests_fn_builder_grow_part2(_suite);

    return all_passed;
}


/*
d_tests_fn_builder_operations_all
  Runs all fluent operation tests.
  Tests the following:
  - d_funtional_builder_map
  - d_funtional_builder_filter
  - d_funtional_builder_and_then
  - d_funtional_builder_where
  - fluent chaining patterns
  - internal grow mechanism
*/
bool d_tests_fn_builder_operations_all(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    d_test_suite_block(_suite, "fn_builder_operations_all");

    all_passed = true;

    all_passed &= d_tests_fn_builder_map(_suite);
    all_passed &= d_tests_fn_builder_filter(_suite);
    all_passed &= d_tests_fn_builder_and_then(_suite);
    all_passed &= d_tests_fn_builder_where(_suite);
    all_passed &= d_tests_fn_builder_chaining(_suite);
    all_passed &= d_tests_fn_builder_grow(_suite);

    return all_passed;
}
