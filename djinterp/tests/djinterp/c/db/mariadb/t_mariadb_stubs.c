/*******************************************************************************
* djinterp [c]                                                 t_mariadb_stubs.c
*
*   The MariaDB layer built without a client library (D_CFG_MARIADB 0): every
* function answers as such a build must -- D_DB_UNSUPPORTED, or false, 0 or
* NULL where it cannot fail -- writes the answers it promises even on
* failure, and leaves every other output alone. Building and linking it at
* all proves the stubs need neither mysql.h nor a library.
*
*   Build (from the repo root), at any language level from C99, with no client
* -- or let ci/check_db_mariadb.sh do it, given `stubs`:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc -DD_CFG_MARIADB=0              \
*        src/djinterp/c/db/db_*.c src/djinterp/c/db/mariadb/mariadb_*.c       \
*        tests/djinterp/c/db/mariadb/t_mariadb_stubs.c -o t_mariadb_stubs
*
*
* path:      /tests/djinterp/c/db/mariadb/t_mariadb_stubs.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.23
*******************************************************************************/
// std
#include <stdio.h>   // printf
#include <string.h>  // strcmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/mariadb/mariadb.h"  // unit under test
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t, uint64_t


//==============================================================================
// HARNESS
//==============================================================================

// g_checks, g_failures
//   counters: the checks run, and those that failed.
static int g_checks   = 0;
static int g_failures = 0;

// g_marker
//   object: whose address stands in for a handle, which a build without a
// client can never make; the stubs never look behind it.
static int g_marker = 0;

/*
d_tests_mariadb_stubs_expect
  Counts one check, and prints it when it fails.
*/
static void
d_tests_mariadb_stubs_expect(
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

// D_TESTS_MARIADB_STUBS_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_MARIADB_STUBS_CHECK(_cond)                                    \
    d_tests_mariadb_stubs_expect((_cond), #_cond, __LINE__)

// D_TESTS_MARIADB_STUBS_UNSUPPORTED
//   macro: checks that a call reports D_DB_UNSUPPORTED, recording the call's
// own text and line, which again only the preprocessor can supply.
#define D_TESTS_MARIADB_STUBS_UNSUPPORTED(_call)                              \
    d_tests_mariadb_stubs_expect(((_call) == D_DB_UNSUPPORTED),               \
                                 #_call,                                      \
                                 __LINE__)

//==============================================================================
// TESTS
//==============================================================================

/*
d_tests_mariadb_stubs_common
  The library and server queries.
  Tests the following:
  - no client: the empty string, 0, not MariaDB's
  - no server: UNKNOWN, NULL, 0; no version reached; no feature supported
  - no native handle; no error; the layer's own results keep their messages,
    D_DB_UNSUPPORTED saying why
*/
static void
d_tests_mariadb_stubs_common(
    void
)
{
    struct d_mariadb* const db = (struct d_mariadb*)(void*)&g_marker;

    printf("mariadb_common\n");

    D_TESTS_MARIADB_STUBS_CHECK(strcmp(d_mariadb_client_version(), "") == 0);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_client_version_number() == 0ul);
    D_TESTS_MARIADB_STUBS_CHECK(!d_mariadb_client_is_mariadb());
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_server_flavor(db) ==
                                D_MARIADB_FLAVOR_UNKNOWN);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_server_version(db) == NULL);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_server_version_number(db) == 0ul);
    D_TESTS_MARIADB_STUBS_CHECK(
        !d_mariadb_server_at_least(db, D_MARIADB_FLAVOR_MARIADB, 0u, 0u, 0u));

    for (int f = D_MARIADB_FEATURE_CTE;
         f <= D_MARIADB_FEATURE_QUERY_ATTRIBUTES;
         ++f)
    {
        D_TESTS_MARIADB_STUBS_CHECK(
            !d_mariadb_supports(db, (enum d_mariadb_feature)f));
    }

    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_native(db) == NULL);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_error_code(db) == 0u);
    D_TESTS_MARIADB_STUBS_CHECK(strcmp(d_mariadb_error_message(db), "") == 0);
    D_TESTS_MARIADB_STUBS_CHECK(strcmp(d_mariadb_sqlstate(db), "") == 0);
    D_TESTS_MARIADB_STUBS_CHECK(
        strcmp(d_mariadb_result_message(D_DB_NOT_FOUND), "not found") == 0);
    D_TESTS_MARIADB_STUBS_CHECK(
        strcmp(d_mariadb_result_message(D_DB_INVALID_ARGUMENT),
               "invalid argument") == 0);
    D_TESTS_MARIADB_STUBS_CHECK(
        strstr(d_mariadb_result_message(D_DB_UNSUPPORTED),
               "no client library") != NULL);
    D_TESTS_MARIADB_STUBS_CHECK(
        strcmp(d_mariadb_result_message(D_DB_MISMATCH),
               d_db_status_message(D_DB_MISMATCH)) == 0);
    D_TESTS_MARIADB_STUBS_CHECK(
        strcmp(d_mariadb_result_message(1146), "unknown result") == 0);

    return;
}

