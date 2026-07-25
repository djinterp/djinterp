#include "test_compress_options_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_compress_options_level_builder_scope
  the generic-effort preset.
  Tests the following:
  - compress_level_options records the requested effort in the generic level
  - it moves nothing else, so the diff against pristine names level alone
  - every per-codec block stays pristine
*/
bool
tests_compress_options_level_builder_scope()
{
    const dj::compress_options fresh;
    const dj::compress_options o = dt::compress_level_options(6);

    D_CO_CHECK(o.level == 6);
    D_CO_CHECK(!dt::compress_options_are_default(o));
    D_CO_CHECK(dt::describe_compress_diff_from_default(o) == "level");

    // every block is untouched
    D_CO_CHECK(dt::deflate_options_equal(fresh.deflate, o.deflate));
    D_CO_CHECK(dt::bzip2_options_equal(fresh.bzip2, o.bzip2));
    D_CO_CHECK(dt::lzma_options_equal(fresh.lzma, o.lzma));
    D_CO_CHECK(dt::zstd_options_equal(fresh.zstd, o.zstd));
    D_CO_CHECK(dt::lz4_options_equal(fresh.lz4, o.lz4));
    D_CO_CHECK(dt::brotli_options_equal(fresh.brotli, o.brotli));

    // in particular the zstd block's own level did not follow
    D_CO_CHECK(o.zstd.level == fresh.zstd.level);
    D_CO_CHECK(o.lz4.level == fresh.lz4.level);

    return true;
}

/*
tests_compress_options_level_builder_any_value
  the preset's transparency.
  Tests the following:
  - negative, zero and large efforts are carried through unclamped
  - requesting the default effort produces a set that is still pristine
  - two calls with the same effort produce equal sets
*/
bool
tests_compress_options_level_builder_any_value()
{
    D_CO_CHECK(dt::compress_level_options(-1).level == -1);
    D_CO_CHECK(dt::compress_level_options(0).level == 0);
    D_CO_CHECK(dt::compress_level_options(1).level == 1);
    D_CO_CHECK(dt::compress_level_options(9).level == 9);
    D_CO_CHECK(dt::compress_level_options(22).level == 22);
    D_CO_CHECK(dt::compress_level_options(-7).level == -7);

    // -1 IS the default effort, so that set is pristine and its diff empty
    const dj::compress_options at_default = dt::compress_level_options(-1);

    D_CO_CHECK(dt::compress_options_are_default(at_default));
    D_CO_CHECK(dt::describe_compress_diff_from_default(at_default).empty());

    // any other effort is not pristine
    D_CO_CHECK(!dt::compress_options_are_default(
                   dt::compress_level_options(0)));

    // the builder is deterministic
    D_CO_CHECK(dt::compress_options_equal(dt::compress_level_options(6),
                                          dt::compress_level_options(6)));
    D_CO_CHECK(!dt::compress_options_equal(dt::compress_level_options(6),
                                           dt::compress_level_options(7)));

    return true;
}

/*
tests_compress_options_deflate_builder_scope
  the DEFLATE preset's blast radius.
  Tests the following:
  - the other five blocks and the generic level stay pristine
  - the diff against pristine names only DEFLATE fields
  - the DEFLATE comparator sees the change
*/
bool
tests_compress_options_deflate_builder_scope()
{
    const dj::compress_options fresh;
    const dj::compress_options o =
        dt::deflate_tuned_options(15, 8, co_bump(fresh.deflate.strategy));

    D_CO_CHECK(o.level == fresh.level);
    D_CO_CHECK(dt::bzip2_options_equal(fresh.bzip2, o.bzip2));
    D_CO_CHECK(dt::lzma_options_equal(fresh.lzma, o.lzma));
    D_CO_CHECK(dt::zstd_options_equal(fresh.zstd, o.zstd));
    D_CO_CHECK(dt::lz4_options_equal(fresh.lz4, o.lz4));
    D_CO_CHECK(dt::brotli_options_equal(fresh.brotli, o.brotli));

    // the owning block does notice
    D_CO_CHECK(!dt::deflate_options_equal(fresh.deflate, o.deflate));
    D_CO_CHECK(!dt::compress_options_are_default(o));

    // and every named field belongs to deflate
    const std::vector<std::string> parts =
        co_split_csv(dt::describe_compress_diff_from_default(o));
    std::size_t                    i = 0;

    D_CO_CHECK(parts.size() == 3u);

    for (i = 0; i < parts.size(); ++i)
    {
        D_CO_CHECK(parts[i].compare(0, 8, "deflate.") == 0);
    }

    return true;
}

