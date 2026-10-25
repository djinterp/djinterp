/*******************************************************************************
* djinterp [tools]                                               dhost_vcxproj.c
*
* Definitions for the declarations in dhost_vcxproj.h.
*   Each file open is a dmsvs document, which keeps its elements' handles
* across its changes; the extension hands a program handles of its own, one
* list for every file, each naming a file and the document's handle there,
* so one number is all a program needs to name an element in any file
* open. A file closed keeps its place in the list of files, empty, so a
* handle into it names nothing rather than an element of another file.
*   The MSBuild functions are built on the document's: an item added is an
* element added beside its kind and in order, and, in a filters file, with
* the <Filter> naming where it shows, each filter on the way made if it is
* not there.
*
* path:      /src/djinterp/tools/dawk/ext/msvs/dhost_vcxproj.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dhost_vcxproj.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, memset, strcmp, strlen
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dhost.h"               // d_awk_host_function
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dmsvs.h"      // d_msvs_compare_paths
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dmsvs_doc.h"  // d_msvs_doc
#include "../../../../../../inc/djinterp/tools/dawk/dsplice.h"             // d_text_read
#include "../../../../../../inc/djinterp/tools/dawk/dvalue.h"              // d_awk_value_text


// D_INTERNAL_MOST_ATTRIBUTES
//   constant: how many attributes vcxproj_add and vcxproj_insert take.
#define D_INTERNAL_MOST_ATTRIBUTES 8u

// D_INTERNAL_XMLNS
//   constant: MSBuild's namespace, which every root it reads declares.
#define D_INTERNAL_XMLNS "http://schemas.microsoft.com/developer/msbuild/2003"


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Extension
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_file
//   struct: one file open, or none once closed, and the extension's handle
//   for each of the document's, 0 until given.
struct d_internal_file
{
    struct d_msvs_doc* doc;
    size_t*            globals;
    size_t             global_capacity;
};

// 1.1.2
// d_internal_handle
//   struct: what a handle names: a file, and the document's handle there.
struct d_internal_handle
{
    size_t file;
    size_t local;
};

// 1.1.3
// d_host_vcxproj
//   struct: the files, the handles, room for a decoded text, and the last
//   failure.
struct d_host_vcxproj
{
    struct d_internal_file*   files;
    size_t                    file_count;
    size_t                    file_capacity;
    struct d_internal_handle* handles;
    size_t                    handle_count;
    size_t                    handle_capacity;
    char*                     scratch;
    size_t                    scratch_capacity;
    char                      error[512];
};

// 1.1.4
// d_internal_at
//   struct: an element a handle names: its file, the file's records, and
//   its index there.
struct d_internal_at
{
    size_t                  file;
    struct d_msvs_doc*      doc;
    const struct d_msvs*    records;
    size_t                  element;
};


//==============================================================================
// 2.  HELPERS
//==============================================================================


/*
d_internal_text, d_internal_number
  An argument's text, and an argument as a handle: a value below 1 is 0,
which names nothing.
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
d_internal_fail
  Gives -1 for a failure the program can recover from, vcxproj_error()
saying why.
*/
static bool
d_internal_fail(
    struct d_host_vcxproj* _host,
    struct d_awk_value*    _out,
    const char*            _why
)
{
    (void)snprintf(_host->error, sizeof(_host->error), "%s",
                   ( (_why) && (_why[0] != '\0') ) ? _why : "failed");
    d_awk_value_set_number(_out, -1.0);

    return true;
}


/*
d_internal_scratch
  Room for a decoded text of up to `_length` bytes.
*/
static char*
d_internal_scratch(
    struct d_host_vcxproj* _host,
    size_t                 _length
)
{
    // the room grows, and is kept
    if ((_length + 1u) > _host->scratch_capacity)
    {
        const size_t wanted = (_length + 1u) * 2u;
        char* const  grown  = realloc(_host->scratch, wanted);

        if (!grown)
        {
            return NULL;
        }

        _host->scratch          = grown;
        _host->scratch_capacity = wanted;
    }

    return _host->scratch;
}


/*
d_internal_attribute
  An attribute's value, decoded into the scratch room; NULL when absent.
*/
static const char*
d_internal_attribute(
    struct d_host_vcxproj* _host,
    const struct d_msvs*   _records,
    size_t                 _element,
    const char*            _name
)
{
    size_t start  = 0;
    size_t length = 0;

    // an attribute not there has no value
    if (!d_msvs_attribute(_records, &_records->elements[_element], _name,
                          &start, &length))
    {
        return NULL;
    }

    char* const out = d_internal_scratch(_host, length);

    if (out)
    {
        (void)d_msvs_value(_records, start, length, out);
    }

    return out;
}


/*
d_internal_element_text
  An element's text, decoded into the scratch room.
*/
static const char*
d_internal_element_text(
    struct d_host_vcxproj* _host,
    const struct d_msvs*   _records,
    size_t                 _element
)
{
    const struct d_msvs_element* const element =
        &_records->elements[_element];
    char* const out = d_internal_scratch(_host, element->content_end -
                                                element->content_start);

    if (out)
    {
        (void)d_msvs_text(_records, element, out);
    }

    return out;
}


//==============================================================================
// 3.  FILES AND HANDLES
//==============================================================================


