/*******************************************************************************
* djinterp [tools]                                                   dhost_sln.c
*
* Definitions for the declarations in dhost_sln.h.
*   Each solution open is a dmsvs document. A .slnx is changed through its
* elements: a <Project> added in order beneath its <Folder> or the
* <Solution>, a <BuildDependency> beneath a <Project>, a <BuildType> or a
* <Platform> beneath <Configurations>. A .sln is changed through its lines,
* each change a splice of whole lines: a project's lines before Global; a
* folder's the same, nested by a line of NestedProjects; a dependency's line
* in its project's ProjectDependencies; a configuration's in
* SolutionConfigurationPlatforms, with each project's two lines in
* ProjectConfigurationPlatforms. A section a change needs is made where
* Visual Studio puts it. Every splice is read again before the next is made,
* so each offset is taken from the reading of the text as it then is.
*
* path:      /src/djinterp/tools/dawk/ext/msvs/dhost_sln.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dhost_sln.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free, qsort
#include <string.h>  // memcpy, memset, strcmp, strlen, strchr, strrchr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dhost.h"               // d_awk_host_function
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dmsvs.h"      // d_msvs_guid
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dmsvs_doc.h"  // d_msvs_doc
#include "../../../../../../inc/djinterp/tools/dawk/dsplice.h"             // d_text_read
#include "../../../../../../inc/djinterp/tools/dawk/dvalue.h"              // d_awk_value_text


// D_INTERNAL_FOLDER_TYPE
//   constant: the project type a .sln gives a solution folder.
#define D_INTERNAL_FOLDER_TYPE "2150E333-8FDC-42A3-9474-1A3956D46DE8"

// D_INTERNAL_PATH
//   constant: the most bytes a path, a folder or a line is given here.
#define D_INTERNAL_PATH 1024u


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Extension
//------------------------------------------------------------------------------
// 1.1.1
// d_host_sln
//   struct: the solutions open, each NULL once closed, and the last
//   failure.
struct d_host_sln
{
    struct d_msvs_doc** solutions;
    size_t              count;
    size_t              capacity;
    char                error[512];
};

// 1.1.2
// d_internal_span
//   struct: a span of a .sln's lines to remove.
struct d_internal_span
{
    size_t start;
    size_t end;
};


//==============================================================================
// 2.  HELPERS
//==============================================================================


/*
d_internal_text, d_internal_number
  An argument's text, and an argument as a number from 1: a value below 1
is 0, which names nothing.
*/
static const char*
d_internal_text(
    struct d_awk_value* _value
)
{
    return d_awk_value_text(_value, NULL, NULL);
}


static size_t
d_internal_number(
    struct d_awk_value* _value
)
{
    const double number = d_awk_value_number(_value);

    // NaN, and anything below 1, names nothing
    if (!(number >= 1.0))
    {
        return 0u;
    }

    return (number < 4294967296.0) ? (size_t)number : 0u;
}


/*
d_internal_copy_arg
  An argument's text into a buffer of the caller's, cut to fit.
*/
static void
d_internal_copy_arg(
    struct d_awk_value* _value,
    char*               _out,
    size_t              _capacity
)
{
    (void)snprintf(_out, _capacity, "%s", d_internal_text(_value));

    return;
}


/*
d_internal_fail
  Gives -1 for a failure the program can recover from, sln_error() saying
why.
*/
static bool
d_internal_fail(
    struct d_host_sln*  _host,
    struct d_awk_value* _out,
    const char*         _why
)
{
    (void)snprintf(_host->error, sizeof(_host->error), "%s",
                   ( (_why) && (_why[0] != '\0') ) ? _why : "failed");
    d_awk_value_set_number(_out, -1.0);

    return true;
}


/*
d_internal_solution
  The solution a number names, or NULL.
*/
static struct d_msvs_doc*
d_internal_solution(
    const struct d_host_sln* _host,
    struct d_awk_value*      _value
)
{
    const size_t number = d_internal_number(_value);

    // a number never given, or of a solution closed, names nothing
    if ( (number == 0u) ||
         (number > _host->count) )
    {
        return NULL;
    }

    return _host->solutions[number - 1u];
}


/*
d_internal_folder_of
  A folder as the records write one, "/tools/dawk/", from any spelling of
it: `\` or `/` between its names, with or without one at either end; ""
for the top.
*/
static void
d_internal_folder_of(
    const char* _folder,
    char*       _out,
    size_t      _capacity
)
{
    size_t used = 0;
    size_t at   = 0;

    _out[0] = '\0';

    // each name of the folder, however it is separated
    while ( (_folder[at] != '\0') &&
            ((used + 3u) < _capacity) )
    {
        // each name, a `/` before it
        while ( (_folder[at] == '/') ||
                (_folder[at] == '\\') )
        {
            at++;
        }

        if (_folder[at] == '\0')
        {
            break;
        }

        _out[used++] = '/';

        // the name, up to the next separator
        while ( (_folder[at] != '\0') &&
                (_folder[at] != '/') &&
                (_folder[at] != '\\') &&
                ((used + 2u) < _capacity) )
        {
            _out[used++] = _folder[at++];
        }
    }

    // a folder ends in `/`; the top is ""
    if (used > 0u)
    {
        _out[used++] = '/';
    }

    _out[used] = '\0';

    return;
}


/*
d_internal_project_at
  The index of the project with a path, or D_MSVS_NONE.
*/
static size_t
d_internal_project_at(
    const struct d_msvs* _records,
    const char*          _path
)
{
    // the first project whose path is the one asked for
    for (size_t at = 0; at < _records->project_count; ++at)
    {
        // paths compare as Visual Studio compares them
        if (d_msvs_compare_paths(_records->projects[at].path, _path) == 0)
        {
            return at;
        }
    }

    return D_MSVS_NONE;
}


/*
d_internal_folder_at
  The index of the folder with a path, as the records write one, or
D_MSVS_NONE.
*/
static size_t
d_internal_folder_at(
    const struct d_msvs* _records,
    const char*          _folder
)
{
    // the first folder whose path is the one asked for
    for (size_t at = 0; at < _records->folder_count; ++at)
    {
        // folders compare as paths do
        if (d_msvs_compare_paths(_records->folders[at].path, _folder) == 0)
        {
            return at;
        }
    }

    return D_MSVS_NONE;
}


/*
d_internal_upper
  A GUID as a .sln writes one: upper case, in braces.
*/
static void
d_internal_upper(
    const char* _guid,
    char*       _out,
    size_t      _capacity
)
{
    (void)snprintf(_out, _capacity, "{%s}", _guid);

    // the letters raised, the digits and dashes as they are
    for (char* at = _out; *at != '\0'; ++at)
    {
        // only the letters change
        if ( (*at >= 'a') &&
             (*at <= 'z') )
        {
            *at = (char)(*at - 'a' + 'A');
        }
    }

    return;
}


/*
d_internal_eol
  The line ending the solution uses.
*/
static const char*
d_internal_eol(
    const struct d_msvs_doc* _doc
)
{
    return (d_msvs_doc_records(_doc)->crlf) ? "\r\n" : "\n";
}


/*
d_internal_line_start, d_internal_line_end
  Where the line holding a byte begins, and where the next begins.
*/
static size_t
d_internal_line_start(
    const struct d_msvs* _records,
    size_t               _at
)
{
    size_t at = _at;

    // back to the line ending before it
    while ( (at > 0u) &&
            (_records->text[at - 1u] != '\n') )
    {
        at--;
    }

    return at;
}


static size_t
d_internal_line_end(
    const struct d_msvs* _records,
    size_t               _at
)
{
    size_t at = _at;

    // on to the line ending after it
    while ( (at < _records->length) &&
            (_records->text[at] != '\n') )
    {
        at++;
    }

    return (at < _records->length) ? (at + 1u) : at;
}


/*
d_internal_trimmed
  A line's bytes from `_start` to `_end`, without the space around them.
*/
static void
d_internal_trimmed(
    const struct d_msvs* _records,
    size_t               _start,
    size_t               _end,
    size_t*              _out_start,
    size_t*              _out_end
)
{
    size_t from = _start;
    size_t to   = _end;

    while ( (from < to) &&
            ( (_records->text[from] == ' ') ||
              (_records->text[from] == '\t') ) )
    {
        from++;
    }

    while ( (to > from) &&
            ( (_records->text[to - 1u] == ' ')  ||
              (_records->text[to - 1u] == '\t') ||
              (_records->text[to - 1u] == '\r') ||
              (_records->text[to - 1u] == '\n') ) )
    {
        to--;
    }

    *_out_start = from;
    *_out_end   = to;

    return;
}


/*
d_internal_left_is
  Whether a line's text before its `=`, trimmed, is a text, compared without
case: a GUID, or a configuration.
*/
static bool
d_internal_left_is(
    const struct d_msvs* _records,
    size_t               _line,
    size_t               _end,
    const char*          _left
)
{
    size_t from = 0;
    size_t to   = 0;

    d_internal_trimmed(_records, _line, _end, &from, &to);

    const char* const equals = memchr(&_records->text[from], '=', to - from);

    // a line with no `=` has no left side
    if (!equals)
    {
        return false;
    }

    size_t left_end = (size_t)(equals - _records->text);

    // back over the space before the `=`
    while ( (left_end > from) &&
            ( (_records->text[left_end - 1u] == ' ') ||
              (_records->text[left_end - 1u] == '\t') ) )
    {
        left_end--;
    }

    char   left[D_INTERNAL_PATH];
    size_t span = left_end - from;

    span = (span < sizeof(left)) ? span : (sizeof(left) - 1u);
    memcpy(left, &_records->text[from], span);
    left[span] = '\0';

    return (d_msvs_compare_paths(left, _left) == 0);
}


