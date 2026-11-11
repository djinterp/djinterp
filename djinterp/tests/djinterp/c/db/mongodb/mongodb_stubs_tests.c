/*******************************************************************************
* djinterp [test]                                          mongodb_stubs_tests.c
*
*   The MongoDB layer built without the driver (D_CFG_MONGODB 0): every
* function answers as such a build must -- D_DB_UNSUPPORTED, or false, 0, ""
* or NULL where it cannot fail -- writes the answers it promises even on
* failure (no handle, no cursor, no stream, "no" for an existence, the counts
* of no writes), and leaves every other output alone. The functions that read
* results need no driver and work as they always do. Building and linking it
* at all proves the stubs need neither mongoc.h nor a library.
*
*   Build: CMake's c_mongodb_stubs_tests, or scripts/ci/check_db_mongodb.sh,
* given `stubs`. By hand, from the repo root, at any language level from C99:
* this file and its runner, mongodb_stubs_tests_runner.c in the CMake leaf
* (build/cmake/config/testing/djinterp/c/db/mongodb/), with DTest's C face
* (src/djinterp/c/modules/test/test_suite.c), db_*.c and the layer's sources,
* and -Iinc -Itests/djinterp/c/db/mongodb -DD_CFG_MONGODB=0, linking nothing
* else.
*
*
* path:      /tests/djinterp/c/db/mongodb/mongodb_stubs_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./mongodb_stubs_tests.h"  // corresponding header
// std
#include <string.h>  // memcmp, memset, strcmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb.h"  // under test
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t


//==============================================================================
// HARNESS
//==============================================================================

// g_suite, g_block
//   state: the suite every check and note goes to, set by the entry point,
// and the block open last, so the tests of one header share its block.
static struct dj_test_suite* g_suite = NULL;
static const char*           g_block = NULL;

// g_marker
//   object: whose address stands in for a handle, a cursor and a stream,
// which a build without the driver can never make; the stubs never look
// behind it.
static int g_marker = 0;

// D_TESTS_MONGODB_STUBS_TEXT
//   macro: the marker's address as text, where a function would write text;
// a stub leaves it as it was.
#define D_TESTS_MONGODB_STUBS_TEXT ((char*)(void*)&g_marker)

/*
d_tests_mongodb_stubs_expect
  Records one check, with what was checked and where, against the test
open last.
*/
static
void d_tests_mongodb_stubs_expect(
    bool        _ok,
    const char* _what,
    int         _line
)
{
    (void)d_test_suite_check(g_suite, _ok, _what, __FILE__, _line);

    return;
}

/*
d_tests_mongodb_stubs_start
  Opens a test, and the block of the header it tests when that changes.
*/
static
void d_tests_mongodb_stubs_start(
    const char* _block,
    const char* _test
)
{
    // a test of another header opens that header's block
    if ( (!g_block) ||
         (strcmp(g_block, _block) != 0) )
    {
        (void)d_test_suite_block(g_suite, _block);
        g_block = _block;
    }

    (void)d_test_suite_test(g_suite, _test);

    return;
}

