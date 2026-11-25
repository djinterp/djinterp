/*******************************************************************************
* djinterp [cpp][interp]                                      skeleton_tests.hpp
*
* The suite of the interpreter's skeleton, its second step (language.hpp,
* evaluate.hpp): a line and a command line read into their terms - words,
* quotes, comments, separators, brackets, the keywords, the comparisons and
* literals, by precedence - and what each refuses; the value's stand-in;
* sequences and pipelines run, a pipeline ended by its first stage in
* trouble, its later stages not run; `and` and `or` short-circuiting by
* truth, `not`, and the comparisons by number, by text and by structure.
*
* HOW TO USE
* ----------
*   The leaf's runner hands d_tests_interp_skeleton_module to
* d_test_run_modules; each section can be run alone, as a
* dj_test_case_spec's function.
*
* OPTIONS
* -------
*   None: a section takes no arguments.
*
* CONFIGURATION
* -------------
*   C++11, the skeleton's floor: below it the module has no blocks.
*
*
* path:      /tests/djinterp/cpp/modules/interp/skeleton_tests.hpp
* link(s):   /docs/djinterp/interp/DESIGN.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  READING A LINE
2.  READING A COMMAND LINE
3.  VALUES
4.  RUNNING
*/

#ifndef DJINTERP_INTERP_SKELETON_TESTS_HPP
#define DJINTERP_INTERP_SKELETON_TESTS_HPP 1

// djinterp
#include "../../../../../inc/djinterp/env/env.h"  // language levels
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // specs


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// 1.  READING A LINE
bool d_tests_interp_read_terms();
bool d_tests_interp_read_words();
bool d_tests_interp_read_refused();
bool d_tests_interp_read_logic();
// 2.  READING A COMMAND LINE
bool d_tests_interp_read_argv();
// 3.  VALUES
bool d_tests_interp_values();
// 4.  RUNNING
bool d_tests_interp_run_sequences();
bool d_tests_interp_run_pipelines();
bool d_tests_interp_run_trouble();
bool d_tests_interp_run_logic();
bool d_tests_interp_run_compare();

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// the suite as a module of the C face's spec form: no blocks below C++11
extern const struct dj_test_module_spec d_tests_interp_skeleton_module;


#endif  // DJINTERP_INTERP_SKELETON_TESTS_HPP
