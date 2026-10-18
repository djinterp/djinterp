/*******************************************************************************
* djinterp [c]                                                mariadb_settings.h
*
* A connection's session settings, read and written in pairs.
*   Each setting here is the connection's own: a SET SESSION, or the client's
* call for it, which lasts until the connection closes and touches no other.
* The named pairs cover what programs change most -- autocommit, isolation,
* the character set, the time zone, the SQL mode, how long to wait for a
* row lock -- and set_variable and get_variable reach any other session
* variable by name.
*   Getters write their answer only on success. A string they return is the
* caller's, released with d_db_free.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_settings.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALUES
    ------
    1.  Enumerated settings
         1.  d_mariadb_isolation
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


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MARIADB_MARIADB_SETTINGS_H
