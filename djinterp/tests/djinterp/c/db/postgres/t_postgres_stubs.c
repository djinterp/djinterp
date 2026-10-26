/*******************************************************************************
* djinterp [c]                                                t_postgres_stubs.c
*
*   The PostgreSQL layer built without libpq (D_CFG_POSTGRES 0): every function
* answers as such a build must -- D_DB_UNSUPPORTED, or false, 0 or NULL where
* it cannot fail -- writes the answers it promises even on failure, and
* leaves every other output alone. The three functions that read results
* need no library and work as they always do. Building and linking it at all
* proves the stubs need neither libpq-fe.h nor a library.
*
*   Build (from the repo root), at any language level from C99, without libpq
* -- or let ci/check_db_postgres.sh do it, given `stubs`:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc -DD_CFG_POSTGRES=0             \
*        src/djinterp/c/db/db_*.c src/djinterp/c/db/postgres/postgres_*.c     \
*        tests/djinterp/c/db/postgres/t_postgres_stubs.c -o t_postgres_stubs
*
*
* path:      /tests/djinterp/c/db/postgres/t_postgres_stubs.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.24
*******************************************************************************/
// std
#include <stdio.h>   // printf
#include <string.h>  // memset, strcmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/postgres/postgres.h"  // under test
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


//==============================================================================
// HARNESS
//==============================================================================

// g_checks, g_failures
//   counters: the checks run, and those that failed.
static int g_checks   = 0;
static int g_failures = 0;

// g_marker
//   object: whose address stands in for a connection, which a build without
// libpq can never make; the stubs never look behind it.
static int g_marker = 0;

/*
d_tests_postgres_stubs_expect
  Counts one check, and prints it when it fails.
*/
static void
d_tests_postgres_stubs_expect(
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

// D_TESTS_POSTGRES_STUBS_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_POSTGRES_STUBS_CHECK(_cond)                                   \
    d_tests_postgres_stubs_expect((_cond), #_cond, __LINE__)

// D_TESTS_POSTGRES_STUBS_UNSUPPORTED
//   macro: checks that a call reports D_DB_UNSUPPORTED, recording the call's
// own text and line, which again only the preprocessor can supply.
#define D_TESTS_POSTGRES_STUBS_UNSUPPORTED(_call)                             \
    d_tests_postgres_stubs_expect(((_call) == D_DB_UNSUPPORTED),              \
                                  #_call,                                     \
                                  __LINE__)

//==============================================================================
// TESTS
//==============================================================================

/*
d_tests_postgres_stubs_common
  The library and server queries, and reading results.
  Tests the following:
  - no library: 0; no server: NULL, 0; no version reached; no feature
  - no connection's error to tell
  - results read as in any build: SQLSTATEs packed and spelled, the
    manual's words for them; D_DB_UNSUPPORTED says the build is why
*/
static void
d_tests_postgres_stubs_common(
    void
)
{
    struct pg_conn* const db          = (struct pg_conn*)(void*)&g_marker;
    char                  sqlstate[6] = "";

    printf("postgres_common\n");

    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_client_version_number() == 0);
    D_TESTS_POSTGRES_STUBS_CHECK(
        !d_postgres_client_supports(D_POSTGRES_CLIENT_PIPELINE));
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_server_version(db) == NULL);
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_server_version_number(db) == 0);
    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_server_at_least(db, 0u, 0u));

    for (int f = D_POSTGRES_FEATURE_PROCEDURES;
         f <= D_POSTGRES_FEATURE_JSONPATH;
         ++f)
    {
        D_TESTS_POSTGRES_STUBS_CHECK(
            !d_postgres_supports(db, (enum d_postgres_feature)f));
    }

    D_TESTS_POSTGRES_STUBS_CHECK(strcmp(d_postgres_error_message(db), "") ==
                                 0);
    D_TESTS_POSTGRES_STUBS_CHECK(
        d_postgres_parameter_status(db, "server_version") == NULL);
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_result_from_sqlstate("42P01") ==
                                 D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_STUBS_CHECK(
        (d_postgres_result_sqlstate(D_POSTGRES_SQLSTATE_UNIQUE_VIOLATION,
                                    sqlstate)) &&
        (strcmp(sqlstate, "23505") == 0));
    D_TESTS_POSTGRES_STUBS_CHECK(
        strcmp(d_postgres_result_message(D_POSTGRES_SQLSTATE_UNDEFINED_TABLE),
               "undefined table") == 0);
    D_TESTS_POSTGRES_STUBS_CHECK(
        strcmp(d_postgres_result_message(D_DB_NOT_FOUND), "not found") == 0);
    D_TESTS_POSTGRES_STUBS_CHECK(
        strstr(d_postgres_result_message(D_DB_UNSUPPORTED), "no libpq") !=
        NULL);
    D_TESTS_POSTGRES_STUBS_CHECK(
        strcmp(d_postgres_result_message(D_DB_MISMATCH),
               d_db_status_message(D_DB_MISMATCH)) == 0);
    D_TESTS_POSTGRES_STUBS_CHECK(
        strcmp(d_postgres_result_message(
                   D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER),
               "undefined parameter") == 0);

    return;
}

