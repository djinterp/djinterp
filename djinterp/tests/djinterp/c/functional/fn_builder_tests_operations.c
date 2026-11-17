/*******************************************************************************
* djinterp [functional]                            fn_builder_tests_operations.c
*
*
* path:      /tests/djinterp/c/functional/fn_builder_tests_operations.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.11
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
test_helper_count_steps
  The number of the builder's steps of one kind.
*/
static
size_t test_helper_count_steps(
    const struct dj_fn_builder*  _builder,
    enum dj_fn_builder_step_kind _kind
)
{
    size_t n;
    size_t s;

    n = 0;

    for (s = 0; s < _builder->count; s++)
    {
        if (_builder->steps[s].kind == _kind)
        {
            n++;
        }
    }

    return n;
}


/*
test_helper_transforms / test_helper_predicates
  The builder's transformer steps, and its predicate steps.
*/
static
size_t test_helper_transforms(
    const struct dj_fn_builder* _builder
)
{
    return test_helper_count_steps(_builder, D_FN_BUILDER_STEP_TRANSFORM);
}

static
size_t test_helper_predicates(
    const struct dj_fn_builder* _builder
)
{
    return test_helper_count_steps(_builder, D_FN_BUILDER_STEP_PREDICATE);
}


/*
d_tests_fn_builder_map
  Tests d_fn_builder_map.
  Tests the following:
  - appending a valid transformer adds one transformer step
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
    result = d_fn_builder_map(builder,
                              test_helper_double_int);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: returns builder pointer for chaining",
                                   result == builder,
                                   "return should be the same builder");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: one transformer after first add",
                                   test_helper_transforms(builder) == 1,
                                   "one transformer should be registered");

    // ---- append a second transformer ----
    result = d_fn_builder_map(builder,
                              test_helper_add_ten);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: two transformers after second add",
                                   (result == builder) &&
                                   (test_helper_transforms(builder) == 2),
                                   "two transformers should be registered");

    // ---- append a third transformer ----
    result = d_fn_builder_map(builder,
                              test_helper_negate);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: three transformers after third add",
                                   (result == builder) &&
                                   (test_helper_transforms(builder) == 3),
                                   "three transformers should be registered");

    // ---- NULL builder ----
    result = d_fn_builder_map(NULL,
                              test_helper_double_int);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: NULL builder returns NULL",
                                   result == NULL,
                                   "cannot add to a NULL builder");

    // ---- NULL transform ----
    result = d_fn_builder_map(builder, NULL);

    //   a missing transformer adds nothing and returns the builder: NULL is
    // kept for a builder that is missing or cannot grow
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: NULL transform adds nothing",
                                   result == builder,
                                   "the builder is returned unchanged");

    // verify builder was not modified by the NULL call
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: NULL transform does not modify builder",
                                   test_helper_transforms(builder) == 3,
                                   "the chain should keep its 3 transformers");

    // ---- both NULL ----
    result = d_fn_builder_map(NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: both NULL returns NULL",
                                   result == NULL,
                                   "all-invalid should return NULL");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_filter
  Tests d_fn_builder_filter.
  Tests the following:
  - appending a valid predicate adds one predicate step
  - return value is the builder itself (for chaining)
  - multiple predicates can be appended sequentially
  - NULL builder returns NULL
  - NULL predicate returns NULL
  - both NULL returns NULL
  - NULL parameters do not modify a valid builder's state
  - adding predicates adds no transformer step
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
    result = d_fn_builder_filter(builder,
                                 test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: returns builder pointer for chaining",
                                   result == builder,
                                   "return should be the same builder");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: one predicate after first add",
                                   test_helper_predicates(builder) == 1,
                                   "one predicate should be registered");

    // ---- append a second predicate ----
    result = d_fn_builder_filter(builder,
                                 test_helper_is_positive);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: two predicates after second add",
                                   (result == builder) &&
                                   (test_helper_predicates(builder) == 2),
                                   "two predicates should be registered");

    // ---- predicates add no transformer ----
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: no transformer added",
                                   test_helper_transforms(builder) == 0,
                                   "adding predicates should not affect transforms");

    // ---- NULL builder ----
    result = d_fn_builder_filter(NULL,
                                 test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: NULL builder returns NULL",
                                   result == NULL,
                                   "cannot add to a NULL builder");

    // ---- NULL predicate ----
    result = d_fn_builder_filter(builder, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: NULL predicate adds nothing",
                                   result == builder,
                                   "the builder is returned unchanged");

    // verify builder was not modified
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: NULL predicate does not modify builder",
                                   test_helper_predicates(builder) == 2,
                                   "the chain should keep its 2 predicates");

    // ---- both NULL ----
    result = d_fn_builder_filter(NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: both NULL returns NULL",
                                   result == NULL,
                                   "all-invalid should return NULL");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_and_then
  Tests d_fn_builder_and_then (alias for map).
  Tests the following:
  - appending via and_then adds a transformer step
  - and_then returns the builder pointer
  - and_then with NULL builder returns NULL
  - and_then with NULL transform returns NULL
  - and_then and map append to the one chain
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
    result = d_fn_builder_and_then(builder,
                                   test_helper_double_int);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "and_then: appends transformer, returns builder",
                                   (result == builder) &&
                                   (test_helper_transforms(builder) == 1),
                                   "alias should behave identically to map");

    // ---- mixed: map then and_then share the same array ----
    d_fn_builder_map(builder, test_helper_add_ten);
    d_fn_builder_and_then(builder, test_helper_negate);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "and_then: appends to the chain map appends to",
                                   test_helper_transforms(builder) == 3,
                                   "mixed map/and_then should accumulate in one chain");

    // ---- NULL builder ----
    result = d_fn_builder_and_then(NULL,
                                   test_helper_double_int);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "and_then: NULL builder returns NULL",
                                   result == NULL,
                                   "should delegate NULL check to map");

    // ---- NULL transform ----
    result = d_fn_builder_and_then(builder, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "and_then: NULL transform adds nothing",
                                   result == builder,
                                   "delegates to map: builder unchanged");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_where
  Tests d_fn_builder_where (alias for filter).
  Tests the following:
  - appending via where adds a predicate step
  - where returns the builder pointer
  - where with NULL builder returns NULL
  - where with NULL predicate returns NULL
  - where and filter append to the one chain
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
    result = d_fn_builder_where(builder,
                                test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "where: appends predicate, returns builder",
                                   (result == builder) &&
                                   (test_helper_predicates(builder) == 1),
                                   "alias should behave identically to filter");

    // ---- mixed: filter then where share the same array ----
    d_fn_builder_filter(builder, test_helper_is_positive);
    d_fn_builder_where(builder, test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "where: appends to the chain filter appends to",
                                   test_helper_predicates(builder) == 3,
                                   "mixed filter/where should accumulate in one chain");

    // ---- NULL builder ----
    result = d_fn_builder_where(NULL,
                                test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "where: NULL builder returns NULL",
                                   result == NULL,
                                   "should delegate NULL check to filter");

    // ---- NULL predicate ----
    result = d_fn_builder_where(builder, NULL);

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
  - the steps are recorded in call order, each with its function
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
    result  = d_fn_builder_map(builder,
                               test_helper_double_int);
    result  = d_fn_builder_filter(result,
                                  test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: map -> filter returns same builder",
                                   result == builder,
                                   "chaining should pass through the same pointer");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: map -> filter counts correct",
                                   (test_helper_transforms(builder) == 1) &&
                                   (test_helper_predicates(builder) == 1),
                                   "1 transform and 1 predicate");

    d_fn_builder_free(builder);

    // ---- chain: map -> and_then -> filter -> where ----
    builder = d_fn_builder_new();
    result  = d_fn_builder_map(builder,
                               test_helper_double_int);
    result  = d_fn_builder_and_then(result,
                                    test_helper_add_ten);
    result  = d_fn_builder_filter(result,
                                  test_helper_is_even);
    result  = d_fn_builder_where(result,
                                 test_helper_is_positive);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: map -> and_then -> filter -> where returns builder",
                                   result == builder,
                                   "four-stage chain should yield same pointer");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: four-stage counts correct",
                                   (test_helper_transforms(builder) == 2) &&
                                   (test_helper_predicates(builder) == 2),
                                   "2 transforms and 2 predicates");

    // the steps are recorded in call order, each with its function
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: steps recorded in call order",
                                   (builder->count == 4) &&
                                   (builder->steps[0].kind ==
                                        D_FN_BUILDER_STEP_TRANSFORM) &&
                                   (builder->steps[0].fn.transform ==
                                        test_helper_double_int) &&
                                   (builder->steps[1].kind ==
                                        D_FN_BUILDER_STEP_TRANSFORM) &&
                                   (builder->steps[1].fn.transform ==
                                        test_helper_add_ten) &&
                                   (builder->steps[2].kind ==
                                        D_FN_BUILDER_STEP_PREDICATE) &&
                                   (builder->steps[2].fn.predicate ==
                                        test_helper_is_even) &&
                                   (builder->steps[3].kind ==
                                        D_FN_BUILDER_STEP_PREDICATE) &&
                                   (builder->steps[3].fn.predicate ==
                                        test_helper_is_positive),
                                   "map, and_then, filter, where in that order");

    d_fn_builder_free(builder);

    // ---- chain breaks on NULL return ----
    builder = d_fn_builder_new();
    result  = d_fn_builder_map(builder,
                               test_helper_double_int);

    // a NULL transform mid-chain adds nothing; the chain carries on
    result  = d_fn_builder_map(result, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: NULL transform mid-chain adds nothing",
                                   result == builder,
                                   "the chain keeps its builder");

    // builder itself should still be valid with 1 transform
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: builder state preserved after chain break",
                                   test_helper_transforms(builder) == 1,
                                   "the first successful map should persist");

    d_fn_builder_free(builder);

    // ---- direct return-value chaining (idiomatic usage) ----
    builder = d_fn_builder_new();
    result  = d_fn_builder_filter(
                  d_fn_builder_map(builder,
                                          test_helper_double_int),
                  test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: nested call chaining works",
                                   result == builder,
                                   "filter(map(b, t), p) should return b");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: nested call counts correct",
                                   (test_helper_transforms(builder) == 1) &&
                                   (test_helper_predicates(builder) == 1),
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
 *   - predicates alone grow the chain
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
        result = d_fn_builder_map(builder,
                                  test_helper_double_int);

        if (!result)
        {
            break;
        }
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: fill to initial capacity succeeds",
                                   test_helper_transforms(builder) ==
                                       initial_cap,
                                   "should fit without reallocation");

    // ---- exceed capacity: triggers grow ----
    result = d_fn_builder_map(builder,
                              test_helper_add_ten);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: exceeding capacity succeeds (transforms)",
                                   result == builder,
                                   "grow should reallocate and return builder");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: step count correct after grow",
                                   test_helper_transforms(builder) ==
                                       initial_cap + 1,
                                   "should be initial_cap + 1");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: capacity increased after grow",
                                   builder->capacity > initial_cap,
                                   "capacity should have doubled");

    // ---- keep adding well beyond initial capacity ----
    for (i = test_helper_transforms(builder); i < 20; i++)
    {
        result = d_fn_builder_map(builder,
                                  test_helper_negate);

        if (!result)
        {
            break;
        }
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: 20 transforms accumulated successfully",
                                   test_helper_transforms(builder) == 20,
                                   "multiple grow cycles should work");

    d_fn_builder_free(builder);

    // ---- predicates alone grow the chain ----
    builder = d_fn_builder_new();

    for (i = 0; i < 12; i++)
    {
        result = d_fn_builder_filter(builder,
                                     test_helper_is_even);

        if (!result)
        {
            break;
        }
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: 12 predicates accumulated (exceeds initial cap)",
                                   test_helper_predicates(builder) == 12,
                                   "the chain should grow as needed");

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
 *   - mixed: the chain grows, alternating
 */
