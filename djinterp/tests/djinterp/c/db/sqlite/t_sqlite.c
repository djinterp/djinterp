/*******************************************************************************
* djinterp [c]                                                        t_sqlite.c
*
*   Conformance harness for djinterp's SQLite convenience layer: every
* function in c/db/sqlite/sqlite_*.h and c/db/db_common.h, its success paths,
* its documented failures and its argument checks, against whatever SQLite it
* is linked with. Where the answer depends on the library -- a feature 3.25.0
* added, a refusal before 3.15.0 -- the check asks the library's version and
* expects the matching answer, so one harness passes on every release from
* the floor, 3.7.17, onward.
*   Each d_sqlite_supports answer is also checked against the library itself,
* by running SQL that needs the feature: a version table that drifts from the
* truth fails here, not in a program.
*   The tests follow the headers, section by section, and each starts from a
* connection of its own, so a failure belongs to the function it names. Files
* go in the directory given as the first argument, "." by default, and are
* removed before and after.
*
*   Build (from the repo root), at any language level from C99, against an
* SQLite amalgamation or a system library -- or let ci/check_db_sqlite.sh,
* which runs it across compilers, standards and SQLite releases:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc -I<sqlite3.h's directory>       \
*        src/djinterp/c/db/db_*.c src/djinterp/c/db/sqlite/sqlite_[a-z]*.c    \
*        tests/djinterp/c/db/sqlite/t_sqlite.c <sqlite3.c, or -lsqlite3>      \
*        -o t_sqlite
*
*
* path:      /tests/djinterp/c/db/sqlite/t_sqlite.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.23
*******************************************************************************/
// std
#include <limits.h>  // INT_MAX
#include <math.h>    // NAN
#include <stdio.h>   // FILE, fopen, fputs, printf, remove, snprintf
#include <stdlib.h>  // getenv
#include <string.h>  // memcmp, memset, strchr, strcmp, strlen, strstr
#if !defined(_WIN32)
    #include <unistd.h>  // symlink
#endif  // !defined(_WIN32)
// djinterp
#include "../../../../../inc/djinterp/c/db/sqlite/sqlite.h"  // unit under test
// sqlite
#include <sqlite3.h>  // SQLITE_* result codes, sqlite3_exec for setup
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, int64_t


//==============================================================================
// HARNESS
//==============================================================================

// g_checks, g_failures
//   counters: the checks run, and those that failed.
static int g_checks   = 0;
static int g_failures = 0;

// g_directory
//   setting: where the tests make their files.
static const char* g_directory = ".";

/*
d_tests_sqlite_expect
  Counts one check, and prints it when it fails.
*/
static void
d_tests_sqlite_expect(
    bool        _ok,
    const char* _what,
    int         _line
)
{
    ++g_checks;

    // report a failure with what was being checked, and where
    if (!_ok)
    {
        ++g_failures;
        printf("  FAIL line %d: %s\n", _line, _what);
    }

    return;
}

// D_TESTS_SQLITE_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_SQLITE_CHECK(_cond)                                           \
    d_tests_sqlite_expect((_cond), #_cond, __LINE__)

//==============================================================================
// FIXTURES
//==============================================================================

// d_tests_sqlite_feature_probe
//   struct: a feature, and SQL that runs only where the library has it.
struct d_tests_sqlite_feature_probe
{
    enum d_sqlite_feature feature;  // the feature
    const char*           sql;      // SQL that needs it
};

// d_tests_sqlite_probes
//   table: SQL for every feature that SQL can show; the others are checked
// against their releases.
static const struct d_tests_sqlite_feature_probe
d_tests_sqlite_probes[] =
{
    { D_SQLITE_FEATURE_PARTIAL_INDEX,
      "CREATE TABLE p(a); CREATE INDEX pi ON p(a) WHERE a > 0" },
    { D_SQLITE_FEATURE_WITHOUT_ROWID,
      "CREATE TABLE w(a PRIMARY KEY) WITHOUT ROWID" },
    { D_SQLITE_FEATURE_CTE,
      "WITH x(a) AS (SELECT 1) SELECT a FROM x" },
    { D_SQLITE_FEATURE_EXPRESSION_INDEX,
      "CREATE TABLE e(a); CREATE INDEX ei ON e(lower(a))" },
    { D_SQLITE_FEATURE_ROW_VALUES,
      "SELECT (1, 2) = (1, 2)" },
    { D_SQLITE_FEATURE_UPSERT,
      "CREATE TABLE u(a PRIMARY KEY, b); "
      "INSERT INTO u VALUES(1, 1) ON CONFLICT(a) DO UPDATE SET b = 2" },
    { D_SQLITE_FEATURE_WINDOW_FUNCTIONS,
      "SELECT row_number() OVER () FROM (SELECT 1)" },
    { D_SQLITE_FEATURE_RENAME_COLUMN,
      "CREATE TABLE r(a); ALTER TABLE r RENAME COLUMN a TO b" },
    { D_SQLITE_FEATURE_GENERATED_COLUMNS,
      "CREATE TABLE g(a, b AS (a + 1))" },
    { D_SQLITE_FEATURE_DROP_COLUMN,
      "CREATE TABLE d(a, b); ALTER TABLE d DROP COLUMN b" },
    { D_SQLITE_FEATURE_RETURNING,
      "CREATE TABLE t(a); INSERT INTO t VALUES(1) RETURNING a" },
    { D_SQLITE_FEATURE_STRICT_TABLES,
      "CREATE TABLE s(a INTEGER) STRICT" },
    { D_SQLITE_FEATURE_JSON,
      "SELECT json('{}')" },
    { D_SQLITE_FEATURE_MATH_FUNCTIONS,
      "SELECT sin(0)" },
    { D_SQLITE_FEATURE_FTS5,
      "CREATE VIRTUAL TABLE f USING fts5(a)" },
    { D_SQLITE_FEATURE_RTREE,
      "CREATE VIRTUAL TABLE rt USING rtree(id, x0, x1)" }
};

// d_tests_sqlite_every_type
//   table: a column of every d_db_type, between them carrying every column
// flag, DEFAULTs, a size and a scale; the last is the only one with type_sql,
// which a STRICT table would refuse, so the STRICT test leaves it out.
static const struct d_db_column
d_tests_sqlite_every_type[] =
{
    { "c_id",   D_DB_TYPE_BIG_INTEGER,
      D_DB_COLUMN_PRIMARY_KEY | D_DB_COLUMN_AUTO_INCREMENT,
      NULL, 0u, 0u, NULL, 0u },
    { "c_none", D_DB_TYPE_NONE,           0u, NULL, 0u, 0u, NULL, 0u },
    { "c_bool", D_DB_TYPE_BOOLEAN,        D_DB_COLUMN_NOT_NULL,
      "0", 0u, 0u, NULL, 0u },
    { "c_int",  D_DB_TYPE_INTEGER,        D_DB_COLUMN_UNIQUE,
      NULL, 0u, 0u, NULL, 0u },
    { "c_real", D_DB_TYPE_FLOATING_POINT, 0u, "1.5", 0u, 0u, NULL, 0u },
    { "c_dec",  D_DB_TYPE_DECIMAL,        0u, NULL, 10u, 2u, NULL, 0u },
    { "c_str",  D_DB_TYPE_STRING,         0u, "'it''s'", 64u, 0u, NULL, 0u },
    { "c_bin",  D_DB_TYPE_BINARY,         0u, NULL, 0u, 0u, NULL, 0u },
    { "c_date", D_DB_TYPE_DATE,           0u, NULL, 0u, 0u, NULL, 0u },
    { "c_ts",   D_DB_TYPE_TIMESTAMP,      0u,
      "CURRENT_TIMESTAMP", 0u, 0u, NULL, 0u },
    { "c_json", D_DB_TYPE_JSON,           0u, NULL, 0u, 0u, NULL, 0u },
    { "c_own",  D_DB_TYPE_NONE,           0u, NULL, 0u, 0u,
      "VARCHAR(20)", 0u }
};

// d_tests_sqlite_composite
//   table: three columns, the first two a composite primary key.
static const struct d_db_column
d_tests_sqlite_composite[] =
{
    { "a", D_DB_TYPE_STRING,  D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u, NULL, 0u },
    { "b", D_DB_TYPE_INTEGER, D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u, NULL, 0u },
    { "c", D_DB_TYPE_JSON,    0u,                      NULL, 0u, 0u, NULL, 0u }
};

// d_tests_sqlite_ordered
//   table: a composite key whose places order it b, a: otherwise than the
// table.
static const struct d_db_column
d_tests_sqlite_ordered[] =
{
    { "a", D_DB_TYPE_INTEGER, D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u, NULL, 2u },
    { "b", D_DB_TYPE_INTEGER, D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u, NULL, 1u },
    { "c", D_DB_TYPE_INTEGER, 0u,                      NULL, 0u, 0u, NULL, 0u }
};

/*
d_tests_sqlite_at_least
  The library's version, the one the layer's run-time checks read.
*/
static bool
d_tests_sqlite_at_least(
    int _major,
    int _minor,
    int _patch
)
{
    return (sqlite3_libversion_number() >=
            ((_major * 1000000) + (_minor * 1000) + _patch));
}

/*
d_tests_sqlite_path
  A file name inside the test directory.
*/
static const char*
d_tests_sqlite_path(
    char*       _buffer,
    size_t      _size,
    const char* _name
)
{
    (void)snprintf(_buffer, _size, "%s/%s", g_directory, _name);

    return _buffer;
}

/*
d_tests_sqlite_exists
  Whether a file can be opened for reading.
*/
static bool
d_tests_sqlite_exists(
    const char* _path
)
{
    FILE* const file = fopen(_path, "rb");

    // closing what was opened
    if (file)
    {
        (void)fclose(file);
    }

    return (file != NULL);
}

/*
d_tests_sqlite_write_file
  Replaces a file's contents with text.
*/
static void
d_tests_sqlite_write_file(
    const char* _path,
    const char* _text
)
{
    FILE* const file = fopen(_path, "wb");

    // write only to a file that opened
    if (file)
    {
        (void)fputs(_text, file);
        (void)fclose(file);
    }

    return;
}

/*
d_tests_sqlite_remove_all
  Removes a database file and the three SQLite may keep beside it, whether
or not they exist.
*/
static void
d_tests_sqlite_remove_all(
    const char* _path
)
{
    static const char* const suffixes[] = { "", "-journal", "-wal", "-shm" };
    char                     buffer[1024] = "";

    for (size_t i = 0u; i < (sizeof(suffixes) / sizeof(*suffixes)); ++i)
    {
        (void)snprintf(buffer, sizeof(buffer), "%s%s", _path, suffixes[i]);
        (void)remove(buffer);
    }

    return;
}

/*
d_tests_sqlite_int
  One integer from a query, or -999 when the query fails or has no row.
*/
static int64_t
d_tests_sqlite_int(
    sqlite3*    _db,
    const char* _sql
)
{
    int64_t value = -999;

    // the layer's own reader, which is checked in its own right
    if (d_sqlite_query_int64(_db, &value, _sql) != SQLITE_OK)
    {
        return -999;
    }

    return value;
}

/*
d_tests_sqlite_has_name
  Whether a name list holds a name, compared exactly.
*/
static bool
d_tests_sqlite_has_name(
    const struct d_db_name_list* _list,
    const char*                  _name
)
{
    for (size_t i = 0u; i < _list->count; ++i)
    {
        // an exact match ends the search
        if (strcmp(_list->names[i], _name) == 0)
        {
            return true;
        }
    }

    return false;
}

/*
d_tests_sqlite_find_column
  A column of a column list, by exact name, or NULL.
*/
static const struct d_db_column_info*
d_tests_sqlite_find_column(
    const struct d_db_column_list* _list,
    const char*                    _name
)
{
    for (size_t i = 0u; i < _list->count; ++i)
    {
        // an exact match ends the search
        if (strcmp(_list->columns[i].name, _name) == 0)
        {
            return &_list->columns[i];
        }
    }

    return NULL;
}

/*
d_tests_sqlite_same_page
  Whether a page of rows read a few at a time holds the columns of the rows
read all at once, and `_count` of their rows from `_first`, value for value.
*/
static bool
d_tests_sqlite_same_page(
    const struct d_db_rows* _page,
    const struct d_db_rows* _all,
    size_t                  _first,
    size_t                  _count
)
{
    // the same columns, and as many rows as expected
    if ( (_page->column_count != _all->column_count) ||
         (_page->row_count != _count)                ||
         ((_first + _count) > _all->row_count) )
    {
        return false;
    }

    for (size_t c = 0u; c < _all->column_count; ++c)
    {
        // a column named or typed otherwise
        if ( (strcmp(_page->columns[c].name, _all->columns[c].name) != 0) ||
             (_page->columns[c].type != _all->columns[c].type)            ||
             (strcmp(_page->columns[c].type_name,
                     _all->columns[c].type_name) != 0) )
        {
            return false;
        }
    }

    // the same values, row by row
    for (size_t i = 0u; i < (_count * _all->column_count); ++i)
    {
        const struct d_db_value* const got  = &_page->values[i];
        const struct d_db_value* const want =
            &_all->values[(_first * _all->column_count) + i];
        bool                           same = ( (got->type == want->type) &&
                                                (got->size == want->size) );

        // the value itself, as its type holds it
        if (same)
        {
            switch (got->type)
            {
                case D_DB_VALUE_INTEGER:
                    same = (got->as.integer == want->as.integer);

                    break;

                case D_DB_VALUE_REAL:
                    same = (memcmp(&got->as.real,
                                   &want->as.real,
                                   sizeof(double)) == 0);

                    break;

                case D_DB_VALUE_TEXT:
                case D_DB_VALUE_BLOB:
                    same = (memcmp(got->as.blob, want->as.blob, got->size) ==
                            0);

                    break;

                default:
                    break;
            }
        }

        // the first difference ends the comparison
        if (!same)
        {
            return false;
        }
    }

    return true;
}

/*
d_tests_sqlite_probe
  Whether SQL needing a feature runs, in a fresh in-memory database opened
natively, so that nothing the layer does is involved.
*/
static bool
d_tests_sqlite_probe(
    const char* _sql
)
{
    sqlite3* db     = NULL;
    int      result = sqlite3_open(":memory:", &db);

    // the probe itself
    if (result == SQLITE_OK)
    {
        result = sqlite3_exec(db, _sql, NULL, NULL, NULL);
    }

    sqlite3_close(db);

    return (result == SQLITE_OK);
}

/*
d_tests_sqlite_open_values
  An in-memory connection holding one table whose name needs quoting,
we"ird(a, b), with the rows (1, 'x'), (2, NULL) and (3, 'it''s').
*/
static sqlite3*
d_tests_sqlite_open_values(
    void
)
{
    sqlite3* db = NULL;

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db,
                      "CREATE TABLE \"we\"\"ird\"(a, b);"
                      "INSERT INTO \"we\"\"ird\" VALUES (1, 'x');"
                      "INSERT INTO \"we\"\"ird\" VALUES (2, NULL);"
                      "INSERT INTO \"we\"\"ird\" VALUES (3, 'it''s')") ==
        SQLITE_OK);

    return db;
}

/*
d_tests_sqlite_open_schemas
  An in-memory connection with a second in-memory database attached as
"aux", so that a test can work in main, temp and an attached schema at once.
*/
static sqlite3*
d_tests_sqlite_open_schemas(
    void
)
{
    sqlite3* db = NULL;

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_attach_database(db, ":memory:", "aux") == SQLITE_OK);

    return db;
}

/*
d_tests_sqlite_open_people
  An in-memory connection holding people(id, email UNIQUE, last, first), two
of them Smiths, and audit(n), one row of 0, for the index, view and trigger
tests.
*/
static sqlite3*
d_tests_sqlite_open_people(
    void
)
{
    sqlite3* db = NULL;

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db,
                      "CREATE TABLE people(id INTEGER PRIMARY KEY, "
                      "email TEXT UNIQUE, last TEXT, first TEXT);"
                      "CREATE TABLE audit(n INTEGER);"
                      "INSERT INTO people VALUES (1, 'a@x', 'Smith', 'Al');"
                      "INSERT INTO people VALUES (2, 'b@x', 'Smith', 'Bo');"
                      "INSERT INTO audit VALUES (0)") == SQLITE_OK);

    return db;
}

/*
d_tests_sqlite_open_file
  A database file, made anew, holding t(a INTEGER PRIMARY KEY, b TEXT) with
an index on b and three rows, 'one' to 'three', and a second in-memory
database attached as "aux" holding x(a).
*/
static sqlite3*
d_tests_sqlite_open_file(
    const char* _path
)
{
    sqlite3* db = NULL;

    d_tests_sqlite_remove_all(_path);
    D_TESTS_SQLITE_CHECK(d_sqlite_open(_path, &db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db,
                      "CREATE TABLE t(a INTEGER PRIMARY KEY, b TEXT);"
                      "CREATE INDEX tb ON t(b);"
                      "INSERT INTO t(b) VALUES ('one');"
                      "INSERT INTO t(b) VALUES ('two');"
                      "INSERT INTO t(b) VALUES ('three')") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_attach_database(db, ":memory:", "aux") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "CREATE TABLE aux.x(a)") == SQLITE_OK);

    return db;
}

//==============================================================================
// db_common.h
//==============================================================================

