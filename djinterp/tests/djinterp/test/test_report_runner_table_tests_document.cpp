/******************************************************************************
* djinterp [tests]                    test_report_runner_table_tests_document.cpp
*
*   The PDF write path (finish -> emit_document -> write_pdf_report ->
* write_bytes_to_file), exercised end-to-end against real temp files: a
* whole_run document at the output path, a per_module document per module, the
* {module}/{index}/{ext} filename fields, the whole_run fallback name, the txt
* no-op, the unwritable-path notice, and its suppression under `silent`.  Every
* written file is confirmed a well-formed PDF and then removed.
*
*   The archive (.7z) bundle is compiled only under D_TEST_REPORT_ENABLE_ARCHIVE
* and is out of scope for this default-build suite (see the suite header).
*
* path:      /tests/djinterp/test/output/test_report_runner_table_tests_document.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

#include "test_report_runner_table_tests.hpp"


NS_DJINTERP
NS_TESTING


bool
tests_report_runner_table_pdf_whole_run()
{
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    const std::string _path = temp_out_path("whole.pdf");
    _rb.use_pdf(_path);
    _rb.module("m", "");
    _rb.run("t", &rr_pred_pass);

    const int _code = _rb.finish();

    D_RRT_CHECK(_code == 0);
    D_RRT_CHECK(file_exists(_path));
    D_RRT_CHECK(pdf_wellformed(read_file(_path)));
    D_RRT_CHECK(contains(_cap.str(), "wrote report: " + _path));

    remove_file(_path);

    return true;
}


bool
tests_report_runner_table_pdf_per_module()
{
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    const std::string _prefix = temp_out_path("pm_");
    _rb.set_split(dt::test_output_split::per_module);
    _rb.set_file_name_pattern(_prefix + "{module}_{index}.{ext}");
    _rb.use_pdf();

    _rb.module("first", "");
    _rb.run("t", &rr_pred_pass);
    _rb.module("second", "");
    _rb.run("u", &rr_pred_pass);

    const int _code = _rb.finish();

    const std::string _pa = _prefix + "first_0.pdf";
    const std::string _pb = _prefix + "second_1.pdf";

    D_RRT_CHECK(_code == 0);
    D_RRT_CHECK(file_exists(_pa));
    D_RRT_CHECK(file_exists(_pb));
    D_RRT_CHECK(pdf_wellformed(read_file(_pa)));
    D_RRT_CHECK(pdf_wellformed(read_file(_pb)));
    D_RRT_CHECK(contains(_cap.str(), "wrote report: " + _pa));
    D_RRT_CHECK(contains(_cap.str(), "wrote report: " + _pb));

    remove_file(_pa);
    remove_file(_pb);

    return true;
}


bool
tests_report_runner_table_pdf_filename_pattern()
{
    // per_module with {module}/{index}/{ext}: each module's fields land in its
    // path, and the module index increments across the run.
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    const std::string _prefix = temp_out_path("fp_");
    _rb.set_split(dt::test_output_split::per_module);
    _rb.set_file_name_pattern(_prefix + "{module}_{index}.{ext}");
    _rb.use_pdf();

    _rb.module("alpha", "");
    _rb.run("t", &rr_pred_pass);
    _rb.module("beta", "");
    _rb.run("u", &rr_pred_pass);

    _rb.finish();

    const std::string _alpha = _prefix + "alpha_0.pdf";
    const std::string _beta  = _prefix + "beta_1.pdf";

    D_RRT_CHECK(file_exists(_alpha));   // {module}=alpha, {index}=0, {ext}=pdf
    D_RRT_CHECK(file_exists(_beta));    // {module}=beta,  {index}=1, {ext}=pdf

    remove_file(_alpha);
    remove_file(_beta);

    return true;
}


bool
tests_report_runner_table_pdf_whole_run_fallback_name()
{
    // whole_run + no output path: the name falls back to the "report" module
    // name folded through the pattern.
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    const std::string _prefix = temp_out_path("fb_");
    _rb.set_file_name_pattern(_prefix + "{module}.{ext}");
    _rb.use_pdf();                                   // pdf, no output path
    _rb.module("m", "");
    _rb.run("t", &rr_pred_pass);

    const int _code = _rb.finish();

    const std::string _expected = _prefix + "report.pdf";

    D_RRT_CHECK(_code == 0);
    D_RRT_CHECK(file_exists(_expected));
    D_RRT_CHECK(pdf_wellformed(read_file(_expected)));

    remove_file(_expected);

    return true;
}


bool
tests_report_runner_table_txt_writes_nothing()
{
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    // the default document is txt: finish writes no file and no file notices
    _rb.module("m", "");
    _rb.run("t", &rr_pred_pass);
    _rb.finish();

    const std::string _t = _cap.str();
    D_RRT_CHECK(!contains(_t, "wrote report:"));
    D_RRT_CHECK(!contains(_t, "report document was not written"));

    return true;
}


bool
tests_report_runner_table_pdf_write_failure_message()
{
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    // a path under a nonexistent directory: fopen fails, nothing is written
    const std::string _bad = "/nonexistent_dir_djinterp_xyz/report.pdf";
    _rb.use_pdf(_bad);
    _rb.module("m", "");
    _rb.run("t", &rr_pred_pass);
    _rb.finish();

    D_RRT_CHECK(contains(_cap.str(), "(report document was not written)"));
    D_RRT_CHECK(!file_exists(_bad));

    return true;
}


bool
tests_report_runner_table_pdf_silent_suppresses_message()
{
    console_capture    _cap;
    dt::report_builder _rb;
    _rb.set_file(_cap.f);

    // same unwritable path, but silent show suppresses the notice
    _rb.use_pdf("/nonexistent_dir_djinterp_xyz/report.pdf");
    _rb.set_show(dt::test_show::silent);
    _rb.module("m", "");
    _rb.run("t", &rr_pred_pass);
    _rb.finish();

    D_RRT_CHECK(!contains(_cap.str(), "report document was not written"));

    return true;
}


NS_END  // testing
NS_END  // djinterp
