/*******************************************************************************
* djinterp [core]                                                  t_db_core.hpp
*
*   The checks every vendor's core/db harness shares: the operations
* database_connection writes once for every vendor -- running SQL and
* reading its rows, prepared statements, transactions and their guard,
* failures and the refusals of text a NUL would cut short, and how
* statements live and die with their connection -- run over a vendor's
* connection, with a dialect that says what differs: the SQL that makes a
* table, how parameters are written, the codes a failure carries and the
* field types the vendor reports. Each harness (t_sqlite_core.cpp,
* t_mariadb_core.cpp, t_postgres_core.cpp) adds the vendor's own members.
*   A header of test code, included by one harness each; core/db needs
* C++17, which every harness checks first.
*
*
* path:      /tests/unintegrated/core/db/t_db_core.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.27
*******************************************************************************/

#ifndef DJINTERP_TESTS_CORE_DB_T_DB_CORE_HPP
#define DJINTERP_TESTS_CORE_DB_T_DB_CORE_HPP 1

// std
#include <algorithm>  // std::reverse
#include <chrono>     // std::chrono's clocks, seconds, microseconds
#include <climits>    // LLONG_MIN
#include <cstdio>     // std::printf
#include <cstdlib>    // std::getenv
#include <cstring>    // std::memset, std::strlen, std::strncmp, std::strstr
#include <memory>     // std::unique_ptr
#include <optional>   // std::optional
#include <stdexcept>  // std::logic_error, std::out_of_range
#include <string>     // std::string
#include <thread>     // std::this_thread::sleep_for
#include <type_traits>  // std::false_type, std::true_type, std::void_t
#include <utility>    // std::declval, std::move
#include <variant>    // std::holds_alternative, std::get
#include <vector>     // std::vector
#if defined(D_TESTS_DB_CORE_SOCKETS)
    #if defined(_WIN32)
        #include <winsock2.h>  // WSAPoll, struct pollfd
        #if defined(_MSC_VER)
            #pragma comment(lib, "ws2_32.lib")
        #endif  // _MSC_VER
    #else
        #include <poll.h>      // poll, struct pollfd
    #endif
#endif  // D_TESTS_DB_CORE_SOCKETS
// djinterp
#include "../../../../inc/djinterp/core/db/database.hpp"  // the vocabulary
#include "../../../../inc/djinterp/core/db/database_async.hpp"
                                                  // async_operation
#include "../../../../inc/djinterp/core/db/database_table.hpp"
                                                  // database_table
#include "../../../../inc/djinterp/core/db/database_table_render.hpp"
                                                  // relational_table_view,
                                                  // store_table, load_table


