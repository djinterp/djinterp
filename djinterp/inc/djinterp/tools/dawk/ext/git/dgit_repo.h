/*******************************************************************************
* djinterp [tools]                                                   dgit_repo.h
*
* A git repository, read directly.
*   Everything is read from the repository's files, as git itself would read
* them, and nothing runs git. A repository is found as git finds one: from a
* directory up, a .git directory or a .git file's `gitdir:`, a bare
* repository, GIT_DIR and GIT_WORK_TREE, a linked work tree's commondir. Its
* format is checked: SHA-1 or SHA-256 names, and refs in files -- a
* repository whose refs are in a reftable is refused.
*   What is read: the configuration, every scope git reads -- the system's,
* the user's, the repository's, the work tree's, the command's from the
* environment -- each file's includes followed, by path and by condition;
* refs, loose and packed, symbolic ones followed; objects, loose and packed,
* deltas applied, alternates searched; commits, trees and tags parsed;
* revisions as rev-parse names them; history as rev-list walks it, by
* commit date, first parents only if asked, limited to a path as git limits
* it; the index, of every version; and status as git status --porcelain
* reports it, with the ignore and attribute files of the work tree.
*   What is not: a write of any kind -- the repository is only read -- and
* what git would ask another program for: a filter's clean command, a
* textconv, a hook. A file a filter cleans is compared by its stat alone.
*   Independent of dgit.h, which reads history for dcheck's sheets.
*
* path:      /inc/djinterp/tools/dawk/ext/git/dgit_repo.h
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
         1.  D_GIT_REPO_NONE
         2.  D_GIT_OID_MOST
    2.  Kinds
         1.  d_git_object_type
         2.  d_git_scope
         3.  d_git_untracked
    3.  Names and objects
         1.  d_git_oid
         2.  d_git_oids
         3.  d_git_object
         4.  d_git_person
         5.  d_git_commit
         6.  d_git_tag
         7.  d_git_tree_entry
    4.  Repository
         1.  d_git_repo
         2.  d_git_setting
         3.  d_git_ref
         4.  d_git_refs
         5.  d_git_index_entry
         6.  d_git_log_options
         7.  d_git_status_options
         8.  d_git_status_entry
         9.  d_git_status
        10.  d_git_ignore_match
2.  OPERATIONS
    ----------
    1.  Repositories
    2.  Configuration
    3.  Names
    4.  Objects
    5.  Refs
    6.  Revisions
    7.  History
    8.  Index
    9.  Work tree
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_REPO_H
#define DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_REPO_H 1

// std
#include <stdbool.h>      // bool
#include <stddef.h>       // size_t
// djinterp
#include "./dgit_text.h"  // d_git_text_values, d_git_buffer
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint8_t, uint32_t, int64_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_GIT_REPO_NONE
//   constant: the index of a record that is not there.
#define D_GIT_REPO_NONE ((size_t)-1)

// 1.1.2
// D_GIT_OID_MOST
//   constant: the most bytes an object's name has: SHA-256's 32.
#define D_GIT_OID_MOST 32u

// 1.2    Kinds
//------------------------------------------------------------------------------
// 1.2.1
// d_git_object_type
//   enum: what an object is, numbered as git numbers them.
enum d_git_object_type
{
    D_GIT_OBJECT_NONE = 0,
    D_GIT_OBJECT_COMMIT,
    D_GIT_OBJECT_TREE,
    D_GIT_OBJECT_BLOB,
    D_GIT_OBJECT_TAG
};

// 1.2.2
// d_git_scope
//   enum: where a setting comes from, lowest first.
enum d_git_scope
{
    D_GIT_SCOPE_SYSTEM = 0,  // /etc/gitconfig, GIT_CONFIG_SYSTEM
    D_GIT_SCOPE_GLOBAL,      // ~/.gitconfig, git/config, GIT_CONFIG_GLOBAL
    D_GIT_SCOPE_LOCAL,       // the repository's config
    D_GIT_SCOPE_WORKTREE,    // config.worktree, with extensions.worktreeConfig
    D_GIT_SCOPE_COMMAND      // GIT_CONFIG_COUNT and GIT_CONFIG_PARAMETERS
};

// 1.2.3
// d_git_untracked
//   enum: how status shows files git does not track, as
//   status.showUntrackedFiles says it.
enum d_git_untracked
{
    D_GIT_UNTRACKED_NORMAL = 0,  // a directory of them as the directory
    D_GIT_UNTRACKED_NO,          // none
    D_GIT_UNTRACKED_ALL          // every file
};

// 1.3    Names and objects
//------------------------------------------------------------------------------
// 1.3.1
// d_git_oid
//   struct: an object's name: as many bytes as the repository's hash has,
//   the rest 0.
struct d_git_oid
{
    uint8_t bytes[D_GIT_OID_MOST];
};

// 1.3.2
// d_git_oids
//   struct: names, `count` of them, in an order the giver says.
struct d_git_oids
{
    struct d_git_oid* items;
    size_t            count;
    size_t            capacity;
};

// 1.3.3
// d_git_object
//   struct: an object's type and content, `size` bytes, allocated and
//   terminated.
struct d_git_object
{
    enum d_git_object_type type;
    uint8_t*               data;
    size_t                 size;
};

// 1.3.4
// d_git_person
//   struct: an author, committer or tagger, as a commit or tag writes one:
//   the name and the address, each borrowed from the object's content,
//   the time in seconds since 1970, and the zone, `offset` minutes from
//   UTC, as written -- "+0100" -- in `zone`.
struct d_git_person
{
    const char* name;
    size_t      name_length;
    const char* email;
    size_t      email_length;
    int64_t     time;
    int         offset;
    char        zone[8];
};

// 1.3.5
// d_git_commit
//   struct: a commit, parsed: its tree, its parents in order, allocated,
//   its author and committer, the encoding a header names -- NULL for
//   none -- and its message, after the headers' blank line, each borrowed
//   from the object's content.
struct d_git_commit
{
    struct d_git_oid    tree;
    struct d_git_oid*   parents;
    size_t              parent_count;
    struct d_git_person author;
    struct d_git_person committer;
    const char*         encoding;
    size_t              encoding_length;
    const char*         message;
    size_t              message_length;
};

// 1.3.6
// d_git_tag
//   struct: an annotated tag, parsed: what it names, of what type, its
//   name, its tagger, if it has one, and its message, borrowed.
struct d_git_tag
{
    struct d_git_oid       object;
    enum d_git_object_type type;
    const char*            name;
    size_t                 name_length;
    bool                   has_tagger;
    struct d_git_person    tagger;
    const char*            message;
    size_t                 message_length;
};

// 1.3.7
// d_git_tree_entry
//   struct: one entry of a tree: its mode, octal as git writes it --
//   100644, 100755, 120000, 040000, 160000 -- its name, borrowed, and
//   its object.
struct d_git_tree_entry
{
    uint32_t         mode;
    const char*      name;
    size_t           name_length;
    struct d_git_oid oid;
};

// 1.4    Repository
//------------------------------------------------------------------------------
// 1.4.1
// d_git_repo
//   struct: an open repository: its paths, format, configuration, and what
//   has been read of its refs, objects and index. Layout is private.
struct d_git_repo;

// 1.4.2
// d_git_setting
//   struct: one setting, as git config --list --show-origin gives it: its
//   canonical key, its value -- NULL for a name alone -- the file it is
//   in, its line from 1, its scope, and whether it was included.
struct d_git_setting
{
    const char*      key;
    const char*      value;
    const char*      file;
    size_t           line;
    enum d_git_scope scope;
    bool             included;
};

// 1.4.3
// d_git_ref
//   struct: one ref: its full name, the object it names -- `resolved`
//   false for a symbolic ref that leads nowhere -- the ref a symbolic one
//   names, NULL for another, and, for one that names a tag, what peeling
//   the tag gives.
struct d_git_ref
{
    char*            name;
    struct d_git_oid oid;
    bool             resolved;
    char*            target;
    struct d_git_oid peeled;
    bool             has_peeled;
};

// 1.4.4
// d_git_refs
//   struct: refs, `count` of them, ordered by name.
struct d_git_refs
{
    struct d_git_ref* items;
    size_t            count;
    size_t            capacity;
};

// 1.4.5
// d_git_index_entry
//   struct: one entry of the index: its path, its mode and object, its
//   stage -- 0, or 1 to 3 for a conflict -- its flags, and the stat data
//   git keeps of its file.
struct d_git_index_entry
{
    const char*      path;
    size_t           path_length;
    uint32_t         mode;
    struct d_git_oid oid;
    unsigned         stage;
    bool             assume_valid;
    bool             skip_worktree;
    bool             intent_to_add;
    uint32_t         ctime;
    uint32_t         ctime_nanoseconds;
    uint32_t         mtime;
    uint32_t         mtime_nanoseconds;
    uint32_t         dev;
    uint32_t         ino;
    uint32_t         uid;
    uint32_t         gid;
    uint32_t         size;
};

// 1.4.6
// d_git_log_options
//   struct: what to walk: revisions to start from -- one written ^rev
//   excluding all it reaches -- the most commits to give, 0 for all,
//   whether to follow first parents only, and a path that limits the walk
//   to commits that change it, NULL for none.
struct d_git_log_options
{
    const char* const* revisions;
    size_t             revision_count;
    size_t             most;
    bool               first_parent;
    const char*        path;
};

// 1.4.7
// d_git_status_options
//   struct: what status shows: untracked files, how, and ignored files.
struct d_git_status_options
{
    enum d_git_untracked untracked;
    bool                 ignored;
};

// 1.4.8
// d_git_status_entry
//   struct: one line of git status --porcelain: the index's state and
//   the work tree's -- ' ', 'M', 'A', 'D', 'T', 'U', '?', '!' -- and the
//   path, allocated, a directory's with its trailing `/`.
struct d_git_status_entry
{
    char  index;
    char  work;
    char* path;
};

// 1.4.9
// d_git_status
//   struct: status's lines, `count` of them, in git's order: the tracked
//   by path, then the untracked, then the ignored.
struct d_git_status
{
    struct d_git_status_entry* items;
    size_t                     count;
    size_t                     capacity;
};

// 1.4.10
// d_git_ignore_match
//   struct: what decided whether a path is ignored, as git check-ignore
//   -v gives it: the file, from the top of the work tree or as it was
//   named, its line from 1, and the pattern as written; `ignored` is
//   false for a negated pattern.
struct d_git_ignore_match
{
    const char* file;
    size_t      line;
    const char* pattern;
    size_t      pattern_length;
    bool        ignored;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Repositories
//------------------------------------------------------------------------------
/**
 * @brief Opens the repository a path is in, as git finds it.
 *
 * @note From the path up, the first directory with a .git -- a directory,
 *       or a file naming one -- or that is itself a repository's; GIT_DIR,
 *       when it is set, names the repository instead, and GIT_WORK_TREE
 *       or core.worktree its work tree. GIT_CEILING_DIRECTORIES stops the
 *       search.
 *
 * @param[in]  _path      where to begin; `NULL` for the working directory.
 * @param[out] _problem   receives why no repository was opened.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller frees the repository with d_git_repo_free.
 * @return the repository; `NULL` when there is none, its format is one
 *         this does not read, its configuration cannot be read, or memory
 *         ran out.
 */
