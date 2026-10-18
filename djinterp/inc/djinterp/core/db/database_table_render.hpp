/*******************************************************************************
* djinterp [core]                                      database_table_render.hpp
*
* The database target of the container render fold, for the tables core/db
* holds: a database_table seen as the tabular template, and the fold's two
* directions through it.
*   A container renders to a store through the tabular template Theta =
* { table, row, cell, key, foreign-ref } (containers.tex, "Database
* serializability" and "The table"), folding twice:
*
*       container  --enc_T-->  Theta  --[[.]]_D-->  store
*
* This header is enc_T's half for a database_table, and for every vendor's
* table, each of which derives from it: relational_table_view presents one
* to Theta, as the read surface the table already has -- rows(), cols(),
* cell(), get_schema(), key_columns() -- without copying it. [[.]]_D, the
* store's half, is the table's own write path: the vendor's layer spells
* each column's type and every change to the table (d_<vendor>_create_table
* and the rest, through the faces), quote_identifier (database.hpp) quotes
* its names, and commit() writes its rows. So store_D, for a database_table,
* is its commit(), and retrieve_D, the adjoint direction, its refresh().
*   A database_table is not a table_base: table_base<Derived, Type> is a
* CRTP mixin over one contiguous buffer of one type of cell, row by row,
* reached through data(), and a database_table holds rows apart, each a
* vector of values typed by column. A relational table is the rank-2,
* column-typed projection of the general container table, blind to the
* order of its rows, and enters the fold through the tabular template
* rather than the contiguous one.
*   A container that is not a database_table -- one table-shaped, to be
* written into a new table -- needs the container serial layer's encoder,
* which this header does not yet see: store_D and retrieve_D refuse one at
* compile time, and the fold's binding, table_traits, is compiled only when
* that layer defines DJINTERP_HAVE_CONTAINER_TABLE_RENDER.
*
*
* path:      /inc/djinterp/core/db/database_table_render.hpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DETECTION
    ---------
    1.  Database tables
         1.  is_database_table
         2.  is_database_table_v
2.  THE TABULAR TEMPLATE
    --------------------
    1.  The view
         1.  relational_table_view
         2.  make_relational_table_view
3.  THE STORE TARGET
    ----------------
    1.  The fold's two directions
         1.  store_D
         2.  retrieve_D
4.  THE FOLD'S BINDING
    ------------------
    1.  table_traits
*/

#ifndef DJINTERP_DB_DATABASE_TABLE_RENDER_HPP
#define DJINTERP_DB_DATABASE_TABLE_RENDER_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <memory>       // std::addressof
#include <string>       // std::string
#include <type_traits>  // std::true_type, std::false_type,
                        // std::remove_reference_t, std::remove_cv_t,
                        // std::is_const_v, std::is_convertible_v,
                        // std::enable_if_t
#include <utility>      // std::declval
#include <vector>       // std::vector
// djinterp
#include "../../djinterp.hpp"     // framework root
#include "./database.hpp"         // field_type, query_exception, store_D,
                                  // retrieve_D
#include "./database_table.hpp"   // database_table, table_schema,
                                  // column_info


NS_DJINTERP


//==============================================================================
// 1.  DETECTION
//==============================================================================


NS_INTERNAL

    // database_table_base_test -- std::true_type for a pointer to any
    // database_table<Connection, ValueType, Config>, a vendor's table
    // converting to its base; std::false_type for anything else
    template<typename Connection,
             typename ValueType,
             typename Config>
    std::true_type
    database_table_base_test(
        const database_table<Connection, ValueType, Config>*
    );

    std::false_type
    database_table_base_test(...);

    template<typename Type>
    using is_database_table_impl =
        decltype(database_table_base_test(
            std::declval<std::remove_reference_t<Type>*>()));

NS_END  // internal


// 1.1    Database tables
//------------------------------------------------------------------------------
// 1.1.1
// is_database_table
//   trait: whether Type, its references and qualifiers aside, is a
// database_table, or derives from one publicly -- every vendor's table,
// mysql_common_table among them, with no trait of its own.
template<typename Type>
struct is_database_table
    : internal::is_database_table_impl<Type>
{};

// 1.1.2
// is_database_table_v
//   constant: is_database_table<Type>::value.
template<typename Type>
inline constexpr bool is_database_table_v = is_database_table<Type>::value;


//==============================================================================
// 2.  THE TABULAR TEMPLATE
//==============================================================================


