/******************************************************************************
* djinterp [meta]                                             table_traits.hpp
*
* djinterp table traits header:
*   Unified table traits module providing config-based and structural SFINAE
* traits for ALL table-like types in the djinterp framework — both
* fixed-dimension tables (table<T, Rows, Cols>) and dynamic database-backed
* tables (database_table<Connection>).
*
*   CONTENTS:
*     I.    Detection idiom (container-local)
*     II.   Member detection operations (config members, span/split/partition)
*     III.  Value extraction traits (get_header_rows, get_spans, etc.)
*     IV.   Feature detection traits (has_header_rows, is_config_type, etc.)
*     V.    Span traits (is_span_type, span_contains, is_merged_cell, etc.)
*     V.b   Split traits
*     V.c   Cell layout (combined merge + split classification)
*     V.d   Multi-header traits
*     V.e   Partition traits
*     V.f   Structural column/row merge and split traits
*     VI.   Empty config
*     VII.  Structural immutability detection
*     VIII. Table config classification (table_config_class)
*     IX.   Cell region classification (fixed dimensions)
*     X.    Dimension computation (fixed dimensions)
*
*   Sections I–VIII operate on _Config types and arbitrary types probed for
* structural member presence — nothing in those sections requires compile-time
* row/column counts.  Sections IX–X extend with traits that require fixed
* _Rows and _Cols (cell_position, table_dimensions).
*
*   Cell-level merge/split/partition ACCESSOR METHODS for the table class are
* provided separately in table_layout.hpp.
*
*   PORTABILITY:
*   Compatible with C++11 and later. Uses portable trait access patterns.
*
* path:      \inc\meta\table_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.06.20
******************************************************************************/

#ifndef DJINTERP_TABLE_TRAITS_
#define DJINTERP_TABLE_TRAITS_ 1

#include <cstddef>
#include <tuple>
#include <type_traits>
#include "..\..\djinterp.h"
#include "..\..\type_traits.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   DETECTION IDIOM
    // =========================================================================

    NS_INTERNAL

        // detector
        //   trait: primary template (failure case).
        template<typename    _Default,
                 typename    _AlwaysVoid,
                 template<typename...> typename _Op,
                 typename... _Args>
        struct detector
        {
            using value_t = std::false_type;
            using type    = _Default;
        };

        // detector
        //   trait: specialization (success case).
        template<typename    _Default,
                 template<typename...> typename _Op,
                 typename... _Args>
        struct detector<_Default, void_t<_Op<_Args...>>, _Op, _Args...>
        {
            using value_t = std::true_type;
            using type    = _Op<_Args...>;
        };

        // nonesuch
        //   type: placeholder for detection failures.
        struct nonesuch
        {
            nonesuch()                      = delete;
            ~nonesuch()                     = delete;
            nonesuch(const nonesuch&)       = delete;
            void operator=(const nonesuch&) = delete;
        };

        // is_detected
        //   trait: true if _Op<_Args...> is well-formed.
        template<template<typename...> typename _Op,
                 typename... _Args>
        using is_detected = typename detector<nonesuch, void, _Op, _Args...>::value_t;

        // detected_t
        //   type: _Op<_Args...> if well-formed, else nonesuch.
        template<template<typename...> typename _Op,
                 typename... _Args>
        using detected_t = typename detector<nonesuch, void, _Op, _Args...>::type;

        // detected_or_t
        //   type: _Op<_Args...> if well-formed, else _Default.
        template<typename    _Default,
                 template<typename...> typename _Op,
                 typename... _Args>
        using detected_or_t = typename detector<_Default, void, _Op, _Args...>::type;

    NS_END  // internal


    // =========================================================================
    // II.  MEMBER DETECTION OPERATIONS
    // =========================================================================

    NS_INTERNAL

        // -----------------------------------------------------------------
        //  config member detectors
        // -----------------------------------------------------------------
        template<typename _C> using detect_header_rows  = decltype(_C::header_rows);
        template<typename _C> using detect_header_cols  = decltype(_C::header_cols);
        template<typename _C> using detect_header_depth = decltype(_C::header_depth);
        template<typename _C> using detect_footer_rows  = decltype(_C::footer_rows);
        template<typename _C> using detect_footer_cols  = decltype(_C::footer_cols);
        template<typename _C> using detect_total_rows   = decltype(_C::total_rows);
        template<typename _C> using detect_total_cols   = decltype(_C::total_cols);

        template<typename _C> using detect_total_row_position = decltype(_C::total_row_position);
        template<typename _C> using detect_total_col_position = decltype(_C::total_col_position);

        template<typename _C> using detect_spans        = typename _C::spans;
        template<typename _C> using detect_splits       = typename _C::splits;
        template<typename _C> using detect_multi_header = typename _C::multi_header;
        template<typename _C> using detect_partitions   = typename _C::partitions;
        template<typename _C> using detect_col_merges   = typename _C::col_merges;
        template<typename _C> using detect_col_splits   = typename _C::col_splits;
        template<typename _C> using detect_row_merges   = typename _C::row_merges;
        template<typename _C> using detect_row_splits   = typename _C::row_splits;

        // -----------------------------------------------------------------
        //  span member detectors (for user-defined span types)
        // -----------------------------------------------------------------
        template<typename _S> using detect_span_row      = decltype(_S::row);
        template<typename _S> using detect_span_col      = decltype(_S::col);
        template<typename _S> using detect_span_row_span = decltype(_S::row_span);
        template<typename _S> using detect_span_col_span = decltype(_S::col_span);

        // -----------------------------------------------------------------
        //  split member detectors (for user-defined split types)
        // -----------------------------------------------------------------
        template<typename _S> using detect_split_row      = decltype(_S::row);
        template<typename _S> using detect_split_col      = decltype(_S::col);
        template<typename _S> using detect_split_sub_rows = decltype(_S::sub_rows);
        template<typename _S> using detect_split_sub_cols = decltype(_S::sub_cols);

        // -----------------------------------------------------------------
        //  multi-header entry member detectors
        // -----------------------------------------------------------------
        template<typename _E> using detect_mh_level    = decltype(_E::level);
        template<typename _E> using detect_mh_col      = decltype(_E::col);
        template<typename _E> using detect_mh_col_span = decltype(_E::col_span);

        // -----------------------------------------------------------------
        //  partition member detectors
        // -----------------------------------------------------------------
        template<typename _P> using detect_part_row_start = decltype(_P::row_start);
        template<typename _P> using detect_part_col_start = decltype(_P::col_start);
        template<typename _P> using detect_part_row_count = decltype(_P::row_count);
        template<typename _P> using detect_part_col_count = decltype(_P::col_count);

        // -----------------------------------------------------------------
        //  column merge member detectors
        // -----------------------------------------------------------------
        template<typename _M> using detect_col_merge_col   = decltype(_M::col);
        template<typename _M> using detect_col_merge_count = decltype(_M::count);

        // -----------------------------------------------------------------
        //  column split member detectors
        // -----------------------------------------------------------------
        template<typename _S> using detect_col_split_col      = decltype(_S::col);
        template<typename _S> using detect_col_split_sub_cols = decltype(_S::sub_cols);

        // -----------------------------------------------------------------
        //  row merge member detectors
        // -----------------------------------------------------------------
        template<typename _M> using detect_row_merge_row   = decltype(_M::row);
        template<typename _M> using detect_row_merge_count = decltype(_M::count);

        // -----------------------------------------------------------------
        //  row split member detectors
        // -----------------------------------------------------------------
        template<typename _S> using detect_row_split_row      = decltype(_S::row);
        template<typename _S> using detect_row_split_sub_rows = decltype(_S::sub_rows);

    NS_END  // internal


    // =========================================================================
    // III. VALUE EXTRACTION TRAITS
    // =========================================================================

    // get_header_rows
    //   trait: extracts header_rows from config, defaulting to 0.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_header_rows, _Config>::value>
    struct get_header_rows : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Config>
    struct get_header_rows<_Config, true>
        : std::integral_constant<std::size_t, _Config::header_rows>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr std::size_t get_header_rows_v = get_header_rows<_Config>::value;
