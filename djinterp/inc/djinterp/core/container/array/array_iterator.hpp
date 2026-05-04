/******************************************************************************
* djinterp [container]                                      array_iterator.hpp
*
* Array-specific iterator adapters.
*   Thin zero-overhead wrappers for patterns unique to contiguous
* array storage:
*
*     circular_iterator<T>   - wraps around a fixed-capacity buffer,
*                              enabling ring-buffer iteration from any
*                              head position.
*     chunk_iterator<Iter>   - groups a flat sequence into fixed-size
*                              sub-ranges (chunks), each exposed as a
*                              (data, size) pair.
*     window_iterator<Iter>  - slides a fixed-width window across the
*                              sequence, yielding overlapping
*                              sub-ranges.
*
*   All iterators are constexpr-capable and satisfy at least the
* ForwardIterator structural requirements.
*
*   PORTABILITY:
*   C++11 baseline.  Mutating constexpr operations require C++14
* relaxed constexpr; on C++11, the iterator is constexpr-
* constructible but mutators degrade to non-constexpr (handled by
* the same D_CONSTEXPR macro used in constexpr_iterator.hpp).
*
*
* path:      /inc/djinterp/core/container/array/array_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.   vocabulary: chunk_ref / window_ref
2.   circular_iterator
3.   chunk_iterator
4.   window_iterator
5.   factory functions
*/

#ifndef DJINTERP_ARRAY_ITERATOR_
#define DJINTERP_ARRAY_ITERATOR_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
// djinterp
#include "../../../core/djinterp.hpp"


// ===========================================================================
//  Constexpr feature gate (mutators)
// ===========================================================================
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #define D_INTERNAL_AIT_CONSTEXPR D_CONSTEXPR
#else
    #define D_INTERNAL_AIT_CONSTEXPR
#endif


NS_DJINTERP

// ===========================================================================
// I.   Vocabulary: chunk_ref / window_ref
// ===========================================================================
// Lightweight non-owning views yielded by chunk_iterator
// and window_iterator on dereference.

// chunk_ref
//   struct: a (pointer, size) pair describing one chunk.
template<typename _Type>
struct chunk_ref
{
    const _Type* data;
    std::size_t  size;

    D_CONSTEXPR const _Type&
    operator[](
        std::size_t _i
    ) const D_NOEXCEPT
    {
        return data[_i];
    }

    D_CONSTEXPR const _Type*
    begin() const D_NOEXCEPT
    {
        return data;
    }

    D_CONSTEXPR const _Type*
    end() const D_NOEXCEPT
    {
        return (data + size);
    }

    D_CONSTEXPR bool
    empty() const D_NOEXCEPT
    {
        return (size == 0);
    }
};

// window_ref
//   alias: a sliding window is structurally identical to a
// chunk - reuse the type.
#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
    template<typename _Type>
    using window_ref = chunk_ref<_Type>;
#endif


// ===========================================================================
// II.  circular_iterator
// ===========================================================================

// circular_iterator
//   class: iterates over a contiguous buffer with wrap-
// around semantics.  Given a buffer of capacity N starting
// at base, a head offset, and a logical index, accesses
// base[(head + index) % capacity].
template<typename _Type>
class circular_iterator
{
public:
    using value_type        = _Type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const _Type*;
    using reference         = const _Type&;
    using iterator_category = std::random_access_iterator_tag;

    D_CONSTEXPR
    circular_iterator() D_NOEXCEPT
        : m_base(nullptr),
          m_capacity(0),
          m_head(0),
          m_index(0)
    {}

    D_CONSTEXPR
    circular_iterator(
        const _Type* _base,
        std::size_t  _capacity,
        std::size_t  _head,
        std::size_t  _index
    ) D_NOEXCEPT
        : m_base(_base),
          m_capacity(_capacity),
          m_head(_head),
          m_index(_index)
    {}

    // --- dereference ---

    D_CONSTEXPR reference
    operator*() const D_NOEXCEPT
    {
        return m_base[physical_index()];
    }

