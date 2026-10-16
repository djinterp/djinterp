/******************************************************************************
* djinterp [test]                               pool_tests_acquire_release.cpp
*
*   Section VI part 2 of the pool suite - the vending path.
*
*   acquire() takes three routes in order: pop the free list (when the
* release policy keeps one), bump within the current block, then grow and
* retry.  release() forks on the same policy constant: thread the slot back
* onto the free list, or do nothing but decrement the live count.  These
* tests walk every one of those branches and pin the observable ordering
* the free list guarantees.
*
*
* path:      /tests/djinterp/core/memory/pool_tests_acquire_release.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

// std
#include <cstring>
// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_acquire_returns_distinct_slots
  Verifies successive acquires return non-null, pairwise distinct
  addresses, both inside a single block and across block boundaries, for
  every release policy.
*/
bool
tests_acquire_returns_distinct_slots()
{
    constexpr std::size_t count = 10;
    void*                 slots[count] = {};

    {
        pool_fl_chunked pool(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            slots[i] = pool.acquire();
            D_PR_CHECK(slots[i] != nullptr);
        }

        D_PR_CHECK(pool_all_distinct(slots, count));
        D_PR_CHECK(pool.size() == count);
    }

    {
        pool_fl_contig pool(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            slots[i] = pool.acquire();
            D_PR_CHECK(slots[i] != nullptr);
        }

        D_PR_CHECK(pool_all_distinct(slots, count));
    }

    {
        pool_mono_chunked pool(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            slots[i] = pool.acquire();
            D_PR_CHECK(slots[i] != nullptr);
        }

        D_PR_CHECK(pool_all_distinct(slots, count));
    }

    {
        pool_gen_contig pool(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            slots[i] = pool.acquire();
            D_PR_CHECK(slots[i] != nullptr);
        }

        D_PR_CHECK(pool_all_distinct(slots, count));
    }

    return true;
}

/*
tests_acquired_slots_are_independently_writable
  Verifies a full element can be written through each slot and read back
  unchanged - the slots really are that wide, correctly aligned, and do
  not alias one another.
*/
bool
tests_acquired_slots_are_independently_writable()
{
    constexpr std::size_t count = 16;

    pool_fl_chunked pool(count);
    pool_medium_t*  elements[count] = {};

    for (std::size_t i = 0; i < count; ++i)
    {
        void* slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);

        elements[i] = static_cast<pool_medium_t*>(slot);
        elements[i]->m_lo = static_cast<std::uint64_t>(i) + 1u;
        elements[i]->m_hi = ~(static_cast<std::uint64_t>(i) + 1u);
    }

    // every value survives every other write
    for (std::size_t i = 0; i < count; ++i)
    {
        D_PR_CHECK(elements[i]->m_lo ==
                   (static_cast<std::uint64_t>(i) + 1u));
        D_PR_CHECK(elements[i]->m_hi ==
                   ~(static_cast<std::uint64_t>(i) + 1u));
    }

    // rewriting one slot leaves its neighbours untouched
    elements[7]->m_lo = 0xDEADBEEFu;
    D_PR_CHECK(elements[6]->m_lo == 7u);
    D_PR_CHECK(elements[8]->m_lo == 9u);
    D_PR_CHECK(elements[7]->m_lo == 0xDEADBEEFu);

    return true;
}

