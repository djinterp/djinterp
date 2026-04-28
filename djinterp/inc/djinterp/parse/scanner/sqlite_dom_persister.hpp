/******************************************************************************
* djinterp [dom]                                     sqlite_dom_persister.hpp
*
* SQLite persistence of a cpp_scan_result:
*   This header defines `sqlite_dom_persister`, a templated writer that
* takes the flat aggregate produced by cpp_scanner (cpp_scan_result) and
* persists it to a SQLite database via a connection that conforms to the
* sqlite_connection interface.
*
*   The on-disk schema mirrors the in-memory DOM 1:1 so that the
* round-trip (scan -> persist -> reload -> arena) is lossless and so
* that ad-hoc SQL queries over the catalogue remain natural:
*
*     dom_strings     (id INTEGER PK, value TEXT)
*     dom_nodes       (stable_id INTEGER PK, parent_stable_id INTEGER,
*                      kind, access, storage, qualifiers,
*                      name_id, type_spelling_id, comment_id,
*                      file_id, line, column, is_definition)
*     dom_cpp_nodes   (stable_id INTEGER PK FK,
*                      qualified_name_id, return_type_id, signature_id,
*                      mangled_name_id, underlying_type_id,
*                      param_count, template_param_count,
*                      base_count, member_count)
*     dom_edges       (from_stable_id, to_stable_id)
*
*   All four tables are created idempotently with IF NOT EXISTS.  The
* entire persist runs inside a single IMMEDIATE transaction so that
* large catalogues amortize fsync cost; on failure, the transaction is
* rolled back and the report carries the status.
*
*   The persister is templated on the connection type so it does not
* hard-couple to sqlite::sqlite_connection.  Any type that satisfies the
* structural contract (execute, execute_update, prepare, begin_immediate,
* commit, rollback, table_exists, and a statement type with bind_int /
* bind_int64 / bind_string / bind_null / execute / reset) will work.
* The contract is enforced via deferred static_asserts, mirroring
* dom_writer and scanner_base.
*
*
* path:      /inc/cpp/dom/sqlite_dom_persister.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.17
******************************************************************************/

#ifndef DJINTERP_DOM_SQLITE_PERSISTER_
#define DJINTERP_DOM_SQLITE_PERSISTER_ 1

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "../core/djinterp.hpp"
#include "../database/sqlite/sqlite_traits.hpp"
#include "../scan/cpp_scanner.hpp"
#include "./cpp_dom_node.hpp"
#include "./dom_node.hpp"


NS_DJINTERP
NS_DOM


// ================================================================
//  sqlite_dom_schema
// ================================================================

