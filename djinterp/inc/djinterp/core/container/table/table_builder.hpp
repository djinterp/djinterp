/******************************************************************************
* djinterp [container]                                        table_builder.hpp
*
*   The TYPE DSL -- the compile-time front end for declaring a table.  A
* table_builder<...> is a table written as a type: a list of DECLARATORS naming
* the axes (columns<> / rows<>) and the cells (row<>, cell<>, merged_cell<>,
* split_cell<>), which this header FOLDS into the table model -- a table_shape
* (the domain I_T and the cell types tau_c) and a layout (the overlay Gamma) --
* whence a concrete container is realized.  It is the compose leg of the prism
* (ch-parsing.tex) with C++ types, rather than text, as the surface; the text leg
* is table_parser, and both meet at the same model.
*
*   EVERY DECLARATOR IS A TYPE.  A cell entry is a TYPE, uniformly -- so a row may
* mix a bare cell with a merged_cell<> or a split_cell<> in one pack (a C++ pack
* cannot mix types and values, and the declarators must interleave freely).  A
* compile-time VALUE rides in a cell as a value-carrying type: val<V> lifts an
* NTTP (C++17), and any carrier / integral_constant / tag type serves elsewhere.
* This keeps the whole surface at the C++11 baseline of table_shape.
*
*   A CELL ADVANCES THE COLUMN CURSOR BY ITS SPAN.  This is the whole fold.  A
* plain cell spans one column; a merged_cell<R, C, V> spans C of them (and R
* rows).  So
*
*       row<a, merged_cell<1, 2, b>, w, x, y, z>       // 1+2+1+1+1+1 = 7 columns
*
* is a row of a SEVEN-column table, and a merge is registered at the cursor
* position it is declared at, its anchor -- min_lex R_C, exactly as the formalism
* names a layout cell (containers.tex).  A flat rows<...> is walked the same way
* and wraps at the table's width, so the rows it holds are INFERRED, spans and
* all, rather than counted.
*
*   THE INDEPENDENT AXIS is the first axis declarator: leading with columns<>
* declares the table column-wise (the ordinary reading, rows dependent), leading
* with rows<> declares its TRANSPOSE.  Since a table is an indexed tuple, a
* transposed declaration is a pure re-reading of the same object -- the coordinate
* swap tau_{r,c} <-> tau_{c,r} -- so every declaration below has an inverse for
* free, with no second code path.
*
*   RANK.  Each top-level columns<> group contributes one dimension and the cell
* data contributes the dependent one, so k = (column groups) + 1; the leading
* rank-2 case (one group) is fully folded here, and a multi-group declaration
* records its group structure and its flattened rank-2 view (the data spans the
* groups concatenated, as the width sum) pending the nested realization.
*
*   PORTABILITY:
*   C++11 baseline, as table_shape / table_layout; val<> (the NTTP lift) is C++17.
*
*
* path:      /inc/djinterp/core/container/table/table_builder.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.14
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    declarators                (columns / rows / row / column / cell /
                                  merged_cell / split_cell / val)
II.   type_list                  (the fold's accumulator)
III.  cell traits                (cell_rows / cell_cols / cell_value_t)
IV.   declarator classification  (is_columns / is_rows / is_row / table_axis)
V.    transposition              (transpose_decl: the free symmetry)
VI.   the fold                   (span walk: width, height, merges)
VII.  column collection          (columns, groups, data, leading axis,
                                  normalize_decls, builder_model)
VIII. table_builder              (the DSL surface)
IX.   detection                  (is_table_builder)
X.    concepts                   (C++20 analogs)
*/

#ifndef DJINTERP_CONTAINER_TABLE_BUILDER_
#define DJINTERP_CONTAINER_TABLE_BUILDER_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../../config/core/container/table/cfg_table.h"
#include "../../djinterp.hpp"    // NS_*, D_CONSTEXPR, clean_t, D_ENV_*
#include "./table_shape.hpp"      // table_shape, dynamic_extent, realization_of
#include "./table_layout.hpp"     // region, layout, layout_valid, split
#include "./table_options.hpp"    // the strictness vocabulary (C++11) + the
                                  // option surface / pack partition (C++17)


NS_DJINTERP


// ===========================================================================
// I.   declarators
// ===========================================================================
//   The DSL's vocabulary.  Each is an empty type -- a declarator is read by the
// fold, never instantiated as an object.

