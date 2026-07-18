/******************************************************************************
* djinterp [container]                                         linked_node.hpp
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
*   _LinkType defaults to `self*`, which resolves to `linked_node*`.
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_CONTAINER_NODE_LINKED_
#define DJINTERP_CONTAINER_NODE_LINKED_ 1

// require env.h to be included first
//#ifndef DJINTERP_ENVIRONMENT_
//    #error "linked_node.hpp requires env.h to be included first"
//#endif
//
//// only meaningful in C++ mode
//#ifndef __cplusplus
//    #error "linked_node.hpp can only be used in C++ compilation mode"
//#endif
//
//// requires C++11 or higher
//#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
//    #error "linked_node.hpp requires C++11 or higher"
//#endif

// std
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP

// =========================================================================
// I.   LINKED NODE
// =========================================================================

// linked_node
//   class: a self-referential node holding a _Type payload and
// _NumLinks indexed connections stored in a fixed std::array.
template<typename    _Type,
            std::size_t _NumLinks,
            typename    _LinkType = self*>
class linked_node
{
public:
    using self_type  = linked_node;
    using value_type = _Type;
    using link_type  = resolve_self_t<_LinkType, self_type>;
    using size_type  = std::size_t;
    using edges_type = std::array<link_type, _NumLinks>;

    static D_CONSTEXPR size_type num_links = _NumLinks;

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

    D_CONSTEXPR value_type& data()
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

    template<size_type _I>
    D_CONSTEXPR link_type& get_node()
    {
        static_assert(_I < _NumLinks,
                        "Link index out of range.");
        return m_edges[_I];
    }

    template<size_type _I>
    D_CONSTEXPR const link_type& get_node() const
    {
        static_assert(_I < _NumLinks,
                        "Link index out of range.");
        return m_edges[_I];
    }

    D_CONSTEXPR link_type& get_node(size_type _index)
    {
        return m_edges[_index];
    }

    D_CONSTEXPR const link_type& get_node(size_type _index) const
    {
        return m_edges[_index];
    }

    // edges
    //   method: returns mutable reference to the link array.
    // Harmonized name with dynamic_node and iterators.
    D_CONSTEXPR edges_type& edges()
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
    //   function: returns the null/sentinel value for the link type.
    // For raw pointers this is nullptr; for indices it is size_type(-1).
    static D_CONSTEXPR link_type null_link()
    {
        return link_type{};
    }

private:
    value_type m_data;
    edges_type m_edges;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_NODE_LINKED_