/*******************************************************************************
* djinterp [c]                                                       t_mongodb.c
*
*   Conformance harness for djinterp's MongoDB convenience layer: every
* function in c/db/mongodb/mongodb_*.h, its success paths, its documented
* failures and its argument checks, against a live deployment -- MongoDB from
* 4.2, a replica set -- through whichever driver it is linked with, 1.21 and
* later of either line. Where the answer depends on the server or the
* driver, the check asks the handle the server's version, or the layer the
* driver's, and expects the matching answer, so one harness passes on every
* pair the layer supports.
*   Each d_mongodb_supports answer is also checked against the server
* itself, by doing what needs the feature: a version table that drifts from
* the truth fails here, not in a program.
*   The tests follow the headers, section by section. Each works in a
* database of its own, named after the run -- an ObjectId the harness makes,
* so that two runs against one deployment never meet -- and after the test,
* made current on its handle and dropped at the end.
*
*   Deployment: DJINTERP_TEST_MONGODB_URI, a connection string, says where
* and as whom (default mongodb://127.0.0.1:27017/); the user needs the root
* role, since the harness drops databases, compacts and validates. The
* connection string carries the password, so give it in the environment
* only, never in a file. DJINTERP_TEST_MONGODB_STANDALONE_URI, optional,
* names a standalone server, on which the checks that need one run: that a
* transaction and a change stream are D_DB_UNSUPPORTED there. The harness
* fails, saying why, when it cannot connect.
*
*   Build (from the repo root), at any language level from C99, against the
* driver -- or let scripts/ci/check_db_mongodb.sh, which runs it across
* compilers, standards, drivers and servers:
*     cc -std=c99 -D_XOPEN_SOURCE=700 -Wall -Wextra -Werror                   \
*        $(pkg-config --cflags libmongoc-1.0)                                  \
*        src/djinterp/c/db/db_*.c src/djinterp/c/db/mongodb/mongodb_*.c       \
*        src/djinterp/c/sync/dmutex.c                                         \
*        tests/unintegrated/c/db/mongodb/t_mongodb.c                          \
*        $(pkg-config --libs libmongoc-1.0) -lpthread -o t_mongodb
*     (mongoc2 in place of libmongoc-1.0 for the 2.x line)
*
*
* path:      /tests/unintegrated/c/db/mongodb/t_mongodb.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.29
*******************************************************************************/
// WIN32_LEAN_AND_MEAN, NOMINMAX
//   knobs (Windows'): <windows.h>, which mongoc.h includes through
// <winsock2.h>, without its rarer headers -- rpcndr.h among them, which makes
// `small` a macro -- and without its min and max macros, as the other
// vendors' harnesses have it; they precede every include.
#if defined(_WIN32)
    #if !defined(WIN32_LEAN_AND_MEAN)
        #define WIN32_LEAN_AND_MEAN 1
    #endif  // WIN32_LEAN_AND_MEAN
    #if !defined(NOMINMAX)
        #define NOMINMAX 1
    #endif  // NOMINMAX
#endif  // _WIN32
// D_CFG_ENV_USING_MONGODB
//   knob (env's): on, since the harness calls the driver itself, to adopt a
// client and to make the run's name, as a program asking for env's
// detection does; the layer turns it on for its own sources alone.
#ifndef D_CFG_ENV_USING_MONGODB
    #define D_CFG_ENV_USING_MONGODB 1
#endif  // D_CFG_ENV_USING_MONGODB

// std
#include <stdio.h>   // printf, snprintf, fopen, fwrite, fread, remove
#include <stdlib.h>  // getenv
#include <string.h>  // memcmp, memset, strcmp, strlen, strncmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb.h"  // under test
#include "../../../../../inc/djinterp/env/db/mongodb/env_mongodb.h"  // mongoc.h
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_connection.h"  // the defaults
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t, INT64_MAX


//==============================================================================
// HARNESS
//==============================================================================

// g_checks, g_failures, g_skipped
//   counters: the checks run, those that failed, and the groups skipped for
// want of what they need.
static int g_checks   = 0;
static int g_failures = 0;
static int g_skipped  = 0;

// g_uri, g_standalone
//   settings: the deployment's connection string, and a standalone server's,
// or NULL, from the environment.
static const char* g_uri        = "mongodb://127.0.0.1:27017/";
static const char* g_standalone = NULL;

// g_run
//   setting: the prefix of every database the run makes: "dtm" and an
// ObjectId of the harness's, whose random part no other process shares.
static char g_run[32] = "";

// g_unreached
//   state: why the latest connection d_tests_mongodb_connect tried could not
// be made, for the run's last line when the deployment is not there.
static char g_unreached[256] = "";

/*
d_tests_mongodb_expect
  Counts one check, and prints it when it fails.
*/
static
void d_tests_mongodb_expect(
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

// D_TESTS_MONGODB_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_MONGODB_CHECK(_cond)                                          \
    d_tests_mongodb_expect((_cond), #_cond, __LINE__)

/*
d_tests_mongodb_expect_result
  Counts one check of a result, and prints the handle's words when the
result is not the one expected.
*/
static
void d_tests_mongodb_expect_result(
    const struct dj_mongodb* _db,
    int                      _result,
    int                      _expected,
    const char*              _what,
    int                      _line
)
{
    d_tests_mongodb_expect(_result == _expected, _what, _line);

    // the result, its domain and code, and what the handle says
    if (_result != _expected)
    {
        printf("    got %d (domain %u, code %u), expected %d: %s [%s]\n",
               _result,
               d_mongodb_result_domain(_result),
               d_mongodb_result_code(_result),
               _expected,
               d_mongodb_error_message(_db),
               d_mongodb_result_message(_result));
    }

    return;
}

// D_TESTS_MONGODB_RESULT
//   macro: records a check that a call returned a given result, with the
// call's own text and line, which only the preprocessor can supply.
#define D_TESTS_MONGODB_RESULT(_db, _call, _expected)                         \
    d_tests_mongodb_expect_result((_db),                                       \
                                  (_call),                                     \
                                  (_expected),                                 \
                                  #_call " == " #_expected,                    \
                                  __LINE__)

// D_TESTS_MONGODB_OK
//   macro: records a check that a call succeeded; a macro, as
// D_TESTS_MONGODB_RESULT is, for the call's text and line.
#define D_TESTS_MONGODB_OK(_db, _call)                                        \
    D_TESTS_MONGODB_RESULT((_db), (_call), 0)


//==============================================================================
// FIXTURES
//==============================================================================

/*
d_tests_mongodb_setting
  An environment variable, or a default when it is unset or empty.
*/
static
const char* d_tests_mongodb_setting(
    const char* _name,
    const char* _default
)
{
    const char* const value = getenv(_name);

    return ( (value) && (value[0] != '\0') ) ? value : _default;
}

/*
d_tests_mongodb_connect
  A handle on the deployment, or on the standalone server; NULL, said why,
when none can be made.
*/
static
struct dj_mongodb* d_tests_mongodb_connect(
    const char* _uri
)
{
    struct dj_mongodb*              db       = NULL;
    char*                          reason   = NULL;
    struct dj_mongodb_options       options  = { NULL, NULL, NULL, NULL,
                                                0u,   NULL, NULL, NULL };

    options.uri = _uri;

    // a handle, or a reason
    if (d_mongodb_connect_with(&options, NULL, 0u, &db, &reason) != 0)
    {
        printf("  cannot connect: %s\n", (reason) ? reason : "no reason");
        snprintf(g_unreached,
                 sizeof(g_unreached),
                 "%s",
                 (reason) ? reason : "no reason");
    }

    d_db_free(reason);

    return db;
}

/*
d_tests_mongodb_begin
  A handle for one test, its database the test's own, dropped first in case
a run before left it, and current.
*/
static
struct dj_mongodb* d_tests_mongodb_begin(
    const char* _word,
    char*       _name,
    size_t      _size
)
{
    struct dj_mongodb* const db = d_tests_mongodb_connect(g_uri);

    snprintf(_name, _size, "%s_%s", g_run, _word);

    // the database, empty and current
    if (db)
    {
        D_TESTS_MONGODB_OK(db,
                           d_mongodb_drop_database(db, _name, D_DB_IF_EXISTS));
        D_TESTS_MONGODB_OK(db, d_mongodb_use_database(db, _name));
    }

    return db;
}

/*
d_tests_mongodb_end
  Drops the test's database and closes its handle; a transaction left open
is rolled back first.
*/
static
void d_tests_mongodb_end(
    struct dj_mongodb* _db,
    const char*        _name
)
{
    // nothing was opened
    if (!_db)
    {
        return;
    }

    if (d_mongodb_in_transaction(_db))
    {
        (void)d_mongodb_rollback(_db);
    }

    D_TESTS_MONGODB_OK(_db, d_mongodb_drop_database(_db,
                                                    _name,
                                                    D_DB_IF_EXISTS));
    D_TESTS_MONGODB_OK(_db, d_mongodb_close(_db));

    return;
}

/*
d_tests_mongodb_row
  A row's document as text, or "" for no such row or a value not text.
*/
static
const char* d_tests_mongodb_row(
    const struct dj_db_rows* _rows,
    size_t                   _row
)
{
    const struct dj_db_value* const value = d_db_rows_value(_rows, _row, 0u);

    return ( (value) &&
             (value->type == D_DB_VALUE_TEXT) ) ? value->as.text : "";
}

/*
d_tests_mongodb_contains
  Whether text holds a part, and says what it held where it does not.
*/
static
bool d_tests_mongodb_contains(
    const char* _text,
    const char* _part
)
{
    const bool found = ( (_text) &&
                         (strstr(_text, _part) != NULL) );

    // what was there instead
    if (!found)
    {
        printf("    %.300s\n    does not hold %s\n",
               (_text) ? _text : "(null)",
               _part);
    }

    return found;
}

/*
d_tests_mongodb_rows_hold
  Whether some row of the rows holds a part.
*/
static
bool d_tests_mongodb_rows_hold(
    const struct dj_db_rows* _rows,
    const char*              _part
)
{
    for (size_t i = 0u; i < _rows->row_count; ++i)
    {
        if (strstr(d_tests_mongodb_row(_rows, i), _part))
        {
            return true;
        }
    }

    printf("    no row of %lu holds %s\n",
           (unsigned long)_rows->row_count,
           _part);

    return false;
}

/*
d_tests_mongodb_has_name
  Whether a name list holds a name, exactly.
*/
static
bool d_tests_mongodb_has_name(
    const struct dj_db_name_list* _names,
    const char*                   _name
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
d_tests_mongodb_count
  The documents a filter matches, or -1 on a failure.
*/
static
int64_t d_tests_mongodb_count(
    struct dj_mongodb* _db,
    const char*        _collection,
    const char*        _filter
)
{
    int64_t count = -1;

    return (d_mongodb_count_documents(_db,
                                      NULL,
                                      _collection,
                                      _filter,
                                      NULL,
                                      &count) == 0) ? count : -1;
}

/*
d_tests_mongodb_no_counts
  Whether counts are those of no writes.
*/
static
bool d_tests_mongodb_no_counts(
    const struct dj_mongodb_write_counts* _counts
)
{
    return ( (_counts->inserted == 0) &&
             (_counts->matched == 0)  &&
             (_counts->modified == 0) &&
             (_counts->deleted == 0)  &&
             (_counts->upserted == 0) );
}

/*
d_tests_mongodb_driver_at_least
  Whether the driver loaded is of a release or later.
*/
static
bool d_tests_mongodb_driver_at_least(
    unsigned _major,
    unsigned _minor
)
{
    return (d_mongodb_client_version_number() >=
            ( ((unsigned long)_major * 10000ul) +
              ((unsigned long)_minor * 100ul) ));
}

/*
d_tests_mongodb_skip
  Says a group, or a part of one, is skipped, and why.
*/
static
void d_tests_mongodb_skip(
    const char* _what,
    const char* _why
)
{
    ++g_skipped;
    printf("  skipped: %s (%s)\n", _what, _why);

    return;
}


//==============================================================================
// 1.  COMMON
//==============================================================================

/*
d_tests_mongodb_results
  mongodb_common.h's results, read without a server.
  Tests the following:
  - 0, db_common.h's negatives and a server's code are not packed, their
    domain 0 and their code their own (0 for the negatives)
  - each layer constant is packed in the layer's domain, at its code
  - a packed driver code and a packed libbson code give back their domain
    and their code
  - only the driver's stream and server selection domains are unreachable
  - every result has words, the layer's own for its constants, and a
    pointer to d_mongodb_error_message for a server's or the driver's
  - a real failure of libbson's -- JSON that ends too soon -- and of the
    driver's -- a connection string it cannot read -- are packed so
*/
static
void d_tests_mongodb_results(void)
{
    const int driver = D_MONGODB_CLIENT_ERRORS + (13 * 65536) + 5;
    const int bson   = D_MONGODB_CLIENT_ERRORS +
                       ((D_MONGODB_DOMAIN_BSON + 1) * 65536) + 2;

    printf("results\n");

    // what is not packed
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_client(0));
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_client(D_DB_INVALID_ARGUMENT));
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_client(
                              D_MONGODB_ERROR_DUPLICATE_KEY));
    D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(11000) == 0u);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_code(11000) == 11000u);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_code(D_DB_NOT_FOUND) == 0u);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_code(0) == 0u);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(D_DB_UNSUPPORTED) == 0u);

    // the layer's constants
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(
                              D_MONGODB_LAYER_NO_MEMORY));
    D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(D_MONGODB_LAYER_NO_MEMORY) ==
                          (unsigned)D_MONGODB_DOMAIN_LAYER);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_code(D_MONGODB_LAYER_NO_MEMORY) ==
                          1u);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_code(D_MONGODB_LAYER_TOO_LARGE) ==
                          2u);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_code(
                              D_MONGODB_LAYER_UNDESCRIBED) == 3u);

    // the driver's and libbson's, packed
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(driver));
    D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(driver) == 13u);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_code(driver) == 5u);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(bson) ==
                          (unsigned)(D_MONGODB_DOMAIN_BSON + 1));
    D_TESTS_MONGODB_CHECK(d_mongodb_result_code(bson) == 2u);

    // unreachable: a broken connection, or no server
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_unreachable(
                              D_MONGODB_CLIENT_ERRORS +
                              (int)(D_MONGODB_DOMAIN_STREAM * 65536u) + 4));
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_unreachable(
                              D_MONGODB_CLIENT_ERRORS +
                              (int)(D_MONGODB_DOMAIN_SERVER_SELECTION *
                                    65536u) + 2));
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_unreachable(driver));
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_unreachable(bson));
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_unreachable(
                              D_MONGODB_LAYER_NO_MEMORY));
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_unreachable(11000));
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_unreachable(0));
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_unreachable(
                              D_DB_UNSUPPORTED));

    // words for each
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_result_message(0),
                                 d_db_status_message(0)) == 0);
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_result_message(D_DB_NOT_FOUND),
                                 d_db_status_message(D_DB_NOT_FOUND)) == 0);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_mongodb_result_message(
                                  D_MONGODB_LAYER_NO_MEMORY),
                              "allocate"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_mongodb_result_message(
                                  D_MONGODB_LAYER_TOO_LARGE),
                              "too large"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_mongodb_result_message(
                                  D_MONGODB_LAYER_UNDESCRIBED),
                              "described none"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_mongodb_result_message(11000),
                              "server"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_mongodb_result_message(driver),
                              "driver's"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_mongodb_result_message(bson),
                              "libbson's"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_mongodb_result_message(
                                  D_MONGODB_CLIENT_ERRORS +
                                  (D_MONGODB_DOMAIN_LAYER * 65536) + 99),
                              "unknown"));

    struct dj_mongodb* db     = NULL;
    const int         result = d_mongodb_connect_uri("notauri", &db);

    // a real failure of the driver's, and of libbson's
    D_TESTS_MONGODB_CHECK(db == NULL);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(result));
    D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(result) <
                          (unsigned)D_MONGODB_DOMAIN_BSON);

    db = d_tests_mongodb_connect(g_uri);

    if (db)
    {
        const int json = d_mongodb_exec(db, "admin", "{\"ping\": 1");

        D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(json) ==
                              (unsigned)(D_MONGODB_DOMAIN_BSON +
                                         BSON_ERROR_JSON));
        D_TESTS_MONGODB_CHECK(d_mongodb_error_code(db) == json);
        D_TESTS_MONGODB_OK(db, d_mongodb_close(db));
    }

    return;
}

/*
d_tests_mongodb_library
  The driver's versions and its log.
  Tests the following:
  - the driver's string is the loaded library's, and its number agrees
  - libbson's string is there
  - the log's level reads as the knob's until set; a level outside -1 to 6
    is refused, the level kept; each level sets and reads back, -1 too
*/
static
void d_tests_mongodb_library(void)
{
    unsigned major = 0u;
    unsigned minor = 0u;
    unsigned patch = 0u;

    printf("library\n");

    // the versions
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_client_version(),
                                 mongoc_get_version()) == 0);
    D_TESTS_MONGODB_CHECK(sscanf(d_mongodb_client_version(),
                                 "%u.%u.%u",
                                 &major,
                                 &minor,
                                 &patch) == 3);
    D_TESTS_MONGODB_CHECK(d_mongodb_client_version_number() ==
                          (major * 10000ul) + (minor * 100ul) + patch);
    D_TESTS_MONGODB_CHECK(d_mongodb_client_version_number() >=
                          (unsigned long)D_MONGODB_FLOOR_DRIVER_VERSION);
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_bson_version(),
                                 bson_get_version()) == 0);

    // the log
    D_TESTS_MONGODB_CHECK(d_mongodb_get_log_level() ==
                          D_CFG_MONGODB_LOG_LEVEL);
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_set_log_level(-2),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_set_log_level(7),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(d_mongodb_get_log_level() ==
                          D_CFG_MONGODB_LOG_LEVEL);

    for (int level = -1; level <= 6; ++level)
    {
        D_TESTS_MONGODB_OK(NULL, d_mongodb_set_log_level(level));
        D_TESTS_MONGODB_CHECK(d_mongodb_get_log_level() == level);
    }

    // the harness's own: nothing passed on, so that the failures it makes
    // on purpose print none of the driver's errors
    D_TESTS_MONGODB_OK(NULL, d_mongodb_set_log_level(-1));

    return;
}

/*
d_tests_mongodb_server
  What the handle learned of the server.
  Tests the following:
  - the version string is buildInfo's, and the number is its parts
  - at_least is true at the version and below, false above it
  - the wire version is the floor's or newer
  - a replica set's topology and name, which hello gives
  - a NULL handle answers NULL, 0, false and UNKNOWN
*/
static
void d_tests_mongodb_server(void)
{
    struct dj_mongodb* const db = d_tests_mongodb_connect(g_uri);
    char*                   reply = NULL;
    char                    quoted[80];

    printf("server\n");

    // a handle to ask
    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    const unsigned long number = d_mongodb_server_version_number(db);
    const unsigned      major  = (unsigned)(number / 10000ul);
    const unsigned      minor  = (unsigned)((number / 100ul) % 100ul);
    const unsigned      patch  = (unsigned)(number % 100ul);

    printf("  server %s (%lu), wire %d, driver %s\n",
           d_mongodb_server_version(db),
           number,
           d_mongodb_server_wire_version(db),
           d_mongodb_client_version());

    // the version, as buildInfo writes it
    D_TESTS_MONGODB_OK(db, d_mongodb_command(db,
                                             "admin",
                                             &reply,
                                             "{\"buildInfo\": 1}"));
    snprintf(quoted,
             sizeof(quoted),
             "\"version\" : \"%s\"",
             d_mongodb_server_version(db));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(reply, quoted));
    d_db_free(reply);
    D_TESTS_MONGODB_CHECK(number >= (unsigned long)
                                    D_MONGODB_FLOOR_SERVER_VERSION);

    // at the version, below it, above it
    D_TESTS_MONGODB_CHECK(d_mongodb_server_at_least(db, major, minor, patch));
    D_TESTS_MONGODB_CHECK(d_mongodb_server_at_least(db, 4u, 2u, 0u));
    D_TESTS_MONGODB_CHECK(!d_mongodb_server_at_least(db,
                                                     major,
                                                     minor,
                                                     patch + 1u));
    D_TESTS_MONGODB_CHECK(!d_mongodb_server_at_least(db, 99u, 0u, 0u));
    D_TESTS_MONGODB_CHECK(d_mongodb_server_wire_version(db) >= 8);

    // the deployment
    D_TESTS_MONGODB_CHECK( (d_mongodb_server_topology(db) ==
                            D_MONGODB_TOPOLOGY_REPLICA_SET) ||
                           (d_mongodb_server_topology(db) ==
                            D_MONGODB_TOPOLOGY_SHARDED) );

    if (d_mongodb_server_topology(db) == D_MONGODB_TOPOLOGY_REPLICA_SET)
    {
        D_TESTS_MONGODB_CHECK(d_mongodb_server_replica_set(db) != NULL);
    }

    // no handle, no server
    D_TESTS_MONGODB_CHECK(d_mongodb_server_version(NULL) == NULL);
    D_TESTS_MONGODB_CHECK(d_mongodb_server_version_number(NULL) == 0ul);
    D_TESTS_MONGODB_CHECK(!d_mongodb_server_at_least(NULL, 0u, 0u, 0u));
    D_TESTS_MONGODB_CHECK(d_mongodb_server_wire_version(NULL) == 0);
    D_TESTS_MONGODB_CHECK(d_mongodb_server_topology(NULL) ==
                          D_MONGODB_TOPOLOGY_UNKNOWN);
    D_TESTS_MONGODB_CHECK(d_mongodb_server_replica_set(NULL) == NULL);
    D_TESTS_MONGODB_CHECK(!d_mongodb_supports(NULL,
                                              D_MONGODB_FEATURE_TRANSACTIONS));
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));

    return;
}

