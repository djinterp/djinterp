/*******************************************************************************
* djinterp [test]                                          compose_tests_tap.cpp
*
*   Tests for compose.hpp Section V: tap, the pass-through side-effect
* injector.  Covers value pass-through, observation of the side effect,
* invocation count, and composition into a pipe chain (tap returns its
* input so it can sit transparently between stages).
*
*
* path:      /tests/djinterp/cpp/functional/compose_tests_tap.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "compose_tests.hpp"


NS_DJINTERP
NS_TESTING



/*
compose_tests_tap
  Exercises compose.hpp Section V (tap).
  Tests the following:
  - tap returns its input value unchanged
  - the wrapped side-effect function observes the value
  - the side effect fires exactly once per call
  - tap is transparent inside a pipe chain (does not alter the result)
*/
bool compose_tests_tap()
{
    bool verdict = true;

    DUnitTestTally tally;

    // pass-through and observation
    run_unit_test(verdict,
        tally,
        "tap returns input unchanged and runs the side effect",
        [&]()
        {
            int seen = 0;
            auto t   = ::djinterp::tap([&seen](int _v) { seen = _v; });

            const int result = t(99);

            record_assertion(verdict,
                (result == 99),
                "tap returns its input unchanged");

            record_assertion(verdict,
                (seen == 99),
                "tap side effect observed the value");
        });

    // invocation count: the side effect fires once per call
    run_unit_test(verdict,
        tally,
        "tap fires its side effect exactly once per call",
        [&]()
        {
            int count = 0;
            auto t    = ::djinterp::tap([&count](int) { ++count; });

            t(1);
            t(2);
            t(3);

            record_assertion(verdict,
                (count == 3),
                "tap side effect fired once per call (3 calls)");
        });

    // transparency inside a pipe chain
    run_unit_test(verdict,
        tally,
        "tap is transparent inside a pipe chain",
        [&]()
        {
            int seen = 0;
            auto probe = ::djinterp::tap([&seen](int _v) { seen = _v; });

            // pipe(add_one, tap)(5): add_one(5) = 6, tap observes 6 and
            // returns 6 -- the chain result must equal add_one alone.
            const int piped = ::djinterp::pipe(add_one(), probe)(5);

            record_assertion(verdict,
                (piped == 6),
                "pipe(add_one, tap)(5) == 6 (tap does not alter result)");

            record_assertion(verdict,
                (seen == 6),
                "tap inside the chain observed the intermediate value");
        });

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
