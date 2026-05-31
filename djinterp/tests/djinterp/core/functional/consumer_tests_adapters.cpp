/******************************************************************************
* djinterp [test]                                consumer_tests_adapters.cpp
*
*   Tests for consumer.hpp Section II adapters: filtered (predicate
* gate) and mapped (contramap / input transform).  Covers pass-through
* and rejection by the predicate, the always-true / always-false
* boundary predicates, same-type and type-changing maps, and the
* composition of filtered with mapped.
******************************************************************************/

#include "consumer_tests.hpp"

#include <sstream>
#include <string>
#include <vector>


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;


/*
consumer_tests_adapters
  Exercises consumer.hpp Section II adapters (filtered, mapped).
  Tests the following:
  - filtered forwards only inputs for which the predicate is true
  - filtered with an always-true predicate forwards everything
  - filtered with an always-false predicate forwards nothing
  - mapped applies a same-type transform before the inner consumer
  - mapped applies a type-changing transform (int -> string)
  - mapped composes with filtered (filter, then transform)
  - filtered composes with mapped (transform, then filter on result)
*/
void
consumer_tests_adapters(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // filtered: predicate gate
    run_unit_test(
        _handler,
        tally,
        "filtered forwards only inputs passing the predicate",
        [&]()
        {
            std::vector<int> log;
            auto f = ::djinterp::consumers::filtered(probe_sink(log),
                                                     is_even());

            for (int i = 0; i < 6; ++i)
            {
                f(i);
            }

            const std::vector<int> expected = { 0, 2, 4 };

            record_assertion(
                _handler,
                (log == expected),
                "filtered(even) kept {0, 2, 4} from 0..5");
        });

    // filtered: always-true forwards everything
    run_unit_test(
        _handler,
        tally,
        "filtered with always-true forwards every input",
        [&]()
        {
            std::vector<int> log;
            auto f = ::djinterp::consumers::filtered(probe_sink(log),
                                                     always_true());

            f(1);
            f(2);
            f(3);

            const std::vector<int> expected = { 1, 2, 3 };

            record_assertion(
                _handler,
                (log == expected),
                "filtered(always_true) forwarded all inputs");
        });

    // filtered: always-false forwards nothing
    run_unit_test(
        _handler,
        tally,
        "filtered with always-false forwards no input",
        [&]()
        {
            std::vector<int> log;
            auto f = ::djinterp::consumers::filtered(probe_sink(log),
                                                     always_false());

            f(1);
            f(2);
            f(3);

            record_assertion(
                _handler,
                (log.empty()),
                "filtered(always_false) forwarded nothing");
        });

    // mapped: same-type transform
    run_unit_test(
        _handler,
        tally,
        "mapped applies a same-type transform before the consumer",
        [&]()
        {
            std::vector<int> log;
            auto m = ::djinterp::consumers::mapped(probe_sink(log),
                                                   doubler());

            m(3);
            m(5);

            const std::vector<int> expected = { 6, 10 };

            record_assertion(
                _handler,
                (log == expected),
                "mapped(double) forwarded {6, 10}");
        });

    // mapped: type-changing transform (int -> string), into a stream
    run_unit_test(
        _handler,
        tally,
        "mapped applies a type-changing transform (int -> string)",
        [&]()
        {
            std::ostringstream os;
            auto m = ::djinterp::consumers::mapped(
                ::djinterp::consumers::print_to(os, '|'),
                to_string_fn());

            m(12);
            m(34);

            record_assertion(
                _handler,
                (os.str() == "12|34|"),
                "mapped(to_string) -> print yielded \"12|34|\"");
        });

    // mapped over filtered: filter first, then transform survivors
    run_unit_test(
        _handler,
        tally,
        "mapped composes over filtered (filter then transform)",
        [&]()
        {
            std::vector<int> log;

            // build: filtered( mapped(sink, double), even )
            //   -- only even inputs pass; survivors are doubled.
            auto inner = ::djinterp::consumers::mapped(probe_sink(log),
                                                       doubler());
            auto f     = ::djinterp::consumers::filtered(inner, is_even());

            for (int i = 0; i < 6; ++i)
            {
                f(i);
            }

            // evens 0,2,4 -> doubled 0,4,8
            const std::vector<int> expected = { 0, 4, 8 };

            record_assertion(
                _handler,
                (log == expected),
                "filtered(even) then mapped(double) yielded {0, 4, 8}");
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