#endif

    // get_header_cols
    //   trait: extracts header_cols from config, defaulting to 0.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_header_cols, _Config>::value>
    struct get_header_cols : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Config>
    struct get_header_cols<_Config, true>
        : std::integral_constant<std::size_t, _Config::header_cols>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr std::size_t get_header_cols_v = get_header_cols<_Config>::value;
#endif

    // get_header_depth
    //   trait: extracts header_depth from config. Defaults to header_rows
    // when not explicitly specified (each header row is its own level).
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_header_depth, _Config>::value>
    struct get_header_depth
        : std::integral_constant<std::size_t, get_header_rows<_Config>::value>
    {
    };

    template<typename _Config>
    struct get_header_depth<_Config, true>
        : std::integral_constant<std::size_t, _Config::header_depth>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr std::size_t get_header_depth_v = get_header_depth<_Config>::value;
#endif

    // get_footer_rows
    //   trait: extracts footer_rows from config, defaulting to 0.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_footer_rows, _Config>::value>
    struct get_footer_rows : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Config>
    struct get_footer_rows<_Config, true>
        : std::integral_constant<std::size_t, _Config::footer_rows>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr std::size_t get_footer_rows_v = get_footer_rows<_Config>::value;
#endif

    // get_footer_cols
    //   trait: extracts footer_cols from config, defaulting to 0.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_footer_cols, _Config>::value>
    struct get_footer_cols : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Config>
    struct get_footer_cols<_Config, true>
        : std::integral_constant<std::size_t, _Config::footer_cols>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr std::size_t get_footer_cols_v = get_footer_cols<_Config>::value;
#endif

    // get_total_rows
    //   trait: extracts total_rows from config, defaulting to 0.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_total_rows, _Config>::value>
    struct get_total_rows : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Config>
    struct get_total_rows<_Config, true>
        : std::integral_constant<std::size_t, _Config::total_rows>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr std::size_t get_total_rows_v = get_total_rows<_Config>::value;
#endif

    // get_total_cols
    //   trait: extracts total_cols from config, defaulting to 0.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_total_cols, _Config>::value>
    struct get_total_cols : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Config>
    struct get_total_cols<_Config, true>
        : std::integral_constant<std::size_t, _Config::total_cols>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr std::size_t get_total_cols_v = get_total_cols<_Config>::value;
#endif

    // get_spans
    //   trait: extracts spans tuple from config, defaulting to empty tuple.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_spans, _Config>::value>
    struct get_spans
    {
        using type = std::tuple<>;
    };

    template<typename _Config>
    struct get_spans<_Config, true>
    {
        using type = typename _Config::spans;
    };

    template<typename _Config>
    using get_spans_t = typename get_spans<_Config>::type;

    // get_splits
    //   trait: extracts splits tuple from config, defaulting to empty tuple.
    // Each element of the tuple is a user-defined split descriptor with
    // row/col/sub_rows/sub_cols members describing logical cell subdivision.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_splits, _Config>::value>
    struct get_splits
    {
        using type = std::tuple<>;
    };

    template<typename _Config>
    struct get_splits<_Config, true>
    {
        using type = typename _Config::splits;
    };

    template<typename _Config>
    using get_splits_t = typename get_splits<_Config>::type;


    // -----------------------------------------------------------------
    //  total positioning
    // -----------------------------------------------------------------

    // total_row_placement
    //   enum: identifies placement of total rows relative to the data
    // region. Values are integer-compatible for tagless config usage.
    enum class total_row_placement : std::size_t
    {
        after_data  = 0,
        before_data = 1
    };

    // total_col_placement
    //   enum: identifies placement of total columns relative to the
    // data region.
    enum class total_col_placement : std::size_t
    {
        after_data  = 0,
        before_data = 1
    };

    // get_total_row_position
    //   trait: extracts total_row_position from config. Defaults to
    // after_data (0) when absent, placing totals between data and footer.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_total_row_position, _Config>::value>
    struct get_total_row_position
        : std::integral_constant<std::size_t,
            static_cast<std::size_t>(total_row_placement::after_data)>
    {
    };

    template<typename _Config>
    struct get_total_row_position<_Config, true>
        : std::integral_constant<std::size_t, _Config::total_row_position>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr std::size_t get_total_row_position_v =
        get_total_row_position<_Config>::value;
#endif

    // get_total_col_position
    //   trait: extracts total_col_position from config. Defaults to
    // after_data (0) when absent.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_total_col_position, _Config>::value>
    struct get_total_col_position
        : std::integral_constant<std::size_t,
            static_cast<std::size_t>(total_col_placement::after_data)>
    {
    };

    template<typename _Config>
    struct get_total_col_position<_Config, true>
        : std::integral_constant<std::size_t, _Config::total_col_position>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr std::size_t get_total_col_position_v =
        get_total_col_position<_Config>::value;
