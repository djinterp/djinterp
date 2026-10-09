/*******************************************************************************
* djinterp [parsegen]                                 parsegen_grammar_tests.hpp
*
* The neutral-grammar suite: the tree a frontend builds, repetition as bounds,
* the two kinds of choice, capability accumulation, and canonical rendering.
*   The suite is a table of sections, defined in parsegen_grammar_tests.cpp
* beside the sections themselves, so the table's length is the number of
* sections this build has and a runner reads it rather than assuming one.
*
*
* path:      /tests/djinterp/parsegen/parsegen_grammar_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_PARSEGEN_GRAMMAR_TESTS_HPP
#define DJINTERP_PARSEGEN_PARSEGEN_GRAMMAR_TESTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error, as
// the faces it tests are. The owner's ruling: compile at every level first;
// port to C++98 only where something needs it.
#include "../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>                     // std::size_t
// djinterp
#include "../parse/c/tests_section.h"  // d_tests_section


// the suite's sections, in the order they run
extern const d_tests_section d_tests_parsegen_grammar[];
extern const std::size_t     d_tests_parsegen_grammar_count;


#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_PARSEGEN_GRAMMAR_TESTS_HPP
