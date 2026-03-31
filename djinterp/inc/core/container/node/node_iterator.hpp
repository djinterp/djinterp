/******************************************************************************
* djinterp [container]                                       node_iterator.hpp
*
* Generic node iterators:
* Provides constexpr-friendly, trait-driven iterators for polymorphic
* and variant-based node topologies. Relies on value-semantic
* polymorphism (std::variant) and policy-driven state management to
* achieve zero-cost abstractions over complex graphs.
*

* path:      /inc/container/node/node_iterator.hpp   
* link(s)    TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_NODE_ITERATOR_
#define DJINTERP_CONTAINER_NODE_ITERATOR_ 1

#include <iterator>
#include <variant>
#include <vector>
#include "../../djinterp.hpp"
#include "../../meta/node_traits.hpp"


NS_DJINTERP
NS_CONTAINER

    // =========================================================================
    // I.   TRAVERSAL POLICIES
    // =========================================================================

    // dfs_policy
    //   struct: Depth-First Search policy.
    // State requirement: LIFO stack.
    struct dfs_policy
    {
        template<typename _State,
                 typename _Node>
        static D_CONSTEXPR void extract(const _Node& _node,
                                        _State&      _state)
        {
            // At compile-time, we discard leaf nodes entirely.
            if constexpr (djinterp::traits::is_dynamic_node_v<_Node>)
            {
                const auto& edges = _node.edges();
                
                // Push in reverse to ensure the first child is popped first
                for (auto it = edges.rbegin(); it != edges.rend(); ++it)
                {
                    _state.push_back(*it);
                }
            }
            // Future extension: handle tuple_node heterogeneous extraction here
        }

        template<typename _State>
        static D_CONSTEXPR auto pop(_State& _state) -> typename _State::value_type
        {
            auto val = _state.back();
            _state.pop_back();
            
            return val;
        }
    };

    // bfs_policy
    //   struct: Breadth-First Search policy.
    // State requirement: FIFO queue. (Implemented via vector for
    // constexpr safety).
    struct bfs_policy
    {
        template<typename _State,
                 typename _Node>
        static D_CONSTEXPR void extract(const _Node& _node,
                                        _State&      _state)
        {
            if constexpr (djinterp::traits::is_dynamic_node_v<_Node>)
            {
                const auto& edges = _node.edges();
                
                for (const auto& edge : edges)
                {
                    _state.queue.push_back(edge);
                }
            }
        }

        template<typename _State>
        static D_CONSTEXPR auto pop(_State& _state) -> typename _State::queue::value_type
        {
            return _state.queue[_state.head++];
        }
    };


    // =========================================================================
    // II.  ITERATOR STATE WRAPPERS
    // =========================================================================

    NS_INTERNAL

        // bfs_state
        //   struct: BFS requires a read-head to simulate a queue with a
        // vector.
        template<typename _IndexType>
        struct bfs_state
        {
            std::vector<_IndexType> queue;
            std::size_t             head = 0;

            D_CONSTEXPR bool empty() const
            {
                return (head >= queue.size());
            }
        };

    NS_END  // internal


    // =========================================================================
    // III. ARENA ITERATOR
    // =========================================================================

    // arena_iterator
    //   class: A stateful, forward iterator that traverses an external arena
    // of variant nodes using a specified traversal policy.
    template<typename _ArenaType,
             typename _ValueType,
             typename _Policy = dfs_policy>
    class arena_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = _ValueType;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        using arena_ptr         = const _ArenaType*;
        using index_type        = typename _ArenaType::size_type;

    private:
        // Resolve state type based on policy
        using state_type = std::conditional_t<
            std::is_same_v<_Policy, bfs_policy>,
            internal::bfs_state<index_type>,
            std::vector<index_type>
        >;

    public:
        // -----------------------------------------------------------------
        // constructors
        // -----------------------------------------------------------------

        // arena_iterator
        //   constructor: default. Constructs an end() iterator.
        D_CONSTEXPR
        arena_iterator()
            : m_arena(nullptr)
            , m_current_id(static_cast<index_type>(-1))
        {}

        // arena_iterator
        //   constructor: constructs from an arena and a starting root ID.
        D_CONSTEXPR
        arena_iterator(arena_ptr  _arena,
                       index_type _root_id)
            : m_arena(_arena)
            , m_current_id(_root_id)
        {
            if (m_arena && (m_current_id < m_arena->size()))
            {
                push_to_state(m_current_id);
                advance(); // Prime the pump
            }
            else
            {
                m_current_id = static_cast<index_type>(-1);
            }
        }


        // -----------------------------------------------------------------
        // element access
        // -----------------------------------------------------------------

        D_CONSTEXPR reference operator*() const
        {
            // Unwrap the std::variant at compile-time and return the common data
            return std::visit([](auto& node) -> reference {
                return node.data();
            }, m_arena->at(m_current_id));
        }

        D_CONSTEXPR pointer operator->() const
        {
            return &operator*();
        }


        // -----------------------------------------------------------------
        // traversal
        // -----------------------------------------------------------------

        D_CONSTEXPR arena_iterator& operator++()
        {
            advance();
            
            return *this;
        }

        D_CONSTEXPR arena_iterator operator++(int)
        {
            arena_iterator tmp = *this;
            advance();
            
            return tmp;
        }


        // -----------------------------------------------------------------
        // comparison
        // -----------------------------------------------------------------

        D_CONSTEXPR friend bool operator==(const arena_iterator& _a,
                                           const arena_iterator& _b)
        {
            return (_a.m_current_id == _b.m_current_id);
        }

        D_CONSTEXPR friend bool operator!=(const arena_iterator& _a,
                                           const arena_iterator& _b)
        {
            return !(_a == _b);
        }

    private:
        D_CONSTEXPR void push_to_state(index_type _id)
        {
            if constexpr (std::is_same_v<_Policy, bfs_policy>)
            {
                m_state.queue.push_back(_id);
            }
            else
            {
                m_state.push_back(_id);
            }
        }

        D_CONSTEXPR void advance()
        {
            if (m_state.empty())
            {
                m_current_id = static_cast<index_type>(-1); // Reached end
                return;
            }

            // Pop the next node ID to visit using the policy
            m_current_id = _Policy::pop(m_state);

            // Visit the concrete node type and extract its children into the state
            std::visit([this](const auto& concrete_node) {
                _Policy::extract(concrete_node, m_state);
            }, m_arena->at(m_current_id));
        }

        arena_ptr  m_arena;
        index_type m_current_id;
        state_type m_state;
    };


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_NODE_ITERATOR_