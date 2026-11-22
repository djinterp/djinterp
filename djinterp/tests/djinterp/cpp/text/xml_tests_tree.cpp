/*******************************************************************************
* djinterp [cpp][text]                                        xml_tests_tree.cpp
*
*   The groups for xml/xml_template.hpp, on its default backend: the
* attribute, nodes and their links, the document and its prolog, the text
* write() produces, find_child_by_path, and the tree's own types against
* xml.hpp's protocol. The default backend neither parses nor saves, and the
* groups hold it to that: both return false and change nothing.
*
*
* path:      /tests/djinterp/cpp/text/xml_tests_tree.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "xml_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <cstddef>  // NULL
#include <string>   // std::string
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/xml/xml.hpp"  // the protocol
#include "../../../../inc/djinterp/cpp/text/xml/xml_template.hpp"  // the tree


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// xml_document, xml_node
//   the tree on its default backend.
typedef djinterp::xml::DXmlDocument<djinterp::xml::DXmlDefaultBackend>
    xml_document;
typedef djinterp::xml::DXmlNode<djinterp::xml::DXmlDefaultBackend>
    xml_node;


/*
d_tests_xml_same
  Whether two nodes are the same node: the same backend node, or both
invalid.
*/
static bool
d_tests_xml_same(
    const xml_node& _a,
    const xml_node& _b
)
{
    return (_a.backend_handle() == _b.backend_handle());
}


/*
d_tests_xml_tree_attributes
  Tests the following:
  - a default DXmlAttribute has an empty name and value
  - the two-argument one holds what it was given
  - == compares both name and value; make_attribute builds the same
*/
void
d_tests_xml_tree_attributes(void)
{
    using djinterp::xml::DXmlAttribute;
    using djinterp::xml::make_attribute;

    const DXmlAttribute blank;
    const DXmlAttribute lang("lang", "en");

    D_TESTS_TEXT_CHECK(blank.name.empty() && blank.value.empty());
    D_TESTS_TEXT_CHECK((lang.name == "lang") && (lang.value == "en"));
    D_TESTS_TEXT_CHECK(lang == make_attribute("lang", "en"));
    D_TESTS_TEXT_CHECK(!(lang == make_attribute("lang", "fr")) &&
                       !(lang == make_attribute("dir", "en")));
    D_TESTS_TEXT_CHECK(blank == DXmlAttribute());

    return;
}


/*
d_tests_xml_tree_nodes
  Tests the following:
  - a document's root is a valid element with its name, and no parent
  - add_child links child and parent; first_child, find_child (the first of
    a name) and child_count follow
  - set_attribute returns true when it adds and false when it replaces;
    attributes keep their order; find_attribute, attribute_count and
    remove_attribute
  - set_text and text
  - remove_child removes the first child of a name, and says whether it
    did
  - a default node is invalid, with no backend node
*/
void
d_tests_xml_tree_nodes(void)
{
    using djinterp::xml::DXmlAttribute;
    using djinterp::DXmlNodeKind;

    xml_document         doc("catalog");
    xml_node             root  = doc.root_element();
    xml_node             book  = root.add_child("book");
    xml_node             title = book.add_child("title");
    xml_node             other = root.add_child("shelf");
    xml_node             twin  = root.add_child("shelf");
    const xml_node       unset;
    const DXmlAttribute* id    = NULL;

    D_TESTS_TEXT_CHECK(root.valid() && (root.name() == "catalog") &&
                       (root.kind() == DXmlNodeKind::element));
    D_TESTS_TEXT_CHECK(!root.parent().valid());

    D_TESTS_TEXT_CHECK((root.child_count() == 3) && (book.child_count() == 1));
    D_TESTS_TEXT_CHECK(d_tests_xml_same(book.parent(), root) &&
                       d_tests_xml_same(title.parent(), book));
    D_TESTS_TEXT_CHECK(d_tests_xml_same(root.first_child(), book) &&
                       !title.first_child().valid());
    D_TESTS_TEXT_CHECK(d_tests_xml_same(root.find_child("shelf"), other) &&
                       !d_tests_xml_same(root.find_child("shelf"), twin));
    D_TESTS_TEXT_CHECK(!root.find_child("nope").valid());

    D_TESTS_TEXT_CHECK(book.set_attribute("id", "b1"));
    D_TESTS_TEXT_CHECK(!book.set_attribute("id", "b2"));
    D_TESTS_TEXT_CHECK(book.set_attribute("lang", "en") &&
                       (book.attribute_count() == 2));
    D_TESTS_TEXT_CHECK((book.attributes()[0].name == "id") &&
                       (book.attributes()[1].name == "lang"));
    id = book.find_attribute("id");
    D_TESTS_TEXT_CHECK((id != NULL) && (id->value == "b2"));
    D_TESTS_TEXT_CHECK(book.find_attribute("missing") == NULL);
    D_TESTS_TEXT_CHECK(book.remove_attribute("id") &&
                       !book.remove_attribute("id") &&
                       (book.attribute_count() == 1));

    title.set_text("Dune");
    D_TESTS_TEXT_CHECK((title.text() == "Dune") && book.text().empty());

    D_TESTS_TEXT_CHECK(root.remove_child("shelf") &&
                       (root.child_count() == 2));
    D_TESTS_TEXT_CHECK(d_tests_xml_same(root.find_child("shelf"), twin));
    D_TESTS_TEXT_CHECK(root.remove_child("shelf") &&
                       !root.remove_child("shelf") &&
                       (root.child_count() == 1));

    D_TESTS_TEXT_CHECK(!unset.valid() && (unset.backend_handle() == NULL));

    return;
}


