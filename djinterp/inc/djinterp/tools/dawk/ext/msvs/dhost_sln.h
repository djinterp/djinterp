/*******************************************************************************
* djinterp [tools]                                                   dhost_sln.h
*
* The sln extension: Visual Studio's solutions, read, changed and made from
* awk.
*   A solution in either form -- a .slnx, which is XML, or a .sln, Visual
* Studio's older text -- is read the same: the projects it lists, each with
* the solution folder holding it and the projects it builds after; its
* folders; and the configurations it builds, each "Debug|x64". A program
* opens one, or makes one new, and is given its number, which every other
* function takes first.
*   The changes are the same for both forms, each made in the form's own
* way, at once, and kept only when the solution then reads whole: a project
* added or removed, a folder made, a dependency added or removed, a
* configuration added or removed. A .sln names each project by an id: one
* given, or the ProjectGuid of the project's file when it can be read, or
* one made from the project's path, the same every time. A project removed
* takes with it its configurations, its nesting, and every dependency on it.
*   A .slnx builds every build type on every platform it names, so there a
* configuration is removed by its build type, "Debug|*", or its platform,
* "*|x64"; a .sln takes either of those, or one configuration whole.
*   One of the two parts of the msvs extension, which adds vcxproj besides;
* each can be installed alone.
*
* path:      /inc/djinterp/tools/dawk/ext/msvs/dhost_sln.h
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
         1.  d_host_sln
2.  OPERATIONS
    ----------
    1.  The extension
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_MSVS_DHOST_SLN_H
#define DJINTERP_TOOLS_DAWK_EXT_MSVS_DHOST_SLN_H 1

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
// d_host_sln
//   struct: the extension's state behind the sln_* functions: the solutions
//   open and the last failure.  Layout is private.
struct d_host_sln;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    The extension
//------------------------------------------------------------------------------
// d_host_sln_install adds the sln_* functions to an interpreter, which then
// calls into the extension until it is freed; the extension outlives the
// interpreter.  The functions, as a program calls them; `s` is a
// solution's number, and `i` a project's or a folder's, from 1:
//   files
//     sln_open(path)               the solution's number, or -1
//     sln_new(path)                a solution not yet on disk, in the form
//                                  its path names, with no projects; its
//                                  number, or -1
//     sln_save(s[, path])          1 written, 2 the file held it already, or
//                                  -1; saved to `path`, it is that file from
//                                  then on
//     sln_close(s)                 1; unsaved changes are dropped
//     sln_error()                  why the last -1 was given
//     sln_path(s)                  its path
//     sln_form(s)                  slnx or sln
//     sln_changes(s)               changes made since it was opened or saved
//   reading
//     sln_projects(s)              how many projects it lists
//     sln_project(s, i, field)     project i's name, path, type, id, folder
//                                  -- "/tools/dawk/", or "" at the top -- or
//                                  line
//     sln_find(s, path)            the project with that path; 0 for none
//     sln_dependencies(s, i)       how many projects project i builds after
//     sln_dependency(s, i, k)      the path of the k-th
//     sln_folders(s)               how many folders it has
//     sln_folder(s, i)             folder i's path, "/tools/dawk/"
//     sln_configs(s)               how many configurations it builds
//     sln_config(s, i)             configuration i, "Debug|x64"
//   changes, each 1 or -1 unless said
//     sln_add_project(s, path[, folder[, id]])
//                                  the project, in the folder, which is made;
//                                  its number, or -1; a project there
//                                  already is its own number
//     sln_remove_project(s, path)  1, 0 for none, or -1
//     sln_add_folder(s, folder)    the folder, and each holding it
//     sln_add_dependency(s, path, on)
//                                  project `path` builds after `on`
//     sln_remove_dependency(s, path, on)
//                                  1, 0 for none, or -1
//     sln_add_config(s, config)    "Debug|x64", for every project
//     sln_remove_config(s, config) "Debug|x64", "Debug|*" or "*|x64"; 1, 0
//                                  for none, or -1
// Paths are compared as Visual Studio compares them: in any case, with `/`
// and `\` alike; a folder may be written "tools/dawk" or "/tools/dawk/".
// A number that names nothing reads as "" or 0, and changes nothing,
// giving -1.
struct d_host_sln* d_host_sln_new(void);
void               d_host_sln_free(struct d_host_sln* _host);
bool               d_host_sln_install(struct d_host_sln*   _host,
                                      struct d_awk_interp* _interp);


#endif  // DJINTERP_TOOLS_DAWK_EXT_MSVS_DHOST_SLN_H
