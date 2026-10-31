/*******************************************************************************
* djinterp [c]                                                mongodb_internal.c
*
* Implementation of the plumbing mongodb_internal.h declares for the MongoDB
* layer's modules: failures packed and recorded, JSON read and written,
* commands run, rows built, the catalog asked, writes counted, and the
* driver's log.
*   A failure is recorded whole, as the handle's last: its result, its
* message cut at a character's start, the server's name for its code, its
* reply. Any failure the server reports inside a transaction ends the
* transaction on the server -- a duplicate key, a bad operator, a command of
* no such name alike -- so the layer marks it failed then, and not for a
* failure the server never saw.
*   JSON is read in three steps: the layer finds where the document or the
* array given ends, from its brackets, outside its strings, and refuses
* anything after it but space; it checks that text against JSON's grammar
* (RFC 8259); libbson then reads exactly that text. libbson alone would pass
* over a comma and what follows, read an array where a document is due, and
* read "" as no document without a word; and every release of it here takes
* text that is not JSON -- a bare n or N as 0.0, -01 as 0, NaN and Infinity
* in any case -- and 1.21's drops a field whose value is a word beginning
* with n. The grammar's refusals are libbson's code for corrupt JSON, as its
* own are, so that what is refused, and how, is the same on every release.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_internal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.29
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "./mongodb_internal.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // vsnprintf
#include <stdlib.h>  // malloc, free
#include <string.h>  // memchr, memcmp, memcpy, memset, strchr, strcmp,
                     // strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, the lists
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_document.h"  // counts
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "../db_internal.h"  // buffers, builders, the formatter
// over the driver only: the lock the driver's log handler is guarded with,
// which the stubs have no use for, and which dmutex.h cannot give from C11
// where the C library has no <threads.h>, as mingw-w64's (db_audit.md,
// 3.7.6)
#if (D_INTERNAL_MONGODB == 1)
    #include "../../../../../inc/djinterp/c/sync/dmutex.h"  // d_call_once, dj_mutex_t
#endif  // D_INTERNAL_MONGODB
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t, uint8_t,
                                                         // uint32_t


#if (D_INTERNAL_MONGODB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_MONGODB_JSON_LEVELS
//   constant: the most levels libbson's reader takes, every release here
// alike: each document and array open is one, the outermost among them,
// and a key or a scalar one more than the container it is in -- so 98
// documents deep, a field at the bottom; deeper, it refuses.
#define D_INTERNAL_MONGODB_JSON_LEVELS 99u

// dj_internal_mongodb_scan
//   struct: a check of JSON text against JSON's grammar, where it has got
// to.
struct dj_internal_mongodb_scan
{
    const char* at;      // the next character
    const char* end;     // past the text's last
    const char* why;     // the refusal; NULL for none
    size_t      depth;   // the documents and arrays open
    size_t      levels;  // the most levels there may be
    bool        due;     // a value is due next
    char        open[D_INTERNAL_MONGODB_JSON_LEVELS];
                         // each open one's opening bracket
};

// dj_internal_mongodb_prefs
//   struct: a read preference as it is read, before the driver's is made.
struct dj_internal_mongodb_prefs
{
    mongoc_read_mode_t mode;       // the mode, where has_mode
    bool               has_mode;   // whether one was given
    bson_t             tags;       // the tags, in place, where has_tags
    bool               has_tags;   // whether they were given
    int64_t            staleness;  // maxStalenessSeconds; -1: none
};

// d_internal_mongodb_code_name
//   struct: a server code the layer's contracts name, and the server's name
// for it, for a code the layer answers itself.
struct dj_internal_mongodb_code_name
{
    int         code;  // the server's code
    const char* name;  // its name, as the server's codeName writes it
};

// CODE_NAMES
//   table: the codes mongodb_common.h names, with the server's names for
// them.
static const struct dj_internal_mongodb_code_name CODE_NAMES[] =
{
    { D_MONGODB_ERROR_BAD_VALUE,                     "BadValue" },
    { D_MONGODB_ERROR_FAILED_TO_PARSE,               "FailedToParse" },
    { D_MONGODB_ERROR_UNAUTHORIZED,                  "Unauthorized" },
    { D_MONGODB_ERROR_TYPE_MISMATCH,                 "TypeMismatch" },
    { D_MONGODB_ERROR_ILLEGAL_OPERATION,             "IllegalOperation" },
    { D_MONGODB_ERROR_NAMESPACE_NOT_FOUND,           "NamespaceNotFound" },
    { D_MONGODB_ERROR_INDEX_NOT_FOUND,               "IndexNotFound" },
    { D_MONGODB_ERROR_NAMESPACE_EXISTS,              "NamespaceExists" },
    { D_MONGODB_ERROR_MAX_TIME_MS_EXPIRED,           "MaxTimeMSExpired" },
    { D_MONGODB_ERROR_COMMAND_NOT_FOUND,             "CommandNotFound" },
    { D_MONGODB_ERROR_INVALID_NAMESPACE,             "InvalidNamespace" },
    { D_MONGODB_ERROR_INDEX_OPTIONS_CONFLICT,        "IndexOptionsConflict" },
    { D_MONGODB_ERROR_INDEX_KEY_SPECS_CONFLICT,      "IndexKeySpecsConflict" },
    { D_MONGODB_ERROR_WRITE_CONFLICT,                "WriteConflict" },
    { D_MONGODB_ERROR_COMMAND_NOT_SUPPORTED_ON_VIEW,
      "CommandNotSupportedOnView" },
    { D_MONGODB_ERROR_NO_SUCH_TRANSACTION,           "NoSuchTransaction" },
    { D_MONGODB_ERROR_DUPLICATE_KEY,                 "DuplicateKey" },
    { D_MONGODB_ERROR_TRANSACTION_CURSOR,            "Location50740" }
};

// VALUE_PREFIX, VALUE_SUFFIX
//   constants: what libbson's writers put around a value they write as the
// one field "v" of a document.
static const char VALUE_PREFIX[] = "{ \"v\" : ";
static const char VALUE_SUFFIX[] = " }";

// d_internal_mongodb_log_once
//   once flag: the driver and the log's lock, initialized once.
static dj_once_flag_t d_internal_mongodb_log_once = D_ONCE_FLAG_INIT;

// d_internal_mongodb_log_lock
//   mutex: guards the three that follow.
static dj_mutex_t d_internal_mongodb_log_lock;

// d_internal_mongodb_log_ready
//   flag: the lock was made; without it, nothing below is touched.
static bool d_internal_mongodb_log_ready = false;

// d_internal_mongodb_log_installed
//   flag: the layer's handler was installed, by a handle or by a setter.
static bool d_internal_mongodb_log_installed = false;

// d_internal_mongodb_log_level
//   value: the level installed, or the knob's while none is.
static int d_internal_mongodb_log_level = D_INTERNAL_MONGODB_LOG_LEVEL;

// d_internal_mongodb_log_levels
//   table: each level a handler may pass, -1 first, which the handler is
// given a pointer into as its user data and reads, so that it never takes
// the lock the setter holds while the driver holds its own. Never written;
// not const, since the driver's user data is a void*.
static int d_internal_mongodb_log_levels[] = { -1, 0, 1, 2, 3, 4, 5, 6 };

D_STATIC_ASSERT( (sizeof(d_internal_mongodb_log_levels) /
                  sizeof(d_internal_mongodb_log_levels[0])) == 8u,
                 "a level from -1 to 6 indexes the table at level + 1" );


//==============================================================================
// 1.  RESULTS
//==============================================================================

// 1.1    Packing
//------------------------------------------------------------------------------
/*
d_internal_mongodb_pack
  The second error API reports the server's failures in MONGOC_ERROR_SERVER
and a write concern's in MONGOC_ERROR_WRITE_CONCERN, the code the server's;
those come back as they are. Every other domain is under 256 -- libmongoc's
end in the twenties, libbson's at four -- so its low byte loses nothing.
*/
int d_internal_mongodb_pack(
    const bson_error_t* _error,
    bool                _from_bson
)
{
    // a failure that describes nothing of itself
    if ( (!_error)            ||
         ( (_error->domain == 0u) &&
           (_error->code == 0u) ) )
    {
        return D_MONGODB_LAYER_UNDESCRIBED;
    }

    // the server's codes, and a write concern's, unchanged
    if ( (!_from_bson)                                            &&
         ( (_error->domain == (uint32_t)MONGOC_ERROR_SERVER) ||
           (_error->domain == (uint32_t)MONGOC_ERROR_WRITE_CONCERN) ) &&
         (_error->code > 0u)                                      &&
         (_error->code < (uint32_t)D_MONGODB_CLIENT_ERRORS) )
    {
        return (int)_error->code;
    }

    const uint32_t domain = ( (_from_bson) ? (uint32_t)D_MONGODB_DOMAIN_BSON
                                           : 0u ) +
                            (_error->domain & 0xFFu);

    return (int)( (uint32_t)D_MONGODB_CLIENT_ERRORS +
                  (domain * 65536u) +
                  (_error->code & 0xFFFFu) );
}

// 1.2    Recording
//------------------------------------------------------------------------------
/**
 * @brief Copies text into a fixed buffer, cut, where it must be, at the
 *        start of a UTF-8 character rather than inside one.
 *
 * @param[out] _out     the buffer, terminated after.
 * @param[in]  _size    its size, at least 1.
 * @param[in]  _text    the text; `NULL` copies "".
 * @param[in]  _length  how many bytes of it there are.
 */
static
void d_internal_mongodb_copy_text(
    char*       _out,
    size_t      _size,
    const char* _text,
    size_t      _length
)
{
    size_t length = (_text) ? _length : 0u;

    // a cut, where one is needed, goes back to a character's first byte
    if (length >= _size)
    {
        length = _size - 1u;

        while ( (length > 0u) &&
                ( (((unsigned char)_text[length]) & 0xC0u) == 0x80u ) )
        {
            --length;
        }
    }

    // the text, when there is any
    if (length > 0u)
    {
        memcpy(_out, _text, length);
    }

    _out[length] = '\0';

    return;
}

/**
 * @brief Finds the server's name for a failure's code in its reply: the
 *        command's codeName, or its first write error's, or its write
 *        concern error's.
 *
 * @param[in] _reply  the reply; may be `NULL`.
 * @return the name, valid while the reply is, or `NULL` for none.
 */
static
const char* d_internal_mongodb_reply_name(
    const bson_t* _reply
)
{
    static const char* const PATHS[] =
    {
        "codeName",
        "writeErrors.0.codeName",
        "writeConcernError.codeName"
    };

    // no reply names nothing
    if (!_reply)
    {
        return NULL;
    }

    size_t index = 0u;

    // the first of the places a name is kept that holds one
    for (index = 0u; index < (sizeof(PATHS) / sizeof(PATHS[0])); ++index)
    {
        bson_iter_t top;
        bson_iter_t found;

        if ( (bson_iter_init(&top, _reply))                         &&
             (bson_iter_find_descendant(&top, PATHS[index], &found)) &&
             (BSON_ITER_HOLDS_UTF8(&found)) )
        {
            return bson_iter_utf8(&found, NULL);
        }
    }

    return NULL;
}

/**
 * @brief The server's name for a code the layer's contracts name.
 *
 * @param[in] _code  the code.
 * @return the name, or "" for a code of no name the layer knows.
 */
static
const char* d_internal_mongodb_code_name(
    int _code
)
{
    size_t index = 0u;

    // the code's entry, where it has one
    for (index = 0u;
         index < (sizeof(CODE_NAMES) / sizeof(CODE_NAMES[0]));
         ++index)
    {
        if (CODE_NAMES[index].code == _code)
        {
            return CODE_NAMES[index].name;
        }
    }

    return "";
}

