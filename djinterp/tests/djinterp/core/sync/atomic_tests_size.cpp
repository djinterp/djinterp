/******************************************************************************
* djinterp [test]                                       atomic_tests_size.cpp
*
*   Section I - atomic_size.  A single-threaded, exhaustive pass over the
* semantic element-count wrapper: construction, load/store (default and
* explicit memory orders), fetch_add / fetch_sub (including the well-defined
* unsigned wrap edge), the increment / decrement conveniences, both flavours
* of compare-exchange (weak retried against spurious failure, strong
* deterministic), the implicit size_t conversion, and - under C++20 - the
* wait / notify surface.
*
*
* path:      /tests/djinterp/core/sync/atomic_tests_size.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.13
******************************************************************************/

// std
#include <cstddef>
#include <cstdio>
#include <limits>

// djinterp
#include "atomic_tests.hpp"   // establishes the D_ENV_LANG_* feature gates
                             //   (and pulls <atomic> via atomic.hpp)

// std (C++20-only; gated on the feature macro the header above establishes,
// so this include must follow it)
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #include <atomic>
    #include <thread>
#endif


// D_AT_CHECK
//   local assertion: on a false condition, prints the failing expression and
// source location and returns false from the enclosing predicate.  Unique
// letters (AT = Atomic Tests) so co-compiled suites never collide.
#define D_AT_CHECK(_cond)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("      [check] FAILED: %s\n"                           \
                        "               at %s:%d\n",                           \
                        #_cond, __FILE__, __LINE__);                           \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
    while (false)


NS_DJINTERP
NS_TESTING


// -------------------------------------------------------------------------
//  construction
// -------------------------------------------------------------------------

bool
tests_atomic_size_default_ctor()
{
    atomic_size s;

    D_AT_CHECK(s.load() == 0);
    D_AT_CHECK(static_cast<std::size_t>(s) == 0);   // conversion agrees

    return true;
}


bool
tests_atomic_size_explicit_ctor()
{
    atomic_size a(7);
    D_AT_CHECK(a.load() == 7);

    atomic_size z(0);
    D_AT_CHECK(z.load() == 0);

    // edge: the maximum representable count
    atomic_size big(std::numeric_limits<std::size_t>::max());
    D_AT_CHECK(big.load() == std::numeric_limits<std::size_t>::max());

    return true;
}


// -------------------------------------------------------------------------
//  load / store
// -------------------------------------------------------------------------

bool
tests_atomic_size_load_store()
{
    atomic_size s;

    s.store(42);
    D_AT_CHECK(s.load() == 42);

    // explicit memory orders exercise the order-taking overloads
    s.store(100, std::memory_order_release);
    D_AT_CHECK(s.load(std::memory_order_acquire) == 100);

    s.store(0, std::memory_order_relaxed);
    D_AT_CHECK(s.load(std::memory_order_relaxed) == 0);

    return true;
}


// -------------------------------------------------------------------------
//  fetch operations
// -------------------------------------------------------------------------

bool
tests_atomic_size_fetch_add()
{
    atomic_size s(10);

    D_AT_CHECK(s.fetch_add(5) == 10);   // returns the PREVIOUS value
    D_AT_CHECK(s.load() == 15);         // and applies the delta

    // adding zero is a no-op that still observes the current value
    D_AT_CHECK(s.fetch_add(0) == 15);
    D_AT_CHECK(s.load() == 15);

    // explicit order overload
    D_AT_CHECK(s.fetch_add(1, std::memory_order_relaxed) == 15);
    D_AT_CHECK(s.load() == 16);

    return true;
}


bool
tests_atomic_size_fetch_sub()
{
    atomic_size s(10);

    D_AT_CHECK(s.fetch_sub(4) == 10);
    D_AT_CHECK(s.load() == 6);

    // subtract down to exactly zero
    D_AT_CHECK(s.fetch_sub(6) == 6);
    D_AT_CHECK(s.load() == 0);

    // edge: unsigned wrap, 0 - 1 == SIZE_MAX (well-defined for unsigned)
    D_AT_CHECK(s.fetch_sub(1) == 0);
    D_AT_CHECK(s.load() == std::numeric_limits<std::size_t>::max());

    return true;
}


// -------------------------------------------------------------------------
//  convenience: increment / decrement
// -------------------------------------------------------------------------

