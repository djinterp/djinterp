/******************************************************************************
* djinterp [test]                                       pool_tests_geometry.cpp
*
*   Section V of the pool suite - slot geometry.
*
*   A slot must be large enough to hold either the element or the intrusive
* free-list link, and aligned to the stricter of the two; the size is then
* rounded up to that alignment so slots tile a block exactly.  The five
* element fixtures cover every branch of that computation, and the last two
* tests pin the runtime consequence: storage handed out by acquire() really
* does satisfy the alignment the pool advertises, and live slots never
* overlap.
*
*
* path:      /tests/djinterp/core/memory/pool_tests_geometry.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // geometry_of
    //   alias: a pool over _Type with fixed policies, so a geometry test
    // names only the element type it cares about.
    template<typename _Type>
    using geometry_of =
        pool_resource<_Type,
                      free_list_release_policy,
                      chunked_block_policy<4>,
                      pool_growth_exact>;
}


/*
tests_slot_geometry_clamps_small_types
  Verifies an element smaller than the free-list link still yields a slot
  large enough to hold that link - otherwise release() would scribble past
  the end of the slot when threading it onto the free list.
*/
bool
tests_slot_geometry_clamps_small_types()
{
    using small_pool = geometry_of<pool_small_t>;

    D_PR_CHECK(sizeof(pool_small_t) < sizeof(pool_link_probe));

    D_PR_CHECK(small_pool::bytes_per_slot() >= sizeof(pool_link_probe));
    D_PR_CHECK(small_pool::bytes_per_slot() >= sizeof(pool_small_t));
    D_PR_CHECK(small_pool::bytes_per_slot() ==
               pool_expected_slot<pool_small_t>());
    D_PR_CHECK(small_pool::alignment() ==
               pool_expected_align<pool_small_t>());

    // the slot alignment is raised to the link's, not left at char's
    D_PR_CHECK(small_pool::alignment() >= alignof(pool_link_probe));
    D_PR_CHECK(small_pool::alignment() > alignof(pool_small_t));

    // and a slot really can hold a link
    small_pool pool;
    void*      slot = pool.acquire();
    D_PR_CHECK(slot != nullptr);
    pool.release(slot);
    D_PR_CHECK(pool.empty());

    return true;
}

/*
tests_slot_geometry_rounds_up_to_alignment
  Verifies a raw size that is not a multiple of the slot alignment is
  padded up to one, so consecutive slots stay aligned across a block.
*/
bool
tests_slot_geometry_rounds_up_to_alignment()
{
    using odd_pool = geometry_of<pool_odd_t>;

    // the fixture must actually exercise the padding branch
    D_PR_CHECK((sizeof(pool_odd_t) %
                pool_expected_align<pool_odd_t>()) != 0u);

    D_PR_CHECK(odd_pool::bytes_per_slot() ==
               pool_expected_slot<pool_odd_t>());
    D_PR_CHECK(odd_pool::bytes_per_slot() > sizeof(pool_odd_t));
    D_PR_CHECK((odd_pool::bytes_per_slot() % odd_pool::alignment()) == 0u);

    // an exactly-sized element needs no padding at all
    using medium_pool = geometry_of<pool_medium_t>;
    D_PR_CHECK(medium_pool::bytes_per_slot() == sizeof(pool_medium_t));
    D_PR_CHECK((medium_pool::bytes_per_slot() %
                medium_pool::alignment()) == 0u);

    // consecutive slots in a block stay aligned once the size is padded
    odd_pool pool(4);
    void*    slots[4] = { nullptr, nullptr, nullptr, nullptr };

    for (std::size_t i = 0; i < 4; ++i)
    {
        slots[i] = pool.acquire();
        D_PR_CHECK(slots[i] != nullptr);
        D_PR_CHECK(pool_is_aligned(slots[i], odd_pool::alignment()));
    }

    return true;
}

