/*******************************************************************************
* djinterp [core]                                                        xml.hpp
*
*   Foundational XML module for the djinterp framework. Provides shared
* types, node-type enumerations, encoding identifiers, backend tag
* dispatch, the tag-based detection trait `is_xml_backend`, the full set
* of structural SFINAE detection traits for attribute / node / document /
* backend types, and -- under C++20 -- the matching concept wrappers.
*
*   This is the foundation header: it has no dependency on the templated
* node/document types. `xml_template.hpp` is a separate module that
* includes this header for its shared types and trait system; including
* `xml.hpp` alone gives the vocabulary (types, traits, concepts) without
* pulling in a concrete document implementation.
*
*   CONSOLIDATION:
*   The detection traits (formerly `xml_template_traits.hpp`) and the
* C++20 concept wrappers (formerly `xml_template_concepts.hpp`) now live
* here directly, so a single `#include "xml.hpp"` brings in the shared
* types, the trait system, and the concepts together.
*
*   LIBRARY AGNOSTICISM:
*   This module does NOT include or depend on any third-party XML
* library (libxml++, pugixml, tinyxml2, RapidXML, etc.). All
* interaction with such libraries is by structural duck-typing through
* the trait system below. Adapters for specific libraries live in their
* own headers (e.g. `xml_libxmlpp_adapter.hpp`) and are NOT pulled in by
* this header.
*
*   STRUCTURAL DETECTION:
*   All detection is purely structural -- no base-class checks, no
* registration, and (beyond the optional `backend_tag`) no tag types.
* Expose the right members and the trait system classifies the type
* automatically. To accommodate the divergent naming conventions of the
* major C++ XML libraries, both short-form (`name()`, `value()`) and
* getter-form (`get_name()`, `get_value()`) accessors are detected.
*
*   BACKEND MODEL:
*   `xml_node` and `xml_document` are templated over a `Backend` type
* parameter. The backend is any type that satisfies the structural
* protocol detected by `is_xml_backend`. A default in-memory backend
* (`xml_default_backend`) is provided in `xml_template.hpp`.
*
*   COMPAT:
*   C++11: all traits via struct::value
*   C++14: _v variable templates (gated)
*   C++17: if constexpr usable in consumer code
*   C++20: concept wrappers (gated behind D_ENV_CPP_FEATURE_LANG_CONCEPTS)
*
*
* path:      /inc/djinterp/core/text/xml/xml.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.08
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TYPES & CONSTANTS
      ------------------------
      i.    xml_string_t
      ii.   xml_size_t
      iii.  D_XML_DEFAULT_VERSION
      iv.   D_XML_DEFAULT_ENCODING
      v.    D_XML_DEFAULT_INDENT

II.   CORE ENUMERATIONS
      -----------------
      a. xml_node_kind
      b. xml_encoding
      c.    xml_standalone

III.  BACKEND TAG DISPATCH
      --------------------
      a. xml_backend_tag
      b. xml_default_backend_tag
      c.    xml_libxmlpp_backend_tag
            d. xml_pugixml_backend_tag
            e. xml_tinyxml2_backend_tag
            f. xml_rapidxml_backend_tag

IV.   BACKEND TAG DETECTION
      ---------------------
      i.    has_backend_tag_helper (internal)
      ii.   is_xml_backend
            a. is_xml_backend_v

V.    ACCESSOR DETECTION (NAME / VALUE / KIND)
      ----------------------------------------
      a. has_name_method,         has_get_name_method
      b. has_value_method,        has_get_value_method
      c.    has_kind_method,         has_get_kind_method
            d. has_name_access,         has_value_access, has_kind_access

VI.   ATTRIBUTE DETECTION
      -------------------
      a. has_attributes_method,   has_get_attributes_method
      b. has_find_attribute_method
      c.    has_set_attribute_method
            d. has_remove_attribute_method
            e. has_attribute_count_method
            f. is_xml_attribute

VII.  CHILD / TRAVERSAL DETECTION
      ---------------------------
      a. has_children_method,     has_get_children_method
      b. has_first_child_method
      c.    has_parent_method
            d. has_find_child_method
            e. has_add_child_method
            f. has_remove_child_method
            g. has_child_count_method
            h. has_traversal

VIII. TEXT CONTENT DETECTION
      ----------------------
      a. has_text_method,         has_get_text_method
      b. has_set_text_method
      c.    has_text_access

IX.   NODE CLASSIFICATION
      -------------------
      a. is_xml_node
      b. is_xml_element
      c.    xml_node_class

X.    DOCUMENT DETECTION
      ------------------
      a. has_root_element_method, has_document_element_method
      b. has_version_method
      c.    has_encoding_method
            d. has_standalone_method
            e. has_parse_method
            f. has_write_method
            g. has_save_method
            h. is_xml_document
      i.    xml_document_class

XI.   BACKEND PROTOCOL DETECTION
      --------------------------
      a. has_node_type_alias
      b. has_attribute_type_alias
      c.    has_document_type_alias
            d. has_make_document_method
            e. is_xml_backend_complete

XII.  VARIABLE TEMPLATES
      ------------------

XIII. CONCEPTS (C++20)
      ----------------
      a. leaf protocol concepts       (attribute, node, element, document)
      b. capability concepts          (mutable, parseable, writable, savable)
      c.    composition concepts         (full_xml_node, full_xml_document)
            d. backend concepts             (xml_backend_type, complete_xml_backend)
*/

