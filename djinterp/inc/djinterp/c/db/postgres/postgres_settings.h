/*******************************************************************************
* djinterp [c]                                               postgres_settings.h
*
* Session settings on a PostgreSQL connection, in pairs: each set_ has its
* get_.
*   The server calls them run-time parameters: search_path, statement_timeout,
* TimeZone and some three hundred more. The common ones have functions of
* their own, which take and give values in C's terms -- milliseconds, an
* enumeration -- and d_postgres_set_parameter and d_postgres_get_parameter
* reach any other by name, as text. A setting lasts for the session, unless
* the transaction it was made in rolls back, which undoes it; RESET or a new
* connection restores the server's default.
*   The client encoding and the time zone are read from libpq's copy, which
* the server keeps current, reporting every change; libpq quotes values for
* the encoding it last reported.
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_settings.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SETTINGS
    --------
    1.  Isolation levels
         1.  d_postgres_isolation
    2.  The search path
    3.  Timeouts
    4.  Isolation
    5.  Encoding and time zone
    6.  Any parameter
*/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_SETTINGS_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_SETTINGS_H 1

// djinterp
#include "../../djinterp.h"     // framework root
#include "./postgres_common.h"  // pg_conn, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SETTINGS
//==============================================================================
// Each set_ returns 0, the SQLSTATE of a failure (22023, a value the server
// refuses), or D_DB_INVALID_ARGUMENT for a NULL argument or a value outside
// the function's range; each get_ returns 0 and writes the value -- a string
// the caller releases with d_db_free -- or a failure as the set_ does, writing
// nothing.


// 1.1    Isolation levels
//------------------------------------------------------------------------------
// 1.1.1
// d_postgres_isolation
//   enum: the SQL standard's isolation levels, as the server takes them.
enum d_postgres_isolation
{
    D_POSTGRES_ISOLATION_READ_UNCOMMITTED = 0,  // taken, and run as READ
                                                // COMMITTED
    D_POSTGRES_ISOLATION_READ_COMMITTED,        // the server's default
    D_POSTGRES_ISOLATION_REPEATABLE_READ,
    D_POSTGRES_ISOLATION_SERIALIZABLE
};

// 1.2    The search path
//------------------------------------------------------------------------------
// the search path -- the schemas unqualified names are looked for in, in
// order, as SQL writes the list: "app, public", with any name that needs
// them in double quotes. The first that exists is the current schema, which
// every NULL schema the layer is given means.
int    d_postgres_set_search_path(struct pg_conn* _db,
                                  const char*     _path) D_NOTHROW;
int    d_postgres_get_search_path(struct pg_conn* _db,
                                  char**          _path) D_NOTHROW;

// 1.3    Timeouts
//------------------------------------------------------------------------------
// timeouts, in milliseconds, 0 meaning none, the server's default: how long
// a statement may run before the server cancels it (statement_timeout), and
// how long one may wait for a lock (lock_timeout), each failing with 57014 or
// 55P03 when it runs out. Up to 2147483647 (24 days and more).
int    d_postgres_set_statement_timeout(struct pg_conn* _db,
                                        unsigned        _milliseconds)
                                        D_NOTHROW;
int    d_postgres_get_statement_timeout(struct pg_conn* _db,
                                        unsigned*       _milliseconds)
                                        D_NOTHROW;
int    d_postgres_set_lock_timeout(struct pg_conn* _db,
                                   unsigned        _milliseconds) D_NOTHROW;
int    d_postgres_get_lock_timeout(struct pg_conn* _db,
                                   unsigned*       _milliseconds) D_NOTHROW;

// 1.4    Isolation
//------------------------------------------------------------------------------
// isolation -- the level the session's transactions begin with
// (default_transaction_isolation), unless d_postgres_begin_transaction asks
// for another.
int    d_postgres_set_isolation_level(struct pg_conn*           _db,
                                      enum d_postgres_isolation _level)
                                      D_NOTHROW;
int    d_postgres_get_isolation_level(struct pg_conn*            _db,
                                      enum d_postgres_isolation* _level)
                                      D_NOTHROW;

// 1.5    Encoding and time zone
//------------------------------------------------------------------------------
// encoding and time zone -- the client encoding, as the server names it
// ("UTF8", "LATIN1"), which libpq then quotes values for; and the time zone,
// as the server names it ("UTC", "Europe/Berlin", "+02"), which timestamptz
// values are shown in.
int    d_postgres_set_client_encoding(struct pg_conn* _db,
                                      const char*     _encoding) D_NOTHROW;
int    d_postgres_get_client_encoding(struct pg_conn* _db,
                                      char**          _encoding) D_NOTHROW;
int    d_postgres_set_time_zone(struct pg_conn* _db,
                                const char*     _zone) D_NOTHROW;
int    d_postgres_get_time_zone(struct pg_conn* _db,
                                char**          _zone) D_NOTHROW;

// 1.6    Any parameter
//------------------------------------------------------------------------------
// any parameter -- by its name, letters, digits, underscores and, for an
// extension's own, a dot ("work_mem", "myapp.tenant"), and its value as text,
// as SET takes it ("64MB", "on"): _set_parameter sets it for the session, as
// set_config(name, value, false) does; _get_parameter reads it, as
// current_setting() does. An unknown name is 42704.
int    d_postgres_set_parameter(struct pg_conn* _db,
                                const char*     _name,
                                const char*     _value) D_NOTHROW;
int    d_postgres_get_parameter(struct pg_conn* _db,
                                const char*     _name,
                                char**          _value) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_SETTINGS_H