// sqlite_dom_schema
//   namespace: canonical table names, version marker, and DDL
// for the persisted DOM schema.  Exposed publicly so callers
// may introspect or extend the schema (e.g. add indexes beyond
// the default set).
namespace sqlite_dom_schema
{

// D_DOM_SCHEMA_VERSION
//   constant: monotonic integer bumped whenever the schema
// changes in a non-backward-compatible way.  Stored in the
// schema_meta table so readers can refuse mismatched catalogues.
constexpr std::int32_t D_DOM_SCHEMA_VERSION = 1;

// table names
constexpr const char* table_meta      = "dom_schema_meta";
constexpr const char* table_strings   = "dom_strings";
constexpr const char* table_nodes     = "dom_nodes";
constexpr const char* table_cpp_nodes = "dom_cpp_nodes";
constexpr const char* table_edges     = "dom_edges";

// ddl_meta
//   SQL: schema-version marker table.
constexpr const char* ddl_meta =
    "CREATE TABLE IF NOT EXISTS dom_schema_meta ("
        "key TEXT PRIMARY KEY, "
        "value TEXT NOT NULL"
    ")";

// ddl_strings
//   SQL: interned string pool.  id 0 is reserved for the null
// sentinel; higher ids mirror dom_string_id values from the
// source dom_string_table.
constexpr const char* ddl_strings =
    "CREATE TABLE IF NOT EXISTS dom_strings ("
        "id INTEGER PRIMARY KEY, "
        "value TEXT NOT NULL"
    ")";

// ddl_nodes
//   SQL: the common DOM node fields.  parent_stable_id is
// denormalized into the row so that tree reconstruction is a
// single scan rather than a join over a separate edges table.
constexpr const char* ddl_nodes =
    "CREATE TABLE IF NOT EXISTS dom_nodes ("
        "stable_id         INTEGER PRIMARY KEY, "
        "parent_stable_id  INTEGER, "
        "kind              INTEGER NOT NULL, "
        "access            INTEGER NOT NULL, "
        "storage           INTEGER NOT NULL, "
        "qualifiers        INTEGER NOT NULL, "
        "name_id           INTEGER NOT NULL, "
        "type_spelling_id  INTEGER NOT NULL, "
        "comment_id        INTEGER NOT NULL, "
        "file_id           INTEGER NOT NULL, "
        "line              INTEGER NOT NULL, "
        "column            INTEGER NOT NULL, "
        "is_definition     INTEGER NOT NULL"
    ")";

// ddl_cpp_nodes
//   SQL: the cpp_dom_node-specific extension fields.  Joined
// to dom_nodes by stable_id.  A node without a cpp row is a
// plain dom_node (e.g. pure-C declaration).
constexpr const char* ddl_cpp_nodes =
    "CREATE TABLE IF NOT EXISTS dom_cpp_nodes ("
        "stable_id             INTEGER PRIMARY KEY, "
        "qualified_name_id     INTEGER NOT NULL, "
        "return_type_id        INTEGER NOT NULL, "
        "signature_id          INTEGER NOT NULL, "
        "mangled_name_id       INTEGER NOT NULL, "
        "underlying_type_id    INTEGER NOT NULL, "
        "param_count           INTEGER NOT NULL, "
        "template_param_count  INTEGER NOT NULL, "
        "base_count            INTEGER NOT NULL, "
        "member_count          INTEGER NOT NULL"
    ")";

// ddl_edges
//   SQL: dependency edges — one row per (from, to) pair.  Not
// deduplicated; callers can apply DISTINCT or GROUP BY on
// query if needed.
constexpr const char* ddl_edges =
    "CREATE TABLE IF NOT EXISTS dom_edges ("
        "from_stable_id INTEGER NOT NULL, "
        "to_stable_id   INTEGER NOT NULL"
    ")";

// ddl_indexes
//   SQL: the default supporting indexes.  Keep slim — heavy
// indexing slows the write and is easy to add post-hoc.
constexpr const char* ddl_index_nodes_parent =
    "CREATE INDEX IF NOT EXISTS "
    "idx_dom_nodes_parent ON dom_nodes(parent_stable_id)";

constexpr const char* ddl_index_nodes_file =
    "CREATE INDEX IF NOT EXISTS "
    "idx_dom_nodes_file ON dom_nodes(file_id)";

constexpr const char* ddl_index_nodes_kind =
    "CREATE INDEX IF NOT EXISTS "
    "idx_dom_nodes_kind ON dom_nodes(kind)";

constexpr const char* ddl_index_edges_from =
    "CREATE INDEX IF NOT EXISTS "
    "idx_dom_edges_from ON dom_edges(from_stable_id)";

constexpr const char* ddl_index_edges_to =
    "CREATE INDEX IF NOT EXISTS "
    "idx_dom_edges_to ON dom_edges(to_stable_id)";


// ================================================================
//  prepared-statement SQL
// ================================================================

// sql_insert_string
//   SQL: single-row upsert into dom_strings.
constexpr const char* sql_insert_string =
    "INSERT OR REPLACE INTO dom_strings(id, value) VALUES(?, ?)";

// sql_insert_node
//   SQL: single-row upsert into dom_nodes.  Thirteen columns.
constexpr const char* sql_insert_node =
    "INSERT OR REPLACE INTO dom_nodes("
        "stable_id, parent_stable_id, kind, access, storage, "
        "qualifiers, name_id, type_spelling_id, comment_id, "
        "file_id, line, column, is_definition"
    ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

// sql_insert_cpp_node
//   SQL: single-row upsert into dom_cpp_nodes.  Ten columns.
constexpr const char* sql_insert_cpp_node =
    "INSERT OR REPLACE INTO dom_cpp_nodes("
        "stable_id, qualified_name_id, return_type_id, signature_id, "
        "mangled_name_id, underlying_type_id, param_count, "
        "template_param_count, base_count, member_count"
    ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

// sql_insert_edge
//   SQL: single-row insert into dom_edges.  Not deduplicated.
constexpr const char* sql_insert_edge =
    "INSERT INTO dom_edges(from_stable_id, to_stable_id) "
    "VALUES(?, ?)";

// sql_insert_meta
//   SQL: upsert into the version/meta table.
constexpr const char* sql_insert_meta =
    "INSERT OR REPLACE INTO dom_schema_meta(key, value) "
    "VALUES(?, ?)";

}  // namespace sqlite_dom_schema


