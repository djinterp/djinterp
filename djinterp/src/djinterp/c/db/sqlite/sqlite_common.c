/*******************************************************************************
* djinterp [c]                                                   sqlite_common.c
*
* Implementation of sqlite_common.h.
*   Feature answers come from the library loaded at run time: its version for
* SQL syntax, sqlite3_compileoption_used for what a build may leave out. A
* shared library can be newer or older than the header the layer was compiled
* against, and only the loaded library knows what it accepts.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_common.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../inc/djinterp/config/c/db/sqlite/cfg_sqlite_common.h"  // D_INTERNAL_SQLITE
#include "./sqlite_internal.h"  // sqlite3.h, the floor, the plumbing


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_internal_sqlite_feature_need
//   struct: what one feature needs of the library loaded.
struct d_internal_sqlite_feature_need
{
    enum d_sqlite_feature feature;  // the feature
    int                   since;    // the release that introduced it
    const char*           option;   // a build option it needs too, or NULL
};

// d_internal_sqlite_feature_needs
//   table: every feature, the release that introduced it, numbered as
// SQLITE_VERSION_NUMBER numbers releases (3008000 is 3.8.0), and, for those a
// build can leave out, the option that puts it in. JSON's row is its JSON1
// years; from 3.38.0 it is built in, which d_sqlite_supports checks first.
static const struct d_internal_sqlite_feature_need
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
    { D_SQLITE_FEATURE_RTREE,             3007017, "ENABLE_RTREE"          }
};

//==============================================================================
// 2.  LIBRARY
//==============================================================================

// 2.1    Versions
//------------------------------------------------------------------------------
/*
d_sqlite_library_version
  The loaded library's own string, which outlives every connection.
*/
const char*
d_sqlite_library_version(
    void
)
{
    return sqlite3_libversion();
}

/*
d_sqlite_library_version_number
  The loaded library's own number; compare it with
d_sqlite_header_version_number to see whether a shared library differs from
the header the layer was built against.
*/
int
d_sqlite_library_version_number(
    void
)
{
    return sqlite3_libversion_number();
}

/*
d_sqlite_header_version_number
  The version env_sqlite.h detected when this file was compiled: the header's
SQLITE_VERSION_NUMBER, unless the build set D_ENV_SQLITE_DETECTED_VERSION by
hand to target an older library.
*/
int
d_sqlite_header_version_number(
    void
)
{
    return D_ENV_SQLITE_VERSION_ID;
}

/*
d_sqlite_library_at_least
  The loaded version is taken apart and compared a part at a time, so no
argument, however large, can overflow an encoded number. A negative part names
no release and answers `false`.
*/
bool
d_sqlite_library_at_least(
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
    enum d_sqlite_feature _feature
)
{
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
        const struct d_internal_sqlite_feature_need* const need =
            &d_internal_sqlite_feature_needs[i];

        // the feature's row decides
        if (need->feature == _feature)
        {
            return ( (d_internal_sqlite_runtime_at_least(need->since)) &&
                     ( (!need->option) ||
                       (sqlite3_compileoption_used(need->option) != 0) ) );
        }
    }

    return false;
}

// 2.3    Result messages
//------------------------------------------------------------------------------
/*
d_sqlite_result_message
  The negative results are db_common.h's, which SQLite cannot describe and
d_db_status_message does; everything else is SQLite's, and sqlite3_errstr
knows its extended codes too.
*/
const char*
d_sqlite_result_message(
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
const char*
d_sqlite_error_message(
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
const char*
d_sqlite_library_version(
    void
)
{
    return "";
}

/*
d_sqlite_library_version_number
  Without SQLite the version is 0, below every release.
*/
int
d_sqlite_library_version_number(
    void
)
{
    return 0;
}

/*
d_sqlite_header_version_number
  Without SQLite the version is 0, below every release.
*/
int
d_sqlite_header_version_number(
    void
)
{
    return 0;
}

/*
d_sqlite_library_at_least
  Without SQLite, false.
*/
bool
d_sqlite_library_at_least(
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
    enum d_sqlite_feature _feature
)
{
    (void)_feature;

    return false;
}

/*
d_sqlite_result_message
  Without SQLite only the layer's own results occur; this build's
D_DB_UNSUPPORTED says why, and any other value is unknown.
*/
const char*
d_sqlite_result_message(
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
const char*
d_sqlite_error_message(
    struct sqlite3* _db
)
{
    (void)_db;

    return NULL;
}

#endif  // D_INTERNAL_SQLITE