// columns
//   type: one axis of the table, given by its cell types -- the axis-typed
// reading tau_{r,c} = tau_c, of extent sizeof...(_Types).  Leading a declaration
// with columns<> makes the columns the INDEPENDENT axis (the ordinary reading).
// A second columns<> group adds a dimension (k = groups + 1).
template<typename... _Types>
struct columns
{
    static D_CONSTEXPR std::size_t extent = sizeof...(_Types);

    using types_type = std::tuple<_Types...>;
};

// rows
//   type: the transposed counterpart of columns<> when it LEADS a declaration
// (the rows become the independent axis); otherwise, a flat run of cell entries,
// wrapped at the table's width, so the individual rows are inferred -- the
// "every four entries is a row" reading of a four-column table.
template<typename... _Types>
struct rows
{
    static D_CONSTEXPR std::size_t extent = sizeof...(_Types);

    using types_type = std::tuple<_Types...>;
};

// row
//   type: exactly one row of the table, given by its cell entries.  An entry may
// be a bare cell type, a cell<>, a merged_cell<>, or a split_cell<>; the row's
// width is the sum of their column spans.
template<typename... _Cells>
struct row
{
    static D_CONSTEXPR std::size_t count = sizeof...(_Cells);
};

// column
//   type: exactly one COLUMN of the table, given by its cell entries -- the
// transpose of row<>, used to fill a declaration whose independent axis is the
// rows (one that leads with rows<>).
template<typename... _Cells>
struct column
{
    static D_CONSTEXPR std::size_t count = sizeof...(_Cells);
};

// cell
//   type: one atomic cell holding _Value -- the trivial layout cell (Gamma_0,
// span 1x1).  Superfluous inside a row<> (a bare entry is already a cell) and
// accepted for symmetry with the merged / split forms.
template<typename _Value>
struct cell
{
    using value_type = _Value;
};

// merged_cell
//   type: a cell spanning _Rows x _Cols atomic positions and holding one value
// (|R_C| > 1: layout-aware access returns _Value for every covered position,
// which defers to the anchor -- the position the merge is declared at).
template<std::size_t _Rows,
         std::size_t _Cols,
         typename    _Value>
struct merged_cell
{
    using value_type = _Value;

    static D_CONSTEXPR std::size_t rows = _Rows;
    static D_CONSTEXPR std::size_t cols = _Cols;
};

// split_cell
//   type: a cell spanning _Rows x _Cols, refined into _Pieces sub-cells whose
// contents are _Content (typically a row<>).  The refinement of a layout cell:
// its region is partitioned into finer pieces.
template<std::size_t _Rows,
         std::size_t _Cols,
         std::size_t _Pieces,
         typename    _Content>
struct split_cell
{
    using content_type = _Content;

    static D_CONSTEXPR std::size_t rows   = _Rows;
    static D_CONSTEXPR std::size_t cols   = _Cols;
    static D_CONSTEXPR std::size_t pieces = _Pieces;
};

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
// val
//   type: lifts a compile-time VALUE into the type-level cell vocabulary, so a
// value cell reads as val<42> among the type cells.  The bridge between the
// all-types declarator surface and NTTP values.
template<auto _V>
using val = std::integral_constant<decltype(_V), _V>;
#endif


// ===========================================================================
// II.  type_list
// ===========================================================================

// type_list
//   type: the fold's accumulator -- a bare list of types (the collected merges,
// the collected column types).  Kept local so the DSL's fold does not drag in the
// full tuple algebra.
template<typename... _Types>
struct type_list
{
    static D_CONSTEXPR std::size_t size = sizeof...(_Types);
};

NS_INTERNAL

    // list_cat
    //   trait: concatenate two type_lists.
    template<typename _A,
             typename _B>
    struct list_cat;

    template<typename... _As,
             typename... _Bs>
    struct list_cat<type_list<_As...>, type_list<_Bs...>>
    {
        using type = type_list<_As..., _Bs...>;
    };

    template<typename _A,
             typename _B>
    using list_cat_t = typename list_cat<_A, _B>::type;

    // list_to_layout
    //   trait: reinterpret a collected type_list of regions as a layout<> (the
    // fold accumulates a list; the model wants the cover).
    template<typename _List>
    struct list_to_layout;

    template<typename... _Regions>
    struct list_to_layout<type_list<_Regions...>>
    {
        using type = layout<_Regions...>;
    };

    // list_to_shape
    //   trait: reinterpret a collected type_list of column types as a
    // table_shape of height _Height.
    template<std::size_t _Height,
             typename    _List>
    struct list_to_shape;

    template<std::size_t _Height,
             typename... _Cols>
    struct list_to_shape<_Height, type_list<_Cols...>>
    {
        using type = table_shape<_Height, _Cols...>;
    };

NS_END  // internal


