/******************************************************************************
* djinterp [container]                                          table_base.hpp
*
*   Shared support for the rank-2 table containers of Part II (static_table,
* fixed_table, table).  A table is a container whose positions are addressed
* by multi-indices of fixed rank -- a tuple presented as a container -- with
*   Pos(T) = I_T subset N^k,   val_T : Pos(T) -> cell type,   |T| = |I_T|.
* This header fixes the leading case: k = 2 (the row-and-column table), a
* rectangular box domain I_T = {0..R-1} x {0..C-1}, and one cell type for
* every cell (cell-homogeneous).  Cells are laid out row-major and contiguous,
* so the atomic order is the lexicographic order on multi-indices the axis of
* Sortedness assigns a table.
*
*   THREE PIECES:
*   1. basic_row_view / basic_column_view -- the subtable and projection of the
*      formal definition.  A row is the rank-1 subtable T[r]; a column is the
*      projection T[*,c] (strided).  Each is a non-owning window, const when its
*      cell parameter is const-qualified.
*   2. row_cursor / column_cursor -- proxy iterators that sweep rows and columns;
*      dereferencing yields a view by value (a cursor, not a handle onto a
*      stored reference).
*   3. table_base<_Derived, ...> -- a CRTP mixin supplying every read-only
*      operation a rank-2 table has, written once against a contiguous cell
*      buffer.  The derived container supplies exactly three hooks -- data(),
*      rows(), cols() -- and inherits rank/shape/size, checked and unchecked
*      cell access, row (subtable) and column (projection) access, and the cell
*      and row const-iteration surfaces.  The base never owns or allocates.
*
*   AXES CARRIED HERE (shared by all three tables):
*   Beyond the read-only cell/subtable/projection surface, this base supplies
* the shared face of several Part I axes: Structure (element_type, node_type,
* the hierarchical structure_category tag, and depth = rank -- a rank-k table is
* the uniformly-nested F_1[..F_k[tau]..]), Iterability (the cell, row, and
* column cursors), Multiplicity (count, the per-class occurrence count #_E under
* identity equivalence), and Sortedness (is_sorted / is_row_sorted, the
* checkable "sits in comparator order" predicates).  Boundedness and the
* structural side of Sortedness (sort_rows) differ per concrete table and live
* there.  The three axes that separate the concrete tables -- Lifetime, Storage,
* Mutability (Part I, in order) -- also live in the derived classes; this base
* is the material they share.
*
*   PORTABILITY:
*   C++14 baseline (relaxed constexpr for the checked accessors).
*
*
* path:      /inc/djinterp/core/container/table/table_base.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.04
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Internal Views and Cursors
      1. basic_row_view        (rank-1 subtable T[r])
      2. column_cursor         (strided iterator over a column)
      3. basic_column_view     (projection T[*,c])
      4. row_cursor            (iterator over rows)
II.   table_base (CRTP read-only rank-2 surface)
*/

#ifndef DJINTERP_CONTAINER_TABLE_BASE_
#define DJINTERP_CONTAINER_TABLE_BASE_ 1

// std
#include <cstddef>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"   // NS_*, D_CONSTEXPR, D_NODISCARD, clean_t
// djinterp - axis detection traits (the classifiers the tables answer to)
#include "../traits/iterable_container_traits.hpp"       // Iterability
#include "../traits/iterator_category_traits.hpp"        // iterator-category (needed below)
#include "../traits/bounded_container_traits.hpp"        // Boundedness
#include "../traits/container_multiplicity_traits.hpp"   // Multiplicity
#include "../traits/ordered_container_traits.hpp"        // Ordering  (needs iterator-category)
#include "../traits/sorted_container_traits.hpp"         // Sortedness (includes ordered)
#include "../traits/flat_container_traits.hpp"           // Structure  (flat/hierarchical + tags)
// NOTE: the composite axes Filterability (container_filter_traits.hpp) and
// Transformability (container_transform_traits.hpp) are DETECTION-ONLY and are
// deliberately NOT pulled in here.  They are derived (read + build) and, in the
// current framework, (a) both define internal::has_value_type_helper unguarded,
// so the two cannot share a translation unit, and (b) the filter cluster's
// constexpr chain requires C++17.  The umbrella container_traits.hpp likewise
// includes neither.  The tables' filter/transform CLASSIFICATION is documented
// per axis below and verified out-of-band against those classifiers; the
// OPERATIONS (map / map_inplace / filter_rows / select_rows) are unaffected.


