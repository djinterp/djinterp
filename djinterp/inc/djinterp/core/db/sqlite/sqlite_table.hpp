/*******************************************************************************
* djinterp [core]                                               sqlite_table.hpp
*
* A table of an SQLite database held in memory: database_table over an
* sqlite_connection, with what SQLite adds.
*   sqlite_table is database_table<sqlite_connection> -- the cache, refresh,
* commit and the table's operations, through the SQLite layer -- and adds:
* the attached database the table lives in, under the name ATTACH gave it;
* STRICT tables (SQLite 3.37.0) and WITHOUT ROWID tables (3.8.2), which
* create_table makes; and upkeep: ANALYZE of the table, VACUUM of its
* database, and REINDEX of the table's indexes.
*   SQLite types its values as they are stored, not as a column declares
* them, unless the table is STRICT; the layer declares each column as
* sqlite_schema.h tabulates, and a value is read back as stored.
*
*
* path:      /inc/djinterp/core/db/sqlite/sqlite_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.23
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  IDENTIFIERS
    -----------
    1.  sqlite_quote
2.  SQLITE TABLE
    ------------
    1.  The class
         1.  sqlite_table
*/

#ifndef DJINTERP_DB_SQLITE_SQLITE_TABLE_HPP
#define DJINTERP_DB_SQLITE_SQLITE_TABLE_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>   // std::string
#include <utility>  // std::move
// djinterp
#include "../../../djinterp.hpp"  // framework root
#include "../database_table.hpp"  // database_table, table_schema, sync_config
#include "./sqlite.hpp"           // sqlite_connection, D_SQLITE_STRICT,
                                  // D_SQLITE_WITHOUT_ROWID


NS_DJINTERP


//==============================================================================
// 1.  IDENTIFIERS
//==============================================================================


// 1.1    sqlite_quote
//------------------------------------------------------------------------------
// sqlite_quote
//   function: an identifier quoted as SQLite and the standard quote one, in
// double quotes, each double quote inside doubled; as quote_identifier(name,
// database_type::sqlite) quotes it, but for a NUL, which this keeps.
inline std::string
sqlite_quote(
    const std::string& _id
)
{
    std::string quoted;

    quoted.reserve(_id.size() + 2u);
    quoted += '"';

    // each double quote doubled
    for (const char c : _id)
    {
        if (c == '"')
        {
            quoted += '"';
        }

        quoted += c;
    }

    quoted += '"';

    return quoted;
}


//==============================================================================
// 2.  SQLITE TABLE
//==============================================================================


// 2.1    The class
//------------------------------------------------------------------------------
// 2.1.1
// sqlite_table
//   class template: a table of an SQLite database, as the banner describes.
// The attached database is the table's schema (set_schema_name), "main" or
// "" for the database the connection opened, "temp" for its temporary one.
// STRICT and WITHOUT ROWID apply to the next create_table; a library older
// than either refuses it, with D_DB_UNSUPPORTED. Each member throws as
// database_table's do. Not copyable; movable.
//
// Usage:
//     sqlite_table<> notes(db, schema);  // a table_schema with a key
//
//     notes.set_strict(true);
//     notes.create_table();             // CREATE TABLE ... STRICT
template<typename Config = void>
class sqlite_table : public database_table<sqlite_connection, value, Config>
{
private:
    using base_type = database_table<sqlite_connection, value, Config>;

public:
    using typename base_type::size_type;
    using typename base_type::value_type;
    using typename base_type::row_type;
    using typename base_type::connection_type;
    using typename base_type::schema_type;
    using self_type = sqlite_table<Config>;

    using type_support = sqlite_type_support;
    using version_info = sqlite_version_info;

    // construction -- as database_table's
    sqlite_table()
        : base_type(),
          m_without_rowid(false),
          m_strict(false)
    {}