// ===========================================================================
// III. cell traits
// ===========================================================================
//   How many atomic positions an entry spans.  The whole fold turns on this: a
// plain cell spans one column, a merged / split cell spans its declared box.

// cell_rows / cell_cols
//   traits: the row and column span of a cell entry.  The primary treats any
// type as a plain 1x1 cell -- so a bare entry (a value-carrying type, a tag, a
// val<>) needs no opt-in; the specializations give the merged / split forms
// their declared box.
template<typename _Cell>
struct cell_rows : std::integral_constant<std::size_t, 1>
{};

template<typename _Cell>
struct cell_cols : std::integral_constant<std::size_t, 1>
{};

template<std::size_t _R,
         std::size_t _C,
         typename    _V>
struct cell_rows<merged_cell<_R, _C, _V>>
    : std::integral_constant<std::size_t, _R>
{};

template<std::size_t _R,
         std::size_t _C,
         typename    _V>
struct cell_cols<merged_cell<_R, _C, _V>>
    : std::integral_constant<std::size_t, _C>
{};

template<std::size_t _R,
         std::size_t _C,
         std::size_t _P,
         typename    _Content>
struct cell_rows<split_cell<_R, _C, _P, _Content>>
    : std::integral_constant<std::size_t, _R>
{};

template<std::size_t _R,
         std::size_t _C,
         std::size_t _P,
         typename    _Content>
struct cell_cols<split_cell<_R, _C, _P, _Content>>
    : std::integral_constant<std::size_t, _C>
{};

// cell_value_t
//   type: the value a cell entry holds.  A bare entry is its own value; cell<>,
// merged_cell<> unwrap to their _Value.  (A split_cell holds a refinement, not a
// single value, so it keeps its content type.)
template<typename _Cell>
struct cell_value
{
    using type = _Cell;
};

template<typename _Value>
struct cell_value<cell<_Value>>
{
    using type = _Value;
};

template<std::size_t _R,
         std::size_t _C,
         typename    _V>
struct cell_value<merged_cell<_R, _C, _V>>
{
    using type = _V;
};

template<typename _Cell>
using cell_value_t = typename cell_value<_Cell>::type;


// ===========================================================================
// IV.  declarator classification
// ===========================================================================

NS_INTERNAL

    template<typename _Type>
    struct is_columns_impl : std::false_type
    {};

    template<typename... _Ts>
    struct is_columns_impl<columns<_Ts...>> : std::true_type
    {};

    template<typename _Type>
    struct is_rows_impl : std::false_type
    {};

    template<typename... _Ts>
    struct is_rows_impl<rows<_Ts...>> : std::true_type
    {};

    template<typename _Type>
    struct is_row_impl : std::false_type
    {};

    template<typename... _Ts>
    struct is_row_impl<row<_Ts...>> : std::true_type
    {};

NS_END  // internal

// is_columns / is_rows / is_row
//   traits: true iff _Type is the named declarator.
template<typename _Type>
struct is_columns : internal::is_columns_impl<clean_t<_Type>>
{};

template<typename _Type>
struct is_rows : internal::is_rows_impl<clean_t<_Type>>
{};

template<typename _Type>
struct is_row : internal::is_row_impl<clean_t<_Type>>
{};

// table_axis
//   enum: which axis a declaration makes independent -- read from its FIRST axis
// declarator.  Leading with columns<> is the ordinary column-wise reading;
// leading with rows<> declares the transpose.
enum class table_axis
{
    column_major,   // columns<> leads: the columns are the independent axis
    row_major       // rows<> leads: the transpose reading
};


// ===========================================================================
// V.   transposition
// ===========================================================================
//   The free symmetry.  A table is an indexed tuple, so exchanging its two
// coordinates is a pure RE-READING of the same object -- there is no second table
// and no second fold.  A declaration that leads with rows<> is therefore
// NORMALIZED by transposing every declarator (columns<> <-> rows<>, row<> <->
// column<>, and a spanning cell's box), which yields the equivalent column-wise
// declaration; the one fold then runs on that.  This is what buys "every example
// may be declared inversely" for one price.

// transpose_decl
//   trait: the transpose of a declarator -- the same declaration read with its
// coordinates exchanged.  The primary leaves a bare cell entry alone (a value is
// orientation-free); the specializations swap each declarator with its opposite
// and each spanning cell's box.
template<typename _Decl>
struct transpose_decl
{
    using type = _Decl;
};

// an axis group and its opposite
template<typename... _Ts>
struct transpose_decl<columns<_Ts...>>
{
    using type = rows<_Ts...>;
};

