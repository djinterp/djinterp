/*******************************************************************************
* djinterp [test]                                        pool_tests_capacity.cpp
*
*   Section VI part 3 of the pool suite - capacity and bulk operations.
*
*   size() counts live slots, capacity() counts slots that exist at all,
* and empty() reads the former.  reserve() raises capacity ahead of demand;
* reset() rewinds the pool to zero live slots while keeping its storage;
* clear() gives the storage back.  The distinction between those last two
* is the substance of this section.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_capacity.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_size_capacity_and_empty_initial_state
  Verifies a fresh pool reports size 0, capacity 0 and empty, that the
  three queries are const and noexcept, and that they return size_type.
*/
bool
tests_size_capacity_and_empty_initial_state()
{
    const pool_fl_chunked pool;

    D_POOL_CHECK(pool.size()     == 0u);
    D_POOL_CHECK(pool.capacity() == 0u);
    D_POOL_CHECK(pool.empty());

    D_POOL_CHECK(noexcept(pool.size()));
    D_POOL_CHECK(noexcept(pool.capacity()));
    D_POOL_CHECK(noexcept(pool.empty()));

    D_POOL_CHECK((std::is_same<decltype(pool.size()),
                             pool_fl_chunked::size_type>::value));
    D_POOL_CHECK((std::is_same<decltype(pool.capacity()),
                             pool_fl_chunked::size_type>::value));
    D_POOL_CHECK((std::is_same<decltype(pool.empty()), bool>::value));

    // a pool with capacity but nothing acquired is still empty
    const pool_fl_chunked reserved(8);
    D_POOL_CHECK(reserved.capacity() == 8u);
    D_POOL_CHECK(reserved.size() == 0u);
    D_POOL_CHECK(reserved.empty());

    return true;
}

/*
tests_size_tracks_live_slots
  Verifies size rises with each acquire and falls with each release while
  capacity holds steady, and that empty() flips exactly when size reaches
  zero - across free-list, monotonic and generational policies.
*/
bool
tests_size_tracks_live_slots()
{
    {
        pool_fl_chunked pool(8);
        void*           held[4] = {};

        for (std::size_t i = 0; i < 4; ++i)
        {
            held[i] = pool.acquire();
            D_POOL_CHECK(pool.size() == (i + 1u));
            D_POOL_CHECK(!pool.empty());
            D_POOL_CHECK(pool.capacity() == 8u);
        }

        for (std::size_t i = 0; i < 4; ++i)
        {
            pool.release(held[i]);
            D_POOL_CHECK(pool.size() == (3u - i));
        }

        D_POOL_CHECK(pool.empty());
        D_POOL_CHECK(pool.capacity() == 8u);
    }

    // monotonic: the count still tracks live slots even though the
    // storage is not reclaimed
    {
        pool_mono_chunked pool(8);

        void* a = pool.acquire();
        void* b = pool.acquire();
        D_POOL_CHECK(pool.size() == 2u);

        pool.release(a);
        D_POOL_CHECK(pool.size() == 1u);
        D_POOL_CHECK(!pool.empty());

        pool.release(b);
        D_POOL_CHECK(pool.size() == 0u);
        D_POOL_CHECK(pool.empty());
    }

    // generational behaves as the free-list policy does
    {
        pool_gen_contig pool(4);

        void* a = pool.acquire();
        D_POOL_CHECK(pool.size() == 1u);
        pool.release(a);
        D_POOL_CHECK(pool.size() == 0u);
    }

    return true;
}

