/*******************************************************************************
* djinterp [core]                                                     sqlite.cpp
*
* Definitions for sqlite.hpp: sqlite_connection's hooks and members, and the
* SQLite layer's failures, over the layer's face.
*   Every operation is one call of the face, whose result decides: SQLITE_OK
* returns, anything else throws, through sqlite_layer::raise, with the
* result as the exception's error_code() and SQLite's own message for the
* connection where SQLite gave one. Opening opens into a face of its own and
* applies the configuration's settings there, so that a connection is either
* open with every setting applied or not open at all.
*
*
* path:      /src/djinterp/core/db/sqlite/sqlite.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../inc/djinterp/core/db/sqlite/sqlite.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstring>  // std::strcmp
#include <string>   // std::string, std::to_string
#include <utility>  // std::pair
#include <vector>   // std::vector
// djinterp
#include "../../../../../inc/djinterp/c/db/sqlite/sqlite.h"  // d_sqlite_*,
                                                              // D_SQLITE_*
#include "../../../../../inc/djinterp/core/db/database_statement.hpp"
                                       // prepared_statement, failure_kind
// re_std
#include "../../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int64_t


NS_DJINTERP

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

namespace
{

// SQLITE_MISUSE_CODE
//   constant: SQLite's SQLITE_MISUSE, which the layer also returns for its
// own refusals, so that the connection's message may not be about it;
// sqlite3.h, which names it, is not included here.
const int SQLITE_MISUSE_CODE = 21;

// JOURNAL_MODE_NAMES
//   constant: the journal modes' names, as SQLite writes them, in the order
// of enum d_sqlite_journal_mode.
const char* const JOURNAL_MODE_NAMES[] = { "delete", "truncate", "persist",
                                           "memory", "wal",      "off" };

/**
 * @brief Finds a journal mode by its name, in any ASCII case.
 *
 * @param[in]  _name  the name.
 * @param[out] _mode  receives the mode; written only when found.
 * @return `true` when the name is one of the six.
 */
bool
journal_mode_from_name(
    const std::string&          _name,
    enum d_sqlite_journal_mode& _mode
) noexcept
{
    std::string folded(_name);

    // ASCII folding, whatever the C locale says
    for (char& c : folded)
    {
        if ( (c >= 'A') &&
             (c <= 'Z') )
        {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }

    // each name in its enumerator's place
    for (int i = 0; i < 6; ++i)
    {
        if (folded == JOURNAL_MODE_NAMES[i])
        {
            _mode = static_cast<enum d_sqlite_journal_mode>(i);

            return true;
        }
    }

    return false;
}

/**
 * @brief The layer's transaction mode for a configured one.
 *
 * @param[in] _mode  the configured mode.
 * @return the layer's mode.
 */
enum d_sqlite_transaction_mode
layer_transaction_mode(
    sqlite_transaction_mode _mode
) noexcept
{
    // the three SQLite has
    switch (_mode)
    {
        case sqlite_transaction_mode::immediate:
            return D_SQLITE_TRANSACTION_IMMEDIATE;

        case sqlite_transaction_mode::exclusive:
            return D_SQLITE_TRANSACTION_EXCLUSIVE;

        case sqlite_transaction_mode::deferred:
        default:
            return D_SQLITE_TRANSACTION_DEFERRED;
    }
}

/**
 * @brief Raises a failure of a call on a face not yet the connection's.
 *
 * @param[in] _db         the face the call ran on.
 * @param[in] _result     the call's result.
 * @param[in] _operation  the operation's name.
 * @throws connection_exception, unless `_result` is SQLITE_OK.
 */
void
check_opening(
    const face::sqlite& _db,
    int                 _result,
    const char*         _operation
)
{
    // only a failure throws
    if (_result != 0)
    {
        sqlite_layer::raise(_db.handle(),
                            failure_kind::connection,
                            _operation,
                            _result);
    }

    return;
}

}  // namespace

//==============================================================================
// VIII. SQLITE CONNECTION
//==============================================================================

