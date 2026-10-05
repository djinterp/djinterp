/*******************************************************************************
* djinterp [core]                                         constexpr_iterator.hpp
*
* djinterp constexpr_iterator.hpp
*
* Compile-time iterator for constexpr containers.
*   Provides constexpr_iterator<T>, a fully constexpr random-access
* iterator that operates over contiguous storage at compile time.
* This is the constexpr counterpart to the standard iterator pair:
*
*     iterator            - mutable,   runtime
*     const_iterator      - immutable, runtime
*     constexpr_iterator  - immutable, compile-time (constexpr)
*
*   constexpr_iterator satisfies all five LegacyRandomAccessIterator
* structural requirements in a constexpr context.  On C++20+ it
* additionally models std::contiguous_iterator via
* contiguous_iterator_tag.
*
*   For containers that store their data in constexpr-accessible
* storage (e.g. constexpr std::array, constexpr array),
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
*   constexpr auto range =
*       make_constexpr_range(data.data(), data.size());
*   static_assert(range.size() == 4);
*   static_assert(range[2] == 3);
*
*   PORTABILITY:
*   C++11 baseline for relaxed constexpr is required only for
* mutating operations on the iterator's internal pointer; under
* C++11 the iterator is still constexpr-constructible but the
* increment/decrement and arithmetic in-place mutators degrade
* to non-constexpr.  C++14+ enables full constexpr usage.
*
*
* path:      /inc/djinterp/core/container/iterator/constexpr_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.23
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    constexpr_iterator
      ------------------

II.   constexpr_range
      ---------------

III.  Factory Functions
      -----------------

IV.   Compile-Time Algorithms
      -----------------------
*/

#ifndef DJINTERP_CONTAINER_ITERATOR_CONSTEXPR_ITERATOR_HPP
#define DJINTERP_CONTAINER_ITERATOR_CONSTEXPR_ITERATOR_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"


// ===========================================================================
//  Constexpr feature gate
// ===========================================================================
// Mutating constexpr operations require C++14 relaxed constexpr.
// On C++11 we still expose a constexpr-constructible iterator,
// but mutators are not constexpr.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #define D_INTERNAL_CXIT_CONSTEXPR D_CONSTEXPR
#else
    #define D_INTERNAL_CXIT_CONSTEXPR
#endif


NS_DJINTERP

// ===========================================================================
// I.   constexpr_iterator
// ===========================================================================

// constexpr_iterator
//   class: a fully constexpr random-access iterator over contiguous const
// storage. All operations are constexpr, enabling compile-time iteration and
// algorithms.
//   The iterator is always immutable: dereference yields const Type&. This
// mirrors const_iterator semantics but in a constexpr-evaluable context.
template<typename Type>
class constexpr_iterator
{
public:
    using value_type      = Type;
    using difference_type = std::ptrdiff_t;
    using pointer         = const Type*;
    using reference       = const Type&;

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    using iterator_category = std::contiguous_iterator_tag;
    using iterator_concept  = std::contiguous_iterator_tag;
#else
    using iterator_category = std::random_access_iterator_tag;
#endif

    // --- construction ---

    D_CONSTEXPR
    constexpr_iterator() D_NOEXCEPT
        : m_ptr(nullptr)
    {}

    D_CONSTEXPR explicit
    constexpr_iterator(
        const Type* _ptr
    ) D_NOEXCEPT
        : m_ptr(_ptr)
    {}

    // --- dereference ---

    D_CONSTEXPR reference
    operator*() const D_NOEXCEPT
    {
        return *m_ptr;
    }

    D_CONSTEXPR pointer
    operator->() const D_NOEXCEPT
    {
        return m_ptr;
    }

    D_CONSTEXPR reference
    operator[](
        difference_type _n
    ) const D_NOEXCEPT
    {
        return m_ptr[_n];
    }

    // --- increment / decrement ---

    D_INTERNAL_CXIT_CONSTEXPR constexpr_iterator&
    operator++() D_NOEXCEPT
    {
        ++m_ptr;

        return *this;
    }

    D_INTERNAL_CXIT_CONSTEXPR constexpr_iterator
    operator++(int) D_NOEXCEPT
    {
        constexpr_iterator tmp = *this;
        ++m_ptr;

        return tmp;
    }

