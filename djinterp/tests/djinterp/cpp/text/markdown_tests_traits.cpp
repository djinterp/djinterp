/*******************************************************************************
* djinterp [cpp][text]                                 markdown_tests_traits.cpp
*
*   The groups for markdown/markdown_template_traits.hpp and
* markdown/markdown_template_concepts.hpp: the default backend's facades,
* document and backend meet the protocols, at no cost in size; probes that
* expose part of a protocol meet its loose form or its getters' form; and the
* concepts, from C++20, agree with the traits.
*
*
* path:      /tests/djinterp/cpp/text/markdown_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "markdown_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups, builders
// std
#include <ostream>  // std::ostream
#include <string>   // std::string


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

//==============================================================================
// PROBES
//==============================================================================

// DTestsMarkdownKindOnly
//   probe: a block kind, and nothing else.
struct DTestsMarkdownKindOnly
{
    djinterp::markdown::DMarkdownBlockKind block_kind() const
    {
        return djinterp::markdown::DMarkdownBlockKind::paragraph;
    }
};

// DTestsMarkdownGetterInline
//   probe: an inline through getters alone.
struct DTestsMarkdownGetterInline
{
    djinterp::markdown::DMarkdownInlineKind get_inline_kind() const
    {
        return djinterp::markdown::DMarkdownInlineKind::link;
    }
    std::string get_url() const { return std::string(); }
};

// DTestsMarkdownRenderOnly
//   probe: a renderer onto a stream, without a flavour.
struct DTestsMarkdownRenderOnly
{
    void render_to_html(std::ostream&) const {}
};


/*
d_tests_markdown_traits
  Tests the following:
  - the default backend's block, inline and document meet the protocols,
    and its document every render target
  - the backend is complete: its three node types and its factory
  - a facade is one pointer: the facades cost nothing
  - a kind alone meets the loose block protocol, not the strict one;
    getters meet the inline protocol; rendering onto a stream without a
    flavour meets only the loose document protocol; int meets none
*/
void
d_tests_markdown_traits(void)
{
    using namespace djinterp::markdown;

    typedef DMarkdownDocumentClass<md_document> document_class;

    D_TESTS_TEXT_CHECK(is_markdown_block<md_block>::value);
    D_TESTS_TEXT_CHECK(is_markdown_inline<md_inline>::value);
    D_TESTS_TEXT_CHECK(is_markdown_document<md_document>::value);
    D_TESTS_TEXT_CHECK(document_class::is_doc && document_class::has_flavor);
    D_TESTS_TEXT_CHECK(document_class::renders_markdown &&
                       document_class::renders_html     &&
                       document_class::renders_xml      &&
                       document_class::renders_plaintext);
    D_TESTS_TEXT_CHECK(is_markdown_backend_complete<md_backend>::value);
    D_TESTS_TEXT_CHECK(has_make_markdown_document_method<md_backend>::value);
    D_TESTS_TEXT_CHECK((sizeof(md_block) == sizeof(void*)) &&
                       (sizeof(md_inline) == sizeof(void*)));

    D_TESTS_TEXT_CHECK(is_markdown_block_loose<DTestsMarkdownKindOnly>::value);
    D_TESTS_TEXT_CHECK(!is_markdown_block<DTestsMarkdownKindOnly>::value);
    D_TESTS_TEXT_CHECK(is_markdown_inline<DTestsMarkdownGetterInline>::value);
    D_TESTS_TEXT_CHECK(
        is_markdown_document_loose<DTestsMarkdownRenderOnly>::value);
    D_TESTS_TEXT_CHECK(!is_markdown_document<DTestsMarkdownRenderOnly>::value);
    D_TESTS_TEXT_CHECK(!is_markdown_block_loose<int>::value &&
                       !is_markdown_inline_loose<int>::value &&
                       !is_markdown_document_loose<int>::value &&
                       !is_markdown_backend_complete<int>::value);

    return;
}

#endif  // C++11


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

/*
d_tests_markdown_concepts
  Tests the following:
  - the default backend's block, inline and document satisfy the concepts,
    the document the full one, and the backend the complete one
  - the probes satisfy what their traits say, and int nothing
*/
void
d_tests_markdown_concepts(void)
{
    using namespace djinterp::markdown;

    D_TESTS_TEXT_CHECK(markdown_block_type<md_block>);
    D_TESTS_TEXT_CHECK(markdown_inline_type<md_inline>);
    D_TESTS_TEXT_CHECK(full_markdown_document<md_document>);
    D_TESTS_TEXT_CHECK(complete_markdown_backend<md_backend>);
    D_TESTS_TEXT_CHECK(markdown_block_loose_type<DTestsMarkdownKindOnly> &&
                       !markdown_block_type<DTestsMarkdownKindOnly>);
    D_TESTS_TEXT_CHECK(markdown_inline_type<DTestsMarkdownGetterInline>);
    D_TESTS_TEXT_CHECK(!markdown_document_type<DTestsMarkdownRenderOnly>);
    D_TESTS_TEXT_CHECK(!markdown_backend_type<int>);

    return;
}

#endif  // C++20