/**
 * @brief Records a failure on a handle as its last.
 *
 * @param[in,out] _db       the handle; `NULL` records nothing.
 * @param[in]     _result   the result returned for it.
 * @param[in]     _message  its message; `NULL` for none.
 * @param[in]     _length   the message's length in bytes.
 * @param[in]     _reply    the server's reply, copied; `NULL`, or an empty
 *                          one, for none.
 * @param[in]     _aborts   whether the server's open transaction did not
 *                          survive it.
 * @return `_result`.
 *
 * The name comes from the reply where the server gave one, and otherwise,
 * for a code the layer's contracts name, from the layer's table: a code the
 * layer answers itself is named as the server would name it.
 */
static
int d_internal_mongodb_record(
    struct dj_mongodb* _db,
    int                _result,
    const char*        _message,
    size_t             _length,
    const bson_t*      _reply,
    bool               _aborts
)
{
    // nowhere to record it
    if (!_db)
    {
        return _result;
    }

    const char* const name = d_internal_mongodb_reply_name(_reply);

    _db->error = _result;
    d_internal_mongodb_copy_text(_db->message,
                                 sizeof(_db->message),
                                 _message,
                                 _length);

    // the server's name, or the layer's for a code it knows
    if (name)
    {
        d_internal_mongodb_copy_text(_db->error_name,
                                     sizeof(_db->error_name),
                                     name,
                                     strlen(name));
    }
    else
    {
        const char* const known = ( (_result > 0) &&
                                    (_result < D_MONGODB_CLIENT_ERRORS) )
                                      ? d_internal_mongodb_code_name(_result)
                                      : "";

        d_internal_mongodb_copy_text(_db->error_name,
                                     sizeof(_db->error_name),
                                     known,
                                     strlen(known));
    }

    // the reply replaces the last failure's
    if (_db->reply)
    {
        bson_destroy(_db->reply);
    }

    _db->reply = ( (_reply) &&
                   (!bson_empty(_reply)) ) ? bson_copy(_reply)
                                           : NULL;

    // a failure the server's transaction did not survive marks it failed
    if ( (_aborts)      &&
         (_db->session) &&
         (mongoc_client_session_in_transaction(_db->session)) )
    {
        _db->failed = true;
    }

    return _result;
}

/**
 * @brief The length of a driver's message, which fills a fixed array and
 *        may, in principle, fill it without a terminator.
 *
 * @param[in] _error  the failure.
 * @return the message's length in bytes.
 */
static
size_t d_internal_mongodb_error_length(
    const bson_error_t* _error
)
{
    const char* const end = (const char*)memchr(_error->message,
                                                '\0',
                                                sizeof(_error->message));

    return (end) ? (size_t)(end - _error->message)
                 : sizeof(_error->message);
}

/**
 * @brief Records a driver's failure, whether or not the operation carried
 *        the open transaction's session.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _error    the failure.
 * @param[in]     _reply    its reply; may be `NULL`.
 * @param[in]     _session  whether the operation carried the session.
 * @return the failure's result.
 *
 * A failure the server reported, or one the driver labelled
 * TransientTransactionError -- a broken connection inside a transaction --
 * is one the server's transaction did not survive.
 */
static
int d_internal_mongodb_fail_with(
    struct dj_mongodb*  _db,
    const bson_error_t* _error,
    const bson_t*       _reply,
    bool                _session
)
{
    static const char UNDESCRIBED[] =
        "the driver reported a failure, and described none";

    const int result = d_internal_mongodb_pack(_error, false);

    const bool aborts = ( (_session) &&
                          ( (_error->domain ==
                             (uint32_t)MONGOC_ERROR_SERVER)           ||
                            (_error->domain ==
                             (uint32_t)MONGOC_ERROR_WRITE_CONCERN)    ||
                            ( (_reply) &&
                              (mongoc_error_has_label(
                                  _reply,
                                  "TransientTransactionError")) ) ) );

    const size_t length = d_internal_mongodb_error_length(_error);

    // a failure without words gets the layer's
    if (length == 0u)
    {
        return d_internal_mongodb_record(_db,
                                         result,
                                         UNDESCRIBED,
                                         sizeof(UNDESCRIBED) - 1u,
                                         _reply,
                                         aborts);
    }

    return d_internal_mongodb_record(_db,
                                     result,
                                     _error->message,
                                     length,
                                     _reply,
                                     aborts);
}

/*
d_internal_mongodb_fail
  An operation that carried the session, if one is open.
*/
int d_internal_mongodb_fail(
    struct dj_mongodb*  _db,
    const bson_error_t* _error,
    const bson_t*       _reply
)
{
    return d_internal_mongodb_fail_with(_db, _error, _reply, true);
}

/*
d_internal_mongodb_fail_outside
  An operation that ran outside the transaction: the catalog, GridFS. Its
failure leaves the transaction as it was.
*/
int d_internal_mongodb_fail_outside(
    struct dj_mongodb*  _db,
    const bson_error_t* _error,
    const bson_t*       _reply
)
{
    return d_internal_mongodb_fail_with(_db, _error, _reply, false);
}

/*
d_internal_mongodb_fail_bson
  libbson's failures never reach the server; its domains are packed apart
from libmongoc's.
*/
int d_internal_mongodb_fail_bson(
    struct dj_mongodb*  _db,
    const bson_error_t* _error
)
{
    return d_internal_mongodb_record(_db,
                                     d_internal_mongodb_pack(_error, true),
                                     _error->message,
                                     d_internal_mongodb_error_length(_error),
                                     NULL,
                                     false);
}

/*
d_internal_mongodb_answer
  The message is formatted into the handle's own buffer, the cut, where one
is needed, moved back to a character's start.
*/
int d_internal_mongodb_answer(
    struct dj_mongodb* _db,
    int                _result,
    const char*        _format,
    ...
)
{
    // nowhere to record it
    if (!_db)
    {
        return _result;
    }

    char    text[D_INTERNAL_MONGODB_MESSAGE_SIZE];
    va_list args;

    va_start(args, _format);
    const int written = vsnprintf(text, sizeof(text), _format, args);
    va_end(args);

    const size_t length = (written < 0)
                              ? 0u
                              : ( ((size_t)written < sizeof(text))
                                      ? (size_t)written
                                      : sizeof(text) );

    return d_internal_mongodb_record(_db,
                                     _result,
                                     text,
                                     length,
                                     NULL,
                                     false);
}

/**
 * @brief Records a reply's first write error or write concern error, where
 *        a command or a write reported success.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _reply    the reply.
 * @param[in]     _session  whether the operation carried the session.
 * @return 0 where the reply reports none; else the failure's code.
 */
static
int d_internal_mongodb_reply_failure_with(
    struct dj_mongodb* _db,
    const bson_t*      _reply,
    bool               _session
)
{
    bson_iter_t top;
    bson_iter_t code;
    bson_iter_t message;

    // the first write error, or else the write concern's
    const bool written =
        ( (bson_iter_init(&top, _reply)) &&
          (bson_iter_find_descendant(&top, "writeErrors.0.code", &code)) );
    const bool concern =
        ( (!written)                    &&
          (bson_iter_init(&top, _reply)) &&
          (bson_iter_find_descendant(&top, "writeConcernError.code", &code)) );

    // nothing failed
    if ( (!written) &&
         (!concern) )
    {
        return 0;
    }

    const int64_t value = (BSON_ITER_HOLDS_NUMBER(&code))
                              ? bson_iter_as_int64(&code)
                              : 0;
    const int result = ( (value > 0) &&
                         (value < D_MONGODB_CLIENT_ERRORS) )
                           ? (int)value
                           : D_MONGODB_LAYER_UNDESCRIBED;

    const bool has_message =
        ( (bson_iter_init(&top, _reply)) &&
          (bson_iter_find_descendant(&top,
                                     (written) ? "writeErrors.0.errmsg"
                                               : "writeConcernError.errmsg",
                                     &message)) &&
          (BSON_ITER_HOLDS_UTF8(&message)) );

    uint32_t          length = 0u;
    const char* const text   = (has_message)
                                   ? bson_iter_utf8(&message, &length)
                                   : NULL;

    return d_internal_mongodb_record(_db,
                                     result,
                                     text,
                                     (size_t)length,
                                     _reply,
                                     _session);
}

/*
d_internal_mongodb_reply_failure
  A write error, or a write concern's, is the server's: one inside a
transaction ends it.
*/
int d_internal_mongodb_reply_failure(
    struct dj_mongodb* _db,
    const bson_t*      _reply
)
{
    return d_internal_mongodb_reply_failure_with(_db, _reply, true);
}


//==============================================================================
// 2.  JSON
//==============================================================================

// 2.1    Reading
//------------------------------------------------------------------------------
/**
 * @brief Passes over JSON's white space.
 *
 * @param[in] _text  the text.
 * @return the first character that is not space, a tab, a carriage return
 *         or a line feed.
 */
static
const char* d_internal_mongodb_skip_space(
    const char* _text
)
{
    const char* cursor = _text;

    // JSON's four, and no others
    while ( (*cursor == ' ')  ||
            (*cursor == '\t') ||
            (*cursor == '\n') ||
            (*cursor == '\r') )
    {
        ++cursor;
    }

    return cursor;
}

/**
 * @brief Finds the end of the object or the array a text begins with.
 *
 * @param[in] _text  the text, at its opening bracket.
 * @return the character after its closing bracket, or `NULL` where the text
 *         ends first.
 *
 * Brackets are counted alike, outside strings, whose escapes are passed
 * over; a bracket of the wrong kind is libbson's to refuse, which it does
 * on reading what this finds.
 */
static
const char* d_internal_mongodb_container_end(
    const char* _text
)
{
    size_t      depth     = 0u;
    bool        in_string = false;
    const char* cursor    = _text;

    // to the bracket that closes the first
    for (cursor = _text; *cursor; ++cursor)
    {
        const char c = *cursor;

        // inside a string, only its end and its escapes matter
        if (in_string)
        {
            if ( (c == '\\') &&
                 (cursor[1] != '\0') )
            {
                ++cursor;
            }
            else if (c == '"')
            {
                in_string = false;
            }

            continue;
        }

        // a string, an opening, a closing
        if (c == '"')
        {
            in_string = true;
        }
        else if ( (c == '{') ||
                  (c == '[') )
        {
            ++depth;
        }
        else if ( ( (c == '}') ||
                    (c == ']') ) &&
                  (--depth == 0u) )
        {
            return cursor + 1;
        }
    }

    return NULL;
}

/**
 * @brief Finds the end of the scalar a text begins with: a string, after
 *        its closing quote; anything else, at the first space or
 *        punctuation JSON gives meaning to.
 *
 * @param[in] _text  the text, at the scalar's first character.
 * @return the character after the scalar.
 */
static
const char* d_internal_mongodb_scalar_end(
    const char* _text
)
{
    const char* cursor = _text + 1;

    // a string runs to its closing quote
    if (*_text == '"')
    {
        while ( (*cursor) &&
                (*cursor != '"') )
        {
            cursor += ( (*cursor == '\\') &&
                        (cursor[1] != '\0') ) ? 2 : 1;
        }

        return (*cursor) ? cursor + 1 : cursor;
    }

    // a number, true, false, null: to space or punctuation
    while ( (*cursor)                    &&
            (!strchr(" \t\r\n{}[],:\"", *cursor)) )
    {
        ++cursor;
    }

    return cursor;
}

/**
 * @brief The character a check of JSON has got to.
 *
 * @param[in] _scan  the check.
 * @return the character; '\0' past the text's end.
 */
static
char d_internal_mongodb_peek(
    const struct dj_internal_mongodb_scan* _scan
)
{
    return (_scan->at < _scan->end) ? *_scan->at : '\0';
}

/**
 * @brief Ends a check of JSON with a refusal.
 *
 * @param[in,out] _scan  the check, which keeps where it stopped.
 * @param[in]     _why   the refusal, a constant.
 * @return `false`.
 */
