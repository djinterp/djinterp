/*******************************************************************************
* djinterp [tools]                                                   dgit_repo.c
*
* Definitions for the repositories, configuration, names, refs, revisions
* and history of dgit_repo.h; dgit_object.c reads objects, and dgit_index.c
* the index and the work tree.
*   Opening follows git's setup: GIT_DIR, or a walk up from a directory for a
* .git -- a directory, or a file naming one -- or a bare repository, each
* proved a repository as git proves one, by a HEAD that reads as a ref or a
* name, and objects and refs beside it. The format is read from the
* repository's own configuration before anything else is.
*   The configuration is every scope's files, in git's order, each read by
* dgit_text and its settings kept in order: an include's settings come in
* place of the include, conditions decided as git decides them -- gitdir:,
* gitdir/i:, onbranch:, and hasconfig:remote.*.url:, which needs every
* remote's URL first, and so a first pass that follows every such include.
*   Refs are read as the files backend reads them: a ref of one of a work
* tree's own names from its directory, any other from the common one, a
* loose ref before a packed one, symbolic refs followed five deep. A
* revision is read from its end, as get_oid_1 reads one: ^ and ~, then ^{},
* then a name in hex, a ref, a describe's -g, a prefix in hex. History is
* walked as revision.c walks it: a list by commit date, ties first in first
* out, the parents of a commit put on it as it comes off, and, when a
* revision is excluded, the whole walk first, then what is left of it.
*
* path:      /src/djinterp/tools/dawk/ext/git/dgit_repo.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_repo.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, calloc, realloc, free, getenv, qsort
#include <string.h>  // memchr, memcmp, memcpy, memmove, memset, strchr,
                     // strcmp, strlen, strncmp, strrchr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"                // d_fs_names
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_text.h"  // d_git_text
#include "./dgit_internal.h"                                              // d_git_repo layout, d_git_file_*
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint32_t,
                                                     // int64_t


// D_INTERNAL_INCLUDE_DEPTH
//   constant: how deep includes go before git refuses them.
#define D_INTERNAL_INCLUDE_DEPTH 10u

// D_INTERNAL_SYMREF_DEPTH
//   constant: how many symbolic refs git follows before giving up.
#define D_INTERNAL_SYMREF_DEPTH 5u

//==============================================================================
// 1.  STRINGS AND PATHS
//==============================================================================


bool
d_git_repo_fail(
    struct d_git_repo* _repo,
    const char*        _what,
    const char*        _detail
)
{
    // a repository to say it of
    if (_repo)
    {
        (void)snprintf(_repo->problem,
                       sizeof(_repo->problem),
                       "%s%s%s",
                       _what,
                       (_detail) ? ": " : "",
                       (_detail) ? _detail : "");
    }

    return false;
}


/*
d_internal_dup
  `_length` bytes, allocated and terminated.
*/
static char*
d_internal_dup(
    const char* _text,
    size_t      _length
)
{
    char* const copy = malloc(_length + 1u);

    // memory that ran out
    if (!copy)
    {
        return NULL;
    }

    // nothing to copy leaves the terminator
    if (_length > 0u)
    {
        memcpy(copy, _text, _length);
    }

    copy[_length] = '\0';

    return copy;
}


/*
d_internal_dup_text
  A string, allocated; NULL for NULL.
*/
static char*
d_internal_dup_text(
    const char* _text
)
{
    return (_text) ? d_internal_dup(_text, strlen(_text)) : NULL;
}


const char*
d_git_repo_keep(
    struct d_git_repo* _repo,
    const char*        _text,
    size_t             _length
)
{
    // room for one more
    if (_repo->string_count == _repo->string_capacity)
    {
        const size_t wanted = (_repo->string_capacity > 0u)
                              ? (_repo->string_capacity * 2u)
                              : 64u;
        char** const grown  = realloc(_repo->strings,
                                      wanted * sizeof(*grown));

        // memory that ran out keeps nothing
        if (!grown)
        {
            return NULL;
        }

        _repo->strings         = grown;
        _repo->string_capacity = wanted;
    }

    char* const copy = d_internal_dup(_text, _length);

    // the copy is the repository's
    if (copy)
    {
        _repo->strings[_repo->string_count] = copy;
        ++_repo->string_count;
    }

    return copy;
}


char*
d_git_path_join(
    const char* _directory,
    const char* _name
)
{
    const size_t left  = strlen(_directory);
    const size_t right = strlen(_name);
    const bool   slash = ( (left > 0u)                      &&
                           (right > 0u)                     &&
                           (_directory[left - 1u] != '/') );
    char* const  path  = malloc(left + right + 2u);

    // memory that ran out
    if (!path)
    {
        return NULL;
    }

    memcpy(path, _directory, left);

    // a slash between them, where neither has one
    if (slash)
    {
        path[left] = '/';
    }

    memcpy(&path[left + (slash ? 1u : 0u)], _name, right + 1u);

    return path;
}


#if defined(_WIN32)

/*
d_internal_separator, d_internal_root_length
  Whether a byte separates a path's names -- `/` or `\` -- and how long a
path's root is: a separator, a drive's `C:` and a separator, a drive's
`C:` alone, or none.
*/
static bool
d_internal_separator(
    char _c
)
{
    return ( (_c == '/') ||
             (_c == '\\') );
}


static size_t
d_internal_root_length(
    const char* _path
)
{
    // a separator, a drive and a separator, or a drive alone
    if (d_internal_separator(_path[0]))
    {
        return 1u;
    }
    else if ( (_path[0] != '\0') &&
              (_path[1] == ':') )
    {
        return (d_internal_separator(_path[2])) ? 3u : 2u;
    }

    return 0u;
}

#else

/*
d_internal_separator, d_internal_root_length
  Whether a byte separates a path's names -- `/` alone, as a backslash is
a name's own on POSIX -- and how long a path's root is: `/`, or none.
*/
static bool
d_internal_separator(
    char _c
)
{
    return (_c == '/');
}


static size_t
d_internal_root_length(
    const char* _path
)
{
    return (_path[0] == '/') ? 1u : 0u;
}

#endif  // _WIN32


/*
d_internal_is_absolute
  Whether a path has a root.
*/
static bool
d_internal_is_absolute(
    const char* _path
)
{
    return (d_internal_root_length(_path) > 0u);
}


/*
d_internal_absolute
  A path made absolute against a directory, `.` and `..` taken out of it as
written, links not followed, its separators `/`; allocated.
  Long as a path's root and its parts are worked out on both systems' terms.
*/
static char*
d_internal_absolute(
    const char* _base,
    const char* _path
)
{
    char* const joined = (d_internal_is_absolute(_path))
                         ? d_internal_dup_text(_path)
                         : d_git_path_join(_base, _path);
    char* const out    = (joined) ? malloc(strlen(joined) + 2u) : NULL;

    // memory that ran out
    if (!out)
    {
        free(joined);

        return NULL;
    }

    const size_t root   = d_internal_root_length(joined);
    size_t       length = root;
    size_t       at     = root;

    memcpy(out, joined, root);

    // the root's separator is `/`
    if ( (root > 0u) &&
         (d_internal_separator(out[root - 1u])) )
    {
        out[root - 1u] = '/';
    }

    // each name: `.` and nothing dropped, `..` taking the last away
    while (joined[at] != '\0')
    {
        size_t end = at;

        // the name's end
        while ( (joined[end] != '\0') &&
                (!d_internal_separator(joined[end])) )
        {
            ++end;
        }

        const size_t size = end - at;

        // a name kept, or the last taken away
        if ( (size == 2u) &&
             (joined[at] == '.') &&
             (joined[at + 1u] == '.') )
        {
            // back past the last name, and the slash before it
            while ( (length > root) &&
                    (out[length - 1u] != '/') )
            {
                --length;
            }

            length = (length > root) ? (length - 1u) : length;
        }
        else if ( (size > 0u) &&
                  ( (size != 1u) || (joined[at] != '.') ) )
        {
            // a slash between it and the name before
            if (length > root)
            {
                out[length++] = '/';
            }

            memcpy(&out[length], &joined[at], size);
            length += size;
        }

        at = (joined[end] != '\0') ? (end + 1u) : end;
    }

    out[length] = '\0';
    free(joined);

    return out;
}


/*
d_internal_parent
  The directory a path is in, allocated; NULL at the root.
*/
static char*
d_internal_parent(
    const char* _path
)
{
    const char* const slash = strrchr(_path, '/');

    // the root has no parent
    if ( (!slash) ||
         ( (slash == _path) && (_path[1] == '\0') ) )
    {
        return NULL;
    }

    return d_internal_dup(_path,
                          (slash == _path) ? 1u : (size_t)(slash - _path));
}


/*
d_internal_kind
  What a path names, a link not followed.
*/
static enum d_git_file_kind
d_internal_kind(
    const char* _path
)
{
    struct d_git_file_info info;

    return (d_git_file_info(_path, &info)) ? info.kind : D_GIT_FILE_NONE;
}


/*
d_internal_is_dir
  Whether a path is a directory, or a link to one.
*/
static bool
d_internal_is_dir(
    const char* _path
)
{
    const enum d_git_file_kind kind = d_internal_kind(_path);

    // a link is what it leads to
    if (kind == D_GIT_FILE_LINK)
    {
        char* const real = d_git_file_real(_path);
        const bool  dir  = ( (real) &&
                             (d_internal_kind(real) == D_GIT_FILE_DIRECTORY) );

        free(real);

        return dir;
    }

    return (kind == D_GIT_FILE_DIRECTORY);
}


/*
d_internal_read_in
  A file in a directory, read whole and terminated; NULL when it is not
there.
*/
static char*
d_internal_read_in(
    const char* _directory,
    const char* _name,
    size_t*     _out_length
)
{
    char* const path = d_git_path_join(_directory, _name);
    char* const text = (path) ? d_git_file_read(path, _out_length) : NULL;

    free(path);

    return text;
}


/*
d_internal_trim_end
  Takes white space off a text's end.
*/
static void
d_internal_trim_end(
    char* _text
)
{
    size_t length = strlen(_text);

    // spaces, tabs and line endings
    while ( (length > 0u) &&
            ( (_text[length - 1u] == '\n') ||
              (_text[length - 1u] == '\r') ||
              (_text[length - 1u] == ' ')  ||
              (_text[length - 1u] == '\t') ) )
    {
        _text[--length] = '\0';
    }

    return;
}


//==============================================================================
// 2.  NAMES
//==============================================================================


/*
d_internal_hex_digit
  A hex digit's worth, in either case, or -1.
*/
static int
d_internal_hex_digit(
    char _c
)
{
    return ( (_c >= '0') && (_c <= '9') ) ? (_c - '0')
         : ( (_c >= 'a') && (_c <= 'f') ) ? (_c - 'a' + 10)
         : ( (_c >= 'A') && (_c <= 'F') ) ? (_c - 'A' + 10)
         : -1;
}


/*
d_internal_parse_hex_of
  A name in hex, `_size` bytes of it.
*/
static bool
d_internal_parse_hex_of(
    const char*       _hex,
    size_t            _size,
    struct d_git_oid* _out
)
{
    memset(_out, 0, sizeof(*_out));

    // two digits to each byte
    for (size_t at = 0u; at < _size; ++at)
    {
        const int high = d_internal_hex_digit(_hex[at * 2u]);
        const int low  = (high >= 0)
                         ? d_internal_hex_digit(_hex[(at * 2u) + 1u])
                         : -1;

        // a digit that is not one
        if (low < 0)
        {
            return false;
        }

        _out->bytes[at] = (uint8_t)((high << 4) | low);
    }

    return true;
}


size_t
d_git_repo_hash_size(
    const struct d_git_repo* _repo
)
{
    return (_repo) ? d_git_hash_size(_repo->hash) : 20u;
}


void
d_git_repo_hex(
    const struct d_git_repo* _repo,
    const struct d_git_oid*  _oid,
    char*                    _out
)
{
    static const char digits[] = "0123456789abcdef";
    const size_t      size     = d_git_repo_hash_size(_repo);

    // two digits to each byte
    for (size_t at = 0u; at < size; ++at)
    {
        _out[at * 2u]        = digits[_oid->bytes[at] >> 4];
        _out[(at * 2u) + 1u] = digits[_oid->bytes[at] & 15u];
    }

    _out[size * 2u] = '\0';

    return;
}


bool
d_git_repo_parse_hex(
    const struct d_git_repo* _repo,
    const char*              _hex,
    struct d_git_oid*        _out
)
{
    const size_t size = d_git_repo_hash_size(_repo);

    return ( (_hex) &&
             (strlen(_hex) == (size * 2u)) &&
             (d_internal_parse_hex_of(_hex, size, _out)) );
}


bool
d_git_repo_same_oid(
    const struct d_git_repo* _repo,
    const struct d_git_oid*  _left,
    const struct d_git_oid*  _right
)
{
    return (memcmp(_left->bytes,
                   _right->bytes,
                   d_git_repo_hash_size(_repo)) == 0);
}


void
d_git_oids_release(
    struct d_git_oids* _oids
)
{
    // nothing to release
    if (!_oids)
    {
        return;
    }

    free(_oids->items);
    memset(_oids, 0, sizeof(*_oids));

    return;
}


/*
d_internal_oids_add
  One more name at the end.
*/
static bool
d_internal_oids_add(
    struct d_git_oids*      _oids,
    const struct d_git_oid* _oid
)
{
    // room for one more
    if (_oids->count == _oids->capacity)
    {
        const size_t            wanted = (_oids->capacity > 0u)
                                         ? (_oids->capacity * 2u)
                                         : 16u;
        struct d_git_oid* const grown  = realloc(_oids->items,
                                                 wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            return false;
        }

        _oids->items    = grown;
        _oids->capacity = wanted;
    }

    _oids->items[_oids->count] = *_oid;
    ++_oids->count;

    return true;
}


//==============================================================================
// 3.  OPENING
//==============================================================================


/*
d_internal_headref
  Whether a HEAD reads as git's validate_headref wants one: a link into
refs/, "ref:" and a ref in refs/, or a name in hex -- forty digits, as git
reads a HEAD before it knows the repository's hash.
*/
static bool
d_internal_headref(
    const char* _path
)
{
    size_t length = 0u;

    // a link into refs/
    if (d_internal_kind(_path) == D_GIT_FILE_LINK)
    {
        char* const target = d_git_file_link(_path, &length);
        const bool  refs   = ( (target) &&
                               (strncmp(target, "refs/", 5u) == 0) );

        free(target);

        return refs;
    }

    char* const text = d_git_file_read(_path, &length);
    bool        head = false;

    // a symbolic ref, or a name
    if ( (text) &&
         (strncmp(text, "ref:", 4u) == 0) )
    {
        const char* at = &text[4];

        // white space after the colon
        while ( (*at == ' ') ||
                (*at == '\t') ||
                (*at == '\n') ||
                (*at == '\r') )
        {
            ++at;
        }

        head = (strncmp(at, "refs/", 5u) == 0);
    }
    else if (text)
    {
        struct d_git_oid oid;

        head = ( (length >= 40u) &&
                 (d_internal_parse_hex_of(text, 20u, &oid)) );
    }

    free(text);

    return head;
}


/*
d_internal_common_of
  The common directory of a repository's directory: what its commondir
file names, relative to it, or itself.
*/
static char*
d_internal_common_of(
    const char* _git_dir
)
{
    size_t      length = 0u;
    char* const named  = d_internal_read_in(_git_dir, "commondir", &length);

    // no commondir: the directory is its own
    if (!named)
    {
        return d_internal_dup_text(_git_dir);
    }

    d_internal_trim_end(named);

    char* const common = d_internal_absolute(_git_dir, named);

    free(named);

    return common;
}


/*
d_internal_is_git_dir
  Whether a directory is a repository's, as git's is_git_directory proves
one: a HEAD that reads as one, and objects -- or GIT_OBJECT_DIRECTORY --
and refs in its common directory.
*/
static bool
d_internal_is_git_dir(
    const char* _directory
)
{
    char* const head = d_git_path_join(_directory, "HEAD");
    bool        is   = ( (head) && (d_internal_headref(head)) );

    free(head);

    // the common directory's objects and refs
    if (is)
    {
        char* const       common  = d_internal_common_of(_directory);
        const char* const store   = getenv("GIT_OBJECT_DIRECTORY");
        char* const       objects = (common) ? d_git_path_join(common,
                                                               "objects")
                                             : NULL;
        char* const       refs    = (common) ? d_git_path_join(common, "refs")
                                             : NULL;

        is = ( (objects) &&
               (refs)    &&
               (d_internal_is_dir( (store) ? store : objects )) &&
               (d_internal_is_dir(refs)) );
        free(common);
        free(objects);
        free(refs);
    }

    return is;
}


/*
d_internal_gitfile
  The repository a .git file names, `gitdir: path`, the path relative to
the file's directory; NULL, said, for a file that names none.
*/
static char*
d_internal_gitfile(
    struct d_git_repo* _repo,
    const char*        _directory,
    const char*        _file
)
{
    size_t      length = 0u;
    char* const text   = d_git_file_read(_file, &length);

    // a file that begins as one
    if ( (!text) ||
         (strncmp(text, "gitdir: ", 8u) != 0) )
    {
        free(text);
        (void)d_git_repo_fail(_repo, "invalid gitfile format", _file);

        return NULL;
    }

    d_internal_trim_end(text);

    char* const git_dir = d_internal_absolute(_directory, &text[8]);

    free(text);

    // what it names must be a repository
    if ( (git_dir) &&
         (!d_internal_is_git_dir(git_dir)) )
    {
        (void)d_git_repo_fail(_repo, "not a git repository", git_dir);
        free(git_dir);

        return NULL;
    }

    return git_dir;
}


/*
d_internal_is_ceiling
  Whether a directory is one of GIT_CEILING_DIRECTORIES, which the walk up
does not go into.
*/
static bool
d_internal_is_ceiling(
    const char* _directory
)
{
    const char* const ceilings = getenv("GIT_CEILING_DIRECTORIES");
    const size_t      length   = strlen(_directory);

    // each of the list, apart by colons
    for (const char* at = ceilings; ( (at) && (*at != '\0') );)
    {
        const char* const colon = strchr(at, ':');
        size_t            size  = (colon) ? (size_t)(colon - at) : strlen(at);

        // a trailing slash does not count
        while ( (size > 1u) &&
                (at[size - 1u] == '/') )
        {
            --size;
        }

        // the directory itself
        if ( (size == length) &&
             (memcmp(at, _directory, length) == 0) )
        {
            return true;
        }

        at = (colon) ? (colon + 1) : NULL;
    }

    return false;
}


// d_internal_found
//   struct: where a repository was found: its directory, and its work
//   tree, NULL for a bare one.
struct d_internal_found
{
    char* git_dir;
    char* work_tree;
};


