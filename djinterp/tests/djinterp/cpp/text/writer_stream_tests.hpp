/*******************************************************************************
* djinterp [test]                                        writer_stream_tests.hpp
*
* text's writers on the output stream, in DTest's C face
* (c/modules/test/test_suite.h).
*   One entry point builds the module "writer_stream" imperatively: a block
* for writer.hpp, printer.hpp and document.hpp, with a test per group of
* checks, each skipped below C++17, printer.hpp's floor.
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
*
*
* path:      /tests/djinterp/cpp/text/writer_stream_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_TEXT_WRITER_STREAM_TESTS_HPP
#define DJINTERP_TESTS_CPP_TEXT_WRITER_STREAM_TESTS_HPP 1

// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "writer_stream" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_writer_all(dj_test_suite* _suite);


#endif  // DJINTERP_TESTS_CPP_TEXT_WRITER_STREAM_TESTS_HPP
