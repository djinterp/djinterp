/*******************************************************************************
* djinterp [cpp][text]                                             css_tests.cpp
*
*   The module "css": its entry point, which opens a block per header and a
* test per group, and runs each group or skips it below its header's floor.
*
*
* path:      /tests/djinterp/cpp/text/css_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "css_tests.hpp"  // the suite's entry point, DTest's C face


/*
d_tests_css_all
  Binds the suite, opens the module, then each header's block and its
tests.
*/
void
d_tests_css_all(
    dj_test_suite* _suite
)
{
    static const char* const below17 = "below C++17, the CSS headers' floor";

    d_tests_text_bind(_suite);

    (void)d_test_suite_module(_suite, "css");
    (void)d_test_suite_describe(_suite,
                                "text's CSS headers: vocabulary, the "
                                "stylesheet and its renderings, the printer");

    (void)d_test_suite_block(_suite, "css/css.hpp");
    d_tests_text_run(_suite, "rules",
        "each rule kind's categories, and the predicates over them",
        D_TESTS_TEXT_AT_CPP17(d_tests_css_rules), below17);
    d_tests_text_run(_suite, "at-rules",
        "every at-rule's keyword both ways, in any case, and its rule kind",
        D_TESTS_TEXT_AT_CPP17(d_tests_css_at_rules), below17);
    d_tests_text_run(_suite, "names",
        "custom properties, the name constants, and backend detection",
        D_TESTS_TEXT_AT_CPP17(d_tests_css_names), below17);

    (void)d_test_suite_block(_suite, "css/css_template.hpp");
    d_tests_text_run(_suite, "build",
        "rules and declarations added, found, replaced and removed; nested "
        "rules; the facades read back",
        D_TESTS_TEXT_AT_CPP17(d_tests_css_sheet_build), below17);
    d_tests_text_run(_suite, "render",
        "formatted, minified and SCSS output; !important; nesting; "
        "comments that can't end early",
        D_TESTS_TEXT_AT_CPP17(d_tests_css_sheet_render), below17);
    d_tests_text_run(_suite, "at-rules",
        "statements, blocks, descriptors, keyframes and unknown at-rules "
        "with blocks",
        D_TESTS_TEXT_AT_CPP17(d_tests_css_sheet_at_rules), below17);
    d_tests_text_run(_suite, "factories",
        "each factory returns a handle owning what it names; an unadopted "
        "one frees it",
        D_TESTS_TEXT_AT_CPP17(d_tests_css_detached_factories), below17);
    d_tests_text_run(_suite, "adopt",
        "adopt takes a handle only, its module's or CSS's, and refuses an "
        "empty parent or handle, or a cycle",
        D_TESTS_TEXT_AT_CPP17(d_tests_css_detached_adopt), below17);

    (void)d_test_suite_block(_suite, "css/css_printer.hpp");
    d_tests_text_run(_suite, "printer",
        "the printer's sink, the string and stream helpers, and a file, in "
        "each render mode",
        D_TESTS_TEXT_AT_CPP17(d_tests_css_printer), below17);

    d_tests_text_bind(NULL);

    return;
}
