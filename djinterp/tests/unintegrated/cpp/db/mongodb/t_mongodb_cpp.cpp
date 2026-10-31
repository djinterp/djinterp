/*******************************************************************************
* djinterp [c]                                                 t_mongodb_cpp.cpp
*
*   The MongoDB layer as C++ sees it: one translation unit, compiled as C++ at
* every level from ISO strict C++98 to C++26, that includes the layer's
* umbrella and nothing of the driver's, and drives a deployment through it.
* It proves the headers are clean C++ at each level, without mongoc.h or
* bson.h; that struct dj_mongodb_options, struct dj_mongodb_setting and struct
* dj_mongodb_write_counts take positional initializers where designated ones
* do not exist; and that the extern "C" linkage meets the C objects the
* sources compile to.
*   The checks that need no server always run. A session against one runs
* when DJINTERP_TEST_MONGODB_URI, as for t_mongodb.c, reaches one;
* DJINTERP_TEST_MONGODB_REQUIRED=1 makes a deployment that cannot be reached
* a failure. Below the 64-bit floor the layer's headers declare no
* connection, and this test checks nothing. Over a layer built without the
* driver (D_CFG_MONGODB 0), it checks that no handle is made and nothing is
* there.
*
*   Build: compile the layer's C sources and src/djinterp/c/sync/dmutex.c as
* C, this file as C++, and link them with the driver and threads --
* scripts/ci/check_db_mongodb.sh does, at every level.
*
*
* path:      /tests/unintegrated/cpp/db/mongodb/t_mongodb_cpp.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstdlib>  // std::getenv
#include <cstring>  // std::memcpy, std::strcmp, std::strlen, std::strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb.h"  // under test
// re_std
#include "../../../../../inc/re_std/cpp/cstdint/cstdint.hpp"  // re_std::int64_t,
                                                         // INT64_MAX


#if defined(INT64_MAX)

namespace
{

int g_checks   = 0;
int g_failures = 0;

/*
d_tests_mongodb_cpp_expect
  Counts one check, and prints it when it fails.
*/
void d_tests_mongodb_cpp_expect(
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
d_tests_mongodb_cpp_setting
  An environment variable, or a default when it is unset or empty.
*/
const char* d_tests_mongodb_cpp_setting(
    const char* _name,
    const char* _default
)
{
    const char* const value = std::getenv(_name);

    return ( (value) && (value[0] != '\0') ) ? value : _default;
}

/*
d_tests_mongodb_cpp_name
  "dtmcpp" and the digits of the server's number for the connection, read
from its hello's reply as canonical Extended JSON writes it, so that runs
against one deployment at once do not meet; "dtmcpp0" where the reply has
none. sprintf is deprecated on Apple's platforms, and snprintf is not
C++98's, so the name is copied by hand.
*/
void d_tests_mongodb_cpp_name(
    char*       _out,
    std::size_t _size,
    const char* _reply
)
{
    static const char prefix[] = "dtmcpp";
    static const char key[]    = "\"connectionId\" : { \"$numberInt\" : \"";
    const char*       digits   = (_reply) ? std::strstr(_reply, key) : 0;
    std::size_t       length   = sizeof(prefix) - 1u;

    std::memcpy(_out, prefix, length);
    digits = (digits) ? digits + (sizeof(key) - 1u) : "0";

    // the digits, as far as they go and the name has room
    while ( (*digits >= '0')         &&
            (*digits <= '9')         &&
            (length + 1u < _size) )
    {
        _out[length++] = *digits++;
    }

    _out[length] = '\0';

    return;
}

}  // namespace