/*
d_tests_postgres_stubs_connection
  Connecting, closing and liveness.
  Tests the following:
  - every connect is D_DB_UNSUPPORTED, the connection and the reason NULL
  - close takes NULL as always; anything else is no connection
  - ping is D_DB_UNSUPPORTED; there is no backend
*/
static void
d_tests_postgres_stubs_connection(
    void
)
{
    struct pg_conn* const     marker  = (struct pg_conn*)(void*)&g_marker;
    struct pg_conn*           db      = marker;
    char*                     reason  = (char*)(void*)&g_marker;
    struct d_postgres_options options =
    {
        "localhost", "user", NULL, NULL, 0u, NULL
    };
    const char* const         keywords[1] = { "application_name" };
    const char* const         values[1]   = { "d_tests" };

    printf("postgres_connection\n");

    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_connect(&options, &db,
                                                          &reason));
    D_TESTS_POSTGRES_STUBS_CHECK( (db == NULL) && (reason == NULL) );
    db     = marker;
    reason = (char*)(void*)&g_marker;
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_connect_string("host=x",
                                                                 &db,
                                                                 &reason));
    D_TESTS_POSTGRES_STUBS_CHECK( (db == NULL) && (reason == NULL) );
    db     = marker;
    reason = (char*)(void*)&g_marker;
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_connect_with("host=x",
                                                               keywords,
                                                               values,
                                                               1u,
                                                               &db,
                                                               &reason));
    D_TESTS_POSTGRES_STUBS_CHECK( (db == NULL) && (reason == NULL) );
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_connect(NULL, NULL, NULL));
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_close(NULL) == D_DB_OK);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_close(marker));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_ping(marker));
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_backend_pid(marker) == 0);

    return;
}

/*
d_tests_postgres_stubs_pending
  Whether a pending call was left as a finished one leaves it.
*/
static bool
d_tests_postgres_stubs_pending(
    const struct d_db_pending* _pending
)
{
    return ( (_pending->socket == D_DB_SOCKET_NONE) &&
             (_pending->events == 0u)               &&
             (_pending->timeout_ms == 0u)           &&
             (_pending->deadline == 0.0)            &&
             (_pending->state == 0u) );
}

