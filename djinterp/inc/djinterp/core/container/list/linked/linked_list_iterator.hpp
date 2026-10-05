/*******************************************************************************
* djinterp [core]                                       linked_list_iterator.hpp
*
* Versatile iterator family for every permutation of linked-list node:
*   This header provides a single iterator class template,
* linked_list_iterator, that adapts itself - through node-trait
* introspection - to the topology of the underlying node:
*     - singly-linked : forward_iterator_tag
*     - doubly-linked : bidirectional_iterator_tag
*     - xor-linked    : bidirectional_iterator_tag (with prev state)
*     - skip-list     : forward_iterator_tag at level 0
*   It also handles the cross-cutting axes:
*     - linear vs circular            (end-detection rule)
*     - sentinel vs no-sentinel       (end-pointer comparison)
*     - mutable vs const              (template IsConst flag)
*     - forward vs reverse            (template IsReverse flag)
*   The iterator is non-owning: it merely traverses an external node
* graph through the link members detected by linked_list_traits.hpp.
* Iterator validity follows the standard linked-list rules - only the
* iterator pointing at an erased node is invalidated, all others
* remain stable.
*   For range-based-for support, ranges and adaptors are provided as
* free functions:
*     for (auto& x : a_linked_list)              // standard
*     for (auto& x : reversed(a_linked_list))    // reverse adaptor
*
* PORTABILITY:
*   C++11 baseline.  The file uses a tag-dispatch pattern internally
* rather than `if constexpr` so it works on every supported standard.
*
*
* path:      /inc/djinterp/core/container/list/linked/linked_list_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.    iterator-category dispatch (next/prev advancement)
      --------------------------------------------------

2.    linked_list_iterator class template
      -----------------------------------

3.    convenience aliases (const, reverse, etc.)
      ------------------------------------------

4.    range adaptor (reversed)
      ------------------------

5.    free constructors (begin/end helpers)
      -------------------------------------
*/

#ifndef DJINTERP_CONTAINER_LIST_LINKED_LINKED_LIST_ITERATOR_HPP
#define DJINTERP_CONTAINER_LIST_LINKED_LINKED_LIST_ITERATOR_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
// djinterp
#include "../../../../djinterp.hpp"
#include "./linked_list_traits.hpp"
// re_std
#include "../../../../../re_std/cstdint/cstdint.hpp"  // re_std::uintptr_t


NS_DJINTERP


// ===========================================================================
// 1.   ITERATOR-CATEGORY DISPATCH
// ===========================================================================

NS_INTERNAL

    // ll_iterator_category
    //   trait: derives the standard iterator-category tag from the node shape
    // detected by linked_list_traits.hpp. Doubly- and XOR-linked nodes yield
    // bidirectional; everything else yields forward.
    template<typename Node,
             bool     Bi = ( is_doubly_linked_node<Node>::value ||
                              is_xor_linked_node<Node>::value )>
    struct ll_iterator_category
    {
        using type = std::forward_iterator_tag;
    };

    // ll_iterator_category<Node, true>
    //   helper: the case where `( is_doubly_linked_node<Node>::value ||
    // is_xor_linked_node<Node>::value )` is true; it maps to
    // `std::bidirectional_iterator_tag`.
    template<typename Node>
    struct ll_iterator_category<Node, true>
    {
        using type = std::bidirectional_iterator_tag;
    };

    // ll_payload_type
    //   trait: deduces the payload type carried by Node. Prefers a `value`
    // member over a `data()` accessor.
    template<typename Node,
             bool     HasValue = internal::has_node_value_helper<Node>::value>
    struct ll_payload_type
    {
        using type = typename std::remove_reference<
            decltype(std::declval<Node&>().data())>::type;
    };

    // ll_payload_type<Node, true>
    //   helper: the case where `internal::has_node_value_helper<Node>::value`
    // is true; it maps to `typename std::remove_reference<
    // decltype(std::declval<Node&>().value)>::type`.
    template<typename Node>
    struct ll_payload_type<Node, true>
    {
        using type = typename std::remove_reference<
            decltype(std::declval<Node&>().value)>::type;
    };

    // ll_payload_ref
    //   helper: returns a reference to the payload through whichever accessor
    // Node exposes. Dispatches at compile time via the HasValue tag.
    template<typename Node>
    inline auto
    ll_payload_ref(
        Node&     _n,
        std::true_type  /* has .value */
    ) noexcept -> decltype(_n.value)&
    {
        return _n.value;
    }

    template<typename Node>
    inline auto
    ll_payload_ref(
        Node&     _n,
        std::false_type /* uses .data() */
    ) noexcept -> decltype(_n.data())&
    {
        return _n.data();
    }


