/*******************************************************************************
* djinterp [tools]                                                 dhost_cmake.c
*
* Definitions for the declarations in dhost_cmake.h.
*   Each file open is a dcmake document; its handle is its place in the list
* of files, from 1, and a file closed keeps its place, empty, so a handle to
* it names nothing rather than another file. What a file holds is read from
* the document's records as they are now, each field by its name; a change
* is the document's, which keeps it or says why not.
*
* path:      /src/djinterp/tools/dawk/ext/cmake/dhost_cmake.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/cmake/dhost_cmake.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, calloc, free
#include <string.h>  // memcpy, strcmp, strlen
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dhost.h"                 // d_awk_host_function
#include "../../../../../../inc/djinterp/tools/dawk/dvalue.h"                // d_awk_value_text
#include "../../../../../../inc/djinterp/tools/dawk/ext/cmake/dcmake.h"      // d_cmake
#include "../../../../../../inc/djinterp/tools/dawk/ext/cmake/dcmake_doc.h"  // d_cmake_doc


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Extension
//------------------------------------------------------------------------------
// 1.1.1
// d_host_cmake
//   struct: the files, open or closed, and the last failure.
struct d_host_cmake
{
    struct d_cmake_doc** files;
    size_t               file_count;
    size_t               file_capacity;
    char                 error[512];
};

// 1.1.2
// D_INTERNAL_CALL_VALUES
//   constant: the most values cmake_call writes, as a host function takes
//   sixteen arguments and the first is the name.
#define D_INTERNAL_CALL_VALUES 15u


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
  Gives -1 for a failure a program can recover from, cmake_error() saying
why.
*/
static bool
d_internal_fail(
    struct d_host_cmake* _host,
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
d_internal_set_text, d_internal_set_input, d_internal_set_number
  Gives a string; text of the file, which compares as a number when it
looks like one, as a field does; or a number.
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
d_internal_file
  The file a handle names, or NULL, cmake_error() saying why.
*/
static struct d_cmake_doc*
d_internal_file(
    struct d_host_cmake* _host,
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
  Gives "" for a field not known, cmake_error() saying so.
*/
static bool
d_internal_unknown_field(
    struct d_host_cmake* _host,
    struct d_awk_value*  _out,
    const char*          _field
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
    struct d_host_cmake*      _host,
    struct d_awk_value*       _out,
    const struct d_cmake_doc* _doc,
    bool                      _ok
)
{
    // refused: the document says why
    if (!_ok)
    {
        return d_internal_fail(_host, _out, d_cmake_doc_problem(_doc));
    }

    return d_internal_set_number(_out, 1.0);
}


/*
d_internal_value_of
  An argument's value, allocated for the caller to free; NULL when memory
runs out.
*/
static char*
d_internal_value_of(
    const struct d_cmake*          _cmake,
    const struct d_cmake_argument* _argument,
    size_t*                        _out_length
)
{
    char* const value = malloc((_argument->end - _argument->start) + 1u);

    // the value, when there is room for it
    if (value)
    {
        *_out_length = d_cmake_value(_cmake, _argument, value);
    }

    return value;
}


/*
d_internal_lines
  Items terminated one after another, given as lines: each terminator but
the last a line's end.
*/
static bool
d_internal_lines(
    struct d_awk_value* _out,
    char*               _items,
    size_t              _used,
    size_t              _count
)
{
    // each terminator between two items
    for (size_t at = 0; (_count > 0u) && ((at + 1u) < _used); ++at)
    {
        // a line's end
        if (_items[at] == '\0')
        {
            _items[at] = '\n';
        }
    }

    return d_awk_value_set_string(_out, _items,
                                  (_count > 0u) ? (_used - 1u) : 0u);
}


//==============================================================================
// 3.  FILES
//==============================================================================


/*
d_internal_add_file
  Keeps a file, and gives its handle.
*/
static bool
d_internal_add_file(
    struct d_host_cmake* _host,
    struct d_awk_value*  _out,
    struct d_cmake_doc*  _doc
)
{
    // room for one more
    if (_host->file_count >= _host->file_capacity)
    {
        const size_t         wanted = (_host->file_capacity > 0u)
                                          ? (_host->file_capacity * 2u) : 8u;
        struct d_cmake_doc** grown  = realloc(_host->files,
                                              wanted * sizeof(*grown));

        // a file that cannot be kept is freed, and the handle not given
        if (!grown)
        {
            d_cmake_doc_free(_doc);

            return d_internal_fail(_host, _out, "out of memory");
        }

        _host->files         = grown;
        _host->file_capacity = wanted;
    }

    _host->files[_host->file_count++] = _doc;

    return d_internal_set_number(_out, (double)_host->file_count);
}


/*
cmake_open(path), cmake_new(path[, text])
  A file from disk, or of a text.
*/
static bool
d_internal_cmake_open(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const host = _user;
    char                       why[512];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    struct d_cmake_doc* const doc = d_cmake_doc_read(d_internal_text(_args[0]),
                                                     why, sizeof(why));

    // why it did not read
    if (!doc)
    {
        return d_internal_fail(host, _out, why);
    }

    return d_internal_add_file(host, _out, doc);
}


static bool
d_internal_cmake_new(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const host = _user;
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
    struct d_cmake_doc* const doc    = d_cmake_doc_make(path, text, length,
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
cmake_save(f[, path]), cmake_close(f), cmake_error()
*/
static bool
d_internal_cmake_save(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const host = _user;
    struct d_cmake_doc* const  doc  = d_internal_file(host, _args[0]);
    char                       why[512];

    (void)_problem;
    (void)_capacity;

    // a handle to no file
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const enum d_cmake_saved saved = d_cmake_doc_save(
                                         doc,
                                         (_count > 1u)
                                             ? d_internal_text(_args[1])
                                             : NULL,
                                         why, sizeof(why));

    // why it could not be written
    if (saved == D_CMAKE_SAVE_FAILED)
    {
        return d_internal_fail(host, _out, why);
    }

    return d_internal_set_number(_out,
                                 (saved == D_CMAKE_SAVE_WRITTEN) ? 1.0 : 0.0);
}


static bool
d_internal_cmake_close(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const host   = _user;
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

    d_cmake_doc_free(host->files[handle - 1u]);
    host->files[handle - 1u] = NULL;

    return d_internal_set_number(_out, 1.0);
}


static bool
d_internal_cmake_error(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const host = _user;

    (void)_args;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_set_text(_out, host->error);
}


// d_internal_about
//   enum: what cmake_path ... cmake_comments give of a file.
enum d_internal_about
{
    D_INTERNAL_ABOUT_PATH = 0,
    D_INTERNAL_ABOUT_TEXT,
    D_INTERNAL_ABOUT_CHANGES,
    D_INTERNAL_ABOUT_GENERATED,
    D_INTERNAL_ABOUT_COMMANDS,
    D_INTERNAL_ABOUT_COMMENTS
};


/*
d_internal_file_fact
  One fact of a file, by what is asked.
*/
static bool
d_internal_file_fact(
    void*                 _user,
    struct d_awk_value*   _handle,
    struct d_awk_value*   _out,
    enum d_internal_about _about
)
{
    struct d_host_cmake* const      host = _user;
    const struct d_cmake_doc* const doc  = d_internal_file(host, _handle);

    // a handle to no file
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    size_t                      length  = 0;
    const char* const           text    = d_cmake_doc_text(doc, &length);
    const struct d_cmake* const records = d_cmake_doc_records(doc);

    // a table of what is asked
    switch (_about)
    {
        case D_INTERNAL_ABOUT_PATH:
            return d_internal_set_text(_out, d_cmake_doc_path(doc));

        case D_INTERNAL_ABOUT_TEXT:
            return d_awk_value_set_string(_out, text, length);

        case D_INTERNAL_ABOUT_CHANGES:
            return d_internal_set_number(_out,
                                         (double)d_cmake_doc_changes(doc));

        case D_INTERNAL_ABOUT_GENERATED:
            return d_internal_set_number(_out,
                                         d_cmake_generated(text, length)
                                             ? 1.0 : 0.0);

        case D_INTERNAL_ABOUT_COMMANDS:
            return d_internal_set_number(_out,
                                         (double)records->command_count);

        case D_INTERNAL_ABOUT_COMMENTS:
        default:
            break;
    }

    return d_internal_set_number(_out, (double)records->comment_count);
}


/*
cmake_path(f), cmake_text(f), cmake_changes(f), cmake_generated(f),
cmake_commands(f), cmake_comments(f)
*/
static bool
d_internal_cmake_path(
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

    return d_internal_file_fact(_user, _args[0], _out, D_INTERNAL_ABOUT_PATH);
}


static bool
d_internal_cmake_text(
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

    return d_internal_file_fact(_user, _args[0], _out, D_INTERNAL_ABOUT_TEXT);
}


static bool
d_internal_cmake_changes(
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

    return d_internal_file_fact(_user, _args[0], _out,
                                D_INTERNAL_ABOUT_CHANGES);
}


static bool
d_internal_cmake_generated(
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

    return d_internal_file_fact(_user, _args[0], _out,
                                D_INTERNAL_ABOUT_GENERATED);
}


static bool
d_internal_cmake_commands(
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

    return d_internal_file_fact(_user, _args[0], _out,
                                D_INTERNAL_ABOUT_COMMANDS);
}


static bool
d_internal_cmake_comments(
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

    return d_internal_file_fact(_user, _args[0], _out,
                                D_INTERNAL_ABOUT_COMMENTS);
}


//==============================================================================
// 4.  COMMANDS
//==============================================================================


/*
d_internal_command_at
  The command a handle and an index name, and its file, or NULL,
cmake_error() saying why.
*/
static const struct d_cmake_command*
d_internal_command_at(
    struct d_host_cmake* _host,
    struct d_awk_value*  _handle,
    struct d_awk_value*  _index,
    struct d_cmake_doc** _out_doc
)
{
    struct d_cmake_doc* const doc = d_internal_file(_host, _handle);

    // a handle to no file
    if (!doc)
    {
        return NULL;
    }

    const struct d_cmake* const records = d_cmake_doc_records(doc);
    const size_t                n       = d_internal_index(_index);

    // a command the file has
    if ( (n == 0u) ||
         (n > records->command_count) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "the file has no command %s", d_internal_text(_index));

        return NULL;
    }

    *_out_doc = doc;

    return &records->commands[n - 1u];
}


/*
d_internal_indent_of
  The blanks a command's line begins with.
*/
static bool
d_internal_indent_of(
    const struct d_cmake*         _cmake,
    const struct d_cmake_command* _command,
    struct d_awk_value*           _out
)
{
    size_t line = _command->start;

    // back to the line's start
    while ( (line > 0u) &&
            (_cmake->text[line - 1u] != '\n') )
    {
        line--;
    }

    size_t end = line;

    // over its blanks
    while ( (_cmake->text[end] == ' ') ||
            (_cmake->text[end] == '\t') )
    {
        end++;
    }

    return d_awk_value_set_string(_out, &_cmake->text[line], end - line);
}


/*
d_internal_command_field
  One field of a command.
  Long as it is a table of the fields, each a line or two.
*/
static bool
d_internal_command_field(
    struct d_host_cmake*          _host,
    const struct d_cmake*         _cmake,
    const struct d_cmake_command* _command,
    const char*                   _field,
    struct d_awk_value*           _out
)
{
    static const char* const fields[] =
    {
        "name", "line", "column", "end_line", "depth", "block", "role",
        "partner", "unpaired", "args", "text", "indent"
    };
    static const char* const roles[] = { "", "open", "branch", "close" };
    const size_t             count   = sizeof(fields) / sizeof(fields[0]);

    // a table of the fields
    switch (d_internal_field(_field, fields, count))
    {
        case 0:
            return d_internal_set_input(_out, &_cmake->text[_command->start],
                                        _command->name_length);

        case 1:
            return d_internal_set_number(_out, (double)_command->line);

        case 2:
            return d_internal_set_number(_out, (double)_command->column);

        case 3:
            return d_internal_set_number(_out, (double)_command->end_line);

        case 4:
            return d_internal_set_number(_out, (double)_command->depth);

        case 5:
            return d_internal_set_text(_out,
                                       d_cmake_block_name(_command->block));

        case 6:
            return d_internal_set_text(_out, roles[_command->role]);

        case 7:
            return d_internal_set_number(_out,
                                         (_command->partner == D_CMAKE_NONE)
                                             ? 0.0
                                             : (double)(_command->partner +
                                                        1u));

        case 8:
            return d_internal_set_number(_out,
                                         (_command->unpaired) ? 1.0 : 0.0);

        case 9:
            return d_internal_set_number(_out,
                                         (double)_command->argument_count);

        case 10:
            return d_internal_set_input(_out, &_cmake->text[_command->start],
                                        _command->end - _command->start);

        case 11:
            return d_internal_indent_of(_cmake, _command, _out);

        default:
            break;
    }

    return d_internal_unknown_field(_host, _out, _field);
}


/*
cmake_command(f, n, field), cmake_name(f, n), cmake_args(f, n)
*/
static bool
d_internal_command_common(
    void*                _user,
    struct d_awk_value** _args,
    struct d_awk_value*  _out,
    const char*          _field
)
{
    struct d_host_cmake* const          host    = _user;
    struct d_cmake_doc*                 doc     = NULL;
    const struct d_cmake_command* const command =
        d_internal_command_at(host, _args[0], _args[1], &doc);

    // a command the file has
    if (!command)
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_command_field(host, d_cmake_doc_records(doc), command,
                                    _field, _out);
}


static bool
d_internal_cmake_command(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    char* const field = d_internal_copy_arg(_args[2]);

    (void)_count;

    // the field's name, copied, as the others are read over it
    if (!field)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    const bool ok = d_internal_command_common(_user, _args, _out, field);

    free(field);

    return ok;
}


static bool
d_internal_cmake_name(
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

    return d_internal_command_common(_user, _args, _out, "name");
}


static bool
d_internal_cmake_args(
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

    return d_internal_command_common(_user, _args, _out, "args");
}


/*
cmake_find(f, name[, after])
  The first command after `after` -- 0, the start, by default -- whose name
is `name`, as CMake compares names.
*/
static bool
d_internal_cmake_find(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const      host = _user;
    const struct d_cmake_doc* const doc  = d_internal_file(host, _args[0]);
    char* const                     name = d_internal_copy_arg(_args[1]);

    (void)_problem;
    (void)_capacity;

    // a handle to a file, and a name copied
    if ( (!doc) ||
         (!name) )
    {
        free(name);

        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const struct d_cmake* const records = d_cmake_doc_records(doc);
    const size_t                after   = (_count > 2u)
                                              ? d_internal_index(_args[2])
                                              : 0u;
    size_t                      found   = 0;

    // the first so named after the place
    for (size_t at = after;
         (found == 0u) && (at < records->command_count);
         ++at)
    {
        // a name as CMake compares names
        if (d_cmake_is(records, &records->commands[at], name))
        {
            found = at + 1u;
        }
    }

    free(name);

    return d_internal_set_number(_out, (double)found);
}


//==============================================================================
// 5.  ARGUMENTS
//==============================================================================


/*
d_internal_argument_at
  The argument a handle and two indices name, and its file, or NULL,
cmake_error() saying why.
*/
static const struct d_cmake_argument*
d_internal_argument_at(
    struct d_host_cmake* _host,
    struct d_awk_value** _args,
    struct d_cmake_doc** _out_doc
)
{
    const struct d_cmake_command* const command =
        d_internal_command_at(_host, _args[0], _args[1], _out_doc);

    // a command the file has
    if (!command)
    {
        return NULL;
    }

    const size_t k = d_internal_index(_args[2]);

    // an argument it has
    if ( (k == 0u) ||
         (k > command->argument_count) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "command %s has no argument %s",
                       d_internal_text(_args[1]), d_internal_text(_args[2]));

        return NULL;
    }

    const struct d_cmake* const records = d_cmake_doc_records(*_out_doc);

    return &records->arguments[command->first_argument + k - 1u];
}


/*
d_internal_items
  The arguments CMake passes for an argument, a line each: a quoted or a
bracket one's value, whole; an unquoted one's text divided as a list, its
empty items dropped.
*/
static bool
d_internal_items(
    struct d_host_cmake*           _host,
    const struct d_cmake*          _cmake,
    const struct d_cmake_argument* _argument,
    struct d_awk_value*            _out
)
{
    const size_t span   = _argument->end - _argument->start;
    char* const  passed = malloc(span + 1u);
    char* const  items  = malloc(span + 2u);

    // room for the text and its items
    if ( (!passed) ||
         (!items) )
    {
        free(passed);
        free(items);

        return d_internal_fail(_host, _out, "out of memory");
    }

    const size_t length = d_cmake_passed(_cmake, _argument, passed);
    size_t       used   = 0;
    size_t       count  = 0;

    // an unquoted argument divided; another passed whole
    if (_argument->kind == D_CMAKE_UNQUOTED)
    {
        count = d_cmake_list_items(passed, length, false, items, &used);
    }
    else
    {
        memcpy(items, passed, length);
        items[length] = '\0';
        used          = length + 1u;
        count         = 1u;
    }

    const bool set = d_internal_lines(_out, items, used, count);

    free(passed);
    free(items);

    return set;
}


/*
d_internal_argument_value
  An argument's value, as text of the file.
*/
static bool
d_internal_argument_value(
    struct d_host_cmake*           _host,
    const struct d_cmake*          _cmake,
    const struct d_cmake_argument* _argument,
    struct d_awk_value*            _out
)
{
    size_t      length = 0;
    char* const value  = d_internal_value_of(_cmake, _argument, &length);

    // room for the value
    if (!value)
    {
        return d_internal_fail(_host, _out, "out of memory");
    }

    const bool set = d_internal_set_input(_out, value, length);

    free(value);

    return set;
}


/*
d_internal_argument_field
  One field of an argument.
  Long as it is a table of the fields, each a line or two.
*/
static bool
d_internal_argument_field(
    struct d_host_cmake*           _host,
    const struct d_cmake*          _cmake,
    const struct d_cmake_argument* _argument,
    const char*                    _field,
    struct d_awk_value*            _out
)
{
    static const char* const fields[] =
    {
        "text", "value", "items", "kind", "line", "column", "end_line",
        "legacy", "paren"
    };
    static const char* const kinds[] = { "unquoted", "quoted", "bracket" };
    const size_t             count   = sizeof(fields) / sizeof(fields[0]);
    const char               paren[] = { _argument->paren, '\0' };

    // a table of the fields
    switch (d_internal_field(_field, fields, count))
    {
        case 0:
            return d_internal_set_input(_out,
                                        &_cmake->text[_argument->start],
                                        _argument->end - _argument->start);

        case 1:
            return d_internal_argument_value(_host, _cmake, _argument, _out);

        case 2:
            return d_internal_items(_host, _cmake, _argument, _out);

        case 3:
            return d_internal_set_text(_out, kinds[_argument->kind]);

        case 4:
            return d_internal_set_number(_out, (double)_argument->line);

        case 5:
            return d_internal_set_number(_out, (double)_argument->column);

        case 6:
            return d_internal_set_number(_out, (double)_argument->end_line);

        case 7:
            return d_internal_set_number(_out,
                                         (_argument->legacy) ? 1.0 : 0.0);

        case 8:
            return d_internal_set_text(_out, paren);

        default:
            break;
    }

    return d_internal_unknown_field(_host, _out, _field);
}


/*
cmake_arg(f, n, k[, field]), cmake_value(f, n, k)
*/
static bool
d_internal_cmake_arg(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const host  = _user;
    char* const                field = (_count > 3u)
                                           ? d_internal_copy_arg(_args[3])
                                           : NULL;
    struct d_cmake_doc*        doc   = NULL;

    // the field's name, copied, as the others are read over it
    if ( (_count > 3u) &&
         (!field) )
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    const struct d_cmake_argument* const argument =
        d_internal_argument_at(host, _args, &doc);

    // an argument the command has
    if (!argument)
    {
        free(field);

        return d_internal_fail(host, _out, host->error);
    }

    const bool ok = d_internal_argument_field(host, d_cmake_doc_records(doc),
                                              argument,
                                              (field) ? field : "text", _out);

    free(field);

    return ok;
}


static bool
d_internal_cmake_value(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const host = _user;
    struct d_cmake_doc*        doc  = NULL;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    const struct d_cmake_argument* const argument =
        d_internal_argument_at(host, _args, &doc);

    // an argument the command has
    if (!argument)
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_argument_value(host, d_cmake_doc_records(doc), argument,
                                     _out);
}


/*
cmake_find_arg(f, n, value[, after])
  The first argument of command n after `after` whose value is `value`.
*/
static bool
d_internal_cmake_find_arg(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const host   = _user;
    char* const                wanted = d_internal_copy_arg(_args[2]);
    struct d_cmake_doc*        doc    = NULL;

    (void)_problem;
    (void)_capacity;

    const struct d_cmake_command* const command =
        d_internal_command_at(host, _args[0], _args[1], &doc);

    // a command the file has, and the value copied
    if ( (!command) ||
         (!wanted) )
    {
        free(wanted);

        return d_internal_fail(host, _out, (command) ? "out of memory"
                                                     : host->error);
    }

    const struct d_cmake* const records = d_cmake_doc_records(doc);
    const size_t                after   = (_count > 3u)
                                              ? d_internal_index(_args[3])
                                              : 0u;
    size_t                      found   = 0;
    bool                        failed  = false;

    // each argument after the place, the first of that value
    for (size_t k = after;
         (found == 0u) && (!failed) && (k < command->argument_count);
         ++k)
    {
        const struct d_cmake_argument* const argument =
            &records->arguments[command->first_argument + k];
        size_t                               length   = 0;
        char* const                          value    =
            d_internal_value_of(records, argument, &length);

        failed = (!value);
        found  = ( (value) && (strcmp(value, wanted) == 0) ) ? (k + 1u) : 0u;
        free(value);
    }

    free(wanted);

    // memory that ran out
    if (failed)
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    return d_internal_set_number(_out, (double)found);
}


//==============================================================================
// 6.  COMMENTS
//==============================================================================


/*
d_internal_body
  What a comment says: a line comment's text past its `#`; a bracket
comment's within its brackets, a newline directly after the opening one
dropped, as a bracket argument drops it.
*/
static bool
d_internal_body(
    const struct d_cmake*         _cmake,
    const struct d_cmake_comment* _comment,
    struct d_awk_value*           _out
)
{
    const char* const text   = &_cmake->text[_comment->start];
    const size_t      length = _comment->end - _comment->start;

    // a line comment's text past its `#`
    if (!_comment->bracket)
    {
        return d_awk_value_set_string(_out, &text[1], length - 1u);
    }

    size_t equals = 0;

    // the `=` of its opening: `#[`, the `=`, `[`
    while ( ((equals + 2u) < length) &&
            (text[2u + equals] == '=') )
    {
        equals++;
    }

    size_t       from = equals + 3u;
    const size_t to   = (length >= (from + equals + 2u))
                            ? (length - equals - 2u) : from;

    // a table of the line endings directly after the opening, which are no
    // part of it
    if ( (from < to) &&
         (text[from] == '\n') )
    {
        from++;
    }
    else if ( ((from + 1u) < to) &&
              (text[from] == '\r') &&
              (text[from + 1u] == '\n') )
    {
        from += 2u;
    }

    return d_awk_value_set_string(_out, &text[from], to - from);
}


/*
d_internal_comment_at
  The comment a handle and an index name, or NULL, cmake_error() saying
why.
*/
static const struct d_cmake_comment*
d_internal_comment_at(
    struct d_host_cmake*   _host,
    struct d_awk_value*    _handle,
    struct d_awk_value*    _index,
    const struct d_cmake** _out_records
)
{
    const struct d_cmake_doc* const doc = d_internal_file(_host, _handle);

    // a handle to no file
    if (!doc)
    {
        return NULL;
    }

    const struct d_cmake* const records = d_cmake_doc_records(doc);
    const size_t                index   = d_internal_index(_index);

    // a comment the file has
    if ( (index == 0u) ||
         (index > records->comment_count) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "the file has no comment %s", d_internal_text(_index));

        return NULL;
    }

    *_out_records = records;

    return &records->comments[index - 1u];
}


/*
cmake_comment(f, i, field)
  Long as it is a table of the fields, each a line or two.
*/
static bool
d_internal_cmake_comment(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] =
    {
        "text", "body", "line", "column", "end_line", "bracket", "trailing",
        "command"
    };
    struct d_host_cmake* const host    = _user;
    const struct d_cmake*      records = NULL;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    const struct d_cmake_comment* const comment =
        d_internal_comment_at(host, _args[0], _args[1], &records);

    // a comment the file has
    if (!comment)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const char* const field = d_internal_text(_args[2]);
    const size_t      count = sizeof(fields) / sizeof(fields[0]);

    // a table of the fields
    switch (d_internal_field(field, fields, count))
    {
        case 0:
            return d_awk_value_set_string(_out,
                                          &records->text[comment->start],
                                          comment->end - comment->start);

        case 1:
            return d_internal_body(records, comment, _out);

        case 2:
            return d_internal_set_number(_out, (double)comment->line);

        case 3:
            return d_internal_set_number(_out, (double)comment->column);

        case 4:
            return d_internal_set_number(_out, (double)comment->end_line);

        case 5:
            return d_internal_set_number(_out,
                                         (comment->bracket) ? 1.0 : 0.0);

        case 6:
            return d_internal_set_number(_out,
                                         (comment->trailing) ? 1.0 : 0.0);

        case 7:
            return d_internal_set_number(_out,
                                         (comment->within == D_CMAKE_NONE)
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

    return (index > 0u) ? (index - 1u) : D_CMAKE_NONE;
}


// d_internal_edit
//   enum: the changes cmake_set_arg ... cmake_remove_command make.
enum d_internal_edit
{
    D_INTERNAL_SET_ARG = 0,
    D_INTERNAL_INSERT_ARG,
    D_INTERNAL_INSERT_AFTER,
    D_INTERNAL_REMOVE_ARG,
    D_INTERNAL_SET_NAME,
    D_INTERNAL_SET_COMMAND,
    D_INTERNAL_INSERT_COMMAND,
    D_INTERNAL_REMOVE_COMMAND
};


/*
d_internal_edit_text
  Which argument holds a change's text: the last of those a change takes,
or 0 for a change with none.
*/
static size_t
d_internal_edit_text(
    enum d_internal_edit _edit
)
{
    // a table of the changes and where their texts are
    switch (_edit)
    {
        case D_INTERNAL_SET_ARG:
        case D_INTERNAL_INSERT_ARG:
        case D_INTERNAL_INSERT_AFTER:
            return 3u;

        case D_INTERNAL_SET_NAME:
        case D_INTERNAL_SET_COMMAND:
        case D_INTERNAL_INSERT_COMMAND:
            return 2u;

        case D_INTERNAL_REMOVE_ARG:
        case D_INTERNAL_REMOVE_COMMAND:
        default:
            break;
    }

    return 0u;
}


/*
d_internal_edit_doc
  Makes a change to a document, its indices from 0.
  Long as it is a table of the changes, each a call.
*/
static bool
d_internal_edit_doc(
    struct d_cmake_doc*  _doc,
    enum d_internal_edit _edit,
    struct d_awk_value** _args,
    size_t               _count,
    const char*          _text
)
{
    const size_t n = d_internal_less_one(_args[1]);

    // a table of the changes
    switch (_edit)
    {
        case D_INTERNAL_SET_ARG:
            return d_cmake_doc_set_argument(_doc, n,
                                            d_internal_less_one(_args[2]),
                                            _text);

        case D_INTERNAL_INSERT_ARG:
            return d_cmake_doc_insert_argument(_doc, n,
                                               d_internal_less_one(_args[2]),
                                               _text, false);

        case D_INTERNAL_INSERT_AFTER:
            return d_cmake_doc_insert_argument(_doc, n,
                                               d_internal_less_one(_args[2]),
                                               _text, true);

        case D_INTERNAL_REMOVE_ARG:
            return d_cmake_doc_remove_argument(_doc, n,
                                               d_internal_less_one(_args[2]));

        case D_INTERNAL_SET_NAME:
            return d_cmake_doc_set_name(_doc, n, _text);

        case D_INTERNAL_SET_COMMAND:
            return d_cmake_doc_set_command(_doc, n, _text);

        case D_INTERNAL_INSERT_COMMAND:
            return d_cmake_doc_insert_commands(_doc, n, _text);

        case D_INTERNAL_REMOVE_COMMAND:
        default:
            break;
    }

    return d_cmake_doc_remove_commands(_doc, n,
                                       (_count > 2u)
                                           ? d_internal_less_one(_args[2])
                                           : n);
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
    struct d_host_cmake* const host  = _user;
    const size_t               which = d_internal_edit_text(_edit);
    struct d_cmake_doc* const  doc   = d_internal_file(host, _args[0]);

    // a handle to no file
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    char* const text = (which > 0u) ? d_internal_copy_arg(_args[which])
                                    : NULL;

    // a text that could not be copied
    if ( (which > 0u) &&
         (!text) )
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    const bool ok = d_internal_edit_doc(doc, _edit, _args, _count,
                                        (text) ? text : "");

    free(text);

    return d_internal_changed(host, _out, doc, ok);
}


/*
cmake_set_arg(f, n, k, text), cmake_insert_arg(f, n, k, text),
cmake_insert_after(f, n, k, text), cmake_remove_arg(f, n, k),
cmake_set_name(f, n, name), cmake_set_command(f, n, text),
cmake_insert_command(f, n, text), cmake_remove_command(f, n[, last])
*/
static bool
d_internal_cmake_set_arg(
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

    return d_internal_edit(_user, _args, _count, _out, D_INTERNAL_SET_ARG);
}


static bool
d_internal_cmake_insert_arg(
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

    return d_internal_edit(_user, _args, _count, _out,
                           D_INTERNAL_INSERT_ARG);
}


static bool
d_internal_cmake_insert_after(
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

    return d_internal_edit(_user, _args, _count, _out,
                           D_INTERNAL_INSERT_AFTER);
}


static bool
d_internal_cmake_remove_arg(
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

    return d_internal_edit(_user, _args, _count, _out,
                           D_INTERNAL_REMOVE_ARG);
}


static bool
d_internal_cmake_set_name(
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

    return d_internal_edit(_user, _args, _count, _out, D_INTERNAL_SET_NAME);
}


static bool
d_internal_cmake_set_command(
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

    return d_internal_edit(_user, _args, _count, _out,
                           D_INTERNAL_SET_COMMAND);
}


static bool
d_internal_cmake_insert_command(
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

    return d_internal_edit(_user, _args, _count, _out,
                           D_INTERNAL_INSERT_COMMAND);
}


static bool
d_internal_cmake_remove_command(
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

    return d_internal_edit(_user, _args, _count, _out,
                           D_INTERNAL_REMOVE_COMMAND);
}


//==============================================================================
// 8.  WRITING
//==============================================================================


/*
d_internal_written
  Gives a text a writer made, and frees it; -1, cmake_error() saying why,
for one it could not make.
*/
static bool
d_internal_written(
    struct d_host_cmake* _host,
    struct d_awk_value*  _out,
    char*                _text,
    size_t               _length,
    const char*          _why
)
{
    // a value that could not be written so
    if (!_text)
    {
        return d_internal_fail(_host, _out, _why);
    }

    const bool set = d_awk_value_set_string(_out, _text, _length);

    free(_text);

    return set;
}


/*
d_internal_write_one
  A value written as an argument of a kind, or as plainly as it can be at
D_CMAKE_UNQUOTED with `_any`.
*/
static bool
d_internal_write_one(
    struct d_host_cmake*       _host,
    struct d_awk_value*        _value,
    struct d_awk_value*        _out,
    enum d_cmake_argument_kind _kind,
    bool                       _any
)
{
    size_t            length  = 0;
    const char* const value   = d_awk_value_text(_value, NULL, &length);
    size_t            written = 0;
    char* const       text    = (_any)
                                    ? d_cmake_write_any(value, length,
                                                        &written)
                                    : d_cmake_write(value, length, _kind,
                                                    &written);

    return d_internal_written(_host, _out, text, written,
                              (_kind == D_CMAKE_BRACKET)
                                  ? "a bracket argument cannot hold a CR LF"
                                  : "out of memory");
}


/*
cmake_quote(value), cmake_bracket(value), cmake_argument(value)
*/
static bool
d_internal_cmake_quote(
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

    return d_internal_write_one(_user, _args[0], _out, D_CMAKE_QUOTED, false);
}


static bool
d_internal_cmake_bracket(
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

    return d_internal_write_one(_user, _args[0], _out, D_CMAKE_BRACKET,
                                false);
}


static bool
d_internal_cmake_argument(
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

    return d_internal_write_one(_user, _args[0], _out, D_CMAKE_UNQUOTED,
                                true);
}


/*
d_internal_join_call
  The name, `(`, the values a space apart, and `)`; each value is freed.
*/
static char*
d_internal_join_call(
    const char* _name,
    size_t      _name_length,
    char**      _values,
    size_t*     _lengths,
    size_t      _count,
    size_t*     _out_length
)
{
    size_t total = _name_length + 2u;

    // the room every value takes, and a space between
    for (size_t at = 0; at < _count; ++at)
    {
        total += _lengths[at] + ( (at > 0u) ? 1u : 0u );
    }

    char* const text = malloc(total + 1u);
    size_t      used = 0;

    // the name and the `(`
    if (text)
    {
        memcpy(text, _name, _name_length);
        used         = _name_length;
        text[used++] = '(';
    }

    // each value, a space apart, freed once copied
    for (size_t at = 0; at < _count; ++at)
    {
        // the text, when there is room for it
        if (text)
        {
            text[used] = ' ';
            used      += (at > 0u) ? 1u : 0u;
            memcpy(&text[used], _values[at], _lengths[at]);
            used      += _lengths[at];
        }

        free(_values[at]);
    }

    // the `)`
    if (text)
    {
        text[used++] = ')';
        text[used]   = '\0';
    }

    *_out_length = used;

    return text;
}


/*
cmake_call(name[, value ...])
  A command of values, each written as plainly as it can be, a space
apart.
*/
static bool
d_internal_cmake_call(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const host = _user;
    char*                      values[D_INTERNAL_CALL_VALUES];
    size_t                     lengths[D_INTERNAL_CALL_VALUES];
    size_t                     made = 0;
    const char*                why  = NULL;

    (void)_problem;
    (void)_capacity;

    // each value after the name, written
    for (size_t at = 1u;
         (!why) && (at < _count) && (made < D_INTERNAL_CALL_VALUES);
         ++at)
    {
        size_t            length = 0;
        const char* const value  = d_awk_value_text(_args[at], NULL, &length);

        values[made] = d_cmake_write_any(value, length, &lengths[made]);
        why          = (values[made]) ? NULL : "out of memory";
        made        += (values[made]) ? 1u : 0u;
    }

    size_t            span = 0;
    const char* const name = d_awk_value_text(_args[0], NULL, &span);

    // a name a command may have
    if ( (!why) &&
         (!d_cmake_is_name(name, span)) )
    {
        (void)snprintf(host->error, sizeof(host->error),
                       "not a command's name: %s", name);
        why = host->error;
    }

    size_t      length = 0;
    char* const text   = d_internal_join_call(name, span, values, lengths,
                                              made, &length);

    // a value or a name refused gives no command
    if (why)
    {
        free(text);

        return d_internal_fail(host, _out, why);
    }

    return d_internal_written(host, _out, text, length, "out of memory");
}


/*
cmake_unquote(text)
  The value of a text read as the one argument of a command.
*/
static bool
d_internal_cmake_unquote(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_cmake* const host    = _user;
    size_t                     length  = 0;
    const char* const          text    = d_awk_value_text(_args[0], NULL,
                                                          &length);
    char* const                wrapped = malloc(length + 4u);
    struct d_cmake             cmake;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // the text, wrapped as `x(...)`
    if (!wrapped)
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    wrapped[0] = 'x';
    wrapped[1] = '(';
    memcpy(&wrapped[2], text, length);
    wrapped[length + 2u] = ')';
    wrapped[length + 3u] = '\0';

    const bool  read  = d_cmake_read(&cmake, wrapped, length + 3u, NULL, 0u);
    const bool  one   = (read)                                &&
                        (length > 0u)                         &&
                        (cmake.argument_count == 1u)          &&
                        (cmake.comment_count == 0u)           &&
                        (cmake.arguments[0].start == 2u)      &&
                        (cmake.arguments[0].end == (length + 2u));
    size_t      span  = 0;
    char* const value = (one) ? d_internal_value_of(&cmake,
                                                    &cmake.arguments[0],
                                                    &span)
                              : NULL;

    d_cmake_release(&cmake);
    free(wrapped);

    // a text that is not one argument
    if (!one)
    {
        (void)snprintf(host->error, sizeof(host->error),
                       "not one argument: %s", d_internal_text(_args[0]));

        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_written(host, _out, value, span, "out of memory");
}


/*
cmake_split(list[, keep])
  The items, a line each.
*/
static bool
d_internal_cmake_split(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    size_t            length = 0;
    const char* const list   = d_awk_value_text(_args[0], NULL, &length);
    char* const       items  = malloc(length + 1u);

    (void)_problem;
    (void)_capacity;

    // room for the items
    if (!items)
    {
        return d_internal_fail(_user, _out, "out of memory");
    }

    const bool   keep  = ( (_count > 1u) &&
                           (d_awk_value_number(_args[1]) != 0.0) );
    size_t       used  = 0;
    const size_t count = d_cmake_list_items(list, length, keep, items, &used);
    const bool   set   = d_internal_lines(_out, items, used, count);

    free(items);

    return set;
}


//==============================================================================
// 9.  THE EXTENSION
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
        { "cmake_open",           1u, 1u,  d_internal_cmake_open           },
        { "cmake_new",            1u, 2u,  d_internal_cmake_new            },
        { "cmake_save",           1u, 2u,  d_internal_cmake_save           },
        { "cmake_close",          1u, 1u,  d_internal_cmake_close          },
        { "cmake_error",          0u, 0u,  d_internal_cmake_error          },
        { "cmake_path",           1u, 1u,  d_internal_cmake_path           },
        { "cmake_text",           1u, 1u,  d_internal_cmake_text           },
        { "cmake_changes",        1u, 1u,  d_internal_cmake_changes        },
        { "cmake_generated",      1u, 1u,  d_internal_cmake_generated      },
        { "cmake_commands",       1u, 1u,  d_internal_cmake_commands       },
        { "cmake_command",        3u, 3u,  d_internal_cmake_command        },
        { "cmake_name",           2u, 2u,  d_internal_cmake_name           },
        { "cmake_args",           2u, 2u,  d_internal_cmake_args           },
        { "cmake_find",           2u, 3u,  d_internal_cmake_find           },
        { "cmake_arg",            3u, 4u,  d_internal_cmake_arg            },
        { "cmake_value",          3u, 3u,  d_internal_cmake_value          },
        { "cmake_find_arg",       3u, 4u,  d_internal_cmake_find_arg       },
        { "cmake_comments",       1u, 1u,  d_internal_cmake_comments       },
        { "cmake_comment",        3u, 3u,  d_internal_cmake_comment        },
        { "cmake_set_arg",        4u, 4u,  d_internal_cmake_set_arg        },
        { "cmake_insert_arg",     4u, 4u,  d_internal_cmake_insert_arg     },
        { "cmake_insert_after",   4u, 4u,  d_internal_cmake_insert_after   },
        { "cmake_remove_arg",     3u, 3u,  d_internal_cmake_remove_arg     },
        { "cmake_set_name",       3u, 3u,  d_internal_cmake_set_name       },
        { "cmake_set_command",    3u, 3u,  d_internal_cmake_set_command    },
        { "cmake_insert_command", 3u, 3u,  d_internal_cmake_insert_command },
        { "cmake_remove_command", 2u, 3u,  d_internal_cmake_remove_command },
        { "cmake_quote",          1u, 1u,  d_internal_cmake_quote          },
        { "cmake_bracket",        1u, 1u,  d_internal_cmake_bracket        },
        { "cmake_argument",       1u, 1u,  d_internal_cmake_argument       },
        { "cmake_call",           1u, 16u, d_internal_cmake_call           },
        { "cmake_unquote",        1u, 1u,  d_internal_cmake_unquote        },
        { "cmake_split",          1u, 2u,  d_internal_cmake_split          }
    };

    *_out_count = sizeof(functions) / sizeof(functions[0]);

    return functions;
}


struct d_host_cmake*
d_host_cmake_new(void)
{
    return calloc(1u, sizeof(struct d_host_cmake));
}


void
d_host_cmake_free(
    struct d_host_cmake* _host
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
        d_cmake_doc_free(_host->files[index]);
    }

    free(_host->files);
    free(_host);

    return;
}


bool
d_host_cmake_install(
    struct d_host_cmake* _host,
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
