/*******************************************************************************
* djinterp [c]                                                        file_ops.h
*
* Whole-file operations -- removing, renaming, copying.
*   d_file_rename is atomic; d_file_copy is not, and cannot be. That asymmetry
* is the module's main hazard and is documented per function rather than
* assumed.
*
*
* path:      /inc/djinterp/c/fs/file_ops.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
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

#ifndef DJINTERP_C_FS_FILE_OPS_H
#define DJINTERP_C_FS_FILE_OPS_H 1

// djinterp
#include "./file_common.h"                   // fs foundation, D_EXTERN_C_*
#include "../../config/c/fs/cfg_file_ops.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Removal
//------------------------------------------------------------------------------
/**
 * @brief Removes a file or an empty directory (ISO C remove).
 *
 * @param[in] _path  the path to remove.
 * @return 0, or -1 on failure with errno set.
 */
int d_file_remove(const char* _path);
/**
 * @brief Removes a name from the filesystem, refusing directories.
 *
 * @note    It removes the NAME, not necessarily the file: the data survives
 *          while another hard link or an open descriptor still refers to it.
 * @warning The ISO C backend has only remove(), so there a directory is not
 *          refused.
 *
 * @param[in] _path  the path to unlink.
 * @return 0, or -1 on failure with errno set.
 */
int d_file_unlink(const char* _path);

// 1.2    Movement
//------------------------------------------------------------------------------
/**
 * @brief Renames or moves a file, atomically within one filesystem.
 *
 * @note    An observer sees the old name or the new one -- never neither,
 *          never both, and never a partial file. Across filesystems this
 *          fails with EXDEV rather than copying behind the caller's back,
 *          unless D_CFG_FILE_OPS_RENAME_CROSS_DEVICE accepts a NON-ATOMIC
 *          copy and delete instead.
 * @warning With `_overwrite` 0 the existence check is not atomic with the
 *          rename: another process can create `_new` in between.
 *
 * @param[in] _old        the existing path.
 * @param[in] _new        the new path.
 * @param[in] _overwrite  non-zero to replace an existing `_new`, 0 to fail
 *                        with EEXIST. POSIX always replaces and Win32 never
 *                        does, so every caller must say which it wants.
 * @return 0, or -1 on failure with errno set.
 */
int d_file_rename(const char* _old,
                  const char* _new,
                  int         _overwrite);

// 1.3    Duplication
//------------------------------------------------------------------------------
/**
 * @brief Copies a file's contents -- and by default its permission bits -- to
 *        a new path.
 *
 * @note    Where configured, the platform's engine does the work:
 *          copy_file_range on Linux, which may make the copy an instant
 *          reflink, or fcopyfile on macOS, which also copies metadata. A
 *          declined engine falls back to a portable copy with the same
 *          result. Owner, timestamps, extended attributes and ACLs are not
 *          copied, except by fcopyfile.
 * @warning Not atomic: a reader sees the destination grow, and a crash leaves
 *          it partial. When that matters, copy to a sibling and d_file_rename
 *          it into place. A failed copy removes its partial destination.
 *
 * @param[in] _src  the source path; must name a regular file.
 * @param[in] _dst  the destination path; an existing file is replaced unless
 *                  D_CFG_FILE_OPS_COPY_OVERWRITE is 0.
 * @return 0, or -1 on failure with errno set; ENOSYS on the ISO C backend.
 */
int d_file_copy(const char* _src,
                const char* _dst);


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FS_FILE_OPS_H
