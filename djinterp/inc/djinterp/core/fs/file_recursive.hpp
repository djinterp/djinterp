/******************************************************************************
* djinterp [fs]                                             file_recursive.hpp
*
*   Recursive traversal (roadmap Phase 9) -- walking a whole tree, and removing
* one. Both are built entirely on the pieces already in place: directory (the
* one-level walk), status (to classify an entry whose d_type the filesystem did
* not report), and operations (to delete). There is no new C module here; this
* is composition.
*
*   WHY A TEMPLATE VISITOR, NOT AN ITERATOR. A recursive_directory_iterator
* would have to own a STACK of open directory handles, one per level -- and a
* growable container of a move-only handle is not expressible on C++98, where
* there is no move to grow it with. A recursive FUNCTION sidesteps that: each
* level's directory is a local, and the C++ call stack is the level stack, with
* no depth cap and no per-tier divergence. walk() therefore takes a visitor and
* recurses. The visitor is any callable -- a function pointer on C++98, a lambda
* on C++11+ -- so the surface is identical on every standard.
*
*   SYMLINKS ARE NOT FOLLOWED. A directory walk that follows symlinks can loop
* forever (a link pointing at an ancestor) or wander out of the tree entirely.
* Both walk() and remove_all() see a symlink as a symlink and stop there -- they
* never descend through it. This falls out naturally from d_type, which reports
* an entry's OWN type without following, so a link-to-directory is a link, not a
* directory; where d_type is absent, symlink_status (also no-follow) draws the
* same line.
*
*   walk() is PRE-ORDER and FAIL-FAST: it visits a directory, then its contents,
* and stops at the first directory it cannot read (with _ec set). remove_all()
* is POST-ORDER by necessity -- a directory cannot be removed until its children
* are -- and IDEMPOTENT: removing something already gone is success, not ENOENT.
*
* 
* path:      /inc/djinterp/core/fs/file_recursive.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    walk           recursive pre-order traversal with a visitor
II.   remove_all     recursive post-order delete
*/

#ifndef DJINTERP_FS_FILE_RECURSIVE_
#define DJINTERP_FS_FILE_RECURSIVE_ 1

#include "file_path.hpp"
#include "file_common.hpp"
#include "file_stat.hpp"
#include "file_dir.hpp"
#include "file_ops.hpp"

#include <vector>                 // child gather in remove_all
#include <cstddef>                // std::size_t


NS_DJINTERP

// ===========================================================================
// I.   walk
// ===========================================================================

NS_INTERNAL
    // entry_is_directory
    //   function: does this entry name a real directory (NOT a symlink to one)?
    // Prefers the readdir d_type, which is no-follow; where that is unreported,
    // falls back to a no-follow symlink_status so a link is never mistaken for
    // the directory it points at.
    inline bool
    entry_is_directory(const path& _child, const directory_entry& _e)
    {
        if (_e.type_known())
        {
            return _e.is_directory();
        }

        error       probe;
        file_status st = symlink_status(_child, probe);

        return (!probe.failed()) && st.is_directory();
    }

    // walk_impl
    //   function: the recursion. Visits every entry of _dir in pre-order,
    // descending into real subdirectories only. Fail-fast: a directory that
    // cannot be opened or read stops the walk with _ec set.
    template<typename _Visitor>
    bool
    walk_impl(const path& _dir, _Visitor& _visit, unsigned _depth, error& _ec)
    {
        directory       d(_dir, _ec);
        directory_entry e;

        if (!d.is_open())
        {
            return false;   // _ec set by the directory constructor
        }

        while (d.read(e, _ec))
        {
            path child   = _dir / e.name();
            bool is_dir  = entry_is_directory(child, e);

            _visit(child, _depth, is_dir);

            if (is_dir)
            {
                if (!walk_impl(child, _visit, _depth + 1, _ec))
                {
                    return false;   // propagate the failing level's _ec
                }
            }
        }

        return !_ec.failed();   // false if read() ended on an error
    }
NS_END  // internal


// walk
//   function: recursively visit every entry under _root, calling
//     _visit(const path& full_path, unsigned depth, bool is_directory)
// for each, in pre-order (a directory before its contents). Symlinks are not
// followed. Returns true when the whole tree was walked; false, with _ec set,
// at the first directory that could not be read.
//
//   The visitor is taken by value but is free to hold references to external
// state (capture-by-reference on C++11+, a pointer member on C++98) -- one copy
// is threaded through the whole recursion, so such state accumulates across the
// walk. Taking it by value is what lets a temporary lambda be passed inline.
template<typename _Visitor>
bool
walk(const path& _root, _Visitor _visit, error& _ec)
{
    return internal::walk_impl(_root, _visit, 0, _ec);
}


// ===========================================================================
// II.  remove_all
// ===========================================================================

// remove_all
//   function: remove _p and everything beneath it, returning true on success.
// A directory is emptied depth-first and then removed; a file or a symlink is
// unlinked (the LINK, never the target it points at). Removing a path that is
// not there is success -- the postcondition (nothing at _p) already holds -- so
// a true return means _p is gone, whether or not this call did the removing.
//
//   Children are gathered into a list and the directory handle CLOSED before
// any of them are removed, rather than deleting while the same directory stream
// is being read -- which is fragile across platforms. Each entry owns its name,
// so the gathered paths stay valid after the handle is gone.
inline bool
remove_all(const path& _p, error& _ec)
{
    file_status st = symlink_status(_p, _ec);   // no-follow: a link is a link

    if (_ec.failed())
    {
        return false;
    }

    if (st.type() == file_status::type_not_found)
    {
        _ec.clear();
        return true;   // already absent
    }

    if (st.is_directory())
    {
        std::vector<path> children;

        {
            directory       d(_p, _ec);
            directory_entry e;

            if (!d.is_open())
            {
                return false;
            }

            while (d.read(e, _ec))
            {
                children.push_back(_p / e.name());
            }

            if (_ec.failed())
            {
                return false;   // a read error -- do not delete a partial view
            }
        }   // directory handle closed here, before any removal

        for (std::size_t i = 0; i < children.size(); ++i)
        {
            if (!remove_all(children[i], _ec))
            {
                return false;
            }
        }

        return remove_directory(_p, _ec);
    }

    // a regular file or a symlink: unlink removes the entry itself
    return remove_file(_p, _ec);
}

NS_END  // djinterp

#endif // DJINTERP_FS_FILE_RECURSIVE_
