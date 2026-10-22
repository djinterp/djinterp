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
* rather than its target. On Windows a junction is a link too, as c/filesys
* describes one.
*   NO OS. The Windows privilege wrinkle that file_link.h documents (symlink
* creation may need a privilege) is the C layer's to report through errno;
* there is no platform branch here.
*
*
* path:      /inc/djinterp/core/filesys/file_link.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.20
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

#ifndef DJINTERP_FILESYS_FILE_LINK_HPP
#define DJINTERP_FILESYS_FILE_LINK_HPP 1

// std
#include <cerrno>   // EINVAL, ENAMETOOLONG, ENOMEM
#include <cstddef>  // std::size_t
// djinterp
#include "file_path.hpp"                // path
#include "file_common.hpp"              // error, the D_* kit
#include "file_stat.hpp"                // internal::type_query, file_kind
#include "../../c/filesys/file_link.h"  // d_file_symlink, d_file_readlink,
                                        // d_file_is_symlink


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

    // c/filesys creates the link, or reports through errno
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
NS_INTERNAL

    /**
     * @brief Reads a link's text into a scratch buffer, growing the buffer
     *        until the text fits.
     *
     * @note d_file_readlink truncates without saying so, as readlink(2)
     *       does, so a text that fills the buffer may have been cut short; it
     *       is read again into twice the room. A link's text may well be
     *       longer than D_FILE_PATH_MAX -- Windows keeps up to 16 KiB of it --
     *       but past 64 KiB only a filesystem that fills every buffer it is
     *       given remains, and that is ENAMETOOLONG.
     *
     * @param[in]     _path     the symbolic link to read.
     * @param[in,out] _scratch  the buffer; on success it holds the text.
     * @return the text's length, or -1 with errno set.
     */
    inline ssize_t
    read_link_text(
        const path&   _path,
        path_scratch& _scratch
    )
    {
        const std::size_t limit = (std::size_t)1 << 16;

        // until a text leaves room after it for its terminator
        for (;;)
        {
            const std::size_t room = _scratch.size() - 1;
            const ssize_t     n    = d_file_readlink(_path.c_str(),
                                                     _scratch.data(),
                                                     room);

            // a failure, or a text that did not fill the room, is the answer
            if ( (n < 0) ||
                 ((std::size_t)n < room) )
            {
                return n;
            }

            // a filesystem that fills every buffer has no answer to give
            if (_scratch.size() >= limit)
            {
                errno = ENAMETOOLONG;

                return -1;
            }

            // twice the room, or nowhere to read into
            if (!_scratch.grow(_scratch.size() * 2))
            {
                errno = ENOMEM;

                return -1;
            }
        }
    }

NS_END  // internal

/**
 * @brief Reads the target a symbolic link stores, as text, WITHOUT resolving
 *        it.
 *
 * @note The whole text, however long: a target longer than D_FILE_PATH_MAX
 *       is not cut short.
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

        return path(static_cast<const char*>(0));
    }

    internal::path_scratch scratch;
    const ssize_t          n = internal::read_link_text(_path,
                                                        scratch);

    // a failed read reports the platform's reason
    if (n < 0)
    {
        _ec = error::from_errno();

        return path(static_cast<const char*>(0));
    }

    // readlink writes no terminator; place it by the count
    scratch.data()[n] = '\0';
    _ec.clear();

    return path(scratch.data());
}

// 1.3    Inspection
//------------------------------------------------------------------------------
/**
 * @brief Reports whether a path is a symbolic link, WITHOUT following it.
 *
 * @note Equivalent to symlink_status(_path).is_symlink(); this is the
 *       one-question form for when the single bit is all you need. Like
 *       is_directory(), it tells "not a link" from "could not look": nothing
 *       there is a plain false, and a path that cannot be examined sets
 *       `_ec`.
 *
 * @param[in]  _path  the path to test.
 * @param[out] _ec    cleared on success, and when nothing is there; EINVAL
 *                    for an invalid path, otherwise the platform's reason
 *                    the path could not be examined.
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

    // the no-follow type query: it reports what it could not find out
    return internal::type_query(_path,
                                false,
                                _ec) == file_kind::type_symlink;
}


NS_END  // djinterp

#endif  // D_FILE_LINK_IS_AVAILABLE

#endif  // DJINTERP_FILESYS_FILE_LINK_HPP
