/*******************************************************************************
* djinterp [c]                                                mariadb_database.h
*
* The databases on a MariaDB or MySQL server, and the connection's current
* one.
*   A server holds many databases -- what SQL calls schemas, and what SQLite
* calls attached databases -- and SQL names an object in one as
* database.object. So these are the functions the other layers spell the
* same way: d_mariadb_has_database asks whether a database exists on the
* server, as d_sqlite_has_database asks whether one is attached.
*   A database the layer creates takes the layer's character set,
* D_CFG_MARIADB_CHARSET (utf8mb4 unless the build says otherwise).
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_database.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DATABASES
    ---------
    1.  Existence
    2.  Creating and dropping
    3.  Listing
2.  THE CURRENT DATABASE
    --------------------
    1.  Choosing and asking
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_DATABASE_H
#define DJINTERP_C_DB_MARIADB_MARIADB_DATABASE_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "../db_common.h"      // d_db_name_list, D_DB_IF_*
#include "./mariadb_common.h"  // d_mariadb, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  DATABASES
//==============================================================================


// 1.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the server has a database of that name, as the
// server's own lower_case_table_names setting matches names. The plain form
// answers false for a failure or a bad argument; the _s form returns 0 and
// writes the answer, or the failure, the answer then false.
bool   d_mariadb_has_database(struct d_mariadb* _db,
                              const char*       _name) D_NOTHROW;
int    d_mariadb_has_database_s(struct d_mariadb* _db,
                                const char*       _name,
                                bool*             _exists) D_NOTHROW;

// 1.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Creates a database, in the layer's character set.
 *
 * @param[in] _db     the connection.
 * @param[in] _name   the database.
 * @param[in] _flags  0, or D_DB_IF_NOT_EXISTS to succeed when it is there.
 * @return 0, the client's error number for a failure (1007, it exists; 1044,
 *         no privilege), or D_DB_INVALID_ARGUMENT for a `NULL` argument, an
 *         empty name or another flag.
 */
int    d_mariadb_create_database(struct d_mariadb* _db,
                                 const char*       _name,
                                 unsigned          _flags) D_NOTHROW;
/**
 * @brief Drops a database and everything in it.
 *
 * @param[in] _db     the connection.
 * @param[in] _name   the database.
 * @param[in] _flags  0, or D_DB_IF_EXISTS to succeed when it is not there.
 * @return 0, the client's error number for a failure (1008, no such
 *         database), or D_DB_INVALID_ARGUMENT for a `NULL` argument, an empty
 *         name or another flag.
 */
int    d_mariadb_drop_database(struct d_mariadb* _db,
                               const char*       _name,
                               unsigned          _flags) D_NOTHROW;

// 1.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists the databases the connection can see, the server's own among
 *        them (information_schema, mysql, ...), sorted by name, byte by
 *        byte.
 *
 * @param[in]  _db     the connection.
 * @param[out] _names  receives the names; written only on success.
 * @return 0, the client's error number for a failure, CR_OUT_OF_MEMORY (2008)
 *         when the list could not be built, or D_DB_INVALID_ARGUMENT for a
 *         `NULL` argument.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int    d_mariadb_list_databases(struct d_mariadb*      _db,
                                struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 2.  THE CURRENT DATABASE
//==============================================================================


// 2.1    Choosing and asking
//------------------------------------------------------------------------------
// the current database -- the one unqualified names, and every NULL schema the
// layer is given, refer to. _use_database makes one current: 0, the client's
// error number (1049, no such database), or D_DB_INVALID_ARGUMENT for a NULL
// argument or an empty name. _current_database names it, in a string the
// caller releases with d_db_free: 0, D_DB_NOT_FOUND when none is current, or a
// failure as for the queries.
int    d_mariadb_use_database(struct d_mariadb* _db,
                              const char*       _name) D_NOTHROW;
int    d_mariadb_current_database(struct d_mariadb* _db,
                                  char**            _name) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MARIADB_MARIADB_DATABASE_H
