/*******************************************************************************
* djinterp [c]                                                t_postgres_cpp.cpp
*
*   The PostgreSQL layer as C++ sees it: one translation unit, compiled as C++
* at every level from ISO strict C++98 to C++26, that includes the layer's
* umbrella and nothing of libpq's, and drives a server through it. It proves
* the headers are clean C++ at each level (libpq-fe.h spells `long long` on
* Windows), that struct d_postgres_options and struct d_db_column take
* positional initializers where designated ones do not exist, and that the
* extern "C" linkage meets the C objects the sources compile to.
*   The checks that need no server always run. A session against one runs
* when the harness's settings reach one (DJINTERP_TEST_POSTGRES_HOST, _PORT,
* _USER, _PASSWORD, _DATABASE, as for t_postgres.c);
* DJINTERP_TEST_POSTGRES_REQUIRED=1 makes a server that cannot be reached a
* failure. Below the 64-bit floor the layer's headers compile to nothing, and
* so does this test. Over a layer built without libpq (D_CFG_POSTGRES 0), it
* checks that no connection is made and no statement prepared.
*
*   Build: compile the layer's C sources as C, this file as C++, and link them
* with libpq -- ci/check_db_postgres.sh does, at every level.
*
*
* path:      /tests/unintegrated/c/db/postgres/t_postgres_cpp.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.26
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstdlib>  // std::getenv, std::strtoul
#include <cstring>  // std::memcpy, std::strcmp
// djinterp
#include "../../../../../inc/djinterp/c/db/postgres/postgres.h"  // under test
// re_std
#include "../../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int64_t,
                                                         // INT64_MAX


#if defined(INT64_MAX)

namespace
{

int g_checks   = 0;
int g_failures = 0;

/*
d_tests_postgres_cpp_expect
  Counts one check, and prints it when it fails.
*/
void
d_tests_postgres_cpp_expect(
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
        std::printf("  FAIL line %d: %s\n", _line, _what);
    }

    return;
}

/*
d_tests_postgres_cpp_name
  "d_tests_postgres_cpp_" and a number, its digits written by hand: sprintf
is deprecated on Apple's platforms, and snprintf is not C++98's.
*/
void
d_tests_postgres_cpp_name(
    char*         _out,
    unsigned long _number
)
{
    static const char prefix[]   = "d_tests_postgres_cpp_";
    const std::size_t length     = sizeof(prefix) - 1u;
    char              digits[24] = "";
    std::size_t       count      = 0u;

    // the digits, last first
    do
    {
        digits[count++] = static_cast<char>('0' + (_number % 10ul));
        _number        /= 10ul;
    } while (_number != 0ul);

    std::memcpy(_out, prefix, length);

    for (std::size_t i = 0u; i < count; ++i)
    {
        _out[length + i] = digits[count - 1u - i];
    }

    _out[length + count] = '\0';

    return;
}

/*
d_tests_postgres_cpp_setting
  An environment variable, or a default when it is unset or empty.
*/
const char*
d_tests_postgres_cpp_setting(
    const char* _name,
    const char* _default
)
{
    const char* const value = std::getenv(_name);

    return ( (value) && (value[0] != '\0') ) ? value : _default;
}

}  // namespace

// D_TESTS_POSTGRES_CPP_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_POSTGRES_CPP_CHECK(_cond)                                     \
    d_tests_postgres_cpp_expect((_cond), #_cond, __LINE__)

namespace
{

/*
d_tests_postgres_cpp_serverless
  What needs no server: reading results, quoting a name, and the layer's
refusal of NULL connections.
*/
void
d_tests_postgres_cpp_serverless(
    bool _stubs
)
{
    char* quoted = d_postgres_quote_identifier("a\"b");

    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_result_from_sqlstate("42P01") ==
        D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_CPP_CHECK(
        (_stubs) ? (quoted == 0)
                 : ( (quoted) && (std::strcmp(quoted, "\"a\"\"b\"") == 0) ));
    d_db_free(quoted);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_exec(0, "SELECT 1") ==
        ((_stubs) ? D_DB_UNSUPPORTED : D_DB_INVALID_ARGUMENT));
    D_TESTS_POSTGRES_CPP_CHECK(!d_postgres_has_table(0, 0, "t"));
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_close(0) == D_DB_OK);

    return;
}

