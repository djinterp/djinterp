/******************************************************************************
* djinterp [test]                                 result_tests_conversions.cpp
*
* maybe-conversion tests for result<T, E>: the ok() / err() member accessors
* (section I) and the free to_maybe (section VI).
*
*   test_conversions -- result::ok(), result::err(), and to_maybe across both
*                       branches.
******************************************************************************/

#include <string>
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


using ri = result<int, std::string>;


void test_conversions(test::test_handler& _h)
{
    ri okv  = ok<int, std::string>(5);
    ri errv = err<int, std::string>("bad");

    // ok(): ok -> just(value), err -> nothing
    maybe<int> ok_just = okv.ok();
    test::record_assertion(_h, ok_just.has_value() && ok_just.value() == 5,
        "convert: ok().ok() yields just(value)");

    maybe<int> ok_none = errv.ok();
    test::record_assertion(_h, !ok_none.has_value(),
        "convert: err().ok() yields nothing");

    // err(): err -> just(error), ok -> nothing
    maybe<std::string> err_just = errv.err();
    test::record_assertion(_h,
        err_just.has_value() && err_just.value() == "bad",
        "convert: err().err() yields just(error)");

    maybe<std::string> err_none = okv.err();
    test::record_assertion(_h, !err_none.has_value(),
        "convert: ok().err() yields nothing");

    // free to_maybe: discards the error, mirrors result::ok()
    maybe<int> tm_ok = to_maybe(okv);
    test::record_assertion(_h, tm_ok.has_value() && tm_ok.value() == 5,
        "convert: to_maybe(ok) yields just(value)");

    maybe<int> tm_err = to_maybe(errv);
    test::record_assertion(_h, !tm_err.has_value(),
        "convert: to_maybe(err) yields nothing");

    return;
}


NS_END  // testing
NS_END  // djinterp
