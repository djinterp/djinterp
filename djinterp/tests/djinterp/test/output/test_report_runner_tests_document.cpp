/*******************************************************************************
* djinterp [test]                          test_report_runner_tests_document.cpp
*
*
* path:      /tests/djinterp/test/output/test_report_runner_tests_document.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "test_report_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_report_runner_txt_writes_no_file
  the document knob's gate.
  Tests the following:
  - the default txt document writes nothing at all, so a run stays
    console-only
  - the not-written note is NOT printed, because emission never began
  - naming an output file does not by itself cause a write
*/
bool
tests_report_runner_txt_writes_no_file()
{
    rr_capture cap("dtest_rr_txt_console.txt");

    D_RR_CHECK(cap.valid());

    const std::string path = "dtest_rr_txt_never.pdf";

    rr_remove(path);

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);
    rb.set_output_file(path);
    rb.module("m");
    rr_seed_units(rb, 1, 0);
    rb.set_console(true);
    rb.finish();

    D_RR_CHECK(!rr_file_exists(path));
    D_RR_CHECK(!rr_contains(cap.text(), "wrote report: "));
    D_RR_CHECK(!rr_contains(cap.text(), "(report document was not written)"));

    // the same holds for the other non-pdf document types
    rb.set_document(dt::test_doc_type::html);
    rb.finish();

    D_RR_CHECK(!rr_file_exists(path));

    rr_remove(path);

    return true;
}

/*
tests_report_runner_pdf_whole_run_writes_one
  the whole-run split.
  Tests the following:
  - exactly one document is written for a multi-module run
  - the file really is a PDF byte stream
  - it covers every module of the run
*/
bool
tests_report_runner_pdf_whole_run_writes_one()
{
    const std::string path = "dtest_rr_whole.pdf";

    rr_remove(path);

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf(path);
    rb.set_split(dt::test_output_split::whole_run);

    rb.module("mod_alpha");
    rr_seed_units(rb, 2, 0);
    rb.module("mod_beta");
    rr_seed_units(rb, 1, 1);

    rb.finish();

    D_RR_CHECK(rr_file_exists(path));

    const std::string bytes = rr_read_file(path);

    D_RR_CHECK(!bytes.empty());
    D_RR_CHECK(bytes.compare(0, 5, "%PDF-") == 0);
    D_RR_CHECK(rr_contains(bytes, "%%EOF"));

    // one document carrying both modules
    D_RR_CHECK(rr_contains(bytes, "mod_alpha"));
    D_RR_CHECK(rr_contains(bytes, "mod_beta"));

    rr_remove(path);

    return true;
}

/*
tests_report_runner_pdf_output_path_used
  naming the whole-run document.
  Tests the following:
  - the output_file slot names the file exactly
  - the fallback name is NOT produced when a path was given
  - the cover metadata reaches the document
*/
bool
tests_report_runner_pdf_output_path_used()
{
    const std::string path     = "dtest_rr_named.pdf";
    const std::string fallback = "report.pdf";

    rr_remove(path);
    rr_remove(fallback);

    dt::report_builder rb;

    rb.set_console(false);
    rb.set_title("A Titled Run");
    rb.use_pdf(path);
    rb.module("m");
    rr_seed_units(rb, 1, 0);
    rb.finish();

    D_RR_CHECK(rr_file_exists(path));
    D_RR_CHECK(!rr_file_exists(fallback));

    const std::string bytes = rr_read_file(path);

    D_RR_CHECK(rr_contains(bytes, "A Titled Run"));

    rr_remove(path);

    return true;
}

