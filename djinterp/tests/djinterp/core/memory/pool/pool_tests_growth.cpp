/*******************************************************************************
* djinterp [test]                                          pool_tests_growth.cpp
*
*   Section VI part 7 of the pool suite - growth.
*
*   The two block layouts diverge sharply here.  A chunked pool links a new
* block onto the chain and never touches the old ones, so pointers into it
* stay valid; a contiguous pool allocates a wider buffer, copies the used
* prefix into it, rewrites the free list to point into the copy, and frees
* the original - so its pointers do not.  The free-list rebase is checked
* exactly: released slots must reappear at the same slot indices in the new
* allocation, in the same order.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_growth.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <algorithm>
#include <cstring>
#include <vector>
// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // stamp / read_stamp
    //   helpers: write and read a sentinel through raw slot storage
    // without forming a typed reference to memory the pool may have
    // relocated with memcpy.
    inline void
    stamp(
        void*         _slot,
        std::uint64_t _value
    ) noexcept
    {
        std::memcpy(_slot, &_value, sizeof(_value));

        return;
    }

    inline std::uint64_t
    read_stamp(
        const void* _slot
    ) noexcept
    {
        std::uint64_t value = 0;
        std::memcpy(&value, _slot, sizeof(value));

        return value;
    }
}


/*
tests_chunked_growth_keeps_pointers_stable
  Verifies values written through slots held across a growth event survive
  it intact - the chunked layout links new blocks in rather than relocating
  the existing ones, exactly as its pointer_stable constant promises.
*/
bool
tests_chunked_growth_keeps_pointers_stable()
{
    D_POOL_CHECK(pool_fl_chunked::pointer_stable == true);

    pool_fl_chunked pool(4);

    void* held[4] = {};

    for (std::size_t i = 0; i < 4; ++i)
    {
        held[i] = pool.acquire();
        D_POOL_CHECK(held[i] != nullptr);
        stamp(held[i], static_cast<std::uint64_t>(0xA000u + i));
    }

    const std::size_t capacity_before = pool.capacity();

    // force several growth events
    for (std::size_t i = 0; i < 40; ++i)
    {
        D_POOL_CHECK(pool.acquire() != nullptr);
    }

    D_POOL_CHECK(pool.capacity() > capacity_before);

    // the original pointers still address their own slots
    for (std::size_t i = 0; i < 4; ++i)
    {
        D_POOL_CHECK(read_stamp(held[i]) ==
                   static_cast<std::uint64_t>(0xA000u + i));
    }

    // ... and are still writable, and still owned by the pool
    for (std::size_t i = 0; i < 4; ++i)
    {
        stamp(held[i], static_cast<std::uint64_t>(0xB000u + i));
        D_POOL_CHECK(read_stamp(held[i]) ==
                   static_cast<std::uint64_t>(0xB000u + i));
    }

    const std::size_t size_before = pool.size();
    pool.release(held[0]);
    D_POOL_CHECK(pool.size() == (size_before - 1u));
    D_POOL_CHECK(pool.acquire() == held[0]);

    return true;
}

/*
tests_contiguous_growth_relocates_storage
  Verifies growing a contiguous pool moves the backing allocation, as its
  pointer_stable == false constant advertises.  The implementation
  allocates the wider buffer before freeing the old one, so the two cannot
  share an address.
*/
bool
tests_contiguous_growth_relocates_storage()
{
    D_POOL_CHECK(pool_fl_contig::pointer_stable == false);

    pool_fl_contig pool(2);

    void* const first_base = pool.acquire();
    D_POOL_CHECK(first_base != nullptr);
    D_POOL_CHECK(pool.capacity() == 2u);

    // grow past the initial allocation
    pool.acquire();
    pool.acquire();
    D_POOL_CHECK(pool.capacity() > 2u);

    // rewind the bump cursor: the next slot is index 0 of the CURRENT
    // allocation, which is a different buffer from the original
    pool.reset();
    void* const second_base = pool.acquire();

    D_POOL_CHECK(second_base != nullptr);
    D_POOL_CHECK(second_base != first_base);

    // the pool remains fully consistent afterwards
    D_POOL_CHECK(pool.size() == 1u);
    D_POOL_CHECK(pool.bytes_allocated() ==
               (pool.capacity() * pool_fl_contig::bytes_per_slot()));

    return true;
}