static
bool d_internal_mongodb_refuse(
    struct dj_internal_mongodb_scan* _scan,
    const char*                      _why
)
{
    _scan->why = _why;

    return false;
}

/**
 * @brief Passes over JSON's white space in a check.
 *
 * @param[in,out] _scan  the check.
 */
static
void d_internal_mongodb_scan_space(
    struct dj_internal_mongodb_scan* _scan
)
{
    // JSON's four, and no others
    while ( (_scan->at < _scan->end) &&
            ( (*_scan->at == ' ')  ||
              (*_scan->at == '\t') ||
              (*_scan->at == '\n') ||
              (*_scan->at == '\r') ) )
    {
        ++_scan->at;
    }

    return;
}

/**
 * @brief Checks an escape inside a string.
 *
 * @param[in] _scan  the check, at the escape's backslash.
 * @return the escape's length, 2, or 6 for \u and its digits; 0 for no
 *         escape JSON has.
 */
static
size_t d_internal_mongodb_escape_length(
    const struct dj_internal_mongodb_scan* _scan
)
{
    static const char HEX[] = "0123456789abcdefABCDEF";
    const char        kind  = (_scan->at + 1 < _scan->end) ? _scan->at[1]
                                                           : '\0';
    size_t            digits = 0u;

    // one of JSON's eight
    if ( (kind != '\0') &&
         (kind != 'u')  &&
         (strchr("\"\\/bfnrt", kind)) )
    {
        return 2u;
    }

    // \u, and four hex digits
    while ( (kind == 'u')                             &&
            (digits < 4u)                             &&
            (_scan->at + 2 + digits < _scan->end)     &&
            (_scan->at[2 + digits] != '\0')           &&
            (strchr(HEX, _scan->at[2 + digits])) )
    {
        ++digits;
    }

    return ( (kind == 'u') &&
             (digits == 4u) ) ? 6u : 0u;
}

/**
 * @brief Checks a string, from its opening quote to past its closing one.
 *
 * @param[in,out] _scan  the check, at the opening quote; at a refusal, at
 *                       the character refused.
 * @return whether it is one; libbson checks its UTF-8 and its code points.
 */
static
bool d_internal_mongodb_scan_string(
    struct dj_internal_mongodb_scan* _scan
)
{
    ++_scan->at;

    // to the closing quote
    while (_scan->at < _scan->end)
    {
        const unsigned char c      = (unsigned char)*_scan->at;
        const size_t        escape = (c == (unsigned char)'\\')
                                         ? d_internal_mongodb_escape_length(
                                               _scan)
                                         : 1u;

        // the end, a character JSON escapes, an escape it has not
        if (c == (unsigned char)'"')
        {
            ++_scan->at;

            return true;
        }

        if (c < 0x20u)
        {
            return d_internal_mongodb_refuse(_scan,
                                             "a string holds a control "
                                             "character, which JSON "
                                             "escapes");
        }

        if (escape == 0u)
        {
            return d_internal_mongodb_refuse(_scan,
                                             "an escape is \\\", \\\\, "
                                             "\\/, \\b, \\f, \\n, \\r, "
                                             "\\t, or \\u and four hex "
                                             "digits");
        }

        _scan->at += escape;
    }

    return d_internal_mongodb_refuse(_scan, "the text ends inside a string");
}

/**
 * @brief Passes over the decimal digits a check has got to.
 *
 * @param[in,out] _scan  the check.
 * @return how many there were.
 */
static
size_t d_internal_mongodb_scan_digits(
    struct dj_internal_mongodb_scan* _scan
)
{
    size_t count = 0u;

    // each digit
    while ( (d_internal_mongodb_peek(_scan) >= '0') &&
            (d_internal_mongodb_peek(_scan) <= '9') )
    {
        ++_scan->at;
        ++count;
    }

    return count;
}

/**
 * @brief Checks a number: a sign, an integer with no leading zero, a
 *        fraction, an exponent.
 *
 * @param[in,out] _scan  the check, at its first character.
 * @return whether it is one; libbson decides its type and its range.
 */
static
bool d_internal_mongodb_scan_number(
    struct dj_internal_mongodb_scan* _scan
)
{
    // a sign, and an integer: 0, or a digit that is not 0 and more
    if (d_internal_mongodb_peek(_scan) == '-')
    {
        ++_scan->at;
    }

    if (d_internal_mongodb_peek(_scan) == '0')
    {
        ++_scan->at;
    }
    else if (d_internal_mongodb_scan_digits(_scan) == 0u)
    {
        return d_internal_mongodb_refuse(_scan,
                                         "a number begins with a digit, "
                                         "after its sign; NaN and Infinity "
                                         "are written {\"$numberDouble\": "
                                         "\"NaN\"}");
    }

    if ( (d_internal_mongodb_peek(_scan) >= '0') &&
         (d_internal_mongodb_peek(_scan) <= '9') )
    {
        return d_internal_mongodb_refuse(_scan,
                                         "a number's integer begins with 0 "
                                         "only where it is 0");
    }

    // a fraction, and an exponent, each with a digit at least
    if (d_internal_mongodb_peek(_scan) == '.')
    {
        ++_scan->at;

        if (d_internal_mongodb_scan_digits(_scan) == 0u)
        {
            return d_internal_mongodb_refuse(_scan,
                                             "a fraction has a digit after "
                                             "its point");
        }
    }

    if ( (d_internal_mongodb_peek(_scan) == 'e') ||
         (d_internal_mongodb_peek(_scan) == 'E') )
    {
        ++_scan->at;

        if ( (d_internal_mongodb_peek(_scan) == '+') ||
             (d_internal_mongodb_peek(_scan) == '-') )
        {
            ++_scan->at;
        }

        if (d_internal_mongodb_scan_digits(_scan) == 0u)
        {
            return d_internal_mongodb_refuse(_scan,
                                             "an exponent has a digit");
        }
    }

    return true;
}

/**
 * @brief Checks a literal: a word that is true, false or null, exactly.
 *
 * @param[in,out] _scan  the check, at the word's first letter.
 * @return whether it is one.
 */
static
bool d_internal_mongodb_scan_literal(
    struct dj_internal_mongodb_scan* _scan
)
{
    const char* const start = _scan->at;

    // the word's letters
    while ( ( (d_internal_mongodb_peek(_scan) >= 'a') &&
              (d_internal_mongodb_peek(_scan) <= 'z') ) ||
            ( (d_internal_mongodb_peek(_scan) >= 'A') &&
              (d_internal_mongodb_peek(_scan) <= 'Z') ) )
    {
        ++_scan->at;
    }

    const size_t length = (size_t)(_scan->at - start);

    // one of the three
    if ( ( (length == 4u) &&
           (memcmp(start, "true", 4u) == 0) ) ||
         ( (length == 5u) &&
           (memcmp(start, "false", 5u) == 0) ) ||
         ( (length == 4u) &&
           (memcmp(start, "null", 4u) == 0) ) )
    {
        return true;
    }

    _scan->at = start;

    return d_internal_mongodb_refuse(_scan,
                                     "a word is true, false or null; NaN "
                                     "and Infinity are written "
                                     "{\"$numberDouble\": \"NaN\"}");
}

/**
 * @brief Checks that a check has room for one more level: a document or an
 *        array opening, a key, a scalar.
 *
 * @param[in,out] _scan  the check.
 * @return whether it has.
 */
static
bool d_internal_mongodb_scan_room(
    struct dj_internal_mongodb_scan* _scan
)
{
    return (_scan->depth < _scan->levels)
               ? true
               : d_internal_mongodb_refuse(_scan,
                                           "the text nests more deeply "
                                           "than libbson reads");
}

/**
 * @brief Checks a key and its colon, where a document's key is due.
 *
 * @param[in,out] _scan  the check, at the key; a value is due after.
 * @return whether they are there.
 */
static
bool d_internal_mongodb_scan_key(
    struct dj_internal_mongodb_scan* _scan
)
{
    // room for it, a string, and a colon
    if (!d_internal_mongodb_scan_room(_scan))
    {
        return false;
    }

    if (d_internal_mongodb_peek(_scan) != '"')
    {
        return d_internal_mongodb_refuse(_scan,
                                         (_scan->at < _scan->end)
                                             ? "a key is due, a string"
                                             : "the text ends where a key "
                                               "is due");
    }

    if (!d_internal_mongodb_scan_string(_scan))
    {
        return false;
    }

    d_internal_mongodb_scan_space(_scan);

    if (d_internal_mongodb_peek(_scan) != ':')
    {
        return d_internal_mongodb_refuse(_scan, "a colon is due after a key");
    }

    ++_scan->at;
    _scan->due = true;

    return true;
}

/**
 * @brief Checks a document's or an array's opening, and an empty one's
 *        closing.
 *
 * @param[in,out] _scan  the check, at the opening bracket.
 * @return whether it may open there.
 */
static
bool d_internal_mongodb_scan_open(
    struct dj_internal_mongodb_scan* _scan
)
{
    const char opening = *_scan->at;
    const char closing = (opening == '{') ? '}' : ']';

    // within the levels libbson reads
    if (!d_internal_mongodb_scan_room(_scan))
    {
        return false;
    }

    _scan->open[_scan->depth] = opening;
    ++_scan->depth;
    ++_scan->at;
    d_internal_mongodb_scan_space(_scan);

    // empty, or its first key or its first value
    if (d_internal_mongodb_peek(_scan) == closing)
    {
        ++_scan->at;
        --_scan->depth;
        _scan->due = false;

        return true;
    }

    _scan->due = true;

    return (opening == '{') ? d_internal_mongodb_scan_key(_scan) : true;
}

/**
 * @brief Checks a value, where one is due.
 *
 * @param[in,out] _scan  the check, at its first character.
 * @return whether one begins there, whole for a scalar.
 */
static
bool d_internal_mongodb_scan_value(
    struct dj_internal_mongodb_scan* _scan
)
{
    const char c = d_internal_mongodb_peek(_scan);

    _scan->due = false;

    // by its first character, a scalar a level of its own
    switch (c)
    {
        case '{':
        case '[':
            return d_internal_mongodb_scan_open(_scan);

        case '\0':
            return d_internal_mongodb_refuse(_scan,
                                             "the text ends where a value "
                                             "is due");

        default:
            break;
    }

    if (!d_internal_mongodb_scan_room(_scan))
    {
        return false;
    }

    if (c == '"')
    {
        return d_internal_mongodb_scan_string(_scan);
    }

    if ( (c == '-') ||
         ( (c >= '0') &&
           (c <= '9') ) )
    {
        return d_internal_mongodb_scan_number(_scan);
    }

    return ( ( (c >= 'a') &&
               (c <= 'z') ) ||
             ( (c >= 'A') &&
               (c <= 'Z') ) )
               ? d_internal_mongodb_scan_literal(_scan)
               : d_internal_mongodb_refuse(_scan,
                                           "a value is due: a document, an "
                                           "array, a string, a number, "
                                           "true, false or null");
}

/**
 * @brief Checks what follows a value inside a document or an array: a
 *        comma and the next, or the closing bracket.
 *
 * @param[in,out] _scan  the check, past the value and its space.
 * @return whether one of them is there.
 */
static
bool d_internal_mongodb_scan_after(
    struct dj_internal_mongodb_scan* _scan
)
{
    const bool in_document = (_scan->open[_scan->depth - 1u] == '{');
    const char c           = d_internal_mongodb_peek(_scan);

    // the next, which a document keys
    if (c == ',')
    {
        ++_scan->at;
        d_internal_mongodb_scan_space(_scan);
        _scan->due = true;

        return (in_document) ? d_internal_mongodb_scan_key(_scan) : true;
    }

    // the close
    if (c == ( (in_document) ? '}' : ']' ))
    {
        ++_scan->at;
        --_scan->depth;

        return true;
    }

    return d_internal_mongodb_refuse(
               _scan,
               (_scan->at >= _scan->end)
                   ? ( (in_document) ? "the text ends inside a document"
                                     : "the text ends inside an array" )
                   : ( (in_document) ? "a comma or a closing brace is due"
                                     : "a comma or a closing bracket is "
                                       "due" ));
}

