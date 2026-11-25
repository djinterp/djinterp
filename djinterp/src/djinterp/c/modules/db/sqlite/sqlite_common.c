/*******************************************************************************
* djinterp [c][db]                                               sqlite_common.c
*
* Implementation of sqlite_common.h.
*   Feature answers come from the library loaded at run time: its version for
* SQL syntax, sqlite3_compileoption_used for what a build may leave out. A
* shared library can be newer or older than the header the layer was compiled
* against, and only the loaded library knows what it accepts.
*
*
* path:      /src/djinterp/c/modules/db/sqlite/sqlite_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.21
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "../../../../../../inc/djinterp/c/modules/db/sqlite/sqlite_common.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/config/c/modules/db/sqlite/cfg_sqlite_common.h"  // D_INTERNAL_SQLITE
#include "./sqlite_internal.h"  // sqlite3.h, the floor, the plumbing


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// dj_internal_sqlite_feature_need
//   struct: what one feature needs of the library loaded.
struct dj_internal_sqlite_feature_need
{
    enum dj_sqlite_feature feature;  // the feature
    int                   since;    // the release that introduced it
    const char*           option;   // a build option it needs too, or NULL
};

// dj_internal_sqlite_feature_omit
//   struct: a build option that takes a feature out.
struct dj_internal_sqlite_feature_omit
{
    enum dj_sqlite_feature feature;  // the feature
    const char*           option;   // the option
};

// d_internal_sqlite_feature_needs
//   table: every feature, the release that introduced it, numbered as
// SQLITE_VERSION_NUMBER numbers releases (3008000 is 3.8.0), and, for those a
// build can leave out, the option that puts it in. JSON's row is its JSON1
// years; from 3.38.0 it is built in, which d_sqlite_supports checks first.
// Encryption's row is a codec's option; SQLCipher, which may not report one,
// is asked.
static const struct dj_internal_sqlite_feature_need
d_internal_sqlite_feature_needs[] =
{
    { D_SQLITE_FEATURE_PARTIAL_INDEX,     3008000, NULL                    },
    { D_SQLITE_FEATURE_WITHOUT_ROWID,     3008002, NULL                    },
    { D_SQLITE_FEATURE_CTE,               3008003, NULL                    },
    { D_SQLITE_FEATURE_EXPRESSION_INDEX,  3009000, NULL                    },
    { D_SQLITE_FEATURE_ROW_VALUES,        3015000, NULL                    },
    { D_SQLITE_FEATURE_VACUUM_SCHEMA,     3015000, NULL                    },
    { D_SQLITE_FEATURE_OPTIMIZE,          3018000, NULL                    },
    { D_SQLITE_FEATURE_UPSERT,            3024000, NULL                    },
    { D_SQLITE_FEATURE_WINDOW_FUNCTIONS,  3025000, NULL                    },
    { D_SQLITE_FEATURE_RENAME_COLUMN,     3025000, NULL                    },
    { D_SQLITE_FEATURE_VACUUM_INTO,       3027000, NULL                    },
    { D_SQLITE_FEATURE_GENERATED_COLUMNS, 3031000, NULL                    },
    { D_SQLITE_FEATURE_DROP_COLUMN,       3035000, NULL                    },
    { D_SQLITE_FEATURE_RETURNING,         3035000, NULL                    },
    { D_SQLITE_FEATURE_STRICT_TABLES,     3037000, NULL                    },
    { D_SQLITE_FEATURE_JSON,              3009000, "ENABLE_JSON1"          },
    { D_SQLITE_FEATURE_MATH_FUNCTIONS,    3035000, "ENABLE_MATH_FUNCTIONS" },
    { D_SQLITE_FEATURE_FTS5,              3009000, "ENABLE_FTS5"           },
    { D_SQLITE_FEATURE_RTREE,             3007017, "ENABLE_RTREE"          },
    { D_SQLITE_FEATURE_WAL,               3007000, NULL                    },
    { D_SQLITE_FEATURE_ENCRYPTION,        3007017, "HAS_CODEC"             }
};

// d_internal_sqlite_feature_omits
//   table: the features a build can take out, and the option that does.
static const struct dj_internal_sqlite_feature_omit
d_internal_sqlite_feature_omits[] =
{
    { D_SQLITE_FEATURE_WAL, "OMIT_WAL" }
};

/*
d_internal_sqlite_omitted
  Whether the loaded library was built without a feature, by an option of
d_internal_sqlite_feature_omits.
*/
static bool
d_internal_sqlite_omitted(
    enum dj_sqlite_feature _feature
)
{
    const size_t count = sizeof(d_internal_sqlite_feature_omits) /
                         sizeof(d_internal_sqlite_feature_omits[0]);

    for (size_t i = 0; i < count; ++i)
    {
        // the feature's option decides
        if (d_internal_sqlite_feature_omits[i].feature == _feature)
        {
            return (sqlite3_compileoption_used(
                        d_internal_sqlite_feature_omits[i].option) != 0);
        }
    }

    return false;
}