/*
tests_contiguous_growth_rebases_the_free_list
  Verifies the free list survives a contiguous reallocation: entries are
  rewritten into the new buffer at their original slot indices, handed back
  in the original LIFO order, and the used prefix is copied across intact.
*/
bool
tests_contiguous_growth_rebases_the_free_list()
{
    constexpr std::size_t slot = pool_fl_contig::bytes_per_slot();

    pool_fl_contig pool(4);

    // fill the initial allocation: slot indices 0, 1, 2, 3
    void* const a = pool.acquire();
    void* const b = pool.acquire();
    void* const c = pool.acquire();
    void* const d = pool.acquire();
    D_POOL_CHECK(pool.size() == 4u);
    D_POOL_CHECK(pool.capacity() == 4u);

    // sentinel in slot 0, so the memcpy of the used prefix is checked too
    stamp(a, 0xFEEDFACEu);
    stamp(d, 0x0D15EA5Eu);

    // release index 1 then index 2, so the free list is c -> b
    pool.release(b);
    pool.release(c);
    D_POOL_CHECK(pool.size() == 2u);

    // reallocate to eight slots: prefix copied, free list rebased
    D_POOL_CHECK(pool.reserve(8));
    D_POOL_CHECK(pool.capacity() == 8u);

    // the free list is drained first, in LIFO order: c then b
    void* const recycled_c = pool.acquire();
    void* const recycled_b = pool.acquire();
    D_POOL_CHECK(recycled_c != nullptr);
    D_POOL_CHECK(recycled_b != nullptr);

    // then the untouched bump region: indices 4, 5, 6, 7
    void* const bump[4] = { pool.acquire(), pool.acquire(),
                            pool.acquire(), pool.acquire() };

    for (std::size_t i = 0; i < 4; ++i)
    {
        D_POOL_CHECK(bump[i] != nullptr);
    }

    D_POOL_CHECK(pool.size() == 8u);

    // index 4 pins the base of the new allocation
    char* const base = static_cast<char*>(bump[0]) - (4u * slot);

    // the bump slots are the consecutive tail of that allocation
    for (std::size_t i = 0; i < 4; ++i)
    {
        D_POOL_CHECK(static_cast<char*>(bump[i]) ==
                   (base + ((4u + i) * slot)));
    }

    // the rebased entries landed at their ORIGINAL indices - 2 and 1 -
    // which is the whole point of the rebase pass
    D_POOL_CHECK(static_cast<char*>(recycled_c) == (base + (2u * slot)));
    D_POOL_CHECK(static_cast<char*>(recycled_b) == (base + (1u * slot)));

    // every live slot is distinct and inside the new allocation
    void* const live[6] = { recycled_c, recycled_b,
                            bump[0], bump[1], bump[2], bump[3] };
    D_POOL_CHECK(pool_all_distinct(live, 6));

    for (std::size_t i = 0; i < 6; ++i)
    {
        char* const address = static_cast<char*>(live[i]);
        D_POOL_CHECK(address >= base);
        D_POOL_CHECK(address < (base + (8u * slot)));
    }

    // the used prefix was copied, not merely re-pointed
    D_POOL_CHECK(read_stamp(base + (0u * slot)) == 0xFEEDFACEu);
    D_POOL_CHECK(read_stamp(base + (3u * slot)) == 0x0D15EA5Eu);

    // the old pointers must NOT be dereferenced, but the pool is still
    // internally consistent
    (void)a;
    (void)d;
    D_POOL_CHECK(pool.capacity() == 8u);
    D_POOL_CHECK(pool.bytes_allocated() == (8u * slot));

    return true;
}