/*
tests_acquire_grows_on_exhaustion
  Verifies that once the bump region and the free list are both empty,
  acquire takes the grow-and-retry branch, raises capacity, and still
  returns a usable slot.
*/
bool
tests_acquire_grows_on_exhaustion()
{
    // chunked: capacity rises, existing slots stay put
    {
        pool_fl_chunked pool(2);
        D_PR_CHECK(pool.capacity() == 2u);

        void* first  = pool.acquire();
        void* second = pool.acquire();
        D_PR_CHECK(pool.size() == 2u);
        D_PR_CHECK(pool.capacity() == 2u);

        void* third = pool.acquire();
        D_PR_CHECK(third != nullptr);
        D_PR_CHECK(pool.capacity() > 2u);
        D_PR_CHECK(pool.size() == 3u);
        D_PR_CHECK(third != first);
        D_PR_CHECK(third != second);
    }

    // contiguous: capacity rises the same way
    {
        pool_fl_contig pool(2);

        pool.acquire();
        pool.acquire();
        D_PR_CHECK(pool.capacity() == 2u);

        void* third = pool.acquire();
        D_PR_CHECK(third != nullptr);
        D_PR_CHECK(pool.capacity() > 2u);
        D_PR_CHECK(pool.size() == 3u);
    }

    // growing from nothing at all
    {
        pool_mono_chunked pool;
        D_PR_CHECK(pool.capacity() == 0u);

        void* slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);
        D_PR_CHECK(pool.capacity() >= 1u);
        D_PR_CHECK(pool.size() == 1u);
    }

    return true;
}

/*
tests_release_null_pointer_is_ignored
  Verifies release(nullptr) takes the guard branch and returns without
  decrementing the live count or corrupting the free list - the live
  count is unsigned, so a stray decrement here would wrap.
*/
bool
tests_release_null_pointer_is_ignored()
{
    // on an empty pool: the count must not wrap below zero
    {
        pool_fl_chunked pool;

        pool.release(nullptr);

        D_PR_CHECK(pool.size() == 0u);
        D_PR_CHECK(pool.empty());
        D_PR_CHECK(pool.capacity() == 0u);
    }

    // with slots outstanding: the count is untouched
    {
        pool_fl_chunked pool(4);

        void* first  = pool.acquire();
        void* second = pool.acquire();
        D_PR_CHECK(pool.size() == 2u);

        pool.release(nullptr);
        D_PR_CHECK(pool.size() == 2u);

        // ... and the free list is still coherent afterwards
        pool.release(first);
        D_PR_CHECK(pool.size() == 1u);
        D_PR_CHECK(pool.acquire() == first);

        pool.release(second);
        D_PR_CHECK(pool.size() == 1u);
    }

    // the same guard applies to a monotonic pool
    {
        pool_mono_chunked pool(4);
        pool.acquire();

        pool.release(nullptr);
        D_PR_CHECK(pool.size() == 1u);
    }

    // release is noexcept
    {
        pool_fl_chunked pool;
        D_PR_CHECK(noexcept(pool.release(nullptr)));
    }

    return true;
}

/*
tests_free_list_recycles_in_lifo_order
  Verifies released slots are threaded onto an intrusive stack and come
  back in strict last-in first-out order, for both block layouts and for
  the generational policy as well.
*/
bool
tests_free_list_recycles_in_lifo_order()
{
    {
        pool_fl_chunked pool(4);

        void* a = pool.acquire();
        void* b = pool.acquire();
        void* c = pool.acquire();

        pool.release(a);
        pool.release(b);
        pool.release(c);
        D_PR_CHECK(pool.size() == 0u);

        // last released comes back first
        D_PR_CHECK(pool.acquire() == c);
        D_PR_CHECK(pool.acquire() == b);
        D_PR_CHECK(pool.acquire() == a);
        D_PR_CHECK(pool.size() == 3u);
    }

    // interleaved release and acquire keeps the stack discipline
    {
        pool_fl_contig pool(4);

        void* a = pool.acquire();
        void* b = pool.acquire();

        pool.release(a);
        D_PR_CHECK(pool.acquire() == a);

        pool.release(b);
        pool.release(a);
        D_PR_CHECK(pool.acquire() == a);
        D_PR_CHECK(pool.acquire() == b);
    }

    // generational recycles through the same list
    {
        pool_gen_chunked pool(4);

        void* a = pool.acquire();
        void* b = pool.acquire();

        pool.release(a);
        pool.release(b);

        D_PR_CHECK(pool.acquire() == b);
        D_PR_CHECK(pool.acquire() == a);
    }

    return true;
}

