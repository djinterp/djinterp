/*******************************************************************************
* djinterp [c]                                                   mongodb_query.c
*
* Implementation of mongodb_query.h.
*   A command runs through mongoc_client_command_with_opts, which applies
* none of the client's concerns or preference -- the command runs as written
* -- on the primary. A write's count is read by what the command is, its
* first field: insert, update, delete and findAndModify count their
* documents, and no other command writes any, though count's reply has an
* n too.
*   A command that returns a cursor is read to its end through the driver's
* cursor, made from the reply, so that every getMore goes where the command
* went: the server is selected first and both are sent to it, or, inside a
* transaction, the session takes them where it is pinned.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_query.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_query.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
#include <string.h>  // strcmp
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, dj_db_rows
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_document.h"  // counts
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "../db_internal.h"       // the rows builder
#include "./mongodb_internal.h"  // mongoc.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t, int32_t


#if (D_INTERNAL_MONGODB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// dj_internal_mongodb_found_id
//   struct: an _id read from a reply, copied out of the iterator that found
// it, which bson_iter_value points into; its own pointers are into the
// reply's bytes, valid while the reply is.
struct dj_internal_mongodb_found_id
{
    bool         found;  // whether there is one
    bson_value_t value;  // the value
};

/**
 * @brief Reads a number from a document by its dotted path.
 *
 * @param[in] _document  the document.
 * @param[in] _path      the path.
 * @return the number; 0 where it is not there, or not a number.
 */
static
int64_t d_internal_mongodb_number_at(
    const bson_t* _document,
    const char*   _path
)
{
    bson_iter_t top;
    bson_iter_t found;

    return ( (bson_iter_init(&top, _document))                    &&
             (bson_iter_find_descendant(&top, _path, &found))     &&
             (BSON_ITER_HOLDS_NUMBER(&found)) )
               ? bson_iter_as_int64(&found)
               : 0;
}

/**
 * @brief Reads an update command's upserts from its reply: how many, and
 *        the last one's _id.
 *
 * @param[in]  _reply  the reply.
 * @param[out] _id     receives the last _id, if there is one.
 * @return how many documents were upserted.
 */
static
int64_t d_internal_mongodb_upserts(
    const bson_t*                        _reply,
    struct dj_internal_mongodb_found_id* _id
)
{
    bson_iter_t field;
    bson_iter_t entry;
    int64_t     count = 0;

    _id->found = false;

    // each entry of upserted, {index, _id}
    if ( (bson_iter_init_find(&field, _reply, "upserted")) &&
         (BSON_ITER_HOLDS_ARRAY(&field))                   &&
         (bson_iter_recurse(&field, &entry)) )
    {
        while (bson_iter_next(&entry))
        {
            bson_iter_t id;

            ++count;

            if ( (BSON_ITER_HOLDS_DOCUMENT(&entry))     &&
                 (bson_iter_recurse(&entry, &id))       &&
                 (bson_iter_find(&id, "_id")) )
            {
                _id->value = *bson_iter_value(&id);
                _id->found = true;
            }
        }
    }

    return count;
}

/**
 * @brief Reads findAndModify's counts from its lastErrorObject.
 *
 * @param[in]  _command  the command.
 * @param[in]  _reply    its reply.
 * @param[out] _counts   receives the counts.
 * @param[out] _id       receives an upsert's _id, if there is one.
 *
 * A remove deletes its n; an upsert that inserted names the _id; any other
 * wrote the n it matched, whether or not the document changed, which the
 * reply does not say.
 */
static
void d_internal_mongodb_find_and_modify_counts(
    const bson_t*                        _command,
    const bson_t*                        _reply,
    struct dj_mongodb_write_counts*      _counts,
    struct dj_internal_mongodb_found_id* _id
)
{
    bson_iter_t field;
    bson_iter_t top;
    bson_iter_t upserted;

    const int64_t n       = d_internal_mongodb_number_at(_reply,
                                                         "lastErrorObject.n");
    const bool    removes = ( (bson_iter_init_find(&field,
                                                   _command,
                                                   "remove")) &&
                              (bson_iter_as_bool(&field)) );

    _id->found = ( (bson_iter_init(&top, _reply)) &&
                   (bson_iter_find_descendant(&top,
                                              "lastErrorObject.upserted",
                                              &upserted)) );

    // the upsert's _id, copied out of its iterator
    if (_id->found)
    {
        _id->value = *bson_iter_value(&upserted);
    }

    // the one document, by what was done to it
    if (removes)
    {
        _counts->deleted = n;
    }
    else if (_id->found)
    {
        _counts->upserted = n;
    }
    else
    {
        _counts->matched  = n;
        _counts->modified = n;
    }

    return;
}