/*
d_tests_sqlite_db_common
  db_common.h's vocabulary.
  Tests the following:
  - the status codes are 0 and distinct negatives, MISMATCH among them with
    a message, and the value past the last without one
  - the shared flags fit below D_DB_VENDOR_FLAGS, and SQLite's above them
  - every d_db_type has a name, and a stray value is "unknown"
  - the release functions accept NULL and empty lists, and leave them empty
*/
static void
d_tests_sqlite_db_common(
    void
)
{
    struct d_db_name_list   names   = { 0u, NULL };
    struct d_db_column_list columns = { 0u, NULL };

    printf("db_common\n");

    D_TESTS_SQLITE_CHECK(D_DB_OK == 0);
    D_TESTS_SQLITE_CHECK(D_DB_UNSUPPORTED < 0);
    D_TESTS_SQLITE_CHECK(D_DB_EXISTS < 0);
    D_TESTS_SQLITE_CHECK(D_DB_NOT_FOUND < 0);
    D_TESTS_SQLITE_CHECK(D_DB_UNSUPPORTED != D_DB_EXISTS);
    D_TESTS_SQLITE_CHECK(D_DB_EXISTS != D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(D_DB_MISMATCH < 0);
    D_TESTS_SQLITE_CHECK( (D_DB_MISMATCH != D_DB_UNSUPPORTED)      &&
                          (D_DB_MISMATCH != D_DB_EXISTS)           &&
                          (D_DB_MISMATCH != D_DB_NOT_FOUND)        &&
                          (D_DB_MISMATCH != D_DB_INVALID_ARGUMENT) );
    D_TESTS_SQLITE_CHECK(d_db_status_message(D_DB_MISMATCH) != NULL);
    D_TESTS_SQLITE_CHECK(d_db_status_message(D_DB_WAIT) != NULL);
    D_TESTS_SQLITE_CHECK(d_db_status_message(D_DB_WAIT - 1) == NULL);
    D_TESTS_SQLITE_CHECK(
        (D_DB_IF_NOT_EXISTS | D_DB_IF_EXISTS | D_DB_UNIQUE) <
        D_DB_VENDOR_FLAGS);
    D_TESTS_SQLITE_CHECK((D_SQLITE_STRICT & (D_DB_VENDOR_FLAGS - 1u)) == 0u);
    D_TESTS_SQLITE_CHECK(D_SQLITE_STRICT != D_SQLITE_WITHOUT_ROWID);

    D_TESTS_SQLITE_CHECK(strcmp(d_db_type_name(D_DB_TYPE_NONE), "none") == 0);
    D_TESTS_SQLITE_CHECK(
        strcmp(d_db_type_name(D_DB_TYPE_BIG_INTEGER), "big_integer") == 0);
    D_TESTS_SQLITE_CHECK(strcmp(d_db_type_name(D_DB_TYPE_UUID), "uuid") == 0);
    D_TESTS_SQLITE_CHECK(
        strcmp(d_db_type_name((enum d_db_type)99), "unknown") == 0);

    for (int t = D_DB_TYPE_NONE; t <= D_DB_TYPE_UUID; ++t)
    {
        D_TESTS_SQLITE_CHECK(
            strcmp(d_db_type_name((enum d_db_type)t), "unknown") != 0);
    }

    d_db_name_list_free(NULL);
    d_db_column_list_free(NULL);
    d_db_name_list_free(&names);
    d_db_column_list_free(&columns);
    d_db_free(NULL);
    D_TESTS_SQLITE_CHECK( (names.count == 0u) && (names.names == NULL) );
    D_TESTS_SQLITE_CHECK( (columns.count == 0u) && (columns.columns == NULL) );

    return;
}

//==============================================================================
// db_common.h: rows
//==============================================================================

/*
d_tests_sqlite_value
  A value as a layer fills one in: an INTEGER or a REAL holding a number, or
TEXT or a BLOB holding the bytes of a C string; any other type holds nothing.
*/
static struct d_db_value
d_tests_sqlite_value(
    enum d_db_value_type _type,
    int64_t              _integer,
    double               _real,
    const char*          _bytes
)
{
    struct d_db_value value = { _type, 0u, { 0 } };

    switch (_type)
    {
        case D_DB_VALUE_INTEGER:
            value.as.integer = _integer;

            break;

        case D_DB_VALUE_REAL:
            value.as.real = _real;

            break;

        case D_DB_VALUE_TEXT:
            value.as.text = _bytes;
            value.size    = strlen(_bytes);

            break;

        case D_DB_VALUE_BLOB:
            value.as.blob = _bytes;
            value.size    = strlen(_bytes);

            break;

        case D_DB_VALUE_NULL:
        default:
            break;
    }

    return value;
}

/*
d_tests_sqlite_text_value
  A TEXT value of a C string.
*/
static struct d_db_value
d_tests_sqlite_text_value(
    const char* _text
)
{
    return d_tests_sqlite_value(D_DB_VALUE_TEXT, 0, 0.0, _text);
}

/*
d_tests_sqlite_integer_conversions
  d_db_value_int64, on values made by hand.
  Tests the following:
  - an INTEGER, whatever it holds
  - a REAL with no fraction, from -2^63 up to but not including 2^63, which
    has no int64_t; a fraction, NaN and an infinity are D_DB_MISMATCH
  - text that is an integer, either sign, up to the limits and not past
    them; a fraction, an exponent, spaces, a NUL inside and "" are not
  - NULL and a BLOB are D_DB_MISMATCH; a mismatch writes nothing
*/
static void
d_tests_sqlite_integer_conversions(
    void
)
{
    static const char* const refused[] =
    {
        "", "4.0", "4e2", " 4", "4 ", "0x10", "--4", "+", "9223372036854775808",
        "-9223372036854775809"
    };
    struct d_db_value value = { D_DB_VALUE_NULL, 0u, { 0 } };
    int64_t           out   = 0;

    printf("db_common: integer conversions\n");

    value = d_tests_sqlite_value(D_DB_VALUE_INTEGER, INT64_MIN, 0.0, NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_int64(&value, &out) == D_DB_OK) &&
                          (out == INT64_MIN) );
    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, -3.0, NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_int64(&value, &out) == D_DB_OK) &&
                          (out == -3) );
    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, -9223372036854775808.0,
                                 NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_int64(&value, &out) == D_DB_OK) &&
                          (out == INT64_MIN) );
    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, 9007199254740992.0, NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_int64(&value, &out) == D_DB_OK) &&
                          (out == 9007199254740992) );

    // refused, and nothing written
    out   = 77;
    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, 9223372036854775808.0,
                                 NULL);
    D_TESTS_SQLITE_CHECK(d_db_value_int64(&value, &out) == D_DB_MISMATCH);
    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, 2.5, NULL);
    D_TESTS_SQLITE_CHECK(d_db_value_int64(&value, &out) == D_DB_MISMATCH);
    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, NAN, NULL);
    D_TESTS_SQLITE_CHECK(d_db_value_int64(&value, &out) == D_DB_MISMATCH);
    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, -INFINITY, NULL);
    D_TESTS_SQLITE_CHECK(d_db_value_int64(&value, &out) == D_DB_MISMATCH);
    value = d_tests_sqlite_value(D_DB_VALUE_NULL, 0, 0.0, NULL);
    D_TESTS_SQLITE_CHECK(d_db_value_int64(&value, &out) == D_DB_MISMATCH);
    value = d_tests_sqlite_value(D_DB_VALUE_BLOB, 0, 0.0, "12");
    D_TESTS_SQLITE_CHECK(d_db_value_int64(&value, &out) == D_DB_MISMATCH);
    D_TESTS_SQLITE_CHECK(out == 77);

    // text
    value = d_tests_sqlite_text_value("-42");
    D_TESTS_SQLITE_CHECK( (d_db_value_int64(&value, &out) == D_DB_OK) &&
                          (out == -42) );
    value = d_tests_sqlite_text_value("+7");
    D_TESTS_SQLITE_CHECK( (d_db_value_int64(&value, &out) == D_DB_OK) &&
                          (out == 7) );
    value = d_tests_sqlite_text_value("9223372036854775807");
    D_TESTS_SQLITE_CHECK( (d_db_value_int64(&value, &out) == D_DB_OK) &&
                          (out == INT64_MAX) );
    value = d_tests_sqlite_text_value("-9223372036854775808");
    D_TESTS_SQLITE_CHECK( (d_db_value_int64(&value, &out) == D_DB_OK) &&
                          (out == INT64_MIN) );

    out = 77;

    for (size_t i = 0u; i < (sizeof(refused) / sizeof(refused[0])); ++i)
    {
        value = d_tests_sqlite_text_value(refused[i]);

        // say which text was read, beyond the line
        if (d_db_value_int64(&value, &out) != D_DB_MISMATCH)
        {
            printf("  text \"%s\" read as an integer\n", refused[i]);
        }

        D_TESTS_SQLITE_CHECK(d_db_value_int64(&value, &out) == D_DB_MISMATCH);
    }

    // a NUL inside the text: the size says there is more than the C string
    value      = d_tests_sqlite_text_value("12");
    value.size = 3u;
    D_TESTS_SQLITE_CHECK(d_db_value_int64(&value, &out) == D_DB_MISMATCH);
    D_TESTS_SQLITE_CHECK(out == 77);

    D_TESTS_SQLITE_CHECK(d_db_value_int64(NULL, &out) ==
                         D_DB_INVALID_ARGUMENT);
    D_TESTS_SQLITE_CHECK(d_db_value_int64(&value, NULL) ==
                         D_DB_INVALID_ARGUMENT);

    return;
}

/*
d_tests_sqlite_real_conversions
  d_db_value_double, on values made by hand.
  Tests the following:
  - a REAL, NaN and the infinities included
  - an INTEGER a double holds exactly -- 2^53, -2^63 -- and not one it does
    not: 2^53 + 1, 2^63 - 1
  - text that is a number, with a fraction, an exponent or neither; text
    of any other shape, a NUL inside included, is not
  - NULL and a BLOB are D_DB_MISMATCH; a mismatch writes nothing
*/
static void
d_tests_sqlite_real_conversions(
    void
)
{
    static const char* const refused[] =
    {
        "", ".", "e5", "1e", "1,5", " 1.5", "1.5 ", "inf", "NaN", "0x1p3",
        "1e999"
    };
    struct d_db_value value = { D_DB_VALUE_NULL, 0u, { 0 } };
    double            out   = 0.0;

    printf("db_common: real conversions\n");

    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, 0.1, NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_double(&value, &out) == D_DB_OK) &&
                          (out == 0.1) );
    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, NAN, NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_double(&value, &out) == D_DB_OK) &&
                          (isnan(out)) );
    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, -INFINITY, NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_double(&value, &out) == D_DB_OK) &&
                          (isinf(out)) && (out < 0.0) );
    value = d_tests_sqlite_value(D_DB_VALUE_INTEGER, 9007199254740992, 0.0,
                                 NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_double(&value, &out) == D_DB_OK) &&
                          (out == 9007199254740992.0) );
    value = d_tests_sqlite_value(D_DB_VALUE_INTEGER, INT64_MIN, 0.0, NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_double(&value, &out) == D_DB_OK) &&
                          (out == -9223372036854775808.0) );

    // refused, and nothing written
    out   = 7.5;
    value = d_tests_sqlite_value(D_DB_VALUE_INTEGER, 9007199254740993, 0.0,
                                 NULL);
    D_TESTS_SQLITE_CHECK(d_db_value_double(&value, &out) == D_DB_MISMATCH);
    value = d_tests_sqlite_value(D_DB_VALUE_INTEGER, INT64_MAX, 0.0, NULL);
    D_TESTS_SQLITE_CHECK(d_db_value_double(&value, &out) == D_DB_MISMATCH);
    value = d_tests_sqlite_value(D_DB_VALUE_NULL, 0, 0.0, NULL);
    D_TESTS_SQLITE_CHECK(d_db_value_double(&value, &out) == D_DB_MISMATCH);
    value = d_tests_sqlite_value(D_DB_VALUE_BLOB, 0, 0.0, "1.5");
    D_TESTS_SQLITE_CHECK(d_db_value_double(&value, &out) == D_DB_MISMATCH);
    D_TESTS_SQLITE_CHECK(out == 7.5);

    // text
    value = d_tests_sqlite_text_value("-2.5");
    D_TESTS_SQLITE_CHECK( (d_db_value_double(&value, &out) == D_DB_OK) &&
                          (out == -2.5) );
    value = d_tests_sqlite_text_value("1e3");
    D_TESTS_SQLITE_CHECK( (d_db_value_double(&value, &out) == D_DB_OK) &&
                          (out == 1000.0) );
    value = d_tests_sqlite_text_value(".5");
    D_TESTS_SQLITE_CHECK( (d_db_value_double(&value, &out) == D_DB_OK) &&
                          (out == 0.5) );
    value = d_tests_sqlite_text_value("0.30000000000000004");
    D_TESTS_SQLITE_CHECK( (d_db_value_double(&value, &out) == D_DB_OK) &&
                          (out == 0.30000000000000004) );
    value = d_tests_sqlite_text_value("12");
    D_TESTS_SQLITE_CHECK( (d_db_value_double(&value, &out) == D_DB_OK) &&
                          (out == 12.0) );

    out = 7.5;

    for (size_t i = 0u; i < (sizeof(refused) / sizeof(refused[0])); ++i)
    {
        value = d_tests_sqlite_text_value(refused[i]);

        // say which text was read, beyond the line
        if (d_db_value_double(&value, &out) != D_DB_MISMATCH)
        {
            printf("  text \"%s\" read as a number\n", refused[i]);
        }

        D_TESTS_SQLITE_CHECK(d_db_value_double(&value, &out) ==
                             D_DB_MISMATCH);
    }

    value      = d_tests_sqlite_text_value("1.5");
    value.size = 4u;
    D_TESTS_SQLITE_CHECK(d_db_value_double(&value, &out) == D_DB_MISMATCH);
    D_TESTS_SQLITE_CHECK(out == 7.5);

    D_TESTS_SQLITE_CHECK(d_db_value_double(NULL, &out) ==
                         D_DB_INVALID_ARGUMENT);
    D_TESTS_SQLITE_CHECK(d_db_value_double(&value, NULL) ==
                         D_DB_INVALID_ARGUMENT);

    return;
}

/*
d_tests_sqlite_bool_conversions
  d_db_value_bool, on values made by hand.
  Tests the following:
  - an INTEGER: 0 false, any other true
  - PostgreSQL's words for a boolean, in any ASCII case, whole
  - other words, prefixes and spaced words; a REAL, NULL and a BLOB are
    D_DB_MISMATCH; a mismatch writes nothing
*/
static void
d_tests_sqlite_bool_conversions(
    void
)
{
    static const char* const truths[]    =
    {
        "t", "TRUE", "y", "Yes", "on", "1"
    };
    static const char* const falsities[] =
    {
        "f", "False", "n", "NO", "Off", "0"
    };
    static const char* const neither[]   =
    {
        "", "maybe", "tr", "2", " t", "on ", "-1", "yess"
    };
    struct d_db_value value = { D_DB_VALUE_NULL, 0u, { 0 } };
    bool              out   = false;

    printf("db_common: boolean conversions\n");

    value = d_tests_sqlite_value(D_DB_VALUE_INTEGER, 0, 0.0, NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_bool(&value, &out) == D_DB_OK) &&
                          (!out) );
    value = d_tests_sqlite_value(D_DB_VALUE_INTEGER, -5, 0.0, NULL);
    D_TESTS_SQLITE_CHECK( (d_db_value_bool(&value, &out) == D_DB_OK) &&
                          (out) );

    for (size_t i = 0u; i < (sizeof(truths) / sizeof(truths[0])); ++i)
    {
        out   = false;
        value = d_tests_sqlite_text_value(truths[i]);
        D_TESTS_SQLITE_CHECK( (d_db_value_bool(&value, &out) == D_DB_OK) &&
                              (out) );
        value = d_tests_sqlite_text_value(falsities[i]);
        D_TESTS_SQLITE_CHECK( (d_db_value_bool(&value, &out) == D_DB_OK) &&
                              (!out) );
    }

    out = true;

    for (size_t i = 0u; i < (sizeof(neither) / sizeof(neither[0])); ++i)
    {
        value = d_tests_sqlite_text_value(neither[i]);
        D_TESTS_SQLITE_CHECK(d_db_value_bool(&value, &out) == D_DB_MISMATCH);
    }

    value = d_tests_sqlite_value(D_DB_VALUE_REAL, 0, 1.0, NULL);
    D_TESTS_SQLITE_CHECK(d_db_value_bool(&value, &out) == D_DB_MISMATCH);
    value = d_tests_sqlite_value(D_DB_VALUE_NULL, 0, 0.0, NULL);
    D_TESTS_SQLITE_CHECK(d_db_value_bool(&value, &out) == D_DB_MISMATCH);
    value = d_tests_sqlite_value(D_DB_VALUE_BLOB, 0, 0.0, "t");
    D_TESTS_SQLITE_CHECK(d_db_value_bool(&value, &out) == D_DB_MISMATCH);
    value      = d_tests_sqlite_text_value("t");
    value.size = 2u;
    D_TESTS_SQLITE_CHECK(d_db_value_bool(&value, &out) == D_DB_MISMATCH);
    D_TESTS_SQLITE_CHECK(out);

    D_TESTS_SQLITE_CHECK(d_db_value_bool(NULL, &out) ==
                         D_DB_INVALID_ARGUMENT);
    D_TESTS_SQLITE_CHECK(d_db_value_bool(&value, NULL) ==
                         D_DB_INVALID_ARGUMENT);

    return;
}

/*
d_tests_sqlite_rows_reading
  Finding values and columns in rows, and releasing them.
  Tests the following:
  - d_db_rows_value finds each value row by row, and answers NULL past the
    last row or column, and for NULL rows
  - a column found by its exact name before a match without case, and by a
    match without case when no exact one exists; a name not there is
    D_DB_NOT_FOUND, and leaves the index alone
  - d_db_rows_free empties the rows, and accepts NULL and empty rows
  - NULL arguments are D_DB_INVALID_ARGUMENT
*/
static void
d_tests_sqlite_rows_reading(
    void
)
{
    sqlite3*         db    = NULL;
    struct d_db_rows rows  = { 0u, 0u, NULL, NULL };
    struct d_db_rows empty = { 0u, 0u, NULL, NULL };
    size_t           index = 0u;

    printf("db_common: reading rows\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db,
                            &rows,
                            "SELECT 1 AS \"ID\", 'a' AS \"id\", 2.5 AS x "
                            "UNION ALL SELECT 2, 'b', NULL") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 3u) &&
                          (rows.row_count == 2u) );

    const struct d_db_value* const first = d_db_rows_value(&rows, 0u, 0u);
    const struct d_db_value* const last  = d_db_rows_value(&rows, 1u, 2u);

    D_TESTS_SQLITE_CHECK( (first)                               &&
                          (first->type == D_DB_VALUE_INTEGER)   &&
                          (first->as.integer == 1) );
    D_TESTS_SQLITE_CHECK( (last) &&
                          (last->type == D_DB_VALUE_NULL) );
    D_TESTS_SQLITE_CHECK(d_db_rows_value(&rows, 1u, 1u) == &rows.values[4]);
    D_TESTS_SQLITE_CHECK(d_db_rows_value(&rows, 2u, 0u) == NULL);
    D_TESTS_SQLITE_CHECK(d_db_rows_value(&rows, 0u, 3u) == NULL);
    D_TESTS_SQLITE_CHECK(d_db_rows_value(&rows, (size_t)-1, 0u) == NULL);
    D_TESTS_SQLITE_CHECK(d_db_rows_value(NULL, 0u, 0u) == NULL);
    D_TESTS_SQLITE_CHECK(d_db_rows_value(&empty, 0u, 0u) == NULL);

    // the exact spelling first, then any case
    D_TESTS_SQLITE_CHECK(
        (d_db_rows_column_index(&rows, "id", &index) == D_DB_OK) &&
        (index == 1u));
    D_TESTS_SQLITE_CHECK(
        (d_db_rows_column_index(&rows, "ID", &index) == D_DB_OK) &&
        (index == 0u));
    D_TESTS_SQLITE_CHECK(
        (d_db_rows_column_index(&rows, "iD", &index) == D_DB_OK) &&
        (index == 0u));
    D_TESTS_SQLITE_CHECK(
        (d_db_rows_column_index(&rows, "X", &index) == D_DB_OK) &&
        (index == 2u));
    index = 99u;
    D_TESTS_SQLITE_CHECK(d_db_rows_column_index(&rows, "y", &index) ==
                         D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(d_db_rows_column_index(&rows, "", &index) ==
                         D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(d_db_rows_column_index(&empty, "x", &index) ==
                         D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(index == 99u);
    D_TESTS_SQLITE_CHECK(d_db_rows_column_index(NULL, "x", &index) ==
                         D_DB_INVALID_ARGUMENT);
    D_TESTS_SQLITE_CHECK(d_db_rows_column_index(&rows, NULL, &index) ==
                         D_DB_INVALID_ARGUMENT);
    D_TESTS_SQLITE_CHECK(d_db_rows_column_index(&rows, "x", NULL) ==
                         D_DB_INVALID_ARGUMENT);

    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 0u) &&
                          (rows.row_count == 0u)    &&
                          (rows.columns == NULL)    &&
                          (rows.values == NULL) );
    d_db_rows_free(&rows);
    d_db_rows_free(&empty);
    d_db_rows_free(NULL);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

//==============================================================================
// sqlite_common.h
//==============================================================================

/*
d_tests_sqlite_versions
  The library's version, as a string, a number and a comparison.
  Tests the following:
  - a non-empty string; a number at or above the floor, SQLite's own
  - the header number equal to SQLITE_VERSION_NUMBER
  - library_at_least on each side of the exact version, and with parts no
    release can have, overflow-sized ones included
*/
static void
d_tests_sqlite_versions(
    void
)
{
    const int library = sqlite3_libversion_number();
    const int major   = library / 1000000;
    const int minor   = (library / 1000) % 1000;
    const int patch   = library % 1000;

    printf("sqlite_common: versions (SQLite %s, compiled against %d)\n",
           d_sqlite_library_version(),
           d_sqlite_header_version_number());

    D_TESTS_SQLITE_CHECK(strlen(d_sqlite_library_version()) > 0u);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_library_version_number() >= D_SQLITE_FLOOR_VERSION_NUMBER);
    D_TESTS_SQLITE_CHECK(d_sqlite_library_version_number() == library);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_header_version_number() == SQLITE_VERSION_NUMBER);

    D_TESTS_SQLITE_CHECK(d_sqlite_library_at_least(3, 7, 17));
    D_TESTS_SQLITE_CHECK(d_sqlite_library_at_least(2, 999, 999));
    D_TESTS_SQLITE_CHECK(d_sqlite_library_at_least(major, minor, patch));
    D_TESTS_SQLITE_CHECK(!d_sqlite_library_at_least(major, minor, patch + 1));
    D_TESTS_SQLITE_CHECK(!d_sqlite_library_at_least(major, minor + 1, 0));
    D_TESTS_SQLITE_CHECK(!d_sqlite_library_at_least(major + 1, 0, 0));
    D_TESTS_SQLITE_CHECK(!d_sqlite_library_at_least(99, 0, 0));
    D_TESTS_SQLITE_CHECK(!d_sqlite_library_at_least(3, 1000, 0));
    D_TESTS_SQLITE_CHECK(!d_sqlite_library_at_least(INT_MAX, 0, 0));
    D_TESTS_SQLITE_CHECK(!d_sqlite_library_at_least(major, INT_MAX, 0));
    D_TESTS_SQLITE_CHECK(!d_sqlite_library_at_least(-1, 0, 0));
    D_TESTS_SQLITE_CHECK(!d_sqlite_library_at_least(3, 0, -1));

    return;
}

/*
d_tests_sqlite_features
  The feature table, against the library.
  Tests the following:
  - every SQL-testable feature: d_sqlite_supports agrees with SQL that needs
    it, in a database the layer never touched
  - the version-only features agree with their releases
  - WAL as the build says (OMIT_WAL); encryption, which no build here has;
    the threading level, sqlite3_threadsafe's
  - a value outside the enumeration is no feature
*/
static void
d_tests_sqlite_features(
    void
)
{
    const size_t count = sizeof(d_tests_sqlite_probes) /
                         sizeof(d_tests_sqlite_probes[0]);

    printf("sqlite_common: features\n");

    for (size_t i = 0u; i < count; ++i)
    {
        const struct d_tests_sqlite_feature_probe* const probe =
            &d_tests_sqlite_probes[i];
        const bool claimed = d_sqlite_supports(probe->feature);
        const bool works   = d_tests_sqlite_probe(probe->sql);

        // say which feature disagreed, beyond the line
        if (claimed != works)
        {
            printf("  feature %d: supports says %d, SQL says %d (%s)\n",
                   (int)probe->feature,
                   (int)claimed,
                   (int)works,
                   probe->sql);
        }

        D_TESTS_SQLITE_CHECK(claimed == works);
    }

    D_TESTS_SQLITE_CHECK(d_sqlite_supports(D_SQLITE_FEATURE_VACUUM_SCHEMA) ==
                         d_tests_sqlite_at_least(3, 15, 0));
    D_TESTS_SQLITE_CHECK(d_sqlite_supports(D_SQLITE_FEATURE_OPTIMIZE) ==
                         d_tests_sqlite_at_least(3, 18, 0));
    D_TESTS_SQLITE_CHECK(d_sqlite_supports(D_SQLITE_FEATURE_VACUUM_INTO) ==
                         d_tests_sqlite_at_least(3, 27, 0));
    D_TESTS_SQLITE_CHECK(d_sqlite_supports(D_SQLITE_FEATURE_WAL) ==
                         (sqlite3_compileoption_used("OMIT_WAL") == 0));
    D_TESTS_SQLITE_CHECK(!d_sqlite_supports(D_SQLITE_FEATURE_ENCRYPTION));
    D_TESTS_SQLITE_CHECK(d_sqlite_library_threadsafe() ==
                         sqlite3_threadsafe());
    D_TESTS_SQLITE_CHECK(!d_sqlite_supports((enum d_sqlite_feature)999));
    D_TESTS_SQLITE_CHECK(!d_sqlite_supports((enum d_sqlite_feature)-1));

    return;
}

