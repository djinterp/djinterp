/******************************************************************************
* djinterp [container]                                               table.hpp
*
*   table -- the fully dynamic member of the table trio.  A rank-2,
* rectangular, cell-homogeneous table whose two extents are read and changed at
* runtime: cells may be overwritten, and whole rows and columns may be added or
* removed.  It is the table analogue of `std::vector` lifted to two coordinates,
* kept rectangular (every row the same width).
*
*   ITS PLACE ON THE FIRST FEW AXES (Part I, in order):
*   - Lifetime  : mutable_storage  -- an ordinary runtime object.
*   - Storage   : dynamic_storage  -- a heap-backed std::vector; a table whose
*                                     row count is read at runtime is dynamic.
*   - Mutability: fully_mutable    -- both capabilities: element mutation
*                                     overwrites a cell value with I_T fixed, and
*                                     structural mutation changes I_T itself
*                                     (appending a row alters a bound function,
*                                     hence the domain), which dynamic storage
*                                     and spare capacity allow.
*   It is ordered (row-major = lexicographic on multi-indices), unbounded (|T|
* is capped only by available capacity), and iterable.
*
*   RECTANGULARITY:
*   The domain is kept a box I_T = {0..rows-1} x {0..cols-1}: every inserted row
* must match the current width and every inserted column the current height.  A
* jagged table (rows of differing widths) is a distinct, planned container; this
* one holds the rectangular invariant  cells.size() == rows * cols.  An empty
* table remembers its width, so rows may be appended to a 0-row table of known
* column count.
*
*   RELATION TO THE SIBLINGS:
*   static_table and fixed_table fix the shape at compile time (immutable and
* element-mutable respectively) in inline storage.  The read-only surface is
* inherited from table_base; this class adds element mutation and the full
* structural surface.
*
*   INVALIDATION:
*   Cell pointers, iterators, and row/column views are backed by the vector's
* buffer and are invalidated by any structural change or reallocation, exactly
* as for std::vector.
*
*   PORTABILITY:
*   C++14 baseline.
*
*
* path:      /inc/djinterp/core/container/table/table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.04
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_table (detection trait)
II.   table (class)
      1. member types and axis positions
      2. construction
      3. table_base hooks
      4. element access (writable)
      5. subtable / projection (writable)
      6. cell / row iteration (writable)
      7. row structural operations
      8. column structural operations
      9. reshape / capacity / whole-grid
III.  equality / swap
*/

#ifndef DJINTERP_CONTAINER_TABLE_
#define DJINTERP_CONTAINER_TABLE_ 1

// std
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"                     // NS_*, D_CONSTEXPR, clean_t
#include "./table_base.hpp"                        // table_base, row/column views
#include "../container_options.hpp"                // axis enums, options base
#include "../traits/mutable_container_traits.hpp"  // mutability grade
#include "../serial/encode_options.hpp"             // enc_tau<E>, put_length<L,E>, serial enums
#include "../serial/decode_options.hpp"             // dec_tau<E>, get_length<L,E>


NS_DJINTERP


// ===========================================================================
// I.   is_table (detection trait)
// ===========================================================================

// table (fwd)
//   class: forward declaration for the detection trait below.
template<typename    _Type,
         typename    _DifferenceType,
         typename    _SizeType,
         typename    _Iterator,
         typename    _ConstIterator,
         typename... _Options>
class table;

// is_table
//   trait: true when _Type (after stripping cv/ref) is a specialization of
// table.
NS_INTERNAL

    template<typename _Type>
    struct is_table_impl : std::false_type
    {};

    template<typename    _T,
             typename    _D,
             typename    _S,
             typename    _I,
             typename    _CI,
             typename... _O>
    struct is_table_impl<table<_T, _D, _S, _I, _CI, _O...>>
        : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_table : internal::is_table_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_table_v
//   value: variable-template shorthand for is_table<_Type>::value.
template<typename _Type>
inline constexpr bool is_table_v = is_table<_Type>::value;
#endif


// ===========================================================================
// II.  table (class)
// ===========================================================================