/*
d_internal_give
  The extension's handle for an element of a file, given the first time it
is asked for; 0 when memory runs out.
*/
static size_t
d_internal_give(
    struct d_host_vcxproj* _host,
    size_t                 _file,
    size_t                 _element
)
{
    struct d_internal_file* const file  = &_host->files[_file];
    const size_t                  local = d_msvs_doc_handle(file->doc,
                                                            _element);

    // an element not there has no handle
    if (local == 0u)
    {
        return 0u;
    }

    // the file's list of handles grows to hold the document's
    if (local >= file->global_capacity)
    {
        const size_t  wanted = (local + 1u) * 2u;
        size_t* const grown  = realloc(file->globals,
                                       wanted * sizeof(size_t));

        if (!grown)
        {
            return 0u;
        }

        memset(&grown[file->global_capacity], 0,
               (wanted - file->global_capacity) * sizeof(size_t));
        file->globals         = grown;
        file->global_capacity = wanted;
    }

    // a handle given before is the handle
    if (file->globals[local] != 0u)
    {
        return file->globals[local];
    }

    // the list of handles grows to hold one more
    if (_host->handle_count >= _host->handle_capacity)
    {
        const size_t wanted = (_host->handle_capacity == 0u)
                            ? 256u : (_host->handle_capacity * 2u);
        struct d_internal_handle* const grown =
            realloc(_host->handles, wanted * sizeof(*grown));

        if (!grown)
        {
            return 0u;
        }

        _host->handles         = grown;
        _host->handle_capacity = wanted;
    }

    // handle 0 is no element's
    if (_host->handle_count == 0u)
    {
        _host->handles[0].file  = 0;
        _host->handles[0].local = 0;
        _host->handle_count     = 1u;
    }

    const size_t global = _host->handle_count++;

    _host->handles[global].file  = _file;
    _host->handles[global].local = local;
    file->globals[local]         = global;

    return global;
}


/*
d_internal_give_out
  Gives the program an element's handle, 0 for none.
*/
static bool
d_internal_give_out(
    struct d_host_vcxproj* _host,
    struct d_awk_value*    _out,
    size_t                 _file,
    size_t                 _element
)
{
    const size_t handle = (_element == D_MSVS_NONE)
                        ? 0u : d_internal_give(_host, _file, _element);

    d_awk_value_set_number(_out, (double)handle);

    return true;
}


/*
d_internal_find
  The element a program's handle names, in a file open; false for none.
*/
static bool
d_internal_find(
    struct d_host_vcxproj* _host,
    struct d_awk_value*    _value,
    struct d_internal_at*  _out
)
{
    const size_t global = d_internal_number(_value);

    // a handle never given, or into a file closed, names nothing
    if ( (global == 0u) ||
         (global >= _host->handle_count) ||
         (!_host->files[_host->handles[global].file].doc) )
    {
        return false;
    }

    const struct d_internal_handle* const handle = &_host->handles[global];

    _out->file    = handle->file;
    _out->doc     = _host->files[handle->file].doc;
    _out->records = d_msvs_doc_records(_out->doc);
    _out->element = d_msvs_doc_element(_out->doc, handle->local);

    return (_out->element != D_MSVS_NONE);
}


/*
d_internal_take
  Makes a document a file open, and gives its root's handle; -1 when memory
runs out.
*/
static bool
d_internal_take(
    struct d_host_vcxproj* _host,
    struct d_msvs_doc*     _doc,
    struct d_awk_value*    _out
)
{
    // only an XML file has elements to name
    if (!d_msvs_doc_records(_doc)->xml)
    {
        d_msvs_doc_free(_doc);

        return d_internal_fail(_host, _out, "not an MSBuild file: a .sln "
                                            "is the sln extension's");
    }

    // the list of files grows to hold one more
    if (_host->file_count >= _host->file_capacity)
    {
        const size_t wanted = (_host->file_capacity == 0u)
                            ? 8u : (_host->file_capacity * 2u);
        struct d_internal_file* const grown =
            realloc(_host->files, wanted * sizeof(*grown));

        if (!grown)
        {
            d_msvs_doc_free(_doc);

            return d_internal_fail(_host, _out, "out of memory");
        }

        _host->files         = grown;
        _host->file_capacity = wanted;
    }

    struct d_internal_file* const file = &_host->files[_host->file_count++];

    file->doc             = _doc;
    file->globals         = NULL;
    file->global_capacity = 0;

    const size_t root = d_internal_give(_host, _host->file_count - 1u, 0u);

    if (root == 0u)
    {
        return d_internal_fail(_host, _out, "out of memory");
    }

    d_awk_value_set_number(_out, (double)root);

    return true;
}


/*
d_internal_skeleton
  The text of a new file of a kind: a declaration, and a root as Visual
Studio writes it for that kind, with nothing in it.
*/
static const char*
d_internal_skeleton(
    enum d_msvs_kind _kind
)
{
    static const char project[] =
        "\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
        "<Project DefaultTargets=\"Build\" xmlns=\"" D_INTERNAL_XMLNS "\">"
        "\r\n</Project>";
    static const char filters[] =
        "\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
        "<Project ToolsVersion=\"4.0\" xmlns=\"" D_INTERNAL_XMLNS "\">"
        "\r\n</Project>";
    static const char user[] =
        "\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
        "<Project ToolsVersion=\"Current\" xmlns=\"" D_INTERNAL_XMLNS "\">"
        "\r\n</Project>";

    // a project builds; filters and property sheets name a tools version
    if (_kind == D_MSVS_PROJECT)
    {
        return project;
    }

    return (_kind == D_MSVS_USER) ? user : filters;
}


//==============================================================================
// 4.  FILES
//==============================================================================


/*
vcxproj_open(path)
  The root's handle, or -1.
*/
static bool
d_internal_vcxproj_open(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    char                         why[384];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    struct d_msvs_doc* const doc = d_msvs_doc_read(d_internal_text(_args[0]),
                                                   why, sizeof(why));

    // a file that cannot be read, or does not read whole, is not opened
    if (!doc)
    {
        return d_internal_fail(host, _out, why);
    }

    return d_internal_take(host, doc, _out);
}