/*
tests_slot_geometry_large_and_overaligned_types
  Verifies a link-dominating element drives the slot size while an
  over-aligned element drives the slot alignment, and that the slot size
  remains a whole multiple of the alignment in both cases.
*/
bool
tests_slot_geometry_large_and_overaligned_types()
{
    using large_pool = geometry_of<pool_large_t>;
    using over_pool  = geometry_of<pool_overaligned_t>;

    // large: the element dominates the link on size
    D_PR_CHECK(sizeof(pool_large_t) > sizeof(pool_link_probe));
    D_PR_CHECK(large_pool::bytes_per_slot() == sizeof(pool_large_t));
    D_PR_CHECK(large_pool::bytes_per_slot() ==
               pool_expected_slot<pool_large_t>());
    D_PR_CHECK(large_pool::alignment() ==
               pool_expected_align<pool_large_t>());

    // over-aligned: the element dominates the link on alignment
    D_PR_CHECK(alignof(pool_overaligned_t) > alignof(pool_link_probe));
    D_PR_CHECK(over_pool::alignment() == alignof(pool_overaligned_t));
    D_PR_CHECK(over_pool::alignment() ==
               pool_expected_align<pool_overaligned_t>());
    D_PR_CHECK(over_pool::bytes_per_slot() ==
               pool_expected_slot<pool_overaligned_t>());

    // in every case the slot tiles cleanly
    D_PR_CHECK((large_pool::bytes_per_slot() %
                large_pool::alignment()) == 0u);
    D_PR_CHECK((over_pool::bytes_per_slot() %
                over_pool::alignment()) == 0u);

    return true;
}

/*
tests_slot_geometry_queries_are_static_constexpr
  Verifies bytes_per_slot() and alignment() are static, noexcept, return
  size_type, are usable in a constant expression, and agree with the
  matching public constants.
*/
bool
tests_slot_geometry_queries_are_static_constexpr()
{
    using pool_type = pool_fl_chunked;
    using size_type = pool_type::size_type;

    // usable without an object, and in a constant expression
    constexpr size_type slot  = pool_type::bytes_per_slot();
    constexpr size_type align = pool_type::alignment();

    static_assert(slot > 0, "slot size must be positive.");
    static_assert(align > 0, "slot alignment must be positive.");
    static_assert(pool_type::bytes_per_slot() == pool_type::slot_size,
                  "bytes_per_slot must mirror slot_size.");
    static_assert(pool_type::alignment() == pool_type::slot_align,
                  "alignment must mirror slot_align.");

    D_PR_CHECK(slot == pool_type::slot_size);
    D_PR_CHECK(align == pool_type::slot_align);

    D_PR_CHECK(noexcept(pool_type::bytes_per_slot()));
    D_PR_CHECK(noexcept(pool_type::alignment()));

    D_PR_CHECK((std::is_same<decltype(pool_type::bytes_per_slot()),
                             size_type>::value));
    D_PR_CHECK((std::is_same<decltype(pool_type::alignment()),
                             size_type>::value));
    D_PR_CHECK((std::is_same<size_type, std::size_t>::value));

    return true;
}

/*
tests_slot_geometry_is_policy_independent
  Verifies slot size and alignment are a function of the element type
  alone - swapping the release, block, or growth policy must not move
  them.
*/
bool
tests_slot_geometry_is_policy_independent()
{
    const std::size_t slot  = pool_expected_slot<pool_medium_t>();
    const std::size_t align = pool_expected_align<pool_medium_t>();

    D_PR_CHECK(pool_fl_chunked::bytes_per_slot()        == slot);
    D_PR_CHECK(pool_fl_contig::bytes_per_slot()         == slot);
    D_PR_CHECK(pool_mono_chunked::bytes_per_slot()      == slot);
    D_PR_CHECK(pool_mono_contig::bytes_per_slot()       == slot);
    D_PR_CHECK(pool_gen_chunked::bytes_per_slot()       == slot);
    D_PR_CHECK(pool_gen_contig::bytes_per_slot()        == slot);
    D_PR_CHECK(pool_fl_chunked_step::bytes_per_slot()   == slot);
    D_PR_CHECK(pool_fl_chunked_fixed::bytes_per_slot()  == slot);

    D_PR_CHECK(pool_fl_chunked::alignment()             == align);
    D_PR_CHECK(pool_fl_contig::alignment()              == align);
    D_PR_CHECK(pool_mono_chunked::alignment()           == align);
    D_PR_CHECK(pool_mono_contig::alignment()            == align);
    D_PR_CHECK(pool_gen_chunked::alignment()            == align);
    D_PR_CHECK(pool_gen_contig::alignment()             == align);

    // the block policy's slot count is likewise irrelevant to geometry
    using tiny_blocks =
        pool_resource<pool_medium_t,
                      free_list_release_policy,
                      chunked_block_policy<1>,
                      pool_growth_exact>;
    using huge_blocks =
        pool_resource<pool_medium_t,
                      free_list_release_policy,
                      chunked_block_policy<1024>,
                      pool_growth_exact>;

    D_PR_CHECK(tiny_blocks::bytes_per_slot() == slot);
    D_PR_CHECK(huge_blocks::bytes_per_slot() == slot);
    D_PR_CHECK(tiny_blocks::alignment()      == align);
    D_PR_CHECK(huge_blocks::alignment()      == align);

    return true;
}

