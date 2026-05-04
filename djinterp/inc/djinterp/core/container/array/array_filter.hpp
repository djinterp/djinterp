/******************************************************************************
* djinterp [container]                                      array_filter.hpp
*
* Array-specific filter operations for contiguous containers.
*   Bridges the generic functional filter infrastructure (filter.hpp,
* filterable_traits.hpp) with array-optimized implementations that
* exploit contiguous storage and random-access iteration.
*
*   Structured as three CRTP layers matching the Lifetime axis:
*
*   array_filter_constexpr_base<D>
*       Compile-time filter predicates over data():
*       constexpr_count_if, constexpr_any_of, constexpr_all_of,
*       constexpr_none_of, constexpr_find_if, constexpr_filter
*       (returns index array at compile time).
*
*   array_filter_immutable_base<D>
*       Runtime read-only filter operations producing new
*       containers or index vectors without modifying the
*       source:
*       filter_into, filter_indices, filter_copy,
*       any_of, all_of, none_of, find_if_index,
*       count_if, partition_point_index,
*       sorted_filter_range (O(log n) for sorted arrays),
*       filter_view adapter.
*
*   array_filter_mutable_base<D>
*       In-place mutating filter operations:
*       filter_in_place (erase-remove),
*       partition, stable_partition,
*       partition_into (split into two outputs),
*       sorted_filter_erase (O(log n) locate + shift).
*
*   All layers SFINAE-gate on iteration capability:
*     constexpr_iterable  — constexpr operations via data()
*     const_iterable      — read-only filter operations
*     iterable            — in-place filter/partition
*     non_iterable        — data()-only count/find
*
*   Free-function overloads provide strategy-dispatched
* filtering that selects the optimal path based on
* container_filter_strategy_v:
*     native       → delegate to .filter()
*     random_access → index-based with data()
*     forward_only  → iterator-based single-pass
*     external      → filter into std::vector
*
* DEPENDENCIES:
*   array_container.hpp            — array CRTP bases
*   container_filter_traits.hpp    — filter strategy detection
*   filter.hpp                     — filter_result, filter_chain
*   functional.hpp                 — is_predicate detection
*
* TABLE OF CONTENTS
* =================
* I.      array_filter_constexpr_base (CRTP)
* II.     array_filter_immutable_base (CRTP)
* III.    array_filter_mutable_base (CRTP)
* IV.     filter_view Adapter
* V.      Free-Function Strategy-Dispatched Filter
* VI.     Free-Function Sorted Filter
* VII.    Free-Function Partition Algorithms
*
*
* path:      \inc\container\array_filter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.28
******************************************************************************/

#ifndef DJINTERP_ARRAY_FILTER_
#define DJINTERP_ARRAY_FILTER_ 1

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>
#include "../../djinterp.hpp"
#include "array_container.hpp"
#include "array_iterator.hpp"
#include "array_container_traits.hpp"
#include "../meta/container_filter_traits.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   array_filter_constexpr_base (CRTP) — constexpr layer
// =============================================================================
// Compile-time filter predicates.  All methods operate via
// data() + size() and are fully constexpr.
//
// These do not require begin()/end() — non-iterable
// containers with data() compile cleanly.

template<typename _Derived>
class array_filter_constexpr_base
{
protected:
    constexpr array_filter_constexpr_base()  = default;
    ~array_filter_constexpr_base() = default;

private:
    constexpr const _Derived& self() const noexcept
    {
        return static_cast<
            const _Derived&>(*this);
    }

    using value_type =
        typename _Derived::value_type;

public:
    // --- constexpr predicate queries ---

