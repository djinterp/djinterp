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
*   On Windows a junction counts as a link too: it names another path, and
* following it unawares does the same damage.
*
*
* path:      /inc/djinterp/c/filesys/file_link.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SYMBOLIC LINKS
    --------------
    1.  Link operations
*/

#ifndef DJINTERP_C_FILESYS_FILE_LINK_H
#define DJINTERP_C_FILESYS_FILE_LINK_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "./file_common.h"                         // foundation, ssize_t
#include "../../config/c/filesys/cfg_file_link.h"  // module configuration


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
 *          fixes a link's file-or-directory kind at creation, judging a
 *          relative target from the link's own directory, where it will be
 *          read; a dangling target makes a file link. Windows also stores '/'
 *          in the target as '\\', the one separator it follows there.
 * @warning Windows requires SeCreateSymbolicLinkPrivilege or Developer Mode;
 *          without either this fails with EPERM, however the build is
 *          configured.
 *
 * @param[in] _target    what the link points at.
 * @param[in] _linkpath  where to create the link.
 * @return 0, or -1 on failure with errno set -- EEXIST when `_linkpath` is
 *         taken.
 */
int     d_file_symlink(const char* _target,
                       const char* _linkpath);
/**
 * @brief Reads the text a symbolic link contains, without resolving it.
 *
 * @note    On Windows this is the text the link holds, as its creator wrote
 *          it, not the path it reaches; a junction reads as a link does.
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
 *        it -- on Windows a junction too.
 *
 * @param[in] _path  the path to test; may be `NULL`.
 * @return non-zero for a symbolic link; 0 otherwise, for `NULL`, or when the
 *         path cannot be examined.
 */
int     d_file_is_symlink(const char* _path);


D_EXTERN_C_END

#endif  // D_INTERNAL_FILE_HAS_SYMLINKS


#endif  // DJINTERP_C_FILESYS_FILE_LINK_H
