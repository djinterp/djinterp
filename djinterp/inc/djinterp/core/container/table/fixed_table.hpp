/******************************************************************************
* djinterp [container]                                         fixed_table.hpp
*
*   fixed_table -- the fixed-shape mutable grid of the table trio.  A rank-2,
* rectangular, cell-homogeneous table whose two extents R and C are fixed by
* the type, held in inline storage, whose cells may be overwritten but whose
* shape never changes.  It is the table analogue of `std::array` lifted to two
* coordinates: value-mutable, no growth.
*
*   ITS PLACE ON THE FIRST FEW AXES (Part I, in order):
*   - Lifetime  : mutable_storage  -- an ordinary runtime object.
*   - Storage   : static_storage   -- an inline std::array, no allocation; the
*                                     compile-time bounds are exactly what an
*                                     inline table requires.
*   - Mutability: element_mutable  -- a cell value v_i may be overwritten with
*                                     I_T left fixed, but no row or column may be
*                                     added or removed (structural mutation would
*                                     change I_T and needs dynamic storage).
*   It is ordered, bounded (|T| = R*C, a compile-time constant), and iterable.
*
*   RELATION TO THE SIBLINGS:
*   static_table freezes Mutability to immutable (a compile-time constant);
* table relaxes Storage to dynamic and Mutability to fully_mutable (rows and
* columns may be added and removed).  The read-only surface is inherited from
* table_base; this class adds the writable half -- non-const cell, row, and
* column access, non-const cell and row iteration, whole-grid fill, and swap.
*
*   PORTABILITY:
*   C++14 baseline (relaxed constexpr for the mutating accessors).
*
*
* path:      /inc/djinterp/core/container/table/fixed_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.04
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_fixed_table (detection trait)
II.   fixed_table (class)
III.  make_fixed_table / equality / swap
*/

#ifndef DJINTERP_CONTAINER_FIXED_TABLE_
#define DJINTERP_CONTAINER_FIXED_TABLE_ 1

// std
#include <algorithm>
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


// DJINTERP_TABLE_MUT_CONSTEXPR
//   constexpr on a MUTATING member only where C++14 relaxed constexpr allows a
// non-const member to be constexpr; empty in C++11.  Undefined at end of file.
#if ( D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL >= 201304L )
    #define DJINTERP_TABLE_MUT_CONSTEXPR  D_CONSTEXPR
#else
    #define DJINTERP_TABLE_MUT_CONSTEXPR
#endif


NS_DJINTERP


// ===========================================================================
// I.   is_fixed_table (detection trait)
// ===========================================================================

// fixed_table (fwd)
//   class: forward declaration for the detection trait below.
template<typename    _Type,
         std::size_t _Rows,
         std::size_t _Cols,
         typename    _DifferenceType,
         typename    _SizeType,
         typename    _Iterator,
         typename    _ConstIterator,
         typename... _Options>
class fixed_table;

// is_fixed_table
//   trait: true when _Type (after stripping cv/ref) is a specialization of
// fixed_table.
NS_INTERNAL

    template<typename _Type>
    struct is_fixed_table_impl : std::false_type
    {};

    template<typename    _T,
             std::size_t _R,
             std::size_t _C,
             typename    _D,
             typename    _S,
             typename    _I,
             typename    _CI,
             typename... _O>
    struct is_fixed_table_impl<fixed_table<_T, _R, _C, _D, _S, _I, _CI, _O...>>
        : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_fixed_table : internal::is_fixed_table_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
// is_fixed_table_v
//   value: variable-template shorthand for is_fixed_table<_Type>::value.
template<typename _Type>
inline constexpr bool is_fixed_table_v = is_fixed_table<_Type>::value;
#endif


// ===========================================================================
// II.  fixed_table (class)
// ===========================================================================

// fixed_table
//   class: a fixed-shape mutable rank-2 cell-homogeneous table of _Rows by
// _Cols cells, stored row-major in an inline std::array.  Inherits the const
// surface from table_base and adds the writable half; the shape is frozen.
template<typename    _Type,
         std::size_t _Rows,
         std::size_t _Cols,
         typename    _DifferenceType = std::ptrdiff_t,
         typename    _SizeType       = std::size_t,
         typename    _Iterator       = _Type*,
         typename    _ConstIterator  = const _Type*,
         typename... _Options>
