#include "test_archive_options_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_archive_options_equal_reflexive
  the baseline the aggregate comparator must satisfy.
  Tests the following:
  - a set equals itself
  - two independently default-constructed sets are equal
  - the module's own default_archive_options agrees with a locally
    constructed set
*/
bool
tests_archive_options_equal_reflexive()
{
    const dj::archive_options a;
    const dj::archive_options b;

    D_AO_CHECK(dt::archive_options_equal(a, a));
    D_AO_CHECK(dt::archive_options_equal(a, b));
    D_AO_CHECK(dt::archive_options_equal(b, a));
    D_AO_CHECK(dt::archive_options_equal(a, dt::default_archive_options()));

    return true;
}

/*
tests_archive_options_equal_symmetric
  operand order.
  Tests the following:
  - the comparison gives the same answer whichever way round its operands
    are passed, for every change in the table
  - this holds for the equal case as well as the unequal one
*/
bool
tests_archive_options_equal_symmetric()
{
    const dj::archive_options   fresh;
    const std::vector<ao_field> t = ao_all_mutators();
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::archive_options moved = ao_mutated(i);

        D_AO_CHECK(dt::archive_options_equal(fresh, moved) ==
                   dt::archive_options_equal(moved, fresh));
        D_AO_CHECK(!dt::archive_options_equal(fresh, moved));
        D_AO_CHECK(!dt::archive_options_equal(moved, fresh));

        const dj::archive_options copy = moved;

        D_AO_CHECK(dt::archive_options_equal(moved, copy));
        D_AO_CHECK(dt::archive_options_equal(copy, moved));
    }

    return true;
}

/*
tests_archive_options_equal_all_own_fields
  whole-aggregate coverage.
  Tests the following:
  - every field in the table is reachable by the aggregate comparator, so
    nothing is dropped between the blocks and the roll-up
  - the five archive-level fields are compared in their own right, not only
    through a block
*/
bool
tests_archive_options_equal_all_own_fields()
{
    const dj::archive_options   fresh;
    const std::vector<ao_field> t = ao_all_mutators();
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::archive_options moved = ao_mutated(i);

        if (dt::archive_options_equal(fresh, moved))
        {
            std::printf("    [FAIL] aggregate missed field %s\n", t[i].label);

            return false;
        }
    }

    // the archive-level fields, named explicitly
    dj::archive_options lv;
    dj::archive_options so;
    dj::archive_options cm;
    dj::archive_options pp;
    dj::archive_options pm;

    lv.level                = (fresh.level + 1);
    so.store_only           = !fresh.store_only;
    cm.comment              = "a comment";
    pp.preserve_permissions = !fresh.preserve_permissions;
    pm.preserve_mtime       = !fresh.preserve_mtime;

    D_AO_CHECK(!dt::archive_options_equal(fresh, lv));
    D_AO_CHECK(!dt::archive_options_equal(fresh, so));
    D_AO_CHECK(!dt::archive_options_equal(fresh, cm));
    D_AO_CHECK(!dt::archive_options_equal(fresh, pp));
    D_AO_CHECK(!dt::archive_options_equal(fresh, pm));

    // none of them disturbed a per-format block
    D_AO_CHECK(dt::zip_options_equal(fresh.zip, lv.zip));
    D_AO_CHECK(dt::tar_options_equal(fresh.tar, so.tar));
    D_AO_CHECK(dt::gz_options_equal(fresh.gz, cm.gz));

    return true;
}