// D_TESTS_MONGODB_STUBS_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_MONGODB_STUBS_CHECK(_cond)                                    \
    d_tests_mongodb_stubs_expect((_cond), #_cond, __LINE__)

// D_TESTS_MONGODB_STUBS_UNSUPPORTED
//   macro: checks that a call reports D_DB_UNSUPPORTED, recording the call's
// own text and line, which again only the preprocessor can supply.
#define D_TESTS_MONGODB_STUBS_UNSUPPORTED(_call)                              \
    d_tests_mongodb_stubs_expect(((_call) == D_DB_UNSUPPORTED),               \
                                 #_call,                                      \
                                 __LINE__)

/*
d_tests_mongodb_stubs_handle
  The marker, as a handle.
*/
static
struct dj_mongodb* d_tests_mongodb_stubs_handle(void)
{
    return (struct dj_mongodb*)(void*)&g_marker;
}

/*
d_tests_mongodb_stubs_untouched
  Whether rows a stub was given are as they were filled: every byte 0x5A.
*/
static
bool d_tests_mongodb_stubs_untouched(
    const void* _object,
    size_t      _size
)
{
    unsigned char pattern[sizeof(struct dj_db_rows)];

    memset(pattern, 0x5A, sizeof(pattern));

    return ( (_size <= sizeof(pattern)) &&
             (memcmp(_object, pattern, _size) == 0) );
}

/*
d_tests_mongodb_stubs_no_counts
  Whether counts are those of no writes.
*/
static
bool d_tests_mongodb_stubs_no_counts(
    const struct dj_mongodb_write_counts* _counts
)
{
    return ( (_counts->inserted == 0) &&
             (_counts->matched == 0)  &&
             (_counts->modified == 0) &&
             (_counts->deleted == 0)  &&
             (_counts->upserted == 0) );
}


//==============================================================================
// TESTS
//==============================================================================

/*
d_tests_mongodb_stubs_results
  Reading results, which needs no driver.
  Tests the following:
  - the server's codes are the server's, unpacked: domain 0, code itself
  - the layer's and libbson's are packed, their domain and code recovered
  - db_common.h's negatives are neither; 0 is nothing
  - the server selection domain is unreachable, the others not
  - the messages: D_DB_UNSUPPORTED says the build is why, the layer's own
    codes their words, a server's code points to d_mongodb_error_message
*/
static
void d_tests_mongodb_stubs_results(void)
{
    const int bson = (int)(D_MONGODB_CLIENT_ERRORS +
                           (D_MONGODB_DOMAIN_BSON + 1) * 65536 + 2);

    d_tests_mongodb_stubs_start("results", "results");

    D_TESTS_MONGODB_STUBS_CHECK(
        !d_mongodb_result_is_client(D_MONGODB_ERROR_DUPLICATE_KEY));
    D_TESTS_MONGODB_STUBS_CHECK(
        d_mongodb_result_domain(D_MONGODB_ERROR_DUPLICATE_KEY) == 0u);
    D_TESTS_MONGODB_STUBS_CHECK(
        d_mongodb_result_code(D_MONGODB_ERROR_DUPLICATE_KEY) == 11000u);
    D_TESTS_MONGODB_STUBS_CHECK(
        d_mongodb_result_is_client(D_MONGODB_LAYER_NO_MEMORY));
    D_TESTS_MONGODB_STUBS_CHECK(
        d_mongodb_result_domain(D_MONGODB_LAYER_NO_MEMORY) ==
        (unsigned)D_MONGODB_DOMAIN_LAYER);
    D_TESTS_MONGODB_STUBS_CHECK(
        d_mongodb_result_code(D_MONGODB_LAYER_UNDESCRIBED) == 3u);
    D_TESTS_MONGODB_STUBS_CHECK( (d_mongodb_result_is_client(bson)) &&
                                 (d_mongodb_result_domain(bson) ==
                                  (unsigned)(D_MONGODB_DOMAIN_BSON + 1)) &&
                                 (d_mongodb_result_code(bson) == 2u) );
    D_TESTS_MONGODB_STUBS_CHECK(
        (!d_mongodb_result_is_client(D_DB_INVALID_ARGUMENT)) &&
        (d_mongodb_result_domain(D_DB_INVALID_ARGUMENT) == 0u) &&
        (d_mongodb_result_code(D_DB_INVALID_ARGUMENT) == 0u));
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_result_code(0) == 0u);
    D_TESTS_MONGODB_STUBS_CHECK(
        (d_mongodb_result_is_unreachable(
             (int)(D_MONGODB_CLIENT_ERRORS +
                   D_MONGODB_DOMAIN_SERVER_SELECTION * 65536u + 2u))) &&
        (!d_mongodb_result_is_unreachable(bson)) &&
        (!d_mongodb_result_is_unreachable(D_DB_UNSUPPORTED)));
    D_TESTS_MONGODB_STUBS_CHECK(
        strstr(d_mongodb_result_message(D_DB_UNSUPPORTED), "no driver") !=
        NULL);
    D_TESTS_MONGODB_STUBS_CHECK(
        strcmp(d_mongodb_result_message(D_DB_NOT_FOUND),
               d_db_status_message(D_DB_NOT_FOUND)) == 0);
    D_TESTS_MONGODB_STUBS_CHECK(
        strcmp(d_mongodb_result_message(D_MONGODB_LAYER_NO_MEMORY),
               "the layer could not allocate what it returns") == 0);
    D_TESTS_MONGODB_STUBS_CHECK(
        strstr(d_mongodb_result_message(D_MONGODB_ERROR_DUPLICATE_KEY),
               "d_mongodb_error_message") != NULL);

    return;
}