    // constexpr_count_if
    //   counts elements satisfying _pred.
    template<typename _Pred>
    constexpr std::size_t
    constexpr_count_if(_Pred _pred) const noexcept
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();
        std::size_t       c = 0;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_pred(p[i]))
            {
                ++c;
            }
        }

        return c;
    }

    // constexpr_any_of
    //   true if any element satisfies _pred.
    template<typename _Pred>
    constexpr bool
    constexpr_any_of(_Pred _pred) const noexcept
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_pred(p[i]))
            {
                return true;
            }
        }

        return false;
    }

    // constexpr_all_of
    //   true if every element satisfies _pred.
    template<typename _Pred>
    constexpr bool
    constexpr_all_of(_Pred _pred) const noexcept
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (!_pred(p[i]))
            {
                return false;
            }
        }

        return true;
    }

    // constexpr_none_of
    //   true if no element satisfies _pred.
    template<typename _Pred>
    constexpr bool
    constexpr_none_of(_Pred _pred) const noexcept
    {
        return !constexpr_any_of(_pred);
    }

    // constexpr_find_if
    //   returns index of first element satisfying _pred,
    // or size() if not found.
    template<typename _Pred>
    constexpr std::size_t
    constexpr_find_if(_Pred _pred) const noexcept
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_pred(p[i]))
            {
                return i;
            }
        }

        return n;
    }

    // constexpr_find_if_not
    //   returns index of first element NOT satisfying
    // _pred, or size() if all satisfy.
    template<typename _Pred>
    constexpr std::size_t
    constexpr_find_if_not(_Pred _pred) const noexcept
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (!_pred(p[i]))
            {
                return i;
            }
        }

        return n;
    }

    // constexpr_is_partitioned
    //   true if all elements satisfying _pred precede
    // all elements not satisfying _pred.
    template<typename _Pred>
    constexpr bool
    constexpr_is_partitioned(
        _Pred _pred) const noexcept
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        // find first element not satisfying pred
        std::size_t i = 0;

        while (i < n && _pred(p[i]))
        {
            ++i;
        }

        // all remaining must also not satisfy pred
        while (i < n)
        {
            if (_pred(p[i]))
            {
                return false;
            }

            ++i;
        }

        return true;
    }
};


// =============================================================================
// II.  array_filter_immutable_base (CRTP) — runtime immutable
// =============================================================================
// Read-only filter operations that produce new containers or
// index vectors.  No modification of the source.
//
// Const-iterable support:
//   Methods requiring iteration are SFINAE-gated on
//   is_const_iterable_container_v or is_iterable_container_v.
//
// Non-iterable:
//   count_if, find_if_index via data() remain available.