template<typename... _Ts>
struct transpose_decl<rows<_Ts...>>
{
    using type = columns<_Ts...>;
};

// a line of cells and its opposite (its entries transpose with it)
template<typename... _Cells>
struct transpose_decl<row<_Cells...>>
{
    using type = column<typename transpose_decl<_Cells>::type...>;
};

template<typename... _Cells>
struct transpose_decl<column<_Cells...>>
{
    using type = row<typename transpose_decl<_Cells>::type...>;
};

// a spanning cell: its box turns with the table
template<std::size_t _R,
         std::size_t _C,
         typename    _V>
struct transpose_decl<merged_cell<_R, _C, _V>>
{
    using type = merged_cell<_C, _R, _V>;
};

template<std::size_t _R,
         std::size_t _C,
         std::size_t _P,
         typename    _Content>
struct transpose_decl<split_cell<_R, _C, _P, _Content>>
{
    using type = split_cell<_C, _R, _P,
                            typename transpose_decl<_Content>::type>;
};

// transpose_decl_t
//   type: shorthand for transpose_decl<_Decl>::type.
template<typename _Decl>
using transpose_decl_t = typename transpose_decl<_Decl>::type;


// ===========================================================================
// VI.  the fold
// ===========================================================================
//   The span walk.  Every cell entry advances a column cursor by its column span
// and, when it covers more than one atomic position, registers a region at the
// cursor -- its anchor.  A row<> walks its entries once; a flat rows<> walks and
// WRAPS at the table's width, inferring the rows it holds.

NS_INTERNAL

    // span_sum
    //   trait: the total column span of a pack of cell entries -- a row's width.
    template<typename...>
    struct span_sum : std::integral_constant<std::size_t, 0>
    {};

    template<typename    _E0,
             typename... _Es>
    struct span_sum<_E0, _Es...>
        : std::integral_constant<std::size_t,
            (cell_cols<_E0>::value + span_sum<_Es...>::value)>
    {};

    // row_merges
    //   trait: walk one row's entries from column _Col, collecting a region for
    // every entry that covers more than one atomic position.  The region's anchor
    // is the cursor at which the entry is declared.
    template<std::size_t _Row,
             std::size_t _Col,
             typename... _Entries>
    struct row_merges
    {
        using type = type_list<>;
    };

    template<std::size_t _Row,
             std::size_t _Col,
             typename    _E0,
             typename... _Es>
    struct row_merges<_Row, _Col, _E0, _Es...>
    {
    private:
        static D_CONSTEXPR std::size_t r = cell_rows<_E0>::value;
        static D_CONSTEXPR std::size_t c = cell_cols<_E0>::value;

        // a spanning entry registers a layout cell; a 1x1 entry does not
        using here =
            typename std::conditional<((r * c) > 1),
                type_list<region<_Row, _Col, r, c>>,
                type_list<>
            >::type;

        using rest = typename row_merges<_Row, _Col + c, _Es...>::type;

    public:
        using type = list_cat_t<here, rest>;
    };

    // flat_walk
    //   trait: walk a flat run of entries from (_Row, _Col), wrapping at _Width.
    // Yields the collected merges and the row cursor the run ends at -- the
    // inference that turns a flat rows<> into rows.
    template<std::size_t _Width,
             std::size_t _Row,
             std::size_t _Col,
             typename... _Entries>
    struct flat_walk
    {
        using merges = type_list<>;

        // a partially-filled row still counts as a row
        static D_CONSTEXPR std::size_t end_row = (_Col == 0) ? _Row : (_Row + 1);
        static D_CONSTEXPR std::size_t end_col = 0;
    };

    template<std::size_t _Width,
             std::size_t _Row,
             std::size_t _Col,
             typename    _E0,
             typename... _Es>
    struct flat_walk<_Width, _Row, _Col, _E0, _Es...>
    {
    private:
        static D_CONSTEXPR std::size_t r = cell_rows<_E0>::value;
        static D_CONSTEXPR std::size_t c = cell_cols<_E0>::value;

        using here =
            typename std::conditional<((r * c) > 1),
                type_list<region<_Row, _Col, r, c>>,
                type_list<>
            >::type;

        // advance, wrapping to the next row at the width
        static D_CONSTEXPR std::size_t next_col_raw = _Col + c;
        static D_CONSTEXPR bool        wraps        = (next_col_raw >= _Width);

        static D_CONSTEXPR std::size_t next_row = wraps ? (_Row + 1) : _Row;
        static D_CONSTEXPR std::size_t next_col = wraps ? 0 : next_col_raw;

        using rest = flat_walk<_Width, next_row, next_col, _Es...>;

    public:
        using merges = list_cat_t<here, typename rest::merges>;

        static D_CONSTEXPR std::size_t end_row = rest::end_row;
        static D_CONSTEXPR std::size_t end_col = rest::end_col;
    };

    // decl_walk
    //   trait: walk the DATA declarators (row<> / rows<>) in order from row
    // _Row, collecting every merge and the final height.  Non-data declarators
    // (the axis groups) are skipped -- they shape the table, they do not fill it.
    template<std::size_t _Width,
             std::size_t _Row,
             typename... _Decls>
    struct decl_walk
    {
        using merges = type_list<>;

        static D_CONSTEXPR std::size_t end_row = _Row;
    };

    // decl_walk: a row<> -- exactly one row, walked from column 0.
    template<std::size_t _Width,
             std::size_t _Row,
             typename... _Cells,
             typename... _Rest>
    struct decl_walk<_Width, _Row, row<_Cells...>, _Rest...>
    {
    private:
        using here = typename row_merges<_Row, 0, _Cells...>::type;
        using rest = decl_walk<_Width, _Row + 1, _Rest...>;

    public:
        using merges = list_cat_t<here, typename rest::merges>;

        static D_CONSTEXPR std::size_t end_row = rest::end_row;
    };

    // decl_walk: a rows<> -- a flat run, wrapped at the width.
    template<std::size_t _Width,
             std::size_t _Row,
             typename... _Cells,
             typename... _Rest>
    struct decl_walk<_Width, _Row, rows<_Cells...>, _Rest...>
    {
    private:
        using walk = flat_walk<_Width, _Row, 0, _Cells...>;
        using rest = decl_walk<_Width, walk::end_row, _Rest...>;

    public:
        using merges = list_cat_t<typename walk::merges, typename rest::merges>;

        static D_CONSTEXPR std::size_t end_row = rest::end_row;
    };

    // decl_walk: anything else (a columns<> group, an option) -- skipped.
    template<std::size_t _Width,
             std::size_t _Row,
             typename    _D0,
             typename... _Rest>
    struct decl_walk<_Width, _Row, _D0, _Rest...>
        : decl_walk<_Width, _Row, _Rest...>
    {};

