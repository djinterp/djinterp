/*******************************************************************************
* djinterp [tools]                                                   dhost_git.c
*
* Definitions for the declarations in dhost_git.h.
*   Each file open is a dgit document, and each repository a dgit repository;
* a handle is a place in one list of both, from 1, and a handle closed keeps
* its place, empty, so it names nothing rather than another. What a file
* holds is read from the document's records as they are now, each field by
* its name; what a repository holds is read as it is asked for.
*   A list is made a line to each item. A path in one is written as it is,
* unless it holds a byte that would break the line or its reading -- a
* control byte, a quote, a backslash -- when it is quoted as git quotes
* one.
*
* path:      /src/djinterp/tools/dawk/ext/git/dhost_git.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dhost_git.h"  // corresponding header
// std
#include <stdio.h>   // fopen, fclose, snprintf
#include <stdlib.h>  // malloc, realloc, free, getenv
#include <string.h>  // memcpy, memset, strchr, strcmp, strlen, strncmp, strstr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_doc.h"   // d_git_doc
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_repo.h"  // d_git_repo
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_text.h"  // d_git_text
#include "../../../../../../inc/djinterp/tools/dawk/dhost.h"              // d_awk_host_function
#include "../../../../../../inc/djinterp/tools/dawk/dvalue.h"             // d_awk_value_text


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Extension
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_slot
//   struct: what a handle names: a file, a repository, or -- closed --
//   neither.
struct d_internal_slot
{
    struct d_git_doc*  doc;
    struct d_git_repo* repo;
};

// 1.1.2
// d_host_git
//   struct: the files and repositories, open or closed, and the last
//   failure.
struct d_host_git
{
    struct d_internal_slot* slots;
    size_t                  slot_count;
    size_t                  slot_capacity;
    char                    error[512];
};

// 1.1.3
// d_internal_fn_count
//   type: how many records of a kind a file's records hold.
typedef size_t (*d_internal_fn_count)(const struct d_git_text* _text);

// 1.1.4
// d_internal_values
//   struct: the values a key has, lowest first, each borrowed -- NULL for
//   a name alone.
struct d_internal_values
{
    const char** items;
    size_t       count;
    size_t       capacity;
    bool         failed;
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
d_internal_copy
  An argument's text, copied and terminated, as another argument read may
be converted over it; its length, when asked.
*/
static char*
d_internal_copy(
    struct d_awk_value* _value,
    size_t*             _out_length
)
{
    size_t            length = 0u;
    const char* const text   = d_awk_value_text(_value, NULL, &length);
    char* const       copy   = malloc(length + 1u);

    // room for it
    if (copy)
    {
        memcpy(copy, (text) ? text : "", length);
        copy[length] = '\0';
    }

    // its length, if asked
    if (_out_length)
    {
        *_out_length = length;
    }

    return copy;
}


/*
d_internal_fail
  Gives -1 for a failure a program can recover from, git_error() saying
why.
*/
static bool
d_internal_fail(
    struct d_host_git*  _host,
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
d_internal_set_text, d_internal_set_bytes, d_internal_set_number,
d_internal_set_flag, d_internal_set_index
  Gives text, bytes of a length, a number, 1 or 0, or an index from 0 as
one from 1 -- 0 for none.
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
d_internal_set_bytes(
    struct d_awk_value* _out,
    const void*         _bytes,
    size_t              _length
)
{
    return d_awk_value_set_string(_out, (_bytes) ? _bytes : "",
                                  (_bytes) ? _length : 0u);
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


static bool
d_internal_set_flag(
    struct d_awk_value* _out,
    bool                _flag
)
{
    return d_internal_set_number(_out, (_flag) ? 1.0 : 0.0);
}


static bool
d_internal_set_index(
    struct d_awk_value* _out,
    size_t              _index
)
{
    return d_internal_set_number(_out, (_index == D_GIT_TEXT_NONE)
                                       ? 0.0
                                       : (double)(_index + 1u));
}


/*
d_internal_set_span
  Gives a span of a file's text, "" for one not there.
*/
static bool
d_internal_set_span(
    struct d_awk_value*     _out,
    const struct d_git_doc* _doc,
    size_t                  _start,
    size_t                  _end
)
{
    size_t            length = 0u;
    const char* const text   = d_git_doc_text(_doc, &length);

    // a span not there is nothing
    if ( (_start == D_GIT_TEXT_NONE) ||
         (_end < _start) ||
         (_end > length) )
    {
        return d_awk_value_set_string(_out, "", 0u);
    }

    return d_awk_value_set_string(_out, &text[_start], _end - _start);
}


/*
d_internal_set_buffer
  Gives a buffer's text, which is freed; -1 for one memory ran out of.
*/
static bool
d_internal_set_buffer(
    struct d_host_git*   _host,
    struct d_awk_value*  _out,
    struct d_git_buffer* _buffer
)
{
    // a buffer memory ran out of
    if (_buffer->failed)
    {
        d_git_buffer_free(_buffer);

        return d_internal_fail(_host, _out, "out of memory");
    }

    const bool set = d_awk_value_set_string(_out,
                                            (_buffer->text) ? _buffer->text
                                                            : "",
                                            _buffer->length);

    d_git_buffer_free(_buffer);

    return set;
}


/*
d_internal_set_hex
  Gives an object's name in hex.
*/
static bool
d_internal_set_hex(
    struct d_awk_value*      _out,
    const struct d_git_repo* _repo,
    const struct d_git_oid*  _oid
)
{
    char hex[(2u * D_GIT_OID_MOST) + 1u];

    d_git_repo_hex(_repo, _oid, hex);

    return d_internal_set_text(_out, hex);
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
  Gives "" for a field not known, git_error() saying so.
*/
static bool
d_internal_unknown_field(
    struct d_host_git*  _host,
    struct d_awk_value* _out,
    const char*         _field
)
{
    (void)snprintf(_host->error, sizeof(_host->error), "no field %s",
                   _field);

    return d_awk_value_set_string(_out, "", 0u);
}


/*
d_internal_add_path
  A path added to a list, as it is, or in git's quotes when it holds a
control byte, a quote or a backslash.
*/
static void
d_internal_add_path(
    struct d_git_buffer* _list,
    const char*          _path,
    size_t               _length
)
{
    bool plain = true;

    // each byte, for one that needs quotes
    for (size_t at = 0u; ( (plain) && (at < _length) ); ++at)
    {
        const unsigned char byte = (unsigned char)_path[at];

        plain = ( (byte >= 0x20u) &&
                  (byte != 0x7fu) &&
                  (byte != '"') &&
                  (byte != '\\') );
    }

    // as it is, or quoted
    if (plain)
    {
        (void)d_git_buffer_add(_list, _path, _length);
    }
    else
    {
        (void)d_git_text_quote_path(_list, _path, _length);
    }

    return;
}


/*
d_internal_dup
  A string, allocated.
*/
static char*
d_internal_dup(
    const char* _text
)
{
    const size_t length = strlen(_text);
    char* const  copy   = malloc(length + 1u);

    // room for it
    if (copy)
    {
        memcpy(copy, _text, length + 1u);
    }

    return copy;
}


/*
d_internal_join
  Two paths with a `/` between, allocated.
*/
static char*
d_internal_join(
    const char* _left,
    const char* _right
)
{
    struct d_git_buffer joined;

    memset(&joined, 0, sizeof(joined));
    (void)d_git_buffer_add_text(&joined, _left);
    (void)d_git_buffer_add_char(&joined, '/');
    (void)d_git_buffer_add_text(&joined, _right);

    return d_git_buffer_take(&joined);
}


//==============================================================================
// 3.  HANDLES
//==============================================================================


/*
d_internal_slot_of
  What a handle names, or NULL, git_error() saying why.
*/
static struct d_internal_slot*
d_internal_slot_of(
    struct d_host_git*  _host,
    struct d_awk_value* _handle
)
{
    const size_t handle = d_internal_index(_handle);

    // a handle in the list, to something not closed
    if ( (handle == 0u) ||
         (handle > _host->slot_count) ||
         ( (!_host->slots[handle - 1u].doc) &&
           (!_host->slots[handle - 1u].repo) ) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "nothing has the handle %s", d_internal_text(_handle));

        return NULL;
    }

    return &_host->slots[handle - 1u];
}


/*
d_internal_doc, d_internal_repo
  The file, or the repository, a handle names, or NULL, git_error() saying
why.
*/
static struct d_git_doc*
d_internal_doc(
    struct d_host_git*  _host,
    struct d_awk_value* _handle
)
{
    const struct d_internal_slot* const slot = d_internal_slot_of(_host,
                                                                  _handle);

    // a repository's handle
    if ( (slot) &&
         (!slot->doc) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "the handle %s is a repository's, not a file's",
                       d_internal_text(_handle));
    }

    return (slot) ? slot->doc : NULL;
}


static struct d_git_repo*
d_internal_repo(
    struct d_host_git*  _host,
    struct d_awk_value* _handle
)
{
    const struct d_internal_slot* const slot = d_internal_slot_of(_host,
                                                                  _handle);

    // a file's handle
    if ( (slot) &&
         (!slot->repo) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "the handle %s is a file's, not a repository's",
                       d_internal_text(_handle));
    }

    return (slot) ? slot->repo : NULL;
}


/*
d_internal_add_slot
  Keeps a file or a repository, and gives its handle.
*/
static bool
d_internal_add_slot(
    struct d_host_git*  _host,
    struct d_awk_value* _out,
    struct d_git_doc*   _doc,
    struct d_git_repo*  _repo
)
{
    // room for one more
    if (_host->slot_count >= _host->slot_capacity)
    {
        const size_t                  wanted = (_host->slot_capacity > 0u)
                                               ? (_host->slot_capacity * 2u)
                                               : 8u;
        struct d_internal_slot* const grown  =
            realloc(_host->slots, wanted * sizeof(*grown));

        // what cannot be kept is freed, and no handle given
        if (!grown)
        {
            d_git_doc_free(_doc);
            d_git_repo_free(_repo);

            return d_internal_fail(_host, _out, "out of memory");
        }

        _host->slots         = grown;
        _host->slot_capacity = wanted;
    }

    _host->slots[_host->slot_count].doc  = _doc;
    _host->slots[_host->slot_count].repo = _repo;
    ++_host->slot_count;

    return d_internal_set_number(_out, (double)_host->slot_count);
}


/*
d_internal_changed
  Gives 1 for a change kept, or -1 for one refused, the file saying why.
*/
static bool
d_internal_changed(
    struct d_host_git*      _host,
    struct d_awk_value*     _out,
    const struct d_git_doc* _doc,
    bool                    _ok
)
{
    // refused: the file says why
    if (!_ok)
    {
        return d_internal_fail(_host, _out, d_git_doc_problem(_doc));
    }

    return d_internal_set_number(_out, 1.0);
}


/*
d_internal_record
  The file a handle names and the index, from 0, of one of its records --
`_count` of them -- an argument names from 1; NULL, git_error() saying
why, for no file or no such record.
*/
static struct d_git_doc*
d_internal_record(
    struct d_host_git*  _host,
    struct d_awk_value* _handle,
    struct d_awk_value* _index,
    d_internal_fn_count _count,
    size_t*             _out_index
)
{
    struct d_git_doc* const doc   = d_internal_doc(_host, _handle);
    const size_t            index = d_internal_index(_index);

    // a file, and a record of it
    if ( (!doc) ||
         (index == 0u) ||
         (index > _count(d_git_doc_records(doc))) )
    {
        // a file there, and no such record
        if (doc)
        {
            (void)snprintf(_host->error, sizeof(_host->error),
                           "no record %s", d_internal_text(_index));
        }

        return NULL;
    }

    *_out_index = index - 1u;

    return doc;
}


/*
d_internal_lines_of, d_internal_patterns_of, d_internal_sections_of,
d_internal_entries_of, d_internal_problems_of
  How many records of a kind a file has.
*/
static size_t
d_internal_lines_of(
    const struct d_git_text* _text
)
{
    return _text->line_count;
}


static size_t
d_internal_patterns_of(
    const struct d_git_text* _text
)
{
    return _text->pattern_count;
}


static size_t
d_internal_sections_of(
    const struct d_git_text* _text
)
{
    return _text->section_count;
}


static size_t
d_internal_entries_of(
    const struct d_git_text* _text
)
{
    return _text->entry_count;
}


static size_t
d_internal_problems_of(
    const struct d_git_text* _text
)
{
    return _text->problem_count;
}


/*
d_internal_count
  Gives how many records of a kind a file has.
*/
static bool
d_internal_count(
    struct d_host_git*  _host,
    struct d_awk_value* _handle,
    struct d_awk_value* _out,
    d_internal_fn_count _count
)
{
    const struct d_git_doc* const doc = d_internal_doc(_host, _handle);

    // a file open
    if (!doc)
    {
        return d_internal_fail(_host, _out, _host->error);
    }

    return d_internal_set_number(_out,
                                 (double)_count(d_git_doc_records(doc)));
}


/*
d_internal_repo_fail
  Gives -1, git_error() saying why the repository refused.
*/
static bool
d_internal_repo_fail(
    struct d_host_git*       _host,
    struct d_awk_value*      _out,
    const struct d_git_repo* _repo
)
{
    return d_internal_fail(_host, _out, d_git_repo_problem(_repo));
}


//==============================================================================
// 4.  FILES
//==============================================================================


/*
d_internal_kind_arg
  The kind an argument names, or D_GIT_TEXT_OTHER, git_error() saying so.
*/
static enum d_git_text_kind
d_internal_kind_arg(
    struct d_host_git*  _host,
    struct d_awk_value* _value
)
{
    const char* const          name = d_internal_text(_value);
    const enum d_git_text_kind kind = d_git_text_kind_named(name);

    // a name none is
    if (kind == D_GIT_TEXT_OTHER)
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "%s is not a kind of file git reads", name);
    }

    return kind;
}