// D_TESTS_MONGODB_CPP_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_MONGODB_CPP_CHECK(_cond)                                      \
    d_tests_mongodb_cpp_expect((_cond), #_cond, __LINE__)

namespace
{

/*
d_tests_mongodb_cpp_serverless
  What needs no server: reading results, and the layer's refusal of NULL
handles.
*/
void d_tests_mongodb_cpp_serverless(
    bool _stubs
)
{
    const int refused = (_stubs) ? D_DB_UNSUPPORTED : D_DB_INVALID_ARGUMENT;

    D_TESTS_MONGODB_CPP_CHECK(
        !d_mongodb_result_is_client(D_MONGODB_ERROR_DUPLICATE_KEY));
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_result_domain(D_MONGODB_LAYER_NO_MEMORY) ==
        static_cast<unsigned>(D_MONGODB_DOMAIN_LAYER));
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_result_code(D_MONGODB_ERROR_NAMESPACE_NOT_FOUND) == 26u);
    D_TESTS_MONGODB_CPP_CHECK(d_mongodb_ping(0) == refused);
    D_TESTS_MONGODB_CPP_CHECK(d_mongodb_exec(0, 0, "{\"ping\": 1}") ==
                              refused);
    D_TESTS_MONGODB_CPP_CHECK(!d_mongodb_has_collection(0, 0, "c"));
    D_TESTS_MONGODB_CPP_CHECK(!d_mongodb_in_transaction(0));
    D_TESTS_MONGODB_CPP_CHECK(d_mongodb_close(0) == 0);
    D_TESTS_MONGODB_CPP_CHECK(d_mongodb_cursor_is_done(0));

    return;
}

/*
d_tests_mongodb_cpp_documents
  Inserts, finds, updates, counts, aggregates and reads a cursor, in the
handle's current database, in two of its formats.
*/
void d_tests_mongodb_cpp_documents(
    struct dj_mongodb* _db
)
{
    struct dj_mongodb_write_counts counts   = { 0, 0, 0, 0, 0 };
    struct dj_db_rows              rows     = { 0u, 0u, 0, 0 };
    struct dj_mongodb_cursor*      cursor   = 0;
    char*                         id       = 0;
    re_std::int64_t               inserted = -1;
    re_std::int64_t               count    = -1;

    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_insert_one(_db, 0, "c", "{\"n\": 1}", &id) == 0);
    D_TESTS_MONGODB_CPP_CHECK( (id) &&
                               (std::strstr(id, "$oid")) );
    d_db_free(id);
    D_TESTS_MONGODB_CPP_CHECK(
        (d_mongodb_insert_many(_db, 0, "c",
                               "[{\"n\": 2}, {\"n\": 3}, {\"n\": 4}]", 0u,
                               &inserted) == 0) &&
        (inserted == 3));
    D_TESTS_MONGODB_CPP_CHECK(
        (d_mongodb_update_many(_db, 0, "c", "{\"n\": {\"$gt\": 2}}",
                               "{\"$set\": {\"big\": true}}", 0,
                               &counts) == 0) &&
        (counts.matched == 2) && (counts.modified == 2));
    D_TESTS_MONGODB_CPP_CHECK(
        (d_mongodb_count_documents(_db, 0, "c", "{\"big\": true}", 0,
                                   &count) == 0) &&
        (count == 2));
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_find(_db, 0, "c", "{\"n\": 1}",
                       "{\"projection\": {\"_id\": 0}}", &rows) == 0);
    D_TESTS_MONGODB_CPP_CHECK(
        (rows.row_count == 1u) && (rows.column_count == 1u) &&
        (rows.columns[0].type == D_DB_TYPE_JSON) &&
        (std::strcmp(rows.values[0].as.text,
                     "{ \"n\" : { \"$numberInt\" : \"1\" } }") == 0));
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_aggregate(_db, 0, "c",
                            "[{\"$group\": {\"_id\": null, \"s\": "
                            "{\"$sum\": \"$n\"}}}]", 0, &rows) == 0);
    D_TESTS_MONGODB_CPP_CHECK( (rows.row_count == 1u) &&
                               (std::strstr(rows.values[0].as.text,
                                            "\"10\"")) );
    d_db_rows_free(&rows);

    // a page at a time, and the document's bytes
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_find_cursor(_db, 0, "c", 0, "{\"batchSize\": 1}",
                              &cursor) == 0);
    D_TESTS_MONGODB_CPP_CHECK(
        (d_mongodb_cursor_fetch(cursor, 3u, &rows) == 0) &&
        (rows.row_count == 3u));
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_CPP_CHECK(d_mongodb_cursor_close(cursor) == 0);
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_set_document_format(_db, D_MONGODB_BSON) == 0);
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_find_one(_db, 0, "c", "{\"n\": 4}", 0, &rows) == 0);
    D_TESTS_MONGODB_CPP_CHECK( (rows.row_count == 1u) &&
                               (rows.values[0].type == D_DB_VALUE_BLOB) &&
                               (rows.values[0].size > 5u) );
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_set_document_format(_db, D_MONGODB_JSON_CANONICAL) == 0);

    return;
}

