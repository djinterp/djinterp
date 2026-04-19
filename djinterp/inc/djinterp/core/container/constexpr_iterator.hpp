/******************************************************************************
* djinterp [container]                                constexpr_iterator.hpp
*
* Compile-time iterator for constexpr containers.
*   Provides constexpr_iterator<T>, a fully constexpr random-access
* iterator that operates over contiguous storage at compile time.
* This is the constexpr counterpart to the standard iterator pair:
*
*   iterator            — mutable,   runtime
*   const_iterator      — immutable, runtime
*   constexpr_iterator  — immutable, compile-time (constexpr)
*
*   constexpr_iterator satisfies all five LegacyRandomAccessIterator
* structural requirements in a constexpr context.  On C++20+ it also
* models std::contiguous_iterator via contiguous_iterator_tag.
*
*   For containers that store their data in constexpr-accessible
* storage (e.g. constexpr std::array, constexpr fixed_array), the
* constexpr_iterator enables compile-time algorithms: fold, find,
* count, transform, etc.
*
* USAGE:
*   constexpr std::array<int, 4> data = {1, 2, 3, 4};
*
*   constexpr auto it  = constexpr_iterator<int>(data.data());
*   constexpr auto end = constexpr_iterator<int>(data.data() + 4);
*
*   static_assert(*it == 1);
*   static_assert(*(it + 2) == 3);
*   static_assert((end - it) == 4);
*
*   // range adapter
*   constexpr auto range =
*       make_constexpr_range(data.data(), data.size());
*   static_assert(range.size() == 4);
*   static_assert(range[2] == 3);
*
* TABLE OF CONTENTS
* =================
* I.      constexpr_iterator
* II.     constexpr_range
* III.    Factory Functions
* IV.     Compile-Time Algorithms
*
*
* path:      \inc\container\constexpr_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONSTEXPR_ITERATOR_
#define DJINTERP_CONSTEXPR_ITERATOR_ 1

#include <cstddef>
#include <iterator>
#include <type_traits>
#include "..\djinterp.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   constexpr_iterator
// =============================================================================

// constexpr_iterator
//   class: a fully constexpr random-access iterator over
// contiguous const storage.  All operations are constexpr,
// enabling compile-time iteration and algorithms.
//
// The iterator is always immutable: dereference yields
// const _Type&.  This mirrors const_iterator semantics but
// in a constexpr-evaluable context.
template<typename _Type>
class constexpr_iterator
{
public:
    // --- standard iterator type aliases ---
    using value_type        = _Type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const _Type*;
    using reference         = const _Type&;

#if (__cplusplus >= 202002L)
    using iterator_category =
        std::contiguous_iterator_tag;
    using iterator_concept  =
        std::contiguous_iterator_tag;
#else
    using iterator_category =
        std::random_access_iterator_tag;
#endif

    // --- construction ---

    constexpr constexpr_iterator() noexcept
        : m_ptr(nullptr)
    {
    }

    constexpr explicit
    constexpr_iterator(const _Type* _ptr) noexcept
        : m_ptr(_ptr)
    {
    }

    // --- dereference ---

    constexpr reference
    operator*() const noexcept
    {
        return *m_ptr;
    }

    constexpr pointer
    operator->() const noexcept
    {
        return m_ptr;
    }

    constexpr reference
    operator[](difference_type _n) const noexcept
    {
        return m_ptr[_n];
    }

    // --- increment / decrement ---

    constexpr constexpr_iterator&
    operator++() noexcept
    {
        ++m_ptr;

        return *this;
    }

    constexpr constexpr_iterator
    operator++(int) noexcept
    {
        constexpr_iterator tmp = *this;
        ++m_ptr;

        return tmp;
    }

    constexpr constexpr_iterator&
    operator--() noexcept
    {
        --m_ptr;

        return *this;
    }

    constexpr constexpr_iterator
    operator--(int) noexcept
    {
        constexpr_iterator tmp = *this;
        --m_ptr;

        return tmp;
    }

    // --- arithmetic ---

    constexpr constexpr_iterator&
    operator+=(difference_type _n) noexcept
    {
        m_ptr += _n;

        return *this;
    }

    constexpr constexpr_iterator&
    operator-=(difference_type _n) noexcept
    {
        m_ptr -= _n;

        return *this;
    }

    friend constexpr constexpr_iterator
    operator+(constexpr_iterator      _it,
              difference_type         _n) noexcept
    {
        return constexpr_iterator(_it.m_ptr + _n);
    }

    friend constexpr constexpr_iterator
    operator+(difference_type         _n,
              constexpr_iterator      _it) noexcept
    {
        return constexpr_iterator(_it.m_ptr + _n);
    }

    friend constexpr constexpr_iterator
    operator-(constexpr_iterator      _it,
              difference_type         _n) noexcept
    {
        return constexpr_iterator(_it.m_ptr - _n);
    }

    friend constexpr difference_type
    operator-(constexpr_iterator _a,
              constexpr_iterator _b) noexcept
    {
        return (_a.m_ptr - _b.m_ptr);
    }

    // --- comparison ---

    friend constexpr bool
    operator==(constexpr_iterator _a,
               constexpr_iterator _b) noexcept
    {
        return (_a.m_ptr == _b.m_ptr);
    }

    friend constexpr bool
    operator!=(constexpr_iterator _a,
               constexpr_iterator _b) noexcept
    {
        return (_a.m_ptr != _b.m_ptr);
    }

    friend constexpr bool
    operator<(constexpr_iterator _a,
              constexpr_iterator _b) noexcept
    {
        return (_a.m_ptr < _b.m_ptr);
    }

    friend constexpr bool
    operator<=(constexpr_iterator _a,
               constexpr_iterator _b) noexcept
    {
        return (_a.m_ptr <= _b.m_ptr);
    }

    friend constexpr bool
    operator>(constexpr_iterator _a,
              constexpr_iterator _b) noexcept
    {
        return (_a.m_ptr > _b.m_ptr);
    }

    friend constexpr bool
    operator>=(constexpr_iterator _a,
               constexpr_iterator _b) noexcept
    {
        return (_a.m_ptr >= _b.m_ptr);
    }

    // --- raw pointer access ---

    constexpr const _Type*
    base() const noexcept
    {
        return m_ptr;
    }

private:
    const _Type* m_ptr;
};


// =============================================================================
// II.  constexpr_range
// =============================================================================
// A lightweight constexpr view over contiguous storage,
// providing begin/end constexpr_iterators and element
// access.

// constexpr_range
//   class: a compile-time range adapter over contiguous
// const storage.  Provides constexpr_iterator-based
// iteration, indexed access, and size queries.
template<typename _Type>
class constexpr_range
{
public:
    using value_type     = _Type;
    using size_type      = std::size_t;
    using iterator       = constexpr_iterator<_Type>;
    using const_iterator = constexpr_iterator<_Type>;

    // the constexpr_iterator type itself
    using constexpr_iterator =
        djinterp::container::constexpr_iterator<_Type>;

    // --- construction ---

    constexpr constexpr_range() noexcept
        : m_data(nullptr)
        , m_size(0)
    {
    }

    constexpr constexpr_range(
        const _Type* _data,
        size_type    _count) noexcept
        : m_data(_data)
        , m_size(_count)
    {
    }

    // construct from any contiguous container with data()
    // and size()
    template<typename _Container>
    constexpr constexpr_range(
        const _Container& _c) noexcept
        : m_data(_c.data())
        , m_size(_c.size())
    {
    }

    // --- iteration ---

    constexpr iterator begin() const noexcept
    {
        return iterator(m_data);
    }

    constexpr iterator end() const noexcept
    {
        return iterator(m_data + m_size);
    }

    constexpr iterator constexpr_begin() const noexcept
    {
        return begin();
    }

    constexpr iterator constexpr_end() const noexcept
    {
        return end();
    }

    // --- access ---

    constexpr const _Type&
    operator[](size_type _index) const noexcept
    {
        return m_data[_index];
    }

    constexpr const _Type&
    front() const noexcept
    {
        return m_data[0];
    }

    constexpr const _Type&
    back() const noexcept
    {
        return m_data[m_size - 1];
    }

    constexpr const _Type*
    data() const noexcept
    {
        return m_data;
    }

    // --- capacity ---

    constexpr size_type
    size() const noexcept
    {
        return m_size;
    }

    constexpr bool
    empty() const noexcept
    {
        return (m_size == 0);
    }

private:
    const _Type* m_data;
    size_type    m_size;
};


// =============================================================================
// III. Factory Functions
// =============================================================================

// make_constexpr_range
//   function: constructs a constexpr_range from a pointer
// and count.
template<typename _Type>
constexpr constexpr_range<_Type>
make_constexpr_range
(
    const _Type* _data,
    std::size_t  _count
) noexcept
{
    return constexpr_range<_Type>(_data, _count);
}

// make_constexpr_range (container)
//   function: constructs a constexpr_range from any
// contiguous container with data() and size().
template<typename _Container>
constexpr auto
make_constexpr_range(const _Container& _c) noexcept
    -> constexpr_range<
           typename _Container::value_type>
{
    return constexpr_range<
        typename _Container::value_type>(
            _c.data(), _c.size());
}

// make_constexpr_range (C array)
//   function: constructs a constexpr_range from a C
// array.
template<typename _Type, std::size_t _N>
constexpr constexpr_range<_Type>
make_constexpr_range(const _Type (&_arr)[_N]) noexcept
{
    return constexpr_range<_Type>(_arr, _N);
}


// =============================================================================
// IV.  Compile-Time Algorithms
// =============================================================================
// Minimal constexpr algorithm vocabulary operating on
// constexpr_range or any constexpr_iterator pair.

// constexpr_find
//   function: returns the first iterator where *it == val,
// or end if not found.
template<typename _Iter,
         typename _Value>
constexpr _Iter
constexpr_find(_Iter _begin, _Iter _end,
               const _Value& _val)
{
    for (; _begin != _end; ++_begin)
    {
        if (*_begin == _val)
        {
            return _begin;
        }
    }

    return _end;
}

// constexpr_find_if
//   function: returns the first iterator where pred(*it)
// is true, or end if not found.
template<typename _Iter,
         typename _Pred>
constexpr _Iter
constexpr_find_if(_Iter _begin, _Iter _end,
                  _Pred _pred)
{
    for (; _begin != _end; ++_begin)
    {
        if (_pred(*_begin))
        {
            return _begin;
        }
    }

    return _end;
}

// constexpr_count
//   function: counts elements equal to val.
template<typename _Iter,
         typename _Value>
constexpr std::size_t
constexpr_count(_Iter _begin, _Iter _end,
                const _Value& _val)
{
    std::size_t n = 0;

    for (; _begin != _end; ++_begin)
    {
        if (*_begin == _val)
        {
            ++n;
        }
    }

    return n;
}

// constexpr_count_if
//   function: counts elements satisfying pred.
template<typename _Iter,
         typename _Pred>
constexpr std::size_t
constexpr_count_if(_Iter _begin, _Iter _end,
                   _Pred _pred)
{
    std::size_t n = 0;

    for (; _begin != _end; ++_begin)
    {
        if (_pred(*_begin))
        {
            ++n;
        }
    }

    return n;
}

// constexpr_all_of
//   function: true if pred(*it) holds for every element.
template<typename _Iter,
         typename _Pred>
constexpr bool
constexpr_all_of(_Iter _begin, _Iter _end,
                 _Pred _pred)
{
    for (; _begin != _end; ++_begin)
    {
        if (!_pred(*_begin))
        {
            return false;
        }
    }

    return true;
}

// constexpr_any_of
//   function: true if pred(*it) holds for at least one
// element.
template<typename _Iter,
         typename _Pred>
constexpr bool
constexpr_any_of(_Iter _begin, _Iter _end,
                 _Pred _pred)
{
    for (; _begin != _end; ++_begin)
    {
        if (_pred(*_begin))
        {
            return true;
        }
    }

    return false;
}

// constexpr_none_of
//   function: true if pred(*it) is false for every
// element.
template<typename _Iter,
         typename _Pred>
constexpr bool
constexpr_none_of(_Iter _begin, _Iter _end,
                  _Pred _pred)
{
    return !constexpr_any_of(_begin, _end, _pred);
}

// constexpr_fold
//   function: left fold over [begin, end) with an initial
// accumulator value.
template<typename _Iter,
         typename _Acc,
         typename _Fn>
constexpr _Acc
constexpr_fold(_Iter       _begin,
               _Iter       _end,
               _Acc        _init,
               _Fn         _fn)
{
    for (; _begin != _end; ++_begin)
    {
        _init = _fn(_init, *_begin);
    }

    return _init;
}

// constexpr_accumulate
//   function: sums elements over [begin, end) starting
// from _init.
template<typename _Iter,
         typename _Value>
constexpr _Value
constexpr_accumulate(_Iter   _begin,
                     _Iter   _end,
                     _Value  _init)
{
    for (; _begin != _end; ++_begin)
    {
        _init = _init + *_begin;
    }

    return _init;
}

// constexpr_min_element
//   function: returns iterator to the smallest element.
template<typename _Iter>
constexpr _Iter
constexpr_min_element(_Iter _begin, _Iter _end)
{
    if (_begin == _end)
    {
        return _end;
    }

    _Iter result = _begin;
    ++_begin;

    for (; _begin != _end; ++_begin)
    {
        if (*_begin < *result)
        {
            result = _begin;
        }
    }

    return result;
}

// constexpr_max_element
//   function: returns iterator to the largest element.
template<typename _Iter>
constexpr _Iter
constexpr_max_element(_Iter _begin, _Iter _end)
{
    if (_begin == _end)
    {
        return _end;
    }

    _Iter result = _begin;
    ++_begin;

    for (; _begin != _end; ++_begin)
    {
        if (*result < *_begin)
        {
            result = _begin;
        }
    }

    return result;
}

// constexpr_equal
//   function: true if ranges [a_begin, a_end) and
// [b_begin, ...) are element-wise equal.
template<typename _IterA,
         typename _IterB>
constexpr bool
constexpr_equal(_IterA _a_begin,
                _IterA _a_end,
                _IterB _b_begin)
{
    for (; _a_begin != _a_end; ++_a_begin, ++_b_begin)
    {
        if (!(*_a_begin == *_b_begin))
        {
            return false;
        }
    }

    return true;
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONSTEXPR_ITERATOR_