/*
d_tests_postgres_stubs_nonblocking
  Connecting, resetting and cancelling without blocking, and the blocking
reset and cancel.
  Tests the following:
  - every start and continuation is D_DB_UNSUPPORTED, nothing pending, the
    connection, the request and the reason NULL where a start makes them
  - reset and cancel are D_DB_UNSUPPORTED; a cancel finishes NULL as always,
    and anything else is no request
*/
static void
d_tests_postgres_stubs_nonblocking(
    void
)
{
    struct pg_conn* const        marker  = (struct pg_conn*)(void*)&g_marker;
    struct pg_cancel_conn* const request =
        (struct pg_cancel_conn*)(void*)&g_marker;
    struct pg_conn*              db      = marker;
    struct pg_cancel_conn*       cancel  = request;
    char*                        reason  = (char*)(void*)&g_marker;
    struct d_db_pending          pending;
    const char* const            keywords[1] = { "application_name" };
    const char* const            values[1]   = { "d_tests" };

    printf("postgres_connection: without blocking\n");

    memset(&pending, 0x5a, sizeof(pending));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_connect_start(NULL,
                                                                &db,
                                                                &pending,
                                                                &reason));
    D_TESTS_POSTGRES_STUBS_CHECK( (db == NULL)     &&
                                  (reason == NULL) &&
                                  (d_tests_postgres_stubs_pending(&pending)) );
    db = marker;
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_connect_string_start(
                                           "host=x",
                                           &db,
                                           &pending,
                                           NULL));
    D_TESTS_POSTGRES_STUBS_CHECK(db == NULL);
    db = marker;
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_connect_with_start(
                                           "host=x",
                                           keywords,
                                           values,
                                           1u,
                                           &db,
                                           &pending,
                                           NULL));
    D_TESTS_POSTGRES_STUBS_CHECK(db == NULL);
    db = marker;
    memset(&pending, 0x5a, sizeof(pending));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_connect_continue(
                                           &db,
                                           D_DB_WAIT_READ,
                                           &pending,
                                           NULL));
    D_TESTS_POSTGRES_STUBS_CHECK( (db == marker) &&
                                  (d_tests_postgres_stubs_pending(&pending)) );
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_reconnect(marker));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_reconnect_start(marker,
                                                              &pending));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_reconnect_continue(marker,
                                                                 0u,
                                                                 &pending));
    D_TESTS_POSTGRES_STUBS_CHECK(d_tests_postgres_stubs_pending(&pending));
    reason = (char*)(void*)&g_marker;
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_cancel(marker, &reason));
    D_TESTS_POSTGRES_STUBS_CHECK(reason == NULL);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_cancel_start(marker,
                                                               &cancel,
                                                               &pending,
                                                               NULL));
    D_TESTS_POSTGRES_STUBS_CHECK(cancel == NULL);
    cancel = request;
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_cancel_continue(&cancel,
                                                                  0u,
                                                                  &pending,
                                                                  NULL));
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_cancel_finish(NULL) == D_DB_OK);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_cancel_finish(request));

    return;
}

/*
d_tests_postgres_stubs_sent_and_statements
  Statements sent, results read, and prepared statements, without blocking.
  Tests the following:
  - every start and continuation is D_DB_UNSUPPORTED, nothing pending, and
    the statement a prepare's start would make NULL
*/
static void
d_tests_postgres_stubs_sent_and_statements(
    void
)
{
    struct pg_conn* const              marker    =
        (struct pg_conn*)(void*)&g_marker;
    struct d_postgres_statement* const fake      =
        (struct d_postgres_statement*)(void*)&g_marker;
    struct d_postgres_statement*       statement = fake;
    struct d_db_rows                   rows      = { 0u, 0u, NULL, NULL };
    struct d_db_pending                pending;
    int64_t                            changes   = 7;

    printf("postgres_protocol, postgres_statement: without blocking\n");

    memset(&pending, 0x5a, sizeof(pending));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_send_query_start(marker,
                                                                   "SELECT 1",
                                                                   &pending));
    D_TESTS_POSTGRES_STUBS_CHECK(d_tests_postgres_stubs_pending(&pending));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_send_query_continue(
                                           marker,
                                           0u,
                                           &pending));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_next_result_start(marker,
                                                                    &pending,
                                                                    &rows,
                                                                    &changes));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_next_result_continue(
                                           marker,
                                           0u,
                                           &pending,
                                           &rows,
                                           &changes));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_prepare_start(marker,
                                                                &statement,
                                                                "SELECT 1",
                                                                &pending));
    D_TESTS_POSTGRES_STUBS_CHECK(statement == NULL);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_prepare_continue(fake,
                                                                   0u,
                                                                   &pending));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_execute_start(fake,
                                                                &pending));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_execute_update_continue(
                                           fake,
                                           0u,
                                           &pending,
                                           &changes));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_execute_query_continue(
                                           fake,
                                           0u,
                                           &pending,
                                           &rows));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_open_cursor_start(fake,
                                                                    &pending));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_open_cursor_continue(
                                           fake,
                                           0u,
                                           &pending));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_fetch_rows_start(fake,
                                                                   1u,
                                                                   &pending,
                                                                   &rows));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_fetch_rows_continue(
                                           fake,
                                           0u,
                                           &pending,
                                           &rows));
    D_TESTS_POSTGRES_STUBS_CHECK( (d_tests_postgres_stubs_pending(&pending)) &&
                                  (changes == 7) );

    return;
}

