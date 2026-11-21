/*******************************************************************************
* djinterp [tools]                                          cli_c_tests_runner.c
*
* Entry point for the command-line kit's C suites.
*   Built as C and linked against the kit and the argument parser, built as
* C at the C floor, and DTest's C face. Each suite is a block of DTest's
* spec form that its own source defines, and this runs them as one module,
* `cli`, through d_test_run_modules: which sections a configuration has is
* decided where the sections are, so the count of tests differs by
* configuration, by design.
*   NO_COLOR is set before the first test, so that a program's `auto`
* colours nothing even when the runner writes to a terminal. The first
* argument, when it is no option, is the directory the probe modules were
* built in, which the plugin suite loads them from; every other is DTest's
* (--failures, --tests, --checks, --no-table, --filter=TEXT, --help).
*
*
* path:      /build/cmake/config/testing/djinterp/tools/cli/cli_c_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.18
*******************************************************************************/
// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_edit_tests.h"  // commands that change a file
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_format_tests.h"  // help
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_io_tests.h"  // streams and files
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_operand_tests.h"  // operands that name no file
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_plugin_tests.h"  // modules
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_program_tests.h"  // the program
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_settings_tests.h"  // settings
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_tests_support.h"  // d_tests_cli_setenv


/*
main
  Runs every C suite, as one module of a block each, in the order they
depend on one another: the streams and files the others write through, the
help the program lays out, the program, its commands that change a file,
its operands that name no file, the modules it loads, then the settings it
keeps.
*/
int main(
    int    _argc,
    char** _argv
)
{
    const struct dj_test_block_spec blocks[] =
    {
        d_tests_cli_io_block,
        d_tests_cli_format_block,
        d_tests_cli_program_block,
        d_tests_cli_edit_block,
        d_tests_cli_operand_block,
        d_tests_cli_plugin_block,
        d_tests_cli_settings_block
    };
    const struct dj_test_module_spec module =
    {
        "cli",
        "the command-line kit, called from C",
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
    d_tests_cli_plugin_modules(probes);

    return d_test_run_modules(_argc,
                              _argv,
                              "djinterp command-line kit: C tests",
                              modules,
                              1u);
}