    D_CONSTEXPR pointer
    operator->() const D_NOEXCEPT
    {
        return &m_base[physical_index()];
    }

    D_CONSTEXPR reference
    operator[](
        difference_type _n
    ) const D_NOEXCEPT
    {
        return m_base[
            ((m_head + m_index +
              static_cast<std::size_t>(_n)) % m_capacity)];
    }

    // --- increment / decrement ---

    D_INTERNAL_AIT_CONSTEXPR circular_iterator&
    operator++() D_NOEXCEPT
    {
        ++m_index;

        return *this;
    }

    D_INTERNAL_AIT_CONSTEXPR circular_iterator
    operator++(int) D_NOEXCEPT
    {
        circular_iterator tmp = *this;
        ++m_index;

        return tmp;
    }

    D_INTERNAL_AIT_CONSTEXPR circular_iterator&
    operator--() D_NOEXCEPT
    {
        --m_index;

        return *this;
    }

    D_INTERNAL_AIT_CONSTEXPR circular_iterator
    operator--(int) D_NOEXCEPT
    {
        circular_iterator tmp = *this;
        --m_index;

        return tmp;
    }

    // --- arithmetic ---

    D_INTERNAL_AIT_CONSTEXPR circular_iterator&
    operator+=(
        difference_type _n
    ) D_NOEXCEPT
    {
        m_index += static_cast<std::size_t>(_n);

        return *this;
    }

    D_INTERNAL_AIT_CONSTEXPR circular_iterator&
    operator-=(
        difference_type _n
    ) D_NOEXCEPT
    {
        m_index -= static_cast<std::size_t>(_n);

        return *this;
    }

    friend D_CONSTEXPR circular_iterator
    operator+(
        circular_iterator _it,
        difference_type   _n
    ) D_NOEXCEPT
    {
        return circular_iterator(
            _it.m_base,
            _it.m_capacity,
            _it.m_head,
            _it.m_index + static_cast<std::size_t>(_n));
    }

    friend D_CONSTEXPR circular_iterator
    operator+(
        difference_type   _n,
        circular_iterator _it
    ) D_NOEXCEPT
    {
        return circular_iterator(
            _it.m_base,
            _it.m_capacity,
            _it.m_head,
            _it.m_index + static_cast<std::size_t>(_n));
    }

    friend D_CONSTEXPR circular_iterator
    operator-(
        circular_iterator _it,
        difference_type   _n
    ) D_NOEXCEPT
    {
        return circular_iterator(
            _it.m_base,
            _it.m_capacity,
            _it.m_head,
            _it.m_index - static_cast<std::size_t>(_n));
    }

    friend D_CONSTEXPR difference_type
    operator-(
        circular_iterator _a,
        circular_iterator _b
    ) D_NOEXCEPT
    {
        return ( static_cast<difference_type>(_a.m_index) -
                 static_cast<difference_type>(_b.m_index) );
    }

    // --- comparison ---

    friend D_CONSTEXPR bool
    operator==(
        circular_iterator _a,
        circular_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_index == _b.m_index);
    }

    friend D_CONSTEXPR bool
    operator!=(
        circular_iterator _a,
        circular_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_index != _b.m_index);
    }

    friend D_CONSTEXPR bool
    operator<(
        circular_iterator _a,
        circular_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_index < _b.m_index);
    }

    friend D_CONSTEXPR bool
    operator<=(
        circular_iterator _a,
        circular_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_index <= _b.m_index);
    }

    friend D_CONSTEXPR bool
    operator>(
        circular_iterator _a,
        circular_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_index > _b.m_index);
    }

    friend D_CONSTEXPR bool
    operator>=(
        circular_iterator _a,
        circular_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_index >= _b.m_index);
    }

    // --- accessors ---

    D_CONSTEXPR std::size_t
    logical_index() const D_NOEXCEPT
    {
        return m_index;
    }

    D_CONSTEXPR std::size_t
    physical_index() const D_NOEXCEPT
    {
        return ((m_head + m_index) % m_capacity);
    }

    D_CONSTEXPR std::size_t
    capacity() const D_NOEXCEPT
    {
        return m_capacity;
    }