/*
d_tests_postgres_stubs_query
  Statements, single values, rows and quoting.
  Tests the following:
  - every running, single-value and rows function is D_DB_UNSUPPORTED, its
    output untouched
  - quoting answers NULL
*/
static void
d_tests_postgres_stubs_query(
    void
)
{
    struct pg_conn* const db    = (struct pg_conn*)(void*)&g_marker;
    int64_t               i     = 7;
    double                d     = 7.0;
    char*                 text  = (char*)(void*)&g_marker;
    bool                  b     = true;
    struct d_db_rows      rows  = { 7u, 7u, NULL, NULL };

    printf("postgres_query\n");

    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_exec(db, "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_execf(db, "SELECT %d", 1));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_exec_changes(db, &i,
                                                               "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_execf_changes(db, &i,
                                                                "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_query_int64(db, &i,
                                                              "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_query_int64f(db, &i,
                                                               "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_query_double(db, &d,
                                                               "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_query_doublef(db, &d,
                                                                "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_query_text(db, &text,
                                                             "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_query_textf(db, &text,
                                                              "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_query_bool(db, &b,
                                                             "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_query_boolf(db, &b,
                                                              "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_query_rows(db, &rows,
                                                             "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_query_rowsf(db, &rows,
                                                              "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_query_rows_params(db, &rows, "SELECT $1", NULL, 0u));
    D_TESTS_POSTGRES_STUBS_CHECK( (i == 7) && (d == 7.0) && (b) &&
                                  (text == (char*)(void*)&g_marker) );
    D_TESTS_POSTGRES_STUBS_CHECK( (rows.column_count == 7u) &&
                                  (rows.row_count == 7u) );
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_quote_identifier("a") == NULL);
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_quote_literal(db, "a") == NULL);

    return;
}

/*
d_tests_postgres_stubs_statement
  Prepared statements.
  Tests the following:
  - preparing is D_DB_UNSUPPORTED, leaving the statement NULL as a failed
    prepare does
  - finalizing succeeds; the describers answer 0 and NULL; closing by name
    is D_DB_UNSUPPORTED
  - every bind, clear and run is D_DB_UNSUPPORTED, the count and the rows
    untouched
*/
static void
d_tests_postgres_stubs_statement(
    void
)
{
    struct pg_conn* const        db        =
        (struct pg_conn*)(void*)&g_marker;
    struct d_postgres_statement* statement =
        (struct d_postgres_statement*)(void*)&g_marker;
    int64_t                      changes   = 7;
    struct d_db_rows             rows      = { 7u, 7u, NULL, NULL };

    printf("postgres_statement\n");

    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_prepare(db, &statement, "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_CHECK(statement == NULL);
    statement = (struct d_postgres_statement*)(void*)&g_marker;
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_preparef(db, &statement, "SELECT %d", 1));
    D_TESTS_POSTGRES_STUBS_CHECK(statement == NULL);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_prepare(db, NULL, "SELECT 1"));

    statement = (struct d_postgres_statement*)(void*)&g_marker;
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_finalize(NULL) == 0);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_close_prepared(db, "p"));
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_parameter_count(statement) == 0u);
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_statement_connection(statement) ==
                                 NULL);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_bind_null(statement, 1u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_bind_bool(statement, 1u, true));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_bind_int64(statement, 1u, 1));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_bind_double(statement, 1u, 1.0));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_bind_text(statement, 1u, "x"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_bind_blob(statement, 1u, "x", 1u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_clear_bindings(statement));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_execute_update(statement, &changes));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_execute_query(statement, &rows));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_open_cursor(statement));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_fetch_rows(statement, 1u, &rows));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_close_cursor(statement));
    D_TESTS_POSTGRES_STUBS_CHECK(changes == 7);
    D_TESTS_POSTGRES_STUBS_CHECK( (rows.column_count == 7u) &&
                                  (rows.row_count == 7u) );

    return;
}

