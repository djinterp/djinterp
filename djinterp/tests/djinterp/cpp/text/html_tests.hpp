/*******************************************************************************
* djinterp [test]                                                 html_tests.hpp
*
*   text's HTML headers -- html/html.hpp, html/html_dialect.hpp,
* html/html_printer.hpp and html/html_template.hpp -- in DTest's C face
* (c/modules/test/test_suite.h).
*   One entry point builds the module "html" imperatively: a block per header
* and a test per group of checks. A group runs at its header's floor and up,
* and below it is skipped, saying why: html.hpp is C++11, html_dialect.hpp
* C++14, html_printer.hpp C++17 and html_template.hpp C++20.
*   The groups live in the section files: html_tests_vocabulary.cpp (html.hpp
* and html_dialect.hpp), html_tests_printer.cpp (html_printer.hpp) and
* html_tests_tree.cpp (html_template.hpp). The printer and the tree are kept
* in separate units: html_printer.hpp includes cpp/util/document/document.hpp,
* whose DXmlDocument clashes with xml_template.hpp's, until the clash is
* settled.
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
* 2.  GROUPS
*     a.  html.hpp
*     b.  html_dialect.hpp
*     c.  html_printer.hpp
*     d.  html_template.hpp
*
*
* path:      /tests/djinterp/cpp/text/html_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_TEXT_HTML_TESTS_HPP
#define DJINTERP_TESTS_CPP_TEXT_HTML_TESTS_HPP 1

// djinterp
#include "text_tests_common.hpp"  // D_TESTS_TEXT_*, DTest's C face


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "html" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_html_all(dj_test_suite* _suite);


//==============================================================================
// 2.  GROUPS
//==============================================================================

// a.  html.hpp (html_tests_vocabulary.cpp)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
void d_tests_html_versions(void);
void d_tests_html_kinds(void);
void d_tests_html_categories(void);
void d_tests_html_traits(void);
#endif  // C++11

// b.  html_dialect.hpp (html_tests_vocabulary.cpp)
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
void d_tests_html_dialect_families(void);
void d_tests_html_dialect_rules(void);
#endif  // C++14

// c.  html_printer.hpp (html_tests_printer.cpp)
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
void d_tests_html_printer(void);
#endif  // C++17

// d.  html_template.hpp (html_tests_tree.cpp)
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
void d_tests_html_tree_elements(void);
void d_tests_html_tree_documents(void);
void d_tests_html_tree_write(void);
void d_tests_html_tree_traits(void);
#endif  // C++20


#endif  // DJINTERP_TESTS_CPP_TEXT_HTML_TESTS_HPP
