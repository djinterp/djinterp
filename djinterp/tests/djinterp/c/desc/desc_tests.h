/*******************************************************************************
* djinterp [test]                                                   desc_tests.h
*
*   The suite of c/desc's core, in DTest's C face: the sink a describer emits
* facts into, what it wants, how it stops and how it nests, the machine it
* writes to, and both describer forms. It has one section, its entry point,
* and the runner in its CMake leaf makes the suite and runs it.
*
*
* path:      /tests/djinterp/c/desc/desc_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DESC_TESTS_H
#define DJINTERP_TESTS_C_DESC_TESTS_H 1

// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


/*
d_tests_desc
  section: the core of desc, under its module, each test in the block of
desc.h.
*/
void d_tests_desc(
    struct dj_test_suite* _suite
);

/*
d_tests_desc_units
  section: desc's units engine, its derived facts, its djin and its macro
layer, each test in the block of desc_units.h, in the module d_tests_desc
opened; call it after d_tests_desc.
*/
void d_tests_desc_units(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_C_DESC_TESTS_H
