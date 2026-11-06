/*******************************************************************************
* djinterp [config]                                            cfg_env_printer.h
*
* Configuration for env_printer.h and env_printer.hpp, the printers of the
* detected environment.
*   Resolves what a unit's report compiles in, the defaults the printer's
* schema pins for its `style` and `sections` variables, and the room a
* printer has: for bindings, variables, text and its compiled templates. The
* defaults are compile-time; a program changes the variables at run time
* (env_printer.h, 2.6), and the program's settings reach every printer made
* in the djin (env_printer.h, 2.5). It sits beside the other env
* configurations, under config/env.
*   targets:  env/env_printer.h -> D_CFG_ENV_PRINTER_CPP_FEATURES,
*                                  D_CFG_ENV_PRINTER_BINDINGS,
*                                  D_CFG_ENV_PRINTER_VARIABLES,
*                                  D_CFG_ENV_PRINTER_TEXT,
*                                  D_CFG_ENV_PRINTER_TEMPLATE_OPS
*             env/env_printer.c -> D_CFG_ENV_PRINTER_SECTIONS,
*                                  D_CFG_ENV_PRINTER_STYLE
*   requires: cfg_common.h
*
*   THE OLD NAMES STILL WORK. env_printer.hpp's D_ENV_PRINTER_INCLUDE_FEATURES
* and D_ENV_PRINTER_INCLUDE_C_FEATURES, defined in the module itself before,
* are read here as the defaults of the knobs that replace them, so a build
* that set either prints what it printed before.
*
*
* path:      /inc/djinterp/config/env/cfg_env_printer.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  What is compiled in
         1.  D_CFG_ENV_PRINTER_CPP_FEATURES
    2.  What is printed by default
         1.  D_CFG_ENV_PRINTER_SECTIONS
         2.  D_CFG_ENV_PRINTER_STYLE
    3.  A printer's room
         1.  D_CFG_ENV_PRINTER_BINDINGS
         2.  D_CFG_ENV_PRINTER_VARIABLES
         3.  D_CFG_ENV_PRINTER_TEXT
         4.  D_CFG_ENV_PRINTER_TEMPLATE_OPS
2.  VALIDATION
    ----------
    1.  Ranges
*/

#ifndef DJINTERP_CONFIG_ENV_CFG_ENV_PRINTER_H
#define DJINTERP_CONFIG_ENV_CFG_ENV_PRINTER_H 1

// djinterp
#include "../cfg_common.h"  // D_CFG_IS_ON, D_CFG_NORM, user overrides


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    What is compiled in
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_PRINTER_CPP_FEATURES
//   configuration: 1 compiles the table of every C++ feature-test result (161
// entries, about 20 KB of constant data) into each C++ unit that collects the
// environment, so the D_ENV_SECTION_CPP_FEATURES section has something to
// print; 0 leaves it out, and that section prints as empty. Defaults to the
// old D_ENV_PRINTER_INCLUDE_FEATURES where a build defines it, to 0 otherwise.
#ifndef D_CFG_ENV_PRINTER_CPP_FEATURES
    #if defined(D_ENV_PRINTER_INCLUDE_FEATURES)
        #define D_CFG_ENV_PRINTER_CPP_FEATURES                                 \
            D_CFG_NORM(D_ENV_PRINTER_INCLUDE_FEATURES)
    #else
        #define D_CFG_ENV_PRINTER_CPP_FEATURES 0
    #endif
#endif  // D_CFG_ENV_PRINTER_CPP_FEATURES

// 1.2    What is printed by default
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_ENV_PRINTER_SECTIONS
//   configuration: the D_ENV_SECTION_* bits the printer's `sections` variable
// holds until a program sets it. Defaults to the seven core sections (0x007F),
// with the C runtime's (0x0080) where the old D_ENV_PRINTER_INCLUDE_C_FEATURES
// is 1 and the two C++ feature sections (0x0300) where the old
// D_ENV_PRINTER_INCLUDE_FEATURES is.
#ifndef D_CFG_ENV_PRINTER_SECTIONS
    #if ( (defined(D_ENV_PRINTER_INCLUDE_C_FEATURES)) &&                       \
          (D_CFG_IS_ON(D_ENV_PRINTER_INCLUDE_C_FEATURES)) )
        #define D_INTERNAL_ENV_PRINTER_C_BITS 0x0080
    #else
        #define D_INTERNAL_ENV_PRINTER_C_BITS 0x0000
    #endif

    #if ( (defined(D_ENV_PRINTER_INCLUDE_FEATURES)) &&                         \
          (D_CFG_IS_ON(D_ENV_PRINTER_INCLUDE_FEATURES)) )
        #define D_INTERNAL_ENV_PRINTER_CPP_BITS 0x0300
    #else
        #define D_INTERNAL_ENV_PRINTER_CPP_BITS 0x0000
    #endif

    #define D_CFG_ENV_PRINTER_SECTIONS                                         \
        ( 0x007F                        |                                      \
          D_INTERNAL_ENV_PRINTER_C_BITS |                                      \
          D_INTERNAL_ENV_PRINTER_CPP_BITS )