template<typename _Derived>
class array_filter_immutable_base
    : public array_filter_constexpr_base<_Derived>
{
protected:
    array_filter_immutable_base()  = default;
    ~array_filter_immutable_base() = default;

private:
    const _Derived& self() const
    {
        return static_cast<
            const _Derived&>(*this);
    }

    using value_type =
        typename _Derived::value_type;

public:
    // --- runtime predicate queries (no iteration) ---

    // count_if
    //   runtime count of elements satisfying _pred.
    // Operates via data() — no iteration dependency.
    template<typename _Pred>
    std::size_t
    count_if(_Pred _pred) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();
        std::size_t       c = 0;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_pred(p[i]))
            {
                ++c;
            }
        }

        return c;
    }

    // find_if_index
    //   returns index of first match, or size().
    // Operates via data() — no iteration dependency.
    template<typename _Pred>
    std::size_t
    find_if_index(_Pred _pred) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_pred(p[i]))
            {
                return i;
            }
        }

        return n;
    }

    // find_if_not_index
    //   returns index of first non-match, or size().
    template<typename _Pred>
    std::size_t
    find_if_not_index(_Pred _pred) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (!_pred(p[i]))
            {
                return i;
            }
        }

        return n;
    }

    // --- runtime predicate queries (iterable) ---

    // any_of
    template<typename _Pred,
             typename D = _Derived>
    auto
    any_of(_Pred _pred) const
        -> typename std::enable_if<
               traits::is_const_iterable_container_v<
                   D>                              ||
               traits::is_iterable_container_v<D>,
               bool>::type
    {
        return std::any_of(
            std::begin(self()),
            std::end(self()),
            _pred);
    }

    // all_of
    template<typename _Pred,
             typename D = _Derived>
    auto
    all_of(_Pred _pred) const
        -> typename std::enable_if<
               traits::is_const_iterable_container_v<
                   D>                              ||
               traits::is_iterable_container_v<D>,
               bool>::type
    {
        return std::all_of(
            std::begin(self()),
            std::end(self()),
            _pred);
    }

    // none_of
    template<typename _Pred,
             typename D = _Derived>
    auto
    none_of(_Pred _pred) const
        -> typename std::enable_if<
               traits::is_const_iterable_container_v<
                   D>                              ||
               traits::is_iterable_container_v<D>,
               bool>::type
    {
        return std::none_of(
            std::begin(self()),
            std::end(self()),
            _pred);
    }

    // --- filter into output ---

    // filter_indices
    //   returns a vector of indices of elements
    // satisfying _pred.  Exploits contiguous data()
    // access — no iterator dependency.
    template<typename _Pred>
    std::vector<std::size_t>
    filter_indices(_Pred _pred) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        std::vector<std::size_t> result;

        result.reserve(n / 2);

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_pred(p[i]))
            {
                result.push_(i);
            }
        }

        return result;
    }

    // filter_copy
    //   returns a new vector containing only elements
    // satisfying _pred.  Uses data() pointer — no
    // iteration dependency.
    template<typename _Pred>
    std::vector<value_type>
    filter_copy(_Pred _pred) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        std::vector<value_type> result;

        result.reserve(n / 2);

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_pred(p[i]))
            {
                result.push_(p[i]);
            }
        }

        return result;
    }

    // filter_into
    //   pushes matching elements into _out.
    // _out must support push_().
    template<typename _Output,
             typename _Pred>
    std::size_t
    filter_into(_Output& _out,
                _Pred    _pred) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();
        std::size_t       c = 0;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_pred(p[i]))
            {
                _out.push_(p[i]);
                ++c;
            }
        }

        return c;
    }

    // partition_copy_into
    //   splits elements into _true_out and _false_out.
    // Returns (true_count, false_count).
    template<typename _TrueOut,
             typename _FalseOut,
             typename _Pred>
    std::pair<std::size_t, std::size_t>
    partition_copy_into(
        _TrueOut&  _true_out,
        _FalseOut& _false_out,
        _Pred      _pred) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();
        std::size_t       tc = 0;
        std::size_t       fc = 0;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_pred(p[i]))
            {
                _true_out.push_(p[i]);
                ++tc;
            }
            else
            {
                _false_out.push_(p[i]);
                ++fc;
            }
        }

        return { tc, fc };
    }

    // --- sorted filter ---

    // sorted_filter_range
    //   for sorted arrays: returns a subarray view
    // of elements in [_lo, _hi) using binary search.
    // O(log n) to locate, O(1) to return the view.
    chunk_ref<value_type>
    sorted_filter_range(
        const value_type& _lo,
        const value_type& _hi) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        // lower_bound for _lo
        std::size_t lo_idx = 0;
        std::size_t hi_idx = n;

        {
            std::size_t a = 0;
            std::size_t b = n;

            while (a < b)
            {
                std::size_t mid = a + (b - a) / 2;

                if (p[mid] < _lo)
                {
                    a = mid + 1;
                }
                else
                {
                    b = mid;
                }
            }

            lo_idx = a;
        }

        // lower_bound for _hi (upper end)
        {
            std::size_t a = lo_idx;
            std::size_t b = n;

            while (a < b)
            {
                std::size_t mid = a + (b - a) / 2;

                if (p[mid] < _hi)
                {
                    a = mid + 1;
                }
                else
                {
                    b = mid;
                }
            }

            hi_idx = a;
        }

        return {
            p + lo_idx,
            (hi_idx > lo_idx)
                ? (hi_idx - lo_idx) : 0
        };
    }

    // partition_point_index
    //   returns index of the first element for which
    // _pred returns false, assuming the array is
    // partitioned w.r.t. _pred.
    // O(log n) via binary search on data().
    template<typename _Pred>
    std::size_t
    partition_point_index(_Pred _pred) const
    {
        const value_type* p = self().data();
        std::size_t lo = 0;
        std::size_t hi = self().size();

        while (lo < hi)
        {
            std::size_t mid = lo + (hi - lo) / 2;

            if (_pred(p[mid]))
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
};


// =============================================================================
// III. array_filter_mutable_base (CRTP) — mutable layer
// =============================================================================
// In-place mutating filter operations.
//
// Iterable support:
//   filter_in_place, partition, stable_partition
//   are SFINAE-gated on is_iterable_container_v.
//
// Non-iterable mutable:
//   filter_compact uses data() pointer for trivially
//   copyable elements.

template<typename _Derived>
class array_filter_mutable_base
    : public array_filter_immutable_base<_Derived>
{
protected:
    array_filter_mutable_base()  = default;
    ~array_filter_mutable_base() = default;

private:
    _Derived& self()
    {
        return static_cast<_Derived&>(*this);
    }

    const _Derived& self() const
    {
        return static_cast<
            const _Derived&>(*this);
    }

    using value_type =
        typename _Derived::value_type;

public:
    // --- in-place filter ---

    // filter_in_place
    //   removes all elements NOT satisfying _pred.
    // Uses erase-remove pattern.
    // Returns count of elements removed.
    template<typename _Pred,
             typename D = _Derived>
    auto
    filter_in_place(_Pred _pred)
        -> typename std::enable_if<
               traits::is_iterable_container_v<D> &&
               traits::has_erase_v<D>,
               std::size_t>::type
    {
        std::size_t old_size = self().size();

        auto new_end = std::remove_if(
            std::begin(self()),
            std::end(self()),
            [&_pred](const value_type& _v)
            {
                return !_pred(_v);
            });

        self().erase(new_end, std::end(self()));

        return old_size - self().size();
    }

    // filter_compact
    //   for trivially copyable elements: compact
    // matching elements to the front of data() via
    // direct pointer manipulation.  Does NOT resize
    // the container — returns the new logical size.
    // Caller must resize/erase the tail.
    template<typename _Pred,
             typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::size_t
    >::type
    filter_compact(_Pred _pred)
    {
        value_type* p  = self().data();
        std::size_t n  = self().size();
        std::size_t wr = 0;

        for (std::size_t rd = 0; rd < n; ++rd)
        {
            if (_pred(p[rd]))
            {
                if (wr != rd)
                {
                    p[wr] = p[rd];
                }

                ++wr;
            }
        }

        return wr;
    }

    // --- partition ---

    // partition
    //   reorders elements so that all satisfying _pred
    // come before those that do not.  Relative order
    // within each group is NOT preserved.
    // Returns index of partition point.
    template<typename _Pred,
             typename D = _Derived>
    auto
    partition(_Pred _pred)
        -> typename std::enable_if<
               traits::is_iterable_container_v<D>,
               std::size_t>::type
    {
        auto it = std::partition(
            std::begin(self()),
            std::end(self()),
            _pred);

        return static_cast<std::size_t>(
            std::distance(
                std::begin(self()), it));
    }

    // stable_partition
    //   reorders elements so that all satisfying _pred
    // come before those that do not.  Relative order
    // within each group IS preserved.
    // Returns index of partition point.
    template<typename _Pred,
             typename D = _Derived>
    auto
    stable_partition(_Pred _pred)
        -> typename std::enable_if<
               traits::is_iterable_container_v<D>,
               std::size_t>::type
    {
        auto it = std::stable_partition(
            std::begin(self()),
            std::end(self()),
            _pred);

        return static_cast<std::size_t>(
            std::distance(
                std::begin(self()), it));
    }

    // partition_into
    //   splits elements in-place: moves matching
    // elements to _true_out, non-matching to
    // _false_out, then clears this container.
    // Returns (true_count, false_count).
    template<typename _TrueOut,
             typename _FalseOut,
             typename _Pred,
             typename D = _Derived>
    auto
    partition_into(
        _TrueOut&  _true_out,
        _FalseOut& _false_out,
        _Pred      _pred)
        -> typename std::enable_if<
               traits::is_iterable_container_v<D> &&
               traits::has_clear_v<D>,
               std::pair<std::size_t,
                         std::size_t>>::type
    {
        std::size_t tc = 0;
        std::size_t fc = 0;

        for (auto it = std::begin(self());
             it != std::end(self()); ++it)
        {
            if (_pred(*it))
            {
                _true_out.push_(
                    std::move(*it));
                ++tc;
            }
            else
            {
                _false_out.push_(
                    std::move(*it));
                ++fc;
            }
        }

        self().clear();

        return { tc, fc };
    }

    // --- sorted in-place filter ---

    // sorted_filter_erase
    //   for sorted arrays: removes all elements in
    // the range [_lo, _hi) using O(log n) binary
    // search + range erase.
    // Returns count of elements removed.
    template<typename D = _Derived>
    auto
    sorted_filter_erase(
        const value_type& _lo,
        const value_type& _hi)
        -> typename std::enable_if<
               traits::is_iterable_container_v<D> &&
               traits::has_erase_v<D>,
               std::size_t>::type
    {
        auto first = std::lower_bound(
            std::begin(self()),
            std::end(self()),
            _lo);

        auto last = std::lower_bound(
            first,
            std::end(self()),
            _hi);

        std::size_t count =
            static_cast<std::size_t>(
                std::distance(first, last));

        if (count > 0)
        {
            self().erase(first, last);
        }

        return count;
    }

    // sorted_filter_keep
    //   for sorted arrays: keeps ONLY elements in
    // the range [_lo, _hi), erasing everything
    // outside.
    // Returns count of elements removed.
    template<typename D = _Derived>
    auto
    sorted_filter_keep(
        const value_type& _lo,
        const value_type& _hi)
        -> typename std::enable_if<
               traits::is_iterable_container_v<D> &&
               traits::has_erase_v<D>,
               std::size_t>::type
    {
        std::size_t old_size = self().size();

        auto first = std::lower_bound(
            std::begin(self()),
            std::end(self()),
            _lo);

        auto last = std::lower_bound(
            first,
            std::end(self()),
            _hi);

        // erase tail first (indices stable)
        if (last != std::end(self()))
        {
            self().erase(last, std::end(self()));
        }

        // erase head
        if (first != std::begin(self()))
        {
            self().erase(
                std::begin(self()), first);
        }

        return old_size - self().size();
    }
};


// =============================================================================
// IV.  filter_view Adapter
// =============================================================================
// Non-owning lazy view that filters an array on iteration.
// Stores a reference and a predicate; yields only matching
// elements via a custom forward iterator.

template<typename _Container,
         typename _Pred>
class array_filter_view
{
public:
    using value_type =
        typename _Container::value_type;
    using size_type = std::size_t;

    // --- filter_view_iterator ---
    class const_iterator
    {
    public:
        using value_type =
            typename _Container::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer         = const value_type*;
        using reference       = const value_type&;
        using iterator_category =
            std::forward_iterator_tag;

        const_iterator() noexcept
            : m_data(nullptr)
            , m_size(0)
            , m_pos(0)
            , m_pred(nullptr)
        {}

        const_iterator(
            const value_type* _data,
            std::size_t       _size,
            std::size_t       _pos,
            const _Pred*      _pred) noexcept
            : m_data(_data)
            , m_size(_size)
            , m_pos(_pos)
            , m_pred(_pred)
        {
            advance_to_match();
        }

        reference
        operator*() const noexcept
        {
            return m_data[m_pos];
        }

        pointer
        operator->() const noexcept
        {
            return &m_data[m_pos];
        }

        const_iterator&
        operator++() noexcept
        {
            ++m_pos;
            advance_to_match();
            return *this;
        }

        const_iterator
        operator++(int) noexcept
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool
        operator==(const_iterator _a,
                   const_iterator _b) noexcept
        {
            return (_a.m_pos == _b.m_pos);
        }

        friend bool
        operator!=(const_iterator _a,
                   const_iterator _b) noexcept
        {
            return (_a.m_pos != _b.m_pos);
        }

    private:
        void advance_to_match() noexcept
        {
            while (m_pos < m_size &&
                   !(*m_pred)(m_data[m_pos]))
            {
                ++m_pos;
            }
        }

        const value_type* m_data;
        std::size_t       m_size;
        std::size_t       m_pos;
        const _Pred*      m_pred;
    };

    // --- construction ---

    array_filter_view(
        const _Container& _c,
        _Pred             _pred) noexcept
        : m_ref(_c)
        , m_pred(std::move(_pred))
    {}

    const_iterator
    begin() const noexcept
    {
        return const_iterator(
            m_ref.data(),
            m_ref.size(),
            0,
            &m_pred);
    }

    const_iterator
    end() const noexcept
    {
        return const_iterator(
            m_ref.data(),
            m_ref.size(),
            m_ref.size(),
            &m_pred);
    }

    // count
    //   O(n) — counts matching elements.
    std::size_t
    count() const
    {
        const auto* p = m_ref.data();
        std::size_t n = m_ref.size();
        std::size_t c = 0;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (m_pred(p[i]))
            {
                ++c;
            }
        }

        return c;
    }

    bool
    empty() const
    {
        const auto* p = m_ref.data();
        std::size_t n = m_ref.size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (m_pred(p[i]))
            {
                return false;
            }
        }

        return true;
    }

