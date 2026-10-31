/*******************************************************************************
* djinterp [core]                                             t_mongodb_core.cpp
*
*   core/db's MongoDB connection, mongo_connection, over the MongoDB layer,
* against a live deployment -- MongoDB from 4.2, a replica set, through
* whichever driver the layer is built with, 1.21 and later of either line.
* MongoDB has no SQL, so the shared checks of t_db_core.hpp, written in SQL,
* do not apply; this harness checks what they check -- running a statement,
* here a command, and reading its rows, transactions and their guard,
* failures and the refusals of text a NUL would cut short, and how what the
* connection hands out lives and dies with it -- over MongoDB's own
* members, and then those: documents, cursors and change streams, the
* schema, GridFS, upkeep, settings, the server and the latest failure; the
* gating of B6, which leaves the connection no prepare; the operations that
* do not block, refused; and mongo_table, its cache, its commit and its
* collection.
*   Deployment: DJINTERP_TEST_MONGODB_URI, a connection string, says where
* and as whom (default mongodb://127.0.0.1:27017/?replicaSet=rs0); the user
* needs the root role. It carries the password, so give it in the
* environment only, never in a file. DJINTERP_TEST_MONGODB_STANDALONE_URI,
* optional, names a standalone server, where a transaction and a change
* stream are refused and an atomic table commit with them. Without a
* deployment only the checks that need none run, unless
* DJINTERP_TEST_MONGODB_REQUIRED is 1, when that fails. Over a layer built
* without the driver (D_CFG_MONGODB 0) it checks that nothing connects.
*   Each run works in a database of its own, named after the moment it
* began, dropped at the end.
*   Below C++17 core/db compiles to nothing, and so does this test.
*
*   Build: compile the layer's C sources and src/djinterp/c/sync/dmutex.c as
* C, src/djinterp/cpp/db/database.cpp, database_statement.cpp and
* mongodb/mongodb.cpp and this file as C++17 or later, and link them with
* the driver and -lpthread -- scripts/ci/check_db_mongodb.sh does.
*
*
* path:      /tests/unintegrated/cpp/db/mongodb/t_mongodb_core.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// djinterp
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER
// std
#include <cstdio>  // std::printf

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>       // std::chrono::system_clock
#include <cstdio>       // std::remove, std::fopen, std::fclose
#include <memory>       // std::unique_ptr
#include <optional>     // std::optional
#include <string>       // std::string, std::to_string
#include <type_traits>  // std::true_type, std::false_type, std::void_t
#include <utility>      // std::declval, std::move
#include <vector>       // std::vector
// djinterp
#include "../../../../../inc/djinterp/cpp/db/mongodb/mongodb.hpp"  // under
                                                                     // test
#include "../../../../../inc/djinterp/cpp/db/mongodb/mongodb_table.hpp"
#include "../t_db_core.hpp"  // the counts and the check macros


namespace
{

namespace db   = djinterp;
namespace core = d_tests_db_core;

using db::mongo_connection;


//==============================================================================
// AT COMPILE TIME
//==============================================================================

// prepare_t, stream_query_t, ... -- the members B6 gates, as calls
template<typename Type,
         typename = void>
struct has_prepare : std::false_type
{};

template<typename Type>
struct has_prepare<Type,
                   std::void_t<decltype(std::declval<Type&>().prepare(
                       std::declval<const std::string&>()))>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_stream_query : std::false_type
{};

template<typename Type>
struct has_stream_query<Type,
                        std::void_t<decltype(std::declval<Type&>()
                                                 .stream_query(
                            std::declval<const std::string&>()))>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_isolation : std::false_type
{};

template<typename Type>
struct has_isolation<Type,
                     std::void_t<decltype(std::declval<Type&>()
                                              .begin_transaction(
                         std::declval<db::isolation_level>()))>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_savepoints : std::false_type
{};

template<typename Type>
struct has_savepoints<Type,
                      std::void_t<decltype(std::declval<Type&>()
                                               .create_savepoint(
                          std::declval<const std::string&>()))>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_set_auto_commit : std::false_type
{};

template<typename Type>
struct has_set_auto_commit<Type,
                           std::void_t<decltype(std::declval<Type&>()
                                                    .set_auto_commit(true))>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_transactions : std::false_type
{};

template<typename Type>
struct has_transactions<
    Type,
    std::void_t<decltype(std::declval<Type&>().begin_transaction()),
                decltype(std::declval<Type&>().commit()),
                decltype(std::declval<Type&>().rollback()),
                decltype(std::declval<const Type&>().in_transaction())>>
    : std::true_type
{};

// B6: no prepared statements, so no prepare and no stream_query; no
// isolation levels, savepoints or auto-commit mode; transactions, gated each
// by its own hook, there
static_assert(!has_prepare<mongo_connection>::value);
static_assert(!has_stream_query<mongo_connection>::value);
static_assert(!has_isolation<mongo_connection>::value);
static_assert(!has_savepoints<mongo_connection>::value);
static_assert(!has_set_auto_commit<mongo_connection>::value);
static_assert(has_transactions<mongo_connection>::value);
static_assert(
    db::internal::has_transaction_state<mongo_connection>::value);
static_assert(
    !db::is_detected<db::internal::prepare_hook_t, mongo_connection>::value);

// the operations that do not block exist, and refuse (ruling 11.26.6)
static_assert(db::has_async_operations<mongo_connection>::value);
static_assert(!db::has_async_operations<int>::value);

// the native handle is the layer's
static_assert(std::is_same_v<mongo_connection::native_handle_type,
                             struct ::dj_mongodb*>);

// the traits, against the class, and refused for int
static_assert(db::is_mongo_connection<mongo_connection>::value);
static_assert(db::is_mongo_connection_v<mongo_connection&>);
static_assert(!db::is_mongo_connection<int>::value);
static_assert(db::has_mongo_document_crud<mongo_connection>::value);
static_assert(db::has_mongo_collections<mongo_connection>::value);
static_assert(db::has_mongo_change_streams<mongo_connection>::value);
static_assert(db::has_mongo_gridfs<mongo_connection>::value);
static_assert(db::has_mongo_transactions<mongo_connection>::value);
static_assert(db::has_mongo_indexes<mongo_connection>::value);
static_assert(db::mongo_can_insert<mongo_connection>);
static_assert(db::mongo_can_find<mongo_connection>);
static_assert(db::mongo_can_aggregate<mongo_connection>);
static_assert(db::mongo_can_watch<mongo_connection>);
static_assert(db::mongo_can_bulk_write<mongo_connection>);
static_assert(db::mongo_can_gridfs<mongo_connection>);
static_assert(db::mongo_can_transact<mongo_connection>);
static_assert(db::mongo_can_create_collection<mongo_connection>);
static_assert(db::mongo_does_document_crud<mongo_connection>);
static_assert(db::mongo_does_change_streams<mongo_connection>);
static_assert(db::mongo_does_transactions<mongo_connection>);
static_assert(db::mongo_is_full_connection<mongo_connection>);
static_assert(!db::mongo_is_full_connection<int>);
static_assert(
    std::is_void_v<db::enable_if_mongo_connection<mongo_connection>>);

// the concepts (C++20) -- djinterp::concepts' own, the class's name among
// them, and the former names, which forward for one release
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
static_assert(db::concepts::async_connection<mongo_connection>);
static_assert(db::concepts::mongo_connection<mongo_connection>);
static_assert(db::concepts::mongo_connection<mongo_connection&>);
static_assert(db::concepts::mongodb_connection<mongo_connection>);
static_assert(!db::concepts::mongo_connection<int>);
static_assert(db::concepts::non_mongo_connection<int>);
static_assert(!db::concepts::non_mongo_connection<mongo_connection>);
static_assert(db::concepts::mongo_document_connection<mongo_connection>);
static_assert(db::concepts::mongo_collection_connection<mongo_connection>);
static_assert(db::concepts::mongo_aggregation_connection<mongo_connection>);
static_assert(db::concepts::mongo_change_stream_connection<mongo_connection>);
static_assert(db::concepts::mongo_gridfs_connection<mongo_connection>);
static_assert(db::concepts::mongo_transaction_connection<mongo_connection>);
static_assert(db::concepts::mongo_index_connection<mongo_connection>);
static_assert(db::concepts::mongo_insert_connection<mongo_connection>);
static_assert(db::concepts::mongo_find_one_connection<mongo_connection>);
static_assert(db::concepts::mongo_update_one_connection<mongo_connection>);
static_assert(db::concepts::mongo_delete_one_connection<mongo_connection>);
static_assert(db::concepts::mongo_replace_one_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_create_collection_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_drop_collection_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_collection_query_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_collection_list_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_watch_collection_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_watch_database_connection<mongo_connection>);
static_assert(db::concepts::mongo_cursor_connection<mongo_connection>);
static_assert(db::concepts::mongo_bulk_write_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_gridfs_upload_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_gridfs_download_connection<mongo_connection>);
static_assert(db::concepts::mongo_create_index_connection<mongo_connection>);
static_assert(db::concepts::mongo_list_indexes_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_begin_transaction_connection<mongo_connection>);
static_assert(db::concepts::mongo_commit_connection<mongo_connection>);
static_assert(db::concepts::mongo_rollback_connection<mongo_connection>);
static_assert(db::concepts::mongo_read_concern_connection<mongo_connection>);
static_assert(db::concepts::mongo_write_concern_connection<mongo_connection>);
static_assert(db::concepts::mongo_find_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_count_documents_connection<mongo_connection>);
static_assert(db::concepts::mongo_insertable_connection<mongo_connection>);
static_assert(db::concepts::mongo_findable_connection<mongo_connection>);
static_assert(db::concepts::mongo_aggregating_connection<mongo_connection>);
static_assert(db::concepts::mongo_watchable_connection<mongo_connection>);
static_assert(db::concepts::mongo_bulk_writable_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_gridfs_capable_connection<mongo_connection>);
static_assert(db::concepts::mongo_transactable_connection<mongo_connection>);
static_assert(
    db::concepts::mongo_collection_creating_connection<mongo_connection>);
static_assert(db::concepts::mongo_crud_connection<mongo_connection>);
static_assert(db::concepts::mongo_streaming_connection<mongo_connection>);
static_assert(db::concepts::mongo_transactional_connection<mongo_connection>);
static_assert(db::concepts::mongo_full_connection<mongo_connection>);
static_assert(db::Mongo_connection<mongo_connection>);
static_assert(!db::Mongo_connection<int>);
static_assert(db::mongo_start_transaction_connection<mongo_connection>);
static_assert(db::mongo_commit_transaction_connection<mongo_connection>);
static_assert(db::mongo_abort_transaction_connection<mongo_connection>);
static_assert(db::mongo_full_connection<mongo_connection>);
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


//==============================================================================
// HELPERS
//==============================================================================

// g_uri, g_standalone, g_database -- the deployment, the standalone server
// ("" for none), and the run's database
std::string g_uri;
std::string g_standalone;
std::string g_database;

/*
contains
  Whether text holds a part; says what it held where it does not.
*/
bool contains(
    const std::string& _text,
    const std::string& _part
)
{
    const bool found = (_text.find(_part) != std::string::npos);

    // what was there instead
    if (!found)
    {
        std::printf("    \"%s\" lacks \"%s\"\n", _text.c_str(), _part.c_str());
    }

    return found;
}