// ================================================================
//  sqlite_persister_mode
// ================================================================

// sqlite_persister_mode
//   constants: strategies for reconciling an incoming scan with
// pre-existing rows.  All modes operate inside a single
// transaction.
namespace sqlite_persister_mode
{
    constexpr std::uint8_t append  = 0x00;
    constexpr std::uint8_t replace = 0x01;
    constexpr std::uint8_t upsert  = 0x02;
}


// ================================================================
//  sqlite_persister_status
// ================================================================

typedef std::int32_t sqlite_persister_status;

constexpr sqlite_persister_status DSqlitePersisterSuccess         =  0;
constexpr sqlite_persister_status DSqlitePersisterSchemaFailure   =  1;
constexpr sqlite_persister_status DSqlitePersisterPrepareFailure  =  2;
constexpr sqlite_persister_status DSqlitePersisterBindFailure     =  3;
constexpr sqlite_persister_status DSqlitePersisterExecuteFailure  =  4;
constexpr sqlite_persister_status DSqlitePersisterTxnFailure      =  5;
constexpr sqlite_persister_status DSqlitePersisterRolledBack      =  6;
constexpr sqlite_persister_status DSqlitePersisterUserBase        = 64;


// ================================================================
//  sqlite_persister_config
// ================================================================

// sqlite_persister_config
//   struct: tuning options for the persist operation.
struct sqlite_persister_config
{
    // mode
    //   field: reconciliation strategy (see sqlite_persister_mode).
    // `append`  — INSERT only; fails on duplicate stable_id.
    // `replace` — DELETE all four tables, then INSERT.
    // `upsert`  — default: INSERT OR REPLACE by primary key.
    std::uint8_t    mode;

    // create_schema
    //   field: run the DDL (idempotent IF NOT EXISTS) before
    // writing.  Disable when the schema is known to exist and
    // you want to skip the startup overhead.
    bool            create_schema;

    // create_indexes
    //   field: create the default supporting indexes as part of
    // the schema DDL.  Safe to disable for high-write,
    // low-query workloads.
    bool            create_indexes;

    // write_edges
    //   field: persist the dependency_edges collection.  Skip
    // when only the symbol tree is wanted and edges aren't used.
    bool            write_edges;

    // enable_wal
    //   field: switch the database to WAL journaling before
    // writing.  Ignored if the connection isn't connected to a
    // persistent file.
    bool            enable_wal;

    // foreign_keys
    //   field: enable foreign-key enforcement for the session.
    // We don't declare FKs in the default DDL; setting this
    // just makes any user-added ones stricter.
    bool            foreign_keys;

    sqlite_persister_config()
        : mode          (sqlite_persister_mode::upsert)
        , create_schema (true)
        , create_indexes(true)
        , write_edges   (true)
        , enable_wal    (false)
        , foreign_keys  (false)
    {}
};


// ================================================================
//  sqlite_persister_diagnostic
// ================================================================

