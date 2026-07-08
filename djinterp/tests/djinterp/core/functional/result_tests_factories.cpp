/******************************************************************************
* djinterp [test]                                   result_tests_factories.cpp
*
* Factory-function tests for result.hpp section III:
*   test_factories -- ok<T, E> and err<T, E>, from lvalue and rvalue sources.
******************************************************************************/

#include <string>
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_factories(test::test_handler& _h)
{
    // ok from rvalue
    result<int, std::string> a = ok<int, std::string>(5);
    test::record_assertion(_h, a.is_ok() && a.value() == 5,
        "factory: ok builds an ok result");

    // ok from lvalue
    int                      lv = 9;
    result<int, std::string> b = ok<int, std::string>(lv);
    test::record_assertion(_h, b.is_ok() && b.value() == 9,
        "factory: ok accepts an lvalue");

    // err from rvalue
    result<int, std::string> c = err<int, std::string>(std::string("oops"));
    test::record_assertion(_h, c.is_err() && c.error() == "oops",
        "factory: err builds an err result");

    // err from lvalue
    std::string              le = "bad";
    result<int, std::string> d = err<int, std::string>(le);
    test::record_assertion(_h, d.is_err() && d.error() == "bad",
        "factory: err accepts an lvalue");

    // distinct T == E still disambiguates via the tag
    result<int, int> same_ok  = ok<int, int>(1);
    result<int, int> same_err = err<int, int>(2);
    test::record_assertion(_h,
        same_ok.is_ok() && same_ok.value() == 1 &&
        same_err.is_err() && same_err.error() == 2,
        "factory: ok/err disambiguate even when T and E coincide");

    return;
}


NS_END  // testing
NS_END  // djinterp