/*
d_tests_sqlite_messages
  Result messages.
  Tests the following:
  - the layer's own codes have their own messages
  - an unknown negative value is "unknown result"
  - SQLite's codes have SQLite's messages
*/
static void
d_tests_sqlite_messages(
    void
)
{
    printf("sqlite_common: result messages\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_result_message(D_DB_UNSUPPORTED) != NULL);
    D_TESTS_SQLITE_CHECK(
        strcmp(d_sqlite_result_message(D_DB_EXISTS), "already exists") == 0);
    D_TESTS_SQLITE_CHECK(
        strcmp(d_sqlite_result_message(D_DB_NOT_FOUND), "not found") == 0);
    D_TESTS_SQLITE_CHECK(
        strcmp(d_sqlite_result_message(D_DB_MISMATCH),
               d_db_status_message(D_DB_MISMATCH)) == 0);
    D_TESTS_SQLITE_CHECK(
        strcmp(d_sqlite_result_message(-77), "unknown result") == 0);
    D_TESTS_SQLITE_CHECK(
        strcmp(d_sqlite_result_message(SQLITE_MISUSE),
               sqlite3_errstr(SQLITE_MISUSE)) == 0);
    D_TESTS_SQLITE_CHECK(
        strcmp(d_sqlite_result_message(SQLITE_OK),
               sqlite3_errstr(SQLITE_OK)) == 0);

    return;
}

/*
d_tests_sqlite_errors
  The connection's error message.
  Tests the following:
  - a failed statement leaves SQLite's own words, naming what failed
  - a statement that succeeds leaves "not an error"
  - a NULL connection has no message
*/
static void
d_tests_sqlite_errors(
    void
)
{
    struct sqlite3* db = NULL;

    printf("sqlite_common: error messages\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(db, "SELECT * FROM no_such_table") ==
                         SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_error_message(db)) &&
        (strstr(d_sqlite_error_message(db), "no_such_table") != NULL));
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(db, "SELECT 1") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_error_message(db)) &&
        (strcmp(d_sqlite_error_message(db), sqlite3_errstr(SQLITE_OK)) == 0));
    D_TESTS_SQLITE_CHECK(d_sqlite_error_message(NULL) == NULL);
    d_sqlite_close(db);

    return;
}

//==============================================================================
// sqlite_connection.h
//==============================================================================

/*
d_tests_sqlite_opening
  Opening and closing.
  Tests the following:
  - an open applies the configured defaults: the busy timeout, foreign keys
  - a file opens read-write and read-only, and a read-only one refuses writes
  - a failed open leaves NULL behind
  - a file that is not a database opens, and fails at first use
*/
static void
d_tests_sqlite_opening(
    void
)
{
    char     path[1024]      = "";
    char     text_path[1024] = "";
    sqlite3* db              = NULL;
    sqlite3* reader          = NULL;
    int      timeout         = -1;
    bool     keys            = false;

    printf("sqlite_connection: opening and closing\n");

    d_tests_sqlite_path(path, sizeof(path), "t_sqlite_opening.db");
    d_tests_sqlite_path(text_path, sizeof(text_path), "t_sqlite_text.db");
    d_tests_sqlite_remove_all(path);

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_busy_timeout(db, &timeout) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(timeout == 5000);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_foreign_keys(db, &keys) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(keys);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    // a file, read-write and then read-only
    D_TESTS_SQLITE_CHECK(d_sqlite_open(path, &db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(db, "CREATE TABLE t(a)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_readonly(path, &reader) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(reader, "INSERT INTO t VALUES (1)") == SQLITE_READONLY);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(reader) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    reader = (sqlite3*)&timeout;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_readonly("t_sqlite_no/such/file.db", &reader) ==
        SQLITE_CANTOPEN);
    D_TESTS_SQLITE_CHECK(reader == NULL);

    // a file that is not a database opens, and fails when first read
    d_tests_sqlite_write_file(text_path,
                              "this is plain text, and no SQLite database");
    D_TESTS_SQLITE_CHECK(d_sqlite_open(text_path, &db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "SELECT * FROM sqlite_master") == SQLITE_NOTADB);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    d_tests_sqlite_remove_all(path);
    d_tests_sqlite_remove_all(text_path);

    return;
}

/*
d_tests_sqlite_opening_with
  Opening as SQLite's flags say, through a VFS named.
  Tests the following:
  - READWRITE | CREATE makes a file and applies the defaults; READWRITE alone
    refuses a missing file; READONLY refuses writes
  - URI reads "file:" paths, their query included; MEMORY opens in memory
  - the default VFS, named, opens; an unknown one is SQLITE_ERROR
  - NOFOLLOW refuses a symbolic link from 3.31.0, and before it is refused
    itself with D_DB_UNSUPPORTED
  - flags that name no way of opening, both of a pair, or an unknown bit are
    SQLITE_MISUSE, as are NULL arguments; every failure leaves NULL behind
*/
static void
d_tests_sqlite_opening_with(
    void
)
{
    char           path[1024]   = "";
    char           linked[1024] = "";
    char           uri[1100]    = "";
    sqlite3*       db           = NULL;
    sqlite3*       other        = NULL;
    int            timeout      = -1;
    bool           keys         = false;
    const unsigned rwc          = D_SQLITE_OPEN_READWRITE |
                                  D_SQLITE_OPEN_CREATE;
    const char*    default_vfs  = sqlite3_vfs_find(NULL)->zName;

    printf("sqlite_connection: opening with flags\n");

    d_tests_sqlite_path(path, sizeof(path), "t_sqlite_with.db");
    d_tests_sqlite_path(linked, sizeof(linked), "t_sqlite_with_link.db");
    d_tests_sqlite_remove_all(path);
    (void)remove(linked);

    // a missing file without CREATE, then made with it, defaults applied
    other = (sqlite3*)&timeout;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path, D_SQLITE_OPEN_READWRITE, NULL, &other) ==
        SQLITE_CANTOPEN);
    D_TESTS_SQLITE_CHECK(other == NULL);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_with(path, rwc, NULL, &db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_busy_timeout(db, &timeout) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(timeout == 5000);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_foreign_keys(db, &keys) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(keys);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(db, "CREATE TABLE t(a)") == SQLITE_OK);

    // read-only, by flag and by URI
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path, D_SQLITE_OPEN_READONLY, NULL, &other) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(other, "INSERT INTO t VALUES (1)") == SQLITE_READONLY);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(other) == SQLITE_OK);

    // a URI's path is written with '/', which a Windows path may not be
    if ( (!strchr(path, '\\')) &&
         (!strchr(path, ':')) )
    {
        (void)snprintf(uri, sizeof(uri), "file:%s?mode=ro", path);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_open_with(uri, rwc | D_SQLITE_OPEN_URI, NULL, &other) ==
            SQLITE_OK);
        D_TESTS_SQLITE_CHECK(d_sqlite_is_readonly(other, NULL));
        D_TESTS_SQLITE_CHECK(d_sqlite_close(other) == SQLITE_OK);
    }

    // the default VFS by its name, and one of no name
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path, D_SQLITE_OPEN_READWRITE, default_vfs,
                           &other) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_has_table(other, NULL, "t"));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(other) == SQLITE_OK);
    other = (sqlite3*)&timeout;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path, rwc, "t_sqlite_no_such_vfs", &other) ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(other == NULL);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    // in memory, the path a name
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with("t_sqlite_named", rwc | D_SQLITE_OPEN_MEMORY, NULL,
                           &db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(strcmp(d_sqlite_database_path(db, NULL), "") == 0);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    // NOFOLLOW, where the library knows it
    if (d_tests_sqlite_at_least(3, 31, 0))
    {
        D_TESTS_SQLITE_CHECK(
            d_sqlite_open_with(path, rwc | D_SQLITE_OPEN_NOFOLLOW, NULL,
                               &db) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);
#if !defined(_WIN32)
        D_TESTS_SQLITE_CHECK(symlink(path, linked) == 0);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_open_with(linked, rwc | D_SQLITE_OPEN_NOFOLLOW, NULL,
                               &db) == SQLITE_CANTOPEN);
        D_TESTS_SQLITE_CHECK(db == NULL);
        D_TESTS_SQLITE_CHECK(d_sqlite_open_with(linked, rwc, NULL, &db) ==
                             SQLITE_OK);
        D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);
        (void)remove(linked);
#endif  // !defined(_WIN32)
    }
    else
    {
        D_TESTS_SQLITE_CHECK(
            d_sqlite_open_with(path, rwc | D_SQLITE_OPEN_NOFOLLOW, NULL,
                               &db) == D_DB_UNSUPPORTED);
        D_TESTS_SQLITE_CHECK(db == NULL);
    }

    // flags that cannot be right, and NULL arguments
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path, 0u, NULL, &db) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path, D_SQLITE_OPEN_CREATE, NULL, &db) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path,
                           D_SQLITE_OPEN_READONLY | D_SQLITE_OPEN_CREATE,
                           NULL, &db) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path,
                           D_SQLITE_OPEN_READONLY | D_SQLITE_OPEN_READWRITE,
                           NULL, &db) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path,
                           rwc | D_SQLITE_OPEN_NOMUTEX |
                           D_SQLITE_OPEN_FULLMUTEX,
                           NULL, &db) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path,
                           rwc | D_SQLITE_OPEN_SHAREDCACHE |
                           D_SQLITE_OPEN_PRIVATECACHE,
                           NULL, &db) == SQLITE_MISUSE);
    db = (sqlite3*)&timeout;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path, rwc | 0x00000008u, NULL, &db) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(db == NULL);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(NULL, rwc, NULL, &db) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_open_with(path, rwc, NULL, NULL) == SQLITE_MISUSE);

    d_tests_sqlite_remove_all(path);

    return;
}

/*
d_tests_sqlite_apply_defaults
  The configured defaults, on a connection SQLite opened itself.
  Tests the following:
  - apply_defaults turns foreign keys on and sets the busy timeout
*/
static void
d_tests_sqlite_apply_defaults(
    void
)
{
    sqlite3* db      = NULL;
    int      timeout = -1;
    bool     keys    = true;

    printf("sqlite_connection: applying defaults\n");

    D_TESTS_SQLITE_CHECK(sqlite3_open(":memory:", &db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_foreign_keys(db, &keys) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!keys);
    D_TESTS_SQLITE_CHECK(d_sqlite_apply_defaults(db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_foreign_keys(db, &keys) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(keys);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_busy_timeout(db, &timeout) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(timeout == 5000);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_location
  Where a database lives, and whether it can be written.
  Tests the following:
  - database_path: "" for memory, the file's path for a file, NULL for a
    name not attached
  - is_readonly: for read-write, read-only and missing schemas
*/
static void
d_tests_sqlite_location(
    void
)
{
    char     path[1024] = "";
    sqlite3* db         = NULL;
    sqlite3* reader     = NULL;

    printf("sqlite_connection: location and access\n");

    d_tests_sqlite_path(path, sizeof(path), "t_sqlite_location.db");
    d_tests_sqlite_remove_all(path);

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(strcmp(d_sqlite_database_path(db, NULL), "") == 0);
    D_TESTS_SQLITE_CHECK(d_sqlite_database_path(db, "nowhere") == NULL);
    D_TESTS_SQLITE_CHECK(!d_sqlite_is_readonly(db, NULL));
    D_TESTS_SQLITE_CHECK(!d_sqlite_is_readonly(db, "nowhere"));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    D_TESTS_SQLITE_CHECK(d_sqlite_open(path, &db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(db, "CREATE TABLE t(a)") == SQLITE_OK);

    const char* const where = d_sqlite_database_path(db, "main");

    D_TESTS_SQLITE_CHECK( (where) &&
                          (strstr(where, "t_sqlite_location.db") != NULL) );
    D_TESTS_SQLITE_CHECK(!d_sqlite_is_readonly(db, "main"));
    D_TESTS_SQLITE_CHECK(d_sqlite_open_readonly(path, &reader) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_is_readonly(reader, NULL));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(reader) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    d_tests_sqlite_remove_all(path);

    return;
}

/*
d_tests_sqlite_rows_changed
  Row ids and change counts.
  Tests the following:
  - last_insert_id after a multi-row insert
  - changes counts the last statement; total_changes the connection's life
*/
static void
d_tests_sqlite_rows_changed(
    void
)
{
    sqlite3* db = NULL;

    printf("sqlite_connection: rows changed\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        sqlite3_exec(db,
                     "CREATE TABLE t(a INTEGER PRIMARY KEY, b);"
                     "INSERT INTO t(b) VALUES (1), (2), (3);",
                     NULL, NULL, NULL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_last_insert_id(db) == 3);
    D_TESTS_SQLITE_CHECK(
        sqlite3_exec(db, "UPDATE t SET b = b + 1", NULL, NULL, NULL) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_changes(db) == 3);
    D_TESTS_SQLITE_CHECK(d_sqlite_total_changes(db) == 6);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_extensions
  Extensions loaded from shared libraries.
  Tests the following:
  - supports says what the library has, the build being one that calls it
  - a library that is not there: SQLITE_ERROR, and SQLite's words for it
  - with the test extension (DJINTERP_TEST_SQLITE_EXTENSION, which
    ci/check_db_sqlite.sh builds): loaded by its default entry point, its
    function there; by a named one, its second function too; an entry point
    it does not have, refused with words; from 3.13.0, SQL's load_extension
    still off after a load, and on once enable_load_extension turns it on,
    and the C API loading again once it is turned off; before 3.13.0, the
    C API refused until loading is on
  - misuse: a NULL connection or path
*/
static void
d_tests_sqlite_extensions(
    void
)
{
    const char* const extension = getenv("DJINTERP_TEST_SQLITE_EXTENSION");
    const bool        separate  = d_tests_sqlite_at_least(3, 13, 0);
    char*             message   = NULL;
    sqlite3*          db        = NULL;

    printf("sqlite_connection: extensions (%s)\n",
           ( (extension) && (extension[0] != '\0') ) ? extension
                                                     : "none built");

    D_TESTS_SQLITE_CHECK(d_sqlite_supports(D_SQLITE_FEATURE_LOAD_EXTENSION) ==
                         (sqlite3_compileoption_used("OMIT_LOAD_EXTENSION") ==
                          0));
    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_load_extension(NULL, "x", NULL, &message) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(message == NULL);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_load_extension(db, NULL, NULL, &message) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_enable_load_extension(NULL, true) == SQLITE_MISUSE);

    // a library that is not there, where loading is possible at all
    if (d_sqlite_supports(D_SQLITE_FEATURE_LOAD_EXTENSION))
    {
        // before 3.13.0 the C API needs loading on
        if (!separate)
        {
            D_TESTS_SQLITE_CHECK(d_sqlite_load_extension(db,
                                                         "t_sqlite_nothing",
                                                         NULL,
                                                         &message) ==
                                 SQLITE_ERROR);
            d_db_free(message);
            D_TESTS_SQLITE_CHECK(
                d_sqlite_enable_load_extension(db, true) == SQLITE_OK);
        }

        D_TESTS_SQLITE_CHECK(d_sqlite_load_extension(db,
                                                     "t_sqlite_no/such_lib",
                                                     NULL,
                                                     &message) ==
                             SQLITE_ERROR);
        D_TESTS_SQLITE_CHECK( (message != NULL) &&
                              (message[0] != '\0') );
        d_db_free(message);
        message = NULL;
    }
    else
    {
        D_TESTS_SQLITE_CHECK(
            d_sqlite_load_extension(db, "x", NULL, &message) ==
            D_DB_UNSUPPORTED);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_enable_load_extension(db, true) == D_DB_UNSUPPORTED);
    }

    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    // the test extension, where it was built and loading is possible
    if ( (!extension)                                         ||
         (extension[0] == '\0')                               ||
         (!d_sqlite_supports(D_SQLITE_FEATURE_LOAD_EXTENSION)) )
    {
        return;
    }

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);

    // before 3.13.0 the C API needs loading on, which lets SQL load too
    if (!separate)
    {
        D_TESTS_SQLITE_CHECK(
            d_sqlite_enable_load_extension(db, true) == SQLITE_OK);
    }

    D_TESTS_SQLITE_CHECK(
        d_sqlite_load_extension(db, extension, NULL, &message) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(message == NULL);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT d_tests_answer()") == 42);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT d_tests_named()") == -999);
    D_TESTS_SQLITE_CHECK(d_sqlite_load_extension(db,
                                                 extension,
                                                 "d_tests_no_such_init",
                                                 &message) == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK( (message != NULL) &&
                          (message[0] != '\0') );
    d_db_free(message);
    message = NULL;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_load_extension(db,
                                extension,
                                "d_tests_sqlite_extension_init",
                                NULL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT d_tests_named()") == 7);

    // SQL's switch, apart from the C API's, from 3.13.0
    if (separate)
    {
        sqlite3* other = NULL;
        char     sql[1200] = "";

        (void)snprintf(sql,
                       sizeof(sql),
                       "SELECT load_extension('%s')",
                       extension);
        D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&other) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            sqlite3_exec(other, sql, NULL, NULL, NULL) == SQLITE_ERROR);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_enable_load_extension(other, true) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            sqlite3_exec(other, sql, NULL, NULL, NULL) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            d_tests_sqlite_int(other, "SELECT d_tests_answer()") == 42);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_enable_load_extension(other, false) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            sqlite3_exec(other, sql, NULL, NULL, NULL) == SQLITE_ERROR);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_load_extension(other,
                                    extension,
                                    "d_tests_sqlite_extension_init",
                                    NULL) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            d_tests_sqlite_int(other, "SELECT d_tests_named()") == 7);
        D_TESTS_SQLITE_CHECK(d_sqlite_close(other) == SQLITE_OK);
    }

    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_connection_arguments
  sqlite_connection.h's answers to arguments that cannot be right.
  Tests the following:
  - each open refuses a NULL path or handle pointer with SQLITE_MISUSE, as
    apply_defaults refuses NULL; close accepts NULL, as sqlite3_close does
  - the queries answer a NULL handle with 0, NULL or false
*/
static void
d_tests_sqlite_connection_arguments(
    void
)
{
    sqlite3* db = NULL;

    printf("sqlite_connection: arguments\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open(NULL, &db) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_open("unused.db", NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_readonly(NULL, &db) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_apply_defaults(NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(NULL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_database_path(NULL, NULL) == NULL);
    D_TESTS_SQLITE_CHECK(!d_sqlite_is_readonly(NULL, NULL));
    D_TESTS_SQLITE_CHECK(d_sqlite_last_insert_id(NULL) == 0);
    D_TESTS_SQLITE_CHECK(d_sqlite_changes(NULL) == 0);
    D_TESTS_SQLITE_CHECK(d_sqlite_total_changes(NULL) == 0);

    return;
}

//==============================================================================
// sqlite_query.h
//==============================================================================

/*
d_tests_sqlite_statements
  Running statements.
  Tests the following:
  - exec runs several statements; a failure stops it there, with SQLite's
    code and message, keeping what ran before it
  - execf quotes a name with %w and a value with %Q
*/
static void
d_tests_sqlite_statements(
    void
)
{
    printf("sqlite_query: running statements\n");

    sqlite3* const db = d_tests_sqlite_open_values();

    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db,
                      "INSERT INTO \"we\"\"ird\" VALUES (4, 'y');"
                      "INSERT INTO nowhere VALUES (1);"
                      "INSERT INTO \"we\"\"ird\" VALUES (5, 'z')") ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(strstr(sqlite3_errmsg(db), "nowhere") != NULL);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT max(a) FROM \"we\"\"ird\"") == 4);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_execf(db,
                       "INSERT INTO \"%w\" VALUES (%d, %Q)",
                       "we\"ird",
                       6,
                       "they're") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db,
                           "SELECT a FROM \"we\"\"ird\" WHERE b = 'they''re'")
        == 6);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_dialects
  The two format dialects.
  Tests the following:
  - a format that names %I or %T is the shared dialect: %I quotes a name, %T
    a qualified one and, with a NULL qualifier, the name alone; %Q quotes a
    value; %g writes the 17 digits that read back as the same double
  - a format that names neither is sqlite3_mprintf's: %g writes six
    significant digits, and %w, %q and %lld work as before
  - %% hides no conversion: "%%I" is text, and the format sqlite3_mprintf's
  - in the shared dialect a NULL name, a NaN and sqlite3_mprintf's own %q are
    SQLITE_MISUSE, and nothing runs
