/*******************************************************************************
* djinterp [cpp][text]                                      xml_tests_traits.cpp
*
*   The groups for xml/xml.hpp and xml/xml_dialect.hpp. xml.hpp classifies
* types by what they expose, so most of its groups are probes: small
* structs shaped like pugixml's, libxml++'s or a DOM's types, that xml.hpp
* has never heard of. Their members are declared and defined only so that
* the traits, which look at types, have something to look at; nothing here
* calls them.
*
*
* path:      /tests/djinterp/cpp/text/xml_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "xml_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <cstddef>  // NULL, std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::is_base_of, std::is_same
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/xml/xml.hpp"  // traits, vocabulary
#include "../../../../inc/djinterp/cpp/text/xml/xml_dialect.hpp"  // dialects


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

//==============================================================================
// PROBES
//==============================================================================

// DTestsXmlShortAttribute
//   probe: a pugixml-style attribute, name() and value().
struct DTestsXmlShortAttribute
{
    std::string name() const  { return std::string("n"); }
    std::string value() const { return std::string("v"); }
};

// DTestsXmlGetterAttribute
//   probe: a libxml++-style attribute, get_name() and get_value().
struct DTestsXmlGetterAttribute
{
    std::string get_name() const  { return std::string("n"); }
    std::string get_value() const { return std::string("v"); }
};

// DTestsXmlNameOnly
//   probe: a name and nothing else.
struct DTestsXmlNameOnly
{
    std::string name() const { return std::string("n"); }
};

// DTestsXmlMutableName
//   probe: a name that isn't const, which the traits don't count.
struct DTestsXmlMutableName
{
    std::string name() { return std::string("n"); }
};

// DTestsXmlFullNode
//   probe: every short-form member of the node protocol, and its setters.
struct DTestsXmlFullNode
{
    std::string      name() const       { return std::string(); }
    std::string      value() const      { return std::string(); }
    int              kind() const       { return 1; }
    std::string      text() const       { return std::string(); }
    std::vector<int> attributes() const { return std::vector<int>(); }
    std::vector<int> children() const   { return std::vector<int>(); }
    int              first_child() const    { return 0; }
    int              parent() const         { return 0; }
    int              attribute_count() const { return 0; }
    int              child_count() const    { return 0; }

    int  find_attribute(const std::string&) const { return 0; }
    int  find_child(const std::string&) const     { return 0; }
    bool set_attribute(const std::string&, const std::string&) { return true; }
    bool remove_attribute(const std::string&)     { return true; }
    int  add_child(const std::string&)            { return 0; }
    bool remove_child(const std::string&)         { return true; }
    void set_text(const std::string&)             {}
};

// DTestsXmlGetterNode
//   probe: a libxml++-style node: getters, and the kind as node_type().
struct DTestsXmlGetterNode
{
    std::string      get_name() const       { return std::string(); }
    std::vector<int> get_children() const   { return std::vector<int>(); }
    std::vector<int> get_attributes() const { return std::vector<int>(); }
    int              node_type() const      { return 1; }
    std::string      get_text() const       { return std::string(); }
};

// DTestsXmlGetKindNode
//   probe: a node whose kind is get_kind(), the getter form the comment on
// has_get_kind_method names.
struct DTestsXmlGetKindNode
{
    std::string      name() const     { return std::string(); }
    std::vector<int> children() const { return std::vector<int>(); }
    int              get_kind() const { return 1; }
};

// DTestsXmlFirstChildNode
//   probe: a node reached through first_child() alone, without attributes.
struct DTestsXmlFirstChildNode
{
    std::string name() const        { return std::string(); }
    int         first_child() const { return 0; }
};

// DTestsXmlFullDocument
//   probe: every member of the document protocol.
struct DTestsXmlFullDocument
{
    int         root_element() const { return 0; }
    std::string version() const      { return std::string(); }
    std::string encoding() const     { return std::string(); }
    int         standalone() const   { return 0; }
    std::string write() const        { return std::string(); }

    bool parse(const std::string&)      { return true; }
    bool save(const std::string&) const { return true; }
};

// DTestsXmlDomDocument
//   probe: a DOM-style document, whose root is document_element().
struct DTestsXmlDomDocument
{
    int document_element() const { return 0; }
};

// DTestsXmlTaggedBackend
//   probe: a complete backend, tagged as pugixml's.
struct DTestsXmlTaggedBackend
{
    using backend_tag    = djinterp::DXmlPugixmlBackendTag;
    using node_type      = DTestsXmlFullNode;
    using attribute_type = DTestsXmlShortAttribute;
    using document_type  = DTestsXmlFullDocument;

    static DTestsXmlFullDocument make_document()
    {
        return DTestsXmlFullDocument();
    }
};

// DTestsXmlUntaggedBackend
//   probe: the three type aliases, with no tag.
struct DTestsXmlUntaggedBackend
{
    using node_type      = int;
    using attribute_type = int;
    using document_type  = int;
};

// DTestsXmlPartialBackend
//   probe: a tag and one alias of the three.
struct DTestsXmlPartialBackend
{
    using backend_tag = djinterp::DXmlBackendTag;
    using node_type   = int;
};


//==============================================================================
// xml.hpp
//==============================================================================

/*
d_tests_xml_vocabulary
  Tests the following:
  - DXmlString is std::string and DXmlSize std::size_t, by default
  - the defaults: version "1.0", encoding "UTF-8", indent of two spaces
  - DXmlNodeKind's values are the DOM's nodeType numbers, and unknown is 0
  - DXmlEncoding and DXmlStandalone, in order, one byte each
  - xml_encoding_name for each encoding, "" for custom, and at compile time
  - the five backend tags derive from DXmlBackendTag
  - is_xml_backend: a nested backend_tag, through cv and references; none
*/
void
d_tests_xml_vocabulary(void)
{
    using namespace djinterp;
    using namespace djinterp::xml;

    D_CONSTEXPR const char* ascii = xml_encoding_name(DXmlEncoding::us_ascii);

    D_TESTS_TEXT_CHECK((std::is_same<DXmlString, std::string>::value));
    D_TESTS_TEXT_CHECK((std::is_same<DXmlSize, std::size_t>::value));
    D_TESTS_TEXT_CHECK(std::string(D_XML_DEFAULT_VERSION) == "1.0");
    D_TESTS_TEXT_CHECK(std::string(D_XML_DEFAULT_ENCODING) == "UTF-8");
    D_TESTS_TEXT_CHECK(std::string(D_XML_DEFAULT_INDENT) == "  ");

    D_TESTS_TEXT_CHECK((static_cast<int>(DXmlNodeKind::unknown) == 0) &&
                       (static_cast<int>(DXmlNodeKind::element) == 1) &&
                       (static_cast<int>(DXmlNodeKind::attribute) == 2) &&
                       (static_cast<int>(DXmlNodeKind::text) == 3));
    D_TESTS_TEXT_CHECK(
        (static_cast<int>(DXmlNodeKind::cdata_section) == 4) &&
        (static_cast<int>(DXmlNodeKind::entity_reference) == 5) &&
        (static_cast<int>(DXmlNodeKind::processing_instruction) == 7) &&
        (static_cast<int>(DXmlNodeKind::comment) == 8));
    D_TESTS_TEXT_CHECK(
        (static_cast<int>(DXmlNodeKind::document) == 9) &&
        (static_cast<int>(DXmlNodeKind::document_type) == 10) &&
        (static_cast<int>(DXmlNodeKind::document_fragment) == 11) &&
        (static_cast<int>(DXmlNodeKind::notation) == 12));
    D_TESTS_TEXT_CHECK((sizeof(DXmlNodeKind) == 1) &&
                       (sizeof(DXmlEncoding) == 1) &&
                       (sizeof(DXmlStandalone) == 1));
    D_TESTS_TEXT_CHECK((static_cast<int>(DXmlEncoding::utf_8) == 0) &&
                       (static_cast<int>(DXmlEncoding::custom) == 7));
    D_TESTS_TEXT_CHECK((static_cast<int>(DXmlStandalone::unspecified) == 0) &&
                       (static_cast<int>(DXmlStandalone::yes) == 1) &&
                       (static_cast<int>(DXmlStandalone::no) == 2));

    D_TESTS_TEXT_CHECK(
        (std::string(xml_encoding_name(DXmlEncoding::utf_8)) == "UTF-8") &&
        (std::string(xml_encoding_name(DXmlEncoding::utf_16)) == "UTF-16") &&
        (std::string(xml_encoding_name(DXmlEncoding::utf_16_le)) ==
         "UTF-16LE") &&
        (std::string(xml_encoding_name(DXmlEncoding::utf_16_be)) ==
         "UTF-16BE"));
    D_TESTS_TEXT_CHECK(
        (std::string(xml_encoding_name(DXmlEncoding::utf_32)) == "UTF-32") &&
        (std::string(xml_encoding_name(DXmlEncoding::iso_8859_1)) ==
         "ISO-8859-1") &&
        (std::string(ascii) == "US-ASCII") &&
        (std::string(xml_encoding_name(DXmlEncoding::custom)).empty()));

    D_TESTS_TEXT_CHECK(
        (std::is_base_of<DXmlBackendTag, DXmlDefaultBackendTag>::value) &&
        (std::is_base_of<DXmlBackendTag, DXmlLibxmlppBackendTag>::value) &&
        (std::is_base_of<DXmlBackendTag, DXmlPugixmlBackendTag>::value) &&
        (std::is_base_of<DXmlBackendTag, DXmlTinyxml2BackendTag>::value) &&
        (std::is_base_of<DXmlBackendTag, DXmlRapidxmlBackendTag>::value));

    D_TESTS_TEXT_CHECK(is_xml_backend<DTestsXmlTaggedBackend>::value);
    D_TESTS_TEXT_CHECK(is_xml_backend<const DTestsXmlTaggedBackend&>::value);
    D_TESTS_TEXT_CHECK(is_xml_backend<DTestsXmlPartialBackend>::value);
    D_TESTS_TEXT_CHECK(!is_xml_backend<DTestsXmlUntaggedBackend>::value);
    D_TESTS_TEXT_CHECK(!is_xml_backend<int>::value);
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_TESTS_TEXT_CHECK(is_xml_backend_v<DTestsXmlTaggedBackend> &&
                       !is_xml_backend_v<DTestsXmlUntaggedBackend>);
#endif

    return;
}


