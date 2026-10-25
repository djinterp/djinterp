/*******************************************************************************
* djinterp [tools]                                                  dhost_msvs.h
*
* The msvs extension: Visual Studio's files, all of them, from awk.
*   An umbrella over two parts: sln, for solutions, and vcxproj, for MSBuild's
* files; installing msvs installs both, and what a file generator needs
* besides -- a GUID made from a name, a path relative to a directory, the
* files a pattern names, and what a path is. A program that only reads
* solutions can install sln alone, and one that only edits projects,
* vcxproj alone.
*
* path:      /inc/djinterp/tools/dawk/ext/msvs/dhost_msvs.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Extension
         1.  d_host_msvs
2.  OPERATIONS
    ----------
    1.  The extension
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_MSVS_DHOST_MSVS_H
#define DJINTERP_TOOLS_DAWK_EXT_MSVS_DHOST_MSVS_H 1

// std
#include <stdbool.h>     // bool
// djinterp
#include "../../dinterp.h"  // d_awk_interp


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Extension
//------------------------------------------------------------------------------
// 1.1.1
// d_host_msvs
//   struct: the two parts, and the last failure of the msvs_* functions.
//   Layout is private.
struct d_host_msvs;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    The extension
//------------------------------------------------------------------------------
// d_host_msvs_install adds every sln_* and vcxproj_* function -- see
// dhost_sln.h and dhost_vcxproj.h -- and these, as a program calls them:
//   msvs_guid(name)              a GUID made from the name, the same every
//                                time: lower case, without braces
//   msvs_relative(directory, path[, separator])
//                                the path as read from the directory, both
//                                from one place; `\` between its parts,
//                                or the separator given
//   msvs_glob(pattern[, which])  the paths the pattern names, `*`, `?` and
//                                [...] matching within a part, sorted and
//                                one to a line; files, or "dirs", or "all"
//   msvs_exists(path)            1 for a file, 2 for a directory, 0 for
//                                nothing
//   msvs_kind(path)              what Visual Studio file its name says it
//                                is: solution, project, filters, user,
//                                props or other
struct d_host_msvs* d_host_msvs_new(void);
void                d_host_msvs_free(struct d_host_msvs* _host);
bool                d_host_msvs_install(struct d_host_msvs*  _host,
                                        struct d_awk_interp* _interp);


#endif  // DJINTERP_TOOLS_DAWK_EXT_MSVS_DHOST_MSVS_H
