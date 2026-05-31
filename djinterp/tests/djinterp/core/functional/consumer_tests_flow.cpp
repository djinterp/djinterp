/******************************************************************************
* djinterp [test]                                     consumer_tests_flow.cpp
*
*   Tests for consumer.hpp Section II stateful flow combinators:
* batched (fire every N-th input), take (first N then ignore), and drop
* (skip first N then forward).  These carry mutable per-instance state,
* so the edge cases around the stride/limit boundaries and the zero
* arguments get particular attention.
******************************************************************************/

#include "consumer_tests.hpp"

#include <vector>


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;


/*
consumer_tests_flow
  Exercises consumer.hpp Section II flow combinators (batched/take/drop).
  Tests the following:
  - batched fires on every stride-th input, dropping intermediates
  - batched with stride 1 forwards every input
  - batched with stride 0 is clamped to stride 1 (no divide-by-zero)
  - batched fires the value that lands on the boundary (not a stale one)
  - take forwards at most the first n inputs, then drops the rest
  - take with n == 0 forwards nothing
  - take with n larger than the stream forwards everything available
  - drop skips the first n inputs, then forwards the rest
  - drop with n == 0 forwards everything
  - drop with n larger than the stream forwards nothing
  - take and drop are complementary partitions of a stream
*/
void
consumer_tests_flow(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // batched: stride 3 over 1..7 fires on the 3rd and 6th inputs
    run_unit_test(
        _handler,
        tally,
        "batched fires on every stride-th input",
        [&]()
        {
            std::vector<int> log;
            auto b = ::djinterp::consumers::batched(probe_sink(log), 3);

            for (int i = 1; i <= 7; ++i)
            {
                b(i);
            }

            // fires when the running count reaches the stride: at the
            // 3rd input (value 3) and the 6th input (value 6).
            const std::vector<int> expected = { 3, 6 };

            record_assertion(
                _handler,
                (log == expected),
                "batched(3) over 1..7 fired {3, 6}");
        });

    // batched: stride 1 forwards everything
    run_unit_test(
        _handler,
        tally,
        "batched with stride 1 forwards every input",
        [&]()
        {
            std::vector<int> log;
            auto b = ::djinterp::consumers::batched(probe_sink(log), 1);

            b(1);
            b(2);
            b(3);

            const std::vector<int> expected = { 1, 2, 3 };

            record_assertion(
                _handler,
                (log == expected),
                "batched(1) forwarded every input");
        });

    // batched: stride 0 is clamped to 1
    run_unit_test(
        _handler,
        tally,
        "batched with stride 0 is treated as stride 1",
        [&]()
        {
            std::vector<int> log;
            auto b = ::djinterp::consumers::batched(probe_sink(log), 0);

            b(1);
            b(2);

            const std::vector<int> expected = { 1, 2 };

            record_assertion(
                _handler,
                (log == expected),
                "batched(0) behaved like batched(1)");
        });

    // batched: fewer inputs than the stride fire nothing
    run_unit_test(
        _handler,
        tally,
        "batched fires nothing before the first boundary",
        [&]()
        {
            std::vector<int> log;
            auto b = ::djinterp::consumers::batched(probe_sink(log), 5);

            b(1);
            b(2);
            b(3);

            record_assertion(
                _handler,
                (log.empty()),
                "batched(5) with 3 inputs fired nothing");
        });

    // take: first 2 of 1..5
    run_unit_test(
        _handler,
        tally,
        "take forwards at most the first n inputs",
        [&]()
        {
            std::vector<int> log;
            auto t = ::djinterp::consumers::take(probe_sink(log), 2);

            for (int i = 1; i <= 5; ++i)
            {
                t(i);
            }

            const std::vector<int> expected = { 1, 2 };

            record_assertion(
                _handler,
                (log == expected),
                "take(2) kept {1, 2} from 1..5");
        });

    // take 0 forwards nothing
    run_unit_test(
        _handler,
        tally,
        "take with n == 0 forwards nothing",
        [&]()
        {
            std::vector<int> log;
            auto t = ::djinterp::consumers::take(probe_sink(log), 0);

            t(1);
            t(2);

            record_assertion(
                _handler,
                (log.empty()),
                "take(0) forwarded nothing");
        });

    // take n larger than stream forwards all available
    run_unit_test(
        _handler,
        tally,
        "take with n beyond the stream forwards all inputs",
        [&]()
        {
            std::vector<int> log;
            auto t = ::djinterp::consumers::take(probe_sink(log), 100);

            t(1);
            t(2);
            t(3);

            const std::vector<int> expected = { 1, 2, 3 };

            record_assertion(
                _handler,
                (log == expected),
                "take(100) over 3 inputs forwarded all three");
        });

    // drop: skip first 2 of 1..5
    run_unit_test(
        _handler,
        tally,
        "drop skips the first n inputs then forwards the rest",
        [&]()
        {
            std::vector<int> log;
            auto d = ::djinterp::consumers::drop(probe_sink(log), 2);

            for (int i = 1; i <= 5; ++i)
            {
                d(i);
            }

            const std::vector<int> expected = { 3, 4, 5 };

            record_assertion(
                _handler,
                (log == expected),
                "drop(2) kept {3, 4, 5} from 1..5");
        });

    // drop 0 forwards everything
    run_unit_test(
        _handler,
        tally,
        "drop with n == 0 forwards everything",
        [&]()
        {
            std::vector<int> log;
            auto d = ::djinterp::consumers::drop(probe_sink(log), 0);

            d(1);
            d(2);

            const std::vector<int> expected = { 1, 2 };

            record_assertion(
                _handler,
                (log == expected),
                "drop(0) forwarded every input");
        });

    // drop n larger than stream forwards nothing
    run_unit_test(
        _handler,
        tally,
        "drop with n beyond the stream forwards nothing",
        [&]()
        {
            std::vector<int> log;
            auto d = ::djinterp::consumers::drop(probe_sink(log), 100);

            d(1);
            d(2);
            d(3);

            record_assertion(
                _handler,
                (log.empty()),
                "drop(100) over 3 inputs forwarded nothing");
        });

    // take and drop partition a stream
    run_unit_test(
        _handler,
        tally,
        "take(n) and drop(n) partition the same stream",
        [&]()
        {
            std::vector<int> head;
            std::vector<int> tail;
            auto t = ::djinterp::consumers::take(probe_sink(head), 2);
            auto d = ::djinterp::consumers::drop(probe_sink(tail), 2);

            for (int i = 1; i <= 5; ++i)
            {
                t(i);
                d(i);
            }

            const std::vector<int> expected_head = { 1, 2 };
            const std::vector<int> expected_tail = { 3, 4, 5 };

            record_assertion(
                _handler,
                (head == expected_head && tail == expected_tail),
                "take(2) | drop(2) partitioned 1..5 into {1,2} | {3,4,5}");
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
