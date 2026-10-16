/******************************************************************************
* djinterp [test]                                     pool_tests_accounting.cpp
*
*   Section VI part 6 of the pool suite - memory accounting.
*
*   bytes_allocated() reports the whole backing footprint, which for the
* chunked layout includes one block header per block and for the contiguous
* layout includes nothing beyond the slots.  bytes_in_use() is the live
* count times the slot size, and utilization() is the ratio of the two
* counters, defined as 0.0 for a pool with no capacity.
*
*   Block-header overhead is predicted from pool_block_probe, a local mirror
* of the module's internal block header, so the expectations are exact
* rather than merely "greater than".
*
*
* path:      /tests/djinterp/core/memory/pool_tests_accounting.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // near
    //   helper: tolerant comparison for the utilization ratio.
    inline bool
    near(
        double _lhs,
        double _rhs
    ) noexcept
    {
        double difference = (_lhs - _rhs);

        if (difference < 0.0)
        {
            difference = -difference;
        }

        return (difference < 1e-12);
    }

    // chunked_expected_bytes
    //   helper: the exact footprint of a chunked pool holding _capacity
    // slots spread over _blocks blocks.
    inline std::size_t
    chunked_expected_bytes(
        std::size_t _capacity,
        std::size_t _blocks,
        std::size_t _slot_size,
        std::size_t _slot_align
    ) noexcept
    {
        return ( (_capacity * _slot_size) +
                 (_blocks * pool_expected_block_header(_slot_align)) );
    }
}


/*
tests_accounting_on_an_empty_pool
  Verifies a pool with no storage reports zero allocated bytes, zero bytes
  in use, and a utilization of exactly 0.0 - the guarded divide-by-zero
  branch.
*/
bool
tests_accounting_on_an_empty_pool()
{
    {
        const pool_fl_chunked pool;

        D_PR_CHECK(pool.bytes_allocated() == 0u);
        D_PR_CHECK(pool.bytes_in_use()    == 0u);
        D_PR_CHECK(near(pool.utilization(), 0.0));
    }
    {
        const pool_fl_contig pool;

        D_PR_CHECK(pool.bytes_allocated() == 0u);
        D_PR_CHECK(pool.bytes_in_use()    == 0u);
        D_PR_CHECK(near(pool.utilization(), 0.0));
    }

    // capacity without any acquisitions: allocated rises, in-use does not
    {
        const pool_fl_chunked pool(8);

        D_PR_CHECK(pool.bytes_allocated() > 0u);
        D_PR_CHECK(pool.bytes_in_use()    == 0u);
        D_PR_CHECK(near(pool.utilization(), 0.0));
    }

    return true;
}

/*
tests_contiguous_accounting_has_no_block_overhead
  Verifies a contiguous pool's bytes_allocated is exactly capacity times
  the slot size - a single allocation carries no per-block header.
*/
bool
tests_contiguous_accounting_has_no_block_overhead()
{
    constexpr std::size_t slot = pool_fl_contig::bytes_per_slot();

    {
        const pool_fl_contig pool(8);

        D_PR_CHECK(pool.capacity() == 8u);
        D_PR_CHECK(pool.bytes_allocated() == (8u * slot));
    }

    // the identity holds at every capacity, including one slot
    for (std::size_t capacity = 1; capacity <= 16; ++capacity)
    {
        const pool_fl_contig pool(capacity);

        D_PR_CHECK(pool.capacity() == capacity);
        D_PR_CHECK(pool.bytes_allocated() == (capacity * slot));
    }

    // and after growth, against the grown capacity
    {
        pool_fl_contig pool(4);

        for (std::size_t i = 0; i < 10; ++i)
        {
            pool.acquire();
        }

        D_PR_CHECK(pool.bytes_allocated() == (pool.capacity() * slot));
    }

    return true;
}

/*
tests_chunked_accounting_includes_block_headers
  Verifies a chunked pool's bytes_allocated is the slot bytes plus exactly
  one block header per block, and that the block count follows from
  slots_per_block.
*/
bool
tests_chunked_accounting_includes_block_headers()
{
    constexpr std::size_t slot  = pool_fl_chunked::bytes_per_slot();
    constexpr std::size_t align = pool_fl_chunked::alignment();

    // eight slots at four per block is exactly two blocks
    {
        const pool_fl_chunked pool(8);

        D_PR_CHECK(pool.capacity() == 8u);
        D_PR_CHECK(pool.bytes_allocated() ==
                   chunked_expected_bytes(8, 2, slot, align));

        // and that is strictly more than the slots alone
        D_PR_CHECK(pool.bytes_allocated() > (8u * slot));
    }

    // four slots is one block
    {
        const pool_fl_chunked pool(4);

        D_PR_CHECK(pool.bytes_allocated() ==
                   chunked_expected_bytes(4, 1, slot, align));
    }

    // ten slots is three blocks: four, four, and a remainder of two
    {
        const pool_fl_chunked pool(10);

        D_PR_CHECK(pool.capacity() == 10u);
        D_PR_CHECK(pool.bytes_allocated() ==
                   chunked_expected_bytes(10, 3, slot, align));
    }

    // a single-slot block still carries a full header
    {
        const pool_fl_chunked pool(1);

        D_PR_CHECK(pool.bytes_allocated() ==
                   chunked_expected_bytes(1, 1, slot, align));
    }

    // the overhead scales with blocks, not with capacity
    {
        const pool_fl_chunked small(4);
        const pool_fl_chunked large(8);

        const std::size_t small_overhead =
            small.bytes_allocated() - (small.capacity() * slot);
        const std::size_t large_overhead =
            large.bytes_allocated() - (large.capacity() * slot);

        D_PR_CHECK(large_overhead == (2u * small_overhead));
    }

    return true;
}