    explicit sqlite_table(
        sqlite_connection& _conn,
        std::string        _table_name,
        table_kind         _kind = table_kind::base_table
    )
        : base_type(_conn, std::move(_table_name), _kind),
          m_without_rowid(false),
          m_strict(false)
    {}

    explicit sqlite_table(
        sqlite_connection& _conn,
        table_schema       _schema,
        table_kind         _kind = table_kind::base_table
    )
        : base_type(_conn, std::move(_schema), _kind),
          m_without_rowid(false),
          m_strict(false)
    {}

    explicit sqlite_table(
        sqlite_connection& _conn,
        table_schema       _schema,
        table_kind         _kind,
        const sync_config& _sync
    )
        : base_type(_conn, std::move(_schema), _kind, _sync),
          m_without_rowid(false),
          m_strict(false)
    {}

    ~sqlite_table() = default;

    sqlite_table(const sqlite_table&)            = delete;
    sqlite_table& operator=(const sqlite_table&) = delete;

    sqlite_table(sqlite_table&&) noexcept            = default;
    sqlite_table& operator=(sqlite_table&&) noexcept = default;

    // attached database -- the name ATTACH gave the database the table lives
    // in, "" for the one the connection opened: the table's schema
    void
    set_attach_alias(
        std::string _alias
    )
    {
        this->set_schema_name(std::move(_alias));

        return;
    }

    const std::string&
    get_attach_alias() const noexcept
    {
        return this->schema_name();
    }

    // qualified_table_name -- the table's name for SQL, as qualified_name()
    std::string
    qualified_table_name() const
    {
        return this->qualified_name();
    }

    // table options -- for the next create_table: WITHOUT ROWID, which needs
    // a primary key, from SQLite 3.8.2; STRICT, which holds each column to
    // its declared type, from 3.37.0
    void
    set_without_rowid(
        bool _enabled
    ) noexcept
    {
        m_without_rowid = _enabled;

        return;
    }

    bool
    is_without_rowid() const noexcept
    {
        return m_without_rowid;
    }

    void
    set_strict(
        bool _enabled
    ) noexcept
    {
        m_strict = _enabled;

        return;
    }

    bool
    is_strict() const noexcept
    {
        return m_strict;
    }

    /**
     * @brief Creates the table the schema describes, as database_table's
     *        create_table does, STRICT and WITHOUT ROWID as set.
     *
     * @param[in] _if_not_exists  whether a table already there is a success.
     * @throws as database_table's create_table; query_exception with
     *         D_DB_UNSUPPORTED for an option the library is too old for.
     */
    void
    create_table(
        bool _if_not_exists = true
    )
    {
        this->create_table_with(((_if_not_exists) ? D_DB_IF_NOT_EXISTS : 0u) |
                                ((m_strict) ? D_SQLITE_STRICT : 0u) |
                                ((m_without_rowid) ? D_SQLITE_WITHOUT_ROWID
                                                   : 0u));

        return;
    }

    // upkeep -- ANALYZE of the table, for the query planner's statistics;
    // VACUUM of the database it lives in, which rebuilds the file and cannot
    // run inside a transaction (the layer refuses a schema's VACUUM before
    // SQLite 3.15.0, D_DB_UNSUPPORTED); REINDEX of the table's indexes. Each
    // throws as database_table's members do.
    void
    analyze_table()
    {
        this->validate_connected("analyze_table");

        (void)this->m_connection->execute_update("ANALYZE " +
                                                 this->qualified_name());

        return;
    }

    void
    vacuum_database()
    {
        this->validate_connected("vacuum_database");

        this->m_connection->run_layer(
            "vacuum",
            [this](face::sqlite& _db) -> int
            {
                return _db.vacuum(this->layer_scope());
            });

        return;
    }

    void
    reindex_table()
    {
        this->validate_connected("reindex_table");

        (void)this->m_connection->execute_update("REINDEX " +
                                                 this->qualified_name());

        return;
    }

protected:
    bool m_without_rowid;
    bool m_strict;
};


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_SQLITE_SQLITE_TABLE_HPP
