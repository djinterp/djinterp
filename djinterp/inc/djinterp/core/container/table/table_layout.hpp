/******************************************************************************
* djinterp [container]                                         table_layout.hpp
*
*   The LAYOUT overlay Gamma of a table -- the third layer of the model
* T = (T_, I_T, Gamma) (containers.tex, The table).  Everything in table_shape
* concerns the ATOMIC table, one value per index; this header adds the optional
* cover that groups atomic positions into LAYOUT CELLS, the visible cells of a
* rendered table, in which a span of atomic positions reads as one.  It is an
* OVERLAY in the sense of Overlays: a discipline over the indexed tuple, not a
* change to it.
*
*   THE FORMAL OBJECTS, at k = 2 rectangular:
*     - a REGION is a box of atomic positions, [row0, row0+rows) x [col0,
*       col0+cols); a layout cell's extent |R_C| is rows*cols.
*     - a MERGE is a region with |R_C| > 1; layout-aware access returns its one
*       value, so T[i] = T[j] for all i, j in the region.
*     - the ANCHOR names the cell once: anchor(C) = min_lex R_C = (row0, col0)
*       for a box; every position reads as its anchor,
*       T[i] = T[anchor(cell_T(i))].
*     - a COVER partitions the atomic domain -- every position in exactly one
*       cell.  Here a cover is given by its MERGES alone; every position no merge
*       covers is its own singleton cell (the trivial cover Gamma_0 on the rest),
*       so the partition is valid exactly when the declared merges are within
*       bounds and pairwise DISJOINT.
*     - a SPLIT refines a cell into s >= 2 pieces partitioning its region.  The
*       descriptor and its partition check are here; splitting a SINGLETON (which
*       needs the atomic domain refined by a projection pi) is deferred.
*
*   TWO INCARNATIONS.  A compile-time layout<Regions...> (the type the builder
* computes from merged_cell declarations) and a runtime_layout value (the parser
* accumulates from a spanning text grid) share the same vocabulary -- owner,
* anchor, extent, validity -- so the two front ends describe one overlay.
*
*   RECTANGULAR, k = 2, FOR NOW, as table_shape; the region generalises to a
* k-box (two corner tuples) without disturbing the surface.
*
*   PORTABILITY:
*   C++11 baseline (regions are std::size_t-parameterised; the runtime layout is
* a plain std::vector of boxes).  The _v shorthands are C++14; concepts C++20.
*
*
* path:      /inc/djinterp/core/container/table/table_layout.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.14
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    region                       (a rectangular box of atomic positions)
II.   region relations             (contains / overlap / within)
III.  layout                       (compile-time cover: the declared merges)
IV.   layout queries               (owner_of / anchor / validity)
V.    split                        (compile-time cell refinement + partition check)
VI.   runtime region + layout      (the value-level overlay)
VII.  detection traits             (is_region / is_layout / is_split)
VIII. concepts                     (C++20 analogs)
*/

#ifndef DJINTERP_CONTAINER_TABLE_LAYOUT_
#define DJINTERP_CONTAINER_TABLE_LAYOUT_ 1

// std
#include <cstddef>
#include <type_traits>
#include <vector>
// djinterp
#include "../../../config/core/container/table/cfg_table.h"
#include "../../djinterp.hpp"   // NS_*, D_CONSTEXPR, D_NODISCARD, clean_t, D_ENV_*


NS_DJINTERP


// ===========================================================================
// I.   region
// ===========================================================================

// region
//   type: a rectangular region of atomic positions -- the span of a layout cell.
// Covers the box [_Row0, _Row0+_Rows) x [_Col0, _Col0+_Cols); its extent is the
// number of atomic positions it holds.  A region is non-empty by construction.
//
//   _Row0, _Col0: the top-left (lexicographically least) atomic position -- the
//                 cell's ANCHOR.
//   _Rows, _Cols: the box shape; the cell is MERGED along a coordinate when that
//                 coordinate's span exceeds one.
template<std::size_t _Row0,
         std::size_t _Col0,
         std::size_t _Rows,
         std::size_t _Cols>
struct region
{
    static_assert(( (_Rows > 0) && (_Cols > 0) ),
                  "region: a layout cell's span must be non-empty.");

    static D_CONSTEXPR std::size_t row0 = _Row0;
    static D_CONSTEXPR std::size_t col0 = _Col0;
    static D_CONSTEXPR std::size_t rows = _Rows;
    static D_CONSTEXPR std::size_t cols = _Cols;

