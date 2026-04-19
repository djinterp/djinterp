/******************************************************************************
* djinterp [container]                                        tree_path.hpp
*
* Path operations for arena-backed trees:
*   This header provides the arena_path_policy — a container_path
* accessor policy for arena<_Payload> — and convenience wrappers that
* eliminate the need to specify the policy at every call site.
*
*   The policy is parameterized on _Payload and a _ComponentAccessor
* callable that extracts a typed component from the payload.  This
* keeps the arena completely generic: it never assumes payloads
* contain a name field or that components are strings.  The caller
* provides the accessor at construction time, and the policy carries
* it as an instance member through all container_path operations.
*
* Contents:
*   - arena_path_policy      accessor policy for container_path
*   - tree_path              convenience class wrapping arena + policy
*   - make_tree_path         factory with deduced template types
*
*
* path:      /inc/cpp/container/tree_path.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_PATH_
#define DJINTERP_CONTAINER_TREE_PATH_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../djinterp.hpp"
#include "../core/path.hpp"
#include "./arena.hpp"
#include "./container_path.hpp"


NS_DJINTERP
NS_CONTAINER


// ================================================================
//  arena_path_policy
// ================================================================

// arena_path_policy
//   class: accessor policy satisfying the container_path_policy
// concept for arena<_Payload>.
//
// _ComponentAccessor must be a callable with signature:
//   _ComponentType (const arena<_Payload>&, node_id)
//
// The returned component is compared via operator== during
// resolve operations.  For string-based trees, _ComponentType
// is typically component_view; for other trees it may be an
// integer, bitset, or any equality-comparable type.
//
// The component accessor is stored as an instance member,
// making the policy thread-safe and self-contained.
template<typename _Payload,
         typename _ComponentAccessor,
         typename _ComponentType>
class arena_path_policy
{
public:
    using container_type = arena<_Payload>;
    using index_type     = node_id;
    using component_type = _ComponentType;

    // --------------------------------------------------------
    //  construction
    // --------------------------------------------------------

    // arena_path_policy
    //   constructs a policy bound to a component accessor.
    explicit arena_path_policy(
            _ComponentAccessor _accessor
        )
            : m_accessor(_accessor)
        {}

    // --------------------------------------------------------
    //  policy interface
    // --------------------------------------------------------

    // null_index
    //   returns the null sentinel value.
    index_type
    null_index() const
    {
        return null_node;
    }

    // is_null
    //   returns true if _id is the null sentinel.
    bool
    is_null
    (
        index_type _id
    ) const
    {
        return (_id == null_node);
    }

    // parent
    //   returns the parent index of _id.
    index_type
    parent
    (
        const container_type& _arena,
        index_type            _id
    ) const
    {
        return _arena[_id].parent;
    }

    // first_child
    //   returns the first child index of _id.
    index_type
    first_child
    (
        const container_type& _arena,
        index_type            _id
    ) const
    {
        return _arena[_id].first_child;
    }

    // next_sibling
    //   returns the next sibling index of _id.
    index_type
    next_sibling
    (
        const container_type& _arena,
        index_type            _id
    ) const
    {
        return _arena[_id].next_sibling;
    }

    // component
    //   returns the component of the element at _id via the
    // bound accessor.
    component_type
    component
    (
        const container_type& _arena,
        index_type            _id
    ) const
    {
        return m_accessor(_arena, _id);
    }

private:
    _ComponentAccessor m_accessor;
};


// ================================================================
//  tree_path
// ================================================================

// tree_path
//   class: convenience wrapper that binds an arena to a
// component accessor and provides path operations without
// requiring the caller to specify the policy or pass it
// manually.
//
// Usage (string components):
//   auto comp_fn = [&pool](
//       const arena<my_payload>& a,
//       node_id id) -> component_view
//   {
//       return {pool.data() + a[id].data.name_offset,
//               a[id].data.name_length};
//   };
//
//   tree_path<my_payload, decltype(comp_fn), component_view>
//       tp(my_arena, comp_fn);
//   node_id n = tp.resolve(root, "src/core/main.cpp");
//   std::string p = tp.build(n);
//
// Usage (integer components):
//   auto key_fn = [](
//       const arena<radix_node>& a,
//       node_id id) -> uint8_t
//   {
//       return a[id].data.key;
//   };
//
//   tree_path<radix_node, decltype(key_fn), uint8_t>
//       tp(my_arena, key_fn);
//   uint8_t keys[] = {0x01, 0x0A, 0xFF};
//   node_id n = tp.resolve(root, keys, 3);
//
template<typename _Payload,
         typename _ComponentAccessor,
         typename _ComponentType>
class tree_path
{
public:
    using policy_type    = arena_path_policy<
        _Payload, _ComponentAccessor, _ComponentType>;
    using container_type = arena<_Payload>;
    using component_type = _ComponentType;

    // --------------------------------------------------------
    //  construction
    // --------------------------------------------------------

    // tree_path
    //   constructs a tree_path bound to an arena and a
    // component accessor.
    tree_path(
            const container_type& _arena,
            _ComponentAccessor    _accessor
        )
            : m_arena(_arena),
              m_policy(_accessor)
        {}

    // --------------------------------------------------------
    //  resolve (generic)
    // --------------------------------------------------------

    // resolve
    //   resolves a component sequence from _root.
    template<typename _Iter>
    node_id
    resolve
    (
        node_id _root,
        _Iter   _begin,
        _Iter   _end
    ) const
    {
        return container_path_resolve(
            m_policy,
            m_arena,
            _root,
            _begin,
            _end);
    }

    // resolve (pointer + count overload)
    //   resolves from a contiguous array of components.
    template<typename _Component>
    node_id
    resolve
    (
        node_id           _root,
        const _Component* _components,
        std::size_t       _count
    ) const
    {
        return container_path_resolve(
            m_policy,
            m_arena,
            _root,
            _components,
            _count);
    }

    // resolve (vector overload)
    //   resolves from a vector of components.
    template<typename _Component>
    node_id
    resolve
    (
        node_id                        _root,
        const std::vector<_Component>& _components
    ) const
    {
        return container_path_resolve(
            m_policy,
            m_arena,
            _root,
            _components);
    }

    // --------------------------------------------------------
    //  resolve (string convenience)
    // --------------------------------------------------------

    // resolve
    //   resolves a string path from _root.  Requires
    // component_type to be comparable with component_view.
    node_id
    resolve
    (
        node_id     _root,
        const char* _path,
        std::size_t _path_len
    ) const
    {
        return container_path_resolve(
            m_policy,
            m_arena,
            _root,
            _path,
            _path_len);
    }

    // resolve (std::string overload)
    //   function: resolves a path given as std::string.
    node_id
    resolve
    (
        node_id            _root,
        const std::string& _path
    ) const
    {
        return resolve(_root, _path.c_str(), _path.size());
    }

    // --------------------------------------------------------
    //  collect
    // --------------------------------------------------------

    // collect
    //   returns the component sequence from root to _id.
    std::vector<component_type>
    collect
    (
        node_id _id
    ) const
    {
        return container_path_collect(
            m_policy,
            m_arena,
            _id);
    }

    // --------------------------------------------------------
    //  build (string convenience)
    // --------------------------------------------------------

    // build
    //   constructs the full path string from root to _id.
    // Requires component_type to be component_view.
    std::string
    build
    (
        node_id _id,
        char    _sep = djinterp::path::path_separator
    ) const
    {
        return container_path_build(
            m_policy,
            m_arena,
            _id,
            _sep);
    }

    // --------------------------------------------------------
    //  depth
    // --------------------------------------------------------

    // depth
    //   returns the depth of _id (root = 0).
    std::size_t
    depth
    (
        node_id _id
    ) const
    {
        return container_path_depth(
            m_policy,
            m_arena,
            _id);
    }

    // --------------------------------------------------------
    //  ancestors
    // --------------------------------------------------------

    // ancestors
    //   returns all ancestors of _id (parent first, root last).
    std::vector<node_id>
    ancestors
    (
        node_id _id
    ) const
    {
        return container_path_ancestors(
            m_policy,
            m_arena,
            _id);
    }

    // ancestor_chain
    //   returns the full chain from root to _id (root first,
    // _id last).
    std::vector<node_id>
    ancestor_chain
    (
        node_id _id
    ) const
    {
        return container_path_ancestor_chain(
            m_policy,
            m_arena,
            _id);
    }

    // --------------------------------------------------------
    //  lca
    // --------------------------------------------------------

    // lca
    //   computes the lowest common ancestor of _a and _b.
    node_id
    lca
    (
        node_id _a,
        node_id _b
    ) const
    {
        return container_path_lca(
            m_policy,
            m_arena,
            _a,
            _b);
    }

    // --------------------------------------------------------
    //  relative
    // --------------------------------------------------------

    // relative
    //   computes the relative path from _from to _to as a
    // path_address.
    path_address<component_type>
    relative
    (
        node_id _from,
        node_id _to
    ) const
    {
        return container_path_relative(
            m_policy,
            m_arena,
            _from,
            _to);
    }

    // relative_string
    //   computes the relative path as a string with ".."
    // segments.  Requires component_type to be component_view.
    std::string
    relative_string
    (
        node_id _from,
        node_id _to,
        char    _sep = djinterp::path::path_separator
    ) const
    {
        return container_path_relative_string(
            m_policy,
            m_arena,
            _from,
            _to,
            _sep);
    }

    // --------------------------------------------------------
    //  is_ancestor
    // --------------------------------------------------------

    // is_ancestor
    //   returns true if _ancestor is an ancestor of _descendant.
    bool
    is_ancestor
    (
        node_id _ancestor,
        node_id _descendant
    ) const
    {
        return container_path_is_ancestor(
            m_policy,
            m_arena,
            _ancestor,
            _descendant);
    }

    // --------------------------------------------------------
    //  accessors
    // --------------------------------------------------------

    // arena_ref
    //   returns a reference to the underlying arena.
    const container_type&
    arena_ref() const
    {
        return m_arena;
    }

    // policy
    //   returns a reference to the policy object.
    const policy_type&
    policy() const
    {
        return m_policy;
    }

private:
    const container_type& m_arena;
    policy_type           m_policy;
};


// ================================================================
//  make_tree_path
// ================================================================

// make_tree_path
//   factory: creates a tree_path with deduced template types.
// _ComponentType must be explicitly specified as the first
// template argument since it cannot be deduced.
//
// Usage:
//   auto tp = make_tree_path<component_view>(
//       my_arena, my_accessor);
template<typename _ComponentType,
         typename _Payload,
         typename _ComponentAccessor>
tree_path<_Payload, _ComponentAccessor, _ComponentType>
make_tree_path
(
    const arena<_Payload>& _arena,
    _ComponentAccessor     _accessor
)
{
    return tree_path<_Payload, _ComponentAccessor, _ComponentType>(
        _arena,
        _accessor);
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TREE_PATH_
