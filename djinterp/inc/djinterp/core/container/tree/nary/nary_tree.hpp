/*******************************************************************************
* djinterp [core]                                                  nary_tree.hpp
*
* N-ary tree container:
*   A heap-allocated, owning n-ary tree using the LCRS (left-child /
* right-sibling) topology with auxiliary parent, last_child, and prev_sibling
* links.  The five-pointer node layout enables O(1) append, prepend, detach,
* insert_after, and insert_before - the full set of operations advertised by
* the
* framework's `o1_*_nary_tree` concepts.
*
*   Inherits `tree_container` for entry-point storage, size tracking,
* allocator
* management, and Rule-of-Five behavior, and `options_container_base` for the
* framework's per-axis options surface.  This file adds the n-ary topology,
* hierarchical access, ADDRESSING, filtering, transformation, and the standard
* subtree mutators.
*
* ============================================================================
* WHAT CHANGED, AND WHY
* ============================================================================
*   This file predates most of the container subframework it belongs to.  Four
* things were wrong with it, and each is a real defect rather than a matter of
* taste:
*
*   1. IT CARRIED ITS OWN nary_tree_node. nary_tree_node.hpp defines that
* class
*      too, on top of linked_node<T, 5>. Two definitions of one class template
*      is an ODR violation the moment both headers reach one translation unit.
*      GONE: the node now comes from nary_tree_node.hpp, which owns it.
*
*   2. IT CARRIED ITS OWN PRE-ORDER ITERATOR, a byte-for-byte copy of the one
* in
*      nary_tree_iterator.hpp -- and `iterator` aliased the COPY, so the
*    header
*      that exists to supply this container's iterators supplied it none. Both
*      copies dragged a std::vector STACK along, which an LCRS node needs not
*    at
*      all: the parent link IS the unwind.  GONE: the iterators come from
*      nary_tree_iterator.hpp, which owns them, and are now O(1)-space,
*      noexcept, allocation-free, and BIDIRECTIONAL -- so this container gains
*      rbegin()/rend(), post-order, leaf order, and level order for nothing.
*
*   3. depth(n) COUNTED PARENT LINKS, which is the LEVEL (lambda), not the
* DEPTH. The spec reserves depth for a node's HEIGHT -- measured downward, to
* its deepest leaf.  They are different numbers on the same node and the tree
* had no way to ask for the second.  level() and height() now both exist;
* depth() is retained as a spelling of level().
*
*   4. IT HAD NO OPTIONS SURFACE, though container_options.hpp calls
*      options_container_base "the canonical base mixin every container in the
*      framework inherits". It does now, and it declares its position on every
*      configurable axis.
*
* ============================================================================
* THE SPEC: WHAT A TREE'S FILTER ACTUALLY MEANS
* ============================================================================
*   A hierarchical container is NOT a sequence, and the single most dangerous
* thing one can do to it is filter it as though it were.
*
*   ADDRESSING. Every component of a tree has a unique PATH from the root, and
* the word of LABELS along that path is its ADDRESS.  A tree is ADDRESSABLE
* exactly when its labelling SEPARATES -- the children of every node bear
* distinct labels -- which is the multiplicity restriction mu_1 read
* sibling-wise.  An LCRS node carries a PAYLOAD, not a label, so a label is
* DERIVED from the value (gamma = lab . val); see is_addressable().
*
*   FILTERING.  Removing a LEAF disturbs nothing.  Removing an INTERIOR node
* severs every descent through it, and the addresses of its whole subtree
* cease
* to resolve. So a selection preserves the survivors' addresses IF AND ONLY IF
* the retained set is PREFIX-CLOSED (ancestor-closed) -- and a predicate on
* VALUES is not in general ancestor-closed. "Keep exactly the nodes that
* match"
* is therefore NOT A TREE OPERATION AT ALL: it orphans whatever hung beneath a
* node it dropped.  This container will not do it, and offers instead the two
* readings that ARE prefix-closed, plus the flat read that asks no structural
* question:
*
*     filter(pred) CONST. Returns a NEW TREE holding the ANCESTOR-CLOSURE of
*                     the matching set: a node is kept if it, OR ANY
*                   DESCENDANT,
*                     matches.  Prefix-closed by construction.  `grep -r`
*                     semantics.  This is the shape both has_native_filter
*                     (container_filter_traits) and has_filter_method
*                     (filter.hpp) probe for -- a CONST member yielding a
*                   RESULT
*                     -- so it is what makes this container's filter strategy
*                     NATIVE rather than external.
*     prune(pred)     the same closure, applied IN PLACE.
*     erase_if(pred)  in place, the DUAL: a matching node is removed TOGETHER
*                     WITH ITS SUBTREE, since its children cannot outlive it.
*                     Also prefix-closed -- the dual of prune, not its
*                     complement.
*     select(pred)    a flat gather of the matching nodes.  Reads only; builds
*                     no tree; raises no structural question.
*
*   TRANSFORMING. A map keeps every position and so keeps every ADDRESS -- but
* only while the labels hold still. Where the label is derived from the value,
* a map that is not injective ON LABELS can COLLIDE two siblings onto one
* label
* and BREAK SEPARATION, costing the tree its addressability. That is not a new
* hazard: it is the loss of mu_1 the Transformability axis already records,
* seen
* in its structural consequence.  transform_preserves_separation() decides it
* BEFORE the map is applied.  transform() is likewise CONST and
* result-returning -- the shape has_native_transform detects -- and
* transform_in_place() is its destructive twin.
*
*   SEQUENCE BRIDGE.  filter.hpp is a SEQUENCE library: its operations are
* std::function<vector<size_t>(const vector<T>&)>. A tree is not a sequence
* and
* does not satisfy is_filterable (it has no push_back and no insert, and
* should
* have neither -- a tree has no "back"). values() / leaves() / nodes() flatten
* it so filter.hpp's chains may run over its ELEMENTS; what comes back is a
* SEQUENCE, and the tree's shape is not recoverable from it.  That is not a
* limitation of the bridge.  It is what the spec says filtering a hierarchy
* costs.
*
* CLASSIFICATION (the configurable axes of container_options.hpp):
*   lifetime      mutable_storage    (a const& gives the immutable view free)
*   ordering      ordered            (children keep insertion order)
*   bounds        unbounded          (max_size sourced from the allocator)
*   multiplicity  multi              (duplicate payloads allowed)
*   structure     hierarchical       (the headline axis; also the opt-in tag)
*   storage_kind  dynamic_storage    (heap-allocated nodes)
*   thread_safety none               (unless LockPolicy says otherwise)
*   backing       fundamental        (not an overlay on another container)
*   iterability iterable (four orders, three of them bidirectional)
*
* TEMPLATE PARAMETERS:
*   ValueType         - user-facing element type
*   Allocator         - node allocator (default std::allocator<node_type>)
*   LockPolicy        - threading policy (default void = no locking)
*   OwnershipPolicy   - entry point ownership (default unique_owning_policy)
*   Options...       - the framework's per-axis options
* (container_options.hpp). A trailing pack, so every existing instantiation
* keeps compiling untouched; the three policy parameters stay positional
* because
* that is where node_container puts them.
*
*
* path:      /inc/djinterp/core/container/tree/nary/nary_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.05.22
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Sibling Range View          (the descent relation, one level)
      -------------------------------------------------------------

II.   N-ary Tree
      ----------
      1.    axes and options surface
      2.    iterators (four orders; three bidirectional)
      3.    topology  (root / parent / children)
      4.    measure   (level / height / depth)
      5.    ADDRESSABILITY  (path / address / resolve / lca / separation)
      6.    iteration + capacity
      7.    mutation
      8.    FILTERABILITY   (filter / prune / erase_if / select)
      9.    TRANSFORMABILITY(transform / transform_in_place / ..._separation)
      10.   sequence bridge (values / leaves / nodes)

III.  Convenience Aliases
      -------------------
*/

#ifndef DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_HPP
#define DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule): node_container.hpp, whose unique_owning_policy holds the nodes,
// is empty below C++17. The owner's ruling: compile at every level first;
// port down only where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"
#include "../../../meta/hierarchical.hpp" // the structure_category opt-in tag
#include "../../../functional/functional_common.hpp"  // is_predicate
#include "../../container_options.hpp"      // options_container_base, the axes
#include "../tree_container.hpp"
#include "./nary_tree_node.hpp"             // THE node.  No local copy.
#include "./nary_tree_iterator.hpp"         // THE iterators.  No local copy.