/*
tests_free_list_is_preferred_over_the_bump_region
  Verifies acquire drains the free list before consuming fresh capacity,
  so a recycled slot is reused even when untouched capacity remains.
*/
bool
tests_free_list_is_preferred_over_the_bump_region()
{
    pool_fl_chunked pool(8);

    void* first = pool.acquire();
    D_PR_CHECK(pool.size() == 1u);
    D_PR_CHECK(pool.capacity() == 8u);

    pool.release(first);
    D_PR_CHECK(pool.size() == 0u);

    // seven slots of the bump region are still untouched, yet the
    // recycled slot is what comes back
    void* again = pool.acquire();
    D_PR_CHECK(again == first);
    D_PR_CHECK(pool.capacity() == 8u);

    // once the list is empty the bump region takes over again
    void* fresh = pool.acquire();
    D_PR_CHECK(fresh != nullptr);
    D_PR_CHECK(fresh != first);

    // and capacity never rose, because free slots were always available
    D_PR_CHECK(pool.capacity() == 8u);

    return true;
}

/*
tests_monotonic_release_does_not_recycle_storage
  Verifies the monotonic policy's release is a bookkeeping no-op: the live
  count drops, but the slot is not returned for reuse, so the next acquire
  consumes fresh capacity instead.
*/
bool
tests_monotonic_release_does_not_recycle_storage()
{
    {
        pool_mono_chunked pool(4);

        void* first = pool.acquire();
        D_PR_CHECK(pool.size() == 1u);

        pool.release(first);
        D_PR_CHECK(pool.size() == 0u);
        D_PR_CHECK(pool.empty());

        // the released slot is NOT handed back - the bump region moves on
        void* next = pool.acquire();
        D_PR_CHECK(next != nullptr);
        D_PR_CHECK(next != first);
    }

    // consequence: a release/acquire loop exhausts capacity and grows,
    // where a free-list pool would have reused one slot forever
    {
        pool_mono_chunked pool(4);
        D_PR_CHECK(pool.capacity() == 4u);

        for (std::size_t i = 0; i < 4; ++i)
        {
            void* slot = pool.acquire();
            D_PR_CHECK(slot != nullptr);
            pool.release(slot);
        }

        D_PR_CHECK(pool.size() == 0u);
        D_PR_CHECK(pool.capacity() == 4u);

        // the fifth cycle has to grow, even though nothing is live
        void* overflow = pool.acquire();
        D_PR_CHECK(overflow != nullptr);
        D_PR_CHECK(pool.capacity() > 4u);
    }

    // only reset returns monotonic storage to circulation
    {
        pool_mono_chunked pool(4);

        for (std::size_t i = 0; i < 4; ++i)
        {
            pool.acquire();
        }

        pool.reset();
        D_PR_CHECK(pool.size() == 0u);
        D_PR_CHECK(pool.capacity() == 4u);

        for (std::size_t i = 0; i < 4; ++i)
        {
            D_PR_CHECK(pool.acquire() != nullptr);
        }

        D_PR_CHECK(pool.capacity() == 4u);
    }

    return true;
}

/*
tests_generational_release_recycles_like_free_list
  Verifies the generational policy takes the same per-slot release path as
  the free-list policy - advancing the generation counter changes nothing
  about how slots are recycled.
*/
bool
tests_generational_release_recycles_like_free_list()
{
    pool_gen_chunked pool(4);

    void* a = pool.acquire();
    void* b = pool.acquire();
    D_PR_CHECK(pool.size() == 2u);

    pool.release(b);
    D_PR_CHECK(pool.size() == 1u);
    D_PR_CHECK(pool.acquire() == b);

    // advancing the generation does not disturb the free list
    pool.release(a);
    pool.advance_generation();
    pool.advance_generation();
    D_PR_CHECK(pool.acquire() == a);
    D_PR_CHECK(pool.current_generation() == 2u);

    // capacity was never consumed beyond the original reservation
    D_PR_CHECK(pool.capacity() == 4u);

    return true;
}