/*
tests_chunked_blocks_respect_slots_per_block
  Verifies a bulk reserve is split into blocks of at most slots_per_block
  slots, with the remainder in a final short block - read back through the
  block-header overhead in bytes_allocated.
*/
bool
tests_chunked_blocks_respect_slots_per_block()
{
    constexpr std::size_t slot   = pool_fl_chunked::bytes_per_slot();
    constexpr std::size_t align  = pool_fl_chunked::alignment();
    const     std::size_t header = pool_expected_block_header(align);

    D_POOL_CHECK(chunked_block_policy<4>::slots_per_block == 4);

    // capacity -> expected block count, at four slots per block
    const std::size_t capacities[6] = { 1, 4, 5, 8, 10, 16 };
    const std::size_t blocks[6]     = { 1, 1, 2, 2, 3,  4 };

    for (std::size_t i = 0; i < 6; ++i)
    {
        const pool_fl_chunked pool(capacities[i]);

        D_POOL_CHECK(pool.capacity() == capacities[i]);
        D_POOL_CHECK(pool.bytes_allocated() ==
                   ((capacities[i] * slot) + (blocks[i] * header)));
    }

    // a larger block policy needs fewer blocks for the same capacity
    {
        using wide_blocks =
            pool_resource<pool_medium_t,
                          free_list_release_policy,
                          chunked_block_policy<16>,
                          pool_growth_exact>;

        const wide_blocks pool(16);

        D_POOL_CHECK(pool.capacity() == 16u);
        D_POOL_CHECK(pool.bytes_allocated() == ((16u * slot) + header));
    }

    // every slot across every block is reachable
    {
        pool_fl_chunked pool(10);
        void*           slots[10] = {};

        for (std::size_t i = 0; i < 10; ++i)
        {
            slots[i] = pool.acquire();
            D_POOL_CHECK(slots[i] != nullptr);
        }

        D_POOL_CHECK(pool_all_distinct(slots, 10));
        D_POOL_CHECK(pool.capacity() == 10u);
    }

    return true;
}

/*
tests_chunked_growth_caps_a_block_at_the_request
  Verifies a growth request smaller than slots_per_block produces a
  correspondingly small block rather than a full-sized one: blocks are
  sized min(slots_per_block, requested).
*/
bool
tests_chunked_growth_caps_a_block_at_the_request()
{
    constexpr std::size_t slot   = pool_fl_chunked::bytes_per_slot();
    constexpr std::size_t align  = pool_fl_chunked::alignment();
    const     std::size_t header = pool_expected_block_header(align);

    // the exact growth policy asks for one slot at a time, so the first
    // acquire on an empty pool creates a ONE-slot block even though the
    // policy allows four
    pool_fl_chunked pool;

    D_POOL_CHECK(pool.acquire() != nullptr);
    D_POOL_CHECK(pool.capacity() == 1u);
    D_POOL_CHECK(pool.bytes_allocated() == ((1u * slot) + header));

    // each subsequent growth adds another one-slot block, so the header
    // overhead grows in step with capacity
    D_POOL_CHECK(pool.acquire() != nullptr);
    D_POOL_CHECK(pool.capacity() == 2u);
    D_POOL_CHECK(pool.bytes_allocated() == ((2u * slot) + (2u * header)));

    D_POOL_CHECK(pool.acquire() != nullptr);
    D_POOL_CHECK(pool.capacity() == 3u);
    D_POOL_CHECK(pool.bytes_allocated() == ((3u * slot) + (3u * header)));

    // a bulk reserve, by contrast, packs full blocks
    {
        pool_fl_chunked bulk;

        D_POOL_CHECK(bulk.reserve(4));
        D_POOL_CHECK(bulk.bytes_allocated() == ((4u * slot) + header));
    }

    return true;
}