/*
d_tests_mongodb_try
  Whether a call that needs a feature succeeds, for the features test,
saying what refused it where it did not and the feature is claimed.
*/
static
bool d_tests_mongodb_try(
    struct dj_mongodb* _db,
    int                _result,
    bool               _claimed,
    const char*        _what
)
{
    // the server's word, where it disagrees with the layer's
    if ( (_claimed) &&
         (_result != 0) )
    {
        printf("    %s: %d, %s\n",
               _what,
               _result,
               d_mongodb_error_message(_db));
    }

    return (_result == 0);
}

/*
d_tests_mongodb_features
  Each d_mongodb_supports answer against the server itself.
  Tests the following:
  - transactions: a transaction begins and commits iff claimed
  - change streams: a stream opens iff claimed
  - time series, clustered and pre- and post-image collections: a create
    with the option succeeds iff claimed
  - the stable API: a handle that declares version 1, strict, has
    buildInfo, which is outside it, refused (323, APIStrictError) iff
    claimed; a server before it takes the declaration and ignores it
  - snapshot reads: a find at readConcern snapshot succeeds iff claimed
  - the client bulk write: succeeds iff claimed, else D_DB_UNSUPPORTED
  - hidden and wildcard indexes: a create succeeds iff claimed
  - a feature outside the enum is not supported
*/
static
void d_tests_mongodb_features(void)
{
    char                           name[64];
    struct dj_mongodb* const        db     = d_tests_mongodb_begin("features",
                                                                  name,
                                                                  sizeof(name));
    struct dj_mongodb_change_stream* stream = NULL;
    struct dj_mongodb*              api    = NULL;
    struct dj_db_rows               rows;
    bool                           claim  = false;
    int                            result = 0;
    struct dj_mongodb_setting       declare[] =
    {
        { D_MONGODB_SETTING_SERVER_API, D_MONGODB_API_STRICT, "1", NULL }
    };
    struct dj_mongodb_options       options = { NULL, NULL, NULL, NULL,
                                               0u,   NULL, NULL, NULL };

    printf("features\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    // transactions
    claim  = d_mongodb_supports(db, D_MONGODB_FEATURE_TRANSACTIONS);
    result = d_mongodb_begin_transaction(db);
    result = (result == 0) ? d_mongodb_commit(db) : result;
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_try(db, result, claim,
                                              "transaction") == claim);

    // change streams
    claim  = d_mongodb_supports(db, D_MONGODB_FEATURE_CHANGE_STREAMS);
    result = d_mongodb_watch_database(db, NULL, NULL, NULL, &stream);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_try(db, result, claim,
                                              "change stream") == claim);
    d_mongodb_change_stream_close(stream);

    // collections of three kinds
    claim  = d_mongodb_supports(db, D_MONGODB_FEATURE_TIME_SERIES);
    result = d_mongodb_create_collection(db, NULL, "ts",
                                         "{\"timeseries\": "
                                         "{\"timeField\": \"t\"}}", 0u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_try(db, result, claim,
                                              "time series") == claim);
    claim  = d_mongodb_supports(db, D_MONGODB_FEATURE_CLUSTERED);
    result = d_mongodb_create_collection(db, NULL, "cl",
                                         "{\"clusteredIndex\": {\"key\": "
                                         "{\"_id\": 1}, \"unique\": true}}",
                                         0u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_try(db, result, claim,
                                              "clustered") == claim);
    claim  = d_mongodb_supports(db, D_MONGODB_FEATURE_PRE_POST_IMAGES);
    result = d_mongodb_create_collection(db, NULL, "pp",
                                         "{\"changeStreamPreAndPostImages\": "
                                         "{\"enabled\": true}}", 0u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_try(db, result, claim,
                                              "pre- and post-images") ==
                          claim);

    // the stable API
    claim       = d_mongodb_supports(db, D_MONGODB_FEATURE_STABLE_API);
    options.uri = g_uri;
    result      = d_mongodb_connect_with(&options, declare, 1u, &api, NULL);
    result      = (result == 0) ? d_mongodb_exec(api, "admin",
                                                 "{\"buildInfo\": 1}")
                                : result;
    D_TESTS_MONGODB_CHECK((result == 323) == claim);

    // the server's word, where it disagrees with the layer's
    if ((result == 323) != claim)
    {
        printf("    stable API: %d, %s\n",
               result,
               (api) ? d_mongodb_error_message(api) : "");
    }

    d_mongodb_close(api);

    // snapshot reads
    claim  = d_mongodb_supports(db, D_MONGODB_FEATURE_SNAPSHOT_READS);
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "s", "{}", NULL));
    result = d_mongodb_find(db, NULL, "s", NULL,
                            "{\"readConcern\": {\"level\": \"snapshot\"}}",
                            &rows);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_try(db, result, claim,
                                              "snapshot read") == claim);

    if (result == 0)
    {
        d_db_rows_free(&rows);
    }

    // the client bulk write
    claim  = d_mongodb_supports(db, D_MONGODB_FEATURE_BULK_WRITE);
    result = d_mongodb_client_bulk_write(
                 db,
                 "[{\"deleteMany\": {\"namespace\": \"admin.nothing_here\", "
                 "\"filter\": {}}}]",
                 0u,
                 NULL,
                 NULL);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_try(db, result, claim,
                                              "client bulk write") == claim);
    D_TESTS_MONGODB_CHECK( (claim) ||
                           (result == D_DB_UNSUPPORTED) );

    // indexes of two kinds
    claim  = d_mongodb_supports(db, D_MONGODB_FEATURE_HIDDEN_INDEXES);
    result = d_mongodb_create_index(db, NULL, "i", "h", "{\"h\": 1}",
                                    "{\"hidden\": true}", 0u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_try(db, result, claim,
                                              "hidden index") == claim);
    claim  = d_mongodb_supports(db, D_MONGODB_FEATURE_WILDCARD_INDEXES);
    result = d_mongodb_create_index(db, NULL, "i", "w", "{\"$**\": 1}",
                                    NULL, 0u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_try(db, result, claim,
                                              "wildcard index") == claim);

    // no such feature
    D_TESTS_MONGODB_CHECK(!d_mongodb_supports(
                              db,
                              (enum dj_mongodb_feature)99));
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_errors
  The handle's last failure.
  Tests the following:
  - a fresh handle has none: 0, "", "", no label, no reply
  - a duplicate key: 11000, the server's message, DuplicateKey (which a
    write error carries no codeName for), its reply with writeErrors
  - a command of no such name: 59, CommandNotFound from the reply
  - a refusal of the layer's: its code and words, no name, no reply
  - a success leaves the failure as it was
  - labels: none of no such name, and a NULL label is none
  - a NULL handle answers 0, "", "", false; NULL arguments are refused
*/
static
void d_tests_mongodb_errors(void)
{
    char                    name[64];
    struct dj_mongodb* const db    = d_tests_mongodb_begin("errors",
                                                          name,
                                                          sizeof(name));
    char*                   reply = NULL;

    printf("errors\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    // none yet: the begin's drop and use succeeded
    D_TESTS_MONGODB_CHECK(d_mongodb_error_code(db) == 0);
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_message(db), "") == 0);
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_name(db), "") == 0);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_error_reply(db, &reply),
                           D_DB_NOT_FOUND);

    // a duplicate key
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c",
                                                "{\"_id\": 1}", NULL));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_one(db, NULL, "c",
                                                "{\"_id\": 1}", NULL),
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_MONGODB_CHECK(d_mongodb_error_code(db) == 11000);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(d_mongodb_error_message(db),
                                                   "E11000"));
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_name(db), "DuplicateKey") ==
                          0);
    D_TESTS_MONGODB_OK(db, d_mongodb_error_reply(db, &reply));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(reply, "writeErrors"));
    d_db_free(reply);
    D_TESTS_MONGODB_CHECK(!d_mongodb_error_has_label(db, "NoSuchLabel"));
    D_TESTS_MONGODB_CHECK(!d_mongodb_error_has_label(db, NULL));

    // a success leaves it
    D_TESTS_MONGODB_OK(db, d_mongodb_ping(db));
    D_TESTS_MONGODB_CHECK(d_mongodb_error_code(db) == 11000);

    // a command of no such name
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(db, NULL, "{\"noSuchCommand\": 1}"),
                           D_MONGODB_ERROR_COMMAND_NOT_FOUND);
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_name(db),
                                 "CommandNotFound") == 0);
    D_TESTS_MONGODB_OK(db, d_mongodb_error_reply(db, &reply));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(reply, "\"codeName\""));
    d_db_free(reply);

    // the layer's own refusal
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(db, NULL, "[1]"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(d_mongodb_error_code(db) == D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(d_mongodb_error_message(db),
                                                   "array"));
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_name(db), "") == 0);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_error_reply(db, &reply),
                           D_DB_NOT_FOUND);

    // NULL
    D_TESTS_MONGODB_CHECK(d_mongodb_error_code(NULL) == 0);
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_message(NULL), "") == 0);
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_name(NULL), "") == 0);
    D_TESTS_MONGODB_CHECK(!d_mongodb_error_has_label(NULL, "x"));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_error_reply(NULL, &reply),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_error_reply(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(d_mongodb_native(NULL) == NULL);
    D_TESTS_MONGODB_CHECK(d_mongodb_native(db) != NULL);
    d_tests_mongodb_end(db, name);

    return;
}


//==============================================================================
// 2.  CONNECTIONS
//==============================================================================

/*
d_tests_mongodb_native_uri
  The connection string a handle's client was made from, as the driver
parsed it.
*/
static
const mongoc_uri_t* d_tests_mongodb_native_uri(
    struct dj_mongodb* _db
)
{
    return mongoc_client_get_uri((mongoc_client_t*)d_mongodb_native(_db));
}

/*
d_tests_mongodb_connect_fields
  d_mongodb_connect from the fields of the run's connection string, read
apart by the driver.
  Tests the following:
  - host, port, user, password, auth_source and replica_set make a handle
    that pings, of the deployment's topology
  - database is the handle's current one
  - a handle made without a database has none
*/
static
void d_tests_mongodb_connect_fields(void)
{
    mongoc_uri_t* const uri = mongoc_uri_new(g_uri);

    printf("connect: fields\n");

    // the string, read apart
    if ( (!uri) ||
         (!mongoc_uri_get_hosts(uri)) )
    {
        d_tests_mongodb_skip("connect from fields",
                             "the run's string has no host the driver reads");
        mongoc_uri_destroy(uri);

        return;
    }

    const mongoc_host_list_t* const host    = mongoc_uri_get_hosts(uri);
    struct dj_mongodb_options        options =
    {
        host->host,
        mongoc_uri_get_username(uri),
        mongoc_uri_get_password(uri),
        "fields_db",
        (unsigned)host->port,
        mongoc_uri_get_auth_source(uri),
        mongoc_uri_get_replica_set(uri),
        NULL
    };
    struct dj_mongodb*               db      = NULL;
    char*                           current = NULL;

    // a handle from the fields, its database current
    D_TESTS_MONGODB_OK(NULL, d_mongodb_connect(&options, &db));
    D_TESTS_MONGODB_OK(db, d_mongodb_ping(db));
    D_TESTS_MONGODB_OK(db, d_mongodb_current_database(db, &current));
    D_TESTS_MONGODB_CHECK( (current) &&
                           (strcmp(current, "fields_db") == 0) );
    d_db_free(current);
    D_TESTS_MONGODB_CHECK(d_mongodb_server_topology(db) !=
                          D_MONGODB_TOPOLOGY_UNKNOWN);
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));

    // without a database, none
    options.database = NULL;
    db               = NULL;
    D_TESTS_MONGODB_OK(NULL, d_mongodb_connect(&options, &db));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_current_database(db, &current),
                           D_DB_NOT_FOUND);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(db, NULL, "{\"ping\": 1}"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));
    mongoc_uri_destroy(uri);

    return;
}

/*
d_tests_mongodb_connect_failures
  What connecting refuses, and the reasons it gives.
  Tests the following:
  - NULL for the handle's place, a NULL string, settings NULL for a count,
    a host beside a string: D_DB_INVALID_ARGUMENT, the handle NULL
  - a string the driver cannot read: its code, packed, and its words
  - a wrong password: the driver's code, and the reason in its words
  - no server: the driver's code once the selection timeout passes, and
    only it is unreachable
  - a message is NULL on success
*/
static
void d_tests_mongodb_connect_failures(void)
{
    struct dj_mongodb*        db      = (struct dj_mongodb*)&db;
    char*                    message = NULL;
    struct dj_mongodb_options options = { NULL, NULL, NULL, NULL,
                                         0u,   NULL, NULL, NULL };
    struct dj_mongodb_setting quick[] =
    {
        { D_MONGODB_SETTING_SERVER_SELECTION_TIMEOUT, 400ul, NULL, NULL }
    };

    printf("connect: failures\n");

    // arguments
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_connect(NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_connect_uri(NULL, &db),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(db == NULL);
    options.uri  = g_uri;
    options.host = "localhost";
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_connect_with(&options, NULL, 0u, &db,
                                                  &message),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(message, "beside"));
    d_db_free(message);
    options.host = NULL;
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_connect_with(&options, NULL, 1u, &db,
                                                  NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_connect_with(&options, NULL, 0u, NULL,
                                                  &message),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(message != NULL);
    d_db_free(message);

    // a string the driver cannot read
    options.uri = "mongodb://";
    message     = NULL;
    const int unread = d_mongodb_connect_with(&options, NULL, 0u, &db,
                                              &message);

    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(unread));
    D_TESTS_MONGODB_CHECK( (db == NULL) &&
                           (message != NULL) );
    d_db_free(message);

    // a wrong password
    options.uri      = g_uri;
    options.user     = "dtm_no_such_user";
    options.password = "wrong";
    message          = NULL;

    const int refused = d_mongodb_connect_with(&options, quick, 1u, &db,
                                               &message);

    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(refused));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(message, "uthenticat"));
    D_TESTS_MONGODB_CHECK(db == NULL);
    d_db_free(message);

    // no server at all
    options.uri      = "mongodb://127.0.0.1:1/";
    options.user     = NULL;
    options.password = NULL;
    message          = NULL;

    const int nothing = d_mongodb_connect_with(&options, quick, 1u, &db,
                                               &message);

    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(nothing));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(message, "suitable"));
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_unreachable(nothing));
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_unreachable(refused));
    D_TESTS_MONGODB_CHECK(!d_mongodb_result_is_unreachable(unread));
    d_db_free(message);

    // success leaves no message
    options.uri = g_uri;
    message     = (char*)&message;
    D_TESTS_MONGODB_OK(NULL, d_mongodb_connect_with(&options, NULL, 0u, &db,
                                                    &message));
    D_TESTS_MONGODB_CHECK(message == NULL);
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));

    return;
}

/*
d_tests_mongodb_connect_settings
  d_mongodb_connect_with's settings, each read back from the driver's
client.
  Tests the following:
  - the layer's defaults: a selection timeout of the knob's, no app name
  - each number setting replaces the string's, and each switch
  - an app name, compressors, and an auth mechanism where the run's string
    names a user: the driver refuses one without
  - a URI_OPTION, read as the string reads it: w=majority is the handle's
    write concern; one the driver cannot read is its code, packed; one it
    does not know it passes over
  - refusals: a number past 32 bits, a URI_OPTION without its value,
    authentication properties without a colon, a declared API of no such
    version or flags, a setting of no such name; loadBalanced beside a
    replica set's name, the driver's
  - a declared strict API: the handle pings, and its version comes from its
    wire version, its string empty, where the server is 5.0 or later
*/
static
void d_tests_mongodb_connect_settings(void)
{
    struct dj_mongodb*        db      = NULL;
    char*                    concern = NULL;
    struct dj_mongodb_options options = { NULL, NULL, NULL, NULL,
                                         0u,   NULL, NULL, g_uri };
    struct dj_mongodb_setting several[] =
    {
        { D_MONGODB_SETTING_SERVER_SELECTION_TIMEOUT, 7000ul, NULL, NULL },
        { D_MONGODB_SETTING_CONNECT_TIMEOUT, 6000ul, NULL, NULL },
        { D_MONGODB_SETTING_SOCKET_TIMEOUT, 0ul, NULL, NULL },
        { D_MONGODB_SETTING_APP_NAME, 0ul, "djinterp tests", NULL },
        { D_MONGODB_SETTING_RETRY_WRITES, 0ul, NULL, NULL },
        { D_MONGODB_SETTING_RETRY_READS, 0ul, NULL, NULL },
        { D_MONGODB_SETTING_COMPRESSORS, 0ul, "zlib", NULL },
        { D_MONGODB_SETTING_URI_OPTION, 0ul, "w", "majority" },
        { D_MONGODB_SETTING_URI_OPTION, 0ul, "dtmNoSuchOption", "1" },
        // last, so that a string without a user leaves it out
        { D_MONGODB_SETTING_AUTH_MECHANISM, 0ul, "SCRAM-SHA-256", NULL }
    };
    size_t                   count   = sizeof(several) / sizeof(several[0]);
    mongoc_uri_t* const      named   = mongoc_uri_new(g_uri);

    // a mechanism without a user, the driver refuses
    if ( (!named) ||
         (!mongoc_uri_get_username(named)) )
    {
        --count;
    }

    mongoc_uri_destroy(named);

    printf("connect: settings\n");

    // the layer's defaults
    D_TESTS_MONGODB_OK(NULL, d_mongodb_connect_with(&options, NULL, 0u, &db,
                                                    NULL));
    D_TESTS_MONGODB_CHECK(mongoc_uri_get_option_as_int32(
                              d_tests_mongodb_native_uri(db),
                              MONGOC_URI_SERVERSELECTIONTIMEOUTMS,
                              0) == D_CFG_MONGODB_SERVER_SELECTION_TIMEOUT_MS);
    D_TESTS_MONGODB_CHECK(mongoc_uri_get_appname(
                              d_tests_mongodb_native_uri(db)) == NULL);
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));

    // each setting, read back
    D_TESTS_MONGODB_OK(NULL, d_mongodb_connect_with(&options,
                                                    several,
                                                    count,
                                                    &db,
                                                    NULL));

    const mongoc_uri_t* const uri = d_tests_mongodb_native_uri(db);

    D_TESTS_MONGODB_CHECK(mongoc_uri_get_option_as_int32(
                              uri, MONGOC_URI_SERVERSELECTIONTIMEOUTMS, 0) ==
                          7000);
    D_TESTS_MONGODB_CHECK(mongoc_uri_get_option_as_int32(
                              uri, MONGOC_URI_CONNECTTIMEOUTMS, 0) == 6000);
    // the driver's getter answers its fallback for a 0, so the option is
    // read from the options themselves
    bson_iter_t socket;

    D_TESTS_MONGODB_CHECK( (bson_iter_init_find_case(
                                &socket,
                                mongoc_uri_get_options(uri),
                                MONGOC_URI_SOCKETTIMEOUTMS)) &&
                           (BSON_ITER_HOLDS_INT32(&socket))   &&
                           (bson_iter_int32(&socket) == 0) );
    D_TESTS_MONGODB_CHECK(!mongoc_uri_get_option_as_bool(
                              uri, MONGOC_URI_RETRYWRITES, true));
    D_TESTS_MONGODB_CHECK(!mongoc_uri_get_option_as_bool(
                              uri, MONGOC_URI_RETRYREADS, true));
    D_TESTS_MONGODB_CHECK( (mongoc_uri_get_appname(uri)) &&
                           (strcmp(mongoc_uri_get_appname(uri),
                                   "djinterp tests") == 0) );
    D_TESTS_MONGODB_OK(db, d_mongodb_ping(db));
    D_TESTS_MONGODB_OK(db, d_mongodb_get_write_concern(db, &concern));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(concern, "majority"));
    d_db_free(concern);
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));

    // refusals
    {
        struct dj_mongodb_setting refused[][1] =
        {
            { { D_MONGODB_SETTING_CONNECT_TIMEOUT, 0x80000000ul, NULL, NULL } },
            { { D_MONGODB_SETTING_URI_OPTION, 0ul, "w", NULL } },
            { { D_MONGODB_SETTING_URI_OPTION, 0ul, NULL, "1" } },
            { { D_MONGODB_SETTING_AUTH_MECHANISM_PROPERTIES, 0ul, "NOCOLON",
                NULL } },
            { { D_MONGODB_SETTING_SERVER_API, 0ul, "2", NULL } },
            { { D_MONGODB_SETTING_SERVER_API, 4ul, "1", NULL } },
            { { D_MONGODB_SETTING_APP_NAME, 0ul, NULL, NULL } },
            { { (enum dj_mongodb_setting_name)99, 0ul, NULL, NULL } }
        };

        for (size_t i = 0u; i < (sizeof(refused) / sizeof(refused[0])); ++i)
        {
            D_TESTS_MONGODB_RESULT(NULL,
                                   d_mongodb_connect_with(&options,
                                                          refused[i],
                                                          1u,
                                                          &db,
                                                          NULL),
                                   D_DB_INVALID_ARGUMENT);
            D_TESTS_MONGODB_CHECK(db == NULL);
        }
    }

    // the driver's own refusals
    {
        struct dj_mongodb_setting unread[] =
        {
            { D_MONGODB_SETTING_URI_OPTION, 0ul, "serverSelectionTimeoutMS",
              "abc" }
        };
        struct dj_mongodb_setting balanced[] =
        {
            { D_MONGODB_SETTING_LOAD_BALANCED, 1ul, NULL, NULL }
        };

        D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(
                                  d_mongodb_connect_with(&options, unread, 1u,
                                                         &db, NULL)));

        mongoc_uri_t* const parsed = mongoc_uri_new(g_uri);

        if ( (parsed) &&
             (mongoc_uri_get_replica_set(parsed) != NULL) )
        {
            D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(
                                      d_mongodb_connect_with(&options,
                                                             balanced,
                                                             1u,
                                                             &db,
                                                             NULL)));
        }

        mongoc_uri_destroy(parsed);
    }

    // a strict declared API, which keeps buildInfo out
    db = d_tests_mongodb_connect(g_uri);

    if ( (db) &&
         (d_mongodb_supports(db, D_MONGODB_FEATURE_STABLE_API)) )
    {
        const unsigned long      release = d_mongodb_server_version_number(db);
        struct dj_mongodb*        strict  = NULL;
        struct dj_mongodb_setting declare[] =
        {
            { D_MONGODB_SETTING_SERVER_API,
              D_MONGODB_API_STRICT | D_MONGODB_API_DEPRECATION_ERRORS,
              "1",
              NULL }
        };

        D_TESTS_MONGODB_OK(NULL, d_mongodb_connect_with(&options, declare,
                                                        1u, &strict, NULL));
        D_TESTS_MONGODB_OK(strict, d_mongodb_ping(strict));
        D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_server_version(strict), "") ==
                              0);
        D_TESTS_MONGODB_CHECK(d_mongodb_server_version_number(strict) ==
                              ((release / 100ul) * 100ul));
        D_TESTS_MONGODB_CHECK(d_mongodb_error_code(strict) == 0);
        D_TESTS_MONGODB_OK(strict, d_mongodb_close(strict));
    }

    d_mongodb_close(db);

    return;
}

