/*******************************************************************************
* djinterp [tools]                                                 dhost_ninja.h
*
* The Ninja extension: Ninja's files, read and changed from awk, and Ninja
* builds loaded as ninja loads them.
*   A file is opened from disk, or made from text, and named by a handle, a
* number from 1. What it holds is counted and read by index, from 1: its
* statements, each with its kind, its name, its paths in their sections and
* its bindings; and its comments. A change is kept only if the file outside
* it reads as it did and the change reads back as asked; a change refused
* gives -1, and ninja_error() says why. Indices are into the file as it is
* now: a change that adds or removes records moves those after it.
*   A build is loaded from its manifest, with what it includes, as ninja
* loads one, and named by a handle of its own: its edges, its nodes, its
* defaults and roots, its rules, pools and variables, each binding of an
* edge evaluated as ninja evaluates it. What ninja would refuse to load is
* refused, in ninja's words, which ninja_error() gives.
*   The writers make the text a file holds: a path or a value escaped as it
* must be, a path made canonical, and a path as ninja writes it into a
* command.
*
* path:      /inc/djinterp/tools/dawk/ext/ninja/dhost_ninja.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Extension
         1.  d_host_ninja
2.  OPERATIONS
    ----------
    1.  The extension
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_NINJA_DHOST_NINJA_H
#define DJINTERP_TOOLS_DAWK_EXT_NINJA_DHOST_NINJA_H 1

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
// d_host_ninja
//   struct: the files open, the builds loaded, and the last failure of the
//   ninja_* functions.  Layout is private.
struct d_host_ninja;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    The extension
//------------------------------------------------------------------------------
// d_host_ninja_install adds these, as a program calls them; f is a file's
// handle, g a build's, n a statement's index, k a path's or a binding's, e
// an edge's, all from 1; a section is output, implicit_output, input,
// implicit, order_only, validation, target or file:
//
// files
//   ninja_open(path)                a file from disk: f, or -1
//   ninja_new(path[, text])         a file not yet on disk, of the text
//   ninja_save(f[, path])           1 written, 0 unchanged, -1 failed
//   ninja_close(f)                  1, or 0 for no file
//   ninja_error()                   why the last call failed; "" when it
//                                   did not, so a text of "-1" and a
//                                   failure are told apart
//   ninja_path(f), ninja_text(f), ninja_changes(f)
// statements
//   ninja_statements(f)             how many
//   ninja_statement(f, n, field)    kind -- variable, rule, build, pool,
//                                   default, include, subninja -- name, a
//                                   build's its rule's; value, unparsed, and
//                                   value_text, as written, a variable's or
//                                   an include's file; text, line,
//                                   end_line, paths, bindings
//   ninja_find(f, kind[, name[, after]])
//                                   the first statement after `after` of the
//                                   kind, and of the name -- a build's
//                                   outputs, an include's file -- 0 for none
// paths and bindings
//   ninja_paths(f, n[, section])    how many, of a section
//   ninja_path_at(f, n, k[, field]) text, as written, by default; value,
//                                   unparsed; section, line
//   ninja_bindings(f, n); ninja_binding(f, n, k[, field])
//                                   name by default; text, as written;
//                                   value, unparsed; line
//   ninja_find_binding(f, n, name)  the last of the name; 0 for none
// comments
//   ninja_comments(f); ninja_comment(f, i, field)
//                                   text, body, line, statement
// changes, each 1 kept, or -1
//   ninja_set_value(f, n, text)     a variable's value, as written
//   ninja_set_name(f, n, name)      a variable's, rule's, pool's name, or
//                                   the rule a build uses
//   ninja_set_binding(f, n, name, text); ninja_remove_binding(f, n, name)
//   ninja_set_path(f, n, k, text)
//   ninja_insert_path(f, n, section, k, text)
//                                   the kth of its section; one past its
//                                   last appends
//   ninja_remove_path(f, n, k)
//   ninja_insert(f, n, text)        statements before statement n; n one
//                                   past the last appends
//   ninja_replace(f, n, text)       one statement for another of its kind
//   ninja_remove(f, n[, last])      statements n to `last`
// writing
//   ninja_escape(text[, how])       escaped as a path, or, `how` "value" or
//                                   "newline", as a value, a newline written
//                                   `$^` for the second; -1 for what cannot
//                                   be written so
//   ninja_canonical(path[, win32]); ninja_shell_escape(path[, win32])
// builds
//   ninja_load(path[, directory[, version]])
//                                   a build from its manifest: g, or -1
//   ninja_unload(g)                 1, or 0 for no build
//   ninja_edges(g); ninja_edge(g, e, field)
//                                   rule, phony, pool, file, line, outputs,
//                                   implicit_outputs, inputs, implicit,
//                                   order_only, validations -- the last six
//                                   each a path to a line
//   ninja_edge_var(g, e, name[, raw])
//                                   a binding evaluated as ninja does, $in
//                                   and $out shell-escaped unless raw; -1
//                                   for a cycle
//   ninja_nodes(g); ninja_node(g, n, field)
//                                   path, in_edge -- 0 for a source --
//                                   out_edges, validation_edges, each an
//                                   edge to a line
//   ninja_find_node(g, path)        made canonical first; 0 for none
//   ninja_defaults(g), ninja_roots(g), ninja_rules(g), ninja_pools(g),
//   ninja_files(g), ninja_warnings(g)
//                                   each one to a line; the default pool,
//                                   which has no name, not among the pools
//   ninja_rule(g, name[, binding])  a binding as written, unparsed, -1 for
//                                   one the rule has not; with none named,
//                                   the names it binds, one to a line
//   ninja_pool(g, name)             its depth; -1 for none
//   ninja_variable(g, name)         the top scope's
struct d_host_ninja* d_host_ninja_new(void);
void                 d_host_ninja_free(struct d_host_ninja* _host);
bool                 d_host_ninja_install(struct d_host_ninja* _host,
                                          struct d_awk_interp* _interp);


#endif  // DJINTERP_TOOLS_DAWK_EXT_NINJA_DHOST_NINJA_H
