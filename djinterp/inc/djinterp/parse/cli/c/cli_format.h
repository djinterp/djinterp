/*******************************************************************************
* djinterp [parse]                                                  cli_format.h
*
* Help as GNU programs lay it out, for the argument parser's formatter.
*   The layout (GRAMMAR.md, section 6):
*
*     Usage: bitwisedom gen [OPTION]... SHEET
*     Write a sheet's header.
*
*     Arguments:
*       SHEET                the sheet: a sheet file, a bitfield, an export
*
*     Options:
*       -o, --out=FILE       the header's file (default: standard output)
*           --level=LEVEL    the language level
*       -h, --help           display this help and exit
*
*     Global options:
*       -q, --quiet          say nothing but errors
*
* -- the usage line first, built from the command's path, its options, its
* subcommands and its positionals, or the command's own usage where it has
* one; the description as a sentence; then the positionals, the
* subcommands, each option group under its own heading, and, for a
* subcommand, the root's options the subcommand also takes; and the footer
* last. An option is its short names, then its long names, the value it
* takes after `=` -- its type name, or VALUE -- in brackets when the value may
* be left out (R7), and its description, with its default and the words a
* value may be appended. Descriptions start where the widest left column
* needs, never past D_INTERNAL_CLI_HELP_COLUMN_MAX, and every line is wrapped
* to the terminal's width, at most D_INTERNAL_CLI_HELP_WIDTH_MAX, or to
* D_INTERNAL_CLI_HELP_WIDTH off a terminal.
*   What help hides stays hidden as the parser hides it: an option or a
* subcommand in the group named "", and a disabled subcommand.
*
*
* path:      /inc/djinterp/parse/cli/c/cli_format.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE FORMATTER
    -------------
    1.  Steps
         1.  d_cli_formatter_steps
    2.  Making one
         1.  d_cli_formatter_use
    3.  Widths
         1.  d_cli_help_width
*/

#ifndef DJINTERP_PARSE_CLI_C_CLI_FORMAT_H
#define DJINTERP_PARSE_CLI_C_CLI_FORMAT_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../c/djinterp.h"                  // framework root
#include "../../../config/parse/cli/cfg_cli.h"    // D_INTERNAL_CLI_FORMATTER
#include "../arg_parser/c/arg_formatter.h"        // d_arg_formatter,
                                                  // d_arg_formatter_vtable


D_EXTERN_C_BEGIN

//==============================================================================
// 1.  THE FORMATTER
//==============================================================================


#if (D_INTERNAL_CLI_FORMATTER == 1)

// 1.1    Steps
//------------------------------------------------------------------------------
// 1.1.1
// d_cli_formatter_steps
//   constant: the kit's steps: make_help builds the whole page, so no other
// step of the parser's is called while it is in use.
extern const struct d_arg_formatter_vtable d_cli_formatter_steps;

#endif  // D_INTERNAL_CLI_FORMATTER

// 1.2    Making one
//------------------------------------------------------------------------------
// 1.2.1
// d_cli_formatter_use
//   setting: makes a formatter lay help out as the kit does -- its steps,
// d_cli_formatter_steps -- or, without D_INTERNAL_CLI_FORMATTER, leaves it the
// parser's own. The formatter's labels and widths are kept. Ignores NULL;
// does not fail.
void        d_cli_formatter_use(struct d_arg_formatter* _formatter);

// 1.3    Widths
//------------------------------------------------------------------------------
// 1.3.1
// d_cli_help_width
//   query: the columns help is wrapped to now: standard output's terminal's,
// at most D_INTERNAL_CLI_HELP_WIDTH_MAX and at least 40, when standard
// output is a terminal; D_INTERNAL_CLI_HELP_WIDTH otherwise. Does not fail.
size_t      d_cli_help_width(void);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_C_CLI_FORMAT_H