*/
static void
d_tests_sqlite_dialects(
    void
)
{
    printf("sqlite_query: format dialects\n");

    sqlite3* const db      = d_tests_sqlite_open_values();
    const double   precise = 0.1234567890123;
    double         real    = 0.0;
    int64_t        number  = 0;
    char*          text    = NULL;

    D_TESTS_SQLITE_CHECK(
        d_sqlite_execf(db, "CREATE TABLE %I (v)", "dia\"lect") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_has_table(db, NULL, "dia\"lect"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_execf(db,
                       "INSERT INTO %T VALUES (%Q), (%d)",
                       "main",
                       "dia\"lect",
                       "it's",
                       7) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_query_int64f(db,
                               &number,
                               "SELECT count(*) FROM %T",
                               (const char*)NULL,
                               "dia\"lect") == SQLITE_OK) &&
        (number == 2));
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_query_textf(db,
                              &text,
                              "SELECT v FROM %I WHERE v = %Q",
                              "dia\"lect",
                              "it's") == SQLITE_OK) &&
        (text)                                    &&
        (strcmp(text, "it's") == 0));
    d_db_free(text);
    text = NULL;

    // %g, round-tripping in the shared dialect and not in sqlite3_mprintf's
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_query_doublef(db,
                                &real,
                                "SELECT %g FROM %I LIMIT 1",
                                precise,
                                "dia\"lect") == SQLITE_OK) &&
        (real == precise));
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_query_doublef(db, &real, "SELECT %g", precise) ==
         SQLITE_OK) &&
        (real == 0.123457));
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_query_int64f(db,
                               &number,
                               "SELECT count(*) FROM \"%w\" WHERE v = '%q'",
                               "dia\"lect",
                               "it's") == SQLITE_OK) &&
        (number == 1));
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_query_int64f(db, &number, "SELECT %lld", (long long)5) ==
         SQLITE_OK) &&
        (number == 5));
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_query_textf(db, &text, "SELECT '%%I'") == SQLITE_OK) &&
        (text)                                                       &&
        (strcmp(text, "%I") == 0));
    d_db_free(text);
    text = NULL;

    // the shared dialect's refusals, which run nothing
    D_TESTS_SQLITE_CHECK(
        d_sqlite_execf(db, "DROP TABLE %I", (const char*)NULL) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_execf(db, "INSERT INTO %I VALUES (%g)", "dia\"lect", NAN) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_execf(db, "INSERT INTO %I VALUES ('%q')", "dia\"lect", "x")
        == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_query_int64f(db,
                               &number,
                               "SELECT count(*) FROM %I",
                               "dia\"lect") == SQLITE_OK) &&
        (number == 2));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_numbers
  Single integers and reals.
  Tests the following:
  - a value, plain and formatted
  - no row is D_DB_NOT_FOUND and leaves the value alone; NULL reads as 0
  - a failure at prepare and a failure at step are SQLite's codes
*/
static void
d_tests_sqlite_numbers(
    void
)
{
    printf("sqlite_query: single numbers\n");

    sqlite3* const db     = d_tests_sqlite_open_values();
    int64_t        number = 0;
    double         real   = 0.0;

    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_int64(db, &number, "SELECT count(*) FROM \"we\"\"ird\"")
        == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(number == 3);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_int64f(db,
                              &number,
                              "SELECT a FROM \"%w\" WHERE b = %Q",
                              "we\"ird",
                              "it's") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(number == 3);
    number = 42;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_int64(db, &number, "SELECT 1 WHERE 0") ==
        D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(number == 42);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_int64(db, &number, "SELECT NULL") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(number == 0);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_int64(db, &number, "SELECT * FROM nowhere") ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_int64(db, &number, "SELECT abs(-9223372036854775807-1)")
        == SQLITE_ERROR);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_double(db, &real, "SELECT 2.5") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(real == 2.5);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_doublef(db, &real, "SELECT %d / 4.0", 3) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(real == 0.75);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_double(db, &real, "SELECT 1 WHERE 0") ==
        D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(real == 0.75);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_text
  Single strings.
  Tests the following:
  - a value, plain and formatted, freed with d_db_free
  - text holding a NUL survives whole
  - NULL reads as a NULL string; no row is D_DB_NOT_FOUND and leaves the
    pointer alone
*/
static void
d_tests_sqlite_text(
    void
)
{
    printf("sqlite_query: single text\n");

    sqlite3* const db     = d_tests_sqlite_open_values();
    int64_t        marker = 0;
    char*          text   = NULL;

    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_text(db, &text, "SELECT 'hello'") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (text) && (strcmp(text, "hello") == 0) );
    d_db_free(text);
    text = NULL;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_textf(db,
                             &text,
                             "SELECT b FROM \"%w\" WHERE a = %d",
                             "we\"ird",
                             3) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (text) && (strcmp(text, "it's") == 0) );
    d_db_free(text);
    text = NULL;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_text(db, &text, "SELECT CAST(x'610062' AS TEXT)") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (text)            &&
                          (text[0] == 'a')  &&
                          (text[1] == '\0') &&
                          (text[2] == 'b')  &&
                          (text[3] == '\0') );
    d_db_free(text);

    text = (char*)&marker;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_text(db, &text, "SELECT NULL") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(text == NULL);
    text = (char*)&marker;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_text(db, &text, "SELECT 'x' WHERE 0") ==
        D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(text == (char*)&marker);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_changes_counted
  Running statements and counting the rows they changed.
  Tests the following:
  - an INSERT, an UPDATE and a DELETE count their rows; of several
    statements, the last counts
  - a trigger's rows are not counted
  - a SELECT counts the rows it returned; a CREATE TABLE straight after an
    UPDATE counts none, though SQLite's own count still holds the UPDATE's
  - a statement that changed nothing counts nothing; text holding no
    statement runs nothing and counts nothing
  - a RETURNING statement counts the rows it changed, where SQLite has
    RETURNING
  - a failure stops the rest, keeps what ran before it, and writes no count
  - the `f` form formats in both dialects
*/
static void
d_tests_sqlite_changes_counted(
    void
)
{
    sqlite3* const db      = d_tests_sqlite_open_people();
    int64_t        changes = -1;

    printf("sqlite_query: rows changed\n");

    D_TESTS_SQLITE_CHECK(
        (d_sqlite_exec_changes(db,
                               &changes,
                               "INSERT INTO people(email, last) VALUES "
                               "('c@x', 'Jones'), ('d@x', 'Jones'), "
                               "('e@x', 'Smith')") == SQLITE_OK) &&
        (changes == 3));
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_exec_changes(db,
                               &changes,
                               "UPDATE people SET first = 'X' "
                               "WHERE last = 'Jones';"
                               "UPDATE people SET first = 'Y' "
                               "WHERE last = 'Smith'") == SQLITE_OK) &&
        (changes == 3));

    // a trigger's rows go uncounted
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db,
                      "CREATE TRIGGER count_people AFTER DELETE ON people "
                      "BEGIN INSERT INTO audit VALUES (old.id); "
                      "INSERT INTO audit VALUES (old.id); END") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_exec_changes(db,
                               &changes,
                               "DELETE FROM people WHERE last = 'Jones'") ==
         SQLITE_OK) &&
        (changes == 2));
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM audit")
                         == 5);

    // a statement that changes nothing
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_exec_changes(db, &changes, "SELECT * FROM people") ==
         SQLITE_OK) &&
        (changes == 3));
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_exec_changes(db,
                               &changes,
                               "UPDATE people SET first = 'Z';"
                               "CREATE TABLE later(a)") == SQLITE_OK) &&
        (changes == 0));
    D_TESTS_SQLITE_CHECK(d_sqlite_changes(db) == 3);
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_exec_changes(db,
                               &changes,
                               "UPDATE people SET first = 'Q' WHERE 0") ==
         SQLITE_OK) &&
        (changes == 0));
    changes = -1;
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_exec_changes(db, &changes, " -- nothing\n;;") ==
         SQLITE_OK) &&
        (changes == 0));
    changes = -1;
    D_TESTS_SQLITE_CHECK( (d_sqlite_exec_changes(db, &changes, "") ==
                           SQLITE_OK) &&
                          (changes == 0) );

    // RETURNING changes rows and returns them: the change counts
    if (d_sqlite_supports(D_SQLITE_FEATURE_RETURNING))
    {
        D_TESTS_SQLITE_CHECK(
            (d_sqlite_exec_changes(db,
                                   &changes,
                                   "UPDATE people SET first = 'R' "
                                   "WHERE last = 'Smith' RETURNING id") ==
             SQLITE_OK) &&
            (changes == 3));
    }

    // a failure: what ran before it stays, and no count is written
    changes = -1;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec_changes(db,
                              &changes,
                              "INSERT INTO later VALUES (1);"
                              "INSERT INTO nowhere VALUES (1);"
                              "INSERT INTO later VALUES (2)") ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(changes == -1);
    D_TESTS_SQLITE_CHECK(strstr(d_sqlite_error_message(db), "nowhere") !=
                         NULL);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM later")
                         == 1);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec_changes(db, &changes, "SELEC 1") == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(changes == -1);

    // both dialects
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_execf_changes(db,
                                &changes,
                                "UPDATE %I SET first = %Q WHERE email = %Q",
                                "people",
                                "it's",
                                "a@x") == SQLITE_OK) &&
        (changes == 1));
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_execf_changes(db,
                                &changes,
                                "DELETE FROM \"%w\" WHERE first = '%q'",
                                "people",
                                "it's") == SQLITE_OK) &&
        (changes == 1));
    changes = -1;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_execf_changes(db,
                               &changes,
                               "UPDATE %I SET first = %g",
                               "people",
                               NAN) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(changes == -1);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_rows_values
  The values a query returns.
  Tests the following:
  - each value as SQLite stored it, whatever its column's type says: the
    extreme integers, a REAL to the last bit, text with a NUL inside, bytes
    with NULs, NULL
  - empty text and an empty BLOB point at a terminator, never NULL
  - an expression's column is named as written, or by its alias, and has no
    type
  - a statement that returns no rows gives its columns and none of them; one
    with no columns gives empty rows, which d_db_rows_free accepts
  - RETURNING rows, where SQLite has RETURNING
  - both dialects of the `f` form
*/
static void
d_tests_sqlite_rows_values(
    void
)
{
    sqlite3* const   db   = d_tests_sqlite_open_values();
    struct d_db_rows rows = { 0u, 0u, NULL, NULL };

    printf("sqlite_query: rows\n");

    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db,
                            &rows,
                            "SELECT 9223372036854775807, "
                            "-9223372036854775807 - 1, 0.1, "
                            "CAST(x'610062' AS TEXT), x'00ff00', NULL, '', "
                            "x'', 1 + 1 AS two") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 9u) &&
                          (rows.row_count == 1u) );

    // values as stored
    if (rows.row_count == 1u)
    {
        const struct d_db_value* const v = rows.values;

        D_TESTS_SQLITE_CHECK( (v[0].type == D_DB_VALUE_INTEGER) &&
                              (v[0].as.integer == INT64_MAX) );
        D_TESTS_SQLITE_CHECK( (v[1].type == D_DB_VALUE_INTEGER) &&
                              (v[1].as.integer == INT64_MIN) );
        D_TESTS_SQLITE_CHECK( (v[2].type == D_DB_VALUE_REAL) &&
                              (v[2].as.real == 0.1) );
        D_TESTS_SQLITE_CHECK( (v[3].type == D_DB_VALUE_TEXT)  &&
                              (v[3].size == 3u)               &&
                              (memcmp(v[3].as.text, "a\0b", 4u) == 0) );
        D_TESTS_SQLITE_CHECK( (v[4].type == D_DB_VALUE_BLOB) &&
                              (v[4].size == 3u)              &&
                              (memcmp(v[4].as.blob, "\0\xff\0", 3u) == 0) );
        D_TESTS_SQLITE_CHECK( (v[5].type == D_DB_VALUE_NULL) &&
                              (v[5].size == 0u) );
        D_TESTS_SQLITE_CHECK( (v[6].type == D_DB_VALUE_TEXT) &&
                              (v[6].size == 0u)              &&
                              (v[6].as.text)                 &&
                              (v[6].as.text[0] == '\0') );
        D_TESTS_SQLITE_CHECK( (v[7].type == D_DB_VALUE_BLOB) &&
                              (v[7].size == 0u)              &&
                              (v[7].as.blob) );
        D_TESTS_SQLITE_CHECK( (v[8].type == D_DB_VALUE_INTEGER) &&
                              (v[8].as.integer == 2) );
    }

    // expressions: named as written or aliased, untyped
    D_TESTS_SQLITE_CHECK(strcmp(rows.columns[8].name, "two") == 0);
    D_TESTS_SQLITE_CHECK(strcmp(rows.columns[2].name, "0.1") == 0);
    D_TESTS_SQLITE_CHECK( (rows.columns[8].type == D_DB_TYPE_NONE) &&
                          (rows.columns[8].type_name)              &&
                          (strcmp(rows.columns[8].type_name, "") == 0) );
    d_db_rows_free(&rows);

    // a column's declared type does not convert what it holds
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db,
                      "CREATE TABLE mixed(n INTEGER);"
                      "INSERT INTO mixed VALUES (1), ('abc'), (2.5), "
                      "(x'01'), (NULL)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db, &rows, "SELECT n FROM mixed ORDER BY rowid")
        == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.row_count == 5u)                       &&
                          (rows.columns[0].type == D_DB_TYPE_BIG_INTEGER) &&
                          (rows.values[0].type == D_DB_VALUE_INTEGER)  &&
                          (rows.values[1].type == D_DB_VALUE_TEXT)     &&
                          (rows.values[2].type == D_DB_VALUE_REAL)     &&
                          (rows.values[3].type == D_DB_VALUE_BLOB)     &&
                          (rows.values[4].type == D_DB_VALUE_NULL) );
    d_db_rows_free(&rows);

    // no rows: the columns alone
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db,
                            &rows,
                            "SELECT a, b FROM \"we\"\"ird\" WHERE 0") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 2u) &&
                          (rows.row_count == 0u)    &&
                          (rows.columns)            &&
                          (rows.values == NULL)     &&
                          (strcmp(rows.columns[1].name, "b") == 0) );
    d_db_rows_free(&rows);

    // no columns: empty
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db,
                            &rows,
                            "INSERT INTO \"we\"\"ird\" VALUES (4, 'y')") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 0u) &&
                          (rows.row_count == 0u)    &&
                          (rows.columns == NULL)    &&
                          (rows.values == NULL) );
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT count(*) FROM \"we\"\"ird\"") == 4);
    d_db_rows_free(&rows);

    // RETURNING: the change, and its rows
    if (d_sqlite_supports(D_SQLITE_FEATURE_RETURNING))
    {
        D_TESTS_SQLITE_CHECK(
            d_sqlite_query_rows(db,
                                &rows,
                                "UPDATE \"we\"\"ird\" SET a = a + 0 "
                                "WHERE a IN (1, 4) RETURNING a, b") ==
            SQLITE_OK);
        D_TESTS_SQLITE_CHECK( (rows.column_count == 2u) &&
                              (rows.row_count == 2u) );
        d_db_rows_free(&rows);
    }

    // both dialects
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rowsf(db,
                             &rows,
                             "SELECT b FROM %I WHERE b = %Q OR a = %g",
                             "we\"ird",
                             "x",
                             0.1234567890123) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.row_count == 1u) &&
                          (strcmp(rows.values[0].as.text, "x") == 0) );
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rowsf(db,
                             &rows,
                             "SELECT a FROM \"%w\" WHERE b = '%q'",
                             "we\"ird",
                             "it's") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.row_count == 1u) &&
                          (rows.values[0].as.integer == 3) );
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_rows_types
  The types the columns of a table's rows are read as.
  Tests the following:
  - every name sqlite_query.h tabulates, matched whole in any case, a size
    in parentheses aside; type_name the declared type as written
  - names not tabulated by SQLite's affinity rules, in its order: FLOATING
    POINT an integer type, TIMESTAMP WITH TIME ZONE a decimal one
  - no declared type, and ANY, read as NONE
*/
static void
d_tests_sqlite_rows_types(
    void
)
{
    static const struct
    {
        const char*    declared;  // the declared type, as written
        enum d_db_type type;      // what the column reads as
    } expected[] =
    {
        { "BOOLEAN",                  D_DB_TYPE_BOOLEAN },
        { "bool",                     D_DB_TYPE_BOOLEAN },
        { "TINYINT",                  D_DB_TYPE_INTEGER },
        { "SmallInt",                 D_DB_TYPE_INTEGER },
        { "MEDIUMINT",                D_DB_TYPE_INTEGER },
        { "INT2",                     D_DB_TYPE_INTEGER },
        { "INT",                      D_DB_TYPE_BIG_INTEGER },
        { "INTEGER",                  D_DB_TYPE_BIG_INTEGER },
        { "BIGINT",                   D_DB_TYPE_BIG_INTEGER },
        { "INT8",                     D_DB_TYPE_BIG_INTEGER },
        { "UNSIGNED BIG INT",         D_DB_TYPE_BIG_INTEGER },
        { "FLOATING POINT",           D_DB_TYPE_BIG_INTEGER },
        { "REAL",                     D_DB_TYPE_FLOATING_POINT },
        { "DOUBLE",                   D_DB_TYPE_FLOATING_POINT },
        { "DOUBLE PRECISION",         D_DB_TYPE_FLOATING_POINT },
        { "FLOAT",                    D_DB_TYPE_FLOATING_POINT },
        { "NUMERIC",                  D_DB_TYPE_DECIMAL },
        { "DECIMAL(10, 2)",           D_DB_TYPE_DECIMAL },
        { "MONEY",                    D_DB_TYPE_DECIMAL },
        { "TIMESTAMP WITH TIME ZONE", D_DB_TYPE_DECIMAL },
        { "TEXT",                     D_DB_TYPE_STRING },
        { "CHARACTER(20)",            D_DB_TYPE_STRING },
        { "VARCHAR(255)",             D_DB_TYPE_STRING },
        { "NVARCHAR(100)",            D_DB_TYPE_STRING },
        { "CLOB",                     D_DB_TYPE_STRING },
        { "BLOB",                     D_DB_TYPE_BINARY },
        { "DATE",                     D_DB_TYPE_DATE },
        { "TIME",                     D_DB_TYPE_TIME },
        { "DATETIME",                 D_DB_TYPE_DATETIME },
        { "TIMESTAMP(3)",             D_DB_TYPE_TIMESTAMP },
        { "JSON",                     D_DB_TYPE_JSON },
        { "jsonb",                    D_DB_TYPE_JSON },
        { "XML",                      D_DB_TYPE_XML },
        { "UUID",                     D_DB_TYPE_UUID },
        { "GUID",                     D_DB_TYPE_UUID },
        { "ANY",                      D_DB_TYPE_NONE },
        { "",                         D_DB_TYPE_NONE }
    };
    const size_t     count = sizeof(expected) / sizeof(expected[0]);
    sqlite3*         db    = NULL;
    struct d_db_rows rows  = { 0u, 0u, NULL, NULL };

    printf("sqlite_query: column types\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);

    for (size_t i = 0u; i < count; ++i)
    {
        D_TESTS_SQLITE_CHECK(
            d_sqlite_execf(db,
                           "CREATE TABLE \"t%d\"(c %s)",
                           (int)i,
                           expected[i].declared) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_query_rowsf(db,
                                 &rows,
                                 "SELECT c FROM \"t%d\"",
                                 (int)i) == SQLITE_OK);

        const bool typed = ( (rows.column_count == 1u)                 &&
                             (rows.columns[0].type == expected[i].type) );

        // say which declared type was misread, beyond the line
        if (!typed)
        {
            printf("  declared \"%s\" read as %s\n",
                   expected[i].declared,
                   (rows.column_count == 1u)
                       ? d_db_type_name(rows.columns[0].type)
                       : "nothing");
        }

        D_TESTS_SQLITE_CHECK(typed);
        D_TESTS_SQLITE_CHECK( (rows.column_count == 1u) &&
                              (rows.columns[0].type_name) &&
                              (strcmp(rows.columns[0].type_name,
                                      expected[i].declared) == 0) );
        d_db_rows_free(&rows);
    }

    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_rows_statements
  What d_sqlite_query_rows takes as a statement, and what a failure leaves.
  Tests the following:
  - semicolons, whitespace and comments around one statement are not a
    second; a block comment left open to the end is not either
  - a second statement is SQLITE_MISUSE before anything runs, even after a
    statement that changes rows; so is text holding none
  - a statement that fails to compile is SQLite's code and message; one that
    fails as it runs, past its first row, too, and the rows are left alone
*/
static void
d_tests_sqlite_rows_statements(
    void
)
{
    sqlite3* const   db     = d_tests_sqlite_open_values();
    struct d_db_rows rows   = { 0u, 0u, NULL, NULL };
    struct d_db_rows marker = { 7u, 7u, NULL, NULL };

    printf("sqlite_query: one statement\n");

    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db,
                            &rows,
                            " ;; SELECT a FROM \"we\"\"ird\" ;; -- done\n"
                            " /* and */ ; /* left open") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(rows.row_count == 3u);
    d_db_rows_free(&rows);

    // more than one statement, or none: nothing runs
    rows = marker;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db,
                            &rows,
                            "INSERT INTO \"we\"\"ird\" VALUES (9, 'z'); "
                            "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT count(*) FROM \"we\"\"ird\"") == 3);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db, &rows, "SELECT 1; SELECT 2") ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db, &rows, "SELECT 1; x") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_query_rows(db, &rows, "") ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db, &rows, " ; -- nothing\n") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rowsf(db, &rows, "SELECT %d; SELECT 2", 1) ==
        SQLITE_MISUSE);

    // failures, compiling and running
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db, &rows, "SELECT * FROM nowhere") ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(strstr(d_sqlite_error_message(db), "nowhere") !=
                         NULL);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db,
                            &rows,
                            "SELECT abs(-9223372036854775806 - a) "
                            "FROM \"we\"\"ird\" ORDER BY a") == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rowsf(db,
                             &rows,
                             "SELECT %I FROM %I",
                             (const char*)NULL,
                             "we\"ird") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 7u) &&
                          (rows.row_count == 7u)    &&
                          (rows.columns == NULL)    &&
                          (rows.values == NULL) );
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_quoting
  Quoting names and values.
  Tests the following:
  - a name's double quotes doubled, and an empty name quoted; NULL refused
  - a value's single quotes doubled; NULL as the keyword
