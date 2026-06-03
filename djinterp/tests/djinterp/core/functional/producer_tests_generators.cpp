/******************************************************************************
* djinterp [functional]                         producer_tests_generators.cpp
*
*   Tests for the source producers that do not wrap another producer:
* iterate, unfold, range, iota, repeat, repeat_n, cycle, generate, empty,
* single, and from_container.
*
*   Infinite producers (iterate / repeat / cycle / generate) are bounded
* with take_n before draining so the tests terminate.
*
* path:      /src/functional/producer_tests_generators.cpp
******************************************************************************/

#include "./producer_tests.hpp"

#include <vector>
#include <string>
#include <utility>


NS_DJINTERP
NS_TESTING


/*
test_producer_generators
  Tests the source producers.
  Tests the following:
  - iterate emits seed, step(seed), step(step(seed)), ... in order
  - unfold emits values until its step signals exhaustion, threading state
  - range: ascending, the default-step overload, descending (negative step),
    an empty range (start == end), a zero step (immediate exhaustion), and a
    range whose direction can never close the gap
  - iota matches range with an implicit step of one
  - repeat emits the same value indefinitely (bounded by take_n)
  - repeat_n emits exactly n copies, and repeat_n(v, 0) emits nothing
  - cycle wraps a container forever, and cycle of an empty container
    exhausts immediately (no hang)
  - generate invokes a nullary function on each pull
  - empty<T> yields no values
  - single emits exactly one value then exhausts
  - from_container drains a container in order, and an empty container
    yields nothing
*/
std::size_t
test_producer_generators(
    test_registry& _reg
)
{
    std::size_t before;

    before = _reg.failures();

    // ---- iterate: 1, 2, 3, 4, 5 (seed 1, step +1), bounded to 5 ----
    {
        std::vector<int> got = take_n(iterate(1, add_one()), 5).collect();
        std::vector<int> want;
        want.push_back(1); want.push_back(2); want.push_back(3);
        want.push_back(4); want.push_back(5);

        D_TESTING_CHECK(_reg, got == want);
    }

    // ---- unfold: countdown 5,4,3,2,1 then exhaust ----
    {
        auto p = unfold<int>(5, unfold_countdown());
        std::vector<int> got = p.collect();
        std::vector<int> want;
        want.push_back(5); want.push_back(4); want.push_back(3);
        want.push_back(2); want.push_back(1);

        D_TESTING_CHECK(_reg, got == want);
    }

    // ---- unfold with an initial state that exhausts immediately ----
    {
        auto p = unfold<int>(0, unfold_countdown());
        std::vector<int> got = p.collect();

        D_TESTING_CHECK(_reg, got.empty());
    }

    // ---- range ascending [1, 5) ----
    {
        std::vector<int> got = range(1, 5).collect();
        std::vector<int> want;
        want.push_back(1); want.push_back(2);
        want.push_back(3); want.push_back(4);

        D_TESTING_CHECK(_reg, got == want);
    }

    // ---- range with explicit step of 2: 0,2,4,6,8 ----
    {
        std::vector<int> got = range(0, 10, 2).collect();
        std::vector<int> want;
        want.push_back(0); want.push_back(2); want.push_back(4);
        want.push_back(6); want.push_back(8);

        D_TESTING_CHECK(_reg, got == want);
    }

    // ---- range descending with negative step: 5,4,3,2,1 ----
    {
        std::vector<int> got = range(5, 0, -1).collect();
        std::vector<int> want;
        want.push_back(5); want.push_back(4); want.push_back(3);
        want.push_back(2); want.push_back(1);

        D_TESTING_CHECK(_reg, got == want);
    }

    // ---- empty range: start == end ----
    {
        std::vector<int> got = range(3, 3).collect();

        D_TESTING_CHECK(_reg, got.empty());
    }

    // ---- zero step: immediate exhaustion (no infinite loop) ----
    {
        std::vector<int> got = range(0, 10, 0).collect();

        D_TESTING_CHECK(_reg, got.empty());
    }

    // ---- direction can never close the gap (positive step, start>end) ----
    {
        std::vector<int> got = range(10, 0, 1).collect();

        D_TESTING_CHECK(_reg, got.empty());
    }

    // ---- iota matches range default step ----
    {
        D_TESTING_CHECK(_reg, iota(2, 6).collect() == range(2, 6).collect());
    }

    // ---- repeat (bounded): five 9s ----
    {
        std::vector<int> got = take_n(repeat(9), 5).collect();
        std::vector<int> want(5, 9);

        D_TESTING_CHECK(_reg, got == want);
    }

    // ---- repeat_n: exactly three 4s ----
    {
        std::vector<int> got = repeat_n(4, 3).collect();
        std::vector<int> want(3, 4);

        D_TESTING_CHECK(_reg, got == want);
    }

    // ---- repeat_n(v, 0): nothing ----
    {
        std::vector<int> got = repeat_n(4, 0).collect();

        D_TESTING_CHECK(_reg, got.empty());
    }

    // ---- cycle: wrap [1,2,3] for 7 pulls -> 1,2,3,1,2,3,1 ----
    {
        std::vector<int> src;
        src.push_back(1); src.push_back(2); src.push_back(3);

        std::vector<int> got = take_n(cycle(src), 7).collect();
        std::vector<int> want;
        want.push_back(1); want.push_back(2); want.push_back(3);
        want.push_back(1); want.push_back(2); want.push_back(3);
        want.push_back(1);

        D_TESTING_CHECK(_reg, got == want);
    }

    // ---- cycle of an empty container exhausts immediately ----
    {
        std::vector<int> empty_src;
        std::vector<int> got = take_n(cycle(empty_src), 5).collect();

        D_TESTING_CHECK(_reg, got.empty());
    }

    // ---- generate: counting source 0..4 (bounded) ----
    {
        std::vector<int> got = take_n(generate(counting_source()), 5).collect();
        std::vector<int> want;
        want.push_back(0); want.push_back(1); want.push_back(2);
        want.push_back(3); want.push_back(4);

        D_TESTING_CHECK(_reg, got == want);
    }

    // ---- generate: constant source ----
    {
        std::vector<int> got =
            take_n(generate(constant_source(8)), 3).collect();
        std::vector<int> want(3, 8);

        D_TESTING_CHECK(_reg, got == want);
    }

    // ---- empty<T> yields nothing ----
    {
        std::vector<int> got = empty<int>().collect();

        D_TESTING_CHECK(_reg, got.empty());
    }

    // ---- single emits exactly one value ----
    {
        std::vector<int> got = single(123).collect();

        D_TESTING_CHECK(_reg, got.size() == 1);
        D_TESTING_CHECK(_reg, (got.size() == 1 && got[0] == 123));
    }

    // ---- single with a string element type ----
    {
        std::vector<std::string> got = single(std::string("x")).collect();

        D_TESTING_CHECK(_reg, got.size() == 1);
        D_TESTING_CHECK(_reg, (got.size() == 1 && got[0] == "x"));
    }

    // ---- from_container drains in order ----
    {
        std::vector<int> src;
        src.push_back(10); src.push_back(20); src.push_back(30);

        std::vector<int> got = from_container(src).collect();

        D_TESTING_CHECK(_reg, got == src);
    }

    // ---- from_container of an empty container yields nothing ----
    {
        std::vector<int> empty_src;
        std::vector<int> got = from_container(empty_src).collect();

        D_TESTING_CHECK(_reg, got.empty());
    }

    // ---- iterate threading a pair state (fibonacci firsts) ----
    {
        // seed (0,1); step (a,b) -> (b, a+b); take first members
        struct fib_step
        {
            std::pair<int, int>
            operator()(std::pair<int, int> _p) const
            {
                return std::make_pair(_p.second, _p.first + _p.second);
            }
        };

        auto fibs = iterate(std::make_pair(0, 1), fib_step());

        std::vector<std::pair<int, int> > pairs =
            take_n(fibs, 6).collect();

        // firsts: 0,1,1,2,3,5
        D_TESTING_CHECK(_reg, pairs.size() == 6);
        if (pairs.size() == 6)
        {
            D_TESTING_CHECK(_reg, pairs[0].first == 0);
            D_TESTING_CHECK(_reg, pairs[1].first == 1);
            D_TESTING_CHECK(_reg, pairs[2].first == 1);
            D_TESTING_CHECK(_reg, pairs[3].first == 2);
            D_TESTING_CHECK(_reg, pairs[4].first == 3);
            D_TESTING_CHECK(_reg, pairs[5].first == 5);
        }
    }

    return (_reg.failures() - before);
}


NS_END  // testing
NS_END  // djinterp