class fixed_table
    : public table_base<fixed_table<_Type,
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
    using base_type = table_base<fixed_table,
                                 _Type,
                                 _SizeType,
                                 _DifferenceType>;

    // the flat, row-major cell store; std::array<_,0> is well-formed.
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

    // cell iteration: a mutable cursor and its const face.
    using iterator        = _Iterator;
    using const_iterator  = _ConstIterator;

    // writable subtable T[r] and projection T[*,c], plus their const faces
    // (the const faces are inherited from table_base and re-exported here).
    using row_type           = internal::basic_row_view<_Type>;
    using const_row_type     = typename base_type::const_row_type;
    using column_type        = internal::basic_column_view<_Type>;
    using const_column_type  = typename base_type::const_column_type;
    using row_iterator       = internal::row_cursor<_Type>;
    using const_row_iterator = typename base_type::const_row_iterator;

    // --- axis positions (the first few axes, then the ones that follow) ---

    static constexpr container_lifetime      lifetime      =
        container_lifetime::mutable_storage;
    static constexpr container_storage_kind  storage_kind  =
        container_storage_kind::static_storage;
    static constexpr mutability              mutability_grade =
        mutability::element_mutable;
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

    // Iterability stage sub-axis: structure is compile-time-expressible but the
    // contents are filled at runtime, so the cursor is statically evaluable
    // while the values are not (the "fixed array, runtime fill" row).
    static constexpr bool compile_time_iterable = true;
    static constexpr bool compile_time_values   = false;

    // Boundedness: a fixed capacity kappa = R*C < infinity.  `extent` is the
    // djinterp compile-time fixed-capacity signal; the domain is left free.
    static constexpr size_type extent = static_cast<size_type>(_Rows * _Cols);
    static constexpr size_type max_cells = extent;

    static constexpr size_type row_extent    = static_cast<size_type>(_Rows);
    static constexpr size_type column_extent = static_cast<size_type>(_Cols);

    // --- construction ---

    // default: value-initializes every cell to _Type{}.
    constexpr fixed_table()
        : m_cells{}
    {}

    // element-wise: exactly R*C cell values in row-major order.  Constrained to
    // two-or-more cells so it never shadows the copy, move, or array forms.
    template<typename... _Cells,
             typename = typename std::enable_if<
                 ( (sizeof...(_Cells) == (_Rows * _Cols)) &&
                   (sizeof...(_Cells) >= 2) )>::type>
    constexpr fixed_table(_Cells&&... _cells)
        : m_cells{ { static_cast<_Type>(static_cast<_Cells&&>(_cells))... } }
    {}

    // from a flat, row-major std::array of the exact cell count.
    explicit constexpr fixed_table(const storage_type& _cells)
        : m_cells(_cells)
    {}

    constexpr fixed_table(const fixed_table&)   = default;
    constexpr fixed_table(fixed_table&&)        = default;
    fixed_table& operator=(const fixed_table&)  = default;
    fixed_table& operator=(fixed_table&&)       = default;
    ~fixed_table()                              = default;

    // filled
    //   factory: a table with every cell equal to _value.
    static DJINTERP_TABLE_MUT_CONSTEXPR fixed_table filled(const _Type& _value)
    {
        fixed_table t;
        t.fill(_value);

        return t;
    }

    // --- table_base hooks (contiguous row-major buffer + the two extents) ---

    D_NODISCARD DJINTERP_TABLE_MUT_CONSTEXPR pointer data() noexcept
    {
        return m_cells.data();
    }

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

    D_NODISCARD static constexpr size_type capacity() noexcept
    {
        return extent;
    }

    D_NODISCARD static constexpr size_type max_size() noexcept
    {
        return extent;
    }

    // full -- a fixed-shape table is always exactly full (size == capacity).
    D_NODISCARD constexpr bool full() const noexcept
    {
        return true;
    }

    // --- writable cell access (element mutation, I_T left fixed) ---

    // re-export the const observers table_base supplies so declaring the
    // non-const overloads below does not hide them.
    using base_type::at;
    using base_type::operator();
    using base_type::operator[];
    using base_type::row;
    using base_type::column;
    using base_type::begin;
    using base_type::end;
    using base_type::row_begin;
    using base_type::row_end;

    // operator() -- unchecked writable cell at (_r, _c), row-major.
    D_NODISCARD DJINTERP_TABLE_MUT_CONSTEXPR reference operator()(
        size_type _r,
        size_type _c
    )
    {
        return m_cells[(_r * _Cols) + _c];
    }

    // at -- checked writable cell; throws std::out_of_range off the domain.
    D_NODISCARD DJINTERP_TABLE_MUT_CONSTEXPR reference at(
        size_type _r,
        size_type _c
    )
    {
        // reject an index outside the rectangular domain I_T
        if (!this->contains(_r, _c))
        {
            throw std::out_of_range("fixed_table::at");
        }

        return m_cells[(_r * _Cols) + _c];
    }

    // --- writable subtable and projection ---

    // row / operator[] -- the writable rank-1 subtable T[r].
    D_NODISCARD DJINTERP_TABLE_MUT_CONSTEXPR row_type row(size_type _r)
    {
        return row_type(m_cells.data() + (_r * _Cols), _Cols);
    }

    D_NODISCARD DJINTERP_TABLE_MUT_CONSTEXPR row_type operator[](size_type _r)
    {
        return row(_r);
    }

    // column -- the writable projection T[*,c] (strided).
    D_NODISCARD DJINTERP_TABLE_MUT_CONSTEXPR column_type column(size_type _c)
    {
        return column_type(m_cells.data() + _c,
                           _Rows,
                           static_cast<difference_type>(_Cols));
    }

    // --- writable cell and row iteration ---

    D_NODISCARD DJINTERP_TABLE_MUT_CONSTEXPR iterator begin() noexcept
    {
        return m_cells.data();
    }

    D_NODISCARD DJINTERP_TABLE_MUT_CONSTEXPR iterator end() noexcept
    {
        return m_cells.data() + (_Rows * _Cols);
    }

    D_NODISCARD DJINTERP_TABLE_MUT_CONSTEXPR row_iterator row_begin() noexcept
    {
        return row_iterator(m_cells.data(), 0, _Cols);
    }

    D_NODISCARD DJINTERP_TABLE_MUT_CONSTEXPR row_iterator row_end() noexcept
    {
        return row_iterator(m_cells.data(),
                            static_cast<difference_type>(_Rows),
                            _Cols);
    }

    // --- whole-grid mutation (shape-preserving) ---

    // fill
    //   writes _value into every cell, leaving I_T fixed.
    DJINTERP_TABLE_MUT_CONSTEXPR void fill(const _Type& _value)
    {
        // overwrite every cell in row-major order
        for (std::size_t i = 0; i < (_Rows * _Cols); ++i)
        {
            m_cells[i] = _value;
        }

        return;
    }

    // swap
    //   exchanges the cells of two same-shape tables.
    DJINTERP_TABLE_MUT_CONSTEXPR void swap(fixed_table& _other) noexcept
    {
        m_cells.swap(_other.m_cells);

        return;
    }

    // --- sortedness: impose comparator order on the rows (shape-preserving) ---

    // sort_rows
    //   stably reorders the rows so consecutive rows are non-decreasing under
    // _cmp(const_row_type a, const_row_type b) -- the sorted overlay on the row
    // dimension (relational ORDER BY).  It permutes which values sit at existing
    // positions and leaves I_T (the R x C shape) fixed, so it is pure element
    // mutation, within reach of an element-mutable table.
    template<typename _RowCompare>
    void sort_rows(_RowCompare _cmp)
    {
        // a table with no rows or a single row is trivially in order
        if (_Rows < 2)
        {
            return;
        }

        const fixed_table& cself = *this;

        // build and stably sort a row-index permutation by the comparator
        std::array<size_type, _Rows> perm{};
        for (size_type i = 0; i < _Rows; ++i)
        {
            perm[i] = i;
        }

        std::stable_sort(perm.begin(), perm.end(),
            [&cself, &_cmp](size_type _a, size_type _b)
            {
                return _cmp(cself.row(_a), cself.row(_b));
            });

        // materialize the reordered grid, then adopt it (I_T unchanged)
        storage_type reordered{};
        for (size_type i = 0; i < _Rows; ++i)
        {
            const size_type src = perm[i] * static_cast<size_type>(_Cols);
            const size_type dst = i       * static_cast<size_type>(_Cols);

            for (size_type c = 0; c < _Cols; ++c)
            {
                reordered[dst + c] = m_cells[src + c];
            }
        }

        m_cells = reordered;

        return;
    }

    // --- transformation (source; same-type map may run in place) ---

    // map
    //   the functorial mapping mu_f: applies _fn to every cell and returns a
    // same-shape fixed_table over the image type sigma = f(tau).  Size and
    // arrangement are preserved exactly; a non-monotone or non-injective _fn may
    // break sortedness or uniqueness.  A fixed_table is a transform SOURCE when
    // the element type changes (its fixed inline store cannot hold a re-typed
    // image in place), so the retyped result is built fresh here.
    template<typename _Fn>
    D_NODISCARD constexpr
    fixed_table<clean_t<decltype(std::declval<_Fn&>()(std::declval<const _Type&>()))>,
                _Rows, _Cols>
    map(_Fn _fn) const
    {
        using mapped_cell =
            clean_t<decltype(std::declval<_Fn&>()(std::declval<const _Type&>()))>;

        std::array<mapped_cell, (_Rows * _Cols)> out{};

        for (std::size_t i = 0; i < (_Rows * _Cols); ++i)
        {
            out[i] = _fn(m_cells[i]);
        }

        return fixed_table<mapped_cell, _Rows, _Cols>(out);
    }

    // map_inplace
    //   the element-preserving mapping mu_f with sigma = tau, applied in place:
    // each cell is overwritten by its image.  This is the native, shape- and
    // size-preserving transform an element-mutable fixed grid supports without
    // building a new container; only the values change, so I_T stays fixed.
    template<typename _Fn>
    DJINTERP_TABLE_MUT_CONSTEXPR void map_inplace(_Fn _fn)
    {
        // the rewrite must be closed on the element type (result assignable to tau)
        for (std::size_t i = 0; i < (_Rows * _Cols); ++i)
        {
            m_cells[i] = static_cast<_Type>(_fn(m_cells[i]));
        }

        return;
    }

    // --- serialization (Serialization: shape + cells, under the serial options) ---

    // encode_into_e / decode_e
    //   the parameterised member enc_tau / dec_tau: the SHAPE (rows then cols,
    // each a length field per <_L,_E>) and the cells (each a leaf under
    // enc_tau<_E>).  This is where the container-serial options reach a table's
    // own bytes.
    template<serial_endian _E,
             serial_length  _L,
             typename       _Sink>
    void encode_into_e(_Sink& _sink) const
    {
        internal::put_length<_L, _E>(_sink, static_cast<std::uint64_t>(rows()));
        internal::put_length<_L, _E>(_sink, static_cast<std::uint64_t>(cols()));

        for (const_pointer p = data(); p != (data() + this->size()); ++p)
        {
            encode_leaf_into<_E>(_sink, *p);
        }

        return;
    }

    template<serial_endian _E,
             serial_length  _L>
    static decode_result<fixed_table> decode_e(byte_reader& _reader)
    {
        std::uint64_t _r = 0;
        std::uint64_t _c = 0;

        if (!internal::get_length<_L, _E>(_reader, _r))
        {
            return decode_failure<fixed_table>();
        }
        if (!internal::get_length<_L, _E>(_reader, _c))
        {
            return decode_failure<fixed_table>();
        }

        // the stream's shape must match this type's fixed shape
        if ( (_r != static_cast<std::uint64_t>(_Rows)) ||
             (_c != static_cast<std::uint64_t>(_Cols)) )
        {
            return decode_failure<fixed_table>();
        }

        storage_type _cells{};
        for (std::size_t i = 0; i < (_Rows * _Cols); ++i)
        {
            decode_result<_Type> _cell = decode_leaf<_E, _Type>(_reader);
            if (!_cell.ok) { return decode_failure<fixed_table>(); }

            _cells[i] = static_cast<_Type&&>(_cell.value);
        }

        return decode_success(fixed_table(_cells));
    }

    // encode_into / decode
    //   the foundational member surface: the default (big-endian, 8-byte count)
    // delegating to the parameterised pair.  This is what the option front ends
    // observe; call encode_into_e<E,L> directly for another (endian, length).
    template<typename _Sink>
    void encode_into(_Sink& _sink) const
    {
        this->template encode_into_e<serial_endian::big,
                                     serial_length::u64>(_sink);

        return;
    }

    static decode_result<fixed_table> decode(byte_reader& _reader)
    {
        return decode_e<serial_endian::big, serial_length::u64>(_reader);
    }