/**
 * @brief Reads what a write command wrote from its reply, by what the
 *        command is.
 *
 * @param[in]  _command  the command.
 * @param[in]  _reply    its reply.
 * @param[out] _counts   receives the counts, all 0 for a command that
 *                       writes none.
 * @param[out] _id       receives an upsert's _id, if there is one.
 * @return `true` for a write command: insert, update, delete or
 *         findAndModify.
 */
static
bool d_internal_mongodb_command_counts(
    const bson_t*                        _command,
    const bson_t*                        _reply,
    struct dj_mongodb_write_counts*      _counts,
    struct dj_internal_mongodb_found_id* _id
)
{
    bson_iter_t first;

    _counts->inserted = 0;
    _counts->matched  = 0;
    _counts->modified = 0;
    _counts->deleted  = 0;
    _counts->upserted = 0;
    _id->found        = false;

    // the command is its first field
    if ( (!bson_iter_init(&first, _command)) ||
         (!bson_iter_next(&first)) )
    {
        return false;
    }

    const char* const name = bson_iter_key(&first);
    const int64_t     n    = d_internal_mongodb_number_at(_reply, "n");

    // each kind of write, as its reply counts
    if (strcmp(name, "insert") == 0)
    {
        _counts->inserted = n;
    }
    else if (strcmp(name, "delete") == 0)
    {
        _counts->deleted = n;
    }
    else if (strcmp(name, "update") == 0)
    {
        _counts->upserted = d_internal_mongodb_upserts(_reply, _id);
        _counts->matched  = n - _counts->upserted;
        _counts->modified = d_internal_mongodb_number_at(_reply, "nModified");
    }
    else if ( (strcmp(name, "findAndModify") == 0) ||
              (strcmp(name, "findandmodify") == 0) )
    {
        d_internal_mongodb_find_and_modify_counts(_command,
                                                  _reply,
                                                  _counts,
                                                  _id);
    }
    else
    {
        return false;
    }

    return true;
}

/**
 * @brief Reads a command's text and the database it runs on.
 *
 * @param[in,out] _db        the handle, not `NULL`.
 * @param[in]     _database  the database given, or `NULL`.
 * @param[in]     _text      the command's text.
 * @param[out]    _command   initialized, and receives the command.
 * @param[out]    _name      receives the database's name.
 * @return 0; D_DB_INVALID_ARGUMENT for no command, or no database; the
 *         text's failure.
 */
static
int d_internal_mongodb_prepare(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _text,
    bson_t*            _command,
    const char**       _name
)
{
    *_name = d_internal_mongodb_database(_db, _database);

    // a database to run it on
    if (!*_name)
    {
        bson_init(_command);

        return _db->error;
    }

    return d_internal_mongodb_read_document(_db, _text, true, _command);
}

/**
 * @brief Records what a command that succeeded wrote, and hands its reply
 *        and its count back where they are wanted.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _command  the command.
 * @param[in]     _reply    its reply.
 * @param[out]    _changes  receives the documents written, or `NULL`.
 * @param[out]    _text     receives the reply in JSON, or `NULL`.
 * @return 0, or D_MONGODB_LAYER_NO_MEMORY for the reply.
 */
static
int d_internal_mongodb_after(
    struct dj_mongodb* _db,
    const bson_t*      _command,
    const bson_t*      _reply,
    int64_t*           _changes,
    char**             _text
)
{
    struct dj_mongodb_write_counts      counts = { 0, 0, 0, 0, 0 };
    struct dj_internal_mongodb_found_id id;
    const bool                         writes =
        d_internal_mongodb_command_counts(_command, _reply, &counts, &id);

    // a write's count and its new _id
    if (writes)
    {
        d_internal_mongodb_wrote(_db, &counts);
        d_internal_mongodb_new_id(_db, (id.found) ? &id.value : NULL);
    }

    // the reply, written out, when it is wanted
    if (_text)
    {
        char* const text = d_internal_mongodb_json(_reply, _db->format);

        if (!text)
        {
            return d_internal_mongodb_answer(_db,
                                             D_MONGODB_LAYER_NO_MEMORY,
                                             "no memory for the reply");
        }

        *_text = text;
    }

    // the count
    if (_changes)
    {
        *_changes = (writes) ? _db->changes : 0;
    }

    return 0;
}

/**
 * @brief Runs a command's text, recording what a write wrote.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _database  the database, or `NULL` for the current one.
 * @param[out]    _changes   receives the documents written, or `NULL`.
 * @param[out]    _reply     receives the reply in JSON, or `NULL`.
 * @param[in]     _text      the command's text.
 * @return as d_mongodb_exec.
 */