// ===========================================================================
// ADVANCEMENT POLICIES - internal tag-dispatched advance / retreat
// ===========================================================================

    // advance_singly
    //   helper: forward step for a singly-linked node.
    template<typename Node>
    inline Node*
    advance_singly(
        Node*  _curr,
        Node*  /* _prev unused */
    ) noexcept
    {
        return (_curr != nullptr) ? _curr->next : nullptr;
    }

    // advance_doubly_forward
    //   helper: forward step for a doubly-linked node.
    template<typename Node>
    inline Node*
    advance_doubly_forward(
        Node* _curr
    ) noexcept
    {
        return (_curr != nullptr) ? _curr->next : nullptr;
    }

    // advance_doubly_backward
    //   helper: backward step for a doubly-linked node.
    template<typename Node>
    inline Node*
    advance_doubly_backward(
        Node* _curr
    ) noexcept
    {
        return (_curr != nullptr) ? _curr->prev : nullptr;
    }

    // advance_xor
    //   helper: forward (or backward - same code) step for an XOR- linked
    // node. XOR-linked traversal needs to know the previous node so that next
    // == link XOR prev.
    template<typename Node>
    inline Node*
    advance_xor(
        Node* _curr,
        Node* _prev
    ) noexcept
    {
        if (_curr == nullptr)
        {
            return nullptr;
        }

        re_std::uintptr_t prev_bits =
            reinterpret_cast<re_std::uintptr_t>(_prev);
        re_std::uintptr_t link_bits =
            static_cast<re_std::uintptr_t>(_curr->link);

        return reinterpret_cast<Node*>(prev_bits ^ link_bits);
    }

NS_END  // internal


// ===========================================================================
// 2.  linked_list_iterator
// ===========================================================================

// linked_list_iterator
//   class: a non-owning forward / bidirectional iterator over a linked-list
// node graph. Adapts to the underlying node shape:
//
//   Node - the concrete node type (must satisfy
//                 is_linked_list_node)
//   IsConst - when true, dereferences yield const references
//   IsReverse - when true, ++ moves toward the head; only valid
//                 when the node shape supports backward traversal
//                 (doubly- or xor-linked).
//
//   Comparison is performed by raw pointer equality on the current node. An
// end-iterator is constructed with m_current == nullptr. Sentinel-based lists
// pass the address of the sentinel node as the end value.
template<typename Node,
         bool     IsConst    = false,
         bool     IsReverse = false>
class linked_list_iterator
{
    // Reverse iteration is only meaningful when the underlying node shape
    // supports backward traversal. Without this guard a user
    // could construct linked_list_iterator<SinglyNode, false, true> and have
    // ++ silently no-op via the catch-all step helper. Failing fast at
    // instantiation is preferable.
    static_assert(
        ( !IsReverse                                    ||
           is_doubly_linked_node<Node>::value          ||
           is_xor_linked_node<Node>::value ),
        "reverse_linked_list_iterator requires a "
        "bidirectional node shape (doubly- or xor-linked).");

public:
    // -----------------------------------------------------------------
    // standard iterator type aliases
    // -----------------------------------------------------------------
    using iterator_category =
        typename internal::ll_iterator_category<Node>::type;
    using value_type        =
        typename internal::ll_payload_type<Node>::type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = typename std::conditional<
                                  IsConst,
                                  const value_type*,
                                  value_type*>::type;
    using reference         = typename std::conditional<
                                  IsConst,
                                  const value_type&,
                                  value_type&>::type;
    using node_type         = Node;
    using node_pointer      = typename std::conditional<
                                  IsConst,
                                  const Node*,
                                  Node*>::type;

    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    // linked_list_iterator
    //   constructor: end-iterator (null current).
    constexpr linked_list_iterator() noexcept
        : m_current(nullptr),
          m_prev(nullptr),
          m_origin(nullptr)
    {}