/**
 * @brief Checks a text against JSON's grammar: one value, whole.
 *
 * @param[in,out] _db      the handle, on which a refusal is recorded.
 * @param[in]     _base    the text the caller gave, which a refusal counts
 *                         its bytes from.
 * @param[in]     _text    the value's first character.
 * @param[in]     _length  the value's length in bytes.
 * @param[in]     _levels  the most documents and arrays it may nest.
 * @return 0, or libbson's code for corrupt JSON, packed and recorded.
 */
static
int d_internal_mongodb_check_json(
    struct dj_mongodb* _db,
    const char*        _base,
    const char*        _text,
    size_t             _length,
    size_t             _levels
)
{
    struct dj_internal_mongodb_scan scan =
        { _text, _text + _length, NULL, 0u, _levels, true, { 0 } };
    bool                           good = true;

    // each value, and what follows each inside a container
    do
    {
        d_internal_mongodb_scan_space(&scan);
        good = (scan.due) ? d_internal_mongodb_scan_value(&scan)
                          : d_internal_mongodb_scan_after(&scan);
    } while ( (good) &&
              ( (scan.due) ||
                (scan.depth > 0u) ) );

    // nothing after the value but space
    if (good)
    {
        d_internal_mongodb_scan_space(&scan);
        good = (scan.at < scan.end)
                   ? d_internal_mongodb_refuse(&scan,
                                               "the text goes on after its "
                                               "value")
                   : true;
    }

    // libbson's code, as its own refusals have it
    if (!good)
    {
        bson_error_t error;

        bson_set_error(&error,
                       BSON_ERROR_JSON,
                       BSON_JSON_ERROR_READ_CORRUPT_JS,
                       "not JSON at byte %lu: %s",
                       (unsigned long)(scan.at - _base),
                       scan.why);

        return d_internal_mongodb_fail_bson(_db, &error);
    }

    return 0;
}

/**
 * @brief Has libbson read exactly the text given into a document.
 *
 * @param[in,out] _db      the handle, on which a failure is recorded.
 * @param[in]     _text    the text.
 * @param[in]     _length  its length in bytes.
 * @param[in,out] _out     an initialized document, which receives it.
 * @return 0, or libbson's failure, packed and recorded.
 *
 * A read that fails without a word -- libbson's reader returns 0 for text
 * it finds no document in -- is JSON that ended too soon.
 */
static
int d_internal_mongodb_parse(
    struct dj_mongodb* _db,
    const char*        _text,
    size_t             _length,
    bson_t*            _out
)
{
    bson_json_reader_t* const reader = bson_json_data_reader_new(false, 0u);
    bson_error_t              error;

    memset(&error, 0, sizeof(error));
    bson_json_data_reader_ingest(reader, (const uint8_t*)_text, _length);

    const int read = bson_json_reader_read(reader, _out, &error);

    bson_json_reader_destroy(reader);

    // the one document read
    if (read == 1)
    {
        return 0;
    }

    // a failure libbson did not describe
    if ( (error.domain == 0u) &&
         (error.code == 0u) )
    {
        error.domain = BSON_ERROR_JSON;
        error.code   = BSON_JSON_ERROR_READ_CORRUPT_JS;
        bson_strncpy(error.message, "Incomplete JSON", sizeof(error.message));
    }

    return d_internal_mongodb_fail_bson(_db, &error);
}

/**
 * @brief Describes what a JSON text begins with, for a message.
 *
 * @param[in] _start  the text's first character that is not space.
 * @return the description.
 */
static
const char* d_internal_mongodb_describe(
    const char* _start
)
{
    switch (*_start)
    {
        case '\0':
            return "is empty";

        case '{':
            return "is a document";

        case '[':
            return "is an array";

        default:
            return "is neither a document nor an array";
    }
}

/**
 * @brief Reads a JSON document or array, whole and alone, into a document.
 *
 * @param[in,out] _db        the handle, on which a failure is recorded.
 * @param[in]     _json      the text; `NULL` for an empty one.
 * @param[in]     _required  whether `NULL` is refused.
 * @param[in]     _array     whether an array is due, else a document.
 * @param[out]    _out       initialized here, and receives it.
 * @return 0; D_DB_INVALID_ARGUMENT for no text where one is required, text
 *         that is the other of the two, or anything after it but space;
 *         libbson's failure, packed.
 */
static
int d_internal_mongodb_read_container(
    struct dj_mongodb* _db,
    const char*        _json,
    bool               _required,
    bool               _array,
    bson_t*            _out
)
{
    const char* const what = (_array) ? "an array" : "a document";

    bson_init(_out);

    // no text: empty, where that will do
    if (!_json)
    {
        return (_required)
                   ? d_internal_mongodb_answer(_db,
                                               D_DB_INVALID_ARGUMENT,
                                               "%s is required, and none "
                                               "was given",
                                               what)
                   : 0;
    }

    const char* const start = d_internal_mongodb_skip_space(_json);

    // the text must open what is due
    if (*start != ( (_array) ? '[' : '{' ))
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "%s was expected, and the JSON "
                                         "text given %s",
                                         what,
                                         d_internal_mongodb_describe(start));
    }

    const char* const end  = d_internal_mongodb_container_end(start);
    const char* const rest = (end) ? d_internal_mongodb_skip_space(end)
                                   : NULL;

    // nothing but space may follow it
    if ( (rest) &&
         (*rest != '\0') )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "the JSON text given goes on after "
                                         "%s, at byte %lu",
                                         what,
                                         (unsigned long)(rest - _json));
    }

    const size_t length = (end) ? (size_t)(end - start) : strlen(start);
    const int    result = d_internal_mongodb_check_json(
                              _db,
                              _json,
                              start,
                              length,
                              D_INTERNAL_MONGODB_JSON_LEVELS);

    return (result == 0)
               ? d_internal_mongodb_parse(_db, start, length, _out)
               : result;
}

/*
d_internal_mongodb_read_document
  One document; an array is refused, which libbson would read as one.
*/
int d_internal_mongodb_read_document(
    struct dj_mongodb* _db,
    const char*        _json,
    bool               _required,
    bson_t*            _out
)
{
    return d_internal_mongodb_read_container(_db,
                                             _json,
                                             _required,
                                             false,
                                             _out);
}

/*
d_internal_mongodb_read_array
  One array, which libbson reads as the document BSON keeps an array as, its
keys "0", "1" and on.
*/
int d_internal_mongodb_read_array(
    struct dj_mongodb* _db,
    const char*        _json,
    bool               _required,
    bson_t*            _out
)
{
    return d_internal_mongodb_read_container(_db,
                                             _json,
                                             _required,
                                             true,
                                             _out);
}

/*
d_internal_mongodb_read_either
  The first character says which; text that opens neither is refused as a
document's would be.
*/
int d_internal_mongodb_read_either(
    struct dj_mongodb* _db,
    const char*        _json,
    bson_t*            _out,
    bool*              _array
)
{
    *_array = ( (_json) &&
                (*d_internal_mongodb_skip_space(_json) == '[') );

    return d_internal_mongodb_read_container(_db,
                                             _json,
                                             true,
                                             *_array,
                                             _out);
}

/**
 * @brief Has libbson read one value, as the one field of {"v": ...}.
 *
 * @param[in,out] _db      the handle, on which a failure is recorded.
 * @param[in]     _text    the value's text, checked.
 * @param[in]     _length  its length in bytes.
 * @param[in,out] _holder  an initialized document, which receives the
 *                         holder.
 * @param[out]    _value   receives the value, pointing into the holder.
 * @return 0; libbson's failure, packed; D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_parse_value(
    struct dj_mongodb* _db,
    const char*        _text,
    size_t             _length,
    bson_t*            _holder,
    bson_value_t*      _value
)
{
    struct dj_internal_db_buffer wrapped = { NULL, 0u, 0u };

    // {"v": the value}
    if ( (!d_internal_db_buffer_append(&wrapped, "{\"v\":", 5u))   ||
         (!d_internal_db_buffer_append(&wrapped, _text, _length)) ||
         (!d_internal_db_buffer_append(&wrapped, "}", 1u)) )
    {
        d_internal_db_buffer_release(&wrapped);

        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory to read a value");
    }

    const int result = d_internal_mongodb_parse(_db,
                                                wrapped.data,
                                                wrapped.length,
                                                _holder);
    bson_iter_t field;

    d_internal_db_buffer_release(&wrapped);

    // the value, inside its holder
    if ( (result == 0) &&
         (bson_iter_init_find(&field, _holder, "v")) )
    {
        *_value = *bson_iter_value(&field);
    }

    return result;
}

/*
d_internal_mongodb_read_value
  libbson reads documents only, so the value is read as the one field of
{"v": ...}, once the layer has found that it is one value and nothing
after: a container by its brackets, a string by its quotes, any other
scalar up to space or punctuation, and checked against JSON's grammar. The
grammar's refusals count bytes from the text given; libbson's, from the
start of that wrapping, five before the value's own.
*/
int d_internal_mongodb_read_value(
    struct dj_mongodb* _db,
    const char*        _json,
    bson_t*            _holder,
    bson_value_t*      _value
)
{
    bson_init(_holder);

    // a value must be given
    if (!_json)
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "a value is required, and none was "
                                         "given");
    }

    const char* const start = d_internal_mongodb_skip_space(_json);
    const char* const end   = ( (*start == '{') ||
                                (*start == '[') )
                                  ? d_internal_mongodb_container_end(start)
                                  : ( (*start)
                                          ? d_internal_mongodb_scalar_end(start)
                                          : NULL );
    const char* const rest  = (end) ? d_internal_mongodb_skip_space(end)
                                    : NULL;

    // one value, and nothing after it but space
    if ( (!*start) ||
         ( (rest) &&
           (*rest != '\0') ) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         (*start) ? "the JSON text given goes "
                                                    "on after its value"
                                                  : "a value was expected, "
                                                    "and the JSON text given "
                                                    "is empty");
    }

    const size_t length  = (end) ? (size_t)(end - start) : strlen(start);
    const int    checked = d_internal_mongodb_check_json(
                               _db,
                               _json,
                               start,
                               length,
                               D_INTERNAL_MONGODB_JSON_LEVELS - 1u);

    // JSON, nesting no more deeply than its holder leaves room for
    if (checked != 0)
    {
        return checked;
    }

    return d_internal_mongodb_parse_value(_db, start, length, _holder, _value);
}

// 2.2    Writing
//------------------------------------------------------------------------------
/**
 * @brief Writes a document as Extended JSON with libbson's writers.
 *
 * @param[in]  _document  the document.
 * @param[in]  _format    relaxed, or canonical for anything else.
 * @param[out] _length    receives the text's length.
 * @return the text, libbson's, released with bson_free; `NULL` on failure.
 */
static char*
d_internal_mongodb_write(
    const bson_t*                  _document,
    enum dj_mongodb_document_format _format,
    size_t*                        _length
)
{
    return (_format == D_MONGODB_JSON_RELAXED)
               ? bson_as_relaxed_extended_json(_document, _length)
               : bson_as_canonical_extended_json(_document, _length);
}

/*
d_internal_mongodb_json
  libbson's text is its allocator's; the caller's is malloc's, for d_db_free.
*/
char*
d_internal_mongodb_json(
    const bson_t*                  _document,
    enum dj_mongodb_document_format _format
)
{
    size_t      length = 0u;
    char* const text   = d_internal_mongodb_write(_document,
                                                  _format,
                                                  &length);

    // libbson could not write it
    if (!text)
    {
        return NULL;
    }

    char* const copy = (char*)malloc(length + 1u);

    // the copy, when there is room
    if (copy)
    {
        memcpy(copy, text, length);
        copy[length] = '\0';
    }

    bson_free(text);

    return copy;
}