/*
d_tests_xml_accessors
  Tests the following:
  - name and value, in the short form (pugixml's) and the getter form
    (libxml++'s), each alone and as the _access trait
  - kind as kind(), as node_type() (libxml++'s) and as get_kind(), the form
    has_get_kind_method's comment names and it once missed
  - text as text() and as get_text()
  - a non-const member isn't an accessor: the traits ask a const object
*/
void
d_tests_xml_accessors(void)
{
    using namespace djinterp;
    using namespace djinterp::xml;

    D_TESTS_TEXT_CHECK(has_name_method<DTestsXmlShortAttribute>::value &&
                       !has_get_name_method<DTestsXmlShortAttribute>::value);
    D_TESTS_TEXT_CHECK(has_get_name_method<DTestsXmlGetterAttribute>::value &&
                       !has_name_method<DTestsXmlGetterAttribute>::value);
    D_TESTS_TEXT_CHECK(has_name_access<DTestsXmlShortAttribute>::value &&
                       has_name_access<DTestsXmlGetterAttribute>::value);
    D_TESTS_TEXT_CHECK(has_value_method<DTestsXmlShortAttribute>::value &&
                       has_get_value_method<DTestsXmlGetterAttribute>::value);
    D_TESTS_TEXT_CHECK(has_value_access<DTestsXmlGetterAttribute>::value &&
                       !has_value_access<DTestsXmlNameOnly>::value);

    D_TESTS_TEXT_CHECK(has_kind_method<DTestsXmlFullNode>::value &&
                       has_kind_access<DTestsXmlFullNode>::value);
    D_TESTS_TEXT_CHECK(has_get_kind_method<DTestsXmlGetterNode>::value &&
                       has_kind_access<DTestsXmlGetterNode>::value);
    D_TESTS_TEXT_CHECK(has_get_kind_method<DTestsXmlGetKindNode>::value &&
                       has_kind_access<DTestsXmlGetKindNode>::value);
    D_TESTS_TEXT_CHECK(!has_kind_access<DTestsXmlNameOnly>::value);

    D_TESTS_TEXT_CHECK(has_text_method<DTestsXmlFullNode>::value &&
                       has_get_text_method<DTestsXmlGetterNode>::value);
    D_TESTS_TEXT_CHECK(has_text_access<DTestsXmlGetterNode>::value &&
                       !has_text_access<DTestsXmlNameOnly>::value);

    D_TESTS_TEXT_CHECK(!has_name_method<DTestsXmlMutableName>::value &&
                       !has_name_access<DTestsXmlMutableName>::value);

    return;
}


