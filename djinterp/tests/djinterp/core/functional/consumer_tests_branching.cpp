/******************************************************************************
* djinterp [test]                               consumer_tests_branching.cpp
*
*   Tests for consumer.hpp Section II branching combinators: conditional
* (route on a predicate) and fallback (try primary, on exception run
* secondary).  Covers true/false routing, the all-true and all-false
* boundaries, single-evaluation of the predicate, the no-throw path of
* fallback, the throwing path, and propagation of secondary exceptions.
******************************************************************************/

#include "consumer_tests.hpp"

#include <vector>


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;


namespace {

    // counting_predicate
    //   class: an is-even predicate that also tallies how many times it
    // was evaluated, so conditional's "evaluate once per input" contract
    // can be checked.
    class counting_predicate
    {
    public:
        explicit counting_predicate(
            std::size_t& _calls
        )
            : m_calls(&_calls)
        {}

        bool operator()(
            const int& _value
        ) const
        {
            ++(*m_calls);

            return (_value % 2) == 0;
        }

    private:
        std::size_t* m_calls;
    };

}  // namespace


/*
consumer_tests_branching
  Exercises consumer.hpp Section II branching (conditional, fallback).
  Tests the following:
  - conditional routes true-predicate inputs to the if_true consumer
  - conditional routes false-predicate inputs to the if_false consumer
  - conditional with always-true sends everything down the true branch
  - conditional with always-false sends everything down the false branch
  - conditional evaluates the predicate exactly once per input
  - fallback runs only the primary when it does not throw
  - fallback runs the secondary when the primary throws
  - fallback lets an exception from the secondary propagate
*/
void
consumer_tests_branching(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // conditional: split evens / odds
    run_unit_test(
        _handler,
        tally,
        "conditional routes inputs by the predicate",
        [&]()
        {
            std::vector<int> evens;
            std::vector<int> odds;
            auto c = ::djinterp::consumers::conditional(
                is_even(),
                probe_sink(evens),
                probe_sink(odds));

            for (int i = 0; i < 5; ++i)
            {
                c(i);
            }

            const std::vector<int> expected_even = { 0, 2, 4 };
            const std::vector<int> expected_odd  = { 1, 3 };

            record_assertion(
                _handler,
                (evens == expected_even && odds == expected_odd),
                "conditional split 0..4 into evens {0,2,4} / odds {1,3}");
        });

    // conditional: always-true uses only the true branch
    run_unit_test(
        _handler,
        tally,
        "conditional with always-true uses only the true branch",
        [&]()
        {
            std::vector<int> t;
            std::vector<int> f;
            auto c = ::djinterp::consumers::conditional(
                always_true(),
                probe_sink(t),
                probe_sink(f));

            c(1);
            c(2);

            const std::vector<int> expected = { 1, 2 };

            record_assertion(
                _handler,
                (t == expected && f.empty()),
                "conditional(always_true) used only the true branch");
        });

    // conditional: always-false uses only the false branch
    run_unit_test(
        _handler,
        tally,
        "conditional with always-false uses only the false branch",
        [&]()
        {
            std::vector<int> t;
            std::vector<int> f;
            auto c = ::djinterp::consumers::conditional(
                always_false(),
                probe_sink(t),
                probe_sink(f));

            c(1);
            c(2);

            const std::vector<int> expected = { 1, 2 };

            record_assertion(
                _handler,
                (f == expected && t.empty()),
                "conditional(always_false) used only the false branch");
        });

    // conditional: predicate evaluated exactly once per input
    run_unit_test(
        _handler,
        tally,
        "conditional evaluates the predicate once per input",
        [&]()
        {
            std::size_t      calls = 0;
            std::vector<int> evens;
            std::vector<int> odds;
            auto c = ::djinterp::consumers::conditional(
                counting_predicate(calls),
                probe_sink(evens),
                probe_sink(odds));

            c(1);
            c(2);
            c(3);

            record_assertion(
                _handler,
                (calls == 3),
                "conditional evaluated the predicate 3 times for 3 inputs");
        });

    // fallback: primary succeeds, secondary untouched
    run_unit_test(
        _handler,
        tally,
        "fallback runs only the primary when it does not throw",
        [&]()
        {
            std::vector<int> primary;
            std::vector<int> secondary;
            auto fb = ::djinterp::consumers::fallback(
                probe_sink(primary),
                probe_sink(secondary));

            fb(1);
            fb(2);

            const std::vector<int> expected = { 1, 2 };

            record_assertion(
                _handler,
                (primary == expected && secondary.empty()),
                "fallback used the primary; secondary stayed empty");
        });

    // fallback: primary throws, secondary handles
    run_unit_test(
        _handler,
        tally,
        "fallback runs the secondary when the primary throws",
        [&]()
        {
            std::vector<int> secondary;
            auto fb = ::djinterp::consumers::fallback(
                throwing_sink(),
                probe_sink(secondary));

            fb(5);
            fb(6);

            const std::vector<int> expected = { 5, 6 };

            record_assertion(
                _handler,
                (secondary == expected),
                "fallback routed {5, 6} to the secondary on throw");
        });

    // fallback: secondary's own exception propagates
    run_unit_test(
        _handler,
        tally,
        "fallback propagates an exception thrown by the secondary",
        [&]()
        {
            auto fb = ::djinterp::consumers::fallback(
                throwing_sink(),
                throwing_sink());

            bool propagated = false;

            try
            {
                fb(1);
            }
            catch (const test_consumer_error&)
            {
                propagated = true;
            }

            record_assertion(
                _handler,
                propagated,
                "fallback let the secondary's exception propagate");
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
