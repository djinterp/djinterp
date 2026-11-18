/*******************************************************************************
* djinterp [config]                                                    cfg_log.h
*
* Configuration for c/log: what a build of the log has. Each knob is a default
* that a build overrides with -D; an option of the log's djin, "log", chooses
* among what the knobs built for a run (log.level, log.format, ...), each
* option's default its knob (docs/djinterp/log/log_design.md).
*   targets:  c/log/log.h ->
*                 D_CFG_LOG_ENABLED,
*                 D_CFG_LOG_LEVELS, D_CFG_LOG_LEVELS_EXTRA,
*                 D_CFG_LOG_LEVELS_MAX, D_CFG_LOG_LEVEL, D_CFG_LOG_LEVEL_MIN,
*                 D_CFG_LOG_FACTS_MAX, D_CFG_LOG_DEPTH_MAX,
*                 D_CFG_LOG_BODY_MAX, D_CFG_LOG_TEMPLATE_OPS,
*                 D_CFG_LOG_CODES, D_CFG_LOG_MODULES_MAX,
*                 D_CFG_LOG_GRADE_LEVEL,
*                 D_CFG_LOG_FORMATS, D_CFG_LOG_FORMAT,
*                 D_CFG_LOG_CODE_STYLE, D_CFG_LOG_NESTING,
*                 D_CFG_LOG_JSON_INTEGERS, D_CFG_LOG_JSON_NONFINITE,
*                 D_CFG_LOG_SCOPE, D_CFG_LOG_RESOURCE, D_CFG_LOG_CONTEXT,
*                 D_CFG_LOG_OBSERVED_TIME, D_CFG_LOG_TIME,
*                 D_CFG_LOG_OPTIONS, D_CFG_LOG_SCOPES, D_CFG_LOG_BINDINGS,
*                 D_CFG_LOG_TEXT_ROOM, D_CFG_LOG_BUFFER,
*                 D_CFG_LOG_THREADSAFE
*   requires: cfg_common.h
*
*
* path:      /inc/djinterp/config/c/log/cfg_log.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  The log
         1.  D_CFG_LOG_ENABLED
    2.  Levels
         1.  D_CFG_LOG_LEVELS
         2.  D_CFG_LOG_LEVELS_EXTRA
         3.  D_CFG_LOG_LEVELS_MAX
         4.  D_CFG_LOG_LEVEL
         5.  D_CFG_LOG_LEVEL_MIN
    3.  Records
         1.  D_CFG_LOG_FACTS_MAX
         2.  D_CFG_LOG_DEPTH_MAX
         3.  D_CFG_LOG_BODY_MAX
         4.  D_CFG_LOG_TEMPLATE_OPS
    4.  Codes and grades
         1.  D_CFG_LOG_CODES
         2.  D_CFG_LOG_MODULES_MAX
         3.  D_CFG_LOG_GRADE_LEVEL
    5.  Writers
         1.  D_CFG_LOG_FORMATS
         2.  D_CFG_LOG_FORMAT
         3.  D_CFG_LOG_CODE_STYLE
         4.  D_CFG_LOG_NESTING
         5.  D_CFG_LOG_JSON_INTEGERS
         6.  D_CFG_LOG_JSON_NONFINITE
    6.  OpenTelemetry's features
         1.  D_CFG_LOG_SCOPE
         2.  D_CFG_LOG_RESOURCE
         3.  D_CFG_LOG_CONTEXT
         4.  D_CFG_LOG_OBSERVED_TIME
         5.  D_CFG_LOG_TIME
    7.  Options, room and threads
         1.  D_CFG_LOG_OPTIONS
         2.  D_CFG_LOG_SCOPES
         3.  D_CFG_LOG_BINDINGS
         4.  D_CFG_LOG_TEXT_ROOM
         5.  D_CFG_LOG_BUFFER
         6.  D_CFG_LOG_THREADSAFE
2.  VALIDATION
    ----------
    1.  Ranges
*/

#ifndef DJINTERP_CONFIG_C_LOG_CFG_LOG_H
#define DJINTERP_CONFIG_C_LOG_CFG_LOG_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_NORM, user overrides


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    The log
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_LOG_ENABLED
//   configuration: 0 builds no log: D_LOG compiles to nothing, and every call
// does nothing. 1 by default.
#ifndef D_CFG_LOG_ENABLED
    #define D_CFG_LOG_ENABLED 1
#endif


// 1.2    Levels
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_LOG_LEVELS
//   configuration: how much of levels a build has, a spectrum: 0 none, every
// record written and level checks compiled away; 1 the seven built in
// (trace 1, debug 5, info 9, notice 10, warning 13, error 17, fatal 21, on
// OpenTelemetry's scale of 1 to 24); 2 the seven and D_CFG_LOG_LEVELS_EXTRA;
// 3 those, and levels added while the program runs (d_log_level_add,
// log.levels). 1 by default.
#ifndef D_CFG_LOG_LEVELS
    #define D_CFG_LOG_LEVELS 1
