/*******************************************************************************
* djinterp [parse]                                               tests_section.h
*
* One named test section, as the parse and parsegen suites list them.
*   A suite is a table of these, defined beside the sections themselves. That
* is deliberate: a section exists from the language level, and under the
* configuration, that its subject exists at, so which sections a build has is
* decided in one place -- the suite's own source, at namespace scope -- and a
* runner walks whatever table it is handed without a condition of its own.
*   It is a plain struct with no behaviour, shared by the C suites and the C++
* ones, and it is not a test framework: DTest is being rewritten, and these
* suites need only a name and a function until it lands.
*
*
* path:      /tests/djinterp/parse/c/tests_section.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_PARSE_C_TESTS_SECTION_H
#define DJINTERP_PARSE_C_TESTS_SECTION_H 1

// std
#include <stdbool.h>  // bool


// d_tests_section
//   struct: one test section -- the name a report prints, and the function
// that runs it and says whether it held.
struct d_tests_section
{
    const char* name;
    bool      (*run)(void);
};


#endif  // DJINTERP_PARSE_C_TESTS_SECTION_H
