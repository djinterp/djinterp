/******************************************************************************
* djinterp [container]                                      table_concepts.hpp
*
* Table concepts:
*   C++20 concepts layered over table.hpp and table_traits.hpp. These
* concepts provide readable constraints for table-type, table-config, and
* table-layout-like types without replacing the existing SFINAE trait
* surface.
*
*   This header covers the full public trait surface from both headers:
*   - table type identity (static / dynamic / any)
*   - structural mutability (immutable shape, shape-modifiable)
*   - config/type classification
*   - region/header/footer/total traits
*   - merged/split/partitioned layout traits
*   - structural row/column merge/split traits
*   - descriptor concepts (span / partition)
*   - fixed-dimension cell-region concepts
*
* path:      /inc/djinterp/core/container/table/table_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.24
******************************************************************************/

#ifndef DJINTERP_TABLE_CONCEPTS_
#define DJINTERP_TABLE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "table_concepts.hpp requires C++ compilation"
#endif

#include "table.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // =========================================================================
    // I.   CONFIG IDENTITY CONCEPTS
    // =========================================================================

    // table_config_type
    //   concept: constrains types recognized as table config types.
    template<typename _Config>
    concept table_config_type =
        is_config_type<_Config>::value;

    // basic_table_config_type
    //   concept: constrains configs with no special regions or layout features.
    template<typename _Config>
    concept basic_table_config_type =
        is_basic_config<_Config>::value;

    // classified_table_config_type
    //   concept: constrains configs recognized by the table config trait surface.
    template<typename _Config>
    concept classified_table_config_type =
        table_config_type<_Config>;


    // =========================================================================
    // II.  REGION / HEADER / FOOTER / TOTAL CONCEPTS
    // =========================================================================

    // header_row_table_config
    //   concept: constrains configs with header rows.
    template<typename _Config>
    concept header_row_table_config =
        has_header_rows<_Config>::value;

    // header_col_table_config
    //   concept: constrains configs with header columns.
    template<typename _Config>
    concept header_col_table_config =
        has_header_cols<_Config>::value;

    // footer_row_table_config
    //   concept: constrains configs with footer rows.
    template<typename _Config>
    concept footer_row_table_config =
        has_footer_rows<_Config>::value;

    // footer_col_table_config
    //   concept: constrains configs with footer columns.
    template<typename _Config>
    concept footer_col_table_config =
        has_footer_cols<_Config>::value;

    // total_row_table_config
    //   concept: constrains configs with total rows.
    template<typename _Config>
    concept total_row_table_config =
        has_total_rows<_Config>::value;

    // total_col_table_config
    //   concept: constrains configs with total columns.
    template<typename _Config>
    concept total_col_table_config =
        has_total_cols<_Config>::value;

    // multi_level_header_table_config
    //   concept: constrains configs with multi-level header structure.
    template<typename _Config>
    concept multi_level_header_table_config =
        has_multi_level_header<_Config>::value;

    // custom_total_position_table_config
    //   concept: constrains configs with explicit total row/column placement.
    template<typename _Config>
    concept custom_total_position_table_config =
        ( has_custom_total_row_position<_Config>::value ||
          has_custom_total_col_position<_Config>::value );

    // headered_table_config
    //   concept: constrains configs with any header region.
    template<typename _Config>
    concept headered_table_config =
        ( has_header_rows<_Config>::value ||
          has_header_cols<_Config>::value );

    // footered_table_config
    //   concept: constrains configs with any footer region.
    template<typename _Config>
    concept footered_table_config =
        ( has_footer_rows<_Config>::value ||
          has_footer_cols<_Config>::value );

    // totaled_table_config
    //   concept: constrains configs with any total region.
    template<typename _Config>
    concept totaled_table_config =
        ( has_total_rows<_Config>::value ||
          has_total_cols<_Config>::value );

    // regioned_table_config
    //   concept: constrains configs with any non-data region.
    template<typename _Config>
    concept regioned_table_config =
        ( headered_table_config<_Config> ||
          footered_table_config<_Config> ||
          totaled_table_config<_Config> );


    // =========================================================================
    // III. DESCRIPTOR CONCEPTS
    // =========================================================================

    // span_descriptor_type
    //   concept: constrains types that satisfy the span descriptor protocol.
    template<typename _Type>
    concept span_descriptor_type =
        is_span_type<_Type>::value;

    // partition_descriptor_type
    //   concept: constrains types that satisfy the partition descriptor protocol.
    template<typename _Type>
    concept partition_descriptor_type =
        is_partition_type<_Type>::value;


    // =========================================================================
    // IV.  LAYOUT-FEATURE CONCEPTS
    // =========================================================================

    // merged_table_config
    //   concept: constrains configs with merged-cell span descriptors.
    template<typename _Config>
    concept merged_table_config =
        has_spans<_Config>::value;

    // split_table_config
    //   concept: constrains configs with split-cell descriptors.
    template<typename _Config>
    concept split_table_config =
        has_splits<_Config>::value;

    // multi_header_table_config
    //   concept: constrains configs with grouped multi-header entries.
    template<typename _Config>
    concept multi_header_table_config =
        has_multi_header<_Config>::value;

    // partitioned_table_config
    //   concept: constrains configs with partition descriptors.
    template<typename _Config>
    concept partitioned_table_config =
        has_partitions<_Config>::value;

    // structural_col_merge_table_config
    //   concept: constrains configs with structural column merges.
    template<typename _Config>
    concept structural_col_merge_table_config =
        has_col_merges<_Config>::value;

    // structural_col_split_table_config
    //   concept: constrains configs with structural column splits.
    template<typename _Config>
    concept structural_col_split_table_config =
        has_col_splits<_Config>::value;

    // structural_row_merge_table_config
    //   concept: constrains configs with structural row merges.
    template<typename _Config>
    concept structural_row_merge_table_config =
        has_row_merges<_Config>::value;

    // structural_row_split_table_config
    //   concept: constrains configs with structural row splits.
    template<typename _Config>
    concept structural_row_split_table_config =
        has_row_splits<_Config>::value;

    // layout_table_config
    //   concept: constrains configs with merged or split cell layout features.
    template<typename _Config>
    concept layout_table_config =
        ( has_spans<_Config>::value ||
          has_splits<_Config>::value );

    // structural_layout_table_config
    //   concept: constrains configs with structural row/column merge/split features.
    template<typename _Config>
    concept structural_layout_table_config =
        ( has_col_merges<_Config>::value ||
          has_col_splits<_Config>::value ||
          has_row_merges<_Config>::value ||
          has_row_splits<_Config>::value );

    // advanced_table_config_type
    //   concept: constrains configs that are not basic.
    template<typename _Config>
    concept advanced_table_config_type =
        table_config_type<_Config> &&
        !basic_table_config_type<_Config>;


    // =========================================================================
    // V.   FIXED-DIMENSION CELL CLASSIFICATION CONCEPTS
    // =========================================================================

    // data_cell_position
    //   concept: constrains compile-time cell coordinates that classify as data.
    template<std::size_t _Row,
             std::size_t _Col,
             std::size_t _TotalRows,
             std::size_t _TotalCols,
             typename    _Config>
    concept data_cell_position =
        ( cell_position<_Row, _Col, _TotalRows, _TotalCols, _Config>::region ==
          cell_region::data );

    // header_cell_position
    //   concept: constrains compile-time cell coordinates that classify as header.
    template<std::size_t _Row,
             std::size_t _Col,
             std::size_t _TotalRows,
             std::size_t _TotalCols,
             typename    _Config>
    concept header_cell_position =
        cell_position<_Row, _Col, _TotalRows, _TotalCols, _Config>::is_header_cell;

    // split_cell_position
    //   concept: constrains compile-time cell coordinates targeted by a split.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Config>
    concept split_cell_position =
        is_split_cell<_Row, _Col, _Config>::value;

    // partitioned_cell_position
    //   concept: constrains compile-time cell coordinates contained in a partition.
    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Config>
    concept partitioned_cell_position =
        cell_partition<_Row, _Col, _Config>::found;


    // =========================================================================
    // VI.  DIMENSION / AGGREGATE CONCEPTS
    // =========================================================================

    // fixed_table_shape
    //   concept: constrains fixed-dimension table shape computations.
    template<std::size_t _Rows,
             std::size_t _Cols,
             typename    _Config>
    concept fixed_table_shape =
        table_config_type<_Config> &&
        ( table_dimensions<_Rows, _Cols, _Config>::total_rows == _Rows ) &&
        ( table_dimensions<_Rows, _Cols, _Config>::total_cols == _Cols );

    // data_bearing_fixed_table_shape
    //   concept: constrains fixed-dimension tables with at least one data cell.
    template<std::size_t _Rows,
             std::size_t _Cols,
             typename    _Config>
    concept data_bearing_fixed_table_shape =
        fixed_table_shape<_Rows, _Cols, _Config> &&
        ( table_dimensions<_Rows, _Cols, _Config>::data_cells > 0 );

    // =========================================================================
    // VII. TABLE TYPE IDENTITY CONCEPTS
    // =========================================================================
    //
    // These concepts operate on the TABLE TYPE itself (a table<> instantiation
    // or any type that satisfies the structural interface), not on a config.
    // They require table.hpp in addition to table_traits.hpp.
    //
    // STATIC vs DYNAMIC:
    //   static_table_type   - compile-time fixed dimensions (table<T,Rows,Cols>)
    //   dynamic_table_type  - runtime dimensions (table<T,table_dynamic,...>
    //                         or database_table<>)
    //   any_table_type      - either form (the primary "is this a table?" gate)
    //
    // Structural mutability (from table_traits.hpp, section VII):
    //   structurally_immutable_table_type - fixed shape, no modifiers (static)
    //   shape_modifiable_table_type       - exposes resize/add_row/etc. (dynamic)
    //   value_mutable_table_type          - immutable shape, mutable cells (static)
    //

    // any_table_type
    //   concept: constrains any type satisfying the djinterp table interface,
    // static or dynamic.  The primary "is this a djinterp table?" gate.
    template<typename _Type>
    concept any_table_type =
        is_table_type<_Type>::value;

    // static_table_type
    //   concept: constrains types that are fixed-dimension djinterp tables:
    // expose compile-time num_rows, num_cols, total_cells, and static
    // rows()/cols(). Only satisfied by table<T, Rows, Cols, Config> with
    // concrete (non-sentinel) extents.
    template<typename _Type>
    concept static_table_type =
        is_static_table_type<_Type>::value;

    // dynamic_table_type
    //   concept: constrains types that are runtime-dimension djinterp tables:
    // expose instance rows()/cols()/cell() and config_type, but lack
    // compile-time dimension constants. Satisfied by the three dynamic
    // table<> partial specializations and by database_table<>.
    template<typename _Type>
    concept dynamic_table_type =
        is_dynamic_table_type<_Type>::value;

    // homogeneous_table_type
    //   concept: constrains table types with contiguous homogeneous storage
    // (expose data()). True for all table<> specializations (both static
    // and dynamic via std::vector). False for heterogeneous typed_table<>
    // and database_table<>.
    template<typename _Type>
    concept homogeneous_table_type =
        is_homogeneous_table<_Type>::value;

    // structurally_immutable_table_type
    //   concept: constrains types with a fixed compile-time shape and no
    // shape-modifying operations. Cell values may still be mutable.
    // Implies static_table_type.
    template<typename _Type>
    concept structurally_immutable_table_type =
        is_structurally_immutable<_Type>::value;

    // shape_modifiable_table_type
    //   concept: constrains types exposing at least one shape-modifier
    // (resize, add_row, remove_row, add_col, remove_col). All dynamic
    // table<> forms satisfy this; static tables do not.
    template<typename _Type>
    concept shape_modifiable_table_type =
        has_shape_modifiers<_Type>::value;

    // value_mutable_table_type
    //   concept: constrains types that are structurally immutable yet
    // provide mutable element access. This is the canonical static table
    // contract: fixed shape, modifiable contents.
    template<typename _Type>
    concept value_mutable_table_type =
        is_value_mutable<_Type>::value;

    // configured_table_type
    //   concept: constrains any table type that carries a non-empty
    // config (i.e. has_table_config<T> is true). Works for both static
    // and dynamic tables since config_type is always a compile-time param.
    template<typename _Type>
    concept configured_table_type =
        any_table_type<_Type> &&
        has_table_config<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_TABLE_CONCEPTS_