    D_INTERNAL_CXIT_CONSTEXPR constexpr_iterator&
    operator--() D_NOEXCEPT
    {
        --m_ptr;

        return *this;
    }

    D_INTERNAL_CXIT_CONSTEXPR constexpr_iterator
    operator--(int) D_NOEXCEPT
    {
        constexpr_iterator tmp = *this;
        --m_ptr;

        return tmp;
    }

    // --- arithmetic ---

    D_INTERNAL_CXIT_CONSTEXPR constexpr_iterator&
    operator+=(
        difference_type _n
    ) D_NOEXCEPT
    {
        m_ptr += _n;

        return *this;
    }

    D_INTERNAL_CXIT_CONSTEXPR constexpr_iterator&
    operator-=(
        difference_type _n
    ) D_NOEXCEPT
    {
        m_ptr -= _n;

        return *this;
    }

    friend D_CONSTEXPR constexpr_iterator
    operator+(
        constexpr_iterator _it,
        difference_type    _n
    ) D_NOEXCEPT
    {
        return constexpr_iterator(_it.m_ptr + _n);
    }

    friend D_CONSTEXPR constexpr_iterator
    operator+(
        difference_type    _n,
        constexpr_iterator _it
    ) D_NOEXCEPT
    {
        return constexpr_iterator(_it.m_ptr + _n);
    }

    friend D_CONSTEXPR constexpr_iterator
    operator-(
        constexpr_iterator _it,
        difference_type    _n
    ) D_NOEXCEPT
    {
        return constexpr_iterator(_it.m_ptr - _n);
    }

    friend D_CONSTEXPR difference_type
    operator-(
        constexpr_iterator _a,
        constexpr_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_ptr - _b.m_ptr);
    }

    // --- comparison ---

    friend D_CONSTEXPR bool
    operator==(
        constexpr_iterator _a,
        constexpr_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_ptr == _b.m_ptr);
    }

    friend D_CONSTEXPR bool
    operator!=(
        constexpr_iterator _a,
        constexpr_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_ptr != _b.m_ptr);
    }

    friend D_CONSTEXPR bool
    operator<(
        constexpr_iterator _a,
        constexpr_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_ptr < _b.m_ptr);
    }

    friend D_CONSTEXPR bool
    operator<=(
        constexpr_iterator _a,
        constexpr_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_ptr <= _b.m_ptr);
    }

    friend D_CONSTEXPR bool
    operator>(
        constexpr_iterator _a,
        constexpr_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_ptr > _b.m_ptr);
    }

    friend D_CONSTEXPR bool
    operator>=(
        constexpr_iterator _a,
        constexpr_iterator _b
    ) D_NOEXCEPT
    {
        return (_a.m_ptr >= _b.m_ptr);
    }

    // --- raw pointer access ---

    D_CONSTEXPR const Type*
    base() const D_NOEXCEPT
    {
        return m_ptr;
    }

private:
    const Type* m_ptr;
};


// ===========================================================================
// II.  constexpr_range
// ===========================================================================

// constexpr_range
//   class: a compile-time range adapter over contiguous const storage.
// Provides constexpr_iterator-based iteration, indexed access, and size
// queries.
template<typename Type>
class constexpr_range
{
public:
    using value_type     = Type;
    using size_type      = std::size_t;
    using iterator       = constexpr_iterator<Type>;
    using const_iterator = constexpr_iterator<Type>;

    // the constexpr_iterator type itself, exposed as a member alias to
    // participate in has_constexpr_iterator_alias detection.
    using constexpr_iterator_type = constexpr_iterator<Type>;

    // --- construction ---

    D_CONSTEXPR
    constexpr_range() D_NOEXCEPT
        : m_data(nullptr),
          m_size(0)
    {}

    D_CONSTEXPR
    constexpr_range(
        const Type* _data,
        size_type    _count
    ) D_NOEXCEPT
        : m_data(_data),
          m_size(_count)
    {}

    // construct from any contiguous container with data() and size().
    template<typename Container>
    D_CONSTEXPR
    constexpr_range(
        const Container& _c
    ) D_NOEXCEPT
        : m_data(_c.data()),
          m_size(_c.size())
    {}

    // --- iteration ---

    D_CONSTEXPR iterator
    begin() const D_NOEXCEPT
    {
        return iterator(m_data);
    }