/*
d_tests_mongodb_adopt_error_api
  The driver's error API of a client adopted: set where it can be, taken
where it is the second, refused where it is the first for good.
*/
static
void d_tests_mongodb_adopt_error_api(void)
{
    mongoc_client_t*      client = mongoc_client_new(g_uri);
    mongoc_uri_t* const   uri    = mongoc_uri_new(g_uri);
    mongoc_client_pool_t* pool   = mongoc_client_pool_new(uri);
    struct dj_mongodb*     db     = NULL;
    struct dj_mongodb*     other  = NULL;

    // never set: the second, its codes the server's
    D_TESTS_MONGODB_OK(NULL, d_mongodb_adopt(client, true, &db));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(db, "admin", "{\"dtmNo\": 1}"),
                           D_MONGODB_ERROR_COMMAND_NOT_FOUND);

    // another handle's client, in the second already
    D_TESTS_MONGODB_OK(NULL, d_mongodb_adopt(d_mongodb_native(db), false,
                                             &other));
    D_TESTS_MONGODB_RESULT(other,
                           d_mongodb_exec(other, "admin", "{\"dtmNo\": 1}"),
                           D_MONGODB_ERROR_COMMAND_NOT_FOUND);
    D_TESTS_MONGODB_OK(other, d_mongodb_close(other));
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));

    // set to the first by its maker
    client = mongoc_client_new(g_uri);
    D_TESTS_MONGODB_CHECK(mongoc_client_set_error_api(
                              client,
                              MONGOC_ERROR_API_VERSION_LEGACY));
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_adopt(client, true, &db),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(db == NULL);
    mongoc_client_destroy(client);

    // a pool's, left in the first, and one set to the second
    client = mongoc_client_pool_pop(pool);
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_adopt(client, false, &db),
                           D_DB_INVALID_ARGUMENT);
    mongoc_client_pool_push(pool, client);
    mongoc_client_pool_destroy(pool);
    pool = mongoc_client_pool_new(uri);
    D_TESTS_MONGODB_CHECK(mongoc_client_pool_set_error_api(
                              pool,
                              MONGOC_ERROR_API_VERSION_2));
    client = mongoc_client_pool_pop(pool);
    D_TESTS_MONGODB_OK(NULL, d_mongodb_adopt(client, false, &db));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(db, "admin", "{\"dtmNo\": 1}"),
                           D_MONGODB_ERROR_COMMAND_NOT_FOUND);
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));
    mongoc_client_pool_push(pool, client);
    mongoc_client_pool_destroy(pool);
    mongoc_uri_destroy(uri);

    return;
}

/*
d_tests_mongodb_adopt
  d_mongodb_adopt, d_mongodb_close and d_mongodb_ping.
  Tests the following:
  - an adopted client is the handle's native one, pings, and is destroyed
    with the handle when owned
  - one not owned outlives the handle, usable by the driver after
  - one that cannot reach a server fails, and stays the caller's
  - the driver's error API: a client never set is set to the second, its
    server codes the server's; one in the second already -- another
    handle's, a pool's set so -- is taken as it is; one its maker or its
    pool set to the first is D_DB_INVALID_ARGUMENT
  - NULL arguments; close of NULL is 0; ping of NULL is refused
*/
static
void d_tests_mongodb_adopt(void)
{
    mongoc_client_t* client = mongoc_client_new(g_uri);
    struct dj_mongodb* db    = NULL;
    bson_t            ping;
    bson_t            reply;
    bson_error_t      error;

    printf("adopt\n");

    // owned
    D_TESTS_MONGODB_CHECK(client != NULL);
    D_TESTS_MONGODB_OK(NULL, d_mongodb_adopt(client, true, &db));
    D_TESTS_MONGODB_CHECK(d_mongodb_native(db) == (void*)client);
    D_TESTS_MONGODB_OK(db, d_mongodb_ping(db));
    D_TESTS_MONGODB_CHECK(d_mongodb_server_version_number(db) >= 40200ul);
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));

    // not owned: the client outlives the handle
    client = mongoc_client_new(g_uri);
    D_TESTS_MONGODB_OK(NULL, d_mongodb_adopt(client, false, &db));
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));
    bson_init(&ping);
    BSON_APPEND_INT32(&ping, "ping", 1);
    D_TESTS_MONGODB_CHECK(mongoc_client_command_simple(client,
                                                       "admin",
                                                       &ping,
                                                       NULL,
                                                       &reply,
                                                       &error));
    bson_destroy(&reply);
    mongoc_client_destroy(client);

    // a client that reaches nothing stays the caller's
    client = mongoc_client_new("mongodb://127.0.0.1:1/"
                               "?serverSelectionTimeoutMS=300");
    db     = (struct dj_mongodb*)&db;
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(
                              d_mongodb_adopt(client, true, &db)));
    D_TESTS_MONGODB_CHECK(db == NULL);
    mongoc_client_destroy(client);
    bson_destroy(&ping);
    d_tests_mongodb_adopt_error_api();

    // NULL
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_adopt(NULL, true, &db),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_adopt((void*)&db, true, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_OK(NULL, d_mongodb_close(NULL));
    D_TESTS_MONGODB_RESULT(NULL,
                           d_mongodb_ping(NULL),
                           D_DB_INVALID_ARGUMENT);

    return;
}

/*
d_tests_mongodb_latest_write
  d_mongodb_changes and d_mongodb_last_insert_id.
  Tests the following:
  - a fresh handle: 0 changes and no id; a NULL handle 0 changes
  - an insert_one: 1, and its id, the layer's ObjectId
  - an update that matches and changes nothing: 0, and no id
  - an upsert: 1, and the upserted id
  - a delete: its count, and no id
  - NULL arguments are refused
*/
static
void d_tests_mongodb_latest_write(void)
{
    char                          name[64];
    struct dj_mongodb* const       db     = d_tests_mongodb_begin("latest",
                                                                 name,
                                                                 sizeof(name));
    char*                         id     = NULL;
    char*                         given  = NULL;
    struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };

    printf("latest write\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    // nothing yet
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 0);
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(NULL) == 0);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_last_insert_id(db, &id),
                           D_DB_NOT_FOUND);

    // an insert, its id the layer's
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c",
                                                "{\"n\": 1}", &given));
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 1);
    D_TESTS_MONGODB_OK(db, d_mongodb_last_insert_id(db, &id));
    D_TESTS_MONGODB_CHECK( (id) &&
                           (given) &&
                           (strcmp(id, given) == 0) );
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(id, "$oid"));
    d_db_free(id);
    d_db_free(given);

    // an update that changes nothing
    D_TESTS_MONGODB_OK(db, d_mongodb_update_one(db, NULL, "c", NULL,
                                                "{\"$set\": {\"n\": 1}}",
                                                NULL, &counts));
    D_TESTS_MONGODB_CHECK( (counts.matched == 1) &&
                           (counts.modified == 0) );
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 0);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_last_insert_id(db, &id),
                           D_DB_NOT_FOUND);

    // an upsert
    D_TESTS_MONGODB_OK(db, d_mongodb_update_one(db, NULL, "c",
                                                "{\"_id\": \"u\"}",
                                                "{\"$set\": {\"n\": 2}}",
                                                "{\"upsert\": true}",
                                                &counts));
    D_TESTS_MONGODB_CHECK(counts.upserted == 1);
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 1);
    D_TESTS_MONGODB_OK(db, d_mongodb_last_insert_id(db, &id));
    D_TESTS_MONGODB_CHECK( (id) &&
                           (strcmp(id, "\"u\"") == 0) );
    d_db_free(id);

    // a delete
    D_TESTS_MONGODB_OK(db, d_mongodb_delete_many(db, NULL, "c", NULL, NULL,
                                                 NULL));
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 2);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_last_insert_id(db, &id),
                           D_DB_NOT_FOUND);

    // NULL
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_last_insert_id(NULL, &id),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_last_insert_id(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}


//==============================================================================
// 3.  COMMANDS
//==============================================================================

