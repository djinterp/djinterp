/******************************************************************************
* djinterp [container]                             hierarchical_iterator.hpp
*
* Tree-aware navigation iterators for hierarchical containers.
*   Provides iterators that navigate tree structures with full
* awareness of the topology: depth tracking, traversal order
* selection, and cursor-style parent/child/sibling movement.
*
*   tree_iterator<Node, Order>  — traverses a tree in a specified
*                                  order (pre, post, level, leaf)
*                                  with depth tracking.
*   tree_cursor<Node>           — stateful cursor with imperative
*                                  navigation (go_parent, go_child,
*                                  go_sibling) for manual tree
*                                  walking.
*
*   Both are parameterized on the node type, which must expose at
* minimum children() returning an iterable of child nodes.
*
* TABLE OF CONTENTS
* =================
* I.      Traversal Order Tag Types
* II.     tree_iterator (pre-order)
* III.    tree_iterator (post-order)
* IV.     tree_iterator (level-order)
* V.      tree_iterator (leaf-only)
* VI.     tree_cursor
* VII.    View Adapters
* VIII.   Factory Functions
*
*
* path:      /inc/container/hierarchical_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.24
******************************************************************************/

#ifndef DJINTERP_HIERARCHICAL_ITERATOR_
#define DJINTERP_HIERARCHICAL_ITERATOR_ 1

#include <cstddef>
#include <iterator>
#include <queue>
#include <stack>
#include <type_traits>
#include <utility>
#include <vector>
#include "../djinterp.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   Traversal Order Tag Types
// =============================================================================
// Lightweight empty structs used as template parameters
// to select traversal order at compile time.

struct pre_order_tag   {};
struct post_order_tag  {};
struct level_order_tag {};
struct leaf_only_tag   {};


// =============================================================================
// II.  tree_iterator (pre-order)
// =============================================================================
// Visits each node before its children.
// Root → Left subtree → Right subtree (generalized to
// all children in order).

template<typename _Node,
         typename _Order = pre_order_tag>
class tree_iterator;

template<typename _Node>
class tree_iterator<_Node, pre_order_tag>
{
public:
    using value_type        = _Node;
    using difference_type   = std::ptrdiff_t;
    using reference         = const _Node&;
    using pointer           = const _Node*;
    using iterator_category =
        std::forward_iterator_tag;

    tree_iterator()
        : m_current(nullptr)
        , m_depth(0)
    {}

    explicit tree_iterator(const _Node* _root)
        : m_current(_root)
        , m_depth(0)
    {}

    reference operator*() const
    {
        return *m_current;
    }

    pointer operator->() const
    {
        return m_current;
    }

    std::size_t depth() const noexcept
    {
        return m_depth;
    }

    tree_iterator& operator++()
    {
        const auto& kids =
            m_current->children();

        // push children in reverse onto stack
        auto it = std::end(kids);
        auto bg = std::begin(kids);

        while (it != bg)
        {
            --it;
            m_stack.push({&(*it), m_depth + 1});
        }

        if (m_stack.empty())
        {
            m_current = nullptr;
            m_depth   = 0;
        }
        else
        {
            auto top  = m_stack.top();
            m_stack.pop();
            m_current = top.first;
            m_depth   = top.second;
        }

        return *this;
    }

    tree_iterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);

        return tmp;
    }

    friend bool operator==(
        const tree_iterator& _a,
        const tree_iterator& _b)
    {
        return (_a.m_current == _b.m_current);
    }

    friend bool operator!=(
        const tree_iterator& _a,
        const tree_iterator& _b)
    {
        return (_a.m_current != _b.m_current);
    }

private:
    using entry =
        std::pair<const _Node*, std::size_t>;

    const _Node*             m_current;
    std::size_t              m_depth;
    std::stack<entry,
               std::vector<entry>> m_stack;
};


// =============================================================================
// III. tree_iterator (post-order)
// =============================================================================
// Visits each node after all its children.
// Left subtree → Right subtree → Root.

template<typename _Node>
class tree_iterator<_Node, post_order_tag>
{
public:
    using value_type        = _Node;
    using difference_type   = std::ptrdiff_t;
    using reference         = const _Node&;
    using pointer           = const _Node*;
    using iterator_category =
        std::forward_iterator_tag;

