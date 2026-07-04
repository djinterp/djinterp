/******************************************************************************
* djinterp [container]                                      container_path.hpp
*
* Path resolution over indexed containers:
*   This header provides templated path traversal and construction for
* any container that supports indexed access to elements which have a
* hierarchical (parent/child) relationship.  The container itself need
* not be a tree - it could be a flat array, a vector, or an arena -
* as long as an accessor policy can extract parent, first_child,
* next_sibling, and component from each element.
*
*   Paths are sequences of typed components.  A component can be any
* type that supports equality comparison: a string view, an integer,
* a bitset, a key - anything.  The policy defines the component type
* and how to extract it from a node.
*
*   The accessor policy is an object with const member functions.
* All functions accept the policy as their first parameter; template
* arguments are fully deducible from the call site.
*
*   The header is organized in two layers:
*
*   Generic layer - works with any component type via operator==.
*     resolve, collect, depth, ancestors, ancestor_chain, lca,
*     relative, is_ancestor.
*
*   String layer - convenience for component_view-based policies.
*     resolve (const char*), build, relative_string.
*
* Contents:
*   Types:
*     - component_view          non-owning view for string components
*     - path_address            relative path as up_count + components
*
*   Generic operations:
*     - container_path_resolve  resolve a component sequence to a node
*     - container_path_collect  collect the component chain root-->node
*     - container_path_depth    compute depth by walking parents
*     - container_path_lca      lowest common ancestor
*     - container_path_ancestors        parent-->root chain
*     - container_path_ancestor_chain   root-->node chain
*     - container_path_relative         relative path between nodes
*     - container_path_is_ancestor      ancestry test
*
*   String convenience:
*     - container_path_resolve  (const char* overload)
*     - container_path_build    construct a string path with separators
*     - container_path_relative_string  relative path as string
*
*
* path:      /inc/djinterp/container/container_path.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.02
******************************************************************************/

#ifndef DJINTERP_CONTAINER_PATH_
#define DJINTERP_CONTAINER_PATH_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "../djinterp.hpp"
#include "../util/path/path.hpp"


NS_DJINTERP


// ================================================================
//  container_path_policy concept
// ================================================================
//
// A valid policy object _policy for container _Container, index
// type _Index, and component type _Component must provide these
// type aliases:
//
//   container_type  - the container being navigated
//   index_type      - the element address type
//   component_type  - the path component type
//
// and these const member functions:
//
//   index_type      null_index  ()                            const
//   bool            is_null     (index_type)                  const
//   index_type      parent      (const container_type&,
//                                index_type)                  const
//   index_type      first_child (const container_type&,
//                                index_type)                  const
//   index_type      next_sibling(const container_type&,
//                                index_type)                  const
//   component_type  component   (const container_type&,
//                                index_type)                  const
//
// component_type must support operator== for comparison against
// the input component sequence during resolve operations.


// ================================================================
//  component_view
// ================================================================

// component_view
//   struct: a lightweight, non-owning view into a character
// buffer.  Used as the component_type for string-based path
// policies where elements are named by substrings of a string
// pool.  Supports equality comparison via memcmp.
struct component_view
{
    const char* data;
    std::size_t length;