// table
//   class: a dynamic, fully-mutable rank-2 cell-homogeneous table, cells stored
// row-major in a std::vector.  Inherits the const surface from table_base and
// adds element mutation plus row and column structural operations, holding the
// rectangular invariant  cells.size() == rows * cols.
template<typename    _Type,
         typename    _DifferenceType = std::ptrdiff_t,
         typename    _SizeType       = std::size_t,
         typename    _Iterator       = _Type*,
         typename    _ConstIterator  = const _Type*,
         typename... _Options>
class table
    : public table_base<table<_Type,
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
    using base_type    = table_base<table,
                                    _Type,
                                    _SizeType,
                                    _DifferenceType>;
    using storage_type = std::vector<_Type>;

public:
    // --- 1. member types and axis positions ---

    using value_type      = _Type;
    using cell_type       = _Type;
    using size_type       = _SizeType;
    using difference_type = _DifferenceType;
    using reference       = _Type&;
    using const_reference = const _Type&;
    using pointer         = _Type*;
    using const_pointer   = const _Type*;

    using iterator        = _Iterator;
    using const_iterator  = _ConstIterator;

    using row_type           = internal::basic_row_view<_Type>;
    using const_row_type     = typename base_type::const_row_type;
    using column_type        = internal::basic_column_view<_Type>;
    using const_column_type  = typename base_type::const_column_type;
    using row_iterator       = internal::row_cursor<_Type>;
    using const_row_iterator = typename base_type::const_row_iterator;

    static constexpr container_lifetime      lifetime      =
        container_lifetime::mutable_storage;
    static constexpr container_storage_kind  storage_kind  =
        container_storage_kind::dynamic_storage;
    static constexpr mutability              mutability_grade =
        mutability::fully_mutable;
    static constexpr container_ordering      ordering      =
        container_ordering::ordered;
    static constexpr container_bounds        bounds        =
        container_bounds::unbounded;
    static constexpr container_iterability   iterability   =
        container_iterability::iterable;
    static constexpr container_multiplicity  multiplicity_grade =
        container_multiplicity::multi;   // m = infinity; cells keyed by position
    static constexpr container_structure     structure     =
        container_structure::hierarchical;  // uniformly nested; depth = rank = 2

    // Iterability stage sub-axis: the row count is a runtime value, so neither
    // the cursor nor the values are compile-time-expressible (the "vector /
    // list" row of the Iterability table) -- runtime-iterable only.
    static constexpr bool compile_time_iterable = false;
    static constexpr bool compile_time_values   = false;

    // --- 2. construction ---

    // default: an empty 0 x 0 table.
    table()
        : m_cells(),
          m_rows(0),
          m_cols(0)
    {}

    // dimensioned: a _rows x _cols table with every cell value-initialized.
    table(
        size_type _rows,
        size_type _cols
    )
        : m_cells(static_cast<std::size_t>(_rows) * static_cast<std::size_t>(_cols)),
          m_rows(_rows),
          m_cols(_cols)
    {}

    // dimensioned + fill: a _rows x _cols table with every cell equal to _value.
    table(
        size_type     _rows,
        size_type     _cols,
        const _Type&  _value
    )
        : m_cells(static_cast<std::size_t>(_rows) * static_cast<std::size_t>(_cols),
                  _value),
          m_rows(_rows),
          m_cols(_cols)
    {}

    // nested rows: table<int>{ {1,2,3}, {4,5,6} }.  Every row must share the
    // first row's width, or the rectangular invariant is violated.
    table(std::initializer_list<std::initializer_list<_Type>> _rows)
        : m_cells(),
          m_rows(0),
          m_cols(0)
    {
        // width is set by the first row; 0 rows leaves a 0 x 0 table
        if (_rows.size() != 0)
        {
            m_cols = static_cast<size_type>(_rows.begin()->size());
        }

        // append each row, checking it against the fixed width
        for (const std::initializer_list<_Type>& r : _rows)
        {
            if (static_cast<size_type>(r.size()) != m_cols)
            {
                throw std::invalid_argument(
                    "table: all rows must have the same width (rectangular).");
            }

            m_cells.insert(m_cells.end(), r.begin(), r.end());
            ++m_rows;
        }
    }

    table(const table&)            = default;
    table(table&&)                 = default;
    table& operator=(const table&) = default;
    table& operator=(table&&)      = default;
    ~table()                       = default;

    // from_flat
    //   factory: adopts a row-major cell buffer, split into _rows x _cols.  The
    // buffer size must equal _rows * _cols.
    static table from_flat(
        storage_type _flat,
        size_type    _rows,
        size_type    _cols
    )
    {
        // the buffer must exactly fill the requested shape
        if (_flat.size() != (static_cast<std::size_t>(_rows) *
                             static_cast<std::size_t>(_cols)))
        {
            throw std::invalid_argument(
                "table::from_flat: buffer size must equal _rows * _cols.");
        }

        table t;
        t.m_cells = static_cast<storage_type&&>(_flat);
        t.m_rows  = _rows;
        t.m_cols  = _cols;

        return t;
    }

    // --- 3. table_base hooks (contiguous row-major buffer + the two extents) ---

    D_NODISCARD pointer data() noexcept
    {
        return m_cells.data();
    }

    D_NODISCARD const_pointer data() const noexcept
    {
        return m_cells.data();
    }

    D_NODISCARD size_type rows() const noexcept
    {
        return m_rows;
    }

    D_NODISCARD size_type cols() const noexcept
    {
        return m_cols;
    }

    // capacity in whole rows the current buffer can hold without reallocating.
    D_NODISCARD size_type row_capacity() const noexcept
    {
        // an unknown width cannot express a row capacity
        if (m_cols == 0)
        {
            return 0;
        }

        return static_cast<size_type>(m_cells.capacity() /
                                      static_cast<std::size_t>(m_cols));
    }

    // --- boundedness accessors (unbounded: capacity is spare buffer, not a cap) ---

    // capacity -- cells the buffer holds without reallocating.  This is spare
    // room, not a fixed cap: a table grows past it, so it is NOT a bounding
    // signal (the paired reserve() below is the framework's growability tell).
    D_NODISCARD size_type capacity() const noexcept
    {
        return static_cast<size_type>(m_cells.capacity());
    }

    // max_size -- the largest cell count the implementation can address.
    D_NODISCARD size_type max_size() const noexcept
    {
        return static_cast<size_type>(m_cells.max_size());
    }

    // --- 4. element access (writable; re-export const observers) ---

    using base_type::at;
    using base_type::operator();
    using base_type::operator[];
    using base_type::row;
    using base_type::column;
    using base_type::begin;
    using base_type::end;
    using base_type::row_begin;
    using base_type::row_end;

    D_NODISCARD reference operator()(
        size_type _r,
        size_type _c
    )
    {
        return m_cells[(static_cast<std::size_t>(_r) *
                        static_cast<std::size_t>(m_cols)) + _c];
    }

    D_NODISCARD reference at(
        size_type _r,
        size_type _c
    )
    {
        // reject an index outside the rectangular domain I_T
        if (!this->contains(_r, _c))
        {
            throw std::out_of_range("table::at");
        }

        return m_cells[(static_cast<std::size_t>(_r) *
                        static_cast<std::size_t>(m_cols)) + _c];
    }

    // --- 5. subtable / projection (writable) ---

    D_NODISCARD row_type row(size_type _r)
    {
        return row_type(m_cells.data() + (static_cast<std::size_t>(_r) *
                                          static_cast<std::size_t>(m_cols)),
                        m_cols);
    }

    D_NODISCARD row_type operator[](size_type _r)
    {
        return row(_r);
    }

    D_NODISCARD column_type column(size_type _c)
    {
        return column_type(m_cells.data() + _c,
                           m_rows,
                           static_cast<difference_type>(m_cols));
    }

    // --- 6. cell / row iteration (writable) ---

    D_NODISCARD iterator begin() noexcept
    {
        return m_cells.data();
    }

    D_NODISCARD iterator end() noexcept
    {
        return m_cells.data() + m_cells.size();
    }

    D_NODISCARD row_iterator row_begin() noexcept
    {
        return row_iterator(m_cells.data(), 0, m_cols);
    }

    D_NODISCARD row_iterator row_end() noexcept
    {
        return row_iterator(m_cells.data(),
                            static_cast<difference_type>(m_rows),
                            m_cols);
    }

    // --- 7. row structural operations (structural mutation: I_T changes) ---

    // push_row (initializer list)
    //   appends a row at the bottom.  On the first row of a width-less table the
    // width is adopted; otherwise the row must match the current width.
    void push_row(std::initializer_list<_Type> _row)
    {
        push_row(_row.begin(), _row.end());

        return;
    }

    // push_row (range)
    //   appends the row [_first, _last).
    template<typename _InputIt>
    void push_row(
        _InputIt _first,
        _InputIt _last
    )
    {
        const std::size_t width =
            static_cast<std::size_t>(std::distance(_first, _last));

        adopt_or_check_width(width);

        m_cells.insert(m_cells.end(), _first, _last);
        ++m_rows;

        return;
    }

    // insert_row
    //   inserts a row before index _at (0 <= _at <= rows).
    void insert_row(
        size_type                    _at,
        std::initializer_list<_Type> _row)
    {
        // an insertion point past the end is out of range
        if (_at > m_rows)
        {
            throw std::out_of_range("table::insert_row");
        }

        adopt_or_check_width(_row.size());

        m_cells.insert(
            m_cells.begin() + (static_cast<std::size_t>(_at) *
                               static_cast<std::size_t>(m_cols)),
            _row.begin(),
            _row.end());
        ++m_rows;

        return;
    }

    // erase_row
    //   removes the row at index _at.  The width is retained, so the emptied
    // table can still take new rows.
    void erase_row(size_type _at)
    {
        // no such row to erase
        if (_at >= m_rows)
        {
            throw std::out_of_range("table::erase_row");
        }

        const std::size_t start =
            static_cast<std::size_t>(_at) * static_cast<std::size_t>(m_cols);

        m_cells.erase(m_cells.begin() + start,
                      m_cells.begin() + start + static_cast<std::size_t>(m_cols));
        --m_rows;

        return;
    }

    // pop_row
    //   removes the last row.
    void pop_row()
    {
        // nothing to remove
        if (m_rows == 0)
        {
            return;
        }

        erase_row(m_rows - 1);

        return;
    }

    // --- 8. column structural operations (rebuild the row-major buffer) ---

    // insert_column
    //   inserts a column before index _at (0 <= _at <= cols).  The supplied
    // cells run top-to-bottom and must number exactly rows (one per row).
    void insert_column(
        size_type                    _at,
        std::initializer_list<_Type> _column)
    {
        // an insertion point past the end is out of range
        if (_at > m_cols)
        {
            throw std::out_of_range("table::insert_column");
        }

        // one new cell per row is required (except for a 0-row table)
        if ( (m_rows != 0) &&
             (static_cast<size_type>(_column.size()) != m_rows) )
        {
            throw std::invalid_argument(
                "table::insert_column: column height must equal rows.");
        }

        // a 0-row table only widens its recorded width
        if (m_rows == 0)
        {
            ++m_cols;

            return;
        }

        const std::size_t old_cols = static_cast<std::size_t>(m_cols);
        const std::size_t new_cols = old_cols + 1;

        storage_type rebuilt;
        rebuilt.reserve(static_cast<std::size_t>(m_rows) * new_cols);

        const _Type* col_src = _column.begin();

        // rebuild row by row, splicing the new cell in at _at
        for (std::size_t r = 0; r < static_cast<std::size_t>(m_rows); ++r)
        {
            const std::size_t base = r * old_cols;

            // cells left of the insertion point
            for (std::size_t c = 0; c < static_cast<std::size_t>(_at); ++c)
            {
                rebuilt.push_back(m_cells[base + c]);
            }

            // the inserted cell for this row
            rebuilt.push_back(col_src[r]);

            // cells at and right of the insertion point
            for (std::size_t c = static_cast<std::size_t>(_at); c < old_cols; ++c)
            {
                rebuilt.push_back(m_cells[base + c]);
            }
        }

        m_cells = static_cast<storage_type&&>(rebuilt);
        m_cols  = static_cast<size_type>(new_cols);

        return;
    }

    // push_column
    //   appends a column on the right.
    void push_column(std::initializer_list<_Type> _column)
    {
        insert_column(m_cols, _column);

        return;
    }

    // erase_column
    //   removes the column at index _at.
    void erase_column(size_type _at)
    {
        // no such column to erase
        if (_at >= m_cols)
        {
            throw std::out_of_range("table::erase_column");
        }

        // a 0-row table only narrows its recorded width
        if (m_rows == 0)
        {
            --m_cols;

            return;
        }

        const std::size_t old_cols = static_cast<std::size_t>(m_cols);
        const std::size_t new_cols = old_cols - 1;

        storage_type rebuilt;
        rebuilt.reserve(static_cast<std::size_t>(m_rows) * new_cols);

        // rebuild row by row, dropping column _at
        for (std::size_t r = 0; r < static_cast<std::size_t>(m_rows); ++r)
        {
            const std::size_t base = r * old_cols;

            for (std::size_t c = 0; c < old_cols; ++c)
            {
                // skip the erased column
                if (c == static_cast<std::size_t>(_at))
                {
                    continue;
                }

                rebuilt.push_back(m_cells[base + c]);
            }
        }

        m_cells = static_cast<storage_type&&>(rebuilt);
        m_cols  = static_cast<size_type>(new_cols);

        return;
    }

    // pop_column
    //   removes the last column.
    void pop_column()
    {
        // nothing to remove
        if (m_cols == 0)
        {
            return;
        }

        erase_column(m_cols - 1);

        return;
    }

    // --- 9. reshape / capacity / whole-grid ---

    // resize
    //   reshapes to _rows x _cols, keeping the cells in the overlapping
    // top-left block and filling any new cells with _value.
    void resize(
        size_type    _rows,
        size_type    _cols,
        const _Type& _value = _Type())
    {
        // a no-op reshape avoids the rebuild
        if ( (_rows == m_rows) &&
             (_cols == m_cols) )
        {
            return;
        }

        storage_type rebuilt(static_cast<std::size_t>(_rows) *
                             static_cast<std::size_t>(_cols),
                             _value);

        const std::size_t copy_rows =
            (static_cast<std::size_t>(_rows) < static_cast<std::size_t>(m_rows))
                ? static_cast<std::size_t>(_rows)
                : static_cast<std::size_t>(m_rows);

        const std::size_t copy_cols =
            (static_cast<std::size_t>(_cols) < static_cast<std::size_t>(m_cols))
                ? static_cast<std::size_t>(_cols)
                : static_cast<std::size_t>(m_cols);

        // carry over the overlapping top-left block
        for (std::size_t r = 0; r < copy_rows; ++r)
        {
            for (std::size_t c = 0; c < copy_cols; ++c)
            {
                rebuilt[(r * static_cast<std::size_t>(_cols)) + c] =
                    m_cells[(r * static_cast<std::size_t>(m_cols)) + c];
            }
        }

        m_cells = static_cast<storage_type&&>(rebuilt);
        m_rows  = _rows;
        m_cols  = _cols;

        return;
    }

    // reserve
    //   reserves buffer capacity for _cells cells.  This is the framework's
    // growability signal (a reserve(size_type) accessor): together with a
    // capacity() that is spare room rather than a cap, it marks the table
    // unbounded, distinguishing it from the fixed-capacity static/fixed tables.
    void reserve(size_type _cells)
    {
        m_cells.reserve(static_cast<std::size_t>(_cells));

        return;
    }

    // reserve_rows
    //   reserves buffer capacity for _row_capacity whole rows (width must be
    // known).
    void reserve_rows(size_type _row_capacity)
    {
        // a width must be established before rows can be reserved
        if (m_cols == 0)
        {
            return;
        }

        m_cells.reserve(static_cast<std::size_t>(_row_capacity) *
                        static_cast<std::size_t>(m_cols));

        return;
    }

    // fill
    //   writes _value into every cell, leaving the shape fixed.
    void fill(const _Type& _value)
    {
        // overwrite every cell
        for (_Type& cell : m_cells)
        {
            cell = _value;
        }

        return;
    }

    // clear
    //   drops all rows and columns, leaving a 0 x 0 table.
    void clear() noexcept
    {
        m_cells.clear();
        m_rows = 0;
        m_cols = 0;

        return;
    }

    // shrink_to_fit
    //   requests release of unused buffer capacity.
    void shrink_to_fit()
    {
        m_cells.shrink_to_fit();

        return;
    }

    // swap
    //   exchanges the contents and shapes of two tables.
    void swap(table& _other) noexcept
    {
        m_cells.swap(_other.m_cells);

        size_type tr = m_rows;
        m_rows        = _other.m_rows;
        _other.m_rows = tr;

        size_type tc = m_cols;
        m_cols        = _other.m_cols;
        _other.m_cols = tc;

        return;
    }

    // sort_rows
    //   stably reorders the rows so consecutive rows are non-decreasing under
    // _cmp(const_row_type a, const_row_type b) -- the sorted overlay on the row
    // dimension (relational ORDER BY).  It permutes which values sit at existing
    // positions and leaves I_T fixed, so despite this table's full mutability it
    // is a pure element mutation, not a structural one.
    template<typename _RowCompare>
    void sort_rows(_RowCompare _cmp)
    {
        // a table with no rows or a single row is trivially in order
        if (m_rows < 2)
        {
            return;
        }

        const table& cself = *this;

        // build and stably sort a row-index permutation by the comparator
        std::vector<size_type> perm(static_cast<std::size_t>(m_rows));
        for (size_type i = 0; i < m_rows; ++i)
        {
            perm[i] = i;
        }

        std::stable_sort(perm.begin(), perm.end(),
            [&cself, &_cmp](size_type _a, size_type _b)
            {
                return _cmp(cself.row(_a), cself.row(_b));
            });

        // materialize the reordered grid, then adopt it (I_T unchanged)
        storage_type reordered;
        reordered.reserve(m_cells.size());

        for (size_type i = 0; i < m_rows; ++i)
        {
            const std::size_t src =
                static_cast<std::size_t>(perm[i]) * static_cast<std::size_t>(m_cols);

            for (std::size_t c = 0; c < static_cast<std::size_t>(m_cols); ++c)
            {
                reordered.push_back(m_cells[src + c]);
            }
        }

        m_cells.swap(reordered);

        return;
    }

    // --- transformation (twin of filterability below) ---

    // map
    //   the functorial mapping mu_f: applies _fn to every cell and returns a
    // same-shape table over the image type sigma = f(tau).  Size and arrangement
    // are preserved exactly; a non-monotone or non-injective _fn may break
    // sortedness or uniqueness.  When sigma differs from tau the result is a
    // fresh table<sigma> (a transform source builds the retyped image elsewhere).
    template<typename _Fn>
    D_NODISCARD
    table<clean_t<decltype(std::declval<_Fn&>()(std::declval<const _Type&>()))>>
    map(_Fn _fn) const
    {
        using mapped_cell =
            clean_t<decltype(std::declval<_Fn&>()(std::declval<const _Type&>()))>;

        std::vector<mapped_cell> out;
        out.reserve(m_cells.size());

        // rewrite each cell by its image, row-major
        for (const _Type& cell : m_cells)
        {
            out.push_back(_fn(cell));
        }

        return table<mapped_cell>::from_flat(
            static_cast<std::vector<mapped_cell>&&>(out), m_rows, m_cols);
    }

    // map_inplace
    //   the element-preserving mapping (sigma = tau) applied in place: each cell
    // is overwritten by its image, leaving I_T fixed.  Size and shape preserved.
    template<typename _Fn>
    void map_inplace(_Fn _fn)
    {
        for (_Type& cell : m_cells)
        {
            cell = static_cast<_Type>(_fn(cell));
        }

        return;
    }

    // --- filterability (row selection: closed on the table, WHERE) ---

    // filter_rows
    //   the selection sigma_phi over rows: returns a fresh table of the rows
    // whose row view satisfies _pred, each keeping its cells and relative order.
    // Row selection preserves rectangularity (a sub-table is a table), so it is
    // closed; |result| <= |this|, and the constant-false predicate yields the
    // empty (0-row) table -- the one bound selection may break.
    template<typename _Pred>
    D_NODISCARD table filter_rows(_Pred _pred) const
    {
        table out;

        // gather the surviving rows into a fresh table (width adopted on first)
        for (size_type r = 0; r < m_rows; ++r)
        {
            const_row_type row_r = this->row(r);

            if (_pred(row_r))
            {
                out.push_row(row_r.begin(), row_r.end());
            }
        }

        return out;
    }

    // filter_rows_inplace
    //   the same selection performed in place: rows failing _pred are removed and
    // the buffer rebuilt from the survivors.  This is the closed, build-capable
    // selection that makes the dynamic table filterable at the row level; the
    // width is retained even when no row survives.
    template<typename _Pred>
    void filter_rows_inplace(_Pred _pred)
    {
        const table& cself = *this;

        storage_type kept;
        size_type    kept_rows = 0;

        // keep only rows satisfying the predicate
        for (size_type r = 0; r < m_rows; ++r)
        {
            const_row_type row_r = cself.row(r);

            if (_pred(row_r))
            {
                const std::size_t base =
                    static_cast<std::size_t>(r) * static_cast<std::size_t>(m_cols);

                for (std::size_t c = 0; c < static_cast<std::size_t>(m_cols); ++c)
                {
                    kept.push_back(m_cells[base + c]);
                }

                ++kept_rows;
            }
        }

        m_cells.swap(kept);
        m_rows = kept_rows;   // m_cols (width) retained

        return;
    }

    // --- serialization (Serialization: shape + cells, under the serial options) ---

    // encode_into_e / decode_e
    //   the parameterised member enc_tau / dec_tau: the SHAPE (rows then cols,
    // each a length field per <_L,_E>) and the cells (each a leaf under
    // enc_tau<_E>).  This is where the container-serial options reach the table's
    // own bytes; the reservation on decode is capped by the reader's remaining
    // bytes, so a corrupt oversized count cannot force a runaway allocation.
    template<serial_endian _E,
             serial_length  _L,
             typename       _Sink>
    void encode_into_e(_Sink& _sink) const
    {
        internal::put_length<_L, _E>(_sink, static_cast<std::uint64_t>(m_rows));
        internal::put_length<_L, _E>(_sink, static_cast<std::uint64_t>(m_cols));

        for (const_pointer p = data(); p != (data() + this->size()); ++p)
        {
            encode_leaf_into<_E>(_sink, *p);
        }

        return;
    }

    template<serial_endian _E,
             serial_length  _L>
    static decode_result<table> decode_e(byte_reader& _reader)
    {
        std::uint64_t _rr = 0;
        std::uint64_t _cc = 0;

        if (!internal::get_length<_L, _E>(_reader, _rr))
        {
            return decode_failure<table>();
        }
        if (!internal::get_length<_L, _E>(_reader, _cc))
        {
            return decode_failure<table>();
        }

        const std::size_t _rows  = static_cast<std::size_t>(_rr);
        const std::size_t _cols  = static_cast<std::size_t>(_cc);
        const std::size_t _count = _rows * _cols;

        storage_type _cells;

        // never trust a corrupt count beyond the bytes left (>= 1 per cell)
        _cells.reserve(
            (_count < _reader.remaining()) ? _count : _reader.remaining());

        for (std::size_t i = 0; i < _count; ++i)
        {
            decode_result<_Type> _cell = decode_leaf<_E, _Type>(_reader);
            if (!_cell.ok) { return decode_failure<table>(); }

            _cells.push_back(static_cast<_Type&&>(_cell.value));
        }

        return decode_success(
            table::from_flat(static_cast<storage_type&&>(_cells),
                             static_cast<size_type>(_rows),
                             static_cast<size_type>(_cols)));
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

    static decode_result<table> decode(byte_reader& _reader)
    {
        return decode_e<serial_endian::big, serial_length::u64>(_reader);
    }

private:
    // adopt_or_check_width
    //   on a width-less (0-column) table, adopts _width as the column count;
    // otherwise requires _width to match, holding the rectangular invariant.
    void adopt_or_check_width(std::size_t _width)
    {
        // a table that has never had a width takes this row's width
        if ( (m_cols == 0) &&
             (m_rows == 0) )
        {
            m_cols = static_cast<size_type>(_width);

            return;
        }

        // otherwise the row must match the established width
        if (_width != static_cast<std::size_t>(m_cols))
        {
            throw std::invalid_argument(
                "table: row width must equal the table width (rectangular).");
        }

        return;
    }

    storage_type m_cells;   // row-major cell buffer; size() == m_rows * m_cols
    size_type    m_rows;    // extent along coordinate 1 (m_1 + 1)
    size_type    m_cols;    // extent along coordinate 2 (m_2 + 1)
};


