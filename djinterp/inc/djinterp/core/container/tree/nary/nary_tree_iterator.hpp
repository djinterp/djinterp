/*******************************************************************************
* djinterp [core]                                         nary_tree_iterator.hpp
*
* Iterators and a cursor for LCRS n-ary trees:
*   This header provides iterators over an LCRS-shaped node graph in four
* traversal orders, plus an imperative navigator analogous to `tree_cursor`
* from `tree_iterator.hpp` (which is binary-only).  All facilities are
* templated on the node type, so mutable and `const` variants share one
* definition.
*
* THE SPEC (Structure, Addressability, Iterability).
*   A container is a FINITE TREE of components rooted at c, and n |> n' ("n'
* is
* a child of n") is its DESCENT relation.  Three of the four orders below are
* LOCAL walks of that relation: each next node is reached by a bounded
* sequence
* of child / sibling / parent steps. A local walk needs NO AUXILIARY STORAGE
* --
* the parent link is the way back up -- and it runs in O(1) space with a
* noexcept step. This is why pre-order, post-order, and leaf order below carry
* nothing but a node, an anchor, and a level.
*
*   LEVEL order is the exception, and the exception is structural, not an
* oversight. It does not walk |>; it enumerates the LEVEL STRATA L_0, L_1, ...
* of the spec, and the frontier of a stratum cannot be recovered from any one
* node by local moves. It must therefore REMEMBER the frontier, which is why
* it
* alone allocates -- O(width), in a deque that is genuinely drained -- and why
* it alone is not bidirectional.
*
*   THE LEVEL of a node (lambda in the spec) is its distance from the anchor,
* which is at level 0.  It is the length of the node's PATH, and hence the
* length of its ADDRESS.  Every iterator here reports it, maintained
* incrementally at no cost.  It is NOT the depth `d` of the spec, which is the
* HEIGHT of a node -- measured downward, to its deepest leaf.  The two are
* complementary halves of the same descent and must not be conflated.
*
*   SEPARATION.  A tree is ADDRESSABLE exactly when the children of every node
* bear DISTINCT labels: that is the multiplicity restriction mu_1 read
* sibling-wise, and without it an address does not determine what it names. An
* LCRS node carries a PAYLOAD, not a label, so a label must be DERIVED from
* the
* value (gamma = lab . val) -- which is precisely the case in which a
* Transformability map that is not injective on labels can COLLIDE two
* siblings
* and cost the tree its addressability.  nary_find_child and
* nary_children_separating below are the descent step and the separation
* predicate at this node form.
*
* TRAVERSAL ORDER TAGS (declared in section 0 below):
*   - pre_order_tag      - root, then children             (bidirectional)
*   - post_order_tag     - children, then root             (bidirectional)
*   - leaf_order_tag     - leaves only, in DFS order       (bidirectional)
*   - level_order_tag    - breadth-first, stratum by stratum   (forward)
*
* ITERATORS:
*   - nary_pre_order_iterator<N>     bidirectional, O(1) space, noexcept
*   - nary_post_order_iterator<N>    bidirectional, O(1) space, noexcept
*   - nary_leaf_iterator<N>          bidirectional, O(1) space, noexcept
*   - nary_level_order_iterator<N>   forward, O(width) space
*
* DISPATCH ALIAS:
*   - nary_tree_iterator<N, OrderTag>  selects one of the four by tag (pre is
*                                      the default)
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
*     - last_child()     (post / leaf / level order, and every operator--)
*     - prev_sibling()   (every operator--, and the cursor)
*   `nary_tree_node<T>` from nary_tree_node.hpp satisfies all of these.  It is
*   the FIVE-link layout that buys the reverse traversals: last_child and
*   prev_sibling are exactly the mirrors of first_child and next_sibling, so
*   each operator-- is its operator++ read in a mirror.  A node form lacking
*   them supports the forward direction only.
*
* END SENTINELS:
*   A default-constructed iterator is an end sentinel, as before, and compares
*   equal to any end of the same order.  An end constructed with
*   nary_iterator_end_tag additionally REMEMBERS THE ANCHOR, which is what
* lets
*   --end() land on the last node of the order.  The two compare equal, so the
*   older contract is unaffected; only the anchored form may be decremented.
*
*
* path:      /inc/djinterp/core/container/tree/nary/nary_tree_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_ITERATOR_HPP
#define DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_ITERATOR_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule): the cursors' D_CONSTEXPR members (go_root and its kin) have
// C++14 bodies. The owner's ruling: compile at every level first; port down
// only where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// std
#include <cstddef>
#include <deque>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../../djinterp.hpp"
#include "../../iterator/tree/tree_iterator.hpp"


NS_DJINTERP

// =========================================================================
// 0.   TRAVERSAL ORDER TAGS
// =========================================================================
//   The four order selectors that pick a traversal.  They are empty tag types
// -- carried only in the type system, to steer nary_iterator_for below -- and
// so are available in every language mode from C++98 onward, needing no
// D_ENV_LANG_IS_CPP* gate.  Three of the four (pre, post, leaf) name the same
// orders as tree_order in tree_iterator.hpp; level order is n-ary specific,
// which is why these are declared here rather than borrowed from the binary
// header.

// pre_order_tag
//   struct: selects pre-order traversal -- a node before its children
// (document order).
struct pre_order_tag
{};

// post_order_tag
//   struct: selects post-order traversal -- a node after its children.
struct post_order_tag
{};

// leaf_order_tag
//   struct: selects leaf-order traversal -- the frontier only, internal nodes
// skipped, in document order.
struct leaf_order_tag
{};

// level_order_tag
//   struct: selects level-order traversal -- breadth-first, stratum by
// stratum.
struct level_order_tag
{};


// =========================================================================
// I.   ITERATOR TYPE TRAITS (internal)
// =========================================================================

NS_INTERNAL

    // nary_iter_traits
    //   trait: derives the standard iterator typedefs from NodeType. Handles
    // the const / non-const split via std::is_const so a single iterator
    // template covers both `nary_tree_node<T>` and `const nary_tree_node<T>`.
    template<typename NodeType>
    struct nary_iter_traits
    {
    private:
        using bare_node_type = typename std::remove_const<
            NodeType>::type;

    public:
        using value_type      = typename bare_node_type::value_type;
        using difference_type = std::ptrdiff_t;

        using reference = typename std::conditional<
            std::is_const<NodeType>::value,
            const value_type&,
            value_type&>::type;

        using pointer = typename std::conditional<
            std::is_const<NodeType>::value,
            const value_type*,
            value_type*>::type;
    };

NS_END  // internal


// nary_iterator_end_tag
//   struct: selects the ANCHORED past-the-end constructor of the iterators
// below. An end that remembers its anchor may be decremented; the
// default-constructed end may not, and the two compare equal.
struct nary_iterator_end_tag
{};


// =========================================================================
// II.  DESCENT PRIMITIVES
// =========================================================================
//   The free operations on the descent relation from which every traversal
// below is built.  Each is O(1) in space and every template argument is
// deducible from the call.

// nary_is_leaf
//   function: true if _node has no children -- a leaf, an element of the
// container in the sense of the spec's Structure.
template<typename NodeType>
D_CONSTEXPR
bool
nary_is_leaf
(
    const NodeType* _node
) noexcept
{
    return ( (_node != nullptr) &&
             (_node->first_child() == nullptr) );
}


// nary_leftmost_leaf
//   function: the leaf reached from _node by taking first children while any
// remain. It is _node itself when _node is a leaf, and it is the FIRST node of
// a post-order or leaf-order walk of the subtree beneath _node.
template<typename NodeType>
D_CONSTEXPR_CPP14
NodeType*
nary_leftmost_leaf
(
    NodeType* _node
) noexcept
{
    NodeType* n;

    if (_node == nullptr)
    {
        return nullptr;
    }

    n = _node;

    // keep taking the first child until there is none
    while (n->first_child() != nullptr)
    {
        n = n->first_child();
    }

    return n;
}


// nary_rightmost_leaf
//   function: the mirror of nary_leftmost_leaf -- the leaf reached by taking
// LAST children while any remain. It is the LAST node of a pre-order or
// leaf-order walk of the subtree beneath _node, and so the node --end() lands
// on for those two orders.
template<typename NodeType>
D_CONSTEXPR_CPP14
NodeType*
nary_rightmost_leaf
(
    NodeType* _node
) noexcept
{
    NodeType* n;

    if (_node == nullptr)
    {
        return nullptr;
    }

    n = _node;

    // keep taking the last child until there is none
    while (n->last_child() != nullptr)
    {
        n = n->last_child();
    }

    return n;
}


// nary_level
//   function: the level (lambda) of _node measured from _anchor, which is at
// level 0 -- the number of descents from the anchor to the node, and hence the
// length of the node's address. O(level).
//
//   This is NOT the depth of the spec, which is a node's HEIGHT, measured
// downward to its deepest leaf. The iterators below maintain this quantity
// incrementally and so never pay for it.
template<typename NodeType>
D_CONSTEXPR_CPP14
std::size_t
nary_level
(
    const NodeType* _node,
    const NodeType* _anchor
) noexcept
{
    std::size_t     level = 0;
    const NodeType* n     = _node;

    // climb to the anchor, counting descents
    while ( (n != nullptr) &&
            (n != _anchor) )
    {
        ++level;

        n = n->parent();
    }

    return level;
}


// nary_identity_label
//   struct: the default labelling -- the value IS its own label. Supplied so
// the descent step below reads the same whether a tree labels its children by
// their whole payload or by a field of it.
struct nary_identity_label
{
    template<typename Type>
    D_CONSTEXPR
    const Type&
    operator()(const Type& _value) const noexcept
    {
        return _value;
    }
};


// nary_find_child
//   function: the child of _parent whose label equals _label, or null if none
// bears it. The label of a child is _labeller(child->data()).
//
//   This is THE DESCENT STEP: one step of resolution, and the whole of what
// the SCANNING strategy of the Addressability axis does. Resolving an address
// is this function folded along the address. The step is O(k) in the branching
// factor; a node holding its children as a map from label to child would make
// it O(1), and one holding them sorted by label O(log k) -- the native and
// ordered strategies. All three compute the same descent; only the cost
// differs.
//
//   PRECONDITION (separation). The result is well defined only where the
// children of _parent bear DISTINCT labels: without that, an address does not
// determine what it names, and this function silently returns the first match.
// See nary_children_separating.
template<typename NodeType,
         typename Label,
         typename Labeller = nary_identity_label>
D_CONSTEXPR_CPP14
NodeType*
nary_find_child
(
    NodeType*   _parent,
    const Label& _label,
    Labeller      _labeller = Labeller()
)
{
    NodeType* child;

    // a null parent has no children to search
    if (_parent == nullptr)
    {
        return nullptr;
    }

    child = _parent->first_child();

    // scan the sibling chain, comparing labels
    while (child != nullptr)
    {
        if (_labeller(child->data()) == _label)
        {
            return child;
        }

        child = child->next_sibling();
    }

    return nullptr;
}


// nary_children_separating
//   function: true if the children of _parent bear pairwise distinct labels --
// the SEPARATION condition of the Addressability axis, imposed at one node.
//
//   Separation is the multiplicity restriction mu_1 read sibling-wise: under
// it a node's children are a MAP from label to child, an address determines
// what it names, and resolve(addr(n)) == n. Without it a label names a SET,
// and no resolution can invert addressing. Because an LCRS node labels by its
// PAYLOAD, separation is a property the tree can LOSE under a map that is not
// injective on labels -- which is exactly the spec's warning that mapping may
// break mu_1. The check is O(k^2) in the branching factor and is meant for a
// debug assertion or a one-off validation, not for the resolution path.
template<typename NodeType,
         typename Labeller = nary_identity_label>
D_CONSTEXPR_CPP14
bool
nary_children_separating
(
    const NodeType* _parent,
    Labeller         _labeller = Labeller()
)
{
    // a null parent trivially separates: it has no children to collide
    if (_parent == nullptr)
    {
        return true;
    }

    const NodeType* outer = _parent->first_child();

    // compare each child against every later sibling
    while (outer != nullptr)
    {
        const NodeType* inner = outer->next_sibling();

        while (inner != nullptr)
        {
            if (_labeller(outer->data()) == _labeller(inner->data()))
            {
                return false;
            }

            inner = inner->next_sibling();
        }

        outer = outer->next_sibling();
    }

    return true;
}


// =========================================================================
// III. PRE-ORDER ITERATOR
// =========================================================================

// nary_pre_order_iterator
//   class: depth-first pre-order BIDIRECTIONAL iterator over an LCRS subtree
// anchored at a given node. Yields a node before descending into its children.
//
//   NO AUXILIARY STORAGE. The parent link is the way back up, so the unwind
// that a stack-based walk would pop is here a climb: from a node with no child
// and no next sibling, ascend until an ancestor -- strictly below the anchor
// -- has one. State is therefore a node, the anchor, and the level, and
// nothing
// else: the iterator is trivially copyable in spirit, its step is noexcept,
// and no traversal ever allocates. (The post-order and leaf iterators below
// have always worked this way; the pre-order one now agrees with them.)
//
//   Subtree-anchored: the iterator never advances past the siblings of the
// anchor, so iterating from any node visits exactly that subtree.
template<typename NodeType>
class nary_pre_order_iterator
{
private:
    using traits_type = internal::nary_iter_traits<NodeType>;

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using node_type         = NodeType;
    using value_type        = typename traits_type::value_type;
    using difference_type   = typename traits_type::difference_type;
    using reference         = typename traits_type::reference;
    using pointer           = typename traits_type::pointer;


    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    // nary_pre_order_iterator (default)
    //   an unanchored end sentinel. It compares equal to any end of this
    // order, but it may not be decremented -- it remembers no anchor.
    D_CONSTEXPR
    nary_pre_order_iterator() noexcept
        : m_current(nullptr),
          m_anchor(nullptr),
          m_level(0)
    {}

    // nary_pre_order_iterator (begin)
    //   an iterator at the first node of the order: the anchor itself.
    D_CONSTEXPR explicit
    nary_pre_order_iterator(
        node_type* _root
    ) noexcept
        : m_current(_root),
          m_anchor(_root),
          m_level(0)
    {}

    // nary_pre_order_iterator (anchored end)
    //   the past-the-end iterator of the traversal anchored at _root, which
    // remembers _root and so may be decremented onto the last node.
    D_CONSTEXPR
    nary_pre_order_iterator(
        node_type* _root,
        nary_iterator_end_tag
    ) noexcept
        : m_current(nullptr),
          m_anchor(_root),
          m_level(0)
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
    //   returns the underlying node pointer. Useful for structural operations
    // that need more than the payload.
    D_CONSTEXPR node_type*
    node() const noexcept
    {
        return m_current;
    }

    // anchor
    //   returns the node the traversal is anchored at, and which bounds it.
    D_CONSTEXPR node_type*
    anchor() const noexcept
    {
        return m_anchor;
    }

    // level
    //   returns the level (lambda) of the current node, measured from the
    // anchor: the number of descents taken to reach it, and hence the length
    // of its address. Maintained incrementally; O(1).
    D_CONSTEXPR std::size_t
    level() const noexcept
    {
        return m_level;
    }


    // -----------------------------------------------------------------
    // traversal
    // -----------------------------------------------------------------

    D_CONSTEXPR_CPP14 nary_pre_order_iterator&
    operator++() noexcept
    {
        advance();

        return *this;
    }

    D_CONSTEXPR_CPP14 nary_pre_order_iterator
    operator++(int) noexcept
    {
        nary_pre_order_iterator tmp(*this);

        advance();

        return tmp;
    }

    D_CONSTEXPR_CPP14 nary_pre_order_iterator&
    operator--() noexcept
    {
        retreat();

        return *this;
    }

    D_CONSTEXPR_CPP14 nary_pre_order_iterator
    operator--(int) noexcept
    {
        nary_pre_order_iterator tmp(*this);

        retreat();

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
    // descend_rightmost
    //   descends from the current node by LAST children while any remain,
    // maintaining the level. Lands on the last node, in pre-order, of the
    // subtree beneath it.
    D_CONSTEXPR_CPP14 void
    descend_rightmost() noexcept
    {
        while (m_current->last_child() != nullptr)
        {
            m_current = m_current->last_child();

            ++m_level;
        }

        return;
    }

    // advance
    //   moves to the next node in pre-order: the first child if there is one;
    // otherwise the next sibling of the nearest ancestor that has one, the
    // climb stopping at the anchor, whose own siblings lie outside the walk.
    // No stack: the parent link IS the unwind.
    D_CONSTEXPR_CPP14 void
    advance() noexcept
    {
        // an iterator already at its end does not advance
        if (m_current == nullptr)
        {
            return;
        }

        // a node is followed by its own subtree
        if (m_current->first_child() != nullptr)
        {
            m_current = m_current->first_child();

            ++m_level;

            return;
        }

        node_type*  node  = m_current;
        std::size_t level = m_level;

        // climb until a next sibling appears, or the anchor is reached
        while (node != m_anchor)
        {
            if (node->next_sibling() != nullptr)
            {
                m_current = node->next_sibling();
                m_level   = level;

                return;
            }

            node = node->parent();

            // a parent chain that misses the anchor is malformed; end the walk
            if (node == nullptr)
            {
                break;
            }

            --level;
        }

        m_current = nullptr;
        m_level   = 0;

        return;
    }

    // retreat
    //   moves to the previous node in pre-order, the exact mirror of advance:
    // from the end, the last node of the subtree (its rightmost-deepest); from
    // a node with a previous sibling, that sibling's rightmost-deepest
    // descendant; otherwise the parent. Decrementing the anchor -- the first
    // node of the order -- is a no-op, as decrementing begin() always is.
    D_CONSTEXPR_CPP14 void
    retreat() noexcept
    {
        // from the end: the last node of the order is the anchor's
        // rightmost-deepest descendant
        if (m_current == nullptr)
        {
            if (m_anchor == nullptr)
            {
                return;
            }

            m_current = m_anchor;
            m_level   = 0;

            descend_rightmost();

            return;
        }

        // the anchor is the first node of the order; there is nothing before it
        if (m_current == m_anchor)
        {
            return;
        }

        // a previous sibling is preceded by the whole of its own subtree
        if (m_current->prev_sibling() != nullptr)
        {
            m_current = m_current->prev_sibling();

            descend_rightmost();

            return;
        }

        // otherwise the parent immediately precedes its first child
        m_current = m_current->parent();

        --m_level;

        return;
    }

    node_type*  m_current;
    node_type*  m_anchor;
    std::size_t m_level;
};


// =========================================================================
// IV.  POST-ORDER ITERATOR
// =========================================================================

// nary_post_order_iterator
//   class: depth-first post-order BIDIRECTIONAL iterator. Yields a node only
// after its entire subtree has been visited, so the anchor is LAST. Uses
// parent links to climb back out; state is a node, the anchor, and the level.
//
//   This is the order a FOLD over the tree bottoms out in: the children of a
// node are complete before the node is reached.
template<typename NodeType>
class nary_post_order_iterator
{
private:
    using traits_type = internal::nary_iter_traits<NodeType>;

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using node_type         = NodeType;
    using value_type        = typename traits_type::value_type;
    using difference_type   = typename traits_type::difference_type;
    using reference         = typename traits_type::reference;
    using pointer           = typename traits_type::pointer;


    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    // nary_post_order_iterator (default)
    //   an unanchored end sentinel; it may not be decremented.
    D_CONSTEXPR
    nary_post_order_iterator() noexcept
        : m_current(nullptr),
          m_anchor(nullptr),
          m_level(0)
    {}

    // nary_post_order_iterator (begin)
    //   an iterator at the first node of the order: the anchor's
    // leftmost-deepest descendant.
    D_CONSTEXPR_CPP14 explicit
    nary_post_order_iterator(
        node_type* _root
    ) noexcept
        : m_current(_root),
          m_anchor(_root),
          m_level(0)
    {
        // post-order opens at the deepest-first node of the subtree
        if (m_current != nullptr)
        {
            descend_leftmost();
        }
    }

    // nary_post_order_iterator (anchored end)
    //   the past-the-end iterator, which remembers _root and so may be
    // decremented onto the last node of the order -- the anchor itself.
    D_CONSTEXPR
    nary_post_order_iterator(
        node_type* _root,
        nary_iterator_end_tag
    ) noexcept
        : m_current(nullptr),
          m_anchor(_root),
          m_level(0)
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

    D_CONSTEXPR node_type*
    anchor() const noexcept
    {
        return m_anchor;
    }

    // level
    //   the level (lambda) of the current node, measured from the anchor.
    D_CONSTEXPR std::size_t
    level() const noexcept
    {
        return m_level;
    }


    // -----------------------------------------------------------------
    // traversal
    // -----------------------------------------------------------------

    D_CONSTEXPR_CPP14 nary_post_order_iterator&
    operator++() noexcept
    {
        advance();

        return *this;
    }

    D_CONSTEXPR_CPP14 nary_post_order_iterator
    operator++(int) noexcept
    {
        nary_post_order_iterator tmp(*this);

        advance();

        return tmp;
    }

    D_CONSTEXPR_CPP14 nary_post_order_iterator&
    operator--() noexcept
    {
        retreat();

        return *this;
    }

    D_CONSTEXPR_CPP14 nary_post_order_iterator
    operator--(int) noexcept
    {
        nary_post_order_iterator tmp(*this);

        retreat();

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
    // descend_leftmost
    //   descends by first children while any remain, maintaining the level.
    D_CONSTEXPR_CPP14 void
    descend_leftmost() noexcept
    {
        while (m_current->first_child() != nullptr)
        {
            m_current = m_current->first_child();

            ++m_level;
        }

        return;
    }

    // advance
    //   moves to the next node in post-order: the anchor is last; otherwise
    // the leftmost-deepest descendant of the next sibling, or -- there being
    // no next sibling -- the parent, whose children are now complete.
    D_CONSTEXPR_CPP14 void
    advance() noexcept
    {
        // an iterator already at its end does not advance
        if (m_current == nullptr)
        {
            return;
        }

        // the anchor is the last node a post-order walk visits
        if (m_current == m_anchor)
        {
            m_current = nullptr;
            m_level   = 0;

            return;
        }

        // no next sibling: this node completed its parent
        if (m_current->next_sibling() == nullptr)
        {
            m_current = m_current->parent();

            // a parent chain that misses the anchor is malformed
            if (m_current == nullptr)
            {
                m_level = 0;

                return;
            }

            --m_level;

            return;
        }

        // otherwise the next subtree opens at its own leftmost leaf
        m_current = m_current->next_sibling();

        descend_leftmost();

        return;
    }

    // retreat
    //   moves to the previous node in post-order, the mirror of advance: from
    // the end, the anchor (which post-order yields last); from a node with
    // children, its LAST child; otherwise the previous sibling of the nearest
    // ancestor that has one.
    D_CONSTEXPR_CPP14 void
    retreat() noexcept
    {
        // from the end: post-order yields the anchor last
        if (m_current == nullptr)
        {
            if (m_anchor == nullptr)
            {
                return;
            }

            m_current = m_anchor;
            m_level   = 0;

            return;
        }

        // a node is preceded by the last of its own children
        if (m_current->last_child() != nullptr)
        {
            m_current = m_current->last_child();

            ++m_level;

            return;
        }

        node_type*  node  = m_current;
        std::size_t level = m_level;

        // climb until a previous sibling appears, or the anchor is reached
        while (node != m_anchor)
        {
            if (node->prev_sibling() != nullptr)
            {
                m_current = node->prev_sibling();
                m_level   = level;

                return;
            }

            node = node->parent();

            if (node == nullptr)
            {
                break;
            }

            --level;
        }

        return;
    }

    node_type*  m_current;
    node_type*  m_anchor;
    std::size_t m_level;
};


// =========================================================================
// V.   LEAF-ONLY ITERATOR
// =========================================================================

// nary_leaf_iterator
//   class: BIDIRECTIONAL iterator yielding only leaves, in DFS discovery
// order. This is the FRONTIER of the container -- its elements, in the sense
// of the spec, the node summand skipped. Implemented with parent links, so no
// auxiliary stack is required in either direction.
template<typename NodeType>
class nary_leaf_iterator
{
private:
    using traits_type = internal::nary_iter_traits<NodeType>;

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using node_type         = NodeType;
    using value_type        = typename traits_type::value_type;
    using difference_type   = typename traits_type::difference_type;
    using reference         = typename traits_type::reference;
    using pointer           = typename traits_type::pointer;


    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    // nary_leaf_iterator (default)
    //   an unanchored end sentinel; it may not be decremented.
    D_CONSTEXPR
    nary_leaf_iterator() noexcept
        : m_current(nullptr),
          m_anchor(nullptr),
          m_level(0)
    {}

    // nary_leaf_iterator (begin)
    //   an iterator at the first leaf of the subtree: its leftmost-deepest
    // descendant.
    D_CONSTEXPR_CPP14 explicit
    nary_leaf_iterator(
        node_type* _root
    ) noexcept
        : m_current(_root),
          m_anchor(_root),
          m_level(0)
    {
        // the frontier opens at the deepest-first node of the subtree
        if (m_current != nullptr)
        {
            descend_leftmost();
        }
    }

    // nary_leaf_iterator (anchored end)
    //   the past-the-end iterator, decrementable onto the last leaf.
    D_CONSTEXPR
    nary_leaf_iterator(
        node_type* _root,
        nary_iterator_end_tag
    ) noexcept
        : m_current(nullptr),
          m_anchor(_root),
          m_level(0)
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

    D_CONSTEXPR node_type*
    anchor() const noexcept
    {
        return m_anchor;
    }

    // level
    //   the level (lambda) of the current leaf, measured from the anchor.
    D_CONSTEXPR std::size_t
    level() const noexcept
    {
        return m_level;
    }


    // -----------------------------------------------------------------
    // traversal
    // -----------------------------------------------------------------

    D_CONSTEXPR_CPP14 nary_leaf_iterator&
    operator++() noexcept
    {
        advance();

        return *this;
    }

    D_CONSTEXPR_CPP14 nary_leaf_iterator
    operator++(int) noexcept
    {
        nary_leaf_iterator tmp(*this);

        advance();

        return tmp;
    }

    D_CONSTEXPR_CPP14 nary_leaf_iterator&
    operator--() noexcept
    {
        retreat();

        return *this;
    }

    D_CONSTEXPR_CPP14 nary_leaf_iterator
    operator--(int) noexcept
    {
        nary_leaf_iterator tmp(*this);

        retreat();

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
    // descend_leftmost / descend_rightmost
    //   descend by first / last children while any remain, maintaining the
    // level. A leaf walk opens on the one and closes on the other.
    D_CONSTEXPR_CPP14 void
    descend_leftmost() noexcept
    {
        while (m_current->first_child() != nullptr)
        {
            m_current = m_current->first_child();

            ++m_level;
        }

        return;
    }

    D_CONSTEXPR_CPP14 void
    descend_rightmost() noexcept
    {
        while (m_current->last_child() != nullptr)
        {
            m_current = m_current->last_child();

            ++m_level;
        }

        return;
    }

    // advance
    //   moves to the next leaf: climb rootward until an ancestor -- strictly
    // below the anchor -- has a next sibling, then descend to that sibling's
    // leftmost leaf. A pre-order step from a leaf lands on the next subtree in
    // document order, and its leftmost leaf is the next leaf.
    D_CONSTEXPR_CPP14 void
    advance() noexcept
    {
        // an iterator already at its end does not advance
        if (m_current == nullptr)
        {
            return;
        }

        node_type*  node  = m_current;
        std::size_t level = m_level;

        while (node != m_anchor)
        {
            if (node->next_sibling() != nullptr)
            {
                m_current = node->next_sibling();
                m_level   = level;

                descend_leftmost();

                return;
            }

            node = node->parent();

            if (node == nullptr)
            {
                break;
            }

            --level;
        }

        m_current = nullptr;
        m_level   = 0;

        return;
    }

    // retreat
    //   moves to the previous leaf, the mirror of advance: from the end, the
    // anchor's rightmost leaf; otherwise climb until an ancestor has a
    // PREVIOUS sibling, then descend to that sibling's RIGHTMOST leaf.
    D_CONSTEXPR_CPP14 void
    retreat() noexcept
    {
        // from the end: the last leaf is the anchor's rightmost-deepest
        if (m_current == nullptr)
        {
            if (m_anchor == nullptr)
            {
                return;
            }

            m_current = m_anchor;
            m_level   = 0;

            descend_rightmost();

            return;
        }

        node_type*  node  = m_current;
        std::size_t level = m_level;

        while (node != m_anchor)
        {
            if (node->prev_sibling() != nullptr)
            {
                m_current = node->prev_sibling();
                m_level   = level;

                descend_rightmost();

                return;
            }

            node = node->parent();

            if (node == nullptr)
            {
                break;
            }

            --level;
        }

        return;
    }

    node_type*  m_current;
    node_type*  m_anchor;
    std::size_t m_level;
};


// =========================================================================
// VI.  LEVEL-ORDER ITERATOR
// =========================================================================

// nary_level_order_iterator
//   class: breadth-first FORWARD iterator -- the enumeration of the level
// strata L_0, L_1, ... of the spec, in order.
//
//   THIS IS THE ONE ORDER THAT IS NOT A LOCAL WALK of the descent relation.
// The next node in pre-, post-, or leaf order is reachable from the current
// one by a bounded sequence of child / sibling / parent steps, so those need
// no memory and run backwards as readily as forwards. The next node of a
// STRATUM is not: it may lie in an arbitrarily distant subtree, and only the
// frontier already discovered knows where. Level order must therefore REMEMBER
// that frontier, which is why it alone allocates, and why it alone is forward.
//
//   The frontier is held in a deque and is genuinely DRAINED as it is
// consumed, so the storage is O(width) -- the widest stratum -- and not O(n).
// Each entry carries its level with it, so level() is exact and free.
template<typename NodeType>
class nary_level_order_iterator
{
private:
    using traits_type = internal::nary_iter_traits<NodeType>;
    using entry_type  = std::pair<NodeType*, std::size_t>;

public:
    using iterator_category = std::forward_iterator_tag;
    using node_type         = NodeType;
    using value_type        = typename traits_type::value_type;
    using difference_type   = typename traits_type::difference_type;
    using reference         = typename traits_type::reference;
    using pointer           = typename traits_type::pointer;


    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    // nary_level_order_iterator (default)
    //   an end sentinel.
    nary_level_order_iterator()
        : m_current(nullptr),
          m_level(0),
          m_frontier()
    {}

    // nary_level_order_iterator (begin)
    //   an iterator at the anchor, which is the whole of stratum L_0.
    explicit
    nary_level_order_iterator(
        node_type* _root
    )
        : m_current(nullptr),
          m_level(0),
          m_frontier()
    {
        // seed the frontier with the anchor, then take it
        if (_root != nullptr)
        {
            m_frontier.push_back(entry_type(_root, 0));

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

    // level
    //   the level (lambda) of the current node -- here the index k of the
    // stratum L_k being enumerated, which is what a breadth-first walk is FOR.
    // Carried alongside each frontier entry, so it is exact and free.
    D_CONSTEXPR std::size_t
    level() const noexcept
    {
        return m_level;
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
        nary_level_order_iterator tmp(*this);

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
    //   takes the next node from the frontier and pushes its children onto the
    // back, each one stratum deeper. Popping the front is what keeps the
    // frontier O(width): a node discovered is a node that will be consumed.
    void
    advance()
    {
        node_type* child;

        // an exhausted frontier is the end of the traversal
        if (m_frontier.empty())
        {
            m_current = nullptr;
            m_level   = 0;

            return;
        }

        m_current = m_frontier.front().first;
        m_level   = m_frontier.front().second;

        m_frontier.pop_front();

        child = m_current->first_child();

        // the children of this node belong to the next stratum
        while (child != nullptr)
        {
            m_frontier.push_back(entry_type(child, m_level + 1));

            child = child->next_sibling();
        }

        return;
    }

    node_type*             m_current;
    std::size_t            m_level;
    std::deque<entry_type> m_frontier;
};


// =========================================================================
// VII. DISPATCH ALIAS
// =========================================================================

NS_INTERNAL

    // nary_iterator_for
    //   trait: maps an order tag to the corresponding iterator class. The
    // primary template is intentionally undefined so unsupported tags surface
    // as a compile error rather than a silent fallback.
    template<typename NodeType,
             typename OrderTag>
    struct nary_iterator_for;

    // nary_iterator_for<NodeType, pre_order_tag>
    //   trait: the `pre_order_tag` case; it maps to
    // `nary_pre_order_iterator<NodeType>`.
    template<typename NodeType>
    struct nary_iterator_for<NodeType, pre_order_tag>
    {
        using type = nary_pre_order_iterator<NodeType>;
    };

    // nary_iterator_for<NodeType, post_order_tag>
    //   trait: the `post_order_tag` case; it maps to
    // `nary_post_order_iterator<NodeType>`.
    template<typename NodeType>
    struct nary_iterator_for<NodeType, post_order_tag>
    {
        using type = nary_post_order_iterator<NodeType>;
    };

    // nary_iterator_for<NodeType, level_order_tag>
    //   trait: the `level_order_tag` case; it maps to
    // `nary_level_order_iterator<NodeType>`.
    template<typename NodeType>
    struct nary_iterator_for<NodeType, level_order_tag>
    {
        using type = nary_level_order_iterator<NodeType>;
    };

    // nary_iterator_for<NodeType, leaf_order_tag>
    //   trait: the `leaf_order_tag` case; it maps to
    // `nary_leaf_iterator<NodeType>`.
    template<typename NodeType>
    struct nary_iterator_for<NodeType, leaf_order_tag>
    {
        using type = nary_leaf_iterator<NodeType>;
    };

NS_END  // internal

// nary_tree_iterator
//   alias: dispatches to one of the four order-specific iterators based on
// OrderTag. Pre-order is the default, matching nary_tree::iterator.
template<typename NodeType,
         typename OrderTag = pre_order_tag>
using nary_tree_iterator =
    typename internal::nary_iterator_for<
        NodeType, OrderTag>::type;


// =========================================================================
// VIII. CURSOR
// =========================================================================

// nary_tree_cursor
//   class: imperative LCRS navigator analogous to tree_cursor<Node> from
// tree_iterator.hpp (which targets binary trees). Every navigation primitive
// runs in O(1)
// when the underlying node has the corresponding link, which for
// nary_tree_node<T> means parent / first_child / last_child / next_sibling /
// prev_sibling are all live.
//
//   The cursor is anchored to a root at construction time and tracks the
// current node. go_*() methods return true
// on successful navigation and false when the requested direction has no link
// (or would leave the subtree, in the case of go_root). set() bypasses
// navigation by jumping the cursor directly to a known node.
template<typename NodeType>
class nary_tree_cursor
{
private:
    using traits_type = internal::nary_iter_traits<NodeType>;

public:
    using node_type       = NodeType;
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

    // level
    //   returns the LEVEL (lambda) of the current node, measured from the
    // cursor's root anchor, which is at level 0: the
    // number of descents from the anchor to the node, and hence the length of
    // its address. O(level).
    //
    //   This quantity is NOT the depth of the spec, which is a node's HEIGHT
    // -- measured downward, to its deepest leaf.
    // Level counts upward, height counts downward, and they agree only at the
    // anchor. The iterators of this header maintain the level incrementally
    // and so never pay this walk.
    size_type
    level() const noexcept
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

    // depth
    //   the former spelling of level(), retained so existing callers continue
    // to compile. It measures the distance from the anchor, which the spec
    // calls the LEVEL; prefer level().
    size_type
    depth() const noexcept
    {
        return level();
    }

    // child_count
    //   returns the number of direct children of the current node.
    // O(child_count).
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
    //   resets the cursor to the anchored root. Returns true if the root is
    // non-null.
    D_CONSTEXPR bool
    go_root() noexcept
    {
        m_current = m_root;

        return (m_current != nullptr);
    }

    // go_parent
    //   moves to the current node's parent unless the cursor is already at the
    // root. Returns true on success.
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
    //   walks the sibling chain of first_child to land on the _index'th child
    // (0-based). Returns true on success. O(_index).
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
    //   jumps the cursor directly to _node. The caller is
    // responsible for ensuring _node belongs to the same subtree as the
    // cursor's anchored root; the cursor does not validate this.
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

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_ITERATOR_HPP
