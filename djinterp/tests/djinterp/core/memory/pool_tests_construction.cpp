/******************************************************************************
* djinterp [test]                                   pool_tests_construction.cpp
*
*   Section VI part 1 of the pool suite - lifetime.
*
*   pool_resource is a move-only owner of raw storage: it may be default
* constructed empty, constructed with a starting capacity, moved from and
* to, and destroyed at any point in that cycle.  Copy is deleted outright.
* These tests pin each of those transitions, including the self-move guard
* and the state a moved-from pool is left in.
*
*
* path:      /tests/djinterp/core/memory/pool_tests_construction.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

// std
#include <utility>
// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_default_construction_is_empty
  Verifies a default-constructed pool reports zero size, zero capacity,
  empty, zero allocated bytes, zero bytes in use, and 0.0 utilization -
  it must not touch the allocator until asked to.
*/
bool
tests_default_construction_is_empty()
{
    pool_fl_chunked chunked;
    D_PR_CHECK(chunked.size()            == 0u);
    D_PR_CHECK(chunked.capacity()        == 0u);
    D_PR_CHECK(chunked.empty());
    D_PR_CHECK(chunked.bytes_allocated() == 0u);
    D_PR_CHECK(chunked.bytes_in_use()    == 0u);
    D_PR_CHECK(chunked.utilization()     == 0.0);

    pool_fl_contig contiguous;
    D_PR_CHECK(contiguous.size()            == 0u);
    D_PR_CHECK(contiguous.capacity()        == 0u);
    D_PR_CHECK(contiguous.empty());
    D_PR_CHECK(contiguous.bytes_allocated() == 0u);

    pool_mono_chunked monotonic;
    D_PR_CHECK(monotonic.capacity() == 0u);
    D_PR_CHECK(monotonic.empty());

    pool_gen_chunked generational;
    D_PR_CHECK(generational.capacity() == 0u);
    D_PR_CHECK(generational.empty());

    // default construction is noexcept
    D_PR_CHECK(std::is_nothrow_default_constructible<
                   pool_fl_chunked>::value);

    return true;
}

/*
tests_capacity_construction_preallocates
  Verifies the capacity constructor reserves at least the requested
  number of slots, allocates backing storage for them, and still reports
  an empty pool because nothing has been acquired yet.
*/
bool
tests_capacity_construction_preallocates()
{
    // chunked: four slots per block, so eight is exactly two blocks
    {
        pool_fl_chunked pool(8);

        D_PR_CHECK(pool.capacity() >= 8u);
        D_PR_CHECK(pool.capacity() == 8u);
        D_PR_CHECK(pool.size() == 0u);
        D_PR_CHECK(pool.empty());
        D_PR_CHECK(pool.bytes_allocated() > 0u);
        D_PR_CHECK(pool.bytes_in_use() == 0u);
    }

    // a request that is not a whole number of blocks still lands exactly
    {
        pool_fl_chunked pool(10);

        D_PR_CHECK(pool.capacity() == 10u);
    }

    // contiguous: one allocation of exactly the requested size
    {
        pool_fl_contig pool(8);

        D_PR_CHECK(pool.capacity() == 8u);
        D_PR_CHECK(pool.empty());
        D_PR_CHECK(pool.bytes_allocated() ==
                   (8u * pool_fl_contig::bytes_per_slot()));
    }

    // the reserved capacity is genuinely usable without further growth
    {
        pool_fl_chunked_fixed pool(5);

        D_PR_CHECK(pool.capacity() == 5u);

        for (std::size_t i = 0; i < 5; ++i)
        {
            D_PR_CHECK(pool.acquire() != nullptr);
        }

        D_PR_CHECK(pool.size() == 5u);
    }

    // the constructor is explicit - a size_type must not convert silently
    D_PR_CHECK((!std::is_convertible<std::size_t,
                                     pool_fl_chunked>::value));

    return true;
}

/*
tests_zero_capacity_construction_allocates_nothing
  Verifies constructing with a capacity request of zero takes the guarded
  branch and allocates no backing storage at all, leaving a pool
  indistinguishable from a default-constructed one.
*/
bool
tests_zero_capacity_construction_allocates_nothing()
{
    pool_fl_chunked chunked(0);
    D_PR_CHECK(chunked.capacity()        == 0u);
    D_PR_CHECK(chunked.size()            == 0u);
    D_PR_CHECK(chunked.bytes_allocated() == 0u);
    D_PR_CHECK(chunked.empty());

    pool_fl_contig contiguous(0);
    D_PR_CHECK(contiguous.capacity()        == 0u);
    D_PR_CHECK(contiguous.bytes_allocated() == 0u);

    // and such a pool still works when it is finally asked for a slot
    void* slot = chunked.acquire();
    D_PR_CHECK(slot != nullptr);
    D_PR_CHECK(chunked.capacity() >= 1u);

    return true;
}

