/*******************************************************************************
* djinterp [tools]                                                         dfs.h
*
* What the tools ask of a file system that ISO C cannot answer.
*   Three questions: what a path names, what a directory holds, and what the
* full name of a path is; and one thing to do: make a directory, and the
* directories above it. ISO C has a file and no directory, so each has a
* POSIX answer and a Windows one, and this is the one unit of the tools that
* holds both: every other unit is ISO C and asks here.
*   A path is bytes, in the encoding the system's narrow interface takes, with
* `/` between its parts. Windows takes `/` as it takes `\`, so a path built
* here reads the same on both.
*
* path:      /inc/djinterp/tools/dawk/dfs.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  Kinds
         1.  d_fs_kind
    2.  Directories
    3.  Paths
*/

#ifndef DJINTERP_TOOLS_DAWK_DFS_H
#define DJINTERP_TOOLS_DAWK_DFS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Kinds
//------------------------------------------------------------------------------
// 1.1.1
// d_fs_kind
//   enum: what a path names, as far as a walk over a tree needs to know.
enum d_fs_kind
{
    D_FS_KIND_NONE = 0,   // nothing that can be read
    D_FS_KIND_FILE,       // anything that is not a directory
    D_FS_KIND_DIRECTORY   // a directory, or a link to one
};

// 1.2    Directories
//------------------------------------------------------------------------------
/**
 * @brief Tells what a path names.
 *
 * @note A link is what it leads to. A separator at the end of the path
 *       changes nothing.
 *
 * @param[in] _path  the path.
 * @return `D_FS_KIND_DIRECTORY` for a directory, `D_FS_KIND_FILE` for
 *         anything else there is, and `D_FS_KIND_NONE` for a path that names
 *         nothing, cannot be read, or is `NULL`.
 */
enum d_fs_kind d_fs_kind_of(const char* _path);
/**
 * @brief Reads the names a directory holds.
 *
 * @note The names come in no order, and `.` and `..` are not among them.
 *
 * @param[in]  _directory  the directory to read.
 * @param[out] _names      the names, each allocated, in an allocated array;
 *                         `NULL` when the directory holds nothing.
 * @param[out] _count      how many names there are.
 * @post   The caller releases the names with `d_fs_names_free`.
 * @return `true` when the directory was read, whatever it holds; `false`
 *         when it could not be opened or memory ran out, with `*_names`
 *         `NULL`, `*_count` 0 and nothing left allocated.
 */
bool           d_fs_names(const char* _directory,
                          char***     _names,
                          size_t*     _count);
/**
 * @brief Releases what `d_fs_names` gave.
 *
 * @param[in] _names  the array; may be `NULL`.
 * @param[in] _count  how many names it holds.
 * @post Every name, and the array, is invalid.
 */
void           d_fs_names_free(char** _names,
                               size_t _count);
/**
 * @brief Makes a directory, and every directory it is in that is missing.
 *
 * @note A directory already there is left as it is. `/` and `\` both
 *       separate on Windows; on POSIX only `/` does.
 *
 * @param[in] _path  the directory.
 * @return `true` when the directory is there afterwards; `false` when it
 *         could not be made, or `_path` is `NULL` or empty.
 */
bool           d_fs_make_directory(const char* _path);
// d_fs_make_parent makes the directory a file's path names, as
// d_fs_make_directory does: everything before its last separator.
bool           d_fs_make_parent(const char* _path);

// 1.3    Paths
//------------------------------------------------------------------------------
/**
 * @brief Gives the full name of a path: from the root, with no `.` or `..`
 *        left in it.
 *
 * @note On POSIX every link in it is followed, so two spellings of one file
 *       give one name, and a path that names nothing has none. On Windows
 *       the name is made from the spelling alone: it is not looked up, so
 *       a path that names nothing still has one, and two spellings that
 *       differ only in case give two.
 *
 * @param[in] _path  the path.
 * @post   The caller releases the name with `free`.
 * @return the name, allocated; `NULL` when the path has none or memory ran
 *         out.
 */
char*          d_fs_full_path(const char* _path);
/**
 * @brief Writes a path the one way the tools read one, with `/` between
 *        its parts.
 *
 * @note On Windows every `\` becomes `/`, which the system takes as well. On
 *       POSIX the path is left as it is: a `\` there is a character of a
 *       name.
 *
 * @param[in,out] _path  the path, rewritten in place; may be `NULL`.
 */
void           d_fs_path_normalize(char* _path);


#endif  // DJINTERP_TOOLS_DAWK_DFS_H
