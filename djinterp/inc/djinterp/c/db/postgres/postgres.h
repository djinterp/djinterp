/*******************************************************************************
* djinterp [c]                                                        postgres.h
*
* Umbrella for djinterp's PostgreSQL convenience layer -- includes every
* module.
*   Convenience, not architecture: each module is its own source, so a
* program that only connects and checks for a table links postgres_common,
* postgres_connection and postgres_schema, and nothing else. Include the
* modules you call in a library; include this in a program or a test, where
* the rest costs nothing that matters.
*   The modules work on libpq's own connection, PGconn, and none of them
* includes libpq-fe.h; include it as well to call libpq directly, passing the
* same pointer. Build the sources against libpq 10 or later and link it
* (-lpq) -- or, where there is none, as stubs that report D_DB_UNSUPPORTED
* (D_CFG_POSTGRES).
*
*
* path:      /inc/djinterp/c/db/postgres/postgres.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.15
*******************************************************************************/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_H 1

// djinterp
#include "./postgres_common.h"       // foundation: results, servers, errors
#include "./postgres_connection.h"   // connect, close, ping
#include "./postgres_query.h"        // exec, rows changed, single values,
                                     // rows, quoting
#include "./postgres_statement.h"    // prepare, bind, execute, finalize
#include "./postgres_database.h"     // databases, schemas, extensions
#include "./postgres_schema.h"       // tables, columns, indexes, views,
                                     // triggers, sequences
#include "./postgres_transaction.h"  // begin, commit, rollback, savepoints
#include "./postgres_settings.h"     // search path, timeouts, isolation,
                                     // parameters
#include "./postgres_maintenance.h"  // vacuum, analyze, reindex, sizes
#include "./postgres_protocol.h"     // COPY in, notifications, statements
                                     // sent without waiting


#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_H
