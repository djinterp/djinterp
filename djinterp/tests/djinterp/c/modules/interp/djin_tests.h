/*******************************************************************************
* djinterp [test]                                                   djin_tests.h
*
*   The suite of modules/interp/djin.h, on DTest's C face: the set's checks
* of a djin's record, its registration of the djin's settings in a registry
* of the suite's own, and the text the program's `list` and `describe` write.
* Each test makes its own registry and frames (djin_tests_set.c), so no test
* sees another's djins.
*
*
* path:      /tests/djinterp/c/modules/interp/djin_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/

#ifndef DJINTERP_TESTS_INTERP_DJIN_TESTS_H
#define DJINTERP_TESTS_INTERP_DJIN_TESTS_H 1

// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"                // framework root
#include "../../../../../inc/djinterp/c/modules/interp/djin.h"     // under test
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h" // DTest's C face


/******************************************************************************
 * I. THE SET
 *****************************************************************************/

bool d_tests_djin_set_init(
    struct dj_test_suite* _suite
);

bool d_tests_djin_register(
    struct dj_test_suite* _suite
);

bool d_tests_djin_register_refused(
    struct dj_test_suite* _suite
);

bool d_tests_djin_register_settings(
    struct dj_test_suite* _suite
);

bool d_tests_djin_find(
    struct dj_test_suite* _suite
);

// I.   aggregation function
bool d_tests_djin_set_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. AS TEXT
 *****************************************************************************/

bool d_tests_djin_write_list(
    struct dj_test_suite* _suite
);

bool d_tests_djin_write_description(
    struct dj_test_suite* _suite
);

bool d_tests_djin_write_long(
    struct dj_test_suite* _suite
);

bool d_tests_djin_write_refused(
    struct dj_test_suite* _suite
);

// II.  aggregation function
bool d_tests_djin_text_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/

bool d_tests_djin_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_INTERP_DJIN_TESTS_H
