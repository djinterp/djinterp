/******************************************************************************
* djinterp [container]                                 linked_tree_iterator.hpp
*
*   The foundational LINKED-TREE iterator: a traversal of the DESCENT relation
* of a node-linked container.  Where the flat iterator visits a flat container's
* own
* positions, and the hierarchical iterator flattens one level of NESTED VALUES
* (the vector-of-vectors regime, F[F[tau]]), this walks the general composite
* structure T = tau + F[T] as it is actually realised in memory: nodes joined by
* parent / first-child / next-sibling links, an arena or a linked tree, whose
* depth is a property of the VALUE and is not recoverable from the type.
*
*   The three iterators are therefore complementary, not competing:
*
*     flat_iterator          positions of a flat container     (depth 1)
*     hierarchical_iterator  leaves beneath nested VALUES      (uniform nesting)
*     linked_tree_iterator   nodes of a LINKED tree, via policy (general case)
*
*   NOTE the name.  /core/container/tree/tree_iterator.hpp already exists and is
* the BINARY tree iterator (it supplies the order tags nary_tree_iterator.hpp
* uses).  This file is a different thing entirely -- a policy-navigated walk of
* any linked hierarchy -- and carries a distinct name and guard so the two never
* shadow one another.
*
*   NAVIGATION IS BY POLICY.  This header takes the same accessor policy that
* container_path.hpp defines, and requires nothing beyond it:
*
*     container_type / index_type / component_type
*     null_index()  is_null(i)  parent(c,i)  first_child(c,i)
*     next_sibling(c,i)         component(c,i)
*
* so a tree_iterator navigates any container -- arena, vector of records, flat
* array of links -- that an accessor policy can present as a hierarchy.  The
* policy is held BY VALUE and must therefore be cheap to copy.
*
*   THE SPEC (Structure, Addressability).  A container is a finite tree of
* components rooted at c, and n |> n' ("n' is a child of n") is its descent
* relation.  This header realises that relation and the traversals of it:
*
*     tree_child_iterator    the descent relation itself, one level: the
*                            children of a node, in sibling order.  This is the
*                            SCANNING strategy of the Addressability axis, and
*                            the primitive on which resolution is built.
*     tree_iterator          a traversal of the whole subtree beneath a node,
*                            in pre-order, post-order, or leaf (frontier) order.
*
* The ASCENT direction -- the unique path root |> ... |> n, the ancestor chain,
* the level and the address -- is the business of path_iterator.hpp, which reads
* the same policy in the opposite direction.  Between them the two exhaust |>.
*
*   The level reported by an iterator is measured from the traversal's OWN root,
* not from the container's; a traversal rooted at n reports n at level 0.  This
* is the spec's reading exactly: an address is relative to a designated root.
*
*   ITERABILITY (the spec, Iterability).
*     STAGE.   Observation and comparison are D_CONSTEXPR.  Advancing must climb
*              and descend -- a loop -- so traversal is constexpr from C++14
*              (relaxed constexpr) and a runtime operation before that, as in
*              the hierarchical iterator, and for the same reason.
*     MODE.    The traversal observes STRUCTURE and yields node handles; it is a
*              structural const traversal.  Values are reached through the
*              container by the handle, and their mutability is the container's
*              affair, not the iterator's.  Structural mutation (insertion,
*              erasure) invalidates any iterator over the affected subtree.
*
*   CATEGORY.  Forward.  A step back would need a previous-sibling link, which
* the policy does not require; scanning for one from first_child would cost O(k)
* in the branching factor, so no operator-- is offered rather than a dishonest
* one.  Should a policy grow prev_sibling, a bidirectional refinement is a
* strict addition to this file and breaks nothing in it.
*
*   PORTABILITY:
*   C++11 baseline.  Observation and comparison are constexpr throughout;
* construction and traversal are constexpr from C++14.
*
*
* path:      /inc/djinterp/core/container/iterator/tree/linked_tree_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    traversal Orders
II.   descent Primitives            (leaf, child count, leftmost leaf,
                                     find-child, separation)
III.  child Iterator                (the descent relation, one level)
IV.   subtree Iterator              (pre-order / post-order / leaf order)
V.    views and Factories
*/

#ifndef DJINTERP_CONTAINER_LINKED_TREE_ITERATOR_
#define DJINTERP_CONTAINER_LINKED_TREE_ITERATOR_ 1

