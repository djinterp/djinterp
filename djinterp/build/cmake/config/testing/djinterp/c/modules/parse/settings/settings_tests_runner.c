/*******************************************************************************
* djinterp [c/parse]                                     settings_tests_runner.c
*
* The runner for parse's settings suite: its options and its spec.
*   It defines DTEST_SPEC_MODE before the suite header, which then gives it
* the tests and the spec provider (test_suite.h 3.2).
*
*
* path:      /build/cmake/config/testing/djinterp/c/modules/parse/settings/settings_tests_runner.c
* link(s):   /docs/djinterp/option/knob_and_option.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "./settings_tests.h"  // d_tests_parse_settings_spec


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

    modules[0] = d_tests_parse_settings_spec();

    return d_test_run_modules(_argc, _argv,
                              "parse/settings.h: the djin parse",
                              modules, 1u);
}
