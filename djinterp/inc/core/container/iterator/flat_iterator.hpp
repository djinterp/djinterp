/******************************************************************************
* djinterp [container]                                    flat_iterator.hpp
*
* Flattening iterators for the djinterp container framework.
*   Provides zero-overhead iterators that present nested or
* hierarchical structures as a single linear sequence.
*
*   concat_iterator<Outer>     — flattens depth-1 nesting by
*                                concatenating inner ranges.
*   dfs_flatten_iterator<Node> — flattens a tree via depth-first
*                                traversal, yielding nodes in
*                                pre-order.
*   bfs_flatten_iterator<Node> — flattens a tree via breadth-first
*                                traversal, yielding nodes level
*                                by level.
*
*   Each iterator stores only the minimal state needed for
* traversal: concat_iterator holds (outer_it, outer_end,
* inner_it); the tree iterators hold a stack or queue of
* pending nodes.
*
* TABLE OF CONTENTS
* =================
* I.      concat_iterator (depth-1 flatten)
* II.     dfs_flatten_iterator (pre-order tree)
* III.    bfs_flatten_iterator (level-order tree)
* IV.     View Adapters
* V.      Factory Functions
*
*
* path:      /inc/container/flat_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.24
******************************************************************************/

#ifndef DJINTERP_FLAT_ITERATOR_
#define DJINTERP_FLAT_ITERATOR_ 1

#include <cstddef>
#include <iterator>
#include <queue>
#include <stack>
#include <type_traits>
#include <vector>
#include "../djinterp.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   concat_iterator (depth-1 flatten)
// =============================================================================
// Iterates over a container-of-containers, yielding the
// inner elements in order.  When the current inner range
// is exhausted, advances to the next outer element.
//
// Example: vector<vector<int>> {{1,2},{3},{4,5}}
//   yields: 1, 2, 3, 4, 5

template<typename _OuterIter>
class concat_iterator
{
public:
    using outer_value =
        typename std::iterator_traits<
            _OuterIter>::value_type;
    using inner_iter =
        decltype(std::begin(
            std::declval<const outer_value&>()));

    using value_type =
        typename std::iterator_traits<
            inner_iter>::value_type;
    using difference_type = std::ptrdiff_t;
    using reference       = const value_type&;
    using pointer         = const value_type*;
    using iterator_category =
        std::forward_iterator_tag;

    // --- construction ---

    concat_iterator()
        : m_outer()
        , m_outer_end()
        , m_inner()
    {}

    concat_iterator(_OuterIter _begin,
                    _OuterIter _end)
        : m_outer(_begin)
        , m_outer_end(_end)
        , m_inner()
    {
        advance_to_valid();
    }

    // --- dereference ---

    reference operator*() const
    {
        return *m_inner;
    }

    pointer operator->() const
    {
        return &(*m_inner);
    }

    // --- increment ---

    concat_iterator& operator++()
    {
        ++m_inner;

        if (m_inner == std::end(*m_outer))
        {
            ++m_outer;
            advance_to_valid();
        }

        return *this;
    }

    concat_iterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);

        return tmp;
    }

    // --- comparison ---

    friend bool operator==(
        const concat_iterator& _a,
        const concat_iterator& _b)
    {
        if (_a.m_outer == _a.m_outer_end &&
            _b.m_outer == _b.m_outer_end)
        {
            return true;
        }

        return ( _a.m_outer == _b.m_outer &&
                 _a.m_inner == _b.m_inner );
    }

    friend bool operator!=(
        const concat_iterator& _a,
        const concat_iterator& _b)
    {
        return !(_a == _b);
    }

    _OuterIter outer() const { return m_outer; }

private:
    void advance_to_valid()
    {
        while (m_outer != m_outer_end)
        {
            m_inner = std::begin(*m_outer);

            if (m_inner != std::end(*m_outer))
            {
                return;
            }

            ++m_outer;
        }
    }

    _OuterIter m_outer;
    _OuterIter m_outer_end;
    inner_iter m_inner;
};


// =============================================================================
// II.  dfs_flatten_iterator (pre-order tree)
// =============================================================================
// Depth-first traversal of a hierarchical structure.
// Yields each node in pre-order (parent before children).
//
// The node type must expose children() returning an
// iterable range of child nodes (by reference or pointer).
//
// Uses an explicit stack to avoid recursion.

template<typename _Node>
class dfs_flatten_iterator
{
public:
    using value_type        = _Node;
    using difference_type   = std::ptrdiff_t;
    using reference         = const _Node&;
    using pointer           = const _Node*;
    using iterator_category =
        std::forward_iterator_tag;

    // --- construction ---

    dfs_flatten_iterator()
        : m_current(nullptr)
    {}

    explicit dfs_flatten_iterator(
        const _Node* _root)
        : m_current(_root)
    {}

    // --- dereference ---

    reference operator*() const
    {
        return *m_current;
    }

    pointer operator->() const
    {
        return m_current;
    }

    // --- increment ---

    dfs_flatten_iterator& operator++()
    {
        // push children in reverse order so that
        // the first child is visited first
        const auto& kids = m_current->children();
        auto it  = std::end(kids);
        auto bg  = std::begin(kids);

        while (it != bg)
        {
            --it;
            m_stack.push(&(*it));
        }

        if (m_stack.empty())
        {
            m_current = nullptr;
        }
        else
        {
            m_current = m_stack.top();
            m_stack.pop();
        }

        return *this;
    }