/*
d_internal_mongodb_value_json
  libbson writes documents only, so the value is written as the one field
of { "v" : ... } and cut out of it; the writers' spacing is fixed.
*/
char*
d_internal_mongodb_value_json(
    const bson_value_t*            _value,
    enum dj_mongodb_document_format _format
)
{
    const size_t prefix = sizeof(VALUE_PREFIX) - 1u;
    const size_t suffix = sizeof(VALUE_SUFFIX) - 1u;
    bson_t       holder;

    bson_init(&holder);

    size_t      length = 0u;
    char* const text   = (bson_append_value(&holder, "v", 1, _value))
                             ? d_internal_mongodb_write(&holder,
                                                        _format,
                                                        &length)
                             : NULL;

    bson_destroy(&holder);

    // the writer's text must be the shape it always is
    if ( (!text)                                               ||
         (length < (prefix + suffix))                          ||
         (memcmp(text, VALUE_PREFIX, prefix) != 0)             ||
         (memcmp(text + length - suffix, VALUE_SUFFIX, suffix) != 0) )
    {
        bson_free(text);

        return NULL;
    }

    const size_t size = length - prefix - suffix;
    char* const  copy = (char*)malloc(size + 1u);

    // the value, when there is room
    if (copy)
    {
        memcpy(copy, text + prefix, size);
        copy[size] = '\0';
    }

    bson_free(text);

    return copy;
}

/*
d_internal_mongodb_strdup
  strdup is POSIX's, not C's.
*/
char* d_internal_mongodb_strdup(
    const char* _text
)
{
    // nothing to copy
    if (!_text)
    {
        return NULL;
    }

    const size_t size = strlen(_text) + 1u;
    char* const  copy = (char*)malloc(size);

    // the copy, when there is room
    if (copy)
    {
        memcpy(copy, _text, size);
    }

    return copy;
}

// 2.3    Formats
//------------------------------------------------------------------------------
/**
 * @brief Appends a string to a buffer as a JSON string: quoted, its quotes,
 *        backslashes and control characters escaped.
 *
 * @param[in,out] _context  a bool, set when memory runs out, so that the
 *                          formatter's one failure can be told apart.
 * @param[in,out] _out      the buffer.
 * @param[in]     _text     the string.
 * @return `true`; `false` for text that is not UTF-8, which JSON cannot
 *         hold, or when memory ran out, the buffer's length as it was.
 */
static
bool d_internal_mongodb_quote(
    void*                         _context,
    struct dj_internal_db_buffer* _out,
    const char*                   _text
)
{
    static const char HEX[] = "0123456789abcdef";

    const size_t length = strlen(_text);
    const size_t mark   = _out->length;
    bool         ok     = true;
    size_t       index  = 0u;

    // JSON is UTF-8
    if (!bson_utf8_validate(_text, length, false))
    {
        return false;
    }

    ok = d_internal_db_buffer_append(_out, "\"", 1u);

    // each byte, escaped where JSON requires it
    for (index = 0u; (ok) && (index < length); ++index)
    {
        const unsigned char c = (unsigned char)_text[index];
        char                escape[6] = { '\\', 'u', '0', '0', '0', '0' };

        if ( (c == '"') ||
             (c == '\\') )
        {
            escape[1] = (char)c;
            ok        = d_internal_db_buffer_append(_out, escape, 2u);
        }
        else if (c < 0x20u)
        {
            escape[4] = HEX[c >> 4];
            escape[5] = HEX[c & 0x0Fu];
            ok        = d_internal_db_buffer_append(_out, escape, 6u);
        }
        else
        {
            ok = d_internal_db_buffer_append(_out, &_text[index], 1u);
        }
    }

    ok = (ok) && (d_internal_db_buffer_append(_out, "\"", 1u));

    // a failure leaves the buffer as it was, and says why
    if (!ok)
    {
        bool* const no_memory = (bool*)_context;

        _out->length = mark;
        *no_memory   = true;
    }

    return ok;
}

/*
d_internal_mongodb_vformat
  The shared formatter, in JSON's dialect, with JSON's quoting for %Q and
%I alike. Its one refusal covers both a value it cannot write and a quote
that ran out of memory; the quoting's flag tells them apart.
*/
int d_internal_mongodb_vformat(
    struct dj_mongodb* _db,
    char**             _text,
    const char*        _format,
    va_list            _args
)
{
    static const struct dj_internal_db_dialect DIALECT =
        D_INTERNAL_DB_JSON_DIALECT;

    bool                               no_memory = false;
    const struct dj_internal_db_quoting quoting   =
    {
        d_internal_mongodb_quote,
        d_internal_mongodb_quote,
        &no_memory
    };
    struct dj_internal_db_buffer        out       = { NULL, 0u, 0u };

    const enum dj_internal_db_format_status status =
        d_internal_db_vformat_dialect(&out,
                                      &quoting,
                                      &DIALECT,
                                      _format,
                                      _args);

    // the command, formatted
    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        *_text = out.data;

        return 0;
    }

    d_internal_db_buffer_release(&out);

    // memory, a value, or the format itself
    if ( (status == D_INTERNAL_DB_FORMAT_NO_MEMORY) ||
         (no_memory) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory to format the command");
    }

    return d_internal_mongodb_answer(_db,
                                     D_DB_INVALID_ARGUMENT,
                                     (status == D_INTERNAL_DB_FORMAT_REFUSED)
                                         ? "the format was given a value "
                                           "JSON cannot hold: a NaN, an "
                                           "infinity, a NUL, text that is "
                                           "not UTF-8, a NULL name, or a %%T"
                                         : "the format has a conversion the "
                                           "layer cannot read");
}


//==============================================================================
// 3.  OPERATIONS
//==============================================================================

// 3.1    Names
//------------------------------------------------------------------------------
/*
d_internal_mongodb_database
  An empty name is refused here, as the server refuses it, since the
driver would send it as written.
*/
const char* d_internal_mongodb_database(
    struct dj_mongodb* _db,
    const char*        _database
)
{
    const char* const name = (_database) ? _database : _db->database;

    // a database must be named, or current
    if (!name)
    {
        (void)d_internal_mongodb_answer(_db,
                                        D_DB_INVALID_ARGUMENT,
                                        "no database was named, and the "
                                        "handle has no current database");

        return NULL;
    }

    // the server has no database of no name
    if (!*name)
    {
        (void)d_internal_mongodb_answer(_db,
                                        D_MONGODB_ERROR_INVALID_NAMESPACE,
                                        "a database's name cannot be empty");

        return NULL;
    }

    return name;
}

// 3.2    Options
//------------------------------------------------------------------------------
/*
d_internal_mongodb_options
  maxTimeMS is added as an int64, as the drivers send it.
*/
int d_internal_mongodb_options(
    struct dj_mongodb* _db,
    const char*        _json,
    unsigned           _what,
    bson_t*            _opts
)
{
    const int result = d_internal_mongodb_read_document(_db,
                                                        _json,
                                                        false,
                                                        _opts);

    // options that are not a document
    if (result != 0)
    {
        return result;
    }

    // the handle's time limit, for a read that names none
    if ( ((_what & D_INTERNAL_MONGODB_READ) != 0u) &&
         (_db->max_time_ms > 0)                    &&
         (!bson_has_field(_opts, "maxTimeMS")) )
    {
        BSON_APPEND_INT64(_opts, "maxTimeMS", _db->max_time_ms);
    }

    return ((_what & D_INTERNAL_MONGODB_SESSION) != 0u)
               ? d_internal_mongodb_session(_db, _opts)
               : 0;
}

/*
d_internal_mongodb_session
  Appended as the driver appends it, sessionId; only while a transaction is
open, since outside one every operation may run in an implicit session of
the driver's own.
*/
int d_internal_mongodb_session(
    struct dj_mongodb* _db,
    bson_t*            _opts
)
{
    bson_error_t error;

    // nothing open
    if ( (!_db->session) ||
         (!mongoc_client_session_in_transaction(_db->session)) )
    {
        return 0;
    }

    return (mongoc_client_session_append(_db->session, _opts, &error))
               ? 0
               : d_internal_mongodb_fail(_db, &error, NULL);
}

// 3.3    Commands
//------------------------------------------------------------------------------
/*
d_internal_mongodb_run
  mongoc_client_command_with_opts applies none of the client's concerns or
preference, only what the options say, which is what "as written" means;
with no read preference it sends to the primary. It initializes the reply
on every path, over the empty one here, which holds no memory.
*/
int d_internal_mongodb_run(
    struct dj_mongodb* _db,
    const char*        _database,
    const bson_t*      _command,
    bool               _outside,
    bson_t*            _reply
)
{
    bson_t       opts;
    bson_error_t error;

    bson_init(_reply);
    bson_init(&opts);

    int result = (_outside) ? 0 : d_internal_mongodb_session(_db, &opts);

    // the command, then what its reply says
    if (result == 0)
    {
        result = (mongoc_client_command_with_opts(_db->client,
                                                  _database,
                                                  _command,
                                                  NULL,
                                                  &opts,
                                                  _reply,
                                                  &error))
                     ? d_internal_mongodb_reply_failure_with(_db,
                                                             _reply,
                                                             !_outside)
                     : d_internal_mongodb_fail_with(_db,
                                                    &error,
                                                    _reply,
                                                    !_outside);
    }

    bson_destroy(&opts);

    return result;
}

// 3.4    Rows
//------------------------------------------------------------------------------
/*
d_internal_mongodb_rows_column
  Both are typed JSON; a value is named as one, a document as one.
*/
bool d_internal_mongodb_rows_column(
    struct dj_internal_db_rows* _rows,
    bool                        _values
)
{
    return d_internal_db_rows_column(_rows,
                                     (_values) ? "value" : "document",
                                     D_DB_TYPE_JSON,
                                     (_values) ? "value" : "document");
}

/*
d_internal_mongodb_rows_document
  BSON goes in as bytes; JSON as text, its length libbson's.
*/
bool
d_internal_mongodb_rows_document(
    struct dj_internal_db_rows*     _rows,
    const bson_t*                  _document,
    enum dj_mongodb_document_format _format
)
{
    // the bytes themselves
    if (_format == D_MONGODB_BSON)
    {
        return d_internal_db_rows_blob(_rows,
                                       bson_get_data(_document),
                                       (size_t)_document->len);
    }

    size_t      length = 0u;
    char* const text   = d_internal_mongodb_write(_document,
                                                  _format,
                                                  &length);
    const bool  added  = (text) &&
                         (d_internal_db_rows_text(_rows, text, length));

    bson_free(text);

    return added;
}

/*
d_internal_mongodb_rows_value
  A value has no BSON of its own, so the BSON format writes it canonical.
*/
bool
d_internal_mongodb_rows_value(
    struct dj_internal_db_rows*     _rows,
    const bson_value_t*            _value,
    enum dj_mongodb_document_format _format
)
{
    char* const text  = d_internal_mongodb_value_json(_value, _format);
    const bool  added = (text) &&
                        (d_internal_db_rows_text(_rows, text, strlen(text)));

    free(text);

    return added;
}

/**
 * @brief Reads a cursor's documents into rows, up to a page's end or the
 *        cursor's.
 *
 * @param[in]     _db       the handle, whose format the rows are written in.
 * @param[in,out] _cursor   the cursor.
 * @param[in]     _max      the most to read.
 * @param[in,out] _builder  the rows, their column made.
 * @param[out]    _more     receives whether the cursor had a document at
 *                          the last ask, `false` at its end or a failure.
 * @return `true`, or `false` where memory ran out.
 */
