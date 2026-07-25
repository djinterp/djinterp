#include "test_compress_options_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_compress_options_equal_reflexive
  the baseline the aggregate comparator must satisfy.
  Tests the following:
  - a set equals itself
  - two independently default-constructed sets are equal, so equality is by
    value rather than by identity
  - the module's own default_compress_options agrees with a locally
    constructed set
*/
bool
tests_compress_options_equal_reflexive()
{
    const dj::compress_options a;
    const dj::compress_options b;

    D_CO_CHECK(dt::compress_options_equal(a, a));
    D_CO_CHECK(dt::compress_options_equal(a, b));
    D_CO_CHECK(dt::compress_options_equal(b, a));

    // the module's factory produces the same value
    D_CO_CHECK(dt::compress_options_equal(a, dt::default_compress_options()));

    return true;
}

/*
tests_compress_options_equal_symmetric
  operand order.
  Tests the following:
  - the comparison gives the same answer whichever way round its operands
    are passed, for every single-field change in the table
  - this holds for the equal case as well as the unequal one
*/
bool
tests_compress_options_equal_symmetric()
{
    const dj::compress_options  fresh;
    const std::vector<co_field> t = co_all_mutators();
    std::size_t                 i = 0;

    // symmetry across every single-field mutation
    for (i = 0; i < t.size(); ++i)
    {
        const dj::compress_options moved = co_mutated(i);

        D_CO_CHECK(dt::compress_options_equal(fresh, moved) ==
                   dt::compress_options_equal(moved, fresh));
        D_CO_CHECK(!dt::compress_options_equal(fresh, moved));
        D_CO_CHECK(!dt::compress_options_equal(moved, fresh));

        // and a set is symmetric with its own copy
        const dj::compress_options copy = moved;

        D_CO_CHECK(dt::compress_options_equal(moved, copy));
        D_CO_CHECK(dt::compress_options_equal(copy, moved));
    }

    return true;
}

/*
tests_compress_options_equal_detects_generic_level
  the one field that belongs to no block.
  Tests the following:
  - the aggregate compares compress_options::level in its own right
  - it is distinct from zstd.level and from lz4.level, which are separate
    fields of their own blocks
*/
bool
tests_compress_options_equal_detects_generic_level()
{
    const dj::compress_options fresh;
    dj::compress_options       moved;

    moved.level = (fresh.level + 1);

    D_CO_CHECK(!dt::compress_options_equal(fresh, moved));

    // moving it back restores equality
    moved.level = fresh.level;
    D_CO_CHECK(dt::compress_options_equal(fresh, moved));

    // the generic level is not the zstd level
    dj::compress_options generic;
    dj::compress_options zstd_only;

    generic.level     = 9;
    zstd_only.zstd.level = 9;

    D_CO_CHECK(!dt::compress_options_equal(generic, zstd_only));
    D_CO_CHECK(!dt::compress_options_equal(generic, fresh));
    D_CO_CHECK(!dt::compress_options_equal(zstd_only, fresh));

    // nor the lz4 level
    dj::compress_options lz4_only;

    lz4_only.lz4.level = 9;
    D_CO_CHECK(!dt::compress_options_equal(generic, lz4_only));
    D_CO_CHECK(!dt::compress_options_equal(zstd_only, lz4_only));

    return true;
}

/*
tests_compress_options_equal_detects_every_field
  whole-surface coverage.
  Tests the following:
  - every one of the fifty fields is reachable by the aggregate comparator,
    so no field is dropped between the blocks and the roll-up
  - reverting the change restores equality in each case
*/
bool
tests_compress_options_equal_detects_every_field()
{
    const dj::compress_options  fresh;
    const std::vector<co_field> t = co_all_mutators();
    std::size_t                 i = 0;

    D_CO_CHECK(t.size() == 50u);

    // every field must be visible to the aggregate
    for (i = 0; i < t.size(); ++i)
    {
        const dj::compress_options moved = co_mutated(i);

        if (dt::compress_options_equal(fresh, moved))
        {
            std::printf("    [FAIL] aggregate missed field %s\n", t[i].label);

            return false;
        }
    }

    // and a set built by applying then undoing a change is equal again
    for (i = 0; i < t.size(); ++i)
    {
        dj::compress_options o = co_mutated(i);

        o = dj::compress_options();
        D_CO_CHECK(dt::compress_options_equal(fresh, o));
    }

    return true;
}

/*
tests_compress_options_equal_multi_change
  several fields at once.
  Tests the following:
  - simultaneous changes across different blocks are still detected
  - a set carrying every field moved is unequal to pristine
  - restoring one of several changes is not enough to regain equality
*/
bool
tests_compress_options_equal_multi_change()
{
    const dj::compress_options  fresh;
    const std::vector<co_field> t = co_all_mutators();
    dj::compress_options        many;
    std::size_t                 i = 0;

    // move one field in each of three different blocks
    many.deflate.window_bits = (fresh.deflate.window_bits + 1);
    many.zstd.workers        = (fresh.zstd.workers + 1);
    many.brotli.large_window = !fresh.brotli.large_window;

    D_CO_CHECK(!dt::compress_options_equal(fresh, many));

    // undoing two of the three still leaves the set unequal
    many.zstd.workers        = fresh.zstd.workers;
    many.brotli.large_window = fresh.brotli.large_window;
    D_CO_CHECK(!dt::compress_options_equal(fresh, many));

    // undoing the last one restores equality
    many.deflate.window_bits = fresh.deflate.window_bits;
    D_CO_CHECK(dt::compress_options_equal(fresh, many));

    // a set with EVERY field moved is unequal
    dj::compress_options all;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(all);
    }

    D_CO_CHECK(!dt::compress_options_equal(fresh, all));

    // and every block comparator disagrees on it
    D_CO_CHECK(!dt::deflate_options_equal(fresh.deflate, all.deflate));
    D_CO_CHECK(!dt::bzip2_options_equal(fresh.bzip2, all.bzip2));
    D_CO_CHECK(!dt::lzma_options_equal(fresh.lzma, all.lzma));
    D_CO_CHECK(!dt::zstd_options_equal(fresh.zstd, all.zstd));
    D_CO_CHECK(!dt::lz4_options_equal(fresh.lz4, all.lz4));
    D_CO_CHECK(!dt::brotli_options_equal(fresh.brotli, all.brotli));

    return true;
}

/*
tests_compress_options_equal_survives_copy
  value semantics.
  Tests the following:
  - a copy-constructed set compares equal to its source
  - an assigned set compares equal to its source
  - a later change to the source does not follow the copy, so the comparator
    is reading two genuinely separate objects
*/
bool
tests_compress_options_equal_survives_copy()
{
    const std::vector<co_field> t = co_all_mutators();
    dj::compress_options        src;
    std::size_t                 i = 0;

    // build a thoroughly non-default source
    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(src);
    }

    const dj::compress_options copied(src);

    D_CO_CHECK(dt::compress_options_equal(src, copied));

    dj::compress_options assigned;

    assigned = src;
    D_CO_CHECK(dt::compress_options_equal(src, assigned));

    // the copies are independent of the source
    src.level = (src.level + 1);

    D_CO_CHECK(!dt::compress_options_equal(src, copied));
    D_CO_CHECK(!dt::compress_options_equal(src, assigned));
    D_CO_CHECK(dt::compress_options_equal(copied, assigned));

    return true;
}

NS_END  // testing
NS_END  // djinterp