/*
d_internal_type_of
  The project type a .sln names a project by, from its file's ending.
*/
static const char*
d_internal_type_of(
    const char* _path
)
{
    // d_internal_kind
    //   struct: an ending, and the type it marks.
    struct d_internal_kind
    {
        const char* ending;
        const char* type;
    };

    static const struct d_internal_kind kinds[] =
    {
        { ".csproj", "FAE04EC0-301F-11D3-BF4B-00C04F79EFBC" },
        { ".vbproj", "F184B08F-C81C-45F6-A57F-5ABD9991F28F" },
        { ".fsproj", "F2A71F9B-5D33-465A-A702-920D77279786" }
    };
    const size_t length = strlen(_path);

    // the endings of .NET's languages, each its own type
    for (size_t at = 0; at < (sizeof(kinds) / sizeof(kinds[0])); ++at)
    {
        const size_t ending = strlen(kinds[at].ending);

        // a .NET project is its language's type
        if ( (length > ending) &&
             (d_msvs_compare_paths(&_path[length - ending],
                                   kinds[at].ending) == 0) )
        {
            return kinds[at].type;
        }
    }

    // and anything else Visual C++'s
    return "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942";
}


/*
d_internal_is_cpp
  Whether a project is one that names x86 Win32: a C++ project's.
*/
static bool
d_internal_is_cpp(
    const char* _path
)
{
    const size_t length = strlen(_path);

    return ( ( (length > 8u) &&
               (d_msvs_compare_paths(&_path[length - 8u], ".vcxproj")
                == 0) ) ||
             ( (length > 9u) &&
               (d_msvs_compare_paths(&_path[length - 9u], ".vcxitems")
                == 0) ) );
}


/*
d_internal_project_id
  The id a new project of a .sln is given: the one asked for; else its
file's ProjectGuid, read from beside the solution; else one made from its
path.  Lower case, without braces.
*/
static void
d_internal_project_id(
    const struct d_msvs_doc* _doc,
    const char*              _path,
    const char*              _asked,
    char*                    _out
)
{
    // an id asked for, in braces or not
    if ( (_asked) &&
         (_asked[0] != '\0') )
    {
        const size_t length = strlen(_asked);
        const bool   braced = (length > 2u) && (_asked[0] == '{') &&
                              (_asked[length - 1u] == '}');

        (void)snprintf(_out, 37u, "%.*s", (int)(braced ? (length - 2u)
                                                       : length),
                       braced ? (_asked + 1) : _asked);

        for (char* at = _out; *at != '\0'; ++at)
        {
            *at = ( (*at >= 'A') && (*at <= 'Z') ) ? (char)(*at - 'A' + 'a')
                                                   : *at;
        }

        return;
    }

    const char* const solution = d_msvs_doc_path(_doc);
    const char* const slash    = strrchr(solution, '/');
    const char* const back     = strrchr(solution, '\\');
    const char* const parts    = ( (slash) && ( (!back) || (slash > back) ) )
                               ? slash : back;
    char              file[D_INTERNAL_PATH];

    (void)snprintf(file, sizeof(file), "%.*s%s",
                   (int)((parts) ? (size_t)(parts - solution + 1) : 0u),
                   solution, _path);

    // the solution's separators, as the system takes them
    for (char* at = file; *at != '\0'; ++at)
    {
        *at = (*at == '\\') ? '/' : *at;
    }

    size_t      length = 0;
    char* const text   = d_text_read(file, &length);
    bool        found  = false;

    // the project's own GUID, when its file can be read
    if (text)
    {
        struct d_msvs records;

        // the project's own GUID, from the first ProjectGuid it has
        if (d_msvs_read(&records, text, length, D_MSVS_PROJECT, NULL, 0u))
        {
            for (size_t at = 0; (!found) && (at < records.element_count);
                 ++at)
            {
                const struct d_msvs_element* const element =
                    &records.elements[at];
                char guid[64];

                if ( (!d_msvs_is(&records, element, "ProjectGuid")) ||
                     (element->child_count > 0u) ||
                     ((element->content_end - element->content_start)
                      >= sizeof(guid)) )
                {
                    continue;
                }

                (void)d_msvs_text(&records, element, guid);
                d_internal_project_id(_doc, _path, guid, _out);
                found = (strlen(_out) == 36u);
            }
        }

        d_msvs_release(&records);
        free(text);
    }

    // else one made from its path, the same each time
    if (!found)
    {
        char seed[D_INTERNAL_PATH + 16u];

        (void)snprintf(seed, sizeof(seed), "project:%s", _path);

        // lower case, `/` between the parts, so one path makes one id
        for (char* at = seed; *at != '\0'; ++at)
        {
            *at = (*at == '\\') ? '/'
                : ( (*at >= 'A') && (*at <= 'Z') ) ? (char)(*at - 'A' + 'a')
                : *at;
        }

        d_msvs_guid(seed, strlen(seed), _out);
    }

    return;
}


/*
d_internal_stem
  A path's file name without its extension.
*/
static void
d_internal_stem(
    const char* _path,
    char*       _out,
    size_t      _capacity
)
{
    const char* name = _path;

    // the name after the last separator
    for (const char* at = _path; *at != '\0'; ++at)
    {
        // the name follows the last separator of either kind
        if ( (*at == '/') ||
             (*at == '\\') )
        {
            name = at + 1;
        }
    }

    const char* const dot = strrchr(name, '.');

    (void)snprintf(_out, _capacity, "%.*s",
                   (int)( (dot && (dot > name)) ? (size_t)(dot - name)
                                                : strlen(name) ),
                   name);

    return;
}


/*
d_internal_splice_text
  Splices a terminated text in at an offset.
*/
static bool
d_internal_splice_text(
    struct d_msvs_doc* _doc,
    size_t             _at,
    const char*        _text
)
{
    return d_msvs_doc_splice(_doc, _at, _at, _text, strlen(_text));
}


//==============================================================================
// 3.  .SLNX
//==============================================================================


/*
d_internal_slnx_place
  Where a child named `_name`, ordered by `_attribute`, goes beneath an
element: before the first such child whose attribute sorts after
`_value`; else after the last such child; else before the first child
named `_after_none` -- what such children stand before -- or last.
*/
static size_t
d_internal_slnx_place(
    const struct d_msvs* _records,
    size_t               _parent,
    const char*          _name,
    const char*          _attribute,
    const char*          _value,
    const char*          _after_none
)
{
    size_t last  = D_MSVS_NONE;
    size_t ahead = D_MSVS_NONE;

    // each child of the name, in order
    for (size_t at = _records->elements[_parent].first_child;
         at != D_MSVS_NONE; at = _records->elements[at].next_sibling)
    {
        const struct d_msvs_element* const element = &_records->elements[at];

        // what children of the name stand before, when there are none
        if ( (ahead == D_MSVS_NONE) &&
             (_after_none) &&
             (d_msvs_is(_records, element, _after_none)) )
        {
            ahead = at;
        }

        if (!d_msvs_is(_records, element, _name))
        {
            continue;
        }

        size_t start  = 0;
        size_t length = 0;
        char   value[D_INTERNAL_PATH];

        // a child without the attribute, or too long to order, is passed
        if ( (d_msvs_attribute(_records, element, _attribute, &start,
                               &length)) &&
             (length < sizeof(value)) )
        {
            (void)d_msvs_value(_records, start, length, value);

            if (d_msvs_compare_paths(value, _value) > 0)
            {
                return at;
            }
        }

        last = at;
    }

    return (last != D_MSVS_NONE) ? _records->elements[last].next_sibling
                                 : ahead;
}


/*
d_internal_slnx_named
  The child of an element named `_name` whose `_attribute` is `_value`, or
D_MSVS_NONE.
*/
static size_t
d_internal_slnx_named(
    const struct d_msvs* _records,
    size_t               _parent,
    const char*          _name,
    const char*          _attribute,
    const char*          _value
)
{
    // each child of the name, until one has the value
    for (size_t at = _records->elements[_parent].first_child;
         at != D_MSVS_NONE; at = _records->elements[at].next_sibling)
    {
        size_t start  = 0;
        size_t length = 0;
        char   value[D_INTERNAL_PATH];

        // a child of the name, the attribute the value
        if ( (d_msvs_is(_records, &_records->elements[at], _name)) &&
             (d_msvs_attribute(_records, &_records->elements[at], _attribute,
                               &start, &length)) &&
             (length < sizeof(value)) )
        {
            (void)d_msvs_value(_records, start, length, value);

            if (d_msvs_compare_paths(value, _value) == 0)
            {
                return at;
            }
        }
    }

    return D_MSVS_NONE;
}


