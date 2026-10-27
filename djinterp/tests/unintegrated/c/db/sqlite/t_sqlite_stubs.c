/*******************************************************************************
* djinterp [c]                                                  t_sqlite_stubs.c
*
*   The SQLite layer built without SQLite (D_CFG_SQLITE 0): every function
* answers as such a build must -- D_DB_UNSUPPORTED, or false, 0 or NULL where
* it cannot fail -- writes the answers it promises even on failure, and leaves
* every other output alone. Building and linking it at all proves the stubs
* need neither sqlite3.h nor the library.
*
*   Build (from the repo root), at any language level from C99, with no SQLite
* -- or let ci/check_db_sqlite.sh do it, given `stubs`:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc -DD_CFG_SQLITE=0               \
*        src/djinterp/c/db/db_*.c src/djinterp/c/db/sqlite/sqlite_[a-z]*.c    \
*        tests/unintegrated/c/db/sqlite/t_sqlite_stubs.c -o t_sqlite_stubs
*
*
* path:      /tests/unintegrated/c/db/sqlite/t_sqlite_stubs.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.26
*******************************************************************************/
// std
#include <stdio.h>   // printf
#include <string.h>  // memset, strcmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/sqlite/sqlite.h"  // unit under test
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, int64_t


//==============================================================================
// HARNESS
//==============================================================================

// g_checks, g_failures
//   counters: the checks run, and those that failed.
static int g_checks   = 0;
static int g_failures = 0;

/*
d_tests_sqlite_stubs_expect
  Counts one check, and prints it when it fails.
*/
static void
d_tests_sqlite_stubs_expect(
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

// D_TESTS_SQLITE_STUBS_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_SQLITE_STUBS_CHECK(_cond)                                     \
    d_tests_sqlite_stubs_expect((_cond), #_cond, __LINE__)

// D_TESTS_SQLITE_STUBS_UNSUPPORTED
//   macro: checks that a call reports D_DB_UNSUPPORTED, recording the call's
// own text and line, which again only the preprocessor can supply.
#define D_TESTS_SQLITE_STUBS_UNSUPPORTED(_call)                               \
    d_tests_sqlite_stubs_expect(((_call) == D_DB_UNSUPPORTED),                \
                                #_call,                                       \
                                __LINE__)

//==============================================================================
// TESTS
//==============================================================================

/*
d_tests_sqlite_stubs_common
  The library queries.
  Tests the following:
  - the version is the empty string and 0; no version is reached; no feature
    is supported
  - the layer's own results keep their messages; anything else is unknown
  - no connection has an error message
  - db_common.h's rows need no SQLite: a conversion converts, and empty
    rows have no value and are released
*/
static void
d_tests_sqlite_stubs_common(
    void
)
{
    int               marker = 0;
    int64_t           number = 0;
    struct d_db_rows  rows   = { 0u, 0u, NULL, NULL };
    struct d_db_value value  = { D_DB_VALUE_NULL, 0u, { 0 } };

    printf("sqlite_common\n");

    D_TESTS_SQLITE_STUBS_CHECK(strcmp(d_sqlite_library_version(), "") == 0);
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_library_version_number() == 0);
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_header_version_number() == 0);
    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_library_at_least(0, 0, 0));
    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_library_at_least(3, 7, 17));

    for (int f = D_SQLITE_FEATURE_PARTIAL_INDEX;
         f <= D_SQLITE_FEATURE_LOAD_EXTENSION;
         ++f)
    {
        D_TESTS_SQLITE_STUBS_CHECK(
            !d_sqlite_supports((enum d_sqlite_feature)f));
    }

    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_library_threadsafe() == 0);

    D_TESTS_SQLITE_STUBS_CHECK(
        strcmp(d_sqlite_result_message(D_DB_EXISTS), "already exists") == 0);
    D_TESTS_SQLITE_STUBS_CHECK(
        strcmp(d_sqlite_result_message(D_DB_NOT_FOUND), "not found") == 0);
    D_TESTS_SQLITE_STUBS_CHECK(
        strstr(d_sqlite_result_message(D_DB_UNSUPPORTED), "no SQLite") !=
        NULL);
    D_TESTS_SQLITE_STUBS_CHECK(
        strcmp(d_sqlite_result_message(D_DB_MISMATCH),
               d_db_status_message(D_DB_MISMATCH)) == 0);
    D_TESTS_SQLITE_STUBS_CHECK(
        strcmp(d_sqlite_result_message(21), "unknown result") == 0);
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_error_message(NULL) == NULL);
    D_TESTS_SQLITE_STUBS_CHECK(
        d_sqlite_error_message((struct sqlite3*)(void*)&marker) == NULL);

    value.type    = D_DB_VALUE_TEXT;
    value.as.text = "12";
    value.size    = 2u;
    D_TESTS_SQLITE_STUBS_CHECK( (d_db_value_int64(&value, &number) == D_DB_OK)
                                && (number == 12) );
    D_TESTS_SQLITE_STUBS_CHECK(d_db_rows_value(&rows, 0u, 0u) == NULL);
    d_db_rows_free(&rows);

    return;
}

