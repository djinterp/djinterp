/*******************************************************************************
* djinterp [core]                                            t_postgres_core.cpp
*
*   core/db's PostgreSQL connection, pg_connection, over the PostgreSQL
* layer, against a live server -- PostgreSQL from 10, through whichever libpq
* the layer is built with. It runs the operations every vendor shares
* (t_db_core.hpp), database_table among them, in a schema of its own, and
* then what pg_connection adds: how a configuration and a connection string
* combine, the session's settings, failed transactions, SQL sent without
* waiting, parameters, COPY, notifications, rows a few at a time, pipelines,
* statements closed by name, escaping, the server's parameters and the
* connection's state, and a connection the server ends; and what
* postgres_table adds: a schema off the search path, commits by COPY, and
* upkeep.
*   Server: DJINTERP_TEST_POSTGRES_HOST (default 127.0.0.1), _PORT (5432),
* _USER (postgres), _PASSWORD (none) and _DATABASE (postgres) say where; the
* user needs to make and drop schemas, and to end its own sessions. Without
* a server only the checks that need none run, unless
* DJINTERP_TEST_POSTGRES_REQUIRED is 1, when that fails. Over a layer built
* without libpq (D_CFG_POSTGRES 0) it checks that nothing connects.
*   Below C++17 core/db compiles to nothing, and so does this test.
*
*   Build: compile the layer's C sources as C, src/djinterp/core/db/
* database.cpp, database_statement.cpp and postgres/postgres.cpp and this
* file as C++17 or later, and link them with libpq -- ci/check_db_postgres.sh
* does.
*
*
* path:      /tests/djinterp/core/db/postgres/t_postgres_core.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.23
*******************************************************************************/
// djinterp
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER
// std
#include <cstdio>  // std::printf, std::remove

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>       // std::chrono::system_clock
#include <cstdlib>      // std::strtoul
#include <cstring>      // std::strstr
#include <fstream>      // std::ifstream, std::ofstream
#include <iterator>     // std::istreambuf_iterator
#include <optional>     // std::optional, std::nullopt
#include <string>       // std::string, std::to_string
#include <type_traits>  // std::is_same_v
#include <vector>       // std::vector
// djinterp
#include "../../../../../inc/djinterp/core/db/postgres/postgres.hpp"  // under
                                                                       // test
#include "../../../../../inc/djinterp/core/db/postgres/postgres_table.hpp"
#include "../t_db_core.hpp"  // the shared checks


