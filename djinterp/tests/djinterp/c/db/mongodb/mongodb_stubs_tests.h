/*******************************************************************************
* djinterp [test]                                          mongodb_stubs_tests.h
*
*   The suite of c/db/mongodb: the layer built without its client, in DTest's C
* face. It has one section, its entry point, and the runner in its CMake leaf
* makes the suite and runs it.
*
*
* path:      /tests/djinterp/c/db/mongodb/mongodb_stubs_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DB_MONGODB_STUBS_TESTS_H
#define DJINTERP_TESTS_C_DB_MONGODB_STUBS_TESTS_H 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


/*
d_tests_mongodb_stubs
  section: every test of the suite, under its module, each test in the
block of the header it tests.
*/
void d_tests_mongodb_stubs(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_C_DB_MONGODB_STUBS_TESTS_H
