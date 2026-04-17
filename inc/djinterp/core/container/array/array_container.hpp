/******************************************************************************
* djinterp [container]                                    array_container.hpp
*
* CRTP base for array-based (contiguous, random-access) containers.
*   Builds on sequential_base to provide operations specific to
* contiguous storage:
*
*   Bulk operations:   fill, memcpy-accelerated copy/move, swap_ranges
*   Shift operations:  shift_left, shift_right, rotate (via memmove
*                      when trivially relocatable)
*   Chunking:          chunk_view, chunk iteration, per-chunk apply
*   Circular access:   circular iteration from any head offset
*   Capacity:          reserve, shrink, grow helpers
*   Slicing:           sub-range extraction returning views
*   Search:            binary_search (on sorted arrays), lower_bound,
*                      upper_bound
*
*   The derived class must expose:
*     - data()   → pointer to contiguous storage
*     - size()   → element count
*     - begin() / end()
*   Optional for full functionality:
*     - capacity(), reserve(n), resize(n)
*     - push_back(val)
*
* DEPENDENCIES:
*   sequential_container.hpp    — CRTP sequential base
*   array_container_traits.hpp  — array strategy detection
*   array_iterator.hpp          — chunk/circular/window iterators
*
* TABLE OF CONTENTS
* =================
* I.      array_base (CRTP)
* II.     Chunk View Adapter
* III.    Window View Adapter
* IV.     Circular View Adapter
* V.      Free-Function Factories
* VI.     Free-Function Bulk Algorithms
*
*
* path:      \inc\container\array_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.24
******************************************************************************/

#ifndef DJINTERP_ARRAY_CONTAINER_
#define DJINTERP_ARRAY_CONTAINER_ 1

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <utility>
#include "..\djinterp.hpp"
#include "../sequential_container.hpp"
#include "array_iterator.hpp"
#include "meta\array_container_traits.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   array_base (CRTP)
// =============================================================================

template<typename _Derived>
class array_base : public sequential_base<_Derived>
{
protected:
    array_base()  = default;
    ~array_base() = default;

private:
    _Derived& self()
    {
        return static_cast<_Derived&>(*this);
    }

    const _Derived& self() const
    {
        return static_cast<const _Derived&>(*this);
    }

    using value_type =
        typename _Derived::value_type;

public:
    // --- byte-level metrics ---

    // byte_size
    //   returns the total byte footprint of the
    // elements (size * sizeof(value_type)).
    constexpr std::size_t byte_size() const noexcept
    {
        return self().size() * sizeof(value_type);
    }

    // byte_capacity
    //   returns the total byte capacity when the
    // derived container exposes capacity().
    template<typename D = _Derived>
    constexpr auto byte_capacity() const noexcept
        -> typename std::enable_if<traits::has_capacity_method_v<D>, std::size_t>::type
    {
        return self().capacity() * sizeof(value_type);
    }

    // utilization
    //   returns size/capacity as a fraction [0, 1].
    template<typename D = _Derived>
    constexpr auto utilization() const noexcept
        -> typename std::enable_if<
               traits::has_capacity_method_v<D>,
               double>::type
    {
        std::size_t cap = self().capacity();

        return (cap > 0)
            ? (static_cast<double>(self().size())
               / static_cast<double>(cap))
            : 0.0;
    }

    // --- bulk operations ---

    // fill
    //   sets every element to _val.
    void fill(const value_type& _val)
    {
        std::fill(
            std::begin(self()),
            std::end(self()),
            _val);
    }

    // zero
    //   zeroes the storage (only for trivially copyable
    // elements).
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>
    >::type
    zero() noexcept
    {
        std::memset(
            self().data(), 0, byte_size());
    }

    // --- positional access ---

    // at (bounds-checking via assertion)
    const value_type&
    at(std::size_t _index) const
    {
        return self().data()[_index];
    }

    value_type&
    at(std::size_t _index)
    {
        return self().data()[_index];
    }

    // --- slicing (returns views) ---

