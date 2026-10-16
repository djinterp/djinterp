/******************************************************************************
* djinterp [test]                             pool_allocator_tests_allocate.cpp
*
*   Section III of the pool allocator suite - the two allocation paths.
*
*   allocate(1) draws a slot from the bound pool; every other request -
* including n == 0, n > 1, and any request made through a handle with no
* pool - goes to global operator new instead.  deallocate mirrors the same
* fork, so a request and its release must agree on which path they took.
* Almost every check here reads the pool's live count before and after a
* call, because that counter is the only externally visible evidence of
* which branch ran.
*
*
* path:      /tests/djinterp/core/memory/pool_allocator_tests_allocate.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

// std
#include <new>
// djinterp
#include "pool_allocator_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_single_allocation_comes_from_the_pool
  Verifies allocate(1) draws a slot from the bound pool, moving its live
  count, and hands back storage inside the pool's own arena.
*/
bool
tests_single_allocation_comes_from_the_pool()
{
    pa_pool  pool(8);
    pa_alloc allocator(pool);

    D_PA_CHECK(pool.size() == 0u);

    pa_element* first = allocator.allocate(1);
    D_PA_CHECK(first != nullptr);
    D_PA_CHECK(pool.size() == 1u);

    pa_element* second = allocator.allocate(1);
    D_PA_CHECK(second != nullptr);
    D_PA_CHECK(second != first);
    D_PA_CHECK(pool.size() == 2u);

    // the storage is writable and independent
    first->m_lo  = 11u;
    first->m_hi  = 22u;
    second->m_lo = 33u;
    second->m_hi = 44u;

    D_PA_CHECK(first->m_lo  == 11u);
    D_PA_CHECK(first->m_hi  == 22u);
    D_PA_CHECK(second->m_lo == 33u);
    D_PA_CHECK(second->m_hi == 44u);

    // the allocation consumed reserved capacity rather than growing
    D_PA_CHECK(pool.capacity() == 8u);
    D_PA_CHECK(pool.bytes_in_use() == (2u * pa_pool::bytes_per_slot()));

    allocator.deallocate(first, 1);
    allocator.deallocate(second, 1);
    D_PA_CHECK(pool.size() == 0u);

    // allocate returns the allocator's pointer type
    D_PA_CHECK((std::is_same<decltype(allocator.allocate(1)),
                             pa_alloc::pointer>::value));

    return true;
}

/*
tests_single_deallocation_returns_to_the_pool
  Verifies deallocate(p, 1) returns the slot to the pool for reuse rather
  than to the global allocator - the recycled slot comes straight back on
  the next request.
*/
bool
tests_single_deallocation_returns_to_the_pool()
{
    pa_pool  pool(8);
    pa_alloc allocator(pool);

    pa_element* slot = allocator.allocate(1);
    D_PA_CHECK(pool.size() == 1u);

    allocator.deallocate(slot, 1);
    D_PA_CHECK(pool.size() == 0u);

    // the free list handed it straight back
    pa_element* again = allocator.allocate(1);
    D_PA_CHECK(again == slot);
    D_PA_CHECK(pool.size() == 1u);
    D_PA_CHECK(pool.capacity() == 8u);

    allocator.deallocate(again, 1);

    // LIFO order is preserved through the allocator, as through the pool
    pa_element* a = allocator.allocate(1);
    pa_element* b = allocator.allocate(1);
    pa_element* c = allocator.allocate(1);

    allocator.deallocate(a, 1);
    allocator.deallocate(b, 1);
    allocator.deallocate(c, 1);

    D_PA_CHECK(allocator.allocate(1) == c);
    D_PA_CHECK(allocator.allocate(1) == b);
    D_PA_CHECK(allocator.allocate(1) == a);

    // deallocate is noexcept and returns nothing
    D_PA_CHECK(noexcept(allocator.deallocate(a, 1)));
    D_PA_CHECK((std::is_same<decltype(allocator.deallocate(a, 1)),
                             void>::value));

    return true;
}

