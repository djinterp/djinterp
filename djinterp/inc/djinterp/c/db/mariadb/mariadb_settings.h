/*******************************************************************************
* djinterp [c]                                                mariadb_settings.h
*
* A connection's session settings, read and written in pairs, and what the
* server reports changing in the session.
*   Each setting here is the connection's own: a SET SESSION, or the client's
* call for it, which lasts until the connection closes and touches no other.
* The named pairs cover what programs change most -- autocommit, isolation,
* the character set, the time zone, the SQL mode, how long to wait for a
* row lock -- and set_variable and get_variable reach any other session
* variable by name. d_mariadb_session_track reads what a statement changed
* in the session, as the server reports it with its reply.
*   Getters write their answer only on success. A string they return is the
* caller's, released with d_db_free.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_settings.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALUES
    ------
    1.  Enumerated settings
         1.  d_mariadb_isolation
    2.  Session state
         1.  d_mariadb_track
2.  TRANSACTION SETTINGS
    --------------------
    1.  Autocommit
    2.  Isolation level
    3.  Lock waits
3.  SESSION SETTINGS
    ----------------
    1.  Character set
    2.  Time zone
    3.  SQL mode
4.  ANY VARIABLE
    ------------
    1.  Setting and getting
5.  SESSION STATE
    -------------
    1.  Changes reported
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_SETTINGS_H
#define DJINTERP_C_DB_MARIADB_MARIADB_SETTINGS_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "./mariadb_common.h"  // d_mariadb, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  VALUES
//==============================================================================


// 1.1    Enumerated settings
//------------------------------------------------------------------------------
// 1.1.1
// d_mariadb_isolation
//   enum: the transaction isolation levels, weakest first.
enum d_mariadb_isolation
{
    D_MARIADB_ISOLATION_READ_UNCOMMITTED = 0,
    D_MARIADB_ISOLATION_READ_COMMITTED,
    D_MARIADB_ISOLATION_REPEATABLE_READ,   // both servers' default
    D_MARIADB_ISOLATION_SERIALIZABLE
};

// 1.2    Session state
//------------------------------------------------------------------------------
// 1.2.1
// d_mariadb_track
//   enum: the kinds of session state a server reports changes to, numbered
// as the protocol numbers them (enum_session_state_type). A server reports
// a kind only as the session variable named here asks.
enum d_mariadb_track
{
    D_MARIADB_TRACK_SYSTEM_VARIABLES = 0,     // each variable changed, then
                                              // its value; those
                                              // session_track_system_variables
                                              // names: by default autocommit,
                                              // the character sets, time_zone
    D_MARIADB_TRACK_SCHEMA,                   // the new current database
                                              // (session_track_schema, on)
    D_MARIADB_TRACK_STATE_CHANGE,             // "1" for any change at all
                                              // (session_track_state_change,
                                              // off)
    D_MARIADB_TRACK_GTIDS,                    // the transaction's GTIDs
                                              // (session_track_gtids, off)
    D_MARIADB_TRACK_TRANSACTION_CHARACTERISTICS,
                                              // the SQL that would begin the
                                              // transaction again
                                              // (session_track_transaction_
                                              // info, CHARACTERISTICS)
    D_MARIADB_TRACK_TRANSACTION_STATE         // eight characters, '_' or a
                                              // letter for each of the
                                              // transaction's states, as
                                              // "T_______" for one open
                                              // (session_track_transaction_
                                              // info, STATE or more)
};


//==============================================================================
// 2.  TRANSACTION SETTINGS
//==============================================================================
// Each returns 0, the client's error number for a failure, or
// D_DB_INVALID_ARGUMENT for a NULL handle or output, or a value outside its
// range.


// 2.1    Autocommit
//------------------------------------------------------------------------------
// autocommit -- whether each statement commits by itself, as it does by
// default; turning it off leaves a transaction open until commit or rollback.
// The getter reads the status the server sent with its last reply, and asks
// nothing
int    d_mariadb_set_autocommit(struct d_mariadb* _db,
                                bool              _enabled) D_NOTHROW;
int    d_mariadb_get_autocommit(struct d_mariadb* _db,
                                bool*             _enabled) D_NOTHROW;

// 2.2    Isolation level
//------------------------------------------------------------------------------
// isolation level -- for the transactions the session begins from now on;
// _set_next for the next transaction alone, begun explicitly or by a
// statement with autocommit off, after which the session's level holds
// again. The server refuses _set_next inside a transaction (1568).
int    d_mariadb_set_isolation_level(struct d_mariadb*        _db,
                                     enum d_mariadb_isolation _level) D_NOTHROW;
int    d_mariadb_get_isolation_level(struct d_mariadb*         _db,
                                     enum d_mariadb_isolation* _level)
                                     D_NOTHROW;
int    d_mariadb_set_next_isolation_level(struct d_mariadb*        _db,
                                          enum d_mariadb_isolation _level)
                                          D_NOTHROW;

// 2.3    Lock waits
//------------------------------------------------------------------------------
// lock waits -- how long, in seconds, a statement waits for a row lock before
// failing with 1205 (innodb_lock_wait_timeout); 1 to 1073741824, MySQL's
// ceiling, which MariaDB lowers to its own
int    d_mariadb_set_lock_wait_timeout(struct d_mariadb* _db,
                                       unsigned          _seconds) D_NOTHROW;
int    d_mariadb_get_lock_wait_timeout(struct d_mariadb* _db,
                                       unsigned*         _seconds) D_NOTHROW;


//==============================================================================
// 3.  SESSION SETTINGS
//==============================================================================
// Each returns 0, the client's error number for a failure (2019, a character
// set the client does not know; 1298, unknown time zone; 1231, a value the
// variable does not take), CR_OUT_OF_MEMORY (2008), or D_DB_INVALID_ARGUMENT
// for a NULL argument.


// 3.1    Character set
//------------------------------------------------------------------------------
// character set -- the connection's, through the client's own call, so that
// the client quotes values for it too; the getter reads the client's record
// of it, and asks nothing
int    d_mariadb_set_charset(struct d_mariadb* _db,
                             const char*       _charset) D_NOTHROW;
int    d_mariadb_get_charset(struct d_mariadb* _db,
                             char**            _charset) D_NOTHROW;

// 3.2    Time zone
//------------------------------------------------------------------------------
// time zone -- "+00:00", "SYSTEM", or a name such as "Europe/Paris" where the
// server has its time zone tables loaded
int    d_mariadb_set_time_zone(struct d_mariadb* _db,
                               const char*       _zone) D_NOTHROW;
int    d_mariadb_get_time_zone(struct d_mariadb* _db,
                               char**            _zone) D_NOTHROW;

// 3.3    SQL mode
//------------------------------------------------------------------------------
// SQL mode -- a comma-separated list, such as
// "STRICT_TRANS_TABLES,NO_ZERO_DATE"; "" for none
int    d_mariadb_set_sql_mode(struct d_mariadb* _db,
                              const char*       _mode) D_NOTHROW;
int    d_mariadb_get_sql_mode(struct d_mariadb* _db,
                              char**            _mode) D_NOTHROW;


//==============================================================================
// 4.  ANY VARIABLE
//==============================================================================


// 4.1    Setting and getting
//------------------------------------------------------------------------------
/**
 * @brief Sets a session variable to the value of an SQL expression.
 *
 * @param[in] _db     the connection.
 * @param[in] _name   the variable, e.g. "wait_timeout"; a letter or
 *                    underscore, then letters, digits and underscores, which
 *                    the layer checks, since a name cannot be quoted here.
 * @param[in] _value  the value as SQL, used as written: "28800", "'UTC'",
 *                    "DEFAULT".
 * @return 0, the client's error number (1193, unknown variable; 1231, a
 *         value it does not take), or D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument or a name that is not one.
 */
