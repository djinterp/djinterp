/*******************************************************************************
* djinterp [test]                                          mariadb_sizes_tests.h
*
*   The suite of c/db/mariadb's checked sizes, in DTest's C face: every size
* the layer computes for an allocation from a count it keeps, given a count
* too large. It has one section, its entry point, and the runner in its CMake
* leaf makes the suite and runs it.
*
*
* path:      /tests/djinterp/c/db/mariadb/mariadb_sizes_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DB_MARIADB_SIZES_TESTS_H
#define DJINTERP_TESTS_C_DB_MARIADB_SIZES_TESTS_H 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


/*
d_tests_mariadb_sizes
  section: every checked size, under its module, each test in the block of
the unit it tests.
*/
void d_tests_mariadb_sizes(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_C_DB_MARIADB_SIZES_TESTS_H