/*
d_tests_xml_tree_documents
  Tests the following:
  - a default document has no root and the default prolog: "1.0",
    "UTF-8", standalone unspecified
  - the prolog's setters
  - root_element and document_element are the same node; set_root replaces
    the root
  - parse returns false and leaves the document as it was; save returns
    false: the default backend does neither
  - make_default_document, with a root and without
*/
void
d_tests_xml_tree_documents(void)
{
    using djinterp::DXmlStandalone;
    using djinterp::xml::make_default_document;

    const xml_document none;
    xml_document       doc("a");
    xml_node           b;

    D_TESTS_TEXT_CHECK(!none.root_element().valid());
    D_TESTS_TEXT_CHECK((none.version() == "1.0") &&
                       (none.encoding() == "UTF-8") &&
                       (none.standalone() == DXmlStandalone::unspecified));

    doc.set_version("1.1");
    doc.set_encoding("ISO-8859-1");
    doc.set_standalone(DXmlStandalone::yes);
    D_TESTS_TEXT_CHECK((doc.version() == "1.1") &&
                       (doc.encoding() == "ISO-8859-1") &&
                       (doc.standalone() == DXmlStandalone::yes));

    D_TESTS_TEXT_CHECK(d_tests_xml_same(doc.root_element(),
                                        doc.document_element()));
    b = doc.set_root("b");
    D_TESTS_TEXT_CHECK((b.name() == "b") &&
                       d_tests_xml_same(doc.root_element(), b));

    D_TESTS_TEXT_CHECK(!doc.parse("<c/>") &&
                       (doc.root_element().name() == "b"));
    D_TESTS_TEXT_CHECK(!doc.save("never_written.xml"));

    D_TESTS_TEXT_CHECK(!make_default_document().root_element().valid());
    D_TESTS_TEXT_CHECK(make_default_document("r").root_element().name() ==
                       "r");

    return;
}


