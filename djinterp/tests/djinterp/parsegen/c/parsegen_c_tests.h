/*******************************************************************************
* djinterp [parsegen]                                         parsegen_c_tests.h
*
* Parsegen's C suite: its C API, called from C.
*   Building a grammar, rendering it, analysing it, and routing it through the
* registry -- the pipeline the C++ suites cover, from a C translation unit.
*   The suite is a table of sections, defined in parsegen_c_tests.c beside the
* sections themselves, so its length is the number of sections this build has.
*
*
* path:      /tests/djinterp/parsegen/c/parsegen_c_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_C_PARSEGEN_C_TESTS_H
#define DJINTERP_PARSEGEN_C_PARSEGEN_C_TESTS_H 1

// std
#include <stddef.h>                       // size_t
// djinterp
#include "../../parse/c/tests_section.h"  // d_tests_section


// the C suite's sections, in the order they run
extern const struct d_tests_section d_tests_parsegen_c[];
extern const size_t                 d_tests_parsegen_c_count;


#endif  // DJINTERP_PARSEGEN_C_PARSEGEN_C_TESTS_H