/*
d_tests_sqlite_stubs_connection
  Connections.
  Tests the following:
  - every open fails and leaves the handle NULL, whatever it held
  - closing NULL succeeds; the queries answer with nothing
  - no extension loads, and nothing is said of it
*/
static void
d_tests_sqlite_stubs_connection(
    void
)
{
    int             marker = 0;
    struct sqlite3* db     = (struct sqlite3*)&marker;

    printf("sqlite_connection\n");

    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_open("t.db", &db));
    D_TESTS_SQLITE_STUBS_CHECK(db == NULL);
    db = (struct sqlite3*)&marker;
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_open_readonly("t.db", &db));
    D_TESTS_SQLITE_STUBS_CHECK(db == NULL);
    db = (struct sqlite3*)&marker;
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_open_memory(&db));
    D_TESTS_SQLITE_STUBS_CHECK(db == NULL);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_open_memory(NULL));
    db = (struct sqlite3*)&marker;
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_open_with("t.db", D_SQLITE_OPEN_READONLY, NULL, &db));
    D_TESTS_SQLITE_STUBS_CHECK(db == NULL);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_open_with(NULL, 0u, NULL, NULL));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_apply_defaults(db));
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_close(NULL) == D_DB_OK);
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_database_path(db, NULL) == NULL);
    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_is_readonly(db, NULL));
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_last_insert_id(db) == 0);
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_changes(db) == 0);
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_total_changes(db) == 0);

    char* message = (char*)&marker;

    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_load_extension(db, "x", NULL, &message));
    D_TESTS_SQLITE_STUBS_CHECK(message == NULL);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_enable_load_extension(db, true));

    return;
}

/*
d_tests_sqlite_stubs_query
  Statements, counts, single values, rows and quoting.
  Tests the following:
  - every statement, counting, single-value and rows function fails, the
    count, value or rows untouched
  - quoting has no SQLite to quote with: NULL
*/
static void
d_tests_sqlite_stubs_query(
    void
)
{
    struct sqlite3*  db     = NULL;
    int64_t          number = 42;
    double           real   = 2.5;
    char*            text   = (char*)&number;
    struct d_db_rows rows   = { 7u, 7u, NULL, NULL };

    printf("sqlite_query\n");

    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_exec(db, "SELECT 1"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_execf(db, "SELECT %d", 1));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_query_int64(db, &number, "SELECT 1"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_query_int64f(db, &number, "SELECT %d", 1));
    D_TESTS_SQLITE_STUBS_CHECK(number == 42);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_query_double(db, &real, "SELECT 1"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_query_doublef(db, &real, "SELECT %d", 1));
    D_TESTS_SQLITE_STUBS_CHECK(real == 2.5);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_query_text(db, &text, "SELECT 1"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_query_textf(db, &text, "SELECT %d", 1));
    D_TESTS_SQLITE_STUBS_CHECK(text == (char*)&number);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_exec_changes(db, &number, "SELECT 1"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_execf_changes(db, &number, "SELECT %d", 1));
    D_TESTS_SQLITE_STUBS_CHECK(number == 42);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_query_rows(db, &rows, "SELECT 1"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_query_rowsf(db, &rows, "SELECT %d", 1));
    D_TESTS_SQLITE_STUBS_CHECK( (rows.column_count == 7u) &&
                                (rows.row_count == 7u) );
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_quote_identifier("a") == NULL);
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_quote_literal("a") == NULL);

    return;
}

