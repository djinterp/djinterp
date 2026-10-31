/*******************************************************************************
* djinterp [tools]                                                         cli.h
*
* The command-line kit's C umbrella: every header of the kit, and so the
* argument parser's.
*   GRAMMAR.md, beside the C++ faces, is the grammar every program the kit
* makes follows; README.md is how a program is made with it.
*
*
* path:      /inc/djinterp/tools/cli/c/cli.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.25
*******************************************************************************/

#ifndef DJINTERP_TOOLS_CLI_C_CLI_H
#define DJINTERP_TOOLS_CLI_C_CLI_H 1

// djinterp
#include "../../../c/modules/parse/cli/arg_parser/arg_parser.h"  // the argument parser
#include "./cli_edit.h"                                  // commands that change
                                                         // a file
#include "./cli_format.h"                                // help's layout
#include "./cli_io.h"                                    // streams, terminals,
                                                         // files
#include "./cli_operand.h"                               // operands that name
                                                         // no file
#include "./cli_plugin.h"                                // modules
#include "./cli_program.h"                               // the program


#endif  // DJINTERP_TOOLS_CLI_C_CLI_H
