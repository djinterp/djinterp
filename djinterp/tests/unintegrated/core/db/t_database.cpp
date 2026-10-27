/*******************************************************************************
* djinterp [core]                                                 t_database.cpp
*
*   core/db's vocabulary and utilities, database.hpp's, which every vendor
* shares and no server is needed for: the exceptions and the refusals, the
* configuration's defaults, connection strings in each form, read and
* written, quoting and escaping, values as text, names, and the connection
* pool over stand-in connections. The vendors'
* harnesses (t_sqlite_core.cpp, t_mariadb_core.cpp, t_postgres_core.cpp)
* test the connections.
*   Below C++17 core/db compiles to nothing, and so does this test.
*
*   Build: compile src/djinterp/core/db/database.cpp and this file as C++17
* or later, with c/db/db_common.c as C -- the database CI scripts do, at
* each level from C++17.
*
*
* path:      /tests/unintegrated/core/db/t_database.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.26
*******************************************************************************/
// djinterp
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER
// std
#include <cstdio>  // std::printf

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>     // std::chrono::system_clock, milliseconds, seconds
#include <cmath>      // std::isnan, std::isinf
#include <limits>     // std::numeric_limits
#include <memory>     // std::unique_ptr, std::make_unique
#include <stdexcept>  // std::runtime_error
#include <string>     // std::string
#include <thread>     // std::this_thread::sleep_for
#include <variant>    // std::get, std::holds_alternative
#include <vector>     // std::vector
// djinterp
#include "../../../../inc/djinterp/core/db/database.hpp"  // under test


namespace db = djinterp;

