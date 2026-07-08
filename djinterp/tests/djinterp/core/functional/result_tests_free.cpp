/******************************************************************************
* djinterp [test]                                        result_tests_free.cpp
*
* Free-function helper tests for result.hpp section VI:
*   test_collect -- container of result<T, E> -> result<vector<T>, E>: ok iff
*                   every element is ok, else the first err (short-circuit).
*   test_combine -- combine two results via a binary function; ok iff both
*                   ok, else the first err.
******************************************************************************/

#include <string>
#include <vector>
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


using ri = result<int, std::string>;


void test_collect(test::test_handler& _h)
{
    // all ok -> ok(vector) preserving order
    std::vector<ri> all_ok;
    all_ok.push_back(ok<int, std::string>(1));
    all_ok.push_back(ok<int, std::string>(2));
    all_ok.push_back(ok<int, std::string>(3));

    result<std::vector<int>, std::string> collected = collect(all_ok);
    bool ok_case = collected.is_ok()
                && collected.value().size() == 3
                && collected.value()[0] == 1
                && collected.value()[1] == 2
                && collected.value()[2] == 3;
    test::record_assertion(_h, ok_case,
        "collect: all-ok yields ok(vector) in order");

    // first err short-circuits and is returned
    std::vector<ri> with_err;
    with_err.push_back(ok<int, std::string>(1));
    with_err.push_back(err<int, std::string>("stop"));
    with_err.push_back(ok<int, std::string>(3));

    result<std::vector<int>, std::string> short_circuit = collect(with_err);
    test::record_assertion(_h,
        short_circuit.is_err() && short_circuit.error() == "stop",
        "collect: first err short-circuits the collection");

    // empty container -> ok(empty vector) (vacuously all ok)
    std::vector<ri> empty_in;
    result<std::vector<int>, std::string> empty_collected = collect(empty_in);
    test::record_assertion(_h,
        empty_collected.is_ok() && empty_collected.value().empty(),
        "collect: empty input yields ok(empty vector)");

    return;
}


void test_combine(test::test_handler& _h)
{
    ri a = ok<int, std::string>(3);
    ri b = ok<int, std::string>(4);
    ri ea = err<int, std::string>("left");
    ri eb = err<int, std::string>("right");

    // both ok -> ok(f(a, b))
    result<int, std::string> sum = combine(a, b, fn_add());
    test::record_assertion(_h, sum.is_ok() && sum.value() == 7,
        "combine: both ok combines via the function");

    // left err -> that err (function not invoked)
    result<int, std::string> left_err = combine(ea, b, fn_add());
    test::record_assertion(_h,
        left_err.is_err() && left_err.error() == "left",
        "combine: left err short-circuits");

    // left ok, right err -> right err
    result<int, std::string> right_err = combine(a, eb, fn_add());
    test::record_assertion(_h,
        right_err.is_err() && right_err.error() == "right",
        "combine: right err is returned when left is ok");

    // both err -> first (left) err wins
    result<int, std::string> both_err = combine(ea, eb, fn_add());
    test::record_assertion(_h,
        both_err.is_err() && both_err.error() == "left",
        "combine: with both err, the left error wins");

    return;
}


NS_END  // testing
NS_END  // djinterp