    // operator==
    //   compares two views for byte-wise equality.
    bool
    operator==
    (
        const component_view& _other
    ) const
    {
        return ( (length == _other.length) &&
                 (std::memcmp(
                      data,
                      _other.data,
                      length) == 0) );
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
//   struct: represents a relative path between two nodes as
// a number of upward steps from the source followed by a
// downward component sequence to the destination.
//
//   up_count = 0, components empty   --> same node
//   up_count = 2, components = {e,f} --> ../../e/f
//
// This representation is component-type-agnostic: it works
// with string paths, integer keys, bit patterns, etc.
template<typename _Component>
struct path_address
{
    std::size_t              up_count;
    std::vector<_Component>  components;
};


// ================================================================
//  container_path_resolve (generic)
// ================================================================

// container_path_resolve
//   function: walks a sequence of components through _container
// starting from _root, using _policy to navigate the hierarchy.
// Returns the index of the resolved element, or the null
// sentinel if any component is not found.
//
// _Iter must dereference to a type that is equality-comparable
// with _Policy::component_type.
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

    // resolve each component in sequence.
    for (_Iter it = _begin; it != _end; ++it)
    {
        if (_policy.is_null(current))
        {
            return current;
        }

        // walk children of current, matching by component.
        _Index child = _policy.first_child(_container, current);
        bool   found = false;

        while (!_policy.is_null(child))
        {
            if (_policy.component(_container, child) == *it)
            {
                current = child;
                found   = true;
                break;
            }

            child = _policy.next_sibling(_container, child);
        }

        if (!found)
        {
            return _policy.null_index();
        }
    }

    return current;
}

// container_path_resolve (pointer + count overload)
//   function: resolves from a contiguous array of components.
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
//   function: resolves from a vector of components.
template<typename _Policy,
         typename _Container,
         typename _Index,
         typename _Component>
_Index
container_path_resolve
(
    const _Policy&                _policy,
    const _Container&             _container,
    _Index                        _root,
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
//  container_path_collect
// ================================================================

// container_path_collect
//   function: collects the component sequence from root down to
// _id by walking the parent chain and reversing.  Returns an
// empty vector if _id is null.
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
    std::vector<typename _Policy::component_type> chain;
    _Index current = _id;

    // collect components from leaf to root.
    while (!_policy.is_null(current))
    {
        chain.push_back(
            _policy.component(_container, current));
        current = _policy.parent(_container, current);
    }

    // reverse to root-first order.
    std::size_t lo = 0;
    std::size_t hi = chain.size();

    while (lo < hi)
    {
        --hi;

        auto tmp   = chain[lo];
        chain[lo]  = chain[hi];
        chain[hi]  = tmp;

        ++lo;
    }

    return chain;
}


// ================================================================
//  container_path_depth
// ================================================================

// container_path_depth
//   function: computes the depth of _id by counting parent
// links.  Root depth is 0.
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
    std::size_t d = 0;
    _Index current = _policy.parent(_container, _id);

    // count parent links to root.
    while (!_policy.is_null(current))
    {
        ++d;
        current = _policy.parent(_container, current);
    }

    return d;
}


// ================================================================
//  container_path_ancestors
// ================================================================

// container_path_ancestors
//   function: collects all ancestors of _id (not including _id)
// from immediate parent to root.
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
    _Index current = _policy.parent(_container, _id);

    // walk from parent to root.
    while (!_policy.is_null(current))
    {
        result.push_back(current);
        current = _policy.parent(_container, current);
    }

    return result;
}

// container_path_ancestor_chain
//   function: collects ancestors from root down to _id
// (including _id).  The returned vector is root-first,
// _id-last.
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
    _Index current = _id;

    // collect ancestors from _id up to root.
    while (!_policy.is_null(current))
    {
        chain.push_back(current);
        current = _policy.parent(_container, current);
    }

    // reverse to root-first order.
    std::size_t lo = 0;
    std::size_t hi = chain.size();

    while (lo < hi)
    {
        --hi;

        _Index tmp  = chain[lo];
        chain[lo]   = chain[hi];
        chain[hi]   = tmp;

        ++lo;
    }

    return chain;
}


// ================================================================
//  container_path_lca
// ================================================================

