/*******************************************************************************
* djinterp [tools]                                               dhost_vcxproj.h
*
* The vcxproj extension: MSBuild's files, read, changed and made from awk.
*   A .vcxproj, its .vcxproj.filters and .vcxproj.user, a .props or a
* .targets: XML whose root is <Project>. A program opens one, or makes one
* new -- from nothing but its root, or from a template file -- and is given
* its root's handle; every element it reaches after has a handle of its own,
* a number that names that element, in that file, for as long as the file
* is open and the element is in it. A handle is never 0, and never names a
* second element: 0 is what a search that finds nothing gives.
*   Every change is made at once and read again, and kept only when the
* file then reads whole: a program cannot leave a file Visual Studio would
* refuse. Changes touch only the bytes they must, so a file saved is the
* file opened and the changes, its line endings and indenting kept. A value
* given is decoded text, escaped for XML as it is written; a value read is
* decoded the same way.
*   MSBuild's own reading: a Condition is understood when it tests the
* configuration or the platform, and an element applies to a configuration
* when nothing holding it says it does not. Properties are their text,
* $(...) unexpanded; imports are not followed.
*   One of the two parts of the msvs extension, which adds sln besides; each
* can be installed alone.
*
* path:      /inc/djinterp/tools/dawk/ext/msvs/dhost_vcxproj.h
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
         1.  d_host_vcxproj
2.  OPERATIONS
    ----------
    1.  The extension
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_MSVS_DHOST_VCXPROJ_H
#define DJINTERP_TOOLS_DAWK_EXT_MSVS_DHOST_VCXPROJ_H 1

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
// d_host_vcxproj
//   struct: the extension's state behind the vcxproj_* functions: the files
//   open, the handles given, the last failure.  Layout is private.
struct d_host_vcxproj;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    The extension
//------------------------------------------------------------------------------
// d_host_vcxproj_install adds the vcxproj_* functions to an interpreter,
// which then calls into the extension until it is freed; the extension
// outlives the interpreter.  The functions, as a program calls them; `e` is
// an element's handle, and any handle of a file names the file:
//   files
//     vcxproj_open(path)            the root's handle, or -1
//     vcxproj_new(path[, template]) a file not yet on disk, its kind its
//                                   path's: the template's text, or only a
//                                   root; the root's handle, or -1
//     vcxproj_save(e[, path])       1 written, 2 the file held it already,
//                                   or -1; saved to `path`, it is that file
//                                   from then on
//     vcxproj_close(e)              1; unsaved changes are dropped
//     vcxproj_error()               why the last -1 was given
//     vcxproj_path(e)               the file's path
//     vcxproj_kind(e)               project, filters, user, props or other
//     vcxproj_changes(e)            changes made since it was opened or saved
//   elements
//     vcxproj_root(e)               the root
//     vcxproj_parent(e)             the parent; 0 for the root
//     vcxproj_first(e)              the first child; 0 for none
//     vcxproj_next(e)               the next sibling; 0 for none
//     vcxproj_children(e)           how many children it has
//     vcxproj_find(e, name[, after])
//                                   the first element within `e` so named
//                                   -- any, for "" -- after `after`
//     vcxproj_name(e)               its name
//     vcxproj_line(e)               the line it starts on
//     vcxproj_depth(e)              how many elements hold it
//     vcxproj_attr(e, name)         an attribute's value; "" when absent
//     vcxproj_has(e, name)          1 when it has the attribute
//     vcxproj_text(e)               its text; "" when it holds elements
//   MSBuild
//     vcxproj_condition(e)          what its own Condition tests: "" for
//                                   none, "Debug|x64", "Debug|*" or "*|x64",
//                                   or "?" for anything else
//     vcxproj_applies(e, config)    1 when it applies to a configuration,
//                                   "Debug|x64"; 0 when something holding
//                                   it says not; 2 when nothing says not
//                                   but some Condition is not understood
//     vcxproj_property(e, name[, config])
//                                   the last value a property group gives
//                                   the property, for the configuration or,
//                                   with none, unconditionally; "" for none
//     vcxproj_item(e, type, include)
//                                   the item of that type, ClCompile or
//                                   ClInclude, naming that file; 0 for none
//     vcxproj_add_item(e, type, include[, filter])
//                                   the item, added beside its kind, in
//                                   order, unless it is there; in a filters
//                                   file, in `filter`, which is made, with
//                                   each filter holding it; its handle, or
//                                   -1
//     vcxproj_remove_item(e, type, include)
//                                   1, 0 for none, or -1
//   changes
//     vcxproj_add(e, name[, text[, attr, value]...])
//                                   a child after the last; its handle, or
//                                   -1
//     vcxproj_insert(e, name[, text[, attr, value]...])
//                                   a sibling before `e`; its handle, or -1
//     vcxproj_set_attr(e, name, value)
//                                   1, or -1
//     vcxproj_remove_attr(e, name)  1, 0 when absent, or -1
//     vcxproj_set_text(e, text)     1, or -1
//     vcxproj_remove(e)             1, or -1; the handles within it name
//                                   nothing after
// Files are compared by name as MSBuild compares them: in any case, with
// `/` and `\` alike.  A handle that names nothing reads as "" or 0, and
// changes nothing, giving -1.
struct d_host_vcxproj* d_host_vcxproj_new(void);
void                   d_host_vcxproj_free(struct d_host_vcxproj* _host);
bool                   d_host_vcxproj_install(struct d_host_vcxproj* _host,
                                              struct d_awk_interp*   _interp);


#endif  // DJINTERP_TOOLS_DAWK_EXT_MSVS_DHOST_VCXPROJ_H
