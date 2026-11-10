/*******************************************************************************
* djinterp [test]                                           pdf_stream_tests.hpp
*
* The C++ PDF writer's stream suite, in DTest's C face
* (c/modules/test/test_suite.h).
*   One entry point builds the module "pdf_stream_cpp" imperatively: a block
* for pdf_document.hpp, with a test per group of checks, each skipped below
* C++17, the PDF module's floor.
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
*
*
* path:      /tests/djinterp/cpp/util/pdf/pdf_stream_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_UTIL_PDF_PDF_STREAM_TESTS_HPP
#define DJINTERP_TESTS_CPP_UTIL_PDF_PDF_STREAM_TESTS_HPP 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "pdf_stream" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_pdf_cpp_all(dj_test_suite* _suite);


#endif  // DJINTERP_TESTS_CPP_UTIL_PDF_PDF_STREAM_TESTS_HPP
