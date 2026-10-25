/*******************************************************************************
* djinterp [c]                                                      t_postgres.c
*
*   Conformance harness for djinterp's PostgreSQL convenience layer: every
* function in c/db/postgres/postgres_*.h, its success paths, its documented
* failures and its argument checks, against a live server -- PostgreSQL from
* 10 -- through whichever libpq it is linked with. Where the answer depends on
* the server, the check asks the server's version and expects the matching
* answer, so one harness passes on every server the layer supports.
*   Each d_postgres_supports answer is also checked against the server itself,
* by running SQL that needs the feature: a version table that drifts from the
* truth fails here, not in a program.
*   The tests follow the headers, section by section. Each makes a schema of
* its own, named after the run and the test, connects with it alone on the
* search path, so that it is the current schema, and drops it at the end, so a
* failure belongs to the function it names and no run sees another's objects.
* Tests of databases make databases named the same way, and drop them.
*
*   Server: DJINTERP_TEST_POSTGRES_HOST (default 127.0.0.1), _PORT (5432),
* _USER (postgres), _PASSWORD (none) and _DATABASE (postgres) say where; the
* user needs to be a superuser, as the postgres user of a fresh server or of
* the official container image is, since the harness creates databases and
* extensions and checkpoints. The harness fails, saying why, when it cannot
* connect.
*
*   Build (from the repo root), at any language level from C99, against libpq
* -- or let ci/check_db_postgres.sh, which runs it across compilers,
* standards, libpq builds and servers:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc                                 \
*        -I"$(pg_config --includedir)"                                        \
*        src/djinterp/c/db/db_*.c src/djinterp/c/db/postgres/postgres_*.c     \
*        tests/djinterp/c/db/postgres/t_postgres.c -lpq -o t_postgres
*
*
* path:      /tests/djinterp/c/db/postgres/t_postgres.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.23
*******************************************************************************/
// D_CFG_ENV_USING_POSTGRESQL
//   knob (env's): on, since the harness calls libpq itself, as a program
// asking for env's detection does; the layer turns it on for its own sources
// alone.
#ifndef D_CFG_ENV_USING_POSTGRESQL
    #define D_CFG_ENV_USING_POSTGRESQL 1
#endif  // D_CFG_ENV_USING_POSTGRESQL

// std
#include <limits.h>  // INT_MAX
#include <locale.h>  // setlocale, LC_NUMERIC
#include <math.h>    // INFINITY, NAN, isinf, isnan
#include <stdio.h>   // printf, snprintf
#include <stdlib.h>  // getenv, setenv, strtoul
#include <string.h>  // memcmp, memset, strcmp, strlen, strncmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/postgres/postgres.h"  // under test
#include "../../../../../inc/djinterp/env/db/postgres/env_postgres.h"  // libpq
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t, INT64_MAX


//==============================================================================
// HARNESS
//==============================================================================

// g_checks, g_failures
//   counters: the checks run, and those that failed.
static int g_checks   = 0;
static int g_failures = 0;

// g_options
//   setting: where the server is, from the environment.
static struct d_postgres_options g_options =
{
    "127.0.0.1", "postgres", NULL, "postgres", 5432u, NULL
};

// g_run
//   setting: the prefix of every schema and database the run makes,
// d_tests_postgres_ and the setup connection's backend process id, which no
// other session has at once.
static char g_run[48] = "";

/*
d_tests_postgres_expect
  Counts one check, and prints it when it fails.
*/
static void
d_tests_postgres_expect(
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

// D_TESTS_POSTGRES_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_POSTGRES_CHECK(_cond)                                         \
    d_tests_postgres_expect((_cond), #_cond, __LINE__)

/*
d_tests_postgres_expect_result
  Counts one check of a result, and prints the server's words when the result
is not the one expected.
*/
static void
d_tests_postgres_expect_result(
    struct pg_conn* _db,
    int             _result,
    int             _expected,
    const char*     _what,
    int             _line
)
{
    char got[6]  = "";
    char want[6] = "";

    d_tests_postgres_expect(_result == _expected, _what, _line);

    // the result, as SQLSTATEs where they are, and what the connection says
    if (_result != _expected)
    {
        (void)d_postgres_result_sqlstate(_result, got);
        (void)d_postgres_result_sqlstate(_expected, want);
        printf("    got %d (%s), expected %d (%s): %s\n",
               _result,
               got,
               _expected,
               want,
               (_result > 0) ? d_postgres_error_message(_db)
                             : d_postgres_result_message(_result));
    }

    return;
}

// D_TESTS_POSTGRES_RESULT
//   macro: records a check that a call returned a given result, with the
// call's own text and line, which only the preprocessor can supply.
#define D_TESTS_POSTGRES_RESULT(_db, _call, _expected)                        \
    d_tests_postgres_expect_result((_db),                                      \
                                   (_call),                                    \
                                   (_expected),                                \
                                   #_call " == " #_expected,                   \
                                   __LINE__)

// D_TESTS_POSTGRES_OK
//   macro: records a check that a call succeeded; a macro, as
// D_TESTS_POSTGRES_RESULT is, for the call's text and line.
#define D_TESTS_POSTGRES_OK(_db, _call)                                       \
    D_TESTS_POSTGRES_RESULT((_db), (_call), 0)

//==============================================================================
// FIXTURES
//==============================================================================

// d_tests_postgres_condition
//   struct: a constant postgres_common.h names, and the SQLSTATE it packs.
struct d_tests_postgres_condition
{
    int         code;      // the constant
    const char* sqlstate;  // its five characters
};

// d_tests_postgres_conditions
//   table: every condition of enum d_postgres_sqlstate, spelled out.
static const struct d_tests_postgres_condition
d_tests_postgres_conditions[] =
{
    { D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION,           "08000" },
    { D_POSTGRES_SQLSTATE_CANNOT_CONNECT,                 "08001" },
    { D_POSTGRES_SQLSTATE_CONNECTION_FAILURE,             "08006" },
    { D_POSTGRES_SQLSTATE_FEATURE_NOT_SUPPORTED,          "0A000" },
    { D_POSTGRES_SQLSTATE_INVALID_PARAMETER_VALUE,        "22023" },
    { D_POSTGRES_SQLSTATE_UNIQUE_VIOLATION,               "23505" },
    { D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION,         "25001" },
    { D_POSTGRES_SQLSTATE_NO_ACTIVE_SQL_TRANSACTION,      "25P01" },
    { D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION,      "25P02" },
    { D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST,  "2BP01" },
    { D_POSTGRES_SQLSTATE_INVALID_SAVEPOINT_SPECIFICATION, "3B001" },
    { D_POSTGRES_SQLSTATE_INVALID_CATALOG_NAME,           "3D000" },
    { D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME,            "3F000" },
    { D_POSTGRES_SQLSTATE_SERIALIZATION_FAILURE,          "40001" },
    { D_POSTGRES_SQLSTATE_DEADLOCK_DETECTED,              "40P01" },
    { D_POSTGRES_SQLSTATE_INSUFFICIENT_PRIVILEGE,         "42501" },
    { D_POSTGRES_SQLSTATE_SYNTAX_ERROR,                   "42601" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_COLUMN,               "42701" },
    { D_POSTGRES_SQLSTATE_UNDEFINED_COLUMN,               "42703" },
    { D_POSTGRES_SQLSTATE_UNDEFINED_OBJECT,               "42704" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_OBJECT,               "42710" },
    { D_POSTGRES_SQLSTATE_UNDEFINED_FUNCTION,             "42883" },
    { D_POSTGRES_SQLSTATE_UNDEFINED_TABLE,                "42P01" },
    { D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER,            "42P02" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_DATABASE,             "42P04" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_PREPARED_STATEMENT,   "42P05" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_SCHEMA,               "42P06" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_TABLE,                "42P07" },
    { D_POSTGRES_SQLSTATE_OUT_OF_MEMORY,                  "53200" },
    { D_POSTGRES_SQLSTATE_OBJECT_IN_USE,                  "55006" },
    { D_POSTGRES_SQLSTATE_LOCK_NOT_AVAILABLE,             "55P03" },
    { D_POSTGRES_SQLSTATE_QUERY_CANCELED,                 "57014" },
    { D_POSTGRES_SQLSTATE_INTERNAL_ERROR,                 "XX000" }
};

// d_tests_postgres_feature_probe
//   struct: a feature, and SQL that runs only where the server has it, run
// inside a transaction that is rolled back.
struct d_tests_postgres_feature_probe
{
    enum d_postgres_feature feature;  // the feature
    const char*             sql;      // SQL that needs it
};

// d_tests_postgres_probes
//   table: SQL for every feature that can be tried inside a transaction;
// REINDEX CONCURRENTLY and DROP DATABASE ... WITH (FORCE), which cannot, are
// tried in d_tests_postgres_feature_statements.
static const struct d_tests_postgres_feature_probe
d_tests_postgres_probes[] =
{
    { D_POSTGRES_FEATURE_PROCEDURES,
      "CREATE PROCEDURE p_proc() LANGUAGE sql AS 'SELECT 1'" },
    { D_POSTGRES_FEATURE_COVERING_INDEXES,
      "CREATE TABLE p_cov (a int, b int); "
      "CREATE INDEX p_cov_i ON p_cov (a) INCLUDE (b)" },
    { D_POSTGRES_FEATURE_GENERATED_COLUMNS,
      "CREATE TABLE p_gen (a int, b int GENERATED ALWAYS AS (a * 2) STORED)" },
    { D_POSTGRES_FEATURE_TRUSTED_EXTENSIONS,
      "SELECT trusted FROM pg_catalog.pg_available_extension_versions" },
    { D_POSTGRES_FEATURE_MULTIRANGES,
      "SELECT '{[1,2)}'::int4multirange" },
    { D_POSTGRES_FEATURE_REPLACE_TRIGGER,
      "CREATE TABLE p_trg (a int); "
      "CREATE FUNCTION p_trg_f() RETURNS trigger LANGUAGE plpgsql "
      "AS 'BEGIN RETURN NEW; END'; "
      "CREATE OR REPLACE TRIGGER p_trg_t BEFORE INSERT ON p_trg "
      "FOR EACH ROW EXECUTE PROCEDURE p_trg_f()" },
    { D_POSTGRES_FEATURE_MERGE,
      "CREATE TABLE p_merge (a int); "
      "MERGE INTO p_merge USING (SELECT 1 AS k) AS s ON false "
      "WHEN NOT MATCHED THEN DO NOTHING" },
    { D_POSTGRES_FEATURE_NULLS_NOT_DISTINCT,
      "CREATE TABLE p_nnd (a int UNIQUE NULLS NOT DISTINCT)" },
    { D_POSTGRES_FEATURE_SQL_JSON,
      "SELECT JSON_ARRAY(1, 2)" },
    { D_POSTGRES_FEATURE_JSON_TABLE,
      "SELECT * FROM JSON_TABLE('[1]'::jsonb, '$[*]' "
      "COLUMNS (v int PATH '$')) AS t" },
    { D_POSTGRES_FEATURE_VIRTUAL_COLUMNS,
      "CREATE TABLE p_vgen (a int, b int GENERATED ALWAYS AS (a * 2) "
      "VIRTUAL)" },
    { D_POSTGRES_FEATURE_UUIDV7,
      "SELECT uuidv7()" },
    { D_POSTGRES_FEATURE_RETURNING_OLD_NEW,
      "CREATE TABLE p_ret (a int); "
      "INSERT INTO p_ret VALUES (1) RETURNING old.a, new.a" },
    { D_POSTGRES_FEATURE_JSONPATH,
      "SELECT jsonb_path_query('[1]'::jsonb, '$[*]')" }
};

// d_tests_postgres_every_type
//   table: one column of every d_db_type NONE excepted, which the server
// refuses, the first the key.
static const struct d_db_column
d_tests_postgres_every_type[] =
{
    { "id",       D_DB_TYPE_BIG_INTEGER,
      D_DB_COLUMN_PRIMARY_KEY | D_DB_COLUMN_AUTO_INCREMENT,
      NULL,       0u,  0u, NULL, 0u },
    { "flag",     D_DB_TYPE_BOOLEAN,        D_DB_COLUMN_NOT_NULL,
      "TRUE",     0u,  0u, NULL, 0u },
    { "count",    D_DB_TYPE_INTEGER,        0u, "0",    0u,  0u, NULL, 0u },
    { "ratio",    D_DB_TYPE_FLOATING_POINT, 0u, NULL,   0u,  0u, NULL, 0u },
    { "price",    D_DB_TYPE_DECIMAL,        0u, "1.50", 10u, 2u, NULL, 0u },
    { "plain",    D_DB_TYPE_DECIMAL,        0u, NULL,   0u,  0u, NULL, 0u },
    { "name",     D_DB_TYPE_STRING,
      D_DB_COLUMN_NOT_NULL | D_DB_COLUMN_UNIQUE,
      "'none'",   80u, 0u, NULL, 0u },
    { "note",     D_DB_TYPE_STRING,         0u, NULL,   0u,  0u, NULL, 0u },
    { "tag",      D_DB_TYPE_STRING,         D_DB_COLUMN_UNIQUE,
      NULL,       0u,  0u, NULL, 0u },
    { "blob",     D_DB_TYPE_BINARY,         0u, NULL,   0u,  0u, NULL, 0u },
    { "digest",   D_DB_TYPE_BINARY,         0u, NULL,   32u, 0u, NULL, 0u },
    { "born",     D_DB_TYPE_DATE,           0u, NULL,   0u,  0u, NULL, 0u },
    { "alarm",    D_DB_TYPE_TIME,           0u, NULL,   3u,  0u, NULL, 0u },
    { "seen",     D_DB_TYPE_DATETIME,       0u, NULL,   0u,  0u, NULL, 0u },
    { "stamp",    D_DB_TYPE_TIMESTAMP,      0u, NULL,   6u,  0u, NULL, 0u },
    { "doc",      D_DB_TYPE_JSON,           0u, NULL,   0u,  0u, NULL, 0u },
    { "page",     D_DB_TYPE_XML,            0u, NULL,   0u,  0u, NULL, 0u },
    { "uuid",     D_DB_TYPE_UUID,           0u, NULL,   0u,  0u, NULL, 0u },
    { "own",      D_DB_TYPE_NONE,           0u, NULL,   0u,  0u, "smallint",
      0u }
};

// d_tests_postgres_every_type_sql
//   table: each column of d_tests_postgres_every_type as format_type writes
// its type, the same on every supported server.
static const char* const
d_tests_postgres_every_type_sql[] =
{
    "bigint",                      "boolean",
    "integer",                     "double precision",
    "numeric(10,2)",               "numeric",
    "character varying(80)",       "text",
    "text",                        "bytea",
    "bytea",                       "date",
    "time(3) without time zone",   "timestamp without time zone",
    "timestamp(6) with time zone", "jsonb",
    "xml",                         "uuid",
    "smallint"
};

// d_tests_postgres_composite
//   table: a two-column key, the second column listed first.
static const struct d_db_column
d_tests_postgres_composite[] =
{
    { "label",  D_DB_TYPE_STRING,  0u,                      NULL, 0u, 0u,
      NULL, 0u },
    { "code",   D_DB_TYPE_INTEGER, D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u,
      NULL, 0u },
    { "region", D_DB_TYPE_STRING,  D_DB_COLUMN_PRIMARY_KEY, NULL, 8u, 0u,
      NULL, 0u }
};

// d_tests_postgres_ordered
//   table: a composite key whose places order it b, a: otherwise than the
// table.
static const struct d_db_column
d_tests_postgres_ordered[] =
{
    { "a", D_DB_TYPE_INTEGER, D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u, NULL, 2u },
    { "b", D_DB_TYPE_INTEGER, D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u, NULL, 1u },
    { "c", D_DB_TYPE_INTEGER, 0u,                      NULL, 0u, 0u, NULL, 0u }
};

// d_tests_postgres_refused
//   table: column sets the layer refuses before the server sees them, each
// with its length: NONE without type_sql, a type outside the enumeration, an
// unknown flag, an empty name, a numeric scale without a size,
// AUTO_INCREMENT off the key, and on a composite key.
static const struct d_db_column
d_tests_postgres_refused[][2] =
{
    { { "a", D_DB_TYPE_NONE,     0u,    NULL, 0u, 0u, NULL, 0u } },
    { { "a", (enum d_db_type)99, 0u,    NULL, 0u, 0u, NULL, 0u } },
    { { "a", D_DB_TYPE_INTEGER,  0x80u, NULL, 0u, 0u, NULL, 0u } },
    { { "",  D_DB_TYPE_INTEGER,  0u,    NULL, 0u, 0u, NULL, 0u } },
    { { "a", D_DB_TYPE_DECIMAL,  0u,    NULL, 0u, 2u, NULL, 0u } },
    { { "a", D_DB_TYPE_INTEGER,  D_DB_COLUMN_AUTO_INCREMENT, NULL, 0u, 0u,
        NULL, 0u } },
    { { "a", D_DB_TYPE_INTEGER,
        D_DB_COLUMN_PRIMARY_KEY | D_DB_COLUMN_AUTO_INCREMENT, NULL, 0u, 0u,
        NULL, 0u },
      { "b", D_DB_TYPE_INTEGER,  D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u,
        NULL, 0u } }
};

// d_tests_postgres_refused_counts
//   table: how many columns each set of d_tests_postgres_refused holds.
static const size_t d_tests_postgres_refused_counts[] =
{
    1u, 1u, 1u, 1u, 1u, 1u, 2u
};

// d_tests_postgres_defaults_sql
//   setting: a table whose columns have every kind of default, to read back.
static const char* const d_tests_postgres_defaults_sql =
    "id integer PRIMARY KEY, a varchar(10) DEFAULT 'abc', b integer DEFAULT 5, "
    "c timestamptz DEFAULT now(), d bytea DEFAULT '\\x7879', "
    "e numeric(5,2) DEFAULT 1.50, f varchar(10) DEFAULT NULL, "
    "g varchar(10) NOT NULL, j date DEFAULT '2020-01-02', "
    "l varchar(10) DEFAULT 'it''s', n varchar(10) DEFAULT 'NULL', "
    "q char(3) DEFAULT '', r double precision DEFAULT -0.5, "
    "s varchar(20) DEFAULT 'back\\slash', "
    "t bigint GENERATED BY DEFAULT AS IDENTITY";

// d_tests_postgres_default_reads
//   table: a column of d_tests_postgres_defaults_sql, a query its default
// fills in for %s, and what the query must read.
static const char* const
d_tests_postgres_default_reads[][3] =
{
    { "a", "SELECT %s",                     "abc"        },
    { "b", "SELECT %s",                     "5"          },
    { "d", "SELECT encode(%s, 'escape')",   "xy"         },
    { "e", "SELECT %s",                     "1.50"       },
    { "j", "SELECT %s",                     "2020-01-02" },
    { "l", "SELECT %s",                     "it's"       },
    { "n", "SELECT %s",                     "NULL"       },
    { "q", "SELECT length(%s)",             "0"          },
    { "r", "SELECT %s",                     "-0.5"       },
    { "s", "SELECT %s",                     "back\\slash" }
};

/*
d_tests_postgres_setting
  An environment variable, or a default when it is unset or empty.
*/
static const char*
d_tests_postgres_setting(
    const char* _name,
    const char* _default
)
{
    const char* const value = getenv(_name);

    return ( (value) && (value[0] != '\0') ) ? value : _default;
}

/*
d_tests_postgres_configure
  g_options from the environment.
*/
static void
d_tests_postgres_configure(
    void
)
{
    g_options.host     = d_tests_postgres_setting(
                             "DJINTERP_TEST_POSTGRES_HOST", "127.0.0.1");
    g_options.user     = d_tests_postgres_setting(
                             "DJINTERP_TEST_POSTGRES_USER", "postgres");
    g_options.password = d_tests_postgres_setting(
                             "DJINTERP_TEST_POSTGRES_PASSWORD", NULL);
    g_options.database = d_tests_postgres_setting(
                             "DJINTERP_TEST_POSTGRES_DATABASE", "postgres");
    g_options.port     = (unsigned)strtoul(
        d_tests_postgres_setting("DJINTERP_TEST_POSTGRES_PORT", "5432"),
        NULL,
        10);

    return;
}

/*
d_tests_postgres_connect
  A connection as the run's settings say, to the run's database or another.
*/
static struct pg_conn*
d_tests_postgres_connect(
    const char* _database
)
{
    struct pg_conn*           db      = NULL;
    char*                     reason  = NULL;
    struct d_postgres_options options = g_options;

    if (_database)
    {
        options.database = _database;
    }

    // a connection, or a reason
    if (d_postgres_connect(&options, &db, &reason) != 0)
    {
        printf("  cannot connect to %s:%u as %s: %s\n",
               options.host,
               options.port,
               options.user,
               (reason) ? reason : "no reason given");
    }

    d_db_free(reason);

    return db;
}

/*
d_tests_postgres_schema_name
  The schema, or database, a test works in: the run's prefix and the test's
word.
*/
static void
d_tests_postgres_schema_name(
    char*       _out,
    size_t      _size,
    const char* _word
)
{
    snprintf(_out, _size, "%s_%s", g_run, _word);

    return;
}

/*
d_tests_postgres_begin
  A fresh schema for one test, and a connection with it alone on the search
path, so that it is the current schema.
*/
static struct pg_conn*
d_tests_postgres_begin(
    const char* _word,
    char*       _schema,
    size_t      _size
)
{
    struct pg_conn* setup = d_tests_postgres_connect(NULL);
    struct pg_conn* db    = NULL;

    d_tests_postgres_schema_name(_schema, _size, _word);

    // a schema of the test's own, empty
    if (setup)
    {
        D_TESTS_POSTGRES_OK(setup,
                            d_postgres_drop_schema(setup,
                                                   _schema,
                                                   D_DB_IF_EXISTS |
                                                   D_POSTGRES_CASCADE));
        D_TESTS_POSTGRES_OK(setup,
                            d_postgres_create_schema(setup, _schema, 0u));
        d_postgres_close(setup);
    }

    db = d_tests_postgres_connect(NULL);

    // the schema current, quoted as the search path writes names
    if (db)
    {
        char* const path = d_postgres_quote_identifier(_schema);

        D_TESTS_POSTGRES_OK(db, d_postgres_set_search_path(db, path));
        d_db_free(path);
    }

    return db;
}

/*
d_tests_postgres_end
  Closes the test's connection and drops its schema.
*/
static void
d_tests_postgres_end(
    struct pg_conn* _db,
    const char*     _schema
)
{
    struct pg_conn* setup = NULL;

    d_postgres_close(_db);
    setup = d_tests_postgres_connect(NULL);

    // the schema goes, whatever the test left in it
    if (setup)
    {
        D_TESTS_POSTGRES_OK(setup,
                            d_postgres_drop_schema(setup,
                                                   _schema,
                                                   D_DB_IF_EXISTS |
                                                   D_POSTGRES_CASCADE));
        d_postgres_close(setup);
    }

    return;
}

/*
d_tests_postgres_int
  A single integer from SQL, or INT64_MIN when there is none.
*/
static int64_t
d_tests_postgres_int(
    struct pg_conn* _db,
    const char*     _sql
)
{
    int64_t value = INT64_MIN;

    // the value, or the sentinel
    if (d_postgres_query_int64(_db, &value, _sql) != 0)
    {
        return INT64_MIN;
    }

    return value;
}

/*
d_tests_postgres_text_is
  Whether a query's single value is the text expected, NULL for NULL.
*/
static bool
d_tests_postgres_text_is(
    struct pg_conn* _db,
    const char*     _sql,
    const char*     _expected
)
{
    char*      text   = NULL;
    const int  result = d_postgres_query_text(_db, &text, _sql);
    const bool same   = (result == 0) &&
                        ( (text)
                              ? ( (_expected) &&
                                  (strcmp(text, _expected) == 0) )
                              : (!_expected) );

    // say what came instead
    if (!same)
    {
        printf("    %s: got %s, expected %s\n",
               _sql,
               (result != 0) ? "a failure" : (text) ? text : "NULL",
               (_expected) ? _expected : "NULL");
    }

    d_db_free(text);

    return same;
}

/*
d_tests_postgres_has_name
  Whether a name list holds a name, exactly.
*/
static bool
d_tests_postgres_has_name(
    const struct d_db_name_list* _names,
    const char*                  _name
)
{
    for (size_t i = 0u; i < _names->count; ++i)
    {
        if (strcmp(_names->names[i], _name) == 0)
        {
            return true;
        }
    }

    return false;
}

/*
d_tests_postgres_count_name
  How many times a name list holds a name.
*/
static size_t
d_tests_postgres_count_name(
    const struct d_db_name_list* _names,
    const char*                  _name
)
{
    size_t count = 0u;

    for (size_t i = 0u; i < _names->count; ++i)
    {
        count += (strcmp(_names->names[i], _name) == 0) ? 1u : 0u;
    }

    return count;
}

/*
d_tests_postgres_sorted
  Whether a name list is in byte order, as the layer promises, repeats
allowed, and ends with its NULL.
*/
static bool
d_tests_postgres_sorted(
    const struct d_db_name_list* _names
)
{
    for (size_t i = 1u; i < _names->count; ++i)
    {
        if (strcmp(_names->names[i - 1u], _names->names[i]) > 0)
        {
            return false;
        }
    }

    return (_names->names[_names->count] == NULL);
}

/*
d_tests_postgres_find_column
  A column of a list by name, or NULL.
*/
static const struct d_db_column_info*
d_tests_postgres_find_column(
    const struct d_db_column_list* _columns,
    const char*                    _name
)
{
    for (size_t i = 0u; i < _columns->count; ++i)
    {
        if (strcmp(_columns->columns[i].name, _name) == 0)
        {
            return &_columns->columns[i];
        }
    }

    return NULL;
}

/*
d_tests_postgres_same_page
  Whether a page of rows read a few at a time holds the columns of the rows
read all at once, and `_count` of their rows from `_first`, value for value.
*/
static bool
d_tests_postgres_same_page(
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
                    same = ( (got->size == 0u) ||
                             (memcmp(got->as.blob, want->as.blob, got->size) ==
                              0) );

                    break;

                default:
                    break;
            }
        }

        // a value read otherwise
        if (!same)
        {
            return false;
        }
    }

    return true;
}

/*
d_tests_postgres_at_least
  Whether the server is that major version or later.
*/
static bool
d_tests_postgres_at_least(
    const struct pg_conn* _db,
    int                   _major
)
{
    return (d_postgres_server_version_number(_db) >= (_major * 10000));
}

/*
d_tests_postgres_works
  Whether SQL runs inside a transaction that is then rolled back, so that a
probe leaves nothing behind, and its failure touches nothing else.
*/
static bool
d_tests_postgres_works(
    struct pg_conn* _db,
    const char*     _sql
)
{
    bool works = false;

    // the transaction, the SQL, and the rollback whatever happened
    if (d_postgres_begin_transaction(_db, D_POSTGRES_TRANSACTION_DEFAULT) == 0)
    {
        works = (d_postgres_exec(_db, _sql) == 0);
        (void)d_postgres_rollback(_db);
    }

    return works;
}

/*
d_tests_postgres_function
  The trigger function the trigger tests attach, in the current schema.
*/
static int
d_tests_postgres_function(
    struct pg_conn* _db
)
{
    return d_postgres_exec(_db,
                           "CREATE FUNCTION t_audit() RETURNS trigger "
                           "LANGUAGE plpgsql AS 'BEGIN RETURN NEW; END'");
}

//==============================================================================
// db_common.h
//==============================================================================

/*
d_tests_postgres_db_common
  db_common.h's vocabulary, as this layer meets it.
  Tests the following:
  - the status codes are 0 and distinct negatives, MISMATCH among them,
    each with a message, and any other value without one
  - the release functions accept NULL and empty lists
*/
static void
d_tests_postgres_db_common(
    void
)
{
    struct d_db_name_list   names   = { 0u, NULL };
    struct d_db_column_list columns = { 0u, NULL };

    printf("db_common\n");

    D_TESTS_POSTGRES_CHECK(D_DB_OK == 0);
    D_TESTS_POSTGRES_CHECK(D_DB_INVALID_ARGUMENT < 0);
    D_TESTS_POSTGRES_CHECK(D_DB_INVALID_ARGUMENT != D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_CHECK(D_DB_INVALID_ARGUMENT != D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(d_db_status_message(D_DB_UNSUPPORTED) != NULL);
    D_TESTS_POSTGRES_CHECK(d_db_status_message(D_DB_NOT_FOUND) != NULL);
    D_TESTS_POSTGRES_CHECK(D_DB_MISMATCH < 0);
    D_TESTS_POSTGRES_CHECK(D_DB_MISMATCH != D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_db_status_message(D_DB_MISMATCH) != NULL);
    D_TESTS_POSTGRES_CHECK(d_db_status_message(D_DB_WAIT) != NULL);
    D_TESTS_POSTGRES_CHECK(d_db_status_message(D_DB_WAIT - 1) == NULL);
    D_TESTS_POSTGRES_CHECK(
        d_db_status_message(D_POSTGRES_SQLSTATE_UNDEFINED_TABLE) == NULL);

    d_db_name_list_free(NULL);
    d_db_column_list_free(NULL);
    d_db_name_list_free(&names);
    d_db_column_list_free(&columns);
    d_db_free(NULL);
    D_TESTS_POSTGRES_CHECK( (names.count == 0u) && (names.names == NULL) );
    D_TESTS_POSTGRES_CHECK( (columns.count == 0u) &&
                            (columns.columns == NULL) );

    return;
}

//==============================================================================
// postgres_common.h
//==============================================================================

/*
d_tests_postgres_sqlstates
  Results as packed SQLSTATEs.
  Tests the following:
  - every named constant packs the SQLSTATE the manual gives its condition,
    and spells back to it
  - "00000" is 0, and 0 spells "00000"
  - text of any other shape, and NULL, is D_DB_INVALID_ARGUMENT
  - a negative result, one past 30 bits, or one holding a character no
    SQLSTATE holds, spells "" and false; so does a NULL buffer
*/
static void
d_tests_postgres_sqlstates(
    void
)
{
    const size_t count      = sizeof(d_tests_postgres_conditions) /
                              sizeof(d_tests_postgres_conditions[0]);
    char         spelled[6] = "";

    printf("postgres_common: SQLSTATEs\n");

    for (size_t i = 0u; i < count; ++i)
    {
        const struct d_tests_postgres_condition* const condition =
            &d_tests_postgres_conditions[i];

        D_TESTS_POSTGRES_CHECK(
            d_postgres_result_from_sqlstate(condition->sqlstate) ==
            condition->code);
        D_TESTS_POSTGRES_CHECK(
            (d_postgres_result_sqlstate(condition->code, spelled)) &&
            (strcmp(spelled, condition->sqlstate) == 0));
        D_TESTS_POSTGRES_CHECK(condition->code > 0);
    }

    D_TESTS_POSTGRES_CHECK(d_postgres_result_from_sqlstate("00000") == 0);
    D_TESTS_POSTGRES_CHECK( (d_postgres_result_sqlstate(0, spelled)) &&
                            (strcmp(spelled, "00000") == 0) );
    D_TESTS_POSTGRES_CHECK(d_postgres_result_from_sqlstate(NULL) ==
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_postgres_result_from_sqlstate("") ==
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_postgres_result_from_sqlstate("4201") ==
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_postgres_result_from_sqlstate("42P011") ==
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_postgres_result_from_sqlstate("42p01") ==
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_postgres_result_from_sqlstate("42:01") ==
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK( (!d_postgres_result_sqlstate(-1, spelled)) &&
                            (spelled[0] == '\0') );
    D_TESTS_POSTGRES_CHECK( (!d_postgres_result_sqlstate(1 << 30, spelled)) &&
                            (spelled[0] == '\0') );
    D_TESTS_POSTGRES_CHECK( (!d_postgres_result_sqlstate(10, spelled)) &&
                            (spelled[0] == '\0') );
    D_TESTS_POSTGRES_CHECK(!d_postgres_result_sqlstate(0, NULL));

    return;
}

/*
d_tests_postgres_messages
  Result messages.
  Tests the following:
  - db_common.h's own values in its own words
  - the four codes the layer gives itself, in its words, distinct
  - a named condition in the manual's words, an unnamed code in its class's,
    and a code of no known class as such
  - a value no result can be is "unknown result"
*/
static void
d_tests_postgres_messages(
    void
)
{
    printf("postgres_common: messages\n");

    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(D_DB_INVALID_ARGUMENT),
               d_db_status_message(D_DB_INVALID_ARGUMENT)) == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(0), d_db_status_message(0)) == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(
                   D_POSTGRES_SQLSTATE_CONNECTION_FAILURE),
               "the connection to the server was lost") == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(D_POSTGRES_SQLSTATE_CANNOT_CONNECT),
               d_postgres_result_message(
                   D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION)) != 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(D_POSTGRES_SQLSTATE_OUT_OF_MEMORY),
               "out of memory") == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(D_POSTGRES_SQLSTATE_UNDEFINED_TABLE),
               "undefined table") == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(
                   D_POSTGRES_SQLSTATE_DUPLICATE_PREPARED_STATEMENT),
               "duplicate prepared statement") == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(
                   D_POSTGRES_SQLSTATE_PROTOCOL_VIOLATION),
               "protocol violation") == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(
                   D_POSTGRES_SQLSTATE_INVALID_TEXT_REPRESENTATION),
               "invalid text representation") == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(D_DB_MISMATCH),
               d_db_status_message(D_DB_MISMATCH)) == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(
                   d_postgres_result_from_sqlstate("42P10")),
               "syntax error or access rule violation") == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(
                   d_postgres_result_from_sqlstate("23503")),
               "integrity constraint violation") == 0);
    D_TESTS_POSTGRES_CHECK(
        strstr(d_postgres_result_message(
                   d_postgres_result_from_sqlstate("ZZ000")),
               "class") != NULL);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(-99), "unknown result") == 0);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(10), "unknown result") == 0);

    return;
}

