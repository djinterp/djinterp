/*******************************************************************************
* djinterp [core]                                               table_layout.hpp
*
*   The LAYOUT overlay Gamma of a table -- the third layer of the model
* T = (T_, I_T, Gamma) (containers.tex, The table).  Everything in table_shape
* concerns the ATOMIC table, one value per index; this header adds the
* optional
* cover that groups atomic positions into LAYOUT CELLS, the visible cells of a
* rendered table, in which a span of atomic positions reads as one.  It is an
* OVERLAY in the sense of Overlays: a discipline over the indexed tuple, not a
* change to it.
*
*   THE FORMAL OBJECTS, at k = 2 rectangular:
*     - a REGION is a box of atomic positions, [row0, row0+rows) x [col0,
*       col0+cols); a layout cell's extent |R_C| is rows*cols.
*     - a MERGE is a region with |R_C| > 1; layout-aware access returns its
*   one
*       value, so T[i] = T[j] for all i, j in the region.
*     - the ANCHOR names the cell once: anchor(C) = min_lex R_C = (row0, col0)
*       for a box; every position reads as its anchor,
*       T[i] = T[anchor(cell_T(i))].
*     - a COVER partitions the atomic domain -- every position in exactly one
*       cell. Here a cover is given by its MERGES alone; every position no
*     merge
*       covers is its own singleton cell (the trivial cover Gamma_0 on the
*     rest),
*       so the partition is valid exactly when the declared merges are within
*       bounds and pairwise DISJOINT.
*     - a SPLIT refines a cell into s >= 2 pieces partitioning its region. The
*       descriptor and its partition check are here; splitting a SINGLETON
*     (which
*       needs the atomic domain refined by a projection pi) is deferred.
*
*   TWO INCARNATIONS.  A compile-time layout<Regions...> (the type the builder
* computes from merged_cell declarations) and a runtime_layout value (the
* parser
* accumulates from a spanning text grid) share the same vocabulary -- owner,
* anchor, extent, validity -- so the two front ends describe one overlay.
*
*   RECTANGULAR, k = 2, FOR NOW, as table_shape; the region generalises to a
* k-box (two corner tuples) without disturbing the surface.
*
*   PORTABILITY:
*   C++11 baseline (regions are std::size_t-parameterised; the runtime layout
* is
* a plain std::vector of boxes).  The _v shorthands are C++14; concepts C++20.
*
*
* path:      /inc/djinterp/core/container/table/table_layout.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    region                       (a rectangular box of atomic positions)
      --------------------------------------------------------------------

II.   region relations             (contains / overlap / within)
      ----------------------------------------------------------

III.  layout                       (compile-time cover: the declared merges)
      ----------------------------------------------------------------------

IV.   layout queries               (owner_of / anchor / validity)
      -----------------------------------------------------------

V.    split                        (compile-time cell refinement + partition check)
      -----------------------------------------------------------------------------

VI.   runtime region + layout      (the value-level overlay)
      ------------------------------------------------------

VII.  detection traits             (is_region / is_layout / is_split)
      ---------------------------------------------------------------

VIII. concepts                     (C++20 analogs)
      --------------------------------------------
*/

#ifndef DJINTERP_CONTAINER_TABLE_TABLE_LAYOUT_HPP
#define DJINTERP_CONTAINER_TABLE_TABLE_LAYOUT_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"   // NS_*, D_CONSTEXPR, D_NODISCARD, clean_t, D_ENV_*
#include "../../../config/core/container/table/cfg_table.h"


NS_DJINTERP


// ===========================================================================
// I.   region
// ===========================================================================

// region
//   type: a rectangular region of atomic positions -- the span of a layout
// cell. Covers the box [Row0, Row0+Rows) x [Col0, Col0+Cols); its extent
// is the number of atomic positions it holds. A region is non-empty by
// construction.
//
//   Row0, Col0: the top-left (lexicographically least) atomic position --
// the
//                 cell's ANCHOR.
//   Rows, Cols: the box shape; the cell is MERGED along a coordinate when
// that
//                 coordinate's span exceeds one.
template<std::size_t Row0,
         std::size_t Col0,
         std::size_t Rows,
         std::size_t Cols>