/*
d_tests_postgres_stubs_database
  Databases, schemas and extensions.
  Tests the following:
  - each has is false, each _s D_DB_UNSUPPORTED with its answer false
  - every create, drop, list and current is D_DB_UNSUPPORTED or NULL, a
    list untouched
*/
static void
d_tests_postgres_stubs_database(
    void
)
{
    struct pg_conn* const db     = (struct pg_conn*)(void*)&g_marker;
    struct d_db_name_list names  = { 7u, NULL };
    bool                  exists = true;
    char*                 name   = NULL;

    printf("postgres_database\n");

    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_has_database(db, "d"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_has_database_s(db, "d",
                                                                 &exists));
    D_TESTS_POSTGRES_STUBS_CHECK(!exists);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_create_database(db, "d", 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_drop_database(db, "d", 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_list_databases(db, &names));
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_current_database(db) == NULL);
    exists = true;
    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_has_schema(db, "s"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_has_schema_s(db, "s",
                                                               &exists));
    D_TESTS_POSTGRES_STUBS_CHECK(!exists);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_create_schema(db, "s", 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_drop_schema(db, "s", 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_list_schemas(db, &names));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_current_schema(db, &name));
    exists = true;
    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_has_extension(db, "e"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_has_extension_s(db, "e",
                                                                  &exists));
    D_TESTS_POSTGRES_STUBS_CHECK(!exists);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_create_extension(db, "e",
                                                                   NULL, 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_drop_extension(db, "e", 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_list_extensions(db, &names));
    D_TESTS_POSTGRES_STUBS_CHECK( (names.count == 7u) && (name == NULL) );

    return;
}

/*
d_tests_postgres_stubs_tables
  Tables and columns.
  Tests the following:
  - each has is false, each _s D_DB_UNSUPPORTED with its answer false
  - every other function is D_DB_UNSUPPORTED, its outputs untouched
*/
static void
d_tests_postgres_stubs_tables(
    void
)
{
    struct pg_conn* const    db      = (struct pg_conn*)(void*)&g_marker;
    struct d_db_name_list    names   = { 7u, NULL };
    struct d_db_column_list  columns = { 7u, NULL };
    bool                     exists  = true;
    int64_t                  count   = 7;
    const struct d_db_column column  =
    {
        "a", D_DB_TYPE_INTEGER, 0u, NULL, 0u, 0u, NULL, 0u
    };

    printf("postgres_schema: tables and columns\n");

    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_has_table(db, NULL, "t"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_has_table_s(db, NULL, "t",
                                                              &exists));
    D_TESTS_POSTGRES_STUBS_CHECK(!exists);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_create_table(db, NULL, "t",
                                                               &column, 1u,
                                                               0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_create_table_sql(db, NULL,
                                                                   "t", "a int",
                                                                   0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_drop_table(db, NULL, "t",
                                                             0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_rename_table(db, NULL, "t",
                                                               "u"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_truncate_table(db, NULL,
                                                                 "t"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_count_rows(db, NULL, "t",
                                                             &count));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_list_tables(db, NULL,
                                                              &names));
    exists = true;
    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_has_column(db, NULL, "t", "a"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_has_column_s(db, NULL, "t",
                                                               "a", &exists));
    D_TESTS_POSTGRES_STUBS_CHECK(!exists);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_add_column(db, NULL, "t",
                                                             &column));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_add_column_sql(db, NULL, "t",
                                                                 "a int"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_rename_column(db, NULL, "t",
                                                                "a", "b"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_drop_column(db, NULL, "t",
                                                              "a"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_list_columns(db, NULL, "t",
                                                               &columns));
    D_TESTS_POSTGRES_STUBS_CHECK( (names.count == 7u)   &&
                                  (columns.count == 7u) &&
                                  (count == 7) );

    return;
}

/*
d_tests_postgres_stubs_objects
  Indexes, views, triggers and sequences.
  Tests the following:
  - each has is false, each _s D_DB_UNSUPPORTED with its answer false
  - every other function is D_DB_UNSUPPORTED, its outputs untouched
*/
static void
d_tests_postgres_stubs_objects(
    void
)
{
    struct pg_conn* const db     = (struct pg_conn*)(void*)&g_marker;
    struct d_db_name_list names  = { 7u, NULL };
    bool                  exists = true;
    int64_t               value  = 7;

    printf("postgres_schema: indexes, views, triggers and sequences\n");

    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_has_index(db, NULL, "i"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_has_index_s(db, NULL, "i",
                                                              &exists));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_create_index(db, NULL, "i",
                                                               "t", "a", 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_drop_index(db, NULL, "i",
                                                             0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_list_indexes(db, NULL, NULL,
                                                               &names));
    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_has_view(db, NULL, "v"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_has_view_s(db, NULL, "v",
                                                             &exists));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_create_view(db, NULL, "v",
                                                              "SELECT 1", 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_drop_view(db, NULL, "v",
                                                            0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_list_views(db, NULL,
                                                             &names));
    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_has_trigger(db, NULL, "g", "t"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_has_trigger_s(db, NULL, "g",
                                                                "t", &exists));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_create_trigger(db, "g", "x",
                                                                 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_drop_trigger(db, NULL, "g",
                                                               "t", 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_list_triggers(db, NULL, NULL,
                                                                &names));
    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_has_sequence(db, NULL, "s"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_has_sequence_s(db, NULL, "s",
                                                                 &exists));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_create_sequence(db, NULL,
                                                                  "s", 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_drop_sequence(db, NULL, "s",
                                                                0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_list_sequences(db, NULL,
                                                                 &names));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_next_value(db, NULL, "s",
                                                             &value));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_last_value(db, &value));
    D_TESTS_POSTGRES_STUBS_CHECK( (!exists) && (names.count == 7u) &&
                                  (value == 7) );

    return;
}

/*
d_tests_postgres_stubs_transaction
  Transactions and savepoints.
  Tests the following:
  - beginning, ending and every savepoint function are D_DB_UNSUPPORTED
  - no transaction is open, failed or not, and no state is known
*/
static void
d_tests_postgres_stubs_transaction(
    void
)
{
    struct pg_conn* const db = (struct pg_conn*)(void*)&g_marker;

    printf("postgres_transaction\n");

    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_begin_transaction(db, D_POSTGRES_TRANSACTION_DEFAULT));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_begin_transaction(db,
                                     D_POSTGRES_TRANSACTION_READ_COMMITTED));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_commit(db));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_rollback(db));
    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_in_transaction(db));
    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_in_failed_transaction(db));
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_transaction_status(db) ==
                                 D_POSTGRES_STATE_UNKNOWN);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_create_savepoint(db, "s"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_release_savepoint(db, "s"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_rollback_to_savepoint(db,
                                                                        "s"));

    return;
}

/*
d_tests_postgres_stubs_settings
  Session settings.
  Tests the following:
  - every set_ and get_ is D_DB_UNSUPPORTED, a get_'s output untouched
*/
static void
d_tests_postgres_stubs_settings(
    void
)
{
    struct pg_conn* const     db    = (struct pg_conn*)(void*)&g_marker;
    char*                     text  = NULL;
    unsigned                  ms    = 7u;
    enum d_postgres_isolation level = D_POSTGRES_ISOLATION_SERIALIZABLE;

    printf("postgres_settings\n");

    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_set_search_path(db, "a"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_get_search_path(db, &text));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_set_statement_timeout(db,
                                                                        1u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_get_statement_timeout(db,
                                                                        &ms));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_set_lock_timeout(db, 1u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_get_lock_timeout(db, &ms));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_set_isolation_level(db,
                                       D_POSTGRES_ISOLATION_READ_COMMITTED));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_get_isolation_level(db,
                                                                      &level));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_set_client_encoding(db,
                                                                      "UTF8"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_get_client_encoding(db,
                                                                      &text));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_set_time_zone(db, "UTC"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_get_time_zone(db, &text));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_set_parameter(db, "a", "b"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_get_parameter(db, "a",
                                                                &text));
    D_TESTS_POSTGRES_STUBS_CHECK( (text == NULL) && (ms == 7u) &&
                                  (level ==
                                   D_POSTGRES_ISOLATION_SERIALIZABLE) );

    return;
}

