/*******************************************************************************
* djinterp [tools]                                                  dhost_msvs.c
*
* Definitions for the declarations in dhost_msvs.h.
*   The two parts are made, installed and freed with the umbrella; the
* functions of its own are a generator's tools. A glob walks the pattern a
* part at a time: a part with no wildcard is a name to look for, and one
* with a wildcard is matched against every name the directory holds, as
* CMake's file(GLOB) does, and the paths found are sorted as CMake sorts
* them, byte by byte.
*
* path:      /src/djinterp/tools/dawk/ext/msvs/dhost_msvs.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dhost_msvs.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free, qsort
#include <string.h>  // memcpy, memset, strcmp, strlen, strpbrk
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"                     // d_fs_names
#include "../../../../../../inc/djinterp/tools/dawk/dhost.h"                   // d_awk_host_function
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dmsvs.h"          // d_msvs_guid
#include "../../../../../../inc/djinterp/tools/dawk/dvalue.h"                  // d_awk_value_text
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dhost_sln.h"      // d_host_sln
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dhost_vcxproj.h"  // d_host_vcxproj


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Extension
//------------------------------------------------------------------------------
// 1.1.1
// d_host_msvs
//   struct: the two parts.
struct d_host_msvs
{
    struct d_host_sln*     sln;
    struct d_host_vcxproj* vcxproj;
};

// 1.1.2
// d_internal_found
//   struct: the paths a glob has found, in an array that grows.
struct d_internal_found
{
    char** paths;
    size_t count;
    size_t capacity;
    bool   failed;
};

// 1.1.3
// d_internal_which
//   enum: what a glob gives.
enum d_internal_which
{
    D_INTERNAL_FILES = 0,
    D_INTERNAL_DIRECTORIES,
    D_INTERNAL_ALL
};


//==============================================================================
// 2.  GLOBS
//==============================================================================


/*
d_internal_match
  Whether a name matches a pattern part: `*` any run, `?` any byte, [...] a
set or range, `[!...]` its complement, anything else itself.
*/
static bool
d_internal_match(
    const char* _pattern,
    const char* _name
)
{
    // the end of the pattern matches only the end of the name
    if (*_pattern == '\0')
    {
        return (*_name == '\0');
    }

    // `*` takes any run of bytes
    if (*_pattern == '*')
    {
        // any run, the shortest first
        for (const char* at = _name; ; ++at)
        {
            if (d_internal_match(_pattern + 1, at))
            {
                return true;
            }

            if (*at == '\0')
            {
                return false;
            }
        }
    }

    // nothing is left of the name for a byte to match
    if (*_name == '\0')
    {
        return false;
    }

    // a set: its members, its ranges, or its complement
    if (*_pattern == '[')
    {
        const char* at      = _pattern + 1;
        const bool  negated = (*at == '!') || (*at == '^');
        bool        matched = false;

        at += (negated) ? 1 : 0;

        // a set's members, the first `]` among them taken as one
        do
        {
            // a range, a to z, unless its dash is the set's last byte
            if ( (at[1] == '-') &&
                 (at[2] != ']') &&
                 (at[2] != '\0') )
            {
                matched = matched || ( (*_name >= at[0]) &&
                                       (*_name <= at[2]) );
                at     += 3;
            }
            else
            {
                matched = matched || (*_name == *at);
                at++;
            }
        }
        while ( (*at != ']') &&
                (*at != '\0') );

        // an unclosed set is the `[` itself
        if (*at == '\0')
        {
            return ( (*_name == '[') &&
                     (d_internal_match(_pattern + 1, _name + 1)) );
        }

        return ( (matched != negated) &&
                 (d_internal_match(at + 1, _name + 1)) );
    }

    return ( ( (*_pattern == '?') || (*_pattern == *_name) ) &&
             (d_internal_match(_pattern + 1, _name + 1)) );
}


/*
d_internal_keep
  Adds a path found.
*/
static void
d_internal_keep(
    struct d_internal_found* _found,
    const char*              _path
)
{
    // a glob that has failed stays failed
    if (_found->failed)
    {
        return;
    }

    // the list of paths grows to hold one more
    if (_found->count >= _found->capacity)
    {
        const size_t wanted = (_found->capacity == 0u)
                            ? 64u : (_found->capacity * 2u);
        char** const grown  = realloc(_found->paths, wanted * sizeof(char*));

        if (!grown)
        {
            _found->failed = true;

            return;
        }

        _found->paths    = grown;
        _found->capacity = wanted;
    }

    const size_t length = strlen(_path);
    char* const  copy   = malloc(length + 1u);

    if (!copy)
    {
        _found->failed = true;

        return;
    }

    memcpy(copy, _path, length + 1u);
    _found->paths[_found->count++] = copy;

    return;
}


/*
d_internal_join
  A directory and a name, `/` between unless the directory is "" or ends
in one.
*/
static char*
d_internal_join(
    const char* _directory,
    const char* _name,
    size_t      _name_length
)
{
    const size_t directory = strlen(_directory);
    const bool   between   = (directory > 0u) &&
                             (_directory[directory - 1u] != '/');
    char* const  path      = malloc(directory + _name_length + 2u);

    // the path needs room of its own
    if (path)
    {
        (void)snprintf(path, directory + _name_length + 2u, "%s%s%.*s",
                       _directory, (between) ? "/" : "", (int)_name_length,
                       _name);
    }

    return path;
}


