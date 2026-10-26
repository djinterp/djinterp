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
*                                                            revised: 2026.10.25
*******************************************************************************/
// djinterp
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER
// std
#include <cstdio>  // std::printf

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstdio>       // std::remove
#include <memory>       // std::unique_ptr
#include <string>       // std::string
#include <type_traits>  // std::is_same_v
#include <vector>       // std::vector
// djinterp
#include "../../../../../inc/djinterp/core/db/sqlite/sqlite.hpp"  // under test
#include "../../../../../inc/djinterp/core/db/sqlite/sqlite_table.hpp"
                                                         // under test
#include "../t_db_core.hpp"  // the shared checks


namespace
{

namespace db = djinterp;

// the operations that do not block, as a trait from C++17 (database_
// connection.hpp, section 2): SQLite's, done as they start
static_assert(db::has_async_operations<db::sqlite_connection>::value);
static_assert(!db::has_async_operations<int>::value);

// the concepts (C++20) -- djinterp::concepts' own, every one of which the
// class satisfies, none of which int does; and a former name, which
// forwards to its concept for one release
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
using concepts_tested = db::sqlite_connection;

static_assert(db::concepts::sqlite_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_connection<int>);
static_assert(db::concepts::async_connection<concepts_tested>);
static_assert(!db::concepts::async_connection<int>);
static_assert(db::concepts::sqlite_openable_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_openable_connection<int>);
static_assert(db::concepts::sqlite_flag_openable_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_flag_openable_connection<int>);
static_assert(db::concepts::sqlite_closable_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_closable_connection<int>);
static_assert(db::concepts::sqlite_basic_file_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_basic_file_connection<int>);
static_assert(db::concepts::sqlite_journal_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_journal_connection<int>);
static_assert(db::concepts::sqlite_pragma_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_pragma_connection<int>);
static_assert(db::concepts::sqlite_backup_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_backup_connection<int>);
static_assert(db::concepts::sqlite_attach_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_attach_connection<int>);
static_assert(db::concepts::sqlite_schema_query_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_schema_query_connection<int>);
static_assert(
    db::concepts::sqlite_extension_loading_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_extension_loading_connection<int>);
static_assert(db::concepts::sqlite_serialization_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_serialization_connection<int>);
static_assert(db::concepts::sqlite_busy_timeout_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_busy_timeout_connection<int>);
static_assert(db::concepts::sqlite_checkpoint_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_checkpoint_connection<int>);
static_assert(db::concepts::sqlite_attachable_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_attachable_connection<int>);
static_assert(db::concepts::sqlite_detachable_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_detachable_connection<int>);
static_assert(
    db::concepts::sqlite_schema_table_query_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_schema_table_query_connection<int>);
static_assert(
    db::concepts::sqlite_table_name_query_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_table_name_query_connection<int>);
static_assert(
    db::concepts::sqlite_extension_loadable_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_extension_loadable_connection<int>);
static_assert(
    db::concepts::sqlite_extension_toggle_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_extension_toggle_connection<int>);
static_assert(db::concepts::sqlite_serializable_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_serializable_connection<int>);
static_assert(db::concepts::sqlite_deserializable_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_deserializable_connection<int>);
static_assert(
    db::concepts::sqlite_transaction_modes_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_transaction_modes_connection<int>);
static_assert(
    db::concepts::sqlite_deferred_transaction_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_deferred_transaction_connection<int>);
static_assert(
    db::concepts::sqlite_immediate_transaction_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_immediate_transaction_connection<int>);
static_assert(
    db::concepts::sqlite_exclusive_transaction_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_exclusive_transaction_connection<int>);
static_assert(db::concepts::sqlite_journaling_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_journaling_connection<int>);
static_assert(db::concepts::sqlite_pragmatic_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_pragmatic_connection<int>);
static_assert(db::concepts::sqlite_backup_capable_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_backup_capable_connection<int>);
static_assert(db::concepts::sqlite_attached_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_attached_connection<int>);
static_assert(db::concepts::sqlite_full_connection<concepts_tested>);
static_assert(!db::concepts::sqlite_full_connection<int>);
static_assert(db::concepts::sqlite_connection<const concepts_tested&>);
static_assert(db::concepts::non_sqlite_connection<int>);
static_assert(!db::concepts::non_sqlite_connection<concepts_tested>);
static_assert(db::Sqlite_connection<concepts_tested>);
static_assert(!db::Sqlite_connection<int>);
#endif

// RESULT_ERROR, RESULT_READONLY, RESULT_CANTOPEN, RESULT_CONSTRAINT
//   constants: the SQLite result codes the checks compare with, by value,
// as sqlite3.h names them.
const int RESULT_ERROR      = 1;   // SQLITE_ERROR
const int RESULT_READONLY   = 8;   // SQLITE_READONLY
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
    &unique_violation,
    nullptr,
    nullptr,
    nullptr
};