    dfs_flatten_iterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);

        return tmp;
    }

    // --- comparison ---

    friend bool operator==(
        const dfs_flatten_iterator& _a,
        const dfs_flatten_iterator& _b)
    {
        return (_a.m_current == _b.m_current);
    }

    friend bool operator!=(
        const dfs_flatten_iterator& _a,
        const dfs_flatten_iterator& _b)
    {
        return (_a.m_current != _b.m_current);
    }

    // --- accessors ---

    const _Node* current() const noexcept
    {
        return m_current;
    }

    std::size_t stack_depth() const noexcept
    {
        return m_stack.size();
    }

private:
    const _Node*                   m_current;
    std::stack<const _Node*,
               std::vector<const _Node*>>
                                   m_stack;
};


// =============================================================================
// III. bfs_flatten_iterator (level-order tree)
// =============================================================================
// Breadth-first traversal of a hierarchical structure.
// Yields nodes level by level (root first, then all
// depth-1 nodes, then all depth-2 nodes, etc.).
//
// Uses an explicit queue.

template<typename _Node>
class bfs_flatten_iterator
{
public:
    using value_type        = _Node;
    using difference_type   = std::ptrdiff_t;
    using reference         = const _Node&;
    using pointer           = const _Node*;
    using iterator_category =
        std::forward_iterator_tag;

    // --- construction ---

    bfs_flatten_iterator()
        : m_current(nullptr)
    {}

    explicit bfs_flatten_iterator(
        const _Node* _root)
        : m_current(_root)
    {}

    // --- dereference ---

    reference operator*() const
    {
        return *m_current;
    }

    pointer operator->() const
    {
        return m_current;
    }

    // --- increment ---

    bfs_flatten_iterator& operator++()
    {
        // enqueue children
        const auto& kids = m_current->children();

        for (const auto& child : kids)
        {
            m_queue.push(&child);
        }

        if (m_queue.empty())
        {
            m_current = nullptr;
        }
        else
        {
            m_current = m_queue.front();
            m_queue.pop();
        }

        return *this;
    }

    bfs_flatten_iterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);

        return tmp;
    }

    // --- comparison ---

    friend bool operator==(
        const bfs_flatten_iterator& _a,
        const bfs_flatten_iterator& _b)
    {
        return (_a.m_current == _b.m_current);
    }

    friend bool operator!=(
        const bfs_flatten_iterator& _a,
        const bfs_flatten_iterator& _b)
    {
        return (_a.m_current != _b.m_current);
    }

    // --- accessors ---

    const _Node* current() const noexcept
    {
        return m_current;
    }

    std::size_t queue_size() const noexcept
    {
        return m_queue.size();
    }

private:
    const _Node*              m_current;
    std::queue<const _Node*>  m_queue;
};


// =============================================================================
// IV.  View Adapters
// =============================================================================

// concat_view
//   view: flattens a container-of-containers into a
// single linear sequence.
template<typename _Container>
class concat_view
{
public:
    using outer_iter =
        decltype(std::cbegin(
            std::declval<const _Container&>()));
    using const_iterator =
        concat_iterator<outer_iter>;
    using value_type =
        typename const_iterator::value_type;
    using size_type = std::size_t;

    explicit concat_view(
        const _Container& _c) noexcept
        : m_ref(_c)
    {}

    const_iterator begin() const
    {
        return const_iterator(
            std::cbegin(m_ref),
            std::cend(m_ref));
    }

    const_iterator end() const
    {
        return const_iterator(
            std::cend(m_ref),
            std::cend(m_ref));
    }

    bool empty() const noexcept
    {
        return m_ref.empty();
    }

private:
    const _Container& m_ref;
};

// dfs_view
//   view: pre-order depth-first traversal of a tree.
template<typename _Node>
class dfs_view
{
public:
    using const_iterator =
        dfs_flatten_iterator<_Node>;
    using value_type = _Node;

    explicit dfs_view(
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

// bfs_view
//   view: level-order breadth-first traversal of a tree.
template<typename _Node>
class bfs_view
{
public:
    using const_iterator =
        bfs_flatten_iterator<_Node>;
    using value_type = _Node;

    explicit bfs_view(
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
// V.   Factory Functions
// =============================================================================

// make_concat_view
template<typename _Container>
concat_view<_Container>
make_concat_view(
    const _Container& _c) noexcept
{
    return concat_view<_Container>(_c);
}

// make_dfs_view
template<typename _Node>
dfs_view<_Node>
make_dfs_view(const _Node& _root) noexcept
{
    return dfs_view<_Node>(_root);
}

// make_bfs_view
template<typename _Node>
bfs_view<_Node>
make_bfs_view(const _Node& _root) noexcept
{
    return bfs_view<_Node>(_root);
}

// make_dfs_iterator
template<typename _Node>
dfs_flatten_iterator<_Node>
make_dfs_iterator(const _Node* _root)
{
    return dfs_flatten_iterator<_Node>(_root);
}

// make_bfs_iterator
template<typename _Node>
bfs_flatten_iterator<_Node>
make_bfs_iterator(const _Node* _root)
{
    return bfs_flatten_iterator<_Node>(_root);
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_FLAT_ITERATOR_
