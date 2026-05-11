/******************************************************************************
* djinterp [markdown]                            markdown_template_concepts.hpp
*
*   C++20 concepts for the Markdown block / inline / document /
* backend protocols. Mirrors the structural traits in
* `markdown_template_traits.hpp` but exposes them as concept
* declarations usable in template constraints, requires-clauses, and
* abbreviated function-template syntax.
*
*   The whole header is gated behind
* `D_ENV_CPP_FEATURE_LANG_CONCEPTS` -- it produces nothing on
* pre-C++20 toolchains so the rest of the markdown module remains
* language-version-agnostic.
*
*   USAGE EXAMPLES:
*
*     // Constrain a function template to markdown blocks only.
*     template<markdown::markdown_block_type _Block>
*     void process(const _Block& b);
*
*     // Constrain a renderer to documents that emit HTML.
*     template<markdown::html_renderable_document _Doc>
*     std::string to_html(const _Doc& d);
*
*     // Constrain a builder to a complete markdown backend.
*     template<markdown::complete_markdown_backend _Backend>
*     auto build();
*
*
* path:      /inc/djinterp/core/util/markdown/markdown_template_concepts.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    BLOCK CONCEPTS
II.   INLINE CONCEPTS
III.  CAPABILITY CONCEPTS
IV.   DOCUMENT CONCEPTS
V.    RENDER-TARGET CONCEPTS
VI.   COMPOSITE CONCEPTS
VII.  BACKEND CONCEPTS
*/

#ifndef DJINTERP_MARKDOWN_TEMPLATE_CONCEPTS_
#define DJINTERP_MARKDOWN_TEMPLATE_CONCEPTS_ 1

// djinterp
#include "../../../djinterp.hpp"
#include "./markdown.hpp"
#include "./markdown_template_traits.hpp"


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// std
#include <concepts>


NS_DJINTERP