struct d_git_repo* d_git_repo_open(const char* _path,
                                   char*       _problem,
                                   size_t      _capacity);
void               d_git_repo_free(struct d_git_repo* _repo);
// the repository's directory, its common directory -- a linked work
// tree's main repository's, else the same -- its work tree, NULL for a
// bare one, the length of its names in bytes, and why the last thing
// asked of it failed, "" when nothing has.
const char*        d_git_repo_git_dir(const struct d_git_repo* _repo);
const char*        d_git_repo_common_dir(const struct d_git_repo* _repo);
const char*        d_git_repo_work_tree(const struct d_git_repo* _repo);
size_t             d_git_repo_hash_size(const struct d_git_repo* _repo);
const char*        d_git_repo_problem(const struct d_git_repo* _repo);

// 2.2    Configuration
//------------------------------------------------------------------------------
// d_git_repo_settings is how many settings the repository's configuration
// holds, d_git_repo_setting one of them, from 0, in git's order: the
// lowest scope first, each file's in its order, an included file's in
// place of its include; NULL past the last.
// d_git_repo_find_setting is the first setting after `_after` -- from the
// first for D_GIT_REPO_NONE -- of a key, in any case git takes;
// d_git_repo_config the value git config key gives, the last setting's,
// and whether there is one: a name alone has the value NULL, and is set.
size_t             d_git_repo_settings(const struct d_git_repo* _repo);
const struct d_git_setting*
                   d_git_repo_setting(const struct d_git_repo* _repo,
                                      size_t                   _index);
