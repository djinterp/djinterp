/*******************************************************************************
* djinterp [test]                                      html_tests_vocabulary.cpp
*
*   The groups for html/html.hpp and html/html_dialect.hpp: versions and
* DOCTYPEs, the element kinds and their tag names, the categories, the
* traits (on probes, as the XML suite's), and the dialects. The kinds are
* looked up at run time as well as at compile time: a recursive comparison
* forced inline once made GCC refuse every run-time lookup outside testing.
*
*
* path:      /tests/djinterp/cpp/text/html_tests_vocabulary.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "html_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <cstddef>  // NULL, std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::is_base_of, std::is_same
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/html/html.hpp"  // vocabulary
#include "../../../../inc/djinterp/cpp/text/html/html_dialect.hpp"  // dialects


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

//==============================================================================
// PROBES
//==============================================================================

// DTestsHtmlElement
//   probe: an XML element with an html_kind(), classes and an id.
struct DTestsHtmlElement
{
    std::string              name() const        { return std::string(); }
    std::vector<int>         children() const    { return std::vector<int>(); }
    std::vector<int>         attributes() const  { return std::vector<int>(); }
    int                      html_kind() const   { return 0; }
    std::vector<std::string> class_list() const
    {
        return std::vector<std::string>();
    }
    bool        has_class(const std::string&) const { return false; }
    void        add_class(const std::string&)       {}
    void        remove_class(const std::string&)    {}
    std::string get_id() const                      { return std::string(); }
    void        set_id(const std::string&)          {}
};

// DTestsHtmlGetterElement
//   probe: get_html_kind(), reached through first_child(), no classes.
struct DTestsHtmlGetterElement
{
    std::string      name() const          { return std::string(); }
    int              first_child() const   { return 0; }
    std::vector<int> attributes() const    { return std::vector<int>(); }
    int              get_html_kind() const { return 0; }
};

// DTestsHtmlXmlElement
//   probe: an XML element and nothing of HTML's.
struct DTestsHtmlXmlElement
{
    std::string      name() const       { return std::string(); }
    std::vector<int> children() const   { return std::vector<int>(); }
    std::vector<int> attributes() const { return std::vector<int>(); }
};

// DTestsHtmlDocument
//   probe: every member of the HTML document protocol.
struct DTestsHtmlDocument
{
    int         root_element() const { return 0; }
    std::string doctype() const      { return std::string(); }
    int         html_version() const { return 0; }
    int         head_element() const { return 0; }
    int         body_element() const { return 0; }
    std::string title() const        { return std::string(); }

    void set_doctype(const std::string&) {}
    void set_title(const std::string&)   {}
};

// DTestsHtmlGetterDocument
//   probe: a DOM's document_element() and get_doctype() alone.
struct DTestsHtmlGetterDocument
{
    int         document_element() const { return 0; }
    std::string get_doctype() const      { return std::string(); }
};

// DTestsHtmlXmlDocument
//   probe: an XML document with neither a DOCTYPE nor a version.
struct DTestsHtmlXmlDocument
{
    int root_element() const { return 0; }
};

// DTestsHtmlBackend
//   probe: a complete HTML backend, tagged as gumbo's.
struct DTestsHtmlBackend
{
    using html_backend_tag = djinterp::html::DHtmlGumboBackendTag;
    using element_type     = int;
    using document_type    = int;

    static int make_html_document() { return 0; }
};

// DTestsHtmlUntaggedBackend
//   probe: the two aliases, with no tag.
struct DTestsHtmlUntaggedBackend
{
    using element_type  = int;
    using document_type = int;
};


//==============================================================================
// html.hpp
//==============================================================================

/*
d_tests_html_versions
  Tests the following:
  - the defaults: the HTML5 DOCTYPE and an indent of two spaces
  - each version's DOCTYPE, as the W3C spells it; "" for custom and
    unspecified
  - the four XHTML versions are XHTML, the others not, also at compile time
*/
void
d_tests_html_versions(void)
{
    using namespace djinterp::html;

    D_CONSTEXPR bool xhtml5 = is_xhtml_version(DHtmlVersion::xhtml5);

    D_TESTS_TEXT_CHECK(std::string(D_HTML_DEFAULT_DOCTYPE) == "<!DOCTYPE html>");
    D_TESTS_TEXT_CHECK(std::string(D_HTML_DEFAULT_INDENT) == "  ");

    D_TESTS_TEXT_CHECK(
        (std::string(html_doctype_string(DHtmlVersion::html5)) ==
         "<!DOCTYPE html>") &&
        (std::string(html_doctype_string(DHtmlVersion::xhtml5)) ==
         "<!DOCTYPE html>"));
    D_TESTS_TEXT_CHECK(
        std::string(html_doctype_string(DHtmlVersion::html4_strict)) ==
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" "
        "\"http://www.w3.org/TR/html4/strict.dtd\">");
    D_TESTS_TEXT_CHECK(
        std::string(html_doctype_string(DHtmlVersion::html4_transitional)) ==
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
        "\"http://www.w3.org/TR/html4/loose.dtd\">");
    D_TESTS_TEXT_CHECK(
        std::string(html_doctype_string(DHtmlVersion::html4_frameset)) ==
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" "
        "\"http://www.w3.org/TR/html4/frameset.dtd\">");
    D_TESTS_TEXT_CHECK(
        std::string(html_doctype_string(DHtmlVersion::xhtml1_strict)) ==
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
        "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">");
    D_TESTS_TEXT_CHECK(
        std::string(html_doctype_string(DHtmlVersion::xhtml1_transitional)) ==
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
        "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">");
    D_TESTS_TEXT_CHECK(
        std::string(html_doctype_string(DHtmlVersion::xhtml1_frameset)) ==
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN\" "
        "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd\">");
    D_TESTS_TEXT_CHECK(
        std::string(html_doctype_string(DHtmlVersion::custom)).empty() &&
        std::string(html_doctype_string(DHtmlVersion::unspecified)).empty());

    D_TESTS_TEXT_CHECK(is_xhtml_version(DHtmlVersion::xhtml1_strict) &&
                       is_xhtml_version(DHtmlVersion::xhtml1_transitional) &&
                       is_xhtml_version(DHtmlVersion::xhtml1_frameset) &&
                       xhtml5);
    D_TESTS_TEXT_CHECK(!is_xhtml_version(DHtmlVersion::html5) &&
                       !is_xhtml_version(DHtmlVersion::html4_strict) &&
                       !is_xhtml_version(DHtmlVersion::custom));

    return;
}


/*
d_tests_html_kinds
  Tests the following:
  - each of the 113 element kinds has a tag name, and the name maps back to
    the kind; seventeen once had neither direction (ruby, param, slot, ...)
  - a few names: html, div, template, param
  - a name built at run time maps to its kind
  - NULL, "", a name in capitals and a custom element are unknown; unknown
    has no name; the enums are one byte, with unknown at 255
  - the lookup is also a constant expression
*/
void
d_tests_html_kinds(void)
{
    using namespace djinterp::html;

    const std::size_t last  = static_cast<std::size_t>(DHtmlElementKind::slot);
    const std::string built = std::string("sec") + "tion";
    D_CONSTEXPR DHtmlElementKind at_compile = html_kind_from_name("table");
    std::size_t      named = 0;
    bool             round = true;
    std::size_t      k;

    for (k = 0; k <= last; ++k)
    {
        const DHtmlElementKind kind = static_cast<DHtmlElementKind>(k);
        const char* const      name = name_from_html_kind(kind);

        if ( (name == NULL) ||
             (name[0] == '\0') )
        {
            round = false;
            continue;
        }

        ++named;
        round = round && (html_kind_from_name(name) == kind);
    }

    D_TESTS_TEXT_CHECK(round && (named == 113) && (last == 112));
    D_TESTS_TEXT_CHECK(
        (std::string(name_from_html_kind(DHtmlElementKind::html_root)) ==
         "html") &&
        (std::string(name_from_html_kind(DHtmlElementKind::div_)) == "div") &&
        (std::string(name_from_html_kind(DHtmlElementKind::template_)) ==
         "template") &&
        (std::string(name_from_html_kind(DHtmlElementKind::param)) ==
         "param"));
    D_TESTS_TEXT_CHECK(html_kind_from_name(built.c_str()) ==
                       DHtmlElementKind::section);

    D_TESTS_TEXT_CHECK((html_kind_from_name(NULL) ==
                        DHtmlElementKind::unknown) &&
                       (html_kind_from_name("") == DHtmlElementKind::unknown));
    D_TESTS_TEXT_CHECK((html_kind_from_name("DIV") ==
                        DHtmlElementKind::unknown) &&
                       (html_kind_from_name("my-widget") ==
                        DHtmlElementKind::unknown));
    D_TESTS_TEXT_CHECK(
        std::string(name_from_html_kind(DHtmlElementKind::unknown)).empty());
    D_TESTS_TEXT_CHECK((static_cast<int>(DHtmlElementKind::unknown) == 255) &&
                       (static_cast<int>(DHtmlAttributeKind::unknown) == 255));
    D_TESTS_TEXT_CHECK((sizeof(DHtmlElementKind) == 1) &&
                       (sizeof(DHtmlAttributeKind) == 1) &&
                       (sizeof(DHtmlVersion) == 1));

    D_TESTS_TEXT_CHECK(at_compile == DHtmlElementKind::table);

    return;
}


/*
d_tests_html_categories
  Tests the following:
  - the void elements are HTML's fourteen, no more: area, base, br, col,
    embed, hr, img, input, link, meta, param, source, track, wbr
  - for every kind, the name-based predicates agree with the kind-based
  - script and style are raw text; title and textarea escapable raw text
  - block, inline, form, metadata and heading samples
  - a custom element has no category
*/
void
d_tests_html_categories(void)
{
    using namespace djinterp::html;

    static const char* const voids[14] =
    {
        "area", "base", "br", "col", "embed", "hr", "img", "input", "link",
        "meta", "param", "source", "track", "wbr"
    };
    const std::size_t last = static_cast<std::size_t>(DHtmlElementKind::slot);
    std::size_t       void_kinds = 0;
    bool              named_voids = true;
    bool              agree = true;
    std::size_t       k;

    for (k = 0; k <= last; ++k)
    {
        const DHtmlElementKind kind = static_cast<DHtmlElementKind>(k);
        const char* const      name = name_from_html_kind(kind);

        void_kinds += is_void_element_kind(kind) ? 1u : 0u;
        agree = agree &&
            (is_void_element_name(name) == is_void_element_kind(kind)) &&
            (is_block_element_name(name) == is_block_element_kind(kind)) &&
            (is_inline_element_name(name) == is_inline_element_kind(kind)) &&
            (is_raw_text_element_name(name) == is_raw_text_element_kind(kind));
    }

    for (k = 0; k < 14; ++k)
    {
        named_voids = named_voids && is_void_element_name(voids[k]);
    }

    D_TESTS_TEXT_CHECK(named_voids && (void_kinds == 14));
    D_TESTS_TEXT_CHECK(agree);

    D_TESTS_TEXT_CHECK(is_raw_text_element_name("script") &&
                       is_raw_text_element_name("style") &&
                       !is_raw_text_element_name("title") &&
                       !is_raw_text_element_name("textarea"));
    D_TESTS_TEXT_CHECK(
        ((html_category_for_kind(DHtmlElementKind::title) &
          hec_escapable_raw) != 0u) &&
        ((html_category_for_kind(DHtmlElementKind::textarea) &
          hec_escapable_raw) != 0u));

    D_TESTS_TEXT_CHECK(is_block_element_name("div") &&
                       is_block_element_name("p") &&
                       !is_block_element_name("span"));
    D_TESTS_TEXT_CHECK(is_inline_element_name("span") &&
                       is_inline_element_name("textarea") &&
                       !is_inline_element_name("div"));
    D_TESTS_TEXT_CHECK(is_form_element_kind(DHtmlElementKind::form) &&
                       is_form_element_kind(DHtmlElementKind::input) &&
                       !is_form_element_kind(DHtmlElementKind::div_));
    D_TESTS_TEXT_CHECK(is_metadata_element_kind(DHtmlElementKind::meta) &&
                       is_metadata_element_kind(DHtmlElementKind::title) &&
                       !is_metadata_element_kind(DHtmlElementKind::body));
    D_TESTS_TEXT_CHECK(
        ((html_category_for_kind(DHtmlElementKind::h1) & hec_heading) != 0u) &&
        ((html_category_for_kind(DHtmlElementKind::h6) & hec_heading) != 0u));

    D_TESTS_TEXT_CHECK(
        (html_category_for_kind(DHtmlElementKind::unknown) == hec_none) &&
        !is_void_element_name("my-widget"));

    return;
}


/*
d_tests_html_traits
  Tests the following:
  - the five backend tags derive from DHtmlBackendTag; is_html_backend
    asks for a nested html_backend_tag
  - the html kind, as html_kind() or get_html_kind(); classes; the id
  - an HTML element is an XML element with an html kind; the loose form
    asks only for the XML element; DHtmlElementClass
  - an HTML document is an XML document with a DOCTYPE or a version;
    DHtmlDocumentClass
  - complete backends; the _v forms (C++14) and the concepts (C++20)
*/
void
d_tests_html_traits(void)
{
    using namespace djinterp::html;

    typedef DHtmlElementClass<DTestsHtmlElement>   element;
    typedef DHtmlDocumentClass<DTestsHtmlDocument> document;

    D_TESTS_TEXT_CHECK(
        (std::is_base_of<DHtmlBackendTag, DHtmlDefaultBackendTag>::value) &&
        (std::is_base_of<DHtmlBackendTag, DHtmlLibxmlppBackendTag>::value) &&
        (std::is_base_of<DHtmlBackendTag, DHtmlGumboBackendTag>::value) &&
        (std::is_base_of<DHtmlBackendTag, DHtmlLexborBackendTag>::value) &&
        (std::is_base_of<DHtmlBackendTag, DHtmlHtmlcxxBackendTag>::value));
    D_TESTS_TEXT_CHECK(is_html_backend<DTestsHtmlBackend>::value &&
                       is_html_backend<const DTestsHtmlBackend&>::value &&
                       !is_html_backend<DTestsHtmlUntaggedBackend>::value);

    D_TESTS_TEXT_CHECK(has_html_kind_access<DTestsHtmlElement>::value &&
                       has_html_kind_access<DTestsHtmlGetterElement>::value &&
                       !has_html_kind_access<DTestsHtmlXmlElement>::value);
    D_TESTS_TEXT_CHECK(has_class_support<DTestsHtmlElement>::value &&
                       has_class_list_method<DTestsHtmlElement>::value &&
                       !has_class_support<DTestsHtmlGetterElement>::value);
    D_TESTS_TEXT_CHECK(has_id_method<DTestsHtmlElement>::value &&
                       has_set_id_method<DTestsHtmlElement>::value &&
                       !has_id_method<DTestsHtmlXmlElement>::value);

    D_TESTS_TEXT_CHECK(is_html_element<DTestsHtmlElement>::value &&
                       is_html_element<DTestsHtmlGetterElement>::value &&
                       !is_html_element<DTestsHtmlXmlElement>::value &&
                       is_html_element_loose<DTestsHtmlXmlElement>::value);
    D_TESTS_TEXT_CHECK(element::is_html_elem && element::has_id &&
                       element::has_set_id);

    D_TESTS_TEXT_CHECK(is_html_document<DTestsHtmlDocument>::value &&
                       is_html_document<DTestsHtmlGetterDocument>::value &&
                       !is_html_document<DTestsHtmlXmlDocument>::value &&
                       is_html_document_loose<DTestsHtmlXmlDocument>::value);
    D_TESTS_TEXT_CHECK(document::is_html_doc && document::has_doctype &&
                       document::has_set_doctype && document::has_version &&
                       document::has_dom_shortcuts && document::has_title &&
                       document::has_set_title);

    D_TESTS_TEXT_CHECK(is_html_backend_complete<DTestsHtmlBackend>::value &&
        is_html_backend_complete<DTestsHtmlUntaggedBackend>::value &&
        has_make_html_document_method<DTestsHtmlBackend>::value &&
        !has_make_html_document_method<DTestsHtmlUntaggedBackend>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_TESTS_TEXT_CHECK(is_html_element_v<DTestsHtmlElement> &&
                       is_html_document_v<DTestsHtmlDocument> &&
                       has_class_support_v<DTestsHtmlElement> &&
                       is_html_backend_complete_v<DTestsHtmlBackend>);
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    D_TESTS_TEXT_CHECK(full_html_element<DTestsHtmlElement> &&
                       html_element_type<DTestsHtmlGetterElement> &&
                       !classed_html_element<DTestsHtmlGetterElement>);
    D_TESTS_TEXT_CHECK(full_html_document<DTestsHtmlDocument> &&
                       doctype_aware_document<DTestsHtmlGetterDocument> &&
                       !versioned_html_document<DTestsHtmlGetterDocument>);
    D_TESTS_TEXT_CHECK(complete_html_backend<DTestsHtmlBackend> &&
                       !complete_html_backend<DTestsHtmlUntaggedBackend>);
#endif

    return;
}

#endif  // C++11


#if D_ENV_LANG_IS_CPP14_OR_HIGHER

//==============================================================================
// html_dialect.hpp
//==============================================================================

// a dialect's checks, at compile time
static_assert(djinterp::html::DHtmlDialectHtml5::is_valid_element_kind(
                  djinterp::html::DHtmlElementKind::main_),
              "main is HTML5's");
static_assert(!djinterp::html::DHtmlDialectHtml4Strict::is_valid_element_kind(
                  djinterp::html::DHtmlElementKind::main_),
              "and not HTML 4's");


/*
d_tests_html_dialect_families
  Tests the following:
  - each alias carries its version
  - the families: HTML5 and XHTML5 are HTML5's; the HTML 4 and XHTML 1
    versions are HTML 4's; strict, frameset and XHTML
  - XHTML dialects defer to XML's XHTML dialect; the others to the
    unspecified one
*/
void
d_tests_html_dialect_families(void)
{
    using namespace djinterp::html;

    D_TESTS_TEXT_CHECK((DHtmlDialectHtml4Strict::flag ==
                        DHtmlVersion::html4_strict) &&
                       (DHtmlDialectXhtml5::flag == DHtmlVersion::xhtml5));

    D_TESTS_TEXT_CHECK(DHtmlDialectHtml5::is_html5_family &&
                       !DHtmlDialectHtml5::is_html4_family &&
                       !DHtmlDialectHtml5::is_xhtml &&
                       DHtmlDialectHtml5::is_strict &&
                       !DHtmlDialectHtml5::is_frameset);
    D_TESTS_TEXT_CHECK(DHtmlDialectXhtml1Strict::is_xhtml &&
                       DHtmlDialectXhtml1Strict::is_html4_family &&
                       DHtmlDialectXhtml1Strict::is_strict);
    D_TESTS_TEXT_CHECK(DHtmlDialectHtml4Frameset::is_frameset &&
                       DHtmlDialectXhtml1Frameset::is_frameset &&
                       !DHtmlDialectHtml4Transitional::is_strict);
    D_TESTS_TEXT_CHECK(DHtmlDialectXhtml5::is_xhtml &&
                       DHtmlDialectXhtml5::is_html5_family);

    D_TESTS_TEXT_CHECK(
        (std::is_same<DHtmlDialectXhtml1Strict::underlying_xml_dialect_t,
                      djinterp::DXmlDialectXhtml>::value) &&
        (std::is_same<DHtmlDialectHtml5::underlying_xml_dialect_t,
                      djinterp::DXmlDialect<
                          djinterp::DXmlDialectFlag::unspecified>>::value));

    return;
}


/*
d_tests_html_dialect_rules
  Tests the following:
  - XHTML self-closes void elements, wants lowercase tags and quoted
    attributes, and allows no unclosed optional tags; HTML the reverse
  - only XHTML 1 emits an XML declaration; legacy presentation is for the
    transitional and frameset versions
  - each dialect's DOCTYPE is its version's
  - HTML5's own elements are invalid in HTML 4 dialects; unknown is invalid
    everywhere; custom allows every known element
  - void per dialect: wbr is HTML5's, br everyone's
  - is_html_dialect: the bundled dialects, through cv and references; not
    an XML dialect, nor an int
*/
void
d_tests_html_dialect_rules(void)
{
    using namespace djinterp::html;

    D_TESTS_TEXT_CHECK(DHtmlDialectXhtml1Strict::self_close_void_elements &&
                       DHtmlDialectXhtml1Strict::require_lowercase_tags &&
                       DHtmlDialectXhtml1Strict::require_quoted_attrs &&
                       !DHtmlDialectXhtml5::allow_unclosed_optional);
    D_TESTS_TEXT_CHECK(!DHtmlDialectHtml5::self_close_void_elements &&
                       !DHtmlDialectHtml5::require_quoted_attrs &&
                       DHtmlDialectHtml5::allow_unclosed_optional &&
                       (DHtmlDialectHtml5::attr_quote_char == '"'));

    D_TESTS_TEXT_CHECK(DHtmlDialectXhtml1Transitional::emit_xml_declaration &&
                       !DHtmlDialectXhtml5::emit_xml_declaration &&
                       !DHtmlDialectHtml5::emit_xml_declaration);
    D_TESTS_TEXT_CHECK(
        DHtmlDialectHtml4Transitional::legacy_presentational_allowed &&
        DHtmlDialectXhtml1Frameset::legacy_presentational_allowed &&
        !DHtmlDialectHtml4Strict::legacy_presentational_allowed);
    D_TESTS_TEXT_CHECK(DHtmlDialectHtml5::html5_features_allowed &&
                       !DHtmlDialectHtml4Strict::html5_features_allowed &&
                       DHtmlDialectHtml4Frameset::frameset_features_allowed);

    D_TESTS_TEXT_CHECK(
        (std::string(DHtmlDialectHtml5::doctype_string()) ==
         "<!DOCTYPE html>") &&
        (std::string(DHtmlDialectHtml4Strict::doctype_string()) ==
         html_doctype_string(DHtmlVersion::html4_strict)));

    D_TESTS_TEXT_CHECK(
        DHtmlDialectHtml5::is_valid_element_kind(DHtmlElementKind::section) &&
        !DHtmlDialectHtml4Strict::is_valid_element_kind(
            DHtmlElementKind::section) &&
        DHtmlDialectHtml4Strict::is_valid_element_kind(DHtmlElementKind::div_));
    D_TESTS_TEXT_CHECK(
        !DHtmlDialectHtml5::is_valid_element_kind(DHtmlElementKind::unknown) &&
        DHtmlDialect<DHtmlVersion::custom>::is_valid_element_kind(
            DHtmlElementKind::slot));

    D_TESTS_TEXT_CHECK(
        DHtmlDialectHtml5::is_void_element(DHtmlElementKind::wbr) &&
        !DHtmlDialectHtml4Strict::is_void_element(DHtmlElementKind::wbr) &&
        DHtmlDialectHtml4Strict::is_void_element(DHtmlElementKind::br));

    D_TESTS_TEXT_CHECK(is_html_dialect<DHtmlDialectHtml5>::value &&
                       is_html_dialect<const DHtmlDialectXhtml5&>::value &&
                       !is_html_dialect<djinterp::DXmlDialect1_0>::value &&
                       !is_html_dialect<int>::value);
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_TESTS_TEXT_CHECK(is_html_dialect_v<DHtmlDialectHtml4Frameset>);
#endif

    return;
}

#endif  // C++14
