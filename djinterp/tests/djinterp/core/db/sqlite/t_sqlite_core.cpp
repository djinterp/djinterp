/*******************************************************************************
* djinterp [core]                                              t_sqlite_core.cpp
*
*   core/db's SQLite connection, sqlite_connection, over the SQLite layer:
* the operations every vendor shares (t_db_core.hpp), on an in-memory
* database, database_table among them, and what sqlite_connection adds --
* opening files, the journal, PRAGMAs, attached databases, the schema,
* transaction modes, backups -- with files in the directory it is given, and
* what sqlite_table adds: attached databases, STRICT and WITHOUT ROWID
* tables, upkeep. Over a layer built without SQLite (D_CFG_SQLITE 0) it
* checks that nothing opens, and that a connection that did not open, and a
* table on it, refuse every operation.
*   Below C++17 core/db compiles to nothing, and so does this test.
*
*   Build: compile the layer's C sources as C, src/djinterp/core/db/
* database.cpp, database_statement.cpp and sqlite/sqlite.cpp and this file as
* C++17 or later, and link them with SQLite -- ci/check_db_sqlite.sh does.
*
*
* path:      /tests/djinterp/core/db/sqlite/t_sqlite_core.cpp
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
#include <cstdio>   // std::remove
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../inc/djinterp/core/db/sqlite/sqlite.hpp"  // under test
#include "../../../../../inc/djinterp/core/db/sqlite/sqlite_table.hpp"
                                                         // under test
#include "../t_db_core.hpp"  // the shared checks


namespace
{

namespace db = djinterp;

// RESULT_ERROR, RESULT_CANTOPEN, RESULT_CONSTRAINT
//   constants: the SQLite result codes the checks compare with, by value,
// as sqlite3.h names them.
const int RESULT_ERROR      = 1;   // SQLITE_ERROR
const int RESULT_CANTOPEN   = 14;  // SQLITE_CANTOPEN
const int RESULT_CONSTRAINT = 19;  // SQLITE_CONSTRAINT

/*
unique_violation
  SQLite's constraint failure, primary or extended.
*/
bool
unique_violation(
    int _code
)
{
    return ( (_code & 0xFF) == RESULT_CONSTRAINT );
}

// g_dialect
//   SQLite's SQL and answers for the shared checks: an INTEGER PRIMARY KEY
// is the rowid, and every integer column 64 bits, as SQLite stores them; a
// point in time is stored as the UTC text bound.
const d_tests_db_core::dialect g_dialect =
{
    "SQLite",
    "CREATE TABLE core_t (id INTEGER PRIMARY KEY, name TEXT, flag BOOLEAN, "
    "small INT, real_value REAL, bytes BLOB, moment DATETIME)",
    "1",
    "0",
    "INSERT INTO core_t (name, flag, small, real_value, bytes, moment) "
    "VALUES (?, ?, ?, ?, ?, ?)",
    "SELECT name, small FROM core_t WHERE id = ?",
    "SELECT moment FROM core_t WHERE id = ?",
    "1971-01-01 00:00:00.25",
    db::field_type::big_integer,
    db::field_type::big_integer,
    RESULT_ERROR,
    &unique_violation
};

