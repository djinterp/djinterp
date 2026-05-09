/******************************************************************************
* djinterp [text]                                                      xml.hpp
*
*   Foundational XML module for the djinterp framework. Provides shared
* types, node-type enumerations, encoding identifiers, backend tag
* dispatch, and the structural-detection trait `is_xml_backend`.
* Sub-module headers (templated node/document types, traits, concepts)
* are included at the bottom.
*
*   LIBRARY AGNOSTICISM:
*   This module does NOT include or depend on any third-party XML
* library (libxml++, pugixml, tinyxml2, RapidXML, etc.). All
* interaction with such libraries is by structural duck-typing through
* the trait system in `xml_template_traits.hpp`. Adapters for specific
* libraries live in their own headers (e.g. `xml_libxmlpp_adapter.hpp`)
* and are NOT pulled in by this header.
*
*   BACKEND MODEL:
*   `xml_node` and `xml_document` are templated over a `_Backend` type
* parameter. The backend is any type that satisfies the structural
* protocol detected by `is_xml_backend`. A default in-memory backend
* (`xml_default_backend`) is provided in `xml_template.hpp`.
*
*
* path:      /inc/djinterp/core/text/xml/xml.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.08
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TYPES & CONSTANTS
      --------------------------
      i.    xml_string_t
      ii.   xml_size_t
      iii.  D_XML_DEFAULT_VERSION
      iv.   D_XML_DEFAULT_ENCODING
      v.    D_XML_DEFAULT_INDENT

II.   CORE ENUMERATIONS
      ------------------
      a. xml_node_kind
      b. xml_encoding
      c. xml_standalone

III.  BACKEND TAG DISPATCH
      ---------------------
      a. xml_backend_tag
      b. xml_default_backend_tag
      c. xml_libxmlpp_backend_tag
      d. xml_pugixml_backend_tag
      e. xml_tinyxml2_backend_tag
      f. xml_rapidxml_backend_tag

IV.   BACKEND DETECTION
      ------------------
      i.    has_backend_tag_helper (internal)
      ii.   is_xml_backend
            a. is_xml_backend_v

V.    SUB-MODULE INCLUDES
      ---------------------
      a. xml_template_traits.hpp
      b. xml_template.hpp
      c. xml_template_concepts.hpp
*/

#ifndef DJINTERP_XML_
#define DJINTERP_XML_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
// djinterp
#include "../../../djinterp.hpp"


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
enum class xml_node_kind : std::uint8_t
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
enum class xml_encoding : std::uint8_t
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
enum class xml_standalone : std::uint8_t
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
///                  IV.   BACKEND DETECTION                                ///
///////////////////////////////////////////////////////////////////////////////

// is_xml_backend
//   trait: detects whether _Type is an XML backend type by
// checking for a nested `backend_tag` alias.

NS_INTERNAL

    // has_backend_tag_helper
    //   trait: SFINAE helper; primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_backend_tag_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_backend_tag_helper (specialization)
    //   trait: success case when _Type::backend_tag exists.
    template<typename _Type>
    struct has_backend_tag_helper<_Type,
                                  void_t<typename _Type::backend_tag>>
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal

// is_xml_backend
//   trait: true if _Type has a nested backend_tag type derived
// from xml_backend_tag.
template<typename _Type>
struct is_xml_backend
{
    D_STATIC_CONSTEXPR bool value =
        internal::has_backend_tag_helper<clean_t<_Type>>::value;
};

// is_xml_backend_v
//   constant: convenience accessor for
// is_xml_backend<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_STATIC_CONSTEXPR bool is_xml_backend_v =
        is_xml_backend<_Type>::value;
#endif


NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                  V.   SUB-MODULE INCLUDES                               ///
///////////////////////////////////////////////////////////////////////////////

#include "./xml_template_traits.hpp"
#include "./xml_template.hpp"
#include "./xml_template_concepts.hpp"


#endif  // DJINTERP_XML_
