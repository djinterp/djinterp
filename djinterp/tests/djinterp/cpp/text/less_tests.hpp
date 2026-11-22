/*******************************************************************************
* djinterp [cpp][text]                                            less_tests.hpp
*
*   The suite for text's Less headers -- css/less/less.hpp and
* css/less/less_template.hpp -- in DTest's C face
* (c/modules/test/test_suite.h).
*   One entry point builds the module "less" imperatively: a block per header
* and a test per group of checks. Both headers' floor is C++17, and below it
* a group is skipped, saying why. The groups live in less_tests_sheet.cpp
* and less_tests_detached.cpp.
*
*
* path:      /tests/djinterp/cpp/text/less_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_TEXT_LESS_TESTS_HPP
#define DJINTERP_TESTS_CPP_TEXT_LESS_TESTS_HPP 1

// djinterp
#include "text_tests_common.hpp"  // D_TESTS_TEXT_*, DTest's C face


/**
 * @brief Builds and runs the module "less" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_less_all(dj_test_suite* _suite);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
void d_tests_less_vocabulary(void);
void d_tests_less_sheet_build(void);
void d_tests_less_sheet_render(void);
void d_tests_less_detached_factories(void);
void d_tests_less_detached_adopt(void);
#endif  // C++17


#endif  // DJINTERP_TESTS_CPP_TEXT_LESS_TESTS_HPP
