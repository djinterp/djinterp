/******************************************************************************
* djinterp [test]                              consumer_tests_primitives.cpp
*
*   Tests for consumer.hpp Section II primitives: print_to (with custom
* and default separators), write_to (container append), discard (null
* sink), and count_into (external counter).  Covers correct output,
* separator handling, empty-stream edge cases, multiple values, and
* the no-op contract of discard.
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
consumer_tests_primitives
  Exercises consumer.hpp Section II primitives.
  Tests the following:
  - print_to(stream, sep): writes value then the custom separator
  - print_to(stream): defaults the separator to '\n'
  - print_to over multiple values concatenates in order
  - print_to with a string separator
  - write_to: appends each input to the container via push_back
  - write_to preserves order and handles a single element
  - discard: accepts input and produces no observable effect
  - count_into: increments the external counter once per input
  - count_into starts from the counter's existing value (no reset)
*/
void
consumer_tests_primitives(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // print_to with explicit separator
    run_unit_test(
        _handler,
        tally,
        "print_to writes each value followed by the separator",
        [&]()
        {
            std::ostringstream os;
            auto p = ::djinterp::consumers::print_to(os, ',');

            p(1);
            p(2);
            p(3);

            record_assertion(
                _handler,
                (os.str() == "1,2,3,"),
                "print_to(os, ',') yields \"1,2,3,\"");
        });

    // print_to with default separator
    run_unit_test(
        _handler,
        tally,
        "print_to defaults its separator to newline",
        [&]()
        {
            std::ostringstream os;
            auto p = ::djinterp::consumers::print_to(os);

            p(7);

            record_assertion(
                _handler,
                (os.str() == "7\n"),
                "print_to(os) yields \"7\\n\"");
        });

    // print_to with a string separator
    run_unit_test(
        _handler,
        tally,
        "print_to accepts a string separator",
        [&]()
        {
            std::ostringstream os;
            auto p = ::djinterp::consumers::print_to(os, std::string(", "));

            p(10);
            p(20);

            record_assertion(
                _handler,
                (os.str() == "10, 20, "),
                "print_to(os, \", \") yields \"10, 20, \"");
        });

    // print_to with no inputs leaves the stream empty
    run_unit_test(
        _handler,
        tally,
        "print_to writes nothing when never called",
        [&]()
        {
            std::ostringstream os;
            auto p = ::djinterp::consumers::print_to(os, ',');
            (void)p;

            record_assertion(
                _handler,
                (os.str().empty()),
                "uninvoked print_to leaves the stream empty");
        });

    // write_to appends in order
    run_unit_test(
        _handler,
        tally,
        "write_to appends each input to the container",
        [&]()
        {
            std::vector<int> v;
            auto w = ::djinterp::consumers::write_to(v);

            w(1);
            w(2);
            w(3);

            const std::vector<int> expected = { 1, 2, 3 };

            record_assertion(
                _handler,
                (v == expected),
                "write_to appended {1, 2, 3} in order");
        });

    // write_to with a single element
    run_unit_test(
        _handler,
        tally,
        "write_to handles a single element",
        [&]()
        {
            std::vector<int> v;
            auto w = ::djinterp::consumers::write_to(v);

            w(42);

            record_assertion(
                _handler,
                (v.size() == 1 && v.front() == 42),
                "write_to appended the single element 42");
        });

    // discard is a no-op
    run_unit_test(
        _handler,
        tally,
        "discard accepts input and does nothing observable",
        [&]()
        {
            auto d = ::djinterp::consumers::discard();

            // calling it must be valid and have no side effect we can
            // observe; success is simply that the calls compile and run.
            d(1);
            d(2);
            d(3);

            record_assertion(
                _handler,
                true,
                "discard invoked three times without effect");
        });

    // count_into increments per input
    run_unit_test(
        _handler,
        tally,
        "count_into increments the counter once per input",
        [&]()
        {
            std::size_t n = 0;
            auto c = ::djinterp::consumers::count_into(n);

            c(0);
            c(0);
            c(0);
            c(0);

            record_assertion(
                _handler,
                (n == 4),
                "count_into reached 4 after four inputs");
        });

    // count_into does not reset an existing counter
    run_unit_test(
        _handler,
        tally,
        "count_into accumulates onto the counter's existing value",
        [&]()
        {
            std::size_t n = 10;
            auto c = ::djinterp::consumers::count_into(n);

            c(1);
            c(2);

            record_assertion(
                _handler,
                (n == 12),
                "count_into advanced 10 -> 12 (no reset)");
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
