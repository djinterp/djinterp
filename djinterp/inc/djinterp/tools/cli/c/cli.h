/*******************************************************************************
* djinterp [c][tools]                                                      cli.h
*
* The command-line kit's C umbrella: every header of the kit, and so the
* argument parser's.
*   docs/djinterp/tools/cli/GRAMMAR.html is the grammar every program the
* kit makes follows; README.html beside it is how a program is made with it.
*
* HOW TO USE
* ----------
*   Include it once for the whole kit, in C: the program, its settings, its
* streams and files, its operands, its commands that change a file, its help,
* and its plugins, with the argument parser beneath them.
*
* OPTIONS
* -------
*   None of its own: each header it includes says its own.
*
* CONFIGURATION
* -------------
*   cfg_cli.h's knobs, read by the headers it includes; D_CFG_CLI_ALL 0 builds
* the smallest kit.
*
*
* path:      /inc/djinterp/tools/cli/c/cli.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.11.21
*******************************************************************************/

#ifndef DJINTERP_TOOLS_CLI_C_CLI_H
#define DJINTERP_TOOLS_CLI_C_CLI_H 1

// djinterp
#include "../../../c/modules/parse/cli/arg_parser/arg_parser.h"  // the argument parser
#include "./cli_edit.h"                                  // commands that change
                                                         // a file
#include "./cli_format.h"                                // help's layout
#include "./cli_help.h"                                  // the description
#include "./cli_io.h"                                    // streams, terminals,
                                                         // files
#include "./cli_operand.h"                               // operands that name
                                                         // no file
#include "./cli_plugin.h"                                // modules
#include "./cli_program.h"                               // the program
#include "./cli_settings.h"                              // the settings


#endif  // DJINTERP_TOOLS_CLI_C_CLI_H