/*
tests_compress_options_deflate_builder_args
  argument pass-through.
  Tests the following:
  - all three arguments reach their own fields, in the right order
  - the arguments are independent, so window_bits and mem_level do not
    cross-contaminate
  - passing the default values back produces a pristine set
*/
bool
tests_compress_options_deflate_builder_args()
{
    const dj::compress_options fresh;
    const dj::deflate_strategy alt = co_bump(fresh.deflate.strategy);

    const dj::compress_options o = dt::deflate_tuned_options(15, 8, alt);

    D_CO_CHECK(o.deflate.window_bits == 15);
    D_CO_CHECK(o.deflate.mem_level == 8);
    D_CO_CHECK(o.deflate.strategy == alt);

    // the two integer knobs do not cross over
    const dj::compress_options swapped =
        dt::deflate_tuned_options(8, 15, alt);

    D_CO_CHECK(swapped.deflate.window_bits == 8);
    D_CO_CHECK(swapped.deflate.mem_level == 15);
    D_CO_CHECK(!dt::deflate_options_equal(o.deflate, swapped.deflate));

    // negative window bits (the raw-DEFLATE convention) pass through
    D_CO_CHECK(dt::deflate_tuned_options(-15, 8, alt).deflate.window_bits ==
               -15);

    // handing back the defaults produces a pristine set
    const dj::compress_options same =
        dt::deflate_tuned_options(fresh.deflate.window_bits,
                                  fresh.deflate.mem_level,
                                  fresh.deflate.strategy);

    D_CO_CHECK(dt::compress_options_are_default(same));
    D_CO_CHECK(dt::describe_compress_diff_from_default(same).empty());

    return true;
}

/*
tests_compress_options_zstd_builder_scope
  the zstd preset's blast radius.
  Tests the following:
  - zstd.level carries the requested effort
  - the other eighteen zstd knobs stay pristine, as do the other blocks
  - the diff names zstd.level alone
*/
bool
tests_compress_options_zstd_builder_scope()
{
    const dj::compress_options fresh;
    const dj::compress_options o = dt::zstd_level_options(19);

    D_CO_CHECK(o.zstd.level == 19);
    D_CO_CHECK(dt::describe_compress_diff_from_default(o) == "zstd.level");

    // the generic level did NOT move
    D_CO_CHECK(o.level == fresh.level);

    // the rest of the zstd block is pristine
    D_CO_CHECK(o.zstd.window_log == fresh.zstd.window_log);
    D_CO_CHECK(o.zstd.workers == fresh.zstd.workers);
    D_CO_CHECK(o.zstd.content_size_flag == fresh.zstd.content_size_flag);
    D_CO_CHECK(o.zstd.dict_id_flag == fresh.zstd.dict_id_flag);
    D_CO_CHECK(o.zstd.strategy == fresh.zstd.strategy);

    // and so is every other block
    D_CO_CHECK(dt::deflate_options_equal(fresh.deflate, o.deflate));
    D_CO_CHECK(dt::bzip2_options_equal(fresh.bzip2, o.bzip2));
    D_CO_CHECK(dt::lzma_options_equal(fresh.lzma, o.lzma));
    D_CO_CHECK(dt::lz4_options_equal(fresh.lz4, o.lz4));
    D_CO_CHECK(dt::brotli_options_equal(fresh.brotli, o.brotli));

    return true;
}

/*
tests_compress_options_zstd_vs_generic_level
  the pairing the two presets exist to test.
  Tests the following:
  - the same effort through the two builders produces different option sets
  - each names a different field in its diff
  - a set carrying both efforts names both fields
*/
bool
tests_compress_options_zstd_vs_generic_level()
{
    const dj::compress_options generic = dt::compress_level_options(9);
    const dj::compress_options zstd    = dt::zstd_level_options(9);

    D_CO_CHECK(!dt::compress_options_equal(generic, zstd));
    D_CO_CHECK(dt::describe_compress_diff_from_default(generic) == "level");
    D_CO_CHECK(dt::describe_compress_diff_from_default(zstd) == "zstd.level");

    // the generic builder leaves zstd.level alone and vice versa
    D_CO_CHECK(generic.zstd.level != 9);
    D_CO_CHECK(zstd.level != 9);

    // the two are visible to different comparators
    const dj::compress_options fresh;

    D_CO_CHECK(dt::zstd_options_equal(fresh.zstd, generic.zstd));
    D_CO_CHECK(!dt::zstd_options_equal(fresh.zstd, zstd.zstd));

    // a set carrying both names both fields
    dj::compress_options both = dt::compress_level_options(9);

    both.zstd.level = 9;

    D_CO_CHECK(dt::describe_compress_diff_from_default(both) ==
               "level, zstd.level");
    D_CO_CHECK(!dt::compress_options_equal(both, generic));
    D_CO_CHECK(!dt::compress_options_equal(both, zstd));

    return true;
}

NS_END  // testing
NS_END  // djinterp
