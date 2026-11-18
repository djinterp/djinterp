/*******************************************************************************
* djinterp [config]                                                   cfg_desc.h
*
* Configuration for c/desc: what a build of desc has. Each knob is a default
* that a build overrides with -D. desc has two engines, both on by default:
* units, which write prose for people, and facts, typed values for machines,
* derived from the same field list (desc_units.h). A knob that leaves part of
* desc out leaves its code and its text out of the binary; an option, read at
* run time, chooses among what the knobs built (desc.ring, desc.layers,
* desc.style, desc.form, desc.format, desc.text).
*   targets:  c/desc/desc_units.h, c/desc/desc_macro.h,
*             c/desc/desc_machines.h, cpp/desc/desc_units.hpp ->
*                 D_CFG_DESC_ENABLED, D_CFG_DESC_UNITS, D_CFG_DESC_FACTS,
*                 D_CFG_DESC_RING_MAX, D_CFG_DESC_RING,
*                 D_CFG_DESC_LAYERS_BUILT, D_CFG_DESC_LAYERS,
*                 D_CFG_DESC_ARTICLES, D_CFG_DESC_OPTIONS,
*                 D_CFG_DESC_SCOPES, D_CFG_DESC_BINDINGS,
*                 D_CFG_DESC_MACRO, D_CFG_DESC_AT_BUILD,
*                 D_CFG_DESC_TEXT, D_CFG_DESC_MACHINES,
*                 D_CFG_DESC_STYLE, D_CFG_DESC_FORM,
*                 D_CFG_DESC_FORMAT, D_CFG_DESC_TEXT_PRESET,
*                 D_CFG_DESC_JSON_INTEGERS, D_CFG_DESC_JSON_NONFINITE
*   requires: cfg_common.h
*
*
* path:      /inc/djinterp/config/c/desc/cfg_desc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  The engines
         1.  D_CFG_DESC_ENABLED
         2.  D_CFG_DESC_UNITS
         3.  D_CFG_DESC_FACTS
    2.  Rings and layers
         1.  D_CFG_DESC_RING_MAX
         2.  D_CFG_DESC_RING
         3.  D_CFG_DESC_LAYERS_BUILT
         4.  D_CFG_DESC_LAYERS
    3.  Words
         1.  D_CFG_DESC_ARTICLES
    4.  Options
         1.  D_CFG_DESC_OPTIONS
         2.  D_CFG_DESC_SCOPES
         3.  D_CFG_DESC_BINDINGS
    5.  The macro layer
         1.  D_CFG_DESC_MACRO
         2.  D_CFG_DESC_AT_BUILD
    6.  Machines and describing
         1.  D_CFG_DESC_TEXT
         2.  D_CFG_DESC_MACHINES
         3.  D_CFG_DESC_STYLE
         4.  D_CFG_DESC_FORM
         5.  D_CFG_DESC_FORMAT
         6.  D_CFG_DESC_TEXT_PRESET
         7.  D_CFG_DESC_JSON_INTEGERS
         8.  D_CFG_DESC_JSON_NONFINITE
2.  VALIDATION
    ----------
    1.  Ranges
*/

#ifndef DJINTERP_CONFIG_C_DESC_CFG_DESC_H
#define DJINTERP_CONFIG_C_DESC_CFG_DESC_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_NORM, user overrides


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    The engines
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_DESC_ENABLED
//   configuration: 0 builds no desc at all: no engine, no tables, no options,
// and no macro layer. 1 by default.
#ifndef D_CFG_DESC_ENABLED
    #define D_CFG_DESC_ENABLED 1
#endif

// 1.1.2
// D_CFG_DESC_UNITS
//   configuration: 1 builds the units engine, which writes prose; 0 leaves it
// out, and a type's units with it (D_DESC_UNITS_TABLE). 1 by default.
#ifndef D_CFG_DESC_UNITS
    #define D_CFG_DESC_UNITS 1
#endif

// 1.1.3
// D_CFG_DESC_FACTS
//   configuration: 1 builds the facts derived from a type's field list
// (d_desc_facts); 0 compiles them out, and a type's field list with them
// (D_DESC_FIELDS_TABLE). desc.h's sink is header-only and costs nothing
// unused, so it stays. 1 by default.
#ifndef D_CFG_DESC_FACTS
    #define D_CFG_DESC_FACTS 1
#endif


// 1.2    Rings and layers
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_DESC_RING_MAX
//   configuration: the outermost ring built, 0 (name) to 3 (detailed). The
// units of the rings past it are not compiled, and their text is not in the
// binary. 3 by default.
#ifndef D_CFG_DESC_RING_MAX
    #define D_CFG_DESC_RING_MAX 3
#endif

// 1.2.2
// D_CFG_DESC_RING
//   configuration: the ring a description has when nobody says, and
// desc.ring's default. 1, a line, by default.
#ifndef D_CFG_DESC_RING
    #define D_CFG_DESC_RING 1