/*
sqlite_layer::raise
  SQLite's message for the connection is about the latest call only when that
call failed in SQLite itself: a negative result is the layer's own, and
SQLITE_MISUSE may be, so for those -- and when there is no connection, or no
message -- the result's own description stands in.
*/
void
sqlite_layer::raise(
    handle_type  _db,
    failure_kind _kind,
    const char*  _operation,
    int          _result
)
{
    const char* said = nullptr;

    // the connection's words, where they are about this failure
    if ( (_result > 0)                   &&
         (_result != SQLITE_MISUSE_CODE) &&
         (_db) )
    {
        said = d_sqlite_error_message(_db);
    }

    // otherwise the result's
    if ( (!said)                                 ||
         (said[0] == '\0')                       ||
         (std::strcmp(said, "not an error") == 0) )
    {
        said = d_sqlite_result_message(_result);
    }

    const std::string message = std::string("SQLite ") + _operation + ": " +
                                ((said) ? said
                                        : "unknown error") +
                                " (" + std::to_string(_result) + ")";

    // the kind of failure decides the exception
    switch (_kind)
    {
        case failure_kind::connection:
            throw connection_exception(message, _result);

        case failure_kind::transaction:
            throw transaction_exception(message, _result);

        case failure_kind::query:
        default:
            throw query_exception(message, _result);
    }
}

template class prepared_statement<sqlite_layer>;

sqlite_connection::sqlite_connection()
    : base_type(),
      m_sqlite_config(),
      m_db()
{}

sqlite_connection::sqlite_connection(
    const connection_config& _config
)
    : base_type(_config),
      m_sqlite_config(_config.database),
      m_db()
{}

sqlite_connection::sqlite_connection(
    const sqlite_connect_config& _config
)
    : base_type(),
      m_sqlite_config(_config),
      m_db()
{
    this->m_config.database = _config.file_path;
}

sqlite_connection::sqlite_connection(
    const std::string& _file_path
)
    : base_type(),
      m_sqlite_config(_file_path),
      m_db()
{
    this->m_config.database = _file_path;
}

/*
sqlite_connection::~sqlite_connection
  The statements go first, while the connection they belong to is open; the
face then closes the connection as it goes.
*/
sqlite_connection::~sqlite_connection()
{
    this->close_session_noexcept();
}

/*
sqlite_connection::open
  An open connection closes first: connect() alone would keep it, and the
new path would wait for the next reconnection.
*/
void
sqlite_connection::open(
    const std::string& _file_path
)
{
    this->disconnect();

    m_sqlite_config.file_path = _file_path;
    this->m_config.database   = _file_path;

    this->connect();

    return;
}

void
sqlite_connection::open(
    const std::string& _file_path,
    sqlite_open_flag   _flags
)
{
    m_sqlite_config.open_flags = _flags;

    open(_file_path);

    return;
}

void
sqlite_connection::close() noexcept
{
    this->disconnect();

    return;
}

void
sqlite_connection::set_journal_mode(
    const std::string& _mode
)
{
    enum d_sqlite_journal_mode mode = D_SQLITE_JOURNAL_DELETE;

    // one of SQLite's six, or nothing to run
    if (!journal_mode_from_name(_mode, mode))
    {
        throw query_exception("SQLite set_journal_mode: '" + _mode +
                              "' is not a journal mode",
                              D_DB_INVALID_ARGUMENT);
    }

    this->run_guarded(
        [this, mode]() -> void
        {
            m_check(m_db.set_journal_mode(face::string_arg(), mode),
                    failure_kind::query,
                    "set_journal_mode");

            return;
        });

    m_sqlite_config.journal_mode = static_cast<sqlite_journal_mode>(mode);

    return;
}

std::string
sqlite_connection::get_journal_mode() const
{
    return this->ask_guarded(
        [this]() -> std::string
        {
            enum d_sqlite_journal_mode mode = D_SQLITE_JOURNAL_DELETE;

            m_check(m_db.get_journal_mode(face::string_arg(), mode),
                    failure_kind::query,
                    "get_journal_mode");

            return JOURNAL_MODE_NAMES[static_cast<int>(mode)];
        });
}

void
sqlite_connection::checkpoint(
    int _mode
)
{
    // passive, full, restart or truncate
    if ( (_mode < 0) ||
         (_mode > 3) )
    {
        throw query_exception("SQLite checkpoint: " +
                              std::to_string(_mode) +
                              " is not a checkpoint mode",
                              D_DB_INVALID_ARGUMENT);
    }

    this->run_guarded(
        [this, _mode]() -> void
        {
            m_check(m_db.checkpoint(
                        face::string_arg(),
                        static_cast<enum d_sqlite_checkpoint_mode>(_mode)),
                    failure_kind::query,
                    "checkpoint");

            return;
        });

    return;
}

void
sqlite_connection::enable_wal()
{
    set_journal_mode("WAL");

    return;
}

