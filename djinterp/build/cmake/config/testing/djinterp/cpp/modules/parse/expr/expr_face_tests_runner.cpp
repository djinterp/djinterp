/*******************************************************************************
* djinterp [cpp/parse]                                expr_face_tests_runner.cpp
*
* The runner for expression core v2's C++ face suite: its options and its
* spec.
*   It defines DTEST_SPEC_MODE before the suite header, which then gives it
* the tests and the spec provider (test_suite.h 3.2).
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/modules/parse/expr/expr_face_tests_runner.cpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "./expr_face_tests.hpp"  // d_tests_parse_expr_face_spec


/*
main
  The runner: the spec's one module, run as the command line asks.
*/
int
main(
    int   _argc,
    char* _argv[]
)
{
    const struct dj_test_module_spec* modules[1];

    modules[0] = d_tests_parse_expr_face_spec();

    return d_test_run_modules(_argc, _argv,
                              "cpp/modules/parse/expr/: the C++ face",
                              modules, 1u);
}