/*
d_tests_mongodb_commands
  d_mongodb_exec, _exec_changes and _command, plain and formatted.
  Tests the following:
  - exec runs a command on the current database, or one named
  - exec_changes counts an insert's n, an update's modified and upserted,
    a delete's n and a findAndModify's document; a count, 0, its n
    notwithstanding; each is the handle's latest write
  - command hands back the reply in the handle's format
  - a reply with ok 0 is the server's code; a reply with write errors and
    ok 1 is the first write error's
  - the f forms write %Q and %I as JSON strings, NULL as null, numbers as
    JSON writes them
  - NULL arguments and no current database are refused
*/
static
void d_tests_mongodb_commands(void)
{
    char                    name[64];
    struct dj_mongodb* const db      = d_tests_mongodb_begin("commands",
                                                            name,
                                                            sizeof(name));
    int64_t                 changes = -1;
    char*                   reply   = NULL;
    struct dj_db_rows        rows;

    printf("commands\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    // exec, here and on another database
    D_TESTS_MONGODB_OK(db, d_mongodb_exec(db, NULL, "{\"create\": \"c\"}"));
    D_TESTS_MONGODB_CHECK(d_mongodb_has_collection(db, NULL, "c"));
    D_TESTS_MONGODB_OK(db, d_mongodb_exec(db, "admin", "{\"ping\": 1}"));

    // counts by the command
    D_TESTS_MONGODB_OK(db, d_mongodb_exec_changes(
                               db, NULL, &changes,
                               "{\"insert\": \"c\", \"documents\": "
                               "[{\"_id\": 1}, {\"_id\": 2}, {\"_id\": 3}]}"));
    D_TESTS_MONGODB_CHECK(changes == 3);
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 3);
    D_TESTS_MONGODB_OK(db, d_mongodb_exec_changes(
                               db, NULL, &changes,
                               "{\"update\": \"c\", \"updates\": ["
                               "{\"q\": {\"_id\": 1}, \"u\": "
                               "{\"$set\": {\"a\": 1}}}, "
                               "{\"q\": {\"_id\": 2}, \"u\": "
                               "{\"$set\": {}}}, "
                               "{\"q\": {\"_id\": 9}, \"u\": "
                               "{\"$set\": {\"a\": 9}}, \"upsert\": true}]}"));
    D_TESTS_MONGODB_CHECK(changes == 2);
    D_TESTS_MONGODB_OK(db, d_mongodb_last_insert_id(db, &reply));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(reply, "\"9\""));
    d_db_free(reply);
    D_TESTS_MONGODB_OK(db, d_mongodb_exec_changes(
                               db, NULL, &changes,
                               "{\"findAndModify\": \"c\", \"query\": "
                               "{\"_id\": 3}, \"update\": "
                               "{\"$set\": {\"b\": 1}}}"));
    D_TESTS_MONGODB_CHECK(changes == 1);
    D_TESTS_MONGODB_OK(db, d_mongodb_exec_changes(
                               db, NULL, &changes,
                               "{\"delete\": \"c\", \"deletes\": "
                               "[{\"q\": {\"_id\": {\"$gt\": 2}}, "
                               "\"limit\": 0}]}"));
    D_TESTS_MONGODB_CHECK(changes == 2);
    D_TESTS_MONGODB_OK(db, d_mongodb_exec_changes(db, NULL, &changes,
                                                  "{\"count\": \"c\"}"));
    D_TESTS_MONGODB_CHECK(changes == 0);

    // the reply
    D_TESTS_MONGODB_OK(db, d_mongodb_command(db, NULL, &reply,
                                             "{\"count\": \"c\"}"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(reply,
                                                   "\"n\" : { \"$numberInt\" "
                                                   ": \"2\" }"));
    d_db_free(reply);

    // failures in the reply: ok 0, and write errors under ok 1
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(db, NULL, "{\"noSuchCommand\": 1}"),
                           D_MONGODB_ERROR_COMMAND_NOT_FOUND);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec_changes(
                               db, NULL, &changes,
                               "{\"insert\": \"c\", \"documents\": "
                               "[{\"_id\": 1}]}"),
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_MONGODB_CHECK(changes == 0);

    // the f forms
    D_TESTS_MONGODB_OK(db, d_mongodb_execf(
                               db, NULL,
                               "{\"insert\": %Q, \"documents\": [{\"_id\": "
                               "%d, \"s\": %Q, \"n\": %s, \"x\": %g, %I: "
                               "%lld}]}",
                               "c", 10, "q\"\\\n\t\x01 \xc3\xa9", NULL, 0.25,
                               "k", 42ll));
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", "{\"_id\": 10}",
                                          NULL, &rows));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u),
                              "\"s\" : \"q\\\"\\\\\\n\\t\\u0001 \xc3\xa9\""));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u),
                              "\"n\" : null"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u),
                              "\"x\" : { \"$numberDouble\" : \"0.25\" }"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u),
                              "\"k\" : { \"$numberInt\" : \"42\" }"));
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_execf_changes(
                               db, NULL, &changes,
                               "{\"delete\": %Q, \"deletes\": [{\"q\": "
                               "{\"_id\": %u}, \"limit\": 1}]}",
                               "c", 10u));
    D_TESTS_MONGODB_CHECK(changes == 1);
    D_TESTS_MONGODB_OK(db, d_mongodb_commandf(db, "admin", &reply,
                                              "{\"ping\": %d}", 1));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(reply, "\"ok\""));
    d_db_free(reply);

    // values the formats cannot write, and formats the layer cannot read
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_execf(db, NULL, "{\"ping\": %Q}",
                                           "bad\xff"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_execf(db, NULL, "{\"ping\": %T}", "a",
                                           "b"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_execf(db, NULL, "{\"ping\": %I}",
                                           (const char*)NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_execf(db, NULL, "{\"ping\": %5d}", 1),
                           D_DB_INVALID_ARGUMENT);

    // NULL arguments, and no database
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(NULL, NULL, "{\"ping\": 1}"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(db, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_execf(db, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec_changes(db, NULL, NULL,
                                                  "{\"ping\": 1}"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_command(db, NULL, NULL,
                                             "{\"ping\": 1}"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(db, "", "{\"ping\": 1}"),
                           D_MONGODB_ERROR_INVALID_NAMESPACE);
    D_TESTS_MONGODB_OK(db, d_mongodb_use_database(db, NULL));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(db, NULL, "{\"ping\": 1}"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_OK(db, d_mongodb_use_database(db, name));
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_json
  How the layer reads the JSON it is given.
  Tests the following:
  - Extended JSON of every form: canonical, relaxed, legacy ($date with a
    string, $oid), each value of its own type
  - a document is one: an array where one is due, text after it -- a
    comma, a second document, a word -- and empty text are
    D_DB_INVALID_ARGUMENT; text that is not JSON, or JSON that ends too
    soon, is libbson's code, packed, in its JSON domain, on every release:
    what libbson itself lets through too -- n, NaN, Infinity, -01, a word,
    a trailing comma, a raw tab -- its message counting bytes from the text
    given; a field 98 documents deep is read, 99 refused alike, as
    libbson's reader refuses it
  - space around the document is passed over
*/
static
void d_tests_mongodb_json(void)
{
    char                    name[64];
    struct dj_mongodb* const db   = d_tests_mongodb_begin("json",
                                                         name,
                                                         sizeof(name));
    struct dj_db_rows        rows;

    printf("json\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    // each form
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(
                               db, NULL, "c",
                               " \n {\"_id\": 1, \"l\": {\"$numberLong\": "
                               "\"5\"}, \"r\": 2.5, \"d\": {\"$date\": "
                               "\"2020-01-02T03:04:05Z\"}, \"o\": {\"$oid\": "
                               "\"0123456789abcdef01234567\"}} \t",
                               NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", NULL, NULL, &rows));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u),
                              "\"l\" : { \"$numberLong\" : \"5\" }"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u),
                              "\"$date\" : { \"$numberLong\" : "
                              "\"1577934245000\" }"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u),
                              "0123456789abcdef01234567"));
    d_db_rows_free(&rows);

    // not one document
    {
        static const char* const NOT_ONE[] =
        {
            "[{\"a\": 1}]", "{\"a\": 1},", "{\"a\": 1}{\"b\": 2}",
            "{\"a\": 1} x", "", "   ", "5", "\"s\"", "null"
        };

        for (size_t i = 0u; i < (sizeof(NOT_ONE) / sizeof(NOT_ONE[0])); ++i)
        {
            D_TESTS_MONGODB_RESULT(db,
                                   d_mongodb_insert_one(db, NULL, "c",
                                                        NOT_ONE[i], NULL),
                                   D_DB_INVALID_ARGUMENT);
        }
    }

    // not JSON, or not all of it
    {
        static const char* const NOT_JSON[] =
        {
            "{\"a\": 1", "{\"a\": }", "{a: 1}", "{\"a\": \"\xff\"}",
            "{\"a\": {\"$numberLong\": \"x\"}}", "{\"a\": n}",
            "{\"a\": N}", "{\"a\": NaN}", "{\"a\": Infinity}",
            "{\"a\": -Infinity}", "{\"a\": -01}", "{\"a\": nothing}",
            "{\"a\": [1, nope]}", "{\"a\": [1,]}", "{\"a\": 1,}",
            "{\"a\": \"x\ty\"}", "{\"a\": \"\\x\"}", "{\"a\": 1.}",
            "{\"a\": [1 2]}"
        };

        for (size_t i = 0u; i < (sizeof(NOT_JSON) / sizeof(NOT_JSON[0]));
             ++i)
        {
            const int result = d_mongodb_insert_one(db, NULL, "c",
                                                    NOT_JSON[i], NULL);

            D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(result) ==
                                  (unsigned)(D_MONGODB_DOMAIN_BSON +
                                             BSON_ERROR_JSON));

            if (d_mongodb_result_domain(result) !=
                (unsigned)(D_MONGODB_DOMAIN_BSON + BSON_ERROR_JSON))
            {
                printf("    %s: %d, %s\n",
                       NOT_JSON[i],
                       result,
                       d_mongodb_error_message(db));
            }
        }
    }

    // the message counts from the text given
    D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(
                              d_mongodb_insert_one(db, NULL, "c",
                                                   "  {\"a\": NaN}", NULL)) ==
                          (unsigned)(D_MONGODB_DOMAIN_BSON +
                                     BSON_ERROR_JSON));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(d_mongodb_error_message(db),
                                                   "at byte 8"));

    // as deep as libbson reads, and no deeper
    for (size_t levels = 98u; levels <= 99u; ++levels)
    {
        char   deep[1024];
        size_t length = 0u;

        for (size_t level = 0u; level < levels; ++level)
        {
            memcpy(deep + length, "{\"a\": ", 6u);
            length += 6u;
        }

        deep[length] = '1';
        memset(deep + length + 1u, '}', levels);
        deep[length + 1u + levels] = '\0';

        const int result = d_mongodb_insert_one(db, NULL, "c", deep, NULL);

        D_TESTS_MONGODB_CHECK( (levels == 98u)
                                   ? (result == 0)
                                   : (d_mongodb_result_domain(result) ==
                                      (unsigned)(D_MONGODB_DOMAIN_BSON +
                                                 BSON_ERROR_JSON)) );
    }

    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(db, "c", NULL) == 2);

    // an array is refused where a document is due, and the reverse
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_aggregate(db, NULL, "c",
                                               "{\"$match\": {}}", NULL,
                                               &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find(db, NULL, "c", "[]", NULL, &rows),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_query_rows
  d_mongodb_query_rows, plain and formatted.
  Tests the following:
  - a command with a cursor: every batch read, through getMore, the rows
    of one column, "document", typed JSON, its type name "document"
  - a command without one: its reply as the one row
  - a cursor's later batch that fails is the server's code
  - NULL arguments are refused
*/
static
void d_tests_mongodb_query_rows(void)
{
    char                    name[64];
    struct dj_mongodb* const db   = d_tests_mongodb_begin("rows",
                                                         name,
                                                         sizeof(name));
    struct dj_db_rows        rows;
    char                    many[16384];
    int                     used = 0;

    printf("query rows\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    used = snprintf(many, sizeof(many),
                    "{\"insert\": \"c\", \"documents\": [");

    for (int i = 0; i < 250; ++i)
    {
        used += snprintf(many + used, sizeof(many) - (size_t)used,
                         "%s{\"_id\": %d}", (i) ? ", " : "", i);
    }

    snprintf(many + used, sizeof(many) - (size_t)used, "]}");
    D_TESTS_MONGODB_OK(db, d_mongodb_exec(db, NULL, many));

    // every batch
    D_TESTS_MONGODB_OK(db, d_mongodb_query_rows(
                               db, NULL, &rows,
                               "{\"find\": \"c\", \"batchSize\": 7, "
                               "\"sort\": {\"_id\": 1}}"));
    D_TESTS_MONGODB_CHECK(rows.row_count == 250u);
    D_TESTS_MONGODB_CHECK(rows.column_count == 1u);
    D_TESTS_MONGODB_CHECK(strcmp(rows.columns[0].name, "document") == 0);
    D_TESTS_MONGODB_CHECK(rows.columns[0].type == D_DB_TYPE_JSON);
    D_TESTS_MONGODB_CHECK(strcmp(rows.columns[0].type_name, "document") ==
                          0);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 249u),
                              "\"249\""));
    d_db_rows_free(&rows);

    // a reply without a cursor
    D_TESTS_MONGODB_OK(db, d_mongodb_query_rowsf(db, NULL, &rows,
                                                 "{\"count\": %Q}", "c"));
    D_TESTS_MONGODB_CHECK(rows.row_count == 1u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u),
                              "\"250\""));
    d_db_rows_free(&rows);

    // the server's refusal, and the arguments
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_query_rows(db, NULL, &rows,
                                                "{\"find\": \"c\", "
                                                "\"filter\": {\"$frob\": 1}}"),
                           D_MONGODB_ERROR_BAD_VALUE);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_query_rows(db, NULL, NULL,
                                                "{\"find\": \"c\"}"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_query_rows(NULL, NULL, &rows,
                                                "{\"find\": \"c\"}"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_query_rowsf(db, NULL, &rows, NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}


//==============================================================================
// 4.  DOCUMENTS
//==============================================================================

/*
d_tests_mongodb_insert
  d_mongodb_insert_one and _insert_many.
  Tests the following:
  - a document without an _id gets the layer's ObjectId, first among its
    fields, handed back; one with an _id keeps it
  - a duplicate _id is 11000
  - many go in, in order; ordered, they stop at the first failure, and the
    count says how many went in; unordered, they go on past it
  - an element that is not a document, no elements, a document where the
    array is due, unknown flags: D_DB_INVALID_ARGUMENT, the count 0
  - NULL arguments, and an empty collection's name, which is 73; a NULL
    handle's count is 0 too
*/
static
void d_tests_mongodb_insert(void)
{
    char                    name[64];
    struct dj_mongodb* const db       = d_tests_mongodb_begin("insert",
                                                             name,
                                                             sizeof(name));
    char*                   id       = NULL;
    int64_t                 inserted = -1;
    struct dj_db_rows        rows;

    printf("insert\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    // one, its _id the layer's and first
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c",
                                                "{\"a\": 1}", &id));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(id, "{ \"$oid\" : \""));
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", NULL, NULL, &rows));
    D_TESTS_MONGODB_CHECK(strncmp(d_tests_mongodb_row(&rows, 0u),
                                  "{ \"_id\" : { \"$oid\"", 18u) == 0);
    D_TESTS_MONGODB_CHECK( (id) &&
                           (strstr(d_tests_mongodb_row(&rows, 0u), id)) );
    d_db_rows_free(&rows);
    d_db_free(id);
    id = NULL;

    // one with its own _id
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c",
                                                "{\"_id\": \"x\"}", &id));
    D_TESTS_MONGODB_CHECK( (id) &&
                           (strcmp(id, "\"x\"") == 0) );
    d_db_free(id);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_one(db, NULL, "c",
                                                "{\"_id\": \"x\"}", NULL),
                           D_MONGODB_ERROR_DUPLICATE_KEY);

    // many, ordered and not
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(db, NULL, "m",
                                                 "[{\"_id\": 1}, {\"_id\": 2}, "
                                                 "{}]", 0u, &inserted));
    D_TESTS_MONGODB_CHECK(inserted == 3);
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 3);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_many(db, NULL, "m",
                                                 "[{\"_id\": 3}, {\"_id\": 1}, "
                                                 "{\"_id\": 4}]",
                                                 0u, &inserted),
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_MONGODB_CHECK(inserted == 1);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(db, "m", "{\"_id\": 4}") ==
                          0);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_many(db, NULL, "m",
                                                 "[{\"_id\": 5}, {\"_id\": 1}, "
                                                 "{\"_id\": 6}]",
                                                 D_MONGODB_UNORDERED,
                                                 &inserted),
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_MONGODB_CHECK(inserted == 2);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(db, "m", "{\"_id\": 6}") ==
                          1);

    // refusals
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_many(db, NULL, "m",
                                                 "[{\"_id\": 7}, 8]", 0u,
                                                 NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_many(db, NULL, "m", "[]", 0u,
                                                 &inserted),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(inserted == 0);
    inserted = -1;
    D_TESTS_MONGODB_CHECK(d_mongodb_insert_many(NULL, NULL, "m", "[{}]", 0u,
                                                &inserted) ==
                          D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(inserted == 0);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_many(db, NULL, "m",
                                                 "{\"_id\": 7}", 0u, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_many(db, NULL, "m", "[{}]",
                                                 0x8000u, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_one(NULL, NULL, "c", "{}", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_one(db, NULL, NULL, "{}", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_one(db, NULL, "", "{}", NULL),
                           D_MONGODB_ERROR_INVALID_NAMESPACE);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_one(db, NULL, "c", NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_find
  d_mongodb_find and _find_one.
  Tests the following:
  - a filter, and the options of a find: sort, limit, skip, projection
  - NULL and "{}" filters match every document
  - a collection that is not there has none
  - find_one hands back the first in the options' sort, its limit
    replaced; none matched is D_DB_NOT_FOUND
  - the server's refusal of an operator; NULL arguments
*/
static
void d_tests_mongodb_find(void)
{
    char                    name[64];
    struct dj_mongodb* const db   = d_tests_mongodb_begin("find",
                                                         name,
                                                         sizeof(name));
    struct dj_db_rows        rows;

    printf("find\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(
                               db, NULL, "c",
                               "[{\"_id\": 1, \"g\": \"a\"}, {\"_id\": 2, "
                               "\"g\": \"b\"}, {\"_id\": 3, \"g\": \"a\"}, "
                               "{\"_id\": 4, \"g\": \"b\"}]",
                               0u, NULL));

    // a filter and options
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", "{\"g\": \"a\"}",
                                          "{\"sort\": {\"_id\": -1}, "
                                          "\"projection\": {\"g\": 0}}",
                                          &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_row(&rows, 0u),
                                 "{ \"_id\" : { \"$numberInt\" : \"3\" } }") ==
                          0);
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", "{}",
                                          "{\"skip\": 1, \"limit\": 2, "
                                          "\"sort\": {\"_id\": 1}}", &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u), "\"2\""));
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", NULL, NULL, &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 4u);
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "none", NULL, NULL,
                                          &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 0u);
    d_db_rows_free(&rows);

    // the first
    D_TESTS_MONGODB_OK(db, d_mongodb_find_one(db, NULL, "c",
                                              "{\"g\": \"b\"}",
                                              "{\"sort\": {\"_id\": -1}, "
                                              "\"limit\": 3}", &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 1u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u), "\"4\""));
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find_one(db, NULL, "c",
                                              "{\"g\": \"z\"}", NULL, &rows),
                           D_DB_NOT_FOUND);

    // refusals
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find(db, NULL, "c", "{\"$frob\": 1}",
                                          NULL, &rows),
                           D_MONGODB_ERROR_BAD_VALUE);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find(db, NULL, "c", NULL, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find_one(db, NULL, "c", NULL, NULL,
                                              NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find(NULL, NULL, "c", NULL, NULL, &rows),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_cell
  A row's cell as text, or "" for no such cell or a value not text; NULL
for a NULL.
*/
static
const char* d_tests_mongodb_cell(
    const struct dj_db_rows* _rows,
    size_t                   _row,
    size_t                   _column
)
{
    const struct dj_db_value* const value = d_db_rows_value(_rows,
                                                           _row,
                                                           _column);

    // NULL, text, or nothing to read
    if ( (value) &&
         (value->type == D_DB_VALUE_NULL) )
    {
        return NULL;
    }

    return ( (value) &&
             (value->type == D_DB_VALUE_TEXT) ) ? value->as.text : "";
}

/*
d_tests_mongodb_find_keyed
  d_mongodb_find_keyed.
  Tests the following:
  - two columns, "_id" and "document", both typed JSON
  - each _id a filter, {"_id": ...}, canonical, that finds its document
    alone; an ObjectId, a string and a document among the ids
  - the document canonical whatever the handle's format: an int64 that fits
    32 bits stays a $numberLong; written back, it reads back the same
  - a filter and the options; a projection without _id leaves the key NULL
  - a collection that is not there has none; inside a transaction it sees
    the transaction's own writes
  - the server's refusal; NULL arguments
*/
static
void d_tests_mongodb_find_keyed(void)
{
    char                    name[64];
    struct dj_mongodb* const db   = d_tests_mongodb_begin("keyed",
                                                         name,
                                                         sizeof(name));
    struct dj_db_rows        rows;
    struct dj_db_rows        again;
    char*                   id   = NULL;

    printf("find_keyed\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_set_document_format(
                               db,
                               D_MONGODB_JSON_RELAXED));
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(
                               db, NULL, "c",
                               "{\"n\": {\"$numberLong\": \"7\"}, "
                               "\"k\": 1}",
                               &id));
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(
                               db, NULL, "c",
                               "[{\"_id\": \"s\", \"k\": 2}, "
                               "{\"_id\": {\"a\": 1}, \"k\": 3}]",
                               0u, NULL));

    // every document, sorted, and the columns
    D_TESTS_MONGODB_OK(db, d_mongodb_find_keyed(db, NULL, "c", NULL,
                                                "{\"sort\": {\"k\": 1}}",
                                                &rows));
    D_TESTS_MONGODB_CHECK( (rows.row_count == 3u) &&
                           (rows.column_count == 2u) );
    D_TESTS_MONGODB_CHECK( (rows.columns) &&
                           (strcmp(rows.columns[0].name, "_id") == 0) &&
                           (rows.columns[0].type == D_DB_TYPE_JSON) &&
                           (strcmp(rows.columns[1].name, "document") == 0) &&
                           (rows.columns[1].type == D_DB_TYPE_JSON) );
    D_TESTS_MONGODB_CHECK( (id) &&
                           (d_tests_mongodb_contains(
                                d_tests_mongodb_cell(&rows, 0u, 0u),
                                id)) );
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_cell(&rows, 0u, 1u),
                              "{ \"$numberLong\" : \"7\" }"));
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_cell(&rows, 1u, 0u),
                                 "{ \"_id\" : \"s\" }") == 0);
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_cell(&rows, 2u, 0u),
                                 "{ \"_id\" : { \"a\" : "
                                 "{ \"$numberInt\" : \"1\" } } }") == 0);

    // each key finds its document alone, and the document read back is
    // written back as it was
    for (size_t r = 0u; r < rows.row_count; ++r)
    {
        int64_t counted = 0;

        D_TESTS_MONGODB_OK(db, d_mongodb_count_documents(
                                   db, NULL, "c",
                                   d_tests_mongodb_cell(&rows, r, 0u),
                                   NULL, &counted));
        D_TESTS_MONGODB_CHECK(counted == 1);
        D_TESTS_MONGODB_OK(db, d_mongodb_replace_one(
                                   db, NULL, "c",
                                   d_tests_mongodb_cell(&rows, r, 0u),
                                   d_tests_mongodb_cell(&rows, r, 1u),
                                   NULL, NULL));
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_find_keyed(db, NULL, "c", NULL,
                                                "{\"sort\": {\"k\": 1}}",
                                                &again));
    D_TESTS_MONGODB_CHECK(again.row_count == rows.row_count);

    for (size_t r = 0u; ( (r < rows.row_count) &&
                          (r < again.row_count) ); ++r)
    {
        D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_cell(&rows, r, 1u),
                                     d_tests_mongodb_cell(&again, r, 1u)) ==
                              0);
    }

    d_db_rows_free(&again);
    d_db_rows_free(&rows);

    // a filter, and a projection that leaves the _id out
    D_TESTS_MONGODB_OK(db, d_mongodb_find_keyed(db, NULL, "c",
                                                "{\"k\": {\"$gte\": 2}}",
                                                "{\"projection\": "
                                                "{\"_id\": 0}}",
                                                &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
    D_TESTS_MONGODB_CHECK( (rows.row_count == 2u) &&
                           (d_tests_mongodb_cell(&rows, 0u, 0u) == NULL) &&
                           (d_tests_mongodb_cell(&rows, 1u, 0u) == NULL) );
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_find_keyed(db, NULL, "none", NULL, NULL,
                                                &rows));
    D_TESTS_MONGODB_CHECK( (rows.row_count == 0u) &&
                           (rows.column_count == 2u) );
    d_db_rows_free(&rows);

    // a transaction's own writes
    if (d_mongodb_supports(db, D_MONGODB_FEATURE_TRANSACTIONS))
    {
        D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction(db));
        D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c",
                                                    "{\"_id\": 9, "
                                                    "\"k\": 9}",
                                                    NULL));
        D_TESTS_MONGODB_OK(db, d_mongodb_find_keyed(db, NULL, "c",
                                                    "{\"k\": 9}", NULL,
                                                    &rows));
        D_TESTS_MONGODB_CHECK(rows.row_count == 1u);
        d_db_rows_free(&rows);
        D_TESTS_MONGODB_OK(db, d_mongodb_rollback(db));
    }

    // refusals
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find_keyed(db, NULL, "c",
                                                "{\"$frob\": 1}", NULL,
                                                &rows),
                           D_MONGODB_ERROR_BAD_VALUE);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find_keyed(db, NULL, "c", NULL, NULL,
                                                NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find_keyed(NULL, NULL, "c", NULL, NULL,
                                                &rows),
                           D_DB_INVALID_ARGUMENT);
    d_db_free(id);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_update
  d_mongodb_update_one, _update_many and _replace_one.
  Tests the following:
  - each counts what it matched and changed, a match left as it was not
    changed, and each is the handle's latest write
  - an update of a pipeline
  - an upsert: its count and its _id
  - options: arrayFilters
  - an update without operators, and a replacement with them, are the
    driver's code; NULL and the wrong shapes are refused
