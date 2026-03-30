/******************************************************************************
* djinterp [container]                                  threadsafe_array.hpp
*
* Thread-safe array container.
*   Synchronized facade over any contiguous backing container.  All
* public operations acquire the appropriate lock internally; the
* caller never touches a mutex directly (though handle-based access
* is available for multi-operation batches).
*
* TEMPLATE PARAMETERS:
*   _Container  — the backing contiguous container
*                 (std::vector<T>, std::array<T,N>, etc.)
*   _Policy     — lock policy (default_lock_policy)
*
* THREE ACCESS TIERS:
*   Lock-free     — size(), empty(), version() via atomic_state.
*                   Zero synchronization cost.
*
*   Single-op     — at(), push_back(), sort(), filter_in_place(),
*                   etc.  Each acquires and releases its own lock.
*                   Convenient but incurs one lock round-trip per
*                   call.
*
*   Handle-based  — read_access() / write_access() return RAII
*                   handles (const_locked_ref / locked_ref) that
*                   hold a lock for the lifetime of the handle.
*                   For batching multiple operations under one
*                   lock.  Also: snapshot() for safe iteration,
*                   batch() for counted batch writes.
*
* CONVENIENCE ALIASES:
*   threadsafe_vector<T, Policy>
*   threadsafe_fixed_array<T, N, Policy>
*
* DEPENDENCIES:
*   threadsafe/container_base.hpp  — CRTP locking base
*   threadsafe/locked_accessor.hpp — locked_ref, locked_apply
*   threadsafe/atomic_state.hpp    — atomic size/version
*   threadsafe/snapshot.hpp        — snapshot_view, batch_guard
*   array_container.hpp            — chunk_ref
*   array_iterator.hpp             — circular_iterator
*
* TABLE OF CONTENTS
* =================
* I.      threadsafe_array
*           a. Construction / Assignment
*           b. Lock-Free Queries
*           c. Handle-Based Access
*           d. Element Access (read-locked)
*           e. Capacity (read-locked / write-locked)
*           f. Modifiers (write-locked)
*           g. Ordering (write-locked)
*           h. Multiplicity (write-locked)
*           i. Search (read-locked)
*           j. Filter (read-locked / write-locked)
*           k. Bulk / Slice (read-locked)
*           l. Optimistic Read
* II.     Convenience Aliases
*
*
* path:      \inc\container\threadsafe_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.29
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_ARRAY_
#define DJINTERP_THREADSAFE_ARRAY_ 1

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>
#include "..\djinterp.hpp"
#include "threadsafe\container_base.hpp"
#include "threadsafe\locked_accessor.hpp"
#include "threadsafe\atomic_state.hpp"
#include "threadsafe\snapshot.hpp"
#include "array_container.hpp"
#include "array_iterator.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <array>
    #include <utility>
#endif


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   threadsafe_array
// =============================================================================

template<typename _Container,
         typename _Policy = default_lock_policy>
class threadsafe_array
    : public threadsafe_container_base<
          threadsafe_array<_Container, _Policy>,
          _Policy>
{
    using base_type = threadsafe_container_base<
        threadsafe_array<_Container, _Policy>,
        _Policy>;

public:
    using container_type  = _Container;
    using value_type      =
        typename _Container::value_type;
    using size_type       = std::size_t;
    using lock_policy_type = _Policy;

    // atomic state type alias for trait detection
    using atomic_size_type =
        decltype(std::declval<atomic_state>().size);

    // =========================================================
    // a. Construction / Assignment
    // =========================================================

    threadsafe_array() = default;

    explicit threadsafe_array(
        const _Container& _c)
        : m_data(_c)
    {
        m_state.store_size(m_data.size());
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    explicit threadsafe_array(_Container&& _c)
        : m_data(std::move(_c))
    {
        m_state.store_size(m_data.size());
    }

    threadsafe_array(
        std::initializer_list<value_type> _init)
        : m_data(_init)
    {
        m_state.store_size(m_data.size());
    }

#endif  // C++11

    // non-copyable (inherits from base)
    // movable on C++11+

    // =========================================================
    // b. Lock-Free Queries
    // =========================================================
    // No synchronization cost.  The atomic_state is
    // maintained by every write operation.

    size_type size() const noexcept
    {
        return m_state.load_size();
    }

    bool empty() const noexcept
    {
        return (m_state.load_size() == 0);
    }

    std::uint64_t version() const noexcept
    {
        return m_state.load_version();
    }

    // =========================================================
    // c. Handle-Based Access
    // =========================================================
    // Return RAII handles holding the lock.  Multiple
    // operations can be performed under one lock.

    // read_access
    //   returns a const_locked_ref holding a read lock.
    const_locked_ref<_Container, _Policy>
    read_access() const
    {
        return const_locked_ref<_Container, _Policy>(
            m_data, base_type::mutex());
    }

    // write_access
    //   returns a locked_ref holding a write lock.
    locked_ref<_Container, _Policy>
    write_access()
    {
        return locked_ref<_Container, _Policy>(
            m_data, base_type::mutex());
    }

    // snapshot
    //   copies all elements under read lock into an
    // independent snapshot_view for safe iteration.
    snapshot_view<_Container, _Policy>
    snapshot() const
    {
        return snapshot_view<_Container, _Policy>(
            m_data, base_type::mutex());
    }

    // batch
    //   returns a batch_guard holding a write lock
    // for multiple operations.
    batch_guard<_Policy>
    batch()
    {
        return batch_guard<_Policy>(
            base_type::mutex());
    }

    // locked_iterate
    //   returns a locked_range for range-for iteration
    // under read lock.
    locked_range<_Container, _Policy>
    locked_iterate() const
    {
        return locked_range<_Container, _Policy>(
            m_data, base_type::mutex());
    }

    // =========================================================
    // d. Element Access (read-locked)
    // =========================================================
    // Each call acquires and releases a read lock.
    // Returns copies — the lock is not held after
    // the call returns.

    // at
    //   returns a copy of the element at _index.
    value_type at(size_type _index) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return m_data[_index];
    }

    // front
    value_type front() const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return m_data.front();
    }

    // back
    value_type back() const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return m_data.back();
    }

    // =========================================================
    // e. Capacity (read-locked / write-locked)
    // =========================================================

    // capacity (read-locked, for dynamic containers)
    template<typename C = _Container>
    auto capacity() const
        -> decltype(std::declval<const C&>().capacity())
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return m_data.capacity();
    }

    // reserve (write-locked)
    template<typename C = _Container>
    auto reserve(size_type _n)
        -> decltype(
            std::declval<C&>().reserve(_n), void())
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data.reserve(_n);
    }

    // shrink_to_fit (write-locked)
    template<typename C = _Container>
    auto shrink_to_fit()
        -> decltype(
            std::declval<C&>().shrink_to_fit(),
            void())
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data.shrink_to_fit();
    }

    // resize (write-locked)
    template<typename C = _Container>
    auto resize(size_type _n)
        -> decltype(
            std::declval<C&>().resize(_n), void())
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data.resize(_n);
        sync_size();
    }

    // resize with fill value
    template<typename C = _Container>
    auto resize(size_type _n,
                const value_type& _val)
        -> decltype(
            std::declval<C&>().resize(
                _n, _val), void())
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data.resize(_n, _val);
        sync_size();
    }

    // =========================================================
    // f. Modifiers (write-locked)
    // =========================================================
    // Each call acquires a write lock, performs the
    // operation, updates atomic_state, and releases.

    // push_back
    template<typename C = _Container>
    auto push_back(const value_type& _val)
        -> decltype(
            std::declval<C&>().push_back(_val),
            void())
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data.push_back(_val);
        m_state.fetch_add_size(1);
        m_state.increment_version();
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // push_back (move)
    template<typename C = _Container>
    auto push_back(value_type&& _val)
        -> decltype(
            std::declval<C&>().push_back(
                std::move(_val)), void())
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data.push_back(std::move(_val));
        m_state.fetch_add_size(1);
        m_state.increment_version();
    }

    // emplace_back
    template<typename... _Args,
             typename C = _Container>
    auto emplace_back(_Args&&... _args)
        -> decltype(
            std::declval<C&>().emplace_back(
                std::forward<_Args>(_args)...),
            void())
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data.emplace_back(
            std::forward<_Args>(_args)...);
        m_state.fetch_add_size(1);
        m_state.increment_version();
    }

#endif  // C++11

    // pop_back
    template<typename C = _Container>
    auto pop_back()
        -> decltype(
            std::declval<C&>().pop_back(), void())
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        if (!m_data.empty())
        {
            m_data.pop_back();
            m_state.fetch_sub_size(1);
            m_state.increment_version();
        }
    }

    // clear
    template<typename C = _Container>
    auto clear()
        -> decltype(
            std::declval<C&>().clear(), void())
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data.clear();
        m_state.store_size(0);
        m_state.increment_version();
    }

    // fill (all elements to _val)
    void fill(const value_type& _val)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        std::fill(
            std::begin(m_data),
            std::end(m_data),
            _val);

        m_state.increment_version();
    }

    // assign (replace contents)
    template<typename _InputIter>
    void assign(_InputIter _first,
                _InputIter _last)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data.assign(_first, _last);
        sync_size();
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // assign (initializer list)
    void assign(
        std::initializer_list<value_type> _init)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data.assign(_init);
        sync_size();
    }

    // swap
    void swap(threadsafe_array& _other)
    {
        if (this == &_other)
        {
            return;
        }

        // lock both in address order to prevent
        // deadlock
        auto* first  = this;
        auto* second = &_other;

        if (first > second)
        {
            std::swap(first, second);
        }

        typename _Policy::write_lock_type g1(
            first->mutex());
        typename _Policy::write_lock_type g2(
            second->mutex());

        std::swap(m_data, _other.m_data);

        sync_size();
        _other.sync_size();
    }

#endif  // C++11

    // =========================================================
    // g. Ordering (write-locked)
    // =========================================================

    // sort
    void sort()
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        std::sort(
            std::begin(m_data),
            std::end(m_data));

        m_state.increment_version();
    }

    // sort (custom comparator)
    template<typename _Compare>
    void sort(_Compare _cmp)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        std::sort(
            std::begin(m_data),
            std::end(m_data),
            _cmp);

        m_state.increment_version();
    }

    // stable_sort
    void stable_sort()
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        std::stable_sort(
            std::begin(m_data),
            std::end(m_data));

        m_state.increment_version();
    }

    // sorted_insert
    //   inserts _val maintaining sorted invariant.
    template<typename C = _Container>
    auto sorted_insert(const value_type& _val)
        -> decltype(
            std::declval<C&>().push_back(_val),
            void())
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        auto pos = std::lower_bound(
            std::begin(m_data),
            std::end(m_data),
            _val);

        m_data.insert(pos, _val);
        m_state.fetch_add_size(1);
        m_state.increment_version();
    }

    // sorted_erase
    //   removes first occurrence of _val from sorted
    // data.  Returns true if found.
    bool sorted_erase(const value_type& _val)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        auto it = std::lower_bound(
            std::begin(m_data),
            std::end(m_data),
            _val);

        if (it != std::end(m_data) &&
            *it == _val)
        {
            m_data.erase(it);
            m_state.fetch_sub_size(1);
            m_state.increment_version();
            return true;
        }

        return false;
    }

    // is_sorted (read-locked)
    bool is_sorted() const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return std::is_sorted(
            std::begin(m_data),
            std::end(m_data));
    }

    // =========================================================
    // h. Multiplicity (write-locked)
    // =========================================================

    // deduplicate
    //   removes adjacent duplicates.
    // Returns count removed.
    size_type deduplicate()
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        size_type old_size = m_data.size();

        auto new_end = std::unique(
            std::begin(m_data),
            std::end(m_data));

        m_data.erase(new_end, std::end(m_data));

        size_type removed =
            old_size - m_data.size();

        if (removed > 0)
        {
            sync_size();
        }

        return removed;
    }

    // make_unique
    //   sort + deduplicate.
    // Returns count of duplicates removed.
    size_type make_unique()
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        std::sort(
            std::begin(m_data),
            std::end(m_data));

        size_type old_size = m_data.size();

        auto new_end = std::unique(
            std::begin(m_data),
            std::end(m_data));

        m_data.erase(new_end, std::end(m_data));

        size_type removed =
            old_size - m_data.size();

        sync_size();

        return removed;
    }

    // remove_all_of
    //   erases every occurrence of _val.
    // Returns count removed.
    size_type remove_all_of(const value_type& _val)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        size_type old_size = m_data.size();

        auto new_end = std::remove(
            std::begin(m_data),
            std::end(m_data),
            _val);

        m_data.erase(new_end, std::end(m_data));

        size_type removed =
            old_size - m_data.size();

        if (removed > 0)
        {
            sync_size();
        }

        return removed;
    }

    // insert_unique
    //   inserts _val only if not present (linear scan).
    // Returns true if inserted.
    bool insert_unique(const value_type& _val)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        auto it = std::find(
            std::begin(m_data),
            std::end(m_data),
            _val);

        if (it != std::end(m_data))
        {
            return false;
        }

        m_data.push_back(_val);
        m_state.fetch_add_size(1);
        m_state.increment_version();
        return true;
    }

    // sorted_insert_unique
    //   inserts into sorted data only if not present.
    // O(log n) search.  Returns true if inserted.
    bool sorted_insert_unique(
        const value_type& _val)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        auto pos = std::lower_bound(
            std::begin(m_data),
            std::end(m_data),
            _val);

        if (pos != std::end(m_data) &&
            *pos == _val)
        {
            return false;
        }

        m_data.insert(pos, _val);
        m_state.fetch_add_size(1);
        m_state.increment_version();
        return true;
    }

    // =========================================================
    // i. Search (read-locked)
    // =========================================================

    // contains
    //   linear search.  O(n).
    bool contains(const value_type& _val) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return (std::find(
            std::begin(m_data),
            std::end(m_data),
            _val) != std::end(m_data));
    }

    // sorted_contains
    //   binary search.  O(log n).
    bool sorted_contains(
        const value_type& _val) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        auto it = std::lower_bound(
            std::begin(m_data),
            std::end(m_data),
            _val);

        return (it != std::end(m_data) &&
                *it == _val);
    }

    // count_of
    //   O(n) count of _val occurrences.
    size_type count_of(
        const value_type& _val) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return static_cast<size_type>(
            std::count(
                std::begin(m_data),
                std::end(m_data),
                _val));
    }

    // lower_bound_index
    //   on sorted data.
    size_type lower_bound_index(
        const value_type& _val) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        auto it = std::lower_bound(
            std::begin(m_data),
            std::end(m_data),
            _val);

        return static_cast<size_type>(
            std::distance(
                std::begin(m_data), it));
    }

    // =========================================================
    // j. Filter (read-locked / write-locked)
    // =========================================================

    // filter_copy (read-locked)
    //   returns a new vector of matching elements.
    template<typename _Pred>
    std::vector<value_type>
    filter_copy(_Pred _pred) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        std::vector<value_type> result;

        result.reserve(m_data.size() / 2);

        for (auto it = std::begin(m_data);
             it != std::end(m_data); ++it)
        {
            if (_pred(*it))
            {
                result.push_back(*it);
            }
        }

        return result;
    }

    // filter_indices (read-locked)
    template<typename _Pred>
    std::vector<size_type>
    filter_indices(_Pred _pred) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        std::vector<size_type> result;
        size_type n = m_data.size();

        for (size_type i = 0; i < n; ++i)
        {
            if (_pred(m_data[i]))
            {
                result.push_back(i);
            }
        }

        return result;
    }

    // count_if (read-locked)
    template<typename _Pred>
    size_type count_if(_Pred _pred) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        size_type c = 0;

        for (auto it = std::begin(m_data);
             it != std::end(m_data); ++it)
        {
            if (_pred(*it))
            {
                ++c;
            }
        }

        return c;
    }

    // any_of (read-locked)
    template<typename _Pred>
    bool any_of(_Pred _pred) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        for (auto it = std::begin(m_data);
             it != std::end(m_data); ++it)
        {
            if (_pred(*it))
            {
                return true;
            }
        }

        return false;
    }

    // all_of (read-locked)
    template<typename _Pred>
    bool all_of(_Pred _pred) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        for (auto it = std::begin(m_data);
             it != std::end(m_data); ++it)
        {
            if (!_pred(*it))
            {
                return false;
            }
        }

        return true;
    }

    // none_of (read-locked)
    template<typename _Pred>
    bool none_of(_Pred _pred) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        for (auto it = std::begin(m_data);
             it != std::end(m_data); ++it)
        {
            if (_pred(*it))
            {
                return false;
            }
        }

        return true;
    }

    // filter_in_place (write-locked)
    //   removes elements NOT satisfying _pred.
    // Returns count removed.
    template<typename _Pred>
    size_type filter_in_place(_Pred _pred)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        size_type old_size = m_data.size();

        auto new_end = std::remove_if(
            std::begin(m_data),
            std::end(m_data),
            [&_pred](const value_type& _v)
            {
                return !_pred(_v);
            });

        m_data.erase(new_end, std::end(m_data));

        size_type removed =
            old_size - m_data.size();

        if (removed > 0)
        {
            sync_size();
        }

        return removed;
    }

    // partition (write-locked)
    //   returns partition point index.
    template<typename _Pred>
    size_type partition(_Pred _pred)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        auto it = std::partition(
            std::begin(m_data),
            std::end(m_data),
            _pred);

        m_state.increment_version();

        return static_cast<size_type>(
            std::distance(
                std::begin(m_data), it));
    }

    // =========================================================
    // k. Bulk / Slice (read-locked)
    // =========================================================

    // subarray_copy
    //   returns a copy of [offset, offset+count).
    std::vector<value_type>
    subarray_copy(size_type _offset,
                  size_type _count) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        size_type sz = m_data.size();
        size_type actual_offset =
            (_offset < sz) ? _offset : sz;
        size_type remaining =
            sz - actual_offset;
        size_type actual_count =
            (_count < remaining)
                ? _count : remaining;

        auto bg = std::begin(m_data);
        std::advance(bg, actual_offset);

        auto nd = bg;
        std::advance(nd, actual_count);

        return std::vector<value_type>(bg, nd);
    }

    // first_n_copy
    std::vector<value_type>
    first_n_copy(size_type _n) const
    {
        return subarray_copy(0, _n);
    }

    // last_n_copy
    std::vector<value_type>
    last_n_copy(size_type _n) const
    {
        size_type sz = size();
        size_type off = (_n < sz) ? (sz - _n) : 0;

        return subarray_copy(off, _n);
    }

    // to_vector
    //   returns a complete copy of the data.
    std::vector<value_type>
    to_vector() const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return std::vector<value_type>(
            std::begin(m_data),
            std::end(m_data));
    }

    // =========================================================
    // l. Optimistic Read (C++11+)
    // =========================================================
    // Version-checked reads without acquiring a lock.
    // Falls back to a real read lock if the version
    // changes during the read.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // optimistic_at
    //   reads element at _index without a lock.
    // Retries if version changes.
    value_type optimistic_at(size_type _index) const
    {
        return optimistic_read<
            _Container, _Policy>(
                m_state,
                m_data,
                base_type::mutex(),
                [_index](const _Container& _c)
                {
                    return _c[_index];
                });
    }

    // optimistic_size
    //   redundant with the lock-free size() for
    // simple containers, but useful as a pattern
    // for containers where size() accesses
    // non-atomic internal state.
    size_type optimistic_size() const
    {
        return optimistic_read<
            _Container, _Policy>(
                m_state,
                m_data,
                base_type::mutex(),
                [](const _Container& _c)
                {
                    return _c.size();
                });
    }

    // apply_read
    //   acquires a read lock and invokes _fn with
    // a const reference to the backing container.
    template<typename _Fn>
    auto apply_read(_Fn&& _fn) const
        -> decltype(_fn(
            std::declval<const _Container&>()))
    {
        return locked_apply_read<
            _Container, _Policy>(
                m_data,
                base_type::mutex(),
                std::forward<_Fn>(_fn));
    }

    // apply_write
    //   acquires a write lock and invokes _fn with
    // a mutable reference.  Updates atomic_state
    // after _fn completes.
    template<typename _Fn>
    auto apply_write(_Fn&& _fn)
        -> decltype(_fn(
            std::declval<_Container&>()))
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        auto result =
            std::forward<_Fn>(_fn)(m_data);

        sync_size();

        return result;
    }

    // apply_write (void return)
    template<typename _Fn>
    void apply_write_void(_Fn&& _fn)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        std::forward<_Fn>(_fn)(m_data);

        sync_size();
    }

#endif  // C++11

private:
    // sync_size
    //   updates the atomic_state from the backing
    // container's actual size.  Must be called under
    // a write lock after any operation that may change
    // the container size.
    void sync_size() noexcept
    {
        m_state.store_size(m_data.size());
        m_state.increment_version();
    }

    _Container   m_data;
    atomic_state m_state;
};


// =============================================================================
// II.  Convenience Aliases
// =============================================================================

// threadsafe_vector
//   alias: threadsafe_array backed by std::vector.
template<typename _Type,
         typename _Policy = default_lock_policy>
using threadsafe_vector =
    threadsafe_array<
        std::vector<_Type>, _Policy>;

// threadsafe_fixed_array
//   alias: threadsafe_array backed by std::array.
template<typename _Type,
         std::size_t _N,
         typename _Policy = default_lock_policy>
using threadsafe_fixed_array =
    threadsafe_array<
        std::array<_Type, _N>, _Policy>;


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_THREADSAFE_ARRAY_
