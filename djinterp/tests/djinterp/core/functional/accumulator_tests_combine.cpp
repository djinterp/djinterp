#include "accumulator_tests.hpp"

// std
#include <tuple>
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_combine_single
  Combines a single accumulator.
  Tests the following:
  - combine of one accumulator yields a 1-tuple
  - the lone result matches the standalone fold
*/
bool
test_combine_single(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    std::tuple<int> result = combine(sum<int>()).run(values);

    D_INTERNAL_ACC_CHECK(std::get<0>(result) == 6);

    return true;
}


/*
test_combine_pair
  Runs two accumulators in lock-step over one pass.
  Tests the following:
  - both accumulators see every element
  - the tuple carries each result in declaration order
*/
bool
test_combine_pair(
)
{
    std::vector<int> values;
    values.push_back(3);
    values.push_back(1);
    values.push_back(4);
    values.push_back(1);
    values.push_back(5);

    std::tuple<int, int> result = combine(sum<int>(), max<int>()).run(values);

    D_INTERNAL_ACC_CHECK(std::get<0>(result) == 14);
    D_INTERNAL_ACC_CHECK(std::get<1>(result) == 5);

    return true;
}


/*
test_combine_heterogeneous
  Combines accumulators with differing output types.
  Tests the following:
  - int (sum), double (mean), and size_t (count) coexist in the tuple
  - each slot holds the correct typed result
*/
bool
test_combine_heterogeneous(
)
{
    std::vector<int> values;
    values.push_back(2);
    values.push_back(4);
    values.push_back(6);

    std::tuple<int, double, std::size_t> result =
        combine(sum<int>(), mean<int>(), count<int>()).run(values);

    D_INTERNAL_ACC_CHECK(std::get<0>(result) == 12);
    D_INTERNAL_ACC_CHECK(approx_eq(std::get<1>(result), 4.0));
    D_INTERNAL_ACC_CHECK(std::get<2>(result) == 3);

    return true;
}


/*
test_combine_run_iterator_range
  Drives a combine over a half-open iterator range.
  Tests the following:
  - the iterator-range run overload feeds all combined accumulators
  - a sub-range is respected
*/
bool
test_combine_run_iterator_range(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);
    values.push_back(99);

    std::tuple<int, std::size_t> result =
        combine(sum<int>(), count<int>()).run(values.begin(),
                                              values.begin() + 3);

    D_INTERNAL_ACC_CHECK(std::get<0>(result) == 6);
    D_INTERNAL_ACC_CHECK(std::get<1>(result) == 3);

    return true;
}


/*
test_combine_empty
  Combines over an empty input.
  Tests the following:
  - every combined accumulator finalises its initial state
  - sum and count both report zero
*/
bool
test_combine_empty(
)
{
    std::vector<int> empty_vec;

    std::tuple<int, std::size_t> result =
        combine(sum<int>(), count<int>()).run(empty_vec);

    D_INTERNAL_ACC_CHECK(std::get<0>(result) == 0);
    D_INTERNAL_ACC_CHECK(std::get<1>(result) == 0);

    return true;
}


/*
run_combine_tests
  Aggregates every combine-section test.
  Tests the following:
  - all single/pair/heterogeneous/range/empty combine tests pass
*/
bool
run_combine_tests(
)
{
    return ( test_combine_single()              &&
             test_combine_pair()                &&
             test_combine_heterogeneous()       &&
             test_combine_run_iterator_range()  &&
             test_combine_empty() );
}


NS_END  // testing
NS_END  // djinterp