/*
throws_client
  Whether a call throws Exception with a failure of the driver's, libbson's
or the layer's, packed above the server's codes.
*/
template<typename Exception,
         typename Function>
bool throws_client(
    Function&& _function
)
{
    try
    {
        _function();
    }
    catch (const Exception& e)
    {
        return db::face::mongodb::result_is_client(e.error_code());
    }

    return false;
}

/*
config
  The deployment's configuration, in a database of the run's: the one given,
or the run's own.
*/
db::mongo_connect_config config(
    const std::string& _database = ""
)
{
    db::mongo_connect_config made(g_uri);

    made.base.database = (_database.empty()) ? g_database
                                             : _database;

    return made;
}

/*
first_text
  The first row's first column of a result set, as text; "" for none.
*/
std::string first_text(
    db::buffered_result_set& _rows
)
{
    return (_rows.next())
               ? _rows.get_string(static_cast<std::size_t>(0))
                     .value_or(std::string())
               : std::string();
}

/*
all_text
  Every row's first column, joined by newlines.
*/
std::string all_text(
    db::buffered_result_set& _rows
)
{
    std::string text;

    // each row's document
    while (_rows.next())
    {
        text += _rows.get_string(static_cast<std::size_t>(0))
                    .value_or(std::string());
        text += "\n";
    }

    return text;
}


//==============================================================================
// GROUPS
//==============================================================================

/*
d_tests_mongodb_core_connecting
  How a configuration connects.
  Tests the following:
  - a connection string alone, its database the configuration's after;
    base's database replaces the string's
  - a host and a port without a string; a wrong password and a server not
    there are connection_exception, the connection then in error
  - a key apart from the certificate is refused, D_DB_UNSUPPORTED
  - text holding a NUL is refused before the layer sees it
  - reconnect() connects again; disconnect() closes
  - adopt: a NULL client is refused; another connection's client, not
    owned, works and is left to its maker
*/
void d_tests_mongodb_core_connecting(void)
{
    std::printf("connecting\n");

    // the string, and base's database over it
    {
        mongo_connection conn(config());

        conn.connect();
        D_TESTS_DB_CORE_CHECK(conn.is_connected());
        D_TESTS_DB_CORE_CHECK(conn.get_database_name() == g_database);
        D_TESTS_DB_CORE_CHECK(conn.ping());
        D_TESTS_DB_CORE_CHECK(!conn.get_server_version().empty());
        D_TESTS_DB_CORE_CHECK(conn.get_native_handle() != nullptr);
        D_TESTS_DB_CORE_CHECK(conn.get_native_client() != nullptr);
        conn.reconnect();
        D_TESTS_DB_CORE_CHECK(conn.is_connected());
        conn.disconnect();
        D_TESTS_DB_CORE_CHECK( (!conn.is_connected()) &&
                               (conn.get_native_client() == nullptr) &&
                               (conn.server_version_number() == 0ul) );
    }

    // failures: a wrong password, a server not there
    {
        db::mongo_connect_config wrong = config();

        wrong.base.username                = "dtm_no_such_user";
        wrong.base.password                = "certainly not the password";
        wrong.server_selection_timeout_ms  = 2000;

        mongo_connection refused(wrong);

        D_TESTS_DB_CORE_THROWS_ANY(refused.connect(),
                                   db::connection_exception);
        D_TESTS_DB_CORE_CHECK(refused.get_state() ==
                              db::DConnectionState::error);
        D_TESTS_DB_CORE_CHECK(refused.get_last_error_code() != 0);
    }

    {
        db::mongo_connect_config nowhere("mongodb://127.0.0.1:1/");

        nowhere.server_selection_timeout_ms = 300;

        mongo_connection absent(nowhere);
        int              code = 0;

        try
        {
            absent.connect();
        }
        catch (const db::connection_exception& e)
        {
            code = e.error_code();
        }

        D_TESTS_DB_CORE_CHECK(db::mongodb_layer::connection_lost(code));
        D_TESTS_DB_CORE_CHECK(contains(absent.get_last_error(),
                                       "MongoDB connect"));
    }

    // a host and a port, no string: refused for the wrong port alike
    {
        db::mongo_connect_config parts;

        parts.base.host                   = "127.0.0.1";
        parts.base.port                   = 1;
        parts.server_selection_timeout_ms = 300;

        mongo_connection absent(parts);

        D_TESTS_DB_CORE_THROWS_ANY(absent.connect(),
                                   db::connection_exception);
    }

    // refusals before the layer: a key apart, a NUL
    {
        db::mongo_connect_config keyed = config();

        keyed.base.ssl_cert = "client.pem";
        keyed.base.ssl_key  = "client.key";

        mongo_connection refused(keyed);

        D_TESTS_DB_CORE_THROWS(refused.connect(),
                               db::connection_exception,
                               D_DB_UNSUPPORTED);
    }

    {
        db::mongo_connect_config nul = config();

        nul.app_name = std::string("dt\0m", 4);

        mongo_connection refused(nul);

        D_TESTS_DB_CORE_THROWS(refused.connect(),
                               db::connection_exception,
                               D_DB_INVALID_ARGUMENT);
    }

    // adopting a client
    {
        mongo_connection maker(config());
        mongo_connection taker(config());

        maker.connect();
        D_TESTS_DB_CORE_THROWS(taker.adopt(nullptr, false),
                               db::connection_exception,
                               D_DB_INVALID_ARGUMENT);
        taker.adopt(maker.get_native_client(), false);
        D_TESTS_DB_CORE_CHECK(taker.is_connected());
        D_TESTS_DB_CORE_CHECK(taker.ping());
        taker.use_database(g_database);
        (void)taker.insert_one("adopted", "{\"by\": \"taker\"}");
        taker.disconnect();
        D_TESTS_DB_CORE_CHECK(maker.count_documents("adopted") == 1);
        maker.drop_collection("adopted");
    }

    return;
}

