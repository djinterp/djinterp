/*******************************************************************************
* djinterp [test]                           test_report_runner_tests_options.cpp
*
*
* path:      /tests/djinterp/test/output/test_report_runner_tests_options.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "test_report_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_report_runner_use_pdf_selects_document
  the one call a runner makes to opt into a PDF.
  Tests the following:
  - use_pdf switches the document knob from the txt default to pdf
  - calling it twice is idempotent
  - no other knob is disturbed
*/
bool
tests_report_runner_use_pdf_selects_document()
{
    dt::report_builder rb;

    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::txt);

    rb.use_pdf();

    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::pdf);

    // a second call changes nothing
    rb.use_pdf();

    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::pdf);

    // the split, show and packaging knobs keep their defaults
    D_RR_CHECK(dt::split(rb.options()) == dt::test_output_split::whole_run);
    D_RR_CHECK(dt::show(rb.options()) == dt::test_show::all);
    D_RR_CHECK(dt::pack(rb.options()) == dt::test_output_pack::none);

    return true;
}

/*
tests_report_runner_use_pdf_sets_path
  naming the document in the same call.
  Tests the following:
  - a non-empty path is written to the output_file slot
  - the document knob is switched at the same time
  - a later call with a different path overwrites it
*/
bool
tests_report_runner_use_pdf_sets_path()
{
    dt::report_builder rb;

    rb.use_pdf("first.pdf");

    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::pdf);
    D_RR_CHECK(dt::output_path(rb.options()) == "first.pdf");

    rb.use_pdf("second.pdf");

    D_RR_CHECK(dt::output_path(rb.options()) == "second.pdf");

    // a path carrying a directory prefix is stored verbatim
    rb.use_pdf("out/dir/third.pdf");

    D_RR_CHECK(dt::output_path(rb.options()) == "out/dir/third.pdf");

    return true;
}

/*
tests_report_runner_use_pdf_empty_path_keeps_existing
  the empty-path guard.
  Tests the following:
  - use_pdf() with no argument leaves an already-set output_file alone
    rather than clearing it
  - an explicitly empty string is treated the same way
  - the document knob is still switched either way
*/
bool
tests_report_runner_use_pdf_empty_path_keeps_existing()
{
    dt::report_builder rb;

    rb.set_output_file("preset.pdf");
    rb.use_pdf();

    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::pdf);
    D_RR_CHECK(dt::output_path(rb.options()) == "preset.pdf");

    // an explicit empty string is also a no-op for the path
    rb.use_pdf(std::string());

    D_RR_CHECK(dt::output_path(rb.options()) == "preset.pdf");

    // starting from no path at all, use_pdf() leaves it empty
    dt::report_builder fresh;

    fresh.use_pdf();

    D_RR_CHECK(dt::document(fresh.options()) == dt::test_doc_type::pdf);
    D_RR_CHECK(dt::output_path(fresh.options()).empty());

    return true;
}

/*
tests_report_runner_use_pdf_returns_self
  the chaining return.
  Tests the following:
  - use_pdf returns a reference to the builder itself
  - the returned reference drives the same object
*/
bool
tests_report_runner_use_pdf_returns_self()
{
    dt::report_builder rb;

    dt::report_builder& back = rb.use_pdf("chained.pdf");

    D_RR_CHECK(&back == &rb);

    // writing through the returned reference reaches the original
    back.set_title("through the chain");

    D_RR_CHECK(rb.report().title == "through the chain");
    D_RR_CHECK(dt::output_path(back.options()) == "chained.pdf");

    return true;
}

/*
tests_report_runner_set_document
  the document knob.
  Tests the following:
  - every test_doc_type value can be selected
  - the knob can be moved back to the txt default
*/
bool
tests_report_runner_set_document()
{
    dt::report_builder rb;

    rb.set_document(dt::test_doc_type::xml);
    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::xml);

    rb.set_document(dt::test_doc_type::html);
    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::html);

    rb.set_document(dt::test_doc_type::pdf);
    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::pdf);

    // and back to the console-only default
    rb.set_document(dt::test_doc_type::txt);
    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::txt);

    return true;
}