/*
d_tests_mongodb_stubs_common
  The driver, its log, the server and the handle's last failure.
  Tests the following:
  - no driver: "" and 0; the log cannot be set, its level -1
  - no server: NULL, 0, no topology, no replica set, no feature
  - no native client, and no last failure: 0, "", "", no label; the reply
    is D_DB_UNSUPPORTED, the place for it untouched
*/
static
void d_tests_mongodb_stubs_common(void)
{
    struct dj_mongodb* const db    = d_tests_mongodb_stubs_handle();
    char*                   reply = D_TESTS_MONGODB_STUBS_TEXT;

    d_tests_mongodb_stubs_start("common", "common");

    D_TESTS_MONGODB_STUBS_CHECK(strcmp(d_mongodb_client_version(), "") == 0);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_client_version_number() == 0ul);
    D_TESTS_MONGODB_STUBS_CHECK(strcmp(d_mongodb_bson_version(), "") == 0);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_set_log_level(2));
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_get_log_level() == -1);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_server_version(db) == NULL);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_server_version_number(db) == 0ul);
    D_TESTS_MONGODB_STUBS_CHECK(!d_mongodb_server_at_least(db, 0u, 0u, 0u));
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_server_wire_version(db) == 0);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_server_topology(db) ==
                                D_MONGODB_TOPOLOGY_UNKNOWN);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_server_replica_set(db) == NULL);

    // no feature at all
    for (int f = D_MONGODB_FEATURE_TRANSACTIONS;
         f <= D_MONGODB_FEATURE_WILDCARD_INDEXES;
         ++f)
    {
        D_TESTS_MONGODB_STUBS_CHECK(
            !d_mongodb_supports(db, (enum dj_mongodb_feature)f));
    }

    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_native(db) == NULL);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_error_code(db) == 0);
    D_TESTS_MONGODB_STUBS_CHECK(strcmp(d_mongodb_error_message(db), "") == 0);
    D_TESTS_MONGODB_STUBS_CHECK(strcmp(d_mongodb_error_name(db), "") == 0);
    D_TESTS_MONGODB_STUBS_CHECK(
        !d_mongodb_error_has_label(db, "TransientTransactionError"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_error_reply(db, &reply));
    D_TESTS_MONGODB_STUBS_CHECK(reply == D_TESTS_MONGODB_STUBS_TEXT);

    return;
}