#endif

// 1.2.2
// D_CFG_LOG_LEVELS_EXTRA
//   configuration: a program's own levels, for D_CFG_LOG_LEVELS 2 and 3: a
// macro of one parameter listing them, each X(name, severity):
//     #define D_CFG_LOG_LEVELS_EXTRA(X) X(audit, 14) X(security, 19)
// None by default.
#ifndef D_CFG_LOG_LEVELS_EXTRA
    #define D_CFG_LOG_LEVELS_EXTRA(X)
#endif

// 1.2.3
// D_CFG_LOG_LEVELS_MAX
//   configuration: the levels the log holds at once, built in, extra and
// added, with D_CFG_LOG_LEVELS 3. 24 by default, one per severity.
#ifndef D_CFG_LOG_LEVELS_MAX
    #define D_CFG_LOG_LEVELS_MAX 24
#endif

// 1.2.4
// D_CFG_LOG_LEVEL
//   configuration: the threshold when nobody says, and log.level's default:
// a severity, 1 to 24; a record below it is not built. 9, info, by default.
#ifndef D_CFG_LOG_LEVEL
    #define D_CFG_LOG_LEVEL 9
#endif

// 1.2.5
// D_CFG_LOG_LEVEL_MIN
//   configuration: a severity below which D_LOG compiles to nothing, at no
// cost; no option can bring those records back. 0, nothing compiled out, by
// default.
#ifndef D_CFG_LOG_LEVEL_MIN
    #define D_CFG_LOG_LEVEL_MIN 0
#endif


// 1.3    Records
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_LOG_FACTS_MAX
//   configuration: the most facts a record keeps, on the caller's stack;
// log.facts_max may lower it. Past it, facts are counted, as OpenTelemetry's
// dropped attributes count. 32 by default.
#ifndef D_CFG_LOG_FACTS_MAX
    #define D_CFG_LOG_FACTS_MAX 32
#endif

// 1.3.2
// D_CFG_LOG_DEPTH_MAX
//   configuration: the deepest a record's facts nest; log.depth_max may
// lower it. 4 by default.
#ifndef D_CFG_LOG_DEPTH_MAX
    #define D_CFG_LOG_DEPTH_MAX 4
#endif

// 1.3.3
// D_CFG_LOG_BODY_MAX
//   configuration: the bytes a record's body, its code's template written
// from its facts, may take; a longer one is cut where a character starts.
// 512 by default.
#ifndef D_CFG_LOG_BODY_MAX
    #define D_CFG_LOG_BODY_MAX 512
#endif

// 1.3.4
// D_CFG_LOG_TEMPLATE_OPS
//   configuration: the operations a code's template may compile to; a code
// whose template needs more has its template's text for a body. 64 by
// default.
#ifndef D_CFG_LOG_TEMPLATE_OPS
    #define D_CFG_LOG_TEMPLATE_OPS 64
#endif


// 1.4    Codes and grades
//------------------------------------------------------------------------------
// 1.4.1
// D_CFG_LOG_CODES
//   configuration: 1 builds codes' names and templates; 0 keeps numbers only,
// so a code is written "net:207" and a body is empty. 1 by default; a module
// may set its own, D_CFG_<MODULE>_LOG_CODES, with this as its default.
#ifndef D_CFG_LOG_CODES
    #define D_CFG_LOG_CODES 1
#endif

// 1.4.2
// D_CFG_LOG_MODULES_MAX
//   configuration: the code tables the log can register at once, for
// d_log_code_find. 64 by default.
#ifndef D_CFG_LOG_MODULES_MAX
    #define D_CFG_LOG_MODULES_MAX 64
#endif

// 1.4.3
// D_CFG_LOG_GRADE_LEVEL
//   configuration: a grade's level, a severity, as a macro of the grade's
// number (D_LOG_GRADE_*: ok, note, caveat, soft, rejected, failed, broken),
// so a record's default level is a constant where it is written and
// D_CFG_LOG_LEVEL_MIN can compile it out. log.grade_<grade> changes one for
// a run. By default (E26 (b)) debug, info, notice, debug, warning, error and
// fatal.
#ifndef D_CFG_LOG_GRADE_LEVEL
    #define D_CFG_LOG_GRADE_LEVEL(grade)                                      \
        ( ((grade) == 0) ?  5 : ((grade) == 1) ?  9 : ((grade) == 2) ? 10 :   \
          ((grade) == 3) ?  5 : ((grade) == 4) ? 13 : ((grade) == 5) ? 17 :   \
                             21 )