NS_DJINTERP


// ===========================================================================
// I.   Internal Views and Cursors
// ===========================================================================

NS_INTERNAL

    // basic_row_view
    //   class: a non-owning window onto one row of a row-major table -- the
    // rank-1 subtable T[r].  _Cell is `const _Type` for a read-only row and
    // `_Type` for a writable one; every member follows that qualification.
    template<typename _Cell>
    class basic_row_view
    {
    public:
        using value_type      = typename std::remove_const<_Cell>::type;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer         = _Cell*;
        using reference       = _Cell&;
        using iterator        = _Cell*;
        using const_iterator  = const value_type*;

        // default: an empty row (null window).
        D_CONSTEXPR basic_row_view() D_NOEXCEPT
            : m_data(nullptr),
              m_size(0)
        {}

        // window: a row of _size cells beginning at _data.
        D_CONSTEXPR basic_row_view(
            pointer   _data,
            size_type _size
        ) D_NOEXCEPT
            : m_data(_data),
              m_size(_size)
        {}

        D_NODISCARD D_CONSTEXPR size_type size() const D_NOEXCEPT
        {
            return m_size;
        }

        D_NODISCARD D_CONSTEXPR bool empty() const D_NOEXCEPT
        {
            return (m_size == 0);
        }

        D_NODISCARD D_CONSTEXPR pointer data() const D_NOEXCEPT
        {
            return m_data;
        }

        // unchecked cell access by column index within the row.
        D_NODISCARD D_CONSTEXPR reference operator[](size_type _c) const
        {
            return m_data[_c];
        }

        // checked cell access; throws std::out_of_range past the row width.
        D_NODISCARD D_CONSTEXPR reference at(size_type _c) const
        {
            // reject a column index outside this row
            if (_c >= m_size)
            {
                throw std::out_of_range("basic_row_view::at");
            }

            return m_data[_c];
        }

        D_NODISCARD D_CONSTEXPR reference front() const
        {
            return m_data[0];
        }

        D_NODISCARD D_CONSTEXPR reference back() const
        {
            return m_data[m_size - 1];
        }

        D_NODISCARD D_CONSTEXPR iterator begin() const D_NOEXCEPT
        {
            return m_data;
        }

        D_NODISCARD D_CONSTEXPR iterator end() const D_NOEXCEPT
        {
            return m_data + m_size;
        }

    private:
        pointer   m_data;
        size_type m_size;
    };


    // column_cursor
    //   class: a strided random-access iterator stepping down one column of a
    // row-major table; the stride is the table's column count.  Backs the
    // column projection T[*,c].
    template<typename _Cell>
    class column_cursor
    {
    public:
        using value_type        = typename std::remove_const<_Cell>::type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = _Cell*;
        using reference         = _Cell&;
        using iterator_category = std::random_access_iterator_tag;

        D_CONSTEXPR column_cursor() D_NOEXCEPT
            : m_ptr(nullptr),
              m_stride(1)
        {}

        D_CONSTEXPR column_cursor(
            pointer         _ptr,
            difference_type _stride
        ) D_NOEXCEPT
            : m_ptr(_ptr),
              m_stride(_stride)
        {}

        D_NODISCARD D_CONSTEXPR reference operator*() const D_NOEXCEPT
        {
            return *m_ptr;
        }

        D_NODISCARD D_CONSTEXPR pointer operator->() const D_NOEXCEPT
        {
            return m_ptr;
        }

        D_NODISCARD D_CONSTEXPR reference operator[](difference_type _n) const D_NOEXCEPT
        {
            return *(m_ptr + (_n * m_stride));
        }

        D_CONSTEXPR column_cursor& operator++() D_NOEXCEPT
        {
            m_ptr += m_stride;

            return *this;
        }

        D_CONSTEXPR column_cursor operator++(int) D_NOEXCEPT
        {
            column_cursor tmp = *this;
            m_ptr += m_stride;

            return tmp;
        }

        D_CONSTEXPR column_cursor& operator--() D_NOEXCEPT
        {
            m_ptr -= m_stride;

            return *this;
        }

        D_CONSTEXPR column_cursor operator--(int) D_NOEXCEPT
        {
            column_cursor tmp = *this;
            m_ptr -= m_stride;

            return tmp;
        }

        D_CONSTEXPR column_cursor& operator+=(difference_type _n) D_NOEXCEPT
        {
            m_ptr += (_n * m_stride);

            return *this;
        }

        D_CONSTEXPR column_cursor& operator-=(difference_type _n) D_NOEXCEPT
        {
            m_ptr -= (_n * m_stride);

            return *this;
        }

        D_NODISCARD friend D_CONSTEXPR column_cursor operator+(
            column_cursor   _it,
            difference_type _n
        ) D_NOEXCEPT
        {
            _it += _n;

            return _it;
        }

        D_NODISCARD friend D_CONSTEXPR column_cursor operator-(
            column_cursor   _it,
            difference_type _n
        ) D_NOEXCEPT
        {
            _it -= _n;

            return _it;
        }

        D_NODISCARD friend D_CONSTEXPR difference_type operator-(
            const column_cursor& _a,
            const column_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_ptr - _b.m_ptr) / _a.m_stride;
        }

        D_NODISCARD friend D_CONSTEXPR bool operator==(
            const column_cursor& _a,
            const column_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_ptr == _b.m_ptr);
        }

        D_NODISCARD friend D_CONSTEXPR bool operator!=(
            const column_cursor& _a,
            const column_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_ptr != _b.m_ptr);
        }

        D_NODISCARD friend D_CONSTEXPR bool operator<(
            const column_cursor& _a,
            const column_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_ptr < _b.m_ptr);
        }

        D_NODISCARD friend D_CONSTEXPR bool operator>(
            const column_cursor& _a,
            const column_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_ptr > _b.m_ptr);
        }

        D_NODISCARD friend D_CONSTEXPR bool operator<=(
            const column_cursor& _a,
            const column_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_ptr <= _b.m_ptr);
        }

        D_NODISCARD friend D_CONSTEXPR bool operator>=(
            const column_cursor& _a,
            const column_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_ptr >= _b.m_ptr);
        }

    private:
        pointer         m_ptr;
        difference_type m_stride;
    };


    // basic_column_view
    //   class: a non-owning strided window onto one column -- the projection
    // T[*,c] = (v_{r,c})_r.  Read-only when _Cell is const-qualified.
    template<typename _Cell>
    class basic_column_view
    {
    public:
        using value_type      = typename std::remove_const<_Cell>::type;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer         = _Cell*;
        using reference       = _Cell&;
        using iterator        = column_cursor<_Cell>;

        D_CONSTEXPR basic_column_view() D_NOEXCEPT
            : m_data(nullptr),
              m_count(0),
              m_stride(1)
        {}

        // window: _count cells beginning at _data, one every _stride cells.
        D_CONSTEXPR basic_column_view(
            pointer         _data,
            size_type       _count,
            difference_type _stride
        ) D_NOEXCEPT
            : m_data(_data),
              m_count(_count),
              m_stride(_stride)
        {}

        D_NODISCARD D_CONSTEXPR size_type size() const D_NOEXCEPT
        {
            return m_count;
        }

        D_NODISCARD D_CONSTEXPR bool empty() const D_NOEXCEPT
        {
            return (m_count == 0);
        }

        // unchecked cell access by row index within the column.
        D_NODISCARD D_CONSTEXPR reference operator[](size_type _r) const
        {
            return *(m_data + (static_cast<difference_type>(_r) * m_stride));
        }

        // checked cell access; throws std::out_of_range past the column height.
        D_NODISCARD D_CONSTEXPR reference at(size_type _r) const
        {
            // reject a row index outside this column
            if (_r >= m_count)
            {
                throw std::out_of_range("basic_column_view::at");
            }

            return (*this)[_r];
        }

        D_NODISCARD D_CONSTEXPR iterator begin() const D_NOEXCEPT
        {
            return iterator(m_data, m_stride);
        }

        D_NODISCARD D_CONSTEXPR iterator end() const D_NOEXCEPT
        {
            return iterator(m_data + (static_cast<difference_type>(m_count) * m_stride),
                            m_stride);
        }

    private:
        pointer         m_data;
        size_type       m_count;
        difference_type m_stride;
    };


    // row_arrow
    //   type: arrow-operator proxy for row_cursor, holding the dereferenced
    // row view so `->` can return a pointer to it.
    template<typename _RowView>
    struct row_arrow
    {
        _RowView row;

        D_CONSTEXPR const _RowView* operator->() const D_NOEXCEPT
        {
            return &row;
        }
    };

    // row_cursor
    //   class: a random-access iterator over the rows of a row-major table.
    // Dereferencing yields a basic_row_view by value (a proxy iterator: its
    // reference is a view, not a handle onto a stored row object).
    template<typename _Cell>
    class row_cursor
    {
    public:
        using value_type        = basic_row_view<_Cell>;
        using difference_type   = std::ptrdiff_t;
        using reference         = basic_row_view<_Cell>;
        using pointer           = row_arrow<basic_row_view<_Cell>>;
        using iterator_category = std::random_access_iterator_tag;

        D_CONSTEXPR row_cursor() D_NOEXCEPT
            : m_base(nullptr),
              m_row(0),
              m_cols(0)
        {}

        D_CONSTEXPR row_cursor(
            _Cell*          _base,
            difference_type _row,
            std::size_t     _cols
        ) D_NOEXCEPT
            : m_base(_base),
              m_row(_row),
              m_cols(_cols)
        {}

        D_NODISCARD D_CONSTEXPR reference operator*() const D_NOEXCEPT
        {
            return reference(m_base + (m_row * static_cast<difference_type>(m_cols)),
                             m_cols);
        }

        D_NODISCARD D_CONSTEXPR pointer operator->() const D_NOEXCEPT
        {
            return pointer{ **this };
        }

        D_NODISCARD D_CONSTEXPR reference operator[](difference_type _n) const D_NOEXCEPT
        {
            return reference(m_base + ((m_row + _n) * static_cast<difference_type>(m_cols)),
                             m_cols);
        }

        D_CONSTEXPR row_cursor& operator++() D_NOEXCEPT
        {
            ++m_row;

            return *this;
        }

        D_CONSTEXPR row_cursor operator++(int) D_NOEXCEPT
        {
            row_cursor tmp = *this;
            ++m_row;

            return tmp;
        }

        D_CONSTEXPR row_cursor& operator--() D_NOEXCEPT
        {
            --m_row;

            return *this;
        }

        D_CONSTEXPR row_cursor operator--(int) D_NOEXCEPT
        {
            row_cursor tmp = *this;
            --m_row;

            return tmp;
        }

        D_CONSTEXPR row_cursor& operator+=(difference_type _n) D_NOEXCEPT
        {
            m_row += _n;

            return *this;
        }

        D_CONSTEXPR row_cursor& operator-=(difference_type _n) D_NOEXCEPT
        {
            m_row -= _n;

            return *this;
        }

        D_NODISCARD friend D_CONSTEXPR row_cursor operator+(
            row_cursor      _it,
            difference_type _n
        ) D_NOEXCEPT
        {
            _it += _n;

            return _it;
        }

        D_NODISCARD friend D_CONSTEXPR row_cursor operator-(
            row_cursor      _it,
            difference_type _n
        ) D_NOEXCEPT
        {
            _it -= _n;

            return _it;
        }

        D_NODISCARD friend D_CONSTEXPR difference_type operator-(
            const row_cursor& _a,
            const row_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_row - _b.m_row);
        }

        D_NODISCARD friend D_CONSTEXPR bool operator==(
            const row_cursor& _a,
            const row_cursor& _b
        ) D_NOEXCEPT
        {
            return ( (_a.m_base == _b.m_base) &&
                     (_a.m_row  == _b.m_row) );
        }

        D_NODISCARD friend D_CONSTEXPR bool operator!=(
            const row_cursor& _a,
            const row_cursor& _b
        ) D_NOEXCEPT
        {
            return !(_a == _b);
        }

        D_NODISCARD friend D_CONSTEXPR bool operator<(
            const row_cursor& _a,
            const row_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_row < _b.m_row);
        }

        D_NODISCARD friend D_CONSTEXPR bool operator>(
            const row_cursor& _a,
            const row_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_row > _b.m_row);
        }

        D_NODISCARD friend D_CONSTEXPR bool operator<=(
            const row_cursor& _a,
            const row_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_row <= _b.m_row);
        }

        D_NODISCARD friend D_CONSTEXPR bool operator>=(
            const row_cursor& _a,
            const row_cursor& _b
        ) D_NOEXCEPT
        {
            return (_a.m_row >= _b.m_row);
        }

    private:
        _Cell*          m_base;
        difference_type m_row;
        std::size_t     m_cols;
    };

