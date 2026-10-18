/*******************************************************************************
* djinterp [config]                                      cfg_sqlite_connection.h
*
* Build-time configuration for c/db/sqlite/sqlite_connection.h: what a
* connection opened by d_sqlite_open, d_sqlite_open_readonly or
* d_sqlite_open_memory is set up with before it is handed back.
*   The defaults are the settings most programs end up adding by hand: a busy
* timeout, so that two connections to one file wait for each other instead of
* failing at once, and foreign-key enforcement, which SQLite leaves off for
* compatibility with schemas written before it existed. A connection opened
* with sqlite3_open_v2 itself gets none of this.
*
*   targets:  c/db/sqlite/sqlite_connection.h, sqlite_connection.c ->
*             D_INTERNAL_SQLITE_OPEN_*
*   requires: cfg_sqlite_common.h
*
*
* path:      /inc/djinterp/config/c/db/sqlite/cfg_sqlite_connection.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Connection defaults
         1.  D_CFG_SQLITE_OPEN_BUSY_TIMEOUT_MS
         2.  D_CFG_SQLITE_OPEN_FOREIGN_KEYS
         3.  D_CFG_SQLITE_OPEN_EXTENDED_CODES
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_SQLITE_OPEN_BUSY_TIMEOUT_MS
         2.  D_INTERNAL_SQLITE_OPEN_FOREIGN_KEYS
         3.  D_INTERNAL_SQLITE_OPEN_EXTENDED_CODES
*/

#ifndef DJINTERP_CONFIG_C_DB_SQLITE_CFG_SQLITE_CONNECTION_H
#define DJINTERP_CONFIG_C_DB_SQLITE_CFG_SQLITE_CONNECTION_H 1

// djinterp
#include "./cfg_sqlite_common.h"  // the layer's shared configuration


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Connection defaults
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_SQLITE_OPEN_BUSY_TIMEOUT_MS
//   knob: how long, in milliseconds, a connection the layer opens waits for a
// lock another connection holds before reporting SQLITE_BUSY (value model).
// 5000, as SQLite's own documentation suggests for interactive programs; 0
// leaves SQLite's default, which is not to wait at all.
#ifndef D_CFG_SQLITE_OPEN_BUSY_TIMEOUT_MS
    #define D_CFG_SQLITE_OPEN_BUSY_TIMEOUT_MS 5000
#endif  // D_CFG_SQLITE_OPEN_BUSY_TIMEOUT_MS

// 1.1.2
// D_CFG_SQLITE_OPEN_FOREIGN_KEYS
//   knob: enforce foreign-key constraints on a connection the layer opens.
//   ON. SQLite parses REFERENCES clauses either way, but only enforces them
// when asked, per connection; a schema that declares them almost always means
// them. Set 0 to keep SQLite's default.
#ifndef D_CFG_SQLITE_OPEN_FOREIGN_KEYS
    #define D_CFG_SQLITE_OPEN_FOREIGN_KEYS 1
#endif  // D_CFG_SQLITE_OPEN_FOREIGN_KEYS

// 1.1.3
// D_CFG_SQLITE_OPEN_EXTENDED_CODES
//   knob: report extended result codes (SQLITE_CONSTRAINT_UNIQUE rather than
// SQLITE_CONSTRAINT) on a connection the layer opens.
//   OFF, SQLite's own default: code comparing a result with SQLITE_CONSTRAINT
// would otherwise stop matching. Turn it on where the detail is wanted, and
// compare `(rc & 0xff)` with the primary code.
#ifndef D_CFG_SQLITE_OPEN_EXTENDED_CODES
    #define D_CFG_SQLITE_OPEN_EXTENDED_CODES 0
#endif  // D_CFG_SQLITE_OPEN_EXTENDED_CODES


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if (D_CFG_NORM(D_CFG_SQLITE_OPEN_BUSY_TIMEOUT_MS) < 0)
    #error "D_CFG_SQLITE_OPEN_BUSY_TIMEOUT_MS must not be negative"
#endif
#if !D_CFG_IS_BOOL(D_CFG_SQLITE_OPEN_FOREIGN_KEYS)
    #error "D_CFG_SQLITE_OPEN_FOREIGN_KEYS must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_SQLITE_OPEN_EXTENDED_CODES)
    #error "D_CFG_SQLITE_OPEN_EXTENDED_CODES must be literally 0 or 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_SQLITE_OPEN_BUSY_TIMEOUT_MS
//   resolved: the busy timeout d_sqlite_open* sets; 0 sets none.
#define D_INTERNAL_SQLITE_OPEN_BUSY_TIMEOUT_MS                                 \
    D_CFG_NORM(D_CFG_SQLITE_OPEN_BUSY_TIMEOUT_MS)

// 3.1.2
// D_INTERNAL_SQLITE_OPEN_FOREIGN_KEYS
//   resolved: 1 when d_sqlite_open* turns foreign-key enforcement on.
#if D_CFG_IS_ON(D_CFG_SQLITE_OPEN_FOREIGN_KEYS)
    #define D_INTERNAL_SQLITE_OPEN_FOREIGN_KEYS 1
#else
    #define D_INTERNAL_SQLITE_OPEN_FOREIGN_KEYS 0
#endif

// 3.1.3
// D_INTERNAL_SQLITE_OPEN_EXTENDED_CODES
//   resolved: 1 when d_sqlite_open* turns extended result codes on.
#if D_CFG_IS_ON(D_CFG_SQLITE_OPEN_EXTENDED_CODES)
    #define D_INTERNAL_SQLITE_OPEN_EXTENDED_CODES 1
#else
    #define D_INTERNAL_SQLITE_OPEN_EXTENDED_CODES 0
#endif


#endif  // DJINTERP_CONFIG_C_DB_SQLITE_CFG_SQLITE_CONNECTION_H
