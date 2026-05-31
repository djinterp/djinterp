#include "accumulator_tests.hpp"

// std
#include <string>
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_combinators_contramap
  Pre-applies a projection to each input before the inner step.
  Tests the following:
  - the adapted accumulator accepts the new input type (std::string)
  - each input is mapped (to its length) and folded by the inner sum
*/
bool
test_combinators_contramap(
)
{
    auto acc = contramap<std::string>(
        sum<int>(),
        [](const std::string& _s) { return static_cast<int>(_s.size()); });

    std::vector<std::string> words;
    words.push_back("a");
    words.push_back("bb");
    words.push_back("ccc");

    D_INTERNAL_ACC_CHECK(acc.run(words) == 6);

    return true;
}


/*
test_combinators_map_output
  Post-applies a transform to the inner accumulator's output.
  Tests the following:
  - a same-type transform rescales the result
  - a type-changing transform (int -> bool) is honoured both ways
*/
bool
test_combinators_map_output(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    auto doubled = map_output(sum<int>(), [](int _x) { return _x * 2; });

    D_INTERNAL_ACC_CHECK(doubled.run(values) == 12);

    auto big = map_output(sum<int>(), [](int _x) { return _x > 5; });

    D_INTERNAL_ACC_CHECK(big.run(values) == true);

    std::vector<int> small;
    small.push_back(1);
    small.push_back(1);

    auto big2 = map_output(sum<int>(), [](int _x) { return _x > 5; });

    D_INTERNAL_ACC_CHECK(big2.run(small) == false);

    return true;
}


/*
test_combinators_filtered
  Gates the inner accumulator's input with a predicate.
  Tests the following:
  - only matching elements reach the inner step
  - non-matching elements are dropped (sum of evens here)
*/
bool
test_combinators_filtered(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);
    values.push_back(4);

    D_INTERNAL_ACC_CHECK(filtered(sum<int>(), is_even()).run(values) == 6);

    return true;
}


/*
test_combinators_take
  Caps how many inputs the inner accumulator observes.
  Tests the following:
  - only the first n inputs are folded
  - take(0) folds nothing (initial state finalised)
  - take(n >= size) folds the entire input
*/
bool
test_combinators_take(
)
{
    std::vector<int> values;
    values.push_back(10);
    values.push_back(20);
    values.push_back(30);

    D_INTERNAL_ACC_CHECK(take(sum<int>(), 2).run(values) == 30);
    D_INTERNAL_ACC_CHECK(take(sum<int>(), 0).run(values) == 0);
    D_INTERNAL_ACC_CHECK(take(sum<int>(), 10).run(values) == 60);

    return true;
}


/*
test_combinators_composition
  Stacks combinators to confirm they nest cleanly.
  Tests the following:
  - filtered feeds an inner sum, then map_output post-transforms it
  - the composed pipeline yields the expected scalar
*/
bool
test_combinators_composition(
)
{
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);
    values.push_back(4);

    // sum the evens (2 + 4 = 6), then add one -> 7
    auto pipeline = map_output(
        filtered(sum<int>(), is_even()),
        [](int _x) { return _x + 1; });

    D_INTERNAL_ACC_CHECK(pipeline.run(values) == 7);

    return true;
}


/*
run_combinators_tests
  Aggregates every combinators-section test.
  Tests the following:
  - all contramap/map_output/filtered/take and composition tests pass
*/
bool
run_combinators_tests(
)
{
    return ( test_combinators_contramap()   &&
             test_combinators_map_output()  &&
             test_combinators_filtered()    &&
             test_combinators_take()        &&
             test_combinators_composition() );
}


NS_END  // testing
NS_END  // djinterp