NS_END  // internal


// ===========================================================================
// VII. column collection
// ===========================================================================
//   The column types, gathered across every top-level axis group.  A rank-2
// declaration has one group and the collection is simply its types; a multi-group
// declaration concatenates them -- the flattened rank-2 view of the rank-k table,
// which is exactly what the data rows span (a 3-group + 4-group declaration is
// filled by seven-column rows).

NS_INTERNAL

    // collect_columns
    //   trait: the concatenated column types of every columns<> (or leading
    // rows<>) group among the declarators, in declaration order.
    template<typename... _Decls>
    struct collect_columns
    {
        using type = type_list<>;
    };

    template<typename... _Ts,
             typename... _Rest>
    struct collect_columns<columns<_Ts...>, _Rest...>
    {
        using type = list_cat_t<type_list<_Ts...>,
                                typename collect_columns<_Rest...>::type>;
    };

    template<typename    _D0,
             typename... _Rest>
    struct collect_columns<_D0, _Rest...>
        : collect_columns<_Rest...>
    {};

    // count_groups
    //   trait: the number of top-level columns<> groups -- the dimension count is
    // this plus one (the dependent data axis).
    template<typename... _Decls>
    struct count_groups : std::integral_constant<std::size_t, 0>
    {};

    template<typename... _Ts,
             typename... _Rest>
    struct count_groups<columns<_Ts...>, _Rest...>
        : std::integral_constant<std::size_t,
            (1 + count_groups<_Rest...>::value)>
    {};

    template<typename    _D0,
             typename... _Rest>
    struct count_groups<_D0, _Rest...>
        : count_groups<_Rest...>
    {};

    // has_data
    //   trait: whether any row<> / rows<> declarator is present (an empty typed
    // table declares a shape and no contents).
    template<typename... _Decls>
    struct has_data : std::false_type
    {};

    template<typename... _Cs,
             typename... _Rest>
    struct has_data<row<_Cs...>, _Rest...> : std::true_type
    {};

    template<typename... _Cs,
             typename... _Rest>
    struct has_data<rows<_Cs...>, _Rest...> : std::true_type
    {};

    template<typename    _D0,
             typename... _Rest>
    struct has_data<_D0, _Rest...> : has_data<_Rest...>
    {};

    // leading_axis
    //   trait: the axis the FIRST axis declarator makes independent.  A leading
    // columns<> is the ordinary column-wise reading; a leading rows<> is the
    // transpose.  With no axis declarator at all, column-major is assumed.
    template<typename... _Decls>
    struct leading_axis
        : std::integral_constant<table_axis, table_axis::column_major>
    {};

    template<typename... _Ts,
             typename... _Rest>
    struct leading_axis<columns<_Ts...>, _Rest...>
        : std::integral_constant<table_axis, table_axis::column_major>
    {};

    template<typename... _Ts,
             typename... _Rest>
    struct leading_axis<rows<_Ts...>, _Rest...>
        : std::integral_constant<table_axis, table_axis::row_major>
    {};

    template<typename    _D0,
             typename... _Rest>
    struct leading_axis<_D0, _Rest...>
        : leading_axis<_Rest...>
    {};

    // row_width_ok
    //   function: whether a row of _actual columns fits a table of _required
    // columns under _s -- the cell_count strictness, applied.  exact demands a
    // one-to-one match; truncate tolerates a surplus (it is dropped); pad
    // tolerates a shortfall (it is filled); lenient tolerates either.
    D_CONSTEXPR inline bool
    row_width_ok(
        table_strictness _s,
        std::size_t      _actual,
        std::size_t      _required
    )
    {
        return ( (_s == table_strictness::exact)    ? (_actual == _required)
               : (_s == table_strictness::truncate) ? (_actual >= _required)
               : (_s == table_strictness::pad)      ? (_actual <= _required)
               :                                      true );
    }

    // flat_span_ok
    //   function: whether a flat run of _total cells fills whole rows of
    // _required columns under _s.  A zero-width table admits no cells at all;
    // otherwise exact demands a whole number of rows and the tolerant grades
    // accept the partial last row they are named for.
    D_CONSTEXPR inline bool
    flat_span_ok(
        table_strictness _s,
        std::size_t      _total,
        std::size_t      _required
    )
    {
        return ( (_required == 0)                ? (_total == 0)
               : (_s == table_strictness::exact) ? ((_total % _required) == 0)
               :                                   true );
    }

    // check_decl_widths
    //   trait: every data declarator fits the table's width under _S.  A row<>
    // is checked against the width directly; a flat rows<> against whole rows.
    // Non-data declarators are skipped.
    template<table_strictness _S,
             std::size_t      _Width,
             typename...      _Decls>
    struct check_decl_widths : std::true_type
    {};

    template<table_strictness _S,
             std::size_t      _Width,
             typename...      _Cells,
             typename...      _Rest>
    struct check_decl_widths<_S, _Width, row<_Cells...>, _Rest...>
        : std::integral_constant<bool,
            ( row_width_ok(_S, span_sum<_Cells...>::value, _Width) &&
              check_decl_widths<_S, _Width, _Rest...>::value )>
    {};

    template<table_strictness _S,
             std::size_t      _Width,
             typename...      _Cells,
             typename...      _Rest>
    struct check_decl_widths<_S, _Width, rows<_Cells...>, _Rest...>
        : std::integral_constant<bool,
            ( flat_span_ok(_S, span_sum<_Cells...>::value, _Width) &&
              check_decl_widths<_S, _Width, _Rest...>::value )>
    {};

    template<table_strictness _S,
             std::size_t      _Width,
             typename         _D0,
             typename...      _Rest>
    struct check_decl_widths<_S, _Width, _D0, _Rest...>
        : check_decl_widths<_S, _Width, _Rest...>
    {};

    // normalize_decls
    //   trait: the declaration in its canonical column-wise form -- as written
    // when it already leads with columns<>, transposed when it leads with rows<>.
    // Yields a type_list so the fold has a single entry point.
    template<bool        _Transpose,
             typename... _Decls>
    struct normalize_decls
    {
        using type = type_list<_Decls...>;
    };

    template<typename... _Decls>
    struct normalize_decls<true, _Decls...>
    {
        using type = type_list<transpose_decl_t<_Decls>...>;
    };

    // builder_model
    //   trait: THE FOLD, over an already-normalized declaration.  Gathers the
    // column types, walks the data for the height and the merges, and assembles
    // the shape and the cover.  Everything table_builder exposes is read from
    // here, so the transposed and untransposed declarations share one engine.
    template<typename _NormalizedList>
    struct builder_model;

    template<typename... _Decls>
    struct builder_model<type_list<_Decls...>>
    {
        // the column types, concatenated across the axis groups
        using column_list = typename collect_columns<_Decls...>::type;

        static D_CONSTEXPR std::size_t width = column_list::size;

        // the span walk over the data declarators
        using walk = decl_walk<width, 0, _Decls...>;

        static D_CONSTEXPR std::size_t height = walk::end_row;

        using merge_list = typename walk::merges;

        static D_CONSTEXPR std::size_t group_count =
            count_groups<_Decls...>::value;

        static D_CONSTEXPR bool declares_data = has_data<_Decls...>::value;

        using shape_type  = typename list_to_shape<height, column_list>::type;
        using layout_type = typename list_to_layout<merge_list>::type;

        // widths_conform -- whether every data declarator fits the width under a
        // given cell_count strictness.  A member template so the grade is read
        // once, by table_builder, from the option pack.
        template<table_strictness _S>
        struct widths_conform
            : check_decl_widths<_S, width, _Decls...>
        {};
    };