/*
vcxproj_new(path[, template])
  The root's handle of a file not yet saved, or -1.
*/
static bool
d_internal_vcxproj_new(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    char                         why[384];
    char* const                  path = malloc(strlen(d_internal_text(
                                                   _args[0])) + 1u);

    (void)_problem;
    (void)_capacity;

    // the path is copied, as the template's text is read past it
    if (!path)
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    memcpy(path, d_internal_text(_args[0]),
           strlen(d_internal_text(_args[0])) + 1u);

    const enum d_msvs_kind kind = d_msvs_kind_of(path);

    // a solution is the sln extension's to make
    if (kind == D_MSVS_SOLUTION)
    {
        free(path);

        return d_internal_fail(host, _out, "a solution is made with "
                                           "sln_new");
    }

    size_t      length = 0;
    const char* model  = (_count > 1u) ? d_internal_text(_args[1]) : NULL;
    char* const text   = (model) ? d_text_read(model, &length) : NULL;

    // a template that cannot be read makes nothing
    if ( (model) &&
         (!text) )
    {
        (void)snprintf(why, sizeof(why), "cannot read: %s", model);
        free(path);

        return d_internal_fail(host, _out, why);
    }

    const char* const        skeleton = d_internal_skeleton(kind);
    struct d_msvs_doc* const doc      =
        d_msvs_doc_make(path, (text) ? text : skeleton,
                        (text) ? length : strlen(skeleton), why, sizeof(why));

    free(text);
    free(path);

    if (!doc)
    {
        return d_internal_fail(host, _out, why);
    }

    return d_internal_take(host, doc, _out);
}


/*
vcxproj_save(e[, path])
  1 written, 2 the file held it already, or -1.
*/
static bool
d_internal_vcxproj_save(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;

    (void)_problem;
    (void)_capacity;

    // a handle into no file saves nothing
    if (!d_internal_find(host, _args[0], &at))
    {
        return d_internal_fail(host, _out, "no such element");
    }

    const enum d_msvs_saved saved =
        d_msvs_doc_save(at.doc, (_count > 1u) ? d_internal_text(_args[1])
                                              : NULL);

    // a save that fails says why
    if (saved == D_MSVS_SAVE_FAILED)
    {
        return d_internal_fail(host, _out, d_msvs_doc_problem(at.doc));
    }

    d_awk_value_set_number(_out, (saved == D_MSVS_SAVE_WRITTEN) ? 1.0 : 2.0);

    return true;
}


/*
vcxproj_close(e)
  1, or 0 when `e` names nothing; the file's place is kept, empty.
*/
static bool
d_internal_vcxproj_close(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // closing nothing is not a mistake
    if (!d_internal_find(host, _args[0], &at))
    {
        d_awk_value_set_number(_out, 0.0);

        return true;
    }

    struct d_internal_file* const file = &host->files[at.file];

    d_msvs_doc_free(file->doc);
    free(file->globals);
    file->doc             = NULL;
    file->globals         = NULL;
    file->global_capacity = 0;
    d_awk_value_set_number(_out, 1.0);

    return true;
}


/*
vcxproj_error()
  Why the last -1 was given.
*/
static bool
d_internal_vcxproj_error(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_host_vcxproj* const host = _user;

    (void)_args;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_awk_value_set_string(_out, host->error, strlen(host->error));
}


/*
vcxproj_path(e), vcxproj_kind(e), vcxproj_changes(e)
  The file's path, its kind, and the changes it has had.
*/
static bool
d_internal_vcxproj_path(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_internal_at at;
    const char*          path = "";

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle into no file has no path
    if (d_internal_find(_user, _args[0], &at))
    {
        path = d_msvs_doc_path(at.doc);
    }

    return d_awk_value_set_string(_out, path, strlen(path));
}


static bool
d_internal_vcxproj_kind(
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
    struct d_internal_at at;
    const char*          kind = "";

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle into no file has no kind
    if (d_internal_find(_user, _args[0], &at))
    {
        kind = names[d_msvs_doc_kind(at.doc)];
    }

    return d_awk_value_set_string(_out, kind, strlen(kind));
}


static bool
d_internal_vcxproj_changes(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_internal_at at;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_awk_value_set_number(_out,
                           (d_internal_find(_user, _args[0], &at))
                           ? (double)d_msvs_doc_changes(at.doc) : 0.0);

    return true;
}


//==============================================================================
// 5.  ELEMENTS
//==============================================================================


/*
vcxproj_root(e), vcxproj_parent(e), vcxproj_first(e), vcxproj_next(e)
  An element's relations, each 0 for none.
*/
static bool
d_internal_relation(
    struct d_host_vcxproj* _host,
    struct d_awk_value*    _value,
    struct d_awk_value*    _out,
    int                    _which
)
{
    struct d_internal_at at;

    // a handle naming nothing is related to nothing
    if (!d_internal_find(_host, _value, &at))
    {
        d_awk_value_set_number(_out, 0.0);

        return true;
    }

    const struct d_msvs_element* const element =
        &at.records->elements[at.element];
    const size_t related = (_which == 0) ? 0u
                         : (_which == 1) ? element->parent
                         : (_which == 2) ? element->first_child
                         :                 element->next_sibling;

    return d_internal_give_out(_host, _out, at.file, related);
}


static bool
d_internal_vcxproj_root(
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

    return d_internal_relation(_user, _args[0], _out, 0);
}


static bool
d_internal_vcxproj_parent(
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

    return d_internal_relation(_user, _args[0], _out, 1);
}


static bool
d_internal_vcxproj_first(
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

    return d_internal_relation(_user, _args[0], _out, 2);
}


static bool
d_internal_vcxproj_next(
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

    return d_internal_relation(_user, _args[0], _out, 3);
}


/*
vcxproj_children(e), vcxproj_line(e), vcxproj_depth(e)
  Counts and places, each 0 for an element not there.
*/
static bool
d_internal_measure(
    struct d_host_vcxproj* _host,
    struct d_awk_value*    _value,
    struct d_awk_value*    _out,
    int                    _which
)
{
    struct d_internal_at at;
    double               measure = 0.0;

    // a handle naming nothing measures nothing
    if (d_internal_find(_host, _value, &at))
    {
        const struct d_msvs_element* const element =
            &at.records->elements[at.element];

        measure = (_which == 0) ? (double)element->child_count
                : (_which == 1) ? (double)element->line
                :                 (double)element->depth;
    }

    d_awk_value_set_number(_out, measure);

    return true;
}


static bool
d_internal_vcxproj_children(
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

    return d_internal_measure(_user, _args[0], _out, 0);
}


