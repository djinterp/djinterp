#include "accumulator_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_reducers_sum
  Folds with additive identity.
  Tests the following:
  - mixed positive/negative values total correctly
  - a single value reduces to itself
*/
bool
test_reducers_sum(
)
{
    std::vector<int> values;
    values.push_back(10);
    values.push_back(-3);
    values.push_back(5);

    D_INTERNAL_ACC_CHECK(sum<int>().run(values) == 12);

    std::vector<int> single;
    single.push_back(42);

    D_INTERNAL_ACC_CHECK(sum<int>().run(single) == 42);

    return true;
}


/*
test_reducers_product
  Folds with multiplicative identity.
  Tests the following:
  - the empty product is the identity (1)
  - a normal product multiplies every element
  - a zero element collapses the product to zero
*/
bool
test_reducers_product(
)
{
    std::vector<int> empty_vec;

    D_INTERNAL_ACC_CHECK(product<int>().run(empty_vec) == 1);

    std::vector<int> values;
    values.push_back(2);
    values.push_back(3);
    values.push_back(4);

    D_INTERNAL_ACC_CHECK(product<int>().run(values) == 24);

    std::vector<int> with_zero;
    with_zero.push_back(7);
    with_zero.push_back(0);
    with_zero.push_back(9);

    D_INTERNAL_ACC_CHECK(product<int>().run(with_zero) == 0);

    return true;
}


/*
test_reducers_count
  Counts elements regardless of value.
  Tests the following:
  - count() ignores element values and returns the element tally
  - the output is std::size_t
*/
bool
test_reducers_count(
)
{
    std::vector<int> values;
    values.push_back(0);
    values.push_back(0);
    values.push_back(-100);

    std::size_t n = count<int>().run(values);

    D_INTERNAL_ACC_CHECK(n == 3);

    return true;
}


/*
test_reducers_count_if
  Counts only elements satisfying a predicate.
  Tests the following:
  - count_if tallies matches (even numbers here)
  - no matches yields zero
*/
bool
test_reducers_count_if(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);
    values.push_back(4);
    values.push_back(6);

    D_INTERNAL_ACC_CHECK(count_if<int>(is_even()).run(values) == 3);

    std::vector<int> odds;
    odds.push_back(1);
    odds.push_back(3);

    D_INTERNAL_ACC_CHECK(count_if<int>(is_even()).run(odds) == 0);

    return true;
}


/*
test_reducers_min
  Tracks the running minimum.
  Tests the following:
  - the minimum of a mixed set is found
  - duplicates of the minimum do not perturb the result
  - negatives are handled
*/
bool
test_reducers_min(
)
{
    std::vector<int> values;
    values.push_back(3);
    values.push_back(-7);
    values.push_back(-7);
    values.push_back(2);

    D_INTERNAL_ACC_CHECK(min<int>().run(values) == -7);

    std::vector<int> single;
    single.push_back(9);

    D_INTERNAL_ACC_CHECK(min<int>().run(single) == 9);

    return true;
}


/*
test_reducers_max
  Tracks the running maximum.
  Tests the following:
  - the maximum of a mixed set is found
  - a single value reduces to itself
*/
bool
test_reducers_max(
)
{
    std::vector<int> values;
    values.push_back(3);
    values.push_back(-7);
    values.push_back(11);
    values.push_back(2);

    D_INTERNAL_ACC_CHECK(max<int>().run(values) == 11);

    return true;
}


/*
test_reducers_min_by
  Minimum under a projection.
  Tests the following:
  - min_by compares by the projected key (point::x)
  - the full element is returned, not the key
*/
bool
test_reducers_min_by(
)
{
    std::vector<point> values;
    values.push_back(point{ 5, 1 });
    values.push_back(point{ 2, 9 });
    values.push_back(point{ 8, 0 });

    point result = min_by<point>(by_x()).run(values);

    D_INTERNAL_ACC_CHECK(result.x == 2);
    D_INTERNAL_ACC_CHECK(result.y == 9);

    return true;
}


/*
test_reducers_max_by
  Maximum under a projection.
  Tests the following:
  - max_by compares by the projected key (point::y)
  - the full element is returned
*/
bool
test_reducers_max_by(
)
{
    std::vector<point> values;
    values.push_back(point{ 5, 1 });
    values.push_back(point{ 2, 9 });
    values.push_back(point{ 8, 0 });

    point result = max_by<point>(by_y()).run(values);

    D_INTERNAL_ACC_CHECK(result.y == 9);
    D_INTERNAL_ACC_CHECK(result.x == 2);

    return true;
}


/*
test_reducers_empty_edges
  Documents the empty-input behaviour of the reducers.
  Tests the following:
  - sum/count/count_if over no elements yield their identities (0)
  - min/max over no elements yield the value-initialised sentinel (0),
    since the "seen" flag was never set
*/
bool
test_reducers_empty_edges(
)
{
    std::vector<int> empty_vec;

    D_INTERNAL_ACC_CHECK(sum<int>().run(empty_vec) == 0);
    D_INTERNAL_ACC_CHECK(count<int>().run(empty_vec) == 0);
    D_INTERNAL_ACC_CHECK(count_if<int>(is_even()).run(empty_vec) == 0);
    D_INTERNAL_ACC_CHECK(min<int>().run(empty_vec) == 0);
    D_INTERNAL_ACC_CHECK(max<int>().run(empty_vec) == 0);

    return true;
}


/*
run_reducers_tests
  Aggregates every reducers-section test.
  Tests the following:
  - all sum/product/count/min/max family tests pass
*/
bool
run_reducers_tests(
)
{
    return ( test_reducers_sum()         &&
             test_reducers_product()     &&
             test_reducers_count()       &&
             test_reducers_count_if()    &&
             test_reducers_min()         &&
             test_reducers_max()         &&
             test_reducers_min_by()      &&
             test_reducers_max_by()      &&
             test_reducers_empty_edges() );
}


NS_END  // testing
NS_END  // djinterp
