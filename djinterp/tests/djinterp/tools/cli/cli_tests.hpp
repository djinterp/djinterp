/*******************************************************************************
* djinterp [tools]                                                 cli_tests.hpp
*
* The command-line kit's C++ suite: the face of cli_program.hpp, cli_io.hpp,
* cli_edit.hpp, cli_operand.hpp and cli_plugin.hpp, called from C++ across a
* real language boundary.
*   The C suites hold the kit's behaviour -- every message, page and file;
* this one holds the face's way to it: a program made and released with the
* parser it is; a command's work as any function object -- a functor at
* every level, a lambda from C++11 -- kept and freed with the program; the
* run and its halves; output to sinks and to streams; the reports with a
* message given whole, `%` and all; an exception a command's work lets out,
* said as the program's error where exceptions are on; a module's program
* made from its host; the files read and written with std::string, and a
* named operand read; a command that changes a file, its operand bound to a
* std::string, by each rule; a module found, loaded and refused through
* DPluginModule; and the build each program carries from its constructor,
* and the lines it adds.
*   The suite is a table of sections, defined in cli_tests.cpp beside the
* sections themselves: the lambdas' from C++11, and the exceptions' where
* the face catches them.
*
*
* path:      /tests/djinterp/tools/cli/cli_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.25
*******************************************************************************/

#ifndef DJINTERP_TOOLS_CLI_CLI_TESTS_HPP
#define DJINTERP_TOOLS_CLI_CLI_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../inc/djinterp/env/env.h"  // language levels
#include "../../../../inc/djinterp/config/c/modules/parse/cli/arg_parser/cfg_arg_parser.h"  // D_INTERNAL_ARG_PARSER_CPP_CATCH
#include "../../c/modules/parse/tests_section.h"     // dj_tests_section


// the directory the probe modules were built in; NULL for the runner's own
void d_tests_cli_cpp_modules(
    const char* _directory
);

// 1.  THE PROGRAM
bool d_tests_cli_cpp_program();
bool d_tests_cli_cpp_halves();
bool d_tests_cli_cpp_reports();

#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)
// 2.  EXCEPTIONS
bool d_tests_cli_cpp_exceptions();
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// 3.  LAMBDAS
bool d_tests_cli_cpp_lambdas();
#endif

// 4.  A MODULE'S PROGRAM
bool d_tests_cli_cpp_module_side();

// 5.  FILES
bool d_tests_cli_cpp_files();

// 6.  EDITS
bool d_tests_cli_cpp_edits();

// 7.  PLUGINS
bool d_tests_cli_cpp_plugins();

// 8.  THE VERSION
bool d_tests_cli_cpp_version();

// the suite's sections, in the order they run
extern const dj_tests_section d_tests_cli_cpp[];
extern const std::size_t     d_tests_cli_cpp_count;


#endif  // DJINTERP_TOOLS_CLI_CLI_TESTS_HPP