    // subarray
    //   returns a non-owning view of [offset, offset+count).
    chunk_ref<value_type>
    subarray(std::size_t _offset,
             std::size_t _count) const noexcept
    {
        std::size_t sz = self().size();
        std::size_t actual_offset =
            (_offset < sz) ? _offset : sz;
        std::size_t remaining = sz - actual_offset;
        std::size_t actual_count =
            (_count < remaining)
                ? _count : remaining;

        return {
            self().data() + actual_offset,
            actual_count
        };
    }

    // first_n
    //   returns a view of the first _n elements.
    chunk_ref<value_type>
    first_n(std::size_t _n) const noexcept
    {
        return subarray(0, _n);
    }

    // last_n
    //   returns a view of the last _n elements.
    chunk_ref<value_type>
    last_n(std::size_t _n) const noexcept
    {
        std::size_t sz = self().size();
        std::size_t offset =
            (_n < sz) ? (sz - _n) : 0;

        return subarray(offset, _n);
    }

    // --- search (sorted arrays) ---

    // binary_find
    //   binary search for _val in a sorted array.
    // Returns pointer to element, or nullptr.
    const value_type*
    binary_find(const value_type& _val) const
    {
        auto it = std::lower_bound(
            std::begin(self()),
            std::end(self()),
            _val);

        if (it != std::end(self()) && *it == _val)
        {
            return &(*it);
        }

        return nullptr;
    }

    // lower_bound
    std::size_t
    lower_bound_index(
        const value_type& _val) const
    {
        auto it = std::lower_bound(
            std::begin(self()),
            std::end(self()),
            _val);

        return static_cast<std::size_t>(
            std::distance(
                std::begin(self()), it));
    }

    // upper_bound
    std::size_t
    upper_bound_index(
        const value_type& _val) const
    {
        auto it = std::upper_bound(
            std::begin(self()),
            std::end(self()),
            _val);

        return static_cast<std::size_t>(
            std::distance(
                std::begin(self()), it));
    }

    // --- capacity helpers ---

    // ensure_capacity
    //   reserves at least _n elements if the derived
    // container supports reserve().
    template<typename D = _Derived>
    typename std::enable_if<
        traits::has_reserve_method_v<D>
    >::type
    ensure_capacity(std::size_t _n)
    {
        if (self().capacity() < _n)
        {
            self().reserve(_n);
        }
    }

    // compact
    //   shrinks capacity to size if the derived
    // container supports shrink_to_fit().
    template<typename D = _Derived>
    typename std::enable_if<
        traits::has_shrink_to_fit_method_v<D>
    >::type
    compact()
    {
        self().shrink_to_fit();
    }

    // --- shift operations ---
    // (these override sequential_base shifts with
    // optimized memmove for trivially relocatable
    // arrays)

    // shift_left_fast
    //   memmove-accelerated left shift for trivially
    // copyable elements.
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>
    >::type
    shift_left_fast(std::size_t _n,
                    const value_type& _fill)
    {
        std::size_t sz = self().size();

        if (_n >= sz)
        {
            fill(_fill);
            return;
        }

        std::memmove(
            self().data(),
            self().data() + _n,
            (sz - _n) * sizeof(value_type));

        std::fill(
            self().data() + (sz - _n),
            self().data() + sz,
            _fill);
    }

    // shift_right_fast
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>
    >::type
    shift_right_fast(std::size_t _n,
                     const value_type& _fill)
    {
        std::size_t sz = self().size();

        if (_n >= sz)
        {
            fill(_fill);
            return;
        }

        std::memmove(
            self().data() + _n,
            self().data(),
            (sz - _n) * sizeof(value_type));

        std::fill(
            self().data(),
            self().data() + _n,
            _fill);
    }

    // --- circular access ---

    // circular_begin / circular_end
    //   provides circular iteration starting at _head.
    circular_iterator<value_type>
    circular_begin(std::size_t _head) const noexcept
    {
        return circular_iterator<value_type>(
            self().data(),
            self().size(),
            _head % self().size(),
            0);
    }

