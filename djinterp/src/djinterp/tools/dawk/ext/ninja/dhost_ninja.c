/*******************************************************************************
* djinterp [tools]                                                 dhost_ninja.c
*
* Definitions for the declarations in dhost_ninja.h.
*   Each file open is a dninja document, and each build loaded a dninja_eval
* build; a handle is a place in the list of its kind, from 1, and one let go
* keeps its place, empty, so a handle to it names nothing rather than
* another. What a file or a build holds is read from its records as they
* are now, each field by its name; a change is the document's, which keeps
* it or says why not. A list -- of paths, of edges -- is given a line to an
* item.
*
* path:      /src/djinterp/tools/dawk/ext/ninja/dhost_ninja.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/ninja/dhost_ninja.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, calloc, free
#include <string.h>  // memcpy, strcmp, strlen
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dhost.h"                  // d_awk_host_function
#include "../../../../../../inc/djinterp/tools/dawk/dvalue.h"                 // d_awk_value_text
#include "../../../../../../inc/djinterp/tools/dawk/ext/ninja/dninja.h"       // d_ninja
#include "../../../../../../inc/djinterp/tools/dawk/ext/ninja/dninja_doc.h"   // d_ninja_doc
#include "../../../../../../inc/djinterp/tools/dawk/ext/ninja/dninja_eval.h"  // d_ninja_graph


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Extension
//------------------------------------------------------------------------------
// 1.1.1
// d_host_ninja
//   struct: the files and builds, open or let go, and the last failure.
struct d_host_ninja
{
    struct d_ninja_doc**   files;
    size_t                 file_count;
    size_t                 file_capacity;
    struct d_ninja_graph** graphs;
    size_t                 graph_count;
    size_t                 graph_capacity;
    char                   error[768];
};

// 1.1.2
// d_internal_lines
//   struct: lines being joined, in an array that grows.
struct d_internal_lines
{
    char*  text;
    size_t length;
    size_t capacity;
    size_t count;
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
    char* const       copy   = malloc(length + 1u);

    // a copy that cannot be made is NULL
    if (copy)
    {
        memcpy(copy, text, length);
        copy[length] = '\0';
    }

    return copy;
}


/*
d_internal_fail
  Gives -1 for a failure a program can recover from, ninja_error() saying
why.
*/
static bool
d_internal_fail(
    struct d_host_ninja* _host,
    struct d_awk_value*  _out,
    const char*          _why
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
d_internal_clear
  Clears the last failure, as each call but ninja_error() does first, so
that ninja_error() is "" after a call that did not fail.
*/
static void
d_internal_clear(
    void* _user
)
{
    struct d_host_ninja* const host = _user;

    host->error[0] = '\0';

    return;
}


/*
d_internal_set_text, d_internal_set_input, d_internal_set_number
  Gives a string; text of a file, which compares as a number when it looks
like one, as a field does; or a number.
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
d_internal_set_input(
    struct d_awk_value* _out,
    const char*         _text,
    size_t              _length
)
{
    return d_awk_value_set_input(_out, _text, _length);
}


static bool
d_internal_set_number(
    struct d_awk_value* _out,
    double              _number
)
{
    d_awk_value_set_number(_out, _number);

    return true;
}


/*
d_internal_set_owned
  Gives a text made for the caller, and frees it; -1 for one not made.
*/
static bool
d_internal_set_owned(
    struct d_host_ninja* _host,
    struct d_awk_value*  _out,
    char*                _text,
    const char*          _why
)
{
    // a text that could not be made
    if (!_text)
    {
        return d_internal_fail(_host, _out, _why);
    }

    const bool set = d_internal_set_input(_out, _text, strlen(_text));

    free(_text);

    return set;
}


/*
d_internal_field
  A field's place among a table of names; `_count` for none.
*/
static size_t
d_internal_field(
    const char*        _field,
    const char* const* _names,
    size_t             _count
)
{
    // each name in turn
    for (size_t at = 0u; at < _count; ++at)
    {
        // the field's
        if (strcmp(_field, _names[at]) == 0)
        {
            return at;
        }
    }

    return _count;
}


/*
d_internal_unknown_field
  Gives "" for a field not known, ninja_error() saying so.
*/
static bool
d_internal_unknown_field(
    struct d_host_ninja* _host,
    struct d_awk_value*  _out,
    const char*          _field
)
{
    (void)snprintf(_host->error, sizeof(_host->error), "no field %s",
                   _field);

    return d_awk_value_set_string(_out, "", 0u);
}


/*
d_internal_add_line
  Adds an item to lines being joined, a line ending before all but the
first.
*/
static void
d_internal_add_line(
    struct d_internal_lines* _lines,
    const char*              _text,
    size_t                   _length
)
{
    const size_t more = _length + 2u;

    // a list that failed takes nothing more
    if (_lines->failed)
    {
        return;
    }

    // room for the item, its line ending and a terminator
    if ((_lines->length + more) > _lines->capacity)
    {
        size_t wanted = (_lines->capacity > 0u) ? _lines->capacity : 64u;

        // doubled until it holds the item
        while (wanted < (_lines->length + more))
        {
            wanted *= 2u;
        }

        char* const grown = realloc(_lines->text, wanted);

        // memory that ran out fails the list
        if (!grown)
        {
            _lines->failed = true;

            return;
        }

        _lines->text     = grown;
        _lines->capacity = wanted;
    }

    // a line ending between two items
    if (_lines->count > 0u)
    {
        _lines->text[_lines->length++] = '\n';
    }

    memcpy(&_lines->text[_lines->length], _text, _length);
    _lines->length               += _length;
    _lines->text[_lines->length]  = '\0';
    _lines->count++;

    return;
}


/*
d_internal_give_lines
  Gives the lines joined, and frees them.
*/
static bool
d_internal_give_lines(
    struct d_host_ninja*     _host,
    struct d_awk_value*      _out,
    struct d_internal_lines* _lines
)
{
    // a list that could not be made
    if (_lines->failed)
    {
        free(_lines->text);

        return d_internal_fail(_host, _out, "out of memory");
    }

    const bool set = d_awk_value_set_string(_out,
                                            (_lines->text) ? _lines->text
                                                           : "",
                                            _lines->length);

    free(_lines->text);

    return set;
}


/*
d_internal_section
  A section by its name, or -1 for a name none has.
*/
static int
d_internal_section(
    const char* _name
)
{
    static const char* const names[] =
    {
        "output", "implicit_output", "input", "implicit", "order_only",
        "validation", "target", "file"
    };
    const size_t             count = sizeof(names) / sizeof(names[0]);
    const size_t             found = d_internal_field(_name, names, count);

    return (found < count) ? (int)found : -1;
}


//==============================================================================
// 3.  FILES
//==============================================================================


/*
d_internal_file
  The file a handle names, or NULL, ninja_error() saying why.
*/
static struct d_ninja_doc*
d_internal_file(
    struct d_host_ninja* _host,
    struct d_awk_value*  _handle
)
{
    const size_t handle = d_internal_index(_handle);

    // a handle in the list, to a file not closed
    if ( (handle == 0u) ||
         (handle > _host->file_count) ||
         (!_host->files[handle - 1u]) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "no file has the handle %s", d_internal_text(_handle));

        return NULL;
    }

    return _host->files[handle - 1u];
}


