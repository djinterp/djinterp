/*******************************************************************************
* djinterp [c]                                                t_db_face_cost.cpp
*
*   What a C++ face costs over the C layer it wraps: nothing. Each pair of
* functions here makes one call twice, once through a layer and once through
* its face -- a question, of each kind of server; a value written through a
* parameter; a column array; text owned and released; rows read, converted
* and released; a statement prepared, bound, run and finalized; a cursor
* opened, read and closed; a connection closed; from C++11 an f form, in
* each dialect, its literal format checked at compile time from C++20; from
* C++23 a std::expected result -- and
* scripts/check_db_face_cost.py compiles this file at -O2 with each compiler,
* at each level, and checks that the two halves of every pair compile to the
* same instructions. A connection reaches each half as a pointer to it: a
* pointer to the layer's handle, or a reference to the face, which holds one.
*   Nothing here runs: the file is compiled, never linked, and so needs no
* client library. Below the 64-bit floor the faces compile to nothing, and
* so does this file.
*
*
* path:      /tests/unintegrated/cpp/db/t_db_face_cost.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.28
*******************************************************************************/
// djinterp
#include "../../../../inc/djinterp/cpp/db/sqlite/sqlite_face.hpp"      // under
                                                                     // test
#include "../../../../inc/djinterp/cpp/db/mariadb/mariadb_face.hpp"    // under
                                                                     // test
#include "../../../../inc/djinterp/cpp/db/postgres/postgres_face.hpp"  // under
                                                                     // test
#include "../../../../inc/djinterp/cpp/db/mongodb/mongodb_face.hpp"    // under
                                                                     // test
// re_std
#include "../../../../inc/re_std/cpp/cstdint/cstdint.hpp"  // re_std::int64_t,
                                                      // INT64_MAX


#if defined(INT64_MAX)

namespace face = djinterp::face;

namespace
{

// g_columns
//   a table's columns, for the column-array pair.
const struct dj_db_column g_columns[] =
{
    { "id", D_DB_TYPE_BIG_INTEGER, D_DB_COLUMN_PRIMARY_KEY, 0, 0u, 0u, 0, 0u }
};

}  // namespace

// a question answered
bool d_tests_db_face_cost_c_has(
    struct sqlite3* const* _db
)
{
    return d_sqlite_has_table(*_db, 0, "t");
}

bool d_tests_db_face_cost_face_has(
    const face::sqlite& _db
)
{
    return _db.has_table(0, "t");
}

bool d_tests_db_face_cost_c_mariadb(
    struct dj_mariadb* const* _db
)
{
    return d_mariadb_has_table(*_db, 0, "t");
}

bool d_tests_db_face_cost_face_mariadb(
    const face::mariadb& _db
)
{
    return _db.has_table(0, "t");
}

bool d_tests_db_face_cost_c_postgres(
    struct pg_conn* const* _db
)
{
    return d_postgres_has_table(*_db, 0, "t");
}

bool d_tests_db_face_cost_face_postgres(
    const face::postgres& _db
)
{
    return _db.has_table(0, "t");
}

bool d_tests_db_face_cost_c_mongodb(
    struct dj_mongodb* const* _db
)
{
    return d_mongodb_has_collection(*_db, 0, "c");
}

bool d_tests_db_face_cost_face_mongodb(
    const face::mongodb& _db
)
{
    return _db.has_collection(0, "c");
}

// a value written through a parameter
int d_tests_db_face_cost_c_count(
    struct sqlite3* const* _db,
    re_std::int64_t*       _count
)
{
    return d_sqlite_query_int64(*_db, _count, "SELECT count(*) FROM t");
}

int d_tests_db_face_cost_face_count(
    face::sqlite&    _db,
    re_std::int64_t& _count
)
{
    return _db.query_int64(_count, "SELECT count(*) FROM t");
}

// a column array, its count deduced
int d_tests_db_face_cost_c_create(
    struct sqlite3* const* _db
)
{
    return d_sqlite_create_table(*_db, 0, "t", g_columns, 1u, 0u);
}

int d_tests_db_face_cost_face_create(
    face::sqlite& _db
)
{
    return _db.create_table(0, "t", g_columns, 0u);
}

// text owned, and released when it goes
bool d_tests_db_face_cost_c_text(
    struct sqlite3* const* _db
)
{
    char*      value  = D_NULLPTR;
    const int  result = d_sqlite_query_text(*_db, &value, "SELECT 'x'");
    const bool found  = ( (result == 0) &&
                          (value != D_NULLPTR) );

    // the text, when there is some
    if (value)
    {
        d_db_free(value);
    }

    return found;
}

bool d_tests_db_face_cost_face_text(
    face::sqlite& _db
)
{
    face::text value;
    const int  result = _db.query_text(value, "SELECT 'x'");

    return ( (result == 0) &&
             (!value.is_null()) );
}

// rows read, a value converted, and the rows released when they go
bool d_tests_db_face_cost_c_rows(
    struct sqlite3* const* _db,
    re_std::int64_t*       _first
)
{
    struct dj_db_rows found  = { 0u, 0u, D_NULLPTR, D_NULLPTR };
    const int        result = d_sqlite_query_rows(*_db, &found,
                                                  "SELECT a FROM t");
    const bool       read   = ( (result == 0) &&
                                (d_db_value_int64(
                                     d_db_rows_value(&found, 0u, 0u),
                                     _first) == 0) );

    // the rows, when there are some
    if (found.columns)
    {
        d_db_rows_free(&found);
    }

    return read;
}