// ---------------------------------------------------------------------------
// axis conformance -- the framework's classifiers must agree with the declared
// positions (representative instantiation).
// ---------------------------------------------------------------------------
namespace table_axis_conformance
{
    using table_probe = table<int>;

    // Iterability: iterable, and non-const (fully mutable -> settable cursor).
    static_assert(is_iterable_container_v<table_probe>,
                  "table must classify as iterable.");
    static_assert(iteration_mode_of<table_probe>::value
                      == iteration_mode::non_const,
                  "table iteration must be non-const (fully mutable).");

    // Boundedness: no fixed cap (capacity() is spare room, reserve() grows) ->
    // unbounded.
    static_assert(is_unbounded_container_v<table_probe>,
                  "table must classify as unbounded.");
    static_assert(!is_bounded_container_v<table_probe>,
                  "table must not classify as bounded.");

    // Multiplicity: comparator-less, position-keyed -> sequence (m = infinity).
    static_assert(multiplicity_kind_of<table_probe>::value
                      == multiplicity_kind::sequence,
                  "table must classify as a sequence (m = infinity).");

    // Sortedness / Ordering: ordered, and order-dependent (not sorted in itself).
    static_assert(is_ordered_container_v<table_probe>,
                  "table must classify as ordered.");
    static_assert(sortedness_of<table_probe>::value
                      == sortedness::order_dependent,
                  "table must classify as order-dependent (unsorted).");