    // extent -- |R_C|, the atomic positions the cell spans.
    static D_CONSTEXPR std::size_t extent = (_Rows * _Cols);

    // anchor -- min_lex R_C, the (row, col) the cell is named by.
    static D_CONSTEXPR std::size_t anchor_row = _Row0;
    static D_CONSTEXPR std::size_t anchor_col = _Col0;

    // is_merge -- whether the cell spans more than one atomic position.
    static D_CONSTEXPR bool is_merge = (extent > 1);

    // merged_along_* -- whether the cell spans that coordinate (b_r - a_r + 1 > 1).
    static D_CONSTEXPR bool merged_along_rows = (_Rows > 1);
    static D_CONSTEXPR bool merged_along_cols = (_Cols > 1);
};

// merge
//   type: a region intended as a merged cell -- an alias for region, named for
// the call site (a merged_cell<Rows, Cols, V> declaration places one of these at
// the atomic position it is declared at).  It carries no extra data; whether it
// is truly a merge (extent > 1) is region::is_merge.
template<std::size_t _Row0,
         std::size_t _Col0,
         std::size_t _Rows,
         std::size_t _Cols>
using merge = region<_Row0, _Col0, _Rows, _Cols>;


// ===========================================================================
// II.  region relations
// ===========================================================================

// region_contains
//   trait: whether region _R covers the atomic position (_Row, _Col).
template<typename    _R,
         std::size_t _Row,
         std::size_t _Col>
struct region_contains
    : std::integral_constant<bool,
        ( (_Row >= _R::row0) && (_Row < _R::row0 + _R::rows) &&
          (_Col >= _R::col0) && (_Col < _R::col0 + _R::cols) )>
{};

// regions_overlap
//   trait: whether two regions share any atomic position -- their row ranges and
// their column ranges both intersect.
template<typename _A,
         typename _B>
struct regions_overlap
    : std::integral_constant<bool,
        ( (_A::row0 < _B::row0 + _B::rows) &&
          (_B::row0 < _A::row0 + _A::rows) &&
          (_A::col0 < _B::col0 + _B::cols) &&
          (_B::col0 < _A::col0 + _A::cols) )>
{};

// region_within
//   trait: whether region _Inner lies entirely inside region _Outer.
template<typename _Inner,
         typename _Outer>
struct region_within
    : std::integral_constant<bool,
        ( (_Inner::row0 >= _Outer::row0) &&
          (_Inner::col0 >= _Outer::col0) &&
          (_Inner::row0 + _Inner::rows <= _Outer::row0 + _Outer::rows) &&
          (_Inner::col0 + _Inner::cols <= _Outer::col0 + _Outer::cols) )>
{};


// ===========================================================================
// III. layout
// ===========================================================================

NS_INTERNAL

    // find_owner
    //   trait: the first region of the pack covering (_Row, _Col), or the
    // singleton region at (_Row, _Col) when none does -- the owner function
    // cell_T made total by the trivial cover on the un-merged rest.
    template<std::size_t _Row,
             std::size_t _Col,
             typename... _Regions>
    struct find_owner
    {
        // no declared merge covers it: the position is its own singleton cell
        using type = region<_Row, _Col, 1, 1>;
    };

    template<std::size_t _Row,
             std::size_t _Col,
             typename    _Head,
             typename... _Tail>
    struct find_owner<_Row, _Col, _Head, _Tail...>
    {
        using type =
            typename std::conditional<
                region_contains<_Head, _Row, _Col>::value,
                _Head,
                typename find_owner<_Row, _Col, _Tail...>::type
            >::type;
    };

NS_END  // internal

// layout
//   type: a compile-time cover, given by its declared merges.  Every atomic
// position a merge does not cover is its own singleton cell, so this is the
// laid-out table's Gamma with the trivial cover filling the rest.  The empty
// layout is the ordinary (un-merged) table, Gamma_0.
//
//   _Regions...: the declared merges (region<>s).
template<typename... _Regions>
struct layout
{
    // merge_count -- the number of declared merges.
    static D_CONSTEXPR std::size_t merge_count = sizeof...(_Regions);

    // has_merges / wears_no_merges -- whether any cell spans more than one
    // position; wears_no_merges is the Gamma_0 (ordinary table) case.
    static D_CONSTEXPR bool has_merges      = (merge_count > 0);
    static D_CONSTEXPR bool wears_no_merges = (merge_count == 0);