NS_END  // internal


// ===========================================================================
// II.  table_base (CRTP read-only rank-2 surface)
// ===========================================================================

// table_base
//   class: CRTP mixin supplying the read-only surface every rank-2
// cell-homogeneous table shares -- rank and shape, checked and unchecked cell
// access, row (subtable) and column (projection) access, and the cell and row
// const-iteration surfaces.  It reads the derived container through three
// hooks it must expose publicly:
//     const_pointer data() const;   // contiguous, row-major cell buffer
//     size_type     rows() const;   // m_1 + 1
//     size_type     cols() const;   // m_2 + 1
// The base stores nothing and allocates nothing.
template<typename _Derived,
         typename _Type,
         typename _SizeType,
         typename _DifferenceType>
class table_base
{
public:
    // --- member types (the container value function, in STL vocabulary) ---

    using value_type       = _Type;
    using cell_type        = _Type;   // formal name for the element at an index
    using size_type        = _SizeType;
    using difference_type  = _DifferenceType;
    using reference        = _Type&;
    using const_reference  = const _Type&;
    using pointer          = _Type*;
    using const_pointer    = const _Type*;

    // index_type
    //   struct: a multi-index into a rank-2 table -- the coordinate pair
    // (row, column) that addresses one cell.
    struct index_type
    {
        size_type row;
        size_type column;
    };

