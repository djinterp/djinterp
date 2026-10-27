/*******************************************************************************
* djinterp [core]                                             t_mariadb_core.cpp
*
*   core/db's MySQL-family connections, mariadb_connection and
* mysql_connection, over the MariaDB layer, against a live server -- MariaDB
* from 10.3 or MySQL from 5.7, through whichever client the layer is built
* with. Each leaf runs the operations every vendor shares (t_db_core.hpp) and
* the family's own members (mysql_common_connection's) -- the session's reset
* and its tracked changes among them -- in a database of its own,
* database_table among them, and then its own configuration: MariaDB's
* storage engine and Galera's causal reads, MySQL's TLS modes, connect
* attributes, the settings it refuses and its query attributes; and its
* table class, mariadb_table or mysql_table, with what mysql_common_table
* adds to both. Where an answer depends on the server, the check asks it.
*   Server: DJINTERP_TEST_MARIADB_HOST (default 127.0.0.1), _PORT (3306),
* _USER (root), _PASSWORD (none) and _SOCKET (none) say where; the user needs
* every privilege on databases named d_tests_core_%. Without a server only
* the checks that need none run, unless DJINTERP_TEST_MARIADB_REQUIRED is 1,
* when that fails. Over a layer built without a client (D_CFG_MARIADB 0) it
* checks that nothing connects.
*   Below C++17 core/db compiles to nothing, and so does this test.
*
*   Build: compile the layer's C sources as C, src/djinterp/core/db/
* database.cpp, database_statement.cpp, mysql/mysql_common.cpp,
* mariadb/mariadb.cpp and mysql/mysql.cpp and this file as C++17 or later,
* and link them with the client -- ci/check_db_mariadb.sh does.
*
*
* path:      /tests/unintegrated/core/db/mariadb/t_mariadb_core.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.27
*******************************************************************************/
// _WIN32_WINNT
//   knob (Windows'): Vista's API at least, for WSAPoll, where nothing asked
// for more; it precedes every include.
#if ( (defined(_WIN32)) &&                                                    \
      (!defined(_WIN32_WINNT)) )
    #define _WIN32_WINNT 0x0600
#endif  // _WIN32
// WIN32_LEAN_AND_MEAN, NOMINMAX
//   knobs (Windows'): <windows.h>, which <winsock2.h> and the clients' own
// headers include, without its rarer headers -- rpcndr.h among them, which
// makes `small` a macro -- and without its min and max macros.
#if defined(_WIN32)
    #if !defined(WIN32_LEAN_AND_MEAN)
        #define WIN32_LEAN_AND_MEAN 1
    #endif  // WIN32_LEAN_AND_MEAN
    #if !defined(NOMINMAX)
        #define NOMINMAX 1
    #endif  // NOMINMAX
#endif  // _WIN32
// djinterp
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER
// std
#include <cstdio>  // std::printf

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>       // std::chrono::system_clock
#include <cstdlib>      // std::strtoul
#include <string>       // std::string, std::to_string
#include <type_traits>  // std::is_same_v
#include <vector>       // std::vector
// djinterp
#include "../../../../../inc/djinterp/core/db/mariadb/mariadb.hpp"  // under
#include "../../../../../inc/djinterp/core/db/mysql/mysql.hpp"      // test
#include "../../../../../inc/djinterp/core/db/mariadb/mariadb_table.hpp"
#include "../../../../../inc/djinterp/core/db/mysql/mysql_table.hpp"
// D_TESTS_DB_CORE_SOCKETS
//   switch: the shared checks wait on the server's socket, for the
// operations that do not block.
#define D_TESTS_DB_CORE_SOCKETS 1
#include "../t_db_core.hpp"  // the shared checks