/*
tests_construction_across_the_policy_matrix
  Verifies every release/block combination constructs with a capacity,
  serves a slot, takes it back, and destroys cleanly - the smoke test that
  no policy pairing is unimplementable.
*/
bool
tests_construction_across_the_policy_matrix()
{
    {
        pool_fl_chunked pool(4);
        void*           slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);
        pool.release(slot);
        D_PR_CHECK(pool.empty());
    }
    {
        pool_fl_contig pool(4);
        void*          slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);
        pool.release(slot);
        D_PR_CHECK(pool.empty());
    }
    {
        pool_mono_chunked pool(4);
        void*             slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);
        pool.release(slot);
        D_PR_CHECK(pool.empty());
    }
    {
        pool_mono_contig pool(4);
        void*            slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);
        pool.release(slot);
        D_PR_CHECK(pool.empty());
    }
    {
        pool_gen_chunked pool(4);
        void*            slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);
        pool.release(slot);
        D_PR_CHECK(pool.empty());
    }
    {
        pool_gen_contig pool(4);
        void*           slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);
        pool.release(slot);
        D_PR_CHECK(pool.empty());
    }

    // over-aligned elements and single-slot blocks are not special cases
    {
        pool_over_chunked pool(4);
        D_PR_CHECK(pool.acquire() != nullptr);
    }
    {
        using single_slot_blocks =
            pool_resource<pool_medium_t,
                          free_list_release_policy,
                          chunked_block_policy<1>,
                          pool_growth_exact>;

        single_slot_blocks pool(4);
        D_PR_CHECK(pool.capacity() == 4u);
        D_PR_CHECK(pool.acquire() != nullptr);
    }

    return true;
}

/*
tests_pool_is_non_copyable
  Verifies the copy constructor and copy assignment operator are deleted -
  a pool owns raw storage that cannot be shared by value.
*/
bool
tests_pool_is_non_copyable()
{
    D_PR_CHECK(!std::is_copy_constructible<pool_fl_chunked>::value);
    D_PR_CHECK(!std::is_copy_assignable<pool_fl_chunked>::value);
    D_PR_CHECK(!std::is_copy_constructible<pool_fl_contig>::value);
    D_PR_CHECK(!std::is_copy_assignable<pool_fl_contig>::value);
    D_PR_CHECK(!std::is_copy_constructible<pool_mono_chunked>::value);
    D_PR_CHECK(!std::is_copy_constructible<pool_gen_chunked>::value);

    // and therefore not trivially copyable either
    D_PR_CHECK(!std::is_trivially_copyable<pool_fl_chunked>::value);

    return true;
}

/*
tests_pool_move_operations_are_noexcept
  Verifies move construction and move assignment exist and are declared
  noexcept, so containers of pools can relocate without a fallback path.
*/
bool
tests_pool_move_operations_are_noexcept()
{
    D_PR_CHECK(std::is_move_constructible<pool_fl_chunked>::value);
    D_PR_CHECK(std::is_move_assignable<pool_fl_chunked>::value);

    D_PR_CHECK(std::is_nothrow_move_constructible<pool_fl_chunked>::value);
    D_PR_CHECK(std::is_nothrow_move_assignable<pool_fl_chunked>::value);
    D_PR_CHECK(std::is_nothrow_move_constructible<pool_fl_contig>::value);
    D_PR_CHECK(std::is_nothrow_move_assignable<pool_fl_contig>::value);
    D_PR_CHECK(std::is_nothrow_move_constructible<
                   pool_mono_chunked>::value);
    D_PR_CHECK(std::is_nothrow_move_constructible<
                   pool_gen_chunked>::value);

    // destruction never throws
    D_PR_CHECK(std::is_nothrow_destructible<pool_fl_chunked>::value);

    return true;
}

/*
tests_move_construction_transfers_ownership
  Verifies the destination adopts the source's size, capacity and backing
  storage, that a slot acquired before the move is still served by the
  destination, and that the source is left empty and owning nothing.
*/
bool
tests_move_construction_transfers_ownership()
{
    pool_fl_chunked source(8);

    void* first  = source.acquire();
    void* second = source.acquire();
    D_PR_CHECK(first  != nullptr);
    D_PR_CHECK(second != nullptr);

    const std::size_t size_before     = source.size();
    const std::size_t capacity_before = source.capacity();
    const std::size_t bytes_before    = source.bytes_allocated();

    pool_fl_chunked destination(std::move(source));

    D_PR_CHECK(destination.size()            == size_before);
    D_PR_CHECK(destination.capacity()        == capacity_before);
    D_PR_CHECK(destination.bytes_allocated() == bytes_before);

    // the source is emptied, not merely detached
    D_PR_CHECK(source.size()            == 0u);
    D_PR_CHECK(source.capacity()        == 0u);
    D_PR_CHECK(source.bytes_allocated() == 0u);
    D_PR_CHECK(source.empty());

    // the moved storage still belongs to the destination: chunked blocks
    // do not relocate, so the earlier pointers remain in its arena and
    // can be handed back to it
    destination.release(first);
    destination.release(second);
    D_PR_CHECK(destination.size() == 0u);
    D_PR_CHECK(destination.capacity() == capacity_before);

    return true;
}