/*
tests_report_runner_pdf_empty_path_falls_back
  the whole-run fallback name.
  Tests the following:
  - with no output_file, the name comes from the pattern with the module
    name `report`
  - the document is still written and still complete
*/
bool
tests_report_runner_pdf_empty_path_falls_back()
{
    const std::string expected = "dtest_rr_fb_report.pdf";

    rr_remove(expected);

    dt::report_builder rb;

    rb.set_console(false);
    rb.set_document(dt::test_doc_type::pdf);
    rb.set_file_name_pattern("dtest_rr_fb_{module}.{ext}");

    D_RR_CHECK(dt::output_path(rb.options()).empty());

    rb.module("some_module");
    rr_seed_units(rb, 1, 0);
    rb.finish();

    // the whole-run document is named after `report`, not the module
    D_RR_CHECK(rr_file_exists(expected));
    D_RR_CHECK(!rr_file_exists("dtest_rr_fb_some_module.pdf"));

    const std::string bytes = rr_read_file(expected);

    D_RR_CHECK(bytes.compare(0, 5, "%PDF-") == 0);
    D_RR_CHECK(rr_contains(bytes, "some_module"));

    rr_remove(expected);

    return true;
}

/*
tests_report_runner_pdf_per_module_writes_each
  the per-module split.
  Tests the following:
  - one document is written per module
  - each carries only its own module, not the others
  - the run's cover metadata is copied into every document
*/
bool
tests_report_runner_pdf_per_module_writes_each()
{
    const std::string a = "dtest_rr_pm_first.pdf";
    const std::string b = "dtest_rr_pm_second.pdf";

    rr_remove(a);
    rr_remove(b);

    dt::report_builder rb;

    rb.set_console(false);
    rb.set_document(dt::test_doc_type::pdf);
    rb.set_split(dt::test_output_split::per_module);
    rb.set_file_name_pattern("dtest_rr_pm_{module}.{ext}");
    rb.set_title("Shared Cover");

    rb.module("first");
    rr_seed_units(rb, 2, 0);
    rb.module("second");
    rr_seed_units(rb, 1, 0);

    rb.finish();

    D_RR_CHECK(rr_file_exists(a));
    D_RR_CHECK(rr_file_exists(b));

    const std::string ab = rr_read_file(a);
    const std::string bb = rr_read_file(b);

    D_RR_CHECK(ab.compare(0, 5, "%PDF-") == 0);
    D_RR_CHECK(bb.compare(0, 5, "%PDF-") == 0);

    // each document is confined to its own module
    D_RR_CHECK(rr_contains(ab, "first"));
    D_RR_CHECK(!rr_contains(ab, "second"));
    D_RR_CHECK(rr_contains(bb, "second"));

    // but both carry the run's cover metadata
    D_RR_CHECK(rr_contains(ab, "Shared Cover"));
    D_RR_CHECK(rr_contains(bb, "Shared Cover"));

    rr_remove(a);
    rr_remove(b);

    return true;
}

/*
tests_report_runner_pdf_per_module_ignores_output_path
  where per-module names come from.
  Tests the following:
  - per-module naming is driven by the pattern alone
  - the output_file slot is not consulted, so no file takes that name
*/
bool
tests_report_runner_pdf_per_module_ignores_output_path()
{
    const std::string slot     = "dtest_rr_ignored_slot.pdf";
    const std::string expected = "dtest_rr_ig_only.pdf";

    rr_remove(slot);
    rr_remove(expected);

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf(slot);
    rb.set_split(dt::test_output_split::per_module);
    rb.set_file_name_pattern("dtest_rr_ig_{module}.{ext}");

    rb.module("only");
    rr_seed_units(rb, 1, 0);
    rb.finish();

    D_RR_CHECK(rr_file_exists(expected));
    D_RR_CHECK(!rr_file_exists(slot));

    // the slot itself was never cleared - it was simply not used
    D_RR_CHECK(dt::output_path(rb.options()) == slot);

    rr_remove(expected);

    return true;
}

