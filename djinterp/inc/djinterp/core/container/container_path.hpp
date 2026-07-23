/******************************************************************************
* djinterp [container]                                      container_path.hpp
*
* Path resolution over indexed containers:
*   This header provides templated path traversal and construction for any
* container that supports indexed access to elements which have a hierarchical
* (parent/child) relationship.  The container itself need not be a tree - it
* could be a flat array, a vector, or an arena - as long as an accessor policy
* can extract parent, first_child, next_sibling, and component from each
* element.
*
*   The accessor policy is an object with const member functions.  All functions
* accept the policy as their first parameter; template arguments are fully
* deducible from the call site.
*
* THE SPEC (Addressability).  Two objects are in play here and they are NOT the
* same object; the whole of what follows turns on keeping them apart.
*
*     PATH      path(n) = ( c |> n_1 |> ... |> n_N ),   a chain of COMPONENTS.
*               It NAMES N + 1 components -- the root among them.
*     ADDRESS   addr(n) = < gamma(n_1), ..., gamma(n_N) >,  a word of LABELS.
*               It carries N labels.  THE ROOT CONTRIBUTES NONE: addressing
*               starts from the root, it is not a step taken.
*
* So |addr(n)| == level(n), and an address is exactly one label SHORTER than its
* path is long.  Resolution descends by consuming labels, one per CHILD step:
*
*     resolve(root, addr(n)) == n        and       addr(resolve(root, w)) == w
*
* and this round trip is the contract every function below is written to keep.
* container_path_round_trips will assert it over a whole subtree.
*
* SEPARATION IS A PRECONDITION, NOT A NICETY.  The round trip holds if and only
* if the labelling SEPARATES -- the children of every node bear distinct
* components.  That is the multiplicity restriction mu_1 read sibling-wise:
* under
* it a node's children are a MAP from label to child.  Without it, one address
* names two components, resolve returns whichever it scanned first, and NO
* implementation of resolve can invert addr.  container_path_is_separating and
* container_path_is_separating_subtree decide it; resolve does not check,
* because
* checking on the resolution path would cost O(k) per step for a property of the
* TREE, not of the query.
*
* LEVEL IS NOT DEPTH.  container_path_level counts parent links UPWARD, to the
* root; that is the spec's lambda, and it is the length of an address.  The
* spec's depth is a node's HEIGHT, counted DOWNWARD to its deepest leaf.  They
* are different numbers on the same node.  container_path_depth is retained as a
* spelling of level(), because that is what it always computed.
*
* WHAT CHANGED, AND WHY IT MATTERS:
*   - collect() used to include the ROOT's own component, so it returned
*     level(n) + 1 components where resolve consumes level(n).  build(n)
* produced
*     "root/a/b" and resolve(root, build(n)) != n.  It now stops at the anchor,
*     exclusive.  (Two neighbours already had this right:
* container_path_relative
*     stops at the lca, and path_split("/a/b") yields ["a","b"].)
*   - relative() returned {0, {}} when the lca was null -- the same value it
*     returns for relative(a, a).  path_address now carries `valid`.
*   - The sibling scan was open-coded twice here and once more in the iterator
*     header.  Resolution now folds tree_find_child, which is the descent step.
*   - component_view::operator== called memcmp(null, null, 0) on two empty
* views.
*
* Contents:
*   Types:
*     - component_view                  non-owning view for string components
*     - path_address                    relative address: up_count + labels
*
*   Generic operations:
*     - container_path_resolve          resolve a label word to a component
*     - container_path_address          the LABEL WORD root-->node (root
* excluded)
*     - container_path_collect          alias of address (compatibility
* spelling)
*     - container_path_level            lambda: parent links to the root
*     - container_path_depth            retained spelling of level()
*     - container_path_root             the null-parented ancestor
*     - container_path_ancestors        parent-->root chain
*     - container_path_ancestor_chain   root-->node chain
*     - container_path_lca              the MEET in the prefix order
*     - container_path_relative         relative address between two nodes
*     - container_path_is_ancestor      strict ancestry (the prefix order)
*     - container_path_is_ancestor_or_self
*     - container_path_is_separating    mu_1 at one node
*     - container_path_is_separating_subtree
*     - container_path_round_trips      the Proposition, executable
*
*   String convenience:
*     - container_path_resolve          (const char* / std::string overloads)
*     - container_path_build            construct a string path with separators
*     - container_path_build_from       the same, anchored at a chosen ancestor
*     - container_path_build_is_faithful   no label may contain the separator
*     - container_path_relative_string  relative address as a ".." string
*
*
* path:      /inc/djinterp/core/container/container_path.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.02
******************************************************************************/

