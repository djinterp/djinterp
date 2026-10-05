/*******************************************************************************
* djinterp [core]                                                keyed_table.hpp
*
*   keyed_table -- the KEYED relational overlay (Overlays: containers as
* restriction bundles), the axis-typed table realised as an array of records.
* Where the cell-homogeneous tables carry one element type at every cell, a
* relational row is a heterogeneous RECORD -- the value type is a product
* tau = Key x Val -- and the overlay keys it through a projection key: tau ->
* Key.
* This is the RowType form the cell-homogeneous modules deferred.
*
*   THE BUNDLE. Over a backing sequence of RowType records this container
* wears:
*
*     keyed  eta_{Key,Val} (static)      the value type is a record and a
*                                         projection extracts its key; the
*                                         duplicate-equivalence is re-based
*                                       onto
*                                         the key (E -> E_key), so a
*                                       "duplicate"
*                                         is a repeated KEY, whatever its
*                                       value.
*     sorted varsigma (sequence-level) records are kept in non-decreasing key
*                                         order, so a comparator supplies the
*                                       one
*                                         monotone enumeration and lookup is a
*                                         binary search.
*     multiplicity on E_key               mu_1 (unique keys) gives the MAP;
*                                         mu_m>1 (keys may repeat) the
*                                       MULTIMAP.
*
*   Thus keyed_table<..., true> = {eta, mu_1^{E_key}, varsigma} -- a sorted
* map,
*        keyed_table<..., false> = {eta, mu_m^{E_key}, varsigma}  -- a sorted
*   multimap.  The horizontal strengthening of the overlay lattice (map <=
*   multimap) is exactly the UniqueKeys flag.
*
*   PRESERVATION. insert places a record at its sorted key position (or, for a
* map, replaces the record of an equal key); erase removes without disturbing
* the order of the rest. Records are exposed read-only (rewriting a stored key
* would break eta and varsigma), so the surface is keyed lookup +
* order-preserving
* structural change -- the discipline an overlay demands.
*
*   BACKING INDEPENDENCE.  The overlay names no backing beyond a contiguous
* record store; its identity is the restriction set. It exposes key_type, so
* the
* framework reads it as a keyed (associative) container rather than a bare
* ordered
* sequence, and reports varsigma through the opt-in sorted_invariant marker.
*
*   PORTABILITY:
*   C++17 (std::invoke for projection by functor OR pointer-to-member; the
* options surface).
*
*
* path:      /inc/djinterp/core/container/table/keyed_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.05
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_keyed_table (detection trait)
      --------------------------------

II.   keyed_table (class)
      -------------------
      1.    member types and overlay / axis markers
      2.    construction
      3.    record access (delegated, const)
      4.    key queries (binary search over the sorted key order)
      5.    structural mutation (invariant-preserving)

III.  make_keyed_table
      ----------------
*/

#ifndef DJINTERP_CONTAINER_TABLE_KEYED_TABLE_HPP
#define DJINTERP_CONTAINER_TABLE_KEYED_TABLE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"                     // NS_*, D_CONSTEXPR, clean_t
#include "../container_options.hpp"                // axis enums, options base


NS_DJINTERP


// ===========================================================================
// I.   is_keyed_table (detection trait)
// ===========================================================================

// keyed_table (fwd)
template<typename    RowType,
         typename    KeyProj,
         typename    KeyCompare,
         bool        UniqueKeys,
         typename    SizeType,
         typename    DifferenceType,
         typename... Options>
class keyed_table;

// is_keyed_table
//   trait: true when Type (after stripping cv/ref) is a specialization of
// keyed_table.
NS_INTERNAL

    template<typename Type>
    struct is_keyed_table_impl : std::false_type
    {};

    template<typename    R,
             typename    P,
             typename    C,
             bool        U,
             typename    S,
             typename    D,
             typename... O>
    struct is_keyed_table_impl<keyed_table<R, P, C, U, S, D, O...>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct is_keyed_table : internal::is_keyed_table_impl<clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool is_keyed_table_v = is_keyed_table<Type>::value;
#endif


// ===========================================================================
// II.  keyed_table (class)
// ===========================================================================

// keyed_table
//   class: a relational table of RowType records, kept sorted by the key that
// KeyProj projects and compared by KeyCompare, with unique keys (map) or
// repeated keys (multimap). Records are stored contiguously and exposed
// read-only; mutation is keyed insert / erase that preserves the bundle.
template<typename    RowType,
         typename    KeyProj,
         typename    KeyCompare     = std::less<>,
         bool        UniqueKeys     = true,
         typename    SizeType       = std::size_t,
         typename    DifferenceType = std::ptrdiff_t,
         typename... Options>