private:
    const _Container& m_ref;
    _Pred             m_pred;
};

// make_array_filter_view
template<typename _Container,
         typename _Pred>
array_filter_view<_Container, _Pred>
make_array_filter_view(
    const _Container& _c,
    _Pred             _pred) noexcept
{
    return array_filter_view<_Container, _Pred>(
        _c, std::move(_pred));
}


// =============================================================================
// V.   Free-Function Strategy-Dispatched Filter
// =============================================================================
// Selects the optimal filter path based on
// container_filter_strategy_v.

// array_filter
//   filters a contiguous array by predicate, returning
// a new vector.  Optimized for random-access via data()
// pointer.
template<typename _Container,
         typename _Pred>
inline typename std::enable_if<
    traits::is_contiguous_array_v<_Container>,
    std::vector<typename _Container::value_type>
>::type
array_filter(const _Container& _src,
             _Pred             _pred)
{
    using V = typename _Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();

    std::vector<V> result;

    result.reserve(n / 2);

    for (std::size_t i = 0; i < n; ++i)
    {
        if (_pred(p[i]))
        {
            result.push_(p[i]);
        }
    }

    return result;
}

// array_filter_into
//   filters _src into _dst (any container with
// push_).  Returns count of elements added.
template<typename _Container,
         typename _Output,
         typename _Pred>
