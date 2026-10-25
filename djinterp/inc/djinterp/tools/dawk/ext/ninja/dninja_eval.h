/*******************************************************************************
* djinterp [tools]                                                 dninja_eval.h
*
* A Ninja build, loaded as ninja loads it.
*   A manifest is read with dninja, and each statement taken as ninja's
* manifest parser takes it, in order: variables bound in their scope as
* they are read, a subninja's scope within its parent's and an include's
* the same as its parent's; rules and pools defined; each build's paths
* evaluated in its scope and made canonical, its outputs and inputs made
* the nodes of a graph, its pool and its dyndep found; the defaults found
* among the nodes. What ninja refuses as it loads is refused, in ninja's
* words, at the line ninja names: a rule or a pool not defined or defined
* twice, a path that comes to nothing, an output two builds make, a default
* no build names, a file that cannot be read, a version the build requires
* that the ninja acted as is too old for.
*   A build's bindings are evaluated as ninja's EdgeEnv evaluates them:
* $in, $in_newline and $out from its paths, then the build's own bindings,
* then its rule's, evaluated in turn in the build's sense, then the scopes
* about it; a cycle among a rule's bindings is reported as ninja reports it.
* $in and $out are written shell-escaped where ninja escapes them.
*   The ninja acted as is a version: what it reads and how it words what it
* refuses follow the version, from 1.11 to 1.14, as the banner of dninja.h
* and each operation here say.
*
* path:      /inc/djinterp/tools/dawk/ext/ninja/dninja_eval.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Constants
         1.  D_NINJA_VERSION
    2.  Loading
         1.  d_ninja_options
         2.  d_ninja_graph
2.  OPERATIONS
    ----------
    1.  Loading
    2.  Edges
    3.  Nodes
    4.  Targets
    5.  Rules, pools and variables
    6.  Files and warnings
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_NINJA_DNINJA_EVAL_H
#define DJINTERP_TOOLS_DAWK_EXT_NINJA_DNINJA_EVAL_H 1

// std
#include <stdbool.h>   // bool
#include <stddef.h>    // size_t
// djinterp
#include "./dninja.h"  // d_ninja_section


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_NINJA_VERSION
//   constant: the ninja acted as when none is named: the newest, whose
//   reading the others' are measured against.
#define D_NINJA_VERSION "1.14.0"

// 1.2    Loading
//------------------------------------------------------------------------------
// 1.2.1
// d_ninja_options
//   struct: how a build is loaded: the directory ninja runs in, which the
//   paths of the manifest and of its includes are read from, NULL or "" for
//   the process's; the version of ninja acted as, NULL for D_NINJA_VERSION;
//   and whether paths and escaping are Windows'.
struct d_ninja_options
{
    const char* directory;
    const char* version;
    bool        win32;
};

// 1.2.2
// d_ninja_graph
//   struct: a build loaded: its files, scopes, rules, pools, edges and
//   nodes, and its defaults.  Layout is private.
struct d_ninja_graph;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Loading
//------------------------------------------------------------------------------
/**
 * @brief Loads a build from its manifest, as ninja loads one.
 *
 * @param[in]  _path      the manifest, as ninja would be given it.
 * @param[in]  _options   how; may be `NULL` for the defaults.
 * @param[out] _problem   receives why it could not be loaded, as ninja's
 *                        first line says it: "file:line: message", or
 *                        "loading 'file': why", or a version's refusal.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller frees the build with d_ninja_graph_free.
 * @return the build, or `NULL` when ninja would not load it, or memory ran
 *         out.
 */
struct d_ninja_graph* d_ninja_load(const char*                   _path,
                                   const struct d_ninja_options* _options,
                                   char*                         _problem,
                                   size_t                        _capacity);
// d_ninja_load_text loads a build from a manifest's text, named `_path`;
// what it includes is read from disk.
struct d_ninja_graph* d_ninja_load_text(const char*      _path,
                                        const char*      _text,
                                        size_t           _length,
                                        const struct d_ninja_options*
                                                         _options,
                                        char*            _problem,
                                        size_t           _capacity);
void                  d_ninja_graph_free(struct d_ninja_graph* _graph);

// 2.2    Edges
//------------------------------------------------------------------------------
// The edges in the order the manifest gives them, from 0.  A section's nodes
// are given for D_NINJA_OUTPUT, D_NINJA_IMPLICIT_OUTPUT, D_NINJA_INPUT,
// D_NINJA_IMPLICIT, D_NINJA_ORDER_ONLY and D_NINJA_VALIDATION, each an array
// the build owns; an edge's pool is "" for the default.  Its file and line
// are the build statement's.
size_t                d_ninja_edge_count(const struct d_ninja_graph* _graph);
const char*           d_ninja_edge_rule(const struct d_ninja_graph* _graph,
                                        size_t                      _edge);
bool                  d_ninja_edge_phony(const struct d_ninja_graph* _graph,
                                         size_t                      _edge);