/*
d_internal_slnx_add_folder
  A <Folder> for the folder and each holding it, in order among the
folders, ahead of the projects at the top.  The folder's element, or
D_MSVS_NONE.
*/
static size_t
d_internal_slnx_add_folder(
    struct d_msvs_doc* _doc,
    const char*        _folder
)
{
    const size_t length = strlen(_folder);
    size_t       folder = D_MSVS_NONE;
    char         level[D_INTERNAL_PATH];

    // the folder and each folder holding it, outermost first
    for (size_t end = 1u; end < length; ++end)
    {
        // each level ends at a `/`
        if (_folder[end] != '/')
        {
            continue;
        }

        (void)snprintf(level, sizeof(level), "%.*s", (int)(end + 1u),
                       _folder);

        const struct d_msvs* const records = d_msvs_doc_records(_doc);

        folder = d_internal_slnx_named(records, 0u, "Folder", "Name", level);

        // a folder there already is the one
        if (folder != D_MSVS_NONE)
        {
            continue;
        }

        const char* const               name[] = { "Name", level };
        const struct d_msvs_new_element made   = { "Folder", NULL, name, 1u };

        folder = d_msvs_doc_insert(_doc, 0u,
                                   d_internal_slnx_place(records, 0u,
                                                         "Folder", "Name",
                                                         level, "Project"),
                                   &made);

        // a folder that cannot be made holds nothing
        if (folder == D_MSVS_NONE)
        {
            return D_MSVS_NONE;
        }
    }

    return folder;
}


/*
d_internal_slnx_add_project
  A <Project> in order beneath its folder, or the solution.
*/
static bool
d_internal_slnx_add_project(
    struct d_msvs_doc* _doc,
    const char*        _path,
    const char*        _folder,
    const char*        _id
)
{
    const size_t parent = (_folder[0] != '\0')
                        ? d_internal_slnx_add_folder(_doc, _folder) : 0u;

    // a folder that cannot be made holds nothing
    if (parent == D_MSVS_NONE)
    {
        return false;
    }

    const char* const pairs[] = { "Path", _path, "Id", _id };
    const struct d_msvs_new_element made =
    {
        "Project", NULL, pairs, ( (_id) && (_id[0] != '\0') ) ? 2u : 1u
    };
    const struct d_msvs* const records = d_msvs_doc_records(_doc);

    return (d_msvs_doc_insert(_doc, parent,
                              d_internal_slnx_place(records, parent,
                                                    "Project", "Path", _path,
                                                    NULL),
                              &made) != D_MSVS_NONE);
}


/*
d_internal_slnx_remove_dependencies
  Removes every <BuildDependency> on a project, beneath any project.
*/
static bool
d_internal_slnx_remove_dependencies(
    struct d_msvs_doc* _doc,
    const char*        _path,
    size_t             _within
)
{
    bool ok = true;

    // until no dependency on the project is left
    for (;;)
    {
        const struct d_msvs* const records = d_msvs_doc_records(_doc);
        size_t                     found   = D_MSVS_NONE;

        // the first, within the one project or any
        for (size_t at = 0; (found == D_MSVS_NONE) &&
                            (at < records->project_count); ++at)
        {
            const size_t project = records->projects[at].element;

            // within one project, or any
            if ( (_within == D_MSVS_NONE) ||
                 (_within == at) )
            {
                found = d_internal_slnx_named(records, project,
                                              "BuildDependency", "Project",
                                              _path);
            }
        }

        // none left
        if (found == D_MSVS_NONE)
        {
            break;
        }

        ok = d_msvs_doc_remove(_doc, found);

        if (!ok)
        {
            break;
        }
    }

    return ok;
}


/*
d_internal_slnx_configurations
  The <Configurations> element, made first in the solution when there is
none, with the build types Visual Studio builds when none is named made
explicit, so that one named does not take their place.  D_MSVS_NONE when
it cannot be made.
*/
static size_t
d_internal_slnx_configurations(
    struct d_msvs_doc* _doc
)
{
    const struct d_msvs* records = d_msvs_doc_records(_doc);
    size_t               held    = d_msvs_child(records, 0u,
                                                "Configurations");

    // made, first
    if (held == D_MSVS_NONE)
    {
        const struct d_msvs_new_element made = { "Configurations", NULL,
                                                 NULL, 0u };

        return d_msvs_doc_insert(_doc, 0u, records->elements[0].first_child,
                                 &made);
    }

    // platforms alone build Debug and Release, which are written out
    if ( (d_msvs_child(records, held, "BuildType") == D_MSVS_NONE) &&
         (d_msvs_child(records, held, "Platform") != D_MSVS_NONE) )
    {
        static const char* const debug[]   = { "Name", "Debug" };
        static const char* const release[] = { "Name", "Release" };
        const struct d_msvs_new_element types[] =
        {
            { "BuildType", NULL, debug,   1u },
            { "BuildType", NULL, release, 1u }
        };

        // Debug, then Release, before the first platform
        for (size_t at = 0; at < 2u; ++at)
        {
            records = d_msvs_doc_records(_doc);

            if (d_msvs_doc_insert(_doc, held,
                                  d_msvs_child(records, held, "Platform"),
                                  &types[at]) == D_MSVS_NONE)
            {
                return D_MSVS_NONE;
            }
        }
    }

    return held;
}


/*
d_internal_slnx_add_config
  A <BuildType> and a <Platform>, each unless named already.
*/
static bool
d_internal_slnx_add_config(
    struct d_msvs_doc* _doc,
    const char*        _type,
    const char*        _platform
)
{
    const size_t held = d_internal_slnx_configurations(_doc);

    // the configurations must be there to be added to
    if (held == D_MSVS_NONE)
    {
        return false;
    }

    const char* const names[2]  = { "BuildType", "Platform" };
    const char* const values[2] = { _type, _platform };

    // the build type, then the platform
    for (size_t which = 0; which < 2u; ++which)
    {
        const struct d_msvs* const records = d_msvs_doc_records(_doc);

        // one named already needs nothing
        if (d_internal_slnx_named(records, held, names[which], "Name",
                                  values[which]) != D_MSVS_NONE)
        {
            continue;
        }

        size_t last = D_MSVS_NONE;

        // the last of its kind, which the new one follows
        for (size_t at = records->elements[held].first_child;
             at != D_MSVS_NONE; at = records->elements[at].next_sibling)
        {
            if (d_msvs_is(records, &records->elements[at], names[which]))
            {
                last = at;
            }
        }

        const char* const               pair[] = { "Name", values[which] };
        const struct d_msvs_new_element made   = { names[which], NULL, pair,
                                                   1u };
        const size_t before =
            (last != D_MSVS_NONE) ? records->elements[last].next_sibling
            : (which == 0u)       ? d_msvs_child(records, held, "Platform")
            :                       D_MSVS_NONE;

        if (d_msvs_doc_insert(_doc, held, before, &made) == D_MSVS_NONE)
        {
            return false;
        }
    }

    return true;
}


//==============================================================================
// 4.  .SLN
//==============================================================================


/*
d_internal_sln_section
  The index of a global section by its name, or D_MSVS_NONE.
*/
static size_t
d_internal_sln_section(
    const struct d_msvs* _records,
    const char*          _name
)
{
    // the section of that name
    for (size_t at = 0; at < _records->section_count; ++at)
    {
        if (strcmp(_records->sections[at].name, _name) == 0)
        {
            return at;
        }
    }

    return D_MSVS_NONE;
}


/*
d_internal_sln_global
  Global ... EndGlobal, made at the end of a .sln that has none.
*/
static bool
d_internal_sln_global(
    struct d_msvs_doc* _doc
)
{
    const struct d_msvs* const records = d_msvs_doc_records(_doc);
    const char* const          eol     = d_internal_eol(_doc);
    char                       text[64];

    // a Global there already is the one
    if (records->global_start != D_MSVS_NONE)
    {
        return true;
    }

    const bool ended = (records->length > 0u) &&
                       (records->text[records->length - 1u] == '\n');

    (void)snprintf(text, sizeof(text), "%sGlobal%sEndGlobal%s",
                   (ended) ? "" : eol, eol, eol);

    return d_internal_splice_text(_doc, records->length, text);
}


/*
d_internal_sln_ensure
  A global section, made where Visual Studio writes it, unless there:
SolutionConfigurationPlatforms first; ProjectConfigurationPlatforms after
it; NestedProjects before ExtensibilityGlobals, or last.
*/
static size_t
d_internal_sln_ensure(
    struct d_msvs_doc* _doc,
    const char*        _name
)
{
    // the section there is the one
    if (!d_internal_sln_global(_doc))
    {
        return D_MSVS_NONE;
    }

    const struct d_msvs* records = d_msvs_doc_records(_doc);
    const size_t         held    = d_internal_sln_section(records, _name);

    // a section there already is the one
    if (held != D_MSVS_NONE)
    {
        return held;
    }

    const bool   solution = (strcmp(_name,
                                    "SolutionConfigurationPlatforms") == 0);
    const bool   project  = (strcmp(_name,
                                    "ProjectConfigurationPlatforms") == 0);
    const size_t configs  = d_internal_sln_section(
                                records, "SolutionConfigurationPlatforms");
    const size_t extended = d_internal_sln_section(records,
                                                   "ExtensibilityGlobals");
    size_t       at       = records->global_close;

    // the configurations first, the projects' after the solution's, and
    // any other before ExtensibilityGlobals, or last
    if ( (solution) ||
         ( (project) && (configs == D_MSVS_NONE) ) )
    {
        at = d_internal_line_end(records, records->global_start);
    }
    else if (project)
    {
        at = records->sections[configs].end;
    }
    else if (extended != D_MSVS_NONE)
    {
        at = records->sections[extended].start;
    }

    const char* const eol = d_internal_eol(_doc);
    char              text[256];

    (void)snprintf(text, sizeof(text),
                   "\tGlobalSection(%s) = %s%s\tEndGlobalSection%s", _name,
                   (project) ? "postSolution" : "preSolution", eol, eol);

    // a section that cannot be made takes no line
    if (!d_internal_splice_text(_doc, at, text))
    {
        return D_MSVS_NONE;
    }

    return d_internal_sln_section(d_msvs_doc_records(_doc), _name);
}