size_t             d_git_repo_find_setting(const struct d_git_repo* _repo,
                                           const char*              _key,
                                           size_t                   _after);
const char*        d_git_repo_config(const struct d_git_repo* _repo,
                                     const char*              _key,
                                     bool*                    _out_set);

// 2.3    Names
//------------------------------------------------------------------------------
// d_git_repo_hex writes a name in hex, terminated, into at least
// 2 * D_GIT_OID_MOST + 1 bytes; d_git_repo_parse_hex reads a name of the
// repository's length in hex, in either case, false for anything else.
void               d_git_repo_hex(const struct d_git_repo* _repo,
                                  const struct d_git_oid*  _oid,
                                  char*                    _out);
bool               d_git_repo_parse_hex(const struct d_git_repo* _repo,
                                        const char*              _hex,
                                        struct d_git_oid*        _out);
bool               d_git_repo_same_oid(const struct d_git_repo* _repo,
                                       const struct d_git_oid*  _left,
                                       const struct d_git_oid*  _right);
// d_git_repo_abbreviate writes the shortest prefix of a name, of at least
// `_least` hex digits -- core.abbrev's, 7, for 0 -- that names no other
// object, as git rev-parse --short does.
bool               d_git_repo_abbreviate(struct d_git_repo*      _repo,
                                         const struct d_git_oid* _oid,
                                         size_t                  _least,
                                         char*                   _out);