static bool
d_internal_tests_fn_builder_grow_part2(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    bool                  all_passed;
    bool                  alternating;
    size_t                i;

    all_passed = true;

    // ---- mixed: the chain grows, alternating ----
    builder = d_fn_builder_new();

    for (i = 0; i < 10; i++)
    {
        d_fn_builder_map(builder, test_helper_double_int);
        d_fn_builder_filter(builder, test_helper_is_even);
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: mixed 10 transforms + 10 predicates",
                                   (test_helper_transforms(builder) == 10) &&
                                   (test_helper_predicates(builder) == 10),
                                   "the one chain should hold both kinds");

    // the growth kept the order: transformer, predicate, transformer, ...
    alternating = (builder->count == 20);

    for (i = 0; (alternating) && (i < builder->count); i++)
    {
        alternating = (builder->steps[i].kind ==
                       ( (i % 2 == 0) ? D_FN_BUILDER_STEP_TRANSFORM
                                      : D_FN_BUILDER_STEP_PREDICATE ));
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "grow: growth keeps the call order",
                                   alternating,
                                   "steps should alternate as they were added");

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
  - predicates alone also grow the chain beyond initial capacity
  - mixed transforms and predicates grow one chain, in call order
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
d_tests_fn_builder_deprecated_names
  Tests the misspelt d_funtional_builder_* names, kept until 0.9.0-rc.1.
  Tests the following:
  - each names the same function as its d_fn_builder_* replacement
  - a chain built through them is the chain the new names build
*/
bool d_tests_fn_builder_deprecated_names(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    struct dj_fn_builder* result;
    bool                  all_passed;

    d_test_suite_test(_suite, "fn_builder_deprecated_names");

    all_passed = true;

    // ---- the same functions ----
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "deprecated: the four names are aliases",
                                   (d_funtional_builder_map ==
                                        d_fn_builder_map) &&
                                   (d_funtional_builder_filter ==
                                        d_fn_builder_filter) &&
                                   (d_funtional_builder_and_then ==
                                        d_fn_builder_and_then) &&
                                   (d_funtional_builder_where ==
                                        d_fn_builder_where),
                                   "each should name its replacement");

    // ---- a chain through the old names ----
    builder = d_fn_builder_new();
    result  = d_funtional_builder_where(
                  d_funtional_builder_map(builder, test_helper_double_int),
                  test_helper_is_even);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "deprecated: old names build the same chain",
                                   (result == builder) &&
                                   (builder->count == 2) &&
                                   (builder->steps[0].kind ==
                                        D_FN_BUILDER_STEP_TRANSFORM) &&
                                   (builder->steps[1].kind ==
                                        D_FN_BUILDER_STEP_PREDICATE),
                                   "map then where, in call order");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_operations_all
  Runs all fluent operation tests.
  Tests the following:
  - d_fn_builder_map
  - d_fn_builder_filter
  - d_fn_builder_and_then
  - d_fn_builder_where
  - fluent chaining patterns
  - internal grow mechanism
  - the deprecated d_funtional_builder_* names
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
    all_passed &= d_tests_fn_builder_deprecated_names(_suite);

    return all_passed;
}