/*
d_tests_sqlite_core_opening
  Opening and closing.
  Tests the following:
  - each constructor names the file to open; connect opens it, and the
    state, the vendor and the library's version follow
  - ping, a native handle, is_in_memory; disconnect, close and reconnect
  - the file connect(config) names is the configuration's and in memory or
    not as it says: ":memory:", and, with the uri flag, "file::memory:" and
    mode=memory among a URI's parameters, but not mode=memoryx
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

    // the file connect(config) names, and whether it is in memory
    db::sqlite_connection renamed(path);
    db::connection_config in_memory;

    in_memory.database = ":memory:";
    renamed.connect(in_memory);
    D_TESTS_DB_CORE_CHECK( (renamed.is_in_memory()) &&
                           (renamed.get_sqlite_config().file_path ==
                            ":memory:") );
    renamed.close();

    db::sqlite_connect_config uri;

    uri.open_flags = db::sqlite_open_flag::read_write |
                     db::sqlite_open_flag::create     |
                     db::sqlite_open_flag::uri;

    for (const char* name : { "file::memory:",
                              "file:shared?cache=shared&mode=memory",
                              "file:once?mode=memory" })
    {
        uri.file_path = name;
        renamed.set_sqlite_config(uri);
        D_TESTS_DB_CORE_CHECK(renamed.is_in_memory());
    }

    uri.file_path = "file:never?mode=memoryx";
    renamed.set_sqlite_config(uri);
    D_TESTS_DB_CORE_CHECK(!renamed.is_in_memory());

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
d_tests_sqlite_core_support
  What the library loaded can do, asked of it at run time.
  Tests the following:
  - each supports_* answers by the library's release: CTEs (3.8.3), upsert
    (3.24.0), window functions (3.25.0), generated columns (3.31.0),
    RETURNING (3.35.0), STRICT tables (3.37.0); supports_modern_sql the
    first five together
  - JSON and FTS5 as the library's build says, read with PRAGMA
    compile_options; WAL, which no build here leaves out; encryption, which
    none here has; a library built thread-safe, as every build here is
  - the compile-time struct by its name, and by its former name
*/
void
d_tests_sqlite_core_support(
    db::sqlite_connection& _db
)
{
    std::printf("support\n");

    const int library = db::face::sqlite::library_version_number();

    D_TESTS_DB_CORE_CHECK( (db::sqlite_connection::supports_cte() ==
                            (library >= 3008003)) &&
                           (db::sqlite_connection::supports_upsert() ==
                            (library >= 3024000)) &&
                           (db::sqlite_connection::supports_window_functions()
                            == (library >= 3025000)) &&
                           (db::sqlite_connection::supports_generated_columns()
                            == (library >= 3031000)) &&
                           (db::sqlite_connection::supports_returning() ==
                            (library >= 3035000)) &&
                           (db::sqlite_connection::supports_strict_tables() ==
                            (library >= 3037000)) &&
                           (db::sqlite_connection::supports_modern_sql() ==
                            (library >= 3035000)) );

    // the build's options, as the library lists them
    std::vector<std::string>                 options;
    std::unique_ptr<db::buffered_result_set> rows =
        _db.execute_query("PRAGMA compile_options");

    while (rows->next())
    {
        options.push_back(rows->get_string(0u).value_or(""));
    }

    const auto built_with = [&options](const char* _option) -> bool
    {
        // an option, with or without its value ("THREADSAFE=1")
        for (const std::string& option : options)
        {
            if (option.compare(0u, std::string(_option).size(), _option) == 0)
            {
                return true;
            }
        }

        return false;
    };
    const bool json = (library >= 3038000) ? !built_with("OMIT_JSON")
                                           : built_with("ENABLE_JSON1");

    D_TESTS_DB_CORE_CHECK( (!options.empty()) &&
                           (db::sqlite_connection::supports_json() == json) &&
                           (db::sqlite_connection::supports_fts5() ==
                            built_with("ENABLE_FTS5")) &&
                           (db::sqlite_connection::supports_wal()) &&
                           (!db::sqlite_connection::supports_encryption()) &&
                           (db::sqlite_connection::is_thread_safe()) );

    // the compile-time struct, and its former name
    static_assert(std::is_same_v<
                      db::sqlite_connection::client_capabilities,
                      db::sqlite_client_capabilities>,
                  "client_capabilities is the compile-time struct");
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    static_assert(std::is_same_v<db::sqlite_type_support,
                                 db::sqlite_client_capabilities>,
                  "sqlite_type_support is the struct's former name");
#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

    return;
}

