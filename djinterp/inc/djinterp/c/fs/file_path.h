/*******************************************************************************
* djinterp [c]                                                       file_path.h
*
* Lexical path manipulation -- what a path SAYS, never what is on disk.
*   Nothing in this module issues a system call. d_path_dirname("/nowhere/x")
* is "/nowhere" whether or not that directory exists, and d_path_normalize
* does not resolve symlinks because it cannot see them. That is a feature: it
* makes the whole module testable on any machine with no filesystem, no
* permissions and no temp directory, and it makes it the natural bottom for a
* C++ path type whose lexical operations are specified the same way.
*   The counterpart -- the operations that must ask the filesystem -- are
* d_dir_get_current / d_dir_set_current / d_path_resolve in file_dir.h.
*
*
* path:      /inc/djinterp/c/fs/file_path.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PATHS
    -----
    1.  Decomposition
    2.  Composition
    3.  Inspection
    4.  Canonicalization
*/

#ifndef DJINTERP_C_FS_FILE_PATH_H
#define DJINTERP_C_FS_FILE_PATH_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "./file_common.h"                    // fs foundation, D_EXTERN_C_*
#include "../../config/c/fs/cfg_file_path.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  PATHS
//==============================================================================
// Every function writing into a caller's buffer returns that buffer, or NULL
// with errno set: EINVAL for a bad argument, ERANGE when the result does not
// fit. A path is never truncated to fit -- a prefix of a path is how a program
// deletes the wrong directory.


// 1.1    Decomposition
//------------------------------------------------------------------------------
/**
 * @brief Extracts the directory component of a path, lexically.
 *
 * @note Trailing separators are ignored, so "a/b/" and "a/b" both give "a".
 *       "/path/to/file.txt" gives "/path/to", "/file.txt" gives "/",
 *       "file.txt" and "" give ".", and "/" gives "/".
 *
 * @param[in]  _path     the path to decompose.
 * @param[out] _buf      receives the directory component.
 * @param[in]  _bufsize  size of `_buf`, in bytes.
 * @return `_buf`, or `NULL` on failure with errno set.
 */
char*       d_path_dirname(const char* _path,
                           char*       _buf,
                           size_t      _bufsize);
/**
 * @brief Extracts the final component of a path.
 *
 * @note Trailing separators are ignored, as the shell's basename does: "a/b/"
 *       gives "b", "/" gives "/", and "" gives "".
 *
 * @param[in]  _path     the path to decompose.
 * @param[out] _buf      receives the final component.
 * @param[in]  _bufsize  size of `_buf`, in bytes.
 * @return `_buf`, or `NULL` on failure with errno set.
 */
char*       d_path_basename(const char* _path,
                            char*       _buf,
                            size_t      _bufsize);
/**
 * @brief Finds the extension of a path's final component, dot included.
 *
 * @note A leading dot does not start an extension (".bashrc" has none), and a
 *       dot in a parent directory does not count ("/a.d/file" has none).
 *       "archive.tar.gz" gives ".gz", and "file." gives ".".
 *
 * @param[in] _path  the path to inspect.
 * @return a pointer INTO `_path` at the dot, living exactly as long as `_path`
 *         does, or `NULL` when there is no extension.
 */
const char* d_path_extension(const char* _path);
/**
 * @brief Extracts the final component of a path with its extension removed.
 *
 * @note It agrees with d_path_extension by construction -- stem plus
 *       extension is the basename for every input -- so ".bashrc" is its own
 *       stem and "archive.tar.gz" gives "archive.tar".
 *
 * @param[in]  _path     the path to decompose.
 * @param[out] _buf      receives the stem.
 * @param[in]  _bufsize  size of `_buf`, in bytes.
 * @return `_buf`, or `NULL` on failure with errno set.
 */
char*       d_path_stem(const char* _path,
                        char*       _buf,
                        size_t      _bufsize);

// 1.2    Composition
//------------------------------------------------------------------------------
/**
 * @brief Joins two path components with exactly one separator between them.
 *
 * @note A `NULL` or empty component is skipped, so an optional base
 *       directory can be passed straight through. With
 *       D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS (the default) an absolute second
 *       component replaces the first, as in every other path library.
 *
 * @param[out] _buf      receives the joined path.
 * @param[in]  _bufsize  size of `_buf`, in bytes.
 * @param[in]  _path1    the first component; may be `NULL` or empty.
 * @param[in]  _path2    the second component; may be `NULL` or empty.
 * @return `_buf`, or `NULL` on failure with errno set.
 */
char*       d_path_join(char*       _buf,
                        size_t      _bufsize,
                        const char* _path1,
                        const char* _path2);

// 1.3    Inspection
//------------------------------------------------------------------------------
// inspection -- NULL is a relative path with no root, not an error. "C:x" is
// drive-relative, NOT absolute: it is x relative to drive C's current
// directory, and only "C:\x" is anchored. The root is what ".." may never
// climb above and what a join must never split.
int         d_path_is_absolute(const char* _path);
size_t      d_path_root_length(const char* _path);

// 1.4    Canonicalization
//------------------------------------------------------------------------------
/**
 * @brief Cleans a path lexically: collapses separator runs, drops ".",
 *        resolves ".." against the preceding component, and emits this
 *        build's separator.
 *
 * @warning Lexical ".." matches the kernel only when no component is a
 *          symbolic link: given /x/link -> /y/z, this makes "/x/link/.." into
 *          "/x" where the kernel says "/y". For a path that exists, use
 *          d_path_resolve.
 *
 * @param[in]  _path     the path to normalize.
 * @param[out] _buf      receives the normalized path; "" normalizes to ".".
 * @param[in]  _bufsize  size of `_buf`, in bytes; at least 2.
 * @return `_buf`, or `NULL` on failure with errno set.
 */
char*       d_path_normalize(const char* _path,
                             char*       _buf,
                             size_t      _bufsize);


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FS_FILE_PATH_H