/*
tests_growth_policy_drives_new_capacity
  Verifies post-growth capacity follows the growth policy's compute(),
  not any rule internal to the pool - the same pool shape under three
  policies produces three different capacity sequences.
*/
bool
tests_growth_policy_drives_new_capacity()
{
    // exact: compute(current, current + 1) == current + 1, so one slot
    // per growth
    {
        pool_fl_chunked pool;

        for (std::size_t i = 1; i <= 8; ++i)
        {
            D_POOL_CHECK(pool.acquire() != nullptr);
            D_POOL_CHECK(pool.capacity() == i);
        }
    }

    // step of four: capacity climbs 4, 8, 12, ...
    {
        pool_fl_chunked_step pool;

        D_POOL_CHECK(pool.acquire() != nullptr);
        D_POOL_CHECK(pool.capacity() == 4u);

        // the next three come out of the existing bump region
        for (std::size_t i = 0; i < 3; ++i)
        {
            D_POOL_CHECK(pool.acquire() != nullptr);
            D_POOL_CHECK(pool.capacity() == 4u);
        }

        // the fifth forces the next step
        D_POOL_CHECK(pool.acquire() != nullptr);
        D_POOL_CHECK(pool.capacity() == 8u);

        for (std::size_t i = 0; i < 3; ++i)
        {
            pool.acquire();
        }

        D_POOL_CHECK(pool.capacity() == 8u);
        D_POOL_CHECK(pool.acquire() != nullptr);
        D_POOL_CHECK(pool.capacity() == 12u);
    }

    // doubling: 1, 2, 4, 8, 16 ...
    {
        pool_fl_chunked_double pool;

        const std::size_t expected[5] = { 1, 2, 4, 8, 16 };
        std::size_t       index       = 0;
        std::size_t       previous    = 0;

        for (std::size_t i = 0; i < 16; ++i)
        {
            pool.acquire();

            if ( (pool.capacity() != previous) && (index < 5) )
            {
                D_POOL_CHECK(pool.capacity() == expected[index]);
                previous = pool.capacity();
                ++index;
            }
        }

        D_POOL_CHECK(index == 5u);
    }

    return true;
}

/*
tests_refused_growth_bounds_the_pool
  Verifies a can_grow == false policy caps the pool at whatever was
  explicitly reserved, no matter how hard acquire is pushed, and that the
  footprint never changes as a result of failed acquires.
*/
bool
tests_refused_growth_bounds_the_pool()
{
    pool_fl_chunked_fixed pool(4);

    const std::size_t capacity_before = pool.capacity();
    const std::size_t bytes_before    = pool.bytes_allocated();

    for (std::size_t i = 0; i < 4; ++i)
    {
        D_POOL_CHECK(pool.acquire() != nullptr);
    }

    // a hundred refused acquires must change nothing at all
    for (std::size_t i = 0; i < 100; ++i)
    {
        D_POOL_CHECK(pool.acquire() == nullptr);
    }

    D_POOL_CHECK(pool.capacity()        == capacity_before);
    D_POOL_CHECK(pool.bytes_allocated() == bytes_before);
    D_POOL_CHECK(pool.size()            == 4u);

    // reset makes the same bounded capacity available again
    pool.reset();
    D_POOL_CHECK(pool.capacity() == capacity_before);

    for (std::size_t i = 0; i < 4; ++i)
    {
        D_POOL_CHECK(pool.acquire() != nullptr);
    }

    D_POOL_CHECK(pool.acquire() == nullptr);

    // contiguous is bounded identically
    {
        pool_fl_contig_fixed contiguous(3);

        for (std::size_t i = 0; i < 3; ++i)
        {
            D_POOL_CHECK(contiguous.acquire() != nullptr);
        }

        D_POOL_CHECK(contiguous.acquire() == nullptr);
        D_POOL_CHECK(contiguous.capacity() == 3u);
    }

    return true;
}

