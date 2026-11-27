/*******************************************************************************
* djinterp [cpp][tools]                                                  cli.hpp
*
* The command-line kit's C++ umbrella: every header of the kit's C++ face,
* and so the argument parser's face and both C libraries.
*   docs/djinterp/tools/cli/GRAMMAR.html is the grammar every program the
* kit makes follows; README.html beside it is how a program is made with it.
*
* HOW TO USE
* ----------
*   Include it once for the whole kit, in C++: cli::program and the rest of the
* kit's C++ face, its streams and files, its operands, its commands that change
* a file and its plugins, with the argument parser's C++ face and both C
* libraries beneath them.
*
* OPTIONS
* -------
*   None of its own: each header it includes says its own.
*
* CONFIGURATION
* -------------
*   cfg_cli.h's knobs, as for the C kit (cli.h); D_CFG_CLI_ALL 0 builds the
* smallest kit.
*
*
* path:      /inc/djinterp/tools/cli/cli.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.11.11
*******************************************************************************/

#ifndef DJINTERP_TOOLS_CLI_HPP
#define DJINTERP_TOOLS_CLI_HPP 1

// djinterp
#include "../../cpp/modules/parse/cli/arg_parser/arg_parser.hpp"  // the argument parser's
                                                      // face
#include "./c/cli.h"                                  // the kit's C library
#include "./cli_edit.hpp"                             // commands that change
                                                      // a file
#include "./cli_io.hpp"                               // streams, terminals,
                                                      // files
#include "./cli_operand.hpp"                          // operands that name
                                                      // no file
#include "./cli_plugin.hpp"                           // modules
#include "./cli_program.hpp"                          // the program


#endif  // DJINTERP_TOOLS_CLI_HPP
