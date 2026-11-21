/*******************************************************************************
* djinterp [test]                                                 html_tests.cpp
*
*   The module "html": its entry point, which opens a block per header and a
* test per group, and runs each group or skips it below its header's floor.
*
*
* path:      /tests/djinterp/cpp/text/html_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "html_tests.hpp"  // the suite's entry point, DTest's C face


/*
d_tests_html_vocabulary_blocks
  The blocks for html.hpp (C++11) and html_dialect.hpp (C++14).
*/
static void
d_tests_html_vocabulary_blocks(
    dj_test_suite* _suite
)
{
    static const char* const below11 = "below C++11, html.hpp's floor";
    static const char* const below14 = "below C++14, html_dialect.hpp's floor";

    (void)d_test_suite_block(_suite, "html/html.hpp");
    d_tests_text_run(_suite, "versions",
        "the versions' DOCTYPEs and which are XHTML; the defaults",
        D_TESTS_TEXT_AT_CPP11(d_tests_html_versions), below11);
    d_tests_text_run(_suite, "kinds",
        "every element kind round-trips through its tag name, at run time; "
        "unknown names and kinds",
        D_TESTS_TEXT_AT_CPP11(d_tests_html_kinds), below11);
    d_tests_text_run(_suite, "categories",
        "the void elements are HTML's fourteen; raw text, block, inline, "
        "form and metadata; by kind and by name",
        D_TESTS_TEXT_AT_CPP11(d_tests_html_categories), below11);
    d_tests_text_run(_suite, "traits",
        "backends, elements and documents classified by what they expose; "
        "the _v forms and the concepts",
        D_TESTS_TEXT_AT_CPP11(d_tests_html_traits), below11);

    (void)d_test_suite_block(_suite, "html/html_dialect.hpp");
    d_tests_text_run(_suite, "families",
        "each dialect's family, strictness, frameset and XHTML-ness, and "
        "its XML dialect",
        D_TESTS_TEXT_AT_CPP14(d_tests_html_dialect_families), below14);
    d_tests_text_run(_suite, "rules",
        "emission rules, DOCTYPEs, valid and void elements per dialect, and "
        "is_html_dialect",
        D_TESTS_TEXT_AT_CPP14(d_tests_html_dialect_rules), below14);

    return;
}


/*
d_tests_html_all
  Binds the suite, opens the module, then each header's block and its
tests.
*/
void
d_tests_html_all(
    dj_test_suite* _suite
)
{
    d_tests_text_bind(_suite);

    (void)d_test_suite_module(_suite, "html");
    (void)d_test_suite_describe(_suite,
                                "text's HTML headers: vocabulary and traits, "
                                "dialects, the print policy, and the tree");

    d_tests_html_vocabulary_blocks(_suite);

    (void)d_test_suite_block(_suite, "html/html_printer.hpp");
    d_tests_text_run(_suite, "policy",
        "DHtmlPrintPolicy's escaping and element forms, and a document "
        "printed through it",
        D_TESTS_TEXT_AT_CPP17(d_tests_html_printer),
        "below C++17, html_printer.hpp's floor");

    (void)d_test_suite_block(_suite, "html/html_template.hpp");
    d_tests_text_run(_suite, "elements",
        "kinds, ids and classes on DHtmlElement",
        D_TESTS_TEXT_AT_CPP20(d_tests_html_tree_elements),
        "below C++20, html_template.hpp's floor");
    d_tests_text_run(_suite, "documents",
        "versions and DOCTYPEs, head, body and title, and the two document "
        "factories",
        D_TESTS_TEXT_AT_CPP20(d_tests_html_tree_documents),
        "below C++20, html_template.hpp's floor");
    d_tests_text_run(_suite, "write",
        "the serialised text: void elements, empty and raw-text elements, "
        "escaping, XHTML's slash",
        D_TESTS_TEXT_AT_CPP20(d_tests_html_tree_write),
        "below C++20, html_template.hpp's floor");
    d_tests_text_run(_suite, "traits",
        "the tree's own types meet html.hpp's protocol, at no cost in size",
        D_TESTS_TEXT_AT_CPP20(d_tests_html_tree_traits),
        "below C++20, html_template.hpp's floor");
    d_tests_text_run(_suite, "beside the printer",
        "the trees and html_printer.hpp's printed documents share a unit, "
        "each name meaning its own type",
        D_TESTS_TEXT_AT_CPP20(d_tests_html_tree_beside_printer),
        "below C++20, html_template.hpp's floor");
    d_tests_text_run(_suite, "factories",
        "the seven factories return handles owning what they name; an "
        "unadopted one frees it",
        D_TESTS_TEXT_AT_CPP20(d_tests_html_detached_factories),
        "below C++20, html_template.hpp's floor");
    d_tests_text_run(_suite, "adopt",
        "adopt takes a handle only, builds then places, and refuses an "
        "empty parent or handle, or a cycle",
        D_TESTS_TEXT_AT_CPP20(d_tests_html_detached_adopt),
        "below C++20, html_template.hpp's floor");

    d_tests_text_bind(NULL);

    return;
}