*/
static void
d_tests_sqlite_quoting(
    void
)
{
    char* text = NULL;

    printf("sqlite_query: quoting\n");

    text = d_sqlite_quote_identifier("a\"b");
    D_TESTS_SQLITE_CHECK( (text) && (strcmp(text, "\"a\"\"b\"") == 0) );
    d_db_free(text);
    text = d_sqlite_quote_identifier("");
    D_TESTS_SQLITE_CHECK( (text) && (strcmp(text, "\"\"") == 0) );
    d_db_free(text);
    D_TESTS_SQLITE_CHECK(d_sqlite_quote_identifier(NULL) == NULL);
    text = d_sqlite_quote_literal("it's");
    D_TESTS_SQLITE_CHECK( (text) && (strcmp(text, "'it''s'") == 0) );
    d_db_free(text);
    text = d_sqlite_quote_literal(NULL);
    D_TESTS_SQLITE_CHECK( (text) && (strcmp(text, "NULL") == 0) );
    d_db_free(text);

    return;
}

/*
d_tests_sqlite_query_arguments
  sqlite_query.h's answers to arguments that cannot be right.
  Tests the following:
  - a NULL handle, statement, format or value pointer is SQLITE_MISUSE, for
    every running, counting, single-value and rows function
*/
static void
d_tests_sqlite_query_arguments(
    void
)
{
    sqlite3*         db     = NULL;
    int64_t          number = 0;
    double           real   = 0.0;
    char*            text   = NULL;
    struct d_db_rows rows   = { 0u, 0u, NULL, NULL };

    printf("sqlite_query: arguments\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(NULL, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(db, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_execf(db, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_int64(db, NULL, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_int64f(NULL, &number, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_double(db, &real, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_doublef(db, NULL, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_text(NULL, &text, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_textf(db, &text, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec_changes(NULL, &number, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec_changes(db, NULL, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec_changes(db, &number, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_execf_changes(NULL, &number, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_execf_changes(db, NULL, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_execf_changes(db, &number, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(NULL, &rows, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db, NULL, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_query_rows(db, &rows, NULL) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rowsf(NULL, &rows, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rowsf(db, NULL, "SELECT 1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_query_rowsf(db, &rows, NULL) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK( (rows.columns == NULL) &&
                          (rows.values == NULL) );
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

//==============================================================================
// sqlite_statement.h
//==============================================================================

/*
d_tests_sqlite_preparing
  Preparing, describing and finalizing statements.
  Tests the following:
  - parameters of every spelling SQLite reads, counted and found by name
    with their prefixes; a name not there, or written without its prefix,
    is D_DB_NOT_FOUND and leaves the index alone; ?NNN raises the count
  - the statement is SQLite's own, and knows its connection
  - a statement that does not compile is SQLite's code and message, and
    leaves NULL; a second statement, or none, is SQLITE_MISUSE
  - preparef in both dialects, and refusing what the shared dialect refuses
  - finalize accepts NULL, and succeeds after a run that failed
*/
static void
d_tests_sqlite_preparing(
    void
)
{
    sqlite3* const       db        = d_tests_sqlite_open_values();
    struct sqlite3_stmt* statement = NULL;
    size_t               index     = 0u;
    int64_t              changes   = 0;

    printf("sqlite_statement: preparing\n");

    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db,
                         &statement,
                         "SELECT ?, :name, @at, $dollar") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_parameter_count(statement) == 4u);
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_parameter_index(statement, ":name", &index) == SQLITE_OK) &&
        (index == 2u));
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_parameter_index(statement, "@at", &index) == SQLITE_OK) &&
        (index == 3u));
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_parameter_index(statement, "$dollar", &index) ==
         SQLITE_OK) &&
        (index == 4u));
    index = 99u;
    D_TESTS_SQLITE_CHECK(d_sqlite_parameter_index(statement, "name", &index)
                         == D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_parameter_index(statement, ":nobody", &index) ==
        D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(index == 99u);
    D_TESTS_SQLITE_CHECK(d_sqlite_statement_connection(statement) == db);
    D_TESTS_SQLITE_CHECK(sqlite3_bind_parameter_count(statement) == 4);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(statement) == SQLITE_OK);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db, &statement, "SELECT ?5, ?2") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_parameter_count(statement) == 5u);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(statement) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db, &statement, " ; SELECT 1 ; -- one\n") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_parameter_count(statement) == 0u);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(statement) == SQLITE_OK);

    // failures leave NULL behind
    statement = (struct sqlite3_stmt*)&index;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db, &statement, "SELECT * FROM nowhere") ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(statement == NULL);
    D_TESTS_SQLITE_CHECK(strstr(d_sqlite_error_message(db), "nowhere") !=
                         NULL);
    statement = (struct sqlite3_stmt*)&index;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db, &statement, "SELECT 1; SELECT 2") ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(statement == NULL);
    statement = (struct sqlite3_stmt*)&index;
    D_TESTS_SQLITE_CHECK(d_sqlite_prepare(db, &statement, "-- none") ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(statement == NULL);

    // both dialects, and a refusal
    D_TESTS_SQLITE_CHECK(
        d_sqlite_preparef(db,
                          &statement,
                          "SELECT b FROM %I WHERE a = ?",
                          "we\"ird") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_parameter_count(statement) == 1u);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(statement) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_preparef(db,
                          &statement,
                          "SELECT b FROM \"%w\" WHERE b = '%q' AND a = ?",
                          "we\"ird",
                          "it's") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_parameter_count(statement) == 1u);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(statement) == SQLITE_OK);
    statement = (struct sqlite3_stmt*)&index;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_preparef(db,
                          &statement,
                          "SELECT %g FROM %I",
                          NAN,
                          "we\"ird") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(statement == NULL);

    // finalizing after a failed run, and NULL
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db,
                         &statement,
                         "SELECT abs(-9223372036854775807 - 1)") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_update(statement, &changes) ==
                         SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(statement) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(NULL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_binding
  Giving parameters values.
  Tests the following:
  - each bind stores its value as SQLite's storage class says: NULL, a
    boolean as 1 or 0, the extreme integers, a REAL, text, bytes with NULs
  - text and bytes are copied: changing the caller's buffer after the bind
    changes nothing
  - a NaN stores NULL; NULL text and NULL bytes bind NULL; a pointer to no
    bytes binds the empty BLOB
  - a parameter numbered 0, past the last, or past int's range is
    SQLITE_RANGE; text or bytes past SQLite's length limit SQLITE_TOOBIG
  - clearing makes every parameter NULL again
*/
static void
d_tests_sqlite_binding(
    void
)
{
    sqlite3*             db        = NULL;
    struct sqlite3_stmt* statement = NULL;
    struct d_db_rows     rows      = { 0u, 0u, NULL, NULL };
    char                 text[]    = "it's";
    unsigned char        bytes[]   = { 0x00, 0x01, 0x00 };
    int64_t              changes   = 0;

    printf("sqlite_statement: binding\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "CREATE TABLE v(n, t, f, i, x, r, s, b)") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db,
                         &statement,
                         "INSERT INTO v VALUES (?, ?, ?, ?, ?, ?, ?, ?)") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_null(statement, 1u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_bool(statement, 2u, true) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_bool(statement, 3u, false) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(statement, 4u, INT64_MIN) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(statement, 5u, INT64_MAX) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_double(statement, 6u, 0.1) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_text(statement, 7u, text) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_bind_blob(statement, 8u, bytes, sizeof(bytes)) ==
        SQLITE_OK);

    // the copies outlive the caller's buffers
    text[0]  = 'X';
    bytes[1] = 0xff;
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_execute_update(statement, &changes) == SQLITE_OK) &&
        (changes == 1));

    // what NULL, NaN and no bytes bind
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_double(statement, 6u, NAN) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_text(statement, 7u, NULL) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_blob(statement, 8u, bytes, 0u) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_blob(statement, 5u, NULL, 3u) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_update(statement, NULL) ==
                         SQLITE_OK);

    // cleared: every value NULL
    D_TESTS_SQLITE_CHECK(d_sqlite_clear_bindings(statement) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_update(statement, NULL) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_query_rows(db,
                            &rows,
                            "SELECT typeof(n), t, f, i, x, r, s, b, "
                            "typeof(b), typeof(r), typeof(x) "
                            "FROM v ORDER BY rowid") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(rows.row_count == 3u);

    // the first row: each storage class
    if (rows.row_count == 3u)
    {
        const struct d_db_value* const v = rows.values;

        D_TESTS_SQLITE_CHECK(strcmp(v[0].as.text, "null") == 0);
        D_TESTS_SQLITE_CHECK( (v[1].type == D_DB_VALUE_INTEGER) &&
                              (v[1].as.integer == 1) );
        D_TESTS_SQLITE_CHECK( (v[2].type == D_DB_VALUE_INTEGER) &&
                              (v[2].as.integer == 0) );
        D_TESTS_SQLITE_CHECK(v[3].as.integer == INT64_MIN);
        D_TESTS_SQLITE_CHECK(v[4].as.integer == INT64_MAX);
        D_TESTS_SQLITE_CHECK( (v[5].type == D_DB_VALUE_REAL) &&
                              (v[5].as.real == 0.1) );
        D_TESTS_SQLITE_CHECK( (v[6].type == D_DB_VALUE_TEXT) &&
                              (strcmp(v[6].as.text, "it's") == 0) );
        D_TESTS_SQLITE_CHECK( (v[7].type == D_DB_VALUE_BLOB) &&
                              (v[7].size == 3u)              &&
                              (memcmp(v[7].as.blob, "\0\1\0", 3u) == 0) );

        // the second: NaN, NULL text and NULL bytes as NULL; no bytes empty
        D_TESTS_SQLITE_CHECK(v[11 + 5].type == D_DB_VALUE_NULL);
        D_TESTS_SQLITE_CHECK(v[11 + 6].type == D_DB_VALUE_NULL);
        D_TESTS_SQLITE_CHECK( (v[11 + 7].type == D_DB_VALUE_BLOB) &&
                              (v[11 + 7].size == 0u) );
        D_TESTS_SQLITE_CHECK(strcmp(v[11 + 8].as.text, "blob") == 0);
        D_TESTS_SQLITE_CHECK(strcmp(v[11 + 9].as.text, "null") == 0);
        D_TESTS_SQLITE_CHECK(strcmp(v[11 + 10].as.text, "null") == 0);

        // the third: all NULL
        D_TESTS_SQLITE_CHECK( (v[22 + 1].type == D_DB_VALUE_NULL) &&
                              (v[22 + 4].type == D_DB_VALUE_NULL) &&
                              (v[22 + 7].type == D_DB_VALUE_NULL) );
    }

    d_db_rows_free(&rows);

    // numbers a statement does not have
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_null(statement, 0u) == SQLITE_RANGE);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(statement, 9u, 1) ==
                         SQLITE_RANGE);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_text(statement, 0u, "x") ==
                         SQLITE_RANGE);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_blob(statement, 9u, bytes, 1u) ==
                         SQLITE_RANGE);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_bool(statement, 9u, true) ==
                         SQLITE_RANGE);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_double(statement, 9u, 1.0) ==
                         SQLITE_RANGE);

    // a number past int's range, where size_t can hold one
    if ((size_t)-1 > (size_t)INT_MAX)
    {
        D_TESTS_SQLITE_CHECK(
            d_sqlite_bind_int64(statement, (size_t)INT_MAX + 1u, 1) ==
            SQLITE_RANGE);
    }

    // past the length limit, set at the least SQLite allows from 3.46.0
    (void)sqlite3_limit(db, SQLITE_LIMIT_LENGTH, 30);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_bind_text(statement, 7u, "thirty-one characters, no more") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_bind_text(statement, 7u, "thirty-one characters, or more.")
        == SQLITE_TOOBIG);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_bind_blob(statement, 8u, "thirty-one characters, or more.",
                           31u) == SQLITE_TOOBIG);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(statement) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_running
  Running statements, once and again.
  Tests the following:
  - execute_update counts an INSERT's rows; values stay bound between runs
  - a run that fails is SQLite's code, writes no count, and leaves the
    statement ready: the next run, rebound, succeeds
  - execute_update counts a SELECT's rows; execute_query returns them, and
    the same again on a second run
  - execute_query of a statement with no rows gives empty rows, and runs it
  - a run failing past its first row leaves the rows alone; the statement
    runs again afterwards
  - a statement may outlive d_sqlite_close, which waits for it
*/
static void
d_tests_sqlite_running(
    void
)
{
    sqlite3* const       db        = d_tests_sqlite_open_values();
    struct sqlite3_stmt* insert    = NULL;
    struct sqlite3_stmt* select    = NULL;
    struct d_db_rows     rows      = { 0u, 0u, NULL, NULL };
    struct d_db_rows     marker    = { 7u, 7u, NULL, NULL };
    int64_t              changes   = -1;

    printf("sqlite_statement: running\n");

    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "CREATE TABLE k(id INTEGER PRIMARY KEY, v)") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db, &insert, "INSERT INTO k VALUES (?, ?)") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(insert, 1u, 1) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_text(insert, 2u, "one") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_execute_update(insert, &changes) == SQLITE_OK) &&
        (changes == 1));

    // the same values again: the key collides
    changes = -1;
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_update(insert, &changes) ==
                         SQLITE_CONSTRAINT);
    D_TESTS_SQLITE_CHECK(changes == -1);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(insert, 1u, 2) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_execute_update(insert, &changes) == SQLITE_OK) &&
        (changes == 1));
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM k") ==
                         2);

    // a SELECT, counted and collected, twice
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db,
                         &select,
                         "SELECT id, v FROM k WHERE id >= ? ORDER BY id") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(select, 1u, 1) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_execute_update(select, &changes) == SQLITE_OK) &&
        (changes == 2));

    for (int run = 0; run < 2; ++run)
    {
        D_TESTS_SQLITE_CHECK(d_sqlite_execute_query(select, &rows) ==
                             SQLITE_OK);
        D_TESTS_SQLITE_CHECK( (rows.column_count == 2u)            &&
                              (rows.row_count == 2u)               &&
                              (rows.values[2].as.integer == 2)     &&
                              (strcmp(rows.values[1].as.text, "one") == 0) );
        d_db_rows_free(&rows);
    }

    // a statement with no rows
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(insert, 1u, 3) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_query(insert, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 0u) &&
                          (rows.row_count == 0u) );
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM k") ==
                         3);
    d_db_rows_free(&rows);

    // a run failing past its first row, then a good one
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(select) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db,
                         &select,
                         "SELECT abs(? - id) FROM k ORDER BY id") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_bind_int64(select, 1u, -9223372036854775807 + 1) ==
        SQLITE_OK);
    rows = marker;
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_query(select, &rows) ==
                         SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 7u) &&
                          (rows.row_count == 7u) );
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(select, 1u, 0) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_query(select, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(rows.row_count == 3u);
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(select) == SQLITE_OK);

    // closing first: the connection waits for the statement
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(insert) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_cursors
  Rows a few at a time.
  Tests the following:
  - each fetch returns the columns, named and typed as execute_query's are,
    and the next rows, no more than asked for, each value as execute_query
    reads it
  - a short page ends the run, the statement reset: the next fetch begins
    another, from the first row
  - rows that fill their last page exactly end with an empty page, which
    still describes the columns; a query finding no rows describes them too
  - part-way through a run its values cannot change: binding one or clearing
    them is SQLITE_MISUSE, and the run goes on; between runs they rebind
  - between fetches the connection runs other statements, reading and
    writing, and the run goes on where it stopped
  - a statement without rows runs on the first fetch, not on opening; a
    further fetch runs it again
  - execute_query part-way through a run ends it and begins another;
    close_cursor ends one part-way, and the next fetch starts over
  - a step failing part-way through a run is SQLite's code, described on the
    connection, writes no rows, and ends the run; a good run follows
*/
static void
d_tests_sqlite_cursors(
    void
)
{
    sqlite3*             db     = NULL;
    struct sqlite3_stmt* select = NULL;
    struct sqlite3_stmt* insert = NULL;
    struct d_db_rows     all    = { 0u, 0u, NULL, NULL };
    struct d_db_rows     rows   = { 0u, 0u, NULL, NULL };
    struct d_db_rows     marker = { 7u, 7u, NULL, NULL };

    printf("sqlite_statement: rows a few at a time\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db,
                      "CREATE TABLE k(id INTEGER PRIMARY KEY, v TEXT, r REAL, "
                      "b BLOB);"
                      "CREATE TABLE log(n);"
                      "INSERT INTO k VALUES (1, 'a', 1.5, x'00ff');"
                      "INSERT INTO k VALUES (2, NULL, NULL, NULL);"
                      "INSERT INTO k VALUES (3, 'c', -2.25, x'');"
                      "INSERT INTO k VALUES (4, 'it''s', 0.0, x'61');"
                      "INSERT INTO k VALUES (5, '', 3.0, NULL)") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db,
                         &select,
                         "SELECT id, v, r, b FROM k WHERE id >= ? "
                         "ORDER BY id") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(select, 1u, 1) == SQLITE_OK);

    // the rows all at once, to hold the pages to
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_query(select, &all) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (all.column_count == 4u) &&
                          (all.row_count == 5u) );

    // pages of two: 1 and 2, 3 and 4, then 5 alone, which ends the run
    D_TESTS_SQLITE_CHECK(d_sqlite_open_cursor(select) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 2u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 0u, 2u));
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 2u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 2u, 2u));
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(sqlite3_stmt_busy(select));
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 2u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 4u, 1u));
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(!sqlite3_stmt_busy(select));

    // ended, so the next fetch starts over
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 3u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 0u, 3u));
    d_db_rows_free(&rows);

    // part-way through, the values stay; the run goes on
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(select, 1u, 4) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_clear_bindings(select) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 1u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 3u, 1u));
    d_db_rows_free(&rows);

    // other statements between fetches, reading and writing
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM k") ==
                         5);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(db, "INSERT INTO log VALUES (0)") ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 5u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 4u, 1u));
    d_db_rows_free(&rows);

    // rebound between runs: from 2, four rows filling two pages exactly, and
    // then an empty page, which ends the run
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(select, 1u, 2) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_cursor(select) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 2u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 1u, 2u));
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 2u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 3u, 2u));
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(sqlite3_stmt_busy(select));
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 2u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 5u, 0u));
    D_TESTS_SQLITE_CHECK( (rows.columns != NULL) &&
                          (rows.values == NULL) );
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(!sqlite3_stmt_busy(select));

    // no row found: the columns alone
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(select, 1u, 9) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_cursor(select) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 4u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 5u, 0u));
    d_db_rows_free(&rows);

    // a statement without rows runs on the first fetch, and again on another
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db, &insert, "INSERT INTO log VALUES (?)") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(insert, 1u, 1) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_cursor(insert) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM log") ==
                         1);
    rows = marker;
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(insert, 3u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 0u) &&
                          (rows.row_count == 0u)    &&
                          (rows.columns == NULL) );
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM log") ==
                         2);
    D_TESTS_SQLITE_CHECK(!sqlite3_stmt_busy(insert));
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(insert, 3u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM log") ==
                         3);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(insert) == SQLITE_OK);

    // execute_query part-way through ends the run and begins its own
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(select, 1u, 1) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_cursor(select) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 2u, &rows) == SQLITE_OK);
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_query(select, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 0u, 5u));
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(!sqlite3_stmt_busy(select));
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 2u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 0u, 2u));
    d_db_rows_free(&rows);

    // close_cursor part-way, twice: the next fetch starts over
    D_TESTS_SQLITE_CHECK(d_sqlite_close_cursor(select) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!sqlite3_stmt_busy(select));
    D_TESTS_SQLITE_CHECK(d_sqlite_close_cursor(select) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 1u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_same_page(&rows, &all, 0u, 1u));
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(d_sqlite_close_cursor(select) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(select) == SQLITE_OK);

    // a step failing part-way, at the second row: abs() of the lowest
    // integer overflows
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db,
                         &select,
                         "SELECT abs(? - id) FROM k ORDER BY id") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_bind_int64(select, 1u, -9223372036854775807 + 1) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_cursor(select) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 1u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.row_count == 1u) &&
                          (rows.values[0].as.integer ==
                           9223372036854775807) );
    d_db_rows_free(&rows);
    rows = marker;
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 3u, &rows) ==
                         SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 7u) &&
                          (rows.row_count == 7u) );
    D_TESTS_SQLITE_CHECK(
        (d_sqlite_error_message(db)) &&
        (strstr(d_sqlite_error_message(db), "overflow") != NULL));
    D_TESTS_SQLITE_CHECK(!sqlite3_stmt_busy(select));
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(select, 1u, 0) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_cursor(select) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(select, 9u, &rows) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (rows.row_count == 5u) &&
                          (rows.values[4].as.integer == 5) );
    d_db_rows_free(&rows);

    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(select) == SQLITE_OK);
    d_db_rows_free(&all);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_statement_locks
  No lock outlives a run.
  Tests the following:
  - a statement left part-way through its rows by SQLite's own step holds
    a lock that keeps a second connection from writing (the control)
  - after execute_query and execute_update, the second connection writes
  - a run read a few rows at a time holds the lock part-way, as SQLite's own
    step does, until a short page or close_cursor ends it