static
bool d_internal_mongodb_rows_page(
    const struct dj_mongodb*    _db,
    mongoc_cursor_t*            _cursor,
    size_t                      _max,
    struct dj_internal_db_rows* _builder,
    bool*                       _more
)
{
    const bson_t* document = NULL;
    bool          ok       = true;
    size_t        count    = 0u;

    *_more = true;

    // documents, up to the page's end or the cursor's
    while ( (ok)     &&
            (*_more) &&
            (count < _max) )
    {
        *_more = mongoc_cursor_next(_cursor, &document);
        ok     = (!*_more) ||
                 (d_internal_mongodb_rows_document(_builder,
                                                   document,
                                                   _db->format));
        count += (*_more) ? 1u : 0u;
    }

    return ok;
}

/*
d_internal_mongodb_cursor_rows
  A cursor's failure shows only once mongoc_cursor_next returns false, so
it is asked then, and only then. Done is the driver's guess when the page
filled -- mongoc_cursor_more may say there are more and find none -- and
certain when the cursor ran out.
*/
int d_internal_mongodb_cursor_rows(
    struct dj_mongodb* _db,
    mongoc_cursor_t*   _cursor,
    size_t             _max,
    struct dj_db_rows* _rows,
    bool*              _done,
    bool               _session
)
{
    struct dj_internal_db_rows builder = { { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          0u,
                                          0u };
    bool                      more     = true;
    const bool                ok       =
        ( (d_internal_mongodb_rows_column(&builder, false)) &&
          (d_internal_mongodb_rows_page(_db,
                                        _cursor,
                                        _max,
                                        &builder,
                                        &more)) );
    bson_error_t              error;
    const bson_t*             reply    = NULL;

    // the cursor's failure, which ends it
    if ( (ok)    &&
         (!more) &&
         (mongoc_cursor_error_document(_cursor, &error, &reply)) )
    {
        d_internal_db_rows_release(&builder);

        if (_done)
        {
            *_done = true;
        }

        return d_internal_mongodb_fail_with(_db, &error, reply, _session);
    }

    // the rows, packed
    if ( (!ok) ||
         (!d_internal_db_rows_finish(&builder, _rows)) )
    {
        d_internal_db_rows_release(&builder);

        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the documents read");
    }

    // whether more may follow
    if (_done)
    {
        *_done = ( (!more) ||
                   (!mongoc_cursor_more(_cursor)) );
    }

    return 0;
}

// 3.5    The catalog
//------------------------------------------------------------------------------
/**
 * @brief Opens a listCollections cursor of names and types, outside any
 *        transaction.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database.
 * @param[in]  _filter    the filter on what it lists.
 * @param[out] _handle    receives the database the cursor reads, which the
 *                        caller destroys after it.
 * @return the cursor, which the caller destroys.
 *
 * nameOnly keeps the server from reading every collection's options; with
 * authorizedCollections, a user who may not list them all lists those it
 * may use, as the drivers' listCollectionNames does.
 */
static
mongoc_cursor_t* d_internal_mongodb_list_cursor(
    struct dj_mongodb*  _db,
    const char*         _database,
    const bson_t*       _filter,
    mongoc_database_t** _handle
)
{
    bson_t opts;

    bson_init(&opts);
    BSON_APPEND_DOCUMENT(&opts, "filter", _filter);
    BSON_APPEND_BOOL(&opts, "nameOnly", true);
    BSON_APPEND_BOOL(&opts, "authorizedCollections", true);

    *_handle = mongoc_client_get_database(_db->client, _database);

    mongoc_cursor_t* const cursor =
        mongoc_database_find_collections_with_opts(*_handle, &opts);

    bson_destroy(&opts);

    return cursor;
}

/**
 * @brief Ends a listCollections cursor: its failure, if it had one,
 *        recorded as outside the transaction.
 *
 * @param[in] _db      the handle.
 * @param[in] _cursor  the cursor, destroyed.
 * @param[in] _handle  its database, destroyed.
 * @return 0, or the failure.
 */
static
int d_internal_mongodb_list_end(
    struct dj_mongodb* _db,
    mongoc_cursor_t*   _cursor,
    mongoc_database_t* _handle
)
{
    bson_error_t  error;
    const bson_t* reply  = NULL;
    const int     result = (mongoc_cursor_error_document(_cursor,
                                                         &error,
                                                         &reply))
                               ? d_internal_mongodb_fail_outside(_db,
                                                                 &error,
                                                                 reply)
                               : 0;

    mongoc_cursor_destroy(_cursor);
    mongoc_database_destroy(_handle);

    return result;
}

/*
d_internal_mongodb_kind_of
  listCollections answers "collection", "view" or, from 5.0, "timeseries";
only a view is not a collection here.
*/
int
d_internal_mongodb_kind_of(
    struct dj_mongodb*             _db,
    const char*                   _database,
    const char*                   _name,
    enum dj_internal_mongodb_kind* _kind
)
{
    bson_t filter;

    bson_init(&filter);
    BSON_APPEND_UTF8(&filter, "name", _name);
    *_kind = D_INTERNAL_MONGODB_NOTHING;

    mongoc_database_t*     handle = NULL;
    mongoc_cursor_t* const cursor = d_internal_mongodb_list_cursor(_db,
                                                                   _database,
                                                                   &filter,
                                                                   &handle);
    const bson_t*          entry  = NULL;

    // the entry, if the name has one
    while (mongoc_cursor_next(cursor, &entry))
    {
        bson_iter_t type;

        *_kind = ( (bson_iter_init_find(&type, entry, "type")) &&
                   (BSON_ITER_HOLDS_UTF8(&type))              &&
                   (strcmp(bson_iter_utf8(&type, NULL), "view") == 0) )
                     ? D_INTERNAL_MONGODB_VIEW
                     : D_INTERNAL_MONGODB_COLLECTION;
    }

    const int result = d_internal_mongodb_list_end(_db, cursor, handle);

    bson_destroy(&filter);

    // a failure finds nothing
    if (result != 0)
    {
        *_kind = D_INTERNAL_MONGODB_NOTHING;
    }

    return result;
}

/*
d_internal_mongodb_list_kind
  Filtered on the server by type: a view's is "view", and a collection's
anything else.
*/
int
d_internal_mongodb_list_kind(
    struct dj_mongodb*            _db,
    const char*                  _database,
    enum dj_internal_mongodb_kind _kind,
    struct dj_db_name_list*       _names
)
{
    bson_t filter;

    bson_init(&filter);

    // {type: "view"}, or {type: {$ne: "view"}}
    if (_kind == D_INTERNAL_MONGODB_VIEW)
    {
        BSON_APPEND_UTF8(&filter, "type", "view");
    }
    else
    {
        bson_t not_view;

        BSON_APPEND_DOCUMENT_BEGIN(&filter, "type", &not_view);
        BSON_APPEND_UTF8(&not_view, "$ne", "view");
        bson_append_document_end(&filter, &not_view);
    }

    struct dj_internal_db_names builder = { { NULL, 0u, 0u },
                                           { NULL, 0u, 0u },
                                           0u };
    mongoc_database_t*         handle  = NULL;
    mongoc_cursor_t* const     cursor  = d_internal_mongodb_list_cursor(
                                             _db,
                                             _database,
                                             &filter,
                                             &handle);
    const bson_t*              entry   = NULL;
    bool                       ok      = true;

    // each name, in the server's order
    while ( (ok) &&
            (mongoc_cursor_next(cursor, &entry)) )
    {
        bson_iter_t name;

        ok = ( (!bson_iter_init_find(&name, entry, "name")) ||
               (!BSON_ITER_HOLDS_UTF8(&name))              ||
               (d_internal_db_names_add(&builder,
                                        bson_iter_utf8(&name, NULL))) );
    }

    int result = d_internal_mongodb_list_end(_db, cursor, handle);

    bson_destroy(&filter);

    // the names, packed
    if ( (result == 0) &&
         ( (!ok) ||
           (!d_internal_db_names_finish(&builder, _names)) ) )
    {
        result = d_internal_mongodb_answer(_db,
                                           D_MONGODB_LAYER_NO_MEMORY,
                                           "no memory for the names listed");
    }

    d_internal_db_names_release(&builder);

    return result;
}

// 3.6    Writes
//------------------------------------------------------------------------------
/**
 * @brief Reads a count from a reply, by either of two names.
 *
 * @param[in]  _reply   the reply.
 * @param[in]  _first   the CRUD specification's name.
 * @param[in]  _second  a bulk write's, or `NULL`.
 * @return the count; 0 where neither is there.
 */
static
int64_t d_internal_mongodb_count(
    const bson_t* _reply,
    const char*   _first,
    const char*   _second
)
{
    bson_iter_t field;

    // the first name, then the second
    if ( ( (bson_iter_init_find(&field, _reply, _first)) ||
           ( (_second) &&
             (bson_iter_init_find(&field, _reply, _second)) ) ) &&
         (BSON_ITER_HOLDS_NUMBER(&field)) )
    {
        return bson_iter_as_int64(&field);
    }

    return 0;
}

/*
d_internal_mongodb_counts
  The CRUD functions' replies name their counts insertedCount and the like;
a bulk write's nInserted, nRemoved and the like.
*/
void d_internal_mongodb_counts(
    const bson_t*                   _reply,
    struct dj_mongodb_write_counts* _counts
)
{
    _counts->inserted = d_internal_mongodb_count(_reply,
                                                 "insertedCount",
                                                 "nInserted");
    _counts->matched  = d_internal_mongodb_count(_reply,
                                                 "matchedCount",
                                                 "nMatched");
    _counts->modified = d_internal_mongodb_count(_reply,
                                                 "modifiedCount",
                                                 "nModified");
    _counts->deleted  = d_internal_mongodb_count(_reply,
                                                 "deletedCount",
                                                 "nRemoved");
    _counts->upserted = d_internal_mongodb_count(_reply,
                                                 "upsertedCount",
                                                 "nUpserted");

    return;
}

/*
d_internal_mongodb_wrote
  A write's count replaces the last, and its new id, which d_internal_
mongodb_new_id records after it, replaces the last id: a write that made no
new document leaves none.
*/
void d_internal_mongodb_wrote(
    struct dj_mongodb*                    _db,
    const struct dj_mongodb_write_counts* _counts
)
{
    _db->changes = _counts->inserted +
                   _counts->modified +
                   _counts->deleted  +
                   _counts->upserted;

    d_internal_mongodb_new_id(_db, NULL);

    return;
}

/*
d_internal_mongodb_new_id
  The value is copied, since what it points into is the reply's.
*/
void d_internal_mongodb_new_id(
    struct dj_mongodb*  _db,
    const bson_value_t* _id
)
{
    // the last id goes
    if (_db->has_id)
    {
        bson_value_destroy(&_db->id);
        _db->has_id = false;
    }

    // the new one, if there is one
    if (_id)
    {
        bson_value_copy(_id, &_db->id);
        _db->has_id = true;
    }

    return;
}

// 3.7    The log
//------------------------------------------------------------------------------
/**
 * @brief The layer's handler for the driver's log.
 *
 * @param[in] _level      the entry's level.
 * @param[in] _domain     its domain.
 * @param[in] _message    its message.
 * @param[in] _user_data  the level passed: a pointer into
 *                        d_internal_mongodb_log_levels.
 *
 * The driver calls it holding its own lock; it reads only its user data,
 * which the setter replaces through the driver, under that lock.
 */
static
void d_internal_mongodb_log_handler(
    mongoc_log_level_t _level,
    const char*        _domain,
    const char*        _message,
    void*              _user_data
)
{
    const int passed = *((const int*)_user_data);

    // what is at the level passed, or more urgent
    if ((int)_level <= passed)
    {
        mongoc_log_default_handler(_level, _domain, _message, NULL);
    }

    return;
}

/**
 * @brief Initializes the driver and makes the log's lock, once for the
 *        process.
 *
 * mongoc_init is the driver's to call once, which it guards itself; it is
 * called here so that the layer's first call makes it, as
 * mongodb_connection.h promises.
 */
static
void d_internal_mongodb_initialize(void)
{
    mongoc_init();
    d_internal_mongodb_log_ready =
        (d_mutex_init(&d_internal_mongodb_log_lock) == D_MUTEX_SUCCESS);

    return;
}

/**
 * @brief Installs the layer's handler at a level; the lock is held.
 *
 * @param[in] _level  the level, from -1 to 6.
 */
static
void d_internal_mongodb_install(
    int _level
)
{
    mongoc_log_set_handler(d_internal_mongodb_log_handler,
                           &d_internal_mongodb_log_levels[_level + 1]);
    d_internal_mongodb_log_level     = _level;
    d_internal_mongodb_log_installed = true;

    return;
}

/*
d_internal_mongodb_start
  The knob's -1 installs nothing, leaving the handler the driver's or the
program's; a setter that ran first has the say.
*/
void d_internal_mongodb_start(void)
{
    d_call_once(&d_internal_mongodb_log_once, d_internal_mongodb_initialize);

    // the log, once, at the knob's level
    if ( (d_internal_mongodb_log_ready) &&
         (d_mutex_lock(&d_internal_mongodb_log_lock) == D_MUTEX_SUCCESS) )
    {
        if ( (!d_internal_mongodb_log_installed) &&
             (D_INTERNAL_MONGODB_LOG_LEVEL >= 0) )
        {
            d_internal_mongodb_install(D_INTERNAL_MONGODB_LOG_LEVEL);
        }

        (void)d_mutex_unlock(&d_internal_mongodb_log_lock);
    }

    return;
}

/*
d_internal_mongodb_log_set
  The lock is held while the driver installs the handler, which takes the
driver's own; the handler never takes the layer's, so the two locks are
only ever taken in that order.
*/
bool d_internal_mongodb_log_set(
    int _level
)
{
    d_call_once(&d_internal_mongodb_log_once, d_internal_mongodb_initialize);

    // no lock, no change
    if ( (!d_internal_mongodb_log_ready) ||
         (d_mutex_lock(&d_internal_mongodb_log_lock) != D_MUTEX_SUCCESS) )
    {
        return false;
    }

    d_internal_mongodb_install(_level);
    (void)d_mutex_unlock(&d_internal_mongodb_log_lock);

    return true;
}

/*
d_internal_mongodb_log_get
  Read under the lock, as it is written.
*/
int d_internal_mongodb_log_get(void)
{
    d_call_once(&d_internal_mongodb_log_once, d_internal_mongodb_initialize);

    // without the lock, nothing was installed: the knob's
    if ( (!d_internal_mongodb_log_ready) ||
         (d_mutex_lock(&d_internal_mongodb_log_lock) != D_MUTEX_SUCCESS) )
    {
        return D_INTERNAL_MONGODB_LOG_LEVEL;
    }

    const int level = d_internal_mongodb_log_level;

    (void)d_mutex_unlock(&d_internal_mongodb_log_lock);

    return level;
}


// 3.8    Collections
//------------------------------------------------------------------------------
/*
d_internal_mongodb_collection
  The driver asserts on a NULL name and sends an empty one as written; the
layer refuses both first.
*/
mongoc_collection_t* d_internal_mongodb_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name
)
{
    const char* const database = d_internal_mongodb_database(_db, _database);

    // a database, and a collection's name
    if (!database)
    {
        return NULL;
    }

    if ( (!_name) ||
         (!*_name) )
    {
        (void)d_internal_mongodb_answer(
                  _db,
                  (_name) ? D_MONGODB_ERROR_INVALID_NAMESPACE
                          : D_DB_INVALID_ARGUMENT,
                  (_name) ? "a collection's name cannot be empty"
                          : "no collection was named");

        return NULL;
    }

    return mongoc_client_get_collection(_db->client, database, _name);
}