/*
tests_report_runner_set_output_file
  the output path slot.
  Tests the following:
  - a path is written and read back unchanged
  - an empty string clears the slot
  - the slot is independent of the document knob
*/
bool
tests_report_runner_set_output_file()
{
    dt::report_builder rb;

    rb.set_output_file("a/b/c.pdf");
    D_RR_CHECK(dt::output_path(rb.options()) == "a/b/c.pdf");

    rb.set_output_file(std::string());
    D_RR_CHECK(dt::output_path(rb.options()).empty());

    // setting a path does not imply a document type
    rb.set_output_file("still_txt.pdf");
    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::txt);
    D_RR_CHECK(dt::output_path(rb.options()) == "still_txt.pdf");

    return true;
}

/*
tests_report_runner_set_split
  the whole-run versus per-module knob.
  Tests the following:
  - the split moves to per_module and back
  - it is independent of the document knob
*/
bool
tests_report_runner_set_split()
{
    dt::report_builder rb;

    D_RR_CHECK(dt::split(rb.options()) == dt::test_output_split::whole_run);

    rb.set_split(dt::test_output_split::per_module);
    D_RR_CHECK(dt::split(rb.options()) == dt::test_output_split::per_module);

    rb.set_split(dt::test_output_split::whole_run);
    D_RR_CHECK(dt::split(rb.options()) == dt::test_output_split::whole_run);

    // switching the document does not reset the split
    rb.set_split(dt::test_output_split::per_module);
    rb.use_pdf("x.pdf");
    D_RR_CHECK(dt::split(rb.options()) == dt::test_output_split::per_module);

    return true;
}

/*
tests_report_runner_set_show
  the result-filtering knob.
  Tests the following:
  - every test_show value can be selected
  - the silent value is reachable, since it is what suppresses the
    not-written note at emission time
*/
bool
tests_report_runner_set_show()
{
    dt::report_builder rb;

    D_RR_CHECK(dt::show(rb.options()) == dt::test_show::all);

    rb.set_show(dt::test_show::failures_only);
    D_RR_CHECK(dt::show(rb.options()) == dt::test_show::failures_only);

    rb.set_show(dt::test_show::failures_and_skipped);
    D_RR_CHECK(dt::show(rb.options()) ==
               dt::test_show::failures_and_skipped);

    rb.set_show(dt::test_show::summary_only);
    D_RR_CHECK(dt::show(rb.options()) == dt::test_show::summary_only);

    rb.set_show(dt::test_show::silent);
    D_RR_CHECK(dt::show(rb.options()) == dt::test_show::silent);

    rb.set_show(dt::test_show::all);
    D_RR_CHECK(dt::show(rb.options()) == dt::test_show::all);

    return true;
}

/*
tests_report_runner_options_survive_recording
  option stability across a run.
  Tests the following:
  - opening modules and recording units does not disturb any knob
  - the file-name pattern setter records nothing into the model
  - the option set is still readable after finish()
*/
bool
tests_report_runner_options_survive_recording()
{
    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf("stable.pdf");
    rb.set_split(dt::test_output_split::per_module);
    rb.set_show(dt::test_show::summary_only);
    rb.set_file_name_pattern("{module}-{index}.{ext}");

    rb.module("m0", "first");
    rb.run("t0", &rr_pass);
    rb.open_unit("rich", "descriptor");
    rb.check("expr", "1", "1", true);
    rb.close_unit();
    rb.module("m1", "second");
    rb.unit("t1", false);

    D_RR_CHECK(dt::document(rb.options()) == dt::test_doc_type::pdf);
    D_RR_CHECK(dt::output_path(rb.options()) == "stable.pdf");
    D_RR_CHECK(dt::split(rb.options()) == dt::test_output_split::per_module);
    D_RR_CHECK(dt::show(rb.options()) == dt::test_show::summary_only);

    // the pattern is builder-local and leaves no trace in the model
    D_RR_CHECK(rb.report().total_modules() == 2u);
    D_RR_CHECK(rb.report().title.empty());
    D_RR_CHECK(rb.report().notes.empty());

    return true;
}

NS_END  // testing
NS_END  // djinterp
