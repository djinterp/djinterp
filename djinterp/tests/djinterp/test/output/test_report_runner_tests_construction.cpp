/*******************************************************************************
* djinterp [test]                      test_report_runner_tests_construction.cpp
*
*
* path:      /tests/djinterp/test/output/test_report_runner_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "test_report_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_report_runner_default_construction
  the framework-default builder.
  Tests the following:
  - the accumulating report starts empty, with no modules and no cover
    metadata
  - the option set is the framework default: a txt document, the whole-run
    split, everything shown and nothing packed
  - the console starts enabled
*/
bool
tests_report_runner_default_construction()
{
    dt::report_builder rb;

    D_RR_CHECK(rb.report().modules.empty());
    D_RR_CHECK(rb.report().total_modules() == 0u);
    D_RR_CHECK(rb.report().total_units() == 0u);
    D_RR_CHECK(rb.report().total_checks() == 0u);
    D_RR_CHECK(rb.report().title.empty());
    D_RR_CHECK(rb.report().subtitle.empty());
    D_RR_CHECK(rb.report().author.empty());

    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::txt);
    D_RR_CHECK(dt::split(rb.options()) == dt::test_output_split::whole_run);
    D_RR_CHECK(dt::show(rb.options()) == dt::test_show::all);
    D_RR_CHECK(dt::pack(rb.options()) == dt::test_output_pack::none);
    D_RR_CHECK(dt::output_path(rb.options()).empty());

    // an empty run is not a clean pass
    D_RR_CHECK(rb.report().verdict() == dt::report_verdict::empty);
    D_RR_CHECK(!rb.report().passed());
    D_RR_CHECK(rb.report().exit_code() == 1);

    return true;
}

/*
tests_report_runner_option_set_construction
  the seeded builder.
  Tests the following:
  - a builder constructed from an option set carries that set, not the
    framework defaults
  - the seeded knobs survive construction intact
  - the report is still empty, so seeding options records nothing
*/
bool
tests_report_runner_option_set_construction()
{
    dt::test_option_set o = dt::default_test_options();

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    o.set<dt::test_option::document>(dt::test_doc_type::pdf);
    o.set<dt::test_option::split>(dt::test_output_split::per_module);
    o.set<dt::test_option::show>(dt::test_show::silent);
    o.set<dt::test_option::output_file>(std::string("seeded.pdf"));
#else
    o.document    = dt::test_doc_type::pdf;
    o.split       = dt::test_output_split::per_module;
    o.show        = dt::test_show::silent;
    o.output_path = "seeded.pdf";
#endif

    dt::report_builder rb(o);

    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::pdf);
    D_RR_CHECK(dt::split(rb.options()) == dt::test_output_split::per_module);
    D_RR_CHECK(dt::show(rb.options()) == dt::test_show::silent);
    D_RR_CHECK(dt::output_path(rb.options()) == "seeded.pdf");

    D_RR_CHECK(rb.report().modules.empty());

    return true;
}

/*
tests_report_runner_report_accessors
  reaching the accumulating model.
  Tests the following:
  - report() returns the live model, so a direct edit is visible through a
    later read
  - the const overload reads the same object
  - advanced population through the model is picked up by the tallies
*/
bool
tests_report_runner_report_accessors()
{
    dt::report_builder rb;

    rb.report().title = "written directly";

    D_RR_CHECK(rb.report().title == "written directly");

    // the const overload sees the same object
    const dt::report_builder& crb = rb;

    D_RR_CHECK(crb.report().title == "written directly");
    D_RR_CHECK(crb.report().modules.empty());

    // a module pushed straight into the model is counted like any other
    rb.report().modules.push_back(dt::report_module("direct", "seeded"));
    rb.report().modules[0].add_result_unit("leaf", true);

    D_RR_CHECK(crb.report().total_modules() == 1u);
    D_RR_CHECK(crb.report().total_units() == 1u);
    D_RR_CHECK(crb.report().total_checks() == 1u);
    D_RR_CHECK(crb.report().passed());

    return true;
}

