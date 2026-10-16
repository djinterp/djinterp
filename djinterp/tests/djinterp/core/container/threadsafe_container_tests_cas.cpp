/*******************************************************************************
* djinterp [test]                             threadsafe_container_tests_cas.cpp
*
*   Layer 5 - the CAS retry infrastructure: exponential_off (backoff) and
* cas_loop (the generic compare-exchange retry).  Defines the "cas" block.
*
*
* path:      /tests/djinterp/core/container/threadsafe_container_tests_cas.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
// djinterp
#include "threadsafe_container_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_cas_exponential_off
//   Verifies the backoff runs its whole schedule - the doubling spin phase and
//   the yield phase past the cap - and resets, without error.  It exposes no
//   observable state, so passing is "drove every branch and returned".
//   Tests the following:
//   - many calls across the spin-doubling and the yield-past-cap paths
//   - reset() followed by further calls
bool
tests_cas_exponential_off()
{
    exponential_off off;                    // default 4 .. 1024
    for (int i = 0; i < 16; ++i)            // doubles past the cap into yield
    {
        off();
    }
    off.reset();
    off();

    exponential_off small(1, 4);            // reaches the yield path quickly
    for (int i = 0; i < 6; ++i)
    {
        small();
    }
    return true;
}

// tests_cas_loop_success
//   Verifies an uncontended cas_loop commits and reports success, both with an
//   unlimited attempt budget and with a bounded one.
//   Tests the following:
//   - unlimited: returns true and lands current + 5
//   - bounded (max 5): returns true and lands current * 2
bool
tests_cas_loop_success()
{
    std::atomic<std::size_t> v(10);

    bool ok = cas_loop(v, [](std::size_t _cur) { return _cur + 5; });
    D_TC_CHECK(ok);
    D_TC_CHECK(v.load() == 15u);

    bool ok2 = cas_loop(v, [](std::size_t _cur) { return _cur * 2; }, 5);
    D_TC_CHECK(ok2);
    D_TC_CHECK(v.load() == 30u);
    return true;
}

// tests_cas_loop_update
//   Verifies the update function receives the current value and the committed
//   result is exactly the transformation it returned.
//   Tests the following:
//   - a squaring update lands current * current
//   - an additive update lands current + 100
bool
tests_cas_loop_update()
{
    std::atomic<std::size_t> v(7);

    cas_loop(v, [](std::size_t _cur) { return _cur * _cur; });
    D_TC_CHECK(v.load() == 49u);

    std::atomic<std::size_t> w(3);
    cas_loop(w, [](std::size_t _cur) { return _cur + 100; });
    D_TC_CHECK(w.load() == 103u);
    return true;
}

NS_END  // testing
NS_END  // djinterp
