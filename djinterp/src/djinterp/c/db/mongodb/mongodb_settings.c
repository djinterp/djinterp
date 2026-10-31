/*******************************************************************************
* djinterp [c]                                                mongodb_settings.c
*
* Implementation of mongodb_settings.h.
*   The concerns and the preference are the client's: the driver hands each
* collection the client's as it gets one, and the layer gets one for each
* operation, so a change applies from the next. Each is read from JSON as
* mongodb_internal.c reads it, which checks what the driver would send as
* written, and written back from the driver's own record: the write
* concern as the driver appends it to a command, the others field by field.
*   The document format and the time limit are the handle's alone.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_settings.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_settings.h"  // corresponding header
// std
#include <stddef.h>  // NULL
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "./mongodb_internal.h"  // mongoc.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_MONGODB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Writes a setting's document out, for a get.
 *
 * @param[in]  _db        the handle, whose format writes it.
 * @param[in]  _document  the document.
 * @param[out] _out       receives the text, written only on success.
 * @return 0, or D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_setting_out(
    const struct dj_mongodb* _db,
    const bson_t*            _document,
    char**                   _out
)
{
    char* const text = d_internal_mongodb_json(_document, _db->format);

    // room for it
    if (!text)
    {
        return D_MONGODB_LAYER_NO_MEMORY;
    }

    *_out = text;

    return 0;
}

/**
 * @brief The name a command gives a read mode.
 *
 * @param[in] _mode  the mode.
 * @return the name.
 */
static
const char* d_internal_mongodb_mode_name(
    mongoc_read_mode_t _mode
)
{
    switch (_mode)
    {
        case MONGOC_READ_PRIMARY_PREFERRED:
            return "primaryPreferred";

        case MONGOC_READ_SECONDARY:
            return "secondary";

        case MONGOC_READ_SECONDARY_PREFERRED:
            return "secondaryPreferred";

        case MONGOC_READ_NEAREST:
            return "nearest";

        default:
            return "primary";
    }
}


//==============================================================================
// 1.  SETTINGS
//==============================================================================

// 1.1    Read concern
//------------------------------------------------------------------------------
/*
d_mongodb_set_read_concern
  NULL, or a document with no level, is the driver's default read concern,
which sends none.
*/
int d_mongodb_set_read_concern(
    struct dj_mongodb* _db,
    const char*        _concern
)
{
    mongoc_read_concern_t* concern = NULL;
    bson_t                 document;

    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_read_document(_db,
                                                  _concern,
                                                  false,
                                                  &document);

    result = (result == 0)
                 ? d_internal_mongodb_read_concern(_db, &document, &concern)
                 : result;

    // the concern, or the default
    if (result == 0)
    {
        if (!concern)
        {
            concern = mongoc_read_concern_new();
        }

        mongoc_client_set_read_concern(_db->client, concern);
    }

    mongoc_read_concern_destroy(concern);
    bson_destroy(&document);

    return result;
}

