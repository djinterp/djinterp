/******************************************************************************
* djinterp [container]                                    array_iterator.hpp
*
* Array-specific iterator adapters.
*   Thin zero-overhead wrappers for patterns unique to contiguous
* array storage:
*
*   circular_iterator<T>   — wraps around a fixed-capacity buffer,
*                            enabling ring-buffer iteration from any
*                            head position.
*   chunk_iterator<Iter>   — groups a flat sequence into fixed-size
*                            sub-ranges (chunks), each exposed as a
*                            (data, size) pair.
*   window_iterator<Iter>  — slides a fixed-width window across the
*                            sequence, yielding overlapping sub-ranges.
*
*   All iterators are constexpr-capable and satisfy at least
* ForwardIterator structurally.
*
* TABLE OF CONTENTS
* =================
* I.      Vocabulary: chunk_ref / window_ref
* II.     circular_iterator
* III.    chunk_iterator
* IV.     window_iterator
* V.      Factory Functions
*
*
* path:      \inc\container\array_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.24
******************************************************************************/

#ifndef DJINTERP_ARRAY_ITERATOR_
#define DJINTERP_ARRAY_ITERATOR_ 1

#include <cstddef>
#include <iterator>
#include <type_traits>
#include "..\djinterp.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   Vocabulary: chunk_ref / window_ref
// =============================================================================
// Lightweight non-owning views yielded by chunk_iterator
// and window_iterator on dereference.

// chunk_ref
//   struct: a (pointer, size) pair describing one chunk.
template<typename _Type>
struct chunk_ref
{
    const _Type* data;
    std::size_t  size;

    constexpr const _Type&
    operator[](std::size_t _i) const noexcept
    {
        return data[_i];
    }

    constexpr const _Type* begin() const noexcept
    {
        return data;
    }

    constexpr const _Type* end() const noexcept
    {
        return data + size;
    }

    constexpr bool
    empty() const noexcept
    {
        return (size == 0);
    }
};

// window_ref
//   type alias: a sliding window is structurally
// identical to a chunk — reuse the type.
template<typename _Type>
using window_ref = chunk_ref<_Type>;


// =============================================================================
// II.  circular_iterator
// =============================================================================
// Iterates over a contiguous buffer with wrap-around
// semantics.  Given a buffer of capacity N starting at
// base, a head offset, and a logical index, accesses
// base[(head + index) % capacity].

template<typename _Type>
class circular_iterator
{
public:
    using value_type        = _Type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const _Type*;
    using reference         = const _Type&;
    using iterator_category =
        std::random_access_iterator_tag;

    constexpr circular_iterator() noexcept
        : m_base(nullptr)
        , m_capacity(0)
        , m_head(0)
        , m_index(0)
    {}

    constexpr circular_iterator(
        const _Type* _base,
        std::size_t  _capacity,
        std::size_t  _head,
        std::size_t  _index) noexcept
        : m_base(_base)
        , m_capacity(_capacity)
        , m_head(_head)
        , m_index(_index)
    {}

    // --- dereference ---

    constexpr reference
    operator*() const noexcept
    {
        return m_base[physical_index()];
    }

    constexpr pointer
    operator->() const noexcept
    {
        return &m_base[physical_index()];
    }

    constexpr reference
    operator[](difference_type _n) const noexcept
    {
        std::size_t idx =
            (m_head + m_index +
             static_cast<std::size_t>(_n))
            % m_capacity;

        return m_base[idx];
    }

    // --- increment / decrement ---

    constexpr circular_iterator&
    operator++() noexcept
    {
        ++m_index;
        return *this;
    }

    constexpr circular_iterator
    operator++(int) noexcept
    {
        auto tmp = *this;
        ++m_index;
        return tmp;
    }

    constexpr circular_iterator&
    operator--() noexcept
    {
        --m_index;
        return *this;
    }

    constexpr circular_iterator
    operator--(int) noexcept
    {
        auto tmp = *this;
        --m_index;
        return tmp;
    }

    // --- arithmetic ---

    constexpr circular_iterator&
    operator+=(difference_type _n) noexcept
    {
        m_index += static_cast<std::size_t>(_n);
        return *this;
    }

    constexpr circular_iterator&
    operator-=(difference_type _n) noexcept
    {
        m_index -= static_cast<std::size_t>(_n);
        return *this;
    }

