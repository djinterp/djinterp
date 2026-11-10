/*******************************************************************************
* djinterp [test]                                   result_tests_combinators.cpp
*
* Combinator-factory / pipeline tests for result.hpp section IV:
*   test_combinators -- or_value_with, map_err_with, unwrap_with, all driven
*                       through operator| against a result.
*
*
* path:      /tests/djinterp/cpp/functional/result_tests_combinators.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <cstddef>
#include <stdexcept>
#include <string>
// djinterp
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


using ri = result<int, std::string>;


bool test_combinators()
{
    bool verdict = true;

    ri okv  = ok<int, std::string>(5);
    ri errv = err<int, std::string>("e");

    // or_value_with: ok -> value, err -> default
    record_assertion(verdict, (okv | or_value_with(0)) == 5,
        "pipe: or_value_with returns value when ok");
    record_assertion(verdict, (errv | or_value_with(-1)) == -1,
        "pipe: or_value_with returns default when err");

    // map_err_with: transforms err side, passes ok through
    result<int, std::size_t> mapped_err = errv | map_err_with(DFnErrLen());
    record_assertion(verdict,
        mapped_err.is_err() && mapped_err.error() == 1u,
        "pipe: map_err_with transforms the error");

    result<int, std::size_t> mapped_ok = okv | map_err_with(DFnErrLen());
    record_assertion(verdict,
        mapped_ok.is_ok() && mapped_ok.value() == 5,
        "pipe: map_err_with leaves an ok value untouched");

    // unwrap_with: ok -> value
    record_assertion(verdict, (okv | unwrap_with("nope")) == 5,
        "pipe: unwrap_with returns value when ok");

    // unwrap_with: err -> throws with message
    bool        threw = false;
    std::string what;
    try
    {
        (void)(errv | unwrap_with("kaboom"));
    }
    catch (const std::runtime_error& _ex)
    {
        threw = true;
        what  = _ex.what();
    }
    record_assertion(verdict, threw && what == "kaboom",
        "pipe: unwrap_with throws with its message when err");

    // chained pipeline: map the error, then extract or default
    int chained = errv
                | map_err_with(DFnErrPrefix())
                | or_value_with(-7);
    record_assertion(verdict, chained == -7,
        "pipe: map_err_with then or_value_with composes on an err");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
