/*******************************************************************************
* djinterp [test]                                                     test_dfs.c
*
* Exercises `dfs.h`, and the walk over a tree that is built on it.
*   Run from the repository's root, over the small tree kept beside this file
* for it: tests/dawk/fs/root. What is checked is what the header promises on
* every system, so the same file is the test on POSIX and on Windows; where
* the two differ by design -- what `\` is, whether a full name is looked up
* -- each answer the header allows is accepted, and none it does not.
*
* path:      /tests/dawk/test_dfs.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.10
*******************************************************************************/
#include "../../inc/djinterp/tools/dawk/dfs.h"    // d_fs_kind_of, d_fs_names
// std
#include <stdio.h>                                // printf
#include <stdlib.h>                               // free, qsort
#include <string.h>                               // strcmp, strlen, strchr
// djinterp
#include "../../inc/djinterp/tools/dawk/dtree.h"  // d_awk_source_tree_init


// ROOT
//   constant: the tree the checks read, from the repository's root.
static const char* const ROOT = "tests/dawk/fs/root";

// failures
//   variable: how many checks have not held.
static int failures = 0;

/*
d_internal_check
  Counts a check that did not hold, and says which.
*/
static void
d_internal_check(
    bool        _held,
    const char* _what
)
{
    if (!_held)
    {
        (void)printf("  FAIL %s\n", _what);
        ++failures;
    }

    return;
}

/*
d_internal_by_name
  Orders two names for qsort: the system gives them in no order.
*/
static int
d_internal_by_name(
    const void* _left,
    const void* _right
)
{
    const char* const* const left  = (const char* const*)_left;
    const char* const* const right = (const char* const*)_right;

    return strcmp(*left, *right);
}

/*
d_internal_holds
  Reads a directory and tells whether it holds exactly the names given, which
are given sorted. The names read are released whatever the answer.
*/
static bool
d_internal_holds(
    const char*        _directory,
    const char* const* _expected,
    size_t             _expected_count
)
{
    char** names = NULL;
    size_t count = 0u;

    if (!d_fs_names(_directory, &names, &count))
    {
        return false;
    }

    if (count > 1u)
    {
        qsort(names, count, sizeof(char*), d_internal_by_name);
    }

    bool same = (count == _expected_count);

    for (size_t at = 0u; (same) && (at < count); ++at)
    {
        same = (strcmp(names[at], _expected[at]) == 0);
    }

    d_fs_names_free(names, count);

    return same;
}

/*
d_internal_is_absolute
  Tells whether a path starts at a root: `/` on POSIX; on Windows a drive, as
in `C:\`, or the two separators of a network name.
*/
static bool
d_internal_is_absolute(
    const char* _path
)
{
    const size_t length = strlen(_path);

    if ( (length >= 1u) &&
         (_path[0] == '/') )
    {
        return true;
    }

    if ( (length >= 2u)        &&
         (_path[0] == '\\')    &&
         (_path[1] == '\\') )
    {
        return true;
    }

    return ( (length >= 3u)     &&
             (_path[1] == ':')  &&
             ( (_path[2] == '\\') ||
               (_path[2] == '/') ) );
}

/*
d_internal_kinds
  What a path names: a directory, with a separator at its end or without; a
file; and nothing.
*/
static void
d_internal_kinds(void)
{
    d_internal_check(d_fs_kind_of(ROOT) == D_FS_KIND_DIRECTORY,
                     "the root is a directory");
    d_internal_check(d_fs_kind_of("tests/dawk/fs/root/") ==
                         D_FS_KIND_DIRECTORY,
                     "a separator at its end changes nothing");
    d_internal_check(d_fs_kind_of("tests/dawk/fs/root/b/d") ==
                         D_FS_KIND_DIRECTORY,
                     "a directory two levels down is a directory");
    d_internal_check(d_fs_kind_of("tests/dawk/fs/root/a.txt") ==
                         D_FS_KIND_FILE,
                     "a file is a file");
    d_internal_check(d_fs_kind_of("tests/dawk/fs/root/none") ==
                         D_FS_KIND_NONE,
                     "a path that names nothing names nothing");
    d_internal_check(d_fs_kind_of("tests/dawk/fs/root/a.txt/x") ==
                         D_FS_KIND_NONE,
                     "nor does a path beneath a file");
    d_internal_check(d_fs_kind_of(NULL) == D_FS_KIND_NONE,
                     "nor does no path at all");

    return;
}

