/*******************************************************************************
* djinterp [cpp][text]                             markdown_tests_vocabulary.cpp
*
*   The groups for markdown/markdown.hpp: the block and inline kinds' category
* masks and the predicates over them, headings' kinds and levels both ways,
* and the backend tags. Each predicate is checked at compile time, where it
* is constant, and at run time, through a volatile, where it is a call.
*
*
* path:      /tests/djinterp/cpp/text/markdown_tests_vocabulary.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "markdown_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::is_base_of
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/markdown/markdown.hpp"
                                                      // the vocabulary


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// md_block_kind, md_inline_kind
//   the kinds, shortly.
typedef djinterp::markdown::DMarkdownBlockKind  md_block_kind;
typedef djinterp::markdown::DMarkdownInlineKind md_inline_kind;

// a constant predicate holds at compile time
static_assert(djinterp::markdown::is_heading_block_kind(
                  md_block_kind::heading_3),
              "heading_3 is a heading");
static_assert(djinterp::markdown::heading_level_from_kind(
                  md_block_kind::heading_6) == 6,
              "heading_6 is level 6");
static_assert(!djinterp::markdown::is_container_inline_kind(
                  md_inline_kind::code_span),
              "a code span wraps nothing");


/*
d_tests_markdown_runtime_block
  A block kind that the compiler can't see through.
*/
static md_block_kind
d_tests_markdown_runtime_block(
    md_block_kind _kind
)
{
    volatile md_block_kind kind = _kind;

    return kind;
}

/*
d_tests_markdown_runtime_inline
  An inline kind that the compiler can't see through.
*/
static md_inline_kind
d_tests_markdown_runtime_inline(
    md_inline_kind _kind
)
{
    volatile md_inline_kind kind = _kind;

    return kind;
}


/*
d_tests_markdown_blocks
  Tests the following:
  - the document, a quote, the lists, items and the table's parts hold
    blocks; headings, paragraphs and table cells hold inlines
  - code is a leaf: fenced, indented and math, which is code too
  - the predicates select by category, not by name: a list item is not a
    list, a task item is an item, a table cell is a table's
  - raw and metadata blocks, and unknown, which has no category
*/
void
d_tests_markdown_blocks(void)
{
    using djinterp::markdown::markdown_category_for_block_kind;
    using djinterp::markdown::is_container_block_kind;
    using djinterp::markdown::is_leaf_block_kind;
    using djinterp::markdown::is_code_block_kind;
    using djinterp::markdown::is_list_block_kind;
    using djinterp::markdown::is_list_item_block_kind;
    using djinterp::markdown::is_table_block_kind;
    using djinterp::markdown::mbc_block_holder;
    using djinterp::markdown::mbc_inline_holder;
    using djinterp::markdown::mbc_metadata;
    using djinterp::markdown::mbc_none;
    using djinterp::markdown::mbc_raw;

    const md_block_kind document  =
        d_tests_markdown_runtime_block(md_block_kind::document);
    const md_block_kind paragraph =
        d_tests_markdown_runtime_block(md_block_kind::paragraph);
    const md_block_kind cell      =
        d_tests_markdown_runtime_block(md_block_kind::table_cell);

    D_TESTS_TEXT_CHECK(is_container_block_kind(document));
    D_TESTS_TEXT_CHECK((markdown_category_for_block_kind(document) &
                        mbc_block_holder) != 0u);
    D_TESTS_TEXT_CHECK(is_leaf_block_kind(paragraph) &&
                       !is_container_block_kind(paragraph));
    D_TESTS_TEXT_CHECK((markdown_category_for_block_kind(paragraph) &
                        mbc_inline_holder) != 0u);
    D_TESTS_TEXT_CHECK(is_table_block_kind(cell) && is_leaf_block_kind(cell));
    D_TESTS_TEXT_CHECK(is_container_block_kind(md_block_kind::block_quote));
    D_TESTS_TEXT_CHECK(is_container_block_kind(md_block_kind::table_row));

    D_TESTS_TEXT_CHECK(is_code_block_kind(md_block_kind::fenced_code_block));
    D_TESTS_TEXT_CHECK(is_code_block_kind(md_block_kind::indented_code_block));
    D_TESTS_TEXT_CHECK(is_code_block_kind(md_block_kind::math_block));
    D_TESTS_TEXT_CHECK(!is_code_block_kind(md_block_kind::html_block));

    D_TESTS_TEXT_CHECK(is_list_block_kind(md_block_kind::ordered_list));
    D_TESTS_TEXT_CHECK(is_list_block_kind(md_block_kind::unordered_list));
    D_TESTS_TEXT_CHECK(!is_list_block_kind(md_block_kind::list_item));
    D_TESTS_TEXT_CHECK(is_list_item_block_kind(md_block_kind::list_item));
    D_TESTS_TEXT_CHECK(is_list_item_block_kind(
                           md_block_kind::task_list_item));
    D_TESTS_TEXT_CHECK(!is_list_item_block_kind(md_block_kind::ordered_list));

    D_TESTS_TEXT_CHECK((markdown_category_for_block_kind(
                            md_block_kind::html_block) & mbc_raw) != 0u);
    D_TESTS_TEXT_CHECK((markdown_category_for_block_kind(
                            md_block_kind::yaml_front_matter) &
                        mbc_metadata) != 0u);
    D_TESTS_TEXT_CHECK(markdown_category_for_block_kind(
                           d_tests_markdown_runtime_block(
                               md_block_kind::unknown)) == mbc_none);
    D_TESTS_TEXT_CHECK(!is_leaf_block_kind(md_block_kind::unknown) &&
                       !is_container_block_kind(md_block_kind::unknown));

    return;
}


