/******************************************************************************
* djinterp [text]                                     html_template_traits.hpp
*
*   Structural SFINAE detection traits for HTML element / document /
* backend types. Layered on top of `xml_template_traits.hpp`: any HTML
* type that satisfies the XML protocol automatically inherits XML
* trait classification, so this header focuses purely on the
* HTML-specific surface area (DOCTYPE access, doc-version queries,
* class-list helpers, head/body shortcuts).
*
*   This header is what makes the HTML module library-agnostic in the
* same way the XML traits do for XML. Any third-party HTML library --
* gumbo, lexbor, htmlcxx, libxml++ in HTML mode -- whose types satisfy
* the structural protocol classifies automatically. Both naming
* conventions are detected: short-form (`doctype()`, `head()`) AND
* getter-form (`get_doctype()`, `get_head_element()`).
*
*   DETECTED PROTOCOLS:
*
*   html_element protocol:
*     An XML node whose name resolves to a known HTML element kind.
*     Detected via XML traits + has_html_kind_method (or via a runtime
*     name lookup wrapped by `is_html_element`).
*
*   html_document protocol:
*     An XML document plus optional DOCTYPE / version / head / body
*     accessors. The minimum bar is the XML document protocol; the
*     advanced bar adds HTML-specific accessors.
*
*   html_backend protocol:
*     A type tagged with `html_backend_tag` (detected by `is_html_backend`
*     in `html.hpp`) plus the same nested-type-alias completeness check
*     used by the XML backend protocol.
*
*
* path:      /inc/djinterp/core/text/html/html_template_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.08
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HTML KIND ACCESSOR DETECTION
II.   CLASS LIST DETECTION
III.  DOCTYPE & VERSION DETECTION
IV.   HEAD / BODY ACCESSOR DETECTION
V.    HTML ELEMENT CLASSIFICATION
VI.   HTML DOCUMENT CLASSIFICATION
VII.  BACKEND COMPLETENESS DETECTION
VIII. VARIABLE TEMPLATES
*/

#ifndef DJINTERP_HTML_TEMPLATE_TRAITS_
#define DJINTERP_HTML_TEMPLATE_TRAITS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "../xml/xml_template_traits.hpp"


NS_DJINTERP

namespace html {


///////////////////////////////////////////////////////////////////////////////
///                I.   HTML KIND ACCESSOR DETECTION                        ///
///////////////////////////////////////////////////////////////////////////////

// has_html_kind_method
//   trait: true if _Type exposes an `html_kind()` const method
// returning an `html_element_kind` discriminator. This is the
// HTML-specific analogue of XML's `has_kind_method` and is what
// lets generic code branch on element kind without reading the
// tag-name string.
template<typename _Type,
         typename = void>
struct has_html_kind_method : std::false_type
{};

template<typename _Type>
struct has_html_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().html_kind())
>> : std::true_type
{};


// has_get_html_kind_method
//   trait: true if _Type exposes `get_html_kind()` (getter-form).
template<typename _Type,
         typename = void>
struct has_get_html_kind_method : std::false_type
{};

template<typename _Type>
struct has_get_html_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_html_kind())
>> : std::true_type
{};


// has_html_kind_access
//   trait: true if _Type exposes html_kind in either form.
template<typename _Type>
struct has_html_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_html_kind_method<_Type>::value ||
          has_get_html_kind_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                II.   CLASS LIST DETECTION                               ///
///////////////////////////////////////////////////////////////////////////////

// has_class_list_method
//   trait: true if _Type exposes `class_list()` const returning
// an iterable of class-name strings.
template<typename _Type,
         typename = void>
struct has_class_list_method : std::false_type
{};

template<typename _Type>
struct has_class_list_method<_Type, void_t<
    decltype(std::declval<const _Type&>().class_list())
>> : std::true_type
{};


// has_add_class_method
//   trait: true if _Type exposes `add_class(const std::string&)`.
template<typename _Type,
         typename = void>
struct has_add_class_method : std::false_type
{};

