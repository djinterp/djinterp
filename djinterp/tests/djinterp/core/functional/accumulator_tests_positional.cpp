#include "accumulator_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_positional_first
  Captures the first element seen.
  Tests the following:
  - the first value is retained
  - later values do not overwrite it
*/
bool
test_positional_first(
)
{
    std::vector<int> values;
    values.push_back(10);
    values.push_back(20);
    values.push_back(30);

    D_INTERNAL_ACC_CHECK(first<int>().run(values) == 10);

    return true;
}


/*
test_positional_last
  Captures the last element seen.
  Tests the following:
  - the final value overwrites all earlier ones
  - a single value reduces to itself
*/
bool
test_positional_last(
)
{
    std::vector<int> values;
    values.push_back(10);
    values.push_back(20);
    values.push_back(30);

    D_INTERNAL_ACC_CHECK(last<int>().run(values) == 30);

    std::vector<int> single;
    single.push_back(77);

    D_INTERNAL_ACC_CHECK(last<int>().run(single) == 77);

    return true;
}


/*
test_positional_nth
  Selects the element at a given zero-based index.
  Tests the following:
  - nth(0) yields the first element
  - an interior index yields the matching element
  - the "seen" counter advances past consumed elements
*/
bool
test_positional_nth(
)
{
    std::vector<int> values;
    values.push_back(10);
    values.push_back(20);
    values.push_back(30);

    D_INTERNAL_ACC_CHECK(nth<int>(0).run(values) == 10);
    D_INTERNAL_ACC_CHECK(nth<int>(1).run(values) == 20);
    D_INTERNAL_ACC_CHECK(nth<int>(2).run(values) == 30);

    return true;
}


/*
test_positional_empty_edges
  Documents the sentinel behaviour at the boundaries.
  Tests the following:
  - first/last over no elements yield the value-initialised default (0)
  - nth with an out-of-range index yields the default (value never set)
*/
bool
test_positional_empty_edges(
)
{
    std::vector<int> empty_vec;

    D_INTERNAL_ACC_CHECK(first<int>().run(empty_vec) == 0);
    D_INTERNAL_ACC_CHECK(last<int>().run(empty_vec) == 0);
    D_INTERNAL_ACC_CHECK(nth<int>(0).run(empty_vec) == 0);

    std::vector<int> values;
    values.push_back(10);
    values.push_back(20);

    // index past the end -> default sentinel
    D_INTERNAL_ACC_CHECK(nth<int>(5).run(values) == 0);

    return true;
}


/*
run_positional_tests
  Aggregates every positional-section test.
  Tests the following:
  - all first/last/nth tests pass
*/
bool
run_positional_tests(
)
{
    return ( test_positional_first()       &&
             test_positional_last()        &&
             test_positional_nth()         &&
             test_positional_empty_edges() );
}


NS_END  // testing
NS_END  // djinterp
