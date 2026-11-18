/*******************************************************************************
* djinterp [test]                                                  xml_tests.cpp
*
*   The module "xml": its entry point, which opens a block per header and a
* test per group, and runs each group or skips it below its header's floor.
*
*
* path:      /tests/djinterp/cpp/text/xml_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "xml_tests.hpp"  // the suite's entry point, DTest's C face


/*
d_tests_xml_vocabulary_block
  The block for xml.hpp, C++11 and up.
*/
static void
d_tests_xml_vocabulary_block(
    dj_test_suite* _suite
)
{
    static const char* const below = "below C++11, xml.hpp's floor";

    (void)d_test_suite_block(_suite, "xml/xml.hpp");

    d_tests_text_run(_suite, "vocabulary",
        "the shared types and defaults, the enums' values, the encodings' "
        "names, the backend tags and is_xml_backend",
        D_TESTS_TEXT_AT_CPP11(d_tests_xml_vocabulary), below);
    d_tests_text_run(_suite, "accessors",
        "name, value, kind and text, in their short and getter forms, "
        "detected on const members only",
        D_TESTS_TEXT_AT_CPP11(d_tests_xml_accessors), below);
    d_tests_text_run(_suite, "structure",
        "attributes, nodes and elements classified by what they expose; "
        "DXmlNodeClass; the _v forms and the concepts",
        D_TESTS_TEXT_AT_CPP11(d_tests_xml_structure), below);
    d_tests_text_run(_suite, "documents",
        "documents by their root accessor, DXmlDocumentClass, and complete "
        "backends",
        D_TESTS_TEXT_AT_CPP11(d_tests_xml_documents), below);

    return;
}


/*
d_tests_xml_dialect_block
  The block for xml_dialect.hpp, C++14 and up.
*/
static void
d_tests_xml_dialect_block(
    dj_test_suite* _suite
)
{
    static const char* const below = "below C++14, xml_dialect.hpp's floor";

    (void)d_test_suite_block(_suite, "xml/xml_dialect.hpp");

    d_tests_text_run(_suite, "properties",
        "each dialect's flag, version, namespace and emission rules",
        D_TESTS_TEXT_AT_CPP14(d_tests_xml_dialect_properties), below);
    d_tests_text_run(_suite, "names",
        "valid element and attribute names, at run time and at compile time",
        D_TESTS_TEXT_AT_CPP14(d_tests_xml_dialect_names), below);
    d_tests_text_run(_suite, "detection",
        "is_xml_dialect: the bundled dialects, a custom one, and types that "
        "aren't",
        D_TESTS_TEXT_AT_CPP14(d_tests_xml_dialect_detection), below);

    return;
}


/*
d_tests_xml_tree_block
  The block for xml_template.hpp, C++20 and up.
*/
static void
d_tests_xml_tree_block(
    dj_test_suite* _suite
)
{
    static const char* const below = "below C++20, xml_template.hpp's floor";

    (void)d_test_suite_block(_suite, "xml/xml_template.hpp");

    d_tests_text_run(_suite, "attributes",
        "DXmlAttribute: empty, named, compared; make_attribute",
        D_TESTS_TEXT_AT_CPP20(d_tests_xml_tree_attributes), below);
    d_tests_text_run(_suite, "nodes",
        "children and parents, attributes added, replaced and removed, text, "
        "and the invalid node",
        D_TESTS_TEXT_AT_CPP20(d_tests_xml_tree_nodes), below);
    d_tests_text_run(_suite, "documents",
        "the prolog's defaults and setters, the root, make_default_document; "
        "the default backend neither parses nor saves",
        D_TESTS_TEXT_AT_CPP20(d_tests_xml_tree_documents), below);
    d_tests_text_run(_suite, "write",
        "the serialised text: prolog, indentation, empty elements, and text "
        "and values escaped",
        D_TESTS_TEXT_AT_CPP20(d_tests_xml_tree_write), below);
    d_tests_text_run(_suite, "path",
        "find_child_by_path: segments, extra slashes, the empty path, and "
        "misses",
        D_TESTS_TEXT_AT_CPP20(d_tests_xml_tree_path), below);
    d_tests_text_run(_suite, "traits",
        "the tree's own types meet xml.hpp's protocol",
        D_TESTS_TEXT_AT_CPP20(d_tests_xml_tree_traits), below);

    return;
}


/*
d_tests_xml_all
  Binds the suite, opens the module, then each header's block and its
tests.
*/
void
d_tests_xml_all(
    dj_test_suite* _suite
)
{
    d_tests_text_bind(_suite);

    (void)d_test_suite_module(_suite, "xml");
    (void)d_test_suite_describe(_suite,
                                "text's XML headers: vocabulary and traits, "
                                "dialects, and the tree");

    d_tests_xml_vocabulary_block(_suite);
    d_tests_xml_dialect_block(_suite);
    d_tests_xml_tree_block(_suite);

    d_tests_text_bind(NULL);

    return;
}