*/
static
void d_tests_mongodb_update(void)
{
    char                          name[64];
    struct dj_mongodb* const       db     = d_tests_mongodb_begin("update",
                                                                 name,
                                                                 sizeof(name));
    struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };
    struct dj_db_rows              rows;

    printf("update\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(
                               db, NULL, "c",
                               "[{\"_id\": 1, \"n\": 1, \"a\": [1, 5]}, "
                               "{\"_id\": 2, \"n\": 1}, {\"_id\": 3, "
                               "\"n\": 2}]",
                               0u, NULL));

    // one, many, the counts
    D_TESTS_MONGODB_OK(db, d_mongodb_update_one(db, NULL, "c",
                                                "{\"n\": 1}",
                                                "{\"$inc\": {\"n\": 10}}",
                                                NULL, &counts));
    D_TESTS_MONGODB_CHECK( (counts.matched == 1) &&
                           (counts.modified == 1) &&
                           (counts.upserted == 0) );
    D_TESTS_MONGODB_OK(db, d_mongodb_update_many(db, NULL, "c", NULL,
                                                 "{\"$set\": {\"n\": 2}}",
                                                 NULL, &counts));
    D_TESTS_MONGODB_CHECK( (counts.matched == 3) &&
                           (counts.modified == 2) );
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 2);

    // a pipeline
    D_TESTS_MONGODB_OK(db, d_mongodb_update_many(db, NULL, "c", NULL,
                                                 "[{\"$set\": {\"m\": "
                                                 "{\"$add\": [\"$n\", "
                                                 "\"$_id\"]}}}]",
                                                 NULL, &counts));
    D_TESTS_MONGODB_CHECK(counts.modified == 3);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(db, "c", "{\"m\": 5}") == 1);

    // arrayFilters
    D_TESTS_MONGODB_OK(db, d_mongodb_update_one(db, NULL, "c",
                                                "{\"_id\": 1}",
                                                "{\"$set\": {\"a.$[e]\": 0}}",
                                                "{\"arrayFilters\": "
                                                "[{\"e\": {\"$gt\": 2}}]}",
                                                &counts));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(db, "c", "{\"a\": [1, 0]}") ==
                          1);

    // an upsert, and a replacement
    D_TESTS_MONGODB_OK(db, d_mongodb_replace_one(db, NULL, "c",
                                                 "{\"_id\": 9}",
                                                 "{\"r\": true}",
                                                 "{\"upsert\": true}",
                                                 &counts));
    D_TESTS_MONGODB_CHECK( (counts.upserted == 1) &&
                           (counts.matched == 0) );
    D_TESTS_MONGODB_OK(db, d_mongodb_replace_one(db, NULL, "c",
                                                 "{\"_id\": 9}",
                                                 "{\"r\": false}", NULL,
                                                 &counts));
    D_TESTS_MONGODB_CHECK( (counts.matched == 1) &&
                           (counts.modified == 1) );
    D_TESTS_MONGODB_OK(db, d_mongodb_find_one(db, NULL, "c",
                                              "{\"_id\": 9}", NULL, &rows));
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_row(&rows, 0u),
                                 "{ \"_id\" : { \"$numberInt\" : \"9\" }, "
                                 "\"r\" : false }") == 0);
    d_db_rows_free(&rows);

    // refusals
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(
                              d_mongodb_update_one(db, NULL, "c", NULL,
                                                   "{\"n\": 1}", NULL,
                                                   NULL)));
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(
                              d_mongodb_replace_one(db, NULL, "c", NULL,
                                                    "{\"$set\": {}}", NULL,
                                                    NULL)));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_replace_one(db, NULL, "c", NULL,
                                                 "[{\"$set\": {}}]", NULL,
                                                 NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_update_one(db, NULL, "c", NULL, NULL,
                                                NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_update_one(db, NULL, "c", "[]",
                                                "{\"$set\": {}}", NULL,
                                                NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_update_many(NULL, NULL, "c", NULL,
                                                 "{\"$set\": {}}", NULL,
                                                 NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_delete_count
  d_mongodb_delete_one, _delete_many, _count_documents,
_estimated_document_count and _distinct.
  Tests the following:
  - deletes count what they deleted
  - an exact count, with skip and limit; the estimate; 0 for a collection
    that is not there
  - distinct's values, one row each, column "value", typed JSON; a dotted
    field; a filter
  - inside a transaction, the estimate is D_DB_UNSUPPORTED and the
    transaction stays open; an exact count runs in it
  - NULL arguments
*/
static
void d_tests_mongodb_delete_count(void)
{
    char                    name[64];
    struct dj_mongodb* const db    = d_tests_mongodb_begin("delete",
                                                          name,
                                                          sizeof(name));
    int64_t                 count = -1;
    struct dj_db_rows        rows;

    printf("delete and count\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(
                               db, NULL, "c",
                               "[{\"g\": 1, \"s\": {\"t\": \"x\"}}, "
                               "{\"g\": 1, \"s\": {\"t\": \"y\"}}, "
                               "{\"g\": 2, \"s\": {\"t\": \"x\"}}, "
                               "{\"g\": 3}, {\"g\": 3}]",
                               0u, NULL));

    // counts
    D_TESTS_MONGODB_OK(db, d_mongodb_count_documents(db, NULL, "c",
                                                     "{\"g\": 1}", NULL,
                                                     &count));
    D_TESTS_MONGODB_CHECK(count == 2);
    D_TESTS_MONGODB_OK(db, d_mongodb_count_documents(db, NULL, "c", NULL,
                                                     "{\"skip\": 1, "
                                                     "\"limit\": 3}",
                                                     &count));
    D_TESTS_MONGODB_CHECK(count == 3);
    D_TESTS_MONGODB_OK(db, d_mongodb_estimated_document_count(db, NULL, "c",
                                                              NULL, &count));
    D_TESTS_MONGODB_CHECK(count == 5);
    D_TESTS_MONGODB_OK(db, d_mongodb_count_documents(db, NULL, "none", NULL,
                                                     NULL, &count));
    D_TESTS_MONGODB_CHECK(count == 0);
    D_TESTS_MONGODB_OK(db, d_mongodb_estimated_document_count(db, NULL,
                                                              "none", NULL,
                                                              &count));
    D_TESTS_MONGODB_CHECK(count == 0);

    // distinct
    D_TESTS_MONGODB_OK(db, d_mongodb_distinct(db, NULL, "c", "s.t", NULL,
                                              NULL, &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
    D_TESTS_MONGODB_CHECK(strcmp(rows.columns[0].name, "value") == 0);
    D_TESTS_MONGODB_CHECK(rows.columns[0].type == D_DB_TYPE_JSON);
    D_TESTS_MONGODB_CHECK(strcmp(rows.columns[0].type_name, "value") == 0);
    D_TESTS_MONGODB_CHECK( (d_tests_mongodb_rows_hold(&rows, "\"x\"")) &&
                           (d_tests_mongodb_rows_hold(&rows, "\"y\"")) );
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_distinct(db, NULL, "c", "g",
                                              "{\"g\": {\"$gt\": 1}}", NULL,
                                              &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_rows_hold(
                              &rows,
                              "{ \"$numberInt\" : \"2\" }"));
    d_db_rows_free(&rows);

    // in a transaction
    if (d_mongodb_supports(db, D_MONGODB_FEATURE_TRANSACTIONS))
    {
        D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction(db));
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_estimated_document_count(
                                   db, NULL, "c", NULL, &count),
                               D_DB_UNSUPPORTED);
        D_TESTS_MONGODB_CHECK(!d_mongodb_in_failed_transaction(db));
        D_TESTS_MONGODB_OK(db, d_mongodb_count_documents(db, NULL, "c",
                                                         NULL, NULL,
                                                         &count));
        D_TESTS_MONGODB_CHECK(count == 5);
        D_TESTS_MONGODB_OK(db, d_mongodb_commit(db));
    }

    // deletes
    D_TESTS_MONGODB_OK(db, d_mongodb_delete_one(db, NULL, "c", "{\"g\": 3}",
                                                NULL, &count));
    D_TESTS_MONGODB_CHECK(count == 1);
    D_TESTS_MONGODB_OK(db, d_mongodb_delete_many(db, NULL, "c",
                                                 "{\"g\": {\"$lt\": 3}}",
                                                 NULL, &count));
    D_TESTS_MONGODB_CHECK(count == 3);
    D_TESTS_MONGODB_OK(db, d_mongodb_delete_one(db, NULL, "c", "{\"g\": 9}",
                                                NULL, &count));
    D_TESTS_MONGODB_CHECK(count == 0);

    // NULL
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_distinct(db, NULL, "c", NULL, NULL, NULL,
                                              &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_distinct(db, NULL, "c", "g", NULL, NULL,
                                              NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_count_documents(NULL, NULL, "c", NULL,
                                                     NULL, &count),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_delete_one(db, NULL, NULL, NULL, NULL,
                                                NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_aggregate
  d_mongodb_aggregate.
  Tests the following:
  - a collection's pipeline, its options: batchSize over many batches
  - a database's pipeline, $documents, from 5.1
  - a pipeline that ends in $out writes and returns none
  - a document where the pipeline is due, and NULL, are refused
*/
static
void d_tests_mongodb_aggregate(void)
{
    char                    name[64];
    struct dj_mongodb* const db   = d_tests_mongodb_begin("aggregate",
                                                         name,
                                                         sizeof(name));
    struct dj_db_rows        rows;

    printf("aggregate\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(
                               db, NULL, "c",
                               "[{\"g\": 1, \"v\": 2}, {\"g\": 1, \"v\": 3}, "
                               "{\"g\": 2, \"v\": 4}]",
                               0u, NULL));

    // the collection's
    D_TESTS_MONGODB_OK(db, d_mongodb_aggregate(
                               db, NULL, "c",
                               "[{\"$group\": {\"_id\": \"$g\", \"s\": "
                               "{\"$sum\": \"$v\"}}}, {\"$sort\": "
                               "{\"_id\": 1}}]",
                               "{\"batchSize\": 1}", &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_row(&rows, 0u),
                                 "{ \"_id\" : { \"$numberInt\" : \"1\" }, "
                                 "\"s\" : { \"$numberInt\" : \"5\" } }") ==
                          0);
    d_db_rows_free(&rows);

    // the database's
    if (d_mongodb_server_at_least(db, 5u, 1u, 0u))
    {
        D_TESTS_MONGODB_OK(db, d_mongodb_aggregate(
                                   db, NULL, NULL,
                                   "[{\"$documents\": [{\"x\": 1}, "
                                   "{\"x\": 2}]}]",
                                   NULL, &rows));
        D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
        d_db_rows_free(&rows);
    }

    // $out
    D_TESTS_MONGODB_OK(db, d_mongodb_aggregate(db, NULL, "c",
                                               "[{\"$match\": {\"g\": 1}}, "
                                               "{\"$out\": \"o\"}]",
                                               NULL, &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 0u);
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(db, "o", NULL) == 2);

    // refusals
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_aggregate(db, NULL, "c", NULL, NULL,
                                               &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_aggregate(db, NULL, "c",
                                               "[{\"$frob\": {}}]", NULL,
                                               &rows),
                           40324);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_aggregate(db, NULL, "c", "[]", NULL,
                                               NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_find_and_modify
  d_mongodb_find_one_and_update, _and_replace and _and_delete.
  Tests the following:
  - the document as it was, and with returnDocument "after" as it is; the
    options' sort chooses the first; projection
  - an update of a pipeline
  - an upsert that asks for the document as it was: D_DB_NOT_FOUND, the
    document made, its _id the handle's latest
  - none matched: D_DB_NOT_FOUND
  - an update without operators, a replacement with them, a delete asked
    for its document after, or to upsert, an option of the wrong type:
    D_DB_INVALID_ARGUMENT; an option of no such name, the server's code
  - each counts as the handle's latest write
*/
static
void d_tests_mongodb_find_and_modify(void)
{
    char                    name[64];
    struct dj_mongodb* const db   = d_tests_mongodb_begin("fam",
                                                         name,
                                                         sizeof(name));
    struct dj_db_rows        rows;
    char*                   id   = NULL;

    printf("find and modify\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(
                               db, NULL, "c",
                               "[{\"_id\": 1, \"n\": 1}, {\"_id\": 2, "
                               "\"n\": 1}]",
                               0u, NULL));

    // before and after, the first in the sort
    D_TESTS_MONGODB_OK(db, d_mongodb_find_one_and_update(
                               db, NULL, "c", "{\"n\": 1}",
                               "{\"$inc\": {\"n\": 1}}",
                               "{\"sort\": {\"_id\": -1}}", &rows));
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_row(&rows, 0u),
                                 "{ \"_id\" : { \"$numberInt\" : \"2\" }, "
                                 "\"n\" : { \"$numberInt\" : \"1\" } }") ==
                          0);
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 1);
    D_TESTS_MONGODB_OK(db, d_mongodb_find_one_and_update(
                               db, NULL, "c", "{\"_id\": 1}",
                               "[{\"$set\": {\"n\": {\"$add\": [\"$n\", "
                               "10]}}}]",
                               "{\"returnDocument\": \"after\", "
                               "\"projection\": {\"_id\": 0}}", &rows));
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_row(&rows, 0u),
                                 "{ \"n\" : { \"$numberInt\" : \"11\" } }") ==
                          0);
    d_db_rows_free(&rows);

    // replace, and an upsert asked for as it was
    D_TESTS_MONGODB_OK(db, d_mongodb_find_one_and_replace(
                               db, NULL, "c", "{\"_id\": 2}", "{\"r\": 1}",
                               "{\"returnDocument\": \"after\"}", &rows));
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_row(&rows, 0u),
                                 "{ \"_id\" : { \"$numberInt\" : \"2\" }, "
                                 "\"r\" : { \"$numberInt\" : \"1\" } }") ==
                          0);
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find_one_and_update(
                               db, NULL, "c", "{\"_id\": 3}",
                               "{\"$set\": {\"u\": 1}}",
                               "{\"upsert\": true}", &rows),
                           D_DB_NOT_FOUND);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(db, "c", "{\"_id\": 3}") ==
                          1);
    D_TESTS_MONGODB_OK(db, d_mongodb_last_insert_id(db, &id));
    D_TESTS_MONGODB_CHECK( (id) &&
                           (strcmp(id, "{ \"$numberInt\" : \"3\" }") == 0) );
    d_db_free(id);

    // delete
    D_TESTS_MONGODB_OK(db, d_mongodb_find_one_and_delete(
                               db, NULL, "c", "{\"_id\": 3}", NULL, &rows));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u), "\"u\""));
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 1);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find_one_and_delete(
                               db, NULL, "c", "{\"_id\": 3}", NULL, &rows),
                           D_DB_NOT_FOUND);

    // refusals
    {
        static const char* const OPTIONS[] =
        {
            "{\"returnDocument\": \"later\"}", "{\"upsert\": 1}",
            "{\"sort\": 5}", "{\"maxTimeMS\": -1}"
        };

        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_find_one_and_update(
                                   db, NULL, "c", NULL, "{\"n\": 1}", NULL,
                                   &rows),
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_find_one_and_replace(
                                   db, NULL, "c", NULL, "{\"$set\": {}}",
                                   NULL, &rows),
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_find_one_and_delete(
                                   db, NULL, "c", NULL,
                                   "{\"returnDocument\": \"after\"}", &rows),
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_find_one_and_delete(
                                   db, NULL, "c", NULL,
                                   "{\"upsert\": true}", &rows),
                               D_DB_INVALID_ARGUMENT);

        for (size_t i = 0u; i < (sizeof(OPTIONS) / sizeof(OPTIONS[0])); ++i)
        {
            D_TESTS_MONGODB_RESULT(db,
                                   d_mongodb_find_one_and_update(
                                       db, NULL, "c", NULL,
                                       "{\"$set\": {}}", OPTIONS[i], &rows),
                                   D_DB_INVALID_ARGUMENT);
        }

        D_TESTS_MONGODB_CHECK(d_mongodb_find_one_and_update(
                                  db, NULL, "c", NULL,
                                  "{\"$set\": {\"q\": 1}}",
                                  "{\"dtmBogus\": 1}", &rows) > 0);
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_find_one_and_update(
                                   db, NULL, "c", NULL,
                                   "{\"$set\": {}}", NULL, NULL),
                               D_DB_INVALID_ARGUMENT);
    }

    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_bulk
  d_mongodb_bulk_write and _client_bulk_write.
  Tests the following:
  - every model, in one bulk write, and the counts it reports
  - ordered, the writes stop at the first failure, which is its code, the
    counts of those before; unordered, they go on; the reply holds each
  - options: bypassDocumentValidation past a validator; comment and let,
    where the driver takes them (1.22) and the server (4.4, 5.0), the
    driver's refusal where it does not, else the server's
  - a model of no such name, without its parts, an empty array, ordered
    among the options, unknown flags: D_DB_INVALID_ARGUMENT; a field the
    driver does not take, its code; a refusal's counts, a NULL handle's
    too, zeros
  - the client's bulk write, from 8.0: models across collections and
    databases, its counts, and its failures by the place of the write;
    before, D_DB_UNSUPPORTED
