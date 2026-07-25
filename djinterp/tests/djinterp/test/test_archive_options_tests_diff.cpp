#include "test_archive_options_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_archive_options_diff_empty_when_equal
  the no-change case.
  Tests the following:
  - two identical sets produce an empty diff string, not a placeholder
  - a set compared with itself is empty
  - a thoroughly non-default set compared with its own copy is empty
*/
bool
tests_archive_options_diff_empty_when_equal()
{
    const dj::archive_options   a;
    const dj::archive_options   b;
    const std::vector<ao_field> t = ao_all_mutators();
    std::size_t                 i = 0;

    D_AO_CHECK(dt::describe_option_diff(a, a).empty());
    D_AO_CHECK(dt::describe_option_diff(a, b).empty());

    dj::archive_options all;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(all);
    }

    const dj::archive_options copy = all;

    D_AO_CHECK(dt::describe_option_diff(all, copy).empty());
    D_AO_CHECK(dt::archive_options_equal(all, copy));

    return true;
}

/*
tests_archive_options_diff_names_every_field
  the label emitted for each field.
  Tests the following:
  - each single change produces exactly the documented label
  - the result is a single entry, so no unrelated field is named alongside
  - all thirty-two table rows are covered
*/
bool
tests_archive_options_diff_names_every_field()
{
    const dj::archive_options   fresh;
    const std::vector<ao_field> t = ao_all_mutators();
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::archive_options      moved = ao_mutated(i);
        const std::string              d =
            dt::describe_option_diff(fresh, moved);
        const std::vector<std::string> parts = ao_split_csv(d);

        if ( (parts.size() != 1u) ||
             (parts[0] != std::string(t[i].label)) )
        {
            std::printf("    [FAIL] field %s produced diff [%s]\n",
                        t[i].label, d.c_str());

            return false;
        }
    }

    return true;
}

/*
tests_archive_options_diff_codec_is_block_granular
  the deliberate coarseness.
  Tests the following:
  - a knob inside a per-codec block collapses onto that block's label
  - it never escapes as a field name, so a caller tuning a container is not
    shown which of nineteen zstd knobs moved
  - two different knobs of the same block produce the SAME label
*/
bool
tests_archive_options_diff_codec_is_block_granular()
{
    const dj::archive_options         fresh;
    const std::vector<ao_codec_field> c = ao_codec_fields();
    std::size_t                       i = 0;

    for (i = 0; i < c.size(); ++i)
    {
        dj::archive_options moved;

        c[i].apply(moved.codec);

        const std::string d = dt::describe_option_diff(fresh, moved);

        if (d != std::string(c[i].coarse_label))
        {
            std::printf("    [FAIL] codec field gave [%s], want [%s]\n",
                        d.c_str(), c[i].coarse_label);

            return false;
        }

        // exactly one label, and it names the block rather than the field
        D_AO_CHECK(ao_split_csv(d).size() == 1u);
    }

    // two different zstd knobs both report "codec.zstd"
    dj::archive_options z1;
    dj::archive_options z2;

    z1.codec.zstd.window_log = 21;
    z2.codec.zstd.workers    = 4;

    D_AO_CHECK(dt::describe_option_diff(fresh, z1) == "codec.zstd");
    D_AO_CHECK(dt::describe_option_diff(fresh, z2) == "codec.zstd");

    // and a set moving both still names the block once
    dj::archive_options both;

    both.codec.zstd.window_log = 21;
    both.codec.zstd.workers    = 4;

    D_AO_CHECK(dt::describe_option_diff(fresh, both) == "codec.zstd");

    // the field name itself never appears
    D_AO_CHECK(!ao_contains(dt::describe_option_diff(fresh, both),
                            "window_log"));
    D_AO_CHECK(!ao_contains(dt::describe_option_diff(fresh, both),
                            "workers"));

    return true;
}