/*
tests_move_assignment_transfers_ownership
  Verifies move assignment releases whatever the destination already
  owned before adopting the source's state, and empties the source.
*/
bool
tests_move_assignment_transfers_ownership()
{
    pool_fl_chunked source(12);
    source.acquire();
    source.acquire();
    source.acquire();

    const std::size_t size_before     = source.size();
    const std::size_t capacity_before = source.capacity();

    // the destination starts out owning a different arena entirely
    pool_fl_chunked destination(4);
    destination.acquire();
    D_PR_CHECK(destination.capacity() == 4u);
    D_PR_CHECK(destination.size() == 1u);

    destination = std::move(source);

    D_PR_CHECK(destination.size()     == size_before);
    D_PR_CHECK(destination.capacity() == capacity_before);
    D_PR_CHECK(source.size()          == 0u);
    D_PR_CHECK(source.capacity()      == 0u);

    // move assignment onto an empty destination works the same way
    pool_fl_contig from(6);
    from.acquire();
    pool_fl_contig onto;
    onto = std::move(from);
    D_PR_CHECK(onto.capacity() == 6u);
    D_PR_CHECK(onto.size()     == 1u);
    D_PR_CHECK(from.capacity() == 0u);

    return true;
}

/*
tests_self_move_assignment_is_a_noop
  Verifies the `this != &_other` guard holds: assigning a pool to itself
  must not release its own storage out from under it.
*/
bool
tests_self_move_assignment_is_a_noop()
{
    pool_fl_chunked pool(8);

    void* slot = pool.acquire();
    pool.acquire();

    const std::size_t size_before     = pool.size();
    const std::size_t capacity_before = pool.capacity();
    const std::size_t bytes_before    = pool.bytes_allocated();

    // routed through a reference so the compiler cannot elide the call
    pool_fl_chunked& alias = pool;
    pool = std::move(alias);

    D_PR_CHECK(pool.size()            == size_before);
    D_PR_CHECK(pool.capacity()        == capacity_before);
    D_PR_CHECK(pool.bytes_allocated() == bytes_before);

    // the storage survived, so the slot can still be returned and reused
    pool.release(slot);
    D_PR_CHECK(pool.size() == (size_before - 1u));
    D_PR_CHECK(pool.acquire() == slot);

    return true;
}

/*
tests_moved_from_pool_is_valid_and_reusable
  Verifies a moved-from pool is left in a well-defined empty state - not
  merely destructible, but able to grow and serve slots again.
*/
bool
tests_moved_from_pool_is_valid_and_reusable()
{
    // move construction
    {
        pool_fl_chunked source(8);
        source.acquire();

        pool_fl_chunked sink(std::move(source));
        (void)sink;

        D_PR_CHECK(source.empty());
        D_PR_CHECK(source.capacity() == 0u);

        void* revived = source.acquire();
        D_PR_CHECK(revived != nullptr);
        D_PR_CHECK(source.size() == 1u);
        source.release(revived);
        D_PR_CHECK(source.empty());
    }

    // move assignment, contiguous layout
    {
        pool_fl_contig source(8);
        source.acquire();

        pool_fl_contig sink;
        sink = std::move(source);

        D_PR_CHECK(source.empty());
        D_PR_CHECK(source.capacity() == 0u);

        D_PR_CHECK(source.reserve(4));
        D_PR_CHECK(source.capacity() == 4u);
        D_PR_CHECK(source.acquire() != nullptr);
    }

    // reset and clear on a moved-from pool are safe
    {
        pool_fl_chunked source(4);
        pool_fl_chunked sink(std::move(source));
        (void)sink;

        source.reset();
        source.clear();
        D_PR_CHECK(source.empty());
        D_PR_CHECK(source.capacity() == 0u);
    }

    return true;
}

/*
tests_repeated_construction_destruction_is_clean
  Verifies many construct/grow/acquire/destroy cycles across both layouts
  complete without corrupting the allocator - the destructor must free
  every block it linked, exactly once.  Run under a sanitizer this is the
  suite's leak and double-free probe.
*/
bool
tests_repeated_construction_destruction_is_clean()
{
    for (std::size_t round = 0; round < 64; ++round)
    {
        {
            pool_fl_chunked pool;

            for (std::size_t i = 0; i < (round % 17); ++i)
            {
                D_PR_CHECK(pool.acquire() != nullptr);
            }
        }
        {
            pool_fl_contig pool(round % 13);

            for (std::size_t i = 0; i < (round % 13); ++i)
            {
                D_PR_CHECK(pool.acquire() != nullptr);
            }
        }
        {
            pool_gen_chunked pool(round % 9);
            pool.advance_generation();
        }
        {
            // destroyed immediately after a clear, so the destructor
            // runs against already-released state
            pool_fl_chunked pool(8);
            pool.acquire();
            pool.clear();
        }
        {
            // destroyed while slots are still outstanding
            pool_mono_chunked pool(8);
            pool.acquire();
            pool.acquire();
        }
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
