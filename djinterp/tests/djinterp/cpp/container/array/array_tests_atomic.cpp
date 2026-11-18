/*******************************************************************************
* djinterp [test]                                         array_tests_atomic.cpp
*
*   DAtomicArray tests: construction, the per-element atomic operations,
* iteration over the atomics, and four threads' increments with none lost.
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests_atomic.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// std
#include <atomic>       // std::atomic, std::memory_order
#include <cstddef>      // std::size_t
#include <thread>       // std::thread
#include <type_traits>  // std::is_copy_constructible
#include <vector>       // std::vector
// djinterp
#include "array_tests.hpp"  // suite header (normal face)


NS_DJINTERP
NS_TESTING

/*
tests_atomic_construction
  Construction and the constants.
  Tests the following:
  - default construction stores 0 in every element
  - the fill constructor stores its value in every element
  - extent, size() and empty(); an extent-0 array is empty
  - value_type and atomic_value_type
  - no copy or move, construction or assignment
*/
bool tests_atomic_construction()
{
    atomic4 a;
    atomic4 f(7);

    for (std::size_t i = 0; i < 4; ++i)
    {
        D_AY_CHECK(a.load(i) == 0);
        D_AY_CHECK(f.load(i) == 7);
    }

    D_AY_CHECK(atomic4::extent == 4 && a.size() == 4 && !a.empty());
    D_AY_CHECK(atomic4::lifetime == DArrayLifetime::mutable_lifetime);
    D_AY_CHECK(atomic4::iterable);

    DAtomicArray<DArrayOptType<int>, DArrayOptExtent<0>> z;
    D_AY_CHECK(z.empty() && z.size() == 0);
    D_AY_CHECK(z.is_lock_free());

    D_AY_CHECK((std::is_same<atomic4::value_type, int>::value));
    D_AY_CHECK((std::is_same<atomic4::atomic_value_type,
                             std::atomic<int>>::value));

    D_AY_CHECK(!std::is_copy_constructible<atomic4>::value);
    D_AY_CHECK(!std::is_move_constructible<atomic4>::value);
    D_AY_CHECK(!std::is_copy_assignable<atomic4>::value);
    D_AY_CHECK(!std::is_move_assignable<atomic4>::value);

    return true;
}

/*
tests_atomic_load_store_exchange
  load, store and exchange per element.
  Tests the following:
  - store then load, at the default and at relaxed order
  - exchange returns the old value and stores the new
  - the other elements are untouched
*/
bool tests_atomic_load_store_exchange()
{
    atomic4 a;

    a.store(1, 5);
    D_AY_CHECK(a.load(1) == 5);

    a.store(2, 6, std::memory_order_relaxed);
    D_AY_CHECK(a.load(2, std::memory_order_relaxed) == 6);

    D_AY_CHECK(a.exchange(1, 9) == 5);
    D_AY_CHECK(a.load(1) == 9);

    D_AY_CHECK(a.load(0) == 0 && a.load(3) == 0);

    return true;
}

/*
tests_atomic_fetch_operations
  The fetch operations return the old value.
  Tests the following:
  - from 12: fetch_add 3 gives 12 (15 stored)
  - fetch_sub 5 gives 15 (10 stored)
  - fetch_and 6 gives 10 (2 stored)
  - fetch_or 5 gives 2 (7 stored)
  - fetch_xor 3 gives 7 (4 stored)
  - the other elements are untouched
*/
bool tests_atomic_fetch_operations()
{
    atomic4 a;
    a.store(0, 12);

    D_AY_CHECK(a.fetch_add(0, 3) == 12);
    D_AY_CHECK(a.fetch_sub(0, 5) == 15);
    D_AY_CHECK(a.fetch_and(0, 6) == 10);
    D_AY_CHECK(a.fetch_or(0, 5) == 2);
    D_AY_CHECK(a.fetch_xor(0, 3) == 7);
    D_AY_CHECK(a.load(0) == 4);

    D_AY_CHECK(a.load(1) == 0 && a.load(3) == 0);

    return true;
}

/*
tests_atomic_compare_exchange
  The compare-exchange pair.
  Tests the following:
  - strong, expected matches: true, desired stored
  - strong, expected stale: false, expected loaded with the current value,
    nothing stored
  - weak, retried in a loop, reaches its value
*/
bool tests_atomic_compare_exchange()
{
    atomic4 a;
    a.store(2, 4);

    int expected = 4;
    D_AY_CHECK(a.compare_exchange_strong(2, expected, 8));
    D_AY_CHECK(a.load(2) == 8);

    expected = 4;
    D_AY_CHECK(!a.compare_exchange_strong(2, expected, 9));
    D_AY_CHECK(expected == 8);
    D_AY_CHECK(a.load(2) == 8);

    while (!a.compare_exchange_weak(2, expected, expected + 1))
    {
    }

    D_AY_CHECK(a.load(2) == 9);

    return true;
}

/*
tests_atomic_fill_and_iteration
  fill, iteration over the atomics, data() and is_lock_free().
  Tests the following:
  - fill stores its value everywhere
  - range-for visits four std::atomic<int>, and a store through one lands
  - end - begin is the extent; data() is begin; const begin is const
  - is_lock_free() is true where std::atomic<int> is always lock-free
*/
bool tests_atomic_fill_and_iteration()
{
    atomic4 a;
    a.fill(3);

    int sum = 0;

    for (auto& x : a)
    {
        sum += x.load();
    }

    D_AY_CHECK(sum == 12);

    a.begin()[1].store(20);
    D_AY_CHECK(a.load(1) == 20);

    D_AY_CHECK(a.end() - a.begin() == 4);
    D_AY_CHECK(a.data() == a.begin());
    D_AY_CHECK(a.data()[2].load() == 3);

    const atomic4& c = a;
    D_AY_CHECK((std::is_same<decltype(c.begin()),
                             const std::atomic<int>*>::value));

    if (std::atomic<int>::is_always_lock_free)
    {
        D_AY_CHECK(a.is_lock_free());
    }

    return true;
}

/*
tests_atomic_concurrent_increments
  No increment is lost under contention.
  Tests the following:
  - four threads each fetch_add 1 to every element 5000 times
  - every element ends at 20000
*/
bool tests_atomic_concurrent_increments()
{
    const int threads = 4;
    const int rounds  = 5000;

    atomic4                  a;
    std::vector<std::thread> pool;

    for (int t = 0; t < threads; ++t)
    {
        pool.emplace_back(
            [&a]()
            {
                for (int r = 0; r < rounds; ++r)
                {
                    for (std::size_t i = 0; i < 4; ++i)
                    {
                        a.fetch_add(i, 1, std::memory_order_relaxed);
                    }
                }
            });
    }

    for (auto& th : pool)
    {
        th.join();
    }

    for (std::size_t i = 0; i < 4; ++i)
    {
        D_AY_CHECK(a.load(i) == threads * rounds);
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