/*
d_internal_glob
  Walks the parts of a pattern from `_rest`, under `_directory`, keeping the
paths of the kind asked for that the whole pattern names.
*/
static void
d_internal_glob(
    const char*              _directory,
    const char*              _rest,
    enum d_internal_which    _which,
    struct d_internal_found* _found
)
{
    const char* const slash  = strchr(_rest, '/');
    const size_t      length = (slash) ? (size_t)(slash - _rest)
                                       : strlen(_rest);
    const char* const next   = (slash) ? (slash + 1) : NULL;
    char* const       part   = malloc(length + 1u);

    // the part needs room of its own
    if (!part)
    {
        _found->failed = true;

        return;
    }

    memcpy(part, _rest, length);
    part[length] = '\0';

    // a part with no wildcard is a name looked for; another, matched
    char** names = NULL;
    size_t count = 0;
    bool   wild  = (strpbrk(part, "*?[") != NULL);

    // a name looked for is the one name; a pattern is matched against all
    if (!wild)
    {
        names = malloc(sizeof(char*));
        count = (names) ? 1u : 0u;

        if (names)
        {
            names[0] = part;
        }
    }
    else if (!d_fs_names((_directory[0] != '\0') ? _directory : ".", &names,
                         &count))
    {
        names = NULL;
        count = 0;
    }

    // each name the directory holds, or the one looked for
    for (size_t at = 0; at < count; ++at)
    {
        // a hidden name is matched only by a part that begins with `.`
        if ( (wild) &&
             ( (!d_internal_match(part, names[at])) ||
               ( (names[at][0] == '.') && (part[0] != '.') ) ) )
        {
            continue;
        }

        char* const path = d_internal_join(_directory, names[at],
                                           strlen(names[at]));

        if (!path)
        {
            _found->failed = true;
            break;
        }

        const enum d_fs_kind kind = d_fs_kind_of(path);

        // the last part names what is kept; another, where to go on
        if ( (next) &&
             (kind == D_FS_KIND_DIRECTORY) )
        {
            d_internal_glob(path, next, _which, _found);
        }
        else if ( (!next) &&
                  ( ( (kind == D_FS_KIND_FILE) &&
                      (_which != D_INTERNAL_DIRECTORIES) ) ||
                    ( (kind == D_FS_KIND_DIRECTORY) &&
                      (_which != D_INTERNAL_FILES) ) ) )
        {
            d_internal_keep(_found, path);
        }

        free(path);
    }

    // the names a directory gave are its own to free
    if (wild)
    {
        d_fs_names_free(names, count);
    }
    else
    {
        free(names);
    }

    free(part);

    return;
}


/*
d_internal_compare_paths
  Byte order, for qsort over an array of paths.
*/
static int
d_internal_compare_paths(
    const void* _left,
    const void* _right
)
{
    return strcmp(*(char* const*)_left, *(char* const*)_right);
}


//==============================================================================
// 3.  FUNCTIONS
//==============================================================================


/*
msvs_guid(name)
  A version 5 GUID of the name.
*/
static bool
d_internal_msvs_guid(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    size_t            length = 0;
    const char* const name   = d_awk_value_text(_args[0], NULL, &length);
    char              guid[37];

    (void)_user;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_msvs_guid(name, length, guid);

    return d_awk_value_set_string(_out, guid, strlen(guid));
}


/*
msvs_relative(directory, path[, separator])
  The path from the directory.
*/
static bool
d_internal_msvs_relative(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const size_t length    = strlen(d_awk_value_text(_args[0], NULL, NULL));
    char* const  directory = malloc(length + 1u);
    char         separator = '\\';

    (void)_user;

    // the directory is copied, as the next argument's text is read
    if (!directory)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    memcpy(directory, d_awk_value_text(_args[0], NULL, NULL), length + 1u);

    // a separator given, or `\`
    if (_count > 2u)
    {
        const char* const given = d_awk_value_text(_args[2], NULL, NULL);

        separator = (given[0] != '\0') ? given[0] : '\\';
    }

    char* const relative = d_msvs_relative(directory,
                                           d_awk_value_text(_args[1], NULL,
                                                            NULL),
                                           separator);

    free(directory);

    // a path above where the two begin has no relative spelling
    if (!relative)
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const bool ok = d_awk_value_set_string(_out, relative, strlen(relative));

    free(relative);

    return ok;
}