void               d_git_oids_release(struct d_git_oids* _oids);

// 2.4    Objects
//------------------------------------------------------------------------------
/**
 * @brief Reads an object: loose, or from a pack, its deltas applied, in
 *        the repository or an alternate.
 *
 * @param[in,out] _repo  the repository; what it reads it may keep.
 * @param[in]     _oid   the object's name.
 * @param[out]    _out   receives the object; release it with
 *                       d_git_object_release, whatever is returned.
 * @return `true` if the object was read and its content is sound; `false`
 *         when there is no such object, it is damaged, or memory ran out,
 *         d_git_repo_problem saying which.
 */
bool               d_git_repo_read(struct d_git_repo*      _repo,
                                   const struct d_git_oid* _oid,
                                   struct d_git_object*    _out);
void               d_git_object_release(struct d_git_object* _object);
// d_git_repo_has is whether the repository holds an object;
// d_git_repo_hash_object the name git would give content of a type, as
// git hash-object does, writing nothing.
bool               d_git_repo_has(struct d_git_repo*      _repo,
                                  const struct d_git_oid* _oid);
void               d_git_repo_hash_object(const struct d_git_repo* _repo,
                                          enum d_git_object_type   _type,
                                          const void*              _data,
                                          size_t                   _size,
                                          struct d_git_oid*        _out);
