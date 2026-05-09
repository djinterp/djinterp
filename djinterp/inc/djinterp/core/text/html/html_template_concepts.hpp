/******************************************************************************
* djinterp [text]                                   html_template_concepts.hpp
*
*   C++20 concepts for the HTML element / document / backend
* protocols. These mirror the structural traits in
* `html_template_traits.hpp` but expose them as concept declarations
* that can be used directly in template constraints, requires-clauses,
* and abbreviated function-template syntax. Layered on top of the XML
* concepts so any HTML type that satisfies an XML concept inherits
* that classification automatically.
*
*   The whole header is gated behind
* `D_ENV_CPP_FEATURE_LANG_CONCEPTS` -- it produces nothing on
* pre-C++20 toolchains so the rest of the HTML module remains
* language-version-agnostic.
*
*   USAGE EXAMPLES:
*
*     // Constrain a function template to HTML elements only.
*     template<html::html_element_type _Element>
*     void render(const _Element& e);
*
*     // Constrain a builder to HTML documents that expose head/body
*     // shortcuts.
*     template<html::dom_shortcut_document _Doc>
*     void inject_meta(_Doc& d, const std::string& name,
*                                const std::string& value);
*
*     // Constrain a backend type to a complete HTML backend.
*     template<html::complete_html_backend _Backend>
*     auto build();
*
*
* path:      /inc/djinterp/core/text/html/html_template_concepts.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.08
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HTML ELEMENT CONCEPTS
II.   HTML DOCUMENT CONCEPTS
III.  CAPABILITY CONCEPTS (CLASS / ID / DOCTYPE / DOM SHORTCUTS)
IV.   COMPOSITE CONCEPTS
V.    HTML BACKEND CONCEPTS
*/

#ifndef DJINTERP_HTML_TEMPLATE_CONCEPTS_
#define DJINTERP_HTML_TEMPLATE_CONCEPTS_ 1

// djinterp
#include "../../../djinterp.hpp"
#include "./html.hpp"
#include "./html_template_traits.hpp"
#include "../xml/xml_template_concepts.hpp"


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// std
#include <concepts>
#include <string>


NS_DJINTERP

namespace html {


///////////////////////////////////////////////////////////////////////////////
///                I.   HTML ELEMENT CONCEPTS                               ///
///////////////////////////////////////////////////////////////////////////////

// html_element_type
//   concept: satisfied by any type that satisfies the XML element
// protocol AND exposes an html_kind discriminator (either
// `html_kind()` or `get_html_kind()`).
template<typename _Type>
concept html_element_type =
       ( ::djinterp::xml_element_type<_Type> )
    && ( has_html_kind_access<_Type>::value );


// html_element_loose_type
//   concept: looser variant; any type satisfying the XML element
// protocol is treated as an HTML element candidate. Use when
// adapting third-party DOM types into the HTML facades.
template<typename _Type>
concept html_element_loose_type =
       ( ::djinterp::xml_element_type<_Type> );


///////////////////////////////////////////////////////////////////////////////
///                II.   HTML DOCUMENT CONCEPTS                             ///
///////////////////////////////////////////////////////////////////////////////

// html_document_type
//   concept: satisfied by any type that satisfies the XML document
// protocol AND exposes either a doctype accessor or an HTML
// version accessor.
template<typename _Type>
concept html_document_type =
       ( ::djinterp::xml_document_type<_Type> )
    && (    ( has_doctype_access<_Type>::value )
         || ( has_html_version_method<_Type>::value ) );


// html_document_loose_type
//   concept: looser variant; any type satisfying the XML document
// protocol is treated as an HTML document candidate.
template<typename _Type>
concept html_document_loose_type =
       ( ::djinterp::xml_document_type<_Type> );


///////////////////////////////////////////////////////////////////////////////
///   III.   CAPABILITY CONCEPTS (CLASS / ID / DOCTYPE / DOM SHORTCUTS)     ///
///////////////////////////////////////////////////////////////////////////////

// classed_html_element
//   concept: HTML element that exposes the full class-list
// interface (has_class + add_class + remove_class).
template<typename _Type>
concept classed_html_element =
       ( html_element_type<_Type> )
    && ( has_class_support<_Type>::value );


// identifiable_html_element
//   concept: HTML element that exposes both id read and id write
// accessors.
template<typename _Type>
concept identifiable_html_element =
       ( html_element_type<_Type> )
    && ( has_id_method<_Type>::value )
    && ( has_set_id_method<_Type>::value );


// doctype_aware_document
//   concept: HTML document with a doctype accessor.
template<typename _Type>
concept doctype_aware_document =
       ( html_document_type<_Type> )
    && ( has_doctype_access<_Type>::value );


// versioned_html_document
//   concept: HTML document with an html_version accessor.
template<typename _Type>
concept versioned_html_document =
       ( html_document_type<_Type> )
    && ( has_html_version_method<_Type>::value );


// dom_shortcut_document
//   concept: HTML document with both head_element and body_element
// accessors.
template<typename _Type>
concept dom_shortcut_document =
       ( html_document_type<_Type> )
    && ( has_html_dom_shortcuts<_Type>::value );


// titled_html_document
//   concept: HTML document with title read+write accessors.
template<typename _Type>
concept titled_html_document =
       ( html_document_type<_Type> )
    && ( has_title_method<_Type>::value )
    && ( has_set_title_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                IV.   COMPOSITE CONCEPTS                                 ///
///////////////////////////////////////////////////////////////////////////////

// full_html_element
//   concept: HTML element exposing every common HTML-side
// accessor (kind + class-list + id read/write).
template<typename _Type>
concept full_html_element =
       ( html_element_type<_Type> )
    && ( classed_html_element<_Type> )
    && ( identifiable_html_element<_Type> );


// full_html_document
//   concept: HTML document exposing every common HTML-side
// accessor (doctype + version + head/body shortcuts + title).
template<typename _Type>
concept full_html_document =
       ( html_document_type<_Type> )
    && ( doctype_aware_document<_Type> )
    && ( versioned_html_document<_Type> )
    && ( dom_shortcut_document<_Type> )
    && ( titled_html_document<_Type> );


///////////////////////////////////////////////////////////////////////////////
///                V.   HTML BACKEND CONCEPTS                               ///
///////////////////////////////////////////////////////////////////////////////

// html_backend_type
//   concept: satisfied by any type tagged with `html_backend_tag`
// (i.e. any type for which `is_html_backend<T>::value` is true).
template<typename _Type>
concept html_backend_type =
       ( is_html_backend<_Type>::value );


// complete_html_backend
//   concept: an HTML backend that additionally exposes the full
// nested-type-alias protocol AND a make_html_document factory.
// Code that needs to instantiate `html_document<_Backend>` from
// scratch should constrain on this concept.
template<typename _Type>
concept complete_html_backend =
       ( html_backend_type<_Type> )
    && ( is_html_backend_complete<_Type>::value )
    && ( has_make_html_document_method<_Type>::value );


}   // namespace html
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // DJINTERP_HTML_TEMPLATE_CONCEPTS_
