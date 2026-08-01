/******************************************************************************
* djinterp [container]                                          table_shape.hpp
*
*   The type-level SHAPE of a table -- the structural half of the table model
* both declaration front ends (table_builder, table_parser) fold their input
* into.  A table is a rank-k tuple T = (T_, I_T, Gamma) (containers.tex, The
* table); this header carries the first two layers for the leading case k = 2,
* rectangular, at the type level:
*
*     - the DOMAIN I_T, a box {0..height-1} x {0..width-1}, and
*     - the CELL-TYPE assignment, one type per column (tau_{r,c} = tau_c) --
*       the axis-typed / column-typed reading, of which the cell-homogeneous
*       table (every tau_c equal) is the special case S = empty.
*
*   WHAT A SHAPE DECIDES.  A shape names width, height, and the column types,
* and from those the framework reads which container the built table REALIZES
* into (realization_kind): a static or dynamic height crossed with a homogeneous
* or heterogeneous column list picks static_table / table / record_table.  The
* shape is thus the pivot between the surface DSL and the container trio -- the
* builder computes a shape, and the realization kind names its backing.
*
*   STRUCTURE ONLY -- NO CELL VALUES.  A shape describes the table's TYPE, not
* its contents; the cell VALUES v_i are a separate layer of the model (their
* compile-time encoding is a distinct concern; see the design notes).  Keeping
* values out here lets a shape be a pure, value-free type usable on any standard.
*
*   RECTANGULAR, k = 2, FOR NOW.  Jagged and sparse domains, and rank k > 2 (the
* uniformly-nested F_1[..F_k[tau]..] realized by letting a column type be itself
* a table_shape), are planned siblings kept out of this first cut; the width /
* height / column-type surface generalises to them without disturbing callers.
*
*   PORTABILITY:
*   C++11 baseline (the shape is std::size_t- and type-parameterised; no auto
* NTTP).  The _v shorthands are C++14; the concept face is C++20.
*
*
* path:      /inc/djinterp/core/container/table/table_shape.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.14
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    dynamic_extent               (the runtime-height sentinel)
II.   pack helpers                 (all_same / pack_type_at / first_type)
III.  table_shape                  (the rank-2 rectangular shape)
IV.   realization_kind             (which container a shape backs onto)
V.    realization_of               (shape -> realization_kind)
VI.   is_table_shape               (detection trait)
VII.  concepts                     (C++20 analogs)
*/

#ifndef DJINTERP_CONTAINER_TABLE_SHAPE_
#define DJINTERP_CONTAINER_TABLE_SHAPE_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../../config/core/container/table/cfg_table.h"
#include "../../djinterp.hpp"   // NS_*, D_CONSTEXPR, clean_t, D_ENV_* feature macros


NS_DJINTERP


// ===========================================================================
// I.   dynamic_extent
// ===========================================================================

// dynamic_extent
//   value: the height sentinel for a runtime-sized table -- a table whose row
// count is read at runtime (Lifetime: runtime-expressible size), as opposed to a
// height fixed by the type.  Mirrors std::dynamic_extent in spirit.  The width is
// always a compile-time quantity here (it is the length of the column-type list),
// so only the height admits this sentinel.
D_CONSTEXPR std::size_t dynamic_extent = static_cast<std::size_t>(-1);


// ===========================================================================
// II.  pack helpers
// ===========================================================================
//   Small type-pack utilities used by the shape queries.  Local (rather than
// pulled from dtuple) so a shape is a light, standalone type.

NS_INTERNAL

    // all_same
    //   trait: every type in the pack is identical.  An empty or singleton pack
    // is vacuously uniform.
    template<typename...>
    struct all_same : std::true_type
    {};

    template<typename _Only>
    struct all_same<_Only> : std::true_type
    {};

    template<typename    _First,
             typename    _Second,
             typename... _Rest>
    struct all_same<_First, _Second, _Rest...>
        : std::integral_constant<bool,
            ( std::is_same<_First, _Second>::value &&
              all_same<_Second, _Rest...>::value )>
    {};

    // pack_type_at_t
    //   type: the _Index-th type of the pack (0-based), via the tuple protocol.
    template<std::size_t _Index,
             typename... _Types>
    using pack_type_at_t =
        typename std::tuple_element<_Index, std::tuple<_Types...>>::type;

    // first_type
    //   trait: the first type of a pack, or void for an empty pack.  Lazy so it
    // is well-formed at width 0 (where pack_type_at_t<0> would be ill-formed).
    template<bool        _NonEmpty,
             typename... _Types>
    struct first_type
    {
        using type = void;
    };

    template<typename    _First,
             typename... _Rest>
    struct first_type<true, _First, _Rest...>
    {
        using type = _First;
    };

NS_END  // internal


// ===========================================================================
// III. table_shape
// ===========================================================================