*/
static void
d_tests_sqlite_statement_locks(
    void
)
{
    char                 path[1024] = "";
    sqlite3*             writer     = NULL;
    struct sqlite3_stmt* statement  = NULL;
    struct d_db_rows     rows       = { 0u, 0u, NULL, NULL };

    printf("sqlite_statement: locks\n");

    d_tests_sqlite_path(path, sizeof(path), "t_sqlite_locks.db");

    sqlite3* const db = d_tests_sqlite_open_file(path);

    D_TESTS_SQLITE_CHECK(d_sqlite_open(path, &writer) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_set_busy_timeout(writer, 0) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db, &statement, "SELECT a FROM t ORDER BY a") ==
        SQLITE_OK);

    // the control: a statement mid-way holds the database
    D_TESTS_SQLITE_CHECK(sqlite3_step(statement) == SQLITE_ROW);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "BEGIN EXCLUSIVE") ==
                         SQLITE_BUSY);
    (void)sqlite3_reset(statement);

    // the layer's runs end reset
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_query(statement, &rows) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(rows.row_count == 3u);
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "BEGIN EXCLUSIVE") ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "COMMIT") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_update(statement, NULL) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "BEGIN EXCLUSIVE") ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "COMMIT") == SQLITE_OK);

    // a run a few rows at a time: held part-way, released by a short page
    D_TESTS_SQLITE_CHECK(d_sqlite_open_cursor(statement) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "BEGIN EXCLUSIVE") ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "COMMIT") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(statement, 1u, &rows) ==
                         SQLITE_OK);
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "BEGIN EXCLUSIVE") ==
                         SQLITE_BUSY);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(statement, 5u, &rows) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(rows.row_count == 2u);
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "BEGIN EXCLUSIVE") ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "COMMIT") == SQLITE_OK);

    // ... and by close_cursor
    D_TESTS_SQLITE_CHECK(d_sqlite_open_cursor(statement) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(statement, 2u, &rows) ==
                         SQLITE_OK);
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "BEGIN EXCLUSIVE") ==
                         SQLITE_BUSY);
    D_TESTS_SQLITE_CHECK(d_sqlite_close_cursor(statement) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "BEGIN EXCLUSIVE") ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(writer, "COMMIT") == SQLITE_OK);

    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(statement) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(writer) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);
    d_tests_sqlite_remove_all(path);

    return;
}

/*
d_tests_sqlite_statement_arguments
  sqlite_statement.h's answers to arguments that cannot be right.
  Tests the following:
  - a NULL connection, statement pointer, text or format is SQLITE_MISUSE,
    the statement left NULL where there is one to leave
  - every bind, clear and run refuses a NULL statement with SQLITE_MISUSE,
    as execute_query refuses NULL rows; the describers answer 0 or NULL
  - the cursor calls refuse a NULL statement, and fetch_rows NULL rows or
    none asked for, with SQLITE_MISUSE, the rows untouched
*/
static void
d_tests_sqlite_statement_arguments(
    void
)
{
    sqlite3*             db        = NULL;
    struct sqlite3_stmt* statement = NULL;
    struct d_db_rows     rows      = { 0u, 0u, NULL, NULL };
    struct d_db_rows     marker    = { 7u, 7u, NULL, NULL };
    size_t               index     = 0u;
    int64_t              changes   = 0;

    printf("sqlite_statement: arguments\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    statement = (struct sqlite3_stmt*)&index;
    D_TESTS_SQLITE_CHECK(d_sqlite_prepare(NULL, &statement, "SELECT 1") ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(statement == NULL);
    D_TESTS_SQLITE_CHECK(d_sqlite_prepare(db, NULL, "SELECT 1") ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_prepare(db, &statement, NULL) ==
                         SQLITE_MISUSE);
    statement = (struct sqlite3_stmt*)&index;
    D_TESTS_SQLITE_CHECK(d_sqlite_preparef(NULL, &statement, "SELECT 1") ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(statement == NULL);
    D_TESTS_SQLITE_CHECK(d_sqlite_preparef(db, NULL, "SELECT 1") ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_preparef(db, &statement, NULL) ==
                         SQLITE_MISUSE);

    D_TESTS_SQLITE_CHECK(d_sqlite_parameter_count(NULL) == 0u);
    D_TESTS_SQLITE_CHECK(d_sqlite_parameter_index(NULL, ":a", &index) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_statement_connection(NULL) == NULL);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_null(NULL, 1u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_bool(NULL, 1u, true) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_int64(NULL, 1u, 1) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_double(NULL, 1u, 1.0) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_text(NULL, 1u, "x") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_bind_blob(NULL, 1u, "x", 1u) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_clear_bindings(NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_update(NULL, &changes) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_query(NULL, &rows) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_cursor(NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(NULL, 1u, &rows) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close_cursor(NULL) == SQLITE_MISUSE);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_prepare(db, &statement, "SELECT :a") == SQLITE_OK);
    rows = marker;
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(statement, 0u, &rows) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK( (rows.column_count == 7u) &&
                          (rows.row_count == 7u) );
    D_TESTS_SQLITE_CHECK(d_sqlite_fetch_rows(statement, 1u, NULL) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_parameter_index(statement, NULL, &index) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_parameter_index(statement, ":a", NULL) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_execute_query(statement, NULL) ==
                         SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_finalize(statement) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

//==============================================================================
// sqlite_database.h
//==============================================================================

/*
d_tests_sqlite_attached
  Attached databases.
  Tests the following:
  - main and temp always exist; a name not attached does not; case is ignored
  - attach, under a name that needs quoting too; attaching a name twice fails
  - list_databases: main first, every attached name, NULL-terminated
  - detach; detaching what is not attached, or main, fails
*/
static void
d_tests_sqlite_attached(
    void
)
{
    char                  aux_path[1024] = "";
    sqlite3*              db             = NULL;
    struct d_db_name_list names          = { 0u, NULL };
    bool                  exists         = true;

    printf("sqlite_database: attached databases\n");

    d_tests_sqlite_path(aux_path, sizeof(aux_path), "t_sqlite_aux.db");
    d_tests_sqlite_remove_all(aux_path);

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_has_database(db, "main"));
    D_TESTS_SQLITE_CHECK(d_sqlite_has_database(db, "temp"));
    D_TESTS_SQLITE_CHECK(d_sqlite_has_database(db, "MAIN"));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_database(db, "aux"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_database_s(db, "aux", &exists) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!exists);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_attach_database(db, aux_path, "aux") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_has_database(db, "aux"));
    D_TESTS_SQLITE_CHECK(d_sqlite_has_database(db, "Aux"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_attach_database(db, ":memory:", "we\"ird") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_has_database(db, "we\"ird"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_attach_database(db, aux_path, "aux") == SQLITE_ERROR);

    D_TESTS_SQLITE_CHECK(d_sqlite_list_databases(db, &names) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (names.count >= 3u) &&
                          (names.names[names.count] == NULL) );
    D_TESTS_SQLITE_CHECK(strcmp(names.names[0], "main") == 0);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_has_name(&names, "aux"));
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_has_name(&names, "we\"ird"));
    d_db_name_list_free(&names);
    D_TESTS_SQLITE_CHECK(names.names == NULL);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_detach_database(db, "we\"ird") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_detach_database(db, "aux") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_database(db, "aux"));
    D_TESTS_SQLITE_CHECK(d_sqlite_detach_database(db, "aux") == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_detach_database(db, "main") == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    d_tests_sqlite_remove_all(aux_path);

    return;
}

/*
d_tests_sqlite_database_files
  Database files.
  Tests the following:
  - create_database_file: new, existing with and without D_DB_IF_NOT_EXISTS,
    and a file that is not a database; the new file is recognised
  - delete_database_file: with its three companions, and missing with and
    without D_DB_IF_EXISTS
  - is_database_file: a database, an empty file, text, ":memory:"
*/
static void
d_tests_sqlite_database_files(
    void
)
{
    static const char* const suffixes[] = { "-journal", "-wal", "-shm" };
    char                     path[1024]      = "";
    char                     companion[1100] = "";

    printf("sqlite_database: database files\n");

    d_tests_sqlite_path(path, sizeof(path), "t_sqlite_database.db");
    d_tests_sqlite_remove_all(path);

    D_TESTS_SQLITE_CHECK(!d_sqlite_is_database_file(path));
    D_TESTS_SQLITE_CHECK(d_sqlite_create_database_file(path, 0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_is_database_file(path));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_database_file(path, 0u) == D_DB_EXISTS);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_database_file(path, D_DB_IF_NOT_EXISTS) == SQLITE_OK);

    for (size_t i = 0u; i < (sizeof(suffixes) / sizeof(*suffixes)); ++i)
    {
        (void)snprintf(companion, sizeof(companion), "%s%s", path,
                       suffixes[i]);
        d_tests_sqlite_write_file(companion, "x");
    }

    D_TESTS_SQLITE_CHECK(d_sqlite_delete_database_file(path, 0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_tests_sqlite_exists(path));

    for (size_t i = 0u; i < (sizeof(suffixes) / sizeof(*suffixes)); ++i)
    {
        (void)snprintf(companion, sizeof(companion), "%s%s", path,
                       suffixes[i]);
        D_TESTS_SQLITE_CHECK(!d_tests_sqlite_exists(companion));
    }

    D_TESTS_SQLITE_CHECK(
        d_sqlite_delete_database_file(path, 0u) == D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_delete_database_file(path, D_DB_IF_EXISTS) == SQLITE_OK);

    d_tests_sqlite_write_file(path, "");
    D_TESTS_SQLITE_CHECK(d_sqlite_is_database_file(path));
    d_tests_sqlite_write_file(path, "no database, just some text in a file");
    D_TESTS_SQLITE_CHECK(!d_sqlite_is_database_file(path));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_database_file(path, D_DB_IF_NOT_EXISTS) ==
        SQLITE_NOTADB);
    D_TESTS_SQLITE_CHECK(!d_sqlite_is_database_file(":memory:"));

    d_tests_sqlite_remove_all(path);

    return;
}

/*
d_tests_sqlite_serialization
  Databases as bytes.
  Tests the following:
  - supports says what the library has: from 3.36.0 unless built without
    it, from 3.23.0 where built with it
  - serialize: a database's bytes, a whole number of pages, the file's
    header first; "temp", whatever it holds; an attached file; no such
    database, D_DB_NOT_FOUND
  - deserialize: the bytes as "main" of another connection, its rows there,
    and writes that grow it; read-only, which refuses writes; into an
    attached database; an empty database; bytes that are no database,
    SQLITE_NOTADB when read; "temp", SQLITE_ERROR
  - where the library cannot, both D_DB_UNSUPPORTED
  - misuse: a NULL connection or out-parameter, NULL bytes of a size
*/
static void
d_tests_sqlite_serialization(
    void
)
{
    const bool supported =
        ( (d_tests_sqlite_at_least(3, 36, 0)) &&
          (sqlite3_compileoption_used("OMIT_DESERIALIZE") == 0) ) ||
        ( (d_tests_sqlite_at_least(3, 23, 0)) &&
          (sqlite3_compileoption_used("ENABLE_DESERIALIZE") != 0) );
    char     path[1024] = "";
    sqlite3* db         = NULL;
    sqlite3* other      = NULL;
    void*    bytes      = NULL;
    size_t   size       = 0u;

    printf("sqlite_database: databases as bytes\n");

    D_TESTS_SQLITE_CHECK(
        d_sqlite_supports(D_SQLITE_FEATURE_SERIALIZE) == supported);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_serialize(NULL, NULL, &bytes, &size) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_serialize(db, NULL, NULL, &size) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_serialize(db, NULL, &bytes, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_deserialize(NULL, NULL, NULL, 0u, false) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_deserialize(db, NULL, NULL, 8u, false) == SQLITE_MISUSE);

    // a library without it
    if (!supported)
    {
        D_TESTS_SQLITE_CHECK(d_sqlite_serialize(db, NULL, &bytes, &size) ==
                             D_DB_UNSUPPORTED);
        D_TESTS_SQLITE_CHECK(d_sqlite_deserialize(db, NULL, "x", 1u, false) ==
                             D_DB_UNSUPPORTED);
        D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

        return;
    }

    D_TESTS_SQLITE_CHECK(
        sqlite3_exec(db,
                     "CREATE TABLE t (a INTEGER PRIMARY KEY, b TEXT); "
                     "INSERT INTO t VALUES (1, 'one'), (2, 'two');",
                     NULL,
                     NULL,
                     NULL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_serialize(db, NULL, &bytes, &size) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (bytes != NULL) &&
                          (size >= 512u) &&
                          ((size % (size_t)d_tests_sqlite_int(
                                      db, "PRAGMA page_size")) == 0u) &&
                          (memcmp(bytes, "SQLite format 3", 16u) == 0) );

    // the bytes as another connection's main, which grows as it is written
    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&other) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_deserialize(other, NULL, bytes, size, false) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(other, "SELECT a FROM t WHERE b = 'two'") == 2);
    D_TESTS_SQLITE_CHECK(
        sqlite3_exec(other,
                     "WITH RECURSIVE n(i) AS (SELECT 3 UNION ALL SELECT i + 1 "
                     "FROM n WHERE i < 2000) INSERT INTO t SELECT i, "
                     "printf('%0200d', i) FROM n",
                     NULL,
                     NULL,
                     NULL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(other, "SELECT count(*) FROM t") == 2000);

    // read-only, into an attached database
    D_TESTS_SQLITE_CHECK(
        d_sqlite_attach_database(other, ":memory:", "aux") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_deserialize(other, "aux", bytes, size, true) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(other, "SELECT count(*) FROM aux.t") == 2);
    D_TESTS_SQLITE_CHECK(
        sqlite3_exec(other,
                     "INSERT INTO aux.t VALUES (9, 'nine')",
                     NULL,
                     NULL,
                     NULL) == SQLITE_READONLY);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_deserialize(other, "temp", bytes, size, false) ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_deserialize(other, "nope", bytes, size, false) ==
        SQLITE_ERROR);
    d_db_free(bytes);
    bytes = NULL;

    // an empty database; bytes that are none
    D_TESTS_SQLITE_CHECK(
        d_sqlite_deserialize(other, "aux", NULL, 0u, false) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(other,
                           "SELECT count(*) FROM aux.sqlite_master") == 0);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_deserialize(other,
                             "aux",
                             "plain text, no database here at all",
                             35u,
                             false) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        sqlite3_exec(other,
                     "SELECT count(*) FROM aux.sqlite_master",
                     NULL,
                     NULL,
                     NULL) == SQLITE_NOTADB);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(other) == SQLITE_OK);

    // temp, a file attached, and none such
    D_TESTS_SQLITE_CHECK(
        d_sqlite_serialize(db, "temp", &bytes, &size) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (size == 0u) ||
                          (memcmp(bytes, "SQLite format 3", 16u) == 0) );
    d_db_free(bytes);
    bytes = NULL;
    d_tests_sqlite_path(path, sizeof(path), "t_sqlite_serialized.db");
    d_tests_sqlite_remove_all(path);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_attach_database(db, path, "disk") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        sqlite3_exec(db,
                     "CREATE TABLE disk.d (x); INSERT INTO disk.d VALUES (5)",
                     NULL,
                     NULL,
                     NULL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_serialize(db, "disk", &bytes, &size) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&other) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_deserialize(other, NULL, bytes, size, false) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(other, "SELECT x FROM d") == 5);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(other) == SQLITE_OK);
    d_db_free(bytes);
    bytes = NULL;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_serialize(db, "nope", &bytes, &size) == D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);
    d_tests_sqlite_remove_all(path);

    return;
}

/*
d_tests_sqlite_database_arguments
  sqlite_database.h's answers to arguments that cannot be right.
  Tests the following:
  - NULL and empty names, paths and handles; flags a function does not take
  - has_database_s answers false when it fails, where it can
*/
static void
d_tests_sqlite_database_arguments(
    void
)
{
    sqlite3* db     = NULL;
    bool     exists = true;

    printf("sqlite_database: arguments\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_database(NULL, "main"));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_database(db, ""));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_database_s(NULL, "main", &exists) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(!exists);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_database_s(db, NULL, &exists) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_database_s(db, "main", NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_attach_database(db, NULL, "x") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_attach_database(db, "unused.db", "") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_detach_database(NULL, "x") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_list_databases(db, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    D_TESTS_SQLITE_CHECK(!d_sqlite_is_database_file(NULL));
    D_TESTS_SQLITE_CHECK(!d_sqlite_is_database_file(""));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_database_file(NULL, 0u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_database_file("unused.db", D_DB_IF_EXISTS) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_delete_database_file("", 0u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_delete_database_file("unused.db", D_DB_IF_NOT_EXISTS) ==
        SQLITE_MISUSE);

    return;
}

//==============================================================================
// sqlite_schema.h
//==============================================================================

/*
d_tests_sqlite_table_existence
  Tables made from SQL, in main, temp and an attached schema.
  Tests the following:
  - create_table_sql, and again under D_DB_IF_NOT_EXISTS; refused without it
  - has_table: case, each schema, temp on every version, and a table asked
    for in the wrong schema
  - has_table_s: a schema not attached is SQLite's error, and answers false
*/
static void
d_tests_sqlite_table_existence(
    void
)
{
    printf("sqlite_schema: tables, existence\n");

    sqlite3* const db     = d_tests_sqlite_open_schemas();
    bool           exists = true;

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, NULL, "Users", "id, name", 0u) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, NULL, "users", "x", 0u) ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, NULL, "users", "x",
                                  D_DB_IF_NOT_EXISTS) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, "temp", "scratch", "v", 0u) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, "aux", "far \"away\"", "v", 0u) ==
        SQLITE_OK);

    D_TESTS_SQLITE_CHECK(d_sqlite_has_table(db, NULL, "users"));
    D_TESTS_SQLITE_CHECK(d_sqlite_has_table(db, "MAIN", "USERS"));
    D_TESTS_SQLITE_CHECK(d_sqlite_has_table(db, "temp", "scratch"));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(db, NULL, "scratch"));
    D_TESTS_SQLITE_CHECK(d_sqlite_has_table(db, "aux", "far \"away\""));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(db, NULL, "nothing"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_table_s(db, "nowhere", "users", &exists) ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(!exists);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_table_s(db, NULL, "users", &exists) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(exists);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_table_changes
  Renaming and dropping tables.
  Tests the following:
  - rename_table moves the name
  - drop_table, again (an error), and again under D_DB_IF_EXISTS
  - drop_table in an attached schema
*/
static void
d_tests_sqlite_table_changes(
    void
)
{
    printf("sqlite_schema: tables, changes\n");

    sqlite3* const db = d_tests_sqlite_open_schemas();

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, NULL, "b_table", "v", 0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, "aux", "far \"away\"", "v", 0u) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_rename_table(db, NULL, "b_table", "c_table") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(db, NULL, "b_table"));
    D_TESTS_SQLITE_CHECK(d_sqlite_has_table(db, NULL, "c_table"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_table(db, NULL, "c_table", 0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_table(db, NULL, "c_table", 0u) == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_table(db, NULL, "c_table", D_DB_IF_EXISTS) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_table(db, "aux", "far \"away\"", 0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(db, "aux", "far \"away\""));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_table_rows
  Counting and emptying tables.
  Tests the following:
  - count_rows
  - truncate_table empties a table and restarts its AUTOINCREMENT sequence,
    as TRUNCATE does; a table without one empties too
  - a failed truncate leaves no transaction open
*/
static void
d_tests_sqlite_table_rows(
    void
)
{
    printf("sqlite_schema: tables, rows\n");

    sqlite3* const db    = d_tests_sqlite_open_schemas();
    int64_t        count = -1;

    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db,
                      "CREATE TABLE users(id INTEGER PRIMARY KEY "
                      "AUTOINCREMENT, name TEXT NOT NULL);"
                      "CREATE TABLE plain(v);"
                      "INSERT INTO users(name) VALUES ('a');"
                      "INSERT INTO users(name) VALUES ('b');"
                      "INSERT INTO users(name) VALUES ('c');"
                      "INSERT INTO plain VALUES (1)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_count_rows(db, NULL, "users", &count) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(count == 3);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_truncate_table(db, NULL, "users") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_count_rows(db, NULL, "users", &count) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(count == 0);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO users(name) VALUES ('d')") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_last_insert_id(db) == 1);
    D_TESTS_SQLITE_CHECK(!d_sqlite_in_transaction(db));

    D_TESTS_SQLITE_CHECK(
        d_sqlite_truncate_table(db, "main", "plain") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT count(*) FROM plain") == 0);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_truncate_table(db, NULL, "nothing") == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(!d_sqlite_in_transaction(db));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_table_listing
  Listing tables.
  Tests the following:
  - list_tables: sorted, SQLite's own sqlite_sequence left out
  - temp's tables, read from its own catalogue on every version
  - a schema not attached is SQLite's error
*/
static void
d_tests_sqlite_table_listing(
    void
)
{
    printf("sqlite_schema: tables, listing\n");

    sqlite3* const        db    = d_tests_sqlite_open_schemas();
    struct d_db_name_list names = { 0u, NULL };

    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db,
                      "CREATE TABLE b_table(v);"
                      "CREATE TABLE Users(id INTEGER PRIMARY KEY "
                      "AUTOINCREMENT);"
                      "INSERT INTO Users DEFAULT VALUES;"
                      "CREATE TEMP TABLE scratch(v)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_list_tables(db, NULL, &names) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (names.count == 2u)                    &&
                          (strcmp(names.names[0], "Users") == 0) &&
                          (strcmp(names.names[1], "b_table") == 0) );
    D_TESTS_SQLITE_CHECK(!d_tests_sqlite_has_name(&names, "sqlite_sequence"));
    d_db_name_list_free(&names);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_tables(db, "temp", &names) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (names.count == 1u) &&
                          (strcmp(names.names[0], "scratch") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_tables(db, "aux", &names) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (names.count == 0u) &&
                          (names.names)       &&
                          (names.names[0] == NULL) );
    d_db_name_list_free(&names);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_tables(db, "nowhere", &names) == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_table_arguments
  The table functions' answers to arguments that cannot be right.
  Tests the following:
  - NULL handles, empty schemas and names, empty SQL, NULL outputs, and
    flags a function does not take
*/
static void
d_tests_sqlite_table_arguments(
    void
)
{
    printf("sqlite_schema: tables, arguments\n");

    sqlite3* const        db    = d_tests_sqlite_open_schemas();
    struct d_db_name_list names = { 0u, NULL };

    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(NULL, NULL, "users"));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(db, "", "users"));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(db, NULL, ""));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_table_s(db, NULL, "users", NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, NULL, "x", "", 0u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, NULL, "x", "a", D_DB_IF_EXISTS) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_table(db, NULL, "x", D_DB_IF_NOT_EXISTS) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_rename_table(db, NULL, "users", NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_count_rows(db, NULL, "users", NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_truncate_table(db, "", "users") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_tables(NULL, NULL, &names) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_list_tables(db, NULL, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_described_types
  A table described with struct d_db_column, one column of every type.
  Tests the following:
  - each d_db_type's declared type, and type_sql used as written, in the
    columns' order
*/
static void
d_tests_sqlite_described_types(
    void
)
{
    static const char* const types[] =
    {
        "INTEGER", "", "INTEGER", "INTEGER", "REAL", "NUMERIC", "TEXT",
        "BLOB", "TEXT", "TEXT", "TEXT", "VARCHAR(20)"
    };
    const size_t count = sizeof(d_tests_sqlite_every_type) /
                         sizeof(d_tests_sqlite_every_type[0]);

    printf("sqlite_schema: described tables, types\n");

    sqlite3* const          db      = d_tests_sqlite_open_schemas();
    struct d_db_column_list columns = { 0u, NULL };

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "typed", d_tests_sqlite_every_type,
                              count, 0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_columns(db, NULL, "typed", &columns) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(columns.count == count);

    for (size_t i = 0u; (i < columns.count) && (i < count); ++i)
    {
        D_TESTS_SQLITE_CHECK(
            strcmp(columns.columns[i].name,
                   d_tests_sqlite_every_type[i].name) == 0);
        D_TESTS_SQLITE_CHECK(strcmp(columns.columns[i].type, types[i]) == 0);
    }

    d_db_column_list_free(&columns);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_described_constraints
  The flags and DEFAULTs of a described table.
  Tests the following:
  - a sole key with AUTOINCREMENT, NOT NULL, UNIQUE and DEFAULTs, as
    list_columns reads them back and as the table then behaves
*/
static void
d_tests_sqlite_described_constraints(
    void
)
{
    const size_t count = sizeof(d_tests_sqlite_every_type) /
                         sizeof(d_tests_sqlite_every_type[0]);

    printf("sqlite_schema: described tables, constraints\n");

    sqlite3* const          db      = d_tests_sqlite_open_schemas();
    struct d_db_column_list columns = { 0u, NULL };

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "typed", d_tests_sqlite_every_type,
                              count, 0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_columns(db, NULL, "typed", &columns) == SQLITE_OK);

    // keys, constraints and defaults, where there are enough columns to ask
    if (columns.count == count)
    {
        D_TESTS_SQLITE_CHECK(columns.columns[0].primary_key == 1);
        D_TESTS_SQLITE_CHECK(columns.columns[1].primary_key == 0);
        D_TESTS_SQLITE_CHECK(columns.columns[2].not_null);
        D_TESTS_SQLITE_CHECK(!columns.columns[3].not_null);
        D_TESTS_SQLITE_CHECK(columns.columns[1].default_sql == NULL);
        D_TESTS_SQLITE_CHECK( (columns.columns[6].default_sql) &&
                              (strstr(columns.columns[6].default_sql,
                                      "it''s") != NULL) );
    }

    d_db_column_list_free(&columns);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO typed(c_int) VALUES (7)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT c_id FROM typed") == 1);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT count(*) FROM sqlite_sequence "
                               "WHERE name = 'typed'") == 1);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO typed(c_int) VALUES (7)") ==
        SQLITE_CONSTRAINT);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT typeof(c_real) = 'real' AND "
                               "c_real = 1.5 FROM typed") == 1);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_described_keys
  A composite key, and creating a described table twice.
  Tests the following:
  - two key columns become a table constraint, numbered in order
  - key places order the key otherwise than the table; places that repeat,
    skip, number a column off the key, or leave a key column out are
    SQLITE_MISUSE, and make nothing
  - D_DB_IF_NOT_EXISTS makes the second create a success
