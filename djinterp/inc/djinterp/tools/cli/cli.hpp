/*******************************************************************************
* djinterp [tools]                                                       cli.hpp
*
* The command-line kit's C++ umbrella: every header of the kit's C++ face,
* and so the argument parser's face and both C libraries.
*   GRAMMAR.md, beside this file, is the grammar every program the kit makes
* follows; README.md is how a program is made with it.
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