#ifndef DJINTERP_TEXT_XML_XML_HPP
#define DJINTERP_TEXT_XML_XML_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint8_t


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED TYPES & CONSTANTS                            ///
///////////////////////////////////////////////////////////////////////////////

NS_DJINTERP


// xml_string_t
//   type: string type used for XML names, attribute values, text
// content, and CDATA payloads. Defined as std::string by default;
// define D_XML_STRING_TYPE before inclusion to override (e.g. with
// std::u8string under C++20 or a small-string-optimised wrapper).
#ifndef D_XML_STRING_TYPE
    using xml_string_t = std::string;
#else
    using xml_string_t = D_XML_STRING_TYPE;
#endif


// xml_size_t
//   type: unsigned size type for XML counts (children, attributes,
// document size). Defined as std::size_t by default.
#ifndef D_XML_SIZE_TYPE
    using xml_size_t = std::size_t;
#else
    using xml_size_t = D_XML_SIZE_TYPE;
#endif


// D_XML_DEFAULT_VERSION
//   constant: default XML version string emitted in the prolog.
#ifndef D_XML_DEFAULT_VERSION
    #define D_XML_DEFAULT_VERSION       "1.0"
#endif


// D_XML_DEFAULT_ENCODING
//   constant: default XML encoding string emitted in the prolog.
#ifndef D_XML_DEFAULT_ENCODING
    #define D_XML_DEFAULT_ENCODING      "UTF-8"
#endif


// D_XML_DEFAULT_INDENT
//   constant: default whitespace string used per indentation level
// when serialising.
#ifndef D_XML_DEFAULT_INDENT
    #define D_XML_DEFAULT_INDENT        "  "
#endif


///////////////////////////////////////////////////////////////////////////////
///                  II.   CORE ENUMERATIONS                                ///
///////////////////////////////////////////////////////////////////////////////

// xml_node_kind
//   enum: discriminator for the kind of an XML node. Modelled on the
// W3C DOM Node.nodeType set, narrowed to the subset most XML
// libraries agree on. Values are intentionally non-contiguous so new
// kinds can be inserted without breaking existing serialisation.
enum class xml_node_kind : re_std::uint8_t
{
    element                = 1,
    attribute              = 2,
    text                   = 3,
    cdata_section          = 4,
    entity_reference       = 5,
    processing_instruction = 7,
    comment                = 8,
    document               = 9,
    document_type          = 10,
    document_fragment      = 11,
    notation               = 12,
    unknown                = 0
};


// xml_encoding
//   enum: well-known character encodings used by XML documents.
// Carry the literal string via `xml_encoding_name`; backends are
// free to accept arbitrary encoding strings beyond this list.
enum class xml_encoding : re_std::uint8_t
{
    utf_8,
    utf_16,
    utf_16_le,
    utf_16_be,
    utf_32,
    iso_8859_1,
    us_ascii,
    custom
};


// xml_standalone
//   enum: tri-state for the XML declaration's `standalone`
// attribute. `unspecified` means the attribute is omitted entirely
// from the prolog.
enum class xml_standalone : re_std::uint8_t
{
    unspecified,
    yes,
    no
};


