/*******************************************************************************
* djinterp [core]                                                   file_ops.hpp
*
* Whole-file operations -- remove, rename, copy (roadmap Phase 7).
*   These are free functions over paths, not a new handle type, so there is no
* ownership to manage and none of the D_*_ portability kit here: the tier
* ladder for this header is FLAT, the same source on every standard.
*   THE ATOMICITY ASYMMETRY, carried up from file_ops.h. rename() is atomic
* within a filesystem -- the destination is either the old file or the new
* one, never a half-written thing, and never briefly absent. copy_file() is
* NOT atomic and cannot be: it reads and writes bytes, so a reader watching the
* destination can see it partially written, and a crash mid-copy leaves it
* that way. When you need "replace this file as one indivisible step", rename
* onto it; copy_file is for duplication, not safe replacement. This is the
* module's main hazard, and it is stated here rather than assumed.
*   THE REMOVAL FAMILY. Three names, by what each will remove:
*     remove()           -- a file OR an empty directory (the general one)
*     remove_file()      -- a file only; refuses a directory
*     remove_directory() -- an empty directory only (in file_dir.hpp)
*   None of them recurse. Emptying a directory tree is remove_all, in
* file_recursive.hpp -- never a surprise inside remove(). And none of them
* follows a symbolic link: a link is removed itself, its target never, on
* Windows a link or junction to a directory included.
*   NO OS. Every decision was made in c/filesys; these are three-line
* wrappers -- validate the path, call the C function, translate errno.
*
*
* path:      /inc/djinterp/core/filesys/file_ops.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  Removal
    2.  Movement
    3.  Duplication
*/

#ifndef DJINTERP_FILESYS_FILE_OPS_HPP
#define DJINTERP_FILESYS_FILE_OPS_HPP 1

// std
#include <cerrno>  // EINVAL
// djinterp
#include "file_path.hpp"               // path
#include "file_common.hpp"             // error, the D_* kit
#include "../../c/filesys/file_ops.h"  // d_file_remove, d_file_unlink,
                                       // d_file_rename, d_file_copy


NS_DJINTERP


//==============================================================================
// 1.  OPERATIONS
//==============================================================================
// Each validates its paths, calls the c/filesys operation, and translates
// errno: true on success with the error cleared, false with it set.


// 1.1    Removal
//------------------------------------------------------------------------------
/**
 * @brief Removes a file or an EMPTY directory.
 *
 * @note Does not recurse: the platform refuses a non-empty directory
 *       (ENOTEMPTY). Removing something that is not there is ENOENT, so a
 *       true return always means this call did the removing.
 *
 * @param[in]  _p   the path to remove.
 * @param[out] _ec  cleared on success; EINVAL for an invalid path, otherwise
 *                  the platform's code.
 * @return true when this call removed `_p`.
 */
inline bool
remove(
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

    // c/filesys removes the path, or reports through errno
    if (d_file_remove(_p.c_str()) != 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return true;
}

/**
 * @brief Removes a file, and only a file.
 *
 * @note The narrower promise is the point: this cannot delete a directory by
 *       surprise. A directory is refused (EISDIR, EPERM or EACCES, by
 *       platform); use remove() or remove_directory() for one. A symbolic
 *       link to a directory is no directory, and is removed as a link.
 *
 * @param[in]  _p   the file to remove.
 * @param[out] _ec  cleared on success; EINVAL for an invalid path, otherwise
 *                  the platform's code.
 * @return true when this call removed `_p`.
 */
inline bool
remove_file(
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

    // c/filesys unlinks the file, or reports through errno
    if (d_file_unlink(_p.c_str()) != 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return true;
}

// 1.2    Movement
//------------------------------------------------------------------------------
/**
 * @brief Renames or moves a file ATOMICALLY within a filesystem, OVERWRITING
 *        the destination if it exists (POSIX rename semantics).
 *
 * @note Atomic means `_to` is always either its old contents or `_from`'s --
 *       never a partial state, never momentarily missing -- which is exactly
 *       what makes rename the tool for replacing a file safely. Across
 *       filesystems the platform may refuse (EXDEV) because it cannot promise
 *       atomicity there; that is reported, not papered over with a
 *       non-atomic copy.
 *
 * @param[in]  _from  the existing path.
 * @param[in]  _to    the new path.
 * @param[out] _ec    cleared on success; EINVAL for an invalid path,
 *                    otherwise the platform's code.
 * @return true on success.
 */
inline bool
rename(
    const path& _from,
    const path& _to,
    error&      _ec
)
{
    // an invalid path names nothing to move
    if ( (!_from.valid()) ||
         (!_to.valid()) )
    {
        _ec.assign(EINVAL);

        return false;
    }

    // POSIX semantics: 1 asks d_file_rename to overwrite
    if (d_file_rename(_from.c_str(),
                      _to.c_str(),
                      1) != 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return true;
}

// 1.3    Duplication
//------------------------------------------------------------------------------
/**
 * @brief Copies the contents of one file to another, overwriting the
 *        destination unless D_CFG_FILE_OPS_COPY_OVERWRITE is 0.
 *
 * @warning NOT atomic (see the banner): a reader can observe `_dst` partway
 *          written, and a crash can leave it that way. For an indivisible
 *          replacement, copy to a temporary and rename() it onto `_dst`.
 *
 * @param[in]  _src  the file to copy; a missing one is ENOENT.
 * @param[in]  _dst  the destination; `_src` itself under any name is refused
 *                   with EINVAL and left untouched.
 * @param[out] _ec   cleared on success; EINVAL for an invalid path, otherwise
 *                   the platform's code.
 * @return true on success.
 */
inline bool
copy_file(
    const path& _src,
    const path& _dst,
    error&      _ec
)
{
    // an invalid path names nothing to copy
    if ( (!_src.valid()) ||
         (!_dst.valid()) )
    {
        _ec.assign(EINVAL);

        return false;
    }

    // c/filesys copies the file, or reports through errno
    if (d_file_copy(_src.c_str(),
                    _dst.c_str()) != 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return true;
}


NS_END  // djinterp

#endif  // DJINTERP_FILESYS_FILE_OPS_HPP