static bool
d_internal_vcxproj_line(
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

    return d_internal_measure(_user, _args[0], _out, 1);
}


static bool
d_internal_vcxproj_depth(
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

    return d_internal_measure(_user, _args[0], _out, 2);
}


/*
vcxproj_find(e, name[, after])
  Within an element, its descendants follow it in order up to the first
element that starts past its end.
*/
static bool
d_internal_vcxproj_find(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         within;
    struct d_internal_at         after;
    const char* const            name = d_internal_text(_args[1]);

    (void)_problem;
    (void)_capacity;

    // a search within nothing finds nothing
    if (!d_internal_find(host, _args[0], &within))
    {
        d_awk_value_set_number(_out, 0.0);

        return true;
    }

    const struct d_msvs* const records = within.records;
    const size_t               end     = records->elements[within.element].end;
    size_t                     from    = within.element + 1u;

    // a search after an element of the same file goes on from it
    if ( (_count > 2u) &&
         (d_internal_find(host, _args[2], &after)) &&
         (after.file == within.file) &&
         (after.element > within.element) )
    {
        from = after.element + 1u;
    }

    // each element within it, in order, from where the search goes on
    for (size_t at = from; (at < records->element_count) &&
                           (records->elements[at].start < end); ++at)
    {
        // any element, for no name, or the one so named
        if ( (name[0] == '\0') ||
             (d_msvs_is(records, &records->elements[at], name)) )
        {
            return d_internal_give_out(host, _out, within.file, at);
        }
    }

    d_awk_value_set_number(_out, 0.0);

    return true;
}


/*
vcxproj_name(e), vcxproj_text(e)
  An element's name, as written, and its text, decoded.
*/
static bool
d_internal_vcxproj_name(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_internal_at at;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle naming nothing has no name
    if (!d_internal_find(_user, _args[0], &at))
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const struct d_msvs_element* const element =
        &at.records->elements[at.element];

    return d_awk_value_set_string(_out, &at.records->text[element->name],
                                  element->name_length);
}


static bool
d_internal_vcxproj_text(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;

    (void)_count;
    (void)_capacity;

    // a handle naming nothing has no text
    if (!d_internal_find(host, _args[0], &at))
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const char* const text = d_internal_element_text(host, at.records,
                                                     at.element);

    if (!text)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    return d_awk_value_set_input(_out, text, strlen(text));
}


/*
vcxproj_attr(e, name), vcxproj_has(e, name)
  An attribute's value, decoded, and whether it is there.
*/
static bool
d_internal_vcxproj_attr(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle naming nothing has no attributes
    if (!d_internal_find(host, _args[0], &at))
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const char* const value = d_internal_attribute(host, at.records,
                                                   at.element,
                                                   d_internal_text(_args[1]));

    return (value) ? d_awk_value_set_input(_out, value, strlen(value))
                   : d_awk_value_set_string(_out, "", 0u);
}


static bool
d_internal_vcxproj_has(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_internal_at at;
    size_t               start  = 0;
    size_t               length = 0;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_awk_value_set_number(_out,
                           ( (d_internal_find(_user, _args[0], &at)) &&
                             (d_msvs_attribute(at.records,
                                               &at.records->elements[
                                                   at.element],
                                               d_internal_text(_args[1]),
                                               &start, &length)) )
                           ? 1.0 : 0.0);

    return true;
}


//==============================================================================
// 6.  MSBUILD
//==============================================================================


/*
d_internal_condition
  What an element's own Condition tests, with the configuration it tests in
`_tested`.
*/
static enum d_msvs_condition
d_internal_condition(
    struct d_host_vcxproj* _host,
    const struct d_msvs*   _records,
    size_t                 _element,
    char*                  _tested,
    size_t                 _capacity
)
{
    const char* const condition = d_internal_attribute(_host, _records,
                                                       _element,
                                                       "Condition");

    return d_msvs_condition_of(condition, _tested, _capacity);
}


/*
d_internal_applies
  1 when nothing from an element out to the root says it does not apply to
a configuration, 2 when nothing says so but something is not understood,
and 0 when something says so.  With no configuration, only an element no
Condition on configurations holds applies.
*/
static int
d_internal_applies(
    struct d_host_vcxproj* _host,
    const struct d_msvs*   _records,
    size_t                 _element,
    const char*            _config
)
{
    int applies = 1;

    // each element from this one out to the root
    for (size_t at = _element; at != D_MSVS_NONE;
         at = _records->elements[at].parent)
    {
        char                        tested[256];
        const enum d_msvs_condition condition =
            d_internal_condition(_host, _records, at, tested,
                                 sizeof(tested));

        // a condition on the configuration decides; any other is unknown
        if ( (condition == D_MSVS_CONFIGURATION) &&
             ( (!_config) || (!d_msvs_config_matches(tested, _config)) ) )
        {
            return 0;
        }

        // a condition not understood leaves the answer uncertain
        if (condition == D_MSVS_UNKNOWN)
        {
            applies = 2;
        }
    }

    return applies;
}


/*
vcxproj_condition(e)
  "", the configuration tested, or "?".
*/
static bool
d_internal_vcxproj_condition(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_internal_at at;
    char                 tested[256] = "";

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle naming nothing is held to nothing
    if (!d_internal_find(_user, _args[0], &at))
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const enum d_msvs_condition condition =
        d_internal_condition(_user, at.records, at.element, tested,
                             sizeof(tested));
    const char* const said = (condition == D_MSVS_ALWAYS)        ? ""
                           : (condition == D_MSVS_CONFIGURATION) ? tested
                           :                                       "?";

    return d_awk_value_set_string(_out, said, strlen(said));
}