/*
d_tests_sqlite_stubs_statement
  Prepared statements.
  Tests the following:
  - preparing fails, leaving the statement NULL as a failed prepare does
  - finalizing succeeds; the describers answer 0 and NULL
  - every bind, clear and run fails, the count and the rows untouched; so
    does every cursor call
*/
static void
d_tests_sqlite_stubs_statement(
    void
)
{
    struct sqlite3*      db        = NULL;
    int                  marker    = 0;
    struct sqlite3_stmt* statement = (struct sqlite3_stmt*)(void*)&marker;
    size_t               index     = 9u;
    int64_t              changes   = 42;
    struct d_db_rows     rows      = { 7u, 7u, NULL, NULL };

    printf("sqlite_statement\n");

    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_prepare(db, &statement, "SELECT 1"));
    D_TESTS_SQLITE_STUBS_CHECK(statement == NULL);
    statement = (struct sqlite3_stmt*)(void*)&marker;
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_preparef(db, &statement, "SELECT %d", 1));
    D_TESTS_SQLITE_STUBS_CHECK(statement == NULL);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_prepare(db, NULL, "SELECT 1"));

    statement = (struct sqlite3_stmt*)(void*)&marker;
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_finalize(NULL) == D_DB_OK);
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_parameter_count(statement) == 0u);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_parameter_index(statement, ":a", &index));
    D_TESTS_SQLITE_STUBS_CHECK(index == 9u);
    D_TESTS_SQLITE_STUBS_CHECK(d_sqlite_statement_connection(statement) ==
                               NULL);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_bind_null(statement, 1u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_bind_bool(statement, 1u, true));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_bind_int64(statement, 1u, 1));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_bind_double(statement, 1u, 1.0));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_bind_text(statement, 1u, "x"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_bind_blob(statement, 1u, "x", 1u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_clear_bindings(statement));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_execute_update(statement, &changes));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_execute_query(statement, &rows));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_open_cursor(statement));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_fetch_rows(statement, 2u, &rows));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_close_cursor(statement));
    D_TESTS_SQLITE_STUBS_CHECK(changes == 42);
    D_TESTS_SQLITE_STUBS_CHECK( (rows.column_count == 7u) &&
                                (rows.row_count == 7u) );

    return;
}

/*
d_tests_sqlite_stubs_database
  Attached databases, database files, and databases as bytes.
  Tests the following:
  - no database exists; has_database_s says so and fails
  - attaching, detaching and listing fail, the list untouched
  - creating and deleting a file fail; no file is a database
  - serializing and deserializing fail, nothing written
*/
static void
d_tests_sqlite_stubs_database(
    void
)
{
    struct sqlite3*       db     = NULL;
    struct d_db_name_list names  = { 7u, NULL };
    bool                  exists = true;

    printf("sqlite_database\n");

    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_has_database(db, "main"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_has_database_s(db, "main", &exists));
    D_TESTS_SQLITE_STUBS_CHECK(!exists);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_attach_database(db, "t.db", "aux"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_detach_database(db, "aux"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_list_databases(db, &names));
    D_TESTS_SQLITE_STUBS_CHECK(names.count == 7u);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_create_database_file("t_sqlite_stubs.db", 0u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_delete_database_file("t_sqlite_stubs.db", D_DB_IF_EXISTS));
    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_is_database_file("t_sqlite_stubs.db"));

    void*  bytes = NULL;
    size_t size  = 3u;

    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_serialize(db, NULL, &bytes, &size));
    D_TESTS_SQLITE_STUBS_CHECK( (bytes == NULL) &&
                                (size == 3u) );
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_deserialize(db, NULL, "x", 1u, false));

    return;
}

