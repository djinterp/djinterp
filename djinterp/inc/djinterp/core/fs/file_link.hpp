/******************************************************************************
* djinterp [fs]                                                  file_link.hpp
*
*   Symbolic links (roadmap Phase 8) -- create one, read where it points, ask
* whether a path is one. Free functions over paths, no handle, so no D_*_
* portability kit and a FLAT tier ladder.
*
*   A SYMLINK IS TEXT. read_symlink returns the link's TARGET exactly as stored
* -- it does NOT resolve it. The target may be relative, may point at nothing,
* may point at another link; reading the link tells you what it SAYS, not what
* it reaches. To resolve (follow the chain to a real file), that is d_realpath's
* job in file_dir, a different and fallible operation. Keeping the two separate
* is deliberate: reading a link is cheap and cannot fail for "the target does
* not exist", because a dangling link is a perfectly valid link.
*
*   is_symlink vs status. is_symlink(p) is the direct one-question form. It is
* also exactly symlink_status(p).is_symlink() -- the difference is only whether
* you want the single bit or the whole snapshot. Both do NOT follow the link
* (a symlink reports as a symlink), which is the only way to see the link
* itself rather than its target.
*
*   NO OS. The Windows privilege wrinkle that file_link.h documents (symlink
* creation may need a privilege) is the C layer's to report through errno;
* there is no platform branch here.
*
*
* path:      /inc/djinterp/core/fs/file_link.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.18
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    create_symlink     make a link pointing at a target
II.   read_symlink       read the target text (does NOT resolve)
III.  is_symlink         is this path a symlink (does NOT follow)
*/

#ifndef DJINTERP_FS_FILE_LINK_
#define DJINTERP_FS_FILE_LINK_ 1

#include "file_path.hpp"
#include "file_common.hpp"

#include "../../c/fs/file_link.h"    // d_symlink, d_readlink, d_is_symlink

#include <cerrno>                 // EINVAL


#if D_FILE_LINK_IS_AVAILABLE

NS_DJINTERP

// create_symlink
//   function: create a symlink at _link_path that points at _target. _target
// is stored verbatim -- it is NOT checked for existence (a link may point at
// something not yet there), and a relative target is resolved later relative to
// the link's own directory, not to here.
inline bool
create_symlink(const path& _target, const path& _link_path, error& _ec)
{
    if (!_target.valid() || !_link_path.valid())
    {
        _ec.assign(EINVAL);
        return false;
    }

    if (d_symlink(_target.c_str(), _link_path.c_str()) != 0)
    {
        _ec = error::from_errno();
        return false;
    }

    _ec.clear();
    return true;
}

// read_symlink
//   function: the target a symlink stores, as text, WITHOUT resolving it.
// Returns an invalid path with _ec set if _path is not a symlink or cannot be
// read. (d_readlink follows POSIX and does not NUL-terminate its buffer; this
// terminates by the returned length, which is the one detail a caller must not
// get wrong -- so it is done here, once.)
inline path
read_symlink(const path& _path, error& _ec)
{
    char    buf[D_FILE_PATH_MAX + 1];
    ssize_t n;

    if (!_path.valid())
    {
        _ec.assign(EINVAL);
        return path();
    }

    n = d_readlink(_path.c_str(), buf, sizeof(buf) - 1);

    if (n < 0)
    {
        _ec = error::from_errno();
        return path();
    }

    buf[n] = '\0';   // readlink writes no terminator; place it by the count
    _ec.clear();
    return path(buf);
}

// is_symlink
//   function: whether _path is a symlink, WITHOUT following it. Equivalent to
// symlink_status(_path).is_symlink(); this is the one-syscall form for when the
// single bit is all you need.
inline bool
is_symlink(const path& _path, error& _ec)
{
    int result;

    if (!_path.valid())
    {
        _ec.assign(EINVAL);
        return false;
    }

    result = d_is_symlink(_path.c_str());

    if (result < 0)
    {
        _ec = error::from_errno();
        return false;
    }

    _ec.clear();
    return result != 0;
}

NS_END  // djinterp

#endif // D_FILE_LINK_IS_AVAILABLE

#endif // DJINTERP_FS_FILE_LINK_