/*
vcxproj_applies(e, config)
  1, 0 or 2.
*/
static bool
d_internal_vcxproj_applies(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_internal_at at;
    char                 config[256];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    (void)snprintf(config, sizeof(config), "%s", d_internal_text(_args[1]));

    // a handle naming nothing applies to nothing
    if (!d_internal_find(_user, _args[0], &at))
    {
        d_awk_value_set_number(_out, 0.0);

        return true;
    }

    d_awk_value_set_number(_out, (double)d_internal_applies(_user,
                                                            at.records,
                                                            at.element,
                                                            config));

    return true;
}


/*
vcxproj_property(e, name[, config])
  Every property group's child so named, in order, the last that applies
winning, as MSBuild's evaluation would leave it if nothing were expanded.
*/
static bool
d_internal_vcxproj_property(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;
    char                         name[256];
    char                         config[256];
    size_t                       found = D_MSVS_NONE;

    (void)_problem;
    (void)_capacity;

    (void)snprintf(name, sizeof(name), "%s", d_internal_text(_args[1]));
    (void)snprintf(config, sizeof(config), "%s",
                   (_count > 2u) ? d_internal_text(_args[2]) : "");

    // a handle naming nothing holds no properties
    if (!d_internal_find(host, _args[0], &at))
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const struct d_msvs* const records = at.records;

    // every property group of the file, in order
    for (size_t group = records->elements[0].first_child;
         group != D_MSVS_NONE; group = records->elements[group].next_sibling)
    {
        // only a property group sets properties
        if (!d_msvs_is(records, &records->elements[group], "PropertyGroup"))
        {
            continue;
        }

        // and each property it sets
        for (size_t property = records->elements[group].first_child;
             property != D_MSVS_NONE;
             property = records->elements[property].next_sibling)
        {
            // the property, where it applies
            if ( (d_msvs_is(records, &records->elements[property], name)) &&
                 (d_internal_applies(host, records, property,
                                     (config[0] != '\0') ? config : NULL)
                  != 0) )
            {
                found = property;
            }
        }
    }

    // a property no group sets has no value
    if (found == D_MSVS_NONE)
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const char* const text = d_internal_element_text(host, records, found);

    return (text) ? d_awk_value_set_input(_out, text, strlen(text))
                  : d_awk_value_set_string(_out, "", 0u);
}


/*
d_internal_item
  The item of a type naming a file, among a file's item groups; D_MSVS_NONE
for none.
*/
static size_t
d_internal_item(
    struct d_host_vcxproj* _host,
    const struct d_msvs*   _records,
    const char*            _type,
    const char*            _include
)
{
    // every item of the file, in order
    for (size_t at = 0; at < _records->element_count; ++at)
    {
        const struct d_msvs_element* const element = &_records->elements[at];

        // an item of the type, in an item group
        if ( (element->parent == D_MSVS_NONE) ||
             (!d_msvs_is(_records, element, _type)) ||
             (!d_msvs_is(_records, &_records->elements[element->parent],
                         "ItemGroup")) )
        {
            continue;
        }

        const char* const include = d_internal_attribute(_host, _records, at,
                                                         "Include");

        // the one naming the file
        if ( (include) &&
             (d_msvs_compare_paths(include, _include) == 0) )
        {
            return at;
        }
    }

    return D_MSVS_NONE;
}


/*
vcxproj_item(e, type, include)
  The item's handle, or 0.
*/
static bool
d_internal_vcxproj_item(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;
    char                         type[256];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    (void)snprintf(type, sizeof(type), "%s", d_internal_text(_args[1]));

    // a handle naming nothing holds no items
    if (!d_internal_find(host, _args[0], &at))
    {
        d_awk_value_set_number(_out, 0.0);

        return true;
    }

    return d_internal_give_out(host, _out, at.file,
                               d_internal_item(host, at.records, type,
                                               d_internal_text(_args[2])));
}


/*
d_internal_group_for
  The item group an item of a type joins: the last holding items of that
type, or a new one -- in a project, before the import of the C++ targets,
where Visual Studio keeps its items; elsewhere, after the last child.
D_MSVS_NONE when one cannot be made.
*/
static size_t
d_internal_group_for(
    struct d_host_vcxproj* _host,
    struct d_msvs_doc*     _doc,
    const char*            _type
)
{
    const struct d_msvs* records = d_msvs_doc_records(_doc);
    size_t               group   = D_MSVS_NONE;
    size_t               before  = D_MSVS_NONE;

    // the root's children: its imports and its item groups
    for (size_t at = records->elements[0].first_child; at != D_MSVS_NONE;
         at = records->elements[at].next_sibling)
    {
        const struct d_msvs_element* const element = &records->elements[at];

        // the import an item group goes before, in a project
        if ( (before == D_MSVS_NONE) &&
             (d_msvs_doc_kind(_doc) == D_MSVS_PROJECT) &&
             (d_msvs_is(records, element, "Import")) )
        {
            const char* const project = d_internal_attribute(_host, records,
                                                             at, "Project");
            const size_t      length  = (project) ? strlen(project) : 0u;

            if ( (length >= 21u) &&
                 (d_msvs_compare_paths(&project[length - 21u],
                                       "Microsoft.Cpp.targets") == 0) )
            {
                before = at;
            }
        }

        // a group holding the type
        if ( (d_msvs_is(records, element, "ItemGroup")) &&
             (d_msvs_child(records, at, _type) != D_MSVS_NONE) )
        {
            group = at;
        }
    }

    // a new group, where Visual Studio would put it
    if (group == D_MSVS_NONE)
    {
        const struct d_msvs_new_element made = { "ItemGroup", NULL, NULL,
                                                 0u };

        group = d_msvs_doc_insert(_doc, 0u, before, &made);
    }

    return group;
}


