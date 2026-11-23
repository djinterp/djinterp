/*******************************************************************************
* djinterp [cpp][text]                                   markup_string_tests.hpp
*
*   text's markup string templates - markup_string_template.hpp and its two
* facades, html/html_string_template.hpp and xml/xml_string_template.hpp --
* in DTest's C face (c/modules/test/test_suite.h).
*   One entry point builds the module "markup_string" imperatively: a block
* per header and a test per group of checks. All three headers are C++11 at
* their floor; below it each test is skipped, saying why.
*   The groups live in the section files, markup_string_tests_engine.cpp and
* markup_string_tests_facades.cpp, and record their checks through
* D_TESTS_TEXT_CHECK (text_tests_common.hpp).
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
* 2.  GROUPS
*     a.  markup_string_template.hpp
*     b.  the HTML and XML facades
*
*
* path:      /tests/djinterp/cpp/text/markup_string_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_TEXT_MARKUP_STRING_TESTS_HPP
#define DJINTERP_TESTS_CPP_TEXT_MARKUP_STRING_TESTS_HPP 1

// djinterp
#include "text_tests_common.hpp"  // D_TESTS_TEXT_*, DTest's C face


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "markup_string" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_markup_string_all(dj_test_suite* _suite);


//==============================================================================
// 2.  GROUPS
//==============================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// a.  markup_string_template.hpp (markup_string_tests_engine.cpp)
void d_tests_markup_string_escape(void);
void d_tests_markup_string_variables(void);
void d_tests_markup_string_sections(void);
void d_tests_markup_string_literals(void);
void d_tests_markup_string_partials(void);
void d_tests_markup_string_syntax(void);
void d_tests_markup_string_lifecycle(void);

// b.  the HTML and XML facades (markup_string_tests_facades.cpp)
void d_tests_markup_string_html(void);
void d_tests_markup_string_xml(void);

#endif  // C++11


#endif  // DJINTERP_TESTS_CPP_TEXT_MARKUP_STRING_TESTS_HPP
