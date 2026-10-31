/*******************************************************************************
* djinterp [c]                                                         mongodb.h
*
* Umbrella for djinterp's MongoDB convenience layer -- includes every module.
*   Convenience, not architecture: each module is its own source, so a
* program that only connects and inserts links mongodb_common,
* mongodb_connection and mongodb_document, mongodb_internal beneath them, and
* nothing else. Include the modules you call in a library; include this in a
* program or a test, where the rest costs nothing that matters.
*   The modules work through the layer's handle, struct dj_mongodb, around the
* driver's mongoc_client_t, and none of them includes mongoc.h; include it
* as well to call the driver directly, with d_mongodb_native's client. Build
* the sources against MongoDB's C driver, 1.21 or later of either line, and
* link it (pkg-config's libmongoc-1.0 or mongoc2) -- or, where there is none,
* as stubs that report D_DB_UNSUPPORTED (D_CFG_MONGODB).
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_H
#define DJINTERP_C_DB_MONGODB_MONGODB_H 1

// djinterp
#include "./mongodb_common.h"       // foundation: results, servers, errors
#include "./mongodb_connection.h"   // connect, adopt, close, ping, the latest
                                    // write
#include "./mongodb_query.h"        // commands: run, reply, documents
#include "./mongodb_document.h"     // insert, find, update, delete, count,
                                    // aggregate, bulk writes
#include "./mongodb_cursor.h"       // cursors, change streams
#include "./mongodb_database.h"     // databases, the current one
#include "./mongodb_schema.h"       // collections, views, indexes
#include "./mongodb_transaction.h"  // begin, commit, rollback
#include "./mongodb_settings.h"     // concerns, read preference, document
                                    // format, time limit
#include "./mongodb_gridfs.h"       // files in a bucket
#include "./mongodb_maintenance.h"  // sizes, compact, validate


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_H