#ifndef DJINTERP_CONTAINER_PATH_
#define DJINTERP_CONTAINER_PATH_ 1

// std
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "../paradigm/path/path.hpp"
#include "./iterator/linked_tree_iterator.hpp"
#include "./iterator/path_iterator.hpp"


NS_DJINTERP


// ================================================================
//  container_path_policy concept
// ================================================================
//
// A valid policy object _policy for container _Container, index type _Index,
// and component type _Component must provide these type aliases:
//
//   container_type  - the container being navigated
//   index_type      - the element address type
//   component_type  - the path component type (the LABEL)
//
// and these const member functions:
//
//   index_type      null_index  ()                                   const
//   bool            is_null     (index_type)                         const
//   index_type      parent      (const container_type&, index_type)  const
//   index_type      first_child (const container_type&, index_type)  const
//   index_type      next_sibling(const container_type&, index_type)  const
//   component_type  component   (const container_type&, index_type)  const
//
// component_type must support operator== against the labels supplied to a
// resolve.
//
// SEPARATION.  Every operation that RESOLVES presupposes that the children of a
// node bear DISTINCT components.  The policy cannot enforce this -- it is a
// property of the tree, not of the accessor -- so it is stated here and decided
// by container_path_is_separating.  A tree that does not separate is at best
// MULTI-addressable: a label names a SET of children, and no resolve inverts
// addressing.


// ================================================================
//  component_view
// ================================================================

// component_view
//   struct: a lightweight, non-owning view into a character buffer.  Used as
// the
// component_type for string-based path policies where elements are named by
// substrings of a string pool.
struct component_view
{
    const char* data;
    std::size_t length;

    // operator==
    //   compares two views for byte-wise equality.  The zero-length case is
    // short-circuited: memcmp with a null pointer is undefined even for a count
    // of zero, and two empty views are equal without inspecting any byte.
    bool
    operator==
    (
        const component_view& _other
    ) const
    {
        if (length != _other.length)
        {
            return false;
        }

        if (length == 0)
        {
            return true;
        }

        return (std::memcmp(data, _other.data, length) == 0);
    }

    // operator!=
    //   compares two views for byte-wise inequality.
    bool
    operator!=
    (
        const component_view& _other
    ) const
    {
        return !(*this == _other);
    }
};


// ================================================================
//  path_address
// ================================================================

// path_address
//   struct: a RELATIVE address -- the (k, v) pair of the spec.  k ascents from
// the source to the meet of the two addresses, followed by the label word v
// descending from the meet to the destination.
//
//   valid = false, up_count = 0, components empty  --> NO relative address
//                                                      exists: the two nodes
// lie
//                                                      in different trees and
//                                                      their addresses have no
//                                                      meet.
//   valid = true,  up_count = 0, components empty  --> the same node.
//   valid = true,  up_count = 2, components = {e,f} --> ../../e/f
//
//   The `valid` flag is what tells those first two cases apart.  Without it the
// caller cannot distinguish "these nodes are unrelated" from "these nodes are
// the same", which are not remotely the same answer.
//
//   This representation is component-type-agnostic: it works with string paths,
// integer keys, bit patterns, and anything else a policy names children by.
template<typename _Component>
struct path_address
{
    std::size_t             up_count;
    std::vector<_Component> components;
    bool                    valid;

    // path_address (default)
    //   an INVALID address: no relation between the two nodes has been
    // established.  container_path_relative sets valid explicitly.
    path_address()
        : up_count(0),
          components(),
          valid(false)
    {
    }
};


// ================================================================
//  container_path_resolve (generic)
// ================================================================