// sqlite_persister_diagnostic
//   struct: non-fatal issue descriptor dispatched through the
// persister's on_diagnostic callback.
struct sqlite_persister_diagnostic
{
    sqlite_persister_status     status;
    std::uint64_t               stable_id;
    std::string                 detail;

    sqlite_persister_diagnostic()
        : status   (DSqlitePersisterSuccess)
        , stable_id(0)
        , detail   ()
    {}

    sqlite_persister_diagnostic(sqlite_persister_status  _status,
                                std::uint64_t            _stable_id,
                                const std::string&       _detail)
        : status   (_status)
        , stable_id(_stable_id)
        , detail   (_detail)
    {}
};


// ================================================================
//  sqlite_persister_report
// ================================================================

// sqlite_persister_report
//   struct: cumulative counters and final status for a single
// persist invocation.  Queryable after persist() returns.
struct sqlite_persister_report
{
    sqlite_persister_status     status;
    std::size_t                 strings_written;
    std::size_t                 nodes_written;
    std::size_t                 cpp_nodes_written;
    std::size_t                 edges_written;
    std::size_t                 parents_wired;
    std::size_t                 rows_failed;
    bool                        transaction_committed;

    sqlite_persister_report()
        : status               (DSqlitePersisterSuccess)
        , strings_written      (0)
        , nodes_written        (0)
        , cpp_nodes_written    (0)
        , edges_written        (0)
        , parents_wired        (0)
        , rows_failed          (0)
        , transaction_committed(false)
    {}

    // reset
    //   zeroes all fields.
    void reset()
    {
        status                = DSqlitePersisterSuccess;
        strings_written       = 0;
        nodes_written         = 0;
        cpp_nodes_written     = 0;
        edges_written         = 0;
        parents_wired         = 0;
        rows_failed           = 0;
        transaction_committed = false;

        return;
    }

    // ok
    //   returns true when the persist ran cleanly: status is
    // success, the transaction committed, and no row failed.
    bool ok() const
    {
        return ( (status                == DSqlitePersisterSuccess) &&
                 (rows_failed           == 0)                       &&
                 (transaction_committed == true) );
    }
};


// ================================================================
//  sqlite_persister_callbacks
// ================================================================

// sqlite_persister_callbacks
//   struct: optional progress and diagnostic hooks.
struct sqlite_persister_callbacks
{
    // on_phase_begin
    //   invoked at the start of each phase with its 1-based
    // index and canonical name (1 = schema, 2 = strings,
    // 3 = nodes, 4 = cpp_nodes, 5 = edges).
    std::function<void(int /*_phase*/,
                       const char* /*_name*/)>
        on_phase_begin;

    // on_phase_end
    //   invoked at the end of each phase with the number of
    // rows written.
    std::function<void(int /*_phase*/,
                       std::size_t /*_rows*/)>
        on_phase_end;

    // on_diagnostic
    //   invoked for each non-fatal issue encountered.
    std::function<void(const sqlite_persister_diagnostic& /*_diag*/)>
        on_diagnostic;
};


// ================================================================
//  sqlite_dom_persister
// ================================================================

// sqlite_dom_persister
//   class: generic persister from cpp_scan_result into SQLite.
// Templated on _ConnectionType so it is not hard-coupled to
// djinterp::db::sqlite::sqlite_connection — any connection
// conforming to the structural contract works.
//
//   Required connection methods:
//     - execute(const std::string&)           -> bool
//     - execute_update(const std::string&)    -> int64_t
//     - prepare(const std::string&)           -> unique_ptr<statement>
//     - begin_immediate()                     -> void
//     - commit()                              -> void
//     - rollback()                            -> void
//     - table_exists(const std::string&)      -> bool   [optional]
//     - execute_pragma(const std::string&,
//                      const std::string&)    -> void   [optional]
//
//   Required statement interface (as returned by prepare):
//     - bind_int    (int pos, int       v)    -> void
//     - bind_int64  (int pos, int64_t   v)    -> void
//     - bind_string (int pos, const str&)     -> void
//     - bind_null   (int pos)                 -> void
//     - execute()                             -> void
//     - reset()                               -> void
template<typename _ConnectionType>
class sqlite_dom_persister
{
public:
    using connection_type = _ConnectionType;
    using config_type     = sqlite_persister_config;
    using report_type     = sqlite_persister_report;
    using callbacks_type  = sqlite_persister_callbacks;

