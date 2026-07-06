/******************************************************************************
* djinterp [container]                                    ordered_container.hpp
*
*   The ORDER axis (the spec, Sortedness): whether a container has POSITIONAL
* IDENTITY.  A container is ORDERED iff a position function is part of its
* structure - it may be written as a finite sequence (e_1, ..., e_n) in which each
* position is meaningful, so a permutation of the elements is a DIFFERENT
* container.  It is UNORDERED iff no position function is part of its structure -
* two containers differing only by a permutation are equal; there is no meaningful
* first, second, or next element.
*
*   Note this is positional identity, NOT storage layout or presentation: an
* associative container (set, multiset, map) is UNORDERED here - its identity is
* its bag, permutations are equal - even though a comparator lets it be ENUMERATED
* in sorted order.  That monotone enumeration is the Sortedness axis's concern
* (sorted_container.hpp), not a positional order.  The distinguishing structural
* tell is thus a key_type: a keyed / associative container is unordered; an
* iterable without one is a positional sequence, and ordered.
*
*   This header upgrades the former sequential_container.hpp: it corrects the
* verdict (positional identity excludes ALL associatives, not only the hash-
* ordered ones), and provides the CRTP base of order-dependent operations and the
* free order algorithms for any ordered container.  Element ORDER being defined,
* these read positions without knowing the backing.  Sortedness - whether those
* positions are in comparator order - is layered on top in sorted_container.hpp.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language as the rest do.
*
* TABLE OF CONTENTS
* =================
* I.    Order-axis traits + sequential layout
* II.   ordered_base (CRTP order operations)
* III.  Free-function order algorithms
*
*
* path:      /inc/djinterp/core/container/ordered_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.24
*                                                          revised: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_ORDERED_
#define DJINTERP_CONTAINER_ORDERED_ 1

// std
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../meta/trait_detect.hpp"              // D_TYPE_TRAIT_* detection macros, D_VOID_T
#include "./traits/ordered_container_traits.hpp" // order-axis classifiers (canonical home)


NS_DJINTERP


// ===========================================================================
// I.   Order-axis traits + sequential layout
// ===========================================================================
//   The order-axis CLASSIFIERS - is_ordered_container / is_unordered_container,
// the ordering_kind enum + ordering_kind_of, and the sequential_layout enum +
// sequential_layout_of - are owned by ordered_container_traits.hpp (included
// above) and re-exported through it.  This file provides the order OPERATIONS
// (the ordered_base CRTP mixin and the free order algorithms) only.  Earlier
// revisions carried a verbatim copy of the classifiers here; the duplicates
// were removed to end the one-definition-rule conflict wherever both this file
// and the traits header met in a translation unit.


// ===========================================================================
// II.  ordered_base (CRTP order operations)
// ===========================================================================

//   Order-dependent operations for any ordered container.  The derived class
// exposes begin() / end() (and size() for the rotate / shift operations); this
// base never allocates or owns data.  Return types are written with trailing
// decltype so the operations remain C++11-clean.

template<typename _Derived>
class ordered_base
{
protected:
    ordered_base()  = default;
    ~ordered_base() = default;

public:
    // --- positional access ---

    // front
    //   returns a reference to the first element.
    template<typename _D = _Derived>
    auto front() const
        -> decltype(*std::begin(std::declval<const _D&>()))
    {
        return *std::begin(self());
    }

    // back
    //   returns a reference to the last element.
    template<typename _D = _Derived>
    auto back() const
        -> decltype(*std::begin(std::declval<const _D&>()))
    {
        auto _it = std::end(self());
        --_it;

        return *_it;
    }

    // nth
    //   returns an iterator to the _n-th element.
    template<typename _D = _Derived>
    auto nth(std::size_t _n) const
        -> decltype(std::begin(std::declval<const _D&>()))
    {
        auto _it = std::begin(self());
        std::advance(_it, _n);

        return _it;
    }

    // --- order predicates ---

    // is_sorted
    //   true iff the elements are in non-descending default order along their
    // positions - a checkable property of THIS container's contents.
    bool is_sorted() const
    {
        return std::is_sorted(std::begin(self()), std::end(self()));
    }

    // is_sorted (custom comparator)
    template<typename _Compare>
    bool is_sorted(_Compare _cmp) const
    {
        return std::is_sorted(std::begin(self()), std::end(self()), _cmp);
    }

    // is_palindrome
    //   true iff the sequence reads the same forwards and backwards.
    bool is_palindrome() const
    {
        auto _fwd = std::begin(self());
        auto _rev = std::end(self());

        // an empty sequence is trivially a palindrome.
        if (_fwd == _rev)
        {
            return true;
        }

        --_rev;

        while (_fwd != _rev)
        {
            if (!(*_fwd == *_rev))
            {
                return false;
            }

            ++_fwd;

            if (_fwd == _rev)
            {
                break;
            }

            --_rev;
        }

        return true;
    }

    // --- mutating order operations (in place on the derived container) ---

    // reverse
    //   reverses the element order.
    void reverse()
    {
        std::reverse(std::begin(self()), std::end(self()));

        return;
    }

    // rotate_left
    //   rotates the elements left by _n positions.
    void rotate_left(std::size_t _n)
    {
        auto _mid = std::begin(self());
        std::advance(_mid, _n % self().size());

        std::rotate(std::begin(self()), _mid, std::end(self()));

        return;
    }

    // rotate_right
    //   rotates the elements right by _n positions.
    void rotate_right(std::size_t _n)
    {
        std::size_t _sz = self().size();

        // nothing to rotate in an empty sequence.
        if (_sz == 0)
        {
            return;
        }

        rotate_left(_sz - (_n % _sz));

        return;
    }

    // shift_left
    //   shifts the elements left by _n, filling vacated positions with _fill.
    template<typename _Value>
    void shift_left(
        std::size_t   _n,
        const _Value& _fill
    )
    {
        std::size_t _sz = self().size();

        // a shift at or beyond the size clears to fill.
        if (_n >= _sz)
        {
            std::fill(std::begin(self()), std::end(self()), _fill);

            return;
        }

        auto _src = std::begin(self());
        std::advance(_src, _n);
        std::move(_src, std::end(self()), std::begin(self()));

        auto _tail = std::end(self());
        std::advance(_tail, -static_cast<std::ptrdiff_t>(_n));
        std::fill(_tail, std::end(self()), _fill);

        return;
    }

    // shift_right
    //   shifts the elements right by _n, filling vacated positions with _fill.
    template<typename _Value>
    void shift_right(
        std::size_t   _n,
        const _Value& _fill
    )
    {
        std::size_t _sz = self().size();

        // a shift at or beyond the size clears to fill.
        if (_n >= _sz)
        {
            std::fill(std::begin(self()), std::end(self()), _fill);

            return;
        }

        auto _src = std::end(self());
        std::advance(_src, -static_cast<std::ptrdiff_t>(_n));
        std::move_backward(std::begin(self()), _src, std::end(self()));

        auto _head = std::begin(self());
        std::advance(_head, _n);
        std::fill(std::begin(self()), _head, _fill);

        return;
    }

private:
    // self
    //   the derived reference, via the CRTP downcast.
    _Derived&
    self()
    {
        return static_cast<_Derived&>(*this);
    }

    const _Derived&
    self() const
    {
        return static_cast<const _Derived&>(*this);
    }
};


// ===========================================================================
// III. Free-function order algorithms
// ===========================================================================

// starts_with
//   function: true iff _container begins with _prefix.
template<typename _Container,
         typename _Prefix>
typename std::enable_if<
        is_ordered_container<_Container>::value
     && is_ordered_container<_Prefix>::value,
    bool
>::type
starts_with(
    const _Container& _container,
    const _Prefix&    _prefix
)
{
    auto _c_it  = std::begin(_container);
    auto _c_end = std::end(_container);
    auto _p_it  = std::begin(_prefix);
    auto _p_end = std::end(_prefix);

    // walk the prefix, matching element-wise.
    for (; _p_it != _p_end; ++_c_it, ++_p_it)
    {
        if (_c_it == _c_end || !(*_c_it == *_p_it))
        {
            return false;
        }
    }

    return true;
}

// ends_with
//   function: true iff _container ends with _suffix.
template<typename _Container,
         typename _Suffix>
typename std::enable_if<
        is_ordered_container<_Container>::value
     && is_ordered_container<_Suffix>::value,
    bool
>::type
ends_with(
    const _Container& _container,
    const _Suffix&    _suffix
)
{
    auto _c_sz = _container.size();
    auto _s_sz = _suffix.size();

    // a longer suffix cannot fit.
    if (_s_sz > _c_sz)
    {
        return false;
    }

    auto _c_it = std::begin(_container);
    std::advance(_c_it, _c_sz - _s_sz);

    auto _s_it = std::begin(_suffix);

    // match the aligned tail element-wise.
    for (; _s_it != std::end(_suffix); ++_c_it, ++_s_it)
    {
        if (!(*_c_it == *_s_it))
        {
            return false;
        }
    }

    return true;
}

// contains_subsequence
//   function: true iff _sub appears as a contiguous subsequence of _container.
template<typename _Container,
         typename _Sub>
typename std::enable_if<
        is_ordered_container<_Container>::value
     && is_ordered_container<_Sub>::value,
    bool
>::type
contains_subsequence(
    const _Container& _container,
    const _Sub&       _sub
)
{
    return ( std::search(std::begin(_container), std::end(_container),
                         std::begin(_sub),       std::end(_sub))
             != std::end(_container) );
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_ORDERED_
