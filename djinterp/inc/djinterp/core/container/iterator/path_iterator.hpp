/******************************************************************************
* djinterp [container]                                        path_iterator.hpp
*
*   The PATH iterator: a traversal of the descent relation UPWARD -- the unique
* chain of components from a node back to its root.  It is the exact converse of
* linked_tree_iterator.hpp, which walks the same relation downward, and between
* them the two exhaust it.
*
*     linked_tree_iterator   the descent |>  : root -> ... -> node   (downward)
*     path_iterator          the ascent  |>^-1: node -> ... -> root  (upward)
*
*   THE SPEC (Structure, Addressability).  Because the components of a container
* form a finite tree, THE PATH TO A COMPONENT EXISTS AND IS UNIQUE:
*
*       path(n)  =  ( c |> n_1 |> n_2 |> ... |> n_N ),      n_N = n
*
* so paths and components are in bijection, and a path may be identified with
* the component it ends at.  Its length N is the LEVEL of n, written lambda(n),
* and it is also the length of n's ADDRESS -- the word of labels along the path,
*
*       addr(n)  =  < gamma(n_1), gamma(n_2), ..., gamma(n_N) >.
*
*   THE ANCHOR CONTRIBUTES NO LABEL.  Addressing STARTS from the root; the root
* is not a step taken.  So addr(c) is the empty word, |addr(n)| == lambda(n),
* and
* an iterator over path(n) yields lambda(n) + 1 NODES but only lambda(n) LABELS.
* Conflating the two is the single commonest error in a path implementation, and
* it is what container_path_address exists to get right.
*
*   THE ASCENT IS LOCAL; THE DESCENT ALONG A PATH IS NOT.  Each step upward is
* one parent link, so this iterator needs no storage and runs in O(1) space.
* Downward is not symmetric: from an ancestor there is NO WAY to know which of
* its children leads to the target without consulting the target.  The
* root-first
* reading of a path is therefore not a cheap reversal -- it is either the ascent
* MATERIALISED (container_path_ancestor_chain) or the address RESOLVED
* (resolve(addr(n))), and the second is what resolution has always been.  This
* is
* the same asymmetry that makes level order the one non-local tree traversal.
*
*   ANCHORING.  A path iterator is bounded by an ANCHOR, which it yields last.
* Anchoring at the container's root gives the absolute path; anchoring at any
* other ancestor gives the path RELATIVE to that ancestor, and the level it
* reports is relative to it too -- which is the spec's reading exactly: an
* address is relative to a designated root.  A node that is not in the anchor's
* subtree has no path to it, and the iterator is empty.
*
*   NAVIGATION IS BY POLICY.  The same accessor policy that container_path.hpp
* and linked_tree_iterator.hpp take; only parent(), is_null(), null_index(), and
* component() are used here.  The policy is held BY VALUE and must be cheap to
* copy.
*
*   CATEGORY.  Forward.  A step back down would need to know the target, which
* an
* iterator that has already left it does not; see the asymmetry above.
*
*   PORTABILITY:
*   C++11 baseline.  Observation and comparison are constexpr throughout;
* construction and traversal are constexpr from C++14.
*
*
* TABLE OF CONTENTS
* =================
* I.    Ascent Primitives           (level, reachability)
* II.   Path Iterator               (the ascent, lazily)
* III.  Path View and Factories
*
*
* path:      /inc/djinterp/core/container/iterator/path_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.12
******************************************************************************/

#ifndef DJINTERP_CONTAINER_PATH_ITERATOR_
#define DJINTERP_CONTAINER_PATH_ITERATOR_ 1

// std
#include <cstddef>
#include <iterator>
// djinterp
#include "../../djinterp.hpp"   // D_CONSTEXPR, NS_*, feature macros


// D_ITER_CONSTEXPR_MUT
//   an operation that must loop -- climbing a parent chain -- is constexpr only
// where a constexpr function may mutate, that is C++14 (relaxed constexpr)
// onward; before that it is a runtime step.
#ifndef D_ITER_CONSTEXPR_MUT
    #if ( D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL >= 201304L )
        #define D_ITER_CONSTEXPR_MUT  constexpr
    #else
        #define D_ITER_CONSTEXPR_MUT
    #endif