// table_shape
//   type: the rank-2 rectangular shape of a table -- a height and a list of
// column types.  The domain is the box {0..height-1} x {0..width-1}; the cell
// type at column c is the c-th column type (tau_{r,c} = tau_c).  Homogeneity is
// derived, not declared: a shape is cell-homogeneous exactly when every column
// type is the same, and axis-typed otherwise.
//
//   _Height : the row count, or dynamic_extent for a runtime-sized table.
//   _Cols...: the column types; the width is their count.
//
// Example:
//   using grid = table_shape<3, int, int, int>;   // 3 x 3, homogeneous (int)
//   using rec  = table_shape<dynamic_extent, int, std::string, double>;
//                                                 // n x 3 record, heterogeneous
template<std::size_t _Height,
         typename... _Cols>
struct table_shape
{
    // --- extents ---

    // rank
    //   value: the number of coordinates -- 2 for this rectangular row/column
    // shape (a rank-k generalisation lets a column type be a table_shape).
    static D_CONSTEXPR std::size_t rank = 2;

    // width
    //   value: the number of columns (the length of the column-type list).
    static D_CONSTEXPR std::size_t width = sizeof...(_Cols);

    // height
    //   value: the number of rows, or dynamic_extent when runtime-sized.
    static D_CONSTEXPR std::size_t height = _Height;

    // static_height
    //   value: whether the height is fixed by the type (vs read at runtime).
    static D_CONSTEXPR bool static_height = (_Height != dynamic_extent);

    // --- cell types ---

    // is_homogeneous
    //   value: whether one cell type serves every column (S = empty) -- the
    // cell-homogeneous table.  A width-0 shape is vacuously homogeneous.
    static D_CONSTEXPR bool is_homogeneous = internal::all_same<_Cols...>::value;

    // element_type
    //   type: the common cell type when homogeneous (the type of column 0), or
    // void for a width-0 shape.  Only meaningful when is_homogeneous is true.
    using element_type =
        typename internal::first_type<(sizeof...(_Cols) > 0), _Cols...>::type;

    // columns_type
    //   type: the column-type list as a tuple -- the axis-typed record's column
    // types, and the row-record type of a heterogeneous realization.
    using columns_type = std::tuple<_Cols...>;

    // column_type_t
    //   type: the cell type of column _C (tau_c).  _C must be < width.
    template<std::size_t _C>
    using column_type_t = internal::pack_type_at_t<_C, _Cols...>;
};


// ===========================================================================
// IV.  realization_kind
// ===========================================================================

// realization_kind
//   enum: the container family a shape realizes into.  A static or dynamic
// height crossed with a homogeneous or heterogeneous column list:
//
//     static_homogeneous  -> static_table  (constexpr, inline, immutable grid)
//     dynamic_homogeneous -> table         (dynamic, cell-homogeneous)
//     static_record       -> record_table  (fixed height, column-typed record)
//     dynamic_record      -> record_table  (dynamic, column-typed record)
//
// The element-mutable but fixed-shape backing (fixed_table) and the bounded /
// keyed / sorted overlays are further refinements the option surface selects on
// top of this base classification; they do not change the family named here.
enum class realization_kind
{
    static_homogeneous,
    dynamic_homogeneous,
    static_record,
    dynamic_record
};


// ===========================================================================
// V.   realization_of
// ===========================================================================

// realization_of
//   trait: the realization_kind a table_shape backs onto -- the pivot the builder
// reads to pick a concrete container for a computed shape.
template<typename _Shape>
struct realization_of;

template<std::size_t _Height,
         typename... _Cols>
struct realization_of<table_shape<_Height, _Cols...>>
{
private:
    using shape_type = table_shape<_Height, _Cols...>;

    static D_CONSTEXPR bool s = shape_type::static_height;
    static D_CONSTEXPR bool h = shape_type::is_homogeneous;

public:
    static D_CONSTEXPR realization_kind value =
        ( s ? ( h ? realization_kind::static_homogeneous
                  : realization_kind::static_record )
            : ( h ? realization_kind::dynamic_homogeneous
                  : realization_kind::dynamic_record ) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// realization_of_v
//   value: shorthand for realization_of<_Shape>::value.
template<typename _Shape>
D_CONSTEXPR realization_kind realization_of_v = realization_of<_Shape>::value;
#endif


// ===========================================================================
// VI.  is_table_shape
// ===========================================================================

NS_INTERNAL

    // is_table_shape_impl
    //   trait: primary (not a table_shape).
    template<typename _Type>
    struct is_table_shape_impl : std::false_type
    {};

    template<std::size_t _Height,
             typename... _Cols>
    struct is_table_shape_impl<table_shape<_Height, _Cols...>> : std::true_type
    {};

NS_END  // internal

// is_table_shape
//   trait: true iff _Type (after stripping cv/ref) is a table_shape.
template<typename _Type>
struct is_table_shape
    : internal::is_table_shape_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_table_shape_v
//   value: shorthand for is_table_shape<_Type>::value.
template<typename _Type>
D_CONSTEXPR bool is_table_shape_v = is_table_shape<_Type>::value;
#endif


// ===========================================================================
// VII. concepts   (C++20 analogs)
// ===========================================================================

#if D_INTERNAL_TABLE_CONCEPTS

// TableShape
//   concept: satisfied iff _Type is a table_shape.  Parallels is_table_shape_v.
template<typename _Type>
concept TableShape = is_table_shape_v<_Type>;

#endif  // D_INTERNAL_TABLE_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TABLE_SHAPE_
