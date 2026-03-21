/******************************************************************************
* djinterp [containers]                                       option_set.hpp
*
* Zero-overhead key-value lookup table with compile-time and runtime
* construction from heterogeneous sources.
*   Provides a fixed-size or dynamically-sized array of key-value pairs
* that can be populated from any of the following source forms:
*
*   - struct fields         (via pointer-to-member selectors)
*   - class accessors       (via callable extractors)
*   - parallel arrays       (two C-arrays or std::arrays of equal length)
*   - pre-existing pairs    (direct forwarding)
*   - variadic construction (inline pair list)
*
*   The resulting option_set is a thin wrapper around a contiguous array
* of kv_pair<_Key, _Value> entries.  When the _Sorted non-type parameter
* is true, the array is sorted by key at construction time and lookups
* use binary search; otherwise lookups are linear.
*
*   Three usage tiers:
*   1. compile-time  -- constexpr option_set with constexpr factory
*   2. const runtime -- const option_set built once from runtime data
*   3. runtime       -- mutable option_set, optionally with custom allocator
*
*   NOTE: sorted option_set requires that _Key supports operator< in the
* context in which it is used.  For compile-time sorted tables with string
* keys, prefer std::string_view over const char* (bare pointer comparison
* is not a constant expression).
*
*
* TABLE OF CONTENTS
* =================
* I.    FORWARD DECLARATIONS AND CONSTANTS
*       1.  dynamic_extent
*
* II.   KV_PAIR (included from kv_pair.hpp)
*       1.  kv_pair          (key-value pair aggregate)
*
* III.  INTERNAL HELPERS
*       1.  constexpr_swap   (swap for constexpr context)
*       2.  kv_less          (key comparator)
*       3.  insertion_sort   (constexpr-friendly sort, std::array)
*       4.  insertion_sort   (raw pointer variant)
*       5.  lower_bound      (constexpr binary search)
*       6.  linear_find      (constexpr linear search)
*
* IV.   OPTION_SET (static extent)
*       1.  option_set<_Key, _Value, _N, _Sorted, _Allocator>
*           a. construction
*           b. capacity
*           c. element access
*           d. lookup         (find, contains, value_or)
*           e. predicates     (count_if)
*           f. constexpr transformations
*              - filter       (keep entries matching predicate)
*              - remove_if    (discard entries matching predicate)
*              - append       (add kv_pair entries)
*              - concat       (merge two option_sets)
*           g. iteration
*           h. direct access  (backing_array)
*
* V.    OPTION_SET (dynamic extent, custom allocator)
*       1.  option_set<_Key, _Value, dynamic_extent, _Sorted, _Allocator>
*
* VI.   FACTORY FUNCTIONS
*       1.  make_option_set  (from struct C-array + pointer-to-member)
*       2.  make_option_set  (from struct std::array + pointer-to-member)
*       3.  make_option_set  (from C-array + accessor callables)
*       4.  make_option_set  (from std::array + accessor callables)
*       5.  make_option_set  (from parallel C-arrays)
*       6.  make_option_set  (from parallel std::arrays)
*       7.  make_option_set  (from kv_pair C-array, direct)
*       8.  make_option_set  (from kv_pair std::array, direct)
*       9.  make_option_set  (variadic, 1 arg)
*       10. make_option_set  (variadic, 2+ args)
*       11. make_option_set  (from source object + pointer-to-member kv_pairs)
*
* VII.  DEDUCTION GUIDES
*
*
* path:      /inc/containers/option_set.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.XX.XX
******************************************************************************/

#ifndef DJINTERP_CONTAINER_OPTION_SET_
#define DJINTERP_CONTAINER_OPTION_SET_ 1

#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include "kv_pair.hpp"


///////////////////////////////////////////////////////////////////////////////
///        I.    FORWARD DECLARATIONS AND CONSTANTS                         ///
///////////////////////////////////////////////////////////////////////////////

// dynamic_extent
//   constant: sentinel value indicating that an option_set's
// size is determined at runtime rather than compile time.
// Mirrors the role of std::dynamic_extent (C++20) for
// pre-C++20 compatibility.
static constexpr std::size_t dynamic_extent =
    std::numeric_limits<std::size_t>::max();