private:
    const _Type* m_base;
    std::size_t  m_capacity;
    std::size_t  m_head;
    std::size_t  m_index;
};


// ===========================================================================
// III. chunk_iterator
// ===========================================================================

// chunk_iterator
//   class: groups a flat contiguous range into non-
// overlapping chunks of m_chunk_size elements.  The last
// chunk may be smaller.
template<typename _Iterator>
class chunk_iterator
{
public:
    using base_value_type =
        typename std::iterator_traits<_Iterator>::value_type;
    using value_type        = chunk_ref<base_value_type>;
    using difference_type   = std::ptrdiff_t;
    using reference         = value_type;
    using pointer           = const value_type*;
    using iterator_category = std::forward_iterator_tag;

    D_CONSTEXPR
    chunk_iterator() D_NOEXCEPT
        : m_current(),
          m_end(),
          m_chunk_size(0)
    {}

    D_CONSTEXPR
    chunk_iterator(
        _Iterator   _current,
        _Iterator   _end,
        std::size_t _chunk_size
    ) D_NOEXCEPT
        : m_current(_current),
          m_end(_end),
          m_chunk_size(_chunk_size)
    {}

    D_CONSTEXPR value_type
    operator*() const D_NOEXCEPT
    {
        const std::size_t remaining =
            static_cast<std::size_t>(m_end - m_current);
        const std::size_t actual =
            (remaining < m_chunk_size)
                ? remaining
                : m_chunk_size;

        return value_type{ &(*m_current), actual };
    }

    D_INTERNAL_AIT_CONSTEXPR chunk_iterator&
    operator++() D_NOEXCEPT
    {
        const std::size_t remaining =
            static_cast<std::size_t>(m_end - m_current);
        const std::size_t advance =
            (remaining < m_chunk_size)
                ? remaining
                : m_chunk_size;

        m_current += static_cast<typename
            std::iterator_traits<_Iterator>::difference_type>(advance);

        return *this;
    }

    D_INTERNAL_AIT_CONSTEXPR chunk_iterator
    operator++(int) D_NOEXCEPT
    {
        chunk_iterator tmp = *this;
        ++(*this);

        return tmp;
    }

    friend D_CONSTEXPR bool
    operator==(
        chunk_iterator _a,
        chunk_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_current == _b.m_current);
    }

    friend D_CONSTEXPR bool
    operator!=(
        chunk_iterator _a,
        chunk_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_current != _b.m_current);
    }

    D_CONSTEXPR _Iterator
    base() const D_NOEXCEPT
    {
        return m_current;
    }

    D_CONSTEXPR std::size_t
    chunk_size() const D_NOEXCEPT
    {
        return m_chunk_size;
    }

private:
    _Iterator   m_current;
    _Iterator   m_end;
    std::size_t m_chunk_size;
};


// ===========================================================================
// IV.  window_iterator
// ===========================================================================

// window_iterator
//   class: slides a fixed-width window across a contiguous
// range, yielding overlapping sub-ranges.  Advances by 1
// element per increment.
template<typename _Iterator>
class window_iterator
{
public:
    using base_value_type =
        typename std::iterator_traits<_Iterator>::value_type;
    using value_type        = chunk_ref<base_value_type>;
    using difference_type   = std::ptrdiff_t;
    using reference         = value_type;
    using pointer           = const value_type*;
    using iterator_category = std::forward_iterator_tag;

    D_CONSTEXPR
    window_iterator() D_NOEXCEPT
        : m_current(),
          m_end(),
          m_window_size(0)
    {}

    D_CONSTEXPR
    window_iterator(
        _Iterator   _current,
        _Iterator   _end,
        std::size_t _window_size
    ) D_NOEXCEPT
        : m_current(_current),
          m_end(_end),
          m_window_size(_window_size)
    {}