namespace d_tests_db_core
{

namespace db = djinterp;

// g_checks, g_failures
//   the harness's counts, one pair for the whole program.
inline int g_checks   = 0;
inline int g_failures = 0;

/*
expect
  Counts one check, and prints it when it fails.
*/
inline void
expect(
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
setting
  An environment variable, or a default where it is unset or empty.
*/
inline std::string
setting(
    const char* _name,
    const char* _default
)
{
    const char* const value = std::getenv(_name);

    return ( (value) &&
             (value[0] != '\0') ) ? std::string(value)
                                  : std::string((_default) ? _default
                                                           : "");
}

// dialect
//   struct: what a vendor's SQL and answers are for the shared checks. The
// table is core_t, with the columns id (generated), name (text), flag
// (boolean), small (a 32-bit integer), real_value (a double), bytes (binary)
// and moment (a point in time); insert names every column but id, as
// parameters 1 to 6, and select_by_id takes the id as parameter 1.
struct dialect
{
    const char*    vendor;            // the name a failure's message begins
                                      // with: the server that answered
    const char*    create_table;      // CREATE TABLE core_t (...)
    const char*    true_literal;      // a boolean column's true, in SQL
    const char*    false_literal;     // and its false
    const char*    insert;            // INSERT ... VALUES (p1, ..., p6)
    const char*    select_by_id;      // SELECT name, small ... WHERE id = p1
    const char*    moment_as_text;    // SELECT the moment of row p1 as UTC
                                      // text, "YYYY-MM-DD HH:MM:SS.ffffff"
                                      // with its own trailing digits
    const char*    moment_expected;   // that text for the instant bound
    db::field_type id_type;           // the field type id reports
    db::field_type small_type;        // and small, INT
    int            syntax_error;      // what "SELEC 1" fails with
    bool         (*unique_violation)(int _code);  // a duplicate key's code
    const char*    slow_query;        // a query of one row, 0, that the
                                      // server takes a fraction of a second
                                      // over; NULL without a server
    const char*    long_query;        // and one it takes seconds over
    const char*    long_running;      // SQL counting the sessions, of id
                                      // %lu, that run long_query; NULL
                                      // without a server
    bool         (*cancelled)(int _code);  // a statement stopped by a
                                           // cancel's code; NULL without a
                                           // server
};

// table_abilities
//   struct: what the server can do to a table's columns, which the table
// checks ask before using it: RENAME COLUMN (SQLite 3.25.0, MySQL 8.0.3,
// MariaDB 10.5.2), DROP COLUMN (SQLite 3.35.0), and generated columns
// (SQLite 3.31.0, PostgreSQL 12; MySQL and MariaDB at their floors).
struct table_abilities
{
    bool rename_column;
    bool drop_column;
    bool generated_columns;
};

}  // namespace d_tests_db_core

// D_TESTS_DB_CORE_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_DB_CORE_CHECK(_cond)                                          \
    ::d_tests_db_core::expect((_cond), #_cond, __LINE__)

// D_TESTS_DB_CORE_THROWS
//   macro: records a check that a statement throws the exception named, with
// the error code expected.
#define D_TESTS_DB_CORE_THROWS(_statement, _exception, _code)                 \
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
        ::d_tests_db_core::expect(thrown_,                                    \
                                  #_statement " throws " #_exception,         \
                                  __LINE__);                                  \
    } while (0)

// D_TESTS_DB_CORE_THROWS_ANY
//   macro: records a check that a statement throws the exception named,
// whatever its code.
#define D_TESTS_DB_CORE_THROWS_ANY(_statement, _exception)                    \
    do                                                                        \
    {                                                                         \
        bool thrown_ = false;                                                 \
                                                                              \
        try                                                                   \
        {                                                                     \
            _statement;                                                       \
        }                                                                     \
        catch (const _exception&)                                             \
        {                                                                     \
            thrown_ = true;                                                   \
        }                                                                     \
                                                                              \
        ::d_tests_db_core::expect(thrown_,                                    \
                                  #_statement " throws " #_exception,         \
                                  __LINE__);                                  \
    } while (0)


namespace d_tests_db_core
{

/*
count_rows
  The rows core_t holds.
*/
template<typename Connection>
re_std::int64_t
count_rows(
    Connection& _db
)
{
    const std::optional<db::value> found =
        _db.execute_scalar("SELECT COUNT(*) FROM core_t");

    return ( (found) &&
             (std::holds_alternative<re_std::int64_t>(*found)) )
               ? std::get<re_std::int64_t>(*found)
               : -1;
}

/*
queries
  Running SQL, and reading its rows.
  Tests the following:
  - execute makes the table; execute_update counts the rows a statement
    inserts, or updates to new values, get_affected_rows says the same,
    get_last_insert_id names the id generated
  - a result's columns: their count, names, the index of a name in any
    ASCII case, their field types
  - each value as the column's storage gives it: the id as the vendor's
    integer, a boolean, INT as the vendor types it, a double, bytes, NULL
  - the cursor: before the first row, next and previous, first and last,
    absolute from either end, current_row and row_count
  - conversions: a number as text, an integer as a double, and what does
    not convert exactly, type_conversion_exception with D_DB_MISMATCH
  - the current row by name, the rows after the cursor, a scalar and a
    single row; an empty result
*/
template<typename Connection>
void
queries(
    Connection&    _db,
    const dialect& _dialect
)
{
    std::printf("queries\n");

    const std::string insert = "INSERT INTO core_t (name, flag, small, "
                               "real_value) VALUES ";

    D_TESTS_DB_CORE_CHECK(_db.execute(_dialect.create_table));
    D_TESTS_DB_CORE_CHECK(_db.execute_update(insert + "('a', " +
                                             _dialect.true_literal +
                                             ", 4, 1.5)") == 1);
    D_TESTS_DB_CORE_CHECK(_db.get_affected_rows() == 1);
    D_TESTS_DB_CORE_CHECK(_db.get_last_insert_id() == 1);
    D_TESTS_DB_CORE_CHECK(_db.execute_update(insert + "('b', " +
                                             _dialect.false_literal +
                                             ", 6, 0.1)") == 1);
    D_TESTS_DB_CORE_CHECK(_db.execute_update(
        "INSERT INTO core_t (name, small) VALUES ('c', 2147483647)") == 1);
    D_TESTS_DB_CORE_CHECK(_db.get_last_insert_id() == 3);

    // an update that changes what it matches, which every vendor counts
    D_TESTS_DB_CORE_CHECK(_db.execute_update(
        "UPDATE core_t SET small = small + 1 WHERE id < 3") == 2);
    D_TESTS_DB_CORE_CHECK(_db.get_affected_rows() == 2);

    std::unique_ptr<db::buffered_result_set> rows = _db.execute_query(
        "SELECT id, name, flag, small, real_value, bytes, moment "
        "FROM core_t ORDER BY id");

    D_TESTS_DB_CORE_CHECK( (rows->column_count() == 7u) &&
                           (rows->row_count() == 3) );
    D_TESTS_DB_CORE_CHECK( (rows->column_name(1u) == "name") &&
                           (rows->column_index("SMALL") == 3u) );
    D_TESTS_DB_CORE_CHECK(rows->column_type(0u) == _dialect.id_type);
    D_TESTS_DB_CORE_CHECK(rows->column_type(2u) == db::field_type::boolean);
    D_TESTS_DB_CORE_CHECK(rows->current_row() == -1);
    D_TESTS_DB_CORE_THROWS_ANY((void)rows->get_value(0u), std::out_of_range);

    // the first row, value by value
    D_TESTS_DB_CORE_CHECK(rows->next());
    D_TESTS_DB_CORE_CHECK(rows->get_long(0u).value_or(0) == 1);
    D_TESTS_DB_CORE_CHECK(std::get<std::string>(rows->get_value(1u)) == "a");
    D_TESTS_DB_CORE_CHECK( (std::holds_alternative<bool>(
                                rows->get_value(2u))) &&
                           (std::get<bool>(rows->get_value(2u))) );
    D_TESTS_DB_CORE_CHECK(rows->column_type(3u) == _dialect.small_type);
    D_TESTS_DB_CORE_CHECK(
        (_dialect.small_type == db::field_type::integer)
            ? ( (std::holds_alternative<re_std::int32_t>(
                     rows->get_value(3u))) &&
                (std::get<re_std::int32_t>(rows->get_value(3u)) == 5) )
            : ( (std::holds_alternative<re_std::int64_t>(
                     rows->get_value(3u))) &&
                (std::get<re_std::int64_t>(rows->get_value(3u)) == 5) ));
    D_TESTS_DB_CORE_CHECK(std::get<double>(rows->get_value(4u)) == 1.5);
    D_TESTS_DB_CORE_CHECK( (rows->is_null(5u)) &&
                           (std::holds_alternative<std::monostate>(
                                rows->get_value(6u))) );
    D_TESTS_DB_CORE_CHECK(!rows->get_string(5u));

    // conversions, by index and by name
    D_TESTS_DB_CORE_CHECK(rows->get_string("small").value_or("") == "5");
    D_TESTS_DB_CORE_CHECK(rows->get_double(3u).value_or(0.0) == 5.0);
    D_TESTS_DB_CORE_CHECK(rows->get_bool("flag").value_or(false));
    D_TESTS_DB_CORE_THROWS((void)rows->get_int(1u),
                           db::type_conversion_exception,
                           D_DB_MISMATCH);
    D_TESTS_DB_CORE_THROWS((void)rows->get_long(4u),
                           db::type_conversion_exception,
                           D_DB_MISMATCH);

    // the cursor
    D_TESTS_DB_CORE_CHECK(rows->next());
    D_TESTS_DB_CORE_CHECK(rows->get_string(4u).value_or("") == "0.1");
    D_TESTS_DB_CORE_CHECK(rows->next());
    D_TESTS_DB_CORE_CHECK(rows->get_int(3u).value_or(0) == 2147483647);
    D_TESTS_DB_CORE_CHECK(!rows->next());
    D_TESTS_DB_CORE_CHECK(rows->previous());
    D_TESTS_DB_CORE_CHECK( (rows->first()) &&
                           (rows->current_row() == 0) );
    D_TESTS_DB_CORE_CHECK( (rows->last()) &&
                           (rows->current_row() == 2) );
    D_TESTS_DB_CORE_CHECK( (rows->absolute(-3)) &&
                           (rows->current_row() == 0) );
    D_TESTS_DB_CORE_CHECK(!rows->absolute(3));

    // whole rows
    D_TESTS_DB_CORE_CHECK(rows->absolute(1));

    const db::row current = rows->get_current_row();

    D_TESTS_DB_CORE_CHECK(std::get<std::string>(current.at("name")) == "b");
    D_TESTS_DB_CORE_CHECK(rows->get_all_rows().size() == 1u);
    D_TESTS_DB_CORE_CHECK( (!rows->next()) &&
                           (rows->absolute(-4) == false) );
    D_TESTS_DB_CORE_CHECK( (rows->first()) &&
                           (rows->previous() == false) &&
                           (rows->get_all_rows().size() == 3u) );
    D_TESTS_DB_CORE_CHECK(count_rows(_db) == 3);

    const std::optional<db::row> second = _db.execute_single_row(
        "SELECT name FROM core_t WHERE id = 2");

    D_TESTS_DB_CORE_CHECK( (second) &&
                           (std::get<std::string>(second->at("name")) ==
                            "b") );
    D_TESTS_DB_CORE_CHECK(!_db.execute_single_row(
        "SELECT name FROM core_t WHERE id = 99"));
    D_TESTS_DB_CORE_CHECK(!_db.execute_scalar(
        "SELECT name FROM core_t WHERE id = 99"));

    std::unique_ptr<db::buffered_result_set> none = _db.execute_query(
        "SELECT name FROM core_t WHERE id = 99");

    D_TESTS_DB_CORE_CHECK( (none->row_count() == 0) &&
                           (!none->next()) &&
                           (none->column_count() == 1u) );

    return;
}

/*
statements
  Prepared statements.
  Tests the following:
  - the parameters' count; each bind by type, and bind_value with each of
    value's alternatives, a point in time among them, which reads back as
    the instant bound
  - execute_update counts, and the connection's affected rows and last id
    follow; execute answers true; a statement runs again with new values
  - clear_parameters leaves NULL; reset keeps the values
  - a statement's rows; text holding a NUL, and a parameter that is not
    there, refused with query_exception
*/
template<typename Connection>
void
statements(
    Connection&    _db,
    const dialect& _dialect
)
{
    using clock = std::chrono::system_clock;

    std::printf("statements\n");

    auto insert = _db.prepare(_dialect.insert);

    D_TESTS_DB_CORE_CHECK(insert->parameter_count() == 6u);

    const clock::time_point instant =
        clock::time_point(std::chrono::seconds(86400 * 365)) +
        std::chrono::microseconds(250000);

    insert->bind_string(1u, "d");
    insert->bind_bool(2u, true);
    insert->bind_int(3u, -3);
    insert->bind_double(4u, 2.25);
    insert->bind_binary(5u, std::vector<re_std::uint8_t>{ 0x00u, 0xFFu });
    insert->bind_value(6u, db::value(instant));
    D_TESTS_DB_CORE_CHECK(insert->execute_update() == 1);
    D_TESTS_DB_CORE_CHECK(_db.get_affected_rows() == 1);
    D_TESTS_DB_CORE_CHECK(_db.get_last_insert_id() == 4);

    // each alternative, through bind_value
    insert->bind_value(1u, db::value(std::string("e")));
    insert->bind_value(2u, db::value(false));
    insert->bind_value(3u, db::value(static_cast<re_std::int32_t>(9)));
    insert->bind_value(4u, db::value(0.5));
    insert->bind_value(5u, db::value(std::vector<re_std::uint8_t>()));
    insert->bind_value(6u, db::value());
    D_TESTS_DB_CORE_CHECK(insert->execute());
    D_TESTS_DB_CORE_CHECK(_db.get_last_insert_id() == 5);
    insert->bind_value(3u, db::value(static_cast<re_std::int64_t>(11)));
    insert->bind_null(1u);
    insert->reset();
    D_TESTS_DB_CORE_CHECK(insert->execute_update() == 1);
    insert->clear_parameters();
    D_TESTS_DB_CORE_CHECK(insert->execute_update() == 1);
    D_TESTS_DB_CORE_CHECK(count_rows(_db) == 7);

    std::unique_ptr<db::buffered_result_set> blank = _db.execute_query(
        "SELECT name, small FROM core_t WHERE id = 7");

    D_TESTS_DB_CORE_CHECK( (blank->next()) &&
                           (blank->is_null(0u)) &&
                           (blank->is_null(1u)) );

    // a statement's rows, the moment read back
    auto select = _db.prepare(_dialect.select_by_id);

    select->bind_long(1u, 4);

    std::unique_ptr<db::buffered_result_set> found = select->execute_query();

    D_TESTS_DB_CORE_CHECK( (found->next()) &&
                           (found->get_string(0u).value_or("") == "d") &&
                           (found->get_int(1u).value_or(0) == -3) );

    std::unique_ptr<db::buffered_result_set> binary = _db.execute_query(
        "SELECT bytes FROM core_t WHERE id = 4");

    D_TESTS_DB_CORE_CHECK( (binary->next()) &&
                           (binary->get_binary(0u).value_or(
                                std::vector<re_std::uint8_t>()).size() ==
                            2u) );

    auto moment = _db.prepare(_dialect.moment_as_text);

    moment->bind_long(1u, 4);

    std::unique_ptr<db::buffered_result_set> when = moment->execute_query();

    D_TESTS_DB_CORE_CHECK( (when->next()) &&
                           (when->get_string(0u).value_or("") ==
                            _dialect.moment_expected) );

    // refusals
    D_TESTS_DB_CORE_THROWS(insert->bind_string(1u, std::string("a\0b", 3)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS_ANY(insert->bind_int(99u, 1), db::query_exception);
    D_TESTS_DB_CORE_CHECK(_db.is_connected());

    return;
}

/*
transactions
  Transactions, their guard, savepoints and levels.
  Tests the following:
  - begin_transaction, in_transaction, rollback undoing and commit keeping
  - the guard: rolled back when it goes, kept when committed, inactive
    after either, refusing a second commit
  - savepoints: what follows one undone, the transaction kept
  - a transaction at a level; auto-commit outside one
*/
template<typename Connection>
void
transactions(
    Connection& _db
)
{
    std::printf("transactions\n");

    const re_std::int64_t before = count_rows(_db);

    D_TESTS_DB_CORE_CHECK(!_db.in_transaction());
    _db.begin_transaction();
    D_TESTS_DB_CORE_CHECK(_db.in_transaction());
    _db.execute("DELETE FROM core_t");
    _db.rollback();
    D_TESTS_DB_CORE_CHECK( (!_db.in_transaction()) &&
                           (count_rows(_db) == before) );

    // the guard
    {
        db::transaction<Connection> work(_db);

        D_TESTS_DB_CORE_CHECK(work.is_active());
        _db.execute("DELETE FROM core_t WHERE id = 1");
    }

    D_TESTS_DB_CORE_CHECK(count_rows(_db) == before);

    {
        db::transaction<Connection> work(_db);

        _db.execute("DELETE FROM core_t WHERE id = 7");
        work.commit();
        D_TESTS_DB_CORE_CHECK(!work.is_active());
        D_TESTS_DB_CORE_THROWS_ANY(work.commit(), db::transaction_exception);
    }

    D_TESTS_DB_CORE_CHECK(count_rows(_db) == before - 1);

    // savepoints
    {
        db::transaction<Connection> work(_db);

        work.create_savepoint("core_s");
        _db.execute("DELETE FROM core_t");
        work.rollback_to_savepoint("core_s");
        work.release_savepoint("core_s");
        D_TESTS_DB_CORE_CHECK(_db.in_transaction());
        work.commit();
    }

    D_TESTS_DB_CORE_CHECK(count_rows(_db) == before - 1);

    // a level
    {
        db::transaction<Connection> work(_db,
                                         db::isolation_level::serializable);

        D_TESTS_DB_CORE_CHECK(_db.in_transaction());
        work.rollback();
    }

    D_TESTS_DB_CORE_CHECK( (!_db.in_transaction()) &&
                           (_db.get_auto_commit()) );

    return;
}

/*
errors
  Failures, and what the connection keeps of them.
  Tests the following:
  - a syntax error: query_exception with the vendor's code, its message
    beginning with the name of the server that answered, then the
    operation; get_last_error_code and has_error follow, clear_error
    forgets; the connection stays open
  - each operation names itself in its failure and in get_last_error:
    execute, execute_update, execute_query, execute_scalar and
    execute_single_row, though several run the same hook
  - a duplicate key, the vendor's code for it
  - SQL, or a savepoint's name, holding a NUL: query_exception with
    D_DB_INVALID_ARGUMENT, before the database sees it
*/
template<typename Connection>
void
errors(
    Connection&    _db,
    const dialect& _dialect
)
{
    std::printf("errors\n");

    try
    {
        _db.execute("SELEC 1");
        D_TESTS_DB_CORE_CHECK(false);
    }
    catch (const db::query_exception& e)
    {
        D_TESTS_DB_CORE_CHECK(e.error_code() == _dialect.syntax_error);
        D_TESTS_DB_CORE_CHECK(std::strncmp(e.what(),
                                           _dialect.vendor,
                                           std::strlen(_dialect.vendor)) ==
                              0);
        D_TESTS_DB_CORE_CHECK(std::strstr(e.what(), " execute: ") !=
                              nullptr);
    }

    // each operation under its own name, the hook it shares notwithstanding
    const auto names = [&_db](auto&& _call, const char* _label) -> void
    {
        try
        {
            _call();
            D_TESTS_DB_CORE_CHECK(false);
        }
        catch (const db::query_exception& e)
        {
            D_TESTS_DB_CORE_CHECK( (std::strstr(e.what(), _label) != nullptr) &&
                                   (_db.get_last_error().find(_label) !=
                                    std::string::npos) );
        }

        return;
    };

    names([&_db]() { (void)_db.execute_update("SELEC 1"); },
          " execute_update: ");
    names([&_db]() { (void)_db.execute_query("SELEC 1"); },
          " execute_query: ");
    names([&_db]() { (void)_db.execute_scalar("SELEC 1"); },
          " execute_scalar: ");
    names([&_db]() { (void)_db.execute_single_row("SELEC 1"); },
          " execute_single_row: ");

    D_TESTS_DB_CORE_CHECK( (_db.has_error()) &&
                           (_db.get_last_error_code() ==
                            _dialect.syntax_error) &&
                           (!_db.get_last_error().empty()) );
    _db.clear_error();
    D_TESTS_DB_CORE_CHECK( (!_db.has_error()) &&
                           (_db.get_last_error_code() == 0) );
    D_TESTS_DB_CORE_CHECK( (_db.is_connected()) &&
                           (_db.get_state() ==
                            db::connection_state::connected) );

    try
    {
        _db.execute("INSERT INTO core_t (id, name) VALUES (2, 'again')");
        D_TESTS_DB_CORE_CHECK(false);
    }
    catch (const db::query_exception& e)
    {
        D_TESTS_DB_CORE_CHECK(_dialect.unique_violation(e.error_code()));
    }

    D_TESTS_DB_CORE_THROWS(
        _db.execute(std::string("DELETE FROM core_t\0 WHERE id = 2", 31)),
        db::query_exception,
        D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS(
        (void)_db.execute_query(std::string("SELECT 1\0", 9)),
        db::query_exception,
        D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS((void)_db.prepare(std::string("SELECT\0 1", 9)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    _db.begin_transaction();
    D_TESTS_DB_CORE_THROWS(_db.create_savepoint(std::string("s\0", 2)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    _db.rollback();
    D_TESTS_DB_CORE_CHECK(count_rows(_db) == 6);
    D_TESTS_DB_CORE_CHECK(_db.is_connected());

    return;
}

/*
streams
  Rows a page at a time.
  Tests the following:
  - a stream's rows are the buffered run's, value for value, its first page
    read before execute_stream returns -- the columns known; row_count is -1
    until the last page, then every row; current_row counts across pages
  - while the run is open the connection's operations, a prepare, another
    statement's run and another stream throw query_exception with the
    layer's busy code where its runs hold the connection, and run where they
    do not (SQLite); the statement's own values cannot change, on every
    vendor; the run's end frees the connection
  - forward only: previous, first beyond the first page and absolute behind
    throw D_DB_UNSUPPORTED; first within it, absolute ahead and last read on;
    a negative absolute counts back within the last page; a page filled
    exactly ends with an empty one
  - running the statement again, closing the stream, destroying it, or the
    statement going, ends the run: the stream is closed, its next move
    throws, and the connection is free; a page of no rows is refused
  - stream_query's stream holds a statement of its own
*/
template<typename Connection>
void
streams(
    Connection&    _db,
    const dialect& _dialect
)
{
    using layer_type = typename Connection::layer_type;

    std::printf("streams\n");

    const re_std::int64_t before = count_rows(_db);
    const std::string     query  = "SELECT name, small FROM core_t WHERE "
                                   "name = 'stream' ORDER BY small";
    auto                  insert = _db.prepare(_dialect.insert);

    // seven rows to read, small 1 to 7
    for (re_std::int32_t i = 1; i <= 7; ++i)
    {
        insert->clear_parameters();
        insert->bind_string(1u, "stream");
        insert->bind_int(3u, i);
        (void)insert->execute_update();
    }

    std::unique_ptr<db::buffered_result_set> all = _db.execute_query(query);
    auto select = _db.prepare(query);
    auto other  = _db.prepare("SELECT COUNT(*) FROM core_t");

    // pages of three: the buffered run's rows
    {
        auto            stream = select->execute_stream(3u);
        re_std::int64_t read   = 0;
        bool            same   = true;

        D_TESTS_DB_CORE_CHECK( (stream->is_open())            &&
                               (!stream->is_ended())          &&
                               (stream->column_count() == 2u) &&
                               (stream->column_name(1u) == "small") &&
                               (stream->row_count() == -1)    &&
                               (stream->current_row() == -1) );

        // each row against the buffered run's
        while (stream->next())
        {
            same = ( (same)                                         &&
                     (all->next())                                  &&
                     (stream->current_row() == read)                &&
                     (stream->get_long(1u) == all->get_long(1u))    &&
                     (stream->get_string("name") ==
                      all->get_string("name")) );
            ++read;
        }

        D_TESTS_DB_CORE_CHECK( (same)                         &&
                               (read == 7)                    &&
                               (stream->row_count() == 7)     &&
                               (stream->current_row() == 7)   &&
                               (stream->is_ended())           &&
                               (!stream->next()) );
    }

    // while the run is open
    {
        auto stream = select->execute_stream(3u);

        D_TESTS_DB_CORE_CHECK(stream->next());

        // the connection is the run's where the layer's runs hold it
        if constexpr (layer_type::holds_connection)
        {
            D_TESTS_DB_CORE_THROWS(_db.execute("SELECT 1"),
                                   db::query_exception,
                                   layer_type::busy_code);
            D_TESTS_DB_CORE_THROWS((void)_db.prepare("SELECT 1"),
                                   db::query_exception,
                                   layer_type::busy_code);
            D_TESTS_DB_CORE_THROWS((void)other->execute_query(),
                                   db::query_exception,
                                   layer_type::busy_code);
            D_TESTS_DB_CORE_THROWS((void)other->execute_stream(),
                                   db::query_exception,
                                   layer_type::busy_code);
            D_TESTS_DB_CORE_THROWS((void)_db.stream_query("SELECT 1"),
                                   db::query_exception,
                                   layer_type::busy_code);
            D_TESTS_DB_CORE_CHECK(_db.is_connected());
        }
        else
        {
            D_TESTS_DB_CORE_CHECK(count_rows(_db) == before + 7);
            D_TESTS_DB_CORE_CHECK(other->execute_query()->next());
        }

        // the statement's own values, on every vendor
        D_TESTS_DB_CORE_THROWS(select->clear_parameters(),
                               db::query_exception,
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_DB_CORE_THROWS(select->bind_long(1u, 1),
                               db::query_exception,
                               D_DB_INVALID_ARGUMENT);

        // the rest, and the connection free again
        const db::result_rows rest = stream->get_all_rows();

        D_TESTS_DB_CORE_CHECK( (rest.size() == 6u) &&
                               (stream->is_ended()) );
        D_TESTS_DB_CORE_CHECK(count_rows(_db) == before + 7);
        select->clear_parameters();
    }

    // forward only
    {
        auto stream = select->execute_stream(3u);

        D_TESTS_DB_CORE_CHECK( (stream->next()) && (stream->next()) );
        D_TESTS_DB_CORE_THROWS((void)stream->previous(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_CHECK( (stream->first()) &&
                               (stream->current_row() == 0) );
        D_TESTS_DB_CORE_CHECK( (stream->absolute(4)) &&
                               (stream->get_long(1u).value_or(0) == 5) );
        D_TESTS_DB_CORE_THROWS((void)stream->first(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_THROWS((void)stream->absolute(2),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_CHECK( (stream->absolute(-1))                    &&
                               (stream->get_long(1u).value_or(0) == 7)  &&
                               (stream->row_count() == 7) );
        D_TESTS_DB_CORE_THROWS((void)stream->absolute(-3),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_CHECK( (!stream->absolute(9)) &&
                               (stream->current_row() == 7) );
    }

    // a page filled exactly, then an empty one; last over both
    {
        auto stream = select->execute_stream(7u);

        D_TESTS_DB_CORE_CHECK( (!stream->is_ended())                    &&
                               (stream->last())                         &&
                               (stream->get_long(1u).value_or(0) == 7)  &&
                               (stream->current_row() == 6)             &&
                               (stream->row_count() == 7) );
        D_TESTS_DB_CORE_CHECK( (!stream->next())           &&
                               (stream->column_count() == 2u) );
    }

    // ending a run part-way: running again, closing, destroying, and the
    // statement going
    {
        auto stream = select->execute_stream(2u);

        D_TESTS_DB_CORE_CHECK(stream->next());
        D_TESTS_DB_CORE_CHECK(select->execute_query()->row_count() == 7);
        D_TESTS_DB_CORE_CHECK(!stream->is_open());
        D_TESTS_DB_CORE_THROWS_ANY((void)stream->next(), db::query_exception);
        D_TESTS_DB_CORE_CHECK(count_rows(_db) == before + 7);

        stream = select->execute_stream(2u);
        stream->close();
        D_TESTS_DB_CORE_CHECK( (!stream->is_open()) &&
                               (count_rows(_db) == before + 7) );

        stream = select->execute_stream(2u);
        stream.reset();
        D_TESTS_DB_CORE_CHECK(count_rows(_db) == before + 7);

        auto brief = _db.prepare(query);

        stream = brief->execute_stream(2u);
        brief.reset();
        D_TESTS_DB_CORE_CHECK(!stream->is_open());
        D_TESTS_DB_CORE_THROWS_ANY((void)stream->next(), db::query_exception);
        D_TESTS_DB_CORE_CHECK(count_rows(_db) == before + 7);

        D_TESTS_DB_CORE_THROWS((void)select->execute_stream(0u),
                               db::query_exception,
                               D_DB_INVALID_ARGUMENT);
    }

    // the connection's own stream, over a statement of its own
    {
        auto stream = _db.stream_query(query, 4u);

        D_TESTS_DB_CORE_CHECK( (stream->next()) &&
                               (stream->get_string(0u).value_or("") ==
                                "stream") );
        D_TESTS_DB_CORE_CHECK(stream->get_all_rows().size() == 6u);
        D_TESTS_DB_CORE_CHECK(count_rows(_db) == before + 7);
    }

    (void)_db.execute_update("DELETE FROM core_t WHERE name = 'stream'");
    D_TESTS_DB_CORE_CHECK(count_rows(_db) == before);

    return;
}

/*
lifetimes
  How statements and connections live and die together. `_make` makes a
connection to the harness's database, not yet connected.
  Tests the following:
  - disconnect closes every statement prepared on the connection, which then
    refuses to run; the connection refuses too, not connected; reconnect
    opens it again
  - disconnect closes a stream open on the connection, and frees it
  - a connection moved: the one moved to is open, its statements with it;
    the one moved from is disconnected
  - a connection that goes closes its statements first
  - auto_reconnect is refused, D_DB_UNSUPPORTED; a setting holding a NUL,
    D_DB_INVALID_ARGUMENT; either leaves the connection in error
*/
template<typename Connection,
         typename Make>
void
lifetimes(
    Make _make
)
{
    std::printf("lifetimes\n");

    Connection db = _make();

    db.connect();

    auto kept = db.prepare("SELECT 1");

    db.disconnect();
    D_TESTS_DB_CORE_CHECK( (!kept->is_open()) &&
                           (db.get_state() ==
                            db::connection_state::disconnected) );
    D_TESTS_DB_CORE_THROWS_ANY((void)kept->execute_query(),
                               db::query_exception);
    D_TESTS_DB_CORE_THROWS_ANY(db.execute("SELECT 1"),
                               db::connection_exception);
    db.reconnect();
    D_TESTS_DB_CORE_CHECK(db.is_connected());

    // a stream open as its connection disconnects
    {
        auto stream = db.stream_query("SELECT 1 UNION ALL SELECT 2", 1u);

        D_TESTS_DB_CORE_CHECK(stream->next());
        db.disconnect();
        D_TESTS_DB_CORE_CHECK(!stream->is_open());
        D_TESTS_DB_CORE_THROWS_ANY((void)stream->next(), db::query_exception);
        db.reconnect();
        D_TESTS_DB_CORE_CHECK(db.execute_scalar("SELECT 3").has_value());
    }

    // moves
    auto follows = db.prepare("SELECT 1");
    Connection moved(std::move(db));

    D_TESTS_DB_CORE_CHECK( (moved.is_connected()) &&
                           (!db.is_connected()) );
    D_TESTS_DB_CORE_CHECK( (follows->is_open()) &&
                           (follows->execute_query()->row_count() == 1) );

    Connection assigned = _make();

    assigned = std::move(moved);
    D_TESTS_DB_CORE_CHECK( (assigned.is_connected()) &&
                           (!moved.is_connected()) &&
                           (follows->is_open()) );

    // a connection that goes takes its statements with it
    {
        Connection brief = _make();

        brief.connect();
        kept = brief.prepare("SELECT 1");
        D_TESTS_DB_CORE_CHECK(kept->is_open());
    }

    D_TESTS_DB_CORE_CHECK(!kept->is_open());

    // settings refused
    db::connection_config config = assigned.get_config();

    config.auto_reconnect = true;
    D_TESTS_DB_CORE_THROWS(_make().connect(config),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    config.auto_reconnect = false;
    config.password       = std::string("pass\0word", 9);

    Connection refused = _make();

    D_TESTS_DB_CORE_THROWS(refused.connect(config),
                           db::connection_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_CHECK(refused.get_state() ==
                          db::connection_state::error);

    return;
}

/*
number
  A value read as an integer, whichever integer the vendor typed it; LLONG_MIN
for any other value.
*/
inline long long
number(
    const db::value& _value
)
{
    if (const re_std::int32_t* narrow = std::get_if<re_std::int32_t>(&_value))
    {
        return *narrow;
    }

    if (const re_std::int64_t* big = std::get_if<re_std::int64_t>(&_value))
    {
        return *big;
    }

    return LLONG_MIN;
}

/*
text
  A value read as text; "" for any other value.
*/
inline std::string
text(
    const db::value& _value
)
{
    const std::string* const held = std::get_if<std::string>(&_value);

    return (held) ? *held
                  : std::string();
}

/*
scalar
  The first column of a query's first row, as an integer; LLONG_MIN for none.
*/
template<typename Connection>
long long
scalar(
    Connection&        _db,
    const std::string& _query
)
{
    const std::optional<db::value> found = _db.execute_scalar(_query);

    return (found) ? number(*found)
                   : LLONG_MIN;
}

/*
int_value, big_value, text_value
  Cells, for rows written by hand.
*/
inline db::value
int_value(
    int _number
)
{
    return db::value(static_cast<re_std::int32_t>(_number));
}

inline db::value
big_value(
    long long _number
)
{
    return db::value(static_cast<re_std::int64_t>(_number));
}

inline db::value
text_value(
    const char* _text
)
{
    return db::value(std::string(_text));
}

/*
tables
  database_table over a vendor's connection, Table being it or a vendor's
table class.
  Tests the following:
  - a table described and created through the layer; a second create a
    success under IF NOT EXISTS and a failure without it; exists
  - fetch_schema: the columns' names, types, NULLs, defaults, declared types
    and the key
  - a composite key in primary_key_columns' order, read back so; a list
    naming a column twice or one the schema lacks, D_DB_INVALID_ARGUMENT
  - commit: new rows inserted, the keys the database chose read back; a
    changed row updated alone; a removed row deleted; a key given kept; a
    NULL left to a default; two rows of one key refused before anything is
    written; a failure rolled back, the cache kept dirty, and inside the
    program's transaction undone under a savepoint, the transaction usable;
    rows read a page at a time committed by their key
  - an OFFSET without a LIMIT, on every vendor
  - without a key: rows never read inserted, then read; a filtered cache
    replacing only the rows its filter read; a page refused, D_DB_UNSUPPORTED
  - a key changed in place, the child rows following it under ON UPDATE
    CASCADE: a chain of changes ordered so that each key is free when taken;
    rows exchanging keys refused, D_DB_UNSUPPORTED, nothing written, the cache
    kept; a cell changed through a reference committed; rows reordered no
    change, and a reordered row's key still changed in place
  - a generated column, where the server has them: described so, never
    written, its values read again after an insert or an update; a change to
    one refused, D_DB_UNSUPPORTED, by update_cell and, through a reference,
    by commit; a schema holding one refused by create_table, D_DB_UNSUPPORTED
  - the alter_ members, refused while the cache is dirty; rename_table
  - a view: read, never changed; a temporary table found and written
  - sync: auto_commit, and on_access reading a stale cache again
  - the render view, its key as commit() takes it; store_table and load_table,
    a connection other than the table's refused, D_DB_INVALID_ARGUMENT
  - truncate_table, drop_table; refusals of NUL text, of a schema of no
    columns and of another width; a table with no connection; moves
*/
template<typename Table,
         typename Connection>
void
tables(
    Connection&            _db,
    const dialect&         _dialect,
    const table_abilities& _can
)
{
    std::printf("tables\n");

    // a table described, then created through the layer
    db::table_schema schema;
    db::column_info  id("id", db::field_type::big_integer, false, true);
    db::column_info  name("name", db::field_type::string, false);

    id.is_auto_increment = true;
    name.max_length      = 40u;
    name.default_value   = "'unnamed'";
    schema.table_name    = "core_tables";
    schema.columns       = { id,
                             name,
                             db::column_info("score", db::field_type::integer),
                             db::column_info("ratio",
                                             db::field_type::floating_point) };
    schema.primary_key_columns = { "id" };

    Table made(_db, schema);

    made.drop_table();
    D_TESTS_DB_CORE_CHECK(!made.exists());
    made.create_table();
    D_TESTS_DB_CORE_CHECK(made.exists());
    made.create_table();
    D_TESTS_DB_CORE_THROWS_ANY(made.create_table(false), db::query_exception);

    // a composite key in the order primary_key_columns gives, not the
    // table's; a list naming a column twice, or one the schema lacks
    db::table_schema keyed;

    keyed.table_name          = "core_keyed";
    keyed.columns             = {
        db::column_info("a", db::field_type::integer, false),
        db::column_info("b", db::field_type::integer, false),
        db::column_info("c", db::field_type::integer) };
    keyed.primary_key_columns = { "b", "a" };

    Table ordered(_db, keyed);

    ordered.drop_table();
    ordered.create_table();

    Table reread(_db, std::string("core_keyed"));

    reread.fetch_schema();
    D_TESTS_DB_CORE_CHECK( (reread.get_schema().primary_key_columns ==
                            std::vector<std::string>{ "b", "a" }) &&
                           (reread.key_columns() ==
                            std::vector<std::size_t>{ 1u, 0u }) );
    ordered.drop_table();
    keyed.primary_key_columns = { "b", "b" };

    Table twice(_db, keyed);

    D_TESTS_DB_CORE_THROWS(twice.create_table(),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    keyed.primary_key_columns = { "b", "nope" };

    Table missing(_db, keyed);

    D_TESTS_DB_CORE_THROWS(missing.create_table(),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_CHECK(!missing.exists());

    // described by the database
    Table t(_db, std::string("core_tables"));

    t.fetch_schema();
    D_TESTS_DB_CORE_CHECK( (t.cols() == 4u) &&
                           (t.column_name(1u) == "name") &&
                           (t.key_columns() ==
                            std::vector<std::size_t>{ 0u }) );
    D_TESTS_DB_CORE_CHECK( (t.column_type(0u) == db::field_type::big_integer) &&
                           (t.column_type(1u) == db::field_type::string) &&
                           (t.column_type(2u) == _dialect.small_type) &&
                           (t.column_type(3u) ==
                            db::field_type::floating_point) );

    const db::column_info& described = t.get_schema().columns[1];

    D_TESTS_DB_CORE_CHECK( (!described.nullable) &&
                           (described.default_value) &&
                           (described.default_value->find("unnamed") !=
                            std::string::npos) &&
                           (described.sql_type) );
    D_TESTS_DB_CORE_CHECK(t.get_schema().columns[2].nullable);

    // new rows, the keys the database chose read back
    t.set_order("id");
    t.refresh();
    D_TESTS_DB_CORE_CHECK( (t.rows() == 0u) &&
                           (!t.is_dirty()) &&
                           (!t.is_stale()) );
    t.insert_row({ db::value(), text_value("a"), int_value(1),
                   db::value(0.5) });
    t.insert_row({ db::value(), text_value("b"), int_value(2), db::value() });
    D_TESTS_DB_CORE_CHECK(t.is_dirty());
    t.commit();
    D_TESTS_DB_CORE_CHECK( (!t.is_dirty()) &&
                           (t.rows() == 2u) &&
                           (t.row_count_remote() == 2) );
    D_TESTS_DB_CORE_CHECK( (number(t.cell(0u, 0u)) == 1) &&
                           (number(t.cell(1u, 0u)) == 2) );

    // a changed row updated, and nothing else
    t.update_cell(0u, "score", int_value(10));
    t.commit();
    D_TESTS_DB_CORE_CHECK(_db.get_affected_rows() == 1);
    D_TESTS_DB_CORE_CHECK( (scalar(_db, "SELECT score FROM core_tables "
                                        "WHERE id = 1") == 10) &&
                           (scalar(_db, "SELECT score FROM core_tables "
                                        "WHERE id = 2") == 2) );

    // a row removed, a key given, a NULL left to the default
    t.remove_row(1u);
    t.commit();
    D_TESTS_DB_CORE_CHECK(t.row_count_remote() == 1);
    t.insert_row({ big_value(10), text_value("c"), db::value(),
                   db::value(1.25) });
    t.commit();
    D_TESTS_DB_CORE_CHECK( (t.rows() == 2u) &&
                           (number(t.cell(1u, 0u)) == 10) &&
                           (t.row_count_remote() == 2) );
    t.insert_row({ db::value(), db::value(), int_value(3), db::value() });
    t.commit();
    D_TESTS_DB_CORE_CHECK( (t.rows() == 3u) &&
                           (!t.is_dirty()) );
    D_TESTS_DB_CORE_CHECK(scalar(_db, "SELECT COUNT(*) FROM core_tables "
                                      "WHERE score = 3 AND "
                                      "name = 'unnamed'") == 1);

    // two rows of one key, refused before anything is written
    t.insert_row({ big_value(10), text_value("dup"), db::value(),
                   db::value() });
    D_TESTS_DB_CORE_THROWS(t.commit(),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_CHECK( (t.is_dirty()) &&
                           (t.row_count_remote() == 3) );
    t.refresh();

    // a failure rolled back: the update goes with the insert that clashed
    t.set_where("id = 1");
    t.refresh();
    D_TESTS_DB_CORE_CHECK(t.rows() == 1u);
    t.update_cell(0u, "score", int_value(11));
    t.insert_row({ big_value(10), text_value("clash"), db::value(),
                   db::value() });
    D_TESTS_DB_CORE_THROWS_ANY(t.commit(), db::query_exception);
    D_TESTS_DB_CORE_CHECK( (t.is_dirty()) &&
                           (scalar(_db, "SELECT score FROM core_tables "
                                        "WHERE id = 1") == 10) );

    // inside the program's transaction, undone under a savepoint
    _db.begin_transaction();
    D_TESTS_DB_CORE_THROWS_ANY(t.commit(), db::query_exception);
    D_TESTS_DB_CORE_CHECK( (_db.in_transaction()) &&
                           (scalar(_db, "SELECT COUNT(*) FROM core_tables") ==
                            3) );
    t.remove_row(1u);
    t.commit();
    D_TESTS_DB_CORE_CHECK(scalar(_db, "SELECT score FROM core_tables "
                                      "WHERE id = 1") == 11);
    _db.rollback();
    D_TESTS_DB_CORE_CHECK(scalar(_db, "SELECT score FROM core_tables "
                                      "WHERE id = 1") == 10);
    t.clear_where();
    t.refresh();

    // a page, committed by its key; an offset alone
    t.set_limit(1u);
    t.set_offset(1u);
    t.refresh();
    D_TESTS_DB_CORE_CHECK(t.rows() == 1u);
    t.update_cell(0u, "score", int_value(20));
    t.commit();
    D_TESTS_DB_CORE_CHECK(scalar(_db, "SELECT COUNT(*) FROM core_tables "
                                      "WHERE score = 20") == 1);
    t.set_limit(std::nullopt);
    t.refresh();
    D_TESTS_DB_CORE_CHECK(t.rows() == 2u);
    t.set_offset(std::nullopt);
    t.refresh();
    D_TESTS_DB_CORE_CHECK(t.rows() == 3u);

    // without a key
    db::table_schema plain_schema;

    plain_schema.table_name = "core_plain";
    plain_schema.columns    = { db::column_info("a", db::field_type::integer),
                                db::column_info("b", db::field_type::string) };

    Table plain(_db, plain_schema);

    plain.drop_table();
    plain.create_table();
    plain.insert_row({ int_value(1), text_value("x") });
    plain.insert_row({ int_value(2), text_value("y") });
    plain.insert_row({ int_value(3), text_value("z") });
    plain.commit();
    D_TESTS_DB_CORE_CHECK( (!plain.is_dirty()) &&
                           (plain.rows() == 3u) &&
                           (plain.row_count_remote() == 3) );
    plain.set_where("a >= 2");
    plain.set_order("a");
    plain.refresh();
    D_TESTS_DB_CORE_CHECK(plain.rows() == 2u);
    plain.update_cell(0u, "b", text_value("Y"));
    plain.commit();
    D_TESTS_DB_CORE_CHECK( (scalar(_db, "SELECT COUNT(*) FROM core_plain") ==
                            3) &&
                           (scalar(_db, "SELECT COUNT(*) FROM core_plain "
                                        "WHERE a = 1 AND b = 'x'") == 1) &&
                           (scalar(_db, "SELECT COUNT(*) FROM core_plain "
                                        "WHERE b = 'Y'") == 1) );
    plain.clear_where();
    plain.set_limit(2u);
    plain.refresh();
    plain.update_cell(0u, "b", text_value("w"));
    D_TESTS_DB_CORE_THROWS(plain.commit(),
                           db::query_exception,
                           D_DB_UNSUPPORTED);
    plain.set_limit(std::nullopt);
    plain.refresh();

    // a key changed in place: its child rows follow it, as ON UPDATE
    // CASCADE has them, where a delete and an insert would have taken them
    // with ON DELETE CASCADE
    _db.execute("CREATE TABLE core_parent (id INTEGER NOT NULL, "
                "name VARCHAR(20), PRIMARY KEY (id))");
    _db.execute("CREATE TABLE core_child (id INTEGER NOT NULL, "
                "parent INTEGER, PRIMARY KEY (id), FOREIGN KEY (parent) "
                "REFERENCES core_parent (id) ON UPDATE CASCADE "
                "ON DELETE CASCADE)");
    _db.execute("INSERT INTO core_parent (id, name) VALUES (1, 'one'), "
                "(2, 'two'), (3, 'three')");
    _db.execute("INSERT INTO core_child (id, parent) VALUES (10, 1), "
                "(20, 2), (30, 3)");

    Table parents(_db, std::string("core_parent"));

    parents.fetch_schema();
    parents.set_order("id");
    parents.refresh();

    // a chain: each key the next one up, the last moved first
    for (std::size_t r = 0; r < 3u; ++r)
    {
        parents.update_cell(r, "id", int_value(static_cast<int>(r) + 2));
    }

    parents.commit();
    D_TESTS_DB_CORE_CHECK( (scalar(_db, "SELECT COUNT(*) FROM core_child") ==
                            3) &&
                           (scalar(_db, "SELECT parent FROM core_child "
                                        "WHERE id = 10") == 2) &&
                           (scalar(_db, "SELECT parent FROM core_child "
                                        "WHERE id = 30") == 4) );

    // an exchange, which no vendor writes row by row: refused, nothing
    // written, the cache kept
    parents.update_cell(0u, "id", int_value(3));
    parents.update_cell(1u, "id", int_value(2));
    D_TESTS_DB_CORE_THROWS(parents.commit(),
                           db::query_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_CHECK( (parents.is_dirty()) &&
                           (scalar(_db, "SELECT parent FROM core_child "
                                        "WHERE id = 10") == 2) );
    parents.refresh();

    // a cell changed through a reference is a change; rows reordered are not
    parents.cell(0u, 1u) = text_value("uno");
    D_TESTS_DB_CORE_CHECK(parents.is_dirty());
    parents.commit();
    D_TESTS_DB_CORE_CHECK(scalar(_db, "SELECT COUNT(*) FROM core_parent "
                                      "WHERE name = 'uno'") == 1);
    std::reverse(parents.begin(), parents.end());
    D_TESTS_DB_CORE_CHECK(!parents.is_dirty());

    // a row sorted elsewhere keeps its row read: its key changed in place
    parents.update_cell(0u, "id", int_value(9));
    parents.commit();
    D_TESTS_DB_CORE_CHECK( (scalar(_db, "SELECT parent FROM core_child "
                                        "WHERE id = 30") == 9) &&
                           (scalar(_db, "SELECT COUNT(*) FROM core_child") ==
                            3) );
    _db.execute("DROP TABLE core_child");
    parents.drop_table();

    // a generated column: read, never written, its values read again
    if (_can.generated_columns)
    {
        _db.execute("CREATE TABLE core_gen (id INTEGER PRIMARY KEY, "
                    "a INTEGER, b INTEGER GENERATED ALWAYS AS (a * 2) "
                    "STORED)");

        Table computed(_db, std::string("core_gen"));

        computed.fetch_schema();
        const std::vector<db::column_info>& described_gen =
            computed.get_schema().columns;

        D_TESTS_DB_CORE_CHECK( (described_gen[2].is_generated) &&
                               (!described_gen[1].is_generated) );
        computed.refresh();
        computed.insert_row({ int_value(1), int_value(3), db::value() });
        computed.commit();
        D_TESTS_DB_CORE_CHECK( (computed.rows() == 1u) &&
                               (number(computed.cell(0u, 2u)) == 6) );
        computed.update_cell(0u, "a", int_value(4));
        computed.commit();
        D_TESTS_DB_CORE_CHECK(number(computed.cell(0u, 2u)) == 8);
        D_TESTS_DB_CORE_THROWS(computed.update_cell(0u, "b", int_value(99)),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        computed.cell(0u, 2u) = int_value(99);
        D_TESTS_DB_CORE_CHECK(computed.is_dirty());
        D_TESTS_DB_CORE_THROWS(computed.commit(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_CHECK(scalar(_db, "SELECT b FROM core_gen") == 8);
        computed.refresh();

        db::table_schema copy = computed.get_schema();

        copy.table_name = "core_gen2";

        Table copied(_db, copy);

        D_TESTS_DB_CORE_THROWS(copied.create_table(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        computed.drop_table();
    }

    // the table's columns changed through the layer
    db::column_info extra("extra", db::field_type::integer);

    extra.default_value = "7";
    t.alter_add_column(extra);
    D_TESTS_DB_CORE_CHECK( (t.cols() == 5u) &&
                           (number(t.cell(0u, 4u)) == 7) );
    t.update_cell(0u, "extra", int_value(8));
    D_TESTS_DB_CORE_THROWS(t.alter_drop_column("extra"),
                           db::query_exception,
                           D_DB_UNSUPPORTED);
    t.commit();

    if (_can.rename_column)
    {
        t.alter_rename_column("extra", "more");
        D_TESTS_DB_CORE_CHECK( (t.column_name(4u) == "more") &&
                               (scalar(_db, "SELECT COUNT(*) FROM "
                                            "core_tables WHERE more = 8") ==
                                1) );
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(t.alter_rename_column("extra", "more"),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }

    const std::string added = (_can.rename_column) ? "more" : "extra";

    if (_can.drop_column)
    {
        t.alter_drop_column(added);
        D_TESTS_DB_CORE_CHECK( (t.cols() == 4u) &&
                               (t.rows() == 3u) &&
                               (t.get_row(0u).size() == 4u) );
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(t.alter_drop_column(added),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }

    t.rename_table("core_tables2");
    D_TESTS_DB_CORE_CHECK( (t.table_name() == "core_tables2") &&
                           (t.exists()) &&
                           (t.row_count_remote() == 3) );
    t.rename_table("core_tables");

    // a view
    _db.execute("CREATE VIEW core_v AS SELECT id, name FROM core_tables");

    Table view(_db, std::string("core_v"), db::table_kind::view);

    D_TESTS_DB_CORE_CHECK(view.exists());
    view.refresh();
    D_TESTS_DB_CORE_CHECK( (view.rows() == 3u) &&
                           (view.cols() == 2u) );
    D_TESTS_DB_CORE_THROWS_ANY(view.add_row(), db::query_exception);
    D_TESTS_DB_CORE_THROWS_ANY(view.drop_table(), db::query_exception);
    _db.execute("DROP VIEW core_v");
    D_TESTS_DB_CORE_CHECK(!view.exists());

    // a temporary table
    _db.execute("CREATE TEMPORARY TABLE core_tmp (a INT)");

    Table temporary(_db, std::string("core_tmp"), db::table_kind::temporary);

    D_TESTS_DB_CORE_CHECK(temporary.exists());
    temporary.fetch_schema();
    D_TESTS_DB_CORE_CHECK(temporary.cols() == 1u);
    temporary.add_row();
    temporary.update_cell(0u, 0u, int_value(5));
    temporary.commit();
    D_TESTS_DB_CORE_CHECK( (scalar(_db, "SELECT a FROM core_tmp") == 5) &&
                           (temporary.rows() == 1u) );
    _db.execute("DROP TABLE core_tmp");
    D_TESTS_DB_CORE_CHECK(!temporary.exists());

    // sync: each change committed at once; a stale cache read on access
    db::sync_config at_once;

    at_once.auto_commit = true;
    t.set_sync_config(at_once);
    t.update_cell(0u, "score", int_value(30));
    D_TESTS_DB_CORE_CHECK( (!t.is_dirty()) &&
                           (scalar(_db, "SELECT COUNT(*) FROM core_tables "
                                        "WHERE score = 30") == 1) );
    t.set_sync_config(db::sync_config(db::sync_policy::on_access));
    _db.execute("UPDATE core_tables SET score = 31 WHERE id = 1");
    t.invalidate();
    D_TESTS_DB_CORE_CHECK( (number(t.cell(0u, 2u)) == 31) &&
                           (!t.is_stale()) );
    t.set_sync_config(db::sync_config());

    // the render view, and the fold's two directions
    const db::relational_table_view<Table> seen =
        db::make_relational_table_view(t);

    D_TESTS_DB_CORE_CHECK( (seen.table_name() == "core_tables") &&
                           (seen.rows() == 3u) &&
                           (seen.cols() == t.cols()) &&
                           (seen.has_primary_key()) &&
                           (seen.is_primary_key_column(0u)) &&
                           (!seen.is_primary_key_column(1u)) &&
                           (seen.primary_key_columns() ==
                                std::vector<std::string>{"id"}) &&
                           (seen.column_name(1u) == "name") &&
                           (seen.column_field_type(1u) ==
                                t.get_schema().columns[1].type) &&
                           (!seen.foreign_ref_of(1u).engaged) &&
                           (&seen.cell(0u, 1u) == &t.cell(0u, 1u)) &&
                           (&seen.table() == &t) );
    t.update_cell(0u, "score", int_value(32));
    db::store_table(_db, t);
    D_TESTS_DB_CORE_CHECK( (!t.is_dirty()) &&
                           (scalar(_db, "SELECT score FROM core_tables "
                                        "WHERE id = 1") == 32) );
    t.invalidate();
    db::load_table(_db, t);
    D_TESTS_DB_CORE_CHECK(!t.is_stale());

    // a connection other than the table's is refused, nothing written
    Connection elsewhere;

    t.update_cell(0u, "score", int_value(33));
    D_TESTS_DB_CORE_THROWS(db::store_table(elsewhere, t),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS(db::load_table(elsewhere, t),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_CHECK( (t.is_dirty()) &&
                           (number(t.cell(0u, 2u)) == 33) &&
                           (scalar(_db, "SELECT score FROM core_tables "
                                        "WHERE id = 1") == 32) );
    t.refresh();

    // emptied, then dropped
    t.truncate_table();
    D_TESTS_DB_CORE_CHECK( (t.rows() == 0u) &&
                           (t.row_count_remote() == 0) &&
                           (!t.is_dirty()) );
    t.drop_table();
    D_TESTS_DB_CORE_CHECK( (!t.exists()) &&
                           (t.rows() == 0u) );
    t.drop_table();
    D_TESTS_DB_CORE_THROWS_ANY(t.drop_table(false), db::query_exception);
    D_TESTS_DB_CORE_THROWS_ANY(t.fetch_schema(), db::query_exception);

    // refusals
    D_TESTS_DB_CORE_THROWS(t.rename_table(std::string("a\0b", 3)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);

    db::table_schema bad;

    bad.table_name = "core_bad";
    bad.columns.push_back(db::column_info(std::string("a\0b", 3),
                                          db::field_type::integer));

    Table refused(_db, bad);
    Table no_columns(_db, std::string("core_none"));
    Table unbound;

    D_TESTS_DB_CORE_THROWS(refused.create_table(),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS(no_columns.create_table(),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS(plain.resize(1u, 9u),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_DB_CORE_THROWS_ANY((void)plain.at(99u, 0u), std::out_of_range);
    D_TESTS_DB_CORE_THROWS_ANY(unbound.refresh(), db::connection_exception);
    D_TESTS_DB_CORE_THROWS_ANY((void)unbound.exists(),
                               db::connection_exception);

    // a table moved
    Table moved(std::move(plain));

    D_TESTS_DB_CORE_CHECK( (moved.get_connection() == &_db) &&
                           (plain.get_connection() == nullptr) &&
                           (moved.rows() == 3u) &&
                           (plain.rows() == 0u) );
    moved.drop_table();
    D_TESTS_DB_CORE_CHECK(_db.is_connected());

    return;
}

#if defined(D_TESTS_DB_CORE_SOCKETS)

/*
wait
  Waits as a pending operation says, with poll(2) or WSAPoll, and answers
what came, as d_db_wait; trouble on the socket reads as ready, for the
operation to find, and no socket as everything having come.
*/
inline unsigned
wait(
    const d_db_pending& _pending
)
{
    const unsigned ready = static_cast<unsigned>(D_DB_WAIT_READ) |
                           static_cast<unsigned>(D_DB_WAIT_WRITE);
    struct pollfd  fd;

    // no socket to wait on
    if (_pending.socket == D_DB_SOCKET_NONE)
    {
        return _pending.events & ready;
    }

    std::memset(&fd, 0, sizeof(fd));
#if defined(_WIN32)
    fd.fd     = static_cast<SOCKET>(_pending.socket);
#else
    fd.fd     = static_cast<int>(_pending.socket);
#endif
    fd.events = static_cast<short>(
                    ( ((_pending.events & D_DB_WAIT_READ) != 0u)
                          ? POLLIN
                          : 0 ) |
                    ( ((_pending.events & D_DB_WAIT_WRITE) != 0u)
                          ? POLLOUT
                          : 0 ));

    const int timeout = ((_pending.events & D_DB_WAIT_TIMEOUT) != 0u)
                            ? static_cast<int>(_pending.timeout_ms)
                            : -1;
#if defined(_WIN32)
    const int came    = WSAPoll(&fd, 1u, timeout);
#else
    const int came    = poll(&fd, 1u, timeout);
#endif

    // the deadline first; trouble, for the operation to find
    if (came == 0)
    {
        return static_cast<unsigned>(D_DB_WAIT_TIMEOUT);
    }

    if ( (came < 0) ||
         ((fd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) )
    {
        return _pending.events & ready;
    }

    return ( ((fd.revents & POLLIN) != 0)
                 ? static_cast<unsigned>(D_DB_WAIT_READ)
                 : 0u ) |
           ( ((fd.revents & POLLOUT) != 0)
                 ? static_cast<unsigned>(D_DB_WAIT_WRITE)
                 : 0u );
}

#endif  // D_TESTS_DB_CORE_SOCKETS

/*
drive
  An operation started, where it was not, and driven to its end, as a
program's loop would: its result, or its failure thrown. Without sockets --
SQLite -- every operation is done as it starts.
*/
template<typename Result>
Result
drive(
    db::async_operation<Result>& _operation
)
{
    // started here, unless the caller started it
    if (!_operation.started())
    {
        _operation.start();
    }

    for (long i = 0; ( (!_operation.done()) && (i < 10000000L) ); ++i)
    {
#if defined(D_TESTS_DB_CORE_SOCKETS)
        _operation.resume(wait(_operation.pending()));
#else
        _operation.resume(0u);
#endif
    }

    return _operation.result();
}

/*
first_long
  The first value of rows as an integer, or LLONG_MIN.
*/
inline long long
first_long(
    db::buffered_result_set& _rows
)
{
    // the first row's first value
    if ( (!_rows.next()) ||
         (_rows.column_count() == 0u) )
    {
        return LLONG_MIN;
    }

    const std::optional<re_std::int64_t> value =
        _rows.get_long(static_cast<std::size_t>(0));

    return (value) ? static_cast<long long>(*value) : LLONG_MIN;
}

// has_thread_id, has_backend_pid
//   traits: whether a connection names its session as the MySQL family does
// (get_thread_id) or as PostgreSQL does (get_backend_pid).
template<typename Type,
         typename = void>
struct has_thread_id : std::false_type
{};

template<typename Type>
struct has_thread_id<Type,
                     std::void_t<decltype(
                         std::declval<const Type&>().get_thread_id())>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_backend_pid : std::false_type
{};

template<typename Type>
struct has_backend_pid<Type,
                       std::void_t<decltype(
                           std::declval<const Type&>().get_backend_pid())>>
    : std::true_type
{};

/*
session_id
  The server's id for a connection's session -- the MySQL family's thread
id, PostgreSQL's backend pid -- or 0 for a vendor with neither.
*/
template<typename Connection>
unsigned long
session_id(
    const Connection& _db
)
{
    if constexpr (has_thread_id<Connection>::value)
    {
        return static_cast<unsigned long>(_db.get_thread_id());
    }
    else if constexpr (has_backend_pid<Connection>::value)
    {
        return static_cast<unsigned long>(_db.get_backend_pid());
    }
    else
    {
        (void)_db;

        return 0ul;
    }
}

/*
await_running
  Whether the server runs the dialect's long query on the session of _db,
asked every 20 milliseconds for five seconds on a connection of _make's: a
cancel the server receives before it has begun a statement is lost --
MariaDB discards a KILL QUERY that comes between statements -- so the check
of cancel() waits until the statement runs, as t_mariadb.c's does.
*/
template<typename Connection,
         typename Make>
bool
await_running(
    const Connection& _db,
    const dialect&    _dialect,
    Make&             _make
)
{
    char sql[256] = "";

    (void)std::snprintf(sql,
                        sizeof(sql),
                        _dialect.long_running,
                        session_id(_db));

    try
    {
        Connection watcher = _make();

        watcher.connect();

        for (int i = 0; i < 250; ++i)
        {
            std::unique_ptr<db::buffered_result_set> rows =
                watcher.execute_query(sql);

            if ( (rows) &&
                 (first_long(*rows) == 1) )
            {
                return true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    catch (const std::exception&)
    {
        return false;
    }

    return false;
}

#if D_INTERNAL_DB_ASYNC_AWAIT

// poll_loop
//   class: the smallest loop an operation can be awaited through: one watch
// at a time, waited on with wait(), until none is left.
class poll_loop
{
public:
    void
    watch(
        const d_db_pending& _pending,
        db::db_wakeup       _wakeup
    )
    {
        m_pending = _pending;
        m_wakeup.emplace(_wakeup);

        return;
    }

    void
    run()
    {
        // each wakeup may watch again
        while (m_wakeup)
        {
            const db::db_wakeup woken = *m_wakeup;

            m_wakeup.reset();
#if defined(D_TESTS_DB_CORE_SOCKETS)
            woken(wait(m_pending));
#else
            woken(0u);
#endif
        }

        return;
    }

private:
    d_db_pending                 m_pending {};
    std::optional<db::db_wakeup> m_wakeup;
};

static_assert(db::concepts::db_event_loop<poll_loop>);
static_assert(!db::concepts::db_event_loop<int>);

// task
//   struct: a coroutine that starts at once and runs to its end through the
// loop; one it does not catch is a failed check.
struct task
{
    struct promise_type
    {
        task
        get_return_object() noexcept
        {
            return task{};
        }

        std::suspend_never
        initial_suspend() noexcept
        {
            return {};
        }

        std::suspend_never
        final_suspend() noexcept
        {
            return {};
        }

        void
        return_void() noexcept
        {
            return;
        }

        void
        unhandled_exception() noexcept
        {
            expect(false, "a coroutine let an exception go", __LINE__);

            return;
        }
    };
};

/*
awaited
  The operations awaited: a query's rows, a count, a failure thrown by
co_await; `_finished` once the coroutine reaches its end.
*/
template<typename Connection>
task
awaited(
    Connection&    _db,
    const dialect& _dialect,
    poll_loop&     _loop,
    bool&          _finished
)
{
    auto query = _db.execute_query_async("SELECT 41 + 1");
    std::unique_ptr<db::buffered_result_set> rows = co_await query.on(_loop);

    D_TESTS_DB_CORE_CHECK( (rows) && (first_long(*rows) == 42) );

    auto                  update  = _db.execute_update_async(
                                        "UPDATE core_t SET small = small "
                                        "WHERE name = 'async'");
    const re_std::int64_t changes = co_await update.on(_loop);

    D_TESTS_DB_CORE_CHECK(changes >= 0);

    auto failing = _db.execute_async("SELEC 1");
    bool thrown  = false;

    try
    {
        (void)co_await failing.on(_loop);
    }
    catch (const db::query_exception& e)
    {
        thrown = (e.error_code() == _dialect.syntax_error);
    }

    D_TESTS_DB_CORE_CHECK(thrown);

    _finished = true;

    co_return;
}

#endif  // D_INTERNAL_DB_ASYNC_AWAIT

/*
async_operations
  The operations that do not block (database_async.hpp), driven as a
program's loop drives them. `_make` makes a connection to the harness's
database, and `_refused` one the database refuses, neither connected;
`_statements_wait` is whether a prepared statement's operations work --
false for libmysqlclient, whose are D_DB_UNSUPPORTED.
  Tests the following:
  - an operation does nothing before start(); resume() before it, result()
    before done() or twice, and an empty operation's start() throw
    std::logic_error
  - execute_update_async counts what it inserts, execute_query_async reads
    rows as execute_query does, execute_async gives true; a failure is
    result()'s to throw, with the vendor's code, the connection free after
  - SQL the server takes a while over is under way after start(): every
    other operation of the connection -- SQL, a statement's run, another
    operation, a bind on a statement under way -- throws query_exception
    with the busy code; done, the connection is free. SQLite's are done as
    they start
  - a prepared statement's operations give what its blocking runs give
  - cancel() stops a long statement, soon: the vendor's refusal, or the
    result where the server let the statement finish -- cancelled once the
    server runs it, which a second connection watches for, since a cancel
    that comes before the server has begun the statement is lost
  - connect_async opens a connection as connect() does, and leaves an open
    one open; one refused is connection_exception, the connection in error
  - an operation destroyed under way leaves the connection lost, until
    reconnect()
  - from C++20, each awaited in a coroutine through a minimal loop
*/
template<typename Connection,
         typename Make,
         typename MakeRefused>
void
async_operations(
    Connection&    _db,
    const dialect& _dialect,
    Make           _make,
    MakeRefused    _refused,
    bool           _statements_wait
)
{
    using layer_type = typename Connection::layer_type;

    std::printf("operations that do not block\n");

    // the order of the calls
    {
        db::async_operation<bool> empty;
        auto                      unstarted = _db.execute_async("SELECT 1");

        D_TESTS_DB_CORE_THROWS_ANY(empty.start(), std::logic_error);
        D_TESTS_DB_CORE_CHECK( (!unstarted.started()) &&
                               (!unstarted.done()) &&
                               (unstarted.pending().socket ==
                                D_DB_SOCKET_NONE) );
        D_TESTS_DB_CORE_THROWS_ANY(unstarted.resume(0u), std::logic_error);
        D_TESTS_DB_CORE_THROWS_ANY((void)unstarted.result(),
                                   std::logic_error);
        D_TESTS_DB_CORE_CHECK(drive(unstarted));
        D_TESTS_DB_CORE_THROWS_ANY((void)unstarted.result(),
                                   std::logic_error);
        D_TESTS_DB_CORE_THROWS_ANY(unstarted.start(), std::logic_error);
    }

    // counts, rows, true; a failure
    {
        auto insert = _db.execute_update_async(
                          "INSERT INTO core_t (name, small) VALUES "
                          "('async', 1), ('async', 2), ('async', 3)");
        auto select = _db.execute_query_async(
                          "SELECT small FROM core_t WHERE name = 'async' "
                          "ORDER BY small");
        auto failed = _db.execute_query_async("SELEC 1");

        D_TESTS_DB_CORE_CHECK(drive(insert) == 3);
        D_TESTS_DB_CORE_CHECK(_db.get_affected_rows() == 3);

        std::unique_ptr<db::buffered_result_set> rows = drive(select);

        D_TESTS_DB_CORE_CHECK( (rows) && (rows->row_count() == 3) &&
                               (first_long(*rows) == 1) );
        D_TESTS_DB_CORE_THROWS((void)drive(failed),
                               db::query_exception,
                               _dialect.syntax_error);
        D_TESTS_DB_CORE_CHECK(_db.execute("SELECT 1"));
    }

    // a prepared statement's, as its blocking runs
    {
        auto select = _db.prepare("SELECT small FROM core_t WHERE name = "
                                  "'async' ORDER BY small");
        auto query  = select->execute_query_async();
        auto count  = select->execute_update_async();

        // libmysqlclient runs none without waiting
        if (!_statements_wait)
        {
            D_TESTS_DB_CORE_THROWS((void)drive(query),
                                   db::query_exception,
                                   D_DB_UNSUPPORTED);
        }
        else
        {
            std::unique_ptr<db::buffered_result_set> rows = drive(query);

            D_TESTS_DB_CORE_CHECK( (rows) && (rows->row_count() == 3) &&
                                   (first_long(*rows) == 1) );
            D_TESTS_DB_CORE_CHECK(drive(count) == 3);
        }

        D_TESTS_DB_CORE_CHECK(select->execute_query()->row_count() == 3);
    }

    // SQL the server takes a while over: under way, the connection held
    if constexpr (layer_type::holds_connection)
    {
        auto slow   = _db.execute_query_async(_dialect.slow_query);
        auto other  = _db.prepare("SELECT 1");
        auto second = _db.execute_update_async("SELECT 1");

        slow.start();
        D_TESTS_DB_CORE_CHECK( (slow.started()) && (!slow.done()) &&
                               (slow.pending().socket != D_DB_SOCKET_NONE) );
        D_TESTS_DB_CORE_THROWS(_db.execute("SELECT 1"),
                               db::query_exception,
                               layer_type::busy_code);
        D_TESTS_DB_CORE_THROWS((void)other->execute_update(),
                               db::query_exception,
                               layer_type::busy_code);
        second.start();
        D_TESTS_DB_CORE_CHECK(second.done());
        D_TESTS_DB_CORE_THROWS((void)second.result(),
                               db::query_exception,
                               layer_type::busy_code);
        D_TESTS_DB_CORE_CHECK(first_long(*drive(slow)) == 0);
        D_TESTS_DB_CORE_CHECK(other->execute_update() >= 0);

        // a statement's own operation holds its values
        if (_statements_wait)
        {
            auto statement = _db.prepare(_dialect.slow_query);
            auto running   = statement->execute_query_async();

            running.start();
            D_TESTS_DB_CORE_CHECK(!running.done());
            D_TESTS_DB_CORE_THROWS(statement->clear_parameters(),
                                   db::query_exception,
                                   layer_type::busy_code);
            D_TESTS_DB_CORE_THROWS(_db.execute("SELECT 1"),
                                   db::query_exception,
                                   layer_type::busy_code);
            D_TESTS_DB_CORE_CHECK(first_long(*drive(running)) == 0);
            statement->clear_parameters();
        }

        // cancelled, soon, once the server runs it (await_running)
        auto long_one = _db.execute_query_async(_dialect.long_query);
        bool stopped  = false;

        long_one.start();
        D_TESTS_DB_CORE_CHECK(await_running(_db, _dialect, _make));

        const auto began = std::chrono::steady_clock::now();

        long_one.cancel();

        try
        {
            (void)drive(long_one);
            stopped = true;
        }
        catch (const db::query_exception& e)
        {
            stopped = _dialect.cancelled(e.error_code());
        }

        D_TESTS_DB_CORE_CHECK(stopped);
        D_TESTS_DB_CORE_CHECK(std::chrono::steady_clock::now() - began <
                              std::chrono::seconds(4));
        D_TESTS_DB_CORE_CHECK(_db.execute("SELECT 1"));
    }
    else
    {
        auto at_once = _db.execute_query_async("SELECT 1");

        at_once.start();
        D_TESTS_DB_CORE_CHECK(at_once.done());
        D_TESTS_DB_CORE_CHECK(at_once.result()->row_count() == 1);
    }

#if D_INTERNAL_DB_ASYNC_AWAIT
    // awaited
    {
        poll_loop loop;
        bool      finished = false;

        awaited(_db, _dialect, loop, finished);
        loop.run();
        D_TESTS_DB_CORE_CHECK(finished);
    }
#endif  // D_INTERNAL_DB_ASYNC_AWAIT

    // connecting
    {
        Connection fresh   = _make();
        Connection refused = _refused();
        auto       opening = fresh.connect_async();

        drive(opening);
        D_TESTS_DB_CORE_CHECK( (fresh.is_connected()) &&
                               (fresh.execute_query("SELECT 1")->row_count() ==
                                1) );

        auto again = fresh.connect_async();

        again.start();
        D_TESTS_DB_CORE_CHECK( (again.done()) && (fresh.is_connected()) );
        again.result();

        auto failing = refused.connect_async();

        D_TESTS_DB_CORE_THROWS_ANY(drive(failing), db::connection_exception);
        D_TESTS_DB_CORE_CHECK(refused.get_state() ==
                              db::connection_state::error);
    }

    // abandoned under way: the connection lost until reconnect()
    if constexpr (layer_type::holds_connection)
    {
        {
            auto abandoned = _db.execute_query_async(_dialect.slow_query);

            abandoned.start();
        }

        D_TESTS_DB_CORE_CHECK(_db.get_state() == db::connection_state::error);
        D_TESTS_DB_CORE_THROWS_ANY(_db.execute("SELECT 1"),
                                   db::connection_exception);
        _db.reconnect();
        D_TESTS_DB_CORE_CHECK( (_db.is_connected()) &&
                               (_db.execute("SELECT 1")) );
    }

    _db.execute("DELETE FROM core_t WHERE name = 'async'");

    return;
}

/*
summary
  The harness's last line: the counts, what it ran against, the level.
*/
inline int
summary(
    const std::string& _against
)
{
    std::printf("%d checks, %d failed (%s, as C++ %ld)\n",
                g_checks,
                g_failures,
                _against.c_str(),
                static_cast<long>(__cplusplus));

    return (g_failures == 0) ? 0 : 1;
}

}  // namespace d_tests_db_core


#endif  // DJINTERP_TESTS_CORE_DB_T_DB_CORE_HPP
