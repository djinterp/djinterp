/******************************************************************************
* djinterp [container]                                        flat_iterator.hpp
*
*   The foundational FLAT iterator: a positional traversal over a flat container's
* leaf positions (structure depth 1).  It is a random-access iterator over
* contiguous storage, and it realises the two independent iterability axes (the
* spec, Iterability) directly:
*
*     STAGE.   Every observing and every FUNCTIONAL operation (dereference,
*              comparison, and the pure `next` / `prev` / `operator+` that return a
*              NEW iterator) is D_CONSTEXPR, so a traversal runs at compile time
*              wherever the storage is statically addressable, and at runtime
*              otherwise.  Compile-time iteration is thus functional: advancing
*              yields a fresh iterator rather than mutating one in place, matching
*              the spec's compile-time non-const = functional-update reading.  The
*              in-place mutators (++, --, +=, -=) are the runtime path; they become
*              constexpr as well from C++14, where a constexpr function may mutate.
*     MODE.    Constness is a type parameter.  A non-const iterator grants a
*              settable reference (_Type&) - it may replace the value at a position,
*              the position set unchanged - while a const iterator grants only a
*              read-only reference (const _Type&).  A non-const iterator converts to
*              its const counterpart, never the reverse.
*
*   PORTABILITY:
*   C++11 baseline.  Functional and observing operations are constexpr throughout;
* the in-place mutators are constexpr from C++14 (relaxed constexpr).
*
*
* path:      /inc/djinterp/core/container/iterator/flat_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_FLAT_ITERATOR_
#define DJINTERP_CONTAINER_FLAT_ITERATOR_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"   // D_CONSTEXPR, NS_*, feature macros


// D_ITER_CONSTEXPR_MUT
//   an in-place iterator mutator is constexpr only where a constexpr function may
// mutate - C++14 (relaxed constexpr) onward; before that it is a runtime operation.
#ifndef D_ITER_CONSTEXPR_MUT
    #if ( D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL >= 201304L )
        #define D_ITER_CONSTEXPR_MUT  constexpr
    #else
        #define D_ITER_CONSTEXPR_MUT
    #endif
#endif


NS_DJINTERP


// flat_iterator
//   class: a random-access iterator over a flat container's contiguous storage.
// Parameterised on the element type and on constness; the latter fixes whether the
// per-position access is settable.
template<typename _Type,
         bool     _Const = false>
class flat_iterator
{
public:
    using value_type        = _Type;
    using difference_type   = std::ptrdiff_t;
    using size_type         = std::size_t;
    using iterator_category = std::random_access_iterator_tag;

    using pointer =
        typename std::conditional<_Const, const _Type*, _Type*>::type;
    using reference =
        typename std::conditional<_Const, const _Type&, _Type&>::type;

    // ------------------------------------------------------------------
    //  construction
    // ------------------------------------------------------------------

    // flat_iterator (default)
    //   a singular iterator addressing nothing.
    constexpr flat_iterator() noexcept
        : m_ptr(nullptr)
    {}

    // flat_iterator (pointer)
    //   an iterator addressing the position at _ptr.
    constexpr explicit flat_iterator(pointer _ptr) noexcept
        : m_ptr(_ptr)
    {}

    // flat_iterator (const conversion)
    //   a non-const iterator converts to its const counterpart (never the
    // reverse).  Enabled only when THIS is the const flavour, taking the non-const.
    template<bool _C = _Const,
             typename = typename std::enable_if<_C>::type>
    constexpr flat_iterator(const flat_iterator<_Type, false>& _other) noexcept
        : m_ptr(_other.raw())
    {}

    // ------------------------------------------------------------------
    //  access (observing - constexpr throughout)
    // ------------------------------------------------------------------

    // operator*
    //   the reference at the current position (settable when non-const).
    constexpr reference operator*() const noexcept
    {
        return *m_ptr;
    }

    // operator->
    //   a pointer to the current position.
    constexpr pointer operator->() const noexcept
    {
        return m_ptr;
    }

    // operator[]
    //   the reference _n positions away.
    constexpr reference operator[](difference_type _n) const noexcept
    {
        return m_ptr[_n];
    }

    // raw
    //   the underlying pointer (exposed for the const conversion above).
    constexpr pointer raw() const noexcept
    {
        return m_ptr;
    }

    // ------------------------------------------------------------------
    //  functional traversal (returns a NEW iterator - constexpr in C++11)
    // ------------------------------------------------------------------

    // next
    //   the iterator one position forward.
    constexpr flat_iterator next() const noexcept
    {
        return flat_iterator(m_ptr + 1);
    }

    // prev
    //   the iterator one position back.
    constexpr flat_iterator prev() const noexcept
    {
        return flat_iterator(m_ptr - 1);
    }

    // advanced
    //   the iterator _n positions away.
    constexpr flat_iterator advanced(difference_type _n) const noexcept
    {
        return flat_iterator(m_ptr + _n);
    }

    // operator+ / operator-
    constexpr flat_iterator operator+(difference_type _n) const noexcept
    {
        return flat_iterator(m_ptr + _n);
    }

    constexpr flat_iterator operator-(difference_type _n) const noexcept
    {
        return flat_iterator(m_ptr - _n);
    }

    // operator- (distance)
    //   the number of positions from _other to this.
    constexpr difference_type
    operator-(const flat_iterator& _other) const noexcept
    {
        return m_ptr - _other.m_ptr;
    }

    // ------------------------------------------------------------------
    //  in-place traversal (runtime; constexpr from C++14)
    // ------------------------------------------------------------------

    // operator++ (pre / post)
    D_ITER_CONSTEXPR_MUT flat_iterator& operator++() noexcept
    {
        ++m_ptr;

        return *this;
    }

    D_ITER_CONSTEXPR_MUT flat_iterator operator++(int) noexcept
    {
        flat_iterator _tmp(*this);
        ++m_ptr;

        return _tmp;
    }

    // operator-- (pre / post)
    D_ITER_CONSTEXPR_MUT flat_iterator& operator--() noexcept
    {
        --m_ptr;

        return *this;
    }

    D_ITER_CONSTEXPR_MUT flat_iterator operator--(int) noexcept
    {
        flat_iterator _tmp(*this);
        --m_ptr;

        return _tmp;
    }

    // operator+= / operator-=
    D_ITER_CONSTEXPR_MUT flat_iterator& operator+=(difference_type _n) noexcept
    {
        m_ptr += _n;

        return *this;
    }

    D_ITER_CONSTEXPR_MUT flat_iterator& operator-=(difference_type _n) noexcept
    {
        m_ptr -= _n;

        return *this;
    }

    // ------------------------------------------------------------------
    //  comparison (constexpr throughout)
    // ------------------------------------------------------------------

    constexpr bool operator==(const flat_iterator& _o) const noexcept
    {
        return m_ptr == _o.m_ptr;
    }

    constexpr bool operator!=(const flat_iterator& _o) const noexcept
    {
        return m_ptr != _o.m_ptr;
    }

    constexpr bool operator<(const flat_iterator& _o) const noexcept
    {
        return m_ptr < _o.m_ptr;
    }

    constexpr bool operator>(const flat_iterator& _o) const noexcept
    {
        return m_ptr > _o.m_ptr;
    }

    constexpr bool operator<=(const flat_iterator& _o) const noexcept
    {
        return m_ptr <= _o.m_ptr;
    }

    constexpr bool operator>=(const flat_iterator& _o) const noexcept
    {
        return m_ptr >= _o.m_ptr;
    }

private:
    pointer m_ptr;
};


// operator+ (n + it)
//   the symmetric scalar-plus-iterator form.
template<typename _Type,
         bool     _Const>
constexpr flat_iterator<_Type, _Const>
operator+(
    typename flat_iterator<_Type, _Const>::difference_type _n,
    const flat_iterator<_Type, _Const>&                    _it
) noexcept
{
    return _it + _n;
}


// ===========================================================================
//  factories
// ===========================================================================

// make_flat_iterator
//   factory: a non-const flat iterator at _ptr.
template<typename _Type>
constexpr flat_iterator<_Type, false>
make_flat_iterator(_Type* _ptr) noexcept
{
    return flat_iterator<_Type, false>(_ptr);
}

// make_const_flat_iterator
//   factory: a const flat iterator at _ptr.
template<typename _Type>
constexpr flat_iterator<_Type, true>
make_const_flat_iterator(const _Type* _ptr) noexcept
{
    return flat_iterator<_Type, true>(_ptr);
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_FLAT_ITERATOR_
