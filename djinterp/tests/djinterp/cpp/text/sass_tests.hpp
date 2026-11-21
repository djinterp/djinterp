/*******************************************************************************
* djinterp [test]                                                 sass_tests.hpp
*
*   The suite for text's Sass headers -- css/sass/sass.hpp,
* css/sass/sass_template.hpp and css/sass/sass_printer.hpp -- in DTest's C
* face (c/modules/test/test_suite.h).
*   One entry point builds the module "sass" imperatively: a block per header
* and a test per group of checks. The headers' floor is C++17, and below it a
* group is skipped, saying why. The groups live in sass_tests_sheet.cpp
* and sass_tests_detached.cpp.
*
*
* path:      /tests/djinterp/cpp/text/sass_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_TEXT_SASS_TESTS_HPP
#define DJINTERP_TESTS_CPP_TEXT_SASS_TESTS_HPP 1

// djinterp
#include "text_tests_common.hpp"  // D_TESTS_TEXT_*, DTest's C face


/**
 * @brief Builds and runs the module "sass" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_sass_all(dj_test_suite* _suite);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
void d_tests_sass_vocabulary(void);
void d_tests_sass_sheet_build(void);
void d_tests_sass_sheet_scss(void);
void d_tests_sass_sheet_indented(void);
void d_tests_sass_detached_factories(void);
void d_tests_sass_detached_adopt(void);
void d_tests_sass_printer(void);
#endif  // C++17


#endif  // DJINTERP_TESTS_CPP_TEXT_SASS_TESTS_HPP
