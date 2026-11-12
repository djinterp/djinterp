/*******************************************************************************
* djinterp [cpp][test]                         test_options_tests_option_set.cpp
*
*   Sections B.II / B.III / B.IV - the option set's defaults, read through the
* free accessor seam.  default_test_options() yields a fully-defaulted set, and
* every accessor is exercised against it: the numbering / timing knobs, the
* layout knobs, presentation, destinations, filtering, the format templates, the
* packaging knobs (including the compress_opts / archive_opts bundles, exercised
* for reference stability since their contents are backend-specific), and the
* empty route list.  Reading the defaults through the accessors covers both the
* documented floor and every accessor body; the keyed get<>/set<> are checked
* against the same members.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_options_tests_option_set.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "test_options_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_opt_defaults_numbering_timing
//   numbering on, ordinal style, timing off, automatic unit.
bool tests_opt_defaults_numbering_timing()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::numbering(o)    == true)                          && ok;
    ok = D_TO_CHECK(dt::number_style(o) == dt::DTestNumberStyle::ordinal)&& ok;
    ok = D_TO_CHECK(dt::show_timing(o)  == false)                         && ok;
    ok = D_TO_CHECK(dt::time_unit(o)    == dt::DTestTimeUnit::automatic) && ok;

    return ok;
}

// tests_opt_defaults_layout
//   80-column, word-wrapped, word mode, indent 2.
bool tests_opt_defaults_layout()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::line_width(o)   == static_cast<std::size_t>(80)) && ok;
    ok = D_TO_CHECK(dt::word_wrap(o)    == true)                         && ok;
    ok = D_TO_CHECK(dt::wrap_mode(o)    == dt::DTestWrapMode::word)     && ok;
    ok = D_TO_CHECK(dt::indent_width(o) == static_cast<std::size_t>(2))  && ok;

    return ok;
}

// tests_opt_defaults_presentation
//   color on, plain-text document.
bool tests_opt_defaults_presentation()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::color(o)    == true)                    && ok;
    ok = D_TO_CHECK(dt::document(o) == dt::test_doc_type::text)  && ok;

    return ok;
}

// tests_opt_defaults_destinations
//   the console sink, no output file.
bool tests_opt_defaults_destinations()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::sinks(o)       == dt::DTestSink::console) && ok;
    ok = D_TO_CHECK(dt::output_path(o) == std::string())         && ok;

    return ok;
}

// tests_opt_defaults_filtering
//   show all, no failure cap, do not stop on failure.
bool tests_opt_defaults_filtering()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::show(o)            == dt::DTestShow::all)         && ok;
    ok = D_TO_CHECK(dt::max_failures(o)    == static_cast<std::size_t>(0))&& ok;
    ok = D_TO_CHECK(dt::stop_on_failure(o) == false)                     && ok;

    return ok;
}

// tests_opt_defaults_formats
//   the three default {key} templates.
bool tests_opt_defaults_formats()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::format_test(o)    == "{index}. {name} [{status}] {duration}") && ok;
    ok = D_TO_CHECK(dt::format_module(o)  == "{name}")                                && ok;
    ok = D_TO_CHECK(dt::format_summary(o) ==
                    "{passed}/{total} passed, {failed} failed, {skipped} skipped")    && ok;

    return ok;
}

// tests_opt_defaults_packaging
//   what the build's packaging knobs give (cfg_test_options.h, falling back to
// cfg_output_packaging.h): unpacked, gzip codec and zip container when no knob
// is set; the container knob's gz and tar.gz land on the right enumerators;
// the entry knobs reach archive_opts only when set; whole-run split; the
// option-bundle accessors return stable references.
bool tests_opt_defaults_packaging()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    // the knobs, as the constructor converts them
    ok = D_TO_CHECK(dt::pack(o) ==
                    static_cast<dt::DTestOutputPack>(
                        D_INTERNAL_TEST_OUTPUT_PACK)) && ok;
    ok = D_TO_CHECK(dt::compressor(o) ==
                    static_cast<dt::test_compressor>(
                        D_INTERNAL_TEST_OUTPUT_CODEC)) && ok;

    // the container knob speaks dj_format_id, which orders gz and tar.gz the
    // other way round from test_archive_format
#if   ( D_CFG_NORM(D_CFG_TEST_OUTPUT_FORMAT) == 2 )
    ok = D_TO_CHECK(dt::archive_format(o) ==
                    dt::test_archive_format::gz) && ok;
#elif ( D_CFG_NORM(D_CFG_TEST_OUTPUT_FORMAT) == 3 )
    ok = D_TO_CHECK(dt::archive_format(o) ==
                    dt::test_archive_format::tar_gz) && ok;
#else
    ok = D_TO_CHECK(dt::archive_format(o) ==
                    static_cast<dt::test_archive_format>(
                        D_CFG_NORM(D_CFG_TEST_OUTPUT_FORMAT))) && ok;
#endif

    // with no knob set: unpacked, gzip, zip
#if ( (D_CFG_NORM(D_CFG_TEST_OUTPUT_PACK) == 0)  &&                           \
      (D_CFG_NORM(D_CFG_TEST_OUTPUT_CODEC) == 3) &&                           \
      (D_CFG_NORM(D_CFG_TEST_OUTPUT_FORMAT) == 0) )
    ok = D_TO_CHECK(dt::pack(o) == dt::DTestOutputPack::none) && ok;
    ok = D_TO_CHECK(dt::compressor(o) == dt::test_compressor::gzip) && ok;
    ok = D_TO_CHECK(dt::archive_format(o) ==
                    dt::test_archive_format::zip) && ok;
#endif

    // the entry knobs: set, they are archive_opts'; unset, archive_opts leaves
    // its own knob unset for cfg_archive.h to fill
#if ( D_INTERNAL_TEST_OUTPUT_STORE_ONLY >= 0 )
    ok = D_TO_CHECK(dt::archive_opts(o).store_only ==
                    D_INTERNAL_TEST_OUTPUT_STORE_ONLY) && ok;
#else
    ok = D_TO_CHECK(dt::archive_opts(o).store_only ==
                    D_ARCHIVE_KNOB_UNSET) && ok;
#endif
#if ( D_INTERNAL_TEST_OUTPUT_LEVEL >= 0 )
    ok = D_TO_CHECK(dt::archive_opts(o).level ==
                    D_INTERNAL_TEST_OUTPUT_LEVEL) && ok;
#else
    ok = D_TO_CHECK(dt::archive_opts(o).level ==
                    D_ARCHIVE_KNOB_UNSET) && ok;
#endif

    ok = D_TO_CHECK(dt::split(o)          == dt::DTestOutputSplit::whole_run) && ok;

    // compress_opts / archive_opts: exercise the accessors (their contents are
    // backend-specific, so assert only that each returns a stable reference).
    ok = D_TO_CHECK(&dt::compress_opts(o) == &dt::compress_opts(o)) && ok;
    ok = D_TO_CHECK(&dt::archive_opts(o)  == &dt::archive_opts(o))  && ok;

    return ok;
}

// tests_opt_defaults_routes
//   no routes by default.
bool tests_opt_defaults_routes()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::routes(o).empty()) && ok;

    return ok;
}

// tests_opt_keyed_access
//   one shape at every level: set<key>() writes the member the key names, and
// get<key>(), the member and the free accessor all read it back.
bool tests_opt_keyed_access()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    // the members exist at every level the module compiles at
    bool dt::test_option_set::* const member_show_timing =
        &dt::test_option_set::show_timing;
    (void)member_show_timing;

    o.set<dt::test_option::line_width>(static_cast<std::size_t>(100));
    ok = D_TO_CHECK(o.line_width == 100u) && ok;
    ok = D_TO_CHECK(o.get<dt::test_option::line_width>() == 100u) && ok;
    ok = D_TO_CHECK(dt::line_width(o) == 100u) && ok;

    o.set<dt::test_option::show>(dt::DTestShow::failures_only);
    ok = D_TO_CHECK(o.show == dt::DTestShow::failures_only) && ok;

    o.set<dt::test_option::output_file>(std::string("keyed.txt"));
    ok = D_TO_CHECK(o.output_path == "keyed.txt") && ok;
    ok = D_TO_CHECK(dt::output_path(o) == "keyed.txt") && ok;

    o.set<dt::test_option::destination>(dt::DTestSink::console);
    ok = D_TO_CHECK(o.get<dt::test_option::destination>() ==
                    dt::DTestSink::console) && ok;

    // a member written directly reads back through its key
    o.color = false;
    ok = D_TO_CHECK(!o.get<dt::test_option::color>()) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
