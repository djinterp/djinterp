/******************************************************************************
* djinterp [text]                                              xml_dialect.hpp
*
*   Compile-time policy template that captures an XML dialect /
* profile via a single enum-parameterized template. Properties of the
* dialect (whether to emit the XML declaration, whether to sort
* attributes for canonicalization, whether to allow XML 1.1 character
* set, etc.) are exposed as `D_STATIC_CONSTEXPR` constants and
* `static D_CONSTEXPR` functions whose values fold at compile time
* when the template is instantiated -- zero runtime cost.
*
*   The enum `xml_dialect_flag` is the canonical, greppable list of
* every recognized XML profile. The template `xml_dialect<_Flag>`
* derives every behavioural property from that single parameter via
* internal switch expressions (for the small properties) and
* delegations to free helpers in `internal::` (for the larger
* validation tables).
*
*   ZERO OVERHEAD:
*   When the template is instantiated for a fixed flag, every
* property collapses to a constant the optimiser inlines. No vtable,
* no member, no runtime dispatch.
*
*   EXTENSIBILITY:
*   Third parties wanting a custom dialect can either use the
* `xml_dialect_flag::custom` flag (and intercept it in the helpers),
* OR define their own struct exposing the same `D_STATIC_CONSTEXPR`
* surface and pass it wherever an `xml_dialect<_Flag>` would be
* accepted. The trait `is_xml_dialect<_Type>` recognizes both forms.
*
*
* path:      /inc/djinterp/core/util/text/xml_dialect.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.09
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    XML DIALECT FLAG
II.   INTERNAL CLASSIFICATION HELPERS
III.  INTERNAL VALIDATION HELPERS
IV.   xml_dialect<_Flag>
V.    CONVENIENCE ALIASES
VI.   DIALECT DETECTION TRAITS
*/

#ifndef DJINTERP_XML_DIALECT_
#define DJINTERP_XML_DIALECT_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "./xml.hpp"


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///                I.   XML DIALECT FLAG                                    ///
///////////////////////////////////////////////////////////////////////////////

// xml_dialect_flag
//   enum: enumerates every recognized XML profile / dialect.
// Backed by `std::uint8_t`. Adding a new dialect means appending
// a value here and updating the small set of internal helpers
// below; no new specialization or source file is needed.
enum class xml_dialect_flag : std::uint8_t
{
    xml_1_0,                    // standard XML 1.0
    xml_1_1,                    // XML 1.1 (relaxed character rules)
    canonical_1_0,              // C14N 1.0
    canonical_1_1,              // C14N 1.1
    canonical_exclusive_1_0,    // exclusive C14N 1.0
    soap_envelope,              // SOAP 1.2 envelope variant
    xhtml_compatible,           // XML output also valid as XHTML
    svg,                        // SVG 1.1 / 2 markup variant
    xslt,                       // XSLT stylesheet variant
    custom,                     // user-defined; helpers fall through
    unspecified                 // unknown; runtime-only
};