NS_END  // internal


// ===========================================================================
// VIII. table_builder
// ===========================================================================

// table_builder
//   type: a table declared as a type.  Reads its declarators, folds them into a
// shape and a layout, and exposes the model the realization consumes.
//
//   _Decls...: the declarators, in order --
//     columns<...>      an axis group (its types are the column types)
//     rows<...>         a flat run of cell entries (wrapped at the width), or,
//                       when it LEADS, the transposed axis declarator
//     row<...>          exactly one row of cell entries
//   with merged_cell<> / split_cell<> / cell<> / bare types as the entries.
//
// Example (a 3-column table, one row, with a two-column merge):
//   using t = table_builder<columns<int, int, int>,
//                           row<val<1>, merged_cell<1, 2, val<2>>>>;
//   static_assert(t::width  == 3, "");
//   static_assert(t::height == 1, "");
//   static_assert(t::has_merges, "");
template<typename... _Decls>
struct table_builder
{
private:
    // the declaration as WRITTEN, then NORMALIZED: a row-major declaration is
    // transposed into the equivalent column-wise one, and the single fold runs
    // on that (section V).
    static D_CONSTEXPR table_axis declared_axis_ =
        internal::leading_axis<_Decls...>::value;

    using normalized_ =
        typename internal::normalize_decls<
            (declared_axis_ == table_axis::row_major), _Decls...>::type;