/*
tests_acquire_release_cycles_do_not_grow_the_pool
  Verifies a long balanced acquire/release loop reuses a single slot and
  never raises capacity - the free list must not leak entries.
*/
bool
tests_acquire_release_cycles_do_not_grow_the_pool()
{
    // one slot, recycled a thousand times
    {
        pool_fl_chunked pool(1);
        D_PR_CHECK(pool.capacity() == 1u);

        void* expected = nullptr;

        for (std::size_t i = 0; i < 1000; ++i)
        {
            void* slot = pool.acquire();
            D_PR_CHECK(slot != nullptr);

            if (i == 0)
            {
                expected = slot;
            }
            else
            {
                D_PR_CHECK(slot == expected);
            }

            pool.release(slot);
        }

        D_PR_CHECK(pool.capacity() == 1u);
        D_PR_CHECK(pool.size() == 0u);
    }

    // a wider working set churns without growing either
    {
        constexpr std::size_t width = 8;
        pool_fl_contig        pool(width);
        void*                 held[width] = {};

        for (std::size_t round = 0; round < 200; ++round)
        {
            for (std::size_t i = 0; i < width; ++i)
            {
                held[i] = pool.acquire();
                D_PR_CHECK(held[i] != nullptr);
            }

            D_PR_CHECK(pool.size() == width);
            D_PR_CHECK(pool_all_distinct(held, width));

            for (std::size_t i = 0; i < width; ++i)
            {
                pool.release(held[i]);
            }

            D_PR_CHECK(pool.size() == 0u);
        }

        D_PR_CHECK(pool.capacity() == width);
    }

    return true;
}

/*
tests_acquire_fails_when_growth_is_refused
  Verifies the can_grow == false branch: once the reserved capacity is
  exhausted acquire returns nullptr rather than allocating, and the pool
  recovers as soon as a slot is released.
*/
bool
tests_acquire_fails_when_growth_is_refused()
{
    // chunked, with capacity reserved up front
    {
        pool_fl_chunked_fixed pool(3);
        D_PR_CHECK(pool.capacity() == 3u);

        void* a = pool.acquire();
        void* b = pool.acquire();
        void* c = pool.acquire();
        D_PR_CHECK(a != nullptr);
        D_PR_CHECK(b != nullptr);
        D_PR_CHECK(c != nullptr);
        D_PR_CHECK(pool.size() == 3u);

        // exhausted: the growth policy refuses, so acquire fails
        D_PR_CHECK(pool.acquire() == nullptr);
        D_PR_CHECK(pool.size() == 3u);
        D_PR_CHECK(pool.capacity() == 3u);

        // repeated failures stay failures, and change nothing
        D_PR_CHECK(pool.acquire() == nullptr);
        D_PR_CHECK(pool.capacity() == 3u);

        // releasing one makes exactly one more available
        pool.release(b);
        D_PR_CHECK(pool.acquire() == b);
        D_PR_CHECK(pool.acquire() == nullptr);
    }

    // contiguous behaves identically
    {
        pool_fl_contig_fixed pool(2);

        D_PR_CHECK(pool.acquire() != nullptr);
        D_PR_CHECK(pool.acquire() != nullptr);
        D_PR_CHECK(pool.acquire() == nullptr);
        D_PR_CHECK(pool.capacity() == 2u);
    }

    // a fixed pool that was never given capacity can serve nothing
    {
        pool_fl_chunked_fixed pool;

        D_PR_CHECK(pool.capacity() == 0u);
        D_PR_CHECK(pool.acquire() == nullptr);
        D_PR_CHECK(pool.size() == 0u);
        D_PR_CHECK(pool.bytes_allocated() == 0u);
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
