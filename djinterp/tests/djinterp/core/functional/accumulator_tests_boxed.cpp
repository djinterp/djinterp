#include "accumulator_tests.hpp"

// std
#include <list>
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_boxed_run_container
  Folds a container through a type-erased accumulator.
  Tests the following:
  - box_accumulator wraps an unboxed sum
  - run(container) reproduces the unboxed result
*/
bool
test_boxed_run_container(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    boxed_accumulator<int, int> boxed = box_accumulator(sum<int>());

    D_INTERNAL_ACC_CHECK(boxed.run(values) == 6);

    return true;
}


/*
test_boxed_step_finalize
  Drives a boxed accumulator one value at a time.
  Tests the following:
  - step() forwards through the erased interface and chains
  - finalize() produces the erased output
*/
bool
test_boxed_step_finalize(
)
{
    boxed_accumulator<int, int> boxed = box_accumulator(sum<int>());

    boxed.step(5).step(7);

    D_INTERNAL_ACC_CHECK(boxed.finalize() == 12);

    return true;
}


/*
test_boxed_run_non_vector_container
  Folds a non-vector container through the boxed run path.
  Tests the following:
  - run() copies an arbitrary range (std::list) into its working buffer
  - the fold result is unaffected by the source container type
*/
bool
test_boxed_run_non_vector_container(
)
{
    std::list<int> values;
    values.push_back(4);
    values.push_back(8);
    values.push_back(15);

    boxed_accumulator<int, int> boxed = box_accumulator(sum<int>());

    D_INTERNAL_ACC_CHECK(boxed.run(values) == 27);

    return true;
}


/*
test_boxed_factory_deduction
  Confirms box_accumulator carries the inner accumulator's types.
  Tests the following:
  - box_accumulator deduces (input, output) from the inner typedefs
  - a double-producing inner (mean) survives erasure with its output type
  - direct construction of boxed_accumulator works equivalently
*/
bool
test_boxed_factory_deduction(
)
{
    std::vector<int> values;
    values.push_back(2);
    values.push_back(4);
    values.push_back(6);

    boxed_accumulator<int, double> boxed_mean = box_accumulator(mean<int>());

    D_INTERNAL_ACC_CHECK(approx_eq(boxed_mean.run(values), 4.0));

    boxed_accumulator<int, int> direct(sum<int>());

    D_INTERNAL_ACC_CHECK(direct.run(values) == 12);

    return true;
}


/*
run_boxed_tests
  Aggregates every boxed-section test.
  Tests the following:
  - all boxed_accumulator/box_accumulator tests pass
*/
bool
run_boxed_tests(
)
{
    return ( test_boxed_run_container()            &&
             test_boxed_step_finalize()            &&
             test_boxed_run_non_vector_container() &&
             test_boxed_factory_deduction() );
}


NS_END  // testing
NS_END  // djinterp