/*
d_internal_sqlite_can_serialize
  The build must call it (D_INTERNAL_SQLITE_SERIALIZE), and the library loaded
have it: from 3.36.0 unless built without it, and from 3.23.0 where built with
it.
*/
static
bool d_internal_sqlite_can_serialize(void)
{
#if (D_INTERNAL_SQLITE_SERIALIZE == 1)
    return ( ( (d_internal_sqlite_runtime_at_least(3036000)) &&
               (sqlite3_compileoption_used("OMIT_DESERIALIZE") == 0) ) ||
             ( (d_internal_sqlite_runtime_at_least(3023000)) &&
               (sqlite3_compileoption_used("ENABLE_DESERIALIZE") != 0) ) );
#else
    return false;
#endif
}

/*
d_internal_sqlite_can_load_extensions
  The build must call it (D_INTERNAL_SQLITE_LOAD_EXTENSION), and the library
loaded not have left it out.
*/
static
bool d_internal_sqlite_can_load_extensions(void)
{
#if (D_INTERNAL_SQLITE_LOAD_EXTENSION == 1)
    return (sqlite3_compileoption_used("OMIT_LOAD_EXTENSION") == 0);
#else
    return false;
#endif
}

/*
d_internal_sqlite_is_sqlcipher
  SQLCipher answers PRAGMA cipher_version with its version, and any other
SQLite with no row, on a private in-memory database (Zetetic's own advice);
a library that cannot open one answers `false`.
*/
static
bool d_internal_sqlite_is_sqlcipher(void)
{
    sqlite3*      db        = NULL;
    sqlite3_stmt* statement = NULL;
    bool          found     = false;

    // a private database, for the question alone
    if (sqlite3_open_v2(":memory:",
                        &db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        NULL) == SQLITE_OK)
    {
        if (sqlite3_prepare_v2(db,
                               "PRAGMA cipher_version",
                               -1,
                               &statement,
                               NULL) == SQLITE_OK)
        {
            found = (sqlite3_step(statement) == SQLITE_ROW);
        }

        sqlite3_finalize(statement);
    }

    sqlite3_close(db);

    return found;
}

//==============================================================================
// 2.  LIBRARY
//==============================================================================

// 2.1    Versions
//------------------------------------------------------------------------------
/*
d_sqlite_library_version
  The loaded library's own string, which outlives every connection.
*/
const char* d_sqlite_library_version(void)
{
    return sqlite3_libversion();
}

/*
d_sqlite_library_version_number
  The loaded library's own number; compare it with
d_sqlite_header_version_number to see whether a shared library differs from
the header the layer was built against.
*/
int d_sqlite_library_version_number(void)
{
    return sqlite3_libversion_number();
}

/*
d_sqlite_header_version_number
  The version env_sqlite.h detected when this file was compiled: the header's
SQLITE_VERSION_NUMBER, unless the build set D_ENV_SQLITE_DETECTED_VERSION by
hand to target an older library.
*/
int d_sqlite_header_version_number(void)
{
    return D_ENV_SQLITE_VERSION_ID;
}

/*
d_sqlite_library_at_least
  The loaded version is taken apart and compared a part at a time, so no
argument, however large, can overflow an encoded number. A negative part names
no release and answers `false`.
*/
bool d_sqlite_library_at_least(
    int _major,
    int _minor,
    int _patch
)
{
    // parameter validation
    if ( (_major < 0) ||
         (_minor < 0) ||
         (_patch < 0) )
    {
        return false;
    }

    const int library = sqlite3_libversion_number();
    const int major   = D_ENV_SQLITE_DECODE_MAJOR(library);
    const int minor   = D_ENV_SQLITE_DECODE_MINOR(library);

    // the first part that differs decides
    if (major != _major)
    {
        return (major > _major);
    }

    if (minor != _minor)
    {
        return (minor > _minor);
    }

    return (D_ENV_SQLITE_DECODE_PATCH(library) >= _patch);
}

