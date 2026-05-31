/******************************************************************************
* djinterp [test]                                  compose_tests_memoize.cpp
*
*   Tests for compose.hpp Section VI: memoize, the caching wrapper for a
* pure unary function.  Covers correct results, caching (the wrapped
* function runs once per distinct input), cache growth across distinct
* inputs, cache_size reporting, clear_cache, recomputation after a
* clear, and deduction of the output type via the callable_result_t
* default template argument.
******************************************************************************/

#include "compose_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;


namespace {

    // instrumented pure function: counts invocations so the test can
    // distinguish a cache hit from a recomputation.  File-local state is
    // fine here because the memoize section is a single translation unit.
    int g_call_count = 0;

    int counted_add_one(const int& _x)
    {
        ++g_call_count;

        return _x + 1;
    }

}  // namespace


/*
compose_tests_memoize
  Exercises compose.hpp Section VI (memoize).
  Tests the following:
  - the wrapped function produces correct results
  - a repeated input is served from cache (underlying fn runs once)
  - distinct inputs each run the underlying fn and grow the cache
  - cache_size reflects the number of distinct cached inputs
  - clear_cache empties the cache
  - a cleared input is recomputed on the next call
  - the output type is deduced via callable_result_t (no explicit 3rd arg)
*/
void
compose_tests_memoize(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // correctness + caching of a repeated input
    run_unit_test(
        _handler,
        tally,
        "memoize returns correct results and caches repeats",
        [&]()
        {
            g_call_count = 0;

            auto m = ::djinterp::memoize<int (*)(const int&), int>(
                &counted_add_one);

            record_assertion(
                _handler,
                (m(5) == 6),
                "memoized counted_add_one(5) == 6");

            // repeat the same input -- should be served from cache
            record_assertion(
                _handler,
                (m(5) == 6),
                "repeated input returns the same result");

            record_assertion(
                _handler,
                (g_call_count == 1),
                "underlying function ran exactly once for a repeat");

            record_assertion(
                _handler,
                (m.cache_size() == 1),
                "cache holds one entry after one distinct input");
        });

    // distinct inputs grow the cache
    run_unit_test(
        _handler,
        tally,
        "memoize grows the cache for distinct inputs",
        [&]()
        {
            g_call_count = 0;

            auto m = ::djinterp::memoize<int (*)(const int&), int>(
                &counted_add_one);

            (void)m(5);
            (void)m(6);
            (void)m(7);

            record_assertion(
                _handler,
                (g_call_count == 3),
                "three distinct inputs ran the function three times");

            record_assertion(
                _handler,
                (m.cache_size() == 3),
                "cache holds three entries for three distinct inputs");
        });

    // clear_cache + recomputation
    run_unit_test(
        _handler,
        tally,
        "clear_cache empties the cache and forces recomputation",
        [&]()
        {
            g_call_count = 0;

            auto m = ::djinterp::memoize<int (*)(const int&), int>(
                &counted_add_one);

            (void)m(5);

            record_assertion(
                _handler,
                (m.cache_size() == 1),
                "cache holds one entry before clear");

            m.clear_cache();

            record_assertion(
                _handler,
                (m.cache_size() == 0),
                "cache is empty after clear_cache");

            // recompute the previously-cached input
            record_assertion(
                _handler,
                (m(5) == 6),
                "cleared input recomputes to the same result");

            record_assertion(
                _handler,
                (g_call_count == 2),
                "underlying function ran again after the clear");
        });

    // output-type deduction: no explicit _Output template argument
    run_unit_test(
        _handler,
        tally,
        "memoize deduces the output type via callable_result_t",
        [&]()
        {
            // free_add_one returns int; we supply only _Function and
            // _Input and rely on the defaulted _Output.
            auto m = ::djinterp::memoize<int (*)(const int&), int>(
                &free_add_one);

            record_assertion(
                _handler,
                (m(41) == 42),
                "deduced-output memoize(41) == 42");

            record_assertion(
                _handler,
                (::djinterp::is_memoized<decltype(m)>::value),
                "the wrapper is structurally a memoize helper");
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