/*
d_tests_mongodb_stubs_connection
  Connecting, adopting, closing, and the handle's latest write.
  Tests the following:
  - every connect and adopt is D_DB_UNSUPPORTED, the handle NULL; connect_with
    says why in its message, which the caller frees
  - close takes anything as no handle, and NULL as always
  - ping is D_DB_UNSUPPORTED; no write was counted; no id, its place
    untouched
*/
static
void d_tests_mongodb_stubs_connection(void)
{
    struct dj_mongodb* const        marker  = d_tests_mongodb_stubs_handle();
    struct dj_mongodb*              db      = marker;
    char*                          message = D_TESTS_MONGODB_STUBS_TEXT;
    char*                          id      = D_TESTS_MONGODB_STUBS_TEXT;
    const struct dj_mongodb_options options = { "localhost", NULL, NULL, NULL,
                                               0u,          NULL, NULL, NULL };
    const struct dj_mongodb_setting setting = { D_MONGODB_SETTING_APP_NAME,
                                               0ul, "t", NULL };

    d_tests_mongodb_stubs_start("connection", "connection");

    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_connect(&options, &db));
    D_TESTS_MONGODB_STUBS_CHECK(db == NULL);
    db = marker;
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_connect_uri("mongodb://127.0.0.1/", &db));
    D_TESTS_MONGODB_STUBS_CHECK(db == NULL);
    db = marker;
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_connect_with(&options, &setting, 1u, &db, &message));
    D_TESTS_MONGODB_STUBS_CHECK( (db == NULL) &&
                                 (message)    &&
                                 (message != D_TESTS_MONGODB_STUBS_TEXT) &&
                                 (strstr(message, "no driver")) );

    // the message is the caller's
    if (message != D_TESTS_MONGODB_STUBS_TEXT)
    {
        d_db_free(message);
    }

    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_connect_with(NULL, NULL, 0u, NULL, NULL));
    db = marker;
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_adopt(&g_marker, false, &db));
    D_TESTS_MONGODB_STUBS_CHECK(db == NULL);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_close(NULL) == 0);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_close(marker) == 0);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_ping(marker));
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_changes(marker) == 0);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_last_insert_id(marker, &id));
    D_TESTS_MONGODB_STUBS_CHECK(id == D_TESTS_MONGODB_STUBS_TEXT);

    return;
}