namespace
{

namespace db   = djinterp;
namespace core = d_tests_db_core;

// the concepts (C++20) -- djinterp::concepts' own, the class's among them,
// and a former name, which forwards to its concept for one release
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
static_assert(db::concepts::pg_connection<db::pg_connection>);
static_assert(db::concepts::pg_connection<db::pg_connection&>);
static_assert(!db::concepts::pg_connection<int>);
static_assert(db::concepts::non_postgres_connection<int>);
static_assert(db::Pg_connection<db::pg_connection>);
static_assert(!db::Pg_connection<int>);

// every concept of the family, against the class (C8): one the class
// failed would name a member it does not have; the non_ concepts fail it
// by design
static_assert(db::concepts::postgres_connection<db::pg_connection>);
static_assert(db::concepts::pg_connection<db::pg_connection>);
static_assert(!db::concepts::non_postgres_connection<db::pg_connection>);
static_assert(db::concepts::postgres_async_connection<db::pg_connection>);
static_assert(db::concepts::postgres_pipeline_connection<db::pg_connection>);
static_assert(db::concepts::postgres_copy_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_listen_notify_connection<db::pg_connection>);
static_assert(db::concepts::postgres_diagnostics_connection<db::pg_connection>);
static_assert(db::concepts::postgres_escape_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_large_object_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_schema_query_connection<db::pg_connection>);
static_assert(db::concepts::postgres_send_query_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_result_polling_connection<db::pg_connection>);
static_assert(db::concepts::postgres_busy_state_query<db::pg_connection>);
static_assert(
    db::concepts::postgres_input_consuming_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_pipeline_enterable_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_pipeline_exitable_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_pipeline_sync_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_pipeline_flush_connection<db::pg_connection>);
static_assert(db::concepts::postgres_copy_in_connection<db::pg_connection>);
static_assert(db::concepts::postgres_copy_data_connection<db::pg_connection>);
static_assert(db::concepts::postgres_copy_end_connection<db::pg_connection>);
static_assert(db::concepts::postgres_listenable_connection<db::pg_connection>);
static_assert(db::concepts::postgres_notifiable_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_notification_query_connection<db::pg_connection>);
static_assert(db::concepts::postgres_parameterized_execution_connection<
                  db::pg_connection>);
static_assert(db::concepts::postgres_backend_pid_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_transaction_status_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_parameter_status_connection<db::pg_connection>);
static_assert(db::concepts::postgres_has_table_query<db::pg_connection>);
static_assert(db::concepts::postgres_table_names_query<db::pg_connection>);
static_assert(
    db::concepts::postgres_escape_literal_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_escape_identifier_connection<db::pg_connection>);
static_assert(db::concepts::postgres_lo_import_connection<db::pg_connection>);
static_assert(db::concepts::postgres_lo_export_connection<db::pg_connection>);
static_assert(db::concepts::postgres_async_dispatchable<db::pg_connection>);
static_assert(db::concepts::postgres_pipelined_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_copy_protocol_connection<db::pg_connection>);
static_assert(
    db::concepts::postgres_listen_notifiable_connection<db::pg_connection>);
static_assert(db::concepts::postgres_full_connection<db::pg_connection>);
static_assert(db::concepts::pg_full_connection<db::pg_connection>);
#endif

/*
unique_violation
  PostgreSQL's duplicate key, 23505.
*/
bool
unique_violation(
    int _code
)
{
    return (_code == D_POSTGRES_SQLSTATE_UNIQUE_VIOLATION);
}

/*
sqlstate
  A failure's SQLSTATE, spelled.
*/
std::string
sqlstate(
    const db::exception& _failure
)
{
    return db::face::postgres::result_sqlstate(_failure.error_code());
}

// g_dialect
//   PostgreSQL's SQL and answers for the shared checks: bigserial ids,
// timestamptz for a point in time, bound as UTC with its offset and read
// back at UTC as text, whose trailing zeros PostgreSQL leaves off.
const core::dialect g_dialect =
{
    "PostgreSQL",
    "CREATE TABLE core_t (id bigserial PRIMARY KEY, name text, "
    "flag boolean, small integer, real_value double precision, "
    "bytes bytea, moment timestamptz)",
    "TRUE",
    "FALSE",
    "INSERT INTO core_t (name, flag, small, real_value, bytes, moment) "
    "VALUES ($1, $2, $3, $4, $5, $6)",
    "SELECT name, small FROM core_t WHERE id = $1",
    "SELECT (moment AT TIME ZONE 'UTC')::text FROM core_t WHERE id = $1",
    "1971-01-01 00:00:00.25",
    db::field_type::big_integer,
    db::field_type::integer,
    D_POSTGRES_SQLSTATE_SYNTAX_ERROR,
    &unique_violation
};

/*
server_config
  The run's server, as PostgreSQL's configuration, its search path the
schema given.
*/
db::pg_connect_config
server_config(
    const std::string& _schema
)
{
    db::pg_connect_config config;

    config.base.host     = core::setting("DJINTERP_TEST_POSTGRES_HOST",
                                         "127.0.0.1");
    config.base.port     = static_cast<re_std::uint16_t>(std::strtoul(
        core::setting("DJINTERP_TEST_POSTGRES_PORT", "5432").c_str(),
        nullptr,
        10));
    config.base.username = core::setting("DJINTERP_TEST_POSTGRES_USER",
                                         "postgres");
    config.base.password = core::setting("DJINTERP_TEST_POSTGRES_PASSWORD",
                                         "");
    config.base.database = core::setting("DJINTERP_TEST_POSTGRES_DATABASE",
                                         "postgres");
    config.search_path   = _schema;

    return config;
}

/*
keywords
  The run's server as a keyword connection string, with more after it.
*/
std::string
keywords(
    const std::string& _more
)
{
    const db::pg_connect_config config = server_config("");

    return "host='" + config.base.host + "' port=" +
           std::to_string(config.base.port) + " user='" +
           config.base.username + "'" +
           ( (config.base.password.empty())
                 ? std::string()
                 : " password='" + config.base.password + "'" ) +
           " " + _more;
}

/*
d_tests_postgres_core_configuring
  How a configuration and a connection string combine, and the session's
settings.
  Tests the following:
  - the string has the last word: its application_name overrides the
    field's; the fields fill in what it leaves out, the database among them
  - a URI read by parse_connection_string connects, its scheme no keyword;
    a list of hosts with their ports is split into libpq's lists
  - the search path, the statement timeout, the client encoding and the
    application name, as the configuration names them; base.schema holding
    a comma, a search path
  - a keyword libpq does not know, D_DB_INVALID_ARGUMENT in libpq's words;
    one it knows passes; a wrong password, 08001 in libpq's words
  - get_pg_config's base is the generic configuration connect(config) put
    in place, though an open connection did not reconnect
*/
void
d_tests_postgres_core_configuring(
    const std::string& _schema
)
{
    std::printf("configuring\n");

    db::pg_connect_config both(keywords("application_name=from_string"));

    both.application_name = "from_field";
    both.base.database    = server_config("").base.database;

    db::pg_connection combined(both);

    combined.connect();
    D_TESTS_DB_CORE_CHECK(combined.get_parameter_status("application_name") ==
                          "from_string");
    D_TESTS_DB_CORE_CHECK(std::get<std::string>(*combined.execute_scalar(
                              "SELECT current_database()")) ==
                          both.base.database);

    // a URI, and a host list
    const db::pg_connect_config base = server_config("");
    const std::string           authority =
        base.base.username + ":" + base.base.password + "@" +
        base.base.host + ":" + std::to_string(base.base.port);
    db::connection_config       uri = db::parse_connection_string(
        "postgresql://" + authority + "/" + base.base.database +
        "?application_name=from_uri");
    db::pg_connection           from_uri(uri);

    from_uri.connect();
    D_TESTS_DB_CORE_CHECK(from_uri.get_parameter_status("application_name") ==
                          "from_uri");
    from_uri.connect(base.base);
    D_TESTS_DB_CORE_CHECK(
        from_uri.get_pg_config().base.custom_options.empty());
    uri.host = base.base.host + ":" + std::to_string(base.base.port) + "," +
               base.base.host + ":" + std::to_string(base.base.port);

    db::pg_connection listed(uri);

    listed.connect();
    D_TESTS_DB_CORE_CHECK(listed.is_connected());

    // the session's settings
    db::pg_connect_config session = server_config(_schema);

    session.statement_timeout_ms = 60000;
    session.client_encoding      = "LATIN1";
    session.application_name     = "core";
    session.extra_params["target_session_attrs"] = "any";

    db::pg_connection settled(session);

    settled.connect();
    D_TESTS_DB_CORE_CHECK(std::get<std::string>(*settled.execute_scalar(
                              "SELECT current_schema()")) == _schema);
    D_TESTS_DB_CORE_CHECK(std::get<std::string>(*settled.execute_scalar(
                              "SHOW statement_timeout")) == "1min");
    D_TESTS_DB_CORE_CHECK( (settled.get_parameter_status("client_encoding") ==
                            "LATIN1") &&
                           (settled.get_parameter_status("application_name") ==
                            "core") );

    db::pg_connect_config listed_schema = server_config("");

    listed_schema.base.schema = _schema + ", public";

    db::pg_connection searching(listed_schema);

    searching.connect();
    D_TESTS_DB_CORE_CHECK(std::get<std::string>(*searching.execute_scalar(
                              "SELECT current_schema()")) == _schema);

    // what is refused
    db::pg_connect_config unknown = server_config("");

    unknown.extra_params["no_such_option"] = "x";

    try
    {
        db::pg_connection(unknown).connect();
        D_TESTS_DB_CORE_CHECK(false);
    }
    catch (const db::connection_exception& e)
    {
        D_TESTS_DB_CORE_CHECK( (e.error_code() == D_DB_INVALID_ARGUMENT) &&
                               (std::string(e.what()).find(
                                    "no_such_option") != std::string::npos) );
    }

    db::pg_connect_config wrong = server_config("");

    wrong.base.password = "not the password";

    try
    {
        db::pg_connection(wrong).connect();
        D_TESTS_DB_CORE_CHECK(false);
    }
    catch (const db::connection_exception& e)
    {
        D_TESTS_DB_CORE_CHECK(sqlstate(e) == "08001");
    }

    return;
}

/*
d_tests_postgres_core_transactions
  PostgreSQL's transactions.
  Tests the following:
  - a statement that fails inside one leaves it failed: the state 'E', and
    every statement after it 25P02, until a rollback, which the guard makes
  - a commit of a failed transaction rolls it back and throws, 25P02, the
    transaction ended
  - beginning inside a transaction, transaction_exception with 25001
  - the server's words for 25P02 and 3F000, which the layer gives on its own
    too, when the server gave them
  - READ COMMITTED by default; the session's level set, and returned to
    the server's default; a transaction at REPEATABLE READ
  - no auto-commit mode to turn off: set_auto_commit does not exist
*/
void
d_tests_postgres_core_transactions(
    db::pg_connection& _db
)
{
    std::printf("PostgreSQL's transactions\n");

    {
        db::transaction<db::pg_connection> work(_db);

        D_TESTS_DB_CORE_CHECK(_db.get_transaction_status() == 'T');
        D_TESTS_DB_CORE_THROWS(_db.execute("SELECT 1 / 0"),
                               db::query_exception,
                               D_POSTGRES_SQLSTATE('2', '2', '0', '1', '2'));
        D_TESTS_DB_CORE_CHECK(_db.get_transaction_status() == 'E');
        D_TESTS_DB_CORE_THROWS(_db.execute("SELECT 1"),
                               db::query_exception,
                               D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
        D_TESTS_DB_CORE_CHECK(_db.get_last_error().find("aborted") !=
                              std::string::npos);
    }

    D_TESTS_DB_CORE_THROWS(_db.execute("CREATE TABLE d_tests_none.t "
                                       "(a integer)"),
                           db::query_exception,
                           D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME);
    D_TESTS_DB_CORE_CHECK(_db.get_last_error().find("d_tests_none") !=
                          std::string::npos);

    D_TESTS_DB_CORE_CHECK( (_db.get_transaction_status() == 'I') &&
                           (!_db.in_transaction()) );
    _db.begin_transaction();
    D_TESTS_DB_CORE_THROWS_ANY(_db.execute("SELECT 1 / 0"),
                               db::query_exception);
    D_TESTS_DB_CORE_THROWS(_db.commit(),
                           db::transaction_exception,
                           D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
    D_TESTS_DB_CORE_CHECK(!_db.in_transaction());
    _db.begin_transaction();
    D_TESTS_DB_CORE_THROWS(_db.begin_transaction(),
                           db::transaction_exception,
                           D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION);
    _db.rollback();

    // levels
    D_TESTS_DB_CORE_CHECK(_db.get_isolation_level() ==
                          db::isolation_level::read_committed);
    _db.set_isolation_level(db::isolation_level::serializable);
    D_TESTS_DB_CORE_CHECK(_db.get_isolation_level() ==
                          db::isolation_level::serializable);
    _db.set_isolation_level(db::isolation_level::default_level);
    D_TESTS_DB_CORE_CHECK(_db.get_isolation_level() ==
                          db::isolation_level::read_committed);
    _db.begin_transaction(db::isolation_level::repeatable_read);
    D_TESTS_DB_CORE_CHECK(std::get<std::string>(*_db.execute_scalar(
                              "SHOW transaction_isolation")) ==
                          "repeatable read");
    _db.commit();
    D_TESTS_DB_CORE_CHECK(
        (!db::is_detected<db::internal::set_auto_commit_hook_t,
                          db::pg_connection>::value));

    return;
}

/*
d_tests_postgres_core_protocol
  SQL sent without waiting, parameters, COPY and notifications.
  Tests the following:
  - send_query: the state 'A' until every result is read, each result's
    rows and count, then nullptr; ping, meanwhile, reads without ending it;
    is_busy and consume_input
  - exec_params with text and NULL values; a value holding a NUL refused
  - COPY: rows sent in chunks and counted; abandoned, keeping none; a row the
    server cannot read, 22P02, at the end; rows with no COPY, 08000
  - LISTEN and NOTIFY: the connection's own notification, with its payload
    and its backend's process id; none after it; unlisten by channel and
    all; an empty channel refused; a payload too long, 22023
  - large objects: a file in and out again, byte for byte; an OID no object
    has, 42704; a file that is not there, D_DB_NOT_FOUND; a path holding a
    NUL refused; the large-object traits (C++17) and concepts (C++20)
*/
void
d_tests_postgres_core_protocol(
    db::pg_connection& _db
)
{
    std::printf("the protocol\n");

    // SQL sent
    D_TESTS_DB_CORE_CHECK(_db.send_query(
        "SELECT 1; UPDATE core_t SET small = small + 0 WHERE id < 3"));
    D_TESTS_DB_CORE_CHECK(_db.get_transaction_status() == 'A');
    D_TESTS_DB_CORE_CHECK(_db.ping());

    std::unique_ptr<db::buffered_result_set> first  = _db.get_result();
    std::unique_ptr<db::buffered_result_set> second = _db.get_result();

    D_TESTS_DB_CORE_CHECK( (first) &&
                           (first->row_count() == 1) );
    D_TESTS_DB_CORE_CHECK( (second) &&
                           (second->column_count() == 0u) &&
                           (_db.get_affected_rows() == 2) );
    D_TESTS_DB_CORE_CHECK(!_db.get_result());
    D_TESTS_DB_CORE_CHECK( (_db.get_transaction_status() == 'I') &&
                           (!_db.is_busy()) &&
                           (_db.consume_input()) );

    // parameters
    std::unique_ptr<db::buffered_result_set> computed = _db.exec_params(
        "SELECT $1::int + 1, $2::text IS NULL",
        std::vector<std::optional<std::string>>{ std::string("41"),
                                                 std::nullopt });

    D_TESTS_DB_CORE_CHECK( (computed->next()) &&
                           (computed->get_int(0u).value_or(0) == 42) &&
                           (computed->get_bool(1u).value_or(false)) );

    std::unique_ptr<db::buffered_result_set> named = _db.exec_params(
        "SELECT name FROM core_t WHERE id = $1",
        std::vector<std::string>{ "2" });

    D_TESTS_DB_CORE_CHECK( (named->next()) &&
                           (named->get_string(0u).value_or("") == "b") );
    D_TESTS_DB_CORE_THROWS(
        (void)_db.exec_params("SELECT $1",
                              std::vector<std::string>{
                                  std::string("a\0b", 3) }),
        db::query_exception,
        D_DB_INVALID_ARGUMENT);

    // COPY
    const char rows[] = "1\tx\n2\ty\n";

    _db.execute("CREATE TABLE copied (a int, b text)");
    _db.copy_in_start("COPY copied (a, b) FROM STDIN");
    D_TESTS_DB_CORE_CHECK(_db.get_transaction_status() == 'A');
    _db.copy_data(rows, 4u);
    _db.copy_data(rows + 4, sizeof(rows) - 5u);
    D_TESTS_DB_CORE_CHECK( (_db.copy_end() == 2) &&
                           (_db.get_affected_rows() == 2) );
    _db.copy_in_start("COPY copied (a, b) FROM STDIN");
    _db.copy_data("3\tz\n", 4u);
    _db.copy_abort("changed our mind");
    _db.copy_in_start("COPY copied (a, b) FROM STDIN");
    _db.copy_data("x\ty\n", 4u);
    D_TESTS_DB_CORE_THROWS(_db.copy_end(),
                           db::query_exception,
                           D_POSTGRES_SQLSTATE('2', '2', 'P', '0', '2'));
    D_TESTS_DB_CORE_THROWS(_db.copy_data("4\tw\n", 4u),
                           db::query_exception,
                           D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    D_TESTS_DB_CORE_CHECK(std::get<re_std::int64_t>(*_db.execute_scalar(
                              "SELECT count(*) FROM copied")) == 2);

    // notifications
    _db.listen("core_channel");
    _db.notify("core_channel", "hello");

    const std::optional<db::pg_notification> arrived = _db.get_notification();

    D_TESTS_DB_CORE_CHECK( (arrived) &&
                           (arrived->channel == "core_channel") &&
                           (arrived->payload == "hello") &&
                           (arrived->backend_pid == _db.get_backend_pid()) );
    D_TESTS_DB_CORE_CHECK(!_db.get_notification());
    _db.unlisten("core_channel");
    _db.unlisten_all();
    D_TESTS_DB_CORE_THROWS(_db.unlisten(""),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS(_db.notify("core_channel", std::string(8000u, 'p')),
                           db::query_exception,
                           D_POSTGRES_SQLSTATE_INVALID_PARAMETER_VALUE);

    // large objects
    const std::string in   = "d_tests_postgres_core_lo_in.bin";
    const std::string out  = "d_tests_postgres_core_lo_out.bin";
    std::string       data;

    for (int i = 0; i < 70000; ++i)
    {
        data.push_back(static_cast<char>(i % 251));
    }

    {
        std::ofstream file(in, std::ios::binary);

        file.write(data.data(), static_cast<std::streamsize>(data.size()));
    }

    const unsigned int oid = _db.lo_import(in);

    _db.lo_export(oid, out);

    {
        std::ifstream     file(out, std::ios::binary);
        const std::string back((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

        D_TESTS_DB_CORE_CHECK( (oid != 0u) && (back == data) );
    }

    D_TESTS_DB_CORE_THROWS(_db.lo_export(0u, out),
                           db::query_exception,
                           D_POSTGRES_SQLSTATE('4', '2', '7', '0', '4'));
    D_TESTS_DB_CORE_THROWS((void)_db.lo_import("d_tests_no_such_file"),
                           db::query_exception,
                           D_DB_NOT_FOUND);
    D_TESTS_DB_CORE_THROWS((void)_db.lo_import(std::string("a\0b", 3)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    _db.execute("SELECT lo_unlink(" + std::to_string(oid) + ")");
    (void)std::remove(in.c_str());
    (void)std::remove(out.c_str());
    static_assert(db::has_pg_large_objects_v<db::pg_connection>,
                  "pg_connection copies large objects");
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(db::concepts::postgres_large_object_connection<
                      db::pg_connection>);
    static_assert(db::concepts::postgres_lo_import_connection<
                      db::pg_connection>);
    static_assert(!db::concepts::postgres_large_object_connection<int>);
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

    return;
}

/*
d_tests_postgres_core_pipelines
  Rows a few at a time, pipelines, and statements closed by name.
  Tests the following:
  - rows a row a result, then the statement's end, which counts them; asked
    for with nothing sent, 08000; fewer than one row refused; more than one
    by libpq's release (17)
  - where libpq has pipelines (14): statements queued and synchronized,
    their results in order -- rows; a failure, its SQLSTATE and its message;
    25P02 for one skipped -- nullptr at each synchronization point, the mode
    aborted until it; an operation that waits for its result, 08000; rows a
    row a result for the statement asked for; a statement after the last
    point, read once waited for, or polled for once asked for; leaving, and
    synchronizing or asking outside the mode, 08000
  - where it has none: D_DB_UNSUPPORTED, the mode off
  - a statement PREPARE made, closed by its name, and closed again; an empty
    name, or one holding a NUL, refused
  - the pipeline concepts (C++20)
*/
void
d_tests_postgres_core_pipelines(
    db::pg_connection& _db
)
{
    std::printf("rows a few at a time, pipelines\n");

    // the rows of a statement a row a result, then its end
    D_TESTS_DB_CORE_CHECK(_db.send_query("SELECT generate_series(1, 2)"));
    _db.set_chunked_rows_mode(1);

    for (int r = 0; r < 2; ++r)
    {
        std::unique_ptr<db::buffered_result_set> row = _db.get_result();

        D_TESTS_DB_CORE_CHECK( (row) && (row->row_count() == 1) );
    }

    std::unique_ptr<db::buffered_result_set> end = _db.get_result();

    D_TESTS_DB_CORE_CHECK( (end) &&
                           (end->row_count() == 0) &&
                           (end->column_count() == 1u) &&
                           (_db.get_affected_rows() == 2) );
    D_TESTS_DB_CORE_CHECK(!_db.get_result());
    D_TESTS_DB_CORE_THROWS(_db.set_chunked_rows_mode(1),
                           db::query_exception,
                           D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    D_TESTS_DB_CORE_THROWS(_db.set_chunked_rows_mode(0),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_CHECK(db::pg_connection::supports_chunked_rows() ==
                          (db::face::postgres::client_version_number() >=
                           170000));

    if (!db::pg_connection::supports_chunked_rows())
    {
        D_TESTS_DB_CORE_THROWS(_db.set_chunked_rows_mode(2),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }

    D_TESTS_DB_CORE_CHECK(_db.get_pipeline_status() ==
                          db::pg_pipeline_status::off);

    if (db::pg_connection::supports_pipeline())
    {
        _db.enter_pipeline();
        D_TESTS_DB_CORE_CHECK(_db.get_pipeline_status() ==
                              db::pg_pipeline_status::on);

        // a batch with a failure, and one after it
        _db.send_query("SELECT 1");
        _db.send_query("SELECT * FROM no_table");
        _db.send_query("SELECT 2");
        _db.pipeline_sync();
        _db.send_query("SELECT 3");
        _db.pipeline_sync();
        D_TESTS_DB_CORE_THROWS(_db.execute("SELECT 9"),
                               db::query_exception,
                               D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
        D_TESTS_DB_CORE_CHECK(_db.get_result() != nullptr);

        try
        {
            (void)_db.get_result();
            D_TESTS_DB_CORE_CHECK(false);
        }
        catch (const db::query_exception& e)
        {
            D_TESTS_DB_CORE_CHECK( (e.error_code() ==
                                    D_POSTGRES_SQLSTATE_UNDEFINED_TABLE) &&
                                   (std::strstr(e.what(), "no_table") !=
                                    nullptr) );
        }

        D_TESTS_DB_CORE_CHECK(_db.get_pipeline_status() ==
                              db::pg_pipeline_status::aborted);
        D_TESTS_DB_CORE_THROWS(
            (void)_db.get_result(),
            db::query_exception,
            D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
        D_TESTS_DB_CORE_CHECK(!_db.get_result());
        D_TESTS_DB_CORE_CHECK(_db.get_pipeline_status() ==
                              db::pg_pipeline_status::on);

        std::unique_ptr<db::buffered_result_set> third = _db.get_result();

        D_TESTS_DB_CORE_CHECK( (third) &&
                               (third->next()) &&
                               (third->get_int(0u).value_or(0) == 3) );
        D_TESTS_DB_CORE_CHECK(!_db.get_result());
        D_TESTS_DB_CORE_CHECK(!_db.get_result());

        // a row a result, for the statement asked for
        _db.send_query("SELECT generate_series(1, 2)");
        _db.send_query("SELECT generate_series(1, 2)");
        _db.pipeline_sync();
        _db.set_chunked_rows_mode(1);

        std::unique_ptr<db::buffered_result_set> one = _db.get_result();

        D_TESTS_DB_CORE_CHECK( (one) && (one->row_count() == 1) );
        D_TESTS_DB_CORE_CHECK(_db.get_result() != nullptr);
        D_TESTS_DB_CORE_CHECK(_db.get_result() != nullptr);

        std::unique_ptr<db::buffered_result_set> both = _db.get_result();

        D_TESTS_DB_CORE_CHECK( (both) && (both->row_count() == 2) );
        D_TESTS_DB_CORE_CHECK(!_db.get_result());

        // after the last synchronization point: waited for, then polled for
        _db.send_query("SELECT 42");

        std::unique_ptr<db::buffered_result_set> waited = _db.get_result();

        D_TESTS_DB_CORE_CHECK( (waited) &&
                               (waited->next()) &&
                               (waited->get_int(0u).value_or(0) == 42) );
        D_TESTS_DB_CORE_CHECK(!_db.get_result());
        _db.send_query("SELECT 43");
        _db.pipeline_flush();

        unsigned long waits = 0u;

        while ( (_db.is_busy()) &&
                (waits < 100000000u) )
        {
            (void)_db.consume_input();
            ++waits;
        }

        std::unique_ptr<db::buffered_result_set> polled = _db.get_result();

        D_TESTS_DB_CORE_CHECK( (polled) &&
                               (polled->next()) &&
                               (polled->get_int(0u).value_or(0) == 43) );
        _db.pipeline_sync();
        D_TESTS_DB_CORE_CHECK(!_db.get_result());

        _db.exit_pipeline();
        D_TESTS_DB_CORE_CHECK(_db.get_pipeline_status() ==
                              db::pg_pipeline_status::off);
        D_TESTS_DB_CORE_THROWS(_db.pipeline_sync(),
                               db::query_exception,
                               D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
        D_TESTS_DB_CORE_THROWS(_db.pipeline_flush(),
                               db::query_exception,
                               D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
        D_TESTS_DB_CORE_CHECK(std::get<re_std::int32_t>(
                                  *_db.execute_scalar("SELECT 5")) == 5);
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(_db.enter_pipeline(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_THROWS(_db.pipeline_sync(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_THROWS(_db.pipeline_flush(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_CHECK(_db.get_pipeline_status() ==
                              db::pg_pipeline_status::off);
    }

    // a statement PREPARE made, closed by its name
    _db.execute("PREPARE core_named AS SELECT 1");
    _db.close_prepared("core_named");
    D_TESTS_DB_CORE_THROWS(_db.execute("EXECUTE core_named"),
                           db::query_exception,
                           D_POSTGRES_SQLSTATE('2', '6', '0', '0', '0'));
    _db.close_prepared("core_named");
    D_TESTS_DB_CORE_THROWS(_db.close_prepared(""),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS(_db.close_prepared(std::string("a\0b", 3)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);

    // the pipeline concepts
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(db::concepts::postgres_pipeline_connection<
                      db::pg_connection>);
    static_assert(db::concepts::postgres_pipeline_flush_connection<
                      db::pg_connection>);
    static_assert(db::concepts::postgres_pipelined_connection<
                      db::pg_connection>);
    static_assert(!db::concepts::postgres_pipeline_flush_connection<int>);
#endif
    static_assert(db::has_pg_pipeline_v<db::pg_connection>,
                  "pg_connection has pipelines");

    return;
}

/*
d_tests_postgres_core_session
  Escaping, the server's parameters, the connection's state, the schema,
and a connection the server ends.
  Tests the following:
  - escape_literal quotes for the connection, a backslash in the E'' form;
    escape_identifier doubles a double quote; NULs refused
  - the backend's process id, the server's encoding and version
  - has_table and list_tables in the current schema
  - the last id: 0 in a session that has used no sequence
  - a connection pg_terminate_backend ends: connection_exception, the
    connection in error and its state 'U' once disconnected; reconnect
*/
void
d_tests_postgres_core_session(
    db::pg_connection& _db,
    const std::string& _schema
)
{
    std::printf("the session\n");

    D_TESTS_DB_CORE_CHECK(_db.escape_literal("it's") == "'it''s'");
    D_TESTS_DB_CORE_CHECK(_db.escape_literal("a\\b") == " E'a\\\\b'");
    D_TESTS_DB_CORE_CHECK(_db.escape_identifier("a\"b") == "\"a\"\"b\"");
    D_TESTS_DB_CORE_THROWS((void)_db.escape_literal(std::string("\0", 1)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_CHECK( (_db.get_backend_pid() > 0) &&
                           (std::get<re_std::int32_t>(*_db.execute_scalar(
                                "SELECT pg_backend_pid()")) ==
                            _db.get_backend_pid()) );
    D_TESTS_DB_CORE_CHECK( (_db.get_parameter_status("server_encoding") ==
                            "UTF8") &&
                           (_db.get_parameter_status("no_such") == "") &&
                           (!_db.get_server_version().empty()) );
    D_TESTS_DB_CORE_CHECK( (_db.has_table("core_t")) &&
                           (!_db.has_table("nope")) );
    D_TESTS_DB_CORE_CHECK(_db.list_tables() ==
                          (std::vector<std::string>{ "copied", "core_t" }));
    D_TESTS_DB_CORE_CHECK(_db.get_native_handle() != nullptr);

    db::pg_connection fresh(server_config(_schema));

    fresh.connect();
    D_TESTS_DB_CORE_CHECK(fresh.get_last_insert_id() == 0);

    // ended by the server
    db::pg_connection killer(server_config(_schema));

    killer.connect();
    killer.execute("SELECT pg_terminate_backend(" +
                   std::to_string(fresh.get_backend_pid()) + ")");
    D_TESTS_DB_CORE_THROWS_ANY(fresh.execute("SELECT 1"),
                               db::connection_exception);
    D_TESTS_DB_CORE_CHECK(fresh.get_state() == db::connection_state::error);
    fresh.disconnect();
    D_TESTS_DB_CORE_CHECK( (fresh.get_transaction_status() == 'U') &&
                           (fresh.get_backend_pid() == 0) );
    fresh.reconnect();
    D_TESTS_DB_CORE_CHECK(fresh.is_connected());

    return;
}

/*
d_tests_postgres_core_tables
  What postgres_table adds to database_table, on a connection whose search
path is `_schema`.
  Tests the following:
  - a table in a schema off the search path: created there, found, written,
    its name qualified; not reached unqualified
  - a commit by COPY: text holding a tab, a newline and a backslash, bytes,
    a point in time, a boolean, a double and NULLs, read back as written; a
    row that leaves its key to an identity, inserted instead and read back;
    a row the server refuses failing the whole commit, which is rolled back
  - VACUUM, VACUUM FULL, ANALYZE and REINDEX through the layer, CONCURRENTLY
    from 12; VACUUM inside a transaction refused, 25001
  - rows exchanging keys under a DEFERRABLE key: written, the key deferred
    for the updates; inside the program's transaction as well
*/
void
d_tests_postgres_core_tables(
    db::pg_connection& _db,
    const std::string& _schema
)
{
    using clock = std::chrono::system_clock;

    std::printf("postgres_table\n");

    const std::string other = _schema + "_other";
    db::table_schema  schema;

    _db.execute("CREATE SCHEMA " + other);
    schema.table_name = "events";
    schema.columns    = { db::column_info("id",
                                          db::field_type::big_integer,
                                          false,
                                          true),
                          db::column_info("body", db::field_type::string),
                          db::column_info("data", db::field_type::binary),
                          db::column_info("at", db::field_type::timestamp),
                          db::column_info("flag", db::field_type::boolean),
                          db::column_info("n",
                                          db::field_type::floating_point) };

    db::postgres_table<> events(_db, schema);

    events.set_schema_name(other);
    D_TESTS_DB_CORE_CHECK( (events.get_schema_name() == other) &&
                           (events.qualified_table_name() ==
                            "\"" + other + "\".\"events\"") );
    events.create_table();
    D_TESTS_DB_CORE_CHECK(events.exists());
    D_TESTS_DB_CORE_THROWS_ANY((void)_db.execute_query(
                                   "SELECT * FROM events"),
                               db::query_exception);

    // a commit by COPY
    const clock::time_point instant =
        clock::time_point(std::chrono::seconds(86400 * 365)) +
        std::chrono::microseconds(250000);

    events.set_use_copy_on_commit(true);
    D_TESTS_DB_CORE_CHECK(events.uses_copy_on_commit());
    events.insert_row({ core::big_value(1),
                        core::text_value("tab\there\nnew\\line"),
                        db::value(std::vector<re_std::uint8_t>{ 0x00u, 0x5Cu,
                                                                0xFFu }),
                        db::value(instant),
                        db::value(true),
                        db::value(2.5) });
    events.insert_row({ core::big_value(2), db::value(), db::value(),
                        db::value(), db::value(), db::value() });
    events.commit();
    D_TESTS_DB_CORE_CHECK( (!events.is_dirty()) &&
                           (events.row_count_remote() == 2) );

    const std::string table = "\"" + other + "\".events";

    D_TESTS_DB_CORE_CHECK(core::scalar(_db, "SELECT COUNT(*) FROM " + table +
                                            " WHERE body = E'tab\\there"
                                            "\\nnew\\\\line' AND "
                                            "data = '\\x005cff'::bytea AND "
                                            "at = TIMESTAMPTZ '1971-01-01 "
                                            "00:00:00.25+00' AND flag AND "
                                            "n = 2.5") == 1);
    D_TESTS_DB_CORE_CHECK(core::scalar(_db, "SELECT COUNT(*) FROM " + table +
                                            " WHERE id = 2 AND body IS NULL "
                                            "AND data IS NULL AND at IS NULL "
                                            "AND flag IS NULL AND n IS "
                                            "NULL") == 1);

    // a row the server refuses -- its key taken behind the table's back --
    // fails the whole COPY
    _db.execute("INSERT INTO " + table + " (id) VALUES (5)");
    events.insert_row({ core::big_value(3), core::text_value("ok"),
                        db::value(), db::value(), db::value(),
                        db::value() });
    events.insert_row({ core::big_value(5), core::text_value("again"),
                        db::value(), db::value(), db::value(),
                        db::value() });
    D_TESTS_DB_CORE_THROWS(events.commit(),
                           db::query_exception,
                           D_POSTGRES_SQLSTATE_UNIQUE_VIOLATION);
    D_TESTS_DB_CORE_CHECK( (events.is_dirty()) &&
                           (events.row_count_remote() == 3) &&
                           (!_db.in_transaction()) &&
                           (_db.is_connected()) );
    events.refresh();
    D_TESTS_DB_CORE_CHECK(events.rows() == 3u);

    // a key left to an identity: inserted, not copied
    db::table_schema generated;
    db::column_info  serial("id", db::field_type::big_integer, false, true);

    serial.is_auto_increment = true;
    generated.table_name     = "serials";
    generated.columns        = { serial,
                                 db::column_info("v",
                                                 db::field_type::integer) };

    db::postgres_table<> serials(_db, generated);

    serials.set_schema_name(other);
    serials.create_table();
    serials.set_use_copy_on_commit(true);
    serials.insert_row({ db::value(), core::int_value(4) });
    serials.commit();
    D_TESTS_DB_CORE_CHECK( (serials.rows() == 1u) &&
                           (core::number(serials.cell(0u, 0u)) == 1) );

    // upkeep through the layer
    events.analyze_table();
    events.vacuum_table();
    events.vacuum_full_table();
    events.reindex_table();

    if (_db.get_face().server_at_least(12u, 0u))
    {
        events.reindex_table(true);
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(events.reindex_table(true),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }

    _db.begin_transaction();
    D_TESTS_DB_CORE_THROWS(events.vacuum_table(),
                           db::query_exception,
                           D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION);
    _db.rollback();
    _db.execute("DROP SCHEMA " + other + " CASCADE");
    D_TESTS_DB_CORE_CHECK(!events.exists());

    // rows exchanging keys under a DEFERRABLE key, deferred for the
    // updates, in the program's transaction too (no foreign key can
    // reference a deferrable one)
    _db.execute("CREATE TABLE swapped (id integer PRIMARY KEY DEFERRABLE, "
                "name text)");
    _db.execute("INSERT INTO swapped VALUES (1, 'one'), (2, 'two')");

    db::postgres_table<> exchanged(_db, std::string("swapped"));

    exchanged.fetch_schema();
    exchanged.set_order("id");
    exchanged.refresh();
    exchanged.update_cell(0u, "id", db::value(re_std::int32_t(2)));
    exchanged.update_cell(1u, "id", db::value(re_std::int32_t(1)));
    exchanged.commit();
    D_TESTS_DB_CORE_CHECK(std::get<std::string>(*_db.execute_scalar(
                              "SELECT name FROM swapped WHERE id = 2")) ==
                          "one");

    {
        db::transaction<db::pg_connection> work(_db);

        exchanged.update_cell(0u, "id", db::value(re_std::int32_t(1)));
        exchanged.update_cell(1u, "id", db::value(re_std::int32_t(2)));
        exchanged.commit();
        work.commit();
    }

    D_TESTS_DB_CORE_CHECK(std::get<std::string>(*_db.execute_scalar(
                              "SELECT name FROM swapped WHERE id = 1")) ==
                          "one");
    _db.execute("DROP TABLE swapped");

    return;
}

/*
d_tests_postgres_core_support
  What the server connected and the libpq loaded have, asked at run time.
  Tests the following:
  - before connecting, nothing of the server's
  - each server feature by the server's release: JSONB, logical
    replication, partitioning and parallel query on every server the layer
    takes; the JSON path language (12), multiranges (14), MERGE (15); each
    composite with the release that completes it (15, 16)
  - pipelines by libpq's release (14), with or without a connection
  - the compile-time struct by its name, and by its former names
*/
void
d_tests_postgres_core_support(
    const std::string& _schema
)
{
    std::printf("support\n");

    db::pg_connection db(server_config(_schema));

    D_TESTS_DB_CORE_CHECK( (!db.supports_jsonb()) &&
                           (!db.supports_merge()) &&
                           (!db.supports_modern_sql()) );
    D_TESTS_DB_CORE_CHECK(db::pg_connection::supports_pipeline() ==
                          (db::face::postgres::client_version_number() >=
                           140000));
    db.connect();

    const auto at_least = [&db](unsigned _major) -> bool
    {
        return db.get_face().server_at_least(_major, 0u);
    };

    D_TESTS_DB_CORE_CHECK( (db.supports_jsonb()) &&
                           (db.supports_logical_repl()) &&
                           (db.supports_partitioning()) &&
                           (db.supports_parallel_query()) &&
                           (db.supports_jsonpath() == at_least(12u)) &&
                           (db.supports_multirange() == at_least(14u)) &&
                           (db.supports_merge() == at_least(15u)) &&
                           (db.supports_modern_sql() == at_least(15u)) &&
                           (db.supports_modern_json() == at_least(16u)) );
    db.disconnect();
    D_TESTS_DB_CORE_CHECK(!db.supports_jsonb());

    // the compile-time struct, and its former names
    static_assert(std::is_same_v<db::pg_connection::client_capabilities,
                                 db::pg_client_capabilities>,
                  "client_capabilities is the compile-time struct");
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    static_assert( (std::is_same_v<db::pg_type_support,
                                   db::pg_client_capabilities>) &&
                   (std::is_same_v<db::pg_feature_support,
                                   db::pg_client_capabilities>),
                  "the former names are the struct's");
#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

    return;
}

/*
d_tests_postgres_core_stubs
  Over a layer built without libpq.
  Tests the following:
  - connect throws connection_exception with D_DB_UNSUPPORTED, and leaves
    the connection in error; the state is 'U'
  - no libpq to support pipelines
*/
void
d_tests_postgres_core_stubs(
    void
)
{
    std::printf("over the stubs\n");

    db::pg_connection db(server_config(""));

    D_TESTS_DB_CORE_THROWS(db.connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_CHECK( (db.get_state() == db::connection_state::error) &&
                           (db.get_transaction_status() == 'U') );
    D_TESTS_DB_CORE_THROWS_ANY(db.execute("SELECT 1"),
                               db::connection_exception);
    D_TESTS_DB_CORE_CHECK( (!db::pg_connection::supports_pipeline()) &&
                           (!db.supports_jsonb()) );

    return;
}

}  // namespace

/*
main
  Over libpq, every group where a server answers, in a schema of the run's
own, dropped at the end; over the stubs, what the stubs report.
*/
int
main(
    void
)
{
    const bool stubs    = (db::face::postgres::client_version_number() == 0);
    const bool required =
        (core::setting("DJINTERP_TEST_POSTGRES_REQUIRED", "0") == "1");

    // over the stubs
    if (stubs)
    {
        d_tests_postgres_core_stubs();

        return core::summary("built without libpq");
    }

    db::pg_connection setup(server_config(""));

    // a server to reach, or nothing to run against
    try
    {
        setup.connect();
    }
    catch (const db::connection_exception& e)
    {
        std::printf("  no server reached (%s)\n", e.what());
        D_TESTS_DB_CORE_CHECK(!required);

        return core::summary("no server");
    }

    const std::string schema = "d_tests_core_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count() %
        100000000);

    setup.execute("DROP SCHEMA IF EXISTS " + schema + " CASCADE");
    setup.execute("CREATE SCHEMA " + schema);

    {
        db::pg_connection db(server_config(schema));

        db.connect();
        core::queries(db, g_dialect);
        core::statements(db, g_dialect);
        core::transactions(db);
        core::errors(db, g_dialect);
        core::streams(db, g_dialect);
        core::lifetimes<db::pg_connection>(
            [&schema]() { return db::pg_connection(server_config(schema)); });
        core::tables<db::database_table<db::pg_connection>>(
            db,
            g_dialect,
            core::table_abilities
            {
                true,
                true,
                db.get_face().server_at_least(12u, 0u)
            });
        d_tests_postgres_core_configuring(schema);
        d_tests_postgres_core_support(schema);
        d_tests_postgres_core_transactions(db);
        d_tests_postgres_core_protocol(db);
        d_tests_postgres_core_pipelines(db);
        d_tests_postgres_core_session(db, schema);
        d_tests_postgres_core_tables(db, schema);
    }

    setup.execute("DROP SCHEMA " + schema + " CASCADE");

    return core::summary("PostgreSQL " + setup.get_server_version() +
                         ", libpq " +
                         std::to_string(
                             db::face::postgres::client_version_number()));
}

#else

/*
main
  Below C++17 core/db is absent, and there is nothing to test.
*/
int
main(
    void
)
{
    std::printf("core/db is absent below C++17\n");

    return 0;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
