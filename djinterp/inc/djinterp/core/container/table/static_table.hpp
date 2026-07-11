/******************************************************************************
* djinterp [container]                                        static_table.hpp
*
*   static_table -- the compile-time-constant member of the table trio.  A
* rank-2, rectangular, cell-homogeneous table whose two extents R and C are
* fixed by the type and whose whole value is a static constant: the cells are
* determined at c (compile time), so the object is denotable as a constexpr
* value and every cell read is a constant expression.
*
*   ITS PLACE ON THE FIRST FEW AXES (Part I, in order):
*   - Lifetime  : constexpr_storage -- the value is fixed at compile time.
*   - Storage   : static_storage    -- an inline std::array, no allocation.
*   - Mutability: immutable         -- only const observation; no cell may be
*                                      overwritten and no row or column added,
*                                      so I_T and every v_i are frozen.
*   It is ordered (row-major = lexicographic on multi-indices), bounded (|T| =
* R*C is a compile-time constant), and iterable.  It is the table analogue of a
* `constexpr std::array` lifted to two coordinates.
*
*   RELATION TO THE SIBLINGS:
*   fixed_table relaxes Mutability to element_mutable (writable cells, frozen
* shape); table relaxes Storage to dynamic and Mutability to fully_mutable
* (rows and columns may be added and removed).  All three inherit the read-only
* surface from table_base.
*
*   NOTE ON CELL TYPES:
*   The primary template is cell-homogeneous -- one _Type for every cell, the
* leading case (a grid of numbers, a matrix over a ring).  The axis-typed
* (column-typed relational) table, whose rows are heterogeneous records, is the
* _RowType specialisation noted at the foot of this header; it is not built
* here.
*
*   PORTABILITY:
*   C++14 baseline.
*
*
* path:      /inc/djinterp/core/container/table/static_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.04
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_static_table (detection trait)
II.   static_table (class)
III.  make_static_table / equality
*/

#ifndef DJINTERP_CONTAINER_STATIC_TABLE_
#define DJINTERP_CONTAINER_STATIC_TABLE_ 1

// std
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                     // NS_*, D_CONSTEXPR, clean_t
#include "./table_base.hpp"                        // table_base, row/column views
#include "../container_options.hpp"                // axis enums, options base
#include "../traits/mutable_container_traits.hpp"  // mutability grade
#include "../serial/encode_options.hpp"             // enc_tau<E>, put_length<L,E>, serial enums
#include "../serial/decode_options.hpp"             // dec_tau<E>, get_length<L,E>


NS_DJINTERP


// ===========================================================================
// I.   is_static_table (detection trait)
// ===========================================================================

// static_table (fwd)
//   class: forward declaration for the detection trait below.
template<typename    _Type,
         std::size_t _Rows,
         std::size_t _Cols,
         typename    _DifferenceType,
         typename    _SizeType,
         typename    _Iterator,
         typename    _ConstIterator,
         typename... _Options>
class static_table;

// is_static_table
//   trait: true when _Type (after stripping cv/ref) is a specialization of
// static_table.
NS_INTERNAL

    template<typename _Type>
    struct is_static_table_impl : std::false_type
    {};

    template<typename    _T,
             std::size_t _R,
             std::size_t _C,
             typename    _D,
             typename    _S,
             typename    _I,
             typename    _CI,
             typename... _O>
    struct is_static_table_impl<static_table<_T, _R, _C, _D, _S, _I, _CI, _O...>>
        : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_static_table : internal::is_static_table_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_static_table_v
//   value: variable-template shorthand for is_static_table<_Type>::value.
template<typename _Type>
inline constexpr bool is_static_table_v = is_static_table<_Type>::value;
#endif


// ===========================================================================
// II.  static_table (class)
// ===========================================================================

