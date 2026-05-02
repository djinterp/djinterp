/******************************************************************************
* djinterp [container]                                      tree_iterator.hpp
*
* Pointer-Linked Tree Iterators:
* Provides traversal iterators and an imperative cursor for pointer-linked
* tree structures. Unlike arena_iterator (which traverses variant arenas),
* these operate directly on node pointers using trait-detected navigation
* members (left/right/parent for binary; children/parent for n-ary).
*
* TRAVERSAL ORDERS (tag types):
*   - pre_order_tag        - root, left, right (DFS)
*   - in_order_tag         - left, root, right (binary only)
*   - post_order_tag       - left, right, root
*   - level_order_tag      - breadth-first
*   - leaf_order_tag       - only leaf nodes, DFS discovery order
*
* ITERATOR:
*   - tree_iterator<Node, Order>  - forward iterator over a tree from a root
*
* CURSOR:
*   - tree_cursor<Node>           - imperative navigator with go_parent,
*     go_left, go_right, go_first_child, go_next_sibling, go_root
*
* REQUIREMENTS:
*   Binary trees: Node must satisfy is_binary_node (has left/right access).
*   Parented traversals: Node must satisfy is_parented (has parent access).
*   N-ary trees: Node must have edges() or a children-like interface.
*
* path:      /inc/container/tree_iterator.hpp
* link(s):   TBA
* author(s): djinterp AI Agent                                date: 2026.03.31
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_ITERATOR_
#define DJINTERP_CONTAINER_TREE_ITERATOR_ 1

#include <iterator>
#include <vector>
#include <cstddef>
#include "../../djinterp.hpp"
#include "../node/node_traits.hpp"


NS_DJINTERP

    // =========================================================================
    // I.   TRAVERSAL ORDER TAGS
    // =========================================================================

    // pre_order_tag
    //   tag: depth-first pre-order traversal (root before children).
    struct pre_order_tag
    {};

    // in_order_tag
    //   tag: depth-first in-order traversal (binary trees only:
    // left, root, right).
    struct in_order_tag
    {};

    // post_order_tag
    //   tag: depth-first post-order traversal (children before root).
    struct post_order_tag
    {};

    // level_order_tag
    //   tag: breadth-first / level-order traversal.
    struct level_order_tag
    {};

    // leaf_order_tag
    //   tag: yields only leaf nodes in DFS discovery order.
    struct leaf_order_tag
    {};


    // =========================================================================
    // II.  NODE ACCESS HELPERS (internal)
    // =========================================================================
    // Thin wrappers that dispatch to field or method access based on
    // what the node actually exposes. Uses the unified access traits
    // from node_traits.hpp.

    NS_INTERNAL

        // get_left
        //   function: retrieves left child pointer from a node.
        // Dispatches to `.left` member or `.left()` method.
        template<typename _N>
        D_CONSTEXPR auto get_left(_N* _node)
            -> decltype(_node->left())
        {
            return _node->left();
        }

        // get_right
        //   function: retrieves right child pointer from a node.
        template<typename _N>
        D_CONSTEXPR auto get_right(_N* _node)
            -> decltype(_node->right())
        {
            return _node->right();
        }

        // get_parent
        //   function: retrieves parent pointer from a node.
        template<typename _N>
        D_CONSTEXPR auto get_parent(_N* _node)
            -> decltype(_node->parent())
        {
            return _node->parent();
        }

        // is_null
        //   function: tests whether a node pointer is null.
        template<typename _N>
        D_CONSTEXPR bool is_null(_N* _ptr)
        {
            return (_ptr == nullptr);
        }

        // is_leaf_node
        //   function: tests whether a binary node is a leaf
        // (both children null).
        template<typename _N>
        D_CONSTEXPR bool is_leaf_node(_N* _node)
        {
            if (is_null(_node))
            {
                return false;
            }

            return ( is_null(get_left(_node)) &&
                     is_null(get_right(_node)) );
        }

        // push_leftmost
        //   function: pushes all nodes along the left spine onto the stack.
        template<typename _N>
        D_CONSTEXPR void push_leftmost(_N*                _node,
                                       std::vector<_N*>&  _stack)
        {
            while (!is_null(_node))
            {
                _stack.push_back(_node);
                _node = get_left(_node);
            }

            return;
        }

        // push_post_order
        //   function: pushes nodes for post-order traversal.
        // Descends as far left as possible, then right.
        template<typename _N>
        D_CONSTEXPR void push_post_order(_N*               _node,
                                         std::vector<_N*>& _stack)
        {
            while (!is_null(_node))
            {
                _stack.push_back(_node);

                if (!is_null(get_left(_node)))
                {
                    _node = get_left(_node);
                }
                else
                {
                    _node = get_right(_node);
                }
            }

            return;
        }

    NS_END  // internal


    // =========================================================================
    // III. TREE ITERATOR
    // =========================================================================

    // tree_iterator
    //   class: a forward iterator that traverses a pointer-linked tree
    // using a specified traversal order. Maintains an internal stack or
    // queue to track traversal state without modifying the tree.
    //
    // Template parameters:
    //   _NodeType   - the concrete node type (must satisfy binary node traits)
    //   _OrderTag   - one of the traversal order tags
    template<typename _NodeType,
             typename _OrderTag = pre_order_tag>
    class tree_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using node_type         = _NodeType;
        using value_type        = typename node_type::value_type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;
        using const_pointer     = const value_type*;
        using const_reference   = const value_type&;

        // -----------------------------------------------------------------
        // constructors
        // -----------------------------------------------------------------

        // tree_iterator
        //   constructor: default. Constructs an end() sentinel.
        D_CONSTEXPR
        tree_iterator()
            : m_current(nullptr)
        {}

        // tree_iterator
        //   constructor: constructs from a root node pointer.
        D_CONSTEXPR explicit
        tree_iterator(node_type* _root)
            : m_current(nullptr)
        {
            if (!internal::is_null(_root))
            {
                initialize(_root);
            }
        }

        // -----------------------------------------------------------------
        // element access
        // -----------------------------------------------------------------

        D_CONSTEXPR reference operator*() const
        {
            return m_current->data();
        }

        D_CONSTEXPR pointer operator->() const
        {
            return &(m_current->data());
        }

        // node
        //   method: returns the underlying node pointer (useful for
        // structural operations).
        D_CONSTEXPR node_type* node() const
        {
            return m_current;
        }

        // -----------------------------------------------------------------
        // traversal
        // -----------------------------------------------------------------

        D_CONSTEXPR tree_iterator& operator++()
        {
            advance();

            return *this;
        }

        D_CONSTEXPR tree_iterator operator++(int)
        {
            tree_iterator tmp = *this;
            advance();

            return tmp;
        }

        // -----------------------------------------------------------------
        // comparison
        // -----------------------------------------------------------------

        D_CONSTEXPR friend bool operator==(const tree_iterator& _a,
                                           const tree_iterator& _b)
        {
            return (_a.m_current == _b.m_current);
        }

        D_CONSTEXPR friend bool operator!=(const tree_iterator& _a,
                                           const tree_iterator& _b)
        {
            return !(_a == _b);
        }

    private:
        // =================================================================
        // initialization dispatch
        // =================================================================

        D_CONSTEXPR void initialize(node_type* _root)
        {
            if constexpr (std::is_same_v<_OrderTag, pre_order_tag>)
            {
                m_stack.push_back(_root);
                advance_pre_order();
            }
            else if constexpr (std::is_same_v<_OrderTag, in_order_tag>)
            {
                internal::push_leftmost(_root, m_stack);
                advance_in_order();
            }
            else if constexpr (std::is_same_v<_OrderTag, post_order_tag>)
            {
                internal::push_post_order(_root, m_stack);
                advance_post_order();
            }
            else if constexpr (std::is_same_v<_OrderTag, level_order_tag>)
            {
                m_stack.push_back(_root);
                m_head = 0;
                advance_level_order();
            }
            else if constexpr (std::is_same_v<_OrderTag, leaf_order_tag>)
            {
                m_stack.push_back(_root);
                advance_leaf_order();
            }

            return;
        }

        // =================================================================
        // advance dispatch
        // =================================================================

        D_CONSTEXPR void advance()
        {
            if constexpr (std::is_same_v<_OrderTag, pre_order_tag>)
            {
                advance_pre_order();
            }
            else if constexpr (std::is_same_v<_OrderTag, in_order_tag>)
            {
                advance_in_order();
            }
            else if constexpr (std::is_same_v<_OrderTag, post_order_tag>)
            {
                advance_post_order();
            }
            else if constexpr (std::is_same_v<_OrderTag, level_order_tag>)
            {
                advance_level_order();
            }
            else if constexpr (std::is_same_v<_OrderTag, leaf_order_tag>)
            {
                advance_leaf_order();
            }

            return;
        }

        // =================================================================
        // pre-order: root, left subtree, right subtree
        // =================================================================

        D_CONSTEXPR void advance_pre_order()
        {
            if (m_stack.empty())
            {
                m_current = nullptr;

                return;
            }

            m_current = m_stack.back();
            m_stack.pop_back();

            // Push right first so left is processed first (LIFO)
            auto* right = internal::get_right(m_current);
            auto* left  = internal::get_left(m_current);

            if (!internal::is_null(right))
            {
                m_stack.push_back(right);
            }

            if (!internal::is_null(left))
            {
                m_stack.push_back(left);
            }

            return;
        }

        // =================================================================
        // in-order: left subtree, root, right subtree (binary only)
        // =================================================================

        D_CONSTEXPR void advance_in_order()
        {
            if (m_stack.empty())
            {
                m_current = nullptr;

                return;
            }

            m_current = m_stack.back();
            m_stack.pop_back();

            // After visiting current, push the left spine of the right child
            auto* right = internal::get_right(m_current);

            if (!internal::is_null(right))
            {
                internal::push_leftmost(right, m_stack);
            }

            return;
        }

        // =================================================================
        // post-order: left subtree, right subtree, root
        // =================================================================

        D_CONSTEXPR void advance_post_order()
        {
            if (m_stack.empty())
            {
                m_current = nullptr;

                return;
            }

            m_current = m_stack.back();
            m_stack.pop_back();

            // If the stack is non-empty and the current node is the left
            // child of the stack top, process the right subtree
            if (!m_stack.empty())
            {
                auto* top = m_stack.back();

                if (m_current == internal::get_left(top))
                {
                    auto* right = internal::get_right(top);

                    if (!internal::is_null(right))
                    {
                        internal::push_post_order(right, m_stack);
                    }
                }
            }

            return;
        }

        // =================================================================
        // level-order (BFS): breadth-first via FIFO queue
        // =================================================================

        D_CONSTEXPR void advance_level_order()
        {
            if (m_head >= m_stack.size())
            {
                m_current = nullptr;

                return;
            }

            m_current = m_stack[m_head];
            ++m_head;

            // Enqueue children
            auto* left  = internal::get_left(m_current);
            auto* right = internal::get_right(m_current);

            if (!internal::is_null(left))
            {
                m_stack.push_back(left);
            }

            if (!internal::is_null(right))
            {
                m_stack.push_back(right);
            }

            return;
        }

        // =================================================================
        // leaf-order: only leaf nodes, DFS discovery
        // =================================================================

        D_CONSTEXPR void advance_leaf_order()
        {
            while (!m_stack.empty())
            {
                auto* node = m_stack.back();
                m_stack.pop_back();

                auto* right = internal::get_right(node);
                auto* left  = internal::get_left(node);

                // Push right then left so left is visited first
                if (!internal::is_null(right))
                {
                    m_stack.push_back(right);
                }

                if (!internal::is_null(left))
                {
                    m_stack.push_back(left);
                }

                // Yield only leaf nodes
                if (internal::is_leaf_node(node))
                {
                    m_current = node;

                    return;
                }
            }

            m_current = nullptr;

            return;
        }

        node_type*              m_current;
        std::vector<node_type*> m_stack;
        std::size_t             m_head = 0;
    };


    // =========================================================================
    // IV.  CONST TREE ITERATOR
    // =========================================================================

    // const_tree_iterator
    //   class: const variant of tree_iterator. Yields const references
    // to node data.
    template<typename _NodeType,
             typename _OrderTag = pre_order_tag>
    class const_tree_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using node_type         = const _NodeType;
        using value_type        = typename _NodeType::value_type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        D_CONSTEXPR
        const_tree_iterator()
            : m_inner()
        {}

        D_CONSTEXPR explicit
        const_tree_iterator(const _NodeType* _root)
            : m_inner(const_cast<_NodeType*>(_root))
        {}

        // Allow implicit conversion from non-const iterator
        D_CONSTEXPR
        const_tree_iterator(const tree_iterator<_NodeType, _OrderTag>& _other)
            : m_inner(_other)
        {}

        D_CONSTEXPR reference operator*() const
        {
            return *m_inner;
        }

        D_CONSTEXPR pointer operator->() const
        {
            return m_inner.operator->();
        }

        D_CONSTEXPR const _NodeType* node() const
        {
            return m_inner.node();
        }

        D_CONSTEXPR const_tree_iterator& operator++()
        {
            ++m_inner;

            return *this;
        }

        D_CONSTEXPR const_tree_iterator operator++(int)
        {
            const_tree_iterator tmp = *this;
            ++m_inner;

            return tmp;
        }

        D_CONSTEXPR friend bool operator==(const const_tree_iterator& _a,
                                           const const_tree_iterator& _b)
        {
            return (_a.m_inner == _b.m_inner);
        }

        D_CONSTEXPR friend bool operator!=(const const_tree_iterator& _a,
                                           const const_tree_iterator& _b)
        {
            return !(_a == _b);
        }

    private:
        tree_iterator<_NodeType, _OrderTag> m_inner;
    };


    // =========================================================================
    // V.   TREE CURSOR
    // =========================================================================

    // tree_cursor
    //   class: an imperative, non-owning navigator over a pointer-linked
    // tree. Provides explicit navigation commands (go_left, go_right,
    // go_parent, etc.) and query methods (is_leaf, is_root, depth).
    //
    // Unlike tree_iterator, a cursor does not maintain traversal state -
    // it simply points to a single node and moves on command.
    template<typename _NodeType>
    class tree_cursor
    {
    public:
        using node_type  = _NodeType;
        using value_type = typename node_type::value_type;

        // -----------------------------------------------------------------
        // constructors
        // -----------------------------------------------------------------

        D_CONSTEXPR
        tree_cursor()
            : m_current(nullptr),
              m_root(nullptr)
        {}

        D_CONSTEXPR explicit
        tree_cursor(node_type* _root)
            : m_current(_root),
              m_root(_root)
        {}

        // -----------------------------------------------------------------
        // query
        // -----------------------------------------------------------------

        D_CONSTEXPR bool valid() const
        {
            return (m_current != nullptr);
        }

        D_CONSTEXPR explicit operator bool() const
        {
            return valid();
        }

        D_CONSTEXPR bool is_root() const
        {
            return (m_current == m_root);
        }

        D_CONSTEXPR bool is_leaf() const
        {
            if (!valid())
            {
                return false;
            }

            return internal::is_leaf_node(m_current);
        }

        D_CONSTEXPR bool has_left() const
        {
            return ( valid() &&
                     !internal::is_null(internal::get_left(m_current)) );
        }

        D_CONSTEXPR bool has_right() const
        {
            return ( valid() &&
                     !internal::is_null(internal::get_right(m_current)) );
        }

        D_CONSTEXPR bool has_parent() const
        {
            if constexpr (has_parent_access<node_type>::value)
            {
                return ( valid() &&
                         !internal::is_null(internal::get_parent(m_current)) );
            }
            else
            {
                return false;
            }
        }

        // depth
        //   method: computes the depth of the current node from the root.
        // Requires parent access. Returns 0 for root or if no parent link.
        D_CONSTEXPR std::size_t depth() const
        {
            if constexpr (has_parent_access<node_type>::value)
            {
                std::size_t d = 0;
                auto*       n = m_current;

                while (!internal::is_null(n) && n != m_root)
                {
                    n = internal::get_parent(n);
                    ++d;
                }

                return d;
            }
            else
            {
                return 0;
            }
        }

        // -----------------------------------------------------------------
        // element access
        // -----------------------------------------------------------------

        D_CONSTEXPR value_type& data()
        {
            return m_current->data();
        }

        D_CONSTEXPR const value_type& data() const
        {
            return m_current->data();
        }

        D_CONSTEXPR node_type* node() const
        {
            return m_current;
        }

        // -----------------------------------------------------------------
        // navigation
        // -----------------------------------------------------------------

        // go_left
        //   method: moves cursor to left child. Returns true on success.
        D_CONSTEXPR bool go_left()
        {
            if (!has_left())
            {
                return false;
            }

            m_current = internal::get_left(m_current);

            return true;
        }

        // go_right
        //   method: moves cursor to right child. Returns true on success.
        D_CONSTEXPR bool go_right()
        {
            if (!has_right())
            {
                return false;
            }

            m_current = internal::get_right(m_current);

            return true;
        }

        // go_parent
        //   method: moves cursor to parent. Returns true on success.
        // Only available when node supports parent access.
        D_CONSTEXPR bool go_parent()
        {
            if constexpr (has_parent_access<node_type>::value)
            {
                if (!has_parent())
                {
                    return false;
                }

                m_current = internal::get_parent(m_current);

                return true;
            }
            else
            {
                return false;
            }
        }

        // go_root
        //   method: resets cursor to the tree root. Returns true if
        // root is valid.
        D_CONSTEXPR bool go_root()
        {
            m_current = m_root;

            return (m_current != nullptr);
        }

        // go_first_child
        //   method: for binary trees, equivalent to go_left.
        D_CONSTEXPR bool go_first_child()
        {
            return go_left();
        }

        // go_next_sibling
        //   method: for binary trees with parent access, moves to the
        // right sibling. If cursor is the left child of its parent,
        // moves to the right child. Otherwise fails.
        D_CONSTEXPR bool go_next_sibling()
        {
            if constexpr (has_parent_access<node_type>::value)
            {
                if (!has_parent())
                {
                    return false;
                }

                auto* par = internal::get_parent(m_current);

                // If we're the left child, move to the right child
                if (m_current == internal::get_left(par))
                {
                    auto* right = internal::get_right(par);

                    if (!internal::is_null(right))
                    {
                        m_current = right;

                        return true;
                    }
                }

                return false;
            }
            else
            {
                return false;
            }
        }

        // set
        //   method: explicitly sets the cursor to a given node.
        D_CONSTEXPR void set(node_type* _node)
        {
            m_current = _node;

            return;
        }

        // -----------------------------------------------------------------
        // comparison
        // -----------------------------------------------------------------

        D_CONSTEXPR friend bool operator==(const tree_cursor& _a,
                                           const tree_cursor& _b)
        {
            return (_a.m_current == _b.m_current);
        }

        D_CONSTEXPR friend bool operator!=(const tree_cursor& _a,
                                           const tree_cursor& _b)
        {
            return !(_a == _b);
        }

    private:
        node_type* m_current;
        node_type* m_root;
    };


    // =========================================================================
    // VI.  CONVENIENCE ALIASES
    // =========================================================================

    // pre_order_iterator
    //   type: tree_iterator with pre-order traversal.
    template<typename _N>
    using pre_order_iterator = tree_iterator<_N, pre_order_tag>;

    // in_order_iterator
    //   type: tree_iterator with in-order traversal (binary only).
    template<typename _N>
    using in_order_iterator = tree_iterator<_N, in_order_tag>;

    // post_order_iterator
    //   type: tree_iterator with post-order traversal.
    template<typename _N>
    using post_order_iterator = tree_iterator<_N, post_order_tag>;

    // level_order_iterator
    //   type: tree_iterator with level-order (BFS) traversal.
    template<typename _N>
    using level_order_iterator = tree_iterator<_N, level_order_tag>;

    // leaf_iterator
    //   type: tree_iterator that yields only leaf nodes.
    template<typename _N>
    using leaf_iterator = tree_iterator<_N, leaf_order_tag>;


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TREE_ITERATOR_