/*
d_tests_xml_tree_write
  Tests the following:
  - the prolog, then the root: two spaces of indent a level, an element
    with no children and no text as <x/>, text inline
  - standalone="yes" and standalone="no" when set, and the version and
    encoding as set
  - text and attribute values are escaped: &, < and > everywhere, and " in
    a value; a document with no root is its prolog alone
  - an element with text and children: the text, then the children
*/
void
d_tests_xml_tree_write(void)
{
    using djinterp::DXmlStandalone;

    xml_document catalog("catalog");
    xml_node     book = catalog.root_element().add_child("book");
    xml_document flags("f");
    xml_document escaped("p");
    xml_document mixed("a");
    const xml_document none;

    book.set_attribute("id", "b1");
    book.add_child("title").set_text("Dune");
    catalog.root_element().add_child("empty");

    D_TESTS_TEXT_CHECK(catalog.write() ==
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<catalog>\n"
        "  <book id=\"b1\">\n"
        "    <title>Dune</title>\n"
        "  </book>\n"
        "  <empty/>\n"
        "</catalog>\n");

    flags.set_standalone(DXmlStandalone::yes);
    D_TESTS_TEXT_CHECK(flags.write() ==
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<f/>\n");
    flags.set_standalone(DXmlStandalone::no);
    flags.set_version("1.1");
    flags.set_encoding("UTF-16");
    D_TESTS_TEXT_CHECK(flags.write() ==
        "<?xml version=\"1.1\" encoding=\"UTF-16\" standalone=\"no\"?>\n"
        "<f/>\n");

    escaped.root_element().set_attribute("title", "a \"b\" & <c>");
    escaped.root_element().set_text("x < y & z > w \"q\"");
    D_TESTS_TEXT_CHECK(escaped.write() ==
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<p title=\"a &quot;b&quot; &amp; &lt;c&gt;\">"
        "x &lt; y &amp; z &gt; w \"q\"</p>\n");
    D_TESTS_TEXT_CHECK(none.write() ==
                       "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

    mixed.root_element().set_text("t");
    mixed.root_element().add_child("b");
    D_TESTS_TEXT_CHECK(mixed.write() ==
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<a>t\n"
        "  <b/>\n"
        "</a>\n");

    return;
}


/*
d_tests_xml_tree_path
  Tests the following:
  - "a/b/c" walks down by name
  - leading, trailing and doubled slashes are skipped
  - the empty path is the start
  - a missing segment, anywhere, gives an invalid node; so does an invalid
    start
*/
void
d_tests_xml_tree_path(void)
{
    using djinterp::xml::find_child_by_path;

    xml_document doc("root");
    xml_node     root = doc.root_element();
    xml_node     a    = root.add_child("a");
    xml_node     b    = a.add_child("b");
    xml_node     c    = b.add_child("c");

    D_TESTS_TEXT_CHECK(d_tests_xml_same(find_child_by_path(root, "a/b/c"), c));
    D_TESTS_TEXT_CHECK(d_tests_xml_same(find_child_by_path(root, "/a//b/"),
                                        b));
    D_TESTS_TEXT_CHECK(d_tests_xml_same(find_child_by_path(root, ""), root));
    D_TESTS_TEXT_CHECK(!find_child_by_path(root, "a/x/c").valid() &&
                       !find_child_by_path(root, "a/b/x").valid() &&
                       !find_child_by_path(root, "x").valid());
    D_TESTS_TEXT_CHECK(!find_child_by_path(xml_node(), "a").valid());

    return;
}


/*
d_tests_xml_tree_traits
  Tests the following:
  - DXmlNode is a node and an element, and full and mutable (C++20)
  - DXmlDocument is a document, full, parseable, writable and savable
  - DXmlDefaultBackend is a backend, complete, with make_document
*/
void
d_tests_xml_tree_traits(void)
{
    using namespace djinterp;
    using namespace djinterp::xml;

    D_TESTS_TEXT_CHECK(is_xml_node<xml_node>::value &&
                       is_xml_element<xml_node>::value);
    D_TESTS_TEXT_CHECK(is_xml_document<xml_document>::value);
    D_TESTS_TEXT_CHECK(is_xml_backend<DXmlDefaultBackend>::value &&
                       is_xml_backend_complete<DXmlDefaultBackend>::value &&
                       has_make_document_method<DXmlDefaultBackend>::value);
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    D_TESTS_TEXT_CHECK(full_xml_node<xml_node> && mutable_xml_node<xml_node>);
    D_TESTS_TEXT_CHECK(full_xml_document<xml_document> &&
                       parseable_xml_document<xml_document> &&
                       writable_xml_document<xml_document> &&
                       savable_xml_document<xml_document>);
    D_TESTS_TEXT_CHECK(complete_xml_backend<DXmlDefaultBackend>);
#endif

    return;
}

#endif  // C++20
