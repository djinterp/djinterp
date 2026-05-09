/******************************************************************************
* djinterp [text]                                      xml_template_traits.hpp
*
*   Structural SFINAE detection traits for XML attribute, node, document,
* and backend types. All detection is purely structural -- no tag types,
* no base-class checks, no registration. Expose the right members and
* the trait system classifies the type automatically.
*
*   This header is what makes the XML module library-agnostic. Any
* third-party XML library whose types satisfy the structural protocol
* (member names + signatures) is detected as XML-shaped, regardless of
* its provenance. To accommodate the divergent naming conventions of
* the major C++ XML libraries, both short-form (`name()`, `value()`)
* and getter-form (`get_name()`, `get_value()`) accessors are detected.
*
*   DETECTED PROTOCOLS:
*
*   xml_attribute protocol:
*     A type exposing a name and a value, both string-convertible.
*     Detected via name()/get_name() and value()/get_value() in
*     either method form.
*
*   xml_node protocol:
*     A type exposing a node kind, a name, and (optionally) attributes,
*     children, and text content. The minimum protocol is name() + a
*     way to enumerate children.
*
*   xml_document protocol:
*     A type exposing a root_element() / document_element() and
*     (optionally) prolog accessors (version, encoding, standalone).
*
*   xml_backend protocol:
*     A type exposing nested type aliases (`node_type`, `attribute_type`,
*     `document_type`) plus factory functions for parsing and writing.
*
*   COMPAT:
*   C++11: all traits via struct::value
*   C++14: _v variable templates (gated)
*   C++17: if constexpr usable in consumer code
*   C++20: concept wrappers in xml_template_concepts.hpp
*
*
* path:      /inc/djinterp/core/text/xml/xml_template_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.08
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    NAME / VALUE ACCESSOR DETECTION
      ---------------------------------
      a. has_name_method,         has_get_name_method
      b. has_value_method,        has_get_value_method
      c. has_kind_method,         has_get_kind_method
      d. has_name_access,         has_value_access, has_kind_access

II.   ATTRIBUTE DETECTION
      --------------------
      a. has_attributes_method,   has_get_attributes_method
      b. has_find_attribute_method
      c. has_set_attribute_method
      d. has_remove_attribute_method
      e. has_attribute_count_method
      f. is_xml_attribute

III.  CHILD / TRAVERSAL DETECTION
      ----------------------------
      a. has_children_method,     has_get_children_method
      b. has_first_child_method
      c. has_parent_method
      d. has_find_child_method
      e. has_add_child_method
      f. has_remove_child_method
      g. has_child_count_method

IV.   TEXT CONTENT DETECTION
      ------------------------
      a. has_text_method,         has_get_text_method
      b. has_set_text_method

V.    NODE CLASSIFICATION
      ---------------------
      a. is_xml_node
      b. is_xml_element
      c. xml_node_class

VI.   DOCUMENT DETECTION
      -------------------
      a. has_root_element_method, has_document_element_method
      b. has_version_method
      c. has_encoding_method
      d. has_standalone_method
      e. has_parse_method
      f. has_write_method
      g. has_save_method
      h. is_xml_document
      i. xml_document_class

VII.  BACKEND DETECTION
      ------------------
      a. has_node_type_alias
      b. has_attribute_type_alias
      c. has_document_type_alias
      d. has_make_document_method
      e. is_xml_backend_complete

VIII. VARIABLE TEMPLATES
*/

#ifndef DJINTERP_XML_TEMPLATE_TRAITS_
#define DJINTERP_XML_TEMPLATE_TRAITS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///           I.   NAME / VALUE ACCESSOR DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------
//  name accessor
// ---------------------------------------------------------------------

// has_name_method
//   trait: true if _Type exposes a `name()` const method (pugixml /
// short-form convention).
template<typename _Type,
         typename = void>
struct has_name_method : std::false_type
{};

template<typename _Type>
struct has_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().name())
>> : std::true_type
{};


// has_get_name_method
//   trait: true if _Type exposes a `get_name()` const method (libxml++
// / getter-form convention).
template<typename _Type,
         typename = void>
struct has_get_name_method : std::false_type
{};

template<typename _Type>
struct has_get_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_name())
>> : std::true_type
{};


