/******************************************************************************
* djinterp [container]                                        table_layout.hpp
*
* djinterp table layout module:
*   Provides cell merge, split, and partition accessor methods for the
* fixed-dimension table class. These are convenience wrappers around the
* table_config_class and table_dimensions traits — include this header
* after table.hpp to extend a table type with layout query accessors.
*
*   CONTENTS:
*     I.   Table layout queries (merge/split/partition accessors)
*
*   All queries are free-function templates operating on any type that
* exposes a config_type alias and a dimensions type alias (i.e. any
* instantiation of table<T, Rows, Cols, Config>).
*
*   PORTABILITY:
*   Compatible with C++11 and later. Uses portable trait access patterns.
*
* path:      /inc/djinterp/container/table/table_layout.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.06.20
******************************************************************************/

#ifndef DJINTERP_TABLE_LAYOUT_
#define DJINTERP_TABLE_LAYOUT_ 1

#include "..\..\djinterp.h"
#include "..\meta\table_traits.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   TABLE LAYOUT QUERIES
    // =========================================================================
    //
    // Free-function templates providing merge, split, and partition queries
    // for any table type exposing config_type and dimensions aliases. These
    // were formerly member functions of the table class; they are separated
    // here to keep the core table class focused on container semantics.
    //

    // -----------------------------------------------------------------
    //  partition queries
    // -----------------------------------------------------------------

    // table_partition_count
    //   function: returns the number of partition descriptors.
    template<typename _Table>
    static D_CONSTEXPR typename _Table::size_type
    table_partition_count() noexcept
    {
        return _Table::dimensions::partition_count;
    }

    // table_has_partitions
    //   function: returns whether the config defines any partitions.
    template<typename _Table>
    static D_CONSTEXPR bool
    table_has_partitions() noexcept
    {
        return (_Table::dimensions::partition_count > 0);
    }


    // -----------------------------------------------------------------
    //  structural column/row merge and split queries
    // -----------------------------------------------------------------

    // table_col_merge_count
    //   function: returns the number of structural column merges.
    template<typename _Table>
    static D_CONSTEXPR typename _Table::size_type
    table_col_merge_count() noexcept
    {
        return _Table::dimensions::col_merge_count;
    }

    // table_col_split_count
    //   function: returns the number of structural column splits.
    template<typename _Table>
    static D_CONSTEXPR typename _Table::size_type
    table_col_split_count() noexcept
    {
        return _Table::dimensions::col_split_count;
    }

    // table_row_merge_count
    //   function: returns the number of structural row merges.
    template<typename _Table>
    static D_CONSTEXPR typename _Table::size_type
    table_row_merge_count() noexcept
    {
        return _Table::dimensions::row_merge_count;
    }

    // table_row_split_count
    //   function: returns the number of structural row splits.
    template<typename _Table>
    static D_CONSTEXPR typename _Table::size_type
    table_row_split_count() noexcept
    {
        return _Table::dimensions::row_split_count;
    }

    // table_has_structural_merges
    //   function: returns whether the config defines any structural
    // column or row merges.
    template<typename _Table>
    static D_CONSTEXPR bool
    table_has_structural_merges() noexcept
    {
        return ( (_Table::dimensions::col_merge_count > 0) ||
                 (_Table::dimensions::row_merge_count > 0) );
    }

    // table_has_structural_splits
    //   function: returns whether the config defines any structural
    // column or row splits.
    template<typename _Table>
    static D_CONSTEXPR bool
    table_has_structural_splits() noexcept
    {
        return ( (_Table::dimensions::col_split_count > 0) ||
                 (_Table::dimensions::row_split_count > 0) );
    }


    // -----------------------------------------------------------------
    //  cell merge / split config queries
    // -----------------------------------------------------------------

    // table_has_merged_cells
    //   function: returns whether the config defines any merged spans.
    template<typename _Table>
    static D_CONSTEXPR bool
    table_has_merged_cells() noexcept
    {
        return table_config_class<typename _Table::config_type>::has_merged_cells;
    }

    // table_has_split_cells
    //   function: returns whether the config defines any split
    // descriptors.
    template<typename _Table>
    static D_CONSTEXPR bool
    table_has_split_cells() noexcept
    {
        return table_config_class<typename _Table::config_type>::has_split_cells;
    }

    // table_has_layout_features
    //   function: returns whether the config defines any merges or
    // splits.
    template<typename _Table>
    static D_CONSTEXPR bool
    table_has_layout_features() noexcept
    {
        return table_config_class<typename _Table::config_type>::has_layout_features;
    }

    // table_merge_count
    //   function: returns the number of merged span descriptors.
    template<typename _Table>
    static D_CONSTEXPR typename _Table::size_type
    table_merge_count() noexcept
    {
        return table_config_class<typename _Table::config_type>::merge_count;
    }

    // table_split_count
    //   function: returns the number of split descriptors.
    template<typename _Table>
    static D_CONSTEXPR typename _Table::size_type
    table_split_count() noexcept
    {
        return table_config_class<typename _Table::config_type>::split_count;
    }

    // table_is_basic
    //   function: returns whether the config has no special features.
    template<typename _Table>
    static D_CONSTEXPR bool
    table_is_basic() noexcept
    {
        return table_config_class<typename _Table::config_type>::is_basic;
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_TABLE_LAYOUT_
