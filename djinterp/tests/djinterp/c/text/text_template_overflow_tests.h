/*******************************************************************************
* djinterp [test]                                 text_template_overflow_tests.h
*
* text_template.c's overflow suite in DTest's C face
* (c/modules/test/test_suite.h).
*   One entry point builds the module "text_template_overflow". Its section
* includes text_template.c whole, to reach the static growth, so the leaf
* builds it alone and does not link text_template.c again.
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
*
*
* path:      /tests/djinterp/c/text/text_template_overflow_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_TEXT_TEXT_TEMPLATE_OVERFLOW_TESTS_H
#define DJINTERP_TESTS_C_TEXT_TEXT_TEMPLATE_OVERFLOW_TESTS_H 1

// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "text_template_overflow" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_tt_overflow_all(struct dj_test_suite* _suite);


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_TEXT_TEMPLATE_OVERFLOW_TESTS_H