/*
d_tests_postgres_client
  The client library.
  Tests the following:
  - its version is libpq's own, PQlibVersion's, at or above the floor
  - what it can do, by the header's feature macros and the library's
    release: pipelines (14), chunked rows and closing a statement (17); a
    value outside the enumeration is no feature
*/
static void
d_tests_postgres_client(
    void
)
{
    const int library = PQlibVersion();

    printf("postgres_common: client (libpq %d)\n",
           d_postgres_client_version_number());

    D_TESTS_POSTGRES_CHECK(d_postgres_client_version_number() == library);
    D_TESTS_POSTGRES_CHECK(d_postgres_client_version_number() >=
                           D_POSTGRES_FLOOR_CLIENT_VERSION_NUM);
#if defined(LIBPQ_HAS_PIPELINING)
    D_TESTS_POSTGRES_CHECK(d_postgres_client_supports(
                               D_POSTGRES_CLIENT_PIPELINE) ==
                           (library >= 140000));
#else
    D_TESTS_POSTGRES_CHECK(!d_postgres_client_supports(
                               D_POSTGRES_CLIENT_PIPELINE));
#endif
#if defined(LIBPQ_HAS_CHUNK_MODE)
    D_TESTS_POSTGRES_CHECK(d_postgres_client_supports(
                               D_POSTGRES_CLIENT_CHUNKED_ROWS) ==
                           (library >= 170000));
#else
    D_TESTS_POSTGRES_CHECK(!d_postgres_client_supports(
                               D_POSTGRES_CLIENT_CHUNKED_ROWS));
#endif
#if defined(LIBPQ_HAS_CLOSE_PREPARED)
    D_TESTS_POSTGRES_CHECK(d_postgres_client_supports(
                               D_POSTGRES_CLIENT_CLOSE_PREPARED) ==
                           (library >= 170000));
#else
    D_TESTS_POSTGRES_CHECK(!d_postgres_client_supports(
                               D_POSTGRES_CLIENT_CLOSE_PREPARED));
#endif
    D_TESTS_POSTGRES_CHECK(!d_postgres_client_supports(
                               (enum d_postgres_client_feature)99));
    D_TESTS_POSTGRES_CHECK(!d_postgres_client_supports(
                               (enum d_postgres_client_feature)-1));

    return;
}

/*
d_tests_postgres_versions
  The server's version.
  Tests the following:
  - a version at or above the floor, as the server itself reports it
  - the string begins with the version's major number
  - server_at_least, below, at and above the version, by both parts
  - NULL answers NULL, 0 and false
*/
static void
d_tests_postgres_versions(
    void
)
{
    struct pg_conn* const db = d_tests_postgres_connect(NULL);

    printf("postgres_common: versions (%s)\n",
           (db) ? d_postgres_server_version(db) : "no server");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    const int         version  = d_postgres_server_version_number(db);
    const unsigned    major    = (unsigned)(version / 10000);
    const unsigned    minor    = (unsigned)(version % 10000);
    const char* const text     = d_postgres_server_version(db);
    char              lead[16] = "";

    snprintf(lead, sizeof(lead), "%u", major);
    D_TESTS_POSTGRES_CHECK(version >= D_POSTGRES_FLOOR_SERVER_VERSION_NUM);
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db,
                             "SELECT current_setting('server_version_num')"
                             "::integer") == version);
    D_TESTS_POSTGRES_CHECK( (text) &&
                            (strncmp(text, lead, strlen(lead)) == 0) );
    D_TESTS_POSTGRES_CHECK(d_postgres_server_at_least(db, 10u, 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_server_at_least(db, 9u, 6u));
    D_TESTS_POSTGRES_CHECK(d_postgres_server_at_least(db, major, minor));
    D_TESTS_POSTGRES_CHECK(!d_postgres_server_at_least(db, major,
                                                       minor + 1u));
    D_TESTS_POSTGRES_CHECK(!d_postgres_server_at_least(db, major + 1u, 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_server_version(NULL) == NULL);
    D_TESTS_POSTGRES_CHECK(d_postgres_server_version_number(NULL) == 0);
    D_TESTS_POSTGRES_CHECK(!d_postgres_server_at_least(NULL, 0u, 0u));

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_features
  d_postgres_supports, against the server itself.
  Tests the following:
  - each feature that can be tried in a transaction is supported exactly
    when the server runs SQL that needs it
  - a value outside the enumeration, and a NULL connection, support nothing
*/
static void
d_tests_postgres_features(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("features",
                                                        schema,
                                                        sizeof(schema));
    const size_t    count      = sizeof(d_tests_postgres_probes) /
                                 sizeof(d_tests_postgres_probes[0]);

    printf("postgres_common: features\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    for (size_t i = 0u; i < count; ++i)
    {
        const struct d_tests_postgres_feature_probe* const probe =
            &d_tests_postgres_probes[i];
        const bool works    = d_tests_postgres_works(db, probe->sql);
        const bool supports = d_postgres_supports(db, probe->feature);

        D_TESTS_POSTGRES_CHECK(works == supports);

        if (works != supports)
        {
            printf("    feature %d: supports says %d, the server %d\n",
                   (int)probe->feature,
                   (int)supports,
                   (int)works);
        }
    }

    D_TESTS_POSTGRES_CHECK(!d_postgres_supports(db,
                                                (enum d_postgres_feature)99));
    D_TESTS_POSTGRES_CHECK(!d_postgres_supports(NULL,
                                                D_POSTGRES_FEATURE_MERGE));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_feature_statements
  The features whose statements cannot run inside a transaction.
  Tests the following:
  - REINDEX CONCURRENTLY runs exactly where REINDEX_CONCURRENTLY says
  - DROP DATABASE ... WITH (FORCE) parses exactly where DROP_DATABASE_FORCE
    says, run on a database that is not there
*/
static void
d_tests_postgres_feature_statements(
    void
)
{
    char            schema[96] = "";
    char            absent[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("featstmt",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_common: features outside transactions\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    d_tests_postgres_schema_name(absent, sizeof(absent), "never");
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE r (a int)"));
    D_TESTS_POSTGRES_CHECK(
        (d_postgres_exec(db, "REINDEX TABLE CONCURRENTLY r") == 0) ==
        d_postgres_supports(db, D_POSTGRES_FEATURE_REINDEX_CONCURRENTLY));
    D_TESTS_POSTGRES_CHECK(
        (d_postgres_execf(db,
                          "DROP DATABASE IF EXISTS %I WITH (FORCE)",
                          absent) == 0) ==
        d_postgres_supports(db, D_POSTGRES_FEATURE_DROP_DATABASE_FORCE));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_reported_parameters
  What the server reports of the session.
  Tests the following:
  - server_version is the version d_postgres_server_version gives
  - integer_datetimes is on, as every supported server has it
  - a setting the server reports follows a SET, with no round trip
  - an unreported name, and NULL arguments, answer NULL
*/
static void
d_tests_postgres_reported_parameters(
    void
)
{
    struct pg_conn* db = d_tests_postgres_connect(NULL);

    printf("postgres_common: reported parameters\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_CHECK(
        (d_postgres_parameter_status(db, "server_version")) &&
        (strcmp(d_postgres_parameter_status(db, "server_version"),
                d_postgres_server_version(db)) == 0));
    D_TESTS_POSTGRES_CHECK(
        (d_postgres_parameter_status(db, "integer_datetimes")) &&
        (strcmp(d_postgres_parameter_status(db, "integer_datetimes"),
                "on") == 0));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db, "SET application_name = 'dj'"));
    D_TESTS_POSTGRES_CHECK(
        (d_postgres_parameter_status(db, "application_name")) &&
        (strcmp(d_postgres_parameter_status(db, "application_name"),
                "dj") == 0));
    D_TESTS_POSTGRES_CHECK(d_postgres_parameter_status(db, "work_mem") ==
                           NULL);
    D_TESTS_POSTGRES_CHECK(d_postgres_parameter_status(db, NULL) == NULL);
    D_TESTS_POSTGRES_CHECK(d_postgres_parameter_status(NULL,
                                                       "server_version") ==
                           NULL);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_errors
  The connection's error message.
  Tests the following:
  - libpq's words after a failure, and none after a success
  - a result of the layer's own leaves the message as it was
  - NULL answers ""
*/
static void
d_tests_postgres_errors(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("errors",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_common: errors\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db, "SELECT * FROM no_table"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_CHECK(strstr(d_postgres_error_message(db),
                                  "no_table") != NULL);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(strstr(d_postgres_error_message(db),
                                  "no_table") != NULL);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "SELECT 1"));
    D_TESTS_POSTGRES_CHECK(strcmp(d_postgres_error_message(db), "") == 0);
    D_TESTS_POSTGRES_CHECK(strcmp(d_postgres_error_message(NULL), "") == 0);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_own_results
  d_postgres_result_is_own.
  Tests the following:
  - a transaction already open, a failed one at COMMIT, and a parameter a
    statement lacks are the layer's own, its message the result's
  - the same codes from the server are not, and the connection's message is
    the server's: CREATE DATABASE in a transaction (25001), a schema that
    does not exist (3F000), a statement in a failed transaction (25P02)
  - libpq's answer since forgets the layer's own
  - negative results, memory and NULL are always the layer's own
*/
static void
d_tests_postgres_own_results(
    void
)
{
    char                         schema[96] = "";
    struct pg_conn*              db         = d_tests_postgres_begin(
                                                  "own_results",
                                                  schema,
                                                  sizeof(schema));
    struct d_postgres_statement* statement  = NULL;

    printf("postgres_common: own results\n");

    D_TESTS_POSTGRES_CHECK(d_postgres_result_is_own(NULL, D_DB_UNSUPPORTED));
    D_TESTS_POSTGRES_CHECK(d_postgres_result_is_own(
                               NULL,
                               D_POSTGRES_SQLSTATE_OUT_OF_MEMORY));
    D_TESTS_POSTGRES_CHECK(!d_postgres_result_is_own(
                               NULL,
                               D_POSTGRES_SQLSTATE_UNDEFINED_TABLE));

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    // 25001: the layer's own at BEGIN, the server's at CREATE DATABASE
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db,
                                D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_begin_transaction(
                                db,
                                D_POSTGRES_TRANSACTION_DEFAULT),
                            D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION);
    D_TESTS_POSTGRES_CHECK(d_postgres_result_is_own(
                               db,
                               D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db,
                                            "CREATE DATABASE d_tests_never"),
                            D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION);
    D_TESTS_POSTGRES_CHECK(!d_postgres_result_is_own(
                               db,
                               D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION));
    D_TESTS_POSTGRES_CHECK(strstr(d_postgres_error_message(db),
                                  "CREATE DATABASE") != NULL);

    // 25P02: the server's in a failed transaction, the layer's at COMMIT
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db, "SELECT 1"),
                            D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
    D_TESTS_POSTGRES_CHECK(!d_postgres_result_is_own(
                               db,
                               D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION));
    D_TESTS_POSTGRES_CHECK(strstr(d_postgres_error_message(db),
                                  "aborted") != NULL);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_commit(db),
                            D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
    D_TESTS_POSTGRES_CHECK(d_postgres_result_is_own(
                               db,
                               D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION));

    // libpq's answer since forgets it
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "SELECT 1"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_result_is_own(
                               db,
                               D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION));

    // 3F000: the server's, for a schema that does not exist
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db,
                                            "CREATE TABLE d_tests_none.t "
                                            "(a integer)"),
                            D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME);
    D_TESTS_POSTGRES_CHECK(!d_postgres_result_is_own(
                               db,
                               D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME));
    D_TESTS_POSTGRES_CHECK(strstr(d_postgres_error_message(db),
                                  "d_tests_none") != NULL);

    // 42P02: the layer's own, for a parameter the statement lacks
    D_TESTS_POSTGRES_OK(db, d_postgres_prepare(db,
                                               &statement,
                                               "SELECT $1::integer"));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_bind_int64(statement, 2u, 1),
                            D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER);
    D_TESTS_POSTGRES_CHECK(d_postgres_result_is_own(
                               db,
                               D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER));
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(statement));

    d_tests_postgres_end(db, schema);

    return;
}

//==============================================================================
// postgres_connection.h
//==============================================================================

/*
d_tests_postgres_conninfo_is
  Whether libpq holds a value for one of a connection's keywords.
*/
static bool
d_tests_postgres_conninfo_is(
    struct pg_conn* _db,
    const char*     _keyword,
    const char*     _expected
)
{
    PQconninfoOption* const options = PQconninfo(_db);
    bool                    same    = false;

    for (const PQconninfoOption* option = options;
         (option) && (option->keyword);
         ++option)
    {
        if (strcmp(option->keyword, _keyword) == 0)
        {
            same = ( (option->val) &&
                     (strcmp(option->val, _expected) == 0) );

            break;
        }
    }

    PQconninfoFree(options);

    return same;
}

/*
d_tests_postgres_float_digits
  extra_float_digits on every connection the layer makes.
  Tests the following:
  - 3, from the layer's startup option
  - 3 where options of the caller's replace the layer's without naming it,
    set on the connection
  - the caller's own value where its options name one
*/
static void
d_tests_postgres_float_digits(
    void
)
{
    struct pg_conn* db         = d_tests_postgres_connect(NULL);
    char            port[16]   = "";
    const char*     keywords[] = { "host", "port", "user", "password",
                                   "dbname", "options" };
    const char*     values[]   = { g_options.host, port, g_options.user,
                                   g_options.password, g_options.database,
                                   "-c application_name=d_tests_digits" };

    printf("postgres_connection: extra_float_digits\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    (void)snprintf(port, sizeof(port), "%u", g_options.port);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               db,
                               "SHOW extra_float_digits",
                               "3"));
    d_postgres_close(db);
    db = NULL;
    D_TESTS_POSTGRES_OK(NULL, d_postgres_connect_with(NULL,
                                                      keywords,
                                                      values,
                                                      6u,
                                                      &db,
                                                      NULL));
    D_TESTS_POSTGRES_CHECK( (db) &&
                            (d_tests_postgres_text_is(
                                 db,
                                 "SHOW extra_float_digits",
                                 "3")) );
    d_postgres_close(db);
    db        = NULL;
    values[5] = "-c extra_float_digits=0";
    D_TESTS_POSTGRES_OK(NULL, d_postgres_connect_with(NULL,
                                                      keywords,
                                                      values,
                                                      6u,
                                                      &db,
                                                      NULL));
    D_TESTS_POSTGRES_CHECK( (db) &&
                            (d_tests_postgres_text_is(
                                 db,
                                 "SHOW extra_float_digits",
                                 "0")) );
    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_connecting
  d_postgres_connect.
  Tests the following:
  - a connection as the settings say, with no reason given
  - it is libpq's own PGconn, which libpq's calls take as they are
  - a NULL handle to fill is D_DB_INVALID_ARGUMENT
  - close takes NULL and a connection alike, and returns 0
*/
static void
d_tests_postgres_connecting(
    void
)
{
    struct pg_conn* db     = NULL;
    char*           reason = NULL;

    printf("postgres_connection: connecting\n");

    D_TESTS_POSTGRES_OK(db, d_postgres_connect(&g_options, &db, &reason));
    D_TESTS_POSTGRES_CHECK(db != NULL);
    D_TESTS_POSTGRES_CHECK(reason == NULL);

    // libpq's own calls on the same pointer
    if (db)
    {
        PGconn* const native = db;

        D_TESTS_POSTGRES_CHECK(PQstatus(native) == CONNECTION_OK);
        D_TESTS_POSTGRES_CHECK(strcmp(PQdb(native), g_options.database) == 0);
    }

    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect(&g_options, NULL, &reason),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(reason == NULL);
    D_TESTS_POSTGRES_CHECK(d_postgres_close(db) == 0);
    D_TESTS_POSTGRES_CHECK(d_postgres_close(NULL) == 0);

    return;
}

/*
d_tests_postgres_connect_failures
  Connections that cannot be made.
  Tests the following:
  - a wrong password and a port with no server are 08001, the handle NULL,
    and the reason libpq's words, without their final newline
  - a NULL reason is accepted
*/
static void
d_tests_postgres_connect_failures(
    void
)
{
    struct pg_conn*           db      = NULL;
    char*                     reason  = NULL;
    struct d_postgres_options options = g_options;

    printf("postgres_connection: failures\n");

    options.password = "certainly not the password";
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect(&options, &db, &reason),
                            D_POSTGRES_SQLSTATE_CANNOT_CONNECT);
    D_TESTS_POSTGRES_CHECK(db == NULL);
    D_TESTS_POSTGRES_CHECK( (reason) &&
                            (strlen(reason) > 0u) &&
                            (reason[strlen(reason) - 1u] != '\n') );
    d_db_free(reason);
    reason = NULL;

    options      = g_options;
    options.host = "127.0.0.1";
    options.port = 1u;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect(&options, &db, &reason),
                            D_POSTGRES_SQLSTATE_CANNOT_CONNECT);
    D_TESTS_POSTGRES_CHECK( (reason) &&
                            (strstr(reason, "1") != NULL) );
    d_db_free(reason);

    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect(&options, &db, NULL),
                            D_POSTGRES_SQLSTATE_CANNOT_CONNECT);
    D_TESTS_POSTGRES_CHECK(db == NULL);

    return;
}

#if !defined(_WIN32)

/*
d_tests_postgres_environment_wins
  libpq's environment beside the layer's defaults: PGOPTIONS still applies,
next to the layer's startup option, and PGCLIENTENCODING wins over the
layer's encoding. Both are set for one connection, and unset after it.
*/
static void
d_tests_postgres_environment_wins(
    void
)
{
    (void)setenv("PGOPTIONS", "-c work_mem=7MB", 1);
    (void)setenv("PGCLIENTENCODING", "LATIN1", 1);

    struct pg_conn* const db = d_tests_postgres_connect(NULL);

    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SHOW work_mem",
                                                    "7MB"));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_text_is(db, "SHOW client_min_messages", "warning"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SHOW client_encoding",
                                                    "LATIN1"));
    d_postgres_close(db);
    (void)unsetenv("PGOPTIONS");
    (void)unsetenv("PGCLIENTENCODING");

    return;
}

#else

/*
d_tests_postgres_environment_wins
  Nothing on Windows, where libpq's C runtime may keep an environment of its
own, which the harness's cannot reach.
*/
static void
d_tests_postgres_environment_wins(
    void
)
{
    return;
}

#endif  // !defined(_WIN32)

/*
d_tests_postgres_connect_defaults
  What a connection the layer makes is set up with.
  Tests the following:
  - UTF-8, NOTICE-level messages left unsent, and the connect timeout, unless
    the build or libpq's environment says otherwise
  - PGOPTIONS still applies, beside the layer's own startup option, and
    PGCLIENTENCODING wins over the layer's encoding (POSIX only, where the
    harness can set the environment)
*/
static void
d_tests_postgres_connect_defaults(
    void
)
{
    struct pg_conn* db = d_tests_postgres_connect(NULL);

    printf("postgres_connection: defaults\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SHOW client_encoding",
                                                    "UTF8"));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_text_is(db, "SHOW client_min_messages", "warning"));
    D_TESTS_POSTGRES_CHECK( (getenv("PGCONNECT_TIMEOUT")) ||
                            (d_tests_postgres_conninfo_is(db,
                                                          "connect_timeout",
                                                          "10")) );
    d_postgres_close(db);
    d_tests_postgres_environment_wins();

    return;
}

/*
d_tests_postgres_conninfo
  A keyword connection string for the run's settings, with more after it.
*/
static void
d_tests_postgres_conninfo(
    char*       _out,
    size_t      _size,
    const char* _more
)
{
    snprintf(_out,
             _size,
             "host='%s' port=%u user='%s' dbname='%s'%s%s%s %s",
             g_options.host,
             g_options.port,
             g_options.user,
             g_options.database,
             (g_options.password) ? " password='" : "",
             (g_options.password) ? g_options.password : "",
             (g_options.password) ? "'" : "",
             _more);

    return;
}

/*
d_tests_postgres_connect_strings
  d_postgres_connect_string.
  Tests the following:
  - keywords and a URI each connect
  - the string's own parameters win over the layer's defaults
  - a string libpq cannot read is D_DB_INVALID_ARGUMENT, with its reason
  - a NULL handle to fill is D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_connect_strings(
    void
)
{
    struct pg_conn* db        = NULL;
    char*           reason    = NULL;
    char            text[512] = "";

    printf("postgres_connection: connection strings\n");

    d_tests_postgres_conninfo(text, sizeof(text), "");
    D_TESTS_POSTGRES_OK(db, d_postgres_connect_string(text, &db, &reason));
    D_TESTS_POSTGRES_CHECK( (db != NULL) && (reason == NULL) );
    d_postgres_close(db);
    db = NULL;

    snprintf(text,
             sizeof(text),
             "postgresql://%s%s%s@%s:%u/%s",
             g_options.user,
             (g_options.password) ? ":" : "",
             (g_options.password) ? g_options.password : "",
             g_options.host,
             g_options.port,
             g_options.database);
    D_TESTS_POSTGRES_OK(db, d_postgres_connect_string(text, &db, &reason));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SELECT current_user",
                                                    g_options.user));
    d_postgres_close(db);
    db = NULL;

    d_tests_postgres_conninfo(text,
                              sizeof(text),
                              "client_encoding=LATIN1 "
                              "options='-c client_min_messages=notice'");
    D_TESTS_POSTGRES_OK(db, d_postgres_connect_string(text, &db, &reason));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SHOW client_encoding",
                                                    "LATIN1"));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_text_is(db, "SHOW client_min_messages", "notice"));
    d_postgres_close(db);
    db = NULL;

    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect_string("host=x port", &db,
                                                      &reason),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK( (db == NULL) && (reason != NULL) );
    d_db_free(reason);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect_string(text, NULL, NULL),
                            D_DB_INVALID_ARGUMENT);

    return;
}

/*
d_tests_postgres_connect_with
  d_postgres_connect_with.
  Tests the following:
  - keywords alone connect, any keyword libpq knows applies, and the
    defaults stand where none names them
  - the string's parts override the keywords, which still apply where the
    string names nothing
  - a dbname keyword names the database a string leaves out, and a single
    word is a database's name
  - a NULL or empty value is left out
  - a keyword libpq does not know, or does not spell as it does, is
    D_DB_INVALID_ARGUMENT, in libpq's words; a NULL keyword, NULL arrays
    for a count and a NULL handle to fill are D_DB_INVALID_ARGUMENT
  - a string libpq cannot read is D_DB_INVALID_ARGUMENT, with its reason,
    and with no keywords the arrays may be NULL
*/
static void
d_tests_postgres_connect_with(
    void
)
{
    struct pg_conn* db           = NULL;
    char*           reason       = NULL;
    char            port[16]     = "";
    char            text[512]    = "";
    const char*     keywords[7]  = { NULL };
    const char*     values[7]    = { NULL };
    const char*     name_word[1] = { "dbname" };
    const char*     name_value[1] = { NULL };

    printf("postgres_connection: keywords beside a string\n");

    snprintf(port, sizeof(port), "%u", g_options.port);
    keywords[0]   = "host";
    values[0]     = g_options.host;
    keywords[1]   = "port";
    values[1]     = port;
    keywords[2]   = "user";
    values[2]     = g_options.user;
    keywords[3]   = "password";
    values[3]     = g_options.password;
    keywords[4]   = "dbname";
    values[4]     = g_options.database;
    keywords[5]   = "application_name";
    values[5]     = "d_tests_keywords";
    keywords[6]   = "client_encoding";
    values[6]     = "LATIN1";
    name_value[0] = g_options.database;

    // keywords alone, the defaults where they name nothing
    D_TESTS_POSTGRES_OK(db, d_postgres_connect_with(NULL,
                                                    keywords,
                                                    values,
                                                    7u,
                                                    &db,
                                                    &reason));
    D_TESTS_POSTGRES_CHECK( (db != NULL) && (reason == NULL) );
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SHOW application_name",
                                                    "d_tests_keywords"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SHOW client_encoding",
                                                    "LATIN1"));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_text_is(db, "SHOW client_min_messages", "warning"));
    d_postgres_close(db);
    db = NULL;

    // the string overrides the keywords: their port is one nothing hears
    d_tests_postgres_conninfo(text,
                              sizeof(text),
                              "application_name=d_tests_string");
    values[1] = "1";
    D_TESTS_POSTGRES_OK(db, d_postgres_connect_with(text,
                                                    keywords,
                                                    values,
                                                    7u,
                                                    &db,
                                                    &reason));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SHOW application_name",
                                                    "d_tests_string"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SHOW client_encoding",
                                                    "LATIN1"));
    d_postgres_close(db);
    db        = NULL;
    values[1] = port;

    // a URI, likewise
    snprintf(text,
             sizeof(text),
             "postgresql://%s%s%s@%s:%u/%s?application_name=d_tests_uri",
             g_options.user,
             (g_options.password) ? ":" : "",
             (g_options.password) ? g_options.password : "",
             g_options.host,
             g_options.port,
             g_options.database);
    values[1] = "1";
    D_TESTS_POSTGRES_OK(db, d_postgres_connect_with(text,
                                                    keywords,
                                                    values,
                                                    7u,
                                                    &db,
                                                    &reason));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SHOW application_name",
                                                    "d_tests_uri"));
    d_postgres_close(db);
    db        = NULL;
    values[1] = port;

    // a string without a database, the keyword's
    snprintf(text,
             sizeof(text),
             "host='%s' port=%u user='%s'%s%s%s",
             g_options.host,
             g_options.port,
             g_options.user,
             (g_options.password) ? " password='" : "",
             (g_options.password) ? g_options.password : "",
             (g_options.password) ? "'" : "");
    D_TESTS_POSTGRES_OK(db, d_postgres_connect_with(text,
                                                    name_word,
                                                    name_value,
                                                    1u,
                                                    &db,
                                                    &reason));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               db,
                               "SELECT current_database()",
                               g_options.database));
    d_postgres_close(db);
    db = NULL;

    // a single word, a database's name; an empty and a NULL value left out
    values[5] = "";
    values[6] = NULL;
    D_TESTS_POSTGRES_OK(db, d_postgres_connect_with(g_options.database,
                                                    keywords,
                                                    values,
                                                    7u,
                                                    &db,
                                                    &reason));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               db,
                               "SELECT current_database()",
                               g_options.database));
    D_TESTS_POSTGRES_CHECK( (getenv("PGAPPNAME")) ||
                            (d_tests_postgres_text_is(db,
                                                      "SHOW application_name",
                                                      "")) );
    D_TESTS_POSTGRES_CHECK( (getenv("PGCLIENTENCODING")) ||
                            (d_tests_postgres_text_is(db,
                                                      "SHOW client_encoding",
                                                      "UTF8")) );
    d_postgres_close(db);
    db        = NULL;
    values[5] = "d_tests_keywords";
    values[6] = "LATIN1";

    // keywords libpq does not know, or that are not there
    keywords[6] = "no_such_option";
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect_with(NULL, keywords, values,
                                                    7u, &db, &reason),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK( (db == NULL)      &&
                            (reason != NULL)  &&
                            (strstr(reason, "no_such_option") != NULL) );
    d_db_free(reason);
    reason      = NULL;
    keywords[6] = "Host";
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect_with(NULL, keywords, values,
                                                    7u, &db, &reason),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK( (reason != NULL) &&
                            (strstr(reason, "\"Host\"") != NULL) );
    d_db_free(reason);
    reason      = NULL;
    keywords[6] = NULL;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect_with(NULL, keywords, values,
                                                    7u, &db, &reason),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK( (db == NULL) && (reason == NULL) );
    keywords[6] = "client_encoding";

    // arrays missing for a count, and no handle to fill
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect_with(NULL, NULL, values, 1u,
                                                    &db, &reason),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect_with(NULL, keywords, NULL, 1u,
                                                    &db, &reason),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK( (db == NULL) && (reason == NULL) );
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect_with(NULL, keywords, values,
                                                    7u, NULL, NULL),
                            D_DB_INVALID_ARGUMENT);

    // a string libpq cannot read; and one alone, the arrays NULL
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_connect_with("host=x port", keywords,
                                                    values, 7u, &db,
                                                    &reason),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK( (db == NULL) && (reason != NULL) );
    d_db_free(reason);
    reason = NULL;
    d_tests_postgres_conninfo(text, sizeof(text), "");
    D_TESTS_POSTGRES_OK(db, d_postgres_connect_with(text, NULL, NULL, 0u,
                                                    &db, &reason));
    D_TESTS_POSTGRES_CHECK( (db != NULL) && (reason == NULL) );
    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_liveness
  d_postgres_ping and d_postgres_backend_pid.
  Tests the following:
  - ping answers 0, inside a failed transaction too, which it leaves failed
  - the backend's process id is pg_backend_pid()'s
  - NULL is D_DB_INVALID_ARGUMENT, and process id 0
