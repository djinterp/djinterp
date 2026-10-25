/*******************************************************************************
* djinterp [config]                                    cfg_postgres_connection.h
*
* Build-time configuration for c/db/postgres/postgres_connection.h: what a
* connection d_postgres_connect makes is set up with before it is handed back.
*   A bounded connect timeout, so that a server that does not answer is
* reported rather than waited on for as long as TCP is willing to wait; UTF-8
* as the client encoding, whatever the server's; and NOTICE messages left
* unsent, as client_min_messages = warning has it, so that the notices an
* IF [NOT] EXISTS raises when it skips do not reach libpq's default notice
* processor, which prints them on stderr. Each is the first value the layer
* gives libpq, so a connection string that names the same parameter wins; a
* connection made with libpq's own calls gets none of them. libpq's
* environment wins as well: where PGCONNECT_TIMEOUT or PGCLIENTENCODING is
* set, the layer leaves that parameter to it, and PGOPTIONS follows the
* layer's own startup option, so that its -c settings, client_min_messages
* among them, come last and win.
*   No knob: the startup option sets extra_float_digits to 3 as well, so that
* a double a server before 12 writes reads back exactly; where options of
* the caller's replace the layer's and do not name it, the layer sets it on
* the connection before handing it back.
*
*   targets:  c/db/postgres/postgres_connection.h, postgres_connection.c ->
*             D_INTERNAL_POSTGRES_CONNECT_TIMEOUT, _CLIENT_ENCODING,
*             _CLIENT_MIN_MESSAGES
*   requires: cfg_postgres_common.h
*
*
* path:      /inc/djinterp/config/c/db/postgres/cfg_postgres_connection.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Connection defaults
         1.  D_CFG_POSTGRES_CONNECT_TIMEOUT_S
         2.  D_CFG_POSTGRES_CLIENT_ENCODING
         3.  D_CFG_POSTGRES_CLIENT_MIN_MESSAGES
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_POSTGRES_CONNECT_TIMEOUT
         2.  D_INTERNAL_POSTGRES_CLIENT_ENCODING
         3.  D_INTERNAL_POSTGRES_CLIENT_MIN_MESSAGES
*/

#ifndef DJINTERP_CONFIG_C_DB_POSTGRES_CFG_POSTGRES_CONNECTION_H
#define DJINTERP_CONFIG_C_DB_POSTGRES_CFG_POSTGRES_CONNECTION_H 1

// djinterp
#include "./cfg_postgres_common.h"  // the layer's shared configuration


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Connection defaults
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_POSTGRES_CONNECT_TIMEOUT_S
//   knob: how long, in seconds, a connection the layer makes waits for the
// server to answer; 0 leaves libpq's default, which waits indefinitely.
// Defaults to 10. libpq reads values below 2 as 2.
#ifndef D_CFG_POSTGRES_CONNECT_TIMEOUT_S
    #define D_CFG_POSTGRES_CONNECT_TIMEOUT_S 10
#endif  // D_CFG_POSTGRES_CONNECT_TIMEOUT_S

// 1.1.2
// D_CFG_POSTGRES_CLIENT_ENCODING
//   knob: the client encoding a connection the layer makes asks for, as a
// string the server knows: the encoding of every string the layer sends and
// receives. The default, "UTF8", is the whole of Unicode; "" leaves libpq's
// default, which follows the server's encoding unless PGCLIENTENCODING says
// otherwise.
#ifndef D_CFG_POSTGRES_CLIENT_ENCODING
    #define D_CFG_POSTGRES_CLIENT_ENCODING "UTF8"
#endif  // D_CFG_POSTGRES_CLIENT_ENCODING

// 1.1.3
// D_CFG_POSTGRES_CLIENT_MIN_MESSAGES
//   knob: the client_min_messages a connection the layer makes starts with,
// as a level the server knows. The default, "warning", keeps NOTICE-level
// messages -- "relation already exists, skipping", "drop cascades to 2 other
// objects" -- from being sent at all, as several frameworks' adapters do;
// "" leaves the server's default, "notice", which libpq's default notice
// processor prints on stderr.
#ifndef D_CFG_POSTGRES_CLIENT_MIN_MESSAGES
    #define D_CFG_POSTGRES_CLIENT_MIN_MESSAGES "warning"
#endif  // D_CFG_POSTGRES_CLIENT_MIN_MESSAGES


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
// validation: a timeout is a count of seconds, never negative
#if ((D_CFG_POSTGRES_CONNECT_TIMEOUT_S + 0) < 0)
    #error "D_CFG_POSTGRES_CONNECT_TIMEOUT_S must not be negative"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_POSTGRES_CONNECT_TIMEOUT
//   brief: the connect timeout the layer passes to libpq, as unsigned seconds.
#define D_INTERNAL_POSTGRES_CONNECT_TIMEOUT                                    \
    ((unsigned)(D_CFG_POSTGRES_CONNECT_TIMEOUT_S))

// 3.1.2
// D_INTERNAL_POSTGRES_CLIENT_ENCODING
//   brief: D_CFG_POSTGRES_CLIENT_ENCODING, which the sources read through this
// name.
#define D_INTERNAL_POSTGRES_CLIENT_ENCODING D_CFG_POSTGRES_CLIENT_ENCODING

// 3.1.3
// D_INTERNAL_POSTGRES_CLIENT_MIN_MESSAGES
//   brief: D_CFG_POSTGRES_CLIENT_MIN_MESSAGES, which the sources read through
// this name.
#define D_INTERNAL_POSTGRES_CLIENT_MIN_MESSAGES                                \
    D_CFG_POSTGRES_CLIENT_MIN_MESSAGES


#endif  // DJINTERP_CONFIG_C_DB_POSTGRES_CFG_POSTGRES_CONNECTION_H