/*
d_tests_mariadb_stubs_connection
  Connections.
  Tests the following:
  - connect and adopt fail, leaving NULL behind, connect_with's reason too
  - close accepts NULL, and refuses anything else, which no build made
  - changing user, resetting and ping fail; ids, counts and warnings are 0;
    no option reads back
*/
static void
d_tests_mariadb_stubs_connection(
    void
)
{
    struct d_mariadb* const db      = (struct d_mariadb*)(void*)&g_marker;
    struct d_mariadb*       made    = db;
    const struct d_mariadb_options options = { "localhost", NULL, NULL, NULL,
                                               0u, NULL };
    const struct d_mariadb_setting setting = { D_MARIADB_SETTING_COMPRESS, 1ul,
                                               NULL, NULL };
    char*                          status  = (char*)(void*)&g_marker;
    unsigned int                   number  = 0u;

    printf("mariadb_connection\n");

    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_connect(&options, &made));
    D_TESTS_MARIADB_STUBS_CHECK(made == NULL);
    made = db;
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_connect(NULL, &made));
    D_TESTS_MARIADB_STUBS_CHECK(made == NULL);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_connect(&options, NULL));
    made = db;
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_connect_with(&options, &setting, 1u, &made, &status));
    D_TESTS_MARIADB_STUBS_CHECK(made == NULL);
    D_TESTS_MARIADB_STUBS_CHECK(status == NULL);
    status = (char*)(void*)&g_marker;
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_connect_with(NULL, NULL, 1u, NULL, NULL));
    made = db;
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_adopt(&g_marker, true, &made));
    D_TESTS_MARIADB_STUBS_CHECK(made == NULL);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_close(NULL) == D_DB_OK);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_close(db));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_change_user(db, "user", "password", NULL));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_reset_connection(db));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_reset_connection(NULL));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_ping(db));
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_connection_id(db) == 0ul);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_server_status(db, &status));
    D_TESTS_MARIADB_STUBS_CHECK(status == (char*)(void*)&g_marker);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_last_insert_id(db) == 0u);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_changes(db) == 0);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_warning_count(db) == 0u);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_get_option(db, 0, &number));

    return;
}

/*
d_tests_mariadb_stubs_query
  Statements, counts, values, rows, quoting and query attributes.
  Tests the following:
  - every statement and query is D_DB_UNSUPPORTED, its count, value or rows
    untouched
  - quoting gives NULL
  - setting and clearing query attributes are D_DB_UNSUPPORTED
*/
static void
d_tests_mariadb_stubs_query(
    void
)
{
    struct d_mariadb* const db     = (struct d_mariadb*)(void*)&g_marker;
    int64_t                 number = 7;
    double                  real   = 7.0;
    char*                   text   = (char*)(void*)&g_marker;
    struct d_db_rows        rows   = { 7u, 7u, NULL, NULL };

    printf("mariadb_query\n");

    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_exec(db, "SELECT 1"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_execf(db, "SELECT %d", 1));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_exec_script(db, "SELECT 1"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_query_int64(db, &number, "SELECT 1"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_query_int64f(db, &number, "SELECT %d", 1));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_query_double(db, &real, "SELECT 1"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_query_doublef(db, &real, "SELECT %g", 1.0));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_query_text(db, &text, "SELECT 1"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_query_textf(db, &text, "SELECT %Q", "x"));
    D_TESTS_MARIADB_STUBS_CHECK(number == 7);
    D_TESTS_MARIADB_STUBS_CHECK(real == 7.0);
    D_TESTS_MARIADB_STUBS_CHECK(text == (char*)(void*)&g_marker);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_exec_changes(db, &number, "SELECT 1"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_execf_changes(db, &number, "SELECT %d", 1));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_query_rows(db, &rows, "SELECT 1"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_query_rowsf(db, &rows, "SELECT %d", 1));
    D_TESTS_MARIADB_STUBS_CHECK(number == 7);
    D_TESTS_MARIADB_STUBS_CHECK( (rows.column_count == 7u) &&
                                 (rows.row_count == 7u) );
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_quote_identifier("x") == NULL);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_quote_literal(db, "x") == NULL);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_set_query_attribute(db, "name", "value"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_clear_query_attributes(db));

    return;
}