    // owner_of -- the layout cell owning atomic position (_Row, _Col): a declared
    // merge that covers it, or the singleton cell there.  Read its anchor_row /
    // anchor_col for the layout-aware access T[i] = T[anchor(cell(i))].
    template<std::size_t _Row,
             std::size_t _Col>
    using owner_of =
        typename internal::find_owner<_Row, _Col, _Regions...>::type;
};

// trivial_layout
//   type: the trivial cover Gamma_0 -- no merges, every position its own cell.
using trivial_layout = layout<>;


// ===========================================================================
// IV.  layout queries
// ===========================================================================

NS_INTERNAL

    // all_within
    //   trait: every region lies within the box {0..H-1} x {0..W-1}.
    template<std::size_t _H,
             std::size_t _W,
             typename... _Regions>
    struct all_within : std::true_type
    {};

    template<std::size_t _H,
             std::size_t _W,
             typename    _R0,
             typename... _Rs>
    struct all_within<_H, _W, _R0, _Rs...>
        : std::integral_constant<bool,
            ( (_R0::row0 + _R0::rows <= _H) &&
              (_R0::col0 + _R0::cols <= _W) &&
              all_within<_H, _W, _Rs...>::value )>
    {};

    // disjoint_from_all
    //   trait: _Head overlaps none of the pack.
    template<typename    _Head,
             typename... _Rest>
    struct disjoint_from_all : std::true_type
    {};

    template<typename    _Head,
             typename    _R0,
             typename... _Rs>
    struct disjoint_from_all<_Head, _R0, _Rs...>
        : std::integral_constant<bool,
            ( !regions_overlap<_Head, _R0>::value &&
              disjoint_from_all<_Head, _Rs...>::value )>
    {};

    // pairwise_disjoint
    //   trait: no two regions in the pack overlap.
    template<typename...>
    struct pairwise_disjoint : std::true_type
    {};

    template<typename    _Head,
             typename... _Rest>
    struct pairwise_disjoint<_Head, _Rest...>
        : std::integral_constant<bool,
            ( disjoint_from_all<_Head, _Rest...>::value &&
              pairwise_disjoint<_Rest...>::value )>
    {};

    // layout_valid_impl
    //   trait: the declared merges of a layout are within an H x W table and
    // pairwise disjoint -- the condition for the merges-plus-singletons cover to
    // partition the atomic domain.
    template<typename    _Layout,
             std::size_t _H,
             std::size_t _W>
    struct layout_valid_impl;

    template<typename... _Regions,
             std::size_t  _H,
             std::size_t  _W>
    struct layout_valid_impl<layout<_Regions...>, _H, _W>
        : std::integral_constant<bool,
            ( all_within<_H, _W, _Regions...>::value &&
              pairwise_disjoint<_Regions...>::value )>
    {};

NS_END  // internal

// layout_valid
//   trait: whether _Layout is a valid cover of an _Height x _Width table -- every
// declared merge within bounds and no two overlapping.  (Positions no merge
// covers are singletons, so disjoint, in-bounds merges are exactly what a valid
// partition needs.)
template<typename    _Layout,
         std::size_t _Height,
         std::size_t _Width>
struct layout_valid
    : internal::layout_valid_impl<clean_t<_Layout>, _Height, _Width>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// layout_valid_v
//   value: shorthand for layout_valid<_Layout, _Height, _Width>::value.
template<typename    _Layout,
         std::size_t _Height,
         std::size_t _Width>
D_CONSTEXPR bool layout_valid_v =
    layout_valid<_Layout, _Height, _Width>::value;
#endif


// ===========================================================================
// V.   split
// ===========================================================================

NS_INTERNAL

    // extent_sum
    //   trait: the total extent of a pack of regions.
    template<typename...>
    struct extent_sum
        : std::integral_constant<std::size_t, 0>
    {};

    template<typename    _R0,
             typename... _Rs>
    struct extent_sum<_R0, _Rs...>
        : std::integral_constant<std::size_t,
            (_R0::extent + extent_sum<_Rs...>::value)>
    {};

    // all_within_region
    //   trait: every piece lies within _Parent.
    template<typename    _Parent,
             typename... _Pieces>
    struct all_within_region : std::true_type
    {};

    template<typename    _Parent,
             typename    _P0,
             typename... _Ps>
    struct all_within_region<_Parent, _P0, _Ps...>
        : std::integral_constant<bool,
            ( region_within<_P0, _Parent>::value &&
              all_within_region<_Parent, _Ps...>::value )>
    {};

