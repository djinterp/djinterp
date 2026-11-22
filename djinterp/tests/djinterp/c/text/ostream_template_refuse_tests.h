/*******************************************************************************
* djinterp [c][text]                             ostream_template_refuse_tests.h
*
*   The template engine's refusing UTF-8 knob (D_CFG_OSTREAM_TEMPLATE_UTF8 =
* D_CFG_OSTREAM_TEMPLATE_UTF8_REFUSE), in DTest's C face
* (c/modules/test/test_suite.h). One entry point builds the module
* "ostream_template_refuse".
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
*
*
* path:      /tests/djinterp/c/text/ostream_template_refuse_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_TEXT_OSTREAM_TEMPLATE_REFUSE_TESTS_H
#define DJINTERP_TESTS_C_TEXT_OSTREAM_TEMPLATE_REFUSE_TESTS_H 1

// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "ostream_template_refuse" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_template_refuse_all(struct dj_test_suite* _suite);


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_OSTREAM_TEMPLATE_REFUSE_TESTS_H