    // Structure: uniformly nested (rank-2) -> hierarchical.
    static_assert(is_hierarchical_container_v<table_probe>,
                  "table must classify as hierarchical (uniformly nested).");
    static_assert(!is_flat_container_v<table_probe>,
                  "table must not classify as flat.");
    static_assert(structure_kind_of<table_probe>::value
                      == structure_kind::hierarchical,
                  "table structure_kind must be hierarchical.");

    // Filterability and Transformability (composite, detection-only): at the
    // CELL level a filter/transform SOURCE -- it grows by rows, not cells, so it
    // has no cell-level build (push_back) and a re-typed image or cell-selection
    // is a fresh container.  Its filterability/transformability is at the ROW
    // level (filter_rows_inplace / map_inplace), which the element-level trait
    // does not measure.  Verified out-of-band (see the note in static_table.hpp).
}


// ===========================================================================
// III. equality / swap
// ===========================================================================

// operator== / operator!=
//   compares two tables cell-by-cell after a shape check (positional identity).
template<typename    _Type,
         typename    _D,
         typename    _S,
         typename    _I,
         typename    _CI,
         typename... _O>
D_NODISCARD bool operator==(
    const table<_Type, _D, _S, _I, _CI, _O...>& _a,
    const table<_Type, _D, _S, _I, _CI, _O...>& _b)
{
    return _a.content_equals(_b);
}