NS_END  // internal

// split
//   type: a refinement of a cell -- its region _Parent partitioned into pieces
// _Pieces... (s >= 2 sub-regions).  The descriptor the builder's split_cell and
// the parser's sub-cell grid map onto.  Splitting a cell whose region is already
// a singleton cannot partition it and needs the atomic domain refined by a
// projection pi (containers.tex); that case is deferred.
template<typename    _Parent,
         typename... _Pieces>
struct split
{
    static_assert((sizeof...(_Pieces) >= 2),
                  "split: a refinement partitions a cell into two or more pieces.");

    using parent = _Parent;

    // piece_count -- the number of sub-cells the parent is split into.
    static D_CONSTEXPR std::size_t piece_count = sizeof...(_Pieces);
};

NS_INTERNAL

    // split_valid_impl
    //   trait: the pieces lie within the parent, are pairwise disjoint, and their
    // extents sum to the parent's -- for integer boxes, exactly a partition.
    template<typename _Split>
    struct split_valid_impl;

    template<typename    _Parent,
             typename... _Pieces>
    struct split_valid_impl<split<_Parent, _Pieces...>>
        : std::integral_constant<bool,
            ( all_within_region<_Parent, _Pieces...>::value  &&
              pairwise_disjoint<_Pieces...>::value           &&
              (extent_sum<_Pieces...>::value == _Parent::extent) )>
    {};

NS_END  // internal

// split_valid
//   trait: whether _Split's pieces tile its parent region exactly -- within,
// disjoint, and area-complete.
template<typename _Split>
struct split_valid
    : internal::split_valid_impl<clean_t<_Split>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// split_valid_v
//   value: shorthand for split_valid<_Split>::value.
template<typename _Split>
D_CONSTEXPR bool split_valid_v = split_valid<_Split>::value;
#endif


// ===========================================================================
// VI.  runtime region + layout
// ===========================================================================

// region_value
//   struct: the runtime counterpart of region -- a box the parser accumulates
// from a spanning text grid.  Same box arithmetic (extent, contains, anchor) as
// the compile-time region.
struct region_value
{
    std::size_t row0;
    std::size_t col0;
    std::size_t rows;
    std::size_t cols;

    D_CONSTEXPR region_value() D_NOEXCEPT
        : row0(0),
          col0(0),
          rows(1),
          cols(1)
    {}

    D_CONSTEXPR region_value(
        std::size_t _row0,
        std::size_t _col0,
        std::size_t _rows,
        std::size_t _cols
    ) D_NOEXCEPT
        : row0(_row0),
          col0(_col0),
          rows(_rows),
          cols(_cols)
    {}

    // extent -- |R_C|.
    D_NODISCARD D_CONSTEXPR std::size_t extent() const D_NOEXCEPT
    {
        return (rows * cols);
    }

    // is_merge -- spans more than one atomic position.
    D_NODISCARD D_CONSTEXPR bool is_merge() const D_NOEXCEPT
    {
        return (extent() > 1);
    }

    // contains -- covers the atomic position (_row, _col).
    D_NODISCARD D_CONSTEXPR bool contains(
        std::size_t _row,
        std::size_t _col
    ) const D_NOEXCEPT
    {
        return ( (_row >= row0) && (_row < row0 + rows) &&
                 (_col >= col0) && (_col < col0 + cols) );
    }

    // anchor_row / anchor_col -- min_lex R_C.
    D_NODISCARD D_CONSTEXPR std::size_t anchor_row() const D_NOEXCEPT
    {
        return row0;
    }

    D_NODISCARD D_CONSTEXPR std::size_t anchor_col() const D_NOEXCEPT
    {
        return col0;
    }
};

// regions_overlap (runtime)
//   function: whether two runtime regions share any atomic position.
D_NODISCARD D_CONSTEXPR inline bool
regions_overlap_rt(
    const region_value& _a,
    const region_value& _b
) D_NOEXCEPT
{
    return ( (_a.row0 < _b.row0 + _b.rows) &&
             (_b.row0 < _a.row0 + _a.rows) &&
             (_a.col0 < _b.col0 + _b.cols) &&
             (_b.col0 < _a.col0 + _a.cols) );
}

// runtime_layout
//   class: the value-level cover -- the declared merges, with singletons implied
// on the rest, exactly as the compile-time layout.  The parser adds a merge per
// spanning cell it recognises; a consumer reads owner_of / anchor.
class runtime_layout
{
public:
    using merge_store = std::vector<region_value>;

