/*******************************************************************************
* djinterp [c]                                                        file_ops.h
*
* Whole-file operations -- removing, renaming, copying.
*   d_file_rename is atomic; d_file_copy is not, and cannot be. That asymmetry
* is the module's main hazard and is documented per function rather than
* assumed.
*
*
* path:      /inc/djinterp/c/filesys/file_ops.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
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

#ifndef DJINTERP_C_FILESYS_FILE_OPS_H
#define DJINTERP_C_FILESYS_FILE_OPS_H 1

// djinterp
#include "./file_common.h"                        // foundation, D_EXTERN_C_*
#include "../../config/c/filesys/cfg_file_ops.h"  // module configuration


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Removal
//------------------------------------------------------------------------------
/**
 * @brief Removes a file or an empty directory (ISO C remove).
 *
 * @note A symbolic link is removed itself, never its target. On Windows,
 *       whose C runtime's remove takes files only, a directory is removed as
 *       POSIX removes one -- and a symbolic link or junction to a directory
 *       goes as a link.
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
 * @note    A symbolic link is a name like any other, removed without its
 *          target -- on Windows a link or junction to a directory too, which
 *          the C runtime's _unlink would refuse.
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
 *          copy and delete instead -- and that only for a regular file: a
 *          symlink or a directory still fails with EXDEV.
 * @warning With `_overwrite` 0 on POSIX the existence check is not atomic
 *          with the rename: another process can create `_new` in between.
 *          Any existing name counts, a dangling symlink included. Windows
 *          refuses atomically.
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
 * @note    With D_CFG_FILE_OPS_COPY_PRESERVE_MODE an existing destination
 *          takes the source's permission bits before any of its data, so a
 *          private source is never written into a file others can read.
 * @warning Not atomic: a reader sees the destination grow, and a crash leaves
 *          it partial. When that matters, copy to a sibling and d_file_rename
 *          it into place. A failed copy removes its partial destination.
 *
 * @param[in] _src  the source path; must name a regular file.
 * @param[in] _dst  the destination path; an existing file is replaced unless
 *                  D_CFG_FILE_OPS_COPY_OVERWRITE is 0.
 * @return 0, or -1 on failure with errno set: EINVAL when the source is not
 *         a regular file or is the destination itself under another name,
 *         which is left untouched; ENOSYS on the ISO C backend.
 */
int d_file_copy(const char* _src,
                const char* _dst);


D_EXTERN_C_END


#endif  // DJINTERP_C_FILESYS_FILE_OPS_H