    // linked_list_iterator
    //   constructor: from a starting node. Used by begin() / end() factories.
    // _origin is non-null for circular lists; the iterator considers itself
    // "at end" when it returns to the origin after at least one advance.
    explicit
    linked_list_iterator(
        node_pointer _start,
        node_pointer _prev   = nullptr,
        node_pointer _origin = nullptr
    ) noexcept
        : m_current(_start),
          m_prev(_prev),
          m_origin(_origin)
    {}

    // -----------------------------------------------------------------
    // dereference
    // -----------------------------------------------------------------

    reference
    operator*() const noexcept
    {
        return internal::ll_payload_ref(
            *const_cast<Node*>(m_current),
            djinterp::bool_constant<
                internal::has_node_value_helper<Node>::value>{});
    }

    pointer
    operator->() const noexcept
    {
        return &(operator*());
    }

    // -----------------------------------------------------------------
    // forward advance - operator++
    // -----------------------------------------------------------------

    linked_list_iterator&
    operator++() noexcept
    {
        m_step_advance(
            djinterp::bool_constant<IsReverse>{},
            ll_node_shape{});

        return *this;
    }

    linked_list_iterator
    operator++(int) noexcept
    {
        linked_list_iterator tmp = *this;

        ++(*this);

        return tmp;
    }

    // -----------------------------------------------------------------
    // backward advance - operator-- (only when the node supports it)
    // -----------------------------------------------------------------

    template<typename N = Node,
             typename = typename std::enable_if<
                 ( is_doubly_linked_node<N>::value ||
                   is_xor_linked_node<N>::value )>::type>
    linked_list_iterator&
    operator--() noexcept
    {
        m_step_retreat(
            djinterp::bool_constant<IsReverse>{},
            ll_node_shape{});

        return *this;
    }

    template<typename N = Node,
             typename = typename std::enable_if<
                 ( is_doubly_linked_node<N>::value ||
                   is_xor_linked_node<N>::value )>::type>
    linked_list_iterator
    operator--(int) noexcept
    {
        linked_list_iterator tmp = *this;

        --(*this);

        return tmp;
    }

    // -----------------------------------------------------------------
    // comparison
    // -----------------------------------------------------------------

    friend bool
    operator==(
        const linked_list_iterator& _a,
        const linked_list_iterator& _b
    ) noexcept
    {
        return (_a.m_current == _b.m_current);
    }

    friend bool
    operator!=(
        const linked_list_iterator& _a,
        const linked_list_iterator& _b
    ) noexcept
    {
        return (_a.m_current != _b.m_current);
    }

    // -----------------------------------------------------------------
    // raw node access (used by container implementations during
    // splice / erase to recover the underlying node from an iterator)
    // -----------------------------------------------------------------

    node_pointer
    node() const noexcept
    {
        return m_current;
    }

    node_pointer
    prev_node() const noexcept
    {
        return m_prev;
    }

private:
    // internal node-shape tag - selects the right step helper at compile time
    // without requiring `if constexpr`.
    enum ll_shape
    {
        shape_singly,
        shape_doubly,
        shape_xor,
        shape_skip
    };

    static constexpr ll_shape
    pick_shape() noexcept
    {
        return is_xor_linked_node<Node>::value
                 ? shape_xor
             : is_doubly_linked_node<Node>::value
                 ? shape_doubly
             : is_skip_list_node<Node>::value
                 ? shape_skip
                 : shape_singly;
    }

    using ll_node_shape = std::integral_constant<ll_shape, pick_shape()>;

    // -----------------------------------------------------------------
    // step helpers - singly
    // -----------------------------------------------------------------