/*
d_tests_sqlite_stubs_tables
  Tables and columns.
  Tests the following:
  - no table or column exists; the _s forms say so and fail
  - every other table and column function fails, its output untouched
*/
static void
d_tests_sqlite_stubs_tables(
    void
)
{
    static const struct d_db_column column =
    {
        "a", D_DB_TYPE_INTEGER, 0u, NULL, 0u, 0u, NULL, 0u
    };
    struct sqlite3*         db      = NULL;
    struct d_db_name_list   names   = { 7u, NULL };
    struct d_db_column_list columns = { 7u, NULL };
    int64_t                 count   = 42;
    bool                    exists  = true;

    printf("sqlite_schema: tables and columns\n");

    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_has_table(db, NULL, "t"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_has_table_s(db, NULL, "t", &exists));
    D_TESTS_SQLITE_STUBS_CHECK(!exists);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_create_table(db, NULL, "t", &column, 1u, 0u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_create_table_sql(db, NULL, "t", "a", 0u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_drop_table(db, NULL, "t", 0u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_rename_table(db, NULL, "t", "u"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_truncate_table(db, NULL, "t"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_count_rows(db, NULL, "t", &count));
    D_TESTS_SQLITE_STUBS_CHECK(count == 42);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_list_tables(db, NULL, &names));
    D_TESTS_SQLITE_STUBS_CHECK(names.count == 7u);

    exists = true;
    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_has_column(db, NULL, "t", "a"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_has_column_s(db, NULL, "t", "a", &exists));
    D_TESTS_SQLITE_STUBS_CHECK(!exists);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_add_column(db, NULL, "t", &column));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_add_column_sql(db, NULL, "t", "b"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_rename_column(db, NULL, "t", "a", "b"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_drop_column(db, NULL, "t", "a"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_list_columns(db, NULL, "t", &columns));
    D_TESTS_SQLITE_STUBS_CHECK(columns.count == 7u);

    return;
}

/*
d_tests_sqlite_stubs_objects
  Indexes, views and triggers.
  Tests the following:
  - none exists; the _s forms say so and fail
  - every other function fails, its list untouched
*/
static void
d_tests_sqlite_stubs_objects(
    void
)
{
    struct sqlite3*       db     = NULL;
    struct d_db_name_list names  = { 7u, NULL };
    bool                  exists = true;

    printf("sqlite_schema: indexes, views, triggers\n");

    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_has_index(db, NULL, "i"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_has_index_s(db, NULL, "i", &exists));
    D_TESTS_SQLITE_STUBS_CHECK(!exists);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_create_index(db, NULL, "i", "t", "a", 0u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_drop_index(db, NULL, "i", 0u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_list_indexes(db, NULL, NULL, &names));

    exists = true;
    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_has_view(db, NULL, "v"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_has_view_s(db, NULL, "v", &exists));
    D_TESTS_SQLITE_STUBS_CHECK(!exists);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_create_view(db, NULL, "v", "SELECT 1", 0u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_drop_view(db, NULL, "v", 0u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_list_views(db, NULL, &names));

    exists = true;
    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_has_trigger(db, NULL, "g"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_has_trigger_s(db, NULL, "g", &exists));
    D_TESTS_SQLITE_STUBS_CHECK(!exists);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_create_trigger(db, NULL, "g", "AFTER INSERT ON t BEGIN END",
                                0u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_drop_trigger(db, NULL, "g", 0u));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_list_triggers(db, NULL, NULL, &names));
    D_TESTS_SQLITE_STUBS_CHECK(names.count == 7u);

    return;
}

/*
d_tests_sqlite_stubs_transaction
  Transactions and savepoints.
  Tests the following:
  - every function fails; no transaction is ever open
*/
static void
d_tests_sqlite_stubs_transaction(
    void
)
{
    struct sqlite3* db = NULL;

    printf("sqlite_transaction\n");

    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_begin_transaction(db, D_SQLITE_TRANSACTION_DEFERRED));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_commit(db));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_rollback(db));
    D_TESTS_SQLITE_STUBS_CHECK(!d_sqlite_in_transaction(db));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_create_savepoint(db, "s"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_release_savepoint(db, "s"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_rollback_to_savepoint(db, "s"));

    return;
}

/*
d_tests_sqlite_stubs_settings
  Settings.
  Tests the following:
  - every setter and getter fails, the getter's output untouched
*/
static void
d_tests_sqlite_stubs_settings(
    void
)
{
    struct sqlite3*            db      = NULL;
    bool                       enabled = true;
    int                        timeout = 42;
    enum d_sqlite_journal_mode mode    = D_SQLITE_JOURNAL_WAL;
    enum d_sqlite_synchronous  level   = D_SQLITE_SYNCHRONOUS_FULL;
    int32_t                    number  = 42;
    char*                      text    = NULL;

    printf("sqlite_settings\n");

    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_set_foreign_keys(db, false));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_get_foreign_keys(db, &enabled));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_set_busy_timeout(db, 5));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_get_busy_timeout(db, &timeout));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_set_journal_mode(db, NULL, D_SQLITE_JOURNAL_DELETE));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_get_journal_mode(db, NULL, &mode));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_set_synchronous(db, NULL, D_SQLITE_SYNCHRONOUS_OFF));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_get_synchronous(db, NULL, &level));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_set_user_version(db, NULL, 1));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_get_user_version(db, NULL, &number));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_set_application_id(db, NULL, 1));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_get_application_id(db, NULL, &number));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_set_pragma(db, NULL, "cache_size", "1"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_get_pragma(db, NULL, "cache_size", &text));
    D_TESTS_SQLITE_STUBS_CHECK( (enabled)                          &&
                                (timeout == 42)                    &&
                                (mode == D_SQLITE_JOURNAL_WAL)     &&
                                (level == D_SQLITE_SYNCHRONOUS_FULL) &&
                                (number == 42)                     &&
                                (text == NULL) );

    return;
}