inline typename std::enable_if<
    traits::is_contiguous_array_v<_Container>,
    std::size_t
>::type
array_filter_into(const _Container& _src,
                  _Output&          _dst,
                  _Pred             _pred)
{
    using V = typename _Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();
    std::size_t c = 0;

    for (std::size_t i = 0; i < n; ++i)
    {
        if (_pred(p[i]))
        {
            _dst.push_(p[i]);
            ++c;
        }
    }

    return c;
}

// array_filter_indices
//   returns indices of matching elements.
template<typename _Container,
         typename _Pred>
inline typename std::enable_if<
    traits::is_contiguous_array_v<_Container>,
    std::vector<std::size_t>
>::type
array_filter_indices(const _Container& _src,
                     _Pred             _pred)
{
    using V = typename _Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();

    std::vector<std::size_t> result;

    result.reserve(n / 2);

    for (std::size_t i = 0; i < n; ++i)
    {
        if (_pred(p[i]))
        {
            result.push_(i);
        }
    }

    return result;
}


// =============================================================================
// VI.  Free-Function Sorted Filter
// =============================================================================
// For sorted contiguous arrays: exploits ordering for
// O(log n) range location.

// array_sorted_filter_range
//   returns a non-owning view of elements in [_lo, _hi)
// from a sorted array.  O(log n).
template<typename _Container>
inline typename std::enable_if<
    traits::is_contiguous_array_v<_Container>,
    chunk_ref<typename _Container::value_type>