// static_table
//   class: a compile-time-constant rank-2 cell-homogeneous table of _Rows by
// _Cols cells, stored row-major in an inline std::array.  Immutable: it exposes
// only the const surface inherited from table_base.
template<typename    _Type,
         std::size_t _Rows,
         std::size_t _Cols,
         typename    _DifferenceType = std::ptrdiff_t,
         typename    _SizeType       = std::size_t,
         typename    _Iterator       = const _Type*,
         typename    _ConstIterator  = const _Type*,
         typename... _Options>
class static_table
    : public table_base<static_table<_Type,
                                     _Rows,
                                     _Cols,
                                     _DifferenceType,
                                     _SizeType,
                                     _Iterator,
                                     _ConstIterator,
                                     _Options...>,
                        _Type,
                        _SizeType,
                        _DifferenceType>,
      public options_container_base<_Options...>
{
private:
    using base_type = table_base<static_table,
                                 _Type,
                                 _SizeType,
                                 _DifferenceType>;

    // the flat, row-major cell store; std::array<_,0> is well-formed, so a
    // zero-extent table is legal.
    using storage_type = std::array<_Type, (_Rows * _Cols)>;

public:
    // --- member types ---

    using value_type      = _Type;
    using cell_type       = _Type;
    using size_type       = _SizeType;
    using difference_type = _DifferenceType;
    using reference       = _Type&;
    using const_reference = const _Type&;
    using pointer         = _Type*;
    using const_pointer   = const _Type*;

    // immutability collapses the two cell-iterator faces onto one const cursor;
    // both are still named for a uniform iterator surface across the trio.
    using iterator        = _Iterator;
    using const_iterator  = _ConstIterator;

    // --- axis positions (the first few axes, then the ones that follow) ---

    static constexpr container_lifetime      lifetime      =
        container_lifetime::constexpr_storage;
    static constexpr container_storage_kind  storage_kind  =
        container_storage_kind::static_storage;
    static constexpr mutability              mutability_grade =
        mutability::immutable;
    static constexpr container_ordering      ordering      =
        container_ordering::ordered;
    static constexpr container_bounds        bounds        =
        container_bounds::bounded;
    static constexpr container_iterability   iterability   =
        container_iterability::iterable;
    static constexpr container_multiplicity  multiplicity_grade =
        container_multiplicity::multi;   // m = infinity; cells keyed by position
    static constexpr container_structure     structure     =
        container_structure::hierarchical;  // uniformly nested; depth = rank = 2

    // Iterability stage sub-axis: structure AND contents are compile-time-
    // expressible, so the whole traversal -- cursor and values alike -- is
    // static (the "fixed array of constants" row of the Iterability table).
    static constexpr bool compile_time_iterable = true;
    static constexpr bool compile_time_values   = true;

    // Boundedness: a fixed capacity kappa = R*C < infinity.  `extent` is the
    // djinterp compile-time fixed-capacity signal the bounded-container trait
    // reads; the domain is left free (no value interval).
    static constexpr size_type extent = static_cast<size_type>(_Rows * _Cols);
    static constexpr size_type max_cells = extent;

    // the two compile-time extents (m_1 + 1, m_2 + 1).
    static constexpr size_type row_extent    = static_cast<size_type>(_Rows);
    static constexpr size_type column_extent = static_cast<size_type>(_Cols);

    // --- construction ---

    // default: value-initializes every cell to _Type{}.
    constexpr static_table()
        : m_cells{}
    {}

    // element-wise: exactly R*C cell values in row-major order, e.g.
    //   static_table<int,2,3>{ 1,2,3, 4,5,6 }.
    // Constrained to two-or-more cells so it never shadows the copy, move, or
    // std::array constructors; a 1x1 table uses `filled` or the array form.
    template<typename... _Cells,
             typename = typename std::enable_if<
                 ( (sizeof...(_Cells) == (_Rows * _Cols)) &&
                   (sizeof...(_Cells) >= 2) )>::type>
    constexpr static_table(_Cells&&... _cells)
        : m_cells{ { static_cast<_Type>(static_cast<_Cells&&>(_cells))... } }
    {}

    // from a flat, row-major std::array of the exact cell count.
    explicit constexpr static_table(const storage_type& _cells)
        : m_cells(_cells)
    {}

    constexpr static_table(const static_table&)            = default;
    constexpr static_table(static_table&&)                 = default;
    static_table& operator=(const static_table&)           = default;
    static_table& operator=(static_table&&)                = default;
    ~static_table()                                        = default;

    // filled
    //   factory: a table with every cell equal to _value.  A named factory,
    // not a constructor, so a single-argument fill never competes with the
    // element-wise constructor.
    static constexpr static_table filled(const _Type& _value)
    {
        static_table t;

        // write the fill value into every cell
        for (std::size_t i = 0; i < (_Rows * _Cols); ++i)
        {
            t.m_cells[i] = _value;
        }

        return t;
    }

    // --- table_base hooks (contiguous row-major buffer + the two extents) ---

    D_NODISCARD constexpr const_pointer data() const noexcept
    {
        return m_cells.data();
    }

    D_NODISCARD static constexpr size_type rows() noexcept
    {
        return row_extent;
    }

    D_NODISCARD static constexpr size_type cols() noexcept
    {
        return column_extent;
    }

    // --- boundedness accessors (fixed capacity kappa = R*C) ---

    // capacity -- the fixed cap on |T|; equal to size for a fixed-shape table.
    D_NODISCARD static constexpr size_type capacity() noexcept
    {
        return extent;
    }

    // max_size -- the largest |T| the type can ever hold (also R*C).
    D_NODISCARD static constexpr size_type max_size() noexcept
    {
        return extent;
    }

    // full -- a fixed-shape table is always exactly full (size == capacity).
    D_NODISCARD constexpr bool full() const noexcept
    {
        return true;
    }

    // --- transformation (source only: image built in a fresh table) ---

    // map
    //   the functorial mapping mu_f: applies _fn to every cell and returns a
    // same-shape static_table over the image type sigma = f(tau).  Size and
    // arrangement are preserved exactly (|mu_f| = |T|, positions unmoved); a
    // non-monotone or non-injective _fn may break sortedness or uniqueness, per
    // the axis.  static_table is a transform SOURCE -- its fixed inline store
    // cannot receive a re-typed image in place -- so the result is built fresh;
    // being compile-time iterable, a constexpr _fn makes this a functional
    // transformation at stage c (a new constant).
    template<typename _Fn>
    D_NODISCARD constexpr
    static_table<clean_t<decltype(std::declval<_Fn&>()(std::declval<const _Type&>()))>,
                 _Rows, _Cols>
    map(_Fn _fn) const
    {
        using mapped_cell =
            clean_t<decltype(std::declval<_Fn&>()(std::declval<const _Type&>()))>;

        std::array<mapped_cell, (_Rows * _Cols)> out{};

        // rewrite each cell by its image, in row-major order
        for (std::size_t i = 0; i < (_Rows * _Cols); ++i)
        {
            out[i] = _fn(m_cells[i]);
        }

        return static_table<mapped_cell, _Rows, _Cols>(out);
    }

    // --- serialization (Serialization: shape + cells, under the serial options) ---

    // encode_into_e
    //   the parameterised member enc_tau: writes the SHAPE -- rows then cols,
    // each a length field per <_L, _E> (put_length) -- then the cells in row-major
    // order, each a leaf under enc_tau<_E> (encode_leaf_into).  This is where the
    // container-serial options (byte order _E, count width _L) actually reach a
    // table's own bytes; the non-parameterised member below fixes the default.
    template<serial_endian _E,
             serial_length  _L,
             typename       _Sink>
    void encode_into_e(_Sink& _sink) const
    {
        // shape: a nested container encodes its shape (the two extents), each a
        // length field in the chosen width and byte order
        internal::put_length<_L, _E>(_sink, static_cast<std::uint64_t>(rows()));
        internal::put_length<_L, _E>(_sink, static_cast<std::uint64_t>(cols()));

        // cells, row-major, each a leaf in the chosen byte order
        for (const_pointer p = data(); p != (data() + this->size()); ++p)
        {
            encode_leaf_into<_E>(_sink, *p);
        }

        return;
    }

    // decode_e
    //   the parameterised member dec_tau: reads the shape (per <_L,_E>), checks it
    // matches this fixed R x C type, then reads R*C cells (per enc_tau<_E>) and
    // rebuilds -- a fresh object equal at the serialised level.
    template<serial_endian _E,
             serial_length  _L>
    static decode_result<static_table> decode_e(byte_reader& _reader)
    {
        std::uint64_t _r = 0;
        std::uint64_t _c = 0;

        if (!internal::get_length<_L, _E>(_reader, _r))
        {
            return decode_failure<static_table>();
        }
        if (!internal::get_length<_L, _E>(_reader, _c))
        {
            return decode_failure<static_table>();
        }

        // the stream's shape must match this type's fixed shape
        if ( (_r != static_cast<std::uint64_t>(_Rows)) ||
             (_c != static_cast<std::uint64_t>(_Cols)) )
        {
            return decode_failure<static_table>();
        }

        // R*C cells, row-major; the first short/invalid cell fails the decode
        storage_type _cells{};
        for (std::size_t i = 0; i < (_Rows * _Cols); ++i)
        {
            decode_result<_Type> _cell = decode_leaf<_E, _Type>(_reader);
            if (!_cell.ok) { return decode_failure<static_table>(); }

            _cells[i] = static_cast<_Type&&>(_cell.value);
        }

        return decode_success(static_table(_cells));
    }

    // encode_into / decode
    //   the foundational member surface: the default encoding (big-endian scalars,
    // an 8-byte count), delegating to the parameterised pair.  This is what the
    // leaf member surface and the option front ends observe -- a member encoder
    // owns its byte layout and so is endian-agnostic to the generic recursion, so
    // encode_using<E,L,F>(t) yields THIS default; call encode_into_e<E,L> directly
    // for another (endian, length).
    template<typename _Sink>
    void encode_into(_Sink& _sink) const
    {
        this->template encode_into_e<serial_endian::big,
                                     serial_length::u64>(_sink);

        return;
    }

    static decode_result<static_table> decode(byte_reader& _reader)
    {
        return decode_e<serial_endian::big, serial_length::u64>(_reader);
    }

private:
    storage_type m_cells;
};


