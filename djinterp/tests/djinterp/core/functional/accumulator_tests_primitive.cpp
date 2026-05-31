#include "accumulator_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_primitive_construct_and_state
  Constructs a pre-built accumulator and inspects its initial state.
  Tests the following:
  - the forwarding constructor seeds m_state from the factory's value
  - state() exposes the live state by const reference
  - sum<int> starts at the value-initialised additive identity (0)
*/
bool
test_primitive_construct_and_state(
)
{
    accumulator<int, int, int,
                internal::sum_step<int>,
                internal::identity_final<int> > acc = sum<int>();

    D_INTERNAL_ACC_CHECK(acc.state() == 0);
    D_INTERNAL_ACC_CHECK(acc.finalize() == 0);

    return true;
}


/*
test_primitive_step_chaining
  Drives a sum one value at a time.
  Tests the following:
  - step() mutates the state in place
  - step() returns *this so calls can be chained
  - the chained result equals the running total
*/
bool
test_primitive_step_chaining(
)
{
    auto acc = sum<int>();

    acc.step(1).step(2).step(3);

    D_INTERNAL_ACC_CHECK(acc.state() == 6);
    D_INTERNAL_ACC_CHECK(acc.finalize() == 6);

    return true;
}


/*
test_primitive_finalize_is_pure
  Verifies finalize() does not disturb the accumulator state.
  Tests the following:
  - finalize() is const and may be called repeatedly
  - repeated finalize() returns a stable value
  - stepping after a finalize() continues from the prior state
*/
bool
test_primitive_finalize_is_pure(
)
{
    auto acc = sum<int>();

    acc.step(10);

    D_INTERNAL_ACC_CHECK(acc.finalize() == 10);
    D_INTERNAL_ACC_CHECK(acc.finalize() == 10);

    acc.step(5);

    D_INTERNAL_ACC_CHECK(acc.finalize() == 15);

    return true;
}


/*
test_primitive_run_container
  Folds an entire container in one call.
  Tests the following:
  - run(container) visits every element via range-based for
  - the finalized output matches the manual total
*/
bool
test_primitive_run_container(
)
{
    std::vector<int> values;
    values.push_back(4);
    values.push_back(8);
    values.push_back(15);
    values.push_back(16);

    D_INTERNAL_ACC_CHECK(sum<int>().run(values) == 43);

    return true;
}


/*
test_primitive_run_iterator_range
  Folds a half-open iterator range.
  Tests the following:
  - run(first, last) consumes [first, last)
  - a sub-range is respected (the last element is excluded)
*/
bool
test_primitive_run_iterator_range(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);
    values.push_back(99);

    // exclude the final element
    D_INTERNAL_ACC_CHECK(
        sum<int>().run(values.begin(), values.begin() + 3) == 6);

    return true;
}


/*
test_primitive_run_raw_array
  Folds a raw pointer + count buffer.
  Tests the following:
  - run(data, count) walks count elements from data
  - the array overload is selected over the iterator overload
*/
bool
test_primitive_run_raw_array(
)
{
    const int   data[] = { 5, 10, 20, 40 };
    std::size_t count  = sizeof(data) / sizeof(data[0]);

    D_INTERNAL_ACC_CHECK(sum<int>().run(data, count) == 75);

    return true;
}


/*
test_primitive_run_empty_inputs
  Exercises every run() overload with no elements.
  Tests the following:
  - run(empty container) finalizes the untouched initial state
  - run(first == last) is a no-op
  - run(data, 0) is a no-op
*/
bool
test_primitive_run_empty_inputs(
)
{
    std::vector<int> empty_vec;
    const int*       null_data = nullptr;

    D_INTERNAL_ACC_CHECK(sum<int>().run(empty_vec) == 0);
    D_INTERNAL_ACC_CHECK(
        sum<int>().run(empty_vec.begin(), empty_vec.end()) == 0);
    D_INTERNAL_ACC_CHECK(sum<int>().run(null_data, 0) == 0);

    return true;
}


/*
test_primitive_functor_accessors
  Uses the stored functors directly via the const accessors.
  Tests the following:
  - step_fn() returns the step functor, which mutates a supplied state
  - finalize_fn() returns the finalize functor, which maps a state to output
*/
bool
test_primitive_functor_accessors(
)
{
    auto acc = sum<int>();

    int scratch = 100;
    acc.step_fn()(scratch, 23);

    D_INTERNAL_ACC_CHECK(scratch == 123);
    D_INTERNAL_ACC_CHECK(acc.finalize_fn()(scratch) == 123);

    return true;
}


/*
test_primitive_make_accumulator
  Builds a bespoke accumulator from raw functors.
  Tests the following:
  - make_accumulator deduces State/Step/Final and fixes Input/Output
  - a hand-written running-sum behaves like the pre-built sum
*/
bool
test_primitive_make_accumulator(
)
{
    auto acc = make_accumulator<int, int>(
        0,
        [](int& _s, const int& _v) { _s += _v; },
        [](const int& _s) { return _s; });

    std::vector<int> values;
    values.push_back(7);
    values.push_back(8);
    values.push_back(9);

    D_INTERNAL_ACC_CHECK(acc.run(values) == 24);

    return true;
}


/*
test_primitive_copy_independence
  Confirms copies progress independently after construction.
  Tests the following:
  - a copy captures the source's state at the moment of copying
  - stepping one copy leaves the other unchanged
*/
bool
test_primitive_copy_independence(
)
{
    auto original = sum<int>();
    auto twin     = original;

    original.step(50);

    D_INTERNAL_ACC_CHECK(original.finalize() == 50);
    D_INTERNAL_ACC_CHECK(twin.finalize() == 0);

    return true;
}


/*
run_primitive_tests
  Aggregates every primitive-section test.
  Tests the following:
  - all accumulator-class and make_accumulator tests pass
*/
bool
run_primitive_tests(
)
{
    return ( test_primitive_construct_and_state() &&
             test_primitive_step_chaining()       &&
             test_primitive_finalize_is_pure()    &&
             test_primitive_run_container()       &&
             test_primitive_run_iterator_range()  &&
             test_primitive_run_raw_array()       &&
             test_primitive_run_empty_inputs()    &&
             test_primitive_functor_accessors()   &&
             test_primitive_make_accumulator()    &&
             test_primitive_copy_independence() );
}


NS_END  // testing
NS_END  // djinterp