#endif


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    ASCENT PRIMITIVES                                     ///
///////////////////////////////////////////////////////////////////////////////

// path_reaches
//   function: true if _anchor lies on the parent chain of _node -- that is, if
// _node is _anchor or a descendant of it, and so HAS a path to _anchor.
//
//   This is the ANCESTOR relation, and by the spec it is the PREFIX ORDER
// transported along addr: _anchor is an ancestor of _node exactly when
// addr(_anchor) is a prefix of addr(_node).  Walking the chain is the cheap way
// to decide it; comparing addresses is the same question asked twice.
template<typename _Policy,
         typename _Container,
         typename _Index>
D_ITER_CONSTEXPR_MUT
bool
path_reaches
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _node,
    _Index            _anchor
)
{
    _Index node = _node;

    // climb the parent chain looking for the anchor
    while (!_policy.is_null(node))
    {
        if (node == _anchor)
        {
            return true;
        }

        node = _policy.parent(_container, node);
    }

    return false;
}


// path_level
//   function: the LEVEL (lambda) of _node measured from _anchor, which is at
// level 0 -- the number of descents from the anchor down to the node, and hence
// the length of the node's address.  O(lambda).
//
//   Returns 0 when _node does not reach _anchor; use path_reaches to tell that
// case apart from the node BEING the anchor.  This is NOT the depth of the
// spec,
// which is a node's HEIGHT, measured downward to its deepest leaf: level counts
// upward, height counts downward, and they agree only at the anchor.
template<typename _Policy,
         typename _Container,
         typename _Index>