/*
tests_archive_options_equal_detects_embedded_codec
  the nested compress_options.
  Tests the following:
  - a change anywhere inside the embedded codec is detected, however deeply
    nested
  - the aggregate reaches it through the compress comparator, so the two
    agree on every codec sub-field the sweep touches
  - a codec change disturbs no per-format block
*/
bool
tests_archive_options_equal_detects_embedded_codec()
{
    const dj::archive_options         fresh;
    const std::vector<ao_codec_field> c = ao_codec_fields();
    std::size_t                       i = 0;

    for (i = 0; i < c.size(); ++i)
    {
        dj::archive_options moved;

        c[i].apply(moved.codec);

        // the aggregate sees it
        if (dt::archive_options_equal(fresh, moved))
        {
            std::printf("    [FAIL] aggregate missed codec field for %s\n",
                        c[i].coarse_label);

            return false;
        }

        // and so does the compress comparator it delegates to
        D_AO_CHECK(!dt::compress_options_equal(fresh.codec, moved.codec));

        // while every per-format block is undisturbed
        D_AO_CHECK(dt::zip_options_equal(fresh.zip, moved.zip));
        D_AO_CHECK(dt::tar_options_equal(fresh.tar, moved.tar));
        D_AO_CHECK(dt::gz_options_equal(fresh.gz, moved.gz));
        D_AO_CHECK(dt::sevenzip_options_equal(fresh.sevenzip,
                                              moved.sevenzip));
        D_AO_CHECK(dt::rar_options_equal(fresh.rar, moved.rar));
    }

    // the codec's own level is distinct from the archive-level one
    dj::archive_options arch;
    dj::archive_options codec;

    arch.level        = 9;
    codec.codec.level = 9;

    D_AO_CHECK(!dt::archive_options_equal(arch, codec));
    D_AO_CHECK(!dt::archive_options_equal(fresh, arch));
    D_AO_CHECK(!dt::archive_options_equal(fresh, codec));

    return true;
}

/*
tests_archive_options_equal_multi_change
  several fields at once.
  Tests the following:
  - simultaneous changes across different blocks are detected
  - undoing all but one is not enough to regain equality
  - a set with every table field moved is unequal, and every block
    comparator disagrees on it
*/
bool
tests_archive_options_equal_multi_change()
{
    const dj::archive_options   fresh;
    const std::vector<ao_field> t = ao_all_mutators();
    dj::archive_options         many;
    std::size_t                 i = 0;

    many.store_only        = !fresh.store_only;
    many.zip.utf8_names    = !fresh.zip.utf8_names;
    many.rar.solid         = !fresh.rar.solid;
    many.codec.zstd.workers = (fresh.codec.zstd.workers + 1);

    D_AO_CHECK(!dt::archive_options_equal(fresh, many));

    // undo three of the four; still unequal
    many.zip.utf8_names     = fresh.zip.utf8_names;
    many.rar.solid          = fresh.rar.solid;
    many.codec.zstd.workers = fresh.codec.zstd.workers;
    D_AO_CHECK(!dt::archive_options_equal(fresh, many));

    // undo the last; equal again
    many.store_only = fresh.store_only;
    D_AO_CHECK(dt::archive_options_equal(fresh, many));

    // everything moved at once
    dj::archive_options all;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(all);
    }

    D_AO_CHECK(!dt::archive_options_equal(fresh, all));
    D_AO_CHECK(!dt::zip_options_equal(fresh.zip, all.zip));
    D_AO_CHECK(!dt::tar_options_equal(fresh.tar, all.tar));
    D_AO_CHECK(!dt::gz_options_equal(fresh.gz, all.gz));
    D_AO_CHECK(!dt::sevenzip_options_equal(fresh.sevenzip, all.sevenzip));
    D_AO_CHECK(!dt::rar_options_equal(fresh.rar, all.rar));
    D_AO_CHECK(!dt::compress_options_equal(fresh.codec, all.codec));

    return true;
}

/*
tests_archive_options_equal_survives_copy
  value semantics.
  Tests the following:
  - a copy-constructed set compares equal to its source
  - an assigned set compares equal to its source
  - a later change to the source does not follow either copy, including a
    change inside the embedded codec or a string field
*/
bool
tests_archive_options_equal_survives_copy()
{
    const std::vector<ao_field> t = ao_all_mutators();
    dj::archive_options         src;
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(src);
    }

    const dj::archive_options copied(src);
    dj::archive_options       assigned;

    assigned = src;

    D_AO_CHECK(dt::archive_options_equal(src, copied));
    D_AO_CHECK(dt::archive_options_equal(src, assigned));

    // a later change to the source does not follow the copies
    src.zip.password += "more";
    D_AO_CHECK(!dt::archive_options_equal(src, copied));
    D_AO_CHECK(!dt::archive_options_equal(src, assigned));
    D_AO_CHECK(dt::archive_options_equal(copied, assigned));

    // nor does one buried in the embedded codec
    dj::archive_options src2  = copied;
    const dj::archive_options snap = src2;

    src2.codec.lzma.dict_size = (src2.codec.lzma.dict_size + 1u);
    D_AO_CHECK(!dt::archive_options_equal(src2, snap));
    D_AO_CHECK(dt::archive_options_equal(snap, copied));

    return true;
}

NS_END  // testing
NS_END  // djinterp
