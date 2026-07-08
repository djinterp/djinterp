/******************************************************************************
* djinterp [test]                                 monad_tests_composition.cpp
*
*   Tests for monad.hpp Section II sequencing/composition operations:
* monad_then (sequence, discard the first value), kleisli_compose
* (>=> -- compose two monadic arrows), and lift_m2 (binary applicative
* lift).  Covers success and short-circuit paths, value-discarding
* semantics, Kleisli associativity, type-changing lifts, and the
* left-bias of lift_m2.
******************************************************************************/
#include "./monad_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;


/*
monad_tests_composition
  Exercises monad.hpp Section II sequencing/composition.
  Tests the following:
  - monad_then sequences and yields the second monad on a some first
  - monad_then short-circuits to none when the first monad is none
  - monad_then discards the first value but keeps the second's value
  - monad_then can change the monad's inner type across the sequence
  - kleisli_compose threads a value through two arrows (f >=> g)
  - kleisli_compose short-circuits when the first arrow fails
  - kleisli_compose short-circuits when the second arrow fails
  - kleisli_compose can change types across the two arrows
  - kleisli_compose is associative ((f>=>g)>=>h == f>=>(g>=>h))
  - lift_m2 combines two some values with a binary function
  - lift_m2 short-circuits if either argument monad is none
  - lift_m2 can produce a different inner type (int,int -> string)
*/
void
monad_tests_composition(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // monad_then: some first -> second
    run_unit_test(
        _handler,
        tally,
        "monad_then yields the second monad after a some first",
        [&]()
        {
            auto m = ::djinterp::monad_then(some(1), some(99));

            record_assertion(
                _handler,
                (m.is_some() && m.value() == 99),
                "then(some(1), some(99)) is some(99)");
        });

    // monad_then: none first short-circuits
    run_unit_test(
        _handler,
        tally,
        "monad_then short-circuits when the first monad is none",
        [&]()
        {
            auto m = ::djinterp::monad_then(test_maybe<int>::none(),
                                            some(99));

            record_assertion(
                _handler,
                (!m.is_some()),
                "then(none, some(99)) is none");
        });

    // monad_then: discards first value, changes type
    run_unit_test(
        _handler,
        tally,
        "monad_then discards the first value and may change type",
        [&]()
        {
            auto m = ::djinterp::monad_then(
                some(7), some(std::string("kept")));

            const bool type_ok =
                std::is_same<decltype(m), test_maybe<std::string> >::value;

            record_assertion(
                _handler,
                (type_ok && m.is_some() && m.value() == "kept"),
                "then(some(7), some(\"kept\")) is some(\"kept\")");
        });

    // kleisli_compose: success
    run_unit_test(
        _handler,
        tally,
        "kleisli_compose threads a value through two arrows",
        [&]()
        {
            auto k = ::djinterp::kleisli_compose(arrow_inc(),
                                                 arrow_times_ten());
            auto m = k(4);   // (4 + 1) * 10 = 50

            record_assertion(
                _handler,
                (m.is_some() && m.value() == 50),
                "(inc >=> times_ten)(4) is some(50)");
        });

    // kleisli_compose: first arrow fails
    run_unit_test(
        _handler,
        tally,
        "kleisli_compose short-circuits when the first arrow fails",
        [&]()
        {
            auto k = ::djinterp::kleisli_compose(arrow_to_none(),
                                                 arrow_times_ten());
            auto m = k(4);

            record_assertion(
                _handler,
                (!m.is_some()),
                "(to_none >=> times_ten)(4) is none");
        });

    // kleisli_compose: second arrow fails
    run_unit_test(
        _handler,
        tally,
        "kleisli_compose short-circuits when the second arrow fails",
        [&]()
        {
            auto k = ::djinterp::kleisli_compose(arrow_inc(),
                                                 arrow_to_none());
            auto m = k(4);

            record_assertion(
                _handler,
                (!m.is_some()),
                "(inc >=> to_none)(4) is none");
        });

    // kleisli_compose: type-changing
    run_unit_test(
        _handler,
        tally,
        "kleisli_compose can change types across arrows",
        [&]()
        {
            auto k = ::djinterp::kleisli_compose(arrow_inc(),
                                                 arrow_to_string());
            auto m = k(4);   // inc -> 5 -> to_string -> "5"

            const bool type_ok =
                std::is_same<decltype(m), test_maybe<std::string> >::value;

            record_assertion(
                _handler,
                (type_ok && m.is_some() && m.value() == "5"),
                "(inc >=> to_string)(4) is some(\"5\")");
        });

    // kleisli associativity
    run_unit_test(
        _handler,
        tally,
        "kleisli_compose is associative",
        [&]()
        {
            // (inc >=> times_ten) >=> inc   vs   inc >=> (times_ten >=> inc)
            auto left = ::djinterp::kleisli_compose(
                ::djinterp::kleisli_compose(arrow_inc(), arrow_times_ten()),
                arrow_inc());
            auto right = ::djinterp::kleisli_compose(
                arrow_inc(),
                ::djinterp::kleisli_compose(arrow_times_ten(), arrow_inc()));

            auto lm = left(4);    // ((4+1)*10)+1 = 51
            auto rm = right(4);

            record_assertion(
                _handler,
                (lm == rm && lm.value() == 51),
                "(f>=>g)>=>h == f>=>(g>=>h)");
        });

    // lift_m2: both some
    run_unit_test(
        _handler,
        tally,
        "lift_m2 combines two some values",
        [&]()
        {
            auto m = ::djinterp::lift_m2(some(3), some(4), binary_add());

            record_assertion(
                _handler,
                (m.is_some() && m.value() == 7),
                "lift_m2(some(3), some(4), add) is some(7)");
        });

    // lift_m2: first none
    run_unit_test(
        _handler,
        tally,
        "lift_m2 short-circuits when the first monad is none",
        [&]()
        {
            auto m = ::djinterp::lift_m2(test_maybe<int>::none(),
                                         some(4), binary_add());

            record_assertion(
                _handler,
                (!m.is_some()),
                "lift_m2(none, some(4), add) is none");
        });

    // lift_m2: second none
    run_unit_test(
        _handler,
        tally,
        "lift_m2 short-circuits when the second monad is none",
        [&]()
        {
            auto m = ::djinterp::lift_m2(some(3),
                                         test_maybe<int>::none(),
                                         binary_add());

            record_assertion(
                _handler,
                (!m.is_some()),
                "lift_m2(some(3), none, add) is none");
        });

    // lift_m2: type-changing
    run_unit_test(
        _handler,
        tally,
        "lift_m2 can produce a different inner type",
        [&]()
        {
            auto m = ::djinterp::lift_m2(some(2), some(5),
                                         binary_concat_sum());

            const bool type_ok =
                std::is_same<decltype(m), test_maybe<std::string> >::value;

            record_assertion(
                _handler,
                (type_ok && m.is_some() && m.value() == "7"),
                "lift_m2(some(2), some(5), concat_sum) is some(\"7\")");
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