/*
d_internal_discover_at
  One directory of the walk up: a .git file, a .git directory that is a
repository's, or the directory itself a repository's. False with nothing
found to go on; false, said, for a .git file that names none.
*/
static bool
d_internal_discover_at(
    struct d_git_repo*       _repo,
    const char*              _directory,
    struct d_internal_found* _out,
    bool*                    _out_broken
)
{
    char* const                dot  = d_git_path_join(_directory, ".git");
    const enum d_git_file_kind kind = (dot) ? d_internal_kind(dot)
                                            : D_GIT_FILE_NONE;

    *_out_broken = false;

    // a .git file names the repository
    if ( (kind == D_GIT_FILE_REGULAR) ||
         ( (kind == D_GIT_FILE_LINK) && (!d_internal_is_dir(dot)) ) )
    {
        _out->git_dir   = d_internal_gitfile(_repo, _directory, dot);
        _out->work_tree = (_out->git_dir) ? d_internal_dup_text(_directory)
                                          : NULL;
        *_out_broken    = (!_out->git_dir);
        free(dot);

        return (_out->git_dir != NULL);
    }

    // a .git directory, or the directory itself
    if ( (dot) &&
         (d_internal_is_dir(dot)) &&
         (d_internal_is_git_dir(dot)) )
    {
        _out->git_dir   = dot;
        _out->work_tree = d_internal_dup_text(_directory);

        return true;
    }

    free(dot);

    // a bare repository
    if (d_internal_is_git_dir(_directory))
    {
        _out->git_dir   = d_internal_dup_text(_directory);
        _out->work_tree = NULL;

        return true;
    }

    return false;
}


/*
d_internal_discover
  The walk up from a directory, as git's setup does it, to the root or a
ceiling.
*/
static bool
d_internal_discover(
    struct d_git_repo*       _repo,
    const char*              _start,
    struct d_internal_found* _out
)
{
    char* directory = d_internal_dup_text(_start);

    // from the directory up
    while (directory)
    {
        bool broken = false;

        // found here, or a .git file that names nothing
        if ( (d_internal_discover_at(_repo, directory, _out, &broken)) ||
             (broken) )
        {
            free(directory);

            return !broken;
        }

        char* const parent = d_internal_parent(directory);

        free(directory);
        directory = ( (parent) && (!d_internal_is_ceiling(parent)) )
                    ? parent
                    : NULL;

        // a ceiling is not gone into
        if ( (parent) &&
             (!directory) )
        {
            free(parent);
        }
    }

    return d_git_repo_fail(_repo,
                           "not a git repository (or any of the parent "
                           "directories)",
                           _start);
}


// d_internal_format
//   struct: what a repository's own configuration says of its format.
struct d_internal_format
{
    int64_t version;
    bool    sha256;
    bool    worktree_config;
    bool    bare;
    bool    bare_set;
    char*   worktree;
};


