/*******************************************************************************
* djinterp [cpp][text]                                   markup_string_tests.cpp
*
*   The module "markup_string": its entry point, which opens a block per
* header and a test per group, and runs each group or skips it below C++11,
* the three headers' floor.
*
*
* path:      /tests/djinterp/cpp/text/markup_string_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "markup_string_tests.hpp"  // the suite's entry point, DTest's C face


// k_below
//   the reason a test is skipped below the headers' floor.
static const char* const k_below = "below C++11, the headers' floor";


/*
d_tests_markup_string_engine
  The block for markup_string_template.hpp.
*/
static void
d_tests_markup_string_engine(
    dj_test_suite* _suite
)
{
    (void)d_test_suite_block(_suite, "markup_string_template.hpp");

    d_tests_text_run(_suite, "escape",
        "the XML, HTML and null escape policies: the five characters, and "
        "everything else untouched",
        D_TESTS_TEXT_AT_CPP11(d_tests_markup_string_escape), k_below);
    d_tests_text_run(_suite, "variables",
        "{name} escaped, {&name} raw, blanks trimmed, a missing name empty; "
        "integers and booleans",
        D_TESTS_TEXT_AT_CPP11(d_tests_markup_string_variables), k_below);
    d_tests_text_run(_suite, "sections",
        "a truthy scalar renders its body once, a list once per item with "
        "the item as the context; nesting",
        D_TESTS_TEXT_AT_CPP11(d_tests_markup_string_sections), k_below);
    d_tests_text_run(_suite, "literals",
        "comments render nothing; an unterminated tag, and a close that "
        "closes nothing, are literal",
        D_TESTS_TEXT_AT_CPP11(d_tests_markup_string_literals), k_below);
    d_tests_text_run(_suite, "partials",
        "registered by value and by pointer, overridden by the context, "
        "nested, removed",
        D_TESTS_TEXT_AT_CPP11(d_tests_markup_string_partials), k_below);
    d_tests_text_run(_suite, "syntax",
        "the nine bundled syntaxes, explicit delimiters, no delimiters, and "
        "a directive class turned off",
        D_TESTS_TEXT_AT_CPP11(d_tests_markup_string_syntax), k_below);
    d_tests_text_run(_suite, "lifecycle",
        "source and set_source, render with no context, render_to, copies, "
        "the factories",
        D_TESTS_TEXT_AT_CPP11(d_tests_markup_string_lifecycle), k_below);

    return;
}


/*
d_tests_markup_string_all
  Binds the suite, opens the module, then each header's block and its
tests.
*/
void
d_tests_markup_string_all(
    dj_test_suite* _suite
)
{
    d_tests_text_bind(_suite);

    (void)d_test_suite_module(_suite, "markup_string");
    (void)d_test_suite_describe(_suite,
                                "text's markup string templates: the engine "
                                "and its HTML and XML facades");

    d_tests_markup_string_engine(_suite);

    (void)d_test_suite_block(_suite, "html/html_string_template.hpp");
    d_tests_text_run(_suite, "facade",
        "HTML's escaping, the base's types and size, the syntax aliases, the "
        "inherited constructors and the factories",
        D_TESTS_TEXT_AT_CPP11(d_tests_markup_string_html), k_below);

    (void)d_test_suite_block(_suite, "xml/xml_string_template.hpp");
    d_tests_text_run(_suite, "facade",
        "XML's escaping, the base's types and size, the syntax aliases, the "
        "inherited constructors and the factories",
        D_TESTS_TEXT_AT_CPP11(d_tests_markup_string_xml), k_below);

    d_tests_text_bind(NULL);

    return;
}
