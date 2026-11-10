/*******************************************************************************
* djinterp [test]                                    maybe_tests_combinators.cpp
*
* Combinator-factory / pipeline tests for maybe.hpp section IV:
*   test_combinators -- or_else_with, unwrap_or_with, filter_with, expect_with,
*                       all driven through operator| against a maybe.
*
*
* path:      /tests/djinterp/cpp/functional/maybe_tests_combinators.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.01
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <stdexcept>
#include <string>
// djinterp
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_combinators()
{
    bool verdict = true;

    maybe<int> v(5);
    maybe<int> e;

    // or_else_with: present -> value, absent -> default
    record_assertion(verdict, (v | or_else_with(0)) == 5,
        "pipe: or_else_with returns value when present");
    record_assertion(verdict, (e | or_else_with(-1)) == -1,
        "pipe: or_else_with returns default when empty");

    // unwrap_or_with is an alias for or_else_with
    record_assertion(verdict, (e | unwrap_or_with(7)) == 7,
        "pipe: unwrap_or_with aliases or_else_with");

    // filter_with: pass keeps the maybe, fail yields nothing
    maybe<int> kept = v | filter_with(pred_is_positive());
    record_assertion(verdict, kept.has_value() && kept.value() == 5,
        "pipe: filter_with keeps a passing value");

    maybe<int> dropped = v | filter_with(pred_is_even());
    record_assertion(verdict, !dropped.has_value(),
        "pipe: filter_with drops a failing value");

    // filter_with on empty stays empty
    maybe<int> filtered_empty = e | filter_with(pred_is_positive());
    record_assertion(verdict, !filtered_empty.has_value(),
        "pipe: filter_with on empty stays empty");

    // expect_with: present returns value
    record_assertion(verdict, (v | expect_with("nope")) == 5,
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
    record_assertion(verdict, threw && msg == "absent",
        "pipe: expect_with throws with its message when empty");

    // chained pipeline: filter (pass) then or_else
    int chained = v
                | filter_with(pred_is_positive())
                | or_else_with(0);
    record_assertion(verdict, chained == 5,
        "pipe: filter_with then or_else_with composes");

    // chained pipeline: filter (fail) collapses to the default
    int collapsed = v
                  | filter_with(pred_is_even())
                  | or_else_with(-9);
    record_assertion(verdict, collapsed == -9,
        "pipe: a failing filter feeds the or_else default");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