    void
    m_step_advance(
        std::false_type /* not reverse */,
        std::integral_constant<ll_shape, shape_singly>
    ) noexcept
    {
        if (m_current != nullptr)
        {
            node_pointer nxt = m_current->next;

            // circular detection: returning to origin signals end
            if ( m_origin != nullptr &&
                 nxt      == m_origin )
            {
                m_prev    = m_current;
                m_current = nullptr;
            }
            else
            {
                m_prev    = m_current;
                m_current = nxt;
            }
        }
    }

    // -----------------------------------------------------------------
    // step helpers - doubly
    // -----------------------------------------------------------------

    void
    m_step_advance(
        std::false_type /* not reverse */,
        std::integral_constant<ll_shape, shape_doubly>
    ) noexcept
    {
        if (m_current != nullptr)
        {
            node_pointer nxt = m_current->next;

            if ( m_origin != nullptr &&
                 nxt      == m_origin )
            {
                m_prev    = m_current;
                m_current = nullptr;
            }
            else
            {
                m_prev    = m_current;
                m_current = nxt;
            }
        }
    }

    void
    m_step_advance(
        std::true_type /* reverse */,
        std::integral_constant<ll_shape, shape_doubly>
    ) noexcept
    {
        if (m_current != nullptr)
        {
            node_pointer prv = m_current->prev;

            if ( m_origin != nullptr &&
                 prv      == m_origin )
            {
                m_current = nullptr;
            }
            else
            {
                m_current = prv;
            }
        }
    }

    void
    m_step_retreat(
        std::false_type /* not reverse */,
        std::integral_constant<ll_shape, shape_doubly>
    ) noexcept
    {
        if (m_current != nullptr)
        {
            m_current = m_current->prev;
        }
    }

    void
    m_step_retreat(
        std::true_type /* reverse */,
        std::integral_constant<ll_shape, shape_doubly>
    ) noexcept
    {
        if (m_current != nullptr)
        {
            m_current = m_current->next;
        }
    }

    // -----------------------------------------------------------------
    // step helpers - xor-linked
    // -----------------------------------------------------------------

    void
    m_step_advance(
        std::false_type /* not reverse */,
        std::integral_constant<ll_shape, shape_xor>
    ) noexcept
    {
        node_pointer nxt = internal::advance_xor(m_current, m_prev);

        if ( m_origin != nullptr &&
             nxt      == m_origin )
        {
            m_prev    = m_current;
            m_current = nullptr;
        }
        else
        {
            m_prev    = m_current;
            m_current = nxt;
        }
    }

    void
    m_step_advance(
        std::true_type /* reverse */,
        std::integral_constant<ll_shape, shape_xor>
    ) noexcept
    {
        // For reverse iteration on an XOR list we keep m_prev as the node
        // "ahead" of m_current and step backward by treating that node as the
        // prev for the XOR computation.
        node_pointer prv = internal::advance_xor(m_current, m_prev);

        m_prev    = m_current;
        m_current = prv;
    }

    void
    m_step_retreat(
        std::false_type /* not reverse */,
        std::integral_constant<ll_shape, shape_xor>
    ) noexcept
    {
        // operator-- on a forward XOR iterator: same as m_step_advance with
        // the "reverse" tag.
        m_step_advance(std::true_type{},
                       std::integral_constant<ll_shape, shape_xor>{});
    }

    void
    m_step_retreat(
        std::true_type /* reverse */,
        std::integral_constant<ll_shape, shape_xor>
    ) noexcept
    {
        m_step_advance(std::false_type{},
                       std::integral_constant<ll_shape, shape_xor>{});
    }

    // -----------------------------------------------------------------
    // step helpers - skip list (forward at level 0)
    // -----------------------------------------------------------------

    void
    m_step_advance(
        std::false_type /* not reverse */,
        std::integral_constant<ll_shape, shape_skip>
    ) noexcept
    {
        if (m_current != nullptr)
        {
            node_pointer nxt = m_current->forwards[0];

            if ( m_origin != nullptr &&
                 nxt      == m_origin )
            {
                m_prev    = m_current;
                m_current = nullptr;
            }
            else
            {
                m_prev    = m_current;
                m_current = nxt;
            }
        }
    }