// d_git_object_type_name is a type's name, "commit", "tree", "blob" or
// "tag", and d_git_object_type_named the type of a name.
const char*        d_git_object_type_name(enum d_git_object_type _type);
enum d_git_object_type
                   d_git_object_type_named(const char* _name);
/**
 * @brief Parses a commit's content.
 *
 * @param[in]  _repo    the repository, for the length of its names.
 * @param[in]  _object  the commit; what is parsed borrows from it.
 * @param[out] _out     receives the commit; release it with
 *                      d_git_commit_release, whatever is returned.
 * @return `true` if it is a commit git would read; `false` otherwise.
 */
bool               d_git_repo_parse_commit(const struct d_git_repo*   _repo,
                                           const struct d_git_object* _object,
                                           struct d_git_commit*       _out);
void               d_git_commit_release(struct d_git_commit* _commit);
// d_git_repo_parse_tag parses an annotated tag's content, borrowing from
// it; false for one git would not read.
bool               d_git_repo_parse_tag(const struct d_git_repo*   _repo,
                                        const struct d_git_object* _object,
                                        struct d_git_tag*          _out);
// d_git_repo_tree_next reads a tree's entry at `*_at`, from 0, and moves
// `*_at` past it, borrowing from the tree; false at its end, or for an
// entry git would not read.
bool               d_git_repo_tree_next(const struct d_git_repo*   _repo,
                                        const struct d_git_object* _tree,
                                        size_t*                    _at,
                                        struct d_git_tree_entry*   _out);

// 2.5    Refs
//------------------------------------------------------------------------------
// d_git_repo_resolve gives what a ref of a full name names, symbolic refs
// followed -- HEAD, refs/heads/main -- and, when `_out_target` is not
// NULL, the ref a symbolic one named first, allocated, NULL for another;
// false for a ref that is not there or leads nowhere.
// d_git_repo_refs gives every ref whose name begins with a prefix -- ""
// for all under refs/ -- loose and packed, ordered by name; release it
// with d_git_refs_release, whatever is returned.
bool               d_git_repo_resolve(struct d_git_repo* _repo,
                                      const char*        _name,
                                      struct d_git_oid*  _out,
                                      char**             _out_target);
bool               d_git_repo_refs(struct d_git_repo* _repo,
                                   const char*        _prefix,
                                   struct d_git_refs* _out);
void               d_git_refs_release(struct d_git_refs* _refs);
// d_git_repo_symbolic gives the ref a symbolic ref names, not followed --
// HEAD's branch, even one with no commit yet -- allocated; NULL for a ref
// that is not symbolic.
char*              d_git_repo_symbolic(struct d_git_repo* _repo,
                                       const char*        _name);

// 2.6    Revisions
//------------------------------------------------------------------------------
/**
 * @brief Names an object as git rev-parse names one.
 *
 * @note A name in hex, whole or a prefix of four digits or more that is
 *       one object's alone; a ref, as rev-parse's rules complete one --
 *       name, refs/name, refs/tags/name, refs/heads/name,
 *       refs/remotes/name, refs/remotes/name/HEAD; `@` for HEAD; a ref's
 *       reflog, ref@{n}, and the current branch's, @{n}; the branch the
 *       nth checkout before left, @{-n}; a branch's upstream, @{u} and
 *       @{upstream}; after any of these, ^ and ^n for a parent, ~n for a
 *       first-parent ancestor, ^{} and ^{type} to peel, and :path for what
 *       a tree holds there; :path and :n:path for the index's entry. A
 *       reflog's dates, @{push}, :/text and ^{/text} are refused.
 *
 * @param[in,out] _repo  the repository.
 * @param[in]     _name  the revision.
 * @param[out]    _out   receives the object's name.
 * @return `true` if the revision names an object; `false` otherwise,
 *         d_git_repo_problem saying why.
 */