    friend constexpr circular_iterator
    operator+(circular_iterator _it,
              difference_type   _n) noexcept
    {
        _it.m_index +=
            static_cast<std::size_t>(_n);
        return _it;
    }

    friend constexpr circular_iterator
    operator+(difference_type   _n,
              circular_iterator _it) noexcept
    {
        _it.m_index +=
            static_cast<std::size_t>(_n);
        return _it;
    }

    friend constexpr circular_iterator
    operator-(circular_iterator _it,
              difference_type   _n) noexcept
    {
        _it.m_index -=
            static_cast<std::size_t>(_n);
        return _it;
    }

    friend constexpr difference_type
    operator-(circular_iterator _a,
              circular_iterator _b) noexcept
    {
        return static_cast<difference_type>(
            _a.m_index) -
               static_cast<difference_type>(
                   _b.m_index);
    }

    // --- comparison ---

    friend constexpr bool
    operator==(circular_iterator _a,
               circular_iterator _b) noexcept
    {
        return (_a.m_index == _b.m_index);
    }

    friend constexpr bool
    operator!=(circular_iterator _a,
               circular_iterator _b) noexcept
    {
        return (_a.m_index != _b.m_index);
    }

    friend constexpr bool
    operator<(circular_iterator _a,
              circular_iterator _b) noexcept
    {
        return (_a.m_index < _b.m_index);
    }

    friend constexpr bool
    operator<=(circular_iterator _a,
               circular_iterator _b) noexcept
    {
        return (_a.m_index <= _b.m_index);
    }

    friend constexpr bool
    operator>(circular_iterator _a,
              circular_iterator _b) noexcept
    {
        return (_a.m_index > _b.m_index);
    }

    friend constexpr bool
    operator>=(circular_iterator _a,
               circular_iterator _b) noexcept
    {
        return (_a.m_index >= _b.m_index);
    }

    // --- accessors ---

    constexpr std::size_t
    logical_index() const noexcept
    {
        return m_index;
    }

    constexpr std::size_t
    physical_index() const noexcept
    {
        return (m_head + m_index) % m_capacity;
    }

    constexpr std::size_t
    capacity() const noexcept
    {
        return m_capacity;
    }

private:
    const _Type* m_base;
    std::size_t  m_capacity;
    std::size_t  m_head;
    std::size_t  m_index;
};


// =============================================================================
// III. chunk_iterator
// =============================================================================
// Groups a flat contiguous range into non-overlapping
// chunks of _chunk_size elements.  The last chunk may
// be smaller.

template<typename _Iter>
class chunk_iterator
{
public:
    using base_value_type =
        typename std::iterator_traits<
            _Iter>::value_type;
    using value_type      =
        chunk_ref<base_value_type>;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type;
    using pointer         = const value_type*;
    using iterator_category =
        std::forward_iterator_tag;

    constexpr chunk_iterator() noexcept
        : m_current()
        , m_end()
        , m_chunk_size(0)
    {}

    constexpr chunk_iterator(
        _Iter       _current,
        _Iter       _end,
        std::size_t _chunk_size) noexcept
        : m_current(_current)
        , m_end(_end)
        , m_chunk_size(_chunk_size)
    {}

    constexpr value_type
    operator*() const noexcept
    {
        std::size_t remaining =
            static_cast<std::size_t>(
                m_end - m_current);

        std::size_t actual =
            (remaining < m_chunk_size)
                ? remaining : m_chunk_size;

        return { &(*m_current), actual };
    }

    constexpr chunk_iterator&
    operator++() noexcept
    {
        std::size_t remaining =
            static_cast<std::size_t>(
                m_end - m_current);

        std::size_t advance =
            (remaining < m_chunk_size)
                ? remaining : m_chunk_size;

        m_current += static_cast<
            typename std::iterator_traits<
                _Iter>::difference_type>(advance);

        return *this;
    }

    constexpr chunk_iterator
    operator++(int) noexcept
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    friend constexpr bool
    operator==(chunk_iterator _a,
               chunk_iterator _b) noexcept
    {
        return (_a.m_current == _b.m_current);
    }

    friend constexpr bool
    operator!=(chunk_iterator _a,
               chunk_iterator _b) noexcept
    {
        return (_a.m_current != _b.m_current);
    }