static
int d_internal_mongodb_exec_text(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _changes,
    char**             _reply,
    const char*        _text
)
{
    // a handle to run on
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_t      command;
    bson_t      reply;
    const char* name   = NULL;
    int         result = d_internal_mongodb_prepare(_db,
                                                    _database,
                                                    _text,
                                                    &command,
                                                    &name);

    bson_init(&reply);
    result = (result == 0)
                 ? d_internal_mongodb_run(_db, name, &command, false, &reply)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_after(_db,
                                            &command,
                                            &reply,
                                            _changes,
                                            _reply)
                 : result;

    bson_destroy(&reply);
    bson_destroy(&command);

    return result;
}

/**
 * @brief Formats a command, then runs it as d_internal_mongodb_exec_text.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _database  the database, or `NULL`.
 * @param[out]    _changes   receives the documents written, or `NULL`.
 * @param[out]    _reply     receives the reply in JSON, or `NULL`.
 * @param[in]     _format    the format.
 * @param[in]     _args      its arguments.
 * @return as d_mongodb_execf.
 */
static
int d_internal_mongodb_vexec(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _changes,
    char**             _reply,
    const char*        _format,
    va_list            _args
)
{
    char* text = NULL;

    // a handle, and a format
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (!_format)
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "no command was given");
    }

    const int formatted = d_internal_mongodb_vformat(_db,
                                                     &text,
                                                     _format,
                                                     _args);
    const int result    = (formatted == 0)
                              ? d_internal_mongodb_exec_text(_db,
                                                             _database,
                                                             _changes,
                                                             _reply,
                                                             text)
                              : formatted;

    free(text);

    return result;
}

/**
 * @brief Runs a command and reads it into rows: its cursor's documents to
 *        the end, or its reply as one row.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _name     the database.
 * @param[in]     _command  the command.
 * @param[out]    _rows     receives the rows.
 * @return as d_mongodb_query_rows.
 */
static
int d_internal_mongodb_rows_of(
    struct dj_mongodb* _db,
    const char*        _name,
    const bson_t*      _command,
    struct dj_db_rows* _rows
)
{
    mongoc_cursor_t*          cursor  = NULL;
    bson_t                    reply;
    struct dj_internal_db_rows builder = { { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          0u,
                                          0u };
    int                       result  = d_internal_mongodb_run_cursor(_db,
                                                                      _name,
                                                                      _command,
                                                                      &cursor,
                                                                      &reply);

    // the cursor, read to its end
    if (cursor)
    {
        result = d_internal_mongodb_cursor_rows(_db,
                                                cursor,
                                                (size_t)-1,
                                                _rows,
                                                NULL,
                                                true);
        mongoc_cursor_destroy(cursor);
    }
    else if ( (result == 0) &&
              ( (!d_internal_mongodb_rows_column(&builder, false))       ||
                (!d_internal_mongodb_rows_document(&builder,
                                                   &reply,
                                                   _db->format))         ||
                (!d_internal_db_rows_finish(&builder, _rows)) ) )
    {
        d_internal_db_rows_release(&builder);
        result = d_internal_mongodb_answer(_db,
                                           D_MONGODB_LAYER_NO_MEMORY,
                                           "no memory for the reply");
    }

    bson_destroy(&reply);

    return result;
}

/**
 * @brief Reads a command's text into rows, as d_mongodb_query_rows.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _database  the database, or `NULL`.
 * @param[out]    _rows      receives the rows.
 * @param[in]     _text      the command's text.
 * @return as d_mongodb_query_rows.
 */
static
int d_internal_mongodb_query_text(
    struct dj_mongodb* _db,
    const char*        _database,
    struct dj_db_rows* _rows,
    const char*        _text
)
{
    // a handle, and somewhere for the rows
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (!_rows)
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "no place was given for the rows");
    }

    bson_t      command;
    const char* name   = NULL;
    int         result = d_internal_mongodb_prepare(_db,
                                                    _database,
                                                    _text,
                                                    &command,
                                                    &name);

    result = (result == 0)
                 ? d_internal_mongodb_rows_of(_db, name, &command, _rows)
                 : result;
    bson_destroy(&command);

    return result;
}


//==============================================================================
// 1.  COMMANDS
//==============================================================================

// 1.1    Running commands
//------------------------------------------------------------------------------
/*
d_mongodb_exec
  The command as written, its reply discarded.
*/
int d_mongodb_exec(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _command
)
{
    return d_internal_mongodb_exec_text(_db, _database, NULL, NULL, _command);
}

/*
d_mongodb_execf
  The caller's format.
*/
int d_mongodb_execf(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result = d_internal_mongodb_vexec(_db,
                                                _database,
                                                NULL,
                                                NULL,
                                                _format,
                                                args);

    va_end(args);

    return result;
}

