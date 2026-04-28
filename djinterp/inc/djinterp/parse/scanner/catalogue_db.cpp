#include "catalogue_db.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <iostream>
#include <sstream>

namespace d_catalogue {

// ============================================================
// d_symbol_filter defaults
// ============================================================

/*
d_symbol_filter::d_symbol_filter
  Default-constructs a filter with no constraints.

Parameter(s):
  (none)
Return:
  (constructor)
*/
d_symbol_filter::d_symbol_filter()
    : name_pattern()
    , module_name()
    , parent_usr()
    , file_pattern()
    , categories()
    , access_levels()
    , definitions_only(false)
    , templates_only(false)
    , limit(0)
    , offset(0)
{
}

// ============================================================
// helper: custom deleter for sqlite3_stmt
// ============================================================

static void
m_stmt_deleter
(
    sqlite3_stmt* _stmt
)
{
    if (_stmt)
    {
        sqlite3_finalize(_stmt);
    }

    return;
}

// ============================================================
// construction / destruction
// ============================================================

/*
d_catalogue_db::d_catalogue_db
  Default-constructs a closed database handle.

Parameter(s):
  (none)
Return:
  (constructor)
*/
d_catalogue_db::d_catalogue_db()
    : m_db(nullptr)
    , m_config()
    , m_last_error()
{
}

/*
d_catalogue_db::d_catalogue_db
  Constructs and opens the database with the given configuration.

Parameter(s):
  _config: database configuration.
Return:
  (constructor)
*/
d_catalogue_db::d_catalogue_db
(
    const d_database_config& _config
)
    : m_db(nullptr)
    , m_config(_config)
    , m_last_error()
{
    open(_config);
}

/*
d_catalogue_db::~d_catalogue_db
  Closes the database connection if open.

Parameter(s):
  (none)
Return:
  (destructor)
*/
d_catalogue_db::~d_catalogue_db()
{
    close();
}

// ============================================================
// lifecycle
// ============================================================

/*
d_catalogue_db::open
  Opens (or creates) an SQLite database at the given path.

Parameter(s):
  _path: filesystem path for the database file.
Return:
  true on success, false on failure.
*/
bool
d_catalogue_db::open
(
    const std::string& _path
)
{
    int rc;

    // close any existing connection
    close();

    rc = sqlite3_open(_path.c_str(), &m_db);

    // check for open failure
    if (rc != SQLITE_OK)
    {
        m_last_error = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;

        return false;
    }

    // set busy timeout
    sqlite3_busy_timeout(m_db, m_config.busy_timeout_ms);

    // enable WAL mode for better concurrency
    m_exec("PRAGMA journal_mode=WAL;");
    m_exec("PRAGMA foreign_keys=ON;");

    return true;
}

/*
d_catalogue_db::open
  Opens the database using the stored configuration.

Parameter(s):
  _config: database configuration to apply and use.
Return:
  true on success.
*/
bool
d_catalogue_db::open
(
    const d_database_config& _config
)
{
    m_config = _config;

    return open(_config.db_path);
}

/*
d_catalogue_db::close
  Closes the database connection.

Parameter(s):
  (none)
Return:
  none.
*/
void
d_catalogue_db::close()
{
    if (m_db)
    {
        sqlite3_close(m_db);
        m_db = nullptr;
    }

    return;
}

/*
d_catalogue_db::is_open
  Returns true if the database connection is active.

Parameter(s):
  (none)
Return:
  true if connected.
*/
bool
d_catalogue_db::is_open() const
{
    return (m_db != nullptr);
}

// ============================================================
// schema
// ============================================================

// D_INTERNAL_SCHEMA_SYMBOLS
//   constant: SQL for the symbols table.
static const char* D_INTERNAL_SCHEMA_SYMBOLS = R"SQL(
    CREATE TABLE IF NOT EXISTS symbols (
        id                INTEGER PRIMARY KEY AUTOINCREMENT,
        name              TEXT    NOT NULL,
        qualified_name    TEXT,
        usr               TEXT    UNIQUE NOT NULL,
        category          TEXT    NOT NULL,
        access            TEXT    DEFAULT 'none',
        linkage           TEXT    DEFAULT 'none',

        def_file          TEXT,
        def_line          INTEGER DEFAULT 0,
        def_column        INTEGER DEFAULT 0,
        decl_file         TEXT,
        decl_line         INTEGER DEFAULT 0,
        decl_column       INTEGER DEFAULT 0,

        type_spelling     TEXT,
        return_type       TEXT,
        underlying_type   TEXT,

        is_const          INTEGER DEFAULT 0,
        is_static         INTEGER DEFAULT 0,
        is_virtual        INTEGER DEFAULT 0,
        is_pure_virtual   INTEGER DEFAULT 0,
        is_inline         INTEGER DEFAULT 0,
        is_constexpr      INTEGER DEFAULT 0,
        is_noexcept       INTEGER DEFAULT 0,
        is_template       INTEGER DEFAULT 0,
        is_variadic       INTEGER DEFAULT 0,
        is_definition     INTEGER DEFAULT 0,
        is_deprecated     INTEGER DEFAULT 0,

        parameters        TEXT,
        template_params   TEXT,
        base_classes      TEXT,
        enum_constants    TEXT,

        raw_comment       TEXT,
        brief_comment     TEXT,
        category_tag      TEXT,
        description       TEXT,
        return_doc        TEXT,
        parameter_docs    TEXT,

        module_name       TEXT,
        parent_usr        TEXT,
        parent_name       TEXT
    );
)SQL";

// D_INTERNAL_SCHEMA_MODULES
//   constant: SQL for the modules table.
static const char* D_INTERNAL_SCHEMA_MODULES = R"SQL(
    CREATE TABLE IF NOT EXISTS modules (
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
        name             TEXT    UNIQUE NOT NULL,
        path             TEXT,
        description      TEXT,
        header_files     TEXT,
        source_files     TEXT,
        includes         TEXT,
        depends_on       TEXT
    );
)SQL";