/*
tests_bytes_in_use_tracks_live_slots
  Verifies bytes_in_use is always the live count times the slot size, for
  every release policy - including monotonic, where the storage is not
  reclaimed but the live count still falls.
*/
bool
tests_bytes_in_use_tracks_live_slots()
{
    constexpr std::size_t slot = pool_fl_chunked::bytes_per_slot();

    {
        pool_fl_chunked pool(8);

        D_PR_CHECK(pool.bytes_in_use() == 0u);

        void* a = pool.acquire();
        D_PR_CHECK(pool.bytes_in_use() == (1u * slot));

        void* b = pool.acquire();
        D_PR_CHECK(pool.bytes_in_use() == (2u * slot));

        pool.acquire();
        D_PR_CHECK(pool.bytes_in_use() == (3u * slot));
        D_PR_CHECK(pool.bytes_in_use() == (pool.size() * slot));

        pool.release(a);
        D_PR_CHECK(pool.bytes_in_use() == (2u * slot));

        pool.release(b);
        D_PR_CHECK(pool.bytes_in_use() == (1u * slot));
    }

    // monotonic: bytes_in_use follows the live count, not the storage
    // actually consumed
    {
        pool_mono_chunked pool(4);

        void* a = pool.acquire();
        void* b = pool.acquire();
        D_PR_CHECK(pool.bytes_in_use() == (2u * slot));

        pool.release(a);
        pool.release(b);
        D_PR_CHECK(pool.bytes_in_use() == 0u);

        // even though the arena is still fully consumed
        D_PR_CHECK(pool.bytes_allocated() > 0u);
    }

    // generational
    {
        pool_gen_contig pool(4);

        pool.acquire();
        pool.acquire();
        D_PR_CHECK(pool.bytes_in_use() == (2u * slot));
    }

    // bytes_in_use never exceeds bytes_allocated
    {
        pool_fl_chunked pool;

        for (std::size_t i = 0; i < 30; ++i)
        {
            pool.acquire();
            D_PR_CHECK(pool.bytes_in_use() <= pool.bytes_allocated());
        }
    }

    return true;
}

/*
tests_utilization_reports_size_over_capacity
  Verifies utilization is exactly the live count divided by capacity,
  including the partial, empty and completely full cases.
*/
bool
tests_utilization_reports_size_over_capacity()
{
    {
        pool_fl_chunked pool(8);

        D_PR_CHECK(near(pool.utilization(), 0.0));

        pool.acquire();
        pool.acquire();
        D_PR_CHECK(near(pool.utilization(), 2.0 / 8.0));

        pool.acquire();
        pool.acquire();
        D_PR_CHECK(near(pool.utilization(), 4.0 / 8.0));
        D_PR_CHECK(near(pool.utilization(), 0.5));
    }

    // a completely full pool reports exactly 1.0
    {
        pool_fl_chunked pool(4);

        for (std::size_t i = 0; i < 4; ++i)
        {
            pool.acquire();
        }

        D_PR_CHECK(pool.size() == pool.capacity());
        D_PR_CHECK(near(pool.utilization(), 1.0));
    }

    // it tracks the ratio back down as slots are released
    {
        pool_fl_contig pool(4);

        void* a = pool.acquire();
        void* b = pool.acquire();
        void* c = pool.acquire();
        void* d = pool.acquire();
        D_PR_CHECK(near(pool.utilization(), 1.0));

        pool.release(a);
        D_PR_CHECK(near(pool.utilization(), 0.75));
        pool.release(b);
        D_PR_CHECK(near(pool.utilization(), 0.5));
        pool.release(c);
        D_PR_CHECK(near(pool.utilization(), 0.25));
        pool.release(d);
        D_PR_CHECK(near(pool.utilization(), 0.0));
    }

    // the general identity, checked against the counters themselves
    {
        pool_fl_chunked pool(16);

        for (std::size_t i = 0; i < 16; ++i)
        {
            pool.acquire();

            const double expected =
                ( static_cast<double>(pool.size()) /
                  static_cast<double>(pool.capacity()) );

            D_PR_CHECK(near(pool.utilization(), expected));
        }
    }

    return true;
}