bool
tests_atomic_size_increment()
{
    atomic_size s;   // 0

    D_AT_CHECK(s.increment() == 0);   // returns previous, then +1
    D_AT_CHECK(s.load() == 1);
    D_AT_CHECK(s.increment() == 1);
    D_AT_CHECK(s.load() == 2);

    // explicit order overload
    D_AT_CHECK(s.increment(std::memory_order_seq_cst) == 2);
    D_AT_CHECK(s.load() == 3);

    return true;
}


bool
tests_atomic_size_decrement()
{
    atomic_size s(2);

    D_AT_CHECK(s.decrement() == 2);
    D_AT_CHECK(s.load() == 1);
    D_AT_CHECK(s.decrement() == 1);
    D_AT_CHECK(s.load() == 0);

    // edge: decrement from zero wraps to SIZE_MAX
    D_AT_CHECK(s.decrement() == 0);
    D_AT_CHECK(s.load() == std::numeric_limits<std::size_t>::max());

    return true;
}


// -------------------------------------------------------------------------
//  compare-exchange
// -------------------------------------------------------------------------

bool
tests_atomic_size_cas_strong()
{
    atomic_size s(5);

    // success: expected matches -> swaps, returns true, expected untouched
    std::size_t expected = 5;
    D_AT_CHECK(s.compare_exchange_strong(expected, 9));
    D_AT_CHECK(s.load() == 9);
    D_AT_CHECK(expected == 5);

    // failure: expected wrong -> returns false, value unchanged, and expected
    //          is rewritten to the actual current value
    std::size_t wrong = 1;
    D_AT_CHECK(!s.compare_exchange_strong(wrong, 100));
    D_AT_CHECK(s.load() == 9);
    D_AT_CHECK(wrong == 9);

    // explicit success/failure order overload
    std::size_t e2 = 9;
    D_AT_CHECK(s.compare_exchange_strong(
        e2, 11, std::memory_order_acq_rel, std::memory_order_acquire));
    D_AT_CHECK(s.load() == 11);

    return true;
}


bool
tests_atomic_size_cas_weak()
{
    atomic_size s(5);

    // success path: weak may fail spuriously, so retry.  Single-threaded, so a
    // spurious failure reloads 'expected' to the still-current 5 and the loop
    // simply tries again.
    std::size_t expected = 5;
    bool        ok       = false;
    for (int i = 0; (i < 10000) && !ok; ++i)
    {
        ok = s.compare_exchange_weak(expected, 9);
    }
    D_AT_CHECK(ok);
    D_AT_CHECK(s.load() == 9);

    // failure path is deterministic: weak never *succeeds* spuriously, so a
    // wrong expected always fails and rewrites expected to the actual value.
    std::size_t wrong = 1;
    D_AT_CHECK(!s.compare_exchange_weak(wrong, 100));
    D_AT_CHECK(wrong == 9);
    D_AT_CHECK(s.load() == 9);

    return true;
}


// -------------------------------------------------------------------------
//  conversion
// -------------------------------------------------------------------------

bool
tests_atomic_size_conversion()
{
    atomic_size s(77);

    const std::size_t v = s;   // implicit operator std::size_t()
    D_AT_CHECK(v == 77);

    // usable directly in a size_t expression (converts, then arithmetic)
    D_AT_CHECK((s + 3) == 80);

    s.store(0);
    const std::size_t w = s;
    D_AT_CHECK(w == 0);

    return true;
}


// -------------------------------------------------------------------------
//  C++20 wait / notify
// -------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

bool
tests_atomic_size_wait_returns_on_change()
{
    atomic_size s(1);

    // current (1) != old (0), so wait must return immediately - never block
    s.wait(0);
    D_AT_CHECK(s.load() == 1);

    return true;
}


bool
tests_atomic_size_notify()
{
    atomic_size      s(0);
    std::atomic<int> woke(0);

    std::thread waiter([&]
    {
        s.wait(0);   // blocks while value == 0
        woke.fetch_add(1, std::memory_order_relaxed);
    });

    // publish the new value, THEN wake.  (If the waiter has not yet reached
    // wait(), it will observe 1 != 0 and return without needing the signal -
    // so this can never deadlock.)
    s.store(1, std::memory_order_release);
    s.notify_one();

    waiter.join();
    D_AT_CHECK(woke.load() == 1);
    D_AT_CHECK(s.load() == 1);

    // notify_all with no waiter is a well-defined no-op (line coverage)
    s.notify_all();

    return true;
}

#endif  // C++20


NS_END  // testing
NS_END  // djinterp

#undef D_AT_CHECK