struct region
{
    static_assert(( (Rows > 0) && (Cols > 0) ),
                  "region: a layout cell's span must be non-empty.");

    static D_CONSTEXPR std::size_t row0 = Row0;
    static D_CONSTEXPR std::size_t col0 = Col0;
    static D_CONSTEXPR std::size_t rows = Rows;
    static D_CONSTEXPR std::size_t cols = Cols;

    // extent -- |R_C|, the atomic positions the cell spans.
    static D_CONSTEXPR std::size_t extent = (Rows * Cols);

    // anchor -- min_lex R_C, the (row, col) the cell is named by.
    static D_CONSTEXPR std::size_t anchor_row = Row0;
    static D_CONSTEXPR std::size_t anchor_col = Col0;

    // is_merge -- whether the cell spans more than one atomic position.
    static D_CONSTEXPR bool is_merge = (extent > 1);

    // merged_along_* -- whether the cell spans that coordinate (b_r - a_r + 1
    // > 1).
    static D_CONSTEXPR bool merged_along_rows = (Rows > 1);
    static D_CONSTEXPR bool merged_along_cols = (Cols > 1);
};

// merge
//   type: a region intended as a merged cell -- an alias for region, named for
// the call site (a merged_cell<Rows, Cols, V> declaration places one of these
// at the atomic position it is declared at). It carries no extra data; whether
// it is truly a merge (extent > 1) is region::is_merge.
template<std::size_t Row0,
         std::size_t Col0,
         std::size_t Rows,
         std::size_t Cols>
using merge = region<Row0, Col0, Rows, Cols>;


// ===========================================================================
// II.  region relations
// ===========================================================================

// region_contains
//   trait: whether region R covers the atomic position (Row, Col).
template<typename    R,
         std::size_t Row,
         std::size_t Col>
struct region_contains
    : std::integral_constant<bool,
        ( (Row >= R::row0) && (Row < R::row0 + R::rows) &&
          (Col >= R::col0) && (Col < R::col0 + R::cols) )>
{};

// regions_overlap
//   trait: whether two regions share any atomic position -- their row ranges
// and their column ranges both intersect.
template<typename A,
         typename B>
struct regions_overlap
    : std::integral_constant<bool,
        ( (A::row0 < B::row0 + B::rows) &&
          (B::row0 < A::row0 + A::rows) &&
          (A::col0 < B::col0 + B::cols) &&
          (B::col0 < A::col0 + A::cols) )>
{};

// region_within
//   trait: whether region Inner lies entirely inside region Outer.
template<typename Inner,
         typename Outer>
struct region_within
    : std::integral_constant<bool,
        ( (Inner::row0 >= Outer::row0) &&
          (Inner::col0 >= Outer::col0) &&
          (Inner::row0 + Inner::rows <= Outer::row0 + Outer::rows) &&
          (Inner::col0 + Inner::cols <= Outer::col0 + Outer::cols) )>
{};


// ===========================================================================
// III. layout
// ===========================================================================

NS_INTERNAL

    // find_owner
    //   trait: the first region of the pack covering (Row, Col), or the
    // singleton region at (Row, Col) when none does -- the owner function
    // cell_T made total by the trivial cover on the un-merged rest.
    template<std::size_t Row,
             std::size_t Col,
             typename... Regions>
    struct find_owner
    {
        // no declared merge covers it: the position is its own singleton cell
        using type = region<Row, Col, 1, 1>;
    };

    template<std::size_t Row,
             std::size_t Col,
             typename    Head,
             typename... Tail>
    struct find_owner<Row, Col, Head, Tail...>
    {
        using type =
            typename std::conditional<
                region_contains<Head, Row, Col>::value,
                Head,
                typename find_owner<Row, Col, Tail...>::type
            >::type;
    };

NS_END  // internal

// layout
//   type: a compile-time cover, given by its declared merges. Every atomic
// position a merge does not cover is its own singleton cell, so this is the
// laid-out table's Gamma with the trivial cover filling the rest. The empty
// layout is the ordinary (un-merged) table, Gamma_0.
//
//   Regions...: the declared merges (region<>s).
template<typename... Regions>
struct layout
{
    // merge_count -- the number of declared merges.
    static D_CONSTEXPR std::size_t merge_count = sizeof...(Regions);