/*
d_tests_sqlite_core_bytes
  Databases as bytes, and extensions.
  Tests the following:
  - supports_serialization by the library's release (3.36.0; none of the
    builds here has it before, by option)
  - serialize: the file's header first; no such database, D_DB_NOT_FOUND; a
    name holding a NUL, D_DB_INVALID_ARGUMENT
  - deserialize into a file connection's main: its rows there, is_in_memory
    until the next open, which finds the file as it was; read-only, which
    refuses writes (SQLITE_READONLY); "temp", SQLITE_ERROR
  - where the library cannot, D_DB_UNSUPPORTED; before connecting,
    connection_exception
  - supports_extension_loading, which every build here has; a library that
    is not there, SQLITE_ERROR with SQLite's words; with the test extension
    (DJINTERP_TEST_SQLITE_EXTENSION), its function by the default entry
    point and by a named one, an entry point it lacks refused, and SQL's
    load_extension off until enable_load_extension turns it on (from 3.13.0;
    before, the C API too)
*/
void
d_tests_sqlite_core_bytes(
    const std::string& _directory
)
{
    std::printf("bytes and extensions\n");

    const int         library   = db::face::sqlite::library_version_number();
    const std::string path      = _directory + "/t_sqlite_core_bytes.db";
    const std::string extension =
        d_tests_db_core::setting("DJINTERP_TEST_SQLITE_EXTENSION", "");

    db::sqlite_connection memory(":memory:");
    db::sqlite_connection file(path);

    D_TESTS_DB_CORE_CHECK(db::sqlite_connection::supports_serialization() ==
                          (library >= 3036000));
    D_TESTS_DB_CORE_THROWS_ANY((void)memory.serialize(),
                               db::connection_exception);
    memory.connect();
    memory.execute("CREATE TABLE b (id INTEGER PRIMARY KEY, v TEXT)");
    memory.execute("INSERT INTO b VALUES (1, 'one'), (2, 'two')");
    (void)std::remove(path.c_str());
    file.connect();
    file.execute("CREATE TABLE f (x)");

    // a library without it
    if (!db::sqlite_connection::supports_serialization())
    {
        D_TESTS_DB_CORE_THROWS((void)memory.serialize(),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
        D_TESTS_DB_CORE_THROWS(file.deserialize(
                                   std::vector<re_std::uint8_t>(1u, 0u)),
                               db::query_exception,
                               D_DB_UNSUPPORTED);
    }
    else
    {
        const std::vector<re_std::uint8_t> bytes = memory.serialize();

        D_TESTS_DB_CORE_CHECK( (bytes.size() >= 512u) &&
                               (std::string(bytes.begin(),
                                            bytes.begin() + 15) ==
                                "SQLite format 3") );
        D_TESTS_DB_CORE_THROWS((void)memory.serialize("nope"),
                               db::query_exception,
                               D_DB_NOT_FOUND);
        D_TESTS_DB_CORE_THROWS((void)memory.serialize(std::string("a\0", 2)),
                               db::query_exception,
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_DB_CORE_CHECK(!file.is_in_memory());
        file.deserialize(bytes);
        D_TESTS_DB_CORE_CHECK( (file.is_in_memory()) &&
                               (d_tests_db_core::scalar(
                                    file,
                                    "SELECT id FROM b WHERE v = 'two'") ==
                                2) );
        file.execute("INSERT INTO b VALUES (3, 'three')");
        D_TESTS_DB_CORE_THROWS(file.deserialize(bytes, "temp"),
                               db::query_exception,
                               RESULT_ERROR);
        file.disconnect();
        file.connect();
        D_TESTS_DB_CORE_CHECK( (!file.is_in_memory()) &&
                               (file.has_table("f")) &&
                               (!file.has_table("b")) );
        file.deserialize(bytes, "main", true);
        D_TESTS_DB_CORE_THROWS(file.execute("INSERT INTO b VALUES (4, 'x')"),
                               db::query_exception,
                               RESULT_READONLY);
    }

    file.close();
    (void)std::remove(path.c_str());

    // extensions: a library that is not there
    D_TESTS_DB_CORE_CHECK(db::sqlite_connection::supports_extension_loading());

    // before 3.13.0 the C API needs loading on
    if (library < 3013000)
    {
        D_TESTS_DB_CORE_THROWS(memory.load_extension("t_sqlite_nothing"),
                               db::query_exception,
                               RESULT_ERROR);
        memory.enable_load_extension(true);
    }

    try
    {
        memory.load_extension("t_sqlite_no/such_lib");
        D_TESTS_DB_CORE_CHECK(false);
    }
    catch (const db::query_exception& e)
    {
        D_TESTS_DB_CORE_CHECK( (e.error_code() == RESULT_ERROR) &&
                               (std::string(e.what()).find(
                                    "load_extension: ") !=
                                std::string::npos) );
    }

    // the test extension, where it was built
    if (extension.empty())
    {
        std::printf("  (no extension built)\n");

        return;
    }

    memory.load_extension(extension);
    D_TESTS_DB_CORE_CHECK(d_tests_db_core::scalar(memory,
                                                  "SELECT d_tests_answer()") ==
                          42);
    D_TESTS_DB_CORE_THROWS(memory.load_extension(extension, "d_tests_no_init"),
                           db::query_exception,
                           RESULT_ERROR);
    memory.load_extension(extension, "d_tests_sqlite_extension_init");
    D_TESTS_DB_CORE_CHECK(d_tests_db_core::scalar(memory,
                                                  "SELECT d_tests_named()") ==
                          7);

    // SQL's own switch, from 3.13.0
    if (library >= 3013000)
    {
        db::sqlite_connection other(":memory:");
        const std::string     sql = "SELECT load_extension('" + extension +
                                    "')";

        other.connect();
        D_TESTS_DB_CORE_THROWS_ANY(other.execute(sql), db::query_exception);
        other.enable_load_extension(true);
        other.execute(sql);
        D_TESTS_DB_CORE_CHECK(d_tests_db_core::scalar(
                                  other,
                                  "SELECT d_tests_answer()") == 42);
        other.enable_load_extension(false);
        D_TESTS_DB_CORE_THROWS_ANY(other.execute(sql), db::query_exception);
    }

    return;
}

/*
d_tests_sqlite_core_stubs
  Over a layer built without SQLite.
  Tests the following:
  - connect throws connection_exception with D_DB_UNSUPPORTED, and leaves
    the connection in error
  - every operation then refuses, not connected, and so does a table's
  - there is no library to support anything
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
    D_TESTS_DB_CORE_CHECK( (!db::sqlite_connection::supports_wal()) &&
                           (!db::sqlite_connection::supports_cte()) &&
                           (!db::sqlite_connection::supports_modern_sql()) &&
                           (!db::sqlite_connection::is_thread_safe()) );

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
    d_tests_db_core::streams(memory, g_dialect);
    d_tests_db_core::async_operations(
        memory,
        g_dialect,
        []() -> db::sqlite_connection
        {
            return db::sqlite_connection(":memory:");
        },
        []() -> db::sqlite_connection
        {
            return db::sqlite_connection("/d_tests_no_such_directory/x.db");
        },
        true);
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
    d_tests_sqlite_core_support(memory);
    d_tests_sqlite_core_opening(directory);
    d_tests_sqlite_core_settings(directory);
    d_tests_sqlite_core_schema(directory);
    d_tests_sqlite_core_bytes(directory);
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