// container_path_resolve
//   function: descends from _root, consuming one label per CHILD step, and
// returns the component the label word names -- or the null sentinel if any
// label matches no child.
//
//   This is the fold of the descent step (tree_find_child) along the word.  A
// word of N labels descends N levels, so resolve(root, addr(n)) lands on the
// component at level N: the root itself consumes NOTHING, which is why an
// address of the root is the empty word.
//
//   PRECONDITION: separation (see the policy concept).  Where two siblings
// share
// a label, this returns the first the sibling scan reaches.
template<typename _Policy,
         typename _Container,
         typename _Index,
         typename _Iter>
_Index
container_path_resolve
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _root,
    _Iter             _begin,
    _Iter             _end
)
{
    _Index current = _root;

    // fold the descent step along the label word
    for (_Iter it = _begin; it != _end; ++it)
    {
        if (_policy.is_null(current))
        {
            return _policy.null_index();
        }

        current = tree_find_child(_policy, _container, current, *it);
    }

    return current;
}

// container_path_resolve (pointer + count overload)
//   function: resolves from a contiguous array of labels.
template<typename _Policy,
         typename _Container,
         typename _Index,
         typename _Component>
_Index
container_path_resolve
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _root,
    const _Component* _components,
    std::size_t       _count
)
{
    return container_path_resolve(
        _policy,
        _container,
        _root,
        _components,
        _components + _count);
}

// container_path_resolve (vector overload)
//   function: resolves from a vector of labels.
template<typename _Policy,
         typename _Container,
         typename _Index,
         typename _Component>
_Index
container_path_resolve
(
    const _Policy&                 _policy,
    const _Container&              _container,
    _Index                         _root,
    const std::vector<_Component>& _components
)
{
    return container_path_resolve(
        _policy,
        _container,
        _root,
        _components.data(),
        _components.size());
}


// ================================================================
//  container_path_root / container_path_level
// ================================================================

// container_path_root
//   function: the root of _id's tree -- the ancestor with no parent.  Returns
// _id itself when _id is already a root, and the null sentinel when _id is
// null.
template<typename _Policy,
         typename _Container,
         typename _Index>
_Index
container_path_root
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _id
)
{
    _Index current = _id;

    // climb until a node with no parent is found
    while (!_policy.is_null(current))
    {
        _Index parent = _policy.parent(_container, current);

        if (_policy.is_null(parent))
        {
            return current;
        }

        current = parent;
    }

    return _policy.null_index();
}


// container_path_level
//   function: the LEVEL (lambda) of _id -- the number of parent links from _id
// up to its root, which is at level 0.  This is the length of _id's address.
//
//   It is NOT the spec's depth, which is a node's HEIGHT, counted downward to
// its deepest leaf.  The two are the complementary halves of one descent.
template<typename _Policy,
         typename _Container,
         typename _Index>
std::size_t
container_path_level
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _id
)
{
    std::size_t level   = 0;
    _Index      current = _policy.parent(_container, _id);

    // count parent links to the root
    while (!_policy.is_null(current))
    {
        ++level;

        current = _policy.parent(_container, current);
    }

    return level;
}

// container_path_level (anchored overload)
//   function: the level of _id measured from _anchor, which is at level 0.
// Returns 0 when _id does not reach _anchor; container_path_is_ancestor_or_self
// tells that case apart from _id BEING the anchor.
template<typename _Policy,
         typename _Container,
         typename _Index>
std::size_t
container_path_level
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _id,
    _Index            _anchor
)
{
    return path_level(_policy, _container, _id, _anchor);
}


// container_path_depth
//   function: the retained spelling of container_path_level.  It counts parent
// links upward, which is the LEVEL; the spec reserves `depth` for a node's
// height.  Kept so existing callers continue to compile; prefer level().
template<typename _Policy,
         typename _Container,
         typename _Index>
std::size_t
container_path_depth
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _id
)
{
    return container_path_level(_policy, _container, _id);
}


// ================================================================
//  container_path_address / container_path_collect
// ================================================================

