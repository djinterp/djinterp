/*******************************************************************************
* djinterp [cpp][text]                                            less_tests.cpp
*
*   The module "less": its entry point, which opens a block per header and a
* test per group, and runs each group or skips it below its header's floor.
*
*
* path:      /tests/djinterp/cpp/text/less_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "less_tests.hpp"  // the suite's entry point, DTest's C face


/*
d_tests_less_all
  Binds the suite, opens the module, then each header's block and its
tests.
*/
void
d_tests_less_all(
    dj_test_suite* _suite
)
{
    static const char* const below17 = "below C++17, the Less headers' floor";

    d_tests_text_bind(_suite);

    (void)d_test_suite_module(_suite, "less");
    (void)d_test_suite_describe(_suite,
                                "text's Less headers: vocabulary, the "
                                "stylesheet and its source");

    (void)d_test_suite_block(_suite, "css/less/less.hpp");
    d_tests_text_run(_suite, "vocabulary",
        "rule categories, at-keywords in any case, variable and mixin "
        "names, backend tags",
        D_TESTS_TEXT_AT_CPP17(d_tests_less_vocabulary), below17);

    (void)d_test_suite_block(_suite, "css/less/less_template.hpp");
    d_tests_text_run(_suite, "build",
        "variables, mixins with parameters and guards, imports and "
        "plugins, read back through the facade",
        D_TESTS_TEXT_AT_CPP17(d_tests_less_sheet_build), below17);
    d_tests_text_run(_suite, "source",
        "the Less source: declarations, guards, quoted URLs, comments, "
        "keyframes and at-rules",
        D_TESTS_TEXT_AT_CPP17(d_tests_less_sheet_render), below17);
    d_tests_text_run(_suite, "factories",
        "each factory returns a handle owning what it names; an unadopted "
        "one frees it",
        D_TESTS_TEXT_AT_CPP17(d_tests_less_detached_factories), below17);
    d_tests_text_run(_suite, "adopt",
        "adopt takes a handle only, its module's or CSS's, and refuses an "
        "empty parent or handle, or a cycle",
        D_TESTS_TEXT_AT_CPP17(d_tests_less_detached_adopt), below17);

    d_tests_text_bind(NULL);

    return;
}