// std
#include <cstddef>
#include <iterator>
// djinterp
#include "../../djinterp.hpp"   // D_CONSTEXPR, NS_*, feature macros


// D_ITER_CONSTEXPR_MUT
//   an operation that must loop -- climbing to a next sibling, descending to a
// leftmost leaf -- is constexpr only where a constexpr function may mutate,
// that is C++14 (relaxed constexpr) onward; before that it is a runtime step.
#ifndef D_ITER_CONSTEXPR_MUT
    #if ( D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL >= 201304L )
        #define D_ITER_CONSTEXPR_MUT  constexpr
    #else
        #define D_ITER_CONSTEXPR_MUT
    #endif
#endif


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    TRAVERSAL ORDERS                                      ///
///////////////////////////////////////////////////////////////////////////////

// tree_order
//   enum: the order in which a subtree traversal visits its nodes.
//
//     pre    a node before its children (document order).  The order in which
//            resolution descends, and the order an address is spelled.
//     post   a node after its children.  The order a fold bottoms out in: the
//            children of a node are complete before the node is reached.
//     leaf   the frontier only -- the elements of the container, in document
//            order, internal nodes skipped.
enum class tree_order
{
    pre,
    post,
    leaf
};


///////////////////////////////////////////////////////////////////////////////
///             II.   DESCENT PRIMITIVES                                    ///
///////////////////////////////////////////////////////////////////////////////
//   The free operations on a policy from which every traversal below is built.
// Each takes the policy first, as container_path.hpp's operations do, and every
// template argument is deducible from the call.

// tree_is_leaf
//   function: true if _node has no children -- a leaf, an element of the
// container in the sense of the spec's Structure.
template<typename _Policy,
         typename _Container,
         typename _Index>
D_CONSTEXPR
bool
tree_is_leaf
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _node
)
{
    return _policy.is_null(_policy.first_child(_container, _node));
}


// tree_child_count
//   function: the branching factor of _node -- the number of its children.  The
// cost of a scanning resolution step is linear in this.
template<typename _Policy,
         typename _Container,
         typename _Index>
D_ITER_CONSTEXPR_MUT
std::size_t
tree_child_count
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _node
)
{
    std::size_t count = 0;
    _Index      child = _policy.first_child(_container, _node);

    // walk the sibling chain of the first child
    while (!_policy.is_null(child))
    {
        ++count;

        child = _policy.next_sibling(_container, child);
    }

    return count;
}


// tree_leftmost_leaf
//   function: the leaf reached from _node by taking first children while any
// remain.  It is _node itself when _node is a leaf, and it is the first node of
// a post-order or leaf-order walk of the subtree beneath _node.
template<typename _Policy,
         typename _Container,
         typename _Index>
D_ITER_CONSTEXPR_MUT
_Index
tree_leftmost_leaf
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _node
)
{
    _Index node  = _node;
    _Index child = _policy.first_child(_container, node);

    // keep taking the first child until there is none
    while (!_policy.is_null(child))
    {
        node  = child;
        child = _policy.first_child(_container, node);
    }

    return node;
}


// tree_find_child
//   function: the child of _parent whose component equals _label, or the null
// sentinel if none bears it.
//
//   This is THE DESCENT STEP: one step of resolution, and the whole of what the
// scanning strategy of the Addressability axis does.  Resolving an address is
// this function folded along the address, and container_path_resolve is exactly
// that fold.  The step is O(k) in the branching factor; a node holding its
// children as a map from label to child would make it O(1), and a node holding
// them sorted by label O(log k) -- the native and ordered strategies.  All
// three compute the same descent; only the cost differs.
//
//   PRECONDITION (separation).  The result is well defined only where the
// children of _parent bear DISTINCT components: without that, an address does
// not determine what it names, and this function silently returns the first
// match.  See tree_children_separating.
template<typename _Policy,
         typename _Container,
         typename _Index,
         typename _Label>
D_ITER_CONSTEXPR_MUT
_Index
tree_find_child
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _parent,
    const _Label&     _label
)
{
    // a null parent has no children to search
    if (_policy.is_null(_parent))
    {
        return _policy.null_index();
    }

    _Index child = _policy.first_child(_container, _parent);

    // scan the sibling chain, comparing components
    while (!_policy.is_null(child))
    {
        if (_policy.component(_container, child) == _label)
        {
            return child;
        }

        child = _policy.next_sibling(_container, child);
    }

    return _policy.null_index();
}