/*
d_tests_markdown_headings
  Tests the following:
  - heading_level_from_kind gives 1 to 6 for the six heading kinds, and 0
    for any other
  - heading_kind_from_level is its inverse on 1 to 6, and gives unknown
    outside them
  - both, at run time, for every level
*/
void
d_tests_markdown_headings(void)
{
    using djinterp::markdown::heading_kind_from_level;
    using djinterp::markdown::heading_level_from_kind;
    using djinterp::markdown::is_heading_block_kind;

    bool round_trip = true;

    for (int level = 1; level <= 6; ++level)
    {
        volatile int        held = level;
        const md_block_kind kind = heading_kind_from_level(held);

        round_trip = round_trip                         &&
                     is_heading_block_kind(kind)        &&
                     (heading_level_from_kind(kind) == level);
    }

    D_TESTS_TEXT_CHECK(round_trip);
    D_TESTS_TEXT_CHECK(heading_level_from_kind(md_block_kind::heading_1) == 1);
    D_TESTS_TEXT_CHECK(heading_level_from_kind(
                           d_tests_markdown_runtime_block(
                               md_block_kind::paragraph)) == 0);
    D_TESTS_TEXT_CHECK(heading_kind_from_level(0) == md_block_kind::unknown);
    D_TESTS_TEXT_CHECK(heading_kind_from_level(7) == md_block_kind::unknown);
    D_TESTS_TEXT_CHECK(!is_heading_block_kind(md_block_kind::paragraph));

    return;
}