/*
d_internal_known_extension
  Whether a repository of format 1 may name an extension: those git 2.43
knows, which change nothing this reads.
*/
static bool
d_internal_known_extension(
    const char* _name
)
{
    static const char* const known[] = { "noop", "noop-v1", "preciousobjects",
                                         "partialclone", "worktreeconfig",
                                         "objectformat", "compatobjectformat",
                                         "refstorage" };

    // each one known
    for (size_t at = 0u; at < (sizeof(known) / sizeof(known[0])); ++at)
    {
        // one of them
        if (strcmp(_name, known[at]) == 0)
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_format_entry
  One setting of the repository's own configuration, as git's
read_repository_format reads it: the format's version, its extensions --
objectformat sha256, refstorage refused -- core.bare and core.worktree.
*/
static bool
d_internal_format_entry(
    struct d_git_repo*        _repo,
    const char*               _key,
    const char*               _value,
    struct d_internal_format* _out
)
{
    // the version, its extensions, and the work tree
    if (strcmp(_key, "core.repositoryformatversion") == 0)
    {
        return ( (_value) && (d_git_text_int(_value, &_out->version)) )
               ? true
               : d_git_repo_fail(_repo, "bad repository format version",
                                 _value);
    }
    else if (strcmp(_key, "core.bare") == 0)
    {
        _out->bare_set = d_git_text_bool(_value, &_out->bare);
    }
    else if (strcmp(_key, "core.worktree") == 0)
    {
        free(_out->worktree);
        _out->worktree = d_internal_dup_text((_value) ? _value : "");
    }
    else if (strcmp(_key, "extensions.objectformat") == 0)
    {
        _out->sha256 = ( (_value) && (strcmp(_value, "sha256") == 0) );

        return ( (_value) &&
                 ( (_out->sha256) || (strcmp(_value, "sha1") == 0) ) )
               ? true
               : d_git_repo_fail(_repo, "invalid value for "
                                        "extensions.objectformat", _value);
    }
    else if (strcmp(_key, "extensions.worktreeconfig") == 0)
    {
        (void)d_git_text_bool(_value, &_out->worktree_config);
    }
    else if (strcmp(_key, "extensions.refstorage") == 0)
    {
        return ( (_value) && (strcmp(_value, "files") == 0) )
               ? true
               : d_git_repo_fail(_repo, "refs that are not files are not "
                                        "read", _value);
    }

    return true;
}


/*
d_internal_read_format
  Reads the repository's own configuration for its format: version 0 or
1, and of 1 only extensions git knows.
  Long as each extension git knows is checked in turn.
*/
static bool
d_internal_read_format(
    struct d_git_repo*        _repo,
    struct d_internal_format* _out
)
{
    size_t      length = 0u;
    char* const text   = d_internal_read_in(_repo->common_dir,
                                            "config",
                                            &length);
    struct d_git_text records;
    char              problem[64];
    bool              read = true;

    memset(_out, 0, sizeof(*_out));

    // no configuration is format 0
    if (!text)
    {
        return true;
    }

    read = d_git_text_read(&records,
                           text,
                           length,
                           D_GIT_TEXT_CONFIG,
                           problem,
                           sizeof(problem));

    // each setting
    for (size_t at = 0u; ( (read) && (at < records.entry_count) ); ++at)
    {
        read = d_internal_format_entry(
                   _repo,
                   d_git_text_string(&records, records.entries[at].key),
                   d_git_text_string(&records, records.entries[at].value),
                   _out);
    }

    // of format 1, an extension git does not know refuses the repository
    for (size_t at = 0u;
         ( (read) && (_out->version >= 1) && (at < records.entry_count) );
         ++at)
    {
        const char* const key = d_git_text_string(&records,
                                                  records.entries[at].key);

        // an extension unknown
        if ( (strncmp(key, "extensions.", 11u) == 0) &&
             (!d_internal_known_extension(&key[11])) )
        {
            read = d_git_repo_fail(_repo, "unknown repository extension",
                                   &key[11]);
        }
    }

    d_git_text_release(&records);
    free(text);

    // a version this reads
    if ( (read) &&
         ( (_out->version < 0) || (_out->version > 1) ) )
    {
        read = d_git_repo_fail(_repo, "expected git repo version <= 1",
                               NULL);
    }

    return read;
}


//==============================================================================
// 4.  CONFIGURATION
//==============================================================================


// d_internal_loader
//   struct: settings being read: the repository they are for, where they
//   go, how deep the includes are, whether the work tree has a file of its
//   own, and the remote URLs hasconfig asks of
//   -- read, the first time one asks, by a pass that follows every such
//   include, `unconditional`; within one of its includes, a URL is
//   refused.
struct d_internal_loader
{
    struct d_git_repo*    repo;
    struct d_git_setting* settings;
    size_t                count;
    size_t                capacity;
    unsigned              depth;
    bool                  worktree_config;
    bool                  unconditional;
    bool                  forbid_urls;
    bool                  urls_read;
    const char**          urls;
    size_t                url_count;
};

// d_internal_source
//   struct: a file being read: its path, kept, its scope, and whether it
//   was included.
struct d_internal_source
{
    const char*      file;
    enum d_git_scope scope;
    bool             included;
};

static bool d_internal_load_text(struct d_internal_loader*       _loader,
                                 const struct d_internal_source* _source,
                                 const char*                     _text,
                                 size_t                          _length);


/*
d_internal_add_setting
  One setting more, its strings kept by the repository.
*/
static bool
d_internal_add_setting(
    struct d_internal_loader*       _loader,
    const struct d_internal_source* _source,
    const char*                     _key,
    const char*                     _value,
    size_t                          _line
)
{
    // room for one more
    if (_loader->count == _loader->capacity)
    {
        const size_t                wanted = (_loader->capacity > 0u)
                                             ? (_loader->capacity * 2u)
                                             : 64u;
        struct d_git_setting* const grown  = realloc(_loader->settings,
                                                     wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            return d_git_repo_fail(_loader->repo, "out of memory", NULL);
        }

        _loader->settings = grown;
        _loader->capacity = wanted;
    }

    struct d_git_setting* const setting = &_loader->settings[_loader->count];

    setting->key      = d_git_repo_keep(_loader->repo, _key, strlen(_key));
    setting->value    = (_value) ? d_git_repo_keep(_loader->repo,
                                                   _value,
                                                   strlen(_value))
                                 : NULL;
    setting->file     = _source->file;
    setting->line     = _line;
    setting->scope    = _source->scope;
    setting->included = _source->included;

    // its strings kept
    if ( (!setting->key) ||
         ( (_value) && (!setting->value) ) )
    {
        return d_git_repo_fail(_loader->repo, "out of memory", NULL);
    }

    ++_loader->count;

    return true;
}


/*
d_internal_home
  The user's home directory: HOME, or USERPROFILE where there is no HOME.
*/
static const char*
d_internal_home(void)
{
    const char* const home = getenv("HOME");

    return ( (home) && (home[0] != '\0') ) ? home : getenv("USERPROFILE");
}


/*
d_internal_interpolate
  A path as git's interpolate_path expands one: `~/` and a lone `~` the
home directory; NULL for `~user`, which this does not expand, or when
there is no home.
*/
static char*
d_internal_interpolate(
    const char* _path
)
{
    // a path that does not begin with `~` is as it is
    if (_path[0] != '~')
    {
        return d_internal_dup_text(_path);
    }

    const char* const home = d_internal_home();

    // `~` or `~/`, and a home to put there
    if ( (!home) ||
         ( (_path[1] != '\0') && (_path[1] != '/') ) )
    {
        return NULL;
    }

    return d_git_path_join(home, (_path[1] == '/') ? &_path[2] : "");
}


/*
d_internal_load_file
  A file's settings, if it is there: git reads past a file that is not.
*/
static bool
d_internal_load_file(
    struct d_internal_loader*       _loader,
    const struct d_internal_source* _source
)
{
    size_t      length = 0u;
    char* const text   = d_git_file_read(_source->file, &length);

    // a file that is not there has no settings
    if (!text)
    {
        return true;
    }

    const bool loaded = d_internal_load_text(_loader, _source, text, length);

    free(text);

    return loaded;
}


/*
d_internal_include
  An include: its path expanded, made relative to the including file's
directory when it is not absolute, and read in the including file's scope
-- not past the depth git allows.
*/
static bool
d_internal_include(
    struct d_internal_loader*       _loader,
    const struct d_internal_source* _source,
    const char*                     _path
)
{
    char* const expanded = (_path) ? d_internal_interpolate(_path) : NULL;

    // a path that expands
    if (!expanded)
    {
        return d_git_repo_fail(_loader->repo,
                               (_path) ? "could not expand include path"
                                       : "missing value for include.path",
                               _path);
    }

    // a relative path needs a file to be relative to
    if ( (!d_internal_is_absolute(expanded)) &&
         (_source->file[0] == '\0') )
    {
        free(expanded);

        return d_git_repo_fail(_loader->repo,
                               "relative config includes must come from "
                               "files",
                               NULL);
    }

    char* const directory = d_internal_parent(_source->file);
    char* const path      = d_internal_absolute((directory) ? directory : "/",
                                                expanded);

    free(directory);
    free(expanded);

    // the depth git allows
    if ( (!path) ||
         (_loader->depth >= D_INTERNAL_INCLUDE_DEPTH) )
    {
        free(path);

        return d_git_repo_fail(_loader->repo,
                               (path) ? "exceeded maximum include depth"
                                      : "out of memory",
                               NULL);
    }

    struct d_internal_source source = { d_git_repo_keep(_loader->repo,
                                                        path,
                                                        strlen(path)),
                                        _source->scope,
                                        true };

    free(path);
    ++_loader->depth;

    const bool loaded = ( (source.file) &&
                          (d_internal_load_file(_loader, &source)) );

    --_loader->depth;

    return loaded;
}


/*
d_internal_gitdir_try
  One try of a gitdir: pattern against a repository's path: the literal
part a `./` pattern begins with compared first, the rest by wildmatch.
*/
static bool
d_internal_gitdir_try(
    const char* _pattern,
    size_t      _prefix,
    const char* _text,
    bool        _icase
)
{
    const size_t length = strlen(_text);

    // the literal part
    if (_prefix > 0u)
    {
        // too short, or unlike
        if (length < _prefix)
        {
            return false;
        }

        // compared in either case, when asked
        for (size_t at = 0u; at < _prefix; ++at)
        {
            const char left  = ( (_icase) &&
                                 (_pattern[at] >= 'A') &&
                                 (_pattern[at] <= 'Z') )
                               ? (char)(_pattern[at] + 32)
                               : _pattern[at];
            const char right = ( (_icase) &&
                                 (_text[at] >= 'A') &&
                                 (_text[at] <= 'Z') )
                               ? (char)(_text[at] + 32)
                               : _text[at];

            // a byte unlike
            if (left != right)
            {
                return false;
            }
        }
    }

    return d_git_text_wildmatch(&_pattern[_prefix],
                                strlen(&_pattern[_prefix]),
                                &_text[_prefix],
                                length - _prefix,
                                D_GIT_TEXT_PATHNAME
                                    | ((_icase) ? D_GIT_TEXT_CASEFOLD : 0u));
}


/*
d_internal_gitdir_pattern
  A gitdir: condition's pattern, as git's prepare_include_condition_pattern
makes it: expanded; `./` made the including file's directory, its real
name, which `_out_prefix` counts; another that is not absolute put below
`**` and a slash; one that ends in `/` given `**` after it.
*/
static char*
d_internal_gitdir_pattern(
    const char* _condition,
    size_t      _length,
    const char* _file,
    size_t*     _out_prefix
)
{
    char* const written  = d_internal_dup(_condition, _length);
    char* const expanded = (written) ? d_internal_interpolate(written) : NULL;
    char*       real     = NULL;
    struct d_git_buffer pattern;

    memset(&pattern, 0, sizeof(pattern));
    *_out_prefix = 0u;

    // a relative pattern, `./`, or one not absolute
    if ( (expanded) &&
         (expanded[0] == '.') &&
         (expanded[1] == '/') )
    {
        real = d_git_file_real(_file);

        const char* const slash = (real) ? strrchr(real, '/') : NULL;

        (void)d_git_buffer_add(&pattern,
                               (slash) ? real : "",
                               (slash) ? (size_t)(slash - real) : 0u);
        *_out_prefix = pattern.length + 1u;
        (void)d_git_buffer_add_text(&pattern, &expanded[1]);
    }
    else if (expanded)
    {
        (void)d_git_buffer_add_text(&pattern,
                                    (d_internal_is_absolute(expanded))
                                    ? ""
                                    : "**/");
        (void)d_git_buffer_add_text(&pattern, expanded);
    }

    // a directory's pattern takes all below it
    if ( (pattern.length > 0u) &&
         (pattern.text[pattern.length - 1u] == '/') )
    {
        (void)d_git_buffer_add_text(&pattern, "**");
    }

    free(written);
    free(expanded);
    free(real);

    return d_git_buffer_take(&pattern);
}


/*
d_internal_by_gitdir
  Whether a gitdir: condition holds: its pattern against the repository's
directory's real name, then against its name as written.
*/
static bool
d_internal_by_gitdir(
    const struct d_internal_loader* _loader,
    const char*                     _condition,
    size_t                          _length,
    const char*                     _file,
    bool                            _icase
)
{
    size_t      prefix  = 0u;
    char* const pattern = d_internal_gitdir_pattern(_condition,
                                                    _length,
                                                    _file,
                                                    &prefix);
    char* const real    = d_git_file_real(_loader->repo->git_dir);
    const bool  holds   = ( (pattern) &&
                            ( ( (real) &&
                                (d_internal_gitdir_try(pattern,
                                                       prefix,
                                                       real,
                                                       _icase)) ) ||
                              (d_internal_gitdir_try(pattern,
                                                     prefix,
                                                     _loader->repo->git_dir,
                                                     _icase)) ) );

    free(pattern);
    free(real);

    return holds;
}


/*
d_internal_branch
  The branch HEAD names, past refs/heads/, allocated; NULL for a HEAD that
names none.
*/
static char*
d_internal_branch(
    const struct d_git_repo* _repo
)
{
    size_t      length = 0u;
    char* const text   = d_internal_read_in(_repo->git_dir, "HEAD", &length);
    char*       branch = NULL;

    // a symbolic ref to a branch
    if ( (text) &&
         (strncmp(text, "ref:", 4u) == 0) )
    {
        const char* at = &text[4];

        // the blanks after the colon
        while ( (*at == ' ') ||
                (*at == '\t') )
        {
            ++at;
        }

        d_internal_trim_end(text);
        branch = (strncmp(at, "refs/heads/", 11u) == 0)
                 ? d_internal_dup_text(&at[11])
                 : NULL;
    }

    free(text);

    return branch;
}


/*
d_internal_by_branch
  Whether an onbranch: condition holds: its pattern, `**` after a trailing
`/`, against the branch HEAD names.
*/
static bool
d_internal_by_branch(
    const struct d_internal_loader* _loader,
    const char*                     _condition,
    size_t                          _length
)
{
    char* const         branch  = d_internal_branch(_loader->repo);
    struct d_git_buffer pattern;

    memset(&pattern, 0, sizeof(pattern));
    (void)d_git_buffer_add(&pattern, _condition, _length);

    // a directory's pattern takes all below it
    if ( (_length > 0u) &&
         (_condition[_length - 1u] == '/') )
    {
        (void)d_git_buffer_add_text(&pattern, "**");
    }

    const bool holds = ( (branch) &&
                         (!pattern.failed) &&
                         (d_git_text_wildmatch(pattern.text,
                                               pattern.length,
                                               branch,
                                               strlen(branch),
                                               D_GIT_TEXT_PATHNAME)) );

    free(branch);
    d_git_buffer_free(&pattern);

    return holds;
}


static bool d_internal_load_all(struct d_internal_loader* _loader);


/*
d_internal_read_urls
  Every remote's URL, read by a pass of its own that follows every
hasconfig include, as git's populate_remote_urls reads them.
*/
static bool
d_internal_read_urls(
    struct d_internal_loader* _loader
)
{
    struct d_internal_loader pass;

    memset(&pass, 0, sizeof(pass));
    pass.repo            = _loader->repo;
    pass.worktree_config = _loader->worktree_config;
    pass.unconditional   = true;

    // the pass, whole
    if (!d_internal_load_all(&pass))
    {
        free(pass.settings);

        return false;
    }

    _loader->urls      = calloc(pass.count + 1u, sizeof(*_loader->urls));
    _loader->urls_read = (_loader->urls != NULL);

    // each remote.*.url
    for (size_t at = 0u; ( (_loader->urls) && (at < pass.count) ); ++at)
    {
        const char* const key  = pass.settings[at].key;
        const char* const last = strrchr(key, '.');

        // a remote's url, with a subsection and a value
        if ( (strncmp(key, "remote.", 7u) == 0)  &&
             (last > &key[6])                     &&
             (strcmp(last, ".url") == 0)          &&
             (pass.settings[at].value) )
        {
            _loader->urls[_loader->url_count++] = pass.settings[at].value;
        }
    }

    free(pass.settings);

    return (_loader->urls_read) ? true
                                : d_git_repo_fail(_loader->repo,
                                                  "out of memory",
                                                  NULL);
}


/*
d_internal_by_url
  Whether a hasconfig:remote.*.url: condition holds: always, in the pass
that reads the URLs; otherwise when a URL matches its glob.
*/
static bool
d_internal_by_url(
    struct d_internal_loader* _loader,
    const char*               _glob,
    size_t                    _length,
    bool*                     _out_holds
)
{
    *_out_holds = _loader->unconditional;

    // the URLs, read once, before the first is asked of
    if ( (!_loader->unconditional) &&
         (!_loader->urls_read) &&
         (!d_internal_read_urls(_loader)) )
    {
        return false;
    }

    // any URL that matches
    for (size_t at = 0u;
         ( (!*_out_holds) && (at < _loader->url_count) );
         ++at)
    {
        *_out_holds = d_git_text_wildmatch(_glob,
                                           _length,
                                           _loader->urls[at],
                                           strlen(_loader->urls[at]),
                                           D_GIT_TEXT_PATHNAME);
    }

    return true;
}


/*
d_internal_condition
  Whether an includeIf's condition holds, as git's include_condition_is_true
decides it; one it does not know never does.
*/
static bool
d_internal_condition(
    struct d_internal_loader*       _loader,
    const struct d_internal_source* _source,
    const char*                     _condition,
    size_t                          _length,
    bool*                           _out_holds
)
{
    static const char url[] = "hasconfig:remote.*.url:";

    *_out_holds = false;

    // each kind of condition, by its prefix
    if ( (_length >= 7u) &&
         (strncmp(_condition, "gitdir:", 7u) == 0) )
    {
        *_out_holds = d_internal_by_gitdir(_loader, &_condition[7],
                                           _length - 7u, _source->file,
                                           false);
    }
    else if ( (_length >= 9u) &&
              (strncmp(_condition, "gitdir/i:", 9u) == 0) )
    {
        *_out_holds = d_internal_by_gitdir(_loader, &_condition[9],
                                           _length - 9u, _source->file,
                                           true);
    }
    else if ( (_length >= 9u) &&
              (strncmp(_condition, "onbranch:", 9u) == 0) )
    {
        *_out_holds = d_internal_by_branch(_loader, &_condition[9],
                                           _length - 9u);
    }
    else if ( (_length >= (sizeof(url) - 1u)) &&
              (strncmp(_condition, url, sizeof(url) - 1u) == 0) )
    {
        return d_internal_by_url(_loader,
                                 &_condition[sizeof(url) - 1u],
                                 _length - (sizeof(url) - 1u),
                                 _out_holds);
    }

    return true;
}


/*
d_internal_setting_includes
  What a setting just read includes: include.path its path; an
includeIf.<condition>.path its path when the condition holds -- read, in
the pass that reads URLs, with a URL in it refused.
*/
static bool
d_internal_setting_includes(
    struct d_internal_loader*       _loader,
    const struct d_internal_source* _source,
    const char*                     _key,
    const char*                     _value
)
{
    const char* const last = strrchr(_key, '.');
    bool              holds = false;

    // a URL where a hasconfig include may not have one
    if ( (_loader->forbid_urls)                 &&
         (strncmp(_key, "remote.", 7u) == 0)    &&
         (last > &_key[6])                      &&
         (strcmp(last, ".url") == 0) )
    {
        return d_git_repo_fail(_loader->repo,
                               "remote URLs cannot be configured in file "
                               "directly or indirectly included by "
                               "includeIf.hasconfig:remote.*.url",
                               NULL);
    }

    // include.path, or includeIf's
    if (strcmp(_key, "include.path") == 0)
    {
        return d_internal_include(_loader, _source, _value);
    }
    else if ( (strncmp(_key, "includeif.", 10u) != 0) ||
              (last <= &_key[9])                       ||
              (strcmp(last, ".path") != 0) )
    {
        return true;
    }

    const char* const condition = &_key[10];
    const size_t      length    = (size_t)(last - condition);

    // the condition, and the include if it holds
    if (!d_internal_condition(_loader, _source, condition, length, &holds))
    {
        return false;
    }
    else if (!holds)
    {
        return true;
    }

    const bool forbid  = _loader->forbid_urls;
    const bool hascfg  = (strncmp(condition, "hasconfig:", 10u) == 0);

    _loader->forbid_urls = ( (forbid) ||
                             ( (_loader->unconditional) && (hascfg) ) );

    const bool loaded = d_internal_include(_loader, _source, _value);

    _loader->forbid_urls = forbid;

    return loaded;
}


/*
d_internal_load_text
  A file's text read as git reads it, each setting kept and its includes
followed in its place. A file git cannot read to its end git refuses, and
so does this.
*/
static bool
d_internal_load_text(
    struct d_internal_loader*       _loader,
    const struct d_internal_source* _source,
    const char*                     _text,
    size_t                          _length
)
{
    struct d_git_text records;
    char              problem[64];

    // the records
    if (!d_git_text_read(&records,
                         _text,
                         _length,
                         D_GIT_TEXT_CONFIG,
                         problem,
                         sizeof(problem)))
    {
        d_git_text_release(&records);

        return d_git_repo_fail(_loader->repo, problem, _source->file);
    }

    // a file git cannot read to its end
    if (records.broken != D_GIT_TEXT_NONE)
    {
        (void)snprintf(problem,
                       sizeof(problem),
                       "bad config line %zu in file",
                       records.broken + 1u);
        d_git_text_release(&records);

        return d_git_repo_fail(_loader->repo, problem, _source->file);
    }

    bool loaded = true;

    // each setting, then what it includes
    for (size_t at = 0u; ( (loaded) && (at < records.entry_count) ); ++at)
    {
        const struct d_git_text_entry* const entry = &records.entries[at];
        const char* const key   = d_git_text_string(&records, entry->key);
        const char* const value = d_git_text_string(&records, entry->value);

        loaded = ( (d_internal_add_setting(_loader,
                                           _source,
                                           key,
                                           value,
                                           entry->line + 1u)) &&
                   (d_internal_setting_includes(_loader,
                                                _source,
                                                key,
                                                value)) );
    }

    d_git_text_release(&records);

    return loaded;
}


/*
d_internal_load_scope
  A scope's file, if it names one: a path kept, read.
*/
static bool
d_internal_load_scope(
    struct d_internal_loader* _loader,
    const char*               _path,
    enum d_git_scope          _scope
)
{
    // no file, no settings
    if ( (!_path) ||
         (_path[0] == '\0') )
    {
        return true;
    }

    struct d_internal_source source = { d_git_repo_keep(_loader->repo,
                                                        _path,
                                                        strlen(_path)),
                                        _scope,
                                        false };

    return (source.file) ? d_internal_load_file(_loader, &source)
                         : d_git_repo_fail(_loader->repo, "out of memory",
                                           NULL);
}


/*
d_internal_load_global
  The user's files: GIT_CONFIG_GLOBAL alone when it is set; otherwise git's
own directory under XDG_CONFIG_HOME, or ~/.config, then ~/.gitconfig.
*/
static bool
d_internal_load_global(
    struct d_internal_loader* _loader
)
{
    const char* const named = getenv("GIT_CONFIG_GLOBAL");

    // the one file named
    if (named)
    {
        return d_internal_load_scope(_loader, named, D_GIT_SCOPE_GLOBAL);
    }

    const char* const xdg  = getenv("XDG_CONFIG_HOME");
    const char* const home = d_internal_home();
    char* const       base = ( (xdg) && (xdg[0] != '\0') )
                             ? d_internal_dup_text(xdg)
                             : ( (home) ? d_git_path_join(home, ".config")
                                        : NULL );
    char* const       own  = (base) ? d_git_path_join(base, "git/config")
                                    : NULL;
    char* const       user = (home) ? d_git_path_join(home, ".gitconfig")
                                    : NULL;
    const bool        read = ( (d_internal_load_scope(_loader,
                                                      own,
                                                      D_GIT_SCOPE_GLOBAL)) &&
                               (d_internal_load_scope(_loader,
                                                      user,
                                                      D_GIT_SCOPE_GLOBAL)) );

    free(base);
    free(own);
    free(user);

    return read;
}


/*
d_internal_command_pair
  One setting of the command's: its key made canonical, as git's
config_parse_pair makes it.
*/
static bool
d_internal_command_pair(
    struct d_internal_loader* _loader,
    const char*               _key,
    const char*               _value
)
{
    static const struct d_internal_source command = { "",
                                                      D_GIT_SCOPE_COMMAND,
                                                      false };
    struct d_git_buffer key;

    memset(&key, 0, sizeof(key));

    // a key git takes
    if ( (!_key) ||
         (_key[0] == '\0') ||
         (!d_git_text_key(&key, _key)) )
    {
        d_git_buffer_free(&key);

        return d_git_repo_fail(_loader->repo, "invalid key", _key);
    }

    const bool added = ( (d_internal_add_setting(_loader,
                                                 &command,
                                                 key.text,
                                                 _value,
                                                 0u)) &&
                         (d_internal_setting_includes(_loader,
                                                      &command,
                                                      key.text,
                                                      _value)) );

    d_git_buffer_free(&key);

    return added;
}


/*
d_internal_load_count
  GIT_CONFIG_COUNT's settings: GIT_CONFIG_KEY_n and GIT_CONFIG_VALUE_n for
each n below the count.
*/
static bool
d_internal_load_count(
    struct d_internal_loader* _loader
)
{
    const char* const count = getenv("GIT_CONFIG_COUNT");
    int64_t           total = 0;

    // no count, no settings
    if (!count)
    {
        return true;
    }
    else if ( (!d_git_text_int(count, &total)) ||
              (total < 0) ||
              (total > 2147483647) )
    {
        return d_git_repo_fail(_loader->repo, "bogus count in "
                                              "GIT_CONFIG_COUNT", count);
    }

    // each pair
    for (int64_t at = 0; at < total; ++at)
    {
        char name[48];

        (void)snprintf(name, sizeof(name), "GIT_CONFIG_KEY_%d", (int)at);

        const char* const key = getenv(name);

        (void)snprintf(name, sizeof(name), "GIT_CONFIG_VALUE_%d", (int)at);

        const char* const value = getenv(name);

        // both must be there
        if ( (!key) ||
             (!value) )
        {
            return d_git_repo_fail(_loader->repo,
                                   "missing config key or value", name);
        }

        // the pair
        if (!d_internal_command_pair(_loader, key, value))
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_sq_step
  One single-quoted word of GIT_CONFIG_PARAMETERS, as git's
sq_dequote_step reads it: within quotes as it is, and between quotes `\'`
or `\!` as themselves. `*_at` is left past it.
*/
static bool
d_internal_sq_step(
    const char**         _at,
    struct d_git_buffer* _out
)
{
    const char* at = *_at;

    // a word begins with a quote
    if (*at != '\'')
    {
        return false;
    }

    // to the quote that ends it
    for (;;)
    {
        const char c = *++at;

        // a word left open
        if (c == '\0')
        {
            return false;
        }
        else if (c != '\'')
        {
            (void)d_git_buffer_add_char(_out, c);

            continue;
        }

        // out of the quotes: an escaped quote or bang, and back in
        if ( (at[1] == '\\') &&
             ( (at[2] == '\'') || (at[2] == '!') ) &&
             (at[3] == '\'') )
        {
            (void)d_git_buffer_add_char(_out, at[2]);
            at += 3;

            continue;
        }

        *_at = at + 1;

        return !_out->failed;
    }
}


/*
d_internal_parameter
  One word of GIT_CONFIG_PARAMETERS: 'key=value' of old, split at its first
`=` -- a key with none is a name alone -- or 'key'='value' and 'key'= of
new.
*/
static bool
d_internal_parameter(
    struct d_internal_loader* _loader,
    const char**              _at
)
{
    struct d_git_buffer key;
    struct d_git_buffer value;
    bool                done = false;

    memset(&key, 0, sizeof(key));
    memset(&value, 0, sizeof(value));

    // a quoted word, then old style or new
    if (!d_internal_sq_step(_at, &key))
    {
        done = false;
    }
    else if ( (**_at == '\0') ||
              (**_at == ' ') )
    {
        char* const equals = strchr(key.text, '=');

        // a name alone, or a value after the `=`
        if (equals)
        {
            *equals = '\0';
        }

        done = d_internal_command_pair(_loader, key.text,
                                       (equals) ? (equals + 1) : NULL);
    }
    else if ( (**_at == '=') &&
              ( ((*_at)[1] == '\0') || ((*_at)[1] == ' ') ) )
    {
        ++*_at;
        done = d_internal_command_pair(_loader, key.text, NULL);
    }
    else if (**_at == '=')
    {
        ++*_at;
        done = ( (d_internal_sq_step(_at, &value)) &&
                 (d_internal_command_pair(_loader,
                                          key.text,
                                          (value.text) ? value.text : "")) );
    }

    d_git_buffer_free(&key);
    d_git_buffer_free(&value);

    return done;
}


/*
d_internal_load_parameters
  GIT_CONFIG_PARAMETERS's settings, its words apart by spaces.
*/
static bool
d_internal_load_parameters(
    struct d_internal_loader* _loader
)
{
    const char* at = getenv("GIT_CONFIG_PARAMETERS");

    // each word
    while ( (at) &&
            (*at != '\0') )
    {
        // the blanks between words
        if (*at == ' ')
        {
            ++at;

            continue;
        }

        // a word that is one
        if (!d_internal_parameter(_loader, &at))
        {
            return ( (_loader->repo->problem[0] != '\0') )
                   ? false
                   : d_git_repo_fail(_loader->repo,
                                     "bogus format in "
                                     "GIT_CONFIG_PARAMETERS",
                                     NULL);
        }
    }

    return true;
}


/*
d_internal_load_all
  Every scope's settings, in git's order: the system's -- unless
GIT_CONFIG_NOSYSTEM -- the user's, the repository's, the work tree's when
extensions.worktreeConfig is set, and the command's.
*/
static bool
d_internal_load_all(
    struct d_internal_loader* _loader
)
{
    struct d_git_repo* const repo     = _loader->repo;
    const char* const        nosystem = getenv("GIT_CONFIG_NOSYSTEM");
    const char* const        system   = getenv("GIT_CONFIG_SYSTEM");
    char* const              local    = d_git_path_join(repo->common_dir,
                                                        "config");
    char* const              worktree = d_git_path_join(repo->git_dir,
                                                        "config.worktree");
    bool                     skip     = false;

    // a setting git reads as a boolean
    if (nosystem)
    {
        (void)d_git_text_bool(nosystem, &skip);
    }

    const bool loaded =
        ( (local)                                                     &&
          (worktree)                                                  &&
          ( (skip) ||
            (d_internal_load_scope(_loader,
                                   (system) ? system : "/etc/gitconfig",
                                   D_GIT_SCOPE_SYSTEM)) )             &&
          (d_internal_load_global(_loader))                           &&
          (d_internal_load_scope(_loader, local, D_GIT_SCOPE_LOCAL))  &&
          ( (!_loader->worktree_config) ||
            (d_internal_load_scope(_loader,
                                   worktree,
                                   D_GIT_SCOPE_WORKTREE)) )           &&
          (d_internal_load_count(_loader))                            &&
          (d_internal_load_parameters(_loader)) );

    free(local);
    free(worktree);

    return loaded;
}


/*
d_internal_repo_dirs
  Where the repository is: GIT_DIR's, proved one, or the one the walk up
from `_start` finds; and its common directory, GIT_COMMON_DIR's when that
is set. `_out_explicit` says GIT_DIR named it.
*/
static bool
d_internal_repo_dirs(
    struct d_git_repo*       _repo,
    const char*              _cwd,
    const char*              _start,
    struct d_internal_found* _out,
    bool*                    _out_explicit
)
{
    const char* const named  = getenv("GIT_DIR");
    const char* const common = getenv("GIT_COMMON_DIR");

    memset(_out, 0, sizeof(*_out));
    *_out_explicit = ( (named) && (named[0] != '\0') );

    // GIT_DIR, which must be a repository's
    if (*_out_explicit)
    {
        _out->git_dir = d_internal_absolute(_cwd, named);

        // a repository there
        if ( (!_out->git_dir) ||
             (!d_internal_is_git_dir(_out->git_dir)) )
        {
            (void)d_git_repo_fail(_repo, "not a git repository", named);
            free(_out->git_dir);
            _out->git_dir = NULL;

            return false;
        }
    }
    else if (!d_internal_discover(_repo, _start, _out))
    {
        return false;
    }

    _repo->git_dir    = _out->git_dir;
    _repo->common_dir = ( (common) && (common[0] != '\0') )
                        ? d_internal_absolute(_cwd, common)
                        : d_internal_common_of(_out->git_dir);
    _out->git_dir     = NULL;

    return (_repo->common_dir) ? true
                               : d_git_repo_fail(_repo, "out of memory",
                                                 NULL);
}


/*
d_internal_work_tree
  The repository's work tree, as git's setup decides it: GIT_WORK_TREE;
core.worktree, relative to the repository's directory; none for core.bare;
the working directory when GIT_DIR named the repository; else where the
walk found its .git. A linked work tree's common configuration decides
neither core.bare nor core.worktree.
*/
static char*
d_internal_work_tree(
    const struct d_git_repo*        _repo,
    const char*                     _cwd,
    struct d_internal_found*        _found,
    const struct d_internal_format* _format,
    bool                            _explicit
)
{
    const char* const named  = getenv("GIT_WORK_TREE");
    const bool        linked = (strcmp(_repo->git_dir, _repo->common_dir) != 0);
    char* const       found  = _found->work_tree;

    _found->work_tree = NULL;

    // each, in git's order
    if ( (named) &&
         (named[0] != '\0') )
    {
        free(found);

        return d_internal_absolute(_cwd, named);
    }
    else if ( (!linked) &&
              (_format->worktree) )
    {
        free(found);

        return d_internal_absolute(_repo->git_dir, _format->worktree);
    }
    else if ( (!linked) &&
              (_format->bare_set) &&
              (_format->bare) )
    {
        free(found);

        return NULL;
    }
    else if (_explicit)
    {
        free(found);

        return d_internal_dup_text(_cwd);
    }

    return found;
}


/*
d_internal_open
  The steps of opening, once the repository has somewhere to say why one
failed: where it is, its format, its work tree, its configuration.
*/
static bool
d_internal_open(
    struct d_git_repo* _repo,
    const char*        _cwd,
    const char*        _start
)
{
    struct d_internal_found  found;
    struct d_internal_format format;
    bool                     explicit_dir = false;

    memset(&format, 0, sizeof(format));

    // where it is, and its format
    if ( (!d_internal_repo_dirs(_repo, _cwd, _start, &found, &explicit_dir)) ||
         (!d_internal_read_format(_repo, &format)) )
    {
        free(found.work_tree);
        free(format.worktree);

        return false;
    }

    _repo->hash      = (format.sha256) ? D_GIT_HASH_SHA256 : D_GIT_HASH_SHA1;
    _repo->work_tree = d_internal_work_tree(_repo,
                                            _cwd,
                                            &found,
                                            &format,
                                            explicit_dir);

    struct d_internal_loader loader;

    memset(&loader, 0, sizeof(loader));
    loader.repo            = _repo;
    loader.worktree_config = format.worktree_config;
    free(format.worktree);

    const bool loaded = d_internal_load_all(&loader);

    _repo->settings         = loader.settings;
    _repo->setting_count    = loader.count;
    _repo->setting_capacity = loader.capacity;
    free((void*)loader.urls);

    return loaded;
}


struct d_git_repo*
d_git_repo_open(
    const char* _path,
    char*       _problem,
    size_t      _capacity
)
{
    struct d_git_repo* const repo = calloc(1u, sizeof(*repo));
    char* const              cwd  = d_git_file_cwd();
    char*                    start = (cwd) ? d_internal_absolute(cwd,
                                                                 (_path)
                                                                 ? _path
                                                                 : ".")
                                           : NULL;

    // the repository, and where to begin
    if ( (!repo) ||
         (!start) )
    {
        (void)snprintf(_problem, _capacity, "out of memory");
        free(repo);
        free(cwd);
        free(start);

        return NULL;
    }

    // a file's directory is where to begin
    if (d_internal_kind(start) == D_GIT_FILE_REGULAR)
    {
        char* const parent = d_internal_parent(start);

        free(start);
        start = parent;
    }

    const bool opened = ( (start) &&
                          (d_internal_open(repo, cwd, start)) );

    free(cwd);
    free(start);

    // a repository that did not open says why, and goes
    if (!opened)
    {
        (void)snprintf(_problem,
                       _capacity,
                       "%s",
                       (repo->problem[0] != '\0') ? repo->problem
                                                  : "out of memory");
        d_git_repo_free(repo);

        return NULL;
    }

    return repo;
}


void
d_git_repo_free(
    struct d_git_repo* _repo
)
{
    // nothing to free
    if (!_repo)
    {
        return;
    }

    // the strings the settings hold
    for (size_t at = 0u; at < _repo->string_count; ++at)
    {
        free(_repo->strings[at]);
    }

    // the packed refs
    for (size_t at = 0u; at < _repo->packed_count; ++at)
    {
        free(_repo->packed[at].name);
    }

    d_git_stores_release(_repo);
    d_git_index_release(_repo);
    d_git_rules_release(_repo);
    free(_repo->packed);
    free(_repo->strings);
    free(_repo->settings);
    free(_repo->git_dir);
    free(_repo->common_dir);
    free(_repo->work_tree);
    free(_repo);

    return;
}


const char*
d_git_repo_git_dir(
    const struct d_git_repo* _repo
)
{
    return (_repo) ? _repo->git_dir : NULL;
}


const char*
d_git_repo_common_dir(
    const struct d_git_repo* _repo
)
{
    return (_repo) ? _repo->common_dir : NULL;
}


const char*
d_git_repo_work_tree(
    const struct d_git_repo* _repo
)
{
    return (_repo) ? _repo->work_tree : NULL;
}


const char*
d_git_repo_problem(
    const struct d_git_repo* _repo
)
{
    return (_repo) ? _repo->problem : "";
}


//==============================================================================
// 5.  SETTINGS
//==============================================================================


size_t
d_git_repo_settings(
    const struct d_git_repo* _repo
)
{
    return (_repo) ? _repo->setting_count : 0u;
}


const struct d_git_setting*
d_git_repo_setting(
    const struct d_git_repo* _repo,
    size_t                   _index
)
{
    return ( (_repo) && (_index < _repo->setting_count) )
           ? &_repo->settings[_index]
           : NULL;
}


size_t
d_git_repo_find_setting(
    const struct d_git_repo* _repo,
    const char*              _key,
    size_t                   _after
)
{
    struct d_git_buffer key;

    memset(&key, 0, sizeof(key));

    // a key git takes
    if ( (!_repo) ||
         (!d_git_text_key(&key, _key)) )
    {
        d_git_buffer_free(&key);

        return D_GIT_REPO_NONE;
    }

    size_t found = D_GIT_REPO_NONE;

    // from the setting after `_after`
    for (size_t at = (_after == D_GIT_REPO_NONE) ? 0u : (_after + 1u);
         at < _repo->setting_count;
         ++at)
    {
        // the key, exactly
        if (strcmp(_repo->settings[at].key, key.text) == 0)
        {
            found = at;

            break;
        }
    }

    d_git_buffer_free(&key);

    return found;
}


const char*
d_git_repo_config(
    const struct d_git_repo* _repo,
    const char*              _key,
    bool*                    _out_set
)
{
    size_t last = D_GIT_REPO_NONE;

    // each setting of the key, the last kept
    for (size_t at = d_git_repo_find_setting(_repo, _key, D_GIT_REPO_NONE);
         at != D_GIT_REPO_NONE;
         at = d_git_repo_find_setting(_repo, _key, at))
    {
        last = at;
    }

    // whether it is set, if asked
    if (_out_set)
    {
        *_out_set = (last != D_GIT_REPO_NONE);
    }

    return (last != D_GIT_REPO_NONE) ? _repo->settings[last].value : NULL;
}


bool
d_git_repo_flag(
    const struct d_git_repo* _repo,
    const char*              _key,
    bool                     _default
)
{
    bool              set   = false;
    bool              flag  = _default;
    const char* const value = d_git_repo_config(_repo, _key, &set);

    // a boolean, or the default
    if ( (!set) ||
         (!d_git_text_bool(value, &flag)) )
    {
        return _default;
    }

    return flag;
}


//==============================================================================
// 6.  REFS
//==============================================================================


// d_internal_raw
//   enum: what reading one ref, not followed, finds.
enum d_internal_raw
{
    D_INTERNAL_RAW_MISSING = 0,
    D_INTERNAL_RAW_BROKEN,
    D_INTERNAL_RAW_OID,
    D_INTERNAL_RAW_SYMBOLIC
};


/*
d_internal_refname_ok
  Whether a ref's name is one git's check_refname_format allows, one level
allowed: no name of it begins with `.` or ends with ".lock", none is
empty, and none holds "..", "@{", a control byte or any of ` ~^:?*[\`;
nor is the whole "@", or ends with `.`.
*/
static bool
d_internal_refname_ok(
    const char* _name
)
{
    const size_t length = strlen(_name);
    size_t       start  = 0u;

    // the whole must be a name, not "@", and not end with `.` or `/`
    if ( (length == 0u)                     ||
         (strcmp(_name, "@") == 0)          ||
         (_name[length - 1u] == '.')        ||
         (_name[length - 1u] == '/') )
    {
        return false;
    }

    // each byte, each name's start and end
    for (size_t at = 0u; at <= length; ++at)
    {
        const unsigned char c = (unsigned char)_name[at];

        // a name's end: not empty, not hidden, not a lock
        if ( (c == '/') ||
             (c == '\0') )
        {
            const size_t size = at - start;

            // empty, beginning with `.`, or ending with ".lock"
            if ( (size == 0u)                                  ||
                 (_name[start] == '.')                         ||
                 ( (size >= 5u) &&
                   (memcmp(&_name[at - 5u], ".lock", 5u) == 0) ) )
            {
                return false;
            }

            start = at + 1u;

            continue;
        }

        // a byte, or a pair, git refuses
        if ( (c < 0x20u) || (c == 0x7Fu) ||
             (strchr(" ~^:?*[\\", (int)c)) ||
             ( (c == '.') && (_name[at + 1u] == '.') ) ||
             ( (c == '@') && (_name[at + 1u] == '{') ) )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_worktree_ref
  Whether a ref is a work tree's own, kept in its own directory: HEAD and
the other pseudorefs -- names of capitals and `_` -- and those under
refs/worktree/, refs/bisect/ and refs/rewritten/.
*/
static bool
d_internal_worktree_ref(
    const char* _name
)
{
    // under refs/, three kinds are the work tree's
    if (strncmp(_name, "refs/", 5u) == 0)
    {
        return ( (strncmp(_name, "refs/worktree/", 14u) == 0) ||
                 (strncmp(_name, "refs/bisect/", 12u) == 0)   ||
                 (strncmp(_name, "refs/rewritten/", 15u) == 0) );
    }

    // a pseudoref's syntax
    for (const char* at = _name; *at; ++at)
    {
        // a capital or an underscore
        if ( ( (*at < 'A') || (*at > 'Z') ) &&
             (*at != '_') )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_read_loose
  One loose ref, as the files backend reads one: missing for no file or a
directory; a link into refs/ a symbolic ref; content "ref:" and a name a
symbolic ref; a name in hex, with nothing but white space after it, a
name; anything else broken.
  Long as a loose ref is read as git's files backend reads one: a symbolic
ref, a name, or neither.
*/
static enum d_internal_raw
d_internal_read_loose(
    const struct d_git_repo* _repo,
    const char*              _name,
    struct d_git_oid*        _out,
    char**                   _out_target
)
{
    const char* const          base = (d_internal_worktree_ref(_name))
                                      ? _repo->git_dir
                                      : _repo->common_dir;
    char* const                path = d_git_path_join(base, _name);
    const enum d_git_file_kind kind = (path) ? d_internal_kind(path)
                                             : D_GIT_FILE_NONE;
    size_t                     length = 0u;

    // a link into refs/
    if (kind == D_GIT_FILE_LINK)
    {
        char* const target = d_git_file_link(path, &length);

        // one into refs/ names a ref
        if ( (target) &&
             (strncmp(target, "refs/", 5u) == 0) )
        {
            free(path);
            *_out_target = target;

            return D_INTERNAL_RAW_SYMBOLIC;
        }

        free(target);
    }

    char* const text = ( (kind == D_GIT_FILE_REGULAR) ||
                         (kind == D_GIT_FILE_LINK) )
                       ? d_git_file_read(path, &length)
                       : NULL;

    free(path);

    // nothing there to read
    if (!text)
    {
        return D_INTERNAL_RAW_MISSING;
    }

    d_internal_trim_end(text);

    const size_t        size = d_git_repo_hash_size(_repo);
    enum d_internal_raw raw  = D_INTERNAL_RAW_BROKEN;

    // a symbolic ref, or a name and white space
    if (strncmp(text, "ref:", 4u) == 0)
    {
        const char* at = &text[4];

        // the white space after the colon
        while ( (*at == ' ') ||
                (*at == '\t') )
        {
            ++at;
        }

        *_out_target = d_internal_dup_text(at);
        raw          = (*_out_target) ? D_INTERNAL_RAW_SYMBOLIC
                                      : D_INTERNAL_RAW_BROKEN;
    }
    else if ( (strlen(text) >= (size * 2u))                          &&
              (d_internal_parse_hex_of(text, size, _out))            &&
              ( (text[size * 2u] == '\0') ||
                (text[size * 2u] == ' ')  ||
                (text[size * 2u] == '\t') ||
                (text[size * 2u] == '\n') ||
                (text[size * 2u] == '\r') ) )
    {
        raw = D_INTERNAL_RAW_OID;
    }

    free(text);

    return raw;
}


/*
d_internal_compare_packed
  Orders packed refs by name.
*/
static int
d_internal_compare_packed(
    const void* _left,
    const void* _right
)
{
    const struct d_git_packed_ref* const left  = _left;
    const struct d_git_packed_ref* const right = _right;

    return strcmp(left->name, right->name);
}


/*
d_internal_packed_line
  One line of packed-refs: a name in hex, a space and a ref; or `^` and the
name the ref before it peels to. A comment is passed over.
  Long as packed-refs' lines -- its header, a ref, a peeled line -- are told
apart and read here.
*/
static bool
d_internal_packed_line(
    struct d_git_repo* _repo,
    const char*        _line,
    size_t             _length,
    size_t*            _capacity
)
{
    const size_t size = d_git_repo_hash_size(_repo);

    // a comment, a peeled line, or a ref
    if ( (_length == 0u) ||
         (_line[0] == '#') )
    {
        return true;
    }
    else if (_line[0] == '^')
    {
        // a ref before it to peel
        if ( (_repo->packed_count == 0u) ||
             (_length < (1u + (size * 2u))) ||
             (!d_internal_parse_hex_of(&_line[1],
                                       size,
                                       &_repo->packed[_repo->packed_count - 1u]
                                            .peeled)) )
        {
            return true;
        }

        _repo->packed[_repo->packed_count - 1u].has_peeled = true;

        return true;
    }

    struct d_git_packed_ref ref;

    memset(&ref, 0, sizeof(ref));

    // a name, a space, and a ref
    if ( (_length < ((size * 2u) + 2u))                   ||
         (_line[size * 2u] != ' ')                        ||
         (!d_internal_parse_hex_of(_line, size, &ref.oid)) )
    {
        return true;
    }

    // room for one more
    if (_repo->packed_count == *_capacity)
    {
        const size_t                   wanted = (*_capacity > 0u)
                                                ? (*_capacity * 2u)
                                                : 64u;
        struct d_git_packed_ref* const grown  =
            realloc(_repo->packed, wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            return false;
        }

        _repo->packed = grown;
        *_capacity    = wanted;
    }

    ref.name = d_internal_dup(&_line[(size * 2u) + 1u],
                              _length - (size * 2u) - 1u);

    // the name kept
    if (!ref.name)
    {
        return false;
    }

    _repo->packed[_repo->packed_count] = ref;
    ++_repo->packed_count;

    return true;
}


/*
d_internal_packed_read
  packed-refs, read the first time it is wanted, its refs ordered by name.
*/
static bool
d_internal_packed_read(
    struct d_git_repo* _repo
)
{
    // read once
    if (_repo->packed_read)
    {
        return true;
    }

    size_t      length   = 0u;
    size_t      capacity = 0u;
    char* const text     = d_internal_read_in(_repo->common_dir,
                                              "packed-refs",
                                              &length);
    bool        read     = true;

    _repo->packed_read = true;

    // each line
    for (size_t at = 0u; ( (text) && (read) && (at < length) );)
    {
        const char* const end  = memchr(&text[at], '\n', length - at);
        const size_t      stop = (end) ? (size_t)(end - text) : length;
        size_t            line = stop - at;

        // a `\r` before the `\n` is the ending's
        if ( (line > 0u) &&
             (text[at + line - 1u] == '\r') )
        {
            --line;
        }

        read = d_internal_packed_line(_repo, &text[at], line, &capacity);
        at   = stop + 1u;
    }

    free(text);

    // ordered by name, as git keeps them
    if (_repo->packed_count > 1u)
    {
        qsort(_repo->packed,
              _repo->packed_count,
              sizeof(*_repo->packed),
              d_internal_compare_packed);
    }

    return (read) ? true : d_git_repo_fail(_repo, "out of memory", NULL);
}


/*
d_internal_packed_find
  The packed ref of a name, by binary search; D_GIT_REPO_NONE for none.
*/
static size_t
d_internal_packed_find(
    struct d_git_repo* _repo,
    const char*        _name
)
{
    // packed-refs read
    if (!d_internal_packed_read(_repo))
    {
        return D_GIT_REPO_NONE;
    }

    size_t low  = 0u;
    size_t high = _repo->packed_count;

    // halving the range
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);
        const int    order  = strcmp(_repo->packed[middle].name, _name);

        // the name, or a half
        if (order == 0)
        {
            return middle;
        }
        else if (order < 0)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return D_GIT_REPO_NONE;
}


/*
d_internal_read_raw
  One ref, not followed: loose, or else packed. A loose ref that is broken
does not fall back to a packed one, as git's does not.
*/
static enum d_internal_raw
d_internal_read_raw(
    struct d_git_repo* _repo,
    const char*        _name,
    struct d_git_oid*  _out,
    char**             _out_target
)
{
    const enum d_internal_raw loose = d_internal_read_loose(_repo,
                                                            _name,
                                                            _out,
                                                            _out_target);

    // loose, or broken
    if (loose != D_INTERNAL_RAW_MISSING)
    {
        return loose;
    }

    const size_t packed = d_internal_packed_find(_repo, _name);

    // packed, or nowhere
    if (packed == D_GIT_REPO_NONE)
    {
        return D_INTERNAL_RAW_MISSING;
    }

    *_out = _repo->packed[packed].oid;

    return D_INTERNAL_RAW_OID;
}


bool
d_git_repo_resolve(
    struct d_git_repo* _repo,
    const char*        _name,
    struct d_git_oid*  _out,
    char**             _out_target
)
{
    char* current = d_internal_dup_text(_name);

    // the first target, if asked for, is none until one is read
    if (_out_target)
    {
        *_out_target = NULL;
    }

    // a ref at a time, five deep
    for (unsigned depth = 0u;
         ( (current) && (depth <= D_INTERNAL_SYMREF_DEPTH) );
         ++depth)
    {
        char* target = NULL;

        // a name git allows
        if (!d_internal_refname_ok(current))
        {
            break;
        }

        const enum d_internal_raw raw = d_internal_read_raw(_repo,
                                                            current,
                                                            _out,
                                                            &target);

        // a name ends the walk; a symbolic ref goes on
        if (raw == D_INTERNAL_RAW_OID)
        {
            free(current);

            return true;
        }
        else if (raw != D_INTERNAL_RAW_SYMBOLIC)
        {
            break;
        }

        // the first target, if asked for
        if ( (depth == 0u) &&
             (_out_target) )
        {
            *_out_target = d_internal_dup_text(target);
        }

        free(current);
        current = target;
    }

    free(current);

    // a target given for a ref that leads nowhere is taken back
    if (_out_target)
    {
        free(*_out_target);
        *_out_target = NULL;
    }

    return d_git_repo_fail(_repo, "no such ref", _name);
}


char*
d_git_repo_symbolic(
    struct d_git_repo* _repo,
    const char*        _name
)
{
    struct d_git_oid oid;
    char*            target = NULL;

    // a ref git allows, read but not followed
    if ( (!_repo) ||
         (!_name) ||
         (!d_internal_refname_ok(_name)) ||
         (d_internal_read_raw(_repo, _name, &oid, &target)
              != D_INTERNAL_RAW_SYMBOLIC) )
    {
        free(target);

        return NULL;
    }

    return target;
}


/*
d_internal_refs_add
  One ref more for a listing, its name copied; not one already there,
which a loose ref was read before.
*/
static bool
d_internal_refs_add(
    struct d_git_refs* _refs,
    const char*        _name
)
{
    // room for one more
    if (_refs->count == _refs->capacity)
    {
        const size_t            wanted = (_refs->capacity > 0u)
                                         ? (_refs->capacity * 2u)
                                         : 64u;
        struct d_git_ref* const grown  = realloc(_refs->items,
                                                 wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            return false;
        }

        _refs->items    = grown;
        _refs->capacity = wanted;
    }

    struct d_git_ref* const ref = &_refs->items[_refs->count];

    memset(ref, 0, sizeof(*ref));
    ref->name = d_internal_dup_text(_name);

    // the name kept
    if (!ref->name)
    {
        return false;
    }

    ++_refs->count;

    return true;
}


// d_internal_walk
//   struct: a walk of loose refs: where they are kept, the names to list,
//   and the subtrees of a work tree's own refs, which a linked work
//   tree's common directory does not hold for it.
struct d_internal_walk
{
    const char*        base;
    struct d_git_refs* refs;
    bool               skip_own;
};


/*
d_internal_walk_refs
  Every loose ref below a directory of refs, by name: a directory walked,
a file listed if its name is one git allows.
*/
static bool
d_internal_walk_refs(
    const struct d_internal_walk* _walk,
    const char*                   _name
)
{
    char* const path  = d_git_path_join(_walk->base, _name);
    char**      names = NULL;
    size_t      count = 0u;

    // the directory's names, or none
    if ( (!path) ||
         (!d_fs_names(path, &names, &count)) )
    {
        free(path);

        return true;
    }

    bool walked = true;

    // each name
    for (size_t at = 0u; ( (walked) && (at < count) ); ++at)
    {
        char* const name  = d_git_path_join(_name, names[at]);
        char* const there = (name) ? d_git_path_join(_walk->base, name)
                                   : NULL;
        const enum d_git_file_kind kind = (there) ? d_internal_kind(there)
                                                  : D_GIT_FILE_NONE;

        // a directory walked, a ref listed, a work tree's own passed
        if ( (!name) ||
             (!there) )
        {
            walked = false;
        }
        else if ( (_walk->skip_own) &&
                  (d_internal_worktree_ref(name)) )
        {
        }
        else if (kind == D_GIT_FILE_DIRECTORY)
        {
            walked = d_internal_walk_refs(_walk, name);
        }
        else if ( (d_internal_refname_ok(name)) &&
                  (kind != D_GIT_FILE_NONE) )
        {
            walked = d_internal_refs_add(_walk->refs, name);
        }

        free(name);
        free(there);
    }

    d_fs_names_free(names, count);
    free(path);

    return walked;
}


/*
d_internal_compare_refs
  Orders refs by name.
*/
static int
d_internal_compare_refs(
    const void* _left,
    const void* _right
)
{
    const struct d_git_ref* const left  = _left;
    const struct d_git_ref* const right = _right;

    return strcmp(left->name, right->name);
}


/*
d_internal_refs_unique
  Orders refs by name and leaves one of each: a loose ref and a packed
one of a name are one ref.
*/
static void
d_internal_refs_unique(
    struct d_git_refs* _refs
)
{
    size_t kept = 0u;

    // ordered, then the repeats dropped
    if (_refs->count > 1u)
    {
        qsort(_refs->items,
              _refs->count,
              sizeof(*_refs->items),
              d_internal_compare_refs);
    }

    // each name once
    for (size_t at = 0u; at < _refs->count; ++at)
    {
        // one already kept
        if ( (kept > 0u) &&
             (strcmp(_refs->items[kept - 1u].name,
                     _refs->items[at].name) == 0) )
        {
            free(_refs->items[at].name);

            continue;
        }

        _refs->items[kept] = _refs->items[at];
        ++kept;
    }

    _refs->count = kept;

    return;
}


/*
d_internal_ref_peel
  What a listed ref names: followed, and, when it names a tag, peeled --
packed-refs's peeled line for a packed ref, else the tag read.
*/
static bool
d_internal_ref_peel(
    struct d_git_repo* _repo,
    struct d_git_ref*  _ref
)
{
    char*        target = NULL;
    const size_t packed = d_internal_packed_find(_repo, _ref->name);

    _ref->resolved = d_git_repo_resolve(_repo, _ref->name, &_ref->oid, &target);
    _ref->target   = target;

    // a ref that leads nowhere is not listed
    if (!_ref->resolved)
    {
        return false;
    }

    struct d_git_oid at = _ref->oid;

    // packed-refs's peeled line, for a ref packed and not loose
    if ( (packed != D_GIT_REPO_NONE) &&
         (_repo->packed[packed].has_peeled) &&
         (d_git_repo_same_oid(_repo, &_repo->packed[packed].oid, &at)) )
    {
        _ref->peeled     = _repo->packed[packed].peeled;
        _ref->has_peeled = true;

        return true;
    }

    // a tag peeled, as many deep as there are
    for (unsigned depth = 0u; depth < 32u; ++depth)
    {
        struct d_git_object object;
        struct d_git_tag    tag;
        const bool          read = d_git_repo_read(_repo, &at, &object);
        const bool          is   = ( (read) &&
                                     (object.type == D_GIT_OBJECT_TAG) &&
                                     (d_git_repo_parse_tag(_repo,
                                                           &object,
                                                           &tag)) );

        // what is not a tag ends the peeling
        if (!is)
        {
            d_git_object_release(&object);

            break;
        }

        at = tag.object;
        d_git_object_release(&object);
        _ref->peeled     = at;
        _ref->has_peeled = true;
    }

    _repo->problem[0] = '\0';

    return true;
}


bool
d_git_repo_refs(
    struct d_git_repo* _repo,
    const char*        _prefix,
    struct d_git_refs* _out
)
{
    const char* const prefix = ( (_prefix) && (_prefix[0] != '\0') )
                               ? _prefix
                               : "refs/";
    const bool        linked = ( (_repo) &&
                                 (strcmp(_repo->git_dir,
                                         _repo->common_dir) != 0) );

    memset(_out, 0, sizeof(*_out));

    // a repository, and its packed refs
    if ( (!_repo) ||
         (!d_internal_packed_read(_repo)) )
    {
        return false;
    }

    struct d_internal_walk common = { _repo->common_dir, _out, linked };
    struct d_internal_walk own    = { _repo->git_dir, _out, false };
    bool                   listed = ( (d_internal_walk_refs(&common, "refs")) &&
                                      ( (!linked) ||
                                        ( (d_internal_walk_refs(&own,
                                               "refs/worktree")) &&
                                          (d_internal_walk_refs(&own,
                                               "refs/bisect")) &&
                                          (d_internal_walk_refs(&own,
                                               "refs/rewritten")) ) ) );

    // the packed refs besides
    for (size_t at = 0u; ( (listed) && (at < _repo->packed_count) ); ++at)
    {
        listed = d_internal_refs_add(_out, _repo->packed[at].name);
    }

    d_internal_refs_unique(_out);

    size_t kept = 0u;

    // those of the prefix that lead somewhere, peeled
    for (size_t at = 0u; ( (listed) && (at < _out->count) ); ++at)
    {
        struct d_git_ref ref = _out->items[at];

        // of the prefix and leading somewhere, or let go
        if ( (strncmp(ref.name, prefix, strlen(prefix)) == 0) &&
             (d_internal_ref_peel(_repo, &ref)) )
        {
            _out->items[kept++] = ref;

            continue;
        }

        free(ref.name);
        free(ref.target);
    }

    _out->count = (listed) ? kept : _out->count;

    return (listed) ? true : d_git_repo_fail(_repo, "out of memory", NULL);
}


void
d_git_refs_release(
    struct d_git_refs* _refs
)
{
    // nothing to release
    if (!_refs)
    {
        return;
    }

    // each ref's strings
    for (size_t at = 0u; at < _refs->count; ++at)
    {
        free(_refs->items[at].name);
        free(_refs->items[at].target);
    }

    free(_refs->items);
    memset(_refs, 0, sizeof(*_refs));

    return;
}


//==============================================================================
// 7.  REVISIONS
//==============================================================================


// d_internal_rules
//   constant: rev-parse's rules for completing a ref's name, in order: a
//   prefix and a suffix around the name.
static const char* const d_internal_rules[6][2] =
{
    { "",             ""      },
    { "refs/",        ""      },
    { "refs/tags/",   ""      },
    { "refs/heads/",  ""      },
    { "refs/remotes/", ""     },
    { "refs/remotes/", "/HEAD" }
};


/*
d_internal_dwim
  A ref's name completed by the first of rev-parse's rules that names a
ref that leads somewhere.
*/
static bool
d_internal_dwim(
    struct d_git_repo* _repo,
    const char*        _name,
    size_t             _length,
    struct d_git_oid*  _out
)
{
    // each rule in order
    for (size_t rule = 0u; rule < 6u; ++rule)
    {
        struct d_git_buffer full;

        memset(&full, 0, sizeof(full));
        (void)d_git_buffer_add_text(&full, d_internal_rules[rule][0]);
        (void)d_git_buffer_add(&full, _name, _length);
        (void)d_git_buffer_add_text(&full, d_internal_rules[rule][1]);

        const bool found = ( (!full.failed) &&
                             (d_git_repo_resolve(_repo, full.text, _out,
                                                 NULL)) );

        d_git_buffer_free(&full);

        // the first that leads somewhere
        if (found)
        {
            _repo->problem[0] = '\0';

            return true;
        }
    }

    // a name that is no ref's is no fault of a ref's
    _repo->problem[0] = '\0';

    return false;
}


/*
d_internal_ambiguous_path
  Whether a name is one git's ambiguous_path refuses as a ref: a name whose
last part, or any part but the last, is empty or only dots.
*/
static bool
d_internal_ambiguous_path(
    const char* _name,
    size_t      _length
)
{
    bool slash = true;

    // each byte: a slash after an empty part ends the name refused
    for (size_t at = 0u; at < _length; ++at)
    {
        const char c = _name[at];

        // a NUL ends it, a slash after nothing refuses it
        if ( (c == '\0') ||
             ( (c == '/') && (slash) ) )
        {
            break;
        }

        slash = (c == '/') ? true : ( (c == '.') ? slash : false );
    }

    return slash;
}


/*
d_internal_all_hex
  Whether bytes are all hex digits.
*/
static bool
d_internal_all_hex(
    const char* _text,
    size_t      _length
)
{
    // each a digit
    for (size_t at = 0u; at < _length; ++at)
    {
        // one that is not
        if (d_internal_hex_digit(_text[at]) < 0)
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_short
  A prefix in hex of four digits or more, naming one object alone.
*/
static bool
d_internal_short(
    struct d_git_repo* _repo,
    const char*        _name,
    size_t             _length,
    struct d_git_oid*  _out
)
{
    // a prefix of a length git reads, all hex
    if ( (_length < 4u) ||
         (_length > (d_git_repo_hash_size(_repo) * 2u)) ||
         (!d_internal_all_hex(_name, _length)) )
    {
        return false;
    }

    return d_git_repo_prefix_find(_repo, _name, _length, _out);
}


static bool d_internal_rev_part(struct d_git_repo* _repo,
                                const char*        _name,
                                size_t             _length,
                                struct d_git_oid*  _out);
static char* d_internal_prior_checkout(const struct d_git_repo* _repo,
                                      unsigned long            _nth);
static bool  d_internal_digits(const char*    _text,
                               size_t         _length,
                               unsigned long* _out);


// d_internal_reflog_entry
//   struct: one line of a reflog: the name a ref had, the one it took, and
//   the message, borrowed from the log's text.
struct d_internal_reflog_entry
{
    struct d_git_oid old_oid;
    struct d_git_oid new_oid;
    const char*      message;
    size_t           message_length;
};

// d_internal_reflog
//   struct: a reflog read whole: its text and its lines, oldest first.
struct d_internal_reflog
{
    char*                           text;
    struct d_internal_reflog_entry* entries;
    size_t                          count;
};


/*
d_internal_reflog_read
  A ref's reflog, logs/<ref> -- a work tree's own refs' in its directory --
each line two names in hex, an identity, a tab and a message. False when
there is none.
*/
static bool
d_internal_reflog_read(
    const struct d_git_repo*  _repo,
    const char*               _ref,
    struct d_internal_reflog* _out
)
{
    const size_t hash   = d_git_repo_hash_size(_repo);
    char* const  logs   = d_git_path_join((d_internal_worktree_ref(_ref))
                                          ? _repo->git_dir
                                          : _repo->common_dir,
                                          "logs");
    char* const  path   = (logs) ? d_git_path_join(logs, _ref) : NULL;
    size_t       length = 0u;

    memset(_out, 0, sizeof(*_out));
    _out->text = (path) ? d_git_file_read(path, &length) : NULL;
    free(logs);
    free(path);

    // a log there
    if (!_out->text)
    {
        return false;
    }

    size_t lines = 0u;

    // its lines, counted
    for (size_t at = 0u; at < length; ++at)
    {
        lines += (_out->text[at] == '\n') ? 1u : 0u;
    }

    _out->entries = calloc(lines + 1u, sizeof(*_out->entries));

    // each line that reads as one
    for (char* line = _out->text;
         ( (_out->entries) && (line) && (*line != '\0') );)
    {
        char* const end = strchr(line, '\n');
        char* const tab = strchr(line, '\t');
        struct d_internal_reflog_entry* const entry =
            &_out->entries[_out->count];

        // the line alone
        if (end)
        {
            *end = '\0';
        }

        // two names, a space between, and a message after a tab
        if ( (strlen(line) > ((hash * 4u) + 1u)) &&
             (line[hash * 2u] == ' ') &&
             (d_internal_parse_hex_of(line, hash, &entry->old_oid)) &&
             (d_internal_parse_hex_of(&line[(hash * 2u) + 1u], hash,
                                      &entry->new_oid)) )
        {
            entry->message        = (tab) ? (tab + 1) : "";
            entry->message_length = strlen(entry->message);
            ++_out->count;
        }

        line = (end) ? (end + 1) : NULL;
    }

    return (_out->entries != NULL);
}


/*
d_internal_reflog_free
  Lets go of a reflog read.
*/
static void
d_internal_reflog_free(
    struct d_internal_reflog* _log
)
{
    free(_log->text);
    free(_log->entries);
    memset(_log, 0, sizeof(*_log));

    return;
}


/*
d_internal_is_null
  Whether a name is all zeros, as a reflog writes for a ref that was not.
*/
static bool
d_internal_is_null(
    const struct d_git_repo* _repo,
    const struct d_git_oid*  _oid
)
{
    const size_t size = d_git_repo_hash_size(_repo);

    // each byte zero
    for (size_t at = 0u; at < size; ++at)
    {
        // one that is not
        if (_oid->bytes[at] != 0u)
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_reflog_nth
  A ref's value n changes ago, as git's read_ref_at counts them: @{0} the
newest line's new name; @{n} the old name of the nth newest -- or of the
first older with one, past a line that made the ref.
*/
static bool
d_internal_reflog_nth(
    struct d_git_repo* _repo,
    const char*        _ref,
    unsigned long      _nth,
    struct d_git_oid*  _out
)
{
    struct d_internal_reflog log;

    // a log, with lines
    if ( (!d_internal_reflog_read(_repo, _ref, &log)) ||
         (log.count == 0u) )
    {
        d_internal_reflog_free(&log);

        return d_git_repo_fail(_repo, "the ref has no reflog", _ref);
    }

    // the newest line's new name
    if (_nth == 0u)
    {
        *_out = log.entries[log.count - 1u].new_oid;
        d_internal_reflog_free(&log);

        return true;
    }

    bool found = false;

    // from the nth newest back, the first old name that is one
    for (size_t at = (_nth <= log.count) ? (log.count - _nth) : 0u;
         ( (_nth <= log.count) && (!found) );
         --at)
    {
        found = !d_internal_is_null(_repo, &log.entries[at].old_oid);
        *_out = (found) ? log.entries[at].old_oid : *_out;

        // the oldest line ends the search
        if (at == 0u)
        {
            break;
        }
    }

    d_internal_reflog_free(&log);

    return (found) ? true : d_git_repo_fail(_repo, "log only has so many "
                                                   "entries", _ref);
}


/*
d_internal_has_log
  Whether a ref has a reflog to read.
*/
static bool
d_internal_has_log(
    const struct d_git_repo* _repo,
    const char*              _ref
)
{
    struct d_internal_reflog log;
    const bool               found = d_internal_reflog_read(_repo, _ref,
                                                            &log);

    d_internal_reflog_free(&log);

    return found;
}


/*
d_internal_log_of
  The ref whose reflog a name names, as git's dwim_log finds it: the first
of rev-parse's rules naming a ref that leads somewhere and has a log, or
whose symbolic target has one; for no name, the branch HEAD is on, or HEAD
itself.
*/
static char*
d_internal_log_of(
    struct d_git_repo* _repo,
    const char*        _name,
    size_t             _length
)
{
    // none: HEAD's branch, or HEAD
    if (_length == 0u)
    {
        char* const branch = d_git_repo_symbolic(_repo, "HEAD");

        return (branch) ? branch : d_internal_dup_text("HEAD");
    }

    char* found = NULL;

    // each rule in order, until one has a log
    for (size_t rule = 0u; ( (!found) && (rule < 6u) ); ++rule)
    {
        struct d_git_buffer full;
        struct d_git_oid    oid;
        char*               target = NULL;

        memset(&full, 0, sizeof(full));
        (void)d_git_buffer_add_text(&full, d_internal_rules[rule][0]);
        (void)d_git_buffer_add(&full, _name, _length);
        (void)d_git_buffer_add_text(&full, d_internal_rules[rule][1]);

        // a ref there, leading somewhere: its log, or its target's
        if ( (!full.failed) &&
             (d_internal_refname_ok(full.text)) &&
             (d_git_repo_resolve(_repo, full.text, &oid, &target)) )
        {
            found = (d_internal_has_log(_repo, full.text))
                    ? d_git_buffer_take(&full)
                    : ( (target) && (d_internal_has_log(_repo, target)) )
                      ? d_internal_dup_text(target)
                      : NULL;
        }

        free(target);
        d_git_buffer_free(&full);
    }

    _repo->problem[0] = '\0';

    return found;
}


/*
d_internal_branch_named
  A branch's name as a mark's base gives it: @{-n} the nth prior checkout's,
anything else itself; allocated.
*/
static char*
d_internal_branch_named(
    const struct d_git_repo* _repo,
    const char*              _name,
    size_t                   _length
)
{
    unsigned long nth = 0u;

    // a prior checkout
    if ( (_length > 4u) &&
         (strncmp(_name, "@{-", 3u) == 0) &&
         (_name[_length - 1u] == '}') &&
         (d_internal_digits(&_name[3], _length - 4u, &nth)) )
    {
        return d_internal_prior_checkout(_repo, nth);
    }

    return d_internal_dup(_name, _length);
}


/*
d_internal_prior_checkout
  The branch the nth checkout before this one left, @{-n}, as git's
interpret_nth_prior_checkout reads HEAD's reflog for it: each "checkout:
moving from A to B", newest first, the nth A.
*/
static char*
d_internal_prior_checkout(
    const struct d_git_repo* _repo,
    unsigned long            _nth
)
{
    static const char        prefix[] = "checkout: moving from ";
    struct d_internal_reflog log;
    char*                    branch = NULL;
    unsigned long            seen   = 0u;

    // HEAD's log
    if ( (_nth == 0u) ||
         (!d_internal_reflog_read(_repo, "HEAD", &log)) )
    {
        return NULL;
    }

    // newest first, each checkout
    for (size_t at = log.count; ( (!branch) && (at-- > 0u) );)
    {
        const char* const message = log.entries[at].message;
        const char* const to      = strstr(message, " to ");

        // a checkout, the nth
        if ( (strncmp(message, prefix, sizeof(prefix) - 1u) == 0) &&
             (to) &&
             (++seen == _nth) )
        {
            branch = d_internal_dup(&message[sizeof(prefix) - 1u],
                                    (size_t)(to - message)
                                        - (sizeof(prefix) - 1u));
        }
    }

    d_internal_reflog_free(&log);

    return branch;
}


/*
d_internal_map_refspec
  A ref mapped by a fetch refspec, +src:dst, a `*` in each taking the same
text; NULL when the refspec does not take it.
*/
static char*
d_internal_map_refspec(
    const char* _refspec,
    const char* _ref
)
{
    const char* const spec  = (_refspec[0] == '+') ? &_refspec[1] : _refspec;
    const char* const colon = strchr(spec, ':');

    // a source and a destination
    if (!colon)
    {
        return NULL;
    }

    const size_t      source = (size_t)(colon - spec);
    const char* const star   = memchr(spec, '*', source);
    struct d_git_buffer out;

    memset(&out, 0, sizeof(out));

    // no star: the ref itself, or nothing
    if (!star)
    {
        return ( (strlen(_ref) == source) &&
                 (strncmp(spec, _ref, source) == 0) )
               ? d_internal_dup_text(colon + 1)
               : NULL;
    }

    const size_t before = (size_t)(star - spec);
    const size_t after  = source - before - 1u;
    const size_t length = strlen(_ref);

    // the text around the star matches
    if ( (length < (before + after)) ||
         (strncmp(_ref, spec, before) != 0) ||
         (strncmp(&_ref[length - after], star + 1, after) != 0) )
    {
        return NULL;
    }

    const char* const dst_star = strchr(colon + 1, '*');

    // the destination, the star's text in place of its own
    if (!dst_star)
    {
        return d_internal_dup_text(colon + 1);
    }

    (void)d_git_buffer_add(&out, colon + 1, (size_t)(dst_star - colon - 1));
    (void)d_git_buffer_add(&out, &_ref[before], length - before - after);
    (void)d_git_buffer_add_text(&out, dst_star + 1);

    return d_git_buffer_take(&out);
}


/*
d_internal_upstream
  A branch's upstream, @{u}, as git's branch_get finds it: its
branch.<name>.merge, through the first of its remote's fetch refspecs
that maps it, or itself for a remote of ".".
  Long as git's branch_get is, which it follows: the branch, its merge, its
remote, and the remote's refspecs.
*/
static bool
d_internal_upstream(
    struct d_git_repo* _repo,
    const char*        _branch,
    size_t             _length,
    struct d_git_oid*  _out
)
{
    char* const branch = (_length > 0u) ? d_internal_dup(_branch, _length)
                                        : d_git_repo_symbolic(_repo, "HEAD");
    const char* const name = ( (branch) &&
                               (strncmp(branch, "refs/heads/", 11u) == 0) )
                             ? &branch[11]
                             : branch;
    struct d_git_buffer key;
    bool                set = false;

    memset(&key, 0, sizeof(key));
    (void)d_git_buffer_add_text(&key, "branch.");
    (void)d_git_buffer_add_text(&key, (name) ? name : "");
    (void)d_git_buffer_add_text(&key, ".merge");

    const char* const merge  = (name) ? d_git_repo_config(_repo, key.text,
                                                          &set)
                                      : NULL;

    key.length -= 5u;
    (void)d_git_buffer_add_text(&key, "remote");

    const char* const remote = (merge) ? d_git_repo_config(_repo, key.text,
                                                           &set)
                                       : NULL;
    char*             tracked = NULL;

    // a remote of "." is this repository; another maps by its refspecs
    if ( (remote) &&
         (strcmp(remote, ".") == 0) )
    {
        tracked = d_internal_dup_text(merge);
    }
    else if (remote)
    {
        key.length = 0u;
        (void)d_git_buffer_add_text(&key, "remote.");
        (void)d_git_buffer_add_text(&key, remote);
        (void)d_git_buffer_add_text(&key, ".fetch");

        // each refspec, until one maps the branch
        for (size_t at = d_git_repo_find_setting(_repo, key.text,
                                                 D_GIT_REPO_NONE);
             ( (!tracked) && (at != D_GIT_REPO_NONE) );
             at = d_git_repo_find_setting(_repo, key.text, at))
        {
            tracked = (_repo->settings[at].value)
                      ? d_internal_map_refspec(_repo->settings[at].value,
                                               merge)
                      : NULL;
        }
    }

    const bool found = ( (tracked) &&
                         (d_git_repo_resolve(_repo, tracked, _out, NULL)) );

    // why not, when not
    if (!found)
    {
        (void)d_git_repo_fail(_repo, "no upstream configured for branch",
                              name);
    }

    free(tracked);
    free(branch);
    d_git_buffer_free(&key);

    return found;
}


/*
d_internal_digits
  Whether text is all decimal digits, and their number.
*/
static bool
d_internal_digits(
    const char*    _text,
    size_t         _length,
    unsigned long* _out
)
{
    *_out = 0u;

    // each a digit
    for (size_t at = 0u; at < _length; ++at)
    {
        // one that is not, or a number too large
        if ( (_text[at] < '0') ||
             (_text[at] > '9') ||
             (*_out > 100000000u) )
        {
            return false;
        }

        *_out = (*_out * 10u) + (unsigned long)(_text[at] - '0');
    }

    return (_length > 0u);
}


/*
d_internal_at_mark
  A name's @{...}, the last there: @{-n} a prior checkout, when it begins
the name; @{u} and @{upstream} the upstream, in any case; @{n} the reflog's
nth; @{push} and dates are not read. `_out_marked` says there was one.
  Long as get_oid_basic's reading of a mark is, which it follows: the mark
found, then each of its kinds.
*/
static bool
d_internal_at_mark(
    struct d_git_repo* _repo,
    const char*        _name,
    size_t             _length,
    struct d_git_oid*  _out,
    bool*              _out_marked
)
{
    size_t at = _length;

    *_out_marked = false;

    // a closing brace, and the last @{ before it
    if ( (_length < 3u) ||
         (_name[_length - 1u] != '}') )
    {
        return false;
    }

    // back to the last @{
    while ( (at > 0u) &&
            ( (_name[at - 1u] != '@') || (_name[at] != '{') ) )
    {
        --at;
    }

    // none
    if (at == 0u)
    {
        return false;
    }

    const char* const inner  = &_name[at + 1u];
    const size_t      length = _length - at - 2u;
    const size_t      base   = at - 1u;
    unsigned long     nth    = 0u;
    char              word[16];

    *_out_marked = true;
    memset(word, 0, sizeof(word));

    // the mark's word, in lower case
    for (size_t letter = 0u; ( (letter < length) && (letter < 15u) ); ++letter)
    {
        word[letter] = ( (inner[letter] >= 'A') && (inner[letter] <= 'Z') )
                       ? (char)(inner[letter] + 32)
                       : inner[letter];
    }

    // a prior checkout, an upstream, a reflog's nth
    if ( (length > 1u) &&
         (inner[0] == '-') &&
         (d_internal_digits(&inner[1], length - 1u, &nth)) )
    {
        char* const branch = (base == 0u) ? d_internal_prior_checkout(_repo,
                                                                      nth)
                                          : NULL;
        const bool  found  = ( (branch) &&
                               (d_internal_rev_part(_repo, branch,
                                                    strlen(branch), _out)) );

        free(branch);

        return (found) ? true : d_git_repo_fail(_repo, "no such prior "
                                                       "checkout", NULL);
    }
    else if ( (strcmp(word, "u") == 0) ||
              (strcmp(word, "upstream") == 0) )
    {
        char* const branch = d_internal_branch_named(_repo, _name, base);
        const bool  found  = ( (branch) &&
                               (d_internal_upstream(_repo, branch,
                                                    strlen(branch), _out)) );

        free(branch);

        // why not, when not and no reason given
        if ( (!found) &&
             (_repo->problem[0] == '\0') )
        {
            (void)d_git_repo_fail(_repo, "no such branch", NULL);
        }

        return found;
    }
    else if (!d_internal_digits(inner, length, &nth))
    {
        return d_git_repo_fail(_repo, "reflog dates and @{push} are not "
                                      "read", NULL);
    }

    char* const branch = d_internal_branch_named(_repo, _name, base);
    char* const ref    = (branch) ? d_internal_log_of(_repo, branch,
                                                      strlen(branch))
                                  : NULL;
    const bool  found  = ( (ref) &&
                           (d_internal_reflog_nth(_repo, ref, nth, _out)) );

    free(ref);
    free(branch);

    // why not, when not and no reason given
    if ( (!found) &&
         (_repo->problem[0] == '\0') )
    {
        (void)d_git_repo_fail(_repo, "no reflog for the name", NULL);
    }

    return found;
}


/*
d_internal_basic
  A name as git's get_oid_basic reads it: whole in hex; `@` for HEAD; a ref
rev-parse's rules complete -- not one ambiguous_path refuses. The reflog's
@{...} is not read.
*/
static bool
d_internal_basic(
    struct d_git_repo* _repo,
    const char*        _name,
    size_t             _length,
    struct d_git_oid*  _out
)
{
    // whole in hex
    if ( (_length == (d_git_repo_hash_size(_repo) * 2u)) &&
         (d_internal_parse_hex_of(_name, d_git_repo_hash_size(_repo), _out)) )
    {
        return true;
    }

    bool       marked = false;
    const bool found  = d_internal_at_mark(_repo, _name, _length, _out,
                                           &marked);

    // a reflog's, a prior checkout's, or an upstream's
    if (marked)
    {
        return found;
    }

    // `@` alone is HEAD; a name refused, or one rev-parse completes
    if ( (_length == 1u) &&
         (_name[0] == '@') )
    {
        return d_internal_dwim(_repo, "HEAD", 4u, _out);
    }
    else if ( (_length > 0u) &&
              (d_internal_ambiguous_path(_name, _length)) )
    {
        return false;
    }

    return d_internal_dwim(_repo, _name, _length, _out);
}


/*
d_internal_describe
  A describe's name, SOMETHING-gHEX, by the hex after its last -g, as git's
get_describe_name reads it.
*/
static bool
d_internal_describe(
    struct d_git_repo* _repo,
    const char*        _name,
    size_t             _length,
    struct d_git_oid*  _out
)
{
    // from the end, past the hex, to a `g` after a `-`
    for (size_t at = _length; at-- > 2u;)
    {
        // the `g`, after its `-`
        if ( (d_internal_hex_digit(_name[at]) < 0) &&
             (_name[at] == 'g') &&
             (_name[at - 1u] == '-') )
        {
            return d_internal_short(_repo, &_name[at + 1u],
                                    _length - at - 1u, _out);
        }
    }

    return false;
}


/*
d_internal_peel_to
  An object peeled to a type, as git's peel_to_type peels: a tag to what
it names, a commit to its tree, until the type is reached; D_GIT_OBJECT_NONE
for anything at all, which must be there.
*/
static bool
d_internal_peel_to(
    struct d_git_repo*     _repo,
    struct d_git_oid*      _oid,
    enum d_git_object_type _type
)
{
    // a step at a time, as deep as tags go
    for (unsigned depth = 0u; depth < 64u; ++depth)
    {
        struct d_git_object object;

        // the object there
        if (!d_git_repo_read(_repo, _oid, &object))
        {
            d_git_object_release(&object);

            return false;
        }

        const enum d_git_object_type type = object.type;
        bool                         on   = false;

        // a tag goes on to what it names; a commit to its tree
        if ( (type == _type) ||
             (_type == D_GIT_OBJECT_NONE) )
        {
            d_git_object_release(&object);

            return true;
        }
        else if (type == D_GIT_OBJECT_TAG)
        {
            struct d_git_tag tag;

            on   = d_git_repo_parse_tag(_repo, &object, &tag);
            *_oid = (on) ? tag.object : *_oid;
        }
        else if ( (type == D_GIT_OBJECT_COMMIT) &&
                  (_type == D_GIT_OBJECT_TREE) )
        {
            struct d_git_commit commit;

            on   = d_git_repo_parse_commit(_repo, &object, &commit);
            *_oid = (on) ? commit.tree : *_oid;
            d_git_commit_release(&commit);
        }

        d_git_object_release(&object);

        // a type that does not peel to the one wanted
        if (!on)
        {
            return d_git_repo_fail(_repo, "the object does not peel to a",
                                   d_git_object_type_name(_type));
        }
    }

    return d_git_repo_fail(_repo, "tags too deep to peel", NULL);
}


/*
d_internal_peel_tags
  An object peeled of its tags, to the first that is not one: ^{}.
*/
static bool
d_internal_peel_tags(
    struct d_git_repo* _repo,
    struct d_git_oid*  _oid
)
{
    // a tag at a time
    for (unsigned depth = 0u; depth < 64u; ++depth)
    {
        struct d_git_object object;
        struct d_git_tag    tag;

        // the object there
        if (!d_git_repo_read(_repo, _oid, &object))
        {
            d_git_object_release(&object);

            return false;
        }

        const bool tagged = ( (object.type == D_GIT_OBJECT_TAG) &&
                              (d_git_repo_parse_tag(_repo, &object, &tag)) );

        d_git_object_release(&object);

        // not a tag: peeled
        if (!tagged)
        {
            return true;
        }

        *_oid = tag.object;
    }

    return d_git_repo_fail(_repo, "tags too deep to peel", NULL);
}


static bool d_internal_rev_part(struct d_git_repo* _repo,
                                const char*        _name,
                                size_t             _length,
                                struct d_git_oid*  _out);


/*
d_internal_parent_of
  A commit's parent: ^0 the commit itself, ^n its nth parent.
*/
static bool
d_internal_parent_of(
    struct d_git_repo* _repo,
    struct d_git_oid*  _oid,
    unsigned           _nth
)
{
    struct d_git_object object;
    struct d_git_commit commit;

    // the commit, peeled to one
    if (!d_internal_peel_to(_repo, _oid, D_GIT_OBJECT_COMMIT))
    {
        return false;
    }
    else if (_nth == 0u)
    {
        return true;
    }

    const bool read = ( (d_git_repo_read(_repo, _oid, &object)) &&
                        (d_git_repo_parse_commit(_repo, &object, &commit)) );
    const bool has  = ( (read) && (_nth <= commit.parent_count) );

    // the parent, if it has one so numbered
    if (has)
    {
        *_oid = commit.parents[_nth - 1u];
    }

    // the commit's, if it was parsed
    if (read)
    {
        d_git_commit_release(&commit);
    }

    d_git_object_release(&object);

    return (has) ? true : d_git_repo_fail(_repo, "no such parent", NULL);
}


/*
d_internal_suffix
  A trailing ~n or ^n: its kind, the length before it, and n -- 1 when no
digits follow, as git counts it.
*/
static char
d_internal_suffix(
    const char* _name,
    size_t      _length,
    size_t*     _out_base,
    unsigned*   _out_count
)
{
    size_t at = _length;

    // back past the digits
    while ( (at > 0u) &&
            (_name[at - 1u] >= '0') &&
            (_name[at - 1u] <= '9') )
    {
        --at;
    }

    // a ~ or ^ before them
    if ( (at == 0u) ||
         ( (_name[at - 1u] != '~') && (_name[at - 1u] != '^') ) )
    {
        return '\0';
    }

    unsigned count = 0u;

    // the digits, if there are any
    for (size_t digit = at; digit < _length; ++digit)
    {
        count = (count * 10u) + (unsigned)(_name[digit] - '0');
    }

    *_out_base  = at - 1u;
    *_out_count = (at == _length) ? 1u : count;

    return _name[at - 1u];
}


/*
d_internal_onion
  A trailing ^{type}: what is before it, peeled -- ^{} of its tags, ^{object}
not at all, ^{commit}, ^{tree}, ^{blob}, ^{tag} to the type.
*/
static bool
d_internal_onion(
    struct d_git_repo* _repo,
    const char*        _name,
    size_t             _length,
    struct d_git_oid*  _out,
    bool*              _out_was
)
{
    size_t open = _length;

    *_out_was = false;

    // a ^{ before the closing brace
    while ( (open > 1u) &&
            ( (_name[open - 1u] != '{') || (_name[open - 2u] != '^') ) )
    {
        --open;
    }

    // not one
    if ( (_length < 3u) ||
         (_name[_length - 1u] != '}') ||
         (open <= 1u) )
    {
        return false;
    }

    *_out_was = true;

    const char* const type   = &_name[open];
    const size_t      typlen = _length - open - 1u;
    char              named[16];

    // what is before it
    if (!d_internal_rev_part(_repo, _name, open - 2u, _out))
    {
        return false;
    }

    // ^{} peels tags; ^{/regex} is not read; else a type, or object
    if (typlen == 0u)
    {
        return d_internal_peel_tags(_repo, _out);
    }
    else if ( (type[0] == '/') ||
              (typlen >= sizeof(named)) )
    {
        return d_git_repo_fail(_repo, "not a type to peel to", NULL);
    }

    memcpy(named, type, typlen);
    named[typlen] = '\0';

    const enum d_git_object_type wanted = d_git_object_type_named(named);

    // a type, or object for any
    if ( (wanted == D_GIT_OBJECT_NONE) &&
         (strcmp(named, "object") != 0) )
    {
        return d_git_repo_fail(_repo, "not a type to peel to", named);
    }

    return d_internal_peel_to(_repo, _out, wanted);
}


/*
d_internal_rev_part
  A revision without a path, as git's get_oid_1 reads one: ~n and ^n off
its end, then ^{...}, then a name, a describe's, a prefix in hex.
*/
static bool
d_internal_rev_part(
    struct d_git_repo* _repo,
    const char*        _name,
    size_t             _length,
    struct d_git_oid*  _out
)
{
    size_t     base  = 0u;
    unsigned   count = 0u;
    const char kind  = d_internal_suffix(_name, _length, &base, &count);
    bool       onion = false;

    // ^n or ~n
    if (kind != '\0')
    {
        // what is before it
        if (!d_internal_rev_part(_repo, _name, base, _out))
        {
            return false;
        }

        // a parent, or first parents
        if (kind == '^')
        {
            return d_internal_parent_of(_repo, _out, count);
        }

        // the first parent, n times
        for (unsigned step = 0u; step < count; ++step)
        {
            // each step's parent
            if (!d_internal_parent_of(_repo, _out, 1u))
            {
                return false;
            }
        }

        return d_internal_peel_to(_repo, _out, D_GIT_OBJECT_COMMIT);
    }

    const bool peeled = d_internal_onion(_repo, _name, _length, _out, &onion);

    // ^{...}, peeled or not
    if (onion)
    {
        return peeled;
    }

    return ( (d_internal_basic(_repo, _name, _length, _out))    ||
             (d_internal_describe(_repo, _name, _length, _out)) ||
             (d_internal_short(_repo, _name, _length, _out)) );
}


/*
d_internal_colon
  Where a revision's `:` before its path is: the first not within braces,
as git finds it; D_GIT_REPO_NONE for none.
*/
static size_t
d_internal_colon(
    const char* _name
)
{
    unsigned depth = 0u;

    // each byte, braces counted
    for (size_t at = 0u; _name[at] != '\0'; ++at)
    {
        // a brace, or the colon outside them
        if (_name[at] == '{')
        {
            ++depth;
        }
        else if ( (_name[at] == '}') &&
                  (depth > 0u) )
        {
            --depth;
        }
        else if ( (_name[at] == ':') &&
                  (depth == 0u) )
        {
            return at;
        }
    }

    return D_GIT_REPO_NONE;
}


/*
d_internal_index_rev
  :path and :n:path: the index's entry of the path at stage 0, or n.
*/
static bool
d_internal_index_rev(
    struct d_git_repo* _repo,
    const char*        _name,
    struct d_git_oid*  _out
)
{
    const bool        staged = ( (_name[1] >= '0') &&
                                 (_name[1] <= '3') &&
                                 (_name[2] == ':') );
    const unsigned    stage  = (staged) ? (unsigned)(_name[1] - '0') : 0u;
    const char* const path   = (staged) ? &_name[3] : &_name[1];

    // :/text searches messages, which is not read
    if (_name[1] == '/')
    {
        return d_git_repo_fail(_repo, "searching messages is not read",
                               NULL);
    }

    const size_t at = d_git_repo_index_find(_repo,
                                            (strncmp(path, "./", 2u) == 0)
                                            ? &path[2]
                                            : path,
                                            stage);

    // the entry
    if (at == D_GIT_REPO_NONE)
    {
        return d_git_repo_fail(_repo, "path is not in the index", path);
    }

    *_out = d_git_repo_index_entry(_repo, at)->oid;

    return true;
}


bool
d_git_repo_rev(
    struct d_git_repo* _repo,
    const char*        _name,
    struct d_git_oid*  _out
)
{
    // a repository, and a revision
    if ( (!_repo) ||
         (!_name) ||
         (_name[0] == '\0') )
    {
        return (_repo) ? d_git_repo_fail(_repo, "no revision", NULL) : false;
    }

    _repo->problem[0] = '\0';

    // the index's entry
    if (_name[0] == ':')
    {
        return d_internal_index_rev(_repo, _name, _out);
    }

    const size_t colon = d_internal_colon(_name);

    // no path: the revision alone
    if (colon == D_GIT_REPO_NONE)
    {
        return ( (d_internal_rev_part(_repo, _name, strlen(_name), _out)) ||
                 ( (_repo->problem[0] != '\0')
                   ? false
                   : d_git_repo_fail(_repo, "unknown revision", _name) ) );
    }

    const char* path = &_name[colon + 1u];
    uint32_t    mode = 0u;

    // a tree-ish, peeled to its tree
    if ( (!d_internal_rev_part(_repo, _name, colon, _out)) ||
         (!d_internal_peel_to(_repo, _out, D_GIT_OBJECT_TREE)) )
    {
        return (_repo->problem[0] != '\0')
               ? false
               : d_git_repo_fail(_repo, "unknown revision", _name);
    }

    // a path of "./" is from the top here
    if (strncmp(path, "./", 2u) == 0)
    {
        path += 2;
    }

    // the tree itself, or what it holds at the path
    if (path[0] == '\0')
    {
        return true;
    }

    return ( (d_git_tree_find(_repo, _out, path, _out, &mode)) ||
             (d_git_repo_fail(_repo, "path does not exist in the tree",
                              path)) );
}


//==============================================================================
// 8.  HISTORY
//==============================================================================


// D_INTERNAL_SEEN ... D_INTERNAL_RESULT
//   constant: the marks a walk puts on a commit, as revision.c and
//   commit-reach.c name theirs.
#define D_INTERNAL_SEEN          1u
#define D_INTERNAL_ADDED         2u
#define D_INTERNAL_UNINTERESTING 4u
#define D_INTERNAL_TREESAME      8u
#define D_INTERNAL_PARENT1       16u
#define D_INTERNAL_PARENT2       32u
#define D_INTERNAL_STALE         64u
#define D_INTERNAL_RESULT        128u

// D_INTERNAL_SLOP
//   constant: how many commits past the last interesting one a limited
//   walk goes on, as git's does, for dates that are out of order.
#define D_INTERNAL_SLOP 5

// d_internal_node
//   struct: a commit of a walk: its name, its date and tree once parsed,
//   its parents -- which simplifying may cut -- its marks, what its tree
//   holds at the walk's path, once asked, and the last walk of ancestors
//   that visited it.
struct d_internal_node
{
    struct d_git_oid oid;
    int64_t          date;
    struct d_git_oid tree;
    size_t*          parents;
    size_t           parent_count;
    unsigned         flags;
    bool             parsed;
    bool             path_asked;
    bool             path_present;
    uint32_t         path_mode;
    struct d_git_oid path_oid;
    unsigned         visit;
};

// d_internal_slot
//   struct: a commit waiting in the walk's queue, and when it came.
struct d_internal_slot
{
    size_t   node;
    uint64_t order;
};

// d_internal_walker
//   struct: a walk: its commits and the table that finds them by name,
//   the queue -- latest date first, ties first in first out -- the
//   shallow commits, whose parents git does not see, and what the walk is
//   limited by, and how many walks of ancestors there have been.
struct d_internal_walker
{
    struct d_git_repo*      repo;
    struct d_internal_node* nodes;
    size_t                  count;
    size_t                  capacity;
    size_t*                 table;
    size_t                  table_size;
    struct d_internal_slot* heap;
    size_t                  heap_count;
    size_t                  heap_capacity;
    uint64_t                order;
    struct d_git_oids       shallow;
    const char*             path;
    bool                    first_parent;
    bool                    failed;
    unsigned                generation;
};


/*
d_internal_hash_of
  Where a name begins its search in the walk's table.
*/
static size_t
d_internal_hash_of(
    const struct d_git_oid* _oid,
    size_t                  _size
)
{
    const size_t hash = ((size_t)_oid->bytes[0] << 24) |
                        ((size_t)_oid->bytes[1] << 16) |
                        ((size_t)_oid->bytes[2] << 8)  |
                        (size_t)_oid->bytes[3];

    return hash & (_size - 1u);
}


/*
d_internal_rehash
  The table, doubled, every commit put in it again.
*/
static bool
d_internal_rehash(
    struct d_internal_walker* _walker
)
{
    const size_t  size  = (_walker->table_size > 0u)
                          ? (_walker->table_size * 2u)
                          : 1024u;
    size_t* const table = calloc(size, sizeof(*table));

    // memory that ran out
    if (!table)
    {
        return false;
    }

    // each commit, at its hash or after
    for (size_t at = 0u; at < _walker->count; ++at)
    {
        size_t slot = d_internal_hash_of(&_walker->nodes[at].oid, size);

        // the first free slot
        while (table[slot] != 0u)
        {
            slot = (slot + 1u) & (size - 1u);
        }

        table[slot] = at + 1u;
    }

    free(_walker->table);
    _walker->table      = table;
    _walker->table_size = size;

    return true;
}


/*
d_internal_node_of
  The commit of a name, made when the walk has not met it;
D_GIT_REPO_NONE when memory runs out.
*/
static size_t
d_internal_node_of(
    struct d_internal_walker* _walker,
    const struct d_git_oid*   _oid
)
{
    // a table half full at most
    if ( ((_walker->count + 1u) * 2u) > _walker->table_size )
    {
        // doubled
        if (!d_internal_rehash(_walker))
        {
            _walker->failed = true;

            return D_GIT_REPO_NONE;
        }
    }

    const size_t size = d_git_repo_hash_size(_walker->repo);
    size_t       slot = d_internal_hash_of(_oid, _walker->table_size);

    // the name, or a free slot
    while (_walker->table[slot] != 0u)
    {
        const size_t node = _walker->table[slot] - 1u;

        // the commit, met before
        if (memcmp(_walker->nodes[node].oid.bytes, _oid->bytes, size) == 0)
        {
            return node;
        }

        slot = (slot + 1u) & (_walker->table_size - 1u);
    }

    // room for one more
    if (_walker->count == _walker->capacity)
    {
        const size_t                  wanted = (_walker->capacity > 0u)
                                               ? (_walker->capacity * 2u)
                                               : 256u;
        struct d_internal_node* const grown  =
            realloc(_walker->nodes, wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            _walker->failed = true;

            return D_GIT_REPO_NONE;
        }

        _walker->nodes    = grown;
        _walker->capacity = wanted;
    }

    struct d_internal_node* const node = &_walker->nodes[_walker->count];

    memset(node, 0, sizeof(*node));
    node->oid            = *_oid;
    _walker->table[slot] = _walker->count + 1u;

    return _walker->count++;
}


/*
d_internal_is_shallow
  Whether a commit is one the repository's shallow file names, whose
parents git does not see.
*/
static bool
d_internal_is_shallow(
    const struct d_internal_walker* _walker,
    const struct d_git_oid*         _oid
)
{
    // each named
    for (size_t at = 0u; at < _walker->shallow.count; ++at)
    {
        // the commit
        if (d_git_repo_same_oid(_walker->repo,
                                &_walker->shallow.items[at],
                                _oid))
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_parse_node
  A commit read and parsed, once: its date the committer's, its parents
met -- none for a shallow commit.
*/
static bool
d_internal_parse_node(
    struct d_internal_walker* _walker,
    size_t                    _node
)
{
    // parsed already
    if (_walker->nodes[_node].parsed)
    {
        return true;
    }

    struct d_git_object object;
    struct d_git_commit commit;
    const bool          read = ( (d_git_repo_read(_walker->repo,
                                                  &_walker->nodes[_node].oid,
                                                  &object)) &&
                                 (object.type == D_GIT_OBJECT_COMMIT) &&
                                 (d_git_repo_parse_commit(_walker->repo,
                                                          &object,
                                                          &commit)) );

    // a commit that is one
    if (!read)
    {
        d_git_object_release(&object);

        return d_git_repo_fail(_walker->repo, "cannot read commit", NULL);
    }

    const size_t  count   = (d_internal_is_shallow(_walker,
                                                   &_walker->nodes[_node].oid))
                            ? 0u
                            : commit.parent_count;
    size_t* const parents = calloc(count + 1u, sizeof(*parents));
    bool          met     = (parents != NULL);

    // each parent met
    for (size_t at = 0u; ( (met) && (at < count) ); ++at)
    {
        parents[at] = d_internal_node_of(_walker, &commit.parents[at]);
        met         = (parents[at] != D_GIT_REPO_NONE);
    }

    struct d_internal_node* const node = &_walker->nodes[_node];

    node->date         = commit.committer.time;
    node->tree         = commit.tree;
    node->parents      = parents;
    node->parent_count = (met) ? count : 0u;
    node->parsed       = met;
    d_git_commit_release(&commit);
    d_git_object_release(&object);

    return (met) ? true : d_git_repo_fail(_walker->repo, "out of memory",
                                          NULL);
}


/*
d_internal_before_slot
  Whether a queued commit comes off before another: a later date first, of
the same date the one that came first.
*/
static bool
d_internal_before_slot(
    const struct d_internal_walker* _walker,
    const struct d_internal_slot*   _left,
    const struct d_internal_slot*   _right
)
{
    const int64_t left  = _walker->nodes[_left->node].date;
    const int64_t right = _walker->nodes[_right->node].date;

    return ( (left > right) ||
             ( (left == right) && (_left->order < _right->order) ) );
}


/*
d_internal_push
  A commit onto the queue, in its place.
*/
static bool
d_internal_push(
    struct d_internal_walker* _walker,
    size_t                    _node
)
{
    // room for one more
    if (_walker->heap_count == _walker->heap_capacity)
    {
        const size_t                  wanted = (_walker->heap_capacity > 0u)
                                               ? (_walker->heap_capacity * 2u)
                                               : 256u;
        struct d_internal_slot* const grown  =
            realloc(_walker->heap, wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            _walker->failed = true;

            return false;
        }

        _walker->heap          = grown;
        _walker->heap_capacity = wanted;
    }

    size_t at = _walker->heap_count++;

    _walker->heap[at].node  = _node;
    _walker->heap[at].order = _walker->order++;

    // up, past each that comes off after it
    while (at > 0u)
    {
        const size_t up = (at - 1u) / 2u;

        // in its place
        if (!d_internal_before_slot(_walker,
                                    &_walker->heap[at],
                                    &_walker->heap[up]))
        {
            break;
        }

        const struct d_internal_slot swap = _walker->heap[at];

        _walker->heap[at] = _walker->heap[up];
        _walker->heap[up] = swap;
        at                = up;
    }

    return true;
}


/*
d_internal_pop
  The commit that comes off the queue next.
*/
static size_t
d_internal_pop(
    struct d_internal_walker* _walker
)
{
    const size_t node = _walker->heap[0].node;

    _walker->heap[0] = _walker->heap[--_walker->heap_count];

    size_t at = 0u;

    // down, past each that comes off before it
    for (;;)
    {
        const size_t left  = (at * 2u) + 1u;
        const size_t right = left + 1u;
        size_t       first = at;

        // the first of it and its two below
        if ( (left < _walker->heap_count) &&
             (d_internal_before_slot(_walker,
                                     &_walker->heap[left],
                                     &_walker->heap[first])) )
        {
            first = left;
        }

        // the right below
        if ( (right < _walker->heap_count) &&
             (d_internal_before_slot(_walker,
                                     &_walker->heap[right],
                                     &_walker->heap[first])) )
        {
            first = right;
        }

        // in its place
        if (first == at)
        {
            break;
        }

        const struct d_internal_slot swap = _walker->heap[at];

        _walker->heap[at]    = _walker->heap[first];
        _walker->heap[first] = swap;
        at                   = first;
    }

    return node;
}


// d_internal_stack
//   struct: commits waiting to be marked, by index.
struct d_internal_stack
{
    size_t* items;
    size_t  count;
    size_t  capacity;
};


/*
d_internal_stack_add
  One commit onto the stack.
*/
static bool
d_internal_stack_add(
    struct d_internal_stack* _stack,
    size_t                   _node
)
{
    // room for one more
    if (_stack->count == _stack->capacity)
    {
        const size_t  wanted = (_stack->capacity > 0u)
                               ? (_stack->capacity * 2u)
                               : 64u;
        size_t* const grown  = realloc(_stack->items, wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            return false;
        }

        _stack->items    = grown;
        _stack->capacity = wanted;
    }

    _stack->items[_stack->count++] = _node;

    return true;
}


/*
d_internal_stack_push
  A commit's parents onto the stack.
*/
static bool
d_internal_stack_push(
    struct d_internal_stack*      _stack,
    const struct d_internal_node* _node
)
{
    // each parent
    for (size_t at = 0u; at < _node->parent_count; ++at)
    {
        // one more
        if (!d_internal_stack_add(_stack, _node->parents[at]))
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_mark_parents
  Marks the parents of a commit uninteresting, and theirs, as far as
commits have been parsed, as git's mark_parents_uninteresting does.
*/
static bool
d_internal_mark_parents(
    struct d_internal_walker* _walker,
    size_t                    _node
)
{
    struct d_internal_stack pending;
    bool                    marked = true;

    memset(&pending, 0, sizeof(pending));
    marked = d_internal_stack_push(&pending, &_walker->nodes[_node]);

    // each pending commit, its parents pending in turn
    while ( (marked) &&
            (pending.count > 0u) )
    {
        struct d_internal_node* const record =
            &_walker->nodes[pending.items[--pending.count]];

        // marked already
        if ((record->flags & D_INTERNAL_UNINTERESTING) != 0u)
        {
            continue;
        }

        record->flags |= D_INTERNAL_UNINTERESTING;
        marked         = d_internal_stack_push(&pending, record);
    }

    free(pending.items);

    return (marked) ? true : d_git_repo_fail(_walker->repo, "out of memory",
                                             NULL);
}


/*
d_internal_path_of
  What a commit's tree holds at the walk's path, asked once.
*/
static void
d_internal_path_of(
    struct d_internal_walker* _walker,
    size_t                    _node
)
{
    struct d_internal_node* const node = &_walker->nodes[_node];

    // asked already
    if (node->path_asked)
    {
        return;
    }

    struct d_git_oid found;
    uint32_t         mode = 0u;

    node->path_present = d_git_tree_find(_walker->repo,
                                         &node->tree,
                                         _walker->path,
                                         &found,
                                         &mode);
    _walker->nodes[_node].path_asked = true;
    _walker->nodes[_node].path_oid   = found;
    _walker->nodes[_node].path_mode  = mode;
    _walker->repo->problem[0]        = '\0';

    return;
}


/*
d_internal_same_path
  Whether two commits' trees hold the same at the walk's path: nothing in
both, or the same object of the same mode.
*/
static bool
d_internal_same_path(
    struct d_internal_walker* _walker,
    size_t                    _left,
    size_t                    _right
)
{
    d_internal_path_of(_walker, _left);
    d_internal_path_of(_walker, _right);

    const struct d_internal_node* const left  = &_walker->nodes[_left];
    const struct d_internal_node* const right = &_walker->nodes[_right];

    // nothing in both, or the same in both
    if ( (!left->path_present) ||
         (!right->path_present) )
    {
        return (left->path_present == right->path_present);
    }

    return ( (left->path_mode == right->path_mode) &&
             (d_git_repo_same_oid(_walker->repo,
                                  &left->path_oid,
                                  &right->path_oid)) );
}


/*
d_internal_simplify
  A commit simplified as git's try_to_simplify_commit simplifies one for a
path: a root is TREESAME when its tree has nothing there; a commit with a
parent of the same there, that parent interesting, has that parent alone
and is TREESAME; otherwise it is TREESAME only when no parent that counts
differs -- the interesting ones, if any are, else all.
  Long as try_to_simplify_commit is, which it follows step by step.
*/
static bool
d_internal_simplify(
    struct d_internal_walker* _walker,
    size_t                    _node
)
{
    bool   relevant_change   = false;
    bool   irrelevant_change = false;
    size_t relevant_parents  = 0u;

    // a root
    if (_walker->nodes[_node].parent_count == 0u)
    {
        d_internal_path_of(_walker, _node);

        // nothing at the path
        if (!_walker->nodes[_node].path_present)
        {
            _walker->nodes[_node].flags |= D_INTERNAL_TREESAME;
        }

        return true;
    }

    // each parent, the first alone for first parents
    for (size_t at = 0u; at < _walker->nodes[_node].parent_count; ++at)
    {
        const size_t parent   = _walker->nodes[_node].parents[at];
        const bool   relevant = ((_walker->nodes[parent].flags
                                  & D_INTERNAL_UNINTERESTING) == 0u);

        relevant_parents += (relevant) ? 1u : 0u;

        // first parents only compare with the first
        if ( (at == 1u) &&
             (_walker->first_parent) )
        {
            break;
        }

        // the parent parsed
        if (!d_internal_parse_node(_walker, parent))
        {
            return false;
        }

        // the same: this parent alone, if it counts
        if (d_internal_same_path(_walker, _node, parent))
        {
            // one that does not count is passed over
            if (!relevant)
            {
                continue;
            }

            _walker->nodes[_node].parents[0]    = parent;
            _walker->nodes[_node].parent_count  = 1u;
            _walker->nodes[_node].flags        |= D_INTERNAL_TREESAME;

            return true;
        }

        relevant_change   = ( (relevant_change) || (relevant) );
        irrelevant_change = ( (irrelevant_change) || (!relevant) );
    }

    // TREESAME when nothing that counts changed
    if ( (relevant_parents > 0u) ? (!relevant_change)
                                 : (!irrelevant_change) )
    {
        _walker->nodes[_node].flags |= D_INTERNAL_TREESAME;
    }

    return true;
}


/*
d_internal_put_parent
  A parent onto the queue, once: marked seen when it goes on.
*/
static bool
d_internal_put_parent(
    struct d_internal_walker* _walker,
    size_t                    _parent
)
{
    // seen already
    if ((_walker->nodes[_parent].flags & D_INTERNAL_SEEN) != 0u)
    {
        return true;
    }

    _walker->nodes[_parent].flags |= D_INTERNAL_SEEN;

    return d_internal_push(_walker, _parent);
}


/*
d_internal_process_parents
  A commit's parents, as git's process_parents puts them on the queue,
once a commit: an uninteresting commit's every parent, marked, theirs
marked as far as they are parsed; an interesting one's, simplified for a
path, the first alone for first parents.
  Long as process_parents is, which it follows.
*/
static bool
d_internal_process_parents(
    struct d_internal_walker* _walker,
    size_t                    _node
)
{
    // once a commit
    if ((_walker->nodes[_node].flags & D_INTERNAL_ADDED) != 0u)
    {
        return true;
    }

    _walker->nodes[_node].flags |= D_INTERNAL_ADDED;

    // the commit parsed
    if (!d_internal_parse_node(_walker, _node))
    {
        return false;
    }

    // an uninteresting commit's parents are too
    if ((_walker->nodes[_node].flags & D_INTERNAL_UNINTERESTING) != 0u)
    {
        // each parent
        for (size_t at = 0u; at < _walker->nodes[_node].parent_count; ++at)
        {
            const size_t parent = _walker->nodes[_node].parents[at];

            _walker->nodes[parent].flags |= D_INTERNAL_UNINTERESTING;

            // parsed, its parents marked; then onto the queue
            if ( (d_internal_parse_node(_walker, parent)) &&
                 ( (!d_internal_mark_parents(_walker, parent)) ||
                   (!d_internal_put_parent(_walker, parent)) ) )
            {
                return false;
            }
        }

        _walker->repo->problem[0] = '\0';

        return !_walker->failed;
    }

    // simplified for a path
    if ( (_walker->path) &&
         (!d_internal_simplify(_walker, _node)) )
    {
        return false;
    }

    // each parent, or the first
    for (size_t at = 0u; at < _walker->nodes[_node].parent_count; ++at)
    {
        const size_t parent = _walker->nodes[_node].parents[at];

        // parsed, and onto the queue
        if ( (!d_internal_parse_node(_walker, parent)) ||
             (!d_internal_put_parent(_walker, parent)) )
        {
            return false;
        }

        // the first alone
        if (_walker->first_parent)
        {
            break;
        }
    }

    return true;
}


/*
d_internal_read_shallow
  The shallow file's commits, a name in hex to a line.
*/
static bool
d_internal_read_shallow(
    struct d_internal_walker* _walker
)
{
    size_t      length = 0u;
    char* const text   = d_internal_read_in(_walker->repo->common_dir,
                                            "shallow",
                                            &length);
    bool        read   = true;

    // each line
    for (char* line = text; ( (line) && (read) && (*line != '\0') );)
    {
        char* const end = strchr(line, '\n');

        // the line alone
        if (end)
        {
            *end = '\0';
        }

        d_internal_trim_end(line);

        struct d_git_oid oid;

        // a name, kept
        if (d_git_repo_parse_hex(_walker->repo, line, &oid))
        {
            read = d_internal_oids_add(&_walker->shallow, &oid);
        }

        line = (end) ? (end + 1) : NULL;
    }

    free(text);

    return read;
}


/*
d_internal_walker_free
  Lets go of what a walk made.
*/
static void
d_internal_walker_free(
    struct d_internal_walker* _walker
)
{
    // each commit's parents
    for (size_t at = 0u; at < _walker->count; ++at)
    {
        free(_walker->nodes[at].parents);
    }

    free(_walker->nodes);
    free(_walker->table);
    free(_walker->heap);
    d_git_oids_release(&_walker->shallow);

    return;
}


/*
d_internal_compare_starts
  Orders a walk's first commits by date, the latest first; qsort is not
stable, so the order they were given breaks ties.
*/
static int
d_internal_compare_starts(
    const void* _left,
    const void* _right
)
{
    const int64_t* const left  = _left;
    const int64_t* const right = _right;

    // date, then the order given
    if (left[0] != right[0])
    {
        return (left[0] > right[0]) ? -1 : 1;
    }

    return (left[1] < right[1]) ? -1 : ( (left[1] > right[1]) ? 1 : 0 );
}


/*
d_internal_start
  One revision a walk starts from: named, peeled to its commit, marked --
uninteresting, its parents too, when it is excluded -- and kept to be
queued, once.
*/
static bool
d_internal_start(
    struct d_internal_walker* _walker,
    const char*               _revision,
    struct d_internal_stack*  _starts,
    bool*                     _out_limited
)
{
    const bool       excluded = (_revision[0] == '^');
    struct d_git_oid oid;

    // the revision's commit
    if ( (!d_git_repo_rev(_walker->repo, &_revision[(excluded) ? 1 : 0],
                          &oid)) ||
         (!d_internal_peel_to(_walker->repo, &oid, D_GIT_OBJECT_COMMIT)) )
    {
        return false;
    }

    const size_t node = d_internal_node_of(_walker, &oid);

    // the commit met, and parsed
    if ( (node == D_GIT_REPO_NONE) ||
         (!d_internal_parse_node(_walker, node)) )
    {
        return false;
    }

    // excluded: it and its parents uninteresting
    if (excluded)
    {
        _walker->nodes[node].flags |= D_INTERNAL_UNINTERESTING;
        *_out_limited               = true;

        // its parents too
        if (!d_internal_mark_parents(_walker, node))
        {
            return false;
        }
    }

    // queued once
    if ((_walker->nodes[node].flags & D_INTERNAL_SEEN) != 0u)
    {
        return true;
    }

    _walker->nodes[node].flags |= D_INTERNAL_SEEN;

    return d_internal_stack_add(_starts, node);
}


/*
d_internal_queue_starts
  The first commits onto the queue, by date, ties in the order given, as
prepare_revision_walk sorts them.
*/
static bool
d_internal_queue_starts(
    struct d_internal_walker*      _walker,
    const struct d_internal_stack* _starts
)
{
    int64_t* const keys = malloc((_starts->count + 1u) * 2u * sizeof(*keys));

    // room for the keys
    if (!keys)
    {
        return false;
    }

    // each start's date and place
    for (size_t at = 0u; at < _starts->count; ++at)
    {
        keys[at * 2u]        = _walker->nodes[_starts->items[at]].date;
        keys[(at * 2u) + 1u] = (int64_t)at;
    }

    qsort(keys, _starts->count, 2u * sizeof(*keys), d_internal_compare_starts);

    bool queued = true;

    // in that order
    for (size_t at = 0u; ( (queued) && (at < _starts->count) ); ++at)
    {
        queued = d_internal_push(_walker,
                                 _starts->items[(size_t)keys[(at * 2u) + 1u]]);
    }

    free(keys);

    return queued;
}


/*
d_internal_still_interesting
  Whether a limited walk goes on past an uninteresting commit, as git's
still_interesting decides: while the queue holds a commit as late as the
last kept, or any interesting one, and for a few commits more.
*/
static int
d_internal_still_interesting(
    const struct d_internal_walker* _walker,
    int64_t                         _date,
    int                             _slop
)
{
    // nothing left
    if (_walker->heap_count == 0u)
    {
        return 0;
    }

    // a commit no older than the last kept
    if (_date <= _walker->nodes[_walker->heap[0].node].date)
    {
        return D_INTERNAL_SLOP;
    }

    // any interesting one
    for (size_t at = 0u; at < _walker->heap_count; ++at)
    {
        // one interesting
        if ((_walker->nodes[_walker->heap[at].node].flags
             & D_INTERNAL_UNINTERESTING) == 0u)
        {
            return D_INTERNAL_SLOP;
        }
    }

    return _slop - 1;
}


/*
d_internal_wanted
  Whether a commit comes out of a walk: not uninteresting, nor TREESAME to
the path the walk is limited to.
*/
static bool
d_internal_wanted(
    const struct d_internal_walker* _walker,
    size_t                          _node
)
{
    const unsigned flags = _walker->nodes[_node].flags;

    return ( ((flags & D_INTERNAL_UNINTERESTING) == 0u) &&
             ( (!_walker->path) ||
               ((flags & D_INTERNAL_TREESAME) == 0u) ) );
}


/*
d_internal_limit
  A limited walk, as git's limit_list makes one: the whole queue walked
first, interesting commits kept in the order they came off, until only
uninteresting ones remain; then the kept that are still interesting.
*/
static bool
d_internal_limit(
    struct d_internal_walker*       _walker,
    const struct d_git_log_options* _options,
    struct d_git_oids*              _out
)
{
    struct d_internal_stack kept;
    int64_t                 date = INT64_MAX;
    int                     slop = D_INTERNAL_SLOP;
    bool                    went = true;

    memset(&kept, 0, sizeof(kept));

    // the queue, until only the uninteresting remain
    while ( (went) &&
            (_walker->heap_count > 0u) )
    {
        const size_t node = d_internal_pop(_walker);

        went = d_internal_process_parents(_walker, node);

        // uninteresting: its parents marked; perhaps the end
        if ( (went) &&
             ((_walker->nodes[node].flags & D_INTERNAL_UNINTERESTING)
                  != 0u) )
        {
            went = d_internal_mark_parents(_walker, node);
            slop = d_internal_still_interesting(_walker, date, slop);

            // the end
            if (slop == 0)
            {
                break;
            }

            continue;
        }

        date = _walker->nodes[node].date;
        went = ( (went) &&
                 (d_internal_stack_add(&kept, node)) );
    }

    // the kept that came out interesting, as many as asked
    for (size_t at = 0u;
         ( (went) &&
           (at < kept.count) &&
           ( (_options->most == 0u) || (_out->count < _options->most) ) );
         ++at)
    {
        // one wanted
        if (d_internal_wanted(_walker, kept.items[at]))
        {
            went = d_internal_oids_add(_out,
                                       &_walker->nodes[kept.items[at]].oid);
        }
    }

    free(kept.items);

    return went;
}


/*
d_internal_stream
  A walk with nothing excluded, as git's get_revision_1 makes one: each
commit off the queue, its parents on, given unless simplified away.
*/
static bool
d_internal_stream(
    struct d_internal_walker*       _walker,
    const struct d_git_log_options* _options,
    struct d_git_oids*              _out
)
{
    // until the queue is empty, or enough have come
    while ( (_walker->heap_count > 0u) &&
            ( (_options->most == 0u) || (_out->count < _options->most) ) )
    {
        const size_t node = d_internal_pop(_walker);

        // the parents, then the commit if it is wanted
        if ( (!d_internal_process_parents(_walker, node)) ||
             ( (d_internal_wanted(_walker, node)) &&
               (!d_internal_oids_add(_out, &_walker->nodes[node].oid)) ) )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_walk_path
  The walk's path as a pathspec names one: no trailing slash; NULL for
none, or for the top.
*/
static char*
d_internal_walk_path(
    const char* _path
)
{
    // no path, or the top
    if ( (!_path) ||
         (_path[0] == '\0') ||
         (strcmp(_path, ".") == 0) )
    {
        return NULL;
    }

    char* const path   = d_internal_dup_text(
                             (strncmp(_path, "./", 2u) == 0) ? &_path[2]
                                                             : _path);
    size_t      length = (path) ? strlen(path) : 0u;

    // the trailing slashes off
    while ( (length > 0u) &&
            (path[length - 1u] == '/') )
    {
        path[--length] = '\0';
    }

    return path;
}


bool
d_git_repo_log(
    struct d_git_repo*              _repo,
    const struct d_git_log_options* _options,
    struct d_git_oids*              _out
)
{
    struct d_internal_walker walker;
    struct d_internal_stack  starts;
    bool                     limited = false;

    memset(_out, 0, sizeof(*_out));

    // a repository, and what to walk
    if ( (!_repo) ||
         (!_options) )
    {
        return false;
    }

    memset(&walker, 0, sizeof(walker));
    memset(&starts, 0, sizeof(starts));
    walker.repo         = _repo;
    walker.first_parent = _options->first_parent;

    char* const path = d_internal_walk_path(_options->path);

    walker.path = path;

    bool walked = d_internal_read_shallow(&walker);

    // each revision, then the walk
    for (size_t at = 0u;
         ( (walked) && (at < _options->revision_count) );
         ++at)
    {
        walked = d_internal_start(&walker,
                                  _options->revisions[at],
                                  &starts,
                                  &limited);
    }

    walked = ( (walked) &&
               (d_internal_queue_starts(&walker, &starts)) &&
               ( (limited) ? d_internal_limit(&walker, _options, _out)
                           : d_internal_stream(&walker, _options, _out) ) &&
               (!walker.failed) );
    free(starts.items);
    free(path);
    d_internal_walker_free(&walker);

    return walked;
}


/*
d_internal_has_fresh
  Whether the queue holds a commit not yet marked stale.
*/
static bool
d_internal_has_fresh(
    const struct d_internal_walker* _walker
)
{
    // each queued
    for (size_t at = 0u; at < _walker->heap_count; ++at)
    {
        // one not stale
        if ((_walker->nodes[_walker->heap[at].node].flags
             & D_INTERNAL_STALE) == 0u)
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_paint
  Paints down from two commits to what they share, as git's
paint_down_to_common does: each commit off the queue takes the marks of
the commits it was reached from to its parents; one reached from both is
a result, and its parents are stale.
*/
static bool
d_internal_paint(
    struct d_internal_walker* _walker,
    size_t                    _one,
    size_t                    _two,
    struct d_internal_stack*  _out
)
{
    _walker->nodes[_one].flags |= D_INTERNAL_PARENT1;
    _walker->nodes[_two].flags |= D_INTERNAL_PARENT2;

    bool painted = ( (d_internal_push(_walker, _two)) &&
                     (d_internal_push(_walker, _one)) );

    // while anything queued is not stale
    while ( (painted) &&
            (d_internal_has_fresh(_walker)) )
    {
        const size_t node  = d_internal_pop(_walker);
        unsigned     flags = _walker->nodes[node].flags
                             & (D_INTERNAL_PARENT1 | D_INTERNAL_PARENT2
                                | D_INTERNAL_STALE);

        // reached from both: a result, its parents stale
        if (flags == (D_INTERNAL_PARENT1 | D_INTERNAL_PARENT2))
        {
            // a result once
            if ((_walker->nodes[node].flags & D_INTERNAL_RESULT) == 0u)
            {
                _walker->nodes[node].flags |= D_INTERNAL_RESULT;
                painted = d_internal_stack_add(_out, node);
            }

            flags |= D_INTERNAL_STALE;
        }

        painted = ( (painted) &&
                    (d_internal_parse_node(_walker, node)) );

        // the marks onto each parent that lacks them
        for (size_t at = 0u;
             ( (painted) && (at < _walker->nodes[node].parent_count) );
             ++at)
        {
            const size_t parent = _walker->nodes[node].parents[at];

            // a parent with them all already
            if ((_walker->nodes[parent].flags & flags) == flags)
            {
                continue;
            }

            painted = d_internal_parse_node(_walker, parent);
            _walker->nodes[parent].flags |= flags;
            painted = ( (painted) && (d_internal_push(_walker, parent)) );
        }
    }

    return painted;
}


/*
d_internal_reaches
  Whether one commit reaches another by its parents: a walk of every
ancestor, each visited once by this walk's generation.
*/
static bool
d_internal_reaches(
    struct d_internal_walker* _walker,
    size_t                    _from,
    size_t                    _to,
    bool*                     _out_reaches
)
{
    struct d_internal_stack pending;

    memset(&pending, 0, sizeof(pending));
    *_out_reaches = false;
    ++_walker->generation;

    bool went = d_internal_stack_add(&pending, _from);

    // a commit at a time, until the other is met
    while ( (went) &&
            (pending.count > 0u) )
    {
        const size_t node = pending.items[--pending.count];

        // met, or visited
        if (node == _to)
        {
            *_out_reaches = true;

            break;
        }
        else if (_walker->nodes[node].visit == _walker->generation)
        {
            continue;
        }

        _walker->nodes[node].visit = _walker->generation;
        went = ( (d_internal_parse_node(_walker, node)) &&
                 (d_internal_stack_push(&pending, &_walker->nodes[node])) );
    }

    free(pending.items);

    return went;
}


/*
d_internal_remove_redundant
  Leaves among merge bases only those no other reaches, as git's
remove_redundant does.
*/
static bool
d_internal_remove_redundant(
    struct d_internal_walker* _walker,
    struct d_internal_stack*  _bases
)
{
    size_t kept = 0u;

    // each base, kept unless another reaches it
    for (size_t at = 0u; at < _bases->count; ++at)
    {
        bool reached = false;

        // each other base
        for (size_t other = 0u;
             ( (!reached) && (other < _bases->count) );
             ++other)
        {
            // another, which reaches it
            if ( (other != at) &&
                 (!d_internal_reaches(_walker,
                                      _bases->items[other],
                                      _bases->items[at],
                                      &reached)) )
            {
                return false;
            }
        }

        // kept, in its order
        if (!reached)
        {
            _bases->items[kept++] = _bases->items[at];
        }
    }

    _bases->count = kept;

    return true;
}


/*
d_git_repo_merge_base
  Long as paint_down_to_common and remove_redundant are each run here in
turn.
*/
bool
d_git_repo_merge_base(
    struct d_git_repo*      _repo,
    const struct d_git_oid* _left,
    const struct d_git_oid* _right,
    struct d_git_oids*      _out
)
{
    struct d_internal_walker walker;
    struct d_internal_stack  bases;

    memset(_out, 0, sizeof(*_out));

    // a repository and two commits
    if ( (!_repo) ||
         (!_left) ||
         (!_right) )
    {
        return false;
    }

    struct d_git_oid left  = *_left;
    struct d_git_oid right = *_right;

    memset(&walker, 0, sizeof(walker));
    memset(&bases, 0, sizeof(bases));
    walker.repo = _repo;

    // each peeled to its commit
    if ( (!d_internal_peel_to(_repo, &left, D_GIT_OBJECT_COMMIT)) ||
         (!d_internal_peel_to(_repo, &right, D_GIT_OBJECT_COMMIT)) )
    {
        return false;
    }

    const size_t one = d_internal_node_of(&walker, &left);
    const size_t two = (one != D_GIT_REPO_NONE)
                       ? d_internal_node_of(&walker, &right)
                       : D_GIT_REPO_NONE;
    bool         found = ( (two != D_GIT_REPO_NONE) &&
                           (d_internal_read_shallow(&walker)) &&
                           (d_internal_parse_node(&walker, one)) &&
                           (d_internal_parse_node(&walker, two)) );

    // one commit is its own base; two are painted down to theirs
    if ( (found) &&
         (one == two) )
    {
        found = d_internal_oids_add(_out, &left);
    }
    else if (found)
    {
        found = ( (d_internal_paint(&walker, one, two, &bases)) &&
                  (d_internal_remove_redundant(&walker, &bases)) );

        // those not made stale after, in the order found
        for (size_t at = 0u; ( (found) && (at < bases.count) ); ++at)
        {
            // one not stale
            if ((walker.nodes[bases.items[at]].flags & D_INTERNAL_STALE)
                    == 0u)
            {
                found = d_internal_oids_add(_out,
                                            &walker.nodes[bases.items[at]]
                                                 .oid);
            }
        }
    }

    free(bases.items);
    d_internal_walker_free(&walker);

    return found;
}