#endif

// 1.2.3
// D_CFG_DESC_LAYERS_BUILT
//   configuration: the layers whose units are built, a bit for each (1 <<
// enum dj_desc_layer). A unit of a layer left out is not compiled. Every
// layer, 0x1FF, by default.
#ifndef D_CFG_DESC_LAYERS_BUILT
    #define D_CFG_DESC_LAYERS_BUILT 0x1FF
#endif

// 1.2.4
// D_CFG_DESC_LAYERS
//   configuration: the layers a description shows when nobody says, and
// desc.layers's default. Every layer, 0x1FF, by default.
#ifndef D_CFG_DESC_LAYERS
    #define D_CFG_DESC_LAYERS 0x1FF
#endif


// 1.3    Words
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_DESC_ARTICLES
//   configuration: 1 writes "a" or "an" before a description from the line
// ring out; 0 writes none, and builds none of the code that chooses one. 1 by
// default.
#ifndef D_CFG_DESC_ARTICLES
    #define D_CFG_DESC_ARTICLES 1
#endif


// 1.4    Options
//------------------------------------------------------------------------------
// 1.4.1
// D_CFG_DESC_OPTIONS
//   configuration: 1 builds desc's djin, "desc", whose variables ring and
// layers a settings file, the environment or the command line may set, for
// every type or for one (desc.file_tree.ring); 0 leaves the knobs to decide
// alone. 1 by default; below the option core's 64-bit floor it builds as 0.
#ifndef D_CFG_DESC_OPTIONS
    #define D_CFG_DESC_OPTIONS 1
#endif

// 1.4.2
// D_CFG_DESC_SCOPES
//   configuration: the types that may have options of their own: one scope
// each, named when the type is declared to desc (d_desc_declare). 16 by
// default.
#ifndef D_CFG_DESC_SCOPES
    #define D_CFG_DESC_SCOPES 16
#endif

// 1.4.3
// D_CFG_DESC_BINDINGS
//   configuration: the bindings desc's djin holds at once, one per source per
// scope per variable bound. 64 by default.
#ifndef D_CFG_DESC_BINDINGS
    #define D_CFG_DESC_BINDINGS 64
#endif


// 1.5    The macro layer
//------------------------------------------------------------------------------
// 1.5.1
// D_CFG_DESC_MACRO
//   configuration: 1 builds the macro layer (desc_macro.h): descriptions the
// preprocessor makes, as string literals, of what a type and the knobs fix.
// Its kit needs variadic macros; without them, only #if-made words are
// built. 1 by default.
#ifndef D_CFG_DESC_MACRO
    #define D_CFG_DESC_MACRO 1
#endif

// 1.5.2
// D_CFG_DESC_AT_BUILD
//   configuration: 1 has the compiler print desc's own build sentence with
// #pragma message while it builds desc_units.c. Clang counts it as a
// warning. 0 by default.
#ifndef D_CFG_DESC_AT_BUILD
    #define D_CFG_DESC_AT_BUILD 0
#endif


// 1.6    Machines and describing
//------------------------------------------------------------------------------
// 1.6.1
// D_CFG_DESC_TEXT
//   configuration: 1 builds desc's own support for text describers: the
// text and JSON machines' text operations, and desc.form's "text". 0 leaves
// them out: a text describer's words still reach a machine as one fact,
// through desc.h's adapter, and desc.form=text is refused by name. 1 by
// default.
#ifndef D_CFG_DESC_TEXT
    #define D_CFG_DESC_TEXT 1
#endif

// 1.6.2
// D_CFG_DESC_MACHINES
//   configuration: desc's machines built, a bit for each: 0x1 text, 0x2
// JSON, 0x4 collecting (1 << D_DESC_FORMAT_*). A format whose machine is
// left out is refused by name. Every one, 0x7, by default.
#ifndef D_CFG_DESC_MACHINES
    #define D_CFG_DESC_MACHINES 0x7
#endif

// 1.6.3
// D_CFG_DESC_STYLE
//   configuration: how describe writes a type when nobody says, and
// desc.style's default: 0 prose from its units, 1 its facts. 0 by default,
// or 1 when the units are not built.
#ifndef D_CFG_DESC_STYLE
    #if D_CFG_DESC_UNITS
        #define D_CFG_DESC_STYLE 0
    #else
        #define D_CFG_DESC_STYLE 1
    #endif
#endif

// 1.6.4
// D_CFG_DESC_FORM
//   configuration: the describer that runs when there are both, when nobody
// says, and desc.form's default: 0 the fact describer, 1 the text
// describer. 0 by default.
#ifndef D_CFG_DESC_FORM
    #define D_CFG_DESC_FORM 0
#endif

