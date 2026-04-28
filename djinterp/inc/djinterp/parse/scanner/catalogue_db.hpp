// catalogue_db.hpp
//
//   Module 2: database storage layer. Stores all scanned symbol
// and module information in an SQLite database for persistent
// querying, filtering, and export to the wiki module.

#ifndef D_CATALOGUE_DATABASE_CATALOGUE_DB_HPP
#define D_CATALOGUE_DATABASE_CATALOGUE_DB_HPP

#include "../common/types.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// forward-declare the opaque sqlite3 handle so callers do not
// need to include sqlite3.h
struct sqlite3;
struct sqlite3_stmt;

namespace d_catalogue {

// ============================================================
// query filters
// ============================================================

// d_symbol_filter
//   struct: filter criteria for querying symbols from the
//   database. All fields are optional; only non-empty fields
//   are applied.
struct d_symbol_filter
{
    std::string              name_pattern;
    std::string              module_name;
    std::string              parent_usr;
    std::string              file_pattern;
    std::vector<DSymbolCategory>  categories;
    std::vector<DAccessSpecifier> access_levels;
    bool                     definitions_only;
    bool                     templates_only;
    int                      limit;
    int                      offset;

    d_symbol_filter();
};

// ============================================================
// database statistics
// ============================================================

// d_db_stats
//   struct: aggregate statistics from the database.
struct d_db_stats
{
    int64_t                            total_symbols;
    int64_t                            total_modules;
    int64_t                            total_files;
    int64_t                            total_dependencies;
    std::map<DSymbolCategory, int64_t> category_counts;
    std::map<std::string, int64_t>     module_symbol_counts;
};

// ============================================================
// catalogue database
// ============================================================

// d_catalogue_db
//   class: SQLite-backed storage for framework symbol catalogues.
// Provides CRUD operations, filtered queries, full-text search,
// and dependency graph access for the wiki generator.
class d_catalogue_db
{
private:
    using stmt_ptr = std::unique_ptr<sqlite3_stmt,
                         void(*)(sqlite3_stmt*)>;

public:
    d_catalogue_db();
    explicit d_catalogue_db(const d_database_config& _config);
    ~d_catalogue_db();

    // -- lifecycle --

    bool open(const std::string& _path);
    bool open(const d_database_config& _config);
    void close();
    bool is_open() const;

    // -- schema --

    bool create_tables();
    bool drop_tables();

    // -- bulk import (from scanner) --

    bool begin_transaction();
    bool commit_transaction();
    bool rollback_transaction();

    bool insert_symbol(const d_symbol_info& _sym);
    bool insert_module(const d_module_info& _mod);
    bool insert_dependency(const std::string& _from_usr, const std::string& _to_usr);

    int64_t bulk_insert_symbols(const std::map<std::string, d_symbol_info>& _symbols);
    int64_t bulk_insert_modules(const std::map<std::string, d_module_info>& _modules);

    // -- queries --

    std::vector<d_symbol_info> query_symbols(const d_symbol_filter& _filter) const;
    std::vector<d_symbol_info> symbols_in_module(const std::string& _module) const;
    std::vector<d_symbol_info> symbols_by_category(DSymbolCategory _cat) const;
    std::vector<d_symbol_info> search_symbols(const std::string& _text) const;

    d_symbol_info find_symbol_by_usr(const std::string& _usr) const;
    d_symbol_info find_symbol_by_name(const std::string& _name) const;

    std::vector<d_module_info> all_modules() const;
    d_module_info              find_module(const std::string& _name) const;

    // -- dependency queries --

    std::vector<std::string> direct_dependencies(const std::string& _usr) const;
    std::vector<std::string> direct_dependents(const std::string& _usr) const;
    std::vector<std::pair<std::string, std::string>> all_dependency_edges() const;
    std::set<std::string> module_dependencies(const std::string& _module) const;

    // -- iteration --

    void for_each_symbol(std::function<void(const d_symbol_info&)> _fn) const;
    void for_each_module(std::function<void(const d_module_info&)> _fn) const;

    // -- statistics --

    d_db_stats statistics() const;

    // -- error handling --

    std::string last_error() const;

private:
    // -- schema helpers --

    bool m_exec(const std::string& _sql);
    bool m_exec(const std::string& _sql, std::string& _err);

    // -- prepared statement helpers --

    stmt_ptr m_prepare(const std::string& _sql) const;
    bool     m_bind_text(sqlite3_stmt* _stmt, int _idx, const std::string& _val) const;
    bool     m_bind_int(sqlite3_stmt* _stmt, int _idx, int64_t _val) const;

    std::string m_column_text(sqlite3_stmt* _stmt, int _idx) const;
    int64_t     m_column_int(sqlite3_stmt* _stmt, int _idx) const;

    // -- row extraction --

    d_symbol_info m_extract_symbol_row(sqlite3_stmt* _stmt) const;
    d_module_info m_extract_module_row(sqlite3_stmt* _stmt) const;

    // -- serialization helpers --

    std::string m_serialize_string_vec(const std::vector<std::string>& _vec) const;
    std::vector<std::string> m_deserialize_string_vec(const std::string& _str) const;

    std::string m_serialize_params(const std::vector<d_parameter_info>& _params) const;
    std::vector<d_parameter_info> m_deserialize_params(const std::string& _str) const;

    std::string m_serialize_param_docs(const std::vector<std::pair<std::string, std::string>>& _docs) const;
    std::vector<std::pair<std::string, std::string>> m_deserialize_param_docs(const std::string& _str) const;

    std::string m_serialize_enum_constants(const std::vector<std::pair<std::string, int64_t>>& _consts) const;
    std::vector<std::pair<std::string, int64_t>> m_deserialize_enum_constants(const std::string& _str) const;

    // -- members --

    sqlite3*          m_db;
    d_database_config m_config;
    std::string       m_last_error;
};

} // namespace d_catalogue

#endif // D_CATALOGUE_DATABASE_CATALOGUE_DB_HPP