/*
tests_bulk_allocation_bypasses_the_pool
  Verifies allocate(n) for n greater than one leaves the pool completely
  untouched and yields usable, correctly sized storage from the global
  allocator instead.
*/
bool
tests_bulk_allocation_bypasses_the_pool()
{
    pa_pool  pool(8);
    pa_alloc allocator(pool);

    const std::size_t capacity_before = pool.capacity();

    pa_element* block = allocator.allocate(4);
    D_PA_CHECK(block != nullptr);

    // the pool is entirely unaware of it
    D_PA_CHECK(pool.size() == 0u);
    D_PA_CHECK(pool.capacity() == capacity_before);
    D_PA_CHECK(pool.bytes_in_use() == 0u);

    // the storage really is four elements wide
    for (std::size_t i = 0; i < 4; ++i)
    {
        block[i].m_lo = static_cast<std::uint64_t>(i);
        block[i].m_hi = static_cast<std::uint64_t>(i) * 10u;
    }

    for (std::size_t i = 0; i < 4; ++i)
    {
        D_PA_CHECK(block[i].m_lo == static_cast<std::uint64_t>(i));
        D_PA_CHECK(block[i].m_hi == (static_cast<std::uint64_t>(i) * 10u));
    }

    allocator.deallocate(block, 4);
    D_PA_CHECK(pool.size() == 0u);
    D_PA_CHECK(pool.capacity() == capacity_before);

    // a larger request behaves the same way
    pa_element* wide = allocator.allocate(64);
    D_PA_CHECK(wide != nullptr);
    D_PA_CHECK(pool.size() == 0u);
    wide[63].m_lo = 7u;
    D_PA_CHECK(wide[63].m_lo == 7u);
    allocator.deallocate(wide, 64);

    // the two paths interleave without interfering
    pa_element* pooled = allocator.allocate(1);
    pa_element* bulk   = allocator.allocate(2);
    D_PA_CHECK(pool.size() == 1u);
    allocator.deallocate(bulk, 2);
    D_PA_CHECK(pool.size() == 1u);
    allocator.deallocate(pooled, 1);
    D_PA_CHECK(pool.size() == 0u);

    return true;
}

/*
tests_zero_length_allocation
  Verifies allocate(0) takes the bypass path - it is not a single-slot
  request - and that whatever it returns round-trips safely through
  deallocate(p, 0).
*/
bool
tests_zero_length_allocation()
{
    pa_pool  pool(4);
    pa_alloc allocator(pool);

    pa_element* empty = allocator.allocate(0);

    // the pool is untouched either way
    D_PA_CHECK(pool.size() == 0u);
    D_PA_CHECK(pool.capacity() == 4u);

    allocator.deallocate(empty, 0);
    D_PA_CHECK(pool.size() == 0u);

    // the pool is still fully usable afterwards
    pa_element* slot = allocator.allocate(1);
    D_PA_CHECK(slot != nullptr);
    D_PA_CHECK(pool.size() == 1u);
    allocator.deallocate(slot, 1);
    D_PA_CHECK(pool.size() == 0u);

    return true;
}

/*
tests_deallocate_ignores_a_null_pointer
  Verifies deallocate(nullptr, n) takes the guard branch and returns
  without touching the pool - for the single-slot length as well as the
  bulk one, since a stray pool release would corrupt the free list.
*/
bool
tests_deallocate_ignores_a_null_pointer()
{
    pa_pool  pool(4);
    pa_alloc allocator(pool);

    pa_element* live = allocator.allocate(1);
    D_PA_CHECK(pool.size() == 1u);

    allocator.deallocate(nullptr, 1);
    D_PA_CHECK(pool.size() == 1u);

    allocator.deallocate(nullptr, 0);
    allocator.deallocate(nullptr, 8);
    D_PA_CHECK(pool.size() == 1u);
    D_PA_CHECK(pool.capacity() == 4u);

    // the free list is still coherent
    allocator.deallocate(live, 1);
    D_PA_CHECK(pool.size() == 0u);
    D_PA_CHECK(allocator.allocate(1) == live);

    // and on an unbound handle
    pa_other_alloc unbound(allocator);
    D_PA_CHECK(unbound.resource() == nullptr);
    unbound.deallocate(nullptr, 1);

    return true;
}

