#include ".\fn_builder_tests_sa.h"


/******************************************************************************
 * TEST HELPERS: callbacks for fluent operation tests
 *****************************************************************************/

/*
test_helper_double_int
  Transformer: multiplies each int by 2.
*/
static bool
test_helper_double_int
(
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
static bool
test_helper_add_ten
(
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
static bool
test_helper_negate
(
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
static bool
test_helper_is_even
(
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
static bool
test_helper_is_positive
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return (*(const int*)_element) > 0;
}


/*
d_tests_sa_fn_builder_map
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
bool
d_tests_sa_fn_builder_map
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    struct d_fn_builder* result;
    bool                 all_passed;

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- append one transformer ----
    result = d_funtional_builder_map(builder,
                                     test_helper_double_int);

    all_passed &= d_assert_standalone(
        result == builder,
        "map: returns builder pointer for chaining",
        "return should be the same builder",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->transform_count == 1,
        "map: transform_count is 1 after first add",
        "one transformer should be registered",
        _test_info);

    // ---- append a second transformer ----
    result = d_funtional_builder_map(builder,
                                     test_helper_add_ten);

    all_passed &= d_assert_standalone(
        result == builder && builder->transform_count == 2,
        "map: transform_count is 2 after second add",
        "two transformers should be registered",
        _test_info);

    // ---- append a third transformer ----
    result = d_funtional_builder_map(builder,
                                     test_helper_negate);

    all_passed &= d_assert_standalone(
        result == builder && builder->transform_count == 3,
        "map: transform_count is 3 after third add",
        "three transformers should be registered",
        _test_info);

    // ---- NULL builder ----
    result = d_funtional_builder_map(NULL,
                                     test_helper_double_int);

    all_passed &= d_assert_standalone(
        result == NULL,
        "map: NULL builder returns NULL",
        "cannot add to a NULL builder",
        _test_info);

    // ---- NULL transform ----
    result = d_funtional_builder_map(builder, NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "map: NULL transform returns NULL",
        "cannot add a NULL function pointer",
        _test_info);

    // verify builder was not modified by the NULL call
    all_passed &= d_assert_standalone(
        builder->transform_count == 3,
        "map: NULL transform does not modify builder",
        "transform_count should remain 3",
        _test_info);

    // ---- both NULL ----
    result = d_funtional_builder_map(NULL, NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "map: both NULL returns NULL",
        "all-invalid should return NULL",
        _test_info);

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_filter
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
bool
d_tests_sa_fn_builder_filter
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    struct d_fn_builder* result;
    bool                 all_passed;

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- append one predicate ----
    result = d_funtional_builder_filter(builder,
                                        test_helper_is_even);

    all_passed &= d_assert_standalone(
        result == builder,
        "filter: returns builder pointer for chaining",
        "return should be the same builder",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->predicate_count == 1,
        "filter: predicate_count is 1 after first add",
        "one predicate should be registered",
        _test_info);

    // ---- append a second predicate ----
    result = d_funtional_builder_filter(builder,
                                        test_helper_is_positive);

    all_passed &= d_assert_standalone(
        result == builder && builder->predicate_count == 2,
        "filter: predicate_count is 2 after second add",
        "two predicates should be registered",
        _test_info);

    // ---- transform_count independent of predicate_count ----
    all_passed &= d_assert_standalone(
        builder->transform_count == 0,
        "filter: transform_count still 0",
        "adding predicates should not affect transforms",
        _test_info);

    // ---- NULL builder ----
    result = d_funtional_builder_filter(NULL,
                                        test_helper_is_even);

    all_passed &= d_assert_standalone(
        result == NULL,
        "filter: NULL builder returns NULL",
        "cannot add to a NULL builder",
        _test_info);

    // ---- NULL predicate ----
    result = d_funtional_builder_filter(builder, NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "filter: NULL predicate returns NULL",
        "cannot add a NULL function pointer",
        _test_info);

    // verify builder was not modified
    all_passed &= d_assert_standalone(
        builder->predicate_count == 2,
        "filter: NULL predicate does not modify builder",
        "predicate_count should remain 2",
        _test_info);

    // ---- both NULL ----
    result = d_funtional_builder_filter(NULL, NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "filter: both NULL returns NULL",
        "all-invalid should return NULL",
        _test_info);

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_and_then
  Tests d_funtional_builder_and_then (alias for map).
  Tests the following:
  - appending via and_then increments transform_count
  - and_then returns the builder pointer
  - and_then with NULL builder returns NULL
  - and_then with NULL transform returns NULL
  - and_then shares the same array as map
*/
bool
d_tests_sa_fn_builder_and_then
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    struct d_fn_builder* result;
    bool                 all_passed;

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- and_then appends a transformer ----
    result = d_funtional_builder_and_then(builder,
                                          test_helper_double_int);

    all_passed &= d_assert_standalone(
        result == builder && builder->transform_count == 1,
        "and_then: appends transformer, returns builder",
        "alias should behave identically to map",
        _test_info);

    // ---- mixed: map then and_then share the same array ----
    d_funtional_builder_map(builder, test_helper_add_ten);
    d_funtional_builder_and_then(builder, test_helper_negate);

    all_passed &= d_assert_standalone(
        builder->transform_count == 3,
        "and_then: shares transform array with map",
        "mixed map/and_then should accumulate in same array",
        _test_info);

    // ---- NULL builder ----
    result = d_funtional_builder_and_then(NULL,
                                          test_helper_double_int);

    all_passed &= d_assert_standalone(
        result == NULL,
        "and_then: NULL builder returns NULL",
        "should delegate NULL check to map",
        _test_info);

    // ---- NULL transform ----
    result = d_funtional_builder_and_then(builder, NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "and_then: NULL transform returns NULL",
        "should delegate NULL check to map",
        _test_info);

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_where
  Tests d_funtional_builder_where (alias for filter).
  Tests the following:
  - appending via where increments predicate_count
  - where returns the builder pointer
  - where with NULL builder returns NULL
  - where with NULL predicate returns NULL
  - where shares the same array as filter
*/
bool
d_tests_sa_fn_builder_where
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    struct d_fn_builder* result;
    bool                 all_passed;

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- where appends a predicate ----
    result = d_funtional_builder_where(builder,
                                       test_helper_is_even);

    all_passed &= d_assert_standalone(
        result == builder && builder->predicate_count == 1,
        "where: appends predicate, returns builder",
        "alias should behave identically to filter",
        _test_info);

    // ---- mixed: filter then where share the same array ----
    d_funtional_builder_filter(builder, test_helper_is_positive);
    d_funtional_builder_where(builder, test_helper_is_even);

    all_passed &= d_assert_standalone(
        builder->predicate_count == 3,
        "where: shares predicate array with filter",
        "mixed filter/where should accumulate in same array",
        _test_info);

    // ---- NULL builder ----
    result = d_funtional_builder_where(NULL,
                                       test_helper_is_even);

    all_passed &= d_assert_standalone(
        result == NULL,
        "where: NULL builder returns NULL",
        "should delegate NULL check to filter",
        _test_info);

    // ---- NULL predicate ----
    result = d_funtional_builder_where(builder, NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "where: NULL predicate returns NULL",
        "should delegate NULL check to filter",
        _test_info);

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_chaining
  Tests fluent chaining of multiple map/filter/and_then/where calls.
  Tests the following:
  - chained map->filter builds expected counts
  - chained map->and_then->filter->where builds expected counts
  - chain breaks on NULL return (NULL transform mid-chain)
  - chaining using return value directly works
*/
bool
d_tests_sa_fn_builder_chaining
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    struct d_fn_builder* result;
    bool                 all_passed;

    all_passed = true;

    // ---- chain: map -> filter ----
    builder = d_fn_builder_new();
    result  = d_funtional_builder_map(builder,
                                      test_helper_double_int);
    result  = d_funtional_builder_filter(result,
                                         test_helper_is_even);

    all_passed &= d_assert_standalone(
        result == builder,
        "chain: map -> filter returns same builder",
        "chaining should pass through the same pointer",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->transform_count == 1 &&
        builder->predicate_count == 1,
        "chain: map -> filter counts correct",
        "1 transform and 1 predicate",
        _test_info);

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

    all_passed &= d_assert_standalone(
        result == builder,
        "chain: map -> and_then -> filter -> where returns builder",
        "four-stage chain should yield same pointer",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->transform_count == 2 &&
        builder->predicate_count == 2,
        "chain: four-stage counts correct",
        "2 transforms and 2 predicates",
        _test_info);

    d_fn_builder_free(builder);

    // ---- chain breaks on NULL return ----
    builder = d_fn_builder_new();
    result  = d_funtional_builder_map(builder,
                                      test_helper_double_int);

    // inserting NULL transform breaks the chain
    result  = d_funtional_builder_map(result, NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "chain: NULL transform mid-chain returns NULL",
        "chain should break when invalid arg inserted",
        _test_info);

    // builder itself should still be valid with 1 transform
    all_passed &= d_assert_standalone(
        builder->transform_count == 1,
        "chain: builder state preserved after chain break",
        "the first successful map should persist",
        _test_info);

    d_fn_builder_free(builder);

    // ---- direct return-value chaining (idiomatic usage) ----
    builder = d_fn_builder_new();
    result  = d_funtional_builder_filter(
                  d_funtional_builder_map(builder,
                                          test_helper_double_int),
                  test_helper_is_even);

    all_passed &= d_assert_standalone(
        result == builder,
        "chain: nested call chaining works",
        "filter(map(b, t), p) should return b",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->transform_count == 1 &&
        builder->predicate_count == 1,
        "chain: nested call counts correct",
        "1 transform and 1 predicate from nested calls",
        _test_info);

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_grow
  Tests the internal grow mechanism by exceeding the initial capacity.
  Tests the following:
  - adding up to initial capacity (8) succeeds without grow
  - adding the 9th element triggers grow and succeeds
  - adding many more elements (16, 20) continues to succeed
  - predicate array also grows correctly beyond initial capacity
  - mixed transforms and predicates both grow independently
*/
bool
d_tests_sa_fn_builder_grow
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    struct d_fn_builder* result;
    bool                 all_passed;
    size_t               i;
    size_t               initial_cap;

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

    all_passed &= d_assert_standalone(
        builder->transform_count == initial_cap,
        "grow: fill to initial capacity succeeds",
        "should fit without reallocation",
        _test_info);

    // ---- exceed capacity: triggers grow ----
    result = d_funtional_builder_map(builder,
                                     test_helper_add_ten);

    all_passed &= d_assert_standalone(
        result == builder,
        "grow: exceeding capacity succeeds (transforms)",
        "grow should reallocate and return builder",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->transform_count == initial_cap + 1,
        "grow: transform_count correct after grow",
        "should be initial_cap + 1",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->capacity > initial_cap,
        "grow: capacity increased after grow",
        "capacity should have doubled",
        _test_info);

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

    all_passed &= d_assert_standalone(
        builder->transform_count == 20,
        "grow: 20 transforms accumulated successfully",
        "multiple grow cycles should work",
        _test_info);

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

    all_passed &= d_assert_standalone(
        builder->predicate_count == 12,
        "grow: 12 predicates accumulated (exceeds initial cap)",
        "predicate array should grow as needed",
        _test_info);

    d_fn_builder_free(builder);

    // ---- mixed: both arrays grow ----
    builder = d_fn_builder_new();

    for (i = 0; i < 10; i++)
    {
        d_funtional_builder_map(builder, test_helper_double_int);
        d_funtional_builder_filter(builder, test_helper_is_even);
    }

    all_passed &= d_assert_standalone(
        builder->transform_count == 10 &&
        builder->predicate_count == 10,
        "grow: mixed 10 transforms + 10 predicates",
        "both arrays should grow independently",
        _test_info);

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_operations_all
  Runs all fluent operation tests.
  Tests the following:
  - d_funtional_builder_map
  - d_funtional_builder_filter
  - d_funtional_builder_and_then
  - d_funtional_builder_where
  - fluent chaining patterns
  - internal grow mechanism
*/
bool
d_tests_sa_fn_builder_operations_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_fn_builder_map(_test_info);
    all_passed &= d_tests_sa_fn_builder_filter(_test_info);
    all_passed &= d_tests_sa_fn_builder_and_then(_test_info);
    all_passed &= d_tests_sa_fn_builder_where(_test_info);
    all_passed &= d_tests_sa_fn_builder_chaining(_test_info);
    all_passed &= d_tests_sa_fn_builder_grow(_test_info);

    return all_passed;
}