    D_CONSTEXPR value_type
    operator*() const D_NOEXCEPT
    {
        const std::size_t remaining =
            static_cast<std::size_t>(m_end - m_current);
        const std::size_t actual =
            (remaining < m_window_size)
                ? remaining
                : m_window_size;

        return value_type{ &(*m_current), actual };
    }

    D_INTERNAL_AIT_CONSTEXPR window_iterator&
    operator++() D_NOEXCEPT
    {
        ++m_current;

        return *this;
    }

    D_INTERNAL_AIT_CONSTEXPR window_iterator
    operator++(int) D_NOEXCEPT
    {
        window_iterator tmp = *this;
        ++m_current;

        return tmp;
    }

    friend D_CONSTEXPR bool
    operator==(
        window_iterator _a,
        window_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_current == _b.m_current);
    }

    friend D_CONSTEXPR bool
    operator!=(
        window_iterator _a,
        window_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_current != _b.m_current);
    }

    D_CONSTEXPR _Iterator
    base() const D_NOEXCEPT
    {
        return m_current;
    }

    D_CONSTEXPR std::size_t
    window_size() const D_NOEXCEPT
    {
        return m_window_size;
    }

private:
    _Iterator   m_current;
    _Iterator   m_end;
    std::size_t m_window_size;
};


// ===========================================================================
// V.   Factory Functions
// ===========================================================================

// make_circular_iterator
//   factory: constructs a circular_iterator over a fixed
// buffer.
template<typename _Type>
D_CONSTEXPR circular_iterator<_Type>
make_circular_iterator(
    const _Type* _base,
    std::size_t  _capacity,
    std::size_t  _head,
    std::size_t  _index
) D_NOEXCEPT
{
    return circular_iterator<_Type>(_base,
                                    _capacity,
                                    _head,
                                    _index);
}

// make_chunk_iterator
//   factory: constructs the begin chunk_iterator.
template<typename _Iterator>
D_CONSTEXPR chunk_iterator<_Iterator>
make_chunk_iterator(
    _Iterator   _begin,
    _Iterator   _end,
    std::size_t _chunk_size
) D_NOEXCEPT
{
    return chunk_iterator<_Iterator>(_begin,
                                     _end,
                                     _chunk_size);
}

// make_chunk_end
//   factory: constructs the past-the-end chunk_iterator.
template<typename _Iterator>
D_CONSTEXPR chunk_iterator<_Iterator>
make_chunk_end(
    _Iterator   _end,
    std::size_t _chunk_size
) D_NOEXCEPT
{
    return chunk_iterator<_Iterator>(_end,
                                     _end,
                                     _chunk_size);
}

// make_window_iterator
//   factory: constructs the begin window_iterator.
template<typename _Iterator>
D_CONSTEXPR window_iterator<_Iterator>
make_window_iterator(
    _Iterator   _begin,
    _Iterator   _end,
    std::size_t _window_size
) D_NOEXCEPT
{
    return window_iterator<_Iterator>(_begin,
                                      _end,
                                      _window_size);
}

// make_window_end
//   factory: constructs the past-the-end window_iterator.
// The end iterator is positioned so the last full window's
// first element is at (end - window_size + 1).
template<typename _Iterator>
D_INTERNAL_AIT_CONSTEXPR window_iterator<_Iterator>
make_window_end(
    _Iterator   _begin,
    _Iterator   _end,
    std::size_t _window_size
) D_NOEXCEPT
{
    const std::size_t dist =
        static_cast<std::size_t>(_end - _begin);

    // not enough data for any full window - return _end
    if (dist < _window_size)
    {
        return window_iterator<_Iterator>(_end,
                                          _end,
                                          _window_size);
    }

    _Iterator last_start = _begin +
        static_cast<typename
            std::iterator_traits<_Iterator>::difference_type>(
            dist - _window_size + 1);

    return window_iterator<_Iterator>(last_start,
                                      _end,
                                      _window_size);
}


NS_END  // djinterp


#undef D_INTERNAL_AIT_CONSTEXPR


#endif  // DJINTERP_ARRAY_ITERATOR_