private:
    storage_type m_cells;
};


// ---------------------------------------------------------------------------
// axis conformance -- the framework's classifiers must agree with the declared
// positions (representative instantiation).
// ---------------------------------------------------------------------------
namespace table_axis_conformance
{
    using fixed_table_probe = fixed_table<int, 2, 3>;

    // Iterability: iterable, and non-const (element-mutable -> settable cursor).
    static_assert(is_iterable_container_v<fixed_table_probe>,
                  "fixed_table must classify as iterable.");
    static_assert(iteration_mode_of<fixed_table_probe>::value
                      == iteration_mode::non_const,
                  "fixed_table iteration must be non-const (element-mutable).");

    // Boundedness: fixed capacity kappa = R*C < infinity.
    static_assert(is_bounded_container_v<fixed_table_probe>,
                  "fixed_table must classify as bounded.");
    static_assert(!is_unbounded_container_v<fixed_table_probe>,
                  "fixed_table must not classify as unbounded.");

    // Multiplicity: comparator-less, position-keyed -> sequence (m = infinity).
    static_assert(multiplicity_kind_of<fixed_table_probe>::value
                      == multiplicity_kind::sequence,
                  "fixed_table must classify as a sequence (m = infinity).");

    // Sortedness / Ordering: ordered, and order-dependent (not sorted in itself).
    static_assert(is_ordered_container_v<fixed_table_probe>,
                  "fixed_table must classify as ordered.");
    static_assert(sortedness_of<fixed_table_probe>::value
                      == sortedness::order_dependent,
                  "fixed_table must classify as order-dependent (unsorted).");

