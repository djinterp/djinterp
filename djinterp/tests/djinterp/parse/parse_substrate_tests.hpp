/*******************************************************************************
* djinterp [parse]                                     parse_substrate_tests.hpp
*
* The execution-substrate suite: the diagnostic channel, the machine and its
* operator registry, and the claim that the C++ faces and the C types are the
* same objects.
*   The suite is a table of sections, defined in parse_substrate_tests.cpp
* beside the sections themselves. A registration shape that exists only from
* C++17 or C++20 has a section only from that level, so the table's length is
* the number of sections this build has, and a runner reads it rather than
* assuming one.
*
*
* path:      /tests/djinterp/parse/parse_substrate_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSE_SUBSTRATE_TESTS_HPP
#define DJINTERP_PARSE_PARSE_SUBSTRATE_TESTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error, as
// the faces it tests are. The owner's ruling: compile at every level first;
// port to C++98 only where something needs it.
#include "../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>              // std::size_t
// djinterp
#include "./c/tests_section.h"  // d_tests_section


// the substrate's sections, in the order they run
extern const d_tests_section d_tests_parse_substrate[];
extern const std::size_t     d_tests_parse_substrate_count;


#endif  // floor, for now

#endif  // DJINTERP_PARSE_PARSE_SUBSTRATE_TESTS_HPP
