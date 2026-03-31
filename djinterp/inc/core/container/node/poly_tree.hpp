/******************************************************************************
* djinterp [container]                                          poly_tree.hpp
*
* Polymorphic Arena Tree:
* A high-performance, variant-based tree container. It stores mixed node
* topologies in a contiguous arena, avoiding heap fragmentation and virtual
* dispatch overhead.
*

* path:      /inc/container/poly_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_POLY_TREE_
#define DJINTERP_CONTAINER_POLY_TREE_ 1

#include <vector>
#include <variant>
#include <utility>
#include "../djinterp.hpp"
#include "node/node.hpp"
#include "node/node_iterator.hpp"


NS_DJINTERP
NS_CONTAINER

    // =========================================================================
    // I.   POLY TREE
    // =========================================================================

    // poly_tree
    //   class: A generic container that manages a contiguous memory arena of
    //   variant nodes. Allows O(1) insertion and trait-driven iteration.
    template<typename _ValueType,
             typename... _NodeTypes>
    class poly_tree
    {
    public:
        using value_type   = _ValueType;
        using node_variant = std::variant<_NodeTypes...>;
        using arena_type   = std::vector<node_variant>;
        using index_type   = typename arena_type::size_type;

        // Iterator exposed as DFS by default
        using iterator     = arena_iterator<arena_type, value_type, dfs_policy>;
        using bfs_iterator = arena_iterator<arena_type, value_type, bfs_policy>;

        // -----------------------------------------------------------------
        // constructors
        // -----------------------------------------------------------------

        D_CONSTEXPR
        poly_tree()
            : m_arena{}
            , m_root_id(static_cast<index_type>(-1))
        {}

        D_CONSTEXPR explicit
        poly_tree(std::size_t _reserve_size)
            : m_arena{}
            , m_root_id(static_cast<index_type>(-1))
        {
            m_arena.reserve(_reserve_size);
        }

        // -----------------------------------------------------------------
        // capacity
        // -----------------------------------------------------------------

        D_CONSTEXPR bool empty() const
        {
            return m_arena.empty();
        }

        D_CONSTEXPR std::size_t size() const
        {
            return m_arena.size();
        }

        // -----------------------------------------------------------------
        // modifiers
        // -----------------------------------------------------------------

        // emplace_node
        //   method: Constructs a node directly in the arena and returns its ID.
        template<typename _ConcreteNode,
                 typename... _Args>
        D_CONSTEXPR index_type emplace_node(_Args&&... _args)
        {
            index_type id = m_arena.size();
            m_arena.emplace_back(std::in_place_type<_ConcreteNode>,
                                 std::forward<_Args>(_args)...);

            if (m_root_id == static_cast<index_type>(-1))
            {
                m_root_id = id;
            }

            return id;
        }

        // set_root
        //   method: Explicitly updates the entry point for iterators.
        D_CONSTEXPR void set_root(index_type _id)
        {
            m_root_id = _id;
        }

        // get_node
        //   method: Retrieves a mutable reference to the variant.
        D_CONSTEXPR node_variant& get_node(index_type _id)
        {
            return m_arena[_id];
        }

        // -----------------------------------------------------------------
        // iterators
        // -----------------------------------------------------------------

        D_CONSTEXPR iterator begin() const
        {
            return iterator(&m_arena, m_root_id);
        }

        D_CONSTEXPR iterator end() const
        {
            return iterator();
        }

        D_CONSTEXPR bfs_iterator bfs_begin() const
        {
            return bfs_iterator(&m_arena, m_root_id);
        }

        D_CONSTEXPR bfs_iterator bfs_end() const
        {
            return bfs_iterator();
        }

    private:
        arena_type m_arena;
        index_type m_root_id;
    };


NS_END  // container
NS_END  // djinterp

#endif  // DJINTERP_CONTAINER_POLY_TREE_