    // contiguous, row-major cell iteration.
    using const_iterator         = const _Type*;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // the rank-1 subtable T[r] and the projection T[*,c], read-only.
    using const_row_type         = internal::basic_row_view<const _Type>;
    using const_column_type      = internal::basic_column_view<const _Type>;
    using const_row_iterator     = internal::row_cursor<const _Type>;

    // rank
    //   value: the table's fixed dimension k.  This is the row-and-column
    // table, so k = 2.
    static constexpr size_type rank = static_cast<size_type>(2);

    // --- structure (uniformly nested: a rank-k table is F_1[..F_k[tau]..]) ---

    // element_type
    //   the leaf/base type tau carried at each cell -- an atomic type, not a
    // container.  The cell-homogeneous table carries this single element type.
    using element_type = _Type;

    // node_type
    //   the F[T] node summand -- a component that is itself a sub-container.  For
    // this rank-2 table the node at the outer level is a row (F_2[tau]); a row
    // view is container-shaped (value_type + size()), so it is the formal node.
    using node_type = const_row_type;

    // structure_category
    //   the opt-in structural tag asserting hierarchy.  A table stores its cells
    // in a flat row-major buffer, so the value_type chain does not reveal the
    // nesting; this tag asserts the type-level uniform nesting the chain hides,
    // which the framework's structure classifier then reads as authoritative.
    using structure_category = hierarchical;

