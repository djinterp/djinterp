/*******************************************************************************
* djinterp [config]                                     cfg_mongodb_connection.h
*
* Build-time configuration for c/db/mongodb/mongodb_connection.h: what a
* connection d_mongodb_connect makes is set up with before it is handed back.
*   A bounded wait for a server, so that one that does not answer is
* reported rather than waited on for the driver's 30 seconds, and the name
* the connection gives itself in the server's logs and in currentOp. Each is
* the first value the layer gives the driver, so a connection string that
* names the same option wins; a client a program made with the driver's own
* calls gets neither.
*   No knob: the layer always asks the driver for its second error API, in
* which a failure the server reports comes with the server's own code
* (mongodb_common.h).
*
*   targets:  c/db/mongodb/mongodb_connection.h, mongodb_connection.c ->
*             D_INTERNAL_MONGODB_SERVER_SELECTION_TIMEOUT_MS,
*             D_INTERNAL_MONGODB_APP_NAME
*   requires: cfg_mongodb_common.h
*
*
* path:      /inc/djinterp/config/c/db/mongodb/cfg_mongodb_connection.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Connection defaults
         1.  D_CFG_MONGODB_SERVER_SELECTION_TIMEOUT_MS
         2.  D_CFG_MONGODB_APP_NAME
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_MONGODB_SERVER_SELECTION_TIMEOUT_MS
         2.  D_INTERNAL_MONGODB_APP_NAME
*/

#ifndef DJINTERP_CONFIG_C_DB_MONGODB_CFG_MONGODB_CONNECTION_H
#define DJINTERP_CONFIG_C_DB_MONGODB_CFG_MONGODB_CONNECTION_H 1

// djinterp
#include "./cfg_mongodb_common.h"  // the layer's shared configuration


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Connection defaults
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_MONGODB_SERVER_SELECTION_TIMEOUT_MS
//   knob: how long, in milliseconds, an operation waits for a server it may
// use -- the primary for a write, any member a read preference allows for a
// read -- before it fails; 0 leaves the driver's own, 30000. Defaults to
// 10000, the connect timeout the other layers keep.
#ifndef D_CFG_MONGODB_SERVER_SELECTION_TIMEOUT_MS
    #define D_CFG_MONGODB_SERVER_SELECTION_TIMEOUT_MS 10000
#endif  // D_CFG_MONGODB_SERVER_SELECTION_TIMEOUT_MS

// 1.1.2
// D_CFG_MONGODB_APP_NAME
//   knob: the name a connection the layer makes gives itself in the handshake,
// which the server writes in its log and currentOp shows; "" sends none. At
// most 128 bytes, as the driver allows. Defaults to "".
#ifndef D_CFG_MONGODB_APP_NAME
    #define D_CFG_MONGODB_APP_NAME ""
#endif  // D_CFG_MONGODB_APP_NAME


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
// validation: a timeout is a count of milliseconds, never negative, and one
// the URI's 32-bit option holds
#if ( ((D_CFG_MONGODB_SERVER_SELECTION_TIMEOUT_MS + 0) < 0) ||                 \
      ((D_CFG_MONGODB_SERVER_SELECTION_TIMEOUT_MS + 0) > 2147483647) )
    #error "D_CFG_MONGODB_SERVER_SELECTION_TIMEOUT_MS must be 0 to 2147483647"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_MONGODB_SERVER_SELECTION_TIMEOUT_MS
//   brief: the server selection timeout the layer gives the driver, as an int
// of milliseconds; 0 for the driver's own.
#define D_INTERNAL_MONGODB_SERVER_SELECTION_TIMEOUT_MS                         \
    ((int)(D_CFG_MONGODB_SERVER_SELECTION_TIMEOUT_MS))

// 3.1.2
// D_INTERNAL_MONGODB_APP_NAME
//   brief: D_CFG_MONGODB_APP_NAME, which the sources read through this name.
#define D_INTERNAL_MONGODB_APP_NAME D_CFG_MONGODB_APP_NAME


#endif  // DJINTERP_CONFIG_C_DB_MONGODB_CFG_MONGODB_CONNECTION_H
