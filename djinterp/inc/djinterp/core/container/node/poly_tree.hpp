/******************************************************************************
* djinterp [container]                                          poly_tree.hpp
*
* Polymorphic Arena Tree:
* A high-performance, variant-based tree container. It stores mixed node
* topologies in a contiguous arena, avoiding heap fragmentation and virtual
* dispatch overhead.
*
* 
* path:      /inc/djinterp/core/container/node/poly_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_POLY_TREE_
#define DJINTERP_CONTAINER_POLY_TREE_ 1

// std
#include <vector>
#include <variant>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "node_common.hpp"
#include "node_iterator.hpp"


NS_DJINTERP

    // =========================================================================
    // I.   POLY TREE
    // =========================================================================

    // poly_tree
    //   class: a generic container that manages a contiguous memory arena of
    //   variant nodes. Allows O(1) insertion and trait-driven iteration.
    template<typename _ValueType,
             typename... _NodeTypes>
    class poly_tree
    {
    public:
        using value_type   = _ValueType;
        using node_variant = std::variant<_NodeTypes...>;
        using arena_type   = std::vector<node_variant>;
        using size_type    = typename arena_type::size_type;
        using index_type   = size_type;

        // Iterator types: DFS by default
        using iterator       = arena_iterator<arena_type, value_type, dfs_policy>;
        using const_iterator = const_arena_iterator<arena_type, value_type, dfs_policy>;
        using bfs_iterator   = arena_iterator<arena_type, value_type, bfs_policy>;
        using const_bfs_iterator =
            const_arena_iterator<arena_type, value_type, bfs_policy>;
        using post_order_iterator =
            arena_iterator<arena_type, value_type, post_order_policy>;
        using const_post_order_iterator =
            const_arena_iterator<arena_type, value_type, post_order_policy>;

        // Sentinel value
        static constexpr index_type npos = static_cast<index_type>(-1);

        // -----------------------------------------------------------------
        // constructors
        // -----------------------------------------------------------------

        D_CONSTEXPR
        poly_tree()
            : m_arena{},
              m_root_id(npos)
        {}

        D_CONSTEXPR explicit
        poly_tree(std::size_t _reserve_size)
            : m_arena{},
              m_root_id(npos)
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

        D_CONSTEXPR size_type size() const
        {
            return m_arena.size();
        }

        D_CONSTEXPR size_type capacity() const
        {
            return m_arena.capacity();
        }

        D_CONSTEXPR bool contains(index_type _id) const
        {
            return (_id < m_arena.size());
        }

        // -----------------------------------------------------------------
        // root access
        // -----------------------------------------------------------------

        D_CONSTEXPR index_type root_id() const
        {
            return m_root_id;
        }

        D_CONSTEXPR bool has_root() const
        {
            return (m_root_id != npos);
        }

        // -----------------------------------------------------------------
        // modifiers
        // -----------------------------------------------------------------

        // emplace_node
        //   method: constructs a node directly in the arena and returns its ID.
        template<typename _ConcreteNode,
                 typename... _Args>
        D_CONSTEXPR index_type emplace_node(_Args&&... _args)
        {
            index_type id = m_arena.size();
            m_arena.emplace_back(std::in_place_type<_ConcreteNode>,
                                 std::forward<_Args>(_args)...);

            if (m_root_id == npos)
            {
                m_root_id = id;
            }

            return id;
        }

        // set_root
        //   method: explicitly updates the entry point for iterators.
        D_CONSTEXPR void set_root(index_type _id)
        {
            m_root_id = _id;

            return;
        }

        // get_node
        //   method: retrieves a mutable reference to the variant.
        D_CONSTEXPR node_variant& get_node(index_type _id)
        {
            return m_arena[_id];
        }

        // get_node (const)
        //   method: retrieves a const reference to the variant.
        D_CONSTEXPR const node_variant& get_node(index_type _id) const
        {
            return m_arena[_id];
        }

        // clear
        //   method: removes all nodes and resets root.
        D_CONSTEXPR void clear()
        {
            m_arena.clear();
            m_root_id = npos;

            return;
        }

        // reserve
        //   method: pre-allocates arena capacity.
        D_CONSTEXPR void reserve(size_type _capacity)
        {
            m_arena.reserve(_capacity);

            return;
        }

        // swap
        //   method: exchanges contents with another poly_tree.
        D_CONSTEXPR void swap(poly_tree& _other) noexcept
        {
            m_arena.swap(_other.m_arena);
            djinterp::constexpr_swap(m_root_id, _other.m_root_id);

            return;
        }

        // -----------------------------------------------------------------
        // iterators — DFS (pre-order, default)
        // -----------------------------------------------------------------

        D_CONSTEXPR iterator begin()
        {
            return iterator(&m_arena, m_root_id);
        }

        D_CONSTEXPR iterator end()
        {
            return iterator();
        }

        D_CONSTEXPR const_iterator begin() const
        {
            return const_iterator(&m_arena, m_root_id);
        }

        D_CONSTEXPR const_iterator end() const
        {
            return const_iterator();
        }

        D_CONSTEXPR const_iterator cbegin() const
        {
            return const_iterator(&m_arena, m_root_id);
        }

        D_CONSTEXPR const_iterator cend() const
        {
            return const_iterator();
        }

        // -----------------------------------------------------------------
        // iterators — BFS (level-order)
        // -----------------------------------------------------------------

        D_CONSTEXPR bfs_iterator bfs_begin()
        {
            return bfs_iterator(&m_arena, m_root_id);
        }

        D_CONSTEXPR bfs_iterator bfs_end()
        {
            return bfs_iterator();
        }

        D_CONSTEXPR const_bfs_iterator bfs_begin() const
        {
            return const_bfs_iterator(&m_arena, m_root_id);
        }

        D_CONSTEXPR const_bfs_iterator bfs_end() const
        {
            return const_bfs_iterator();
        }

        // -----------------------------------------------------------------
        // iterators — post-order
        // -----------------------------------------------------------------

        D_CONSTEXPR post_order_iterator post_begin()
        {
            return post_order_iterator(&m_arena, m_root_id);
        }

        D_CONSTEXPR post_order_iterator post_end()
        {
            return post_order_iterator();
        }

        D_CONSTEXPR const_post_order_iterator post_begin() const
        {
            return const_post_order_iterator(&m_arena, m_root_id);
        }

        D_CONSTEXPR const_post_order_iterator post_end() const
        {
            return const_post_order_iterator();
        }

        // -----------------------------------------------------------------
        // comparison
        // -----------------------------------------------------------------

        D_CONSTEXPR friend bool operator==(const poly_tree& _a,
                                           const poly_tree& _b)
        {
            return ( _a.m_root_id == _b.m_root_id &&
                     _a.m_arena   == _b.m_arena );
        }

        D_CONSTEXPR friend bool operator!=(const poly_tree& _a,
                                           const poly_tree& _b)
        {
            return !(_a == _b);
        }

    private:
        arena_type m_arena;
        index_type m_root_id;
    };


NS_END  // djinterp

#endif  // DJINTERP_CONTAINER_POLY_TREE_