>::type
array_sorted_filter_range(
    const _Container&                          _src,
    const typename _Container::value_type& _lo,
    const typename _Container::value_type& _hi)
{
    using V = typename _Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();

    auto lo_it = std::lower_bound(p, p + n, _lo);
    auto hi_it = std::lower_bound(lo_it, p + n, _hi);

    std::size_t lo_idx =
        static_cast<std::size_t>(lo_it - p);
    std::size_t hi_idx =
        static_cast<std::size_t>(hi_it - p);

    return {
        p + lo_idx,
        (hi_idx > lo_idx)
            ? (hi_idx - lo_idx) : 0
    };
}

// array_sorted_filter_copy
//   copies elements in [_lo, _hi) from a sorted array
// into a new vector.  O(log n + k) where k is result
// size.
template<typename _Container>
inline typename std::enable_if<
    traits::is_contiguous_array_v<_Container>,
    std::vector<typename _Container::value_type>
>::type
array_sorted_filter_copy(
    const _Container&                          _src,
    const typename _Container::value_type& _lo,
    const typename _Container::value_type& _hi)
{
    auto ref = array_sorted_filter_range(
        _src, _lo, _hi);

    return std::vector<
        typename _Container::value_type>(
            ref.begin(), ref.end());
}


// =============================================================================
// VII. Free-Function Partition Algorithms
// =============================================================================

