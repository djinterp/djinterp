/*******************************************************************************
* djinterp [core]                                             file_recursive.hpp
*
* Recursive traversal (roadmap Phase 9) -- walking a whole tree, and removing
* one.
*   Both are built entirely on the pieces already in place: directory (the
* one-level walk), status (to classify an entry whose d_type the filesystem did
* not report), and operations (to delete). There is no new C module here; this
* is composition.
*   WHY A TEMPLATE VISITOR, NOT AN ITERATOR. A recursive_directory_iterator
* would have to own a STACK of open directory handles, one per level -- and a
* growable container of a move-only handle is not expressible on C++98, where
* there is no move to grow it with. A recursive FUNCTION sidesteps that: each
* level's directory is a local, and the C++ call stack is the level stack, with
* no depth cap and no per-tier divergence. walk() therefore takes a visitor and
* recurses. The visitor is any callable -- a function pointer on C++98, a
* lambda on C++11+ -- so the surface is identical on every standard.
*   SYMLINKS ARE NOT FOLLOWED. A directory walk that follows symlinks can loop
* forever (a link pointing at an ancestor) or wander out of the tree entirely.
* Both walk() and remove_all() see a symlink as a symlink and stop there --
* they never descend through it. This falls out naturally from d_type, which
* reports an entry's OWN type without following, so a link-to-directory is a
* link, not a directory; where d_type is absent, symlink_status (also
* no-follow) draws the same line.
*   walk() is PRE-ORDER and FAIL-FAST: it visits a directory, then its
* contents, and stops at the first directory it cannot read (with _ec set).
* remove_all() is POST-ORDER by necessity -- a directory cannot be removed
* until its children are -- and IDEMPOTENT: removing something already gone is
* success, not ENOENT.
*
*
* path:      /inc/djinterp/core/fs/file_recursive.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RECURSION
    ---------
    1.  Traversal
    2.  Removal
*/

#ifndef DJINTERP_FS_FILE_RECURSIVE_HPP
#define DJINTERP_FS_FILE_RECURSIVE_HPP 1

// std
#include <cstddef>  // std::size_t
#include <vector>   // std::vector, the child gather in remove_all
// djinterp
#include "file_path.hpp"    // path
#include "file_common.hpp"  // error, the D_* kit
#include "file_stat.hpp"    // file_status, symlink_status
#include "file_dir.hpp"     // directory, directory_entry, remove_directory
#include "file_ops.hpp"     // remove_file
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


NS_DJINTERP


//==============================================================================
// 1.  RECURSION
//==============================================================================


// 1.1    Traversal
//------------------------------------------------------------------------------
NS_INTERNAL

    /**
     * @brief Reports whether an entry names a real directory -- NOT a symlink
     *        to one.
     *
     * @note Prefers the readdir d_type, which does not follow; where that is
     *       unreported, falls back to a no-follow symlink_status, so a link is
     *       never mistaken for the directory it points at.
     *
     * @param[in] _child  the entry's full path.
     * @param[in] _e      the entry as the directory reported it.
     * @return true for a directory; false otherwise, and when the entry cannot
     *         be examined.
     */
    inline bool
    entry_is_directory(
        const path&            _child,
        const directory_entry& _e
    )
    {
        // the kernel already said what it is
        if (_e.type_known())
        {
            return _e.is_directory();
        }

        error             probe;
        const file_status st = symlink_status(_child,
                                              probe);

        return ( (!probe.failed()) &&
                 (st.is_directory()) );
    }

    /**
     * @brief The recursion behind walk(): visits every entry of a directory in
     *        pre-order, descending into real subdirectories only.
     *
     * @tparam Visitor  the callable's type.
     *
     * @param[in]     _dir    the directory to visit.
     * @param[in,out] _visit  the visitor, shared by the whole recursion.
     * @param[in]     _depth  how many levels below the root `_dir` is.
     * @param[out]    _ec     set at the first directory that cannot be read.
     * @return true when the subtree was walked; false at the first failure.
     */
    template<typename Visitor>
    bool
    walk_impl(
        const path& _dir,
        Visitor&    _visit,
        unsigned    _depth,
        error&      _ec
    )
    {
        directory       d(_dir,
                          _ec);
        directory_entry e;

        // the directory constructor has already set _ec
        if (!d.is_open())
        {
            return false;
        }

        // visit each entry, then descend into it if it is a real directory
        while (d.read(e,
                      _ec))
        {
            const path child  = _dir / e.name();
            const bool is_dir = entry_is_directory(child,
                                                   e);

            _visit(child,
                   _depth,
                   is_dir);

            // fail fast: a subtree that cannot be read stops the whole walk
            if (is_dir)
            {
                // propagate the failing level's _ec
                if (!walk_impl(child,
                               _visit,
                               _depth + 1,
                               _ec))
                {
                    return false;
                }
            }
        }

        // read() ended either at the end of the directory or on an error
        return !_ec.failed();
    }