/*
d_tests_mongodb_stubs_query
  Commands, plain and formatted.
  Tests the following:
  - each is D_DB_UNSUPPORTED, its changes, reply and rows untouched
*/
static
void d_tests_mongodb_stubs_query(void)
{
    struct dj_mongodb* const db      = d_tests_mongodb_stubs_handle();
    int64_t                 changes = 7;
    char*                   reply   = D_TESTS_MONGODB_STUBS_TEXT;
    struct dj_db_rows        rows;

    d_tests_mongodb_stubs_start("query", "query");

    memset(&rows, 0x5A, sizeof(rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_exec(db, NULL,
                                                     "{\"ping\": 1}"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_execf(db, "d",
                                                      "{\"ping\": %d}", 1));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_exec_changes(db, NULL, &changes, "{\"ping\": 1}"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_execf_changes(db, NULL, &changes, "{\"drop\": %Q}", "c"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_command(db, NULL, &reply, "{\"ping\": 1}"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_commandf(db, NULL, &reply, "{\"ping\": %d}", 1));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_query_rows(db, NULL, &rows, "{\"find\": \"c\"}"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_query_rowsf(db, NULL, &rows, "{\"find\": %I}", "c"));
    D_TESTS_MONGODB_STUBS_CHECK( (changes == 7) &&
                                 (reply == D_TESTS_MONGODB_STUBS_TEXT) );
    D_TESTS_MONGODB_STUBS_CHECK(
        d_tests_mongodb_stubs_untouched(&rows, sizeof(rows)));

    return;
}

/*
d_tests_mongodb_stubs_document
  Documents: inserted, found, updated, deleted, counted, aggregated, found
and modified.
  Tests the following:
  - each is D_DB_UNSUPPORTED
  - insert_many's count is 0, as it is on any failure; a NULL place for it
    is taken
  - every other answer is untouched: ids, rows, counts that a failure
    leaves alone
*/
static
void d_tests_mongodb_stubs_document(void)
{
    struct dj_mongodb* const       db       = d_tests_mongodb_stubs_handle();
    char*                         id       = D_TESTS_MONGODB_STUBS_TEXT;
    int64_t                       inserted = 7;
    int64_t                       count    = 7;
    struct dj_mongodb_write_counts counts   = { 7, 7, 7, 7, 7 };
    struct dj_db_rows              rows;

    d_tests_mongodb_stubs_start("document", "document");

    memset(&rows, 0x5A, sizeof(rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_insert_one(db, NULL, "c",
                                                           "{}", &id));
    D_TESTS_MONGODB_STUBS_CHECK(id == D_TESTS_MONGODB_STUBS_TEXT);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_insert_many(db, NULL, "c",
                                                            "[{}]", 0u,
                                                            &inserted));
    D_TESTS_MONGODB_STUBS_CHECK(inserted == 0);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_insert_many(db, NULL, "c",
                                                            "[{}]", 0u,
                                                            NULL));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_find(db, NULL, "c", NULL,
                                                     NULL, &rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_find_one(db, NULL, "c", "{}",
                                                         NULL, &rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_find_keyed(db, NULL, "c",
                                                           NULL, NULL,
                                                           &rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_update_one(db, NULL, "c", "{}", "{\"$set\": {\"a\": 1}}",
                             NULL, &counts));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_update_many(db, NULL, "c", "{}", "{\"$set\": {\"a\": 1}}",
                              NULL, &counts));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_replace_one(db, NULL, "c", "{}", "{\"a\": 1}", NULL,
                              &counts));
    D_TESTS_MONGODB_STUBS_CHECK(counts.inserted == 7);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_delete_one(db, NULL, "c", "{}", NULL, &count));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_delete_many(db, NULL, "c", "{}", NULL, &count));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_count_documents(db, NULL, "c", NULL, NULL, &count));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_estimated_document_count(db, NULL, "c", NULL, &count));
    D_TESTS_MONGODB_STUBS_CHECK(count == 7);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_distinct(db, NULL, "c", "a", NULL, NULL, &rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_aggregate(db, NULL, "c", "[]", NULL, &rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_find_one_and_update(db, NULL, "c", "{}",
                                      "{\"$set\": {\"a\": 1}}", NULL, &rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_find_one_and_replace(db, NULL, "c", "{}", "{}", NULL,
                                       &rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_find_one_and_delete(db, NULL, "c", "{}", NULL, &rows));
    D_TESTS_MONGODB_STUBS_CHECK(
        d_tests_mongodb_stubs_untouched(&rows, sizeof(rows)));

    return;
}

/*
d_tests_mongodb_stubs_bulk
  Bulk writes, to a collection and across them.
  Tests the following:
  - each is D_DB_UNSUPPORTED, its counts zeros, as they are on any
    failure; NULL places for them are taken
*/
static
void d_tests_mongodb_stubs_bulk(void)
{
    struct dj_mongodb* const       db     = d_tests_mongodb_stubs_handle();
    struct dj_mongodb_write_counts counts = { 7, 7, 7, 7, 7 };

    d_tests_mongodb_stubs_start("bulk", "bulk");

    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_bulk_write(db, NULL, "c",
                             "[{\"insertOne\": {\"document\": {}}}]", 0u,
                             NULL, &counts));
    D_TESTS_MONGODB_STUBS_CHECK(d_tests_mongodb_stubs_no_counts(&counts));
    counts.inserted = 7;
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_client_bulk_write(db,
                                    "[{\"insertOne\": {\"namespace\": "
                                    "\"d.c\", \"document\": {}}}]",
                                    0u, NULL, &counts));
    D_TESTS_MONGODB_STUBS_CHECK(d_tests_mongodb_stubs_no_counts(&counts));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_bulk_write(db, NULL, "c", "[]", 0u, NULL, NULL));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_client_bulk_write(db, "[]", 0u, NULL, NULL));

    return;
}

