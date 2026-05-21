/******************************************************************************
* djinterp [core]                                     database_table_traits.hpp
*
* djinterp database-table-traits module:
*   SFINAE-based classification of database-backed table-shaped types —
* the `database_table<>` class from database_table.hpp and any user-
* defined type exposing the same surface. Builds on the in-memory
* `is_table<>` predicate from `table_traits.hpp`: a database table IS
* a table, with additional capabilities for connection management,
* schema introspection, and synchronization with a backing store.
*
*   No base class, no tag, no registration: if the structural detectors
* in this header pick up your type's database-flavoured members, the
* trait system classifies it accordingly.
*
*   STRUCTURE
*   =========
*     I.   NESTED TYPE-ALIAS DETECTION
*            has_connection_type, has_schema_type
*
*     II.  SCHEMA METHOD DETECTION
*            has_get_schema_method, has_column_name_method,
*            has_column_count_method, has_column_type_method,
*            has_cell_by_name_method
*
*     III. SYNC METHOD DETECTION
*            has_refresh_method, has_commit_method,
*            has_fetch_schema_method, has_is_dirty_method,
*            has_is_stale_method, has_invalidate_method,
*            has_last_refresh_method
*
*     IV.  CONNECTION METHOD DETECTION
*            has_get_connection_method, has_set_connection_method,
*            has_is_connected_method
*
*     V.   VIEW / KIND METHOD DETECTION
*            has_is_view_method, has_kind_method, has_table_name_method
*
*     VI.  QUERY-CONFIG METHOD DETECTION
*            has_set_where_method, has_set_order_method,
*            has_set_limit_method, has_set_offset_method,
*            has_exists_method, has_row_count_remote_method
*
*     VII. SYNC-CONFIG METHOD DETECTION
*            has_get_sync_config_method, has_set_sync_config_method
*
*     VIII.AGGREGATE CLASSIFICATION PREDICATES
*            is_database_table, is_in_memory_table,
*            is_view_table, has_persistent_storage,
*            has_sync_capability, has_dirty_tracking,
*            has_stale_tracking, has_schema, has_named_columns,
*            supports_query_config, has_remote_count_capability
*
*     IX.  CLASSIFICATION ENUM
*            table_storage_kind
*
*     X.   AGGREGATE CLASSIFICATION STRUCT
*            database_table_class<_Type>
*
*   ORTHOGONALITY
*   =============
*   This header layers on top of `table_traits.hpp` — every aggregate
* here requires `is_table<>` from that module to hold before checking
* the database-specific overlays. The two trait sets compose:
*   - A pure in-memory table:  `is_table && !is_database_table`
*   - A database-backed table: `is_table &&  is_database_table`
*   - A view of either kind:   add `is_view_table` on top.
*
*   PORTABILITY
*   ===========
*     version: C++11 or higher; `_v` companions C++14+.
*     dependencies:
*       - djinterp.hpp              : NS_DJINTERP, clean_t
*       - core/meta/type_traits.hpp : void_t
*       - core/table_traits.hpp     : is_table base predicate
*       - env_cpp_features.h        : feature-detection macros
*
*
* path:      /inc/djinterp/core/db/database_table_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_DATABASE_TABLE_TRAITS_
#define DJINTERP_DATABASE_TABLE_TRAITS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "../table_traits.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   NESTED TYPE-ALIAS DETECTION
    // =========================================================================
    //   Detects the database-specific nested aliases — the signature
    // members identifying types backed by an external store.

    // has_connection_type
    //   trait: detects a `connection_type` nested alias on `_Type`.
    // The signature member identifying database-backed tables.
    template<typename _Type,
             typename = void>
    struct has_connection_type : std::false_type
    {};

    template<typename _Type>
    struct has_connection_type<_Type,
        void_t<typename _Type::connection_type>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_connection_type_v =
            has_connection_type<_Type>::value;
    #endif

    // has_schema_type
    //   trait: detects a `schema_type` nested alias on `_Type`. The
    // signature member identifying tables that carry explicit column
    // metadata (typical for database tables and other schema-bound
    // table kinds).
    template<typename _Type,
             typename = void>
    struct has_schema_type : std::false_type
    {};

    template<typename _Type>
    struct has_schema_type<_Type,
        void_t<typename _Type::schema_type>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_schema_type_v =
            has_schema_type<_Type>::value;
    #endif


    // =========================================================================
    // II.  SCHEMA METHOD DETECTION
    // =========================================================================
    //   Detects the schema-introspection surface — schema fetch,
    // column-by-name access, column-metadata accessors.

    // has_get_schema_method
    //   trait: detects a `get_schema()` method.
    template<typename _Type,
             typename = void>
    struct has_get_schema_method : std::false_type
    {};

    template<typename _Type>
    struct has_get_schema_method<_Type,
        void_t<decltype(std::declval<const _Type&>().get_schema())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_get_schema_method_v =
            has_get_schema_method<_Type>::value;
    #endif

    // has_column_name_method
    //   trait: detects a `column_name(size_t)` method.
    template<typename _Type,
             typename = void>
    struct has_column_name_method : std::false_type
    {};

    template<typename _Type>
    struct has_column_name_method<_Type,
        void_t<decltype(std::declval<const _Type&>().column_name(
            std::declval<std::size_t>()))>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_column_name_method_v =
            has_column_name_method<_Type>::value;
    #endif

    // has_column_count_method
    //   trait: detects a `column_count()` method.
    template<typename _Type,
             typename = void>
    struct has_column_count_method : std::false_type
    {};

    template<typename _Type>
    struct has_column_count_method<_Type,
        void_t<decltype(std::declval<const _Type&>().column_count())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_column_count_method_v =
            has_column_count_method<_Type>::value;
    #endif

    // has_column_type_method
    //   trait: detects a `column_type(size_t)` method.
    template<typename _Type,
             typename = void>
    struct has_column_type_method : std::false_type
    {};

    template<typename _Type>
    struct has_column_type_method<_Type,
        void_t<decltype(std::declval<const _Type&>().column_type(
            std::declval<std::size_t>()))>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_column_type_method_v =
            has_column_type_method<_Type>::value;
    #endif

    // has_cell_by_name_method
    //   trait: detects a `cell_by_name(size_t, string)` method — the
    // signature of tables with schema-based named-column access.
    template<typename _Type,
             typename = void>
    struct has_cell_by_name_method : std::false_type
    {};

    template<typename _Type>
    struct has_cell_by_name_method<_Type,
        void_t<decltype(std::declval<const _Type&>().cell_by_name(
            std::declval<std::size_t>(),
            std::declval<const std::string&>()))>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_cell_by_name_method_v =
            has_cell_by_name_method<_Type>::value;
    #endif


    // =========================================================================
    // III. SYNC METHOD DETECTION
    // =========================================================================
    //   Detects the synchronization surface — refresh from store, commit
    // to store, dirty/stale state tracking, explicit invalidation.

    // has_refresh_method
    //   trait: detects a `refresh()` method.
    template<typename _Type,
             typename = void>
    struct has_refresh_method : std::false_type
    {};

    template<typename _Type>
    struct has_refresh_method<_Type,
        void_t<decltype(std::declval<_Type&>().refresh())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_refresh_method_v =
            has_refresh_method<_Type>::value;
    #endif

    // has_commit_method
    //   trait: detects a `commit()` method.
    template<typename _Type,
             typename = void>
    struct has_commit_method : std::false_type
    {};

    template<typename _Type>
    struct has_commit_method<_Type,
        void_t<decltype(std::declval<_Type&>().commit())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_commit_method_v =
            has_commit_method<_Type>::value;
    #endif

    // has_fetch_schema_method
    //   trait: detects a `fetch_schema()` method.
    template<typename _Type,
             typename = void>
    struct has_fetch_schema_method : std::false_type
    {};

    template<typename _Type>
    struct has_fetch_schema_method<_Type,
        void_t<decltype(std::declval<_Type&>().fetch_schema())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_fetch_schema_method_v =
            has_fetch_schema_method<_Type>::value;
    #endif

    // has_is_dirty_method
    //   trait: detects an `is_dirty()` method.
    template<typename _Type,
             typename = void>
    struct has_is_dirty_method : std::false_type
    {};

    template<typename _Type>
    struct has_is_dirty_method<_Type,
        void_t<decltype(std::declval<const _Type&>().is_dirty())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_is_dirty_method_v =
            has_is_dirty_method<_Type>::value;
    #endif

    // has_is_stale_method
    //   trait: detects an `is_stale()` method.
    template<typename _Type,
             typename = void>
    struct has_is_stale_method : std::false_type
    {};

    template<typename _Type>
    struct has_is_stale_method<_Type,
        void_t<decltype(std::declval<const _Type&>().is_stale())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_is_stale_method_v =
            has_is_stale_method<_Type>::value;
    #endif

    // has_invalidate_method
    //   trait: detects an `invalidate()` method.
    template<typename _Type,
             typename = void>
    struct has_invalidate_method : std::false_type
    {};

    template<typename _Type>
    struct has_invalidate_method<_Type,
        void_t<decltype(std::declval<_Type&>().invalidate())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_invalidate_method_v =
            has_invalidate_method<_Type>::value;
    #endif

    // has_last_refresh_method
    //   trait: detects a `last_refresh()` method (typically returning
    // a time_point).
    template<typename _Type,
             typename = void>
    struct has_last_refresh_method : std::false_type
    {};

    template<typename _Type>
    struct has_last_refresh_method<_Type,
        void_t<decltype(std::declval<const _Type&>().last_refresh())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_last_refresh_method_v =
            has_last_refresh_method<_Type>::value;
    #endif


    // =========================================================================
    // IV.  CONNECTION METHOD DETECTION
    // =========================================================================
    //   Detects connection-handle accessors and live-state probing.

    // has_get_connection_method
    //   trait: detects a `get_connection()` method.
    template<typename _Type,
             typename = void>
    struct has_get_connection_method : std::false_type
    {};

    template<typename _Type>
    struct has_get_connection_method<_Type,
        void_t<decltype(std::declval<const _Type&>().get_connection())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_get_connection_method_v =
            has_get_connection_method<_Type>::value;
    #endif

    // has_set_connection_method
    //   trait: detects a `set_connection(...)` method.
    template<typename _Type,
             typename = void>
    struct has_set_connection_method : std::false_type
    {};

    template<typename _Type>
    struct has_set_connection_method<_Type,
        void_t<decltype(&_Type::set_connection)>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_set_connection_method_v =
            has_set_connection_method<_Type>::value;
    #endif

    // has_is_connected_method
    //   trait: detects an `is_connected()` method.
    template<typename _Type,
             typename = void>
    struct has_is_connected_method : std::false_type
    {};

    template<typename _Type>
    struct has_is_connected_method<_Type,
        void_t<decltype(std::declval<const _Type&>().is_connected())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_is_connected_method_v =
            has_is_connected_method<_Type>::value;
    #endif


    // =========================================================================
    // V.   VIEW / KIND METHOD DETECTION
    // =========================================================================
    //   Detects table-flavour predicates — whether the table self-
    // identifies as a view, the `kind()` enumerator accessor, and the
    // database-side table name.

    // has_is_view_method
    //   trait: detects an `is_view()` method.
    template<typename _Type,
             typename = void>
    struct has_is_view_method : std::false_type
    {};

    template<typename _Type>
    struct has_is_view_method<_Type,
        void_t<decltype(std::declval<const _Type&>().is_view())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_is_view_method_v =
            has_is_view_method<_Type>::value;
    #endif

    // has_kind_method
    //   trait: detects a `kind()` method (typically returning a
    // `table_kind` or similar enum identifying the table's flavour).
    template<typename _Type,
             typename = void>
    struct has_kind_method : std::false_type
    {};

    template<typename _Type>
    struct has_kind_method<_Type,
        void_t<decltype(std::declval<const _Type&>().kind())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_kind_method_v =
            has_kind_method<_Type>::value;
    #endif

    // has_table_name_method
    //   trait: detects a `table_name()` method (returning the database-
    // side table identifier).
    template<typename _Type,
             typename = void>
    struct has_table_name_method : std::false_type
    {};

    template<typename _Type>
    struct has_table_name_method<_Type,
        void_t<decltype(std::declval<const _Type&>().table_name())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_table_name_method_v =
            has_table_name_method<_Type>::value;
    #endif


    // =========================================================================
    // VI.  QUERY-CONFIG METHOD DETECTION
    // =========================================================================
    //   Detects the per-refresh query configuration surface — WHERE,
    // ORDER BY, LIMIT, OFFSET filters, plus existence and remote-count
    // probes.

    // has_set_where_method
    //   trait: detects a `set_where(...)` method.
    template<typename _Type,
             typename = void>
    struct has_set_where_method : std::false_type
    {};

    template<typename _Type>
    struct has_set_where_method<_Type,
        void_t<decltype(&_Type::set_where)>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_set_where_method_v =
            has_set_where_method<_Type>::value;
    #endif

    // has_set_order_method
    //   trait: detects a `set_order(...)` method.
    template<typename _Type,
             typename = void>
    struct has_set_order_method : std::false_type
    {};

    template<typename _Type>
    struct has_set_order_method<_Type,
        void_t<decltype(&_Type::set_order)>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_set_order_method_v =
            has_set_order_method<_Type>::value;
    #endif

    // has_set_limit_method
    //   trait: detects a `set_limit(...)` method.
    template<typename _Type,
             typename = void>
    struct has_set_limit_method : std::false_type
    {};

    template<typename _Type>
    struct has_set_limit_method<_Type,
        void_t<decltype(&_Type::set_limit)>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_set_limit_method_v =
            has_set_limit_method<_Type>::value;
    #endif

    // has_set_offset_method
    //   trait: detects a `set_offset(...)` method.
    template<typename _Type,
             typename = void>
    struct has_set_offset_method : std::false_type
    {};

    template<typename _Type>
    struct has_set_offset_method<_Type,
        void_t<decltype(&_Type::set_offset)>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_set_offset_method_v =
            has_set_offset_method<_Type>::value;
    #endif

    // has_exists_method
    //   trait: detects an `exists()` method (probes the database for
    // the backing table's existence).
    template<typename _Type,
             typename = void>
    struct has_exists_method : std::false_type
    {};

    template<typename _Type>
    struct has_exists_method<_Type,
        void_t<decltype(std::declval<const _Type&>().exists())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_exists_method_v =
            has_exists_method<_Type>::value;
    #endif

    // has_row_count_remote_method
    //   trait: detects a `row_count_remote()` method (queries the
    // database for the row count without fetching rows).
    template<typename _Type,
             typename = void>
    struct has_row_count_remote_method : std::false_type
    {};

    template<typename _Type>
    struct has_row_count_remote_method<_Type,
        void_t<decltype(std::declval<const _Type&>().row_count_remote())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_row_count_remote_method_v =
            has_row_count_remote_method<_Type>::value;
    #endif


    // =========================================================================
    // VII. SYNC-CONFIG METHOD DETECTION
    // =========================================================================

    // has_get_sync_config_method
    //   trait: detects a `get_sync_config()` method.
    template<typename _Type,
             typename = void>
    struct has_get_sync_config_method : std::false_type
    {};

    template<typename _Type>
    struct has_get_sync_config_method<_Type,
        void_t<decltype(std::declval<const _Type&>().get_sync_config())>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_get_sync_config_method_v =
            has_get_sync_config_method<_Type>::value;
    #endif

    // has_set_sync_config_method
    //   trait: detects a `set_sync_config(...)` method.
    template<typename _Type,
             typename = void>
    struct has_set_sync_config_method : std::false_type
    {};

    template<typename _Type>
    struct has_set_sync_config_method<_Type,
        void_t<decltype(&_Type::set_sync_config)>>
        : std::true_type
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_set_sync_config_method_v =
            has_set_sync_config_method<_Type>::value;
    #endif


    // =========================================================================
    // VIII.AGGREGATE CLASSIFICATION PREDICATES
    // =========================================================================
    //   Higher-level predicates combining leaf detectors into single
    // classification answers. Every aggregate here layers on top of
    // `is_table<>` from `table_traits.hpp` — only types satisfying the
    // base table predicate are considered for the database-specific
    // overlay classifications.

    // is_database_table
    //   trait: a table additionally exposing the database surface —
    // `connection_type` alias, plus `refresh()` and `commit()`
    // methods. This identifies `database_table<>` and any user type
    // implementing the same surface.
    template<typename _Type>
    struct is_database_table
        : std::integral_constant<bool,
            (   is_table<_Type>::value
             && has_connection_type<clean_t<_Type>>::value
             && has_refresh_method<clean_t<_Type>>::value
             && has_commit_method<clean_t<_Type>>::value
            )>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool is_database_table_v =
            is_database_table<_Type>::value;
    #endif

    // is_in_memory_table
    //   trait: a table that is NOT a database table — the symmetric
    // complement. Pure in-memory storage with no external backing.
    template<typename _Type>
    struct is_in_memory_table
        : std::integral_constant<bool,
            (   is_table<_Type>::value
             && !is_database_table<_Type>::value
            )>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool is_in_memory_table_v =
            is_in_memory_table<_Type>::value;
    #endif

    // is_view_table
    //   trait: a database table self-identifying as a view via an
    // `is_view()` predicate method. The predicate's runtime return
    // is the type's concern; here we only detect that the type
    // advertises view-aware behaviour.
    template<typename _Type>
    struct is_view_table
        : std::integral_constant<bool,
            (   is_database_table<_Type>::value
             && has_is_view_method<clean_t<_Type>>::value
            )>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool is_view_table_v =
            is_view_table<_Type>::value;
    #endif

    // has_persistent_storage
    //   trait: the table is backed by external persistent storage,
    // detected via the connection-type alias.
    template<typename _Type>
    struct has_persistent_storage
        : std::integral_constant<bool,
            (   is_table<_Type>::value
             && has_connection_type<clean_t<_Type>>::value
            )>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_persistent_storage_v =
            has_persistent_storage<_Type>::value;
    #endif

    // has_sync_capability
    //   trait: the table can synchronize with an external store —
    // both `refresh()` and `commit()` are present.
    template<typename _Type>
    struct has_sync_capability
        : std::integral_constant<bool,
            (   has_refresh_method<clean_t<_Type>>::value
             && has_commit_method<clean_t<_Type>>::value
            )>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_sync_capability_v =
            has_sync_capability<_Type>::value;
    #endif

    // has_dirty_tracking
    //   trait: the table tracks pending uncommitted modifications —
    // `is_dirty()` is exposed.
    template<typename _Type>
    struct has_dirty_tracking
        : has_is_dirty_method<clean_t<_Type>>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_dirty_tracking_v =
            has_dirty_tracking<_Type>::value;
    #endif

    // has_stale_tracking
    //   trait: the table tracks cache freshness — `is_stale()` is
    // exposed.
    template<typename _Type>
    struct has_stale_tracking
        : has_is_stale_method<clean_t<_Type>>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_stale_tracking_v =
            has_stale_tracking<_Type>::value;
    #endif

    // has_schema
    //   trait: the table carries an explicit schema (column-info
    // metadata), detected via the `schema_type` alias or the
    // `get_schema()` method.
    template<typename _Type>
    struct has_schema
        : std::integral_constant<bool,
            (   has_schema_type<clean_t<_Type>>::value
             || has_get_schema_method<clean_t<_Type>>::value
            )>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_schema_v =
            has_schema<_Type>::value;
    #endif

    // has_named_columns
    //   trait: the table exposes name-keyed cell access via
    // `cell_by_name(size_t, string)`.
    template<typename _Type>
    struct has_named_columns
        : has_cell_by_name_method<clean_t<_Type>>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_named_columns_v =
            has_named_columns<_Type>::value;
    #endif

    // supports_query_config
    //   trait: the table exposes the full per-refresh query
    // configuration surface — WHERE, ORDER BY, LIMIT, OFFSET.
    template<typename _Type>
    struct supports_query_config
        : std::integral_constant<bool,
            (   has_set_where_method<clean_t<_Type>>::value
             && has_set_order_method<clean_t<_Type>>::value
             && has_set_limit_method<clean_t<_Type>>::value
             && has_set_offset_method<clean_t<_Type>>::value
            )>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool supports_query_config_v =
            supports_query_config<_Type>::value;
    #endif

    // has_remote_count_capability
    //   trait: the table can probe the backing store for a row count
    // without fetching rows.
    template<typename _Type>
    struct has_remote_count_capability
        : has_row_count_remote_method<clean_t<_Type>>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_remote_count_capability_v =
            has_remote_count_capability<_Type>::value;
    #endif

    // has_sync_config
    //   trait: the table exposes a sync configuration accessor
    // (`get_sync_config()`).
    template<typename _Type>
    struct has_sync_config
        : has_get_sync_config_method<clean_t<_Type>>
    {};

    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        template<typename _Type>
        inline constexpr bool has_sync_config_v =
            has_sync_config<_Type>::value;
    #endif


    // =========================================================================
    // IX.  CLASSIFICATION ENUM
    // =========================================================================

    // table_storage_kind
    //   enum: primary storage classification — "where does the data
    // live?". The high-level axis distinguishing memory-resident
    // tables from externally-backed ones, with a view subcategory
    // for read-only projections.
    enum class table_storage_kind
    {
        none,      // not a table at all
        in_memory, // owns its data, no external backing
        database,  // backed by a database connection (read/write)
        view       // database view — read-only projection
    };


    // =========================================================================
    // X.   AGGREGATE CLASSIFICATION STRUCT
    // =========================================================================
    //   `database_table_class<_Type>` is the one-stop classification
    // snapshot for database-side concerns. Mirrors `table_class<T>`
    // in `table_traits.hpp` and is meant to be used alongside it — the
    // two trait structs partition the table classification space into
    // table-mechanics (in `table_class`) and database-surface (here).

    template<typename _Type>
    struct database_table_class
    {
    private:
        using clean_type = clean_t<_Type>;

    public:
        // -----------------------------------------------------------------
        //  primary classification
        // -----------------------------------------------------------------
        static constexpr bool is_table             =
            djinterp::is_table<clean_type>::value;
        static constexpr bool is_database_table    =
            djinterp::is_database_table<clean_type>::value;
        static constexpr bool is_in_memory_table   =
            djinterp::is_in_memory_table<clean_type>::value;
        static constexpr bool is_view_table        =
            djinterp::is_view_table<clean_type>::value;

        // -----------------------------------------------------------------
        //  connection / persistence
        // -----------------------------------------------------------------
        static constexpr bool has_connection_type  =
            djinterp::has_connection_type<clean_type>::value;
        static constexpr bool has_persistent_storage =
            djinterp::has_persistent_storage<clean_type>::value;
        static constexpr bool has_get_connection   =
            djinterp::has_get_connection_method<clean_type>::value;
        static constexpr bool has_set_connection   =
            djinterp::has_set_connection_method<clean_type>::value;
        static constexpr bool has_is_connected     =
            djinterp::has_is_connected_method<clean_type>::value;

        // -----------------------------------------------------------------
        //  schema / named-column access
        // -----------------------------------------------------------------
        static constexpr bool has_schema_type      =
            djinterp::has_schema_type<clean_type>::value;
        static constexpr bool has_schema           =
            djinterp::has_schema<clean_type>::value;
        static constexpr bool has_get_schema       =
            djinterp::has_get_schema_method<clean_type>::value;
        static constexpr bool has_column_name      =
            djinterp::has_column_name_method<clean_type>::value;
        static constexpr bool has_column_count     =
            djinterp::has_column_count_method<clean_type>::value;
        static constexpr bool has_column_type      =
            djinterp::has_column_type_method<clean_type>::value;
        static constexpr bool has_named_columns    =
            djinterp::has_named_columns<clean_type>::value;

        // -----------------------------------------------------------------
        //  sync surface
        // -----------------------------------------------------------------
        static constexpr bool has_sync_capability  =
            djinterp::has_sync_capability<clean_type>::value;
        static constexpr bool has_refresh          =
            djinterp::has_refresh_method<clean_type>::value;
        static constexpr bool has_commit           =
            djinterp::has_commit_method<clean_type>::value;
        static constexpr bool has_fetch_schema     =
            djinterp::has_fetch_schema_method<clean_type>::value;
        static constexpr bool has_invalidate       =
            djinterp::has_invalidate_method<clean_type>::value;
        static constexpr bool has_dirty_tracking   =
            djinterp::has_dirty_tracking<clean_type>::value;
        static constexpr bool has_stale_tracking   =
            djinterp::has_stale_tracking<clean_type>::value;
        static constexpr bool has_last_refresh     =
            djinterp::has_last_refresh_method<clean_type>::value;

        // -----------------------------------------------------------------
        //  view / kind metadata
        // -----------------------------------------------------------------
        static constexpr bool has_is_view          =
            djinterp::has_is_view_method<clean_type>::value;
        static constexpr bool has_kind             =
            djinterp::has_kind_method<clean_type>::value;
        static constexpr bool has_table_name       =
            djinterp::has_table_name_method<clean_type>::value;

        // -----------------------------------------------------------------
        //  query configuration
        // -----------------------------------------------------------------
        static constexpr bool supports_query_config =
            djinterp::supports_query_config<clean_type>::value;
        static constexpr bool has_set_where        =
            djinterp::has_set_where_method<clean_type>::value;
        static constexpr bool has_set_order        =
            djinterp::has_set_order_method<clean_type>::value;
        static constexpr bool has_set_limit        =
            djinterp::has_set_limit_method<clean_type>::value;
        static constexpr bool has_set_offset       =
            djinterp::has_set_offset_method<clean_type>::value;
        static constexpr bool has_exists           =
            djinterp::has_exists_method<clean_type>::value;
        static constexpr bool has_remote_count_capability =
            djinterp::has_remote_count_capability<clean_type>::value;

        // -----------------------------------------------------------------
        //  sync configuration
        // -----------------------------------------------------------------
        static constexpr bool has_sync_config      =
            djinterp::has_sync_config<clean_type>::value;
        static constexpr bool has_set_sync_config  =
            djinterp::has_set_sync_config_method<clean_type>::value;

        // -----------------------------------------------------------------
        //  enum classification (single-answer summary)
        // -----------------------------------------------------------------

        // storage_kind
        //   value: primary storage classification — "where does the
        // data live?".
        static constexpr table_storage_kind storage_kind =
            ( !is_table          ? table_storage_kind::none
            : is_view_table      ? table_storage_kind::view
            : is_database_table  ? table_storage_kind::database
            :                      table_storage_kind::in_memory );
    };


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_TABLE_TRAITS_
