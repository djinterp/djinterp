/*******************************************************************************
* djinterp [config]                                     cfg_mariadb_connection.h
*
* Build-time configuration for c/db/mariadb/mariadb_connection.h: what a
* connection d_mariadb_connect makes is set up with before it is handed back.
*   The character set is the layer's, D_CFG_MARIADB_CHARSET. Beyond it, a
* bounded connect timeout, so that a server that does not answer is reported
* rather than waited on for as long as TCP is willing to wait. A connection
* made with the client's own mysql_real_connect and adopted gets none of it.
*
*   targets:  c/db/mariadb/mariadb_connection.h, mariadb_connection.c ->
*             D_INTERNAL_MARIADB_CONNECT_*
*   requires: cfg_mariadb_common.h
*
*
* path:      /inc/djinterp/config/c/db/mariadb/cfg_mariadb_connection.h
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
         1.  D_CFG_MARIADB_CONNECT_TIMEOUT_S
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_MARIADB_CONNECT_TIMEOUT_S
*/

#ifndef DJINTERP_CONFIG_C_DB_MARIADB_CFG_MARIADB_CONNECTION_H
#define DJINTERP_CONFIG_C_DB_MARIADB_CFG_MARIADB_CONNECTION_H 1

// djinterp
#include "./cfg_mariadb_common.h"  // the layer's shared configuration


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Connection defaults
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_MARIADB_CONNECT_TIMEOUT_S
//   knob: how long, in seconds, a connection the layer makes waits for the
// server to answer; 0 leaves the client's default. Defaults to 10.
#ifndef D_CFG_MARIADB_CONNECT_TIMEOUT_S
    #define D_CFG_MARIADB_CONNECT_TIMEOUT_S 10
#endif  // D_CFG_MARIADB_CONNECT_TIMEOUT_S


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
// validation: a timeout is a count of seconds, never negative
#if ((D_CFG_MARIADB_CONNECT_TIMEOUT_S + 0) < 0)
    #error "D_CFG_MARIADB_CONNECT_TIMEOUT_S must not be negative"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_MARIADB_CONNECT_TIMEOUT_S
//   brief: the connect timeout the layer passes to the client, as unsigned.
#define D_INTERNAL_MARIADB_CONNECT_TIMEOUT_S                                   \
    ((unsigned)(D_CFG_MARIADB_CONNECT_TIMEOUT_S))


#endif  // DJINTERP_CONFIG_C_DB_MARIADB_CFG_MARIADB_CONNECTION_H
