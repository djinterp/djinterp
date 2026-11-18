/*******************************************************************************
* djinterp [test]                                                  xml_tests.hpp
*
*   text's XML headers -- xml/xml.hpp, xml/xml_dialect.hpp and
* xml/xml_template.hpp -- in DTest's C face (c/modules/test/test_suite.h).
*   One entry point builds the module "xml" imperatively: a block per header
* and a test per group of checks. A group runs at its header's floor and
* up, and below it is skipped, saying why: xml.hpp is C++11, xml_dialect.hpp
* C++14 and xml_template.hpp C++20.
*   The groups live in the section files, xml_tests_traits.cpp (xml.hpp and
* xml_dialect.hpp) and xml_tests_tree.cpp (xml_template.hpp), and record
* their checks through D_TESTS_TEXT_CHECK (text_tests_common.hpp).
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
* 2.  GROUPS
*     a.  xml.hpp
*     b.  xml_dialect.hpp
*     c.  xml_template.hpp
*
*
* path:      /tests/djinterp/cpp/text/xml_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_TEXT_XML_TESTS_HPP
#define DJINTERP_TESTS_CPP_TEXT_XML_TESTS_HPP 1

// djinterp
#include "text_tests_common.hpp"  // D_TESTS_TEXT_*, DTest's C face


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "xml" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_xml_all(dj_test_suite* _suite);


//==============================================================================
// 2.  GROUPS
//==============================================================================

// a.  xml.hpp (xml_tests_traits.cpp)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
void d_tests_xml_vocabulary(void);
void d_tests_xml_accessors(void);
void d_tests_xml_structure(void);
void d_tests_xml_documents(void);
#endif  // C++11

// b.  xml_dialect.hpp (xml_tests_traits.cpp)
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
void d_tests_xml_dialect_properties(void);
void d_tests_xml_dialect_names(void);
void d_tests_xml_dialect_detection(void);
#endif  // C++14

// c.  xml_template.hpp (xml_tests_tree.cpp)
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
void d_tests_xml_tree_attributes(void);
void d_tests_xml_tree_nodes(void);
void d_tests_xml_tree_documents(void);
void d_tests_xml_tree_write(void);
void d_tests_xml_tree_path(void);
void d_tests_xml_tree_traits(void);
#endif  // C++20


#endif  // DJINTERP_TESTS_CPP_TEXT_XML_TESTS_HPP
