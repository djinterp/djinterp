/*******************************************************************************
* djinterp [core]                                 markdown_template_concepts.hpp
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
*     template<markdown::markdown_block_type Block>
*     void process(const Block& b);
*
*     // Constrain a renderer to documents that emit HTML.
*     template<markdown::html_renderable_document Doc>
*     std::string to_html(const Doc& d);
*
*     // Constrain a builder to a complete markdown backend.
*     template<markdown::complete_markdown_backend Backend>
*     auto build();
*
*
* path:      /inc/djinterp/core/text/markdown/markdown_template_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.10
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    BLOCK CONCEPTS
      --------------

II.   INLINE CONCEPTS
      ---------------

III.  CAPABILITY CONCEPTS
      -------------------

IV.   DOCUMENT CONCEPTS
      -----------------

V.    RENDER-TARGET CONCEPTS
      ----------------------

VI.   COMPOSITE CONCEPTS
      ------------------

VII.  BACKEND CONCEPTS
      ----------------
*/

#ifndef DJINTERP_TEXT_MARKDOWN_MARKDOWN_TEMPLATE_CONCEPTS_HPP
#define DJINTERP_TEXT_MARKDOWN_MARKDOWN_TEMPLATE_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

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
template<typename Type>
concept markdown_block_type =
    is_markdown_block<Type>::value;


// markdown_block_loose_type
//   concept: looser variant -- the block-kind accessor alone
// is sufficient.
template<typename Type>
concept markdown_block_loose_type =
    is_markdown_block_loose<Type>::value;


// container_markdown_block_type
//   concept: a markdown block that itself contains other
// blocks (document, blockquote, list, list_item, ...).
template<typename Type>
concept container_markdown_block_type =
       ( markdown_block_type<Type> )
    && (    has_blocks_method<Type>::value
         || has_children_access<Type>::value );


// leaf_markdown_block_type
//   concept: a markdown block that contains only inlines or
// raw text (paragraph, heading, code block, table cell, ...).
template<typename Type>
concept leaf_markdown_block_type =
       ( markdown_block_type<Type> )
    && (    has_inlines_method<Type>::value
         || has_text_access<Type>::value );


// heading_block_type
//   concept: a markdown block exposing a heading-level
// accessor.
template<typename Type>
concept heading_block_type =
       ( markdown_block_type<Type> )
    && ( has_heading_level_access<Type>::value );


// code_block_type
//   concept: a markdown block exposing a language accessor
// (i.e. classifiable as a code block).
template<typename Type>
concept code_block_type =
       ( markdown_block_type<Type> )
    && ( has_language_access<Type>::value );


// list_block_type
//   concept: a markdown block exposing list-ordering
// information.
template<typename Type>
concept list_block_type =
       ( markdown_block_type<Type> )
    && ( has_list_ordered_method<Type>::value );


// task_list_item_type
//   concept: a markdown block exposing task-checked state.
template<typename Type>
concept task_list_item_type =
       ( markdown_block_type<Type> )
    && ( has_task_checked_method<Type>::value );


// table_block_type
//   concept: a markdown block exposing column alignments
// (i.e. the table block itself, not table rows/cells).
template<typename Type>
concept table_block_type =
       ( markdown_block_type<Type> )
    && ( has_table_alignment_method<Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                II.   INLINE CONCEPTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// markdown_inline_type
//   concept: satisfied by any type that satisfies the markdown
// inline protocol (kind accessor + text/url/children access).
template<typename Type>
concept markdown_inline_type =
    is_markdown_inline<Type>::value;


// markdown_inline_loose_type
//   concept: looser variant -- the inline-kind accessor alone.
template<typename Type>
concept markdown_inline_loose_type =
    is_markdown_inline_loose<Type>::value;


// linked_inline_type
//   concept: a markdown inline exposing url and title
// accessors (i.e. classifiable as a link or image).
template<typename Type>
concept linked_inline_type =
       ( markdown_inline_type<Type> )
    && ( has_url_access<Type>::value );


// image_inline_type
//   concept: a markdown inline exposing alt-text in addition
// to url.
template<typename Type>
concept image_inline_type =
       ( linked_inline_type<Type> )
    && ( has_alt_text_access<Type>::value );


// styled_inline_type
//   concept: a markdown inline that wraps other inlines
// (emphasis, strong, strikethrough, etc.).
template<typename Type>
concept styled_inline_type =
       ( markdown_inline_type<Type> )
    && ( has_children_access<Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                III.   CAPABILITY CONCEPTS                               ///
///////////////////////////////////////////////////////////////////////////////

// mutable_block_type
//   concept: a block exposing text mutation.
template<typename Type>
concept mutable_block_type =
       ( markdown_block_type<Type> )
    && ( has_set_text_method<Type>::value );


// mutable_inline_type
//   concept: an inline exposing text mutation.
template<typename Type>
concept mutable_inline_type =
       ( markdown_inline_type<Type> )
    && ( has_set_text_method<Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                IV.   DOCUMENT CONCEPTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// markdown_document_type
//   concept: a document type exposing flavor + at least one
// render method.
template<typename Type>
concept markdown_document_type =
    is_markdown_document<Type>::value;


// markdown_document_loose_type
//   concept: looser variant -- flavor accessor OR any render
// method.
template<typename Type>
concept markdown_document_loose_type =
    is_markdown_document_loose<Type>::value;


// flavoured_markdown_document
//   concept: a document exposing the flavor accessor.
template<typename Type>
concept flavoured_markdown_document =
    has_flavor_access<Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                V.   RENDER-TARGET CONCEPTS                              ///
///////////////////////////////////////////////////////////////////////////////

// markdown_renderable_document
//   concept: a document exposing render_to_markdown.
template<typename Type>
concept markdown_renderable_document =
    has_render_to_markdown_method<Type>::value;


// html_renderable_document
//   concept: a document exposing render_to_html.
template<typename Type>
concept html_renderable_document =
    has_render_to_html_method<Type>::value;


// xml_renderable_document
//   concept: a document exposing render_to_xml.
template<typename Type>
concept xml_renderable_document =
    has_render_to_xml_method<Type>::value;


// plaintext_renderable_document
//   concept: a document exposing render_to_plaintext.
template<typename Type>
concept plaintext_renderable_document =
    has_render_to_plaintext_method<Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VI.   COMPOSITE CONCEPTS                                 ///
///////////////////////////////////////////////////////////////////////////////

// full_markdown_document
//   concept: a document exposing every render target plus the
// flavor accessor.
template<typename Type>
concept full_markdown_document =
       ( markdown_document_type<Type> )
    && ( markdown_renderable_document<Type> )
    && ( html_renderable_document<Type> )
    && ( xml_renderable_document<Type> )
    && ( plaintext_renderable_document<Type> );


///////////////////////////////////////////////////////////////////////////////
///                VII.   BACKEND CONCEPTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// markdown_backend_type
//   concept: satisfied by any type tagged with
// `markdown_backend_tag`.
template<typename Type>
concept markdown_backend_type =
    is_markdown_backend<Type>::value;


// complete_markdown_backend
//   concept: a markdown backend that additionally exposes the
// full nested-type-alias protocol AND a make_markdown_document
// factory.
template<typename Type>
concept complete_markdown_backend =
       ( markdown_backend_type<Type> )
    && ( is_markdown_backend_complete<Type>::value )
    && ( has_make_markdown_document_method<Type>::value );


}   // namespace markdown
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_TEXT_MARKDOWN_MARKDOWN_TEMPLATE_CONCEPTS_HPP
