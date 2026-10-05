/*******************************************************************************
* djinterp [core]                                      database_table_render.hpp
*   PROJECT:     djinterp / core / db
*
*   INCLUDE:     ../djinterp.hpp
*                ../table_common.hpp
*                ./database_table.hpp
*
*   SUMMARY:
*   Container-subframework bridge for the database target of the render
* fold. This is the "adequacy" half of database serialization: it presents a
* database_table<> (and therefore every vendor leaf that derives from it —
* sqlite_table, postgres_table, oracle_table, db2_table, mysql_table,
* mariadb_table, and the shared mysql_common_table base) to the tabular
* template signature
*
*       Theta = { table, row, cell, key, foreign-ref }
*
* described in containers.tex (sections "Database serializability" and "The
* table"). The container render fold is one catamorphism render[[alpha]] with
* three targets — bits (Serialization), text (Textual expressibility), and
* store (this file). The database target folds twice:
*
*       container  --enc_T-->  Theta (template)  --[[.]]_D-->  store
*
* This header supplies enc_T (container -> template): the *encoder* / adequacy
* witness. The per-vendor renderer [[.]]_D (template -> store: identifier
* quoting, type spelling, upsert shape) already lives on each connection
* (field_type_to_<vendor>_sql / <vendor>_connection::sql_type_name,
* quote_identifier(name, db_type), dialect_format_limit_offset(...)) and in
* the concrete database_table base, so coverage is factored, never M x N.
*
*   WHY AN ADAPTER AND NOT table_base INHERITANCE:
*   table_base<Derived, Type> is a CRTP mixin over a CONTIGUOUS, row-major,
* cell-homogeneous buffer (its hook is data() -> const Type*). A
* database_table stores std::vector<std::vector<value>> — non-contiguous and
* column-typed (tau_{r,c} = tau_c) — so it cannot satisfy the data() hook and
* must not inherit table_base. Instead it is made renderable structurally,
* through the read surface it already exposes (rows(), cols(), cell(r,c),
* get_schema()). This matches the theory: a relational table is a rank-2,
* column-typed, order-blind (at the relation level) projection of the general
* rank-k container table, so it plugs into the *tabular* template rather than
* the generic contiguous-buffer one.
*
*   INTEGRATION SEAM (one point — see NOTE at the bottom):
*   The container render fold selects the tabular encoder through the
* customization-point primary template `table_traits<T>` (declared by the
* container serial layer, e.g. core/container/serial/table_render.hpp). This
* header provides the *content* of that specialization —
* relational_table_view<T> exposes precisely { rows, cols, cell, column
* name/type, primary key, foreign refs } — but the exact spelling of the
* primary template and of the fold entry point lives upstream. The final
* one-line binding is gated on those upstream headers; it is written here
* behind DJINTERP_HAVE_CONTAINER_TABLE_RENDER so this file is self-contained
* and compilable on its own until they are wired in.
*
*   PORTABILITY:
*   Requires C++17 or later.
*
*
* path:      /inc/djinterp/core/db/database_table_render.hpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_DB_DATABASE_TABLE_RENDER_HPP
#define DJINTERP_DB_DATABASE_TABLE_RENDER_HPP

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "./database_table.hpp"

NS_DJINTERP

    // =====================================================================
    //  detection: is T a database_table<> (or a leaf derived from one)?
    // =====================================================================

    // is_database_table
    //   trait: true when T publicly derives from some
    // database_table<Connection, ValueType, Config>. Because every SQL
    // vendor leaf (and mysql_common_table) derives from the concrete
    // database_table base, a single detection covers the whole family — no
    // per-vendor specialization is required.
    namespace internal
    {
        // matches any database_table<...> specialization by deducing its
        // three template arguments from a pointer-to-base conversion.
        template<typename C, typename V, typename Cfg>
        std::true_type
        database_table_base_test(const database_table<C, V, Cfg>*);

        std::false_type database_table_base_test(...);

        template<typename Type>
        using is_database_table_impl =
            decltype(database_table_base_test(
                std::declval<std::remove_reference_t<Type>*>()));
    } // namespace internal

    template<typename Type>
    struct is_database_table
        : internal::is_database_table_impl<Type>
    {
    };

    template<typename Type>
    inline constexpr bool is_database_table_v =
        is_database_table<Type>::value;


    // =====================================================================
    //  relational_table_view: the Theta surface (enc_T)
    // =====================================================================

    // relational_table_view
    //   class template: a lightweight, non-owning view that presents a
    // database_table (or derived leaf) to the tabular template signature
    // Theta. It performs no copying: every accessor forwards to the live
    // table. The render fold consumes this view to emit { table, row, cell,
    // key, foreign-ref } nodes; the vendor renderer then spells each node
    // for its store.
    //
    //   The five Theta node families map as:
    //     table       -> table_name() + the (rows x cols) extent
    //     row         -> row index r in [0, rows())
    //     cell        -> cell(r, c) : const value&   (rank-2, tau_{r,c}=tau_c)
    //     key         -> primary_key_columns()  (position within the row)
    //     foreign-ref -> foreign_ref(c)         (target table + column)
    template<typename Table>
    class relational_table_view
    {
        static_assert(is_database_table_v<Table>,
            "relational_table_view requires a database_table<> (or a leaf "
            "deriving from one). Document / KV / wide-column stores are not "
            "relational tables and are handled by their own modules.");

    public:

        using table_type   = Table;
        using size_type    = std::size_t;
        using value_type   = typename Table::value_type;
        // column_info / table_schema are namespace-scope descriptors in
        // database.hpp / database_table.hpp (not nested in database_table).
        using schema_type  = table_schema;
        using column_type  = column_info;

        // ---- construction -------------------------------------------------

        explicit relational_table_view(const Table& _table) noexcept
            : m_table(&_table)
        {
        }

        // ---- table node ---------------------------------------------------

        // table_name
        //   function: the relation's name (the `table` node of Theta).
        const std::string& table_name() const noexcept
        {
            return m_table->get_schema().table_name;
        }

        // rows / cols
        //   function: the rank-2 extent. rows() is the current cardinality
        // of the local cache; cols() is the arity fixed by the schema.
        size_type rows() const noexcept { return m_table->rows(); }
        size_type cols() const noexcept { return m_table->cols(); }

        // ---- cell node ----------------------------------------------------

        // cell
        //   function: the (r, c) cell (the `cell` node of Theta). Returns a
        // const reference into the live table — no copy. Because the table
        // is column-typed, the dynamic alternative held by value_type at
        // column c is column_field_type(c) for every row.
        const value_type& cell(size_type _row, size_type _col) const
        {
            return m_table->cell(_row, _col);
        }

        // ---- column metadata (drives cell spelling + schema DDL) ----------

        // column_name / column_field_type
        //   function: the name and logical field_type of column c. The
        // vendor renderer turns column_field_type(c) into a store type via
        // its own mapping (field_type_to_<vendor>_sql /
        // connection::sql_type_name), so this view stays vendor-neutral.
        const std::string& column_name(size_type _col) const
        {
            return m_table->get_schema().columns[_col].name;
        }

        field_type column_field_type(size_type _col) const
        {
            return m_table->get_schema().columns[_col].type;
        }

        // ---- key node -----------------------------------------------------

        // is_primary_key_column
        //   function: whether column c participates in the primary key (the
        // `key` node of Theta). A relational table is order-blind at the
        // relation level, so sequence identity (=_seq) requires an explicit
        // position column at the encoder level; callers that need ordered
        // round-tripping add it before rendering.
        bool is_primary_key_column(size_type _col) const
        {
            return m_table->get_schema().columns[_col].is_primary_key;
        }

        const std::vector<std::string>& primary_key_columns() const noexcept
        {
            return m_table->get_schema().primary_key_columns;
        }

        // has_primary_key
        //   function: true when the relation declares a key (selects upsert
        // vs full-replace at the store level).
        bool has_primary_key() const noexcept
        {
            return !m_table->get_schema().primary_key_columns.empty();
        }

        // ---- foreign-ref node ---------------------------------------------

        // foreign_ref
        //   struct: an optional (target table, target column) reference for
        // column c (the `foreign-ref` node of Theta). engaged == false when
        // column c is not a foreign key.
        struct foreign_ref
        {
            bool        engaged = false;
            std::string target_table;
            std::string target_column;
        };

        foreign_ref foreign_ref_of(size_type _col) const
        {
            const column_type& ci = m_table->get_schema().columns[_col];

            foreign_ref fr;

            if (ci.foreign_table.has_value())
            {
                fr.engaged       = true;
                fr.target_table  = ci.foreign_table.value();
                fr.target_column = ci.foreign_column.value_or(std::string{});
            }

            return fr;
        }

        // ---- direct access to the underlying table ------------------------

        const Table& table() const noexcept { return *m_table; }

    private:

        const Table* m_table;
    };


    // make_relational_table_view
    //   function: convenience factory (class-template argument deduction
    // keeps call sites terse: auto v = make_relational_table_view(my_table);).
    template<typename Table>
    relational_table_view<Table>
    make_relational_table_view(const Table& _table) noexcept
    {
        return relational_table_view<Table>(_table);
    }


    // =====================================================================
    //  store_D / retrieve_D  (definitions of the database.hpp declarations)
    // =====================================================================

    // store_D
    //   function template: the "container -> Theta -> store" direction. When
    // RenderableTable is already a database_table<Connection, ...> bound to
    // the store, rendering is exactly its transactional write path (commit),
    // which spells every Theta node through the connection's dialect. The
    // foreign-container arm — encode an arbitrary table-shaped *container*
    // into a fresh database_table via relational_table_view + a schema
    // synthesised from column_field_type(c) — is the enc_T witness and is
    // gated on the upstream container serial layer (see the NOTE below).
    template<typename Connection, typename RenderableTable>
    void store_D(Connection&            _connection,
                 const RenderableTable& _table)
    {
        if constexpr (is_database_table_v<RenderableTable>)
        {
            // the table already knows its connection and how to spell its
            // own Theta nodes; committing renders it into the store.
            (void) _connection;
            const_cast<RenderableTable&>(_table).commit();
        }
        else
        {
            static_assert(is_database_table_v<RenderableTable>,
                "store_D for a non-database_table container requires the "
                "container serial layer (enc_T). Wrap the container in a "
                "relational_table_view and enable "
                "DJINTERP_HAVE_CONTAINER_TABLE_RENDER, or hand store_D a "
                "database_table<> bound to this connection.");
        }
    }

    // retrieve_D
    //   function template: the adjoint "store -> Theta -> container"
    // direction. For a database_table<> bound to the store this is refresh().
    template<typename Connection, typename RenderableTable>
    void retrieve_D(Connection&      _connection,
                    RenderableTable& _table)
    {
        if constexpr (is_database_table_v<RenderableTable>)
        {
            (void) _connection;
            _table.refresh();
        }
        else
        {
            static_assert(is_database_table_v<RenderableTable>,
                "retrieve_D for a non-database_table container requires the "
                "container serial layer (the decode adjoint of enc_T).");
        }
    }


    // =====================================================================
    //  container render-fold binding (gated on the upstream serial layer)
    // =====================================================================
    //
    //   NOTE (integration seam): the container render fold picks the tabular
    // encoder via the customization-point primary template table_traits<T>
    // (declared in the container serial layer). Once that header is on the
    // include path the binding is a single specialization forwarding to the
    // view above, for example:
    //
    //       template<typename Table>
    //       struct table_traits<
    //               Table,
    //               std::enable_if_t<is_database_table_v<Table>>>
    //       {
    //           static constexpr bool is_renderable = true;
    //
    //           using view_type = relational_table_view<Table>;
    //
    //           static view_type view(const Table& _t) noexcept
    //           {
    //               return view_type(_t);
    //           }
    //       };
    //
    //   The exact primary-template name and the fold entry point
    // (render[[Theta]] / the database target of encode_into) must match the
    // upstream declarations, so this binding is compiled only when the
    // container serial layer announces itself:

#if defined(DJINTERP_HAVE_CONTAINER_TABLE_RENDER)

    template<typename Table>
    struct table_traits<
            Table,
            std::enable_if_t<is_database_table_v<Table>>>
    {
        static constexpr bool is_renderable = true;

        using view_type = relational_table_view<Table>;

        static view_type view(const Table& _t) noexcept
        {
            return view_type(_t);
        }
    };

#endif // DJINTERP_HAVE_CONTAINER_TABLE_RENDER

NS_END

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif // DJINTERP_DB_DATABASE_TABLE_RENDER_HPP