// tree_children_separating
//   function: true if the children of _parent bear pairwise distinct
// components -- the SEPARATION condition of the Addressability axis, imposed at
// one node.
//
//   Separation is the multiplicity restriction mu_1 read sibling-wise: under it
// a node's children are a MAP from label to child, an address determines what
// it names, and resolve(addr(n)) == n.  Without it a label names a SET, and no
// resolution can invert addressing.  The check is O(k^2) in the branching
// factor, and is meant for a debug assertion or a one-off validation of a tree,
// not for the resolution path.
template<typename _Policy,
         typename _Container,
         typename _Index>
D_ITER_CONSTEXPR_MUT
bool
tree_children_separating
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _parent
)
{
    // a null parent trivially separates: it has no children to collide
    if (_policy.is_null(_parent))
    {
        return true;
    }

    _Index outer = _policy.first_child(_container, _parent);

    // compare each child against every later sibling
    while (!_policy.is_null(outer))
    {
        _Index inner = _policy.next_sibling(_container, outer);

        while (!_policy.is_null(inner))
        {
            if (_policy.component(_container, outer) ==
                _policy.component(_container, inner))
            {
                return false;
            }

            inner = _policy.next_sibling(_container, inner);
        }

        outer = _policy.next_sibling(_container, outer);
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///             III.  CHILD ITERATOR                                        ///
///////////////////////////////////////////////////////////////////////////////

// tree_child_iterator_end_tag
//   struct: disambiguates the past-the-end constructor of tree_child_iterator.
struct tree_child_iterator_end_tag
{
};


// tree_child_iterator
//   class: a forward iterator over the children of one node, in sibling order.
// It is the descent relation |> of the spec made traversable, and the sibling
// enumeration that a scanning resolution consumes.
//
//   Dereference yields the child's INDEX (a node handle), by const reference to
// the iterator's own state, so the reference is valid for the lifetime of the
// iterator and no longer.  The child's component and its payload are reached
// through the policy and the container respectively.
template<typename _Policy,
         typename _Container>
class tree_child_iterator
{
public:
    using policy_type       = _Policy;
    using container_type    = _Container;
    using index_type        = typename _Policy::index_type;
    using component_type    = typename _Policy::component_type;

    using value_type        = index_type;
    using reference         = const index_type&;
    using pointer           = const index_type*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    // ------------------------------------------------------------------
    //  construction
    // ------------------------------------------------------------------

    // tree_child_iterator (default)
    //   a singular iterator addressing nothing.  It compares equal only to
    // another singular iterator, never to a traversal's end.
    tree_child_iterator()
        : m_policy(),
          m_container(nullptr),
          m_node()
    {
    }

    // tree_child_iterator (begin)
    //   an iterator at the first child of _parent.
    D_ITER_CONSTEXPR_MUT
    tree_child_iterator(
        _Policy           _policy,
        const _Container& _container,
        index_type        _parent
    )
        : m_policy(_policy),
          m_container(&_container),
          m_node(_policy.first_child(_container, _parent))
    {
    }

    // tree_child_iterator (end)
    //   the past-the-end iterator of the children of any node of _container.
    D_ITER_CONSTEXPR_MUT
    tree_child_iterator(
        _Policy           _policy,
        const _Container& _container,
        tree_child_iterator_end_tag
    )
        : m_policy(_policy),
          m_container(&_container),
          m_node(_policy.null_index())
    {
    }

    // ------------------------------------------------------------------
    //  access (observing)
    // ------------------------------------------------------------------

    // operator*
    //   the index of the child at the current position.
    D_CONSTEXPR
    reference
    operator*() const
    {
        return m_node;
    }

    // operator->
    //   a pointer to the index of the child at the current position.
    D_CONSTEXPR
    pointer
    operator->() const
    {
        return &m_node;
    }

    // node
    //   the index of the child at the current position, by value.
    D_CONSTEXPR
    index_type
    node() const
    {
        return m_node;
    }

    // component
    //   the component (the label, in the sense of Addressability) of the child
    // at the current position.
    component_type
    component() const
    {
        return m_policy.component(*m_container, m_node);
    }

    // is_leaf
    //   true if the child at the current position has no children of its own.
    bool
    is_leaf() const
    {
        return tree_is_leaf(m_policy, *m_container, m_node);
    }

    // ------------------------------------------------------------------
    //  traversal
    // ------------------------------------------------------------------

    // operator++ (pre)
    //   advances to the next sibling.
    D_ITER_CONSTEXPR_MUT
    tree_child_iterator&
    operator++()
    {
        m_node = m_policy.next_sibling(*m_container, m_node);

        return *this;
    }

    // operator++ (post)
    D_ITER_CONSTEXPR_MUT
    tree_child_iterator
    operator++(int)
    {
        tree_child_iterator tmp(*this);

        ++(*this);

        return tmp;
    }

    // ------------------------------------------------------------------
    //  comparison
    // ------------------------------------------------------------------

    // operator==
    //   two iterators are equal when they rest on the same node of the same
    // container.  Comparing the container as well is what keeps a singular
    // iterator (which holds none) from ever comparing equal to a real end.
    D_CONSTEXPR
    bool
    operator==(const tree_child_iterator& _other) const
    {
        return ( (m_container == _other.m_container) &&
                 (m_node      == _other.m_node) );
    }

    // operator!=
    D_CONSTEXPR
    bool
    operator!=(const tree_child_iterator& _other) const
    {
        return !(*this == _other);
    }

private:
    _Policy           m_policy;
    const _Container* m_container;
    index_type        m_node;
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   SUBTREE ITERATOR                                      ///
///////////////////////////////////////////////////////////////////////////////

// tree_iterator_end_tag
//   struct: disambiguates the past-the-end constructor of tree_iterator.
struct tree_iterator_end_tag
{
};


// tree_iterator
//   class: a forward iterator over every node of the subtree rooted at a given
// node, in one of the three orders of tree_order.
//
//   The walk needs no stack: the parent link supplies the way back up, so the
// iterator carries only the current node, the root that bounds it, and the
// level.  The root bound is what keeps the traversal inside its subtree -- the
// climb stops there and never follows the root's own siblings.
//
//   The LEVEL is measured from the traversal's root, which is at level 0.  It
// is maintained incrementally, at no cost, and is exactly the lambda of the
// Addressability axis: the length of the path from the root to the current
// node, and hence the length of its address.
//
//   Dereference yields the current node's INDEX, by const reference to the
// iterator's own state (see tree_child_iterator).
template<typename   _Policy,
         typename   _Container,
         tree_order _Order = tree_order::pre>
class tree_iterator
{
public:
    using policy_type       = _Policy;
    using container_type    = _Container;
    using index_type        = typename _Policy::index_type;
    using component_type    = typename _Policy::component_type;

    using value_type        = index_type;
    using reference         = const index_type&;
    using pointer           = const index_type*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    // order
    //   the traversal order of this iterator, as a compile-time constant.  A
    // static constexpr FUNCTION, not a data member: a static constexpr data
    // member of class type needs an out-of-line definition before C++17 if it
    // is ever odr-used, and this must not force one on the caller.
    static D_CONSTEXPR
    tree_order
    order()
    {
        return _Order;
    }

    // ------------------------------------------------------------------
    //  construction
    // ------------------------------------------------------------------

    // tree_iterator (default)
    //   a singular iterator addressing nothing.  It compares equal only to
    // another singular iterator, never to a traversal's end.
    tree_iterator()
        : m_policy(),
          m_container(nullptr),
          m_node(),
          m_root(),
          m_level(0)
    {
    }

    // tree_iterator (begin)
    //   an iterator at the first node of the traversal of the subtree rooted at
    // _root: _root itself in pre-order, its leftmost leaf otherwise.  A null
    // root yields an iterator already at its end.
    D_ITER_CONSTEXPR_MUT
    tree_iterator(
        _Policy           _policy,
        const _Container& _container,
        index_type        _root
    )
        : m_policy(_policy),
          m_container(&_container),
          m_node(_root),
          m_root(_root),
          m_level(0)
    {
        settle_begin();
    }

    // tree_iterator (end)
    //   the past-the-end iterator of the traversal rooted at _root.
    D_ITER_CONSTEXPR_MUT
    tree_iterator(
        _Policy           _policy,
        const _Container& _container,
        index_type        _root,
        tree_iterator_end_tag
    )
        : m_policy(_policy),
          m_container(&_container),
          m_node(_policy.null_index()),
          m_root(_root),
          m_level(0)
    {
    }

    // ------------------------------------------------------------------
    //  access (observing)
    // ------------------------------------------------------------------

    // operator*
    //   the index of the node at the current position.
    D_CONSTEXPR
    reference
    operator*() const
    {
        return m_node;
    }

    // operator->
    //   a pointer to the index of the node at the current position.
    D_CONSTEXPR
    pointer
    operator->() const
    {
        return &m_node;
    }

    // node
    //   the index of the node at the current position, by value.
    D_CONSTEXPR
    index_type
    node() const
    {
        return m_node;
    }

    // root
    //   the node the traversal is rooted at, and which bounds it.
    D_CONSTEXPR
    index_type
    root() const
    {
        return m_root;
    }

    // level
    //   the level of the current node, measured from the traversal's root: the
    // number of descents taken to reach it, and the length of its address.
    D_CONSTEXPR
    std::size_t
    level() const
    {
        return m_level;
    }

    // component
    //   the component (the label) of the node at the current position.  The
    // root of a traversal carries a component like any other node, but it
    // contributes NO label to an address -- addressing starts from it.
    component_type
    component() const
    {
        return m_policy.component(*m_container, m_node);
    }

    // is_leaf
    //   true if the node at the current position has no children.
    bool
    is_leaf() const
    {
        return tree_is_leaf(m_policy, *m_container, m_node);
    }

    // ------------------------------------------------------------------
    //  traversal
    // ------------------------------------------------------------------

    // operator++ (pre)
    //   advances to the next node of the order.
    D_ITER_CONSTEXPR_MUT
    tree_iterator&
    operator++()
    {
        advance();

        return *this;
    }

    // operator++ (post)
    D_ITER_CONSTEXPR_MUT
    tree_iterator
    operator++(int)
    {
        tree_iterator tmp(*this);

        ++(*this);

        return tmp;
    }

    // ------------------------------------------------------------------
    //  comparison
    // ------------------------------------------------------------------

    // operator==
    //   two iterators are equal when they rest on the same node of the same
    // container.  The level is a function of the node and so is not compared.
    D_CONSTEXPR
    bool
    operator==(const tree_iterator& _other) const
    {
        return ( (m_container == _other.m_container) &&
                 (m_node      == _other.m_node) );
    }

    // operator!=
    D_CONSTEXPR
    bool
    operator!=(const tree_iterator& _other) const
    {
        return !(*this == _other);
    }

private:
    // ------------------------------------------------------------------
    //  descent helpers
    // ------------------------------------------------------------------

    // mark_end
    //   places the iterator past the last node of the traversal.
    D_ITER_CONSTEXPR_MUT
    void
    mark_end()
    {
        m_node  = m_policy.null_index();
        m_level = 0;

        return;
    }

    // descend_leftmost
    //   descends from the current node by first children while any remain,
    // maintaining the level.  Lands on the leftmost leaf beneath it.
    D_ITER_CONSTEXPR_MUT
    void
    descend_leftmost()
    {
        index_type child = m_policy.first_child(*m_container, m_node);

        // keep taking the first child until there is none
        while (!m_policy.is_null(child))
        {
            m_node = child;
            ++m_level;

            child = m_policy.first_child(*m_container, m_node);
        }

        return;
    }

    // settle_begin
    //   positions a freshly constructed iterator at the first node of its
    // order: the root in pre-order, the leftmost leaf in post-order and leaf
    // order (which open at the same node, the deepest-first of the subtree).
    D_ITER_CONSTEXPR_MUT
    void
    settle_begin()
    {
        // a null-rooted traversal is empty and already at its end
        if (m_policy.is_null(m_node))
        {
            mark_end();

            return;
        }

        // pre-order opens at the root; the other two open at the leftmost leaf
        if (_Order != tree_order::pre)
        {
            descend_leftmost();
        }

        return;
    }

    // advance_pre
    //   steps to the next node in pre-order: the first child if there is one;
    // otherwise the next sibling of the nearest ancestor that has one, the
    // climb stopping at the root that bounds the traversal.
    D_ITER_CONSTEXPR_MUT
    void
    advance_pre()
    {
        index_type child = m_policy.first_child(*m_container, m_node);

        // a node is followed by its own subtree
        if (!m_policy.is_null(child))
        {
            m_node = child;
            ++m_level;

            return;
        }

        index_type  node  = m_node;
        std::size_t level = m_level;

        // climb until a next sibling appears, or the root is reached
        while (!(node == m_root))
        {
            index_type sibling = m_policy.next_sibling(*m_container, node);

            if (!m_policy.is_null(sibling))
            {
                m_node  = sibling;
                m_level = level;

                return;
            }

            node = m_policy.parent(*m_container, node);

            // a parent chain that misses the root is malformed; end the walk
            if (m_policy.is_null(node))
            {
                break;
            }

            --level;
        }

        mark_end();

        return;
    }

    // advance_post
    //   steps to the next node in post-order: the root is last; otherwise the
    // leftmost leaf of the next sibling, or -- there being no next sibling --
    // the parent, whose children are now complete.
    D_ITER_CONSTEXPR_MUT
    void
    advance_post()
    {
        // the root is the last node a post-order walk visits
        if (m_node == m_root)
        {
            mark_end();

            return;
        }

        index_type sibling = m_policy.next_sibling(*m_container, m_node);

        // no next sibling: this node completed its parent
        if (m_policy.is_null(sibling))
        {
            m_node = m_policy.parent(*m_container, m_node);

            // a parent chain that misses the root is malformed; end the walk
            if (m_policy.is_null(m_node))
            {
                mark_end();

                return;
            }

            --m_level;

            return;
        }

        // otherwise the next subtree opens at its own leftmost leaf
        m_node = sibling;

        descend_leftmost();

        return;
    }

    // advance_leaf
    //   steps to the next leaf.  A pre-order step from a leaf lands on the next
    // subtree in document order, and the leftmost leaf of that subtree is the
    // next leaf -- so the leaf order is the pre-order step composed with a
    // descent, and it visits the frontier of the container exactly.
    D_ITER_CONSTEXPR_MUT
    void
    advance_leaf()
    {
        advance_pre();

        // the walk may have ended; there is nothing left to descend into
        if (!m_policy.is_null(m_node))
        {
            descend_leftmost();
        }

        return;
    }

    // advance
    //   steps to the next node of the traversal.  _Order is a template
    // constant, so the branch below folds and costs nothing at runtime.
    D_ITER_CONSTEXPR_MUT
    void
    advance()
    {
        // an iterator already at its end does not advance
        if (m_policy.is_null(m_node))
        {
            return;
        }

        if (_Order == tree_order::post)
        {
            advance_post();
        }
        else if (_Order == tree_order::leaf)
        {
            advance_leaf();
        }
        else
        {
            advance_pre();
        }

        return;
    }

    _Policy           m_policy;
    const _Container* m_container;
    index_type        m_node;
    index_type        m_root;
    std::size_t       m_level;
};


///////////////////////////////////////////////////////////////////////////////
///             V.    VIEWS AND FACTORIES                                   ///
///////////////////////////////////////////////////////////////////////////////

// tree_child_view
//   class: a range over the children of one node, so the descent relation may
// drive a range-for.  Holds a policy, a container pointer, and the parent; it
// owns nothing.
template<typename _Policy,
         typename _Container>
class tree_child_view
{
public:
    using iterator     = tree_child_iterator<_Policy, _Container>;
    using index_type   = typename _Policy::index_type;

    // tree_child_view
    //   constructs a view of the children of _parent.
    tree_child_view(
        _Policy           _policy,
        const _Container& _container,
        index_type        _parent
    )
        : m_policy(_policy),
          m_container(&_container),
          m_parent(_parent)
    {
    }

    // begin
    //   an iterator at the first child.
    D_ITER_CONSTEXPR_MUT
    iterator
    begin() const
    {
        return iterator(m_policy, *m_container, m_parent);
    }

    // end
    //   the past-the-end iterator.
    D_ITER_CONSTEXPR_MUT
    iterator
    end() const
    {
        return iterator(m_policy,
                        *m_container,
                        tree_child_iterator_end_tag());
    }

    // empty
    //   true if _parent is a leaf.
    bool
    empty() const
    {
        return tree_is_leaf(m_policy, *m_container, m_parent);
    }

    // size
    //   the branching factor of _parent.  Linear in the result.
    std::size_t
    size() const
    {
        return tree_child_count(m_policy, *m_container, m_parent);
    }

private:
    _Policy           m_policy;
    const _Container* m_container;
    index_type        m_parent;
};


// tree_view
//   class: a range over the subtree rooted at a node, in one of the three
// orders, so a traversal may drive a range-for.  Holds a policy, a container
// pointer, and the root; it owns nothing.
template<typename   _Policy,
         typename   _Container,
         tree_order _Order = tree_order::pre>
class tree_view
{
public:
    using iterator     = tree_iterator<_Policy, _Container, _Order>;
    using index_type   = typename _Policy::index_type;

    // order
    //   the traversal order of this view, as a compile-time constant.
    static D_CONSTEXPR
    tree_order
    order()
    {
        return _Order;
    }

    // tree_view
    //   constructs a view of the subtree rooted at _root.
    tree_view(
        _Policy           _policy,
        const _Container& _container,
        index_type        _root
    )
        : m_policy(_policy),
          m_container(&_container),
          m_root(_root)
    {
    }

    // begin
    //   an iterator at the first node of the order.
    D_ITER_CONSTEXPR_MUT
    iterator
    begin() const
    {
        return iterator(m_policy, *m_container, m_root);
    }

    // end
    //   the past-the-end iterator.
    D_ITER_CONSTEXPR_MUT
    iterator
    end() const
    {
        return iterator(m_policy,
                        *m_container,
                        m_root,
                        tree_iterator_end_tag());
    }

    // empty
    //   true if the traversal visits no node -- that is, if the root is null.
    // A non-null root always yields at least itself, in every order.
    bool
    empty() const
    {
        return m_policy.is_null(m_root);
    }

private:
    _Policy           m_policy;
    const _Container* m_container;
    index_type        m_root;
};


// make_tree_iterator
//   factory: an iterator at the first node of the traversal of the subtree
// rooted at _root.  The order is explicit; it cannot be deduced.
//
// Usage:
//   auto it = make_tree_iterator<tree_order::post>(policy, arena, root);
template<tree_order _Order,
         typename   _Policy,
         typename   _Container,
         typename   _Index>
D_ITER_CONSTEXPR_MUT
tree_iterator<_Policy, _Container, _Order>
make_tree_iterator
(
    _Policy           _policy,
    const _Container& _container,
    _Index            _root
)
{
    return tree_iterator<_Policy, _Container, _Order>(
        _policy,
        _container,
        _root);
}


// make_tree_view
//   factory: a range over the subtree rooted at _root, in the given order.
//
// Usage:
//   for (auto n : make_tree_view<tree_order::leaf>(policy, arena, root))
//   {
//       ...
//   }
template<tree_order _Order,
         typename   _Policy,
         typename   _Container,
         typename   _Index>
tree_view<_Policy, _Container, _Order>
make_tree_view
(
    _Policy           _policy,
    const _Container& _container,
    _Index            _root
)
{
    return tree_view<_Policy, _Container, _Order>(
        _policy,
        _container,
        _root);
}


// make_preorder_view
//   factory: a range over the subtree rooted at _root, each node before its
// children -- document order, the order in which an address is spelled.
template<typename _Policy,
         typename _Container,
         typename _Index>
tree_view<_Policy, _Container, tree_order::pre>
make_preorder_view
(
    _Policy           _policy,
    const _Container& _container,
    _Index            _root
)
{
    return tree_view<_Policy, _Container, tree_order::pre>(
        _policy,
        _container,
        _root);
}


// make_postorder_view
//   factory: a range over the subtree rooted at _root, each node after its
// children -- the order in which a fold over the tree bottoms out.
template<typename _Policy,
         typename _Container,
         typename _Index>
tree_view<_Policy, _Container, tree_order::post>
make_postorder_view
(
    _Policy           _policy,
    const _Container& _container,
    _Index            _root
)
{
    return tree_view<_Policy, _Container, tree_order::post>(
        _policy,
        _container,
        _root);
}


// make_leaf_view
//   factory: a range over the frontier of the subtree rooted at _root -- its
// leaves only, in document order.  This is the elements of the container in the
// sense of the spec, the node summand skipped.
template<typename _Policy,
         typename _Container,
         typename _Index>
tree_view<_Policy, _Container, tree_order::leaf>
make_leaf_view
(
    _Policy           _policy,
    const _Container& _container,
    _Index            _root
)
{
    return tree_view<_Policy, _Container, tree_order::leaf>(
        _policy,
        _container,
        _root);
}


// make_child_view
//   factory: a range over the children of _parent -- the descent relation
// itself, one level.
template<typename _Policy,
         typename _Container,
         typename _Index>
tree_child_view<_Policy, _Container>
make_child_view
(
    _Policy           _policy,
    const _Container& _container,
    _Index            _parent
)
{
    return tree_child_view<_Policy, _Container>(
        _policy,
        _container,
        _parent);
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_LINKED_TREE_ITERATOR_
