/******************************************************************************
* djinterp [test]                                      consumer_tests_tee.cpp
*
*   Tests for consumer.hpp Section II tee: variadic broadcast.  Covers
* the single-consumer degenerate case, two- and three-way broadcast,
* invocation order across the inner consumers, and broadcast to
* heterogeneous sink types.
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


namespace {

    // order_sink
    //   class: records a fixed tag into a shared order log on each call,
    // so a tee's left-to-right invocation order can be observed.
    class order_sink
    {
    public:
        order_sink(
            std::vector<int>& _order,
            int               _tag
        )
            : m_order(&_order),
              m_tag(_tag)
        {}

        void operator()(
            const int&
        ) const
        {
            m_order->push_back(m_tag);

            return;
        }

    private:
        std::vector<int>* m_order;
        int               m_tag;
    };

}  // namespace


/*
consumer_tests_tee
  Exercises consumer.hpp Section II tee (broadcast).
  Tests the following:
  - tee with a single consumer degenerates to that consumer
  - tee with two consumers broadcasts every value to both
  - tee with three consumers broadcasts to all three
  - tee invokes its inner consumers in left-to-right order
  - tee broadcasts to heterogeneous sink types (vector + stream)
*/
void
consumer_tests_tee(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // single-consumer degenerate case
    run_unit_test(
        _handler,
        tally,
        "tee with one consumer forwards to just that consumer",
        [&]()
        {
            std::vector<int> log;
            auto t = ::djinterp::consumers::tee(probe_sink(log));

            t(1);
            t(2);

            const std::vector<int> expected = { 1, 2 };

            record_assertion(
                _handler,
                (log == expected),
                "tee(one) forwarded {1, 2}");
        });

    // two-way broadcast
    run_unit_test(
        _handler,
        tally,
        "tee broadcasts every value to two consumers",
        [&]()
        {
            std::vector<int> a;
            std::vector<int> b;
            auto t = ::djinterp::consumers::tee(probe_sink(a),
                                                probe_sink(b));

            t(1);
            t(2);
            t(3);

            const std::vector<int> expected = { 1, 2, 3 };

            record_assertion(
                _handler,
                (a == expected && b == expected),
                "tee(a, b) delivered {1, 2, 3} to both");
        });

    // three-way broadcast
    run_unit_test(
        _handler,
        tally,
        "tee broadcasts to three consumers",
        [&]()
        {
            std::vector<int> a;
            std::vector<int> b;
            std::vector<int> c;
            auto t = ::djinterp::consumers::tee(probe_sink(a),
                                                probe_sink(b),
                                                probe_sink(c));

            t(9);

            record_assertion(
                _handler,
                (a.size() == 1 && b.size() == 1 && c.size() == 1 &&
                 a[0] == 9 && b[0] == 9 && c[0] == 9),
                "tee(a, b, c) delivered 9 to all three");
        });

    // invocation order is left-to-right
    run_unit_test(
        _handler,
        tally,
        "tee invokes its consumers in left-to-right order",
        [&]()
        {
            std::vector<int> order;
            auto t = ::djinterp::consumers::tee(order_sink(order, 1),
                                                order_sink(order, 2),
                                                order_sink(order, 3));

            t(0);

            const std::vector<int> expected = { 1, 2, 3 };

            record_assertion(
                _handler,
                (order == expected),
                "tee fired tags in order {1, 2, 3}");
        });

    // heterogeneous sinks: vector + stream
    run_unit_test(
        _handler,
        tally,
        "tee broadcasts to heterogeneous sink types",
        [&]()
        {
            std::vector<int>   v;
            std::ostringstream os;
            auto t = ::djinterp::consumers::tee(
                probe_sink(v),
                ::djinterp::consumers::print_to(os, ';'));

            t(5);
            t(6);

            const std::vector<int> expected = { 5, 6 };

            record_assertion(
                _handler,
                (v == expected && os.str() == "5;6;"),
                "tee fed a vector and a stream consistently");
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