/*
d_internal_names
  What a directory holds: its own entries and no more, at every level, and
nothing for what is not a directory.
*/
static void
d_internal_names(void)
{
    static const char* const top[]    = { "a.txt", "b", "z.txt" };
    static const char* const middle[] = { "c.txt", "d" };
    static const char* const bottom[] = { "e.txt" };

    d_internal_check(d_internal_holds(ROOT, top, 3u),
                     "the root holds a.txt, b and z.txt, and no . or ..");
    d_internal_check(d_internal_holds("tests/dawk/fs/root/", top, 3u),
                     "and the same read with a separator at its end");
    d_internal_check(d_internal_holds("tests/dawk/fs/root/b", middle, 2u),
                     "b holds c.txt and d");
    d_internal_check(d_internal_holds("tests/dawk/fs/root/b/d", bottom, 1u),
                     "d holds e.txt");

    char** names = NULL;
    size_t count = 7u;

    d_internal_check(!d_fs_names("tests/dawk/fs/root/a.txt", &names, &count),
                     "a file is not read as a directory");
    d_internal_check( (names == NULL) && (count == 0u),
                     "and a read that failed leaves nothing behind");
    d_internal_check(!d_fs_names("tests/dawk/fs/root/none", &names, &count),
                     "nor is a path that names nothing");
    d_internal_check(!d_fs_names(NULL, &names, &count),
                     "nor no path at all");
    d_internal_check(!d_fs_names(ROOT, NULL, &count),
                     "and a read with nowhere to put the names is refused");

    return;
}

/*
d_internal_full_paths
  A full name starts at a root, is its own full name, and is the same for two
spellings of one file that differ by a `..`.
*/
static void
d_internal_full_paths(void)
{
    char* const plain = d_fs_full_path("tests/dawk/fs/root/a.txt");
    char* const round = d_fs_full_path("tests/dawk/fs/root/b/../a.txt");

    d_internal_check( (plain != NULL) && (round != NULL),
                     "a file that is there has a full name");

    if ( (plain) &&
         (round) )
    {
        char* const again = d_fs_full_path(plain);

        d_internal_check(d_internal_is_absolute(plain),
                         "which starts at a root");
        d_internal_check(strcmp(plain, round) == 0,
                         "and is the same by way of a ..");
        d_internal_check( (again != NULL) && (strcmp(plain, again) == 0),
                         "and is its own full name");

        free(again);
    }

    free(plain);
    free(round);

    d_internal_check(d_fs_full_path(NULL) == NULL,
                     "no path has no full name");

    return;
}

/*
d_internal_normalized
  A path with `/` alone is left as it is everywhere. One typed with `\` is
either rewritten to name the same directory, as on Windows, or left as the
name it is, as on POSIX.
*/
static void
d_internal_normalized(void)
{
    char forward[]  = "tests/dawk/fs/root";
    char backward[] = "tests\\dawk\\fs\\root";

    d_fs_path_normalize(forward);
    d_fs_path_normalize(backward);
    d_fs_path_normalize(NULL);

    d_internal_check(strcmp(forward, ROOT) == 0,
                     "a path with / alone is left as it is");

    if (strchr(backward, '\\') == NULL)
    {
        d_internal_check(strcmp(backward, ROOT) == 0,
                         "a path typed with \\ is rewritten with /");
        d_internal_check(d_fs_kind_of(backward) == D_FS_KIND_DIRECTORY,
                         "and names what it named");
    }
    else
    {
        d_internal_check(strcmp(backward, "tests\\dawk\\fs\\root") == 0,
                         "a \\ that is a character of a name is left alone");
    }

    return;
}

/*
d_internal_walk
  The walk over the tree: every file and no directory, each by its path from
the root given, in the one order a sort by name gives on every system.
*/
static void
d_internal_walk(void)
{
    static const char* const expected[] =
    {
        "tests/dawk/fs/root/a.txt",
        "tests/dawk/fs/root/b/c.txt",
        "tests/dawk/fs/root/b/d/e.txt",
        "tests/dawk/fs/root/z.txt"
    };

    struct d_awk_source source = { NULL, NULL, NULL };

    if (!d_awk_source_tree_init(&source, ROOT))
    {
        d_internal_check(false, "the walk begins");

        return;
    }

    const char* text   = NULL;
    size_t      length = 0u;
    size_t      seen   = 0u;
    bool        same   = true;

    while (source.next_record(source.user, &text, &length))
    {
        same = (same)                               &&
               (seen < 4u)                          &&
               (length == strlen(expected[seen]))   &&
               (strcmp(text, expected[seen]) == 0);
        ++seen;
    }

    source.release(source.user);

    d_internal_check(seen == 4u, "the walk yields the four files");
    d_internal_check(same, "each by its path, in order by name");

    return;
}

/*
main
  Runs every group and says how it went: the last line is what a script
reads.
*/
int
main(void)
{
    d_internal_kinds();
    d_internal_names();
    d_internal_full_paths();
    d_internal_normalized();
    d_internal_walk();

    if (failures != 0)
    {
        (void)printf("%d check(s) failed\n", failures);

        return 1;
    }

    (void)printf("all checks passed\n");

    return 0;
}