// D_INTERNAL_SCHEMA_DEPS
//   constant: SQL for the dependency edges table.
static const char* D_INTERNAL_SCHEMA_DEPS = R"SQL(
    CREATE TABLE IF NOT EXISTS dependencies (
        id        INTEGER PRIMARY KEY AUTOINCREMENT,
        from_usr  TEXT    NOT NULL,
        to_usr    TEXT    NOT NULL,
        UNIQUE(from_usr, to_usr)
    );
)SQL";

// D_INTERNAL_SCHEMA_INDICES
//   constant: SQL for creating indices on hot columns.
static const char* D_INTERNAL_SCHEMA_INDICES = R"SQL(
    CREATE INDEX IF NOT EXISTS idx_symbols_name
        ON symbols(name);
    CREATE INDEX IF NOT EXISTS idx_symbols_module
        ON symbols(module_name);
    CREATE INDEX IF NOT EXISTS idx_symbols_category
        ON symbols(category);
    CREATE INDEX IF NOT EXISTS idx_symbols_parent
        ON symbols(parent_usr);
    CREATE INDEX IF NOT EXISTS idx_deps_from
        ON dependencies(from_usr);
    CREATE INDEX IF NOT EXISTS idx_deps_to
        ON dependencies(to_usr);
)SQL";

/*
d_catalogue_db::create_tables
  Creates all catalogue tables and indices if they do not exist.

Parameter(s):
  (none)
Return:
  true on success.
*/
bool
d_catalogue_db::create_tables()
{
    // validate state
    if (!m_db)
    {
        m_last_error = "database not open";

        return false;
    }

    // drop tables if configured
    if (m_config.recreate_tables)
    {
        drop_tables();
    }

    // create each table
    if (!m_exec(D_INTERNAL_SCHEMA_SYMBOLS))
    {
        return false;
    }

    if (!m_exec(D_INTERNAL_SCHEMA_MODULES))
    {
        return false;
    }

    if (!m_exec(D_INTERNAL_SCHEMA_DEPS))
    {
        return false;
    }

    // create indices
    if (!m_exec(D_INTERNAL_SCHEMA_INDICES))
    {
        return false;
    }

    return true;
}

/*
d_catalogue_db::drop_tables
  Drops all catalogue tables.

Parameter(s):
  (none)
Return:
  true on success.
*/
bool
d_catalogue_db::drop_tables()
{
    if (!m_db)
    {
        return false;
    }

    m_exec("DROP TABLE IF EXISTS dependencies;");
    m_exec("DROP TABLE IF EXISTS symbols;");
    m_exec("DROP TABLE IF EXISTS modules;");

    return true;
}

// ============================================================
// transactions
// ============================================================

/*
d_catalogue_db::begin_transaction
  Begins an SQLite transaction.

Parameter(s):
  (none)
Return:
  true on success.
*/
bool
d_catalogue_db::begin_transaction()
{
    return m_exec("BEGIN TRANSACTION;");
}

/*
d_catalogue_db::commit_transaction
  Commits the current transaction.

Parameter(s):
  (none)
Return:
  true on success.
*/
bool
d_catalogue_db::commit_transaction()
{
    return m_exec("COMMIT;");
}

/*
d_catalogue_db::rollback_transaction
  Rolls back the current transaction.

Parameter(s):
  (none)
Return:
  true on success.
*/
bool
d_catalogue_db::rollback_transaction()
{
    return m_exec("ROLLBACK;");
}

// ============================================================
// insertion
// ============================================================

// D_INTERNAL_INSERT_SYMBOL_SQL
//   constant: parameterized INSERT for the symbols table.
static const char* D_INTERNAL_INSERT_SYMBOL_SQL = R"SQL(
    INSERT OR REPLACE INTO symbols (
        name, qualified_name, usr, category, access, linkage,
        def_file, def_line, def_column,
        decl_file, decl_line, decl_column,
        type_spelling, return_type, underlying_type,
        is_const, is_static, is_virtual, is_pure_virtual,
        is_inline, is_constexpr, is_noexcept, is_template,
        is_variadic, is_definition, is_deprecated,
        parameters, template_params, base_classes,
        enum_constants,
        raw_comment, brief_comment, category_tag,
        description, return_doc, parameter_docs,
        module_name, parent_usr, parent_name
    ) VALUES (
        ?1, ?2, ?3, ?4, ?5, ?6,
        ?7, ?8, ?9, ?10, ?11, ?12,
        ?13, ?14, ?15,
        ?16, ?17, ?18, ?19,
        ?20, ?21, ?22, ?23,
        ?24, ?25, ?26,
        ?27, ?28, ?29, ?30,
        ?31, ?32, ?33,
        ?34, ?35, ?36,
        ?37, ?38, ?39
    );
)SQL";