    // Structure: uniformly nested (rank-2) -> hierarchical.
    static_assert(is_hierarchical_container_v<fixed_table_probe>,
                  "fixed_table must classify as hierarchical (uniformly nested).");
    static_assert(!is_flat_container_v<fixed_table_probe>,
                  "fixed_table must not classify as flat.");
    static_assert(structure_kind_of<fixed_table_probe>::value
                      == structure_kind::hierarchical,
                  "fixed_table structure_kind must be hierarchical.");

    // Filterability and Transformability (composite, detection-only): a
    // filter/transform SOURCE.  No cell-level build (push_back), so a re-typed
    // image or a selection is built in a fresh container; a same-type map may
    // still run in place (map_inplace), a native shape-preserving rewrite.
    // Verified out-of-band (see the note in static_table.hpp).
}


// ===========================================================================
// III. make_fixed_table / equality / swap
// ===========================================================================

// make_fixed_table
//   function: builds a fixed_table<_Type, _Rows, _Cols> from R*C cell values
// given in row-major order, deducing the cell type from the first argument.
template<std::size_t _Rows,
         std::size_t _Cols,
         typename    _First,
         typename... _Rest>
D_NODISCARD constexpr
fixed_table<clean_t<_First>, _Rows, _Cols>
make_fixed_table(_First&& _first, _Rest&&... _rest)
{
    static_assert(((1 + sizeof...(_Rest)) == (_Rows * _Cols)),
                  "make_fixed_table: the number of cell values must equal "
                  "_Rows * _Cols.");

    return fixed_table<clean_t<_First>, _Rows, _Cols>(
        static_cast<_First&&>(_first),
        static_cast<_Rest&&>(_rest)...);
}