    D_CONSTEXPR iterator
    end() const D_NOEXCEPT
    {
        return iterator(m_data + m_size);
    }

    D_CONSTEXPR iterator
    constexpr_begin() const D_NOEXCEPT
    {
        return begin();
    }

    D_CONSTEXPR iterator
    constexpr_end() const D_NOEXCEPT
    {
        return end();
    }

    // --- access ---

    D_CONSTEXPR const Type&
    operator[](
        size_type _index
    ) const D_NOEXCEPT
    {
        return m_data[_index];
    }

    D_CONSTEXPR const Type&
    front() const D_NOEXCEPT
    {
        return m_data[0];
    }

    // back
    //   returns a const reference to the last element.
    D_CONSTEXPR const Type&
    back() const D_NOEXCEPT
    {
        return m_data[m_size - 1];
    }

    D_CONSTEXPR const Type*
    data() const D_NOEXCEPT
    {
        return m_data;
    }

    // --- capacity ---

    D_CONSTEXPR size_type
    size() const D_NOEXCEPT
    {
        return m_size;
    }

    D_CONSTEXPR bool
    empty() const D_NOEXCEPT
    {
        return (m_size == 0);
    }

private:
    const Type* m_data;
    size_type    m_size;
};


// ===========================================================================
// III. Factory Functions
// ===========================================================================

// make_constexpr_range
//   function: constructs a constexpr_range from a pointer and count.
template<typename Type>
D_CONSTEXPR constexpr_range<Type>
make_constexpr_range(
    const Type* _data,
    std::size_t  _count
) D_NOEXCEPT
{
    return constexpr_range<Type>(_data, _count);
}

// make_constexpr_range (container)
//   function: constructs a constexpr_range from any contiguous container with
// data() and size().
template<typename Container>
D_CONSTEXPR constexpr_range<typename Container::value_type>
make_constexpr_range(
    const Container& _c
) D_NOEXCEPT
{
    return constexpr_range<typename Container::value_type>(
        _c.data(),
        _c.size());
}

// make_constexpr_range (C array)
//   function: constructs a constexpr_range from a C array.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR constexpr_range<Type>
make_constexpr_range(
    const Type (&_arr)[N]
) D_NOEXCEPT
{
    return constexpr_range<Type>(_arr, N);
}


// ===========================================================================
// IV.  Compile-Time Algorithms
// ===========================================================================
// Minimal constexpr algorithm vocabulary operating on
// constexpr_range or any constexpr_iterator pair.

// constexpr_find
//   function: returns the first iterator where *it == _val, or _end if not
// found.
template<typename Iterator,
         typename Value>
D_INTERNAL_CXIT_CONSTEXPR Iterator
constexpr_find(
    Iterator      _begin,
    Iterator      _end,
    const Value& _val
)
{
    for (; _begin != _end; ++_begin)
    {
        // matched element found
        if (*_begin == _val)
        {
            return _begin;
        }
    }

    return _end;
}

// constexpr_find_if
//   function: returns the first iterator where _predicate(*it) is true, or
// _end if not found.
template<typename Iterator,
         typename Predicate>
D_INTERNAL_CXIT_CONSTEXPR Iterator
constexpr_find_if(
    Iterator   _begin,
    Iterator   _end,
    Predicate _predicate
)
{
    for (; _begin != _end; ++_begin)
    {
        // predicate satisfied
        if (_predicate(*_begin))
        {
            return _begin;
        }
    }

    return _end;
}

// constexpr_count
//   function: counts elements equal to _val.
template<typename Iterator,
         typename Value>
D_INTERNAL_CXIT_CONSTEXPR std::size_t
constexpr_count(
    Iterator      _begin,
    Iterator      _end,
    const Value& _val
)
{
    std::size_t n = 0;

    for (; _begin != _end; ++_begin)
    {
        // matched element
        if (*_begin == _val)
        {
            ++n;
        }
    }

    return n;
}

// constexpr_count_if
//   function: counts elements satisfying _predicate.
template<typename Iterator,
         typename Predicate>
D_INTERNAL_CXIT_CONSTEXPR std::size_t
constexpr_count_if(
    Iterator   _begin,
    Iterator   _end,
    Predicate _predicate
)
{
    std::size_t n = 0;

    for (; _begin != _end; ++_begin)
    {
        // predicate satisfied
        if (_predicate(*_begin))
        {
            ++n;
        }
    }

    return n;
}