// container_path_lca
//   function: computes the lowest common ancestor of _a and _b.
// Returns the null sentinel if they share no common ancestor
// (which should not happen in a well-formed single-root tree).
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
    // equalize depths.
    std::size_t da = container_path_depth(
        _policy,
        _container,
        _a);
    std::size_t db = container_path_depth(
        _policy,
        _container,
        _b);

    _Index ca = _a;
    _Index cb = _b;

    // walk _a up to match _b's depth.
    while (da > db)
    {
        ca = _policy.parent(_container, ca);
        --da;
    }

    // walk _b up to match _a's depth.
    while (db > da)
    {
        cb = _policy.parent(_container, cb);
        --db;
    }

    // walk up in tandem until they meet.
    while (ca != cb)
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
//   function: computes the relative path from _from to _to as
// a path_address: an up_count (steps from _from to LCA) and a
// component sequence (LCA down to _to).  Returns a zero
// path_address if the LCA is null.
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
    using component_type =
        typename _Policy::component_type;

    path_address<component_type> result;
    result.up_count = 0;

    _Index lca = container_path_lca(
        _policy,
        _container,
        _from,
        _to);

    if (_policy.is_null(lca))
    {
        return result;
    }

    // count steps up from _from to lca.
    _Index current = _from;

    while (current != lca)
    {
        ++result.up_count;
        current = _policy.parent(_container, current);
    }

    // collect components from lca down to _to.
    std::vector<component_type> down;
    current = _to;

    while (current != lca)
    {
        down.push_back(
            _policy.component(_container, current));
        current = _policy.parent(_container, current);
    }

    // reverse to lca-first order.
    for (std::size_t i = down.size(); i > 0; --i)
    {
        result.components.push_back(down[i - 1]);
    }

    return result;
}


// ================================================================
//  container_path_is_ancestor
// ================================================================

// container_path_is_ancestor
//   function: returns true if _ancestor is an ancestor of
// _descendant.
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
    _Index current = _policy.parent(_container, _descendant);

    // walk up from _descendant looking for _ancestor.
    while (!_policy.is_null(current))
    {
        if (current == _ancestor)
        {
            return true;
        }

        current = _policy.parent(_container, current);
    }

    return false;
}


// ================================================================
//  String Convenience
// ================================================================
//
// The following functions are convenience wrappers for policies
// whose component_type is component_view.  They handle string
// splitting, joining with separators, and ".." relative path
// construction.
//
// If the policy's component_type is not comparable to
// component_view, these overloads will produce a compile error
// at the call site.


// container_path_resolve (string overload)
//   function: splits a path string into component_view segments
// and resolves through the container.  Requires that the
// policy's component_type is equality-comparable with
// component_view.
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

    auto splits = djinterp::path_split(
        _path, _path_len);

    // resolve each split component.
    for (const auto& s : splits)
    {
        if (_policy.is_null(current))
        {
            return current;
        }

        component_view input{_path + s.offset, s.length};

        // walk children of current, matching by component.
        _Index child = _policy.first_child(_container, current);
        bool   found = false;

        while (!_policy.is_null(child))
        {
            if (_policy.component(_container, child) == input)
            {
                current = child;
                found   = true;
                break;
            }

            child = _policy.next_sibling(_container, child);
        }

        if (!found)
        {
            return _policy.null_index();
        }
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


// container_path_build
//   function: constructs a path string from root to _id by
// collecting component_views and joining with _sep.  Requires
// that the policy's component_type is component_view.
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
    auto chain = container_path_collect(
        _policy,
        _container,
        _id);

    std::string result;

    // join components with separator.
    for (const auto& comp : chain)
    {
        if (!result.empty())
        {
            result += _sep;
        }

        result.append(comp.data, comp.length);
    }

    return result;
}


// container_path_relative_string
//   function: computes the relative path from _from to _to as
// a string with ".." segments and separator _sep.  Requires
// that the policy's component_type is component_view.
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
    auto addr = container_path_relative(
        _policy,
        _container,
        _from,
        _to);

    std::string result;

    // prepend ".." for each step up.
    for (std::size_t i = 0; i < addr.up_count; ++i)
    {
        if (!result.empty())
        {
            result += _sep;
        }

        result += "..";
    }

    // append downward components.
    for (const auto& comp : addr.components)
    {
        if (!result.empty())
        {
            result += _sep;
        }

        result.append(comp.data, comp.length);
    }

    if (result.empty())
    {
        result = ".";
    }

    return result;
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_PATH_