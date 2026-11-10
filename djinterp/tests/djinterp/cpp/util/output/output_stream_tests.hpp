/*******************************************************************************
* djinterp [test]                                        output_stream_tests.hpp
*
* The output stream's C++ suite, in DTest's C face
* (c/modules/test/test_suite.h), which compiles at every C++ level from C++98.
*   One entry point builds the module "output_stream" imperatively: a block
* per header, with a test per group of checks.
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
*
*
* path:      /tests/djinterp/cpp/util/output/output_stream_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_UTIL_OUTPUT_OUTPUT_STREAM_TESTS_HPP
#define DJINTERP_TESTS_CPP_UTIL_OUTPUT_OUTPUT_STREAM_TESTS_HPP 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "output_stream" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_output_stream_all(dj_test_suite* _suite);


#endif  // DJINTERP_TESTS_CPP_UTIL_OUTPUT_OUTPUT_STREAM_TESTS_HPP