    // depth
    //   the uniform-nesting depth d = rank: leaves sit at level d, each
    // coordinate is one level, and d is fixed by the type -- a static quantity,
    // not a per-value height.  (The value_type-chain heuristic under-counts this,
    // seeing only the flat cell buffer; the true type-level depth is the rank.)
    static constexpr size_type depth = rank;

protected:
    // only derived tables construct or destroy the mixin.
    table_base()  = default;
    ~table_base() = default;

private:
    D_CONSTEXPR const _Derived& self() const D_NOEXCEPT
    {
        return static_cast<const _Derived&>(*this);
    }

public:
    // --- shape and size ---

    // rank_of
    //   returns the table's dimension k (always 2 here); the runtime companion
    // to the static `rank` constant.
    D_NODISCARD D_CONSTEXPR size_type rank_of() const D_NOEXCEPT
    {
        return rank;
    }

    // row_count / column_count -- the two extents (m_1 + 1, m_2 + 1).
    D_NODISCARD D_CONSTEXPR size_type row_count() const D_NOEXCEPT
    {
        return self().rows();
    }

    D_NODISCARD D_CONSTEXPR size_type column_count() const D_NOEXCEPT
    {
        return self().cols();
    }

    // size
    //   |T| = |I_T| = rows * columns, the number of atomic cells.
    D_NODISCARD D_CONSTEXPR size_type size() const D_NOEXCEPT
    {
        return (self().rows() * self().cols());
    }