// ---------------------------------------------------------------------------
// axis conformance -- the framework's own structural classifiers must agree
// with the axis positions declared above (checked on a representative
// instantiation, since a self-detecting static_assert needs a complete type).
// ---------------------------------------------------------------------------
namespace table_axis_conformance
{
    using static_table_probe = static_table<int, 2, 3>;

    // Iterability: iterable, and const-only (immutable -> no settable cursor).
    static_assert(is_iterable_container_v<static_table_probe>,
                  "static_table must classify as iterable.");
    static_assert(iteration_mode_of<static_table_probe>::value
                      == iteration_mode::const_only,
                  "static_table iteration must be const-only (immutable).");

    // Boundedness: fixed capacity kappa = R*C < infinity.
    static_assert(is_bounded_container_v<static_table_probe>,
                  "static_table must classify as bounded.");
    static_assert(!is_unbounded_container_v<static_table_probe>,
                  "static_table must not classify as unbounded.");

    // Multiplicity: comparator-less, position-keyed -> sequence (m = infinity).
    static_assert(multiplicity_kind_of<static_table_probe>::value
                      == multiplicity_kind::sequence,
                  "static_table must classify as a sequence (m = infinity).");

    // Sortedness / Ordering: ordered, and order-dependent (not sorted in itself).
    static_assert(is_ordered_container_v<static_table_probe>,
                  "static_table must classify as ordered.");
    static_assert(sortedness_of<static_table_probe>::value
                      == sortedness::order_dependent,
                  "static_table must classify as order-dependent (unsorted).");

