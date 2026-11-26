/*******************************************************************************
* djinterp [c][desc]                                                desc_tests.h
*
*   The suite of c/desc's core, in DTest's C face: the sink a describer emits
* facts into, what it wants, how it stops and how it nests, the machine it
* writes to, and both describer forms; then the units engine, the machines,
* and the describers of memory's, filesys's and DTest's types. The runner
* in its CMake leaf makes the suite and runs each section.
*
*
* path:      /tests/djinterp/c/desc/desc_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.23
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

/*
d_tests_desc_machines
  section: desc's text, JSON and collecting machines, and describing to a
stream as the options say, each test in the block of desc_machines.h, in the
module d_tests_desc opened; call it after d_tests_desc_units.
*/
void d_tests_desc_machines(
    struct dj_test_suite* _suite
);

/*
d_tests_desc_of_mem
  section: desc's describers of memory's types: each type against its
describer's record of it, then each describer's facts and prose, each test
in the block of desc/of/mem_desc.h, in the module d_tests_desc opened; call
it after d_tests_desc_machines.
*/
void d_tests_desc_of_mem(
    struct dj_test_suite* _suite
);

/*
d_tests_desc_of_filesys
  section: desc's describers of filesys's types, as d_tests_desc_of_mem's
are memory's, in the block of desc/of/filesys_desc.h; call it after
d_tests_desc_of_mem.
*/
void d_tests_desc_of_filesys(
    struct dj_test_suite* _suite
);

/*
d_tests_desc_of_test
  section: desc's describers of DTest's C types, in the block of
desc/of/test_desc.h; call it after d_tests_desc_of_filesys.
*/
void d_tests_desc_of_test(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_C_DESC_TESTS_H