    using model_ = internal::builder_model<normalized_>;

public:
    // --- the option surface ---
    //
    //   A declaration may interleave OPTIONS among its declarators.  They come in
    // two vocabularies that cannot share a set (option_set resolves keys with ==
    // and so takes one key_type per set), so the pack is PARTITIONED: the table
    // policies here, the universal container axes there.  Following the
    // framework's own degradation (options_container_base, container_options.hpp),
    // the surface is C++17; below it the fold still runs, at the default grades.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // table_options_type -- the table policies declared in the pack.
    using table_options_type = select_table_options_t<_Decls...>;

    // axis_options_type -- the container axis positions declared in the pack,
    // read with container_axis_value_v and honoured by the realization.
    using axis_options_type = select_axis_options_t<_Decls...>;

    // cell_count_strictness -- how strictly a row's width must match the declared
    // columns (the cell_count policy, or its default).
    static D_CONSTEXPR table_strictness cell_count_strictness =
        cell_count_of<table_options_type>;

    // shape_strictness -- which domain shapes the declaration admits.
    static D_CONSTEXPR shape_policy shape_strictness =
        shape_of<table_options_type>;
#else
    // pre-C++17: no option surface; the fold runs at the default grades.
    static D_CONSTEXPR table_strictness cell_count_strictness =
        table_strictness::exact;

    static D_CONSTEXPR shape_policy shape_strictness =
        shape_policy::rectangular;
#endif

    // --- the declaration, read back ---

    // independent_axis -- which axis the declaration LEADS with.  Leading with
    // columns<> is the ordinary reading; leading with rows<> declares the
    // transpose, and the canonical model below is that transpose (a row-typed
    // table is a column-typed one read the other way).
    static D_CONSTEXPR table_axis independent_axis = declared_axis_;

    // normalized_decls_type -- the declaration after normalization: as written
    // for a column-major declaration, transposed for a row-major one.
    using normalized_decls_type = normalized_;

    // group_count / rank -- each top-level axis group is a dimension, and the
    // cell data is the dependent one: k = groups + 1.
    static D_CONSTEXPR std::size_t group_count = model_::group_count;

    static D_CONSTEXPR std::size_t rank = (group_count + 1);

    // is_rank_2 -- whether the declaration is the leading rank-2 case (one axis
    // group), which realizes directly onto the table containers.
    static D_CONSTEXPR bool is_rank_2 = (group_count <= 1);

    // --- the folded extents (CANONICAL: the column-wise reading) ---

