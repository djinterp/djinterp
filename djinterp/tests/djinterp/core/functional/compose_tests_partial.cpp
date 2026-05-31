/******************************************************************************
* djinterp [test]                                  compose_tests_partial.cpp
*
*   Tests for compose.hpp Section IV: partial_back, which binds the last
* argument of a function.  Covers binding the trailing argument of a
* binary and a ternary callable, the resulting arity reduction, and
* constexpr evaluation.
******************************************************************************/

#include "compose_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;


namespace {

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // partial_back(subtract_two, 3)(10) = subtract_two(10, 3) = 7
    D_CONSTEXPR int k_constexpr_partial =
        ::djinterp::partial_back(subtract_two(), 3)(10);
    static_assert(k_constexpr_partial == 7,
                  "partial_back must evaluate in a constexpr context");
#endif

}  // namespace


/*
compose_tests_partial
  Exercises compose.hpp Section IV (partial_back).
  Tests the following:
  - binding the trailing argument of a binary callable: f(x, z)
  - binding the trailing argument of a ternary callable: f(x, y, z)
  - the bound value is appended after the call-site arguments
  - argument order is preserved (a - b - c semantics survive binding)
  - constexpr evaluation (C++14+)
*/
void
compose_tests_partial(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // bind the trailing argument of a binary callable
    run_unit_test(
        _handler,
        tally,
        "partial_back binds the last argument of a binary callable",
        [&]()
        {
            // partial_back(subtract_two, 3)(10) = subtract_two(10, 3) = 7
            auto sub3 = ::djinterp::partial_back(subtract_two(), 3);

            record_assertion(
                _handler,
                (sub3(10) == 7),
                "partial_back(subtract_two, 3)(10) == 7");

            // the bound value is the SECOND operand, not the first:
            // confirm we did not compute 3 - 10.
            record_assertion(
                _handler,
                (sub3(10) != -7),
                "bound value is appended, not prepended");
        });

    // bind the trailing argument of a ternary callable
    run_unit_test(
        _handler,
        tally,
        "partial_back binds the last argument of a ternary callable",
        [&]()
        {
            // partial_back(subtract_three, 1)(10, 2)
            //   = subtract_three(10, 2, 1) = 10 - 2 - 1 = 7
            auto sub = ::djinterp::partial_back(subtract_three(), 1);

            record_assertion(
                _handler,
                (sub(10, 2) == 7),
                "partial_back(subtract_three, 1)(10, 2) == 7");

            // call-site argument order is preserved
            record_assertion(
                _handler,
                (sub(2, 10) == -9),
                "partial_back(subtract_three, 1)(2, 10) == -9");
        });

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    run_unit_test(
        _handler,
        tally,
        "partial_back evaluates in a constexpr context",
        [&]()
        {
            record_assertion(
                _handler,
                (k_constexpr_partial == 7),
                "constexpr partial_back == 7");
        });
#endif

    return;
}


NS_END  // testing
NS_END  // djinterp
