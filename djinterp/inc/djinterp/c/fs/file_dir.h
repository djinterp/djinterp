/*******************************************************************************
* djinterp [c]                                                        file_dir.h
*
* Directories -- creating, removing, walking -- and the path operations that
* must ask the filesystem.
*   The split from file_path is by cost, not by name: d_path_dirname is
* lexical and free, d_path_resolve is a syscall that resolves symlinks and can
* fail. Both are "path" work; only one of them needs a disk.
*   struct d_dir_t is opaque and owns the entry d_dir_read returns, so that
* pointer stays valid until the next d_dir_read on the same handle -- and dies
* with d_dir_close.
*
*
* path:      /inc/djinterp/c/fs/file_dir.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DIRECTORIES
    -----------
    1.  Creation
    2.  Removal
    3.  Traversal
    4.  Working directory and resolution
*/

#ifndef DJINTERP_C_FS_FILE_DIR_H
#define DJINTERP_C_FS_FILE_DIR_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "./file_common.h"                   // fs foundation, d_dir_t
#include "./file_path.h"                     // the lexical path operations
#include "../../config/c/fs/cfg_file_dir.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  DIRECTORIES
//==============================================================================


// 1.1    Creation
//------------------------------------------------------------------------------
/**
 * @brief Creates a single directory.
 *
 * @param[in] _path  the path to create; every parent must already exist.
 * @param[in] _mode  permission bits, reduced by the process umask on POSIX;
 *                   ignored on Windows, which has none for directories.
 * @return 0, or -1 on failure with errno set: ENOENT for a missing parent,
 *         EEXIST for an existing target.
 */
int                d_dir_create(const char* _path,
                                uint32_t    _mode);
/**
 * @brief Creates a directory and any missing parents (mkdir -p).
 *
 * @note An existing directory satisfies the request unless
 *       D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK is 0. Parents are created with
 *       D_CFG_FILE_DIR_CREATE_MODE, not `_mode`, and a parent that another
 *       process creates concurrently is not an error.
 *
 * @param[in] _path  the path to create.
 * @param[in] _mode  permission bits for the final component only.
 * @return 0, or -1 on failure with errno set; ENOTDIR when a component exists
 *         and is not a directory.
 */
int                d_dir_create_parents(const char* _path,
                                        uint32_t    _mode);

// 1.2    Removal
//------------------------------------------------------------------------------
/**
 * @brief Removes an empty directory.
 *
 * @note There is deliberately no recursive form: deleting a tree is a policy
 *       decision with a symlink-traversal hazard attached, and it does not
 *       belong behind a name this innocent.
 *
 * @param[in] _path  the directory to remove.
 * @return 0, or -1 on failure with errno set; ENOTEMPTY (EEXIST on some
 *         platforms) for a directory that is not empty.
 */
int                d_dir_remove(const char* _path);

// 1.3    Traversal
//------------------------------------------------------------------------------
/**
 * @brief Opens a directory for reading.
 *
 * @param[in] _path  the directory to open.
 * @return a handle, or `NULL` on failure with errno set.
 * @post The caller owns the handle and releases it with d_dir_close.
 */
struct d_dir_t*    d_dir_open(const char* _path);
/**
 * @brief Reads the next entry from a directory.
 *
 * @note    Entries arrive in whatever order the filesystem stores them. "."
 *          and ".." are skipped when D_CFG_FILE_DIR_SKIP_DOTS is set, and an
 *          entry whose name exceeds D_FILE_NAME_MAX is skipped with a warning.
 * @warning `NULL` means both end of directory and failure; set errno to 0
 *          before the call to tell them apart.
 *
 * @param[in] _dir  an open handle.
 * @return the next entry, or `NULL` at the end or on failure.
 * @post The entry belongs to `_dir`: it stays valid until the next d_dir_read
 *       on the same handle, and dies with d_dir_close.
 */
struct d_dirent_t* d_dir_read(struct d_dir_t* _dir);
/**
 * @brief Returns a directory handle to its first entry.
 *
 * @note Unlike rewinddir this reports failure: Win32 has no rewind, so the
 *       search is closed and reopened, and that can fail.
 *
 * @param[in] _dir  an open handle.
 * @return 0, or -1 on failure with errno set.
 */
int                d_dir_rewind(struct d_dir_t* _dir);
/**
 * @brief Closes a directory handle and releases it.
 *
 * @param[in] _dir  the handle to close.
 * @return 0, or -1 on failure with errno set.
 * @post `_dir`, and the entry last read from it, are invalid whatever the
 *       result.
 */
int                d_dir_close(struct d_dir_t* _dir);

// 1.4    Working directory and resolution
//------------------------------------------------------------------------------
/**
 * @brief Retrieves the current working directory.
 *
 * @note The process has ONE working directory, shared by every thread, so the
 *       answer can be stale before it returns. There is deliberately no
 *       allocating form.
 *
 * @param[out] _buf   receives the path.
 * @param[in]  _size  size of `_buf`, in bytes.
 * @return `_buf`, or `NULL` on failure with errno set; ERANGE when the path
 *         does not fit.
 */
char*              d_dir_get_current(char*  _buf,
                                     size_t _size);
/**
 * @brief Changes the current working directory.
 *
 * @warning Process-wide and shared by every thread: another thread's relative
 *          paths resolve against whatever this last set. Prefer absolute
 *          paths.
 *
 * @param[in] _path  the directory to move to.
 * @return 0, or -1 on failure with errno set.
 */
int                d_dir_set_current(const char* _path);
/**
 * @brief Resolves a path to its canonical absolute form, following every
 *        symbolic link on the way.
 *
 * @note Unlike d_path_normalize this asks the filesystem, so every component
 *       must EXIST; a path about to be created cannot be resolved here.
 *
 * @param[in]  _path      the path to resolve.
 * @param[out] _resolved  a buffer of at least D_FILE_PATH_MAX bytes, or
 *                        `NULL` to have the result allocated when
 *                        D_CFG_FILE_DIR_REALPATH_ALLOC is set.
 * @return the resolved path, or `NULL` on failure with errno set; ENOSYS
 *         where the platform has no resolver.
 * @post An allocated result belongs to the caller, who releases it with the
 *       configured deallocator (D_CFG_FILE_FREE).
 */
char*              d_path_resolve(const char* _path,
                                  char*       _resolved);


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FS_FILE_DIR_H
