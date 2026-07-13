/******************************************************************************
* djinterp [test]                                atomic_tests_concurrency.cpp
*
*   Section VI - behaviour under real thread contention.  The single-threaded
* sections prove the API contract; these prove the types are actually atomic
* by hammering them from many threads and asserting DETERMINISTIC final
* invariants (never timing):
*
*     - N threads each increment K times  -> final count == N*K
*     - balanced incrementers + decrementers -> back to the start value
*     - N threads each bump K times        -> final generation == N*K
*     - a lock-free CAS-loop increment from N threads -> final count == N*K
*       (exercises compare_exchange_weak's success AND failure paths under
*        genuine contention)
*     - concurrent construct/destruct churn of atomic_flag_guard on one shared
*       flag leaves the flag CLEAR (its last operation is always a clear)
*
*   The flag churn deliberately does NOT use the guard as a mutex: the guard
* clears on destruct, so it does not hold across threads.  We assert only the
* post-condition that survives any interleaving.
*
*   PORTABILITY: requires threads (C++11+).  atomic.hpp itself requires C++11
* for <atomic>, so this section carries no additional gate.
*
*
* path:      /tests/djinterp/core/sync/atomic_tests_concurrency.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.13
******************************************************************************/

// std
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

// djinterp
#include "atomic_tests.hpp"


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


bool
tests_atomic_size_concurrent_increment()
{
    const int kThreads = 8;
    const int kPer     = 20000;

    atomic_size counter;   // 0

    std::vector<std::thread> ts;
    ts.reserve(static_cast<std::size_t>(kThreads));

    for (int i = 0; i < kThreads; ++i)
    {
        ts.emplace_back([&counter, kPer]
        {
            for (int k = 0; k < kPer; ++k)
            {
                counter.increment();
            }
        });
    }
    for (std::thread& t : ts) { t.join(); }

    D_AT_CHECK(counter.load() ==
               (static_cast<std::size_t>(kThreads) *
                static_cast<std::size_t>(kPer)));

    return true;
}


bool
tests_atomic_size_concurrent_inc_dec()
{
    const int         kPairs = 4;          // kPairs incrementers + kPairs decrementers
    const int         kPer   = 20000;
    const std::size_t kStart = 1000000;    // high enough to never underflow

    atomic_size counter(kStart);

    std::vector<std::thread> ts;
    ts.reserve(static_cast<std::size_t>(kPairs) * 2);

    for (int i = 0; i < kPairs; ++i)
    {
        ts.emplace_back([&counter, kPer]
        {
            for (int k = 0; k < kPer; ++k) { counter.increment(); }
        });
        ts.emplace_back([&counter, kPer]
        {
            for (int k = 0; k < kPer; ++k) { counter.decrement(); }
        });
    }
    for (std::thread& t : ts) { t.join(); }

    // equal increments and decrements net to zero -> back to the start value
    D_AT_CHECK(counter.load() == kStart);

    return true;
}


bool
tests_atomic_version_concurrent_bump()
{
    const int kThreads = 8;
    const int kPer     = 20000;

    atomic_version gen;   // 0

    std::vector<std::thread> ts;
    ts.reserve(static_cast<std::size_t>(kThreads));

    for (int i = 0; i < kThreads; ++i)
    {
        ts.emplace_back([&gen, kPer]
        {
            for (int k = 0; k < kPer; ++k) { gen.bump(); }
        });
    }
    for (std::thread& t : ts) { t.join(); }

    D_AT_CHECK(gen.load() ==
               (static_cast<std::uint64_t>(kThreads) *
                static_cast<std::uint64_t>(kPer)));

    return true;
}


bool
tests_atomic_size_concurrent_cas()
{
    const int kThreads = 8;
    const int kPer     = 10000;

    atomic_size counter;   // 0

    std::vector<std::thread> ts;
    ts.reserve(static_cast<std::size_t>(kThreads));

    for (int i = 0; i < kThreads; ++i)
    {
        ts.emplace_back([&counter, kPer]
        {
            for (int k = 0; k < kPer; ++k)
            {
                // lock-free increment: read, then CAS; on failure the weak CAS
                // reloads 'cur' with the actual value, so the retry converges
                std::size_t cur = counter.load(std::memory_order_relaxed);
                while (!counter.compare_exchange_weak(cur, cur + 1))
                {
                    // 'cur' now holds the fresh value - just retry
                }
            }
        });
    }
    for (std::thread& t : ts) { t.join(); }

    D_AT_CHECK(counter.load() ==
               (static_cast<std::size_t>(kThreads) *
                static_cast<std::size_t>(kPer)));

    return true;
}


bool
tests_atomic_flag_guard_concurrent_churn()
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

    const int kThreads = 8;
    const int kPer     = 20000;

    std::vector<std::thread> ts;
    ts.reserve(static_cast<std::size_t>(kThreads));

    for (int i = 0; i < kThreads; ++i)
    {
        ts.emplace_back([&flag, kPer]
        {
            for (int k = 0; k < kPer; ++k)
            {
                atomic_flag_guard g(flag);   // test_and_set, then clear on exit
                (void)g.was_set();
            }
        });
    }
    for (std::thread& t : ts) { t.join(); }

    // Every construct (set) is program-order followed by its destruct (clear),
    // and all threads have joined - so the last operation in the flag's
    // modification order is a clear.  A fresh guard therefore sees it clear.
    atomic_flag_guard final_guard(flag);
    D_AT_CHECK(!final_guard.was_set());

    return true;
}


NS_END  // testing
NS_END  // djinterp

#undef D_AT_CHECK
