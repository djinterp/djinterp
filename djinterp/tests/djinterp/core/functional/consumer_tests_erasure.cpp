/******************************************************************************
* djinterp [test]                                  consumer_tests_erasure.cpp
*
*   Tests for consumer.hpp Section III type erasure: boxed_consumer<T>
* and box<T>(consumer).  Covers boxing each kind of consumer (primitive,
* adapter, broadcast), storing heterogeneous boxed consumers in a single
* container, reassigning a boxed consumer at runtime, and round-tripping
* values through the erased call.
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
consumer_tests_erasure
  Exercises consumer.hpp Section III (boxed_consumer, box).
  Tests the following:
  - box wraps a primitive consumer (write_to) and forwards correctly
  - box wraps an adapter chain (filtered) and preserves its behavior
  - box wraps a broadcast (tee) and preserves multi-sink delivery
  - a boxed_consumer can be stored and invoked through the erased type
  - heterogeneous boxed consumers share one container type
  - a boxed_consumer can be reassigned to a different target at runtime
  - a default-constructed-then-assigned boxed_consumer works
*/
void
consumer_tests_erasure(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // box a primitive consumer
    run_unit_test(
        _handler,
        tally,
        "box wraps a primitive consumer and forwards inputs",
        [&]()
        {
            std::vector<int> v;
            ::djinterp::boxed_consumer<int> bc =
                ::djinterp::box<int>(::djinterp::consumers::write_to(v));

            bc(11);
            bc(22);

            const std::vector<int> expected = { 11, 22 };

            record_assertion(
                _handler,
                (v == expected),
                "boxed write_to forwarded {11, 22}");
        });

    // box an adapter chain (filtered)
    run_unit_test(
        _handler,
        tally,
        "box wraps a filtered adapter and preserves its gate",
        [&]()
        {
            std::vector<int> v;
            ::djinterp::boxed_consumer<int> bc =
                ::djinterp::box<int>(
                    ::djinterp::consumers::filtered(probe_sink(v),
                                                    is_even()));

            for (int i = 0; i < 6; ++i)
            {
                bc(i);
            }

            const std::vector<int> expected = { 0, 2, 4 };

            record_assertion(
                _handler,
                (v == expected),
                "boxed filtered(even) kept {0, 2, 4}");
        });

    // box a broadcast (tee)
    run_unit_test(
        _handler,
        tally,
        "box wraps a tee broadcast and preserves multi-delivery",
        [&]()
        {
            std::vector<int> a;
            std::vector<int> b;
            ::djinterp::boxed_consumer<int> bc =
                ::djinterp::box<int>(
                    ::djinterp::consumers::tee(probe_sink(a),
                                               probe_sink(b)));

            bc(7);

            record_assertion(
                _handler,
                (a.size() == 1 && b.size() == 1 &&
                 a[0] == 7 && b[0] == 7),
                "boxed tee delivered 7 to both sinks");
        });

    // heterogeneous boxed consumers in one container
    run_unit_test(
        _handler,
        tally,
        "boxed consumers of different targets share one type",
        [&]()
        {
            std::vector<int>   v;
            std::ostringstream os;

            std::vector<::djinterp::boxed_consumer<int>> sinks;
            sinks.push_back(
                ::djinterp::box<int>(::djinterp::consumers::write_to(v)));
            sinks.push_back(
                ::djinterp::box<int>(
                    ::djinterp::consumers::print_to(os, ',')));

            for (std::size_t i = 0; i < sinks.size(); ++i)
            {
                sinks[i](3);
            }

            record_assertion(
                _handler,
                (v.size() == 1 && v[0] == 3 && os.str() == "3,"),
                "heterogeneous boxed consumers each received 3");
        });

    // reassign a boxed consumer at runtime
    run_unit_test(
        _handler,
        tally,
        "a boxed_consumer can be reassigned to a new target",
        [&]()
        {
            std::vector<int> first;
            std::vector<int> second;

            ::djinterp::boxed_consumer<int> bc =
                ::djinterp::box<int>(::djinterp::consumers::write_to(first));

            bc(1);

            // rebind to a different sink
            bc = ::djinterp::box<int>(
                ::djinterp::consumers::write_to(second));

            bc(2);

            const std::vector<int> expected_first  = { 1 };
            const std::vector<int> expected_second = { 2 };

            record_assertion(
                _handler,
                (first == expected_first && second == expected_second),
                "reassigned boxed_consumer routed 1 then 2 to new sink");
        });

    // assign into a default-constructed boxed_consumer
    run_unit_test(
        _handler,
        tally,
        "a default-constructed boxed_consumer accepts assignment",
        [&]()
        {
            std::vector<int> v;

            ::djinterp::boxed_consumer<int> bc;       // empty target
            bc = ::djinterp::box<int>(
                ::djinterp::consumers::write_to(v));

            bc(99);

            record_assertion(
                _handler,
                (v.size() == 1 && v[0] == 99 &&
                 static_cast<bool>(bc)),
                "assigned boxed_consumer is non-empty and forwarded 99");
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
