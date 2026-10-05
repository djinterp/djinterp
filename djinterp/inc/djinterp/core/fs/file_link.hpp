/*******************************************************************************
* djinterp [core]                                                  file_link.hpp
*
* Symbolic links (roadmap Phase 8) -- create one, read where it points, ask
* whether a path is one.
*   Free functions over paths, no handle, so no D_*_ portability kit and a FLAT
* tier ladder.
*   A SYMLINK IS TEXT. read_symlink returns the link's TARGET exactly as stored
* -- it does NOT resolve it. The target may be relative, may point at nothing,
* may point at another link; reading the link tells you what it SAYS, not what
* it reaches. Resolving (following the chain to a real file) is
* d_path_resolve's job in file_dir, a different and fallible operation.
* Keeping the two separate is deliberate: reading a link is cheap and cannot
* fail for "the target does not exist", because a dangling link is a perfectly
* valid link.
*   is_symlink vs status. is_symlink(p) is the direct one-question form. It is
* also exactly symlink_status(p).is_symlink() -- the difference is only whether
* you want the single bit or the whole snapshot. Neither follows the link (a
* symlink reports as a symlink), which is the only way to see the link itself
* rather than its target.
*   NO OS. The Windows privilege wrinkle that file_link.h documents (symlink
* creation may need a privilege) is the C layer's to report through errno;
* there is no platform branch here.
*
*
* path:      /inc/djinterp/core/fs/file_link.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SYMBOLIC LINKS
    --------------
    1.  Creation
    2.  Reading
    3.  Inspection
*/

#ifndef DJINTERP_FS_FILE_LINK_HPP
#define DJINTERP_FS_FILE_LINK_HPP 1

// std
#include <cerrno>  // EINVAL
// djinterp
#include "file_path.hpp"           // path
#include "file_common.hpp"         // error, the D_* kit
#include "../../c/fs/file_link.h"  // d_file_symlink, d_file_readlink,
                                   // d_file_is_symlink
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


#if D_FILE_LINK_IS_AVAILABLE

NS_DJINTERP


//==============================================================================
// 1.  SYMBOLIC LINKS
//==============================================================================


// 1.1    Creation
//------------------------------------------------------------------------------
/**
 * @brief Creates a symbolic link at `_link_path` that points at `_target`.
 *
 * @note `_target` is stored verbatim: it is NOT checked for existence (a link
 *       may point at something not yet there), and a relative target is
 *       resolved later relative to the link's own directory, not to here.
 *
 * @param[in]  _target     what the link points at.
 * @param[in]  _link_path  where to create the link.
 * @param[out] _ec         cleared on success; EINVAL for an invalid path,
 *                         otherwise the platform's code.
 * @return true on success.
 */
inline bool
create_symlink(
    const path& _target,
    const path& _link_path,
    error&      _ec
)
{
    // an invalid path names nothing to link
    if ( (!_target.valid()) ||
         (!_link_path.valid()) )
    {
        _ec.assign(EINVAL);

        return false;
    }

    // c/fs creates the link, or reports through errno
    if (d_file_symlink(_target.c_str(),
                       _link_path.c_str()) != 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return true;
}

// 1.2    Reading
//------------------------------------------------------------------------------
/**
 * @brief Reads the target a symbolic link stores, as text, WITHOUT resolving
 *        it.
 *
 * @param[in]  _path  the symbolic link to read.
 * @param[out] _ec    cleared on success; EINVAL for an invalid path,
 *                    otherwise the platform's code -- EINVAL again when
 *                    `_path` is not a symbolic link.
 * @return the target, or an invalid path on failure.
 */
inline path
read_symlink(
    const path& _path,
    error&      _ec
)
{
    // an invalid path names no link
    if (!_path.valid())
    {
        _ec.assign(EINVAL);

        return path();
    }

    char buf[D_FILE_PATH_MAX + 1];

    // d_file_readlink follows POSIX and writes no terminator, so the
    // buffer keeps a byte back for the one placed below
    const ssize_t n = d_file_readlink(_path.c_str(),
                                      buf,
                                      sizeof(buf) - 1);

    // a failed read reports the platform's reason
    if (n < 0)
    {
        _ec = error::from_errno();

        return path();
    }

    buf[n] = '\0';   // readlink writes no terminator; place it by the count
    _ec.clear();

    return path(buf);
}

// 1.3    Inspection
//------------------------------------------------------------------------------
/**
 * @brief Reports whether a path is a symbolic link, WITHOUT following it.
 *
 * @note Equivalent to symlink_status(_path).is_symlink(); this is the
 *       one-syscall form for when the single bit is all you need.
 *
 * @param[in]  _path  the path to test.
 * @param[out] _ec    cleared on success; EINVAL for an invalid path.
 * @return true for a symbolic link.
 */
inline bool
is_symlink(
    const path& _path,
    error&      _ec
)
{
    // an invalid path names nothing to test
    if (!_path.valid())
    {
        _ec.assign(EINVAL);

        return false;
    }

    const int result = d_file_is_symlink(_path.c_str());

    // a negative answer would be a failure to examine the path
    if (result < 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return result != 0;
}


NS_END  // djinterp

#endif  // D_FILE_LINK_IS_AVAILABLE

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_LINK_HPP