/*
tests_growth_from_zero_capacity
  Verifies the first acquire on a pool that owns nothing provisions
  storage through the grow-and-retry path and returns a usable slot, for
  both layouts and every release policy.
*/
bool
tests_growth_from_zero_capacity()
{
    {
        pool_fl_chunked pool;
        D_POOL_CHECK(pool.capacity() == 0u);
        D_POOL_CHECK(pool.bytes_allocated() == 0u);

        void* slot = pool.acquire();
        D_POOL_CHECK(slot != nullptr);
        D_POOL_CHECK(pool.capacity() >= 1u);
        D_POOL_CHECK(pool.size() == 1u);
        D_POOL_CHECK(pool.bytes_allocated() > 0u);

        stamp(slot, 0x1234u);
        D_POOL_CHECK(read_stamp(slot) == 0x1234u);
    }
    {
        pool_fl_contig pool;
        D_POOL_CHECK(pool.acquire() != nullptr);
        D_POOL_CHECK(pool.capacity() >= 1u);
    }
    {
        pool_mono_contig pool;
        D_POOL_CHECK(pool.acquire() != nullptr);
        D_POOL_CHECK(pool.capacity() >= 1u);
    }
    {
        pool_gen_chunked pool;
        D_POOL_CHECK(pool.acquire() != nullptr);
        D_POOL_CHECK(pool.capacity() >= 1u);
    }

    // and after a clear, which returns the pool to zero capacity
    {
        pool_fl_chunked pool(8);
        pool.acquire();
        pool.clear();

        D_POOL_CHECK(pool.capacity() == 0u);
        D_POOL_CHECK(pool.acquire() != nullptr);
    }

    return true;
}

/*
tests_sustained_growth_yields_unique_slots
  Verifies several thousand acquires across many growth events yield
  unique, correctly aligned, non-overlapping slots, and that the reported
  capacity never falls behind the number of live slots.
*/
bool
tests_sustained_growth_yields_unique_slots()
{
    constexpr std::size_t count = 5000;

    // chunked, doubling growth: pointer-stable across every event
    {
        pool_fl_chunked_double pool;
        std::vector<void*>     slots;
        slots.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            void* slot = pool.acquire();
            D_POOL_CHECK(slot != nullptr);
            D_POOL_CHECK(pool_is_aligned(slot,
                                       pool_fl_chunked_double::alignment()));

            slots.push_back(slot);

            D_POOL_CHECK(pool.capacity() >= pool.size());
        }

        D_POOL_CHECK(pool.size() == count);

        std::sort(slots.begin(), slots.end());
        D_POOL_CHECK(std::adjacent_find(slots.begin(), slots.end()) ==
                   slots.end());

        // returning them all leaves the capacity untouched
        const std::size_t capacity_before = pool.capacity();

        for (std::size_t i = 0; i < count; ++i)
        {
            pool.release(slots[i]);
        }

        D_POOL_CHECK(pool.size() == 0u);
        D_POOL_CHECK(pool.capacity() == capacity_before);

        // and every one of them can be handed out again
        for (std::size_t i = 0; i < count; ++i)
        {
            D_POOL_CHECK(pool.acquire() != nullptr);
        }

        D_POOL_CHECK(pool.capacity() == capacity_before);
    }

    // contiguous, doubling growth: relocation on every event, so only the
    // most recent generation of pointers is examined
    {
        using contig_double =
            pool_resource<pool_medium_t,
                          free_list_release_policy,
                          contiguous_block_policy,
                          pool_growth_double>;

        contig_double pool;

        for (std::size_t i = 0; i < count; ++i)
        {
            void* slot = pool.acquire();
            D_POOL_CHECK(slot != nullptr);
            D_POOL_CHECK(pool_is_aligned(slot, contig_double::alignment()));
        }

        D_POOL_CHECK(pool.size() == count);
        D_POOL_CHECK(pool.capacity() >= count);
        D_POOL_CHECK(pool.bytes_allocated() ==
                   (pool.capacity() * contig_double::bytes_per_slot()));
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
