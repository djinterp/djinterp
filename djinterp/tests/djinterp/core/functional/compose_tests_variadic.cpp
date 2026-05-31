/******************************************************************************
* djinterp [test]                                 compose_tests_variadic.cpp
*
*   Tests for compose.hpp Section III: variadic compose_all (right-to-
* left) and pipe_all (left-to-right).  Covers the single-function base
* case, two- and three-function chains, the documented application
* orders, equivalence with the binary primitives, type-changing chains,
* and constexpr foldability.
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
    // compose_all(f, g, h)(x) = f(g(h(x)))
    // add_one(double(add_one(3))) = add_one(double(4)) = add_one(8) = 9
    D_CONSTEXPR int k_constexpr_compose_all =
        ::djinterp::compose_all(add_one(), doubler(), add_one())(3);
    static_assert(k_constexpr_compose_all == 9,
                  "compose_all must fold in a constexpr context");

    // pipe_all(f, g, h)(x) = h(g(f(x)))
    // add_one(double(add_one(3))) = 9 for this particular operand set
    D_CONSTEXPR int k_constexpr_pipe_all =
        ::djinterp::pipe_all(add_one(), doubler(), add_one())(3);
    static_assert(k_constexpr_pipe_all == 9,
                  "pipe_all must fold in a constexpr context");
#endif

}  // namespace


/*
compose_tests_variadic
  Exercises compose.hpp Section III (compose_all / pipe_all).
  Tests the following:
  - single-function base case folds to the function itself
  - compose_all is right-to-left: compose_all(f,g,h)(x) == f(g(h(x)))
  - pipe_all is left-to-right: pipe_all(f,g,h)(x) == h(g(f(x)))
  - the orders are genuinely opposite for an order-sensitive operand set
  - a two-function chain equals the corresponding binary compose/pipe
  - type-changing variadic chains thread types correctly
  - both fold in a constexpr context (C++14+)
*/
void
compose_tests_variadic(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // single-function base case
    run_unit_test(
        _handler,
        tally,
        "single-function compose_all/pipe_all is identity-of-fold",
        [&]()
        {
            record_assertion(
                _handler,
                (::djinterp::compose_all(add_one())(41) == 42),
                "compose_all(add_one)(41) == 42");

            record_assertion(
                _handler,
                (::djinterp::pipe_all(doubler())(21) == 42),
                "pipe_all(doubler)(21) == 42");
        });

    // compose_all is right-to-left
    run_unit_test(
        _handler,
        tally,
        "compose_all composes right-to-left f(g(h(x)))",
        [&]()
        {
            // add_one(double(add_one(3))) = add_one(double(4))
            //   = add_one(8) = 9
            record_assertion(
                _handler,
                (::djinterp::compose_all(add_one(), doubler(), add_one())(3)
                     == 9),
                "compose_all(add_one, doubler, add_one)(3) == 9");
        });

    // pipe_all is left-to-right
    run_unit_test(
        _handler,
        tally,
        "pipe_all composes left-to-right h(g(f(x)))",
        [&]()
        {
            // add_one(double(add_one(3))) = 9 for this operand set
            record_assertion(
                _handler,
                (::djinterp::pipe_all(add_one(), doubler(), add_one())(3)
                     == 9),
                "pipe_all(add_one, doubler, add_one)(3) == 9");
        });

    // the two orders are genuinely opposite for an order-sensitive set.
    // Use {doubler, add_one}:
    //   compose_all(double, add_one)(x) = double(add_one(x))
    //   pipe_all  (double, add_one)(x) = add_one(double(x))
    // for x = 3: compose_all -> double(4) = 8 ; pipe_all -> add_one(6) = 7
    run_unit_test(
        _handler,
        tally,
        "compose_all and pipe_all use opposite orders",
        [&]()
        {
            record_assertion(
                _handler,
                (::djinterp::compose_all(doubler(), add_one())(3) == 8),
                "compose_all(doubler, add_one)(3) == 8");

            record_assertion(
                _handler,
                (::djinterp::pipe_all(doubler(), add_one())(3) == 7),
                "pipe_all(doubler, add_one)(3) == 7");

            record_assertion(
                _handler,
                (::djinterp::compose_all(doubler(), add_one())(3)
                     != ::djinterp::pipe_all(doubler(), add_one())(3)),
                "the two orders differ on this operand set");
        });

    // two-function variadic equals the binary primitive
    run_unit_test(
        _handler,
        tally,
        "two-function variadic equals the binary primitive",
        [&]()
        {
            record_assertion(
                _handler,
                (::djinterp::compose_all(add_one(), doubler())(5)
                     == ::djinterp::compose(add_one(), doubler())(5)),
                "compose_all == compose for two functions");

            record_assertion(
                _handler,
                (::djinterp::pipe_all(add_one(), doubler())(5)
                     == ::djinterp::pipe(add_one(), doubler())(5)),
                "pipe_all == pipe for two functions");
        });

    // type-changing variadic chain
    run_unit_test(
        _handler,
        tally,
        "variadic chains thread changing types",
        [&]()
        {
            // compose_all(length_of, to_string, add_one)(99)
            //   = length_of(to_string(add_one(99)))
            //   = length_of(to_string(100)) = length_of("100") = 3
            record_assertion(
                _handler,
                (::djinterp::compose_all(length_of(),
                                         to_string_fn(),
                                         add_one())(99)
                     == 3),
                "compose_all length_of . to_string . add_one (99) == 3");

            // pipe_all(add_one, to_string, length_of)(99)
            //   = length_of(to_string(add_one(99))) = 3 as well
            record_assertion(
                _handler,
                (::djinterp::pipe_all(add_one(),
                                      to_string_fn(),
                                      length_of())(99)
                     == 3),
                "pipe_all add_one . to_string . length_of (99) == 3");
        });

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    run_unit_test(
        _handler,
        tally,
        "compose_all/pipe_all fold in a constexpr context",
        [&]()
        {
            record_assertion(
                _handler,
                (k_constexpr_compose_all == 9),
                "constexpr compose_all chain == 9");

            record_assertion(
                _handler,
                (k_constexpr_pipe_all == 9),
                "constexpr pipe_all chain == 9");
        });
#endif

    return;
}


NS_END  // testing
NS_END  // djinterp