    tree_iterator()
        : m_current(nullptr)
        , m_depth(0)
    {}

    explicit tree_iterator(const _Node* _root)
        : m_current(nullptr)
        , m_depth(0)
    {
        if (_root)
        {
            descend_leftmost(_root, 0);
        }
    }

    reference operator*() const
    {
        return *m_current;
    }

    pointer operator->() const
    {
        return m_current;
    }

    std::size_t depth() const noexcept
    {
        return m_depth;
    }

    tree_iterator& operator++()
    {
        if (m_stack.empty())
        {
            m_current = nullptr;
            m_depth   = 0;

            return *this;
        }

        auto top = m_stack.top();
        m_stack.pop();

        // if the popped entry is the right child
        // of the next stack top, visit the next
        // stack top.  Otherwise descend into the
        // next sibling.
        if (!m_stack.empty())
        {
            auto& parent = m_stack.top();
            const auto& pkids =
                parent.first->children();

            // find next unvisited child
            bool found_current = false;

            for (const auto& child : pkids)
            {
                if (found_current)
                {
                    descend_leftmost(
                        &child,
                        parent.second + 1);

                    return *this;
                }

                if (&child == top.first)
                {
                    found_current = true;
                }
            }

            // all children visited — emit parent
            m_current = parent.first;
            m_depth   = parent.second;
        }
        else
        {
            m_current = nullptr;
            m_depth   = 0;
        }

        return *this;
    }

    tree_iterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);

        return tmp;
    }

    friend bool operator==(
        const tree_iterator& _a,
        const tree_iterator& _b)
    {
        return (_a.m_current == _b.m_current);
    }

    friend bool operator!=(
        const tree_iterator& _a,
        const tree_iterator& _b)
    {
        return (_a.m_current != _b.m_current);
    }

private:
    using entry =
        std::pair<const _Node*, std::size_t>;

    void descend_leftmost(const _Node* _node,
                          std::size_t  _d)
    {
        while (true)
        {
            m_stack.push({_node, _d});

            const auto& kids = _node->children();
            auto bg = std::begin(kids);

            if (bg == std::end(kids))
            {
                // leaf — this is the next node
                // to yield
                m_current = _node;
                m_depth   = _d;

                return;
            }

            _node = &(*bg);
            ++_d;
        }
    }

    const _Node*             m_current;
    std::size_t              m_depth;
    std::stack<entry,
               std::vector<entry>> m_stack;
};


// =============================================================================
// IV.  tree_iterator (level-order)
// =============================================================================
// Breadth-first: visits all nodes at depth d before any
// at depth d+1.

template<typename _Node>
class tree_iterator<_Node, level_order_tag>
{
public:
    using value_type        = _Node;
    using difference_type   = std::ptrdiff_t;
    using reference         = const _Node&;
    using pointer           = const _Node*;
    using iterator_category =
        std::forward_iterator_tag;

    tree_iterator()
        : m_current(nullptr)
        , m_depth(0)
    {}

    explicit tree_iterator(const _Node* _root)
        : m_current(_root)
        , m_depth(0)
    {}

    reference operator*() const
    {
        return *m_current;
    }

    pointer operator->() const
    {
        return m_current;
    }

    std::size_t depth() const noexcept
    {
        return m_depth;
    }

    tree_iterator& operator++()
    {
        const auto& kids =
            m_current->children();

        for (const auto& child : kids)
        {
            m_queue.push({&child, m_depth + 1});
        }

        if (m_queue.empty())
        {
            m_current = nullptr;
            m_depth   = 0;
        }
        else
        {
            auto front = m_queue.front();
            m_queue.pop();
            m_current = front.first;
            m_depth   = front.second;
        }

        return *this;
    }

    tree_iterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);

        return tmp;
    }

    friend bool operator==(
        const tree_iterator& _a,
        const tree_iterator& _b)
    {
        return (_a.m_current == _b.m_current);
    }

    friend bool operator!=(
        const tree_iterator& _a,
        const tree_iterator& _b)
    {
        return (_a.m_current != _b.m_current);
    }

private:
    using entry =
        std::pair<const _Node*, std::size_t>;

    const _Node*             m_current;
    std::size_t              m_depth;
    std::queue<entry>        m_queue;
};


