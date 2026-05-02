/******************************************************************************
* djinterp [container]                                  nary_tree_iterator.hpp
*
* Iterators and a cursor for LCRS n-ary trees:
*   This header provides forward iterators over an LCRS-shaped node
* graph in four traversal orders, plus an imperative navigator
* analogous to `tree_cursor` from `tree_iterator.hpp` (which is
* binary-only).  All facilities are templated on the node type, so
* mutable and `const` variants share one definition.
*
* TRAVERSAL ORDERS (tag types reused from tree_iterator.hpp):
*   - pre_order_tag      - root, then children
*   - post_order_tag     - children, then root (uses parent links)
*   - level_order_tag    - breadth-first
*   - leaf_order_tag     - leaves only, in DFS discovery order
*
* ITERATORS (each is a forward iterator):
*   - nary_pre_order_iterator<N>
*   - nary_post_order_iterator<N>
*   - nary_level_order_iterator<N>
*   - nary_leaf_iterator<N>
*
* DISPATCH ALIAS:
*   - nary_tree_iterator<N, OrderTag>  selects one of the four
*                                      iterators by tag (pre is the
*                                      default)
*
* CURSOR:
*   - nary_tree_cursor<N>              imperative LCRS navigator
*
* REQUIREMENTS:
*   The node type must expose, in const-correct form:
*     - data()
*     - parent()
*     - first_child()
*     - next_sibling()
*     - last_child()    (cursor / level / post-order)
*     - prev_sibling()  (cursor)
*   `nary_tree_node<T>` from nary_tree_node.hpp satisfies all of
*   these.
*
* 
* path:      /inc/djinterp/core/container/tree/nary/nary_tree_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_NARY_TREE_ITERATOR_
#define DJINTERP_CONTAINER_NARY_TREE_ITERATOR_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../tree_iterator.hpp"


NS_DJINTERP

// =========================================================================
// I.   ITERATOR TYPE TRAITS (internal)
// =========================================================================

NS_INTERNAL

    // nary_iter_traits
    //   trait: derives the standard iterator typedefs from
    // _NodeType.  Handles the const / non-const split via
    // std::is_const so a single iterator template covers both
    // `nary_tree_node<T>` and `const nary_tree_node<T>`.
    template<typename _NodeType>
    struct nary_iter_traits
    {
    private:
        using bare_node_type = typename std::remove_const<
            _NodeType>::type;

    public:
        using value_type      = typename bare_node_type::value_type;
        using difference_type = std::ptrdiff_t;

        using reference = typename std::conditional<
            std::is_const<_NodeType>::value,
            const value_type&,
            value_type&>::type;

        using pointer = typename std::conditional<
            std::is_const<_NodeType>::value,
            const value_type*,
            value_type*>::type;
    };

NS_END  // internal


// =========================================================================
// II.  PRE-ORDER ITERATOR
// =========================================================================

// nary_pre_order_iterator
//   class: depth-first pre-order forward iterator over an
// LCRS subtree rooted at a given node.  Yields a node before
// descending into its children.  State is a stack of pending
// sibling pointers - at most one per ancestor on the
// root-ward spine - so memory is bounded by tree depth, not
// by node count.
//
//   Subtree-anchored: the iterator never advances past the
// siblings of the start node, so iterating from any node
// visits exactly that subtree.  The end sentinel is the
// default-constructed iterator (m_current == nullptr).
template<typename _NodeType>
class nary_pre_order_iterator
{
private:
    using traits_type = internal::nary_iter_traits<_NodeType>;

public:
    using iterator_category = std::forward_iterator_tag;
    using node_type         = _NodeType;
    using value_type        = typename traits_type::value_type;
    using difference_type   = typename traits_type::difference_type;
    using reference         = typename traits_type::reference;
    using pointer           = typename traits_type::pointer;


    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    D_CONSTEXPR
    nary_pre_order_iterator() noexcept
        : m_current(nullptr),
            m_subtree_root(nullptr),
            m_pending()
    {}

    D_CONSTEXPR explicit
    nary_pre_order_iterator(
        node_type* _root
    )
        : m_current(_root),
            m_subtree_root(_root),
            m_pending()
    {}


    // -----------------------------------------------------------------
    // element access
    // -----------------------------------------------------------------

    D_CONSTEXPR reference
    operator*() const noexcept
    {
        return m_current->data();
    }

    D_CONSTEXPR pointer
    operator->() const noexcept
    {
        return &m_current->data();
    }

    // node
    //   returns the underlying node pointer.  Useful for
    // structural operations that need more than the payload.
    D_CONSTEXPR node_type*
    node() const noexcept
    {
        return m_current;
    }


    // -----------------------------------------------------------------
    // traversal
    // -----------------------------------------------------------------

    nary_pre_order_iterator&
    operator++()
    {
        advance();

        return *this;
    }

    nary_pre_order_iterator
    operator++(int)
    {
        nary_pre_order_iterator tmp;

        tmp = *this;
        advance();

        return tmp;
    }


    // -----------------------------------------------------------------
    // comparison
    // -----------------------------------------------------------------

    D_CONSTEXPR friend bool
    operator==(
        const nary_pre_order_iterator& _a,
        const nary_pre_order_iterator& _b
    ) noexcept
    {
        return (_a.m_current == _b.m_current);
    }

    D_CONSTEXPR friend bool
    operator!=(
        const nary_pre_order_iterator& _a,
        const nary_pre_order_iterator& _b
    ) noexcept
    {
        return !(_a == _b);
    }


private:
    // advance
    //   moves m_current to the next node in pre-order:
    //     1. descend into first_child if present (pushing
    //        next_sibling onto the pending stack unless the
    //        current node is the subtree root, whose siblings
    //        lie outside our scope),
    //     2. otherwise step laterally to next_sibling,
    //     3. otherwise unwind by popping the pending stack -
    //        or, if it is empty, signal end of traversal.
    void
    advance()
    {
        node_type* child;

        if (m_current == nullptr)
        {
            return;
        }

        child = m_current->first_child();

        // rule 1: descend
        if (child != nullptr)
        {
            if ( (m_current             != m_subtree_root) &&
                    (m_current->next_sibling() != nullptr) )
            {
                m_pending.push_back(m_current->next_sibling());
            }

            m_current = child;

            return;
        }

        // rule 2: lateral step (only within the subtree)
        if ( (m_current             != m_subtree_root) &&
                (m_current->next_sibling() != nullptr) )
        {
            m_current = m_current->next_sibling();

            return;
        }

        // rule 3: unwind via the pending stack
        if (!m_pending.empty())
        {
            m_current = m_pending.back();
            m_pending.pop_back();

            return;
        }

        m_current = nullptr;

        return;
    }

    node_type*              m_current;
    node_type*              m_subtree_root;
    std::vector<node_type*> m_pending;
};


// =========================================================================
// III. POST-ORDER ITERATOR
// =========================================================================

// nary_post_order_iterator
//   class: depth-first post-order forward iterator.  Yields
// a node only after its entire subtree has been visited.
// Uses parent pointers to climb back out, so state is just
// the current node and the subtree root - no stack needed.
template<typename _NodeType>
class nary_post_order_iterator
{
private:
    using traits_type = internal::nary_iter_traits<_NodeType>;

public:
    using iterator_category = std::forward_iterator_tag;
    using node_type         = _NodeType;
    using value_type        = typename traits_type::value_type;
    using difference_type   = typename traits_type::difference_type;
    using reference         = typename traits_type::reference;
    using pointer           = typename traits_type::pointer;


    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    D_CONSTEXPR
    nary_post_order_iterator() noexcept
        : m_current(nullptr),
            m_subtree_root(nullptr)
    {}

    D_CONSTEXPR explicit
    nary_post_order_iterator(
        node_type* _root
    ) noexcept
        : m_current(descend_leftmost_deepest(_root)),
            m_subtree_root(_root)
    {}


    // -----------------------------------------------------------------
    // element access
    // -----------------------------------------------------------------

    D_CONSTEXPR reference
    operator*() const noexcept
    {
        return m_current->data();
    }

    D_CONSTEXPR pointer
    operator->() const noexcept
    {
        return &m_current->data();
    }

    D_CONSTEXPR node_type*
    node() const noexcept
    {
        return m_current;
    }


    // -----------------------------------------------------------------
    // traversal
    // -----------------------------------------------------------------

    nary_post_order_iterator&
    operator++() noexcept
    {
        advance();

        return *this;
    }

    nary_post_order_iterator
    operator++(int) noexcept
    {
        nary_post_order_iterator tmp;

        tmp = *this;
        advance();

        return tmp;
    }


    // -----------------------------------------------------------------
    // comparison
    // -----------------------------------------------------------------

    D_CONSTEXPR friend bool
    operator==(
        const nary_post_order_iterator& _a,
        const nary_post_order_iterator& _b
    ) noexcept
    {
        return (_a.m_current == _b.m_current);
    }

    D_CONSTEXPR friend bool
    operator!=(
        const nary_post_order_iterator& _a,
        const nary_post_order_iterator& _b
    ) noexcept
    {
        return !(_a == _b);
    }


private:
    // descend_leftmost_deepest
    //   walks first_child links until reaching a leaf,
    // returning that leaf.  Returns null if _start is null.
    // The starting node itself qualifies if it is already a
    // leaf.
    static node_type*
    descend_leftmost_deepest(
        node_type* _start
    ) noexcept
    {
        node_type* n;

        if (_start == nullptr)
        {
            return nullptr;
        }

        n = _start;

        while (n->first_child() != nullptr)
        {
            n = n->first_child();
        }

        return n;
    }

    // advance
    //   moves m_current to the next node in post-order:
    //     - if we are at the subtree root, the traversal is
    //       complete (the root is the last node yielded),
    //     - otherwise, if a next sibling exists, descend to
    //       its leftmost-deepest descendant,
    //     - otherwise, climb to the parent (which is the next
    //       node visited in post-order).
    void
    advance() noexcept
    {
        node_type* sib;

        if (m_current == nullptr)
        {
            return;
        }

        if (m_current == m_subtree_root)
        {
            m_current = nullptr;

            return;
        }

        sib = m_current->next_sibling();

        if (sib != nullptr)
        {
            m_current = descend_leftmost_deepest(sib);

            return;
        }

        m_current = m_current->parent();

        return;
    }

    node_type* m_current;
    node_type* m_subtree_root;
};


// =========================================================================
// IV.  LEVEL-ORDER ITERATOR
// =========================================================================

// nary_level_order_iterator
//   class: breadth-first forward iterator.  Holds a queue of
// pending nodes (vector + head index for cheap pop_front).
// Memory is O(maximum width of the tree at any level), which
// for balanced trees is roughly O(n / depth).
template<typename _NodeType>
class nary_level_order_iterator
{
private:
    using traits_type = internal::nary_iter_traits<_NodeType>;

public:
    using iterator_category = std::forward_iterator_tag;
    using node_type         = _NodeType;
    using value_type        = typename traits_type::value_type;
    using difference_type   = typename traits_type::difference_type;
    using reference         = typename traits_type::reference;
    using pointer           = typename traits_type::pointer;


    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    D_CONSTEXPR
    nary_level_order_iterator() noexcept
        : m_current(nullptr),
            m_queue(),
            m_head(0)
    {}

    D_CONSTEXPR explicit
    nary_level_order_iterator(
        node_type* _root
    )
        : m_current(nullptr),
            m_queue(),
            m_head(0)
    {
        if (_root != nullptr)
        {
            m_queue.push_back(_root);
            advance();
        }
    }


    // -----------------------------------------------------------------
    // element access
    // -----------------------------------------------------------------

    D_CONSTEXPR reference
    operator*() const noexcept
    {
        return m_current->data();
    }

    D_CONSTEXPR pointer
    operator->() const noexcept
    {
        return &m_current->data();
    }

    D_CONSTEXPR node_type*
    node() const noexcept
    {
        return m_current;
    }


    // -----------------------------------------------------------------
    // traversal
    // -----------------------------------------------------------------

    nary_level_order_iterator&
    operator++()
    {
        advance();

        return *this;
    }

    nary_level_order_iterator
    operator++(int)
    {
        nary_level_order_iterator tmp;

        tmp = *this;
        advance();

        return tmp;
    }


    // -----------------------------------------------------------------
    // comparison
    // -----------------------------------------------------------------

    D_CONSTEXPR friend bool
    operator==(
        const nary_level_order_iterator& _a,
        const nary_level_order_iterator& _b
    ) noexcept
    {
        return (_a.m_current == _b.m_current);
    }

    D_CONSTEXPR friend bool
    operator!=(
        const nary_level_order_iterator& _a,
        const nary_level_order_iterator& _b
    ) noexcept
    {
        return !(_a == _b);
    }


private:
    // advance
    //   pops the next pending node from the queue, makes it
    // the current node, then enqueues all of its children.
    // When the queue is exhausted, signals end of traversal.
    void
    advance()
    {
        node_type* c;

        if (m_head >= m_queue.size())
        {
            m_current = nullptr;

            return;
        }

        m_current = m_queue[m_head];
        ++m_head;

        c = m_current->first_child();

        while (c != nullptr)
        {
            m_queue.push_back(c);
            c = c->next_sibling();
        }

        return;
    }

    node_type*              m_current;
    std::vector<node_type*> m_queue;
    std::size_t             m_head;
};


// =========================================================================
// V.   LEAF-ONLY ITERATOR
// =========================================================================

// nary_leaf_iterator
//   class: forward iterator that yields only leaves, in DFS
// discovery order.  Equivalent to filtering a pre-order walk
// for nodes with no children, but implemented inline using
// parent links so no auxiliary stack is required.
template<typename _NodeType>
class nary_leaf_iterator
{
private:
    using traits_type = internal::nary_iter_traits<_NodeType>;

public:
    using iterator_category = std::forward_iterator_tag;
    using node_type         = _NodeType;
    using value_type        = typename traits_type::value_type;
    using difference_type   = typename traits_type::difference_type;
    using reference         = typename traits_type::reference;
    using pointer           = typename traits_type::pointer;


    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    D_CONSTEXPR
    nary_leaf_iterator() noexcept
        : m_current(nullptr),
            m_subtree_root(nullptr)
    {}

    D_CONSTEXPR explicit
    nary_leaf_iterator(
        node_type* _root
    ) noexcept
        : m_current(descend_leftmost_deepest(_root)),
            m_subtree_root(_root)
    {}


    // -----------------------------------------------------------------
    // element access
    // -----------------------------------------------------------------

    D_CONSTEXPR reference
    operator*() const noexcept
    {
        return m_current->data();
    }

    D_CONSTEXPR pointer
    operator->() const noexcept
    {
        return &m_current->data();
    }

    D_CONSTEXPR node_type*
    node() const noexcept
    {
        return m_current;
    }


    // -----------------------------------------------------------------
    // traversal
    // -----------------------------------------------------------------

    nary_leaf_iterator&
    operator++() noexcept
    {
        advance();

        return *this;
    }

    nary_leaf_iterator
    operator++(int) noexcept
    {
        nary_leaf_iterator tmp;

        tmp = *this;
        advance();

        return tmp;
    }


    // -----------------------------------------------------------------
    // comparison
    // -----------------------------------------------------------------

    D_CONSTEXPR friend bool
    operator==(
        const nary_leaf_iterator& _a,
        const nary_leaf_iterator& _b
    ) noexcept
    {
        return (_a.m_current == _b.m_current);
    }

    D_CONSTEXPR friend bool
    operator!=(
        const nary_leaf_iterator& _a,
        const nary_leaf_iterator& _b
    ) noexcept
    {
        return !(_a == _b);
    }


private:
    static node_type*
    descend_leftmost_deepest(
        node_type* _start
    ) noexcept
    {
        node_type* n;

        if (_start == nullptr)
        {
            return nullptr;
        }

        n = _start;

        while (n->first_child() != nullptr)
        {
            n = n->first_child();
        }

        return n;
    }

    // advance
    //   moves m_current to the next leaf.  Walks rootward
    // until finding an ancestor whose next sibling lies
    // within the subtree, then descends to that sibling's
    // leftmost-deepest descendant.  Stops when the climb
    // reaches the subtree root with nothing left to visit.
    void
    advance() noexcept
    {
        node_type* n;

        if (m_current == nullptr)
        {
            return;
        }

        n = m_current;

        while (true)
        {
            if (n == m_subtree_root)
            {
                m_current = nullptr;

                return;
            }

            if (n->next_sibling() != nullptr)
            {
                m_current =
                    descend_leftmost_deepest(n->next_sibling());

                return;
            }

            n = n->parent();

            if (n == nullptr)
            {
                m_current = nullptr;

                return;
            }
        }
    }

    node_type* m_current;
    node_type* m_subtree_root;
};


// =========================================================================
// VI.  DISPATCH ALIAS
// =========================================================================

NS_INTERNAL

    // nary_iterator_for
    //   trait: maps an order tag to the corresponding
    // iterator class.  Primary template is intentionally
    // undefined so unsupported tags surface as a compile
    // error rather than a silent fallback.
    template<typename _NodeType,
                typename _OrderTag>
    struct nary_iterator_for;

    template<typename _NodeType>
    struct nary_iterator_for<_NodeType, pre_order_tag>
    {
        using type = nary_pre_order_iterator<_NodeType>;
    };

    template<typename _NodeType>
    struct nary_iterator_for<_NodeType, post_order_tag>
    {
        using type = nary_post_order_iterator<_NodeType>;
    };

    template<typename _NodeType>
    struct nary_iterator_for<_NodeType, level_order_tag>
    {
        using type = nary_level_order_iterator<_NodeType>;
    };

    template<typename _NodeType>
    struct nary_iterator_for<_NodeType, leaf_order_tag>
    {
        using type = nary_leaf_iterator<_NodeType>;
    };

NS_END  // internal

// nary_tree_iterator
//   alias: dispatches to one of the four order-specific
// iterators based on _OrderTag.  Pre-order is the default,
// matching nary_tree::iterator.
template<typename _NodeType,
            typename _OrderTag = pre_order_tag>
using nary_tree_iterator =
    typename internal::nary_iterator_for<
        _NodeType, _OrderTag>::type;


// =========================================================================
// VII. CURSOR
// =========================================================================

// nary_tree_cursor
//   class: imperative LCRS navigator analogous to
// tree_cursor<Node> from tree_iterator.hpp (which targets
// binary trees).  Every navigation primitive runs in O(1)
// when the underlying node has the corresponding link, which
// for nary_tree_node<T> means parent / first_child /
// last_child / next_sibling / prev_sibling are all live.
//
//   The cursor is anchored to a root at construction time
// and tracks the current node.  go_*() methods return true
// on successful navigation and false when the requested
// direction has no link (or would leave the subtree, in the
// case of go_root).  set() bypasses navigation by jumping
// the cursor directly to a known node.
template<typename _NodeType>
class nary_tree_cursor
{
private:
    using traits_type = internal::nary_iter_traits<_NodeType>;

public:
    using node_type       = _NodeType;
    using value_type      = typename traits_type::value_type;
    using reference       = typename traits_type::reference;
    using pointer         = typename traits_type::pointer;
    using size_type       = std::size_t;


    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    D_CONSTEXPR
    nary_tree_cursor() noexcept
        : m_current(nullptr),
            m_root(nullptr)
    {}

    D_CONSTEXPR explicit
    nary_tree_cursor(
        node_type* _root
    ) noexcept
        : m_current(_root),
            m_root(_root)
    {}


    // -----------------------------------------------------------------
    // state queries
    // -----------------------------------------------------------------

    D_CONSTEXPR bool
    valid() const noexcept
    {
        return (m_current != nullptr);
    }

    D_CONSTEXPR explicit operator bool() const noexcept
    {
        return valid();
    }

    D_CONSTEXPR bool
    is_root() const noexcept
    {
        return (m_current == m_root);
    }

    D_CONSTEXPR bool
    is_leaf() const noexcept
    {
        return ( valid() &&
                    (m_current->first_child() == nullptr) );
    }

    D_CONSTEXPR bool
    has_parent() const noexcept
    {
        return ( valid() &&
                    (m_current != m_root) &&
                    (m_current->parent() != nullptr) );
    }

    D_CONSTEXPR bool
    has_first_child() const noexcept
    {
        return ( valid() &&
                    (m_current->first_child() != nullptr) );
    }

    D_CONSTEXPR bool
    has_last_child() const noexcept
    {
        return ( valid() &&
                    (m_current->last_child() != nullptr) );
    }

    D_CONSTEXPR bool
    has_next_sibling() const noexcept
    {
        return ( valid() &&
                    (m_current != m_root) &&
                    (m_current->next_sibling() != nullptr) );
    }

    D_CONSTEXPR bool
    has_prev_sibling() const noexcept
    {
        return ( valid() &&
                    (m_current != m_root) &&
                    (m_current->prev_sibling() != nullptr) );
    }


    // -----------------------------------------------------------------
    // measurements
    // -----------------------------------------------------------------

    // depth
    //   returns the depth of the current node measured from
    // the cursor's root anchor (root depth = 0).  O(depth).
    size_type
    depth() const noexcept
    {
        size_type        d;
        const node_type* n;

        if (!valid())
        {
            return 0;
        }

        d = 0;
        n = m_current;

        while ( (n != m_root) &&
                (n != nullptr) )
        {
            ++d;
            n = n->parent();
        }

        return d;
    }

    // child_count
    //   returns the number of direct children of the current
    // node.  O(child_count).
    size_type
    child_count() const noexcept
    {
        if (!valid())
        {
            return 0;
        }

        return m_current->child_count();
    }


    // -----------------------------------------------------------------
    // element access
    // -----------------------------------------------------------------

    D_CONSTEXPR reference
    data() const noexcept
    {
        return m_current->data();
    }

    D_CONSTEXPR node_type*
    node() const noexcept
    {
        return m_current;
    }


    // -----------------------------------------------------------------
    // navigation
    // -----------------------------------------------------------------

    // go_root
    //   resets the cursor to the anchored root.  Returns true
    // if the root is non-null.
    D_CONSTEXPR bool
    go_root() noexcept
    {
        m_current = m_root;

        return (m_current != nullptr);
    }

    // go_parent
    //   moves to the current node's parent unless the cursor
    // is already at the root.  Returns true on success.
    D_CONSTEXPR bool
    go_parent() noexcept
    {
        if (!has_parent())
        {
            return false;
        }

        m_current = m_current->parent();

        return true;
    }

    D_CONSTEXPR bool
    go_first_child() noexcept
    {
        if (!has_first_child())
        {
            return false;
        }

        m_current = m_current->first_child();

        return true;
    }

    D_CONSTEXPR bool
    go_last_child() noexcept
    {
        if (!has_last_child())
        {
            return false;
        }

        m_current = m_current->last_child();

        return true;
    }

    D_CONSTEXPR bool
    go_next_sibling() noexcept
    {
        if (!has_next_sibling())
        {
            return false;
        }

        m_current = m_current->next_sibling();

        return true;
    }

    D_CONSTEXPR bool
    go_prev_sibling() noexcept
    {
        if (!has_prev_sibling())
        {
            return false;
        }

        m_current = m_current->prev_sibling();

        return true;
    }

    // go_child_at
    //   walks the sibling chain of first_child to land on the
    // _index'th child (0-based).  Returns true on success.
    // O(_index).
    bool
    go_child_at(
        size_type _index
    ) noexcept
    {
        node_type* c;
        size_type  i;

        if (!has_first_child())
        {
            return false;
        }

        c = m_current->first_child();
        i = 0;

        while ( (c != nullptr) &&
                (i  < _index) )
        {
            c = c->next_sibling();
            ++i;
        }

        if (c == nullptr)
        {
            return false;
        }

        m_current = c;

        return true;
    }

    // set
    //   jumps the cursor directly to _node.  The caller is
    // responsible for ensuring _node belongs to the same
    // subtree as the cursor's anchored root; the cursor does
    // not validate this.
    D_CONSTEXPR void
    set(
        node_type* _node
    ) noexcept
    {
        m_current = _node;

        return;
    }

    // -----------------------------------------------------------------
    // comparison
    // -----------------------------------------------------------------

    D_CONSTEXPR friend bool
    operator==(
        const nary_tree_cursor& _a,
        const nary_tree_cursor& _b
    ) noexcept
    {
        return (_a.m_current == _b.m_current);
    }

    D_CONSTEXPR friend bool
    operator!=(
        const nary_tree_cursor& _a,
        const nary_tree_cursor& _b
    ) noexcept
    {
        return !(_a == _b);
    }


private:
    node_type* m_current;
    node_type* m_root;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_NARY_TREE_ITERATOR_