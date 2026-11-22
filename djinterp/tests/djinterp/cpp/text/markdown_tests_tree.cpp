/*******************************************************************************
* djinterp [cpp][text]                                   markdown_tests_tree.cpp
*
*   The group for building a tree with markdown/markdown_template.hpp: the
* factories, adoption by blocks and inlines, and the facades that read the
* nodes back. The handles the factories return, and what adoption refuses,
* are markdown_tests_detached.cpp's.
*
*
* path:      /tests/djinterp/cpp/text/markdown_tests_tree.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "markdown_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups, builders


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_markdown_tree_build
  Tests the following:
  - a document starts as an empty document block, in CommonMark unless a
    flavour is given, and a flavour can be changed
  - each factory makes the kind it names, with the fields it's given
  - add_block and add_inline adopt: the child is counted, reached by
    index, and knows its parent
  - a null facade reads as empty; adopting into one, or an empty handle,
    is refused, with an empty facade
*/
void
d_tests_markdown_tree_build(void)
{
    using namespace djinterp::markdown;

    md_document doc;
    md_document gfm(DMarkdownFlavor::github);
    md_block    root = doc.root_block();

    D_TESTS_TEXT_CHECK(root.valid() &&
                       (root.block_kind() == DMarkdownBlockKind::document));
    D_TESTS_TEXT_CHECK(root.child_count() == 0u);
    D_TESTS_TEXT_CHECK(doc.flavor() == DMarkdownFlavor::commonmark);
    D_TESTS_TEXT_CHECK(gfm.get_flavor() == DMarkdownFlavor::github);
    gfm.set_flavor(DMarkdownFlavor::pandoc);
    D_TESTS_TEXT_CHECK(gfm.flavor() == DMarkdownFlavor::pandoc);

    root.add_block(make_heading<md_backend>(9));
    root.add_block(make_fenced_code_block<md_backend>("x = 1\n", "py"));
    root.add_block(make_list<md_backend>(true, 4));
    root.add_block(d_tests_markdown_paragraph("hello"));

    const md_block heading = root.block_child_at(0);
    const md_block code    = root.block_child_at(1);
    const md_block list    = root.block_child_at(2);
    md_block       para    = root.block_child_at(3);

    D_TESTS_TEXT_CHECK(root.child_count() == 4u);
    D_TESTS_TEXT_CHECK(heading.is_heading() && (heading.heading_level() == 6));
    D_TESTS_TEXT_CHECK(code.is_code() && (code.language() == "py") &&
                       (code.info_string() == "py") &&
                       (code.text() == "x = 1\n"));
    D_TESTS_TEXT_CHECK(list.is_list() && list.is_ordered() &&
                       (list.list_start() == 4) && !list.is_loose());
    D_TESTS_TEXT_CHECK(para.backend_handle()->parent ==
                       root.backend_handle());

    const md_inline text = para.inline_child_at(0);

    D_TESTS_TEXT_CHECK(text.valid() && (text.text() == "hello"));
    D_TESTS_TEXT_CHECK(text.backend_handle()->parent ==
                       para.backend_handle());

    md_new_inline link = make_link<md_backend>("https://example.com/", "T");

    link->add_inline(d_tests_markdown_text("site"));
    D_TESTS_TEXT_CHECK((link->url() == "https://example.com/") &&
                       (link->title() == "T") && (link->child_count() == 1u));
    para.add_inline(std::move(link));
    D_TESTS_TEXT_CHECK(para.child_count() == 2u);

    md_block nothing;

    D_TESTS_TEXT_CHECK(!nothing.valid() && (nothing.child_count() == 0u) &&
                       nothing.text().empty() &&
                       !nothing.block_child_at(0).valid());
    D_TESTS_TEXT_CHECK(!nothing.add_block(md_new_block()).valid());
    D_TESTS_TEXT_CHECK(!root.add_block(md_new_block()).valid());
    D_TESTS_TEXT_CHECK(root.child_count() == 4u);
    D_TESTS_TEXT_CHECK(!root.block_child_at(4).valid() &&
                       !root.inline_child_at(9).valid());

    return;
}

#endif  // C++11
