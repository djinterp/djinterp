/*******************************************************************************
* djinterp [c]                                                        file_dir.h
*
* Directories -- creating, removing, walking -- and the path operations that
* must ask the filesystem.
*   The split from file_path is by cost, not by name: d_path_dirname is
* lexical and free, d_path_resolve is a syscall that resolves symlinks and can
* fail. Both are "path" work; only one of them needs a disk.
*   struct d_dir_t is opaque and owns the entry d_dir_read returns, so that
* pointer stays valid until the next read on the same handle -- and dies with
* d_dir_close.
*   A build with neither POSIX nor Windows has no directory API -- ISO C has
* none -- so there every call that needs one fails with ENOSYS; only
* d_dir_entry_path, which is text, still works.
*   One handle reads at either width: d_dir_read64 returns a d_dirent64_t,
* d_dir_read32 a d_dirent32_t, and the unsuffixed d_dir_read follows
* D_FILE_WIDTH (file_common.h has the rules). Only the inode number differs,
* so the one way d_dir_read32 can fail where d_dir_read64 would not is an
* entry whose inode number needs more than 32 bits: that entry fails with
* EOVERFLOW, and the next read continues past it.
*   d_dir_read_name has no width at all. It hands back the name and the type
* and no inode number, so it reads every entry d_dir_read64 would, at either
* width and below the 64-bit floor too -- the reader for a walk that does not
* need the inode number, which is most of them.
*   d_dir_entry_path builds the path of an entry a walk has read, for the
* running platform to open. A walk descends with it rather than d_path_join,
* whose grammar follows D_CFG_FILE_PATH_SYNTAX and so may be the other
* platform's.
*
*
* path:      /inc/djinterp/c/filesys/file_dir.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.19
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
2.  DEFAULT WIDTH
    -------------
    1.  Unsuffixed names
*/

#ifndef DJINTERP_C_FILESYS_FILE_DIR_H
#define DJINTERP_C_FILESYS_FILE_DIR_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "./file_common.h"                        // foundation, d_dir_t,
                                                  // d_dirent32_t, d_dirent64_t
#include "./file_path.h"                          // the lexical path operations
#include "../../config/c/filesys/cfg_file_dir.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint8_t, uint32_t


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
int                  d_dir_create(const char* _path,
                                  uint32_t    _mode);
/**
 * @brief Creates a directory and any missing parents (mkdir -p).
 *
 * @note An existing directory satisfies the request unless
 *       D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK is 0. Parents are created with
 *       D_CFG_FILE_DIR_CREATE_MODE, not `_mode`, and a parent that another
 *       process creates concurrently is not an error.
 * @note Trailing separators are ignored, so "a/b/" makes "a/b" with `_mode`.
 *       The root is never made: a path that is nothing but one ("/", "C:\\")
 *       succeeds when it is there and fails with ENOENT when it is not.
 *
 * @param[in] _path  the path to create.
 * @param[in] _mode  permission bits for the final component only.
 * @return 0, or -1 on failure with errno set; ENOTDIR when a component exists
 *         and is not a directory.
 */
