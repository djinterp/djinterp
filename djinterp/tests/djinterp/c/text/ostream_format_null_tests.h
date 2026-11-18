/*******************************************************************************
* djinterp [test]                                    ostream_format_null_tests.h
*
* printf's heap fallback with no heap, in DTest's C face
* (c/modules/test/test_suite.h).
*   One entry point builds the module "ostream_format_null": a block for
* ostream_format.h on a build whose default memory source is the null source.
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
*
*
* path:      /tests/djinterp/c/text/ostream_format_null_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_TEXT_OSTREAM_FORMAT_NULL_TESTS_H
#define DJINTERP_TESTS_C_TEXT_OSTREAM_FORMAT_NULL_TESTS_H 1

// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "ostream_format_null" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_format_null_all(struct dj_test_suite* _suite);


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_OSTREAM_FORMAT_NULL_TESTS_H