// 2.1    The view
//------------------------------------------------------------------------------
// 2.1.1
// relational_table_view
//   class template: a database_table, or a vendor's, seen as the tabular
// template, Theta. It owns nothing and copies nothing: each member reads the
// table it views, which must outlive it, as the table now is. Theta's nodes
// are:
//     table        table_name(), and the extent, rows() by cols()
//     row          a row's index, from 0 to rows()
//     cell         cell(row, col), the value the cache holds
//     key          the primary key, as the table's commit() takes it:
//                  primary_key_columns(), or, where it names none, the
//                  columns marked is_primary_key
//     foreign-ref  foreign_ref_of(col), the reference the column notes
// Each member throws as the table's own does.
//
// Usage:
//     sqlite_table<> scores(db, "scores");
//
//     scores.refresh();
//
//     const relational_table_view<sqlite_table<>> seen =
//         make_relational_table_view(scores);
//
//     for (std::size_t r = 0; r < seen.rows(); ++r)
//     {
//         ...                         // seen.cell(r, c), for each column
//     }
template<typename Table>
class relational_table_view
{
    static_assert(is_database_table_v<Table>,
                  "relational_table_view views a database_table, or a table "
                  "deriving from one; a document, key-value or wide-column "
                  "store is not a relational table");

public:
    using table_type  = Table;
    using size_type   = std::size_t;
    using value_type  = typename Table::value_type;
    using schema_type = table_schema;
    using column_type = column_info;

    // foreign_ref -- a foreign-ref node: the table and column a column
    // refers to, engaged only where it refers to one
    struct foreign_ref
    {
        bool        engaged = false;
        std::string target_table;
        std::string target_column;
    };

    explicit relational_table_view(
        const Table& _table
    ) noexcept
        : m_table(std::addressof(_table))
    {}

    // table_name -- the table node's name, without its schema
    const std::string&
    table_name() const noexcept
    {
        return m_table->get_schema().table_name;
    }

    // rows, cols -- the extent: the rows the cache holds, and the columns
    // the schema describes
    size_type
    rows() const noexcept
    {
        return m_table->rows();
    }

    size_type
    cols() const noexcept
    {
        return m_table->cols();
    }

    // cell -- a cell node, unchecked, as the table's const cell(): the value
    // the cache holds, read from the database (a value of the column's type,
    // or NULL) or set by the program (what it was set to, which the database
    // converts or refuses when it is committed)
    const value_type&
    cell(
        size_type _row,
        size_type _col
    ) const
    {
        return m_table->cell(_row, _col);
    }

    // column_name, column_field_type -- a column's name, and its type as
    // field_type names it; the vendor's layer spells the type for its
    // database (each connection's sql_type_name names it), so the view
    // speaks no vendor's dialect
    const std::string&
    column_name(
        size_type _col
    ) const
    {
        return m_table->get_schema().columns[_col].name;
    }

    field_type
    column_field_type(
        size_type _col
    ) const
    {
        return m_table->get_schema().columns[_col].type;
    }

    /**
     * @brief Says whether a column is one of the primary key's, as the
     *        table's commit() takes the key.
     *
     * @note A relational table is blind to the order of its rows; a fold
     *       that must give them back in order adds a column that numbers
     *       them, before rendering.
     *
     * @param[in] _col  the column's index, which must be one of the table's.
     * @return whether the key holds it.
     */
    bool
    is_primary_key_column(
        size_type _col
    ) const
    {
        const schema_type& schema = m_table->get_schema();

        // the column marked, where the key names no column
        if (schema.primary_key_columns.empty())
        {
            return schema.columns[_col].is_primary_key;
        }

        // otherwise among those it names
        for (const std::string& name : schema.primary_key_columns)
        {
            if (name == schema.columns[_col].name)
            {
                return true;
            }
        }

        return false;
    }

    // primary_key_columns -- the key's columns by name, in the key's order;
    // throws as the table's key_columns(), with D_DB_MISMATCH for a key
    // that names a column the table lacks
    std::vector<std::string>
    primary_key_columns() const
    {
        std::vector<std::string> names;

        for (const size_type column : m_table->key_columns())
        {
            names.push_back(m_table->get_schema().columns[column].name);
        }

        return names;
    }

    // has_primary_key -- whether the table has a key, which makes its
    // commit() differential, rather than the replacing of the rows read
    bool
    has_primary_key() const noexcept
    {
        const schema_type& schema = m_table->get_schema();

        if (!schema.primary_key_columns.empty())
        {
            return true;
        }

        for (const column_type& column : schema.columns)
        {
            if (column.is_primary_key)
            {
                return true;
            }
        }

        return false;
    }

    // foreign_ref_of -- a column's foreign-ref node, from the reference its
    // column_info notes (a program's note, which fetch_schema() leaves as
    // it was)
    foreign_ref
    foreign_ref_of(
        size_type _col
    ) const
    {
        const column_type& column = m_table->get_schema().columns[_col];
        foreign_ref        reference;

        if (column.foreign_table.has_value())
        {
            reference.engaged       = true;
            reference.target_table  = column.foreign_table.value();
            reference.target_column = column.foreign_column.value_or(
                                          std::string());
        }

        return reference;
    }

    // table -- the table viewed
    const Table&
    table() const noexcept
    {
        return *m_table;
    }

private:
    const Table* m_table;
};

// 2.1.2
// make_relational_table_view
//   function template: a relational_table_view of a table, its type
// deduced.
template<typename Table>
relational_table_view<Table>
make_relational_table_view(
    const Table& _table
) noexcept
{
    return relational_table_view<Table>(_table);
}