    D_NODISCARD D_CONSTEXPR bool empty() const D_NOEXCEPT
    {
        return (size() == 0);
    }

    // contains
    //   true when (_r, _c) is a valid index of I_T.  For a rectangular table
    // this is exactly the conjunction of the two range checks.
    D_NODISCARD D_CONSTEXPR bool contains(
        size_type _r,
        size_type _c
    ) const D_NOEXCEPT
    {
        return ( (_r < self().rows()) &&
                 (_c < self().cols()) );
    }

    // --- cell access (atomic access to the underlying tuple) ---

    // operator() -- unchecked cell value at (_r, _c), row-major.
    D_NODISCARD D_CONSTEXPR const_reference operator()(
        size_type _r,
        size_type _c
    ) const
    {
        return self().data()[(_r * self().cols()) + _c];
    }

    // at -- checked cell value; throws std::out_of_range off the domain, since
    // an out-of-domain index is undefined, not blank.
    D_NODISCARD D_CONSTEXPR const_reference at(
        size_type _r,
        size_type _c
    ) const
    {
        // reject an index outside the rectangular domain I_T
        if (!contains(_r, _c))
        {
            throw std::out_of_range("table_base::at");
        }

        return self().data()[(_r * self().cols()) + _c];
    }

    // --- subtable and projection ---

    // row / operator[] -- the rank-1 subtable T[r], a read-only window onto one
    // row.  operator[] spells the prefix-bracketing T[r] of the definition.
    D_NODISCARD D_CONSTEXPR const_row_type row(size_type _r) const
    {
        return const_row_type(self().data() + (_r * self().cols()),
                              self().cols());
    }

    D_NODISCARD D_CONSTEXPR const_row_type operator[](size_type _r) const
    {
        return row(_r);
    }

    // checked subtable access; throws std::out_of_range past the last row.
    D_NODISCARD D_CONSTEXPR const_row_type row_at(size_type _r) const
    {
        // reject a row index outside the table
        if (_r >= self().rows())
        {
            throw std::out_of_range("table_base::row_at");
        }

        return row(_r);
    }

    // column -- the projection T[*,c], a read-only strided window onto one
    // column.
    D_NODISCARD D_CONSTEXPR const_column_type column(size_type _c) const
    {
        return const_column_type(
            self().data() + _c,
            self().rows(),
            static_cast<difference_type>(self().cols()));
    }

    D_NODISCARD D_CONSTEXPR const_column_type column_at(size_type _c) const
    {
        // reject a column index outside the table
        if (_c >= self().cols())
        {
            throw std::out_of_range("table_base::column_at");
        }

        return column(_c);
    }

    // --- cell const-iteration (row-major = lexicographic on multi-indices) ---

    D_NODISCARD D_CONSTEXPR const_iterator begin() const D_NOEXCEPT
    {
        return self().data();
    }

    D_NODISCARD D_CONSTEXPR const_iterator end() const D_NOEXCEPT
    {
        return self().data() + size();
    }

    D_NODISCARD D_CONSTEXPR const_iterator cbegin() const D_NOEXCEPT
    {
        return begin();
    }

    D_NODISCARD D_CONSTEXPR const_iterator cend() const D_NOEXCEPT
    {
        return end();
    }