namespace
{

int g_checks   = 0;
int g_failures = 0;

/*
d_tests_database_expect
  Counts one check, and prints it when it fails.
*/
void
d_tests_database_expect(
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

// D_TESTS_DATABASE_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_DATABASE_CHECK(_cond)                                         \
    d_tests_database_expect((_cond), #_cond, __LINE__)

// D_TESTS_DATABASE_THROWS
//   macro: records a check that a statement throws the exception named, with
// the error code expected.
#define D_TESTS_DATABASE_THROWS(_statement, _exception, _code)                \
    do                                                                        \
    {                                                                         \
        bool thrown_ = false;                                                 \
                                                                              \
        try                                                                   \
        {                                                                     \
            _statement;                                                       \
        }                                                                     \
        catch (const _exception& e_)                                          \
        {                                                                     \
            thrown_ = (e_.error_code() == (_code));                           \
        }                                                                     \
                                                                              \
        d_tests_database_expect(thrown_,                                      \
                                #_statement " throws " #_exception,           \
                                __LINE__);                                    \
    } while (0)

namespace
{

/*
d_tests_database_exceptions
  database.hpp's exceptions, and the refusals of text a NUL would cut short.
  Tests the following:
  - each exception keeps its message and its code, 0 by default, and is an
    exception, and a std::runtime_error, as well as itself
  - refuse_nul throws query_exception, and refuse_nul_setting
    connection_exception, each with D_DB_INVALID_ARGUMENT, naming the
    operation and what was refused; text without a NUL passes
*/
void
d_tests_database_exceptions(
    void
)
{
    std::printf("exceptions and refusals\n");

    const db::query_exception      query("q", 42);
    const db::connection_exception connection(std::string("c"));

    D_TESTS_DATABASE_CHECK( (std::string(query.what()) == "q") &&
                            (query.error_code() == 42) );
    D_TESTS_DATABASE_CHECK(connection.error_code() == 0);
    D_TESTS_DATABASE_CHECK(
        (dynamic_cast<const db::exception*>(&query) != nullptr) &&
        (dynamic_cast<const std::runtime_error*>(&connection) != nullptr));

    try
    {
        throw db::type_conversion_exception("t", D_DB_MISMATCH);
    }
    catch (const db::exception& e)
    {
        D_TESTS_DATABASE_CHECK(e.error_code() == D_DB_MISMATCH);
    }

    D_TESTS_DATABASE_THROWS(
        db::internal::refuse_nul(std::string("a\0b", 3), "op", "the name"),
        db::query_exception,
        D_DB_INVALID_ARGUMENT);
    D_TESTS_DATABASE_THROWS(
        db::internal::refuse_nul_setting(std::string("\0", 1), "op", "host"),
        db::connection_exception,
        D_DB_INVALID_ARGUMENT);

    try
    {
        db::internal::refuse_nul(std::string("x\0", 2), "execute", "the SQL");
        D_TESTS_DATABASE_CHECK(false);
    }
    catch (const db::query_exception& e)
    {
        D_TESTS_DATABASE_CHECK(std::string(e.what()) ==
                               "execute: the SQL holds a NUL character");
    }

    db::internal::refuse_nul("plain", "op");
    db::internal::refuse_nul_setting("plain", "op", "host");
    D_TESTS_DATABASE_CHECK(true);

    return;
}

/*
d_tests_database_configuration
  connection_config's defaults.
  Tests the following:
  - localhost, the vendor's port, no database or user or password
  - a 30-second connect timeout; no read or write timeout
  - no automatic reconnection, no TLS asked for, verification when it is,
    16 MiB packets, and no custom options
*/
void
d_tests_database_configuration(
    void
)
{
    std::printf("configuration\n");

    const db::connection_config config;

    D_TESTS_DATABASE_CHECK( (config.host == "localhost") &&
                            (config.port == 0u)          &&
                            (config.database.empty())    &&
                            (config.username.empty())    &&
                            (config.password.empty()) );
    D_TESTS_DATABASE_CHECK( (!config.charset) &&
                            (!config.schema)  &&
                            (!config.ssl_ca) );
    D_TESTS_DATABASE_CHECK(config.connect_timeout ==
                           std::chrono::seconds(30));
    D_TESTS_DATABASE_CHECK( (config.read_timeout.count() == 0) &&
                            (config.write_timeout.count() == 0) );
    D_TESTS_DATABASE_CHECK( (!config.auto_reconnect) &&
                            (!config.enable_ssl)     &&
                            (config.verify_ssl) );
    D_TESTS_DATABASE_CHECK(config.max_packet_size == 16u * 1024u * 1024u);
    D_TESTS_DATABASE_CHECK(config.custom_options.empty());

    return;
}

/*
d_tests_database_uris
  parse_connection_string's URI form.
  Tests the following:
  - user, password, host, port and database, percent-decoded; an IPv6 host
    in brackets; the query's pairs as keywords, sslmode among them
  - the scheme in custom_options, under "scheme"; a key no field has there
  - a host list kept whole, ports and all; an empty host
  - SQLite's file after `sqlite://`, `sqlite:` or `sqlite::memory:`
  - a URI without `//`, an unclosed bracket, a bad port, a pair without
    `=` and a malformed escape, each D_DB_INVALID_ARGUMENT
*/
void
d_tests_database_uris(
    void
)
{
    std::printf("connection strings: URIs\n");

    const db::connection_config full = db::parse_connection_string(
        "postgresql://me:p%40ss@[::1]:5433/app%20db?sslmode=require&"
        "application_name=x");

    D_TESTS_DATABASE_CHECK( (full.host == "::1")          &&
                            (full.port == 5433u)          &&
                            (full.database == "app db")   &&
                            (full.username == "me")       &&
                            (full.password == "p@ss") );
    D_TESTS_DATABASE_CHECK( (full.enable_ssl) &&
                            (!full.verify_ssl) );
    D_TESTS_DATABASE_CHECK( (full.custom_options.at("scheme") ==
                             "postgresql") &&
                            (full.custom_options.at("application_name") ==
                             "x") );

    const db::connection_config listed = db::parse_connection_string(
        "postgresql://h1:5432,h2:5433/db");

    D_TESTS_DATABASE_CHECK( (listed.host == "h1:5432,h2:5433") &&
                            (listed.database == "db") );

    const db::connection_config local = db::parse_connection_string(
        "postgresql:///db");

    D_TESTS_DATABASE_CHECK( (local.host.empty()) &&
                            (local.database == "db") );

    const db::connection_config mysql = db::parse_connection_string(
        "mysql://root@db.example");

    D_TESTS_DATABASE_CHECK( (mysql.host == "db.example") &&
                            (mysql.port == 0u)           &&
                            (mysql.username == "root")   &&
                            (mysql.database.empty()) );
    D_TESTS_DATABASE_CHECK(
        db::parse_connection_string("sqlite:///data/app.db").database ==
        "/data/app.db");
    D_TESTS_DATABASE_CHECK(
        db::parse_connection_string("sqlite::memory:").database ==
        ":memory:");
    D_TESTS_DATABASE_CHECK(
        db::parse_connection_string("sqlite:relative.db").database ==
        "relative.db");

    D_TESTS_DATABASE_THROWS(db::parse_connection_string("postgresql:db"),
                            db::exception,
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_DATABASE_THROWS(db::parse_connection_string("postgresql://[::1"),
                            db::exception,
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_DATABASE_THROWS(db::parse_connection_string("mysql://h:70000/"),
                            db::exception,
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_DATABASE_THROWS(db::parse_connection_string("mysql://h/d?x"),
                            db::exception,
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_DATABASE_THROWS(db::parse_connection_string("mysql://h/%zz"),
                            db::exception,
                            D_DB_INVALID_ARGUMENT);

    return;
}

/*
d_tests_database_keywords
  parse_connection_string's keyword forms, and build_connection_string.
  Tests the following:
  - libpq's: pairs separated by spaces, single quotes around a value with
    spaces, `\` escaping a quote or a backslash; keys in any ASCII case
  - ODBC's: pairs separated by semicolons, ODBC's names for the fields,
    and a key no field has kept as written
  - sslmode, the files, connect_timeout, charset and schema by every name
  - an unterminated quote, a pair without `=`, an unknown sslmode and a
    port that is no number, each D_DB_INVALID_ARGUMENT
  - a built string reads back as the same connection, quoting what needs
    it; the charset written as client_encoding; a default timeout left out
*/
void
d_tests_database_keywords(
    void
)
{
    std::printf("connection strings: keywords\n");

    const db::connection_config libpq = db::parse_connection_string(
        "Host=db port=5432 dbname='my db' user=me password='it\\'s \\\\' "
        "sslmode=verify-ca sslrootcert=/ca.pem connect_timeout=7 "
        "client_encoding=UTF8 search_path=app");

    D_TESTS_DATABASE_CHECK( (libpq.host == "db")             &&
                            (libpq.port == 5432u)            &&
                            (libpq.database == "my db")      &&
                            (libpq.username == "me")         &&
                            (libpq.password == "it's \\") );
    D_TESTS_DATABASE_CHECK( (libpq.enable_ssl) &&
                            (libpq.verify_ssl) &&
                            (libpq.ssl_ca == std::string("/ca.pem")) );
    D_TESTS_DATABASE_CHECK(libpq.connect_timeout == std::chrono::seconds(7));
    D_TESTS_DATABASE_CHECK( (libpq.charset == std::string("UTF8")) &&
                            (libpq.schema == std::string("app")) );

    const db::connection_config odbc = db::parse_connection_string(
        "Server=db;Database=app;Uid=me;Pwd=secret;Port=1433;Driver=x");

    D_TESTS_DATABASE_CHECK( (odbc.host == "db")         &&
                            (odbc.database == "app")    &&
                            (odbc.username == "me")     &&
                            (odbc.password == "secret") &&
                            (odbc.port == 1433u) );
    D_TESTS_DATABASE_CHECK(odbc.custom_options.at("Driver") == "x");

    D_TESTS_DATABASE_THROWS(db::parse_connection_string("host='db"),
                            db::exception,
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_DATABASE_THROWS(db::parse_connection_string("host=db port"),
                            db::exception,
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_DATABASE_THROWS(db::parse_connection_string("sslmode=sometimes"),
                            db::exception,
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_DATABASE_THROWS(db::parse_connection_string("port=x"),
                            db::exception,
                            D_DB_INVALID_ARGUMENT);

    db::connection_config made;

    made.host                          = "db.example";
    made.port                          = 6432u;
    made.database                      = "with space";
    made.username                      = "o'brien";
    made.password                      = "back\\slash";
    made.charset                       = "LATIN1";
    made.enable_ssl                    = true;
    made.verify_ssl                    = false;
    made.custom_options["application_name"] = "t";

    const std::string           built = db::build_connection_string(made);
    const db::connection_config again = db::parse_connection_string(built);

    D_TESTS_DATABASE_CHECK(built.find("client_encoding=LATIN1") !=
                           std::string::npos);
    D_TESTS_DATABASE_CHECK(built.find("connect_timeout") ==
                           std::string::npos);
    D_TESTS_DATABASE_CHECK( (again.host == made.host)                   &&
                            (again.port == made.port)                   &&
                            (again.database == made.database)           &&
                            (again.username == made.username)           &&
                            (again.password == made.password)           &&
                            (again.charset == made.charset)             &&
                            (again.enable_ssl)                          &&
                            (!again.verify_ssl)                         &&
                            (again.custom_options == made.custom_options) );

    return;
}

/*
d_tests_database_quoting
  escape_string and quote_identifier.
  Tests the following:
  - a quote doubled in a literal; nothing else changed
  - backticks for MySQL and MariaDB, brackets for SQL Server, double quotes
    for PostgreSQL, SQLite and the rest, the quote inside doubled
  - text or a name holding a NUL, D_DB_INVALID_ARGUMENT
*/
void
d_tests_database_quoting(
    void
)
{
    std::printf("quoting and escaping\n");

    D_TESTS_DATABASE_CHECK(db::escape_string("it's") == "it''s");
    D_TESTS_DATABASE_CHECK(db::escape_string("a\\b") == "a\\b");
    D_TESTS_DATABASE_CHECK(
        db::quote_identifier("a`b", db::database_type::mysql) == "`a``b`");
    D_TESTS_DATABASE_CHECK(
        db::quote_identifier("t", db::database_type::mariadb) == "`t`");
    D_TESTS_DATABASE_CHECK(
        db::quote_identifier("a]b", db::database_type::mssql) == "[a]]b]");
    D_TESTS_DATABASE_CHECK(
        db::quote_identifier("a\"b", db::database_type::postgresql) ==
        "\"a\"\"b\"");
    D_TESTS_DATABASE_CHECK(
        db::quote_identifier("t", db::database_type::sqlite) == "\"t\"");
    D_TESTS_DATABASE_THROWS(db::escape_string(std::string("\0", 1)),
                            db::exception,
                            D_DB_INVALID_ARGUMENT);
    D_TESTS_DATABASE_THROWS(
        db::quote_identifier(std::string("a\0", 2),
                             db::database_type::sqlite),
        db::exception,
        D_DB_INVALID_ARGUMENT);

    return;
}

/*
d_tests_database_values
  value_to_string and string_to_value.
  Tests the following:
  - NULL, booleans, integers, doubles in the fewest digits that read back
    (NaN and the infinities by name), text, bytes in hexadecimal, and a
    point in time as its UTC date and time, a fraction without trailing
    zeros
  - each type read back, booleans by PostgreSQL's words, bytes after `\x`
    or `0x`, a time with `T`, `Z` or an offset, a date alone its midnight
  - an integer too big for `integer` but not for `big_integer`; text that is
    no value of the type, type_conversion_exception with D_DB_MISMATCH
  - decimal text kept exactly
*/
void
d_tests_database_values(
    void
)
{
    using clock = std::chrono::system_clock;

    std::printf("values as text\n");

    D_TESTS_DATABASE_CHECK(db::value_to_string(db::value()) == "NULL");
    D_TESTS_DATABASE_CHECK(db::value_to_string(db::value(true)) == "true");
    D_TESTS_DATABASE_CHECK(db::value_to_string(db::value(
                               static_cast<re_std::int64_t>(-42))) == "-42");
    D_TESTS_DATABASE_CHECK(db::value_to_string(db::value(0.1)) == "0.1");
    D_TESTS_DATABASE_CHECK(
        db::value_to_string(db::value(
            std::numeric_limits<double>::quiet_NaN())) == "NaN");
    D_TESTS_DATABASE_CHECK(
        db::value_to_string(db::value(
            -std::numeric_limits<double>::infinity())) == "-Infinity");
    D_TESTS_DATABASE_CHECK(db::value_to_string(db::value(
                               std::string("text"))) == "text");
    D_TESTS_DATABASE_CHECK(
        db::value_to_string(db::value(std::vector<re_std::uint8_t>{
            0x00u, 0xABu })) == "00ab");

    const clock::time_point instant =
        clock::time_point(std::chrono::seconds(1792148096)) +
        std::chrono::milliseconds(500);

    D_TESTS_DATABASE_CHECK(db::value_to_string(db::value(instant)) ==
                           "2026-10-16 10:54:56.5");

    // reading back
    D_TESTS_DATABASE_CHECK(std::holds_alternative<std::monostate>(
        db::string_to_value("anything", db::field_type::null)));
    D_TESTS_DATABASE_CHECK(std::get<bool>(db::string_to_value(
                               "Yes", db::field_type::boolean)));
    D_TESTS_DATABASE_CHECK(!std::get<bool>(db::string_to_value(
                               "off", db::field_type::boolean)));
    D_TESTS_DATABASE_CHECK(std::get<re_std::int32_t>(db::string_to_value(
                               "-17", db::field_type::integer)) == -17);
    D_TESTS_DATABASE_CHECK(std::get<re_std::int64_t>(db::string_to_value(
                               "2147483648",
                               db::field_type::big_integer)) == 2147483648LL);
    D_TESTS_DATABASE_CHECK(std::isinf(std::get<double>(db::string_to_value(
                               "Infinity", db::field_type::floating_point))));
    D_TESTS_DATABASE_CHECK(std::get<std::vector<re_std::uint8_t>>(
                               db::string_to_value(
                                   "\\x00ff", db::field_type::binary))
                               .size() == 2u);
    D_TESTS_DATABASE_CHECK(std::get<std::vector<re_std::uint8_t>>(
                               db::string_to_value(
                                   "0xAB", db::field_type::binary))
                               .at(0) == 0xABu);
    D_TESTS_DATABASE_CHECK(std::get<clock::time_point>(db::string_to_value(
                               "2026-10-16T12:54:56.5+02:00",
                               db::field_type::timestamp)) == instant);
    D_TESTS_DATABASE_CHECK(std::get<clock::time_point>(db::string_to_value(
                               "2026-10-16 10:54:56.500Z",
                               db::field_type::datetime)) == instant);
    D_TESTS_DATABASE_CHECK(std::get<clock::time_point>(db::string_to_value(
                               "2026-10-16 02:54:56.5-0800",
                               db::field_type::timestamp)) == instant);
    D_TESTS_DATABASE_CHECK(db::value_to_string(db::string_to_value(
                               "2026-10-16", db::field_type::date)) ==
                           "2026-10-16 00:00:00");
    D_TESTS_DATABASE_CHECK(std::get<std::string>(db::string_to_value(
                               "12.50", db::field_type::decimal)) ==
                           "12.50");

    D_TESTS_DATABASE_THROWS(
        db::string_to_value("2147483648", db::field_type::integer),
        db::type_conversion_exception,
        D_DB_MISMATCH);
    D_TESTS_DATABASE_THROWS(
        db::string_to_value("ten", db::field_type::big_integer),
        db::type_conversion_exception,
        D_DB_MISMATCH);
    D_TESTS_DATABASE_THROWS(
        db::string_to_value("maybe", db::field_type::boolean),
        db::type_conversion_exception,
        D_DB_MISMATCH);
    D_TESTS_DATABASE_THROWS(
        db::string_to_value("2026-13-01", db::field_type::date),
        db::type_conversion_exception,
        D_DB_MISMATCH);

    return;
}

/*
d_tests_database_names
  get_type_name and get_field_type_name.
  Tests the following:
  - the vendors' display names, "unknown" for none
  - the field types' enumerator names
*/
void
d_tests_database_names(
    void
)
{
    std::printf("names\n");

    D_TESTS_DATABASE_CHECK(db::get_type_name(db::database_type::postgresql) ==
                           "PostgreSQL");
    D_TESTS_DATABASE_CHECK(db::get_type_name(db::database_type::sqlite) ==
                           "SQLite");
    D_TESTS_DATABASE_CHECK(db::get_type_name(db::database_type::unknown) ==
                           "unknown");
    D_TESTS_DATABASE_CHECK(
        db::get_field_type_name(db::field_type::big_integer) ==
        "big_integer");
    D_TESTS_DATABASE_CHECK(db::get_field_type_name(db::field_type::boolean) ==
                           "boolean");

    return;
}


// pool_counts
//   struct: what the stand-in connections of one pool were asked, and how
// they answer.
struct pool_counts
{
    int         made        = 0;
    int         pings       = 0;
    int         queries     = 0;
    bool        ping_answer = true;
    bool        query_fails = false;
    std::string last_query;
};

// pool_conn
//   class: a connection a pool can lend, which connects and answers a ping
// as its counts say.
class pool_conn
{
public:
    explicit pool_conn(
        pool_counts& _counts
    )
        : m_counts(&_counts),
          m_id(++_counts.made),
          m_connected(false)
    {}

    void
    connect()
    {
        m_connected = true;

        return;
    }

    bool
    ping()
    {
        ++m_counts->pings;

        return m_counts->ping_answer;
    }

    int
    id() const noexcept
    {
        return m_id;
    }

    bool
    connected() const noexcept
    {
        return m_connected;
    }

protected:
    pool_counts* m_counts;

private:
    int  m_id;
    bool m_connected;
};

// pool_query_conn
//   class: a pool_conn that runs statements too, so that a pool checks it
// with its validation_query.
class pool_query_conn : public pool_conn
{
public:
    explicit pool_query_conn(
        pool_counts& _counts
    )
        : pool_conn(_counts)
    {}

    bool
    execute(
        const std::string& _query
    )
    {
        ++m_counts->queries;
        m_counts->last_query = _query;

        // a statement the counts say fails, fails as a connection's would
        if (m_counts->query_fails)
        {
            throw db::query_exception("refused", 1);
        }

        return true;
    }
};

// pool_factory
//   function: a factory of stand-in connections over one set of counts.
template<typename Connection>
std::function<std::unique_ptr<Connection>()>
pool_factory(
    pool_counts& _counts
)
{
    pool_counts* counts = &_counts;

    return [counts]()
    {
        return std::make_unique<Connection>(*counts);
    };
}

// pool_settings
//   function: a pool_config with the sizes and durations given, and no
// checks.
db::pool_config
pool_settings(
    std::size_t               _min,
    std::size_t               _max,
    std::chrono::milliseconds _idle_timeout,
    std::chrono::milliseconds _max_lifetime
)
{
    db::pool_config config;

    config.min_connections = _min;
    config.max_connections = _max;
    config.idle_timeout    = _idle_timeout;
    config.max_lifetime    = _max_lifetime;

    return config;
}

/*
d_tests_database_pool
  connection_pool over stand-in connections.
  Tests the following:
  - the minimum is made and connected up front; a lent connection goes back
    idle; the pool lends the one that came back last, and refuses to go
    past its maximum, connection_exception; it never waits
  - idle_timeout retires idle connections, the longest waiting first, down
    to the minimum, when the pool next lends; max_lifetime retires one when
    it comes back; durations of 0 retire nothing
  - validate_on_acquire checks a connection by validation_query where the
    connection runs statements, replacing one that fails, and by a ping
    where it does not
*/
void
d_tests_database_pool(
    void
)
{
    std::printf("connection pool\n");

    const std::chrono::milliseconds none(0);
    const std::chrono::milliseconds instant(1);
    const std::chrono::milliseconds wait(20);

    // the minimum up front, the last one back lent first, the maximum
    {
        pool_counts                    counts;
        db::connection_pool<pool_conn> pool(pool_factory<pool_conn>(counts),
                                            pool_settings(2, 3, none, none));

        D_TESTS_DATABASE_CHECK( (counts.made == 2) &&
                                (pool.idle_connections() == 2) &&
                                (pool.active_connections() == 0) );
        {
            auto a = pool.acquire();
            auto b = pool.acquire();
            auto c = pool.acquire();

            D_TESTS_DATABASE_CHECK( (a->connected()) &&
                                    (pool.active_connections() == 3) &&
                                    (pool.idle_connections() == 0) &&
                                    (counts.made == 3) );
            D_TESTS_DATABASE_THROWS(pool.acquire(),
                                    db::connection_exception,
                                    0);
            D_TESTS_DATABASE_CHECK(pool.waiting_requests() == 0);

            b.release();
            D_TESTS_DATABASE_CHECK(pool.acquire()->id() == 1);
        }

        D_TESTS_DATABASE_CHECK( (pool.idle_connections() == 3) &&
                                (pool.total_connections() == 3) );
    }

    // idle_timeout: the longest waiting retired first, down to the minimum
    {
        pool_counts                    counts;
        db::connection_pool<pool_conn> pool(
            pool_factory<pool_conn>(counts),
            pool_settings(1, 3, instant, none));
        {
            auto a = pool.acquire();
            auto b = pool.acquire();
            auto c = pool.acquire();

            a.release();
            b.release();
            c.release();
        }
        std::this_thread::sleep_for(wait);

        auto kept = pool.acquire();

        D_TESTS_DATABASE_CHECK( (kept->id() == 3) &&
                                (pool.idle_connections() == 0) &&
                                (pool.total_connections() == 1) &&
                                (counts.made == 3) );
    }

    // max_lifetime: retired when it comes back, and replaced when asked
    {
        pool_counts                    counts;
        db::connection_pool<pool_conn> pool(
            pool_factory<pool_conn>(counts),
            pool_settings(0, 2, none, instant));
        {
            auto a = pool.acquire();

            std::this_thread::sleep_for(wait);
        }

        D_TESTS_DATABASE_CHECK( (pool.total_connections() == 0) &&
                                (pool.acquire()->id() == 2) );
    }

    // durations of 0: nothing retired, however long
    {
        pool_counts                    counts;
        db::connection_pool<pool_conn> pool(
            pool_factory<pool_conn>(counts),
            pool_settings(0, 2, none, none));
        {
            auto a = pool.acquire();
            auto b = pool.acquire();
        }
        std::this_thread::sleep_for(wait);

        D_TESTS_DATABASE_CHECK( (pool.acquire()->id() == 1) &&
                                (pool.idle_connections() == 1) );
    }

    // validation_query where the connection runs statements, ping elsewhere
    {
        pool_counts     counts;
        db::pool_config config = pool_settings(1, 2, none, none);

        config.validate_on_acquire = true;
        config.validation_query    = "SELECT 1";

        db::connection_pool<pool_query_conn> pool(
            pool_factory<pool_query_conn>(counts),
            config);

        D_TESTS_DATABASE_CHECK( (pool.acquire()->id() == 1) &&
                                (counts.queries == 1) &&
                                (counts.last_query == "SELECT 1") &&
                                (counts.pings == 0) );

        counts.query_fails = true;
        D_TESTS_DATABASE_CHECK( (pool.acquire()->id() == 2) &&
                                (counts.made == 2) );

        pool_counts                    pinged;
        db::connection_pool<pool_conn> plain(pool_factory<pool_conn>(pinged),
                                             config);

        D_TESTS_DATABASE_CHECK( (plain.acquire()->id() == 1) &&
                                (pinged.pings == 1) );
    }

    return;
}

}  // namespace

/*
main
  Every group, then the count.
*/
int
main(
    void
)
{
    d_tests_database_exceptions();
    d_tests_database_configuration();
    d_tests_database_uris();
    d_tests_database_keywords();
    d_tests_database_quoting();
    d_tests_database_values();
    d_tests_database_names();
    d_tests_database_pool();

    std::printf("%d checks, %d failed (core/db, as C++ %ld)\n",
                g_checks,
                g_failures,
                static_cast<long>(__cplusplus));

    return (g_failures == 0) ? 0 : 1;
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