/*
d_tests_mongodb_cpp_commands
  Commands, plain and formatted; the schema; a transaction where the
deployment has them; GridFS.
*/
void d_tests_mongodb_cpp_commands(
    struct dj_mongodb* _db
)
{
    static const char       bytes[] = "abc";
    struct dj_db_name_list   names   = { 0u, 0 };
    struct dj_db_rows        rows    = { 0u, 0u, 0, 0 };
    char*                   reply   = 0;
    char*                   file    = 0;
    void*                   data    = 0;
    std::size_t             size    = 0u;
    re_std::int64_t         changes = -1;

    D_TESTS_MONGODB_CPP_CHECK(
        (d_mongodb_execf_changes(_db, 0, &changes,
                                 "{\"insert\": %Q, \"documents\": [{\"s\": "
                                 "%Q, \"n\": %d}]}",
                                 "d", "O'Hara \"x\"", 5) == 0) &&
        (changes == 1));
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_commandf(_db, 0, &reply, "{\"count\": %I}", "d") == 0);
    D_TESTS_MONGODB_CPP_CHECK( (reply) &&
                               (std::strstr(reply, "\"n\"")) );
    d_db_free(reply);
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_query_rows(_db, 0, &rows, "{\"find\": \"d\"}") == 0);
    D_TESTS_MONGODB_CPP_CHECK( (rows.row_count == 1u) &&
                               (std::strstr(rows.values[0].as.text,
                                            "O'Hara \\\"x\\\"")) );
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_create_index(_db, 0, "d", "by_n", "{\"n\": 1}", 0,
                               D_DB_UNIQUE) == 0);
    D_TESTS_MONGODB_CPP_CHECK(d_mongodb_has_index(_db, 0, "d", "by_n"));
    D_TESTS_MONGODB_CPP_CHECK(
        (d_mongodb_list_collections(_db, 0, &names) == 0) &&
        (names.count == 2u));
    d_db_name_list_free(&names);

    // a transaction, where there are any
    if (d_mongodb_supports(_db, D_MONGODB_FEATURE_TRANSACTIONS))
    {
        D_TESTS_MONGODB_CPP_CHECK(d_mongodb_begin_transaction(_db) == 0);
        D_TESTS_MONGODB_CPP_CHECK(
            d_mongodb_insert_one(_db, 0, "d", "{\"n\": 6}", 0) == 0);
        D_TESTS_MONGODB_CPP_CHECK(d_mongodb_in_transaction(_db));
        D_TESTS_MONGODB_CPP_CHECK(d_mongodb_commit(_db) == 0);
    }

    // a file, there and back
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_gridfs_upload(_db, 0, 0, "f", bytes, 3u, 0, &file) == 0);
    D_TESTS_MONGODB_CPP_CHECK(
        (d_mongodb_gridfs_download(_db, 0, 0, file, &data, &size) == 0) &&
        (size == 3u) &&
        (std::memcmp(data, bytes, 3u) == 0));
    d_db_free(data);
    d_db_free(file);

    return;
}