NS_DJINTERP

// =========================================================================
// I.   SIBLING RANGE VIEW
// =========================================================================
//   children(node) returns one of these - a forward range over the LCRS sibling
// chain starting at node->first_child.  Holding the view by value is fine; it
// stores a single pointer.
//
//   THE SPEC.  This is the DESCENT RELATION |> itself, one level: the children
// of a node, in sibling order.  It is also the sibling enumeration a SCANNING
// resolution consumes -- find_child() below is a scan of exactly this range.

NS_INTERNAL

    // sibling_iterator
    //   class: forward iterator over the next_sibling chain beginning at a
    // given node. The end sentinel is the default-constructed iterator
    // (m_current == nullptr).
    template<typename NodeType>
    class sibling_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = typename std::remove_const<
            NodeType>::type::value_type;
        using difference_type   = std::ptrdiff_t;
        using reference         = typename std::conditional<
            std::is_const<NodeType>::value,
            const value_type&,
            value_type&>::type;
        using pointer           = typename std::conditional<
            std::is_const<NodeType>::value,
            const value_type*,
            value_type*>::type;

        D_CONSTEXPR
        sibling_iterator() noexcept
            : m_current(nullptr)
        {}

        D_CONSTEXPR explicit
        sibling_iterator(
            NodeType* _start
        ) noexcept
            : m_current(_start)
        {}

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
        //   returns the underlying node pointer, useful for
        // structural operations on the visited child
        D_CONSTEXPR NodeType*
        node() const noexcept
        {
            return m_current;
        }

        D_CONSTEXPR sibling_iterator&
        operator++() noexcept
        {
            m_current = m_current->next_sibling();

            return *this;
        }

        D_CONSTEXPR sibling_iterator
        operator++(int) noexcept
        {
            sibling_iterator tmp;

            tmp       = *this;
            m_current = m_current->next_sibling();

            return tmp;
        }

        D_CONSTEXPR friend bool
        operator==(
            const sibling_iterator& _a,
            const sibling_iterator& _b
        ) noexcept
        {
            return (_a.m_current == _b.m_current);
        }

        D_CONSTEXPR friend bool
        operator!=(
            const sibling_iterator& _a,
            const sibling_iterator& _b
        ) noexcept
        {
            return !(_a == _b);
        }

    private:
        NodeType* m_current;
    };

NS_END  // internal


// sibling_range
//   class: lightweight forward range over an LCRS sibling chain. Holds a
// single node pointer and exposes begin/end/empty. Returned by
// nary_tree::children(node).
template<typename NodeType>
class sibling_range
{
public:
    using node_type     = NodeType;
    using value_type    = typename std::remove_const<
        NodeType>::type::value_type;
    using iterator      = internal::sibling_iterator<NodeType>;
    using const_iterator =
        internal::sibling_iterator<const NodeType>;

    D_CONSTEXPR
    sibling_range() noexcept
        : m_first(nullptr)
    {}

    D_CONSTEXPR explicit
    sibling_range(
        NodeType* _first
    ) noexcept
        : m_first(_first)
    {}

    D_CONSTEXPR iterator
    begin() const noexcept
    {
        return iterator(m_first);
    }

    D_CONSTEXPR iterator
    end() const noexcept
    {
        return iterator();
    }

    D_CONSTEXPR const_iterator
    cbegin() const noexcept
    {
        return const_iterator(m_first);
    }

    D_CONSTEXPR const_iterator
    cend() const noexcept
    {
        return const_iterator();
    }

    D_CONSTEXPR bool
    empty() const noexcept
    {
        return (m_first == nullptr);
    }

private:
    NodeType* m_first;
};


// =========================================================================
// II.  N-ARY TREE
// =========================================================================

// nary_tree
//   class: heap-allocated, owning n-ary tree using the LCRS topology with
// auxiliary parent / last_child / prev_sibling links. Inherits tree_container
// for entry-point storage, size tracking, allocator management, and
// Rule-of-Five behavior, and options_container_base for the per-axis options
// surface; layers hierarchical access, addressing, iteration in four orders,
// O(1) subtree mutation, filtering, and transformation on top.
template<typename ValueType,
         typename Allocator        = std::allocator<nary_tree_node<ValueType>>,
         typename LockPolicy       = void,
         typename OwnershipPolicy = unique_owning_policy,
         typename... Options>