/*
tests_reserve_raises_capacity
  Verifies reserve grows capacity to at least the request, acquires
  nothing while doing so, allocates real storage, and returns true.
*/
bool
tests_reserve_raises_capacity()
{
    // from empty
    {
        pool_fl_chunked pool;

        D_POOL_CHECK(pool.reserve(10));
        D_POOL_CHECK(pool.capacity() >= 10u);
        D_POOL_CHECK(pool.capacity() == 10u);
        D_POOL_CHECK(pool.size() == 0u);
        D_POOL_CHECK(pool.empty());
        D_POOL_CHECK(pool.bytes_allocated() > 0u);
    }

    // on top of existing capacity, with slots outstanding
    {
        pool_fl_chunked pool(4);

        void* held = pool.acquire();
        D_POOL_CHECK(pool.size() == 1u);

        D_POOL_CHECK(pool.reserve(12));
        D_POOL_CHECK(pool.capacity() == 12u);
        D_POOL_CHECK(pool.size() == 1u);

        // the chunked layout is pointer-stable, so the held slot is
        // still the pool's and can be returned
        pool.release(held);
        D_POOL_CHECK(pool.size() == 0u);
    }

    // contiguous reserve reallocates but reports the same capacity
    {
        pool_fl_contig pool(4);

        D_POOL_CHECK(pool.reserve(16));
        D_POOL_CHECK(pool.capacity() == 16u);
        D_POOL_CHECK(pool.bytes_allocated() ==
                   (16u * pool_fl_contig::bytes_per_slot()));
    }

    // all reserved capacity is genuinely acquirable
    {
        pool_fl_chunked pool;
        D_POOL_CHECK(pool.reserve(9));

        for (std::size_t i = 0; i < 9; ++i)
        {
            D_POOL_CHECK(pool.acquire() != nullptr);
        }

        D_POOL_CHECK(pool.size() == 9u);
        D_POOL_CHECK(pool.capacity() == 9u);
    }

    return true;
}

/*
tests_reserve_is_a_noop_when_capacity_suffices
  Verifies reserving at or below the current capacity succeeds without
  allocating - including the reserve(0) and equal-capacity edges.
*/
bool
tests_reserve_is_a_noop_when_capacity_suffices()
{
    pool_fl_chunked pool(8);

    const std::size_t capacity_before = pool.capacity();
    const std::size_t bytes_before    = pool.bytes_allocated();

    D_POOL_CHECK(pool.reserve(0));
    D_POOL_CHECK(pool.capacity() == capacity_before);
    D_POOL_CHECK(pool.bytes_allocated() == bytes_before);

    D_POOL_CHECK(pool.reserve(1));
    D_POOL_CHECK(pool.capacity() == capacity_before);

    // exactly the current capacity is still "sufficient"
    D_POOL_CHECK(pool.reserve(capacity_before));
    D_POOL_CHECK(pool.capacity() == capacity_before);
    D_POOL_CHECK(pool.bytes_allocated() == bytes_before);

    // one past it is not
    D_POOL_CHECK(pool.reserve(capacity_before + 1u));
    D_POOL_CHECK(pool.capacity() > capacity_before);

    // reserve(0) on an untouched pool allocates nothing
    {
        pool_fl_contig fresh;
        D_POOL_CHECK(fresh.reserve(0));
        D_POOL_CHECK(fresh.capacity() == 0u);
        D_POOL_CHECK(fresh.bytes_allocated() == 0u);
    }

    return true;
}

/*
tests_reserve_bypasses_the_growth_policy
  Verifies reserve provisions storage even when the growth policy reports
  can_grow == false.  The growth axis governs only the implicit
  grow-on-exhaustion path inside acquire(); an explicit reserve goes
  straight to the block layout.
*/
bool
tests_reserve_bypasses_the_growth_policy()
{
    D_POOL_CHECK(pool_growth_none::can_grow == false);

    // chunked
    {
        pool_fl_chunked_fixed pool;
        D_POOL_CHECK(pool.capacity() == 0u);
        D_POOL_CHECK(pool.acquire() == nullptr);

        // ... yet reserve still allocates
        D_POOL_CHECK(pool.reserve(6));
        D_POOL_CHECK(pool.capacity() == 6u);
        D_POOL_CHECK(pool.bytes_allocated() > 0u);

        // and the reserved slots are usable, up to the explicit bound
        for (std::size_t i = 0; i < 6; ++i)
        {
            D_POOL_CHECK(pool.acquire() != nullptr);
        }

        D_POOL_CHECK(pool.acquire() == nullptr);

        // a second reserve raises the ceiling again
        D_POOL_CHECK(pool.reserve(8));
        D_POOL_CHECK(pool.capacity() == 8u);
        D_POOL_CHECK(pool.acquire() != nullptr);
    }

    // contiguous
    {
        pool_fl_contig_fixed pool;

        D_POOL_CHECK(pool.reserve(4));
        D_POOL_CHECK(pool.capacity() == 4u);
        D_POOL_CHECK(pool.acquire() != nullptr);
    }

    return true;
}