/*
d_tests_sqlite_stubs_maintenance
  Upkeep, checkpoints and backups.
  Tests the following:
  - every function fails; a check's verdict is untouched
*/
static void
d_tests_sqlite_stubs_maintenance(
    void
)
{
    struct sqlite3* db = NULL;
    bool            ok = true;

    printf("sqlite_maintenance\n");

    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_vacuum(db, NULL));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_vacuum_into(db, NULL, "c.db"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_analyze(db, NULL));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_optimize(db, NULL));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_integrity_check(db, NULL, &ok));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(d_sqlite_quick_check(db, NULL, &ok));
    D_TESTS_SQLITE_STUBS_CHECK(ok);
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_checkpoint(db, NULL, D_SQLITE_CHECKPOINT_PASSIVE));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_copy_database(db, NULL, db, NULL));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_backup_to_file(db, NULL, "c.db"));
    D_TESTS_SQLITE_STUBS_UNSUPPORTED(
        d_sqlite_restore_from_file(db, NULL, "c.db"));

    return;
}

//==============================================================================
// DRIVER
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
    d_tests_sqlite_stubs_common();
    d_tests_sqlite_stubs_connection();
    d_tests_sqlite_stubs_query();
    d_tests_sqlite_stubs_statement();
    d_tests_sqlite_stubs_database();
    d_tests_sqlite_stubs_tables();
    d_tests_sqlite_stubs_objects();
    d_tests_sqlite_stubs_transaction();
    d_tests_sqlite_stubs_settings();
    d_tests_sqlite_stubs_maintenance();

    printf("%d checks, %d failed (built without SQLite)\n",
           g_checks,
           g_failures);

    return (g_failures == 0) ? 0 : 1;
}