//==============================================================================
// 3.  THE STORE TARGET
//==============================================================================
// The definitions of database.hpp's declarations. For a database_table the
// store is the database its connection reaches, and the connection given
// must be the table's own: a table bound to another is refused, with
// D_DB_INVALID_ARGUMENT, before anything is read or written, and one bound
// to none is refused as its own members refuse it.


NS_INTERNAL

    // refuse_other_store -- query_exception, with D_DB_INVALID_ARGUMENT, for
    // a table bound to a connection other than the one given; a connection
    // of a kind the table cannot bind to does not compile
    template<typename Connection,
             typename Table>
    void
    refuse_other_store(
        const Connection& _connection,
        const Table&      _table,
        const char*       _caller
    )
    {
        using bound_type = typename Table::connection_type;

        static_assert(std::is_convertible_v<const Connection*,
                                            const bound_type*>,
                      "the connection is not of the kind the table binds to");

        const bound_type* store = std::addressof(_connection);

        if ( (_table.get_connection() != nullptr) &&
             (_table.get_connection() != store) )
        {
            throw query_exception(std::string(_caller) +
                                  ": the table is bound to another "
                                  "connection",
                                  D_DB_INVALID_ARGUMENT);
        }

        return;
    }

NS_END  // internal


// 3.1    The fold's two directions
//------------------------------------------------------------------------------
// 3.1.1
// store_D
//   function template: renders a table-shaped container into the store
// behind a connection, through Theta. A database_table, bound to that
// connection, is committed: its commit(), the vendor's own where it has one
// (postgres_table's COPY among them), writes its changes as one unit. It is
// taken by a forwarding reference, and refused at compile time when const:
// a commit changes the table, which then compares with what it wrote. Any
// other container needs the container serial layer, and is refused at
// compile time.
//
// Usage:
//     scores.update_cell(0, "points", value(42));
//     store_D(db, scores);                   // scores.commit()
template<typename Connection,
         typename RenderableTable>
void
store_D(
    Connection&       _connection,
    RenderableTable&& _table
)
{
    using table_type = std::remove_reference_t<RenderableTable>;

    if constexpr (is_database_table_v<table_type>)
    {
        static_assert(!std::is_const_v<table_type>,
                      "store_D commits a database_table, which changes it: "
                      "pass it non-const");

        internal::refuse_other_store(_connection, _table, "store_D");
        _table.commit();
    }
    else
    {
        static_assert(is_database_table_v<table_type>,
                      "store_D renders a container other than a "
                      "database_table through the container serial layer's "
                      "encoder, enc_T, which is not available; give store_D "
                      "a database_table bound to the connection");
    }

    return;
}

// 3.1.2
// retrieve_D
//   function template: the adjoint direction, a table-shaped container
// filled from the store behind a connection, through Theta. A
// database_table, bound to that connection, is refreshed: its refresh()
// reads the rows its query selects, the columns too where it knows none,
// replacing what the cache held. Any other container needs the container
// serial layer, and is refused at compile time.
template<typename Connection,
         typename RenderableTable>
void
retrieve_D(
    Connection&      _connection,
    RenderableTable& _table
)
{
    if constexpr (is_database_table_v<RenderableTable>)
    {
        static_assert(!std::is_const_v<RenderableTable>,
                      "retrieve_D refreshes a database_table, which changes "
                      "it: pass it non-const");

        internal::refuse_other_store(_connection, _table, "retrieve_D");
        _table.refresh();
    }
    else
    {
        static_assert(is_database_table_v<RenderableTable>,
                      "retrieve_D fills a container other than a "
                      "database_table through the container serial layer's "
                      "decoder, the adjoint of enc_T, which is not "
                      "available; give retrieve_D a database_table bound to "
                      "the connection");
    }

    return;
}


//==============================================================================
// 4.  THE FOLD'S BINDING
//==============================================================================
// The container render fold chooses the tabular encoder through the
// customization point table_traits<Type>, which the container serial layer
// declares (core/container/serial/table_render.hpp, for one). Its primary
// template, and the fold's entry point, are that layer's to spell, so the
// specialization that binds a database_table to relational_table_view is
// compiled only when the layer announces itself, defining
// DJINTERP_HAVE_CONTAINER_TABLE_RENDER; until then this header stands on
// its own.

#if defined(DJINTERP_HAVE_CONTAINER_TABLE_RENDER)

// 4.1
// table_traits
//   struct template: a database_table's binding to the fold -- renderable,
// through relational_table_view.
template<typename Table>
struct table_traits<Table,
                    std::enable_if_t<is_database_table_v<Table>>>
{
    static constexpr bool is_renderable = true;

    using view_type = relational_table_view<Table>;

    // view -- the table, seen as the tabular template
    static view_type
    view(
        const Table& _table
    ) noexcept
    {
        return view_type(_table);
    }
};

#endif  // DJINTERP_HAVE_CONTAINER_TABLE_RENDER


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_DATABASE_TABLE_RENDER_HPP