    // has_merges / wears_no_merges -- whether any cell spans more than one
    // position; wears_no_merges is the Gamma_0 (ordinary table) case.
    static D_CONSTEXPR bool has_merges      = (merge_count > 0);
    static D_CONSTEXPR bool wears_no_merges = (merge_count == 0);

    // owner_of -- the layout cell owning atomic position (Row, Col): a
    // declared merge that covers it, or the singleton cell there. Read its
    // anchor_row / anchor_col for the layout-aware access T[i] =
    // T[anchor(cell(i))].
    template<std::size_t Row,
             std::size_t Col>
    using owner_of =
        typename internal::find_owner<Row, Col, Regions...>::type;
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
    template<std::size_t H,
             std::size_t W,
             typename... Regions>
    struct all_within : std::true_type
    {};

    template<std::size_t H,
             std::size_t W,
             typename    R0,
             typename... Rs>
    struct all_within<H, W, R0, Rs...>
        : std::integral_constant<bool,
            ( (R0::row0 + R0::rows <= H) &&
              (R0::col0 + R0::cols <= W) &&
              all_within<H, W, Rs...>::value )>
    {};

    // disjoint_from_all
    //   trait: Head overlaps none of the pack.
    template<typename    Head,
             typename... Rest>
    struct disjoint_from_all : std::true_type
    {};

    template<typename    Head,
             typename    R0,
             typename... Rs>
    struct disjoint_from_all<Head, R0, Rs...>
        : std::integral_constant<bool,
            ( !regions_overlap<Head, R0>::value &&
              disjoint_from_all<Head, Rs...>::value )>
    {};

    // pairwise_disjoint
    //   trait: no two regions in the pack overlap.
    template<typename...>
    struct pairwise_disjoint : std::true_type
    {};

    template<typename    Head,
             typename... Rest>
    struct pairwise_disjoint<Head, Rest...>
        : std::integral_constant<bool,
            ( disjoint_from_all<Head, Rest...>::value &&
              pairwise_disjoint<Rest...>::value )>
    {};

    // layout_valid_impl
    //   trait: the declared merges of a layout are within an H x W table and
    // pairwise disjoint -- the condition for the merges-plus-singletons cover
    // to partition the atomic domain.
    template<typename    Layout,
             std::size_t H,
             std::size_t W>
    struct layout_valid_impl;

    template<typename... Regions,
             std::size_t  H,
             std::size_t  W>
    struct layout_valid_impl<layout<Regions...>, H, W>
        : std::integral_constant<bool,
            ( all_within<H, W, Regions...>::value &&
              pairwise_disjoint<Regions...>::value )>
    {};

NS_END  // internal

// layout_valid
//   trait: whether Layout is a valid cover of an Height x Width table --
// every declared merge within bounds and no two overlapping. (Positions no
// merge covers are singletons, so disjoint, in-bounds merges are exactly what
// a valid partition needs.)
template<typename    Layout,
         std::size_t Height,
         std::size_t Width>
struct layout_valid
    : internal::layout_valid_impl<clean_t<Layout>, Height, Width>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// layout_valid_v
//   value: shorthand for layout_valid<Layout, Height, Width>::value.
template<typename    Layout,
         std::size_t Height,
         std::size_t Width>
D_CONSTEXPR bool layout_valid_v =
    layout_valid<Layout, Height, Width>::value;
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

    template<typename    R0,
             typename... Rs>
    struct extent_sum<R0, Rs...>
        : std::integral_constant<std::size_t,
            (R0::extent + extent_sum<Rs...>::value)>
    {};

    // all_within_region
    //   trait: every piece lies within Parent.
    template<typename    Parent,
             typename... Pieces>
    struct all_within_region : std::true_type
    {};

