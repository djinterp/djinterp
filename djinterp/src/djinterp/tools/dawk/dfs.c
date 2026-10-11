/*******************************************************************************
* djinterp [tools]                                                         dfs.c
*
* What the tools ask of a file system that ISO C cannot answer.
*   Each question is answered twice, once for Windows and once for POSIX, and
* the system chooses between whole functions: no body here is split by a
* conditional. What the two share -- the list of names, and its release -- is
* written once, above them.
*   The Windows half asks the system itself (kernel32's narrow interface) and
* not its C library, whose answers differ between the libraries a Windows
* compiler may come with.
*
* path:      /src/djinterp/tools/dawk/dfs.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.10
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dfs.h"  // corresponding header
// std
#include <stdlib.h>  // malloc, realloc, free, realpath
#include <string.h>  // strcmp, strlen, memcpy
// djinterp
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_IS_OS_WINDOWS
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    // windows
    #include <windows.h>  // FindFirstFileA, GetFileAttributesA
#else
    // posix
    #include <dirent.h>    // DIR, opendir, readdir, closedir
    #include <sys/stat.h>  // struct stat, stat, S_ISDIR
#endif


// d_internal_names
//   struct: the names read so far, in an array that grows by doubling.
struct d_internal_names
{
    char** items;
    size_t count;
    size_t capacity;
};

/*
d_internal_names_add
  Copies one name onto the list, unless it is one of the two every directory
holds for itself and its parent: a walk that took those would never end.
*/
static bool
d_internal_names_add(
    struct d_internal_names* _names,
    const char*              _name
)
{
    // the two navigation entries are not contents
    if ( (strcmp(_name, ".")  == 0) ||
         (strcmp(_name, "..") == 0) )
    {
        return true;
    }

    if (_names->count == _names->capacity)
    {
        const size_t grown_capacity = (_names->capacity == 0u)
                                    ? 16u
                                    : (_names->capacity * 2u);

        char** const grown = realloc(_names->items,
                                     grown_capacity * sizeof(char*));

        // the list as it stands is still the caller's to release
        if (!grown)
        {
            return false;
        }

        _names->items    = grown;
        _names->capacity = grown_capacity;
    }

    const size_t length = strlen(_name);

    char* const copy = malloc(length + 1u);

    if (!copy)
    {
        return false;
    }

    memcpy(copy, _name, length + 1u);

    _names->items[_names->count] = copy;
    ++_names->count;

    return true;
}

/*
d_fs_names_free
  Releases each name and then the array, which is the order they were made in
reversed.
*/
void
d_fs_names_free(
    char** _names,
    size_t _count
)
{
    if (_names)
    {
        for (size_t at = 0u; at < _count; ++at)
        {
            free(_names[at]);
        }

        free(_names);
    }

    return;
}

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)

/*
d_fs_kind_of
  Windows: the attributes of the path, which the system gives for a path with
a separator at its end and for one with `/` in it alike.
*/
enum d_fs_kind
d_fs_kind_of(
    const char* _path
)
{
    // parameter validation first
    if (!_path)
    {
        return D_FS_KIND_NONE;
    }

    const DWORD attributes = GetFileAttributesA(_path);

    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return D_FS_KIND_NONE;
    }

    return ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0u)
           ? D_FS_KIND_DIRECTORY
           : D_FS_KIND_FILE;
}

/*
d_internal_search_pattern
  Windows: the pattern that finds every entry of a directory: its path, a
separator, and `*`. The separator is added only where the directory's own
spelling does not end in one. The caller has seen that the path is not empty.
*/
static char*
d_internal_search_pattern(
    const char* _directory
)
{
    const size_t length    = strlen(_directory);
    const char   last      = _directory[length - 1u];
    const bool   separated = ( (last == '/') || (last == '\\') );

    char* const pattern = malloc(length + 3u);

    if (!pattern)
    {
        return NULL;
    }

    memcpy(pattern, _directory, length);
    memcpy(pattern + length, separated ? "*" : "/*", separated ? 2u : 3u);

    return pattern;
}