/*
d_internal_add_file
  Keeps a file, and gives its handle.
*/
static bool
d_internal_add_file(
    struct d_host_ninja* _host,
    struct d_awk_value*  _out,
    struct d_ninja_doc*  _doc
)
{
    // room for one more
    if (_host->file_count >= _host->file_capacity)
    {
        const size_t         wanted = (_host->file_capacity > 0u)
                                          ? (_host->file_capacity * 2u) : 8u;
        struct d_ninja_doc** grown  = realloc(_host->files,
                                              wanted * sizeof(*grown));

        // a file that cannot be kept is freed, and the handle not given
        if (!grown)
        {
            d_ninja_doc_free(_doc);

            return d_internal_fail(_host, _out, "out of memory");
        }

        _host->files         = grown;
        _host->file_capacity = wanted;
    }

    _host->files[_host->file_count++] = _doc;

    return d_internal_set_number(_out, (double)_host->file_count);
}


/*
ninja_open(path), ninja_new(path[, text])
*/
static bool
d_internal_ninja_open(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host = _user;
    char                       why[512];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    struct d_ninja_doc* const doc = d_ninja_doc_read(d_internal_text(_args[0]),
                                                     why, sizeof(why));

    // why it did not read
    if (!doc)
    {
        return d_internal_fail(host, _out, why);
    }

    return d_internal_add_file(host, _out, doc);
}


static bool
d_internal_ninja_new(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host = _user;
    char* const                path = d_internal_copy_arg(_args[0]);
    char                       why[512];

    (void)_problem;
    (void)_capacity;

    // a path that could not be copied
    if (!path)
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    size_t                    length = 0;
    const char* const         text   = (_count > 1u)
                                           ? d_awk_value_text(_args[1], NULL,
                                                              &length)
                                           : "";
    struct d_ninja_doc* const doc    = d_ninja_doc_make(path, text, length,
                                                        why, sizeof(why));

    free(path);

    // why it could not be made
    if (!doc)
    {
        return d_internal_fail(host, _out, why);
    }

    return d_internal_add_file(host, _out, doc);
}


/*
ninja_save(f[, path]), ninja_close(f), ninja_error()
*/
static bool
d_internal_ninja_save(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host = _user;
    struct d_ninja_doc* const  doc  = d_internal_file(host, _args[0]);
    char                       why[512];

    (void)_problem;
    (void)_capacity;

    // a handle to no file
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const enum d_ninja_saved saved = d_ninja_doc_save(
                                         doc,
                                         (_count > 1u)
                                             ? d_internal_text(_args[1])
                                             : NULL,
                                         why, sizeof(why));

    // why it could not be written
    if (saved == D_NINJA_SAVE_FAILED)
    {
        return d_internal_fail(host, _out, why);
    }

    return d_internal_set_number(_out,
                                 (saved == D_NINJA_SAVE_WRITTEN) ? 1.0 : 0.0);
}


static bool
d_internal_ninja_close(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host   = _user;
    const size_t               handle = d_internal_index(_args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a file open, freed, its place kept
    if ( (handle == 0u) ||
         (handle > host->file_count) ||
         (!host->files[handle - 1u]) )
    {
        return d_internal_set_number(_out, 0.0);
    }

    d_ninja_doc_free(host->files[handle - 1u]);
    host->files[handle - 1u] = NULL;

    return d_internal_set_number(_out, 1.0);
}


static bool
d_internal_ninja_error(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_ninja* const host = _user;

    (void)_args;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_set_text(_out, host->error);
}


/*
d_internal_file_fact
  A fact of a file: 0 its path, 1 its text, 2 its changes, 3 its
statements, 4 its comments.
*/
static bool
d_internal_file_fact(
    void*               _user,
    struct d_awk_value* _handle,
    struct d_awk_value* _out,
    int                 _fact
)
{
    struct d_host_ninja* const      host = _user;
    const struct d_ninja_doc* const doc  = d_internal_file(host, _handle);

    // a handle to no file
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    size_t                      length  = 0;
    const char* const           text    = d_ninja_doc_text(doc, &length);
    const struct d_ninja* const records = d_ninja_doc_records(doc);

    // a table of the facts
    switch (_fact)
    {
        case 0:
            return d_internal_set_text(_out, d_ninja_doc_path(doc));

        case 1:
            return d_awk_value_set_string(_out, text, length);

        case 2:
            return d_internal_set_number(_out,
                                         (double)d_ninja_doc_changes(doc));

        case 3:
            return d_internal_set_number(_out,
                                         (double)records->statement_count);

        default:
            break;
    }

    return d_internal_set_number(_out, (double)records->comment_count);
}


/*
ninja_path(f), ninja_text(f), ninja_changes(f), ninja_statements(f),
ninja_comments(f)
*/
static bool
d_internal_ninja_path(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_file_fact(_user, _args[0], _out, 0);
}


static bool
d_internal_ninja_text(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_file_fact(_user, _args[0], _out, 1);
}


static bool
d_internal_ninja_changes(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_file_fact(_user, _args[0], _out, 2);
}


static bool
d_internal_ninja_statements(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_file_fact(_user, _args[0], _out, 3);
}


static bool
d_internal_ninja_comments(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_file_fact(_user, _args[0], _out, 4);
}


//==============================================================================
// 4.  STATEMENTS
//==============================================================================


/*
d_internal_statement_at
  The statement a handle and an index name, and its records, or NULL,
ninja_error() saying why.
*/
static const struct d_ninja_statement*
d_internal_statement_at(
    struct d_host_ninja*   _host,
    struct d_awk_value*    _handle,
    struct d_awk_value*    _index,
    const struct d_ninja** _out_records
)
{
    const struct d_ninja_doc* const doc = d_internal_file(_host, _handle);

    // a handle to no file
    if (!doc)
    {
        return NULL;
    }

    const struct d_ninja* const records = d_ninja_doc_records(doc);
    const size_t                n       = d_internal_index(_index);

    // a statement the file has
    if ( (n == 0u) ||
         (n > records->statement_count) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "the file has no statement %s",
                       d_internal_text(_index));

        return NULL;
    }

    *_out_records = records;

    return &records->statements[n - 1u];
}


/*
d_internal_unparsed
  A value unparsed, for the caller to free; "" for none.
*/
static char*
d_internal_unparsed(
    const struct d_ninja* _ninja,
    size_t                _value
)
{
    // no value is ""
    if (_value == D_NINJA_NONE)
    {
        return calloc(1u, 1u);
    }

    const struct d_ninja_value* const value = &_ninja->values[_value];
    char* const out = malloc(((value->end - value->start) * 2u) + 1u);

    // the value, unparsed
    if (out)
    {
        (void)d_ninja_unparse(_ninja, value, out);
    }

    return out;
}


/*
d_internal_set_written
  Gives a value as written.
*/
static bool
d_internal_set_written(
    struct d_awk_value*   _out,
    const struct d_ninja* _ninja,
    size_t                _value
)
{
    // no value is ""
    if (_value == D_NINJA_NONE)
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    const struct d_ninja_value* const value = &_ninja->values[_value];

    return d_internal_set_input(_out, &_ninja->text[value->start],
                                value->end - value->start);
}


