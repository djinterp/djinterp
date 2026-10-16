/*******************************************************************************
* djinterp [test]                                    pool_tests_generational.cpp
*
*   Section VI part 4 of the pool suite - the generation counter.
*
*   current_generation() and advance_generation() exist only when the
* release policy reports supports_generational_sweep, so half of this
* section is about the methods being present and half is about their being
* absent.  The gating is structural - a non-generational pool simply has no
* such members - and is checked through the module's own has_* detectors.
*
*   The counter's interaction with the bulk operations is deliberate and
* asymmetric: reset() rewinds slot state only, while clear() reinitialises
* block state and so returns the counter to zero.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_generational.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <utility>
// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    using generation_type =
        generational_release_policy::generation_type;
}


/*
tests_generation_starts_at_zero
  Verifies a freshly constructed generational pool reports generation 0,
  for both block layouts and whether or not capacity was requested.
*/
bool
tests_generation_starts_at_zero()
{
    {
        pool_gen_chunked pool;
        D_POOL_CHECK(pool.current_generation() == 0u);
    }
    {
        pool_gen_chunked pool(8);
        D_POOL_CHECK(pool.current_generation() == 0u);
    }
    {
        pool_gen_contig pool;
        D_POOL_CHECK(pool.current_generation() == 0u);
    }
    {
        pool_gen_contig pool(8);
        D_POOL_CHECK(pool.current_generation() == 0u);
    }

    // acquiring and releasing slots does not move the counter on its own
    {
        pool_gen_chunked pool(4);

        void* slot = pool.acquire();
        D_POOL_CHECK(pool.current_generation() == 0u);

        pool.release(slot);
        D_POOL_CHECK(pool.current_generation() == 0u);
    }

    return true;
}

/*
tests_advance_generation_returns_the_new_value
  Verifies advance_generation pre-increments - the first call returns 1,
  not 0 - that successive calls rise by exactly one, and that
  current_generation observes the latest value.
*/
bool
tests_advance_generation_returns_the_new_value()
{
    pool_gen_chunked pool;

    D_POOL_CHECK(pool.advance_generation() == 1u);
    D_POOL_CHECK(pool.current_generation() == 1u);

    D_POOL_CHECK(pool.advance_generation() == 2u);
    D_POOL_CHECK(pool.advance_generation() == 3u);
    D_POOL_CHECK(pool.current_generation() == 3u);

    // the returned value and the observed value never disagree
    for (std::size_t i = 0; i < 50; ++i)
    {
        const generation_type returned = pool.advance_generation();
        D_POOL_CHECK(returned == pool.current_generation());
    }

    D_POOL_CHECK(pool.current_generation() == 53u);

    // current_generation is a pure observer - calling it does not advance
    D_POOL_CHECK(pool.current_generation() == 53u);
    D_POOL_CHECK(pool.current_generation() == 53u);

    // the contiguous layout behaves identically
    {
        pool_gen_contig contiguous(4);
        D_POOL_CHECK(contiguous.advance_generation() == 1u);
        D_POOL_CHECK(contiguous.current_generation() == 1u);
    }

    return true;
}

/*
tests_generation_counter_spans_a_wide_range
  Verifies the counter holds values past the 16-bit boundary without
  truncation - the type is at least 32 bits wide and is used as such.
*/
bool
tests_generation_counter_spans_a_wide_range()
{
    pool_gen_chunked pool;

    for (std::size_t i = 0; i < 65537u; ++i)
    {
        pool.advance_generation();
    }

    D_POOL_CHECK(pool.current_generation() ==
               static_cast<generation_type>(65537u));

    // a 16-bit counter would have wrapped to 1 here
    D_POOL_CHECK(pool.current_generation() != 1u);
    D_POOL_CHECK(pool.current_generation() > 65535u);

    return true;
}

/*
tests_generation_survives_reset
  Verifies reset rewinds the live count, the free list and the bump cursor
  but deliberately leaves the generation counter alone - only slot state
  is rewound.
*/
bool
tests_generation_survives_reset()
{
    pool_gen_chunked pool(8);

    pool.advance_generation();
    pool.advance_generation();
    const generation_type before = pool.current_generation();
    D_POOL_CHECK(before == 2u);

    pool.acquire();
    pool.acquire();

    pool.reset();

    D_POOL_CHECK(pool.size() == 0u);
    D_POOL_CHECK(pool.capacity() == 8u);
    D_POOL_CHECK(pool.current_generation() == before);

    // repeated resets still leave it be
    pool.reset();
    pool.reset();
    D_POOL_CHECK(pool.current_generation() == before);

    // and the counter keeps advancing from where it was
    D_POOL_CHECK(pool.advance_generation() == (before + 1u));

    // contiguous behaves the same
    {
        pool_gen_contig contiguous(4);
        contiguous.advance_generation();
        contiguous.advance_generation();
        contiguous.advance_generation();

        contiguous.reset();
        D_POOL_CHECK(contiguous.current_generation() == 3u);
    }

    return true;
}