/*
d_tests_postgres_cpp_rows
  Counts changes, reads rows, and runs a prepared statement with a value of
every kind bound, reading the values back through the conversions, in the
session's schema.
*/
void
d_tests_postgres_cpp_rows(
    struct pg_conn* _db,
    const char*     _schema
)
{
    struct d_postgres_statement* statement = 0;
    struct d_db_rows             rows      = { 0u, 0u, 0, 0 };
    re_std::int64_t              changes   = -1;
    re_std::int64_t              number    = 0;
    double                       real      = 0.0;
    bool                         truth     = false;
    std::size_t                  index     = 0u;

    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_execf(_db, "CREATE TABLE %T (i bigint, r float8, t text, "
                              "b bytea, f boolean, n integer)",
                         _schema, "v") == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_preparef(_db, &statement,
                            "INSERT INTO %T VALUES ($1, $2, $3, $4, $5, $6)",
                            _schema, "v") == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_parameter_count(statement) == 6u);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_statement_connection(statement) ==
                               _db);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_bind_int64(statement, 1u, 42) ==
                               D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_bind_double(statement, 2u, 2.5) ==
                               D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_bind_text(statement, 3u, "x") ==
                               D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_bind_blob(statement, 4u, "\0", 1u) == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_bind_bool(statement, 5u, true) ==
                               D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_bind_null(statement, 6u) ==
                               D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(
        (d_postgres_execute_update(statement, &changes) == D_DB_OK) &&
        (changes == 1));
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_clear_bindings(statement) ==
                               D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_execute_query(statement, &rows) ==
                               D_DB_OK);
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_finalize(statement) == D_DB_OK);

    D_TESTS_POSTGRES_CPP_CHECK(
        (d_postgres_execf_changes(_db, &changes, "UPDATE %T SET n = 1",
                                  _schema, "v") == D_DB_OK) &&
        (changes == 2));
    D_TESTS_POSTGRES_CPP_CHECK(
        (d_postgres_execf_changes(_db, &changes,
                                  "DELETE FROM %T WHERE i IS NULL",
                                  _schema, "v") == D_DB_OK) &&
        (changes == 1));
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_query_rowsf(_db, &rows,
                               "SELECT i, r, t, b, f, n FROM %T",
                               _schema, "v") == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK( (rows.column_count == 6u) &&
                                (rows.row_count == 1u)    &&
                                (rows.columns[4].type ==
                                 D_DB_TYPE_BOOLEAN) );
    D_TESTS_POSTGRES_CPP_CHECK(
        (d_db_value_int64(d_db_rows_value(&rows, 0u, 0u), &number) ==
         D_DB_OK) &&
        (number == 42));
    D_TESTS_POSTGRES_CPP_CHECK(
        (d_db_value_double(d_db_rows_value(&rows, 0u, 1u), &real) ==
         D_DB_OK) &&
        (real == 2.5));
    D_TESTS_POSTGRES_CPP_CHECK(
        (d_db_value_bool(d_db_rows_value(&rows, 0u, 4u), &truth) ==
         D_DB_OK) &&
        (truth));
    D_TESTS_POSTGRES_CPP_CHECK(
        d_db_value_int64(d_db_rows_value(&rows, 0u, 2u), &number) ==
        D_DB_MISMATCH);
    D_TESTS_POSTGRES_CPP_CHECK(
        (d_db_rows_column_index(&rows, "T", &index) == D_DB_OK) &&
        (index == 2u));
    D_TESTS_POSTGRES_CPP_CHECK(d_db_rows_value(&rows, 0u, 3u)->size == 1u);
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_query_rows(_db, &rows, "SELECT 'y'::text") == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(
        (rows.row_count == 1u) &&
        (std::strcmp(rows.values[0].as.text, "y") == 0));
    d_db_rows_free(&rows);
    D_TESTS_POSTGRES_CPP_CHECK(
        (d_postgres_exec_changes(_db, &changes, "SELECT 1") == D_DB_OK) &&
        (changes == 1));
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_prepare(_db, &statement, "SELECT $1::integer") ==
        D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_finalize(statement) == D_DB_OK);

    return;
}

