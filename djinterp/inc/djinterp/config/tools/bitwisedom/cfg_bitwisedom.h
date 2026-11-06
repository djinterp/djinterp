/*******************************************************************************
* djinterp [config]                                             cfg_bitwisedom.h
*
* Configuration of bitwisedom, the bitfield manager on djinterp's kit.
*   What its command line takes as a setting's KEY: the names the window's
* Options show and the settings registry holds (`gen.level`), the paths a
* sheet file holds (`generator.level`), or either (ruling cli-taken 16). And
* what `bitwisedom gui` does with a SHEET the window cannot open: hands it
* over with the rest, says so on standard error and ends in trouble when the
* window closes, or refuses it before any window opens (ruling cli-taken
* 25). Each knob sets a default, which an option overrides for a run:
* `--keys=WHICH` on `set` and `get`, `--sheets=WHEN` on `gui`.
*   targets:  the command line's `set` and `get` (cli_rows.cpp), through
*             D_INTERNAL_BITWISEDOM_KEYS; the window's module (plugin.cpp,
*             built in build/cmake/config/ui), through
*             D_INTERNAL_BITWISEDOM_GUI_SHEETS
*   requires: cfg_common.h
*
*
* path:      /inc/djinterp/config/tools/bitwisedom/cfg_bitwisedom.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Settings' keys
         1.  D_CFG_BITWISEDOM_KEYS_*
         2.  D_CFG_BITWISEDOM_KEYS
    2.  The window's sheets
         1.  D_CFG_BITWISEDOM_GUI_SHEETS_*
         2.  D_CFG_BITWISEDOM_GUI_SHEETS
2.  VALIDATION
    ----------
3.  RESOLVED VALUES
    ---------------
    1.  D_INTERNAL_BITWISEDOM_KEYS
    2.  D_INTERNAL_BITWISEDOM_GUI_SHEETS
*/

#ifndef DJINTERP_CONFIG_TOOLS_BITWISEDOM_CFG_BITWISEDOM_H
#define DJINTERP_CONFIG_TOOLS_BITWISEDOM_CFG_BITWISEDOM_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_NORM


//==============================================================================
// 1.  KNOBS
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier -- by -D, a prefix
// header, or cfg_custom.h -- wins.


// 1.1    Settings' keys
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_BITWISEDOM_KEYS_*
//   constant: the values of D_CFG_BITWISEDOM_KEYS, as bits: a KEY may be a
// registry name, a path of the sheet file, or either. Numbering starts at 1:
// D_CFG_NORM reads an empty or misspelled value as 0, which is none of them,
// and stops the build.

// 1.1.1.1
// D_CFG_BITWISEDOM_KEYS_NAMES
//   constant: the registry's names alone, `gen.level`.
#define D_CFG_BITWISEDOM_KEYS_NAMES 1

// 1.1.1.2
// D_CFG_BITWISEDOM_KEYS_PATHS
//   constant: the sheet file's paths alone, `generator.level`.
#define D_CFG_BITWISEDOM_KEYS_PATHS 2

// 1.1.1.3
// D_CFG_BITWISEDOM_KEYS_BOTH
//   constant: a registry name, and for anything the registry does not list,
// the sheet file's path.
#define D_CFG_BITWISEDOM_KEYS_BOTH  3

// 1.1.2
// D_CFG_BITWISEDOM_KEYS
//   knob: what `bitwisedom set` and `get` take as a KEY when `--keys` does
// not say, one of D_CFG_BITWISEDOM_KEYS_*. Default BOTH: one vocabulary, the
// window's, for everything it lists, and the file's for the rest -- a sheet's
// probe, a row's fields -- so that nothing a file holds is out of a script's
// reach.
#ifndef D_CFG_BITWISEDOM_KEYS
    #define D_CFG_BITWISEDOM_KEYS D_CFG_BITWISEDOM_KEYS_BOTH
#endif  // D_CFG_BITWISEDOM_KEYS

// 1.2    The window's sheets
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_BITWISEDOM_GUI_SHEETS_*
//   constant: the values of D_CFG_BITWISEDOM_GUI_SHEETS. Numbering starts at
// 1, as 1.1.1's does.

// 1.2.1.1
// D_CFG_BITWISEDOM_GUI_SHEETS_REFUSE
//   constant: a SHEET the window cannot open is refused, in the commands'
// words, before any window opens, and the run ends in trouble (A).
#define D_CFG_BITWISEDOM_GUI_SHEETS_REFUSE 1

// 1.2.1.2
// D_CFG_BITWISEDOM_GUI_SHEETS_REPORT
//   constant: every SHEET is handed to the window, which says in its status
// bar what it cannot open; the program says so too, on standard error, as
// the window starts, and the run ends in trouble when it closes (B).
#define D_CFG_BITWISEDOM_GUI_SHEETS_REPORT 2

// 1.2.2
// D_CFG_BITWISEDOM_GUI_SHEETS
//   knob: what `bitwisedom gui` does with a SHEET the window cannot open
// when `--sheets` does not say, one of D_CFG_BITWISEDOM_GUI_SHEETS_*.
// Default REPORT: the window opens on what it can, and a script still sees
// the 2 a typo earns.
#ifndef D_CFG_BITWISEDOM_GUI_SHEETS
    #define D_CFG_BITWISEDOM_GUI_SHEETS D_CFG_BITWISEDOM_GUI_SHEETS_REPORT
#endif  // D_CFG_BITWISEDOM_GUI_SHEETS


//==============================================================================
// 2.  VALIDATION
//==============================================================================

#if ( (D_CFG_NORM(D_CFG_BITWISEDOM_KEYS) < D_CFG_BITWISEDOM_KEYS_NAMES) ||     \
      (D_CFG_NORM(D_CFG_BITWISEDOM_KEYS) > D_CFG_BITWISEDOM_KEYS_BOTH) )
    #error "cfg_bitwisedom: D_CFG_BITWISEDOM_KEYS must be one of \
D_CFG_BITWISEDOM_KEYS_NAMES, _PATHS or _BOTH"
#endif

#if ( (D_CFG_NORM(D_CFG_BITWISEDOM_GUI_SHEETS) <                               \
       D_CFG_BITWISEDOM_GUI_SHEETS_REFUSE) ||                                  \
      (D_CFG_NORM(D_CFG_BITWISEDOM_GUI_SHEETS) >                               \
       D_CFG_BITWISEDOM_GUI_SHEETS_REPORT) )
    #error "cfg_bitwisedom: D_CFG_BITWISEDOM_GUI_SHEETS must be \
D_CFG_BITWISEDOM_GUI_SHEETS_REFUSE or _REPORT"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================

// 3.1.1
// D_INTERNAL_BITWISEDOM_KEYS
//   resolved: what a KEY may be, as D_CFG_BITWISEDOM_KEYS_* bits.
#define D_INTERNAL_BITWISEDOM_KEYS                                             \
    D_CFG_NORM(D_CFG_BITWISEDOM_KEYS)

// 3.1.2
// D_INTERNAL_BITWISEDOM_GUI_SHEETS
//   resolved: what the window's run does with a SHEET it cannot open, one of
// D_CFG_BITWISEDOM_GUI_SHEETS_*.
#define D_INTERNAL_BITWISEDOM_GUI_SHEETS                                       \
    D_CFG_NORM(D_CFG_BITWISEDOM_GUI_SHEETS)


#endif  // DJINTERP_CONFIG_TOOLS_BITWISEDOM_CFG_BITWISEDOM_H