/*
d_tests_sqlite_core_opening
  Opening and closing.
  Tests the following:
  - each constructor names the file to open; connect opens it, and the
    state, the vendor and the library's version follow
  - ping, a native handle, is_in_memory; disconnect, close and reconnect
  - open names another file, which is made; open with flags opens read only
  - a file that cannot be opened: connection_exception with
    SQLITE_CANTOPEN, the connection in error, its message naming the path
  - a custom option SQLite has no setting for, D_DB_UNSUPPORTED; a URI's
    scheme passes
*/
void
d_tests_sqlite_core_opening(
    const std::string& _directory
)
{
    std::printf("opening\n");

    const std::string path = _directory + "/t_sqlite_core.db";
    db::sqlite_connection memory(":memory:");

    (void)std::remove(path.c_str());
    D_TESTS_DB_CORE_CHECK(memory.get_state() ==
                          db::connection_state::disconnected);
    memory.connect();
    D_TESTS_DB_CORE_CHECK( (memory.is_connected()) &&
                           (memory.ping()) &&
                           (memory.is_in_memory()) );
    D_TESTS_DB_CORE_CHECK( (memory.get_native_handle() != nullptr) &&
                           (memory.get_database_type() ==
                            db::database_type::sqlite) &&
                           (std::string(memory.get_vendor_name()) ==
                            "sqlite") );
    D_TESTS_DB_CORE_CHECK(!memory.get_server_version().empty());
    memory.close();
    D_TESTS_DB_CORE_CHECK( (!memory.is_connected()) &&
                           (!memory.ping()) &&
                           (memory.get_native_handle() == nullptr) );
    memory.reconnect();
    D_TESTS_DB_CORE_CHECK(memory.is_connected());

    // a file, made, written, and opened read only
    db::sqlite_connection file;

    file.open(path);
    D_TESTS_DB_CORE_CHECK( (file.is_connected()) &&
                           (!file.is_in_memory()) &&
                           (file.get_database_name() == path) );
    file.execute("CREATE TABLE kept (x INTEGER)");
    file.open(path, db::sqlite_open_flag::read_only);
    D_TESTS_DB_CORE_CHECK(file.has_table("kept"));
    D_TESTS_DB_CORE_THROWS_ANY(file.execute("INSERT INTO kept VALUES (1)"),
                               db::query_exception);
    file.close();

    db::connection_config generic;

    generic.database = path;

    db::sqlite_connection from_generic(generic);

    from_generic.connect();
    D_TESTS_DB_CORE_CHECK(from_generic.has_table("kept"));

    db::sqlite_connect_config configured(path);

    configured.journal_mode = db::sqlite_journal_mode::mode_truncate;
    configured.pragmas["user_version"] = "7";

    db::sqlite_connection from_config(configured);

    from_config.connect();
    D_TESTS_DB_CORE_CHECK( (from_config.get_journal_mode() == "truncate") &&
                           (from_config.get_pragma("user_version") == "7") );

    // what cannot open
    db::sqlite_connection nowhere(_directory + "/no/such/dir/x.db");

    try
    {
        nowhere.connect();
        D_TESTS_DB_CORE_CHECK(false);
    }
    catch (const db::connection_exception& e)
    {
        D_TESTS_DB_CORE_CHECK( (e.error_code() == RESULT_CANTOPEN) &&
                               (std::string(e.what()).find("no/such/dir") !=
                                std::string::npos) );
    }

    D_TESTS_DB_CORE_CHECK(nowhere.get_state() == db::connection_state::error);

    generic.database                 = ":memory:";
    generic.custom_options["scheme"] = "sqlite";

    db::sqlite_connection schemed(generic);

    schemed.connect();
    D_TESTS_DB_CORE_CHECK(schemed.is_connected());
    generic.custom_options["mode"] = "ro";

    db::sqlite_connection optioned(generic);

    D_TESTS_DB_CORE_THROWS(optioned.connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    from_generic.disconnect();
    from_config.disconnect();
    (void)std::remove(path.c_str());
    (void)std::remove((path + "-journal").c_str());

    return;
}

/*
d_tests_sqlite_core_settings
  The journal, PRAGMAs, foreign keys and the busy timeout.
  Tests the following:
  - an in-memory database's journal is memory, and keeps it; a file takes
    WAL, and checkpoints in each mode, TRUNCATE D_DB_UNSUPPORTED before
    3.8.8; a mode SQLite does not have, and a checkpoint mode outside 0 to
    3, D_DB_INVALID_ARGUMENT
  - a PRAGMA set and read; one that reports nothing, D_DB_NOT_FOUND
  - foreign keys enforced when on, and not when off
  - the busy timeout set; a PRAGMA's name holding a NUL refused
*/
void
d_tests_sqlite_core_settings(
    const std::string& _directory
)
{
    std::printf("settings\n");

    const std::string     path = _directory + "/t_sqlite_core_wal.db";
    db::sqlite_connection memory(":memory:");

    memory.connect();
    D_TESTS_DB_CORE_CHECK(memory.get_journal_mode() == "memory");
    D_TESTS_DB_CORE_THROWS(memory.set_journal_mode("WAL"),
                           db::query_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_THROWS(memory.set_journal_mode("sideways"),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    memory.set_pragma("user_version", "42");
    D_TESTS_DB_CORE_CHECK(memory.get_pragma("user_version") == "42");
    D_TESTS_DB_CORE_THROWS((void)memory.get_pragma("no_such_pragma"),
                           db::query_exception,
                           D_DB_NOT_FOUND);
    D_TESTS_DB_CORE_THROWS(memory.set_pragma(std::string("a\0", 2), "1"),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);

    // foreign keys
    memory.execute("CREATE TABLE parent (id INTEGER PRIMARY KEY)");
    memory.execute("CREATE TABLE child (p INTEGER REFERENCES parent (id))");
    D_TESTS_DB_CORE_CHECK(memory.get_pragma("foreign_keys") == "1");
    D_TESTS_DB_CORE_THROWS_ANY(memory.execute("INSERT INTO child VALUES (9)"),
                               db::query_exception);
    memory.set_foreign_keys(false);
    D_TESTS_DB_CORE_CHECK(memory.execute_update(
                              "INSERT INTO child VALUES (9)") == 1);
    memory.set_busy_timeout(250);
    D_TESTS_DB_CORE_CHECK(memory.get_pragma("busy_timeout") == "250");

    // WAL, on a file
    db::sqlite_connection file(path);

    (void)std::remove(path.c_str());
    file.connect();
    file.enable_wal();
    D_TESTS_DB_CORE_CHECK(file.get_journal_mode() == "wal");
    file.execute("CREATE TABLE w (x INTEGER)");

    for (int mode = 0; mode < 3; ++mode)
    {
        file.checkpoint(mode);
    }

    // TRUNCATE, from 3.8.8
    if (db::face::sqlite::library_version_number() >= 3008008)
    {
        file.checkpoint(3);
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(file.checkpoint(3),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }

    D_TESTS_DB_CORE_THROWS(file.checkpoint(4),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);
    file.set_journal_mode("delete");
    D_TESTS_DB_CORE_CHECK(file.get_journal_mode() == "delete");
    file.close();
    (void)std::remove(path.c_str());

    return;
}

/*
d_tests_sqlite_core_schema
  Attached databases, the schema, transaction modes and backups.
  Tests the following:
  - a file attached under a name, its tables reachable through it, then
    detached
  - has_table and list_tables, sorted; a name holding a NUL refused
  - each transaction mode begins a transaction
  - a backup to a file holds what the database held, and a backup from it
    puts that back
*/
void
d_tests_sqlite_core_schema(
    const std::string& _directory
)
{
    std::printf("schema and backups\n");

    const std::string     other  = _directory + "/t_sqlite_core_other.db";
    const std::string     backup = _directory + "/t_sqlite_core_backup.db";
    db::sqlite_connection memory(":memory:");

    (void)std::remove(other.c_str());
    (void)std::remove(backup.c_str());
    memory.connect();
    memory.execute("CREATE TABLE zeta (x INTEGER)");
    memory.execute("CREATE TABLE alpha (x INTEGER)");
    D_TESTS_DB_CORE_CHECK( (memory.has_table("zeta")) &&
                           (!memory.has_table("nope")) );
    D_TESTS_DB_CORE_CHECK(memory.list_tables() ==
                          (std::vector<std::string>{ "alpha", "zeta" }));
    D_TESTS_DB_CORE_THROWS((void)memory.has_table(std::string("a\0", 2)),
                           db::query_exception,
                           D_DB_INVALID_ARGUMENT);

    memory.attach_database(other, "other");
    memory.execute("CREATE TABLE other.remote (x INTEGER)");
    D_TESTS_DB_CORE_CHECK(memory.execute_update(
                              "INSERT INTO other.remote VALUES (1)") == 1);
    memory.detach_database("other");
    D_TESTS_DB_CORE_THROWS_ANY(memory.execute("SELECT * FROM other.remote"),
                               db::query_exception);

    // the three modes
    memory.begin_deferred();
    D_TESTS_DB_CORE_CHECK(memory.in_transaction());
    memory.rollback();
    memory.begin_immediate();
    D_TESTS_DB_CORE_CHECK(memory.in_transaction());
    memory.rollback();
    memory.begin_exclusive();
    D_TESTS_DB_CORE_CHECK(memory.in_transaction());
    memory.commit();
    D_TESTS_DB_CORE_CHECK(memory.get_isolation_level() ==
                          db::isolation_level::serializable);

    // backups, both ways
    memory.execute("INSERT INTO alpha VALUES (5)");
    memory.backup_to(backup);
    memory.execute("DELETE FROM alpha");
    memory.backup_from(backup);
    D_TESTS_DB_CORE_CHECK(std::get<re_std::int64_t>(
                              *memory.execute_scalar(
                                  "SELECT COUNT(*) FROM alpha")) == 1);
    D_TESTS_DB_CORE_CHECK(memory.get_face().is_open());
    memory.close();
    (void)std::remove(other.c_str());
    (void)std::remove(backup.c_str());

    return;
}

/*
d_tests_sqlite_core_tables
  What sqlite_table adds to database_table.
  Tests the following:
  - a table in an attached database, by the name ATTACH gave it: created
    there and not in main, written, analyzed, reindexed; its database
    vacuumed, D_DB_UNSUPPORTED before 3.15.0
  - STRICT, from 3.37.0, refusing text in an INTEGER column; WITHOUT ROWID,
    from 3.8.2; each D_DB_UNSUPPORTED before
  - a table moved keeps its options
*/
void
d_tests_sqlite_core_tables(
    void
)
{
    std::printf("sqlite_table\n");

    const int             library = db::face::sqlite::library_version_number();
    db::sqlite_connection memory(":memory:");
    db::table_schema      schema;

    memory.connect();
    memory.attach_database(":memory:", "aux");
    schema.table_name = "notes";
    schema.columns    = { db::column_info("id",
                                          db::field_type::big_integer,
                                          false,
                                          true),
                          db::column_info("body",
                                          db::field_type::string,
                                          false) };

    db::sqlite_table<> notes(memory, schema);

    notes.set_attach_alias("aux");
    D_TESTS_DB_CORE_CHECK( (notes.get_attach_alias() == "aux") &&
                           (notes.qualified_table_name() ==
                            "\"aux\".\"notes\"") );
    notes.create_table();
    D_TESTS_DB_CORE_CHECK( (notes.exists()) &&
                           (!memory.has_table("notes")) );
    notes.insert_row({ db::value(), d_tests_db_core::text_value("hello") });
    notes.commit();
    D_TESTS_DB_CORE_CHECK( (notes.rows() == 1u) &&
                           (d_tests_db_core::scalar(
                                memory,
                                "SELECT COUNT(*) FROM aux.notes") == 1) );
    notes.analyze_table();
    notes.reindex_table();

    // the attached database vacuumed, from 3.15.0
    if (library >= 3015000)
    {
        notes.vacuum_database();
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(notes.vacuum_database(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }

    // STRICT, from 3.37.0
    db::table_schema strict_schema;

    strict_schema.table_name = "strict_t";
    strict_schema.columns    = { db::column_info("n",
                                                 db::field_type::integer) };

    db::sqlite_table<> strict(memory, strict_schema);

    strict.set_strict(true);
    D_TESTS_DB_CORE_CHECK(strict.is_strict());

    if (library >= 3037000)
    {
        strict.create_table();
        strict.insert_row({ d_tests_db_core::text_value("not a number") });
        D_TESTS_DB_CORE_THROWS_ANY(strict.commit(), db::query_exception);
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(strict.create_table(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }

    // WITHOUT ROWID, from 3.8.2
    db::sqlite_table<> keyed(memory, schema);

    keyed.set_without_rowid(true);

    if (library >= 3008002)
    {
        keyed.create_table();
        D_TESTS_DB_CORE_CHECK(d_tests_db_core::scalar(
                                  memory,
                                  "SELECT COUNT(*) FROM sqlite_master WHERE "
                                  "name = 'notes' AND sql LIKE "
                                  "'%WITHOUT ROWID%'") == 1);
    }
    else
    {
        D_TESTS_DB_CORE_THROWS(keyed.create_table(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }

    // moved, with its options
    db::sqlite_table<> moved(std::move(keyed));

    D_TESTS_DB_CORE_CHECK( (moved.is_without_rowid()) &&
                           (!moved.is_strict()) &&
                           (moved.get_connection() == &memory) );
    memory.close();

    return;
}

/*
d_tests_sqlite_core_stubs
  Over a layer built without SQLite.
  Tests the following:
  - connect throws connection_exception with D_DB_UNSUPPORTED, and leaves
    the connection in error
  - every operation then refuses, not connected, and so does a table's
*/
void
d_tests_sqlite_core_stubs(
    void
)
{
    std::printf("over the stubs\n");

    db::sqlite_connection memory(":memory:");

    D_TESTS_DB_CORE_THROWS(memory.connect(),
                           db::connection_exception,
                           D_DB_UNSUPPORTED);
    D_TESTS_DB_CORE_CHECK( (memory.get_state() ==
                            db::connection_state::error) &&
                           (!memory.is_connected()) );
    D_TESTS_DB_CORE_THROWS_ANY(memory.execute("SELECT 1"),
                               db::connection_exception);
    D_TESTS_DB_CORE_THROWS_ANY((void)memory.prepare("SELECT 1"),
                               db::connection_exception);

    // a table on it
    db::sqlite_table<> notes(memory, std::string("notes"));

    D_TESTS_DB_CORE_CHECK(!notes.is_connected());
    D_TESTS_DB_CORE_THROWS_ANY(notes.refresh(), db::connection_exception);
    D_TESTS_DB_CORE_THROWS_ANY(notes.create_table(),
                               db::connection_exception);

    return;
}

}  // namespace

/*
main
  Over SQLite, the shared checks on an in-memory database and SQLite's own
with files in the directory argv[1] names, "." when it is not given; over
the stubs, what the stubs report.
*/
int
main(
    int    _argc,
    char** _argv
)
{
    const bool        stubs     = (db::face::sqlite::library_version_number() ==
                                   0);
    const std::string directory = (_argc > 1) ? _argv[1] : ".";

    // over SQLite, or over the stubs
    if (stubs)
    {
        d_tests_sqlite_core_stubs();

        return d_tests_db_core::summary("built without SQLite");
    }

    db::sqlite_connection memory(":memory:");

    memory.connect();
    d_tests_db_core::queries(memory, g_dialect);
    d_tests_db_core::statements(memory, g_dialect);
    d_tests_db_core::transactions(memory);
    d_tests_db_core::errors(memory, g_dialect);
    d_tests_db_core::lifetimes<db::sqlite_connection>(
        []() { return db::sqlite_connection(":memory:"); });
    d_tests_db_core::tables<db::database_table<db::sqlite_connection>>(
        memory,
        g_dialect,
        d_tests_db_core::table_abilities
        {
            db::face::sqlite::library_version_number() >= 3025000,
            db::face::sqlite::library_version_number() >= 3035000,
            db::face::sqlite::library_version_number() >= 3031000
        });
    d_tests_sqlite_core_opening(directory);
    d_tests_sqlite_core_settings(directory);
    d_tests_sqlite_core_schema(directory);
    d_tests_sqlite_core_tables();

    return d_tests_db_core::summary(std::string("SQLite ") +
                                    db::face::sqlite::library_version());
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
