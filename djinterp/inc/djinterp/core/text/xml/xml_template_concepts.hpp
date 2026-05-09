/******************************************************************************
* djinterp [text]                                    xml_template_concepts.hpp
*
*   C++20 concept wrappers for the XML structural protocols. Each
* concept is a thin shell over the corresponding SFINAE trait in
* `xml_template_traits.hpp`, so a type that satisfies the trait
* satisfies the concept and vice versa. The whole header is gated
* behind `D_ENV_CPP_FEATURE_LANG_CONCEPTS`; under older standards it
* expands to nothing so existing trait-based code keeps compiling
* unchanged.
*
*   USAGE:
*   ------
*   Concepts let consumer code constrain templates declaratively:
*
*     template<xml_node_type _Node>
*     void print_name(const _Node& _n);
*
*     template<xml_document_type _Doc>
*     bool save_to(const _Doc& _d, const std::string& _path);
*
*   They also enable overload selection without the SFINAE noise of
* `enable_if`. Where the trait variant is `is_xml_node<T>::value`,
* the concept variant is simply `xml_node_type<T>`.
*
*   GROUPING:
*   The concepts in this header partition into four families:
*     - leaf protocols       (attribute, node, element, document)
*     - capability protocols (mutable, parseable, writable, savable)
*     - composition protocols (full_xml_node, full_xml_document)
*     - backend protocols     (xml_backend_type, complete_xml_backend)
*
*
* path:      /inc/djinterp/core/text/xml/xml_template_concepts.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.08
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    LEAF PROTOCOL CONCEPTS
      ------------------------
      a. xml_attribute_type
      b. xml_node_type
      c. xml_element_type
      d. xml_document_type

II.   CAPABILITY CONCEPTS
      --------------------
      a. mutable_xml_node
      b. parseable_xml_document
      c. writable_xml_document
      d. savable_xml_document

III.  COMPOSITION CONCEPTS
      ---------------------
      a. full_xml_node
      b. full_xml_document

IV.   BACKEND CONCEPTS
      -----------------
      a. xml_backend_type
      b. complete_xml_backend
*/

#ifndef DJINTERP_XML_TEMPLATE_CONCEPTS_
#define DJINTERP_XML_TEMPLATE_CONCEPTS_ 1

#include "../../../djinterp.hpp"
#include "./xml_template_traits.hpp"


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   LEAF PROTOCOL CONCEPTS                              ///
///////////////////////////////////////////////////////////////////////////////

// xml_attribute_type
//   concept: constrains types satisfying the XML attribute
// protocol -- a name accessor and a value accessor in either
// short-form or get-form.
template<typename _Type>
concept xml_attribute_type =
    is_xml_attribute<_Type>::value;


// xml_node_type
//   concept: constrains types satisfying the minimum XML node
// protocol -- a name accessor and some form of child traversal.
template<typename _Type>
concept xml_node_type =
    is_xml_node<_Type>::value;


// xml_element_type
//   concept: constrains types satisfying the full XML element
// protocol -- node + attribute container.
template<typename _Type>
concept xml_element_type =
    is_xml_element<_Type>::value;


// xml_document_type
//   concept: constrains types satisfying the XML document
// protocol -- a root-element accessor in some form.
template<typename _Type>
concept xml_document_type =
    is_xml_document<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                II.   CAPABILITY CONCEPTS                                ///
///////////////////////////////////////////////////////////////////////////////

// mutable_xml_node
//   concept: constrains nodes that can be mutated in place via
// at least one of set_attribute, add_child, or set_text.
template<typename _Type>
concept mutable_xml_node =
    ( xml_node_type<_Type> &&
      ( has_set_attribute_method<_Type>::value ||
        has_add_child_method<_Type>::value     ||
        has_set_text_method<_Type>::value ) );


// parseable_xml_document
//   concept: constrains documents that can ingest serialised
// XML via a parse(string) method.
template<typename _Type>
concept parseable_xml_document =
    ( xml_document_type<_Type> &&
      has_parse_method<_Type>::value );


// writable_xml_document
//   concept: constrains documents that can serialise to a
// string via write().
template<typename _Type>
concept writable_xml_document =
    ( xml_document_type<_Type> &&
      has_write_method<_Type>::value );


// savable_xml_document
//   concept: constrains documents that can persist to a file
// path via save(path).
template<typename _Type>
concept savable_xml_document =
    ( xml_document_type<_Type> &&
      has_save_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                III.   COMPOSITION CONCEPTS                              ///
///////////////////////////////////////////////////////////////////////////////

// full_xml_node
//   concept: constrains nodes that expose the complete
// structural protocol -- name, kind, attributes, children,
// and text -- regardless of mutability.
template<typename _Type>
concept full_xml_node =
    ( xml_node_type<_Type>          &&
      has_kind_access<_Type>::value &&
      has_text_access<_Type>::value &&
      ( has_attributes_method<_Type>::value ||
        has_get_attributes_method<_Type>::value ) );


// full_xml_document
//   concept: constrains documents that expose the complete
// document protocol -- root + every prolog accessor + read +
// write.
template<typename _Type>
concept full_xml_document =
    ( xml_document_type<_Type>           &&
      has_version_method<_Type>::value   &&
      has_encoding_method<_Type>::value  &&
      has_write_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                IV.   BACKEND CONCEPTS                                   ///
///////////////////////////////////////////////////////////////////////////////

// xml_backend_type
//   concept: constrains types tagged as XML backends via the
// `backend_tag` nested alias.
template<typename _Type>
concept xml_backend_type =
    is_xml_backend<_Type>::value;


// complete_xml_backend
//   concept: constrains backends that expose the full set of
// nested type aliases (node_type, attribute_type, document_type)
// plus a make_document factory. This is the bar a backend must
// clear to be plugged into `xml_node<_Backend>` and
// `xml_document<_Backend>` without further specialisation.
template<typename _Type>
concept complete_xml_backend =
    ( xml_backend_type<_Type>                 &&
      is_xml_backend_complete<_Type>::value   &&
      has_make_document_method<_Type>::value );


NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_XML_TEMPLATE_CONCEPTS_
