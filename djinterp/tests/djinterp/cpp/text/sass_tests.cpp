/*******************************************************************************
* djinterp [cpp][text]                                            sass_tests.cpp
*
*   The module "sass": its entry point, which opens a block per header and a
* test per group, and runs each group or skips it below its header's floor.
*
*
* path:      /tests/djinterp/cpp/text/sass_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "sass_tests.hpp"  // the suite's entry point, DTest's C face


/*
d_tests_sass_all
  Binds the suite, opens the module, then each header's block and its
tests.
*/
void
d_tests_sass_all(
    dj_test_suite* _suite
)
{
    static const char* const below17 = "below C++17, the Sass headers' floor";

    d_tests_text_bind(_suite);

    (void)d_test_suite_module(_suite, "sass");
    (void)d_test_suite_describe(_suite,
                                "text's Sass headers: vocabulary, the "
                                "stylesheet in both syntaxes, the printer");

    (void)d_test_suite_block(_suite, "css/sass/sass.hpp");
    d_tests_text_run(_suite, "vocabulary",
        "rule categories, at-keywords both ways and their rule kinds, "
        "variable and placeholder names",
        D_TESTS_TEXT_AT_CPP17(d_tests_sass_vocabulary), below17);

    (void)d_test_suite_block(_suite, "css/sass/sass_template.hpp");
    d_tests_text_run(_suite, "build",
        "variables, mixins, includes, modules and control flow, read back "
        "through the facade",
        D_TESTS_TEXT_AT_CPP17(d_tests_sass_sheet_build), below17);
    d_tests_text_run(_suite, "scss",
        "SCSS source: !default, quoted URLs, blocks that Sass requires, "
        "comments, keyframes",
        D_TESTS_TEXT_AT_CPP17(d_tests_sass_sheet_scss), below17);
    d_tests_text_run(_suite, "indented",
        "the indented syntax: no braces or semicolons, comments that keep "
        "their later lines",
        D_TESTS_TEXT_AT_CPP17(d_tests_sass_sheet_indented), below17);
    d_tests_text_run(_suite, "factories",
        "each factory returns a handle owning what it names; an unadopted "
        "one frees it",
        D_TESTS_TEXT_AT_CPP17(d_tests_sass_detached_factories), below17);
    d_tests_text_run(_suite, "adopt",
        "adopt takes a handle only, its module's or CSS's, and refuses an "
        "empty parent or handle, or a cycle",
        D_TESTS_TEXT_AT_CPP17(d_tests_sass_detached_adopt), below17);

    (void)d_test_suite_block(_suite, "css/sass/sass_printer.hpp");
    d_tests_text_run(_suite, "printer",
        "the printer's sink and the string and stream helpers, in each "
        "render mode",
        D_TESTS_TEXT_AT_CPP17(d_tests_sass_printer), below17);

    d_tests_text_bind(NULL);

    return;
}