    // ========================================================
    //  construction
    // ========================================================

    // sqlite_dom_persister (connection)
    //   constructs a persister over an existing, connected
    // connection.  The connection must outlive the persister.
    explicit sqlite_dom_persister(_ConnectionType& _conn)
        : m_conn     (_conn)
        , m_config   ()
        , m_callbacks()
        , m_report   ()
    {}

    // sqlite_dom_persister (connection + config)
    //   constructs a persister with a non-default config.
    sqlite_dom_persister(_ConnectionType&            _conn,
                         const sqlite_persister_config& _config)
        : m_conn     (_conn)
        , m_config   (_config)
        , m_callbacks()
        , m_report   ()
    {}

    // disable copying — the persister holds a reference and
    // owns transient prepared statements during persist().
    sqlite_dom_persister(const sqlite_dom_persister&)            = delete;
    sqlite_dom_persister& operator=(const sqlite_dom_persister&) = delete;


    // ========================================================
    //  configuration
    // ========================================================

    // config
    //   returns the current configuration.
    const config_type& config() const
    {
        return m_config;
    }

    // set_config
    //   replaces the current configuration.
    void set_config(const config_type& _config)
    {
        m_config = _config;

        return;
    }

    // set_callbacks
    //   installs the progress / diagnostic callbacks.
    void set_callbacks(const callbacks_type& _cb)
    {
        m_callbacks = _cb;

        return;
    }

    // callbacks
    //   returns the currently installed callbacks.
    const callbacks_type& callbacks() const
    {
        return m_callbacks;
    }


    // ========================================================
    //  state
    // ========================================================

    // report
    //   returns the cumulative report block.
    const report_type& report() const
    {
        return m_report;
    }

    // reset_report
    //   zeroes the report counters.
    void reset_report()
    {
        m_report.reset();

        return;
    }


    // ========================================================
    //  persist
    // ========================================================

    // persist
    //   executes the end-to-end write: schema DDL (optional),
    // then the IMMEDIATE transaction spanning strings, nodes,
    // cpp_nodes, and edges.  On any exception the transaction
    // is rolled back and the failure is recorded in the report.
    //
    //   Returns a const reference to the updated report.
    const report_type& persist(const cpp_scanner::result_type& _result)
    {
        // Deferred structural checks.  Mirrors the pattern in
        // scanner_base and dom_writer — only on first use, so
        // incomplete types at class-definition time do not
        // trigger false failures.

        static_assert(
            db::is_detected<db::execute_t,
                            _ConnectionType>::value,
            "sqlite_dom_persister: connection must provide "
            "`execute(const std::string&)`.");

        static_assert(
            db::is_detected<db::sqlite::sqlite_begin_immediate_t,
                            _ConnectionType>::value,
            "sqlite_dom_persister: connection must provide "
            "`begin_immediate()`.");

        static_assert(
            db::is_detected<db::commit_t,
                            _ConnectionType>::value,
            "sqlite_dom_persister: connection must provide "
            "`commit()`.");

        static_assert(
            db::is_detected<db::rollback_t,
                            _ConnectionType>::value,
            "sqlite_dom_persister: connection must provide "
            "`rollback()`.");

        m_report.reset();

        try
        {
            m_apply_session_pragmas();

            if (m_config.create_schema)
            {
                m_begin_phase(1, "schema");
                m_create_schema();
                m_end_phase(1, 0);
            }

            m_conn.begin_immediate();

            try
            {
                if (m_config.mode == sqlite_persister_mode::replace)
                {
                    m_truncate_tables();
                }

                m_write_meta();
                m_write_strings  (_result);
                m_write_nodes    (_result);
                m_write_cpp_nodes(_result);

                if (m_config.write_edges)
                {
                    m_write_edges(_result);
                }

                m_conn.commit();
                m_report.transaction_committed = true;
            }
            catch (...)
            {
                m_try_rollback();

                m_report.status = DSqlitePersisterRolledBack;

                m_emit_diagnostic(sqlite_persister_diagnostic(
                    DSqlitePersisterRolledBack,
                    0,
                    "transaction rolled back due to exception"
                ));

                throw;
            }
        }
        catch (...)
        {
            // Diagnostics were emitted inside; the caller can
            // check m_report.status and m_report.ok().
            if (m_report.status == DSqlitePersisterSuccess)
            {
                m_report.status = DSqlitePersisterExecuteFailure;
            }
        }

        return m_report;
    }


private:
    // ========================================================
    //  session-level setup
    // ========================================================

