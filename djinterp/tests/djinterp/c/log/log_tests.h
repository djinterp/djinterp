/*******************************************************************************
* djinterp [test]                                                    log_tests.h
*
*   The suite of c/log, in DTest's C face: levels, codes and their keys, the
* threshold and the grades, the body, the three writers, the caps, the
* destinations, a program's writer, and times. It has one section, its entry
* point, and the runner in its CMake leaf makes the suite and runs it.
*
*
* path:      /tests/djinterp/c/log/log_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_LOG_TESTS_H
#define DJINTERP_TESTS_C_LOG_TESTS_H 1

// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


/*
d_tests_log
  section: the log, under its module, each test in the block of log.h.
*/
void d_tests_log(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_C_LOG_TESTS_H