/*
d_internal_statement_field
  One field of a statement.
  Long as it is a table of the fields, each a line or two.
*/
static bool
d_internal_statement_field(
    struct d_host_ninja*            _host,
    const struct d_ninja*           _ninja,
    const struct d_ninja_statement* _statement,
    const char*                     _field,
    struct d_awk_value*             _out
)
{
    static const char* const fields[] =
    {
        "kind", "name", "value", "value_text", "text", "line", "end_line",
        "paths", "bindings"
    };
    const size_t             count = sizeof(fields) / sizeof(fields[0]);

    // a table of the fields
    switch (d_internal_field(_field, fields, count))
    {
        case 0:
            return d_internal_set_text(_out,
                                       d_ninja_kind_name(_statement->kind));

        case 1:
            return d_internal_set_input(_out,
                                        &_ninja->text[_statement->name_start],
                                        _statement->name_end -
                                            _statement->name_start);

        case 2:
            return d_internal_set_owned(_host, _out,
                                        d_internal_unparsed(_ninja,
                                                            _statement->value),
                                        "out of memory");

        case 3:
            return d_internal_set_written(_out, _ninja, _statement->value);

        case 4:
            return d_awk_value_set_string(_out,
                                          &_ninja->text[_statement->start],
                                          _statement->end -
                                              _statement->start);

        case 5:
            return d_internal_set_number(_out, (double)_statement->line);

        case 6:
            return d_internal_set_number(_out, (double)_statement->end_line);

        case 7:
            return d_internal_set_number(_out,
                                         (double)_statement->path_count);

        case 8:
            return d_internal_set_number(_out,
                                         (double)_statement->binding_count);

        default:
            break;
    }

    return d_internal_unknown_field(_host, _out, _field);
}


/*
ninja_statement(f, n, field)
*/
static bool
d_internal_ninja_statement(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host    = _user;
    const struct d_ninja*      records = NULL;
    char* const                field   = d_internal_copy_arg(_args[2]);

    (void)_count;

    // the field's name, copied
    if (!field)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(host, _args[0], _args[1], &records);
    const bool                            ok        =
        (statement) ? d_internal_statement_field(host, records, statement,
                                                 field, _out)
                    : d_internal_fail(host, _out, host->error);

    free(field);

    return ok;
}


/*
d_internal_names
  Whether a statement is of a name: a variable's, a rule's or a pool's, a
build's outputs, an include's or a subninja's file, a default's targets.
*/
static bool
d_internal_names(
    const struct d_ninja*           _ninja,
    const struct d_ninja_statement* _statement,
    const char*                     _name
)
{
    const size_t span = strlen(_name);

    // a statement with a name of its own
    if (_statement->name_end > _statement->name_start)
    {
        // a build's name is its rule's: its outputs name it
        if (_statement->kind != D_NINJA_BUILD)
        {
            return ( ((_statement->name_end - _statement->name_start) ==
                      span) &&
                     (memcmp(&_ninja->text[_statement->name_start], _name,
                             span) == 0) );
        }
    }

    bool found = false;

    // each path that names it
    for (size_t at = 0; (!found) && (at < _statement->path_count); ++at)
    {
        const struct d_ninja_path* const path =
            &_ninja->paths[_statement->first_path + at];
        char* const text = (path->section != D_NINJA_INPUT) &&
                           (path->section != D_NINJA_IMPLICIT) &&
                           (path->section != D_NINJA_ORDER_ONLY) &&
                           (path->section != D_NINJA_VALIDATION)
                               ? d_internal_unparsed(_ninja, path->value)
                               : NULL;

        found = ( (text) &&
                  (strcmp(text, _name) == 0) );
        free(text);
    }

    return found;
}


