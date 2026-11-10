/*******************************************************************************
* djinterp [test]                                             pdf_stream_tests.h
*
* The PDF writer's stream suite in DTest's C face
* (c/modules/test/test_suite.h).
*   One entry point builds the module "pdf_stream" imperatively: a block for
* pdf_builtin_backend.h and pdf_document.h, with a test per group of checks.
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
*
*
* path:      /tests/djinterp/c/util/pdf/pdf_stream_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_UTIL_PDF_PDF_STREAM_TESTS_H
#define DJINTERP_TESTS_C_UTIL_PDF_PDF_STREAM_TESTS_H 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "pdf_stream" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_pdf_all(struct dj_test_suite* _suite);


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_UTIL_PDF_PDF_STREAM_TESTS_H
