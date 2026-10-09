/*******************************************************************************
* djinterp [parse]                                               parse_c_tests.h
*
* The parse substrate's C suite: its C API, called from C.
*   The C++ suites exercise this API through its C++ faces. These sections call
* it from a C translation unit, which is the only way to show at run time, and
* not just at compile time, that the foundation is usable from C on its own.
*   The suite is a table of sections, defined in parse_c_tests.c beside the
* sections themselves, so its length is the number of sections this build has.
*
*
* path:      /tests/djinterp/parse/c/parse_c_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_PARSE_C_PARSE_C_TESTS_H
#define DJINTERP_PARSE_C_PARSE_C_TESTS_H 1

// std
#include <stddef.h>           // size_t
// djinterp
#include "./tests_section.h"  // d_tests_section


// the C suite's sections, in the order they run
extern const struct d_tests_section d_tests_parse_c[];
extern const size_t                 d_tests_parse_c_count;


#endif  // DJINTERP_PARSE_C_PARSE_C_TESTS_H