#endif  // D_CFG_ENV_PRINTER_SECTIONS

// 1.2.2
// D_CFG_ENV_PRINTER_STYLE
//   configuration: the style the printer's `style` variable holds until a
// program sets it: 0 text, 1 json, 2 markdown, 3 csv, 4 keyvalue (enum
// dj_env_style). Defaults to 0.
#ifndef D_CFG_ENV_PRINTER_STYLE
    #define D_CFG_ENV_PRINTER_STYLE 0
#endif  // D_CFG_ENV_PRINTER_STYLE

// 1.3    A printer's room
//------------------------------------------------------------------------------
//   Each sizes an array in struct dj_env_printer, so a printer's size is
// their sum: about 11 KB at the defaults. The djin (env_printer.h, 2.5) has
// the same room for bindings, variables and text.

// 1.3.1
// D_CFG_ENV_PRINTER_BINDINGS
//   configuration: the variables a printer can have bound at once, counting
// a binding local to a section as one more. 64: every variable bound, and
// room for local ones.
#ifndef D_CFG_ENV_PRINTER_BINDINGS
    #define D_CFG_ENV_PRINTER_BINDINGS 64
#endif  // D_CFG_ENV_PRINTER_BINDINGS

// 1.3.2
// D_CFG_ENV_PRINTER_VARIABLES
//   configuration: the variables a printer's templates can name, and its frame
// can declare: its own 22, and the rest for a program's. At least 22.
#ifndef D_CFG_ENV_PRINTER_VARIABLES
    #define D_CFG_ENV_PRINTER_VARIABLES 48
#endif  // D_CFG_ENV_PRINTER_VARIABLES

// 1.3.3
// D_CFG_ENV_PRINTER_TEXT
//   configuration: the bytes a printer keeps text set as text in -- titles,
// rules, templates -- which it compacts when it fills.
#ifndef D_CFG_ENV_PRINTER_TEXT
    #define D_CFG_ENV_PRINTER_TEXT 2048
#endif  // D_CFG_ENV_PRINTER_TEXT

// 1.3.4
// D_CFG_ENV_PRINTER_TEMPLATE_OPS
//   configuration: the operations a printer's nine templates may compile to
// between them. The largest preset, markdown, needs 66 of the default 256.
#ifndef D_CFG_ENV_PRINTER_TEMPLATE_OPS
    #define D_CFG_ENV_PRINTER_TEMPLATE_OPS 256
#endif  // D_CFG_ENV_PRINTER_TEMPLATE_OPS


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Ranges
//------------------------------------------------------------------------------
#if ( (!D_CFG_IS_ON(D_CFG_ENV_PRINTER_CPP_FEATURES)) &&                        \
      (!D_CFG_IS_OFF(D_CFG_ENV_PRINTER_CPP_FEATURES)) )
    #error "D_CFG_ENV_PRINTER_CPP_FEATURES must be 0 or 1"
#endif

#if ( (D_CFG_NORM(D_CFG_ENV_PRINTER_SECTIONS) < 0) ||                          \
      (D_CFG_NORM(D_CFG_ENV_PRINTER_SECTIONS) > 0x03FF) )
    #error "D_CFG_ENV_PRINTER_SECTIONS must be D_ENV_SECTION_* bits"
#endif

#if ( (D_CFG_NORM(D_CFG_ENV_PRINTER_STYLE) < 0) ||                             \
      (D_CFG_NORM(D_CFG_ENV_PRINTER_STYLE) > 4) )
    #error "D_CFG_ENV_PRINTER_STYLE must be a style, 0 to 4"
#endif

#if ( ((D_CFG_ENV_PRINTER_BINDINGS + 0) < 1)      ||                           \
      ((D_CFG_ENV_PRINTER_VARIABLES + 0) < 22)    ||                           \
      ((D_CFG_ENV_PRINTER_TEXT + 0) < 1)          ||                           \
      ((D_CFG_ENV_PRINTER_TEMPLATE_OPS + 0) < 1) )
    #error "D_CFG_ENV_PRINTER_* sizes must be positive; _VARIABLES 22 or more"
#endif


#endif  // DJINTERP_CONFIG_ENV_CFG_ENV_PRINTER_H
