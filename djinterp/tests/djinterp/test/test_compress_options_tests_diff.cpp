#include "test_compress_options_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_compress_options_diff_empty_when_equal
  the no-change case.
  Tests the following:
  - two identical sets produce an empty diff string, not a placeholder
  - a set compared with itself is empty
  - two independently constructed pristine sets are empty
  - a thoroughly non-default set compared with its own copy is empty
*/
bool
tests_compress_options_diff_empty_when_equal()
{
    const dj::compress_options  a;
    const dj::compress_options  b;
    const std::vector<co_field> t = co_all_mutators();
    std::size_t                 i = 0;

    D_CO_CHECK(dt::describe_compress_option_diff(a, a).empty());
    D_CO_CHECK(dt::describe_compress_option_diff(a, b).empty());

    // a heavily modified set against its own copy
    dj::compress_options all;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(all);
    }

    const dj::compress_options copy = all;

    D_CO_CHECK(dt::describe_compress_option_diff(all, copy).empty());

    // an empty diff and an equal verdict always agree
    D_CO_CHECK(dt::compress_options_equal(all, copy));

    return true;
}

/*
tests_compress_options_diff_names_every_field
  the label emitted for each field.
  Tests the following:
  - each single-field change produces exactly that field's documented name
  - the result is a single entry, so no unrelated field is named alongside
  - every one of the fifty fields is covered
*/
bool
tests_compress_options_diff_names_every_field()
{
    const dj::compress_options  fresh;
    const std::vector<co_field> t = co_all_mutators();
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::compress_options     moved = co_mutated(i);
        const std::string              d =
            dt::describe_compress_option_diff(fresh, moved);
        const std::vector<std::string> parts = co_split_csv(d);

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
tests_compress_options_diff_is_field_level
  the granularity that gives this header its purpose.
  Tests the following:
  - a codec knob is named down to the field, qualified by its block
  - it is never collapsed to the bare block name, which is the archive-level
    view's job rather than this one's
  - the generic level is named without any block qualifier
*/
bool
tests_compress_options_diff_is_field_level()
{
    const dj::compress_options fresh;

    // a zstd knob names the field, not just the block
    dj::compress_options z;

    z.zstd.window_log = 21;

    const std::string zd = dt::describe_compress_option_diff(fresh, z);

    D_CO_CHECK(zd == "zstd.window_log");
    D_CO_CHECK(zd != "zstd");
    D_CO_CHECK(co_contains(zd, "."));

    // a second zstd knob names a different field, so the block is not the
    // unit of reporting
    dj::compress_options z2;

    z2.zstd.checksum_flag = !fresh.zstd.checksum_flag;
    D_CO_CHECK(dt::describe_compress_option_diff(fresh, z2) ==
               "zstd.checksum_flag");

    // the generic level carries no block qualifier
    dj::compress_options g;

    g.level = 5;
    D_CO_CHECK(dt::describe_compress_option_diff(fresh, g) == "level");

    // while the zstd block's own level is qualified
    dj::compress_options zl;

    zl.zstd.level = 5;
    D_CO_CHECK(dt::describe_compress_option_diff(fresh, zl) == "zstd.level");

    // every label in the table is qualified except the generic level
    const std::vector<co_field> t = co_all_mutators();
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const std::string label(t[i].label);
        const bool        qualified = (label.find('.') != std::string::npos);

        D_CO_CHECK(qualified == (std::string(t[i].block) != std::string()));
    }

    return true;
}

/*
tests_compress_options_diff_separator_and_order
  the shape of a multi-field list.
  Tests the following:
  - several changed fields are joined by a comma and a space
  - the list follows the module's declaration order, not the order the
    fields were assigned
  - there is no leading or trailing separator
*/
bool
tests_compress_options_diff_separator_and_order()
{
    const dj::compress_options fresh;
    dj::compress_options       o;

    // assign in reverse declaration order to prove the walk decides the
    // emitted order, not the caller
    o.brotli.large_window    = !fresh.brotli.large_window;
    o.lz4.level              = 3;
    o.zstd.level             = 7;
    o.deflate.window_bits    = 15;
    o.level                  = 9;

    const std::string d = dt::describe_compress_option_diff(fresh, o);

    D_CO_CHECK(d == "level, deflate.window_bits, zstd.level, lz4.level, "
                    "brotli.large_window");

    // exactly five entries, comma-and-space separated
    const std::vector<std::string> parts = co_split_csv(d);

    D_CO_CHECK(parts.size() == 5u);
    D_CO_CHECK(parts[0] == "level");
    D_CO_CHECK(parts[4] == "brotli.large_window");

    // no leading or trailing separator, and no double separator
    D_CO_CHECK(d[0] != ',');
    D_CO_CHECK(d[0] != ' ');
    D_CO_CHECK(d[d.size() - 1u] != ',');
    D_CO_CHECK(d[d.size() - 1u] != ' ');
    D_CO_CHECK(!co_contains(d, ",,"));
    D_CO_CHECK(!co_contains(d, "  "));

    // two fields produce exactly one separator
    dj::compress_options two;

    two.level      = 1;
    two.lzma.depth = 2;
    const std::string two_diff =
        dt::describe_compress_option_diff(fresh, two);

    D_CO_CHECK(co_split_csv(two_diff).size() == 2u);

    return true;
}