    // Structure: uniformly nested (rank-2), so hierarchical -- the opt-in tag
    // overrides the flat-storage depth heuristic.
    static_assert(is_hierarchical_container_v<static_table_probe>,
                  "static_table must classify as hierarchical (uniformly nested).");
    static_assert(!is_flat_container_v<static_table_probe>,
                  "static_table must not classify as flat.");
    static_assert(structure_kind_of<static_table_probe>::value
                      == structure_kind::hierarchical,
                  "static_table structure_kind must be hierarchical.");

    // Filterability and Transformability (composite, detection-only): a
    // filter/transform SOURCE.  Readable and mappable/testable, but its fixed
    // inline store has no cell-level build (push_back), so an image or selection
    // is built in a fresh container.  Verified out-of-band against
    // container_{filter,transform}_traits (is_*_source true, is_container_*
    // false, is_*_input_only true); not asserted here because those two headers
    // clash and the filter cluster requires C++17.
}


// ===========================================================================
// III. make_static_table / equality
// ===========================================================================

// make_static_table
//   function: builds a static_table<_Type, _Rows, _Cols> from R*C cell values
// given in row-major order, deducing the cell type from the first argument.
template<std::size_t _Rows,
         std::size_t _Cols,
         typename    _First,
         typename... _Rest>
