/*******************************************************************************
* djinterp [test]                                    markdown_tests_detached.cpp
*
*   The groups for markdown/markdown_template.hpp's handles: what its twenty
* factories return, what add_block and add_inline take and refuse, and the
* setters for what a factory can't say: a list's looseness, a task item's
* box and a table's alignments. A
* block's handle (DMarkdownDetachedBlock) and an inline's
* (DMarkdownDetachedInline) are different types, so a block can't be added
* as an inline, nor an inline as a block. A handle left unadopted here is
* freed when it goes, which the suite's runs under AddressSanitizer and
* LeakSanitizer confirm.
*
*
* path:      /tests/djinterp/cpp/text/markdown_tests_detached.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "markdown_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups, builders
// std
#include <string>  // std::string
#include <vector>  // std::vector
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::false_type, std::is_same, std::true_type
    #include <utility>      // std::declval, std::move
#endif


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_markdown_adds_block, d_tests_markdown_adds_inline
  True when Parent's add_block, or add_inline, takes a Child: compile-time
checks of what adoption refuses.
*/
template<typename Parent,
         typename Child,
         typename = void>
struct d_tests_markdown_adds_block
:   public std::false_type
{};

template<typename Parent,
         typename Child>
struct d_tests_markdown_adds_block<Parent,
                                   Child,
                                   decltype((void)std::declval<Parent&>()
                                                .add_block(
                                                    std::declval<Child>()))>
:   public std::true_type
{};

template<typename Parent,
         typename Child,
         typename = void>
struct d_tests_markdown_adds_inline
:   public std::false_type
{};

template<typename Parent,
         typename Child>
struct d_tests_markdown_adds_inline<Parent,
                                    Child,
                                    decltype((void)std::declval<Parent&>()
                                                 .add_inline(
                                                     std::declval<Child>()))>
:   public std::true_type
{};

// D_TESTS_MARKDOWN_RETURNS
//   true when a factory's call has the handle type given.
#define D_TESTS_MARKDOWN_RETURNS(handle, call)                                 \
    (std::is_same<handle, decltype(djinterp::markdown::call)>::value)


/*
d_tests_markdown_detached_handles
  Tests the following:
  - the twelve block factories return a block's handle, and the eight
    inline factories an inline's; the five table factories make their kinds
  - each handle owns a parentless node, reached through get(), the arrow
    and backend_handle(); one never adopted frees its node
  - an inline's handle can be built under (a link's text) before it's
    placed
*/
void
d_tests_markdown_detached_handles(void)
{
    using namespace djinterp::markdown;

    const bool blocks =
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_paragraph<md_backend>()) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_heading<md_backend>(1)) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_fenced_code_block<md_backend>("")) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_thematic_break<md_backend>()) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_block_quote<md_backend>()) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_list<md_backend>(false)) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_list_item<md_backend>()) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_table<md_backend>()) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_table_header<md_backend>()) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_table_body<md_backend>()) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_table_row<md_backend>()) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_block,
                                 make_table_cell<md_backend>());
    const bool inlines =
        D_TESTS_MARKDOWN_RETURNS(md_new_inline,
                                 make_text<md_backend>("")) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_inline,
                                 make_emphasis<md_backend>()) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_inline,
                                 make_strong<md_backend>()) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_inline,
                                 make_code_span<md_backend>("")) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_inline,
                                 make_link<md_backend>("")) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_inline,
                                 make_image<md_backend>("")) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_inline,
                                 make_autolink<md_backend>("")) &&
        D_TESTS_MARKDOWN_RETURNS(md_new_inline,
                                 make_hard_break<md_backend>());

    D_TESTS_TEXT_CHECK(blocks);
    D_TESTS_TEXT_CHECK(inlines);

    md_new_block  quote = make_block_quote<md_backend>();
    md_new_inline link  = make_link<md_backend>("u");
    md_new_inline lost  = make_text<md_backend>("never placed");

    D_TESTS_TEXT_CHECK(quote.valid() &&
                       (quote->block_kind() ==
                        DMarkdownBlockKind::block_quote) &&
                       (quote.get().backend_handle() ==
                        quote.backend_handle()) &&
                       (quote.backend_handle()->parent == nullptr));
    D_TESTS_TEXT_CHECK(link->add_inline(d_tests_markdown_text("site"))
                           .valid() &&
                       (link->child_count() == 1u) &&
                       (link->inline_child_at(0).text() == "site"));
    D_TESTS_TEXT_CHECK(lost.valid() && (lost->text() == "never placed"));
    D_TESTS_TEXT_CHECK(
        (make_table<md_backend>()->block_kind() == DMarkdownBlockKind::table) &&
        (make_table_header<md_backend>()->block_kind() ==
         DMarkdownBlockKind::table_header) &&
        (make_table_body<md_backend>()->block_kind() ==
         DMarkdownBlockKind::table_body) &&
        (make_table_row<md_backend>()->block_kind() ==
         DMarkdownBlockKind::table_row) &&
        (make_table_cell<md_backend>()->block_kind() ==
         DMarkdownBlockKind::table_cell));

    return;
}