// constexpr_all_of
//   function: true if _predicate(*it) holds for every element.
template<typename Iterator,
         typename Predicate>
D_INTERNAL_CXIT_CONSTEXPR bool
constexpr_all_of(
    Iterator   _begin,
    Iterator   _end,
    Predicate _predicate
)
{
    for (; _begin != _end; ++_begin)
    {
        // any failure short-circuits
        if (!_predicate(*_begin))
        {
            return false;
        }
    }

    return true;
}

// constexpr_any_of
//   function: true if _predicate(*it) holds for at least one element.
template<typename Iterator,
         typename Predicate>
D_INTERNAL_CXIT_CONSTEXPR bool
constexpr_any_of(
    Iterator   _begin,
    Iterator   _end,
    Predicate _predicate
)
{
    for (; _begin != _end; ++_begin)
    {
        // any success short-circuits
        if (_predicate(*_begin))
        {
            return true;
        }
    }

    return false;
}

// constexpr_none_of
//   function: true if _predicate(*it) is false for every element.
template<typename Iterator,
         typename Predicate>
D_INTERNAL_CXIT_CONSTEXPR bool
constexpr_none_of(
    Iterator   _begin,
    Iterator   _end,
    Predicate _predicate
)
{
    return !constexpr_any_of(_begin, _end, _predicate);
}

// constexpr_fold
//   function: left fold over [_begin, _end) with an initial accumulator value.
template<typename Iterator,
         typename Acc,
         typename Fn>
D_INTERNAL_CXIT_CONSTEXPR Acc
constexpr_fold(
    Iterator _begin,
    Iterator _end,
    Acc       _init,
    Fn        _fn
)
{
    for (; _begin != _end; ++_begin)
    {
        _init = _fn(_init, *_begin);
    }

    return _init;
}

// constexpr_accumulate
//   function: sums elements over [_begin, _end) starting from _init.
template<typename Iterator,
         typename Value>
D_INTERNAL_CXIT_CONSTEXPR Value
constexpr_accumulate(
    Iterator _begin,
    Iterator _end,
    Value     _init
)
{
    for (; _begin != _end; ++_begin)
    {
        _init = _init + *_begin;
    }

    return _init;
}

// constexpr_min_element
//   function: returns iterator to the smallest element, or _end if the range
// is empty.
template<typename Iterator>
D_INTERNAL_CXIT_CONSTEXPR Iterator
constexpr_min_element(
    Iterator _begin,
    Iterator _end
)
{
    // empty range guard
    if (_begin == _end)
    {
        return _end;
    }

    Iterator result = _begin;
    ++_begin;

    for (; _begin != _end; ++_begin)
    {
        // strict less keeps the first equal candidate
        if (*_begin < *result)
        {
            result = _begin;
        }
    }

    return result;
}

// constexpr_max_element
//   function: returns iterator to the largest element, or _end if the range is
// empty.
template<typename Iterator>
D_INTERNAL_CXIT_CONSTEXPR Iterator
constexpr_max_element(
    Iterator _begin,
    Iterator _end
)
{
    // empty range guard
    if (_begin == _end)
    {
        return _end;
    }

    Iterator result = _begin;
    ++_begin;

    for (; _begin != _end; ++_begin)
    {
        // strict less keeps the first equal candidate
        if (*result < *_begin)
        {
            result = _begin;
        }
    }

    return result;
}

// constexpr_equal
//   function: true if ranges [_a_begin, _a_end) and [_b_begin, ...) are
// element-wise equal. The second range is presumed to be at least as long.
template<typename IterA,
         typename IterB>
D_INTERNAL_CXIT_CONSTEXPR bool
constexpr_equal(
    IterA _a_begin,
    IterA _a_end,
    IterB _b_begin
)
{
    for (; _a_begin != _a_end; ++_a_begin, ++_b_begin)
    {
        // any inequality short-circuits
        if (!(*_a_begin == *_b_begin))
        {
            return false;
        }
    }

    return true;
}


NS_END  // djinterp


#undef D_INTERNAL_CXIT_CONSTEXPR

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ITERATOR_CONSTEXPR_ITERATOR_HPP
