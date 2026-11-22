/*******************************************************************************
* djinterp [cpp][text]                                       html_tests_tree.cpp
*
*   The groups for html/html_template.hpp, on its default backend: elements,
* documents, the text write() produces, and the tree's own types against
* html.hpp's protocol. Until 11-13 no DHtmlDocument could be instantiated: it
* called a backend_document() its base didn't have.
*   The freestanding factories (make_div and the rest) and adopt are
* html_tests_detached.cpp's.
*   It also includes html_printer.hpp, whose printed documents (djinterp's
* DHtmlDocument and DXmlDocument) share their names with the trees
* (djinterp::html's and djinterp::xml's), to check they share a unit.
*
*
* path:      /tests/djinterp/cpp/text/html_tests_tree.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "html_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <string>  // std::string
#include <vector>  // std::vector
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::is_same
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/html/html_template.hpp"  // tree
#include "../../../../inc/djinterp/cpp/text/html/html_printer.hpp"  // printer
#include "../../../../inc/djinterp/cpp/text/xml/xml_template.hpp"  // XML tree


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// html_document, html_element, xml_node
//   the tree on its default backend.
typedef djinterp::html::DHtmlDocument<djinterp::html::DHtmlDefaultBackend>
    html_document;
typedef djinterp::html::DHtmlElement<djinterp::html::DHtmlDefaultBackend>
    html_element;
typedef djinterp::xml::DXmlNode<djinterp::html::DHtmlDefaultBackend> xml_node;


/*
d_tests_html_tree_elements
  Tests the following:
  - an element's html kind is its name's, get_html_kind the same; an
    invalid element and a custom name are unknown
  - is_void, is_block, is_inline and is_raw_text follow the kind
  - set_id and get_id; no id is ""
  - class_list splits on blanks; has_class; add_class appends once;
    remove_class keeps the others
*/
void
d_tests_html_tree_elements(void)
{
    using djinterp::html::DHtmlElementKind;

    html_document       doc = djinterp::html::make_html5_document<
                                  djinterp::html::DHtmlDefaultBackend>();
    html_element        body = doc.body_element();
    html_element        div(body.add_child("div"));
    const html_element  br(body.add_child("br"));
    const html_element  script(body.add_child("script"));
    const html_element  custom(body.add_child("my-widget"));
    const html_element  invalid;
    std::vector<std::string> classes;

    D_TESTS_TEXT_CHECK((div.html_kind() == DHtmlElementKind::div_) &&
                       (div.get_html_kind() == DHtmlElementKind::div_));
    D_TESTS_TEXT_CHECK((custom.html_kind() == DHtmlElementKind::unknown) &&
                       (invalid.html_kind() == DHtmlElementKind::unknown));
    D_TESTS_TEXT_CHECK(br.is_void() && !div.is_void() && div.is_block() &&
                       !div.is_inline() && script.is_raw_text());

    D_TESTS_TEXT_CHECK(div.get_id().empty());
    div.set_id("main");
    D_TESTS_TEXT_CHECK(div.get_id() == "main");

    div.set_attribute("class", "  a\tb  c ");
    classes = div.class_list();
    D_TESTS_TEXT_CHECK((classes.size() == 3) && (classes[0] == "a") &&
                       (classes[1] == "b") && (classes[2] == "c"));
    D_TESTS_TEXT_CHECK(div.has_class("b") && !div.has_class("d"));

    div.add_class("d");
    div.add_class("a");
    classes = div.class_list();
    D_TESTS_TEXT_CHECK((classes.size() == 4) && (classes[3] == "d"));

    div.remove_class("b");
    D_TESTS_TEXT_CHECK(!div.has_class("b") && div.has_class("a") &&
                       div.has_class("d") && (div.class_list().size() == 3));

    return;
}


/*
d_tests_html_tree_documents
  Tests the following:
  - a default document is HTML5, with HTML5's DOCTYPE and no root
  - a version's constructor and set_html_version set the DOCTYPE;
    set_doctype makes the version custom
  - make_html5_document: an html root with head and body, and the title
  - set_title adds a head when the root has none, and a title in it
  - make_xhtml_document: the version's DOCTYPE, and the title
*/
void
d_tests_html_tree_documents(void)
{
    using djinterp::html::DHtmlDefaultBackend;
    using djinterp::html::DHtmlVersion;
    using djinterp::html::html_doctype_string;

    const html_document plain;
    html_document       strict(DHtmlVersion::html4_strict);
    html_document       titled =
        djinterp::html::make_html5_document<DHtmlDefaultBackend>("T");
    const html_document xhtml =
        djinterp::html::make_xhtml_document<DHtmlDefaultBackend>(
            DHtmlVersion::xhtml1_transitional, "X");
    html_document       headless(DHtmlVersion::html5);

    D_TESTS_TEXT_CHECK((plain.html_version() == DHtmlVersion::html5) &&
                       (plain.doctype() == "<!DOCTYPE html>") &&
                       !plain.root_element().valid());

    D_TESTS_TEXT_CHECK(strict.get_doctype() ==
                       html_doctype_string(DHtmlVersion::html4_strict));
    strict.set_html_version(DHtmlVersion::xhtml5);
    D_TESTS_TEXT_CHECK((strict.html_version() == DHtmlVersion::xhtml5) &&
                       (strict.doctype() == "<!DOCTYPE html>"));
    strict.set_doctype("<!DOCTYPE mine>");
    D_TESTS_TEXT_CHECK((strict.html_version() == DHtmlVersion::custom) &&
                       (strict.doctype() == "<!DOCTYPE mine>"));

    D_TESTS_TEXT_CHECK((titled.root_element().name() == "html") &&
                       (titled.head_element().name() == "head") &&
                       (titled.body_element().name() == "body"));
    D_TESTS_TEXT_CHECK(titled.title() == "T");
    titled.set_title("U");
    D_TESTS_TEXT_CHECK((titled.title() == "U") &&
                       (titled.head_element().child_count() == 1));

    (void)headless.set_root("html");
    D_TESTS_TEXT_CHECK(!headless.head_element().valid() &&
                       headless.title().empty());
    headless.set_title("H");
    D_TESTS_TEXT_CHECK(headless.head_element().valid() &&
                       (headless.title() == "H"));

    D_TESTS_TEXT_CHECK(
        (xhtml.doctype() ==
         html_doctype_string(DHtmlVersion::xhtml1_transitional)) &&
        (xhtml.title() == "X"));

    return;
}