// operator== / operator!=
//   compares two fixed_tables cell-by-cell after a shape check.
template<typename    _Type,
         std::size_t _Rows,
         std::size_t _Cols,
         typename    _D,
         typename    _S,
         typename    _I,
         typename    _CI,
         typename... _O>
D_NODISCARD constexpr bool operator==(
    const fixed_table<_Type, _Rows, _Cols, _D, _S, _I, _CI, _O...>& _a,
    const fixed_table<_Type, _Rows, _Cols, _D, _S, _I, _CI, _O...>& _b)
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
    const fixed_table<_Type, _Rows, _Cols, _D, _S, _I, _CI, _O...>& _a,
    const fixed_table<_Type, _Rows, _Cols, _D, _S, _I, _CI, _O...>& _b)
{
    return !(_a == _b);
}

// swap
//   free swap for two same-shape fixed_tables.
template<typename    _Type,
         std::size_t _Rows,
         std::size_t _Cols,
         typename    _D,
         typename    _S,
         typename    _I,
         typename    _CI,
         typename... _O>
DJINTERP_TABLE_MUT_CONSTEXPR void swap(
    fixed_table<_Type, _Rows, _Cols, _D, _S, _I, _CI, _O...>& _a,
    fixed_table<_Type, _Rows, _Cols, _D, _S, _I, _CI, _O...>& _b) noexcept
{
    _a.swap(_b);

    return;
}


NS_END  // djinterp


#undef DJINTERP_TABLE_MUT_CONSTEXPR


#endif  // DJINTERP_CONTAINER_FIXED_TABLE_