    D_NODISCARD const_reverse_iterator rbegin() const D_NOEXCEPT
    {
        return const_reverse_iterator(end());
    }

    D_NODISCARD const_reverse_iterator rend() const D_NOEXCEPT
    {
        return const_reverse_iterator(begin());
    }

    D_NODISCARD const_reverse_iterator crbegin() const D_NOEXCEPT
    {
        return rbegin();
    }

    D_NODISCARD const_reverse_iterator crend() const D_NOEXCEPT
    {
        return rend();
    }

    // --- row const-iteration (sweep the rank-1 subtables in order) ---

    D_NODISCARD D_CONSTEXPR const_row_iterator row_begin() const D_NOEXCEPT
    {
        return const_row_iterator(self().data(), 0, self().cols());
    }

    D_NODISCARD D_CONSTEXPR const_row_iterator row_end() const D_NOEXCEPT
    {
        return const_row_iterator(
            self().data(),
            static_cast<difference_type>(self().rows()),
            self().cols());
    }

    // --- multiplicity: per-class occurrence count (identity equivalence) ---

    // count
    //   the per-class occurrence count #_E(c, _value) under the identity
    // equivalence: how many cells hold a value equal to _value.  A table caps
    // this at nothing (m = infinity) -- equal values may recur at any indices,
    // since a cell is identified by its position, not its value.
    D_NODISCARD D_CONSTEXPR size_type count(const value_type& _value) const
    {
        const size_type n = size();
        size_type       k = 0;

        // tally cells equal to the probe value, in row-major order
        for (size_type i = 0; i < n; ++i)
        {
            if (self().data()[i] == _value)
            {
                ++k;
            }
        }

        return k;
    }

    // --- sortedness: a checkable property of the stored positions ---

    // is_sorted (comparator)
    //   true iff the cells, visited in row-major (lexicographic multi-index)
    // order, are non-decreasing under _cmp -- the adjacent-pair test that a
    // transitive comparator's sortedness reduces to.  A table is ordered but not
    // sorted in itself; this reports whether a given instance happens to sit in
    // comparator order (the checkable face of the sorted overlay).
    template<typename _Compare>
    D_NODISCARD D_CONSTEXPR bool is_sorted(_Compare _cmp) const
    {
        const size_type n = size();

        // every adjacent pair must be in order (cmp(next, cur) == false)
        for (size_type i = 1; i < n; ++i)
        {
            if (_cmp(self().data()[i], self().data()[i - 1]))
            {
                return false;
            }
        }

        return true;
    }

    // is_sorted ()
    //   the default-comparator form: non-decreasing under operator<.
    D_NODISCARD D_CONSTEXPR bool is_sorted() const
    {
        return is_sorted(std::less<value_type>());
    }

    // is_row_sorted
    //   true iff consecutive rows are non-decreasing under a row comparator
    // _cmp(const_row_type a, const_row_type b) -- the row-dimension reading of
    // sortedness (relational ORDER BY as a checkable property).
    template<typename _RowCompare>
    D_NODISCARD D_CONSTEXPR bool is_row_sorted(_RowCompare _cmp) const
    {
        const size_type r = self().rows();

        // every adjacent row pair must be in order
        for (size_type i = 1; i < r; ++i)
        {
            if (_cmp(row(i), row(i - 1)))
            {
                return false;
            }
        }

        return true;
    }

    // --- content equality ---

    // content_equals
    //   true when two tables have the same shape and equal cells at every
    // index.  A table's identity is positional: cell (_r,_c) of one is compared
    // to cell (_r,_c) of the other.
    template<typename _Other>
    D_NODISCARD D_CONSTEXPR bool content_equals(const _Other& _rhs) const
    {
        // differing shapes cannot hold equal contents
        if ( (row_count()    != _rhs.row_count()) ||
             (column_count() != _rhs.column_count()) )
        {
            return false;
        }

        const size_type n = size();

        // compare cells in row-major order
        for (size_type i = 0; i < n; ++i)
        {
            if (!(self().data()[i] == _rhs.data()[i]))
            {
                return false;
            }
        }

        return true;
    }
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TABLE_BASE_