// =============================================================================
// V.   tree_iterator (leaf-only)
// =============================================================================
// Visits only leaf nodes (nodes with no children), in
// DFS pre-order.

template<typename _Node>
class tree_iterator<_Node, leaf_only_tag>
{
public:
    using value_type        = _Node;
    using difference_type   = std::ptrdiff_t;
    using reference         = const _Node&;
    using pointer           = const _Node*;
    using iterator_category =
        std::forward_iterator_tag;

    tree_iterator()
        : m_inner()
    {}

    explicit tree_iterator(const _Node* _root)
        : m_inner(_root)
    {
        // advance to first leaf
        if (m_inner != tree_iterator<
                _Node, pre_order_tag>())
        {
            advance_to_leaf();
        }
    }

    reference operator*() const
    {
        return *m_inner;
    }

    pointer operator->() const
    {
        return m_inner.operator->();
    }

    std::size_t depth() const noexcept
    {
        return m_inner.depth();
    }

    tree_iterator& operator++()
    {
        ++m_inner;
        advance_to_leaf();

        return *this;
    }

    tree_iterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);

        return tmp;
    }

    friend bool operator==(
        const tree_iterator& _a,
        const tree_iterator& _b)
    {
        return (_a.m_inner == _b.m_inner);
    }

    friend bool operator!=(
        const tree_iterator& _a,
        const tree_iterator& _b)
    {
        return (_a.m_inner != _b.m_inner);
    }

private:
    void advance_to_leaf()
    {
        auto end_it = tree_iterator<
            _Node, pre_order_tag>();

        while (m_inner != end_it)
        {
            const auto& kids =
                (*m_inner).children();

            if (std::begin(kids) ==
                std::end(kids))
            {
                return;  // found a leaf
            }

            ++m_inner;
        }
    }

    tree_iterator<_Node, pre_order_tag> m_inner;
};


// =============================================================================
// VI.  tree_cursor
// =============================================================================
// Stateful cursor for imperative tree navigation.
// Tracks current node and depth, supports directional
// movement: parent, first child, next sibling, prev
// sibling.
//
// The cursor does NOT satisfy the Iterator concept.  It
// is a navigation tool for manual tree walking where the
// traversal order is determined by application logic.

template<typename _Node>
class tree_cursor
{
public:
    tree_cursor()
        : m_current(nullptr)
        , m_depth(0)
    {}

    explicit tree_cursor(const _Node* _root)
        : m_current(_root)
        , m_depth(0)
    {}

    // --- access ---

    const _Node& node() const
    {
        return *m_current;
    }

    const _Node* node_ptr() const noexcept
    {
        return m_current;
    }

    std::size_t depth() const noexcept
    {
        return m_depth;
    }

    bool is_valid() const noexcept
    {
        return (m_current != nullptr);
    }

    explicit operator bool() const noexcept
    {
        return is_valid();
    }

    // --- navigation ---

    // go_parent
    //   moves to the parent node.  Returns false if
    // already at root or parent() is unavailable.
    template<typename N = _Node>
    auto go_parent()
        -> decltype(
               std::declval<const N&>().parent(),
               bool())
    {
        if (!m_current)
        {
            return false;
        }

        auto* p = &(m_current->parent());

        if (!p)
        {
            return false;
        }

        m_current = p;

        if (m_depth > 0)
        {
            --m_depth;
        }

        return true;
    }

    // go_first_child
    //   moves to the first child.  Returns false if
    // no children.
    bool go_first_child()
    {
        if (!m_current)
        {
            return false;
        }

        const auto& kids =
            m_current->children();

        auto bg = std::begin(kids);

        if (bg == std::end(kids))
        {
            return false;
        }

        m_current = &(*bg);
        ++m_depth;

        return true;
    }

    // go_child_at
    //   moves to the Nth child.  Returns false if
    // index is out of range.
    template<typename N = _Node>
    auto go_child_at(std::size_t _index)
        -> decltype(
               std::declval<const N&>().child_at(
                   std::declval<std::size_t>()),
               bool())
    {
        if (!m_current)
        {
            return false;
        }

        auto* child =
            &(m_current->child_at(_index));

        if (!child)
        {
            return false;
        }

        m_current = child;
        ++m_depth;

        return true;
    }

