/*******************************************************************************
* djinterp [core]                                           file_tree_filter.hpp
*
* Filterable file tree with predicate combinator integration:
*   This header bridges file_tree and the functional module, providing
* arena-aware predicate factories and a pipeline-compatible query
* interface.  Predicates operate on (const file_tree&, file_node_id) pairs,
* capturing the tree reference so they compose freely with
* predicate_and, predicate_or, predicate_not, and the variadic
* all_of / any_of / none_of combinators.
*
*   The query class (file_tree_query) produces function_pipeline<file_node_id>
* results, enabling full pipeline chaining (map, take, skip, fold,
* group_by, etc.) on the filtered node set.
*
* Contents:
*   - file_node_predicate     typedef for the predicate signature
*   - predicate factories     by_name, by_ext, by_type, by_size, etc.
*   - glob matching           by_glob with *, ?, ** support
*   - file_tree_query         pipeline-producing query interface
*   - collect / visit helpers  direct traversal with predicates
*
* Usage:
*   file_tree ft;
*   ft.scan("/project");
*
*   // simple: find all .cpp files
*   auto cpp = file_tree_query(ft)
*       .where(by_ext(".cpp"))
*       .to_vector();
*
*   // complex: large .hpp files not in build/
*   auto pred = predicate_and(
*       by_ext(".hpp"),
*       predicate_and(
*           by_size_gt(10000),
*           predicate_not(by_path_contains("build"))));
*
*   auto result = file_tree_query(ft)
*       .where(pred)
*       .sorted([&](file_node_id a, file_node_id b) {
*           return ft[a].data.size > ft[b].data.size;
*       })
*       .to_vector();
*
*   // variadic: .cpp or .hpp or .h, not hidden
*   auto headers = predicate_and(
*       any_of(by_ext(".cpp"), by_ext(".hpp"), by_ext(".h")),
*       predicate_not(by_hidden()));
*
*
* path:      /inc/djinterp/core/container/tree/file/file_tree_filter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.22
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_FILTER_HPP
#define DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_FILTER_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>   // std::sort      -- used by sorted(); was not included
#include <cstddef>
#include <cstring>
#include <functional>
#include <map>         // std::map       -- used by group_by(); was not included
#include <string>
#include <utility>     // std::pair      -- used by partition(); was missing
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"
#include "./file_tree.hpp"
#include "./file_attributes.hpp"
#include "../../../functional/predicate.hpp"   // predicate_and / all_of / ...
#include "../../../functional/pipeline.hpp"    // function_pipeline<T>
// re_std
#include "../../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint64_t


NS_DJINTERP

// NOTE. This header used to open NS_FS. There IS NO NS_FS -- nothing in the
// framework defines that macro, so the name appeared bare in the source and
// the file could not compile. The core, the umbrella, the traits, the concepts
// and all six scanners are in flat djinterp; so is this now.

// bring in the types we depend on. file_node_id / null_file_node come from
// file_tree_common.hpp (via file_tree.hpp). The arena is gone: a file_node_id is a
// POINTER into the pool, not an index. Every bool(file_node_id) predicate below is
// unaffected -- the handle changed type, not role -- and _tree[id].data still
// dereferences it. The functional module is FLAT in djinterp -- there is no
// djinterp::functional namespace, and the eager pipeline is
// function_pipeline<T>, not d_pipeline<T>. These names are therefore already
// in scope; the ten using-declarations that stood here named nothing at all.
// Depended upon:
//
//     function_pipeline<T> pipeline.hpp (source factory: ::from)
//     predicate_and / _or / _not /
//     _xor / _nand / _nor predicate.hpp
//     all_of / any_of / none_of predicate.hpp


// ================================================================
//  file_node_predicate
// ================================================================

// file_node_predicate
//   typedef: the canonical predicate signature for file tree filtering.
// Predicates are evaluated against a file_node_id with the file_tree captured by
// reference inside the callable.
using file_node_predicate = std::function<bool(file_node_id)>;


// ================================================================
//  predicate factories — name matching
// ================================================================

// by_name
//   factory: matches nodes whose name equals _name exactly.
template<typename Tree>
inline file_node_predicate
by_name
(
    const       Tree& _tree,
    const char* _name
)
{
    std::string target(_name);

    return [&_tree, target](file_node_id _id) -> bool
    {
        return (_tree.name_str(_id) == target);
    };
}

// by_name (std::string overload)
template<typename Tree>
inline file_node_predicate
by_name
(
    const              Tree& _tree,
    const std::string& _name
)
{
    return by_name(_tree, _name.c_str());
}

// by_name_contains
//   factory: matches nodes whose name contains _substr.
template<typename Tree>
inline file_node_predicate
by_name_contains
(
    const              Tree& _tree,
    const std::string& _substr
)
{
    return [&_tree, _substr](file_node_id _id) -> bool
    {
        return (_tree.name_str(_id).find(_substr)
                != std::string::npos);
    };
}

// by_name_prefix
//   factory: matches nodes whose name starts with _prefix.
template<typename Tree>
inline file_node_predicate
by_name_prefix
(
    const              Tree& _tree,
    const std::string& _prefix
)
{
    return [&_tree, _prefix](file_node_id _id) -> bool
    {
        std::string n = _tree.name_str(_id);

        if (n.size() < _prefix.size())
        {
            return false;
        }

        return (n.compare(0, _prefix.size(), _prefix) == 0);
    };
}

// by_name_suffix
//   factory: matches nodes whose name ends with _suffix.
template<typename Tree>
inline file_node_predicate
by_name_suffix
(
    const              Tree& _tree,
    const std::string& _suffix
)
{
    return [&_tree, _suffix](file_node_id _id) -> bool
    {
        std::string n = _tree.name_str(_id);

        if (n.size() < _suffix.size())
        {
            return false;
        }

        return (n.compare(
            n.size() - _suffix.size(),
            _suffix.size(),
            _suffix) == 0);
    };
}


// ================================================================
//  predicate factories — extension
// ================================================================

// by_ext
//   factory: matches nodes whose name ends with _ext. _ext should include the
// dot (e.g. ".cpp").
template<typename Tree>
inline file_node_predicate
by_ext
(
    const              Tree& _tree,
    const std::string& _ext
)
{
    return by_name_suffix(_tree, _ext);
}

// by_any_ext
//   factory: matches nodes whose name ends with any of the given extensions.
template<typename Tree>
inline file_node_predicate
by_any_ext
(
    const                    Tree& _tree,
    std::vector<std::string> _exts
)
{
    return [&_tree, _exts](file_node_id _id) -> bool
    {
        std::string n = _tree.name_str(_id);

        for (const auto& ext : _exts)
        {
            if (n.size() >= ext.size() &&
                n.compare(
                    n.size() - ext.size(),
                    ext.size(),
                    ext) == 0)
            {
                return true;
            }
        }

        return false;
    };
}


// ================================================================
//  predicate factories — type
// ================================================================

// by_type
//   factory: matches nodes of the given file_type.
template<typename Tree>
inline file_node_predicate
by_type
(
    const     Tree& _tree,
    file_type _type
)
{
    return [&_tree, _type](file_node_id _id) -> bool
    {
        return (_tree[_id].data.type == _type);
    };
}

// by_directory
//   factory: matches directory nodes.
template<typename Tree>
inline file_node_predicate
by_directory
(
    const Tree& _tree
)
{
    return by_type(_tree, file_type_directory);
}

// by_regular
//   factory: matches regular file nodes.
template<typename Tree>
inline file_node_predicate
by_regular
(
    const Tree& _tree
)
{
    return by_type(_tree, file_type_regular);
}

// by_symlink
//   factory: matches symlink nodes.
template<typename Tree>
inline file_node_predicate
by_symlink
(
    const Tree& _tree
)
{
    return by_type(_tree, file_type_symlink);
}


// ================================================================
//  predicate factories — size
// ================================================================

// by_size_gt
//   factory: matches nodes whose size is greater than _threshold.
template<typename Tree>
inline file_node_predicate
by_size_gt
(
    const         Tree& _tree,
    re_std::uint64_t _threshold
)
{
    return [&_tree, _threshold](file_node_id _id) -> bool
    {
        return (_tree[_id].data.size > _threshold);
    };
}

// by_size_lt
//   factory: matches nodes whose size is less than _threshold.
template<typename Tree>
inline file_node_predicate
by_size_lt
(
    const         Tree& _tree,
    re_std::uint64_t _threshold
)
{
    return [&_tree, _threshold](file_node_id _id) -> bool
    {
        return (_tree[_id].data.size < _threshold);
    };
}

// by_size_between
//   factory: matches nodes whose size is in [_min, _max].
template<typename Tree>
inline file_node_predicate
by_size_between
(
    const         Tree& _tree,
    re_std::uint64_t _min,
    re_std::uint64_t _max
)
{
    return [&_tree, _min, _max](file_node_id _id) -> bool
    {
        re_std::uint64_t sz = _tree[_id].data.size;

        return (sz >= _min && sz <= _max);
    };
}

// by_empty
//   factory: matches nodes with size == 0.
template<typename Tree>
inline file_node_predicate
by_empty
(
    const Tree& _tree
)
{
    return [&_tree](file_node_id _id) -> bool
    {
        return (_tree[_id].data.size == 0);
    };
}


// ================================================================
//  predicate factories — depth
// ================================================================

// by_depth_eq
//   factory: matches nodes at exactly _depth levels from root.
template<typename Tree>
inline file_node_predicate
by_depth_eq
(
    const       Tree& _tree,
    std::size_t _depth
)
{
    return [&_tree, _depth](file_node_id _id) -> bool
    {
        std::size_t d = 0;
        file_node_id current = _tree[_id].parent;

        while (current != null_file_node)
        {
            ++d;
            current = _tree[current].parent;
        }

        return (d == _depth);
    };
}

// by_depth_le
//   factory: matches nodes at depth <= _max_depth.
template<typename Tree>
inline file_node_predicate
by_depth_le
(
    const       Tree& _tree,
    std::size_t _max_depth
)
{
    return [&_tree, _max_depth](file_node_id _id) -> bool
    {
        std::size_t d = 0;
        file_node_id current = _tree[_id].parent;

        while (current != null_file_node)
        {
            ++d;

            if (d > _max_depth)
            {
                return false;
            }

            current = _tree[current].parent;
        }

        return true;
    };
}

// by_max_depth
//   factory: alias for by_depth_le.
template<typename Tree>
inline file_node_predicate
by_max_depth
(
    const       Tree& _tree,
    std::size_t _max_depth
)
{
    return by_depth_le(_tree, _max_depth);
}


// ================================================================
//  predicate factories — path
// ================================================================

// ----------------------------------------------------------------
//  path predicates -- these match the ADDRESS, not the disk path
// ----------------------------------------------------------------
//   These three used to match against full_path(), and could not work: the old
// full_path() prepended THE ROOT'S OWN NAME, so every path began "project/..."
// and by_path_prefix("src") never matched anything.  (The core has since been
// fixed: address() is the tree-relative word of labels and round-trips through
// resolve(); full_path() is the openable path on disk, root_path() +
// address().) They now match the ADDRESS, which is what a caller means by "the
// path of this node in this tree" -- by_path_prefix("src/core") matches, and
// the answer does not depend on where the tree happens to have been scanned
// from. For the on-disk spelling, match full_path() instead; it is a different
// question.

// by_path_contains
//   factory: matches nodes whose full path contains _substr.
template<typename Tree>
inline file_node_predicate
by_path_contains
(
    const              Tree& _tree,
    const std::string& _substr
)
{
    return [&_tree, _substr](file_node_id _id) -> bool
    {
        return (_tree.address(_id).find(_substr)
                != std::string::npos);
    };
}

// by_path_prefix
//   factory: matches nodes whose full path starts with _prefix.
template<typename Tree>
inline file_node_predicate
by_path_prefix
(
    const              Tree& _tree,
    const std::string& _prefix
)
{
    return [&_tree, _prefix](file_node_id _id) -> bool
    {
        std::string p = _tree.address(_id);

        if (p.size() < _prefix.size())
        {
            return false;
        }

        return (p.compare(0, _prefix.size(), _prefix) == 0);
    };
}

// by_ancestor
//   factory: matches nodes that are descendants of _ancestor.
template<typename Tree>
inline file_node_predicate
by_ancestor
(
    const   Tree& _tree,
    file_node_id _ancestor
)
{
    return [&_tree, _ancestor](file_node_id _id) -> bool
    {
        file_node_id current = _tree[_id].parent;

        while (current != null_file_node)
        {
            if (current == _ancestor)
            {
                return true;
            }

            current = _tree[current].parent;
        }

        return false;
    };
}

// by_parent
//   factory: matches nodes that are immediate children of _parent_id.
template<typename Tree>
inline file_node_predicate
by_parent
(
    const   Tree& _tree,
    file_node_id _parent_id
)
{
    return [&_tree, _parent_id](file_node_id _id) -> bool
    {
        return (_tree[_id].parent == _parent_id);
    };
}


// ================================================================
//  predicate factories — hidden
// ================================================================

// by_hidden
//   factory: matches nodes whose name starts with '.' (POSIX hidden file
// convention).
template<typename Tree>
inline file_node_predicate
by_hidden
(
    const Tree& _tree
)
{
    return [&_tree](file_node_id _id) -> bool
    {
        std::size_t len = 0;
        const char* n   = _tree.name(_id, &len);

        return (len > 0 && n[0] == '.');
    };
}


// ================================================================
//  predicate factories — children
// ================================================================

// by_has_children
//   factory: matches nodes that have at least one child.
template<typename Tree>
inline file_node_predicate
by_has_children
(
    const Tree& _tree
)
{
    return [&_tree](file_node_id _id) -> bool
    {
        return _tree[_id].has_children();
    };
}

// by_is_leaf
//   factory: matches nodes with no children.
template<typename Tree>
inline file_node_predicate
by_is_leaf
(
    const Tree& _tree
)
{
    return [&_tree](file_node_id _id) -> bool
    {
        return !_tree[_id].has_children();
    };
}

// by_child_count_gt
//   factory: matches directories with more than _n children.
template<typename Tree>
inline file_node_predicate
by_child_count_gt
(
    const       Tree& _tree,
    std::size_t _n
)
{
    return [&_tree, _n](file_node_id _id) -> bool
    {
        std::size_t count = 0;
        file_node_id c = _tree[_id].first_child;

        while (c != null_file_node)
        {
            ++count;

            if (count > _n)
            {
                return true;
            }

            c = _tree[c].next_sibling;
        }

        return false;
    };
}


// ================================================================
//  predicate factories — glob matching
// ================================================================

NS_INTERNAL

    // glob_match
    //   helper: matches a string against a glob pattern. Supports: * (any
    // sequence), ? (any single char). Does not support ** or character
    // classes.
    inline bool
    glob_match
    (
        const char* _pattern,
        const char* _str
    )
    {
        while (*_pattern != '\0' && *_str != '\0')
        {
            if (*_pattern == '*')
            {
                ++_pattern;

                // trailing * matches everything.
                if (*_pattern == '\0')
                {
                    return true;
                }

                // try matching the rest at every position.
                while (*_str != '\0')
                {
                    if (glob_match(_pattern, _str))
                    {
                        return true;
                    }

                    ++_str;
                }

                return glob_match(_pattern, _str);
            }

            if (*_pattern == '?')
            {
                ++_pattern;
                ++_str;
                continue;
            }

            if (*_pattern != *_str)
            {
                return false;
            }

            ++_pattern;
            ++_str;
        }

        // consume trailing *'s.
        while (*_pattern == '*')
        {
            ++_pattern;
        }

        return (*_pattern == '\0' && *_str == '\0');
    }

    // glob_path_match
    //   helper: matches a path against a glob pattern with ** support. **
    // matches zero or more path components.
    inline bool
    glob_path_match
    (
        const char* _pattern,
        const char* _path
    )
    {
        // handle ** prefix: match any number of directories.
        if (_pattern[0] == '*' && _pattern[1] == '*')
        {
            const char* rest = _pattern + 2;

            // skip separator after **.
            if (*rest == '/')
            {
                ++rest;
            }

            // try matching rest against every suffix of path.
            const char* p = _path;

            while (*p != '\0')
            {
                if (glob_path_match(rest, p))
                {
                    return true;
                }

                // advance to next path component.
                while (*p != '\0' && *p != '/')
                {
                    ++p;
                }

                if (*p == '/')
                {
                    ++p;
                }
            }

            // also try matching rest against empty (** matches nothing).
            return glob_path_match(rest, p);
        }

        // extract current pattern component.
        const char* pat_end = _pattern;

        while (*pat_end != '\0' && *pat_end != '/')
        {
            ++pat_end;
        }

        // extract current path component.
        const char* path_end = _path;

        while (*path_end != '\0' && *path_end != '/')
        {
            ++path_end;
        }

        // create null-terminated copies for the component match.
        std::string pat_comp(_pattern,
            static_cast<std::size_t>(pat_end - _pattern));
        std::string path_comp(_path,
            static_cast<std::size_t>(path_end - _path));

        if (!glob_match(pat_comp.c_str(), path_comp.c_str()))
        {
            return false;
        }

        // both exhausted — match.
        if (*pat_end == '\0' && *path_end == '\0')
        {
            return true;
        }

        // one exhausted, the other not — no match.
        if (*pat_end == '\0' || *path_end == '\0')
        {
            return false;
        }

        // both have more components — recurse.
        return glob_path_match(pat_end + 1, path_end + 1);
    }

NS_END  // internal


// by_glob
//   factory: matches nodes whose name matches a glob pattern (* and ?
// wildcards).
template<typename Tree>
inline file_node_predicate
by_glob
(
    const              Tree& _tree,
    const std::string& _pattern
)
{
    return [&_tree, _pattern](file_node_id _id) -> bool
    {
        return internal::glob_match(
            _pattern.c_str(),
            _tree.name_str(_id).c_str());
    };
}

// by_path_glob
//   factory: matches nodes whose full path matches a glob pattern with **
// support for recursive directory matching.
template<typename Tree>
inline file_node_predicate
by_path_glob
(
    const              Tree& _tree,
    const std::string& _pattern
)
{
    return [&_tree, _pattern](file_node_id _id) -> bool
    {
        return internal::glob_path_match(
            _pattern.c_str(),
            _tree.address(_id).c_str());
    };
}


// ================================================================
//  predicate factories — custom
// ================================================================

// by_custom
//   factory: wraps a user-provided function that receives the tree and
// file_node_id. Useful for predicates that need to inspect multiple fields.
template<typename Tree>
inline file_node_predicate
by_custom
(
    const Tree&                              _tree,
    std::function<bool(const Tree&, file_node_id)> _fn
)
{
    return [&_tree, _fn](file_node_id _id) -> bool
    {
        return _fn(_tree, _id);
    };
}


// ================================================================
//  collect / visit helpers
// ================================================================

// collect_matching
//   gathers all node_ids in the tree (BFS from _root) that satisfy _predicate.
template<typename Tree>
inline std::vector<file_node_id>
collect_matching
(
    const Tree&               _tree,
    file_node_id                   _root,
    const file_node_predicate& _predicate
)
{
    std::vector<file_node_id> result;

    _tree.visit_breadth_first(_root,
        [&](file_node_id _id, std::size_t)
        {
            if (_predicate(_id))
            {
                result.push_back(_id);
            }
        });

    return result;
}

// collect_matching (entire tree)
//   overload that starts from node 0 (root).
template<typename Tree>
inline std::vector<file_node_id>
collect_matching
(
    const Tree&               _tree,
    const file_node_predicate& _predicate
)
{
    if (_tree.empty())
    {
        return std::vector<file_node_id>();
    }

    return collect_matching(_tree, 0, _predicate);
}

// count_matching
//   counts nodes satisfying _predicate without allocating a result vector.
template<typename Tree>
inline std::size_t
count_matching
(
    const Tree&               _tree,
    file_node_id                   _root,
    const file_node_predicate& _predicate
)
{
    std::size_t count = 0;

    _tree.visit_breadth_first(_root,
        [&](file_node_id _id, std::size_t)
        {
            if (_predicate(_id))
            {
                ++count;
            }
        });

    return count;
}

// first_matching
//   returns the first file_node_id satisfying _predicate (DFS), or null_file_node if
// none found.
template<typename Tree>
inline file_node_id
first_matching
(
    const Tree&               _tree,
    file_node_id                   _root,
    const file_node_predicate& _predicate
)
{
    // manual DFS with early exit — visit_depth_first doesn't support
    // short-circuiting.
    struct frame
    {
        file_node_id id;
    };

    std::vector<frame> stack;
    stack.push_back({ _root });

    while (!stack.empty())
    {
        frame f = stack.back();
        stack.pop_back();

        if (_predicate(f.id))
        {
            return f.id;
        }

        file_node_id c = _tree[f.id].last_child;

        while (c != null_file_node)
        {
            stack.push_back({ c });
            c = _tree[c].prev_sibling;
        }
    }

    return null_file_node;
}


// ================================================================
//  file_tree_query
// ================================================================

// file_tree_query
//   class: pipeline-producing query interface for file_tree.
// Wraps a file_tree reference and provides fluent filtering
// that returns function_pipeline<file_node_id> results compatible with the full
// functional module pipeline API.
//   TEMPLATED ON THE TREE. file_tree is a template ALIAS (file_tree.hpp maps
// an operating_system onto a scanner policy), so the bare `const file_tree&`
// this class used to hold was ill-formed -- an alias template named with no
// argument list. Every use site in this header was a hard error; the header
// was
// simply never instantiated, and an uninstantiated header template is never
// checked. Templating also lets an option-carrying tree through unchanged.
template<typename Tree = file_tree_default>
class file_tree_query
{
public:
    using tree_type = Tree;

    // --------------------------------------------------------
    //  construction
    // --------------------------------------------------------

    // file_tree_query
    //   constructs a query over the entire tree.
    explicit
    file_tree_query
    (
        const tree_type& _tree,
        file_node_id          _root = 0
    )
        : m_tree(_tree),
          m_root(_root),
          m_ids ()
    {
        if (!_tree.empty())
        {
            // seed with all node_ids via BFS.
            _tree.visit_breadth_first(_root,
                [this](file_node_id _id, std::size_t)
                {
                    m_ids.push_back(_id);
                });
        }
    }

    // file_tree_query (from existing id set)
    //   constructs a query from a pre-collected set of ids.
    file_tree_query
    (
        const tree_type&       _tree,
        std::vector<file_node_id>&& _ids
    )
        : m_tree(_tree),
          m_root(0),
          m_ids (std::move(_ids))
    {}


    // --------------------------------------------------------
    //  filtering
    // --------------------------------------------------------

    // where
    //   applies a predicate and returns a new query containing only the
    // matching nodes.
    file_tree_query
    where
    (
        const file_node_predicate& _predicate
    ) const
    {
        std::vector<file_node_id> result;
        result.reserve(m_ids.size());

        for (file_node_id id : m_ids)
        {
            if (_predicate(id))
            {
                result.push_back(id);
            }
        }

        return file_tree_query(m_tree, std::move(result));
    }

    // where_not
    //   applies a negated predicate.
    file_tree_query
    where_not
    (
        const file_node_predicate& _predicate
    ) const
    {
        std::vector<file_node_id> result;
        result.reserve(m_ids.size());

        for (file_node_id id : m_ids)
        {
            if (!_predicate(id))
            {
                result.push_back(id);
            }
        }

        return file_tree_query(m_tree, std::move(result));
    }

    // directories
    //   convenience: filter to directory nodes only.
    file_tree_query
    directories() const
    {
        return where(by_directory(m_tree));
    }

    // files
    //   convenience: filter to regular file nodes only.
    file_tree_query
    files() const
    {
        return where(by_regular(m_tree));
    }

    // not_hidden
    //   convenience: exclude hidden files.
    file_tree_query
    not_hidden() const
    {
        return where_not(by_hidden(m_tree));
    }


    // --------------------------------------------------------
    //  ordering
    // --------------------------------------------------------

    // sorted
    //   returns a new query with nodes sorted by _cmp.
    template<typename Compare>
    file_tree_query
    sorted
    (
        Compare _cmp
    ) const
    {
        std::vector<file_node_id> result(m_ids);

        std::sort(result.begin(), result.end(), _cmp);

        return file_tree_query(m_tree, std::move(result));
    }

    // sorted_by_name
    //   convenience: sort by node name ascending.
    file_tree_query
    sorted_by_name() const
    {
        return sorted([this](file_node_id a, file_node_id b) -> bool
        {
            return (m_tree.name_str(a) < m_tree.name_str(b));
        });
    }

    // sorted_by_size
    //   convenience: sort by file size descending.
    file_tree_query
    sorted_by_size() const
    {
        return sorted([this](file_node_id a, file_node_id b) -> bool
        {
            return (m_tree[a].data.size > m_tree[b].data.size);
        });
    }


    // --------------------------------------------------------
    //  limiting
    // --------------------------------------------------------

    // take
    //   returns a query containing at most _n nodes.
    file_tree_query
    take
    (
        std::size_t _n
    ) const
    {
        std::size_t count = (_n < m_ids.size())
                          ? _n : m_ids.size();

        std::vector<file_node_id> result(
            m_ids.begin(),
            m_ids.begin() + static_cast<
                std::vector<file_node_id>::difference_type>(count));

        return file_tree_query(m_tree, std::move(result));
    }

    // skip
    //   returns a query skipping the first _n nodes.
    file_tree_query
    skip
    (
        std::size_t _n
    ) const
    {
        if (_n >= m_ids.size())
        {
            return file_tree_query(
                m_tree, std::vector<file_node_id>());
        }

        std::vector<file_node_id> result(
            m_ids.begin() + static_cast<
                std::vector<file_node_id>::difference_type>(_n),
            m_ids.end());

        return file_tree_query(m_tree, std::move(result));
    }


    // --------------------------------------------------------
    //  terminal operations
    // --------------------------------------------------------

    // to_vector
    //   returns the node_ids as a vector.
    std::vector<file_node_id>
    to_vector() const
    {
        return m_ids;
    }

    // to_pipeline
    //   converts the query result to a function_pipeline<file_node_id> for full
    // functional module integration.
    function_pipeline<file_node_id>
    to_pipeline() const
    {
        return function_pipeline<file_node_id>::from(m_ids);
    }

    // to_names
    //   returns the names of all matching nodes.
    std::vector<std::string>
    to_names() const
    {
        std::vector<std::string> result;
        result.reserve(m_ids.size());

        for (file_node_id id : m_ids)
        {
            result.push_back(m_tree.name_str(id));
        }

        return result;
    }

    // to_paths
    //   returns the full paths of all matching nodes.
    std::vector<std::string>
    to_paths() const
    {
        std::vector<std::string> result;
        result.reserve(m_ids.size());

        for (file_node_id id : m_ids)
        {
            result.push_back(m_tree.full_path(id));
        }

        return result;
    }

    // for_each
    //   invokes _fn on each matching file_node_id.
    template<typename Fn>
    const file_tree_query&
    for_each
    (
        Fn _fn
    ) const
    {
        for (file_node_id id : m_ids)
        {
            _fn(id);
        }

        return *this;
    }

    // fold
    //   folds all matching node_ids with an accumulator.
    template<typename Acc, typename Fn>
    Acc
    fold
    (
        Acc _init,
        Fn   _fn
    ) const
    {
        for (file_node_id id : m_ids)
        {
            _init = _fn(static_cast<const Acc&>(_init), id);
        }

        return _init;
    }

    // any
    //   returns true if any node matches _predicate.
    bool
    any
    (
        const file_node_predicate& _predicate
    ) const
    {
        for (file_node_id id : m_ids)
        {
            if (_predicate(id))
            {
                return true;
            }
        }

        return false;
    }

    // all
    //   returns true if all nodes match _predicate.
    bool
    all
    (
        const file_node_predicate& _predicate
    ) const
    {
        for (file_node_id id : m_ids)
        {
            if (!_predicate(id))
            {
                return false;
            }
        }

        return true;
    }

    // count
    //   returns the number of matching nodes.
    std::size_t
    count() const
    {
        return m_ids.size();
    }

    // count (with predicate)
    //   returns the number of nodes satisfying _predicate.
    std::size_t
    count
    (
        const file_node_predicate& _predicate
    ) const
    {
        std::size_t n = 0;

        for (file_node_id id : m_ids)
        {
            if (_predicate(id))
            {
                ++n;
            }
        }

        return n;
    }

    // empty
    //   returns true if no nodes matched.
    bool
    empty() const
    {
        return m_ids.empty();
    }

    // first
    //   returns the first file_node_id, or null_file_node if empty.
    file_node_id
    first() const
    {
        return m_ids.empty() ? null_file_node : m_ids.front();
    }

    // tree
    //   returns a reference to the underlying file_tree.
    const tree_type&
    tree() const
    {
        return m_tree;
    }


    // --------------------------------------------------------
    //  grouping
    // --------------------------------------------------------

    // group_by_ext
    //   groups matching nodes by file extension.
    std::map<std::string, std::vector<file_node_id>>
    group_by_ext() const
    {
        std::map<std::string, std::vector<file_node_id>> result;

        for (file_node_id id : m_ids)
        {
            std::string n = m_tree.name_str(id);
            std::string ext;

            std::size_t dot = n.rfind('.');

            if (dot != std::string::npos)
            {
                ext = n.substr(dot);
            }

            result[ext].push_back(id);
        }

        return result;
    }

    // group_by
    //   groups matching nodes by a key function.
    template<typename KeyFn>
    auto
    group_by
    (
        KeyFn _key_fn
    ) const -> std::map<decltype(_key_fn(std::declval<file_node_id>())),
                        std::vector<file_node_id>>
    {
        using key_type = decltype(_key_fn(std::declval<file_node_id>()));

        std::map<key_type, std::vector<file_node_id>> result;

        for (file_node_id id : m_ids)
        {
            result[_key_fn(id)].push_back(id);
        }

        return result;
    }

    // partition
    //   splits into (matching, non-matching) queries.
    std::pair<file_tree_query, file_tree_query>
    partition
    (
        const file_node_predicate& _predicate
    ) const
    {
        std::vector<file_node_id> pass;
        std::vector<file_node_id> fail;

        for (file_node_id id : m_ids)
        {
            if (_predicate(id))
            {
                pass.push_back(id);
            }
            else
            {
                fail.push_back(id);
            }
        }

        return std::make_pair(
            file_tree_query(m_tree, std::move(pass)),
            file_tree_query(m_tree, std::move(fail)));
    }


private:
    const tree_type&     m_tree;
    file_node_id              m_root;
    std::vector<file_node_id> m_ids;
};


// make_file_tree_query
//   factory: deduces the tree type. Class-template argument deduction only
// arrived in C++17, so pre-C++17 callers need this rather than spelling the
// backend out by hand.
template<typename Tree>
file_tree_query<Tree>
make_file_tree_query(
    const Tree& _tree
)
{
    return file_tree_query<Tree>(_tree);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_FILTER_HPP
