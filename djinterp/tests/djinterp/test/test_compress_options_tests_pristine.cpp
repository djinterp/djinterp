#include "test_compress_options_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_compress_options_default_is_pristine
  the baseline itself.
  Tests the following:
  - a default-constructed set reports as pristine
  - so does the module's own factory result
  - a pristine set carries an empty diff against default
*/
bool
tests_compress_options_default_is_pristine()
{
    const dj::compress_options a;

    D_CO_CHECK(dt::compress_options_are_default(a));
    D_CO_CHECK(dt::compress_options_are_default(
                   dt::default_compress_options()));
    D_CO_CHECK(dt::describe_compress_diff_from_default(a).empty());

    // the documented defaults themselves, spot-checked where they are not
    // simply zero - these are the values every mutator moves away from
    D_CO_CHECK(a.level == -1);
    D_CO_CHECK(a.lzma.lc == -1);
    D_CO_CHECK(a.lzma.lp == -1);
    D_CO_CHECK(a.lzma.pb == -1);
    D_CO_CHECK(a.zstd.content_size_flag);
    D_CO_CHECK(a.zstd.dict_id_flag);
    D_CO_CHECK(!a.zstd.checksum_flag);
    D_CO_CHECK(a.brotli.quality == -1);

    return true;
}

/*
tests_compress_options_default_returns_fresh_value
  the factory's return discipline.
  Tests the following:
  - default_compress_options returns by value, so mutating one result cannot
    poison a later call
  - two successive calls are equal to each other
*/
bool
tests_compress_options_default_returns_fresh_value()
{
    dj::compress_options first = dt::default_compress_options();

    D_CO_CHECK(dt::compress_options_are_default(first));

    // poison the returned value
    first.level          = 9;
    first.zstd.workers   = 4;
    first.brotli.quality = 11;

    D_CO_CHECK(!dt::compress_options_are_default(first));

    // a later call is unaffected
    const dj::compress_options second = dt::default_compress_options();

    D_CO_CHECK(dt::compress_options_are_default(second));
    D_CO_CHECK(!dt::compress_options_equal(first, second));

    // and two successive calls agree with each other
    D_CO_CHECK(dt::compress_options_equal(dt::default_compress_options(),
                                          dt::default_compress_options()));

    return true;
}

/*
tests_compress_options_are_default_rejects_all
  the predicate's coverage.
  Tests the following:
  - moving any one of the fifty fields makes the pristine predicate report
    false, so no field can drift unnoticed
  - the predicate agrees with an explicit comparison against a fresh set
*/
bool
tests_compress_options_are_default_rejects_all()
{
    const dj::compress_options  fresh;
    const std::vector<co_field> t = co_all_mutators();
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::compress_options moved = co_mutated(i);

        if (dt::compress_options_are_default(moved))
        {
            std::printf("    [FAIL] pristine predicate missed field %s\n",
                        t[i].label);

            return false;
        }

        // the predicate is exactly the comparison against a fresh set
        D_CO_CHECK(dt::compress_options_are_default(moved) ==
                   dt::compress_options_equal(moved, fresh));
    }

    return true;
}

/*
tests_compress_options_are_default_after_revert
  returning to pristine.
  Tests the following:
  - a set moved away from default and then moved back reports pristine again
  - this holds for a numeric field, a flag and an enumerated field alike
  - a set carrying every field moved is not pristine, and assigning a fresh
    set over it restores the verdict
*/
bool
tests_compress_options_are_default_after_revert()
{
    const dj::compress_options  fresh;
    const std::vector<co_field> t = co_all_mutators();
    std::size_t                 i = 0;

    // a numeric field
    dj::compress_options n;

    n.zstd.window_log = 20;
    D_CO_CHECK(!dt::compress_options_are_default(n));
    n.zstd.window_log = fresh.zstd.window_log;
    D_CO_CHECK(dt::compress_options_are_default(n));

    // a flag, including one whose default is true
    dj::compress_options f;

    f.zstd.dict_id_flag = !fresh.zstd.dict_id_flag;
    D_CO_CHECK(!dt::compress_options_are_default(f));
    f.zstd.dict_id_flag = fresh.zstd.dict_id_flag;
    D_CO_CHECK(dt::compress_options_are_default(f));

    // an enumerated field
    dj::compress_options e;

    e.deflate.strategy = co_bump(fresh.deflate.strategy);
    D_CO_CHECK(!dt::compress_options_are_default(e));
    e.deflate.strategy = fresh.deflate.strategy;
    D_CO_CHECK(dt::compress_options_are_default(e));

    // everything moved at once, then wholesale reset
    dj::compress_options all;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(all);
    }

    D_CO_CHECK(!dt::compress_options_are_default(all));
    all = dj::compress_options();
    D_CO_CHECK(dt::compress_options_are_default(all));

    return true;
}

/*
tests_compress_options_mutator_table_is_complete
  the suite's own coverage pin.
  Tests the following:
  - the table carries exactly fifty rows: the generic level plus the
    forty-nine advanced knobs the module documents
  - every label is unique, so no field is covered twice while another is
    covered not at all
  - the per-block counts add up to the documented block sizes
*/
bool
tests_compress_options_mutator_table_is_complete()
{
    const std::vector<co_field> t = co_all_mutators();
    std::size_t                 i = 0;
    std::size_t                 j = 0;

    // the documented shape of the vocabulary
    D_CO_CHECK(t.size() == 50u);

    // every label is unique
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

    // the per-block tallies match the module's documentation
    std::size_t generic = 0;
    std::size_t deflate = 0;
    std::size_t bzip2   = 0;
    std::size_t lzma    = 0;
    std::size_t zstd    = 0;
    std::size_t lz4     = 0;
    std::size_t brotli  = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const std::string b(t[i].block);

        if (b.empty())          { ++generic; }
        else if (b == "deflate") { ++deflate; }
        else if (b == "bzip2")   { ++bzip2; }
        else if (b == "lzma")    { ++lzma; }
        else if (b == "zstd")    { ++zstd; }
        else if (b == "lz4")     { ++lz4; }
        else if (b == "brotli")  { ++brotli; }
    }

    D_CO_CHECK(generic == 1u);
    D_CO_CHECK(deflate == 3u);
    D_CO_CHECK(bzip2 == 4u);
    D_CO_CHECK(lzma == 11u);
    D_CO_CHECK(zstd == 19u);
    D_CO_CHECK(lz4 == 7u);
    D_CO_CHECK(brotli == 5u);

    // 49 advanced knobs on top of the generic level
    D_CO_CHECK((deflate + bzip2 + lzma + zstd + lz4 + brotli) == 49u);

    // and every row's mutator really does move something
    for (i = 0; i < t.size(); ++i)
    {
        D_CO_CHECK(!dt::compress_options_are_default(co_mutated(i)));
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