*/
static void
d_tests_postgres_liveness(
    void
)
{
    struct pg_conn* db = d_tests_postgres_connect(NULL);

    printf("postgres_connection: liveness\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_ping(db));
    D_TESTS_POSTGRES_CHECK(d_postgres_backend_pid(db) > 0);
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db, "SELECT pg_backend_pid()") ==
        (int64_t)d_postgres_backend_pid(db));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_begin_transaction(
                            db,
                            D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_CHECK(d_postgres_exec(db, "SELECT 1 / 0") != 0);
    D_TESTS_POSTGRES_OK(db, d_postgres_ping(db));
    D_TESTS_POSTGRES_CHECK(d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_ping(NULL), D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_postgres_backend_pid(NULL) == 0);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_lose
  Ends a connection's backend from another session, and waits until the
connection knows it.
*/
static void
d_tests_postgres_lose(
    struct pg_conn* _db
)
{
    struct pg_conn* const killer = d_tests_postgres_connect(NULL);

    // the backend ended, as an administrator would end it
    if (killer)
    {
        D_TESTS_POSTGRES_OK(killer,
                            d_postgres_execf(killer,
                                             "SELECT pg_terminate_backend(%d)",
                                             d_postgres_backend_pid(_db)));

        for (int i = 0; (i < 100) && (d_postgres_ping(_db) == 0); ++i)
        {
            (void)d_postgres_exec(killer, "SELECT pg_sleep(0.02)");
        }

        d_postgres_close(killer);
    }

    return;
}

/*
d_tests_postgres_lost_connections
  A connection whose server has gone.
  Tests the following:
  - ping, statements and transaction ends answer 08006, the error message
    saying why
  - the server's version and the backend's process id read as gone
*/
static void
d_tests_postgres_lost_connections(
    void
)
{
    struct pg_conn* db = d_tests_postgres_connect(NULL);

    printf("postgres_connection: lost connections\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    d_tests_postgres_lose(db);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_ping(db),
                            D_POSTGRES_SQLSTATE_CONNECTION_FAILURE);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db, "SELECT 1"),
                            D_POSTGRES_SQLSTATE_CONNECTION_FAILURE);
    D_TESTS_POSTGRES_CHECK(strlen(d_postgres_error_message(db)) > 0u);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_commit(db),
                            D_POSTGRES_SQLSTATE_CONNECTION_FAILURE);
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_transaction(db));
    D_TESTS_POSTGRES_CHECK(d_postgres_backend_pid(db) == 0);
    D_TESTS_POSTGRES_CHECK(d_postgres_server_version(db) == NULL);
    D_TESTS_POSTGRES_CHECK(d_postgres_server_version_number(db) == 0);
    D_TESTS_POSTGRES_CHECK(d_postgres_close(db) == 0);

    return;
}

//==============================================================================
// postgres_query.h
//==============================================================================

/*
d_tests_postgres_statements
  d_postgres_exec and d_postgres_execf.
  Tests the following:
  - a statement runs, and an empty one is a success
  - %I, %T, %Q (NULL as NULL), %d and %u reach SQL quoted and written
  - a % in the plain form's SQL is text
  - a statement the server refuses is its SQLSTATE
  - a format the layer cannot read, and a NaN, are D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_statements(
    void
)
{
    char                  schema[96] = "";
    struct pg_conn*       db         = d_tests_postgres_begin("statements",
                                                              schema,
                                                              sizeof(schema));
    volatile const double zero       = 0.0;

    printf("postgres_query: statements\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "CREATE TABLE s (a integer, b text)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_execf(db,
                                             "INSERT INTO %I VALUES (%d, %Q)",
                                             "s",
                                             1,
                                             "it's"));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_text_is(db, "SELECT b FROM s WHERE a = 1", "it's"));
    D_TESTS_POSTGRES_OK(db, d_postgres_execf(db,
                                             "INSERT INTO %T VALUES (%u, %Q)",
                                             schema,
                                             "s",
                                             2u,
                                             (const char*)NULL));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db, "SELECT count(*) FROM s WHERE b IS NULL") ==
        1);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "SELECT '100%'"));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, ""));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db, "SELEC 1"),
                            D_POSTGRES_SQLSTATE_SYNTAX_ERROR);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_execf(db, "SELECT %y", 1),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_execf(db, "SELECT %g", zero / zero),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_scripts
  Several statements in one call.
  Tests the following:
  - each runs
  - a failure stops the script and rolls back the statements before it,
    which ran as one transaction
  - a script's own COMMIT keeps what it committed, whatever follows
*/
static void
d_tests_postgres_scripts(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("scripts",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_query: scripts\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE a1 (x integer); "
                                            "CREATE TABLE a2 (x integer)"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL, "a1"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL, "a2"));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db,
                                            "INSERT INTO a1 VALUES (1); "
                                            "SELECT 1 / 0; "
                                            "INSERT INTO a1 VALUES (2)"),
                            d_postgres_result_from_sqlstate("22012"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db,
                                                "SELECT count(*) FROM a1") ==
                           0);
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_transaction(db));
    D_TESTS_POSTGRES_CHECK(d_postgres_exec(db,
                                           "BEGIN; INSERT INTO a1 VALUES (3); "
                                           "COMMIT; SELECT 1 / 0") != 0);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db,
                                                "SELECT count(*) FROM a1") ==
                           1);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_copies
  COPY with the client, which the layer does not carry.
  Tests the following:
  - COPY FROM STDIN and COPY TO STDOUT are D_DB_UNSUPPORTED, alone and
    inside a script, and the connection takes the next statement at once,
    outside any transaction
*/
static void
d_tests_postgres_copies(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("copies",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_query: COPY\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE c (a integer); "
                                            "INSERT INTO c VALUES (1), (2)"));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db, "COPY c FROM STDIN"),
                            D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_OK(db, d_postgres_ping(db));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT count(*) FROM c") ==
                           2);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db, "COPY c TO STDOUT"),
                            D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_OK(db, d_postgres_ping(db));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_exec(db,
                                            "SELECT 1; COPY c TO STDOUT; "
                                            "COPY c FROM STDIN; SELECT 2"),
                            D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_OK(db, d_postgres_ping(db));
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_transaction(db));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 7") == 7);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_changes_refused
  A failure writes no count; a NULL connection, count or SQL is
D_DB_INVALID_ARGUMENT.
*/
static void
d_tests_postgres_changes_refused(
    struct pg_conn* _db
)
{
    int64_t n = 99;

    D_TESTS_POSTGRES_RESULT(_db,
                            d_postgres_exec_changes(_db,
                                                    &n,
                                                    "SELECT * FROM no"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_CHECK(n == 99);
    D_TESTS_POSTGRES_RESULT(_db,
                            d_postgres_exec_changes(NULL, &n, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(_db,
                            d_postgres_exec_changes(_db, NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(_db,
                            d_postgres_execf_changes(_db, &n, NULL),
                            D_DB_INVALID_ARGUMENT);

    return;
}

/*
d_tests_postgres_changes
  d_postgres_exec_changes and d_postgres_execf_changes.
  Tests the following:
  - INSERT, UPDATE, DELETE and SELECT count their rows; CREATE counts none
  - a failure writes nothing
  - a NULL connection, count or SQL is D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_changes(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("changes",
                                                        schema,
                                                        sizeof(schema));
    int64_t         n          = -1;

    printf("postgres_query: rows changed\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE g (a integer)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec_changes(
                                db,
                                &n,
                                "INSERT INTO g VALUES (1), (2), (3)"));
    D_TESTS_POSTGRES_CHECK(n == 3);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec_changes(
                                db,
                                &n,
                                "UPDATE g SET a = a + 1 WHERE a > 1"));
    D_TESTS_POSTGRES_CHECK(n == 2);
    D_TESTS_POSTGRES_OK(db, d_postgres_execf_changes(db,
                                                     &n,
                                                     "DELETE FROM g "
                                                     "WHERE a = %d",
                                                     4));
    D_TESTS_POSTGRES_CHECK(n == 1);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec_changes(db, &n, "SELECT * FROM g"));
    D_TESTS_POSTGRES_CHECK(n == 2);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec_changes(db,
                                                    &n,
                                                    "CREATE TABLE g2 (a int)"));
    D_TESTS_POSTGRES_CHECK(n == 0);
    d_tests_postgres_changes_refused(db);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_integers
  d_postgres_query_int64 and its f form.
  Tests the following:
  - integers at both ends of int64_t, read exactly
  - a fraction or an exponent truncated toward zero; NULL as 0
  - no row is D_DB_NOT_FOUND, a word or a number past int64_t
    D_DB_INVALID_ARGUMENT, each writing nothing
*/
static void
d_tests_postgres_integers(
    void
)
{
    struct pg_conn* db = d_tests_postgres_connect(NULL);
    int64_t         v  = 0;

    printf("postgres_query: integers\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 42") == 42);
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db, "SELECT 9223372036854775807") == INT64_MAX);
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db, "SELECT (-9223372036854775807 - 1)::bigint")
        == INT64_MIN);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 1.9") == 1);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT -1.9") == -1);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 1e3::float8") ==
                           1000);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT NULL") == 0);
    D_TESTS_POSTGRES_OK(db, d_postgres_query_int64f(db,
                                                    &v,
                                                    "SELECT %d + %lld",
                                                    2,
                                                    40ll));
    D_TESTS_POSTGRES_CHECK(v == 42);
    v = 5;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_int64(db, &v,
                                                   "SELECT 1 WHERE false"),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_int64(db, &v, "SELECT 'abc'"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_int64(db, &v, "SELECT 1e19"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(v == 5);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_int64(db, &v, "SELECT nope"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_COLUMN);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_doubles
  d_postgres_query_double and its f form.
  Tests the following:
  - numbers read to the nearest double, NULL as 0
  - NaN, which the server writes as a word, is D_DB_INVALID_ARGUMENT
  - %g writes a double that reads back the same
  - both directions keep `.` for the point under a decimal-comma locale
*/
static void
d_tests_postgres_doubles(
    void
)
{
    struct pg_conn*   db       = d_tests_postgres_connect(NULL);
    double            v        = 0.0;
    const char* const locales[] = { "C", "de_DE.UTF-8", "ps_AF.UTF-8" };

    printf("postgres_query: doubles\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    for (size_t i = 0u; i < sizeof(locales) / sizeof(locales[0]); ++i)
    {
        // a locale this machine lacks is passed over
        if (!setlocale(LC_NUMERIC, locales[i]))
        {
            continue;
        }

        D_TESTS_POSTGRES_OK(db, d_postgres_query_double(db, &v,
                                                        "SELECT -2.25"));
        D_TESTS_POSTGRES_CHECK(v == -2.25);
        D_TESTS_POSTGRES_OK(db, d_postgres_query_doublef(db,
                                                         &v,
                                                         "SELECT %g::float8",
                                                         0.1));
        D_TESTS_POSTGRES_CHECK(v == 0.1);
        D_TESTS_POSTGRES_OK(db, d_postgres_query_doublef(db,
                                                         &v,
                                                         "SELECT %f + 0.5",
                                                         1.0));
        D_TESTS_POSTGRES_CHECK(v == 1.5);
    }

    (void)setlocale(LC_NUMERIC, "C");
    D_TESTS_POSTGRES_OK(db, d_postgres_query_double(db, &v, "SELECT NULL"));
    D_TESTS_POSTGRES_CHECK(v == 0.0);
    D_TESTS_POSTGRES_OK(db, d_postgres_query_double(db, &v, "SELECT 1e300"));
    D_TESTS_POSTGRES_CHECK(v == 1e300);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_double(db, &v,
                                                    "SELECT 'NaN'::float8"),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_text
  d_postgres_query_text and its f form.
  Tests the following:
  - text read whole, empty text as "", NULL as NULL
  - a value with quotes, backslashes and characters past ASCII reaches the
    server through %Q and comes back the same
  - no row is D_DB_NOT_FOUND, writing nothing
*/
static void
d_tests_postgres_text(
    void
)
{
    struct pg_conn*   db           = d_tests_postgres_connect(NULL);
    char*             text         = NULL;
    char              untouched[]  = "untouched";
    const char* const weird        =
        "it's a \\ \"test\" \xc3\xa9\xe2\x82\xac";

    printf("postgres_query: text\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db, "SELECT 'abc'",
                                                    "abc"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db, "SELECT ''", ""));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db, "SELECT NULL", NULL));
    D_TESTS_POSTGRES_OK(db, d_postgres_query_textf(db,
                                                   &text,
                                                   "SELECT %Q::text",
                                                   weird));
    D_TESTS_POSTGRES_CHECK( (text) && (strcmp(text, weird) == 0) );
    d_db_free(text);
    text = untouched;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_text(db, &text,
                                                  "SELECT 'x' WHERE false"),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(strcmp(text, "untouched") == 0);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_bools
  d_postgres_query_bool and its f form.
  Tests the following:
  - true and false as the server writes them, NULL as false
  - a value that is not a boolean is D_DB_INVALID_ARGUMENT, writing nothing
*/
static void
d_tests_postgres_bools(
    void
)
{
    struct pg_conn* db = d_tests_postgres_connect(NULL);
    bool            b  = false;

    printf("postgres_query: booleans\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_query_bool(db, &b, "SELECT true"));
    D_TESTS_POSTGRES_CHECK(b);
    D_TESTS_POSTGRES_OK(db, d_postgres_query_boolf(db,
                                                   &b,
                                                   "SELECT %Q = 'y'",
                                                   "n"));
    D_TESTS_POSTGRES_CHECK(!b);
    b = true;
    D_TESTS_POSTGRES_OK(db, d_postgres_query_bool(db, &b,
                                                  "SELECT NULL::boolean"));
    D_TESTS_POSTGRES_CHECK(!b);
    b = true;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_bool(db, &b, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(b);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_rows_values
  d_postgres_query_rows and its f form: the values.
  Tests the following:
  - boolean as INTEGER 1 and 0; integers to both ends of int64_t; doubles,
    NaN and the infinities included, to the last bit on every server (the
    layer's extra_float_digits); a real; numeric as TEXT, NaN
    too; text; bytea with NULs as a BLOB, from hex output and from escape
    output; NULL
  - empty text and empty bytes point at a terminator
  - columns named as the server names them: an alias, a column, ?column?
  - no rows: the columns alone; no columns: empty rows; RETURNING rows
  - two statements are the server's syntax error, before either runs; a
    failure leaves the rows alone
  - values read the same under a locale whose point is a comma
*/
static void
d_tests_postgres_rows_values(
    void
)
{
    char             schema[96] = "";
    struct pg_conn*  db         = d_tests_postgres_begin("rows",
                                                         schema,
                                                         sizeof(schema));
    struct d_db_rows rows       = { 0u, 0u, NULL, NULL };
    struct d_db_rows marker     = { 7u, 7u, NULL, NULL };

    printf("postgres_query: rows\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "CREATE TABLE v (id integer PRIMARY "
                                        "KEY, flag boolean, big bigint, "
                                        "d double precision, r real, "
                                        "n numeric(10,2), t text, b bytea)"));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "INSERT INTO v VALUES "
                                        "(1, true, -9223372036854775808, "
                                        "0.1::float8 + 0.2::float8, 0.25, "
                                        "12.34, 'it''s', '\\x00ff00'), "
                                        "(2, false, 9223372036854775807, "
                                        "'-Infinity', NULL, NULL, '', '\\x')"));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows(db,
                                              &rows,
                                              "SELECT * FROM v ORDER BY id"));
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 8u) &&
                            (rows.row_count == 2u) );

    // the values of each row
    if ( (rows.column_count == 8u) &&
         (rows.row_count == 2u) )
    {
        const struct d_db_value* const one = &rows.values[0];
        const struct d_db_value* const two = &rows.values[8];

        D_TESTS_POSTGRES_CHECK( (one[1].type == D_DB_VALUE_INTEGER) &&
                                (one[1].as.integer == 1)            &&
                                (two[1].as.integer == 0) );
        D_TESTS_POSTGRES_CHECK( (one[2].type == D_DB_VALUE_INTEGER) &&
                                (one[2].as.integer == INT64_MIN)    &&
                                (two[2].as.integer == INT64_MAX) );
        D_TESTS_POSTGRES_CHECK( (one[3].type == D_DB_VALUE_REAL) &&
                                (one[3].as.real == 0.30000000000000004) );
        D_TESTS_POSTGRES_CHECK( (two[3].type == D_DB_VALUE_REAL) &&
                                (isinf(two[3].as.real))          &&
                                (two[3].as.real < 0.0) );
        D_TESTS_POSTGRES_CHECK( (one[4].type == D_DB_VALUE_REAL) &&
                                (one[4].as.real == 0.25) );
        D_TESTS_POSTGRES_CHECK( (one[5].type == D_DB_VALUE_TEXT) &&
                                (strcmp(one[5].as.text, "12.34") == 0) );
        D_TESTS_POSTGRES_CHECK( (one[6].type == D_DB_VALUE_TEXT) &&
                                (strcmp(one[6].as.text, "it's") == 0) );
        D_TESTS_POSTGRES_CHECK(
            (one[7].type == D_DB_VALUE_BLOB) &&
            (one[7].size == 3u)              &&
            (memcmp(one[7].as.blob, "\0\xff\0", 3u) == 0));
        D_TESTS_POSTGRES_CHECK( (two[4].type == D_DB_VALUE_NULL) &&
                                (two[5].type == D_DB_VALUE_NULL) );
        D_TESTS_POSTGRES_CHECK( (two[6].type == D_DB_VALUE_TEXT) &&
                                (two[6].size == 0u)              &&
                                (two[6].as.text[0] == '\0') );
        D_TESTS_POSTGRES_CHECK( (two[7].type == D_DB_VALUE_BLOB) &&
                                (two[7].size == 0u)              &&
                                (two[7].as.blob) );
    }

    d_db_rows_free(&rows);

    // the words PostgreSQL writes for numbers, and bytes in escape output
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows(db,
                                              &rows,
                                              "SELECT 'NaN'::float8, "
                                              "'Infinity'::float8, "
                                              "'NaN'::numeric"));
    D_TESTS_POSTGRES_CHECK( (rows.row_count == 1u)                      &&
                            (rows.values[0].type == D_DB_VALUE_REAL)    &&
                            (isnan(rows.values[0].as.real))             &&
                            (isinf(rows.values[1].as.real))             &&
                            (rows.values[2].type == D_DB_VALUE_TEXT)    &&
                            (strcmp(rows.values[2].as.text, "NaN") == 0) );
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "SET bytea_output = escape"));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows(db, &rows,
                                              "SELECT b FROM v WHERE id = 1"));
    D_TESTS_POSTGRES_CHECK(
        (rows.row_count == 1u)                    &&
        (rows.values[0].type == D_DB_VALUE_BLOB)  &&
        (rows.values[0].size == 3u)               &&
        (memcmp(rows.values[0].as.blob, "\0\xff\0", 3u) == 0));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "RESET bytea_output"));

    // names
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows(db,
                                              &rows,
                                              "SELECT id AS ident, t, 1 + 1 "
                                              "FROM v WHERE id = 1"));
    D_TESTS_POSTGRES_CHECK(
        (rows.column_count == 3u)                       &&
        (strcmp(rows.columns[0].name, "ident") == 0)    &&
        (strcmp(rows.columns[1].name, "t") == 0)        &&
        (strcmp(rows.columns[2].name, "?column?") == 0) &&
        (rows.values[2].as.integer == 2));
    d_db_rows_free(&rows);

    // no rows, no columns, and RETURNING
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows(db,
                                              &rows,
                                              "SELECT id, t FROM v "
                                              "WHERE false"));
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 2u) &&
                            (rows.row_count == 0u)    &&
                            (rows.values == NULL) );
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows(db,
                                              &rows,
                                              "UPDATE v SET t = 'x' "
                                              "WHERE id = 2"));
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 0u) &&
                            (rows.row_count == 0u)    &&
                            (rows.columns == NULL) );
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_text_is(db, "SELECT t FROM v WHERE id = 2", "x"));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows(db,
                                              &rows,
                                              "UPDATE v SET t = t "
                                              "RETURNING id, t"));
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 2u) &&
                            (rows.row_count == 2u) );
    d_db_rows_free(&rows);

    // failures
    rows = marker;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rows(db,
                                                  &rows,
                                                  "INSERT INTO v (id) "
                                                  "VALUES (9); SELECT 1"),
                            D_POSTGRES_SQLSTATE_SYNTAX_ERROR);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT count(*) FROM v")
                           == 2);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rows(db, &rows,
                                                  "SELECT nope FROM v"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_COLUMN);
    D_TESTS_POSTGRES_CHECK(strstr(d_postgres_error_message(db), "nope"));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rowsf(db, &rows, "SELECT %I",
                                                   (const char*)NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 7u) &&
                            (rows.row_count == 7u) );

    // formatted, and under a comma for the point where one is installed
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rowsf(db,
                                               &rows,
                                               "SELECT t FROM %T "
                                               "WHERE t = %Q",
                                               schema,
                                               "v",
                                               "it's"));
    D_TESTS_POSTGRES_CHECK(rows.row_count == 1u);
    d_db_rows_free(&rows);

    if ( (setlocale(LC_NUMERIC, "de_DE.UTF-8")) ||
         (setlocale(LC_NUMERIC, "ps_AF.UTF-8")) )
    {
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_query_rows(db, &rows,
                                                  "SELECT r, -2.5::float8 "
                                                  "FROM v WHERE id = 1"));
        D_TESTS_POSTGRES_CHECK( (rows.row_count == 1u)           &&
                                (rows.values[0].as.real == 0.25) &&
                                (rows.values[1].as.real == -2.5) );
        d_db_rows_free(&rows);
        (void)setlocale(LC_NUMERIC, "C");
    }

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_rows_types
  d_postgres_query_rows: the types of the columns.
  Tests the following:
  - every type postgres_query.h tabulates, with its name as format_type
    writes it
  - a type it does not tabulate is NONE, named ""; void is named