    // width -- the columns the canonical table spans (the group widths, summed:
    // the flattened rank-2 view a rank-k declaration's rows fill).
    static D_CONSTEXPR std::size_t width = model_::width;

    // height -- the rows the data declarators fill, INFERRED: each row<> is one
    // row, and a flat rows<> is walked and wrapped at the width, spans included.
    static D_CONSTEXPR std::size_t height = model_::height;

    // declared_width / declared_height -- the extents as LITERALLY written.  They
    // are the canonical pair for a column-major declaration and the swapped pair
    // for a row-major one, the two being transposes of one table.
    static D_CONSTEXPR std::size_t declared_width =
        (declared_axis_ == table_axis::row_major) ? height : width;

    static D_CONSTEXPR std::size_t declared_height =
        (declared_axis_ == table_axis::row_major) ? width : height;

    // declares_data -- whether any cells were declared (an empty typed table
    // declares a shape alone).
    static D_CONSTEXPR bool declares_data = model_::declares_data;

    // --- the model ---

    // shape_type -- the folded table_shape: the domain and the cell types, in the
    // canonical column-wise reading.
    using shape_type = typename model_::shape_type;

    // layout_type -- the folded cover Gamma: the declared merges, with singletons
    // implied on the rest.
    using layout_type = typename model_::layout_type;

    // columns_type -- the column types as a tuple (the row-record type of a
    // heterogeneous realization).
    using columns_type = typename shape_type::columns_type;

    // --- read-through queries ---

    // is_homogeneous -- whether one cell type serves every column.
    static D_CONSTEXPR bool is_homogeneous = shape_type::is_homogeneous;

    // element_type -- the common cell type when homogeneous.
    using element_type = typename shape_type::element_type;

    // has_merges -- whether the declaration wears a non-trivial layout.
    static D_CONSTEXPR bool has_merges  = layout_type::has_merges;

    static D_CONSTEXPR std::size_t merge_count = layout_type::merge_count;

    // realization -- the container family the shape backs onto.
    static D_CONSTEXPR realization_kind realization =
        realization_of<shape_type>::value;

    // owner_of -- the layout cell owning atomic position (_R, _C): a declared
    // merge covering it, or the singleton there.
    template<std::size_t _R,
             std::size_t _C>
    using owner_of = typename layout_type::template owner_of<_R, _C>;

    // --- conformance ---

    // widths_conform -- whether every data declarator fits the table's width
    // under the declared cell_count strictness.  Exact (the default) demands a
    // one-to-one match; truncate / pad / lenient relax it in the named direction.
    static D_CONSTEXPR bool widths_conform =
        model_::template widths_conform<cell_count_strictness>::value;

    static_assert(widths_conform,
        "table_builder: a row does not fit the declared columns.  Its cells' "
        "spans must sum to the table's width (a merged / split cell counts for "
        "the whole box it spans), and a flat rows<> must fill whole rows.  "
        "Relax this with a cell_count option -- truncate_counts (drop a "
        "surplus), pad_counts (fill a shortfall), or lenient_counts (either).");

    // layout_conforms -- whether the declared merges form a valid cover of the
    // folded extents (in bounds, pairwise disjoint).
    static D_CONSTEXPR bool layout_conforms =
        layout_valid<layout_type, height, width>::value;

    static_assert(layout_conforms,
        "table_builder: the declared merged / split cells do not form a valid "
        "cover -- a spanning cell runs past the table's extents, or two of them "
        "overlap.  A layout cell's region must lie within the table and no two "
        "may share an atomic position.");
};


// ===========================================================================
// IX.  detection
// ===========================================================================

NS_INTERNAL

    template<typename _Type>
    struct is_table_builder_impl : std::false_type
    {};

    template<typename... _Decls>
    struct is_table_builder_impl<table_builder<_Decls...>> : std::true_type
    {};

NS_END  // internal

// is_table_builder
//   trait: true iff _Type (after stripping cv/ref) is a table_builder.
template<typename _Type>
struct is_table_builder
    : internal::is_table_builder_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_table_builder_v
//   value: shorthand for is_table_builder<_Type>::value.
template<typename _Type>
D_CONSTEXPR bool is_table_builder_v = is_table_builder<_Type>::value;
#endif


// ===========================================================================
// X.   concepts   (C++20 analogs)
// ===========================================================================

#if D_INTERNAL_TABLE_CONCEPTS

// TableBuilder
//   concept: satisfied iff _Type is a table_builder.
template<typename _Type>
concept TableBuilder = is_table_builder_v<_Type>;

#endif  // D_INTERNAL_TABLE_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TABLE_BUILDER_