// 3.9    Concerns
//------------------------------------------------------------------------------
/**
 * @brief Reads an integer from a number of any of BSON's three kinds, a
 *        double only where it has no fraction.
 *
 * @param[in]  _field  the field.
 * @param[out] _value  receives the integer.
 * @return `true`, or `false` for a field that is not such a number.
 */
static
bool d_internal_mongodb_integer(
    const bson_iter_t* _field,
    int64_t*           _value
)
{
    // int32 and int64, as they are
    if ( (BSON_ITER_HOLDS_INT32(_field)) ||
         (BSON_ITER_HOLDS_INT64(_field)) )
    {
        *_value = bson_iter_as_int64(_field);

        return true;
    }

    const double real = (BSON_ITER_HOLDS_DOUBLE(_field))
                            ? bson_iter_double(_field)
                            : 0.5;

    // a double, where it is an integer an int64 holds
    if ( (real >= -9.2e18)  &&
         (real <= 9.2e18)   &&
         (real == (double)(int64_t)real) )
    {
        *_value = (int64_t)real;

        return true;
    }

    return false;
}

/**
 * @brief Records a concern's refusal: a field of no such name, or a value
 *        it cannot take.
 *
 * @param[in,out] _db     the handle.
 * @param[in]     _what   the concern, for the message.
 * @param[in]     _field  the field.
 * @return D_DB_INVALID_ARGUMENT.
 */
static
int d_internal_mongodb_refuse_field(
    struct dj_mongodb* _db,
    const char*        _what,
    const char*        _field
)
{
    return d_internal_mongodb_answer(_db,
                                     D_DB_INVALID_ARGUMENT,
                                     "%s cannot take its field \"%s\" as "
                                     "given",
                                     _what,
                                     _field);
}

// dj_internal_mongodb_wc_fields
//   struct: a write concern's fields, as read before they are applied.
struct dj_internal_mongodb_wc_fields
{
    int64_t     w;        // a count of members; -1 for none
    const char* tag;      // "majority" or a tag set's name; NULL for none
    int         journal;  // j: 0, 1, or -1 for none
    int64_t     timeout;  // wtimeout in milliseconds; -1 for none
};

/**
 * @brief Reads one field of a write concern.
 *
 * @param[in]     _field   the field.
 * @param[in,out] _fields  what has been read.
 * @return `true`, or `false` for a field of no such name or a value it
 *         cannot take.
 */
static
bool d_internal_mongodb_wc_field(
    const bson_iter_t*                    _field,
    struct dj_internal_mongodb_wc_fields* _fields
)
{
    const char* const key = bson_iter_key(_field);

    // w: text, or a count above 0
    if (strcmp(key, "w") == 0)
    {
        if (BSON_ITER_HOLDS_UTF8(_field))
        {
            _fields->tag = bson_iter_utf8(_field, NULL);

            return true;
        }

        return ( (d_internal_mongodb_integer(_field, &_fields->w)) &&
                 (_fields->w > 0)                                  &&
                 (_fields->w <= INT32_MAX) );
    }

    // j: a boolean
    if (strcmp(key, "j") == 0)
    {
        if (!BSON_ITER_HOLDS_BOOL(_field))
        {
            return false;
        }

        _fields->journal = (bson_iter_bool(_field)) ? 1 : 0;

        return true;
    }

    // the timeout, by either name
    if ( (strcmp(key, "wtimeout") == 0) ||
         (strcmp(key, "wtimeoutMS") == 0) )
    {
        return ( (d_internal_mongodb_integer(_field, &_fields->timeout)) &&
                 (_fields->timeout >= 0) );
    }

    return false;
}

/*
d_internal_mongodb_write_concern
  The fields are read first and applied after, in the driver's order: w,
which for "majority" resets the timeout, then j, then the timeout. wtimeout
is the server's name; wtimeoutMS, a connection string's, is read too.
*/
int d_internal_mongodb_write_concern(
    struct dj_mongodb*       _db,
    const bson_t*            _document,
    mongoc_write_concern_t** _out
)
{
    struct dj_internal_mongodb_wc_fields fields = { -1, NULL, -1, -1 };
    bson_iter_t                         field;

    *_out = NULL;

    // each field, read and checked
    if (bson_iter_init(&field, _document))
    {
        while (bson_iter_next(&field))
        {
            if (!d_internal_mongodb_wc_field(&field, &fields))
            {
                return d_internal_mongodb_refuse_field(_db,
                                                       "a write concern",
                                                       bson_iter_key(&field));
            }
        }
    }

    // the default, which is no concern at all
    if ( (fields.w < 0)       &&
         (!fields.tag)        &&
         (fields.journal < 0) &&
         (fields.timeout < 0) )
    {
        return 0;
    }

    *_out = mongoc_write_concern_new();

    // w: majority, a tag set's name, or a count
    if ( (fields.tag) &&
         (strcmp(fields.tag, "majority") == 0) )
    {
        mongoc_write_concern_set_wmajority(*_out, 0);
    }
    else if (fields.tag)
    {
        mongoc_write_concern_set_wtag(*_out, fields.tag);
    }
    else if (fields.w > 0)
    {
        mongoc_write_concern_set_w(*_out, (int32_t)fields.w);
    }

    // j, then the timeout
    if (fields.journal >= 0)
    {
        mongoc_write_concern_set_journal(*_out, (fields.journal != 0));
    }

    if (fields.timeout >= 0)
    {
        mongoc_write_concern_set_wtimeout_int64(*_out, fields.timeout);
    }

    return 0;
}

/*
d_internal_mongodb_read_concern
  The level is checked against the server's five, which the driver would
send whatever they were.
*/
int d_internal_mongodb_read_concern(
    struct dj_mongodb*      _db,
    const bson_t*           _document,
    mongoc_read_concern_t** _out
)
{
    static const char* const LEVELS[] =
    {
        "local", "available", "majority", "linearizable", "snapshot"
    };

    bson_iter_t field;
    const char* level = NULL;
    size_t      index = 0u;

    *_out = NULL;

    // level, the one field
    if (bson_iter_init(&field, _document))
    {
        while (bson_iter_next(&field))
        {
            if ( (strcmp(bson_iter_key(&field), "level") != 0) ||
                 (!BSON_ITER_HOLDS_UTF8(&field)) )
            {
                return d_internal_mongodb_refuse_field(_db,
                                                       "a read concern",
                                                       bson_iter_key(&field));
            }

            level = bson_iter_utf8(&field, NULL);
        }
    }

    // none: the default
    if (!level)
    {
        return 0;
    }

    // one of the five
    for (index = 0u; index < (sizeof(LEVELS) / sizeof(LEVELS[0])); ++index)
    {
        if (strcmp(level, LEVELS[index]) == 0)
        {
            *_out = mongoc_read_concern_new();
            (void)mongoc_read_concern_set_level(*_out, level);

            return 0;
        }
    }

    return d_internal_mongodb_answer(_db,
                                     D_DB_INVALID_ARGUMENT,
                                     "a read concern has no level \"%s\"",
                                     level);
}

/**
 * @brief The driver's read mode of a name.
 *
 * @param[in]  _name  the name, as a command writes it.
 * @param[out] _mode  receives the mode.
 * @return `true`, or `false` for a name of no mode.
 */
static
bool d_internal_mongodb_read_mode(
    const char*         _name,
    mongoc_read_mode_t* _mode
)
{
    static const char* const NAMES[] =
    {
        "primary", "primaryPreferred", "secondary", "secondaryPreferred",
        "nearest"
    };
    static const mongoc_read_mode_t MODES[] =
    {
        MONGOC_READ_PRIMARY, MONGOC_READ_PRIMARY_PREFERRED,
        MONGOC_READ_SECONDARY, MONGOC_READ_SECONDARY_PREFERRED,
        MONGOC_READ_NEAREST
    };

    size_t index = 0u;

    // the name's mode
    for (index = 0u; index < (sizeof(NAMES) / sizeof(NAMES[0])); ++index)
    {
        if (strcmp(_name, NAMES[index]) == 0)
        {
            *_mode = MODES[index];

            return true;
        }
    }

    return false;
}

