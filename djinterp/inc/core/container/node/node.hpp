/******************************************************************************
* djinterp [container]                                                node.hpp
*
* Generic node definitions:
* Provides topology policies including leaf, dynamic, and heterogeneous
* nodes, supporting value-semantic polymorphism and stateful arenas.
*
* author(s): Samuel 'teer' Neal-Blim
* link: TBA
* file: /inc/container/node/node.hpp                          date: 2026.03.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_NODE_
#define DJINTERP_CONTAINER_NODE_ 1

#include <memory>
#include <tuple>
#include <vector>
#include "../../djinterp.hpp"
#include "../../type_traits.hpp"


NS_DJINTERP
NS_CONTAINER

    // =========================================================================
    // I.   leaf_node
    // =========================================================================

    // leaf_node
    //   Topology: Terminal/Zero-link. Contains purely data.
    //   Highly cache-efficient for variant-based tree leaves.
    template<typename _Type>
    class leaf_node
    {
    public:
        using self_type  = leaf_node;
        using value_type = _Type;

        static D_CONSTEXPR std::size_t num_links = 0;

        D_CONSTEXPR
        leaf_node()
            : m_data{}
        {}

        D_CONSTEXPR explicit
        leaf_node(const value_type& _val)
            : m_data(_val)
        {}

        D_CONSTEXPR value_type& data()
        {
            return m_data;
        }

        D_CONSTEXPR const value_type& data() const
        {
            return m_data;
        }

    private:
        value_type m_data;
    };


    // =========================================================================
    // II.  dynamic_node
    // =========================================================================

    // dynamic_node
    //   Topology: Dynamic Homogeneous. Edges are stored in a standard
    //   container. Supports std::allocator_arg_t for stateful arena injection.
    template<typename _Type,
             typename _NodeAllocator = std::allocator<_Type>,
             template<typename, typename> class _ContainerType = std::vector>
    class dynamic_node
    {
    public:
        using self_type      = dynamic_node;
        using value_type     = _Type;
        using allocator_type = _NodeAllocator;

        // Resolve self* to the actual type
        using link_type = resolve_self_t<self*, self_type>;

        // Rebind the allocator for the links
        using edge_allocator = typename std::allocator_traits<allocator_type>::template rebind_alloc<link_type>;

        // The final dynamically-sized edge container
        using container_type = _ContainerType<link_type, edge_allocator>;

        // -----------------------------------------------------------------
        // constructors (stateless / std::allocator)
        // -----------------------------------------------------------------

        D_CONSTEXPR
        dynamic_node()
            : m_data{},
              m_edges{}
        {}

        D_CONSTEXPR explicit
        dynamic_node(const value_type& _val)
            : m_data(_val),
              m_edges{}
        {}

        // -----------------------------------------------------------------
        // allocator-extended constructors (stateful / arenas)
        // -----------------------------------------------------------------

        D_CONSTEXPR
        dynamic_node(std::allocator_arg_t,
                     const allocator_type& _alloc)
            : m_data{},
              m_edges(_alloc)
        {}

        D_CONSTEXPR
        dynamic_node(std::allocator_arg_t,
                     const allocator_type& _alloc,
                     const value_type&     _val)
            : m_data(_val),
              m_edges(_alloc)
        {}

        // -----------------------------------------------------------------
        // accessors
        // -----------------------------------------------------------------

        D_CONSTEXPR value_type& data()
        {
            return m_data;
        }

        D_CONSTEXPR const value_type& data() const
        {
            return m_data;
        }

        D_CONSTEXPR container_type& edges()
        {
            return m_edges;
        }

        D_CONSTEXPR const container_type& edges() const
        {
            return m_edges;
        }

    private:
        value_type     m_data;
        container_type m_edges;
    };


    // =========================================================================
    // III. tuple_node
    // =========================================================================

    // tuple_node
    //   Topology: Static Heterogeneous. Edges are distinct types evaluated
    //   at compile time. Applies resolve_self to every edge in the pack.
    template<typename    _Type,
             typename... _Edges>
    class tuple_node
    {
    public:
        using self_type  = tuple_node;
        using value_type = _Type;

        // Apply resolve_self recursively across the parameter pack
        using edge_tuple_type = std::tuple<resolve_self_t<_Edges, self_type>...>;

        static D_CONSTEXPR std::size_t edge_groups = sizeof...(_Edges);

        D_CONSTEXPR
        tuple_node()
            : m_data{},
              m_edges{}
        {}

        D_CONSTEXPR explicit
        tuple_node(const value_type& _val)
            : m_data(_val),
              m_edges{}
        {}

        D_CONSTEXPR value_type& data()
        {
            return m_data;
        }

        D_CONSTEXPR const value_type& data() const
        {
            return m_data;
        }

        template<std::size_t _Index>
        D_CONSTEXPR auto& get_edge_group()
        {
            static_assert(_Index < edge_groups,
                          "Edge index out of bounds.");
            return std::get<_Index>(m_edges);
        }

        template<std::size_t _Index>
        D_CONSTEXPR const auto& get_edge_group() const
        {
            static_assert(_Index < edge_groups,
                          "Edge index out of bounds.");
            return std::get<_Index>(m_edges);
        }

    private:
        value_type      m_data;
        edge_tuple_type m_edges;
    };

NS_END  // container
NS_END  // djinterp

#endif  // DJINTERP_CONTAINER_NODE_