/*
d_mongodb_get_read_concern
  {"level": ...}, or {} for the default.
*/
int d_mongodb_get_read_concern(
    const struct dj_mongodb* _db,
    char**                   _concern
)
{
    // a handle, and somewhere for the concern
    if ( (!_db) ||
         (!_concern) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const char* const level = mongoc_read_concern_get_level(
                                  mongoc_client_get_read_concern(_db->client));
    bson_t            document;

    bson_init(&document);

    // the level, where there is one
    if (level)
    {
        BSON_APPEND_UTF8(&document, "level", level);
    }

    const int result = d_internal_mongodb_setting_out(_db,
                                                      &document,
                                                      _concern);

    bson_destroy(&document);

    return result;
}

// 1.2    Write concern
//------------------------------------------------------------------------------
/*
d_mongodb_set_write_concern
  NULL, or an empty document, is the driver's default write concern, which
sends none.
*/
int d_mongodb_set_write_concern(
    struct dj_mongodb* _db,
    const char*        _concern
)
{
    mongoc_write_concern_t* concern = NULL;
    bson_t                  document;

    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_read_document(_db,
                                                  _concern,
                                                  false,
                                                  &document);

    result = (result == 0)
                 ? d_internal_mongodb_write_concern(_db, &document, &concern)
                 : result;

    // the concern, or the default
    if (result == 0)
    {
        if (!concern)
        {
            concern = mongoc_write_concern_new();
        }

        mongoc_client_set_write_concern(_db->client, concern);
    }

    mongoc_write_concern_destroy(concern);
    bson_destroy(&document);

    return result;
}

/*
d_mongodb_get_write_concern
  As the driver appends it to a command, its "writeConcern"; {} for the
default, which it appends none of.
*/
int d_mongodb_get_write_concern(
    const struct dj_mongodb* _db,
    char**                   _concern
)
{
    // a handle, and somewhere for the concern
    if ( (!_db) ||
         (!_concern) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    mongoc_write_concern_t* const copy =
        mongoc_write_concern_copy(mongoc_client_get_write_concern(_db->client));
    bson_t                        holder;
    bson_t                        document;

    bson_init(&holder);
    (void)mongoc_write_concern_append(copy, &holder);
    mongoc_write_concern_destroy(copy);

    // the concern appended, or none
    if (!d_internal_mongodb_subdocument(&holder, "writeConcern", &document))
    {
        bson_init(&document);
    }

    const int result = d_internal_mongodb_setting_out(_db,
                                                      &document,
                                                      _concern);

    bson_destroy(&document);
    bson_destroy(&holder);

    return result;
}

// 1.3    Read preference
//------------------------------------------------------------------------------
/*
d_mongodb_set_read_preference
  NULL is the primary.
*/
int d_mongodb_set_read_preference(
    struct dj_mongodb* _db,
    const char*        _preference
)
{
    mongoc_read_prefs_t* preference = NULL;
    bson_t               document;

    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_init(&document);

    int result = (_preference)
                     ? d_internal_mongodb_read_document(_db,
                                                        _preference,
                                                        true,
                                                        &document)
                     : 0;

    // the preference given, or the primary
    if (_preference)
    {
        result = (result == 0)
                     ? d_internal_mongodb_read_prefs(_db,
                                                     &document,
                                                     &preference)
                     : result;
    }
    else
    {
        preference = mongoc_read_prefs_new(MONGOC_READ_PRIMARY);
    }

    if (result == 0)
    {
        mongoc_client_set_read_prefs(_db->client, preference);
    }

    mongoc_read_prefs_destroy(preference);
    bson_destroy(&document);

    return result;
}

/*
d_mongodb_get_read_preference
  The mode by its name, the tags where there are any, the staleness where
there is one.
*/
int d_mongodb_get_read_preference(
    const struct dj_mongodb* _db,
    char**                   _preference
)
{
    // a handle, and somewhere for the preference
    if ( (!_db) ||
         (!_preference) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const mongoc_read_prefs_t* const preference =
        mongoc_client_get_read_prefs(_db->client);
    const bson_t* const              tags       =
        mongoc_read_prefs_get_tags(preference);
    const int64_t                    staleness  =
        mongoc_read_prefs_get_max_staleness_seconds(preference);
    bson_t                           document;

    bson_init(&document);
    BSON_APPEND_UTF8(&document,
                     "mode",
                     d_internal_mongodb_mode_name(
                         mongoc_read_prefs_get_mode(preference)));

    // tags and staleness, where set
    if ( (tags) &&
         (!bson_empty(tags)) )
    {
        BSON_APPEND_ARRAY(&document, "tags", tags);
    }

    if (staleness > 0)
    {
        BSON_APPEND_INT64(&document, "maxStalenessSeconds", staleness);
    }

    const int result = d_internal_mongodb_setting_out(_db,
                                                      &document,
                                                      _preference);

    bson_destroy(&document);

    return result;
}

// 1.4    Document format
//------------------------------------------------------------------------------
/*
d_mongodb_set_document_format
  The handle's alone.
*/
int
d_mongodb_set_document_format(
    struct dj_mongodb*              _db,
    enum dj_mongodb_document_format _format
)
{
    // a handle, and a format of the three
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if ( (_format != D_MONGODB_JSON_CANONICAL) &&
         (_format != D_MONGODB_JSON_RELAXED)   &&
         (_format != D_MONGODB_BSON) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "no document format %d",
                                         (int)_format);
    }

    _db->format = _format;

    return 0;
}

/*
d_mongodb_get_document_format
  As set, or as the knob said.
*/
enum dj_mongodb_document_format
d_mongodb_get_document_format(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->format : D_MONGODB_JSON_CANONICAL;
}

// 1.5    Time limit
//------------------------------------------------------------------------------
/*
d_mongodb_set_max_time_ms
  The handle's alone, read by each read's options.
*/
int d_mongodb_set_max_time_ms(
    struct dj_mongodb* _db,
    int64_t            _milliseconds
)
{
    // a handle, and a limit of 0 or more
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (_milliseconds < 0)
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "a time limit is 0 or more "
                                         "milliseconds");
    }

    _db->max_time_ms = _milliseconds;

    return 0;
}

/*
d_mongodb_get_max_time_ms
  As set.
*/
int64_t d_mongodb_get_max_time_ms(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->max_time_ms : 0;
}

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports what a build
// without the driver can: D_DB_UNSUPPORTED, or the default where it cannot
// fail.

/*
d_mongodb_set_read_concern
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_set_read_concern(
    struct dj_mongodb* _db,
    const char*        _concern
)
{
    (void)_db;
    (void)_concern;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_get_read_concern
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_get_read_concern(
    const struct dj_mongodb* _db,
    char**                   _concern
)
{
    (void)_db;
    (void)_concern;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_set_write_concern
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_set_write_concern(
    struct dj_mongodb* _db,
    const char*        _concern
)
{
    (void)_db;
    (void)_concern;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_get_write_concern
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_get_write_concern(
    const struct dj_mongodb* _db,
    char**                   _concern
)
{
    (void)_db;
    (void)_concern;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_set_read_preference
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_set_read_preference(
    struct dj_mongodb* _db,
    const char*        _preference
)
{
    (void)_db;
    (void)_preference;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_get_read_preference
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_get_read_preference(
    const struct dj_mongodb* _db,
    char**                   _preference
)
{
    (void)_db;
    (void)_preference;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_set_document_format
  Without the driver, D_DB_UNSUPPORTED.
*/
int
d_mongodb_set_document_format(
    struct dj_mongodb*              _db,
    enum dj_mongodb_document_format _format
)
{
    (void)_db;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_get_document_format
  Without the driver, the canonical format.
*/
enum dj_mongodb_document_format
d_mongodb_get_document_format(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return D_MONGODB_JSON_CANONICAL;
}

/*
d_mongodb_set_max_time_ms
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_set_max_time_ms(
    struct dj_mongodb* _db,
    int64_t            _milliseconds
)
{
    (void)_db;
    (void)_milliseconds;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_get_max_time_ms
  Without the driver, 0.
*/
int64_t d_mongodb_get_max_time_ms(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return 0;
}

#endif  // D_INTERNAL_MONGODB == 1
