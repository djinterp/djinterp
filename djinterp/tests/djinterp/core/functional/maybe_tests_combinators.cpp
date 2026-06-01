/******************************************************************************
* djinterp [test]                                  maybe_tests_combinators.cpp
*
* Combinator-factory / pipeline tests for maybe.hpp section IV:
*   test_combinators -- or_else_with, unwrap_or_with, filter_with, expect_with,
*                       all driven through operator| against a maybe.
******************************************************************************/

#include <string>
#include <stdexcept>
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_combinators(test::test_handler& _h)
{
    maybe<int> v(5);
    maybe<int> e;

    // or_else_with: present -> value, absent -> default
    test::record_assertion(_h, (v | or_else_with(0)) == 5,
        "pipe: or_else_with returns value when present");
    test::record_assertion(_h, (e | or_else_with(-1)) == -1,
        "pipe: or_else_with returns default when empty");

    // unwrap_or_with is an alias for or_else_with
    test::record_assertion(_h, (e | unwrap_or_with(7)) == 7,
        "pipe: unwrap_or_with aliases or_else_with");

    // filter_with: pass keeps the maybe, fail yields nothing
    maybe<int> kept = v | filter_with(pred_is_positive());
    test::record_assertion(_h, kept.has_value() && kept.value() == 5,
        "pipe: filter_with keeps a passing value");

    maybe<int> dropped = v | filter_with(pred_is_even());
    test::record_assertion(_h, !dropped.has_value(),
        "pipe: filter_with drops a failing value");

    // filter_with on empty stays empty
    maybe<int> filtered_empty = e | filter_with(pred_is_positive());
    test::record_assertion(_h, !filtered_empty.has_value(),
        "pipe: filter_with on empty stays empty");

    // expect_with: present returns value
    test::record_assertion(_h, (v | expect_with("nope")) == 5,
        "pipe: expect_with returns value when present");

    // expect_with: empty throws with the message
    bool        threw = false;
    std::string msg;
    try
    {
        (void)(e | expect_with("absent"));
    }
    catch (const std::runtime_error& _ex)
    {
        threw = true;
        msg   = _ex.what();
    }
    test::record_assertion(_h, threw && msg == "absent",
        "pipe: expect_with throws with its message when empty");

    // chained pipeline: filter (pass) then or_else
    int chained = v
                | filter_with(pred_is_positive())
                | or_else_with(0);
    test::record_assertion(_h, chained == 5,
        "pipe: filter_with then or_else_with composes");

    // chained pipeline: filter (fail) collapses to the default
    int collapsed = v
                  | filter_with(pred_is_even())
                  | or_else_with(-9);
    test::record_assertion(_h, collapsed == -9,
        "pipe: a failing filter feeds the or_else default");

    return;
}


NS_END  // testing
NS_END  // djinterp