// 1.2    Documents written
//------------------------------------------------------------------------------
/*
d_mongodb_exec_changes
  The command as written, its count read by what it is.
*/
int d_mongodb_exec_changes(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _changes,
    const char*        _command
)
{
    // somewhere for the count
    if ( (_db) &&
         (!_changes) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "no place was given for the count");
    }

    return d_internal_mongodb_exec_text(_db,
                                        _database,
                                        _changes,
                                        NULL,
                                        _command);
}

/*
d_mongodb_execf_changes
  The caller's format.
*/
int d_mongodb_execf_changes(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _changes,
    const char*        _format,
    ...
)
{
    // somewhere for the count
    if ( (_db) &&
         (!_changes) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "no place was given for the count");
    }

    va_list args;

    va_start(args, _format);

    const int result = d_internal_mongodb_vexec(_db,
                                                _database,
                                                _changes,
                                                NULL,
                                                _format,
                                                args);

    va_end(args);

    return result;
}

// 1.3    Replies
//------------------------------------------------------------------------------
/*
d_mongodb_command
  The command as written, its reply written out.
*/
int d_mongodb_command(
    struct dj_mongodb* _db,
    const char*        _database,
    char**             _reply,
    const char*        _command
)
{
    // somewhere for the reply
    if ( (_db) &&
         (!_reply) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "no place was given for the reply");
    }

    return d_internal_mongodb_exec_text(_db,
                                        _database,
                                        NULL,
                                        _reply,
                                        _command);
}

/*
d_mongodb_commandf
  The caller's format.
*/
int d_mongodb_commandf(
    struct dj_mongodb* _db,
    const char*        _database,
    char**             _reply,
    const char*        _format,
    ...
)
{
    // somewhere for the reply
    if ( (_db) &&
         (!_reply) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "no place was given for the reply");
    }

    va_list args;

    va_start(args, _format);

    const int result = d_internal_mongodb_vexec(_db,
                                                _database,
                                                NULL,
                                                _reply,
                                                _format,
                                                args);

    va_end(args);

    return result;
}

// 1.4    Rows
//------------------------------------------------------------------------------
/*
d_mongodb_query_rows
  The command as written, read into rows.
*/
int d_mongodb_query_rows(
    struct dj_mongodb* _db,
    const char*        _database,
    struct dj_db_rows* _rows,
    const char*        _command
)
{
    return d_internal_mongodb_query_text(_db, _database, _rows, _command);
}

/*
d_mongodb_query_rowsf
  The caller's format, then as d_mongodb_query_rows.
*/
int d_mongodb_query_rowsf(
    struct dj_mongodb* _db,
    const char*        _database,
    struct dj_db_rows* _rows,
    const char*        _format,
    ...
)
{
    char* text = NULL;

    // a handle, and a format
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (!_format)
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "no command was given");
    }

    va_list args;

    va_start(args, _format);

    const int formatted = d_internal_mongodb_vformat(_db,
                                                     &text,
                                                     _format,
                                                     args);

    va_end(args);

    const int result = (formatted == 0)
                           ? d_internal_mongodb_query_text(_db,
                                                           _database,
                                                           _rows,
                                                           text)
                           : formatted;

    free(text);

    return result;
}

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports D_DB_UNSUPPORTED,
// its answers untouched.

/*
d_mongodb_exec
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_exec(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _command
)
{
    (void)_db;
    (void)_database;
    (void)_command;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_execf
  Without the driver, D_DB_UNSUPPORTED; the arguments are not read.
*/
int d_mongodb_execf(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _format,
    ...
)
{
    (void)_db;
    (void)_database;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_exec_changes
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_exec_changes(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _changes,
    const char*        _command
)
{
    (void)_db;
    (void)_database;
    (void)_changes;
    (void)_command;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_execf_changes
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_execf_changes(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _changes,
    const char*        _format,
    ...
)
{
    (void)_db;
    (void)_database;
    (void)_changes;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_command
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_command(
    struct dj_mongodb* _db,
    const char*        _database,
    char**             _reply,
    const char*        _command
)
{
    (void)_db;
    (void)_database;
    (void)_reply;
    (void)_command;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_commandf
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_commandf(
    struct dj_mongodb* _db,
    const char*        _database,
    char**             _reply,
    const char*        _format,
    ...
)
{
    (void)_db;
    (void)_database;
    (void)_reply;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_query_rows
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_query_rows(
    struct dj_mongodb* _db,
    const char*        _database,
    struct dj_db_rows* _rows,
    const char*        _command
)
{
    (void)_db;
    (void)_database;
    (void)_rows;
    (void)_command;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_query_rowsf
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_query_rowsf(
    struct dj_mongodb* _db,
    const char*        _database,
    struct dj_db_rows* _rows,
    const char*        _format,
    ...
)
{
    (void)_db;
    (void)_database;
    (void)_rows;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MONGODB == 1