    // m_apply_session_pragmas
    //   applies the session-scoped PRAGMAs from the config
    // (WAL, foreign_keys).  Best-effort — failures here are
    // non-fatal.
    void m_apply_session_pragmas()
    {
        if (m_config.enable_wal)
        {
            m_exec("PRAGMA journal_mode = WAL");
        }

        if (m_config.foreign_keys)
        {
            m_exec("PRAGMA foreign_keys = ON");
        }

        return;
    }

    // m_create_schema
    //   runs the DDL for every table and optional index.
    // Idempotent — every statement is `... IF NOT EXISTS`.
    void m_create_schema()
    {
        m_exec(sqlite_dom_schema::ddl_meta);
        m_exec(sqlite_dom_schema::ddl_strings);
        m_exec(sqlite_dom_schema::ddl_nodes);
        m_exec(sqlite_dom_schema::ddl_cpp_nodes);
        m_exec(sqlite_dom_schema::ddl_edges);

        if (m_config.create_indexes)
        {
            m_exec(sqlite_dom_schema::ddl_index_nodes_parent);
            m_exec(sqlite_dom_schema::ddl_index_nodes_file);
            m_exec(sqlite_dom_schema::ddl_index_nodes_kind);
            m_exec(sqlite_dom_schema::ddl_index_edges_from);
            m_exec(sqlite_dom_schema::ddl_index_edges_to);
        }

        return;
    }

    // m_truncate_tables
    //   clears every DOM table.  Used by mode=replace.  Runs
    // inside the outer transaction so that a mid-write failure
    // still restores the prior catalogue.
    void m_truncate_tables()
    {
        m_exec_update(std::string("DELETE FROM ") +
                      sqlite_dom_schema::table_edges);
        m_exec_update(std::string("DELETE FROM ") +
                      sqlite_dom_schema::table_cpp_nodes);
        m_exec_update(std::string("DELETE FROM ") +
                      sqlite_dom_schema::table_nodes);
        m_exec_update(std::string("DELETE FROM ") +
                      sqlite_dom_schema::table_strings);

        return;
    }

    // m_write_meta
    //   writes the schema version marker and a timestamp row.
    void m_write_meta()
    {
        auto stmt = m_conn.prepare(sqlite_dom_schema::sql_insert_meta);

        stmt->bind_string(1, "schema_version");
        stmt->bind_string(
            2,
            std::to_string(sqlite_dom_schema::D_DOM_SCHEMA_VERSION)
        );
        stmt->execute();
        stmt->reset();

        return;
    }


    // ========================================================
    //  phase implementations
    // ========================================================

    // m_write_strings
    //   inserts every interned string from _result.strings.
    // Slot 0 is the null sentinel and is elided.
    void m_write_strings(const cpp_scanner::result_type& _result)
    {
        m_begin_phase(2, "strings");

        auto stmt = m_conn.prepare(sqlite_dom_schema::sql_insert_string);

        std::size_t n = _result.strings.size();

        for (dom_string_id id = 1;
             id < static_cast<dom_string_id>(n);
             ++id)
        {
            stmt->reset();

            stmt->bind_int64(1, static_cast<std::int64_t>(id));
            stmt->bind_string(2, _result.strings.resolve(id));
            stmt->execute();

            m_report.strings_written += 1;
        }

        m_end_phase(2, m_report.strings_written);

        return;
    }