/*
d_catalogue_db::insert_symbol
  Inserts or replaces a single symbol in the database.

Parameter(s):
  _sym: the symbol to insert.
Return:
  true on success.
*/
bool
d_catalogue_db::insert_symbol
(
    const d_symbol_info& _sym
)
{
    stmt_ptr stmt = m_prepare(D_INTERNAL_INSERT_SYMBOL_SQL);

    // check preparation
    if (!stmt)
    {
        return false;
    }

    // bind all 39 columns
    m_bind_text(stmt.get(),  1, _sym.name);
    m_bind_text(stmt.get(),  2, _sym.qualified_name);
    m_bind_text(stmt.get(),  3, _sym.usr);
    m_bind_text(stmt.get(),  4,
                d_symbol_category_to_string(_sym.category));
    m_bind_text(stmt.get(),  5,
                d_access_specifier_to_string(_sym.access));
    m_bind_text(stmt.get(),  6,
                d_linkage_kind_to_string(_sym.linkage));

    m_bind_text(stmt.get(),  7, _sym.definition_loc.file);
    m_bind_int (stmt.get(),  8, _sym.definition_loc.line);
    m_bind_int (stmt.get(),  9, _sym.definition_loc.column);
    m_bind_text(stmt.get(), 10, _sym.declaration_loc.file);
    m_bind_int (stmt.get(), 11, _sym.declaration_loc.line);
    m_bind_int (stmt.get(), 12, _sym.declaration_loc.column);

    m_bind_text(stmt.get(), 13, _sym.type_spelling);
    m_bind_text(stmt.get(), 14, _sym.return_type);
    m_bind_text(stmt.get(), 15, _sym.underlying_type);

    m_bind_int(stmt.get(), 16, _sym.is_const        ? 1 : 0);
    m_bind_int(stmt.get(), 17, _sym.is_static       ? 1 : 0);
    m_bind_int(stmt.get(), 18, _sym.is_virtual      ? 1 : 0);
    m_bind_int(stmt.get(), 19, _sym.is_pure_virtual  ? 1 : 0);
    m_bind_int(stmt.get(), 20, _sym.is_inline       ? 1 : 0);
    m_bind_int(stmt.get(), 21, _sym.is_constexpr    ? 1 : 0);
    m_bind_int(stmt.get(), 22, _sym.is_noexcept     ? 1 : 0);
    m_bind_int(stmt.get(), 23, _sym.is_template     ? 1 : 0);
    m_bind_int(stmt.get(), 24, _sym.is_variadic     ? 1 : 0);
    m_bind_int(stmt.get(), 25, _sym.is_definition   ? 1 : 0);
    m_bind_int(stmt.get(), 26, _sym.is_deprecated   ? 1 : 0);

    m_bind_text(stmt.get(), 27,
                m_serialize_params(_sym.parameters));
    m_bind_text(stmt.get(), 28,
                m_serialize_string_vec(
                    _sym.template_parameters));
    m_bind_text(stmt.get(), 29,
                m_serialize_string_vec(_sym.base_classes));
    m_bind_text(stmt.get(), 30,
                m_serialize_enum_constants(
                    _sym.enum_constants));

    m_bind_text(stmt.get(), 31, _sym.comment.raw_comment);
    m_bind_text(stmt.get(), 32, _sym.comment.brief_comment);
    m_bind_text(stmt.get(), 33, _sym.comment.category_tag);
    m_bind_text(stmt.get(), 34, _sym.comment.description);
    m_bind_text(stmt.get(), 35, _sym.comment.return_doc);
    m_bind_text(stmt.get(), 36,
                m_serialize_param_docs(
                    _sym.comment.parameter_docs));

    m_bind_text(stmt.get(), 37, _sym.module_name);
    m_bind_text(stmt.get(), 38, _sym.parent_usr);
    m_bind_text(stmt.get(), 39, _sym.parent_name);

    // execute
    int rc = sqlite3_step(stmt.get());

    if (rc != SQLITE_DONE)
    {
        m_last_error = sqlite3_errmsg(m_db);

        return false;
    }

    return true;
}

/*
d_catalogue_db::insert_module
  Inserts or replaces a single module in the database.

Parameter(s):
  _mod: the module to insert.
Return:
  true on success.
*/
bool
d_catalogue_db::insert_module
(
    const d_module_info& _mod
)
{
    stmt_ptr stmt = m_prepare(
        "INSERT OR REPLACE INTO modules "
        "(name, path, description, header_files, "
        "source_files, includes, depends_on) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);");

    if (!stmt)
    {
        return false;
    }

    m_bind_text(stmt.get(), 1, _mod.name);
    m_bind_text(stmt.get(), 2, _mod.path);
    m_bind_text(stmt.get(), 3, _mod.description);
    m_bind_text(stmt.get(), 4,
                m_serialize_string_vec(_mod.header_files));
    m_bind_text(stmt.get(), 5,
                m_serialize_string_vec(_mod.source_files));
    m_bind_text(stmt.get(), 6,
                m_serialize_string_vec(_mod.includes));
    m_bind_text(stmt.get(), 7,
                m_serialize_string_vec(
                    std::vector<std::string>(
                        _mod.depends_on_modules.begin(),
                        _mod.depends_on_modules.end())));

    int rc = sqlite3_step(stmt.get());

    if (rc != SQLITE_DONE)
    {
        m_last_error = sqlite3_errmsg(m_db);

        return false;
    }

    return true;
}

/*
d_catalogue_db::insert_dependency
  Records a single dependency edge.

Parameter(s):
  _from_usr: the source symbol's USR.
  _to_usr:   the target symbol's USR.
Return:
  true on success.
*/
bool
d_catalogue_db::insert_dependency
(
    const std::string& _from_usr,
    const std::string& _to_usr
)
{
    stmt_ptr stmt = m_prepare(
        "INSERT OR IGNORE INTO dependencies "
        "(from_usr, to_usr) VALUES (?1, ?2);");

    if (!stmt)
    {
        return false;
    }

    m_bind_text(stmt.get(), 1, _from_usr);
    m_bind_text(stmt.get(), 2, _to_usr);

    int rc = sqlite3_step(stmt.get());

    if (rc != SQLITE_DONE)
    {
        m_last_error = sqlite3_errmsg(m_db);

        return false;
    }

    return true;
}

/*
d_catalogue_db::bulk_insert_symbols
  Inserts all symbols from a scanner's symbol map, wrapped in
  a transaction for performance.

Parameter(s):
  _symbols: the scanner's symbol map (USR -> d_symbol_info).
Return:
  the number of symbols successfully inserted.
*/
int64_t
d_catalogue_db::bulk_insert_symbols
(
    const std::map<std::string, d_symbol_info>& _symbols
)
{
    int64_t count;

    count = 0;

    if (m_config.use_transactions)
    {
        begin_transaction();
    }

    // insert each symbol and its dependency edges
    for (const auto& [usr, sym] : _symbols)
    {
        if (insert_symbol(sym))
        {
            count++;

            // insert dependency edges
            for (const auto& ref : sym.referenced_usrs)
            {
                insert_dependency(usr, ref);
            }
        }
    }

    if (m_config.use_transactions)
    {
        commit_transaction();
    }

    return count;
}

