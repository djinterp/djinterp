/*******************************************************************************
* djinterp [core]                                             t_mariadb_core.cpp
*
*   core/db's MySQL-family connections, mariadb_connection and
* mysql_connection, over the MariaDB layer, against a live server -- MariaDB
* from 10.3 or MySQL from 5.7, through whichever client the layer is built
* with. Each leaf runs the operations every vendor shares (t_db_core.hpp) and
* the family's own members (mysql_common_connection's), in a database of its
* own, database_table among them, and then its own configuration: MariaDB's
* storage engine and Galera's causal reads, MySQL's TLS modes, connect
* attributes and the settings it refuses; and its table class, mariadb_table
* or mysql_table, with what mysql_common_table adds to both. Where an answer
* depends on the server, the check asks it.
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
* path:      /tests/djinterp/core/db/mariadb/t_mariadb_core.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
// djinterp
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER
// std
#include <cstdio>  // std::printf

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>   // std::chrono::system_clock
#include <cstdlib>  // std::strtoul
#include <string>   // std::string, std::to_string
#include <vector>   // std::vector
// djinterp
#include "../../../../../inc/djinterp/core/db/mariadb/mariadb.hpp"  // under
#include "../../../../../inc/djinterp/core/db/mysql/mysql.hpp"      // test
#include "../../../../../inc/djinterp/core/db/mariadb/mariadb_table.hpp"
#include "../../../../../inc/djinterp/core/db/mysql/mysql_table.hpp"
#include "../t_db_core.hpp"  // the shared checks


namespace
{

namespace db   = djinterp;
namespace core = d_tests_db_core;

// ER_* -- the server errors the checks compare with, as both servers number
// them
const int ER_ACCESS_DENIED  = 1045;
const int ER_DUP_ENTRY      = 1062;
const int ER_PARSE_ERROR    = 1064;

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
        &unique_violation
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
  - use_database names the current database; get_stat, the connection's
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
    D_TESTS_DB_CORE_CHECK(_db.get_database_name() == "information_schema");
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
  A connection the server drops.
  Tests the following:
  - a connection another one KILLs: its next statement throws
    connection_exception, 2013 or 2006, and leaves it in error, which a
    statement prepared on it meets too; reconnect opens it again
*/
template<typename Connection>
void
d_tests_mariadb_core_lost(
    const std::string& _database
)
{
    std::printf("a lost connection\n");

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
        D_TESTS_DB_CORE_CHECK( (e.error_code() == 2013) ||
                               (e.error_code() == 2006) );
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
  - the default storage engine set once connected; one the server lacks
    leaves the connection unopened, connection_exception
  - Galera's causal reads, where the server has the variable: wsrep_sync_wait
    is 1
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
    config.default_storage_engine = "No'Such";

    db::mariadb_connection refused(config);

    D_TESTS_DB_CORE_THROWS_ANY(refused.connect(), db::connection_exception);
    D_TESTS_DB_CORE_CHECK(!refused.is_connected());

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
  - the settings the layer cannot apply, refused with D_DB_UNSUPPORTED:
    tls_version, auth_plugin, use_x_protocol
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

    // TLS required, where the server has it
    std::unique_ptr<db::buffered_result_set> have = plain.execute_query(
        "SHOW VARIABLES LIKE 'have_ssl'");

    if ( (have->next()) &&
         (have->get_string(1u).value_or("") == "YES") )
    {
        config.ssl_mode = db::mysql_ssl_mode::required;

        db::mysql_connection secured(config);

        secured.connect();
        cipher = secured.execute_query("SHOW SESSION STATUS LIKE 'Ssl_cipher'");
        D_TESTS_DB_CORE_CHECK( (cipher->next()) &&
                               (!cipher->get_string(1u).value_or("").empty()) );
    }

    // what the layer cannot apply
    config.ssl_mode    = db::mysql_ssl_mode::preferred;
    config.tls_version = "TLSv1.3";
    D_TESTS_DB_CORE_THROWS(db::mysql_connection(config).connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    config.tls_version.clear();
    config.auth_plugin = "caching_sha2_password";
    D_TESTS_DB_CORE_THROWS(db::mysql_connection(config).connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    config.auth_plugin.clear();
    config.use_x_protocol = true;
    D_TESTS_DB_CORE_THROWS(db::mysql_connection(config).connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);

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
d_tests_mariadb_core_leaf
  One leaf, start to end, in a database of its own.
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
