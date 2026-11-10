/*******************************************************************************
* djinterp [test]                                   result_tests_conversions.cpp
*
* maybe-conversion tests for result<T, E>: the ok() / err() member accessors
* (section I) and the free to_maybe (section VI).
*
*   test_conversions -- result::ok(), result::err(), and to_maybe across both
*                       branches.
*
*
* path:      /tests/djinterp/cpp/functional/result_tests_conversions.cpp
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


using ri = result<int, std::string>;


bool test_conversions()
{
    bool verdict = true;

    ri okv  = ok<int, std::string>(5);
    ri errv = err<int, std::string>("bad");

    // ok(): ok -> just(value), err -> nothing
    maybe<int> ok_just = okv.ok();
    record_assertion(verdict, ok_just.has_value() && ok_just.value() == 5,
        "convert: ok().ok() yields just(value)");

    maybe<int> ok_none = errv.ok();
    record_assertion(verdict, !ok_none.has_value(),
        "convert: err().ok() yields nothing");

    // err(): err -> just(error), ok -> nothing
    maybe<std::string> err_just = errv.err();
    record_assertion(verdict,
        err_just.has_value() && err_just.value() == "bad",
        "convert: err().err() yields just(error)");

    maybe<std::string> err_none = okv.err();
    record_assertion(verdict, !err_none.has_value(),
        "convert: ok().err() yields nothing");

    // free to_maybe: discards the error, mirrors result::ok()
    maybe<int> tm_ok = to_maybe(okv);
    record_assertion(verdict, tm_ok.has_value() && tm_ok.value() == 5,
        "convert: to_maybe(ok) yields just(value)");

    maybe<int> tm_err = to_maybe(errv);
    record_assertion(verdict, !tm_err.has_value(),
        "convert: to_maybe(err) yields nothing");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