/*
d_catalogue_db::bulk_insert_modules
  Inserts all modules from a scanner's module map.

Parameter(s):
  _modules: the scanner's module map.
Return:
  the number of modules successfully inserted.
*/
int64_t
d_catalogue_db::bulk_insert_modules
(
    const std::map<std::string, d_module_info>& _modules
)
{
    int64_t count;

    count = 0;

    if (m_config.use_transactions)
    {
        begin_transaction();
    }

    for (const auto& [name, mod] : _modules)
    {
        if (insert_module(mod))
        {
            count++;
        }
    }

    if (m_config.use_transactions)
    {
        commit_transaction();
    }

    return count;
}

// ============================================================
// queries
// ============================================================

/*
d_catalogue_db::query_symbols
  Queries symbols with flexible filtering.

Parameter(s):
  _filter: the filter criteria to apply.
Return:
  a vector of matching d_symbol_info records.
*/
std::vector<d_symbol_info>
d_catalogue_db::query_symbols
(
    const d_symbol_filter& _filter
)
const
{
    std::vector<d_symbol_info> results;
    std::string                sql;
    std::string                where_clause;
    std::vector<std::string>   conditions;

    sql = "SELECT * FROM symbols";

    // build WHERE conditions
    if (!_filter.name_pattern.empty())
    {
        conditions.push_back(
            "name LIKE '%" + _filter.name_pattern + "%'");
    }

    if (!_filter.module_name.empty())
    {
        conditions.push_back(
            "module_name = '" + _filter.module_name + "'");
    }

    if (!_filter.parent_usr.empty())
    {
        conditions.push_back(
            "parent_usr = '" + _filter.parent_usr + "'");
    }

    if (!_filter.file_pattern.empty())
    {
        conditions.push_back(
            "def_file LIKE '%" + _filter.file_pattern + "%'");
    }

    if (_filter.definitions_only)
    {
        conditions.push_back("is_definition = 1");
    }

    if (_filter.templates_only)
    {
        conditions.push_back("is_template = 1");
    }

    // category filter
    if (!_filter.categories.empty())
    {
        std::string cat_list = "(";

        for (size_t i = 0; i < _filter.categories.size(); i++)
        {
            if (i > 0)
            {
                cat_list += ",";
            }

            cat_list += "'";
            cat_list += d_symbol_category_to_string(
                            _filter.categories[i]);
            cat_list += "'";
        }

        cat_list += ")";
        conditions.push_back("category IN " + cat_list);
    }

    // assemble WHERE
    if (!conditions.empty())
    {
        sql += " WHERE ";

        for (size_t i = 0; i < conditions.size(); i++)
        {
            if (i > 0)
            {
                sql += " AND ";
            }

            sql += conditions[i];
        }
    }

    sql += " ORDER BY module_name, name";

    // apply LIMIT / OFFSET
    if (_filter.limit > 0)
    {
        sql += " LIMIT " + std::to_string(_filter.limit);

        if (_filter.offset > 0)
        {
            sql += " OFFSET " + std::to_string(_filter.offset);
        }
    }

    sql += ";";

    // execute
    stmt_ptr stmt = m_prepare(sql);

    if (!stmt)
    {
        return results;
    }

    // fetch rows
    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        results.push_back(m_extract_symbol_row(stmt.get()));
    }

    return results;
}

/*
d_catalogue_db::symbols_in_module
  Returns all symbols belonging to the named module.

Parameter(s):
  _module: the module name.
Return:
  a vector of matching symbols.
*/
std::vector<d_symbol_info>
d_catalogue_db::symbols_in_module
(
    const std::string& _module
)
const
{
    d_symbol_filter filter;
    filter.module_name = _module;

    return query_symbols(filter);
}

/*
d_catalogue_db::symbols_by_category
  Returns all symbols of the given category.

Parameter(s):
  _cat: the category to filter.
Return:
  a vector of matching symbols.
*/
std::vector<d_symbol_info>
d_catalogue_db::symbols_by_category
(
    DSymbolCategory _cat
)
const
{
    d_symbol_filter filter;
    filter.categories.push_back(_cat);

    return query_symbols(filter);
}

/*
d_catalogue_db::search_symbols
  Full-text search across name, qualified_name, description,
  and raw_comment.

Parameter(s):
  _text: the search text.
Return:
  a vector of matching symbols.
*/
std::vector<d_symbol_info>
d_catalogue_db::search_symbols
(
    const std::string& _text
)
const
{
    std::vector<d_symbol_info> results;
    std::string sql;

    sql = "SELECT * FROM symbols WHERE "
          "name LIKE ?1 OR "
          "qualified_name LIKE ?1 OR "
          "description LIKE ?1 OR "
          "raw_comment LIKE ?1 "
          "ORDER BY name;";

    stmt_ptr stmt = m_prepare(sql);

    if (!stmt)
    {
        return results;
    }

    std::string pattern = "%" + _text + "%";
    m_bind_text(stmt.get(), 1, pattern);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        results.push_back(m_extract_symbol_row(stmt.get()));
    }

    return results;
}

/*
d_catalogue_db::find_symbol_by_usr
  Finds a single symbol by USR.

Parameter(s):
  _usr: the USR to search for.
Return:
  the symbol if found, or a default-constructed d_symbol_info.
*/
d_symbol_info
d_catalogue_db::find_symbol_by_usr
(
    const std::string& _usr
)
const
{
    d_symbol_info result;

    stmt_ptr stmt = m_prepare(
        "SELECT * FROM symbols WHERE usr = ?1;");

    if (!stmt)
    {
        return result;
    }

    m_bind_text(stmt.get(), 1, _usr);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        result = m_extract_symbol_row(stmt.get());
    }

    return result;
}

