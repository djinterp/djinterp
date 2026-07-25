#include "test_archive_options_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_archive_options_default_is_pristine
  the baseline itself.
  Tests the following:
  - a default-constructed set reports pristine, as does the factory result
  - its embedded codec reports pristine too
  - the diff against default is empty
  - the documented non-zero defaults are the values every mutator moves from
*/
bool
tests_archive_options_default_is_pristine()
{
    const dj::archive_options a;

    D_AO_CHECK(dt::options_are_default(a));
    D_AO_CHECK(dt::options_are_default(dt::default_archive_options()));
    D_AO_CHECK(dt::codec_is_default(a));
    D_AO_CHECK(dt::describe_diff_from_default(a).empty());

    // the defaults that are not simply zero or empty
    D_AO_CHECK(a.level == -1);
    D_AO_CHECK(!a.store_only);
    D_AO_CHECK(a.preserve_permissions);
    D_AO_CHECK(a.preserve_mtime);
    D_AO_CHECK(a.zip.utf8_names);
    D_AO_CHECK(!a.tar.numeric_owner);
    D_AO_CHECK(!a.gz.store_name);
    D_AO_CHECK(a.gz.store_mtime);
    D_AO_CHECK(a.sevenzip.solid);
    D_AO_CHECK(a.sevenzip.header_compression);
    D_AO_CHECK(!a.sevenzip.header_encryption);
    D_AO_CHECK(a.sevenzip.threads == 0);
    D_AO_CHECK(a.rar.level == -1);
    D_AO_CHECK(!a.rar.solid);
    D_AO_CHECK(!a.rar.recovery_record);

    // and the embedded codec carries the compress vocabulary's own defaults
    D_AO_CHECK(a.codec.level == -1);

    return true;
}

/*
tests_archive_options_default_returns_fresh_value
  the factory's return discipline.
  Tests the following:
  - default_archive_options returns by value, so mutating one result cannot
    poison a later call
  - two successive calls are equal to each other
*/
bool
tests_archive_options_default_returns_fresh_value()
{
    dj::archive_options first = dt::default_archive_options();

    D_AO_CHECK(dt::options_are_default(first));

    first.store_only     = true;
    first.comment        = "poisoned";
    first.codec.level    = 9;
    first.zip.password   = "pw";

    D_AO_CHECK(!dt::options_are_default(first));
    D_AO_CHECK(!dt::codec_is_default(first));

    const dj::archive_options second = dt::default_archive_options();

    D_AO_CHECK(dt::options_are_default(second));
    D_AO_CHECK(dt::codec_is_default(second));
    D_AO_CHECK(!dt::archive_options_equal(first, second));

    D_AO_CHECK(dt::archive_options_equal(dt::default_archive_options(),
                                         dt::default_archive_options()));

    return true;
}

/*
tests_archive_options_are_default_rejects_all
  the whole-tree predicate's coverage.
  Tests the following:
  - moving any field in the table makes the pristine predicate report false
  - the predicate is exactly the comparison against a freshly constructed set
  - a set moved and then reset reports pristine again
*/
bool
tests_archive_options_are_default_rejects_all()
{
    const dj::archive_options   fresh;
    const std::vector<ao_field> t = ao_all_mutators();
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::archive_options moved = ao_mutated(i);

        if (dt::options_are_default(moved))
        {
            std::printf("    [FAIL] pristine predicate missed field %s\n",
                        t[i].label);

            return false;
        }

        D_AO_CHECK(dt::options_are_default(moved) ==
                   dt::archive_options_equal(moved, fresh));
    }

    // wholesale reset restores the verdict
    dj::archive_options all;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(all);
    }

    D_AO_CHECK(!dt::options_are_default(all));
    all = dj::archive_options();
    D_AO_CHECK(dt::options_are_default(all));

    return true;
}