// 2.2    Features
//------------------------------------------------------------------------------
/*
d_sqlite_supports
  SQL syntax is a matter of version; JSON, the math functions, FTS5 and R*Tree
also depend on the build, which sqlite3_compileoption_used reports. JSON alone
changed sides: an option until 3.38.0, built in since, and removable only by
OMIT_JSON. An extension loaded at run time is not seen: this reports what is
built in. A value outside the table is no feature, and answers `false`.
*/
bool
d_sqlite_supports(
    enum dj_sqlite_feature _feature
)
{
    // the APIs this build may not call, and the library may not have
    if (_feature == D_SQLITE_FEATURE_SERIALIZE)
    {
        return d_internal_sqlite_can_serialize();
    }

    if (_feature == D_SQLITE_FEATURE_LOAD_EXTENSION)
    {
        return d_internal_sqlite_can_load_extensions();
    }

    // JSON, since it joined the core
    if ( (_feature == D_SQLITE_FEATURE_JSON) &&
         (d_internal_sqlite_runtime_at_least(
             D_ENV_SQLITE_ENCODE_VERSION(3, 38, 0))) )
    {
        return (sqlite3_compileoption_used("OMIT_JSON") == 0);
    }

    const size_t count = sizeof(d_internal_sqlite_feature_needs) /
                         sizeof(d_internal_sqlite_feature_needs[0]);

    for (size_t i = 0; i < count; ++i)
    {
        const struct dj_internal_sqlite_feature_need* const need =
            &d_internal_sqlite_feature_needs[i];

        // the feature's row decides
        if (need->feature == _feature)
        {
            // an encrypting library that names no codec among its options
            if ( (_feature == D_SQLITE_FEATURE_ENCRYPTION) &&
                 (sqlite3_compileoption_used(need->option) == 0) )
            {
                return d_internal_sqlite_is_sqlcipher();
            }

            return ( (d_internal_sqlite_runtime_at_least(need->since)) &&
                     ( (!need->option) ||
                       (sqlite3_compileoption_used(need->option) != 0) ) &&
                     (!d_internal_sqlite_omitted(_feature)) );
        }
    }

    return false;
}

/*
d_sqlite_library_threadsafe
  sqlite3_threadsafe reports the SQLITE_THREADSAFE the library was compiled
with.
*/
int d_sqlite_library_threadsafe(void)
{
    return sqlite3_threadsafe();
}

// 2.3    Result messages
//------------------------------------------------------------------------------
/*
d_sqlite_result_message
  The negative results are db_common.h's, which SQLite cannot describe and
d_db_status_message does; everything else is SQLite's, and sqlite3_errstr
knows its extended codes too.
*/
const char* d_sqlite_result_message(
    int _result
)
{
    // the layer's own results, in db_common.h's words
    if (_result < 0)
    {
        const char* const message = d_db_status_message(_result);

        return (message) ? message : "unknown result";
    }

    return sqlite3_errstr(_result);
}

//==============================================================================
// 3.  CONNECTIONS
//==============================================================================

// 3.1    Errors
//------------------------------------------------------------------------------
/*
d_sqlite_error_message
  sqlite3_errmsg, which reads a NULL connection as one that ran out of
memory; the layer answers NULL for it instead.
*/
const char* d_sqlite_error_message(
    struct sqlite3* _db
)
{
    return (_db) ? sqlite3_errmsg(_db) : NULL;
}

#else

//==============================================================================
// WITHOUT SQLITE
//==============================================================================
// Built as stubs (D_CFG_SQLITE 0), each function reports what a build without
// SQLite can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_sqlite_library_version
  Without SQLite there is no version: the empty string, never NULL.
*/
const char* d_sqlite_library_version(void)
{
    return "";
}

/*
d_sqlite_library_version_number
  Without SQLite the version is 0, below every release.
*/
int d_sqlite_library_version_number(void)
{
    return 0;
}

/*
d_sqlite_header_version_number
  Without SQLite the version is 0, below every release.
*/
int d_sqlite_header_version_number(void)
{
    return 0;
}

/*
d_sqlite_library_at_least
  Without SQLite, false.
*/
bool d_sqlite_library_at_least(
    int _major,
    int _minor,
    int _patch
)
{
    (void)_major;
    (void)_minor;
    (void)_patch;

    return false;
}

/*
d_sqlite_supports
  Without SQLite, false.
*/
bool
d_sqlite_supports(
    enum dj_sqlite_feature _feature
)
{
    (void)_feature;

    return false;
}

/*
d_sqlite_library_threadsafe
  Without SQLite, 0.
*/
int d_sqlite_library_threadsafe(void)
{
    return 0;
}

/*
d_sqlite_result_message
  Without SQLite only the layer's own results occur; this build's
D_DB_UNSUPPORTED says why, and any other value is unknown.
*/
const char* d_sqlite_result_message(
    int _result
)
{
    const char* const message = d_db_status_message(_result);

    // the build, not the library, is what cannot
    if (_result == D_DB_UNSUPPORTED)
    {
        return "this build of the SQLite layer has no SQLite";
    }

    return (message) ? message : "unknown result";
}

/*
d_sqlite_error_message
  Without SQLite there is no connection to have failed: NULL.
*/
const char* d_sqlite_error_message(
    struct sqlite3* _db
)
{
    (void)_db;

    return NULL;
}