D_NODISCARD constexpr
static_table<clean_t<_First>, _Rows, _Cols>
make_static_table(_First&& _first, _Rest&&... _rest)
{
    static_assert(((1 + sizeof...(_Rest)) == (_Rows * _Cols)),
                  "make_static_table: the number of cell values must equal "
                  "_Rows * _Cols.");

    return static_table<clean_t<_First>, _Rows, _Cols>(
        static_cast<_First&&>(_first),
        static_cast<_Rest&&>(_rest)...);
}

// operator== / operator!=
//   compares two static_tables cell-by-cell after a shape check (positional
// identity: cell (r,c) against cell (r,c)).
template<typename    _Type,
         std::size_t _Rows,
         std::size_t _Cols,
         typename    _D,
         typename    _S,
         typename    _I,
         typename    _CI,
         typename... _O>
D_NODISCARD constexpr bool operator==(
    const static_table<_Type, _Rows, _Cols, _D, _S, _I, _CI, _O...>& _a,
    const static_table<_Type, _Rows, _Cols, _D, _S, _I, _CI, _O...>& _b)
{
    return _a.content_equals(_b);
}

template<typename    _Type,
         std::size_t _Rows,
         std::size_t _Cols,
         typename    _D,
         typename    _S,
         typename    _I,
         typename    _CI,
         typename... _O>
D_NODISCARD constexpr bool operator!=(
    const static_table<_Type, _Rows, _Cols, _D, _S, _I, _CI, _O...>& _a,
    const static_table<_Type, _Rows, _Cols, _D, _S, _I, _CI, _O...>& _b)
{
    return !(_a == _b);
}


NS_END  // djinterp


// ---------------------------------------------------------------------------
// _RowType (axis-typed / column-typed relational) extension -- NOT built here.
//
//   The column-typed relational table of the formal definition (k = 2, S = {2},
// cell type tau_{r,c} = tau_c fixed by the column) is the heterogeneous-row
// case: a row is a record, not a run of one cell type.  That specialisation
// takes a _RowType (a tuple/record describing the columns) in place of the
// cell-homogeneous _Type here, and its column projection returns a
// single-column-typed view.  It layers on the tuple's dependent (record) form
// and belongs with the Overlays axis (keying a coordinate); it is a planned
// sibling, deliberately outside this cell-homogeneous module.
// ---------------------------------------------------------------------------


#endif  // DJINTERP_CONTAINER_STATIC_TABLE_