    // m_write_nodes
    //   inserts every cpp_dom_node's base (dom_node) fields
    // into dom_nodes.  Denormalizes parent_stable_id from the
    // parent_by_stable_id map so readers can reconstruct the
    // tree without a join.
    void m_write_nodes(const cpp_scanner::result_type& _result)
    {
        m_begin_phase(3, "nodes");

        auto stmt = m_conn.prepare(sqlite_dom_schema::sql_insert_node);

        for (const cpp_dom_node& n : _result.nodes)
        {
            stmt->reset();

            // parent lookup — denormalized so that the tree can
            // be reconstructed from dom_nodes alone.
            auto pit = _result.parent_by_stable_id.find(n.stable_id);

            stmt->bind_int64(1,
                static_cast<std::int64_t>(n.stable_id));

            if (pit != _result.parent_by_stable_id.end())
            {
                stmt->bind_int64(2,
                    static_cast<std::int64_t>(pit->second));

                m_report.parents_wired += 1;
            }
            else
            {
                stmt->bind_null(2);
            }

            stmt->bind_int   (3,  static_cast<int>(n.kind));
            stmt->bind_int   (4,  static_cast<int>(n.access));
            stmt->bind_int   (5,  static_cast<int>(n.storage));
            stmt->bind_int64 (6,  static_cast<std::int64_t>(n.qualifiers));
            stmt->bind_int64 (7,  static_cast<std::int64_t>(n.name));
            stmt->bind_int64 (8,  static_cast<std::int64_t>(n.type_spelling));
            stmt->bind_int64 (9,  static_cast<std::int64_t>(n.comment));
            stmt->bind_int64 (10, static_cast<std::int64_t>(n.file));
            stmt->bind_int64 (11, static_cast<std::int64_t>(n.line));
            stmt->bind_int64 (12, static_cast<std::int64_t>(n.column));
            stmt->bind_int   (13, n.is_definition ? 1 : 0);

            stmt->execute();

            m_report.nodes_written += 1;
        }

        m_end_phase(3, m_report.nodes_written);

        return;
    }

    // m_write_cpp_nodes
    //   inserts every cpp_dom_node's extension fields into
    // dom_cpp_nodes, keyed on stable_id.
    void m_write_cpp_nodes(const cpp_scanner::result_type& _result)
    {
        m_begin_phase(4, "cpp_nodes");

        auto stmt = m_conn.prepare(sqlite_dom_schema::sql_insert_cpp_node);

        for (const cpp_dom_node& n : _result.nodes)
        {
            stmt->reset();

            stmt->bind_int64(1,
                static_cast<std::int64_t>(n.stable_id));
            stmt->bind_int64(2,
                static_cast<std::int64_t>(n.qualified_name));
            stmt->bind_int64(3,
                static_cast<std::int64_t>(n.return_type));
            stmt->bind_int64(4,
                static_cast<std::int64_t>(n.signature));
            stmt->bind_int64(5,
                static_cast<std::int64_t>(n.mangled_name));
            stmt->bind_int64(6,
                static_cast<std::int64_t>(n.underlying_type));
            stmt->bind_int(7,
                static_cast<int>(n.param_count));
            stmt->bind_int(8,
                static_cast<int>(n.template_param_count));
            stmt->bind_int(9,
                static_cast<int>(n.base_count));
            stmt->bind_int(10,
                static_cast<int>(n.member_count));

            stmt->execute();

            m_report.cpp_nodes_written += 1;
        }

        m_end_phase(4, m_report.cpp_nodes_written);

        return;
    }