template<typename    _Type,
         typename    _D,
         typename    _S,
         typename    _I,
         typename    _CI,
         typename... _O>
D_NODISCARD bool operator!=(
    const table<_Type, _D, _S, _I, _CI, _O...>& _a,
    const table<_Type, _D, _S, _I, _CI, _O...>& _b)
{
    return !(_a == _b);
}

// swap
//   free swap for two tables.
template<typename    _Type,
         typename    _D,
         typename    _S,
         typename    _I,
         typename    _CI,
         typename... _O>
void swap(
    table<_Type, _D, _S, _I, _CI, _O...>& _a,
    table<_Type, _D, _S, _I, _CI, _O...>& _b) noexcept
{
    _a.swap(_b);

    return;
}


// select_rows
//   the external selection strategy: builds a fresh table<element_type> from the
// rows of any table-like source whose row view satisfies _pred.  This is how a
// filter SOURCE (static_table, fixed_table -- fixed-shape, so unable to receive
// an arbitrary sub-selection in place) feeds a selection: the survivors are
// gathered into the filterable table type.  Arrangement and per-row cells are
// preserved; the row order carries over unchanged.
template<typename _Src,
         typename _Pred>
D_NODISCARD table<typename _Src::element_type>
select_rows(
    const _Src& _src,
    _Pred       _pred)
{
    table<typename _Src::element_type> out;

    using src_size = typename _Src::size_type;

    // gather rows of the source that satisfy the predicate
    for (src_size r = 0; r < _src.row_count(); ++r)
    {
        auto row_r = _src.row(r);

        if (_pred(row_r))
        {
            out.push_row(row_r.begin(), row_r.end());
        }
    }

    return out;
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TABLE_