// has_name_access
//   trait: true if _Type exposes a name accessor in either form.
template<typename _Type>
struct has_name_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_name_method<_Type>::value ||
          has_get_name_method<_Type>::value );
};


// ---------------------------------------------------------------------
//  value accessor
// ---------------------------------------------------------------------

// has_value_method
//   trait: true if _Type exposes a `value()` const method.
template<typename _Type,
         typename = void>
struct has_value_method : std::false_type
{};

template<typename _Type>
struct has_value_method<_Type, void_t<
    decltype(std::declval<const _Type&>().value())
>> : std::true_type
{};


// has_get_value_method
//   trait: true if _Type exposes a `get_value()` const method.
template<typename _Type,
         typename = void>
struct has_get_value_method : std::false_type
{};

template<typename _Type>
struct has_get_value_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_value())
>> : std::true_type
{};


// has_value_access
//   trait: true if _Type exposes a value accessor in either form.
template<typename _Type>
struct has_value_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_value_method<_Type>::value ||
          has_get_value_method<_Type>::value );
};


// ---------------------------------------------------------------------
//  kind accessor
// ---------------------------------------------------------------------

// has_kind_method
//   trait: true if _Type exposes a `kind()` const method returning a
// node-kind discriminator.
template<typename _Type,
         typename = void>
struct has_kind_method : std::false_type
{};

template<typename _Type>
struct has_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().kind())
>> : std::true_type
{};


// has_get_kind_method
//   trait: true if _Type exposes a `get_kind()` or `node_type()` const
// method. Detection covers `node_type()` because libxml++ exposes the
// kind under that name.
template<typename _Type,
         typename = void>
struct has_get_kind_method : std::false_type
{};

template<typename _Type>
struct has_get_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().node_type())
>> : std::true_type
{};


// has_kind_access
//   trait: true if _Type exposes a kind accessor in either form.
template<typename _Type>
struct has_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_kind_method<_Type>::value ||
          has_get_kind_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                II.   ATTRIBUTE DETECTION                                ///
///////////////////////////////////////////////////////////////////////////////

// has_attributes_method
//   trait: true if _Type exposes an `attributes()` const method
// returning an iterable.
template<typename _Type,
         typename = void>
struct has_attributes_method : std::false_type
{};

template<typename _Type>
struct has_attributes_method<_Type, void_t<
    decltype(std::declval<const _Type&>().attributes())
>> : std::true_type
{};


// has_get_attributes_method
//   trait: true if _Type exposes `get_attributes()` (libxml++).
template<typename _Type,
         typename = void>
struct has_get_attributes_method : std::false_type
{};

template<typename _Type>
struct has_get_attributes_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_attributes())
>> : std::true_type
{};


// has_find_attribute_method
//   trait: true if _Type exposes `find_attribute(name)` returning
// either an attribute pointer or an iterator-like value.
template<typename _Type,
         typename = void>
struct has_find_attribute_method : std::false_type
{};