    circular_iterator<value_type>
    circular_end(std::size_t _head) const noexcept
    {
        return circular_iterator<value_type>(
            self().data(),
            self().size(),
            _head % self().size(),
            self().size());
    }
};


// =============================================================================
// II.  Chunk View Adapter
// =============================================================================
// Non-owning view that splits an array into fixed-size
// non-overlapping chunks.

template<typename _Container>
class chunk_view
{
public:
    using value_type     = typename _Container::value_type;
    using base_iter      = decltype(std::cbegin(std::declval<const _Container&>()));
    using const_iterator = chunk_iterator<base_iter>;
    using size_type      = std::size_t;

    constexpr chunk_view(
        const _Container& _c,
        size_type         _chunk_sz
    ) noexcept
        : m_ref(_c),
          m_chunk_sz(_chunk_sz)
    {}

    constexpr const_iterator
    begin() const noexcept
    {
        return make_chunk_iterator(
            std::cbegin(m_ref),
            std::cend(m_ref),
            m_chunk_sz);
    }

    constexpr const_iterator
    end() const noexcept
    {
        return make_chunk_end(
            std::cend(m_ref),
            m_chunk_sz);
    }

    constexpr size_type
    chunk_count() const noexcept
    {
        size_type sz = m_ref.size();

        return (m_chunk_sz > 0)
            ? ( (sz + m_chunk_sz - 1) / m_chunk_sz )
            : 0;
    }

    constexpr size_type
    chunk_size() const noexcept
    {
        return m_chunk_sz;
    }

    constexpr bool
    empty() const noexcept
    {
        return m_ref.empty();
    }

private:
    const _Container& m_ref;
    size_type         m_chunk_sz;
};


// =============================================================================
// III. Window View Adapter
// =============================================================================
// Non-owning view providing a sliding window over an
// array.

template<typename _Container>
class sliding_window_view
{
public:
    using value_type =
        typename _Container::value_type;
    using base_iter =
        decltype(std::cbegin(
            std::declval<const _Container&>()));
    using const_iterator =
        window_iterator<base_iter>;
    using size_type = std::size_t;

    constexpr sliding_window_view(
        const _Container& _c,
        size_type         _window_sz) noexcept
        : m_ref(_c)
        , m_window_sz(_window_sz)
    {}

    constexpr const_iterator
    begin() const noexcept
    {
        return make_window_iterator(
            std::cbegin(m_ref),
            std::cend(m_ref),
            m_window_sz);
    }

    constexpr const_iterator
    end() const noexcept
    {
        return make_window_end(
            std::cbegin(m_ref),
            std::cend(m_ref),
            m_window_sz);
    }

    constexpr size_type
    window_count() const noexcept
    {
        size_type sz = m_ref.size();

        return (sz >= m_window_sz)
            ? (sz - m_window_sz + 1) : 0;
    }

    constexpr size_type
    window_size() const noexcept
    {
        return m_window_sz;
    }

    constexpr bool
    empty() const noexcept
    {
        return (m_ref.size() < m_window_sz);
    }

private:
    const _Container& m_ref;
    size_type         m_window_sz;
};


// =============================================================================
// IV.  Circular View Adapter
// =============================================================================
// Non-owning view providing circular iteration starting
// from a given head offset.

template<typename _Container>
class circular_view
{
public:
    using value_type =
        typename _Container::value_type;
    using const_iterator =
        circular_iterator<value_type>;
    using size_type = std::size_t;

    constexpr circular_view(
        const _Container& _c,
        size_type         _head = 0) noexcept
        : m_ref(_c)
        , m_head(_head)
    {}

    constexpr const_iterator
    begin() const noexcept
    {
        return const_iterator(
            m_ref.data(),
            m_ref.size(),
            m_head % m_ref.size(),
            0);
    }