class nary_tree
    : public tree_container<ValueType,
                            nary_tree_node<ValueType>,
                            Allocator,
                            LockPolicy,
                            OwnershipPolicy>,
      public options_container_base<Options...>
{
private:
    using base = tree_container<ValueType,
                                nary_tree_node<ValueType>,
                                Allocator,
                                LockPolicy,
                                OwnershipPolicy>;

    using option_base = options_container_base<Options...>;

    using alloc_traits = std::allocator_traits<Allocator>;

public:
    // -----------------------------------------------------------------
    // re-exported aliases
    // -----------------------------------------------------------------

    using typename base::value_type;
    using typename base::node_type;
    using typename base::allocator_type;
    using typename base::lock_policy;
    using typename base::ownership_policy;
    using typename base::size_type;
    using typename base::difference_type;
    using typename base::depth_type;
    using typename base::reference;
    using typename base::const_reference;
    using typename base::pointer;
    using typename base::const_pointer;

    // level_type
    //   the LEVEL (lambda) of a node: its distance from the root, which is the
    // length of its address. Named apart from depth_type because the spec's
    // DEPTH is a node's HEIGHT, and the two are different numbers. Both are
    // std::size_t; the distinction is in the name and in what it counts.
    using level_type = std::size_t;

    // -----------------------------------------------------------------
    // 1.  the STRUCTURE axis: the opt-in tag
    // -----------------------------------------------------------------

    // structure_category
    //   the OPT-IN structure tag (structure/hierarchical.hpp) that
    // hierarchical_container_traits consults alongside its structural probes.
    // The node_type member is already the strong tell; this states the same
    // thing outright, so no heuristic has to infer it.
    using structure_category = hierarchical;

    // -----------------------------------------------------------------
    // 1b. the options surface (C++17 and later)
    // -----------------------------------------------------------------
    //   Each axis reads its configured position out of the option pack, falling
    // back to the tree's own natural value.  A user who passes an option this
    // container has no hook for is silently ignored, per container_options.hpp.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    using typename option_base::options_type;

    static constexpr container_structure axis_structure =
        container_structure::hierarchical;   // not configurable: a tree nests

    static constexpr container_lifetime axis_lifetime =
        container_axis_value_v<options_type,
                               container_axis::lifetime,
                               container_lifetime::mutable_storage>;

    static constexpr container_ordering axis_ordering =
        container_axis_value_v<options_type,
                               container_axis::ordering,
                               container_ordering::ordered>;

    static constexpr container_bounds axis_bounds =
        container_axis_value_v<options_type,
                               container_axis::bounds,
                               container_bounds::unbounded>;

    static constexpr container_multiplicity axis_multiplicity =
        container_axis_value_v<options_type,
                               container_axis::multiplicity,
                               container_multiplicity::multi>;

    static constexpr container_storage_kind axis_storage_kind =
        container_axis_value_v<options_type,
                               container_axis::storage_kind,
                               container_storage_kind::dynamic_storage>;

    static constexpr container_thread_safety axis_thread_safety =
        container_axis_value_v<options_type,
                               container_axis::thread_safety,
                               container_thread_safety::none>;

    static constexpr container_backing axis_backing =
        container_axis_value_v<options_type,
                               container_axis::backing,
                               container_backing::fundamental>;

    static constexpr container_iterability axis_iterability =
        container_axis_value_v<options_type,
                               container_axis::iterability,
                               container_iterability::iterable>;

    // a tree cannot be configured flat. The structure axis is the one axis
    // this container does not merely default -- it FIXES.
    static_assert(
        container_axis_value_v<options_type,
                               container_axis::structure,
                               container_structure::hierarchical>
            == container_structure::hierarchical,
        "nary_tree is hierarchical by construction: "
        "container_opt_structure<flat> cannot be honoured.");

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


    // -----------------------------------------------------------------
    // 2.  iterators
    // -----------------------------------------------------------------
    //   All four orders, from nary_tree_iterator.hpp.  Pre-, post-, and leaf
    // order are BIDIRECTIONAL (the five-link node affords it), carry no
    // auxiliary storage, and step in noexcept.  Level order enumerates the
    // strata and is forward-only, because it is the one order that is not a
    // local walk of the descent relation and so must remember its frontier.

    using iterator       = nary_tree_iterator<node_type, pre_order_tag>;
    using const_iterator = nary_tree_iterator<const node_type, pre_order_tag>;

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using postorder_iterator =
        nary_tree_iterator<node_type, post_order_tag>;
    using const_postorder_iterator =
        nary_tree_iterator<const node_type, post_order_tag>;

    using leaf_iterator =
        nary_tree_iterator<node_type, leaf_order_tag>;
    using const_leaf_iterator =
        nary_tree_iterator<const node_type, leaf_order_tag>;

    using level_iterator =
        nary_tree_iterator<node_type, level_order_tag>;
    using const_level_iterator =
        nary_tree_iterator<const node_type, level_order_tag>;

    using cursor       = nary_tree_cursor<node_type>;
    using const_cursor = nary_tree_cursor<const node_type>;

    using sibling_range_type       = sibling_range<node_type>;
    using const_sibling_range_type = sibling_range<const node_type>;


    // -----------------------------------------------------------------
    // constructors - forward to base
    // -----------------------------------------------------------------

    using base::base;


    // -----------------------------------------------------------------
    // move semantics
    // -----------------------------------------------------------------
    //
    //   nary_tree must declare its move operations EXPLICITLY.  The
    // user-declared destructor below suppresses implicit move generation per
    // the Rule of Five, and the default unique_owning_policy deletes the
    // implicit copy operations - leaving the type both non-copyable and
    // non-movable if no explicit move is provided.  That breaks every consumer
    // that returns an nary_tree by value.
    //
    //   Move-from state: the source's entry pointer is released (transferred to
    // *this) and its size is reset to zero.  The source remains a valid empty
    // nary_tree whose destructor's destroy_all() walks zero nodes - no
    // double-free, no undefined behaviour.  options_container_base is
    // stateless, so it default-constructs and needs no naming here.

    nary_tree(
        nary_tree&& _other
    ) noexcept
        : base(static_cast<base&&>(_other))
    {}

    nary_tree&
    operator=(
        nary_tree&& _other
    ) noexcept
    {
        if (this != &_other)
        {
            // tear down our own contents first; destroy_all is a no-op if we
            // were already empty.
            destroy_all();

            base::operator=(static_cast<base&&>(_other));
        }

        return *this;
    }


    // -----------------------------------------------------------------
    // copy semantics - explicitly deleted
    // -----------------------------------------------------------------
    //   With the default unique_owning_policy a tree OWNS the entire node
    // graph. A defaulted copy would duplicate the entry POINTER, not the graph,
    // and both trees' destructors would then sweep the same nodes: a double
    // free. Copying a tree requires a deep walk plus a per-node clone, which is
    // not the framework's intent; other policies (shared, non-owning) that may
    // legitimately want copyable trees should provide a policy-aware clone()
    // rather than reaching for operator=.
    nary_tree(const nary_tree&)            = delete;
    nary_tree& operator=(const nary_tree&) = delete;


    // -----------------------------------------------------------------
    // destructor - sweeps the entire owned graph
    // -----------------------------------------------------------------
    ~nary_tree()
    {
        destroy_all();
    }


    // -----------------------------------------------------------------
    // 3.  topology
    // -----------------------------------------------------------------

    // root
    //   inherited as base::root() - the entry node, or nullptr.
    using base::root;
    using base::has_root;

    // parent
    //   the parent of _node, or nullptr if _node is the root or null.
    D_CONSTEXPR node_type*
    parent(
        node_type* _node
    ) const noexcept
    {
        return ( (_node != nullptr) ? _node->parent() : nullptr );
    }

    D_CONSTEXPR const node_type*
    parent(
        const node_type* _node
    ) const noexcept
    {
        return ( (_node != nullptr) ? _node->parent() : nullptr );
    }

    // children
    //   a forward range over _node's direct children -- the descent relation
    // |>, one level.  An empty range for a null or leaf node.
    D_CONSTEXPR sibling_range_type
    children(
        node_type* _node
    ) noexcept
    {
        return sibling_range_type(
            (_node != nullptr) ? _node->first_child() : nullptr);
    }

    D_CONSTEXPR const_sibling_range_type
    children(
        const node_type* _node
    ) const noexcept
    {
        return const_sibling_range_type(
            (_node != nullptr) ? _node->first_child() : nullptr);
    }

    // children
    //   no-arg overload: the children of the root.
    D_CONSTEXPR sibling_range_type
    children() noexcept
    {
        return children(base::entry_point());
    }

    D_CONSTEXPR const_sibling_range_type
    children() const noexcept
    {
        return const_sibling_range_type(
            (base::entry_point() != nullptr)
                ? base::entry_point()->first_child()
                : nullptr);
    }


    // -----------------------------------------------------------------
    // 4.  measure: LEVEL is not DEPTH
    // -----------------------------------------------------------------
    //   Two numbers, both natural, both wanted, and they are not the same:
    //
    //     level(n)   counts UPWARD, root to node.  It is the spec's lambda, the
    //                length of n's PATH, and the length of n's ADDRESS.
    //     height(n)  counts DOWNWARD, node to its deepest leaf.  It is the
    //                spec's DEPTH -- the number the structure axis means when
    // it calls a container "depth >= 2".
    //
    //   They agree only at a root that is also a leaf.  For a node at level L
    // in a tree of height H, level(n) + height(n) <= H, with equality exactly
    // when n lies on a longest root-to-leaf descent.  This container used to
    // have only the first, and to call it depth.

    // level
    //   the LEVEL (lambda) of _node: the number of parent links from it to the
    // root, which is at level 0. O(level). This is the length of the node's
    // address; every iterator of nary_tree_iterator.hpp reports it for free.
    level_type
    level(
        const node_type* _node
    ) const noexcept
    {
        return nary_level(_node, root());
    }

    // height
    //   the HEIGHT of _node: the longest descent below it, counted in links. A
    // leaf has height 0. This is what the spec calls DEPTH. O(subtree).
    level_type
    height(
        const node_type* _node
    ) const noexcept
    {
        level_type       best;
        const node_type* c;

        if (_node == nullptr)
        {
            return 0;
        }

        best = 0;
        c    = _node->first_child();

        // the height of a node is one more than the tallest of its children
        while (c != nullptr)
        {
            level_type h = height(c) + 1;

            if (h > best)
            {
                best = h;
            }

            c = c->next_sibling();
        }

        return best;
    }

    // height
    //   the height of the whole tree -- the DEPTH of the container, in the
    // sense the structure axis means. An empty tree has height 0; so does a
    // lone root, and has_root() tells them apart.
    level_type
    height() const noexcept
    {
        return height(root());
    }

    // depth
    //   the retained spelling of level(). It counts parent links upward, which
    // is the LEVEL; the spec reserves depth for the height. Kept so existing
    // callers compile; prefer level().
    depth_type
    depth(
        const node_type* _node
    ) const noexcept
    {
        return static_cast<depth_type>(level(_node));
    }


    // -----------------------------------------------------------------
    // 5.  ADDRESSABILITY
    // -----------------------------------------------------------------
    //   A tree's components are in bijection with the PATHS to them, and a path
    // is named by the word of LABELS along it -- its ADDRESS.  The root
    // contributes NO label: addressing starts from it, it is not a step taken.
    // So |address(n)| == level(n), which is exactly what resolve() consumes.
    //
    //   An LCRS node carries a PAYLOAD, not a label, so a label must be DERIVED
    // from the value: gamma = lab . val.  The Labeller parameter is that lab,
    // and it defaults to nary_identity_label -- the value IS its own label.
    //
    //   SEPARATION is the precondition of all of it.  Nothing below checks it
    // on the resolution path (that would cost O(k) per step for a property of
    // the TREE, not of the query); is_addressable() decides it once.

    // find_child
    //   the child of _node bearing _label, or nullptr. THE DESCENT STEP: one
    // step of resolution, and the whole of the scanning strategy. O(k) in the
    // branching factor.
    template<typename Label,
             typename Labeller = nary_identity_label>
    node_type*
    find_child(
        node_type*    _node,
        const Label& _label,
        Labeller      _labeller = Labeller()
    ) const
    {
        return nary_find_child(_node, _label, _labeller);
    }

    // resolve
    //   the node an ADDRESS names, or nullptr. Resolution is the descent step
    // FOLDED along the label word, descending one level per label from the
    // root.
    //
    //   PRECONDITION: separation. Where two siblings share a label, this lands
    // on whichever the sibling scan reaches first. is_addressable() decides.
    template<typename Iter,
             typename Labeller = nary_identity_label>
    node_type*
    resolve(
        Iter      _begin,
        Iter      _end,
        Labeller _labeller = Labeller()
    )
    {
        node_type* current = base::entry_point();

        for (Iter it = _begin; it != _end; ++it)
        {
            if (current == nullptr)
            {
                return nullptr;
            }

            current = nary_find_child(current, *it, _labeller);
        }

        return current;
    }

    // resolve (vector overload)
    //   resolves an address held in a vector of labels.
    template<typename Label,
             typename Labeller = nary_identity_label>
    node_type*
    resolve(
        const std::vector<Label>& _address,
        Labeller                   _labeller = Labeller()
    )
    {
        return resolve(_address.begin(), _address.end(), _labeller);
    }

    // path
    //   path(_node): the chain of components from the root down to _node,
    // ROOT-FIRST, _node last. It names level(_node) + 1 components -- one more
    // than the address has labels, the extra one being the root. Empty when
    // _node is null or is not in this tree.
    std::vector<const node_type*>
    path(
        const node_type* _node
    ) const
    {
        std::vector<const node_type*> chain;
        const node_type*              n = _node;

        // climb to the root, then reverse
        while (n != nullptr)
        {
            chain.push_back(n);

            n = n->parent();
        }

        reverse_range(chain);

        return chain;
    }

    // address
    //   addr(_node): the word of LABELS along path(_node), root-first.
    //
    //   THE ROOT CONTRIBUTES NO LABEL, so the word has exactly level(_node)
    // entries -- which is exactly what resolve() consumes. Given a separating
    // labelling,
    //
    //       resolve(address(n)) == n for every n in the tree,
    //
    // and that round trip is the contract this whole section is written to
    // keep.
    template<typename Labeller = nary_identity_label>
    auto
    address(
        const node_type* _node,
        Labeller         _labeller = Labeller()
    ) const -> std::vector<
                   typename std::decay<
                       decltype(_labeller(
                           std::declval<const value_type&>()))>::type>
    {
        using label_type = typename std::decay<
            decltype(_labeller(std::declval<const value_type&>()))>::type;

        std::vector<label_type> word;
        const node_type*        n    = _node;
        const node_type*        stop = root();

        // take a label at every step EXCEPT the root's
        while ( (n != nullptr) &&
                (n != stop) )
        {
            word.push_back(_labeller(n->data()));

            n = n->parent();
        }

        // the root was never met: _node is not in this tree, so it has no
        // address
        if (n == nullptr)
        {
            word.clear();

            return word;
        }

        reverse_range(word);

        return word;
    }

    // is_ancestor
    //   true if _ancestor is a STRICT ancestor of _descendant. By the spec
    // this is the PREFIX ORDER: addr(_ancestor) is a proper prefix of
    // addr(_descendant). Walking the chain decides it in O(level).
    bool
    is_ancestor(
        const node_type* _ancestor,
        const node_type* _descendant
    ) const noexcept
    {
        const node_type* n;

        if ( (_ancestor == nullptr) ||
             (_descendant == nullptr) ||
             (_ancestor == _descendant) )
        {
            return false;
        }

        n = _descendant->parent();

        while (n != nullptr)
        {
            if (n == _ancestor)
            {
                return true;
            }

            n = n->parent();
        }

        return false;
    }

    // lca
    //   the lowest common ancestor of _a and _b -- which the spec reads as the
    // MEET of their addresses in the prefix order, the longest common prefix
    // of the two words. In a single-rooted tree the meet ALWAYS exists (at
    // worst it is the empty word, the root), so this cannot fail for two nodes
    // of one tree; it returns nullptr only for nodes of different trees.
    const node_type*
    lca(
        const node_type* _a,
        const node_type* _b
    ) const noexcept
    {
        level_type       la;
        level_type       lb;
        const node_type* ca;
        const node_type* cb;

        if ( (_a == nullptr) ||
             (_b == nullptr) )
        {
            return nullptr;
        }

        la = level(_a);
        lb = level(_b);
        ca = _a;
        cb = _b;

        // equalise levels: a common prefix is no longer than the shorter word
        while (la > lb)
        {
            ca = ca->parent();

            --la;
        }

        while (lb > la)
        {
            cb = cb->parent();

            --lb;
        }

        // climb in tandem until the two chains meet
        while (ca != cb)
        {
            if ( (ca == nullptr) ||
                 (cb == nullptr) )
            {
                return nullptr;
            }

            ca = ca->parent();
            cb = cb->parent();
        }

        return ca;
    }

    // is_separating
    //   true if the children of _node bear pairwise distinct labels -- the
    // SEPARATION condition at one node. O(k^2) in the branching factor.
    template<typename Labeller = nary_identity_label>
    bool
    is_separating(
        const node_type* _node,
        Labeller         _labeller = Labeller()
    ) const
    {
        return nary_children_separating(_node, _labeller);
    }

    // is_addressable
    //   true if EVERY node of the tree separates its children -- the condition
    // under which this tree is ADDRESSABLE, and under which
    // resolve(address(n)) == n holds for every n.
    //
    //   Separation is the multiplicity restriction mu_1 read sibling-wise. It
    // is NOT a property of the type -- an nary_tree labels by its PAYLOAD, so
    // it is a property of the VALUES currently in it, and a transformation can
    // destroy it (see transform_preserves_separation). Hence a runtime query,
    // not a trait. O(n * k^2).
    template<typename Labeller = nary_identity_label>
    bool
    is_addressable(
        Labeller _labeller = Labeller()
    ) const
    {
        const node_type* r = root();

        if (r == nullptr)
        {
            return true;
        }

        for (const_iterator it(r), e(r, nary_iterator_end_tag());
             it != e;
             ++it)
        {
            if (!nary_children_separating(it.node(), _labeller))
            {
                return false;
            }
        }

        return true;
    }


    // -----------------------------------------------------------------
    // 6.  iteration
    // -----------------------------------------------------------------
    //   begin()/end() are PRE-ORDER, as before.  end() now carries the anchor,
    // which is what lets --end() land on the last node and so gives this
    // container rbegin()/rend() for the first time.  A default-constructed
    // iterator remains an end sentinel and compares equal to this one, so every
    // existing loop is untouched.

    D_CONSTEXPR iterator
    begin() noexcept
    {
        return iterator(base::entry_point());
    }

    D_CONSTEXPR iterator
    end() noexcept
    {
        return iterator(base::entry_point(), nary_iterator_end_tag());
    }

    D_CONSTEXPR const_iterator
    begin() const noexcept
    {
        return const_iterator(base::entry_point());
    }

    D_CONSTEXPR const_iterator
    end() const noexcept
    {
        return const_iterator(base::entry_point(), nary_iterator_end_tag());
    }

    D_CONSTEXPR const_iterator
    cbegin() const noexcept
    {
        return begin();
    }

    D_CONSTEXPR const_iterator
    cend() const noexcept
    {
        return end();
    }

    // rbegin / rend
    //   reverse PRE-ORDER iteration. New: the pre-order iterator became
    // bidirectional when its stack went away and the five-link node's
    // last_child / prev_sibling were finally used.
    reverse_iterator
    rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    reverse_iterator
    rend() noexcept
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator
    rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator
    rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator
    crbegin() const noexcept
    {
        return rbegin();
    }

    const_reverse_iterator
    crend() const noexcept
    {
        return rend();
    }

    // postorder_begin / postorder_end
    //   the order a FOLD over the tree bottoms out in: a node's children are
    // complete before the node is reached. Bidirectional.
    postorder_iterator
    postorder_begin() noexcept
    {
        return postorder_iterator(base::entry_point());
    }

    postorder_iterator
    postorder_end() noexcept
    {
        return postorder_iterator(base::entry_point(),
                                  nary_iterator_end_tag());
    }

    const_postorder_iterator
    postorder_begin() const noexcept
    {
        return const_postorder_iterator(base::entry_point());
    }

    const_postorder_iterator
    postorder_end() const noexcept
    {
        return const_postorder_iterator(base::entry_point(),
                                        nary_iterator_end_tag());
    }

    // leaf_begin / leaf_end
    //   the FRONTIER: the tree's leaves, in document order. These are its
    // ELEMENTS in the sense of the spec -- the node summand skipped.
    // Bidirectional.
    leaf_iterator
    leaf_begin() noexcept
    {
        return leaf_iterator(base::entry_point());
    }

    leaf_iterator
    leaf_end() noexcept
    {
        return leaf_iterator(base::entry_point(), nary_iterator_end_tag());
    }

    const_leaf_iterator
    leaf_begin() const noexcept
    {
        return const_leaf_iterator(base::entry_point());
    }

    const_leaf_iterator
    leaf_end() const noexcept
    {
        return const_leaf_iterator(base::entry_point(),
                                   nary_iterator_end_tag());
    }

    // level_begin / level_end
    //   breadth-first: the level strata L_0, L_1, ... in order. Forward only,
    // and the only order that allocates -- it is not a local walk of the
    // descent relation, so it must remember its frontier. Each iterator
    // reports the stratum it is on through level().
    level_iterator
    level_begin() noexcept
    {
        return level_iterator(base::entry_point());
    }

    level_iterator
    level_end() noexcept
    {
        return level_iterator();
    }

    const_level_iterator
    level_begin() const noexcept
    {
        return const_level_iterator(base::entry_point());
    }

    const_level_iterator
    level_end() const noexcept
    {
        return const_level_iterator();
    }

    // root_cursor
    //   an imperative navigator anchored at the root.
    cursor
    root_cursor() noexcept
    {
        return cursor(base::entry_point());
    }

    const_cursor
    root_cursor() const noexcept
    {
        return const_cursor(base::entry_point());
    }


    // -----------------------------------------------------------------
    // capacity
    // -----------------------------------------------------------------

    using base::empty;
    using base::size;
    using base::max_size;

    // -----------------------------------------------------------------
    // mutation: root creation
    // -----------------------------------------------------------------

    // emplace_root
    //   constructs a root node from _args and installs it. Any prior tree is
    // discarded via clear(). Returns the new root pointer. The size counter is
    // reset to 1.
    template<typename... Args>
    node_type*
    emplace_root(
        Args&&... _args
    )
    {
        node_type* n;

        clear();
        n = make_node(std::forward<Args>(_args)...);

        base::set_entry(n);
        base::set_size(1);

        return n;
    }


    // -----------------------------------------------------------------
    // mutation: child insertion (O(1) - axis-aligned)
    // -----------------------------------------------------------------

    // append_child
    //   constructs a new node from _args and links it as _parent's last child.
    // O(1). Returns the new node.
    template<typename... Args>
    node_type*
    append_child(
        node_type* _parent,
        Args&&... _args
    )
    {
        node_type* n;

        n = make_node(std::forward<Args>(_args)...);
        link_as_last_child(_parent, n);
        base::increment_size();

        return n;
    }

    // prepend_child
    //   constructs a new node from _args and links it as _parent's first
    // child. O(1). Returns the new node.
    template<typename... Args>
    node_type*
    prepend_child(
        node_type* _parent,
        Args&&... _args
    )
    {
        node_type* n;

        n = make_node(std::forward<Args>(_args)...);
        link_as_first_child(_parent, n);
        base::increment_size();

        return n;
    }


    // -----------------------------------------------------------------
    // mutation: sibling insertion (O(1) - axis-aligned)
    // -----------------------------------------------------------------

    // insert_after
    //   constructs a new node from _args and links it as _sibling's immediate
    // next sibling. _sibling must not be the root. O(1). Returns the new node.
    template<typename... Args>
    node_type*
    insert_after(
        node_type* _sibling,
        Args&&... _args
    )
    {
        node_type* n;

        n = make_node(std::forward<Args>(_args)...);
        link_as_next_sibling(_sibling, n);
        base::increment_size();

        return n;
    }

    // insert_before
    //   constructs a new node from _args and links it as _sibling's immediate
    // previous sibling. _sibling must not be the root. O(1). Returns the new
    // node.
    template<typename... Args>
    node_type*
    insert_before(
        node_type* _sibling,
        Args&&... _args
    )
    {
        node_type* n;

        n = make_node(std::forward<Args>(_args)...);
        link_as_prev_sibling(_sibling, n);
        base::increment_size();

        return n;
    }


    // -----------------------------------------------------------------
    // mutation: structural moves
    // -----------------------------------------------------------------

    // detach
    //   unlinks _node from its parent and siblings, leaving its own subtree
    // intact. Caller takes ownership of the returned pointer. O(1). Detaching
    // the root yields the entry pointer and resets the tree to empty.
    node_type*
    detach(
        node_type* _node
    )
    {
        if (_node == nullptr)
        {
            return nullptr;
        }

        // Special case: detaching the root collapses the tree. release_entry
        // already zeros the size counter, so no separate accounting is needed.
        if (_node == base::entry_point())
        {
            return base::release_entry();
        }

        unlink_node(_node);
        base::set_size(base::size() - subtree_size(_node));

        return _node;
    }

    // remove_subtree
    //   detaches _node and destroys it together with all of its descendants.
    // Equivalent to detach() followed by a sweep through every reachable
    // descendant.
    void
    remove_subtree(
        node_type* _node
    )
    {
        node_type* detached;

        if (_node == nullptr)
        {
            return;
        }

        detached = detach(_node);
        destroy_subtree(detached);

        return;
    }

    // move_subtree
    //   re-parents _node onto _new_parent as the new last child. Validates
    // against cycles (re-parenting onto self or any descendant). O(depth) for
    // the cycle check, O(1) for the relink.
    bool
    move_subtree(
        node_type* _node,
        node_type* _new_parent
    )
    {
        const node_type* p;

        if ( (_node       == nullptr) ||
                (_new_parent == nullptr) ||
                (_node       == _new_parent) )
        {
            return false;
        }

        // refuse if _new_parent lies in the moving subtree
        p = _new_parent;

        while (p != nullptr)
        {
            if (p == _node)
            {
                return false;
            }

            p = p->parent();
        }

        // root moves are not supported through this path
        if (_node == base::entry_point())
        {
            return false;
        }

        unlink_node(_node);
        link_as_last_child(_new_parent, _node);

        return true;
    }


    // -----------------------------------------------------------------
    // bulk operations
    // -----------------------------------------------------------------

    // clear
    //   destroys every node in the tree and resets the entry and size to
    // empty. Always safe; no-op on an empty tree.
    void
    clear() noexcept
    {
        destroy_all();

        return;
    }



    // -----------------------------------------------------------------
    // 8.  FILTERABILITY
    // -----------------------------------------------------------------
    //   READ THIS BEFORE USING IT.  A tree is not a sequence, and "keep exactly
    // the nodes that match" is not an operation on one.  Removing a LEAF
    // disturbs nothing; removing an INTERIOR node severs every descent through
    // it, and the addresses of its whole subtree cease to resolve.  A selection
    // preserves the survivors' addresses IF AND ONLY IF the retained set is
    // PREFIX-CLOSED (ancestor-closed), and a predicate on VALUES is not in
    // general ancestor-closed.
    //
    //   So the naive reading is not offered.  These are:
    //
    //     filter(p)    const.  Returns a NEW TREE holding the ANCESTOR-CLOSURE
    //                  of the matching set: a node is kept if it, OR ANY
    //                  DESCENDANT, matches.  Prefix-closed by construction, so
    //                  every survivor keeps its address.  `grep -r` semantics.
    //     prune(p)     the same closure, applied IN PLACE.  Returns the count
    //                  removed.
    //     erase_if(p)  in place, the DUAL: a matching node is removed TOGETHER
    //                  WITH ITS SUBTREE, since its children cannot outlive it.
    //                  Also prefix-closed.
    //     select(p)    read only: gather the matching nodes.  No tree is built,
    //                  so no structural question arises.
    //
    //   prune and erase_if are DUALS, not complements: prune(p) keeps what p
    // reaches from BELOW, erase_if(p) drops what p reaches from ABOVE.  All
    // three mutating readings are total and all three preserve addressability.
    // Nothing else does.
    //
    //   WHY filter() IS THE CONST ONE.  The framework's native_filter_helper
    // (container_filter_traits.hpp) and filter.hpp's has_filter_method BOTH
    // probe for `declval<const T&>().filter(bool(*)(const value_type&))` -- a
    // CONST member producing a RESULT.  That is the "read plus build" reading
    // of the Filterability axis, and satisfying it is what makes this
    // container's filter strategy NATIVE rather than external.  The in-place
    // readings are named apart precisely so the word `filter` can mean what the
    // axis says it means.

    // filter
    //   const: a NEW TREE holding the ANCESTOR-CLOSURE of {n : _pred(n)} -- a
    // node is kept if it, or any node beneath it, matches. This tree is not
    // touched.
    //
    //   PREFIX-CLOSED, and therefore ADDRESSABILITY-PRESERVING: if a node is
    // kept, so is every one of its ancestors -- an ancestor of a match is
    // itself an ancestor of a match -- so every survivor's path is intact in
    // the result and its address still resolves there. That is precisely the
    // condition the spec puts on filtering a hierarchy, and it is why this,
    // and not the naive reading, is what filter MEANS here.
    //
    //   When nothing matches, the closure is empty and the result is an empty
    // tree. This is the primitive has_native_filter detects.
    template<typename Pred,
             typename std::enable_if<
                 is_predicate<Pred, const ValueType&>::value,
                 int>::type = 0>
    nary_tree
    filter(
        Pred _pred
    ) const
    {
        nary_tree        out;
        const node_type* r = root();

        if (r == nullptr)
        {
            return out;
        }

        clone_closure(r, out, nullptr, _pred);

        return out;
    }

    // prune
    //   the same ANCESTOR-CLOSURE, applied IN PLACE. Returns the number of
    // nodes removed. The destructive twin of filter(), for when the tree is
    // large and a copy is not wanted.
    template<typename Pred,
             typename std::enable_if<
                 is_predicate<Pred, const ValueType&>::value,
                 int>::type = 0>
    size_type
    prune(
        Pred _pred
    )
    {
        size_type  before = base::size();
        node_type* r      = base::entry_point();

        if (r == nullptr)
        {
            return 0;
        }

        // the root survives only if something in the tree matched
        if (!prune_node(r, _pred))
        {
            clear();

            return before;
        }

        return (before - base::size());
    }

    // erase_if
    //   removes every node satisfying _pred TOGETHER WITH ITS SUBTREE. Returns
    // the number of nodes removed.
    //
    //   The subtree goes too because it must: a child cannot outlive the
    // parent that addresses it. The retained set is {m : no ancestor-or-self
    // of m matches}, which is PREFIX-CLOSED -- if m is kept then so is its
    // parent, whose ancestors-or-self are a subset of m's -- so this too
    // preserves the survivors' addresses. It is the DUAL of prune, not its
    // complement.
    template<typename Pred,
             typename std::enable_if<
                 is_predicate<Pred, const ValueType&>::value,
                 int>::type = 0>
    size_type
    erase_if(
        Pred _pred
    )
    {
        size_type  before = base::size();
        node_type* r      = base::entry_point();

        if (r == nullptr)
        {
            return 0;
        }

        // a matching root takes the whole tree with it
        if (_pred(static_cast<const value_type&>(r->data())))
        {
            clear();

            return before;
        }

        erase_children_if(r, _pred);

        return (before - base::size());
    }

    // select
    //   the nodes whose value satisfies _pred, in pre-order. A read: the tree
    // is not modified and no addresses move. This is the container in its
    // is_filter_source reading -- supplying elements to a selection without
    // building one.
    template<typename Pred,
             typename std::enable_if<
                 is_predicate<Pred, const ValueType&>::value,
                 int>::type = 0>
    std::vector<node_type*>
    select(
        Pred _pred
    )
    {
        std::vector<node_type*> out;
        node_type*              r = base::entry_point();

        if (r == nullptr)
        {
            return out;
        }

        for (iterator it(r), e(r, nary_iterator_end_tag()); it != e; ++it)
        {
            if (_pred(static_cast<const value_type&>(*it)))
            {
                out.push_back(it.node());
            }
        }

        return out;
    }

    template<typename Pred,
             typename std::enable_if<
                 is_predicate<Pred, const ValueType&>::value,
                 int>::type = 0>
    std::vector<const node_type*>
    select(
        Pred _pred
    ) const
    {
        std::vector<const node_type*> out;
        const node_type*              r = root();

        if (r == nullptr)
        {
            return out;
        }

        for (const_iterator it(r), e(r, nary_iterator_end_tag());
             it != e;
             ++it)
        {
            if (_pred(*it))
            {
                out.push_back(it.node());
            }
        }

        return out;
    }

    // count_if
    //   how many nodes satisfy _pred. A read.
    template<typename Pred,
             typename std::enable_if<
                 is_predicate<Pred, const ValueType&>::value,
                 int>::type = 0>
    size_type
    count_if(
        Pred _pred
    ) const
    {
        size_type        n = 0;
        const node_type* r = root();

        if (r == nullptr)
        {
            return 0;
        }

        for (const_iterator it(r), e(r, nary_iterator_end_tag());
             it != e;
             ++it)
        {
            if (_pred(*it))
            {
                ++n;
            }
        }

        return n;
    }


    // -----------------------------------------------------------------
    // 9.  TRANSFORMABILITY
    // -----------------------------------------------------------------

    // transform
    //   const: a NEW TREE of the SAME SHAPE, every value rewritten to _fn(v).
    // Every position is kept and every arrangement with it, so every PATH --
    // and hence every ADDRESS -- carries over intact.
    //
    //   WITH ONE EXCEPTION, and it is the spec's. Where the label is DERIVED
    // from the value (gamma = lab . val), an _fn that is not injective ON
    // LABELS can collide two siblings onto one label and BREAK SEPARATION --
    // and a tree that does not separate is not addressable, whatever its
    // shape.
    // This is not a new hazard: it is the loss of mu_1 that the
    // Transformability axis already records, seen here in its structural
    // consequence. Ask transform_preserves_separation() BEFORE applying _fn,
    // not after.
    //
    //   Const and result-returning for the same reason filter() is: this is
    // the shape has_native_transform detects.
    template<typename Fn,
             typename std::enable_if<
                 is_callable<Fn, const ValueType&>::value,
                 int>::type = 0>
    nary_tree
    transform(
        Fn _fn
    ) const
    {
        nary_tree        out;
        const node_type* r = root();

        if (r == nullptr)
        {
            return out;
        }

        node_type* nr = out.emplace_root(_fn(r->data()));

        clone_mapped(r, out, nr, _fn);

        return out;
    }

    // transform_in_place
    //   rewrites every value where it stands, v <- _fn(v). Same guarantee and
    // same caveat as transform(), without the copy.
    template<typename Fn,
             typename std::enable_if<
                 is_callable<Fn, const ValueType&>::value,
                 int>::type = 0>
    void
    transform_in_place(
        Fn _fn
    )
    {
        node_type* r = base::entry_point();

        if (r == nullptr)
        {
            return;
        }

        for (iterator it(r), e(r, nary_iterator_end_tag()); it != e; ++it)
        {
            *it = _fn(static_cast<const value_type&>(*it));
        }

        return;
    }

    // transform_preserves_separation
    //   true if applying _fn would leave every node's children bearing
    // distinct labels -- that is, if the map would NOT cost this tree its
    // addressability.
    //
    //   The executable form of the spec's warning. Selection preserves mu_1
    // and so preserves separation; a map MAY break mu_1, and where the label
    // is value-derived that break IS a break of addressability. Checked
    // WITHOUT mutating anything, so a caller may decide before committing. O(n
    // * k^2).
    template<typename Fn,
             typename Labeller = nary_identity_label>
    bool
    transform_preserves_separation(
        Fn        _fn,
        Labeller _labeller = Labeller()
    ) const
    {
        const node_type* r = root();

        if (r == nullptr)
        {
            return true;
        }

        for (const_iterator it(r), e(r, nary_iterator_end_tag());
             it != e;
             ++it)
        {
            const node_type* outer = it.node()->first_child();

            // would the children of this node still separate?
            while (outer != nullptr)
            {
                const node_type* inner = outer->next_sibling();

                while (inner != nullptr)
                {
                    if (_labeller(_fn(outer->data())) ==
                        _labeller(_fn(inner->data())))
                    {
                        return false;
                    }

                    inner = inner->next_sibling();
                }

                outer = outer->next_sibling();
            }
        }

        return true;
    }


    // -----------------------------------------------------------------
    // 10. SEQUENCE BRIDGE
    // -----------------------------------------------------------------
    //   filter.hpp is a SEQUENCE library: its operations are
    // std::function<vector<size_t>(const vector<T>&)>, and its is_filterable --
    // like the framework's is_container_filterable -- wants push_back or
    // insert. This container has neither and should have neither: a tree has no
    // "back" to push onto, and WHERE a new node goes is a structural decision,
    // not an appending one.  It is therefore a filter SOURCE (is_filter_source)
    // and INPUT-ONLY (is_filter_input_only) by those traits, which is exactly
    // right -- and it is separately NATIVE (has_native_filter), because
    // filter() above gives it a selection primitive of its own.
    //
    //   These flatten it so filter.hpp's chains and builders may run over its
    // ELEMENTS.  What comes back is a SEQUENCE: the tree's shape is not
    // recoverable from it, and a filtered sequence cannot be poured back in.
    // That is not a shortcoming of the bridge -- it is what the spec says
    // filtering a hierarchy costs, and the tree-shaped alternatives are
    // filter() / prune() / erase_if() above.

    // values
    //   every value in the tree, in pre-order (document order).
    std::vector<value_type>
    values() const
    {
        std::vector<value_type> out;
        const node_type*        r = root();

        if (r == nullptr)
        {
            return out;
        }

        out.reserve(base::size());

        for (const_iterator it(r), e(r, nary_iterator_end_tag());
             it != e;
             ++it)
        {
            out.push_back(*it);
        }

        return out;
    }

    // leaves
    //   the FRONTIER: the values at the tree's leaves, in document order.
    // These are its ELEMENTS in the sense of the spec -- the node summand
    // skipped.
    std::vector<value_type>
    leaves() const
    {
        std::vector<value_type> out;
        const node_type*        r = root();

        if (r == nullptr)
        {
            return out;
        }

        for (const_leaf_iterator it(r), e(r, nary_iterator_end_tag());
             it != e;
             ++it)
        {
            out.push_back(*it);
        }

        return out;
    }

    // nodes
    //   every node handle, in pre-order. For callers that need the STRUCTURE
    // alongside the values -- a level, an address, a parent -- which a flat
    // vector of values throws away.
    std::vector<node_type*>
    nodes()
    {
        std::vector<node_type*> out;
        node_type*              r = base::entry_point();

        if (r == nullptr)
        {
            return out;
        }

        out.reserve(base::size());

        for (iterator it(r), e(r, nary_iterator_end_tag()); it != e; ++it)
        {
            out.push_back(it.node());
        }

        return out;
    }

    std::vector<const node_type*>
    nodes() const
    {
        std::vector<const node_type*> out;
        const node_type*              r = root();

        if (r == nullptr)
        {
            return out;
        }

        out.reserve(base::size());

        for (const_iterator it(r), e(r, nary_iterator_end_tag());
             it != e;
             ++it)
        {
            out.push_back(it.node());
        }

        return out;
    }

private:
    // -----------------------------------------------------------------
    // node construction / destruction
    // -----------------------------------------------------------------

    template<typename... Args>
    node_type*
    make_node(
        Args&&... _args
    )
    {
        allocator_type a;
        node_type*     n;

        a = base::get_allocator();
        n = alloc_traits::allocate(a, 1);
        alloc_traits::construct(a,
                                n,
                                std::forward<Args>(_args)...);

        return n;
    }

    void
    destroy_node(
        node_type* _node
    ) noexcept
    {
        allocator_type a;

        if (_node == nullptr)
        {
            return;
        }

        a = base::get_allocator();
        alloc_traits::destroy(a, _node);
        alloc_traits::deallocate(a, _node, 1);

        return;
    }

    // destroy_subtree
    //   iteratively destroys _node and all descendants. Uses
    // an explicit work stack to keep stack depth bounded by
    // the heap, not the tree depth, which matters for pathologically deep
    // trees.
    void
    destroy_subtree(
        node_type* _node
    ) noexcept
    {
        std::vector<node_type*> stack;
        node_type*              n;
        node_type*              c;
        node_type*              next;

        if (_node == nullptr)
        {
            return;
        }

        stack.push_back(_node);

        while (!stack.empty())
        {
            n = stack.back();
            stack.pop_back();

            // push children before destroying the parent
            c = n->first_child();

            while (c != nullptr)
            {
                next = c->next_sibling();
                stack.push_back(c);
                c = next;
            }

            destroy_node(n);
        }

        return;
    }

    // destroy_all
    //   destroys every node owned by this tree. Two paths:
    //     - unique_owning_policy: release the head from the
    //       unique_ptr (which does NOT destroy it) and sweep
    //       the entire graph through the allocator. This is
    //       the only ownership mode that cleans descendants
    //       reliably.
    //     - any other policy: defer to base::clear(). For
    //       non_owning the caller is responsible for node
    //       lifetime. For shared_owning the head's refcount
    //       is decremented; if we held the last reference the
    //       head is destroyed but raw-pointer descendants
    //       leak - that combination is not recommended for
    //       this node design and the user should pick
    //       unique_owning_policy if they want automatic
    //       cleanup.
    void
    destroy_all() noexcept
    {
        node_type* r;

        if constexpr (
            std::is_same<OwnershipPolicy,
                            unique_owning_policy>::value)
        {
            r = base::entry_point();

            if (r != nullptr)
            {
                base::release_entry();
                destroy_subtree(r);
            }

            base::set_size(0);
        }
        else
        {
            base::clear();
        }

        return;
    }


    // -----------------------------------------------------------------
    // link helpers (private; preserve all five LCRS pointers)
    // -----------------------------------------------------------------

    static void
    link_as_first_child(
        node_type* _parent,
        node_type* _child
    ) noexcept
    {
        node_type* old_first;

        old_first = _parent->first_child_slot();

        _child->parent_slot()       = _parent;
        _child->prev_sibling_slot() = nullptr;
        _child->next_sibling_slot() = old_first;

        if (old_first != nullptr)
        {
            old_first->prev_sibling_slot() = _child;
        }
        else
        {
            _parent->last_child_slot() = _child;
        }

        _parent->first_child_slot() = _child;

        return;
    }

    static void
    link_as_last_child(
        node_type* _parent,
        node_type* _child
    ) noexcept
    {
        node_type* old_last;

        old_last = _parent->last_child_slot();

        _child->parent_slot()       = _parent;
        _child->prev_sibling_slot() = old_last;
        _child->next_sibling_slot() = nullptr;

        if (old_last != nullptr)
        {
            old_last->next_sibling_slot() = _child;
        }
        else
        {
            _parent->first_child_slot() = _child;
        }

        _parent->last_child_slot() = _child;

        return;
    }

    static void
    link_as_next_sibling(
        node_type* _anchor,
        node_type* _new_node
    ) noexcept
    {
        node_type* old_next;
        node_type* p;

        old_next = _anchor->next_sibling_slot();
        p        = _anchor->parent_slot();

        _new_node->parent_slot()       = p;
        _new_node->prev_sibling_slot() = _anchor;
        _new_node->next_sibling_slot() = old_next;

        _anchor->next_sibling_slot() = _new_node;

        if (old_next != nullptr)
        {
            old_next->prev_sibling_slot() = _new_node;
        }
        else if (p != nullptr)
        {
            p->last_child_slot() = _new_node;
        }

        return;
    }

    static void
    link_as_prev_sibling(
        node_type* _anchor,
        node_type* _new_node
    ) noexcept
    {
        node_type* old_prev;
        node_type* p;

        old_prev = _anchor->prev_sibling_slot();
        p        = _anchor->parent_slot();

        _new_node->parent_slot()       = p;
        _new_node->prev_sibling_slot() = old_prev;
        _new_node->next_sibling_slot() = _anchor;

        _anchor->prev_sibling_slot() = _new_node;

        if (old_prev != nullptr)
        {
            old_prev->next_sibling_slot() = _new_node;
        }
        else if (p != nullptr)
        {
            p->first_child_slot() = _new_node;
        }

        return;
    }

    // unlink_node
    //   removes _node from its current parent / sibling links, leaving its own
    // subtree intact. All four affected pointers (parent's first/last and the
    // two siblings' next/prev) are repaired. O(1).
    static void
    unlink_node(
        node_type* _node
    ) noexcept
    {
        node_type* p;
        node_type* prev;
        node_type* next;

        p    = _node->parent_slot();
        prev = _node->prev_sibling_slot();
        next = _node->next_sibling_slot();

        if (prev != nullptr)
        {
            prev->next_sibling_slot() = next;
        }
        else if (p != nullptr)
        {
            p->first_child_slot() = next;
        }

        if (next != nullptr)
        {
            next->prev_sibling_slot() = prev;
        }
        else if (p != nullptr)
        {
            p->last_child_slot() = prev;
        }

        _node->parent_slot()       = nullptr;
        _node->prev_sibling_slot() = nullptr;
        _node->next_sibling_slot() = nullptr;

        return;
    }

    // subtree_size
    //   counts the number of nodes in _node's subtree (inclusive).
    // O(subtree_size). Used by detach() to keep the tree's size counter
    // consistent.
    static size_type
    subtree_size(
        const node_type* _node
    ) noexcept
    {
        std::vector<const node_type*> stack;
        const node_type*              n;
        const node_type*              c;
        size_type                     n_count;

        if (_node == nullptr)
        {
            return 0;
        }

        n_count = 0;
        stack.push_back(_node);

        while (!stack.empty())
        {
            n = stack.back();
            stack.pop_back();
            ++n_count;

            c = n->first_child();

            while (c != nullptr)
            {
                stack.push_back(c);
                c = c->next_sibling();
            }
        }

        return n_count;
    }

    // -----------------------------------------------------------------
    // private: the prefix-closed selection helpers
    // -----------------------------------------------------------------

    // clone_closure
    //   clones into _out the ANCESTOR-CLOSURE of {n : _pred(n)} taken over
    // _src's subtree, hanging the result under _dst_parent (or as _out's root
    // when _dst_parent is null). Returns true if _src was kept -- that is, if
    // _src or anything beneath it matched.
    //
    //   The node is cloned FIRST and un-cloned if it turns out to have earned
    // no place: whether a node survives is a question about its DESCENDANTS,
    // so
    // it cannot be answered before they are walked, and cloning eagerly lets
    // that one walk do both jobs. The undo is O(1) -- if nothing beneath _src
    // matched then every child clone was already removed, so the clone is a
    // leaf by the time we reach it. Total cost stays O(n).
    template<typename Pred>
    bool
    clone_closure(
        const node_type* _src,
        nary_tree&       _out,
        node_type*       _dst_parent,
        Pred&           _pred
    ) const
    {
        node_type*       d;
        const node_type* c;
        bool             any = false;

        d = ( (_dst_parent == nullptr)
                  ? _out.emplace_root(_src->data())
                  : _out.append_child(_dst_parent, _src->data()) );

        c = _src->first_child();

        while (c != nullptr)
        {
            if (clone_closure(c, _out, d, _pred))
            {
                any = true;
            }

            c = c->next_sibling();
        }

        if ( any ||
             _pred(_src->data()) )
        {
            return true;
        }

        // nothing at or beneath _src matched: it has no place in the closure
        _out.remove_subtree(d);

        return false;
    }

    // clone_mapped
    //   clones _src's children into _out under _dst, mapping every value
    // through _fn and preserving sibling order -- so the result has the SAME
    // SHAPE, and every address carries over.
    template<typename Fn>
    void
    clone_mapped(
        const node_type* _src,
        nary_tree&       _out,
        node_type*       _dst,
        Fn&             _fn
    ) const
    {
        const node_type* c;

        c = _src->first_child();

        while (c != nullptr)
        {
            node_type* d = _out.append_child(_dst, _fn(c->data()));

            clone_mapped(c, _out, d, _fn);

            c = c->next_sibling();
        }

        return;
    }

    // prune_node
    //   returns true if _node, or ANY node beneath it, satisfies _pred -- and,
    // on the way, destroys every child subtree that contains no match. The
    // in-place counterpart of clone_closure.
    //
    //   The recursion is post-order by necessity: whether a node survives is a
    // question about its DESCENDANTS, so they must be settled first. A child's
    // next_sibling is captured BEFORE the child can be unlinked, or the walk
    // would follow a dead pointer.
    //
    //   What this computes is the ANCESTOR-CLOSURE of {n : _pred(n)}, and that
    // set is prefix-closed by construction: an ancestor of a match is an
    // ancestor of a match. Hence every survivor keeps its path, and its
    // address still resolves.
    template<typename Pred>
    bool
    prune_node(
        node_type* _node,
        Pred&     _pred
    )
    {
        bool       any = false;
        node_type* c;

        c = _node->first_child();

        while (c != nullptr)
        {
            // capture the link before the child can be unlinked
            node_type* next = c->next_sibling();

            if (prune_node(c, _pred))
            {
                any = true;
            }
            else
            {
                // nothing beneath c matched: c's whole subtree goes
                remove_subtree(c);
            }

            c = next;
        }

        return ( any ||
                 _pred(static_cast<const value_type&>(_node->data())) );
    }

    // erase_children_if
    //   removes every child subtree of _node whose root satisfies _pred, and
    // recurses into the survivors. A matching node takes its subtree with it,
    // because a child cannot outlive the parent that addresses it; a survivor
    // is descended into, because a match may still lie beneath it.
    template<typename Pred>
    void
    erase_children_if(
        node_type* _node,
        Pred&     _pred
    )
    {
        node_type* c;

        c = _node->first_child();

        while (c != nullptr)
        {
            // capture the link before the child can be unlinked
            node_type* next = c->next_sibling();

            if (_pred(static_cast<const value_type&>(c->data())))
            {
                remove_subtree(c);
            }
            else
            {
                erase_children_if(c, _pred);
            }

            c = next;
        }

        return;
    }

    // reverse_range
    //   reverses a vector in place. Used to turn the leaf-first climb of a
    // parent chain into the root-first order a path and an address are read
    // in.
    template<typename Container>
    static void
    reverse_range(
        Container& _c
    ) noexcept
    {
        std::size_t lo = 0;
        std::size_t hi = _c.size();

        while (lo < hi)
        {
            --hi;

            auto tmp = _c[lo];
            _c[lo]   = _c[hi];
            _c[hi]   = tmp;

            ++lo;
        }

        return;
    }
};