*/
static void
d_tests_postgres_rows_types(
    void
)
{
    static const struct
    {
        const char*    declared;  // the type, as a cast names it
        enum d_db_type type;      // what it reads as
        const char*    name;      // and its name
    } expected[] =
    {
        { "boolean",          D_DB_TYPE_BOOLEAN,        "boolean" },
        { "smallint",         D_DB_TYPE_INTEGER,        "smallint" },
        { "integer",          D_DB_TYPE_INTEGER,        "integer" },
        { "bigint",           D_DB_TYPE_BIG_INTEGER,    "bigint" },
        { "oid",              D_DB_TYPE_BIG_INTEGER,    "oid" },
        { "real",             D_DB_TYPE_FLOATING_POINT, "real" },
        { "double precision", D_DB_TYPE_FLOATING_POINT, "double precision" },
        { "numeric(8,3)",     D_DB_TYPE_DECIMAL,        "numeric" },
        { "text",             D_DB_TYPE_STRING,         "text" },
        { "varchar(9)",       D_DB_TYPE_STRING,         "character varying" },
        { "char(3)",          D_DB_TYPE_STRING,         "character" },
        { "name",             D_DB_TYPE_STRING,         "name" },
        { "\"char\"",         D_DB_TYPE_STRING,         "\"char\"" },
        { "bytea",            D_DB_TYPE_BINARY,         "bytea" },
        { "date",             D_DB_TYPE_DATE,           "date" },
        { "time",             D_DB_TYPE_TIME,
          "time without time zone" },
        { "timetz",           D_DB_TYPE_TIME,           "time with time zone" },
        { "timestamp",        D_DB_TYPE_DATETIME,
          "timestamp without time zone" },
        { "timestamptz",      D_DB_TYPE_TIMESTAMP,
          "timestamp with time zone" },
        { "json",             D_DB_TYPE_JSON,           "json" },
        { "jsonb",            D_DB_TYPE_JSON,           "jsonb" },
        { "xml",              D_DB_TYPE_XML,            "xml" },
        { "uuid",             D_DB_TYPE_UUID,           "uuid" },
        { "interval",         D_DB_TYPE_NONE,           "interval" },
        { "inet",             D_DB_TYPE_NONE,           "inet" },
        { "cidr",             D_DB_TYPE_NONE,           "cidr" },
        { "macaddr",          D_DB_TYPE_NONE,           "macaddr" },
        { "bit(3)",           D_DB_TYPE_NONE,           "bit" },
        { "varbit(5)",        D_DB_TYPE_NONE,           "bit varying" },
        { "money",            D_DB_TYPE_NONE,           "money" },
        { "point",            D_DB_TYPE_NONE,           "" },
        { "int4range",        D_DB_TYPE_NONE,           "" }
    };
    const size_t     count = sizeof(expected) / sizeof(expected[0]);
    struct pg_conn*  db    = d_tests_postgres_connect(NULL);
    struct d_db_rows rows  = { 0u, 0u, NULL, NULL };

    printf("postgres_query: column types\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    for (size_t i = 0u; i < count; ++i)
    {
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_query_rowsf(db,
                                                   &rows,
                                                   "SELECT NULL::%s AS c",
                                                   expected[i].declared));

        const bool typed = ( (rows.column_count == 1u)                  &&
                             (rows.row_count == 1u)                     &&
                             (rows.columns[0].type == expected[i].type) &&
                             (strcmp(rows.columns[0].type_name,
                                     expected[i].name) == 0)            &&
                             (rows.values[0].type == D_DB_VALUE_NULL) );

        // say which type was misread, beyond the line
        if (!typed)
        {
            printf("    %s read as %s (%s)\n",
                   expected[i].declared,
                   (rows.column_count == 1u)
                       ? d_db_type_name(rows.columns[0].type)
                       : "nothing",
                   (rows.column_count == 1u) ? rows.columns[0].type_name
                                             : "");
        }

        D_TESTS_POSTGRES_CHECK(typed);
        d_db_rows_free(&rows);
    }

    // void: a function that returns nothing returns ""
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows(db, &rows,
                                              "SELECT pg_sleep(0)"));
    D_TESTS_POSTGRES_CHECK( (rows.row_count == 1u)                         &&
                            (rows.columns[0].type == D_DB_TYPE_NONE)       &&
                            (strcmp(rows.columns[0].type_name, "void") == 0) &&
                            (rows.values[0].type == D_DB_VALUE_TEXT)       &&
                            (rows.values[0].size == 0u) );
    d_db_rows_free(&rows);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_rows_params
  d_postgres_query_rows_params.
  Tests the following:
  - values reach their parameters as text, typed by them, NULL as NULL, a
    bytea as its text; text that would be SQL stays a value
  - no values at all, with NULL values
  - a count that does not match is 08P01, a value a type cannot read 22P02,
    two statements 42601
  - NULL arguments, NULL values with a count, and a count past 65535 are
    D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_rows_params(
    void
)
{
    struct pg_conn*    db       = d_tests_postgres_connect(NULL);
    struct d_db_rows   rows     = { 0u, 0u, NULL, NULL };
    const char* const  values[] = { "41", "it's; DROP TABLE x", NULL,
                                    "\\x0102" };
    const char* const  word[]   = { "x" };
    int64_t            number   = 0;
    bool               flag     = false;

    printf("postgres_query: rows with parameters\n");

    memset(&rows, 0, sizeof(rows));

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows_params(
                            db,
                            &rows,
                            "SELECT $1::int + 1 AS n, $2::text AS t, "
                            "$3::text IS NULL AS z, $4::bytea AS b",
                            values,
                            4u));
    D_TESTS_POSTGRES_CHECK( (rows.row_count == 1u) &&
                            (rows.column_count == 4u) );
    D_TESTS_POSTGRES_CHECK(
        (d_db_value_int64(d_db_rows_value(&rows, 0u, 0u), &number) == 0) &&
        (number == 42));
    D_TESTS_POSTGRES_CHECK(
        (d_db_rows_value(&rows, 0u, 1u)) &&
        (strcmp(d_db_rows_value(&rows, 0u, 1u)->as.text,
                "it's; DROP TABLE x") == 0));
    D_TESTS_POSTGRES_CHECK(
        (d_db_value_bool(d_db_rows_value(&rows, 0u, 2u), &flag) == 0) &&
        (flag));
    D_TESTS_POSTGRES_CHECK(
        (d_db_rows_value(&rows, 0u, 3u)) &&
        (d_db_rows_value(&rows, 0u, 3u)->type == D_DB_VALUE_BLOB) &&
        (d_db_rows_value(&rows, 0u, 3u)->size == 2u) &&
        (memcmp(d_db_rows_value(&rows, 0u, 3u)->as.blob, "\x01\x02", 2u) ==
         0));
    d_db_rows_free(&rows);

    // no values
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows_params(db, &rows, "SELECT 7",
                                                     NULL, 0u));
    D_TESTS_POSTGRES_CHECK(
        (d_db_value_int64(d_db_rows_value(&rows, 0u, 0u), &number) == 0) &&
        (number == 7));
    d_db_rows_free(&rows);

    // what the server refuses
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rows_params(
                                db, &rows, "SELECT $1::int + $2::int",
                                word, 1u),
                            D_POSTGRES_SQLSTATE_PROTOCOL_VIOLATION);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rows_params(
                                db, &rows, "SELECT $1::int", word, 1u),
                            D_POSTGRES_SQLSTATE_INVALID_TEXT_REPRESENTATION);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rows_params(
                                db, &rows, "SELECT 1; SELECT 2", NULL, 0u),
                            D_POSTGRES_SQLSTATE_SYNTAX_ERROR);

    // what cannot be right
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rows_params(
                                db, &rows, "SELECT $1", NULL, 1u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rows_params(
                                db, &rows, "SELECT $1", values, 65536u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rows_params(
                                db, NULL, "SELECT 1", NULL, 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rows_params(
                                db, &rows, NULL, NULL, 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL,
                            d_postgres_query_rows_params(
                                NULL, &rows, "SELECT 1", NULL, 0u),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_quoting_names
  d_postgres_quote_identifier.
  Tests the following:
  - a name in double quotes, any double quote in it doubled, "" for ""
  - NULL for NULL
  - a quoted name with every awkward character names a table the server
    keeps under exactly that name
*/
static void
d_tests_postgres_quoting_names(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("qnames",
                                                        schema,
                                                        sizeof(schema));
    char*           quoted     = d_postgres_quote_identifier("a\"b");

    printf("postgres_query: quoting names\n");

    D_TESTS_POSTGRES_CHECK( (quoted) && (strcmp(quoted, "\"a\"\"b\"") == 0) );
    d_db_free(quoted);
    quoted = d_postgres_quote_identifier("");
    D_TESTS_POSTGRES_CHECK( (quoted) && (strcmp(quoted, "\"\"") == 0) );
    d_db_free(quoted);
    D_TESTS_POSTGRES_CHECK(d_postgres_quote_identifier(NULL) == NULL);

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_execf(db,
                                             "CREATE TABLE %I (a integer)",
                                             "We\"ird 'Name; DROP"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL,
                                                "We\"ird 'Name; DROP"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "we\"ird 'name"));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_quoted_backslash
  A quoted backslash reads back as one, with standard_conforming_strings off
and then on; from 19 the server refuses off, and only on is tried.
*/
static void
d_tests_postgres_quoted_backslash(
    struct pg_conn* _db
)
{
    char sql[64] = "";

    for (int scs = 0; scs < 2; ++scs)
    {
        const int set = d_postgres_set_parameter(_db,
                                                 "standard_conforming_strings",
                                                 (scs) ? "on" : "off");

        // from 19 the server has only standard strings, and refuses off
        if ( (!scs) &&
             (set == D_POSTGRES_SQLSTATE_FEATURE_NOT_SUPPORTED) )
        {
            continue;
        }

        D_TESTS_POSTGRES_OK(_db, set);

        char* const quoted = d_postgres_quote_literal(_db, "a\\b");

        snprintf(sql, sizeof(sql), "SELECT %s", (quoted) ? quoted : "NULL");
        D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(_db, sql, "a\\b"));
        d_db_free(quoted);
    }

    return;
}

/*
d_tests_postgres_quoting_values
  d_postgres_quote_literal.
  Tests the following:
  - a value in single quotes, quotes doubled, NULL as the keyword
  - a backslash, written so that the server reads it back as one whatever
    standard_conforming_strings says, on servers before 19 that still let
    it be turned off
  - NULL for a NULL connection; for text not valid in the connection's
    encoding, NULL from libpq's releases of February 2025 on, and before
    them a value the server refuses, 22021
*/
static void
d_tests_postgres_quoting_values(
    void
)
{
    struct pg_conn* db      = d_tests_postgres_connect(NULL);
    char*           quoted  = NULL;
    char            sql[64] = "";

    printf("postgres_query: quoting values\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    quoted = d_postgres_quote_literal(db, "it's");
    D_TESTS_POSTGRES_CHECK( (quoted) && (strcmp(quoted, "'it''s'") == 0) );
    d_db_free(quoted);
    quoted = d_postgres_quote_literal(db, NULL);
    D_TESTS_POSTGRES_CHECK( (quoted) && (strcmp(quoted, "NULL") == 0) );
    d_db_free(quoted);

    d_tests_postgres_quoted_backslash(db);
    D_TESTS_POSTGRES_CHECK(d_postgres_quote_literal(NULL, "x") == NULL);
    quoted = d_postgres_quote_literal(db, "\xff\xfe");

    // refused by libpq since its February 2025 releases, by the server before
    if (quoted)
    {
        snprintf(sql, sizeof(sql), "SELECT %s", quoted);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_exec(db, sql),
                                d_postgres_result_from_sqlstate("22021"));
        d_db_free(quoted);
    }

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_query_arguments
  The query functions' argument checks.
  Tests the following:
  - a NULL connection, value, count, rows or SQL is D_DB_INVALID_ARGUMENT,
    for every function and form
*/
static void
d_tests_postgres_query_arguments(
    void
)
{
    struct pg_conn*  db   = d_tests_postgres_connect(NULL);
    int64_t          i    = 0;
    double           d    = 0.0;
    char*            text = NULL;
    bool             b    = false;
    struct d_db_rows rows = { 0u, 0u, NULL, NULL };

    printf("postgres_query: arguments\n");

    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execf(NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execf(db, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_int64(db, NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_int64(db, &i, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_int64f(NULL, &i, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_double(db, NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_doublef(db, &d, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_text(NULL, &text,
                                                      "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_textf(db, NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_bool(db, &b, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_boolf(db, NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(text == NULL);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execf_changes(NULL, &i,
                                                         "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execf_changes(db, NULL,
                                                         "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec_changes(db, &i, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_rows(NULL, &rows,
                                                      "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_rows(db, NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_rows(db, &rows, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_rowsf(NULL, &rows,
                                                       "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_rowsf(db, NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_rowsf(db, &rows, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK( (rows.columns == NULL) &&
                            (rows.values == NULL) );

    d_postgres_close(db);

    return;
}

//==============================================================================
// postgres_statement.h
//==============================================================================

/*
d_tests_postgres_preparing
  Preparing, describing and finalizing statements.
  Tests the following:
  - parameters counted by the highest the statement uses; the statement
    knows its connection, and the server holds it while it lives
  - a statement the server will not prepare is its SQLSTATE, with its
    message, and leaves NULL; two statements are the server's syntax error
  - preparef formats with the connection's quoting
  - finalize accepts NULL, and drops the server's copy
  - a name the server still holds, left by a statement whose copy could not
    be dropped, does not stop a prepare, nor fail the transaction one runs
    in: a statement finalized in a failed transaction, whose record's
    address the next prepare reuses
*/
static void
d_tests_postgres_preparing(
    void
)
{
    char                         schema[96] = "";
    struct pg_conn*              db         = d_tests_postgres_begin("prepare",
                                                                     schema,
                                                                     sizeof(
                                                                       schema));
    struct d_postgres_statement* statement  = NULL;
    char*                        name       = NULL;

    printf("postgres_statement: preparing\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE p (a integer, "
                                            "b text)"));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db,
                                           &statement,
                                           "INSERT INTO p VALUES ($1, $2)"));
    D_TESTS_POSTGRES_CHECK(d_postgres_parameter_count(statement) == 2u);
    D_TESTS_POSTGRES_CHECK(d_postgres_statement_connection(statement) == db);
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db,
                             "SELECT count(*) FROM pg_prepared_statements")
        == 1);
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_text(db,
                                              &name,
                                              "SELECT name FROM "
                                              "pg_prepared_statements"));
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(statement));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db,
                             "SELECT count(*) FROM pg_prepared_statements")
        == 0);
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &statement,
                                           "SELECT $2::integer, $1::text"));
    D_TESTS_POSTGRES_CHECK(d_postgres_parameter_count(statement) == 2u);
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(statement));
    D_TESTS_POSTGRES_OK(db, d_postgres_prepare(db, &statement, "SELECT 1"));
    D_TESTS_POSTGRES_CHECK(d_postgres_parameter_count(statement) == 0u);
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(statement));

    // refusals
    statement = (struct d_postgres_statement*)&g_options;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_prepare(db, &statement, "SELEC bad"),
                            D_POSTGRES_SQLSTATE_SYNTAX_ERROR);
    D_TESTS_POSTGRES_CHECK(statement == NULL);
    D_TESTS_POSTGRES_CHECK(strstr(d_postgres_error_message(db), "SELEC"));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_prepare(db, &statement,
                                               "SELECT * FROM none"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_prepare(db, &statement,
                                               "SELECT 1; SELECT 2"),
                            D_POSTGRES_SQLSTATE_SYNTAX_ERROR);
    D_TESTS_POSTGRES_CHECK(statement == NULL);
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db,
                             "SELECT count(*) FROM pg_prepared_statements")
        == 0);

    // formatted
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_preparef(db,
                                            &statement,
                                            "SELECT %I FROM %T WHERE b = %Q "
                                            "AND a = $1",
                                            "a",
                                            schema,
                                            "p",
                                            "it's"));
    D_TESTS_POSTGRES_CHECK(d_postgres_parameter_count(statement) == 1u);
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(statement));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_preparef(db, &statement, "SELECT %I",
                                                (const char*)NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(statement == NULL);
    D_TESTS_POSTGRES_OK(NULL, d_postgres_finalize(NULL));

    // a stale copy under the name a statement's record had: where the next
    // record lands at the same address, the name is taken, and dropped
    if (name)
    {
        D_TESTS_POSTGRES_OK(db, d_postgres_execf(db,
                                                 "PREPARE %I AS SELECT 1",
                                                 name));
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_prepare(db, &statement,
                                               "SELECT 2 + $1::integer"));
        D_TESTS_POSTGRES_OK(db, d_postgres_finalize(statement));
        D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "DEALLOCATE ALL"));
    }

    // a copy a failed transaction kept, before libpq 17, under the name the
    // next prepare, inside a transaction, may take: looked for and dropped
    // first, and the transaction goes on
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_begin_transaction(
                            db,
                            D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_prepare(db, &statement, "SELECT 4"));
    D_TESTS_POSTGRES_CHECK(d_postgres_exec(db, "SELECT 1 / 0") != 0);
    (void)d_postgres_finalize(statement);
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_begin_transaction(
                            db,
                            D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_prepare(db, &statement, "SELECT 5"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(statement));
    D_TESTS_POSTGRES_OK(db, d_postgres_commit(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "DEALLOCATE ALL"));

    d_db_free(name);
    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_binding
  Giving parameters values.
  Tests the following:
  - each bind stores its value: NULL, booleans to boolean parameters and to
    integer ones, the extreme integers, doubles exactly with NaN and the
    infinities, text, bytes with NULs
  - text and bytes are copied: changing the caller's buffer after the bind
    changes nothing
  - NULL text and NULL bytes bind NULL; a pointer to no bytes binds the
    empty bytea
  - a parameter numbered 0 or past the last is 42P02, which has words of its
    own; clearing makes every parameter NULL again
  - the server infers each parameter's type from the statement
*/
static void
d_tests_postgres_binding(
    void
)
{
    char                         schema[96] = "";
    struct pg_conn*              db         = d_tests_postgres_begin("binding",
                                                                     schema,
                                                                     sizeof(
                                                                       schema));
    struct d_postgres_statement* statement  = NULL;
    struct d_db_rows             rows       = { 0u, 0u, NULL, NULL };
    char                         text[]     = "it's";
    unsigned char                bytes[]    = { 0x00, 0x01, 0x00 };
    int64_t                      changes    = 0;

    printf("postgres_statement: binding\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "CREATE TABLE v (k serial PRIMARY "
                                        "KEY, n integer, t boolean, "
                                        "f boolean, lo bigint, hi bigint, "
                                        "r double precision, s text, "
                                        "b bytea)"));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db,
                                           &statement,
                                           "INSERT INTO v (n, t, f, lo, hi, "
                                           "r, s, b) VALUES "
                                           "($1, $2, $3, $4, $5, $6, $7, $8)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_null(statement, 1u));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_bool(statement, 2u, true));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_bool(statement, 3u, false));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(statement, 4u, INT64_MIN));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(statement, 5u, INT64_MAX));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_double(statement, 6u,
                                                   0.30000000000000004));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_text(statement, 7u, text));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_bind_blob(statement, 8u, bytes,
                                             sizeof(bytes)));

    // the copies outlive the caller's buffers
    text[0]  = 'X';
    bytes[1] = 0xffu;
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(statement, &changes));
    D_TESTS_POSTGRES_CHECK(changes == 1);

    // the words for NaN and the infinities; NULL text and bytes; no bytes
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_double(statement, 6u, NAN));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_text(statement, 7u, NULL));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_blob(statement, 8u, bytes, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(statement, NULL));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_double(statement, 6u, INFINITY));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_blob(statement, 8u, NULL, 2u));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(statement, NULL));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_double(statement, 6u, -INFINITY));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(statement, NULL));

    // cleared: NULL throughout
    D_TESTS_POSTGRES_OK(db, d_postgres_clear_bindings(statement));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(statement, NULL));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_query_rows(db,
                                              &rows,
                                              "SELECT n, t, f, lo, hi, "
                                              "r = 0.30000000000000004, "
                                              "s, b, r FROM v ORDER BY k"));
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 9u) &&
                            (rows.row_count == 5u) );

    // the rows: every kind; NaN, NULLs and no bytes; the infinities; NULLs
    if ( (rows.column_count == 9u) &&
         (rows.row_count == 5u) )
    {
        const struct d_db_value* const v = rows.values;

        D_TESTS_POSTGRES_CHECK(v[0].type == D_DB_VALUE_NULL);
        D_TESTS_POSTGRES_CHECK( (v[1].as.integer == 1) &&
                                (v[2].as.integer == 0) );
        D_TESTS_POSTGRES_CHECK( (v[3].as.integer == INT64_MIN) &&
                                (v[4].as.integer == INT64_MAX) );
        D_TESTS_POSTGRES_CHECK(v[5].as.integer == 1);
        D_TESTS_POSTGRES_CHECK( (v[6].type == D_DB_VALUE_TEXT) &&
                                (strcmp(v[6].as.text, "it's") == 0) );
        D_TESTS_POSTGRES_CHECK( (v[7].type == D_DB_VALUE_BLOB) &&
                                (v[7].size == 3u)              &&
                                (memcmp(v[7].as.blob, "\0\1\0", 3u) == 0) );
        D_TESTS_POSTGRES_CHECK( (v[9 + 8].type == D_DB_VALUE_REAL) &&
                                (isnan(v[9 + 8].as.real)) );
        D_TESTS_POSTGRES_CHECK(v[9 + 6].type == D_DB_VALUE_NULL);
        D_TESTS_POSTGRES_CHECK( (v[9 + 7].type == D_DB_VALUE_BLOB) &&
                                (v[9 + 7].size == 0u) );
        D_TESTS_POSTGRES_CHECK( (isinf(v[18 + 8].as.real))  &&
                                (v[18 + 8].as.real > 0.0)   &&
                                (v[18 + 7].type == D_DB_VALUE_NULL) );
        D_TESTS_POSTGRES_CHECK( (isinf(v[27 + 8].as.real)) &&
                                (v[27 + 8].as.real < 0.0) );
        D_TESTS_POSTGRES_CHECK( (v[36 + 1].type == D_DB_VALUE_NULL) &&
                                (v[36 + 4].type == D_DB_VALUE_NULL) &&
                                (v[36 + 8].type == D_DB_VALUE_NULL) );
    }

    d_db_rows_free(&rows);

    // numbers the statement does not have
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_null(statement, 0u),
                            D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_int64(statement, 9u, 1),
                            D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_bool(statement, 9u, true),
                            D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_double(statement, 9u, 1.0),
                            D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_double(statement, 0u, NAN),
                            D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_text(statement, 0u, "x"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_blob(statement, 9u, "x", 1u),
                            D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER);
    D_TESTS_POSTGRES_CHECK(
        strcmp(d_postgres_result_message(
                   D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER),
               "undefined parameter") == 0);
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(statement));

    // inferred types: a boolean as an integer, text as a number's input
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &statement,
                                           "SELECT $1::integer + 1, "
                                           "$2::numeric * 2, $3"));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_bool(statement, 1u, true));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_text(statement, 2u, "1.25"));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_text(statement, 3u, "\xc3\xa9"));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_query(statement, &rows));
    D_TESTS_POSTGRES_CHECK( (rows.row_count == 1u)                         &&
                            (rows.values[0].as.integer == 2)               &&
                            (strcmp(rows.values[1].as.text, "2.50") == 0)  &&
                            (strcmp(rows.values[2].as.text,
                                    "\xc3\xa9") == 0)                      &&
                            (rows.columns[2].type == D_DB_TYPE_STRING) );
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_text(statement, 1u, "x"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execute_query(statement, &rows),
                            d_postgres_result_from_sqlstate("22P02"));
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(statement));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_running
  Running statements, once and again.
  Tests the following:
  - execute_update counts an INSERT's rows; values stay bound between runs
  - a run that fails is the server's SQLSTATE, writes no count, and leaves
    the statement ready: the next run, rebound, succeeds
  - execute_update counts a SELECT's rows, and none for CREATE TABLE;
    execute_query returns a SELECT's rows, the same on a second run, text
    longer than any before it whole
  - execute_query of a statement without rows gives empty rows, and runs
    it; RETURNING gives rows; COPY with the client is D_DB_UNSUPPORTED,
    ended so that the connection runs its next statement
  - a statement survives a transaction rolled back; inside a failed one a
    run is refused, and finalizing drops the server's copy where libpq can
    close it there (17), and otherwise leaves it to the session
*/
static void
d_tests_postgres_running(
    void
)
{
    char                         schema[96] = "";
    struct pg_conn*              db         = d_tests_postgres_begin("running",
                                                                     schema,
                                                                     sizeof(
                                                                       schema));
    struct d_postgres_statement* insert     = NULL;
    struct d_postgres_statement* select     = NULL;
    struct d_db_rows             rows       = { 0u, 0u, NULL, NULL };
    int64_t                      changes    = -1;
    static char                  longest[70001] = "";
#if defined(LIBPQ_HAS_CLOSE_PREPARED)
    const int                    closed     = 0;  // closed by the protocol
    const int64_t                left       = 0;
#else
    const int                    closed     =     // DEALLOCATE, refused
        D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION;
    const int64_t                left       = 1;
#endif

    printf("postgres_statement: running\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    memset(longest, 'x', sizeof(longest) - 1u);
    longest[sizeof(longest) - 1u] = '\0';

    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "CREATE TABLE k (id integer PRIMARY "
                                        "KEY, v text)"));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &insert,
                                           "INSERT INTO k VALUES ($1, $2)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(insert, 1u, 1));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_text(insert, 2u, "one"));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(insert, &changes));
    D_TESTS_POSTGRES_CHECK(changes == 1);

    // the same values again: the key collides
    changes = -1;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execute_update(insert, &changes),
                            D_POSTGRES_SQLSTATE_UNIQUE_VIOLATION);
    D_TESTS_POSTGRES_CHECK(changes == -1);
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(insert, 1u, 2));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_text(insert, 2u, longest));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(insert, &changes));
    D_TESTS_POSTGRES_CHECK(changes == 1);

    // a SELECT, counted, then collected twice
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &select,
                                           "SELECT id, v FROM k "
                                           "WHERE id >= $1 ORDER BY id"));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(select, 1u, 1));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(select, &changes));
    D_TESTS_POSTGRES_CHECK(changes == 2);

    for (int run = 0; run < 2; ++run)
    {
        D_TESTS_POSTGRES_OK(db, d_postgres_execute_query(select, &rows));
        D_TESTS_POSTGRES_CHECK( (rows.column_count == 2u)                 &&
                                (rows.row_count == 2u)                    &&
                                (rows.values[0].as.integer == 1)          &&
                                (strcmp(rows.values[1].as.text,
                                        "one") == 0)                      &&
                                (rows.values[3].size ==
                                 sizeof(longest) - 1u)                    &&
                                (strcmp(rows.values[3].as.text,
                                        longest) == 0) );
        d_db_rows_free(&rows);
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(select));

    // without rows; RETURNING; CREATE TABLE
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(insert, 1u, 3));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_query(insert, &rows));
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 0u) &&
                            (rows.row_count == 0u) );
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT count(*) FROM k")
                           == 3);
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &select,
                                           "UPDATE k SET v = $1 "
                                           "WHERE id > 1 RETURNING id"));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_text(select, 1u, "two"));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_query(select, &rows));
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 1u) &&
                            (rows.row_count == 2u) );
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(select, &changes));
    D_TESTS_POSTGRES_CHECK(changes == 2);
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(select));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &select,
                                           "CREATE TABLE later (a integer)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(select, &changes));
    D_TESTS_POSTGRES_CHECK(changes == 0);
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(select));

    // COPY with the client: ended at once, the connection ready after
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &select, "COPY k TO STDOUT"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execute_update(select, NULL),
                            D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execute_query(select, &rows),
                            D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT count(*) FROM k")
                           == 3);
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(select));

    // transactions: rolled back, then failed
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "BEGIN"));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &select,
                                           "SELECT count(*) FROM k"));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "ROLLBACK"));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_query(select, &rows));
    D_TESTS_POSTGRES_CHECK( (rows.row_count == 1u) &&
                            (rows.values[0].as.integer == 3) );
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "BEGIN"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(db, "SELECT 1 / 0"),
                            d_postgres_result_from_sqlstate("22012"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execute_update(insert, NULL),
                            D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_finalize(select), closed);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "ROLLBACK"));
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(insert));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db,
                             "SELECT count(*) FROM pg_prepared_statements")
        == left);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_statement_arguments
  The statement functions' argument checks.
  Tests the following:
  - a NULL connection, statement pointer, text or format is
    D_DB_INVALID_ARGUMENT, the statement left NULL where there is one
  - every bind, clear and run refuses a NULL statement, as execute_query
    refuses NULL rows; the describers answer 0 or NULL
*/
static void
d_tests_postgres_statement_arguments(
    void
)
{
    struct pg_conn*              db        = d_tests_postgres_connect(NULL);
    struct d_postgres_statement* statement = NULL;
    struct d_db_rows             rows      = { 0u, 0u, NULL, NULL };
    int64_t                      changes   = 0;

    printf("postgres_statement: arguments\n");

    statement = (struct d_postgres_statement*)&g_options;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_prepare(NULL, &statement,
                                                   "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(statement == NULL);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_prepare(db, NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_prepare(db, &statement, NULL),
                            D_DB_INVALID_ARGUMENT);
    statement = (struct d_postgres_statement*)&g_options;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_preparef(NULL, &statement,
                                                    "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(statement == NULL);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_preparef(db, NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_preparef(db, &statement, NULL),
                            D_DB_INVALID_ARGUMENT);

    D_TESTS_POSTGRES_CHECK(d_postgres_parameter_count(NULL) == 0u);
    D_TESTS_POSTGRES_CHECK(d_postgres_statement_connection(NULL) == NULL);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_null(NULL, 1u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_bool(NULL, 1u, true),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_int64(NULL, 1u, 1),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_double(NULL, 1u, 1.0),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_double(NULL, 1u, NAN),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_text(NULL, 1u, "x"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_bind_blob(NULL, 1u, "x", 1u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_clear_bindings(NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execute_update(NULL, &changes),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execute_query(NULL, &rows),
                            D_DB_INVALID_ARGUMENT);

    // nothing more without a server
    if (db)
    {
        D_TESTS_POSTGRES_OK(db, d_postgres_prepare(db, &statement,
                                                   "SELECT 1"));
        D_TESTS_POSTGRES_RESULT(db, d_postgres_execute_query(statement, NULL),
                                D_DB_INVALID_ARGUMENT);
        D_TESTS_POSTGRES_OK(db, d_postgres_finalize(statement));
    }

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_closing_by_name
  Closing a statement by its name.
  Tests the following:
  - a statement PREPARE made, closed; closing it again, or one the session
    never had, succeeds
  - in a failed transaction: closed where libpq has PQclosePrepared (17),
    and 25P02 where it has not
  - NULL arguments and an empty name
*/
static void
d_tests_postgres_closing_by_name(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("closing",
                                                        schema,
                                                        sizeof(schema));
    const char*     held       = "SELECT count(*) FROM "
                                 "pg_catalog.pg_prepared_statements WHERE "
                                 "name LIKE 'p_close%'";

    printf("postgres_statement: closing by name\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "PREPARE p_close_one AS SELECT 1"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, held) == 1);
    D_TESTS_POSTGRES_OK(db, d_postgres_close_prepared(db, "p_close_one"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, held) == 0);
    D_TESTS_POSTGRES_OK(db, d_postgres_close_prepared(db, "p_close_one"));
    D_TESTS_POSTGRES_OK(db, d_postgres_close_prepared(db, "p_close_never"));

    // in a failed transaction
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "PREPARE p_close_two AS SELECT 2"));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "BEGIN"));
    D_TESTS_POSTGRES_CHECK(d_postgres_exec(db, "SELECT 1 / 0") != 0);
    D_TESTS_POSTGRES_RESULT(
        db,
        d_postgres_close_prepared(db, "p_close_two"),
        (d_postgres_client_supports(D_POSTGRES_CLIENT_CLOSE_PREPARED))
            ? 0
            : D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "ROLLBACK"));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db, held) ==
        ((d_postgres_client_supports(D_POSTGRES_CLIENT_CLOSE_PREPARED)) ? 0
                                                                        : 1));
    D_TESTS_POSTGRES_OK(db, d_postgres_close_prepared(db, "p_close_two"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, held) == 0);

    D_TESTS_POSTGRES_RESULT(NULL,
                            d_postgres_close_prepared(NULL, "p_close_one"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_close_prepared(db, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_close_prepared(db, ""),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_cursors
  Rows a few at a time.
  Tests the following:
  - each fetch returns the columns, named and typed as execute_query's are,
    and the next rows, no more than asked for, each value as execute_query
    reads it
  - a short page ends the run; a fetch after is D_DB_INVALID_ARGUMENT, and
    the connection runs its next statement
  - while a run is open, everything that would wait on the connection -- a
    statement, a query with or without values, a formatted one, a
    notification, a COPY, a prepare, another statement's run, a close by
    name -- is 08000, the layer's own answer, having read none of the run's
    rows; a send is libpq's own 08000; values bound meanwhile serve the next
    run
  - rows that fill their last page exactly end with an empty page, which
    still describes the columns; a query finding no rows describes them too
  - a command runs on opening, holds the connection until its first fetch,
    which ends the run with no columns
  - running the statement again part-way through a run ends it and begins
    another; close_cursor ends one part-way, and again does nothing
  - another statement finalized part-way through a run is released with
    08000, its server copy kept; the statement's own finalize ends its run
  - a row failing part-way through a run is the server's failure, which the
    connection still describes; the run has ended, and the connection runs
    its next statement; a failure among the rows close_cursor reads away is
    reported too
  - pipeline mode refuses a run; NULL arguments, no rows asked for, and no
    run open are D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_cursors(
    void
)
{
    char                         schema[96] = "";
    struct pg_conn*              db         = d_tests_postgres_begin("cursors",
                                                                     schema,
                                                                     sizeof(
                                                                       schema));
    struct d_postgres_statement* select     = NULL;
    struct d_postgres_statement* other      = NULL;
    struct d_postgres_statement* insert     = NULL;
    struct d_db_rows             all        = { 0u, 0u, NULL, NULL };
    struct d_db_rows             rows       = { 0u, 0u, NULL, NULL };
    struct d_db_rows             marker     = { 7u, 7u, NULL, NULL };
    int64_t                      changes    = -1;
    const char* const            values[1]  = { "1" };
    const char*                  held       = "SELECT count(*) FROM "
                                              "pg_catalog.pg_prepared_statements";
    const int                    by_zero    =
        d_postgres_result_from_sqlstate("22012");
    const int                    busy       =
        D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION;

    printf("postgres_statement: rows a few at a time\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "CREATE TABLE k (id int PRIMARY KEY, "
                                        "v text, r float8, b bytea, "
                                        "d numeric(10,2), t timestamp)"));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "INSERT INTO k VALUES "
                                        "(1, 'a', 1.5, '\\x00ff', 1.25, "
                                        "'2020-01-02 03:04:05'), "
                                        "(2, NULL, NULL, NULL, NULL, NULL), "
                                        "(3, repeat('x', 70000), -0.5, "
                                        "'\\x', 3, '2001-01-01'), "
                                        "(4, 'it''s', 0, '\\x', -2.50, "
                                        "'1999-12-31 23:59:59'), "
                                        "(5, '', 1e300, '\\x61', 0.00, "
                                        "'2000-02-29 00:00:00')"));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &select,
                                           "SELECT id, v, r, b, d, t FROM k "
                                           "WHERE id >= $1 ORDER BY id"));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &other,
                                           "SELECT count(*) FROM k"));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(select, 1u, 1));

    // the rows all at once, to hold the pages to
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_query(select, &all));
    D_TESTS_POSTGRES_CHECK( (all.column_count == 6u) &&
                            (all.row_count == 5u)    &&
                            (all.values[13].size == 70000u) );

    // pages of two: 1 and 2, 3 and 4, then 5 alone, which ends the run
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 2u, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 0u, 2u));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 2u, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 2u, 2u));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 2u, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 4u, 1u));
    d_db_rows_free(&rows);
    rows = marker;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_fetch_rows(select, 2u, &rows),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 7u) &&
                            (rows.row_count == 7u) );
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 7") == 7);

    // part-way through, the connection is the run's
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 1u, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 0u, 1u));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(db, "SELECT 1"), busy);
    D_TESTS_POSTGRES_CHECK(d_postgres_result_is_own(db, busy));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_query_rows(db, &rows, "SELECT 1"),
                            busy);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rows_params(db, &rows,
                                                         "SELECT $1::int",
                                                         values, 1u),
                            busy);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_query_rowsf(db, &rows, "SELECT %d", 1),
                            busy);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_notify(db, "cursors", "x"), busy);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_copy_in_start(db,
                                                     "COPY k (id) FROM STDIN"),
                            busy);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execute_update(other, &changes),
                            busy);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_execute_query(other, &rows), busy);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_open_cursor(other), busy);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_fetch_rows(other, 1u, &rows),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_close_cursor(other), 0);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_close_prepared(db, "p_none"),
                            busy);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_send_query(db, "SELECT 1"), busy);
    insert = (struct d_postgres_statement*)&g_options;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_prepare(db, &insert, "SELECT 1"),
                            busy);
    D_TESTS_POSTGRES_CHECK(insert == NULL);
    D_TESTS_POSTGRES_CHECK(changes == -1);
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(select, 1u, 4));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 9u, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 1u, 4u));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_update(other, &changes));
    D_TESTS_POSTGRES_CHECK(changes == 1);

    // the value bound part-way serves the next run: from 4, two rows filling
    // a page exactly, and then an empty page, which ends the run
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 2u, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 3u, 2u));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(db, "SELECT 1"), busy);
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 2u, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 5u, 0u));
    D_TESTS_POSTGRES_CHECK( (rows.columns != NULL) &&
                            (rows.values == NULL) );
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 7") == 7);

    // no row found: the columns alone
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(select, 1u, 9));
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 4u, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 5u, 0u));
    d_db_rows_free(&rows);

    // a command runs on opening, and holds the connection until its first
    // fetch
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &insert,
                                           "INSERT INTO k (id) VALUES ($1)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(insert, 1u, 6));
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(insert));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(db, "SELECT 1"), busy);
    rows = marker;
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(insert, 3u, &rows));
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 0u) &&
                            (rows.row_count == 0u)    &&
                            (rows.columns == NULL) );
    D_TESTS_POSTGRES_RESULT(db, d_postgres_fetch_rows(insert, 3u, &rows),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT count(*) FROM k")
                           == 6);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "DELETE FROM k WHERE id = 6"));
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(insert));

    // running the statement again part-way: its run ends, another begins
    D_TESTS_POSTGRES_OK(db, d_postgres_bind_int64(select, 1u, 1));
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 2u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_query(select, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 0u, 5u));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_fetch_rows(select, 2u, &rows),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 1u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 2u, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 0u, 2u));
    d_db_rows_free(&rows);

    // close_cursor part-way, and again
    D_TESTS_POSTGRES_OK(db, d_postgres_close_cursor(select));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 7") == 7);
    D_TESTS_POSTGRES_OK(db, d_postgres_close_cursor(select));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_fetch_rows(select, 1u, &rows),
                            D_DB_INVALID_ARGUMENT);

    // another statement finalized part-way: released, its copy kept
    const int64_t copies = d_tests_postgres_int(db, held);

    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 1u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_finalize(other), busy);
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 9u, &rows));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_same_page(&rows, &all, 1u, 4u));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, held) == copies);

    // a statement prepared since, inside a transaction, comes to no harm
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "BEGIN"));
    D_TESTS_POSTGRES_OK(db, d_postgres_prepare(db, &other, "SELECT 11"));
    D_TESTS_POSTGRES_OK(db, d_postgres_execute_query(other, &rows));
    D_TESTS_POSTGRES_CHECK(rows.row_count == 1u);
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "COMMIT"));
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(other));

    // the statement's own finalize ends its run
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 1u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(select));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 7") == 7);

    // a row failing part-way: the third divides by zero (no sort, which
    // would have the server compute every row before sending the first)
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_prepare(db, &select,
                                           "SELECT i, CASE WHEN i < 3 "
                                           "THEN i ELSE 1 / (i - i) END "
                                           "FROM generate_series(1, 5) "
                                           "AS i"));
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 2u, &rows));
    D_TESTS_POSTGRES_CHECK( (rows.row_count == 2u) &&
                            (rows.values[3].as.integer == 2) );
    d_db_rows_free(&rows);
    rows = marker;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_fetch_rows(select, 2u, &rows),
                            by_zero);
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 7u) &&
                            (rows.row_count == 7u) );
    D_TESTS_POSTGRES_CHECK(!d_postgres_result_is_own(db, by_zero));
    D_TESTS_POSTGRES_CHECK(strstr(d_postgres_error_message(db),
                                  "division by zero") != NULL);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_fetch_rows(select, 2u, &rows),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 7") == 7);

    // and closed before it comes: the failure is still the run's
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 1u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_close_cursor(select), by_zero);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 7") == 7);
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(select));

    // pipeline mode reads its results its own way
    if (d_postgres_client_supports(D_POSTGRES_CLIENT_PIPELINE))
    {
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_prepare(db, &select, "SELECT 1"));
        D_TESTS_POSTGRES_OK(db, d_postgres_enter_pipeline(db));
        D_TESTS_POSTGRES_RESULT(db, d_postgres_open_cursor(select), busy);
        D_TESTS_POSTGRES_OK(db, d_postgres_exit_pipeline(db));
        D_TESTS_POSTGRES_OK(db, d_postgres_finalize(select));
    }

    // arguments
    D_TESTS_POSTGRES_OK(db, d_postgres_prepare(db, &select, "SELECT 1"));
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_open_cursor(NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_fetch_rows(NULL, 1u, &rows),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_close_cursor(NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_fetch_rows(select, 0u, &rows),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_fetch_rows(select, 1u, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_OK(db, d_postgres_close_cursor(select));
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(select));

    d_db_rows_free(&all);
    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_large_objects
  Large objects, to and from the client's files.
  Tests the following:
  - a file of more than two chunks, imported and exported again, byte for
    byte; an empty file too; outside a transaction each copy commits
  - inside the caller's transaction the copy is the transaction's: rolled
    back, the object is gone, and exporting it is 42704, the server's
  - a file that is not there is D_DB_NOT_FOUND, and a path that names a
    directory 58030, the layer's own, the session untouched
  - a failed transaction is 25P02, and a run that holds the connection
    08000, each the layer's own
  - NULL arguments are D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_large_objects(
    void
)
{
    char                         schema[96] = "";
    struct pg_conn*              db         = d_tests_postgres_begin(
                                                  "large_objects",
                                                  schema,
                                                  sizeof(schema));
    const char* const            in         = "d_tests_postgres_lo_in.bin";
    const char* const            out        = "d_tests_postgres_lo_out.bin";
    const size_t                 size       = 150001u;
    unsigned char*               bytes      = (unsigned char*)malloc(size);
    unsigned char*               back       = (unsigned char*)malloc(size);
    unsigned int                 oid        = 0u;
    unsigned int                 empty      = 0u;
    struct d_postgres_statement* select     = NULL;
    struct d_db_rows             rows       = { 0u, 0u, NULL, NULL };
    const int                    undefined  =
        d_postgres_result_from_sqlstate("42704");

    printf("postgres_protocol: large objects\n");

    // nothing more without a server, or memory
    if ( (!db)    ||
         (!bytes) ||
         (!back) )
    {
        D_TESTS_POSTGRES_CHECK( (db != NULL) && (bytes != NULL) &&
                                (back != NULL) );
        free(bytes);
        free(back);
        d_tests_postgres_end(db, schema);

        return;
    }

    // a file of every byte value, over two chunks and a byte
    for (size_t i = 0u; i < size; ++i)
    {
        bytes[i] = (unsigned char)((i * 7u) & 0xFFu);
    }

    FILE* file = fopen(in, "wb");

    D_TESTS_POSTGRES_CHECK( (file != NULL) &&
                            (fwrite(bytes, 1u, size, file) == size) &&
                            (fclose(file) == 0) );

    // in and out again, outside a transaction
    D_TESTS_POSTGRES_OK(db, d_postgres_lo_import(db, in, &oid));
    D_TESTS_POSTGRES_CHECK( (oid != 0u) &&
                            (PQtransactionStatus(db) == PQTRANS_IDLE) );
    D_TESTS_POSTGRES_OK(db, d_postgres_lo_export(db, oid, out));
    file = fopen(out, "rb");
    D_TESTS_POSTGRES_CHECK( (file != NULL) &&
                            (fread(back, 1u, size, file) == size) &&
                            (fgetc(file) == EOF) &&
                            (memcmp(bytes, back, size) == 0) );

    if (file)
    {
        (void)fclose(file);
    }

    // an empty file
    file = fopen(in, "wb");
    D_TESTS_POSTGRES_CHECK( (file != NULL) && (fclose(file) == 0) );
    D_TESTS_POSTGRES_OK(db, d_postgres_lo_import(db, in, &empty));
    D_TESTS_POSTGRES_OK(db, d_postgres_lo_export(db, empty, out));
    file = fopen(out, "rb");
    D_TESTS_POSTGRES_CHECK( (file != NULL) && (fgetc(file) == EOF) );

    if (file)
    {
        (void)fclose(file);
    }

    // inside the caller's transaction, rolled back with it
    {
        unsigned int brief = 0u;

        D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "BEGIN"));
        D_TESTS_POSTGRES_OK(db, d_postgres_lo_import(db, in, &brief));
        D_TESTS_POSTGRES_CHECK(PQtransactionStatus(db) == PQTRANS_INTRANS);
        D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "ROLLBACK"));
        D_TESTS_POSTGRES_RESULT(db, d_postgres_lo_export(db, brief, out),
                                undefined);
        D_TESTS_POSTGRES_CHECK(!d_postgres_result_is_own(db, undefined));
        D_TESTS_POSTGRES_CHECK(PQtransactionStatus(db) == PQTRANS_IDLE);
    }

    // files the client cannot have
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_lo_import(db, "d_tests_no_such_file",
                                                 &oid),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_lo_export(db, oid, "."),
                            D_POSTGRES_SQLSTATE_IO_ERROR);
    D_TESTS_POSTGRES_CHECK(
        d_postgres_result_is_own(db, D_POSTGRES_SQLSTATE_IO_ERROR));
    D_TESTS_POSTGRES_CHECK(PQtransactionStatus(db) == PQTRANS_IDLE);

    // a failed transaction, and a run holding the connection
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "BEGIN"));
    D_TESTS_POSTGRES_CHECK(d_postgres_exec(db, "SELECT 1/0") != 0);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_lo_export(db, oid, out),
                            D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
    D_TESTS_POSTGRES_CHECK(
        d_postgres_result_is_own(
            db,
            D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "ROLLBACK"));
    D_TESTS_POSTGRES_OK(db, d_postgres_prepare(db, &select, "SELECT 1"));
    D_TESTS_POSTGRES_OK(db, d_postgres_open_cursor(select));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_lo_import(db, in, &oid),
                            D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    D_TESTS_POSTGRES_OK(db, d_postgres_fetch_rows(select, 2u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_finalize(select));

    // arguments
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_lo_import(NULL, in, &oid),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_lo_import(db, NULL, &oid),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_lo_import(db, in, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_lo_export(NULL, oid, out),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_lo_export(db, oid, NULL),
                            D_DB_INVALID_ARGUMENT);

    (void)d_postgres_exec(db, "SELECT lo_unlink(oid) FROM "
                              "pg_catalog.pg_largeobject_metadata");
    (void)remove(in);
    (void)remove(out);
    free(bytes);
    free(back);
    d_tests_postgres_end(db, schema);

    return;
}