/*
tests_archive_options_diff_codec_level_is_named
  the one codec field reported individually.
  Tests the following:
  - codec.level is called out by name, unlike the blocks around it
  - it is distinct from the archive-level `level`
  - a set moving both names both, in the module's order
*/
bool
tests_archive_options_diff_codec_level_is_named()
{
    const dj::archive_options fresh;

    dj::archive_options cl;
    dj::archive_options al;

    cl.codec.level = 9;
    al.level       = 9;

    D_AO_CHECK(dt::describe_option_diff(fresh, cl) == "codec.level");
    D_AO_CHECK(dt::describe_option_diff(fresh, al) == "level");

    // the two labels are different strings, so a caller can tell which
    // effort moved
    D_AO_CHECK(dt::describe_option_diff(fresh, cl) !=
               dt::describe_option_diff(fresh, al));

    // both at once, in declaration order (archive level first)
    dj::archive_options both;

    both.level       = 9;
    both.codec.level = 9;

    D_AO_CHECK(dt::describe_option_diff(fresh, both) == "level, codec.level");

    // codec.level does not collapse into any block label
    D_AO_CHECK(!ao_contains(dt::describe_option_diff(fresh, cl),
                            "codec.zstd"));
    D_AO_CHECK(!ao_contains(dt::describe_option_diff(fresh, cl),
                            "codec.deflate"));

    return true;
}

/*
tests_archive_options_diff_separator_and_order
  the shape of a multi-field list.
  Tests the following:
  - several changed fields are joined by a comma and a space
  - the list follows the module's declaration order, not the assignment
    order: archive level, then codec, then zip / tar / gz / 7z / rar
  - there is no leading, trailing or doubled separator
*/
bool
tests_archive_options_diff_separator_and_order()
{
    const dj::archive_options fresh;
    dj::archive_options       o;

    // assign in reverse declaration order
    o.rar.solid          = !fresh.rar.solid;
    o.sevenzip.threads   = 4;
    o.gz.store_name      = !fresh.gz.store_name;
    o.tar.numeric_owner  = !fresh.tar.numeric_owner;
    o.zip.utf8_names     = !fresh.zip.utf8_names;
    o.codec.zstd.workers = 2;
    o.codec.level        = 3;
    o.store_only         = !fresh.store_only;

    const std::string d = dt::describe_option_diff(fresh, o);

    D_AO_CHECK(d == "store_only, codec.level, codec.zstd, zip.utf8_names, "
                    "tar.numeric_owner, gz.store_name, sevenzip.threads, "
                    "rar.solid");

    const std::vector<std::string> parts = ao_split_csv(d);

    D_AO_CHECK(parts.size() == 8u);
    D_AO_CHECK(parts[0] == "store_only");
    D_AO_CHECK(parts[1] == "codec.level");
    D_AO_CHECK(parts[2] == "codec.zstd");
    D_AO_CHECK(parts[7] == "rar.solid");

    // no stray separators
    D_AO_CHECK(d[0] != ',');
    D_AO_CHECK(d[0] != ' ');
    D_AO_CHECK(d[d.size() - 1u] != ',');
    D_AO_CHECK(d[d.size() - 1u] != ' ');
    D_AO_CHECK(!ao_contains(d, ",,"));
    D_AO_CHECK(!ao_contains(d, "  "));

    // two fields produce exactly one separator
    dj::archive_options two;

    two.level      = 1;
    two.rar.solid  = true;
    D_AO_CHECK(ao_split_csv(
                   dt::describe_option_diff(fresh, two)).size() == 2u);

    return true;
}

/*
tests_archive_options_diff_lists_all_fields_at_once
  the maximal diff.
  Tests the following:
  - a set with every table field moved names all thirty-two
  - each label appears exactly once
  - the emitted order matches the table's order end to end
*/
bool
tests_archive_options_diff_lists_all_fields_at_once()
{
    const dj::archive_options   fresh;
    const std::vector<ao_field> t = ao_all_mutators();
    dj::archive_options         all;
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(all);
    }

    const std::string              d = dt::describe_option_diff(fresh, all);
    const std::vector<std::string> parts = ao_split_csv(d);

    D_AO_CHECK(parts.size() == 32u);
    D_AO_CHECK(parts.size() == t.size());

    for (i = 0; i < t.size(); ++i)
    {
        if (parts[i] != std::string(t[i].label))
        {
            std::printf("    [FAIL] position %u: got [%s], expected [%s]\n",
                        static_cast<unsigned int>(i),
                        parts[i].c_str(), t[i].label);

            return false;
        }
    }

    return true;
}