/*
d_tests_postgres_cpp_session
  Makes a schema, a table from column descriptions and positional
initializers, and drives it through every kind of call; then drops it.
*/
void
d_tests_postgres_cpp_session(
    struct pg_conn* _db
)
{
    static const struct d_db_column columns[] =
    {
        { "id",   D_DB_TYPE_BIG_INTEGER,
          D_DB_COLUMN_PRIMARY_KEY | D_DB_COLUMN_AUTO_INCREMENT,
          0, 0u, 0u, 0, 0u },
        { "name", D_DB_TYPE_STRING, D_DB_COLUMN_NOT_NULL,
          "'?'", 40u, 0u, 0, 0u }
    };
    char                    schema[64] = "";
    struct d_db_name_list   names      = { 0u, 0 };
    struct d_db_column_list info       = { 0u, 0 };
    re_std::int64_t         count      = -1;
    char*                   text       = 0;

    d_tests_postgres_cpp_name(
        schema,
        static_cast<unsigned long>(d_postgres_backend_pid(_db)));
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_create_schema(_db, schema, 0u) == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_create_table(_db, schema, "people", columns, 2u,
                                D_DB_IF_NOT_EXISTS) == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_has_table(_db, schema, "people"));
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_has_column(_db, schema, "people",
                                                     "name"));
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_list_tables(_db, schema, &names) == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK( (names.count == 1u) &&
                                (std::strcmp(names.names[0], "people") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_list_columns(_db, schema, "people", &info) == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK( (info.count == 2u)                 &&
                                (info.columns[0].primary_key == 1) &&
                                (info.columns[1].not_null) );
    d_db_column_list_free(&info);

    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_begin_transaction(_db, D_POSTGRES_TRANSACTION_DEFAULT) ==
        D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_execf(_db, "INSERT INTO %T (name) VALUES (%Q)",
                         schema, "people", "O'Hara") == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_commit(_db) == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_count_rows(_db, schema, "people", &count) == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK(count == 1);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_query_textf(_db, &text, "SELECT name FROM %T",
                               schema, "people") == D_DB_OK);
    D_TESTS_POSTGRES_CPP_CHECK( (text) && (std::strcmp(text, "O'Hara") == 0) );
    d_db_free(text);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_query_int64(_db, &count, "SELECT 1 WHERE false") ==
        D_DB_NOT_FOUND);
    d_tests_postgres_cpp_rows(_db, schema);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_drop_schema(_db, schema, D_POSTGRES_CASCADE) == D_DB_OK);

    return;
}

/*
d_tests_postgres_cpp_stubs
  The layer built without libpq: no connection is made, NULL closes, and
nothing exists.
*/
void
d_tests_postgres_cpp_stubs(
    void
)
{
    const struct d_postgres_options options = { "localhost", "postgres", 0, 0,
                                                0u, 0 };
    struct pg_conn*                 db        = 0;
    struct d_postgres_statement*    statement = 0;
    struct d_db_rows                rows      = { 0u, 0u, 0, 0 };

    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_connect(&options, &db, 0) == D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_CPP_CHECK(db == 0);
    D_TESTS_POSTGRES_CPP_CHECK(!d_postgres_has_database(db, "postgres"));
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_query_rows(db, &rows, "SELECT 1") == D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_CPP_CHECK(
        d_postgres_prepare(db, &statement, "SELECT 1") == D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_CPP_CHECK(statement == 0);
    D_TESTS_POSTGRES_CPP_CHECK(d_postgres_finalize(statement) == D_DB_OK);

    return;
}

}  // namespace

/*
main
  The server-free checks, then a session where a server is reached; or,
over a layer built without libpq -- whose version is then 0 -- the stubs'
checks.
*/
int
main(
    void
)
{
    const bool stubs    = (d_postgres_client_version_number() == 0);
    const bool required = (std::strcmp(
        d_tests_postgres_cpp_setting("DJINTERP_TEST_POSTGRES_REQUIRED", "0"),
        "1") == 0);
    const struct d_postgres_options options =
    {
        d_tests_postgres_cpp_setting("DJINTERP_TEST_POSTGRES_HOST",
                                     "127.0.0.1"),
        d_tests_postgres_cpp_setting("DJINTERP_TEST_POSTGRES_USER",
                                     "postgres"),
        d_tests_postgres_cpp_setting("DJINTERP_TEST_POSTGRES_PASSWORD", 0),
        d_tests_postgres_cpp_setting("DJINTERP_TEST_POSTGRES_DATABASE",
                                     "postgres"),
        static_cast<unsigned>(std::strtoul(
            d_tests_postgres_cpp_setting("DJINTERP_TEST_POSTGRES_PORT",
                                         "5432"),
            0,
            10)),
        0
    };
    struct pg_conn* db = 0;

    d_tests_postgres_cpp_serverless(stubs);

    // the stubs, or a session where a server answers
    if (stubs)
    {
        d_tests_postgres_cpp_stubs();
    }
    else if (d_postgres_connect(&options, &db, 0) == D_DB_OK)
    {
        d_tests_postgres_cpp_session(db);
        d_postgres_close(db);
    }
    else
    {
        std::printf("  no server reached; the server-free checks only\n");
        D_TESTS_POSTGRES_CPP_CHECK(!required);
    }

    std::printf("%d checks, %d failed (%s %d, as C++ %ld)\n",
                g_checks,
                g_failures,
                (stubs) ? "built without libpq:" : "libpq",
                d_postgres_client_version_number(),
                static_cast<long>(__cplusplus));

    return (g_failures == 0) ? 0 : 1;
}

#else

/*
main
  Below the 64-bit floor the layer is absent, and there is nothing to test.
*/
int
main(
    void
)
{
    std::printf("the PostgreSQL layer is absent below the 64-bit floor\n");

    return 0;
}

#endif  // defined(INT64_MAX)
