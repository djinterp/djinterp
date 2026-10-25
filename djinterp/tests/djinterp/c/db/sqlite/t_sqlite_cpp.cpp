/*******************************************************************************
* djinterp [c]                                                  t_sqlite_cpp.cpp
*
*   The SQLite layer as C++ sees it: one translation unit, compiled as C++ at
* every level from ISO strict C++98 to C++26, that includes the layer's
* umbrella and nothing of SQLite's, and drives a database through it. It
* proves the headers are clean C++ at each level (sqlite3.h is not: it spells
* `long long`), that struct d_db_column takes a positional initializer where
* designated ones do not exist, and that the extern "C" linkage meets the C
* objects the sources compile to.
*   Below the 64-bit floor the layer's headers compile to nothing, and so does
* this test, which then only reports that it had nothing to run. Over a layer
* built without SQLite (D_CFG_SQLITE 0), it checks that no connection opens
* and no statement prepares.
*
*   Build: compile the layer's C sources and SQLite as C, this file as C++, and
* link them -- ci/check_db_sqlite.sh does, at every level.
*
*
* path:      /tests/djinterp/c/db/sqlite/t_sqlite_cpp.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstring>  // std::strcmp
// djinterp
#include "../../../../../inc/djinterp/c/db/sqlite/sqlite.h"  // unit under test
// re_std
#include "../../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int64_t,
                                                         // INT64_MAX


#if defined(INT64_MAX)

namespace
{

int g_checks   = 0;
int g_failures = 0;

/*
d_tests_sqlite_cpp_expect
  Counts one check, and prints it when it fails.
*/
void
d_tests_sqlite_cpp_expect(
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

}  // namespace

// D_TESTS_SQLITE_CPP_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_SQLITE_CPP_CHECK(_cond)                                       \
    d_tests_sqlite_cpp_expect((_cond), #_cond, __LINE__)

namespace
{

/*
d_tests_sqlite_cpp_database
  Creates a table from column descriptions and a positional initializer,
looks at it through every kind of query, and changes it inside a transaction.
*/
void
d_tests_sqlite_cpp_database(
    void
)
{
    static const struct d_db_column columns[] =
    {
        { "id",   D_DB_TYPE_BIG_INTEGER, D_DB_COLUMN_PRIMARY_KEY, 0, 0u, 0u,
          0, 0u },
        { "name", D_DB_TYPE_STRING,      D_DB_COLUMN_NOT_NULL,    "'?'", 0u, 0u,
          0, 0u }
    };
    struct sqlite3*         db      = 0;
    struct d_db_name_list   names   = { 0u, 0 };
    struct d_db_column_list info    = { 0u, 0 };
    re_std::int64_t         count   = -1;
    char*                   text    = 0;

    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_open_memory(&db) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_create_table(db, 0, "people", columns, 2u,
                              D_DB_IF_NOT_EXISTS) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_has_table(db, 0, "people"));
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_has_column(db, "main", "people", "NAME"));
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_list_tables(db, 0, &names) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK( (names.count == 1u) &&
                              (std::strcmp(names.names[0], "people") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_list_columns(db, 0, "people", &info) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK( (info.count == 2u)                     &&
                              (info.columns[0].primary_key == 1)     &&
                              (info.columns[1].not_null) );
    d_db_column_list_free(&info);

    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_begin_transaction(db, D_SQLITE_TRANSACTION_IMMEDIATE) ==
        D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_execf(db, "INSERT INTO people(name) VALUES (%Q)", "O'Hara")
        == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_commit(db) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_count_rows(db, 0, "people", &count) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(count == 1);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_query_text(db, &text, "SELECT name FROM people") == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK( (text) && (std::strcmp(text, "O'Hara") == 0) );
    d_db_free(text);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_query_int64(db, &count, "SELECT 1 WHERE 0") ==
        D_DB_NOT_FOUND);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_close(db) == D_DB_OK);

    return;
}

