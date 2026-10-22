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
* path:      /inc/djinterp/c/filesys/file_path.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
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

#ifndef DJINTERP_C_FILESYS_FILE_PATH_H
#define DJINTERP_C_FILESYS_FILE_PATH_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "./file_common.h"                         // foundation, D_EXTERN_C_*
#include "../../config/c/filesys/cfg_file_path.h"  // module configuration


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
 *       "file.txt" and "" give ".", and "/" gives "/". A root is never cut:
 *       under Windows syntax "C:\\x" gives "C:\\", "C:x" gives "C:", and
 *       "\\\\server\\share\\x" gives the share.
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
 * @note A path that ends in a separator has an empty final name, and so no
 *       extension: "a/b.txt/" has none, as in std::filesystem -- a pointer
 *       into `_path` would carry the separator. And a root is no name, so
 *       under Windows syntax "C:.txt" names the hidden ".txt" and has none.
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
 *       stem, "archive.tar.gz" gives "archive.tar", and "a/b.txt/", with no
 *       extension, gives "b.txt".
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
 *       directory can be passed straight through. Otherwise the first loses
 *       its trailing separators, the second its leading ones, and
 *       D_FILE_PATH_OUT_SEP goes between -- unless the first is a root that
 *       needs none: "/" and "b" give "/b", and under Windows syntax "C:" and
 *       "b" give the drive-relative "C:b". A root is never split, so under
 *       POSIX syntax "//" and "b" give "//b". Only the junction changes; a
 *       "." or ".." in either component stays, for d_path_normalize.
 * @note With D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS (the default) a second
 *       component with a root decides how much of the first survives, as
 *       std::filesystem and Python's ntpath decide it. An absolute one
 *       replaces the first outright. Under Windows syntax one naming a drive
 *       ("D:x") replaces it too, unless the first is on that drive, when the
 *       rest joins on ("C:\\a" and "c:x" give "C:\\a\\x"); and a root
 *       directory alone keeps only the first's volume ("C:\\a" and "\\x"
 *       give "C:\\x"). Without the knob the two are joined as text.
 *
 * @param[out] _buf      receives the joined path.
 * @param[in]  _bufsize  size of `_buf`, in bytes.
 * @param[in]  _path1    the first component; may be `NULL` or empty.
 * @param[in]  _path2    the second component; may be `NULL` or empty.
 * @pre `_buf` may be `_path1` itself, to join onto a path in place, but must
 *      not overlap `_path2`.
 * @return `_buf`, or `NULL` on failure with errno set.
 */
char*       d_path_join(char*       _buf,
                        size_t      _bufsize,
                        const char* _path1,
                        const char* _path2);

// 1.3    Inspection
//------------------------------------------------------------------------------
// inspection -- NULL is a relative path with no root, not an error. Absolute
// means anchored, and under Windows syntax two roots are not: "C:x" is x in
// drive C's current directory, and "\\x" is x at the root of whichever drive
// is current. Only "C:\\x" and a share are absolute there; under POSIX syntax
// every root is. The root is what ".." may never climb above and what a join
// must never split, and d_path_root_length measures it: 1 for "/", 2 for "//"
// and "C:", 3 for "C:\\", the whole of "\\\\server\\share". Behind the
// long-path prefix the root is the prefix and what it opens: 7 for
// "\\\\?\\C:\\", the whole of "\\\\?\\UNC\\server\\share".
int         d_path_is_absolute(const char* _path);
size_t      d_path_root_length(const char* _path);

// 1.4    Canonicalization
//------------------------------------------------------------------------------
/**
 * @brief Cleans a path lexically: drops ".", resolves ".." against the
 *        component before it, collapses separator runs, drops a trailing
 *        separator, and writes D_FILE_PATH_OUT_SEP for every separator.
 *
 * @note The root stays as it is, with only its separators respelled, and
 *       ".." never climbs above it: "/.." is "/", as POSIX says. A ".." with
 *       nothing to climb over stays, so "../a" is already clean -- as does
 *       one after a bare drive, since "C:.." is the parent of drive C's
 *       current directory.
 * @note Under Windows syntax a relative result that would begin with a drive
 *       keeps "." and a separator ahead of it: ".\\D:" names a file in the
 *       current directory, where "D:" would name drive D's current directory.
 * @note D_CFG_FILE_PATH_COLLAPSE_SEPARATORS, _STRIP_TRAILING_SEP and
 *       _NORMALIZE_DOTDOT each turn their step off. However they are set, a
 *       result normalizes to itself, and an absolute path stays absolute.
 * @warning Lexical ".." matches the kernel only when no component is a
 *          symbolic link: given /x/link -> /y/z, this makes "/x/link/.." into
 *          "/x" where the kernel says "/y". For a path that exists, use
 *          d_path_resolve.
 *
 * @param[in]  _path     the path to normalize.
 * @param[out] _buf      receives the normalized path; "" normalizes to ".".
 *                       It may be `_path` itself, to normalize in place.
 * @param[in]  _bufsize  size of `_buf`, in bytes; at least 2, and enough for
 *                       `_path` and its terminator -- the result is never
 *                       longer, and a buffer that cannot hold the input is
 *                       refused with ERANGE.
 * @return `_buf`, or `NULL` on failure with errno set.
 */
char*       d_path_normalize(const char* _path,
                             char*       _buf,
                             size_t      _bufsize);


D_EXTERN_C_END


#endif  // DJINTERP_C_FILESYS_FILE_PATH_H