// xml_encoding_name
//   function: returns the canonical IANA-style string for an
// `xml_encoding` value, or an empty string for `custom`.
D_CONSTEXPR_INLINE const char*
xml_encoding_name(
    xml_encoding _enc
)
{
    return (_enc == xml_encoding::utf_8)      ? "UTF-8"
         : (_enc == xml_encoding::utf_16)     ? "UTF-16"
         : (_enc == xml_encoding::utf_16_le)  ? "UTF-16LE"
         : (_enc == xml_encoding::utf_16_be)  ? "UTF-16BE"
         : (_enc == xml_encoding::utf_32)     ? "UTF-32"
         : (_enc == xml_encoding::iso_8859_1) ? "ISO-8859-1"
         : (_enc == xml_encoding::us_ascii)   ? "US-ASCII"
         :                                      "";
}


///////////////////////////////////////////////////////////////////////////////
///                III.   BACKEND TAG DISPATCH                              ///
///////////////////////////////////////////////////////////////////////////////

// xml_backend_tag
//   struct: empty base tag for all XML backend tag types. Roots
// the tag hierarchy used by `is_xml_backend` and by adapter
// specialisations of `xml_node` / `xml_document`.
struct xml_backend_tag
{};

// xml_default_backend_tag
//   struct: tag identifying the bundled in-memory backend defined
// in `xml_template.hpp`.
struct xml_default_backend_tag : xml_backend_tag
{};

// xml_libxmlpp_backend_tag
//   struct: tag identifying a libxml++-based backend. The adapter
// itself lives in a separate header; this tag is provided here so
// detection code never has to depend on libxml++ headers.
struct xml_libxmlpp_backend_tag : xml_backend_tag
{};

// xml_pugixml_backend_tag
//   struct: tag identifying a pugixml-based backend.
struct xml_pugixml_backend_tag : xml_backend_tag
{};

// xml_tinyxml2_backend_tag
//   struct: tag identifying a tinyxml2-based backend.
struct xml_tinyxml2_backend_tag : xml_backend_tag
{};

// xml_rapidxml_backend_tag
//   struct: tag identifying a RapidXML-based backend.
struct xml_rapidxml_backend_tag : xml_backend_tag
{};


///////////////////////////////////////////////////////////////////////////////
///                  IV.   BACKEND TAG DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// is_xml_backend
//   trait: detects whether Type is an XML backend type by
// checking for a nested `backend_tag` alias. This is the tag-based
// gate; the *structural* completeness check (nested type aliases +
// make_document) is `is_xml_backend_complete`, in section XI.

NS_INTERNAL

    // has_backend_tag_helper
    //   trait: SFINAE helper; primary template (failure case).
    template<typename Type,
             typename = void>
    struct has_backend_tag_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_backend_tag_helper (specialization)
    //   trait: success case when Type::backend_tag exists.
    template<typename Type>
    struct has_backend_tag_helper<Type,
                                  void_t<typename Type::backend_tag>>
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal

// is_xml_backend
//   trait: true if Type has a nested backend_tag type derived
// from xml_backend_tag.
template<typename Type>
struct is_xml_backend
{
    D_STATIC_CONSTEXPR bool value =
        internal::has_backend_tag_helper<clean_t<Type>>::value;
};

// is_xml_backend_v
//   constant: convenience accessor for
// is_xml_backend<Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_STATIC_CONSTEXPR bool is_xml_backend_v =
        is_xml_backend<Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///           V.   ACCESSOR DETECTION (NAME / VALUE / KIND)                 ///
///////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------
//  name accessor
// ---------------------------------------------------------------------

// has_name_method
//   trait: true if Type exposes a `name()` const method (pugixml /
// short-form convention).
template<typename Type,
         typename = void>
struct has_name_method : std::false_type
{};

template<typename Type>
struct has_name_method<Type, void_t<
    decltype(std::declval<const Type&>().name())
>> : std::true_type
{};


// has_get_name_method
//   trait: true if Type exposes a `get_name()` const method (libxml++
// / getter-form convention).
template<typename Type,
         typename = void>
struct has_get_name_method : std::false_type
{};

template<typename Type>
struct has_get_name_method<Type, void_t<
    decltype(std::declval<const Type&>().get_name())
>> : std::true_type
{};


// has_name_access
//   trait: true if Type exposes a name accessor in either form.
template<typename Type>
struct has_name_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_name_method<Type>::value ||
          has_get_name_method<Type>::value );
};