void
sqlite_connection::set_pragma(
    const std::string& _pragma,
    const std::string& _value
)
{
    this->run_guarded(
        [this, &_pragma, &_value]() -> void
        {
            internal::refuse_nul(_pragma, "set_pragma", "the PRAGMA");
            internal::refuse_nul(_value, "set_pragma", "the value");
            m_check(m_db.set_pragma(face::string_arg(), _pragma, _value),
                    failure_kind::query,
                    "set_pragma");

            return;
        });

    return;
}

std::string
sqlite_connection::get_pragma(
    const std::string& _pragma
) const
{
    return this->ask_guarded(
        [this, &_pragma]() -> std::string
        {
            std::string setting;

            internal::refuse_nul(_pragma, "get_pragma", "the PRAGMA");
            m_check(m_db.get_pragma(face::string_arg(), _pragma, setting),
                    failure_kind::query,
                    "get_pragma");

            return setting;
        });
}

void
sqlite_connection::set_foreign_keys(
    bool _enabled
)
{
    this->run_guarded(
        [this, _enabled]() -> void
        {
            m_check(m_db.set_foreign_keys(_enabled),
                    failure_kind::query,
                    "set_foreign_keys");

            return;
        });

    m_sqlite_config.enable_foreign_keys = _enabled;

    return;
}

void
sqlite_connection::set_busy_timeout(
    int _timeout_ms
)
{
    this->run_guarded(
        [this, _timeout_ms]() -> void
        {
            m_check(m_db.set_busy_timeout(_timeout_ms),
                    failure_kind::query,
                    "set_busy_timeout");

            return;
        });

    m_sqlite_config.busy_timeout_ms = _timeout_ms;

    return;
}

void
sqlite_connection::attach_database(
    const std::string& _file_path,
    const std::string& _name
)
{
    this->run_guarded(
        [this, &_file_path, &_name]() -> void
        {
            internal::refuse_nul(_file_path, "attach_database", "the path");
            internal::refuse_nul(_name, "attach_database", "the name");
            m_check(m_db.attach_database(_file_path, _name),
                    failure_kind::query,
                    "attach_database");

            return;
        });

    return;
}

void
sqlite_connection::detach_database(
    const std::string& _name
)
{
    this->run_guarded(
        [this, &_name]() -> void
        {
            internal::refuse_nul(_name, "detach_database", "the name");
            m_check(m_db.detach_database(_name),
                    failure_kind::query,
                    "detach_database");

            return;
        });

    return;
}

bool
sqlite_connection::has_table(
    const std::string& _table_name
) const
{
    return this->ask_guarded(
        [this, &_table_name]() -> bool
        {
            bool exists = false;

            internal::refuse_nul(_table_name, "has_table", "the name");
            m_check(m_db.has_table_s(face::string_arg(), _table_name, exists),
                    failure_kind::query,
                    "has_table");

            return exists;
        });
}

std::vector<std::string>
sqlite_connection::list_tables() const
{
    return this->ask_guarded(
        [this]() -> std::vector<std::string>
        {
            std::vector<std::string> names;

            m_check(m_db.list_tables(face::string_arg(), names),
                    failure_kind::query,
                    "list_tables");

            return names;
        });
}

void
sqlite_connection::begin_deferred()
{
    this->run_guarded(
        [this]() -> void
        {
            m_check(m_db.begin_transaction(D_SQLITE_TRANSACTION_DEFERRED),
                    failure_kind::transaction,
                    "begin_deferred");

            return;
        });

    this->m_in_transaction = true;

    return;
}

void
sqlite_connection::begin_immediate()
{
    this->run_guarded(
        [this]() -> void
        {
            m_check(m_db.begin_transaction(D_SQLITE_TRANSACTION_IMMEDIATE),
                    failure_kind::transaction,
                    "begin_immediate");

            return;
        });

    this->m_in_transaction = true;

    return;
}

void
sqlite_connection::begin_exclusive()
{
    this->run_guarded(
        [this]() -> void
        {
            m_check(m_db.begin_transaction(D_SQLITE_TRANSACTION_EXCLUSIVE),
                    failure_kind::transaction,
                    "begin_exclusive");

            return;
        });

    this->m_in_transaction = true;

    return;
}

void
sqlite_connection::backup_to(
    const std::string& _dest_path
)
{
    this->run_guarded(
        [this, &_dest_path]() -> void
        {
            internal::refuse_nul(_dest_path, "backup_to", "the path");
            m_check(m_db.backup_to_file(face::string_arg(), _dest_path),
                    failure_kind::query,
                    "backup_to");

            return;
        });

    return;
}

void
sqlite_connection::backup_from(
    const std::string& _source_path
)
{
    this->run_guarded(
        [this, &_source_path]() -> void
        {
            internal::refuse_nul(_source_path, "backup_from", "the path");
            m_check(m_db.restore_from_file(face::string_arg(), _source_path),
                    failure_kind::query,
                    "backup_from");

            return;
        });

    return;
}