/*
d_tests_mongodb_core_commands
  execute_query, execute_update, execute, execute_scalar,
execute_single_row and run_command, with a command document.
  Tests the following:
  - a command without a cursor is its reply, one row, of field_type::json,
    named "document"; one with a cursor is its documents
  - execute_update counts what a command wrote, which get_affected_rows()
    then answers
  - text that is not JSON is libbson's, packed; a command of no such name is
    the server's 59, and its name; both query_exception
  - a NUL is refused, D_DB_INVALID_ARGUMENT
*/
void d_tests_mongodb_core_commands(
    mongo_connection& _db
)
{
    std::printf("commands\n");

    std::unique_ptr<db::buffered_result_set> reply =
        _db.execute_query("{\"ping\": 1}");

    D_TESTS_DB_CORE_CHECK( (reply->column_count() == 1u) &&
                           (reply->column_name(0u) == "document") &&
                           (reply->column_type(0u) == db::field_type::json) );
    D_TESTS_DB_CORE_CHECK(contains(first_text(*reply), "\"ok\""));

    // writes, counted
    D_TESTS_DB_CORE_CHECK(_db.execute_update(
                              "{\"insert\": \"cmd\", \"documents\": "
                              "[{\"_id\": 1}, {\"_id\": 2}]}") == 2);
    D_TESTS_DB_CORE_CHECK(_db.get_affected_rows() == 2);
    D_TESTS_DB_CORE_CHECK(_db.execute("{\"update\": \"cmd\", \"updates\": "
                                      "[{\"q\": {\"_id\": 1}, \"u\": "
                                      "{\"$set\": {\"n\": 5}}}]}"));
    D_TESTS_DB_CORE_CHECK(_db.get_affected_rows() == 1);

    // a find's documents, and the first of them
    std::unique_ptr<db::buffered_result_set> found =
        _db.execute_query("{\"find\": \"cmd\", \"sort\": {\"_id\": 1}}");

    D_TESTS_DB_CORE_CHECK(found->row_count() == 2);

    const std::optional<db::value> scalar =
        _db.execute_scalar("{\"find\": \"cmd\", \"filter\": {\"_id\": 1}}");

    D_TESTS_DB_CORE_CHECK( (scalar.has_value()) &&
                           (contains(db::value_to_string(*scalar),
                                     "\"n\"")) );
    D_TESTS_DB_CORE_CHECK(_db.execute_single_row(
                              "{\"find\": \"cmd\", \"filter\": "
                              "{\"_id\": 3}}").has_value() == false);

    // run_command's reply, on another database
    D_TESTS_DB_CORE_CHECK(contains(_db.run_command("{\"buildInfo\": 1}",
                                                   "admin"),
                                   "\"version\""));

    // failures
    int bad_json = 0;

    try
    {
        (void)_db.execute_query("{\"ping\": 1");
    }
    catch (const db::query_exception& e)
    {
        bad_json = e.error_code();
    }

    D_TESTS_DB_CORE_CHECK( (db::face::mongodb::result_is_client(bad_json)) &&
                           (db::face::mongodb::result_domain(bad_json) >=
                            D_MONGODB_DOMAIN_BSON) );
    D_TESTS_DB_CORE_THROWS(_db.execute_update("{\"dtmNoSuchCommand\": 1}"),
                           db::query_exception,
                           D_MONGODB_ERROR_COMMAND_NOT_FOUND);
    D_TESTS_DB_CORE_CHECK(_db.get_error_name() == "CommandNotFound");
    D_TESTS_DB_CORE_CHECK(contains(_db.get_last_error(),
                                   "server 59, CommandNotFound"));
    D_TESTS_DB_CORE_THROWS(_db.execute_query(std::string("{\"ping\":\0 1}",
                                                         12)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS(_db.run_command(std::string("{\0}", 3)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    _db.drop_collection("cmd");

    return;
}

/*
d_tests_mongodb_core_documents
  The document members.
  Tests the following:
  - insert_one hands back the _id, an ObjectId the layer made or the
    document's own; insert_many counts, ordered or not
  - get_last_insert_id is an integer _id, D_DB_MISMATCH for an ObjectId,
    which get_last_inserted_id gives as JSON
  - find with a filter and options; find_one and none; find_keyed's two
    columns
  - update_one and update_many, replace_one, delete_one and delete_many,
    their counts, and get_affected_rows after each
  - count_documents, estimated_document_count, distinct, aggregate on a
    collection
  - the find-and-modify members, and none matched
  - bulk_write's counts, and its first failure's code with every error in
    the reply; client_bulk_write from 8.0, D_DB_UNSUPPORTED before
  - a duplicate key: 11000, DuplicateKey, its reply naming the key
  - NUL refusals
*/
void d_tests_mongodb_core_documents(
    mongo_connection& _db
)
{
    std::printf("documents\n");

    const std::string made = _db.insert_one("docs", "{\"n\": 1}");

    D_TESTS_DB_CORE_CHECK(contains(made, "$oid"));
    D_TESTS_DB_CORE_CHECK( (_db.get_last_inserted_id().has_value()) &&
                           (*_db.get_last_inserted_id() == made) );
    D_TESTS_DB_CORE_THROWS(_db.get_last_insert_id(),
                           db::query_exception,
                           D_DB_MISMATCH);
    D_TESTS_DB_CORE_CHECK(contains(_db.insert_one("docs",
                                                  "{\"_id\": 42, \"n\": 2}"),
                                   "42"));
    D_TESTS_DB_CORE_CHECK(_db.get_last_insert_id() == 42);
    D_TESTS_DB_CORE_CHECK(_db.get_affected_rows() == 1);
    D_TESTS_DB_CORE_CHECK(_db.insert_many("docs",
                                          "[{\"n\": 3}, {\"n\": 4}, "
                                          "{\"n\": 5}]") == 3);
    D_TESTS_DB_CORE_CHECK(_db.get_affected_rows() == 3);

    // a duplicate, unordered: the others go in, the first failure thrown
    D_TESTS_DB_CORE_THROWS(_db.insert_many("docs",
                                           "[{\"_id\": 42}, {\"n\": 6}]",
                                           false),
                           db::query_exception,
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_DB_CORE_CHECK(_db.get_affected_rows() == 1);
    D_TESTS_DB_CORE_CHECK(_db.get_error_name() == "DuplicateKey");
    D_TESTS_DB_CORE_CHECK( (_db.get_error_reply().has_value()) &&
                           (contains(*_db.get_error_reply(), "writeErrors")) );

    // finding
    std::unique_ptr<db::buffered_result_set> found =
        _db.find("docs", "{\"n\": {\"$gte\": 3}}",
                 "{\"sort\": {\"n\": -1}, \"projection\": {\"_id\": 0}}");

    D_TESTS_DB_CORE_CHECK(found->row_count() == 4);
    D_TESTS_DB_CORE_CHECK(contains(first_text(*found), "6"));
    D_TESTS_DB_CORE_CHECK(_db.find("docs")->row_count() == 6);
    D_TESTS_DB_CORE_CHECK(_db.find_one("docs", "{\"_id\": 42}").has_value());
    D_TESTS_DB_CORE_CHECK(!_db.find_one("docs", "{\"_id\": 43}").has_value());

    std::unique_ptr<db::buffered_result_set> keyed =
        _db.find_keyed("docs", "{\"_id\": 42}");

    D_TESTS_DB_CORE_CHECK( (keyed->column_count() == 2u) &&
                           (keyed->column_name(0u) == "_id") &&
                           (keyed->next()) &&
                           (keyed->get_string(static_cast<std::size_t>(0))
                                == std::string("{ \"_id\" : { \"$numberInt\" "
                                               ": \"42\" } }")) );

    // changing
    db::mongo_write_result written =
        _db.update_one("docs", "{\"_id\": 42}", "{\"$set\": {\"n\": 20}}");

    D_TESTS_DB_CORE_CHECK( (written.matched == 1) &&
                           (written.modified == 1) );
    written = _db.update_many("docs", "{\"n\": {\"$lt\": 6}}",
                              "{\"$inc\": {\"n\": 100}}");
    D_TESTS_DB_CORE_CHECK( (written.matched == 4) &&
                           (written.modified == 4) &&
                           (_db.get_affected_rows() == 4) );
    written = _db.update_one("docs", "{\"_id\": 77}",
                             "{\"$set\": {\"n\": 77}}",
                             "{\"upsert\": true}");
    D_TESTS_DB_CORE_CHECK( (written.upserted == 1) &&
                           (_db.get_last_insert_id() == 77) );
    written = _db.replace_one("docs", "{\"_id\": 77}", "{\"n\": 78}");
    D_TESTS_DB_CORE_CHECK(written.modified == 1);
    D_TESTS_DB_CORE_CHECK(throws_client<db::query_exception>(
        [&_db]()
        {
            (void)_db.replace_one("docs", "{\"_id\": 77}",
                                  "{\"$set\": {\"n\": 1}}");
        }));
    D_TESTS_DB_CORE_CHECK(_db.delete_one("docs", "{\"_id\": 77}") == 1);
    D_TESTS_DB_CORE_CHECK(_db.delete_one("docs", "{\"_id\": 77}") == 0);

    // counting, distinct values, a pipeline
    D_TESTS_DB_CORE_CHECK(_db.count_documents("docs") == 6);
    D_TESTS_DB_CORE_CHECK(_db.count_documents("docs",
                                              "{\"n\": {\"$gt\": 100}}") ==
                          4);
    D_TESTS_DB_CORE_CHECK(_db.count_documents("nothing") == 0);
    D_TESTS_DB_CORE_CHECK(_db.estimated_document_count("docs") == 6);
    D_TESTS_DB_CORE_CHECK(_db.distinct("docs", "n")->row_count() == 6);
    D_TESTS_DB_CORE_CHECK(contains(
        first_text(*_db.aggregate("docs",
                                  "[{\"$group\": {\"_id\": null, "
                                  "\"total\": {\"$sum\": 1}}}]")),
        "6"));

    // finding and modifying
    const std::optional<std::string> before =
        _db.find_one_and_update("docs", "{\"_id\": 42}",
                                "{\"$set\": {\"n\": 21}}");

    D_TESTS_DB_CORE_CHECK( (before.has_value()) &&
                           (contains(*before, "20")) );
    D_TESTS_DB_CORE_CHECK(contains(
        _db.find_one_and_replace("docs", "{\"_id\": 42}", "{\"n\": 22}",
                                 "{\"returnDocument\": \"after\"}")
            .value_or(""),
        "22"));
    D_TESTS_DB_CORE_CHECK(!_db.find_one_and_delete("docs", "{\"_id\": 99}")
                               .has_value());
    D_TESTS_DB_CORE_CHECK(_db.find_one_and_delete("docs", "{\"_id\": 42}")
                              .has_value());

    // many writes at once
    written = _db.bulk_write("docs",
                             "[{\"insertOne\": {\"document\": "
                             "{\"_id\": 500}}}, {\"deleteMany\": "
                             "{\"filter\": {\"n\": {\"$gt\": 100}}}}]");
    D_TESTS_DB_CORE_CHECK( (written.inserted == 1) &&
                           (written.deleted == 4) );
    D_TESTS_DB_CORE_THROWS(_db.bulk_write("docs",
                                          "[{\"insertOne\": {\"document\": "
                                          "{\"_id\": 500}}}]"),
                           db::query_exception,
                           D_MONGODB_ERROR_DUPLICATE_KEY);

    // across collections, from 8.0
    const std::string models = "[{\"insertOne\": {\"namespace\": \"" +
                               g_database + ".other\", \"document\": "
                               "{\"_id\": 1}}}]";

    if (_db.supports_bulk_write())
    {
        D_TESTS_DB_CORE_CHECK(_db.client_bulk_write(models).inserted == 1);
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(_db.client_bulk_write(models),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }

    // refusals
    D_TESTS_DB_CORE_THROWS(_db.insert_one("docs",
                                          std::string("{\"a\": \"\0\"}", 11)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS(_db.find(std::string("do\0cs", 5)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_CHECK(throws_client<db::query_exception>(
        [&_db]() { (void)_db.update_one("docs", "{}", "{\"n\": 1}"); }));
    _db.drop_collection("docs");
    _db.drop_collection("other");

    return;
}

/*
d_tests_mongodb_core_cursors
  Cursors, and how they live with the connection.
  Tests the following:
  - find_cursor's pages, a short one at the end, is_done then; a fetch of 0
    is D_DB_INVALID_ARGUMENT; aggregate_cursor and command_cursor
  - a closed cursor refuses a read, D_DB_INVALID_ARGUMENT
  - disconnecting closes the cursors the connection handed out, and one that
    outlives its connection object refuses, without touching the handle
  - a cursor opened in a transaction, read after it commits, is
    D_MONGODB_ERROR_TRANSACTION_CURSOR
*/
void d_tests_mongodb_core_cursors(
    mongo_connection& _db
)
{
    std::printf("cursors\n");

    std::string documents = "[";

    // five documents
    for (int i = 0; i < 5; ++i)
    {
        documents += ( (i > 0) ? ", " : "" ) +
                     std::string("{\"_id\": ") + std::to_string(i) + "}";
    }

    documents += "]";
    (void)_db.insert_many("cur", documents);

    std::unique_ptr<db::mongo_cursor> cursor =
        _db.find_cursor("cur", "", "{\"sort\": {\"_id\": 1}, "
                                   "\"batchSize\": 2}");

    D_TESTS_DB_CORE_CHECK( (cursor->is_open()) &&
                           (!cursor->is_done()) );
    D_TESTS_DB_CORE_CHECK(cursor->fetch(3u)->row_count() == 3);
    D_TESTS_DB_CORE_CHECK(cursor->fetch(3u)->row_count() == 2);
    D_TESTS_DB_CORE_CHECK(cursor->is_done());
    D_TESTS_DB_CORE_CHECK(cursor->fetch(3u)->row_count() == 0);
    cursor->close();
    D_TESTS_DB_CORE_THROWS(cursor->fetch(1u),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);

    cursor = _db.find_cursor("cur");
    D_TESTS_DB_CORE_THROWS(cursor->fetch(0u),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    cursor = _db.aggregate_cursor("cur", "[{\"$match\": {\"_id\": "
                                         "{\"$gte\": 3}}}]");
    D_TESTS_DB_CORE_CHECK(cursor->fetch(10u)->row_count() == 2);
    cursor = _db.command_cursor("{\"listCollections\": 1, "
                                "\"nameOnly\": true}");
    D_TESTS_DB_CORE_CHECK(cursor->fetch(100u)->row_count() >= 1);

    // the connection closes what it handed out
    {
        mongo_connection other(config());

        other.connect();
        cursor = other.find_cursor("cur");
        other.disconnect();
        D_TESTS_DB_CORE_CHECK(!cursor->is_open());
        D_TESTS_DB_CORE_THROWS(cursor->fetch(1u),
                               db::query_exception,
                               D_DB_INVALID_ARGUMENT);
        other.connect();
        cursor = other.find_cursor("cur");
    }

    D_TESTS_DB_CORE_CHECK(!cursor->is_open());
    D_TESTS_DB_CORE_THROWS(cursor->fetch(1u),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    cursor.reset();

    // a transaction's cursor, read after it ends
    if (_db.supports_transactions())
    {
        _db.begin_transaction();
        cursor = _db.find_cursor("cur", "", "{\"batchSize\": 1}");
        D_TESTS_DB_CORE_CHECK(cursor->fetch(1u)->row_count() == 1);
        _db.commit();
        D_TESTS_DB_CORE_THROWS(cursor->fetch(1u),
                               db::query_exception,
                               D_MONGODB_ERROR_TRANSACTION_CURSOR);
        cursor.reset();
        D_TESTS_DB_CORE_CHECK(_db.count_documents("cur") == 5);
    }

    _db.drop_collection("cur");

    return;
}

/*
d_tests_mongodb_core_streams
  Change streams.
  Tests the following:
  - a collection's stream sees an insert, with its resume token after;
    none before the first batch
  - a stream started after the token sees what came after it alone
  - the database's and the deployment's streams open
  - a closed stream refuses; disconnecting closes one
*/
void d_tests_mongodb_core_streams(
    mongo_connection& _db
)
{
    std::printf("change streams\n");

    // a deployment without them
    if (!_db.supports_change_streams())
    {
        D_TESTS_DB_CORE_THROWS(_db.watch_collection("events"),
                               db::query_exception,
                               D_DB_UNSUPPORTED);

        return;
    }

    _db.create_collection("events");

    std::unique_ptr<db::mongo_change_stream> stream =
        _db.watch_collection("events", "", "{\"maxAwaitTimeMS\": 200}");

    (void)_db.insert_one("events", "{\"_id\": 1}");

    std::string seen;

    // the event, within a few waits
    for (int tries = 0; ( (tries < 20) &&
                          (seen.empty()) ); ++tries)
    {
        seen = all_text(*stream->fetch(10u));
    }

    D_TESTS_DB_CORE_CHECK(contains(seen, "\"insert\""));

    const std::optional<std::string> token = stream->resume_token();

    D_TESTS_DB_CORE_CHECK( (token.has_value()) &&
                           (contains(*token, "_data")) );

    (void)_db.insert_one("events", "{\"_id\": 2}");

    std::unique_ptr<db::mongo_change_stream> after =
        _db.watch_collection("events", "",
                             "{\"maxAwaitTimeMS\": 200, \"resumeAfter\": " +
                                 token.value_or("{}") + "}");

    seen.clear();

    for (int tries = 0; ( (tries < 20) &&
                          (seen.empty()) ); ++tries)
    {
        seen = all_text(*after->fetch(10u));
    }

    D_TESTS_DB_CORE_CHECK( (contains(seen, "2")) &&
                           (seen.find("\"_id\" : { \"$numberInt\" : "
                                      "\"1\" }") == std::string::npos) );

    // the other two, open; closed ones refuse
    D_TESTS_DB_CORE_CHECK(_db.watch_database()->is_open());
    D_TESTS_DB_CORE_CHECK(_db.watch_deployment("[{\"$match\": "
                                               "{\"ns.db\": \"" +
                                               g_database + "\"}}]")
                              ->is_open());
    stream->close();
    D_TESTS_DB_CORE_THROWS(stream->fetch(1u),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS(stream->resume_token(),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);

    {
        mongo_connection other(config());

        other.connect();
        stream = other.watch_collection("events");
    }

    D_TESTS_DB_CORE_CHECK(!stream->is_open());
    _db.drop_collection("events");

    return;
}

/*
d_tests_mongodb_core_schema
  Databases, collections, views and indexes.
  Tests the following:
  - has_database, list_databases, drop_database; use_database, which the
    configuration then names
  - create_collection, its options and if_not_exists; has_collection,
    list_collections, get_collection_info, rename_collection with and
    without drop_target, drop_collection with and without if_exists
  - create_view, has_view, list_views, drop_view
  - create_index, unique; has_index, list_indexes, describe_indexes,
    drop_index; an index of the name there is D_DB_EXISTS without
    if_not_exists
*/
void d_tests_mongodb_core_schema(
    mongo_connection& _db
)
{
    std::printf("schema\n");

    // databases
    const std::string elsewhere = g_database + "_b";

    _db.use_database(elsewhere);
    D_TESTS_DB_CORE_CHECK(_db.get_database_name() == elsewhere);
    (void)_db.insert_one("c", "{}");
    D_TESTS_DB_CORE_CHECK(_db.has_database(elsewhere));

    const std::vector<std::string> databases = _db.list_databases();

    D_TESTS_DB_CORE_CHECK(std::find(databases.begin(),
                                    databases.end(),
                                    elsewhere) != databases.end());
    _db.use_database(g_database);
    _db.drop_database(elsewhere);
    D_TESTS_DB_CORE_CHECK(!_db.has_database(elsewhere));
    _db.drop_database(elsewhere);
    D_TESTS_DB_CORE_THROWS(_db.use_database("no/such"),
                           db::query_exception,
                           D_MONGODB_ERROR_INVALID_NAMESPACE);

    // collections
    _db.create_collection("capped", "{\"capped\": true, \"size\": 4096}");
    _db.create_collection("capped");
    D_TESTS_DB_CORE_THROWS(_db.create_collection("capped", "", false),
                           db::query_exception,
                           D_MONGODB_ERROR_NAMESPACE_EXISTS);
    D_TESTS_DB_CORE_CHECK(_db.has_collection("capped"));
    D_TESTS_DB_CORE_CHECK(contains(_db.get_collection_info("capped"),
                                   "\"capped\" : true"));
    _db.create_collection("other");
    _db.rename_collection("capped", "renamed");
    D_TESTS_DB_CORE_THROWS(_db.rename_collection("renamed", "other"),
                           db::query_exception,
                           D_MONGODB_ERROR_NAMESPACE_EXISTS);
    _db.rename_collection("renamed", "other", true);
    D_TESTS_DB_CORE_CHECK( (_db.has_collection("other")) &&
                           (!_db.has_collection("renamed")) );

    // views
    _db.create_view("view", "other", "[{\"$match\": {}}]");
    D_TESTS_DB_CORE_CHECK( (_db.has_view("view")) &&
                           (!_db.has_collection("view")) );
    D_TESTS_DB_CORE_CHECK(_db.list_views() ==
                          std::vector<std::string>{ "view" });

    const std::vector<std::string> collections = _db.list_collections();

    D_TESTS_DB_CORE_CHECK( (std::find(collections.begin(),
                                      collections.end(),
                                      "other") != collections.end()) &&
                           (std::find(collections.begin(),
                                      collections.end(),
                                      "view") == collections.end()) );
    _db.drop_view("view");
    D_TESTS_DB_CORE_CHECK(!_db.has_view("view"));

    // indexes
    _db.create_index("indexed", "by_n", "{\"n\": 1}", true);
    _db.create_index("indexed", "by_n", "{\"m\": 1}");
    D_TESTS_DB_CORE_THROWS(_db.create_index("indexed", "by_n", "{\"n\": 1}",
                                            false, "", false),
                           db::query_exception,
                           D_DB_EXISTS);
    D_TESTS_DB_CORE_CHECK(_db.has_index("indexed", "by_n"));
    D_TESTS_DB_CORE_CHECK(_db.list_indexes("indexed") ==
                          (std::vector<std::string>{ "_id_", "by_n" }));
    D_TESTS_DB_CORE_CHECK(contains(all_text(*_db.describe_indexes("indexed")),
                                   "\"unique\" : true"));
    (void)_db.insert_one("indexed", "{\"n\": 1}");
    D_TESTS_DB_CORE_THROWS(_db.insert_one("indexed", "{\"n\": 1}"),
                           db::query_exception,
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    _db.drop_index("indexed", "by_n");
    _db.drop_index("indexed", "by_n");
    D_TESTS_DB_CORE_THROWS(_db.drop_index("indexed", "by_n", false),
                           db::query_exception,
                           D_MONGODB_ERROR_INDEX_NOT_FOUND);

    // dropping
    _db.drop_collection("other");
    _db.drop_collection("indexed");
    _db.drop_collection("other");
    D_TESTS_DB_CORE_THROWS(_db.drop_collection("other", false),
                           db::query_exception,
                           D_MONGODB_ERROR_NAMESPACE_NOT_FOUND);

    return;
}

/*
d_tests_mongodb_core_transactions
  Transactions, their guard, and their failures.
  Tests the following:
  - begin_transaction, in_transaction, commit, rollback, what each left
  - begin_transaction with options; the guard, its rollback and commit
  - one already open is transaction_exception, the driver's code, the open
    one left as it was
  - a write that fails fails the transaction on the server:
    in_failed_transaction, until the rollback
  - get_auto_commit: true outside, false inside
*/
void d_tests_mongodb_core_transactions(
    mongo_connection& _db
)
{
    std::printf("transactions\n");

    // a deployment without them
    if (!_db.supports_transactions())
    {
        D_TESTS_DB_CORE_THROWS(_db.begin_transaction(),
                               db::transaction_exception,
                               D_DB_UNSUPPORTED);

        return;
    }

    _db.create_collection("tx");
    D_TESTS_DB_CORE_CHECK( (!_db.in_transaction()) &&
                           (_db.get_auto_commit()) );
    _db.begin_transaction();
    D_TESTS_DB_CORE_CHECK( (_db.in_transaction()) &&
                           (!_db.get_auto_commit()) );
    (void)_db.insert_one("tx", "{\"_id\": 1}");
    _db.rollback();
    D_TESTS_DB_CORE_CHECK(_db.count_documents("tx") == 0);
    _db.begin_transaction("{\"writeConcern\": {\"w\": \"majority\"}, "
                          "\"maxCommitTimeMS\": 5000}");
    (void)_db.insert_one("tx", "{\"_id\": 2}");
    D_TESTS_DB_CORE_CHECK(throws_client<db::transaction_exception>(
        [&_db]() { _db.begin_transaction(); }));
    D_TESTS_DB_CORE_CHECK(_db.in_transaction());
    _db.commit();
    D_TESTS_DB_CORE_CHECK(_db.count_documents("tx") == 1);

    // the guard
    {
        db::transaction<mongo_connection> work(_db);

        (void)_db.insert_one("tx", "{\"_id\": 3}");
    }

    D_TESTS_DB_CORE_CHECK(_db.count_documents("tx") == 1);

    {
        db::transaction<mongo_connection> work(_db);

        (void)_db.insert_one("tx", "{\"_id\": 4}");
        work.commit();
    }

    D_TESTS_DB_CORE_CHECK(_db.count_documents("tx") == 2);

    // a failure fails the transaction
    _db.begin_transaction();
    D_TESTS_DB_CORE_THROWS(_db.insert_one("tx", "{\"_id\": 2}"),
                           db::query_exception,
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_DB_CORE_CHECK(_db.in_failed_transaction());
    _db.rollback();
    D_TESTS_DB_CORE_CHECK( (!_db.in_transaction()) &&
                           (!_db.in_failed_transaction()) );
    _db.drop_collection("tx");

    return;
}

/*
d_tests_mongodb_core_gridfs
  GridFS.
  Tests the following:
  - bytes uploaded and downloaded, NULs among them; an empty file
  - a file of the client's uploaded and downloaded; gridfs_find_id, the
    newest of a name, and none; gridfs_find
  - deleting, and D_DB_NOT_FOUND after; a bucket of its own
*/
void d_tests_mongodb_core_gridfs(
    mongo_connection& _db
)
{
    std::printf("gridfs\n");

    const std::vector<re_std::uint8_t> bytes = { 0u, 1u, 2u, 0u, 255u };
    const std::string                  id    =
        _db.gridfs_upload("a.bin", bytes, "{\"kind\": \"test\"}");

    D_TESTS_DB_CORE_CHECK(_db.gridfs_download(id) == bytes);
    D_TESTS_DB_CORE_CHECK(_db.gridfs_find_id("a.bin").value_or("") == id);
    D_TESTS_DB_CORE_CHECK(!_db.gridfs_find_id("none.bin").has_value());
    D_TESTS_DB_CORE_CHECK(contains(all_text(*_db.gridfs_find(
                                       "{\"filename\": \"a.bin\"}")),
                                   "\"kind\" : \"test\""));

    const std::string empty = _db.gridfs_upload("empty.bin", {}, "", "own");

    D_TESTS_DB_CORE_CHECK(_db.gridfs_download(empty, "own").empty());
    D_TESTS_DB_CORE_THROWS(_db.gridfs_download(empty),
                           db::query_exception,
                           D_DB_NOT_FOUND);

    // files of the client's
    const std::string in  = "t_mongodb_core_in.bin";
    const std::string out = "t_mongodb_core_out.bin";
    std::FILE* const  file = std::fopen(in.c_str(), "wb");

    if (file)
    {
        (void)std::fwrite("file", 1u, 4u, file);
        (void)std::fclose(file);

        const std::string from = _db.gridfs_upload_file("f.txt", in);

        _db.gridfs_download_file(from, out);
        D_TESTS_DB_CORE_CHECK(_db.gridfs_download(from).size() == 4u);
        (void)std::remove(in.c_str());
        (void)std::remove(out.c_str());
    }

    // deleting
    _db.gridfs_delete(id);
    D_TESTS_DB_CORE_THROWS(_db.gridfs_delete(id),
                           db::query_exception,
                           D_DB_NOT_FOUND);
    D_TESTS_DB_CORE_THROWS(_db.gridfs_download(id),
                           db::query_exception,
                           D_DB_NOT_FOUND);

    return;
}

/*
d_tests_mongodb_core_upkeep
  Sizes, compact and validate.
  Tests the following:
  - a collection's size and the database's, more than 0 once written
  - compact, forced on a primary as the layer forces it
  - validate's verdict and report, and the full checks
*/
void d_tests_mongodb_core_upkeep(
    mongo_connection& _db
)
{
    std::printf("upkeep\n");

    (void)_db.insert_one("kept", "{\"x\": \"some text to take room\"}");
    D_TESTS_DB_CORE_CHECK(_db.get_collection_size("kept") > 0);
    D_TESTS_DB_CORE_CHECK(_db.get_database_size() > 0);
    _db.compact("kept");

    const db::mongo_validation checked = _db.validate("kept");

    D_TESTS_DB_CORE_CHECK( (checked.valid) &&
                           (contains(checked.report, "\"valid\" : true")) );
    D_TESTS_DB_CORE_CHECK(_db.validate("kept", true).valid);
    _db.drop_collection("kept");

    return;
}

/*
d_tests_mongodb_core_settings
  The connection's concerns, preference, format and time limit.
  Tests the following:
  - each concern and the preference, set as JSON and as its enum, read back;
    "" the deployment's own
  - a w of 0 is refused, D_DB_INVALID_ARGUMENT
  - relaxed and BSON formats change how documents come back; canonical
    again after
  - a time limit is read back; negative refused
*/
void d_tests_mongodb_core_settings(
    mongo_connection& _db
)
{
    std::printf("settings\n");

    _db.set_read_concern(db::mongo_read_concern::majority);
    D_TESTS_DB_CORE_CHECK(contains(_db.get_read_concern(), "majority"));
    _db.set_read_concern("");
    D_TESTS_DB_CORE_CHECK(_db.get_read_concern() == "{ }");
    _db.set_write_concern(db::mongo_write_concern::majority);
    D_TESTS_DB_CORE_CHECK(contains(_db.get_write_concern(), "majority"));
    _db.set_write_concern("{\"w\": 1, \"j\": true}");
    D_TESTS_DB_CORE_CHECK(contains(_db.get_write_concern(), "\"j\" : true"));
    D_TESTS_DB_CORE_THROWS(_db.set_write_concern("{\"w\": 0}"),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    _db.set_write_concern("");
    _db.set_read_preference(db::mongo_read_preference::primary_preferred);
    D_TESTS_DB_CORE_CHECK(contains(_db.get_read_preference(),
                                   "primaryPreferred"));
    _db.set_read_preference("");

    // the formats
    (void)_db.insert_one("fmt", "{\"_id\": 1, \"n\": {\"$numberLong\": "
                                "\"5\"}}");
    D_TESTS_DB_CORE_CHECK(_db.get_document_format() ==
                          db::mongo_document_format::canonical);
    D_TESTS_DB_CORE_CHECK(contains(_db.find_one("fmt").value_or(""),
                                   "$numberLong"));
    _db.set_document_format(db::mongo_document_format::relaxed);
    D_TESTS_DB_CORE_CHECK(_db.find_one("fmt").value_or("").find("$number") ==
                          std::string::npos);
    _db.set_document_format(db::mongo_document_format::bson);

    std::unique_ptr<db::buffered_result_set> bson = _db.find("fmt");

    D_TESTS_DB_CORE_CHECK( (bson->next()) &&
                           (bson->get_binary(static_cast<std::size_t>(0))
                                .has_value()) );
    _db.set_document_format(db::mongo_document_format::canonical);

    // the time limit
    _db.set_max_time_ms(5000);
    D_TESTS_DB_CORE_CHECK(_db.get_max_time_ms() == 5000);
    _db.set_max_time_ms(0);
    D_TESTS_DB_CORE_THROWS(_db.set_max_time_ms(-1),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    _db.drop_collection("fmt");

    return;
}

/*
d_tests_mongodb_core_server
  The server, the driver and the latest failure.
  Tests the following:
  - the version's number agrees with its string; server_at_least
  - the topology is a replica set's, the set's name the string's
  - transactions and change streams supported on a replica set; the
    features by version
  - the driver's versions; the log level set and read back, -1 to 6, and
    out of range refused
  - an error's label: a write conflict's is TransientTransactionError
*/
void d_tests_mongodb_core_server(
    mongo_connection& _db
)
{
    std::printf("the server and the driver\n");

    const unsigned long number = _db.server_version_number();

    D_TESTS_DB_CORE_CHECK(number >= 40200ul);
    D_TESTS_DB_CORE_CHECK(contains(_db.get_server_version(),
                                   std::to_string(number / 10000ul) + "."));
    D_TESTS_DB_CORE_CHECK( (_db.server_at_least(4u, 2u)) &&
                           (!_db.server_at_least(99u, 0u)) );
    D_TESTS_DB_CORE_CHECK(_db.server_wire_version() >= 8);
    D_TESTS_DB_CORE_CHECK(_db.get_server_topology() ==
                          db::mongo_topology::replica_set);
    D_TESTS_DB_CORE_CHECK(!_db.get_replica_set_name().empty());
    D_TESTS_DB_CORE_CHECK( (_db.supports_transactions()) &&
                           (_db.supports_change_streams()) &&
                           (_db.supports_wildcard_indexes()) );
    D_TESTS_DB_CORE_CHECK(_db.supports_time_series() ==
                          _db.server_at_least(5u, 0u));
    D_TESTS_DB_CORE_CHECK(_db.supports_hidden_indexes() ==
                          _db.server_at_least(4u, 4u));
    D_TESTS_DB_CORE_CHECK(_db.supports_clustered_collections() ==
                          _db.server_at_least(5u, 3u));
    D_TESTS_DB_CORE_CHECK(_db.supports_pre_post_images() ==
                          _db.server_at_least(6u, 0u));
    D_TESTS_DB_CORE_CHECK(_db.supports_snapshot_reads() ==
                          _db.server_at_least(5u, 0u));
    D_TESTS_DB_CORE_CHECK( (!_db.supports_stable_api()) ||
                           (_db.server_at_least(5u, 0u)) );

    // the driver
    D_TESTS_DB_CORE_CHECK( (mongo_connection::get_client_version_number() >=
                            12100ul) &&
                           (!mongo_connection::get_client_version().empty()) &&
                           (!mongo_connection::get_bson_version().empty()) );

    const int level = mongo_connection::get_log_level();

    mongo_connection::set_log_level(-1);
    D_TESTS_DB_CORE_CHECK(mongo_connection::get_log_level() == -1);
    D_TESTS_DB_CORE_THROWS(mongo_connection::set_log_level(7),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    mongo_connection::set_log_level(level);

    // a write conflict between two transactions, labelled transient
    mongo_connection other(config());

    other.connect();
    (void)_db.insert_one("conflict", "{\"_id\": 1, \"n\": 0}");
    _db.begin_transaction();
    other.begin_transaction();
    (void)_db.update_one("conflict", "{\"_id\": 1}", "{\"$set\": {\"n\": 1}}");
    D_TESTS_DB_CORE_THROWS(other.update_one("conflict", "{\"_id\": 1}",
                                            "{\"$set\": {\"n\": 2}}"),
                           db::query_exception,
                           D_MONGODB_ERROR_WRITE_CONFLICT);
    D_TESTS_DB_CORE_CHECK( (other.has_error_label(
                                "TransientTransactionError")) &&
                           (!other.has_error_label("NoSuchLabel")) );
    other.rollback();
    _db.commit();
    _db.drop_collection("conflict");

    return;
}

/*
d_tests_mongodb_core_async
  The operations that do not block, refused (ruling 11.26.6).
  Tests the following:
  - connect_async's result throws connection_exception, D_DB_UNSUPPORTED,
    the connection as it was
  - execute_query_async, execute_update_async and execute_async throw
    query_exception, D_DB_UNSUPPORTED, nothing run; the latest failure is
    the refusal
*/
void d_tests_mongodb_core_async(
    mongo_connection& _db
)
{
    std::printf("operations that do not block\n");

    {
        mongo_connection fresh(config());
        auto             connecting = fresh.connect_async();

        connecting.start();
        D_TESTS_DB_CORE_CHECK(connecting.done());
        D_TESTS_DB_CORE_THROWS(connecting.result(),
                               db::connection_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_CHECK(fresh.get_state() ==
                              db::DConnectionState::disconnected);
    }

    auto query = _db.execute_query_async("{\"ping\": 1}");

    query.start();
    D_TESTS_DB_CORE_THROWS(query.result(),
                           db::query_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_CHECK(_db.get_last_error_code() == D_DB_UNSUPPORTED);

    auto update = _db.execute_update_async("{\"insert\": \"async\", "
                                           "\"documents\": [{}]}");

    update.start();
    D_TESTS_DB_CORE_THROWS(update.result(),
                           db::query_exception,
                           D_DB_UNSUPPORTED);

    auto run = _db.execute_async("{\"ping\": 1}");

    run.start();
    D_TESTS_DB_CORE_THROWS(run.result(),
                           db::query_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_CHECK(_db.count_documents("async") == 0);
    D_TESTS_DB_CORE_CHECK(_db.is_connected());

    return;
}

/*
d_tests_mongodb_core_closed
  A connection that is not open.
  Tests the following:
  - every kind of member throws connection_exception
  - the answers that need no round trip are their empty ones
*/
void d_tests_mongodb_core_closed(void)
{
    std::printf("not connected\n");

    mongo_connection closed(config());

    D_TESTS_DB_CORE_THROWS_ANY(closed.execute_query("{\"ping\": 1}"),
                               db::connection_exception);
    D_TESTS_DB_CORE_THROWS_ANY((void)closed.insert_one("c", "{}"),
                               db::connection_exception);
    D_TESTS_DB_CORE_THROWS_ANY((void)closed.count_documents("c"),
                               db::connection_exception);
    D_TESTS_DB_CORE_THROWS_ANY((void)closed.find_cursor("c"),
                               db::connection_exception);
    D_TESTS_DB_CORE_THROWS_ANY(closed.begin_transaction(),
                               db::connection_exception);
    D_TESTS_DB_CORE_THROWS_ANY(closed.use_database("x"),
                               db::connection_exception);
    D_TESTS_DB_CORE_CHECK( (!closed.ping()) &&
                           (!closed.in_transaction()) &&
                           (!closed.in_failed_transaction()) &&
                           (!closed.supports_transactions()) &&
                           (closed.server_version_number() == 0ul) &&
                           (closed.get_server_topology() ==
                            db::mongo_topology::unknown) &&
                           (closed.get_replica_set_name().empty()) &&
                           (closed.get_error_name().empty()) &&
                           (!closed.get_error_reply().has_value()) &&
                           (closed.get_document_format() ==
                            db::mongo_document_format::canonical) &&
                           (closed.get_max_time_ms() == 0) &&
                           (closed.get_native_client() == nullptr) &&
                           (closed.get_auto_commit()) );

    return;
}

/*
d_tests_mongodb_core_table
  mongo_table.
  Tests the following:
  - create_table and exists; refresh reads every document, each with its
    key; a filter, a sort, a page; a limit of 0 reads none
  - commit replaces a changed document, deletes a removed one, inserts a new
    one, and touches no other; the cache is read again after an insert
  - an int64 that fits 32 bits stays an int64 through a commit
  - a failed write rolls the whole commit back, the cache still dirty
  - a changed document another session deleted is D_DB_NOT_FOUND, the
    commit rolled back
  - inside the program's transaction, a commit is part of it
  - row_count_remote, rename_table, truncate_table, drop_table
  - a table moved from holds nothing and no connection; an unbound one
    refuses, connection_exception
*/
void d_tests_mongodb_core_table(
    mongo_connection& _db
)
{
    std::printf("mongo_table\n");

    db::mongo_table<> table(_db, "tbl");

    table.create_table();
    D_TESTS_DB_CORE_CHECK(table.exists());
    (void)_db.insert_many("tbl",
                          "[{\"_id\": 1, \"n\": {\"$numberLong\": \"1\"}}, "
                          "{\"_id\": 2, \"n\": 2}, {\"_id\": 3, \"n\": 3}]");
    table.set_sort("{\"_id\": 1}");
    table.refresh();
    D_TESTS_DB_CORE_CHECK( (table.size() == 3u) &&
                           (!table.is_dirty()) &&
                           (!table.is_stale()) );
    D_TESTS_DB_CORE_CHECK(table.key(0) ==
                          std::string("{ \"_id\" : { \"$numberInt\" : "
                                      "\"1\" } }"));

    // a change, a removal, an addition; and one left alone
    std::string changed = table.document(0);

    changed.replace(changed.find("\"1\" } }"), 7u, "\"1\" }, \"m\" : 9 }");
    table.set_document(0, changed);
    table.remove_document(1);
    (void)table.add_document("{\"_id\": 4, \"n\": 4}");
    D_TESTS_DB_CORE_CHECK(table.is_dirty());
    table.commit();
    D_TESTS_DB_CORE_CHECK( (!table.is_dirty()) &&
                           (table.size() == 3u) &&
                           (_db.count_documents("tbl") == 3) &&
                           (!_db.find_one("tbl", "{\"_id\": 2}")
                                 .has_value()) &&
                           (_db.count_documents("tbl", "{\"m\": 9}") == 1) );
    D_TESTS_DB_CORE_CHECK(contains(_db.find_one("tbl", "{\"_id\": 1}")
                                       .value_or(""),
                                   "\"$numberLong\" : \"1\""));
    table.commit();

    // a new document whose _id is taken: the server's 11000, and the
    // replacement before it rolled back with it
    db::mongo_table<> dup(_db, "tbl");

    dup.set_sort("{\"_id\": 1}");
    dup.refresh();
    dup.set_document(0, "{\"_id\": 1, \"m\": 10}");
    (void)dup.add_document("{\"_id\": 3}");
    D_TESTS_DB_CORE_THROWS(dup.commit(),
                           db::query_exception,
                           D_MONGODB_ERROR_DUPLICATE_KEY);
    D_TESTS_DB_CORE_CHECK( (_db.count_documents("tbl") == 3) &&
                           (_db.count_documents("tbl", "{\"m\": 10}") ==
                            0) &&
                           (dup.is_dirty()) );

    // a document another session deleted
    db::mongo_table<> stale(_db, "tbl");

    stale.set_filter("{\"_id\": 4}");
    stale.refresh();
    (void)_db.delete_one("tbl", "{\"_id\": 4}");
    stale.set_document(0, "{\"_id\": 4, \"n\": 40}");
    D_TESTS_DB_CORE_THROWS(stale.commit(),
                           db::query_exception,
                           D_DB_NOT_FOUND);
    D_TESTS_DB_CORE_CHECK(_db.count_documents("tbl") == 2);

    // inside the program's transaction
    db::mongo_table<> inner(_db, "tbl");

    inner.refresh();
    _db.begin_transaction();
    (void)inner.add_document("{\"_id\": 5}");
    inner.commit();
    D_TESTS_DB_CORE_CHECK(_db.in_transaction());
    _db.rollback();
    D_TESTS_DB_CORE_CHECK(_db.count_documents("tbl") == 2);

    // the query, and the collection
    inner.set_filter("{\"_id\": {\"$gte\": 1}}");
    inner.set_limit(1u);
    inner.set_offset(1u);
    D_TESTS_DB_CORE_CHECK(inner.is_stale());
    inner.refresh();
    D_TESTS_DB_CORE_CHECK(inner.size() == 1u);
    inner.set_limit(0u);
    inner.refresh();
    D_TESTS_DB_CORE_CHECK(inner.empty());
    D_TESTS_DB_CORE_CHECK(inner.row_count_remote() == 2);
    inner.rename_table("tbl2");
    D_TESTS_DB_CORE_CHECK( (inner.table_name() == "tbl2") &&
                           (inner.exists()) &&
                           (!_db.has_collection("tbl")) );
    inner.truncate_table();
    D_TESTS_DB_CORE_CHECK(_db.count_documents("tbl2") == 0);
    inner.drop_table();
    D_TESTS_DB_CORE_CHECK(!inner.exists());

    // moved, and unbound
    db::mongo_table<> moved(std::move(inner));

    D_TESTS_DB_CORE_CHECK( (inner.get_connection() == nullptr) &&
                           (inner.empty()) &&
                           (moved.get_connection() == &_db) );
    D_TESTS_DB_CORE_THROWS_ANY(inner.refresh(), db::connection_exception);
    D_TESTS_DB_CORE_THROWS_ANY((void)table.at(99u), std::out_of_range);
    table.drop_table();

    return;
}

/*
d_tests_mongodb_core_standalone
  A standalone server, where transactions and change streams are not.
  Tests the following:
  - its topology; transactions and change streams refused,
    D_DB_UNSUPPORTED
  - an atomic table commit is refused; set_atomic(false) writes without one
*/
void d_tests_mongodb_core_standalone(void)
{
    std::printf("standalone\n");

    // none named
    if (g_standalone.empty())
    {
        std::printf("  no standalone server named: skipped\n");

        return;
    }

    db::mongo_connect_config alone(g_standalone);

    alone.base.database = g_database;

    mongo_connection db_alone(alone);

    db_alone.connect();
    D_TESTS_DB_CORE_CHECK(db_alone.get_server_topology() ==
                          db::mongo_topology::standalone);
    D_TESTS_DB_CORE_THROWS(db_alone.begin_transaction(),
                           db::transaction_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_THROWS(db_alone.watch_collection("c"),
                           db::query_exception,
                           D_DB_UNSUPPORTED);

    db::mongo_table<> table(db_alone, "c");

    table.refresh();
    (void)table.add_document("{\"_id\": 1}");
    D_TESTS_DB_CORE_THROWS(table.commit(),
                           db::query_exception,
                           D_DB_UNSUPPORTED);
    table.set_atomic(false);
    table.commit();
    D_TESTS_DB_CORE_CHECK(db_alone.count_documents("c") == 1);
    db_alone.drop_database(g_database);

    return;
}

/*
d_tests_mongodb_core_stubs
  Over a layer built without the driver.
  Tests the following:
  - connect throws connection_exception with D_DB_UNSUPPORTED, and leaves
    the connection in error; nothing runs
  - the driver's versions are "" and 0; setting the log level is
    D_DB_UNSUPPORTED
*/
void d_tests_mongodb_core_stubs(void)
{
    std::printf("over the stubs\n");

    mongo_connection conn("mongodb://127.0.0.1/");

    D_TESTS_DB_CORE_THROWS(conn.connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_CHECK(conn.get_state() == db::DConnectionState::error);
    D_TESTS_DB_CORE_THROWS_ANY(conn.execute("{\"ping\": 1}"),
                               db::connection_exception);
    D_TESTS_DB_CORE_CHECK( (mongo_connection::get_client_version().empty()) &&
                           (mongo_connection::get_client_version_number() ==
                            0ul) );
    D_TESTS_DB_CORE_THROWS(mongo_connection::set_log_level(1),
                           db::query_exception,
                           D_DB_UNSUPPORTED);

    return;
}

}  // namespace

/*
main
  Over the driver, every group where a deployment answers, in a database of
the run's own, dropped at the end; over the stubs, what the stubs report.
*/
int main(void)
{
    const bool stubs    = (mongo_connection::get_client_version_number() == 0);
    const bool required =
        (core::setting("DJINTERP_TEST_MONGODB_REQUIRED", "0") == "1");

    g_uri        = core::setting("DJINTERP_TEST_MONGODB_URI",
                                 "mongodb://127.0.0.1:27017/"
                                 "?replicaSet=rs0");
    g_standalone = core::setting("DJINTERP_TEST_MONGODB_STANDALONE_URI", "");
    g_database   = "dtc" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count() %
        1000000000000);

    // over the stubs
    if (stubs)
    {
        d_tests_mongodb_core_stubs();

        return core::summary("built without the driver");
    }

    mongo_connection setup(config());

    // the driver's log kept quiet: the checks provoke what it reports
    mongo_connection::set_log_level(-1);

    // a deployment to reach, or nothing to run against
    try
    {
        setup.connect();
    }
    catch (const db::connection_exception& e)
    {
        std::printf("  no deployment reached (%s)\n", e.what());
        D_TESTS_DB_CORE_CHECK(!required);

        return core::summary("no deployment");
    }

    setup.drop_database(g_database);

    {
        mongo_connection db(config());

        db.connect();
        d_tests_mongodb_core_connecting();
        d_tests_mongodb_core_commands(db);
        d_tests_mongodb_core_documents(db);
        d_tests_mongodb_core_cursors(db);
        d_tests_mongodb_core_streams(db);
        d_tests_mongodb_core_schema(db);
        d_tests_mongodb_core_transactions(db);
        d_tests_mongodb_core_gridfs(db);
        d_tests_mongodb_core_upkeep(db);
        d_tests_mongodb_core_settings(db);
        d_tests_mongodb_core_server(db);
        d_tests_mongodb_core_async(db);
        d_tests_mongodb_core_closed();
        d_tests_mongodb_core_table(db);
        d_tests_mongodb_core_standalone();
    }

    setup.drop_database(g_database);

    return core::summary("MongoDB " + setup.get_server_version() +
                         ", driver " +
                         mongo_connection::get_client_version());
}

#else

/*
main
  Below C++17 core/db is absent, and there is nothing to test.
*/
int main(void)
{
    std::printf("core/db is absent below C++17\n");

    return 0;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