/*
tests_utilization_stays_within_bounds
  Verifies utilization never leaves [0.0, 1.0] and never falls during a
  fill cycle with no intervening releases, including across growth events
  where capacity jumps ahead of size.
*/
bool
tests_utilization_stays_within_bounds()
{
    // monotonically non-decreasing within a fixed capacity
    {
        pool_fl_chunked pool(16);

        double previous = pool.utilization();

        for (std::size_t i = 0; i < 16; ++i)
        {
            pool.acquire();

            const double current = pool.utilization();
            D_PR_CHECK(current >= previous);
            D_PR_CHECK(current >= 0.0);
            D_PR_CHECK(current <= 1.0);

            previous = current;
        }

        D_PR_CHECK(near(previous, 1.0));
    }

    // across growth the ratio may dip, but never out of bounds
    {
        pool_fl_chunked_double pool;

        for (std::size_t i = 0; i < 100; ++i)
        {
            pool.acquire();

            const double current = pool.utilization();
            D_PR_CHECK(current >= 0.0);
            D_PR_CHECK(current <= 1.0);
        }
    }

    // a monotonic pool's ratio falls on release, still within bounds
    {
        pool_mono_chunked pool(8);

        void* held[8] = {};

        for (std::size_t i = 0; i < 8; ++i)
        {
            held[i] = pool.acquire();
        }

        for (std::size_t i = 0; i < 8; ++i)
        {
            pool.release(held[i]);

            const double current = pool.utilization();
            D_PR_CHECK(current >= 0.0);
            D_PR_CHECK(current <= 1.0);
        }

        D_PR_CHECK(near(pool.utilization(), 0.0));
    }

    return true;
}

/*
tests_accounting_after_reset_and_clear
  Verifies reset preserves the allocated footprint while zeroing bytes in
  use and utilization, and clear zeroes all three.
*/
bool
tests_accounting_after_reset_and_clear()
{
    // reset
    {
        pool_fl_chunked pool(8);

        pool.acquire();
        pool.acquire();
        pool.acquire();

        const std::size_t allocated_before = pool.bytes_allocated();
        D_PR_CHECK(allocated_before > 0u);
        D_PR_CHECK(pool.bytes_in_use() > 0u);

        pool.reset();

        D_PR_CHECK(pool.bytes_allocated() == allocated_before);
        D_PR_CHECK(pool.bytes_in_use() == 0u);
        D_PR_CHECK(near(pool.utilization(), 0.0));
    }

    // clear
    {
        pool_fl_chunked pool(8);

        pool.acquire();
        pool.acquire();

        pool.clear();

        D_PR_CHECK(pool.bytes_allocated() == 0u);
        D_PR_CHECK(pool.bytes_in_use() == 0u);
        D_PR_CHECK(near(pool.utilization(), 0.0));
    }

    // contiguous, both operations
    {
        pool_fl_contig pool(8);
        pool.acquire();

        const std::size_t allocated_before = pool.bytes_allocated();
        pool.reset();
        D_PR_CHECK(pool.bytes_allocated() == allocated_before);
        D_PR_CHECK(pool.bytes_in_use() == 0u);

        pool.clear();
        D_PR_CHECK(pool.bytes_allocated() == 0u);
    }

    // allocated bytes never fall while the pool is only growing
    {
        pool_fl_chunked pool;

        std::size_t previous = pool.bytes_allocated();

        for (std::size_t i = 0; i < 40; ++i)
        {
            pool.acquire();

            const std::size_t current = pool.bytes_allocated();
            D_PR_CHECK(current >= previous);

            previous = current;
        }
    }

    return true;
}

/*
tests_accounting_queries_are_const_and_noexcept
  Verifies all three accounting queries are callable on a const pool, are
  noexcept, and return the documented types.
*/
bool
tests_accounting_queries_are_const_and_noexcept()
{
    using pool_type = pool_fl_chunked;
    using size_type = pool_type::size_type;

    const pool_type pool(4);

    D_PR_CHECK(noexcept(pool.bytes_allocated()));
    D_PR_CHECK(noexcept(pool.bytes_in_use()));
    D_PR_CHECK(noexcept(pool.utilization()));

    D_PR_CHECK((std::is_same<decltype(pool.bytes_allocated()),
                             size_type>::value));
    D_PR_CHECK((std::is_same<decltype(pool.bytes_in_use()),
                             size_type>::value));
    D_PR_CHECK((std::is_same<decltype(pool.utilization()),
                             double>::value));

    // the module's own detector agrees the full triple is present
    D_PR_CHECK(has_bytes_allocated_v<pool_type>);
    D_PR_CHECK(has_bytes_in_use_v<pool_type>);
    D_PR_CHECK(has_utilization_v<pool_type>);
    D_PR_CHECK(has_memory_accounting_v<pool_type>);

    return true;
}


NS_END  // testing
NS_END  // djinterp