template<typename _Type>
struct has_add_class_method<_Type, void_t<
    decltype(std::declval<_Type&>().add_class(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_remove_class_method
//   trait: true if _Type exposes `remove_class(const std::string&)`.
template<typename _Type,
         typename = void>
struct has_remove_class_method : std::false_type
{};

template<typename _Type>
struct has_remove_class_method<_Type, void_t<
    decltype(std::declval<_Type&>().remove_class(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_has_class_method
//   trait: true if _Type exposes `has_class(const std::string&)`
// returning a boolean.
template<typename _Type,
         typename = void>
struct has_has_class_method : std::false_type
{};

template<typename _Type>
struct has_has_class_method<_Type, void_t<
    decltype(std::declval<const _Type&>().has_class(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_id_method
//   trait: true if _Type exposes `get_id()` const returning a
// string-convertible id value. Both `id()` and `get_id()` forms
// detected.
template<typename _Type,
         typename = void>
struct has_id_method : std::false_type
{};

template<typename _Type>
struct has_id_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_id())
>> : std::true_type
{};


// has_set_id_method
//   trait: true if _Type exposes `set_id(const std::string&)`.
template<typename _Type,
         typename = void>
struct has_set_id_method : std::false_type
{};

template<typename _Type>
struct has_set_id_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_id(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_class_support
//   trait: true if _Type exposes the full class-list interface
// (has_class + add_class + remove_class).
template<typename _Type>
struct has_class_support
{
    D_STATIC_CONSTEXPR bool value =
        ( has_has_class_method<_Type>::value    &&
          has_add_class_method<_Type>::value    &&
          has_remove_class_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                III.   DOCTYPE & VERSION DETECTION                       ///
///////////////////////////////////////////////////////////////////////////////

// has_doctype_method
//   trait: true if _Type exposes `doctype()` const returning a
// string-convertible DOCTYPE declaration.
template<typename _Type,
         typename = void>
struct has_doctype_method : std::false_type
{};

template<typename _Type>
struct has_doctype_method<_Type, void_t<
    decltype(std::declval<const _Type&>().doctype())
>> : std::true_type
{};


// has_get_doctype_method
//   trait: true if _Type exposes `get_doctype()` const.
template<typename _Type,
         typename = void>
struct has_get_doctype_method : std::false_type
{};

template<typename _Type>
struct has_get_doctype_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_doctype())
>> : std::true_type
{};


// has_doctype_access
//   trait: true if _Type exposes a doctype accessor in either form.
template<typename _Type>
struct has_doctype_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_doctype_method<_Type>::value ||
          has_get_doctype_method<_Type>::value );
};


// has_set_doctype_method
//   trait: true if _Type exposes `set_doctype(const std::string&)`.
template<typename _Type,
         typename = void>
struct has_set_doctype_method : std::false_type
{};

template<typename _Type>
struct has_set_doctype_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_doctype(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_html_version_method
//   trait: true if _Type exposes `html_version()` const returning
// an `html_version` enum value.
template<typename _Type,
         typename = void>
struct has_html_version_method : std::false_type
{};

template<typename _Type>
struct has_html_version_method<_Type, void_t<
    decltype(std::declval<const _Type&>().html_version())
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                IV.   HEAD / BODY ACCESSOR DETECTION                     ///
///////////////////////////////////////////////////////////////////////////////

// has_head_element_method
//   trait: true if _Type exposes `head_element()` const returning
// a node-shaped facade for the `<head>` element.
template<typename _Type,
         typename = void>
struct has_head_element_method : std::false_type
{};

template<typename _Type>
struct has_head_element_method<_Type, void_t<
    decltype(std::declval<const _Type&>().head_element())
>> : std::true_type
{};


// has_body_element_method
//   trait: true if _Type exposes `body_element()` const returning
// a node-shaped facade for the `<body>` element.
template<typename _Type,
         typename = void>
struct has_body_element_method : std::false_type
{};

template<typename _Type>
struct has_body_element_method<_Type, void_t<
    decltype(std::declval<const _Type&>().body_element())
>> : std::true_type
{};


// has_title_method
//   trait: true if _Type exposes `title()` const returning the
// document title as a string.
template<typename _Type,
         typename = void>
struct has_title_method : std::false_type
{};

template<typename _Type>
struct has_title_method<_Type, void_t<
    decltype(std::declval<const _Type&>().title())
>> : std::true_type
{};


// has_set_title_method
//   trait: true if _Type exposes `set_title(const std::string&)`.
template<typename _Type,
         typename = void>
struct has_set_title_method : std::false_type
{};

template<typename _Type>
struct has_set_title_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_title(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_html_dom_shortcuts
//   trait: true if _Type exposes head_element + body_element.
template<typename _Type>
struct has_html_dom_shortcuts
{
    D_STATIC_CONSTEXPR bool value =
        ( has_head_element_method<_Type>::value &&
          has_body_element_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                V.   HTML ELEMENT CLASSIFICATION                         ///
///////////////////////////////////////////////////////////////////////////////

// is_html_element
//   trait: true if _Type satisfies the XML element protocol AND
// exposes an HTML kind discriminator. The minimum bar is the XML
// element protocol (the type behaves like a tag-attribute-text
// node); the HTML kind accessor is what marks the type as
// HTML-aware. Library backends without an `html_kind()` method
// can still be treated as HTML by wrapping them in `html_element`
// (which derives from xml_node and adds the accessor).
template<typename _Type>
struct is_html_element
{
    D_STATIC_CONSTEXPR bool value =
        ( is_xml_element<_Type>::value &&
          has_html_kind_access<_Type>::value );
};


// is_html_element_loose
//   trait: looser detection -- any XML element is treated as a
// candidate HTML element. Used by adapter code that wants to wrap
// raw XML/DOM types into HTML facades regardless of provenance.
template<typename _Type>
struct is_html_element_loose
{
    D_STATIC_CONSTEXPR bool value =
        is_xml_element<_Type>::value;
};


// html_element_class
//   struct: comprehensive classification of an HTML-element-shaped
// type. Aggregates XML node classification with HTML extensions.
template<typename _Type>
struct html_element_class
{
    // identity
    D_STATIC_CONSTEXPR bool is_html_elem      =
        is_html_element<_Type>::value;
    D_STATIC_CONSTEXPR bool is_xml_node       =
        ::djinterp::is_xml_node<_Type>::value;
    D_STATIC_CONSTEXPR bool is_xml_element    =
        ::djinterp::is_xml_element<_Type>::value;

    // HTML semantic accessors
    D_STATIC_CONSTEXPR bool has_html_kind     =
        has_html_kind_access<_Type>::value;

    // class list
    D_STATIC_CONSTEXPR bool has_class_list    =
        has_class_list_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_add_class     =
        has_add_class_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_remove_class  =
        has_remove_class_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_has_class     =
        has_has_class_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_full_classes  =
        has_class_support<_Type>::value;

    // id
    D_STATIC_CONSTEXPR bool has_id            =
        has_id_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_set_id        =
        has_set_id_method<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   HTML DOCUMENT CLASSIFICATION                       ///
///////////////////////////////////////////////////////////////////////////////

// is_html_document
//   trait: true if _Type satisfies the XML document protocol AND
// exposes either a doctype accessor or HTML version accessor.
template<typename _Type>
struct is_html_document
{
    D_STATIC_CONSTEXPR bool value =
        ( is_xml_document<_Type>::value &&
          ( has_doctype_access<_Type>::value      ||
            has_html_version_method<_Type>::value ) );
};


// is_html_document_loose
//   trait: looser detection -- any XML document is treated as a
// candidate HTML document.
template<typename _Type>
struct is_html_document_loose
{
    D_STATIC_CONSTEXPR bool value =
        is_xml_document<_Type>::value;
};


// html_document_class
//   struct: comprehensive classification of an HTML-document-shaped
// type. Aggregates XML document classification with HTML extensions.
template<typename _Type>
struct html_document_class
{
    // identity
    D_STATIC_CONSTEXPR bool is_html_doc       =
        is_html_document<_Type>::value;
    D_STATIC_CONSTEXPR bool is_xml_doc        =
        ::djinterp::is_xml_document<_Type>::value;

    // doctype
    D_STATIC_CONSTEXPR bool has_doctype       =
        has_doctype_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_set_doctype   =
        has_set_doctype_method<_Type>::value;

    // version
    D_STATIC_CONSTEXPR bool has_version       =
        has_html_version_method<_Type>::value;

    // shortcuts
    D_STATIC_CONSTEXPR bool has_head          =
        has_head_element_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_body          =
        has_body_element_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_dom_shortcuts =
        has_html_dom_shortcuts<_Type>::value;
    D_STATIC_CONSTEXPR bool has_title         =
        has_title_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_set_title     =
        has_set_title_method<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VII.   BACKEND COMPLETENESS DETECTION                    ///
///////////////////////////////////////////////////////////////////////////////

// has_html_element_type_alias
//   trait: true if _Type exposes a nested `element_type` or
// `html_element_type` alias naming the concrete element class.
template<typename _Type,
         typename = void>
struct has_html_element_type_alias : std::false_type
{};

template<typename _Type>
struct has_html_element_type_alias<_Type, void_t<
    typename _Type::element_type
>> : std::true_type
{};


// has_html_document_type_alias
//   trait: true if _Type exposes a nested `document_type` alias.
template<typename _Type,
         typename = void>
struct has_html_document_type_alias : std::false_type
{};

template<typename _Type>
struct has_html_document_type_alias<_Type, void_t<
    typename _Type::document_type
>> : std::true_type
{};


// has_make_html_document_method
//   trait: true if _Type exposes a static factory
// `make_html_document()` returning a document_type instance.
template<typename _Type,
         typename = void>
struct has_make_html_document_method : std::false_type
{};

template<typename _Type>
struct has_make_html_document_method<_Type, void_t<
    decltype(_Type::make_html_document())
>> : std::true_type
{};


// is_html_backend_complete
//   trait: true if _Type exposes the full HTML backend protocol
// (every nested type alias plus the make_html_document factory).
// Stronger than `is_html_backend` (which only checks for the
// backend tag); used by code that needs to confidently
// instantiate `html_element<_Backend>` and
// `html_document<_Backend>`.
template<typename _Type>
struct is_html_backend_complete
{
    D_STATIC_CONSTEXPR bool value =
        ( has_html_element_type_alias<_Type>::value  &&
          has_html_document_type_alias<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VIII.   VARIABLE TEMPLATES                               ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool has_html_kind_access_v =
        has_html_kind_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_class_support_v =
        has_class_support<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_doctype_access_v =
        has_doctype_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_html_version_method_v =
        has_html_version_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_html_dom_shortcuts_v =
        has_html_dom_shortcuts<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_html_element_v =
        is_html_element<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_html_element_loose_v =
        is_html_element_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_html_document_v =
        is_html_document<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_html_document_loose_v =
        is_html_document_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_html_backend_complete_v =
        is_html_backend_complete<_Type>::value;

#endif  // variable templates


}   // namespace html
NS_END  // djinterp


#endif  // DJINTERP_HTML_TEMPLATE_TRAITS_
