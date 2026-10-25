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
*                                                            revised: 2026.10.21
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
#include <cerrno>   // errno, EINVAL, ELOOP, ENOENT, ENOTDIR
#include <vector>   // std::vector, the walk's open directories
// djinterp
#include "file_path.hpp"    // path
#include "file_common.hpp"  // error, the D_* kit
#include "file_dir.hpp"     // d_dir_open_at, d_dir_remove_all, the C layer's


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
     * @brief Whether a name is "." or "..", which name no entry to visit.
     *
     * @param[in] _name  the name.
     * @return true for either.
     */
    inline bool
    is_dots(
        const char* _name
    )
    {
        return ( (_name[0] == '.') &&
                 ( (_name[1] == '\0') ||
                   ( (_name[1] == '.') &&
                     (_name[2] == '\0') ) ) );
    }

    // walk_frame
    //   struct (internal): one directory open on the way down: its handle,
    // its path, and the depth its entries are visited at.
    struct walk_frame
    {
        struct d_dir_t* m_dir;
        path            m_path;
        unsigned        m_depth;
    };

    // walk_stack
    //   class (internal): the directories open from the root down, each
    // closed however the walk ends -- a failure, or a visitor that throws.
    class walk_stack
    {
    public:
        walk_stack(void)
            : m_frames()
        {}

        ~walk_stack(void)
        {
            while (!m_frames.empty())
            {
                pop();
            }
        }

        void push(struct d_dir_t* _dir,
                  const path&     _path,
                  unsigned        _depth)
        {
            walk_frame frame;

            frame.m_dir   = _dir;
            frame.m_path  = _path;
            frame.m_depth = _depth;

            // owner_guard
            //   struct (internal): closes the handle should the frame not be
            // kept -- push_back throwing where exceptions are on.
            struct owner_guard
            {
                struct d_dir_t* m_dir;

                ~owner_guard(void)
                {
                    if (m_dir)
                    {
                        (void)d_dir_close(m_dir);
                    }
                }
            };

            owner_guard guard = { _dir };

            m_frames.push_back(frame);
            guard.m_dir = 0;

            return;
        }

        void pop(void)
        {
            (void)d_dir_close(m_frames.back().m_dir);
            m_frames.pop_back();

            return;
        }

        walk_frame& back(void)
        {
            return m_frames.back();
        }

        bool empty(void) const
        {
            return m_frames.empty();
        }

    private:
        D_DELETED_FN(walk_stack(const walk_stack& _other))
        D_DELETED_FN(walk_stack& operator=(const walk_stack& _other))

        std::vector<walk_frame> m_frames;
    };

    /**
     * @brief The loop behind walk(): every entry in pre-order, descending only
     *        into what d_dir_open_at opens -- a directory, never a link to one,
     *        and looked up in the directory it was read from.
     *
     * @tparam Visitor  the callable's type.
     *
     * @param[in]     _root   the directory whose tree to visit.
     * @param[in,out] _visit  the visitor.
     * @param[out]    _ec     set at the first failure.
     * @return true when the tree was walked; false at the first failure.
     */
    template<typename Visitor>
    inline bool
    walk_tree(
        const path& _root,
        Visitor&    _visit,
        error&      _ec
    )
    {
        // an invalid root names nothing to walk
        if (!_root.valid())
        {
            _ec.assign(EINVAL);

            return false;
        }

        // the root itself may be named through a link, as any path may
        struct d_dir_t* const root = d_dir_open(_root.c_str());

        if (!root)
        {
            _ec = error::from_errno();

            return false;
        }

        walk_stack stack;

        stack.push(root,
                   _root,
                   0);

        // clear from here on: entry_path leaves _ec as it was on success
        _ec.clear();

        while (!stack.empty())
        {
            walk_frame& top  = stack.back();
            uint8_t     type = DT_UNKNOWN;

            errno = 0;

            const char* const name = d_dir_read_name(top.m_dir,
                                                     &type);

            // the end of a directory, or a read that failed
            if (!name)
            {
                if (errno != 0)
                {
                    _ec = error::from_errno();

                    return false;
                }

                stack.pop();

                continue;
            }

            // "." and ".." are no entries
            if (is_dots(name))
            {
                continue;
            }

            const path child = entry_path(top.m_path,
                                          path(name),
                                          _ec);

            // an entry whose path cannot be made stops the walk, with why
            if (_ec.failed())
            {
                return false;
            }

            struct d_dir_t* sub = 0;

            // a directory, or what may be one, opened without following a
            // link; refused, it is visited as what it is, no directory
            if ( (type == DT_DIR) ||
                 (type == DT_UNKNOWN) )
            {
                sub = d_dir_open_at(top.m_dir,
                                    name);

                if ( (!sub)              &&
                     (errno != ELOOP)    &&
                     (errno != ENOTDIR)  &&
                     (errno != ENOENT) )
                {
                    _ec = error::from_errno();

                    return false;
                }
            }

            const unsigned depth = top.m_depth;

            _visit(child,
                   depth,
                   (sub != 0));

            // its entries next, a level down
            if (sub)
            {
                stack.push(sub,
                           child,
                           depth + 1);
            }
        }

        _ec.clear();

        return true;
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
    return internal::walk_tree(_root,
                               _visit,
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
 * @note Proof against a process that swaps a directory in the tree for a
 *       link while it runs -- the race std::filesystem's own remove_all had
 *       (CVE-2022-21658) -- as d_dir_remove_all is, which it is. One handle
 *       is open per level of depth; a tree past PATH_MAX goes too.
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
    // an invalid path names nothing to remove
    if (!_p.valid())
    {
        _ec.assign(EINVAL);

        return false;
    }

    // d_dir_remove_all: relative to each directory it read, links as
    // themselves, nothing there already a success
    if (d_dir_remove_all(_p.c_str()) != 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return true;
}


NS_END  // djinterp

#endif  // DJINTERP_FILESYS_FILE_RECURSIVE_HPP