D_ITER_CONSTEXPR_MUT
std::size_t
path_level
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _node,
    _Index            _anchor
)
{
    std::size_t level;
    _Index      node;

    level = 0;
    node  = _node;

    // count descents from the anchor by climbing to it
    while (!_policy.is_null(node))
    {
        if (node == _anchor)
        {
            return level;
        }

        ++level;

        node = _policy.parent(_container, node);
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
///             II.   PATH ITERATOR                                         ///
///////////////////////////////////////////////////////////////////////////////

// path_iterator_end_tag
//   struct: disambiguates the past-the-end constructor of path_iterator.
struct path_iterator_end_tag
{
};


// path_iterator
//   class: a forward iterator over path(n) -- the unique chain of components
// from a node up to its anchor -- yielded LEAF-FIRST, the node itself first and
// the anchor last.
//
//   The anchor IS yielded: path(n) is the chain (c |> ... |> n), and c is one
// of
// its components.  What the anchor does not contribute is a LABEL; see
// container_path_address, which walks this iterator and skips exactly one node.
// An iterator over the path of a node at level N therefore yields N + 1 nodes.
//
//   Dereference yields the node's INDEX, by const reference to the iterator's
// own state, so the reference is valid for the lifetime of the iterator and no
// longer -- the same contract the other iterators of this family keep.
//
//   The LEVEL is exact and O(1) to read.  It is established once, at
// construction, by a single climb to the anchor (O(lambda) -- the same cost the
// traversal itself will pay), and thereafter decremented, reaching 0 at the
// anchor.  A node that does not reach the anchor has no path to it, and the
// iterator is constructed already at its end.
template<typename _Policy,
         typename _Container>
class path_iterator
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

    // path_iterator (default)
    //   a singular iterator addressing nothing.  It compares equal only to
    // another singular iterator, never to a real traversal's end.
    path_iterator()
        : m_policy(),
          m_container(nullptr),
          m_node(),
          m_anchor(),
          m_level(0)
    {
    }

    // path_iterator (begin)
    //   an iterator at _node, the first component of path(_node).  If _node
    // does
    // not reach _anchor it has no path to it, and the iterator is already at
    // its
    // end.
    D_ITER_CONSTEXPR_MUT
    path_iterator(
        _Policy           _policy,
        const _Container& _container,
        index_type        _node,
        index_type        _anchor
    )
        : m_policy(_policy),
          m_container(&_container),
          m_node(_node),
          m_anchor(_anchor),
          m_level(0)
    {
        settle_begin();
    }

    // path_iterator (end)
    //   the past-the-end iterator of any path anchored at _anchor.
    D_ITER_CONSTEXPR_MUT
    path_iterator(
        _Policy           _policy,
        const _Container& _container,
        index_type        _anchor,
        path_iterator_end_tag
    )
        : m_policy(_policy),
          m_container(&_container),
          m_node(_policy.null_index()),
          m_anchor(_anchor),
          m_level(0)
    {
    }

    // ------------------------------------------------------------------
    //  access (observing)
    // ------------------------------------------------------------------

    // operator*
    //   the index of the component at the current position.
    D_CONSTEXPR
    reference
    operator*() const
    {
        return m_node;
    }

    // operator->
    //   a pointer to the index of the component at the current position.
    D_CONSTEXPR
    pointer
    operator->() const
    {
        return &m_node;
    }

    // node
    //   the index of the component at the current position, by value.
    D_CONSTEXPR
    index_type
    node() const
    {
        return m_node;
    }

    // anchor
    //   the node the path is anchored at, which the walk yields last.
    D_CONSTEXPR
    index_type
    anchor() const
    {
        return m_anchor;
    }

    // level
    //   the level (lambda) of the current component, measured from the anchor,
    // which is at level 0.  Maintained incrementally; O(1).
    D_CONSTEXPR
    std::size_t
    level() const
    {
        return m_level;
    }

    // is_anchor
    //   true if the walk is resting on the anchor -- the last component of the
    // path, and the one that contributes NO label to an address.
    D_CONSTEXPR
    bool
    is_anchor() const
    {
        return (m_node == m_anchor);
    }

    // component
    //   the component (the label, in the sense of Addressability) of the node
    // at
    // the current position.
    //
    //   Reading this AT THE ANCHOR is almost always a mistake: the anchor is
    // where addressing starts, not a step taken, so its label is not part of
    // any
    // address rooted there.  Guard with is_anchor().
    component_type
    component() const
    {
        return m_policy.component(*m_container, m_node);
    }

    // ------------------------------------------------------------------
    //  traversal
    // ------------------------------------------------------------------

    // operator++ (pre)
    //   ascends one link, to the parent.  Stepping off the anchor ends the
    // walk.
    D_ITER_CONSTEXPR_MUT
    path_iterator&
    operator++()
    {
        advance();

        return *this;
    }

    // operator++ (post)
    D_ITER_CONSTEXPR_MUT
    path_iterator
    operator++(int)
    {
        path_iterator tmp(*this);

        advance();

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
    operator==(const path_iterator& _other) const
    {
        return ( (m_container == _other.m_container) &&
                 (m_node      == _other.m_node) );
    }

    // operator!=
    D_CONSTEXPR
    bool
    operator!=(const path_iterator& _other) const
    {
        return !(*this == _other);
    }

private:
    // mark_end
    //   places the iterator past the last component of the path.
    D_ITER_CONSTEXPR_MUT
    void
    mark_end()
    {
        m_node  = m_policy.null_index();
        m_level = 0;

        return;
    }

    // settle_begin
    //   establishes the level by one climb to the anchor.  A node that never
    // meets the anchor has no path to it, and the walk is empty.
    D_ITER_CONSTEXPR_MUT
    void
    settle_begin()
    {
        std::size_t level;
        index_type  node;

        level = 0;
        node  = m_node;

        // climb to the anchor, counting descents
        while (!m_policy.is_null(node))
        {
            if (node == m_anchor)
            {
                m_level = level;

                return;
            }

            ++level;

            node = m_policy.parent(*m_container, node);
        }

        // the anchor is not on this node's parent chain: there is no path
        mark_end();

        return;
    }

    // advance
    //   ascends to the parent.  The anchor is the last component yielded, so
    // stepping off it ends the walk -- the walk must never climb past its own
    // anchor, whose own ancestors lie outside the path.
    D_ITER_CONSTEXPR_MUT
    void
    advance()
    {
        // an iterator already at its end does not advance
        if (m_policy.is_null(m_node))
        {
            return;
        }

        // the anchor is the last component of the path
        if (m_node == m_anchor)
        {
            mark_end();

            return;
        }

        m_node = m_policy.parent(*m_container, m_node);

        // a parent chain that misses the anchor is malformed; end the walk
        if (m_policy.is_null(m_node))
        {
            mark_end();

            return;
        }

        --m_level;

        return;
    }

    _Policy           m_policy;
    const _Container* m_container;
    index_type        m_node;
    index_type        m_anchor;
    std::size_t       m_level;
};


///////////////////////////////////////////////////////////////////////////////
///             III.  PATH VIEW AND FACTORIES                               ///
///////////////////////////////////////////////////////////////////////////////

// path_view
//   class: a range over path(_node) -- the chain of components from _node up to
// _anchor, leaf-first -- so a path may drive a range-for.  Holds a policy, a
// container pointer, the node, and the anchor; it owns nothing, and it
// allocates
// nothing.  This is what replaces materialising an ancestor chain into a vector
// merely to walk it once.
template<typename _Policy,
         typename _Container>
class path_view
{
public:
    using iterator   = path_iterator<_Policy, _Container>;
    using index_type = typename _Policy::index_type;

    // path_view
    //   constructs a view of path(_node), anchored at _anchor.
    path_view(
        _Policy           _policy,
        const _Container& _container,
        index_type        _node,
        index_type        _anchor
    )
        : m_policy(_policy),
          m_container(&_container),
          m_node(_node),
          m_anchor(_anchor)
    {
    }

    // begin
    //   an iterator at _node, the first component of the path.
    D_ITER_CONSTEXPR_MUT
    iterator
    begin() const
    {
        return iterator(m_policy, *m_container, m_node, m_anchor);
    }

    // end
    //   the past-the-end iterator.
    D_ITER_CONSTEXPR_MUT
    iterator
    end() const
    {
        return iterator(m_policy,
                        *m_container,
                        m_anchor,
                        path_iterator_end_tag());
    }

    // empty
    //   true if _node has no path to _anchor -- that is, if it does not reach
    // it.  A node that DOES reach the anchor always yields at least itself.
    bool
    empty() const
    {
        return !path_reaches(m_policy, *m_container, m_node, m_anchor);
    }

    // level
    //   the level (lambda) of _node, and so the length of its ADDRESS.  The
    // path itself yields one more component than this -- the anchor, which
    // contributes no label.
    std::size_t
    level() const
    {
        return path_level(m_policy, *m_container, m_node, m_anchor);
    }

    // size
    //   the number of COMPONENTS on the path, which is level() + 1: a path of
    // length N names N + 1 components, the anchor included.  Named apart from
    // level() precisely because the two differ by exactly one, and that one is
    // the anchor.
    std::size_t
    size() const
    {
        if (empty())
        {
            return 0;
        }

        return (level() + 1);
    }

private:
    _Policy           m_policy;
    const _Container* m_container;
    index_type        m_node;
    index_type        m_anchor;
};


// make_path_view
//   factory: a range over path(_node) anchored at _anchor -- the components
// from
// _node up to and including _anchor, leaf-first.
//
// Usage:
//   for (auto n : make_path_view(policy, arena, node, root))
//   {
//       ...
//   }
template<typename _Policy,
         typename _Container,
         typename _Index>
path_view<_Policy, _Container>
make_path_view
(
    _Policy           _policy,
    const _Container& _container,
    _Index            _node,
    _Index            _anchor
)
{
    return path_view<_Policy, _Container>(
        _policy,
        _container,
        _node,
        _anchor);
}


// make_path_iterator
//   factory: an iterator at _node, the first component of path(_node).
template<typename _Policy,
         typename _Container,
         typename _Index>
D_ITER_CONSTEXPR_MUT
path_iterator<_Policy, _Container>
make_path_iterator
(
    _Policy           _policy,
    const _Container& _container,
    _Index            _node,
    _Index            _anchor
)
{
    return path_iterator<_Policy, _Container>(
        _policy,
        _container,
        _node,
        _anchor);
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_PATH_ITERATOR_