/*
d_tests_sqlite_cpp_rows
  Counts changes, reads rows, and runs a prepared statement with a value of
every kind bound, reading the values back through the conversions.
*/
void
d_tests_sqlite_cpp_rows(
    void
)
{
    struct sqlite3*      db        = 0;
    struct sqlite3_stmt* statement = 0;
    struct d_db_rows     rows      = { 0u, 0u, 0, 0 };
    re_std::int64_t      changes   = -1;
    re_std::int64_t      number    = 0;
    double               real      = 0.0;
    bool                 truth     = false;
    std::size_t          index     = 0u;

    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_open_memory(&db) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_exec(db, "CREATE TABLE v(i INTEGER, r REAL, t TEXT, b BLOB, "
                          "f BOOLEAN, n)") == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_prepare(db, &statement,
                         "INSERT INTO v VALUES (?, ?, :t, ?, ?, ?)") ==
        D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_parameter_count(statement) == 6u);
    D_TESTS_SQLITE_CPP_CHECK(
        (d_sqlite_parameter_index(statement, ":t", &index) == D_DB_OK) &&
        (index == 3u));
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_statement_connection(statement) == db);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_bind_int64(statement, 1u, 42) ==
                             D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_bind_double(statement, 2u, 2.5) ==
                             D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_bind_text(statement, 3u, "x") ==
                             D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_bind_blob(statement, 4u, "\0", 1u) ==
                             D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_bind_bool(statement, 5u, true) ==
                             D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_bind_null(statement, 6u) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(
        (d_sqlite_execute_update(statement, &changes) == D_DB_OK) &&
        (changes == 1));
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_clear_bindings(statement) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_execute_query(statement, &rows) ==
                             D_DB_OK);
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_finalize(statement) == D_DB_OK);

    D_TESTS_SQLITE_CPP_CHECK(
        (d_sqlite_exec_changes(db, &changes, "UPDATE v SET n = 1") ==
         D_DB_OK) &&
        (changes == 2));
    D_TESTS_SQLITE_CPP_CHECK(
        (d_sqlite_execf_changes(db, &changes, "DELETE FROM %I WHERE i IS %s",
                                "v", "NULL") == D_DB_OK) &&
        (changes == 1));
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_query_rows(db, &rows, "SELECT i, r, t, b, f, n FROM v") ==
        D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK( (rows.column_count == 6u) &&
                              (rows.row_count == 1u)    &&
                              (rows.columns[4].type == D_DB_TYPE_BOOLEAN) );
    D_TESTS_SQLITE_CPP_CHECK(
        (d_db_value_int64(d_db_rows_value(&rows, 0u, 0u), &number) ==
         D_DB_OK) &&
        (number == 42));
    D_TESTS_SQLITE_CPP_CHECK(
        (d_db_value_double(d_db_rows_value(&rows, 0u, 1u), &real) ==
         D_DB_OK) &&
        (real == 2.5));
    D_TESTS_SQLITE_CPP_CHECK(
        (d_db_value_bool(d_db_rows_value(&rows, 0u, 4u), &truth) ==
         D_DB_OK) &&
        (truth));
    D_TESTS_SQLITE_CPP_CHECK(
        d_db_value_int64(d_db_rows_value(&rows, 0u, 2u), &number) ==
        D_DB_MISMATCH);
    D_TESTS_SQLITE_CPP_CHECK(
        (d_db_rows_column_index(&rows, "T", &index) == D_DB_OK) &&
        (index == 2u));
    D_TESTS_SQLITE_CPP_CHECK(d_db_rows_value(&rows, 0u, 3u)->size == 1u);
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_query_rowsf(db, &rows, "SELECT %Q", "y") == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(
        (rows.row_count == 1u) &&
        (std::strcmp(rows.values[0].as.text, "y") == 0));
    d_db_rows_free(&rows);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_preparef(db, &statement, "SELECT * FROM %I", "v") ==
        D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_finalize(statement) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_close(db) == D_DB_OK);

    return;
}

/*
d_tests_sqlite_cpp_stubs
  The layer built without SQLite: no connection opens, NULL closes, and
nothing exists.
*/
void
d_tests_sqlite_cpp_stubs(
    void
)
{
    struct sqlite3*      db        = 0;
    struct sqlite3_stmt* statement = 0;
    struct d_db_rows     rows      = { 0u, 0u, 0, 0 };

    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_open_memory(&db) == D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_CPP_CHECK(db == 0);
    D_TESTS_SQLITE_CPP_CHECK(!d_sqlite_has_table(db, 0, "people"));
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_query_rows(db, &rows, "SELECT 1") == D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_CPP_CHECK(
        d_sqlite_prepare(db, &statement, "SELECT 1") == D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_CPP_CHECK(statement == 0);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_finalize(statement) == D_DB_OK);
    D_TESTS_SQLITE_CPP_CHECK(d_sqlite_close(db) == D_DB_OK);

    return;
}

}  // namespace

/*
main
  Drives a database through the layer, or, when the layer was built without
SQLite -- whose version is then 0 -- checks that it says so.
*/
int
main(
    void
)
{
    const bool stubs = (d_sqlite_library_version_number() == 0);

    // a build without SQLite has its stubs to check, and nothing more
    if (stubs)
    {
        d_tests_sqlite_cpp_stubs();
    }
    else
    {
        d_tests_sqlite_cpp_database();
        d_tests_sqlite_cpp_rows();
    }

    std::printf("%d checks, %d failed (%s%s, as C++ %ld)\n",
                g_checks,
                g_failures,
                (stubs) ? "built without SQLite" : "SQLite ",
                d_sqlite_library_version(),
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
    std::printf("the SQLite layer is absent below the 64-bit floor\n");

    return 0;
}

#endif  // defined(INT64_MAX)