/*
d_catalogue_db::find_symbol_by_name
  Finds the first symbol with the given unqualified name.

Parameter(s):
  _name: the name to search for.
Return:
  the first match, or a default-constructed d_symbol_info.
*/
d_symbol_info
d_catalogue_db::find_symbol_by_name
(
    const std::string& _name
)
const
{
    d_symbol_info result;

    stmt_ptr stmt = m_prepare(
        "SELECT * FROM symbols WHERE name = ?1 LIMIT 1;");

    if (!stmt)
    {
        return result;
    }

    m_bind_text(stmt.get(), 1, _name);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        result = m_extract_symbol_row(stmt.get());
    }

    return result;
}

/*
d_catalogue_db::all_modules
  Returns all module records.

Parameter(s):
  (none)
Return:
  a vector of all d_module_info records.
*/
std::vector<d_module_info>
d_catalogue_db::all_modules() const
{
    std::vector<d_module_info> results;

    stmt_ptr stmt = m_prepare(
        "SELECT * FROM modules ORDER BY name;");

    if (!stmt)
    {
        return results;
    }

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        results.push_back(m_extract_module_row(stmt.get()));
    }

    return results;
}

/*
d_catalogue_db::find_module
  Finds a module by name.

Parameter(s):
  _name: the module name.
Return:
  the module if found, or a default-constructed d_module_info.
*/
d_module_info
d_catalogue_db::find_module
(
    const std::string& _name
)
const
{
    d_module_info result;

    stmt_ptr stmt = m_prepare(
        "SELECT * FROM modules WHERE name = ?1;");

    if (!stmt)
    {
        return result;
    }

    m_bind_text(stmt.get(), 1, _name);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        result = m_extract_module_row(stmt.get());
    }

    return result;
}

// ============================================================
// dependency queries
// ============================================================

/*
d_catalogue_db::direct_dependencies
  Returns all USRs that _usr directly depends on.

Parameter(s):
  _usr: the source symbol's USR.
Return:
  a vector of target USRs.
*/
std::vector<std::string>
d_catalogue_db::direct_dependencies
(
    const std::string& _usr
)
const
{
    std::vector<std::string> result;

    stmt_ptr stmt = m_prepare(
        "SELECT to_usr FROM dependencies "
        "WHERE from_usr = ?1;");

    if (!stmt)
    {
        return result;
    }

    m_bind_text(stmt.get(), 1, _usr);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        result.push_back(m_column_text(stmt.get(), 0));
    }

    return result;
}

/*
d_catalogue_db::direct_dependents
  Returns all USRs that depend on _usr (reverse edges).

Parameter(s):
  _usr: the target symbol's USR.
Return:
  a vector of source USRs that reference _usr.
*/
std::vector<std::string>
d_catalogue_db::direct_dependents
(
    const std::string& _usr
)
const
{
    std::vector<std::string> result;

    stmt_ptr stmt = m_prepare(
        "SELECT from_usr FROM dependencies "
        "WHERE to_usr = ?1;");

    if (!stmt)
    {
        return result;
    }

    m_bind_text(stmt.get(), 1, _usr);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        result.push_back(m_column_text(stmt.get(), 0));
    }

    return result;
}

/*
d_catalogue_db::all_dependency_edges
  Returns every dependency edge in the database.

Parameter(s):
  (none)
Return:
  a vector of (from_usr, to_usr) pairs.
*/
std::vector<std::pair<std::string, std::string>>
d_catalogue_db::all_dependency_edges() const
{
    std::vector<std::pair<std::string, std::string>> edges;

    stmt_ptr stmt = m_prepare(
        "SELECT from_usr, to_usr FROM dependencies;");

    if (!stmt)
    {
        return edges;
    }

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        edges.emplace_back(m_column_text(stmt.get(), 0),
                           m_column_text(stmt.get(), 1));
    }

    return edges;
}

/*
d_catalogue_db::module_dependencies
  Returns the set of modules that _module depends on,
  by resolving symbol-level dependencies to module names.

Parameter(s):
  _module: the module name.
Return:
  a set of dependent module names.
*/
std::set<std::string>
d_catalogue_db::module_dependencies
(
    const std::string& _module
)
const
{
    std::set<std::string> result;

    stmt_ptr stmt = m_prepare(
        "SELECT DISTINCT s2.module_name "
        "FROM symbols s1 "
        "JOIN dependencies d ON s1.usr = d.from_usr "
        "JOIN symbols s2 ON d.to_usr = s2.usr "
        "WHERE s1.module_name = ?1 "
        "AND s2.module_name != ?1;");

    if (!stmt)
    {
        return result;
    }

    m_bind_text(stmt.get(), 1, _module);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        std::string dep = m_column_text(stmt.get(), 0);

        if (!dep.empty())
        {
            result.insert(dep);
        }
    }

    return result;
}

// ============================================================
// iteration
// ============================================================

/*
d_catalogue_db::for_each_symbol
  Iterates all symbols and calls _fn for each.

Parameter(s):
  _fn: the callback to invoke per symbol.
Return:
  none.
*/
void
d_catalogue_db::for_each_symbol
(
    std::function<void(const d_symbol_info&)> _fn
)
const
{
    stmt_ptr stmt = m_prepare(
        "SELECT * FROM symbols ORDER BY module_name, name;");

    if (!stmt)
    {
        return;
    }

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        _fn(m_extract_symbol_row(stmt.get()));
    }

    return;
}

/*
d_catalogue_db::for_each_module
  Iterates all modules and calls _fn for each.

Parameter(s):
  _fn: the callback to invoke per module.
Return:
  none.
*/
void
d_catalogue_db::for_each_module
(
    std::function<void(const d_module_info&)> _fn
)
const
{
    stmt_ptr stmt = m_prepare(
        "SELECT * FROM modules ORDER BY name;");

    if (!stmt)
    {
        return;
    }

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        _fn(m_extract_module_row(stmt.get()));
    }

    return;
}

