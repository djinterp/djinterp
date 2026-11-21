/*******************************************************************************
* djinterp [tools]                                          cli_c_sweep_runner.c
*
* Entry point for the command-line kit's allocation sweep.
*   Built as C and linked against the kit and the argument parser with
* malloc, calloc, realloc and free wrapped (-Wl,--wrap=malloc and kin), so
* that the sweep's wrappers stand in front of every allocation either makes;
* a linker without the switch cannot build it, and the leaf leaves it out
* there. DTest's C face is linked with them, and its own allocations pass
* the wrappers untouched: they are made only while no run is measured. The
* sweep is one block of DTest's spec form, run as the module `cli_sweep`
* through d_test_run_modules.
*   NO_COLOR is set before the first test, as the C runner sets it. The
* first argument, when it is no option, is the directory the probe modules
* were built in, which the plugin cases load them from; every other is
* DTest's.
*
*
* path:      /build/cmake/config/testing/djinterp/tools/cli/cli_c_sweep_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.18
*******************************************************************************/
// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_sweep_tests.h"  // the sweep
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_tests_support.h"  // d_tests_cli_setenv


/*
main
  Runs the sweep's sections, as one module of one block.
*/
int main(
    int    _argc,
    char** _argv
)
{
    const struct dj_test_block_spec blocks[] =
    {
        d_tests_cli_sweep_block
    };
    const struct dj_test_module_spec module =
    {
        "cli_sweep",
        "the command-line kit with every allocation refused in turn",
        blocks,
        sizeof(blocks) / sizeof(blocks[0])
    };
    const struct dj_test_module_spec* const modules[] = { &module };
    const char* const                       probes    =
        d_tests_cli_take_modules(&_argc,
                                 _argv);

    // nothing coloured unless a case asks; the probe modules' directory
    (void)d_tests_cli_setenv("NO_COLOR",
                             "1");
    d_tests_cli_sweep_modules(probes);

    return d_test_run_modules(_argc,
                              _argv,
                              "djinterp command-line kit: allocation sweep",
                              modules,
                              1u);
}