/*
d_internal_sln_add_line
  A line, indented twice, at the end of a global section.
*/
static bool
d_internal_sln_add_line(
    struct d_msvs_doc* _doc,
    const char*        _section,
    const char*        _line
)
{
    const size_t section = d_internal_sln_ensure(_doc, _section);

    // the section must be there to take the line
    if (section == D_MSVS_NONE)
    {
        return false;
    }

    const struct d_msvs* const records = d_msvs_doc_records(_doc);
    char                       text[D_INTERNAL_PATH + 16u];

    (void)snprintf(text, sizeof(text), "\t\t%s%s", _line,
                   d_internal_eol(_doc));

    return d_internal_splice_text(_doc, records->sections[section].close,
                                  text);
}


/*
d_internal_sln_project_lines
  The lines of a project of the folder type, or of any other, before
Global.
*/
static bool
d_internal_sln_project_lines(
    struct d_msvs_doc* _doc,
    const char*        _type,
    const char*        _name,
    const char*        _path,
    const char*        _id
)
{
    // the projects stand before Global
    if (!d_internal_sln_global(_doc))
    {
        return false;
    }

    const struct d_msvs* const records = d_msvs_doc_records(_doc);
    const char* const          eol     = d_internal_eol(_doc);
    char                       type[48];
    char                       id[48];
    char                       text[(3u * D_INTERNAL_PATH) + 128u];

    d_internal_upper(_type, type, sizeof(type));
    d_internal_upper(_id, id, sizeof(id));
    (void)snprintf(text, sizeof(text),
                   "Project(\"%s\") = \"%s\", \"%s\", \"%s\"%sEndProject%s",
                   type, _name, _path, id, eol, eol);

    return d_internal_splice_text(_doc, records->global_start, text);
}


/*
d_internal_sln_nest
  Nests one thing in a folder: a line of NestedProjects.
*/
static bool
d_internal_sln_nest(
    struct d_msvs_doc* _doc,
    const char*        _child,
    const char*        _parent
)
{
    char child[48];
    char parent[48];
    char line[128];

    d_internal_upper(_child, child, sizeof(child));
    d_internal_upper(_parent, parent, sizeof(parent));
    (void)snprintf(line, sizeof(line), "%s = %s", child, parent);

    return d_internal_sln_add_line(_doc, "NestedProjects", line);
}


/*
d_internal_sln_add_folder
  A folder project for the folder and each holding it, each nested in the
one before; the folder's id, lower case, to `_out`, or "" for the top.
*/
static bool
d_internal_sln_add_folder(
    struct d_msvs_doc* _doc,
    const char*        _folder,
    char*              _out
)
{
    const size_t length = strlen(_folder);
    char         parent[37] = "";
    char         level[D_INTERNAL_PATH];

    _out[0] = '\0';

    // the folder and each folder holding it, outermost first
    for (size_t end = 1u; end < length; ++end)
    {
        // each level ends at a `/`
        if (_folder[end] != '/')
        {
            continue;
        }

        (void)snprintf(level, sizeof(level), "%.*s", (int)(end + 1u),
                       _folder);

        const struct d_msvs* const records = d_msvs_doc_records(_doc);
        const size_t               held    = d_internal_folder_at(records,
                                                                  level);

        // a folder there already is the one
        if (held != D_MSVS_NONE)
        {
            (void)snprintf(_out, 37u, "%s", records->folders[held].id);
            (void)snprintf(parent, sizeof(parent), "%s", _out);
            continue;
        }

        char seed[D_INTERNAL_PATH + 16u];
        char name[D_INTERNAL_PATH];

        (void)snprintf(seed, sizeof(seed), "folder:%s", level);

        // lower case, so one folder makes one id
        for (char* at = seed; *at != '\0'; ++at)
        {
            *at = ( (*at >= 'A') && (*at <= 'Z') ) ? (char)(*at - 'A' + 'a')
                                                   : *at;
        }

        d_msvs_guid(seed, strlen(seed), _out);

        // the folder's own name is the last of the path's
        level[strlen(level) - 1u] = '\0';

        const char* const own = strrchr(level, '/');

        (void)snprintf(name, sizeof(name), "%s", (own) ? (own + 1) : level);

        if ( (!d_internal_sln_project_lines(_doc, D_INTERNAL_FOLDER_TYPE,
                                            name, name, _out)) ||
             ( (parent[0] != '\0') &&
               (!d_internal_sln_nest(_doc, _out, parent)) ) )
        {
            return false;
        }

        (void)snprintf(parent, sizeof(parent), "%s", _out);
    }

    return true;
}


/*
d_internal_sln_platform
  The platform a project builds for a solution's: Win32 for a C++ project's
x86, and the solution's own otherwise.
*/
static const char*
d_internal_sln_platform(
    const char* _path,
    const char* _platform
)
{
    return ( (d_internal_is_cpp(_path)) &&
             (d_msvs_compare_paths(_platform, "x86") == 0) ) ? "Win32"
                                                             : _platform;
}


/*
d_internal_sln_build_lines
  A project's two lines for a configuration, in
ProjectConfigurationPlatforms: what it builds, and that it builds.
*/
static bool
d_internal_sln_build_lines(
    struct d_msvs_doc* _doc,
    const char*        _id,
    const char*        _path,
    const char*        _config
)
{
    const char* const bar = strchr(_config, '|');
    char              id[48];
    char              type[D_INTERNAL_PATH];
    char              line[(3u * D_INTERNAL_PATH) + 64u];

    // a configuration names a platform
    if (!bar)
    {
        return false;
    }

    d_internal_upper(_id, id, sizeof(id));
    (void)snprintf(type, sizeof(type), "%.*s", (int)(bar - _config),
                   _config);

    const char* const platform = d_internal_sln_platform(_path, bar + 1);

    (void)snprintf(line, sizeof(line), "%s.%s.ActiveCfg = %s|%s", id,
                   _config, type, platform);

    // the line saying what the project builds, then the one that it builds
    if (!d_internal_sln_add_line(_doc, "ProjectConfigurationPlatforms", line))
    {
        return false;
    }

    (void)snprintf(line, sizeof(line), "%s.%s.Build.0 = %s|%s", id, _config,
                   type, platform);

    return d_internal_sln_add_line(_doc, "ProjectConfigurationPlatforms",
                                   line);
}


/*
d_internal_lines_add_project
  A project's lines, its configurations', and its nesting.
*/
static bool
d_internal_lines_add_project(
    struct d_msvs_doc* _doc,
    const char*        _path,
    const char*        _folder,
    const char*        _id
)
{
    char folder[37] = "";
    char id[37];
    char name[D_INTERNAL_PATH];

    d_internal_project_id(_doc, _path, _id, id);
    d_internal_stem(_path, name, sizeof(name));

    // its folder first, then its lines
    if ( (_folder[0] != '\0') &&
         (!d_internal_sln_add_folder(_doc, _folder, folder)) )
    {
        return false;
    }

    // the project's lines, then its configurations and its nesting
    if (!d_internal_sln_project_lines(_doc, d_internal_type_of(_path), name,
                                      _path, id))
    {
        return false;
    }

    // a line for each configuration the solution builds
    for (size_t at = 0; at < d_msvs_doc_records(_doc)->configuration_count;
         ++at)
    {
        char config[D_INTERNAL_PATH];

        (void)snprintf(config, sizeof(config), "%s",
                       d_msvs_doc_records(_doc)->configurations[at]);

        if (!d_internal_sln_build_lines(_doc, id, _path, config))
        {
            return false;
        }
    }

    return ( (folder[0] == '\0') ||
             (d_internal_sln_nest(_doc, id, folder)) );
}


/*
d_internal_compare_spans
  Spans by where they start, the last first.
*/
static int
d_internal_compare_spans(
    const void* _left,
    const void* _right
)
{
    const struct d_internal_span* const left  = _left;
    const struct d_internal_span* const right = _right;

    return (left->start > right->start) ? -1
         : (left->start < right->start) ?  1
         :                                 0;
}