/*
tests_report_runner_options_accessors
  reaching the option set.
  Tests the following:
  - options() returns the live set, so a knob the conveniences do not
    cover can still be written
  - the const overload reads the same object
  - a packaging knob written this way is read back through the free
    accessors
*/
bool
tests_report_runner_options_accessors()
{
    dt::report_builder rb;

    rr_set_pack(rb.options(), dt::test_output_pack::archive);
    rr_set_archive_format(rb.options(), dt::test_archive_format::sevenzip);

    const dt::report_builder& crb = rb;

    D_RR_CHECK(dt::pack(crb.options()) == dt::test_output_pack::archive);
    D_RR_CHECK(dt::archive_format(crb.options()) ==
               dt::test_archive_format::sevenzip);

    // and the untouched knobs keep their defaults
    D_RR_CHECK(dt::document(crb.options()) == dt::test_doc_type::txt);
    D_RR_CHECK(dt::split(crb.options()) == dt::test_output_split::whole_run);

    return true;
}

/*
tests_report_runner_metadata_setters
  the cover-metadata conveniences.
  Tests the following:
  - each setter writes its own field of the report and no other
  - the fields are independent and survive one another
  - an empty string clears a field rather than being ignored
*/
bool
tests_report_runner_metadata_setters()
{
    dt::report_builder rb;

    rb.set_title("The Title");
    rb.set_subtitle("The Subtitle");
    rb.set_author("The Author");
    rb.set_description("The Description");
    rb.set_notes("The Notes");

    D_RR_CHECK(rb.report().title == "The Title");
    D_RR_CHECK(rb.report().subtitle == "The Subtitle");
    D_RR_CHECK(rb.report().author == "The Author");
    D_RR_CHECK(rb.report().description == "The Description");
    D_RR_CHECK(rb.report().notes == "The Notes");

    // overwriting one field leaves the others alone
    rb.set_title("A New Title");

    D_RR_CHECK(rb.report().title == "A New Title");
    D_RR_CHECK(rb.report().subtitle == "The Subtitle");
    D_RR_CHECK(rb.report().notes == "The Notes");

    // and an empty string genuinely clears
    rb.set_subtitle(std::string());

    D_RR_CHECK(rb.report().subtitle.empty());
    D_RR_CHECK(rb.report().title == "A New Title");

    return true;
}

/*
tests_report_runner_console_toggle
  silencing the live console.
  Tests the following:
  - with the console off, opening a module and recording units writes
    nothing to the sink
  - the model is still accumulated in full
  - re-enabling the console resumes emission
*/
bool
tests_report_runner_console_toggle()
{
    rr_capture cap("dtest_rr_console_toggle.txt");

    D_RR_CHECK(cap.valid());

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);

    rb.module("quiet", "a description");
    rb.unit("u0", true);
    rb.unit("u1", false);

    D_RR_CHECK(cap.text().empty());

    // the model was accumulated regardless
    D_RR_CHECK(rb.report().total_modules() == 1u);
    D_RR_CHECK(rb.report().total_units() == 2u);
    D_RR_CHECK(rb.report().modules[0].name == "quiet");

    // turning the console back on resumes emission
    rb.set_console(true);
    rb.unit("u2", true);

    D_RR_CHECK(!cap.text().empty());
    D_RR_CHECK(rr_contains(cap.text(), "u2"));

    // and the earlier, silenced units were never printed
    D_RR_CHECK(!rr_contains(cap.text(), "u0"));
    D_RR_CHECK(!rr_contains(cap.text(), "u1"));

    return true;
}

/*
tests_report_runner_null_file_sink
  a builder with nowhere to write.
  Tests the following:
  - a null sink suppresses output even with the console enabled, rather
    than writing through a null pointer
  - recording and finishing still work and still accumulate the model
  - the exit code is decided by the model, not by the sink
*/
bool
tests_report_runner_null_file_sink()
{
    dt::report_builder rb;

    rb.set_file(nullptr);
    rb.set_console(true);

    rb.module("nowhere", "no sink at all");
    rb.unit("u0", true);
    rb.open_unit("rich", "a descriptor");
    rb.check("expr", "1", "1", true);
    rb.close_unit();

    D_RR_CHECK(rb.report().total_modules() == 1u);
    D_RR_CHECK(rb.report().total_units() == 2u);
    D_RR_CHECK(rb.report().total_checks() == 2u);

    // finishing with a null sink is safe and still decides the run
    D_RR_CHECK(rb.finish() == 0);
    D_RR_CHECK(rb.report().passed());

    return true;
}

NS_END  // testing
NS_END  // djinterp