/*
d_tests_mongodb_stubs_cursor
  Cursors and change streams.
  Tests the following:
  - each opening is D_DB_UNSUPPORTED, the cursor or the stream NULL
  - reading either is D_DB_UNSUPPORTED, the rows and the token untouched; a
    cursor is done; closing either is 0, NULL among them
*/
static
void d_tests_mongodb_stubs_cursor(void)
{
    struct dj_mongodb* const               db      =
        d_tests_mongodb_stubs_handle();
    struct dj_mongodb_cursor* const        marker  =
        (struct dj_mongodb_cursor*)(void*)&g_marker;
    struct dj_mongodb_change_stream* const flowing =
        (struct dj_mongodb_change_stream*)(void*)&g_marker;
    struct dj_mongodb_cursor*              cursor  = marker;
    struct dj_mongodb_change_stream*       stream  = flowing;
    char*                                 token   = D_TESTS_MONGODB_STUBS_TEXT;
    struct dj_db_rows                      rows;

    d_tests_mongodb_stubs_start("cursor", "cursor");

    memset(&rows, 0x5A, sizeof(rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_find_cursor(db, NULL, "c", NULL, NULL, &cursor));
    D_TESTS_MONGODB_STUBS_CHECK(cursor == NULL);
    cursor = marker;
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_aggregate_cursor(db, NULL, "c", "[]", NULL, &cursor));
    D_TESTS_MONGODB_STUBS_CHECK(cursor == NULL);
    cursor = marker;
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_command_cursor(db, NULL, "{\"listCollections\": 1}",
                                 &cursor));
    D_TESTS_MONGODB_STUBS_CHECK(cursor == NULL);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_cursor_fetch(marker, 1u,
                                                             &rows));
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_cursor_is_done(marker));
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_cursor_is_done(NULL));
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_cursor_close(marker) == 0);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_cursor_close(NULL) == 0);

    // change streams
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_watch_collection(db, NULL, "c", NULL, NULL, &stream));
    D_TESTS_MONGODB_STUBS_CHECK(stream == NULL);
    stream = flowing;
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_watch_database(db, NULL, NULL, NULL, &stream));
    D_TESTS_MONGODB_STUBS_CHECK(stream == NULL);
    stream = flowing;
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_watch_deployment(db, "[]", NULL, &stream));
    D_TESTS_MONGODB_STUBS_CHECK(stream == NULL);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_change_stream_fetch(flowing, 1u, &rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_change_stream_resume_token(flowing, &token));
    D_TESTS_MONGODB_STUBS_CHECK(token == D_TESTS_MONGODB_STUBS_TEXT);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_change_stream_close(flowing) == 0);
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_change_stream_close(NULL) == 0);
    D_TESTS_MONGODB_STUBS_CHECK(
        d_tests_mongodb_stubs_untouched(&rows, sizeof(rows)));

    return;
}

/*
d_tests_mongodb_stubs_database
  Databases.
  Tests the following:
  - has is false; has_s is D_DB_UNSUPPORTED and "no"
  - drop, list, use and current are D_DB_UNSUPPORTED, their answers
    untouched
*/
static
void d_tests_mongodb_stubs_database(void)
{
    struct dj_mongodb* const db     = d_tests_mongodb_stubs_handle();
    bool                    exists = true;
    char*                   name   = D_TESTS_MONGODB_STUBS_TEXT;
    struct dj_db_name_list   names;

    d_tests_mongodb_stubs_start("database", "database");

    memset(&names, 0x5A, sizeof(names));
    D_TESTS_MONGODB_STUBS_CHECK(!d_mongodb_has_database(db, "d"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_has_database_s(db, "d",
                                                               &exists));
    D_TESTS_MONGODB_STUBS_CHECK(!exists);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_drop_database(db, "d", 0u));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_list_databases(db, &names));
    D_TESTS_MONGODB_STUBS_CHECK(
        d_tests_mongodb_stubs_untouched(&names, sizeof(names)));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_use_database(db, "d"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_current_database(db, &name));
    D_TESTS_MONGODB_STUBS_CHECK(name == D_TESTS_MONGODB_STUBS_TEXT);

    return;
}