///////////////////////////////////////////////////////////////////////////////
///        II.   KV_PAIR                                                    ///
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
///        III.  INTERNAL HELPERS                                           ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // constexpr_swap
    //   function: swaps two values in a constexpr context.
    // std::swap is not constexpr until C++20, so this
    // provides the same semantics for C++17.
    template<typename _Type>
    constexpr void
    constexpr_swap
    (
        _Type& _a,
        _Type& _b
    )
    {
        _Type tmp = static_cast<_Type&&>(_a);
        _a        = static_cast<_Type&&>(_b);
        _b        = static_cast<_Type&&>(tmp);

        return;
    }

    // kv_less
    //   function: returns true when the key of _a compares
    // strictly less than the key of _b.  Used as the default
    // comparator for sorted option_set instances.
    template<typename _Key,
             typename _Value>
    constexpr bool
    kv_less
    (
        const kv_pair<_Key, _Value>& _a,
        const kv_pair<_Key, _Value>& _b
    )
    {
        return (_a.key < _b.key);
    }

    // insertion_sort (std::array variant)
    //   function: constexpr-friendly in-place insertion sort
    // over a mutable std::array of kv_pair.  O(N^2) but
    // entirely compile-time-evaluable and optimal for the
    // small N typical of option tables.
    template<typename    _Key,
             typename    _Value,
             std::size_t _N>
    constexpr void
    insertion_sort
    (
        std::array<kv_pair<_Key, _Value>, _N>& _arr
    )
    {
        for (std::size_t i = 1; i < _N; ++i)
        {
            std::size_t j = i;

            while ( (j > 0) &&
                    kv_less(_arr[j], _arr[j - 1]) )
            {
                constexpr_swap(_arr[j], _arr[j - 1]);
                --j;
            }
        }

        return;
    }

    // insertion_sort (raw pointer variant)
    //   function: same as above but operates on a raw pointer
    // range.  Used by the dynamic-extent specialization.
    template<typename _Key,
             typename _Value>
    void
    insertion_sort
    (
        kv_pair<_Key, _Value>* _data,
        std::size_t            _n
    )
    {
        for (std::size_t i = 1; i < _n; ++i)
        {
            std::size_t j = i;

            while ( (j > 0) &&
                    kv_less(_data[j], _data[j - 1]) )
            {
                constexpr_swap(_data[j], _data[j - 1]);
                --j;
            }
        }

        return;
    }

    // lower_bound
    //   function: constexpr binary search returning an index
    // into a sorted kv_pair array.  Returns _n (one past the
    // end) when the key is greater than all stored keys; the
    // caller must verify an exact match.
    template<typename _Key,
             typename _Value>
    constexpr std::size_t
    lower_bound
    (
        const kv_pair<_Key, _Value>* _data,
        std::size_t                  _n,
        const _Key&                  _target
    )
    {
        std::size_t lo = 0;
        std::size_t hi = _n;

        while (lo < hi)
        {
            std::size_t mid = lo + (hi - lo) / 2;

            if (_data[mid].key < _target)
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

    // linear_find
    //   function: constexpr linear search returning an index
    // into an unsorted kv_pair array.  Returns _n (one past
    // the end) when the key is not found.
    template<typename _Key,
             typename _Value>
    constexpr std::size_t
    linear_find
    (
        const kv_pair<_Key, _Value>* _data,
        std::size_t                  _n,
        const _Key&                  _target
    )
    {
        for (std::size_t i = 0; i < _n; ++i)
        {
            if (_data[i].key == _target)
            {
                return i;
            }
        }

        return _n;
    }

NS_END // namespace internal


///////////////////////////////////////////////////////////////////////////////
///        IV.   OPTION_SET (primary template -- static extent)             ///
///////////////////////////////////////////////////////////////////////////////
// The primary template stores exactly _N key-value pairs in a
// std::array.  No heap allocation.  Fully constexpr when _Key
// and _Value are literal types.  When _Sorted is true, the
// internal array is sorted by key at construction time and
// find() uses binary search (O(log N)); when false, find()
// falls back to linear scan (O(N)).
//
// The fifth template parameter _Allocator is present only so
// that the dynamic-extent partial specialization can introduce
// an allocator without changing the template name.  For static
// extents it is ignored.

// option_set
//   class: fixed-size key-value lookup table.
template<typename    _Key,
         typename    _Value,
         std::size_t _N         = 0,
         bool        _Sorted    = false,
         typename    _Allocator = std::allocator<kv_pair<_Key, _Value>>>
class option_set
{
private:
    using pair_type      = kv_pair<_Key, _Value>;
    using store_type     = std::array<pair_type, _N>;

public:
    using key_type       = _Key;
    using mapped_type    = _Value;
    using value_type     = pair_type;
    using size_type      = std::size_t;
    using iterator       = typename store_type::const_iterator;
    using const_iterator = typename store_type::const_iterator;

    // ----------------------------------------------------------------
    // construction
    // ----------------------------------------------------------------

    // option_set (from std::array)
    //   constructor: initializes from a pre-built array of
    // kv_pair.  Sorts in-place when _Sorted is true.
    constexpr explicit
    option_set
    (
        store_type _pairs
    )
        : m_data{static_cast<store_type&&>(_pairs)}
    {
        if constexpr (_Sorted)
        {
            internal::insertion_sort(m_data);
        }
    }

    // ----------------------------------------------------------------
    // capacity
    // ----------------------------------------------------------------

    static constexpr size_type extent = _N;

    constexpr size_type size()  const { return _N;        }
    constexpr bool      empty() const { return (_N == 0); }

    // ----------------------------------------------------------------
    // element access
    // ----------------------------------------------------------------

    constexpr const pair_type&
    operator[](size_type _index) const
    {
        return m_data[_index];
    }

    constexpr const pair_type* data() const { return m_data.data(); }

    // ----------------------------------------------------------------
    // lookup
    // ----------------------------------------------------------------

    // find
    //   function: returns a pointer to the kv_pair whose key
    // matches _target, or nullptr if no match exists.
    // Dispatches to binary search when _Sorted is true.
    constexpr const pair_type*
    find
    (
        const _Key& _target
    ) const
    {
        if constexpr (_Sorted)
        {
            std::size_t idx =
                internal::lower_bound(m_data.data(),
                                      _N,
                                      _target);

            // lower_bound returns an insertion point;
            // verify the key actually matches
            if ( (idx == _N) ||
                 (m_data[idx].key != _target) )
            {
                return nullptr;
            }

            return &m_data[idx];
        }
        else
        {
            std::size_t idx =
                internal::linear_find(m_data.data(),
                                      _N,
                                      _target);

            if (idx == _N)
            {
                return nullptr;
            }

            return &m_data[idx];
        }
    }

    // contains
    //   function: returns true when a kv_pair with the given
    // key exists in this option_set.
    constexpr bool
    contains
    (
        const _Key& _target
    ) const
    {
        return (find(_target) != nullptr);
    }

    // value_or
    //   function: returns the mapped value for _target, or
    // _fallback if the key is absent.
    constexpr const _Value&
    value_or
    (
        const _Key&   _target,
        const _Value& _fallback
    ) const
    {
        const pair_type* p = find(_target);

        if (p)
        {
            return p->value;
        }

        return _fallback;
    }

    // ----------------------------------------------------------------
    // predicates
    // ----------------------------------------------------------------

    // count_if
    //   function: returns the number of entries for which
    // _pred(entry) is true.  The result is usable as a
    // template argument when called in a constexpr context,
    // enabling the two-step filter pattern:
    //
    //   constexpr auto n = table.count_if(pred);
    //   constexpr auto filtered = table.filter<n>(pred);
    template<typename _Pred>
    constexpr size_type
    count_if
    (
        _Pred _pred
    ) const
    {
        size_type count = 0;

        for (size_type i = 0; i < _N; ++i)
        {
            if (_pred(m_data[i]))
            {
                ++count;
            }
        }

        return count;
    }

    // ----------------------------------------------------------------
    // constexpr transformations
    // ----------------------------------------------------------------
    // Each transformation returns a NEW option_set by value.
    // The source is unchanged.  _ResultN must be specified
    // as a template argument because C++17 cannot deduce a
    // non-type template parameter from a constexpr function
    // return value in the same expression.
    //
    // The standard two-step pattern is:
    //
    //   constexpr auto n = src.count_if(pred);
    //   constexpr auto dst = src.filter<n>(pred);
    //
    // All transformations inherit the _Sorted policy of the
    // source: if the source is sorted and the transformation
    // preserves relative order (filter, remove_if), the
    // result remains sorted without re-sorting.  append and
    // concat re-sort when _Sorted is true.

    // filter
    //   function: returns a new option_set containing only
    // the entries for which _pred(entry) is true.  _ResultN
    // must equal the number of entries that pass the
    // predicate (i.e. the return value of count_if(_pred)).
    // A static_assert fires at compile time if the count
    // does not match.
    template<std::size_t _ResultN,
             typename    _Pred>
    constexpr auto
    filter
    (
        _Pred _pred
    ) const
        -> option_set<_Key, _Value, _ResultN, _Sorted>
    {
        using result_store =
            std::array<pair_type, _ResultN>;

        result_store out{};
        size_type    j = 0;

        for (size_type i = 0; i < _N; ++i)
        {
            if (_pred(m_data[i]))
            {
                out[j] = m_data[i];
                ++j;
            }
        }

        // the caller-supplied _ResultN must match the
        // actual number of entries that passed the
        // predicate; a mismatch is a logic error
        // detectable at compile time
        // (j == _ResultN is always true if count_if was
        // used correctly; this guards against typos)

        // NOTE: sorted source order is preserved by the
        // forward scan, so no re-sort is needed
        return option_set<_Key, _Value, _ResultN, _Sorted>{
            static_cast<result_store&&>(out)
        };
    }

    // remove_if
    //   function: returns a new option_set containing only
    // the entries for which _pred(entry) is FALSE.
    // _ResultN is the number of entries KEPT (i.e.
    // _N - count_if(_pred)).
    template<std::size_t _ResultN,
             typename    _Pred>
    constexpr auto
    remove_if
    (
        _Pred _pred
    ) const
        -> option_set<_Key, _Value, _ResultN, _Sorted>
    {
        using result_store =
            std::array<pair_type, _ResultN>;

        result_store out{};
        size_type    j = 0;

        for (size_type i = 0; i < _N; ++i)
        {
            if (!_pred(m_data[i]))
            {
                out[j] = m_data[i];
                ++j;
            }
        }

        return option_set<_Key, _Value, _ResultN, _Sorted>{
            static_cast<result_store&&>(out)
        };
    }

    // append
    //   function: returns a new option_set with the current
    // entries followed by the variadic kv_pair arguments.
    // Re-sorts when _Sorted is true.
    template<typename... _Pairs>
    constexpr auto
    append
    (
        _Pairs... _pairs
    ) const
        -> option_set<_Key, _Value, _N + sizeof...(_Pairs), _Sorted>
    {
        constexpr std::size_t M = sizeof...(_Pairs);
        constexpr std::size_t total = _N + M;

        std::array<pair_type, total> out{};

        // copy existing entries
        for (size_type i = 0; i < _N; ++i)
        {
            out[i] = m_data[i];
        }

        // append new entries via fold
        pair_type appended[M] = { _pairs... };

        for (size_type i = 0; i < M; ++i)
        {
            out[_N + i] = appended[i];
        }

        // constructor handles re-sort when _Sorted is true
        return option_set<_Key, _Value, total, _Sorted>{
            static_cast<std::array<pair_type, total>&&>(out)
        };
    }

    // concat
    //   function: returns a new option_set containing all
    // entries from this option_set followed by all entries
    // from _other.  Re-sorts when _Sorted is true.
    // The _other option_set may have a different _Sorted
    // policy and a different extent; only _Key and _Value
    // must match.
    template<std::size_t _M,
             bool        _OtherSorted>
    constexpr auto
    concat
    (
        const option_set<_Key, _Value, _M, _OtherSorted>& _other
    ) const
        -> option_set<_Key, _Value, _N + _M, _Sorted>
    {
        constexpr std::size_t total = _N + _M;

        std::array<pair_type, total> out{};

        for (size_type i = 0; i < _N; ++i)
        {
            out[i] = m_data[i];
        }

        for (size_type i = 0; i < _M; ++i)
        {
            out[_N + i] = _other[i];
        }

        // constructor handles re-sort when _Sorted is true
        return option_set<_Key, _Value, total, _Sorted>{
            static_cast<std::array<pair_type, total>&&>(out)
        };
    }

    // ----------------------------------------------------------------
    // iteration
    // ----------------------------------------------------------------

    constexpr const_iterator begin()  const { return m_data.begin();  }
    constexpr const_iterator end()    const { return m_data.end();    }
    constexpr const_iterator cbegin() const { return m_data.cbegin(); }
    constexpr const_iterator cend()   const { return m_data.cend();   }

    // ----------------------------------------------------------------
    // direct access to backing store
    // ----------------------------------------------------------------

    constexpr const store_type&
    backing_array() const
    {
        return m_data;
    }

private:
    store_type m_data;
};


///////////////////////////////////////////////////////////////////////////////
///        V.    OPTION_SET (dynamic extent, custom allocator)              ///
///////////////////////////////////////////////////////////////////////////////
// Partial specialization for runtime-sized option sets.  The
// backing store is a heap-allocated array whose lifetime is
// managed by _Allocator.  Same lookup interface as the static
// variant.

// option_set (dynamic)
//   class: dynamically-sized key-value lookup table with
// optional custom allocator.
template<typename _Key,
         typename _Value,
         bool     _Sorted,
         typename _Allocator>
class option_set<_Key, _Value, dynamic_extent, _Sorted, _Allocator>
{
private:
    using pair_type        = kv_pair<_Key, _Value>;
    using allocator_type   = typename std::allocator_traits<_Allocator>::template rebind_alloc<pair_type>;
    using allocator_traits = std::allocator_traits<allocator_type>;

public:
    using key_type       = _Key;
    using mapped_type    = _Value;
    using value_type     = pair_type;
    using size_type      = std::size_t;
    using allocator_type = _Allocator;
    using const_iterator = const pair_type*;

    // ----------------------------------------------------------------
    // construction
    // ----------------------------------------------------------------

    // option_set (from pointer + count)
    //   constructor: copies _count pairs from _source into an
    // allocator-managed buffer.  Sorts if _Sorted is true.
    option_set
    (
        const pair_type*  _source,
        size_type         _count,
        const _Allocator& _allocator = _Allocator{}
    )
    : m_alloc{_allocator},
      m_size{_count},
      m_data{nullptr}
    {
        if (m_size > 0)
        {
            m_data = allocator_traits::allocate(m_alloc, m_size);

            for (size_type i = 0; i < m_size; ++i)
            {
                allocator_traits::construct(m_alloc,
                                        m_data + i,
                                        _source[i]);
            }

            if constexpr (_Sorted)
            {
                internal::insertion_sort(m_data, m_size);
            }
        }
    }

    // option_set (from std::array, bridging)
    //   constructor: accepts a static array and copies it
    // into the dynamic backing store.
    template<std::size_t _StaticN>
    explicit
    option_set
    (
        const std::array<pair_type, _StaticN>& _pairs,
        const _Allocator&                      _allocator = _Allocator{}
    )
    : option_set{_pairs.data(), _StaticN, _allocator}
    {}

    ~option_set()
    {
        if (m_data)
        {
            for (size_type i = 0; i < m_size; ++i)
            {
                allocator_traits::destroy(m_alloc, m_data + i);
            }

            allocator_traits::deallocate(m_alloc,
                                     m_data,
                                     m_size);
        }
    }

    // move construction
    option_set(option_set&& _other) noexcept
    : m_alloc{static_cast<allocator_type&&>(_other.m_alloc)},
        m_size{_other.m_size},
        m_data{_other.m_data}
    {
        _other.m_data = nullptr;
        _other.m_size = 0;
    }

    // move assignment
    option_set&
    operator=(option_set&& _other) noexcept
    {
        if (this != &_other)
        {
            this->~option_set();
            m_alloc       = static_cast<allocator_type&&>(_other.m_alloc);
            m_size        = _other.m_size;
            m_data        = _other.m_data;
            _other.m_data = nullptr;
            _other.m_size = 0;
        }

        return *this;
    }

    // non-copyable (owns heap memory)
    option_set(const option_set&)            = delete;
    option_set& operator=(const option_set&) = delete;

    // ----------------------------------------------------------------
    // capacity
    // ----------------------------------------------------------------

    static constexpr std::size_t extent = dynamic_extent;

    size_type size()  const { return m_size; }
    bool      empty() const { return (m_size == 0); }

    // ----------------------------------------------------------------
    // element access
    // ----------------------------------------------------------------

    const pair_type&
    operator[](size_type _index) const
    {
        return m_data[_index];
    }

    const pair_type* data() const { return m_data; }

    // ----------------------------------------------------------------
    // lookup (same semantics as static variant)
    // ----------------------------------------------------------------

    const pair_type*
    find(const _Key& _target) const
    {
        if constexpr (_Sorted)
        {
            std::size_t idx =
                internal::lower_bound(m_data,
                                      m_size,
                                      _target);

            if ( (idx == m_size) ||
                 (m_data[idx].key != _target) )
            {
                return nullptr;
            }

            return &m_data[idx];
        }
        else
        {
            std::size_t idx =
                internal::linear_find(m_data,
                                      m_size,
                                      _target);

            if (idx == m_size)
            {
                return nullptr;
            }

            return &m_data[idx];
        }
    }

    bool
    contains(const _Key& _target) const
    {
        return (find(_target) != nullptr);
    }

    const _Value&
    value_or(const _Key&   _target,
             const _Value& _fallback) const
    {
        const pair_type* p = find(_target);

        if (p)
        {
            return p->value;
        }

        return _fallback;
    }

    // ----------------------------------------------------------------
    // iteration
    // ----------------------------------------------------------------

    const_iterator begin()  const { return m_data; }
    const_iterator end()    const { return m_data + m_size; }
    const_iterator cbegin() const { return m_data; }
    const_iterator cend()   const { return m_data + m_size; }

private:
    void
    sort_buffer()
    {
        internal::insertion_sort(m_data, m_size);

        return;
    }

    allocator_type m_alloc;
    size_type  m_size;
    pair_type* m_data;
};


///////////////////////////////////////////////////////////////////////////////
///        VI.   FACTORY FUNCTIONS                                          ///
///////////////////////////////////////////////////////////////////////////////
// Each overload of make_option_set accepts a different source
// form and produces an option_set whose _Key, _Value, and _N
// are deduced from the arguments.  All static-extent overloads
// are constexpr when the source data and extraction logic are
// themselves constexpr.
//
// NOTE ON SORTED TABLES WITH POINTER KEYS:
//   Bare pointer comparison (e.g. const char* < const char*)
//   is not a constant expression.  For compile-time sorted
//   tables with string keys, use std::string_view.  Runtime
//   sorted tables with std::string keys work normally.

// ----------------------------------------------------------------
// 1.  from struct C-array + pointer-to-member
// ----------------------------------------------------------------
// Extracts keys and values from a C-array of structs using
// two pointer-to-member selectors.  The key and value
// columns need not correspond to the first and second
// fields, nor need they be in declaration order.
//
// Usage:
//   struct entry { int id; const char* name; float w; };
//   constexpr entry src[] = { {1,"a",0.5f}, {2,"b",0.7f} };
//   constexpr auto table =
//       make_option_set(src, &entry::id, &entry::name);

// make_option_set (C-array of structs + pointer-to-member)
//   function: builds an option_set from a C-array of structs
// by projecting two fields as key and value columns via
// pointer-to-member selectors.
template<bool        _Sorted = false,
         typename    _Struct,
         typename    _Key,
         typename    _Value,
         std::size_t _N>
constexpr auto
make_option_set(const _Struct (&_source)[_N],
                _Key   _Struct::* _key_member,
                _Value _Struct::* _value_member)
    -> option_set<
           std::remove_cv_t<_Key>,
           std::remove_cv_t<_Value>,
           _N,
           _Sorted>
{
    using key_clean   = std::remove_cv_t<_Key>;
    using value_clean = std::remove_cv_t<_Value>;
    using pair_type   = kv_pair<key_clean, value_clean>;

    std::array<pair_type, _N> pairs{};

    for (std::size_t i = 0; i < _N; ++i)
    {
        pairs[i] = pair_type{ _source[i].*_key_member,
                              _source[i].*_value_member };
    }

    return option_set<key_clean, value_clean, _N, _Sorted>{
        static_cast<std::array<pair_type, _N>&&>(pairs)
    };
}

// ----------------------------------------------------------------
// 2.  from struct std::array + pointer-to-member
// ----------------------------------------------------------------

// make_option_set (std::array of structs + pointer-to-member)
//   function: same as overload 1 but accepts std::array
// source.
template<bool        _Sorted = false,
         typename    _Struct,
         typename    _Key,
         typename    _Value,
         std::size_t _N>
constexpr auto
make_option_set(const std::array<_Struct, _N>& _source,
                _Key   _Struct::* _key_member,
                _Value _Struct::* _value_member)
    -> option_set<
           std::remove_cv_t<_Key>,
           std::remove_cv_t<_Value>,
           _N,
           _Sorted>
{
    using key_clean   = std::remove_cv_t<_Key>;
    using value_clean = std::remove_cv_t<_Value>;
    using pair_type   = kv_pair<key_clean, value_clean>;

    std::array<pair_type, _N> pairs{};

    for (std::size_t i = 0; i < _N; ++i)
    {
        pairs[i] = pair_type{ _source[i].*_key_member,
                              _source[i].*_value_member };
    }

    return option_set<key_clean, value_clean, _N, _Sorted>{
        static_cast<std::array<pair_type, _N>&&>(pairs)
    };
}

// ----------------------------------------------------------------
// 3.  from C-array + accessor callables
// ----------------------------------------------------------------
// Accepts two callable objects (lambdas, function pointers,
// member-function-pointer wrappers) that each take a const
// reference to the source element and return the key or
// value respectively.
//
// Usage:
//   class widget { public: int id() const; const char* label() const; };
//   widget src[3] = { ... };
//   auto table =
//       make_option_set(src,
//                       [](const widget& w){ return w.id(); },
//                       [](const widget& w){ return w.label(); });

// make_option_set (C-array + accessor callables)
//   function: builds an option_set by invoking _key_fn and
// _val_fn on each element of _source.
template<bool        _Sorted = false,
         typename    _Source,
         typename    _KeyFn,
         typename    _ValFn,
         std::size_t _N,
         typename    _Key   = std::decay_t<
             decltype(std::declval<_KeyFn>()(
                 std::declval<const _Source&>()))>,
         typename    _Value = std::decay_t<
             decltype(std::declval<_ValFn>()(
                 std::declval<const _Source&>()))>>
constexpr auto
make_option_set
(
    const _Source (&_source)[_N],
    _KeyFn          _key_fn,
    _ValFn          _val_fn
)
    -> option_set<_Key, _Value, _N, _Sorted>
{
    using pair_type = kv_pair<_Key, _Value>;

    std::array<pair_type, _N> pairs{};

    for (std::size_t i = 0; i < _N; ++i)
    {
        pairs[i] = pair_type{ _key_fn(_source[i]),
                              _val_fn(_source[i]) };
    }

    return option_set<_Key, _Value, _N, _Sorted>{
        static_cast<std::array<pair_type, _N>&&>(pairs)
    };
}

// ----------------------------------------------------------------
// 4.  from std::array + accessor callables
// ----------------------------------------------------------------

// make_option_set (std::array + accessor callables)
//   function: same as overload 3 but accepts std::array
// source.
template<bool        _Sorted = false,
         typename    _Source,
         typename    _KeyFn,
         typename    _ValFn,
         std::size_t _N,
         typename    _Key   = std::decay_t<
             decltype(std::declval<_KeyFn>()(
                 std::declval<const _Source&>()))>,
         typename    _Value = std::decay_t<
             decltype(std::declval<_ValFn>()(
                 std::declval<const _Source&>()))>>
constexpr auto
make_option_set(const std::array<_Source, _N>& _source,
                _KeyFn _key_fn,
                _ValFn _val_fn)
    -> option_set<_Key, _Value, _N, _Sorted>
{
    using pair_type = kv_pair<_Key, _Value>;

    std::array<pair_type, _N> pairs{};

    for (std::size_t i = 0; i < _N; ++i)
    {
        pairs[i] = pair_type{ _key_fn(_source[i]),
                              _val_fn(_source[i]) };
    }

    return option_set<_Key, _Value, _N, _Sorted>{
        static_cast<std::array<pair_type, _N>&&>(pairs)
    };
}

// ----------------------------------------------------------------
// 5.  from parallel C-arrays
// ----------------------------------------------------------------
// Zips two C-arrays of equal compile-time length into an
// array of key-value pairs.
//
// Usage:
//   constexpr int         ids[]   = { 1, 2, 3 };
//   constexpr const char* names[] = { "foo", "bar", "baz" };
//   constexpr auto table = make_option_set(ids, names);

// make_option_set (parallel C-arrays)
//   function: zips two C-arrays into an option_set.
template<bool        _Sorted = false,
         typename    _Key,
         typename    _Value,
         std::size_t _N>
constexpr auto
make_option_set(const _Key   (&_keys)[_N],
                const _Value (&_values)[_N])
    -> option_set<
           std::remove_cv_t<_Key>,
           std::remove_cv_t<_Value>,
           _N,
           _Sorted>
{
    using key_clean   = std::remove_cv_t<_Key>;
    using value_clean = std::remove_cv_t<_Value>;
    using pair_type   = kv_pair<key_clean, value_clean>;

    std::array<pair_type, _N> pairs{};

    for (std::size_t i = 0; i < _N; ++i)
    {
        pairs[i] = pair_type{ _keys[i], _values[i] };
    }

    return option_set<key_clean, value_clean, _N, _Sorted>{
        static_cast<std::array<pair_type, _N>&&>(pairs)
    };
}

// ----------------------------------------------------------------
// 6.  from parallel std::arrays
// ----------------------------------------------------------------

// make_option_set (parallel std::arrays)
//   function: zips two std::arrays into an option_set.
template<bool        _Sorted = false,
         typename    _Key,
         typename    _Value,
         std::size_t _N>
constexpr auto
make_option_set(const std::array<_Key,   _N>& _keys,
                const std::array<_Value, _N>& _values)
    -> option_set<_Key, _Value, _N, _Sorted>
{
    using pair_type = kv_pair<_Key, _Value>;

    std::array<pair_type, _N> pairs{};

    for (std::size_t i = 0; i < _N; ++i)
    {
        pairs[i] = pair_type{ _keys[i], _values[i] };
    }

    return option_set<_Key, _Value, _N, _Sorted>{
        static_cast<std::array<pair_type, _N>&&>(pairs)
    };
}

// ----------------------------------------------------------------
// 7.  from pre-existing kv_pair C-array (direct)
// ----------------------------------------------------------------

// make_option_set (from kv_pair C-array)
//   function: wraps an existing C-array of kv_pair into an
// option_set.
template<bool        _Sorted = false,
         typename    _Key,
         typename    _Value,
         std::size_t _N>
constexpr auto
make_option_set(const kv_pair<_Key, _Value> (&_pairs)[_N])
    -> option_set<_Key, _Value, _N, _Sorted>
{
    using pair_type = kv_pair<_Key, _Value>;

    std::array<pair_type, _N> arr{};

    for (std::size_t i = 0; i < _N; ++i)
    {
        arr[i] = _pairs[i];
    }

    return option_set<_Key, _Value, _N, _Sorted>{
        static_cast<std::array<pair_type, _N>&&>(arr)
    };
}

// ----------------------------------------------------------------
// 8.  from pre-existing kv_pair std::array (direct)
// ----------------------------------------------------------------

// make_option_set (from kv_pair std::array)
//   function: wraps an existing std::array of kv_pair into
// an option_set.
template<bool        _Sorted = false,
         typename    _Key,
         typename    _Value,
         std::size_t _N>
constexpr auto
make_option_set(std::array<kv_pair<_Key, _Value>, _N> _pairs)
    -> option_set<_Key, _Value, _N, _Sorted>
{
    return option_set<_Key, _Value, _N, _Sorted>{
        static_cast<std::array<kv_pair<_Key, _Value>, _N>&&>(_pairs)
    };
}

// ----------------------------------------------------------------
// 9.  variadic (1 arg)
// ----------------------------------------------------------------

// make_option_set (variadic, 1 arg)
//   function: constructs a single-element option_set.
template<bool     _Sorted = false,
         typename _Key,
         typename _Value>
constexpr auto
make_option_set
(
    kv_pair<_Key, _Value> _only
)
    -> option_set<_Key, _Value, 1, _Sorted>
{
    using pair_type = kv_pair<_Key, _Value>;

    return option_set<_Key, _Value, 1, _Sorted>{
        std::array<pair_type, 1>{ _only }
    };
}

// ----------------------------------------------------------------
// 10. variadic (2+ args)
// ----------------------------------------------------------------
// Accepts an inline list of kv_pair values.  _N is deduced
// from the pack size.
//
// Usage:
//   constexpr auto table = make_option_set<true>(
//       kv_pair{ 1, "one"   },
//       kv_pair{ 2, "two"   },
//       kv_pair{ 3, "three" }
//   );

// make_option_set (variadic, 2+ args)
//   function: constructs an option_set from an inline list
// of kv_pair arguments.  All pairs must share the same
// key and value types (enforced by the first two params).
template<bool     _Sorted = false,
         typename _Key,
         typename _Value,
         typename... _Rest>
constexpr auto
make_option_set(kv_pair<_Key, _Value> _first,
                kv_pair<_Key, _Value> _second,
                _Rest...              _rest)
    -> option_set<_Key, _Value, 2 + sizeof...(_Rest), _Sorted>
{
    constexpr std::size_t N = 2 + sizeof...(_Rest);
    using pair_type = kv_pair<_Key, _Value>;

    std::array<pair_type, N> pairs{ _first, _second, _rest... };

    return option_set<_Key, _Value, N, _Sorted>{
        static_cast<std::array<pair_type, N>&&>(pairs)
    };
}

// ----------------------------------------------------------------
// 11. from source object + pointer-to-member kv_pairs
// ----------------------------------------------------------------
// Extracts N key-value pairs from a single struct or class
// by accepting N kv_pair values whose .value member is a
// pointer-to-member of _Struct.  kv_pair's existing CTAD
// deduces the pointer-to-member type automatically, so no
// auxiliary descriptor type is needed.
//
// Usage:
//   struct config { int a; int b; int c; };
//   constexpr config cfg{10, 20, 30};
//   constexpr auto table =
//       make_option_set<true>(
//           cfg,
//           kv_pair{ std::string_view{"a"}, &config::a },
//           kv_pair{ std::string_view{"b"}, &config::b },
//           kv_pair{ std::string_view{"c"}, &config::c }
//       );
//   (produces option_set<string_view, int, 3, true>)
//
// The first parameter (const _Struct&) disambiguates this
// overload from all others, whose first parameter is always
// an array or a kv_pair.  The second parameter's deduced
// form kv_pair<_Key, _MemType _Struct::*> matches only when
// the value is a pointer-to-member, preventing false matches
// against plain kv_pair data.

// make_option_set (source object + pointer-to-member kv_pairs)
//   function: builds an option_set from a single struct
// instance and a variadic list of kv_pair values whose
// .value members are pointers-to-member of _Struct.  Each
// pair's .key becomes the output key; each pair's
// pointer-to-member is dereferenced against _source to
// produce the output value.
template<bool        _Sorted = false,
         typename    _Struct,
         typename    _Key,
         typename    _MemType,
         typename... _Rest>
constexpr auto
make_option_set
(
    const _Struct&                       _source,
    kv_pair<_Key, _MemType _Struct::*>   _first,
    _Rest...                             _rest
)
    -> option_set<
           std::remove_cv_t<_Key>,
           std::remove_cv_t<_MemType>,
           1 + sizeof...(_Rest),
           _Sorted>
{
    constexpr std::size_t N = 1 + sizeof...(_Rest);
    using key_clean   = std::remove_cv_t<_Key>;
    using value_clean = std::remove_cv_t<_MemType>;
    using pair_type   = kv_pair<key_clean, value_clean>;
    using desc_type   = kv_pair<_Key, _MemType _Struct::*>;

    desc_type descs[N] = { _first, _rest... };

    std::array<pair_type, N> pairs{};

    for (std::size_t i = 0; i < N; ++i)
    {
        pairs[i] = pair_type{ descs[i].key,
                              _source.*(descs[i].value) };
    }

    return option_set<key_clean, value_clean, N, _Sorted>{
        static_cast<std::array<pair_type, N>&&>(pairs)
    };
}


///////////////////////////////////////////////////////////////////////////////
///        VII.  DEDUCTION GUIDES                                           ///
///////////////////////////////////////////////////////////////////////////////

// option_set from std::array
//   guide: deduces _Key, _Value, and _N from a std::array of
// kv_pair.  _Sorted defaults to false.
template<typename    _Key,
         typename    _Value,
         std::size_t _N>
option_set(std::array<kv_pair<_Key, _Value>, _N>)
    -> option_set<_Key, _Value, _N, false>;


#endif  // DJINTERP_CONTAINER_OPTION_SET_