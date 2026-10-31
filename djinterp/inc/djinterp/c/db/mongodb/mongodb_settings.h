/*******************************************************************************
* djinterp [c]                                                mongodb_settings.h
*
* What a handle's operations read with and write with, and how it writes
* documents.
*   The read concern says how current and how durable what a read sees must
* be (local, available, majority, linearizable, snapshot); the write concern
* how many members must have a write, and journalled, before it counts; the
* read preference which members a read may use. Each is the handle's
* default, which the document functions, the cursors and a transaction
* follow (mongodb_document.h, mongodb_cursor.h, mongodb_transaction.h), and
* which their options may replace for one operation; commands run as
* written (mongodb_query.h). Each is given and read back as MongoDB writes it
* in a command -- {"level": "majority"}, {"w": "majority", "j": true,
* "wtimeout": 5000}, {"mode": "secondaryPreferred", "tags": [{"dc": "east"}],
* "maxStalenessSeconds": 120} -- as JSON, NULL for the deployment's own
* default; a connection string's own, read when the handle is made, is the
* starting value.
*   The document format is how the layer writes the documents it returns
* (mongodb_common.h, dj_mongodb_document_format); the time limit, the
* maxTimeMS the layer gives each read -- a find, an aggregation, a count, a
* distinct, a find-and-modify -- whose options name none, 0 for none.
*   Each `set` returns 0; D_DB_INVALID_ARGUMENT for a NULL handle, JSON that
* is not the document the setting is, a level, a mode or a w of no such
* name, or a value out of range; libbson's code, packed, for text that is
* not JSON. Each `get` writes its answer only on success, released with
* d_db_free.
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_settings.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SETTINGS
    --------
    1.  Read concern
    2.  Write concern
    3.  Read preference
    4.  Document format
    5.  Time limit
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_SETTINGS_H
#define DJINTERP_C_DB_MONGODB_MONGODB_SETTINGS_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "./mongodb_common.h"  // dj_mongodb, dj_mongodb_document_format
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // INT64_MAX: section 1.5's
                                                 // floor; int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SETTINGS
//==============================================================================


// 1.1    Read concern
//------------------------------------------------------------------------------
// read concern -- the handle's read concern, as {"level": "..."}, NULL or
// "{}" for the deployment's default; read back as JSON, "{}" for the default
int d_mongodb_set_read_concern(
    struct dj_mongodb* _db,
    const char*        _concern
) D_NOTHROW;
int d_mongodb_get_read_concern(
    const struct dj_mongodb* _db,
    char**                   _concern
) D_NOTHROW;

// 1.2    Write concern
//------------------------------------------------------------------------------
// write concern -- the handle's write concern: w, a count of members or
// "majority" or a tag set's name; j, whether journalled; wtimeout, in
// milliseconds; NULL or "{}" for the deployment's default. A w of 0, which
// asks for no acknowledgement, is refused, since the layer reports what
// every write did; read back as JSON, "{}" for the default
int d_mongodb_set_write_concern(
    struct dj_mongodb* _db,
    const char*        _concern
) D_NOTHROW;
int d_mongodb_get_write_concern(
    const struct dj_mongodb* _db,
    char**                   _concern
) D_NOTHROW;

// 1.3    Read preference
//------------------------------------------------------------------------------
// read preference -- the handle's read preference: a mode (primary,
// primaryPreferred, secondary, secondaryPreferred, nearest), tag sets, a
// maxStalenessSeconds of 90 or more, which the primary mode takes neither of;
// NULL for primary; read back as JSON
int d_mongodb_set_read_preference(
    struct dj_mongodb* _db,
    const char*        _preference
) D_NOTHROW;
int d_mongodb_get_read_preference(
    const struct dj_mongodb* _db,
    char**                   _preference
) D_NOTHROW;

// 1.4    Document format
//------------------------------------------------------------------------------
// document format -- how the handle writes the documents it returns; a value
// outside enum dj_mongodb_document_format is D_DB_INVALID_ARGUMENT. Read
// back, D_MONGODB_JSON_CANONICAL for a NULL handle
int d_mongodb_set_document_format(
    struct dj_mongodb*              _db,
    enum dj_mongodb_document_format _format
) D_NOTHROW;
enum dj_mongodb_document_format d_mongodb_get_document_format(
    const struct dj_mongodb* _db
) D_NOTHROW;

#if defined(INT64_MAX)
// 1.5    Time limit
//------------------------------------------------------------------------------
// time limit -- the maxTimeMS, in milliseconds, the layer gives each read
// whose options name none, 0 for none; a negative one is
// D_DB_INVALID_ARGUMENT. A read that runs past it fails with the server's 50,
// MaxTimeMSExpired. Read back, 0 for a NULL handle
int d_mongodb_set_max_time_ms(
    struct dj_mongodb* _db,
    int64_t            _milliseconds
) D_NOTHROW;
int64_t d_mongodb_get_max_time_ms(
    const struct dj_mongodb* _db
) D_NOTHROW;
#endif  // defined(INT64_MAX)


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_SETTINGS_H