/*
d_tests_markdown_inlines
  Tests the following:
  - text, breaks, code spans, images and autolinks are atomic; emphasis,
    strong, strikethrough and links wrap other inlines
  - emphasis covers every styling kind, strong included
  - links are links, autolinks and email autolinks; an image is not
  - the three breaks are breaks; unknown has no category
*/
void
d_tests_markdown_inlines(void)
{
    using djinterp::markdown::markdown_category_for_inline_kind;
    using djinterp::markdown::is_atomic_inline_kind;
    using djinterp::markdown::is_container_inline_kind;
    using djinterp::markdown::is_emphasis_inline_kind;
    using djinterp::markdown::is_link_inline_kind;
    using djinterp::markdown::is_image_inline_kind;
    using djinterp::markdown::is_break_inline_kind;
    using djinterp::markdown::mic_none;

    const md_inline_kind text =
        d_tests_markdown_runtime_inline(md_inline_kind::text);
    const md_inline_kind link =
        d_tests_markdown_runtime_inline(md_inline_kind::link);

    D_TESTS_TEXT_CHECK(is_atomic_inline_kind(text) &&
                       !is_container_inline_kind(text));
    D_TESTS_TEXT_CHECK(is_container_inline_kind(link) &&
                       is_link_inline_kind(link));
    D_TESTS_TEXT_CHECK(is_atomic_inline_kind(md_inline_kind::code_span));
    D_TESTS_TEXT_CHECK(is_atomic_inline_kind(md_inline_kind::image) &&
                       is_image_inline_kind(md_inline_kind::image));
    D_TESTS_TEXT_CHECK(!is_link_inline_kind(md_inline_kind::image));
    D_TESTS_TEXT_CHECK(is_link_inline_kind(md_inline_kind::autolink) &&
                       is_link_inline_kind(md_inline_kind::autolink_email));

    D_TESTS_TEXT_CHECK(is_emphasis_inline_kind(md_inline_kind::emphasis));
    D_TESTS_TEXT_CHECK(is_emphasis_inline_kind(md_inline_kind::strong));
    D_TESTS_TEXT_CHECK(is_emphasis_inline_kind(
                           md_inline_kind::strikethrough));
    D_TESTS_TEXT_CHECK(!is_emphasis_inline_kind(md_inline_kind::code_span));

    D_TESTS_TEXT_CHECK(is_break_inline_kind(md_inline_kind::soft_break));
    D_TESTS_TEXT_CHECK(is_break_inline_kind(md_inline_kind::hard_break));
    D_TESTS_TEXT_CHECK(is_break_inline_kind(
                           md_inline_kind::hard_line_break));
    D_TESTS_TEXT_CHECK(markdown_category_for_inline_kind(
                           d_tests_markdown_runtime_inline(
                               md_inline_kind::unknown)) == mic_none);

    return;
}


// DTestsMarkdownBackend, DTestsMarkdownUntagged
//   probes: a type with a backend tag, and one without.
struct DTestsMarkdownBackend
{
    typedef djinterp::markdown::DMarkdownCmarkBackendTag markdown_backend_tag;
};

struct DTestsMarkdownUntagged
{
    typedef int block_type;
};


/*
d_tests_markdown_backends
  Tests the following:
  - every backend tag derives from DMarkdownBackendTag
  - is_markdown_backend finds a nested markdown_backend_tag, through
    references and cv-qualifiers, and nothing else
*/
void
d_tests_markdown_backends(void)
{
    using djinterp::markdown::DMarkdownBackendTag;
    using djinterp::markdown::is_markdown_backend;

    D_TESTS_TEXT_CHECK((std::is_base_of<
                           DMarkdownBackendTag,
                           djinterp::markdown::DMarkdownDefaultBackendTag
                       >::value));
    D_TESTS_TEXT_CHECK((std::is_base_of<
                           DMarkdownBackendTag,
                           djinterp::markdown::DMarkdownMd4cBackendTag
                       >::value));
    D_TESTS_TEXT_CHECK((std::is_base_of<
                           DMarkdownBackendTag,
                           djinterp::markdown::DMarkdownMmarkBackendTag
                       >::value));

    D_TESTS_TEXT_CHECK(is_markdown_backend<DTestsMarkdownBackend>::value);
    D_TESTS_TEXT_CHECK(
        is_markdown_backend<const DTestsMarkdownBackend&>::value);
    D_TESTS_TEXT_CHECK(
        is_markdown_backend<
            djinterp::markdown::DMarkdownDefaultBackend>::value);
    D_TESTS_TEXT_CHECK(!is_markdown_backend<DTestsMarkdownUntagged>::value);
    D_TESTS_TEXT_CHECK(!is_markdown_backend<int>::value);

    return;
}

#endif  // C++11
