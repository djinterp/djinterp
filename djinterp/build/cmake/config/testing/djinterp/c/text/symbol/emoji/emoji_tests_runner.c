/*******************************************************************************
* djinterp [c][text]                                        emoji_tests_runner.c
*
* The runner for c/text/symbol/emoji/emoji.h: one module spec, the suite's own.
*
*
* path:      /build/cmake/config/testing/djinterp/c/text/symbol/emoji/emoji_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "emoji_tests.h"  // d_tests_emoji_spec


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
        { d_tests_emoji_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "emoji unit tests",
                              modules,
                              1);
}