/*
d_fs_names
  Windows: a search, whose first entry comes with the search itself and the
rest one call at a time. A drive's root may hold nothing at all, not even
`.`, and the search then fails with "file not found": that is a directory
read, holding nothing.
*/
bool
d_fs_names(
    const char* _directory,
    char***     _names,
    size_t*     _count
)
{
    // parameter validation first
    if ( (!_directory)             ||
         (_directory[0] == '\0')   ||
         (!_names)                 ||
         (!_count) )
    {
        return false;
    }

    *_names = NULL;
    *_count = 0u;

    char* const pattern = d_internal_search_pattern(_directory);

    if (!pattern)
    {
        return false;
    }

    WIN32_FIND_DATAA found  = { 0 };
    const HANDLE     search = FindFirstFileA(pattern, &found);

    free(pattern);

    if (search == INVALID_HANDLE_VALUE)
    {
        return (GetLastError() == ERROR_FILE_NOT_FOUND);
    }

    struct d_internal_names names = { NULL, 0u, 0u };
    bool                    ok    = true;

    // the first entry came with the search itself
    do
    {
        ok = d_internal_names_add(&names, found.cFileName);
    }
    while ( (ok) && (FindNextFileA(search, &found) != 0) );

    (void)FindClose(search);

    if (!ok)
    {
        d_fs_names_free(names.items, names.count);

        return false;
    }

    *_names = names.items;
    *_count = names.count;

    return true;
}

/*
d_fs_full_path
  Windows: asked twice, once for the length and once for the name, since the
system writes into a buffer the caller sizes. The system makes the name from
the spelling and the current directory, and does not look the path up.
*/
char*
d_fs_full_path(
    const char* _path
)
{
    // parameter validation first
    if ( (!_path) ||
         (_path[0] == '\0') )
    {
        return NULL;
    }

    // with no buffer, the answer is the size one needs, terminator included
    const DWORD needed = GetFullPathNameA(_path, 0u, NULL, NULL);

    if (needed == 0u)
    {
        return NULL;
    }

    char* const full = malloc((size_t)needed);

    if (!full)
    {
        return NULL;
    }

    // with one, it is the length written, terminator excluded
    const DWORD written = GetFullPathNameA(_path, needed, full, NULL);

    if ( (written == 0u) ||
         (written >= needed) )
    {
        free(full);

        return NULL;
    }

    return full;
}

/*
d_fs_path_normalize
  Windows: `\` and `/` both separate, and the tools split on `/` alone, so
every `\` is rewritten. No name on Windows can hold either.
*/
void
d_fs_path_normalize(
    char* _path
)
{
    if (_path)
    {
        for (char* at = _path; *at != '\0'; ++at)
        {
            if (*at == '\\')
            {
                *at = '/';
            }
        }
    }

    return;
}

#else

/*
d_fs_kind_of
  POSIX: stat, which follows a link to what it names and fails on one that
leads nowhere.
*/
enum d_fs_kind
d_fs_kind_of(
    const char* _path
)
{
    // parameter validation first
    if (!_path)
    {
        return D_FS_KIND_NONE;
    }

    struct stat information = { 0 };

    if (stat(_path, &information) != 0)
    {
        return D_FS_KIND_NONE;
    }

    return S_ISDIR(information.st_mode) ? D_FS_KIND_DIRECTORY
                                        : D_FS_KIND_FILE;
}

/*
d_fs_names
  POSIX: readdir until it has no more. A name is copied as it is read, since
the entry it is read from is the stream's and is overwritten by the next.
*/
bool
d_fs_names(
    const char* _directory,
    char***     _names,
    size_t*     _count
)
{
    // parameter validation first
    if ( (!_directory) ||
         (!_names)     ||
         (!_count) )
    {
        return false;
    }

    *_names = NULL;
    *_count = 0u;

    DIR* const stream = opendir(_directory);

    if (!stream)
    {
        return false;
    }

    struct d_internal_names names = { NULL, 0u, 0u };
    bool                    ok    = true;

    for (const struct dirent* entry = readdir(stream);
         (ok) && (entry);
         entry = readdir(stream))
    {
        ok = d_internal_names_add(&names, entry->d_name);
    }

    (void)closedir(stream);

    if (!ok)
    {
        d_fs_names_free(names.items, names.count);

        return false;
    }

    *_names = names.items;
    *_count = names.count;

    return true;
}

/*
d_fs_full_path
  POSIX: realpath with no buffer, which allocates what the name needs, so no
limit on a path's length is assumed here.
*/
char*
d_fs_full_path(
    const char* _path
)
{
    // parameter validation first
    if (!_path)
    {
        return NULL;
    }

    return realpath(_path, NULL);
}

/*
d_fs_path_normalize
  POSIX: nothing to do. `/` is the one separator, and `\` is a character a
name may hold.
*/
void
d_fs_path_normalize(
    char* _path
)
{
    (void)_path;

    return;
}

#endif  // D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