#endif


// 1.5    Writers
//------------------------------------------------------------------------------
// 1.5.1
// D_CFG_LOG_FORMATS
//   configuration: the writers built, a bit each: 0x1 text, 0x2 JSON lines,
// 0x4 OTLP/JSON. A format left out is refused by name in log.format. Every
// one, 0x7, by default.
#ifndef D_CFG_LOG_FORMATS
    #define D_CFG_LOG_FORMATS 0x7
#endif

// 1.5.2
// D_CFG_LOG_FORMAT
//   configuration: the writer when nobody says, and log.format's default: 0
// text, 1 JSON lines, 2 OTLP/JSON. 0 by default.
#ifndef D_CFG_LOG_FORMAT
    #define D_CFG_LOG_FORMAT 0
#endif

// 1.5.3
// D_CFG_LOG_CODE_STYLE
//   configuration: how text writes a code, and log.code_style's default: 0 by
// name ("net.connect_timeout"), 1 by number ("net:207"), 2 both. 0 by
// default.
#ifndef D_CFG_LOG_CODE_STYLE
    #define D_CFG_LOG_CODE_STYLE 0
#endif

// 1.5.4
// D_CFG_LOG_NESTING
//   configuration: how JSON lines write nested facts, and log.nesting's
// default: 0 nested objects, 1 dotted keys for tools that want flat
// attributes. 0 by default.
#ifndef D_CFG_LOG_NESTING
    #define D_CFG_LOG_NESTING 0
#endif

// 1.5.5
// D_CFG_LOG_JSON_INTEGERS
//   configuration: how JSON lines write an integer, and log.json_integers'
// default: 0 a number, every digit kept; 1 a string, exact for readers that
// read numbers as doubles (E48 (d)). OTLP/JSON writes 64-bit integers as
// strings always, as its specification says. 0 by default.
#ifndef D_CFG_LOG_JSON_INTEGERS
    #define D_CFG_LOG_JSON_INTEGERS 0
#endif

// 1.5.6
// D_CFG_LOG_JSON_NONFINITE
//   configuration: how JSON lines write NaN and the infinities, and
// log.json_nonfinite's default: 0 null, 1 "NaN", "Infinity", "-Infinity"
// (E49 (c)). 0 by default.
#ifndef D_CFG_LOG_JSON_NONFINITE
    #define D_CFG_LOG_JSON_NONFINITE 0
#endif


// 1.6    OpenTelemetry's features
//------------------------------------------------------------------------------
// 1.6.1
// D_CFG_LOG_SCOPE
//   configuration: 1 builds the instrumentation scope, the record's module;
// log.scope turns it on or off. 1 by default.
#ifndef D_CFG_LOG_SCOPE
    #define D_CFG_LOG_SCOPE 1
#endif

// 1.6.2
// D_CFG_LOG_RESOURCE
//   configuration: 1 builds the resource: the program, its process, from a
// provider a program may replace; log.resource turns it on (OTLP) or off
// (JSON lines). 1 by default.
#ifndef D_CFG_LOG_RESOURCE
    #define D_CFG_LOG_RESOURCE 1
#endif

// 1.6.3
// D_CFG_LOG_CONTEXT
//   configuration: 1 builds trace and span ids and trace flags, from a
// context provider a tracing library supplies; log.context turns them on.
// 0 by default: djinterp makes no traces.
#ifndef D_CFG_LOG_CONTEXT
    #define D_CFG_LOG_CONTEXT 0
#endif

// 1.6.4
// D_CFG_LOG_OBSERVED_TIME
//   configuration: 1 builds the observed time, when the log saw a record;
// log.observed_time turns it on. 1 by default.
#ifndef D_CFG_LOG_OBSERVED_TIME
    #define D_CFG_LOG_OBSERVED_TIME 1
#endif

// 1.6.5
// D_CFG_LOG_TIME
//   configuration: how a time is written, and log.time's default: 0 to 3,
// UTC with 0, 3, 6 or 9 digits of a second; 4 to 7, local time with its
// offset, the same. 1, UTC to the millisecond, by default.
#ifndef D_CFG_LOG_TIME
    #define D_CFG_LOG_TIME 1
#endif


// 1.7    Options, room and threads
//------------------------------------------------------------------------------
// 1.7.1
// D_CFG_LOG_OPTIONS
//   configuration: 1 builds the log's djin, "log", whose rows a settings
// file, the environment and the command line set; 0 leaves the knobs to
// decide alone. 1 by default; below the option core's 64-bit floor it builds
// as 0.
#ifndef D_CFG_LOG_OPTIONS
    #define D_CFG_LOG_OPTIONS 1