// ---------------------------------------------------------------------
//  value accessor
// ---------------------------------------------------------------------

// has_value_method
//   trait: true if Type exposes a `value()` const method.
template<typename Type,
         typename = void>
struct has_value_method : std::false_type
{};

template<typename Type>
struct has_value_method<Type, void_t<
    decltype(std::declval<const Type&>().value())
>> : std::true_type
{};


// has_get_value_method
//   trait: true if Type exposes a `get_value()` const method.
template<typename Type,
         typename = void>
struct has_get_value_method : std::false_type
{};

template<typename Type>
struct has_get_value_method<Type, void_t<
    decltype(std::declval<const Type&>().get_value())
>> : std::true_type
{};


// has_value_access
//   trait: true if Type exposes a value accessor in either form.
template<typename Type>
struct has_value_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_value_method<Type>::value ||
          has_get_value_method<Type>::value );
};


// ---------------------------------------------------------------------
//  kind accessor
// ---------------------------------------------------------------------

// has_kind_method
//   trait: true if Type exposes a `kind()` const method returning a
// node-kind discriminator.
template<typename Type,
         typename = void>
struct has_kind_method : std::false_type
{};

template<typename Type>
struct has_kind_method<Type, void_t<
    decltype(std::declval<const Type&>().kind())
>> : std::true_type
{};


// has_get_kind_method
//   trait: true if Type exposes a `get_kind()` or `node_type()` const
// method. Detection covers `node_type()` because libxml++ exposes the
// kind under that name.
template<typename Type,
         typename = void>
struct has_get_kind_method : std::false_type
{};

template<typename Type>
struct has_get_kind_method<Type, void_t<
    decltype(std::declval<const Type&>().node_type())
>> : std::true_type
{};


// has_kind_access
//   trait: true if Type exposes a kind accessor in either form.
template<typename Type>
struct has_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_kind_method<Type>::value ||
          has_get_kind_method<Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   ATTRIBUTE DETECTION                                ///
///////////////////////////////////////////////////////////////////////////////

// has_attributes_method
//   trait: true if Type exposes an `attributes()` const method
// returning an iterable.
template<typename Type,
         typename = void>
struct has_attributes_method : std::false_type
{};

template<typename Type>
struct has_attributes_method<Type, void_t<
    decltype(std::declval<const Type&>().attributes())
>> : std::true_type
{};


// has_get_attributes_method
//   trait: true if Type exposes `get_attributes()` (libxml++).
template<typename Type,
         typename = void>
struct has_get_attributes_method : std::false_type
{};

template<typename Type>
struct has_get_attributes_method<Type, void_t<
    decltype(std::declval<const Type&>().get_attributes())
>> : std::true_type
{};


// has_find_attribute_method
//   trait: true if Type exposes `find_attribute(name)` returning
// either an attribute pointer or an iterator-like value.
template<typename Type,
         typename = void>
struct has_find_attribute_method : std::false_type
{};