class keyed_table
    : public options_container_base<Options...>
{
private:
    using storage_type = std::vector<RowType>;

public:
    // --- 1. member types and overlay / axis markers ---

    using value_type       = RowType;   // a row is a record (tau = Key x Val)
    using record_type      = RowType;
    using size_type        = SizeType;
    using difference_type  = DifferenceType;
    using reference        = const RowType&;   // records are read-only under eta/varsigma
    using const_reference  = const RowType&;
    using pointer          = const RowType*;
    using const_pointer    = const RowType*;
    using const_iterator   = typename storage_type::const_iterator;
    using key_projection   = KeyProj;
    using key_compare      = KeyCompare;

    // key_type -- the projected key, deduced from applying KeyProj to a
    // record (std::invoke resolves both functors and pointer-to-member
    // projections).
    using key_type = clean_t<
        decltype(std::invoke(std::declval<const KeyProj&>(),
                             std::declval<const RowType&>()))>;

    // npos -- "no such record" sentinel for the positional key queries.
    static constexpr size_type npos = static_cast<size_type>(-1);

    // overlay markers (the restriction bundle this container wears).
    static constexpr bool keyed            = true;          // eta
    static constexpr bool sorted_invariant = true;          // varsigma
    static constexpr bool unique_keys      = UniqueKeys;   // mu_1 vs mu_m on E_key

    // axis positions.
    static constexpr container_lifetime      lifetime      =
        container_lifetime::mutable_storage;
    static constexpr container_storage_kind  storage_kind  =
        container_storage_kind::dynamic_storage;
    static constexpr container_ordering      ordering      =
        container_ordering::sorted;
    static constexpr container_bounds        bounds        =
        container_bounds::unbounded;
    static constexpr container_iterability   iterability   =
        container_iterability::iterable;
    static constexpr container_multiplicity  multiplicity_grade =
        UniqueKeys ? container_multiplicity::unique
                    : container_multiplicity::multi;

    // --- 2. construction ---

    keyed_table()
        : m_rows(),
          m_proj(),
          m_cmp()
    {}

    explicit keyed_table(
        KeyProj     _proj,
        KeyCompare _cmp = KeyCompare()
    )
        : m_rows(),
          m_proj(_proj),
          m_cmp(_cmp)
    {}

    // record list: each record is inserted at its sorted key position, so any
    // input order yields the keyed, sorted invariant.
    keyed_table(
        std::initializer_list<RowType> _records,
        KeyProj                         _proj = KeyProj(),
        KeyCompare                      _cmp  = KeyCompare()
    )
        : m_rows(),
          m_proj(_proj),
          m_cmp(_cmp)
    {
        // insert record by record; each restores eta/varsigma (and mu on E_key)
        for (const RowType& rec : _records)
        {
            insert(rec);
        }
    }

    keyed_table(const keyed_table&)            = default;
    keyed_table(keyed_table&&)                 = default;
    keyed_table& operator=(const keyed_table&) = default;
    keyed_table& operator=(keyed_table&&)      = default;
    ~keyed_table()                             = default;

    // --- 3. record access (delegated, const) ---

    D_NODISCARD size_type size() const noexcept
    {
        return static_cast<size_type>(m_rows.size());
    }

    D_NODISCARD bool empty() const noexcept
    {
        return m_rows.empty();
    }

    // record at a position in key order (unchecked / checked).
    D_NODISCARD const_reference operator[](size_type _i) const
    {
        return m_rows[static_cast<std::size_t>(_i)];
    }

    D_NODISCARD const_reference at(size_type _i) const
    {
        return m_rows.at(static_cast<std::size_t>(_i));
    }

    // key_of -- the key a record projects.
    D_NODISCARD key_type key_of(const RowType& _rec) const
    {
        return std::invoke(m_proj, _rec);
    }

    // record iteration, in key order.
    D_NODISCARD const_iterator begin() const noexcept
    {
        return m_rows.begin();
    }

    D_NODISCARD const_iterator end() const noexcept
    {
        return m_rows.end();
    }

    D_NODISCARD const_iterator cbegin() const noexcept
    {
        return m_rows.cbegin();
    }

    D_NODISCARD const_iterator cend() const noexcept
    {
        return m_rows.cend();
    }

    // --- 4. key queries (binary search over the sorted key order) ---

    // lower_bound -- position of the first record whose key is not less than
    // _k.
    D_NODISCARD size_type lower_bound(const key_type& _k) const
    {
        size_type lo = 0;
        size_type hi = size();

        while (lo < hi)
        {
            const size_type mid = lo + ((hi - lo) / 2);

            if (m_cmp(key_of(m_rows[mid]), _k))
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

    // upper_bound -- position of the first record whose key is greater than _k.
    D_NODISCARD size_type upper_bound(const key_type& _k) const
    {
        size_type lo = 0;
        size_type hi = size();

        while (lo < hi)
        {
            const size_type mid = lo + ((hi - lo) / 2);

            if (m_cmp(_k, key_of(m_rows[mid])))
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

    // find -- pointer to a record whose key is equivalent to _k, or nullptr.
    D_NODISCARD const RowType* find(const key_type& _k) const
    {
        const size_type pos = lower_bound(_k);

        // equivalent under the comparator: neither key is less than the other
        if ( (pos < size()) &&
             (!m_cmp(_k, key_of(m_rows[pos]))) )
        {
            return &m_rows[static_cast<std::size_t>(pos)];
        }

        return nullptr;
    }

    // index_of -- position of a record equivalent to _k in key order, or npos.
    D_NODISCARD size_type index_of(const key_type& _k) const
    {
        const size_type pos = lower_bound(_k);

        if ( (pos < size()) &&
             (!m_cmp(_k, key_of(m_rows[pos]))) )
        {
            return pos;
        }

        return npos;
    }

    // contains_key -- whether any record carries a key equivalent to _k.
    D_NODISCARD bool contains_key(const key_type& _k) const
    {
        return (find(_k) != nullptr);
    }

    // count_key -- how many records carry a key equivalent to _k (0 or 1 when
    // keys are unique).
    D_NODISCARD size_type count_key(const key_type& _k) const
    {
        return (upper_bound(_k) - lower_bound(_k));
    }

    // --- 5. structural mutation (invariant-preserving) ---

    // insert -- place _rec at its sorted key position. For a map (unique keys)
    // a record of an equivalent key is replaced; for a multimap the new record
    // is added after any equal keys. Returns the position it occupies.
    size_type insert(RowType _rec)
    {
        const key_type k = key_of(_rec);

        // map: an equivalent key already present is overwritten in place
        if (UniqueKeys)
        {
            const size_type pos = lower_bound(k);

            if ( (pos < size()) &&
                 (!m_cmp(k, key_of(m_rows[pos]))) )
            {
                m_rows[static_cast<std::size_t>(pos)] =
                    static_cast<RowType&&>(_rec);

                return pos;
            }

            m_rows.insert(m_rows.begin() +
                          static_cast<std::ptrdiff_t>(pos),
                          static_cast<RowType&&>(_rec));

            return pos;
        }

        // multimap: insert after any equal keys (stable append within the run)
        const size_type pos = upper_bound(k);

        m_rows.insert(m_rows.begin() +
                      static_cast<std::ptrdiff_t>(pos),
                      static_cast<RowType&&>(_rec));

        return pos;
    }

    // erase_at -- remove the record at position _i (in key order).
    void erase_at(size_type _i)
    {
        m_rows.erase(m_rows.begin() + static_cast<std::ptrdiff_t>(_i));

        return;
    }

    // erase_key -- remove every record whose key is equivalent to _k; returns
    // the number removed (0 or 1 when keys are unique).
    size_type erase_key(const key_type& _k)
    {
        const size_type lo = lower_bound(_k);
        const size_type hi = upper_bound(_k);

        // the equivalent keys occupy the contiguous run [lo, hi)
        m_rows.erase(m_rows.begin() + static_cast<std::ptrdiff_t>(lo),
                     m_rows.begin() + static_cast<std::ptrdiff_t>(hi));

        return (hi - lo);
    }

    // clear -- drop all records.
    void clear() noexcept
    {
        m_rows.clear();

        return;
    }

private:
    storage_type m_rows;   // records, invariant: sorted by projected key
    KeyProj      m_proj;   // key projection tau -> Key
    KeyCompare   m_cmp;    // strict-weak order on keys
};


// ===========================================================================
// III. make_keyed_table
// ===========================================================================

// make_keyed_table
//   function: build a unique-key (map) keyed_table over RowType records with
// the given key projection, deducing the record and projection types.
template<typename RowType,
         typename KeyProj>
D_NODISCARD keyed_table<RowType, KeyProj>
make_keyed_table(
    std::initializer_list<RowType> _records,
    KeyProj                         _proj)
{
    return keyed_table<RowType, KeyProj>(_records, _proj);
}

// make_multikeyed_table
//   function: the multimap counterpart (keys may repeat).
template<typename RowType,
         typename KeyProj>
D_NODISCARD keyed_table<RowType, KeyProj, std::less<>, false>
make_multikeyed_table(
    std::initializer_list<RowType> _records,
    KeyProj                         _proj)
{
    return keyed_table<RowType, KeyProj, std::less<>, false>(_records, _proj);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TABLE_KEYED_TABLE_HPP