*/
static
void d_tests_mongodb_bulk(void)
{
    char                          name[64];
    char                          writes[1024];
    struct dj_mongodb* const       db     = d_tests_mongodb_begin("bulk",
                                                                 name,
                                                                 sizeof(name));
    struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };
    char*                         reply  = NULL;

    printf("bulk\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    // every model
    D_TESTS_MONGODB_OK(db, d_mongodb_bulk_write(
                               db, NULL, "c",
                               "[{\"insertOne\": {\"document\": {\"_id\": 1}}},"
                               " {\"insertOne\": {\"document\": {\"_id\": 2}}},"
                               " {\"updateOne\": {\"filter\": {\"_id\": 1}, "
                               "\"update\": {\"$set\": {\"a\": 1}}}},"
                               " {\"updateMany\": {\"filter\": {}, \"update\": "
                               "[{\"$set\": {\"b\": 1}}]}},"
                               " {\"replaceOne\": {\"filter\": {\"_id\": 3}, "
                               "\"replacement\": {\"r\": 1}, \"upsert\": "
                               "true}},"
                               " {\"deleteOne\": {\"filter\": {\"_id\": 2}}},"
                               " {\"deleteMany\": {\"filter\": {\"_id\": "
                               "{\"$gt\": 5}}}}]",
                               0u, NULL, &counts));
    D_TESTS_MONGODB_CHECK( (counts.inserted == 2) &&
                           (counts.matched == 3) &&
                           (counts.modified == 3) &&
                           (counts.upserted == 1) &&
                           (counts.deleted == 1) );

    // ordered and unordered failures
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_bulk_write(
                               db, NULL, "c",
                               "[{\"insertOne\": {\"document\": {\"_id\": "
                               "4}}}, {\"insertOne\": {\"document\": "
                               "{\"_id\": 1}}}, {\"insertOne\": "
                               "{\"document\": {\"_id\": 5}}}]",
                               0u, NULL, &counts),
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_MONGODB_CHECK(counts.inserted == 1);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_bulk_write(
                               db, NULL, "c",
                               "[{\"insertOne\": {\"document\": {\"_id\": "
                               "1}}}, {\"insertOne\": {\"document\": "
                               "{\"_id\": 4}}}, {\"insertOne\": "
                               "{\"document\": {\"_id\": 6}}}]",
                               D_MONGODB_UNORDERED, NULL, &counts),
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_MONGODB_CHECK(counts.inserted == 1);
    D_TESTS_MONGODB_OK(db, d_mongodb_error_reply(db, &reply));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(reply, "writeErrors"));
    d_db_free(reply);

    // bypassing a validator
    D_TESTS_MONGODB_OK(db, d_mongodb_create_collection(
                               db, NULL, "v",
                               "{\"validator\": {\"n\": {\"$type\": "
                               "\"int\"}}}", 0u));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_bulk_write(
                               db, NULL, "v",
                               "[{\"insertOne\": {\"document\": {\"n\": "
                               "\"s\"}}}]",
                               0u, NULL, NULL),
                           121);
    D_TESTS_MONGODB_OK(db, d_mongodb_bulk_write(
                               db, NULL, "v",
                               "[{\"insertOne\": {\"document\": {\"n\": "
                               "\"s\"}}}]",
                               0u, "{\"bypassDocumentValidation\": true}",
                               NULL));

    // comment and let, where the driver takes them, and the server
    {
        const bool          driver  = d_tests_mongodb_driver_at_least(1u,
                                                                      22u);
        const unsigned long server  = d_mongodb_server_version_number(db);
        const int           comment = d_mongodb_bulk_write(
                                          db, NULL, "c",
                                          "[{\"deleteMany\": {\"filter\": "
                                          "{\"_id\": -1}}}]",
                                          0u, "{\"comment\": \"dtm\"}",
                                          NULL);
        const int           let     = d_mongodb_bulk_write(
                                          db, NULL, "c",
                                          "[{\"deleteMany\": {\"filter\": "
                                          "{\"$expr\": {\"$eq\": [\"$_id\", "
                                          "\"$$x\"]}}}}]",
                                          0u, "{\"let\": {\"x\": -1}}",
                                          NULL);

        D_TESTS_MONGODB_CHECK( (!driver)
                                   ? d_mongodb_result_is_client(comment)
                                   : (server >= 40400ul)
                                         ? (comment == 0)
                                         : ( (comment > 0) &&
                                             (!d_mongodb_result_is_client(
                                                  comment)) ) );
        D_TESTS_MONGODB_CHECK( (!driver)
                                   ? d_mongodb_result_is_client(let)
                                   : (server >= 50000ul)
                                         ? (let == 0)
                                         : ( (let > 0) &&
                                             (!d_mongodb_result_is_client(
                                                  let)) ) );
    }

    // refusals
    {
        static const char* const NOT_WRITES[] =
        {
            "[{\"frob\": {}}]", "[{\"insertOne\": {}}]",
            "[{\"updateOne\": {\"update\": {\"$set\": {}}}}]",
            "[{\"replaceOne\": {\"filter\": {}}}]", "[]", "[5]",
            ("[{\"insertOne\": {\"document\": {}}, \"deleteOne\": "
             "{\"filter\": {}}}]")
        };

        for (size_t i = 0u; i < (sizeof(NOT_WRITES) / sizeof(NOT_WRITES[0]));
             ++i)
        {
            D_TESTS_MONGODB_RESULT(db,
                                   d_mongodb_bulk_write(db, NULL, "c",
                                                        NOT_WRITES[i], 0u,
                                                        NULL, NULL),
                                   D_DB_INVALID_ARGUMENT);
        }

        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_bulk_write(
                                   db, NULL, "c",
                                   "[{\"deleteOne\": {\"filter\": {}}}]", 0u,
                                   "{\"ordered\": false}", NULL),
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_bulk_write(
                                   db, NULL, "c",
                                   "[{\"deleteOne\": {\"filter\": {}}}]",
                                   0x8000u, NULL, NULL),
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(
                                  d_mongodb_bulk_write(
                                      db, NULL, "c",
                                      "[{\"deleteOne\": {\"filter\": {}, "
                                      "\"dtmBogus\": 1}}]",
                                      0u, NULL, NULL)));

        // a refusal's counts, and a NULL handle's: none
        struct dj_mongodb_write_counts stale = { 9, 9, 9, 9, 9 };

        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_bulk_write(db, NULL, "c", "[]", 0u,
                                                    NULL, &stale),
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_MONGODB_CHECK(d_tests_mongodb_no_counts(&stale));
        stale.inserted = 9;
        D_TESTS_MONGODB_CHECK(d_mongodb_bulk_write(NULL, NULL, "c", "[]", 0u,
                                                   NULL, &stale) ==
                              D_DB_INVALID_ARGUMENT);
        D_TESTS_MONGODB_CHECK(d_tests_mongodb_no_counts(&stale));
        stale.deleted = 9;
        D_TESTS_MONGODB_CHECK(d_mongodb_client_bulk_write(NULL, "[]", 0u,
                                                          NULL, &stale) ==
                              D_DB_INVALID_ARGUMENT);
        D_TESTS_MONGODB_CHECK(d_tests_mongodb_no_counts(&stale));
    }

    // the client's bulk write
    if (!d_mongodb_supports(db, D_MONGODB_FEATURE_BULK_WRITE))
    {
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_client_bulk_write(
                                   db, "[{\"insertOne\": {\"namespace\": "
                                   "\"a.b\", \"document\": {}}}]",
                                   0u, NULL, NULL),
                               D_DB_UNSUPPORTED);
        d_tests_mongodb_end(db, name);

        return;
    }

    snprintf(writes, sizeof(writes),
             "[{\"insertOne\": {\"namespace\": \"%s.x\", \"document\": "
             "{\"_id\": 1}}},"
             " {\"insertOne\": {\"namespace\": \"%s.y\", \"document\": "
             "{\"_id\": 1}}},"
             " {\"updateOne\": {\"namespace\": \"%s.x\", \"filter\": "
             "{\"_id\": 1}, \"update\": {\"$set\": {\"a\": 1}}}},"
             " {\"updateMany\": {\"namespace\": \"%s.x\", \"filter\": "
             "{\"_id\": 2}, \"update\": {\"$set\": {\"a\": 1}}, \"upsert\": "
             "true}},"
             " {\"replaceOne\": {\"namespace\": \"%s.y\", \"filter\": "
             "{\"_id\": 1}, \"replacement\": {\"r\": 1}}},"
             " {\"deleteMany\": {\"namespace\": \"%s.x\", \"filter\": "
             "{\"_id\": 9}}}]",
             name, name, name, name, name, name);
    D_TESTS_MONGODB_OK(db, d_mongodb_client_bulk_write(
                               db, writes, 0u,
                               "{\"writeConcern\": {\"w\": \"majority\"}, "
                               "\"comment\": \"dtm\"}",
                               &counts));
    D_TESTS_MONGODB_CHECK( (counts.inserted == 2) &&
                           (counts.matched == 2) &&
                           (counts.modified == 2) &&
                           (counts.upserted == 1) &&
                           (counts.deleted == 0) );
    D_TESTS_MONGODB_CHECK(d_mongodb_changes(db) == 5);

    // its failures, by place
    snprintf(writes, sizeof(writes),
             "[{\"insertOne\": {\"namespace\": \"%s.x\", \"document\": "
             "{\"_id\": 7}}},"
             " {\"insertOne\": {\"namespace\": \"%s.x\", \"document\": "
             "{\"_id\": 1}}},"
             " {\"insertOne\": {\"namespace\": \"%s.y\", \"document\": "
             "{\"_id\": 1}}}]",
             name, name, name);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_client_bulk_write(db, writes,
                                                       D_MONGODB_UNORDERED,
                                                       NULL, &counts),
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_MONGODB_CHECK(counts.inserted == 1);
    D_TESTS_MONGODB_OK(db, d_mongodb_error_reply(db, &reply));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              reply,
                              "\"index\" : { \"$numberLong\" : \"1\" }"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(reply, "\"2\" : {"));
    d_db_free(reply);

    // its refusals
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_client_bulk_write(
                               db, "[{\"insertOne\": {\"document\": {}}}]",
                               0u, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_client_bulk_write(
                               db, "[{\"deleteOne\": {\"namespace\": \"a.b\", "
                               "\"filter\": {}, \"upsert\": true}}]",
                               0u, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_client_bulk_write(
                               db, "[{\"deleteOne\": {\"namespace\": \"a.b\", "
                               "\"filter\": {}}}]",
                               0u, "{\"frob\": 1}", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_client_bulk_write(db, "[]", 0u, NULL,
                                                       NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}


//==============================================================================
// 5.  CURSORS AND CHANGE STREAMS
//==============================================================================

/*
d_tests_mongodb_cursors
  The cursors of mongodb_cursor.h.
  Tests the following:
  - a find's cursor read a page at a time, past the server's batches, in
    order, to a short page; then done, and empty as often as it is read
  - the handle runs other operations while a cursor is open
  - an aggregation's cursor, and a command's, listCollections'; a command
    whose reply holds no cursor is refused
  - a cursor opened in a transaction ends with it, committed or rolled
    back: a read of it then is D_MONGODB_ERROR_TRANSACTION_CURSOR, as
    often as it is read, in another transaction or none, which it leaves
    open; closed in the next transaction, it leaves that one sound; one
    read to its end before reads as done; one opened outside reads inside
  - NULL arguments and a page of 0 are refused; is_done of NULL is true;
    close of NULL is 0
*/
static
void d_tests_mongodb_cursors(void)
{
    char                     name[64];
    struct dj_mongodb* const  db     = d_tests_mongodb_begin("cursors",
                                                            name,
                                                            sizeof(name));
    struct dj_mongodb_cursor* cursor = NULL;
    struct dj_db_rows         rows;
    char                     many[8192];
    int                      used   = 0;
    size_t                   seen   = 0u;

    printf("cursors\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    used = snprintf(many, sizeof(many), "[");

    for (int i = 0; i < 25; ++i)
    {
        used += snprintf(many + used, sizeof(many) - (size_t)used,
                         "%s{\"_id\": %d}", (i) ? ", " : "", i);
    }

    snprintf(many + used, sizeof(many) - (size_t)used, "]");
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(db, NULL, "c", many, 0u,
                                                 NULL));

    // a page at a time
    D_TESTS_MONGODB_OK(db, d_mongodb_find_cursor(db, NULL, "c", NULL,
                                                 "{\"sort\": {\"_id\": 1}, "
                                                 "\"batchSize\": 4}",
                                                 &cursor));
    D_TESTS_MONGODB_CHECK(!d_mongodb_cursor_is_done(cursor));

    while (!d_mongodb_cursor_is_done(cursor))
    {
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_fetch(cursor, 10u, &rows));
        D_TESTS_MONGODB_CHECK(rows.row_count <= 10u);

        // the first of each page, in order
        if (rows.row_count > 0u)
        {
            char first[48];

            snprintf(first, sizeof(first), "\"%lu\"", (unsigned long)seen);
            D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                                      d_tests_mongodb_row(&rows, 0u),
                                      first));
        }

        seen += rows.row_count;
        d_db_rows_free(&rows);

        // the handle does other work meanwhile
        D_TESTS_MONGODB_OK(db, d_mongodb_ping(db));
    }

    D_TESTS_MONGODB_CHECK(seen == 25u);
    D_TESTS_MONGODB_OK(db, d_mongodb_cursor_fetch(cursor, 5u, &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 0u);
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_cursor_fetch(cursor, 0u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_cursor_fetch(cursor, 1u, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_OK(db, d_mongodb_cursor_close(cursor));

    // an aggregation's, and a command's
    D_TESTS_MONGODB_OK(db, d_mongodb_aggregate_cursor(
                               db, NULL, "c",
                               "[{\"$match\": {\"_id\": {\"$lt\": 3}}}]",
                               NULL, &cursor));
    D_TESTS_MONGODB_OK(db, d_mongodb_cursor_fetch(cursor, 100u, &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 3u);
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_cursor_close(cursor));
    D_TESTS_MONGODB_OK(db, d_mongodb_command_cursor(
                               db, NULL, "{\"listCollections\": 1}",
                               &cursor));
    D_TESTS_MONGODB_OK(db, d_mongodb_cursor_fetch(cursor, 100u, &rows));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_rows_hold(&rows, "\"c\""));
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_cursor_close(cursor));
    cursor = (struct dj_mongodb_cursor*)&cursor;
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_command_cursor(db, NULL,
                                                    "{\"ping\": 1}",
                                                    &cursor),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(cursor == NULL);

    // in a transaction, and after it
    if (d_mongodb_supports(db, D_MONGODB_FEATURE_TRANSACTIONS))
    {
        struct dj_mongodb_cursor* other = NULL;
        struct dj_mongodb_cursor* read  = NULL;
        struct dj_mongodb_cursor* out   = NULL;

        D_TESTS_MONGODB_OK(db, d_mongodb_find_cursor(db, NULL, "c", NULL,
                                                     "{\"batchSize\": 2}",
                                                     &out));
        D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction(db));
        D_TESTS_MONGODB_OK(db, d_mongodb_find_cursor(db, NULL, "c", NULL,
                                                     "{\"batchSize\": 2}",
                                                     &cursor));
        D_TESTS_MONGODB_OK(db, d_mongodb_find_cursor(db, NULL, "c", NULL,
                                                     "{\"batchSize\": 2}",
                                                     &other));
        D_TESTS_MONGODB_OK(db, d_mongodb_find_cursor(
                                   db, NULL, "c",
                                   "{\"_id\": {\"$lt\": 2}}", NULL, &read));
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_fetch(cursor, 2u, &rows));
        d_db_rows_free(&rows);
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_fetch(read, 10u, &rows));
        D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
        d_db_rows_free(&rows);
        D_TESTS_MONGODB_CHECK(d_mongodb_cursor_is_done(read));
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_fetch(out, 2u, &rows));
        D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
        d_db_rows_free(&rows);
        D_TESTS_MONGODB_OK(db, d_mongodb_commit(db));

        // ended, outside any transaction
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_cursor_fetch(cursor, 10u, &rows),
                               D_MONGODB_ERROR_TRANSACTION_CURSOR);
        D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_name(db),
                                     "Location50740") == 0);
        D_TESTS_MONGODB_CHECK(d_mongodb_cursor_is_done(cursor));
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_fetch(read, 10u, &rows));
        D_TESTS_MONGODB_CHECK(rows.row_count == 0u);
        d_db_rows_free(&rows);

        // and inside the next, which it leaves sound
        D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction(db));
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_cursor_fetch(cursor, 10u, &rows),
                               D_MONGODB_ERROR_TRANSACTION_CURSOR);
        D_TESTS_MONGODB_CHECK(!d_mongodb_in_failed_transaction(db));
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_close(other));
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_fetch(out, 2u, &rows));
        D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
        d_db_rows_free(&rows);
        D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c",
                                                    "{\"_id\": 100}", NULL));
        D_TESTS_MONGODB_OK(db, d_mongodb_commit(db));
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_close(cursor));
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_close(read));
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_close(out));

        // a rollback ends them too
        D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction(db));
        D_TESTS_MONGODB_OK(db, d_mongodb_find_cursor(db, NULL, "c", NULL,
                                                     "{\"batchSize\": 2}",
                                                     &cursor));
        D_TESTS_MONGODB_OK(db, d_mongodb_rollback(db));
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_cursor_fetch(cursor, 1u, &rows),
                               D_MONGODB_ERROR_TRANSACTION_CURSOR);
        D_TESTS_MONGODB_OK(db, d_mongodb_cursor_close(cursor));
        D_TESTS_MONGODB_OK(db, d_mongodb_exec(db, NULL,
                                              "{\"delete\": \"c\", "
                                              "\"deletes\": [{\"q\": "
                                              "{\"_id\": 100}, \"limit\": "
                                              "1}]}"));
    }

    // NULL
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find_cursor(db, NULL, "c", NULL, NULL,
                                                 NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_cursor_fetch(NULL, 1u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(d_mongodb_cursor_is_done(NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_cursor_close(NULL));
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_wait_events
  Reads a stream until it has handed back `_want` events or several waits
have gone by, into one count; the event read last is kept in `_last`.
*/
static
size_t d_tests_mongodb_wait_events(
    struct dj_mongodb*               _db,
    struct dj_mongodb_change_stream* _stream,
    size_t                           _want,
    char*                            _last,
    size_t                           _size
)
{
    size_t           got = 0u;
    struct dj_db_rows rows;

    // a few waits, each of the options' maxAwaitTimeMS
    for (int wait = 0; (wait < 20) && (got < _want); ++wait)
    {
        D_TESTS_MONGODB_OK(_db, d_mongodb_change_stream_fetch(_stream,
                                                              10u,
                                                              &rows));

        if (rows.row_count > 0u)
        {
            snprintf(_last, _size, "%s",
                     d_tests_mongodb_row(&rows, rows.row_count - 1u));
        }

        got += rows.row_count;
        d_db_rows_free(&rows);
    }

    return got;
}

/*
d_tests_mongodb_change_streams
  The change streams of mongodb_cursor.h.
  Tests the following:
  - a collection's stream sees an insert, an update and a delete made after
    it opened, as events; none when nothing happens in the wait
  - its pipeline filters the events; fullDocument updateLookup
  - a resume token after an event, and a stream opened after it with
    resumeAfter sees what came later and not what came before
  - a database's and a deployment's stream see a collection's events
  - a stream over nothing a server refuses fails to open, the stream NULL
  - NULL arguments; close of NULL is 0
*/
static
void d_tests_mongodb_change_streams(void)
{
    char                            name[64];
    char                            last[1024]   = "";
    char                            resume[1024] = "";
    char                            options[1280];
    struct dj_mongodb* const         db           = d_tests_mongodb_begin(
                                                       "streams",
                                                       name,
                                                       sizeof(name));
    struct dj_mongodb_change_stream* stream       = NULL;
    char*                           token        = NULL;
    struct dj_db_rows                rows;

    printf("change streams\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    if (!d_mongodb_supports(db, D_MONGODB_FEATURE_CHANGE_STREAMS))
    {
        d_tests_mongodb_skip("change streams", "the deployment has none");
        d_tests_mongodb_end(db, name);

        return;
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_create_collection(db, NULL, "c", NULL,
                                                       0u));

    // a collection's stream
    D_TESTS_MONGODB_OK(db, d_mongodb_watch_collection(
                               db, NULL, "c",
                               "[{\"$match\": {\"operationType\": {\"$in\": "
                               "[\"insert\", \"update\", \"delete\"]}}}]",
                               "{\"maxAwaitTimeMS\": 200, \"fullDocument\": "
                               "\"updateLookup\"}",
                               &stream));
    {
        // a token from the opening batch's postBatchResumeToken, or none
        const int result = d_mongodb_change_stream_resume_token(stream,
                                                                &token);

        D_TESTS_MONGODB_CHECK( (result == 0) ||
                               (result == D_DB_NOT_FOUND) );
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_change_stream_fetch(stream, 5u, &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 0u);
    d_db_rows_free(&rows);
    d_db_free(token);
    token = NULL;
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c",
                                                "{\"_id\": 1, \"n\": 1}",
                                                NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_update_one(db, NULL, "c",
                                                "{\"_id\": 1}",
                                                "{\"$set\": {\"n\": 2}}",
                                                NULL, NULL));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_wait_events(db, stream, 2u, last,
                                                      sizeof(last)) == 2u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(last, "\"update\""));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(last, "\"fullDocument\""));
    D_TESTS_MONGODB_OK(db, d_mongodb_change_stream_resume_token(stream,
                                                                &token));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(token, "\"_data\""));
    snprintf(resume, sizeof(resume), "%s", (token) ? token : "{}");
    d_db_free(token);
    D_TESTS_MONGODB_OK(db, d_mongodb_change_stream_close(stream));

    // resumed after the update: the delete, and not the insert
    D_TESTS_MONGODB_OK(db, d_mongodb_delete_one(db, NULL, "c",
                                                "{\"_id\": 1}", NULL, NULL));
    snprintf(options, sizeof(options),
             "{\"maxAwaitTimeMS\": 200, \"resumeAfter\": %s}", resume);
    D_TESTS_MONGODB_OK(db, d_mongodb_watch_collection(db, NULL, "c", NULL,
                                                      options, &stream));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_wait_events(db, stream, 1u, last,
                                                      sizeof(last)) == 1u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(last, "\"delete\""));
    D_TESTS_MONGODB_OK(db, d_mongodb_change_stream_close(stream));

    // a database's and the deployment's
    D_TESTS_MONGODB_OK(db, d_mongodb_watch_database(
                               db, NULL, NULL, "{\"maxAwaitTimeMS\": 200}",
                               &stream));
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "d", "{}", NULL));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_wait_events(db, stream, 1u, last,
                                                      sizeof(last)) >= 1u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(last, "\"d\""));
    D_TESTS_MONGODB_OK(db, d_mongodb_change_stream_close(stream));
    snprintf(options, sizeof(options),
             "[{\"$match\": {\"ns.db\": \"%s\"}}]", name);
    D_TESTS_MONGODB_OK(db, d_mongodb_watch_deployment(
                               db, options, "{\"maxAwaitTimeMS\": 200}",
                               &stream));
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "e", "{}", NULL));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_wait_events(db, stream, 1u, last,
                                                      sizeof(last)) >= 1u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(last, "\"e\""));
    D_TESTS_MONGODB_OK(db, d_mongodb_change_stream_close(stream));

    // refused at once, and NULL
    stream = (struct dj_mongodb_change_stream*)&stream;
    D_TESTS_MONGODB_CHECK(d_mongodb_watch_collection(
                              db, NULL, "c", "[{\"$frob\": {}}]", NULL,
                              &stream) > 0);
    D_TESTS_MONGODB_CHECK(stream == NULL);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_watch_collection(db, NULL, "c", NULL,
                                                      NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_watch_collection(db, NULL, NULL, NULL,
                                                      NULL, &stream),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_change_stream_fetch(NULL, 1u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_change_stream_resume_token(NULL, &token),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_OK(db, d_mongodb_change_stream_close(NULL));
    d_tests_mongodb_end(db, name);

    return;
}


//==============================================================================
// 6.  DATABASES AND SCHEMA
//==============================================================================

/*
d_tests_mongodb_databases
  mongodb_database.h.
  Tests the following:
  - a database exists from its first collection: has and the list say so,
    by its exact name
  - a drop of one that is not there is D_DB_NOT_FOUND, unless
    D_DB_IF_EXISTS; a drop of one that is takes it from the list
  - inside a transaction a drop is D_DB_UNSUPPORTED, the transaction open
  - use_database: a name of any database, there or not, current after;
    NULL leaves none; the names MongoDB forbids are 73, the current kept
  - current_database: a copy; none is D_DB_NOT_FOUND
  - NULL arguments
*/
static
void d_tests_mongodb_databases(void)
{
    char                    name[64];
    char                    other[96];
    struct dj_mongodb* const db      = d_tests_mongodb_begin("databases",
                                                            name,
                                                            sizeof(name));
    bool                    exists  = true;
    char*                   current = NULL;
    struct dj_db_name_list   names;

    printf("databases\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    snprintf(other, sizeof(other), "%s_other", name);

    // none until a collection
    D_TESTS_MONGODB_OK(db, d_mongodb_has_database_s(db, name, &exists));
    D_TESTS_MONGODB_CHECK(!exists);
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c", "{}", NULL));
    D_TESTS_MONGODB_CHECK(d_mongodb_has_database(db, name));
    D_TESTS_MONGODB_OK(db, d_mongodb_list_databases(db, &names));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_has_name(&names, name));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_has_name(&names, "admin"));
    D_TESTS_MONGODB_CHECK(names.names[names.count] == NULL);
    d_db_name_list_free(&names);

    // drops
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_drop_database(db, other, 0u),
                           D_DB_NOT_FOUND);
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_database(db, other,
                                                   D_DB_IF_EXISTS));
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, other, "c", "{}", NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_database(db, other, 0u));
    D_TESTS_MONGODB_CHECK(!d_mongodb_has_database(db, other));

    if (d_mongodb_supports(db, D_MONGODB_FEATURE_TRANSACTIONS))
    {
        D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction(db));
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_drop_database(db, name, 0u),
                               D_DB_UNSUPPORTED);
        D_TESTS_MONGODB_CHECK(d_mongodb_in_transaction(db));
        D_TESTS_MONGODB_CHECK(!d_mongodb_in_failed_transaction(db));
        D_TESTS_MONGODB_OK(db, d_mongodb_rollback(db));
    }

    // the current database
    D_TESTS_MONGODB_OK(db, d_mongodb_use_database(db, other));
    D_TESTS_MONGODB_OK(db, d_mongodb_current_database(db, &current));
    D_TESTS_MONGODB_CHECK( (current) &&
                           (strcmp(current, other) == 0) );
    d_db_free(current);

    {
        static const char* const FORBIDDEN[] =
        {
            "", "a/b", "a\\b", "a.b", "a b", "a\"b", "a$b",
            "a234567890123456789012345678901234567890123456789012345678901234"
        };

        for (size_t i = 0u; i < (sizeof(FORBIDDEN) / sizeof(FORBIDDEN[0]));
             ++i)
        {
            D_TESTS_MONGODB_RESULT(db,
                                   d_mongodb_use_database(db, FORBIDDEN[i]),
                                   D_MONGODB_ERROR_INVALID_NAMESPACE);
        }
    }

    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_name(db),
                                 "InvalidNamespace") == 0);
    D_TESTS_MONGODB_OK(db, d_mongodb_current_database(db, &current));
    D_TESTS_MONGODB_CHECK( (current) &&
                           (strcmp(current, other) == 0) );
    d_db_free(current);
    D_TESTS_MONGODB_OK(db, d_mongodb_use_database(db, NULL));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_current_database(db, &current),
                           D_DB_NOT_FOUND);
    D_TESTS_MONGODB_OK(db, d_mongodb_use_database(db, name));

    // NULL
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_has_database_s(db, NULL, &exists),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(!exists);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_has_database_s(db, name, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(!d_mongodb_has_database(NULL, name));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_list_databases(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_use_database(NULL, name),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_current_database(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_collections
  The collections of mongodb_schema.h.
  Tests the following:
  - create, with options -- a capped size -- and its info says so; has, a
    view not counting; the list, views left out
  - a create of one that is there is 48 on every server, unless
    D_DB_IF_NOT_EXISTS; a drop of one that is not, 26, unless
    D_DB_IF_EXISTS; a drop takes a view too
  - rename within the database and into another; onto one that is there,
    48, unless D_MONGODB_DROP_TARGET; of one that is not, 26
  - collection_info of nothing is 26
  - in a transaction: a drop and a rename are D_DB_UNSUPPORTED, the
    transaction open; a create joins it from 4.4
  - unknown flags, NULL arguments, and an empty name, 73
*/
static
void d_tests_mongodb_collections(void)
{
    char                    name[64];
    char                    other[96];
    struct dj_mongodb* const db     = d_tests_mongodb_begin("collections",
                                                           name,
                                                           sizeof(name));
    bool                    exists = true;
    char*                   info   = NULL;
    struct dj_db_name_list   names;

    printf("collections\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    snprintf(other, sizeof(other), "%s_to", name);

    // create, with options
    D_TESTS_MONGODB_OK(db, d_mongodb_has_collection_s(db, NULL, "c",
                                                      &exists));
    D_TESTS_MONGODB_CHECK(!exists);
    D_TESTS_MONGODB_OK(db, d_mongodb_create_collection(
                               db, NULL, "c",
                               "{\"capped\": true, \"size\": 65536}", 0u));
    D_TESTS_MONGODB_CHECK(d_mongodb_has_collection(db, NULL, "c"));
    D_TESTS_MONGODB_OK(db, d_mongodb_collection_info(db, NULL, "c", &info));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(info, "\"capped\" : true"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(info,
                                                   "\"type\" : "
                                                   "\"collection\""));
    d_db_free(info);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_collection(db, NULL, "c", NULL,
                                                       0u),
                           D_MONGODB_ERROR_NAMESPACE_EXISTS);
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_name(db),
                                 "NamespaceExists") == 0);
    D_TESTS_MONGODB_OK(db, d_mongodb_create_collection(db, NULL, "c", NULL,
                                                       D_DB_IF_NOT_EXISTS));

    // the list, views left out
    D_TESTS_MONGODB_OK(db, d_mongodb_create_view(db, NULL, "v", "c", NULL,
                                                 NULL, 0u));
    D_TESTS_MONGODB_OK(db, d_mongodb_create_collection(db, NULL, "d", NULL,
                                                       0u));
    D_TESTS_MONGODB_OK(db, d_mongodb_list_collections(db, NULL, &names));
    D_TESTS_MONGODB_CHECK( (d_tests_mongodb_has_name(&names, "c")) &&
                           (d_tests_mongodb_has_name(&names, "d")) &&
                           (!d_tests_mongodb_has_name(&names, "v")) );
    d_db_name_list_free(&names);
    D_TESTS_MONGODB_CHECK(!d_mongodb_has_collection(db, NULL, "v"));

    // renames
    D_TESTS_MONGODB_OK(db, d_mongodb_rename_collection(db, NULL, "d", NULL,
                                                       "e", 0u));
    D_TESTS_MONGODB_CHECK( (!d_mongodb_has_collection(db, NULL, "d")) &&
                           (d_mongodb_has_collection(db, NULL, "e")) );
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_rename_collection(db, NULL, "e", NULL,
                                                       "c", 0u),
                           D_MONGODB_ERROR_NAMESPACE_EXISTS);
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "e", "{\"k\": 1}",
                                                NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_rename_collection(db, NULL, "e", NULL,
                                                       "c",
                                                       D_MONGODB_DROP_TARGET));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(db, "c", "{\"k\": 1}") == 1);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_rename_collection(db, NULL, "nothing",
                                                       NULL, "x", 0u),
                           D_MONGODB_ERROR_NAMESPACE_NOT_FOUND);
    D_TESTS_MONGODB_OK(db, d_mongodb_rename_collection(db, NULL, "c", other,
                                                       "moved", 0u));
    D_TESTS_MONGODB_CHECK(d_mongodb_has_collection(db, other, "moved"));
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_database(db, other, 0u));

    // drops
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_drop_collection(db, NULL, "c", 0u),
                           D_MONGODB_ERROR_NAMESPACE_NOT_FOUND);
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_collection(db, NULL, "c",
                                                     D_DB_IF_EXISTS));
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_collection(db, NULL, "v", 0u));
    D_TESTS_MONGODB_CHECK(!d_mongodb_has_view(db, NULL, "v"));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_collection_info(db, NULL, "c", &info),
                           D_MONGODB_ERROR_NAMESPACE_NOT_FOUND);

    // in a transaction
    if (d_mongodb_supports(db, D_MONGODB_FEATURE_TRANSACTIONS))
    {
        D_TESTS_MONGODB_OK(db, d_mongodb_create_collection(db, NULL, "t",
                                                           NULL, 0u));
        D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction(db));
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_drop_collection(db, NULL, "t", 0u),
                               D_DB_UNSUPPORTED);
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_rename_collection(db, NULL, "t",
                                                           NULL, "u", 0u),
                               D_DB_UNSUPPORTED);
        D_TESTS_MONGODB_RESULT(db,
                               d_mongodb_create_collection(db, NULL, "n",
                                                           NULL, 0u),
                               (d_mongodb_server_at_least(db, 4u, 4u, 0u))
                                   ? 0
                                   : D_DB_UNSUPPORTED);
        D_TESTS_MONGODB_CHECK(!d_mongodb_in_failed_transaction(db));
        D_TESTS_MONGODB_OK(db, d_mongodb_commit(db));
        D_TESTS_MONGODB_CHECK(d_mongodb_has_collection(db, NULL, "n") ==
                              d_mongodb_server_at_least(db, 4u, 4u, 0u));
    }

    // refusals
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_collection(db, NULL, "c", NULL,
                                                       D_DB_IF_EXISTS),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_drop_collection(db, NULL, "c",
                                                     D_DB_IF_NOT_EXISTS),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_collection(db, NULL, NULL, NULL,
                                                       0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_collection(db, NULL, "", NULL,
                                                       0u),
                           D_MONGODB_ERROR_INVALID_NAMESPACE);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_collection(db, NULL, "x", "[]",
                                                       0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_has_collection_s(db, NULL, "x", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_list_collections(db, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_collection_info(db, NULL, "x", NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_views
  The views of mongodb_schema.h.
  Tests the following:
  - a view over a collection, its pipeline applied when read, its options
    given; a view over a view, which must be given the same collation, the
    server refusing another; has and the list, collections left out
  - a create of a name that is there, a collection or a view, is 48,
    unless D_DB_IF_NOT_EXISTS
  - drop_view drops a view and never a collection: a name with none is 26,
    a collection of it left as it is, unless D_DB_IF_EXISTS
  - a view is written to never: the server's 166
  - no source, a document where the pipeline is due: refused
*/
static
void d_tests_mongodb_views(void)
{
    char                    name[64];
    struct dj_mongodb* const db   = d_tests_mongodb_begin("views",
                                                         name,
                                                         sizeof(name));
    struct dj_db_name_list   names;
    struct dj_db_rows        rows;

    printf("views\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(
                               db, NULL, "c",
                               "[{\"n\": 1}, {\"n\": 2}, {\"n\": 3}]", 0u,
                               NULL));

    // a view, and a view of it
    D_TESTS_MONGODB_OK(db, d_mongodb_create_view(
                               db, NULL, "v", "c",
                               "[{\"$match\": {\"n\": {\"$gt\": 1}}}]",
                               "{\"collation\": {\"locale\": \"en\"}}", 0u));
    D_TESTS_MONGODB_OK(db, d_mongodb_create_view(
                               db, NULL, "w", "v",
                               "[{\"$project\": {\"_id\": 0}}]",
                               "{\"collation\": {\"locale\": \"en\"}}", 0u));

    const int conflict = d_mongodb_create_view(
                             db, NULL, "x", "v", NULL,
                             "{\"collation\": {\"locale\": \"fr\"}}", 0u);

    D_TESTS_MONGODB_CHECK( (conflict > 0) &&
                           (!d_mongodb_result_is_client(conflict)) );
    D_TESTS_MONGODB_CHECK(!d_mongodb_has_view(db, NULL, "x"));
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "w", NULL,
                                          "{\"sort\": {\"n\": 1}}", &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_row(&rows, 0u),
                                 "{ \"n\" : { \"$numberInt\" : \"2\" } }") ==
                          0);
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_CHECK(d_mongodb_has_view(db, NULL, "v"));
    D_TESTS_MONGODB_CHECK(!d_mongodb_has_view(db, NULL, "c"));
    D_TESTS_MONGODB_OK(db, d_mongodb_list_views(db, NULL, &names));
    D_TESTS_MONGODB_CHECK( (names.count == 2u) &&
                           (d_tests_mongodb_has_name(&names, "v")) &&
                           (d_tests_mongodb_has_name(&names, "w")) );
    d_db_name_list_free(&names);

    // names taken
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_view(db, NULL, "v", "c", NULL,
                                                 NULL, 0u),
                           D_MONGODB_ERROR_NAMESPACE_EXISTS);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_view(db, NULL, "c", "c", NULL,
                                                 NULL, 0u),
                           D_MONGODB_ERROR_NAMESPACE_EXISTS);
    D_TESTS_MONGODB_OK(db, d_mongodb_create_view(db, NULL, "c", "c", NULL,
                                                 NULL, D_DB_IF_NOT_EXISTS));

    // written to never
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_one(db, NULL, "v", "{}", NULL),
                           D_MONGODB_ERROR_COMMAND_NOT_SUPPORTED_ON_VIEW);

    // drops
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_drop_view(db, NULL, "c", 0u),
                           D_MONGODB_ERROR_NAMESPACE_NOT_FOUND);
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_view(db, NULL, "c",
                                               D_DB_IF_EXISTS));
    D_TESTS_MONGODB_CHECK(d_mongodb_has_collection(db, NULL, "c"));
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_view(db, NULL, "w", 0u));
    D_TESTS_MONGODB_CHECK(!d_mongodb_has_view(db, NULL, "w"));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_drop_view(db, NULL, "w", 0u),
                           D_MONGODB_ERROR_NAMESPACE_NOT_FOUND);

    // refusals
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_view(db, NULL, "x", NULL, NULL,
                                                 NULL, 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_view(db, NULL, "x", "c",
                                                 "{\"$match\": {}}", NULL,
                                                 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_has_view_s(db, NULL, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_list_views(NULL, NULL, &names),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_indexes
  The indexes of mongodb_schema.h.
  Tests the following:
  - create, has, the list with "_id_" first, the descriptions; the
    collection is created by an index on it
  - D_DB_UNIQUE makes a unique index, which refuses a repeated key with
    11000; options: sparse, partialFilterExpression
  - an index of a name that is there is D_DB_EXISTS, unless
    D_DB_IF_NOT_EXISTS; another's keys under a new name are 85, a name
    with other keys 86
  - a drop of one that is not there is 27 on every server, unless
    D_DB_IF_EXISTS; "_id_" the server refuses
  - a collection that is not there has no indexes
  - refusals: options that name the keys or the name, unique beside
    D_DB_UNIQUE, no keys, no name, unknown flags
*/
static
void d_tests_mongodb_indexes(void)
{
    char                    name[64];
    struct dj_mongodb* const db     = d_tests_mongodb_begin("indexes",
                                                           name,
                                                           sizeof(name));
    bool                    exists = true;
    struct dj_db_name_list   names;
    struct dj_db_rows        rows;

    printf("indexes\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    // nothing yet
    D_TESTS_MONGODB_OK(db, d_mongodb_has_index_s(db, NULL, "c", "_id_",
                                                 &exists));
    D_TESTS_MONGODB_CHECK(!exists);
    D_TESTS_MONGODB_OK(db, d_mongodb_list_indexes(db, NULL, "c", &names));
    D_TESTS_MONGODB_CHECK(names.count == 0u);
    d_db_name_list_free(&names);

    // create, unique, with options
    D_TESTS_MONGODB_OK(db, d_mongodb_create_index(db, NULL, "c", "k",
                                                  "{\"k\": 1}", NULL,
                                                  D_DB_UNIQUE));
    D_TESTS_MONGODB_CHECK(d_mongodb_has_collection(db, NULL, "c"));
    D_TESTS_MONGODB_OK(db, d_mongodb_create_index(
                               db, NULL, "c", "p", "{\"p\": -1, \"q\": 1}",
                               "{\"sparse\": true}", 0u));
    D_TESTS_MONGODB_OK(db, d_mongodb_create_index(
                               db, NULL, "c", "partial", "{\"r\": 1}",
                               "{\"partialFilterExpression\": {\"r\": "
                               "{\"$gt\": 5}}}", 0u));
    D_TESTS_MONGODB_CHECK(d_mongodb_has_index(db, NULL, "c", "p"));
    D_TESTS_MONGODB_OK(db, d_mongodb_list_indexes(db, NULL, "c", &names));
    D_TESTS_MONGODB_CHECK( (names.count == 4u) &&
                           (strcmp(names.names[0], "_id_") == 0) &&
                           (d_tests_mongodb_has_name(&names, "partial")) );
    d_db_name_list_free(&names);
    D_TESTS_MONGODB_OK(db, d_mongodb_describe_indexes(db, NULL, "c", &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 4u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_rows_hold(&rows,
                                                    "\"unique\" : true"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_rows_hold(&rows,
                                                    "\"sparse\" : true"));
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c", "{\"k\": 1}",
                                                NULL));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_one(db, NULL, "c", "{\"k\": 1}",
                                                NULL),
                           D_MONGODB_ERROR_DUPLICATE_KEY);

    // names and keys taken
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_index(db, NULL, "c", "p",
                                                  "{\"p\": -1, \"q\": 1}",
                                                  "{\"sparse\": true}", 0u),
                           D_DB_EXISTS);
    D_TESTS_MONGODB_OK(db, d_mongodb_create_index(db, NULL, "c", "p",
                                                  "{\"other\": 1}", NULL,
                                                  D_DB_IF_NOT_EXISTS));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_index(db, NULL, "c", "p2",
                                                  "{\"p\": -1, \"q\": 1}",
                                                  "{\"sparse\": true}", 0u),
                           D_MONGODB_ERROR_INDEX_OPTIONS_CONFLICT);

    // drops
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_index(db, NULL, "c", "p", 0u));
    D_TESTS_MONGODB_CHECK(!d_mongodb_has_index(db, NULL, "c", "p"));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_drop_index(db, NULL, "c", "p", 0u),
                           D_MONGODB_ERROR_INDEX_NOT_FOUND);
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_name(db),
                                 "IndexNotFound") == 0);
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_index(db, NULL, "c", "p",
                                                D_DB_IF_EXISTS));
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_index(db, NULL, "none", "p",
                                                D_DB_IF_EXISTS));
    D_TESTS_MONGODB_CHECK(d_mongodb_drop_index(db, NULL, "c", "_id_", 0u) >
                          0);

    // refusals
    {
        static const char* const OPTIONS[] =
        {
            "{\"key\": {\"a\": 1}}", "{\"name\": \"z\"}"
        };

        for (size_t i = 0u; i < (sizeof(OPTIONS) / sizeof(OPTIONS[0])); ++i)
        {
            D_TESTS_MONGODB_RESULT(db,
                                   d_mongodb_create_index(db, NULL, "c", "z",
                                                          "{\"z\": 1}",
                                                          OPTIONS[i], 0u),
                                   D_DB_INVALID_ARGUMENT);
        }
    }

    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_index(db, NULL, "c", "z",
                                                  "{\"z\": 1}",
                                                  "{\"unique\": false}",
                                                  D_DB_UNIQUE),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_index(db, NULL, "c", "z", NULL,
                                                  NULL, 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_index(db, NULL, "c", NULL,
                                                  "{\"z\": 1}", NULL, 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_create_index(db, NULL, "c", "z",
                                                  "{\"z\": 1}", NULL,
                                                  D_DB_IF_EXISTS),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_has_index_s(db, NULL, "c", NULL,
                                                 &exists),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_list_indexes(db, NULL, "c", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_describe_indexes(db, NULL, "c", NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}


//==============================================================================
// 7.  TRANSACTIONS
//==============================================================================

/*
d_tests_mongodb_transactions
  mongodb_transaction.h.
  Tests the following:
  - writes in a transaction are seen by another handle only once it
    commits, and never once it rolls back
  - begin with options: a snapshot read concern, a majority write concern,
    maxCommitTimeMS; options of no such name or value are refused
  - a second begin is the driver's code, the first still open
  - any failure the server reports ends the transaction: it reads failed,
    the next operation is 251 labelled TransientTransactionError, and a
    rollback ends it without a failure of its own
  - a refusal of the layer's leaves it open and not failed
  - a commit or a rollback with none open is the driver's code, before the
    handle's first transaction and after one
  - NULL arguments; state of NULL is false
*/
static
void d_tests_mongodb_transactions(void)
{
    char                    name[64];
    struct dj_mongodb* const db      = d_tests_mongodb_begin("transactions",
                                                            name,
                                                            sizeof(name));
    struct dj_mongodb*       watcher = NULL;

    printf("transactions\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    if (!d_mongodb_supports(db, D_MONGODB_FEATURE_TRANSACTIONS))
    {
        d_tests_mongodb_skip("transactions", "the deployment has none");
        d_tests_mongodb_end(db, name);

        return;
    }

    // none before the first
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(d_mongodb_commit(db)));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(d_mongodb_error_message(db),
                                                   "No transaction"));
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(d_mongodb_rollback(db)));
    D_TESTS_MONGODB_CHECK(!d_mongodb_in_transaction(db));

    D_TESTS_MONGODB_OK(db, d_mongodb_create_collection(db, NULL, "c", NULL,
                                                       0u));
    watcher = d_tests_mongodb_connect(g_uri);
    D_TESTS_MONGODB_OK(watcher, d_mongodb_use_database(watcher, name));

    // commit
    D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction(db));
    D_TESTS_MONGODB_CHECK(d_mongodb_in_transaction(db));
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c",
                                                "{\"_id\": 1}", NULL));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(db, "c", NULL) == 1);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(watcher, "c", NULL) == 0);
    D_TESTS_MONGODB_OK(db, d_mongodb_commit(db));
    D_TESTS_MONGODB_CHECK(!d_mongodb_in_transaction(db));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(watcher, "c", NULL) == 1);

    // rollback
    D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction(db));
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c",
                                                "{\"_id\": 2}", NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_rollback(db));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(watcher, "c", NULL) == 1);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(d_mongodb_rollback(db)));

    // options
    D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction_with(
                               db,
                               "{\"readConcern\": {\"level\": \"snapshot\"}, "
                               "\"writeConcern\": {\"w\": \"majority\"}, "
                               "\"readPreference\": {\"mode\": \"primary\"}, "
                               "\"maxCommitTimeMS\": 5000}"));
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(
                              d_mongodb_begin_transaction(db)));
    D_TESTS_MONGODB_CHECK(d_mongodb_in_transaction(db));
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c",
                                                "{\"_id\": 3}", NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_commit(db));

    {
        static const char* const OPTIONS[] =
        {
            "{\"readConcern\": {\"level\": \"bogus\"}}",
            "{\"writeConcern\": {\"w\": 0}}",
            "{\"readPreference\": {}}", "{\"frob\": 1}",
            "{\"maxCommitTimeMS\": -1}", "{\"readConcern\": 1}", "[]"
        };

        for (size_t i = 0u; i < (sizeof(OPTIONS) / sizeof(OPTIONS[0])); ++i)
        {
            D_TESTS_MONGODB_RESULT(db,
                                   d_mongodb_begin_transaction_with(
                                       db,
                                       OPTIONS[i]),
                                   D_DB_INVALID_ARGUMENT);
            D_TESTS_MONGODB_CHECK(!d_mongodb_in_transaction(db));
        }
    }

    // a failure the server reports ends it
    D_TESTS_MONGODB_OK(db, d_mongodb_begin_transaction(db));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_exec(db, NULL, "[]"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(!d_mongodb_in_failed_transaction(db));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_one(db, NULL, "c",
                                                "{\"_id\": 1}", NULL),
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_MONGODB_CHECK(d_mongodb_in_transaction(db));
    D_TESTS_MONGODB_CHECK(d_mongodb_in_failed_transaction(db));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_insert_one(db, NULL, "c",
                                                "{\"_id\": 4}", NULL),
                           D_MONGODB_ERROR_NO_SUCH_TRANSACTION);
    D_TESTS_MONGODB_CHECK(d_mongodb_error_has_label(
                              db,
                              "TransientTransactionError"));
    D_TESTS_MONGODB_OK(db, d_mongodb_rollback(db));
    D_TESTS_MONGODB_CHECK( (!d_mongodb_in_transaction(db)) &&
                           (!d_mongodb_in_failed_transaction(db)) );
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_count(watcher, "c", NULL) == 2);

    // NULL
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_begin_transaction(NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_commit(NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_rollback(NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(!d_mongodb_in_transaction(NULL));
    D_TESTS_MONGODB_CHECK(!d_mongodb_in_failed_transaction(NULL));
    d_mongodb_close(watcher);
    d_tests_mongodb_end(db, name);

    return;
}


//==============================================================================
// 8.  SETTINGS
//==============================================================================

/*
d_tests_mongodb_settings
  The concerns and the preference of mongodb_settings.h.
  Tests the following:
  - each reads back as set, as a command writes it, and NULL sets the
    deployment's default back
  - the write concern applies to the next write
  - refusals: a level, a mode or a field of no such name; a w of 0; tags or
    a staleness with the primary; a staleness under 90; no mode; text that
    is not JSON, libbson's code
  - NULL arguments
*/
static
void d_tests_mongodb_settings(void)
{
    char                    name[64];
    struct dj_mongodb* const db   = d_tests_mongodb_begin("settings",
                                                         name,
                                                         sizeof(name));
    char*                   text = NULL;
    struct dj_db_rows        rows;

    printf("settings\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    // the read concern
    D_TESTS_MONGODB_OK(db, d_mongodb_get_read_concern(db, &text));
    D_TESTS_MONGODB_CHECK( (text) &&
                           (strcmp(text, "{ }") == 0) );
    d_db_free(text);
    D_TESTS_MONGODB_OK(db, d_mongodb_set_read_concern(db,
                                                      "{\"level\": "
                                                      "\"majority\"}"));
    D_TESTS_MONGODB_OK(db, d_mongodb_get_read_concern(db, &text));
    D_TESTS_MONGODB_CHECK( (text) &&
                           (strcmp(text, "{ \"level\" : \"majority\" }") ==
                            0) );
    d_db_free(text);
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", NULL, NULL, &rows));
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_set_read_concern(db, NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_get_read_concern(db, &text));
    D_TESTS_MONGODB_CHECK( (text) &&
                           (strcmp(text, "{ }") == 0) );
    d_db_free(text);

    // the write concern
    D_TESTS_MONGODB_OK(db, d_mongodb_get_write_concern(db, &text));
    D_TESTS_MONGODB_CHECK( (text) &&
                           (strcmp(text, "{ }") == 0) );
    d_db_free(text);
    D_TESTS_MONGODB_OK(db, d_mongodb_set_write_concern(
                               db,
                               "{\"w\": \"majority\", \"j\": true, "
                               "\"wtimeout\": 5000}"));
    D_TESTS_MONGODB_OK(db, d_mongodb_get_write_concern(db, &text));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(text, "\"majority\""));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(text, "\"j\" : true"));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(text, "\"wtimeout\""));
    d_db_free(text);
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c", "{}", NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_set_write_concern(db, "{\"w\": 1}"));
    D_TESTS_MONGODB_OK(db, d_mongodb_get_write_concern(db, &text));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(text, "\"w\""));
    d_db_free(text);
    D_TESTS_MONGODB_OK(db, d_mongodb_set_write_concern(db, NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_get_write_concern(db, &text));
    D_TESTS_MONGODB_CHECK( (text) &&
                           (strcmp(text, "{ }") == 0) );
    d_db_free(text);

    // the read preference
    D_TESTS_MONGODB_OK(db, d_mongodb_get_read_preference(db, &text));
    D_TESTS_MONGODB_CHECK( (text) &&
                           (strcmp(text, "{ \"mode\" : \"primary\" }") ==
                            0) );
    d_db_free(text);
    D_TESTS_MONGODB_OK(db, d_mongodb_set_read_preference(
                               db,
                               "{\"mode\": \"secondaryPreferred\", \"tags\": "
                               "[{\"dc\": \"east\"}, {}], "
                               "\"maxStalenessSeconds\": 120}"));
    D_TESTS_MONGODB_OK(db, d_mongodb_get_read_preference(db, &text));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(text,
                                                   "\"secondaryPreferred\""));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(text, "\"east\""));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(text,
                                                   "\"maxStalenessSeconds\""));
    d_db_free(text);
    D_TESTS_MONGODB_OK(db, d_mongodb_set_read_preference(db, NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_get_read_preference(db, &text));
    D_TESTS_MONGODB_CHECK( (text) &&
                           (strcmp(text, "{ \"mode\" : \"primary\" }") ==
                            0) );
    d_db_free(text);

    // refusals
    {
        static const char* const CONCERNS[] =
        {
            "{\"level\": \"bogus\"}", "{\"level\": 1}", "{\"frob\": 1}",
            "[]"
        };
        static const char* const WRITES[] =
        {
            "{\"w\": 0}", "{\"w\": -1}", "{\"j\": 1}", "{\"wtimeout\": -5}",
            "{\"fsync\": true}"
        };
        static const char* const PREFERENCES[] =
        {
            "{}", "{\"mode\": \"bogus\"}",
            "{\"mode\": \"primary\", \"tags\": [{}]}",
            "{\"mode\": \"primary\", \"maxStalenessSeconds\": 120}",
            "{\"mode\": \"nearest\", \"maxStalenessSeconds\": 10}",
            "{\"mode\": \"nearest\", \"hedge\": {}}"
        };

        for (size_t i = 0u; i < (sizeof(CONCERNS) / sizeof(CONCERNS[0])); ++i)
        {
            D_TESTS_MONGODB_RESULT(db,
                                   d_mongodb_set_read_concern(db,
                                                              CONCERNS[i]),
                                   D_DB_INVALID_ARGUMENT);
        }

        for (size_t i = 0u; i < (sizeof(WRITES) / sizeof(WRITES[0])); ++i)
        {
            D_TESTS_MONGODB_RESULT(db,
                                   d_mongodb_set_write_concern(db, WRITES[i]),
                                   D_DB_INVALID_ARGUMENT);
        }

        for (size_t i = 0u;
             i < (sizeof(PREFERENCES) / sizeof(PREFERENCES[0]));
             ++i)
        {
            D_TESTS_MONGODB_RESULT(db,
                                   d_mongodb_set_read_preference(
                                       db,
                                       PREFERENCES[i]),
                                   D_DB_INVALID_ARGUMENT);
        }
    }

    D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(
                              d_mongodb_set_read_concern(db, "{\"level\"")) ==
                          (unsigned)(D_MONGODB_DOMAIN_BSON + BSON_ERROR_JSON));

    // NULL
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_set_read_concern(NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_get_read_concern(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_get_write_concern(NULL, &text),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_get_read_preference(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_document_format
  The handle's document format.
  Tests the following:
  - canonical by default: every value's type written out
  - relaxed: numbers as JSON numbers, an int64 of 32 bits as a number
  - BSON: each row a BLOB of the document's bytes, its size the document's
    length; a value, distinct's, still JSON, canonical
  - the format of no such value is refused; NULL reads canonical
*/
static
void d_tests_mongodb_document_format(void)
{
    char                    name[64];
    struct dj_mongodb* const db   = d_tests_mongodb_begin("format",
                                                         name,
                                                         sizeof(name));
    struct dj_db_rows        rows;

    printf("document format\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(
                               db, NULL, "c",
                               "{\"_id\": 1, \"l\": {\"$numberLong\": \"7\"}, "
                               "\"d\": 1.5}",
                               NULL));

    // canonical
    D_TESTS_MONGODB_CHECK(d_mongodb_get_document_format(db) ==
                          D_MONGODB_JSON_CANONICAL);
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", NULL, NULL, &rows));
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_row(&rows, 0u),
                                 "{ \"_id\" : { \"$numberInt\" : \"1\" }, "
                                 "\"l\" : { \"$numberLong\" : \"7\" }, "
                                 "\"d\" : { \"$numberDouble\" : \"1.5\" } }") ==
                          0);
    d_db_rows_free(&rows);

    // relaxed
    D_TESTS_MONGODB_OK(db, d_mongodb_set_document_format(
                               db, D_MONGODB_JSON_RELAXED));
    D_TESTS_MONGODB_CHECK(d_mongodb_get_document_format(db) ==
                          D_MONGODB_JSON_RELAXED);
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", NULL, NULL, &rows));
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_row(&rows, 0u),
                                 "{ \"_id\" : 1, \"l\" : 7, \"d\" : 1.5 }") ==
                          0);
    d_db_rows_free(&rows);

    // BSON
    D_TESTS_MONGODB_OK(db, d_mongodb_set_document_format(db,
                                                         D_MONGODB_BSON));
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", NULL, NULL, &rows));

    {
        const struct dj_db_value* const value = d_db_rows_value(&rows, 0u, 0u);
        bson_t                         document;

        D_TESTS_MONGODB_CHECK( (value) &&
                               (value->type == D_DB_VALUE_BLOB) );
        D_TESTS_MONGODB_CHECK( (value) &&
                               (bson_init_static(&document,
                                                 (const uint8_t*)
                                                 value->as.blob,
                                                 value->size)) &&
                               (bson_count_keys(&document) == 3u) );
    }

    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_distinct(db, NULL, "c", "l", NULL, NULL,
                                              &rows));
    D_TESTS_MONGODB_CHECK(strcmp(d_tests_mongodb_row(&rows, 0u),
                                 "{ \"$numberLong\" : \"7\" }") == 0);
    d_db_rows_free(&rows);

    // refusals, and NULL
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_set_document_format(
                               db,
                               (enum dj_mongodb_document_format)7),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_CHECK(d_mongodb_get_document_format(db) ==
                          D_MONGODB_BSON);
    D_TESTS_MONGODB_CHECK(d_mongodb_get_document_format(NULL) ==
                          D_MONGODB_JSON_CANONICAL);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_set_document_format(
                               NULL,
                               D_MONGODB_JSON_RELAXED),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_time_limit
  The handle's time limit for reads.
  Tests the following:
  - 0 by default and for NULL; a negative one is refused
  - a read that runs past it fails with 50, MaxTimeMSExpired: a find, a
    count, an aggregation
  - a read whose options name their own maxTimeMS keeps it
  - 0 takes the limit off