/**
 * @brief Reads one field of a read preference.
 *
 * @param[in]     _field  the field: mode, tags or maxStalenessSeconds.
 * @param[in,out] _prefs  receives it.
 * @return `true`, or `false` for a field of no such name, or a value it
 *         cannot take.
 */
static
bool d_internal_mongodb_read_pref(
    const bson_iter_t*                _field,
    struct dj_internal_mongodb_prefs* _prefs
)
{
    const char* const key    = bson_iter_key(_field);
    uint32_t          length = 0u;
    const uint8_t*    data   = NULL;

    // a mode of the five, by name
    if (strcmp(key, "mode") == 0)
    {
        _prefs->has_mode = ( (BSON_ITER_HOLDS_UTF8(_field)) &&
                             (d_internal_mongodb_read_mode(
                                  bson_iter_utf8(_field, NULL),
                                  &_prefs->mode)) );

        return _prefs->has_mode;
    }

    // tags: an array of documents, read in place
    if ( (strcmp(key, "tags") == 0) &&
         (BSON_ITER_HOLDS_ARRAY(_field)) )
    {
        bson_iter_array(_field, &length, &data);
        _prefs->has_tags = bson_init_static(&_prefs->tags, data, length);

        return _prefs->has_tags;
    }

    // a staleness of -1, none, or of 90 seconds at least
    return ( (strcmp(key, "maxStalenessSeconds") == 0)              &&
             (d_internal_mongodb_integer(_field, &_prefs->staleness)) &&
             ( (_prefs->staleness == -1) ||
               (_prefs->staleness >= 90) ) );
}

/*
d_internal_mongodb_read_prefs
  maxStalenessSeconds of -1 is the driver's "none", and is taken as the
absence it means; any other under 90 is refused, as the server would.
*/
int d_internal_mongodb_read_prefs(
    struct dj_mongodb*    _db,
    const bson_t*         _document,
    mongoc_read_prefs_t** _out
)
{
    struct dj_internal_mongodb_prefs prefs;
    bson_iter_t                     field;

    memset(&prefs, 0, sizeof(prefs));
    prefs.mode      = MONGOC_READ_PRIMARY;
    prefs.staleness = -1;
    *_out           = NULL;

    // each field, read and checked
    if (bson_iter_init(&field, _document))
    {
        while (bson_iter_next(&field))
        {
            if (!d_internal_mongodb_read_pref(&field, &prefs))
            {
                return d_internal_mongodb_refuse_field(
                           _db,
                           "a read preference",
                           bson_iter_key(&field));
            }
        }
    }

    // a mode, and nothing the primary cannot take
    if ( (!prefs.has_mode) ||
         ( (prefs.mode == MONGOC_READ_PRIMARY) &&
           ( (prefs.has_tags) ||
             (prefs.staleness >= 0) ) ) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         (prefs.has_mode)
                                             ? "the primary mode takes no "
                                               "tags and no staleness"
                                             : "a read preference needs its "
                                               "mode");
    }

    *_out = mongoc_read_prefs_new(prefs.mode);

    // tags and staleness, where given
    if (prefs.has_tags)
    {
        mongoc_read_prefs_set_tags(*_out, &prefs.tags);
    }

    if (prefs.staleness >= 0)
    {
        mongoc_read_prefs_set_max_staleness_seconds(*_out, prefs.staleness);
    }

    return 0;
}

/*
d_internal_mongodb_subdocument
  bson_init_static reads the bytes where they are.
*/
bool d_internal_mongodb_subdocument(
    const bson_t* _document,
    const char*   _field,
    bson_t*       _out
)
{
    bson_iter_t field;

    return ( (bson_iter_init_find(&field, _document, _field)) &&
             (d_internal_mongodb_field_document(&field, _out)) );
}

/*
d_internal_mongodb_field_document
  bson_init_static reads the bytes where they are.
*/
bool d_internal_mongodb_field_document(
    const bson_iter_t* _field,
    bson_t*            _out
)
{
    uint32_t       length = 0u;
    const uint8_t* data   = NULL;

    // a document, read where it is
    if (!BSON_ITER_HOLDS_DOCUMENT(_field))
    {
        return false;
    }

    bson_iter_document(_field, &length, &data);

    return bson_init_static(_out, data, length);
}

// 3.10   Cursors
//------------------------------------------------------------------------------
/*
d_internal_mongodb_open_find
  The driver's cursor copies the namespace, the preference and the concern
it needs, so the collection goes before the cursor is read.
*/
int d_internal_mongodb_open_find(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    bool               _one,
    mongoc_cursor_t**  _cursor
)
{
    mongoc_collection_t* const collection =
        d_internal_mongodb_collection(_db, _database, _collection);

    *_cursor = NULL;

    // a collection to read
    if (!collection)
    {
        return _db->error;
    }

    bson_t filter;
    bson_t given;
    bson_t opts;
    int    result = d_internal_mongodb_read_document(_db,
                                                     _filter,
                                                     false,
                                                     &filter);

    bson_init(&given);
    bson_init(&opts);
    result = (result == 0)
                 ? d_internal_mongodb_options(_db,
                                              _options,
                                              D_INTERNAL_MONGODB_READ |
                                              D_INTERNAL_MONGODB_SESSION,
                                              &given)
                 : result;

    // one document: its limit, and one batch
    if (_one)
    {
        bson_copy_to_excluding_noinit(&given,
                                      &opts,
                                      "limit",
                                      "singleBatch",
                                      NULL);
        BSON_APPEND_INT64(&opts, "limit", 1);
        BSON_APPEND_BOOL(&opts, "singleBatch", true);
    }
    else
    {
        bson_concat(&opts, &given);
    }

    *_cursor = (result == 0)
                   ? mongoc_collection_find_with_opts(collection,
                                                      &filter,
                                                      &opts,
                                                      NULL)
                   : NULL;

    bson_destroy(&opts);
    bson_destroy(&given);
    bson_destroy(&filter);
    mongoc_collection_destroy(collection);

    return result;
}

/*
d_internal_mongodb_open_aggregate
  A database's aggregation is {aggregate: 1}, the driver's
mongoc_database_aggregate.
*/
int d_internal_mongodb_open_aggregate(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _pipeline,
    const char*        _options,
    mongoc_cursor_t**  _cursor
)
{
    const char* const          name       = d_internal_mongodb_database(
                                                _db,
                                                _database);
    mongoc_collection_t* const collection =
        ( (name) &&
          (_collection) )
            ? d_internal_mongodb_collection(_db, name, _collection)
            : NULL;

    *_cursor = NULL;

    // a database, and a collection where one is named
    if ( (!name) ||
         ( (_collection) &&
           (!collection) ) )
    {
        return _db->error;
    }

    bson_t pipeline;
    bson_t opts;
    int    result = d_internal_mongodb_read_array(_db,
                                                  _pipeline,
                                                  true,
                                                  &pipeline);

    bson_init(&opts);
    result = (result == 0)
                 ? d_internal_mongodb_options(_db,
                                              _options,
                                              D_INTERNAL_MONGODB_READ |
                                              D_INTERNAL_MONGODB_SESSION,
                                              &opts)
                 : result;

    // the collection's pipeline, or the database's
    if ( (result == 0) &&
         (collection) )
    {
        *_cursor = mongoc_collection_aggregate(collection,
                                               MONGOC_QUERY_NONE,
                                               &pipeline,
                                               &opts,
                                               NULL);
    }
    else if (result == 0)
    {
        mongoc_database_t* const database =
            mongoc_client_get_database(_db->client, name);

        *_cursor = mongoc_database_aggregate(database, &pipeline, &opts, NULL);
        mongoc_database_destroy(database);
    }

    bson_destroy(&opts);
    bson_destroy(&pipeline);
    mongoc_collection_destroy(collection);

    return result;
}

/**
 * @brief Runs a command on a server its cursor can return to.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _name     the database.
 * @param[in]     _command  the command.
 * @param[out]    _opts     initialized, and receives what the cursor
 *                          needs: the server's id, or the session.
 * @param[out]    _reply    initialized, and receives the reply.
 * @return 0, or the failure, recorded.
 */
static
int d_internal_mongodb_run_pinned(
    struct dj_mongodb* _db,
    const char*        _name,
    const bson_t*      _command,
    bson_t*            _opts,
    bson_t*            _reply
)
{
    bson_error_t error;

    bson_init(_opts);
    bson_init(_reply);

    // the session, in a transaction; the server's id, out of one
    if ( (_db->session) &&
         (mongoc_client_session_in_transaction(_db->session)) )
    {
        const int result = d_internal_mongodb_session(_db, _opts);

        if (result != 0)
        {
            return result;
        }
    }
    else
    {
        mongoc_server_description_t* const server =
            mongoc_client_select_server(_db->client, true, NULL, &error);

        if (!server)
        {
            return d_internal_mongodb_fail(_db, &error, NULL);
        }

        BSON_APPEND_INT32(_opts,
                          "serverId",
                          (int32_t)mongoc_server_description_id(server));
        mongoc_server_description_destroy(server);
    }

    return (mongoc_client_command_with_opts(_db->client,
                                            _name,
                                            _command,
                                            NULL,
                                            _opts,
                                            _reply,
                                            &error))
               ? d_internal_mongodb_reply_failure(_db, _reply)
               : d_internal_mongodb_fail(_db, &error, _reply);
}

/*
d_internal_mongodb_run_cursor
  A reply holds a cursor where its "cursor" is a document with a
firstBatch. The driver's cursor takes the reply and destroys it, and the
reply is initialized again, empty, so that the caller destroys it either
way.
*/
int d_internal_mongodb_run_cursor(
    struct dj_mongodb* _db,
    const char*        _name,
    const bson_t*      _command,
    mongoc_cursor_t**  _cursor,
    bson_t*            _reply
)
{
    bson_t      opts;
    bson_iter_t field;
    bson_iter_t batch;
    const int   result = d_internal_mongodb_run_pinned(_db,
                                                       _name,
                                                       _command,
                                                       &opts,
                                                       _reply);

    *_cursor = NULL;

    // a cursor, which takes the reply
    if ( (result == 0)                                   &&
         (bson_iter_init_find(&field, _reply, "cursor")) &&
         (BSON_ITER_HOLDS_DOCUMENT(&field))              &&
         (bson_iter_recurse(&field, &batch))             &&
         (bson_iter_find(&batch, "firstBatch")) )
    {
        *_cursor = mongoc_cursor_new_from_command_reply_with_opts(_db->client,
                                                                  _reply,
                                                                  &opts);
        bson_init(_reply);
    }

    bson_destroy(&opts);

    return result;
}

/*
d_internal_mongodb_end_cursors
  Each cursor is unlinked as it ends; one whose documents were all read is
done, and reads as done does, rather than ended.
*/
void d_internal_mongodb_end_cursors(
    struct dj_mongodb* _db
)
{
    struct dj_mongodb_cursor* cursor = _db->cursors;

    // each of the transaction's cursors
    while (cursor)
    {
        struct dj_mongodb_cursor* const next = cursor->next;

        mongoc_cursor_destroy(cursor->cursor);
        cursor->cursor   = NULL;
        cursor->ended    = !cursor->done;
        cursor->done     = true;
        cursor->previous = NULL;
        cursor->next     = NULL;
        cursor           = next;
    }

    _db->cursors = NULL;

    return;
}

#else

// an ISO C translation unit must declare something; built as stubs, the
// layer has no plumbing
typedef int dj_internal_mongodb_no_plumbing;

#endif  // D_INTERNAL_MONGODB == 1
