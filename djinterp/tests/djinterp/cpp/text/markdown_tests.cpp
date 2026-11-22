/*******************************************************************************
* djinterp [cpp][text]                                        markdown_tests.cpp
*
*   The module "markdown": its entry point, which opens a block per header
* and a test per group, and runs each group or skips it below its header's
* floor.
*
*
* path:      /tests/djinterp/cpp/text/markdown_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "markdown_tests.hpp"  // the suite's entry point, DTest's C face


/*
d_tests_markdown_template_block
  The block for markdown_template.hpp: building a tree, and its renderings.
*/
static void
d_tests_markdown_template_block(
    dj_test_suite* _suite
)
{
    static const char* const below11 =
        "below C++11, markdown_template.hpp's floor";

    (void)d_test_suite_block(_suite, "markdown/markdown_template.hpp");
    d_tests_text_run(_suite, "build",
        "the factories make what they name; blocks and inlines adopt them, "
        "and the facades read them back",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_tree_build), below11);
    d_tests_text_run(_suite, "handles",
        "the block factories, tables' included, return a block's handle, the "
        "inline ones an inline's; an unadopted one frees its node",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_detached_handles), below11);
    d_tests_text_run(_suite, "adopt",
        "add_block and add_inline take their own kind's handle only, and "
        "refuse an empty parent or handle, or a cycle",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_detached_adopt), below11);
    d_tests_text_run(_suite, "setters",
        "a table's alignments, a list's looseness, a task item's box: set, "
        "read back, and ignored on an empty facade",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_detached_setters), below11);
    d_tests_text_run(_suite, "markdown",
        "text, code, links and blocks written to read back as the same "
        "tree: escaping, fences, destinations",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_render_markdown), below11);
    d_tests_text_run(_suite, "lists",
        "ordered lists numbered from their start, nested and multi-block "
        "items indented, tight and loose",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_render_lists), below11);
    d_tests_text_run(_suite, "html",
        "CommonMark's and GFM's HTML: tight items, task boxes, tables' "
        "header cells and alignment, escaping",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_render_html), below11);
    d_tests_text_run(_suite, "plaintext",
        "the text alone: a block a line, a table's cells by tabs, an "
        "image's alternative text",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_render_plaintext), below11);
    d_tests_text_run(_suite, "xml",
        "CommonMark's XML as cmark writes it: elements, attributes, text "
        "kept as it is",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_render_xml), below11);

    return;
}


/*
d_tests_markdown_all
  Binds the suite, opens the module, then each header's block and its
tests.
*/
void
d_tests_markdown_all(
    dj_test_suite* _suite
)
{
    static const char* const below11 = "below C++11, markdown.hpp's floor";

    d_tests_text_bind(_suite);

    (void)d_test_suite_module(_suite, "markdown");
    (void)d_test_suite_describe(_suite,
                                "text's Markdown headers: vocabulary, the "
                                "tree and its renderings, traits");

    (void)d_test_suite_block(_suite, "markdown/markdown.hpp");
    d_tests_text_run(_suite, "blocks",
        "each block kind's categories, and the predicates over them",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_blocks), below11);
    d_tests_text_run(_suite, "headings",
        "heading kinds and levels both ways, at compile and run time",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_headings), below11);
    d_tests_text_run(_suite, "inlines",
        "each inline kind's categories, and the predicates over them",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_inlines), below11);
    d_tests_text_run(_suite, "backends",
        "the backend tags, and is_markdown_backend",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_backends), below11);

    d_tests_markdown_template_block(_suite);

    (void)d_test_suite_block(_suite, "markdown/markdown_template_traits.hpp");
    d_tests_text_run(_suite, "traits",
        "the tree's facades, document and backend meet the protocol "
        "traits; probes that don't, don't",
        D_TESTS_TEXT_AT_CPP11(d_tests_markdown_traits),
        "below C++11, markdown_template_traits.hpp's floor");
    d_tests_text_run(_suite, "concepts",
        "the concepts agree with the traits",
        D_TESTS_TEXT_AT_CPP20(d_tests_markdown_concepts),
        "below C++20, which concepts need");

    d_tests_text_bind(NULL);

    return;
}