/*
git_open(path[, kind]), git_new(path[, kind[, text]])
  A file from disk, or of a text.
*/
static bool
d_internal_git_open(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const   host = _user;
    const enum d_git_text_kind kind = (_count > 1u)
                                      ? d_internal_kind_arg(host, _args[1])
                                      : D_GIT_TEXT_OTHER;
    char                       why[512];

    (void)_problem;
    (void)_capacity;

    // a kind named that none is
    if ( (_count > 1u) &&
         (kind == D_GIT_TEXT_OTHER) )
    {
        return d_internal_fail(host, _out, host->error);
    }

    char* const path = d_internal_copy(_args[0], NULL);

    // a path that could not be copied
    if (!path)
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    struct d_git_doc* const doc = d_git_doc_read(path, kind, why,
                                                 sizeof(why));

    free(path);

    return (doc) ? d_internal_add_slot(host, _out, doc, NULL)
                 : d_internal_fail(host, _out, why);
}


static bool
d_internal_git_new(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const   host   = _user;
    const bool                 named  = ( (_count > 1u) &&
                                          (d_internal_text(_args[1])[0]
                                               != '\0') );
    const enum d_git_text_kind kind   = (named)
                                        ? d_internal_kind_arg(host, _args[1])
                                        : D_GIT_TEXT_OTHER;
    size_t                     length = 0u;
    char                       why[512];

    (void)_problem;
    (void)_capacity;

    // a kind named that none is -- "" for the path's
    if ( (named) &&
         (kind == D_GIT_TEXT_OTHER) )
    {
        return d_internal_fail(host, _out, host->error);
    }

    char* const path = d_internal_copy(_args[0], NULL);
    char* const text = (_count > 2u) ? d_internal_copy(_args[2], &length)
                                     : NULL;

    // the arguments copied
    if ( (!path) ||
         ( (_count > 2u) && (!text) ) )
    {
        free(path);
        free(text);

        return d_internal_fail(host, _out, "out of memory");
    }

    struct d_git_doc* const doc = d_git_doc_make(path, kind, text, length,
                                                 why, sizeof(why));

    free(path);
    free(text);

    return (doc) ? d_internal_add_slot(host, _out, doc, NULL)
                 : d_internal_fail(host, _out, why);
}


/*
git_save(f[, path])
  1 written, 0 unchanged -- the file there holds the text already -- or
-1.
*/
static bool
d_internal_git_save(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const      host = _user;
    const struct d_git_doc* const doc  = d_internal_doc(host, _args[0]);
    char                          why[512];

    (void)_problem;
    (void)_capacity;

    // a file open
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    char* const path = (_count > 1u) ? d_internal_copy(_args[1], NULL)
                                     : NULL;

    // a path named, copied
    if ( (_count > 1u) &&
         (!path) )
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    const enum d_git_saved saved = d_git_doc_save(doc, path, why,
                                                  sizeof(why));

    free(path);

    // why it was not saved
    if (saved == D_GIT_SAVE_FAILED)
    {
        return d_internal_fail(host, _out, why);
    }

    return d_internal_set_flag(_out, (saved == D_GIT_SAVE_WRITTEN));
}