    runtime_layout()
        : m_merges()
    {}

    // add_merge -- record a merged cell spanning _rows x _cols from (_row0, _col0).
    void add_merge(
        std::size_t _row0,
        std::size_t _col0,
        std::size_t _rows,
        std::size_t _cols
    )
    {
        m_merges.push_back(region_value(_row0, _col0, _rows, _cols));

        return;
    }

    // has_merges / wears_no_merges -- the Gamma vs Gamma_0 distinction.
    D_NODISCARD bool has_merges() const D_NOEXCEPT
    {
        return (!m_merges.empty());
    }

    D_NODISCARD bool wears_no_merges() const D_NOEXCEPT
    {
        return m_merges.empty();
    }

    // merge_count -- the number of declared merges.
    D_NODISCARD std::size_t merge_count() const D_NOEXCEPT
    {
        return m_merges.size();
    }

    // owner_of -- the layout cell owning (_row, _col): a declared merge that
    // covers it, or the singleton cell there.
    D_NODISCARD region_value owner_of(
        std::size_t _row,
        std::size_t _col
    ) const
    {
        // return the first declared merge that covers the position
        for (const region_value& _m : m_merges)
        {
            if (_m.contains(_row, _col))
            {
                return _m;
            }
        }

        // none does: the position is its own singleton cell
        return region_value(_row, _col, 1, 1);
    }

    // valid -- every declared merge lies within an _height x _width table and no
    // two overlap: the runtime cover-validity check.
    D_NODISCARD bool valid(
        std::size_t _height,
        std::size_t _width
    ) const
    {
        const std::size_t n = m_merges.size();

        // every merge must lie within the table bounds
        for (std::size_t i = 0; i < n; ++i)
        {
            const region_value& _m = m_merges[i];

            if ( (_m.row0 + _m.rows > _height) ||
                 (_m.col0 + _m.cols > _width) )
            {
                return false;
            }
        }

        // no two merges may overlap
        for (std::size_t i = 0; i < n; ++i)
        {
            for (std::size_t j = i + 1; j < n; ++j)
            {
                if (regions_overlap_rt(m_merges[i], m_merges[j]))
                {
                    return false;
                }
            }
        }

        return true;
    }

    // merges -- the declared merges.
    D_NODISCARD const merge_store& merges() const D_NOEXCEPT
    {
        return m_merges;
    }

private:
    merge_store m_merges;
};


// ===========================================================================
// VII. detection traits
// ===========================================================================

NS_INTERNAL

    template<typename _Type>
    struct is_region_impl : std::false_type
    {};

    template<std::size_t _R,
             std::size_t _C,
             std::size_t _Rows,
             std::size_t _Cols>
    struct is_region_impl<region<_R, _C, _Rows, _Cols>> : std::true_type
    {};

    template<typename _Type>
    struct is_layout_impl : std::false_type
    {};

    template<typename... _Regions>
    struct is_layout_impl<layout<_Regions...>> : std::true_type
    {};

    template<typename _Type>
    struct is_split_impl : std::false_type
    {};

    template<typename    _Parent,
             typename... _Pieces>
    struct is_split_impl<split<_Parent, _Pieces...>> : std::true_type
    {};

NS_END  // internal

// is_region / is_layout / is_split
//   traits: true iff _Type (after stripping cv/ref) is the named layout type.
template<typename _Type>
struct is_region : internal::is_region_impl<clean_t<_Type>>
{};

template<typename _Type>
struct is_layout : internal::is_layout_impl<clean_t<_Type>>
{};

template<typename _Type>
struct is_split : internal::is_split_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Type>
D_CONSTEXPR bool is_region_v = is_region<_Type>::value;

template<typename _Type>
D_CONSTEXPR bool is_layout_v = is_layout<_Type>::value;

template<typename _Type>
D_CONSTEXPR bool is_split_v = is_split<_Type>::value;
#endif


// ===========================================================================
// VIII. concepts   (C++20 analogs)
// ===========================================================================

#if D_INTERNAL_TABLE_CONCEPTS

template<typename _Type>
concept Region = is_region_v<_Type>;

template<typename _Type>
concept Layout = is_layout_v<_Type>;

template<typename _Type>
concept Split = is_split_v<_Type>;

#endif  // D_INTERNAL_TABLE_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TABLE_LAYOUT_
