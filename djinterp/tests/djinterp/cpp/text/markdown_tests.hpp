/*******************************************************************************
* djinterp [cpp][text]                                        markdown_tests.hpp
*
*   The suite for text's Markdown headers -- markdown/markdown.hpp,
* markdown/markdown_template.hpp and its traits and concepts -- in DTest's C
* face (c/modules/test/test_suite.h).
*   One entry point builds the module "markdown" imperatively: a block per
* header and a test per group of checks. Every header's floor is C++11, and
* below it a group is skipped, saying why; the concepts are C++20's.
*   The groups live in the section files: markdown_tests_vocabulary.cpp
* (markdown.hpp), markdown_tests_tree.cpp (building a tree),
* markdown_tests_detached.cpp (the factories' handles and adoption),
* markdown_tests_render.cpp (its four renderings), and
* markdown_tests_traits.cpp (the traits and the concepts).
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
* 2.  GROUPS
*     a.  markdown.hpp
*     b.  markdown_template.hpp
*     c.  markdown_template_traits.hpp and _concepts.hpp
* 3.  BUILDERS
*
*
* path:      /tests/djinterp/cpp/text/markdown_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_TEXT_MARKDOWN_TESTS_HPP
#define DJINTERP_TESTS_CPP_TEXT_MARKDOWN_TESTS_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
// djinterp
#include "text_tests_common.hpp"  // D_TESTS_TEXT_*, DTest's C face
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/markdown/markdown.hpp"
                                                      // the tree


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "markdown" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_markdown_all(dj_test_suite* _suite);


//==============================================================================
// 2.  GROUPS
//==============================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// a.  markdown.hpp (markdown_tests_vocabulary.cpp)
void d_tests_markdown_blocks(void);
void d_tests_markdown_headings(void);
void d_tests_markdown_inlines(void);
void d_tests_markdown_backends(void);

// b.  markdown_template.hpp (markdown_tests_tree.cpp, _render.cpp)
void d_tests_markdown_tree_build(void);
void d_tests_markdown_detached_handles(void);
void d_tests_markdown_detached_adopt(void);
void d_tests_markdown_detached_setters(void);
void d_tests_markdown_render_markdown(void);
void d_tests_markdown_render_lists(void);
void d_tests_markdown_render_html(void);
void d_tests_markdown_render_plaintext(void);
void d_tests_markdown_render_xml(void);

// c.  the traits and concepts (markdown_tests_traits.cpp)
void d_tests_markdown_traits(void);

#endif  // C++11

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
void d_tests_markdown_concepts(void);
#endif  // C++20


//==============================================================================
// 3.  BUILDERS
//==============================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// md_backend, md_document, md_block, md_inline, md_new_block, md_new_inline
//   the default backend's tree, shortly: its facades, and the handles its
// factories return.
typedef djinterp::markdown::DMarkdownDefaultBackend       md_backend;
typedef djinterp::markdown::DMarkdownDocument<md_backend> md_document;
typedef djinterp::markdown::DMarkdownBlock<md_backend>    md_block;
typedef djinterp::markdown::DMarkdownInline<md_backend>   md_inline;
typedef djinterp::markdown::DMarkdownDetachedBlock<md_backend>
    md_new_block;
typedef djinterp::markdown::DMarkdownDetachedInline<md_backend>
    md_new_inline;

/*
d_tests_markdown_text
  A text inline.
*/
inline md_new_inline
d_tests_markdown_text(
    const std::string& _text
)
{
    return djinterp::markdown::make_text<md_backend>(_text);
}

/*
d_tests_markdown_holding
  A block given one text inline: a paragraph's, a heading's or a cell's.
*/
inline md_new_block
d_tests_markdown_holding(
    md_new_block       _block,
    const std::string& _text
)
{
    _block->add_inline(d_tests_markdown_text(_text));

    return _block;
}

/*
d_tests_markdown_paragraph
  A paragraph of one text.
*/
inline md_new_block
d_tests_markdown_paragraph(
    const std::string& _text
)
{
    return d_tests_markdown_holding(
               djinterp::markdown::make_paragraph<md_backend>(), _text);
}

/*
d_tests_markdown_item
  A list item holding a paragraph of one text.
*/
inline md_new_block
d_tests_markdown_item(
    const std::string& _text
)
{
    md_new_block item = djinterp::markdown::make_list_item<md_backend>();

    item->add_block(d_tests_markdown_paragraph(_text));

    return item;
}

/*
d_tests_markdown_list
  A list of items, each a paragraph of one text.
*/
inline md_new_block
d_tests_markdown_list(
    bool                     _ordered,
    int                      _start,
    const char* const*       _texts,
    std::size_t              _count
)
{
    md_new_block list = djinterp::markdown::make_list<md_backend>(_ordered,
                                                                  _start);

    for (std::size_t i = 0; i < _count; ++i)
    {
        list->add_block(d_tests_markdown_item(_texts[i]));
    }

    return list;
}

#endif  // C++11


#endif  // DJINTERP_TESTS_CPP_TEXT_MARKDOWN_TESTS_HPP
