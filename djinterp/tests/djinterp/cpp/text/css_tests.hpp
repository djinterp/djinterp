/*******************************************************************************
* djinterp [cpp][text]                                             css_tests.hpp
*
*   The suite for text's CSS headers -- css/css.hpp, css/css_template.hpp
* and css/css_printer.hpp -- in DTest's C face (c/modules/test/test_suite.h).
*   One entry point builds the module "css" imperatively: a block per header
* and a test per group of checks. Every header's floor is C++17, and below it
* a group is skipped, saying why.
*   The groups live in the section files: css_tests_vocabulary.cpp
* (css.hpp), css_tests_sheet.cpp and css_tests_detached.cpp
* (css_template.hpp), and css_tests_printer.cpp (css_printer.hpp).
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
* 2.  GROUPS
*     a.  css.hpp
*     b.  css_template.hpp
*     c.  css_printer.hpp
*
*
* path:      /tests/djinterp/cpp/text/css_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_TEXT_CSS_TESTS_HPP
#define DJINTERP_TESTS_CPP_TEXT_CSS_TESTS_HPP 1

// djinterp
#include "text_tests_common.hpp"  // D_TESTS_TEXT_*, DTest's C face


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "css" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_css_all(dj_test_suite* _suite);


//==============================================================================
// 2.  GROUPS
//==============================================================================

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// a.  css.hpp (css_tests_vocabulary.cpp)
void d_tests_css_rules(void);
void d_tests_css_at_rules(void);
void d_tests_css_names(void);

// b.  css_template.hpp (css_tests_sheet.cpp, css_tests_detached.cpp)
void d_tests_css_sheet_build(void);
void d_tests_css_sheet_render(void);
void d_tests_css_sheet_at_rules(void);
void d_tests_css_detached_factories(void);
void d_tests_css_detached_adopt(void);

// c.  css_printer.hpp (css_tests_printer.cpp)
void d_tests_css_printer(void);

#endif  // C++17


#endif  // DJINTERP_TESTS_CPP_TEXT_CSS_TESTS_HPP
