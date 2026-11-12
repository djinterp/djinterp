/*******************************************************************************
* djinterp [tools]                                           djinterp_commands.h
*
* The djinterp program's 0.8 preview: a program on the command-line kit
* whose commands read the djins and the settings of the program's registry
* (docs/djinterp/interp/DESIGN.md, 6.4).
*
*     djinterp list                        the djins, a line each, then the
*                                          settings no djin names
*     djinterp describe DJIN               a djin's commands and settings,
*                                          or a frame's settings
*     djinterp get [--show-origin] [NAME]...
*                                          settings as statements that load
*                                          back; every one, without a NAME
*     djinterp set SETTING...              settings, each NAME=VALUE,
*                                          written to the user's file, read
*                                          as FILE's next run
*
*   The djins are the program's own, registered before its first parse;
* the 0.8 preview has none yet, so `list` names the settings the registry
* holds -- the kit's `cli` among them -- as "settings only".
*
*
* path:      /src/djinterp/tools/djinterp/djinterp_commands.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DJINTERP_COMMANDS_H
#define DJINTERP_TOOLS_DJINTERP_COMMANDS_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // dj_arg_parser
#include "../../../../inc/djinterp/tools/cli/c/cli_program.h"  // dj_cli_program


D_EXTERN_C_BEGIN

// d_djinterp_program_make
/**
 * @brief Makes the program over a parser d_arg_parser_init made: the kit's
 *        program, its djins' set over the program's registry, and its four
 *        commands.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether everything was made; on failure the program's run says
 *         why, as the kit's does.
 */
bool d_djinterp_program_make(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
);

D_EXTERN_C_END


#endif  // DJINTERP_TOOLS_DJINTERP_COMMANDS_H
