/******************************************************************************
* djinterp [test]                                    monad_tests_protocol.cpp
*
*   Tests for monad.hpp Section I: the monad protocol -- is_monad
* detection and the monad_traits surface that every concrete monad
* must provide.  Covers positive detection of the test monad, negative
* detection of non-monads, decay-insensitivity of is_monad, and direct
* exercise of the traits primitives (unit, bind, value_type, rebind).
******************************************************************************/
#include "./monad_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;


/*
monad_tests_protocol
  Exercises monad.hpp Section I (is_monad, monad_traits).
  Tests the following:
  - is_monad is true for a type with a monad_traits specialization
  - is_monad is false for a plain type and for a builtin
  - is_monad sees through references and cv-qualification (decay)
  - monad_traits::unit lifts a value into a some-state monad
  - monad_traits::bind threads a some value through a Kleisli arrow
  - monad_traits::bind short-circuits on a none input
  - monad_traits::value_type / rebind expose the expected types
*/
void
monad_tests_protocol(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // is_monad: positive
    run_unit_test(
        _handler,
        tally,
        "is_monad detects a specialized monad",
        [&]()
        {
            record_assertion(
                _handler,
                (::djinterp::is_monad<test_maybe<int> >::value),
                "is_monad<test_maybe<int>> is true");
        });

    // is_monad: negative
    run_unit_test(
        _handler,
        tally,
        "is_monad rejects non-monad types",
        [&]()
        {
            record_assertion(
                _handler,
                (!::djinterp::is_monad<not_a_monad>::value),
                "is_monad<not_a_monad> is false");

            record_assertion(
                _handler,
                (!::djinterp::is_monad<int>::value),
                "is_monad<int> is false");
        });

    // is_monad: decay-insensitive (reference / const)
    run_unit_test(
        _handler,
        tally,
        "is_monad sees through reference and const qualification",
        [&]()
        {
            const bool ref_ok =
                ::djinterp::is_monad<test_maybe<int>&>::value;
            const bool cref_ok =
                ::djinterp::is_monad<const test_maybe<int>&>::value;

            record_assertion(
                _handler,
                (ref_ok && cref_ok),
                "is_monad holds for lvalue-ref and const-ref forms");
        });

    // monad_traits::unit
    run_unit_test(
        _handler,
        tally,
        "monad_traits::unit lifts a value into a some-state monad",
        [&]()
        {
            test_maybe<int> m =
                ::djinterp::monad_traits<test_maybe<int> >::unit(7);

            record_assertion(
                _handler,
                (m.is_some() && m.value() == 7),
                "unit(7) is some(7)");
        });

    // monad_traits::bind on some
    run_unit_test(
        _handler,
        tally,
        "monad_traits::bind threads a some value through an arrow",
        [&]()
        {
            test_maybe<int> m =
                ::djinterp::monad_traits<test_maybe<int> >::bind(
                    some(5), arrow_inc());

            record_assertion(
                _handler,
                (m.is_some() && m.value() == 6),
                "bind(some(5), inc) is some(6)");
        });

    // monad_traits::bind on none short-circuits
    run_unit_test(
        _handler,
        tally,
        "monad_traits::bind short-circuits on a none input",
        [&]()
        {
            test_maybe<int> m =
                ::djinterp::monad_traits<test_maybe<int> >::bind(
                    test_maybe<int>::none(), arrow_inc());

            record_assertion(
                _handler,
                (!m.is_some()),
                "bind(none, inc) is none");
        });

    // monad_traits associated types
    run_unit_test(
        _handler,
        tally,
        "monad_traits exposes value_type and rebind",
        [&]()
        {
            const bool value_ok = std::is_same<
                ::djinterp::monad_traits<test_maybe<int> >::value_type,
                int>::value;
            const bool rebind_ok = std::is_same<
                ::djinterp::monad_traits<test_maybe<int> >::
                    template rebind<std::string>,
                test_maybe<std::string> >::value;

            record_assertion(
                _handler,
                (value_ok && rebind_ok),
                "value_type is int and rebind<string> is test_maybe<string>");
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