*/
static
void d_tests_mongodb_time_limit(void)
{
    char                    name[64];
    struct dj_mongodb* const db    = d_tests_mongodb_begin("time",
                                                          name,
                                                          sizeof(name));
    struct dj_db_rows        rows;
    int64_t                 count = 0;
    static const char       SLOW[] = "{\"$where\": \"sleep(100) || true\"}";
    // a count aggregates, where $where is not allowed; $function is, from
    // 4.4
    static const char       SLOW_COUNT[] =
        "{\"$expr\": {\"$function\": {\"body\": \"function() { sleep(100); "
        "return true; }\", \"args\": [], \"lang\": \"js\"}}}";
    const bool              counts = (d_mongodb_server_version_number(db) >=
                                      40400ul);

    printf("time limit\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(db, NULL, "c",
                                                 "[{}, {}, {}]", 0u, NULL));
    D_TESTS_MONGODB_CHECK(d_mongodb_get_max_time_ms(db) == 0);
    D_TESTS_MONGODB_CHECK(d_mongodb_get_max_time_ms(NULL) == 0);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_set_max_time_ms(db, -1),
                           D_DB_INVALID_ARGUMENT);

    // past the limit
    D_TESTS_MONGODB_OK(db, d_mongodb_set_max_time_ms(db, 50));
    D_TESTS_MONGODB_CHECK(d_mongodb_get_max_time_ms(db) == 50);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_find(db, NULL, "c", SLOW, NULL, &rows),
                           D_MONGODB_ERROR_MAX_TIME_MS_EXPIRED);
    D_TESTS_MONGODB_CHECK(strcmp(d_mongodb_error_name(db),
                                 "MaxTimeMSExpired") == 0);

    // a count's, where it can be slowed
    if (counts)
    {
        const int slow = d_mongodb_count_documents(db, NULL, "c", SLOW_COUNT,
                                                   NULL, &count);

        // 5.0 can report the time run out inside the JavaScript as 11601,
        // Interrupted
        D_TESTS_MONGODB_CHECK( (slow == D_MONGODB_ERROR_MAX_TIME_MS_EXPIRED) ||
                               (slow == 11601) );
    }
    else
    {
        d_tests_mongodb_skip("a count past the limit", "$function from 4.4");
    }

    // the options' own
    D_TESTS_MONGODB_OK(db, d_mongodb_find(db, NULL, "c", SLOW,
                                          "{\"maxTimeMS\": 60000}", &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 3u);
    d_db_rows_free(&rows);

    // none
    D_TESTS_MONGODB_OK(db, d_mongodb_set_max_time_ms(db, 0));
    D_TESTS_MONGODB_OK(db, d_mongodb_count_documents(
                               db, NULL, "c", (counts) ? SLOW_COUNT : NULL,
                               NULL, &count));
    D_TESTS_MONGODB_CHECK(count == 3);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_set_max_time_ms(NULL, 5),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}