template<typename _Type>
struct has_find_attribute_method<_Type, void_t<
    decltype(std::declval<const _Type&>().find_attribute(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_set_attribute_method
//   trait: true if _Type exposes `set_attribute(name, value)`.
template<typename _Type,
         typename = void>
struct has_set_attribute_method : std::false_type
{};

template<typename _Type>
struct has_set_attribute_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_attribute(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_remove_attribute_method
//   trait: true if _Type exposes `remove_attribute(name)`.
template<typename _Type,
         typename = void>
struct has_remove_attribute_method : std::false_type
{};

template<typename _Type>
struct has_remove_attribute_method<_Type, void_t<
    decltype(std::declval<_Type&>().remove_attribute(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_attribute_count_method
//   trait: true if _Type exposes `attribute_count()` const.
template<typename _Type,
         typename = void>
struct has_attribute_count_method : std::false_type
{};

template<typename _Type>
struct has_attribute_count_method<_Type, void_t<
    decltype(std::declval<const _Type&>().attribute_count())
>> : std::true_type
{};


// is_xml_attribute
//   trait: minimum protocol -- a name accessor and a value accessor.
// This keeps the bar low so libxml++'s `Attribute`, pugixml's
// `xml_attribute`, and our own `xml_attribute` all qualify.
template<typename _Type>
struct is_xml_attribute
{
    D_STATIC_CONSTEXPR bool value =
        ( has_name_access<_Type>::value &&
          has_value_access<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///             III.   CHILD / TRAVERSAL DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// has_children_method
//   trait: true if _Type exposes `children()` const returning an
// iterable.
template<typename _Type,
         typename = void>
struct has_children_method : std::false_type
{};

template<typename _Type>
struct has_children_method<_Type, void_t<
    decltype(std::declval<const _Type&>().children())
>> : std::true_type
{};


// has_get_children_method
//   trait: true if _Type exposes `get_children()` (libxml++).
template<typename _Type,
         typename = void>
struct has_get_children_method : std::false_type
{};

template<typename _Type>
struct has_get_children_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_children())
>> : std::true_type
{};


// has_first_child_method
//   trait: true if _Type exposes `first_child()` (pugixml /
// linked-list traversal style).
template<typename _Type,
         typename = void>
struct has_first_child_method : std::false_type
{};

template<typename _Type>
struct has_first_child_method<_Type, void_t<
    decltype(std::declval<const _Type&>().first_child())
>> : std::true_type
{};


// has_parent_method
//   trait: true if _Type exposes `parent()` const.
template<typename _Type,
         typename = void>
struct has_parent_method : std::false_type
{};

template<typename _Type>
struct has_parent_method<_Type, void_t<
    decltype(std::declval<const _Type&>().parent())
>> : std::true_type
{};


// has_find_child_method
//   trait: true if _Type exposes `find_child(name)`.
template<typename _Type,
         typename = void>
struct has_find_child_method : std::false_type
{};

template<typename _Type>
struct has_find_child_method<_Type, void_t<
    decltype(std::declval<const _Type&>().find_child(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_add_child_method
//   trait: true if _Type exposes `add_child(name)`.
template<typename _Type,
         typename = void>
struct has_add_child_method : std::false_type
{};

template<typename _Type>
struct has_add_child_method<_Type, void_t<
    decltype(std::declval<_Type&>().add_child(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_remove_child_method
//   trait: true if _Type exposes `remove_child(name)`.
template<typename _Type,
         typename = void>
struct has_remove_child_method : std::false_type
{};

template<typename _Type>
struct has_remove_child_method<_Type, void_t<
    decltype(std::declval<_Type&>().remove_child(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_child_count_method
//   trait: true if _Type exposes `child_count()` const.
template<typename _Type,
         typename = void>
struct has_child_count_method : std::false_type
{};

template<typename _Type>
struct has_child_count_method<_Type, void_t<
    decltype(std::declval<const _Type&>().child_count())
>> : std::true_type
{};


// has_traversal
//   trait: true if _Type exposes ANY child-enumeration accessor
// (children/get_children/first_child).
template<typename _Type>
struct has_traversal
{
    D_STATIC_CONSTEXPR bool value =
        ( has_children_method<_Type>::value     ||
          has_get_children_method<_Type>::value ||
          has_first_child_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   TEXT CONTENT DETECTION                             ///
///////////////////////////////////////////////////////////////////////////////

// has_text_method
//   trait: true if _Type exposes `text()` returning the
// concatenated text content of its descendants.
template<typename _Type,
         typename = void>
struct has_text_method : std::false_type
{};

template<typename _Type>
struct has_text_method<_Type, void_t<
    decltype(std::declval<const _Type&>().text())
>> : std::true_type
{};


// has_get_text_method
//   trait: true if _Type exposes `get_text()`.
template<typename _Type,
         typename = void>
struct has_get_text_method : std::false_type
{};

template<typename _Type>
struct has_get_text_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_text())
>> : std::true_type
{};


// has_set_text_method
//   trait: true if _Type exposes `set_text(text)`.
template<typename _Type,
         typename = void>
struct has_set_text_method : std::false_type
{};

template<typename _Type>
struct has_set_text_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_text(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_text_access
//   trait: true if _Type exposes a text accessor in either form.
template<typename _Type>
struct has_text_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_text_method<_Type>::value ||
          has_get_text_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                V.   NODE CLASSIFICATION                                 ///
///////////////////////////////////////////////////////////////////////////////

// is_xml_node
//   trait: minimum node protocol -- exposes a name accessor and
// some form of child traversal. Deliberately permissive so that
// pugixml, libxml++, tinyxml2, and our default backend all qualify.
template<typename _Type>
struct is_xml_node
{
    D_STATIC_CONSTEXPR bool value =
        ( has_name_access<_Type>::value &&
          has_traversal<_Type>::value );
};


// is_xml_element
//   trait: full element-node protocol -- node + attributes + text
// access. An element is the concrete tag type that carries
// attributes and child content.
template<typename _Type>
struct is_xml_element
{
    D_STATIC_CONSTEXPR bool value =
        ( is_xml_node<_Type>::value &&
          ( has_attributes_method<_Type>::value     ||
            has_get_attributes_method<_Type>::value ) );
};


// xml_node_class
//   struct: comprehensive classification of an XML-node-shaped
// type. Aggregates every detection trait into one read-out so
// generic code can branch on capabilities cheaply.
template<typename _Type>
struct xml_node_class
{
    // identity
    D_STATIC_CONSTEXPR bool is_node       =
        is_xml_node<_Type>::value;
    D_STATIC_CONSTEXPR bool is_element    =
        is_xml_element<_Type>::value;

    // accessors
    D_STATIC_CONSTEXPR bool has_name      =
        has_name_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_value     =
        has_value_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_kind      =
        has_kind_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_text      =
        has_text_access<_Type>::value;

    // attributes
    D_STATIC_CONSTEXPR bool has_attrs     =
        ( has_attributes_method<_Type>::value ||
          has_get_attributes_method<_Type>::value );
    D_STATIC_CONSTEXPR bool has_find_attr =
        has_find_attribute_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_set_attr  =
        has_set_attribute_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_attr_count =
        has_attribute_count_method<_Type>::value;

    // traversal
    D_STATIC_CONSTEXPR bool has_kids      =
        has_traversal<_Type>::value;
    D_STATIC_CONSTEXPR bool has_first_kid =
        has_first_child_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_find_kid  =
        has_find_child_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_add_kid   =
        has_add_child_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_kid_count =
        has_child_count_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_parent    =
        has_parent_method<_Type>::value;

    // mutability
    D_STATIC_CONSTEXPR bool is_mutable    =
        ( has_set_attr     ||
          has_add_kid      ||
          has_set_text_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   DOCUMENT DETECTION                                 ///
///////////////////////////////////////////////////////////////////////////////

// has_root_element_method
//   trait: true if _Type exposes `root_element()` const.
template<typename _Type,
         typename = void>
struct has_root_element_method : std::false_type
{};

template<typename _Type>
struct has_root_element_method<_Type, void_t<
    decltype(std::declval<const _Type&>().root_element())
>> : std::true_type
{};


// has_document_element_method
//   trait: true if _Type exposes `document_element()` (DOM-style).
template<typename _Type,
         typename = void>
struct has_document_element_method : std::false_type
{};

template<typename _Type>
struct has_document_element_method<_Type, void_t<
    decltype(std::declval<const _Type&>().document_element())
>> : std::true_type
{};


// has_root_access
//   trait: true if _Type exposes a root-element accessor in
// either form.
template<typename _Type>
struct has_root_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_root_element_method<_Type>::value ||
          has_document_element_method<_Type>::value );
};


// has_version_method
//   trait: true if _Type exposes `version()` const.
template<typename _Type,
         typename = void>
struct has_version_method : std::false_type
{};

template<typename _Type>
struct has_version_method<_Type, void_t<
    decltype(std::declval<const _Type&>().version())
>> : std::true_type
{};


// has_encoding_method
//   trait: true if _Type exposes `encoding()` const.
template<typename _Type,
         typename = void>
struct has_encoding_method : std::false_type
{};

template<typename _Type>
struct has_encoding_method<_Type, void_t<
    decltype(std::declval<const _Type&>().encoding())
>> : std::true_type
{};


// has_standalone_method
//   trait: true if _Type exposes `standalone()` const.
template<typename _Type,
         typename = void>
struct has_standalone_method : std::false_type
{};

template<typename _Type>
struct has_standalone_method<_Type, void_t<
    decltype(std::declval<const _Type&>().standalone())
>> : std::true_type
{};


// has_parse_method
//   trait: true if _Type exposes `parse(const std::string&)`. Used
// to detect documents that can ingest serialised XML in-place.
template<typename _Type,
         typename = void>
struct has_parse_method : std::false_type
{};

template<typename _Type>
struct has_parse_method<_Type, void_t<
    decltype(std::declval<_Type&>().parse(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_write_method
//   trait: true if _Type exposes `write()` returning a string-
// convertible serialisation.
template<typename _Type,
         typename = void>
struct has_write_method : std::false_type
{};

template<typename _Type>
struct has_write_method<_Type, void_t<
    decltype(std::declval<const _Type&>().write())
>> : std::true_type
{};


// has_save_method
//   trait: true if _Type exposes `save(const std::string&)` taking
// a file path.
template<typename _Type,
         typename = void>
struct has_save_method : std::false_type
{};

template<typename _Type>
struct has_save_method<_Type, void_t<
    decltype(std::declval<const _Type&>().save(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// is_xml_document
//   trait: minimum document protocol -- exposes a root-element
// accessor in some form.
template<typename _Type>
struct is_xml_document
{
    D_STATIC_CONSTEXPR bool value =
        has_root_access<_Type>::value;
};


// xml_document_class
//   struct: comprehensive classification of an XML-document-shaped
// type. Aggregates every detection trait into one read-out.
template<typename _Type>
struct xml_document_class
{
    // identity
    D_STATIC_CONSTEXPR bool is_document    =
        is_xml_document<_Type>::value;

    // root access
    D_STATIC_CONSTEXPR bool has_root       =
        has_root_access<_Type>::value;

    // prolog accessors
    D_STATIC_CONSTEXPR bool has_version    =
        has_version_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_encoding   =
        has_encoding_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_standalone =
        has_standalone_method<_Type>::value;

    // I/O
    D_STATIC_CONSTEXPR bool can_parse      =
        has_parse_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_write      =
        has_write_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_save       =
        has_save_method<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VII.   BACKEND DETECTION                                 ///
///////////////////////////////////////////////////////////////////////////////

// has_node_type_alias
//   trait: true if _Type exposes a nested `node_type` alias. A
// backend's `node_type` names the concrete node class for that
// backend.
template<typename _Type,
         typename = void>
struct has_node_type_alias : std::false_type
{};

template<typename _Type>
struct has_node_type_alias<_Type, void_t<
    typename _Type::node_type
>> : std::true_type
{};


// has_attribute_type_alias
//   trait: true if _Type exposes a nested `attribute_type` alias.
template<typename _Type,
         typename = void>
struct has_attribute_type_alias : std::false_type
{};

template<typename _Type>
struct has_attribute_type_alias<_Type, void_t<
    typename _Type::attribute_type
>> : std::true_type
{};


// has_document_type_alias
//   trait: true if _Type exposes a nested `document_type` alias.
template<typename _Type,
         typename = void>
struct has_document_type_alias : std::false_type
{};

template<typename _Type>
struct has_document_type_alias<_Type, void_t<
    typename _Type::document_type
>> : std::true_type
{};


// has_make_document_method
//   trait: true if _Type exposes a static or member factory
// `make_document()` returning a document_type instance.
template<typename _Type,
         typename = void>
struct has_make_document_method : std::false_type
{};

template<typename _Type>
struct has_make_document_method<_Type, void_t<
    decltype(_Type::make_document())
>> : std::true_type
{};


// is_xml_backend_complete
//   trait: true if _Type exposes the full backend protocol --
// every nested type alias and a make_document factory. Stronger
// than `is_xml_backend` (which only checks for the backend_tag).
template<typename _Type>
struct is_xml_backend_complete
{
    D_STATIC_CONSTEXPR bool value =
        ( has_node_type_alias<_Type>::value      &&
          has_attribute_type_alias<_Type>::value &&
          has_document_type_alias<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VIII.  VARIABLE TEMPLATES                                ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool has_name_access_v =
        has_name_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_value_access_v =
        has_value_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_kind_access_v =
        has_kind_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_text_access_v =
        has_text_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_traversal_v =
        has_traversal<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_root_access_v =
        has_root_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_xml_attribute_v =
        is_xml_attribute<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_xml_node_v =
        is_xml_node<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_xml_element_v =
        is_xml_element<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_xml_document_v =
        is_xml_document<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_xml_backend_complete_v =
        is_xml_backend_complete<_Type>::value;

#endif  // variable templates


NS_END  // djinterp


#endif  // DJINTERP_XML_TEMPLATE_TRAITS_