// ============================================================
// statistics
// ============================================================

/*
d_catalogue_db::statistics
  Computes aggregate statistics from the database.

Parameter(s):
  (none)
Return:
  a populated d_db_stats.
*/
d_db_stats
d_catalogue_db::statistics() const
{
    d_db_stats stats;

    stats.total_symbols      = 0;
    stats.total_modules      = 0;
    stats.total_files        = 0;
    stats.total_dependencies = 0;

    // total symbols
    {
        stmt_ptr stmt = m_prepare(
            "SELECT COUNT(*) FROM symbols;");

        if ( (stmt) &&
             (sqlite3_step(stmt.get()) == SQLITE_ROW) )
        {
            stats.total_symbols = m_column_int(stmt.get(), 0);
        }
    }

    // total modules
    {
        stmt_ptr stmt = m_prepare(
            "SELECT COUNT(*) FROM modules;");

        if ( (stmt) &&
             (sqlite3_step(stmt.get()) == SQLITE_ROW) )
        {
            stats.total_modules = m_column_int(stmt.get(), 0);
        }
    }

    // total unique files
    {
        stmt_ptr stmt = m_prepare(
            "SELECT COUNT(DISTINCT def_file) FROM symbols "
            "WHERE def_file IS NOT NULL AND def_file != '';");

        if ( (stmt) &&
             (sqlite3_step(stmt.get()) == SQLITE_ROW) )
        {
            stats.total_files = m_column_int(stmt.get(), 0);
        }
    }

    // total dependencies
    {
        stmt_ptr stmt = m_prepare(
            "SELECT COUNT(*) FROM dependencies;");

        if ( (stmt) &&
             (sqlite3_step(stmt.get()) == SQLITE_ROW) )
        {
            stats.total_dependencies =
                m_column_int(stmt.get(), 0);
        }
    }

    // category breakdown
    {
        stmt_ptr stmt = m_prepare(
            "SELECT category, COUNT(*) FROM symbols "
            "GROUP BY category;");

        if (stmt)
        {
            while (sqlite3_step(stmt.get()) == SQLITE_ROW)
            {
                std::string cat_str =
                    m_column_text(stmt.get(), 0);
                int64_t cnt =
                    m_column_int(stmt.get(), 1);
                DSymbolCategory cat =
                    d_symbol_category_from_string(cat_str);

                stats.category_counts[cat] = cnt;
            }
        }
    }

    // per-module counts
    {
        stmt_ptr stmt = m_prepare(
            "SELECT module_name, COUNT(*) FROM symbols "
            "GROUP BY module_name;");

        if (stmt)
        {
            while (sqlite3_step(stmt.get()) == SQLITE_ROW)
            {
                std::string mod =
                    m_column_text(stmt.get(), 0);
                int64_t cnt =
                    m_column_int(stmt.get(), 1);

                stats.module_symbol_counts[mod] = cnt;
            }
        }
    }

    return stats;
}

// ============================================================
// error handling
// ============================================================

/*
d_catalogue_db::last_error
  Returns the last error message.

Parameter(s):
  (none)
Return:
  the error string.
*/
std::string
d_catalogue_db::last_error() const
{
    return m_last_error;
}

// ============================================================
// internal helpers
// ============================================================

/*
d_catalogue_db::m_exec
  Executes a single SQL statement.

Parameter(s):
  _sql: the SQL to execute.
Return:
  true on success.
*/
bool
d_catalogue_db::m_exec
(
    const std::string& _sql
)
{
    std::string err;

    return m_exec(_sql, err);
}

/*
d_catalogue_db::m_exec
  Executes a single SQL statement with error output.

Parameter(s):
  _sql: the SQL to execute.
  _err: receives the error message on failure.
Return:
  true on success.
*/
bool
d_catalogue_db::m_exec
(
    const std::string& _sql,
    std::string&       _err
)
{
    char* err_msg;
    int   rc;

    err_msg = nullptr;
    rc      = sqlite3_exec(m_db,
                           _sql.c_str(),
                           nullptr,
                           nullptr,
                           &err_msg);

    if (rc != SQLITE_OK)
    {
        if (err_msg)
        {
            _err         = err_msg;
            m_last_error = err_msg;
            sqlite3_free(err_msg);
        }

        return false;
    }

    return true;
}

/*
d_catalogue_db::m_prepare
  Prepares an SQL statement.

Parameter(s):
  _sql: the SQL to prepare.
Return:
  a managed statement pointer, or a null stmt_ptr on failure.
*/
d_catalogue_db::stmt_ptr
d_catalogue_db::m_prepare
(
    const std::string& _sql
)
const
{
    sqlite3_stmt* raw;
    int           rc;

    raw = nullptr;
    rc  = sqlite3_prepare_v2(m_db,
                             _sql.c_str(),
                             -1,
                             &raw,
                             nullptr);

    if (rc != SQLITE_OK)
    {
        return stmt_ptr(nullptr, m_stmt_deleter);
    }

    return stmt_ptr(raw, m_stmt_deleter);
}

/*
d_catalogue_db::m_bind_text
  Binds a text value to a prepared statement parameter.

Parameter(s):
  _stmt: the statement.
  _idx:  the 1-based parameter index.
  _val:  the text value.
Return:
  true on success.
*/
bool
d_catalogue_db::m_bind_text
(
    sqlite3_stmt*      _stmt,
    int                _idx,
    const std::string& _val
)
const
{
    return sqlite3_bind_text(_stmt,
                             _idx,
                             _val.c_str(),
                             -1,
                             SQLITE_TRANSIENT) == SQLITE_OK;
}