/*
d_tests_mariadb_stubs_statement
  Prepared statements.
  Tests the following:
  - preparing is D_DB_UNSUPPORTED, leaving the statement NULL as a failed
    prepare does
  - finalizing succeeds; the describers answer 0 and NULL
  - every bind, clear and run is D_DB_UNSUPPORTED, the count and the rows
    untouched; so is every cursor call
*/
static void
d_tests_mariadb_stubs_statement(
    void
)
{
    struct d_mariadb* const     db        =
        (struct d_mariadb*)(void*)&g_marker;
    struct d_mariadb_statement* statement =
        (struct d_mariadb_statement*)(void*)&g_marker;
    int64_t                     changes   = 7;
    struct d_db_rows            rows      = { 7u, 7u, NULL, NULL };

    printf("mariadb_statement\n");

    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_prepare(db, &statement, "SELECT 1"));
    D_TESTS_MARIADB_STUBS_CHECK(statement == NULL);
    statement = (struct d_mariadb_statement*)(void*)&g_marker;
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_preparef(db, &statement, "SELECT %d", 1));
    D_TESTS_MARIADB_STUBS_CHECK(statement == NULL);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_prepare(db, NULL, "SELECT 1"));

    statement = (struct d_mariadb_statement*)(void*)&g_marker;
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_finalize(NULL) == 0);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_parameter_count(statement) == 0u);
    D_TESTS_MARIADB_STUBS_CHECK(d_mariadb_statement_connection(statement) ==
                                NULL);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_bind_null(statement, 1u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_bind_bool(statement, 1u, true));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_bind_int64(statement, 1u, 1));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_bind_double(statement, 1u, 1.0));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_bind_text(statement, 1u, "x"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_bind_blob(statement, 1u, "x", 1u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_clear_bindings(statement));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_execute_update(statement, &changes));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_execute_query(statement, &rows));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_open_cursor(statement));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_fetch_rows(statement, 2u, &rows));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_close_cursor(statement));
    D_TESTS_MARIADB_STUBS_CHECK(changes == 7);
    D_TESTS_MARIADB_STUBS_CHECK( (rows.column_count == 7u) &&
                                 (rows.row_count == 7u) );

    return;
}