/*
git_close(h)
  1, the file or repository let go of; 0 for nothing open.
*/
static bool
d_internal_git_close(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const      host = _user;
    struct d_internal_slot* const slot = d_internal_slot_of(host, _args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // something open
    if (slot)
    {
        d_git_doc_free(slot->doc);
        d_git_repo_free(slot->repo);
        slot->doc  = NULL;
        slot->repo = NULL;
    }

    return d_internal_set_flag(_out, (slot != NULL));
}


/*
git_error()
  Why the last failure failed.
*/
static bool
d_internal_git_error(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    const struct d_host_git* const host = _user;

    (void)_args;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_set_text(_out, host->error);
}


// d_internal_about
//   enum: what of a file git_path and its kin give.
enum d_internal_about
{
    D_INTERNAL_ABOUT_PATH = 0,
    D_INTERNAL_ABOUT_KIND,
    D_INTERNAL_ABOUT_TEXT,
    D_INTERNAL_ABOUT_CHANGES
};


/*
d_internal_about_doc
  Gives a file's path, kind, text, or how many changes it has kept.
*/
static bool
d_internal_about_doc(
    struct d_host_git*    _host,
    struct d_awk_value*   _handle,
    struct d_awk_value*   _out,
    enum d_internal_about _about
)
{
    const struct d_git_doc* const doc    = d_internal_doc(_host, _handle);
    size_t                        length = 0u;

    // a file open
    if (!doc)
    {
        return d_internal_fail(_host, _out, _host->error);
    }

    // a table of what is asked
    switch (_about)
    {
        case D_INTERNAL_ABOUT_PATH:
            return d_internal_set_text(_out, d_git_doc_path(doc));
        case D_INTERNAL_ABOUT_KIND:
            return d_internal_set_text(_out, d_git_text_kind_name(
                                                 d_git_doc_kind(doc)));
        case D_INTERNAL_ABOUT_TEXT:
        {
            const char* const text = d_git_doc_text(doc, &length);

            return d_internal_set_bytes(_out, text, length);
        }
        default:
            return d_internal_set_number(_out,
                                         (double)d_git_doc_changes(doc));
    }
}


/*
git_path(f), git_kind(f), git_text(f), git_changes(f)
*/
static bool
d_internal_git_path(
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

    return d_internal_about_doc(_user, _args[0], _out, D_INTERNAL_ABOUT_PATH);
}


static bool
d_internal_git_kind(
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

    return d_internal_about_doc(_user, _args[0], _out, D_INTERNAL_ABOUT_KIND);
}


static bool
d_internal_git_text(
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

    return d_internal_about_doc(_user, _args[0], _out, D_INTERNAL_ABOUT_TEXT);
}


static bool
d_internal_git_changes(
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

    return d_internal_about_doc(_user, _args[0], _out,
                                D_INTERNAL_ABOUT_CHANGES);
}


/*
git_kind_of(path)
  The kind a file's name says: ignore, attributes, config, or other.
*/
static bool
d_internal_git_kind_of(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    (void)_user;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_internal_set_text(_out, d_git_text_kind_name(
                                         d_git_text_kind_of(
                                             d_internal_text(_args[0]))));
}


/*
git_problems(f), git_problem(f, i, field)
  What git refuses in a file: line, what.
*/
static bool
d_internal_git_problems(
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

    return d_internal_count(_user, _args[0], _out, d_internal_problems_of);
}


static bool
d_internal_git_problem(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = { "line", "what" };
    struct d_host_git* const host     = _user;
    size_t                   index    = 0u;
    const struct d_git_doc*  doc      = d_internal_record(
                                            host, _args[0], _args[1],
                                            d_internal_problems_of, &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a file, and a problem of it
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_git_text* const         text    = d_git_doc_records(doc);
    const struct d_git_text_problem* const problem = &text->problems[index];
    const char* const                      field   = d_internal_text(_args[2]);

    // a table of the fields
    switch (d_internal_field(field, fields, 2u))
    {
        case 0:  return d_internal_set_number(_out,
                                              (double)(problem->line + 1u));
        case 1:  return d_internal_set_text(_out, d_git_text_string(
                                                      text, problem->what));
        default: return d_internal_unknown_field(host, _out, field);
    }
}


//==============================================================================
// 5.  LINES
//==============================================================================


/*
d_internal_line_kind_name
  A line's kind as a program names it.
*/
static const char*
d_internal_line_kind_name(
    enum d_git_text_line_kind _kind
)
{
    // a table of the kinds
    switch (_kind)
    {
        case D_GIT_TEXT_BLANK:   return "blank";
        case D_GIT_TEXT_COMMENT: return "comment";
        case D_GIT_TEXT_PATTERN: return "pattern";
        case D_GIT_TEXT_MACRO:   return "macro";
        case D_GIT_TEXT_SECTION: return "section";
        case D_GIT_TEXT_ENTRY:   return "entry";
        default:                 return "refused";
    }
}


/*
git_lines(f), git_line(f, i, field)
  A file's lines: kind, text -- the first's past a byte-order mark, as a
line is set -- pattern, section, entry.
*/
static bool
d_internal_git_lines(
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

    return d_internal_count(_user, _args[0], _out, d_internal_lines_of);
}


static bool
d_internal_git_line(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = { "kind", "text", "pattern",
                                          "section", "entry" };
    struct d_host_git* const host     = _user;
    size_t                   index    = 0u;
    const struct d_git_doc*  doc      = d_internal_record(
                                            host, _args[0], _args[1],
                                            d_internal_lines_of, &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a file, and a line of it
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_git_text_line* const line =
        &d_git_doc_records(doc)->lines[index];
    const char* const                   field = d_internal_text(_args[2]);

    // a table of the fields
    switch (d_internal_field(field, fields, 5u))
    {
        case 0:  return d_internal_set_text(_out, d_internal_line_kind_name(
                                                      line->kind));
        case 1:  return d_internal_set_span(_out, doc,
                                            ( (index == 0u) &&
                                              (d_git_doc_records(doc)->bom) )
                                            ? 3u : line->start,
                                            line->end);
        case 2:  return d_internal_set_index(_out, line->pattern);
        case 3:  return d_internal_set_index(_out, line->section);
        case 4:  return d_internal_set_index(_out, line->entry);
        default: return d_internal_unknown_field(host, _out, field);
    }
}


// d_internal_line_change
//   enum: what git_insert_line and its kin do to a line.
enum d_internal_line_change
{
    D_INTERNAL_LINE_INSERT = 0,
    D_INTERNAL_LINE_ADD,
    D_INTERNAL_LINE_SET,
    D_INTERNAL_LINE_REMOVE
};


/*
d_internal_change_line
  Inserts, adds, sets or removes a line: the index, from 1, of the line
made -- 1 for one removed -- or -1, the file saying why.
*/
static bool
d_internal_change_line(
    struct d_host_git*          _host,
    struct d_awk_value**        _args,
    struct d_awk_value*         _out,
    enum d_internal_line_change _change
)
{
    struct d_git_doc* const doc   = d_internal_doc(_host, _args[0]);
    const size_t            lines = (doc) ? d_git_doc_records(doc)->line_count
                                          : 0u;
    const size_t            index = (_change == D_INTERNAL_LINE_ADD)
                                    ? (lines + 1u)
                                    : d_internal_index(_args[1]);
    const size_t            most  = ( (_change == D_INTERNAL_LINE_INSERT) ||
                                      (_change == D_INTERNAL_LINE_ADD) )
                                    ? (lines + 1u)
                                    : lines;

    // a file, and a line of it -- or, to insert, the place past its last
    if ( (!doc) ||
         (index == 0u) ||
         (index > most) )
    {
        return d_internal_fail(_host, _out, (doc) ? "no such line"
                                                  : _host->error);
    }

    // a line removed
    if (_change == D_INTERNAL_LINE_REMOVE)
    {
        return d_internal_changed(_host, _out, doc,
                                  d_git_doc_remove_line(doc, index - 1u));
    }

    char* const text = d_internal_copy(
                           _args[(_change == D_INTERNAL_LINE_ADD) ? 1 : 2],
                           NULL);
    size_t      made = D_GIT_TEXT_NONE;

    // a line made, or replaced
    if (text)
    {
        made = (_change == D_INTERNAL_LINE_SET)
               ? d_git_doc_set_line(doc, index - 1u, text)
               : d_git_doc_insert_line(doc, index - 1u, text);
    }

    free(text);

    // why not
    if (made == D_GIT_TEXT_NONE)
    {
        return d_internal_fail(_host, _out, (text) ? d_git_doc_problem(doc)
                                                   : "out of memory");
    }

    return d_internal_set_index(_out, made);
}


/*
git_insert_line(f, before, text), git_add_line(f, text),
git_set_line(f, i, text), git_remove_line(f, i)
*/
static bool
d_internal_git_insert_line(
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

    return d_internal_change_line(_user, _args, _out,
                                  D_INTERNAL_LINE_INSERT);
}


static bool
d_internal_git_add_line(
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

    return d_internal_change_line(_user, _args, _out, D_INTERNAL_LINE_ADD);
}


static bool
d_internal_git_set_line(
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

    return d_internal_change_line(_user, _args, _out, D_INTERNAL_LINE_SET);
}


static bool
d_internal_git_remove_line(
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

    return d_internal_change_line(_user, _args, _out,
                                  D_INTERNAL_LINE_REMOVE);
}


//==============================================================================
// 6.  PATTERNS
//==============================================================================


/*
git_patterns(f), git_pattern(f, i, field)
  An ignore or attribute file's patterns: line, text, pattern, negated,
directory, anywhere, quoted, macro, states.
*/
static bool
d_internal_git_patterns(
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

    return d_internal_count(_user, _args[0], _out, d_internal_patterns_of);
}


static bool
d_internal_git_pattern(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = { "line", "text", "pattern",
                                          "negated", "directory",
                                          "anywhere", "quoted", "macro",
                                          "states" };
    struct d_host_git* const host     = _user;
    size_t                   index    = 0u;
    const struct d_git_doc*  doc      = d_internal_record(
                                            host, _args[0], _args[1],
                                            d_internal_patterns_of, &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a file, and a pattern of it
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_git_text* const         text    = d_git_doc_records(doc);
    const struct d_git_text_pattern* const pattern = &text->patterns[index];
    const char* const                      field   = d_internal_text(_args[2]);

    // a table of the fields
    switch (d_internal_field(field, fields, 9u))
    {
        case 0:  return d_internal_set_number(_out,
                                              (double)(pattern->line + 1u));
        case 1:  return d_internal_set_span(_out, doc, pattern->start,
                                            pattern->end);
        case 2:  return d_internal_set_bytes(_out, &text->strings[
                                                       pattern->text],
                                             pattern->length);
        case 3:  return d_internal_set_flag(_out, pattern->negated);
        case 4:  return d_internal_set_flag(_out, pattern->directory);
        case 5:  return d_internal_set_flag(_out, pattern->anywhere);
        case 6:  return d_internal_set_flag(_out, pattern->quoted);
        case 7:  return d_internal_set_flag(_out, pattern->macro);
        case 8:  return d_internal_set_number(_out,
                                              (double)pattern->attribute_count);
        default: return d_internal_unknown_field(host, _out, field);
    }
}


/*
d_internal_state_name
  A state as a program names it.
*/
static const char*
d_internal_state_name(
    enum d_git_text_state _state
)
{
    // a table of the states
    switch (_state)
    {
        case D_GIT_TEXT_SET:         return "set";
        case D_GIT_TEXT_UNSET:       return "unset";
        case D_GIT_TEXT_UNSPECIFIED: return "unspecified";
        default:                     return "value";
    }
}


/*
git_state(f, i, n, field)
  The nth state of a pattern's line: name, state, value, text.
*/
static bool
d_internal_git_state(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = { "name", "state", "value", "text" };
    struct d_host_git* const host     = _user;
    size_t                   index    = 0u;
    const struct d_git_doc*  doc      = d_internal_record(
                                            host, _args[0], _args[1],
                                            d_internal_patterns_of, &index);
    const size_t             nth      = d_internal_index(_args[2]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    const struct d_git_text* const text = (doc) ? d_git_doc_records(doc)
                                                : NULL;

    // a file, a pattern of it, and a state of the pattern
    if ( (!text) ||
         (nth == 0u) ||
         (nth > text->patterns[index].attribute_count) )
    {
        return d_internal_fail(host, _out, (doc) ? "no such state"
                                                 : host->error);
    }

    const struct d_git_text_attribute* const state =
        &text->attributes[text->patterns[index].first_attribute + nth - 1u];
    const char* const                        field = d_internal_text(_args[3]);

    // a table of the fields
    switch (d_internal_field(field, fields, 4u))
    {
        case 0:  return d_internal_set_span(_out, doc, state->name_start,
                                            state->name_end);
        case 1:  return d_internal_set_text(_out, d_internal_state_name(
                                                      state->state));
        case 2:  return d_internal_set_span(_out, doc, state->value_start,
                                            state->value_end);
        case 3:  return d_internal_set_span(_out, doc, state->start,
                                            state->end);
        default: return d_internal_unknown_field(host, _out, field);
    }
}


/*
git_set_state(f, i, state), git_remove_state(f, i, name)
  A state, as written -- text, -text, !text, eol=lf -- put on a pattern's
line, or every state of a name taken off it.
*/
static bool
d_internal_git_set_state(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host  = _user;
    size_t                   index = 0u;
    struct d_git_doc* const  doc   = d_internal_record(host, _args[0],
                                                       _args[1],
                                                       d_internal_patterns_of,
                                                       &index);
    char* const              text  = (doc) ? d_internal_copy(_args[2], NULL)
                                           : NULL;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a file, a pattern of it, and the state copied
    if (!text)
    {
        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    char* const                 equals = strchr(text, '=');
    const enum d_git_text_state state  = (equals) ? D_GIT_TEXT_VALUE
                                         : (text[0] == '-') ? D_GIT_TEXT_UNSET
                                         : (text[0] == '!')
                                           ? D_GIT_TEXT_UNSPECIFIED
                                           : D_GIT_TEXT_SET;
    const char* const           name   = ( (state == D_GIT_TEXT_UNSET) ||
                                           (state == D_GIT_TEXT_UNSPECIFIED) )
                                         ? &text[1]
                                         : text;

    // a value apart from its name
    if (equals)
    {
        *equals = '\0';
    }

    const bool ok = d_git_doc_set_state(doc, index, name, state,
                                        (equals) ? (equals + 1) : NULL);

    free(text);

    return d_internal_changed(host, _out, doc, ok);
}


static bool
d_internal_git_remove_state(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host  = _user;
    size_t                   index = 0u;
    struct d_git_doc* const  doc   = d_internal_record(host, _args[0],
                                                       _args[1],
                                                       d_internal_patterns_of,
                                                       &index);
    char* const              name  = (doc) ? d_internal_copy(_args[2], NULL)
                                           : NULL;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a file, a pattern of it, and the name copied
    if (!name)
    {
        return d_internal_fail(host, _out, (doc) ? "out of memory"
                                                 : host->error);
    }

    const bool ok = d_git_doc_remove_state(doc, index, name);

    free(name);

    return d_internal_changed(host, _out, doc, ok);
}


/*
d_internal_query_of
  A path as a lookup asks of it: a directory's when it ends with `/`, the
slashes off.
*/
static struct d_git_text_query
d_internal_query_of(
    const char* _path
)
{
    struct d_git_text_query query;
    size_t                  length = strlen(_path);

    memset(&query, 0, sizeof(query));
    query.directory = ( (length > 0u) && (_path[length - 1u] == '/') );

    // the trailing slashes off
    while ( (length > 0u) &&
            (_path[length - 1u] == '/') )
    {
        --length;
    }

    query.path   = _path;
    query.length = length;

    return query;
}


/*
d_internal_file_scope
  A file as the one scope of a lookup, from the top of the work tree, its
macros counting.
*/
static struct d_git_text_scope
d_internal_file_scope(
    const struct d_git_doc* _doc
)
{
    struct d_git_text_scope scope;

    scope.text   = d_git_doc_records(_doc);
    scope.base   = "";
    scope.macros = true;

    return scope;
}


/*
d_internal_file_match
  The pattern of a file that decides a path, from 0: an ignore file's as
git decides, a directory above the path first; an attribute file's the
last that matches it; D_GIT_TEXT_NONE for none.
*/
static size_t
d_internal_file_match(
    const struct d_git_doc* _doc,
    const char*             _path,
    bool*                   _out_ignored
)
{
    const struct d_git_text_scope scope = d_internal_file_scope(_doc);
    const struct d_git_text_query query = d_internal_query_of(_path);
    struct d_git_text_match       match;

    *_out_ignored = false;

    // an ignore file decides as git does
    if (scope.text->kind == D_GIT_TEXT_IGNORE)
    {
        const bool decided = d_git_text_ignored(&scope, 1u, &query, &match);

        *_out_ignored = ( (decided) && (match.ignored) );

        return (decided) ? match.pattern : D_GIT_TEXT_NONE;
    }

    // else the last pattern that matches
    for (size_t at = scope.text->pattern_count; at > 0u; --at)
    {
        // one that matches
        if (d_git_text_matches(&scope, at - 1u, &query))
        {
            return at - 1u;
        }
    }

    return D_GIT_TEXT_NONE;
}


/*
git_match(f, path)
  The pattern that decides a path, from 1; 0 for none.
*/
static bool
d_internal_git_match(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const      host    = _user;
    const struct d_git_doc* const doc     = d_internal_doc(host, _args[0]);
    bool                          ignored = false;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a file open
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_set_index(_out, d_internal_file_match(
                                          doc, d_internal_text(_args[1]),
                                          &ignored));
}


//==============================================================================
// 7.  CONFIGURATION FILES
//==============================================================================


/*
git_sections(f), git_section(f, i, field)
  A configuration's sections: name, subsection, prefix, line, legacy,
entries, first.
*/
static bool
d_internal_git_sections(
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

    return d_internal_count(_user, _args[0], _out, d_internal_sections_of);
}


static bool
d_internal_git_section(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = { "name", "subsection", "prefix",
                                          "line", "legacy", "entries",
                                          "first" };
    struct d_host_git* const host     = _user;
    size_t                   index    = 0u;
    const struct d_git_doc*  doc      = d_internal_record(
                                            host, _args[0], _args[1],
                                            d_internal_sections_of, &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a file, and a section of it
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_git_text* const         text    = d_git_doc_records(doc);
    const struct d_git_text_section* const section = &text->sections[index];
    const char* const                      prefix  =
        d_git_text_string(text, section->prefix);
    const char* const                      dot     = strchr(prefix, '.');
    const char* const                      field   = d_internal_text(_args[2]);

    // a table of the fields
    switch (d_internal_field(field, fields, 7u))
    {
        case 0:  return d_internal_set_span(_out, doc, section->name_start,
                                            section->name_end);
        case 1:  return d_internal_set_text(_out, (dot) ? (dot + 1) : "");
        case 2:  return d_internal_set_text(_out, prefix);
        case 3:  return d_internal_set_number(_out,
                                              (double)(section->line + 1u));
        case 4:  return d_internal_set_flag(_out, section->legacy);
        case 5:  return d_internal_set_number(_out,
                                              (double)section->entry_count);
        case 6:  return d_internal_set_index(_out, (section->entry_count > 0u)
                                                   ? section->first_entry
                                                   : D_GIT_TEXT_NONE);
        default: return d_internal_unknown_field(host, _out, field);
    }
}


/*
git_entries(f), git_entry(f, i, field)
  A configuration's entries: key, name, value, has_value, raw, line,
last_line, section, text.
*/
static bool
d_internal_git_entries(
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

    return d_internal_count(_user, _args[0], _out, d_internal_entries_of);
}


static bool
d_internal_git_entry(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = { "key", "name", "value",
                                          "has_value", "raw", "line",
                                          "last_line", "section", "text" };
    struct d_host_git* const host     = _user;
    size_t                   index    = 0u;
    const struct d_git_doc*  doc      = d_internal_record(
                                            host, _args[0], _args[1],
                                            d_internal_entries_of, &index);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a file, and an entry of it
    if (!doc)
    {
        return d_internal_fail(host, _out, host->error);
    }

    const struct d_git_text* const       text  = d_git_doc_records(doc);
    const struct d_git_text_entry* const entry = &text->entries[index];
    const char* const                    field = d_internal_text(_args[2]);

    // a table of the fields
    switch (d_internal_field(field, fields, 9u))
    {
        case 0:  return d_internal_set_text(_out, d_git_text_string(
                                                      text, entry->key));
        case 1:  return d_internal_set_span(_out, doc, entry->name_start,
                                            entry->name_end);
        case 2:  return d_internal_set_text(_out, d_git_text_string(
                                                      text, entry->value));
        case 3:  return d_internal_set_flag(_out,
                                            (entry->value != D_GIT_TEXT_NONE));
        case 4:  return d_internal_set_span(_out, doc, entry->value_start,
                                            entry->value_end);
        case 5:  return d_internal_set_number(_out,
                                              (double)(entry->line + 1u));
        case 6:  return d_internal_set_number(_out,
                                              (double)(entry->last_line + 1u));
        case 7:  return d_internal_set_index(_out, entry->section);
        case 8:  return d_internal_set_span(_out, doc, entry->start,
                                            entry->end);
        default: return d_internal_unknown_field(host, _out, field);
    }
}


/*
d_internal_canonical
  A key as git compares keys, or -- `_section` -- a section's name,
"section" or "section.subsection", its section in lower case; allocated,
NULL for one git refuses.
*/
static char*
d_internal_canonical(
    const char* _name,
    bool        _section
)
{
    struct d_git_buffer canonical;

    memset(&canonical, 0, sizeof(canonical));

    // a key, as git takes one
    if (!_section)
    {
        // one git refuses
        if (!d_git_text_key(&canonical, _name))
        {
            d_git_buffer_free(&canonical);

            return NULL;
        }

        return d_git_buffer_take(&canonical);
    }

    const char* const dot    = strchr(_name, '.');
    const size_t      length = (dot) ? (size_t)(dot - _name) : strlen(_name);

    // the section's name in lower case, the subsection as it is
    for (size_t at = 0u; at < length; ++at)
    {
        const char byte = _name[at];

        (void)d_git_buffer_add_char(&canonical,
                                    ( (byte >= 'A') && (byte <= 'Z') )
                                    ? (char)(byte + ('a' - 'A'))
                                    : byte);
    }

    (void)d_git_buffer_add_text(&canonical, (dot) ? dot : "");

    return d_git_buffer_take(&canonical);
}


/*
git_find_entry(f, key[, after]), git_find_section(f, name[, after])
  The first entry of a key, or section of a name, after `after`; 0 for
none.
*/
static bool
d_internal_find(
    struct d_host_git*   _host,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    bool                 _section
)
{
    const struct d_git_doc* const doc   = d_internal_doc(_host, _args[0]);
    const size_t                  after = (_count > 2u)
                                          ? d_internal_index(_args[2])
                                          : 0u;

    // a file open
    if (!doc)
    {
        return d_internal_fail(_host, _out, _host->error);
    }

    char* const  name  = d_internal_canonical(d_internal_text(_args[1]),
                                              _section);
    const size_t from  = (after > 0u) ? (after - 1u) : D_GIT_TEXT_NONE;
    const size_t found = (!name) ? D_GIT_TEXT_NONE
                         : (_section)
                           ? d_git_text_find_section(d_git_doc_records(doc),
                                                     name, from)
                           : d_git_text_find_entry(d_git_doc_records(doc),
                                                   name, from);

    free(name);

    return d_internal_set_index(_out, found);
}


static bool
d_internal_git_find_entry(
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

    return d_internal_find(_user, _args, _count, _out, false);
}


static bool
d_internal_git_find_section(
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

    return d_internal_find(_user, _args, _count, _out, true);
}


// d_internal_config_change
//   enum: what git_set and its kin do to a configuration.
enum d_internal_config_change
{
    D_INTERNAL_CONFIG_SET = 0,
    D_INTERNAL_CONFIG_ADD,
    D_INTERNAL_CONFIG_UNSET,
    D_INTERNAL_CONFIG_RENAME,
    D_INTERNAL_CONFIG_REMOVE
};


/*
d_internal_change_config
  Sets, adds or unsets a key, or renames or removes a section, as git
config does: 1, or -1, the file saying why.
*/
static bool
d_internal_change_config(
    struct d_host_git*            _host,
    struct d_awk_value**          _args,
    size_t                        _count,
    struct d_awk_value*           _out,
    enum d_internal_config_change _change
)
{
    struct d_git_doc* const doc   = d_internal_doc(_host, _args[0]);
    char* const             name  = (doc) ? d_internal_copy(_args[1], NULL)
                                          : NULL;
    const bool              other = ( (_change != D_INTERNAL_CONFIG_UNSET) &&
                                      (_change != D_INTERNAL_CONFIG_REMOVE) );
    char* const             value = ( (name) && (other) )
                                    ? d_internal_copy(_args[2], NULL)
                                    : NULL;
    bool                    ok    = false;

    // a file, and the arguments copied
    if ( (!name) ||
         ( (other) && (!value) ) )
    {
        free(name);

        return d_internal_fail(_host, _out, (doc) ? "out of memory"
                                                  : _host->error);
    }

    // a table of the changes
    switch (_change)
    {
        case D_INTERNAL_CONFIG_SET:
            ok = d_git_doc_set(doc, name, value);
            break;
        case D_INTERNAL_CONFIG_ADD:
            ok = d_git_doc_add(doc, name, value);
            break;
        case D_INTERNAL_CONFIG_UNSET:
            ok = d_git_doc_unset(doc, name, ( (_count > 2u) &&
                                              (d_awk_value_is_true(
                                                   _args[2])) ));
            break;
        case D_INTERNAL_CONFIG_RENAME:
            ok = d_git_doc_rename_section(doc, name, value);
            break;
        default:
            ok = d_git_doc_remove_section(doc, name);
            break;
    }

    free(name);
    free(value);

    return d_internal_changed(_host, _out, doc, ok);
}


/*
git_set(f, key, value), git_add(f, key, value), git_unset(f, key[, all]),
git_rename_section(f, name, to), git_remove_section(f, name)
*/
static bool
d_internal_git_set(
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

    return d_internal_change_config(_user, _args, _count, _out,
                                    D_INTERNAL_CONFIG_SET);
}


static bool
d_internal_git_add(
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

    return d_internal_change_config(_user, _args, _count, _out,
                                    D_INTERNAL_CONFIG_ADD);
}


static bool
d_internal_git_unset(
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

    return d_internal_change_config(_user, _args, _count, _out,
                                    D_INTERNAL_CONFIG_UNSET);
}


static bool
d_internal_git_rename_section(
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

    return d_internal_change_config(_user, _args, _count, _out,
                                    D_INTERNAL_CONFIG_RENAME);
}


static bool
d_internal_git_remove_section(
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

    return d_internal_change_config(_user, _args, _count, _out,
                                    D_INTERNAL_CONFIG_REMOVE);
}


/*
git_set_entry(f, i[, value]), git_remove_entry(f, i)
  One entry given a value -- its name alone without one -- or taken away.
*/
static bool
d_internal_change_entry(
    struct d_host_git*   _host,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    bool                 _remove
)
{
    size_t                  index = 0u;
    struct d_git_doc* const doc   = d_internal_record(_host, _args[0],
                                                      _args[1],
                                                      d_internal_entries_of,
                                                      &index);
    char* const             value = ( (doc) && (!_remove) && (_count > 2u) )
                                    ? d_internal_copy(_args[2], NULL)
                                    : NULL;

    // a file, an entry of it, and a value copied when one is given
    if ( (!doc) ||
         ( (!_remove) && (_count > 2u) && (!value) ) )
    {
        return d_internal_fail(_host, _out, (doc) ? "out of memory"
                                                  : _host->error);
    }

    const bool ok = (_remove) ? d_git_doc_remove_entry(doc, index)
                              : d_git_doc_set_entry(doc, index, value);

    free(value);

    return d_internal_changed(_host, _out, doc, ok);
}


static bool
d_internal_git_set_entry(
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

    return d_internal_change_entry(_user, _args, _count, _out, false);
}


static bool
d_internal_git_remove_entry(
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

    return d_internal_change_entry(_user, _args, _count, _out, true);
}


//==============================================================================
// 8.  CONFIGURATION
//==============================================================================


/*
d_internal_values_add
  A value kept among a key's.
*/
static void
d_internal_values_add(
    struct d_internal_values* _values,
    const char*               _value
)
{
    // room for one more
    if ( (!_values->failed) &&
         (_values->count == _values->capacity) )
    {
        const size_t       wanted = (_values->capacity > 0u)
                                    ? (_values->capacity * 2u)
                                    : 8u;
        const char** const grown  = realloc(_values->items,
                                            wanted * sizeof(*grown));

        _values->failed   = (grown == NULL);
        _values->items    = (grown) ? grown : _values->items;
        _values->capacity = (grown) ? wanted : _values->capacity;
    }

    // kept, unless memory ran out
    if (!_values->failed)
    {
        _values->items[_values->count++] = _value;
    }

    return;
}


/*
d_internal_values_of
  The values a key has, lowest first, in a file a handle names -- before
any line git cannot read -- or in a repository's configuration; false,
git_error() saying why, for no such handle, a key git refuses, or memory
that ran out.
  Long as a file's entries and a repository's settings are each gathered
here.
*/
static bool
d_internal_values_of(
    struct d_host_git*        _host,
    struct d_awk_value*       _handle,
    const char*               _key,
    struct d_internal_values* _out
)
{
    const struct d_internal_slot* const slot = d_internal_slot_of(_host,
                                                                  _handle);
    char* const                         key  = d_internal_canonical(_key,
                                                                    false);

    memset(_out, 0, sizeof(*_out));

    // something open, and a key git takes
    if ( (!slot) ||
         (!key) )
    {
        // open: the key is why
        if (slot)
        {
            (void)snprintf(_host->error, sizeof(_host->error),
                           "%s is a key git refuses", _key);
        }

        free(key);

        return false;
    }

    const struct d_git_text* const text = (slot->doc)
                                          ? d_git_doc_records(slot->doc)
                                          : NULL;

    // a file's entries of the key
    for (size_t at = (text) ? d_git_text_find_entry(text, key,
                                                    D_GIT_TEXT_NONE)
                            : D_GIT_TEXT_NONE;
         at != D_GIT_TEXT_NONE;
         at = d_git_text_find_entry(text, key, at))
    {
        d_internal_values_add(_out, d_git_text_string(
                                        text, text->entries[at].value));
    }

    // a repository's settings of the key
    for (size_t at = (slot->repo) ? d_git_repo_find_setting(slot->repo, key,
                                                            D_GIT_REPO_NONE)
                                  : D_GIT_REPO_NONE;
         at != D_GIT_REPO_NONE;
         at = d_git_repo_find_setting(slot->repo, key, at))
    {
        d_internal_values_add(_out, d_git_repo_setting(slot->repo,
                                                       at)->value);
    }

    free(key);

    // memory that ran out
    if (_out->failed)
    {
        free(_out->items);
        (void)snprintf(_host->error, sizeof(_host->error), "out of memory");

        return false;
    }

    return true;
}


// d_internal_config_read
//   enum: what git_config and its kin give of a key's values.
enum d_internal_config_read
{
    D_INTERNAL_READ_LAST = 0,
    D_INTERNAL_READ_ALL,
    D_INTERNAL_READ_COUNT,
    D_INTERNAL_READ_BOOL,
    D_INTERNAL_READ_INT
};


/*
d_internal_read_typed
  Gives the last value of a key as git reads a boolean, or a number; the
default -- 0 unless given -- for a key with none; -1 for a value git would
refuse, a name alone among them for a number.
*/
static bool
d_internal_read_typed(
    struct d_host_git*              _host,
    struct d_awk_value*             _out,
    const struct d_internal_values* _values,
    struct d_awk_value*             _default,
    bool                            _boolean
)
{
    const char* const last   = (_values->count > 0u)
                               ? _values->items[_values->count - 1u]
                               : NULL;
    bool              flag   = false;
    int64_t           number = 0;

    // none: the default, or 0
    if (_values->count == 0u)
    {
        return d_internal_set_number(_out, (_default)
                                           ? d_awk_value_number(_default)
                                           : 0.0);
    }

    // a boolean
    if (_boolean)
    {
        return (d_git_text_bool(last, &flag))
               ? d_internal_set_flag(_out, flag)
               : d_internal_fail(_host, _out, "a boolean git would refuse");
    }

    return ( (last) &&
             (d_git_text_int(last, &number)) )
           ? d_internal_set_number(_out, (double)number)
           : d_internal_fail(_host, _out, "a number git would refuse");
}


/*
d_internal_read_config
  Gives a key's last value, every value a line each, how many, or the last
as a boolean or a number.
*/
static bool
d_internal_read_config(
    struct d_host_git*          _host,
    struct d_awk_value**        _args,
    size_t                      _count,
    struct d_awk_value*         _out,
    enum d_internal_config_read _read
)
{
    char* const              key = d_internal_copy(_args[1], NULL);
    struct d_internal_values values;
    struct d_git_buffer      all;
    bool                     set = false;

    // the key copied, and its values
    if ( (!key) ||
         (!d_internal_values_of(_host, _args[0], key, &values)) )
    {
        free(key);

        return d_internal_fail(_host, _out, (key) ? _host->error
                                                  : "out of memory");
    }

    memset(&all, 0, sizeof(all));
    free(key);

    // a table of what is read
    switch (_read)
    {
        case D_INTERNAL_READ_LAST:
            set = d_internal_set_text(_out, (values.count > 0u)
                                            ? values.items[values.count - 1u]
                                            : "");
            break;
        case D_INTERNAL_READ_ALL:
            // each value, a line
            for (size_t at = 0u; at < values.count; ++at)
            {
                (void)d_git_buffer_add_text(&all, (values.items[at])
                                                  ? values.items[at] : "");
                (void)d_git_buffer_add_char(&all, '\n');
            }

            set = d_internal_set_buffer(_host, _out, &all);
            break;
        case D_INTERNAL_READ_COUNT:
            set = d_internal_set_number(_out, (double)values.count);
            break;
        default:
            set = d_internal_read_typed(_host, _out, &values,
                                        (_count > 2u) ? _args[2] : NULL,
                                        (_read == D_INTERNAL_READ_BOOL));
            break;
    }

    free(values.items);

    return set;
}


/*
git_config(h, key), git_config_all(h, key), git_config_count(h, key),
git_config_bool(h, key[, default]), git_config_int(h, key[, default])
*/
static bool
d_internal_git_config(
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

    return d_internal_read_config(_user, _args, _count, _out,
                                  D_INTERNAL_READ_LAST);
}


static bool
d_internal_git_config_all(
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

    return d_internal_read_config(_user, _args, _count, _out,
                                  D_INTERNAL_READ_ALL);
}


static bool
d_internal_git_config_count(
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

    return d_internal_read_config(_user, _args, _count, _out,
                                  D_INTERNAL_READ_COUNT);
}


static bool
d_internal_git_config_bool(
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

    return d_internal_read_config(_user, _args, _count, _out,
                                  D_INTERNAL_READ_BOOL);
}


static bool
d_internal_git_config_int(
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

    return d_internal_read_config(_user, _args, _count, _out,
                                  D_INTERNAL_READ_INT);
}


//==============================================================================
// 9.  TEXT
//==============================================================================


/*
git_wildmatch(pattern, text[, flags])
  1 when git's wildmatch matches the whole text; flags among pathname and
casefold.
*/
static bool
d_internal_git_wildmatch(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host    = _user;
    size_t                   length  = 0u;
    char* const              pattern = d_internal_copy(_args[0], &length);
    const char* const        flags   = (_count > 2u)
                                       ? d_internal_text(_args[2])
                                       : "";
    const unsigned           wanted  =
        ( (strstr(flags, "pathname")) ? D_GIT_TEXT_PATHNAME : 0u ) |
        ( (strstr(flags, "casefold")) ? D_GIT_TEXT_CASEFOLD : 0u );

    (void)_problem;
    (void)_capacity;

    // the pattern copied
    if (!pattern)
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    size_t            text_length = 0u;
    const char* const text        = d_awk_value_text(_args[1], NULL,
                                                     &text_length);
    const bool        matched     = d_git_text_wildmatch(pattern, length,
                                                         text, text_length,
                                                         wanted);

    free(pattern);

    return d_internal_set_flag(_out, matched);
}


/*
git_key(key)
  A key as git compares keys; "" for one git refuses.
*/
static bool
d_internal_git_key(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    char* const key = d_internal_canonical(d_internal_text(_args[0]), false);
    const bool  set = d_internal_set_text(_out, key);

    (void)_user;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    free(key);

    return set;
}


// d_internal_quoting
//   enum: how git_quote_value and its kin quote text.
enum d_internal_quoting
{
    D_INTERNAL_QUOTE_VALUE = 0,
    D_INTERNAL_QUOTE_SUBSECTION,
    D_INTERNAL_QUOTE_PATH,
    D_INTERNAL_UNQUOTE
};


/*
d_internal_quote
  Gives text quoted as git config writes a value or a subsection, as git
writes a path, or unquoted from C's quotes; -1 for text not quoted, or
badly.
*/
static bool
d_internal_quote(
    struct d_host_git*      _host,
    struct d_awk_value*     _in,
    struct d_awk_value*     _out,
    enum d_internal_quoting _quoting
)
{
    size_t              length = 0u;
    char* const         text   = d_internal_copy(_in, &length);
    struct d_git_buffer quoted;
    size_t              used   = 0u;
    bool                ok     = false;

    memset(&quoted, 0, sizeof(quoted));

    // a table of the quotings
    switch ( (text) ? _quoting : D_INTERNAL_UNQUOTE )
    {
        case D_INTERNAL_QUOTE_VALUE:
            ok = d_git_text_quote_value(&quoted, text);
            break;
        case D_INTERNAL_QUOTE_SUBSECTION:
            ok = d_git_text_quote_subsection(&quoted, text);
            break;
        case D_INTERNAL_QUOTE_PATH:
            ok = d_git_text_quote_path(&quoted, text, length);
            break;
        default:
            ok = ( (text) &&
                   (d_git_text_unquote(&quoted, text, length, &used)) &&
                   (used == length) );
            break;
    }

    free(text);

    // text that cannot be read so
    if (!ok)
    {
        d_git_buffer_free(&quoted);

        return d_internal_fail(_host, _out, (text) ? "not quoted as git "
                                                     "quotes text"
                                                   : "out of memory");
    }

    return d_internal_set_buffer(_host, _out, &quoted);
}


/*
git_quote_value(value), git_quote_subsection(name), git_quote_path(path),
git_unquote(text)
*/
static bool
d_internal_git_quote_value(
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

    return d_internal_quote(_user, _args[0], _out, D_INTERNAL_QUOTE_VALUE);
}


static bool
d_internal_git_quote_subsection(
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

    return d_internal_quote(_user, _args[0], _out,
                            D_INTERNAL_QUOTE_SUBSECTION);
}


static bool
d_internal_git_quote_path(
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

    return d_internal_quote(_user, _args[0], _out, D_INTERNAL_QUOTE_PATH);
}


static bool
d_internal_git_unquote(
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

    return d_internal_quote(_user, _args[0], _out, D_INTERNAL_UNQUOTE);
}


//==============================================================================
// 10. REPOSITORIES
//==============================================================================


/*
git_repo([path])
  The repository a path -- the working directory's, without one -- is in.
*/
static bool
d_internal_git_repo(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host = _user;
    char* const              path = (_count > 0u)
                                    ? d_internal_copy(_args[0], NULL)
                                    : NULL;
    char                     why[512];

    (void)_problem;
    (void)_capacity;

    // a path named, copied
    if ( (_count > 0u) &&
         (!path) )
    {
        return d_internal_fail(host, _out, "out of memory");
    }

    struct d_git_repo* const repo = d_git_repo_open(
                                        ( (path) && (path[0] != '\0') )
                                        ? path
                                        : ".",
                                        why, sizeof(why));

    free(path);

    return (repo) ? d_internal_add_slot(host, _out, NULL, repo)
                  : d_internal_fail(host, _out, why);
}


// d_internal_place
//   enum: which of a repository's places git_dir and its kin give.
enum d_internal_place
{
    D_INTERNAL_PLACE_GIT_DIR = 0,
    D_INTERNAL_PLACE_COMMON_DIR,
    D_INTERNAL_PLACE_WORK_TREE,
    D_INTERNAL_PLACE_HASH
};


/*
d_internal_place_of
  Gives a repository's directory, common directory, work tree -- "" for a
bare one -- or its hash's name.
*/
static bool
d_internal_place_of(
    struct d_host_git*    _host,
    struct d_awk_value*   _handle,
    struct d_awk_value*   _out,
    enum d_internal_place _place
)
{
    const struct d_git_repo* const repo = d_internal_repo(_host, _handle);

    // a repository open
    if (!repo)
    {
        return d_internal_fail(_host, _out, _host->error);
    }

    // a table of the places
    switch (_place)
    {
        case D_INTERNAL_PLACE_GIT_DIR:
            return d_internal_set_text(_out, d_git_repo_git_dir(repo));
        case D_INTERNAL_PLACE_COMMON_DIR:
            return d_internal_set_text(_out, d_git_repo_common_dir(repo));
        case D_INTERNAL_PLACE_WORK_TREE:
            return d_internal_set_text(_out, d_git_repo_work_tree(repo));
        default:
            return d_internal_set_text(_out, (d_git_repo_hash_size(repo)
                                              == 32u) ? "sha256" : "sha1");
    }
}


/*
git_dir(r), git_common_dir(r), git_work_tree(r), git_hash(r)
*/
static bool
d_internal_git_dir(
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

    return d_internal_place_of(_user, _args[0], _out,
                               D_INTERNAL_PLACE_GIT_DIR);
}


static bool
d_internal_git_common_dir(
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

    return d_internal_place_of(_user, _args[0], _out,
                               D_INTERNAL_PLACE_COMMON_DIR);
}


static bool
d_internal_git_work_tree(
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

    return d_internal_place_of(_user, _args[0], _out,
                               D_INTERNAL_PLACE_WORK_TREE);
}


static bool
d_internal_git_hash(
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

    return d_internal_place_of(_user, _args[0], _out, D_INTERNAL_PLACE_HASH);
}


/*
d_internal_readable
  Whether a file can be opened to read.
*/
static bool
d_internal_readable(
    const char* _path
)
{
    FILE* const file = (_path) ? fopen(_path, "rb") : NULL;

    // one opened is closed
    if (file)
    {
        (void)fclose(file);
    }

    return (file != NULL);
}


/*
d_internal_global_file
  The file git config --global writes: GIT_CONFIG_GLOBAL's; else
~/.gitconfig, unless only git/config under XDG_CONFIG_HOME -- or
~/.config -- can be read; allocated.
*/
static char*
d_internal_global_file(void)
{
    const char* const named = getenv("GIT_CONFIG_GLOBAL");
    const char* const home  = getenv("HOME");
    const char* const xdg   = getenv("XDG_CONFIG_HOME");

    // one named
    if (named)
    {
        return d_internal_dup(named);
    }

    char* const user  = (home) ? d_internal_join(home, ".gitconfig") : NULL;
    char* const base  = ( (xdg) && (xdg[0] != '\0') )
                        ? d_internal_join(xdg, "git")
                        : (home) ? d_internal_join(home, ".config/git")
                                 : NULL;
    char* const other = (base) ? d_internal_join(base, "config") : NULL;

    free(base);

    // the XDG file only when it alone is there
    if ( (!d_internal_readable(user)) &&
         (d_internal_readable(other)) )
    {
        free(user);

        return other;
    }

    free(other);

    return user;
}


/*
d_internal_config_path
  The file git config writes for a scope: local the repository's config;
worktree its config.worktree, when extensions.worktreeConfig is set, else
local's; global d_internal_global_file's; system GIT_CONFIG_SYSTEM's or
/etc/gitconfig. Allocated; NULL for a scope none is.
*/
static char*
d_internal_config_path(
    const struct d_git_repo* _repo,
    const char*              _scope
)
{
    bool              set      = false;
    bool              worktree = false;
    const char* const value    = d_git_repo_config(
                                     _repo, "extensions.worktreeconfig", &set);
    const char* const system   = getenv("GIT_CONFIG_SYSTEM");

    // a table of the scopes
    if (strcmp(_scope, "global") == 0)
    {
        return d_internal_global_file();
    }
    else if (strcmp(_scope, "system") == 0)
    {
        return d_internal_dup((system) ? system : "/etc/gitconfig");
    }
    else if ( (strcmp(_scope, "worktree") == 0) &&
              (set) &&
              (d_git_text_bool(value, &worktree)) &&
              (worktree) )
    {
        return d_internal_join(d_git_repo_git_dir(_repo), "config.worktree");
    }
    else if ( (strcmp(_scope, "local") != 0) &&
              (strcmp(_scope, "worktree") != 0) )
    {
        return NULL;
    }

    return d_internal_join(d_git_repo_common_dir(_repo), "config");
}


/*
git_config_file(r[, scope])
  The file git config --scope writes -- local, worktree, global, system --
to open and change.
*/
static bool
d_internal_git_config_file(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const       host = _user;
    const struct d_git_repo* const repo = d_internal_repo(host, _args[0]);

    (void)_problem;
    (void)_capacity;

    // a repository open
    if (!repo)
    {
        return d_internal_fail(host, _out, host->error);
    }

    char* const path = d_internal_config_path(repo, (_count > 1u)
                                                    ? d_internal_text(_args[1])
                                                    : "local");
    const bool  set  = (path) ? d_internal_set_text(_out, path)
                              : d_internal_fail(host, _out, "no such scope");

    free(path);

    return set;
}


/*
git_settings(r), git_setting(r, i, field)
  A repository's settings, in git's order: key, value, has_value, file,
line, scope, included.
*/
static bool
d_internal_git_settings(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const       host = _user;
    const struct d_git_repo* const repo = d_internal_repo(host, _args[0]);

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a repository open
    if (!repo)
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_set_number(_out, (double)d_git_repo_settings(repo));
}


static bool
d_internal_git_setting(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = { "key", "value", "has_value",
                                          "file", "line", "scope",
                                          "included" };
    static const char* const scopes[] = { "system", "global", "local",
                                          "worktree", "command" };
    struct d_host_git* const       host    = _user;
    const struct d_git_repo* const repo    = d_internal_repo(host, _args[0]);
    const size_t                   index   = d_internal_index(_args[1]);
    const struct d_git_setting*    setting = ( (repo) && (index > 0u) )
                                             ? d_git_repo_setting(repo,
                                                                  index - 1u)
                                             : NULL;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a repository, and a setting of it
    if (!setting)
    {
        return d_internal_fail(host, _out, (repo) ? "no such setting"
                                                  : host->error);
    }

    const char* const field = d_internal_text(_args[2]);

    // a table of the fields
    switch (d_internal_field(field, fields, 7u))
    {
        case 0:  return d_internal_set_text(_out, setting->key);
        case 1:  return d_internal_set_text(_out, setting->value);
        case 2:  return d_internal_set_flag(_out, (setting->value != NULL));
        case 3:  return d_internal_set_text(_out, setting->file);
        case 4:  return d_internal_set_number(_out, (double)setting->line);
        case 5:  return d_internal_set_text(_out, scopes[setting->scope]);
        case 6:  return d_internal_set_flag(_out, setting->included);
        default: return d_internal_unknown_field(host, _out, field);
    }
}


//==============================================================================
// 11. OBJECTS
//==============================================================================


/*
d_internal_rev
  What a revision names, as git rev-parse names it; false, git_error()
saying why, for nothing.
*/
static bool
d_internal_rev(
    struct d_host_git*  _host,
    struct d_git_repo*  _repo,
    struct d_awk_value* _name,
    struct d_git_oid*   _out
)
{
    char* const name  = d_internal_copy(_name, NULL);
    const bool  found = ( (name) &&
                          (d_git_repo_rev(_repo, name, _out)) );

    // why not
    if (!found)
    {
        (void)snprintf(_host->error, sizeof(_host->error), "%s",
                       (name) ? d_git_repo_problem(_repo) : "out of memory");
    }

    free(name);

    return found;
}


/*
d_internal_peel
  An object read, peeled to a type as ^{type} peels: a tag to what it tags,
a commit to its tree; false, git_error() saying why, for one that does
not peel to it.
*/
static bool
d_internal_peel(
    struct d_host_git*     _host,
    struct d_git_repo*     _repo,
    struct d_git_oid*      _oid,
    enum d_git_object_type _type,
    struct d_git_object*   _out
)
{
    char       name[(2u * D_GIT_OID_MOST) + 16u];
    const bool read = d_git_repo_read(_repo, _oid, _out);

    // read, and of the type
    if ( (read) &&
         (_out->type == _type) )
    {
        return true;
    }

    d_git_object_release(_out);
    d_git_repo_hex(_repo, _oid, name);
    (void)snprintf(&name[strlen(name)], 16u, "^{%s}",
                   d_git_object_type_name(_type));

    // peeled, and read
    if ( (read) &&
         (d_git_repo_rev(_repo, name, _oid)) &&
         (d_git_repo_read(_repo, _oid, _out)) )
    {
        return true;
    }

    (void)snprintf(_host->error, sizeof(_host->error), "%s",
                   d_git_repo_problem(_repo));

    return false;
}


/*
git_rev(r, name), git_short(r, name[, least]), git_exists(r, name)
  A revision's object in hex, whole or as short as git rev-parse --short
makes it; or whether the repository holds it.
*/
static bool
d_internal_git_rev(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host = _user;
    struct d_git_repo* const repo = d_internal_repo(host, _args[0]);
    struct d_git_oid         oid;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a repository, and what the revision names
    if ( (!repo) ||
         (!d_internal_rev(host, repo, _args[1], &oid)) )
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_set_hex(_out, repo, &oid);
}


static bool
d_internal_git_short(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host  = _user;
    struct d_git_repo* const repo  = d_internal_repo(host, _args[0]);
    const size_t             least = (_count > 2u)
                                     ? d_internal_index(_args[2])
                                     : 0u;
    struct d_git_oid         oid;
    char                     hex[(2u * D_GIT_OID_MOST) + 1u];

    (void)_problem;
    (void)_capacity;

    // a repository, and what the revision names
    if ( (!repo) ||
         (!d_internal_rev(host, repo, _args[1], &oid)) )
    {
        return d_internal_fail(host, _out, host->error);
    }

    // its shortest name
    if (!d_git_repo_abbreviate(repo, &oid, least, hex))
    {
        return d_internal_repo_fail(host, _out, repo);
    }

    return d_internal_set_text(_out, hex);
}


static bool
d_internal_git_exists(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host = _user;
    struct d_git_repo* const repo = d_internal_repo(host, _args[0]);
    struct d_git_oid         oid;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a repository open
    if (!repo)
    {
        return d_internal_fail(host, _out, host->error);
    }

    return d_internal_set_flag(_out,
                               ( (d_internal_rev(host, repo, _args[1],
                                                 &oid)) &&
                                 (d_git_repo_has(repo, &oid)) ));
}


// d_internal_object_part
//   enum: what of an object git_type and its kin give.
enum d_internal_object_part
{
    D_INTERNAL_PART_TYPE = 0,
    D_INTERNAL_PART_SIZE,
    D_INTERNAL_PART_CONTENT
};


/*
d_internal_object_part_of
  Gives an object's type, size or content.
*/
static bool
d_internal_object_part_of(
    struct d_host_git*          _host,
    struct d_awk_value**        _args,
    struct d_awk_value*         _out,
    enum d_internal_object_part _part
)
{
    struct d_git_repo* const repo = d_internal_repo(_host, _args[0]);
    struct d_git_oid         oid;
    struct d_git_object      object;
    bool                     set  = false;

    memset(&object, 0, sizeof(object));

    // a repository, and what the revision names
    if ( (!repo) ||
         (!d_internal_rev(_host, repo, _args[1], &oid)) )
    {
        return d_internal_fail(_host, _out, _host->error);
    }

    // the object read
    if (!d_git_repo_read(repo, &oid, &object))
    {
        d_git_object_release(&object);

        return d_internal_repo_fail(_host, _out, repo);
    }

    // a table of the parts
    switch (_part)
    {
        case D_INTERNAL_PART_TYPE:
            set = d_internal_set_text(_out,
                                      d_git_object_type_name(object.type));
            break;
        case D_INTERNAL_PART_SIZE:
            set = d_internal_set_number(_out, (double)object.size);
            break;
        default:
            set = d_internal_set_bytes(_out, object.data, object.size);
            break;
    }

    d_git_object_release(&object);

    return set;
}


/*
git_type(r, name), git_size(r, name), git_cat(r, name)
  An object's type, its size, and its content, as git cat-file gives
them.
*/
static bool
d_internal_git_type(
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

    return d_internal_object_part_of(_user, _args, _out,
                                     D_INTERNAL_PART_TYPE);
}


static bool
d_internal_git_size(
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

    return d_internal_object_part_of(_user, _args, _out,
                                     D_INTERNAL_PART_SIZE);
}


static bool
d_internal_git_cat(
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

    return d_internal_object_part_of(_user, _args, _out,
                                     D_INTERNAL_PART_CONTENT);
}


/*
d_internal_paragraphs
  A message's subject and body as git's %s and %b give them: the first
paragraph, blank lines before it passed over, its lines joined by a
blank; and what follows it, the blank lines between passed over.
*/
static bool
d_internal_paragraphs(
    struct d_host_git*  _host,
    struct d_awk_value* _out,
    const char*         _message,
    size_t              _length,
    bool                _body
)
{
    struct d_git_buffer subject;
    size_t              at = 0u;
    bool                blank = true;

    memset(&subject, 0, sizeof(subject));

    // each line, until the paragraph's end past the first
    while (at < _length)
    {
        const char* const end   = memchr(&_message[at], '\n', _length - at);
        const size_t      stop  = (end) ? (size_t)(end - _message) : _length;
        size_t            first = at;

        // its blanks, to see whether it is a blank line
        while ( (first < stop) &&
                ( (_message[first] == ' ') || (_message[first] == '\t') ||
                  (_message[first] == '\r') ) )
        {
            ++first;
        }

        // a blank line ends the subject; before it, it is passed over
        if ( (first == stop) &&
             (!blank) )
        {
            break;
        }
        else if (first < stop)
        {
            (void)d_git_buffer_add_text(&subject, (blank) ? "" : " ");
            (void)d_git_buffer_add(&subject, &_message[first], stop - first);
            blank = false;
        }

        at = (end) ? (stop + 1u) : _length;
    }

    // the subject; or the body, past the blank lines after it
    if (!_body)
    {
        return d_internal_set_buffer(_host, _out, &subject);
    }

    d_git_buffer_free(&subject);

    // the blank lines between
    while ( (at < _length) &&
            ( (_message[at] == '\n') || (_message[at] == '\r') ||
              (_message[at] == ' ') || (_message[at] == '\t') ) )
    {
        ++at;
    }

    return d_internal_set_bytes(_out, &_message[at], _length - at);
}


/*
d_internal_person_field
  Gives a person's name, address, time or zone, by the field's place past
the person's own: 0 the name, 1 the address, 2 the time, 3 the zone.
*/
static bool
d_internal_person_field(
    struct d_awk_value*        _out,
    const struct d_git_person* _person,
    size_t                     _which
)
{
    // a table of a person's fields
    switch (_which)
    {
        case 0:  return d_internal_set_bytes(_out, _person->name,
                                             _person->name_length);
        case 1:  return d_internal_set_bytes(_out, _person->email,
                                             _person->email_length);
        case 2:  return d_internal_set_number(_out, (double)_person->time);
        default: return d_internal_set_text(_out, _person->zone);
    }
}


/*
d_internal_parents
  Gives a commit's parents in hex, apart by blanks.
*/
static bool
d_internal_parents(
    struct d_host_git*         _host,
    struct d_awk_value*        _out,
    const struct d_git_repo*   _repo,
    const struct d_git_commit* _commit
)
{
    struct d_git_buffer parents;
    char                hex[(2u * D_GIT_OID_MOST) + 1u];

    memset(&parents, 0, sizeof(parents));

    // each, in order
    for (size_t at = 0u; at < _commit->parent_count; ++at)
    {
        d_git_repo_hex(_repo, &_commit->parents[at], hex);
        (void)d_git_buffer_add_text(&parents, (at > 0u) ? " " : "");
        (void)d_git_buffer_add_text(&parents, hex);
    }

    return d_internal_set_buffer(_host, _out, &parents);
}


/*
d_internal_commit_field
  Gives a field of a commit parsed.
*/
static bool
d_internal_commit_field(
    struct d_host_git*         _host,
    struct d_awk_value*        _out,
    const struct d_git_repo*   _repo,
    const struct d_git_commit* _commit,
    size_t                     _field
)
{
    // a table of the fields: the person's from 4, the message's from 12
    switch (_field)
    {
        case 0:  return d_internal_set_text(_out, "");
        case 1:  return d_internal_set_hex(_out, _repo, &_commit->tree);
        case 2:  return d_internal_parents(_host, _out, _repo, _commit);
        case 3:  return d_internal_set_number(_out,
                                              (double)_commit->parent_count);
        case 4: case 5: case 6: case 7:
                 return d_internal_person_field(_out, &_commit->author,
                                                _field - 4u);
        case 8: case 9: case 10: case 11:
                 return d_internal_person_field(_out, &_commit->committer,
                                                _field - 8u);
        case 12: return d_internal_set_bytes(_out, _commit->encoding,
                                             _commit->encoding_length);
        case 13: return d_internal_set_bytes(_out, _commit->message,
                                             _commit->message_length);
        default: return d_internal_paragraphs(_host, _out, _commit->message,
                                              _commit->message_length,
                                              (_field == 15u));
    }
}


/*
git_commit(r, name, field)
  A commit, a tag peeled to one: oid, tree, parents, parent_count, author,
author_email, author_time, author_zone, committer and its kin, encoding,
message, subject, body.
*/
static bool
d_internal_git_commit(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = {
        "oid", "tree", "parents", "parent_count", "author", "author_email",
        "author_time", "author_zone", "committer", "committer_email",
        "committer_time", "committer_zone", "encoding", "message", "subject",
        "body" };
    struct d_host_git* const host  = _user;
    struct d_git_repo* const repo  = d_internal_repo(host, _args[0]);
    const char* const        field = d_internal_text(_args[2]);
    const size_t             which = d_internal_field(field, fields, 16u);
    struct d_git_oid         oid;
    struct d_git_object      object;
    struct d_git_commit      commit;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    memset(&object, 0, sizeof(object));
    memset(&commit, 0, sizeof(commit));

    // a repository, a commit the revision names, a field of it
    if ( (!repo) ||
         (!d_internal_rev(host, repo, _args[1], &oid)) ||
         (!d_internal_peel(host, repo, &oid, D_GIT_OBJECT_COMMIT, &object)) ||
         (!d_git_repo_parse_commit(repo, &object, &commit)) )
    {
        d_git_commit_release(&commit);
        d_git_object_release(&object);

        return d_internal_fail(host, _out, (repo) ? d_git_repo_problem(repo)
                                                  : host->error);
    }

    const bool set = (which == 16u)
                     ? d_internal_unknown_field(host, _out, field)
                     : (which == 0u)
                       ? d_internal_set_hex(_out, repo, &oid)
                       : d_internal_commit_field(host, _out, repo, &commit,
                                                 which);

    d_git_commit_release(&commit);
    d_git_object_release(&object);

    return set;
}


/*
git_tag(r, name, field)
  An annotated tag: oid, object, type, tag, tagger, tagger_email,
tagger_time, tagger_zone, message.
*/
static bool
d_internal_git_tag(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = {
        "oid", "object", "type", "tag", "tagger", "tagger_email",
        "tagger_time", "tagger_zone", "message" };
    struct d_host_git* const host  = _user;
    struct d_git_repo* const repo  = d_internal_repo(host, _args[0]);
    const char* const        field = d_internal_text(_args[2]);
    const size_t             which = d_internal_field(field, fields, 9u);
    struct d_git_oid         oid;
    struct d_git_object      object;
    struct d_git_tag         tag;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    memset(&object, 0, sizeof(object));

    // a repository, a tag the revision names
    if ( (!repo) ||
         (!d_internal_rev(host, repo, _args[1], &oid)) ||
         (!d_internal_peel(host, repo, &oid, D_GIT_OBJECT_TAG, &object)) ||
         (!d_git_repo_parse_tag(repo, &object, &tag)) )
    {
        d_git_object_release(&object);

        return d_internal_fail(host, _out, (repo) ? d_git_repo_problem(repo)
                                                  : host->error);
    }

    bool set = false;

    // a table of the fields: the tagger's from 4
    switch (which)
    {
        case 0:  set = d_internal_set_hex(_out, repo, &oid);             break;
        case 1:  set = d_internal_set_hex(_out, repo, &tag.object);      break;
        case 2:  set = d_internal_set_text(_out, d_git_object_type_name(
                                                     tag.type));         break;
        case 3:  set = d_internal_set_bytes(_out, tag.name,
                                            tag.name_length);            break;
        case 4: case 5: case 6: case 7:
                 set = (tag.has_tagger)
                       ? d_internal_person_field(_out, &tag.tagger,
                                                 which - 4u)
                       : d_internal_set_text(_out, "");                  break;
        case 8:  set = d_internal_set_bytes(_out, tag.message,
                                            tag.message_length);         break;
        default: set = d_internal_unknown_field(host, _out, field);      break;
    }

    d_git_object_release(&object);

    return set;
}


/*
d_internal_list_tree
  A tree's entries added to a list as git ls-tree writes them -- mode,
type, name, a tab, path -- each path below `_prefix`; with `_deep`, a
subtree's entries in place of its own, as ls-tree -r has it. False when a
tree cannot be read.
*/
static bool
d_internal_list_tree(
    struct d_git_repo*      _repo,
    const struct d_git_oid* _tree,
    struct d_git_buffer*    _prefix,
    bool                    _deep,
    struct d_git_buffer*    _list
)
{
    struct d_git_object     tree;
    struct d_git_tree_entry entry;
    size_t                  at    = 0u;
    bool                    ok    = d_git_repo_read(_repo, _tree, &tree);
    const size_t            stem  = _prefix->length;
    char                    line[(2u * D_GIT_OID_MOST) + 32u];
    char                    hex[(2u * D_GIT_OID_MOST) + 1u];

    // each entry, while the walk is sound
    while ( (ok) &&
            (d_git_repo_tree_next(_repo, &tree, &at, &entry)) )
    {
        const bool subtree = (entry.mode == 040000u);

        _prefix->length = stem;
        (void)d_git_buffer_add(_prefix, entry.name, entry.name_length);

        // a subtree walked, deep; else the entry written
        if ( (subtree) &&
             (_deep) )
        {
            (void)d_git_buffer_add_char(_prefix, '/');
            ok = d_internal_list_tree(_repo, &entry.oid, _prefix, true,
                                      _list);
        }
        else
        {
            d_git_repo_hex(_repo, &entry.oid, hex);
            (void)snprintf(line, sizeof(line), "%06o %s %s\t",
                           (unsigned)entry.mode,
                           (subtree) ? "tree"
                                     : (entry.mode == 0160000u) ? "commit"
                                                                : "blob",
                           hex);
            (void)d_git_buffer_add_text(_list, line);
            d_internal_add_path(_list, _prefix->text, _prefix->length);
            (void)d_git_buffer_add_char(_list, '\n');
        }
    }

    _prefix->length = stem;
    d_git_object_release(&tree);

    return ( (ok) &&
             (!_prefix->failed) );
}


/*
git_tree(r, name[, deep])
  A tree's entries -- a commit's or tag's tree -- as git ls-tree, or
ls-tree -r, writes them.
*/
static bool
d_internal_git_tree(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host = _user;
    struct d_git_repo* const repo = d_internal_repo(host, _args[0]);
    const bool               deep = ( (_count > 2u) &&
                                      (d_awk_value_is_true(_args[2])) );
    struct d_git_oid         oid;
    struct d_git_object      tree;
    struct d_git_buffer      prefix;
    struct d_git_buffer      list;

    (void)_problem;
    (void)_capacity;

    memset(&tree, 0, sizeof(tree));
    memset(&prefix, 0, sizeof(prefix));
    memset(&list, 0, sizeof(list));

    // a repository, and a tree the revision names
    if ( (!repo) ||
         (!d_internal_rev(host, repo, _args[1], &oid)) ||
         (!d_internal_peel(host, repo, &oid, D_GIT_OBJECT_TREE, &tree)) )
    {
        return d_internal_fail(host, _out, host->error);
    }

    d_git_object_release(&tree);
    (void)d_git_buffer_add_text(&prefix, "");

    const bool listed = d_internal_list_tree(repo, &oid, &prefix, deep,
                                             &list);

    d_git_buffer_free(&prefix);

    // a tree that could not be read
    if (!listed)
    {
        d_git_buffer_free(&list);

        return d_internal_repo_fail(host, _out, repo);
    }

    return d_internal_set_buffer(host, _out, &list);
}


/*
git_hash_object(r, content[, type])
  The name git gives content of a type -- a blob's, without one -- as git
hash-object does, writing nothing.
*/
static bool
d_internal_git_hash_object(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const     host = _user;
    struct d_git_repo* const     repo = d_internal_repo(host, _args[0]);
    const enum d_git_object_type type = (_count > 2u)
                                        ? d_git_object_type_named(
                                              d_internal_text(_args[2]))
                                        : D_GIT_OBJECT_BLOB;
    size_t                       length = 0u;
    struct d_git_oid             oid;

    (void)_problem;
    (void)_capacity;

    // a repository, and a type
    if ( (!repo) ||
         (type == D_GIT_OBJECT_NONE) )
    {
        return d_internal_fail(host, _out, (repo) ? "no such type"
                                                  : host->error);
    }

    const char* const content = d_awk_value_text(_args[1], NULL, &length);

    d_git_repo_hash_object(repo, type, content, length, &oid);

    return d_internal_set_hex(_out, repo, &oid);
}


//==============================================================================
// 12. REFS AND HISTORY
//==============================================================================


/*
git_refs(r[, prefix])
  Every ref whose name begins with a prefix -- all under refs/ without one
-- by full name, sorted.
*/
static bool
d_internal_git_refs(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host   = _user;
    struct d_git_repo* const repo   = d_internal_repo(host, _args[0]);
    char* const              prefix = (_count > 1u)
                                      ? d_internal_copy(_args[1], NULL)
                                      : NULL;
    struct d_git_refs        refs;
    struct d_git_buffer      list;

    (void)_problem;
    (void)_capacity;

    memset(&refs, 0, sizeof(refs));
    memset(&list, 0, sizeof(list));

    // a repository, and its refs
    if ( (!repo) ||
         ( (_count > 1u) && (!prefix) ) ||
         (!d_git_repo_refs(repo, (prefix) ? prefix : "", &refs)) )
    {
        free(prefix);
        d_git_refs_release(&refs);

        return (repo) ? d_internal_repo_fail(host, _out, repo)
                      : d_internal_fail(host, _out, host->error);
    }

    // each, a line
    for (size_t at = 0u; at < refs.count; ++at)
    {
        (void)d_git_buffer_add_text(&list, refs.items[at].name);
        (void)d_git_buffer_add_char(&list, '\n');
    }

    free(prefix);
    d_git_refs_release(&refs);

    return d_internal_set_buffer(host, _out, &list);
}


/*
git_ref(r, ref, field)
  A ref by its full name: oid, what it names; target, the ref a symbolic
one names first, else ""; peeled, what a tag it names peels to, else its
oid.
*/
static bool
d_internal_git_ref(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = { "oid", "target", "peeled" };
    struct d_host_git* const host   = _user;
    struct d_git_repo* const repo   = d_internal_repo(host, _args[0]);
    char* const              name   = (repo) ? d_internal_copy(_args[1], NULL)
                                             : NULL;
    char*                    target = NULL;
    struct d_git_oid         oid;
    char                     peel[(2u * D_GIT_OID_MOST) + 4u];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a repository, and a ref that leads somewhere
    if ( (!name) ||
         (!d_git_repo_resolve(repo, name, &oid, &target)) )
    {
        free(name);

        return (repo) ? d_internal_repo_fail(host, _out, repo)
                      : d_internal_fail(host, _out, host->error);
    }

    const char* const field = d_internal_text(_args[2]);
    bool              set   = false;

    d_git_repo_hex(repo, &oid, peel);
    (void)strcat(peel, "^{}");

    // a table of the fields
    switch (d_internal_field(field, fields, 3u))
    {
        case 0:  set = d_internal_set_hex(_out, repo, &oid);              break;
        case 1:  set = d_internal_set_text(_out, target);                 break;
        case 2:  set = (d_git_repo_rev(repo, peel, &oid))
                       ? d_internal_set_hex(_out, repo, &oid)
                       : d_internal_repo_fail(host, _out, repo);          break;
        default: set = d_internal_unknown_field(host, _out, field);       break;
    }

    free(name);
    free(target);

    return set;
}


/*
git_symbolic(r, ref), git_branch(r)
  The ref a symbolic ref names, "" for one that is not; HEAD's branch
without refs/heads/, "" when HEAD is detached.
*/
static bool
d_internal_symbolic(
    struct d_host_git*  _host,
    struct d_awk_value* _handle,
    struct d_awk_value* _ref,
    struct d_awk_value* _out
)
{
    struct d_git_repo* const repo = d_internal_repo(_host, _handle);
    char* const              name = ( (repo) && (_ref) )
                                    ? d_internal_copy(_ref, NULL)
                                    : NULL;

    // a repository, and the ref's name copied
    if ( (!repo) ||
         ( (_ref) && (!name) ) )
    {
        return d_internal_fail(_host, _out, (repo) ? "out of memory"
                                                   : _host->error);
    }

    char* const       target = d_git_repo_symbolic(repo, (name) ? name
                                                                : "HEAD");
    const bool        branch = ( (!_ref) &&
                                 (target) &&
                                 (strncmp(target, "refs/heads/", 11u) == 0) );
    const char* const shown  = (branch) ? &target[11]
                               : (_ref) ? target : "";
    const bool        set    = d_internal_set_text(_out, shown);

    free(target);
    free(name);

    return set;
}


static bool
d_internal_git_symbolic(
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

    return d_internal_symbolic(_user, _args[0], _args[1], _out);
}


static bool
d_internal_git_branch(
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

    return d_internal_symbolic(_user, _args[0], NULL, _out);
}


/*
d_internal_set_oids
  Gives names in hex, a line each, and lets go of them.
*/
static bool
d_internal_set_oids(
    struct d_host_git*       _host,
    struct d_awk_value*      _out,
    const struct d_git_repo* _repo,
    struct d_git_oids*       _oids
)
{
    struct d_git_buffer list;
    char                hex[(2u * D_GIT_OID_MOST) + 1u];

    memset(&list, 0, sizeof(list));

    // each, a line
    for (size_t at = 0u; at < _oids->count; ++at)
    {
        d_git_repo_hex(_repo, &_oids->items[at], hex);
        (void)d_git_buffer_add_text(&list, hex);
        (void)d_git_buffer_add_char(&list, '\n');
    }

    d_git_oids_release(_oids);

    return d_internal_set_buffer(_host, _out, &list);
}


/*
d_internal_words
  Text split at blanks into words, in place, the words' places kept; false
when memory runs out.
*/
static bool
d_internal_words(
    char*         _text,
    const char*** _out,
    size_t*       _out_count
)
{
    size_t count = 0u;

    *_out       = malloc((strlen(_text) / 2u + 1u) * sizeof(**_out));
    *_out_count = 0u;

    // each word, its end made the text's
    for (char* at = _text; ( (*_out) && (*at != '\0') );)
    {
        // blanks between
        while ( (*at == ' ') || (*at == '\t') || (*at == '\n') )
        {
            *at++ = '\0';
        }

        // a word, to its end
        if (*at != '\0')
        {
            (*_out)[count++] = at;

            // its bytes
            while ( (*at != '\0') &&
                    (*at != ' ') && (*at != '\t') && (*at != '\n') )
            {
                ++at;
            }
        }
    }

    *_out_count = count;

    return (*_out != NULL);
}


/*
git_log(r, revisions[, most[, path[, first_parent]]])
  The commits the revisions reach, as git rev-list gives them.
*/
static bool
d_internal_git_log(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host  = _user;
    struct d_git_repo* const repo  = d_internal_repo(host, _args[0]);
    char* const              text  = (repo) ? d_internal_copy(_args[1], NULL)
                                            : NULL;
    char* const              path  = ( (text) && (_count > 3u) )
                                     ? d_internal_copy(_args[3], NULL)
                                     : NULL;
    const char**             words = NULL;
    struct d_git_log_options options;
    struct d_git_oids        oids;

    (void)_problem;
    (void)_capacity;

    memset(&options, 0, sizeof(options));
    memset(&oids, 0, sizeof(oids));

    // a repository, and the revisions copied and split
    if ( (!text) ||
         ( (_count > 3u) && (!path) ) ||
         (!d_internal_words(text, &words, &options.revision_count)) )
    {
        free(text);
        free(path);

        return d_internal_fail(host, _out, (repo) ? "out of memory"
                                                  : host->error);
    }

    options.revisions    = words;
    options.most         = (_count > 2u) ? d_internal_index(_args[2]) : 0u;
    options.path         = ( (path) && (path[0] != '\0') ) ? path : NULL;
    options.first_parent = ( (_count > 4u) &&
                             (d_awk_value_is_true(_args[4])) );

    const bool walked = d_git_repo_log(repo, &options, &oids);

    free(words);
    free(text);
    free(path);

    // a walk that did not finish
    if (!walked)
    {
        d_git_oids_release(&oids);

        return d_internal_repo_fail(host, _out, repo);
    }

    return d_internal_set_oids(host, _out, repo, &oids);
}


/*
git_merge_base(r, a, b)
  The best common ancestors of two commits, as git merge-base --all gives
them.
*/
static bool
d_internal_git_merge_base(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host = _user;
    struct d_git_repo* const repo = d_internal_repo(host, _args[0]);
    struct d_git_oid         left;
    struct d_git_oid         right;
    struct d_git_oids        oids;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    memset(&oids, 0, sizeof(oids));

    // a repository, and what each revision names
    if ( (!repo) ||
         (!d_internal_rev(host, repo, _args[1], &left)) ||
         (!d_internal_rev(host, repo, _args[2], &right)) )
    {
        return d_internal_fail(host, _out, host->error);
    }

    // their bases
    if (!d_git_repo_merge_base(repo, &left, &right, &oids))
    {
        d_git_oids_release(&oids);

        return d_internal_repo_fail(host, _out, repo);
    }

    return d_internal_set_oids(host, _out, repo, &oids);
}


//==============================================================================
// 13. THE INDEX AND THE WORK TREE
//==============================================================================


/*
git_index(r), git_index_entry(r, i, field)
  The index's entries, by path and stage: path, mode, oid, stage, size,
mtime, ctime, assume_valid, skip_worktree, intent_to_add.
*/
static bool
d_internal_git_index(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host  = _user;
    struct d_git_repo* const repo  = d_internal_repo(host, _args[0]);
    const size_t             count = (repo) ? d_git_repo_index_count(repo)
                                            : D_GIT_REPO_NONE;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a repository, and its index read
    if (count == D_GIT_REPO_NONE)
    {
        return (repo) ? d_internal_repo_fail(host, _out, repo)
                      : d_internal_fail(host, _out, host->error);
    }

    return d_internal_set_number(_out, (double)count);
}


static bool
d_internal_git_index_entry(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    static const char* const fields[] = {
        "path", "mode", "oid", "stage", "size", "mtime", "ctime",
        "assume_valid", "skip_worktree", "intent_to_add" };
    struct d_host_git* const host  = _user;
    struct d_git_repo* const repo  = d_internal_repo(host, _args[0]);
    const size_t             index = d_internal_index(_args[1]);
    const size_t             count = (repo) ? d_git_repo_index_count(repo)
                                            : D_GIT_REPO_NONE;
    char                     mode[16];

    (void)_count;
    (void)_problem;
    (void)_capacity;

    // a repository, its index, and an entry of it
    if ( (count == D_GIT_REPO_NONE) ||
         (index == 0u) ||
         (index > count) )
    {
        return d_internal_fail(host, _out, (!repo) ? host->error
                                           : (count == D_GIT_REPO_NONE)
                                             ? d_git_repo_problem(repo)
                                             : "no such entry");
    }

    const struct d_git_index_entry* const entry =
        d_git_repo_index_entry(repo, index - 1u);
    const char* const                     field = d_internal_text(_args[2]);

    (void)snprintf(mode, sizeof(mode), "%06o", (unsigned)entry->mode);

    // a table of the fields
    switch (d_internal_field(field, fields, 10u))
    {
        case 0:  return d_internal_set_bytes(_out, entry->path,
                                             entry->path_length);
        case 1:  return d_internal_set_text(_out, mode);
        case 2:  return d_internal_set_hex(_out, repo, &entry->oid);
        case 3:  return d_internal_set_number(_out, (double)entry->stage);
        case 4:  return d_internal_set_number(_out, (double)entry->size);
        case 5:  return d_internal_set_number(_out, (double)entry->mtime);
        case 6:  return d_internal_set_number(_out, (double)entry->ctime);
        case 7:  return d_internal_set_flag(_out, entry->assume_valid);
        case 8:  return d_internal_set_flag(_out, entry->skip_worktree);
        case 9:  return d_internal_set_flag(_out, entry->intent_to_add);
        default: return d_internal_unknown_field(host, _out, field);
    }
}


/*
git_index_find(r, path[, stage])
  The index's entry of a path, at a stage -- 0 without one -- from 1; 0
for none.
*/
static bool
d_internal_git_index_find(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host  = _user;
    struct d_git_repo* const repo  = d_internal_repo(host, _args[0]);
    const double             stage = (_count > 2u)
                                     ? d_awk_value_number(_args[2])
                                     : 0.0;
    char* const              path  = (repo) ? d_internal_copy(_args[1], NULL)
                                            : NULL;

    (void)_problem;
    (void)_capacity;

    // a repository, and the path copied
    if (!path)
    {
        return d_internal_fail(host, _out, (repo) ? "out of memory"
                                                  : host->error);
    }

    const size_t found = ( (stage >= 0.0) && (stage <= 3.0) )
                         ? d_git_repo_index_find(repo, path,
                                                 (unsigned)stage)
                         : D_GIT_REPO_NONE;

    free(path);

    return d_internal_set_index(_out, found);
}


/*
d_internal_ignore_of
  What decides whether a path is ignored, by a repository's rules or one
ignore file's: whether it is, and the rule -- file:line:pattern -- into a
buffer. False, git_error() saying why, for no such handle, or a file that
is not an ignore file.
*/
static bool
d_internal_ignore_of(
    struct d_host_git*   _host,
    struct d_awk_value*  _handle,
    const char*          _path,
    bool*                _out_ignored,
    struct d_git_buffer* _out_rule
)
{
    const struct d_internal_slot* const slot = d_internal_slot_of(_host,
                                                                  _handle);
    struct d_git_ignore_match           match;
    char                                line[32];

    *_out_ignored = false;

    // a file that is no ignore file
    if ( (slot) &&
         (slot->doc) &&
         (d_git_doc_kind(slot->doc) != D_GIT_TEXT_IGNORE) )
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "%s is not an ignore file", d_git_doc_path(slot->doc));

        return false;
    }

    // a repository's rules
    if ( (slot) &&
         (slot->repo) &&
         (d_git_repo_ignored(slot->repo, _path, &match)) )
    {
        *_out_ignored = match.ignored;
        (void)snprintf(line, sizeof(line), ":%u:", (unsigned)match.line);
        (void)d_git_buffer_add_text(_out_rule, match.file);
        (void)d_git_buffer_add_text(_out_rule, line);
        (void)d_git_buffer_add(_out_rule, match.pattern,
                               match.pattern_length);
    }

    const size_t pattern = ( (slot) && (slot->doc) )
                           ? d_internal_file_match(slot->doc, _path,
                                                   _out_ignored)
                           : D_GIT_TEXT_NONE;

    // a file's pattern
    if (pattern != D_GIT_TEXT_NONE)
    {
        const struct d_git_text* const         text =
            d_git_doc_records(slot->doc);
        const struct d_git_text_pattern* const found =
            &text->patterns[pattern];

        (void)snprintf(line, sizeof(line), ":%u:",
                       (unsigned)(found->line + 1u));
        (void)d_git_buffer_add_text(_out_rule, d_git_doc_path(slot->doc));
        (void)d_git_buffer_add_text(_out_rule, line);
        (void)d_git_buffer_add(_out_rule, &text->text[found->start],
                               found->end - found->start);
    }

    return (slot != NULL);
}


/*
git_ignored(h, path), git_ignore_rule(h, path)
  1 when a repository's rules, or one ignore file, ignore a path, else 0;
and what decided, as git check-ignore -v writes it, "" for nothing.
*/
static bool
d_internal_ignore(
    struct d_host_git*   _host,
    struct d_awk_value** _args,
    struct d_awk_value*  _out,
    bool                 _rule
)
{
    char* const         path    = d_internal_copy(_args[1], NULL);
    bool                ignored = false;
    struct d_git_buffer rule;

    memset(&rule, 0, sizeof(rule));

    // the path copied, and what decides it
    if ( (!path) ||
         (!d_internal_ignore_of(_host, _args[0], path, &ignored, &rule)) )
    {
        free(path);
        d_git_buffer_free(&rule);

        return d_internal_fail(_host, _out, (path) ? _host->error
                                                   : "out of memory");
    }

    free(path);

    // the rule, or whether it ignores
    if (_rule)
    {
        return d_internal_set_buffer(_host, _out, &rule);
    }

    d_git_buffer_free(&rule);

    return d_internal_set_flag(_out, ignored);
}


static bool
d_internal_git_ignored(
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

    return d_internal_ignore(_user, _args, _out, false);
}


static bool
d_internal_git_ignore_rule(
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

    return d_internal_ignore(_user, _args, _out, true);
}


/*
d_internal_attributes_of
  A path's attributes, by a repository's rules or one attribute file's;
false, git_error() saying why, for no such handle, or a file that is not
an attribute file.
*/
static bool
d_internal_attributes_of(
    struct d_host_git*        _host,
    struct d_awk_value*       _handle,
    const char*               _path,
    struct d_git_text_values* _out
)
{
    const struct d_internal_slot* const slot = d_internal_slot_of(_host,
                                                                  _handle);

    memset(_out, 0, sizeof(*_out));

    // nothing open
    if (!slot)
    {
        return false;
    }

    // a repository's rules
    if (slot->repo)
    {
        const bool found = d_git_repo_attributes(slot->repo, _path, _out);

        // why not
        if (!found)
        {
            (void)snprintf(_host->error, sizeof(_host->error), "%s",
                           d_git_repo_problem(slot->repo));
        }

        return found;
    }

    // a file that is no attribute file
    if (d_git_doc_kind(slot->doc) != D_GIT_TEXT_ATTRIBUTES)
    {
        (void)snprintf(_host->error, sizeof(_host->error),
                       "%s is not an attribute file",
                       d_git_doc_path(slot->doc));

        return false;
    }

    const struct d_git_text_scope scope = d_internal_file_scope(slot->doc);
    const struct d_git_text_query query = d_internal_query_of(_path);

    return d_git_text_attributes(&scope, 1u, &query, _out);
}


/*
d_internal_add_state
  An attribute's state added to text: set, unset, or its value.
*/
static void
d_internal_add_state(
    struct d_git_buffer*           _text,
    const struct d_git_text_value* _value
)
{
    // a value, or a state's name
    if (_value->state == D_GIT_TEXT_VALUE)
    {
        (void)d_git_buffer_add(_text, _value->value, _value->value_length);
    }
    else
    {
        (void)d_git_buffer_add_text(_text, d_internal_state_name(
                                               _value->state));
    }

    return;
}


/*
git_attr(h, path[, name])
  An attribute's state for a path -- set, unset, unspecified, or its value
-- or every attribute's, "name: state" a line, as git check-attr -a.
*/
static bool
d_internal_git_attr(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const host = _user;
    char* const              path = d_internal_copy(_args[1], NULL);
    char* const              name = ( (path) && (_count > 2u) )
                                    ? d_internal_copy(_args[2], NULL)
                                    : NULL;
    struct d_git_text_values values;
    struct d_git_buffer      text;

    (void)_problem;
    (void)_capacity;

    memset(&values, 0, sizeof(values));
    memset(&text, 0, sizeof(text));

    // the arguments copied, and the path's attributes
    if ( (!path) ||
         ( (_count > 2u) && (!name) ) ||
         (!d_internal_attributes_of(host, _args[0], path, &values)) )
    {
        free(path);
        free(name);
        d_git_text_values_release(&values);

        return d_internal_fail(host, _out, (path) ? host->error
                                                  : "out of memory");
    }

    const struct d_git_text_value* const one =
        (name) ? d_git_text_value_of(&values, name) : NULL;

    // one, by name -- unspecified when none says it
    if (one)
    {
        d_internal_add_state(&text, one);
    }
    else if (name)
    {
        (void)d_git_buffer_add_text(&text, "unspecified");
    }

    // else each, a line
    for (size_t at = 0u; ( (!name) && (at < values.count) ); ++at)
    {
        (void)d_git_buffer_add(&text, values.items[at].name,
                               values.items[at].name_length);
        (void)d_git_buffer_add_text(&text, ": ");
        d_internal_add_state(&text, &values.items[at]);
        (void)d_git_buffer_add_char(&text, '\n');
    }

    free(path);
    free(name);
    d_git_text_values_release(&values);

    return d_internal_set_buffer(host, _out, &text);
}


/*
d_internal_status_options
  Status's options from their words, as git status takes them: -uno,
-unormal, -uall, --untracked-files=..., --ignored; false for another.
*/
static bool
d_internal_status_options(
    char*                        _text,
    struct d_git_status_options* _out
)
{
    static const char* const untracked[] = { "normal", "no", "all" };
    const char**             words       = NULL;
    size_t                   count       = 0u;
    bool                     ok          = d_internal_words(_text, &words,
                                                            &count);

    memset(_out, 0, sizeof(*_out));
    _out->untracked = D_GIT_UNTRACKED_NORMAL;

    // each word, while each is one status takes
    for (size_t at = 0u; ( (ok) && (at < count) ); ++at)
    {
        const char* const word = words[at];
        const char* const mode = (strncmp(word, "-u", 2u) == 0) ? &word[2]
            : (strncmp(word, "--untracked-files=", 18u) == 0) ? &word[18]
            : NULL;
        const size_t      kind = (mode) ? d_internal_field(mode, untracked,
                                                           3u)
                                        : 3u;

        _out->ignored   = ( (_out->ignored) ||
                            (strcmp(word, "--ignored") == 0) );
        _out->untracked = (kind < 3u) ? (enum d_git_untracked)kind
                                      : _out->untracked;
        ok              = ( (kind < 3u) ||
                            (strcmp(word, "--ignored") == 0) );
    }

    free(words);

    return ok;
}


/*
git_status(r[, options])
  The work tree's status, as git status --porcelain --no-renames gives
it: "XY path" a line.
*/
static bool
d_internal_git_status(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_git* const    host = _user;
    struct d_git_repo* const    repo = d_internal_repo(host, _args[0]);
    char* const                 text = (_count > 1u)
                                       ? d_internal_copy(_args[1], NULL)
                                       : d_internal_dup("");
    struct d_git_status_options options;
    struct d_git_status         status;
    struct d_git_buffer         list;

    (void)_problem;
    (void)_capacity;

    memset(&status, 0, sizeof(status));
    memset(&list, 0, sizeof(list));

    // a repository, and options status takes
    if ( (!repo) ||
         (!text) ||
         (!d_internal_status_options(text, &options)) )
    {
        free(text);

        return d_internal_fail(host, _out, (!repo) ? host->error
                                           : (text) ? "no such option"
                                                    : "out of memory");
    }

    free(text);

    // the status read
    if (!d_git_repo_status(repo, &options, &status))
    {
        d_git_status_release(&status);

        return d_internal_repo_fail(host, _out, repo);
    }

    // each, a line
    for (size_t at = 0u; at < status.count; ++at)
    {
        (void)d_git_buffer_add_char(&list, status.items[at].index);
        (void)d_git_buffer_add_char(&list, status.items[at].work);
        (void)d_git_buffer_add_char(&list, ' ');
        d_internal_add_path(&list, status.items[at].path,
                            strlen(status.items[at].path));
        (void)d_git_buffer_add_char(&list, '\n');
    }

    d_git_status_release(&status);

    return d_internal_set_buffer(host, _out, &list);
}


//==============================================================================
// 14. THE EXTENSION
//==============================================================================


/*
d_internal_functions
  The extension's host functions, as a program calls them, and how many.
  Long as the table of the extension's functions is.
*/
static const struct d_awk_host_function*
d_internal_functions(
    size_t* _out_count
)
{
    static const struct d_awk_host_function functions[] =
    {
        { "git_open",             1u, 2u, d_internal_git_open             },
        { "git_new",              1u, 3u, d_internal_git_new              },
        { "git_save",             1u, 2u, d_internal_git_save             },
        { "git_close",            1u, 1u, d_internal_git_close            },
        { "git_error",            0u, 0u, d_internal_git_error            },
        { "git_path",             1u, 1u, d_internal_git_path             },
        { "git_kind",             1u, 1u, d_internal_git_kind             },
        { "git_text",             1u, 1u, d_internal_git_text             },
        { "git_changes",          1u, 1u, d_internal_git_changes          },
        { "git_kind_of",          1u, 1u, d_internal_git_kind_of          },
        { "git_problems",         1u, 1u, d_internal_git_problems         },
        { "git_problem",          3u, 3u, d_internal_git_problem          },
        { "git_lines",            1u, 1u, d_internal_git_lines            },
        { "git_line",             3u, 3u, d_internal_git_line             },
        { "git_insert_line",      3u, 3u, d_internal_git_insert_line      },
        { "git_add_line",         2u, 2u, d_internal_git_add_line         },
        { "git_set_line",         3u, 3u, d_internal_git_set_line         },
        { "git_remove_line",      2u, 2u, d_internal_git_remove_line      },
        { "git_patterns",         1u, 1u, d_internal_git_patterns         },
        { "git_pattern",          3u, 3u, d_internal_git_pattern          },
        { "git_state",            4u, 4u, d_internal_git_state            },
        { "git_set_state",        3u, 3u, d_internal_git_set_state        },
        { "git_remove_state",     3u, 3u, d_internal_git_remove_state     },
        { "git_match",            2u, 2u, d_internal_git_match            },
        { "git_sections",         1u, 1u, d_internal_git_sections         },
        { "git_section",          3u, 3u, d_internal_git_section          },
        { "git_entries",          1u, 1u, d_internal_git_entries          },
        { "git_entry",            3u, 3u, d_internal_git_entry            },
        { "git_find_entry",       2u, 3u, d_internal_git_find_entry       },
        { "git_find_section",     2u, 3u, d_internal_git_find_section     },
        { "git_set",              3u, 3u, d_internal_git_set              },
        { "git_add",              3u, 3u, d_internal_git_add              },
        { "git_unset",            2u, 3u, d_internal_git_unset            },
        { "git_set_entry",        2u, 3u, d_internal_git_set_entry        },
        { "git_remove_entry",     2u, 2u, d_internal_git_remove_entry     },
        { "git_rename_section",   3u, 3u, d_internal_git_rename_section   },
        { "git_remove_section",   2u, 2u, d_internal_git_remove_section   },
        { "git_config",           2u, 2u, d_internal_git_config           },
        { "git_config_all",       2u, 2u, d_internal_git_config_all       },
        { "git_config_count",     2u, 2u, d_internal_git_config_count     },
        { "git_config_bool",      2u, 3u, d_internal_git_config_bool      },
        { "git_config_int",       2u, 3u, d_internal_git_config_int       },
        { "git_wildmatch",        2u, 3u, d_internal_git_wildmatch        },
        { "git_key",              1u, 1u, d_internal_git_key              },
        { "git_quote_value",      1u, 1u, d_internal_git_quote_value      },
        { "git_quote_subsection", 1u, 1u, d_internal_git_quote_subsection },
        { "git_quote_path",       1u, 1u, d_internal_git_quote_path       },
        { "git_unquote",          1u, 1u, d_internal_git_unquote          },
        { "git_repo",             0u, 1u, d_internal_git_repo             },
        { "git_dir",              1u, 1u, d_internal_git_dir              },
        { "git_common_dir",       1u, 1u, d_internal_git_common_dir       },
        { "git_work_tree",        1u, 1u, d_internal_git_work_tree        },
        { "git_hash",             1u, 1u, d_internal_git_hash             },
        { "git_config_file",      1u, 2u, d_internal_git_config_file      },
        { "git_settings",         1u, 1u, d_internal_git_settings         },
        { "git_setting",          3u, 3u, d_internal_git_setting          },
        { "git_rev",              2u, 2u, d_internal_git_rev              },
        { "git_short",            2u, 3u, d_internal_git_short            },
        { "git_exists",           2u, 2u, d_internal_git_exists           },
        { "git_type",             2u, 2u, d_internal_git_type             },
        { "git_size",             2u, 2u, d_internal_git_size             },
        { "git_cat",              2u, 2u, d_internal_git_cat              },
        { "git_commit",           3u, 3u, d_internal_git_commit           },
        { "git_tag",              3u, 3u, d_internal_git_tag              },
        { "git_tree",             2u, 3u, d_internal_git_tree             },
        { "git_hash_object",      2u, 3u, d_internal_git_hash_object      },
        { "git_refs",             1u, 2u, d_internal_git_refs             },
        { "git_ref",              3u, 3u, d_internal_git_ref              },
        { "git_symbolic",         2u, 2u, d_internal_git_symbolic         },
        { "git_branch",           1u, 1u, d_internal_git_branch           },
        { "git_log",              2u, 5u, d_internal_git_log              },
        { "git_merge_base",       3u, 3u, d_internal_git_merge_base       },
        { "git_index",            1u, 1u, d_internal_git_index            },
        { "git_index_entry",      3u, 3u, d_internal_git_index_entry      },
        { "git_index_find",       2u, 3u, d_internal_git_index_find       },
        { "git_ignored",          2u, 2u, d_internal_git_ignored          },
        { "git_ignore_rule",      2u, 2u, d_internal_git_ignore_rule      },
        { "git_attr",             2u, 3u, d_internal_git_attr             },
        { "git_status",           1u, 2u, d_internal_git_status           }
    };

    *_out_count = sizeof(functions) / sizeof(functions[0]);

    return functions;
}


struct d_host_git*
d_host_git_new(void)
{
    return calloc(1u, sizeof(struct d_host_git));
}


void
d_host_git_free(
    struct d_host_git* _host
)
{
    // nothing to free
    if (!_host)
    {
        return;
    }

    // each file and repository still open
    for (size_t at = 0u; at < _host->slot_count; ++at)
    {
        d_git_doc_free(_host->slots[at].doc);
        d_git_repo_free(_host->slots[at].repo);
    }

    free(_host->slots);
    free(_host);

    return;
}


bool
d_host_git_install(
    struct d_host_git*   _host,
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
