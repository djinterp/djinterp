/******************************************************************************
* djinterp [core]                                   database_table_concepts.hpp
*
* djinterp database-table-concepts module:
*   C++20 concepts layered on top of `database_table_traits.hpp`. These
* concepts provide readable `requires` constraints for database-backed
* table-shaped types — the `database_table<>` class from
* database_table.hpp and any user-defined type exposing the same
* surface.
*
*   This header is intentionally thin: it does not re-implement
* detection. Each concept forwards to the corresponding SFINAE trait
* from `database_table_traits.hpp`, with `clean_t<_Type>` applied at
* the boundary so concepts work transparently across cv-ref-qualified
* inputs.
*
*   SCOPE
*   =====
*   Database-side concerns only — connection management, schema
* introspection, sync surface, query configuration. In-memory table
* concepts live in `table_concepts.hpp`. Database tables are also
* in-memory tables; the two concept sets compose:
*
*     // a generic algorithm constrained to database tables:
*     template<database_table _T> void refresh_and_compute(_T&);
*
*     // a more specific constraint — database tables with named columns:
*     template<typename _T>
*         requires (database_table<_T> && named_column_table<_T>)
*     void lookup_by_column_name(_T&, std::string_view);
*
*   CONTENTS
*   ========
*     I.   FEATURE GATE
*     II.  CORE STORAGE-KIND CONCEPTS
*            database_table, in_memory_table, view_table
*     III. CONNECTION CONCEPTS
*            connected_table, persistent_table,
*            connection_settable_table, connection_probe_table
*     IV.  SCHEMA CONCEPTS
*            schema_bearing_table, schema_fetchable_table,
*            named_column_table, column_introspectable_table
*     V.   SYNC CONCEPTS
*            sync_capable_table, refreshable_table, commitable_table,
*            dirty_trackable_table, stale_trackable_table,
*            invalidatable_table
*     VI.  QUERY-CONFIG CONCEPTS
*            query_configurable_table, where_filterable_table,
*            order_settable_table, paginated_table,
*            existence_probable_table, remote_countable_table
*     VII. SYNC-CONFIG CONCEPTS
*            sync_configurable_table
*     VIII. AGGREGATE CONCEPTS
*            full_database_table
*
*   PORTABILITY
*   ===========
*     version: requires C++20 (concepts).
*     dependencies:
*       - djinterp.hpp                       : NS_DJINTERP, clean_t
*       - core/db/database_table_traits.hpp  : underlying SFINAE traits
*       - core/table_concepts.hpp            : `table` base concept
*
*
* path:      /inc/djinterp/core/db/database_table_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_DATABASE_TABLE_CONCEPTS_
#define DJINTERP_DATABASE_TABLE_CONCEPTS_ 1


// =============================================================================
// I.   FEATURE GATE
// =============================================================================

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "database_table_concepts.hpp requires C++20 concepts support."
#endif

// djinterp
#include "../../djinterp.hpp"
#include "../table_concepts.hpp"
#include "./database_table_traits.hpp"


NS_DJINTERP


    // =========================================================================
    // II.  CORE STORAGE-KIND CONCEPTS
    // =========================================================================

    // database_table
    //   concept: constrains tables exposing the database surface —
    // `connection_type` alias, plus `refresh()` and `commit()` methods.
    // Identifies `database_table<>` instantiations and structurally
    // compatible user types.
    template<typename _Type>
    concept database_table =
        is_database_table<clean_t<_Type>>::value;

    // in_memory_table
    //   concept: constrains tables that are NOT database tables — pure
    // in-memory storage with no external backing. Symmetric complement
    // of `database_table`.
    template<typename _Type>
    concept in_memory_table =
        is_in_memory_table<clean_t<_Type>>::value;

    // view_table
    //   concept: constrains database tables self-identifying as views
    // via an `is_view()` predicate method.
    template<typename _Type>
    concept view_table =
        is_view_table<clean_t<_Type>>::value;


    // =========================================================================
    // III. CONNECTION CONCEPTS
    // =========================================================================

    // connected_table
    //   concept: constrains tables exposing a `get_connection()`
    // accessor.
    template<typename _Type>
    concept connected_table =
        ( database_table<_Type> &&
          has_get_connection_method<clean_t<_Type>>::value );

    // persistent_table
    //   concept: constrains tables backed by external persistent
    // storage (detected via `connection_type` alias).
    template<typename _Type>
    concept persistent_table =
        has_persistent_storage<clean_t<_Type>>::value;

    // connection_settable_table
    //   concept: constrains database tables exposing
    // `set_connection(...)`.
    template<typename _Type>
    concept connection_settable_table =
        ( database_table<_Type> &&
          has_set_connection_method<clean_t<_Type>>::value );

    // connection_probe_table
    //   concept: constrains database tables exposing `is_connected()`.
    template<typename _Type>
    concept connection_probe_table =
        ( database_table<_Type> &&
          has_is_connected_method<clean_t<_Type>>::value );


    // =========================================================================
    // IV.  SCHEMA CONCEPTS
    // =========================================================================

    // schema_bearing_table
    //   concept: constrains tables carrying explicit schema metadata —
    // either a `schema_type` alias or a `get_schema()` method.
    template<typename _Type>
    concept schema_bearing_table =
        has_schema<clean_t<_Type>>::value;

    // schema_fetchable_table
    //   concept: constrains database tables exposing `fetch_schema()` —
    // can pull schema from the backing store.
    template<typename _Type>
    concept schema_fetchable_table =
        ( database_table<_Type> &&
          has_fetch_schema_method<clean_t<_Type>>::value );

    // named_column_table
    //   concept: constrains tables exposing
    // `cell_by_name(row, string_view)`. Database tables with schema-
    // backed named-column access.
    template<typename _Type>
    concept named_column_table =
        has_named_columns<clean_t<_Type>>::value;

    // column_introspectable_table
    //   concept: constrains tables exposing the full column-
    // introspection surface — `column_name(i)`, `column_count()`, and
    // `column_type(i)`.
    template<typename _Type>
    concept column_introspectable_table =
        (   has_column_name_method<clean_t<_Type>>::value
         && has_column_count_method<clean_t<_Type>>::value
         && has_column_type_method<clean_t<_Type>>::value );


    // =========================================================================
    // V.   SYNC CONCEPTS
    // =========================================================================

    // sync_capable_table
    //   concept: constrains tables exposing both `refresh()` and
    // `commit()`.
    template<typename _Type>
    concept sync_capable_table =
        has_sync_capability<clean_t<_Type>>::value;

    // refreshable_table
    //   concept: constrains tables exposing `refresh()`.
    template<typename _Type>
    concept refreshable_table =
        has_refresh_method<clean_t<_Type>>::value;

    // commitable_table
    //   concept: constrains tables exposing `commit()`.
    template<typename _Type>
    concept commitable_table =
        has_commit_method<clean_t<_Type>>::value;

    // dirty_trackable_table
    //   concept: constrains tables exposing `is_dirty()` (uncommitted-
    // modification tracking).
    template<typename _Type>
    concept dirty_trackable_table =
        has_dirty_tracking<clean_t<_Type>>::value;

    // stale_trackable_table
    //   concept: constrains tables exposing `is_stale()` (cache-
    // freshness tracking).
    template<typename _Type>
    concept stale_trackable_table =
        has_stale_tracking<clean_t<_Type>>::value;

    // invalidatable_table
    //   concept: constrains tables exposing `invalidate()`.
    template<typename _Type>
    concept invalidatable_table =
        ( database_table<_Type> &&
          has_invalidate_method<clean_t<_Type>>::value );

    // refresh_timestamped_table
    //   concept: constrains tables exposing `last_refresh()` —
    // queryable refresh-time information.
    template<typename _Type>
    concept refresh_timestamped_table =
        ( database_table<_Type> &&
          has_last_refresh_method<clean_t<_Type>>::value );


    // =========================================================================
    // VI.  QUERY-CONFIG CONCEPTS
    // =========================================================================

    // query_configurable_table
    //   concept: constrains tables exposing the full per-refresh query
    // configuration surface — WHERE, ORDER BY, LIMIT, OFFSET.
    template<typename _Type>
    concept query_configurable_table =
        supports_query_config<clean_t<_Type>>::value;

    // where_filterable_table
    //   concept: constrains tables exposing `set_where(...)`.
    template<typename _Type>
    concept where_filterable_table =
        ( database_table<_Type> &&
          has_set_where_method<clean_t<_Type>>::value );

    // order_settable_table
    //   concept: constrains tables exposing `set_order(...)`.
    template<typename _Type>
    concept order_settable_table =
        ( database_table<_Type> &&
          has_set_order_method<clean_t<_Type>>::value );

    // paginated_table
    //   concept: constrains tables exposing both `set_limit(...)` and
    // `set_offset(...)`.
    template<typename _Type>
    concept paginated_table =
        (   database_table<_Type>
         && has_set_limit_method<clean_t<_Type>>::value
         && has_set_offset_method<clean_t<_Type>>::value );

    // existence_probable_table
    //   concept: constrains tables exposing `exists()` — probes the
    // backing store for the table's existence.
    template<typename _Type>
    concept existence_probable_table =
        ( database_table<_Type> &&
          has_exists_method<clean_t<_Type>>::value );

    // remote_countable_table
    //   concept: constrains tables exposing `row_count_remote()` —
    // queries the backing store for a row count without fetching rows.
    template<typename _Type>
    concept remote_countable_table =
        has_remote_count_capability<clean_t<_Type>>::value;


    // =========================================================================
    // VII. SYNC-CONFIG CONCEPTS
    // =========================================================================

    // sync_configurable_table
    //   concept: constrains tables exposing `get_sync_config()`.
    template<typename _Type>
    concept sync_configurable_table =
        has_sync_config<clean_t<_Type>>::value;

    // sync_config_settable_table
    //   concept: constrains tables exposing `set_sync_config(...)`.
    template<typename _Type>
    concept sync_config_settable_table =
        ( database_table<_Type> &&
          has_set_sync_config_method<clean_t<_Type>>::value );


    // =========================================================================
    // VIII.AGGREGATE CONCEPTS
    // =========================================================================

    // full_database_table
    //   concept: constrains tables exposing the full database-table
    // surface — schema, named-column access, sync capability,
    // dirty/stale tracking, query configuration. Useful as a
    // "feature-rich database table" constraint for algorithms that
    // exercise the entire api.
    template<typename _Type>
    concept full_database_table =
        (   database_table<_Type>
         && schema_bearing_table<_Type>
         && named_column_table<_Type>
         && sync_capable_table<_Type>
         && dirty_trackable_table<_Type>
         && stale_trackable_table<_Type>
         && query_configurable_table<_Type> );


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_TABLE_CONCEPTS_