// container_path_address
//   function: the ADDRESS of _id relative to _anchor -- the word of labels
// along
// path(_id), root-first.
//
//   THE ANCHOR CONTRIBUTES NO LABEL.  Addressing starts from the anchor; the
// anchor is not a step taken.  So the returned word has exactly level(_id)
// entries, which is exactly what container_path_resolve consumes, and
//
//       resolve(anchor, address(anchor, n)) == n
//
// for every n in the anchor's subtree, given a separating labelling.  An empty
// result means either _id == _anchor (whose address is the empty word) or _id
// does not reach _anchor at all; container_path_is_ancestor_or_self
// distinguishes
// them.
template<typename _Policy,
         typename _Container,
         typename _Index>
std::vector<typename _Policy::component_type>
container_path_address
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _anchor,
    _Index            _id
)
{
    using component_type = typename _Policy::component_type;

    std::vector<component_type> word;
    _Index                      current = _id;

    // walk up to the anchor, taking a label at every step EXCEPT the anchor's
    while ( (!_policy.is_null(current)) &&
            (!(current == _anchor)) )
    {
        word.push_back(_policy.component(_container, current));

        current = _policy.parent(_container, current);
    }

    // the anchor was never met: _id is not in its subtree, so it has no address
    if (_policy.is_null(current))
    {
        word.clear();

        return word;
    }

    // reverse to root-first order
    std::size_t lo = 0;
    std::size_t hi = word.size();

    while (lo < hi)
    {
        --hi;

        component_type tmp = word[lo];
        word[lo]           = word[hi];
        word[hi]           = tmp;

        ++lo;
    }

    return word;
}


// container_path_collect
//   function: the ABSOLUTE address of _id -- its label word from the root of
// its
// own tree.  The compatibility spelling of container_path_address.
//
//   BEHAVIOUR CHANGE.  This used to include the ROOT's own component, returning
// level(_id) + 1 labels where resolve consumes level(_id).  It no longer does:
// the root contributes no label, so collect(root) is now the EMPTY word and
// collect(n) has exactly level(n) entries.  container_path_build, which is
// built
// on this, is corrected by the same stroke -- it no longer prefixes the root's
// name to every path it renders.
template<typename _Policy,
         typename _Container,
         typename _Index>
std::vector<typename _Policy::component_type>
container_path_collect
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _id
)
{
    return container_path_address(
        _policy,
        _container,
        container_path_root(_policy, _container, _id),
        _id);
}

// container_path_collect (anchored overload)
//   function: the address of _id relative to _anchor.  A spelling of
// container_path_address, for symmetry with the three-argument form.
template<typename _Policy,
         typename _Container,
         typename _Index>
std::vector<typename _Policy::component_type>
container_path_collect
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _anchor,
    _Index            _id
)
{
    return container_path_address(_policy, _container, _anchor, _id);
}


// ================================================================
//  container_path_ancestors
// ================================================================

// container_path_ancestors
//   function: all ancestors of _id (NOT including _id), from the immediate
// parent up to the root.
//
//   These are the components named by the PROPER PREFIXES of addr(_id), and
// there are exactly level(_id) of them -- one per prefix.  The walk is the path
// iterator; the vector is only for callers that need to keep the chain.  A
// caller that merely wants to WALK it should use make_path_view and allocate
// nothing.
template<typename _Policy,
         typename _Container,
         typename _Index>
std::vector<_Index>
container_path_ancestors
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _id
)
{
    std::vector<_Index> result;
    _Index              current = _policy.parent(_container, _id);

    // walk from the parent to the root
    while (!_policy.is_null(current))
    {
        result.push_back(current);

        current = _policy.parent(_container, current);
    }

    return result;
}

// container_path_ancestor_chain
//   function: path(_id) itself -- the chain of components from the root down to
// _id, root-first, _id last.  It names level(_id) + 1 components: one more than
// the address has labels, the extra one being the root.
template<typename _Policy,
         typename _Container,
         typename _Index>
