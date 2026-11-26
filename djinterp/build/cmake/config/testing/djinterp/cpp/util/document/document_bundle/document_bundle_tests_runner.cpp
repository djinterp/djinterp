/*******************************************************************************
* djinterp [cpp][utility]                       document_bundle_tests_runner.cpp
*
* The runner for cpp/util/document/document_bundle.hpp: one module spec, the
* suite's own.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/util/document/document_bundle/document_bundle_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "document_bundle_tests.hpp"  // d_tests_document_bundle_spec


/*
main
  The runner: one module spec, the suite's own.
*/
int
main(
    int   _argc,
    char* _argv[]
)
{
    const struct dj_test_module_spec* modules[1] =
        { d_tests_document_bundle_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "document_bundle unit tests",
                              modules,
                              1);
}
