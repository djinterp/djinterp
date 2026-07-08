/******************************************************************************
* djinterp [test]                                 result_tests_combinators.cpp
*
* Combinator-factory / pipeline tests for result.hpp section IV:
*   test_combinators -- or_value_with, map_err_with, unwrap_with, all driven
*                       through operator| against a result.
******************************************************************************/

#include <string>
#include <cstddef>
#include <stdexcept>
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


using ri = result<int, std::string>;


void test_combinators(test::test_handler& _h)
{
    ri okv  = ok<int, std::string>(5);
    ri errv = err<int, std::string>("e");

    // or_value_with: ok -> value, err -> default
    test::record_assertion(_h, (okv | or_value_with(0)) == 5,
        "pipe: or_value_with returns value when ok");
    test::record_assertion(_h, (errv | or_value_with(-1)) == -1,
        "pipe: or_value_with returns default when err");

    // map_err_with: transforms err side, passes ok through
    result<int, std::size_t> mapped_err = errv | map_err_with(fn_err_len());
    test::record_assertion(_h,
        mapped_err.is_err() && mapped_err.error() == 1u,
        "pipe: map_err_with transforms the error");

    result<int, std::size_t> mapped_ok = okv | map_err_with(fn_err_len());
    test::record_assertion(_h,
        mapped_ok.is_ok() && mapped_ok.value() == 5,
        "pipe: map_err_with leaves an ok value untouched");

    // unwrap_with: ok -> value
    test::record_assertion(_h, (okv | unwrap_with("nope")) == 5,
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
    test::record_assertion(_h, threw && what == "kaboom",
        "pipe: unwrap_with throws with its message when err");

    // chained pipeline: map the error, then extract or default
    int chained = errv
                | map_err_with(fn_err_prefix())
                | or_value_with(-7);
    test::record_assertion(_h, chained == -7,
        "pipe: map_err_with then or_value_with composes on an err");

    return;
}


NS_END  // testing
NS_END  // djinterp