/*
d_tests_xml_structure
  Tests the following:
  - an attribute is a name and a value, in either form; a name alone isn't
  - a node is a name and a way down (children(), get_children() or
    first_child()); an element is a node with attributes
  - each member trait of the full probe, and the reading-only probe's
    absences
  - DXmlNodeClass gathers them, is_mutable included
  - the _v forms (C++14) and the concepts (C++20) agree with the traits
*/
void
d_tests_xml_structure(void)
{
    using namespace djinterp;
    using namespace djinterp::xml;

    typedef DXmlNodeClass<DTestsXmlFullNode>   full;
    typedef DXmlNodeClass<DTestsXmlGetterNode> getter;

    D_TESTS_TEXT_CHECK(is_xml_attribute<DTestsXmlShortAttribute>::value &&
                       is_xml_attribute<DTestsXmlGetterAttribute>::value);
    D_TESTS_TEXT_CHECK(!is_xml_attribute<DTestsXmlNameOnly>::value);

    D_TESTS_TEXT_CHECK(is_xml_node<DTestsXmlFullNode>::value &&
                       is_xml_element<DTestsXmlFullNode>::value);
    D_TESTS_TEXT_CHECK(is_xml_node<DTestsXmlGetterNode>::value &&
                       is_xml_element<DTestsXmlGetterNode>::value);
    D_TESTS_TEXT_CHECK(is_xml_node<DTestsXmlFirstChildNode>::value &&
                       !is_xml_element<DTestsXmlFirstChildNode>::value);
    D_TESTS_TEXT_CHECK(!is_xml_node<DTestsXmlNameOnly>::value &&
                       !is_xml_node<DTestsXmlShortAttribute>::value);

    D_TESTS_TEXT_CHECK(
        has_find_attribute_method<DTestsXmlFullNode>::value &&
        has_set_attribute_method<DTestsXmlFullNode>::value &&
        has_remove_attribute_method<DTestsXmlFullNode>::value &&
        has_attribute_count_method<DTestsXmlFullNode>::value);
    D_TESTS_TEXT_CHECK(
        has_find_child_method<DTestsXmlFullNode>::value &&
        has_add_child_method<DTestsXmlFullNode>::value &&
        has_remove_child_method<DTestsXmlFullNode>::value &&
        has_child_count_method<DTestsXmlFullNode>::value &&
        has_parent_method<DTestsXmlFullNode>::value &&
        has_set_text_method<DTestsXmlFullNode>::value);
    D_TESTS_TEXT_CHECK(
        !has_set_attribute_method<DTestsXmlGetterNode>::value &&
        !has_add_child_method<DTestsXmlGetterNode>::value &&
        !has_parent_method<DTestsXmlGetterNode>::value);

    D_TESTS_TEXT_CHECK(full::is_node && full::is_element && full::has_name &&
                       full::has_value && full::has_kind && full::has_text);
    D_TESTS_TEXT_CHECK(full::has_attrs && full::has_find_attr &&
                       full::has_set_attr && full::has_attr_count &&
                       full::has_kids && full::has_first_kid);
    D_TESTS_TEXT_CHECK(full::has_find_kid && full::has_add_kid &&
                       full::has_kid_count && full::has_parent &&
                       full::is_mutable);
    D_TESTS_TEXT_CHECK(getter::is_element && getter::has_kind &&
                       getter::has_text && !getter::has_value &&
                       !getter::has_first_kid && !getter::is_mutable);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_TESTS_TEXT_CHECK(is_xml_node_v<DTestsXmlFullNode> &&
                       is_xml_element_v<DTestsXmlGetterNode> &&
                       is_xml_attribute_v<DTestsXmlShortAttribute> &&
                       !is_xml_node_v<DTestsXmlNameOnly>);
    D_TESTS_TEXT_CHECK(has_name_access_v<DTestsXmlGetterAttribute> &&
                       has_value_access_v<DTestsXmlShortAttribute> &&
                       has_kind_access_v<DTestsXmlGetKindNode> &&
                       has_text_access_v<DTestsXmlGetterNode> &&
                       has_traversal_v<DTestsXmlFirstChildNode>);
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    D_TESTS_TEXT_CHECK(xml_attribute_type<DTestsXmlGetterAttribute> &&
                       xml_node_type<DTestsXmlFirstChildNode> &&
                       xml_element_type<DTestsXmlFullNode> &&
                       !xml_node_type<DTestsXmlNameOnly>);
    D_TESTS_TEXT_CHECK(mutable_xml_node<DTestsXmlFullNode> &&
                       !mutable_xml_node<DTestsXmlGetterNode>);
    D_TESTS_TEXT_CHECK(full_xml_node<DTestsXmlFullNode> &&
                       full_xml_node<DTestsXmlGetterNode> &&
                       !full_xml_node<DTestsXmlFirstChildNode>);
#endif

    return;
}


