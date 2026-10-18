/*******************************************************************************
* djinterp [c]                                                          sqlite.h
*
* Umbrella for djinterp's SQLite convenience layer -- includes every module.
*   Convenience, not architecture: each module is its own source, so a
* program that only opens a database and checks for a table links
* sqlite_common, sqlite_connection and sqlite_schema, and nothing else.
* Include the modules you call in a library; include this in a program or a
* test, where the rest costs nothing that matters.
*   The modules' functions all work on SQLite's own sqlite3*, and none of them
* includes sqlite3.h; include it as well to call SQLite directly. Build the
* sources against SQLite 3.7.17 or later, and link SQLite -- or, where there
* is none, as stubs that report D_DB_UNSUPPORTED (D_CFG_SQLITE).
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_H
#define DJINTERP_C_DB_SQLITE_SQLITE_H 1

// djinterp
#include "./sqlite_common.h"       // foundation: versions, features, results
#include "./sqlite_connection.h"   // open, close, path, row ids, changes
#include "./sqlite_query.h"        // exec, single values, rows, quoting
#include "./sqlite_statement.h"    // prepare, bind, execute, finalize
#include "./sqlite_database.h"     // attached databases, database files
#include "./sqlite_schema.h"       // tables, columns, indexes, views, triggers
#include "./sqlite_transaction.h"  // begin, commit, rollback, savepoints
#include "./sqlite_settings.h"     // foreign keys, journal, versions, pragmas
#include "./sqlite_maintenance.h"  // vacuum, analyze, checks, backups


#endif  // DJINTERP_C_DB_SQLITE_SQLITE_H
