/*******************************************************************************
* djinterp [core]                                             file_tree_path.hpp
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
*   file_node_id n = ftp.resolve("src/core/main.cpp");
*   std::string p = ftp.build(n);
*   std::string rel = ftp.relative_string(a, b);
*   file_node_id ancestor = ftp.lca(a, b);
*
*
* path:      /inc/djinterp/core/container/tree/file/file_tree_path.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.22
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_PATH_HPP
#define DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_PATH_HPP 1

// FLOOR, FOR NOW: below C++20 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// std
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"
#include "../../../paradigm/path/path.hpp"
#include "../../container_path.hpp"
#include "./file_tree.hpp"


NS_DJINTERP

// NOTE. This header used to open NS_FS. There IS NO NS_FS -- nothing in the
// framework defines that macro. Flat djinterp, like everything else in fs.


// DEPENDENCIES.
//   path.hpp and container_path.hpp both open plain NS_DJINTERP -- there is no
// djinterp::path and no djinterp::container namespace holding them. The
// twenty-eight using-declarations that stood here named nothing, and this
// header could not compile. Being in djinterp ourselves, the names are already
// in scope. What we lean on: path.hpp path_separator, path_is_separator,
// path_normalize, path_extension, path_stem, path_filename, path_parent,
// path_is_absolute, path_relative_to, path_to_posix, path_to_windows,
// path_meet (was path_common_prefix -- a meet is what it computes; the old
// spelling still resolves), path_level (was path_depth -- it counts UPWARD,
// which is the level; depth is a node's height) container_path.hpp
// component_view,
// path_address, container_path_resolve, container_path_collect,
// container_path_build, container_path_level, container_path_lca,
// container_path_relative, container_path_relative_string,
// container_path_ancestors, container_path_ancestor_chain,
// container_path_is_ancestor arena.hpp file_node_id, null_file_node, arena (these ARE
// nested, in djinterp::container, and are imported below)

// file_node_id / null_file_node / file_node come from file_tree_common.hpp now. The
// arena is GONE -- nodes live in a djinterp pool and a file_node_id is a POINTER,
// not an index -- so there is no arena<file_entry> to name and nothing to
// import from djinterp::container.


// ================================================================
//  file_tree_path_policy
// ================================================================

// file_tree_path_policy
//   class: a concrete container_path policy for file_tree's arena. Reads
// element names directly from the file_tree's string pool via a bound pointer
// stored as an instance member. Returns component_view values for efficient
// zero-copy comparison during path resolution. TEMPLATED ON THE TREE. The
// policy's "container" used to be the arena; there
// is no arena now, and a file_node_id dereferences directly, so the container is
// the TREE and the link accessors just follow pointers.
template<typename Tree>
class file_tree_path_policy
{
public:
    using container_type = Tree;
    using index_type     = file_node_id;
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
        return null_file_node;
    }

    // is_null
    //   returns true if _id is the null sentinel.
    bool
    is_null
    (
        index_type _id
    ) const
    {
        return (_id == null_file_node);
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
        return _id->parent;
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
        return _id->first_child;
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
        return _id->next_sibling;
    }

    // component
    //   returns a component_view of the element at _id from the bound string
    // pool.
    component_type
    component
    (
        const container_type& _arena,
        index_type            _id
    ) const
    {
        const file_entry& e = _id->data;

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
//   class: provides path operations over a file_tree. Wraps the container_path
// free functions with the file_tree- specific policy, constructed once and
// passed through all operations.
//   TEMPLATED ON THE TREE. It used to say `const file_tree&` -- but file_tree
// is a template ALIAS (file_tree.hpp maps an operating_system onto a scanner
// policy), and naming an alias template with no argument list is ill-formed.
// Every one of these three sites was a hard error; the header simply was never
// instantiated, and a header that is never instantiated is never checked.
// Templating on the tree also means an option-carrying tree works unchanged.
template<typename Tree = file_tree_default>
class file_tree_path
{
public:
    using tree_type      = Tree;
    using policy_type    = file_tree_path_policy<Tree>;
    using component_type = component_view;

    // --------------------------------------------------------
    //  construction
    // --------------------------------------------------------

    // file_tree_path
    //   constructs a path helper bound to _tree.
    explicit file_tree_path(
            const tree_type& _tree
        )
            : m_tree(_tree),
              m_policy(_tree.name_pool())
        {}

    // --------------------------------------------------------
    //  resolve
    // --------------------------------------------------------

    // resolve
    //   walks a path string from _root through the tree. Both '/' and '\\' are
    // accepted as separators. Returns null_file_node if any component is not found.
    file_node_id
    resolve
    (
        const char* _path,
        file_node_id     _root = 0
    ) const
    {
        if (m_tree.empty())
        {
            return null_file_node;
        }

        return container_path_resolve(
            m_policy,
            m_tree,
            _root,
            _path,
            std::strlen(_path));
    }

    // resolve (std::string overload)
    //   function: resolves a path given as std::string.
    file_node_id
    resolve
    (
        const std::string& _path,
        file_node_id            _root = 0
    ) const
    {
        return resolve(_path.c_str(), _root);
    }

    // resolve_many
    //   resolves multiple paths and returns the results. Each entry is
    // null_file_node if not found.
    std::vector<file_node_id>
    resolve_many
    (
        const std::vector<std::string>& _paths,
        file_node_id                         _root = 0
    ) const
    {
        std::vector<file_node_id> result;
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
        file_node_id _id
    ) const
    {
        return container_path_collect(
            m_policy,
            m_tree,
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
        file_node_id _id,
        char    _sep = '/'
    ) const
    {
        return container_path_build(
            m_policy,
            m_tree,
            _id,
            _sep);
    }

    // build_normalized
    //   constructs and normalizes the path from root to _id.
    std::string
    build_normalized
    (
        file_node_id _id,
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
        file_node_id _id
    ) const
    {
        return build(_id, '/');
    }

    // build_windows
    //   constructs the path with Windows separators.
    std::string
    build_windows
    (
        file_node_id _id
    ) const
    {
        return build(_id, '\\');
    }

    // build_platform
    //   constructs the path with platform-native separators.
    std::string
    build_platform
    (
        file_node_id _id
    ) const
    {
        return build(_id, path_separator);
    }


    // --------------------------------------------------------
    //  level
    // --------------------------------------------------------

    // level
    //   the LEVEL (lambda) of _id: parent links up to the root, which sits at
    // level 0. This is the length of _id's ADDRESS. It was spelled `depth`,
    // but the spec reserves DEPTH for a node's HEIGHT -- the distance DOWN to
    // its deepest leaf -- and the two are different numbers on the same node.
    // depth() below is kept, and delegates, so no caller breaks.
    std::size_t
    level
    (
        file_node_id _id
    ) const
    {
        return container_path_level(
            m_policy,
            m_tree,
            _id);
    }

    // depth
    //   the retained spelling of level(). It counts parent links UPWARD, which
    // is the level; prefer level(), and see height() for what the spec means
    // by depth. Kept so existing callers continue to compile.
    std::size_t
    depth
    (
        file_node_id _id
    ) const
    {
        return level(_id);
    }

    // height
    //   the HEIGHT of _id: the longest descent BELOW it. This is what the spec
    // calls the node's DEPTH, and the facade had no way to ask for it -- the
    // one measure it named was the other one.
    std::size_t
    height
    (
        file_node_id _id
    ) const
    {
        return m_tree.height(_id);
    }


    // --------------------------------------------------------
    //  ancestors
    // --------------------------------------------------------

    // ancestors
    //   returns all ancestors of _id (parent first, root last).
    std::vector<file_node_id>
    ancestors
    (
        file_node_id _id
    ) const
    {
        return container_path_ancestors(
            m_policy,
            m_tree,
            _id);
    }

    // ancestor_chain
    //   returns the full chain from root to _id (root first).
    std::vector<file_node_id>
    ancestor_chain
    (
        file_node_id _id
    ) const
    {
        return container_path_ancestor_chain(
            m_policy,
            m_tree,
            _id);
    }


    // --------------------------------------------------------
    //  lca
    // --------------------------------------------------------

    // lca
    //   computes the lowest common ancestor of _a and _b.
    file_node_id
    lca
    (
        file_node_id _a,
        file_node_id _b
    ) const
    {
        return container_path_lca(
            m_policy,
            m_tree,
            _a,
            _b);
    }


    // --------------------------------------------------------
    //  relative
    // --------------------------------------------------------

    // relative
    //   computes the relative path from _from to _to as a path_address.
    path_address<component_type>
    relative
    (
        file_node_id _from,
        file_node_id _to
    ) const
    {
        return container_path_relative(
            m_policy,
            m_tree,
            _from,
            _to);
    }

    // relative_string
    //   computes the relative path as a string with ".." segments.
    std::string
    relative_string
    (
        file_node_id _from,
        file_node_id _to,
        char    _sep = '/'
    ) const
    {
        return container_path_relative_string(
            m_policy,
            m_tree,
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
        file_node_id _ancestor,
        file_node_id _descendant
    ) const
    {
        return container_path_is_ancestor(
            m_policy,
            m_tree,
            _ancestor,
            _descendant);
    }

    // is_descendant
    //   returns true if _descendant is a descendant of _ancestor.
    bool
    is_descendant
    (
        file_node_id _descendant,
        file_node_id _ancestor
    ) const
    {
        return is_ancestor(_ancestor, _descendant);
    }


    // --------------------------------------------------------
    //  path queries
    // --------------------------------------------------------

    // all_paths
    //   returns the full path of every node in the tree (BFS order from
    // _root).
    std::vector<std::string>
    all_paths
    (
        file_node_id _root = 0,
        char    _sep  = '/'
    ) const
    {
        std::vector<std::string> result;

        if (m_tree.empty())
        {
            return result;
        }

        m_tree.visit_breadth_first(_root,
            [&](file_node_id _id, std::size_t)
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
        file_node_id _id
    ) const
    {
        return path_extension(m_tree.name_str(_id));
    }

    // stem
    //   returns the stem (filename without extension) of the node at _id.
    std::string
    stem
    (
        file_node_id _id
    ) const
    {
        return path_stem(m_tree.name_str(_id));
    }

    // common_ancestor_path
    //   returns the common prefix path of two nodes.
    std::string
    common_ancestor_path
    (
        file_node_id _a,
        file_node_id _b,
        char    _sep = '/'
    ) const
    {
        file_node_id ancestor = lca(_a, _b);

        if (ancestor == null_file_node)
        {
            return std::string();
        }

        return build(ancestor, _sep);
    }

    // nodes_at_depth
    //   returns all node_ids at a specific depth from _root.
    std::vector<file_node_id>
    nodes_at_depth
    (
        std::size_t _depth,
        file_node_id     _root = 0
    ) const
    {
        std::vector<file_node_id> result;

        if (m_tree.empty())
        {
            return result;
        }

        m_tree.visit_breadth_first(_root,
            [&](file_node_id _id, std::size_t _d)
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
    std::vector<file_node_id>
    siblings
    (
        file_node_id _id
    ) const
    {
        std::vector<file_node_id> result;

        file_node_id par = m_tree[_id].parent;

        if (par == null_file_node)
        {
            return result;
        }

        file_node_id c = m_tree[par].first_child;

        // walk the sibling chain, collecting all except _id.
        while (c != null_file_node)
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
    const tree_type&
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
    const tree_type& m_tree;
    policy_type      m_policy;
};


// make_file_tree_path
//   factory: deduces the tree type. Class-template argument deduction only
// arrived in C++17, so pre-C++17 callers need this to avoid spelling the
// backend out.
template<typename Tree>
file_tree_path<Tree>
make_file_tree_path(
    const Tree& _tree
)
{
    return file_tree_path<Tree>(_tree);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_PATH_HPP