/*
d_tests_mongodb_stubs_schema
  Collections, views and indexes.
  Tests the following:
  - each has is false; each has_s is D_DB_UNSUPPORTED and "no"
  - every create, drop, rename, list, info and description is
    D_DB_UNSUPPORTED, its answers untouched
*/
static
void d_tests_mongodb_stubs_schema(void)
{
    struct dj_mongodb* const db     = d_tests_mongodb_stubs_handle();
    bool                    exists = true;
    char*                   info   = D_TESTS_MONGODB_STUBS_TEXT;
    struct dj_db_name_list   names;
    struct dj_db_rows        rows;

    d_tests_mongodb_stubs_start("schema", "schema");

    memset(&names, 0x5A, sizeof(names));
    memset(&rows, 0x5A, sizeof(rows));

    // collections
    D_TESTS_MONGODB_STUBS_CHECK(!d_mongodb_has_collection(db, NULL, "c"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_has_collection_s(db, NULL, "c", &exists));
    D_TESTS_MONGODB_STUBS_CHECK(!exists);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_create_collection(db, NULL, "c", NULL, 0u));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_drop_collection(db, NULL, "c", 0u));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_rename_collection(db, NULL, "c", NULL, "e", 0u));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_list_collections(db, NULL, &names));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_collection_info(db, NULL, "c", &info));
    D_TESTS_MONGODB_STUBS_CHECK(info == D_TESTS_MONGODB_STUBS_TEXT);

    // views
    exists = true;
    D_TESTS_MONGODB_STUBS_CHECK(!d_mongodb_has_view(db, NULL, "v"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_has_view_s(db, NULL, "v", &exists));
    D_TESTS_MONGODB_STUBS_CHECK(!exists);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_create_view(db, NULL, "v", "c", "[]", NULL, 0u));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_drop_view(db, NULL, "v", 0u));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_list_views(db, NULL, &names));

    // indexes
    exists = true;
    D_TESTS_MONGODB_STUBS_CHECK(!d_mongodb_has_index(db, NULL, "c", "i"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_has_index_s(db, NULL, "c", "i", &exists));
    D_TESTS_MONGODB_STUBS_CHECK(!exists);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_create_index(db, NULL, "c", "i", "{\"a\": 1}", NULL, 0u));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_drop_index(db, NULL, "c", "i", 0u));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_list_indexes(db, NULL, "c", &names));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_describe_indexes(db, NULL, "c", &rows));
    D_TESTS_MONGODB_STUBS_CHECK(
        (d_tests_mongodb_stubs_untouched(&names, sizeof(names))) &&
        (d_tests_mongodb_stubs_untouched(&rows, sizeof(rows))));

    return;
}

/*
d_tests_mongodb_stubs_transaction
  Transactions.
  Tests the following:
  - begin, begin_with, commit and rollback are D_DB_UNSUPPORTED
  - none is open, and none failed
*/
static
void d_tests_mongodb_stubs_transaction(void)
{
    struct dj_mongodb* const db = d_tests_mongodb_stubs_handle();

    d_tests_mongodb_stubs_start("transaction", "transaction");

    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_begin_transaction(db));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_begin_transaction_with(db,
                                         "{\"readConcern\": {\"level\": "
                                         "\"snapshot\"}}"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_commit(db));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_rollback(db));
    D_TESTS_MONGODB_STUBS_CHECK(!d_mongodb_in_transaction(db));
    D_TESTS_MONGODB_STUBS_CHECK(!d_mongodb_in_failed_transaction(db));
    D_TESTS_MONGODB_STUBS_CHECK(!d_mongodb_in_transaction(NULL));

    return;
}

/*
d_tests_mongodb_stubs_settings
  The handle's concerns, read preference, document format and time limit.
  Tests the following:
  - every set is D_DB_UNSUPPORTED; every get that can fail is too, its
    answer untouched
  - the format reads canonical, and the time limit 0
*/
static
void d_tests_mongodb_stubs_settings(void)
{
    struct dj_mongodb* const db      = d_tests_mongodb_stubs_handle();
    char*                   concern = D_TESTS_MONGODB_STUBS_TEXT;

    d_tests_mongodb_stubs_start("settings", "settings");

    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_set_read_concern(db, "{\"level\": \"majority\"}"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_get_read_concern(db,
                                                                 &concern));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_set_write_concern(db, "{\"w\": \"majority\"}"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_get_write_concern(db,
                                                                  &concern));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_set_read_preference(db, "{\"mode\": \"secondary\"}"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_get_read_preference(db, &concern));
    D_TESTS_MONGODB_STUBS_CHECK(concern == D_TESTS_MONGODB_STUBS_TEXT);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_set_document_format(db, D_MONGODB_JSON_RELAXED));
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_get_document_format(db) ==
                                D_MONGODB_JSON_CANONICAL);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_set_max_time_ms(db, 5));
    D_TESTS_MONGODB_STUBS_CHECK(d_mongodb_get_max_time_ms(db) == 0);

    return;
}