/*
d_tests_xml_documents
  Tests the following:
  - a document is a type with a root, as root_element() or as a DOM's
    document_element(); one without is not
  - DXmlDocumentClass gathers the prolog and I/O members
  - is_xml_backend_complete asks for the three type aliases, tagged or not;
    make_document is detected on its own
  - the _v forms (C++14) and the concepts (C++20) agree
*/
void
d_tests_xml_documents(void)
{
    using namespace djinterp;
    using namespace djinterp::xml;

    typedef DXmlDocumentClass<DTestsXmlFullDocument> full;
    typedef DXmlDocumentClass<DTestsXmlDomDocument>  dom;

    D_TESTS_TEXT_CHECK(is_xml_document<DTestsXmlFullDocument>::value &&
                       is_xml_document<DTestsXmlDomDocument>::value);
    D_TESTS_TEXT_CHECK(has_root_element_method<DTestsXmlFullDocument>::value &&
        has_document_element_method<DTestsXmlDomDocument>::value);
    D_TESTS_TEXT_CHECK(!is_xml_document<DTestsXmlFullNode>::value &&
                       !is_xml_document<int>::value);

    D_TESTS_TEXT_CHECK(full::is_document && full::has_root &&
                       full::has_version && full::has_encoding &&
                       full::has_standalone);
    D_TESTS_TEXT_CHECK(full::can_parse && full::can_write && full::can_save);
    D_TESTS_TEXT_CHECK(dom::is_document && !dom::has_version &&
                       !dom::can_parse && !dom::can_write && !dom::can_save);

    D_TESTS_TEXT_CHECK(has_node_type_alias<DTestsXmlTaggedBackend>::value &&
        has_attribute_type_alias<DTestsXmlTaggedBackend>::value &&
        has_document_type_alias<DTestsXmlTaggedBackend>::value);
    D_TESTS_TEXT_CHECK(is_xml_backend_complete<DTestsXmlTaggedBackend>::value &&
        is_xml_backend_complete<DTestsXmlUntaggedBackend>::value &&
        !is_xml_backend_complete<DTestsXmlPartialBackend>::value);
    D_TESTS_TEXT_CHECK(
        has_make_document_method<DTestsXmlTaggedBackend>::value &&
        !has_make_document_method<DTestsXmlUntaggedBackend>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_TESTS_TEXT_CHECK(is_xml_document_v<DTestsXmlDomDocument> &&
                       has_root_access_v<DTestsXmlFullDocument> &&
                       is_xml_backend_complete_v<DTestsXmlTaggedBackend>);
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    D_TESTS_TEXT_CHECK(xml_document_type<DTestsXmlDomDocument> &&
                       parseable_xml_document<DTestsXmlFullDocument> &&
                       writable_xml_document<DTestsXmlFullDocument> &&
                       savable_xml_document<DTestsXmlFullDocument>);
    D_TESTS_TEXT_CHECK(full_xml_document<DTestsXmlFullDocument> &&
                       !full_xml_document<DTestsXmlDomDocument>);
    D_TESTS_TEXT_CHECK(xml_backend_type<DTestsXmlPartialBackend> &&
                       complete_xml_backend<DTestsXmlTaggedBackend> &&
                       !complete_xml_backend<DTestsXmlPartialBackend> &&
                       !complete_xml_backend<DTestsXmlUntaggedBackend>);
#endif

    return;
}

#endif  // C++11


#if D_ENV_LANG_IS_CPP14_OR_HIGHER

//==============================================================================
// xml_dialect.hpp
//==============================================================================

// the dialects' name checks, at compile time
static_assert(djinterp::DXmlDialect1_0::is_valid_element_name("ns:a-b.c_1"),
              "a valid name, at compile time");
static_assert(!djinterp::DXmlDialect1_0::is_valid_element_name("1a"),
              "a name can't start with a digit, at compile time");


// DTestsXmlCustomDialect
//   probe: a dialect of its own, by its flag.
struct DTestsXmlCustomDialect
{
    static constexpr djinterp::DXmlDialectFlag flag =
        djinterp::DXmlDialectFlag::custom;
};

// DTestsXmlIntFlag
//   probe: a flag of the wrong type.
struct DTestsXmlIntFlag
{
    static constexpr int flag = 0;
};


/*
d_tests_xml_dialect_properties
  Tests the following:
  - each alias carries its flag
  - the version is "1.1" for XML 1.1 and "1.0" for the rest
  - the default namespace: SVG's, XSLT's, XHTML's and SOAP's URIs, and ""
    for the others
  - canonical dialects emit no declaration, sort attributes, normalise and
    allow no DTD subset; the others do the reverse
  - lowercase tags for XHTML only, XML 1.1's characters for 1.1 only, and
    namespace declarations for all but custom and unspecified
  - the constants every dialect shares: UTF-8, self-closing empties, '"'
    quotes, no BOM, processing instructions allowed
*/
void
d_tests_xml_dialect_properties(void)
{
    using namespace djinterp;
    using namespace djinterp::xml;

    typedef DXmlDialect<DXmlDialectFlag::custom> custom;

    D_TESTS_TEXT_CHECK((DXmlDialect1_0::flag == DXmlDialectFlag::xml_1_0) &&
        (DXmlDialectCanonicalExcl1_0::flag ==
         DXmlDialectFlag::canonical_exclusive_1_0) &&
        (DXmlDialectXslt::flag == DXmlDialectFlag::xslt));

    D_TESTS_TEXT_CHECK(
        (std::string(DXmlDialect1_1::xml_version_string()) == "1.1") &&
        (std::string(DXmlDialect1_0::xml_version_string()) == "1.0") &&
        (std::string(DXmlDialectSvg::xml_version_string()) == "1.0"));

    D_TESTS_TEXT_CHECK(std::string(DXmlDialectSvg::default_namespace_uri()) ==
                       "http://www.w3.org/2000/svg");
    D_TESTS_TEXT_CHECK(std::string(DXmlDialectXslt::default_namespace_uri()) ==
                       "http://www.w3.org/1999/XSL/Transform");
    D_TESTS_TEXT_CHECK(
        std::string(DXmlDialectXhtml::default_namespace_uri()) ==
        "http://www.w3.org/1999/xhtml");
    D_TESTS_TEXT_CHECK(std::string(DXmlDialectSoap::default_namespace_uri()) ==
                       "http://www.w3.org/2003/05/soap-envelope");
    D_TESTS_TEXT_CHECK(
        std::string(DXmlDialect1_0::default_namespace_uri()).empty());

    D_TESTS_TEXT_CHECK(!DXmlDialectCanonical1_0::emit_xml_declaration &&
                       !DXmlDialectCanonical1_1::emit_standalone_attribute &&
                       DXmlDialectCanonicalExcl1_0::sort_attributes &&
                       DXmlDialectCanonical1_0::normalize_line_endings &&
                       DXmlDialectCanonical1_0::normalize_whitespace_in_attrs &&
                       !DXmlDialectCanonical1_0::allow_dtd_subset);
    D_TESTS_TEXT_CHECK(DXmlDialect1_0::emit_xml_declaration &&
                       DXmlDialect1_0::emit_standalone_attribute &&
                       !DXmlDialect1_0::sort_attributes &&
                       !DXmlDialect1_0::normalize_line_endings &&
                       DXmlDialect1_0::allow_dtd_subset);

    D_TESTS_TEXT_CHECK(DXmlDialectXhtml::require_lowercase_tags &&
                       !DXmlDialectSvg::require_lowercase_tags);
    D_TESTS_TEXT_CHECK(DXmlDialect1_1::allow_xml_1_1_chars &&
                       !DXmlDialect1_0::allow_xml_1_1_chars);
    D_TESTS_TEXT_CHECK(DXmlDialectSoap::emit_namespace_declarations &&
                       !custom::emit_namespace_declarations &&
                       !DXmlDialect<DXmlDialectFlag::unspecified>::
                           emit_namespace_declarations);

    D_TESTS_TEXT_CHECK((DXmlDialectSvg::default_encoding() ==
                        DXmlEncoding::utf_8) &&
                       DXmlDialectSvg::self_close_empty &&
                       DXmlDialectSvg::require_quoted_attrs &&
                       (DXmlDialectSvg::attr_quote_char == '"') &&
                       !DXmlDialectSvg::emit_bom &&
                       DXmlDialectSvg::allow_processing_instructions);

    return;
}


/*
d_tests_xml_dialect_names
  Tests the following:
  - a name starts with an ASCII letter or '_', and goes on with letters,
    digits, '-', '.' and ':'
  - the empty name, NULL, a leading digit, '-' or ':', and a blank or '<'
    inside are invalid
  - attribute names follow the element rule, in every dialect
*/
void
d_tests_xml_dialect_names(void)
{
    using namespace djinterp;
    using namespace djinterp::xml;

    D_TESTS_TEXT_CHECK(DXmlDialect1_0::is_valid_element_name("a") &&
                       DXmlDialect1_0::is_valid_element_name("_x") &&
                       DXmlDialect1_0::is_valid_element_name("a-b.c:d") &&
                       DXmlDialect1_0::is_valid_element_name("A1"));
    D_TESTS_TEXT_CHECK(!DXmlDialect1_0::is_valid_element_name("") &&
                       !DXmlDialect1_0::is_valid_element_name(NULL));
    D_TESTS_TEXT_CHECK(!DXmlDialect1_0::is_valid_element_name("1a") &&
                       !DXmlDialect1_0::is_valid_element_name("-a") &&
                       !DXmlDialect1_0::is_valid_element_name(":a"));
    D_TESTS_TEXT_CHECK(!DXmlDialect1_0::is_valid_element_name("a b") &&
                       !DXmlDialect1_0::is_valid_element_name("a<b"));

    D_TESTS_TEXT_CHECK(DXmlDialectSvg::is_valid_attribute_name("xlink:href") &&
                       !DXmlDialectSvg::is_valid_attribute_name("2x"));
    D_TESTS_TEXT_CHECK(
        DXmlDialectCanonical1_0::is_valid_attribute_name("id") &&
        !DXmlDialectCanonical1_0::is_valid_attribute_name("a=b"));

    return;
}


/*
d_tests_xml_dialect_detection
  Tests the following:
  - every bundled alias is a dialect, also through cv and references
  - a struct with its own DXmlDialectFlag flag is a dialect
  - a flag of another type, or none, is not
*/
void
d_tests_xml_dialect_detection(void)
{
    using namespace djinterp;
    using namespace djinterp::xml;

    D_TESTS_TEXT_CHECK(is_xml_dialect<DXmlDialect1_0>::value &&
                       is_xml_dialect<DXmlDialectCanonical1_1>::value &&
                       is_xml_dialect<DXmlDialectXhtml>::value);
    D_TESTS_TEXT_CHECK(is_xml_dialect<const DXmlDialectSvg&>::value);
    D_TESTS_TEXT_CHECK(is_xml_dialect<DTestsXmlCustomDialect>::value);
    D_TESTS_TEXT_CHECK(!is_xml_dialect<DTestsXmlIntFlag>::value &&
                       !is_xml_dialect<DTestsXmlFullNode>::value &&
                       !is_xml_dialect<int>::value);
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_TESTS_TEXT_CHECK(is_xml_dialect_v<DXmlDialectSoap> &&
                       !is_xml_dialect_v<DTestsXmlIntFlag>);
#endif

    return;
}

#endif  // C++14