namespace
{

namespace db   = djinterp;
namespace core = d_tests_db_core;

// the operations that do not block, as a trait from C++17 (database_
// connection.hpp, section 2)
static_assert(db::has_async_operations<db::mariadb_connection>::value);
static_assert(db::has_async_operations<db::mysql_connection>::value);
static_assert(!db::has_async_operations<int>::value);

// the concepts (C++20) -- djinterp::concepts' own: the family's, which both
// leaves satisfy, and each leaf's; and the former names, which forward to
// their concepts for one release
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
static_assert(db::concepts::mysql_family_connection<db::mariadb_connection>);
static_assert(db::has_mysql_result_modes_v<db::mariadb_connection>);
static_assert(db::has_mysql_result_modes_v<db::mysql_connection>);
static_assert(db::mysql_can_use_result<db::mysql_connection>);
static_assert(db::has_mysql_options_v<db::mariadb_connection>);
static_assert(db::concepts::mysql_family_connection<db::mysql_connection>);
static_assert(!db::concepts::mysql_family_connection<int>);
static_assert(db::concepts::non_mysql_connection<int>);
static_assert(db::concepts::mysql_full_connection<db::mariadb_connection>);
static_assert(db::concepts::mysql_full_connection<db::mysql_connection>);
static_assert(db::concepts::mariadb_connection<db::mariadb_connection>);
static_assert(!db::concepts::mariadb_connection<int>);
static_assert(db::concepts::mysql_ora_connection<db::mysql_connection>);
static_assert(!db::concepts::mysql_ora_connection<int>);
static_assert(db::Mysql_connection<db::mysql_connection>);
static_assert(db::Mariadb_connection<db::mariadb_connection>);
static_assert(db::mysql_ora_connection<db::mysql_connection>);
static_assert(!db::Mysql_connection<int>);

// the reset, which both leaves have, the family's; session tracking, which
// MySQL's concepts name and both leaves have; query attributes, MySQL's
// alone
static_assert(
    db::concepts::mariadb_resettable_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mariadb_reset_connection_capable<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_ora_resettable_connection<db::mysql_connection>);
static_assert(
    db::concepts::mysql_ora_reset_connection_capable<db::mysql_connection>);
static_assert(
    db::concepts::mysql_ora_session_tracking_connection<db::mysql_connection>);
static_assert(
    db::concepts::mysql_ora_session_trackable<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_ora_query_attributes_connection<db::mysql_connection>);
static_assert(
    db::concepts::mysql_ora_query_attribute_settable<db::mysql_connection>);
static_assert(
    !db::concepts::mysql_ora_query_attribute_settable<db::mariadb_connection>);
static_assert(!db::concepts::mariadb_resettable_connection<int>);

// the operations that do not block, on both leaves
static_assert(db::concepts::async_connection<db::mariadb_connection>);
static_assert(db::concepts::async_connection<db::mysql_connection>);
static_assert(!db::concepts::async_connection<int>);

// every concept of the family, against the class (C8): one the class
// failed would name a member it does not have; the non_ concepts fail it
// by design
static_assert(db::concepts::mysql_family_connection<db::mariadb_connection>);
static_assert(!db::concepts::non_mysql_connection<db::mariadb_connection>);
static_assert(db::concepts::mysql_charset_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_multi_result_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_diagnostics_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_result_modes_connection<db::mariadb_connection>);
static_assert(db::concepts::mysql_escape_connection<db::mariadb_connection>);
static_assert(db::concepts::mysql_options_connection<db::mariadb_connection>);
static_assert(db::concepts::mysql_engine_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_use_database_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_autocommit_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_set_option_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_get_option_connection<db::mariadb_connection>);
static_assert(db::concepts::mysql_stat_connection<db::mariadb_connection>);
static_assert(db::concepts::mysql_thread_id_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_warning_count_connection<db::mariadb_connection>);
static_assert(db::concepts::mysql_sqlstate_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_store_result_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_use_result_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_change_user_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_set_engine_connection<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_get_engine_connection<db::mariadb_connection>);
static_assert(db::concepts::mysql_charset_manageable<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_multi_result_iterable<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_result_mode_selectable<db::mariadb_connection>);
static_assert(
    db::concepts::mysql_diagnostic_connection_tagless<db::mariadb_connection>);
static_assert(db::concepts::mysql_charset_settable<db::mariadb_connection>);
static_assert(db::concepts::mysql_charset_gettable<db::mariadb_connection>);
static_assert(db::concepts::mysql_result_iterable<db::mariadb_connection>);
static_assert(db::concepts::mysql_more_results_query<db::mariadb_connection>);
static_assert(db::concepts::mysql_string_escapable<db::mariadb_connection>);
static_assert(db::concepts::mysql_database_selectable<db::mariadb_connection>);
static_assert(db::concepts::mysql_user_switchable<db::mariadb_connection>);
static_assert(db::concepts::mysql_full_connection<db::mariadb_connection>);
static_assert(db::concepts::mysql_family_connection<db::mysql_connection>);
static_assert(!db::concepts::non_mysql_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_charset_connection<db::mysql_connection>);
static_assert(
    db::concepts::mysql_multi_result_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_diagnostics_connection<db::mysql_connection>);
static_assert(
    db::concepts::mysql_result_modes_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_escape_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_options_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_engine_connection<db::mysql_connection>);
static_assert(
    db::concepts::mysql_use_database_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_autocommit_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_set_option_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_get_option_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_stat_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_thread_id_connection<db::mysql_connection>);
static_assert(
    db::concepts::mysql_warning_count_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_sqlstate_connection<db::mysql_connection>);
static_assert(
    db::concepts::mysql_store_result_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_use_result_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_change_user_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_set_engine_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_get_engine_connection<db::mysql_connection>);
static_assert(db::concepts::mysql_charset_manageable<db::mysql_connection>);
static_assert(db::concepts::mysql_multi_result_iterable<db::mysql_connection>);
static_assert(db::concepts::mysql_result_mode_selectable<db::mysql_connection>);
static_assert(
    db::concepts::mysql_diagnostic_connection_tagless<db::mysql_connection>);
static_assert(db::concepts::mysql_charset_settable<db::mysql_connection>);
static_assert(db::concepts::mysql_charset_gettable<db::mysql_connection>);
static_assert(db::concepts::mysql_result_iterable<db::mysql_connection>);
static_assert(db::concepts::mysql_more_results_query<db::mysql_connection>);
static_assert(db::concepts::mysql_string_escapable<db::mysql_connection>);
static_assert(db::concepts::mysql_database_selectable<db::mysql_connection>);
static_assert(db::concepts::mysql_user_switchable<db::mysql_connection>);
static_assert(db::concepts::mysql_full_connection<db::mysql_connection>);
#endif

// ER_* -- the server errors the checks compare with, as both servers number
// them
const int ER_ACCESS_DENIED           = 1045;
const int ER_DUP_ENTRY               = 1062;
const int ER_QUERY_INTERRUPTED       = 1317;
const int ER_PARSE_ERROR             = 1064;
const int ER_UNKNOWN_SYSTEM_VARIABLE = 1193;

/*
unique_violation
  Both servers' duplicate key.
*/
bool
unique_violation(
    int _code
)
{
    return (_code == ER_DUP_ENTRY);
}

/*
cancelled
  Both servers' statement stopped by KILL QUERY.
*/
bool
cancelled(
    int _code
)
{
    return (_code == ER_QUERY_INTERRUPTED);
}

// make_dialect
//   the family's SQL and answers for the shared checks: BIGINT
// AUTO_INCREMENT ids, a DATETIME with microseconds, which the server shows
// to six places; a failure's message names the server, MariaDB or MySQL,
// whichever leaf connected to it.
core::dialect
make_dialect(
    bool _mariadb
)
{
    return core::dialect
    {
        (_mariadb) ? "MariaDB" : "MySQL",
        "CREATE TABLE core_t (id BIGINT AUTO_INCREMENT PRIMARY KEY, "
        "name VARCHAR(64), flag BOOLEAN, small INT, real_value DOUBLE, "
        "bytes BLOB, moment DATETIME(6))",
        "TRUE",
        "FALSE",
        "INSERT INTO core_t (name, flag, small, real_value, bytes, moment) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        "SELECT name, small FROM core_t WHERE id = ?",
        "SELECT moment FROM core_t WHERE id = ?",
        "1971-01-01 00:00:00.250000",
        db::field_type::big_integer,
        db::field_type::integer,
        ER_PARSE_ERROR,
        &unique_violation,
        "SELECT SLEEP(0.3)",
        "SELECT SLEEP(5)",
        "SELECT COUNT(*) FROM information_schema.PROCESSLIST "
        "WHERE ID = %lu AND INFO = 'SELECT SLEEP(5)'",
        &cancelled
    };
}

/*
server_config
  The run's server, as a generic configuration, with a database or none.
*/
db::connection_config
server_config(
    const std::string& _database
)
{
    db::connection_config config;

    config.host     = core::setting("DJINTERP_TEST_MARIADB_HOST",
                                    "127.0.0.1");
    config.port     = static_cast<re_std::uint16_t>(std::strtoul(
        core::setting("DJINTERP_TEST_MARIADB_PORT", "3306").c_str(),
        nullptr,
        10));
    config.username = core::setting("DJINTERP_TEST_MARIADB_USER", "root");
    config.password = core::setting("DJINTERP_TEST_MARIADB_PASSWORD", "");
    config.database = _database;

    return config;
}

/*
family_config
  The run's server, as the family's configuration: the socket too.
*/
db::mysql_connect_config
family_config(
    const std::string& _database
)
{
    db::mysql_connect_config config(server_config(_database));

    config.unix_socket = core::setting("DJINTERP_TEST_MARIADB_SOCKET", "");

    return config;
}

/*
d_tests_mariadb_core_carried
  Whether the client carries TLS without blocking (d_mariadb_client_
nonblocking_tls), where it has calls that do not block at all: false only
for Connector/C before 3.4.10 with Schannel.
*/
bool
d_tests_mariadb_core_carried(
    void
)
{
    return ( (!db::face::mariadb::client_is_mariadb()) ||
             (db::face::mariadb::client_nonblocking_tls()) );
}

// d_tests_mariadb_core_off
//   value: Connector/C's false, a my_bool (a char), for the switches
// d_tests_mariadb_core_untls sets, which the client reads as the connect
// begins.
const char d_tests_mariadb_core_off = 0;

/*
d_tests_mariadb_core_untls
  A connection, not yet made, told not to use TLS, through Connector/C's own
switches, set last so that they stand over the layer's PREFERRED:
MYSQL_OPT_SSL_ENFORCE (38) and MYSQL_OPT_SSL_VERIFY_SERVER_CERT (21), the
same numbers in mysql.h from 3.0 to 3.4.11, which the core leaves out of
view.
*/
template<typename Connection>
Connection
d_tests_mariadb_core_untls(
    Connection _db
)
{
    const int SSL_VERIFY_SERVER_CERT = 21;
    const int SSL_ENFORCE            = 38;

    _db.set_option(SSL_VERIFY_SERVER_CERT, &d_tests_mariadb_core_off);
    _db.set_option(SSL_ENFORCE, &d_tests_mariadb_core_off);

    return _db;
}

/*
d_tests_mariadb_core_under_tls
  Whether a connection runs under TLS: the session's Ssl_cipher, which both
servers report, not empty.
*/
template<typename Connection>
bool
d_tests_mariadb_core_under_tls(
    Connection& _db
)
{
    auto status = _db.execute_query("SHOW SESSION STATUS LIKE 'Ssl_cipher'");

    return ( (status->next()) &&
             (!status->get_string(1u).value_or("").empty()) );
}

/*
fresh_database
  A database of the run's own, made empty.
*/
template<typename Connection>
void
fresh_database(
    Connection&        _setup,
    const std::string& _name
)
{
    _setup.execute("DROP DATABASE IF EXISTS " + _name);
    _setup.execute("CREATE DATABASE " + _name);

    return;
}

/*
d_tests_mariadb_core_family
  mysql_common_connection's own members, on a connection to `_database`,
which core_t is in.
  Tests the following:
  - the character set: utf8mb4, set to another and back
  - escape_string as the client escapes, its quotes left out; a NUL refused
  - use_database names the current database, in get_database_name and in
    get_mysql_config's generic configuration; get_stat, the connection's
    id, the warnings and the SQLSTATE
  - the default storage engine set and read; one the server lacks refused
  - execute_script runs every statement, and stops at the first that fails;
    next_result and more_results have nothing left to read
  - isolation levels: REPEATABLE READ, the default, set to another, and one
    transaction at a third; auto-commit off and on, by both spellings
  - has_table and list_tables in the current database; the face and the
    handle
*/
template<typename Connection>
void
d_tests_mariadb_core_family(
    Connection&        _db,
    const std::string& _database
)
{
    std::printf("the family's members\n");

    D_TESTS_DB_CORE_CHECK(_db.get_charset() == "utf8mb4");
    _db.set_charset("latin1");
    D_TESTS_DB_CORE_CHECK(_db.get_charset() == "latin1");
    _db.set_charset("utf8mb4");
    D_TESTS_DB_CORE_CHECK(_db.escape_string("it's") == "it\\'s");
    D_TESTS_DB_CORE_THROWS((void)_db.escape_string(std::string("a\0", 2)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);

    // the database, diagnostics
    _db.use_database("information_schema");
    D_TESTS_DB_CORE_CHECK( (_db.get_database_name() == "information_schema") &&
                           (_db.get_mysql_config().base.database ==
                            "information_schema") );
    _db.use_database(_database);
    D_TESTS_DB_CORE_CHECK(!_db.get_stat().empty());
    D_TESTS_DB_CORE_CHECK(_db.get_thread_id() > 0ul);
    (void)_db.execute_query("SELECT CAST('x' AS SIGNED)");
    D_TESTS_DB_CORE_CHECK(_db.get_warning_count() >= 1u);
    _db.execute("SELECT 1");
    D_TESTS_DB_CORE_CHECK(_db.get_sqlstate() == "00000");

    // the storage engine
    D_TESTS_DB_CORE_CHECK(_db.get_engine() == "InnoDB");
    _db.set_engine("MyISAM");
    D_TESTS_DB_CORE_CHECK(_db.get_engine() == "MyISAM");
    D_TESTS_DB_CORE_THROWS_ANY(_db.set_engine("No'Such"),
                               db::query_exception);
    _db.set_engine("InnoDB");

    // scripts
    _db.execute_script("CREATE TABLE script_t (x INT); "
                       "INSERT INTO script_t VALUES (1); "
                       "INSERT INTO script_t VALUES (2)");
    D_TESTS_DB_CORE_CHECK(std::get<re_std::int64_t>(*_db.execute_scalar(
                              "SELECT COUNT(*) FROM script_t")) == 2);

    try
    {
        _db.execute_script("INSERT INTO script_t VALUES (3); SELEC 1; "
                           "INSERT INTO script_t VALUES (4)");
        D_TESTS_DB_CORE_CHECK(false);
    }
    catch (const db::query_exception& e)
    {
        D_TESTS_DB_CORE_CHECK(e.error_code() == ER_PARSE_ERROR);
    }

    D_TESTS_DB_CORE_CHECK(std::get<re_std::int64_t>(*_db.execute_scalar(
                              "SELECT COUNT(*) FROM script_t")) == 3);
    D_TESTS_DB_CORE_CHECK( (_db.next_result() == -1) &&
                           (!_db.more_results()) );

    // levels and auto-commit
    D_TESTS_DB_CORE_CHECK(_db.get_isolation_level() ==
                          db::isolation_level::repeatable_read);
    _db.set_isolation_level(db::isolation_level::read_committed);
    _db.begin_transaction(db::isolation_level::serializable);
    _db.rollback();
    D_TESTS_DB_CORE_CHECK(_db.get_isolation_level() ==
                          db::isolation_level::read_committed);
    _db.set_isolation_level(db::isolation_level::repeatable_read);
    _db.set_auto_commit(false);
    D_TESTS_DB_CORE_CHECK(!_db.get_auto_commit());
    _db.set_autocommit(true);
    D_TESTS_DB_CORE_CHECK(_db.get_auto_commit());

    // the schema, the face, the handle
    D_TESTS_DB_CORE_CHECK( (_db.has_table("core_t")) &&
                           (!_db.has_table("nope")) );
    D_TESTS_DB_CORE_CHECK(_db.list_tables() ==
                          (std::vector<std::string>{ "core_t",
                                                     "script_t" }));
    D_TESTS_DB_CORE_CHECK( (_db.get_face().is_open()) &&
                           (_db.get_native_handle() != nullptr) );

    return;
}

/*
d_tests_mariadb_core_session
  Resetting the session, and what the server reports changing in it, on a
connection of Connection's own to `_database`, whose init command sets a
variable.
  Tests the following:
  - supports_reset_connection and supports_session_tracking: the layer's
    answers once connected, false before
  - where the server and the client have the reset: a fresh session -- no
    transaction, user variables gone -- with the database kept, the init
    command run again, and the statements prepared before closed; where
    not, query_exception with D_DB_UNSUPPORTED, nothing changed
  - where they have tracking: a variable the session asks for, with its
    value; nothing for a SELECT; where not, D_DB_UNSUPPORTED
*/
template<typename Connection>
void
d_tests_mariadb_core_session(
    const std::string& _database
)
{
    std::printf("the session: reset and tracking\n");

    db::mysql_connect_config config = family_config(_database);

    config.init_command = "SET @core_init = 42";

    Connection session(config);

    D_TESTS_DB_CORE_CHECK( (!session.supports_reset_connection()) &&
                           (!session.supports_session_tracking()) );
    session.connect();

    const bool resets =
        session.get_face().supports(D_MARIADB_FEATURE_RESET_CONNECTION);
    const bool tracks =
        session.get_face().supports(D_MARIADB_FEATURE_SESSION_TRACKING);

    D_TESTS_DB_CORE_CHECK( (session.supports_reset_connection() == resets) &&
                           (session.supports_session_tracking() == tracks) );

    // a session with something to lose
    session.execute("SET @core_init = 7, @core_kept = 1");

    auto prepared = session.prepare("SELECT 1");

    session.begin_transaction();

    if (resets)
    {
        session.reset_connection();
        D_TESTS_DB_CORE_CHECK( (session.is_connected()) &&
                               (!session.in_transaction()) );
        D_TESTS_DB_CORE_CHECK(std::get<re_std::int64_t>(*session.execute_scalar(
                                  "SELECT @core_init")) == 42);
        D_TESTS_DB_CORE_CHECK(std::holds_alternative<std::monostate>(
                                  *session.execute_scalar(
                                      "SELECT @core_kept")));
        D_TESTS_DB_CORE_CHECK(std::get<std::string>(*session.execute_scalar(
                                  "SELECT DATABASE()")) == _database);
        D_TESTS_DB_CORE_THROWS_ANY((void)prepared->execute_query(),
                                   db::exception);
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(session.reset_connection(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_CHECK(session.in_transaction());
        session.rollback();
    }

    if (tracks)
    {
        session.execute("SET SESSION session_track_system_variables = "
                        "'time_zone'");
        session.execute("SET SESSION time_zone = '+02:00'");
        D_TESTS_DB_CORE_CHECK(
            session.get_session_track_info(
                db::mysql_session_track::system_variables) ==
            (std::vector<std::string>{ "time_zone", "+02:00" }));
        (void)session.execute_query("SELECT 1");
        D_TESTS_DB_CORE_CHECK(
            session.get_session_track_info(
                db::mysql_session_track::system_variables).empty());
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(
            (void)session.get_session_track_info(
                db::mysql_session_track::schema),
            db::query_exception,
            D_DB_UNSUPPORTED);
    }

    return;
}

/*
d_tests_mariadb_core_connecting
  Connecting: what the server refuses, what core/db refuses, and the
client's flags.
  Tests the following:
  - a wrong password: connection_exception with 1045, in the server's words
  - multi_statements, and a custom option the client has no keyword for,
    refused with D_DB_UNSUPPORTED; a URI's scheme passes
  - found_rows makes an UPDATE count the rows it matched
  - change_user: to the same user, kept; with a wrong password, refused --
    MariaDB keeping the connection, MySQL closing it, which is then lost
*/
template<typename Connection>
void
d_tests_mariadb_core_connecting(
    const std::string& _database,
    bool               _mariadb
)
{
    std::printf("connecting\n");

    db::connection_config wrong = server_config(_database);

    wrong.password = "not the password";

    Connection refused(wrong);

    try
    {
        refused.connect();
        D_TESTS_DB_CORE_CHECK(false);
    }
    catch (const db::connection_exception& e)
    {
        D_TESTS_DB_CORE_CHECK( (e.error_code() == ER_ACCESS_DENIED) &&
                               (std::string(e.what()).find(
                                    "Access denied") != std::string::npos) );
    }

    db::mysql_connect_config several = family_config(_database);

    several.multi_statements = true;
    D_TESTS_DB_CORE_THROWS(Connection(several).connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);

    db::connection_config optioned = server_config(_database);

    optioned.custom_options["scheme"] = "mysql";

    Connection schemed(optioned);

    schemed.connect();
    D_TESTS_DB_CORE_CHECK(schemed.is_connected());
    optioned.custom_options["ssl-mode"] = "REQUIRED";
    D_TESTS_DB_CORE_THROWS(Connection(optioned).connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);

    // found rows
    Connection counting(family_config(_database));

    counting.add_client_flag(db::mysql_client_flag::found_rows);
    D_TESTS_DB_CORE_CHECK(counting.has_client_flag(
                              db::mysql_client_flag::found_rows));
    counting.connect();
    D_TESTS_DB_CORE_CHECK(counting.execute_update(
                              "UPDATE core_t SET small = small") > 0);

    // changing user
    const db::connection_config same = server_config(_database);

    counting.change_user(same.username, same.password, _database);
    D_TESTS_DB_CORE_CHECK(counting.is_connected());

    try
    {
        counting.change_user(same.username, "not the password", "");
        D_TESTS_DB_CORE_CHECK(false);
    }
    catch (const db::connection_exception& e)
    {
        D_TESTS_DB_CORE_CHECK( (!_mariadb) &&
                               (e.error_code() == ER_ACCESS_DENIED) &&
                               (counting.get_state() ==
                                db::connection_state::error) );
    }
    catch (const db::query_exception& e)
    {
        D_TESTS_DB_CORE_CHECK( (_mariadb) &&
                               (e.error_code() == ER_ACCESS_DENIED) &&
                               (counting.is_connected()) );
    }

    return;
}

/*
d_tests_mariadb_core_lost
  A connection the server drops, or the client closes.
  Tests the following:
  - a connection another one KILLs: its next statement throws
    connection_exception, 2013 or 2006, and leaves it in error, which a
    statement prepared on it meets too; reconnect opens it again
  - the largest packet the client takes, max_packet_size: a prepared
    statement's values past it, which libmysqlclient refuses -- a
    query_exception, 2020, the connection kept -- and Connector/C sends; a
    row past it, which loses the connection: connection_exception, 2020 or
    2013, the connection in error; connection_config's 16 MiB reads 2 MiB
*/
template<typename Connection>
void
d_tests_mariadb_core_lost(
    const std::string& _database
)
{
    std::printf("a lost connection\n");

    // the largest packet the client takes
    {
        db::mysql_connect_config limited = family_config(_database);
        const std::string        big(2097152u, 'z');
        const bool               refuses =
            (!db::face::mariadb::client_is_mariadb());

        limited.base.max_packet_size = 1024u * 1024u;

        Connection cramped(limited);

        cramped.connect();

        auto measured = cramped.prepare("SELECT LENGTH(?)");

        measured->bind_string(1u, big);

        if (refuses)
        {
            D_TESTS_DB_CORE_THROWS((void)measured->execute_query(),
                                   db::query_exception,
                                   2020);
            D_TESTS_DB_CORE_CHECK( (cramped.is_connected()) &&
                                   (cramped.ping()) );
        }
        else
        {
            std::unique_ptr<db::buffered_result_set> length =
                measured->execute_query();

            D_TESTS_DB_CORE_CHECK( (length->next()) &&
                                   (length->get_long(0u).value_or(0) ==
                                    2097152) );
        }

        try
        {
            (void)cramped.execute_scalar("SELECT REPEAT('x', 2097152)");
            D_TESTS_DB_CORE_CHECK(false);
        }
        catch (const db::connection_exception& e)
        {
            // 2026 where the connection was closed under TLS, as
            // Connector/C before 3.4 reports it
            D_TESTS_DB_CORE_CHECK( (e.error_code() == 2020) ||
                                   (e.error_code() == 2013) ||
                                   (e.error_code() == 2026) );
        }

        D_TESTS_DB_CORE_CHECK( (cramped.get_state() ==
                                db::connection_state::error) &&
                               (!cramped.is_connected()) );
        D_TESTS_DB_CORE_THROWS_ANY((void)measured->execute_query(),
                                   db::exception);

        Connection roomy(family_config(_database));

        roomy.connect();
        D_TESTS_DB_CORE_CHECK(std::get<std::string>(*roomy.execute_scalar(
                                  "SELECT REPEAT('x', 2097152)")).size() ==
                              2097152u);
    }

    Connection db(family_config(_database));
    Connection killer(family_config(_database));

    db.connect();
    killer.connect();

    auto prepared = db.prepare("SELECT COUNT(*) FROM core_t");

    killer.execute("KILL " + std::to_string(db.get_thread_id()));

    try
    {
        db.execute("SELECT 1");
        D_TESTS_DB_CORE_CHECK(false);
    }
    catch (const db::connection_exception& e)
    {
        // 2026 where the server closed it under TLS, as Connector/C before
        // 3.4 reports it
        D_TESTS_DB_CORE_CHECK( (e.error_code() == 2013) ||
                               (e.error_code() == 2006) ||
                               (e.error_code() == 2026) );
    }

    D_TESTS_DB_CORE_CHECK( (db.get_state() == db::connection_state::error) &&
                           (!db.is_connected()) );
    D_TESTS_DB_CORE_THROWS_ANY((void)prepared->execute_query(),
                               db::exception);
    db.reconnect();
    D_TESTS_DB_CORE_CHECK(db.is_connected());

    return;
}

/*
d_tests_mariadb_core_leaf_mariadb
  mariadb_connection's configuration.
  Tests the following:
  - the default storage engine set within the connect; one the server lacks
    leaves the connection unopened, connection_exception, as does a name no
    identifier, refused before connecting (D_DB_INVALID_ARGUMENT)
  - the same through connect_async, which waits for none of it -- without
    TLS where the client cannot carry it so; a character set other than the
    layer's, carried by the handshake, the session's with both connects and
    every pairing
  - Galera's causal reads, where the server has the variable: wsrep_sync_wait
    is 1, after a reset too
  - the configuration read back
*/
void
d_tests_mariadb_core_leaf_mariadb(
    const std::string& _database,
    bool               _mariadb
)
{
    std::printf("mariadb_connection's configuration\n");

    db::mariadb_connect_config config(server_config(_database));

    config.default_storage_engine = "MyISAM";

    db::mariadb_connection engined(config);

    engined.connect();
    D_TESTS_DB_CORE_CHECK(engined.get_engine() == "MyISAM");
    D_TESTS_DB_CORE_CHECK(engined.get_mariadb_config().default_storage_engine ==
                          "MyISAM");
    // connected without waiting, a character set other than the layer's
    // with it
    {
        db::mariadb_connect_config latin1 = config;

        latin1.mysql_config.default_charset = "latin1";

        db::mariadb_connection blocking(latin1);
        db::mariadb_connection waiting =
            (d_tests_mariadb_core_carried())
                ? db::mariadb_connection(latin1)
                : d_tests_mariadb_core_untls(db::mariadb_connection(latin1));
        auto                   opening = waiting.connect_async();

        blocking.connect();
        core::drive(opening);

        for (db::mariadb_connection* each : { &blocking, &waiting })
        {
            D_TESTS_DB_CORE_CHECK( (each->is_connected()) &&
                                   (each->get_engine() == "MyISAM") );
            D_TESTS_DB_CORE_CHECK(std::get<std::string>(*each->execute_scalar(
                                      "SELECT @@character_set_client")) ==
                                  "latin1");
            D_TESTS_DB_CORE_CHECK(each->get_charset() == "latin1");
        }
    }

    config.default_storage_engine = "NoSuchEngine";

    db::mariadb_connection refused(config);
    db::mariadb_connection refused_async =
        (d_tests_mariadb_core_carried())
            ? db::mariadb_connection(config)
            : d_tests_mariadb_core_untls(db::mariadb_connection(config));
    auto                   refusing = refused_async.connect_async();

    D_TESTS_DB_CORE_THROWS_ANY(refused.connect(), db::connection_exception);
    D_TESTS_DB_CORE_CHECK(!refused.is_connected());
    D_TESTS_DB_CORE_THROWS_ANY(core::drive(refusing),
                               db::connection_exception);
    D_TESTS_DB_CORE_CHECK( (!refused_async.is_connected()) &&
                           (refused_async.get_state() ==
                            db::connection_state::error) );

    config.default_storage_engine = "No'Such";

    db::mariadb_connection unnamed(config);

    D_TESTS_DB_CORE_THROWS(unnamed.connect(),
                           db::connection_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_CHECK(!unnamed.is_connected());

    // Galera's variable, which MariaDB's builds with wsrep have
    if ( (_mariadb) &&
         (engined.execute_query(
              "SHOW VARIABLES LIKE 'wsrep_sync_wait'")->row_count() == 1) )
    {
        config.default_storage_engine.clear();
        config.galera_wsrep_sync_wait = true;

        db::mariadb_connection causal(config);

        causal.connect();
        D_TESTS_DB_CORE_CHECK(std::get<re_std::int64_t>(*causal.execute_scalar(
                                  "SELECT @@wsrep_sync_wait")) == 1);
        causal.reset_connection();
        D_TESTS_DB_CORE_CHECK(std::get<re_std::int64_t>(*causal.execute_scalar(
                                  "SELECT @@wsrep_sync_wait")) == 1);
    }

    return;
}

/*
d_tests_mariadb_core_leaf_mysql
  mysql_connection's configuration.
  Tests the following:
  - ssl_mode disabled connects without TLS; required with it, where the
    server has TLS
  - connect attributes, which MySQL's performance_schema shows
  - the TLS version and, where the client has the option, TLS 1.3's cipher
    suite allowed, each the one used where the server has TLS; the suites
    refused with D_DB_UNSUPPORTED where the client has no option
  - an authentication plugin to answer with first, and the clear-text
    switch, each taken
  - session tracking: every kind asked of a MySQL server, a change of
    sql_mode then reported, and asked again after a reset; a MariaDB server,
    without GTIDs to track, refusing it (1193)
  - what no client has, refused with D_DB_UNSUPPORTED: use_x_protocol, a
    statement cache
  - query attributes, where the server and the client have them: one goes
    with the next statement, read back where the server has the
    query_attributes component (installed for the test where it can be,
    and removed again), and clearing drops them; an empty name and a NUL
    are refused; where they do not, D_DB_UNSUPPORTED
*/
void
d_tests_mariadb_core_leaf_mysql(
    const std::string& _database,
    bool               _mariadb
)
{
    std::printf("mysql_connection's configuration\n");

    db::oracle_mysql_connect_config config(server_config(_database));

    config.ssl_mode = db::mysql_ssl_mode::disabled;
    config.connect_attributes["program_role"] = "core";

    db::mysql_connection plain(config);

    plain.connect();

    std::unique_ptr<db::buffered_result_set> cipher = plain.execute_query(
        "SHOW SESSION STATUS LIKE 'Ssl_cipher'");

    // without TLS -- which MariaDB's client, with no switch for DISABLED,
    // uses unasked from its 3.4
    D_TESTS_DB_CORE_CHECK(cipher->next());
    D_TESTS_DB_CORE_CHECK( ( (db::face::mariadb::client_is_mariadb()) &&
                             (db::face::mariadb::client_version_number() >=
                              30400ul) ) ||
                           (cipher->get_string(1u).value_or("x").empty()) );

    // the attribute, where performance_schema keeps it
    if (!_mariadb)
    {
        std::unique_ptr<db::buffered_result_set> attribute =
            plain.execute_query(
                "SELECT ATTR_VALUE "
                "FROM performance_schema.session_connect_attrs "
                "WHERE PROCESSLIST_ID = CONNECTION_ID() "
                "AND ATTR_NAME = 'program_role'");

        D_TESTS_DB_CORE_CHECK( (attribute->next()) &&
                               (attribute->get_string(0u).value_or("") ==
                                "core") );
    }

    // TLS required, where the server has it: the version and the cipher
    // suite allowed, the suite where the client has the option
    std::unique_ptr<db::buffered_result_set> have = plain.execute_query(
        "SHOW VARIABLES LIKE 'have_ssl'");
    const bool suites = ( (!db::face::mariadb::client_is_mariadb()) &&
                          (db::face::mariadb::client_version_number() >=
                           80016ul) );

    if ( (have->next()) &&
         (have->get_string(1u).value_or("") == "YES") )
    {
        config.ssl_mode = db::mysql_ssl_mode::required;

        db::mysql_connection secured(config);

        secured.connect();
        cipher = secured.execute_query("SHOW SESSION STATUS LIKE 'Ssl_cipher'");
        D_TESTS_DB_CORE_CHECK( (cipher->next()) &&
                               (!cipher->get_string(1u).value_or("").empty()) );

        config.tls_version = "TLSv1.2";

        db::mysql_connection versioned(config);

        versioned.connect();
        cipher = versioned.execute_query(
            "SHOW SESSION STATUS LIKE 'Ssl_version'");
        D_TESTS_DB_CORE_CHECK( (cipher->next()) &&
                               (cipher->get_string(1u).value_or("") ==
                                "TLSv1.2") );

        config.tls_version      = "TLSv1.3";
        config.tls_ciphersuites = "TLS_AES_128_GCM_SHA256";

        if (suites)
        {
            db::mysql_connection suited(config);

            suited.connect();
            cipher = suited.execute_query(
                "SHOW SESSION STATUS LIKE 'Ssl_cipher'");
            D_TESTS_DB_CORE_CHECK( (cipher->next()) &&
                                   (cipher->get_string(1u).value_or("") ==
                                    "TLS_AES_128_GCM_SHA256") );
        }

        config.tls_version.clear();
        config.tls_ciphersuites.clear();
    }

    // TLS 1.3's cipher suites, which only libmysqlclient has an option for
    config.ssl_mode         = db::mysql_ssl_mode::preferred;
    config.tls_ciphersuites = "TLS_AES_128_GCM_SHA256";

    if (!suites)
    {
        D_TESTS_DB_CORE_THROWS(db::mysql_connection(config).connect(),
                               db::connection_exception,
                               D_DB_UNSUPPORTED);
    }

    config.tls_ciphersuites.clear();

    // the plugin to answer with first, which the server's own replaces, and
    // the clear-text switch
    config.auth_plugin             = "caching_sha2_password";
    config.enable_cleartext_plugin = true;

    db::mysql_connection authenticated(config);

    authenticated.connect();
    D_TESTS_DB_CORE_CHECK(authenticated.is_connected());
    config.auth_plugin.clear();
    config.enable_cleartext_plugin = false;

    // every kind of session state, which a MariaDB server, without GTIDs to
    // track, refuses
    config.enable_session_tracking = true;

    if (_mariadb)
    {
        D_TESTS_DB_CORE_THROWS(db::mysql_connection(config).connect(),
                               db::connection_exception,
                               ER_UNKNOWN_SYSTEM_VARIABLE);
    }
    else
    {
        db::mysql_connection tracking(config);

        tracking.connect();
        D_TESTS_DB_CORE_CHECK(std::get<std::string>(*tracking.execute_scalar(
                                  "SELECT @@session_track_transaction_info"))
                              == "CHARACTERISTICS");
        tracking.execute("SET SESSION sql_mode = 'ANSI_QUOTES'");

        // the variable and its value, a pair among those every statement
        // changes (statement_id)
        const std::vector<std::string> changed =
            tracking.get_session_track_info(
                db::mysql_session_track::system_variables);
        bool                           found   = false;

        for (std::size_t i = 0u; (i + 1u) < changed.size(); i += 2u)
        {
            found = ( (found) ||
                      ( (changed[i] == "sql_mode") &&
                        (changed[i + 1u] == "ANSI_QUOTES") ) );
        }

        D_TESTS_DB_CORE_CHECK(found);

        // a reset asks for it again
        if (tracking.supports_reset_connection())
        {
            tracking.reset_connection();
            D_TESTS_DB_CORE_CHECK(std::get<std::string>(
                                      *tracking.execute_scalar(
                                          "SELECT @@session_track_gtids")) ==
                                  "OWN_GTID");
        }
    }

    config.enable_session_tracking = false;

    // what no client has
    config.use_x_protocol = true;
    D_TESTS_DB_CORE_THROWS(db::mysql_connection(config).connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    config.use_x_protocol       = false;
    config.statement_cache_size = 16u;
    D_TESTS_DB_CORE_THROWS(db::mysql_connection(config).connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    config.statement_cache_size = 0u;

    // query attributes
    db::mysql_connection tagged(family_config(_database));

    D_TESTS_DB_CORE_CHECK(!tagged.supports_query_attributes());
    tagged.connect();

    const bool can =
        tagged.get_face().supports(D_MARIADB_FEATURE_QUERY_ATTRIBUTES);

    D_TESTS_DB_CORE_CHECK(tagged.supports_query_attributes() == can);

    if (can)
    {
        const char* const component = "'file://component_query_attributes'";
        const char* const read      =
            "SELECT mysql_query_attribute_string('core_a')";
        const bool        had       =
            (std::get<re_std::int64_t>(*tagged.execute_scalar(
                 std::string("SELECT COUNT(*) FROM mysql.component "
                             "WHERE component_urn = ") + component)) == 1);
        bool              made      = false;

        // the component, where the test may install it
        if (!had)
        {
            try
            {
                tagged.execute(std::string("INSTALL COMPONENT ") + component);
                made = true;
            }
            catch (const db::query_exception&)
            {
                std::printf("    no query_attributes component: the values "
                            "go unread\n");
            }
        }

        tagged.set_query_attribute("core_a", "hi");
        D_TESTS_DB_CORE_CHECK( (!(had || made)) ||
                               (std::get<std::string>(*tagged.execute_scalar(
                                    read)) == "hi") );
        D_TESTS_DB_CORE_CHECK( (!(had || made)) ||
                               (std::holds_alternative<std::monostate>(
                                    *tagged.execute_scalar(read))) );
        tagged.set_query_attribute("core_a", "x");
        tagged.clear_query_attributes();
        D_TESTS_DB_CORE_CHECK( (!(had || made)) ||
                               (std::holds_alternative<std::monostate>(
                                    *tagged.execute_scalar(read))) );
        D_TESTS_DB_CORE_THROWS(tagged.set_query_attribute("", "x"),
                               db::query_exception,
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_DB_CORE_THROWS(
            tagged.set_query_attribute(std::string("a\0b", 3), "x"),
            db::query_exception,
            D_DB_INVALID_ARGUMENT);

        if (made)
        {
            tagged.execute(std::string("UNINSTALL COMPONENT ") + component);
        }
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(tagged.set_query_attribute("core_a", "x"),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        tagged.clear_query_attributes();
        D_TESTS_DB_CORE_CHECK(tagged.is_connected());
    }

    return;
}

/*
d_tests_mariadb_core_support
  What the server connected has, asked of each leaf at run time.
  Tests the following:
  - before connecting, nothing
  - each supports_* by the server's flavor and release, as the releases that
    brought each in say: on MariaDB, JSON (10.2.7) as text, INET6 (10.5.0),
    UUID (10.7.0), sequences (10.3.0), system versioning (10.3.4), RETURNING
    (10.5.0), Galera (10.1.0), roles (10.0.5), atomic DDL (10.6.1), and none
    of MySQL's own; on MySQL, JSON (5.7.8) as binary, the X Protocol
    (5.7.12), Group Replication and InnoDB Cluster (5.7.17), window
    functions (8.0.2), CTEs (8.0.1), roles and atomic DDL (8.0.0), and none
    of MariaDB's own; each flavor's modern SQL; a long-term-support series
  - the compile-time struct by its name, and by its former names
*/
void
d_tests_mariadb_core_support(
    const std::string& _database,
    bool               _mariadb
)
{
    std::printf("support\n");

    db::mariadb_connection maria(family_config(_database));
    db::mysql_connection   oracle(family_config(_database));

    D_TESTS_DB_CORE_CHECK( (!maria.supports_json()) &&
                           (!maria.supports_modern_sql()) &&
                           (!oracle.supports_cte()) &&
                           (!oracle.is_lts_release()) );
    maria.connect();
    oracle.connect();

    const unsigned long version = maria.get_face().server_version_number();
    const unsigned long series  = version / 100ul;

    // as each flavor's releases brought them
    if (_mariadb)
    {
        D_TESTS_DB_CORE_CHECK( (maria.supports_json() ==
                                (version >= 100207ul)) &&
                               (!maria.json_is_native()) &&
                               (maria.supports_inet6_type() ==
                                (version >= 100500ul)) &&
                               (maria.supports_uuid_type() ==
                                (version >= 100700ul)) &&
                               (maria.supports_sequences() ==
                                (version >= 100300ul)) &&
                               (maria.supports_system_versioning() ==
                                (version >= 100304ul)) &&
                               (maria.supports_returning() ==
                                (version >= 100500ul)) &&
                               (maria.supports_galera() ==
                                (version >= 100100ul)) &&
                               (maria.supports_modern_sql() ==
                                (version >= 100304ul)) );
        D_TESTS_DB_CORE_CHECK( (!oracle.supports_x_protocol()) &&
                               (!oracle.supports_group_replication()) &&
                               (!oracle.supports_innodb_cluster()) &&
                               (oracle.supports_roles() ==
                                (version >= 100005ul)) &&
                               (oracle.supports_atomic_ddl() ==
                                (version >= 100601ul)) &&
                               (oracle.supports_modern_sql() ==
                                (version >= 100304ul)) &&
                               (oracle.is_lts_release() ==
                                ( (series == 1006ul) || (series == 1011ul) ||
                                  (series == 1104ul) || (series == 1108ul) ||
                                  (series == 1203ul) )) );
    }
    else
    {
        D_TESTS_DB_CORE_CHECK( (oracle.supports_json() ==
                                (version >= 50708ul)) &&
                               (oracle.supports_x_protocol() ==
                                (version >= 50712ul)) &&
                               (oracle.supports_group_replication() ==
                                (version >= 50717ul)) &&
                               (oracle.supports_innodb_cluster() ==
                                (version >= 50717ul)) &&
                               (oracle.supports_window_functions() ==
                                (version >= 80002ul)) &&
                               (oracle.supports_cte() ==
                                (version >= 80001ul)) &&
                               (oracle.supports_roles() ==
                                (version >= 80000ul)) &&
                               (oracle.supports_atomic_ddl() ==
                                (version >= 80000ul)) &&
                               (oracle.supports_modern_sql() ==
                                (version >= 80014ul)) &&
                               (oracle.is_lts_release() ==
                                ( (series == 804ul) ||
                                  (series == 907ul) )) );
        D_TESTS_DB_CORE_CHECK( (maria.json_is_native() ==
                                (version >= 50708ul)) &&
                               (!maria.supports_inet6_type()) &&
                               (!maria.supports_uuid_type()) &&
                               (!maria.supports_sequences()) &&
                               (!maria.supports_system_versioning()) &&
                               (!maria.supports_returning()) &&
                               (!maria.supports_galera()) );
    }

    maria.disconnect();
    D_TESTS_DB_CORE_CHECK(!maria.supports_json());

    // the compile-time struct, and its former names
    static_assert(std::is_same_v<db::mysql_connection::client_capabilities,
                                 db::mysql_client_capabilities>,
                  "client_capabilities is the compile-time struct");
    static_assert(std::is_same_v<db::mariadb_connection::client_capabilities,
                                 db::mariadb_client_capabilities>,
                  "client_capabilities is the compile-time struct");
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    static_assert( (std::is_same_v<db::mysql_type_support,
                                   db::mysql_client_capabilities>) &&
                   (std::is_same_v<db::mysql_feature_support,
                                   db::mysql_client_capabilities>) &&
                   (std::is_same_v<db::mariadb_type_support,
                                   db::mariadb_client_capabilities>) &&
                   (std::is_same_v<db::mariadb_feature_support,
                                   db::mariadb_client_capabilities>),
                  "the former names are the struct's");
#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

    return;
}

/*
d_tests_mariadb_core_common_table
  What mysql_common_table adds, through Table, a leaf's table class, on a
connection to its database.
  Tests the following:
  - a table created with the engine named, which SHOW CREATE TABLE shows,
    the session's default engine put back; one the server lacks refused,
    the default put back too
  - MODIFY COLUMN through the layer: the type and default replaced, the
    cache read again; refused while the cache is dirty
*/
template<typename Table,
         typename Connection>
void
d_tests_mariadb_core_common_table(
    Connection& _db
)
{
    std::printf("mysql_common_table\n");

    const std::optional<db::value> engine =
        _db.execute_scalar("SELECT @@default_storage_engine");
    db::table_schema               schema;

    schema.table_name = "core_engine";
    schema.columns    = { db::column_info("id",
                                          db::field_type::integer,
                                          false,
                                          true),
                          db::column_info("score", db::field_type::integer) };

    Table engined(_db, schema);

    engined.drop_table();
    D_TESTS_DB_CORE_CHECK(engined.get_engine().empty());
    engined.set_engine("MyISAM");
    engined.create_table();
    D_TESTS_DB_CORE_CHECK( (engined.show_create_table().find(
                                "ENGINE=MyISAM") != std::string::npos) &&
                           (_db.execute_scalar(
                                "SELECT @@default_storage_engine") ==
                            engine) );

    db::table_schema refused_schema = schema;

    refused_schema.table_name = "core_refused";

    Table refused(_db, refused_schema);

    refused.set_engine("NoSuchEngine");
    D_TESTS_DB_CORE_THROWS_ANY(refused.create_table(), db::query_exception);
    D_TESTS_DB_CORE_CHECK(_db.execute_scalar(
                              "SELECT @@default_storage_engine") == engine);

    // a column redefined, the rows read again
    Table changed(_db, std::string("core_engine"));
    db::column_info wide("score", db::field_type::big_integer);

    wide.default_value = "5";
    changed.fetch_schema();
    changed.add_row();
    changed.update_cell(0u, 0u, core::int_value(1));
    changed.commit();
    changed.add_row();
    D_TESTS_DB_CORE_THROWS(changed.alter_modify_column(wide),
                           db::query_exception,
                           D_DB_UNSUPPORTED);
    changed.refresh();
    changed.alter_modify_column(wide);
    D_TESTS_DB_CORE_CHECK( (changed.show_create_table().find("bigint") !=
                            std::string::npos) &&
                           (changed.column_type(1u) ==
                            db::field_type::big_integer) &&
                           (changed.rows() == 1u) );
    D_TESTS_DB_CORE_THROWS_ANY(changed.alter_modify_column(
                                   db::column_info("none")),
                               std::out_of_range);
    changed.drop_table();

    return;
}

/*
d_tests_mariadb_core_mysql_table
  mysql_table, on a mysql_connection to `_database`.
  Tests the following:
  - a JSON column told apart where the server has the type, MySQL's; text,
    and no JSON column, on MariaDB
  - an optimizer hint in refresh's query, which MySQL reads and MariaDB
    reads as a comment; one holding the comment's end refused
  - ANALYZE, OPTIMIZE and CHECK TABLE through the layer
*/
void
d_tests_mariadb_core_mysql_table(
    const std::string& _database,
    bool               _mariadb
)
{
    std::printf("mysql_table\n");

    db::mysql_connection db(family_config(_database));

    db.connect();
    db.execute("CREATE TABLE core_json (id INT PRIMARY KEY, doc JSON)");
    db.execute("INSERT INTO core_json VALUES (1, '{\"a\": 1}')");

    db::mysql_table<> documents(db, std::string("core_json"));

    documents.fetch_schema();
    D_TESTS_DB_CORE_CHECK( (documents.is_json_column(1u) == !_mariadb) &&
                           (documents.is_json_column("doc") == !_mariadb) &&
                           (!documents.is_json_column(0u)) &&
                           (!documents.is_json_column(9u)) &&
                           (db::mysql_table<>::json_is_native()) );
    documents.set_optimizer_hint("MAX_EXECUTION_TIME(10000)");
    D_TESTS_DB_CORE_CHECK( (documents.is_stale()) &&
                           (documents.get_optimizer_hint() ==
                            "MAX_EXECUTION_TIME(10000)") );
    documents.refresh();
    D_TESTS_DB_CORE_CHECK(documents.rows() == 1u);
    D_TESTS_DB_CORE_THROWS(documents.set_optimizer_hint("a */ b"),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    documents.clear_optimizer_hint();
    D_TESTS_DB_CORE_CHECK(documents.get_optimizer_hint().empty());

    // upkeep
    documents.analyze_table();
    documents.optimize_table();
    D_TESTS_DB_CORE_CHECK(documents.check_table());
    d_tests_mariadb_core_common_table<db::mysql_table<>>(db);

    return;
}

/*
d_tests_mariadb_core_mariadb_table
  mariadb_table, on a mariadb_connection to `_database`.
  Tests the following:
  - what the server supports, by its flavor and version
  - a system-versioned table, where the server has them: told so by
    fetch_schema, read as of a time before it held a row and after, and in
    every version; a commit refused after a read from the history; a table
    that keeps none refused; on MySQL, no history
  - RETURNING, where the server has it: a row inserted and returned with the
    key it was given, the cache taking it; rows deleted and returned, the
    cache losing them; before MariaDB 10.5.0 and on MySQL, D_DB_UNSUPPORTED
  - a commit with Galera's causal reads asked for, which a server without
    Galera takes as any commit
*/
void
d_tests_mariadb_core_mariadb_table(
    const std::string& _database,
    bool               _mariadb
)
{
    using clock = std::chrono::system_clock;

    std::printf("mariadb_table\n");

    db::mariadb_connection db(family_config(_database));

    db.connect();

    const bool returning = db.get_face().supports(D_MARIADB_FEATURE_RETURNING);
    const bool versioned =
        db.get_face().supports(D_MARIADB_FEATURE_SYSTEM_VERSIONING);

    db.execute("CREATE TABLE core_plainv (id INT PRIMARY KEY, v INT)");

    db::mariadb_table<> plain(db, std::string("core_plainv"));

    plain.fetch_schema();
    D_TESTS_DB_CORE_CHECK( (plain.has_returning_support() == returning) &&
                           (plain.has_system_versioning_support() ==
                            versioned) &&
                           (plain.has_sequences_support() == _mariadb) &&
                           (plain.has_uuid_type_support() ==
                            db.get_face().server_at_least(
                                D_MARIADB_FLAVOR_MARIADB, 10u, 7u, 0u)) &&
                           (!plain.is_system_versioned()) );
    D_TESTS_DB_CORE_THROWS(plain.refresh_all_versions(),
                           db::query_exception,
                           D_DB_UNSUPPORTED);

    // the history, where the server keeps one
    if (versioned)
    {
        db.execute("CREATE TABLE core_ver (id INT PRIMARY KEY, v INT) "
                   "WITH SYSTEM VERSIONING");
        db.execute("INSERT INTO core_ver VALUES (1, 10)");
        db.execute("UPDATE core_ver SET v = 11 WHERE id = 1");

        db::mariadb_table<> history(db, std::string("core_ver"));

        history.fetch_schema();
        D_TESTS_DB_CORE_CHECK(history.is_system_versioned());
        history.refresh_all_versions();
        D_TESTS_DB_CORE_CHECK(history.rows() == 2u);
        history.refresh_as_of(clock::time_point(std::chrono::hours(48)));
        D_TESTS_DB_CORE_CHECK(history.rows() == 0u);
        history.refresh_as_of(clock::now() + std::chrono::hours(1));
        D_TESTS_DB_CORE_CHECK( (history.rows() == 1u) &&
                               (core::number(history.cell(0u, 1u)) == 11) );
        history.refresh_between(clock::time_point(std::chrono::hours(48)),
                                clock::now() + std::chrono::hours(1));
        D_TESTS_DB_CORE_CHECK(history.rows() == 2u);
        history.refresh_as_of(std::string("TIMESTAMP'1971-01-01 00:00:00'"));
        D_TESTS_DB_CORE_CHECK(history.rows() == 0u);
        history.refresh_all_versions();
        history.update_cell(0u, 1u, core::int_value(12));
        D_TESTS_DB_CORE_THROWS(history.commit(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        history.refresh();
        D_TESTS_DB_CORE_CHECK(history.rows() == 1u);
        history.update_cell(0u, 1u, core::int_value(12));
        history.commit();
        D_TESTS_DB_CORE_CHECK(core::scalar(db, "SELECT v FROM core_ver") ==
                              12);
    }

    // RETURNING, where the server has it
    if (returning)
    {
        plain.refresh();
        plain.insert_row({ core::int_value(1), core::int_value(5) });
        plain.commit();

        const db::database_table<db::mariadb_connection>::row_type made =
            plain.insert_row_returning({ core::int_value(2),
                                         core::int_value(6) });

        D_TESTS_DB_CORE_CHECK( (made.size() == 2u) &&
                               (core::number(made[0]) == 2) &&
                               (plain.rows() == 2u) &&
                               (!plain.is_dirty()) );

        const std::vector<db::database_table<db::mariadb_connection>::
                              row_type> gone = plain.delete_returning("v = 5");

        D_TESTS_DB_CORE_CHECK( (gone.size() == 1u) &&
                               (core::number(gone[0][0]) == 1) &&
                               (plain.rows() == 1u) &&
                               (core::scalar(db, "SELECT COUNT(*) FROM "
                                                 "core_plainv") == 1) );
        plain.update_cell(0u, 1u, core::int_value(7));
        plain.commit();
        D_TESTS_DB_CORE_CHECK(core::scalar(db, "SELECT v FROM core_plainv") ==
                              7);
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(
            (void)plain.insert_row_returning({ core::int_value(2),
                                               core::int_value(6) }),
            db::query_exception,
            D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_THROWS((void)plain.delete_returning("v = 5"),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }

    // Galera's causal reads, asked for
    plain.set_galera_sync_on_commit(true);
    D_TESTS_DB_CORE_CHECK(plain.is_galera_sync_on_commit());
    plain.refresh();
    plain.add_row();
    plain.update_cell(plain.rows() - 1u, 0u, core::int_value(9));
    plain.commit();
    D_TESTS_DB_CORE_CHECK(core::scalar(db, "SELECT COUNT(*) FROM core_plainv "
                                           "WHERE id = 9") == 1);
    d_tests_mariadb_core_common_table<db::mariadb_table<>>(db);

    return;
}

/*
d_tests_mariadb_core_tls_refused
  Where the client cannot carry TLS without blocking.
  Tests the following:
  - connect_async that would ask for TLS -- the layer's default -- throws
    connection_exception with D_DB_UNSUPPORTED, the connection in error;
    told not to use TLS, it connects
  - over a connection under TLS, an operation throws query_exception with
    D_DB_UNSUPPORTED, and the connection carries on by blocking
*/
template<typename Connection>
void
d_tests_mariadb_core_tls_refused(
    Connection&        _db,
    const std::string& _database
)
{
    std::printf("operations over TLS: refused, the rest without it\n");

    Connection asking(family_config(_database));
    Connection plain = d_tests_mariadb_core_untls(
                           Connection(family_config(_database)));
    auto       refused = asking.connect_async();
    auto       opening = plain.connect_async();

    D_TESTS_DB_CORE_THROWS(core::drive(refused),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_CHECK(asking.get_state() == db::connection_state::error);
    core::drive(opening);
    D_TESTS_DB_CORE_CHECK( (plain.is_connected()) &&
                           (!d_tests_mariadb_core_under_tls(plain)) );

    // under TLS, where the server offers it
    if (d_tests_mariadb_core_under_tls(_db))
    {
        auto select = _db.execute_query_async("SELECT 1");

        D_TESTS_DB_CORE_THROWS((void)core::drive(select),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_CHECK(_db.execute("SELECT 1"));
    }

    return;
}

/*
d_tests_mariadb_core_leaf
  One leaf, start to end, in a database of its own; the operations that do
not block without TLS where the client cannot carry it so.
*/
template<typename Connection>
void
d_tests_mariadb_core_leaf(
    const std::string& _database,
    bool               _mariadb
)
{
    std::printf("%s\n",
                (Connection::is_mariadb()) ? "== mariadb_connection"
                                           : "== mysql_connection");

    const core::dialect dialect = make_dialect(_mariadb);
    Connection          db(family_config(_database));

    db.connect();
    core::queries(db, dialect);
    core::statements(db, dialect);
    core::transactions(db);
    core::errors(db, dialect);
    core::streams(db, dialect);

    // the operations that do not block: over the leaf's connection, or,
    // where the client cannot carry TLS so, over one without it
    const bool carried = d_tests_mariadb_core_carried();
    Connection untls   = (carried)
                             ? Connection(family_config(_database))
                             : d_tests_mariadb_core_untls(
                                   Connection(family_config(_database)));

    if (!carried)
    {
        d_tests_mariadb_core_tls_refused(db, _database);
        untls.connect();
    }

    core::async_operations(
        (carried) ? db : untls,
        dialect,
        [&_database, carried]() -> Connection
        {
            return (carried) ? Connection(family_config(_database))
                             : d_tests_mariadb_core_untls(
                                   Connection(family_config(_database)));
        },
        [&_database, carried]() -> Connection
        {
            db::mysql_connect_config wrong = family_config(_database);

            wrong.base.password = "certainly not the password";

            return (carried) ? Connection(wrong)
                             : d_tests_mariadb_core_untls(Connection(wrong));
        },
        db::face::mariadb::client_is_mariadb());

    // the client's names for the two ways of reading rows
    {
        auto stored = db.store_result("SELECT 1 UNION ALL SELECT 2");
        auto used   = db.use_result("SELECT 1 UNION ALL SELECT 2", 1u);

        D_TESTS_DB_CORE_CHECK(stored->row_count() == 2);
        D_TESTS_DB_CORE_THROWS((void)db.store_result("SELECT 3"),
                               db::query_exception,
                               2014);
        D_TESTS_DB_CORE_CHECK( (used->next()) && (used->next()) &&
                               (!used->next()) && (used->row_count() == 2) );
        D_TESTS_DB_CORE_CHECK(db.store_result("SELECT 3")->row_count() == 1);
    }

    // an option read back, as the connect set it
    {
        // MYSQL_OPT_CONNECT_TIMEOUT: mysql.h's first option in both
        // clients, which the core leaves out of view
        const int                CONNECT_TIMEOUT = 0;
        db::mysql_connect_config timed           = family_config(_database);
        unsigned int             seconds         = 0u;

        timed.base.connect_timeout = std::chrono::seconds(9);

        Connection reading(timed);

        reading.connect();
        reading.get_option(CONNECT_TIMEOUT, &seconds);
        D_TESTS_DB_CORE_CHECK(seconds == 9u);
        D_TESTS_DB_CORE_THROWS(reading.get_option(-1, &seconds),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_THROWS(reading.get_option(CONNECT_TIMEOUT, nullptr),
                               db::query_exception,
                               D_DB_INVALID_ARGUMENT);
    }
    core::lifetimes<Connection>(
        [&_database]() { return Connection(family_config(_database)); });
    core::tables<db::database_table<Connection>>(
        db,
        dialect,
        core::table_abilities
        {
            db.get_face().supports(D_MARIADB_FEATURE_RENAME_COLUMN),
            true,
            true
        });
    d_tests_mariadb_core_family(db, _database);
    d_tests_mariadb_core_session<Connection>(_database);
    d_tests_mariadb_core_connecting<Connection>(_database, _mariadb);
    d_tests_mariadb_core_lost<Connection>(_database);

    return;
}

/*
d_tests_mariadb_core_stubs
  Over a layer built without a client.
  Tests the following:
  - connect throws connection_exception with D_DB_UNSUPPORTED, for both
    leaves, and leaves the connection in error
*/
void
d_tests_mariadb_core_stubs(
    void
)
{
    std::printf("over the stubs\n");

    db::mariadb_connection mariadb(server_config(""));
    db::mysql_connection   mysql(server_config(""));

    D_TESTS_DB_CORE_THROWS(mariadb.connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_THROWS(mysql.connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_CHECK( (mariadb.get_state() ==
                            db::connection_state::error) &&
                           (!mysql.is_connected()) );
    D_TESTS_DB_CORE_THROWS_ANY(mysql.execute("SELECT 1"),
                               db::connection_exception);

    return;
}

}  // namespace

/*
main
  Over a client, each leaf where a server answers, in databases of the run's
own, dropped at the end; over the stubs, what the stubs report.
*/
int
main(
    void
)
{
    const bool stubs    = (db::face::mariadb::client_version_number() == 0ul);
    const bool required =
        (core::setting("DJINTERP_TEST_MARIADB_REQUIRED", "0") == "1");

    // over the stubs
    if (stubs)
    {
        d_tests_mariadb_core_stubs();

        return core::summary("built without a client");
    }

    db::mariadb_connection setup(family_config(""));

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

    const bool        mariadb = (setup.get_server_version().find("MariaDB") !=
                                 std::string::npos);
    const std::string run     = std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count() %
        100000000);
    const std::string first   = "d_tests_core_" + run + "_m";
    const std::string second  = "d_tests_core_" + run + "_o";

    fresh_database(setup, first);
    fresh_database(setup, second);
    d_tests_mariadb_core_leaf<db::mariadb_connection>(first, mariadb);
    d_tests_mariadb_core_leaf<db::mysql_connection>(second, mariadb);
    d_tests_mariadb_core_leaf_mariadb(first, mariadb);
    d_tests_mariadb_core_leaf_mysql(second, mariadb);
    d_tests_mariadb_core_support(first, mariadb);
    d_tests_mariadb_core_mariadb_table(first, mariadb);
    d_tests_mariadb_core_mysql_table(second, mariadb);
    setup.execute("DROP DATABASE " + first);
    setup.execute("DROP DATABASE " + second);

    return core::summary(setup.get_server_version() + ", client " +
                         db::face::mariadb::client_version());
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
