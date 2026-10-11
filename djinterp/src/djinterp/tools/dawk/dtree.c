/*******************************************************************************
* djinterp [djinterp]                                                    dtree.c
*
* Filesystem tree record source:
*   Yields one record per file beneath a root, in document order: a directory
* is visited before its contents, and entries within a directory are sorted so
* that a run over the same tree twice produces the same NR for the same file.
*   The record text is the file's path.  A path is what a file node's
* to_string would return, so a program written against this source stays
* written correctly when the source is replaced by a real node tree.
*   What a directory holds and what a path names are asked of dfs.h, so this
* unit is ISO C.  dfs.c, which answers, is not, and the conformance build
* links neither.
*
*
* path:      /src/djinterp/tools/dawk/dtree.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.10.10
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dtree.h"  // corresponding header
// std
#include <stdlib.h>  // malloc, realloc, free, qsort
#include <string.h>  // strcmp, strlen, memcpy
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dfs.h"  // d_fs_names, d_fs_kind


// d_internal_tree
//   struct: the walk state.  `pending` is an explicit stack of directories
// still to expand, so depth costs heap rather than C stack and a deep tree
// cannot overflow.  `path` is the buffer handed back as the record.
struct d_internal_tree
{
    char**  pending;
    size_t  pending_count;
    size_t  pending_capacity;

    char*   path;
    size_t  path_capacity;
};


/*
d_internal_tree_compare
  Orders two directory entries by name so document order is deterministic.
*/
static int
d_internal_tree_compare(
    const void* _left,
    const void* _right
)
{
    const char* const* const left  = (const char* const*)_left;
    const char* const* const right = (const char* const*)_right;

    return strcmp(*left, *right);
}


/*
d_internal_tree_push
  Pushes one path onto the pending stack, taking ownership of it.
*/
static bool
d_internal_tree_push(
    struct d_internal_tree* _tree,
    char*                   _path
)
{
    if (_tree->pending_count == _tree->pending_capacity)
    {
        const size_t grown_capacity = (_tree->pending_capacity == 0)
                                    ? 16u
                                    : (_tree->pending_capacity * 2u);

        char** const grown = realloc(_tree->pending,
                                     grown_capacity * sizeof(char*));

        // the caller frees the path when the push fails
        if (!grown)
        {
            return false;
        }

        _tree->pending          = grown;
        _tree->pending_capacity = grown_capacity;
    }

    _tree->pending[_tree->pending_count] = _path;
    ++_tree->pending_count;

    return true;
}


/*
d_internal_tree_join
  Builds "<dir>/<name>" into a fresh allocation, collapsing the case where the
directory already ends in a separator so a root of "/" does not produce "//".
*/
static char*
d_internal_tree_join(
    const char* _dir,
    const char* _name
)
{
    const size_t dir_length  = strlen(_dir);
    const size_t name_length = strlen(_name);
    const bool   has_slash   = (dir_length > 0)
                            && (_dir[dir_length - 1u] == '/');
    const size_t total       = dir_length + (has_slash ? 0u : 1u)
                             + name_length + 1u;

    char* const joined = malloc(total);

    if (!joined)
    {
        return NULL;
    }

    memcpy(joined, _dir, dir_length);

    size_t at = dir_length;

    if (!has_slash)
    {
        joined[at] = '/';
        ++at;
    }

    memcpy(joined + at, _name, name_length);
    joined[at + name_length] = '\0';

    return joined;
}


/*
d_internal_tree_expand
  Reads one directory, sorts its entries, and pushes them so that the first
entry by name is popped first.  The system gives the names in no order, and
not the same order on every system; the sort is what makes a run the same
twice and the same everywhere.  A directory that cannot be opened is skipped
rather than fatal: an unreadable corner of a tree should not end a run over
the rest of it.
*/
static void
d_internal_tree_expand(
    struct d_internal_tree* _tree,
    const char*             _dir
)
{
    char** names = NULL;
    size_t count = 0;

    // an unreadable directory contributes no records and is not an error
    if (!d_fs_names(_dir, &names, &count))
    {
        return;
    }

    size_t kept = 0;

    // each name gives way to its path beneath this directory; one that
    // memory has no room for is left out, as an unreadable one would be
    for (size_t at = 0; at < count; ++at)
    {
        char* const joined = d_internal_tree_join(_dir, names[at]);

        free(names[at]);

        if (joined)
        {
            names[kept] = joined;
            ++kept;
        }
    }

    if (kept > 1)
    {
        qsort(names, kept, sizeof(char*), d_internal_tree_compare);
    }

    // pushed in reverse so the first name by sort order is popped first
    for (size_t at = kept; at > 0; --at)
    {
        if (!d_internal_tree_push(_tree, names[at - 1u]))
        {
            free(names[at - 1u]);
        }
    }

    free(names);

    return;
}


/*
d_internal_tree_next
  Pops the next path, expanding it first when it is a directory.  A directory
yields no record of its own in this source; only files do.  The loop therefore
continues past directories rather than returning for them.
*/
static bool
d_internal_tree_next(
    void*        _user,
    const char** _out_text,
    size_t*      _out_length
)
{
    struct d_internal_tree* const tree = (struct d_internal_tree*)_user;

    while (tree->pending_count > 0)
    {
        --tree->pending_count;

        char* const path = tree->pending[tree->pending_count];

        const enum d_fs_kind kind = d_fs_kind_of(path);

        // a path that names nothing is skipped along with its subtree
        if (kind == D_FS_KIND_NONE)
        {
            free(path);
            continue;
        }

        if (kind == D_FS_KIND_DIRECTORY)
        {
            d_internal_tree_expand(tree, path);
            free(path);
            continue;
        }

        const size_t length = strlen(path);

        // the record buffer is reused, so it grows to the longest path seen
        if (tree->path_capacity < (length + 1u))
        {
            char* const grown = realloc(tree->path, length + 1u);

            if (!grown)
            {
                free(path);

                return false;
            }

            tree->path          = grown;
            tree->path_capacity = length + 1u;
        }

        memcpy(tree->path, path, length + 1u);
        free(path);

        *_out_text   = tree->path;
        *_out_length = length;

        return true;
    }

    return false;
}


/*
d_internal_tree_release
  Frees the walk state and every path still pending.
*/
static void
d_internal_tree_release(
    void* _user
)
{
    struct d_internal_tree* const tree = (struct d_internal_tree*)_user;

    if (tree)
    {
        for (size_t at = 0; at < tree->pending_count; ++at)
        {
            free(tree->pending[at]);
        }

        free(tree->pending);
        free(tree->path);
        free(tree);
    }

    return;
}


/*
d_awk_source_tree_init
  Seeds the walk with the root and fills the descriptor.  The descriptor is
caller-owned so that a host may keep it on the stack; only the walk state
behind it is allocated, and the interpreter's teardown releases that.
*/
bool
d_awk_source_tree_init(
    struct d_awk_source* _source,
    const char*          _root
)
{
    // parameter validation first
    if ((!_source) || (!_root))
    {
        return false;
    }

    struct d_internal_tree* const tree = calloc(1u, sizeof(*tree));

    if (!tree)
    {
        return false;
    }

    const size_t root_length = strlen(_root);

    char* const seed = malloc(root_length + 1u);

    if (!seed)
    {
        free(tree);

        return false;
    }

    memcpy(seed, _root, root_length + 1u);

    if (!d_internal_tree_push(tree, seed))
    {
        free(seed);
        d_internal_tree_release(tree);

        return false;
    }

    _source->user        = tree;
    _source->next_record = d_internal_tree_next;
    _source->release     = d_internal_tree_release;

    return true;
}
