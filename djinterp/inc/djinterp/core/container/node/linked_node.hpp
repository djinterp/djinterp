/*******************************************************************************
* djinterp [core]                                                linked_node.hpp
*
* Generic linked node:
*   Provides a self-referential node type with a fixed number of indexed
* connections and a typed payload. The link type defaults to `self*` and
* is resolved via resolve_self, enabling raw pointer, smart
* pointer, and index-based linking strategies.
*
*   The primary design goal is composability through private or protected
* inheritance. A derived type inherits linked_node, hides the raw indexed
* interface, and exposes named accessors that alias specific indices:
*
*     class bst_node : private linked_node<int, 3>
*     {
*         using base = linked_node<int, 3>;
*
*     public:
*         using base::value_type;
*         using base::link_type;
*         using base::data;
*
*         link_type& left()         { return base::get_node<0>(); }
*         link_type& right()        { return base::get_node<1>(); }
*         link_type& parent()       { return base::get_node<2>(); }
*
*         const link_type& left()   const
*         { return base::get_node<0>(); }
*
*         const link_type& right()  const
*         { return base::get_node<1>(); }
*
*         const link_type& parent() const
*         { return base::get_node<2>(); }
*     };
*
*   Because bst_node inherits privately, only the named interface is
* visible. The indexed get_node<I>() calls are implementation details
* that never leak into the derived type's public API.
*
*   The same mechanism works for any topology:
*     - singly-linked list:  linked_node<T, 1>         (next)
*     - doubly-linked list:  linked_node<T, 2>         (next, prev)
*     - binary tree:         linked_node<T, 2> or <T, 3> (+parent)
*     - N-ary tree:          linked_node<T, N>         (parent + children)
*     - graph adjacency:     linked_node<T, N>         (neighbors)
*
* LINK TYPE RESOLUTION:
*   LinkType defaults to `self*`, which resolves to `linked_node*`.
*   Other link forms are supported via resolve_self:
*     - self*                  -> linked_node<...>*
*     - std::unique_ptr<self>  -> std::unique_ptr<linked_node<...>>
*     - std::shared_ptr<self>  -> std::shared_ptr<linked_node<...>>
*     - std::weak_ptr<self>    -> std::weak_ptr<linked_node<...>>
*     - std::size_t            -> std::size_t  (index-based, no self)
*
* INTERFACE:
*   - data()             -- access the payload
*   - get_node<I>()      -- compile-time indexed link access
*   - get_node(i)        -- runtime indexed link access
*   - edges()            -- reference to the underlying link array
*   - num_links          -- compile-time link count
*   - null_link()        -- returns the null/sentinel value for the link type
*
*
* path:      /inc/djinterp/core/container/node/linked_node.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.22
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_NODE_LINKED_NODE_HPP
#define DJINTERP_CONTAINER_NODE_LINKED_NODE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <array>                  // std::array
#include <cstddef>                // std::size_t
#include <type_traits>            // std::is_integral
#include <utility>                // std::move
// djinterp
#include "../../../djinterp.hpp"  // framework root: self, resolve_self_t


NS_DJINTERP


NS_INTERNAL

    // linked_node_null_link
    //   trait: the null link for a link type: a value-initialized link, which
    // is nullptr for a raw pointer and empty for a smart pointer.
    template<typename Link,
             bool     IsIndex = std::is_integral<Link>::value>
    struct linked_node_null_link
    {
        static D_CONSTEXPR Link value()
        {
            return Link();
        }
    };

    // linked_node_null_link<Link, true>
    //   trait: an integral index's null link is the all-ones value,
    // size_type(-1), since 0 is a valid index.
    template<typename Link>
    struct linked_node_null_link<Link, true>
    {
        static D_CONSTEXPR Link value()
        {
            return static_cast<Link>(-1);
        }
    };

NS_END  // internal

// =========================================================================
// I.   LINKED NODE
// =========================================================================

// linked_node
//   class: a self-referential node holding a Type payload and NumLinks
// indexed connections stored in a fixed std::array.
template<typename    Type,
         std::size_t NumLinks,
         typename    LinkType = self*>
class linked_node
{
public:
    using self_type  = linked_node;
    using value_type = Type;
    using link_type  = resolve_self_t<LinkType, self_type>;
    using size_type  = std::size_t;
    using edges_type = std::array<link_type, NumLinks>;

    static D_CONSTEXPR size_type num_links = NumLinks;

    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    D_CONSTEXPR
    linked_node()
        : m_data{},
          m_edges{}
    {}

    D_CONSTEXPR explicit
    linked_node(const value_type& _value)
        : m_data(_value),
          m_edges{}
    {}

    D_CONSTEXPR explicit
    linked_node(value_type&& _value)
        : m_data(std::move(_value)),
          m_edges{}
    {}

    D_CONSTEXPR
    linked_node(const value_type& _value,
                const edges_type& _edges)
        : m_data(_value),
          m_edges(_edges)
    {}

    D_CONSTEXPR
    linked_node(value_type&&      _value,
                const edges_type& _edges)
        : m_data(std::move(_value)),
          m_edges(_edges)
    {}


    // -----------------------------------------------------------------
    // data access
    // -----------------------------------------------------------------

    D_CONSTEXPR_CPP14 value_type& data()
    {
        return m_data;
    }

    D_CONSTEXPR const value_type& data() const
    {
        return m_data;
    }


    // -----------------------------------------------------------------
    // link access
    // -----------------------------------------------------------------

    template<size_type Index>
    D_CONSTEXPR_CPP14 link_type& get_node()
    {
        static_assert(Index < NumLinks,
                      "Link index out of range.");

        return m_edges[Index];
    }

    template<size_type Index>
    D_CONSTEXPR const link_type& get_node() const
    {
        static_assert(Index < NumLinks,
                      "Link index out of range.");

        return m_edges[Index];
    }

    D_CONSTEXPR_CPP14 link_type& get_node(size_type _index)
    {
        return m_edges[_index];
    }

    D_CONSTEXPR const link_type& get_node(size_type _index) const
    {
        return m_edges[_index];
    }

    // edges
    //   method: returns mutable reference to the link array. Harmonized name
    // with dynamic_node and iterators.
    D_CONSTEXPR_CPP14 edges_type& edges()
    {
        return m_edges;
    }

    D_CONSTEXPR const edges_type& edges() const
    {
        return m_edges;
    }


    // -----------------------------------------------------------------
    // link utilities
    // -----------------------------------------------------------------

    // null_link
    //   function: returns the null/sentinel value for the link type: nullptr
    // for a raw pointer, size_type(-1) for an index -- the values
    // is_null_link (node_common.hpp) tests for.
    static D_CONSTEXPR link_type null_link()
    {
        return internal::linked_node_null_link<link_type>::value();
    }

private:
    value_type m_data;
    edges_type m_edges;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_NODE_LINKED_NODE_HPP