*/
static void
d_tests_sqlite_described_keys(
    void
)
{
    printf("sqlite_schema: described tables, keys\n");

    sqlite3* const          db      = d_tests_sqlite_open_schemas();
    struct d_db_column_list columns = { 0u, NULL };

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, "aux", "pairs", d_tests_sqlite_composite, 3u,
                              0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_columns(db, "aux", "pairs", &columns) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (columns.count == 3u)                 &&
                          (columns.columns[0].primary_key == 1) &&
                          (columns.columns[1].primary_key == 2) &&
                          (columns.columns[2].primary_key == 0) );
    d_db_column_list_free(&columns);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, "aux", "pairs", d_tests_sqlite_composite, 3u,
                              0u) == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, "aux", "pairs", d_tests_sqlite_composite, 3u,
                              D_DB_IF_NOT_EXISTS) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(db, NULL, "pairs"));

    // a key in the order its places give, not the table's
    struct d_db_column bad[3] =
    {
        d_tests_sqlite_ordered[0],
        d_tests_sqlite_ordered[1],
        d_tests_sqlite_ordered[2]
    };

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db,
                                      "aux",
                                      "ordered",
                                      d_tests_sqlite_ordered,
                                      3u,
                                      0u) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_columns(db,
                                      "aux",
                                      "ordered",
                                      &columns) ==
        SQLITE_OK);

    const struct d_db_column_info* const first  =
        d_tests_sqlite_find_column(&columns, "a");
    const struct d_db_column_info* const second =
        d_tests_sqlite_find_column(&columns, "b");

    D_TESTS_SQLITE_CHECK( (first) && (first->primary_key == 2) );
    D_TESTS_SQLITE_CHECK( (second) && (second->primary_key == 1) );
    d_db_column_list_free(&columns);

    // places that repeat, skip, number a column off the key, or leave one
    // key column out
    bad[1].key_place = 2u;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db,
                                      "aux",
                                      "unordered",
                                      bad,
                                      3u,
                                      0u) ==
        SQLITE_MISUSE);
    bad[1].key_place = 3u;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db,
                                      "aux",
                                      "unordered",
                                      bad,
                                      3u,
                                      0u) ==
        SQLITE_MISUSE);
    bad[1].key_place = 0u;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db,
                                      "aux",
                                      "unordered",
                                      bad,
                                      3u,
                                      0u) ==
        SQLITE_MISUSE);
    bad[1].key_place = 1u;
    bad[2].key_place = 3u;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db,
                                      "aux",
                                      "unordered",
                                      bad,
                                      3u,
                                      0u) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(db, "aux", "unordered"));

    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_described_refusals
  Column sets the layer refuses before SQLite sees them.
  Tests the following:
  - AUTO_INCREMENT on a composite key, or without a key
  - a type outside enum d_db_type, an unknown column flag, an empty name
  - no columns, a NULL array, and an unknown table flag
  - nothing is created by any of them
*/
static void
d_tests_sqlite_described_refusals(
    void
)
{
    static const struct d_db_column bad_increment[] =
    {
        { "a", D_DB_TYPE_INTEGER, D_DB_COLUMN_AUTO_INCREMENT, NULL, 0u, 0u,
          NULL, 0u }
    };
    static const struct d_db_column bad_type[] =
    {
        { "a", (enum d_db_type)99, 0u, NULL, 0u, 0u, NULL, 0u }
    };
    static const struct d_db_column bad_flag[] =
    {
        { "a", D_DB_TYPE_INTEGER, 0x8000u, NULL, 0u, 0u, NULL, 0u }
    };
    static const struct d_db_column no_name[] =
    {
        { "", D_DB_TYPE_INTEGER, 0u, NULL, 0u, 0u, NULL, 0u }
    };
    struct d_db_column half[2] =
    {
        d_tests_sqlite_composite[0], d_tests_sqlite_composite[1]
    };

    printf("sqlite_schema: described tables, refusals\n");

    sqlite3* const db = d_tests_sqlite_open_schemas();

    half[0].flags |= D_DB_COLUMN_AUTO_INCREMENT;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "x", half, 2u, 0u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "x", bad_increment, 1u, 0u) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "x", bad_type, 1u, 0u) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "x", bad_flag, 1u, 0u) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "x", no_name, 1u, 0u) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "x", d_tests_sqlite_composite, 0u,
                              0u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "x", NULL, 1u, 0u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "x", d_tests_sqlite_composite, 3u,
                              0x8000u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(db, NULL, "x"));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_described_options
  SQLite's own table options.
  Tests the following:
  - STRICT: its own type names, its type checking, and with WITHOUT ROWID;
    D_DB_UNSUPPORTED below 3.37.0
  - WITHOUT ROWID alone; D_DB_UNSUPPORTED below 3.8.2
*/
static void
d_tests_sqlite_described_options(
    void
)
{
    static const char* const strict_types[] =
    {
        "INTEGER", "ANY", "INTEGER", "INTEGER", "REAL", "TEXT", "TEXT",
        "BLOB", "TEXT", "TEXT", "TEXT"
    };
    const size_t count = sizeof(strict_types) / sizeof(strict_types[0]);

    printf("sqlite_schema: described tables, options\n");

    sqlite3* const          db      = d_tests_sqlite_open_schemas();
    struct d_db_column_list columns = { 0u, NULL };
    const bool              strict  = d_tests_sqlite_at_least(3, 37, 0);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table(db, NULL, "strict_t", d_tests_sqlite_every_type,
                              count, D_SQLITE_STRICT) ==
        ((strict) ? SQLITE_OK : D_DB_UNSUPPORTED));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_columns(db, NULL, "strict_t", &columns) ==
        ((strict) ? SQLITE_OK : D_DB_NOT_FOUND));
    D_TESTS_SQLITE_CHECK(columns.count == ((strict) ? count : 0u));

    for (size_t i = 0u; (i < columns.count) && (i < count); ++i)
    {
        D_TESTS_SQLITE_CHECK(
            strcmp(columns.columns[i].type, strict_types[i]) == 0);
    }

    d_db_column_list_free(&columns);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO strict_t(c_int) VALUES ('x')") ==
        ((strict) ? SQLITE_CONSTRAINT : SQLITE_ERROR));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, NULL, "both", "a INTEGER PRIMARY KEY",
                                  D_SQLITE_STRICT | D_SQLITE_WITHOUT_ROWID) ==
        ((strict) ? SQLITE_OK : D_DB_UNSUPPORTED));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, NULL, "keyed", "k TEXT PRIMARY KEY, v",
                                  D_SQLITE_WITHOUT_ROWID) ==
        ((d_tests_sqlite_at_least(3, 8, 2)) ? SQLITE_OK : D_DB_UNSUPPORTED));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_column_existence
  Asking for columns, adding them, and listing them.
  Tests the following:
  - has_column: case, and a missing table, which has no columns
  - add_column described and as SQL; SQLite's own refusals of NOT NULL
    without a DEFAULT on a table with rows, and of a second primary key
  - list_columns reads the added column back