//==============================================================================
// 9.  GRIDFS AND MAINTENANCE
//==============================================================================

/*
d_tests_mongodb_write_file
  Writes bytes to a local file; whether it could.
*/
static
bool d_tests_mongodb_write_file(
    const char* _path,
    const void* _data,
    size_t      _size
)
{
    FILE* const file = fopen(_path, "wb");
    bool        ok   = (file != NULL);

    // the bytes, then the close
    if (file)
    {
        ok = (fwrite(_data, 1u, _size, file) == _size);
        ok = (fclose(file) == 0) && (ok);
    }

    return ok;
}

/*
d_tests_mongodb_same_file
  Whether a local file holds exactly the bytes given.
*/
static
bool d_tests_mongodb_same_file(
    const char*          _path,
    const unsigned char* _data,
    size_t               _size
)
{
    FILE* const   file = fopen(_path, "rb");
    unsigned char piece[4096];
    size_t        done = 0u;
    bool          same = (file != NULL);

    // each piece, against the bytes
    while (same)
    {
        const size_t got = fread(piece, 1u, sizeof(piece), file);

        if (got == 0u)
        {
            break;
        }

        same  = ( ((done + got) <= _size) &&
                  (memcmp(piece, _data + done, got) == 0) );
        done += got;
    }

    if (file)
    {
        (void)fclose(file);
    }

    return ( (same) &&
             (done == _size) );
}

/*
d_tests_mongodb_gridfs
  mongodb_gridfs.h.
  Tests the following:
  - bytes uploaded from memory download as they were, a NUL after them:
    a few, none, and many more than a chunk, every byte value among them
  - metadata kept, and found by a filter over the files' documents
  - a name's newest revision is the one uploaded last
  - a bucket of another name keeps its own files
  - a local file uploaded, and downloaded into another, byte for byte
  - a delete takes the file: a download of it then, and a second delete,
    are D_DB_NOT_FOUND; a name with no file is D_DB_NOT_FOUND
  - a local file that cannot be opened is the driver's code; an id that is
    not JSON is libbson's, one with text after it D_DB_INVALID_ARGUMENT;
    NULL arguments are refused
*/
static
void d_tests_mongodb_gridfs(void)
{
    char                    name[64];
    char                    from[96];
    char                    into[96];
    struct dj_mongodb* const db     = d_tests_mongodb_begin("gridfs",
                                                           name,
                                                           sizeof(name));
    static unsigned char    big[600000];
    char*                   id     = NULL;
    char*                   newest = NULL;
    char*                   found  = NULL;
    void*                   data   = NULL;
    size_t                  size   = 0u;
    struct dj_db_rows        rows;

    printf("gridfs\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    for (size_t i = 0u; i < sizeof(big); ++i)
    {
        big[i] = (unsigned char)((i * 7u) & 0xFFu);
    }

    // a few bytes, with metadata
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_upload(db, NULL, NULL, "a.txt",
                                                   "hello", 5u,
                                                   "{\"kind\": \"greeting\"}",
                                                   &id));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(id, "$oid"));
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_download(db, NULL, NULL, id,
                                                     &data, &size));
    D_TESTS_MONGODB_CHECK( (size == 5u) &&
                           (memcmp(data, "hello", 6u) == 0) );
    d_db_free(data);

    // revisions
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_upload(db, NULL, NULL, "a.txt",
                                                   "v2", 2u, NULL, &newest));
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_find_id(db, NULL, NULL, "a.txt",
                                                    &found));
    D_TESTS_MONGODB_CHECK( (found) &&
                           (newest) &&
                           (strcmp(found, newest) == 0) );
    d_db_free(found);
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_find(db, NULL, NULL,
                                                 "{\"filename\": \"a.txt\"}",
                                                 NULL, &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 2u);
    d_db_rows_free(&rows);
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_find(db, NULL, NULL,
                                                 "{\"metadata.kind\": "
                                                 "\"greeting\"}",
                                                 NULL, &rows));
    D_TESTS_MONGODB_CHECK(rows.row_count == 1u);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(
                              d_tests_mongodb_row(&rows, 0u), "\"length\""));
    d_db_rows_free(&rows);

    // none, and many chunks, in another bucket
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_upload(db, NULL, "b", "empty",
                                                   NULL, 0u, NULL, &found));
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_download(db, NULL, "b", found,
                                                     &data, &size));
    D_TESTS_MONGODB_CHECK( (size == 0u) &&
                           (data) &&
                           (((char*)data)[0] == '\0') );
    d_db_free(data);
    d_db_free(found);
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_upload(db, NULL, "b", "big",
                                                   big, sizeof(big), NULL,
                                                   &found));
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_download(db, NULL, "b", found,
                                                     &data, &size));
    D_TESTS_MONGODB_CHECK( (size == sizeof(big)) &&
                           (memcmp(data, big, sizeof(big)) == 0) );
    d_db_free(data);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_download(db, NULL, NULL, found,
                                                     &data, &size),
                           D_DB_NOT_FOUND);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_find_id(db, NULL, NULL, "big",
                                                    &newest),
                           D_DB_NOT_FOUND);
    d_db_free(found);
    found = NULL;

    // local files
    snprintf(from, sizeof(from), "%s_from.bin", name);
    snprintf(into, sizeof(into), "%s_into.bin", name);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_write_file(from, big, sizeof(big)));
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_upload_file(db, NULL, NULL,
                                                        "local", from, NULL,
                                                        &found));
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_download_file(db, NULL, NULL,
                                                          found, into));
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_same_file(into, big, sizeof(big)));
    (void)remove(from);
    (void)remove(into);
    D_TESTS_MONGODB_CHECK(d_mongodb_result_is_client(
                              d_mongodb_gridfs_upload_file(
                                  db, NULL, NULL, "x",
                                  "dtm_no_such_dir/no_such_file", NULL,
                                  NULL)));

    // deletes
    D_TESTS_MONGODB_OK(db, d_mongodb_gridfs_delete(db, NULL, NULL, found));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_download(db, NULL, NULL, found,
                                                     &data, &size),
                           D_DB_NOT_FOUND);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_delete(db, NULL, NULL, found),
                           D_DB_NOT_FOUND);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_find_id(db, NULL, NULL, "none",
                                                    &newest),
                           D_DB_NOT_FOUND);
    d_db_free(found);

    // refusals
    D_TESTS_MONGODB_CHECK(d_mongodb_result_domain(
                              d_mongodb_gridfs_download(db, NULL, NULL,
                                                        "nothing", &data,
                                                        &size)) ==
                          (unsigned)(D_MONGODB_DOMAIN_BSON +
                                     BSON_ERROR_JSON));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_download(db, NULL, NULL,
                                                     "\"a\" b", &data,
                                                     &size),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_upload(db, NULL, NULL, NULL,
                                                   "x", 1u, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_upload(db, NULL, NULL, "x", NULL,
                                                   1u, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_upload(db, NULL, NULL, "x", "x",
                                                   1u, "[]", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_download(db, NULL, NULL, id,
                                                     NULL, &size),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_download_file(db, NULL, NULL, id,
                                                          NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_find(db, NULL, NULL, NULL, NULL,
                                                 NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_find_id(db, NULL, NULL, NULL,
                                                    &found),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_gridfs_delete(NULL, NULL, NULL, id),
                           D_DB_INVALID_ARGUMENT);
    d_db_free(id);
    d_db_free(newest);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_maintenance
  mongodb_maintenance.h.
  Tests the following:
  - a collection's size and a database's are more than 0; a view's, and
    nothing's, are D_DB_NOT_FOUND, as is a database that is not there
  - compact succeeds on the primary, which takes it only by force, and on
    a standalone
  - validate finds a sound collection sound, its report the server's, the
    full checks too
  - unknown flags and NULL arguments are refused
*/
static
void d_tests_mongodb_maintenance(void)
{
    char                    name[64];
    char                    other[96];
    struct dj_mongodb* const db     = d_tests_mongodb_begin("maintenance",
                                                           name,
                                                           sizeof(name));
    int64_t                 bytes  = 0;
    bool                    valid  = false;
    char*                   report = NULL;

    printf("maintenance\n");

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    snprintf(other, sizeof(other), "%s_none", name);
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_many(
                               db, NULL, "c",
                               "[{\"s\": \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                               "a\"}, {\"s\": \"bbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                               "bbbb\"}]",
                               0u, NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_create_view(db, NULL, "v", "c", NULL,
                                                 NULL, 0u));

    // sizes
    D_TESTS_MONGODB_OK(db, d_mongodb_collection_size(db, NULL, "c", &bytes));
    D_TESTS_MONGODB_CHECK(bytes > 0);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_collection_size(db, NULL, "v", &bytes),
                           D_DB_NOT_FOUND);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_collection_size(db, NULL, "none",
                                                     &bytes),
                           D_DB_NOT_FOUND);
    D_TESTS_MONGODB_OK(db, d_mongodb_database_size(db, NULL, &bytes));
    D_TESTS_MONGODB_CHECK(bytes > 0);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_database_size(db, other, &bytes),
                           D_DB_NOT_FOUND);

    // compact and validate
    D_TESTS_MONGODB_OK(db, d_mongodb_compact(db, NULL, "c"));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_compact(db, NULL, "none"),
                           D_DB_NOT_FOUND);
    D_TESTS_MONGODB_OK(db, d_mongodb_validate(db, NULL, "c", 0u, &valid,
                                              &report));
    D_TESTS_MONGODB_CHECK(valid);
    D_TESTS_MONGODB_CHECK(d_tests_mongodb_contains(report,
                                                   "\"valid\" : true"));
    d_db_free(report);
    valid = false;
    D_TESTS_MONGODB_OK(db, d_mongodb_validate(db, NULL, "c",
                                              D_MONGODB_VALIDATE_FULL, &valid,
                                              NULL));
    D_TESTS_MONGODB_CHECK(valid);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_validate(db, NULL, "v", 0u, &valid,
                                              NULL),
                           D_DB_NOT_FOUND);

    // refusals
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_validate(db, NULL, "c", 0x8000u, &valid,
                                              NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_validate(db, NULL, "c", 0u, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_collection_size(db, NULL, "c", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_database_size(db, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_compact(db, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_compact(NULL, NULL, "c"),
                           D_DB_INVALID_ARGUMENT);
    d_tests_mongodb_end(db, name);

    return;
}

/*
d_tests_mongodb_standalone
  What a standalone server cannot do, where one is named.
  Tests the following:
  - its topology is standalone, with no replica set's name
  - it has no transactions and no change streams: begin and watch are
    D_DB_UNSUPPORTED, the stream NULL
  - it compacts, the force the layer sends taken
*/
static
void d_tests_mongodb_standalone(void)
{
    struct dj_mongodb*               db     = NULL;
    struct dj_mongodb_change_stream* stream =
        (struct dj_mongodb_change_stream*)&db;

    printf("standalone\n");

    // a standalone server, where one is named
    if (!g_standalone)
    {
        d_tests_mongodb_skip("standalone",
                             "DJINTERP_TEST_MONGODB_STANDALONE_URI unset");

        return;
    }

    db = d_tests_mongodb_connect(g_standalone);

    if (!db)
    {
        D_TESTS_MONGODB_CHECK(db != NULL);

        return;
    }

    D_TESTS_MONGODB_CHECK(d_mongodb_server_topology(db) ==
                          D_MONGODB_TOPOLOGY_STANDALONE);
    D_TESTS_MONGODB_CHECK(d_mongodb_server_replica_set(db) == NULL);
    D_TESTS_MONGODB_CHECK(!d_mongodb_supports(db,
                                              D_MONGODB_FEATURE_TRANSACTIONS));
    D_TESTS_MONGODB_CHECK(!d_mongodb_supports(
                              db,
                              D_MONGODB_FEATURE_CHANGE_STREAMS));
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_begin_transaction(db),
                           D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_RESULT(db,
                           d_mongodb_watch_deployment(db, NULL, NULL,
                                                      &stream),
                           D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_CHECK(stream == NULL);

    // a compaction
    char name[64];

    snprintf(name, sizeof(name), "%s_alone", g_run);
    D_TESTS_MONGODB_OK(db, d_mongodb_use_database(db, name));
    D_TESTS_MONGODB_OK(db, d_mongodb_insert_one(db, NULL, "c", "{}", NULL));
    D_TESTS_MONGODB_OK(db, d_mongodb_compact(db, NULL, "c"));
    D_TESTS_MONGODB_OK(db, d_mongodb_drop_database(db, name, 0u));
    D_TESTS_MONGODB_OK(db, d_mongodb_close(db));

    return;
}


//==============================================================================
// RUN
//==============================================================================

// d_tests_mongodb_all
//   table: every test, in the headers' order.
static void (* const d_tests_mongodb_all[])(void) =
{
    d_tests_mongodb_results,
    d_tests_mongodb_library,
    d_tests_mongodb_server,
    d_tests_mongodb_features,
    d_tests_mongodb_errors,
    d_tests_mongodb_connect_fields,
    d_tests_mongodb_connect_failures,
    d_tests_mongodb_connect_settings,
    d_tests_mongodb_adopt,
    d_tests_mongodb_latest_write,
    d_tests_mongodb_commands,
    d_tests_mongodb_json,
    d_tests_mongodb_query_rows,
    d_tests_mongodb_insert,
    d_tests_mongodb_find,
    d_tests_mongodb_find_keyed,
    d_tests_mongodb_update,
    d_tests_mongodb_delete_count,
    d_tests_mongodb_aggregate,
    d_tests_mongodb_find_and_modify,
    d_tests_mongodb_bulk,
    d_tests_mongodb_cursors,
    d_tests_mongodb_change_streams,
    d_tests_mongodb_databases,
    d_tests_mongodb_collections,
    d_tests_mongodb_views,
    d_tests_mongodb_indexes,
    d_tests_mongodb_transactions,
    d_tests_mongodb_settings,
    d_tests_mongodb_document_format,
    d_tests_mongodb_time_limit,
    d_tests_mongodb_gridfs,
    d_tests_mongodb_maintenance,
    d_tests_mongodb_standalone
};

/*
main
  Names the run, checks the deployment can be reached, runs every test and
reports; the exit status is 0 only if every check passed. Without a
deployment to reach, it says how to name one, and fails.
*/
int main(void)
{
    const size_t      count = sizeof(d_tests_mongodb_all) /
                              sizeof(d_tests_mongodb_all[0]);
    char              oid[25];
    bson_oid_t        run;
    struct dj_mongodb* setup = NULL;

    g_uri        = d_tests_mongodb_setting("DJINTERP_TEST_MONGODB_URI", g_uri);
    g_standalone = d_tests_mongodb_setting(
                       "DJINTERP_TEST_MONGODB_STANDALONE_URI",
                       NULL);
    setup        = d_tests_mongodb_connect(g_uri);

    // a deployment to test against: none named, or the one named, which
    // cannot be connected to, and why
    if (!setup)
    {
        if (getenv("DJINTERP_TEST_MONGODB_URI"))
        {
            printf("no deployment: cannot connect to "
                   "DJINTERP_TEST_MONGODB_URI's: %s\n",
                   g_unreached);
        }
        else
        {
            printf("no deployment: set DJINTERP_TEST_MONGODB_URI\n");
        }

        return 1;
    }

    d_mongodb_close(setup);
    bson_oid_init(&run, NULL);
    bson_oid_to_string(&run, oid);
    snprintf(g_run, sizeof(g_run), "dtm%s", oid);

    // each group, its report out before the next begins
    for (size_t i = 0u; i < count; ++i)
    {
        d_tests_mongodb_all[i]();
        fflush(stdout);
    }

    printf("%d checks, %d failed, %d skipped\n",
           g_checks,
           g_failures,
           g_skipped);

    return (g_failures == 0) ? 0 : 1;
}
