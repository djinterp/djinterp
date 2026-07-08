/******************************************************************************
* djinterp [functional]                          producer_tests_terminals.cpp
*
*   Tests for the free terminal conveniences: collect, for_each, and fold.
* These differ from the inherited producer_base::collect() in that they take
* the producer by lvalue reference and drive it externally.
*
* path:      /src/functional/producer_tests_terminals.cpp
******************************************************************************/

#include "./producer_tests.hpp"

#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_producer_terminals
  Tests the free collect / for_each / fold terminals.
  Tests the following:
  - free collect drains a finite producer (held by lvalue) into a vector
  - free collect of an exhausted/empty producer yields an empty vector
  - for_each forwards every produced value to the consumer in order
  - for_each over an empty producer invokes the consumer zero times
  - fold threads an accumulator through every value (sum), returning the
    final accumulation
  - fold over an empty producer returns the initial accumulator unchanged
  - the free collect and the inherited collect() agree on the same source
*/
void
test_producer_terminals(
    test::test_handler& _h
)
{

    // ---- free collect drains a finite producer ----
    {
        auto p = range(1, 5);                 // 1,2,3,4
        std::vector<int> got = collect(p);
        std::vector<int> want;
        want.push_back(1); want.push_back(2);
        want.push_back(3); want.push_back(4);

        D_TEST_CHECK(_h, got == want);
    }

    // ---- free collect of an empty producer ----
    {
        auto p = empty<int>();
        std::vector<int> got = collect(p);

        D_TEST_CHECK(_h, got.empty());
    }

    // ---- for_each forwards every value in order ----
    {
        auto p = range(1, 4);                 // 1,2,3
        std::vector<int> sink;
        for_each(p, push_consumer(&sink));

        std::vector<int> want;
        want.push_back(1); want.push_back(2); want.push_back(3);

        D_TEST_CHECK(_h, sink == want);
    }

    // ---- for_each over an empty producer calls the consumer zero times ---
    {
        auto p = empty<int>();
        std::vector<int> sink;
        for_each(p, push_consumer(&sink));

        D_TEST_CHECK(_h, sink.empty());
    }

    // ---- fold sums the produced values ----
    {
        auto p = range(1, 5);                 // 1+2+3+4 = 10
        int total = fold(p, 0, sum_step());

        D_TEST_CHECK(_h, total == 10);
    }

    // ---- fold over an empty producer returns the initial value ----
    {
        auto p = empty<int>();
        int total = fold(p, 42, sum_step());

        D_TEST_CHECK(_h, total == 42);
    }

    // ---- free collect and inherited collect() agree ----
    {
        auto p = range(0, 6);
        std::vector<int> via_free = collect(p);
        std::vector<int> via_crtp = range(0, 6).collect();

        D_TEST_CHECK(_h, via_free == via_crtp);
    }

    // ---- fold can build a product as well as a sum ----
    {
        struct mul_step
        {
            int operator()(int _acc, int _v) const { return _acc * _v; }
        };

        auto p = range(1, 5);                 // 1*2*3*4 = 24
        int product = fold(p, 1, mul_step());

        D_TEST_CHECK(_h, product == 24);
    }

    return;
}


NS_END  // testing
NS_END  // djinterp
