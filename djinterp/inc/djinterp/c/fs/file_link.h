/*******************************************************************************
* djinterp [c]                                                       file_link.h
*
* Symbolic links.
*   The whole API is compiled out when D_INTERNAL_FILE_HAS_SYMLINKS is 0, so
* guard your uses with D_FILE_LINK_IS_AVAILABLE.
*   That macro is a claim about the PLATFORM, not about your process. Windows
* has had symlinks since Vista and still refuses to create one without
* SeCreateSymbolicLinkPrivilege -- so d_file_symlink can compile, be available,
* and fail with EPERM for every ordinary user. Handle the runtime failure; do
* not infer it from the macro.
*
*
* path:      /inc/djinterp/c/fs/file_link.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SYMBOLIC LINKS
    --------------
    1.  Link operations
*/

#ifndef DJINTERP_C_FS_FILE_LINK_H
#define DJINTERP_C_FS_FILE_LINK_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "./file_common.h"                    // fs foundation, ssize_t
#include "../../config/c/fs/cfg_file_link.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


#if (D_INTERNAL_FILE_HAS_SYMLINKS == 1)

D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SYMBOLIC LINKS
//==============================================================================


// 1.1    Link operations
//------------------------------------------------------------------------------
/**
 * @brief Creates a symbolic link at `_linkpath` pointing at `_target`.
 *
 * @note    The target is stored as text: it is not resolved, checked, or
 *          required to exist, and a dangling link is not an error. Windows
 *          fixes a link's file-or-directory kind at creation, so there a
 *          dangling target makes a file link.
 * @warning Windows requires SeCreateSymbolicLinkPrivilege or Developer Mode;
 *          without either this fails with EPERM, however the build is
 *          configured.
 *
 * @param[in] _target    what the link points at.
 * @param[in] _linkpath  where to create the link.
 * @return 0, or -1 on failure with errno set.
 */
int     d_file_symlink(const char* _target,
                       const char* _linkpath);
/**
 * @brief Reads the text a symbolic link contains, without resolving it.
 *
 * @warning Follows readlink(2) exactly: the result is not NUL-terminated, and
 *          a target that does not fit is truncated rather than reported. A
 *          result equal to `_bufsize` may be either, so grow and retry.
 *
 * @param[in]  _path     the symbolic link to read.
 * @param[out] _buf      receives the target text.
 * @param[in]  _bufsize  size of `_buf`, in bytes.
 * @return the number of bytes written, or -1 on failure with errno set --
 *         EINVAL when `_path` is not a symbolic link.
 */
ssize_t d_file_readlink(const char* _path,
                        char*       _buf,
                        size_t      _bufsize);
/**
 * @brief Reports whether a path is itself a symbolic link, without following
 *        it.
 *
 * @param[in] _path  the path to test; may be `NULL`.
 * @return non-zero for a symbolic link; 0 otherwise, for `NULL`, or when the
 *         path cannot be examined.
 */
int     d_file_is_symlink(const char* _path);


D_EXTERN_C_END

#endif  // D_INTERNAL_FILE_HAS_SYMLINKS


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FS_FILE_LINK_H
