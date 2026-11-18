/*******************************************************************************
* djinterp [test]                                            text_core_tests.hpp
*
*   text's core C++ headers -- text_align.hpp, text_template.hpp and
* text_pattern.hpp -- in DTest's C face (c/modules/test/test_suite.h).
*   One entry point builds the module "text_core" imperatively: a block per
* header and a test per group of checks. A group runs at its header's floor
* and up, and below it is skipped, saying why: text_align.hpp and
* text_pattern.hpp are C++11, text_template.hpp is C++17 (std::string_view).
*   The groups live in the section files, text_core_tests_template.cpp and
* text_core_tests_pattern.cpp, and record their checks through
* D_TESTS_TEXT_CHECK (text_tests_common.hpp).
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
* 2.  GROUPS
*     a.  text_align.hpp and text_template.hpp
*     b.  text_pattern.hpp
*
*
* path:      /tests/djinterp/cpp/text/text_core_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_TEXT_TEXT_CORE_TESTS_HPP
#define DJINTERP_TESTS_CPP_TEXT_TEXT_CORE_TESTS_HPP 1

// djinterp
#include "text_tests_common.hpp"  // D_TESTS_TEXT_*, DTest's C face


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "text_core" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_text_core_all(dj_test_suite* _suite);


//==============================================================================
// 2.  GROUPS
//==============================================================================

// a.  text_align.hpp and text_template.hpp (text_core_tests_template.cpp)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
void d_tests_text_core_align(void);
#endif  // C++11

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
void d_tests_text_core_template_parser(void);
void d_tests_text_core_template_dialect(void);
void d_tests_text_core_template_render(void);
void d_tests_text_core_template_lookups(void);
void d_tests_text_core_template_copies(void);
void d_tests_text_core_template_pattern(void);
#endif  // C++17

// b.  text_pattern.hpp (text_core_tests_pattern.cpp)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
void d_tests_text_core_pattern_compile(void);
void d_tests_text_core_pattern_escape(void);
void d_tests_text_core_pattern_match(void);
void d_tests_text_core_pattern_find(void);
void d_tests_text_core_pattern_render(void);
void d_tests_text_core_pattern_compose(void);
#endif  // C++11


#endif  // DJINTERP_TESTS_CPP_TEXT_TEXT_CORE_TESTS_HPP
