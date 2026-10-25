/*******************************************************************************
* djinterp [tools]                                                  dhost_wiki.c
*
* Definitions for the declarations in dhost_wiki.h.
*   Each page open is a dwiki document; its handle is its place in the list
* of pages, from 1, and a page closed keeps its place, empty, so a handle to
* it names nothing rather than another page. What a page holds is read from
* the document's records as they are now, each field by its name.
*   A directory's files are found with dawk's file system, a level at a
* time, names that begin with `.` passed over, and sorted byte by byte.
*
* path:      /src/djinterp/tools/dawk/ext/wiki/dhost_wiki.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/wiki/dhost_wiki.h"  // corresponding header
// std
#include <stdio.h>   // rename, snprintf
#include <stdlib.h>  // malloc, realloc, free, qsort, strtod
#include <string.h>  // memcpy, memset, strcmp, strlen, strncmp
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"                 // d_fs_names
#include "../../../../../../inc/djinterp/tools/dawk/dhost.h"               // d_awk_host_function
#include "../../../../../../inc/djinterp/tools/dawk/dvalue.h"              // d_awk_value_text
#include "../../../../../../inc/djinterp/tools/dawk/ext/wiki/dwiki.h"      // d_wiki
#include "../../../../../../inc/djinterp/tools/dawk/ext/wiki/dwiki_doc.h"  // d_wiki_doc


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Extension
//------------------------------------------------------------------------------
// 1.1.1
// d_host_wiki
//   struct: the pages, open or closed, and the last failure.
struct d_host_wiki
{
    struct d_wiki_doc** pages;
    size_t              page_count;
    size_t              page_capacity;
    char                error[512];
};

// 1.1.2
// d_internal_found
//   struct: the paths a walk has found, in an array that grows.
struct d_internal_found
{
    char** paths;
    size_t count;
    size_t capacity;
    bool   failed;
};


//==============================================================================
// 2.  HELPERS
//==============================================================================


/*
d_internal_text, d_internal_index
  An argument's text, and an argument as an index from 1: a value below 1,
or not a number, is 0.
*/
static const char*
d_internal_text(
    struct d_awk_value* _value
)
{
    return d_awk_value_text(_value, NULL, NULL);
}


static size_t
d_internal_index(
    struct d_awk_value* _value
)
{
    const double number = d_awk_value_number(_value);

    // NaN, and anything below 1, is 0
    if (!(number >= 1.0))
    {
        return 0u;
    }

    return (number < 4294967296.0) ? (size_t)number : 0u;
}


/*
d_internal_fail
  Gives -1 for a failure a program can recover from, wiki_error() saying
why.
*/
static bool
d_internal_fail(
    struct d_host_wiki* _host,
    struct d_awk_value* _out,
    const char*         _why
)
{
    // a reason already given stays; another is copied
    if (_why != _host->error)
    {
        (void)snprintf(_host->error, sizeof(_host->error), "%s",
                       ( (_why) && (_why[0] != '\0') ) ? _why : "failed");
    }
    else if (_host->error[0] == '\0')
    {
        (void)snprintf(_host->error, sizeof(_host->error), "failed");
    }

    d_awk_value_set_number(_out, -1.0);

    return true;
}


/*
d_internal_set_text, d_internal_set_span, d_internal_set_buffer
  Gives text: a string's, a span of a page's, a buffer's, which is freed.
*/
static bool
d_internal_set_text(
    struct d_awk_value* _out,
    const char*         _text
)
{
    return d_awk_value_set_string(_out, (_text) ? _text : "",
                                  (_text) ? strlen(_text) : 0u);
}


static bool
d_internal_set_span(
    struct d_awk_value*      _out,
    const struct d_wiki_doc* _doc,
    size_t                   _start,
    size_t                   _end
)
{
    size_t            length = 0;
    const char* const text   = d_wiki_doc_text(_doc, &length);

    // a span not there is nothing
    if ( (_start == D_WIKI_NONE) ||
         (_end < _start) ||
         (_end > length) )
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    return d_awk_value_set_string(_out, &text[_start], _end - _start);
}


static bool
d_internal_set_buffer(
    struct d_host_wiki*   _host,
    struct d_awk_value*   _out,
    struct d_wiki_buffer* _buffer,
    bool                  _ok
)
{
    // a writer that could not write
    if ( (!_ok) ||
         (_buffer->failed) )
    {
        d_wiki_buffer_free(_buffer);

        return d_internal_fail(_host, _out, "the text cannot be written so");
    }

    const bool set = d_awk_value_set_string(_out,
                                            (_buffer->text) ? _buffer->text
                                                            : "",
                                            _buffer->length);

    d_wiki_buffer_free(_buffer);

    return set;
}


/*
d_internal_page
  The page a handle names, or NULL, wiki_error() saying why.
*/
static struct d_wiki_doc*
d_internal_page(
    struct d_host_wiki* _host,
    struct d_awk_value* _handle
)
{
    const size_t handle = d_internal_index(_handle);

    // a handle in the list, to a page not closed
    if ( (handle == 0u) ||
         (handle > _host->page_count) ||
         (!_host->pages[handle - 1u]) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "no page has the handle %s", d_internal_text(_handle));

        return NULL;
    }

    return _host->pages[handle - 1u];
}


/*
d_internal_dialect
  A dialect, by its name or by a page's handle; D_WIKI_OTHER for neither.
*/
static enum d_wiki_dialect
d_internal_dialect(
    struct d_host_wiki* _host,
    struct d_awk_value* _value
)
{
    const char* const         name    = d_internal_text(_value);
    const enum d_wiki_dialect dialect = d_wiki_dialect_named(name);

    // a name first, then a handle
    if (dialect != D_WIKI_OTHER)
    {
        return dialect;
    }

    const size_t handle = d_internal_index(_value);

    // else a page's handle, open
    if ( (handle > 0u) &&
         (handle <= _host->page_count) &&
         (_host->pages[handle - 1u]) )
    {
        return d_wiki_doc_dialect(_host->pages[handle - 1u]);
    }

    (void)snprintf(_host->error, sizeof(_host->error),
                   "%s is not a dialect", name);

    return D_WIKI_OTHER;
}


/*
d_internal_copy_arg
  An argument's text, copied, as the next argument's may be read over it.
*/
static char*
d_internal_copy_arg(
    struct d_awk_value* _value
)
{
    size_t            length = 0;
    const char* const text   = d_awk_value_text(_value, NULL, &length);

    return d_wiki_copy(text, length);
}


/*
d_internal_same
  Whether a field's name is the one given.
*/
static bool
d_internal_same(
    const char* _field,
    const char* _name
)
{
    return (strcmp(_field, _name) == 0);
}


/*
d_internal_unknown_field
  Gives "" for a field not known, wiki_error() saying so.
*/
static bool
d_internal_unknown_field(
    struct d_host_wiki* _host,
    struct d_awk_value* _out,
    const char*         _field
)
{
    (void)snprintf(_host->error, sizeof(_host->error), "no field %s",
                   _field);

    return d_awk_value_set_string(_out, "", 0u);
}


/*
d_internal_changed
  Gives 1 for a change kept, or -1 for one refused, the document saying
why.
*/
static bool
d_internal_changed(
    struct d_host_wiki*      _host,
    struct d_awk_value*      _out,
    const struct d_wiki_doc* _doc,
    bool                     _ok
)
{
    // refused: the document says why
    if (!_ok)
    {
        return d_internal_fail(_host, _out, d_wiki_doc_problem(_doc));
    }

    d_awk_value_set_number(_out, 1.0);

    return true;
}


//==============================================================================
// 3.  PAGES
//==============================================================================


/*
d_internal_add_page
  Keeps a page, and gives its handle.
*/
static bool
d_internal_add_page(
    struct d_host_wiki* _host,
    struct d_awk_value* _out,
    struct d_wiki_doc*  _doc
)
{
    // room for one more
    if (_host->page_count >= _host->page_capacity)
    {
        const size_t        wanted = (_host->page_capacity > 0u)
                                         ? (_host->page_capacity * 2u) : 8u;
        struct d_wiki_doc** grown  = realloc(_host->pages,
                                             wanted * sizeof(*grown));

        // a page that cannot be kept is freed, and the handle not given
        if (!grown)
        {
            d_wiki_doc_free(_doc);

            return d_internal_fail(_host, _out, "out of memory");
        }

        _host->pages         = grown;
        _host->page_capacity = wanted;
    }

    _host->pages[_host->page_count++] = _doc;
    d_awk_value_set_number(_out, (double)_host->page_count);

    return true;
}


/*
wiki_open(path[, dialect]), wiki_new(path[, dialect[, text]])
  A page from its file, or of a text.
*/
static bool
d_internal_wiki_open(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host = _user;
    char* const               path = d_internal_copy_arg(_args[0]);
    enum d_wiki_dialect       dialect = D_WIKI_OTHER;
    char                      why[512];

    (void)_problem;
    (void)_capacity;

    // a path that could not be copied
    if (!path)
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    // a dialect named, else the file's
    if (_count > 1u)
    {
        dialect = d_wiki_dialect_named(d_internal_text(_args[1]));

        // a dialect named that none is
        if (dialect == D_WIKI_OTHER)
        {
            free(path);

            return d_internal_fail(host, _out, "no such dialect");
        }
    }

    struct d_wiki_doc* const doc = d_wiki_doc_read(path, dialect, why,
                                                   sizeof(why));

    free(path);

    // why it did not read
    if (!doc)
    {
        return d_internal_fail(host, _out, why);
    }

    return d_internal_add_page(host, _out, doc);
}


