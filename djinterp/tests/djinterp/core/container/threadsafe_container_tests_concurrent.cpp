/*******************************************************************************
* djinterp [test]                      threadsafe_container_tests_concurrent.cpp
*
*   Integration - genuine multi-threaded contention with std::thread.  Each
* test asserts a DETERMINISTIC invariant that holds for correct code under any
* interleaving (an atomic / lock-free counter loses no updates; a write lock
* serialises a plain counter), so there is no flakiness: a correct module
* always lands the exact total, a broken one does not.  Defines the
* "concurrent" block.
*
*
* path:      /tests/djinterp/core/container/threadsafe_container_tests_concurrent.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <thread>
#include <vector>
// djinterp
#include "threadsafe_container_tests.hpp"

NS_DJINTERP
NS_TESTING

namespace
{
    const int k_threads = 4;
    const int k_iters   = 10000;
}

// tests_concurrent_atomic_size
//   Verifies atomic_state's size counter is truly atomic under contention: no
//   increment is lost, and the symmetric decrements return it to zero.
//   Tests the following:
//   - N threads * M fetch_add_size(1) land exactly N*M
//   - N threads * M fetch_sub_size(1) return the counter to zero
bool
tests_concurrent_atomic_size()
{
    atomic_state st;

    std::vector<std::thread> up;
    for (int t = 0; t < k_threads; ++t)
    {
        up.emplace_back([&st]()
        {
            for (int k = 0; k < k_iters; ++k)
            {
                st.fetch_add_size(1);
            }
        });
    }
    for (std::size_t i = 0; i < up.size(); ++i)
    {
        up[i].join();
    }
    D_TC_CHECK(st.load_size() ==
               static_cast<std::size_t>(k_threads) * k_iters);

    std::vector<std::thread> down;
    for (int t = 0; t < k_threads; ++t)
    {
        down.emplace_back([&st]()
        {
            for (int k = 0; k < k_iters; ++k)
            {
                st.fetch_sub_size(1);
            }
        });
    }
    for (std::size_t i = 0; i < down.size(); ++i)
    {
        down[i].join();
    }
    D_TC_CHECK(st.load_size() == 0u);
    return true;
}

// tests_concurrent_cas_loop
//   Verifies cas_loop applies every increment under contention with no lost
//   updates - the defining correctness property of a CAS retry loop.
//   Tests the following:
//   - N threads each cas_loop-increment M times land exactly N*M
bool
tests_concurrent_cas_loop()
{
    std::atomic<std::size_t> v(0);

    std::vector<std::thread> ts;
    for (int t = 0; t < k_threads; ++t)
    {
        ts.emplace_back([&v]()
        {
            for (int k = 0; k < k_iters; ++k)
            {
                cas_loop(v, [](std::size_t _cur) { return _cur + 1; });
            }
        });
    }
    for (std::size_t i = 0; i < ts.size(); ++i)
    {
        ts[i].join();
    }
    D_TC_CHECK(v.load() ==
               static_cast<std::size_t>(k_threads) * k_iters);
    return true;
}

// tests_concurrent_write_lock
//   Verifies the exclusive write lock provides mutual exclusion: a PLAIN
//   (non-atomic) counter incremented only under write_lock lands the exact
//   total, which is possible only if the lock serialises the increments.
//   Tests the following:
//   - N threads * M increments under write_lock land exactly N*M
bool
tests_concurrent_write_lock()
{
    tc_counter<tc_excl> c;

    std::vector<std::thread> ts;
    for (int t = 0; t < k_threads; ++t)
    {
        ts.emplace_back([&c]()
        {
            for (int k = 0; k < k_iters; ++k)
            {
                typename tc_excl::write_lock_type g(c.mutex());
                c.bump_();
            }
        });
    }
    for (std::size_t i = 0; i < ts.size(); ++i)
    {
        ts[i].join();
    }
    D_TC_CHECK(c.value() ==
               static_cast<long>(k_threads) * k_iters);
    return true;
}

NS_END  // testing
NS_END  // djinterp