#endif


    // -----------------------------------------------------------------
    //  multi-header, partition, and structural merge/split extraction
    // -----------------------------------------------------------------

    // get_multi_header
    //   trait: extracts multi_header tuple from config, defaulting to
    // empty tuple. Each element is a header group descriptor with
    // level/col/col_span members.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_multi_header, _Config>::value>
    struct get_multi_header
    {
        using type = std::tuple<>;
    };

    template<typename _Config>
    struct get_multi_header<_Config, true>
    {
        using type = typename _Config::multi_header;
    };

    template<typename _Config>
    using get_multi_header_t = typename get_multi_header<_Config>::type;

    // get_partitions
    //   trait: extracts partitions tuple from config, defaulting to
    // empty tuple. Each element is a partition descriptor with
    // row_start/col_start/row_count/col_count members.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_partitions, _Config>::value>
    struct get_partitions
    {
        using type = std::tuple<>;
    };

    template<typename _Config>
    struct get_partitions<_Config, true>
    {
        using type = typename _Config::partitions;
    };

    template<typename _Config>
    using get_partitions_t = typename get_partitions<_Config>::type;

    // get_col_merges
    //   trait: extracts col_merges tuple from config, defaulting to
    // empty tuple. Each element has col/count members describing a
    // structural column merge.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_col_merges, _Config>::value>
    struct get_col_merges
    {
        using type = std::tuple<>;
    };

    template<typename _Config>
    struct get_col_merges<_Config, true>
    {
        using type = typename _Config::col_merges;
    };

    template<typename _Config>
    using get_col_merges_t = typename get_col_merges<_Config>::type;

    // get_col_splits
    //   trait: extracts col_splits tuple from config, defaulting to
    // empty tuple. Each element has col/sub_cols members describing a
    // structural column split.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_col_splits, _Config>::value>
    struct get_col_splits
    {
        using type = std::tuple<>;
    };

    template<typename _Config>
    struct get_col_splits<_Config, true>
    {
        using type = typename _Config::col_splits;
    };

    template<typename _Config>
    using get_col_splits_t = typename get_col_splits<_Config>::type;

    // get_row_merges
    //   trait: extracts row_merges tuple from config, defaulting to
    // empty tuple. Each element has row/count members.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_row_merges, _Config>::value>
    struct get_row_merges
    {
        using type = std::tuple<>;
    };

    template<typename _Config>
    struct get_row_merges<_Config, true>
    {
        using type = typename _Config::row_merges;
    };

    template<typename _Config>
    using get_row_merges_t = typename get_row_merges<_Config>::type;

    // get_row_splits
    //   trait: extracts row_splits tuple from config, defaulting to
    // empty tuple. Each element has row/sub_rows members.
    template<typename _Config,
             bool     _Has = internal::is_detected<internal::detect_row_splits, _Config>::value>
    struct get_row_splits
    {
        using type = std::tuple<>;
    };

    template<typename _Config>
    struct get_row_splits<_Config, true>
    {
        using type = typename _Config::row_splits;
    };

    template<typename _Config>
    using get_row_splits_t = typename get_row_splits<_Config>::type;


    // =========================================================================
    // IV.  FEATURE DETECTION TRAITS
    // =========================================================================

    // has_header_rows
    //   trait: true if config specifies non-zero header rows.
    template<typename _Config>
    struct has_header_rows
        : std::integral_constant<bool, (get_header_rows<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_header_rows_v = has_header_rows<_Config>::value;
#endif

    // has_header_cols
    //   trait: true if config specifies non-zero header columns.
    template<typename _Config>
    struct has_header_cols
        : std::integral_constant<bool, (get_header_cols<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_header_cols_v = has_header_cols<_Config>::value;
#endif

    // has_footer_rows
    //   trait: true if config specifies non-zero footer rows.
    template<typename _Config>
    struct has_footer_rows
        : std::integral_constant<bool, (get_footer_rows<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_footer_rows_v = has_footer_rows<_Config>::value;
#endif

    // has_footer_cols
    //   trait: true if config specifies non-zero footer columns.
    template<typename _Config>
    struct has_footer_cols
        : std::integral_constant<bool, (get_footer_cols<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_footer_cols_v = has_footer_cols<_Config>::value;
#endif

    // has_total_rows
    //   trait: true if config specifies non-zero total rows.
    template<typename _Config>
    struct has_total_rows
        : std::integral_constant<bool, (get_total_rows<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_total_rows_v = has_total_rows<_Config>::value;
#endif

    // has_total_cols
    //   trait: true if config specifies non-zero total columns.
    template<typename _Config>
    struct has_total_cols
        : std::integral_constant<bool, (get_total_cols<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_total_cols_v = has_total_cols<_Config>::value;
#endif

    // has_multi_level_header
    //   trait: true if config specifies header_depth > 1, or provides a
    // multi_header type alias, indicating hierarchical header structure.
    template<typename _Config>
    struct has_multi_level_header
        : std::integral_constant<bool,
            ( (get_header_depth<_Config>::value > 1) ||
              internal::is_detected<internal::detect_multi_header, _Config>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_multi_level_header_v =
        has_multi_level_header<_Config>::value;
#endif

    // has_custom_total_row_position
    //   trait: true if config specifies a total_row_position member.
    template<typename _Config>
    struct has_custom_total_row_position
        : internal::is_detected<internal::detect_total_row_position, _Config>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_custom_total_row_position_v =
        has_custom_total_row_position<_Config>::value;
#endif

    // has_custom_total_col_position
    //   trait: true if config specifies a total_col_position member.
    template<typename _Config>
    struct has_custom_total_col_position
        : internal::is_detected<internal::detect_total_col_position, _Config>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_custom_total_col_position_v =
        has_custom_total_col_position<_Config>::value;
#endif

    // has_spans
    //   trait: true if config specifies any merged cell spans.
    template<typename _Config>
    struct has_spans
        : std::integral_constant<bool,
            (std::tuple_size<get_spans_t<_Config>>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_spans_v = has_spans<_Config>::value;
#endif

    // has_splits
    //   trait: true if config specifies any split cell descriptors.
    template<typename _Config>
    struct has_splits
        : std::integral_constant<bool,
            (std::tuple_size<get_splits_t<_Config>>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_splits_v = has_splits<_Config>::value;
#endif

    // has_multi_header
    //   trait: true if config specifies any multi-header group entries.
    template<typename _Config>
    struct has_multi_header
        : std::integral_constant<bool,
            (std::tuple_size<get_multi_header_t<_Config>>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_multi_header_v = has_multi_header<_Config>::value;
#endif

    // has_partitions
    //   trait: true if config specifies any partition descriptors.
    template<typename _Config>
    struct has_partitions
        : std::integral_constant<bool,
            (std::tuple_size<get_partitions_t<_Config>>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_partitions_v = has_partitions<_Config>::value;
#endif

    // has_col_merges
    //   trait: true if config specifies any structural column merges.
    template<typename _Config>
    struct has_col_merges
        : std::integral_constant<bool,
            (std::tuple_size<get_col_merges_t<_Config>>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_col_merges_v = has_col_merges<_Config>::value;
#endif

    // has_col_splits
    //   trait: true if config specifies any structural column splits.
    template<typename _Config>
    struct has_col_splits
        : std::integral_constant<bool,
            (std::tuple_size<get_col_splits_t<_Config>>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_col_splits_v = has_col_splits<_Config>::value;
#endif

    // has_row_merges
    //   trait: true if config specifies any structural row merges.
    template<typename _Config>
    struct has_row_merges
        : std::integral_constant<bool,
            (std::tuple_size<get_row_merges_t<_Config>>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_row_merges_v = has_row_merges<_Config>::value;
#endif

    // has_row_splits
    //   trait: true if config specifies any structural row splits.
    template<typename _Config>
    struct has_row_splits
        : std::integral_constant<bool,
            (std::tuple_size<get_row_splits_t<_Config>>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool has_row_splits_v = has_row_splits<_Config>::value;
#endif

    // is_config_type
    //   trait: true if type has any recognized config member.
    template<typename _Type>
    struct is_config_type
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_header_rows,  _Type>::value ||
              internal::is_detected<internal::detect_header_cols,  _Type>::value ||
              internal::is_detected<internal::detect_footer_rows,  _Type>::value ||
              internal::is_detected<internal::detect_footer_cols,  _Type>::value ||
              internal::is_detected<internal::detect_total_rows,   _Type>::value ||
              internal::is_detected<internal::detect_total_cols,   _Type>::value ||
              internal::is_detected<internal::detect_spans,        _Type>::value ||
              internal::is_detected<internal::detect_splits,       _Type>::value ||
              internal::is_detected<internal::detect_multi_header, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_config_type_v = is_config_type<_Type>::value;
#endif

    // is_basic_config
    //   trait: true if config has no special features (no headers, footers,
    // totals, merged spans, or split cells).
    template<typename _Config>
    struct is_basic_config
        : std::integral_constant<bool,
            ( !has_header_rows<_Config>::value &&
              !has_header_cols<_Config>::value &&
              !has_footer_rows<_Config>::value &&
              !has_footer_cols<_Config>::value &&
              !has_total_rows<_Config>::value  &&
              !has_total_cols<_Config>::value  &&
              !has_spans<_Config>::value        &&
              !has_splits<_Config>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Config>
    constexpr bool is_basic_config_v = is_basic_config<_Config>::value;
#endif


    // =========================================================================
    // V.   SPAN TRAITS (detects user-defined span types)
    // =========================================================================

    // is_span_type
    //   trait: true if type has row/col/row_span/col_span members.
    template<typename _Type>
    struct is_span_type
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_span_row,      _Type>::value &&
              internal::is_detected<internal::detect_span_col,      _Type>::value &&
              internal::is_detected<internal::detect_span_row_span, _Type>::value &&
              internal::is_detected<internal::detect_span_col_span, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_span_type_v = is_span_type<_Type>::value;
#endif

    // get_span_row
    //   trait: extracts row from span type.
    template<typename _Span,
             bool     _Has = internal::is_detected<internal::detect_span_row, _Span>::value>
    struct get_span_row : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Span>
    struct get_span_row<_Span, true>
        : std::integral_constant<std::size_t, _Span::row>
    {
    };

    // get_span_col
    //   trait: extracts col from span type.
    template<typename _Span,
             bool     _Has = internal::is_detected<internal::detect_span_col, _Span>::value>
    struct get_span_col : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Span>
    struct get_span_col<_Span, true>
        : std::integral_constant<std::size_t, _Span::col>
    {
    };

    // get_span_row_span
    //   trait: extracts row_span from span type.
    template<typename _Span,
             bool     _Has = internal::is_detected<internal::detect_span_row_span, _Span>::value>
    struct get_span_row_span : std::integral_constant<std::size_t, 1>
    {
    };

    template<typename _Span>
    struct get_span_row_span<_Span, true>
        : std::integral_constant<std::size_t, _Span::row_span>
    {
    };

    // get_span_col_span
    //   trait: extracts col_span from span type.
    template<typename _Span,
             bool     _Has = internal::is_detected<internal::detect_span_col_span, _Span>::value>
    struct get_span_col_span : std::integral_constant<std::size_t, 1>
    {
    };

    template<typename _Span>
    struct get_span_col_span<_Span, true>
        : std::integral_constant<std::size_t, _Span::col_span>
    {
    };

    // span_contains
    //   trait: checks if a cell position falls within a span.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Span>
    struct span_contains
        : std::integral_constant<bool,
            ( (_Row >= get_span_row<_Span>::value) &&
              (_Row <  get_span_row<_Span>::value + get_span_row_span<_Span>::value) &&
              (_Col >= get_span_col<_Span>::value) &&
              (_Col <  get_span_col<_Span>::value + get_span_col_span<_Span>::value) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _Row, std::size_t _Col, typename _Span>
    constexpr bool span_contains_v = span_contains<_Row, _Col, _Span>::value;
#endif

    // span_is_anchor
    //   trait: checks if a cell position is the anchor of a span.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Span>
    struct span_is_anchor
        : std::integral_constant<bool,
            ( (_Row == get_span_row<_Span>::value) &&
              (_Col == get_span_col<_Span>::value) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _Row, std::size_t _Col, typename _Span>
    constexpr bool span_is_anchor_v = span_is_anchor<_Row, _Col, _Span>::value;
#endif

    NS_INTERNAL

        // check_cell_in_spans
        //   trait: checks if cell is in any span from a tuple.
        template<std::size_t _Row,
                 std::size_t _Col,
                 typename    _Spans>
        struct check_cell_in_spans;

        template<std::size_t _Row,
                 std::size_t _Col>
        struct check_cell_in_spans<_Row, _Col, std::tuple<>> : std::false_type
        {
        };

        template<std::size_t _Row,
                 std::size_t _Col,
                 typename    _Head,
                 typename... _Tail>
        struct check_cell_in_spans<_Row, _Col, std::tuple<_Head, _Tail...>>
            : std::integral_constant<bool,
                ( span_contains<_Row, _Col, _Head>::value ||
                  check_cell_in_spans<_Row, _Col, std::tuple<_Tail...>>::value )>
        {
        };

        // find_cell_span
        //   trait: finds the span containing a cell.
        template<std::size_t _Row,
                 std::size_t _Col,
                 typename    _Spans>
        struct find_cell_span;

        template<std::size_t _Row,
                 std::size_t _Col>
        struct find_cell_span<_Row, _Col, std::tuple<>>
        {
            using type                  = void;
            static constexpr bool found = false;
        };

        template<std::size_t _Row,
                 std::size_t _Col,
                 typename    _Head,
                 typename... _Tail>
        struct find_cell_span<_Row, _Col, std::tuple<_Head, _Tail...>>
        {
        private:
            using rest = find_cell_span<_Row, _Col, std::tuple<_Tail...>>;

        public:
            using type = typename std::conditional<
                span_contains<_Row, _Col, _Head>::value,
                _Head,
                typename rest::type
            >::type;

            static constexpr bool found =
                ( span_contains<_Row, _Col, _Head>::value || rest::found );
        };

    NS_END  // internal

    // is_merged_cell
    //   trait: true if cell at (_Row, _Col) is in a merged region.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Config>
    struct is_merged_cell
        : internal::check_cell_in_spans<_Row, _Col, get_spans_t<_Config>>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _Row, std::size_t _Col, typename _Config>
    constexpr bool is_merged_cell_v = is_merged_cell<_Row, _Col, _Config>::value;
#endif

    // is_span_anchor
    //   trait: true if cell at (_Row, _Col) is anchor of a merged region.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Config>
    struct is_span_anchor
    {
    private:
        using spans      = get_spans_t<_Config>;
        using found_span = internal::find_cell_span<_Row, _Col, spans>;

    public:
        static constexpr bool value =
            ( found_span::found &&
              span_is_anchor<_Row, _Col, typename found_span::type>::value );
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _Row, std::size_t _Col, typename _Config>
    constexpr bool is_span_anchor_v = is_span_anchor<_Row, _Col, _Config>::value;
#endif


    // =========================================================================
    // V.b  SPLIT TRAITS (detects user-defined split types)
    // =========================================================================

    // is_split_type
    //   trait: true if type has row/col/sub_rows/sub_cols members,
    // identifying it as a split descriptor.
    template<typename _Type>
    struct is_split_type
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_split_row,      _Type>::value &&
              internal::is_detected<internal::detect_split_col,      _Type>::value &&
              internal::is_detected<internal::detect_split_sub_rows, _Type>::value &&
              internal::is_detected<internal::detect_split_sub_cols, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_split_type_v = is_split_type<_Type>::value;
#endif

    // get_split_row
    //   trait: extracts row from split type.
    template<typename _Split,
             bool     _Has = internal::is_detected<internal::detect_split_row, _Split>::value>
    struct get_split_row : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Split>
    struct get_split_row<_Split, true>
        : std::integral_constant<std::size_t, _Split::row>
    {
    };

    // get_split_col
    //   trait: extracts col from split type.
    template<typename _Split,
             bool     _Has = internal::is_detected<internal::detect_split_col, _Split>::value>
    struct get_split_col : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Split>
    struct get_split_col<_Split, true>
        : std::integral_constant<std::size_t, _Split::col>
    {
    };

    // get_split_sub_rows
    //   trait: extracts sub_rows from split type, defaulting to 1.
    template<typename _Split,
             bool     _Has = internal::is_detected<internal::detect_split_sub_rows, _Split>::value>
    struct get_split_sub_rows : std::integral_constant<std::size_t, 1>
    {
    };

    template<typename _Split>
    struct get_split_sub_rows<_Split, true>
        : std::integral_constant<std::size_t, _Split::sub_rows>
    {
    };

    // get_split_sub_cols
    //   trait: extracts sub_cols from split type, defaulting to 1.
    template<typename _Split,
             bool     _Has = internal::is_detected<internal::detect_split_sub_cols, _Split>::value>
    struct get_split_sub_cols : std::integral_constant<std::size_t, 1>
    {
    };

    template<typename _Split>
    struct get_split_sub_cols<_Split, true>
        : std::integral_constant<std::size_t, _Split::sub_cols>
    {
    };

    // split_targets
    //   trait: true if the split descriptor targets the cell at (_Row, _Col).
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Split>
    struct split_targets
        : std::integral_constant<bool,
            ( (_Row == get_split_row<_Split>::value) &&
              (_Col == get_split_col<_Split>::value) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _Row, std::size_t _Col, typename _Split>
    constexpr bool split_targets_v = split_targets<_Row, _Col, _Split>::value;
#endif

    // split_is_subdivided
    //   trait: true if the split actually subdivides (sub_rows > 1 or
    // sub_cols > 1). A 1x1 split is effectively no subdivision.
    template<typename _Split>
    struct split_is_subdivided
        : std::integral_constant<bool,
            ( (get_split_sub_rows<_Split>::value > 1) ||
              (get_split_sub_cols<_Split>::value > 1) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Split>
    constexpr bool split_is_subdivided_v = split_is_subdivided<_Split>::value;
#endif

    // split_sub_cell_count
    //   trait: computes the total number of sub-cells for a split descriptor.
    template<typename _Split>
    struct split_sub_cell_count
        : std::integral_constant<std::size_t,
            (get_split_sub_rows<_Split>::value *
             get_split_sub_cols<_Split>::value)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Split>
    constexpr std::size_t split_sub_cell_count_v =
        split_sub_cell_count<_Split>::value;
#endif

    NS_INTERNAL

        // check_cell_in_splits
        //   trait: checks if cell at (_Row, _Col) is targeted by any split
        // in a tuple of split descriptors.
        template<std::size_t _Row,
                 std::size_t _Col,
                 typename    _Splits>
        struct check_cell_in_splits;

        // check_cell_in_splits (base case)
        //   trait: empty tuple — no splits to check.
        template<std::size_t _Row,
                 std::size_t _Col>
        struct check_cell_in_splits<_Row, _Col, std::tuple<>> : std::false_type
        {
        };

        // check_cell_in_splits (recursive case)
        //   trait: checks head split, recurses on tail.
        template<std::size_t _Row,
                 std::size_t _Col,
                 typename    _Head,
                 typename... _Tail>
        struct check_cell_in_splits<_Row, _Col, std::tuple<_Head, _Tail...>>
            : std::integral_constant<bool,
                ( split_targets<_Row, _Col, _Head>::value ||
                  check_cell_in_splits<_Row, _Col, std::tuple<_Tail...>>::value )>
        {
        };

        // find_cell_split
        //   trait: finds the split descriptor targeting a cell.
        template<std::size_t _Row,
                 std::size_t _Col,
                 typename    _Splits>
        struct find_cell_split;

        // find_cell_split (base case)
        //   trait: empty tuple — no match.
        template<std::size_t _Row,
                 std::size_t _Col>
        struct find_cell_split<_Row, _Col, std::tuple<>>
        {
            using type                  = void;
            static constexpr bool found = false;
        };

        // find_cell_split (recursive case)
        //   trait: checks head, recurses on tail.
        template<std::size_t _Row,
                 std::size_t _Col,
                 typename    _Head,
                 typename... _Tail>
        struct find_cell_split<_Row, _Col, std::tuple<_Head, _Tail...>>
        {
        private:
            using rest = find_cell_split<_Row, _Col, std::tuple<_Tail...>>;

        public:
            using type = typename std::conditional<
                split_targets<_Row, _Col, _Head>::value,
                _Head,
                typename rest::type
            >::type;

            static constexpr bool found =
                ( split_targets<_Row, _Col, _Head>::value || rest::found );
        };

    NS_END  // internal

    // is_split_cell
    //   trait: true if cell at (_Row, _Col) is targeted by a split
    // descriptor in the config.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Config>
    struct is_split_cell
        : internal::check_cell_in_splits<_Row, _Col, get_splits_t<_Config>>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _Row, std::size_t _Col, typename _Config>
    constexpr bool is_split_cell_v = is_split_cell<_Row, _Col, _Config>::value;
#endif

    // cell_split_dimensions
    //   trait: extracts the sub-dimensions of the split targeting a cell,
    // or 1x1 if no split targets that cell.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Config>
    struct cell_split_dimensions
    {
    private:
        using splits      = get_splits_t<_Config>;
        using found_split = internal::find_cell_split<_Row, _Col, splits>;
        using split_type  = typename found_split::type;

    public:
        static constexpr bool        is_split =  found_split::found;
        static constexpr std::size_t sub_rows =
            found_split::found
                ? get_split_sub_rows<split_type>::value
                : 1;
        static constexpr std::size_t sub_cols =
            found_split::found
                ? get_split_sub_cols<split_type>::value
                : 1;
        static constexpr std::size_t sub_cells = sub_rows * sub_cols;
    };


    // =========================================================================
    // V.c  CELL LAYOUT (combined merge + split classification)
    // =========================================================================

    // cell_layout_kind
    //   enum: identifies how a cell is logically arranged.
    enum class cell_layout_kind
    {
        normal,
        merged_anchor,
        merged_shadow,
        split
    };

    // cell_layout
    //   trait: computes the combined merge/split layout for a specific cell.
    // A cell is exactly one of: normal, merged anchor, merged shadow, or split.
    // Merges take priority — a cell cannot be both merged and split.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Config>
    struct cell_layout
    {
    private:
        static constexpr bool in_span  = is_merged_cell<_Row, _Col, _Config>::value;
        static constexpr bool is_anchor =
            in_span && is_span_anchor<_Row, _Col, _Config>::value;
        static constexpr bool in_split = is_split_cell<_Row, _Col, _Config>::value;

    public:
        static constexpr cell_layout_kind kind =
            ( is_anchor                  ? cell_layout_kind::merged_anchor :
              (in_span && !is_anchor)    ? cell_layout_kind::merged_shadow :
              (in_split && !in_span)     ? cell_layout_kind::split         :
              cell_layout_kind::normal );

        static constexpr bool is_normal        = (kind == cell_layout_kind::normal);
        static constexpr bool is_merged        = in_span;
        static constexpr bool is_merge_anchor  = is_anchor;
        static constexpr bool is_merge_shadow  = (in_span && !is_anchor);
        static constexpr bool is_subdivided    = (in_split && !in_span);
    };


    // =========================================================================
    // V.d  MULTI-HEADER TRAITS
    // =========================================================================

    // is_multi_header_entry
    //   trait: true if type has level/col/col_span members, identifying
    // it as a multi-header group descriptor.
    template<typename _Type>
    struct is_multi_header_entry
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_mh_level,    _Type>::value &&
              internal::is_detected<internal::detect_mh_col,      _Type>::value &&
              internal::is_detected<internal::detect_mh_col_span, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_multi_header_entry_v =
        is_multi_header_entry<_Type>::value;
#endif

    // get_mh_level
    //   trait: extracts level from a multi-header entry.
    template<typename _Entry,
             bool     _Has = internal::is_detected<internal::detect_mh_level, _Entry>::value>
    struct get_mh_level : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Entry>
    struct get_mh_level<_Entry, true>
        : std::integral_constant<std::size_t, _Entry::level>
    {
    };

    // get_mh_col
    //   trait: extracts col from a multi-header entry.
    template<typename _Entry,
             bool     _Has = internal::is_detected<internal::detect_mh_col, _Entry>::value>
    struct get_mh_col : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _Entry>
    struct get_mh_col<_Entry, true>
        : std::integral_constant<std::size_t, _Entry::col>
    {
    };

    // get_mh_col_span
    //   trait: extracts col_span from a multi-header entry.
    template<typename _Entry,
             bool     _Has = internal::is_detected<internal::detect_mh_col_span, _Entry>::value>
    struct get_mh_col_span : std::integral_constant<std::size_t, 1>
    {
    };

    template<typename _Entry>
    struct get_mh_col_span<_Entry, true>
        : std::integral_constant<std::size_t, _Entry::col_span>
    {
    };

    // mh_entry_contains_col
    //   trait: true if a column falls within a multi-header entry's span.
    template<std::size_t _Col,
             typename    _Entry>
    struct mh_entry_contains_col
        : std::integral_constant<bool,
            ( (_Col >= get_mh_col<_Entry>::value) &&
              (_Col <  get_mh_col<_Entry>::value + get_mh_col_span<_Entry>::value) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _Col, typename _Entry>
    constexpr bool mh_entry_contains_col_v =
        mh_entry_contains_col<_Col, _Entry>::value;
#endif

    // mh_entry_matches_level
    //   trait: true if a multi-header entry is at the specified level.
    template<std::size_t _Level,
             typename    _Entry>
    struct mh_entry_matches_level
        : std::integral_constant<bool,
            (_Level == get_mh_level<_Entry>::value)>
    {
    };

    NS_INTERNAL

        // find_mh_at_level_col
        //   trait: finds the multi-header entry at a given level that
        // contains a specific column.
        template<std::size_t _Level,
                 std::size_t _Col,
                 typename    _Entries>
        struct find_mh_at_level_col;

        // find_mh_at_level_col (base case)
        //   trait: empty tuple — no match.
        template<std::size_t _Level,
                 std::size_t _Col>
        struct find_mh_at_level_col<_Level, _Col, std::tuple<>>
        {
            using type                  = void;
            static constexpr bool found = false;
        };

        // find_mh_at_level_col (recursive case)
        //   trait: checks head, recurses on tail.
        template<std::size_t _Level,
                 std::size_t _Col,
                 typename    _Head,
                 typename... _Tail>
        struct find_mh_at_level_col<_Level, _Col, std::tuple<_Head, _Tail...>>
        {
        private:
            static constexpr bool match =
                ( mh_entry_matches_level<_Level, _Head>::value &&
                  mh_entry_contains_col<_Col, _Head>::value );
            using rest = find_mh_at_level_col<_Level, _Col,
                                               std::tuple<_Tail...>>;

        public:
            using type = typename std::conditional<
                match, _Head, typename rest::type>::type;

            static constexpr bool found = (match || rest::found);
        };

    NS_END  // internal

    // header_cell_group
    //   trait: finds the multi-header group entry covering a given
    // header level and column. Provides the entry type, whether a
    // match was found, and the group's col_span.
    template<std::size_t _Level,
             std::size_t _Col,
             typename    _Config>
    struct header_cell_group
    {
    private:
        using entries = get_multi_header_t<_Config>;
        using result  = internal::find_mh_at_level_col<_Level, _Col, entries>;

    public:
        using type = typename result::type;

        static constexpr bool found = result::found;

        static constexpr std::size_t col_span =
            result::found
                ? get_mh_col_span<typename result::type>::value
                : 1;
    };


    // =========================================================================
    // V.e  PARTITION TRAITS
    // =========================================================================

    // is_partition_type
    //   trait: true if type has row_start/col_start/row_count/col_count
    // members, identifying it as a partition descriptor.
    template<typename _Type>
    struct is_partition_type
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_part_row_start, _Type>::value &&
              internal::is_detected<internal::detect_part_col_start, _Type>::value &&
              internal::is_detected<internal::detect_part_row_count, _Type>::value &&
              internal::is_detected<internal::detect_part_col_count, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_partition_type_v = is_partition_type<_Type>::value;
#endif

    // partition_contains
    //   trait: true if a cell at (_Row, _Col) falls within a partition.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Part>
    struct partition_contains
        : std::integral_constant<bool,
            ( (_Row >= _Part::row_start) &&
              (_Row <  _Part::row_start + _Part::row_count) &&
              (_Col >= _Part::col_start) &&
              (_Col <  _Part::col_start + _Part::col_count) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _Row, std::size_t _Col, typename _Part>
    constexpr bool partition_contains_v =
        partition_contains<_Row, _Col, _Part>::value;
#endif

    NS_INTERNAL

        // find_cell_partition
        //   trait: finds the partition containing a cell, returning its
        // zero-based index.
        template<std::size_t _Row,
                 std::size_t _Col,
                 std::size_t _Index,
                 typename    _Parts>
        struct find_cell_partition;

        // find_cell_partition (base case)
        //   trait: empty tuple — no partition found.
        template<std::size_t _Row,
                 std::size_t _Col,
                 std::size_t _Index>
        struct find_cell_partition<_Row, _Col, _Index, std::tuple<>>
        {
            using type                      = void;
            static constexpr bool found     = false;
            static constexpr std::size_t index = _Index;
        };

        // find_cell_partition (recursive case)
        //   trait: checks head partition, recurses on tail.
        template<std::size_t _Row,
                 std::size_t _Col,
                 std::size_t _Index,
                 typename    _Head,
                 typename... _Tail>
        struct find_cell_partition<_Row, _Col, _Index,
                                   std::tuple<_Head, _Tail...>>
        {
        private:
            static constexpr bool match =
                partition_contains<_Row, _Col, _Head>::value;
            using rest = find_cell_partition<_Row, _Col, _Index + 1,
                                             std::tuple<_Tail...>>;

        public:
            using type = typename std::conditional<
                match, _Head, typename rest::type>::type;

            static constexpr bool found = (match || rest::found);

            static constexpr std::size_t index =
                match ? _Index : rest::index;
        };

    NS_END  // internal

    // cell_partition
    //   trait: determines which partition (if any) contains a cell.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Config>
    struct cell_partition
    {
    private:
        using parts  = get_partitions_t<_Config>;
        using result = internal::find_cell_partition<_Row, _Col, 0, parts>;

    public:
        using type = typename result::type;

        static constexpr bool        found = result::found;
        static constexpr std::size_t index = result::index;
    };


    // =========================================================================
    // V.f  STRUCTURAL COLUMN/ROW MERGE AND SPLIT TRAITS
    // =========================================================================

    // is_col_merge_type
    //   trait: true if type has col/count members, identifying it as
    // a structural column merge descriptor.
    template<typename _Type>
    struct is_col_merge_type
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_col_merge_col,   _Type>::value &&
              internal::is_detected<internal::detect_col_merge_count, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_col_merge_type_v = is_col_merge_type<_Type>::value;
#endif

    // col_merge_contains
    //   trait: true if a column index falls within a column merge range.
    template<std::size_t _Col,
             typename    _Merge>
    struct col_merge_contains
        : std::integral_constant<bool,
            ( (_Col >= _Merge::col) &&
              (_Col <  _Merge::col + _Merge::count) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _Col, typename _Merge>
    constexpr bool col_merge_contains_v =
        col_merge_contains<_Col, _Merge>::value;
#endif

    NS_INTERNAL

        // find_col_merge
        //   trait: finds the column merge descriptor containing a column.
        template<std::size_t _Col,
                 typename    _Merges>
        struct find_col_merge;

        template<std::size_t _Col>
        struct find_col_merge<_Col, std::tuple<>>
        {
            using type                  = void;
            static constexpr bool found = false;
        };

        template<std::size_t _Col,
                 typename    _Head,
                 typename... _Tail>
        struct find_col_merge<_Col, std::tuple<_Head, _Tail...>>
        {
        private:
            using rest = find_col_merge<_Col, std::tuple<_Tail...>>;

        public:
            using type = typename std::conditional<
                col_merge_contains<_Col, _Head>::value,
                _Head, typename rest::type>::type;

            static constexpr bool found =
                ( col_merge_contains<_Col, _Head>::value || rest::found );
        };

    NS_END  // internal

    // column_merge_info
    //   trait: provides the merge descriptor for a column, if any.
    template<std::size_t _Col,
             typename    _Config>
    struct column_merge_info
    {
    private:
        using merges = get_col_merges_t<_Config>;
        using result = internal::find_col_merge<_Col, merges>;

    public:
        using type = typename result::type;

        static constexpr bool found = result::found;

        static constexpr bool is_anchor =
            ( result::found &&
              (_Col == result::type::col) );
    };

    // is_col_split_type
    //   trait: true if type has col/sub_cols members.
    template<typename _Type>
    struct is_col_split_type
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_col_split_col,      _Type>::value &&
              internal::is_detected<internal::detect_col_split_sub_cols, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_col_split_type_v = is_col_split_type<_Type>::value;
#endif

    NS_INTERNAL

        // find_col_split
        //   trait: finds the column split descriptor targeting a column.
        template<std::size_t _Col,
                 typename    _Splits>
        struct find_col_split;

        template<std::size_t _Col>
        struct find_col_split<_Col, std::tuple<>>
        {
            using type                  = void;
            static constexpr bool found = false;
        };

        template<std::size_t _Col,
                 typename    _Head,
                 typename... _Tail>
        struct find_col_split<_Col, std::tuple<_Head, _Tail...>>
        {
        private:
            using rest = find_col_split<_Col, std::tuple<_Tail...>>;

        public:
            using type = typename std::conditional<
                (_Col == _Head::col),
                _Head, typename rest::type>::type;

            static constexpr bool found =
                ( (_Col == _Head::col) || rest::found );
        };

    NS_END  // internal

    // column_split_info
    //   trait: provides the split descriptor for a column, if any.
    template<std::size_t _Col,
             typename    _Config>
    struct column_split_info
    {
    private:
        using splits = get_col_splits_t<_Config>;
        using result = internal::find_col_split<_Col, splits>;

    public:
        using type = typename result::type;

        static constexpr bool found = result::found;

        static constexpr std::size_t sub_cols =
            result::found ? result::type::sub_cols : 1;
    };

    // is_row_merge_type
    //   trait: true if type has row/count members.
    template<typename _Type>
    struct is_row_merge_type
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_row_merge_row,   _Type>::value &&
              internal::is_detected<internal::detect_row_merge_count, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_row_merge_type_v = is_row_merge_type<_Type>::value;
#endif

    // row_merge_contains
    //   trait: true if a row index falls within a row merge range.
    template<std::size_t _Row,
             typename    _Merge>
    struct row_merge_contains
        : std::integral_constant<bool,
            ( (_Row >= _Merge::row) &&
              (_Row <  _Merge::row + _Merge::count) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _Row, typename _Merge>
    constexpr bool row_merge_contains_v =
        row_merge_contains<_Row, _Merge>::value;
#endif

    // is_row_split_type
    //   trait: true if type has row/sub_rows members.
    template<typename _Type>
    struct is_row_split_type
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_row_split_row,      _Type>::value &&
              internal::is_detected<internal::detect_row_split_sub_rows, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_row_split_type_v = is_row_split_type<_Type>::value;
#endif


    // =========================================================================
    // VI.  EMPTY CONFIG
    // =========================================================================

    // empty_config
    //   struct: minimal config for basic tables with no features.
    struct empty_config
    {
    };


    // =========================================================================
    // VII. STRUCTURAL IMMUTABILITY DETECTION
    // =========================================================================
    //
    // A structurally immutable table has a fixed shape (row count, column
    // count, merge/split configuration) at compile time, but its cell values
    // can be modified at runtime. This is distinct from:
    //   - Fully const (nothing changeable)
    //   - Structurally mutable (dynamic row/column count)
    //
    // Detection is purely structural (SFINAE on member presence/absence):
    //   Present:  num_rows, num_cols, size(), cell() or operator[]
    //   Absent:   resize, add_row, remove_row, add_column, remove_column
    //

    NS_INTERNAL

        // shape-modifier detection operations (used to detect absence)
        template<typename _T>
        using detect_resize = decltype(
            std::declval<_T&>().resize(std::size_t{}, std::size_t{}));

        template<typename _T>
        using detect_add_row = decltype(std::declval<_T&>().add_row());

        template<typename _T>
        using detect_remove_row = decltype(
            std::declval<_T&>().remove_row(std::size_t{}));

        template<typename _T>
        using detect_add_column = decltype(std::declval<_T&>().add_column());

        template<typename _T>
        using detect_remove_column = decltype(
            std::declval<_T&>().remove_column(std::size_t{}));

        // dimension-constant detection operations (used to detect presence)
        template<typename _T>
        using detect_num_rows_constant = decltype(
            std::integral_constant<std::size_t, _T::num_rows>{});

        template<typename _T>
        using detect_num_cols_constant = decltype(
            std::integral_constant<std::size_t, _T::num_cols>{});

        // mutable-element detection operations
        template<typename _T>
        using detect_mutable_subscript = decltype(
            std::declval<_T&>()[std::size_t{}]);

        template<typename _T>
        using detect_mutable_cell = decltype(
            std::declval<_T&>().cell(std::size_t{}, std::size_t{}));

    NS_END  // internal

    // has_fixed_dimensions
    //   trait: true if the type exposes compile-time num_rows and num_cols
    // constants.
    template<typename _Type,
             typename = void>
    struct has_fixed_dimensions : std::false_type
    {
    };

    // has_fixed_dimensions (specialization)
    //   trait: SFINAE success — both dimension constants are well-formed.
    template<typename _Type>
    struct has_fixed_dimensions<_Type,
        void_t<internal::detect_num_rows_constant<_Type>,
               internal::detect_num_cols_constant<_Type>>>
        : std::true_type
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_fixed_dimensions_v =
        has_fixed_dimensions<_Type>::value;
#endif

    // has_shape_modifiers
    //   trait: true if the type exposes any method that can change the
    // row or column count at runtime.
    template<typename _Type>
    struct has_shape_modifiers
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_resize,        _Type>::value ||
              internal::is_detected<internal::detect_add_row,       _Type>::value ||
              internal::is_detected<internal::detect_remove_row,    _Type>::value ||
              internal::is_detected<internal::detect_add_column,    _Type>::value ||
              internal::is_detected<internal::detect_remove_column, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_shape_modifiers_v =
        has_shape_modifiers<_Type>::value;
#endif

    // has_mutable_element_access
    //   trait: true if the type provides non-const element access via
    // operator[] or cell().
    template<typename _Type>
    struct has_mutable_element_access
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_mutable_subscript, _Type>::value ||
              internal::is_detected<internal::detect_mutable_cell,      _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_mutable_element_access_v =
        has_mutable_element_access<_Type>::value;
#endif

    // is_structurally_immutable
    //   trait: true if the type has fixed compile-time dimensions and no
    // shape-modifying operations. Cell values may still be mutable.
    template<typename _Type>
    struct is_structurally_immutable
        : std::integral_constant<bool,
            ( has_fixed_dimensions<_Type>::value &&
              !has_shape_modifiers<_Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_structurally_immutable_v =
        is_structurally_immutable<_Type>::value;
#endif

    // is_value_mutable
    //   trait: true if the type is structurally immutable but provides
    // mutable element access. This is the canonical table contract: fixed
    // shape, modifiable contents.
    template<typename _Type>
    struct is_value_mutable
        : std::integral_constant<bool,
            ( is_structurally_immutable<_Type>::value &&
              has_mutable_element_access<_Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_value_mutable_v = is_value_mutable<_Type>::value;
#endif


    // =========================================================================
    // VIII. TABLE CONFIG CLASSIFICATION
    // =========================================================================

    // table_config_class
    //   struct: aggregates all config-level feature detections into a single
    // compile-time classification. Query this instead of individual traits
    // when you need the full config picture.
    template<typename _Config>
    struct table_config_class
    {
        // region features
        static constexpr bool has_header_row   = has_header_rows<_Config>::value;
        static constexpr bool has_header_col   = has_header_cols<_Config>::value;
        static constexpr bool has_footer_row   = has_footer_rows<_Config>::value;
        static constexpr bool has_footer_col   = has_footer_cols<_Config>::value;
        static constexpr bool has_total_row    = has_total_rows<_Config>::value;
        static constexpr bool has_total_col    = has_total_cols<_Config>::value;

        // multi-level header
        static constexpr bool has_multi_level  =
            has_multi_level_header<_Config>::value;
        static constexpr std::size_t header_depth =
            get_header_depth<_Config>::value;

        // total positioning
        static constexpr bool has_custom_total_pos =
            ( has_custom_total_row_position<_Config>::value ||
              has_custom_total_col_position<_Config>::value );

        // cell layout features
        static constexpr bool has_merged_cells = has_spans<_Config>::value;
        static constexpr bool has_split_cells  = has_splits<_Config>::value;

        // multi-header groups
        static constexpr bool has_header_groups =
            has_multi_header<_Config>::value;

        // partitions
        static constexpr bool has_table_partitions =
            has_partitions<_Config>::value;

        // structural column/row merges and splits
        static constexpr bool has_structural_col_merges =
            has_col_merges<_Config>::value;
        static constexpr bool has_structural_col_splits =
            has_col_splits<_Config>::value;
        static constexpr bool has_structural_row_merges =
            has_row_merges<_Config>::value;
        static constexpr bool has_structural_row_splits =
            has_row_splits<_Config>::value;

        // aggregate
        static constexpr bool has_regions =
            ( has_header_row || has_header_col ||
              has_footer_row || has_footer_col ||
              has_total_row  || has_total_col );

        static constexpr bool has_layout_features =
            ( has_merged_cells || has_split_cells );

        static constexpr bool has_structural_features =
            ( has_structural_col_merges || has_structural_col_splits ||
              has_structural_row_merges || has_structural_row_splits );

        static constexpr bool is_basic =
            ( !has_regions          && !has_layout_features &&
              !has_header_groups    && !has_table_partitions &&
              !has_structural_features );

        // merge count (number of span descriptors in config)
        static constexpr std::size_t merge_count =
            std::tuple_size<get_spans_t<_Config>>::value;

        // split count (number of split descriptors in config)
        static constexpr std::size_t split_count =
            std::tuple_size<get_splits_t<_Config>>::value;

        // partition count
        static constexpr std::size_t partition_count =
            std::tuple_size<get_partitions_t<_Config>>::value;

        // structural merge/split counts
        static constexpr std::size_t col_merge_count =
            std::tuple_size<get_col_merges_t<_Config>>::value;
        static constexpr std::size_t col_split_count =
            std::tuple_size<get_col_splits_t<_Config>>::value;
        static constexpr std::size_t row_merge_count =
            std::tuple_size<get_row_merges_t<_Config>>::value;
        static constexpr std::size_t row_split_count =
            std::tuple_size<get_row_splits_t<_Config>>::value;

        // multi-header entry count
        static constexpr std::size_t multi_header_count =
            std::tuple_size<get_multi_header_t<_Config>>::value;
    };


    // =========================================================================
    // IX.  CELL REGION CLASSIFICATION (fixed dimensions)
    // =========================================================================
    //
    // Requires compile-time _TotalRows and _TotalCols to determine which
    // region a cell at (_Row, _Col) belongs to. Not applicable to dynamic-
    // dimension tables; use runtime equivalents for database_table.
    //

    // cell_region
    //   enum: identifies the logical region a cell belongs to.
    enum class cell_region
    {
        header,
        header_col,
        data,
        total,
        total_col,
        footer,
        footer_col,
        header_total,
        corner_top_left,
        corner_top_right,
        corner_bot_left,
        corner_bot_right,
        total_header_col,
        total_footer_col
    };

    // cell_position
    //   trait: determines the region of a cell at compile time.
    template<std::size_t _Row,
             std::size_t _Col,
             std::size_t _TotalRows,
             std::size_t _TotalCols,
             typename    _Config>
    struct cell_position
    {
    private:
        static constexpr std::size_t hdr_rows = get_header_rows<_Config>::value;
        static constexpr std::size_t hdr_cols = get_header_cols<_Config>::value;
        static constexpr std::size_t ftr_rows = get_footer_rows<_Config>::value;
        static constexpr std::size_t ftr_cols = get_footer_cols<_Config>::value;
        static constexpr std::size_t tot_rows = get_total_rows<_Config>::value;
        static constexpr std::size_t tot_cols = get_total_cols<_Config>::value;

        static constexpr bool in_header_row = (_Row < hdr_rows);
        static constexpr bool in_header_col = (_Col < hdr_cols);
        static constexpr bool in_footer_row = (_Row >= _TotalRows - ftr_rows);
        static constexpr bool in_footer_col = (_Col >= _TotalCols - ftr_cols);
        static constexpr bool in_total_row  =
            ( (_Row >= _TotalRows - ftr_rows - tot_rows) &&
              (_Row <  _TotalRows - ftr_rows) );
        static constexpr bool in_total_col  =
            ( (_Col >= _TotalCols - ftr_cols - tot_cols) &&
              (_Col <  _TotalCols - ftr_cols) );

    public:
        static constexpr cell_region region =
            ( (in_header_row && in_header_col) ? cell_region::corner_top_left  :
              (in_header_row && in_footer_col) ? cell_region::corner_top_right :
              (in_footer_row && in_header_col) ? cell_region::corner_bot_left  :
              (in_footer_row && in_footer_col) ? cell_region::corner_bot_right :
              (in_header_row && in_total_col)  ? cell_region::header_total     :
              (in_header_row)                  ? cell_region::header           :
              (in_footer_row)                  ? cell_region::footer           :
              (in_header_col)                  ? cell_region::header_col       :
              (in_footer_col)                  ? cell_region::footer_col       :
              (in_total_row)                   ? cell_region::total            :
              (in_total_col)                   ? cell_region::total_col        :
              cell_region::data );

        static constexpr bool is_data_cell   = (region == cell_region::data);
        static constexpr bool is_header_cell =
            ( (region == cell_region::header) ||
              (region == cell_region::corner_top_left) );
    };


    // =========================================================================
    // X.   DIMENSION COMPUTATION (fixed dimensions)
    // =========================================================================
    //
    // Computes all dimension metrics from compile-time _Rows, _Cols and a
    // _Config type. The static_asserts validate that the config's special
    // rows and columns fit within the declared dimensions.
    //

    // table_dimensions
    //   trait: computes all dimension metrics for a fixed-size table.
    template<std::size_t _Rows,
             std::size_t _Cols,
             typename    _Config>
    struct table_dimensions
    {
        static constexpr std::size_t total_rows  = _Rows;
        static constexpr std::size_t total_cols  = _Cols;
        static constexpr std::size_t total_cells = _Rows * _Cols;

        static constexpr std::size_t header_rows  = get_header_rows<_Config>::value;
        static constexpr std::size_t header_cols  = get_header_cols<_Config>::value;
        static constexpr std::size_t footer_rows  = get_footer_rows<_Config>::value;
        static constexpr std::size_t footer_cols  = get_footer_cols<_Config>::value;
        static constexpr std::size_t summary_rows = get_total_rows<_Config>::value;
        static constexpr std::size_t summary_cols = get_total_cols<_Config>::value;

        static constexpr std::size_t data_rows =
            _Rows - header_rows - footer_rows - summary_rows;
        static constexpr std::size_t data_cols =
            _Cols - header_cols - footer_cols - summary_cols;
        static constexpr std::size_t data_cells = data_rows * data_cols;

        static constexpr std::size_t data_row_start = header_rows;
        static constexpr std::size_t data_col_start = header_cols;
        static constexpr std::size_t data_row_end   = _Rows - footer_rows - summary_rows;
        static constexpr std::size_t data_col_end   = _Cols - footer_cols - summary_cols;

        // structural merge/split counts (from config classification)
        static constexpr std::size_t partition_count =
            std::tuple_size<get_partitions_t<_Config>>::value;
        static constexpr std::size_t col_merge_count =
            std::tuple_size<get_col_merges_t<_Config>>::value;
        static constexpr std::size_t col_split_count =
            std::tuple_size<get_col_splits_t<_Config>>::value;
        static constexpr std::size_t row_merge_count =
            std::tuple_size<get_row_merges_t<_Config>>::value;
        static constexpr std::size_t row_split_count =
            std::tuple_size<get_row_splits_t<_Config>>::value;

        static_assert((header_rows + footer_rows + summary_rows <= _Rows),
                      "Combined special rows exceed total rows.");
        static_assert((header_cols + footer_cols + summary_cols <= _Cols),
                      "Combined special columns exceed total columns.");
    };


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_TABLE_TRAITS_