/*
ninja_find(f, kind[, name[, after]])
*/
static bool
d_internal_ninja_find(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const      host = _user;
    const struct d_ninja_doc* const doc  = d_internal_file(host, _args[0]);
    char* const                     kind = d_internal_copy_arg(_args[1]);
    char* const                     name = (_count > 2u)
                                               ? d_internal_copy_arg(_args[2])
                                               : NULL;

    (void)_problem;
    (void)_capacity;

    // a file, and the names copied
    if ( (!doc) ||
         (!kind) ||
         ( (_count > 2u) && (!name) ) )
    {
        free(kind);
        free(name);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const struct d_ninja* const records = d_ninja_doc_records(doc);
    const size_t                after   = (_count > 3u)
                                              ? d_internal_index(_args[3])
                                              : 0u;
    size_t                      found   = 0;

    // the first after the place, of the kind and the name
    for (size_t at = after;
         (found == 0u) && (at < records->statement_count);
         ++at)
    {
        const struct d_ninja_statement* const statement =
            &records->statements[at];

        // the kind's, and the name's when one is given
        if ( (strcmp(d_ninja_kind_name(statement->kind), kind) == 0) &&
             ( (!name) || (name[0] == '\0') ||
               (d_internal_names(records, statement, name)) ) )
        {
            found = at + 1u;
        }
    }

    free(kind);
    free(name);

    return d_internal_set_number(_out, (double)found);
}


//==============================================================================
// 5.  PATHS AND BINDINGS
//==============================================================================


/*
ninja_paths(f, n[, section])
*/
static bool
d_internal_ninja_paths(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host    = _user;
    const struct d_ninja*      records = NULL;

    (void)_problem;
    (void)_capacity;

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(host, _args[0], _args[1], &records);

    // a statement the file has
    if (!statement)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const int section = (_count > 2u)
                            ? d_internal_section(d_internal_text(_args[2]))
                            : -2;
    size_t    count   = 0;

    // a section named that none is
    if (section == -1)
    {
        (void)snprintf(host->error, sizeof(host->error), "no section %s",
                       d_internal_text(_args[2]));

        return d_internal_fail(host, _out, host->error);
    }

    // each path of the section, or every path
    for (size_t at = 0; at < statement->path_count; ++at)
    {
        count += ( (section == -2) ||
                   ((int)records->paths[statement->first_path + at].section ==
                    section) ) ? 1u : 0u;
    }

    return d_internal_set_number(_out, (double)count);
}


/*
ninja_path_at(f, n, k[, field])
  Long as it is a table of the fields, each a line or two.
*/
static bool
d_internal_ninja_path_at(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    static const char* const fields[] = { "text", "value", "section", "line" };
    struct d_host_ninja* const host    = _user;
    const struct d_ninja*      records = NULL;
    char* const                field   = (_count > 3u)
                                             ? d_internal_copy_arg(_args[3])
                                             : NULL;

    (void)_problem;
    (void)_capacity;

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(host, _args[0], _args[1], &records);
    const size_t                          k         =
        d_internal_index(_args[2]);

    // a path the statement has
    if ( (!statement) ||
         (k == 0u) ||
         (k > statement->path_count) )
    {
        free(field);

        return d_internal_fail(host, _out,
                               (statement) ? "the statement has no such path"
                                           : host->error);
    }

    const struct d_ninja_path* const path =
        &records->paths[statement->first_path + k - 1u];
    const size_t which = d_internal_field((field) ? field : "text", fields,
                                          4u);
    bool         ok    = false;

    // a table of the fields
    switch (which)
    {
        case 0:
            ok = d_internal_set_written(_out, records, path->value);
            break;

        case 1:
            ok = d_internal_set_owned(host, _out,
                                      d_internal_unparsed(records,
                                                          path->value),
                                      "out of memory");
            break;

        case 2:
            ok = d_internal_set_text(_out,
                                     d_ninja_section_name(path->section));
            break;

        case 3:
            ok = d_internal_set_number(
                     _out,
                     (double)d_ninja_line_of(records,
                                             records->values[path->value]
                                                 .start));
            break;

        default:
            ok = d_internal_unknown_field(host, _out, field);
            break;
    }

    free(field);

    return ok;
}


/*
ninja_bindings(f, n)
*/
static bool
d_internal_ninja_bindings(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host    = _user;
    const struct d_ninja*      records = NULL;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(host, _args[0], _args[1], &records);

    // a statement the file has
    if (!statement)
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_set_number(_out, (double)statement->binding_count);
}


/*
ninja_binding(f, n, k[, field])
  Long as it is a table of the fields, each a line or two.
*/
static bool
d_internal_ninja_binding(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    static const char* const fields[] = { "name", "text", "value", "line" };
    struct d_host_ninja* const host    = _user;
    const struct d_ninja*      records = NULL;
    char* const                field   = (_count > 3u)
                                             ? d_internal_copy_arg(_args[3])
                                             : NULL;

    (void)_problem;
    (void)_capacity;

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(host, _args[0], _args[1], &records);
    const size_t                          k         =
        d_internal_index(_args[2]);

    // a binding the statement has
    if ( (!statement) ||
         (k == 0u) ||
         (k > statement->binding_count) )
    {
        free(field);

        return d_internal_fail(host, _out,
                               (statement) ? "the statement has no such "
                                             "binding"
                                           : host->error);
    }

    const struct d_ninja_binding* const binding =
        &records->bindings[statement->first_binding + k - 1u];
    bool                                ok      = false;

    // a table of the fields
    switch (d_internal_field((field) ? field : "name", fields, 4u))
    {
        case 0:
            ok = d_internal_set_input(_out,
                                      &records->text[binding->name_start],
                                      binding->name_end -
                                          binding->name_start);
            break;

        case 1:
            ok = d_internal_set_written(_out, records, binding->value);
            break;

        case 2:
            ok = d_internal_set_owned(host, _out,
                                      d_internal_unparsed(records,
                                                          binding->value),
                                      "out of memory");
            break;

        case 3:
            ok = d_internal_set_number(_out, (double)binding->line);
            break;

        default:
            ok = d_internal_unknown_field(host, _out, field);
            break;
    }

    free(field);

    return ok;
}


/*
ninja_find_binding(f, n, name)
*/
static bool
d_internal_ninja_find_binding(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host    = _user;
    const struct d_ninja*      records = NULL;
    char* const                name    = d_internal_copy_arg(_args[2]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(host, _args[0], _args[1], &records);

    // a statement the file has, and the name copied
    if ( (!statement) ||
         (!name) )
    {
        free(name);

        return d_internal_fail(host, _out, (statement) ? "out of memory"
                                                       : host->error);
    }

    const size_t span  = strlen(name);
    size_t       found = 0;

    // each binding, the last of the name kept
    for (size_t at = 0; at < statement->binding_count; ++at)
    {
        const struct d_ninja_binding* const binding =
            &records->bindings[statement->first_binding + at];

        // the name's
        if ( ((binding->name_end - binding->name_start) == span) &&
             (memcmp(&records->text[binding->name_start], name, span) == 0) )
        {
            found = at + 1u;
        }
    }

    free(name);

    return d_internal_set_number(_out, (double)found);
}


//==============================================================================
// 6.  COMMENTS
//==============================================================================


/*
ninja_comment(f, i, field)
*/
static bool
d_internal_ninja_comment(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    static const char* const fields[] =
    {
        "text", "body", "line", "statement"
    };
    struct d_host_ninja* const      host  = _user;
    const struct d_ninja_doc* const doc   = d_internal_file(host, _args[0]);
    const size_t                    index = d_internal_index(_args[1]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a comment the file has
    if ( (!doc) ||
         (index == 0u) ||
         (index > d_ninja_doc_records(doc)->comment_count) )
    {
        return d_internal_fail(host, _out,
                               (doc) ? "the file has no such comment"
                                     : host->error);
    }

    const struct d_ninja* const         records = d_ninja_doc_records(doc);
    const struct d_ninja_comment* const comment =
        &records->comments[index - 1u];
    const char* const                   field   = d_internal_text(_args[2]);

    // a table of the fields
    switch (d_internal_field(field, fields, 4u))
    {
        case 0:
            return d_awk_value_set_string(_out,
                                          &records->text[comment->start],
                                          comment->end - comment->start);

        case 1:
            return d_awk_value_set_string(_out,
                                          &records->text[comment->start + 1u],
                                          comment->end - comment->start - 1u);

        case 2:
            return d_internal_set_number(_out, (double)comment->line);

        case 3:
            return d_internal_set_number(_out,
                                         (comment->within == D_NINJA_NONE)
                                             ? 0.0
                                             : (double)(comment->within + 1u));

        default:
            break;
    }

    return d_internal_unknown_field(host, _out, field);
}


//==============================================================================
// 7.  CHANGES
//==============================================================================


// d_internal_edit
//   enum: the changes ninja_set_value ... ninja_remove make.
enum d_internal_edit
{
    D_INTERNAL_SET_VALUE = 0,
    D_INTERNAL_SET_NAME,
    D_INTERNAL_SET_BINDING,
    D_INTERNAL_REMOVE_BINDING,
    D_INTERNAL_SET_PATH,
    D_INTERNAL_INSERT_PATH,
    D_INTERNAL_REMOVE_PATH,
    D_INTERNAL_INSERT,
    D_INTERNAL_REPLACE,
    D_INTERNAL_REMOVE
};


/*
d_internal_less_one
  An index from 1 as an index from 0; one that names nothing, 0 here, is
past any the file has.
*/
static size_t
d_internal_less_one(
    struct d_awk_value* _value
)
{
    const size_t index = d_internal_index(_value);

    return (index > 0u) ? (index - 1u) : D_NINJA_NONE;
}


/*
d_internal_insert_path
  A path inserted as the kth of its named section.
*/
static bool
d_internal_insert_path(
    struct d_ninja_doc*  _doc,
    struct d_awk_value** _args,
    const char*          _text
)
{
    const int section = d_internal_section(d_internal_text(_args[2]));

    // a section by its name
    if (section < 0)
    {
        return false;
    }

    return d_ninja_doc_insert_path(_doc, d_internal_less_one(_args[1]),
                                   (enum d_ninja_section)section,
                                   d_internal_less_one(_args[3]), _text);
}


/*
d_internal_edit_doc
  Makes a change to a document, its indices from 0; `_name` and `_text`
are the copies of the arguments the change takes.
  Long as it is a table of the changes, each a call.
*/
static bool
d_internal_edit_doc(
    struct d_ninja_doc*  _doc,
    enum d_internal_edit _edit,
    struct d_awk_value** _args,
    size_t               _count,
    const char*          _name,
    const char*          _text
)
{
    const size_t n = d_internal_less_one(_args[1]);

    // a table of the changes
    switch (_edit)
    {
        case D_INTERNAL_SET_VALUE:
            return d_ninja_doc_set_value(_doc, n, _text);

        case D_INTERNAL_SET_NAME:
            return d_ninja_doc_set_name(_doc, n, _text);

        case D_INTERNAL_SET_BINDING:
            return d_ninja_doc_set_binding(_doc, n, _name, _text);

        case D_INTERNAL_REMOVE_BINDING:
            return d_ninja_doc_remove_binding(_doc, n, _name);

        case D_INTERNAL_SET_PATH:
            return d_ninja_doc_set_path(_doc, n, d_internal_less_one(_args[2]),
                                        _text);

        case D_INTERNAL_INSERT_PATH:
            return d_internal_insert_path(_doc, _args, _text);

        case D_INTERNAL_REMOVE_PATH:
            return d_ninja_doc_remove_path(_doc, n,
                                           d_internal_less_one(_args[2]));

        case D_INTERNAL_INSERT:
            return d_ninja_doc_insert_statements(_doc, n, _text);

        case D_INTERNAL_REPLACE:
            return d_ninja_doc_set_statement(_doc, n, _text);

        case D_INTERNAL_REMOVE:
        default:
            break;
    }

    return d_ninja_doc_remove_statements(_doc, n,
                                         (_count > 2u)
                                             ? d_internal_less_one(_args[2])
                                             : n);
}


/*
d_internal_edit_arguments
  Which arguments hold a change's name and its text: 0 for none.
*/
static void
d_internal_edit_arguments(
    enum d_internal_edit _edit,
    size_t*              _out_name,
    size_t*              _out_text
)
{
    static const size_t names[] = { 0u, 0u, 2u, 2u, 0u, 0u, 0u, 0u, 0u, 0u };
    static const size_t texts[] = { 2u, 2u, 3u, 0u, 3u, 4u, 0u, 2u, 2u, 0u };

    *_out_name = names[_edit];
    *_out_text = texts[_edit];

    return;
}


/*
d_internal_edit
  A change, by what it is, to the file the first argument names.
*/
static bool
d_internal_edit(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    enum d_internal_edit _edit
)
{
    struct d_host_ninja* const host = _user;
    struct d_ninja_doc* const  doc  = d_internal_file(host, _args[0]);
    size_t                     name = 0;
    size_t                     text = 0;

    // a handle to no file
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    // a path's section by a name it has
    if ( (_edit == D_INTERNAL_INSERT_PATH) &&
         (d_internal_section(d_internal_text(_args[2])) < 0) )
    {
        (void)snprintf(host->error, sizeof(host->error),
                       "no section %s: output, implicit_output, input, "
                       "implicit, order_only, validation or target",
                       d_internal_text(_args[2]));

        return d_internal_fail(host, _out, host->error);
    }

    d_internal_edit_arguments(_edit, &name, &text);

    char* const named   = (name > 0u) ? d_internal_copy_arg(_args[name])
                                      : NULL;
    char* const written = (text > 0u) ? d_internal_copy_arg(_args[text])
                                      : NULL;

    // the arguments copied
    if ( ( (name > 0u) && (!named) ) ||
         ( (text > 0u) && (!written) ) )
    {
        free(named);
        free(written);

        return d_internal_fail(host, _out, "out of memory");
    }

    const bool ok = d_internal_edit_doc(doc, _edit, _args, _count,
                                        (named) ? named : "",
                                        (written) ? written : "");

    free(named);
    free(written);

    // refused: the document says why
    if (!ok)
    {
        return d_internal_fail(host, _out, d_ninja_doc_problem(doc));
    }

    return d_internal_set_number(_out, 1.0);
}


/*
ninja_set_value, ninja_set_name, ninja_set_binding, ninja_remove_binding,
ninja_set_path, ninja_insert_path, ninja_remove_path, ninja_insert,
ninja_replace, ninja_remove
*/
static bool
d_internal_ninja_set_value(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_problem;
    (void)_capacity;

    return d_internal_edit(_user, _args, _count, _out, D_INTERNAL_SET_VALUE);
}


static bool
d_internal_ninja_set_name(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_problem;
    (void)_capacity;

    return d_internal_edit(_user, _args, _count, _out, D_INTERNAL_SET_NAME);
}


static bool
d_internal_ninja_set_binding(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_problem;
    (void)_capacity;

    return d_internal_edit(_user, _args, _count, _out,
                           D_INTERNAL_SET_BINDING);
}


static bool
d_internal_ninja_remove_binding(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_problem;
    (void)_capacity;

    return d_internal_edit(_user, _args, _count, _out,
                           D_INTERNAL_REMOVE_BINDING);
}


static bool
d_internal_ninja_set_path(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_problem;
    (void)_capacity;

    return d_internal_edit(_user, _args, _count, _out, D_INTERNAL_SET_PATH);
}


static bool
d_internal_ninja_insert_path(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_problem;
    (void)_capacity;

    return d_internal_edit(_user, _args, _count, _out,
                           D_INTERNAL_INSERT_PATH);
}


static bool
d_internal_ninja_remove_path(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_problem;
    (void)_capacity;

    return d_internal_edit(_user, _args, _count, _out,
                           D_INTERNAL_REMOVE_PATH);
}


static bool
d_internal_ninja_insert(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_problem;
    (void)_capacity;

    return d_internal_edit(_user, _args, _count, _out, D_INTERNAL_INSERT);
}


static bool
d_internal_ninja_replace(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_problem;
    (void)_capacity;

    return d_internal_edit(_user, _args, _count, _out, D_INTERNAL_REPLACE);
}


static bool
d_internal_ninja_remove(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_problem;
    (void)_capacity;

    return d_internal_edit(_user, _args, _count, _out, D_INTERNAL_REMOVE);
}


//==============================================================================
// 8.  WRITING
//==============================================================================


/*
ninja_escape(text[, how])
*/
static bool
d_internal_ninja_escape(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host = _user;
    size_t                     length = 0;
    char* const                text   = d_internal_copy_arg(_args[0]);
    const char* const          how    = (_count > 1u)
                                            ? d_internal_text(_args[1])
                                            : "path";

    (void)_problem;
    (void)_capacity;

    // the text copied
    if (!text)
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    const bool  path    = (strcmp(how, "path") == 0);
    const bool  newline = (strcmp(how, "newline") == 0);
    char* const escaped = d_ninja_escape(text, strlen(text), path, newline,
                                         &length);

    free(text);

    // a text ninja has no way to write so
    if (!escaped)
    {
        return d_internal_fail(host, _out,
                               (path) ? "a path cannot hold a `|`, a newline, "
                                        "a CR or a NUL"
                                      : "a value cannot hold a CR or a NUL, "
                                        "nor a newline unless written `$^`");
    }

    const bool set = d_awk_value_set_string(_out, escaped, length);

    free(escaped);

    return set;
}


/*
ninja_canonical(path[, win32]), ninja_shell_escape(path[, win32])
*/
static bool
d_internal_ninja_canonical(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    size_t            length = 0;
    const char* const path   = d_awk_value_text(_args[0], NULL, &length);
    char* const       out    = malloc(length + 3u);
    const bool        win32  = (_count > 1u) &&
                               (d_awk_value_number(_args[1]) != 0.0);

    (void)_problem;
    (void)_capacity;

    // room for the path
    if (!out)
    {
        return d_internal_fail(_user, _out, "out of memory");
    }

    const size_t span = d_ninja_canonical(path, length, win32, out);
    const bool   set  = d_awk_value_set_string(_out, out, span);

    free(out);

    return set;
}


static bool
d_internal_ninja_shell_escape(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    size_t            length = 0;
    const char* const path   = d_awk_value_text(_args[0], NULL, &length);
    const bool        win32  = (_count > 1u) &&
                               (d_awk_value_number(_args[1]) != 0.0);
    size_t            span   = 0;
    char* const       out    = d_ninja_shell_escape(path, length, win32,
                                                    &span);

    (void)_problem;
    (void)_capacity;

    // room for the path
    if (!out)
    {
        return d_internal_fail(_user, _out, "out of memory");
    }

    const bool set = d_awk_value_set_string(_out, out, span);

    free(out);

    return set;
}


//==============================================================================
// 9.  BUILDS
//==============================================================================


/*
d_internal_graph
  The build a handle names, or NULL, ninja_error() saying why.
*/
static struct d_ninja_graph*
d_internal_graph(
    struct d_host_ninja* _host,
    struct d_awk_value*  _handle
)
{
    const size_t handle = d_internal_index(_handle);

    // a handle in the list, to a build not let go
    if ( (handle == 0u) ||
         (handle > _host->graph_count) ||
         (!_host->graphs[handle - 1u]) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "no build has the handle %s", d_internal_text(_handle));

        return NULL;
    }

    return _host->graphs[handle - 1u];
}


/*
ninja_load(path[, directory[, version]])
*/
static bool
d_internal_ninja_load(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host      = _user;
    char* const                path      = d_internal_copy_arg(_args[0]);
    char* const                directory = (_count > 1u)
                                               ? d_internal_copy_arg(_args[1])
                                               : NULL;
    char* const                version   = (_count > 2u)
                                               ? d_internal_copy_arg(_args[2])
                                               : NULL;
    char                       why[768];

    (void)_problem;
    (void)_capacity;

    struct d_ninja_options options;

    options.directory = ( (directory) && (directory[0] != '\0') ) ? directory
                                                                  : NULL;
    options.version   = ( (version) && (version[0] != '\0') ) ? version
                                                              : NULL;
    options.win32     = false;

    struct d_ninja_graph* const graph =
        (path) ? d_ninja_load(path, &options, why, sizeof(why)) : NULL;

    free(path);
    free(directory);
    free(version);

    // why it would not load
    if (!graph)
    {
        return d_internal_fail(host, _out, why);
    }

    // room for one more
    if (host->graph_count >= host->graph_capacity)
    {
        const size_t           wanted = (host->graph_capacity > 0u)
                                            ? (host->graph_capacity * 2u)
                                            : 4u;
        struct d_ninja_graph** grown  = realloc(host->graphs,
                                                wanted * sizeof(*grown));

        // a build that cannot be kept is let go
        if (!grown)
        {
            d_ninja_graph_free(graph);

            return d_internal_fail(host, _out, "out of memory");
        }

        host->graphs         = grown;
        host->graph_capacity = wanted;
    }

    host->graphs[host->graph_count++] = graph;

    return d_internal_set_number(_out, (double)host->graph_count);
}


/*
ninja_unload(g)
*/
static bool
d_internal_ninja_unload(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host   = _user;
    const size_t               handle = d_internal_index(_args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a build loaded, let go, its place kept
    if ( (handle == 0u) ||
         (handle > host->graph_count) ||
         (!host->graphs[handle - 1u]) )
    {
        return d_internal_set_number(_out, 0.0);
    }

    d_ninja_graph_free(host->graphs[handle - 1u]);
    host->graphs[handle - 1u] = NULL;

    return d_internal_set_number(_out, 1.0);
}


/*
d_internal_node_lines
  Nodes' paths, a line each.
*/
static bool
d_internal_node_lines(
    struct d_host_ninja*        _host,
    const struct d_ninja_graph* _graph,
    const size_t*               _nodes,
    size_t                      _count,
    struct d_awk_value*         _out
)
{
    struct d_internal_lines lines = { NULL, 0u, 0u, 0u, false };

    // each node's path
    for (size_t at = 0; at < _count; ++at)
    {
        const char* const path = d_ninja_node_path(_graph, _nodes[at]);

        d_internal_add_line(&lines, path, strlen(path));
    }

    return d_internal_give_lines(_host, _out, &lines);
}


/*
d_internal_number_lines
  Indices, each one more, a line each.
*/
static bool
d_internal_number_lines(
    struct d_host_ninja* _host,
    const size_t*        _items,
    size_t               _count,
    struct d_awk_value*  _out
)
{
    struct d_internal_lines lines = { NULL, 0u, 0u, 0u, false };

    // each index, from 1
    for (size_t at = 0; at < _count; ++at)
    {
        char         number[32];
        const size_t length = (size_t)snprintf(number, sizeof(number), "%zu",
                                               _items[at] + 1u);

        d_internal_add_line(&lines, number, length);
    }

    return d_internal_give_lines(_host, _out, &lines);
}


/*
ninja_edges(g), ninja_nodes(g)
*/
static bool
d_internal_ninja_edges(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const        host  = _user;
    const struct d_ninja_graph* const graph = d_internal_graph(host,
                                                               _args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a build loaded
    if (!graph)
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_set_number(_out, (double)d_ninja_edge_count(graph));
}


static bool
d_internal_ninja_nodes(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const        host  = _user;
    const struct d_ninja_graph* const graph = d_internal_graph(host,
                                                               _args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a build loaded
    if (!graph)
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_set_number(_out, (double)d_ninja_node_count(graph));
}


/*
d_internal_edge_at
  The edge a handle and an index name, from 0, or D_NINJA_NONE,
ninja_error() saying why.
*/
static size_t
d_internal_edge_at(
    struct d_host_ninja*         _host,
    struct d_awk_value*          _handle,
    struct d_awk_value*          _index,
    struct d_ninja_graph**       _out_graph
)
{
    struct d_ninja_graph* const graph = d_internal_graph(_host, _handle);
    const size_t                e     = d_internal_index(_index);

    // a build loaded, and an edge it has
    if ( (!graph) ||
         (e == 0u) ||
         (e > d_ninja_edge_count(graph)) )
    {
        // an edge not there says so
        if (graph)
        {
            (void)snprintf(_host->error, sizeof(_host->error),
                           "the build has no edge %s",
                           d_internal_text(_index));
        }

        return D_NINJA_NONE;
    }

    *_out_graph = graph;

    return e - 1u;
}


/*
ninja_edge(g, e, field)
  Long as it is a table of the fields, each a line or two.
*/
static bool
d_internal_ninja_edge(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    static const char* const fields[] =
    {
        "outputs", "implicit_outputs", "inputs", "implicit", "order_only",
        "validations", "rule", "phony", "pool", "file", "line"
    };
    struct d_host_ninja* const host  = _user;
    struct d_ninja_graph*      graph = NULL;
    char* const                field = d_internal_copy_arg(_args[2]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    const size_t edge = d_internal_edge_at(host, _args[0], _args[1], &graph);

    // an edge the build has, and the field copied
    if ( (edge == D_NINJA_NONE) ||
         (!field) )
    {
        free(field);

        return d_internal_fail(host, _out, (field) ? host->error
                                                   : "out of memory");
    }

    const size_t which = d_internal_field(field, fields, 11u);
    bool         ok    = false;

    // a table of the fields: the six sections, then the rest
    if (which < 6u)
    {
        const size_t* nodes = NULL;
        const size_t  count = d_ninja_edge_nodes(graph, edge,
                                                 (enum d_ninja_section)which,
                                                 &nodes);

        ok = d_internal_node_lines(host, graph, nodes, count, _out);
    }
    else if (which == 6u)
    {
        ok = d_internal_set_text(_out, d_ninja_edge_rule(graph, edge));
    }
    else if (which == 7u)
    {
        ok = d_internal_set_number(_out,
                                   (d_ninja_edge_phony(graph, edge)) ? 1.0
                                                                     : 0.0);
    }
    else if (which == 8u)
    {
        ok = d_internal_set_text(_out, d_ninja_edge_pool(graph, edge));
    }
    else if (which == 9u)
    {
        ok = d_internal_set_text(_out, d_ninja_edge_file(graph, edge));
    }
    else if (which == 10u)
    {
        ok = d_internal_set_number(_out,
                                   (double)d_ninja_edge_line(graph, edge));
    }
    else
    {
        ok = d_internal_unknown_field(host, _out, field);
    }

    free(field);

    return ok;
}


/*
ninja_edge_var(g, e, name[, raw])
*/
static bool
d_internal_ninja_edge_var(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const host  = _user;
    struct d_ninja_graph*      graph = NULL;
    char* const                name  = d_internal_copy_arg(_args[2]);
    char                       why[512];

    (void)_problem;
    (void)_capacity;

    const size_t edge = d_internal_edge_at(host, _args[0], _args[1], &graph);
    const bool   raw  = (_count > 3u) &&
                        (d_awk_value_number(_args[3]) != 0.0);

    // an edge the build has, and the name copied
    if ( (edge == D_NINJA_NONE) ||
         (!name) )
    {
        free(name);

        return d_internal_fail(host, _out, (name) ? host->error
                                                  : "out of memory");
    }

    char* const value = d_ninja_edge_binding(graph, edge, name, !raw, why,
                                             sizeof(why));

    free(name);

    return d_internal_set_owned(host, _out, value, why);
}


/*
ninja_node(g, n, field)
*/
static bool
d_internal_ninja_node(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    static const char* const fields[] =
    {
        "path", "in_edge", "out_edges", "validation_edges"
    };
    struct d_host_ninja* const        host  = _user;
    const struct d_ninja_graph* const graph = d_internal_graph(host,
                                                               _args[0]);
    const size_t                      n     = d_internal_index(_args[1]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a node the build has
    if ( (!graph) ||
         (n == 0u) ||
         (n > d_ninja_node_count(graph)) )
    {
        return d_internal_fail(host, _out,
                               (graph) ? "the build has no such node"
                                       : host->error);
    }

    const char* const field = d_internal_text(_args[2]);
    const size_t*     edges = NULL;
    size_t            count = 0;

    // a table of the fields
    switch (d_internal_field(field, fields, 4u))
    {
        case 0:
            return d_internal_set_text(_out, d_ninja_node_path(graph,
                                                               n - 1u));

        case 1:
        {
            const size_t edge = d_ninja_node_in_edge(graph, n - 1u);

            return d_internal_set_number(_out,
                                         (edge == D_NINJA_NONE)
                                             ? 0.0 : (double)(edge + 1u));
        }

        case 2:
            count = d_ninja_node_out_edges(graph, n - 1u, &edges);

            return d_internal_number_lines(host, edges, count, _out);

        case 3:
            count = d_ninja_node_validation_edges(graph, n - 1u, &edges);

            return d_internal_number_lines(host, edges, count, _out);

        default:
            break;
    }

    return d_internal_unknown_field(host, _out, field);
}


/*
ninja_find_node(g, path)
*/
static bool
d_internal_ninja_find_node(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const        host   = _user;
    const struct d_ninja_graph* const graph  = d_internal_graph(host,
                                                                _args[0]);
    size_t                            length = 0;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a build loaded
    if (!graph)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const char* const path = d_awk_value_text(_args[1], NULL, &length);
    const size_t      node = d_ninja_node_find(graph, path, length);

    return d_internal_set_number(_out,
                                 (node == D_NINJA_NONE) ? 0.0
                                                        : (double)(node + 1u));
}


/*
d_internal_build_list
  One of a build's lists, a line each: 0 its defaults, 1 its roots, 2 its
rules, 3 its pools, 4 its files, 5 its warnings.
  Long as it is a table of the lists.
*/
static bool
d_internal_build_list(
    void*               _user,
    struct d_awk_value* _handle,
    struct d_awk_value* _out,
    int                 _list
)
{
    struct d_host_ninja* const        host  = _user;
    const struct d_ninja_graph* const graph = d_internal_graph(host,
                                                               _handle);
    struct d_internal_lines           lines = { NULL, 0u, 0u, 0u, false };
    const size_t*                     nodes = NULL;

    // a build loaded
    if (!graph)
    {
        return d_internal_fail(host, _out, host->error);
    }

    // a table of the lists: two of nodes, four of names
    if (_list < 2)
    {
        const size_t count = (_list == 0) ? d_ninja_defaults(graph, &nodes)
                                          : d_ninja_roots(graph, &nodes);

        return d_internal_node_lines(host, graph, nodes, count, _out);
    }

    const size_t count = (_list == 2) ? d_ninja_rule_count(graph)
                       : (_list == 3) ? d_ninja_pool_count(graph)
                       : (_list == 4) ? d_ninja_file_count(graph)
                       :                d_ninja_warning_count(graph);

    // each name
    for (size_t at = 0; at < count; ++at)
    {
        const char* const name = (_list == 2) ? d_ninja_rule_name(graph, at)
                               : (_list == 3) ? d_ninja_pool_name(graph, at)
                               : (_list == 4) ? d_ninja_file_path(graph, at)
                               :                d_ninja_warning(graph, at);

        // the default pool, which has no name, is not listed
        if (name[0] != '\0')
        {
            d_internal_add_line(&lines, name, strlen(name));
        }
    }

    return d_internal_give_lines(host, _out, &lines);
}


/*
ninja_defaults(g), ninja_roots(g), ninja_rules(g), ninja_pools(g),
ninja_files(g), ninja_warnings(g)
*/
static bool
d_internal_ninja_defaults(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_build_list(_user, _args[0], _out, 0);
}


static bool
d_internal_ninja_roots(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_build_list(_user, _args[0], _out, 1);
}


static bool
d_internal_ninja_rules(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_build_list(_user, _args[0], _out, 2);
}


static bool
d_internal_ninja_pools(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_build_list(_user, _args[0], _out, 3);
}


static bool
d_internal_ninja_files(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_build_list(_user, _args[0], _out, 4);
}


static bool
d_internal_ninja_warnings(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_build_list(_user, _args[0], _out, 5);
}


/*
d_internal_rule_named
  The rule of a name among a build's, or D_NINJA_NONE.
*/
static size_t
d_internal_rule_named(
    const struct d_ninja_graph* _graph,
    const char*                 _name
)
{
    // each rule, by name
    for (size_t at = 0; at < d_ninja_rule_count(_graph); ++at)
    {
        // the one of the name
        if (strcmp(d_ninja_rule_name(_graph, at), _name) == 0)
        {
            return at;
        }
    }

    return D_NINJA_NONE;
}


/*
d_internal_rule_names
  The names a rule binds, a line each, in the order ninja lists the names a
rule may bind.
*/
static bool
d_internal_rule_names(
    struct d_host_ninja*        _host,
    const struct d_ninja_graph* _graph,
    size_t                      _rule,
    struct d_awk_value*         _out
)
{
    static const char* const names[] =
    {
        "command", "depfile", "dyndep", "description", "deps", "generator",
        "pool", "restat", "rspfile", "rspfile_content", "msvc_deps_prefix"
    };
    struct d_internal_lines  lines = { NULL, 0u, 0u, 0u, false };

    // each name a rule may bind, listed where it binds it
    for (size_t at = 0; at < (sizeof(names) / sizeof(names[0])); ++at)
    {
        char* const value = d_ninja_rule_binding(_graph, _rule, names[at]);

        // a binding the rule has
        if (value)
        {
            d_internal_add_line(&lines, names[at], strlen(names[at]));
        }

        free(value);
    }

    return d_internal_give_lines(_host, _out, &lines);
}


/*
ninja_rule(g, name[, binding])
*/
static bool
d_internal_ninja_rule(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const        host  = _user;
    const struct d_ninja_graph* const graph = d_internal_graph(host,
                                                               _args[0]);

    (void)_problem;
    (void)_capacity;

    // a build loaded
    if (!graph)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const size_t rule = d_internal_rule_named(graph,
                                              d_internal_text(_args[1]));

    // a rule the build has
    if (rule == D_NINJA_NONE)
    {
        (void)snprintf(host->error, sizeof(host->error),
                       "the build has no rule %s", d_internal_text(_args[1]));

        return d_internal_fail(host, _out, host->error);
    }

    // with no binding named, the names it binds
    if (_count < 3u)
    {
        return d_internal_rule_names(host, graph, rule, _out);
    }

    const char* const binding = d_internal_text(_args[2]);
    char* const       value   = d_ninja_rule_binding(graph, rule, binding);

    // a binding the rule has not
    if (!value)
    {
        (void)snprintf(host->error, sizeof(host->error),
                       "the rule binds no %s", binding);

        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_set_owned(host, _out, value, "out of memory");
}


/*
ninja_pool(g, name), ninja_variable(g, name)
*/
static bool
d_internal_ninja_pool(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const        host  = _user;
    const struct d_ninja_graph* const graph = d_internal_graph(host,
                                                               _args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a build loaded
    if (!graph)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const char* const name  = d_internal_text(_args[1]);
    int               depth = -1;

    // the pool of the name
    for (size_t at = 0; at < d_ninja_pool_count(graph); ++at)
    {
        depth = (strcmp(d_ninja_pool_name(graph, at), name) == 0)
                    ? d_ninja_pool_depth(graph, at) : depth;
    }

    return d_internal_set_number(_out, (double)depth);
}


static bool
d_internal_ninja_variable(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    d_internal_clear(_user);

    struct d_host_ninja* const        host  = _user;
    const struct d_ninja_graph* const graph = d_internal_graph(host,
                                                               _args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a build loaded
    if (!graph)
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_set_owned(host, _out,
                                d_ninja_variable(graph,
                                                 d_internal_text(_args[1])),
                                "out of memory");
}


//==============================================================================
// 10. THE EXTENSION
//==============================================================================


/*
d_internal_functions
  The extension's host functions, as a program calls them.
*/
static const struct d_awk_host_function*
d_internal_functions(
    size_t* _out_count
)
{
    static const struct d_awk_host_function functions[] =
    {
        { "ninja_open",           1u, 1u, d_internal_ninja_open           },
        { "ninja_new",            1u, 2u, d_internal_ninja_new            },
        { "ninja_save",           1u, 2u, d_internal_ninja_save           },
        { "ninja_close",          1u, 1u, d_internal_ninja_close          },
        { "ninja_error",          0u, 0u, d_internal_ninja_error          },
        { "ninja_path",           1u, 1u, d_internal_ninja_path           },
        { "ninja_text",           1u, 1u, d_internal_ninja_text           },
        { "ninja_changes",        1u, 1u, d_internal_ninja_changes        },
        { "ninja_statements",     1u, 1u, d_internal_ninja_statements     },
        { "ninja_statement",      3u, 3u, d_internal_ninja_statement      },
        { "ninja_find",           2u, 4u, d_internal_ninja_find           },
        { "ninja_paths",          2u, 3u, d_internal_ninja_paths          },
        { "ninja_path_at",        3u, 4u, d_internal_ninja_path_at        },
        { "ninja_bindings",       2u, 2u, d_internal_ninja_bindings       },
        { "ninja_binding",        3u, 4u, d_internal_ninja_binding        },
        { "ninja_find_binding",   3u, 3u, d_internal_ninja_find_binding   },
        { "ninja_comments",       1u, 1u, d_internal_ninja_comments       },
        { "ninja_comment",        3u, 3u, d_internal_ninja_comment        },
        { "ninja_set_value",      3u, 3u, d_internal_ninja_set_value      },
        { "ninja_set_name",       3u, 3u, d_internal_ninja_set_name       },
        { "ninja_set_binding",    4u, 4u, d_internal_ninja_set_binding    },
        { "ninja_remove_binding", 3u, 3u, d_internal_ninja_remove_binding },
        { "ninja_set_path",       4u, 4u, d_internal_ninja_set_path       },
        { "ninja_insert_path",    5u, 5u, d_internal_ninja_insert_path    },
        { "ninja_remove_path",    3u, 3u, d_internal_ninja_remove_path    },
        { "ninja_insert",         3u, 3u, d_internal_ninja_insert         },
        { "ninja_replace",        3u, 3u, d_internal_ninja_replace        },
        { "ninja_remove",         2u, 3u, d_internal_ninja_remove         },
        { "ninja_escape",         1u, 2u, d_internal_ninja_escape         },
        { "ninja_canonical",      1u, 2u, d_internal_ninja_canonical      },
        { "ninja_shell_escape",   1u, 2u, d_internal_ninja_shell_escape   },
        { "ninja_load",           1u, 3u, d_internal_ninja_load           },
        { "ninja_unload",         1u, 1u, d_internal_ninja_unload         },
        { "ninja_edges",          1u, 1u, d_internal_ninja_edges          },
        { "ninja_edge",           3u, 3u, d_internal_ninja_edge           },
        { "ninja_edge_var",       3u, 4u, d_internal_ninja_edge_var       },
        { "ninja_nodes",          1u, 1u, d_internal_ninja_nodes          },
        { "ninja_node",           3u, 3u, d_internal_ninja_node           },
        { "ninja_find_node",      2u, 2u, d_internal_ninja_find_node      },
        { "ninja_defaults",       1u, 1u, d_internal_ninja_defaults       },
        { "ninja_roots",          1u, 1u, d_internal_ninja_roots          },
        { "ninja_rules",          1u, 1u, d_internal_ninja_rules          },
        { "ninja_pools",          1u, 1u, d_internal_ninja_pools          },
        { "ninja_files",          1u, 1u, d_internal_ninja_files          },
        { "ninja_warnings",       1u, 1u, d_internal_ninja_warnings       },
        { "ninja_rule",           2u, 3u, d_internal_ninja_rule           },
        { "ninja_pool",           2u, 2u, d_internal_ninja_pool           },
        { "ninja_variable",       2u, 2u, d_internal_ninja_variable       }
    };

    *_out_count = sizeof(functions) / sizeof(functions[0]);

    return functions;
}


struct d_host_ninja*
d_host_ninja_new(void)
{
    return calloc(1u, sizeof(struct d_host_ninja));
}


void
d_host_ninja_free(
    struct d_host_ninja* _host
)
{
    // freeing nothing is allowed
    if (!_host)
    {
        return;
    }

    // each file still open
    for (size_t index = 0; index < _host->file_count; ++index)
    {
        d_ninja_doc_free(_host->files[index]);
    }

    // each build still loaded
    for (size_t index = 0; index < _host->graph_count; ++index)
    {
        d_ninja_graph_free(_host->graphs[index]);
    }

    free(_host->files);
    free(_host->graphs);
    free(_host);

    return;
}


bool
d_host_ninja_install(
    struct d_host_ninja* _host,
    struct d_awk_interp* _interp
)
{
    size_t                                  count     = 0;
    const struct d_awk_host_function* const functions =
        d_internal_functions(&count);

    // parameter validation first
    if ( (!_host) ||
         (!_interp) )
    {
        return false;
    }

    return d_awk_interp_add_functions(_interp, functions, count, _host);
}