bool               d_git_repo_rev(struct d_git_repo* _repo,
                                  const char*        _name,
                                  struct d_git_oid*  _out);

// 2.7    History
//------------------------------------------------------------------------------
/**
 * @brief Walks history as git rev-list does without other options.
 *
 * @note Commits come by commit date, the latest first, a commit never
 *       before one it is a parent of; one reached from an excluded
 *       revision never comes. Limited to a path, a commit comes when its
 *       tree there differs from every parent's, and of a merge whose tree
 *       there is a parent's only that parent is walked, as git simplifies
 *       history by default.
 *
 * @param[in,out] _repo     the repository.
 * @param[in]     _options  what to walk.
 * @param[out]    _out      receives the commits; release it with
 *                          d_git_oids_release, whatever is returned.
 * @return `true` if the walk finished; `false` when a revision names no
 *         commit, a commit cannot be read, or memory ran out.
 */
bool               d_git_repo_log(struct d_git_repo*              _repo,
                                  const struct d_git_log_options* _options,
                                  struct d_git_oids*              _out);
// d_git_repo_merge_base gives the best common ancestors of two commits,
// as git merge-base --all does; false when either is not a commit.
bool               d_git_repo_merge_base(struct d_git_repo*      _repo,
                                         const struct d_git_oid* _left,
                                         const struct d_git_oid* _right,
                                         struct d_git_oids*      _out);

// 2.8    Index
//------------------------------------------------------------------------------
// d_git_repo_index_count is how many entries the index holds -- read the
// first time it is asked, D_GIT_REPO_NONE when it cannot be -- and
// d_git_repo_index_entry one of them, from 0, ordered by path then stage.
// d_git_repo_index_find is the entry of a path at a stage.
size_t             d_git_repo_index_count(struct d_git_repo* _repo);
const struct d_git_index_entry*
                   d_git_repo_index_entry(struct d_git_repo* _repo,
                                          size_t             _index);
size_t             d_git_repo_index_find(struct d_git_repo* _repo,
                                         const char*        _path,
                                         unsigned           _stage);

// 2.9    Work tree
//------------------------------------------------------------------------------
// d_git_repo_ignored is whether git ignores a path of the work tree, as
// git check-ignore decides it for a path it does not track, and what
// decided; a path is a directory's when it ends with `/`, or names one. A
// .gitignore missing from the work tree is read from the index when its
// entry is skip-worktree, and one that is a link is not read, as git
// reads them. False for a path no pattern decides, or a bare repository.
// d_git_repo_attributes gives a path's attributes as git check-attr -a
// does, ordered by name, borrowed from the repository; release it with
// d_git_text_values_release, whatever is returned. Each directory's
// .gitattributes is the work tree's -- the index's when it is missing
// there, or is a link -- or, for GIT_ATTR_SOURCE, attr.tree or a bare
// repository's HEAD, that tree's, as git 2.43 reads them.
bool               d_git_repo_ignored(struct d_git_repo*         _repo,
                                      const char*                _path,
                                      struct d_git_ignore_match* _out);
bool               d_git_repo_attributes(struct d_git_repo*        _repo,
                                         const char*               _path,
                                         struct d_git_text_values* _out);
/**
 * @brief Gives the work tree's status, as git status --porcelain
 *        --no-renames gives it.
 *
 * @param[in,out] _repo     the repository; it must have a work tree.
 * @param[in]     _options  what to show beside the tracked.
 * @param[out]    _out      receives the lines; release it with
 *                          d_git_status_release, whatever is returned.
 * @return `true` if status was read; `false` for a bare repository, an
 *         index or tree that cannot be read, or memory that ran out.
 */
bool               d_git_repo_status(
                       struct d_git_repo*                 _repo,
                       const struct d_git_status_options* _options,
                       struct d_git_status*               _out);
void               d_git_status_release(struct d_git_status* _status);


#endif  // DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_REPO_H