std::vector<_Index>
container_path_ancestor_chain
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _id
)
{
    std::vector<_Index> chain;
    _Index              current = _id;

    // collect from _id up to the root
    while (!_policy.is_null(current))
    {
        chain.push_back(current);

        current = _policy.parent(_container, current);
    }

    // reverse to root-first order
    std::size_t lo = 0;
    std::size_t hi = chain.size();

    while (lo < hi)
    {
        --hi;

        _Index tmp = chain[lo];
        chain[lo]  = chain[hi];
        chain[hi]  = tmp;

        ++lo;
    }

    return chain;
}


// ================================================================
//  container_path_lca
// ================================================================

// container_path_lca
//   function: the lowest common ancestor of _a and _b -- which the spec reads
// as
// the MEET of their addresses in the prefix order,
//
//       lca(a, b) = resolve( addr(a) /\ addr(b) ),
//
// the longest common prefix of the two words.  In a single-rooted container the
// meet ALWAYS exists (at worst it is the empty word, the root), so this cannot
// fail; it returns the null sentinel only for a forest, where the two nodes lie
// in different trees and their addresses have no common prefix at all.
template<typename _Policy,
         typename _Container,
         typename _Index>
_Index
container_path_lca
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _a,
    _Index            _b
)
{
    std::size_t la = container_path_level(_policy, _container, _a);
    std::size_t lb = container_path_level(_policy, _container, _b);

    _Index ca = _a;
    _Index cb = _b;

    // equalise levels: a common prefix cannot be longer than the shorter word
    while (la > lb)
    {
        ca = _policy.parent(_container, ca);

        --la;
    }

    while (lb > la)
    {
        cb = _policy.parent(_container, cb);

        --lb;
    }

    // climb in tandem until the two chains meet
    while (!(ca == cb))
    {
        if ( (_policy.is_null(ca)) ||
             (_policy.is_null(cb)) )
        {
            return _policy.null_index();
        }

        ca = _policy.parent(_container, ca);
        cb = _policy.parent(_container, cb);
    }

    return ca;
}


// ================================================================
//  container_path_relative
// ================================================================

// container_path_relative
//   function: the relative address of _to as seen from _from -- the (k, v) pair
// of the spec.  k is the number of ascents from _from to the meet of the two
// addresses; v is the label word descending from the meet to _to.
//
//   The result is marked INVALID when the two nodes have no meet -- when they
// lie in different trees.  Without that flag the caller could not tell such a
// pair apart from _from == _to, which also yields (0, <>), and those are not
// the
// same answer.
template<typename _Policy,
         typename _Container,
         typename _Index>
path_address<typename _Policy::component_type>
container_path_relative
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _from,
    _Index            _to
)
{
    using component_type = typename _Policy::component_type;

    path_address<component_type> result;

    _Index meet = container_path_lca(_policy, _container, _from, _to);

    // no meet: the two nodes are not addressable from one another
    if (_policy.is_null(meet))
    {
        return result;
    }

    result.valid = true;

    // k: the ascents from _from up to the meet
    _Index current = _from;

    while (!(current == meet))
    {
        ++result.up_count;

        current = _policy.parent(_container, current);
    }

    // v: the labels descending from the meet to _to.  The MEET contributes no
    // label, exactly as a root does not -- it is where this address starts.
    result.components =
        container_path_address(_policy, _container, meet, _to);

    return result;
}


// ================================================================
//  container_path_is_ancestor
// ================================================================

// container_path_is_ancestor_or_self
//   function: true if _ancestor lies on the parent chain of _descendant,
// _itself
// included.  This is the (non-strict) prefix order: addr(_ancestor) is a prefix
// of addr(_descendant).
template<typename _Policy,
         typename _Container,
         typename _Index>
bool
container_path_is_ancestor_or_self
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _ancestor,
    _Index            _descendant
)
{
    return path_reaches(_policy, _container, _descendant, _ancestor);
}


// container_path_is_ancestor
//   function: true if _ancestor is a STRICT ancestor of _descendant -- a PROPER
// prefix of its address.  A node is not its own ancestor.
template<typename _Policy,
         typename _Container,
         typename _Index>
bool
container_path_is_ancestor
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _ancestor,
    _Index            _descendant
)
{
    if (_ancestor == _descendant)
    {
        return false;
    }

    return container_path_is_ancestor_or_self(
        _policy,
        _container,
        _ancestor,
        _descendant);
}


