/*******************************************************************************
* djinterp [test]                              test_options_tests_option_set.cpp
*
*   Sections B.II / B.III / B.IV - the option set's defaults, read through the
* free accessor seam.  default_test_options() yields a fully-defaulted set, and
* every accessor is exercised against it: the numbering / timing knobs, the
* layout knobs, presentation, destinations, filtering, the format templates, the
* packaging knobs (including the compress_opts / archive_opts bundles, exercised
* for reference stability since their contents are backend-specific), and the
* empty route list.  Reading the defaults through the accessors covers both the
* documented floor and every accessor body on whichever option-set face the
* language level selects.
*
*
* path:      /tests/djinterp/test/test_options_tests_option_set.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_options_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_opt_defaults_numbering_timing
//   numbering on, ordinal style, timing off, automatic unit.
bool
tests_opt_defaults_numbering_timing()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::numbering(o)    == true)                          && ok;
    ok = D_TO_CHECK(dt::number_style(o) == dt::test_number_style::ordinal)&& ok;
    ok = D_TO_CHECK(dt::show_timing(o)  == false)                         && ok;
    ok = D_TO_CHECK(dt::time_unit(o)    == dt::test_time_unit::automatic) && ok;

    return ok;
}

// tests_opt_defaults_layout
//   80-column, word-wrapped, word mode, indent 2.
bool
tests_opt_defaults_layout()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::line_width(o)   == static_cast<std::size_t>(80)) && ok;
    ok = D_TO_CHECK(dt::word_wrap(o)    == true)                         && ok;
    ok = D_TO_CHECK(dt::wrap_mode(o)    == dt::test_wrap_mode::word)     && ok;
    ok = D_TO_CHECK(dt::indent_width(o) == static_cast<std::size_t>(2))  && ok;

    return ok;
}

// tests_opt_defaults_presentation
//   color on, plain-text document.
bool
tests_opt_defaults_presentation()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::color(o)    == true)                    && ok;
    ok = D_TO_CHECK(dt::document(o) == dt::test_doc_type::txt)  && ok;

    return ok;
}

// tests_opt_defaults_destinations
//   the console sink, no output file.
bool
tests_opt_defaults_destinations()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::sinks(o)       == dt::test_sink::console) && ok;
    ok = D_TO_CHECK(dt::output_path(o) == std::string())         && ok;

    return ok;
}

// tests_opt_defaults_filtering
//   show all, no failure cap, do not stop on failure.
bool
tests_opt_defaults_filtering()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::show(o)            == dt::test_show::all)         && ok;
    ok = D_TO_CHECK(dt::max_failures(o)    == static_cast<std::size_t>(0))&& ok;
    ok = D_TO_CHECK(dt::stop_on_failure(o) == false)                     && ok;

    return ok;
}

// tests_opt_defaults_formats
//   the three default {key} templates.
bool
tests_opt_defaults_formats()
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
//   unpacked, gzip codec, zip container, whole-run split; the option-bundle
// accessors return stable references.
bool
tests_opt_defaults_packaging()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::pack(o)           == dt::test_output_pack::none)   && ok;
    ok = D_TO_CHECK(dt::compressor(o)     == dt::test_compressor::gzip)    && ok;
    ok = D_TO_CHECK(dt::archive_format(o) == dt::test_archive_format::zip) && ok;
    ok = D_TO_CHECK(dt::split(o)          == dt::test_output_split::whole_run) && ok;

    // compress_opts / archive_opts: exercise the accessors (their contents are
    // backend-specific, so assert only that each returns a stable reference).
    ok = D_TO_CHECK(&dt::compress_opts(o) == &dt::compress_opts(o)) && ok;
    ok = D_TO_CHECK(&dt::archive_opts(o)  == &dt::archive_opts(o))  && ok;

    return ok;
}

// tests_opt_defaults_routes
//   no routes by default.
bool
tests_opt_defaults_routes()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    ok = D_TO_CHECK(dt::routes(o).empty()) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
