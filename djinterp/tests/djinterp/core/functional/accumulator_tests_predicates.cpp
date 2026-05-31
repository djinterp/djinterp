#include "accumulator_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_predicates_all_match
  Folds a conjunction over a predicate.
  Tests the following:
  - all-satisfying input yields true
  - one violation flips the result to false
*/
bool
test_predicates_all_match(
)
{
    std::vector<int> evens;
    evens.push_back(2);
    evens.push_back(4);
    evens.push_back(6);

    D_INTERNAL_ACC_CHECK(all_match<int>(is_even()).run(evens) == true);

    std::vector<int> mixed;
    mixed.push_back(2);
    mixed.push_back(3);
    mixed.push_back(4);

    D_INTERNAL_ACC_CHECK(all_match<int>(is_even()).run(mixed) == false);

    return true;
}


/*
test_predicates_any_match
  Folds a disjunction over a predicate.
  Tests the following:
  - at least one match yields true
  - zero matches yields false
*/
bool
test_predicates_any_match(
)
{
    std::vector<int> has_even;
    has_even.push_back(1);
    has_even.push_back(3);
    has_even.push_back(4);

    D_INTERNAL_ACC_CHECK(any_match<int>(is_even()).run(has_even) == true);

    std::vector<int> all_odd;
    all_odd.push_back(1);
    all_odd.push_back(3);
    all_odd.push_back(5);

    D_INTERNAL_ACC_CHECK(any_match<int>(is_even()).run(all_odd) == false);

    return true;
}


/*
test_predicates_none_match
  Folds a negated disjunction over a predicate.
  Tests the following:
  - zero matches yields true
  - one match flips the result to false
*/
bool
test_predicates_none_match(
)
{
    std::vector<int> all_odd;
    all_odd.push_back(1);
    all_odd.push_back(3);
    all_odd.push_back(5);

    D_INTERNAL_ACC_CHECK(none_match<int>(is_even()).run(all_odd) == true);

    std::vector<int> has_even;
    has_even.push_back(1);
    has_even.push_back(2);
    has_even.push_back(3);

    D_INTERNAL_ACC_CHECK(none_match<int>(is_even()).run(has_even) == false);

    return true;
}


/*
test_predicates_empty_edges
  Confirms the vacuous-truth conventions on empty input.
  Tests the following:
  - all_match over no elements is true (vacuously)
  - any_match over no elements is false
  - none_match over no elements is true (vacuously)
*/
bool
test_predicates_empty_edges(
)
{
    std::vector<int> empty_vec;

    D_INTERNAL_ACC_CHECK(all_match<int>(is_even()).run(empty_vec) == true);
    D_INTERNAL_ACC_CHECK(any_match<int>(is_even()).run(empty_vec) == false);
    D_INTERNAL_ACC_CHECK(none_match<int>(is_even()).run(empty_vec) == true);

    return true;
}


/*
run_predicates_tests
  Aggregates every predicates-section test.
  Tests the following:
  - all all_match/any_match/none_match tests pass
*/
bool
run_predicates_tests(
)
{
    return ( test_predicates_all_match()   &&
             test_predicates_any_match()   &&
             test_predicates_none_match()  &&
             test_predicates_empty_edges() );
}


NS_END  // testing
NS_END  // djinterp