/*
d_internal_place_in
  The child of a group an item goes before, so items of its type stay in
order of their files: the first of its type naming a file after it, or
the sibling after the last of its type; D_MSVS_NONE to go last.
*/
static size_t
d_internal_place_in(
    struct d_host_vcxproj* _host,
    const struct d_msvs*   _records,
    size_t                 _group,
    const char*            _type,
    const char*            _include
)
{
    size_t last = D_MSVS_NONE;

    // the group's items of the type, in order
    for (size_t at = _records->elements[_group].first_child;
         at != D_MSVS_NONE; at = _records->elements[at].next_sibling)
    {
        // only items of the type are in the order
        if (!d_msvs_is(_records, &_records->elements[at], _type))
        {
            continue;
        }

        const char* const include = d_internal_attribute(_host, _records, at,
                                                         "Include");

        // the first naming a file that sorts after this one
        if ( (include) &&
             (d_msvs_compare_paths(include, _include) > 0) )
        {
            return at;
        }

        last = at;
    }

    return (last == D_MSVS_NONE) ? D_MSVS_NONE
                                 : _records->elements[last].next_sibling;
}


/*
d_internal_add_filter
  Makes a filter, and each filter holding it, unless there: a <Filter> item
with a UniqueIdentifier made from its name.  The filters' group is the one
holding filters, or a new one first in the file.
*/
static bool
d_internal_add_filter(
    struct d_host_vcxproj* _host,
    struct d_msvs_doc*     _doc,
    const char*            _filter
)
{
    const size_t length = strlen(_filter);
    char* const  name   = malloc(length + 1u);

    // the filter's name is cut at each `\` in turn
    if (!name)
    {
        return false;
    }

    bool ok = true;

    for (size_t end = 1u; (ok) && (end <= length); ++end)
    {
        // a whole name, or the name of a filter holding it
        if ( (end < length) &&
             (_filter[end] != '\\') )
        {
            continue;
        }

        memcpy(name, _filter, end);
        name[end] = '\0';

        const struct d_msvs* records = d_msvs_doc_records(_doc);

        // a filter there already is left as it is
        if (d_internal_item(_host, records, "Filter", name) != D_MSVS_NONE)
        {
            continue;
        }

        size_t group = D_MSVS_NONE;

        // the item group that holds filters
        for (size_t at = records->elements[0].first_child;
             (group == D_MSVS_NONE) && (at != D_MSVS_NONE);
             at = records->elements[at].next_sibling)
        {
            if ( (d_msvs_is(records, &records->elements[at], "ItemGroup")) &&
                 (d_msvs_child(records, at, "Filter") != D_MSVS_NONE) )
            {
                group = at;
            }
        }

        // a file with no filters yet holds them first
        if (group == D_MSVS_NONE)
        {
            const struct d_msvs_new_element made = { "ItemGroup", NULL, NULL,
                                                     0u };

            group = d_msvs_doc_insert(_doc, 0u, records->elements[0]
                                                    .first_child, &made);
        }

        char              seed[600];
        char              guid[37];
        char              braced[40];
        const char* const include[] = { "Include", name };
        const struct d_msvs_new_element filter = { "Filter", NULL, include,
                                                   1u };

        (void)snprintf(seed, sizeof(seed), "filter:%s", name);
        d_msvs_guid(seed, strlen(seed), guid);
        (void)snprintf(braced, sizeof(braced), "{%s}", guid);

        const struct d_msvs_new_element unique = { "UniqueIdentifier",
                                                   braced, NULL, 0u };
        const size_t made =
            (group == D_MSVS_NONE) ? D_MSVS_NONE
                                   : d_msvs_doc_insert(
                                         _doc, group,
                                         d_internal_place_in(
                                             _host,
                                             d_msvs_doc_records(_doc),
                                             group, "Filter", name),
                                         &filter);

        ok = (made != D_MSVS_NONE) &&
             (d_msvs_doc_insert(_doc, made, D_MSVS_NONE, &unique)
              != D_MSVS_NONE);
    }

    free(name);

    return ok;
}


/*
d_internal_set_filter
  Gives an item of a filters file its <Filter>, in place of the one it has.
*/
static bool
d_internal_set_filter(
    struct d_msvs_doc* _doc,
    size_t             _item,
    const char*        _filter
)
{
    const struct d_msvs* const records = d_msvs_doc_records(_doc);
    const size_t               held    = d_msvs_child(records, _item,
                                                      "Filter");

    // a <Filter> there takes the name; else one is added
    if (held != D_MSVS_NONE)
    {
        return d_msvs_doc_set_text(_doc, held, _filter);
    }

    const struct d_msvs_new_element filter = { "Filter", _filter, NULL, 0u };

    return (d_msvs_doc_insert(_doc, _item, D_MSVS_NONE, &filter)
            != D_MSVS_NONE);
}


/*
vcxproj_add_item(e, type, include[, filter])
  The item, beside its kind and in order; one already there is the item,
given the filter when one is named.
*/
static bool
d_internal_vcxproj_add_item(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;
    char                         type[256];
    char                         filter[512];

    (void)_problem;
    (void)_capacity;

    (void)snprintf(type, sizeof(type), "%s", d_internal_text(_args[1]));
    (void)snprintf(filter, sizeof(filter), "%s",
                   (_count > 3u) ? d_internal_text(_args[3]) : "");

    const size_t length  = strlen(d_internal_text(_args[2]));
    char* const  include = malloc(length + 1u);

    // the include is copied, as the arguments' texts are borrowed
    if (!include)
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    memcpy(include, d_internal_text(_args[2]), length + 1u);

    if (!d_internal_find(host, _args[0], &at))
    {
        free(include);

        return d_internal_fail(host, _out, "no such element");
    }

    const bool filters = (filter[0] != '\0') &&
                         (d_msvs_doc_kind(at.doc) == D_MSVS_FILTERS);
    bool       ok      = (!filters) ||
                         (d_internal_add_filter(host, at.doc, filter));
    size_t     item    = d_internal_item(host, at.records, type, include);

    // an item not there is added beside its kind
    if ( (ok) &&
         (item == D_MSVS_NONE) )
    {
        const char* const               pair[] = { "Include", include };
        const struct d_msvs_new_element made   = { type, NULL, pair, 1u };
        const size_t group = d_internal_group_for(host, at.doc, type);

        item = (group == D_MSVS_NONE)
             ? D_MSVS_NONE
             : d_msvs_doc_insert(at.doc, group,
                                 d_internal_place_in(
                                     host, d_msvs_doc_records(at.doc), group,
                                     type, include),
                                 &made);
        ok   = (item != D_MSVS_NONE);
    }

    // in a filters file it shows in its filter
    if ( (ok) &&
         (filters) )
    {
        const size_t handle = d_msvs_doc_handle(at.doc, item);

        ok   = d_internal_set_filter(at.doc, item, filter);
        item = d_msvs_doc_element(at.doc, handle);
    }

    free(include);

    if (!ok)
    {
        return d_internal_fail(host, _out, d_msvs_doc_problem(at.doc));
    }

    return d_internal_give_out(host, _out, at.file, item);
}


