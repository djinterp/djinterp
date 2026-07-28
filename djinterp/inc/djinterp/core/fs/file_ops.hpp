/******************************************************************************
* djinterp [fs]                                                   file_ops.hpp
*
*   Whole-file operations -- remove, rename, copy (roadmap Phase 7). These are
* free functions over paths, not a new handle type, so there is no ownership to
* manage and none of the D_*_ portability kit here: the tier ladder for this
* header is FLAT, the same source on every standard.
*
*   THE ATOMICITY ASYMMETRY, carried up from file_ops.h. rename() is atomic
* within a filesystem -- the destination is either the old file or the new one,
* never a half-written thing, and never briefly absent. copy_file() is NOT
* atomic and cannot be: it reads and writes bytes, so a reader watching the
* destination can see it partially written, and a crash mid-copy leaves it that
* way. When you need "replace this file as one indivisible step", rename onto
* it; copy_file is for duplication, not safe replacement. This is the module's
* main hazard, and it is stated here rather than assumed.
*
*   THE REMOVAL FAMILY. Three names, by what each will remove:
*     remove()          -- a file OR an empty directory (the general one)
*     remove_file()     -- a file only; refuses a directory
*     remove_directory()-- an empty directory only (lives in file_dir.hpp)
*   None of them recurse. Emptying a directory tree is a walk-and-delete the
* caller writes (or a later fs::remove_all), not a surprise inside remove().
*
*   NO OS. Every decision was made in c/fs; these are three-line wrappers --
* validate the path, call the C function, translate errno.
*
* 
* path:      /inc/djinterp/core/fs/file_ops.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.18
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    REMOVAL          remove / remove_file
II.   MOVEMENT         rename   (atomic; overwrites)
III.  DUPLICATION      copy_file (NOT atomic)
*/

#ifndef DJINTERP_FS_FILE_OPS_
#define DJINTERP_FS_FILE_OPS_ 1

#include "file_path.hpp"
#include "file_common.hpp"

#include "../../c/fs/file_ops.h"     // d_remove, d_unlink, d_rename, d_copy_file

#include <cerrno>                 // EINVAL


NS_DJINTERP

// ===========================================================================
// I.   REMOVAL
// ===========================================================================

// remove
//   function: remove a file or an EMPTY directory. Does not recurse -- a
// non-empty directory is refused by the platform (ENOTEMPTY), reported through
// _ec. Removing something that is not there is ENOENT, so a true return always
// means this call did the removing.
inline bool
remove(const path& _p, error& _ec)
{
    if (!_p.valid())
    {
        _ec.assign(EINVAL);
        return false;
    }

    if (d_remove(_p.c_str()) != 0)
    {
        _ec = error::from_errno();
        return false;
    }

    _ec.clear();
    return true;
}

// remove_file
//   function: remove a file, and only a file -- a directory is refused
// (EISDIR/EPERM, by platform). Use remove() or remove_directory() for a
// directory. The narrower promise is the point: this cannot delete a directory
// by surprise.
inline bool
remove_file(const path& _p, error& _ec)
{
    if (!_p.valid())
    {
        _ec.assign(EINVAL);
        return false;
    }

    if (d_unlink(_p.c_str()) != 0)
    {
        _ec = error::from_errno();
        return false;
    }

    _ec.clear();
    return true;
}


// ===========================================================================
// II.  MOVEMENT
// ===========================================================================

// rename
//   function: move/rename _from to _to, ATOMICALLY within a filesystem, and
// OVERWRITING _to if it exists (POSIX rename semantics). Atomic means _to is
// always either its old contents or _from's -- never a partial state, never
// momentarily missing -- which is exactly what makes rename the tool for
// replacing a file safely. Across filesystems the platform may refuse (EXDEV),
// because it cannot promise atomicity there; that is reported, not papered over
// with a non-atomic copy.
inline bool
rename(const path& _from, const path& _to, error& _ec)
{
    if (!_from.valid() || !_to.valid())
    {
        _ec.assign(EINVAL);
        return false;
    }

    if (d_rename(_from.c_str(), _to.c_str(), 1) != 0)   // 1 == overwrite
    {
        _ec = error::from_errno();
        return false;
    }

    _ec.clear();
    return true;
}


// ===========================================================================
// III. DUPLICATION
// ===========================================================================

// copy_file
//   function: copy the contents of _src to _dst, overwriting _dst. NOT atomic
// (see the header banner): a reader can observe _dst partway written, and a
// crash can leave it that way. For an indivisible replace, copy to a temporary
// and rename() it onto _dst. A missing _src is ENOENT.
inline bool
copy_file(const path& _src, const path& _dst, error& _ec)
{
    if (!_src.valid() || !_dst.valid())
    {
        _ec.assign(EINVAL);
        return false;
    }

    if (d_copy_file(_src.c_str(), _dst.c_str()) != 0)
    {
        _ec = error::from_errno();
        return false;
    }

    _ec.clear();
    return true;
}

NS_END  // djinterp

#endif // DJINTERP_FS_FILE_OPS_