    // m_write_edges
    //   inserts every dependency edge pair.  No deduplication.
    void m_write_edges(const cpp_scanner::result_type& _result)
    {
        m_begin_phase(5, "edges");

        auto stmt = m_conn.prepare(sqlite_dom_schema::sql_insert_edge);

        for (const auto& edge : _result.dependency_edges)
        {
            stmt->reset();

            stmt->bind_int64(1,
                static_cast<std::int64_t>(edge.first));
            stmt->bind_int64(2,
                static_cast<std::int64_t>(edge.second));

            stmt->execute();

            m_report.edges_written += 1;
        }

        m_end_phase(5, m_report.edges_written);

        return;
    }


    // ========================================================
    //  low-level helpers
    // ========================================================

    // m_exec
    //   runs a single SQL statement, no result expected.
    // Diagnoses and rethrows on failure.
    void m_exec(const std::string& _sql)
    {
        try
        {
            m_conn.execute(_sql);
        }
        catch (...)
        {
            m_report.rows_failed += 1;
            m_report.status      = DSqlitePersisterExecuteFailure;

            m_emit_diagnostic(sqlite_persister_diagnostic(
                DSqlitePersisterExecuteFailure,
                0,
                std::string("execute failed: ") + _sql
            ));

            throw;
        }

        return;
    }

    // m_exec_update
    //   runs a single update SQL, returning the affected row
    // count.  Diagnoses and rethrows on failure.
    std::int64_t m_exec_update(const std::string& _sql)
    {
        try
        {
            return m_conn.execute_update(_sql);
        }
        catch (...)
        {
            m_report.rows_failed += 1;
            m_report.status      = DSqlitePersisterExecuteFailure;

            m_emit_diagnostic(sqlite_persister_diagnostic(
                DSqlitePersisterExecuteFailure,
                0,
                std::string("execute_update failed: ") + _sql
            ));

            throw;
        }
    }

    // m_try_rollback
    //   attempts to roll back the current transaction,
    // swallowing any exception from the rollback itself so the
    // originally-thrown exception propagates.
    void m_try_rollback()
    {
        try
        {
            m_conn.rollback();
        }
        catch (...)
        {
            // already in failure path; nothing to do.
        }

        return;
    }


    // ========================================================
    //  callback dispatch
    // ========================================================

    void m_begin_phase(int _phase,
                       const char* _name)
    {
        if (m_callbacks.on_phase_begin)
        {
            m_callbacks.on_phase_begin(_phase, _name);
        }

        return;
    }

    void m_end_phase(int _phase,
                     std::size_t _rows)
    {
        if (m_callbacks.on_phase_end)
        {
            m_callbacks.on_phase_end(_phase, _rows);
        }

        return;
    }

    void m_emit_diagnostic(const sqlite_persister_diagnostic& _diag)
    {
        if (m_callbacks.on_diagnostic)
        {
            m_callbacks.on_diagnostic(_diag);
        }

        return;
    }


    // ========================================================
    //  state
    // ========================================================

    _ConnectionType&        m_conn;
    config_type             m_config;
    callbacks_type          m_callbacks;
    report_type             m_report;
};


// ================================================================
//  persist_cpp_scan_result
// ================================================================

// persist_cpp_scan_result
//   free function: convenience one-shot.  Constructs a
// sqlite_dom_persister over the given connection (with default
// config), invokes persist, and returns its final report.
template<typename _ConnectionType>
sqlite_persister_report
persist_cpp_scan_result
(
    const cpp_scanner::result_type&     _result,
    _ConnectionType&                    _conn
)
{
    sqlite_dom_persister<_ConnectionType> p(_conn);
    p.persist(_result);
    return p.report();
}

// persist_cpp_scan_result (with config)
//   free function: convenience one-shot with a caller-supplied
// config.
template<typename _ConnectionType>
sqlite_persister_report
persist_cpp_scan_result
(
    const cpp_scanner::result_type&     _result,
    _ConnectionType&                    _conn,
    const sqlite_persister_config&      _config
)
{
    sqlite_dom_persister<_ConnectionType> p(_conn, _config);
    p.persist(_result);
    return p.report();
}


NS_END  // dom
NS_END  // djinterp


#endif  // DJINTERP_DOM_SQLITE_PERSISTER_
