/*******************************************************************************
* djinterp [tools]                                                   dhost_git.h
*
* The git extension: git's own files read and changed from awk, and a
* repository read.
*   A file -- .gitignore, .gitattributes, a configuration, .gitmodules -- is
* opened, or made from text, and named by a handle, a number from 1. What it
* holds is counted and read by index, from 1: its lines, its patterns and
* their attributes' states, its sections and entries, and what git refuses
* in it. A change is kept only if the file outside it reads as it did; a
* change refused gives -1, and git_error() says why. Indices are into the
* file as it is now: a change that adds or removes records moves those after
* it.
*   A repository is opened from a path in it, as git finds one, and named by
* a handle as a file is; it is read, never changed. Its configuration, its
* objects by revision, its refs, its history, its index, and its work tree's
* ignored files, attributes and status are each what git would give, with no
* git run. A function that takes a handle to either reads the one it is
* given: git_config, git_ignored, git_attr.
*
* path:      /inc/djinterp/tools/dawk/ext/git/dhost_git.h
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
         1.  d_host_git
2.  OPERATIONS
    ----------
    1.  The extension
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_GIT_DHOST_GIT_H
#define DJINTERP_TOOLS_DAWK_EXT_GIT_DHOST_GIT_H 1

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
// d_host_git
//   struct: the files and repositories open, and the last failure of the
//   git_* functions. Layout is private.
struct d_host_git;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    The extension
//------------------------------------------------------------------------------
// d_host_git_install adds these, as a program calls them; f is a file's
// handle, r a repository's, h either's. A list is a line to each item; a
// path in one is written as it is, unless it holds a control byte, a quote
// or a backslash, when it is in git's quotes. A failure gives -1, and
// git_error() says why.
//
// files
//   git_open(path[, kind])         a file: f, or -1; its kind -- ignore,
//                                  attributes, config -- its name's, unless
//                                  named
//   git_new(path[, kind[, text]])  a file not yet on disk, of the text
//   git_save(f[, path])            1 written, 0 unchanged, -1 failed
//   git_close(h)                   1, or 0 for nothing open
//   git_error()                    why the last failure failed
//   git_path(f), git_kind(f), git_text(f), git_changes(f)
//   git_kind_of(path)              ignore, attributes, config, or other
//   git_problems(f)                how many; git_problem(f, i, field):
//                                  line, what
// lines
//   git_lines(f); git_line(f, i, field)
//                                  kind -- blank, comment, pattern, macro,
//                                  section, entry, refused -- text,
//                                  pattern, section, entry; 0 for none
//   git_insert_line(f, before, text)
//                                  before a line, or after the last: the
//                                  line's index
//   git_add_line(f, text)          after the last
//   git_set_line(f, i, text); git_remove_line(f, i)
// patterns, of ignore and attribute files
//   git_patterns(f); git_pattern(f, i, field)
//                                  line, text -- as written -- pattern --
//                                  as git matches it -- negated, directory,
//                                  anywhere, quoted, macro, states
//   git_state(f, i, n, field)      the nth state of a pattern's line: name,
//                                  state -- set, unset, unspecified,
//                                  value -- value, text
//   git_set_state(f, i, state)     a state as written: text, -text, !text,
//                                  eol=lf
//   git_remove_state(f, i, name)
//   git_match(f, path)             the pattern that decides the path, the
//                                  last that matches it; 0 for none. A path
//                                  ending in `/` is a directory's
// configuration files
//   git_sections(f); git_section(f, i, field)
//                                  name, subsection, prefix, line, legacy,
//                                  entries, first
//   git_entries(f); git_entry(f, i, field)
//                                  key, name, value, has_value, raw, line,
//                                  last_line, section, text
//   git_find_entry(f, key[, after]); git_find_section(f, name[, after])
//                                  the first after `after`; 0 for none
//   git_set(f, key, value)         as git config key value
//   git_add(f, key, value)         as git config --add
//   git_unset(f, key[, all])       as git config --unset, --unset-all
//   git_set_entry(f, i[, value]); git_remove_entry(f, i)
//   git_rename_section(f, name, to); git_remove_section(f, name)
// configuration, a file's or a repository's
//   git_config(h, key)             the value git config key gives; "" for
//                                  none
//   git_config_all(h, key)         every value, a line each
//   git_config_count(h, key)       how many entries the key has
//   git_config_bool(h, key[, default]); git_config_int(h, key[, default])
//                                  as git reads a boolean -- 1 or 0 -- or
//                                  a number
// text
//   git_wildmatch(pattern, text[, flags])
//                                  1 or 0; flags: pathname, casefold
//   git_key(key)                   the key as git compares keys; "" for one
//                                  git refuses
//   git_quote_value(value); git_quote_subsection(name)
//   git_quote_path(path); git_unquote(text)
// repositories
//   git_repo([path])               the repository a path is in: r, or -1
//   git_dir(r), git_common_dir(r), git_work_tree(r) -- "" when bare --
//   git_hash(r)                    sha1 or sha256
//   git_config_file(r[, scope])    the file git config --scope writes:
//                                  local, worktree, global, system
//   git_settings(r); git_setting(r, i, field)
//                                  key, value, has_value, file, line,
//                                  scope, included
// objects, named as git rev-parse names them
//   git_rev(r, name)               the name in hex
//   git_short(r, name[, least])    as git rev-parse --short
//   git_exists(r, name)            1 for a name of an object there, else 0
//   git_type(r, name), git_size(r, name), git_cat(r, name)
//   git_commit(r, name, field)     oid, tree, parents, parent_count,
//                                  author, author_email, author_time,
//                                  author_zone, committer, committer_email,
//                                  committer_time, committer_zone,
//                                  encoding, message, subject, body
//   git_tag(r, name, field)        oid, object, type, tag, tagger,
//                                  tagger_email, tagger_time, tagger_zone,
//                                  message
//   git_tree(r, name[, deep])      as git ls-tree, or ls-tree -r: mode,
//                                  type, name, a tab, path
//   git_hash_object(r, content[, type])
//                                  the name git gives it, writing nothing
// refs and history
//   git_refs(r[, prefix])          every ref, by full name, sorted
//   git_ref(r, ref, field)         oid, target, peeled
//   git_symbolic(r, ref)           the ref a symbolic ref names
//   git_branch(r)                  HEAD's branch, without refs/heads/; ""
//                                  when detached
//   git_log(r, revisions[, most[, path[, first_parent]]])
//                                  the commits, as git rev-list gives them;
//                                  revisions apart by blanks, ^rev to
//                                  exclude
//   git_merge_base(r, a, b)        as git merge-base --all
// the index and the work tree
//   git_index(r); git_index_entry(r, i, field)
//                                  path, mode, oid, stage, size, mtime,
//                                  ctime, assume_valid, skip_worktree,
//                                  intent_to_add
//   git_index_find(r, path[, stage])
//   git_ignored(h, path)           1 when the repository, or an ignore
//                                  file, ignores the path; else 0
//   git_ignore_rule(h, path)       what decided, as git check-ignore -v:
//                                  file:line:pattern; "" for nothing
//   git_attr(h, path[, name])      an attribute's state for the path --
//                                  set, unset, unspecified, or its value --
//                                  or, unnamed, every one, "name: state" a
//                                  line, as git check-attr -a
//   git_status(r[, options])       as git status --porcelain --no-renames,
//                                  "XY path" a line; options -uno, -uall,
//                                  --ignored
struct d_host_git* d_host_git_new(void);
void               d_host_git_free(struct d_host_git* _host);
bool               d_host_git_install(struct d_host_git*   _host,
                                      struct d_awk_interp* _interp);


#endif  // DJINTERP_TOOLS_DAWK_EXT_GIT_DHOST_GIT_H
