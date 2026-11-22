/*******************************************************************************
* djinterp [cpp][text]                                     text_tests_common.hpp
*
*   What every suite of text's C++ headers shares, in DTest's C face
* (c/modules/test/test_suite.h): the check its groups record through, a
* comparison of texts that notes the two when they differ, the run that opens
* a test and runs its group or skips it below its header's floor, and the
* macros that name a group only at its floor and up.
*   Each suite is its own executable. Its entry point binds its suite with
* d_tests_text_bind before its first group, and unbinds it (NULL) after its
* last; text_tests_common.cpp goes into each suite's leaf.
*
*
* TABLE OF CONTENTS
* =================
* 1.  CHECKS
* 2.  RUNS
* 3.  FLOORS
*
*
* path:      /tests/djinterp/cpp/text/text_tests_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_TEXT_TEXT_TESTS_COMMON_HPP
#define DJINTERP_TESTS_CPP_TEXT_TEXT_TESTS_COMMON_HPP 1

// std
#include <cstddef>  // NULL
// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*


//==============================================================================
// 1.  CHECKS
//==============================================================================

// D_TESTS_TEXT_CHECK
//   macro: one check, recorded against the test open in the bound suite. An
// expression with a top-level comma (a template argument list) goes in
// parentheses.
#define D_TESTS_TEXT_CHECK(expr)                                               \
    d_tests_text_check(((expr) ? true : false), #expr, __FILE__, __LINE__)

/**
 * @brief Binds the suite that checks are recorded against.
 *
 * @param[in] _suite  the suite, or NULL to unbind it.
 */
void d_tests_text_bind(dj_test_suite* _suite);

/**
 * @brief Records one check against the test open in the bound suite.
 *
 * @param[in] _passed      whether the check held.
 * @param[in] _expression  the expression checked, as written.
 * @param[in] _file        the file the check is in.
 * @param[in] _line        the line the check is on.
 */
void d_tests_text_check(bool        _passed,
                        const char* _expression,
                        const char* _file,
                        int         _line);

/**
 * @brief Whether a text written is the one expected. When it isn't, notes
 *        both against the open test, newlines and tabs shown as \n and \t,
 *        so that the failing check says what was written.
 *
 * @param[in] _got       the text written.
 * @param[in] _expected  the text expected.
 * @return whether the two are the same.
 */
bool d_tests_text_same(const char* _got,
                       const char* _expected);


//==============================================================================
// 2.  RUNS
//==============================================================================

/**
 * @brief Opens a test and describes it, then runs its group, or skips the
 *        test when the group is NULL (below its header's floor).
 *
 * @param[in,out] _suite        the suite.
 * @param[in]     _name         the test's name.
 * @param[in]     _description  what the test checks.
 * @param[in]     _group        the group, or NULL.
 * @param[in]     _skip_reason  why the test is skipped when _group is NULL.
 */
void d_tests_text_run(dj_test_suite* _suite,
                      const char*    _name,
                      const char*    _description,
                      void           (*_group)(void),
                      const char*    _skip_reason);


//==============================================================================
// 3.  FLOORS
//==============================================================================

// D_TESTS_TEXT_AT_CPP11, D_TESTS_TEXT_AT_CPP14, D_TESTS_TEXT_AT_CPP17,
// D_TESTS_TEXT_AT_CPP20
//   macros: a group at its floor and up; NULL below it, so that the group's
// name, which is declared only from its floor up, is never named there.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #define D_TESTS_TEXT_AT_CPP11(group) (group)
#else
    #define D_TESTS_TEXT_AT_CPP11(group) NULL
#endif

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #define D_TESTS_TEXT_AT_CPP14(group) (group)
#else
    #define D_TESTS_TEXT_AT_CPP14(group) NULL
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_TESTS_TEXT_AT_CPP17(group) (group)
#else
    #define D_TESTS_TEXT_AT_CPP17(group) NULL
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_TESTS_TEXT_AT_CPP20(group) (group)
#else
    #define D_TESTS_TEXT_AT_CPP20(group) NULL
#endif


#endif  // DJINTERP_TESTS_CPP_TEXT_TEXT_TESTS_COMMON_HPP