/*
tests_archive_options_diff_is_symmetric
  operand order.
  Tests the following:
  - the diff names the same fields whichever way round the two sets are
    passed, since it reports inequality rather than direction
  - this holds for every single change and for the maximal one
*/
bool
tests_archive_options_diff_is_symmetric()
{
    const dj::archive_options   fresh;
    const std::vector<ao_field> t = ao_all_mutators();
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::archive_options moved = ao_mutated(i);

        D_AO_CHECK(dt::describe_option_diff(fresh, moved) ==
                   dt::describe_option_diff(moved, fresh));
    }

    dj::archive_options all;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(all);
    }

    D_AO_CHECK(dt::describe_option_diff(fresh, all) ==
               dt::describe_option_diff(all, fresh));

    return true;
}

/*
tests_archive_options_diff_from_default_matches
  the against-pristine convenience.
  Tests the following:
  - describe_diff_from_default agrees with an explicit comparison against a
    freshly constructed set, for every field in the table
  - a pristine set reports an empty diff
  - the empty diff and the pristine predicate always agree
*/
bool
tests_archive_options_diff_from_default_matches()
{
    const dj::archive_options   fresh;
    const std::vector<ao_field> t = ao_all_mutators();
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::archive_options moved = ao_mutated(i);

        D_AO_CHECK(dt::describe_diff_from_default(moved) ==
                   dt::describe_option_diff(moved, fresh));
        D_AO_CHECK(dt::describe_diff_from_default(moved) ==
                   std::string(t[i].label));

        // the diff and the predicate agree
        D_AO_CHECK(dt::describe_diff_from_default(moved).empty() ==
                   dt::options_are_default(moved));
    }

    D_AO_CHECK(dt::describe_diff_from_default(fresh).empty());
    D_AO_CHECK(dt::describe_diff_from_default(
                   dt::default_archive_options()).empty());

    return true;
}

/*
tests_archive_options_diff_delegates
  the two granularities side by side.
  Tests the following:
  - for the same codec change, the archive view names the block while the
    compress view names the field
  - the block named by the coarse view is the prefix of the field named by
    the fine one, so the two agree on WHICH block moved
  - the module's documented escalation path really does give more detail
*/
bool
tests_archive_options_diff_delegates()
{
    const dj::archive_options         fresh;
    const std::vector<ao_codec_field> c = ao_codec_fields();
    std::size_t                       i = 0;

    for (i = 0; i < c.size(); ++i)
    {
        dj::archive_options moved;

        c[i].apply(moved.codec);

        const std::string coarse = dt::describe_option_diff(fresh, moved);
        const std::string fine   =
            dt::describe_compress_option_diff(moved.codec, fresh.codec);

        // the coarse view names the block
        D_AO_CHECK(coarse == std::string(c[i].coarse_label));

        // the fine view names a field, which is strictly more detail
        D_AO_CHECK(!fine.empty());
        D_AO_CHECK(fine.length() > 0u);

        // and they agree on the block: "codec.zstd" -> "zstd."
        const std::string block = coarse.substr(6);   // drop "codec."

        if (fine.compare(0, block.size(), block) != 0)
        {
            std::printf("    [FAIL] coarse [%s] and fine [%s] disagree\n",
                        coarse.c_str(), fine.c_str());

            return false;
        }
    }

    // codec.level is the one field both views name the same way, modulo the
    // archive view's qualifier
    dj::archive_options lv;

    lv.codec.level = 9;

    D_AO_CHECK(dt::describe_option_diff(fresh, lv) == "codec.level");
    D_AO_CHECK(dt::describe_compress_option_diff(lv.codec, fresh.codec) ==
               "level");

    return true;
}

NS_END  // testing
NS_END  // djinterp