/*
tests_clear_rewinds_the_generation_counter
  Verifies clear reinitialises block state and so returns the generation
  counter to zero - the documented asymmetry with reset.
*/
bool
tests_clear_rewinds_the_generation_counter()
{
    pool_gen_chunked pool(8);

    pool.advance_generation();
    pool.advance_generation();
    pool.advance_generation();
    D_POOL_CHECK(pool.current_generation() == 3u);

    pool.clear();

    D_POOL_CHECK(pool.current_generation() == 0u);
    D_POOL_CHECK(pool.capacity() == 0u);

    // and it counts up from zero again afterwards
    D_POOL_CHECK(pool.advance_generation() == 1u);

    // contiguous, and clearing an already-empty pool
    {
        pool_gen_contig contiguous;
        contiguous.advance_generation();
        contiguous.clear();
        D_POOL_CHECK(contiguous.current_generation() == 0u);

        contiguous.clear();
        D_POOL_CHECK(contiguous.current_generation() == 0u);
    }

    return true;
}

/*
tests_generation_is_carried_across_moves
  Verifies move construction and move assignment carry the generation
  counter to the destination, so a relocated pool does not silently reissue
  generations it has already handed out.
*/
bool
tests_generation_is_carried_across_moves()
{
    // move construction
    {
        pool_gen_chunked source(4);

        for (std::size_t i = 0; i < 5; ++i)
        {
            source.advance_generation();
        }

        const generation_type before = source.current_generation();
        D_POOL_CHECK(before == 5u);

        pool_gen_chunked destination(std::move(source));

        D_POOL_CHECK(destination.current_generation() == before);
        D_POOL_CHECK(destination.advance_generation() == (before + 1u));
    }

    // move assignment, onto a destination with its own counter
    {
        pool_gen_chunked source;
        source.advance_generation();
        source.advance_generation();
        source.advance_generation();
        const generation_type before = source.current_generation();

        pool_gen_chunked destination;
        destination.advance_generation();
        D_POOL_CHECK(destination.current_generation() == 1u);

        destination = std::move(source);

        D_POOL_CHECK(destination.current_generation() == before);
    }

    // the moved-from pool is reinitialised, counter included
    {
        pool_gen_chunked source(4);
        source.advance_generation();
        source.advance_generation();

        pool_gen_chunked destination(std::move(source));
        (void)destination;

        D_POOL_CHECK(source.current_generation() == 0u);
    }

    // contiguous layout
    {
        pool_gen_contig source(4);
        source.advance_generation();
        source.advance_generation();
        source.advance_generation();
        source.advance_generation();

        pool_gen_contig destination(std::move(source));
        D_POOL_CHECK(destination.current_generation() == 4u);
    }

    return true;
}

/*
tests_generational_api_is_sfinae_gated
  Verifies the generation methods exist on generational pools and are
  absent - not merely ill-formed on call - from monotonic and free-list
  pools, in both block layouts.
*/
bool
tests_generational_api_is_sfinae_gated()
{
    // present
    D_POOL_CHECK(has_current_generation_v<pool_gen_chunked>);
    D_POOL_CHECK(has_advance_generation_v<pool_gen_chunked>);
    D_POOL_CHECK(has_current_generation_v<pool_gen_contig>);
    D_POOL_CHECK(has_advance_generation_v<pool_gen_contig>);

    // absent on free-list pools
    D_POOL_CHECK(!has_current_generation_v<pool_fl_chunked>);
    D_POOL_CHECK(!has_advance_generation_v<pool_fl_chunked>);
    D_POOL_CHECK(!has_current_generation_v<pool_fl_contig>);
    D_POOL_CHECK(!has_advance_generation_v<pool_fl_contig>);

    // absent on monotonic pools
    D_POOL_CHECK(!has_current_generation_v<pool_mono_chunked>);
    D_POOL_CHECK(!has_advance_generation_v<pool_mono_chunked>);
    D_POOL_CHECK(!has_current_generation_v<pool_mono_contig>);
    D_POOL_CHECK(!has_advance_generation_v<pool_mono_contig>);

    // the gate follows the policy constant, not the block layout
    D_POOL_CHECK(has_generational_sweep_v<pool_gen_chunked>);
    D_POOL_CHECK(!has_generational_sweep_v<pool_fl_chunked>);
    D_POOL_CHECK(!has_generational_sweep_v<pool_mono_chunked>);

    return true;
}

/*
tests_generational_api_signatures
  Verifies both methods return the policy's generation_type, that
  current_generation is callable on a const pool, and that both are
  noexcept.
*/
bool
tests_generational_api_signatures()
{
    D_POOL_CHECK((std::is_same<
                    decltype(std::declval<const pool_gen_chunked&>()
                                 .current_generation()),
                    generation_type>::value));

    D_POOL_CHECK((std::is_same<
                    decltype(std::declval<pool_gen_chunked&>()
                                 .advance_generation()),
                    generation_type>::value));

    D_POOL_CHECK(noexcept(std::declval<const pool_gen_chunked&>()
                            .current_generation()));
    D_POOL_CHECK(noexcept(std::declval<pool_gen_chunked&>()
                            .advance_generation()));

    // current_generation really is const-callable
    const pool_gen_chunked pool;
    D_POOL_CHECK(pool.current_generation() == 0u);

    // the return type is the policy's, not some internal counter type
    D_POOL_CHECK((std::is_same<
                    generation_type,
                    generational_release_policy::generation_type>::value));
    D_POOL_CHECK(std::is_unsigned<generation_type>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