bool d_tests_db_face_cost_face_rows(
    face::sqlite&    _db,
    re_std::int64_t& _first
)
{
    face::rows found;
    const int  result = _db.query_rows(found, "SELECT a FROM t");

    return ( (result == 0) &&
             (face::value_int64(found.value(0u, 0u), _first) == 0) );
}

// a statement prepared, bound, run, and finalized when it goes
int d_tests_db_face_cost_c_statement(
    struct pg_conn* const* _db,
    re_std::int64_t        _value,
    re_std::int64_t*       _changes
)
{
    struct dj_postgres_statement* statement = D_NULLPTR;
    int                          result    = d_postgres_prepare(
        *_db, &statement, "INSERT INTO t VALUES ($1)");

    // bound and run, once prepared
    if (result == 0)
    {
        result = d_postgres_bind_int64(statement, 1u, _value);

        if (result == 0)
        {
            result = d_postgres_execute_update(statement, _changes);
        }
    }

    // the statement, when there is one
    if (statement)
    {
        (void)d_postgres_finalize(statement);
    }

    return result;
}

int d_tests_db_face_cost_face_statement(
    face::postgres&  _db,
    re_std::int64_t  _value,
    re_std::int64_t& _changes
)
{
    face::postgres_statement statement;
    int                      result = _db.prepare(
        statement, "INSERT INTO t VALUES ($1)");

    // bound and run, once prepared
    if (result == 0)
    {
        result = statement.bind_int64(1u, _value);

        if (result == 0)
        {
            result = statement.execute_update(_changes);
        }
    }

    return result;
}

// a cursor opened, a page read, and both released when they go
int d_tests_db_face_cost_c_cursor(
    struct dj_mongodb* const* _db,
    std::size_t*              _seen
)
{
    struct dj_mongodb_cursor* cursor = D_NULLPTR;
    int                      result = d_mongodb_find_cursor(*_db, 0, "c", 0,
                                                            0, &cursor);

    // a page, once opened, released when read
    if (result == 0)
    {
        struct dj_db_rows page = { 0u, 0u, D_NULLPTR, D_NULLPTR };

        result = d_mongodb_cursor_fetch(cursor, 10u, &page);
        *_seen = page.row_count;

        // the page, when there is one
        if (page.columns)
        {
            d_db_rows_free(&page);
        }
    }

    // the cursor, when there is one
    if (cursor)
    {
        (void)d_mongodb_cursor_close(cursor);
    }

    return result;
}

int d_tests_db_face_cost_face_cursor(
    face::mongodb& _db,
    std::size_t&   _seen
)
{
    face::mongodb_cursor cursor;
    int                  result = _db.find_cursor(0, "c", 0, 0, cursor);

    // a page, once opened, released when read
    if (result == 0)
    {
        face::rows page;

        result = cursor.fetch(10u, page);
        _seen  = page.row_count();
    }

    return result;
}

// a connection closed when it goes
void d_tests_db_face_cost_c_close(
    struct sqlite3* _db
)
{
    // a connection, when there is one
    if (_db)
    {
        (void)d_sqlite_close(_db);
    }

    return;
}

void d_tests_db_face_cost_face_close(
    struct sqlite3* _db
)
{
    face::sqlite owned(_db);

    return;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// an f form, in sqlite3_mprintf's dialect and in the shared one; from C++20
// each literal is read against its arguments at compile time, and leaves
// nothing behind
int d_tests_db_face_cost_c_execf(
    struct sqlite3* const* _db,
    int                    _number,
    const char*            _text
)
{
    return d_sqlite_execf(*_db, "INSERT INTO t VALUES (%d, %Q)", _number,
                          _text);
}

int d_tests_db_face_cost_face_execf(
    face::sqlite& _db,
    int           _number,
    const char*   _text
)
{
    return _db.execf("INSERT INTO t VALUES (%d, %Q)", _number, _text);
}

int d_tests_db_face_cost_c_shared(
    struct pg_conn* const* _db,
    long long              _number
)
{
    return d_postgres_execf(*_db, "SELECT %lld FROM %I", _number, "t");
}

int d_tests_db_face_cost_face_shared(
    face::postgres& _db,
    long long       _number
)
{
    return _db.execf("SELECT %lld FROM %I", _number, "t");
}

int d_tests_db_face_cost_c_json(
    struct dj_mongodb* const* _db,
    int                       _number,
    const char*               _text
)
{
    return d_mongodb_execf(*_db, 0, "{\"insert\": %I, \"documents\": "
                                    "[{\"n\": %d, \"t\": %Q}]}",
                           "c", _number, _text);
}

int d_tests_db_face_cost_face_json(
    face::mongodb& _db,
    int            _number,
    const char*    _text
)
{
    return _db.execf(0, "{\"insert\": %I, \"documents\": [{\"n\": %d, "
                        "\"t\": %Q}]}",
                     "c", _number, _text);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED

// the value, or the layer's result
std::expected<re_std::int64_t, int> d_tests_db_face_cost_c_expected(
    struct sqlite3* const* _db
)
{
    re_std::int64_t value  = 0;
    const int       result = d_sqlite_query_int64(*_db, &value,
                                                  "SELECT count(*) FROM t");

    // the value, or the layer's result when it failed
    if (result != 0)
    {
        return std::unexpected(result);
    }

    return value;
}

std::expected<re_std::int64_t, int> d_tests_db_face_cost_face_expected(
    face::sqlite& _db
)
{
    return _db.query_int64("SELECT count(*) FROM t");
}

#endif  // D_INTERNAL_DB_FACE_EXPECTED

#endif  // defined(INT64_MAX)
