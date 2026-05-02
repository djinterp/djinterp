/******************************************************************************
* djinterp [fs]                                          file_tree_path.hpp
*
* Path operations for file_tree:
*   This header provides the concrete instantiation of the tree_path
* module for file_tree.  It binds the component accessor to file_tree's
* internal string pool, so callers never need to think about the
* policy layer — just construct a file_tree_path from a file_tree
* and call resolve(), build(), lca(), relative(), etc.
*
*   Additionally this header provides path-based utilities that are
* specific to filesystem semantics: canonical path construction,
* extension-aware operations, platform separator normalization,
* and bulk path queries (all_paths, resolve_many).
*
* Contents:
*   - file_tree_path_policy   path policy for file_tree
*   - file_tree_path          convenience class
*   - free functions          resolve, build, lca, etc.
*
* Usage:
*   file_tree ft;
*   ft.scan("/project");
*
*   file_tree_path ftp(ft);
*
*   node_id n = ftp.resolve("src/core/main.cpp");
*   std::string p = ftp.build(n);
*   std::string rel = ftp.relative_string(a, b);
*   node_id ancestor = ftp.lca(a, b);
*
*
* path:      /inc/cpp/fs/file_tree_path.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TREE_PATH_
#define DJINTERP_FS_FILE_TREE_PATH_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../../../djinterp.hpp"
#include "../core/path.hpp"
#include "../../arena/arena.hpp"
#include "../../container_path.hpp"
#include "../tree_path.hpp"
#include "./file_tree.hpp"


NS_DJINTERP
NS_FS


// bring in path utilities.
using djinterp::path::path_separator;
using djinterp::path::path_is_separator;
using djinterp::path::path_normalize;
using djinterp::path::path_extension;
using djinterp::path::path_stem;
using djinterp::path::path_filename;
using djinterp::path::path_parent;
using djinterp::path::path_is_absolute;
using djinterp::path::path_common_prefix;
using djinterp::path::path_relative_to;
using djinterp::path::path_to_posix;
using djinterp::path::path_to_windows;
using djinterp::path::path_depth;

using djinterp::container::node_id;
using djinterp::container::null_node;
using djinterp::container::arena;
using djinterp::container::component_view;
using djinterp::container::path_address;
using djinterp::container::container_path_resolve;
using djinterp::container::container_path_collect;
using djinterp::container::container_path_build;
using djinterp::container::container_path_depth;
using djinterp::container::container_path_lca;
using djinterp::container::container_path_relative;
using djinterp::container::container_path_relative_string;
using djinterp::container::container_path_ancestors;
using djinterp::container::container_path_ancestor_chain;
using djinterp::container::container_path_is_ancestor;


// ================================================================
//  file_tree_path_policy
// ================================================================

// file_tree_path_policy
//   class: a concrete container_path policy for file_tree's
// arena.  Reads element names directly from the file_tree's
// string pool via a bound pointer stored as an instance member.
// Returns component_view values for efficient zero-copy
// comparison during path resolution.
class file_tree_path_policy
{
public:
    using container_type = arena<file_entry>;
    using index_type     = node_id;
    using component_type = component_view;

    // --------------------------------------------------------
    //  construction
    // --------------------------------------------------------

    // file_tree_path_policy
    //   constructs a policy bound to a string pool.
    explicit file_tree_path_policy(
            const std::string& _names
        )
            : m_names(&_names)
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
    //   returns a component_view of the element at _id from
    // the bound string pool.
    component_type
    component
    (
        const container_type& _arena,
        index_type            _id
    ) const
    {
        const file_entry& e = _arena[_id].data;

        return component_view{
            m_names->data() + e.name_offset,
            e.name_length
        };
    }

private:
    const std::string* m_names;
};


// ================================================================
//  file_tree_path
// ================================================================

// file_tree_path
//   class: provides path operations over a file_tree.
// Wraps the container_path free functions with the file_tree-
// specific policy, constructed once and passed through all
// operations.
class file_tree_path
{
public:
    using policy_type    = file_tree_path_policy;
    using component_type = component_view;

    // --------------------------------------------------------
    //  construction
    // --------------------------------------------------------

    // file_tree_path
    //   constructs a path helper bound to _tree.
    explicit file_tree_path(
            const file_tree& _tree
        )
            : m_tree(_tree),
              m_policy(_tree.name_pool())
        {}

    // --------------------------------------------------------
    //  resolve
    // --------------------------------------------------------

    // resolve
    //   walks a path string from _root through the tree.
    // Both '/' and '\\' are accepted as separators.
    // Returns null_node if any component is not found.
    node_id
    resolve
    (
        const char* _path,
        node_id     _root = 0
    ) const
    {
        if (m_tree.empty())
        {
            return null_node;
        }

        return container_path_resolve(
            m_policy,
            m_tree.nodes(),
            _root,
            _path,
            std::strlen(_path));
    }

    // resolve (std::string overload)
    //   function: resolves a path given as std::string.
    node_id
    resolve
    (
        const std::string& _path,
        node_id            _root = 0
    ) const
    {
        return resolve(_path.c_str(), _root);
    }

    // resolve_many
    //   resolves multiple paths and returns the results.
    // Each entry is null_node if not found.
    std::vector<node_id>
    resolve_many
    (
        const std::vector<std::string>& _paths,
        node_id                         _root = 0
    ) const
    {
        std::vector<node_id> result;
        result.reserve(_paths.size());

        // resolve each path individually.
        for (const auto& p : _paths)
        {
            result.push_back(resolve(p, _root));
        }

        return result;
    }


    // --------------------------------------------------------
    //  collect
    // --------------------------------------------------------

    // collect
    //   returns the component_view sequence from root to _id.
    std::vector<component_type>
    collect
    (
        node_id _id
    ) const
    {
        return container_path_collect(
            m_policy,
            m_tree.nodes(),
            _id);
    }


    // --------------------------------------------------------
    //  build
    // --------------------------------------------------------

    // build
    //   constructs the full path from root to _id.
    std::string
    build
    (
        node_id _id,
        char    _sep = '/'
    ) const
    {
        return container_path_build(
            m_policy,
            m_tree.nodes(),
            _id,
            _sep);
    }

    // build_normalized
    //   constructs and normalizes the path from root to _id.
    std::string
    build_normalized
    (
        node_id _id,
        char    _sep = '/'
    ) const
    {
        return path_normalize(build(_id, _sep),
                              _sep);
    }

    // build_posix
    //   constructs the path with POSIX separators.
    std::string
    build_posix
    (
        node_id _id
    ) const
    {
        return build(_id, '/');
    }

    // build_windows
    //   constructs the path with Windows separators.
    std::string
    build_windows
    (
        node_id _id
    ) const
    {
        return build(_id, '\\');
    }

    // build_platform
    //   constructs the path with platform-native separators.
    std::string
    build_platform
    (
        node_id _id
    ) const
    {
        return build(_id, path_separator);
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
            m_tree.nodes(),
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
            m_tree.nodes(),
            _id);
    }

    // ancestor_chain
    //   returns the full chain from root to _id (root first).
    std::vector<node_id>
    ancestor_chain
    (
        node_id _id
    ) const
    {
        return container_path_ancestor_chain(
            m_policy,
            m_tree.nodes(),
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
            m_tree.nodes(),
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
            m_tree.nodes(),
            _from,
            _to);
    }

    // relative_string
    //   computes the relative path as a string with ".."
    // segments.
    std::string
    relative_string
    (
        node_id _from,
        node_id _to,
        char    _sep = '/'
    ) const
    {
        return container_path_relative_string(
            m_policy,
            m_tree.nodes(),
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
            m_tree.nodes(),
            _ancestor,
            _descendant);
    }

    // is_descendant
    //   returns true if _descendant is a descendant of _ancestor.
    bool
    is_descendant
    (
        node_id _descendant,
        node_id _ancestor
    ) const
    {
        return is_ancestor(_ancestor, _descendant);
    }


    // --------------------------------------------------------
    //  path queries
    // --------------------------------------------------------

    // all_paths
    //   returns the full path of every node in the tree (BFS
    // order from _root).
    std::vector<std::string>
    all_paths
    (
        node_id _root = 0,
        char    _sep  = '/'
    ) const
    {
        std::vector<std::string> result;

        if (m_tree.empty())
        {
            return result;
        }

        m_tree.visit_breadth_first(_root,
            [&](node_id _id, std::size_t)
            {
                result.push_back(build(_id, _sep));
            });

        return result;
    }

    // extension
    //   returns the extension of the node at _id.
    std::string
    extension
    (
        node_id _id
    ) const
    {
        return path_extension(m_tree.name_str(_id));
    }

    // stem
    //   returns the stem (filename without extension) of the
    // node at _id.
    std::string
    stem
    (
        node_id _id
    ) const
    {
        return path_stem(m_tree.name_str(_id));
    }

    // common_ancestor_path
    //   returns the common prefix path of two nodes.
    std::string
    common_ancestor_path
    (
        node_id _a,
        node_id _b,
        char    _sep = '/'
    ) const
    {
        node_id ancestor = lca(_a, _b);

        if (ancestor == null_node)
        {
            return std::string();
        }

        return build(ancestor, _sep);
    }

    // nodes_at_depth
    //   returns all node_ids at a specific depth from _root.
    std::vector<node_id>
    nodes_at_depth
    (
        std::size_t _depth,
        node_id     _root = 0
    ) const
    {
        std::vector<node_id> result;

        if (m_tree.empty())
        {
            return result;
        }

        m_tree.visit_breadth_first(_root,
            [&](node_id _id, std::size_t _d)
            {
                if (_d == _depth)
                {
                    result.push_back(_id);
                }
            });

        return result;
    }

    // siblings
    //   returns all siblings of _id (excluding _id itself).
    std::vector<node_id>
    siblings
    (
        node_id _id
    ) const
    {
        std::vector<node_id> result;

        node_id par = m_tree[_id].parent;

        if (par == null_node)
        {
            return result;
        }

        node_id c = m_tree[par].first_child;

        // walk the sibling chain, collecting all except _id.
        while (c != null_node)
        {
            if (c != _id)
            {
                result.push_back(c);
            }

            c = m_tree[c].next_sibling;
        }

        return result;
    }


    // --------------------------------------------------------
    //  accessors
    // --------------------------------------------------------

    // tree
    //   returns a reference to the underlying file_tree.
    const file_tree&
    tree() const
    {
        return m_tree;
    }

    // policy
    //   returns a reference to the policy object.
    const policy_type&
    policy() const
    {
        return m_policy;
    }


private:
    const file_tree& m_tree;
    policy_type      m_policy;
};


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_TREE_PATH_