    template<typename    Parent,
             typename    P0,
             typename... Ps>
    struct all_within_region<Parent, P0, Ps...>
        : std::integral_constant<bool,
            ( region_within<P0, Parent>::value &&
              all_within_region<Parent, Ps...>::value )>
    {};

NS_END  // internal

// split
//   type: a refinement of a cell -- its region Parent partitioned into pieces
// Pieces... (s >= 2 sub-regions). The descriptor the builder's split_cell and
// the parser's sub-cell grid map onto. Splitting a cell whose region is
// already
// a singleton cannot partition it and needs the atomic domain refined by a
// projection pi (containers.tex); that case is deferred.
template<typename    Parent,
         typename... Pieces>
struct split
{
    static_assert((sizeof...(Pieces) >= 2),
                  "split: a refinement partitions a cell into two or more pieces.");

    using parent = Parent;

    // piece_count -- the number of sub-cells the parent is split into.
    static D_CONSTEXPR std::size_t piece_count = sizeof...(Pieces);
};

NS_INTERNAL

    // split_valid_impl
    //   trait: the pieces lie within the parent, are pairwise disjoint, and
    // their extents sum to the parent's -- for integer boxes, exactly a
    // partition.
    template<typename Split>
    struct split_valid_impl;

    template<typename    Parent,
             typename... Pieces>
    struct split_valid_impl<split<Parent, Pieces...>>
        : std::integral_constant<bool,
            ( all_within_region<Parent, Pieces...>::value  &&
              pairwise_disjoint<Pieces...>::value           &&
              (extent_sum<Pieces...>::value == Parent::extent) )>
    {};

NS_END  // internal

// split_valid
//   trait: whether Split's pieces tile its parent region exactly -- within,
// disjoint, and area-complete.
template<typename Split>
struct split_valid
    : internal::split_valid_impl<clean_t<Split>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// split_valid_v
//   value: shorthand for split_valid<Split>::value.
template<typename Split>
D_CONSTEXPR bool split_valid_v = split_valid<Split>::value;
#endif


// ===========================================================================
// VI.  runtime region + layout
// ===========================================================================

// region_value
//   struct: the runtime counterpart of region -- a box the parser accumulates
// from a spanning text grid. Same box arithmetic (extent, contains, anchor) as
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
//   class: the value-level cover -- the declared merges, with singletons
// implied on the rest, exactly as the compile-time layout. The parser adds a
// merge per spanning cell it recognises; a consumer reads owner_of / anchor.
class runtime_layout
{
public:
    using merge_store = std::vector<region_value>;

    runtime_layout()
        : m_merges()
    {}

    // add_merge -- record a merged cell spanning _rows x _cols from (_row0,
    // _col0).
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

    // valid -- every declared merge lies within an _height x _width table and
    // no two overlap: the runtime cover-validity check.
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

    template<typename Type>
    struct is_region_impl : std::false_type
    {};

    template<std::size_t R,
             std::size_t C,
             std::size_t Rows,
             std::size_t Cols>
    struct is_region_impl<region<R, C, Rows, Cols>> : std::true_type
    {};

    template<typename Type>
    struct is_layout_impl : std::false_type
    {};

    template<typename... Regions>
    struct is_layout_impl<layout<Regions...>> : std::true_type
    {};

    template<typename Type>
    struct is_split_impl : std::false_type
    {};

    template<typename    Parent,
             typename... Pieces>
    struct is_split_impl<split<Parent, Pieces...>> : std::true_type
    {};

NS_END  // internal

// is_region / is_layout / is_split
//   traits: true iff Type (after stripping cv/ref) is the named layout type.
template<typename Type>
struct is_region : internal::is_region_impl<clean_t<Type>>
{};

template<typename Type>
struct is_layout : internal::is_layout_impl<clean_t<Type>>
{};

template<typename Type>
struct is_split : internal::is_split_impl<clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
D_CONSTEXPR bool is_region_v = is_region<Type>::value;

template<typename Type>
D_CONSTEXPR bool is_layout_v = is_layout<Type>::value;

template<typename Type>
D_CONSTEXPR bool is_split_v = is_split<Type>::value;
#endif


// ===========================================================================
// VIII. concepts   (C++20 analogs)
// ===========================================================================

#if D_INTERNAL_TABLE_CONCEPTS

template<typename Type>
concept Region = is_region_v<Type>;

template<typename Type>
concept Layout = is_layout_v<Type>;

template<typename Type>
concept Split = is_split_v<Type>;

#endif  // D_INTERNAL_TABLE_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TABLE_TABLE_LAYOUT_HPP