*/
static void
d_tests_sqlite_column_existence(
    void
)
{
    static const struct d_db_column note = {
        "note", D_DB_TYPE_STRING, 0u, "''", 0u, 0u, NULL, 0u
    };
    static const struct d_db_column key = {
        "k2", D_DB_TYPE_INTEGER, D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u, NULL, 0u
    };

    printf("sqlite_schema: columns, existence and adding\n");

    sqlite3* const          db      = d_tests_sqlite_open_schemas();
    struct d_db_column_list columns = { 0u, NULL };
    bool                    exists  = true;

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, NULL, "cols", "a INTEGER, b TEXT", 0u)
        == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_has_column(db, NULL, "cols", "a"));
    D_TESTS_SQLITE_CHECK(d_sqlite_has_column(db, "main", "COLS", "B"));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_column(db, NULL, "cols", "c"));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_column(db, NULL, "nothing", "a"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_column_s(db, NULL, "nothing", "a", &exists) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!exists);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_add_column(db, NULL, "cols", &note) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_add_column_sql(db, NULL, "cols", "d REAL DEFAULT 0") ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_has_column(db, NULL, "cols", "note"));
    D_TESTS_SQLITE_CHECK(d_sqlite_has_column(db, NULL, "cols", "d"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO cols(a) VALUES (1)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_add_column_sql(db, NULL, "cols", "e TEXT NOT NULL") ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_add_column(db, NULL, "cols", &key) == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_columns(db, NULL, "cols", &columns) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (columns.count == 4u)                         &&
                          (strcmp(columns.columns[2].name, "note") == 0) &&
                          (strcmp(columns.columns[2].type, "TEXT") == 0) );
    d_db_column_list_free(&columns);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_column_kinds
  Columns whose table decides what they are.
  Tests the following:
  - a column added to a STRICT table takes the STRICT mapping, and the same
    column added to a plain table the plain one, from 3.37.0
  - a generated column is a column to has_column and list_columns, from
    3.31.0
*/
static void
d_tests_sqlite_column_kinds(
    void
)
{
    static const struct d_db_column price = {
        "price", D_DB_TYPE_DECIMAL, 0u, NULL, 0u, 0u, NULL, 0u
    };

    printf("sqlite_schema: columns, kinds\n");

    sqlite3* const                 db      = d_tests_sqlite_open_schemas();
    struct d_db_column_list        columns = { 0u, NULL };
    const struct d_db_column_info* info    = NULL;

    // STRICT, from 3.37.0
    if (d_tests_sqlite_at_least(3, 37, 0))
    {
        D_TESTS_SQLITE_CHECK(
            d_sqlite_create_table_sql(db, NULL, "strict_cols", "a INTEGER",
                                      D_SQLITE_STRICT) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_create_table_sql(db, NULL, "cols", "a INTEGER", 0u) ==
            SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_add_column(db, NULL, "strict_cols", &price) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_add_column(db, NULL, "cols", &price) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_list_columns(db, NULL, "strict_cols", &columns) ==
            SQLITE_OK);
        info = d_tests_sqlite_find_column(&columns, "price");
        D_TESTS_SQLITE_CHECK( (info) && (strcmp(info->type, "TEXT") == 0) );
        d_db_column_list_free(&columns);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_list_columns(db, NULL, "cols", &columns) == SQLITE_OK);
        info = d_tests_sqlite_find_column(&columns, "price");
        D_TESTS_SQLITE_CHECK( (info) && (strcmp(info->type, "NUMERIC") == 0) );
        d_db_column_list_free(&columns);
    }

    // generated columns, from 3.31.0
    if (d_tests_sqlite_at_least(3, 31, 0))
    {
        D_TESTS_SQLITE_CHECK(
            d_sqlite_create_table_sql(db, NULL, "gen",
                                      "a INTEGER, b INTEGER AS (a * 2)",
                                      0u) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK(d_sqlite_has_column(db, NULL, "gen", "b"));
        D_TESTS_SQLITE_CHECK(
            d_sqlite_list_columns(db, NULL, "gen", &columns) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK( (columns.count == 2u) &&
                              (!columns.columns[0].generated) &&
                              (columns.columns[1].generated) );
        d_db_column_list_free(&columns);
    }

    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_column_changes
  Renaming and dropping columns.
  Tests the following:
  - rename_column, or D_DB_UNSUPPORTED below 3.25.0
  - drop_column, or D_DB_UNSUPPORTED below 3.35.0
  - list_columns of a missing table is D_DB_NOT_FOUND, the list left empty
*/
static void
d_tests_sqlite_column_changes(
    void
)
{
    printf("sqlite_schema: columns, changes\n");

    sqlite3* const          db      = d_tests_sqlite_open_schemas();
    struct d_db_column_list columns = { 0u, NULL };
    const bool              renames = d_tests_sqlite_at_least(3, 25, 0);
    const bool              drops   = d_tests_sqlite_at_least(3, 35, 0);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_table_sql(db, NULL, "cols", "a, b, d", 0u) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_rename_column(db, NULL, "cols", "b", "bee") ==
        ((renames) ? SQLITE_OK : D_DB_UNSUPPORTED));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_column(db, NULL, "cols", "bee") == renames);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_column(db, NULL, "cols", "d") ==
        ((drops) ? SQLITE_OK : D_DB_UNSUPPORTED));
    D_TESTS_SQLITE_CHECK(d_sqlite_has_column(db, NULL, "cols", "d") == !drops);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_columns(db, NULL, "nothing", &columns) ==
        D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK( (columns.count == 0u) && (columns.columns == NULL) );
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_column_arguments
  The column functions' answers to arguments that cannot be right.
  Tests the following:
  - NULL outputs and tables, empty names and SQL, and a NULL description
*/
static void
d_tests_sqlite_column_arguments(
    void
)
{
    printf("sqlite_schema: columns, arguments\n");

    sqlite3* const db     = d_tests_sqlite_open_schemas();
    bool           exists = true;

    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_columns(db, NULL, "cols", NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_column_s(db, NULL, "cols", "", &exists) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(!exists);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_add_column(db, NULL, "cols", NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_add_column_sql(db, NULL, "cols", "") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_rename_column(db, NULL, "cols", "a", "") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_column(db, NULL, NULL, "a") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_indexes
  Indexes.
  Tests the following:
  - create: plain, again (an error), idempotent, UNIQUE (over an expression
    where the library has them), and UNIQUE refused over duplicates
  - has_index, ignoring case
  - list_indexes for a table, SQLite's automatic indexes left out, and for
    the schema
  - drop_index, and under D_DB_IF_EXISTS
*/
static void
d_tests_sqlite_indexes(
    void
)
{
    printf("sqlite_schema: indexes\n");

    sqlite3* const        db    = d_tests_sqlite_open_people();
    struct d_db_name_list names = { 0u, NULL };
    const char* const     email =
        (d_sqlite_supports(D_SQLITE_FEATURE_EXPRESSION_INDEX))
            ? "lower(email)"
            : "email";

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_index(db, NULL, "by_name", "people", "last, first",
                              0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_index(db, NULL, "by_name", "people", "last", 0u) ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_index(db, NULL, "by_name", "people", "last",
                              D_DB_IF_NOT_EXISTS) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_index(db, NULL, "by_lower", "people", email,
                              D_DB_UNIQUE) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_index(db, NULL, "one_last", "people", "last",
                              D_DB_UNIQUE) == SQLITE_CONSTRAINT);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_index(db, NULL, "on_audit", "audit", "n", 0u) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_has_index(db, NULL, "BY_NAME"));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_index(db, NULL, "one_last"));

    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_indexes(db, NULL, "people", &names) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (names.count == 2u)                       &&
                          (strcmp(names.names[0], "by_lower") == 0) &&
                          (strcmp(names.names[1], "by_name") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_indexes(db, NULL, NULL, &names) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (names.count == 3u) &&
                          (d_tests_sqlite_has_name(&names, "on_audit")) );
    d_db_name_list_free(&names);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_index(db, NULL, "by_lower", 0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_index(db, NULL, "by_lower"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_index(db, NULL, "by_lower", D_DB_IF_EXISTS) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_views
  Views.
  Tests the following:
  - create, idempotently, and refused a second time without the flag
  - has_view, which a table of the same name does not satisfy
  - the view answers queries; list_views; drop_view
*/
static void
d_tests_sqlite_views(
    void
)
{
    printf("sqlite_schema: views\n");

    sqlite3* const        db    = d_tests_sqlite_open_people();
    struct d_db_name_list names = { 0u, NULL };

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_view(db, NULL, "smiths",
                             "SELECT first FROM people WHERE last = 'Smith'",
                             0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_view(db, NULL, "smiths", "SELECT 1",
                             D_DB_IF_NOT_EXISTS) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_view(db, NULL, "smiths", "SELECT 1", 0u) ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(d_sqlite_has_view(db, NULL, "Smiths"));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_table(db, NULL, "smiths"));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_view(db, NULL, "people"));
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(db, "SELECT count(*) FROM smiths") == 2);
    D_TESTS_SQLITE_CHECK(d_sqlite_list_views(db, NULL, &names) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (names.count == 1u) &&
                          (strcmp(names.names[0], "smiths") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_view(db, NULL, "smiths", 0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_view(db, NULL, "smiths"));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_triggers
  Triggers.
  Tests the following:
  - create, and idempotently; has_trigger
  - the trigger fires
  - list_triggers for its table, and for a table with none: an empty,
    NULL-terminated list
  - drop_trigger, and again (an error)
*/
static void
d_tests_sqlite_triggers(
    void
)
{
    printf("sqlite_schema: triggers\n");

    sqlite3* const        db    = d_tests_sqlite_open_people();
    struct d_db_name_list names = { 0u, NULL };

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_trigger(db, NULL, "count_people",
                                "AFTER INSERT ON people BEGIN "
                                "UPDATE audit SET n = n + 1; END",
                                0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_trigger(db, NULL, "count_people",
                                "AFTER DELETE ON people BEGIN SELECT 1; END",
                                D_DB_IF_NOT_EXISTS) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_has_trigger(db, NULL, "count_people"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO people VALUES (3, 'c@x', 'Lee', 'Cy')")
        == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT n FROM audit") == 1);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_triggers(db, NULL, "people", &names) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (names.count == 1u) &&
                          (strcmp(names.names[0], "count_people") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_triggers(db, NULL, "audit", &names) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (names.count == 0u) &&
                          (names.names)       &&
                          (names.names[0] == NULL) );
    d_db_name_list_free(&names);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_trigger(db, NULL, "count_people", 0u) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_trigger(db, NULL, "count_people"));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_trigger(db, NULL, "count_people", 0u) == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_object_arguments
  The index, view and trigger functions' answers to arguments that cannot be
right.
  Tests the following:
  - empty and NULL names and SQL, NULL outputs, and flags a function does
    not take, D_DB_UNIQUE outside an index among them
*/
static void
d_tests_sqlite_object_arguments(
    void
)
{
    printf("sqlite_schema: indexes, views, triggers, arguments\n");

    sqlite3* const        db     = d_tests_sqlite_open_people();
    struct d_db_name_list names  = { 0u, NULL };
    bool                  exists = true;

    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_index(db, NULL, "x", "people", "last",
                              D_DB_IF_EXISTS) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_index(db, NULL, "x", "people", "", 0u) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_index(db, NULL, "x", NULL, "last", 0u) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_indexes(db, NULL, "", &names) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_has_index_s(db, NULL, "", &exists) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(!exists);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_view(db, NULL, "v", NULL, 0u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_view(db, NULL, "v", "SELECT 1", D_DB_UNIQUE) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_list_views(db, NULL, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_create_trigger(db, NULL, "t", "", 0u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_drop_trigger(db, NULL, "", 0u) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_list_triggers(NULL, NULL, NULL, &names) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_view(db, NULL, NULL));
    D_TESTS_SQLITE_CHECK(!d_sqlite_has_trigger(db, "", "t"));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

//==============================================================================
// sqlite_transaction.h
//==============================================================================

/*
d_tests_sqlite_transactions
  Transactions.
  Tests the following:
  - each mode begins a transaction that in_transaction sees
  - SQLite does not nest transactions
  - commit keeps work; rollback undoes it; neither works outside one
  - argument checks
*/
static void
d_tests_sqlite_transactions(
    void
)
{
    sqlite3* db = NULL;

    printf("sqlite_transaction: transactions\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(db, "CREATE TABLE t(a)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_in_transaction(db));

    D_TESTS_SQLITE_CHECK(
        d_sqlite_begin_transaction(db, D_SQLITE_TRANSACTION_DEFERRED) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_in_transaction(db));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_begin_transaction(db, D_SQLITE_TRANSACTION_DEFERRED) ==
        SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO t VALUES (1)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_commit(db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_in_transaction(db));
    D_TESTS_SQLITE_CHECK(d_sqlite_commit(db) == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(d_sqlite_rollback(db) == SQLITE_ERROR);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_begin_transaction(db, D_SQLITE_TRANSACTION_IMMEDIATE) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO t VALUES (2)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_rollback(db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM t") == 1);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_begin_transaction(db, D_SQLITE_TRANSACTION_EXCLUSIVE) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_in_transaction(db));
    D_TESTS_SQLITE_CHECK(d_sqlite_commit(db) == SQLITE_OK);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_begin_transaction(db, (enum d_sqlite_transaction_mode)7) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_begin_transaction(NULL, D_SQLITE_TRANSACTION_DEFERRED) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_commit(NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_rollback(NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(!d_sqlite_in_transaction(NULL));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_savepoints
  Savepoints.
  Tests the following:
  - rolling back to a savepoint undoes only what followed it; releasing one
    keeps the rest; a released savepoint is gone
  - names that need quoting
  - a savepoint outside a transaction opens one, and its release ends it
  - argument checks
*/
static void
d_tests_sqlite_savepoints(
    void
)
{
    sqlite3* db = NULL;

    printf("sqlite_transaction: savepoints\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_exec(db, "CREATE TABLE t(a)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_begin_transaction(db, D_SQLITE_TRANSACTION_DEFERRED) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_create_savepoint(db, "s 1") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO t VALUES (1)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_create_savepoint(db, "s\"2") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO t VALUES (2)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_rollback_to_savepoint(db, "s\"2") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM t") == 1);
    D_TESTS_SQLITE_CHECK(d_sqlite_release_savepoint(db, "s 1") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_release_savepoint(db, "s 1") == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(d_sqlite_commit(db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM t") == 1);

    D_TESTS_SQLITE_CHECK(d_sqlite_create_savepoint(db, "alone") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_in_transaction(db));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO t VALUES (3)") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_release_savepoint(db, "alone") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!d_sqlite_in_transaction(db));
    D_TESTS_SQLITE_CHECK(d_tests_sqlite_int(db, "SELECT count(*) FROM t") == 2);

    D_TESTS_SQLITE_CHECK(d_sqlite_create_savepoint(db, "") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_release_savepoint(db, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_rollback_to_savepoint(NULL, "x") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

//==============================================================================
// sqlite_settings.h
//==============================================================================

/*
d_tests_sqlite_connection_settings
  Foreign keys and the busy timeout.
  Tests the following:
  - foreign keys off and on, read back; refused inside a transaction, where
    SQLite would ignore the change
  - the busy timeout round trip; a negative one refused
*/
static void
d_tests_sqlite_connection_settings(
    void
)
{
    sqlite3* db      = NULL;
    bool     enabled = false;
    int      timeout = -1;

    printf("sqlite_settings: connection settings\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_set_foreign_keys(db, false) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_foreign_keys(db, &enabled) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(!enabled);
    D_TESTS_SQLITE_CHECK(d_sqlite_set_foreign_keys(db, true) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_foreign_keys(db, &enabled) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(enabled);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_begin_transaction(db, D_SQLITE_TRANSACTION_DEFERRED) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_foreign_keys(db, false) == D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_CHECK(d_sqlite_rollback(db) == SQLITE_OK);

    D_TESTS_SQLITE_CHECK(d_sqlite_set_busy_timeout(db, 250) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_busy_timeout(db, &timeout) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(timeout == 250);
    D_TESTS_SQLITE_CHECK(d_sqlite_set_busy_timeout(db, -1) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_busy_timeout(db, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_journal_modes
  The journal mode.
  Tests the following:
  - memory refuses WAL and takes MEMORY, read back; a stray mode refused
  - a file takes WAL, read back, and returns to DELETE
*/
static void
d_tests_sqlite_journal_modes(
    void
)
{
    char                       path[1024] = "";
    sqlite3*                   db         = NULL;
    enum d_sqlite_journal_mode mode       = D_SQLITE_JOURNAL_OFF;

    printf("sqlite_settings: journal modes\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_journal_mode(db, NULL, D_SQLITE_JOURNAL_WAL) ==
        D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_journal_mode(db, NULL, D_SQLITE_JOURNAL_MEMORY) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_journal_mode(db, NULL, &mode) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(mode == D_SQLITE_JOURNAL_MEMORY);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_journal_mode(db, NULL, (enum d_sqlite_journal_mode)9) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    // a file can do WAL
    d_tests_sqlite_path(path, sizeof(path), "t_sqlite_settings.db");
    d_tests_sqlite_remove_all(path);
    D_TESTS_SQLITE_CHECK(d_sqlite_open(path, &db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_journal_mode(db, NULL, D_SQLITE_JOURNAL_WAL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_journal_mode(db, NULL, &mode) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(mode == D_SQLITE_JOURNAL_WAL);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_journal_mode(db, "main", D_SQLITE_JOURNAL_DELETE) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_get_journal_mode(db, NULL, &mode) ==
                         SQLITE_OK);
    D_TESTS_SQLITE_CHECK(mode == D_SQLITE_JOURNAL_DELETE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    d_tests_sqlite_remove_all(path);

    return;
}

/*
d_tests_sqlite_synchronous
  The synchronous level.
  Tests the following:
  - OFF to FULL, each read back; a stray level refused
  - EXTRA where the library knows it, D_DB_UNSUPPORTED before 3.11.0, where
    SQLite would store it and act as FULL
*/
static void
d_tests_sqlite_synchronous(
    void
)
{
    sqlite3*                  db    = NULL;
    enum d_sqlite_synchronous level = D_SQLITE_SYNCHRONOUS_OFF;

    printf("sqlite_settings: synchronous\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);

    for (int value = 0; value <= 2; ++value)
    {
        D_TESTS_SQLITE_CHECK(
            d_sqlite_set_synchronous(db, NULL,
                                     (enum d_sqlite_synchronous)value) ==
            SQLITE_OK);
        D_TESTS_SQLITE_CHECK(
            d_sqlite_get_synchronous(db, "main", &level) == SQLITE_OK);
        D_TESTS_SQLITE_CHECK((int)level == value);
    }

    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_synchronous(db, NULL, (enum d_sqlite_synchronous)4) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_synchronous(db, NULL, D_SQLITE_SYNCHRONOUS_EXTRA) ==
        ((d_tests_sqlite_at_least(3, 11, 0)) ? SQLITE_OK : D_DB_UNSUPPORTED));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_synchronous(db, NULL, &level) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        level == ((d_tests_sqlite_at_least(3, 11, 0))
                      ? D_SQLITE_SYNCHRONOUS_EXTRA
                      : D_SQLITE_SYNCHRONOUS_FULL));
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_version_numbers
  user_version and application_id.
  Tests the following:
  - each round trip, a negative application_id included
  - each schema keeps its own; a schema not attached is SQLite's error
  - argument checks
*/
static void
d_tests_sqlite_version_numbers(
    void
)
{
    printf("sqlite_settings: version numbers\n");

    sqlite3* const db     = d_tests_sqlite_open_schemas();
    int32_t        number = 0;

    D_TESTS_SQLITE_CHECK(d_sqlite_set_user_version(db, NULL, 42) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_user_version(db, NULL, &number) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(number == 42);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_application_id(db, "main", -1234) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_application_id(db, "main", &number) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(number == -1234);
    D_TESTS_SQLITE_CHECK(d_sqlite_set_user_version(db, "aux", 7) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_user_version(db, "aux", &number) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(number == 7);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_user_version(db, NULL, &number) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(number == 42);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_user_version(db, "nowhere", &number) == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_user_version(db, "", 1) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_application_id(db, NULL, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

/*
d_tests_sqlite_pragmas
  Any pragma, by name.
  Tests the following:
  - set_pragma and get_pragma, unqualified and for an attached schema
  - a pragma that answers nothing is D_DB_NOT_FOUND
  - names that are not names refused before SQLite sees them; NULL values
*/
static void
d_tests_sqlite_pragmas(
    void
)
{
    printf("sqlite_settings: any pragma\n");

    sqlite3* const db   = d_tests_sqlite_open_schemas();
    char*          text = NULL;

    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_pragma(db, NULL, "cache_size", "-4000") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_pragma(db, NULL, "cache_size", &text) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (text) && (strcmp(text, "-4000") == 0) );
    d_db_free(text);
    text = NULL;
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_pragma(db, "aux", "user_version", "9") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_pragma(db, "aux", "user_version", &text) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK( (text) && (strcmp(text, "9") == 0) );
    d_db_free(text);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_pragma(db, NULL, "no_such_pragma", &text) ==
        D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_pragma(db, NULL, "cache_size; DROP TABLE x", "1") ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_pragma(db, NULL, "9lives", "1") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_pragma(db, NULL, "cache_size", NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_get_pragma(db, NULL, "", &text) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

//==============================================================================
// sqlite_maintenance.h
//==============================================================================

/*
d_tests_sqlite_upkeep
  Compacting, statistics and checks.
  Tests the following:
  - vacuum of main, and of an attached schema or D_DB_UNSUPPORTED before
    3.15.0
  - vacuum_into, or D_DB_UNSUPPORTED before 3.27.0; the copy is a database
  - analyze; optimize, or D_DB_UNSUPPORTED before 3.18.0
  - both checks pass a sound database; a schema not attached is an error
*/
static void
d_tests_sqlite_upkeep(
    void
)
{
    char path[1024]      = "";
    char copy_path[1024] = "";
    bool ok              = false;

    printf("sqlite_maintenance: upkeep\n");

    d_tests_sqlite_path(path, sizeof(path), "t_sqlite_upkeep.db");
    d_tests_sqlite_path(copy_path, sizeof(copy_path), "t_sqlite_copy.db");
    d_tests_sqlite_remove_all(copy_path);

    sqlite3* const db = d_tests_sqlite_open_file(path);

    D_TESTS_SQLITE_CHECK(d_sqlite_vacuum(db, NULL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_vacuum(db, "aux") ==
        ((d_tests_sqlite_at_least(3, 15, 0)) ? SQLITE_OK : D_DB_UNSUPPORTED));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_vacuum_into(db, NULL, copy_path) ==
        ((d_tests_sqlite_at_least(3, 27, 0)) ? SQLITE_OK : D_DB_UNSUPPORTED));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_is_database_file(copy_path) ==
        d_tests_sqlite_at_least(3, 27, 0));

    D_TESTS_SQLITE_CHECK(d_sqlite_analyze(db, NULL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_analyze(db, "aux") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_optimize(db, NULL) ==
        ((d_tests_sqlite_at_least(3, 18, 0)) ? SQLITE_OK : D_DB_UNSUPPORTED));
    D_TESTS_SQLITE_CHECK(d_sqlite_integrity_check(db, NULL, &ok) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(ok);
    ok = false;
    D_TESTS_SQLITE_CHECK(d_sqlite_quick_check(db, "aux", &ok) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(ok);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_integrity_check(db, "nowhere", &ok) == SQLITE_ERROR);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    d_tests_sqlite_remove_all(path);
    d_tests_sqlite_remove_all(copy_path);

    return;
}

/*
d_tests_sqlite_checkpoints
  Write-ahead log checkpoints.
  Tests the following:
  - every mode on a WAL file, TRUNCATE refused before 3.8.8
  - a stray mode refused
*/
static void
d_tests_sqlite_checkpoints(
    void
)
{
    char path[1024] = "";

    printf("sqlite_maintenance: checkpoints\n");

    d_tests_sqlite_path(path, sizeof(path), "t_sqlite_checkpoints.db");

    sqlite3* const db = d_tests_sqlite_open_file(path);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_set_journal_mode(db, NULL, D_SQLITE_JOURNAL_WAL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_exec(db, "INSERT INTO t(b) VALUES ('four')") == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_checkpoint(db, NULL, D_SQLITE_CHECKPOINT_PASSIVE) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_checkpoint(db, "main", D_SQLITE_CHECKPOINT_FULL) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_checkpoint(db, NULL, D_SQLITE_CHECKPOINT_RESTART) ==
        SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_checkpoint(db, NULL, D_SQLITE_CHECKPOINT_TRUNCATE) ==
        ((d_tests_sqlite_at_least(3, 8, 8)) ? SQLITE_OK : D_DB_UNSUPPORTED));
    D_TESTS_SQLITE_CHECK(
        d_sqlite_checkpoint(db, NULL, (enum d_sqlite_checkpoint_mode)4) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    d_tests_sqlite_remove_all(path);

    return;
}

/*
d_tests_sqlite_backups
  Copies between connections and files.
  Tests the following:
  - copy_database into another connection
  - backup_to_file, and restore_from_file into a fresh connection
  - restoring from a file that is not a database, or is not there
*/
static void
d_tests_sqlite_backups(
    void
)
{
    char     path[1024]      = "";
    char     copy_path[1024] = "";
    char     text_path[1024] = "";
    sqlite3* other           = NULL;

    printf("sqlite_maintenance: backups\n");

    d_tests_sqlite_path(path, sizeof(path), "t_sqlite_backups.db");
    d_tests_sqlite_path(copy_path, sizeof(copy_path), "t_sqlite_backup.db");
    d_tests_sqlite_path(text_path, sizeof(text_path), "t_sqlite_text2.db");
    d_tests_sqlite_remove_all(copy_path);

    sqlite3* const db = d_tests_sqlite_open_file(path);

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&other) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_copy_database(db, NULL, other, NULL) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(other, "SELECT count(*) FROM t") == 3);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(other) == SQLITE_OK);

    D_TESTS_SQLITE_CHECK(
        d_sqlite_backup_to_file(db, NULL, copy_path) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_is_database_file(copy_path));
    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&other) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_restore_from_file(other, NULL, copy_path) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(
        d_tests_sqlite_int(other, "SELECT a FROM t WHERE b = 'three'") == 3);

    d_tests_sqlite_write_file(text_path, "plain text, no database here at all");
    D_TESTS_SQLITE_CHECK(
        d_sqlite_restore_from_file(other, NULL, text_path) == SQLITE_NOTADB);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_restore_from_file(other, NULL, "t_sqlite_no/such.db") ==
        SQLITE_CANTOPEN);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(other) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    d_tests_sqlite_remove_all(path);
    d_tests_sqlite_remove_all(copy_path);
    d_tests_sqlite_remove_all(text_path);

    return;
}

/*
d_tests_sqlite_maintenance_arguments
  sqlite_maintenance.h's answers to arguments that cannot be right.
  Tests the following:
  - NULL handles and outputs, empty schemas and paths, for every function
*/
static void
d_tests_sqlite_maintenance_arguments(
    void
)
{
    sqlite3* db = NULL;

    printf("sqlite_maintenance: arguments\n");

    D_TESTS_SQLITE_CHECK(d_sqlite_open_memory(&db) == SQLITE_OK);
    D_TESTS_SQLITE_CHECK(d_sqlite_vacuum(NULL, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_vacuum(db, "") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_vacuum_into(db, NULL, "") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_analyze(NULL, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_optimize(db, "") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_quick_check(db, NULL, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_checkpoint(NULL, NULL, D_SQLITE_CHECKPOINT_PASSIVE) ==
        SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_copy_database(db, NULL, NULL, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_backup_to_file(db, NULL, NULL) == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(
        d_sqlite_restore_from_file(NULL, NULL, "unused.db") == SQLITE_MISUSE);
    D_TESTS_SQLITE_CHECK(d_sqlite_close(db) == SQLITE_OK);

    return;
}

//==============================================================================
// DRIVER
//==============================================================================

// d_tests_sqlite_test
//   type: a test, which reports through D_TESTS_SQLITE_CHECK.
typedef void (*d_tests_sqlite_test)(void);

// d_tests_sqlite_all
//   table: every test, in the order of the headers.
static const d_tests_sqlite_test
d_tests_sqlite_all[] =
{
    d_tests_sqlite_db_common,
    d_tests_sqlite_integer_conversions,
    d_tests_sqlite_real_conversions,
    d_tests_sqlite_bool_conversions,
    d_tests_sqlite_rows_reading,
    d_tests_sqlite_versions,
    d_tests_sqlite_features,
    d_tests_sqlite_messages,
    d_tests_sqlite_errors,
    d_tests_sqlite_opening,
    d_tests_sqlite_opening_with,
    d_tests_sqlite_apply_defaults,
    d_tests_sqlite_location,
    d_tests_sqlite_rows_changed,
    d_tests_sqlite_extensions,
    d_tests_sqlite_connection_arguments,
    d_tests_sqlite_statements,
    d_tests_sqlite_dialects,
    d_tests_sqlite_numbers,
    d_tests_sqlite_text,
    d_tests_sqlite_changes_counted,
    d_tests_sqlite_rows_values,
    d_tests_sqlite_rows_types,
    d_tests_sqlite_rows_statements,
    d_tests_sqlite_quoting,
    d_tests_sqlite_query_arguments,
    d_tests_sqlite_preparing,
    d_tests_sqlite_binding,
    d_tests_sqlite_running,
    d_tests_sqlite_cursors,
    d_tests_sqlite_statement_locks,
    d_tests_sqlite_statement_arguments,
    d_tests_sqlite_attached,
    d_tests_sqlite_database_files,
    d_tests_sqlite_serialization,
    d_tests_sqlite_database_arguments,
    d_tests_sqlite_table_existence,
    d_tests_sqlite_table_changes,
    d_tests_sqlite_table_rows,
    d_tests_sqlite_table_listing,
    d_tests_sqlite_table_arguments,
    d_tests_sqlite_described_types,
    d_tests_sqlite_described_constraints,
    d_tests_sqlite_described_keys,
    d_tests_sqlite_described_refusals,
    d_tests_sqlite_described_options,
    d_tests_sqlite_column_existence,
    d_tests_sqlite_column_kinds,
    d_tests_sqlite_column_changes,
    d_tests_sqlite_column_arguments,
    d_tests_sqlite_indexes,
    d_tests_sqlite_views,
    d_tests_sqlite_triggers,
    d_tests_sqlite_object_arguments,
    d_tests_sqlite_transactions,
    d_tests_sqlite_savepoints,
    d_tests_sqlite_connection_settings,
    d_tests_sqlite_journal_modes,
    d_tests_sqlite_synchronous,
    d_tests_sqlite_version_numbers,
    d_tests_sqlite_pragmas,
    d_tests_sqlite_upkeep,
    d_tests_sqlite_checkpoints,
    d_tests_sqlite_backups,
    d_tests_sqlite_maintenance_arguments
};

/*
main
  Runs every test and reports; the exit status is 0 only if every check
passed.
*/
int
main(
    int    _argc,
    char** _argv
)
{
    const size_t count = sizeof(d_tests_sqlite_all) /
                         sizeof(d_tests_sqlite_all[0]);

    // the directory for the files the tests make
    if (_argc > 1)
    {
        g_directory = _argv[1];
    }

    for (size_t i = 0u; i < count; ++i)
    {
        d_tests_sqlite_all[i]();
    }

    printf("%d checks, %d failed (SQLite %s)\n",
           g_checks,
           g_failures,
           sqlite3_libversion());

    return (g_failures == 0) ? 0 : 1;
}