/*
d_tests_html_tree_write
  Tests the following:
  - the DOCTYPE, then the tree, two spaces of indent a level
  - void elements with no end tag (param among them); an empty element
    paired; text and attribute values escaped; a script's text not escaped
  - XHTML closes a void element with " />"
*/
void
d_tests_html_tree_write(void)
{
    using djinterp::html::DHtmlDefaultBackend;
    using djinterp::html::DHtmlVersion;

    html_document page =
        djinterp::html::make_html5_document<DHtmlDefaultBackend>("A & B");
    html_element  body = page.body_element();
    html_document xhtml =
        djinterp::html::make_xhtml_document<DHtmlDefaultBackend>(
            DHtmlVersion::xhtml1_strict);

    html_element(body.add_child("p")).set_attribute("title", "say \"hi\"");
    body.find_child("p").set_text("1 < 2");
    (void)body.add_child("br");
    (void)body.add_child("param");
    (void)body.add_child("div");
    body.add_child("script").set_text("if (a < b && c) {}");

    D_TESTS_TEXT_CHECK(page.write() ==
        "<!DOCTYPE html>\n"
        "<html>\n"
        "  <head>\n"
        "    <title>A &amp; B</title>\n"
        "  </head>\n"
        "  <body>\n"
        "    <p title=\"say &quot;hi&quot;\">1 &lt; 2</p>\n"
        "    <br>\n"
        "    <param>\n"
        "    <div></div>\n"
        "    <script>if (a < b && c) {}</script>\n"
        "  </body>\n"
        "</html>\n");

    (void)xhtml.body_element().add_child("br");
    D_TESTS_TEXT_CHECK(xhtml.write().find("    <br />\n") != std::string::npos);

    return;
}


/*
d_tests_html_tree_traits
  Tests the following:
  - DHtmlElement adds nothing to DXmlNode's size, as the header promises
  - DHtmlElement is an HTML element, classed and identifiable; DHtmlDocument
    an HTML document with every shortcut (C++20 concepts)
  - DHtmlDefaultBackend is a complete backend, for HTML and for XML
*/
void
d_tests_html_tree_traits(void)
{
    using namespace djinterp::html;

    D_TESTS_TEXT_CHECK(sizeof(html_element) == sizeof(xml_node));

    D_TESTS_TEXT_CHECK(is_html_element<html_element>::value &&
                       is_html_document<html_document>::value);
    D_TESTS_TEXT_CHECK(is_html_backend<DHtmlDefaultBackend>::value &&
                       is_html_backend_complete<DHtmlDefaultBackend>::value &&
                       djinterp::is_xml_backend<DHtmlDefaultBackend>::value &&
                       djinterp::xml::is_xml_backend_complete<
                           DHtmlDefaultBackend>::value);
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    D_TESTS_TEXT_CHECK(full_html_element<html_element> &&
                       full_html_document<html_document>);
    D_TESTS_TEXT_CHECK(complete_html_backend<DHtmlDefaultBackend> &&
                       djinterp::xml::complete_xml_backend<
                           DHtmlDefaultBackend>);
#endif

    return;
}

/*
d_tests_html_tree_beside_printer
  The trees and the printer's documents in one unit. Each name means its own
type: the printed documents are djinterp's, the trees djinterp::html's and
djinterp::xml's.
  Tests the following:
  - djinterp::DHtmlDocument (a document printed as HTML) is not
    djinterp::html::DHtmlDocument (the tree), and each can be made here
  - djinterp::DXmlDocument (a document printed as XML) is not
    djinterp::xml::DXmlDocument (the tree), and each can be made here
*/
void
d_tests_html_tree_beside_printer(void)
{
    typedef djinterp::xml::DXmlDocument<djinterp::xml::DXmlDefaultBackend>
        xml_document;

    const html_document           tree;
    const djinterp::DHtmlDocument printed;
    const xml_document            xml_tree =
        djinterp::xml::make_default_document();
    const djinterp::DXmlDocument  xml_printed;

    D_TESTS_TEXT_CHECK((!std::is_same<djinterp::DHtmlDocument,
                                      html_document>::value));
    D_TESTS_TEXT_CHECK((!std::is_same<djinterp::DXmlDocument,
                                      xml_document>::value));
    D_TESTS_TEXT_CHECK(tree.html_version() ==
                       djinterp::html::DHtmlVersion::html5);
    (void)printed;
    (void)xml_tree;
    (void)xml_printed;

    return;
}

#endif  // C++20