int                  d_dir_create_parents(const char* _path,
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
int                  d_dir_remove(const char* _path);

// 1.3    Traversal
//------------------------------------------------------------------------------
/**
 * @brief Opens a directory for reading, at either width.
 *
 * @param[in] _path  the directory to open.
 * @return a handle, or `NULL` on failure with errno set: ENOENT for a missing
 *         directory or an empty path, ENOTDIR for a path that is not a
 *         directory.
 * @post The caller owns the handle and releases it with d_dir_close.
 */
struct d_dir_t*      d_dir_open(const char* _path);
#if D_FILE_64_IS_AVAILABLE
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
 * @post The entry belongs to `_dir`: it stays valid until the next read on
 *       the same handle, at either width, and dies with d_dir_close.
 */
struct d_dirent64_t* d_dir_read64(struct d_dir_t* _dir);
#endif  // D_FILE_64_IS_AVAILABLE
/**
 * @brief d_dir_read64, into the 32-bit layout.
 *
 * @warning `NULL` with errno at EOVERFLOW is one entry whose inode number does
 *          not fit, not the end: the next call reads on past it.
 *
 * @param[in] _dir  an open handle.
 * @return the next entry, or `NULL` at the end or on failure.
 * @post The entry belongs to `_dir`, as d_dir_read64's does.
 */
struct d_dirent32_t* d_dir_read32(struct d_dir_t* _dir);
/**
 * @brief Reads the next entry's name and type, at no width: with no inode
 *        number in the answer there is nothing that can fail to fit.
 *
 * @warning `NULL` means both end of directory and failure; set errno to 0
 *          before the call to tell them apart.
 *
 * @param[in]  _dir   an open handle.
 * @param[out] _type  receives the entry's DT_* type; may be `NULL`.
 * @return the entry's name, or `NULL` at the end or on failure.
 * @post The name belongs to `_dir`, as an entry d_dir_read64 returns does.
 */
const char*          d_dir_read_name(struct d_dir_t* _dir,
                                     uint8_t*        _type);
/**
 * @brief Builds the path of an entry read from a directory, for the running
 *        platform to open: the directory, D_FILE_PATH_SEP, then the name.
 *
 * @note No separator follows a directory that ends in one already, and on
 *       Windows none follows a bare drive: "C:" and "x" give "C:x", x in
 *       drive C's current directory. An empty directory gives the name.
 * @note Not d_path_join, which reads the name in file_path's grammar: a
 *       POSIX build set to Windows syntax would take an entry named "C:x"
 *       for a drive and drop the directory. The name here is never parsed.
 *
 * @param[out] _buf      receives the entry's path.
 * @param[in]  _bufsize  size of `_buf`, in bytes.
 * @param[in]  _dir      the directory the entry was read from.
 * @param[in]  _name     the entry's name.
 * @pre `_buf` may be `_dir` itself, but must not overlap `_name`.
 * @return `_buf`, or `NULL` on failure with errno set: EINVAL for a bad
 *         argument, ERANGE when the path does not fit.
 */
char*                d_dir_entry_path(char*       _buf,
                                      size_t      _bufsize,
                                      const char* _dir,
                                      const char* _name);
/**
 * @brief Returns a directory handle to its first entry.
 *
 * @note Unlike rewinddir this reports failure: Win32 has no rewind, so the
 *       search is closed and reopened, and that can fail.
 *
 * @param[in] _dir  an open handle.
 * @return 0, or -1 on failure with errno set.
 */
int                  d_dir_rewind(struct d_dir_t* _dir);
/**
 * @brief Closes a directory handle and releases it.
 *
 * @param[in] _dir  the handle to close.
 * @return 0, or -1 on failure with errno set.
 * @post `_dir`, and the entry last read from it, are invalid whatever the
 *       result.
 */
int                  d_dir_close(struct d_dir_t* _dir);

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
char*                d_dir_get_current(char*  _buf,
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
int                  d_dir_set_current(const char* _path);
/**
 * @brief Resolves a path to its canonical absolute form, following every
 *        symbolic link on the way.
 *
 * @note Unlike d_path_normalize this asks the filesystem, so every component
 *       must EXIST; a path about to be created cannot be resolved here.
 *
 * @param[in]  _path      the path to resolve.
 * @param[out] _resolved  receives the path; or `NULL` to have it returned in
 *                        a block allocated to fit, when
 *                        D_CFG_FILE_DIR_REALPATH_ALLOC is set.
 * @param[in]  _size      size of `_resolved`, in bytes; ignored when
 *                        `_resolved` is `NULL`.
 * @return `_resolved` or the allocated block, or `NULL` on failure with
 *         errno set; ERANGE when the path does not fit `_size` bytes,
 *         ENAMETOOLONG when it is longer than D_FILE_PATH_MAX allows, and
 *         ENOSYS where the platform has no resolver.
 * @post An allocated result belongs to the caller, who releases it with the
 *       configured deallocator (D_CFG_FILE_FREE).
 */
char*                d_path_resolve(const char* _path,
                                    char*       _resolved,
                                    size_t      _size);


//==============================================================================
// 2.  DEFAULT WIDTH
//==============================================================================


// 2.1    Unsuffixed names
//------------------------------------------------------------------------------
// d_dir_read
//   alias: the reader's twin at D_FILE_WIDTH, so it returns a d_dirent_t
// (file_common.h, 3.2). Opening, rewinding and closing have one width.
#if (D_FILE_WIDTH == 64)
    #define d_dir_read d_dir_read64
#else
    #define d_dir_read d_dir_read32
#endif  // D_FILE_WIDTH == 64


D_EXTERN_C_END


#endif  // DJINTERP_C_FILESYS_FILE_DIR_H