int    d_mariadb_set_variable(struct d_mariadb* _db,
                              const char*       _name,
                              const char*       _value) D_NOTHROW;
/**
 * @brief Reads a session variable, as text.
 *
 * @param[in]  _db     the connection.
 * @param[in]  _name   the variable, checked as for d_mariadb_set_variable.
 * @param[out] _value  receives the value; NULL for a NULL value; written
 *                     only on success.
 * @return 0, the client's error number (1193, unknown variable),
 *         CR_OUT_OF_MEMORY (2008), or D_DB_INVALID_ARGUMENT.
 * @post On success the caller releases `*_value` with d_db_free.
 */
int    d_mariadb_get_variable(struct d_mariadb* _db,
                              const char*       _name,
                              char**            _value) D_NOTHROW;


//==============================================================================
// 5.  SESSION STATE
//==============================================================================


// 5.1    Changes reported
//------------------------------------------------------------------------------
/**
 * @brief Reads what the server reported changing in the session, of one
 *        kind, with its reply to the last command the layer sent: a
 *        statement, the layer's own among them -- d_mariadb_has_table asks
 *        one -- a change of database, user or character set, a reset.
 *
 * @note The server reports changes with the OK packet that ends a reply.
 *       Through Connector/C, rows end without one, so a statement that
 *       returns rows reports none, and the client does not read the one
 *       that answers a command without a statement, so neither
 *       d_mariadb_use_database nor a reset reports any; libmysqlclient
 *       reads them all. A reply that failed, or that carried none, a ping's
 *       among them, reports none, though Connector/C would describe an
 *       earlier reply's. A command a program sends with the client's own
 *       calls is not the layer's.
 *
 * @param[in]  _db       the connection.
 * @param[in]  _kind     the kind of state.
 * @param[out] _entries  receives the entries, in the order the server sent
 *                       them -- for system variables a name and its value,
 *                       one after the other, for each -- and none when the
 *                       reply reported nothing of the kind; written only on
 *                       success. An entry with a NUL byte in it reads only
 *                       to the NUL.
 * @return 0, CR_OUT_OF_MEMORY (2008), D_DB_UNSUPPORTED where the server is
 *         older than MySQL 5.7.4 or MariaDB 10.2.2, or the client has no
 *         session tracking (a MariaDB client before Connector/C 3.0), or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument or a kind outside enum
 *         d_mariadb_track.
 * @post On success the caller releases `_entries` with d_db_name_list_free.
 */
int    d_mariadb_session_track(struct d_mariadb*      _db,
                               enum d_mariadb_track   _kind,
                               struct d_db_name_list* _entries) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MARIADB_MARIADB_SETTINGS_H