/*
tests_compress_options_diff_lists_all_fields_at_once
  the maximal diff.
  Tests the following:
  - a set with every field moved names all fifty
  - each label appears exactly once
  - the emitted order matches the table's order end to end
*/
bool
tests_compress_options_diff_lists_all_fields_at_once()
{
    const dj::compress_options  fresh;
    const std::vector<co_field> t = co_all_mutators();
    dj::compress_options        all;
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(all);
    }

    const std::string              d =
        dt::describe_compress_option_diff(fresh, all);
    const std::vector<std::string> parts = co_split_csv(d);

    D_CO_CHECK(parts.size() == 50u);
    D_CO_CHECK(parts.size() == t.size());

    // every label, once, in the table's order
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
tests_compress_options_diff_is_symmetric
  operand order.
  Tests the following:
  - the diff names the same fields whichever way round the two sets are
    passed, since it reports inequality rather than direction
  - this holds for a single change and for the maximal one
*/
bool
tests_compress_options_diff_is_symmetric()
{
    const dj::compress_options  fresh;
    const std::vector<co_field> t = co_all_mutators();
    std::size_t                 i = 0;

    // every single-field change reads the same both ways
    for (i = 0; i < t.size(); ++i)
    {
        const dj::compress_options moved = co_mutated(i);

        D_CO_CHECK(dt::describe_compress_option_diff(fresh, moved) ==
                   dt::describe_compress_option_diff(moved, fresh));
    }

    // and so does the maximal one
    dj::compress_options all;

    for (i = 0; i < t.size(); ++i)
    {
        t[i].apply(all);
    }

    D_CO_CHECK(dt::describe_compress_option_diff(fresh, all) ==
               dt::describe_compress_option_diff(all, fresh));

    return true;
}

/*
tests_compress_options_diff_from_default_matches
  the against-pristine convenience.
  Tests the following:
  - describe_compress_diff_from_default agrees with an explicit comparison
    against a freshly constructed set, for every field in the table
  - it names the same fields in the same order
*/
bool
tests_compress_options_diff_from_default_matches()
{
    const dj::compress_options  fresh;
    const std::vector<co_field> t = co_all_mutators();
    std::size_t                 i = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::compress_options moved = co_mutated(i);

        D_CO_CHECK(dt::describe_compress_diff_from_default(moved) ==
                   dt::describe_compress_option_diff(moved, fresh));
        D_CO_CHECK(dt::describe_compress_diff_from_default(moved) ==
                   std::string(t[i].label));
    }

    // and for a multi-field set
    dj::compress_options many;

    many.level        = 4;
    many.bzip2.verbosity = 2;
    many.lz4.block_checksum = true;

    D_CO_CHECK(dt::describe_compress_diff_from_default(many) ==
               dt::describe_compress_option_diff(many, fresh));
    D_CO_CHECK(dt::describe_compress_diff_from_default(many) ==
               "level, bzip2.verbosity, lz4.block_checksum");

    return true;
}

/*
tests_compress_options_diff_from_default_empty
  the pristine case of the convenience.
  Tests the following:
  - a pristine set reports no change from default
  - so does the module's own factory result
  - the empty diff agrees with the pristine predicate
*/
bool
tests_compress_options_diff_from_default_empty()
{
    const dj::compress_options a;

    D_CO_CHECK(dt::describe_compress_diff_from_default(a).empty());
    D_CO_CHECK(dt::describe_compress_diff_from_default(
                   dt::default_compress_options()).empty());

    // the diff and the predicate always agree, in both directions
    D_CO_CHECK(dt::describe_compress_diff_from_default(a).empty() ==
               dt::compress_options_are_default(a));

    dj::compress_options moved;

    moved.lzma.threads = 4u;

    D_CO_CHECK(dt::describe_compress_diff_from_default(moved).empty() ==
               dt::compress_options_are_default(moved));
    D_CO_CHECK(!dt::describe_compress_diff_from_default(moved).empty());

    return true;
}

NS_END  // testing
NS_END  // djinterp