    // -----------------------------------------------------------------
    // unused-shape catch-alls (keep tag dispatch closed for SFINAE)
    // -----------------------------------------------------------------

    template<ll_shape S>
    void
    m_step_advance(
        std::true_type /* reverse */,
        std::integral_constant<ll_shape, S>
    ) noexcept
    {
        // Reverse on a forward-only shape is a no-op at runtime; the
        // compile-time static_assert in operator-- guards against user-visible
        // misuse. Reaching here means the shape overload was not provided on
        // purpose.
    }

    // -----------------------------------------------------------------
    // state
    // -----------------------------------------------------------------

    node_pointer m_current;   // node currently referenced by *this
    node_pointer m_prev;      // previous node (xor traversal / hint)
    node_pointer m_origin;    // origin node for circular end-detect
};


// ===========================================================================
// 3. CONVENIENCE ALIASES
// ===========================================================================

// const_linked_list_iterator
//   alias: const variant of linked_list_iterator.
template<typename Node>
using const_linked_list_iterator =
    linked_list_iterator<Node, true, false>;

// reverse_linked_list_iterator
//   alias: reverse variant (only valid for doubly- or xor-linked nodes).
template<typename Node>
using reverse_linked_list_iterator =
    linked_list_iterator<Node, false, true>;

// const_reverse_linked_list_iterator
//   alias: const + reverse variant.
template<typename Node>
using const_reverse_linked_list_iterator =
    linked_list_iterator<Node, true, true>;


// ===========================================================================
// 4.  RANGE ADAPTOR
// ===========================================================================

// linked_list_reverse_view
//   class: lightweight view that flips the begin/end of a list-like container
// so that range-based-for traverses backward. Requires the wrapped container's
// node type to support backward traversal.
template<typename List>
class linked_list_reverse_view
{
public:
    using node_type = typename List::node_type;
    using iterator  = reverse_linked_list_iterator<node_type>;
    using const_iterator =
        const_reverse_linked_list_iterator<node_type>;

    constexpr explicit
    linked_list_reverse_view(List& _list) noexcept
        : m_list(&_list)
    {}

    iterator
    begin() noexcept
    {
        return iterator(m_list->tail(), nullptr, nullptr);
    }

    iterator
    end() noexcept
    {
        return iterator();
    }

    const_iterator
    begin() const noexcept
    {
        return const_iterator(m_list->tail(), nullptr, nullptr);
    }

    const_iterator
    end() const noexcept
    {
        return const_iterator();
    }

private:
    List* m_list;
};

// reversed
//   factory: range adaptor producing a reverse view of _list. Compatible with
// range-based-for; requires _list to satisfy is_doubly_linked_list (or
// is_xor_linked_list).
template<typename List>
inline linked_list_reverse_view<List>
reversed(
    List& _list
) noexcept
{
    return linked_list_reverse_view<List>(_list);
}


// ===========================================================================
// 5.   FREE CONSTRUCTORS
// ===========================================================================
//   Convenience helpers for callers wanting to construct an iterator
// without naming the full template parameters.

// make_linked_list_iterator
//   factory: deduces the node type and constructs a forward iterator.
template<typename Node>
inline linked_list_iterator<Node>
make_linked_list_iterator(
    Node*  _start,
    Node*  _prev   = nullptr,
    Node*  _origin = nullptr
) noexcept
{
    return linked_list_iterator<Node>(_start, _prev, _origin);
}

// make_const_linked_list_iterator
//   factory: deduces the node type and constructs a const forward iterator.
template<typename Node>
inline const_linked_list_iterator<Node>
make_const_linked_list_iterator(
    const Node* _start,
    const Node* _prev   = nullptr,
    const Node* _origin = nullptr
) noexcept
{
    return const_linked_list_iterator<Node>(_start, _prev, _origin);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_LIST_LINKED_LINKED_LIST_ITERATOR_HPP