/*
d_catalogue_db::m_bind_int
  Binds an integer value to a prepared statement parameter.

Parameter(s):
  _stmt: the statement.
  _idx:  the 1-based parameter index.
  _val:  the integer value.
Return:
  true on success.
*/
bool
d_catalogue_db::m_bind_int
(
    sqlite3_stmt* _stmt,
    int           _idx,
    int64_t       _val
)
const
{
    return sqlite3_bind_int64(_stmt, _idx, _val) == SQLITE_OK;
}

/*
d_catalogue_db::m_column_text
  Extracts a text column value.

Parameter(s):
  _stmt: the statement positioned on a row.
  _idx:  the 0-based column index.
Return:
  the column value as std::string.
*/
std::string
d_catalogue_db::m_column_text
(
    sqlite3_stmt* _stmt,
    int           _idx
)
const
{
    const unsigned char* text =
        sqlite3_column_text(_stmt, _idx);

    if (text)
    {
        return std::string(
            reinterpret_cast<const char*>(text));
    }

    return "";
}

/*
d_catalogue_db::m_column_int
  Extracts an integer column value.

Parameter(s):
  _stmt: the statement positioned on a row.
  _idx:  the 0-based column index.
Return:
  the column value as int64_t.
*/
int64_t
d_catalogue_db::m_column_int
(
    sqlite3_stmt* _stmt,
    int           _idx
)
const
{
    return sqlite3_column_int64(_stmt, _idx);
}

// ============================================================
// row extraction
// ============================================================

/*
d_catalogue_db::m_extract_symbol_row
  Populates a d_symbol_info from the current SQLite row.

Parameter(s):
  _stmt: the statement positioned on a row.
Return:
  a populated d_symbol_info.
*/
d_symbol_info
d_catalogue_db::m_extract_symbol_row
(
    sqlite3_stmt* _stmt
)
const
{
    d_symbol_info sym;

    // columns follow the CREATE TABLE order
    sym.id             = m_column_int(_stmt, 0);
    sym.name           = m_column_text(_stmt, 1);
    sym.qualified_name = m_column_text(_stmt, 2);
    sym.usr            = m_column_text(_stmt, 3);
    sym.category       =
        d_symbol_category_from_string(
            m_column_text(_stmt, 4));
    sym.access         =
        (m_column_text(_stmt, 5) == "public")
            ? DAccessSpecifierPublic
        : (m_column_text(_stmt, 5) == "protected")
            ? DAccessSpecifierProtected
        : (m_column_text(_stmt, 5) == "private")
            ? DAccessSpecifierPrivate
            : DAccessSpecifierNone;
    sym.linkage        =
        (m_column_text(_stmt, 6) == "internal")
            ? DLinkageKindInternal
        : (m_column_text(_stmt, 6) == "external")
            ? DLinkageKindExternal
            : DLinkageKindNone;

    sym.definition_loc.file   = m_column_text(_stmt, 7);
    sym.definition_loc.line   = (uint32_t)m_column_int(_stmt, 8);
    sym.definition_loc.column = (uint32_t)m_column_int(_stmt, 9);
    sym.declaration_loc.file   = m_column_text(_stmt, 10);
    sym.declaration_loc.line   = (uint32_t)m_column_int(_stmt, 11);
    sym.declaration_loc.column = (uint32_t)m_column_int(_stmt, 12);

    sym.type_spelling   = m_column_text(_stmt, 13);
    sym.return_type     = m_column_text(_stmt, 14);
    sym.underlying_type = m_column_text(_stmt, 15);

    sym.is_const        = (m_column_int(_stmt, 16) != 0);
    sym.is_static       = (m_column_int(_stmt, 17) != 0);
    sym.is_virtual      = (m_column_int(_stmt, 18) != 0);
    sym.is_pure_virtual = (m_column_int(_stmt, 19) != 0);
    sym.is_inline       = (m_column_int(_stmt, 20) != 0);
    sym.is_constexpr    = (m_column_int(_stmt, 21) != 0);
    sym.is_noexcept     = (m_column_int(_stmt, 22) != 0);
    sym.is_template     = (m_column_int(_stmt, 23) != 0);
    sym.is_variadic     = (m_column_int(_stmt, 24) != 0);
    sym.is_definition   = (m_column_int(_stmt, 25) != 0);
    sym.is_deprecated   = (m_column_int(_stmt, 26) != 0);

    sym.parameters =
        m_deserialize_params(m_column_text(_stmt, 27));
    sym.template_parameters =
        m_deserialize_string_vec(m_column_text(_stmt, 28));
    sym.base_classes =
        m_deserialize_string_vec(m_column_text(_stmt, 29));
    sym.enum_constants =
        m_deserialize_enum_constants(
            m_column_text(_stmt, 30));

    sym.comment.raw_comment   = m_column_text(_stmt, 31);
    sym.comment.brief_comment = m_column_text(_stmt, 32);
    sym.comment.category_tag  = m_column_text(_stmt, 33);
    sym.comment.description   = m_column_text(_stmt, 34);
    sym.comment.return_doc    = m_column_text(_stmt, 35);
    sym.comment.parameter_docs =
        m_deserialize_param_docs(m_column_text(_stmt, 36));

    sym.module_name = m_column_text(_stmt, 37);
    sym.parent_usr  = m_column_text(_stmt, 38);
    sym.parent_name = m_column_text(_stmt, 39);

    return sym;
}

/*
d_catalogue_db::m_extract_module_row
  Populates a d_module_info from the current SQLite row.

Parameter(s):
  _stmt: the statement positioned on a row.
Return:
  a populated d_module_info.
*/
d_module_info
d_catalogue_db::m_extract_module_row
(
    sqlite3_stmt* _stmt
)
const
{
    d_module_info mod;

    mod.id           = m_column_int(_stmt, 0);
    mod.name         = m_column_text(_stmt, 1);
    mod.path         = m_column_text(_stmt, 2);
    mod.description  = m_column_text(_stmt, 3);
    mod.header_files =
        m_deserialize_string_vec(m_column_text(_stmt, 4));
    mod.source_files =
        m_deserialize_string_vec(m_column_text(_stmt, 5));
    mod.includes     =
        m_deserialize_string_vec(m_column_text(_stmt, 6));

    // deserialize depends_on into the set
    std::vector<std::string> deps =
        m_deserialize_string_vec(m_column_text(_stmt, 7));

    for (const auto& d : deps)
    {
        mod.depends_on_modules.insert(d);
    }

    return mod;
}

