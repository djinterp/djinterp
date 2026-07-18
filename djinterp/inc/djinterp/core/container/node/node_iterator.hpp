/******************************************************************************
* djinterp [container]                                       node_iterator.hpp
*
* Generic node iterators:
* Provides constexpr-friendly, trait-driven iterators for polymorphic
* and variant-based node topologies. Relies on value-semantic
* polymorphism (std::variant) and policy-driven state management to
* achieve zero-cost abstractions over complex graphs.
*
* TRAVERSAL POLICIES:
*   - dfs_policy     — depth-first (pre-order) via LIFO stack
*   - bfs_policy     — breadth-first (level-order) via FIFO queue
*   - post_order_policy — depth-first post-order via dual-stack
*
* ITERATOR TYPES:
*   - arena_iterator       — mutable forward iterator over variant arenas
*   - const_arena_iterator — const forward iterator over variant arenas
*
* 
* path:      /inc/djinterp/core/container/node/node_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_NODE_ITERATOR_
#define DJINTERP_CONTAINER_NODE_ITERATOR_ 1

// std
#include <iterator>
#include <variant>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "node_common.hpp"


NS_DJINTERP

    // =========================================================================
    // I.   TRAVERSAL POLICIES
    // =========================================================================

    // dfs_policy
    //   struct: Depth-First Search (pre-order) policy.
    // State requirement: LIFO stack (std::vector used as stack).
    struct dfs_policy
    {
        template<typename _State,
                 typename _Node>
        static D_CONSTEXPR void extract(const _Node& _node,
                                        _State&      _state)
        {
            // At compile-time, we discard leaf nodes entirely.
            if constexpr (djinterp::is_dynamic_node_v<_Node>)
            {
                const auto& edges = _node.edges();

                // Push in reverse to ensure the first child is popped first
                for (auto it = edges.rbegin(); it != edges.rend(); ++it)
                {
                    _state.push_back(*it);
                }
            }
            // Future extension: handle tuple_node heterogeneous extraction
        }

        template<typename _State>
        static D_CONSTEXPR auto pop(_State& _state)
            -> typename _State::value_type
        {
            auto val = _state.back();
            _state.pop_back();

            return val;
        }
    };

    // bfs_policy
    //   struct: Breadth-First Search (level-order) policy.
    // State requirement: FIFO queue. Implemented via vector with a
    // read-head for constexpr safety.
    struct bfs_policy
    {
        template<typename _State,
                 typename _Node>
        static D_CONSTEXPR void extract(const _Node& _node,
                                        _State&      _state)
        {
            if constexpr (djinterp::is_dynamic_node_v<_Node>)
            {
                const auto& edges = _node.edges();

                for (const auto& edge : edges)
                {
                    _state.queue.push_back(edge);
                }
            }
        }

        template<typename _State>
        static D_CONSTEXPR auto pop(_State& _state)
            -> typename _State::value_type
        {
            auto val = _state.queue[_state.head];
            ++_state.head;

            return val;
        }
    };

    // post_order_policy
    //   struct: Depth-First Post-Order policy.
    // Uses a two-stack approach: push children to a work stack,
    // transfer to an output stack, then pop from output to yield
    // nodes in post-order.
    struct post_order_policy
    {
        template<typename _State,
                 typename _Node>
        static D_CONSTEXPR void extract(const _Node& _node,
                                        _State&      _state)
        {
            if constexpr (djinterp::is_dynamic_node_v<_Node>)
            {
                const auto& edges = _node.edges();

                for (const auto& edge : edges)
                {
                    _state.work.push_back(edge);
                }
            }
        }

        // build_output
        //   function: drains the work stack into the output stack,
        // producing post-order when output is read top-down.
        template<typename _State,
                 typename _ArenaType>
        static D_CONSTEXPR void build_output(_State&          _state,
                                             const _ArenaType& _arena)
        {
            while (!_state.work.empty())
            {
                auto id = _state.work.back();
                _state.work.pop_back();
                _state.output.push_back(id);

                // Push children of this node onto work stack
                std::visit([&_state](const auto& concrete_node) {
                    extract(concrete_node, _state);
                }, _arena.at(id));
            }
        }

        template<typename _State>
        static D_CONSTEXPR auto pop(_State& _state)
            -> typename _State::value_type
        {
            auto val = _state.output.back();
            _state.output.pop_back();

            return val;
        }
    };


    // =========================================================================
    // II.  ITERATOR STATE WRAPPERS
    // =========================================================================

    NS_INTERNAL

        // bfs_state
        //   struct: BFS requires a read-head to simulate a queue
        // with a vector.
        template<typename _IndexType>
        struct bfs_state
        {
            using value_type = _IndexType;

            std::vector<_IndexType> queue;
            std::size_t             head = 0;

            D_CONSTEXPR bool empty() const
            {
                return (head >= queue.size());
            }
        };

        // post_order_state
        //   struct: post-order uses a work stack and an output
        // stack.
        template<typename _IndexType>
        struct post_order_state
        {
            using value_type = _IndexType;

            std::vector<_IndexType> work;
            std::vector<_IndexType> output;

            D_CONSTEXPR bool empty() const
            {
                return output.empty();
            }
        };

    NS_END  // internal


    // =========================================================================
    // III. ARENA ITERATOR
    // =========================================================================

    // arena_iterator
    //   class: a stateful, forward iterator that traverses an external arena
    // of variant nodes using a specified traversal policy.
    //
    // Template parameters:
    //   _ArenaType  — the container of variant nodes (e.g. std::vector<variant>)
    //   _ValueType  — the value type yielded on dereference
    //   _Policy     — traversal policy (dfs_policy, bfs_policy, etc.)
    //   _IsConst    — when true, yields const references
    template<typename _ArenaType,
             typename _ValueType,
             typename _Policy  = dfs_policy,
             bool     _IsConst = false>
    class arena_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = _ValueType;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<_IsConst,
                                      const value_type*,
                                      value_type*>;
        using reference         = std::conditional_t<_IsConst,
                                      const value_type&,
                                      value_type&>;

        using arena_ptr         = std::conditional_t<_IsConst,
                                      const _ArenaType*,
                                      const _ArenaType*>;
        using index_type        = typename _ArenaType::size_type;

    private:
        // Resolve state type based on policy
        using state_type = std::conditional_t<
            std::is_same_v<_Policy, bfs_policy>,
            internal::bfs_state<index_type>,
            std::conditional_t<
                std::is_same_v<_Policy, post_order_policy>,
                internal::post_order_state<index_type>,
                std::vector<index_type>
            >
        >;

        // Sentinel value for end-of-iteration
        static constexpr index_type npos =
            static_cast<index_type>(-1);

    public:
        // -----------------------------------------------------------------
        // constructors
        // -----------------------------------------------------------------

        // arena_iterator
        //   constructor: default. Constructs an end() iterator.
        D_CONSTEXPR
        arena_iterator()
            : m_arena(nullptr),
              m_current_id(npos)
        {}

        // arena_iterator
        //   constructor: constructs from an arena and a starting root ID.
        D_CONSTEXPR
        arena_iterator(arena_ptr  _arena,
                       index_type _root_id)
            : m_arena(_arena),
              m_current_id(_root_id)
        {
            if (m_arena && (m_current_id < m_arena->size()))
            {
                if constexpr (std::is_same_v<_Policy, post_order_policy>)
                {
                    // Post-order: build the entire output stack up front
                    m_state.work.push_back(m_current_id);
                    post_order_policy::build_output(m_state, *m_arena);

                    // Prime the first element
                    if (!m_state.empty())
                    {
                        m_current_id = post_order_policy::pop(m_state);
                    }
                    else
                    {
                        m_current_id = npos;
                    }
                }
                else
                {
                    push_to_state(m_current_id);
                    advance();
                }
            }
            else
            {
                m_current_id = npos;
            }
        }


        // -----------------------------------------------------------------
        // element access
        // -----------------------------------------------------------------

        D_CONSTEXPR reference operator*() const
        {
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
            else if constexpr (std::is_same_v<_Policy, post_order_policy>)
            {
                m_state.work.push_back(_id);
            }
            else
            {
                m_state.push_back(_id);
            }
        }

        D_CONSTEXPR void advance()
        {
            if constexpr (std::is_same_v<_Policy, post_order_policy>)
            {
                if (m_state.empty())
                {
                    m_current_id = npos;

                    return;
                }

                m_current_id = post_order_policy::pop(m_state);
            }
            else
            {
                if (m_state.empty())
                {
                    m_current_id = npos;

                    return;
                }

                // Pop the next node ID using the policy
                m_current_id = _Policy::pop(m_state);

                // Visit the concrete node and extract children
                std::visit([this](const auto& concrete_node) {
                    _Policy::extract(concrete_node, m_state);
                }, m_arena->at(m_current_id));
            }
        }

        arena_ptr  m_arena;
        index_type m_current_id;
        state_type m_state;
    };

    // =========================================================================
    // IV.  CONVENIENCE ALIASES
    // =========================================================================

    // const_arena_iterator
    //   type: const variant of arena_iterator.
    template<typename _ArenaType,
             typename _ValueType,
             typename _Policy = dfs_policy>
    using const_arena_iterator =
        arena_iterator<_ArenaType, _ValueType, _Policy, true>;


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_NODE_ITERATOR_