/******************************************************************************
* djinterp [test]                                    atomic_tests_version.cpp
*
*   Section II - atomic_version.  The generation-counter wrapper mirrors
* atomic_size but exposes fetch_add plus the documented bump() mutation
* primitive (no fetch_sub / decrement).  This pass covers construction,
* load/store, fetch_add (with the 64-bit wrap edge), bump (returns the prior
* generation), both compare-exchanges, the implicit uint64_t conversion, a
* dedicated full-width (>32-bit) check, and - under C++20 - wait / notify.
*
*
* path:      /tests/djinterp/core/sync/atomic_tests_version.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.13
******************************************************************************/

// std
#include <cstdint>
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
tests_atomic_version_default_ctor()
{
    atomic_version v;

    D_AT_CHECK(v.load() == 0);
    D_AT_CHECK(static_cast<std::uint64_t>(v) == 0);

    return true;
}


bool
tests_atomic_version_explicit_ctor()
{
    atomic_version a(7);
    D_AT_CHECK(a.load() == 7);

    atomic_version z(0);
    D_AT_CHECK(z.load() == 0);

    // edge: maximum 64-bit generation
    atomic_version big(std::numeric_limits<std::uint64_t>::max());
    D_AT_CHECK(big.load() == std::numeric_limits<std::uint64_t>::max());

    return true;
}


// -------------------------------------------------------------------------
//  load / store
// -------------------------------------------------------------------------

bool
tests_atomic_version_load_store()
{
    atomic_version v;

    v.store(42);
    D_AT_CHECK(v.load() == 42);

    v.store(1000, std::memory_order_release);
    D_AT_CHECK(v.load(std::memory_order_acquire) == 1000);

    v.store(0, std::memory_order_relaxed);
    D_AT_CHECK(v.load(std::memory_order_relaxed) == 0);

    return true;
}


// -------------------------------------------------------------------------
//  fetch_add / bump
// -------------------------------------------------------------------------

bool
tests_atomic_version_fetch_add()
{
    atomic_version v(10);

    D_AT_CHECK(v.fetch_add(5) == 10);   // returns previous
    D_AT_CHECK(v.load() == 15);

    D_AT_CHECK(v.fetch_add(0) == 15);
    D_AT_CHECK(v.load() == 15);

    // edge: wrap at the top of the 64-bit range
    v.store(std::numeric_limits<std::uint64_t>::max());
    D_AT_CHECK(v.fetch_add(1) == std::numeric_limits<std::uint64_t>::max());
    D_AT_CHECK(v.load() == 0);

    // explicit order overload
    D_AT_CHECK(v.fetch_add(3, std::memory_order_relaxed) == 0);
    D_AT_CHECK(v.load() == 3);

    return true;
}


bool
tests_atomic_version_bump()
{
    atomic_version v;   // generation 0

    // bump returns the PRIOR generation - the snapshot a reader compares
    // against - and advances by one
    D_AT_CHECK(v.bump() == 0);
    D_AT_CHECK(v.load() == 1);
    D_AT_CHECK(v.bump() == 1);
    D_AT_CHECK(v.load() == 2);

    // explicit order overload
    D_AT_CHECK(v.bump(std::memory_order_seq_cst) == 2);
    D_AT_CHECK(v.load() == 3);

    return true;
}


// -------------------------------------------------------------------------
//  compare-exchange
// -------------------------------------------------------------------------

bool
tests_atomic_version_cas_strong()
{
    atomic_version v(5);

    std::uint64_t expected = 5;
    D_AT_CHECK(v.compare_exchange_strong(expected, 9));
    D_AT_CHECK(v.load() == 9);
    D_AT_CHECK(expected == 5);

    std::uint64_t wrong = 1;
    D_AT_CHECK(!v.compare_exchange_strong(wrong, 100));
    D_AT_CHECK(v.load() == 9);
    D_AT_CHECK(wrong == 9);   // rewritten to actual

    std::uint64_t e2 = 9;
    D_AT_CHECK(v.compare_exchange_strong(
        e2, 11, std::memory_order_acq_rel, std::memory_order_acquire));
    D_AT_CHECK(v.load() == 11);

    return true;
}


bool
tests_atomic_version_cas_weak()
{
    atomic_version v(5);

    std::uint64_t expected = 5;
    bool          ok       = false;
    for (int i = 0; (i < 10000) && !ok; ++i)
    {
        ok = v.compare_exchange_weak(expected, 9);
    }
    D_AT_CHECK(ok);
    D_AT_CHECK(v.load() == 9);

    std::uint64_t wrong = 1;
    D_AT_CHECK(!v.compare_exchange_weak(wrong, 100));
    D_AT_CHECK(wrong == 9);
    D_AT_CHECK(v.load() == 9);

    return true;
}


// -------------------------------------------------------------------------
//  conversion
// -------------------------------------------------------------------------

bool
tests_atomic_version_conversion()
{
    atomic_version v(77);

    const std::uint64_t x = v;   // implicit operator std::uint64_t()
    D_AT_CHECK(x == 77);

    D_AT_CHECK((v + 3) == 80);

    v.store(0);
    const std::uint64_t y = v;
    D_AT_CHECK(y == 0);

    return true;
}


// -------------------------------------------------------------------------
//  full 64-bit width
// -------------------------------------------------------------------------

bool
tests_atomic_version_large_values()
{
    // values that do not fit in 32 bits confirm the underlying type really is
    // uint64_t (a narrower store would truncate these)
    const std::uint64_t forty_bits = static_cast<std::uint64_t>(1) << 40;
    const std::uint64_t high       = static_cast<std::uint64_t>(0xF00DCAFEBABE1234ULL);

    atomic_version v(forty_bits);
    D_AT_CHECK(v.load() == forty_bits);

    v.store(high);
    D_AT_CHECK(v.load() == high);

    // one bump just below the ceiling, then across it
    v.store(std::numeric_limits<std::uint64_t>::max() - 1);
    D_AT_CHECK(v.bump() == std::numeric_limits<std::uint64_t>::max() - 1);
    D_AT_CHECK(v.load() == std::numeric_limits<std::uint64_t>::max());
    D_AT_CHECK(v.bump() == std::numeric_limits<std::uint64_t>::max());
    D_AT_CHECK(v.load() == 0);   // wrapped

    return true;
}


// -------------------------------------------------------------------------
//  C++20 wait / notify
// -------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

bool
tests_atomic_version_wait_returns_on_change()
{
    atomic_version v(1);

    v.wait(0);   // current (1) != old (0) -> returns immediately
    D_AT_CHECK(v.load() == 1);

    return true;
}


bool
tests_atomic_version_notify()
{
    atomic_version   v(0);
    std::atomic<int> woke(0);

    std::thread waiter([&]
    {
        v.wait(0);
        woke.fetch_add(1, std::memory_order_relaxed);
    });

    v.store(1, std::memory_order_release);
    v.notify_one();

    waiter.join();
    D_AT_CHECK(woke.load() == 1);
    D_AT_CHECK(v.load() == 1);

    v.notify_all();   // no-op with no waiter (line coverage)

    return true;
}

#endif  // C++20


NS_END  // testing
NS_END  // djinterp

#undef D_AT_CHECK