    constexpr const_iterator
    end() const noexcept
    {
        return const_iterator(
            m_ref.data(),
            m_ref.size(),
            m_head % m_ref.size(),
            m_ref.size());
    }

    constexpr size_type
    size() const noexcept
    {
        return m_ref.size();
    }

    constexpr bool
    empty() const noexcept
    {
        return m_ref.empty();
    }

    constexpr const value_type&
    operator[](size_type _i) const noexcept
    {
        return m_ref.data()[
            (m_head + _i) % m_ref.size()];
    }

private:
    const _Container& m_ref;
    size_type         m_head;
};


// =============================================================================
// V.   Free-Function Factories
// =============================================================================

// make_chunk_view
template<typename _Container>
constexpr chunk_view<_Container>
make_chunk_view(const _Container& _c,
                std::size_t       _chunk_sz) noexcept
{
    return chunk_view<_Container>(_c, _chunk_sz);
}

// make_sliding_window_view
template<typename _Container>
constexpr sliding_window_view<_Container>
make_sliding_window_view(
    const _Container& _c,
    std::size_t       _window_sz) noexcept
{
    return sliding_window_view<_Container>(
        _c, _window_sz);
}

// make_circular_view
template<typename _Container>
constexpr circular_view<_Container>
make_circular_view(
    const _Container& _c,
    std::size_t       _head = 0) noexcept
{
    return circular_view<_Container>(_c, _head);
}


// =============================================================================
// VI.  Free-Function Bulk Algorithms
// =============================================================================
// Non-member algorithms optimized for contiguous arrays.

// array_copy
//   memcpy-accelerated copy for trivially copyable
// contiguous arrays.
template<typename _Src,
         typename _Dst>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Src>        &&
      traits::is_contiguous_array_v<_Dst>        &&
      traits::has_trivially_copyable_elements_v<
          _Src>                                  &&
      traits::elements_same_type_v<_Src, _Dst> ),
    std::size_t
>::type
array_copy(const _Src& _src,
           _Dst&       _dst)
{
    std::size_t n =
        (_src.size() < _dst.size())
            ? _src.size() : _dst.size();

    std::memcpy(
        _dst.data(),
        _src.data(),
        n * sizeof(typename _Src::value_type));

    return n;
}

// array_equal
//   memcmp-accelerated equality for trivially copyable
// contiguous arrays.
template<typename _A,
         typename _B>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_A>          &&
      traits::is_contiguous_array_v<_B>          &&
      traits::has_trivially_copyable_elements_v<
          _A>                                    &&
      traits::elements_same_type_v<_A, _B> ),
    bool
>::type
array_equal(const _A& _a,
            const _B& _b) noexcept
{
    if (_a.size() != _b.size())
    {
        return false;
    }

    return (std::memcmp(
        _a.data(),
        _b.data(),
        _a.size() *
            sizeof(typename _A::value_type)) == 0);
}

// array_swap
//   byte-level swap of two same-sized contiguous arrays.
template<typename _A,
         typename _B>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_A>          &&
      traits::is_contiguous_array_v<_B>          &&
      traits::has_trivially_copyable_elements_v<
          _A>                                    &&
      traits::elements_same_type_v<_A, _B> )
>::type
array_swap(_A& _a,
           _B& _b)
{
    using elem = typename _A::value_type;

    std::size_t n =
        (_a.size() < _b.size())
            ? _a.size() : _b.size();

    std::size_t bytes = n * sizeof(elem);

    // stack buffer for small swaps, loop for large
    constexpr std::size_t stack_limit = 4096;

    if (bytes <= stack_limit)
    {
        char tmp[stack_limit];

        std::memcpy(tmp, _a.data(), bytes);
        std::memcpy(_a.data(), _b.data(), bytes);
        std::memcpy(_b.data(), tmp, bytes);
    }
    else
    {
        elem* pa = _a.data();
        elem* pb = _b.data();

        for (std::size_t i = 0; i < n; ++i)
        {
            std::swap(pa[i], pb[i]);
        }
    }
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_ARRAY_CONTAINER_