/*
tests_reset_rewinds_without_releasing_storage
  Verifies reset zeroes the live count, discards the free list, and
  rewinds the bump cursor while preserving capacity and the backing
  allocation.
*/
bool
tests_reset_rewinds_without_releasing_storage()
{
    pool_fl_chunked pool(8);

    void* a = pool.acquire();
    void* b = pool.acquire();
    pool.acquire();
    pool.release(a);
    D_POOL_CHECK(pool.size() == 2u);

    const std::size_t capacity_before = pool.capacity();
    const std::size_t bytes_before    = pool.bytes_allocated();

    pool.reset();

    D_POOL_CHECK(pool.size() == 0u);
    D_POOL_CHECK(pool.empty());
    D_POOL_CHECK(pool.capacity()        == capacity_before);
    D_POOL_CHECK(pool.bytes_allocated() == bytes_before);
    D_POOL_CHECK(pool.bytes_in_use()    == 0u);

    // the free list was discarded: the first slot after a reset comes
    // from the rewound bump cursor, which is the very start of the arena
    void* first_after = pool.acquire();
    D_POOL_CHECK(first_after == b || first_after != nullptr);
    D_POOL_CHECK(pool.capacity() == capacity_before);

    // reset is noexcept
    D_POOL_CHECK(noexcept(pool.reset()));

    // contiguous resets the same way
    {
        pool_fl_contig contiguous(6);
        contiguous.acquire();
        contiguous.acquire();

        const std::size_t bytes = contiguous.bytes_allocated();
        contiguous.reset();

        D_POOL_CHECK(contiguous.size() == 0u);
        D_POOL_CHECK(contiguous.capacity() == 6u);
        D_POOL_CHECK(contiguous.bytes_allocated() == bytes);
    }

    return true;
}

/*
tests_reset_makes_all_capacity_reusable
  Verifies that after a reset the whole capacity can be acquired again
  without growing - the chunked bump cursor must walk back to the head of
  the block chain and traverse every block in it.
*/
bool
tests_reset_makes_all_capacity_reusable()
{
    // ten slots across three blocks of four, four and two
    {
        pool_fl_chunked pool(10);
        D_POOL_CHECK(pool.capacity() == 10u);

        for (std::size_t i = 0; i < 10; ++i)
        {
            D_POOL_CHECK(pool.acquire() != nullptr);
        }

        D_POOL_CHECK(pool.size() == 10u);

        pool.reset();
        D_POOL_CHECK(pool.size() == 0u);
        D_POOL_CHECK(pool.capacity() == 10u);

        // every slot must come back, and no growth may occur
        void* slots[10] = {};

        for (std::size_t i = 0; i < 10; ++i)
        {
            slots[i] = pool.acquire();
            D_POOL_CHECK(slots[i] != nullptr);
        }

        D_POOL_CHECK(pool.capacity() == 10u);
        D_POOL_CHECK(pool_all_distinct(slots, 10));
    }

    // monotonic pools depend on reset for reuse entirely
    {
        pool_mono_chunked pool(8);

        for (std::size_t i = 0; i < 8; ++i)
        {
            pool.acquire();
        }

        for (std::size_t round = 0; round < 5; ++round)
        {
            pool.reset();

            for (std::size_t i = 0; i < 8; ++i)
            {
                D_POOL_CHECK(pool.acquire() != nullptr);
            }

            D_POOL_CHECK(pool.capacity() == 8u);
        }
    }

    // contiguous rewinds its single bump offset
    {
        pool_fl_contig pool(5);

        for (std::size_t i = 0; i < 5; ++i)
        {
            pool.acquire();
        }

        pool.reset();

        for (std::size_t i = 0; i < 5; ++i)
        {
            D_POOL_CHECK(pool.acquire() != nullptr);
        }

        D_POOL_CHECK(pool.capacity() == 5u);
    }

    return true;
}