/*
tests_allocation_without_a_pool_falls_back
  Verifies an allocator holding no pool serves single-slot requests from
  the global allocator, and that deallocate takes the matching path - the
  two must agree or the release is mismatched.
*/
bool
tests_allocation_without_a_pool_falls_back()
{
    pa_pool        pool(8);
    const pa_alloc bound(pool);

    // a rebound handle carries no pool
    pa_other_alloc unbound(bound);
    D_PA_CHECK(unbound.resource() == nullptr);

    pa_other* single = unbound.allocate(1);
    D_PA_CHECK(single != nullptr);
    D_PA_CHECK(pool.size() == 0u);

    single->m_data[0] = 5u;
    single->m_data[5] = 9u;
    D_PA_CHECK(single->m_data[0] == 5u);
    D_PA_CHECK(single->m_data[5] == 9u);

    unbound.deallocate(single, 1);

    // bulk requests behave identically
    pa_other* block = unbound.allocate(3);
    D_PA_CHECK(block != nullptr);
    block[2].m_data[0] = 1u;
    D_PA_CHECK(block[2].m_data[0] == 1u);
    unbound.deallocate(block, 3);

    // repeated round trips through the fallback are stable
    for (std::size_t i = 0; i < 100; ++i)
    {
        pa_other* item = unbound.allocate(1);
        D_PA_CHECK(item != nullptr);
        unbound.deallocate(item, 1);
    }

    D_PA_CHECK(pool.size() == 0u);

    return true;
}

/*
tests_allocation_throws_when_the_pool_is_exhausted
  Verifies a pool that cannot grow makes allocate(1) throw std::bad_alloc
  rather than return null, as the Allocator requirements demand, and that
  the allocator recovers once a slot is released.
*/
bool
tests_allocation_throws_when_the_pool_is_exhausted()
{
    pa_fixed_pool  pool(2);
    pa_fixed_alloc allocator(pool);

    D_PA_CHECK(pool.capacity() == 2u);

    pa_element* first  = allocator.allocate(1);
    pa_element* second = allocator.allocate(1);
    D_PA_CHECK(first  != nullptr);
    D_PA_CHECK(second != nullptr);
    D_PA_CHECK(pool.size() == 2u);

    // the pool itself reports exhaustion by returning null
    D_PA_CHECK(pool.acquire() == nullptr);

    // ... which the allocator must translate into an exception
    bool threw = false;

    try
    {
        pa_element* overflow = allocator.allocate(1);
        (void)overflow;
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }

    D_PA_CHECK(threw);

    // the failed request changed nothing
    D_PA_CHECK(pool.size() == 2u);
    D_PA_CHECK(pool.capacity() == 2u);

    // releasing a slot makes exactly one more request succeed
    allocator.deallocate(first, 1);
    D_PA_CHECK(pool.size() == 1u);

    pa_element* recovered = allocator.allocate(1);
    D_PA_CHECK(recovered == first);

    threw = false;

    try
    {
        pa_element* overflow = allocator.allocate(1);
        (void)overflow;
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }

    D_PA_CHECK(threw);

    // the bulk path is unaffected by the pool's exhaustion
    pa_element* bulk = allocator.allocate(4);
    D_PA_CHECK(bulk != nullptr);
    allocator.deallocate(bulk, 4);

    allocator.deallocate(second, 1);
    allocator.deallocate(recovered, 1);

    return true;
}

