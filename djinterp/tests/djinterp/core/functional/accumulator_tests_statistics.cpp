#include "accumulator_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_statistics_mean
  Computes the arithmetic mean.
  Tests the following:
  - an exact integer mean is reported as a double
  - a single value reduces to itself
*/
bool
test_statistics_mean(
)
{
    std::vector<int> values;
    values.push_back(2);
    values.push_back(4);
    values.push_back(6);

    D_INTERNAL_ACC_CHECK(approx_eq(mean<int>().run(values), 4.0));

    std::vector<int> single;
    single.push_back(5);

    D_INTERNAL_ACC_CHECK(approx_eq(mean<int>().run(single), 5.0));

    return true;
}


/*
test_statistics_variance
  Computes the population variance via Welford's algorithm.
  Tests the following:
  - a two-element set yields the population variance (not sample)
  - a three-element set matches the hand-computed value
*/
bool
test_statistics_variance(
)
{
    std::vector<int> pair_vals;
    pair_vals.push_back(1);
    pair_vals.push_back(3);

    // population variance of {1,3}: mean 2, ((1)+(1))/2 = 1.0
    D_INTERNAL_ACC_CHECK(approx_eq(variance<int>().run(pair_vals), 1.0));

    std::vector<int> triple;
    triple.push_back(2);
    triple.push_back(4);
    triple.push_back(6);

    // population variance of {2,4,6}: (4+0+4)/3 = 8/3
    D_INTERNAL_ACC_CHECK(
        approx_eq(variance<int>().run(triple), 8.0 / 3.0, 1e-9));

    return true;
}


/*
test_statistics_stddev
  Computes the population standard deviation (Newton sqrt of variance).
  Tests the following:
  - a rational case ({1,3}) gives exactly 1.0
  - an irrational case ({2,4,6}) converges to sqrt(8/3)
*/
bool
test_statistics_stddev(
)
{
    std::vector<int> pair_vals;
    pair_vals.push_back(1);
    pair_vals.push_back(3);

    D_INTERNAL_ACC_CHECK(approx_eq(stddev<int>().run(pair_vals), 1.0, 1e-9));

    std::vector<int> triple;
    triple.push_back(2);
    triple.push_back(4);
    triple.push_back(6);

    // sqrt(8/3) ~ 1.632993161855452
    D_INTERNAL_ACC_CHECK(
        approx_eq(stddev<int>().run(triple), 1.632993161855452, 1e-6));

    return true;
}


/*
test_statistics_degenerate
  Exercises the guarded edge cases of the statistics finalizers.
  Tests the following:
  - mean of an empty set is 0 (count == 0 guard)
  - variance/stddev with fewer than two samples are 0 (n < 2 guard)
  - stddev of a zero-variance set is 0 (var <= 0 guard, all values equal)
*/
bool
test_statistics_degenerate(
)
{
    std::vector<int> empty_vec;

    D_INTERNAL_ACC_CHECK(approx_eq(mean<int>().run(empty_vec), 0.0));
    D_INTERNAL_ACC_CHECK(approx_eq(variance<int>().run(empty_vec), 0.0));
    D_INTERNAL_ACC_CHECK(approx_eq(stddev<int>().run(empty_vec), 0.0));

    std::vector<int> single;
    single.push_back(7);

    D_INTERNAL_ACC_CHECK(approx_eq(variance<int>().run(single), 0.0));
    D_INTERNAL_ACC_CHECK(approx_eq(stddev<int>().run(single), 0.0));

    std::vector<int> constant;
    constant.push_back(4);
    constant.push_back(4);
    constant.push_back(4);

    D_INTERNAL_ACC_CHECK(approx_eq(variance<int>().run(constant), 0.0));
    D_INTERNAL_ACC_CHECK(approx_eq(stddev<int>().run(constant), 0.0));

    return true;
}


/*
run_statistics_tests
  Aggregates every statistics-section test.
  Tests the following:
  - all mean/variance/stddev tests pass
*/
bool
run_statistics_tests(
)
{
    return ( test_statistics_mean()       &&
             test_statistics_variance()   &&
             test_statistics_stddev()     &&
             test_statistics_degenerate() );
}


NS_END  // testing
NS_END  // djinterp