namespace markdown {


///////////////////////////////////////////////////////////////////////////////
///                I.   BLOCK CONCEPTS                                      ///
///////////////////////////////////////////////////////////////////////////////

// markdown_block_type
//   concept: satisfied by any type that satisfies the markdown
// block protocol (kind accessor + children/inlines/blocks/text
// access).
template<typename _Type>
concept markdown_block_type =
    is_markdown_block<_Type>::value;


// markdown_block_loose_type
//   concept: looser variant -- the block-kind accessor alone
// is sufficient.
template<typename _Type>
concept markdown_block_loose_type =
    is_markdown_block_loose<_Type>::value;


// container_markdown_block_type
//   concept: a markdown block that itself contains other
// blocks (document, blockquote, list, list_item, ...).
template<typename _Type>
concept container_markdown_block_type =
       ( markdown_block_type<_Type> )
    && (    has_blocks_method<_Type>::value
         || has_children_access<_Type>::value );


// leaf_markdown_block_type
//   concept: a markdown block that contains only inlines or
// raw text (paragraph, heading, code block, table cell, ...).
template<typename _Type>
concept leaf_markdown_block_type =
       ( markdown_block_type<_Type> )
    && (    has_inlines_method<_Type>::value
         || has_text_access<_Type>::value );


// heading_block_type
//   concept: a markdown block exposing a heading-level
// accessor.
template<typename _Type>
concept heading_block_type =
       ( markdown_block_type<_Type> )
    && ( has_heading_level_access<_Type>::value );


// code_block_type
//   concept: a markdown block exposing a language accessor
// (i.e. classifiable as a code block).
template<typename _Type>
concept code_block_type =
       ( markdown_block_type<_Type> )
    && ( has_language_access<_Type>::value );


// list_block_type
//   concept: a markdown block exposing list-ordering
// information.
template<typename _Type>
concept list_block_type =
       ( markdown_block_type<_Type> )
    && ( has_list_ordered_method<_Type>::value );


// task_list_item_type
//   concept: a markdown block exposing task-checked state.
template<typename _Type>
concept task_list_item_type =
       ( markdown_block_type<_Type> )
    && ( has_task_checked_method<_Type>::value );


// table_block_type
//   concept: a markdown block exposing column alignments
// (i.e. the table block itself, not table rows/cells).
template<typename _Type>
concept table_block_type =
       ( markdown_block_type<_Type> )
    && ( has_table_alignment_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                II.   INLINE CONCEPTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// markdown_inline_type
//   concept: satisfied by any type that satisfies the markdown
// inline protocol (kind accessor + text/url/children access).
template<typename _Type>
concept markdown_inline_type =
    is_markdown_inline<_Type>::value;


// markdown_inline_loose_type
//   concept: looser variant -- the inline-kind accessor alone.
template<typename _Type>
concept markdown_inline_loose_type =
    is_markdown_inline_loose<_Type>::value;


// linked_inline_type
//   concept: a markdown inline exposing url and title
// accessors (i.e. classifiable as a link or image).
template<typename _Type>
concept linked_inline_type =
       ( markdown_inline_type<_Type> )
    && ( has_url_access<_Type>::value );


// image_inline_type
//   concept: a markdown inline exposing alt-text in addition
// to url.
template<typename _Type>
concept image_inline_type =
       ( linked_inline_type<_Type> )
    && ( has_alt_text_access<_Type>::value );


// styled_inline_type
//   concept: a markdown inline that wraps other inlines
// (emphasis, strong, strikethrough, etc.).
template<typename _Type>
concept styled_inline_type =
       ( markdown_inline_type<_Type> )
    && ( has_children_access<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                III.   CAPABILITY CONCEPTS                               ///
///////////////////////////////////////////////////////////////////////////////

// mutable_block_type
//   concept: a block exposing text mutation.
template<typename _Type>
concept mutable_block_type =
       ( markdown_block_type<_Type> )
    && ( has_set_text_method<_Type>::value );


// mutable_inline_type
//   concept: an inline exposing text mutation.
template<typename _Type>
concept mutable_inline_type =
       ( markdown_inline_type<_Type> )
    && ( has_set_text_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                IV.   DOCUMENT CONCEPTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// markdown_document_type
//   concept: a document type exposing flavor + at least one
// render method.
template<typename _Type>
concept markdown_document_type =
    is_markdown_document<_Type>::value;


// markdown_document_loose_type
//   concept: looser variant -- flavor accessor OR any render
// method.
template<typename _Type>
concept markdown_document_loose_type =
    is_markdown_document_loose<_Type>::value;


// flavoured_markdown_document
//   concept: a document exposing the flavor accessor.
template<typename _Type>
concept flavoured_markdown_document =
    has_flavor_access<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                V.   RENDER-TARGET CONCEPTS                              ///
///////////////////////////////////////////////////////////////////////////////

// markdown_renderable_document
//   concept: a document exposing render_to_markdown.
template<typename _Type>
concept markdown_renderable_document =
    has_render_to_markdown_method<_Type>::value;


// html_renderable_document
//   concept: a document exposing render_to_html.
template<typename _Type>
concept html_renderable_document =
    has_render_to_html_method<_Type>::value;


// xml_renderable_document
//   concept: a document exposing render_to_xml.
template<typename _Type>
concept xml_renderable_document =
    has_render_to_xml_method<_Type>::value;


// plaintext_renderable_document
//   concept: a document exposing render_to_plaintext.
template<typename _Type>
concept plaintext_renderable_document =
    has_render_to_plaintext_method<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VI.   COMPOSITE CONCEPTS                                 ///
///////////////////////////////////////////////////////////////////////////////

// full_markdown_document
//   concept: a document exposing every render target plus the
// flavor accessor.
template<typename _Type>
concept full_markdown_document =
       ( markdown_document_type<_Type> )
    && ( markdown_renderable_document<_Type> )
    && ( html_renderable_document<_Type> )
    && ( xml_renderable_document<_Type> )
    && ( plaintext_renderable_document<_Type> );


///////////////////////////////////////////////////////////////////////////////
///                VII.   BACKEND CONCEPTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// markdown_backend_type
//   concept: satisfied by any type tagged with
// `markdown_backend_tag`.
template<typename _Type>
concept markdown_backend_type =
    is_markdown_backend<_Type>::value;


// complete_markdown_backend
//   concept: a markdown backend that additionally exposes the
// full nested-type-alias protocol AND a make_markdown_document
// factory.
template<typename _Type>
concept complete_markdown_backend =
       ( markdown_backend_type<_Type> )
    && ( is_markdown_backend_complete<_Type>::value )
    && ( has_make_markdown_document_method<_Type>::value );


}   // namespace markdown
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // DJINTERP_MARKDOWN_TEMPLATE_CONCEPTS_
