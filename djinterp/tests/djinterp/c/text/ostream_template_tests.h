/*******************************************************************************
* djinterp [test]                                       ostream_template_tests.h
*
* The template engine's suite in DTest's C face (c/modules/test/test_suite.h).
*   One entry point builds the module "ostream_template" imperatively: a
* block for ostream_template.h, with a test per group of checks.
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
*
*
* path:      /tests/djinterp/c/text/ostream_template_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_TEXT_OSTREAM_TEMPLATE_TESTS_H
#define DJINTERP_TESTS_C_TEXT_OSTREAM_TEMPLATE_TESTS_H 1

// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "ostream_template" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_template_all(struct dj_test_suite* _suite);


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_OSTREAM_TEMPLATE_TESTS_H
