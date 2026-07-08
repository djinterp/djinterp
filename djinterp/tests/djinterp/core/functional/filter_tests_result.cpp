#include "filter_tests.hpp"

// std
#include <string>
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_result_success
  Verifies the success constructor of filter_result and its inspection
  surface for a non-empty result.
  Tests the following:
  - ok() is true and status() is success for a non-empty result
  - count(), elements(), and indices() report the stored data
  - empty() is false
*/
bool
test_result_success(
)
{
    std::vector<int>         elems = { 10, 20, 30 };
    std::vector<std::size_t> idx   = { 0, 2, 4 };

    filter_result<int> r(std::move(elems), std::move(idx));

    D_INTERNAL_FLT_CHECK(r.ok());
    D_INTERNAL_FLT_CHECK(r.status() == filter_result_status::success);
    D_INTERNAL_FLT_CHECK(!r.empty());
    D_INTERNAL_FLT_CHECK(r.count() == 3);
    D_INTERNAL_FLT_CHECK(r.elements().size() == 3);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 10);
    D_INTERNAL_FLT_CHECK(r.elements()[2] == 30);
    D_INTERNAL_FLT_CHECK(r.indices().size() == 3);
    D_INTERNAL_FLT_CHECK(r.indices()[1] == 2);
    D_INTERNAL_FLT_CHECK(r.error_message().empty());

    return true;
}


/*
test_result_empty_status
  Verifies that the success constructor downgrades status to `empty` when the
  element vector is empty -- so an empty filter is reported as empty, not ok.
  Tests the following:
  - an empty element vector yields status empty and ok() false
  - empty() is true and count() is zero
*/
bool
test_result_empty_status(
)
{
    std::vector<int>         elems;
    std::vector<std::size_t> idx;

    filter_result<int> r(std::move(elems), std::move(idx));

    D_INTERNAL_FLT_CHECK(r.status() == filter_result_status::empty);
    D_INTERNAL_FLT_CHECK(!r.ok());
    D_INTERNAL_FLT_CHECK(r.empty());
    D_INTERNAL_FLT_CHECK(r.count() == 0);

    return true;
}


/*
test_result_error_ctor
  Verifies the error constructor records a status and message and reports
  not-ok.
  Tests the following:
  - an error-status result is not ok and carries its message
  - an invalid-status result reports the invalid status
*/
bool
test_result_error_ctor(
)
{
    filter_result<int> err(filter_result_status::error, "boom");
    D_INTERNAL_FLT_CHECK(!err.ok());
    D_INTERNAL_FLT_CHECK(err.status() == filter_result_status::error);
    D_INTERNAL_FLT_CHECK(err.error_message() == std::string("boom"));
    D_INTERNAL_FLT_CHECK(err.empty());

    filter_result<int> inv(filter_result_status::invalid);
    D_INTERNAL_FLT_CHECK(!inv.ok());
    D_INTERNAL_FLT_CHECK(inv.status() == filter_result_status::invalid);

    return true;
}


/*
test_result_take_elements
  Verifies take_elements moves the stored elements out intact.
  Tests the following:
  - the moved-out vector holds the original elements in order
*/
bool
test_result_take_elements(
)
{
    std::vector<int>         elems = { 1, 2, 3 };
    std::vector<std::size_t> idx   = { 0, 1, 2 };

    filter_result<int> r(std::move(elems), std::move(idx));
    std::vector<int>   taken = r.take_elements();

    D_INTERNAL_FLT_CHECK(taken.size() == 3);
    D_INTERNAL_FLT_CHECK(taken[0] == 1);
    D_INTERNAL_FLT_CHECK(taken[2] == 3);

    return true;
}


/*
test_result_iteration
  Verifies filter_result exposes begin/end so it is range-iterable.
  Tests the following:
  - iterating the result visits its elements in order, summing correctly
*/
bool
test_result_iteration(
)
{
    std::vector<int>         elems = { 4, 5, 6 };
    std::vector<std::size_t> idx   = { 0, 1, 2 };

    filter_result<int> r(std::move(elems), std::move(idx));

    int sum   = 0;
    int steps = 0;
    for (std::vector<int>::const_iterator it = r.begin(); it != r.end(); ++it)
    {
        sum += *it;
        ++steps;
    }

    D_INTERNAL_FLT_CHECK(steps == 3);
    D_INTERNAL_FLT_CHECK(sum == 15);

    return true;
}


/*
run_result_tests
  Aggregates every filter_result test.
  Tests the following:
  - all success / empty-status / error-ctor / take_elements / iteration
    tests pass
*/
bool
run_result_tests(
)
{
    return ( test_result_success()       &&
             test_result_empty_status()  &&
             test_result_error_ctor()    &&
             test_result_take_elements() &&
             test_result_iteration() );
}


NS_END  // testing
NS_END  // djinterp
