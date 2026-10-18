/*******************************************************************************
* djinterp [c]                                                         mariadb.h
*
* Umbrella for djinterp's MariaDB/MySQL convenience layer -- includes every
* module.
*   Convenience, not architecture: each module is its own source, so a
* program that only connects and checks for a table links mariadb_common,
* mariadb_connection and mariadb_schema, and nothing else. Include the
* modules you call in a library; include this in a program or a test, where
* the rest costs nothing that matters.
*   The modules work on the layer's struct d_mariadb, and none of them
* includes mysql.h; include it as well, and cast d_mariadb_native's answer,
* to call the client directly. Build the sources against libmysqlclient 5.7
* or a MariaDB client, and link it (-lmariadb, or -lmysqlclient) -- or,
* where there is neither, as stubs that report D_DB_UNSUPPORTED
* (D_CFG_MARIADB).
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_H
#define DJINTERP_C_DB_MARIADB_MARIADB_H 1

// djinterp
#include "./mariadb_common.h"       // foundation: client, servers, errors
#include "./mariadb_connection.h"   // connect, adopt, close, ids, changes
#include "./mariadb_query.h"        // exec, scripts, single values, rows,
                                    // quoting
#include "./mariadb_statement.h"    // prepare, bind, execute, finalize
#include "./mariadb_database.h"     // databases, the current database
#include "./mariadb_schema.h"       // tables, columns, indexes, views,
                                    // triggers
#include "./mariadb_transaction.h"  // begin, commit, rollback, savepoints
#include "./mariadb_settings.h"     // autocommit, isolation, session
                                    // variables
#include "./mariadb_maintenance.h"  // analyze, optimize, check, checksum


#endif  // DJINTERP_C_DB_MARIADB_MARIADB_H