/*
d_tests_mongodb_stubs_gridfs
  GridFS.
  Tests the following:
  - every upload, download, delete and find is D_DB_UNSUPPORTED, its id,
    bytes, size and rows untouched
*/
static
void d_tests_mongodb_stubs_gridfs(void)
{
    struct dj_mongodb* const db   = d_tests_mongodb_stubs_handle();
    char*                   id   = D_TESTS_MONGODB_STUBS_TEXT;
    void*                   data = &g_marker;
    size_t                  size = 7u;
    struct dj_db_rows        rows;

    d_tests_mongodb_stubs_start("gridfs", "gridfs");

    memset(&rows, 0x5A, sizeof(rows));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_gridfs_upload(db, NULL, NULL, "f", "x", 1u, NULL, &id));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_gridfs_upload_file(db, NULL, NULL, "f", "p", NULL, &id));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_gridfs_find_id(db, NULL, NULL, "f", &id));
    D_TESTS_MONGODB_STUBS_CHECK(id == D_TESTS_MONGODB_STUBS_TEXT);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_gridfs_download(db, NULL, NULL, "1", &data, &size));
    D_TESTS_MONGODB_STUBS_CHECK( (data == &g_marker) &&
                                 (size == 7u) );
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_gridfs_download_file(db, NULL, NULL, "1", "p"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_gridfs_delete(db, NULL, NULL, "1"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_gridfs_find(db, NULL, NULL, NULL, NULL, &rows));
    D_TESTS_MONGODB_STUBS_CHECK(
        d_tests_mongodb_stubs_untouched(&rows, sizeof(rows)));

    return;
}

/*
d_tests_mongodb_stubs_maintenance
  Sizes, compaction and validation.
  Tests the following:
  - each is D_DB_UNSUPPORTED, the sizes, the answer and the report
    untouched
*/
static
void d_tests_mongodb_stubs_maintenance(void)
{
    struct dj_mongodb* const db     = d_tests_mongodb_stubs_handle();
    int64_t                 bytes  = 7;
    bool                    valid  = false;
    char*                   report = D_TESTS_MONGODB_STUBS_TEXT;

    d_tests_mongodb_stubs_start("maintenance", "maintenance");

    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_collection_size(db, NULL, "c", &bytes));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_database_size(db, NULL,
                                                              &bytes));
    D_TESTS_MONGODB_STUBS_CHECK(bytes == 7);
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(d_mongodb_compact(db, NULL, "c"));
    D_TESTS_MONGODB_STUBS_UNSUPPORTED(
        d_mongodb_validate(db, NULL, "c", D_MONGODB_VALIDATE_FULL, &valid,
                           &report));
    D_TESTS_MONGODB_STUBS_CHECK( (!valid) &&
                                 (report == D_TESTS_MONGODB_STUBS_TEXT) );

    return;
}


//==============================================================================
// RUN
//==============================================================================

/*
d_tests_mongodb_stubs
  The suite's section: every test of the layer built without the driver, in
the order of the headers.
*/
void d_tests_mongodb_stubs(
    struct dj_test_suite* _suite
)
{
    g_suite = _suite;
    g_block = NULL;

    (void)d_test_suite_module(_suite, "mongodb_stubs");
    (void)d_test_suite_describe(_suite,
                                "c/db/mongodb built without the driver");

    d_tests_mongodb_stubs_results();
    d_tests_mongodb_stubs_common();
    d_tests_mongodb_stubs_connection();
    d_tests_mongodb_stubs_query();
    d_tests_mongodb_stubs_document();
    d_tests_mongodb_stubs_bulk();
    d_tests_mongodb_stubs_cursor();
    d_tests_mongodb_stubs_database();
    d_tests_mongodb_stubs_schema();
    d_tests_mongodb_stubs_transaction();
    d_tests_mongodb_stubs_settings();
    d_tests_mongodb_stubs_gridfs();
    d_tests_mongodb_stubs_maintenance();

    g_suite = NULL;

    return;
}