/*
tests_acquired_slots_honour_slot_alignment
  Verifies every pointer acquire() hands back satisfies the alignment the
  pool advertises - for naturally aligned elements and, critically, for
  over-aligned ones, in both block layouts and across several growth
  events (so several independent backing allocations are sampled).
*/
bool
tests_acquired_slots_honour_slot_alignment()
{
    // naturally aligned element, chunked
    {
        pool_fl_chunked pool;

        for (std::size_t i = 0; i < 32; ++i)
        {
            void* slot = pool.acquire();
            D_PR_CHECK(slot != nullptr);
            D_PR_CHECK(pool_is_aligned(slot, pool_fl_chunked::alignment()));
        }
    }

    // naturally aligned element, contiguous
    {
        pool_fl_contig pool;

        for (std::size_t i = 0; i < 32; ++i)
        {
            void* slot = pool.acquire();
            D_PR_CHECK(slot != nullptr);
            D_PR_CHECK(pool_is_aligned(slot, pool_fl_contig::alignment()));
        }
    }

    // over-aligned element, chunked - many blocks, so the check does not
    // rest on a single backing allocation happening to be lucky
    {
        pool_over_chunked pool;

        for (std::size_t i = 0; i < 64; ++i)
        {
            void* slot = pool.acquire();
            D_PR_CHECK(slot != nullptr);
            D_PR_CHECK(pool_is_aligned(slot,
                                       pool_over_chunked::alignment()));
            D_PR_CHECK(pool_is_aligned(slot, alignof(pool_overaligned_t)));
        }
    }

    // over-aligned element, contiguous - one allocation, reallocated
    // repeatedly as it grows
    {
        pool_over_contig pool;

        for (std::size_t i = 0; i < 64; ++i)
        {
            void* slot = pool.acquire();
            D_PR_CHECK(slot != nullptr);
            D_PR_CHECK(pool_is_aligned(slot,
                                       pool_over_contig::alignment()));
            D_PR_CHECK(pool_is_aligned(slot, alignof(pool_overaligned_t)));
        }
    }

    // a reserved-up-front pool must be aligned from the first slot
    {
        pool_over_contig reserved(16);

        for (std::size_t i = 0; i < 16; ++i)
        {
            void* slot = reserved.acquire();
            D_PR_CHECK(slot != nullptr);
            D_PR_CHECK(pool_is_aligned(slot, alignof(pool_overaligned_t)));
        }
    }

    return true;
}

/*
tests_acquired_slots_do_not_overlap
  Verifies concurrently held slots are pairwise separated by at least the
  slot size, so writing a whole element through one can never touch
  another - checked within a block, across block boundaries, and after
  free-list recycling.
*/
bool
tests_acquired_slots_do_not_overlap()
{
    constexpr std::size_t count = 12;
    void*                 slots[count] = {};

    // spans three four-slot blocks
    {
        pool_fl_chunked pool;

        for (std::size_t i = 0; i < count; ++i)
        {
            slots[i] = pool.acquire();
            D_PR_CHECK(slots[i] != nullptr);
        }

        D_PR_CHECK(pool_all_distinct(slots, count));
        D_PR_CHECK(pool_slots_disjoint(slots, count,
                                       pool_fl_chunked::bytes_per_slot()));
    }

    // contiguous layout, reserved so no relocation occurs mid-run
    {
        pool_fl_contig pool(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            slots[i] = pool.acquire();
            D_PR_CHECK(slots[i] != nullptr);
        }

        D_PR_CHECK(pool_all_distinct(slots, count));
        D_PR_CHECK(pool_slots_disjoint(slots, count,
                                       pool_fl_contig::bytes_per_slot()));
    }

    // recycled slots are still disjoint from the ones still held
    {
        pool_fl_chunked pool(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            slots[i] = pool.acquire();
        }

        pool.release(slots[2]);
        pool.release(slots[5]);
        pool.release(slots[9]);

        slots[2] = pool.acquire();
        slots[5] = pool.acquire();
        slots[9] = pool.acquire();

        D_PR_CHECK(pool_all_distinct(slots, count));
        D_PR_CHECK(pool_slots_disjoint(slots, count,
                                       pool_fl_chunked::bytes_per_slot()));
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