/*
tests_allocated_storage_is_suitably_aligned
  Verifies storage returned for an over-aligned element satisfies that
  alignment on BOTH paths - the pool-backed one and the operator new
  fallback, which must request the alignment explicitly.
*/
bool
tests_allocated_storage_is_suitably_aligned()
{
    pa_over_pool  pool;
    pa_over_alloc allocator(pool);

    // the pool-backed path, across several growth events so more than one
    // backing allocation is sampled
    for (std::size_t i = 0; i < 32; ++i)
    {
        pa_overaligned* slot = allocator.allocate(1);
        D_PA_CHECK(slot != nullptr);
        D_PA_CHECK(pa_is_aligned(slot, alignof(pa_overaligned)));
        D_PA_CHECK(pa_is_aligned(slot, pa_over_pool::alignment()));
    }

    D_PA_CHECK(pool.size() == 32u);

    // the bulk path
    for (std::size_t count = 2; count <= 8; ++count)
    {
        pa_overaligned* block = allocator.allocate(count);
        D_PA_CHECK(block != nullptr);
        D_PA_CHECK(pa_is_aligned(block, alignof(pa_overaligned)));

        // every element in the block inherits the alignment
        for (std::size_t i = 0; i < count; ++i)
        {
            D_PA_CHECK(pa_is_aligned(&block[i],
                                     alignof(pa_overaligned)));
        }

        allocator.deallocate(block, count);
    }

    // the unbound fallback path must be aligned too
    {
        pa_over_pool  other;
        pa_over_alloc bound(other);
        pool_allocator<pa_element,
                       free_list_release_policy,
                       chunked_block_policy<4>,
                       pa_growth_exact> rebound(bound);

        D_PA_CHECK(rebound.resource() == nullptr);

        pa_element* storage = rebound.allocate(1);
        D_PA_CHECK(storage != nullptr);
        D_PA_CHECK(pa_is_aligned(storage, alignof(pa_element)));
        rebound.deallocate(storage, 1);
    }

    // naturally aligned elements are aligned on both paths as well
    {
        pa_pool     plain_pool(4);
        pa_alloc    plain(plain_pool);
        pa_element* single = plain.allocate(1);
        pa_element* block  = plain.allocate(4);

        D_PA_CHECK(pa_is_aligned(single, alignof(pa_element)));
        D_PA_CHECK(pa_is_aligned(block, alignof(pa_element)));

        plain.deallocate(single, 1);
        plain.deallocate(block, 4);
    }

    return true;
}

/*
tests_allocation_round_trip_recycles_slots
  Verifies a long allocate/deallocate cycle reuses pool slots and never
  grows the pool, and that a wider working set behaves the same way.
*/
bool
tests_allocation_round_trip_recycles_slots()
{
    // one slot, recycled repeatedly
    {
        pa_pool  pool(1);
        pa_alloc allocator(pool);

        pa_element* expected = nullptr;

        for (std::size_t i = 0; i < 1000; ++i)
        {
            pa_element* slot = allocator.allocate(1);
            D_PA_CHECK(slot != nullptr);

            if (i == 0)
            {
                expected = slot;
            }
            else
            {
                D_PA_CHECK(slot == expected);
            }

            allocator.deallocate(slot, 1);
        }

        D_PA_CHECK(pool.capacity() == 1u);
        D_PA_CHECK(pool.size() == 0u);
    }

    // a working set of eight, held and released together
    {
        constexpr std::size_t width = 8;

        pa_pool     pool(width);
        pa_alloc    allocator(pool);
        pa_element* held[width] = {};

        for (std::size_t round = 0; round < 100; ++round)
        {
            for (std::size_t i = 0; i < width; ++i)
            {
                held[i] = allocator.allocate(1);
                D_PA_CHECK(held[i] != nullptr);
                held[i]->m_lo = static_cast<std::uint64_t>(i);
            }

            D_PA_CHECK(pool.size() == width);

            for (std::size_t i = 0; i < width; ++i)
            {
                D_PA_CHECK(held[i]->m_lo ==
                           static_cast<std::uint64_t>(i));
            }

            for (std::size_t i = 0; i < width; ++i)
            {
                allocator.deallocate(held[i], 1);
            }

            D_PA_CHECK(pool.size() == 0u);
        }

        D_PA_CHECK(pool.capacity() == width);
    }

    // mixed single and bulk traffic stays balanced
    {
        pa_pool  pool(4);
        pa_alloc allocator(pool);

        for (std::size_t i = 0; i < 200; ++i)
        {
            pa_element* single = allocator.allocate(1);
            pa_element* bulk   = allocator.allocate(3);

            D_PA_CHECK(pool.size() == 1u);

            allocator.deallocate(bulk, 3);
            allocator.deallocate(single, 1);

            D_PA_CHECK(pool.size() == 0u);
        }

        D_PA_CHECK(pool.capacity() == 4u);
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