static bool
d_internal_wiki_new(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host    = _user;
    char* const               path    = d_internal_copy_arg(_args[0]);
    enum d_wiki_dialect       dialect = D_WIKI_OTHER;
    char*                     text    = NULL;
    char                      why[512];

    (void)_problem;
    (void)_capacity;

    // a dialect named, and a text, perhaps
    if ( (_count > 1u) &&
         (d_internal_text(_args[1])[0] != '\0') )
    {
        dialect = d_wiki_dialect_named(d_internal_text(_args[1]));

        // a dialect named that none is
        if (dialect == D_WIKI_OTHER)
        {
            free(path);

            return d_internal_fail(host, _out, "no such dialect");
        }
    }

    // the page's text, when it is given
    if (_count > 2u)
    {
        text = d_internal_copy_arg(_args[2]);
    }

    struct d_wiki_doc* const doc = (path)
                                       ? d_wiki_doc_make(path, (text) ? text
                                                                      : "",
                                                         (text) ? strlen(text)
                                                                : 0u,
                                                         dialect, why,
                                                         sizeof(why))
                                       : NULL;

    free(path);
    free(text);

    // why it could not be made
    if (!doc)
    {
        return d_internal_fail(host, _out, (path) ? why : "out of memory");
    }

    return d_internal_add_page(host, _out, doc);
}


/*
wiki_save(p[, path]), wiki_close(p), wiki_error()
*/
static bool
d_internal_wiki_save(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host = _user;
    struct d_wiki_doc* const  doc  = d_internal_page(host, _args[0]);
    char                      why[512];

    (void)_problem;
    (void)_capacity;

    // a handle to no page
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const enum d_wiki_saved saved = d_wiki_doc_save(
                                        doc,
                                        (_count > 1u)
                                            ? d_internal_text(_args[1])
                                            : NULL,
                                        why, sizeof(why));

    // why it could not be written
    if (saved == D_WIKI_SAVE_FAILED)
    {
        return d_internal_fail(host, _out, why);
    }

    d_awk_value_set_number(_out, (saved == D_WIKI_SAVE_WRITTEN) ? 1.0 : 0.0);

    return true;
}


static bool
d_internal_wiki_close(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host   = _user;
    const size_t              handle = d_internal_index(_args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a page open, freed, its place kept
    if ( (handle == 0u) ||
         (handle > host->page_count) ||
         (!host->pages[handle - 1u]) )
    {
        d_awk_value_set_number(_out, 0.0);

        return true;
    }

    d_wiki_doc_free(host->pages[handle - 1u]);
    host->pages[handle - 1u] = NULL;
    d_awk_value_set_number(_out, 1.0);

    return true;
}


static bool
d_internal_wiki_error(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host = _user;

    (void)_args;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_set_text(_out, host->error);
}


/*
wiki_path(p), wiki_dialect(p), wiki_text(p), wiki_changes(p), wiki_page(p)
  What a page is, by the field each names.
*/
static bool
d_internal_page_field(
    struct d_host_wiki* _host,
    struct d_awk_value* _handle,
    struct d_awk_value* _out,
    const char*         _field
)
{
    const struct d_wiki_doc* const doc = d_internal_page(_host, _handle);

    // a page open
    if (!doc)
    {
        return d_internal_fail(_host, _out, _host->error);
    }

    // the file it was read from, or is to be saved to
    if (d_internal_same(_field, "path"))
    {
        return d_internal_set_text(_out, d_wiki_doc_path(doc));
    }

    // its dialect's name
    if (d_internal_same(_field, "dialect"))
    {
        const enum d_wiki_dialect dialect = d_wiki_doc_dialect(doc);

        return d_internal_set_text(_out, d_wiki_dialect_name(dialect));
    }

    // its text as it is now
    if (d_internal_same(_field, "text"))
    {
        size_t            length = 0;
        const char* const text   = d_wiki_doc_text(doc, &length);

        return d_awk_value_set_string(_out, text, length);
    }

    // how many changes it has taken
    if (d_internal_same(_field, "changes"))
    {
        d_awk_value_set_number(_out, (double)d_wiki_doc_changes(doc));

        return true;
    }

    char* const name = d_wiki_page_name(d_wiki_doc_path(doc),
                                        d_wiki_doc_dialect(doc));
    const bool  ok   = d_internal_set_text(_out, name);

    free(name);

    return ok;
}


static bool
d_internal_wiki_path(
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

    return d_internal_page_field(_user, _args[0], _out, "path");
}


static bool
d_internal_wiki_dialect(
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

    return d_internal_page_field(_user, _args[0], _out, "dialect");
}


static bool
d_internal_wiki_text(
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

    return d_internal_page_field(_user, _args[0], _out, "text");
}


static bool
d_internal_wiki_changes(
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

    return d_internal_page_field(_user, _args[0], _out, "changes");
}


static bool
d_internal_wiki_page(
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

    return d_internal_page_field(_user, _args[0], _out, "page");
}


//==============================================================================
// 4.  RECORDS
//==============================================================================


// d_internal_counted
//   enum: what a page's records are counted by.
enum d_internal_counted
{
    D_INTERNAL_BLOCKS = 0,
    D_INTERNAL_SECTIONS,
    D_INTERNAL_HEADINGS,
    D_INTERNAL_LINKS,
    D_INTERNAL_TEMPLATES,
    D_INTERNAL_REGIONS,
    D_INTERNAL_PROBLEMS
};


/*
d_internal_count_of
  How many of a kind of record a page has; its headed sections, the lead
not among them.
*/
static size_t
d_internal_count_of(
    const struct d_wiki*    _wiki,
    enum d_internal_counted _kind
)
{
    // a table of the counts, by kind
    switch (_kind)
    {
        case D_INTERNAL_BLOCKS:    return _wiki->block_count;
        case D_INTERNAL_SECTIONS:  return _wiki->section_count - 1u;
        case D_INTERNAL_HEADINGS:  return _wiki->heading_count;
        case D_INTERNAL_LINKS:     return _wiki->link_count;
        case D_INTERNAL_TEMPLATES: return _wiki->template_count;
        case D_INTERNAL_REGIONS:   return _wiki->region_count;
        default:                   return _wiki->problem_count;
    }
}


static bool
d_internal_count(
    struct d_host_wiki*     _host,
    struct d_awk_value*     _handle,
    struct d_awk_value*     _out,
    enum d_internal_counted _kind
)
{
    const struct d_wiki_doc* const doc = d_internal_page(_host, _handle);

    // a page open
    if (!doc)
    {
        return d_internal_fail(_host, _out, _host->error);
    }

    d_awk_value_set_number(_out,
                           (double)d_internal_count_of(d_wiki_doc_records(doc),
                                                       _kind));

    return true;
}


static bool
d_internal_wiki_blocks(
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

    return d_internal_count(_user, _args[0], _out, D_INTERNAL_BLOCKS);
}


static bool
d_internal_wiki_sections(
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

    return d_internal_count(_user, _args[0], _out, D_INTERNAL_SECTIONS);
}


static bool
d_internal_wiki_headings(
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

    return d_internal_count(_user, _args[0], _out, D_INTERNAL_HEADINGS);
}


static bool
d_internal_wiki_links(
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

    return d_internal_count(_user, _args[0], _out, D_INTERNAL_LINKS);
}


static bool
d_internal_wiki_templates(
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

    return d_internal_count(_user, _args[0], _out, D_INTERNAL_TEMPLATES);
}


static bool
d_internal_wiki_regions(
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

    return d_internal_count(_user, _args[0], _out, D_INTERNAL_REGIONS);
}


static bool
d_internal_wiki_problems(
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

    return d_internal_count(_user, _args[0], _out, D_INTERNAL_PROBLEMS);
}


/*
d_internal_record
  The page and the index of one of its records, from 1 -- from 0 for a
section -- or NULL, wiki_error() saying why.
*/
static const struct d_wiki_doc*
d_internal_record(
    struct d_host_wiki*     _host,
    struct d_awk_value*     _handle,
    struct d_awk_value*     _index,
    enum d_internal_counted _kind,
    size_t*                 _out_index
)
{
    const struct d_wiki_doc* const doc = d_internal_page(_host, _handle);

    // a page open, and a record it has
    if (!doc)
    {
        return NULL;
    }

    static const char* const names[] =
    {
        "block", "section", "heading", "link", "template", "region",
        "problem"
    };
    const double number = d_awk_value_number(_index);
    const size_t count  = d_internal_count_of(d_wiki_doc_records(doc), _kind);
    const size_t first  = (_kind == D_INTERNAL_SECTIONS) ? 0u : 1u;

    // an index past those the page has, or before its first
    if ( (!(number >= (double)first)) ||
         (number > (double)count) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "the page has no %s %s", names[_kind],
                       d_internal_text(_index));

        return NULL;
    }

    *_out_index = (size_t)number - first;

    return doc;
}


