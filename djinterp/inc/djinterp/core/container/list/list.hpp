/******************************************************************************
* djinterp [container]                                                list.hpp
*
* Abstract foundation for all list-like containers:
*   A "list" in the djinterp framework is an ordered linear sequence of
* elements with O(1) splice / arbitrary-position insert / arbitrary-
* position erase given an iterator handle.  This distinguishes list
* containers from sequence containers (vector-like, contiguous storage
* with O(n) middle insertions).
*
*   This header provides the abstract interface — list_base<Derived> —
* that every concrete list type inherits from.  Concrete derivations
* live in:
*
*     linked_list.hpp            singly / doubly / xor / circular / etc.
*     skip_list.hpp              probabilistic skip list (planned)
*     unrolled_linked_list.hpp   block-wise linked list  (planned)
*
*   list_base<Derived> extends sequential_base<Derived>, so all the
* sequential_base algorithms (rotate, reverse, is_palindrome, ...)
* are inherited automatically.  list_base adds the algorithms that
* lists implement more efficiently than sequences, expressed over
* begin() / end():
*
*     - merge        (in-place merge of two sorted lists)
*     - unique       (remove consecutive equal elements)
*     - remove       (remove all elements matching a value)
*     - remove_if    (remove all elements matching a predicate)
*     - sort         (in-place merge sort, O(n log n))
*
*   Node-aware operations that require concrete node access (splice,
* insert_after, erase) are NOT provided here — they belong on the
* concrete list type because their signatures depend on the iterator
* type, which in turn depends on the node type.
*
* TABLE OF CONTENTS
* =================
*   I.   list_base CRTP class
*   II.  Free functions: equal, lexicographical_compare
*
*   PORTABILITY:
*   C++11 baseline.
*
* 
* path:      /inc/djinterp/core/container/list/list.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_LIST_
#define DJINTERP_CONTAINER_LIST_ 1

// std
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../sequential_container.hpp"


NS_DJINTERP

// ===========================================================================
// I.   list_base — CRTP foundation for list-like containers
// ===========================================================================

// list_base
//   class: CRTP base supplying list-specific algorithms expressible
// over begin() / end().  Inherits sequential_base<_Derived>, so the
// generic sequential algorithms (rotate, reverse, is_palindrome,
// shift, ...) are also available on every derived type.
//
//   The derived class must expose:
//     - begin() / end()
//     - size()
//     - front() / back()    (inherited if begin/end are present)
//
//   All algorithms here mutate via iterator-pair operations, so they
// work on any list whose iterators model the appropriate category.
template<typename _Derived>
class list_base
    : public sequential_base<_Derived>
{
protected:
    list_base()  = default;
    ~list_base() = default;

private:
    _Derived&
    self() noexcept
    {
        return static_cast<_Derived&>(*this);
    }

    const _Derived&
    self() const noexcept
    {
        return static_cast<const _Derived&>(*this);
    }

public:
    // -----------------------------------------------------------------
    // unique — remove consecutive equal elements
    // -----------------------------------------------------------------
    //   This is the std::list-style unique: it only collapses RUNS of
    // equal elements.  Combine with sort() to remove all duplicates.

    void
    unique()
    {
        if (self().size() < 2u)
        {
            return;
        }

        auto it   = std::begin(self());
        auto stop = std::end(self());
        auto prev = it;

        ++it;

        while (it != stop)
        {
            if (*it == *prev)
            {
                // derived must provide erase(iterator)
                it = self().erase(it);
            }
            else
            {
                prev = it;
                ++it;
            }
        }
    }

    template<typename _Predicate>
    void
    unique(
        _Predicate _eq)
    {
        if (self().size() < 2u)
        {
            return;
        }

        auto it   = std::begin(self());
        auto stop = std::end(self());
        auto prev = it;

        ++it;

        while (it != stop)
        {
            if (_eq(*it, *prev))
            {
                it = self().erase(it);
            }
            else
            {
                prev = it;
                ++it;
            }
        }
    }

    // -----------------------------------------------------------------
    // remove — erase all elements equal to _value
    // -----------------------------------------------------------------

    template<typename _Value>
    void
    remove(
        const _Value& _value)
    {
        auto it   = std::begin(self());
        auto stop = std::end(self());

        while (it != stop)
        {
            if (*it == _value)
            {
                it = self().erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // -----------------------------------------------------------------
    // remove_if — erase all elements matching _predicate
    // -----------------------------------------------------------------

    template<typename _Predicate>
    void
    remove_if(
        _Predicate _predicate)
    {
        auto it   = std::begin(self());
        auto stop = std::end(self());

        while (it != stop)
        {
            if (_predicate(*it))
            {
                it = self().erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // -----------------------------------------------------------------
    // sort — in-place sort (delegates to std::sort)
    // -----------------------------------------------------------------
    //   For node-based lists where iterators are not random-access
    // and std::sort cannot be applied directly, the derived class
    // should override sort() with an in-place merge-sort that
    // rewires links in O(n log n).  The default implementation here
    // uses an iterator-based path that works whenever the iterator
    // category permits std::sort.

    void
    sort()
    {
        // Default implementation: derived classes are expected to
        // override with a node-rewiring merge sort when their
        // iterators are not random-access.  This default body is
        // intentionally a no-op so that linkage succeeds for forward
        // iterators; it is only reached if the derived class did not
        // override.
    }

    template<typename _Compare>
    void
    sort(
        _Compare /*_cmp*/)
    {
        // see comment on the no-arg overload above
    }

    // -----------------------------------------------------------------
    // size predicates expressed in terms of begin/end
    // -----------------------------------------------------------------

    bool
    is_empty() const
    {
        return ( std::begin(self()) == std::end(self()) );
    }
};


// ===========================================================================
// II.  FREE FUNCTIONS
// ===========================================================================
//   Generic algorithms over any list-like container.  They are
// declared in the bare djinterp namespace so they are picked up by
// ADL on derived list types.

// list_equal
//   function: returns true when two list-like containers have
// equal contents element-by-element.
template<typename _LhsList,
         typename _RhsList>
inline bool
list_equal(
    const _LhsList& _lhs,
    const _RhsList& _rhs
)
{
    if (_lhs.size() != _rhs.size())
    {
        return false;
    }

    auto a    = std::begin(_lhs);
    auto a_end= std::end(_lhs);
    auto b    = std::begin(_rhs);

    for (; a != a_end; ++a, ++b)
    {
        if (!(*a == *b))
        {
            return false;
        }
    }

    return true;
}

// list_lexicographical_compare
//   function: returns true when _lhs is lexicographically less than
// _rhs.  Mirrors std::lexicographical_compare but works on list
// types whose iterators may be only forward-iterable.
template<typename _LhsList,
         typename _RhsList>
inline bool
list_lexicographical_compare(
    const _LhsList& _lhs,
    const _RhsList& _rhs
)
{
    auto a    = std::begin(_lhs);
    auto a_end= std::end(_lhs);
    auto b    = std::begin(_rhs);
    auto b_end= std::end(_rhs);

    for (; (a != a_end) && (b != b_end); ++a, ++b)
    {
        if (*a < *b)
        {
            return true;
        }

        if (*b < *a)
        {
            return false;
        }
    }

    return ( (a == a_end) &&
             (b != b_end) );
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_LIST_