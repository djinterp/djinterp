#include "comparator_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_pipeline_then_adapter
  Verifies the `primary | then(secondary)` pipeline form builds the same
  tie-breaker chain as then(primary, secondary).
  Tests the following:
  - the adapter form ties on rank and defers to id
  - the result matches the two-argument then form
  The pipeline operators and single-argument then adapter exist only on the
  C++11+ path; under C++98 the body is a no-op that still reports success.
*/
bool
test_pipeline_then_adapter(
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    record a = { 5, 1 };
    record b = { 5, 2 };

    bool piped = ( comparators::by_key(get_rank())
                   | comparators::then(comparators::by_key(get_id())) )(a, b);
    bool direct = comparators::then(comparators::by_key(get_rank()),
                                    comparators::by_key(get_id()))(a, b);

    D_INTERNAL_CMP_CHECK(piped == true);
    D_INTERNAL_CMP_CHECK(piped == direct);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return true;
}


/*
test_pipeline_reversed_adapter
  Verifies the `comparator | reversed()` pipeline form wraps its left-hand
  side in a reversed comparator.
  Tests the following:
  - the adapter form inverts the natural ordering
  C++11+ only; C++98 body is a no-op reporting success.
*/
bool
test_pipeline_reversed_adapter(
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_INTERNAL_CMP_CHECK(
        ( comparators::natural<int>() | comparators::reversed() )(2, 1)
        == true);
    D_INTERNAL_CMP_CHECK(
        ( comparators::natural<int>() | comparators::reversed() )(1, 2)
        == false);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return true;
}


/*
test_pipeline_reversed_adapter_equiv
  Confirms `cmp | reversed()` is equivalent to reversed(cmp).
  Tests the following:
  - the adapter form and the function form agree across operand orders
  C++11+ only; C++98 body is a no-op reporting success.
*/
bool
test_pipeline_reversed_adapter_equiv(
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    bool piped  = ( comparators::natural<int>() | comparators::reversed() )(3, 7);
    bool direct = comparators::reversed(comparators::natural<int>())(3, 7);

    D_INTERNAL_CMP_CHECK(piped == direct);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return true;
}


/*
test_pipeline_combined_adapters
  Verifies a pipeline mixing both adapters in one expression. Reversing the
  primary then chaining a secondary tie-breaker must respect operator
  precedence and produce a well-formed strict-weak chain.
  Tests the following:
  - (by_key(rank) | reversed()) orders by descending rank
  - chaining | then(by_key(id)) breaks ties on equal rank by ascending id
  C++11+ only; C++98 body is a no-op reporting success.
*/
bool
test_pipeline_combined_adapters(
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    record a = { 5, 1 };
    record b = { 5, 2 };   // equal rank, larger id
    record c = { 9, 0 };   // larger rank

    // descending rank: c (rank 9) precedes a (rank 5).
    D_INTERNAL_CMP_CHECK(
        ( comparators::by_key(get_rank()) | comparators::reversed()
          | comparators::then(comparators::by_key(get_id())) )(c, a) == true);

    // equal rank -> tie-break ascending id: a (id 1) precedes b (id 2).
    D_INTERNAL_CMP_CHECK(
        ( comparators::by_key(get_rank()) | comparators::reversed()
          | comparators::then(comparators::by_key(get_id())) )(a, b) == true);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return true;
}


/*
test_pipeline_raw_callable_lhs
  Confirms the pipeline operators accept a raw comparator-shaped functor on
  the left, not just this module's helpers.
  Tests the following:
  - asc_int | reversed() yields descending order
  C++11+ only; C++98 body is a no-op reporting success.
*/
bool
test_pipeline_raw_callable_lhs(
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_INTERNAL_CMP_CHECK(( asc_int() | comparators::reversed() )(2, 1) == true);
    D_INTERNAL_CMP_CHECK(( asc_int() | comparators::reversed() )(1, 2) == false);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return true;
}


/*
run_pipeline_tests
  Aggregates every pipeline-section test.
  Tests the following:
  - all operator| / adapter tests pass (C++11+); all are trivial passes on
    the C++98 path where the pipeline forms do not exist
*/
bool
run_pipeline_tests(
)
{
    return ( test_pipeline_then_adapter()           &&
             test_pipeline_reversed_adapter()        &&
             test_pipeline_reversed_adapter_equiv()  &&
             test_pipeline_combined_adapters()       &&
             test_pipeline_raw_callable_lhs() );
}


NS_END  // testing
NS_END  // djinterp