///////////////////////////////////////////////////////////////////////////////
///                II.   INTERNAL CLASSIFICATION HELPERS                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_canonical_xml_dialect_helper
    //   function: true if `_flag` is one of the canonical-XML
    // profiles. Several dialect properties (sort attrs, normalize
    // whitespace, omit declaration) all key off this; centralising
    // it avoids drift between them.
    D_CONSTEXPR_INLINE bool
    is_canonical_xml_dialect_helper(
        xml_dialect_flag    _flag
    )
    {
        return ( (_flag == xml_dialect_flag::canonical_1_0)           ||
                 (_flag == xml_dialect_flag::canonical_1_1)           ||
                 (_flag == xml_dialect_flag::canonical_exclusive_1_0) );
    }


    // xml_dialect_default_xml_version_helper
    //   function: returns the version string emitted in the XML
    // declaration (`<?xml version="X.Y" ?>`) for a given dialect.
    D_CONSTEXPR_INLINE const char*
    xml_dialect_default_xml_version_helper(
        xml_dialect_flag    _flag
    )
    {
        return (_flag == xml_dialect_flag::xml_1_1)
                ? "1.1"
             :    "1.0";
    }


    // xml_dialect_default_encoding_helper
    //   function: returns the default encoding for a given
    // dialect. C14N mandates UTF-8; everything else defaults to
    // UTF-8 too but allows override at runtime.
    D_CONSTEXPR_INLINE xml_encoding
    xml_dialect_default_encoding_helper(
        xml_dialect_flag    _flag
    )
    {
        (void)_flag;
        return xml_encoding::utf_8;
    }


    // xml_dialect_default_namespace_uri_helper
    //   function: returns the canonical namespace URI for
    // dialects that have one (SVG, XSLT, XHTML). Returns the
    // empty string for dialects with no fixed namespace.
    D_CONSTEXPR_INLINE const char*
    xml_dialect_default_namespace_uri_helper(
        xml_dialect_flag    _flag
    )
    {
        return (_flag == xml_dialect_flag::svg)
                ? "http://www.w3.org/2000/svg"
             : (_flag == xml_dialect_flag::xslt)
                ? "http://www.w3.org/1999/XSL/Transform"
             : (_flag == xml_dialect_flag::xhtml_compatible)
                ? "http://www.w3.org/1999/xhtml"
             : (_flag == xml_dialect_flag::soap_envelope)
                ? "http://www.w3.org/2003/05/soap-envelope"
             :    "";
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                III.   INTERNAL VALIDATION HELPERS                       ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // xml_dialect_is_name_start_char_helper
    //   function: validates a character against the XML 1.0
    // NameStartChar production (subset: ASCII + '_').
    D_CONSTEXPR_INLINE bool
    xml_dialect_is_name_start_char_helper(
        char    _c
    )
    {
        return ( ((_c >= 'A') && (_c <= 'Z')) ||
                 ((_c >= 'a') && (_c <= 'z')) ||
                 (_c == '_') );
    }


    // xml_dialect_is_name_char_helper
    //   function: validates a character against the XML 1.0
    // NameChar production (subset: ASCII).
    D_CONSTEXPR_INLINE bool
    xml_dialect_is_name_char_helper(
        char    _c
    )
    {
        return ( xml_dialect_is_name_start_char_helper(_c) ||
                 ((_c >= '0') && (_c <= '9'))              ||
                 (_c == '-') || (_c == '.') || (_c == ':') );
    }


    // xml_dialect_is_valid_element_name_helper
    //   function: per-dialect element-name validation. Handles
    // the syntactic tier only -- schema validation is a separate
    // concern. Walks the name iteratively rather than
    // recursively to keep stack usage bounded for large names.
    D_CONSTEXPR_INLINE bool
    xml_dialect_is_valid_element_name_helper(
        xml_dialect_flag    _flag,
        const char*         _name
    )
    {
        // null / empty rejected
        if ((_name == nullptr) || (_name[0] == '\0'))
        {
            return false;
        }

        // first character must be NameStartChar
        if (!xml_dialect_is_name_start_char_helper(_name[0]))
        {
            return false;
        }

        // every subsequent char must be NameChar
        std::size_t i = 1;
        while (_name[i] != '\0')
        {
            if (!xml_dialect_is_name_char_helper(_name[i]))
            {
                return false;
            }
            ++i;
        }

        // dialect-specific restrictions
        // (xhtml requires lowercase but the character set is the
        // same; that check belongs in the emission path, not the
        // syntactic name validator.)
        (void)_flag;
        return true;
    }


    // xml_dialect_is_valid_attribute_name_helper
    //   function: per-dialect attribute-name validation. Same
    // syntactic rules as element names; some dialects (xmlns,
    // xml:* reserved prefixes) layer on additional rules.
    D_CONSTEXPR_INLINE bool
    xml_dialect_is_valid_attribute_name_helper(
        xml_dialect_flag    _flag,
        const char*         _name
    )
    {
        return xml_dialect_is_valid_element_name_helper(_flag, _name);
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                IV.   xml_dialect<_Flag>                                 ///
///////////////////////////////////////////////////////////////////////////////

// xml_dialect
//   struct: compile-time policy capturing the rules of an XML
// dialect / profile. Every property is `D_STATIC_CONSTEXPR`;
// instantiating this template for a specific flag produces a
// type whose properties fold to constants at compile time.
// Use the convenience aliases below for the common dialects.
template<xml_dialect_flag    _Flag>
struct xml_dialect
{
    /// identity

    // flag
    //   constant: the dialect flag this instantiation represents.
    // Lets callers pattern-match at compile time via if-constexpr
    // or static_assert.
    D_STATIC_CONSTEXPR xml_dialect_flag flag = _Flag;


    /// version & encoding

    // xml_version_string
    //   function: returns the version string emitted in the XML
    // declaration.
    static D_CONSTEXPR const char*
    xml_version_string()
    {
        return internal::xml_dialect_default_xml_version_helper(_Flag);
    }

    // default_encoding
    //   function: returns the default encoding for this dialect.
    static D_CONSTEXPR xml_encoding
    default_encoding()
    {
        return internal::xml_dialect_default_encoding_helper(_Flag);
    }

    // default_namespace_uri
    //   function: returns the canonical namespace URI for
    // dialects that have one (SVG, XSLT, XHTML, SOAP). Returns
    // the empty string otherwise.
    static D_CONSTEXPR const char*
    default_namespace_uri()
    {
        return internal::xml_dialect_default_namespace_uri_helper(_Flag);
    }


    /// emission rules

    // emit_xml_declaration
    //   constant: whether to emit `<?xml ... ?>` at the document
    // start. Canonical XML profiles omit it.
    D_STATIC_CONSTEXPR bool emit_xml_declaration = (
        ! internal::is_canonical_xml_dialect_helper(_Flag)
    );

    // emit_standalone_attribute
    //   constant: whether the standalone="..." pseudo-attribute
    // is emitted in the XML declaration. C14N forbids it.
    D_STATIC_CONSTEXPR bool emit_standalone_attribute = (
        ! internal::is_canonical_xml_dialect_helper(_Flag)
    );

    // self_close_empty
    //   constant: whether to self-close empty elements as `<x/>`.
    // True for all standard XML.
    D_STATIC_CONSTEXPR bool self_close_empty = true;

    // require_quoted_attrs
    //   constant: whether attribute values must be quoted.
    // Always true for XML.
    D_STATIC_CONSTEXPR bool require_quoted_attrs = true;

    // attr_quote_char
    //   constant: the quote character used for attribute values.
    // C14N mandates double quotes; other dialects allow either
    // and we pick the standard double quote for predictability.
    D_STATIC_CONSTEXPR char attr_quote_char = '"';

    // require_lowercase_tags
    //   constant: whether tag names must be lowercase. XHTML
    // requires this; pure XML is case-sensitive but does not
    // mandate a particular case.
    D_STATIC_CONSTEXPR bool require_lowercase_tags = (
        (_Flag == xml_dialect_flag::xhtml_compatible)
    );

    // emit_bom
    //   constant: whether to prepend a UTF-8 byte-order mark.
    D_STATIC_CONSTEXPR bool emit_bom = false;


    /// canonicalization rules

    // sort_attributes
    //   constant: whether attributes must be emitted in lexical
    // order. Required by C14N profiles.
    D_STATIC_CONSTEXPR bool sort_attributes = (
        internal::is_canonical_xml_dialect_helper(_Flag)
    );

    // normalize_line_endings
    //   constant: whether to normalize all line endings to LF
    // before emission. Required by C14N.
    D_STATIC_CONSTEXPR bool normalize_line_endings = (
        internal::is_canonical_xml_dialect_helper(_Flag)
    );

    // normalize_whitespace_in_attrs
    //   constant: whether to collapse whitespace in non-CDATA
    // attribute values. Required by C14N.
    D_STATIC_CONSTEXPR bool normalize_whitespace_in_attrs = (
        internal::is_canonical_xml_dialect_helper(_Flag)
    );

    // emit_namespace_declarations
    //   constant: whether xmlns attributes are required on
    // namespaced elements.
    D_STATIC_CONSTEXPR bool emit_namespace_declarations = (
           (_Flag != xml_dialect_flag::custom)
        && (_Flag != xml_dialect_flag::unspecified)
    );


    /// character set rules

    // allow_xml_1_1_chars
    //   constant: whether the broader XML 1.1 character class
    // is allowed (NEL line endings, expanded restricted-char
    // handling).
    D_STATIC_CONSTEXPR bool allow_xml_1_1_chars = (
        (_Flag == xml_dialect_flag::xml_1_1)
    );

    // allow_dtd_subset
    //   constant: whether internal DTD subsets are allowed.
    // Canonical XML profiles forbid them.
    D_STATIC_CONSTEXPR bool allow_dtd_subset = (
        ! internal::is_canonical_xml_dialect_helper(_Flag)
    );

    // allow_processing_instructions
    //   constant: whether <?...?> processing instructions are
    // allowed in document content. Canonical exclusive profiles
    // restrict their handling but do not forbid emission.
    D_STATIC_CONSTEXPR bool allow_processing_instructions = true;


    /// validation predicates (delegate to internal helpers for
    /// the larger tables)

    // is_valid_element_name
    //   function: validates the syntactic form of an element
    // name under this dialect.
    static D_CONSTEXPR bool
    is_valid_element_name(
        const char*     _name
    )
    {
        return internal::xml_dialect_is_valid_element_name_helper(
            _Flag, _name);
    }

    // is_valid_attribute_name
    //   function: validates the syntactic form of an attribute
    // name under this dialect.
    static D_CONSTEXPR bool
    is_valid_attribute_name(
        const char*     _name
    )
    {
        return internal::xml_dialect_is_valid_attribute_name_helper(
            _Flag, _name);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                V.   CONVENIENCE ALIASES                                 ///
///////////////////////////////////////////////////////////////////////////////

// canonical names for the standard dialects. Use these where
// readability matters; both forms are interchangeable since
// every alias is just `xml_dialect<...>`.
using xml_dialect_1_0                = xml_dialect<xml_dialect_flag::xml_1_0>;
using xml_dialect_1_1                = xml_dialect<xml_dialect_flag::xml_1_1>;
using xml_dialect_canonical_1_0      = xml_dialect<xml_dialect_flag::canonical_1_0>;
using xml_dialect_canonical_1_1      = xml_dialect<xml_dialect_flag::canonical_1_1>;
using xml_dialect_canonical_excl_1_0 = xml_dialect<xml_dialect_flag::canonical_exclusive_1_0>;
using xml_dialect_soap               = xml_dialect<xml_dialect_flag::soap_envelope>;
using xml_dialect_xhtml              = xml_dialect<xml_dialect_flag::xhtml_compatible>;
using xml_dialect_svg                = xml_dialect<xml_dialect_flag::svg>;
using xml_dialect_xslt               = xml_dialect<xml_dialect_flag::xslt>;


///////////////////////////////////////////////////////////////////////////////
///                VI.   DIALECT DETECTION TRAITS                           ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // has_xml_dialect_flag_helper
    //   trait: SFINAE detector for `_Type::flag` of type
    // `xml_dialect_flag`. Primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_xml_dialect_flag_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_xml_dialect_flag_helper (specialization)
    //   trait: success case when `_Type::flag` exists and is of
    // type `xml_dialect_flag`.
    template<typename _Type>
    struct has_xml_dialect_flag_helper<
        _Type,
        typename std::enable_if<
            std::is_same<
                clean_t<decltype(_Type::flag)>,
                xml_dialect_flag
            >::value
        >::type
    >
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal


// is_xml_dialect
//   trait: true if `_Type` exposes the XML dialect surface --
// specifically a `flag` member of type `xml_dialect_flag`.
// Detects both `xml_dialect<_Flag>` instantiations and
// third-party policy structs that follow the same convention.
template<typename    _Type>
struct is_xml_dialect
{
    D_STATIC_CONSTEXPR bool value =
        internal::has_xml_dialect_flag_helper<clean_t<_Type>>::value;
};


// is_xml_dialect_v
//   constant: convenience accessor for is_xml_dialect<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_STATIC_CONSTEXPR bool is_xml_dialect_v =
        is_xml_dialect<_Type>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_XML_DIALECT_