// =========================================================================
// III. CONVENIENCE ALIASES
// =========================================================================

// owning_nary_tree
//   alias: nary_tree with unique_owning_policy. Move-only RAII semantics. This
// is the same as the default - provided for symmetry with node_container's
// family of aliases.
template<typename ValueType,
         typename Allocator   = std::allocator<nary_tree_node<ValueType>>,
         typename LockPolicy = void,
         typename... Options>
using owning_nary_tree = nary_tree<ValueType,
                                   Allocator,
                                   LockPolicy,
                                   unique_owning_policy,
                                   Options...>;

// shared_nary_tree
//   alias: nary_tree with shared_owning_policy. Copyable; multiple trees may
// share refcounted ownership of a graph.
template<typename ValueType,
         typename Allocator   = std::allocator<nary_tree_node<ValueType>>,
         typename LockPolicy = void,
         typename... Options>
using shared_nary_tree = nary_tree<ValueType,
                                   Allocator,
                                   LockPolicy,
                                   shared_owning_policy,
                                   Options...>;

// non_owning_nary_tree
//   alias: nary_tree with non_owning_policy. The container does not destroy
// nodes; useful when nodes live in an external arena or pool.
template<typename ValueType,
         typename Allocator   = std::allocator<nary_tree_node<ValueType>>,
         typename LockPolicy = void,
         typename... Options>
using non_owning_nary_tree = nary_tree<ValueType,
                                       Allocator,
                                       LockPolicy,
                                       non_owning_policy,
                                       Options...>;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_HPP