/*
d_tests_mongodb_cpp_session
  Makes a database of its own, drives it through every kind of call, and
drops it.
*/
void d_tests_mongodb_cpp_session(
    struct dj_mongodb* _db
)
{
    char  name[48] = "";
    char* reply    = 0;
    char* current  = 0;

    // the server's number for the connection, which names the database
    if (d_mongodb_command(_db, "admin", &reply, "{\"hello\": 1}") != 0)
    {
        (void)d_mongodb_command(_db, "admin", &reply, "{\"isMaster\": 1}");
    }

    d_tests_mongodb_cpp_name(name, sizeof(name), reply);
    d_db_free(reply);
    D_TESTS_MONGODB_CPP_CHECK(d_mongodb_use_database(_db, name) == 0);
    D_TESTS_MONGODB_CPP_CHECK(
        (d_mongodb_current_database(_db, &current) == 0) &&
        (current) &&
        (std::strcmp(current, name) == 0));
    d_db_free(current);
    D_TESTS_MONGODB_CPP_CHECK(d_mongodb_server_version(_db) != 0);
    d_tests_mongodb_cpp_documents(_db);
    d_tests_mongodb_cpp_commands(_db);
    D_TESTS_MONGODB_CPP_CHECK(d_mongodb_drop_database(_db, name, 0u) == 0);
    D_TESTS_MONGODB_CPP_CHECK(!d_mongodb_has_database(_db, name));

    return;
}

/*
d_tests_mongodb_cpp_stubs
  The layer built without the driver: no handle is made, NULL closes, and
nothing exists.
*/
void d_tests_mongodb_cpp_stubs(void)
{
    const struct dj_mongodb_options options = { "localhost", 0, 0, 0,
                                               0u,          0, 0, 0 };
    const struct dj_mongodb_setting setting = { D_MONGODB_SETTING_APP_NAME,
                                               0ul, "t", 0 };
    struct dj_mongodb_write_counts  counts  = { 7, 7, 7, 7, 7 };
    struct dj_mongodb*              db      = 0;
    char*                          message = 0;

    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_connect_with(&options, &setting, 1u, &db, &message) ==
        D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_CPP_CHECK( (db == 0) && (message) );
    d_db_free(message);
    D_TESTS_MONGODB_CPP_CHECK(!d_mongodb_has_database(db, "admin"));
    D_TESTS_MONGODB_CPP_CHECK(
        d_mongodb_bulk_write(db, 0, "c", "[]", 0u, 0, &counts) ==
        D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_CPP_CHECK( (counts.inserted == 0) &&
                               (counts.upserted == 0) );

    return;
}

}  // namespace

/*
main
  The server-free checks, then a session where a deployment is reached; or,
over a layer built without the driver -- whose version is then 0 -- the
stubs' checks.
*/
int main(void)
{
    const bool        stubs    = (d_mongodb_client_version_number() == 0ul);
    const bool        required = (std::strcmp(
        d_tests_mongodb_cpp_setting("DJINTERP_TEST_MONGODB_REQUIRED", "0"),
        "1") == 0);
    const char* const uri      =
        d_tests_mongodb_cpp_setting("DJINTERP_TEST_MONGODB_URI",
                                    "mongodb://127.0.0.1:27017/");
    struct dj_mongodb* db       = 0;

    d_tests_mongodb_cpp_serverless(stubs);

    // the stubs, or a session where a deployment answers
    if (stubs)
    {
        d_tests_mongodb_cpp_stubs();
    }
    else if ( (d_mongodb_connect_uri(uri, &db) == 0) &&
              (d_mongodb_ping(db) == 0) )
    {
        d_tests_mongodb_cpp_session(db);
    }
    else
    {
        std::printf("  no deployment reached; the server-free checks only\n");
        D_TESTS_MONGODB_CPP_CHECK(!required);
    }

    d_mongodb_close(db);
    std::printf("%d checks, %d failed (%s%s, as C++ %ld)\n",
                g_checks,
                g_failures,
                (stubs) ? "built without the driver" : "driver ",
                d_mongodb_client_version(),
                static_cast<long>(__cplusplus));

    return (g_failures == 0) ? 0 : 1;
}

#else

/*
main
  Below the 64-bit floor the layer declares no connection, and there is
nothing to test.
*/
int main(void)
{
    std::printf("the MongoDB layer is absent below the 64-bit floor\n");

    return 0;
}

#endif  // defined(INT64_MAX)
