/******************************************************************************
* djinterp [container]                                        sorted_table.hpp
*
*   sorted_table -- the first OVERLAY on the table backing (Overlays: containers
* as restriction bundles).  An overlay is a bundle of restrictions a container is
* held to, riding on any backing and preserved by every exposed operation.  This
* one imposes, over a `table` of rows:
*
*     sorted  (sequence-level)  the rows are kept in non-decreasing order of a
*                               designated KEY COLUMN under a cell comparator --
*                               the restriction varsigma of the vocabulary.
*
*   and, optionally, a second restriction composing with it:
*
*     unique keys (static)      mu_1 on the key-equivalence E_key: each key
*                               value occurs at most once, so a repeated key is
*                               an assignment, not a new row.  With this the
*                               overlay is {varsigma, mu_1^{E_key}} -- the sorted
*                               MAP on rows; without it, {varsigma} -- the sorted
*                               sequence of rows, keys free to repeat.
*
*   PRESERVATION.  Every mutator maintains the bundle: insert places (or, when
* keys are unique, replaces) a row at its sorted position, and erase removes
* without disturbing the order of the rest.  Direct cell mutation is therefore
* NOT exposed -- overwriting a key cell could break varsigma -- so the surface is
* read-only cells plus order-preserving structural change, the discipline an
* overlay demands.
*
*   BACKING INDEPENDENCE.  The overlay names no backing beyond delegating to a
* `table`; its identity is the restriction set, not the store.  It reports the
* sorted invariant through the framework's opt-in `sorted_invariant` marker, so
* the Sortedness classifier reads it as `sorted` rather than the bare table's
* `order_dependent` -- the axis note "a sorted table is the overlay" made real.
*
*   KEY / VALUE.  This overlay keys on a COLUMN of a cell-homogeneous table
* (Key = the key cell's type = tau); the fully general keyed relational table,
* whose value type is a heterogeneous Key x Val record (_RowType), is the eta
* overlay layered on the tuple's dependent form, a planned sibling.
*
*   PORTABILITY:
*   C++17 (inherits the table backing and the options surface).
*
*
* path:      /inc/djinterp/core/container/table/sorted_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.05
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_sorted_table (detection trait)
II.   sorted_table (class)
      1. member types and overlay / axis markers
      2. construction
      3. read surface (delegated, const)
      4. key queries (binary search over the sorted order)
      5. structural mutation (invariant-preserving)
III.  make_sorted_table
*/

#ifndef DJINTERP_CONTAINER_SORTED_TABLE_
#define DJINTERP_CONTAINER_SORTED_TABLE_ 1

// std
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"                     // NS_*, D_CONSTEXPR, clean_t
#include "./table.hpp"                             // table backing (+ hierarchical tag)
#include "../container_options.hpp"                // axis enums, options base


NS_DJINTERP


// ===========================================================================
// I.   is_sorted_table (detection trait)
// ===========================================================================

// sorted_table (fwd)
template<typename    _Type,
         std::size_t _KeyCol,
         typename    _CellCompare,
         bool        _UniqueKeys,
         typename    _SizeType,
         typename    _DifferenceType,
         typename... _Options>
class sorted_table;

// is_sorted_table
//   trait: true when _Type (after stripping cv/ref) is a specialization of
// sorted_table.
NS_INTERNAL

    template<typename _Type>
    struct is_sorted_table_impl : std::false_type
    {};

    template<typename    _T,
             std::size_t _K,
             typename    _C,
             bool        _U,
             typename    _S,
             typename    _D,
             typename... _O>
    struct is_sorted_table_impl<sorted_table<_T, _K, _C, _U, _S, _D, _O...>>
        : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_sorted_table : internal::is_sorted_table_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _Type>
inline constexpr bool is_sorted_table_v = is_sorted_table<_Type>::value;
#endif


// ===========================================================================
// II.  sorted_table (class)
// ===========================================================================

// sorted_table
//   class: a table whose rows are held sorted by column _KeyCol under
// _CellCompare, optionally with unique keys.  Wraps a `table` backing and
// exposes a read-only cell surface plus order-preserving structural mutation.
template<typename    _Type,
         std::size_t _KeyCol         = 0,
         typename    _CellCompare    = std::less<_Type>,
         bool        _UniqueKeys     = false,
         typename    _SizeType       = std::size_t,
         typename    _DifferenceType = std::ptrdiff_t,
         typename... _Options>