size_t                d_ninja_edge_nodes(const struct d_ninja_graph* _graph,
                                         size_t               _edge,
                                         enum d_ninja_section _section,
                                         const size_t**       _out_nodes);
const char*           d_ninja_edge_pool(const struct d_ninja_graph* _graph,
                                        size_t                      _edge);
const char*           d_ninja_edge_file(const struct d_ninja_graph* _graph,
                                        size_t                      _edge);
size_t                d_ninja_edge_line(const struct d_ninja_graph* _graph,
                                        size_t                      _edge);
/**
 * @brief Evaluates a binding of an edge as ninja's EdgeEnv does.
 *
 * @param[in]  _graph     the build.
 * @param[in]  _edge      the edge.
 * @param[in]  _name      the binding: command, description, depfile, any.
 * @param[in]  _escape    whether $in and $out are shell-escaped, as ninja
 *                        escapes them for every binding but depfile,
 *                        dyndep and rspfile.
 * @param[out] _problem   receives why it could not be: a cycle among the
 *                        rule's bindings, as ninja words one.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller frees the text.
 * @return the value, "" for a binding the edge has nowhere, or `NULL` for
 *         a cycle or memory that ran out.
 */
char*                 d_ninja_edge_binding(
                          const struct d_ninja_graph* _graph,
                          size_t                      _edge,
                          const char*                 _name,
                          bool                        _escape,
                          char*                       _problem,
                          size_t                      _capacity);

// 2.3    Nodes
//------------------------------------------------------------------------------
// The nodes in the order the manifest first names them, from 0, each a
// canonical path.  A node's in-edge is D_NINJA_NONE for a source; its
// out-edges are the edges it is an input of, and its validation edges
// those it validates; each array is the node's.  d_ninja_node_find finds
// a node by a path, made canonical first; D_NINJA_NONE for none.
size_t                d_ninja_node_count(const struct d_ninja_graph* _graph);
const char*           d_ninja_node_path(const struct d_ninja_graph* _graph,
                                        size_t                      _node);
size_t                d_ninja_node_in_edge(const struct d_ninja_graph* _graph,
                                           size_t                      _node);
size_t                d_ninja_node_out_edges(
                          const struct d_ninja_graph* _graph,
                          size_t                      _node,
                          const size_t**              _out_edges);
size_t                d_ninja_node_validation_edges(
                          const struct d_ninja_graph* _graph,
                          size_t                      _node,
                          const size_t**              _out_edges);
size_t                d_ninja_node_find(const struct d_ninja_graph* _graph,
                                        const char*                 _path,
                                        size_t                      _length);

// 2.4    Targets
//------------------------------------------------------------------------------
// The defaults the manifest names, in its order; the roots, every output
// no edge takes as an input, in edge order, as ninja's RootNodes finds
// them.  Each array is the build's.
size_t                d_ninja_defaults(const struct d_ninja_graph* _graph,
                                       const size_t**              _out_nodes);
size_t                d_ninja_roots(const struct d_ninja_graph* _graph,
                                    const size_t**              _out_nodes);

// 2.5    Rules, pools and variables
//------------------------------------------------------------------------------
// The rules of the top scope, phony among them, sorted by name, as ninja's
// rules tool lists them; d_ninja_rule_binding writes one of a rule's
// bindings as ninja's Unparse does, `NULL` for one it does not have, for
// the caller to free.  The pools, the two ninja defines first -- "" and
// console -- then the manifest's.  d_ninja_variable gives a variable of
// the top scope, "" for none, for the caller to free.
size_t                d_ninja_rule_count(const struct d_ninja_graph* _graph);
const char*           d_ninja_rule_name(const struct d_ninja_graph* _graph,
                                        size_t                      _rule);
char*                 d_ninja_rule_binding(
                          const struct d_ninja_graph* _graph,
                          size_t                      _rule,
                          const char*                 _name);
size_t                d_ninja_pool_count(const struct d_ninja_graph* _graph);
const char*           d_ninja_pool_name(const struct d_ninja_graph* _graph,
                                        size_t                      _pool);
int                   d_ninja_pool_depth(const struct d_ninja_graph* _graph,
                                         size_t                      _pool);
char*                 d_ninja_variable(const struct d_ninja_graph* _graph,
                                       const char*                 _name);

// 2.6    Files and warnings
//------------------------------------------------------------------------------
// The files read, the manifest first, then each include and subninja in
// the order read; the warnings ninja would print as it loaded, in its
// words.
size_t                d_ninja_file_count(const struct d_ninja_graph* _graph);
const char*           d_ninja_file_path(const struct d_ninja_graph* _graph,
                                        size_t                      _file);
size_t                d_ninja_warning_count(
                          const struct d_ninja_graph* _graph);
const char*           d_ninja_warning(const struct d_ninja_graph* _graph,
                                      size_t                      _warning);


#endif  // DJINTERP_TOOLS_DAWK_EXT_NINJA_DNINJA_EVAL_H