/*
d_internal_sln_remove_spans
  Removes spans of lines, the last first, so each offset is still good when
its turn comes.
*/
static bool
d_internal_sln_remove_spans(
    struct d_msvs_doc*      _doc,
    struct d_internal_span* _spans,
    size_t                  _count
)
{
    // with nothing to remove there is nothing to order
    if (_count == 0u)
    {
        return true;
    }

    qsort(_spans, _count, sizeof(*_spans), d_internal_compare_spans);

    // the last first, so each offset is still good when its turn comes
    for (size_t at = 0; at < _count; ++at)
    {
        if (!d_msvs_doc_splice(_doc, _spans[at].start, _spans[at].end, "",
                               0u))
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_sln_pick
  Adds to the spans each line from `_start` to `_close` -- of a section, or
of a project's dependencies -- whose left side is `_left`, or, for a
prefix, that begins with `_left` and a `.`: a project's configurations.
Gives how many spans there are then.
*/
static size_t
d_internal_sln_pick(
    const struct d_msvs*    _records,
    size_t                  _start,
    size_t                  _close,
    const char*             _left,
    bool                    _prefix,
    struct d_internal_span* _spans,
    size_t                  _count,
    size_t                  _capacity
)
{
    size_t count = _count;

    // each line of the span
    for (size_t line = _start; (line < _close) && (count < _capacity);
         line = d_internal_line_end(_records, line))
    {
        const size_t end    = d_internal_line_end(_records, line);
        size_t       from   = 0;
        size_t       to     = 0;
        const size_t length = strlen(_left);

        d_internal_trimmed(_records, line, end, &from, &to);

        // its left side the text, or, for a prefix, its start
        const bool picked =
            (_prefix)
            ? ( ((to - from) > length) &&
                (_records->text[from + length] == '.') &&
                (memcmp(&_records->text[from], _left, length) == 0) )
            : (d_internal_left_is(_records, line, end, _left));

        if (picked)
        {
            _spans[count].start = line;
            _spans[count].end   = end;
            count++;
        }
    }

    return count;
}


/*
d_internal_lines_tidy
  Removes what removals left empty, as Visual Studio would not write it: a
project's ProjectDependencies with no line, and NestedProjects with none.
*/
static bool
d_internal_lines_tidy(
    struct d_msvs_doc* _doc
)
{
    // until nothing empty is left
    for (;;)
    {
        const struct d_msvs* const records = d_msvs_doc_records(_doc);
        struct d_internal_span     span    = { 0u, 0u };
        const size_t               nests   = d_internal_sln_section(
                                                 records, "NestedProjects");

        // a section of dependencies with no line left in it
        for (size_t at = 0; (span.end == 0u) &&
                            (at < records->project_count); ++at)
        {
            const struct d_msvs_project* const project =
                &records->projects[at];

            // a section of dependencies with none left in it
            if ( (project->depends != D_MSVS_NONE) &&
                 (d_internal_line_end(records, project->depends)
                  == project->depends_close) )
            {
                span.start = project->depends;
                span.end   = d_internal_line_end(records,
                                                 project->depends_close);
            }
        }

        if ( (span.end == 0u) &&
             (nests != D_MSVS_NONE) &&
             (records->sections[nests].content
              == records->sections[nests].close) )
        {
            span.start = records->sections[nests].start;
            span.end   = records->sections[nests].end;
        }

        // nothing empty is left
        if (span.end == 0u)
        {
            return true;
        }

        if (!d_msvs_doc_splice(_doc, span.start, span.end, "", 0u))
        {
            return false;
        }
    }
}


/*
d_internal_lines_remove_project
  A project's lines, its configurations' lines, its nesting, and every
dependency on it.
*/
static bool
d_internal_lines_remove_project(
    struct d_msvs_doc* _doc,
    size_t             _project
)
{
    const struct d_msvs* const records  = d_msvs_doc_records(_doc);
    const size_t               capacity = (records->length / 2u) + 2u;
    struct d_internal_span*    spans    = malloc(capacity * sizeof(*spans));
    char                       id[48];

    // the spans need room of their own
    if (!spans)
    {
        return false;
    }

    d_internal_upper(records->projects[_project].id, id, sizeof(id));
    spans[0].start = records->projects[_project].start;
    spans[0].end   = records->projects[_project].end;

    size_t       count   = 1u;
    const size_t configs = d_internal_sln_section(
                               records, "ProjectConfigurationPlatforms");
    const size_t nests   = d_internal_sln_section(records, "NestedProjects");

    // its configurations, and its nesting
    if (configs != D_MSVS_NONE)
    {
        count = d_internal_sln_pick(records, records->sections[configs]
                                                 .content,
                                    records->sections[configs].close, id,
                                    true, spans, count, capacity);
    }

    // its nesting in a folder
    if (nests != D_MSVS_NONE)
    {
        count = d_internal_sln_pick(records, records->sections[nests]
                                                 .content,
                                    records->sections[nests].close, id,
                                    false, spans, count, capacity);
    }

    // and every project's dependency on it
    for (size_t at = 0; at < records->project_count; ++at)
    {
        const struct d_msvs_project* const project = &records->projects[at];

        // every other project's dependency on it
        if ( (at != _project) &&
             (project->depends != D_MSVS_NONE) )
        {
            count = d_internal_sln_pick(records,
                                        d_internal_line_end(
                                            records, project->depends),
                                        project->depends_close, id, false,
                                        spans, count, capacity);
        }
    }

    const bool ok = (d_internal_sln_remove_spans(_doc, spans, count)) &&
                    (d_internal_lines_tidy(_doc));

    free(spans);

    return ok;
}


/*
d_internal_sln_add_dependency
  A line in the project's ProjectDependencies, which is made before its
EndProject when it has none.
*/
static bool
d_internal_sln_add_dependency(
    struct d_msvs_doc* _doc,
    size_t             _project,
    size_t             _on
)
{
    const struct d_msvs* const         records = d_msvs_doc_records(_doc);
    const struct d_msvs_project* const project =
        &records->projects[_project];
    const char* const                  eol     = d_internal_eol(_doc);
    char                               on[48];
    char                               text[256];

    d_internal_upper(records->projects[_on].id, on, sizeof(on));

    // a section there takes the line, unless it has it
    if (project->depends != D_MSVS_NONE)
    {
        struct d_internal_span span;

        if (d_internal_sln_pick(records,
                                d_internal_line_end(records,
                                                    project->depends),
                                project->depends_close, on, false, &span, 0u,
                                1u) > 0u)
        {
            return true;
        }

        (void)snprintf(text, sizeof(text), "\t\t%s = %s%s", on, on, eol);

        return d_internal_splice_text(_doc, project->depends_close, text);
    }

    (void)snprintf(text, sizeof(text),
                   "\tProjectSection(ProjectDependencies) = postProject%s"
                   "\t\t%s = %s%s\tEndProjectSection%s", eol, on, on, eol,
                   eol);

    return d_internal_splice_text(_doc,
                                  d_internal_line_start(records,
                                                        project->end - 1u),
                                  text);
}


/*
d_internal_sln_config_of
  The configuration a ProjectConfigurationPlatforms line names, between
"}." and the `.` after its `|`, into `_out`; false for a line naming none.
*/
static bool
d_internal_sln_config_of(
    const struct d_msvs* _records,
    size_t               _line,
    size_t               _end,
    char*                _out,
    size_t               _capacity
)
{
    size_t from = 0;
    size_t to   = 0;

    d_internal_trimmed(_records, _line, _end, &from, &to);

    const char* const text  = &_records->text[from];
    const size_t      span  = to - from;
    const char* const close = memchr(text, '}', span);

    // {GUID}.Configuration|Platform.Kind = ...
    if ( (!close) ||
         ((size_t)(close - text + 2) >= span) ||
         (close[1] != '.') )
    {
        return false;
    }

    const char* const begin = close + 2;
    const char* const bar   = memchr(begin, '|',
                                     span - (size_t)(begin - text));
    const char* const dot   = (bar) ? memchr(bar, '.',
                                             span - (size_t)(bar - text))
                                    : NULL;

    if (!dot)
    {
        return false;
    }

    (void)snprintf(_out, _capacity, "%.*s", (int)(dot - begin), begin);

    return true;
}


/*
d_internal_sln_remove_config
  The lines of each configuration a pattern matches: the solution's, and
every project's.  How many of the solution's went to `_removed`.
*/
static bool
d_internal_sln_remove_config(
    struct d_msvs_doc* _doc,
    const char*        _pattern,
    size_t*            _removed
)
{
    const struct d_msvs* const records  = d_msvs_doc_records(_doc);
    const size_t               capacity = (records->length / 2u) + 2u;
    struct d_internal_span*    spans    = malloc(capacity * sizeof(*spans));
    size_t                     count    = 0;
    const size_t               solution = d_internal_sln_section(
                                   records, "SolutionConfigurationPlatforms");
    const size_t               projects = d_internal_sln_section(
                                   records, "ProjectConfigurationPlatforms");

    // the spans need room of their own
    if (!spans)
    {
        return false;
    }

    // each of the solution's configurations the pattern matches
    for (size_t at = 0; (solution != D_MSVS_NONE) &&
                        (at < records->configuration_count); ++at)
    {
        // each of the solution's configurations the pattern matches
        if (d_msvs_config_matches(_pattern, records->configurations[at]))
        {
            count = d_internal_sln_pick(records,
                                        records->sections[solution].content,
                                        records->sections[solution].close,
                                        records->configurations[at], false,
                                        spans, count, capacity);
        }
    }

    *_removed = count;

    for (size_t line = (projects != D_MSVS_NONE)
                     ? records->sections[projects].content : 0u;
         (projects != D_MSVS_NONE) &&
         (line < records->sections[projects].close) && (count < capacity);
         line = d_internal_line_end(records, line))
    {
        const size_t end = d_internal_line_end(records, line);
        char         config[D_INTERNAL_PATH];

        // each project's line for a configuration the pattern matches
        if ( (d_internal_sln_config_of(records, line, end, config,
                                       sizeof(config))) &&
             (d_msvs_config_matches(_pattern, config)) )
        {
            spans[count].start = line;
            spans[count].end   = end;
            count++;
        }
    }

    const bool ok = d_internal_sln_remove_spans(_doc, spans, count);

    free(spans);

    return ok;
}


//==============================================================================
// 5.  FILES
//==============================================================================


/*
d_internal_take
  Makes a document a solution open, and gives its number.
*/
static bool
d_internal_take(
    struct d_host_sln*  _host,
    struct d_msvs_doc*  _doc,
    struct d_awk_value* _out
)
{
    // only a solution is the extension's
    if (d_msvs_doc_kind(_doc) != D_MSVS_SOLUTION)
    {
        d_msvs_doc_free(_doc);

        return d_internal_fail(_host, _out, "not a solution: an MSBuild "
                                            "file is the vcxproj "
                                            "extension's");
    }

    // the list of solutions grows to hold one more
    if (_host->count >= _host->capacity)
    {
        const size_t wanted = (_host->capacity == 0u)
                            ? 8u : (_host->capacity * 2u);
        struct d_msvs_doc** const grown =
            realloc(_host->solutions, wanted * sizeof(*grown));

        if (!grown)
        {
            d_msvs_doc_free(_doc);

            return d_internal_fail(_host, _out, "out of memory");
        }

        _host->solutions = grown;
        _host->capacity  = wanted;
    }

    _host->solutions[_host->count++] = _doc;
    d_awk_value_set_number(_out, (double)_host->count);

    return true;
}


/*
sln_open(path)
  The solution's number, or -1.
*/
static bool
d_internal_sln_open(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    char why[384];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    struct d_msvs_doc* const doc = d_msvs_doc_read(d_internal_text(_args[0]),
                                                   why, sizeof(why));

    // a file that cannot be read, or does not read whole, is not opened
    if (!doc)
    {
        return d_internal_fail(_user, _out, why);
    }

    return d_internal_take(_user, doc, _out);
}


/*
sln_new(path)
  A .slnx with no projects, or a .sln as Visual Studio 2022 begins one.
*/
static bool
d_internal_sln_new(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    char       path[D_INTERNAL_PATH];
    char       why[384];
    char       text[1024];
    char       seed[D_INTERNAL_PATH + 16u];
    char       guid[37];
    char       upper[48];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_internal_copy_arg(_args[0], path, sizeof(path));

    const size_t length = strlen(path);
    const bool   xml    = (length > 5u) &&
                          (d_msvs_compare_paths(&path[length - 5u],
                                                ".slnx") == 0);

    // only a solution's name makes a solution
    if (d_msvs_kind_of(path) != D_MSVS_SOLUTION)
    {
        return d_internal_fail(_user, _out, "not a solution's name: "
                                            "a .sln or a .slnx");
    }

    (void)snprintf(seed, sizeof(seed), "solution:%s", path);
    d_msvs_guid(seed, strlen(seed), guid);
    d_internal_upper(guid, upper, sizeof(upper));

    // a .slnx is its root; a .sln, its header and Global's four sections
    if (xml)
    {
        (void)snprintf(text, sizeof(text), "<Solution>\r\n</Solution>\r\n");
    }
    else
    {
        (void)snprintf(text, sizeof(text),
                       "\xEF\xBB\xBF\r\n"
                       "Microsoft Visual Studio Solution File, Format "
                       "Version 12.00\r\n"
                       "# Visual Studio Version 17\r\n"
                       "VisualStudioVersion = 17.14.36327.8\r\n"
                       "MinimumVisualStudioVersion = 10.0.40219.1\r\n"
                       "Global\r\n"
                       "\tGlobalSection(SolutionConfigurationPlatforms) = "
                       "preSolution\r\n"
                       "\tEndGlobalSection\r\n"
                       "\tGlobalSection(ProjectConfigurationPlatforms) = "
                       "postSolution\r\n"
                       "\tEndGlobalSection\r\n"
                       "\tGlobalSection(SolutionProperties) = preSolution\r\n"
                       "\t\tHideSolutionNode = FALSE\r\n"
                       "\tEndGlobalSection\r\n"
                       "\tGlobalSection(ExtensibilityGlobals) = "
                       "postSolution\r\n"
                       "\t\tSolutionGuid = %s\r\n"
                       "\tEndGlobalSection\r\n"
                       "EndGlobal\r\n", upper);
    }

    struct d_msvs_doc* const doc = d_msvs_doc_make(path, text, strlen(text),
                                                   why, sizeof(why));

    if (!doc)
    {
        return d_internal_fail(_user, _out, why);
    }

    return d_internal_take(_user, doc, _out);
}


/*
sln_save(s[, path])
  1 written, 2 the file held it already, or -1.
*/
static bool
d_internal_sln_save(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_msvs_doc* const doc = d_internal_solution(_user, _args[0]);

    (void)_problem;
    (void)_capacity;

    // a number naming nothing saves nothing
    if (!doc)
    {
        return d_internal_fail(_user, _out, "no such solution");
    }

    const enum d_msvs_saved saved =
        d_msvs_doc_save(doc, (_count > 1u) ? d_internal_text(_args[1])
                                           : NULL);

    if (saved == D_MSVS_SAVE_FAILED)
    {
        return d_internal_fail(_user, _out, d_msvs_doc_problem(doc));
    }

    d_awk_value_set_number(_out, (saved == D_MSVS_SAVE_WRITTEN) ? 1.0 : 2.0);

    return true;
}


/*
sln_close(s)
  1, or 0 for a number naming nothing.
*/
static bool
d_internal_sln_close(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_sln* const host = _user;
    struct d_msvs_doc* const doc  = d_internal_solution(host, _args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // closing nothing is not a mistake
    if (doc)
    {
        d_msvs_doc_free(doc);
        host->solutions[d_internal_number(_args[0]) - 1u] = NULL;
    }

    d_awk_value_set_number(_out, (doc) ? 1.0 : 0.0);

    return true;
}


/*
sln_error(), sln_path(s), sln_form(s), sln_changes(s)
  The last failure, and what a solution is.
*/
static bool
d_internal_sln_error(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_host_sln* const host = _user;

    (void)_args;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_awk_value_set_string(_out, host->error, strlen(host->error));
}


static bool
d_internal_sln_path(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_msvs_doc* const doc  = d_internal_solution(_user,
                                                              _args[0]);
    const char* const              path = (doc) ? d_msvs_doc_path(doc) : "";

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_awk_value_set_string(_out, path, strlen(path));
}


static bool
d_internal_sln_form(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_msvs_doc* const doc  = d_internal_solution(_user,
                                                              _args[0]);
    const char* const              form =
        (!doc)                         ? ""
        : (d_msvs_doc_records(doc)->xml) ? "slnx"
        :                                  "sln";

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_awk_value_set_string(_out, form, strlen(form));
}


static bool
d_internal_sln_changes(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_msvs_doc* const doc = d_internal_solution(_user,
                                                             _args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_awk_value_set_number(_out,
                           (doc) ? (double)d_msvs_doc_changes(doc) : 0.0);

    return true;
}


//==============================================================================
// 6.  READING
//==============================================================================


/*
sln_projects(s), sln_folders(s), sln_configs(s)
  How many of each the solution has.
*/
static bool
d_internal_sln_count(
    struct d_host_sln*  _host,
    struct d_awk_value* _value,
    struct d_awk_value* _out,
    int                 _which
)
{
    const struct d_msvs_doc* const doc = d_internal_solution(_host, _value);
    const struct d_msvs* const     records =
        (doc) ? d_msvs_doc_records(doc) : NULL;
    const size_t count = (!records)     ? 0u
                       : (_which == 0)  ? records->project_count
                       : (_which == 1)  ? records->folder_count
                       :                  records->configuration_count;

    d_awk_value_set_number(_out, (double)count);

    return true;
}


static bool
d_internal_sln_projects(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_sln_count(_user, _args[0], _out, 0);
}


static bool
d_internal_sln_folders(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_sln_count(_user, _args[0], _out, 1);
}


static bool
d_internal_sln_configs(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_sln_count(_user, _args[0], _out, 2);
}


/*
d_internal_project_of
  Project `_value`, from 1, of a solution, or NULL.
*/
static const struct d_msvs_project*
d_internal_project_of(
    const struct d_msvs_doc* _doc,
    struct d_awk_value*      _value
)
{
    const size_t number = d_internal_number(_value);

    // a number past the last names nothing
    if ( (!_doc) ||
         (number == 0u) ||
         (number > d_msvs_doc_records(_doc)->project_count) )
    {
        return NULL;
    }

    return &d_msvs_doc_records(_doc)->projects[number - 1u];
}


/*
sln_project(s, i, field)
  name, path, type, id, folder or line; "" for anything else.
*/
static bool
d_internal_sln_project(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_msvs_doc* const     doc     = d_internal_solution(
                                                     _user, _args[0]);
    const struct d_msvs_project* const project = d_internal_project_of(
                                                     doc, _args[1]);
    char                               field[16];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_internal_copy_arg(_args[2], field, sizeof(field));

    // a project not there has no fields
    if (!project)
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    // the line is a number; every other field a string
    if (strcmp(field, "line") == 0)
    {
        d_awk_value_set_number(_out, (double)project->line);

        return true;
    }

    const char* const value = (strcmp(field, "name") == 0)   ? project->name
                            : (strcmp(field, "path") == 0)   ? project->path
                            : (strcmp(field, "type") == 0)   ? project->type
                            : (strcmp(field, "id") == 0)     ? project->id
                            : (strcmp(field, "folder") == 0) ? project->folder
                            :                                  "";

    return d_awk_value_set_string(_out, value, strlen(value));
}


/*
sln_find(s, path)
  The project's number, or 0.
*/
static bool
d_internal_sln_find(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_msvs_doc* const doc = d_internal_solution(_user,
                                                             _args[0]);
    const size_t at = (doc) ? d_internal_project_at(d_msvs_doc_records(doc),
                                                    d_internal_text(_args[1]))
                            : D_MSVS_NONE;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_awk_value_set_number(_out, (at == D_MSVS_NONE) ? 0.0
                                                     : (double)(at + 1u));

    return true;
}


/*
sln_dependencies(s, i), sln_dependency(s, i, k)
  How many projects project i builds after, and the path of the k-th.
*/
static bool
d_internal_sln_dependencies(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_msvs_project* const project =
        d_internal_project_of(d_internal_solution(_user, _args[0]),
                              _args[1]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_awk_value_set_number(_out,
                           (project) ? (double)project->dependency_count
                                     : 0.0);

    return true;
}


static bool
d_internal_sln_dependency(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_msvs_doc* const     doc     = d_internal_solution(
                                                     _user, _args[0]);
    const struct d_msvs_project* const project = d_internal_project_of(
                                                     doc, _args[1]);
    const size_t                       which   = d_internal_number(_args[2]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a dependency past the last is nothing
    if ( (!project) ||
         (which == 0u) ||
         (which > project->dependency_count) )
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const char* const path = d_msvs_doc_records(doc)->dependencies[
                                 project->first_dependency + which - 1u];

    return d_awk_value_set_string(_out, path, strlen(path));
}


/*
sln_folder(s, i), sln_config(s, i)
  A folder's path, and a configuration.
*/
static bool
d_internal_sln_folder(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_msvs_doc* const doc   = d_internal_solution(_user,
                                                               _args[0]);
    const size_t                   which = d_internal_number(_args[1]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a folder past the last is nothing
    if ( (!doc) ||
         (which == 0u) ||
         (which > d_msvs_doc_records(doc)->folder_count) )
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const char* const path = d_msvs_doc_records(doc)->folders[which - 1u]
                                 .path;

    return d_awk_value_set_string(_out, path, strlen(path));
}


static bool
d_internal_sln_config(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_msvs_doc* const doc   = d_internal_solution(_user,
                                                               _args[0]);
    const size_t                   which = d_internal_number(_args[1]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a configuration past the last is nothing
    if ( (!doc) ||
         (which == 0u) ||
         (which > d_msvs_doc_records(doc)->configuration_count) )
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const char* const config =
        d_msvs_doc_records(doc)->configurations[which - 1u];

    return d_awk_value_set_string(_out, config, strlen(config));
}


//==============================================================================
// 7.  CHANGES
//==============================================================================


/*
d_internal_changed
  1 for a change kept, or -1 with the document's reason.
*/
static bool
d_internal_changed(
    struct d_host_sln*       _host,
    struct d_awk_value*      _out,
    const struct d_msvs_doc* _doc,
    bool                     _kept
)
{
    // a change refused says why
    if (!_kept)
    {
        return d_internal_fail(_host, _out, d_msvs_doc_problem(_doc));
    }

    d_awk_value_set_number(_out, 1.0);

    return true;
}


/*
sln_add_project(s, path[, folder[, id]])
  The project's number, or -1; one listed already is its own number.
*/
static bool
d_internal_sln_add_project(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_msvs_doc* const doc = d_internal_solution(_user, _args[0]);
    char                     path[D_INTERNAL_PATH];
    char                     folder[D_INTERNAL_PATH];
    char                     id[64] = "";

    (void)_problem;
    (void)_capacity;

    // a number naming nothing takes no project
    if (!doc)
    {
        return d_internal_fail(_user, _out, "no such solution");
    }

    d_internal_copy_arg(_args[1], path, sizeof(path));
    d_internal_folder_of((_count > 2u) ? d_internal_text(_args[2]) : "",
                         folder, sizeof(folder));

    // an id asked for is the project's
    if (_count > 3u)
    {
        d_internal_copy_arg(_args[3], id, sizeof(id));
    }

    // a project is named by its path
    if (path[0] == '\0')
    {
        return d_internal_fail(_user, _out, "a project needs a path");
    }

    size_t at = d_internal_project_at(d_msvs_doc_records(doc), path);

    // a project listed already is the project
    if (at == D_MSVS_NONE)
    {
        const bool kept = (d_msvs_doc_records(doc)->xml)
                        ? d_internal_slnx_add_project(doc, path, folder, id)
                        : d_internal_lines_add_project(doc, path, folder, id);

        if (!kept)
        {
            return d_internal_fail(_user, _out, d_msvs_doc_problem(doc));
        }

        at = d_internal_project_at(d_msvs_doc_records(doc), path);
    }

    d_awk_value_set_number(_out, (at == D_MSVS_NONE) ? -1.0
                                                     : (double)(at + 1u));

    return true;
}


/*
sln_remove_project(s, path)
  1, 0 for none, or -1.
*/
static bool
d_internal_sln_remove_project(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_msvs_doc* const doc = d_internal_solution(_user, _args[0]);
    char                     path[D_INTERNAL_PATH];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    if (!doc)
    {
        return d_internal_fail(_user, _out, "no such solution");
    }

    d_internal_copy_arg(_args[1], path, sizeof(path));

    const struct d_msvs* const records = d_msvs_doc_records(doc);
    const size_t               at      = d_internal_project_at(records, path);

    // a project not listed is not removed, and that is no failure
    if (at == D_MSVS_NONE)
    {
        d_awk_value_set_number(_out, 0.0);

        return true;
    }

    // a .slnx loses the element; a .sln, its lines and every line naming it
    if (records->xml)
    {
        return d_internal_changed(_user, _out, doc,
                                  (d_msvs_doc_remove(doc,
                                                     records->projects[at]
                                                         .element)) &&
                                  (d_internal_slnx_remove_dependencies(
                                       doc, path, D_MSVS_NONE)));
    }

    return d_internal_changed(_user, _out, doc,
                              d_internal_lines_remove_project(doc, at));
}


/*
sln_add_folder(s, folder)
  The folder, and each holding it.
*/
static bool
d_internal_sln_add_folder_fn(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_msvs_doc* const doc = d_internal_solution(_user, _args[0]);
    char                     folder[D_INTERNAL_PATH];
    char                     id[37];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    if (!doc)
    {
        return d_internal_fail(_user, _out, "no such solution");
    }

    d_internal_folder_of(d_internal_text(_args[1]), folder, sizeof(folder));

    // the top is there already
    if (folder[0] == '\0')
    {
        d_awk_value_set_number(_out, 1.0);

        return true;
    }

    return d_internal_changed(_user, _out, doc,
                              (d_msvs_doc_records(doc)->xml)
                              ? (d_internal_slnx_add_folder(doc, folder)
                                 != D_MSVS_NONE)
                              : d_internal_sln_add_folder(doc, folder, id));
}


/*
d_internal_both
  The two projects a dependency joins, each listed; false with the failure
given when either is not.
*/
static bool
d_internal_both(
    struct d_host_sln*   _host,
    struct d_awk_value** _args,
    struct d_awk_value*  _out,
    struct d_msvs_doc**  _doc,
    size_t*              _project,
    size_t*              _on
)
{
    char path[D_INTERNAL_PATH];
    char on[D_INTERNAL_PATH];

    *_doc = d_internal_solution(_host, _args[0]);

    // the solution, and both projects in it
    if (!*_doc)
    {
        return !d_internal_fail(_host, _out, "no such solution");
    }

    d_internal_copy_arg(_args[1], path, sizeof(path));
    d_internal_copy_arg(_args[2], on, sizeof(on));
    *_project = d_internal_project_at(d_msvs_doc_records(*_doc), path);
    *_on      = d_internal_project_at(d_msvs_doc_records(*_doc), on);

    if ( (*_project == D_MSVS_NONE) ||
         (*_on == D_MSVS_NONE) )
    {
        return !d_internal_fail(_host, _out, "a dependency joins two "
                                             "projects the solution lists");
    }

    return true;
}


/*
sln_add_dependency(s, path, on)
  1, or -1.
*/
static bool
d_internal_sln_add_dependency_fn(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_msvs_doc* doc     = NULL;
    size_t             project = 0;
    size_t             on      = 0;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // both must be listed
    if (!d_internal_both(_user, _args, _out, &doc, &project, &on))
    {
        return true;
    }

    const struct d_msvs* const records = d_msvs_doc_records(doc);

    // a .sln's dependency is a line; a .slnx's an element
    if (!records->xml)
    {
        return d_internal_changed(_user, _out, doc,
                                  d_internal_sln_add_dependency(doc, project,
                                                                on));
    }

    char path[D_INTERNAL_PATH];

    (void)snprintf(path, sizeof(path), "%s", records->projects[on].path);

    const size_t element = records->projects[project].element;

    // a dependency there already needs nothing
    if (d_internal_slnx_named(records, element, "BuildDependency", "Project",
                              path) != D_MSVS_NONE)
    {
        d_awk_value_set_number(_out, 1.0);

        return true;
    }

    const char* const               pair[] = { "Project", path };
    const struct d_msvs_new_element made   = { "BuildDependency", NULL, pair,
                                               1u };

    return d_internal_changed(_user, _out, doc,
                              d_msvs_doc_insert(doc, element, D_MSVS_NONE,
                                                &made) != D_MSVS_NONE);
}


/*
sln_remove_dependency(s, path, on)
  1, 0 for none, or -1.
*/
static bool
d_internal_sln_remove_dependency_fn(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_msvs_doc* doc     = NULL;
    size_t             project = 0;
    size_t             on      = 0;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // both must be listed
    if (!d_internal_both(_user, _args, _out, &doc, &project, &on))
    {
        return true;
    }

    const struct d_msvs* const         records = d_msvs_doc_records(doc);
    const struct d_msvs_project* const holder  = &records->projects[project];
    const size_t                       before  = holder->dependency_count;
    char                               path[D_INTERNAL_PATH];
    bool                               kept    = true;

    (void)snprintf(path, sizeof(path), "%s", records->projects[on].path);

    if (records->xml)
    {
        kept = d_internal_slnx_remove_dependencies(doc, path, project);
    }
    else if (holder->depends != D_MSVS_NONE)
    {
        struct d_internal_span spans[64];
        char                   id[48];

        d_internal_upper(records->projects[on].id, id, sizeof(id));

        const size_t count =
            d_internal_sln_pick(records,
                                d_internal_line_end(records, holder->depends),
                                holder->depends_close, id, false, spans, 0u,
                                64u);

        kept = (d_internal_sln_remove_spans(doc, spans, count)) &&
               (d_internal_lines_tidy(doc));
    }

    if (!kept)
    {
        return d_internal_fail(_user, _out, d_msvs_doc_problem(doc));
    }

    d_awk_value_set_number(_out,
                           (d_msvs_doc_records(doc)->projects[project]
                                .dependency_count < before) ? 1.0 : 0.0);

    return true;
}


/*
sln_add_config(s, config)
  A configuration, for the solution and every project.
*/
static bool
d_internal_sln_add_config_fn(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_msvs_doc* const doc = d_internal_solution(_user, _args[0]);
    char                     config[D_INTERNAL_PATH];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    if (!doc)
    {
        return d_internal_fail(_user, _out, "no such solution");
    }

    d_internal_copy_arg(_args[1], config, sizeof(config));

    char* const bar = strchr(config, '|');

    // a configuration whole: a build type and a platform
    if ( (!bar) ||
         (bar == config) ||
         (bar[1] == '\0') ||
         (strchr(config, '*')) )
    {
        return d_internal_fail(_user, _out, "a configuration is "
                                            "\"Debug|x64\"");
    }

    const struct d_msvs* const records = d_msvs_doc_records(doc);

    // each configuration already built is enough
    for (size_t at = 0; at < records->configuration_count; ++at)
    {
        // one built already needs nothing
        if (d_msvs_compare_paths(records->configurations[at], config) == 0)
        {
            d_awk_value_set_number(_out, 1.0);

            return true;
        }
    }

    *bar = '\0';

    if (records->xml)
    {
        return d_internal_changed(_user, _out, doc,
                                  d_internal_slnx_add_config(doc, config,
                                                             bar + 1));
    }

    *bar = '|';

    char line[(2u * D_INTERNAL_PATH) + 8u];
    bool kept = true;

    (void)snprintf(line, sizeof(line), "%s = %s", config, config);
    kept = d_internal_sln_add_line(doc, "SolutionConfigurationPlatforms",
                                   line);

    // every project builds the configuration, its two lines each
    for (size_t at = 0; (kept) &&
                        (at < d_msvs_doc_records(doc)->project_count); ++at)
    {
        char id[37];
        char path[D_INTERNAL_PATH];

        (void)snprintf(id, sizeof(id), "%s",
                       d_msvs_doc_records(doc)->projects[at].id);
        (void)snprintf(path, sizeof(path), "%s",
                       d_msvs_doc_records(doc)->projects[at].path);
        kept = d_internal_sln_build_lines(doc, id, path, config);
    }

    return d_internal_changed(_user, _out, doc, kept);
}


/*
sln_remove_config(s, config)
  1, 0 for none, or -1.
*/
static bool
d_internal_sln_remove_config_fn(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_msvs_doc* const doc = d_internal_solution(_user, _args[0]);
    char                     config[D_INTERNAL_PATH];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    if (!doc)
    {
        return d_internal_fail(_user, _out, "no such solution");
    }

    d_internal_copy_arg(_args[1], config, sizeof(config));

    char* const bar = strchr(config, '|');

    // a configuration names a platform, or every one
    if (!bar)
    {
        return d_internal_fail(_user, _out, "a configuration is "
                                            "\"Debug|x64\", \"Debug|*\" or "
                                            "\"*|x64\"");
    }

    // a .sln's lines go; a .slnx's build type or platform
    if (!d_msvs_doc_records(doc)->xml)
    {
        size_t removed = 0;

        if (!d_internal_sln_remove_config(doc, config, &removed))
        {
            return d_internal_fail(_user, _out, d_msvs_doc_problem(doc));
        }

        d_awk_value_set_number(_out, (removed > 0u) ? 1.0 : 0.0);

        return true;
    }

    const bool type = (strcmp(bar + 1, "*") == 0);
    const bool all  = (bar == &config[1]) && (config[0] == '*');

    // a build type, or a platform, but not both, nor one whole configuration
    if (type == all)
    {
        return d_internal_fail(_user, _out, "a .slnx builds every build "
                                            "type on every platform: "
                                            "remove \"Debug|*\" or "
                                            "\"*|x64\"");
    }

    *bar = '\0';

    const size_t held = d_internal_slnx_configurations(doc);
    size_t       named = D_MSVS_NONE;

    if (held != D_MSVS_NONE)
    {
        named = d_internal_slnx_named(d_msvs_doc_records(doc), held,
                                      (type) ? "BuildType" : "Platform",
                                      "Name", (type) ? config : (bar + 1));
    }

    // a build type or platform not named is not removed
    if (named == D_MSVS_NONE)
    {
        d_awk_value_set_number(_out, 0.0);

        return true;
    }

    return d_internal_changed(_user, _out, doc,
                              d_msvs_doc_remove(doc, named));
}


//==============================================================================
// 8.  THE EXTENSION
//==============================================================================


// D_INTERNAL_SLN_FUNCTIONS
//   constant: how many host functions the extension adds.
#define D_INTERNAL_SLN_FUNCTIONS 24u


/*
d_internal_functions
  The extension's host functions, as a program calls them.
*/
static const struct d_awk_host_function*
d_internal_functions(void)
{
    static const struct d_awk_host_function functions[
        D_INTERNAL_SLN_FUNCTIONS] =
    {
        { "sln_open",              1u, 1u, d_internal_sln_open           },
        { "sln_new",               1u, 1u, d_internal_sln_new            },
        { "sln_save",              1u, 2u, d_internal_sln_save           },
        { "sln_close",             1u, 1u, d_internal_sln_close          },
        { "sln_error",             0u, 0u, d_internal_sln_error          },
        { "sln_path",              1u, 1u, d_internal_sln_path           },
        { "sln_form",              1u, 1u, d_internal_sln_form           },
        { "sln_changes",           1u, 1u, d_internal_sln_changes        },
        { "sln_projects",          1u, 1u, d_internal_sln_projects       },
        { "sln_project",           3u, 3u, d_internal_sln_project        },
        { "sln_find",              2u, 2u, d_internal_sln_find           },
        { "sln_dependencies",      2u, 2u, d_internal_sln_dependencies   },
        { "sln_dependency",        3u, 3u, d_internal_sln_dependency     },
        { "sln_folders",           1u, 1u, d_internal_sln_folders        },
        { "sln_folder",            2u, 2u, d_internal_sln_folder         },
        { "sln_configs",           1u, 1u, d_internal_sln_configs        },
        { "sln_config",            2u, 2u, d_internal_sln_config         },
        { "sln_add_project",       2u, 4u, d_internal_sln_add_project    },
        { "sln_remove_project",    2u, 2u, d_internal_sln_remove_project },
        { "sln_add_folder",        2u, 2u, d_internal_sln_add_folder_fn  },
        { "sln_add_dependency",    3u, 3u,
          d_internal_sln_add_dependency_fn                                },
        { "sln_remove_dependency", 3u, 3u,
          d_internal_sln_remove_dependency_fn                             },
        { "sln_add_config",        2u, 2u, d_internal_sln_add_config_fn  },
        { "sln_remove_config",     2u, 2u,
          d_internal_sln_remove_config_fn                                 }
    };

    return functions;
}


/*
d_host_sln_new
  An extension with no solution open.
*/
struct d_host_sln*
d_host_sln_new(void)
{
    struct d_host_sln* const host = malloc(sizeof(*host));

    // an extension that cannot be made is NULL
    if (host)
    {
        memset(host, 0, sizeof(*host));
    }

    return host;
}


/*
d_host_sln_free
  Closes every solution open, dropping unsaved changes.
*/
void
d_host_sln_free(
    struct d_host_sln* _host
)
{
    // freeing nothing is allowed
    if (!_host)
    {
        return;
    }

    for (size_t at = 0; at < _host->count; ++at)
    {
        d_msvs_doc_free(_host->solutions[at]);
    }

    free(_host->solutions);
    free(_host);

    return;
}


/*
d_host_sln_install
  The functions are added with the extension as their user pointer.
*/
bool
d_host_sln_install(
    struct d_host_sln*   _host,
    struct d_awk_interp* _interp
)
{
    // parameter validation first
    if ( (!_host) ||
         (!_interp) )
    {
        return false;
    }

    return d_awk_interp_add_functions(_interp, d_internal_functions(),
                                      D_INTERNAL_SLN_FUNCTIONS, _host);
}