// array_partition_copy
//   splits a contiguous array into two outputs by
// predicate.  Returns (true_count, false_count).
template<typename _Container,
         typename _TrueOut,
         typename _FalseOut,
         typename _Pred>
inline typename std::enable_if<
    traits::is_contiguous_array_v<_Container>,
    std::pair<std::size_t, std::size_t>
>::type
array_partition_copy(
    const _Container& _src,
    _TrueOut&         _true_out,
    _FalseOut&        _false_out,
    _Pred             _pred)
{
    using V = typename _Container::value_type;

    const V*    p  = _src.data();
    std::size_t n  = _src.size();
    std::size_t tc = 0;
    std::size_t fc = 0;

    for (std::size_t i = 0; i < n; ++i)
    {
        if (_pred(p[i]))
        {
            _true_out.push_(p[i]);
            ++tc;
        }
        else
        {
            _false_out.push_(p[i]);
            ++fc;
        }
    }

    return { tc, fc };
}

// array_is_partitioned
//   true if all elements satisfying _pred precede
// all elements not satisfying _pred.
template<typename _Container,
         typename _Pred>
inline typename std::enable_if<
    traits::is_contiguous_array_v<_Container>,
    bool
>::type
array_is_partitioned(
    const _Container& _src,
    _Pred             _pred)
{
    return std::is_partitioned(
        std::begin(_src),
        std::end(_src),
        _pred);
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_ARRAY_FILTER_