// ================================================================
//  Separation
// ================================================================

// container_path_is_separating
//   function: true if the children of _node bear pairwise distinct components
// --
// the SEPARATION condition, at one node.
//
//   Separation is the multiplicity restriction mu_1 read sibling-wise.  Under
// it
// a node's children are a MAP from label to child; without it a label names a
// SET, and resolve cannot invert addr.  O(k^2) in the branching factor.
template<typename _Policy,
         typename _Container,
         typename _Index>
bool
container_path_is_separating
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _node
)
{
    return tree_children_separating(_policy, _container, _node);
}


// container_path_is_separating_subtree
//   function: true if EVERY node of the subtree rooted at _root separates --
// the
// condition under which the whole subtree is ADDRESSABLE, and under which the
// round trip below is guaranteed.
template<typename _Policy,
         typename _Container,
         typename _Index>
bool
container_path_is_separating_subtree
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _root
)
{
    tree_view<_Policy, _Container, tree_order::pre> nodes(
        _policy, _container, _root);

    // every node of the subtree must separate its own children
    for (tree_iterator<_Policy, _Container, tree_order::pre> it = nodes.begin();
         it != nodes.end();
         ++it)
    {
        if (!tree_children_separating(_policy, _container, *it))
        {
            return false;
        }
    }

    return true;
}


// container_path_round_trips
//   function: the PROPOSITION of the Addressability axis, made executable --
//
//       resolve(root, address(root, n)) == n     for every n in the subtree,
//
// together with |address(root, n)| == level(n).  It holds if and only if the
// subtree separates, so a failure here is a failure of separation and nothing
// else.  O(n * k) over the subtree; a debug or test-time check, not a hot path.
template<typename _Policy,
         typename _Container,
         typename _Index>
bool
container_path_round_trips
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _root
)
{
    tree_view<_Policy, _Container, tree_order::pre> nodes(
        _policy, _container, _root);

    for (tree_iterator<_Policy, _Container, tree_order::pre> it = nodes.begin();
         it != nodes.end();
         ++it)
    {
        std::vector<typename _Policy::component_type> word =
            container_path_address(_policy, _container, _root, *it);

        // the address of a node has exactly as many labels as its level
        if (word.size() != it.level())
        {
            return false;
        }

        // and resolving it must land back on the node it names
        if (!(container_path_resolve(_policy, _container, _root, word) == *it))
        {
            return false;
        }
    }

    return true;
}


// ================================================================
//  String Convenience
// ================================================================
//
// The following are convenience wrappers for policies whose component_type is
// component_view.  They render an address into a separated string and parse one
// back -- which is, in the spec's reading, the RENDER FOLD at the text target,
// restricted to addresses.
//
// FAITHFULNESS.  A textual address re-parses to the address it came from if,
// and
// only if, the label rendering is injective AND NO LABEL CONTAINS THE
// SEPARATOR.
// A label that does contain it is silently torn into two on the way back, and
// the round trip fails.  container_path_build_is_faithful decides this; nothing
// here escapes on your behalf, because the right escape depends on the format.


// container_path_resolve (string overload)
//   function: splits a path string into labels and resolves it.  path_split
//   drops leading and repeated separators and emits NO label for the root,
// which
//   is exactly the convention container_path_address renders to.
template<typename _Policy,
         typename _Container,
         typename _Index>
_Index
container_path_resolve
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _root,
    const char*       _path,
    std::size_t       _path_len
)
{
    _Index current = _root;

    std::vector<path_component> splits =
        djinterp::path_split(_path, _path_len);

    // fold the descent step along the parsed labels
    for (std::size_t i = 0; i < splits.size(); ++i)
    {
        if (_policy.is_null(current))
        {
            return _policy.null_index();
        }

        component_view label;
        label.data   = _path + splits[i].offset;
        label.length = splits[i].length;

        current = tree_find_child(_policy, _container, current, label);
    }

    return current;
}

// container_path_resolve (std::string overload)
//   function: resolves a path given as std::string.
template<typename _Policy,
         typename _Container,
         typename _Index>