/*
wiki_problem(p, i, field)
*/
static bool
d_internal_wiki_problem(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host  = _user;
    size_t                    index = 0;
    const struct d_wiki_doc*  doc   = d_internal_record(host, _args[0],
                                                        _args[1],
                                                        D_INTERNAL_PROBLEMS,
                                                        &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_wiki_problem* const problem =
        &d_wiki_doc_records(doc)->problems[index];
    const char* const                  field   = d_internal_text(_args[2]);

    // its line
    if (d_internal_same(field, "line"))
    {
        d_awk_value_set_number(_out, (double)problem->line);

        return true;
    }

    // what it is
    if (d_internal_same(field, "what"))
    {
        return d_internal_set_text(_out, problem->what);
    }

    return d_internal_unknown_field(host, _out, field);
}


/*
d_internal_block_kind_name, d_internal_form_name, d_internal_target_name,
d_internal_template_kind_name
  The names a program reads kinds by.
*/
static const char*
d_internal_block_kind_name(
    enum d_wiki_block_kind _kind
)
{
    static const char* const names[] =
    {
        "paragraph", "heading", "code", "preformatted", "quote", "item",
        "table", "rule", "html", "comment", "definition", "metadata"
    };

    return ((size_t)_kind < (sizeof(names) / sizeof(names[0])))
               ? names[_kind] : "other";
}


static const char*
d_internal_form_name(
    enum d_wiki_form _form
)
{
    static const char* const names[] =
    {
        "wiki", "inline", "reference", "auto", "bare", "external",
        "definition", "html", "option", "gallery"
    };

    return ((size_t)_form < (sizeof(names) / sizeof(names[0])))
               ? names[_form] : "other";
}


static const char*
d_internal_target_name(
    enum d_wiki_target _target
)
{
    static const char* const names[] =
    {
        "page", "anchor", "url", "mail", "file", "image", "category", "tag"
    };

    return ((size_t)_target < (sizeof(names) / sizeof(names[0])))
               ? names[_target] : "other";
}


static const char*
d_internal_template_kind_name(
    enum d_wiki_template_kind _kind
)
{
    static const char* const names[] =
    {
        "template", "function", "variable", "parameter"
    };

    return ((size_t)_kind < (sizeof(names) / sizeof(names[0])))
               ? names[_kind] : "other";
}


/*
d_internal_number_or_none
  Gives a number from 1 for an index, 0 for none.
*/
static bool
d_internal_number_or_none(
    struct d_awk_value* _out,
    size_t              _index
)
{
    d_awk_value_set_number(_out, (_index == D_WIKI_NONE)
                                     ? 0.0 : (double)(_index + 1u));

    return true;
}


/*
wiki_block(p, i, field)
*/
static bool
d_internal_wiki_block(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host  = _user;
    size_t                    index = 0;
    const struct d_wiki_doc*  doc   = d_internal_record(host, _args[0],
                                                        _args[1],
                                                        D_INTERNAL_BLOCKS,
                                                        &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page, or an index to no block
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_wiki* const       wiki  = d_wiki_doc_records(doc);
    const struct d_wiki_block* const block = &wiki->blocks[index];
    const char* const                field = d_internal_text(_args[2]);
    char                             marker[2] = { block->marker, '\0' };

    // a table of the fields
    if (d_internal_same(field, "kind"))
    {
        return d_internal_set_text(_out, d_internal_block_kind_name(
                                             block->kind));
    }

    // the line it begins on
    if (d_internal_same(field, "line"))
    {
        d_awk_value_set_number(_out, (double)block->line);

        return true;
    }

    // the line it ends on
    if (d_internal_same(field, "end_line"))
    {
        d_awk_value_set_number(_out, (double)block->end_line);

        return true;
    }

    // how many containers hold it
    if (d_internal_same(field, "depth"))
    {
        d_awk_value_set_number(_out, (double)block->depth);

        return true;
    }

    // a heading's level, an item's depth among lists
    if (d_internal_same(field, "level"))
    {
        d_awk_value_set_number(_out, (double)block->level);

        return true;
    }

    // an item's marker, or a fence's
    if (d_internal_same(field, "marker"))
    {
        return d_internal_set_text(_out, marker);
    }

    // a code block's language
    if (d_internal_same(field, "language"))
    {
        return d_internal_set_span(_out, doc, block->info_start,
                                   block->info_end);
    }

    // the container that holds it
    if (d_internal_same(field, "parent"))
    {
        return d_internal_number_or_none(_out, block->parent);
    }

    // its text, as written
    if (d_internal_same(field, "text"))
    {
        return d_internal_set_span(_out, doc, block->start, block->end);
    }

    // its content: its lines without their markers
    if (d_internal_same(field, "content"))
    {
        struct d_wiki_buffer content = { NULL, 0u, 0u, false };

        return d_internal_set_buffer(host, _out, &content,
                                     d_wiki_content(wiki, index, &content));
    }

    return d_internal_unknown_field(host, _out, field);
}


/*
d_internal_section_path
  A section's path: its title, after the titles of those that hold it,
apart by `/`.
*/
static bool
d_internal_section_path(
    const struct d_wiki*  _wiki,
    size_t                _section,
    struct d_wiki_buffer* _out
)
{
    const struct d_wiki_section* const section = &_wiki->sections[_section];

    // the lead has no title
    if (section->heading == D_WIKI_NONE)
    {
        return d_wiki_buffer_add(_out, "", 0u);
    }

    // its parent's path, then its own title
    if (section->parent != D_WIKI_NONE)
    {
        (void)d_internal_section_path(_wiki, section->parent, _out);
        (void)d_wiki_buffer_add_char(_out, '/');
    }

    return d_wiki_buffer_add_text(_out, _wiki->headings[section->heading]
                                            .title);
}


/*
wiki_section(p, i, field)
*/
static bool
d_internal_wiki_section(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host  = _user;
    size_t                    index = 0;
    const struct d_wiki_doc*  doc   = d_internal_record(host, _args[0],
                                                        _args[1],
                                                        D_INTERNAL_SECTIONS,
                                                        &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page, or an index to no section
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_wiki* const         wiki    = d_wiki_doc_records(doc);
    const struct d_wiki_section* const section = &wiki->sections[index];
    const struct d_wiki_heading* const heading =
        (section->heading != D_WIKI_NONE) ? &wiki->headings[section->heading]
                                          : NULL;
    const char* const                  field   = d_internal_text(_args[2]);

    // a table of the fields
    if (d_internal_same(field, "title"))
    {
        return d_internal_set_text(_out, (heading) ? heading->title : "");
    }

    // its title's anchor
    if (d_internal_same(field, "anchor"))
    {
        return d_internal_set_text(_out, (heading) ? heading->anchor : "");
    }

    // its level, and the lines it runs over
    if ( (d_internal_same(field, "level")) ||
         (d_internal_same(field, "line")) ||
         (d_internal_same(field, "end_line")) )
    {
        const size_t line = d_wiki_line_of(wiki, section->start, NULL);
        const size_t last = d_wiki_line_of(wiki, (section->end >
                                                  section->start)
                                                     ? (section->end - 1u)
                                                     : section->start, NULL);

        d_awk_value_set_number(_out, (d_internal_same(field, "level"))
                                         ? (double)section->level
                                     : (d_internal_same(field, "line"))
                                         ? (double)line : (double)last);

        return true;
    }

    // the section it is within, 0 for the page
    if (d_internal_same(field, "parent"))
    {
        d_awk_value_set_number(_out, (section->parent == D_WIKI_NONE)
                                         ? 0.0 : (double)section->parent);

        return true;
    }

    // how many sections are within it
    if (d_internal_same(field, "subsections"))
    {
        size_t count = 0;

        // those after it that begin before it ends
        for (size_t at = index + 1u; ( (at < wiki->section_count) &&
                                       (wiki->sections[at].start <
                                            section->end) ); ++at)
        {
            count++;
        }

        d_awk_value_set_number(_out, (double)count);

        return true;
    }

    // its titles, from the page's top, apart by `/`
    if (d_internal_same(field, "path"))
    {
        struct d_wiki_buffer path = { NULL, 0u, 0u, false };

        return d_internal_set_buffer(host, _out, &path,
                                     d_internal_section_path(wiki, index,
                                                             &path));
    }

    // all of it, heading and subsections
    if (d_internal_same(field, "text"))
    {
        return d_internal_set_span(_out, doc, section->start, section->end);
    }

    // its own text, past its heading
    if (d_internal_same(field, "body"))
    {
        return d_internal_set_span(_out, doc, section->body,
                                   section->own_end);
    }

    // its heading's line
    if (d_internal_same(field, "heading"))
    {
        return d_internal_set_span(_out, doc, section->start, section->body);
    }

    return d_internal_unknown_field(host, _out, field);
}


/*
wiki_find_section(p, name[, after])
*/
static bool
d_internal_wiki_find_section(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const      host = _user;
    const struct d_wiki_doc* const doc  = d_internal_page(host, _args[0]);
    char* const                    name = d_internal_copy_arg(_args[1]);
    const size_t                   after = (_count > 2u)
                                               ? d_internal_index(_args[2])
                                               : 0u;

    (void)_problem;
    (void)_capacity;

    // a handle to no page, or a name that could not be copied
    if ( (!doc) ||
         (!name) )
    {
        free(name);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const struct d_wiki* const wiki  = d_wiki_doc_records(doc);
    size_t                     found = 0;

    // each section after the one given, by title, anchor or path
    for (size_t index = after + 1u; ( (found == 0u) &&
                                      (index < wiki->section_count) ); ++index)
    {
        const struct d_wiki_heading* const heading =
            &wiki->headings[wiki->sections[index].heading];
        struct d_wiki_buffer               path    = { NULL, 0u, 0u, false };

        // its title, its anchor, or its path
        if ( (strcmp(heading->title, name) == 0) ||
             (strcmp(heading->anchor, name) == 0) ||
             ( (d_internal_section_path(wiki, index, &path)) &&
               (path.text) &&
               (strcmp(path.text, name) == 0) ) )
        {
            found = index;
        }

        d_wiki_buffer_free(&path);
    }

    free(name);
    d_awk_value_set_number(_out, (double)found);

    return true;
}


/*
wiki_heading(p, i, field)
*/
static bool
d_internal_wiki_heading(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host  = _user;
    size_t                    index = 0;
    const struct d_wiki_doc*  doc   = d_internal_record(host, _args[0],
                                                        _args[1],
                                                        D_INTERNAL_HEADINGS,
                                                        &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page, or an index to no heading
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_wiki_heading* const heading =
        &d_wiki_doc_records(doc)->headings[index];
    const char* const                  field   = d_internal_text(_args[2]);

    // a table of the fields
    if (d_internal_same(field, "title"))
    {
        return d_internal_set_text(_out, heading->title);
    }

    // its anchor
    if (d_internal_same(field, "anchor"))
    {
        return d_internal_set_text(_out, heading->anchor);
    }

    // its level, line, and whether it stands at the page's top
    if ( (d_internal_same(field, "level")) ||
         (d_internal_same(field, "line")) ||
         (d_internal_same(field, "top")) )
    {
        d_awk_value_set_number(_out, (d_internal_same(field, "level"))
                                         ? (double)heading->level
                                     : (d_internal_same(field, "line"))
                                         ? (double)heading->line
                                         : ( (heading->top) ? 1.0 : 0.0 ));

        return true;
    }

    return d_internal_unknown_field(host, _out, field);
}


/*
wiki_link(p, i, field)
*/
static bool
d_internal_wiki_link(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host  = _user;
    size_t                    index = 0;
    const struct d_wiki_doc*  doc   = d_internal_record(host, _args[0],
                                                        _args[1],
                                                        D_INTERNAL_LINKS,
                                                        &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page, or an index to no link
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_wiki_link* const link  =
        &d_wiki_doc_records(doc)->links[index];
    const char* const               field = d_internal_text(_args[2]);

    // a table of the fields
    if (d_internal_same(field, "form"))
    {
        return d_internal_set_text(_out, d_internal_form_name(link->form));
    }

    // what it leads to: a page, an anchor, a URL...
    if (d_internal_same(field, "kind"))
    {
        return d_internal_set_text(_out, d_internal_target_name(link->to));
    }

    // its destination, decoded
    if (d_internal_same(field, "target"))
    {
        return d_internal_set_text(_out, link->target);
    }

    // the page, file, image or category it names
    if (d_internal_same(field, "page"))
    {
        return d_internal_set_text(_out, link->page);
    }

    // what follows its `#`
    if (d_internal_same(field, "anchor"))
    {
        return d_internal_set_text(_out, link->anchor);
    }

    // the text it shows
    if (d_internal_same(field, "label"))
    {
        return d_internal_set_text(_out, link->label);
    }

    // the text it is written as
    if (d_internal_same(field, "text"))
    {
        return d_internal_set_span(_out, doc, link->start, link->end);
    }

    // its line, column, and whether it is shown in place
    if ( (d_internal_same(field, "line")) ||
         (d_internal_same(field, "column")) ||
         (d_internal_same(field, "embedded")) )
    {
        d_awk_value_set_number(_out, (d_internal_same(field, "line"))
                                         ? (double)link->line
                                     : (d_internal_same(field, "column"))
                                         ? (double)link->column
                                         : ( (link->embedded) ? 1.0 : 0.0 ));

        return true;
    }

    // a reference's definition
    if (d_internal_same(field, "definition"))
    {
        return d_internal_number_or_none(_out, link->definition);
    }

    // the template it is within
    if (d_internal_same(field, "template"))
    {
        return d_internal_number_or_none(_out, link->parent);
    }

    return d_internal_unknown_field(host, _out, field);
}


/*
wiki_find_link(p, name[, after])
*/
static bool
d_internal_wiki_find_link(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const      host  = _user;
    const struct d_wiki_doc* const doc   = d_internal_page(host, _args[0]);
    char* const                    name  = d_internal_copy_arg(_args[1]);
    const size_t                   after = (_count > 2u)
                                               ? d_internal_index(_args[2])
                                               : 0u;

    (void)_problem;
    (void)_capacity;

    // a handle to no page, or a name that could not be copied
    if ( (!doc) ||
         (!name) )
    {
        free(name);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const struct d_wiki* const wiki  = d_wiki_doc_records(doc);
    size_t                     found = 0;

    // each link after the one given, to the page, or of the target
    for (size_t index = after; ( (found == 0u) &&
                                 (index < wiki->link_count) ); ++index)
    {
        const struct d_wiki_link* const link = &wiki->links[index];

        // its destination as written, or the page it names
        if ( (strcmp(link->target, name) == 0) ||
             ( ( (link->to == D_WIKI_TO_PAGE) ||
                 (link->to == D_WIKI_TO_CATEGORY) ||
                 (link->to == D_WIKI_TO_FILE) ||
                 (link->to == D_WIKI_TO_IMAGE) ) &&
               (link->page[0] != '\0') &&
               (d_wiki_same_page(link->page, name, wiki->dialect)) ) )
        {
            found = index + 1u;
        }
    }

    free(name);
    d_awk_value_set_number(_out, (double)found);

    return true;
}


/*
wiki_template(p, i, field)
*/
static bool
d_internal_wiki_template(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host  = _user;
    size_t                    index = 0;
    const struct d_wiki_doc*  doc   = d_internal_record(host, _args[0],
                                                        _args[1],
                                                        D_INTERNAL_TEMPLATES,
                                                        &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page, or an index to no template
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_wiki_template* const tmpl  =
        &d_wiki_doc_records(doc)->templates[index];
    const char* const                   field = d_internal_text(_args[2]);

    // a table of the fields
    if (d_internal_same(field, "name"))
    {
        return d_internal_set_text(_out, tmpl->name);
    }

    // a template, a parser function, a variable or a parameter
    if (d_internal_same(field, "kind"))
    {
        return d_internal_set_text(_out,
                                   d_internal_template_kind_name(tmpl->kind));
    }

    // the text it is written as
    if (d_internal_same(field, "text"))
    {
        return d_internal_set_span(_out, doc, tmpl->start, tmpl->end);
    }

    // its line, its column, and how many arguments it has
    if ( (d_internal_same(field, "line")) ||
         (d_internal_same(field, "column")) ||
         (d_internal_same(field, "args")) )
    {
        d_awk_value_set_number(_out, (d_internal_same(field, "line"))
                                         ? (double)tmpl->line
                                     : (d_internal_same(field, "column"))
                                         ? (double)tmpl->column
                                         : (double)tmpl->argument_count);

        return true;
    }

    // the template it is within
    if (d_internal_same(field, "parent"))
    {
        return d_internal_number_or_none(_out, tmpl->parent);
    }

    return d_internal_unknown_field(host, _out, field);
}


/*
wiki_find_template(p, name[, after])
*/
static bool
d_internal_wiki_find_template(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const      host  = _user;
    const struct d_wiki_doc* const doc   = d_internal_page(host, _args[0]);
    char* const                    name  = d_internal_copy_arg(_args[1]);
    const size_t                   after = (_count > 2u)
                                               ? d_internal_index(_args[2])
                                               : 0u;

    (void)_problem;
    (void)_capacity;

    // a handle to no page, or a name that could not be copied
    if ( (!doc) ||
         (!name) )
    {
        free(name);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const struct d_wiki* const wiki  = d_wiki_doc_records(doc);
    size_t                     found = 0;

    // each template after the one given, its name as MediaWiki compares it
    for (size_t index = after; ( (found == 0u) &&
                                 (index < wiki->template_count) ); ++index)
    {
        // named as templates' names compare
        if (d_wiki_same_page(wiki->templates[index].name, name,
                             D_WIKI_MEDIAWIKI))
        {
            found = index + 1u;
        }
    }

    free(name);
    d_awk_value_set_number(_out, (double)found);

    return true;
}


/*
d_internal_argument
  The last argument of a template of a key, D_WIKI_NONE for none.
*/
static size_t
d_internal_argument(
    const struct d_wiki* _wiki,
    size_t               _template,
    const char*          _key
)
{
    const struct d_wiki_template* const tmpl  = &_wiki->templates[_template];
    size_t                              found = D_WIKI_NONE;

    // the last of the key
    for (size_t index = 0; index < tmpl->argument_count; ++index)
    {
        // the argument of the key
        if (strcmp(_wiki->arguments[tmpl->first_argument + index].key,
                   _key) == 0)
        {
            found = tmpl->first_argument + index;
        }
    }

    return found;
}


/*
wiki_arg(p, t, key), wiki_has_arg(p, t, key), wiki_arg_key(p, t, n)
*/
static bool
d_internal_arg_common(
    struct d_host_wiki*  _host,
    struct d_awk_value** _args,
    struct d_awk_value*  _out,
    int                  _which
)
{
    size_t                   index = 0;
    const struct d_wiki_doc* doc   = d_internal_record(_host, _args[0],
                                                       _args[1],
                                                       D_INTERNAL_TEMPLATES,
                                                       &index);

    // a handle to no page, or an index to no template
    if (!doc)
    {
        return d_internal_fail(_host, _out, _host->error);
    }

    const struct d_wiki* const wiki = d_wiki_doc_records(doc);

    // the n-th argument's key
    if (_which == 2)
    {
        const size_t n = d_internal_index(_args[2]);

        // an argument's number past those it has
        if ( (n == 0u) ||
             (n > wiki->templates[index].argument_count) )
        {
            return d_awk_value_set_string(_out, "", 0u);
        }

        return d_internal_set_text(_out,
                                   wiki->arguments[wiki->templates[index]
                                                       .first_argument +
                                                   n - 1u].key);
    }

    const size_t argument = d_internal_argument(wiki, index,
                                                d_internal_text(_args[2]));

    // whether it is there
    if (_which == 1)
    {
        d_awk_value_set_number(_out, (argument != D_WIKI_NONE) ? 1.0 : 0.0);

        return true;
    }

    // no argument of the key
    if (argument == D_WIKI_NONE)
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const struct d_wiki_argument* const given = &wiki->arguments[argument];
    size_t                              start = given->value_start;
    size_t                              end   = given->value_end;
    size_t                              length = 0;
    const char* const                   text  = d_wiki_doc_text(doc, &length);

    // a named argument's value is trimmed
    if (given->name_start != D_WIKI_NONE)
    {
        // a named argument's value is trimmed: past the space before it
        while ( (start < end) &&
                (strchr(" \t\r\n", text[start]) != NULL) )
        {
            start++;
        }

        // and before the space after it
        while ( (end > start) &&
                (strchr(" \t\r\n", text[end - 1u]) != NULL) )
        {
            end--;
        }
    }

    return d_internal_set_span(_out, doc, start, end);
}


static bool
d_internal_wiki_arg(
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

    return d_internal_arg_common(_user, _args, _out, 0);
}


static bool
d_internal_wiki_has_arg(
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

    return d_internal_arg_common(_user, _args, _out, 1);
}


static bool
d_internal_wiki_arg_key(
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

    return d_internal_arg_common(_user, _args, _out, 2);
}


/*
wiki_region(p, i, field), wiki_find_region(p, name)
*/
static bool
d_internal_wiki_region(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host  = _user;
    size_t                    index = 0;
    const struct d_wiki_doc*  doc   = d_internal_record(host, _args[0],
                                                        _args[1],
                                                        D_INTERNAL_REGIONS,
                                                        &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page, or an index to no region
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_wiki_region* const region =
        &d_wiki_doc_records(doc)->regions[index];
    const char* const                 field  = d_internal_text(_args[2]);

    // a table of the fields
    if (d_internal_same(field, "name"))
    {
        return d_internal_set_text(_out, region->name);
    }

    // the line its first marker is on
    if (d_internal_same(field, "line"))
    {
        d_awk_value_set_number(_out, (double)region->line);

        return true;
    }

    // the text between its markers
    if (d_internal_same(field, "text"))
    {
        return d_internal_set_span(_out, doc, region->content, region->close);
    }

    return d_internal_unknown_field(host, _out, field);
}


static bool
d_internal_wiki_find_region(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const      host = _user;
    const struct d_wiki_doc* const doc  = d_internal_page(host, _args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const size_t found = d_wiki_find_region(d_wiki_doc_records(doc),
                                            d_internal_text(_args[1]));

    return d_internal_number_or_none(_out, found);
}


//==============================================================================
// 5.  CHANGES
//==============================================================================


/*
d_internal_changing
  The page a change is made to, and an index of it -- from 1, or from 0 for
a section -- checked; NULL when there is none.
*/
static struct d_wiki_doc*
d_internal_changing(
    struct d_host_wiki*     _host,
    struct d_awk_value*     _handle,
    struct d_awk_value*     _index,
    enum d_internal_counted _kind,
    size_t*                 _out_index
)
{
    size_t                   index = 0;
    const struct d_wiki_doc* doc   = d_internal_record(_host, _handle, _index,
                                                       _kind, &index);

    *_out_index = index;

    return (struct d_wiki_doc*)doc;
}


/*
wiki_set_link(p, i, target[, label]), wiki_set_label(p, i, label),
wiki_unlink(p, i)
*/
static bool
d_internal_wiki_set_link(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host   = _user;
    size_t                    index  = 0;
    struct d_wiki_doc* const  doc    = d_internal_changing(host, _args[0],
                                                           _args[1],
                                                           D_INTERNAL_LINKS,
                                                           &index);
    char* const               target = d_internal_copy_arg(_args[2]);
    char* const               label  = (_count > 3u)
                                           ? d_internal_copy_arg(_args[3])
                                           : NULL;

    (void)_problem;
    (void)_capacity;

    // a handle to no page, or text that could not be copied
    if ( (!doc) ||
         (!target) ||
         ( (_count > 3u) && (!label) ) )
    {
        free(target);
        free(label);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const bool ok = d_wiki_doc_set_link(doc, index, target, label);

    free(target);
    free(label);

    return d_internal_changed(host, _out, doc, ok);
}


static bool
d_internal_wiki_set_label(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host  = _user;
    size_t                    index = 0;
    struct d_wiki_doc* const  doc   = d_internal_changing(host, _args[0],
                                                          _args[1],
                                                          D_INTERNAL_LINKS,
                                                          &index);
    char* const               label = d_internal_copy_arg(_args[2]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page, or a label that could not be copied
    if ( (!doc) ||
         (!label) )
    {
        free(label);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const bool ok = d_wiki_doc_set_link(doc, index, NULL, label);

    free(label);

    return d_internal_changed(host, _out, doc, ok);
}


static bool
d_internal_wiki_unlink(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host  = _user;
    size_t                    index = 0;
    struct d_wiki_doc* const  doc   = d_internal_changing(host, _args[0],
                                                          _args[1],
                                                          D_INTERNAL_LINKS,
                                                          &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_changed(host, _out, doc,
                              d_wiki_doc_unlink(doc, index));
}


/*
wiki_set_body(p, i, text), wiki_set_section(p, i, text),
wiki_rename_section(p, i, title), wiki_remove_section(p, i)
*/
static bool
d_internal_section_change(
    struct d_host_wiki*  _host,
    struct d_awk_value** _args,
    struct d_awk_value*  _out,
    int                  _which
)
{
    size_t                   index = 0;
    struct d_wiki_doc* const doc   = d_internal_changing(_host, _args[0],
                                                         _args[1],
                                                         D_INTERNAL_SECTIONS,
                                                         &index);
    char* const              text  = (_which < 3) ? d_internal_copy_arg(
                                                        _args[2])
                                                  : NULL;
    bool                     ok    = false;

    // a handle to no page, or text that could not be copied
    if ( (!doc) ||
         ( (_which < 3) && (!text) ) )
    {
        free(text);

        return d_internal_fail(_host, _out, (doc) ? "out of memory"
                                                  : _host->error);
    }

    // a table of the changes
    switch (_which)
    {
        case 0:  ok = d_wiki_doc_set_body(doc, index, text);       break;
        case 1:  ok = d_wiki_doc_set_section(doc, index, text);    break;
        case 2:  ok = d_wiki_doc_rename_section(doc, index, text); break;
        default: ok = d_wiki_doc_remove_section(doc, index);       break;
    }

    free(text);

    return d_internal_changed(_host, _out, doc, ok);
}


static bool
d_internal_wiki_set_body(
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

    return d_internal_section_change(_user, _args, _out, 0);
}


static bool
d_internal_wiki_set_section(
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

    return d_internal_section_change(_user, _args, _out, 1);
}


static bool
d_internal_wiki_rename_section(
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

    return d_internal_section_change(_user, _args, _out, 2);
}


static bool
d_internal_wiki_remove_section(
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

    return d_internal_section_change(_user, _args, _out, 3);
}


/*
d_internal_top_level
  The level a page's top sections are at: the shallowest there is, or the
dialect's own, 1 for Markdown and 2 for MediaWiki.
*/
static size_t
d_internal_top_level(
    const struct d_wiki* _wiki
)
{
    size_t level = D_WIKI_MOST_LEVEL + 1u;

    // the shallowest section
    for (size_t index = 1u; index < _wiki->section_count; ++index)
    {
        level = (_wiki->sections[index].level < level)
                    ? _wiki->sections[index].level : level;
    }

    // a page with no sections: its dialect's first level
    if (level > D_WIKI_MOST_LEVEL)
    {
        level = (_wiki->dialect == D_WIKI_MEDIAWIKI) ? 2u : 1u;
    }

    return level;
}


/*
wiki_insert_section(p, before, level, title[, body]),
wiki_add_section(p, parent, title[, body])
*/
static bool
d_internal_wiki_insert_section(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host   = _user;
    struct d_wiki_doc* const  doc    = d_internal_page(host, _args[0]);
    const size_t              before = d_internal_index(_args[1]);
    const size_t              level  = d_internal_index(_args[2]);
    char* const               title  = d_internal_copy_arg(_args[3]);
    char* const               body   = (_count > 4u)
                                           ? d_internal_copy_arg(_args[4])
                                           : NULL;

    (void)_problem;
    (void)_capacity;

    // a handle to no page, or a title that could not be copied
    if ( (!doc) ||
         (!title) )
    {
        free(title);
        free(body);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const bool ok = d_wiki_doc_insert_section(doc, before, level, title, body);

    free(title);
    free(body);

    return d_internal_changed(host, _out, doc, ok);
}


static bool
d_internal_wiki_add_section(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host   = _user;
    struct d_wiki_doc* const  doc    = d_internal_page(host, _args[0]);
    const double              number = d_awk_value_number(_args[1]);
    char* const               title  = d_internal_copy_arg(_args[2]);
    char* const               body   = (_count > 3u)
                                           ? d_internal_copy_arg(_args[3])
                                           : NULL;

    (void)_problem;
    (void)_capacity;

    // a handle to no page, or a title that could not be copied
    if ( (!doc) ||
         (!title) )
    {
        free(title);
        free(body);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const struct d_wiki* const wiki   = d_wiki_doc_records(doc);
    const size_t               parent = (number >= 1.0) ? (size_t)number : 0u;

    // a section there is, or the page
    if (parent >= wiki->section_count)
    {
        free(title);
        free(body);

        return d_internal_fail(host, _out, "no section of that number");
    }

    const size_t level = (parent == 0u) ? d_internal_top_level(wiki)
                                        : (wiki->sections[parent].level + 1u);
    const size_t end   = (parent == 0u) ? wiki->length
                                        : wiki->sections[parent].end;
    size_t       before = wiki->section_count;

    // before the first section that begins where it ends
    for (size_t index = parent + 1u; index < wiki->section_count; ++index)
    {
        // the first section past the parent's end
        if (wiki->sections[index].start >= end)
        {
            before = index;

            break;
        }
    }

    const bool ok = (level <= D_WIKI_MOST_LEVEL) &&
                    (d_wiki_doc_insert_section(doc, before, level, title,
                                               body));

    free(title);
    free(body);

    // a level past the deepest a heading may be
    if ( (!ok) &&
         (level > D_WIKI_MOST_LEVEL) )
    {
        return d_internal_fail(host, _out, "a heading is no deeper than "
                                           "level 6");
    }

    // the new section's index
    if (ok)
    {
        d_awk_value_set_number(_out, (double)before);

        return true;
    }

    return d_internal_changed(host, _out, doc, false);
}


/*
wiki_set_region(p, name, text), wiki_add_region(p, name, text[, section]),
wiki_remove_region(p, name[, keep])
*/
static bool
d_internal_wiki_set_region(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host = _user;
    struct d_wiki_doc* const  doc  = d_internal_page(host, _args[0]);
    char* const               name = d_internal_copy_arg(_args[1]);
    char* const               text = d_internal_copy_arg(_args[2]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a handle to no page, or text that could not be copied
    if ( (!doc) ||
         (!name) ||
         (!text) )
    {
        free(name);
        free(text);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const bool ok = d_wiki_doc_set_region(doc, name, text);

    free(name);
    free(text);

    return d_internal_changed(host, _out, doc, ok);
}


static bool
d_internal_wiki_add_region(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host    = _user;
    struct d_wiki_doc* const  doc     = d_internal_page(host, _args[0]);
    char* const               name    = d_internal_copy_arg(_args[1]);
    char* const               text    = d_internal_copy_arg(_args[2]);
    const double              section = (_count > 3u)
                                            ? d_awk_value_number(_args[3])
                                            : -1.0;

    (void)_problem;
    (void)_capacity;

    // a handle to no page, or text that could not be copied
    if ( (!doc) ||
         (!name) ||
         (!text) )
    {
        free(name);
        free(text);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const bool ok = d_wiki_doc_add_region(doc, name, text,
                                          (section >= 0.0)
                                              ? (size_t)section
                                              : D_WIKI_NONE);

    free(name);
    free(text);

    return d_internal_changed(host, _out, doc, ok);
}


static bool
d_internal_wiki_remove_region(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host = _user;
    struct d_wiki_doc* const  doc  = d_internal_page(host, _args[0]);
    char* const               name = d_internal_copy_arg(_args[1]);
    const bool                keep = (_count > 2u) &&
                                     (d_awk_value_is_true(_args[2]));

    (void)_problem;
    (void)_capacity;

    // a handle to no page, or a name that could not be copied
    if ( (!doc) ||
         (!name) )
    {
        free(name);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const bool ok = d_wiki_doc_remove_region(doc, name, keep);

    free(name);

    return d_internal_changed(host, _out, doc, ok);
}


/*
wiki_set_arg(p, t, key, value), wiki_remove_arg(p, t, key)
*/
static bool
d_internal_wiki_set_arg(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host  = _user;
    size_t                    index = 0;
    struct d_wiki_doc* const  doc   = d_internal_changing(host, _args[0],
                                                          _args[1],
                                                          D_INTERNAL_TEMPLATES,
                                                          &index);
    char* const               key   = d_internal_copy_arg(_args[2]);
    char* const               value = (_count > 3u)
                                          ? d_internal_copy_arg(_args[3])
                                          : d_wiki_copy("", 0u);
    bool                      ok    = false;

    (void)_problem;
    (void)_capacity;

    // a handle to no page, or text that could not be copied
    if ( (!doc) ||
         (!key) ||
         (!value) )
    {
        free(key);
        free(value);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    // set, or, with no value given, removed
    ok = (_count > 3u) ? d_wiki_doc_set_argument(doc, index, key, value)
                       : d_wiki_doc_remove_argument(doc, index, key);

    free(key);
    free(value);

    return d_internal_changed(host, _out, doc, ok);
}


//==============================================================================
// 6.  WRITING
//==============================================================================


/*
d_internal_writer
  The dialect a writer is asked for, its first argument: -1 for none.
*/
static bool
d_internal_writer(
    struct d_host_wiki*  _host,
    struct d_awk_value** _args,
    struct d_awk_value*  _out,
    enum d_wiki_dialect* _out_dialect
)
{
    *_out_dialect = d_internal_dialect(_host, _args[0]);

    // a dialect it knows
    if (*_out_dialect == D_WIKI_OTHER)
    {
        (void)d_internal_fail(_host, _out, _host->error);

        return false;
    }

    return true;
}


/*
wiki_heading_text(d, level, title)
*/
static bool
d_internal_wiki_heading_text(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host    = _user;
    struct d_wiki_buffer      text    = { NULL, 0u, 0u, false };
    enum d_wiki_dialect       dialect = D_WIKI_OTHER;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // no dialect: wiki_error() says why
    if (!d_internal_writer(host, _args, _out, &dialect))
    {
        return true;
    }

    const size_t level = d_internal_index(_args[1]);
    char* const  title = d_internal_copy_arg(_args[2]);
    const bool   ok    = (title) &&
                         (d_wiki_write_heading(&text, dialect, level, title));

    free(title);

    return d_internal_set_buffer(host, _out, &text, ok);
}


/*
wiki_link_text(d, page[, label[, anchor]])
*/
static bool
d_internal_wiki_link_text(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host    = _user;
    struct d_wiki_buffer      text    = { NULL, 0u, 0u, false };
    enum d_wiki_dialect       dialect = D_WIKI_OTHER;

    (void)_problem;
    (void)_capacity;

    // no dialect: wiki_error() says why
    if (!d_internal_writer(host, _args, _out, &dialect))
    {
        return true;
    }

    char* const page   = d_internal_copy_arg(_args[1]);
    char* const label  = (_count > 2u) ? d_internal_copy_arg(_args[2]) : NULL;
    char* const anchor = (_count > 3u) ? d_internal_copy_arg(_args[3]) : NULL;
    const bool  ok     = (page) &&
                         (d_wiki_write_page_link(&text, dialect, page, anchor,
                                                 label));

    free(page);
    free(label);
    free(anchor);

    return d_internal_set_buffer(host, _out, &text, ok);
}


/*
d_internal_two_writer
  A writer of one text and a second perhaps: url and label, source and
text, code and language.
*/
static bool
d_internal_two_writer(
    struct d_host_wiki*  _host,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    int                  _which
)
{
    struct d_wiki_buffer text    = { NULL, 0u, 0u, false };
    enum d_wiki_dialect  dialect = D_WIKI_OTHER;
    bool                 ok      = false;

    // no dialect: wiki_error() says why
    if (!d_internal_writer(_host, _args, _out, &dialect))
    {
        return true;
    }

    char* const first  = d_internal_copy_arg(_args[1]);
    char* const second = (_count > 2u) ? d_internal_copy_arg(_args[2]) : NULL;

    // a table of the writers
    if (first)
    {
        // which of the writers that take two texts
        switch (_which)
        {
            case 0:
                ok = d_wiki_write_url_link(&text, dialect, first, second);
                break;

            case 1:
                ok = d_wiki_write_image(&text, dialect, first,
                                        (second) ? second : "");
                break;

            case 2:
                ok = d_wiki_write_code(&text, dialect, first, second);
                break;

            case 3:
                ok = d_wiki_write_inline_code(&text, dialect, first);
                break;

            case 4:
                ok = d_wiki_write_table(&text, dialect, first);
                break;

            case 5:
                ok = d_wiki_write_escape(&text, dialect, first);
                break;

            case 6:
                ok = d_wiki_write_comment(&text, dialect, first);
                break;

            default:
                ok = d_wiki_write_region(&text, dialect, first,
                                         (second) ? second : "");
                break;
        }
    }

    free(first);
    free(second);

    return d_internal_set_buffer(_host, _out, &text, ok);
}


static bool
d_internal_wiki_url_text(
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

    return d_internal_two_writer(_user, _args, _count, _out, 0);
}


static bool
d_internal_wiki_image_text(
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

    return d_internal_two_writer(_user, _args, _count, _out, 1);
}


static bool
d_internal_wiki_code_text(
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

    return d_internal_two_writer(_user, _args, _count, _out, 2);
}


static bool
d_internal_wiki_inline_code(
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

    return d_internal_two_writer(_user, _args, _count, _out, 3);
}


static bool
d_internal_wiki_table_text(
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

    return d_internal_two_writer(_user, _args, _count, _out, 4);
}


static bool
d_internal_wiki_escape(
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

    return d_internal_two_writer(_user, _args, _count, _out, 5);
}


static bool
d_internal_wiki_comment_text(
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

    return d_internal_two_writer(_user, _args, _count, _out, 6);
}


static bool
d_internal_wiki_region_text(
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

    return d_internal_two_writer(_user, _args, _count, _out, 7);
}


/*
wiki_item_text(d, text[, depth[, ordered]])
*/
static bool
d_internal_wiki_item_text(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host    = _user;
    struct d_wiki_buffer      text    = { NULL, 0u, 0u, false };
    enum d_wiki_dialect       dialect = D_WIKI_OTHER;

    (void)_problem;
    (void)_capacity;

    // no dialect: wiki_error() says why
    if (!d_internal_writer(host, _args, _out, &dialect))
    {
        return true;
    }

    const size_t depth   = (_count > 2u) ? d_internal_index(_args[2]) : 1u;
    const bool   ordered = (_count > 3u) && (d_awk_value_is_true(_args[3]));
    char* const  item    = d_internal_copy_arg(_args[1]);
    const bool   ok      = (item) &&
                           (d_wiki_write_item(&text, dialect, item,
                                              (depth > 0u) ? depth : 1u,
                                              ordered));

    free(item);

    return d_internal_set_buffer(host, _out, &text, ok);
}


/*
wiki_toc(p[, least[, most]])
*/
static bool
d_internal_wiki_toc(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const      host = _user;
    const struct d_wiki_doc* const doc  = d_internal_page(host, _args[0]);
    struct d_wiki_buffer           text = { NULL, 0u, 0u, false };

    (void)_problem;
    (void)_capacity;

    // a handle to no page
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_wiki* const wiki  = d_wiki_doc_records(doc);
    const size_t               least = (_count > 1u)
                                           ? d_internal_index(_args[1])
                                           : d_internal_top_level(wiki);
    const size_t               most  = (_count > 2u)
                                           ? d_internal_index(_args[2])
                                           : D_WIKI_MOST_LEVEL;

    (void)d_wiki_buffer_add(&text, "", 0u);

    return d_internal_set_buffer(host, _out, &text,
                                 d_wiki_write_toc(&text, wiki,
                                                  (least > 0u) ? least : 1u,
                                                  most));
}


//==============================================================================
// 7.  NAMES AND FILES
//==============================================================================


/*
wiki_page_name(path[, d]), wiki_page_file(name[, d]),
wiki_same_page(name, name[, d]), wiki_anchor(d, title),
wiki_anchors(d, titles), wiki_dialect_of(path)
*/
static bool
d_internal_names(
    struct d_host_wiki*  _host,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    int                  _which
)
{
    const bool          led     = (_which == 3) || (_which == 6);
    const size_t        given   = (_which == 2) ? 2u : 1u;
    enum d_wiki_dialect dialect = D_WIKI_MARKDOWN;
    char* const         first   = d_internal_copy_arg(_args[(led) ? 1u
                                                                  : 0u]);

    // the dialect given, else the path's for a page's name, else Markdown
    if ( (!led) &&
         (_count > given) )
    {
        dialect = d_internal_dialect(_host, _args[given]);
    }
    else if (led)
    {
        dialect = d_internal_dialect(_host, _args[0]);
    }
    else if ( (_which == 0) &&
              (first) &&
              (d_wiki_dialect_of(first) != D_WIKI_OTHER) )
    {
        dialect = d_wiki_dialect_of(first);
    }

    // a path that could not be copied, or that is no page's
    if ( (!first) ||
         (dialect == D_WIKI_OTHER) )
    {
        free(first);

        return d_internal_fail(_host, _out, (first) ? _host->error
                                                    : "out of memory");
    }

    struct d_wiki_buffer text = { NULL, 0u, 0u, false };
    bool                 ok   = true;

    // a table of the names
    switch (_which)
    {
        case 0:
        case 1:
        {
            char* const name = (_which == 0)
                                   ? d_wiki_page_name(first, dialect)
                                   : d_wiki_page_file(first, dialect);

            ok = (name) && (d_wiki_buffer_add_text(&text, name));
            free(name);
            break;
        }

        case 2:
            d_awk_value_set_number(_out,
                                   (d_wiki_same_page(first,
                                                     d_internal_text(_args[1]),
                                                     dialect)) ? 1.0 : 0.0);
            free(first);

            return true;

        case 3:
            ok = d_wiki_anchor(&text, dialect, first);
            break;

        case 6:
            ok = d_wiki_anchors(&text, dialect, first);
            break;

        case 5:
            ok = d_wiki_page_key(&text, first, dialect);
            break;

        default:
            ok = d_wiki_buffer_add_text(&text, d_wiki_dialect_name(
                                                   d_wiki_dialect_of(first)));
            break;
    }

    free(first);

    return d_internal_set_buffer(_host, _out, &text, ok);
}


static bool
d_internal_wiki_page_name(
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

    return d_internal_names(_user, _args, _count, _out, 0);
}


static bool
d_internal_wiki_page_file(
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

    return d_internal_names(_user, _args, _count, _out, 1);
}


static bool
d_internal_wiki_same_page(
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

    return d_internal_names(_user, _args, _count, _out, 2);
}


static bool
d_internal_wiki_anchor(
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

    return d_internal_names(_user, _args, _count, _out, 3);
}


static bool
d_internal_wiki_anchors(
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

    return d_internal_names(_user, _args, _count, _out, 6);
}


static bool
d_internal_wiki_page_key(
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

    return d_internal_names(_user, _args, _count, _out, 5);
}


static bool
d_internal_wiki_dialect_of(
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

    return d_internal_names(_user, _args, _count, _out, 4);
}


/*
d_internal_match
  Whether a name matches a pattern: `*` any run, `?` any byte, [...] a set
or range, `[!...]` its complement, anything else itself.
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

    // `*` takes any run, the shortest first
    if (*_pattern == '*')
    {
        // `*`: any run of bytes, the shortest first
        for (const char* at = _name; ; ++at)
        {
            // the rest of the pattern, from here
            if (d_internal_match(_pattern + 1, at))
            {
                return true;
            }

            // the name ran out before the pattern matched
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
            // a range, a-z, or a byte
            if ( (at[1] == '-') &&
                 (at[2] != ']') &&
                 (at[2] != '\0') )
            {
                matched = (matched) || ( (*_name >= at[0]) &&
                                         (*_name <= at[2]) );
                at     += 3;
            }
            else
            {
                matched = (matched) || (*_name == *at);
                at++;
            }
        } while ( (*at != ']') &&
                  (*at != '\0') );

        // a `[` never closed is itself
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
d_internal_matches_any
  Whether a name matches any of a list of patterns, apart by spaces; a
list of none, any wiki page's.
*/
static bool
d_internal_matches_any(
    const char* _patterns,
    const char* _name
)
{
    const char* at = _patterns;

    // no patterns: a page of a dialect known
    if (!_patterns)
    {
        return (d_wiki_dialect_of(_name) != D_WIKI_OTHER);
    }

    // each pattern
    while (*at != '\0')
    {
        char   pattern[256];
        size_t length = 0;

        // past the space between patterns
        while (*at == ' ')
        {
            at++;
        }

        // the pattern, to the next space
        while ( (*at != '\0') &&
                (*at != ' ') &&
                (length < (sizeof(pattern) - 1u)) )
        {
            pattern[length++] = *at++;
        }

        pattern[length] = '\0';

        // a pattern the name matches
        if ( (length > 0u) &&
             (d_internal_match(pattern, _name)) )
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_keep_path
  Keeps a path found.
*/
static void
d_internal_keep_path(
    struct d_internal_found* _found,
    char*                    _path
)
{
    // room for one more
    if ( (!_path) ||
         (_found->count >= _found->capacity) )
    {
        const size_t wanted = (_found->capacity > 0u)
                                  ? (_found->capacity * 2u) : 64u;
        char** const grown  = (_path) ? realloc(_found->paths,
                                                wanted * sizeof(char*))
                                      : NULL;

        // a path that cannot be kept fails the listing
        if (!grown)
        {
            free(_path);
            _found->failed = true;

            return;
        }

        _found->paths    = grown;
        _found->capacity = wanted;
    }

    _found->paths[_found->count++] = _path;

    return;
}


/*
d_internal_walk
  The files of a directory that match, and, deep, of those within it;
names that begin with `.` are passed over.
*/
static void
d_internal_walk(
    const char*              _directory,
    const char*              _patterns,
    bool                     _deep,
    size_t                   _depth,
    struct d_internal_found* _found
)
{
    char**       names = NULL;
    size_t       count = 0;
    const size_t span  = strlen(_directory);
    const bool   slash = (span > 0u) &&
                         ( (_directory[span - 1u] == '/') ||
                           (_directory[span - 1u] == '\\') );

    // a directory that cannot be read holds nothing
    if ( (_depth > 64u) ||
         (!d_fs_names(_directory, &names, &count)) )
    {
        return;
    }

    // each name, its path, a file kept or a directory walked
    for (size_t index = 0; index < count; ++index)
    {
        // a name that begins with `.` is hidden
        if (names[index][0] == '.')
        {
            continue;
        }

        const size_t length = span + 1u + strlen(names[index]);
        char* const  path   = malloc(length + 1u);

        // a path that cannot be made fails the listing
        if (!path)
        {
            _found->failed = true;

            break;
        }

        (void)snprintf(path, length + 1u, "%s%s%s", _directory,
                       (slash || (span == 0u)) ? "" : "/", names[index]);

        const enum d_fs_kind kind = d_fs_kind_of(path);

        // a directory walked, when deep; a file kept when a pattern matches it
        if ( (kind == D_FS_KIND_DIRECTORY) &&
             (_deep) )
        {
            d_internal_walk(path, _patterns, true, _depth + 1u, _found);
            free(path);
        }
        else if ( (kind == D_FS_KIND_FILE) &&
                  (d_internal_matches_any(_patterns, names[index])) )
        {
            d_internal_keep_path(_found, path);
        }
        else
        {
            free(path);
        }
    }

    d_fs_names_free(names, count);

    return;
}


static int
d_internal_compare_paths(
    const void* _left,
    const void* _right
)
{
    return strcmp(*(char* const*)_left, *(char* const*)_right);
}


/*
wiki_pages(directory[, deep]), wiki_files(directory, patterns[, deep])
*/
static bool
d_internal_listing(
    struct d_host_wiki*  _host,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    bool                 _patterned
)
{
    struct d_internal_found found     = { NULL, 0u, 0u, false };
    struct d_wiki_buffer    joined    = { NULL, 0u, 0u, false };
    char* const             directory = d_internal_copy_arg(_args[0]);
    char* const             patterns  = (_patterned)
                                            ? d_internal_copy_arg(_args[1])
                                            : NULL;
    const size_t            deep_at   = (_patterned) ? 2u : 1u;
    const bool              deep      = (_count > deep_at) &&
                                        (d_awk_value_is_true(_args[deep_at]));

    // a directory or patterns that could not be copied
    if ( (!directory) ||
         ( (_patterned) && (!patterns) ) )
    {
        free(directory);
        free(patterns);

        return d_internal_fail(_host, _out, "out of memory");
    }

    d_internal_walk(directory, patterns, deep, 0u, &found);

    // sorted, one to a line
    if (found.count > 1u)
    {
        qsort(found.paths, found.count, sizeof(char*),
              d_internal_compare_paths);
    }

    // the paths, a line each
    for (size_t index = 0; index < found.count; ++index)
    {
        // a line ending between them
        if (index > 0u)
        {
            (void)d_wiki_buffer_add_char(&joined, '\n');
        }

        (void)d_wiki_buffer_add_text(&joined, found.paths[index]);
        free(found.paths[index]);
    }

    (void)d_wiki_buffer_add(&joined, "", 0u);
    free(found.paths);
    free(directory);
    free(patterns);

    return d_internal_set_buffer(_host, _out, &joined, !found.failed);
}


static bool
d_internal_wiki_pages(
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

    return d_internal_listing(_user, _args, _count, _out, false);
}


static bool
d_internal_wiki_files(
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

    return d_internal_listing(_user, _args, _count, _out, true);
}


/*
wiki_exists(path), wiki_move(path, to)
*/
static bool
d_internal_wiki_exists(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const enum d_fs_kind kind = d_fs_kind_of(d_internal_text(_args[0]));

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
d_internal_wiki_move(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_wiki* const host = _user;
    char* const               from = d_internal_copy_arg(_args[0]);
    char* const               to   = d_internal_copy_arg(_args[1]);
    bool                      ok   = false;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a file there, and nothing where it goes
    if ( (from) &&
         (to) )
    {
        // only a file is moved
        if (d_fs_kind_of(from) != D_FS_KIND_FILE)
        {
            (void)snprintf(host->error, sizeof(host->error),
                           "%s is not a file", from);
        }
        else if (d_fs_kind_of(to) != D_FS_KIND_NONE)
        {
            (void)snprintf(host->error, sizeof(host->error),
                           "%s is there already", to);
        }
        else if (rename(from, to) != 0)
        {
            (void)snprintf(host->error, sizeof(host->error),
                           "cannot move %s to %s", from, to);
        }
        else
        {
            ok = true;
        }
    }

    free(from);
    free(to);

    // why it could not be moved
    if (!ok)
    {
        return d_internal_fail(host, _out, host->error);
    }

    d_awk_value_set_number(_out, 1.0);

    return true;
}


//==============================================================================
// 8.  THE EXTENSION
//==============================================================================


/*
d_internal_functions
  The extension's host functions, as a program calls them, and how many.
*/
static const struct d_awk_host_function*
d_internal_functions(
    size_t* _out_count
)
{
    static const struct d_awk_host_function functions[] =
    {
        { "wiki_open",           1u, 2u, d_internal_wiki_open           },
        { "wiki_new",            1u, 3u, d_internal_wiki_new            },
        { "wiki_save",           1u, 2u, d_internal_wiki_save           },
        { "wiki_close",          1u, 1u, d_internal_wiki_close          },
        { "wiki_error",          0u, 0u, d_internal_wiki_error          },
        { "wiki_path",           1u, 1u, d_internal_wiki_path           },
        { "wiki_dialect",        1u, 1u, d_internal_wiki_dialect        },
        { "wiki_text",           1u, 1u, d_internal_wiki_text           },
        { "wiki_changes",        1u, 1u, d_internal_wiki_changes        },
        { "wiki_page",           1u, 1u, d_internal_wiki_page           },
        { "wiki_problems",       1u, 1u, d_internal_wiki_problems       },
        { "wiki_problem",        3u, 3u, d_internal_wiki_problem        },
        { "wiki_blocks",         1u, 1u, d_internal_wiki_blocks         },
        { "wiki_block",          3u, 3u, d_internal_wiki_block          },
        { "wiki_sections",       1u, 1u, d_internal_wiki_sections       },
        { "wiki_section",        3u, 3u, d_internal_wiki_section        },
        { "wiki_find_section",   2u, 3u, d_internal_wiki_find_section   },
        { "wiki_set_body",       3u, 3u, d_internal_wiki_set_body       },
        { "wiki_set_section",    3u, 3u, d_internal_wiki_set_section    },
        { "wiki_rename_section", 3u, 3u, d_internal_wiki_rename_section },
        { "wiki_insert_section", 4u, 5u, d_internal_wiki_insert_section },
        { "wiki_add_section",    3u, 4u, d_internal_wiki_add_section    },
        { "wiki_remove_section", 2u, 2u, d_internal_wiki_remove_section },
        { "wiki_headings",       1u, 1u, d_internal_wiki_headings       },
        { "wiki_heading",        3u, 3u, d_internal_wiki_heading        },
        { "wiki_links",          1u, 1u, d_internal_wiki_links          },
        { "wiki_link",           3u, 3u, d_internal_wiki_link           },
        { "wiki_find_link",      2u, 3u, d_internal_wiki_find_link      },
        { "wiki_set_link",       3u, 4u, d_internal_wiki_set_link       },
        { "wiki_set_label",      3u, 3u, d_internal_wiki_set_label      },
        { "wiki_unlink",         2u, 2u, d_internal_wiki_unlink         },
        { "wiki_templates",      1u, 1u, d_internal_wiki_templates      },
        { "wiki_template",       3u, 3u, d_internal_wiki_template       },
        { "wiki_find_template",  2u, 3u, d_internal_wiki_find_template  },
        { "wiki_arg",            3u, 3u, d_internal_wiki_arg            },
        { "wiki_has_arg",        3u, 3u, d_internal_wiki_has_arg        },
        { "wiki_arg_key",        3u, 3u, d_internal_wiki_arg_key        },
        { "wiki_set_arg",        4u, 4u, d_internal_wiki_set_arg        },
        { "wiki_remove_arg",     3u, 3u, d_internal_wiki_set_arg        },
        { "wiki_regions",        1u, 1u, d_internal_wiki_regions        },
        { "wiki_region",         3u, 3u, d_internal_wiki_region         },
        { "wiki_find_region",    2u, 2u, d_internal_wiki_find_region    },
        { "wiki_set_region",     3u, 3u, d_internal_wiki_set_region     },
        { "wiki_add_region",     3u, 4u, d_internal_wiki_add_region     },
        { "wiki_remove_region",  2u, 3u, d_internal_wiki_remove_region  },
        { "wiki_heading_text",   3u, 3u, d_internal_wiki_heading_text   },
        { "wiki_link_text",      2u, 4u, d_internal_wiki_link_text      },
        { "wiki_url_text",       2u, 3u, d_internal_wiki_url_text       },
        { "wiki_image_text",     2u, 3u, d_internal_wiki_image_text     },
        { "wiki_code_text",      2u, 3u, d_internal_wiki_code_text      },
        { "wiki_inline_code",    2u, 2u, d_internal_wiki_inline_code    },
        { "wiki_item_text",      2u, 4u, d_internal_wiki_item_text      },
        { "wiki_table_text",     2u, 2u, d_internal_wiki_table_text     },
        { "wiki_escape",         2u, 2u, d_internal_wiki_escape         },
        { "wiki_comment_text",   2u, 2u, d_internal_wiki_comment_text   },
        { "wiki_region_text",    3u, 3u, d_internal_wiki_region_text    },
        { "wiki_toc",            1u, 3u, d_internal_wiki_toc            },
        { "wiki_page_name",      1u, 2u, d_internal_wiki_page_name      },
        { "wiki_page_file",      1u, 2u, d_internal_wiki_page_file      },
        { "wiki_same_page",      2u, 3u, d_internal_wiki_same_page      },
        { "wiki_page_key",       1u, 2u, d_internal_wiki_page_key       },
        { "wiki_anchor",         2u, 2u, d_internal_wiki_anchor         },
        { "wiki_anchors",        2u, 2u, d_internal_wiki_anchors        },
        { "wiki_dialect_of",     1u, 1u, d_internal_wiki_dialect_of     },
        { "wiki_pages",          1u, 2u, d_internal_wiki_pages          },
        { "wiki_files",          2u, 3u, d_internal_wiki_files          },
        { "wiki_exists",         1u, 1u, d_internal_wiki_exists         },
        { "wiki_move",           2u, 2u, d_internal_wiki_move           }
    };

    *_out_count = sizeof(functions) / sizeof(functions[0]);

    return functions;
}


struct d_host_wiki*
d_host_wiki_new(void)
{
    return calloc(1u, sizeof(struct d_host_wiki));
}


void
d_host_wiki_free(
    struct d_host_wiki* _host
)
{
    // freeing nothing is allowed
    if (!_host)
    {
        return;
    }

    // each page still open
    for (size_t index = 0; index < _host->page_count; ++index)
    {
        d_wiki_doc_free(_host->pages[index]);
    }

    free(_host->pages);
    free(_host);

    return;
}


bool
d_host_wiki_install(
    struct d_host_wiki*  _host,
    struct d_awk_interp* _interp
)
{
    size_t                                  count     = 0;
    const struct d_awk_host_function* const functions = d_internal_functions(
                                                            &count);

    // parameter validation first
    if ( (!_host) ||
         (!_interp) )
    {
        return false;
    }

    return d_awk_interp_add_functions(_interp, functions, count, _host);
}