    constexpr _Iter base() const noexcept
    {
        return m_current;
    }

    constexpr std::size_t
    chunk_size() const noexcept
    {
        return m_chunk_size;
    }

private:
    _Iter       m_current;
    _Iter       m_end;
    std::size_t m_chunk_size;
};


// =============================================================================
// IV.  window_iterator
// =============================================================================
// Slides a fixed-width window across a contiguous range,
// yielding overlapping sub-ranges.  Advances by 1 element
// per increment.

template<typename _Iter>
class window_iterator
{
public:
    using base_value_type =
        typename std::iterator_traits<
            _Iter>::value_type;
    using value_type      =
        window_ref<base_value_type>;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type;
    using pointer         = const value_type*;
    using iterator_category =
        std::forward_iterator_tag;

    constexpr window_iterator() noexcept
        : m_current()
        , m_end()
        , m_window_size(0)
    {}

    constexpr window_iterator(
        _Iter       _current,
        _Iter       _end,
        std::size_t _window_size) noexcept
        : m_current(_current)
        , m_end(_end)
        , m_window_size(_window_size)
    {}

    constexpr value_type
    operator*() const noexcept
    {
        std::size_t remaining =
            static_cast<std::size_t>(
                m_end - m_current);

        std::size_t actual =
            (remaining < m_window_size)
                ? remaining : m_window_size;

        return { &(*m_current), actual };
    }

    constexpr window_iterator&
    operator++() noexcept
    {
        ++m_current;
        return *this;
    }

    constexpr window_iterator
    operator++(int) noexcept
    {
        auto tmp = *this;
        ++m_current;
        return tmp;
    }

    friend constexpr bool
    operator==(window_iterator _a,
               window_iterator _b) noexcept
    {
        return (_a.m_current == _b.m_current);
    }

    friend constexpr bool
    operator!=(window_iterator _a,
               window_iterator _b) noexcept
    {
        return (_a.m_current != _b.m_current);
    }

    constexpr _Iter base() const noexcept
    {
        return m_current;
    }

    constexpr std::size_t
    window_size() const noexcept
    {
        return m_window_size;
    }

private:
    _Iter       m_current;
    _Iter       m_end;
    std::size_t m_window_size;
};


// =============================================================================
// V.   Factory Functions
// =============================================================================

// make_circular_iterator
template<typename _Type>
constexpr circular_iterator<_Type>
make_circular_iterator(
    const _Type* _base,
    std::size_t  _capacity,
    std::size_t  _head,
    std::size_t  _index) noexcept
{
    return circular_iterator<_Type>(
        _base, _capacity, _head, _index);
}

// make_chunk_iterator
template<typename _Iter>
constexpr chunk_iterator<_Iter>
make_chunk_iterator(
    _Iter       _begin,
    _Iter       _end,
    std::size_t _chunk_size) noexcept
{
    return chunk_iterator<_Iter>(
        _begin, _end, _chunk_size);
}

// make_chunk_end
template<typename _Iter>
constexpr chunk_iterator<_Iter>
make_chunk_end(
    _Iter       _end,
    std::size_t _chunk_size) noexcept
{
    return chunk_iterator<_Iter>(
        _end, _end, _chunk_size);
}

// make_window_iterator
template<typename _Iter>
constexpr window_iterator<_Iter>
make_window_iterator(
    _Iter       _begin,
    _Iter       _end,
    std::size_t _window_size) noexcept
{
    return window_iterator<_Iter>(
        _begin, _end, _window_size);
}

// make_window_end
//   the end iterator is positioned so that the last full
// window's first element is at (end - window_size + 1).
template<typename _Iter>
constexpr window_iterator<_Iter>
make_window_end(
    _Iter       _begin,
    _Iter       _end,
    std::size_t _window_size) noexcept
{
    auto dist = static_cast<std::size_t>(
        _end - _begin);

    if (dist < _window_size)
    {
        return window_iterator<_Iter>(
            _end, _end, _window_size);
    }

    auto last_start = _begin +
        static_cast<
            typename std::iterator_traits<
                _Iter>::difference_type>(
                    dist - _window_size + 1);

    return window_iterator<_Iter>(
        last_start, _end, _window_size);
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_ARRAY_ITERATOR_