    // go_next_sibling
    //   moves to the next sibling.  Returns false if
    // no next sibling.
    template<typename N = _Node>
    auto go_next_sibling()
        -> decltype(
               std::declval<const N&>()
                   .next_sibling(),
               bool())
    {
        if (!m_current)
        {
            return false;
        }

        auto* sib =
            &(m_current->next_sibling());

        if (!sib)
        {
            return false;
        }

        m_current = sib;

        return true;
    }

    // go_prev_sibling
    template<typename N = _Node>
    auto go_prev_sibling()
        -> decltype(
               std::declval<const N&>()
                   .prev_sibling(),
               bool())
    {
        if (!m_current)
        {
            return false;
        }

        auto* sib =
            &(m_current->prev_sibling());

        if (!sib)
        {
            return false;
        }

        m_current = sib;

        return true;
    }

    // go_root
    //   returns to the root.
    template<typename N = _Node>
    auto go_root()
        -> decltype(
               std::declval<const N&>().root(),
               bool())
    {
        if (!m_current)
        {
            return false;
        }

        m_current = &(m_current->root());
        m_depth   = 0;

        return true;
    }

    // --- predicates ---

    bool is_leaf() const
    {
        if (!m_current)
        {
            return false;
        }

        const auto& kids =
            m_current->children();

        return (std::begin(kids) ==
                std::end(kids));
    }

    bool is_root() const noexcept
    {
        return (m_depth == 0 &&
                m_current != nullptr);
    }

private:
    const _Node* m_current;
    std::size_t  m_depth;
};


// =============================================================================
// VII. View Adapters
// =============================================================================

// pre_order_view
template<typename _Node>
class pre_order_view
{
public:
    using const_iterator =
        tree_iterator<_Node, pre_order_tag>;

    explicit pre_order_view(
        const _Node& _root) noexcept
        : m_root(&_root)
    {}

    const_iterator begin() const
    {
        return const_iterator(m_root);
    }

    const_iterator end() const
    {
        return const_iterator();
    }

private:
    const _Node* m_root;
};

// post_order_view
template<typename _Node>
class post_order_view
{
public:
    using const_iterator =
        tree_iterator<_Node, post_order_tag>;

    explicit post_order_view(
        const _Node& _root) noexcept
        : m_root(&_root)
    {}

    const_iterator begin() const
    {
        return const_iterator(m_root);
    }

    const_iterator end() const
    {
        return const_iterator();
    }

private:
    const _Node* m_root;
};

// level_order_view
template<typename _Node>
class level_order_view
{
public:
    using const_iterator =
        tree_iterator<_Node, level_order_tag>;

    explicit level_order_view(
        const _Node& _root) noexcept
        : m_root(&_root)
    {}

    const_iterator begin() const
    {
        return const_iterator(m_root);
    }

    const_iterator end() const
    {
        return const_iterator();
    }

private:
    const _Node* m_root;
};

// leaf_view
template<typename _Node>
class leaf_view
{
public:
    using const_iterator =
        tree_iterator<_Node, leaf_only_tag>;

    explicit leaf_view(
        const _Node& _root) noexcept
        : m_root(&_root)
    {}

    const_iterator begin() const
    {
        return const_iterator(m_root);
    }

    const_iterator end() const
    {
        return const_iterator();
    }

private:
    const _Node* m_root;
};


// =============================================================================
// VIII. Factory Functions
// =============================================================================

template<typename _Node>
pre_order_view<_Node>
make_pre_order_view(
    const _Node& _root) noexcept
{
    return pre_order_view<_Node>(_root);
}

template<typename _Node>
post_order_view<_Node>
make_post_order_view(
    const _Node& _root) noexcept
{
    return post_order_view<_Node>(_root);
}

template<typename _Node>
level_order_view<_Node>
make_level_order_view(
    const _Node& _root) noexcept
{
    return level_order_view<_Node>(_root);
}

template<typename _Node>
leaf_view<_Node>
make_leaf_view(
    const _Node& _root) noexcept
{
    return leaf_view<_Node>(_root);
}

template<typename _Node>
tree_cursor<_Node>
make_tree_cursor(
    const _Node& _root) noexcept
{
    return tree_cursor<_Node>(&_root);
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_HIERARCHICAL_ITERATOR_
