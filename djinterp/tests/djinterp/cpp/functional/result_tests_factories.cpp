/*******************************************************************************
* djinterp [test]                                     result_tests_factories.cpp
*
* Factory-function tests for result.hpp section III:
*   test_factories -- ok<T, E> and err<T, E>, from lvalue and rvalue sources.
*
*
* path:      /tests/djinterp/cpp/functional/result_tests_factories.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <string>
// djinterp
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_factories()
{
    bool verdict = true;

    // ok from rvalue
    result<int, std::string> a = ok<int, std::string>(5);
    record_assertion(verdict, a.is_ok() && a.value() == 5,
        "factory: ok builds an ok result");

    // ok from lvalue
    int                      lv = 9;
    result<int, std::string> b = ok<int, std::string>(lv);
    record_assertion(verdict, b.is_ok() && b.value() == 9,
        "factory: ok accepts an lvalue");

    // err from rvalue
    result<int, std::string> c = err<int, std::string>(std::string("oops"));
    record_assertion(verdict, c.is_err() && c.error() == "oops",
        "factory: err builds an err result");

    // err from lvalue
    std::string              le = "bad";
    result<int, std::string> d = err<int, std::string>(le);
    record_assertion(verdict, d.is_err() && d.error() == "bad",
        "factory: err accepts an lvalue");

    // distinct T == E still disambiguates via the tag
    result<int, int> same_ok  = ok<int, int>(1);
    result<int, int> same_err = err<int, int>(2);
    record_assertion(verdict,
        same_ok.is_ok() && same_ok.value() == 1 &&
        same_err.is_err() && same_err.error() == 2,
        "factory: ok/err disambiguate even when T and E coincide");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
