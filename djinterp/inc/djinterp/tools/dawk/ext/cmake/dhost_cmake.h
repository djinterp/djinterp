/*******************************************************************************
* djinterp [tools]                                                 dhost_cmake.h
*
* The CMake extension: CMake files, read and changed from awk.
*   A file is opened from disk, or made from text, and named by a handle, a
* number from 1. What a file holds is counted and read by index, from 1: its
* commands, each with its name, its place, the block it opens, branches or
* closes, and its arguments, each as written, as CMake reads it, and as the
* items CMake passes for it; and its comments. A change is kept only if the
* file outside it reads as it did and the change reads back as asked: an
* argument set, inserted or removed, a command renamed or replaced, commands
* inserted or removed. Every byte a change does not touch -- comments,
* spacing, line endings -- is written back as it was. A change refused gives
* -1, and cmake_error() says why. Indices are into the file as it is now: a
* change that adds or removes commands or arguments moves those after it.
*   The writers make the text a file holds: a value as an argument, quoted,
* bracketed or as plain as it can be, and a whole command of values; and the
* other way, the value of an argument's text, and the items of a list as
* CMake divides one.
*
* path:      /inc/djinterp/tools/dawk/ext/cmake/dhost_cmake.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Extension
         1.  d_host_cmake
2.  OPERATIONS
    ----------
    1.  The extension
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_CMAKE_DHOST_CMAKE_H
#define DJINTERP_TOOLS_DAWK_EXT_CMAKE_DHOST_CMAKE_H 1

// std
#include <stdbool.h>        // bool
// djinterp
#include "../../dinterp.h"  // d_awk_interp


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Extension
//------------------------------------------------------------------------------
// 1.1.1
// d_host_cmake
//   struct: the files open, and the last failure of the cmake_* functions.
//   Layout is private.
struct d_host_cmake;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    The extension
//------------------------------------------------------------------------------
// d_host_cmake_install adds these, as a program calls them; f is a file's
// handle, n a command's index, k an argument's, i a comment's, all from 1:
//
// files
//   cmake_open(path)               a file from disk: f, or -1
//   cmake_new(path[, text])        a file not yet on disk, of the text
//   cmake_save(f[, path])          1 written, 0 unchanged, -1 failed
//   cmake_close(f)                 1, or 0 for no file
//   cmake_error()                  why the last failure failed
//   cmake_path(f), cmake_text(f), cmake_changes(f)
//   cmake_generated(f)             1 for a file CMake or a generator wrote
// commands
//   cmake_commands(f)              how many
//   cmake_command(f, n, field)     name, line, column, end_line, depth,
//                                  block -- if ... block, or "" -- role --
//                                  open, branch, close, or "" -- partner --
//                                  an opener's closer, a branch's or a
//                                  closer's opener, 0 for none -- unpaired,
//                                  args, text, indent
//   cmake_name(f, n), cmake_args(f, n)
//                                  its name and how many arguments it has
//   cmake_find(f, name[, after])   the first command after `after` so
//                                  named, as CMake compares names; 0 for none
// arguments
//   cmake_arg(f, n, k[, field])    text, as written, by default; value, as
//                                  CMake reads it; items, the arguments
//                                  CMake passes for it, a line each; kind --
//                                  unquoted, quoted, bracket -- line,
//                                  column, end_line, legacy, paren
//   cmake_value(f, n, k)           its value
//   cmake_find_arg(f, n, value[, after])
//                                  the first argument after `after` of that
//                                  value; 0 for none
// comments
//   cmake_comments(f); cmake_comment(f, i, field)
//                                  text, body -- what it says, past its `#`
//                                  or within its brackets -- line, column,
//                                  end_line, bracket, trailing, command --
//                                  the one whose arguments hold it, or 0
// changes, each 1 kept, or -1
//   cmake_set_arg(f, n, k, text)   text as written, one argument
//   cmake_insert_arg(f, n, k, text)
//                                  before argument k, in its layout; k one
//                                  past the last appends after the last
//   cmake_insert_after(f, n, k, text)
//                                  after argument k, in its layout
//   cmake_remove_arg(f, n, k)      with its line, when it stood alone there
//   cmake_set_name(f, n, name)     the part it takes in its block kept
//   cmake_set_command(f, n, text)  one command for another, likewise
//   cmake_insert_command(f, n, text)
//                                  commands on lines of their own before
//                                  command n and the comments leading into
//                                  it; n one past the last appends; each
//                                  indented as its block's commands are
//   cmake_remove_command(f, n[, last])
//                                  commands n to `last`, n by default
// writing
//   cmake_quote(value)             a quoted argument
//   cmake_bracket(value)           a bracket argument, or -1
//   cmake_argument(value)          unquoted where it can be, else quoted
//   cmake_call(name[, value ...])  a command of up to 15 values, each as
//                                  cmake_argument writes it
//   cmake_unquote(text)            the value of a text read as one argument,
//                                  or -1
//   cmake_split(list[, keep])      the items of a list as CMake divides it,
//                                  a line each; empty ones only when kept
struct d_host_cmake* d_host_cmake_new(void);
void                 d_host_cmake_free(struct d_host_cmake* _host);
bool                 d_host_cmake_install(struct d_host_cmake* _host,
                                          struct d_awk_interp* _interp);


#endif  // DJINTERP_TOOLS_DAWK_EXT_CMAKE_DHOST_CMAKE_H