const sqlite_connect_config&
sqlite_connection::get_sqlite_config() const noexcept
{
    return m_sqlite_config;
}

void
sqlite_connection::set_sqlite_config(
    const sqlite_connect_config& _config
)
{
    m_sqlite_config         = _config;
    this->m_config.database = _config.file_path;

    return;
}

bool
sqlite_connection::is_in_memory() const noexcept
{
    return ( (m_sqlite_config.file_path == ":memory:") ||
             ( (static_cast<int>(m_sqlite_config.open_flags) &
                static_cast<int>(sqlite_open_flag::memory)) != 0 ) );
}

face::sqlite&
sqlite_connection::get_face() noexcept
{
    return m_db;
}

const face::sqlite&
sqlite_connection::get_face() const noexcept
{
    return m_db;
}

/*
sqlite_connection::connect_helper
  The file opens into a face of its own, the settings are applied there, and
only then does the connection take it: a setting that fails leaves the old
state, and the face that failed closes as it goes. The configuration's
`database` names the file, whichever way it was set; the settings SQLite's
own defaults already give are not run again.
*/
void
sqlite_connection::connect_helper()
{
    const std::string path   = this->m_config.database;
    unsigned          flags  =
        static_cast<unsigned>(static_cast<int>(m_sqlite_config.open_flags));
    face::sqlite      opened;

    internal::refuse_nul_setting(m_sqlite_config.vfs_name,
                                 "SQLite open",
                                 "vfs_name");

    // each PRAGMA the configuration names, by name and value
    for (const std::pair<const std::string, std::string>& pragma :
         m_sqlite_config.pragmas)
    {
        internal::refuse_nul_setting(pragma.first,
                                     "SQLite open",
                                     "a PRAGMA's name");
        internal::refuse_nul_setting(pragma.second,
                                     "SQLite open",
                                     pragma.first.c_str());
    }

    // settings SQLite has nothing to give to; a URI's scheme is none
    for (const std::pair<const std::string, std::string>& option :
         this->m_config.custom_options)
    {
        if (option.first != "scheme")
        {
            throw connection_exception("SQLite open: the custom option '" +
                                           option.first + "' is not "
                                           "supported; a PRAGMA goes in "
                                           "sqlite_connect_config::pragmas",
                                       D_DB_UNSUPPORTED);
        }
    }

    // a cache shared with the process's other connections, when asked for
    if (m_sqlite_config.enable_shared_cache)
    {
        flags |= D_SQLITE_OPEN_SHAREDCACHE;
    }

    const int result = opened.open_with(
        path,
        flags,
        (m_sqlite_config.vfs_name.empty())
            ? face::string_arg()
            : face::string_arg(m_sqlite_config.vfs_name));

    // a file that does not open names itself in the message
    if (result != 0)
    {
        const std::string operation = "open '" + path + "'";

        sqlite_layer::raise(nullptr,
                            failure_kind::connection,
                            operation.c_str(),
                            result);
    }

    check_opening(opened,
                  opened.set_busy_timeout(m_sqlite_config.busy_timeout_ms),
                  "set_busy_timeout");
    check_opening(opened,
                  opened.set_foreign_keys(m_sqlite_config.enable_foreign_keys),
                  "set_foreign_keys");

    // the journal, where it is not SQLite's default
    if ( (m_sqlite_config.enable_wal) ||
         (m_sqlite_config.journal_mode != sqlite_journal_mode::mode_delete) )
    {
        const enum d_sqlite_journal_mode mode =
            (m_sqlite_config.enable_wal)
                ? D_SQLITE_JOURNAL_WAL
                : static_cast<enum d_sqlite_journal_mode>(
                      m_sqlite_config.journal_mode);

        check_opening(opened,
                      opened.set_journal_mode(face::string_arg(), mode),
                      "set_journal_mode");
    }

    // the cache and page sizes, where they are not SQLite's defaults
    if (m_sqlite_config.cache_size != -2000)
    {
        check_opening(opened,
                      opened.set_pragma(face::string_arg(),
                                        "cache_size",
                                        std::to_string(
                                            m_sqlite_config.cache_size)),
                      "cache_size");
    }

    if (m_sqlite_config.page_size != 4096)
    {
        check_opening(opened,
                      opened.set_pragma(face::string_arg(),
                                        "page_size",
                                        std::to_string(
                                            m_sqlite_config.page_size)),
                      "page_size");
    }

    // every other PRAGMA the configuration names, in order
    for (const std::pair<const std::string, std::string>& pragma :
         m_sqlite_config.pragmas)
    {
        check_opening(opened,
                      opened.set_pragma(face::string_arg(),
                                        pragma.first,
                                        pragma.second),
                      "set_pragma");
    }

    m_db.swap(opened);
    m_sqlite_config.file_path = path;

    return;
}