class sorted_table
    : public options_container_base<_Options...>
{
private:
    using backing_type = table<_Type, _DifferenceType, _SizeType>;

public:
    // --- 1. member types and overlay / axis markers ---

    using value_type       = _Type;
    using cell_type        = _Type;
    using size_type        = _SizeType;
    using difference_type  = _DifferenceType;
    using reference        = const _Type&;   // cells are read-only under varsigma
    using const_reference  = const _Type&;
    using pointer          = const _Type*;
    using const_pointer    = const _Type*;
    using key_compare      = _CellCompare;

    using const_iterator     = typename backing_type::const_iterator;
    using const_row_type     = typename backing_type::const_row_type;
    using const_row_iterator = typename backing_type::const_row_iterator;

    // element / structure vocabulary (delegated from the backing).
    using element_type       = _Type;
    using structure_category = hierarchical;   // uniformly nested, like the backing

    // npos -- "no such row" sentinel for the key queries.
    static constexpr size_type npos = static_cast<size_type>(-1);

    // overlay markers (the restriction bundle this container wears).
    static constexpr bool      sorted_invariant = true;         // varsigma
    static constexpr bool      unique_keys      = _UniqueKeys;  // mu_1^{E_key}?
    static constexpr size_type key_column       = static_cast<size_type>(_KeyCol);

    // axis positions.
    static constexpr container_lifetime      lifetime      =
        container_lifetime::mutable_storage;
    static constexpr container_storage_kind  storage_kind  =
        container_storage_kind::dynamic_storage;
    static constexpr container_ordering      ordering      =
        container_ordering::sorted;             // the overlay makes it sorted
    static constexpr container_bounds        bounds        =
        container_bounds::unbounded;
    static constexpr container_iterability   iterability   =
        container_iterability::iterable;
    static constexpr container_multiplicity  multiplicity_grade =
        _UniqueKeys ? container_multiplicity::unique
                    : container_multiplicity::multi;
    static constexpr container_structure     structure     =
        container_structure::hierarchical;
    static constexpr size_type               rank  = static_cast<size_type>(2);
    static constexpr size_type               depth = static_cast<size_type>(2);

    // --- 2. construction ---

    sorted_table()
        : m_base(),
          m_cmp()
    {}

    explicit sorted_table(_CellCompare _cmp)
        : m_base(),
          m_cmp(_cmp)
    {}

    // nested rows: each is inserted at its sorted position, so any input order
    // yields the sorted invariant.
    sorted_table(std::initializer_list<std::initializer_list<_Type>> _rows)
        : m_base(),
          m_cmp()
    {
        // insert row by row; each insert restores varsigma
        for (const std::initializer_list<_Type>& r : _rows)
        {
            insert(r);
        }
    }

    sorted_table(const sorted_table&)            = default;
    sorted_table(sorted_table&&)                 = default;
    sorted_table& operator=(const sorted_table&) = default;
    sorted_table& operator=(sorted_table&&)      = default;
    ~sorted_table()                              = default;

    // --- 3. read surface (delegated, const) ---

    D_NODISCARD size_type rows() const noexcept
    {
        return m_base.rows();
    }

    D_NODISCARD size_type cols() const noexcept
    {
        return m_base.cols();
    }

    D_NODISCARD size_type row_count() const noexcept
    {
        return m_base.row_count();
    }

    D_NODISCARD size_type column_count() const noexcept
    {
        return m_base.column_count();
    }

    D_NODISCARD size_type size() const noexcept
    {
        return m_base.size();
    }

    D_NODISCARD bool empty() const noexcept
    {
        return m_base.empty();
    }

    // unchecked / checked cell reads, delegated to the backing.
    D_NODISCARD const_reference operator()(
        size_type _r,
        size_type _c
    ) const
    {
        return m_base(_r, _c);
    }

    D_NODISCARD const_reference at(
        size_type _r,
        size_type _c
    ) const
    {
        return m_base.at(_r, _c);
    }

    // the rank-1 subtable T[r] (read-only row view).
    D_NODISCARD const_row_type row(size_type _r) const
    {
        return m_base.row(_r);
    }

    D_NODISCARD const_row_type operator[](size_type _r) const
    {
        return m_base[_r];
    }

    // cell and row const-iteration, in sorted (row-major) order.
    D_NODISCARD const_iterator begin() const noexcept
    {
        return m_base.begin();
    }

    D_NODISCARD const_iterator end() const noexcept
    {
        return m_base.end();
    }

    D_NODISCARD const_iterator cbegin() const noexcept
    {
        return m_base.cbegin();
    }

    D_NODISCARD const_iterator cend() const noexcept
    {
        return m_base.cend();
    }

    D_NODISCARD const_row_iterator row_begin() const noexcept
    {
        return m_base.row_begin();
    }

    D_NODISCARD const_row_iterator row_end() const noexcept
    {
        return m_base.row_end();
    }

    // is_sorted -- true by construction; the overlay maintains varsigma.
    D_NODISCARD bool is_sorted() const noexcept
    {
        return true;
    }

    // base -- the underlying table backing (read-only), for interop.
    D_NODISCARD const backing_type& base() const noexcept
    {
        return m_base;
    }

    // --- 4. key queries (binary search over the sorted order) ---

    // key_at -- the key cell of row _r (its value in the key column).
    D_NODISCARD const_reference key_at(size_type _r) const
    {
        return m_base.row(_r)[key_column];
    }

    // lower_bound -- index of the first row whose key is not less than _key.
    D_NODISCARD size_type lower_bound(const _Type& _key) const
    {
        size_type lo = 0;
        size_type hi = m_base.rows();

        // binary search: narrow to the first non-less key
        while (lo < hi)
        {
            const size_type mid = lo + ((hi - lo) / 2);

            if (m_cmp(key_at(mid), _key))
            {
                lo = mid + 1;
            }
            else
            {
                hi = mid;
            }
        }

        return lo;
    }

    // upper_bound -- index of the first row whose key is greater than _key.
    D_NODISCARD size_type upper_bound(const _Type& _key) const
    {
        size_type lo = 0;
        size_type hi = m_base.rows();

        // binary search: narrow to the first greater key
        while (lo < hi)
        {
            const size_type mid = lo + ((hi - lo) / 2);

            if (m_cmp(_key, key_at(mid)))
            {
                hi = mid;
            }
            else
            {
                lo = mid + 1;
            }
        }

        return lo;
    }

    // find -- index of a row whose key is equivalent to _key, or npos.
    D_NODISCARD size_type find(const _Type& _key) const
    {
        const size_type pos = lower_bound(_key);

        // equivalent under the comparator: neither key is less than the other
        if ( (pos < m_base.rows()) &&
             (!m_cmp(_key, key_at(pos))) )
        {
            return pos;
        }

        return npos;
    }

    // contains_key -- whether any row carries a key equivalent to _key.
    D_NODISCARD bool contains_key(const _Type& _key) const
    {
        return (find(_key) != npos);
    }

    // count_key -- how many rows carry a key equivalent to _key (0 or 1 when
    // keys are unique).
    D_NODISCARD size_type count_key(const _Type& _key) const
    {
        return (upper_bound(_key) - lower_bound(_key));
    }

    // --- 5. structural mutation (invariant-preserving) ---

    // insert -- place _row at its sorted position.  When keys are unique and an
    // equivalent key is present, the existing row is replaced (map assignment).
    // Returns the index the row occupies.  varsigma (and, if set, mu_1^{E_key})
    // is preserved.
    size_type insert(std::initializer_list<_Type> _row)
    {
        // the key value governs placement; copy it before the buffer moves
        const _Type key = *(_row.begin() + static_cast<std::ptrdiff_t>(_KeyCol));

        const size_type pos = lower_bound(key);

        // unique-key overlay: an equivalent key already present is overwritten
        if ( _UniqueKeys &&
             (pos < m_base.rows()) &&
             (!m_cmp(key, key_at(pos))) )
        {
            m_base.erase_row(pos);
            m_base.insert_row(pos, _row);

            return pos;
        }

        m_base.insert_row(pos, _row);

        return pos;
    }

    // erase_at -- remove the row at index _r.
    void erase_at(size_type _r)
    {
        m_base.erase_row(_r);

        return;
    }

    // erase_key -- remove every row whose key is equivalent to _key; returns the
    // number removed (0 or 1 when keys are unique).
    size_type erase_key(const _Type& _key)
    {
        const size_type lo = lower_bound(_key);
        const size_type hi = upper_bound(_key);
        const size_type n  = (hi - lo);

        // the equivalent keys occupy the contiguous run [lo, hi)
        for (size_type i = 0; i < n; ++i)
        {
            m_base.erase_row(lo);
        }

        return n;
    }

    // clear -- drop all rows.
    void clear() noexcept
    {
        m_base.clear();

        return;
    }

private:
    backing_type m_base;   // the sorted row store (invariant: sorted by key_column)
    _CellCompare m_cmp;    // the cell comparator applied to the key column
};


// ===========================================================================
// III. make_sorted_table
// ===========================================================================

// make_sorted_table
//   function: build a sorted_table<_Type, _KeyCol> from nested rows, deducing
// the cell type from the first cell of the first row.
template<std::size_t _KeyCol = 0,
         typename    _Type>
D_NODISCARD sorted_table<_Type, _KeyCol>
make_sorted_table(std::initializer_list<std::initializer_list<_Type>> _rows)
{
    return sorted_table<_Type, _KeyCol>(_rows);
}


// ---------------------------------------------------------------------------
// axis / overlay conformance -- the framework's Sortedness classifier must read
// the overlay's invariant as `sorted` (representative instantiation).
// ---------------------------------------------------------------------------
namespace table_axis_conformance
{
    using sorted_table_probe = sorted_table<int>;

    static_assert(is_iterable_container_v<sorted_table_probe>,
                  "sorted_table must classify as iterable.");
    static_assert(is_ordered_container_v<sorted_table_probe>,
                  "sorted_table must classify as ordered.");
    static_assert(sortedness_of<sorted_table_probe>::value == sortedness::sorted,
                  "sorted_table must classify as SORTED (the varsigma overlay).");
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_SORTED_TABLE_