/*
d_tests_postgres_stubs_maintenance
  Upkeep.
  Tests the following:
  - every function is D_DB_UNSUPPORTED, a size untouched
*/
static void
d_tests_postgres_stubs_maintenance(
    void
)
{
    struct pg_conn* const db    = (struct pg_conn*)(void*)&g_marker;
    int64_t               bytes = 7;

    printf("postgres_maintenance\n");

    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_vacuum(db, NULL, NULL, 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_analyze(db, NULL, NULL));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_reindex(db, NULL, "t", 0u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_checkpoint(db));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_table_size(db, NULL, "t",
                                                             &bytes));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_database_size(db, NULL,
                                                                &bytes));
    D_TESTS_POSTGRES_STUBS_CHECK(bytes == 7);

    return;
}

/*
d_tests_postgres_stubs_protocol
  COPY, notifications, statements sent, their rows a few at a time, and
pipelines.
  Tests the following:
  - every function is D_DB_UNSUPPORTED, writing nothing; nothing is busy,
    and no pipeline on
*/
static void
d_tests_postgres_stubs_protocol(
    void
)
{
    struct pg_conn* const db      = (struct pg_conn*)(void*)&g_marker;
    char*                 channel = (char*)(void*)&g_marker;
    char*                 payload = (char*)(void*)&g_marker;
    int                   pid     = 7;
    int64_t               changes = 7;
    unsigned int          oid     = 7u;
    struct d_db_rows      rows    = { 7u, 7u, NULL, NULL };

    printf("postgres_protocol\n");

    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_copy_in_start(db, "COPY t FROM STDIN"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_copy_in_data(db, "x", 1u));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_copy_in_end(db, NULL,
                                                              &changes));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_listen(db, "c"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_unlisten(db, NULL));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_notify(db, "c", "p"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_lo_import(db, "f", &oid));
    D_TESTS_POSTGRES_STUBS_CHECK(oid == 7u);
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_lo_export(db, 1u, "f"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(
        d_postgres_next_notification(db, &channel, &payload, &pid));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_send_query(db, "SELECT 1"));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_next_result(db, &rows,
                                                              &changes));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_consume_input(db));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_set_chunked_rows(db, 1));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_enter_pipeline(db));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_exit_pipeline(db));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_pipeline_sync(db));
    D_TESTS_POSTGRES_STUBS_UNSUPPORTED(d_postgres_pipeline_flush(db));
    D_TESTS_POSTGRES_STUBS_CHECK(d_postgres_pipeline_status(db) ==
                                 D_POSTGRES_PIPELINE_OFF);
    D_TESTS_POSTGRES_STUBS_CHECK(!d_postgres_is_busy(db));
    D_TESTS_POSTGRES_STUBS_CHECK( (channel == (char*)(void*)&g_marker) &&
                                  (payload == (char*)(void*)&g_marker) &&
                                  (pid == 7) && (changes == 7) &&
                                  (rows.column_count == 7u) );

    return;
}

//==============================================================================
// RUN
//==============================================================================

/*
main
  Runs every test and reports; the exit status is 0 only if every check
passed.
*/
int
main(
    void
)
{
    d_tests_postgres_stubs_common();
    d_tests_postgres_stubs_connection();
    d_tests_postgres_stubs_nonblocking();
    d_tests_postgres_stubs_sent_and_statements();
    d_tests_postgres_stubs_query();
    d_tests_postgres_stubs_statement();
    d_tests_postgres_stubs_database();
    d_tests_postgres_stubs_tables();
    d_tests_postgres_stubs_objects();
    d_tests_postgres_stubs_transaction();
    d_tests_postgres_stubs_settings();
    d_tests_postgres_stubs_maintenance();
    d_tests_postgres_stubs_protocol();

    printf("%d checks, %d failed (built without libpq)\n",
           g_checks,
           g_failures);

    return (g_failures == 0) ? 0 : 1;
}