NS_END  // internal

/**
 * @brief Visits every entry under a directory, recursively and in pre-order:
 *        a directory before its contents.
 *
 * @note Symbolic links are reported but never followed. The visitor is taken
 *       by value -- which is what lets a temporary lambda be passed inline --
 *       but one copy is threaded through the whole recursion, so state it
 *       refers to (capture-by-reference on C++11+, a pointer member on C++98)
 *       accumulates across the walk.
 *
 * @tparam Visitor  any callable as
 *                  `void(const path& full_path, unsigned depth,
 *                  bool is_directory)`.
 *
 * @param[in]  _root   the directory to walk.
 * @param[in]  _visit  the visitor, called once per entry.
 * @param[out] _ec     cleared on success; set at the first directory that
 *                     could not be read.
 * @return true when the whole tree was walked; false at the first failure.
 */
template<typename Visitor>
bool
walk(
    const path& _root,
    Visitor     _visit,
    error&      _ec
)
{
    return internal::walk_impl(_root,
                               _visit,
                               0,
                               _ec);
}

// 1.2    Removal
//------------------------------------------------------------------------------
/**
 * @brief Removes a path and everything beneath it.
 *
 * @note A directory is emptied depth-first and then removed; a file or a
 *       symlink is unlinked -- the LINK, never the target it points at.
 *       Removing a path that is not there is success, since the postcondition
 *       (nothing at `_p`) already holds.
 *
 * @param[in]  _p   the path to remove.
 * @param[out] _ec  cleared on success; set at the first failure, which stops
 *                  the removal.
 * @return true when `_p` is gone, whether or not this call did the removing.
 */
inline bool
remove_all(
    const path& _p,
    error&      _ec
)
{
    // no-follow: a link is a link
    const file_status st = symlink_status(_p,
                                          _ec);

    // a path that cannot be examined cannot be removed safely
    if (_ec.failed())
    {
        return false;
    }

    // already absent
    if (st.type() == file_status::type_not_found)
    {
        _ec.clear();

        return true;
    }

    // Children are gathered into a list and the directory handle CLOSED before
    // any of them are removed, rather than deleting while the same directory
    // stream is being read -- which is fragile across platforms. Each entry
    // owns its name, so the gathered paths stay valid after the handle is gone.
    if (st.is_directory())
    {
        std::vector<path> children;

        {
            directory       d(_p,
                              _ec);
            directory_entry e;

            // the directory constructor has already set _ec
            if (!d.is_open())
            {
                return false;
            }

            // gather every child before touching any of them
            while (d.read(e,
                          _ec))
            {
                children.push_back(_p / e.name());
            }

            // a read error: do not delete a partial view
            if (_ec.failed())
            {
                return false;
            }
        }   // the directory handle closes here, before any removal

        // children first: a directory cannot go until they have
        for (std::size_t i = 0; i < children.size(); ++i)
        {
            // stop at the first child that will not go
            if (!remove_all(children[i],
                            _ec))
            {
                return false;
            }
        }

        return remove_directory(_p,
                                _ec);
    }

    // a regular file or a symlink: unlink removes the entry itself
    return remove_file(_p,
                       _ec);
}


NS_END  // djinterp

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_RECURSIVE_HPP