//==============================================================================
// postgres_database.h
//==============================================================================

/*
d_tests_postgres_databases
  Databases: has, create, drop, list.
  Tests the following:
  - a created database is there, listed in byte order with the server's own,
    and gone once dropped
  - creating one that is there is 42P04, a success under D_DB_IF_NOT_EXISTS;
    dropping one that is not is 3D000, a success under D_DB_IF_EXISTS
  - the current database is the one connected to
*/
static void
d_tests_postgres_databases(
    void
)
{
    struct pg_conn*       db       = d_tests_postgres_connect(NULL);
    struct d_db_name_list names    = { 0u, NULL };
    char                  name[96] = "";

    printf("postgres_database: databases\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    d_tests_postgres_schema_name(name, sizeof(name), "db");
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_database(db, name,
                                                     D_DB_IF_EXISTS));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_database(db, name));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_database(db, name, 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_database(db, name));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_database(db, name, 0u),
                            D_POSTGRES_SQLSTATE_DUPLICATE_DATABASE);
    D_TESTS_POSTGRES_OK(db, d_postgres_create_database(db, name,
                                                       D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_databases(db, &names));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_has_name(&names, name));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_has_name(&names, "template1"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_has_name(&names,
                                                     g_options.database));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_sorted(&names));
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_database(db, name, 0u));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_database(db, name));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_drop_database(db, name, 0u),
                            D_POSTGRES_SQLSTATE_INVALID_CATALOG_NAME);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_database(db, name,
                                                     D_DB_IF_EXISTS));
    D_TESTS_POSTGRES_CHECK(strcmp(d_postgres_current_database(db),
                                  g_options.database) == 0);
    D_TESTS_POSTGRES_CHECK(d_postgres_current_database(NULL) == NULL);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_database_transactions
  Databases and transactions, which do not mix.
  Tests the following:
  - creating or dropping one inside a transaction is the server's 25001,
    with D_DB_IF_NOT_EXISTS too, and the database is not made
  - dropping the database connected to is 55006
*/
static void
d_tests_postgres_database_transactions(
    void
)
{
    struct pg_conn* db       = d_tests_postgres_connect(NULL);
    char            name[96] = "";

    printf("postgres_database: databases and transactions\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    d_tests_postgres_schema_name(name, sizeof(name), "dbtxn");
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db,
                                D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_database(db, name, 0u),
                            D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION);
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db,
                                D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_database(db, name,
                                                       D_DB_IF_NOT_EXISTS),
                            D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION);
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_database(db, name));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_drop_database(db, g_options.database,
                                                     0u),
                            D_POSTGRES_SQLSTATE_OBJECT_IN_USE);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_drop_force
  Dropping a database other sessions are connected to.
  Tests the following:
  - without D_POSTGRES_FORCE it is 55006, and the database stays
  - with it, from 13, the sessions are ended and the database dropped;
    before 13 it is D_DB_UNSUPPORTED, sending nothing
*/
static void
d_tests_postgres_drop_force(
    void
)
{
    struct pg_conn* db       = d_tests_postgres_connect(NULL);
    struct pg_conn* other    = NULL;
    char            name[96] = "";

    printf("postgres_database: dropping with FORCE\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    d_tests_postgres_schema_name(name, sizeof(name), "dbforce");
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_database(db, name,
                                                     D_DB_IF_EXISTS));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_database(db, name, 0u));
    other = d_tests_postgres_connect(name);
    D_TESTS_POSTGRES_CHECK(other != NULL);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_drop_database(db, name, 0u),
                            D_POSTGRES_SQLSTATE_OBJECT_IN_USE);
    D_TESTS_POSTGRES_CHECK(d_postgres_has_database(db, name));

    if (d_tests_postgres_at_least(db, 13))
    {
        D_TESTS_POSTGRES_OK(db, d_postgres_drop_database(db, name,
                                                         D_POSTGRES_FORCE));
        D_TESTS_POSTGRES_CHECK(!d_postgres_has_database(db, name));
        D_TESTS_POSTGRES_CHECK(d_postgres_ping(other) != 0);
    }
    else
    {
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_drop_database(db, name,
                                                         D_POSTGRES_FORCE),
                                D_DB_UNSUPPORTED);
        d_postgres_close(other);
        other = NULL;
        D_TESTS_POSTGRES_OK(db, d_postgres_drop_database(db, name, 0u));
    }

    d_postgres_close(other);
    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_schemas
  Schemas: has, create, drop, list.
  Tests the following:
  - a created schema is there, listed in byte order without the system's
    own, which are there all the same
  - creating one that is there is 42P06, a success under D_DB_IF_NOT_EXISTS;
    a name beginning pg_ is the server's 42939
  - dropping one that holds anything is 2BP01, a success under
    D_POSTGRES_CASCADE; one that is not there is 3F000, a success under
    D_DB_IF_EXISTS