#endif

// 1.7.2
// D_CFG_LOG_SCOPES
//   configuration: the modules that may have settings of their own,
// log.<module>.level: one scope each, named when the module's codes are
// registered. 32 by default.
#ifndef D_CFG_LOG_SCOPES
    #define D_CFG_LOG_SCOPES 32
#endif

// 1.7.3
// D_CFG_LOG_BINDINGS
//   configuration: the bindings the djin holds at once. 128 by default.
#ifndef D_CFG_LOG_BINDINGS
    #define D_CFG_LOG_BINDINGS 128
#endif

// 1.7.4
// D_CFG_LOG_TEXT_ROOM
//   configuration: the bytes the djin keeps for its text settings: the
// destination, the service's name, the levels added. 512 by default.
#ifndef D_CFG_LOG_TEXT_ROOM
    #define D_CFG_LOG_TEXT_ROOM 512
#endif

// 1.7.5
// D_CFG_LOG_BUFFER
//   configuration: the bytes the log's destination buffers; it is flushed at
// each record's end, so a record that fits is one write. 4096 by default.
#ifndef D_CFG_LOG_BUFFER
    #define D_CFG_LOG_BUFFER 4096
#endif

// 1.7.6
// D_CFG_LOG_THREADSAFE
//   configuration: 1 takes a lock (c/sync) around the writer and its stream,
// so records from threads do not interleave; call d_log_init before the
// program starts its threads. 0 leaves the log to one thread at a time. 1 by
// default.
#ifndef D_CFG_LOG_THREADSAFE
    #define D_CFG_LOG_THREADSAFE 1
#endif


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Ranges
//------------------------------------------------------------------------------

#if ( (D_CFG_LOG_LEVELS < 0) || (D_CFG_LOG_LEVELS > 3) )
    #error "D_CFG_LOG_LEVELS must be 0 (none) to 3 (added at run time)"
#endif

#if ( (D_CFG_LOG_LEVEL < 1) || (D_CFG_LOG_LEVEL > 24) ||                      \
      (D_CFG_LOG_LEVEL_MIN < 0) || (D_CFG_LOG_LEVEL_MIN > 25) )
    #error "D_CFG_LOG_LEVEL is a severity, 1 to 24; D_CFG_LOG_LEVEL_MIN 0 to 25"
#endif

#if ( (D_CFG_LOG_LEVELS_MAX < 7) || (D_CFG_LOG_LEVELS_MAX > 64) )
    #error "D_CFG_LOG_LEVELS_MAX must be 7 to 64"
#endif

#if ( (D_CFG_LOG_FACTS_MAX < 1) || (D_CFG_LOG_FACTS_MAX > 1024) ||            \
      (D_CFG_LOG_DEPTH_MAX < 0) || (D_CFG_LOG_DEPTH_MAX > 16) )
    #error "D_CFG_LOG_FACTS_MAX must be 1 to 1024, D_CFG_LOG_DEPTH_MAX 0 to 16"
#endif

#if ( (D_CFG_LOG_BODY_MAX < 16) || (D_CFG_LOG_TEMPLATE_OPS < 4) ||            \
      (D_CFG_LOG_BUFFER < 0) || (D_CFG_LOG_TEXT_ROOM < 64) )
    #error "D_CFG_LOG_BODY_MAX >= 16, _TEMPLATE_OPS >= 4, _TEXT_ROOM >= 64"
#endif

#if ( (D_CFG_LOG_FORMATS < 1) || (D_CFG_LOG_FORMATS > 0x7) ||                 \
      (D_CFG_LOG_FORMAT < 0) || (D_CFG_LOG_FORMAT > 2) ||                     \
      (((D_CFG_LOG_FORMATS >> D_CFG_LOG_FORMAT) & 1) == 0) )
    #error "D_CFG_LOG_FORMATS is a mask of 3 bits; D_CFG_LOG_FORMAT one built"
#endif

#if ( (D_CFG_LOG_CODE_STYLE < 0) || (D_CFG_LOG_CODE_STYLE > 2) ||             \
      (D_CFG_LOG_TIME < 0) || (D_CFG_LOG_TIME > 7) )
    #error "D_CFG_LOG_CODE_STYLE must be 0 to 2, D_CFG_LOG_TIME 0 to 7"
#endif

#if ( (D_CFG_LOG_SCOPES < 1) || (D_CFG_LOG_BINDINGS < 16) ||                  \
      (D_CFG_LOG_MODULES_MAX < 1) )
    #error "D_CFG_LOG_SCOPES and _MODULES_MAX positive, _BINDINGS 16 or more"
#endif


#endif  // DJINTERP_CONFIG_C_LOG_CFG_LOG_H
