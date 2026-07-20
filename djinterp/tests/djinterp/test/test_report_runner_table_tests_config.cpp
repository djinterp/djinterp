/******************************************************************************
* djinterp [tests]                    test_report_runner_table_tests_config.cpp
*
*   Construction and configuration: the framework-default options, the
* option-set constructor, the metadata setters, use_pdf, and the face-portable
* document / output_file / split / show setters - each verified through the
* builder's exposed option set and report.
*
* path:      /tests/djinterp/test/output/test_report_runner_table_tests_config.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

#include "test_report_runner_table_tests.hpp"


NS_DJINTERP
NS_TESTING


bool
tests_report_runner_table_default_options()
{
    dt::report_builder _rb;

    // framework defaults: plain-text document, whole-run split, show all
    D_RRT_CHECK(dt::document(_rb.options()) == dt::test_doc_type::txt);
    D_RRT_CHECK(dt::split(_rb.options())    == dt::test_output_split::whole_run);
    D_RRT_CHECK(dt::show(_rb.options())     == dt::test_show::all);

    // and an empty report to start
    D_RRT_CHECK(_rb.report().modules.empty());
    D_RRT_CHECK(_rb.report().title.empty());

    // the const options() / report() overloads read through identically
    const dt::report_builder& _cr = _rb;
    D_RRT_CHECK(dt::document(_cr.options()) == dt::test_doc_type::txt);
    D_RRT_CHECK(_cr.report().modules.empty());

    return true;
}


bool
tests_report_runner_table_explicit_options_ctor()
{
    // seed an option set through one builder, then adopt it into another
    dt::report_builder _seed;
    _seed.set_document(dt::test_doc_type::pdf);

    dt::report_builder _rb(_seed.options());

    D_RRT_CHECK(dt::document(_rb.options()) == dt::test_doc_type::pdf);

    return true;
}


bool
tests_report_runner_table_metadata_setters()
{
    dt::report_builder _rb;

    _rb.set_title("Framework Suite");
    _rb.set_subtitle("nightly");
    _rb.set_author("teer");
    _rb.set_description("the whole battery");
    _rb.set_notes("run on CI");

    D_RRT_CHECK(_rb.report().title       == "Framework Suite");
    D_RRT_CHECK(_rb.report().subtitle    == "nightly");
    D_RRT_CHECK(_rb.report().author      == "teer");
    D_RRT_CHECK(_rb.report().description == "the whole battery");
    D_RRT_CHECK(_rb.report().notes       == "run on CI");

    return true;
}


bool
tests_report_runner_table_use_pdf_variants()
{
    // no argument: select PDF, leave the output path empty, return *this
    {
        dt::report_builder  _rb;
        dt::report_builder& _ref = _rb.use_pdf();

        D_RRT_CHECK(&_ref == &_rb);
        D_RRT_CHECK(dt::document(_rb.options()) == dt::test_doc_type::pdf);
        D_RRT_CHECK(dt::output_path(_rb.options()).empty());
    }

    // with a path: select PDF and set the output path
    {
        dt::report_builder _rb;
        _rb.use_pdf("myreport.pdf");

        D_RRT_CHECK(dt::document(_rb.options()) == dt::test_doc_type::pdf);
        D_RRT_CHECK(dt::output_path(_rb.options()) == "myreport.pdf");
    }

    return true;
}


bool
tests_report_runner_table_set_document_output_file()
{
    dt::report_builder _rb;

    _rb.set_document(dt::test_doc_type::html);
    D_RRT_CHECK(dt::document(_rb.options()) == dt::test_doc_type::html);

    _rb.set_output_file("out.bin");
    D_RRT_CHECK(dt::output_path(_rb.options()) == "out.bin");

    return true;
}


bool
tests_report_runner_table_set_split_show()
{
    dt::report_builder _rb;

    _rb.set_split(dt::test_output_split::per_module);
    D_RRT_CHECK(dt::split(_rb.options()) == dt::test_output_split::per_module);

    _rb.set_show(dt::test_show::silent);
    D_RRT_CHECK(dt::show(_rb.options()) == dt::test_show::silent);

    return true;
}


NS_END  // testing
NS_END  // djinterp