/*
tests_archive_options_codec_default_tracks
  the predicate a container test leans on.
  Tests the following:
  - every archive-level and per-format change leaves codec_is_default true,
    which is the whole point of having a codec-only predicate
  - a set with every container knob moved still reports its codec pristine
*/
bool
tests_archive_options_codec_default_tracks()
{
    const std::vector<ao_field> t = ao_all_mutators();
    dj::archive_options         container;
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const std::string owner(t[i].block);

        if (owner == "codec")
        {
            continue;
        }

        // a non-codec change must not disturb the codec predicate
        const dj::archive_options moved = ao_mutated(i);

        if (!dt::codec_is_default(moved))
        {
            std::printf("    [FAIL] codec predicate reacted to %s\n",
                        t[i].label);

            return false;
        }

        // accumulate them all into one set
        t[i].apply(container);
    }

    // every container knob moved, codec still pristine
    D_AO_CHECK(!dt::options_are_default(container));
    D_AO_CHECK(dt::codec_is_default(container));
    D_AO_CHECK(dt::compress_options_equal(container.codec,
                                          dj::compress_options()));

    return true;
}

/*
tests_archive_options_codec_default_rejects
  the other half of the predicate's contract.
  Tests the following:
  - a change anywhere in the embedded codec makes codec_is_default false
  - this holds across all six per-codec blocks and for codec.level
  - the predicate agrees with an explicit compress comparison
*/
bool
tests_archive_options_codec_default_rejects()
{
    const std::vector<ao_codec_field> c = ao_codec_fields();
    std::size_t                       i = 0;

    for (i = 0; i < c.size(); ++i)
    {
        dj::archive_options moved;

        c[i].apply(moved.codec);

        if (dt::codec_is_default(moved))
        {
            std::printf("    [FAIL] codec predicate missed a %s field\n",
                        c[i].coarse_label);

            return false;
        }

        // the predicate is exactly the compress comparison
        D_AO_CHECK(dt::codec_is_default(moved) ==
                   dt::compress_options_equal(moved.codec,
                                              dj::compress_options()));
    }

    // codec.level too
    dj::archive_options lv;

    lv.codec.level = 9;
    D_AO_CHECK(!dt::codec_is_default(lv));
    D_AO_CHECK(!dt::options_are_default(lv));

    // and a codec change is enough to fail the whole-tree predicate as well
    D_AO_CHECK(!dt::options_are_default(lv));

    return true;
}

/*
tests_archive_options_mutator_table_is_complete
  the suite's own coverage pin.
  Tests the following:
  - the table carries thirty-two rows: five archive-level, seven coarse
    codec labels and twenty per-format fields
  - every label is unique
  - every row's mutator really moves the set away from pristine
*/
bool
tests_archive_options_mutator_table_is_complete()
{
    const std::vector<ao_field> t = ao_all_mutators();
    std::size_t                 i = 0;
    std::size_t                 j = 0;

    D_AO_CHECK(t.size() == 32u);

    // unique labels
    for (i = 0; i < t.size(); ++i)
    {
        for (j = (i + 1u); j < t.size(); ++j)
        {
            if (std::string(t[i].label) == std::string(t[j].label))
            {
                std::printf("    [FAIL] duplicate table label %s\n",
                            t[i].label);

                return false;
            }
        }
    }

    // the documented per-block tallies
    std::size_t arch  = 0;
    std::size_t codec = 0;
    std::size_t zip   = 0;
    std::size_t tar   = 0;
    std::size_t gz    = 0;
    std::size_t szip  = 0;
    std::size_t rar   = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const std::string b(t[i].block);

        if (b == "archive")       { ++arch; }
        else if (b == "codec")    { ++codec; }
        else if (b == "zip")      { ++zip; }
        else if (b == "tar")      { ++tar; }
        else if (b == "gz")       { ++gz; }
        else if (b == "sevenzip") { ++szip; }
        else if (b == "rar")      { ++rar; }
    }

    D_AO_CHECK(arch == 5u);
    D_AO_CHECK(codec == 7u);
    D_AO_CHECK(zip == 5u);
    D_AO_CHECK(tar == 2u);
    D_AO_CHECK(gz == 3u);
    D_AO_CHECK(szip == 6u);
    D_AO_CHECK(rar == 4u);

    // twenty per-format fields across the five blocks
    D_AO_CHECK((zip + tar + gz + szip + rar) == 20u);

    // every mutator moves something
    for (i = 0; i < t.size(); ++i)
    {
        D_AO_CHECK(!dt::options_are_default(ao_mutated(i)));
    }

    // the codec sweep spreads over all six blocks
    const std::vector<ao_codec_field> c = ao_codec_fields();

    D_AO_CHECK(c.size() == 15u);

    return true;
}

NS_END  // testing
NS_END  // djinterp