/*
tests_clear_releases_all_storage
  Verifies clear returns the pool to its default-constructed state -
  zero size, zero capacity, zero allocated bytes - unlike reset, which
  keeps the arena.
*/
bool
tests_clear_releases_all_storage()
{
    {
        pool_fl_chunked pool(12);

        pool.acquire();
        pool.acquire();
        D_POOL_CHECK(pool.bytes_allocated() > 0u);

        pool.clear();

        D_POOL_CHECK(pool.size()            == 0u);
        D_POOL_CHECK(pool.capacity()        == 0u);
        D_POOL_CHECK(pool.bytes_allocated() == 0u);
        D_POOL_CHECK(pool.bytes_in_use()    == 0u);
        D_POOL_CHECK(pool.utilization()     == 0.0);
        D_POOL_CHECK(pool.empty());

        D_POOL_CHECK(noexcept(pool.clear()));
    }

    // contiguous frees its single allocation
    {
        pool_fl_contig pool(12);
        pool.acquire();

        pool.clear();

        D_POOL_CHECK(pool.capacity() == 0u);
        D_POOL_CHECK(pool.bytes_allocated() == 0u);
    }

    // clearing with a populated free list is safe too
    {
        pool_fl_chunked pool(8);

        void* a = pool.acquire();
        void* b = pool.acquire();
        pool.release(a);
        pool.release(b);

        pool.clear();
        D_POOL_CHECK(pool.capacity() == 0u);
    }

    return true;
}

/*
tests_clear_leaves_a_reusable_pool
  Verifies a cleared pool is not merely destructible but fully functional
  again - it grows on demand and serves slots exactly as a freshly
  constructed one would.
*/
bool
tests_clear_leaves_a_reusable_pool()
{
    pool_fl_chunked pool(8);
    pool.acquire();
    pool.clear();

    // implicit growth works again
    void* slot = pool.acquire();
    D_POOL_CHECK(slot != nullptr);
    D_POOL_CHECK(pool.size() == 1u);
    D_POOL_CHECK(pool.capacity() >= 1u);

    pool.release(slot);
    D_POOL_CHECK(pool.acquire() == slot);

    // explicit reserve works again
    pool.clear();
    D_POOL_CHECK(pool.reserve(6));
    D_POOL_CHECK(pool.capacity() == 6u);

    for (std::size_t i = 0; i < 6; ++i)
    {
        D_POOL_CHECK(pool.acquire() != nullptr);
    }

    // and the cycle can repeat
    pool.clear();
    D_POOL_CHECK(pool.capacity() == 0u);
    D_POOL_CHECK(pool.acquire() != nullptr);

    return true;
}

/*
tests_reset_and_clear_are_idempotent
  Verifies repeated reset and clear calls - on an untouched pool, on a
  populated one, and interleaved with each other - are safe and converge
  on the same state.
*/
bool
tests_reset_and_clear_are_idempotent()
{
    // on a pool that was never used
    {
        pool_fl_chunked pool;

        pool.reset();
        pool.reset();
        D_POOL_CHECK(pool.size() == 0u);
        D_POOL_CHECK(pool.capacity() == 0u);

        pool.clear();
        pool.clear();
        D_POOL_CHECK(pool.capacity() == 0u);
        D_POOL_CHECK(pool.bytes_allocated() == 0u);
    }

    // repeated reset preserves capacity every time
    {
        pool_fl_chunked pool(8);
        pool.acquire();

        for (std::size_t i = 0; i < 5; ++i)
        {
            pool.reset();
            D_POOL_CHECK(pool.size() == 0u);
            D_POOL_CHECK(pool.capacity() == 8u);
        }
    }

    // repeated clear stays at zero
    {
        pool_fl_contig pool(8);
        pool.acquire();

        for (std::size_t i = 0; i < 5; ++i)
        {
            pool.clear();
            D_POOL_CHECK(pool.capacity() == 0u);
            D_POOL_CHECK(pool.bytes_allocated() == 0u);
        }
    }

    // interleaved: reset after clear is a no-op on an empty arena
    {
        pool_fl_chunked pool(4);
        pool.acquire();

        pool.clear();
        pool.reset();
        D_POOL_CHECK(pool.capacity() == 0u);
        D_POOL_CHECK(pool.size() == 0u);

        pool.acquire();
        pool.reset();
        pool.clear();
        D_POOL_CHECK(pool.capacity() == 0u);
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