/*
d_tests_mariadb_stubs_database
  Databases.
  Tests the following:
  - the predicates answer no, and the _s form writes it
  - everything else is D_DB_UNSUPPORTED, its outputs untouched
*/
static void
d_tests_mariadb_stubs_database(
    void
)
{
    struct d_mariadb* const db      = (struct d_mariadb*)(void*)&g_marker;
    struct d_db_name_list   names   = { 3u, NULL };
    char*                   current = (char*)(void*)&g_marker;
    bool                    exists  = true;

    printf("mariadb_database\n");

    D_TESTS_MARIADB_STUBS_CHECK(!d_mariadb_has_database(db, "mysql"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_has_database_s(db, "mysql", &exists));
    D_TESTS_MARIADB_STUBS_CHECK(!exists);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_has_database_s(db, "mysql", NULL));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_create_database(db, "x", 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_drop_database(db, "x", 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_list_databases(db, &names));
    D_TESTS_MARIADB_STUBS_CHECK(names.count == 3u);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_use_database(db, "x"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_current_database(db, &current));
    D_TESTS_MARIADB_STUBS_CHECK(current == (char*)(void*)&g_marker);

    return;
}

/*
d_tests_mariadb_stubs_tables
  Tables and columns.
  Tests the following:
  - the predicates answer no, and the _s forms write it
  - everything else is D_DB_UNSUPPORTED, its outputs untouched
*/
static void
d_tests_mariadb_stubs_tables(
    void
)
{
    struct d_mariadb* const  db      = (struct d_mariadb*)(void*)&g_marker;
    const struct d_db_column column  = { "a", D_DB_TYPE_INTEGER, 0u, NULL, 0u,
                                         0u, NULL, 0u };
    struct d_db_name_list    names   = { 3u, NULL };
    struct d_db_column_list  columns = { 3u, NULL };
    int64_t                  count   = 7;
    bool                     exists  = true;

    printf("mariadb_schema: tables and columns\n");

    D_TESTS_MARIADB_STUBS_CHECK(!d_mariadb_has_table(db, NULL, "t"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_has_table_s(db, NULL, "t", &exists));
    D_TESTS_MARIADB_STUBS_CHECK(!exists);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_create_table(db, NULL, "t", &column, 1u, 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_create_table_sql(db, NULL, "t", "a INT", 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_drop_table(db, NULL, "t", 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_rename_table(db, NULL, "t", "u"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_truncate_table(db, NULL, "t"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_count_rows(db, NULL, "t", &count));
    D_TESTS_MARIADB_STUBS_CHECK(count == 7);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_list_tables(db, NULL, &names));
    D_TESTS_MARIADB_STUBS_CHECK(names.count == 3u);

    exists = true;
    D_TESTS_MARIADB_STUBS_CHECK(!d_mariadb_has_column(db, NULL, "t", "a"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_has_column_s(db, NULL, "t", "a", &exists));
    D_TESTS_MARIADB_STUBS_CHECK(!exists);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_add_column(db, NULL, "t", &column));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_add_column_sql(db, NULL, "t", "b INT"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_modify_column(db, NULL, "t", &column));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_rename_column(db, NULL, "t", "a", "b"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_drop_column(db, NULL, "t", "a"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_list_columns(db, NULL, "t", &columns));
    D_TESTS_MARIADB_STUBS_CHECK(columns.count == 3u);

    return;
}

/*
d_tests_mariadb_stubs_objects
  Indexes, views and triggers.
  Tests the following:
  - the predicates answer no, and the _s forms write it
  - everything else is D_DB_UNSUPPORTED, its outputs untouched
*/
static void
d_tests_mariadb_stubs_objects(
    void
)
{
    struct d_mariadb* const db     = (struct d_mariadb*)(void*)&g_marker;
    struct d_db_name_list   names  = { 3u, NULL };
    bool                    exists = true;

    printf("mariadb_schema: indexes, views and triggers\n");

    D_TESTS_MARIADB_STUBS_CHECK(!d_mariadb_has_index(db, NULL, "i", "t"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_has_index_s(db, NULL, "i", "t", &exists));
    D_TESTS_MARIADB_STUBS_CHECK(!exists);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_create_index(db, NULL, "i", "t", "a", 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_drop_index(db, NULL, "i", "t", 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_list_indexes(db, NULL, "t", &names));

    exists = true;
    D_TESTS_MARIADB_STUBS_CHECK(!d_mariadb_has_view(db, NULL, "v"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_has_view_s(db, NULL, "v", &exists));
    D_TESTS_MARIADB_STUBS_CHECK(!exists);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_create_view(db, NULL, "v", "SELECT 1", 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_drop_view(db, NULL, "v", 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_list_views(db, NULL, &names));

    exists = true;
    D_TESTS_MARIADB_STUBS_CHECK(!d_mariadb_has_trigger(db, NULL, "tr"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_has_trigger_s(db, NULL, "tr", &exists));
    D_TESTS_MARIADB_STUBS_CHECK(!exists);
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_create_trigger(db, NULL, "tr",
                                 "BEFORE INSERT ON t FOR EACH ROW SET @a = 1",
                                 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_drop_trigger(db, NULL, "tr", 0u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_list_triggers(db, NULL, NULL, &names));
    D_TESTS_MARIADB_STUBS_CHECK(names.count == 3u);

    return;
}

/*
d_tests_mariadb_stubs_transaction
  Transactions and savepoints.
  Tests the following:
  - every verb is D_DB_UNSUPPORTED; no transaction is open
*/
static void
d_tests_mariadb_stubs_transaction(
    void
)
{
    struct d_mariadb* const db = (struct d_mariadb*)(void*)&g_marker;

    printf("mariadb_transaction\n");

    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_begin_transaction(db, D_MARIADB_TRANSACTION_READ_WRITE));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_commit(db));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_rollback(db));
    D_TESTS_MARIADB_STUBS_CHECK(!d_mariadb_in_transaction(db));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_create_savepoint(db, "s"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_release_savepoint(db, "s"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_rollback_to_savepoint(db, "s"));

    return;
}

/*
d_tests_mariadb_stubs_settings
  Session settings, and the session's changes.
  Tests the following:
  - every setter and getter is D_DB_UNSUPPORTED, the getters' outputs
    untouched
  - the session's changes are D_DB_UNSUPPORTED, the list untouched
*/
static void
d_tests_mariadb_stubs_settings(
    void
)
{
    struct d_mariadb* const  db      = (struct d_mariadb*)(void*)&g_marker;
    bool                     enabled = true;
    enum d_mariadb_isolation level   = D_MARIADB_ISOLATION_SERIALIZABLE;
    unsigned                 seconds = 7u;
    char*                    text    = (char*)(void*)&g_marker;
    struct d_db_name_list    entries = { 7u, NULL };

    printf("mariadb_settings\n");

    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_set_autocommit(db, false));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_get_autocommit(db, &enabled));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_set_isolation_level(db, D_MARIADB_ISOLATION_READ_COMMITTED));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_get_isolation_level(db, &level));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_set_next_isolation_level(db,
                                           D_MARIADB_ISOLATION_SERIALIZABLE));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_set_lock_wait_timeout(db, 5u));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_get_lock_wait_timeout(db, &seconds));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_set_charset(db, "utf8mb4"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_get_charset(db, &text));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_set_time_zone(db, "+00:00"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_get_time_zone(db, &text));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_set_sql_mode(db, ""));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_get_sql_mode(db, &text));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_set_variable(db, "wait_timeout", "10"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_get_variable(db, "wait_timeout", &text));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_session_track(db, D_MARIADB_TRACK_SCHEMA, &entries));
    D_TESTS_MARIADB_STUBS_CHECK(enabled);
    D_TESTS_MARIADB_STUBS_CHECK(level == D_MARIADB_ISOLATION_SERIALIZABLE);
    D_TESTS_MARIADB_STUBS_CHECK(seconds == 7u);
    D_TESTS_MARIADB_STUBS_CHECK(text == (char*)(void*)&g_marker);
    D_TESTS_MARIADB_STUBS_CHECK( (entries.count == 7u) &&
                                 (entries.names == NULL) );

    return;
}

