/*******************************************************************************
* djinterp [parse]                                       parse_program_tests.hpp
*
* The program-IR suite: the canonical character class, the operand pool, and
* the instruction stream they serve.
*   The suite is a table of sections, defined in parse_program_tests.cpp beside
* the sections themselves. A section whose subject a configuration knob removes
* -- a container that owns its storage, the transport format -- is absent from
* a build without it, so the table's length is the number of sections this
* build has.
*
*
* path:      /tests/djinterp/parse/parse_program_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSE_PROGRAM_TESTS_HPP
#define DJINTERP_PARSE_PARSE_PROGRAM_TESTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error, as
// the faces it tests are. The owner's ruling: compile at every level first;
// port to C++98 only where something needs it.
#include "../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>              // std::size_t
// djinterp
#include "./c/tests_section.h"  // d_tests_section


// the program IR's sections, in the order they run
extern const d_tests_section d_tests_parse_program[];
extern const std::size_t     d_tests_parse_program_count;


#endif  // floor, for now

#endif  // DJINTERP_PARSE_PARSE_PROGRAM_TESTS_HPP