/*
msvs_glob(pattern[, which])
  The paths, sorted, one to a line.
*/
static bool
d_internal_msvs_glob(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_internal_found found   = { NULL, 0u, 0u, false };
    const char* const       pattern = d_awk_value_text(_args[0], NULL, NULL);
    enum d_internal_which   which   = D_INTERNAL_FILES;
    char*                   copy    = malloc(strlen(pattern) + 1u);

    (void)_user;

    // the pattern is copied, as the next argument's text is read
    if (!copy)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    memcpy(copy, pattern, strlen(pattern) + 1u);

    // files, directories, or both
    if (_count > 1u)
    {
        const char* const given = d_awk_value_text(_args[1], NULL, NULL);

        which = (strcmp(given, "dirs") == 0) ? D_INTERNAL_DIRECTORIES
              : (strcmp(given, "all") == 0)  ? D_INTERNAL_ALL
              :                                D_INTERNAL_FILES;
    }

    // a pattern from the root begins its walk there
    d_internal_glob((copy[0] == '/') ? "/" : "",
                    (copy[0] == '/') ? (copy + 1) : copy, which, &found);
    free(copy);

    size_t length = 1u;

    // room for every path and a line ending after each
    for (size_t at = 0; at < found.count; ++at)
    {
        length += strlen(found.paths[at]) + 1u;
    }

    char* const joined = (found.failed) ? NULL : malloc(length);
    size_t      used   = 0;

    // sorted as CMake sorts a glob
    if (found.count > 0u)
    {
        qsort(found.paths, found.count, sizeof(char*),
              d_internal_compare_paths);
    }

    // each path, one to a line
    for (size_t at = 0; at < found.count; ++at)
    {
        const size_t span = strlen(found.paths[at]);

        // one to a line, with no line ending after the last
        if (joined)
        {
            memcpy(&joined[used], found.paths[at], span);
            used += span;

            if ((at + 1u) < found.count)
            {
                joined[used++] = '\n';
            }
        }

        free(found.paths[at]);
    }

    free(found.paths);

    if (!joined)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    const bool ok = d_awk_value_set_string(_out, joined, used);

    free(joined);

    return ok;
}


/*
msvs_exists(path), msvs_kind(path)
  What a path names, and what its name says.
*/
static bool
d_internal_msvs_exists(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const enum d_fs_kind kind = d_fs_kind_of(d_awk_value_text(_args[0], NULL,
                                                              NULL));

    (void)_user;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_awk_value_set_number(_out, (kind == D_FS_KIND_FILE)      ? 1.0
                               : (kind == D_FS_KIND_DIRECTORY) ? 2.0
                               :                                 0.0);

    return true;
}


static bool
d_internal_msvs_kind(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const names[] =
    {
        "other", "solution", "project", "filters", "user", "props"
    };
    const char* const kind = names[d_msvs_kind_of(d_awk_value_text(_args[0],
                                                                   NULL,
                                                                   NULL))];

    (void)_user;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_awk_value_set_string(_out, kind, strlen(kind));
}


//==============================================================================
// 4.  THE EXTENSION
//==============================================================================


// D_INTERNAL_MSVS_FUNCTIONS
//   constant: how many host functions the umbrella adds of its own.
#define D_INTERNAL_MSVS_FUNCTIONS 5u


/*
d_internal_functions
  The umbrella's own host functions, as a program calls them.
*/
static const struct d_awk_host_function*
d_internal_functions(void)
{
    static const struct d_awk_host_function functions[
        D_INTERNAL_MSVS_FUNCTIONS] =
    {
        { "msvs_guid",     1u, 1u, d_internal_msvs_guid     },
        { "msvs_relative", 2u, 3u, d_internal_msvs_relative },
        { "msvs_glob",     1u, 2u, d_internal_msvs_glob     },
        { "msvs_exists",   1u, 1u, d_internal_msvs_exists   },
        { "msvs_kind",     1u, 1u, d_internal_msvs_kind     }
    };

    return functions;
}


/*
d_host_msvs_new
  Both parts, or nothing.
*/
struct d_host_msvs*
d_host_msvs_new(void)
{
    struct d_host_msvs* const host = malloc(sizeof(*host));

    // an umbrella that cannot be made is NULL
    if (!host)
    {
        return NULL;
    }

    host->sln     = d_host_sln_new();
    host->vcxproj = d_host_vcxproj_new();

    // and so is one missing either part
    if ( (!host->sln) ||
         (!host->vcxproj) )
    {
        d_host_msvs_free(host);

        return NULL;
    }

    return host;
}


/*
d_host_msvs_free
  Both parts, and the umbrella.
*/
void
d_host_msvs_free(
    struct d_host_msvs* _host
)
{
    // freeing nothing is allowed
    if (!_host)
    {
        return;
    }

    d_host_sln_free(_host->sln);
    d_host_vcxproj_free(_host->vcxproj);
    free(_host);

    return;
}


/*
d_host_msvs_install
  Both parts' functions, then the umbrella's own.
*/
bool
d_host_msvs_install(
    struct d_host_msvs*  _host,
    struct d_awk_interp* _interp
)
{
    // parameter validation first
    if ( (!_host) ||
         (!_interp) )
    {
        return false;
    }

    return ( (d_host_sln_install(_host->sln, _interp)) &&
             (d_host_vcxproj_install(_host->vcxproj, _interp)) &&
             (d_awk_interp_add_functions(_interp, d_internal_functions(),
                                         D_INTERNAL_MSVS_FUNCTIONS,
                                         _host)) );
}