// 1.6.5
// D_CFG_DESC_FORMAT
//   configuration: the machine facts are written by when nobody says, and
// desc.format's default: 0 text, 1 JSON. 0 by default.
#ifndef D_CFG_DESC_FORMAT
    #define D_CFG_DESC_FORMAT 0
#endif

// 1.6.6
// D_CFG_DESC_TEXT_PRESET
//   configuration: the text machine's preset when nobody says, and
// desc.text's default: 0 a report, a line per fact; 1 a line of key=value
// pairs. 0 by default.
#ifndef D_CFG_DESC_TEXT_PRESET
    #define D_CFG_DESC_TEXT_PRESET 0
#endif

// 1.6.7
// D_CFG_DESC_JSON_INTEGERS
//   configuration: how describe's JSON writes an integer, and
// desc.json_integers' default: 0 a number, every digit kept; 1 a string,
// exact for readers that read numbers as doubles, as JavaScript does. The
// log's own JSON has log.json_integers. 0 by default.
#ifndef D_CFG_DESC_JSON_INTEGERS
    #define D_CFG_DESC_JSON_INTEGERS 0
#endif

// 1.6.8
// D_CFG_DESC_JSON_NONFINITE
//   configuration: how describe's JSON writes NaN and the infinities, and
// desc.json_nonfinite's default: 0 null; 1 "NaN", "Infinity", "-Infinity".
// 0 by default.
#ifndef D_CFG_DESC_JSON_NONFINITE
    #define D_CFG_DESC_JSON_NONFINITE 0
#endif


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Ranges
//------------------------------------------------------------------------------

#if ( (D_CFG_DESC_RING_MAX < 0) || (D_CFG_DESC_RING_MAX > 3) )
    #error "D_CFG_DESC_RING_MAX must be 0 (name) to 3 (detailed)"
#endif

#if ( (D_CFG_DESC_RING < 0) || (D_CFG_DESC_RING > 3) )
    #error "D_CFG_DESC_RING must be 0 (name) to 3 (detailed)"
#endif

#if ( (D_CFG_DESC_LAYERS_BUILT < 0) || (D_CFG_DESC_LAYERS_BUILT > 0x1FF) ||  \
      (D_CFG_DESC_LAYERS < 0)       || (D_CFG_DESC_LAYERS > 0x1FF) )
    #error "D_CFG_DESC_LAYERS_BUILT and D_CFG_DESC_LAYERS are masks of 9 bits"
#endif

#if ( (D_CFG_DESC_SCOPES < 1) || (D_CFG_DESC_BINDINGS < 2) )
    #error "D_CFG_DESC_SCOPES must be positive, D_CFG_DESC_BINDINGS 2 or more"
#endif

#if ( (D_CFG_DESC_MACHINES < 0) || (D_CFG_DESC_MACHINES > 0x7) )
    #error "D_CFG_DESC_MACHINES is a mask of 3 bits: text, JSON, collecting"
#endif

#if ( (D_CFG_DESC_STYLE < 0) || (D_CFG_DESC_STYLE > 1)             ||        \
      (D_CFG_DESC_FORM < 0)  || (D_CFG_DESC_FORM > 1)              ||        \
      (D_CFG_DESC_FORMAT < 0) || (D_CFG_DESC_FORMAT > 1)           ||        \
      (D_CFG_DESC_TEXT_PRESET < 0) || (D_CFG_DESC_TEXT_PRESET > 1) )
    #error "D_CFG_DESC_STYLE, _FORM, _FORMAT and _TEXT_PRESET are 0 or 1"
#endif

#if ( (D_CFG_DESC_JSON_INTEGERS < 0) || (D_CFG_DESC_JSON_INTEGERS > 1) ||   \
      (D_CFG_DESC_JSON_NONFINITE < 0) || (D_CFG_DESC_JSON_NONFINITE > 1) )
    #error "D_CFG_DESC_JSON_INTEGERS and _JSON_NONFINITE are 0 or 1"
#endif

#if ( (D_CFG_DESC_STYLE == 0) && (D_CFG_DESC_UNITS == 0) )
    #error "D_CFG_DESC_STYLE 0 (prose) needs D_CFG_DESC_UNITS"
#endif

#if ( (D_CFG_DESC_FORM == 1) && (D_CFG_DESC_TEXT == 0) )
    #error "D_CFG_DESC_FORM 1 (text) needs D_CFG_DESC_TEXT"
#endif

#if ( ((D_CFG_DESC_MACHINES & 0x3) != 0) &&                                 \
      (((D_CFG_DESC_MACHINES >> D_CFG_DESC_FORMAT) & 1) == 0) )
    #error "D_CFG_DESC_FORMAT names a machine D_CFG_DESC_MACHINES leaves out"
#endif


#endif  // DJINTERP_CONFIG_C_DESC_CFG_DESC_H