/*
vcxproj_remove_item(e, type, include)
  1, 0 for none, or -1.
*/
static bool
d_internal_vcxproj_remove_item(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;
    char                         type[256];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    (void)snprintf(type, sizeof(type), "%s", d_internal_text(_args[1]));

    if (!d_internal_find(host, _args[0], &at))
    {
        return d_internal_fail(host, _out, "no such element");
    }

    const size_t item = d_internal_item(host, at.records, type,
                                        d_internal_text(_args[2]));

    // an item not there is not removed, and that is no failure
    if (item == D_MSVS_NONE)
    {
        d_awk_value_set_number(_out, 0.0);

        return true;
    }

    if (!d_msvs_doc_remove(at.doc, item))
    {
        return d_internal_fail(host, _out, d_msvs_doc_problem(at.doc));
    }

    d_awk_value_set_number(_out, 1.0);

    return true;
}


//==============================================================================
// 7.  CHANGES
//==============================================================================


/*
d_internal_add_element
  vcxproj_add and vcxproj_insert: an element made of the arguments after
the handle -- its name, its text, and attributes in pairs -- beneath a
parent, before a sibling or last.
*/
static bool
d_internal_add_element(
    struct d_host_vcxproj* _host,
    struct d_awk_value**   _args,
    size_t                 _count,
    struct d_awk_value*    _out,
    bool                   _before
)
{
    struct d_internal_at at;
    char*                texts[3u + (2u * D_INTERNAL_MOST_ATTRIBUTES)];
    bool                 ok = true;

    // the handle must name an element, and the attributes come in pairs
    if (!d_internal_find(_host, _args[0], &at))
    {
        return d_internal_fail(_host, _out, "no such element");
    }

    // attributes come as names and values in pairs
    if ( (_count > 3u) &&
         ( ((_count - 3u) % 2u) != 0u ) )
    {
        return d_internal_fail(_host, _out, "an attribute with no value");
    }

    // the root has no parent to hold a sibling
    if ( (_before) &&
         (at.records->elements[at.element].parent == D_MSVS_NONE) )
    {
        return d_internal_fail(_host, _out, "the root has no siblings");
    }

    // each argument's text is copied, as each is borrowed only until the next
    for (size_t at_arg = 1u; at_arg < _count; ++at_arg)
    {
        const char* const text   = d_internal_text(_args[at_arg]);
        const size_t      length = strlen(text);

        texts[at_arg] = malloc(length + 1u);
        ok            = ok && (texts[at_arg] != NULL);

        if (texts[at_arg])
        {
            memcpy(texts[at_arg], text, length + 1u);
        }
    }

    const struct d_msvs_new_element element =
    {
        (_count > 1u) ? texts[1] : "",
        ( (_count > 2u) && (texts[2]) && (texts[2][0] != '\0') ) ? texts[2]
                                                                 : NULL,
        (const char* const*)&texts[3],
        (_count > 3u) ? ((_count - 3u) / 2u) : 0u
    };
    const size_t parent = (_before) ? at.records->elements[at.element].parent
                                    : at.element;
    const size_t made   = (ok)
                        ? d_msvs_doc_insert(at.doc, parent,
                                            (_before) ? at.element
                                                      : D_MSVS_NONE,
                                            &element)
                        : D_MSVS_NONE;

    // the copies are freed, whether or not the element was added
    for (size_t at_arg = 1u; at_arg < _count; ++at_arg)
    {
        free(texts[at_arg]);
    }

    // an element not added says why
    if (made == D_MSVS_NONE)
    {
        return d_internal_fail(_host, _out, (ok) ? d_msvs_doc_problem(at.doc)
                                                 : "out of memory");
    }

    return d_internal_give_out(_host, _out, at.file, made);
}


/*
vcxproj_add(e, name[, text[, attr, value]...]),
vcxproj_insert(e, name[, text[, attr, value]...])
  A last child, and a sibling before.
*/
static bool
d_internal_vcxproj_add(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    (void)_problem;
    (void)_capacity;

    return d_internal_add_element(_user, _args, _count, _out, false);
}


static bool
d_internal_vcxproj_insert(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    (void)_problem;
    (void)_capacity;

    return d_internal_add_element(_user, _args, _count, _out, true);
}