// ============================================================
// serialization
// ============================================================

/*
d_catalogue_db::m_serialize_string_vec
  Serializes a vector of strings as pipe-delimited text.

Parameter(s):
  _vec: the vector to serialize.
Return:
  a pipe-delimited string.
*/
std::string
d_catalogue_db::m_serialize_string_vec
(
    const std::vector<std::string>& _vec
)
const
{
    std::string result;

    for (size_t i = 0; i < _vec.size(); i++)
    {
        if (i > 0)
        {
            result += "|";
        }

        result += _vec[i];
    }

    return result;
}

/*
d_catalogue_db::m_deserialize_string_vec
  Deserializes a pipe-delimited string to a vector of strings.

Parameter(s):
  _str: the serialized string.
Return:
  the deserialized vector.
*/
std::vector<std::string>
d_catalogue_db::m_deserialize_string_vec
(
    const std::string& _str
)
const
{
    std::vector<std::string> result;
    std::istringstream       stream(_str);
    std::string              token;

    if (_str.empty())
    {
        return result;
    }

    while (std::getline(stream, token, '|'))
    {
        if (!token.empty())
        {
            result.push_back(token);
        }
    }

    return result;
}

/*
d_catalogue_db::m_serialize_params
  Serializes parameters as "name:type|name:type|...".

Parameter(s):
  _params: the parameter vector.
Return:
  the serialized string.
*/
std::string
d_catalogue_db::m_serialize_params
(
    const std::vector<d_parameter_info>& _params
)
const
{
    std::string result;

    for (size_t i = 0; i < _params.size(); i++)
    {
        if (i > 0)
        {
            result += "|";
        }

        result += _params[i].name + ":"
                + _params[i].type_spelling;
    }

    return result;
}

/*
d_catalogue_db::m_deserialize_params
  Deserializes "name:type|name:type|..." to a parameter vector.

Parameter(s):
  _str: the serialized string.
Return:
  the deserialized parameter vector.
*/
std::vector<d_parameter_info>
d_catalogue_db::m_deserialize_params
(
    const std::string& _str
)
const
{
    std::vector<d_parameter_info> result;
    std::vector<std::string>      parts;

    if (_str.empty())
    {
        return result;
    }

    parts = m_deserialize_string_vec(_str);

    for (const auto& part : parts)
    {
        size_t colon = part.find(':');

        if (colon != std::string::npos)
        {
            d_parameter_info pi;
            pi.name          = part.substr(0, colon);
            pi.type_spelling = part.substr(colon + 1);
            result.push_back(pi);
        }
    }

    return result;
}

/*
d_catalogue_db::m_serialize_param_docs
  Serializes parameter doc pairs as "name:desc|name:desc|...".

Parameter(s):
  _docs: the parameter doc pairs.
Return:
  the serialized string.
*/
std::string
d_catalogue_db::m_serialize_param_docs
(
    const std::vector<std::pair<std::string, std::string>>& _docs
)
const
{
    std::string result;

    for (size_t i = 0; i < _docs.size(); i++)
    {
        if (i > 0)
        {
            result += "|";
        }

        result += _docs[i].first + ":" + _docs[i].second;
    }

    return result;
}

/*
d_catalogue_db::m_deserialize_param_docs
  Deserializes parameter doc pairs.

Parameter(s):
  _str: the serialized string.
Return:
  the deserialized doc pairs.
*/
std::vector<std::pair<std::string, std::string>>
d_catalogue_db::m_deserialize_param_docs
(
    const std::string& _str
)
const
{
    std::vector<std::pair<std::string, std::string>> result;
    std::vector<std::string> parts;

    if (_str.empty())
    {
        return result;
    }

    parts = m_deserialize_string_vec(_str);

    for (const auto& part : parts)
    {
        size_t colon = part.find(':');

        if (colon != std::string::npos)
        {
            result.emplace_back(part.substr(0, colon),
                                part.substr(colon + 1));
        }
    }

    return result;
}

/*
d_catalogue_db::m_serialize_enum_constants
  Serializes enum constants as "name=val|name=val|...".

Parameter(s):
  _consts: the enum constant pairs.
Return:
  the serialized string.
*/
std::string
d_catalogue_db::m_serialize_enum_constants
(
    const std::vector<std::pair<std::string, int64_t>>& _consts
)
const
{
    std::string result;

    for (size_t i = 0; i < _consts.size(); i++)
    {
        if (i > 0)
        {
            result += "|";
        }

        result += _consts[i].first + "="
                + std::to_string(_consts[i].second);
    }

    return result;
}

/*
d_catalogue_db::m_deserialize_enum_constants
  Deserializes enum constants from "name=val|name=val|...".

Parameter(s):
  _str: the serialized string.
Return:
  the deserialized enum constant pairs.
*/
std::vector<std::pair<std::string, int64_t>>
d_catalogue_db::m_deserialize_enum_constants
(
    const std::string& _str
)
const
{
    std::vector<std::pair<std::string, int64_t>> result;
    std::vector<std::string> parts;

    if (_str.empty())
    {
        return result;
    }

    parts = m_deserialize_string_vec(_str);

    for (const auto& part : parts)
    {
        size_t eq = part.find('=');

        if (eq != std::string::npos)
        {
            std::string name = part.substr(0, eq);
            int64_t     val  = 0;

            try
            {
                val = std::stoll(part.substr(eq + 1));
            }
            catch (...)
            {
                val = 0;
            }

            result.emplace_back(name, val);
        }
    }

    return result;
}

} // namespace d_catalogue