/*
d_tests_mariadb_stubs_maintenance
  Upkeep.
  Tests the following:
  - every operation is D_DB_UNSUPPORTED, its answer untouched
*/
static void
d_tests_mariadb_stubs_maintenance(
    void
)
{
    struct d_mariadb* const db       = (struct d_mariadb*)(void*)&g_marker;
    bool                    ok       = true;
    uint64_t                checksum = 7u;

    printf("mariadb_maintenance\n");

    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_analyze_table(db, NULL, "t"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(d_mariadb_optimize_table(db, NULL, "t"));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_check_table(db, NULL, "t", &ok));
    D_TESTS_MARIADB_STUBS_UNSUPPORTED(
        d_mariadb_checksum_table(db, NULL, "t", &checksum));
    D_TESTS_MARIADB_STUBS_CHECK(ok);
    D_TESTS_MARIADB_STUBS_CHECK(checksum == 7u);

    return;
}

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
    d_tests_mariadb_stubs_common();
    d_tests_mariadb_stubs_connection();
    d_tests_mariadb_stubs_query();
    d_tests_mariadb_stubs_statement();
    d_tests_mariadb_stubs_database();
    d_tests_mariadb_stubs_tables();
    d_tests_mariadb_stubs_objects();
    d_tests_mariadb_stubs_transaction();
    d_tests_mariadb_stubs_settings();
    d_tests_mariadb_stubs_maintenance();

    printf("%d checks, %d failed (built without a client)\n",
           g_checks,
           g_failures);

    return (g_failures == 0) ? 0 : 1;
}
