/*******************************************************************************
* djinterp [core]                                             file_recursive.hpp
*
* Recursive traversal (roadmap Phase 9) -- walking a whole tree, and removing
* one.
*   Both are built entirely on the pieces already in place: directory (the
* one-level walk), the mode query behind the status predicates (to classify an
* entry whose d_type the filesystem did not report), and operations (to
* delete). There is no new C module here; this is composition. Classifying
* by mode alone, neither stumbles at D_FILE_WIDTH 32 on a file too large for
* a 32-bit snapshot.
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
* link, not a directory; where d_type is absent, a no-follow mode query draws
* the same line.
*   walk() is PRE-ORDER and FAIL-FAST: it visits a directory, then its
* contents, and stops at the first directory it cannot read (with _ec set).
* remove_all() is POST-ORDER by necessity -- a directory cannot be removed
* until its children are -- and IDEMPOTENT: removing something already gone is
* success, not ENOENT.
*   A CHILD'S PATH IS THE KERNEL'S, NOT THE GRAMMAR'S. Both build it with
* d_dir_entry_path rather than path's operator/, whose grammar follows
* D_CFG_FILE_PATH_SYNTAX: a POSIX build reading Windows paths would take an
* entry named "C:x" for a drive and drop the directory in front of it, and
* remove_all would then delete whatever "C:x" names in the working directory.
*
*
* path:      /inc/djinterp/core/filesys/file_recursive.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RECURSION
    ---------
    1.  Traversal
    2.  Removal
*/

#ifndef DJINTERP_FILESYS_FILE_RECURSIVE_HPP
#define DJINTERP_FILESYS_FILE_RECURSIVE_HPP 1

// std
#include <cstddef>  // std::size_t
#include <vector>   // std::vector, the child gather in remove_all
// djinterp
#include "file_path.hpp"    // path
#include "file_common.hpp"  // error, the D_* kit
#include "file_stat.hpp"    // file_kind, internal::type_query
#include "file_dir.hpp"     // directory, directory_entry, remove_directory
#include "file_ops.hpp"     // remove_file


NS_DJINTERP


//==============================================================================
// 1.  RECURSION
//==============================================================================


// 1.1    Traversal
//------------------------------------------------------------------------------
NS_INTERNAL

    /**
     * @brief Builds the path of an entry read from a directory, for this
     *        platform to open (d_dir_entry_path).
     *
     * @note Never `_dir / _name`: see this header's banner.
     *
     * @param[in]  _dir   the directory the entry was read from.
     * @param[in]  _name  the entry's name.
     * @param[out] _ec    left as it was on success; on failure EINVAL for an
     *                    invalid operand, ENOMEM where the path cannot be
     *                    allocated, and otherwise d_dir_entry_path's reason --
     *                    ENAMETOOLONG for a path past D_FILE_PATH_MAX.
     * @return the entry's path; invalid on failure.
     * @throws std::bad_alloc only when D_CFG_PATH_THROW is 1 and the result
     *         cannot be allocated.
     */
    inline path
    entry_path(
        const path& _dir,
        const path& _name,
        error&      _ec
    )
    {
        path_scratch scratch;

        // an invalid operand names no entry
        if ( (!_dir.valid()) ||
             (!_name.valid()) )
        {
            _ec.assign(EINVAL);

            return path(static_cast<const char*>(0));
        }

        // +2 is the separator and the terminator
        if (!scratch.grow(_dir.size() + _name.size() + 2))
        {
            _ec.assign(ENOMEM);

            return path(static_cast<const char*>(0));
        }

        // the buffer fits by construction; what is left is the platform's
        // limit on a path, D_FILE_PATH_MAX
        if (!d_dir_entry_path(scratch.data(),
                              scratch.size(),
                              _dir.c_str(),
                              _name.c_str()))
        {
            _ec = error::from_errno();

            return path(static_cast<const char*>(0));
        }

        return path(scratch.data());
    }

    /**
     * @brief Reports whether an entry names a real directory -- NOT a symlink
     *        to one.
     *
     * @note Prefers the readdir d_type, which does not follow; where that is
     *       unreported, falls back to a no-follow mode query, so a link is
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

        error                      probe;
        const file_kind::file_type t = internal::type_query(_child,
                                                            false,
                                                            probe);

        return ( (!probe.failed()) &&
                 (t == file_kind::type_directory) );
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
            const path child  = entry_path(_dir,
                                           e.name(),
                                           _ec);

            // an entry whose path cannot be made stops the walk, with why
            if (_ec.failed())
            {
                return false;
            }

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
 * @warning Not proof against a race: each level is examined, then opened
 *          and removed by its path, so a process that can write in the
 *          tree and swaps a directory for a symbolic link in between sends
 *          the removal down the link -- the race std::filesystem's own
 *          remove_all had (CVE-2022-21658). Run it on trees no other user
 *          can write; closing it takes directory-relative calls
 *          (openat/unlinkat) the C layer does not offer yet.
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
    const file_kind::file_type t = internal::type_query(_p,
                                                        false,
                                                        _ec);

    // a path that cannot be examined cannot be removed safely
    if (_ec.failed())
    {
        return false;
    }

    // already absent
    if (t == file_kind::type_not_found)
    {
        _ec.clear();

        return true;
    }

    // Children are gathered into a list and the directory handle CLOSED before
    // any of them are removed, rather than deleting while the same directory
    // stream is being read -- which is fragile across platforms. Each entry
    // owns its name, so the gathered paths stay valid after the handle is gone.
    if (t == file_kind::type_directory)
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
                const path child = internal::entry_path(_p,
                                                        e.name(),
                                                        _ec);

                // a child whose path cannot be made cannot be removed
                if (_ec.failed())
                {
                    return false;
                }

                children.push_back(child);
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

#endif  // DJINTERP_FILESYS_FILE_RECURSIVE_HPP