/*
d_internal_changed
  1 for a change kept, or -1 with the document's reason.
*/
static bool
d_internal_changed(
    struct d_host_vcxproj*   _host,
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
vcxproj_set_attr(e, name, value), vcxproj_remove_attr(e, name),
vcxproj_set_text(e, text), vcxproj_remove(e)
  The document's own changes.
*/
static bool
d_internal_vcxproj_set_attr(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;
    char                         name[256];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    (void)snprintf(name, sizeof(name), "%s", d_internal_text(_args[1]));

    if (!d_internal_find(host, _args[0], &at))
    {
        return d_internal_fail(host, _out, "no such element");
    }

    return d_internal_changed(host, _out, at.doc,
                              d_msvs_doc_set_attribute(
                                  at.doc, at.element, name,
                                  d_internal_text(_args[2])));
}


static bool
d_internal_vcxproj_remove_attr(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;
    size_t                       start  = 0;
    size_t                       length = 0;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    if (!d_internal_find(host, _args[0], &at))
    {
        return d_internal_fail(host, _out, "no such element");
    }

    // an attribute not there is not removed, and that is no failure
    if (!d_msvs_attribute(at.records, &at.records->elements[at.element],
                          d_internal_text(_args[1]), &start, &length))
    {
        d_awk_value_set_number(_out, 0.0);

        return true;
    }

    return d_internal_changed(host, _out, at.doc,
                              d_msvs_doc_remove_attribute(
                                  at.doc, at.element,
                                  d_internal_text(_args[1])));
}


static bool
d_internal_vcxproj_set_text(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    if (!d_internal_find(host, _args[0], &at))
    {
        return d_internal_fail(host, _out, "no such element");
    }

    return d_internal_changed(host, _out, at.doc,
                              d_msvs_doc_set_text(at.doc, at.element,
                                                  d_internal_text(_args[1])));
}


static bool
d_internal_vcxproj_remove(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_vcxproj* const host = _user;
    struct d_internal_at         at;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    if (!d_internal_find(host, _args[0], &at))
    {
        return d_internal_fail(host, _out, "no such element");
    }

    return d_internal_changed(host, _out, at.doc,
                              d_msvs_doc_remove(at.doc, at.element));
}


//==============================================================================
// 8.  THE EXTENSION
//==============================================================================


// D_INTERNAL_VCXPROJ_FUNCTIONS
//   constant: how many host functions the extension adds.
#define D_INTERNAL_VCXPROJ_FUNCTIONS 32u


/*
d_internal_functions
  The extension's host functions, as a program calls them.
*/
static const struct d_awk_host_function*
d_internal_functions(void)
{
    static const struct d_awk_host_function functions[
        D_INTERNAL_VCXPROJ_FUNCTIONS] =
    {
        { "vcxproj_open",        1u, 1u, d_internal_vcxproj_open        },
        { "vcxproj_new",         1u, 2u, d_internal_vcxproj_new         },
        { "vcxproj_save",        1u, 2u, d_internal_vcxproj_save        },
        { "vcxproj_close",       1u, 1u, d_internal_vcxproj_close       },
        { "vcxproj_error",       0u, 0u, d_internal_vcxproj_error       },
        { "vcxproj_path",        1u, 1u, d_internal_vcxproj_path        },
        { "vcxproj_kind",        1u, 1u, d_internal_vcxproj_kind        },
        { "vcxproj_changes",     1u, 1u, d_internal_vcxproj_changes     },
        { "vcxproj_root",        1u, 1u, d_internal_vcxproj_root        },
        { "vcxproj_parent",      1u, 1u, d_internal_vcxproj_parent      },
        { "vcxproj_first",       1u, 1u, d_internal_vcxproj_first       },
        { "vcxproj_next",        1u, 1u, d_internal_vcxproj_next        },
        { "vcxproj_children",    1u, 1u, d_internal_vcxproj_children    },
        { "vcxproj_find",        2u, 3u, d_internal_vcxproj_find        },
        { "vcxproj_name",        1u, 1u, d_internal_vcxproj_name        },
        { "vcxproj_line",        1u, 1u, d_internal_vcxproj_line        },
        { "vcxproj_depth",       1u, 1u, d_internal_vcxproj_depth       },
        { "vcxproj_attr",        2u, 2u, d_internal_vcxproj_attr        },
        { "vcxproj_has",         2u, 2u, d_internal_vcxproj_has         },
        { "vcxproj_text",        1u, 1u, d_internal_vcxproj_text        },
        { "vcxproj_condition",   1u, 1u, d_internal_vcxproj_condition   },
        { "vcxproj_applies",     2u, 2u, d_internal_vcxproj_applies     },
        { "vcxproj_property",    2u, 3u, d_internal_vcxproj_property    },
        { "vcxproj_item",        3u, 3u, d_internal_vcxproj_item        },
        { "vcxproj_add_item",    3u, 4u, d_internal_vcxproj_add_item    },
        { "vcxproj_remove_item", 3u, 3u, d_internal_vcxproj_remove_item },
        { "vcxproj_add",         2u, 3u + (2u * D_INTERNAL_MOST_ATTRIBUTES),
          d_internal_vcxproj_add                                        },
        { "vcxproj_insert",      2u, 3u + (2u * D_INTERNAL_MOST_ATTRIBUTES),
          d_internal_vcxproj_insert                                     },
        { "vcxproj_set_attr",    3u, 3u, d_internal_vcxproj_set_attr    },
        { "vcxproj_remove_attr", 2u, 2u, d_internal_vcxproj_remove_attr },
        { "vcxproj_set_text",    2u, 2u, d_internal_vcxproj_set_text    },
        { "vcxproj_remove",      1u, 1u, d_internal_vcxproj_remove      }
    };

    return functions;
}


/*
d_host_vcxproj_new
  An extension with no file open.
*/
struct d_host_vcxproj*
d_host_vcxproj_new(void)
{
    struct d_host_vcxproj* const host = malloc(sizeof(*host));

    // an extension that cannot be made is NULL
    if (host)
    {
        memset(host, 0, sizeof(*host));
    }

    return host;
}


/*
d_host_vcxproj_free
  Closes every file open, dropping unsaved changes.
*/
void
d_host_vcxproj_free(
    struct d_host_vcxproj* _host
)
{
    // freeing nothing is allowed
    if (!_host)
    {
        return;
    }

    for (size_t at = 0; at < _host->file_count; ++at)
    {
        d_msvs_doc_free(_host->files[at].doc);
        free(_host->files[at].globals);
    }

    free(_host->files);
    free(_host->handles);
    free(_host->scratch);
    free(_host);

    return;
}


/*
d_host_vcxproj_install
  The functions are added with the extension as their user pointer.
*/
bool
d_host_vcxproj_install(
    struct d_host_vcxproj* _host,
    struct d_awk_interp*   _interp
)
{
    // parameter validation first
    if ( (!_host) ||
         (!_interp) )
    {
        return false;
    }

    return d_awk_interp_add_functions(_interp, d_internal_functions(),
                                      D_INTERNAL_VCXPROJ_FUNCTIONS, _host);
}