template<typename Type>
struct has_find_attribute_method<Type, void_t<
    decltype(std::declval<const Type&>().find_attribute(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_set_attribute_method
//   trait: true if Type exposes `set_attribute(name, value)`.
template<typename Type,
         typename = void>
struct has_set_attribute_method : std::false_type
{};

template<typename Type>
struct has_set_attribute_method<Type, void_t<
    decltype(std::declval<Type&>().set_attribute(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_remove_attribute_method
//   trait: true if Type exposes `remove_attribute(name)`.
template<typename Type,
         typename = void>
struct has_remove_attribute_method : std::false_type
{};

template<typename Type>
struct has_remove_attribute_method<Type, void_t<
    decltype(std::declval<Type&>().remove_attribute(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_attribute_count_method
//   trait: true if Type exposes `attribute_count()` const.
template<typename Type,
         typename = void>
struct has_attribute_count_method : std::false_type
{};

template<typename Type>
struct has_attribute_count_method<Type, void_t<
    decltype(std::declval<const Type&>().attribute_count())
>> : std::true_type
{};


// is_xml_attribute
//   trait: minimum protocol -- a name accessor and a value accessor.
// This keeps the bar low so libxml++'s `Attribute`, pugixml's
// `xml_attribute`, and our own `xml_attribute` all qualify.
template<typename Type>
struct is_xml_attribute
{
    D_STATIC_CONSTEXPR bool value =
        ( has_name_access<Type>::value &&
          has_value_access<Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///             VII.   CHILD / TRAVERSAL DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// has_children_method
//   trait: true if Type exposes `children()` const returning an
// iterable.
template<typename Type,
         typename = void>
struct has_children_method : std::false_type
{};

template<typename Type>
struct has_children_method<Type, void_t<
    decltype(std::declval<const Type&>().children())
>> : std::true_type
{};


// has_get_children_method
//   trait: true if Type exposes `get_children()` (libxml++).
template<typename Type,
         typename = void>
struct has_get_children_method : std::false_type
{};

template<typename Type>
struct has_get_children_method<Type, void_t<
    decltype(std::declval<const Type&>().get_children())
>> : std::true_type
{};


// has_first_child_method
//   trait: true if Type exposes `first_child()` (pugixml /
// linked-list traversal style).
template<typename Type,
         typename = void>
struct has_first_child_method : std::false_type
{};

template<typename Type>
struct has_first_child_method<Type, void_t<
    decltype(std::declval<const Type&>().first_child())
>> : std::true_type
{};


// has_parent_method
//   trait: true if Type exposes `parent()` const.
template<typename Type,
         typename = void>
struct has_parent_method : std::false_type
{};

template<typename Type>
struct has_parent_method<Type, void_t<
    decltype(std::declval<const Type&>().parent())
>> : std::true_type
{};


// has_find_child_method
//   trait: true if Type exposes `find_child(name)`.
template<typename Type,
         typename = void>
struct has_find_child_method : std::false_type
{};

template<typename Type>
struct has_find_child_method<Type, void_t<
    decltype(std::declval<const Type&>().find_child(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_add_child_method
//   trait: true if Type exposes `add_child(name)`.
template<typename Type,
         typename = void>
struct has_add_child_method : std::false_type
{};

template<typename Type>
struct has_add_child_method<Type, void_t<
    decltype(std::declval<Type&>().add_child(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_remove_child_method
//   trait: true if Type exposes `remove_child(name)`.
template<typename Type,
         typename = void>
struct has_remove_child_method : std::false_type
{};

template<typename Type>
struct has_remove_child_method<Type, void_t<
    decltype(std::declval<Type&>().remove_child(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_child_count_method
//   trait: true if Type exposes `child_count()` const.
template<typename Type,
         typename = void>
struct has_child_count_method : std::false_type
{};

template<typename Type>
struct has_child_count_method<Type, void_t<
    decltype(std::declval<const Type&>().child_count())
>> : std::true_type
{};


// has_traversal
//   trait: true if Type exposes ANY child-enumeration accessor
// (children/get_children/first_child).
template<typename Type>
struct has_traversal
{
    D_STATIC_CONSTEXPR bool value =
        ( has_children_method<Type>::value     ||
          has_get_children_method<Type>::value ||
          has_first_child_method<Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VIII.   TEXT CONTENT DETECTION                           ///
///////////////////////////////////////////////////////////////////////////////

// has_text_method
//   trait: true if Type exposes `text()` returning the
// concatenated text content of its descendants.
template<typename Type,
         typename = void>
struct has_text_method : std::false_type
{};

template<typename Type>
struct has_text_method<Type, void_t<
    decltype(std::declval<const Type&>().text())
>> : std::true_type
{};


// has_get_text_method
//   trait: true if Type exposes `get_text()`.
template<typename Type,
         typename = void>
struct has_get_text_method : std::false_type
{};

template<typename Type>
struct has_get_text_method<Type, void_t<
    decltype(std::declval<const Type&>().get_text())
>> : std::true_type
{};


// has_set_text_method
//   trait: true if Type exposes `set_text(text)`.
template<typename Type,
         typename = void>
struct has_set_text_method : std::false_type
{};

template<typename Type>
struct has_set_text_method<Type, void_t<
    decltype(std::declval<Type&>().set_text(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_text_access
//   trait: true if Type exposes a text accessor in either form.
template<typename Type>
struct has_text_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_text_method<Type>::value ||
          has_get_text_method<Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                IX.   NODE CLASSIFICATION                                ///
///////////////////////////////////////////////////////////////////////////////

// is_xml_node
//   trait: minimum node protocol -- exposes a name accessor and
// some form of child traversal. Deliberately permissive so that
// pugixml, libxml++, tinyxml2, and our default backend all qualify.
template<typename Type>
struct is_xml_node
{
    D_STATIC_CONSTEXPR bool value =
        ( has_name_access<Type>::value &&
          has_traversal<Type>::value );
};


// is_xml_element
//   trait: full element-node protocol -- node + attributes + text
// access. An element is the concrete tag type that carries
// attributes and child content.
template<typename Type>
struct is_xml_element
{
    D_STATIC_CONSTEXPR bool value =
        ( is_xml_node<Type>::value &&
          ( has_attributes_method<Type>::value     ||
            has_get_attributes_method<Type>::value ) );
};


// xml_node_class
//   struct: comprehensive classification of an XML-node-shaped
// type. Aggregates every detection trait into one read-out so
// generic code can branch on capabilities cheaply.
template<typename Type>
struct xml_node_class
{
    // identity
    D_STATIC_CONSTEXPR bool is_node       =
        is_xml_node<Type>::value;
    D_STATIC_CONSTEXPR bool is_element    =
        is_xml_element<Type>::value;

    // accessors
    D_STATIC_CONSTEXPR bool has_name      =
        has_name_access<Type>::value;
    D_STATIC_CONSTEXPR bool has_value     =
        has_value_access<Type>::value;
    D_STATIC_CONSTEXPR bool has_kind      =
        has_kind_access<Type>::value;
    D_STATIC_CONSTEXPR bool has_text      =
        has_text_access<Type>::value;

    // attributes
    D_STATIC_CONSTEXPR bool has_attrs     =
        ( has_attributes_method<Type>::value ||
          has_get_attributes_method<Type>::value );
    D_STATIC_CONSTEXPR bool has_find_attr =
        has_find_attribute_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_set_attr  =
        has_set_attribute_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_attr_count =
        has_attribute_count_method<Type>::value;

    // traversal
    D_STATIC_CONSTEXPR bool has_kids      =
        has_traversal<Type>::value;
    D_STATIC_CONSTEXPR bool has_first_kid =
        has_first_child_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_find_kid  =
        has_find_child_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_add_kid   =
        has_add_child_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_kid_count =
        has_child_count_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_parent    =
        has_parent_method<Type>::value;

    // mutability
    D_STATIC_CONSTEXPR bool is_mutable    =
        ( has_set_attr     ||
          has_add_kid      ||
          has_set_text_method<Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                X.   DOCUMENT DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

// has_root_element_method
//   trait: true if Type exposes `root_element()` const.
template<typename Type,
         typename = void>
struct has_root_element_method : std::false_type
{};

template<typename Type>
struct has_root_element_method<Type, void_t<
    decltype(std::declval<const Type&>().root_element())
>> : std::true_type
{};


// has_document_element_method
//   trait: true if Type exposes `document_element()` (DOM-style).
template<typename Type,
         typename = void>
struct has_document_element_method : std::false_type
{};

template<typename Type>
struct has_document_element_method<Type, void_t<
    decltype(std::declval<const Type&>().document_element())
>> : std::true_type
{};


// has_root_access
//   trait: true if Type exposes a root-element accessor in
// either form.
template<typename Type>
struct has_root_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_root_element_method<Type>::value ||
          has_document_element_method<Type>::value );
};


// has_version_method
//   trait: true if Type exposes `version()` const.
template<typename Type,
         typename = void>
struct has_version_method : std::false_type
{};

template<typename Type>
struct has_version_method<Type, void_t<
    decltype(std::declval<const Type&>().version())
>> : std::true_type
{};


// has_encoding_method
//   trait: true if Type exposes `encoding()` const.
template<typename Type,
         typename = void>
struct has_encoding_method : std::false_type
{};

template<typename Type>
struct has_encoding_method<Type, void_t<
    decltype(std::declval<const Type&>().encoding())
>> : std::true_type
{};


// has_standalone_method
//   trait: true if Type exposes `standalone()` const.
template<typename Type,
         typename = void>
struct has_standalone_method : std::false_type
{};

template<typename Type>
struct has_standalone_method<Type, void_t<
    decltype(std::declval<const Type&>().standalone())
>> : std::true_type
{};


// has_parse_method
//   trait: true if Type exposes `parse(const std::string&)`. Used
// to detect documents that can ingest serialised XML in-place.
template<typename Type,
         typename = void>
struct has_parse_method : std::false_type
{};

template<typename Type>
struct has_parse_method<Type, void_t<
    decltype(std::declval<Type&>().parse(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_write_method
//   trait: true if Type exposes `write()` returning a string-
// convertible serialisation.
template<typename Type,
         typename = void>
struct has_write_method : std::false_type
{};

template<typename Type>
struct has_write_method<Type, void_t<
    decltype(std::declval<const Type&>().write())
>> : std::true_type
{};


// has_save_method
//   trait: true if Type exposes `save(const std::string&)` taking
// a file path.
template<typename Type,
         typename = void>
struct has_save_method : std::false_type
{};

template<typename Type>
struct has_save_method<Type, void_t<
    decltype(std::declval<const Type&>().save(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// is_xml_document
//   trait: minimum document protocol -- exposes a root-element
// accessor in some form.
template<typename Type>
struct is_xml_document
{
    D_STATIC_CONSTEXPR bool value =
        has_root_access<Type>::value;
};


// xml_document_class
//   struct: comprehensive classification of an XML-document-shaped
// type. Aggregates every detection trait into one read-out.
template<typename Type>
struct xml_document_class
{
    // identity
    D_STATIC_CONSTEXPR bool is_document    =
        is_xml_document<Type>::value;

    // root access
    D_STATIC_CONSTEXPR bool has_root       =
        has_root_access<Type>::value;

    // prolog accessors
    D_STATIC_CONSTEXPR bool has_version    =
        has_version_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_encoding   =
        has_encoding_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_standalone =
        has_standalone_method<Type>::value;

    // I/O
    D_STATIC_CONSTEXPR bool can_parse      =
        has_parse_method<Type>::value;
    D_STATIC_CONSTEXPR bool can_write      =
        has_write_method<Type>::value;
    D_STATIC_CONSTEXPR bool can_save       =
        has_save_method<Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                XI.   BACKEND PROTOCOL DETECTION                         ///
///////////////////////////////////////////////////////////////////////////////

// has_node_type_alias
//   trait: true if Type exposes a nested `node_type` alias. A
// backend's `node_type` names the concrete node class for that
// backend.
template<typename Type,
         typename = void>
struct has_node_type_alias : std::false_type
{};

template<typename Type>
struct has_node_type_alias<Type, void_t<
    typename clean_t<Type>::node_type
>> : std::true_type
{};


// has_attribute_type_alias
//   trait: true if Type exposes a nested `attribute_type` alias.
template<typename Type,
         typename = void>
struct has_attribute_type_alias : std::false_type
{};

template<typename Type>
struct has_attribute_type_alias<Type, void_t<
    typename clean_t<Type>::attribute_type
>> : std::true_type
{};


// has_document_type_alias
//   trait: true if Type exposes a nested `document_type` alias.
template<typename Type,
         typename = void>
struct has_document_type_alias : std::false_type
{};

template<typename Type>
struct has_document_type_alias<Type, void_t<
    typename clean_t<Type>::document_type
>> : std::true_type
{};


// has_make_document_method
//   trait: true if Type exposes a static or member factory
// `make_document()` returning a document_type instance.
template<typename Type,
         typename = void>
struct has_make_document_method : std::false_type
{};

template<typename Type>
struct has_make_document_method<Type, void_t<
    decltype(clean_t<Type>::make_document())
>> : std::true_type
{};


// is_xml_backend_complete
//   trait: true if Type exposes the full backend protocol --
// every nested type alias and a make_document factory. Stronger
// than `is_xml_backend` (which only checks for the backend_tag).
template<typename Type>
struct is_xml_backend_complete
{
    D_STATIC_CONSTEXPR bool value =
        ( has_node_type_alias<Type>::value      &&
          has_attribute_type_alias<Type>::value &&
          has_document_type_alias<Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                XII.  VARIABLE TEMPLATES                                 ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename Type>
    D_CONSTEXPR bool has_name_access_v =
        has_name_access<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool has_value_access_v =
        has_value_access<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool has_kind_access_v =
        has_kind_access<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool has_text_access_v =
        has_text_access<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool has_traversal_v =
        has_traversal<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool has_root_access_v =
        has_root_access<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool is_xml_attribute_v =
        is_xml_attribute<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool is_xml_node_v =
        is_xml_node<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool is_xml_element_v =
        is_xml_element<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool is_xml_document_v =
        is_xml_document<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool is_xml_backend_complete_v =
        is_xml_backend_complete<Type>::value;

#endif  // variable templates


///////////////////////////////////////////////////////////////////////////////
///                XIII.  CONCEPTS (C++20)                                  ///
///////////////////////////////////////////////////////////////////////////////

//   C++20 concept wrappers over the structural protocols above. Each
// concept is a thin shell over the corresponding SFINAE trait, so a
// type that satisfies the trait satisfies the concept and vice versa.
// The whole section is gated behind `D_ENV_CPP_FEATURE_LANG_CONCEPTS`;
// under older standards it expands to nothing so existing trait-based
// code keeps compiling unchanged. Where the trait variant is
// `is_xml_node<T>::value`, the concept variant is simply
// `xml_node_type<T>`.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ---------------------------------------------------------------------
//  leaf protocol concepts
// ---------------------------------------------------------------------

// xml_attribute_type
//   concept: constrains types satisfying the XML attribute
// protocol -- a name accessor and a value accessor in either
// short-form or get-form.
template<typename Type>
concept xml_attribute_type =
    is_xml_attribute<Type>::value;


// xml_node_type
//   concept: constrains types satisfying the minimum XML node
// protocol -- a name accessor and some form of child traversal.
template<typename Type>
concept xml_node_type =
    is_xml_node<Type>::value;


// xml_element_type
//   concept: constrains types satisfying the full XML element
// protocol -- node + attribute container.
template<typename Type>
concept xml_element_type =
    is_xml_element<Type>::value;


// xml_document_type
//   concept: constrains types satisfying the XML document
// protocol -- a root-element accessor in some form.
template<typename Type>
concept xml_document_type =
    is_xml_document<Type>::value;


// ---------------------------------------------------------------------
//  capability concepts
// ---------------------------------------------------------------------

// mutable_xml_node
//   concept: constrains nodes that can be mutated in place via
// at least one of set_attribute, add_child, or set_text.
template<typename Type>
concept mutable_xml_node =
    ( xml_node_type<Type> &&
      ( has_set_attribute_method<Type>::value ||
        has_add_child_method<Type>::value     ||
        has_set_text_method<Type>::value ) );


// parseable_xml_document
//   concept: constrains documents that can ingest serialised
// XML via a parse(string) method.
template<typename Type>
concept parseable_xml_document =
    ( xml_document_type<Type> &&
      has_parse_method<Type>::value );


// writable_xml_document
//   concept: constrains documents that can serialise to a
// string via write().
template<typename Type>
concept writable_xml_document =
    ( xml_document_type<Type> &&
      has_write_method<Type>::value );


// savable_xml_document
//   concept: constrains documents that can persist to a file
// path via save(path).
template<typename Type>
concept savable_xml_document =
    ( xml_document_type<Type> &&
      has_save_method<Type>::value );


// ---------------------------------------------------------------------
//  composition concepts
// ---------------------------------------------------------------------

// full_xml_node
//   concept: constrains nodes that expose the complete
// structural protocol -- name, kind, attributes, children,
// and text -- regardless of mutability.
template<typename Type>
concept full_xml_node =
    ( xml_node_type<Type>          &&
      has_kind_access<Type>::value &&
      has_text_access<Type>::value &&
      ( has_attributes_method<Type>::value ||
        has_get_attributes_method<Type>::value ) );


// full_xml_document
//   concept: constrains documents that expose the complete
// document protocol -- root + every prolog accessor + read +
// write.
template<typename Type>
concept full_xml_document =
    ( xml_document_type<Type>           &&
      has_version_method<Type>::value   &&
      has_encoding_method<Type>::value  &&
      has_write_method<Type>::value );


// ---------------------------------------------------------------------
//  backend concepts
// ---------------------------------------------------------------------

// xml_backend_type
//   concept: constrains types tagged as XML backends via the
// `backend_tag` nested alias.
template<typename Type>
concept xml_backend_type =
    is_xml_backend<Type>::value;


// complete_xml_backend
//   concept: constrains backends that expose the full set of
// nested type aliases (node_type, attribute_type, document_type)
// plus a make_document factory. This is the bar a backend must
// clear to be plugged into `xml_node<Backend>` and
// `xml_document<Backend>` without further specialisation.
template<typename Type>
concept complete_xml_backend =
    ( xml_backend_type<Type>                 &&
      is_xml_backend_complete<Type>::value   &&
      has_make_document_method<Type>::value );

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_TEXT_XML_XML_HPP