#endif  // D_INTERNAL_SQLITE


//==============================================================================
// KINDS, IN EVERY BUILD
//==============================================================================
// Sorting a result needs nothing of SQLite's: its codes are the documented
// numbers, written out here so the layer built without SQLite sorts them too.

// 2.4    Kinds
//------------------------------------------------------------------------------
// g_sqlite_kind, g_sqlite_kind_user
//   state: the function the program set to sort SQLite's results, and what
// it gave with it; none, to use the default. The process's: set before the
// first connection, and read on any thread.
static dj_db_kind_fn g_sqlite_kind      = NULL;
static void*         g_sqlite_kind_user = NULL;

/*
d_sqlite_kind_set
  NULL sets the default again, and drops what came with the old function.
*/
void
d_sqlite_kind_set(
    dj_db_kind_fn _fn,
    void*         _user
)
{
    g_sqlite_kind      = _fn;
    g_sqlite_kind_user = (_fn) ? _user : NULL;

    return;
}

/*
d_sqlite_kind
  Success, SQLITE_ROW, SQLITE_DONE, a notice, a warning and the layer's own
negative results are no failure of SQLite's, so no function sorts them.
*/
enum dj_db_vendor_kind
d_sqlite_kind(
    int32_t _result
)
{
    const int32_t primary = (_result & 0xFF);

    // not a failure: OK, NOTICE, WARNING, ROW, DONE, or the layer's own
    if ( (_result <= 0) ||
         (primary == 27) ||
         (primary == 28) ||
         (primary == 100) ||
         (primary == 101) )
    {
        return D_DB_VENDOR_KIND_NONE;
    }

    return (g_sqlite_kind) ? g_sqlite_kind(_result, NULL, g_sqlite_kind_user)
                           : d_sqlite_kind_default(_result, NULL, NULL);
}

/*
d_sqlite_kind_default
  By the primary result (SQLite's result codes, by number), and by the
extended one where it says more than its primary: SQLITE_ERROR_RETRY and
SQLITE_ERROR_SNAPSHOT are busy, SQLITE_ERROR_MISSING_COLLSEQ not found.
*/
enum dj_db_vendor_kind
d_sqlite_kind_default(
    int32_t     _result,
    const char* _sqlstate,
    void*       _user
)
{
    (void)_sqlstate;
    (void)_user;

    // the extended results that sort apart from their primary
    switch (_result)
    {
        case 257:  // SQLITE_ERROR_MISSING_COLLSEQ
            return D_DB_VENDOR_KIND_NOT_FOUND;
        case 513:  // SQLITE_ERROR_RETRY
        case 769:  // SQLITE_ERROR_SNAPSHOT
            return D_DB_VENDOR_KIND_BUSY;
        default:
            break;
    }

    if (_result <= 0)
    {
        return D_DB_VENDOR_KIND_NONE;
    }

    switch (_result & 0xFF)
    {
        case 1:    // SQLITE_ERROR: a statement SQLite could not prepare
            return D_DB_VENDOR_KIND_SYNTAX;
        case 3:    // SQLITE_PERM
        case 8:    // SQLITE_READONLY
        case 23:   // SQLITE_AUTH: the program's authorizer refused it
            return D_DB_VENDOR_KIND_DENIED;
        case 4:    // SQLITE_ABORT
        case 5:    // SQLITE_BUSY
        case 6:    // SQLITE_LOCKED
        case 9:    // SQLITE_INTERRUPT
        case 15:   // SQLITE_PROTOCOL
        case 17:   // SQLITE_SCHEMA
            return D_DB_VENDOR_KIND_BUSY;
        case 14:   // SQLITE_CANTOPEN
        case 26:   // SQLITE_NOTADB
            return D_DB_VENDOR_KIND_CONNECTION;
        case 18:   // SQLITE_TOOBIG
        case 20:   // SQLITE_MISMATCH
        case 24:   // SQLITE_FORMAT
        case 25:   // SQLITE_RANGE
            return D_DB_VENDOR_KIND_DATA;
        case 19:   // SQLITE_CONSTRAINT
            return D_DB_VENDOR_KIND_CONSTRAINT;
        case 21:   // SQLITE_MISUSE
        case 22:   // SQLITE_NOLFS
            return D_DB_VENDOR_KIND_UNSUPPORTED;
        case 16:   // SQLITE_EMPTY, unused
        case 27:   // SQLITE_NOTICE
        case 28:   // SQLITE_WARNING
        case 100:  // SQLITE_ROW
        case 101:  // SQLITE_DONE
            return D_DB_VENDOR_KIND_NONE;
        default:   // INTERNAL, NOMEM, IOERR, CORRUPT, NOTFOUND, FULL, ...
            return D_DB_VENDOR_KIND_INTERNAL;
    }
}