/*
tests_report_runner_pdf_file_name_pattern
  the builder-local naming pattern.
  Tests the following:
  - the module placeholder expands to each module's name
  - the index placeholder expands to that module's ordinal
  - the extension placeholder expands to pdf
*/
bool
tests_report_runner_pdf_file_name_pattern()
{
    const std::string a = "dtest_rr_pat_alpha_0.pdf";
    const std::string b = "dtest_rr_pat_beta_1.pdf";

    rr_remove(a);
    rr_remove(b);

    dt::report_builder rb;

    rb.set_console(false);
    rb.set_document(dt::test_doc_type::pdf);
    rb.set_split(dt::test_output_split::per_module);
    rb.set_file_name_pattern("dtest_rr_pat_{module}_{index}.{ext}");

    rb.module("alpha");
    rr_seed_units(rb, 1, 0);
    rb.module("beta");
    rr_seed_units(rb, 1, 0);

    rb.finish();

    D_RR_CHECK(rr_file_exists(a));
    D_RR_CHECK(rr_file_exists(b));

    // the index is the module's ordinal, counted from zero
    D_RR_CHECK(!rr_file_exists("dtest_rr_pat_alpha_1.pdf"));
    D_RR_CHECK(!rr_file_exists("dtest_rr_pat_beta_0.pdf"));

    rr_remove(a);
    rr_remove(b);

    return true;
}

/*
tests_report_runner_pdf_write_failure_reports
  an unwritable destination.
  Tests the following:
  - a path that cannot be opened writes no file and is not reported as
    written
  - the console carries the not-written note instead
  - the exit code still reflects the tests, not the write
*/
bool
tests_report_runner_pdf_write_failure_reports()
{
    rr_capture cap("dtest_rr_writefail.txt");

    D_RR_CHECK(cap.valid());

    // a directory that does not exist, so fopen must fail
    const std::string path = "dtest_rr_no_such_dir/report.pdf";

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);
    rb.use_pdf(path);
    rb.module("m");
    rr_seed_units(rb, 2, 0);
    rb.set_console(true);

    const int rc = rb.finish();

    D_RR_CHECK(!rr_file_exists(path));

    const std::string out = cap.text();

    D_RR_CHECK(!rr_contains(out, "wrote report: "));
    D_RR_CHECK(rr_contains(out, "\n(report document was not written)\n"));

    // the tests themselves passed, so the run still succeeds
    D_RR_CHECK(rc == 0);

    // and a silent run suppresses even the note
    rr_capture cap2("dtest_rr_writefail2.txt");

    D_RR_CHECK(cap2.valid());

    dt::report_builder rb2;

    rb2.set_file(cap2.file());
    rb2.set_console(false);
    rb2.use_pdf(path);
    rb2.set_show(dt::test_show::silent);
    rb2.module("m");
    rr_seed_units(rb2, 1, 0);
    rb2.set_console(true);
    rb2.finish();

    D_RR_CHECK(!rr_contains(cap2.text(),
                            "(report document was not written)"));

    return true;
}

/*
tests_report_runner_pdf_console_notes_path
  announcing what was written.
  Tests the following:
  - each written document is announced on the console by path
  - a per-module run announces one path per module
  - the not-written note is not printed when a document was written
*/
bool
tests_report_runner_pdf_console_notes_path()
{
    rr_capture cap("dtest_rr_notes.txt");

    D_RR_CHECK(cap.valid());

    const std::string a = "dtest_rr_note_one.pdf";
    const std::string b = "dtest_rr_note_two.pdf";

    rr_remove(a);
    rr_remove(b);

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);
    rb.set_document(dt::test_doc_type::pdf);
    rb.set_split(dt::test_output_split::per_module);
    rb.set_file_name_pattern("dtest_rr_note_{module}.{ext}");

    rb.module("one");
    rr_seed_units(rb, 1, 0);
    rb.module("two");
    rr_seed_units(rb, 1, 0);

    rb.set_console(true);
    rb.finish();

    const std::string out = cap.text();

    D_RR_CHECK(rr_contains(out, std::string("\nwrote report: ") + a + "\n"));
    D_RR_CHECK(rr_contains(out, std::string("\nwrote report: ") + b + "\n"));
    D_RR_CHECK(rr_count(out, "wrote report: ") == 2u);
    D_RR_CHECK(!rr_contains(out, "(report document was not written)"));

    rr_remove(a);
    rr_remove(b);

    return true;
}

NS_END  // testing
NS_END  // djinterp