/*
d_tests_markdown_detached_setters
  Tests the following:
  - make_table keeps the alignments it's given; set_column_alignments
    replaces them; set_column_alignment sets one column, leaving the
    columns before it that had none unaligned
  - set_loose makes a list loose and tight again
  - set_task gives an item its box, ticked or not; clear_task takes it
    away; is_task and is_checked read it
  - each setter does nothing on an empty facade
*/
void
d_tests_markdown_detached_setters(void)
{
    using namespace djinterp::markdown;

    typedef std::vector<DMarkdownTableAlignment> aligns;

    const DMarkdownTableAlignment left   = DMarkdownTableAlignment::left;
    const DMarkdownTableAlignment center = DMarkdownTableAlignment::center;
    const DMarkdownTableAlignment none   = DMarkdownTableAlignment::none;

    md_new_block table = make_table<md_backend>(aligns(2u, left));
    md_new_block list  = make_list<md_backend>(false);
    md_new_block item  = make_list_item<md_backend>();
    md_block     nothing;

    D_TESTS_TEXT_CHECK(table->column_alignments() == aligns(2u, left));
    table->set_column_alignments(aligns(1u, center));
    D_TESTS_TEXT_CHECK(table->column_alignments() == aligns(1u, center));
    table->set_column_alignment(3u, left);
    D_TESTS_TEXT_CHECK((table->column_alignments().size() == 4u) &&
                       (table->column_alignments()[0] == center) &&
                       (table->column_alignments()[1] == none) &&
                       (table->column_alignments()[2] == none) &&
                       (table->column_alignments()[3] == left));
    table->set_column_alignment(0u, none);
    D_TESTS_TEXT_CHECK((table->column_alignments().size() == 4u) &&
                       (table->column_alignments()[0] == none));

    D_TESTS_TEXT_CHECK(!list->is_loose());
    list->set_loose(true);
    D_TESTS_TEXT_CHECK(list->is_loose());
    list->set_loose(false);
    D_TESTS_TEXT_CHECK(!list->is_loose());

    D_TESTS_TEXT_CHECK(!item->is_task() && !item->is_checked());
    item->set_task(false);
    D_TESTS_TEXT_CHECK(item->is_task() && !item->is_checked());
    item->set_task(true);
    D_TESTS_TEXT_CHECK(item->is_task() && item->is_checked());
    item->clear_task();
    D_TESTS_TEXT_CHECK(!item->is_task() && !item->is_checked());

    nothing.set_loose(true);
    nothing.set_task(true);
    nothing.clear_task();
    nothing.set_column_alignments(aligns(1u, left));
    nothing.set_column_alignment(1u, left);
    D_TESTS_TEXT_CHECK(!nothing.is_loose() && !nothing.is_task() &&
                       nothing.column_alignments().empty());

    return;
}


/*
d_tests_markdown_detached_adopt
  Tests the following:
  - add_block, a block's add_inline and an inline's add_inline each
    append, set the parent, empty the handle and return the child's facade
  - each refuses, with an empty facade and the handle keeping its node: an
    empty parent, an empty handle, the handle's own node, and a node under
    it
  - a facade, a handle not moved, or the other kind's handle doesn't
    compile as their argument
*/
void
d_tests_markdown_detached_adopt(void)
{
    using namespace djinterp::markdown;

    md_document   doc;
    md_block      root  = doc.root_block();
    md_new_block  quote = make_block_quote<md_backend>();
    md_block      held  = quote.get();
    md_block      para  = quote->add_block(make_paragraph<md_backend>());
    md_new_inline em    = make_emphasis<md_backend>();
    md_inline     emph  = em.get();
    md_inline     text  = em->add_inline(d_tests_markdown_text("t"));

    D_TESTS_TEXT_CHECK(para.valid() &&
                       (para.block_kind() == DMarkdownBlockKind::paragraph) &&
                       (para.backend_handle()->parent ==
                        held.backend_handle()) &&
                       (held.child_count() == 1u));
    D_TESTS_TEXT_CHECK(text.valid() && (text.text() == "t") &&
                       (text.backend_handle()->parent ==
                        emph.backend_handle()));

    D_TESTS_TEXT_CHECK(!md_block().add_block(std::move(quote)).valid() &&
                       quote.valid());
    D_TESTS_TEXT_CHECK(!held.add_block(std::move(quote)).valid() &&
                       quote.valid());
    D_TESTS_TEXT_CHECK(!para.add_block(std::move(quote)).valid() &&
                       quote.valid());
    D_TESTS_TEXT_CHECK(!md_inline().add_inline(std::move(em)).valid() &&
                       em.valid());
    D_TESTS_TEXT_CHECK(!emph.add_inline(std::move(em)).valid() &&
                       !text.add_inline(std::move(em)).valid() &&
                       em.valid());
    D_TESTS_TEXT_CHECK(!para.add_inline(md_new_inline()).valid() &&
                       !emph.add_inline(md_new_inline()).valid());

    D_TESTS_TEXT_CHECK((para.add_inline(std::move(em)).backend_handle() ==
                        emph.backend_handle()) &&
                       !em.valid() &&
                       (emph.backend_handle()->parent ==
                        para.backend_handle()));
    D_TESTS_TEXT_CHECK((root.add_block(std::move(quote)).backend_handle() ==
                        held.backend_handle()) &&
                       !quote.valid() && (root.child_count() == 1u));

    const bool blocks =
        (d_tests_markdown_adds_block<md_block, md_new_block>::value) &&
        (!d_tests_markdown_adds_block<md_block, md_new_block&>::value) &&
        (!d_tests_markdown_adds_block<md_block, md_block>::value) &&
        (!d_tests_markdown_adds_block<md_block, md_new_inline>::value);
    const bool inlines =
        (d_tests_markdown_adds_inline<md_block, md_new_inline>::value) &&
        (d_tests_markdown_adds_inline<md_inline, md_new_inline>::value) &&
        (!d_tests_markdown_adds_inline<md_block, md_new_inline&>::value) &&
        (!d_tests_markdown_adds_inline<md_inline, md_inline>::value) &&
        (!d_tests_markdown_adds_inline<md_block, md_new_block>::value) &&
        (!d_tests_markdown_adds_inline<md_inline, md_new_block>::value);

    D_TESTS_TEXT_CHECK(blocks);
    D_TESTS_TEXT_CHECK(inlines);

    return;
}

#endif  // C++11