*/
static void
d_tests_postgres_schemas(
    void
)
{
    struct pg_conn*       db       = d_tests_postgres_connect(NULL);
    struct d_db_name_list names    = { 0u, NULL };
    char                  name[96] = "";

    printf("postgres_database: schemas\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    d_tests_postgres_schema_name(name, sizeof(name), "ns");
    D_TESTS_POSTGRES_OK(db, d_postgres_create_schema(db, name, 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_schema(db, name));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_schema(db, "pg_catalog"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_schema(db, name, 0u),
                            D_POSTGRES_SQLSTATE_DUPLICATE_SCHEMA);
    D_TESTS_POSTGRES_OK(db, d_postgres_create_schema(db, name,
                                                     D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_schema(db, "pg_mine", 0u),
                            d_postgres_result_from_sqlstate("42939"));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_schemas(db, &names));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_has_name(&names, name));
    D_TESTS_POSTGRES_CHECK(!d_tests_postgres_has_name(&names, "pg_catalog"));
    D_TESTS_POSTGRES_CHECK(!d_tests_postgres_has_name(&names,
                                                      "information_schema"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_sorted(&names));
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_OK(db, d_postgres_execf(db,
                                             "CREATE TABLE %T (a integer)",
                                             name,
                                             "t"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_schema(db, name, 0u),
                            D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_schema(db, name,
                                                   D_POSTGRES_CASCADE));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_schema(db, name));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_schema(db, name, 0u),
                            D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_schema(db, name, D_DB_IF_EXISTS));

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_temporary_schema
  "pg_temp", the session's temporary schema.
  Tests the following:
  - it is there only once the session makes a temporary table
  - a table created in it is temporary, and found only there, not in the
    current schema
*/
static void
d_tests_postgres_temporary_schema(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("tmp",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_database: the temporary schema\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_CHECK(!d_postgres_has_schema(db, "pg_temp"));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_table_sql(db, "pg_temp", "tt",
                                                        "a integer", 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_schema(db, "pg_temp"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, "pg_temp", "tt"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "tt"));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_text_is(db,
                                 "SELECT relpersistence FROM pg_class "
                                 "WHERE relname = 'tt'",
                                 "t"));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_current_schema
  d_postgres_current_schema.
  Tests the following:
  - the first schema of the search path that exists
  - none at all is D_DB_NOT_FOUND, and a create in no schema the server's
    3F000, as the layer gives it itself
*/
static void
d_tests_postgres_current_schema(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("current",
                                                        schema,
                                                        sizeof(schema));
    char*           name       = NULL;

    printf("postgres_database: the current schema\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_current_schema(db, &name));
    D_TESTS_POSTGRES_CHECK( (name) && (strcmp(name, schema) == 0) );
    d_db_free(name);
    name = NULL;
    D_TESTS_POSTGRES_OK(db, d_postgres_set_search_path(db,
                                                       "no_such, also_not"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_current_schema(db, &name),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(name == NULL);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table_sql(db, NULL, "t",
                                                        "a integer", 0u),
                            D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME);
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "t"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_current_schema(NULL, &name),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_current_schema(db, NULL),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_extension_available
  Whether the server can install an extension, as pg_available_extensions
lists it.
*/
static bool
d_tests_postgres_extension_available(
    struct pg_conn* _db,
    const char*     _name
)
{
    bool available = false;

    return ( (d_postgres_query_boolf(_db,
                                     &available,
                                     "SELECT EXISTS (SELECT 1 FROM "
                                     "pg_catalog.pg_available_extensions "
                                     "WHERE name = %Q)",
                                     _name) == 0) &&
             (available) );
}

/*
d_tests_postgres_citext
  citext's life: installed into the schema named, 42710 when it is there and
a success under D_DB_IF_NOT_EXISTS; refused while a column has its type,
2BP01, and dropped with it under D_POSTGRES_CASCADE; 42704 when it is gone,
and a success under D_DB_IF_EXISTS.
*/
static void
d_tests_postgres_citext(
    struct pg_conn* _db,
    const char*     _schema
)
{
    (void)d_postgres_drop_extension(_db, "citext", D_DB_IF_EXISTS);
    D_TESTS_POSTGRES_OK(_db, d_postgres_create_extension(_db, "citext",
                                                         _schema, 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_extension(_db, "citext"));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_text_is(_db,
                                 "SELECT extnamespace::regnamespace::text "
                                 "FROM pg_extension WHERE extname = 'citext'",
                                 _schema));
    D_TESTS_POSTGRES_RESULT(_db,
                            d_postgres_create_extension(_db, "citext", NULL,
                                                        0u),
                            D_POSTGRES_SQLSTATE_DUPLICATE_OBJECT);
    D_TESTS_POSTGRES_OK(_db, d_postgres_create_extension(_db, "citext", NULL,
                                                         D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_OK(_db, d_postgres_exec(_db,
                                             "CREATE TABLE ci (c citext)"));
    D_TESTS_POSTGRES_RESULT(_db, d_postgres_drop_extension(_db, "citext", 0u),
                            D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST);
    D_TESTS_POSTGRES_OK(_db, d_postgres_drop_extension(_db, "citext",
                                                       D_POSTGRES_CASCADE));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_extension(_db, "citext"));
    D_TESTS_POSTGRES_RESULT(_db, d_postgres_drop_extension(_db, "citext", 0u),
                            D_POSTGRES_SQLSTATE_UNDEFINED_OBJECT);
    D_TESTS_POSTGRES_OK(_db, d_postgres_drop_extension(_db, "citext",
                                                       D_DB_IF_EXISTS));

    return;
}

/*
d_tests_postgres_extensions
  Extensions: has, create, drop, list.
  Tests the following:
  - plpgsql is installed and listed, the list in byte order
  - citext, where the server has it: installed into the schema named, 42710
    when it is there, a success under D_DB_IF_NOT_EXISTS; refused for a
    column of its type, 2BP01, and dropped under D_POSTGRES_CASCADE; 42704
    when it is not there, a success under D_DB_IF_EXISTS
  - an extension the server cannot install is refused
*/
static void
d_tests_postgres_extensions(
    void
)
{
    char                  schema[96] = "";
    struct pg_conn*       db         = d_tests_postgres_begin("ext",
                                                              schema,
                                                              sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };

    printf("postgres_database: extensions\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_CHECK(d_postgres_has_extension(db, "plpgsql"));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_extensions(db, &names));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_has_name(&names, "plpgsql"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_sorted(&names));
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_CHECK(
        d_postgres_create_extension(db, "no_such_extension", NULL, 0u) > 0);

    // the rest needs contrib's citext
    if (d_tests_postgres_extension_available(db, "citext"))
    {
        d_tests_postgres_citext(db, schema);
    }
    else
    {
        printf("    (citext is not available: its checks are skipped)\n");
    }

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_extension_cascade
  D_POSTGRES_CASCADE on creating an extension.
  Tests the following:
  - earthdistance, where the server has it, installs cube with it, and
    dropping it with D_POSTGRES_CASCADE leaves cube, which nothing needs
*/
static void
d_tests_postgres_extension_cascade(
    void
)
{
    struct pg_conn* db = d_tests_postgres_connect(NULL);

    printf("postgres_database: extensions that need others\n");

    // nothing more without a server, or without contrib's earthdistance
    if ( (!db) ||
         (!d_tests_postgres_extension_available(db, "earthdistance")) )
    {
        printf("    (earthdistance is not available: skipped)\n");
        d_postgres_close(db);

        return;
    }

    (void)d_postgres_drop_extension(db, "earthdistance", D_DB_IF_EXISTS);
    (void)d_postgres_drop_extension(db, "cube", D_DB_IF_EXISTS);
    D_TESTS_POSTGRES_CHECK(d_postgres_create_extension(db, "earthdistance",
                                                       NULL, 0u) != 0);
    D_TESTS_POSTGRES_OK(db, d_postgres_create_extension(db, "earthdistance",
                                                        NULL,
                                                        D_POSTGRES_CASCADE));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_extension(db, "cube"));
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_extension(db, "earthdistance",
                                                      D_POSTGRES_CASCADE));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_extension(db, "cube"));
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_extension(db, "cube", 0u));

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_database_arguments
  The database module's argument checks.
  Tests the following:
  - a NULL connection, a NULL or empty name, an empty schema and an unknown
    flag are D_DB_INVALID_ARGUMENT, and a _s answer then false
  - D_POSTGRES_FORCE before 13 is D_DB_UNSUPPORTED (see drop_force)
*/
static void
d_tests_postgres_database_arguments(
    void
)
{
    struct pg_conn*       db     = d_tests_postgres_connect(NULL);
    struct d_db_name_list names  = { 0u, NULL };
    bool                  exists = true;

    printf("postgres_database: arguments\n");

    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_database_s(db, NULL, &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(!exists);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_schema_s(db, "", &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_extension_s(NULL, "x", &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_database_s(db, "x", NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_database(db, "", 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_database(db, "x",
                                                           D_DB_IF_EXISTS),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_database(NULL, "x", 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_database(db, "x",
                                                         D_POSTGRES_CASCADE),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_databases(db, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_schema(db, NULL, 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_schema(db, "x",
                                                       D_DB_IF_NOT_EXISTS),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_schemas(NULL, &names),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_extension(db, "x", "", 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_extension(db, "x", NULL,
                                                            D_DB_IF_EXISTS),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_extension(db, "", 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_extensions(db, NULL),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(db);

    return;
}

//==============================================================================
// postgres_schema.h: tables
//==============================================================================

/*
d_tests_postgres_table_existence
  d_postgres_has_table and its _s form.
  Tests the following:
  - an ordinary table and a partitioned one are tables; a view, a sequence
    and an index are not
  - another schema is asked by name; one that does not exist has none
*/
static void
d_tests_postgres_table_existence(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("tables",
                                                        schema,
                                                        sizeof(schema));
    bool            exists     = false;

    printf("postgres_schema: table existence\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(
                                db,
                                "CREATE TABLE t (a integer PRIMARY KEY); "
                                "CREATE TABLE p (a integer) "
                                "PARTITION BY RANGE (a); "
                                "CREATE VIEW v AS SELECT 1 AS one; "
                                "CREATE SEQUENCE s"));
    D_TESTS_POSTGRES_OK(db, d_postgres_has_table_s(db, NULL, "t", &exists));
    D_TESTS_POSTGRES_CHECK(exists);
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL, "p"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, schema, "t"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "v"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "s"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "t_pkey"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "T"));
    D_TESTS_POSTGRES_OK(db, d_postgres_has_table_s(db, "no_such_schema", "t",
                                                   &exists));
    D_TESTS_POSTGRES_CHECK(!exists);
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, "public", "t"));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_table_creation
  d_postgres_create_table_sql.
  Tests the following:
  - a table from SQL, in the current schema or one named
  - a name taken is 42P07, a success under D_DB_IF_NOT_EXISTS, by a view
    too, as the server's own clause counts it
  - a schema that does not exist is 3F000
*/
static void
d_tests_postgres_table_creation(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("tcreate",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_schema: table creation\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_create_table_sql(
                                db, NULL, "t",
                                "id bigint PRIMARY KEY, name text NOT NULL",
                                0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL, "t"));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_table_sql(db, schema, "u",
                                                        "a integer", 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL, "u"));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table_sql(db, NULL, "t",
                                                        "a integer", 0u),
                            D_POSTGRES_SQLSTATE_DUPLICATE_TABLE);
    D_TESTS_POSTGRES_OK(db, d_postgres_create_table_sql(db, NULL, "t",
                                                        "a integer",
                                                        D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_column(db, NULL, "t", "name"));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE VIEW v AS SELECT 1"));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_table_sql(db, NULL, "v",
                                                        "a integer",
                                                        D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "v"));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table_sql(db, "no_such", "t",
                                                        "a integer", 0u),
                            D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_table_drops
  d_postgres_drop_table.
  Tests the following:
  - a dropped table is gone; one that is not there is 42P01, a success under
    D_DB_IF_EXISTS
  - a table a view depends on is 2BP01, and dropped with the view under
    D_POSTGRES_CASCADE
*/
static void
d_tests_postgres_table_drops(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("tdrop",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_schema: table drops\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (a integer); "
                                            "CREATE TABLE u (a integer); "
                                            "CREATE VIEW uv AS "
                                            "SELECT a FROM u"));
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_table(db, NULL, "t", 0u));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "t"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_table(db, NULL, "t", 0u),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_table(db, NULL, "t",
                                                  D_DB_IF_EXISTS));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_table(db, NULL, "u", 0u),
                            D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST);
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL, "u"));
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_table(db, schema, "u",
                                                  D_POSTGRES_CASCADE));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "u"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_view(db, NULL, "uv"));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_table_renames
  d_postgres_rename_table.
  Tests the following:
  - a renamed table is there under its new name, and not its old
  - a table that is not there is 42P01; a new name taken, 42P07
*/
static void
d_tests_postgres_table_renames(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("trename",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_schema: table renames\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (a integer); "
                                            "CREATE TABLE taken (a integer)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_rename_table(db, NULL, "t", "New t"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL, "New t"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "t"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_rename_table(db, NULL, "t", "x"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_rename_table(db, schema, "New t",
                                                        "taken"),
                            D_POSTGRES_SQLSTATE_DUPLICATE_TABLE);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_table_rows
  d_postgres_truncate_table and d_postgres_count_rows.
  Tests the following:
  - a count of the rows, and of none after truncation, which restarts the
    table's identity column
  - a table another's foreign key references is refused, 0A000
  - a table that is not there is 42P01, writing no count
*/
static void
d_tests_postgres_table_rows(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("trows",
                                                        schema,
                                                        sizeof(schema));
    int64_t         count      = -1;

    printf("postgres_schema: table rows\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(
                                db,
                                "CREATE TABLE t (id integer GENERATED BY "
                                "DEFAULT AS IDENTITY PRIMARY KEY, a text); "
                                "INSERT INTO t (a) VALUES ('x'), ('y'); "
                                "CREATE TABLE ref (id integer REFERENCES t)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_count_rows(db, NULL, "t", &count));
    D_TESTS_POSTGRES_CHECK(count == 2);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_truncate_table(db, NULL, "t"),
                            D_POSTGRES_SQLSTATE_FEATURE_NOT_SUPPORTED);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_table(db, NULL, "ref", 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_truncate_table(db, schema, "t"));
    D_TESTS_POSTGRES_OK(db, d_postgres_count_rows(db, schema, "t", &count));
    D_TESTS_POSTGRES_CHECK(count == 0);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "INSERT INTO t (a) VALUES ('z')"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT id FROM t") == 1);
    count = -1;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_count_rows(db, NULL, "no", &count),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_CHECK(count == -1);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_truncate_table(db, NULL, "no"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_table_listing
  d_postgres_list_tables.
  Tests the following:
  - the schema's tables, ordinary and partitioned, in byte order, without
    its views, sequences or the session's temporary tables
  - the temporary ones under "pg_temp"
  - a schema that does not exist lists none
*/
static void
d_tests_postgres_table_listing(
    void
)
{
    char                  schema[96] = "";
    struct pg_conn*       db         = d_tests_postgres_begin("tlist",
                                                              schema,
                                                              sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };

    printf("postgres_schema: table listing\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(
                                db,
                                "CREATE TABLE a (x int); CREATE TABLE \"B\" "
                                "(x int); CREATE TABLE _c (x int) PARTITION BY "
                                "LIST (x); CREATE VIEW v AS SELECT 1; "
                                "CREATE SEQUENCE s; CREATE TEMP TABLE tmp "
                                "(x int)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_tables(db, NULL, &names));
    D_TESTS_POSTGRES_CHECK( (names.count == 3u)                   &&
                            (strcmp(names.names[0], "B") == 0)    &&
                            (strcmp(names.names[1], "_c") == 0)   &&
                            (strcmp(names.names[2], "a") == 0)    &&
                            (names.names[3] == NULL) );
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_OK(db, d_postgres_list_tables(db, "pg_temp", &names));
    D_TESTS_POSTGRES_CHECK( (names.count == 1u) &&
                            (strcmp(names.names[0], "tmp") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_OK(db, d_postgres_list_tables(db, "no_such", &names));
    D_TESTS_POSTGRES_CHECK( (names.count == 0u) &&
                            (names.names) &&
                            (names.names[0] == NULL) );
    d_db_name_list_free(&names);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_null_schema
  What a NULL schema means: the current schema, named in every statement.
  Tests the following:
  - a temporary table that shadows one of the current schema's is never
    acted on through NULL, which reaches the current schema's
  - a table only further along the search path is not the current schema's
*/
static void
d_tests_postgres_null_schema(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("nullschema",
                                                        schema,
                                                        sizeof(schema));
    char            path[200]  = "";
    int64_t         count      = 0;

    printf("postgres_schema: the NULL schema\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(
                                db,
                                "CREATE TABLE shadow (a int); "
                                "INSERT INTO shadow VALUES (1); "
                                "CREATE TEMP TABLE shadow (a int)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_count_rows(db, NULL, "shadow", &count));
    D_TESTS_POSTGRES_CHECK(count == 1);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_table(db, NULL, "shadow", 0u));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "shadow"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, "pg_temp", "shadow"));
    snprintf(path, sizeof(path), "\"%s\", pg_catalog, public", schema);
    D_TESTS_POSTGRES_OK(db, d_postgres_set_search_path(db, path));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "pg_class"));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_count_rows(db, NULL, "pg_class",
                                                  &count),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_table_arguments
  The table functions' argument checks.
  Tests the following:
  - a NULL connection, a NULL or empty name, an empty schema, a NULL out
    argument and an unknown flag are D_DB_INVALID_ARGUMENT, and a _s answer
    then false
*/
static void
d_tests_postgres_table_arguments(
    void
)
{
    struct pg_conn*       db     = d_tests_postgres_connect(NULL);
    struct d_db_name_list names  = { 0u, NULL };
    bool                  exists = true;
    int64_t               count  = 0;

    printf("postgres_schema: table arguments\n");

    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_table_s(NULL, NULL, "t",
                                                       &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(!exists);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_table_s(db, "", "t", &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_table_s(db, NULL, "", &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, NULL));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_table_sql(db, NULL, "t",
                                                            NULL, 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_table_sql(db, NULL, "t",
                                                            "a int",
                                                            D_DB_IF_EXISTS),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_table(db, NULL, "t",
                                                      D_DB_IF_NOT_EXISTS),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_table(db, "", "t", 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_rename_table(db, NULL, "t", ""),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_truncate_table(NULL, NULL, "t"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_count_rows(db, NULL, "t", NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_count_rows(db, NULL, NULL, &count),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_tables(db, NULL, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_tables(db, "", &names),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_creation_arguments
  d_postgres_create_table's own argument checks.
  Tests the following:
  - no columns, a NULL array, an unknown flag, a NULL connection and an
    empty name are D_DB_INVALID_ARGUMENT, and nothing is created
*/
static void
d_tests_postgres_creation_arguments(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("cargs",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_schema: creation arguments\n");

    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table(db, NULL, "t",
                                                    d_tests_postgres_composite,
                                                    0u, 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table(db, NULL, "t", NULL, 1u,
                                                    0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table(db, NULL, "t",
                                                    d_tests_postgres_composite,
                                                    3u, D_POSTGRES_CASCADE),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table(NULL, NULL, "t",
                                                    d_tests_postgres_composite,
                                                    3u, 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table(db, NULL, "",
                                                    d_tests_postgres_composite,
                                                    3u, 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "t"));

    d_tests_postgres_end(db, schema);

    return;
}

//==============================================================================
// postgres_schema.h: described columns
//==============================================================================

/*
d_tests_postgres_described_types
  d_postgres_create_table, column by column.
  Tests the following:
  - every d_db_type, and a type_sql, declared as postgres_schema.h
    tabulates, as format_type reports it
*/
static void
d_tests_postgres_described_types(
    void
)
{
    char                    schema[96] = "";
    struct pg_conn*         db         = d_tests_postgres_begin("dtypes",
                                                                schema,
                                                                sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };
    const size_t            count      =
        sizeof(d_tests_postgres_every_type) /
        sizeof(d_tests_postgres_every_type[0]);

    printf("postgres_schema: described types\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_create_table(db, NULL, "every",
                                                    d_tests_postgres_every_type,
                                                    count, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_columns(db, NULL, "every",
                                                    &columns));
    D_TESTS_POSTGRES_CHECK(columns.count == count);

    for (size_t i = 0u; (i < count) && (i < columns.count); ++i)
    {
        const bool same =
            (strcmp(columns.columns[i].name,
                    d_tests_postgres_every_type[i].name) == 0) &&
            (strcmp(columns.columns[i].type,
                    d_tests_postgres_every_type_sql[i]) == 0);

        D_TESTS_POSTGRES_CHECK(same);

        if (!same)
        {
            printf("    column %s: %s, expected %s\n",
                   columns.columns[i].name,
                   columns.columns[i].type,
                   d_tests_postgres_every_type_sql[i]);
        }
    }

    d_db_column_list_free(&columns);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_check_attributes
  The described attributes of d_tests_postgres_every_type's id, flag and
name: an identity key with no default, a default as given, and NOT NULL.
*/
static void
d_tests_postgres_check_attributes(
    const struct d_db_column_list* _columns
)
{
    const struct d_db_column_info* const id   =
        d_tests_postgres_find_column(_columns, "id");
    const struct d_db_column_info* const flag =
        d_tests_postgres_find_column(_columns, "flag");
    const struct d_db_column_info* const name =
        d_tests_postgres_find_column(_columns, "name");

    D_TESTS_POSTGRES_CHECK( (id) && (id->not_null) && (!id->default_sql) &&
                            (id->primary_key == 1) );
    D_TESTS_POSTGRES_CHECK( (flag) && (flag->not_null) &&
                            (flag->default_sql) &&
                            (strcmp(flag->default_sql, "true") == 0) );
    D_TESTS_POSTGRES_CHECK( (name) && (name->not_null) &&
                            (name->primary_key == 0) );

    return;
}

/*
d_tests_postgres_described_attributes
  The attributes a described column carries.
  Tests the following:
  - NOT NULL and DEFAULT as given
  - AUTO_INCREMENT an identity column, NOT NULL, with no default to list,
    which still takes a value given by hand
  - UNIQUE, through an index the server names and the listing leaves out
*/
static void
d_tests_postgres_described_attributes(
    void
)
{
    char                    schema[96] = "";
    struct pg_conn*         db         = d_tests_postgres_begin("dattrs",
                                                                schema,
                                                                sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };
    struct d_db_name_list   names      = { 0u, NULL };
    const size_t            count      =
        sizeof(d_tests_postgres_every_type) /
        sizeof(d_tests_postgres_every_type[0]);

    printf("postgres_schema: described attributes\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_create_table(db, NULL, "every",
                                                    d_tests_postgres_every_type,
                                                    count, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_columns(db, NULL, "every",
                                                    &columns));
    d_tests_postgres_check_attributes(&columns);
    d_db_column_list_free(&columns);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "INSERT INTO every (id, name) "
                                            "VALUES (7, 'seven')"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db,
                                                "SELECT id FROM every") == 7);
    D_TESTS_POSTGRES_CHECK(d_postgres_has_index(db, NULL, "every_name_key"));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_indexes(db, NULL, "every",
                                                    &names));
    D_TESTS_POSTGRES_CHECK(names.count == 0u);
    d_db_name_list_free(&names);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_described_keys
  Primary keys from described columns.
  Tests the following:
  - a composite key, written as a constraint, its columns in the order the
    set gives them, each listed with its place
  - a column outside the key has place 0
  - key places order the key otherwise than the table; places that repeat,
    skip, number a column off the key, or leave a key column out are
    D_DB_INVALID_ARGUMENT, and make nothing
*/
static void
d_tests_postgres_described_keys(
    void
)
{
    char                    schema[96] = "";
    struct pg_conn*         db         = d_tests_postgres_begin("dkeys",
                                                                schema,
                                                                sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };

    printf("postgres_schema: described keys\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_create_table(db, NULL, "c",
                                                    d_tests_postgres_composite,
                                                    3u, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_columns(db, NULL, "c", &columns));
    D_TESTS_POSTGRES_CHECK(columns.count == 3u);

    const struct d_db_column_info* const label  =
        d_tests_postgres_find_column(&columns, "label");
    const struct d_db_column_info* const code   =
        d_tests_postgres_find_column(&columns, "code");
    const struct d_db_column_info* const region =
        d_tests_postgres_find_column(&columns, "region");

    D_TESTS_POSTGRES_CHECK( (label) && (label->primary_key == 0) );
    D_TESTS_POSTGRES_CHECK( (code) && (code->primary_key == 1) &&
                            (code->not_null) );
    D_TESTS_POSTGRES_CHECK( (region) && (region->primary_key == 2) );
    d_db_column_list_free(&columns);
    D_TESTS_POSTGRES_CHECK(d_postgres_has_index(db, NULL, "c_pkey"));


    // a key in the order its places give, not the table's
    struct d_db_column bad[3] =
    {
        d_tests_postgres_ordered[0],
        d_tests_postgres_ordered[1],
        d_tests_postgres_ordered[2]
    };

    D_TESTS_POSTGRES_OK(db,
                        d_postgres_create_table(db,
                                                NULL,
                                                "ordered",
                                                d_tests_postgres_ordered,
                                                3u,
                                                0u));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_list_columns(db,
                                                NULL,
                                                "ordered",
                                                &columns));

    const struct d_db_column_info* const first  =
        d_tests_postgres_find_column(&columns, "a");
    const struct d_db_column_info* const second =
        d_tests_postgres_find_column(&columns, "b");

    D_TESTS_POSTGRES_CHECK( (first) && (first->primary_key == 2) );
    D_TESTS_POSTGRES_CHECK( (second) && (second->primary_key == 1) );
    d_db_column_list_free(&columns);

    // places that repeat, skip, number a column off the key, or leave one
    // key column out
    bad[1].key_place = 2u;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table(db,
                                                    NULL,
                                                    "unordered",
                                                    bad,
                                                    3u,
                                                    0u),
                            D_DB_INVALID_ARGUMENT);
    bad[1].key_place = 3u;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table(db,
                                                    NULL,
                                                    "unordered",
                                                    bad,
                                                    3u,
                                                    0u),
                            D_DB_INVALID_ARGUMENT);
    bad[1].key_place = 0u;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table(db,
                                                    NULL,
                                                    "unordered",
                                                    bad,
                                                    3u,
                                                    0u),
                            D_DB_INVALID_ARGUMENT);
    bad[1].key_place = 1u;
    bad[2].key_place = 3u;
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_table(db,
                                                    NULL,
                                                    "unordered",
                                                    bad,
                                                    3u,
                                                    0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "unordered"));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_described_refusals
  Column sets the layer refuses.
  Tests the following:
  - each of d_tests_postgres_refused is D_DB_INVALID_ARGUMENT, through
    create_table and, for a single column, add_column, and no table is made
*/
static void
d_tests_postgres_described_refusals(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("drefuse",
                                                        schema,
                                                        sizeof(schema));
    const size_t    count      = sizeof(d_tests_postgres_refused) /
                                 sizeof(d_tests_postgres_refused[0]);

    printf("postgres_schema: described refusals\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE base (k int)"));

    for (size_t i = 0u; i < count; ++i)
    {
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_create_table(
                                    db, NULL, "r",
                                    d_tests_postgres_refused[i],
                                    d_tests_postgres_refused_counts[i], 0u),
                                D_DB_INVALID_ARGUMENT);

        if (d_tests_postgres_refused_counts[i] == 1u)
        {
            D_TESTS_POSTGRES_RESULT(db,
                                    d_postgres_add_column(
                                        db, NULL, "base",
                                        &d_tests_postgres_refused[i][0]),
                                    D_DB_INVALID_ARGUMENT);
        }
    }

    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "r"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_column(db, NULL, "base", "a"));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_check_defaults
  The defaults d_tests_postgres_defaults_sql's columns report: a string as
the server keeps it, a function call, and none for no default, a default of
NULL, and an identity column, the last two NOT NULL.
*/
static void
d_tests_postgres_check_defaults(
    const struct d_db_column_list* _columns
)
{
    const struct d_db_column_info* const a = d_tests_postgres_find_column(
                                                 _columns, "a");
    const struct d_db_column_info* const c = d_tests_postgres_find_column(
                                                 _columns, "c");
    const struct d_db_column_info* const f = d_tests_postgres_find_column(
                                                 _columns, "f");
    const struct d_db_column_info* const g = d_tests_postgres_find_column(
                                                 _columns, "g");
    const struct d_db_column_info* const t = d_tests_postgres_find_column(
                                                 _columns, "t");

    D_TESTS_POSTGRES_CHECK( (a) && (a->default_sql) &&
                            (strstr(a->default_sql, "'abc'") ==
                             a->default_sql) );
    D_TESTS_POSTGRES_CHECK( (c) && (c->default_sql) &&
                            (strcmp(c->default_sql, "now()") == 0) );
    D_TESTS_POSTGRES_CHECK( (f) && (!f->default_sql) );
    D_TESTS_POSTGRES_CHECK( (g) && (!g->default_sql) && (g->not_null) );
    D_TESTS_POSTGRES_CHECK( (t) && (!t->default_sql) && (t->not_null) );

    return;
}

/*
d_tests_postgres_defaults
  Column defaults, as list_columns reports them.
  Tests the following:
  - a default as the server keeps it, as SQL
  - none, a default of NULL, and an identity column report NULL
  - a generated column, from 12, reports NULL: its expression is not a
    default
*/
static void
d_tests_postgres_defaults(
    void
)
{
    char                    schema[96] = "";
    struct pg_conn*         db         = d_tests_postgres_begin("defaults",
                                                                schema,
                                                                sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };

    printf("postgres_schema: defaults\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_create_table_sql(
                                db, NULL, "d",
                                d_tests_postgres_defaults_sql, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_columns(db, NULL, "d", &columns));
    d_tests_postgres_check_defaults(&columns);
    d_db_column_list_free(&columns);

    // a generated column, where the server has them
    if (d_tests_postgres_at_least(db, 12))
    {
        D_TESTS_POSTGRES_OK(db, d_postgres_add_column_sql(
                                    db, NULL, "d",
                                    "twice integer GENERATED ALWAYS AS "
                                    "(b * 2) STORED"));
        D_TESTS_POSTGRES_OK(db, d_postgres_list_columns(db, NULL, "d",
                                                        &columns));
        D_TESTS_POSTGRES_CHECK(
            (d_tests_postgres_find_column(&columns, "twice")) &&
            (!d_tests_postgres_find_column(&columns, "twice")->default_sql) &&
            (d_tests_postgres_find_column(&columns, "twice")->generated) &&
            (!d_tests_postgres_find_column(&columns, "b")->generated));
        d_db_column_list_free(&columns);
    }

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_default_values
  What a reported default means.
  Tests the following:
  - each default of d_tests_postgres_defaults_sql, run as SQL, reads as the
    value it was declared with: quotes, backslashes, bytes and all
*/
static void
d_tests_postgres_default_values(
    void
)
{
    char                    schema[96] = "";
    struct pg_conn*         db         = d_tests_postgres_begin("dreads",
                                                                schema,
                                                                sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };
    const size_t            count      =
        sizeof(d_tests_postgres_default_reads) /
        sizeof(d_tests_postgres_default_reads[0]);
    char                    sql[256]   = "";

    printf("postgres_schema: what defaults read as\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_create_table_sql(
                                db, NULL, "d",
                                d_tests_postgres_defaults_sql, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_columns(db, NULL, "d", &columns));

    for (size_t i = 0u; i < count; ++i)
    {
        const char* const* const       read   =
            d_tests_postgres_default_reads[i];
        const struct d_db_column_info* column =
            d_tests_postgres_find_column(&columns, read[0]);

        D_TESTS_POSTGRES_CHECK( (column) && (column->default_sql) );

        // the default, filled into its query
        if ( (column) &&
             (column->default_sql) )
        {
            snprintf(sql, sizeof(sql), read[1], column->default_sql);
            D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db, sql,
                                                            read[2]));
        }
    }

    d_db_column_list_free(&columns);

    d_tests_postgres_end(db, schema);

    return;
}

//==============================================================================
// postgres_schema.h: columns
//==============================================================================

/*
d_tests_postgres_column_existence
  d_postgres_has_column and its _s form.
  Tests the following:
  - a table's columns and a view's are there, by exact name
  - the system's own columns are not, nor any of a table that is not there
*/
static void
d_tests_postgres_column_existence(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("cexist",
                                                        schema,
                                                        sizeof(schema));
    bool            exists     = false;

    printf("postgres_schema: column existence\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "CREATE TABLE t (a int, \"B\" int); "
                                        "CREATE VIEW v AS SELECT a FROM t"));
    D_TESTS_POSTGRES_OK(db, d_postgres_has_column_s(db, NULL, "t", "a",
                                                    &exists));
    D_TESTS_POSTGRES_CHECK(exists);
    D_TESTS_POSTGRES_CHECK(d_postgres_has_column(db, schema, "t", "B"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_column(db, NULL, "t", "b"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_column(db, NULL, "v", "a"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_column(db, NULL, "t", "ctid"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_column(db, NULL, "t", "xmin"));
    D_TESTS_POSTGRES_OK(db, d_postgres_has_column_s(db, NULL, "no", "a",
                                                    &exists));
    D_TESTS_POSTGRES_CHECK(!exists);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_column_additions
  d_postgres_add_column and d_postgres_add_column_sql.
  Tests the following:
  - a described column and one written as SQL are appended
  - an identity primary key can be added to a table without a key
  - a name taken is 42701; a table that is not there, 42P01
*/
static void
d_tests_postgres_column_additions(
    void
)
{
    char                     schema[96] = "";
    struct pg_conn*          db         = d_tests_postgres_begin(
                                              "cadd",
                                              schema,
                                              sizeof(schema));
    const struct d_db_column note       =
    {
        "note", D_DB_TYPE_STRING, D_DB_COLUMN_NOT_NULL, "''", 40u, 0u, NULL, 0u
    };
    const struct d_db_column key        =
    {
        "id", D_DB_TYPE_BIG_INTEGER,
        D_DB_COLUMN_PRIMARY_KEY | D_DB_COLUMN_AUTO_INCREMENT, NULL, 0u, 0u,
        NULL, 0u
    };

    printf("postgres_schema: column additions\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (a int); "
                                            "INSERT INTO t VALUES (1), (2)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_add_column(db, NULL, "t", &note));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_column(db, NULL, "t", "note"));
    D_TESTS_POSTGRES_OK(db, d_postgres_add_column_sql(db, schema, "t",
                                                      "extra jsonb"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_column(db, NULL, "t", "extra"));
    D_TESTS_POSTGRES_OK(db, d_postgres_add_column(db, NULL, "t", &key));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db, "SELECT max(id) FROM t") == 2);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_add_column(db, NULL, "t", &note),
                            D_POSTGRES_SQLSTATE_DUPLICATE_COLUMN);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_add_column_sql(db, NULL, "no",
                                                          "a int"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_column_removals
  d_postgres_rename_column and d_postgres_drop_column.
  Tests the following:
  - a renamed column is there under its new name; a column that is not
    there is 42703; a new name taken, 42701
  - a dropped column is gone; one a view uses is 2BP01; a table's last
    column can go, leaving a table without columns
*/
static void
d_tests_postgres_column_removals(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("cremove",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_schema: column renames and drops\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "CREATE TABLE t (a int, b int); "
                                        "CREATE TABLE u (x int); "
                                        "CREATE VIEW v AS SELECT b FROM t"));
    D_TESTS_POSTGRES_OK(db, d_postgres_rename_column(db, NULL, "t", "a",
                                                     "c"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_column(db, NULL, "t", "c"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_column(db, NULL, "t", "a"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_rename_column(db, NULL, "t", "a",
                                                         "d"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_COLUMN);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_rename_column(db, NULL, "t", "c",
                                                         "b"),
                            D_POSTGRES_SQLSTATE_DUPLICATE_COLUMN);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_column(db, schema, "t", "c"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_column(db, NULL, "t", "c"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_column(db, NULL, "t", "c"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_COLUMN);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_column(db, NULL, "t", "b"),
                            D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_column(db, NULL, "u", "x"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL, "u"));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_column_listing
  d_postgres_list_columns.
  Tests the following:
  - a table's columns in their order, its dropped ones left out
  - a view's columns
  - a table without columns lists none, NULL; one that is not there is
    D_DB_NOT_FOUND, writing nothing
*/
static void
d_tests_postgres_column_listing(
    void
)
{
    char                    schema[96] = "";
    struct pg_conn*         db         = d_tests_postgres_begin("clist",
                                                                schema,
                                                                sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };

    printf("postgres_schema: column listing\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(
                                db,
                                "CREATE TABLE t (z int, gone int, a text); "
                                "ALTER TABLE t DROP COLUMN gone; "
                                "CREATE VIEW v AS SELECT a, z FROM t; "
                                "CREATE TABLE empty ()"));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_columns(db, NULL, "t", &columns));
    D_TESTS_POSTGRES_CHECK( (columns.count == 2u)                         &&
                            (strcmp(columns.columns[0].name, "z") == 0)  &&
                            (strcmp(columns.columns[1].name, "a") == 0)  &&
                            (strcmp(columns.columns[1].type, "text") == 0) );
    d_db_column_list_free(&columns);
    D_TESTS_POSTGRES_OK(db, d_postgres_list_columns(db, schema, "v",
                                                    &columns));
    D_TESTS_POSTGRES_CHECK( (columns.count == 2u) &&
                            (strcmp(columns.columns[0].name, "a") == 0) );
    d_db_column_list_free(&columns);
    D_TESTS_POSTGRES_OK(db, d_postgres_list_columns(db, NULL, "empty",
                                                    &columns));
    D_TESTS_POSTGRES_CHECK( (columns.count == 0u) &&
                            (columns.columns == NULL) );
    columns.count = 77u;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_columns(db, NULL, "no",
                                                        &columns),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(columns.count == 77u);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_column_arguments
  The column functions' argument checks.
  Tests the following:
  - a NULL connection, a NULL or empty name, an empty schema and a NULL
    column or list are D_DB_INVALID_ARGUMENT, and a _s answer then false
*/
static void
d_tests_postgres_column_arguments(
    void
)
{
    struct pg_conn*          db      = d_tests_postgres_connect(NULL);
    struct d_db_column_list  columns = { 0u, NULL };
    bool                     exists  = true;
    const struct d_db_column column  =
    {
        "a", D_DB_TYPE_INTEGER, 0u, NULL, 0u, 0u, NULL, 0u
    };

    printf("postgres_schema: column arguments\n");

    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_column_s(db, NULL, "t", NULL,
                                                        &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(!exists);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_column_s(db, NULL, NULL, "a",
                                                        &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_add_column(db, NULL, "t", NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_add_column(NULL, NULL, "t",
                                                      &column),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_add_column_sql(db, "", "t", "a int"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_add_column_sql(db, NULL, "t", NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_rename_column(db, NULL, "t", "a",
                                                         NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_column(db, NULL, "", "a"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_columns(db, NULL, "t", NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_columns(db, NULL, NULL,
                                                        &columns),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(db);

    return;
}

//==============================================================================
// postgres_schema.h: indexes
//==============================================================================

/*
d_tests_postgres_indexes
  d_postgres_has_index, create_index and list_indexes.
  Tests the following:
  - an index is there, and listed, by schema or by its table, in byte order,
    without those its constraints own, which are there all the same
  - a name taken is 42P07, a success under D_DB_IF_NOT_EXISTS
  - D_DB_UNIQUE over duplicates is 23505
*/
static void
d_tests_postgres_indexes(
    void
)
{
    char                  schema[96] = "";
    struct pg_conn*       db         = d_tests_postgres_begin("indexes",
                                                              schema,
                                                              sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };

    printf("postgres_schema: indexes\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(
                                db,
                                "CREATE TABLE t (id int PRIMARY KEY, a int, "
                                "b text UNIQUE); CREATE TABLE u (x int); "
                                "INSERT INTO t VALUES (1, 5, 'p'), "
                                "(2, 5, 'q')"));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_index(db, NULL, "t_a", "t", "a",
                                                    0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_index(db, schema, "U_x", "u",
                                                    "x", 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_index(db, NULL, "t_a"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_index(db, NULL, "t_pkey"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_index(db, NULL, "t"));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_indexes(db, NULL, NULL, &names));
    D_TESTS_POSTGRES_CHECK( (names.count == 2u)                   &&
                            (strcmp(names.names[0], "U_x") == 0)  &&
                            (strcmp(names.names[1], "t_a") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_OK(db, d_postgres_list_indexes(db, NULL, "u", &names));
    D_TESTS_POSTGRES_CHECK( (names.count == 1u) &&
                            (strcmp(names.names[0], "U_x") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_index(db, NULL, "t_a", "u",
                                                        "x", 0u),
                            D_POSTGRES_SQLSTATE_DUPLICATE_TABLE);
    D_TESTS_POSTGRES_OK(db, d_postgres_create_index(db, NULL, "t_a", "u", "x",
                                                    D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_index(db, NULL, "t_ua", "t",
                                                        "a", D_DB_UNIQUE),
                            D_POSTGRES_SQLSTATE_UNIQUE_VIOLATION);
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_index(db, NULL, "t_ua"));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_index_concurrently
  D_POSTGRES_CONCURRENTLY on table t: an index built outside a transaction,
and 25001 inside one.
*/
static void
d_tests_postgres_index_concurrently(
    struct pg_conn* _db
)
{
    D_TESTS_POSTGRES_OK(_db, d_postgres_create_index(_db, NULL, "t_c", "t",
                                                     "id",
                                                     D_POSTGRES_CONCURRENTLY));
    D_TESTS_POSTGRES_OK(_db, d_postgres_begin_transaction(
                                 _db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_RESULT(_db, d_postgres_create_index(
                                     _db, NULL, "t_c2", "t", "id",
                                     D_POSTGRES_CONCURRENTLY),
                            D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION);
    D_TESTS_POSTGRES_OK(_db, d_postgres_rollback(_db));

    return;
}

/*
d_tests_postgres_index_forms
  The forms an index's key may take.
  Tests the following:
  - an expression, put in parentheses, enforced under D_DB_UNIQUE
  - the rest of the statement, as written: a partial index, a method, and
    INCLUDE where the server has it
  - D_POSTGRES_CONCURRENTLY outside a transaction, and 25001 inside one
*/
static void
d_tests_postgres_index_forms(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("iforms",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_schema: index forms\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (id int, e text, "
                                            "gone boolean, doc jsonb); "
                                            "INSERT INTO t VALUES "
                                            "(1, 'A@x', false, '{}')"));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_index(db, NULL, "t_e", "t",
                                                    "lower(e)", D_DB_UNIQUE));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(db,
                                                "INSERT INTO t (e) "
                                                "VALUES ('a@X')"),
                            D_POSTGRES_SQLSTATE_UNIQUE_VIOLATION);
    D_TESTS_POSTGRES_OK(db, d_postgres_create_index(
                                db, NULL, "t_live", "t",
                                "  (id) WHERE NOT gone", 0u));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db,
                             "SELECT count(*) FROM pg_index WHERE indexrelid "
                             "= 't_live'::regclass AND indpred IS NOT NULL")
        == 1);
    D_TESTS_POSTGRES_OK(db, d_postgres_create_index(db, NULL, "t_doc", "t",
                                                    "USING gin (doc)", 0u));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_text_is(db,
                                 "SELECT amname FROM pg_class c JOIN pg_am a "
                                 "ON a.oid = c.relam WHERE relname = 't_doc'",
                                 "gin"));
    D_TESTS_POSTGRES_CHECK(
        (d_postgres_create_index(db, NULL, "t_cov", "t",
                                 "(id) INCLUDE (e)", 0u) == 0) ==
        d_postgres_supports(db, D_POSTGRES_FEATURE_COVERING_INDEXES));
    d_tests_postgres_index_concurrently(db);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_index_changes
  d_postgres_drop_index.
  Tests the following:
  - a dropped index is gone; one that is not there is 42704, a success under
    D_DB_IF_EXISTS
  - D_POSTGRES_CONCURRENTLY drops one without blocking its table
  - an index a constraint owns is refused, 2BP01
*/
static void
d_tests_postgres_index_changes(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("ichanges",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_schema: index drops\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (id int PRIMARY "
                                            "KEY, a int); CREATE INDEX t_a "
                                            "ON t (a); CREATE INDEX t_b ON t "
                                            "(a, id)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_index(db, NULL, "t_a", 0u));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_index(db, NULL, "t_a"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_index(db, NULL, "t_a", 0u),
                            D_POSTGRES_SQLSTATE_UNDEFINED_OBJECT);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_index(db, NULL, "t_a",
                                                  D_DB_IF_EXISTS));
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_index(db, schema, "t_b",
                                                  D_POSTGRES_CONCURRENTLY |
                                                  D_DB_IF_EXISTS));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_index(db, NULL, "t_b"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_index(db, NULL, "t_pkey", 0u),
                            D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST);
    D_TESTS_POSTGRES_CHECK(d_postgres_has_index(db, NULL, "t_pkey"));

    d_tests_postgres_end(db, schema);

    return;
}

//==============================================================================
// postgres_schema.h: views
//==============================================================================

/*
d_tests_postgres_views
  d_postgres_has_view, create_view and list_views.
  Tests the following:
  - a view is there and listed, in byte order; a table is not a view
  - a name taken is 42P07; a success under D_DB_IF_NOT_EXISTS, by a table's
    name too, which is left as it was
*/
static void
d_tests_postgres_views(
    void
)
{
    char                  schema[96] = "";
    struct pg_conn*       db         = d_tests_postgres_begin("views",
                                                              schema,
                                                              sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };

    printf("postgres_schema: views\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (a int); "
                                            "INSERT INTO t VALUES (1)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_view(db, NULL, "v",
                                                   "SELECT a FROM t", 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_view(db, schema, "V2",
                                                   "SELECT 2 AS b", 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_view(db, NULL, "v"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_view(db, NULL, "t"));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_views(db, NULL, &names));
    D_TESTS_POSTGRES_CHECK( (names.count == 2u)                  &&
                            (strcmp(names.names[0], "V2") == 0)  &&
                            (strcmp(names.names[1], "v") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_view(db, NULL, "v",
                                                       "SELECT 1", 0u),
                            D_POSTGRES_SQLSTATE_DUPLICATE_TABLE);
    D_TESTS_POSTGRES_OK(db, d_postgres_create_view(db, NULL, "v", "SELECT 1",
                                                   D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_view(db, NULL, "t", "SELECT 1",
                                                   D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL, "t"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT a FROM v") == 1);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_view_settling
  D_DB_IF_NOT_EXISTS on a view inside a transaction, which the layer settles
under a savepoint.
  Tests the following:
  - a view that is there is a success, and the transaction is not failed:
    its other work commits
  - a view that is not there is created inside the transaction, and rolled
    back with it
  - any other failure leaves the transaction failed, as a failure does
*/
static void
d_tests_postgres_view_settling(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("vsettle",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_schema: views settled in transactions\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE t (a int); "
                                                "CREATE VIEW v AS SELECT 1"));
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "INSERT INTO t VALUES (1)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_view(db, NULL, "v", "SELECT 2",
                                                   D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_view(db, NULL, "w", "SELECT 3",
                                                   D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_view(db, NULL, "w"));
    D_TESTS_POSTGRES_OK(db, d_postgres_commit(db));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db,
                                                "SELECT count(*) FROM t") == 1);
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_view(db, NULL, "x", "SELECT 4",
                                                   D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_view(db, NULL, "y",
                                                       "SELECT nope",
                                                       D_DB_IF_NOT_EXISTS),
                            D_POSTGRES_SQLSTATE_UNDEFINED_COLUMN);
    D_TESTS_POSTGRES_CHECK(d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_view(db, NULL, "x"));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_view_changes
  D_POSTGRES_REPLACE and d_postgres_drop_view.
  Tests the following:
  - a view replaced keeps its columns and gains one after them; both flags
    at once are D_DB_INVALID_ARGUMENT
  - a dropped view is gone; one that is not there is 42P01, a success under
    D_DB_IF_EXISTS; one another view uses is 2BP01, dropped with it under
    D_POSTGRES_CASCADE
*/
static void
d_tests_postgres_view_changes(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("vchanges",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_schema: view replacements and drops\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_create_view(db, NULL, "v",
                                                   "SELECT 1 AS a", 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_view(db, NULL, "v",
                                                   "SELECT 1 AS a, 2 AS b",
                                                   D_POSTGRES_REPLACE));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_column(db, NULL, "v", "b"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_view(db, NULL, "v",
                                                       "SELECT 1 AS a",
                                                       D_POSTGRES_REPLACE |
                                                       D_DB_IF_NOT_EXISTS),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_OK(db, d_postgres_create_view(db, NULL, "w",
                                                   "SELECT a FROM v", 0u));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_view(db, NULL, "v", 0u),
                            D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_view(db, schema, "v",
                                                 D_POSTGRES_CASCADE));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_view(db, NULL, "v"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_view(db, NULL, "w"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_view(db, NULL, "v", 0u),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_view(db, NULL, "v",
                                                 D_DB_IF_EXISTS));

    d_tests_postgres_end(db, schema);

    return;
}

//==============================================================================
// postgres_schema.h: triggers
//==============================================================================

/*
d_tests_postgres_triggers
  d_postgres_has_trigger, create_trigger and list_triggers.
  Tests the following:
  - a trigger is there on its table, and not on another
  - triggers are listed by schema or by table, in byte order, a name once
    for each table that has a trigger of that name, the server's own for
    foreign keys left out
  - a name taken on the table is 42710
*/
static void
d_tests_postgres_triggers(
    void
)
{
    char                  schema[96] = "";
    struct pg_conn*       db         = d_tests_postgres_begin("triggers",
                                                              schema,
                                                              sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };
    char                  on_t[200]  = "";
    char                  on_u[200]  = "";

    printf("postgres_schema: triggers\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    snprintf(on_t, sizeof(on_t), "BEFORE INSERT ON \"%s\".t FOR EACH ROW "
             "EXECUTE PROCEDURE t_audit()", schema);
    snprintf(on_u, sizeof(on_u), "AFTER UPDATE ON u FOR EACH ROW "
             "EXECUTE PROCEDURE t_audit()");
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (id int PRIMARY "
                                            "KEY); CREATE TABLE u (id int "
                                            "REFERENCES t)"));
    D_TESTS_POSTGRES_OK(db, d_tests_postgres_function(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_trigger(db, "audit", on_t, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_trigger(db, "audit", on_u, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_trigger(db, "Also", on_u, 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_trigger(db, NULL, "audit", "t"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_trigger(db, NULL, "Also", "t"));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_triggers(db, NULL, NULL, &names));
    D_TESTS_POSTGRES_CHECK( (names.count == 3u) &&
                            (strcmp(names.names[0], "Also") == 0) &&
                            (d_tests_postgres_count_name(&names,
                                                         "audit") == 2u) &&
                            (d_tests_postgres_sorted(&names)) );
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_OK(db, d_postgres_list_triggers(db, schema, "t", &names));
    D_TESTS_POSTGRES_CHECK( (names.count == 1u) &&
                            (strcmp(names.names[0], "audit") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_trigger(db, "audit", on_t,
                                                          0u),
                            D_POSTGRES_SQLSTATE_DUPLICATE_OBJECT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_trigger_changes
  Settling, replacing and dropping triggers.
  Tests the following:
  - D_DB_IF_NOT_EXISTS makes a name taken a success, inside a transaction
    too, which stays usable
  - D_POSTGRES_REPLACE, from 14, replaces one; before 14 it is
    D_DB_UNSUPPORTED; both flags are D_DB_INVALID_ARGUMENT
  - a dropped trigger is gone; one that is not there is 42704, a success
    under D_DB_IF_EXISTS, as is one on a table that is not there
*/
static void
d_tests_postgres_trigger_changes(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("tchanges",
                                                        schema,
                                                        sizeof(schema));
    const char*     def        = "BEFORE INSERT ON t FOR EACH ROW "
                                 "EXECUTE PROCEDURE t_audit()";

    printf("postgres_schema: trigger changes\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE t (id int)"));
    D_TESTS_POSTGRES_OK(db, d_tests_postgres_function(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_trigger(db, "tr", def, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_trigger(db, "tr", def,
                                                      D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_commit(db));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_trigger(db, "tr", def,
                                                      D_POSTGRES_REPLACE),
                            (d_tests_postgres_at_least(db, 14))
                                ? 0
                                : D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_create_trigger(db, "tr", def,
                                                      D_POSTGRES_REPLACE |
                                                      D_DB_IF_NOT_EXISTS),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_trigger(db, NULL, "tr", "t", 0u));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_trigger(db, NULL, "tr", "t"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_trigger(db, schema, "tr", "t",
                                                        0u),
                            D_POSTGRES_SQLSTATE_UNDEFINED_OBJECT);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_trigger(db, NULL, "tr", "t",
                                                    D_DB_IF_EXISTS));
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_trigger(db, NULL, "tr", "no",
                                                    D_DB_IF_EXISTS));

    d_tests_postgres_end(db, schema);

    return;
}

//==============================================================================
// postgres_schema.h: sequences
//==============================================================================

/*
d_tests_postgres_sequences
  Sequences: has, create, drop, list.
  Tests the following:
  - a sequence is there, and listed in byte order with those identity
    columns use; a name taken is 42P07, a success under D_DB_IF_NOT_EXISTS
  - a dropped one is gone; one that is not there is 42P01, a success under
    D_DB_IF_EXISTS; one an identity column owns is refused, 2BP01
*/
static void
d_tests_postgres_sequences(
    void
)
{
    char                  schema[96] = "";
    struct pg_conn*       db         = d_tests_postgres_begin("sequences",
                                                              schema,
                                                              sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };

    printf("postgres_schema: sequences\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (id int GENERATED "
                                            "BY DEFAULT AS IDENTITY)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_sequence(db, NULL, "s", 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_sequence(db, NULL, "s"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_sequence(db, schema, "t_id_seq"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_sequence(db, NULL, "t"));
    D_TESTS_POSTGRES_OK(db, d_postgres_list_sequences(db, NULL, &names));
    D_TESTS_POSTGRES_CHECK( (names.count == 2u)                       &&
                            (strcmp(names.names[0], "s") == 0)        &&
                            (strcmp(names.names[1], "t_id_seq") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_sequence(db, NULL, "s", 0u),
                            D_POSTGRES_SQLSTATE_DUPLICATE_TABLE);
    D_TESTS_POSTGRES_OK(db, d_postgres_create_sequence(db, NULL, "s",
                                                       D_DB_IF_NOT_EXISTS));
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_sequence(db, NULL, "s", 0u));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_sequence(db, NULL, "s"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_sequence(db, NULL, "s", 0u),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_sequence(db, NULL, "s",
                                                     D_DB_IF_EXISTS));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_sequence(db, NULL, "t_id_seq",
                                                         0u),
                            D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_sequence_values
  d_postgres_next_value.
  Tests the following:
  - each call hands out the next value, never again, even when the
    transaction that took it rolls back
  - a sequence that is not there is D_DB_NOT_FOUND, writing nothing, and
    leaves a transaction usable
*/
static void
d_tests_postgres_sequence_values(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("seqvalues",
                                                        schema,
                                                        sizeof(schema));
    int64_t         value      = 0;

    printf("postgres_schema: sequence values\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_create_sequence(db, NULL, "s", 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_next_value(db, NULL, "s", &value));
    D_TESTS_POSTGRES_CHECK(value == 1);
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_next_value(db, schema, "s", &value));
    D_TESTS_POSTGRES_CHECK(value == 2);
    value = 99;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_value(db, NULL, "no", &value),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(value == 99);
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_next_value(db, NULL, "s", &value));
    D_TESTS_POSTGRES_CHECK(value == 3);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_last_values
  d_postgres_last_value.
  Tests the following:
  - before the session's first nextval() it is D_DB_NOT_FOUND, writing
    nothing, and inside a transaction leaves it usable
  - after a nextval(), its value; after an INSERT into an identity column,
    the id, inside a transaction too, which stays open
  - in a failed transaction it is 25P02
  - NULL arguments
*/
static void
d_tests_postgres_last_values(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("lastvalues",
                                                        schema,
                                                        sizeof(schema));
    int64_t         value      = 99;

    printf("postgres_schema: last values\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_RESULT(db, d_postgres_last_value(db, &value),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(value == 99);
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_last_value(db, &value),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 5") == 5);
    D_TESTS_POSTGRES_OK(db, d_postgres_commit(db));

    // after values were handed out
    D_TESTS_POSTGRES_OK(db, d_postgres_create_sequence(db, NULL, "s", 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_next_value(db, NULL, "s", &value));
    D_TESTS_POSTGRES_OK(db, d_postgres_last_value(db, &value));
    D_TESTS_POSTGRES_CHECK(value == 1);
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "CREATE TABLE t (id bigint GENERATED "
                                        "BY DEFAULT AS IDENTITY, a int)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "INSERT INTO t (a) VALUES (10), (20)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_last_value(db, &value));
    D_TESTS_POSTGRES_CHECK(value == 2);
    D_TESTS_POSTGRES_CHECK( (d_postgres_in_transaction(db)) &&
                            (!d_postgres_in_failed_transaction(db)) );
    D_TESTS_POSTGRES_OK(db, d_postgres_commit(db));

    // a failed transaction
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_CHECK(d_postgres_exec(db, "SELECT * FROM no_table") != 0);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_last_value(db, &value),
                            D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));

    D_TESTS_POSTGRES_RESULT(db, d_postgres_last_value(db, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_last_value(NULL, &value),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_object_arguments
  The index, view, trigger and sequence functions' argument checks.
  Tests the following:
  - a NULL connection, a NULL or empty name, an empty schema, a missing
    table, a NULL out argument and an unknown flag are
    D_DB_INVALID_ARGUMENT, and a _s answer then false
*/
static void
d_tests_postgres_object_arguments(
    void
)
{
    struct pg_conn*       db     = d_tests_postgres_connect(NULL);
    struct d_db_name_list names  = { 0u, NULL };
    bool                  exists = true;

    printf("postgres_schema: index, view, trigger and sequence arguments\n");

    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_index_s(db, NULL, NULL, &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(!exists);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_index(db, NULL, "i", "t",
                                                        NULL, 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_index(db, NULL, "i", "t",
                                                        "a", D_DB_IF_EXISTS),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_index(db, NULL, "i",
                                                      D_DB_UNIQUE),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_indexes(db, NULL, "", &names),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_view_s(NULL, NULL, "v",
                                                      &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_view(db, NULL, "v", NULL,
                                                       0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_view(db, NULL, "v",
                                                     D_POSTGRES_REPLACE),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_has_trigger_s(db, NULL, "tr", NULL,
                                                         &exists),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_trigger(db, "", "x", 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_drop_trigger(db, NULL, "tr", NULL,
                                                        0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_triggers(db, NULL, "", &names),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_sequence(db, NULL, "s",
                                                           D_DB_IF_EXISTS),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_value(db, NULL, "s", NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_list_sequences(db, NULL, NULL),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(db);

    return;
}

//==============================================================================
// postgres_transaction.h
//==============================================================================

/*
d_tests_postgres_transactions
  Beginning and ending transactions.
  Tests the following:
  - work committed is kept, work rolled back is not
  - in_transaction follows; beginning inside one is 25001, sending nothing
  - committing or rolling back with none open succeeds
  - transaction_status follows: idle, in a transaction, active while SQL
    sent has results to read or a COPY runs, and unknown for NULL
*/
static void
d_tests_postgres_transactions(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("txn",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_transaction: transactions\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE t (a int)"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_transaction(db));
    D_TESTS_POSTGRES_CHECK(d_postgres_transaction_status(db) ==
                           D_POSTGRES_STATE_IDLE);
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_CHECK(d_postgres_in_transaction(db));
    D_TESTS_POSTGRES_CHECK(d_postgres_transaction_status(db) ==
                           D_POSTGRES_STATE_IN_TRANSACTION);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_begin_transaction(
                                    db, D_POSTGRES_TRANSACTION_DEFAULT),
                            D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "INSERT INTO t VALUES (1)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_commit(db));
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_transaction(db));

    // SQL sent, until its last result is read; a COPY, until it ends
    {
        struct d_db_rows rows    = { 0u, 0u, NULL, NULL };
        int64_t          changes = 0;

        D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "SELECT 1"));
        D_TESTS_POSTGRES_CHECK(d_postgres_transaction_status(db) ==
                               D_POSTGRES_STATE_ACTIVE);
        D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
        d_db_rows_free(&rows);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_DB_NOT_FOUND);
        D_TESTS_POSTGRES_CHECK(d_postgres_transaction_status(db) ==
                               D_POSTGRES_STATE_IDLE);
        D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_start(db,
                                                         "COPY t FROM STDIN"));
        D_TESTS_POSTGRES_CHECK(d_postgres_transaction_status(db) ==
                               D_POSTGRES_STATE_ACTIVE);
        D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_end(db, NULL, &changes));
        D_TESTS_POSTGRES_CHECK(d_postgres_transaction_status(db) ==
                               D_POSTGRES_STATE_IDLE);
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "INSERT INTO t VALUES (2)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT sum(a) FROM t") ==
                           1);
    D_TESTS_POSTGRES_OK(db, d_postgres_commit(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_transaction(NULL));
    D_TESTS_POSTGRES_CHECK(d_postgres_transaction_status(NULL) ==
                           D_POSTGRES_STATE_UNKNOWN);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_commit(NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_rollback(NULL),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_failed_transactions
  A transaction a failure has left failed.
  Tests the following:
  - in_failed_transaction says so, as transaction_status does; statements
    are refused, 25P02
  - commit rolls it back and says 25P02, nothing kept; rollback ends one as
    a success
*/
static void
d_tests_postgres_failed_transactions(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("failed",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_transaction: failed transactions\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE t (a int)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "INSERT INTO t VALUES (1)"));
    D_TESTS_POSTGRES_CHECK(d_postgres_exec(db, "SELECT 1 / 0") != 0);
    D_TESTS_POSTGRES_CHECK(d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_CHECK(d_postgres_in_transaction(db));
    D_TESTS_POSTGRES_CHECK(d_postgres_transaction_status(db) ==
                           D_POSTGRES_STATE_IN_FAILED_TRANSACTION);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(db, "SELECT 1"),
                            D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_commit(db),
                            D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_transaction(db));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT count(*) FROM t") ==
                           0);
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_CHECK(d_postgres_exec(db, "SELECT 1 / 0") != 0);
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_failed_transaction(NULL));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_mode_level
  A transaction begun in a mode runs at the isolation level named; it is
rolled back after.
*/
static void
d_tests_postgres_mode_level(
    struct pg_conn*                  _db,
    enum d_postgres_transaction_mode _mode,
    const char*                      _level
)
{
    D_TESTS_POSTGRES_OK(_db, d_postgres_begin_transaction(_db, _mode));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               _db, "SHOW transaction_isolation", _level));
    D_TESTS_POSTGRES_OK(_db, d_postgres_rollback(_db));

    return;
}

/*
d_tests_postgres_transaction_modes
  The modes a transaction begins in.
  Tests the following:
  - DEFAULT is the session's: READ COMMITTED, read write
  - READ_ONLY refuses writes, 25006
  - REPEATABLE_READ and SERIALIZABLE set the level; DEFERRABLE is
    SERIALIZABLE, read only and deferrable; READ_COMMITTED is READ COMMITTED
    whatever the session's level
  - a mode outside the enumeration is D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_transaction_modes(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("modes",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_transaction: modes\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE t (a int)"));
    d_tests_postgres_mode_level(db,
                                D_POSTGRES_TRANSACTION_DEFAULT,
                                "read committed");
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_READ_ONLY));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(db,
                                                "INSERT INTO t VALUES (1)"),
                            d_postgres_result_from_sqlstate("25006"));
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    d_tests_postgres_mode_level(db,
                                D_POSTGRES_TRANSACTION_REPEATABLE_READ,
                                "repeatable read");
    d_tests_postgres_mode_level(db,
                                D_POSTGRES_TRANSACTION_SERIALIZABLE,
                                "serializable");
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFERRABLE));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               db, "SHOW transaction_deferrable", "on"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               db, "SHOW transaction_read_only", "on"));
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));

    // READ COMMITTED for the one transaction, the session's level apart
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "SET default_transaction_isolation "
                                        "TO 'serializable'"));
    d_tests_postgres_mode_level(db,
                                D_POSTGRES_TRANSACTION_DEFAULT,
                                "serializable");
    d_tests_postgres_mode_level(db,
                                D_POSTGRES_TRANSACTION_READ_COMMITTED,
                                "read committed");
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_exec(db,
                                        "RESET default_transaction_isolation"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_begin_transaction(
                                    db,
                                    (enum d_postgres_transaction_mode)9),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_begin_transaction(
                                    db,
                                    (enum d_postgres_transaction_mode)6),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_begin_transaction(
                                    NULL, D_POSTGRES_TRANSACTION_DEFAULT),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_savepoints
  Savepoints.
  Tests the following:
  - rolling back to one undoes what followed it, mends a failed
    transaction, and keeps it; releasing one keeps what it covered
  - outside a transaction, 25P01; one that is not there, 3B001
  - a NULL connection or an empty name is D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_savepoints(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("savepoints",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_transaction: savepoints\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE t (a int)"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_savepoint(db, "s"),
                            D_POSTGRES_SQLSTATE_NO_ACTIVE_SQL_TRANSACTION);
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "INSERT INTO t VALUES (1)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_savepoint(db, "First one"));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "INSERT INTO t VALUES (2)"));
    D_TESTS_POSTGRES_CHECK(d_postgres_exec(db, "SELECT 1 / 0") != 0);
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback_to_savepoint(db,
                                                             "First one"));
    D_TESTS_POSTGRES_CHECK(!d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "INSERT INTO t VALUES (4)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_release_savepoint(db, "First one"));
    D_TESTS_POSTGRES_RESULT(
        db,
        d_postgres_release_savepoint(db, "First one"),
        D_POSTGRES_SQLSTATE_INVALID_SAVEPOINT_SPECIFICATION);
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "INSERT INTO t VALUES (1)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_savepoint(db, "s"));
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "INSERT INTO t VALUES (2)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_release_savepoint(db, "s"));
    D_TESTS_POSTGRES_OK(db, d_postgres_commit(db));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT sum(a) FROM t") ==
                           3);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_create_savepoint(db, ""),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_rollback_to_savepoint(NULL, "s"),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_transactional_ddl
  Schema changes inside a transaction.
  Tests the following:
  - a table created and an index dropped inside a transaction are undone by
    its rollback
*/
static void
d_tests_postgres_transactional_ddl(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("ddl",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_transaction: schema changes roll back\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE t (a int); "
                                                "CREATE INDEX t_a ON t (a)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_create_table_sql(db, NULL, "u",
                                                        "a int", 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_drop_index(db, NULL, "t_a", 0u));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_table(db, NULL, "u"));
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_CHECK(!d_postgres_has_table(db, NULL, "u"));
    D_TESTS_POSTGRES_CHECK(d_postgres_has_index(db, NULL, "t_a"));

    d_tests_postgres_end(db, schema);

    return;
}

//==============================================================================
// postgres_settings.h
//==============================================================================

/*
d_tests_postgres_search_path
  The search path.
  Tests the following:
  - set and read back, as SQL writes it; the current schema follows it
  - a setting made in a transaction that rolls back is undone
  - NULL is D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_search_path(
    void
)
{
    char            schema[96]    = "";
    struct pg_conn* db            = d_tests_postgres_begin("searchpath",
                                                           schema,
                                                           sizeof(schema));
    char*           path          = NULL;
    char            expected[200] = "";

    printf("postgres_settings: the search path\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    snprintf(expected, sizeof(expected), "\"%s\", public", schema);
    D_TESTS_POSTGRES_OK(db, d_postgres_set_search_path(db, expected));
    D_TESTS_POSTGRES_OK(db, d_postgres_get_search_path(db, &path));
    D_TESTS_POSTGRES_CHECK( (path) && (strstr(path, schema) != NULL) &&
                            (strstr(path, "public") != NULL) );
    d_db_free(path);
    path = NULL;
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(db, d_postgres_set_search_path(db, "public"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SELECT current_schema()",
                                                    "public"));
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db,
                                                    "SELECT current_schema()",
                                                    schema));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_set_search_path(db, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_get_search_path(db, NULL),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_timeouts
  Statement and lock timeouts.
  Tests the following:
  - each set and read back in milliseconds, 0 for none
  - a value past INT_MAX is D_DB_INVALID_ARGUMENT
  - a statement that runs too long is cancelled, 57014; a lock waited on
    too long, 55P03
*/
static void
d_tests_postgres_timeouts(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("timeouts",
                                                        schema,
                                                        sizeof(schema));
    struct pg_conn* other      = d_tests_postgres_connect(NULL);
    unsigned        ms         = 1u;

    printf("postgres_settings: timeouts\n");

    // nothing more without a server
    if ( (!db) || (!other) )
    {
        D_TESTS_POSTGRES_CHECK( (db != NULL) && (other != NULL) );
        d_postgres_close(other);
        d_tests_postgres_end(db, schema);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_get_statement_timeout(db, &ms));
    D_TESTS_POSTGRES_CHECK(ms == 0u);
    D_TESTS_POSTGRES_OK(db, d_postgres_set_statement_timeout(db, 1500u));
    D_TESTS_POSTGRES_OK(db, d_postgres_get_statement_timeout(db, &ms));
    D_TESTS_POSTGRES_CHECK(ms == 1500u);
    D_TESTS_POSTGRES_OK(db, d_postgres_set_lock_timeout(db, 250u));
    D_TESTS_POSTGRES_OK(db, d_postgres_get_lock_timeout(db, &ms));
    D_TESTS_POSTGRES_CHECK(ms == 250u);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_set_lock_timeout(db, UINT_MAX),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(db, "SELECT pg_sleep(5)"),
                            D_POSTGRES_SQLSTATE_QUERY_CANCELED);
    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE t (a int)"));
    D_TESTS_POSTGRES_OK(other, d_postgres_begin_transaction(
                                   other, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(other, d_postgres_execf(other,
                                                "LOCK TABLE %T",
                                                schema,
                                                "t"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(db, "SELECT * FROM t"),
                            D_POSTGRES_SQLSTATE_LOCK_NOT_AVAILABLE);
    D_TESTS_POSTGRES_OK(other, d_postgres_rollback(other));
    D_TESTS_POSTGRES_OK(db, d_postgres_set_statement_timeout(db, 0u));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_get_lock_timeout(db, NULL),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(other);
    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_isolation
  The session's isolation level.
  Tests the following:
  - each level set and read back, and the next transaction begun in it
  - a level outside the enumeration is D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_isolation(
    void
)
{
    struct pg_conn*           db    = d_tests_postgres_connect(NULL);
    enum d_postgres_isolation level = D_POSTGRES_ISOLATION_READ_UNCOMMITTED;

    printf("postgres_settings: isolation\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    for (int i = (int)D_POSTGRES_ISOLATION_SERIALIZABLE; i >= 0; --i)
    {
        D_TESTS_POSTGRES_OK(db, d_postgres_set_isolation_level(
                                    db, (enum d_postgres_isolation)i));
        D_TESTS_POSTGRES_OK(db, d_postgres_get_isolation_level(db, &level));
        D_TESTS_POSTGRES_CHECK(level == (enum d_postgres_isolation)i);
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_set_isolation_level(
                                db, D_POSTGRES_ISOLATION_REPEATABLE_READ));
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               db, "SHOW transaction_isolation",
                               "repeatable read"));
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_set_isolation_level(
                                    db, (enum d_postgres_isolation)7),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_get_isolation_level(db, NULL),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_encoding_and_zone
  The client encoding and the time zone.
  Tests the following:
  - each set and read back from libpq's copy, which the server keeps current
  - values quoted under another encoding still reach the server whole
  - a value the server does not know is 22023, and changes nothing
*/
static void
d_tests_postgres_encoding_and_zone(
    void
)
{
    struct pg_conn* db   = d_tests_postgres_connect(NULL);
    char*           text = NULL;

    printf("postgres_settings: encoding and time zone\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_get_client_encoding(db, &text));
    D_TESTS_POSTGRES_CHECK( (text) && (strcmp(text, "UTF8") == 0) );
    d_db_free(text);
    D_TESTS_POSTGRES_OK(db, d_postgres_set_client_encoding(db, "LATIN1"));
    D_TESTS_POSTGRES_OK(db, d_postgres_get_client_encoding(db, &text));
    D_TESTS_POSTGRES_CHECK( (text) && (strcmp(text, "LATIN1") == 0) );
    d_db_free(text);
    D_TESTS_POSTGRES_OK(db, d_postgres_query_textf(db, &text,
                                                   "SELECT length(%Q)",
                                                   "caf\xe9"));
    D_TESTS_POSTGRES_CHECK( (text) && (strcmp(text, "4") == 0) );
    d_db_free(text);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_set_client_encoding(db, "NOPE"),
                            D_POSTGRES_SQLSTATE_INVALID_PARAMETER_VALUE);
    D_TESTS_POSTGRES_OK(db, d_postgres_set_client_encoding(db, "UTF8"));
    D_TESTS_POSTGRES_OK(db, d_postgres_set_time_zone(db, "Europe/Berlin"));
    D_TESTS_POSTGRES_OK(db, d_postgres_get_time_zone(db, &text));
    D_TESTS_POSTGRES_CHECK( (text) && (strcmp(text, "Europe/Berlin") == 0) );
    d_db_free(text);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_set_time_zone(db, "Nowhere/Land"),
                            D_POSTGRES_SQLSTATE_INVALID_PARAMETER_VALUE);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(db, "SHOW TimeZone",
                                                    "Europe/Berlin"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_set_time_zone(db, ""),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_get_client_encoding(NULL, &text),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(db);

    return;
}

/*
d_tests_postgres_parameters
  Any parameter, by name.
  Tests the following:
  - a server parameter and an extension's own set and read back
  - an unknown name is 42704; a value the server refuses, 22023
  - a name holding a quote is quoted, never SQL
  - a NULL argument or an empty name is D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_parameters(
    void
)
{
    struct pg_conn* db    = d_tests_postgres_connect(NULL);
    char*           value = NULL;

    printf("postgres_settings: parameters\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_set_parameter(db, "work_mem", "64MB"));
    D_TESTS_POSTGRES_OK(db, d_postgres_get_parameter(db, "work_mem", &value));
    D_TESTS_POSTGRES_CHECK( (value) && (strcmp(value, "64MB") == 0) );
    d_db_free(value);
    value = NULL;
    D_TESTS_POSTGRES_OK(db, d_postgres_set_parameter(db, "myapp.tenant",
                                                     "acme"));
    D_TESTS_POSTGRES_OK(db, d_postgres_get_parameter(db, "myapp.tenant",
                                                     &value));
    D_TESTS_POSTGRES_CHECK( (value) && (strcmp(value, "acme") == 0) );
    d_db_free(value);
    value = NULL;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_get_parameter(db, "no_such_param",
                                                         &value),
                            D_POSTGRES_SQLSTATE_UNDEFINED_OBJECT);
    D_TESTS_POSTGRES_CHECK(value == NULL);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_set_parameter(db, "work_mem",
                                                         "lots"),
                            D_POSTGRES_SQLSTATE_INVALID_PARAMETER_VALUE);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_get_parameter(db, "a'b", &value),
                            D_POSTGRES_SQLSTATE_UNDEFINED_OBJECT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_set_parameter(db, "", "x"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_set_parameter(db, "work_mem",
                                                         NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_get_parameter(NULL, "work_mem",
                                                         &value),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(db);

    return;
}

//==============================================================================
// postgres_maintenance.h
//==============================================================================

/*
d_tests_postgres_upkeep
  d_postgres_vacuum and d_postgres_analyze.
  Tests the following:
  - a table vacuumed in every form, and analyzed, its statistics then there
  - the whole database vacuumed and analyzed
  - VACUUM inside a transaction is the server's 25001; a table that is not
    there, 42P01
*/
static void
d_tests_postgres_upkeep(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("upkeep",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_maintenance: vacuum and analyze\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (a int); "
                                            "INSERT INTO t SELECT "
                                            "generate_series(1, 100)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_vacuum(db, NULL, "t", 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_vacuum(db, schema, "t",
                                              D_POSTGRES_FULL));
    D_TESTS_POSTGRES_OK(db, d_postgres_vacuum(db, NULL, "t",
                                              D_POSTGRES_ANALYZE));
    D_TESTS_POSTGRES_OK(db, d_postgres_vacuum(db, NULL, "t",
                                              D_POSTGRES_FULL |
                                              D_POSTGRES_ANALYZE));
    D_TESTS_POSTGRES_OK(db, d_postgres_analyze(db, NULL, "t"));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_int(db,
                             "SELECT count(*) FROM pg_stats WHERE "
                             "tablename = 't' AND attname = 'a'") == 1);
    D_TESTS_POSTGRES_OK(db, d_postgres_vacuum(db, NULL, NULL, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_analyze(db, NULL, NULL));
    D_TESTS_POSTGRES_OK(db, d_postgres_begin_transaction(
                                db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_vacuum(db, NULL, "t", 0u),
                            D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION);
    D_TESTS_POSTGRES_OK(db, d_postgres_rollback(db));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_analyze(db, NULL, "no"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_reindex_checkpoint
  d_postgres_reindex and d_postgres_checkpoint.
  Tests the following:
  - a table's indexes rebuilt; D_POSTGRES_CONCURRENTLY from 12, and
    D_DB_UNSUPPORTED before
  - a table that is not there is 42P01
  - a checkpoint, as the superuser the harness runs as
*/
static void
d_tests_postgres_reindex_checkpoint(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("reindex",
                                                        schema,
                                                        sizeof(schema));

    printf("postgres_maintenance: reindex and checkpoint\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (a int PRIMARY "
                                            "KEY)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_reindex(db, NULL, "t", 0u));
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_reindex(db, schema, "t",
                                               D_POSTGRES_CONCURRENTLY),
                            (d_tests_postgres_at_least(db, 12))
                                ? 0
                                : D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_reindex(db, NULL, "no", 0u),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_OK(db, d_postgres_checkpoint(db));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_checkpoint(NULL),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_sizes
  d_postgres_table_size and d_postgres_database_size.
  Tests the following:
  - a table's size counts its indexes too; a database's is more than nothing,
    the current one's the same as by name
  - a table or database that is not there is D_DB_NOT_FOUND, writing
    nothing
*/
static void
d_tests_postgres_sizes(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("sizes",
                                                        schema,
                                                        sizeof(schema));
    int64_t         table      = 0;
    int64_t         by_name    = 0;
    int64_t         current    = 0;

    printf("postgres_maintenance: sizes\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE t (a int PRIMARY "
                                            "KEY); INSERT INTO t SELECT "
                                            "generate_series(1, 1000)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_table_size(db, NULL, "t", &table));
    D_TESTS_POSTGRES_CHECK(
        table == d_tests_postgres_int(db,
                                      "SELECT pg_total_relation_size('t')"));
    D_TESTS_POSTGRES_CHECK(
        table > d_tests_postgres_int(db, "SELECT pg_relation_size('t')"));
    D_TESTS_POSTGRES_OK(db, d_postgres_database_size(db, NULL, &current));
    D_TESTS_POSTGRES_OK(db, d_postgres_database_size(db, g_options.database,
                                                     &by_name));
    D_TESTS_POSTGRES_CHECK( (current > 0) && (current == by_name) );
    table = -1;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_table_size(db, NULL, "no", &table),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_database_size(db, "no_such_db",
                                                         &table),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(table == -1);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_upkeep_arguments
  The maintenance functions' argument checks.
  Tests the following:
  - a schema without a table, an empty name, an unknown flag, a NULL
    connection and a NULL size are D_DB_INVALID_ARGUMENT
*/
static void
d_tests_postgres_upkeep_arguments(
    void
)
{
    struct pg_conn* db   = d_tests_postgres_connect(NULL);
    int64_t         size = 0;

    printf("postgres_maintenance: arguments\n");

    D_TESTS_POSTGRES_RESULT(db, d_postgres_vacuum(db, "s", NULL, 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_vacuum(db, NULL, "",
                                                  0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_vacuum(db, NULL, "t",
                                                  D_POSTGRES_CASCADE),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_analyze(NULL, NULL, "t"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_analyze(db, "", "t"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_reindex(db, NULL, NULL, 0u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_reindex(db, NULL, "t",
                                                   D_POSTGRES_FULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_table_size(db, NULL, "t", NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_table_size(db, NULL, NULL, &size),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_database_size(db, "", &size),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_database_size(NULL, NULL, &size),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(db);

    return;
}

//==============================================================================
// postgres_protocol.h
//==============================================================================

/*
d_tests_postgres_copy_in
  Copying rows in.
  Tests the following:
  - rows sent in chunks that split a row, \N for NULL, ended: every row
    taken and counted; CSV through the COPY's own options
  - abandoned with a message: 57014, no row taken; a row the server cannot
    read: 22P02 at the end; the connection ready after each
  - a statement that is no COPY FROM STDIN runs and is refused; a COPY to
    the client is ended; a COPY of a missing table fails as the server says
  - data or an end with no COPY in progress is 08000
  - NULL arguments
*/
static void
d_tests_postgres_copy_in(
    void
)
{
    char            schema[96] = "";
    struct pg_conn* db         = d_tests_postgres_begin("copyin",
                                                        schema,
                                                        sizeof(schema));
    int64_t         changes    = -1;

    printf("postgres_protocol: copying rows in\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db,
                                            "CREATE TABLE c (a int, b text)"));

    // three rows, the second split between chunks
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_start(
                                db, "COPY c (a, b) FROM STDIN"));
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_data(db, "1\tone\n2\ttw",
                                                    strlen("1\tone\n2\ttw")));
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_data(db, "o\n3\t\\N\n",
                                                    strlen("o\n3\t\\N\n")));
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_data(db, NULL, 0u));
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_end(db, NULL, &changes));
    D_TESTS_POSTGRES_CHECK(changes == 3);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               db, "SELECT b FROM c WHERE a = 2", "two"));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               db, "SELECT b FROM c WHERE a = 3", NULL));

    // CSV
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_start(
                                db, "COPY c FROM STDIN (FORMAT csv)"));
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_data(db, "4,\"fo,ur\"\n",
                                                    strlen("4,\"fo,ur\"\n")));
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_end(db, NULL, NULL));
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               db, "SELECT b FROM c WHERE a = 4", "fo,ur"));

    // abandoned, and refused
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_start(db, "COPY c FROM STDIN"));
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_data(db, "5\tfive\n",
                                                    strlen("5\tfive\n")));
    changes = -1;
    D_TESTS_POSTGRES_RESULT(db, d_postgres_copy_in_end(db, "changed my mind",
                                                       &changes),
                            D_POSTGRES_SQLSTATE_QUERY_CANCELED);
    D_TESTS_POSTGRES_CHECK(changes == -1);
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_start(db, "COPY c FROM STDIN"));
    D_TESTS_POSTGRES_OK(db, d_postgres_copy_in_data(db, "six\tsix\n",
                                                    strlen("six\tsix\n")));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_copy_in_end(db, NULL, &changes),
                            D_POSTGRES_SQLSTATE_INVALID_TEXT_REPRESENTATION);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT count(*) FROM c")
                           == 4);

    // what is no COPY FROM STDIN
    D_TESTS_POSTGRES_RESULT(db, d_postgres_copy_in_start(
                                    db, "UPDATE c SET b = 'x' WHERE a = 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_text_is(
                               db, "SELECT b FROM c WHERE a = 1", "x"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_copy_in_start(db,
                                                         "COPY c TO STDOUT"),
                            D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 1") == 1);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_copy_in_start(
                                    db, "COPY no_table FROM STDIN"),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);

    // no COPY in progress
    D_TESTS_POSTGRES_RESULT(db, d_postgres_copy_in_data(db, "x", 1u),
                            D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_copy_in_end(db, NULL, &changes),
                            D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 2") == 2);

    D_TESTS_POSTGRES_RESULT(db, d_postgres_copy_in_start(db, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_copy_in_data(db, NULL, 1u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_copy_in_start(NULL, "COPY"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_copy_in_data(NULL, "x", 1u),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_copy_in_end(NULL, NULL, NULL),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_arrival
  The next notification on a connection, a round trip apart from the send
so that one sent before it has reached the socket; D_DB_NOT_FOUND when
none had. The texts are the caller's.
*/
static int
d_tests_postgres_arrival(
    struct pg_conn* _db,
    char**          _channel,
    char**          _payload,
    int*            _pid
)
{
    (void)d_postgres_ping(_db);

    return d_postgres_next_notification(_db, _channel, _payload, _pid);
}

/*
d_tests_postgres_notifications
  Notifications.
  Tests the following:
  - nothing has arrived before anything is sent
  - one sent from another connection arrives, with its payload and the
    sender's backend; one sent from the listener's own too, without payload
  - one sent inside a transaction arrives when it commits
  - after UNLISTEN nothing arrives; the channel's case is kept
  - a payload too long is 22023
  - NULL and empty arguments
*/
static void
d_tests_postgres_notifications(
    void
)
{
    struct pg_conn* listener   = d_tests_postgres_connect(NULL);
    struct pg_conn* sender     = d_tests_postgres_connect(NULL);
    char            name[96]   = "";
    char            long_payload[8001] = "";
    char*           channel    = NULL;
    char*           payload    = NULL;
    int             pid        = 0;

    printf("postgres_protocol: notifications\n");

    // nothing more without a server
    if ( (!listener) ||
         (!sender) )
    {
        D_TESTS_POSTGRES_CHECK( (listener != NULL) && (sender != NULL) );
        d_postgres_close(listener);
        d_postgres_close(sender);

        return;
    }

    d_tests_postgres_schema_name(name, sizeof(name), "Notes");
    D_TESTS_POSTGRES_OK(listener, d_postgres_listen(listener, name));
    D_TESTS_POSTGRES_RESULT(listener,
                            d_tests_postgres_arrival(listener, &channel,
                                                     &payload, &pid),
                            D_DB_NOT_FOUND);

    // from another connection
    D_TESTS_POSTGRES_OK(sender, d_postgres_notify(sender, name, "hello"));
    D_TESTS_POSTGRES_OK(listener,
                        d_tests_postgres_arrival(listener, &channel,
                                                 &payload, &pid));
    D_TESTS_POSTGRES_CHECK( (channel) && (strcmp(channel, name) == 0) );
    D_TESTS_POSTGRES_CHECK( (payload) && (strcmp(payload, "hello") == 0) );
    D_TESTS_POSTGRES_CHECK(pid == d_postgres_backend_pid(sender));
    d_db_free(channel);
    d_db_free(payload);
    channel = NULL;
    payload = NULL;
    D_TESTS_POSTGRES_RESULT(listener,
                            d_tests_postgres_arrival(listener, &channel,
                                                     &payload, &pid),
                            D_DB_NOT_FOUND);

    // from its own, without payload, the payload and pid not wanted
    D_TESTS_POSTGRES_OK(listener, d_postgres_notify(listener, name, NULL));
    D_TESTS_POSTGRES_OK(listener,
                        d_tests_postgres_arrival(listener, &channel,
                                                 NULL, NULL));
    D_TESTS_POSTGRES_CHECK( (channel) && (strcmp(channel, name) == 0) );
    d_db_free(channel);
    channel = NULL;

    // inside a transaction, at its commit
    D_TESTS_POSTGRES_OK(sender, d_postgres_begin_transaction(
                                    sender, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_OK(sender, d_postgres_notify(sender, name, "later"));
    D_TESTS_POSTGRES_RESULT(listener,
                            d_tests_postgres_arrival(listener, &channel,
                                                     &payload, &pid),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_OK(sender, d_postgres_commit(sender));
    D_TESTS_POSTGRES_OK(listener,
                        d_tests_postgres_arrival(listener, &channel,
                                                 &payload, &pid));
    D_TESTS_POSTGRES_CHECK( (payload) && (strcmp(payload, "later") == 0) );
    d_db_free(channel);
    d_db_free(payload);
    channel = NULL;
    payload = NULL;

    // no longer listening, to it or to anything
    D_TESTS_POSTGRES_OK(listener, d_postgres_unlisten(listener, name));
    D_TESTS_POSTGRES_OK(sender, d_postgres_notify(sender, name, "gone"));
    D_TESTS_POSTGRES_RESULT(listener,
                            d_tests_postgres_arrival(listener, &channel,
                                                     &payload, &pid),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_OK(listener, d_postgres_listen(listener, name));
    D_TESTS_POSTGRES_OK(listener, d_postgres_unlisten(listener, NULL));
    D_TESTS_POSTGRES_OK(sender, d_postgres_notify(sender, name, "gone"));
    D_TESTS_POSTGRES_RESULT(listener,
                            d_tests_postgres_arrival(listener, &channel,
                                                     &payload, &pid),
                            D_DB_NOT_FOUND);

    // a payload past the server's limit
    memset(long_payload, 'p', sizeof(long_payload) - 1u);
    long_payload[sizeof(long_payload) - 1u] = '\0';
    D_TESTS_POSTGRES_RESULT(sender,
                            d_postgres_notify(sender, name, long_payload),
                            D_POSTGRES_SQLSTATE_INVALID_PARAMETER_VALUE);

    D_TESTS_POSTGRES_RESULT(listener, d_postgres_listen(listener, ""),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(listener, d_postgres_listen(listener, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(listener, d_postgres_unlisten(listener, ""),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(sender, d_postgres_notify(sender, NULL, "x"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(listener,
                            d_postgres_next_notification(listener, NULL,
                                                         &payload, &pid),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_listen(NULL, name),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_unlisten(NULL, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_notify(NULL, name, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL,
                            d_postgres_next_notification(NULL, &channel,
                                                         NULL, NULL),
                            D_DB_INVALID_ARGUMENT);

    d_postgres_close(listener);
    d_postgres_close(sender);

    return;
}

/*
d_tests_postgres_sent_statements
  Statements sent without waiting.
  Tests the following:
  - several statements sent at once give their results in turn -- rows, a
    count, rows -- then D_DB_NOT_FOUND, again and again
  - while a statement runs the connection is busy and refuses to send
    more; ready once its results are read; a statement run before then is
    08000, the layer's own answer, and the results are still there to read
  - a failure ends the results; a COPY is ended
  - NULL arguments
*/
static void
d_tests_postgres_sent_statements(
    void
)
{
    char             schema[96] = "";
    struct pg_conn*  db         = d_tests_postgres_begin("sent",
                                                         schema,
                                                         sizeof(schema));
    struct d_db_rows rows       = { 0u, 0u, NULL, NULL };
    int64_t          changes    = -1;
    int64_t          number     = 0;
    long             waits      = 0;
    int              consumed   = 0;

    printf("postgres_protocol: statements sent\n");

    memset(&rows, 0, sizeof(rows));

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE t (a int)"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_result(db, &rows, &changes),
                            D_DB_NOT_FOUND);

    // three statements, three results
    D_TESTS_POSTGRES_OK(db, d_postgres_send_query(
                                db,
                                "SELECT 1 AS a; "
                                "INSERT INTO t VALUES (1), (2); "
                                "SELECT 'x' AS s"));
    D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 1u) &&
                            (strcmp(rows.columns[0].name, "a") == 0) &&
                            (changes == 1) );
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
    D_TESTS_POSTGRES_CHECK( (rows.column_count == 0u) && (changes == 2) );
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, NULL));
    D_TESTS_POSTGRES_CHECK( (rows.row_count == 1u) &&
                            (d_db_rows_value(&rows, 0u, 0u)) &&
                            (strcmp(d_db_rows_value(&rows, 0u, 0u)->as.text,
                                    "x") == 0) );
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_result(db, &rows, &changes),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_result(db, &rows, &changes),
                            D_DB_NOT_FOUND);

    // busy while it runs, refusing to send more
    D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db,
                                                  "SELECT pg_sleep(0.2), 7"));
    D_TESTS_POSTGRES_CHECK(d_postgres_is_busy(db));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_send_query(db, "SELECT 1"),
                            D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);

    // what has come read, until the result is whole
    while ( (d_postgres_is_busy(db)) &&
            (consumed == 0)          &&
            (waits < 100000000) )
    {
        consumed = d_postgres_consume_input(db);
        ++waits;
    }

    D_TESTS_POSTGRES_CHECK(consumed == 0);
    D_TESTS_POSTGRES_CHECK(!d_postgres_is_busy(db));
    D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
    D_TESTS_POSTGRES_CHECK(
        (d_db_value_int64(d_db_rows_value(&rows, 0u, 1u), &number) == 0) &&
        (number == 7));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_result(db, &rows, &changes),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 3") == 3);

    // a statement run before the results are read is refused, and they are
    // still there
    D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "SELECT 8"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_exec(db, "SELECT 9"),
                            D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    D_TESTS_POSTGRES_CHECK(
        d_postgres_result_is_own(db,
                                 D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION));
    D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
    D_TESTS_POSTGRES_CHECK(
        (d_db_value_int64(d_db_rows_value(&rows, 0u, 0u), &number) == 0) &&
        (number == 8));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_result(db, &rows, &changes),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT 9") == 9);

    // a failure, and a COPY
    D_TESTS_POSTGRES_OK(db, d_postgres_send_query(
                                db,
                                "SELECT 1; SELECT * FROM no_table; SELECT 3"));
    D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_result(db, &rows, &changes),
                            D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_result(db, &rows, &changes),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "COPY t FROM STDIN"));
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_result(db, &rows, &changes),
                            D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_result(db, &rows, &changes),
                            D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_CHECK(d_tests_postgres_int(db, "SELECT count(*) FROM t")
                           == 2);

    D_TESTS_POSTGRES_RESULT(db, d_postgres_send_query(db, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(db, d_postgres_next_result(db, NULL, &changes),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_send_query(NULL, "SELECT 1"),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_next_result(NULL, &rows, NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL, d_postgres_consume_input(NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(!d_postgres_is_busy(NULL));

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_rows_read
  Reads every result left of the SQL sent, freeing the rows: the results'
rows one after another, and the largest count any of them gave. Returns how
many results came, or -1 for a failure.
*/
static int
d_tests_postgres_rows_read(
    struct pg_conn* _db,
    size_t*         _rows,
    size_t*         _last,
    int64_t*        _count
)
{
    struct d_db_rows rows    = { 0u, 0u, NULL, NULL };
    int64_t          changes = 0;
    int              results = 0;
    int              result  = 0;

    *_rows  = 0u;
    *_last  = 0u;
    *_count = 0;

    // each result, until none is left
    while ((result = d_postgres_next_result(_db, &rows, &changes)) == 0)
    {
        *_rows  += rows.row_count;
        *_last   = rows.row_count;
        *_count  = (changes > *_count) ? changes : *_count;
        ++results;
        d_db_rows_free(&rows);
    }

    return (result == D_DB_NOT_FOUND) ? results : -1;
}

/*
d_tests_postgres_chunked_rows
  Rows a few at a time.
  Tests the following:
  - one row a result, as every libpq has it: each row its own result, then
    the statement's end, with no rows and the count
  - more, where libpq has them (17): chunks of up to that many, the last
    with the count, then the end; and D_DB_UNSUPPORTED where it has not
  - asked for straight after the send, it holds for every statement sent;
    asked for between one statement's results and the next's, for the rest
  - asked for with nothing sent, or once the statement's rows have begun:
    08000, the rows then coming as they were
  - fewer than one row; a NULL connection
*/
static void
d_tests_postgres_chunked_rows(
    void
)
{
    char             schema[96] = "";
    struct pg_conn*  db         = d_tests_postgres_begin("chunks",
                                                         schema,
                                                         sizeof(schema));
    struct d_db_rows rows       = { 0u, 0u, NULL, NULL };
    int64_t          count      = 0;
    size_t           total      = 0u;
    size_t           last       = 0u;

    printf("postgres_protocol: rows a few at a time\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    // a row a result, and the end
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_send_query(
                            db,
                            "SELECT generate_series(1, 5) AS n"));
    D_TESTS_POSTGRES_OK(db, d_postgres_set_chunked_rows(db, 1));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_rows_read(db, &total, &last, &count) == 6);
    D_TESTS_POSTGRES_CHECK( (total == 5u) &&
                            (last == 0u)  &&
                            (count == 5) );

    // more than one, where libpq has it
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_send_query(
                            db,
                            "SELECT generate_series(1, 5) AS n"));

    if (d_postgres_client_supports(D_POSTGRES_CLIENT_CHUNKED_ROWS))
    {
        D_TESTS_POSTGRES_OK(db, d_postgres_set_chunked_rows(db, 2));
        D_TESTS_POSTGRES_CHECK(
            d_tests_postgres_rows_read(db, &total, &last, &count) == 4);
        D_TESTS_POSTGRES_CHECK( (total == 5u) &&
                                (last == 0u)  &&
                                (count == 5) );
    }
    else
    {
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_set_chunked_rows(db, 2),
                                D_DB_UNSUPPORTED);
        D_TESTS_POSTGRES_CHECK(
            d_tests_postgres_rows_read(db, &total, &last, &count) == 1);
        D_TESTS_POSTGRES_CHECK(total == 5u);
    }

    // every statement sent: two rows and the end, then a row and the end
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_send_query(
                            db,
                            "SELECT 1 AS n UNION ALL SELECT 2; SELECT 3"));
    D_TESTS_POSTGRES_OK(db, d_postgres_set_chunked_rows(db, 1));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_rows_read(db, &total, &last, &count) == 5);
    D_TESTS_POSTGRES_CHECK( (total == 3u) &&
                            (last == 0u)  &&
                            (count == 2) );

    // the statements after the first
    D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "SELECT 1; SELECT 2"));
    D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, NULL));
    D_TESTS_POSTGRES_CHECK(rows.row_count == 1u);
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_OK(db, d_postgres_set_chunked_rows(db, 1));
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_rows_read(db, &total, &last, &count) == 2);
    D_TESTS_POSTGRES_CHECK( (total == 1u) && (last == 0u) );

    // nothing sent, or the rows begun
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_set_chunked_rows(db, 1),
                            D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    D_TESTS_POSTGRES_OK(db,
                        d_postgres_send_query(
                            db,
                            "SELECT generate_series(1, 3) AS n"));
    D_TESTS_POSTGRES_OK(db, d_postgres_set_chunked_rows(db, 1));
    D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, NULL));
    D_TESTS_POSTGRES_CHECK(rows.row_count == 1u);
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_set_chunked_rows(db, 1),
                            D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    D_TESTS_POSTGRES_CHECK(
        d_tests_postgres_rows_read(db, &total, &last, &count) == 3);
    D_TESTS_POSTGRES_CHECK( (total == 2u) && (count == 3) );

    D_TESTS_POSTGRES_RESULT(db,
                            d_postgres_set_chunked_rows(db, 0),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL,
                            d_postgres_set_chunked_rows(NULL, 1),
                            D_DB_INVALID_ARGUMENT);

    d_tests_postgres_end(db, schema);

    return;
}

/*
d_tests_postgres_pipelines
  Pipelines.
  Tests the following:
  - where libpq has them (14): entered, and entered again, rows a few at a
    time asked for just before holding no further; statements queued
    and synchronized give their results in order -- rows, a count, a
    failure, which the connection's message still describes, 25P02 for the
    statement it skipped, the layer's own answer -- then D_DB_NOT_FOUND at
    the synchronization point, then the next batch's, then D_DB_NOT_FOUND
    again and again; the failed batch rolled back; the mode aborted until
    the point, and on after it
  - a call that waits for its result is 08000 in the mode; two statements
    in one send are the server's syntax error; leaving with results still to
    read is 08000, and leaving after them returns the connection as it was
  - rows a few at a time, asked for statement by statement
  - a statement queued after the last synchronization point: its result
    read all the same, waited for, or polled for once asked for
  - a transaction a statement begins outlasts its synchronization point
  - synchronizing or asking for results outside the mode is 08000
  - where libpq has no pipelines: D_DB_UNSUPPORTED, the mode off
  - NULL arguments
*/
static void
d_tests_postgres_pipelines(
    void
)
{
    char             schema[96] = "";
    struct pg_conn*  db         = d_tests_postgres_begin("pipelines",
                                                         schema,
                                                         sizeof(schema));
    struct d_db_rows rows       = { 0u, 0u, NULL, NULL };
    int64_t          changes    = -1;
    int64_t          number     = 0;
    size_t           total      = 0u;
    size_t           last       = 0u;
    int64_t          count      = 0;
    int              consumed   = 0;
    unsigned long    waits      = 0u;

    printf("postgres_protocol: pipelines\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_POSTGRES_CHECK(db != NULL);

        return;
    }

    D_TESTS_POSTGRES_CHECK(d_postgres_pipeline_status(db) ==
                           D_POSTGRES_PIPELINE_OFF);

    if (d_postgres_client_supports(D_POSTGRES_CLIENT_PIPELINE))
    {
        // rows a row a result just before, which the pipeline does not keep
        D_TESTS_POSTGRES_OK(db, d_postgres_exec(db, "CREATE TABLE t (a int)"));
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_send_query(
                                db,
                                "SELECT generate_series(1, 2) AS n"));
        D_TESTS_POSTGRES_OK(db, d_postgres_set_chunked_rows(db, 1));
        D_TESTS_POSTGRES_CHECK(
            d_tests_postgres_rows_read(db, &total, &last, &count) == 3);
        D_TESTS_POSTGRES_OK(db, d_postgres_enter_pipeline(db));
        D_TESTS_POSTGRES_OK(db, d_postgres_enter_pipeline(db));
        D_TESTS_POSTGRES_CHECK(d_postgres_pipeline_status(db) ==
                               D_POSTGRES_PIPELINE_ON);

        // two batches
        D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "SELECT 10 AS a"));
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_send_query(db,
                                                  "INSERT INTO t VALUES (1)"));
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_send_query(db,
                                                  "SELECT * FROM no_table"));
        D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "SELECT 2"));
        D_TESTS_POSTGRES_OK(db, d_postgres_pipeline_sync(db));
        D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "SELECT 3"));
        D_TESTS_POSTGRES_OK(db, d_postgres_pipeline_sync(db));
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_exec(db, "SELECT 9"),
                                D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);

        // the first batch: rows, a count, a failure, one skipped, the point
        D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
        D_TESTS_POSTGRES_CHECK(
            (d_db_value_int64(d_db_rows_value(&rows, 0u, 0u), &number) == 0) &&
            (number == 10) &&
            (strcmp(rows.columns[0].name, "a") == 0));
        d_db_rows_free(&rows);
        D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
        D_TESTS_POSTGRES_CHECK( (rows.column_count == 0u) && (changes == 1) );
        d_db_rows_free(&rows);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
        D_TESTS_POSTGRES_CHECK(strstr(d_postgres_error_message(db),
                                      "no_table") != NULL);
        D_TESTS_POSTGRES_CHECK(d_postgres_pipeline_status(db) ==
                               D_POSTGRES_PIPELINE_ABORTED);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
        D_TESTS_POSTGRES_CHECK(
            d_postgres_result_is_own(
                db,
                D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION));
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_DB_NOT_FOUND);
        D_TESTS_POSTGRES_CHECK(d_postgres_pipeline_status(db) ==
                               D_POSTGRES_PIPELINE_ON);

        // the second, and nothing after it
        D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
        D_TESTS_POSTGRES_CHECK(
            (d_db_value_int64(d_db_rows_value(&rows, 0u, 0u), &number) == 0) &&
            (number == 3));
        d_db_rows_free(&rows);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_DB_NOT_FOUND);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_DB_NOT_FOUND);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_DB_NOT_FOUND);

        // two statements at once; leaving too early
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_send_query(db, "SELECT 1; SELECT 2"));
        D_TESTS_POSTGRES_OK(db, d_postgres_pipeline_sync(db));
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_exit_pipeline(db),
                                D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_POSTGRES_SQLSTATE_SYNTAX_ERROR);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_DB_NOT_FOUND);

        // rows a few at a time, for the statement asked for alone: the
        // first a row a result, the second all at once, the third a row a
        // result again
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_send_query(
                                db,
                                "SELECT generate_series(1, 2) AS n"));
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_send_query(
                                db,
                                "SELECT generate_series(1, 2) AS n"));
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_send_query(
                                db,
                                "SELECT generate_series(1, 2) AS n"));
        D_TESTS_POSTGRES_OK(db, d_postgres_pipeline_sync(db));
        D_TESTS_POSTGRES_OK(db, d_postgres_set_chunked_rows(db, 1));
        D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
        D_TESTS_POSTGRES_CHECK(rows.row_count == 1u);
        d_db_rows_free(&rows);
        D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
        D_TESTS_POSTGRES_CHECK(rows.row_count == 1u);
        d_db_rows_free(&rows);
        D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
        D_TESTS_POSTGRES_CHECK( (rows.row_count == 0u) &&
                                (rows.column_count == 1u) &&
                                (changes == 2) );
        d_db_rows_free(&rows);
        D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
        D_TESTS_POSTGRES_CHECK( (rows.row_count == 2u) && (changes == 2) );
        d_db_rows_free(&rows);
        D_TESTS_POSTGRES_OK(db, d_postgres_set_chunked_rows(db, 1));
        D_TESTS_POSTGRES_CHECK(
            d_tests_postgres_rows_read(db, &total, &last, &count) == 3);
        D_TESTS_POSTGRES_CHECK( (total == 2u) &&
                                (last == 0u)  &&
                                (count == 2) );
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_set_chunked_rows(db, 1),
                                D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);

        // a statement after the last synchronization point, waited for
        D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "SELECT 42"));
        D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
        D_TESTS_POSTGRES_CHECK(
            (d_db_value_int64(d_db_rows_value(&rows, 0u, 0u), &number) == 0) &&
            (number == 42));
        d_db_rows_free(&rows);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_DB_NOT_FOUND);

        // and polled for, once asked for
        D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "SELECT 43"));
        D_TESTS_POSTGRES_OK(db, d_postgres_pipeline_flush(db));

        while ( (d_postgres_is_busy(db)) &&
                (consumed == 0)          &&
                (waits < 100000000u) )
        {
            consumed = d_postgres_consume_input(db);
            ++waits;
        }

        D_TESTS_POSTGRES_CHECK(consumed == 0);
        D_TESTS_POSTGRES_CHECK(!d_postgres_is_busy(db));
        D_TESTS_POSTGRES_OK(db, d_postgres_next_result(db, &rows, &changes));
        D_TESTS_POSTGRES_CHECK(
            (d_db_value_int64(d_db_rows_value(&rows, 0u, 0u), &number) == 0) &&
            (number == 43));
        d_db_rows_free(&rows);
        D_TESTS_POSTGRES_OK(db, d_postgres_pipeline_sync(db));
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_next_result(db, &rows, &changes),
                                D_DB_NOT_FOUND);

        // a transaction across synchronization points: each point's
        // statement, then the point
        D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "BEGIN"));
        D_TESTS_POSTGRES_OK(db, d_postgres_pipeline_sync(db));
        D_TESTS_POSTGRES_OK(db,
                            d_postgres_send_query(db,
                                                  "INSERT INTO t VALUES (2)"));
        D_TESTS_POSTGRES_OK(db, d_postgres_pipeline_sync(db));
        D_TESTS_POSTGRES_OK(db, d_postgres_send_query(db, "COMMIT"));
        D_TESTS_POSTGRES_OK(db, d_postgres_pipeline_sync(db));
        D_TESTS_POSTGRES_CHECK(
            d_tests_postgres_rows_read(db, &total, &last, &count) == 1);
        D_TESTS_POSTGRES_CHECK(
            d_tests_postgres_rows_read(db, &total, &last, &count) == 1);
        D_TESTS_POSTGRES_CHECK(count == 1);
        D_TESTS_POSTGRES_CHECK(
            d_tests_postgres_rows_read(db, &total, &last, &count) == 1);

        D_TESTS_POSTGRES_OK(db, d_postgres_exit_pipeline(db));
        D_TESTS_POSTGRES_OK(db, d_postgres_exit_pipeline(db));
        D_TESTS_POSTGRES_CHECK(d_postgres_pipeline_status(db) ==
                               D_POSTGRES_PIPELINE_OFF);
        D_TESTS_POSTGRES_CHECK(
            d_tests_postgres_int(db, "SELECT count(*) FROM t WHERE a = 1") ==
            0);
        D_TESTS_POSTGRES_CHECK(
            d_tests_postgres_int(db, "SELECT count(*) FROM t WHERE a = 2") ==
            1);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_pipeline_sync(db),
                                D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_pipeline_flush(db),
                                D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    }
    else
    {
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_enter_pipeline(db),
                                D_DB_UNSUPPORTED);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_exit_pipeline(db),
                                D_DB_UNSUPPORTED);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_pipeline_sync(db),
                                D_DB_UNSUPPORTED);
        D_TESTS_POSTGRES_RESULT(db,
                                d_postgres_pipeline_flush(db),
                                D_DB_UNSUPPORTED);
        D_TESTS_POSTGRES_CHECK(d_postgres_pipeline_status(db) ==
                               D_POSTGRES_PIPELINE_OFF);
    }

    D_TESTS_POSTGRES_RESULT(NULL,
                            d_postgres_enter_pipeline(NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL,
                            d_postgres_exit_pipeline(NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL,
                            d_postgres_pipeline_sync(NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_RESULT(NULL,
                            d_postgres_pipeline_flush(NULL),
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_CHECK(d_postgres_pipeline_status(NULL) ==
                           D_POSTGRES_PIPELINE_OFF);

    d_tests_postgres_end(db, schema);

    return;
}

//==============================================================================
// RUN
//==============================================================================

// d_tests_postgres_all
//   table: every test, in the headers' order.
static void (* const d_tests_postgres_all[])(void) =
{
    d_tests_postgres_db_common,
    d_tests_postgres_sqlstates,
    d_tests_postgres_messages,
    d_tests_postgres_client,
    d_tests_postgres_versions,
    d_tests_postgres_features,
    d_tests_postgres_feature_statements,
    d_tests_postgres_reported_parameters,
    d_tests_postgres_errors,
    d_tests_postgres_own_results,
    d_tests_postgres_connecting,
    d_tests_postgres_float_digits,
    d_tests_postgres_connect_failures,
    d_tests_postgres_connect_defaults,
    d_tests_postgres_connect_strings,
    d_tests_postgres_connect_with,
    d_tests_postgres_liveness,
    d_tests_postgres_lost_connections,
    d_tests_postgres_statements,
    d_tests_postgres_scripts,
    d_tests_postgres_copies,
    d_tests_postgres_changes,
    d_tests_postgres_integers,
    d_tests_postgres_doubles,
    d_tests_postgres_text,
    d_tests_postgres_bools,
    d_tests_postgres_rows_values,
    d_tests_postgres_rows_types,
    d_tests_postgres_rows_params,
    d_tests_postgres_quoting_names,
    d_tests_postgres_quoting_values,
    d_tests_postgres_query_arguments,
    d_tests_postgres_preparing,
    d_tests_postgres_binding,
    d_tests_postgres_running,
    d_tests_postgres_statement_arguments,
    d_tests_postgres_closing_by_name,
    d_tests_postgres_cursors,
    d_tests_postgres_large_objects,
    d_tests_postgres_databases,
    d_tests_postgres_database_transactions,
    d_tests_postgres_drop_force,
    d_tests_postgres_schemas,
    d_tests_postgres_temporary_schema,
    d_tests_postgres_current_schema,
    d_tests_postgres_extensions,
    d_tests_postgres_extension_cascade,
    d_tests_postgres_database_arguments,
    d_tests_postgres_table_existence,
    d_tests_postgres_table_creation,
    d_tests_postgres_table_drops,
    d_tests_postgres_table_renames,
    d_tests_postgres_table_rows,
    d_tests_postgres_table_listing,
    d_tests_postgres_null_schema,
    d_tests_postgres_table_arguments,
    d_tests_postgres_creation_arguments,
    d_tests_postgres_described_types,
    d_tests_postgres_described_attributes,
    d_tests_postgres_described_keys,
    d_tests_postgres_described_refusals,
    d_tests_postgres_defaults,
    d_tests_postgres_default_values,
    d_tests_postgres_column_existence,
    d_tests_postgres_column_additions,
    d_tests_postgres_column_removals,
    d_tests_postgres_column_listing,
    d_tests_postgres_column_arguments,
    d_tests_postgres_indexes,
    d_tests_postgres_index_forms,
    d_tests_postgres_index_changes,
    d_tests_postgres_views,
    d_tests_postgres_view_settling,
    d_tests_postgres_view_changes,
    d_tests_postgres_triggers,
    d_tests_postgres_trigger_changes,
    d_tests_postgres_sequences,
    d_tests_postgres_sequence_values,
    d_tests_postgres_last_values,
    d_tests_postgres_object_arguments,
    d_tests_postgres_transactions,
    d_tests_postgres_failed_transactions,
    d_tests_postgres_transaction_modes,
    d_tests_postgres_savepoints,
    d_tests_postgres_transactional_ddl,
    d_tests_postgres_search_path,
    d_tests_postgres_timeouts,
    d_tests_postgres_isolation,
    d_tests_postgres_encoding_and_zone,
    d_tests_postgres_parameters,
    d_tests_postgres_upkeep,
    d_tests_postgres_reindex_checkpoint,
    d_tests_postgres_sizes,
    d_tests_postgres_upkeep_arguments,
    d_tests_postgres_copy_in,
    d_tests_postgres_notifications,
    d_tests_postgres_sent_statements,
    d_tests_postgres_chunked_rows,
    d_tests_postgres_pipelines
};

/*
main
  Connects once to name the run, runs every test and reports; the exit
status is 0 only if every check passed. Without a server to reach, it says
how to name one, and fails.
*/
int
main(
    void
)
{
    const size_t    count = sizeof(d_tests_postgres_all) /
                            sizeof(d_tests_postgres_all[0]);
    struct pg_conn* setup = NULL;

    d_tests_postgres_configure();
    setup = d_tests_postgres_connect(NULL);

    // a server to test against
    if (!setup)
    {
        printf("no server: set DJINTERP_TEST_POSTGRES_HOST, _PORT, _USER, "
               "_PASSWORD or _DATABASE\n");

        return 1;
    }

    snprintf(g_run,
             sizeof(g_run),
             "d_tests_postgres_%d",
             d_postgres_backend_pid(setup));
    printf("server %s; libpq %d\n",
           d_postgres_server_version(setup),
           d_postgres_client_version_number());
    d_postgres_close(setup);

    for (size_t i = 0u; i < count; ++i)
    {
        d_tests_postgres_all[i]();
    }

    printf("%d checks, %d failed\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