void
sqlite_connection::disconnect_helper() noexcept
{
    (void)m_db.close();

    return;
}

bool
sqlite_connection::is_connected_helper() const noexcept
{
    return m_db.is_open();
}

bool
sqlite_connection::ping_helper() const noexcept
{
    return m_db.is_open();
}

face::rows
sqlite_connection::execute_query_helper(
    const std::string& _query
)
{
    face::rows found;

    m_check(m_db.query_rows(found, _query),
            failure_kind::query,
            "execute_query");

    return found;
}

re_std::int64_t
sqlite_connection::execute_update_helper(
    const std::string& _query
)
{
    re_std::int64_t changes = 0;

    m_check(m_db.exec_changes(changes, _query),
            failure_kind::query,
            "execute_update");

    return changes;
}

face::sqlite_statement
sqlite_connection::prepare_helper(
    const std::string& _query
)
{
    face::sqlite_statement prepared;

    m_check(m_db.prepare(prepared, _query),
            failure_kind::query,
            "prepare");

    return prepared;
}

std::string
sqlite_connection::get_server_version_helper() const
{
    const char* const version = face::sqlite::library_version();

    return (version) ? version
                     : "";
}

re_std::int64_t
sqlite_connection::get_last_insert_id_helper() const noexcept
{
    return m_db.last_insert_id();
}

void
sqlite_connection::begin_transaction_helper()
{
    m_check(m_db.begin_transaction(layer_transaction_mode(
                m_sqlite_config.default_transaction_mode)),
            failure_kind::transaction,
            "begin_transaction");

    return;
}

/*
sqlite_connection::begin_transaction_helper (isolation_level)
  SQLite runs every transaction serializable, which every level asked for is
satisfied by, so the level changes nothing; read_uncommitted, which SQLite
has only for a shared cache, is set_isolation_level's, for the session.
*/
void
sqlite_connection::begin_transaction_helper(
    isolation_level _isolation
)
{
    static_cast<void>(_isolation);

    begin_transaction_helper();

    return;
}

void
sqlite_connection::commit_helper()
{
    m_check(m_db.commit(), failure_kind::transaction, "commit");

    return;
}

void
sqlite_connection::rollback_helper()
{
    m_check(m_db.rollback(), failure_kind::transaction, "rollback");

    return;
}

bool
sqlite_connection::in_transaction_helper() const noexcept
{
    return m_db.in_transaction();
}

isolation_level
sqlite_connection::get_isolation_level_helper() const
{
    std::string setting;

    m_check(m_db.get_pragma(face::string_arg(), "read_uncommitted", setting),
            failure_kind::transaction,
            "get_isolation_level");

    return (setting == "1") ? isolation_level::read_uncommitted
                            : isolation_level::serializable;
}

void
sqlite_connection::set_isolation_level_helper(
    isolation_level _level
)
{
    m_check(m_db.set_pragma(face::string_arg(),
                            "read_uncommitted",
                            (_level == isolation_level::read_uncommitted)
                                ? "1"
                                : "0"),
            failure_kind::transaction,
            "set_isolation_level");

    return;
}

void
sqlite_connection::create_savepoint_helper(
    const std::string& _name
)
{
    m_check(m_db.create_savepoint(_name),
            failure_kind::transaction,
            "create_savepoint");

    return;
}

void
sqlite_connection::release_savepoint_helper(
    const std::string& _name
)
{
    m_check(m_db.release_savepoint(_name),
            failure_kind::transaction,
            "release_savepoint");

    return;
}

void
sqlite_connection::rollback_to_savepoint_helper(
    const std::string& _name
)
{
    m_check(m_db.rollback_to_savepoint(_name),
            failure_kind::transaction,
            "rollback_to_savepoint");

    return;
}

struct ::sqlite3*
sqlite_connection::native_handle_helper() const noexcept
{
    return m_db.handle();
}

void
sqlite_connection::m_check(
    int          _result,
    failure_kind _kind,
    const char*  _operation
) const
{
    // only a failure throws
    if (_result != 0)
    {
        sqlite_layer::raise(m_db.handle(), _kind, _operation, _result);
    }

    return;
}

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