_Index
container_path_resolve
(
    const _Policy&     _policy,
    const _Container&  _container,
    _Index             _root,
    const std::string& _path
)
{
    return container_path_resolve(
        _policy,
        _container,
        _root,
        _path.c_str(),
        _path.size());
}


// container_path_build_from
//   function: renders the address of _id relative to _anchor as a string,
// labels joined by _sep.  The ANCHOR contributes no segment, so the result has
// exactly level(_id) segments and parses straight back through
// container_path_resolve.
template<typename _Policy,
         typename _Container,
         typename _Index>
std::string
container_path_build_from
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _anchor,
    _Index            _id,
    char              _sep = djinterp::path_separator
)
{
    std::vector<component_view> word =
        container_path_address(_policy, _container, _anchor, _id);

    std::string result;

    // join the labels with the separator
    for (std::size_t i = 0; i < word.size(); ++i)
    {
        if (i > 0)
        {
            result += _sep;
        }

        result.append(word[i].data, word[i].length);
    }

    return result;
}


// container_path_build
//   function: renders the ABSOLUTE address of _id -- from the root of its own
// tree -- as a separated string.
//
//   BEHAVIOUR CHANGE.  This used to prefix the ROOT's own name to every path it
// built ("root/a/b"), because container_path_collect included it.  It no longer
// does: the root contributes no segment, so build(n) now yields "a/b" and
// resolve(root, build(n)) == n, which was the whole point.
template<typename _Policy,
         typename _Container,
         typename _Index>
std::string
container_path_build
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _id,
    char              _sep = djinterp::path_separator
)
{
    return container_path_build_from(
        _policy,
        _container,
        container_path_root(_policy, _container, _id),
        _id,
        _sep);
}


// container_path_build_is_faithful
//   function: true if the address of _id can be rendered with _sep and parsed
// back unchanged -- that is, if NO label on its path contains the separator.
//
//   Where this is false, container_path_build still produces a string, but
// container_path_resolve will tear the offending label in two and land
// somewhere
// else, or nowhere.  The rendering is then lossy, and the format must escape
// the
// separator or choose another.
template<typename _Policy,
         typename _Container,
         typename _Index>
bool
container_path_build_is_faithful
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _id,
    char              _sep = djinterp::path_separator
)
{
    std::vector<component_view> word = container_path_collect(
        _policy,
        _container,
        _id);

    for (std::size_t i = 0; i < word.size(); ++i)
    {
        // an empty label renders to nothing and is dropped by the parse
        if (word[i].length == 0)
        {
            return false;
        }

        for (std::size_t c = 0; c < word[i].length; ++c)
        {
            if ( (word[i].data[c] == _sep) ||
                 (djinterp::path_is_separator(word[i].data[c])) )
            {
                return false;
            }
        }
    }

    return true;
}


// container_path_relative_string
//   function: renders the relative address from _from to _to as a string with
// ".." segments.
//
//   Returns "." for the same node -- the (0, <>) address -- and the EMPTY
// string
// when no relative address exists at all, the two nodes lying in different
// trees.  Those are different answers and are now spelled differently.
template<typename _Policy,
         typename _Container,
         typename _Index>
std::string
container_path_relative_string
(
    const _Policy&    _policy,
    const _Container& _container,
    _Index            _from,
    _Index            _to,
    char              _sep = djinterp::path_separator
)
{
    path_address<component_view> addr = container_path_relative(
        _policy,
        _container,
        _from,
        _to);

    // no meet: there is no relative address to render
    if (!addr.valid)
    {
        return std::string();
    }

    std::string result;

    // one ".." per ascent to the meet
    for (std::size_t i = 0; i < addr.up_count; ++i)
    {
        if (!result.empty())
        {
            result += _sep;
        }

        result += "..";
    }

    // then the labels descending from the meet
    for (std::size_t i = 0; i < addr.components.size(); ++i)
    {
        if (!result.empty())
        {
            result += _sep;
        }

        result.append(addr.components[i].data, addr.components[i].length);
    }

    // the same node: no ascent and no descent
    if (result.empty())
    {
        result = ".";
    }

    return result;
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_PATH_
