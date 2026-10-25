/*******************************************************************************
* djinterp [tools]                                                  dninja_doc.c
*
* Definitions for the declarations in dninja_doc.h.
*   Every change is a candidate first: the new text, read into records of
* its own. The statements are in the text's order and never overlap, so
* those the splice leaves whole before it are a prefix of the old and of the
* new, those it leaves whole after it a suffix of each, and the few it cuts
* lie between: those before must be the file's own, and those after the
* file's own moved by what the splice added. A change of a kind then checks
* the statements between for what it was to do; only then are the file's
* text and records the candidate's. Comments are compared as statements
* are.
*   A change that adds to a statement splices within it -- before its last
* line ending, not past it -- so that the statement is one the splice cuts.
*   Files are read and written with dsplice's helpers, the directories a
* file goes in made with dfs's.
*
* path:      /src/djinterp/tools/dawk/ext/ninja/dninja_doc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/ninja/dninja_doc.h"  // corresponding header
// std
#include <stddef.h>  // ptrdiff_t
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, calloc, realloc, free
#include <string.h>  // memcmp, memcpy, memset, strlen
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"      // d_fs_make_parent
#include "../../../../../../inc/djinterp/tools/dawk/dsplice.h"  // d_text_read,
                                                                // d_text_write


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Documents
//------------------------------------------------------------------------------
// 1.1.1
// d_ninja_doc
//   struct: the file: its path, text and records, the changes it has taken,
//   where the last was, and why the last failed.
struct d_ninja_doc
{
    char*          path;
    char*          text;
    size_t         length;
    struct d_ninja records;
    size_t         changes;
    size_t         start;
    size_t         old_end;
    size_t         new_end;
    char           problem[512];
};

// 1.1.2
// d_internal_candidate
//   struct: a change not yet kept: the new text and its records, where the
//   splice stands in the old text and the new, and how the statements fall
//   about it -- `before` whole before it in both, the old's from
//   `old_after` and the new's from `new_after` whole after it.
struct d_internal_candidate
{
    char*          text;
    size_t         length;
    struct d_ninja records;
    size_t         start;
    size_t         old_end;
    size_t         new_end;
    size_t         before;
    size_t         old_after;
    size_t         new_after;
    ptrdiff_t      shift;
    ptrdiff_t      lines;
};

// 1.1.3
// d_internal_buffer
//   struct: text being written, in an array that grows.
struct d_internal_buffer
{
    char*  text;
    size_t length;
    size_t capacity;
    bool   failed;
};

// 1.1.4
// d_internal_indices
//   struct: indices, borrowed.
struct d_internal_indices
{
    const size_t* items;
    size_t        count;
};

// 1.1.5
// d_internal_expect
//   struct: what a change to one statement is to leave of it: a binding
//   changed or added, or some gone, a path changed, added or gone, or a
//   value or a name changed; D_NINJA_NONE for none of a kind.
struct d_internal_expect
{
    const char*               text;
    size_t                    binding;
    bool                      binding_added;
    const char*               binding_name;
    struct d_internal_indices removed_bindings;
    size_t                    path;
    bool                      path_added;
    bool                      path_removed;
    enum d_ninja_section      section;
    bool                      value;
    const char*               name;
};


//==============================================================================
// 2.  BUFFERS
//==============================================================================


/*
d_internal_add, d_internal_add_char
  Appends to a buffer, which remembers a failure and takes nothing more.
*/
static bool
d_internal_add(
    struct d_internal_buffer* _buffer,
    const char*               _text,
    size_t                    _length
)
{
    // a buffer that failed takes nothing more
    if (_buffer->failed)
    {
        return false;
    }

    // room for the text and a terminator
    if ((_buffer->length + _length + 1u) > _buffer->capacity)
    {
        size_t wanted = (_buffer->capacity > 0u) ? _buffer->capacity : 64u;

        // doubled until it holds the text
        while (wanted < (_buffer->length + _length + 1u))
        {
            wanted *= 2u;
        }

        char* const grown = realloc(_buffer->text, wanted);

        // memory that ran out fails the buffer
        if (!grown)
        {
            _buffer->failed = true;

            return false;
        }

        _buffer->text     = grown;
        _buffer->capacity = wanted;
    }

    // the text, when there is any
    if (_length > 0u)
    {
        memcpy(&_buffer->text[_buffer->length], _text, _length);
    }

    _buffer->length                += _length;
    _buffer->text[_buffer->length]  = '\0';

    return true;
}


static bool
d_internal_add_char(
    struct d_internal_buffer* _buffer,
    char                      _c
)
{
    return d_internal_add(_buffer, &_c, 1u);
}


static void
d_internal_buffer_free(
    struct d_internal_buffer* _buffer
)
{
    free(_buffer->text);
    memset(_buffer, 0, sizeof(*_buffer));

    return;
}


/*
d_internal_copy
  A string of its own.
*/
static char*
d_internal_copy(
    const char* _text,
    size_t      _length
)
{
    char* const copy = malloc(_length + 1u);

    // a copy that cannot be made is NULL
    if (copy)
    {
        memcpy(copy, _text, _length);
        copy[_length] = '\0';
    }

    return copy;
}


/*
d_internal_same_text
  Whether two spans hold the same bytes.
*/
static bool
d_internal_same_text(
    const char* _left,
    size_t      _left_length,
    const char* _right,
    size_t      _right_length
)
{
    return ( (_left_length == _right_length) &&
             ( (_left_length == 0u) ||
               (memcmp(_left, _right, _left_length) == 0) ) );
}


//==============================================================================
// 3.  DOCUMENTS
//==============================================================================


/*
d_internal_new
  A document of a text it takes, read; NULL, the text freed, when it does
not read as Ninja.
*/
static struct d_ninja_doc*
d_internal_new(
    const char* _path,
    char*       _text,
    size_t      _length,
    char*       _problem,
    size_t      _capacity
)
{
    struct d_ninja_doc* const doc  = calloc(1u, sizeof(*doc));
    char* const               path = d_internal_copy(_path, strlen(_path));
    char                      why[256];

    // a document needs room of its own
    if ( (!doc) ||
         (!path) )
    {
        free(doc);
        free(path);
        free(_text);
        (void)snprintf(_problem, _capacity, "out of memory");

        return NULL;
    }

    doc->path   = path;
    doc->text   = _text;
    doc->length = _length;

    // a text ninja would refuse is no document
    if (!d_ninja_read(&doc->records, doc->text, doc->length, why,
                      sizeof(why)))
    {
        (void)snprintf(_problem, _capacity, "%s: %s", path, why);
        d_ninja_doc_free(doc);

        return NULL;
    }

    return doc;
}


struct d_ninja_doc*
d_ninja_doc_read(
    const char* _path,
    char*       _problem,
    size_t      _capacity
)
{
    size_t length = 0;

    // parameter validation first
    if (!_path)
    {
        (void)snprintf(_problem, _capacity, "no file is named");

        return NULL;
    }

    char* const text = d_text_read(_path, &length);

    // a file that cannot be read has no document
    if (!text)
    {
        (void)snprintf(_problem, _capacity, "cannot read: %s", _path);

        return NULL;
    }

    return d_internal_new(_path, text, length, _problem, _capacity);
}


struct d_ninja_doc*
d_ninja_doc_make(
    const char* _path,
    const char* _text,
    size_t      _length,
    char*       _problem,
    size_t      _capacity
)
{
    const char* const text = (_text) ? _text : "";
    const size_t      span = (_text) ? _length : 0u;
    char* const       copy = d_internal_copy(text, span);

    // a text needs room of its own
    if (!copy)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return NULL;
    }

    return d_internal_new((_path) ? _path : "", copy, span, _problem,
                          _capacity);
}


void
d_ninja_doc_free(
    struct d_ninja_doc* _doc
)
{
    // freeing nothing is allowed
    if (!_doc)
    {
        return;
    }

    d_ninja_release(&_doc->records);
    free(_doc->text);
    free(_doc->path);
    free(_doc);

    return;
}


const struct d_ninja*
d_ninja_doc_records(
    const struct d_ninja_doc* _doc
)
{
    return (_doc) ? &_doc->records : NULL;
}


const char*
d_ninja_doc_path(
    const struct d_ninja_doc* _doc
)
{
    return (_doc) ? _doc->path : "";
}


size_t
d_ninja_doc_changes(
    const struct d_ninja_doc* _doc
)
{
    return (_doc) ? _doc->changes : 0u;
}


bool
d_ninja_doc_changed(
    const struct d_ninja_doc* _doc,
    size_t*                   _out_start,
    size_t*                   _out_old_end,
    size_t*                   _out_new_end
)
{
    // a change, to say where it was
    if ( (!_doc) ||
         (_doc->changes == 0u) )
    {
        return false;
    }

    // where it began
    if (_out_start)
    {
        *_out_start = _doc->start;
    }

    // where it ended in the text before
    if (_out_old_end)
    {
        *_out_old_end = _doc->old_end;
    }

    // and in the text after
    if (_out_new_end)
    {
        *_out_new_end = _doc->new_end;
    }

    return true;
}


const char*
d_ninja_doc_problem(
    const struct d_ninja_doc* _doc
)
{
    return (_doc) ? _doc->problem : "";
}


const char*
d_ninja_doc_text(
    const struct d_ninja_doc* _doc,
    size_t*                   _out_length
)
{
    // the length, when it is asked for
    if (_out_length)
    {
        *_out_length = (_doc) ? _doc->length : 0u;
    }

    return (_doc) ? _doc->text : "";
}


/*
d_internal_fail
  Says why a change failed, and gives false.
*/
static bool
d_internal_fail(
    struct d_ninja_doc* _doc,
    const char*         _why
)
{
    (void)snprintf(_doc->problem, sizeof(_doc->problem), "%s", _why);

    return false;
}


//==============================================================================
// 4.  COMPARING RECORDS
//==============================================================================


/*
d_internal_moved
  A place, or a line, of the old text as it is in the new: moved when it is
after the splice.
*/
static size_t
d_internal_moved(
    size_t    _at,
    ptrdiff_t _shift
)
{
    return (size_t)((ptrdiff_t)_at + _shift);
}


/*
d_internal_same_value
  Whether a value of the old records is one of the new, moved.
*/
static bool
d_internal_same_value(
    const struct d_ninja* _old,
    size_t                _left,
    const struct d_ninja* _new,
    size_t                _right,
    ptrdiff_t             _shift
)
{
    // none is none
    if ( (_left == D_NINJA_NONE) ||
         (_right == D_NINJA_NONE) )
    {
        return (_left == _right);
    }

    const struct d_ninja_value* const a = &_old->values[_left];
    const struct d_ninja_value* const b = &_new->values[_right];

    return ( (d_internal_moved(a->start, _shift) == b->start) &&
             (d_internal_moved(a->end, _shift) == b->end) &&
             (a->piece_count == b->piece_count) );
}


/*
d_internal_same_statement
  Whether a statement of the old records is one of the new, moved: where it
is, its name, its paths in their sections, and its bindings.
  Long as each part of a statement is compared in turn.
*/
static bool
d_internal_same_statement(
    const struct d_ninja_doc*          _doc,
    const struct d_internal_candidate* _candidate,
    size_t                             _left,
    size_t                             _right,
    bool                               _after
)
{
    const struct d_ninja* const           old   = &_doc->records;
    const struct d_ninja* const           fresh = &_candidate->records;
    const struct d_ninja_statement* const a     = &old->statements[_left];
    const struct d_ninja_statement* const b     = &fresh->statements[_right];
    const ptrdiff_t                       shift = (_after) ? _candidate->shift
                                                           : 0;
    const ptrdiff_t                       lines = (_after) ? _candidate->lines
                                                           : 0;

    // the statement itself
    if ( (a->kind != b->kind) ||
         (d_internal_moved(a->start, shift) != b->start) ||
         (d_internal_moved(a->end, shift) != b->end) ||
         (d_internal_moved(a->header_end, shift) != b->header_end) ||
         (d_internal_moved(a->line, lines) != b->line) ||
         (d_internal_moved(a->end_line, lines) != b->end_line) ||
         (d_internal_moved(a->name_start, shift) != b->name_start) ||
         (d_internal_moved(a->name_end, shift) != b->name_end) ||
         (!d_internal_same_value(old, a->value, fresh, b->value, shift)) ||
         (a->path_count != b->path_count) ||
         (a->binding_count != b->binding_count) )
    {
        return false;
    }

    // each path, in its section
    for (size_t at = 0; at < a->path_count; ++at)
    {
        const struct d_ninja_path* const x = &old->paths[a->first_path + at];
        const struct d_ninja_path* const y =
            &fresh->paths[b->first_path + at];

        // the same section, the same value
        if ( (x->section != y->section) ||
             (!d_internal_same_value(old, x->value, fresh, y->value, shift)) )
        {
            return false;
        }
    }

    // each binding
    for (size_t at = 0; at < a->binding_count; ++at)
    {
        const struct d_ninja_binding* const x =
            &old->bindings[a->first_binding + at];
        const struct d_ninja_binding* const y =
            &fresh->bindings[b->first_binding + at];

        // the same line, name and value
        if ( (d_internal_moved(x->start, shift) != y->start) ||
             (d_internal_moved(x->end, shift) != y->end) ||
             (d_internal_moved(x->name_start, shift) != y->name_start) ||
             (d_internal_moved(x->name_end, shift) != y->name_end) ||
             (!d_internal_same_value(old, x->value, fresh, y->value, shift)) )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_mapped_within
  Whether a comment's statement in the old records is its statement in the
new: before the splice the same, after it moved, between it any between.
*/
static bool
d_internal_mapped_within(
    const struct d_internal_candidate* _candidate,
    size_t                             _old,
    size_t                             _new
)
{
    // a table of where the statement stood: nowhere, before, after, between
    if (_old == D_NINJA_NONE)
    {
        return (_new == D_NINJA_NONE);
    }
    else if (_old < _candidate->before)
    {
        return (_new == _old);
    }
    else if (_old >= _candidate->old_after)
    {
        return (_new == (_old - _candidate->old_after +
                         _candidate->new_after));
    }

    return ( (_new != D_NINJA_NONE) &&
             (_new >= _candidate->before) &&
             (_new < _candidate->new_after) );
}


/*
d_internal_place_statements
  How the statements of both texts fall about the splice: those wholly
before it in the old, which the new must hold as they were -- a statement
the splice cut may end where it starts now, and is not among them -- and
those wholly after it in each; false when the new has too few, or as many
do not lie after it in both.
*/
static bool
d_internal_place_statements(
    const struct d_ninja_doc*    _doc,
    struct d_internal_candidate* _candidate
)
{
    const struct d_ninja* const old   = &_doc->records;
    const struct d_ninja* const fresh = &_candidate->records;
    size_t                      left  = 0;

    // those wholly before the splice, in the old
    while ( (left < old->statement_count) &&
            (old->statements[left].end <= _candidate->start) )
    {
        left++;
    }

    size_t     right   = left;
    const bool as_many = (left <= fresh->statement_count);

    _candidate->before = left;

    // those wholly after it, in the old
    while ( (left < old->statement_count) &&
            (old->statements[left].start < _candidate->old_end) )
    {
        left++;
    }

    // ...and in the new
    while ( (right < fresh->statement_count) &&
            (fresh->statements[right].start < _candidate->new_end) )
    {
        right++;
    }

    _candidate->old_after = left;
    _candidate->new_after = right;

    return ( (as_many) &&
             ( (old->statement_count - left) ==
               (fresh->statement_count - right) ) );
}


/*
d_internal_outside_statements
  Whether every statement outside the splice is the file's own; gives the
line of the first that is not.
*/
static bool
d_internal_outside_statements(
    const struct d_ninja_doc*          _doc,
    const struct d_internal_candidate* _candidate,
    size_t*                            _out_line
)
{
    const size_t after = _doc->records.statement_count -
                         _candidate->old_after;

    // those before
    for (size_t at = 0; at < _candidate->before; ++at)
    {
        // the same, where it was
        if (!d_internal_same_statement(_doc, _candidate, at, at, false))
        {
            *_out_line = _doc->records.statements[at].line;

            return false;
        }
    }

    // those after
    for (size_t at = 0; at < after; ++at)
    {
        // the same, moved
        if (!d_internal_same_statement(_doc, _candidate,
                                       _candidate->old_after + at,
                                       _candidate->new_after + at, true))
        {
            *_out_line =
                _doc->records.statements[_candidate->old_after + at].line;

            return false;
        }
    }

    return true;
}


/*
d_internal_outside_comments
  Whether every comment outside the splice is the file's own: those before
it the same, those after it moved, as many of each.
  Long as the two lists are walked side by side through the three places,
each a loop.
*/
static bool
d_internal_outside_comments(
    const struct d_ninja_doc*          _doc,
    const struct d_internal_candidate* _candidate,
    size_t*                            _out_line
)
{
    const struct d_ninja* const old   = &_doc->records;
    const struct d_ninja* const fresh = &_candidate->records;
    size_t                      left  = 0;
    size_t                      old_after = 0;
    size_t                      new_after = 0;

    // those before, in the old, which the new must hold as they were
    while ( (left < old->comment_count) &&
            (old->comments[left].end <= _candidate->start) )
    {
        left++;
    }

    old_after = left;
    new_after = left;

    // past those cut, in the old
    while ( (old_after < old->comment_count) &&
            (old->comments[old_after].start < _candidate->old_end) )
    {
        old_after++;
    }

    // ...and in the new
    while ( (new_after < fresh->comment_count) &&
            (fresh->comments[new_after].start < _candidate->new_end) )
    {
        new_after++;
    }

    // as many before, and as many after
    if ( (left > fresh->comment_count) ||
         ((old->comment_count - old_after) !=
          (fresh->comment_count - new_after)) )
    {
        *_out_line = (left < old->comment_count) ? old->comments[left].line
                                                 : old->line_count;

        return false;
    }

    // each before, then each after, the same
    for (size_t at = 0; at < old->comment_count; ++at)
    {
        const bool                          after = (at >= old_after);
        const size_t                        other = (after)
                                                        ? (at - old_after +
                                                           new_after)
                                                        : at;
        const struct d_ninja_comment* const a     = &old->comments[at];
        const ptrdiff_t                     shift = (after) ? _candidate->shift
                                                            : 0;
        const ptrdiff_t                     lines = (after) ? _candidate->lines
                                                            : 0;

        // one the splice cut is the change's
        if ( (at >= left) &&
             (at < old_after) )
        {
            continue;
        }

        const struct d_ninja_comment* const b = &fresh->comments[other];

        // the same place and line, and its statement
        if ( (d_internal_moved(a->start, shift) != b->start) ||
             (d_internal_moved(a->end, shift) != b->end) ||
             (d_internal_moved(a->line, lines) != b->line) ||
             (!d_internal_mapped_within(_candidate, a->within, b->within)) )
        {
            *_out_line = a->line;

            return false;
        }
    }

    return true;
}


//==============================================================================
// 5.  CHANGES
//==============================================================================


/*
d_internal_with_endings
  Text with each lone line feed written as the file ends its lines.
*/
static bool
d_internal_with_endings(
    const struct d_ninja_doc* _doc,
    const char*               _text,
    size_t                    _length,
    struct d_internal_buffer* _out
)
{
    // each byte, a CR before a lone line feed in a CR LF file
    for (size_t at = 0; at < _length; ++at)
    {
        // a lone line feed, in a CR LF file, gets its CR
        if ( (_doc->records.crlf) &&
             (_text[at] == '\n')  &&
             ( (at == 0u) || (_text[at - 1u] != '\r') ) )
        {
            (void)d_internal_add_char(_out, '\r');
        }

        (void)d_internal_add_char(_out, _text[at]);
    }

    (void)d_internal_add(_out, "", 0u);

    return !_out->failed;
}


/*
d_internal_count_lines
  How many line feeds a span holds.
*/
static ptrdiff_t
d_internal_count_lines(
    const char* _text,
    size_t      _length
)
{
    ptrdiff_t count = 0;

    // each line feed
    for (size_t at = 0; at < _length; ++at)
    {
        count += (_text[at] == '\n') ? 1 : 0;
    }

    return count;
}


static void
d_internal_discard(
    struct d_internal_candidate* _candidate
)
{
    d_ninja_release(&_candidate->records);
    free(_candidate->text);
    memset(_candidate, 0, sizeof(*_candidate));

    return;
}


/*
d_internal_build
  The candidate's text: the file before the splice, the text written with
the file's endings, and the file after.
*/
static bool
d_internal_build(
    struct d_ninja_doc*          _doc,
    size_t                       _start,
    size_t                       _end,
    const char*                  _text,
    size_t                       _length,
    struct d_internal_candidate* _out
)
{
    struct d_internal_buffer insert = { NULL, 0u, 0u, false };

    // the text, its line endings the file's
    if (!d_internal_with_endings(_doc, _text, _length, &insert))
    {
        d_internal_buffer_free(&insert);

        return d_internal_fail(_doc, "out of memory");
    }

    _out->length  = _start + insert.length + (_doc->length - _end);
    _out->text    = malloc(_out->length + 1u);
    _out->start   = _start;
    _out->old_end = _end;
    _out->new_end = _start + insert.length;
    _out->shift   = (ptrdiff_t)_out->new_end - (ptrdiff_t)_end;
    _out->lines   = d_internal_count_lines(insert.text, insert.length) -
                    d_internal_count_lines(&_doc->text[_start],
                                           _end - _start);

    // memory that ran out
    if (!_out->text)
    {
        d_internal_buffer_free(&insert);

        return d_internal_fail(_doc, "out of memory");
    }

    memcpy(_out->text, _doc->text, _start);

    // the new text, when there is any
    if (insert.length > 0u)
    {
        memcpy(&_out->text[_start], insert.text, insert.length);
    }

    memcpy(&_out->text[_out->new_end], &_doc->text[_end],
           _doc->length - _end);
    _out->text[_out->length] = '\0';
    d_internal_buffer_free(&insert);

    return true;
}


/*
d_internal_try
  Makes a candidate of a splice, and keeps it only if the file outside it
reads as it did.
*/
static bool
d_internal_try(
    struct d_ninja_doc*          _doc,
    size_t                       _start,
    size_t                       _end,
    const char*                  _text,
    size_t                       _length,
    struct d_internal_candidate* _out
)
{
    char   why[256];
    size_t line = 0;

    memset(_out, 0, sizeof(*_out));

    // the splice within the text
    if ( (_start > _end) ||
         (_end > _doc->length) )
    {
        return d_internal_fail(_doc, "the change is not within the file");
    }

    // the new text
    if (!d_internal_build(_doc, _start, _end, _text, _length, _out))
    {
        return false;
    }

    // the new text read, or the change refused
    if (!d_ninja_read(&_out->records, _out->text, _out->length, why,
                      sizeof(why)))
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the change would leave the file unreadable: %s",
                       why);
        d_internal_discard(_out);

        return false;
    }

    // the statements outside the splice as they were
    if ( (!d_internal_place_statements(_doc, _out)) ||
         (!d_internal_outside_statements(_doc, _out, &line)) )
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the change would change the statement at line %zu, "
                       "outside it",
                       (line > 0u) ? line : _doc->records.line_count);
        d_internal_discard(_out);

        return false;
    }

    // and the comments
    if (!d_internal_outside_comments(_doc, _out, &line))
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the change would change the comment at line %zu, "
                       "outside it", line);
        d_internal_discard(_out);

        return false;
    }

    return true;
}


/*
d_internal_commit
  Keeps a candidate: the file is its text and records now.
*/
static bool
d_internal_commit(
    struct d_ninja_doc*          _doc,
    struct d_internal_candidate* _candidate
)
{
    d_ninja_release(&_doc->records);
    free(_doc->text);
    _doc->text       = _candidate->text;
    _doc->length     = _candidate->length;
    _doc->records    = _candidate->records;
    _doc->start      = _candidate->start;
    _doc->old_end    = _candidate->old_end;
    _doc->new_end    = _candidate->new_end;
    _doc->changes   += 1u;
    _doc->problem[0] = '\0';
    memset(_candidate, 0, sizeof(*_candidate));

    return true;
}


bool
d_ninja_doc_splice(
    struct d_ninja_doc* _doc,
    size_t              _start,
    size_t              _end,
    const char*         _text,
    size_t              _length
)
{
    struct d_internal_candidate candidate;

    // parameter validation first
    if ( (!_doc) ||
         ( (!_text) && (_length > 0u) ) )
    {
        return false;
    }

    // the splice tried, and refused if anything outside it changes
    if (!d_internal_try(_doc, _start, _end, (_text) ? _text : "", _length,
                        &candidate))
    {
        return false;
    }

    return d_internal_commit(_doc, &candidate);
}


//==============================================================================
// 6.  READING BACK
//==============================================================================


/*
d_internal_unparsed
  A value of a reading, unparsed, for the caller to free; NULL when memory
runs out.
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
        return d_internal_copy("", 0u);
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
d_internal_same_unparsed
  Whether two values, each of its own reading, unparse alike.
*/
static bool
d_internal_same_unparsed(
    const struct d_ninja* _left,
    size_t                _a,
    const struct d_ninja* _right,
    size_t                _b
)
{
    char* const one   = d_internal_unparsed(_left, _a);
    char* const other = d_internal_unparsed(_right, _b);
    const bool  same  = ( (one) &&
                          (other) &&
                          (strcmp(one, other) == 0) );

    free(one);
    free(other);

    return same;
}


/*
d_internal_raw_is
  Whether a value of a reading is written as a text, the file's line
endings aside.
*/
static bool
d_internal_raw_is(
    const struct d_ninja_doc* _doc,
    const struct d_ninja*     _ninja,
    size_t                    _value,
    const char*               _text
)
{
    struct d_internal_buffer text = { NULL, 0u, 0u, false };

    // the text as the file would hold it
    if (!d_internal_with_endings(_doc, _text, strlen(_text), &text))
    {
        d_internal_buffer_free(&text);

        return false;
    }

    const struct d_ninja_value* const value = &_ninja->values[_value];
    const bool same = d_internal_same_text(&_ninja->text[value->start],
                                           value->end - value->start,
                                           text.text, text.length);

    d_internal_buffer_free(&text);

    return same;
}


/*
d_internal_name_is
  Whether a span of a reading's text is a name.
*/
static bool
d_internal_name_is(
    const struct d_ninja* _ninja,
    size_t                _start,
    size_t                _end,
    const char*           _name
)
{
    return d_internal_same_text(&_ninja->text[_start], _end - _start, _name,
                                strlen(_name));
}


/*
d_internal_alike
  Whether two statements, each of its own reading, say the same: kind,
name, value, paths in their sections, and bindings, each value unparsed.
  Long as each part is compared in turn.
*/
static bool
d_internal_alike(
    const struct d_ninja* _left,
    size_t                _a,
    const struct d_ninja* _right,
    size_t                _b
)
{
    const struct d_ninja_statement* const a = &_left->statements[_a];
    const struct d_ninja_statement* const b = &_right->statements[_b];

    // the statement itself
    if ( (a->kind != b->kind) ||
         (!d_internal_same_text(&_left->text[a->name_start],
                                a->name_end - a->name_start,
                                &_right->text[b->name_start],
                                b->name_end - b->name_start)) ||
         (!d_internal_same_unparsed(_left, a->value, _right, b->value)) ||
         (a->path_count != b->path_count) ||
         (a->binding_count != b->binding_count) )
    {
        return false;
    }

    // each path
    for (size_t at = 0; at < a->path_count; ++at)
    {
        const struct d_ninja_path* const x = &_left->paths[a->first_path + at];
        const struct d_ninja_path* const y =
            &_right->paths[b->first_path + at];

        // its section and its value
        if ( (x->section != y->section) ||
             (!d_internal_same_unparsed(_left, x->value, _right, y->value)) )
        {
            return false;
        }
    }

    // each binding
    for (size_t at = 0; at < a->binding_count; ++at)
    {
        const struct d_ninja_binding* const x =
            &_left->bindings[a->first_binding + at];
        const struct d_ninja_binding* const y =
            &_right->bindings[b->first_binding + at];

        // its name and its value
        if ( (!d_internal_same_text(&_left->text[x->name_start],
                                    x->name_end - x->name_start,
                                    &_right->text[y->name_start],
                                    y->name_end - y->name_start)) ||
             (!d_internal_same_unparsed(_left, x->value, _right, y->value)) )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_paths_back
  Whether the new statement's paths are the old's, but for the one the
change set, added or took away, which holds the text in its section.
*/
static bool
d_internal_paths_back(
    const struct d_ninja_doc*       _doc,
    const struct d_ninja*           _fresh,
    const struct d_ninja_statement* _a,
    const struct d_ninja_statement* _b,
    const struct d_internal_expect* _expect
)
{
    const struct d_ninja* const old   = &_doc->records;
    const size_t                count = (_expect->path_added)
                                            ? (_a->path_count + 1u)
                                      : (_expect->path_removed)
                                            ? (_a->path_count - 1u)
                                            : _a->path_count;

    // as many as there are to be
    if (_b->path_count != count)
    {
        return false;
    }

    // each path now, against the one it was
    for (size_t k = 0; k < count; ++k)
    {
        const struct d_ninja_path* const y = &_fresh->paths[_b->first_path +
                                                            k];

        // the one written holds the text, in its section
        if ( (!_expect->path_removed) &&
             (k == _expect->path) )
        {
            // the text, and the section asked for
            if ( (y->section != _expect->section) ||
                 (!d_internal_raw_is(_doc, _fresh, y->value,
                                     _expect->text)) )
            {
                return false;
            }

            continue;
        }

        const size_t from =
            ( (_expect->path_added) && (k > _expect->path) ) ? (k - 1u)
          : ( (_expect->path_removed) && (k >= _expect->path) ) ? (k + 1u)
          :                                                       k;
        const struct d_ninja_path* const x = &old->paths[_a->first_path +
                                                         from];

        // the rest, as they were
        if ( (x->section != y->section) ||
             (!d_internal_same_unparsed(old, x->value, _fresh, y->value)) )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_removed
  Whether an old binding is among those the change removes.
*/
static bool
d_internal_removed(
    const struct d_internal_expect* _expect,
    size_t                          _binding
)
{
    // each removed
    for (size_t at = 0; at < _expect->removed_bindings.count; ++at)
    {
        // this one
        if (_expect->removed_bindings.items[at] == _binding)
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_bindings_back
  Whether the new statement's bindings are the old's, but for those the
change set, added or took away: one set or added holds its name and the
text.
  Long as the old bindings are walked beside the new, past those removed.
*/
static bool
d_internal_bindings_back(
    const struct d_ninja_doc*       _doc,
    const struct d_ninja*           _fresh,
    const struct d_ninja_statement* _a,
    const struct d_ninja_statement* _b,
    const struct d_internal_expect* _expect
)
{
    const struct d_ninja* const old  = &_doc->records;
    const size_t                gone = _expect->removed_bindings.count;
    size_t                      from = 0;

    // as many as there are to be
    if (_b->binding_count != (_a->binding_count - gone +
                              ( (_expect->binding_added) ? 1u : 0u )))
    {
        return false;
    }

    // each binding now, against the one it was
    for (size_t k = 0; k < _b->binding_count; ++k)
    {
        const struct d_ninja_binding* const y =
            &_fresh->bindings[_b->first_binding + k];

        // past those removed
        while (d_internal_removed(_expect, from))
        {
            from++;
        }

        const bool changed = (_expect->binding != D_NINJA_NONE) &&
                             ( ( (_expect->binding_added) &&
                                 (k == (_b->binding_count - 1u)) ) ||
                               ( (!_expect->binding_added) &&
                                 (from == _expect->binding) ) );

        // the one set holds its name and the text
        if (changed)
        {
            // its name and value
            if ( (!d_internal_name_is(_fresh, y->name_start, y->name_end,
                                      _expect->binding_name)) ||
                 (!d_internal_raw_is(_doc, _fresh, y->value, _expect->text)) )
            {
                return false;
            }

            from += (_expect->binding_added) ? 0u : 1u;
            continue;
        }

        const struct d_ninja_binding* const x =
            &old->bindings[_a->first_binding + from];

        // the rest, as they were
        if ( (!d_internal_same_text(&old->text[x->name_start],
                                    x->name_end - x->name_start,
                                    &_fresh->text[y->name_start],
                                    y->name_end - y->name_start)) ||
             (!d_internal_same_unparsed(old, x->value, _fresh, y->value)) )
        {
            return false;
        }

        from++;
    }

    return true;
}


/*
d_internal_statement_change
  Tries a splice within statement `_n`, and keeps it if the statement reads
back as the change was to leave it: the one statement cut, its kind, its
name, its value, its paths and its bindings each as expected.
  Long as each part of the expectation is checked in turn.
*/
static bool
d_internal_statement_change(
    struct d_ninja_doc*             _doc,
    size_t                          _n,
    const struct d_internal_expect* _expect,
    size_t                          _start,
    size_t                          _end,
    const char*                     _splice
)
{
    struct d_internal_candidate candidate;

    // the splice
    if (!d_internal_try(_doc, _start, _end, _splice, strlen(_splice),
                        &candidate))
    {
        return false;
    }

    const struct d_ninja* const           fresh = &candidate.records;
    const struct d_ninja_statement* const a     =
        &_doc->records.statements[_n];
    const struct d_ninja_statement* const b     =
        ( (candidate.before == _n) &&
          (candidate.old_after == (_n + 1u)) &&
          (candidate.new_after == (_n + 1u)) ) ? &fresh->statements[_n]
                                               : NULL;
    const char* const                     name  =
        (_expect->name) ? _expect->name : NULL;
    bool                                  kept  =
        (b != NULL) && (a->kind == b->kind);

    // its name: the one given, or the one it had
    kept = (kept) &&
           ( (name) ? d_internal_name_is(fresh, b->name_start, b->name_end,
                                         name)
                    : d_internal_same_text(&_doc->records.text[a->name_start],
                                           a->name_end - a->name_start,
                                           &fresh->text[b->name_start],
                                           b->name_end - b->name_start) );

    // its value: the text given, or the one it had; an include's is its
    // file, which its paths answer for
    kept = (kept) &&
           ( (_expect->value) ? d_internal_raw_is(_doc, fresh, b->value,
                                                  _expect->text)
           : ( (a->kind == D_NINJA_INCLUDE) ||
               (a->kind == D_NINJA_SUBNINJA) ) ? true
                              : d_internal_same_unparsed(&_doc->records,
                                                         a->value, fresh,
                                                         b->value) );

    // its paths and bindings
    kept = (kept) &&
           (d_internal_paths_back(_doc, fresh, a, b, _expect)) &&
           (d_internal_bindings_back(_doc, fresh, a, b, _expect));

    // a change that did not read back is not kept
    if (!kept)
    {
        d_internal_discard(&candidate);

        return d_internal_fail(_doc, "the change does not read back as "
                                     "given");
    }

    return d_internal_commit(_doc, &candidate);
}


/*
d_internal_expect_none
  An expectation that the statement is left as it was.
*/
static void
d_internal_expect_none(
    struct d_internal_expect* _expect,
    const char*               _text
)
{
    memset(_expect, 0, sizeof(*_expect));
    _expect->text    = _text;
    _expect->binding = D_NINJA_NONE;
    _expect->path    = D_NINJA_NONE;

    return;
}


/*
d_internal_statement_at
  Statement `_n`, or NULL, the document saying why.
*/
static const struct d_ninja_statement*
d_internal_statement_at(
    struct d_ninja_doc* _doc,
    size_t              _n
)
{
    // one the file has
    if (_n >= _doc->records.statement_count)
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the file has no statement %zu", _n + 1u);

        return NULL;
    }

    return &_doc->records.statements[_n];
}


//==============================================================================
// 7.  VALUES, NAMES AND BINDINGS
//==============================================================================


/*
d_internal_after_equals
  The text a value is spliced in as: a space before it where the value
stands against its `=`, so it does not run into it.
*/
static bool
d_internal_after_equals(
    const struct d_ninja_doc* _doc,
    size_t                    _start,
    const char*               _text,
    struct d_internal_buffer* _out
)
{
    // a space after a bare `=`, for a value that has text
    if ( (_start > 0u) &&
         (_doc->text[_start - 1u] == '=') &&
         (_text[0] != '\0') )
    {
        (void)d_internal_add_char(_out, ' ');
    }

    (void)d_internal_add(_out, _text, strlen(_text));

    return !_out->failed;
}


bool
d_ninja_doc_set_value(
    struct d_ninja_doc* _doc,
    size_t              _statement,
    const char*         _text
)
{
    struct d_internal_buffer splice = { NULL, 0u, 0u, false };
    struct d_internal_expect expect;

    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(_doc, _statement);

    // a variable the file has
    if ( (!statement) ||
         (statement->kind != D_NINJA_VARIABLE) )
    {
        return (statement) ? d_internal_fail(_doc, "the statement is no "
                                                   "variable")
                           : false;
    }

    const struct d_ninja_value* const value =
        &_doc->records.values[statement->value];
    const size_t                      start = value->start;
    const size_t                      end   = value->end;

    d_internal_expect_none(&expect, _text);
    expect.value = true;

    const bool kept = (d_internal_after_equals(_doc, start, _text, &splice)) &&
                      (d_internal_statement_change(_doc, _statement, &expect,
                                                   start, end, splice.text));

    d_internal_buffer_free(&splice);

    return kept;
}


bool
d_ninja_doc_set_name(
    struct d_ninja_doc* _doc,
    size_t              _statement,
    const char*         _name
)
{
    struct d_internal_expect expect;

    // parameter validation first
    if ( (!_doc) ||
         (!_name) )
    {
        return false;
    }

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(_doc, _statement);

    // a statement with a name
    if ( (!statement) ||
         (statement->name_end == statement->name_start) )
    {
        return (statement) ? d_internal_fail(_doc, "the statement has no "
                                                   "name")
                           : false;
    }

    // a name ninja reads
    if (!d_ninja_is_name(_name, strlen(_name)))
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "not a name: %s", _name);

        return false;
    }

    d_internal_expect_none(&expect, "");
    expect.name = _name;

    return d_internal_statement_change(_doc, _statement, &expect,
                                       statement->name_start,
                                       statement->name_end, _name);
}


/*
d_internal_last_binding
  The last binding of a name of a statement, from 0 among its bindings, or
D_NINJA_NONE.
*/
static size_t
d_internal_last_binding(
    const struct d_ninja_doc*       _doc,
    const struct d_ninja_statement* _statement,
    const char*                     _name
)
{
    size_t found = D_NINJA_NONE;

    // each binding, the last of the name kept
    for (size_t at = 0; at < _statement->binding_count; ++at)
    {
        const struct d_ninja_binding* const binding =
            &_doc->records.bindings[_statement->first_binding + at];

        // the name's
        if (d_internal_name_is(&_doc->records, binding->name_start,
                               binding->name_end, _name))
        {
            found = at;
        }
    }

    return found;
}


/*
d_internal_line_ending
  Where the line ending a statement's last line stands: the CR of a CR LF,
or the LF; the statement's end for a last line the file does not end.
*/
static size_t
d_internal_line_ending(
    const struct d_ninja_doc*       _doc,
    const struct d_ninja_statement* _statement
)
{
    const size_t end = _statement->end;

    // a last line with no ending
    if ( (end == 0u) ||
         (_doc->text[end - 1u] != '\n') )
    {
        return end;
    }

    return ( (end >= 2u) &&
             (_doc->text[end - 2u] == '\r') ) ? (end - 2u) : (end - 1u);
}


/*
d_internal_new_binding
  A binding added after a statement's last, before that line's ending: a
line ending, the indentation of its first binding or two spaces, the name,
` = ` and the text.
*/
static bool
d_internal_new_binding(
    const struct d_ninja_doc*       _doc,
    const struct d_ninja_statement* _statement,
    const char*                     _name,
    const char*                     _text,
    struct d_internal_buffer*       _out
)
{
    (void)d_internal_add_char(_out, '\n');

    // the indentation of its bindings, or two spaces
    if (_statement->binding_count > 0u)
    {
        const struct d_ninja_binding* const first =
            &_doc->records.bindings[_statement->first_binding];

        (void)d_internal_add(_out, &_doc->text[first->start],
                             first->name_start - first->start);
    }
    else
    {
        (void)d_internal_add(_out, "  ", 2u);
    }

    (void)d_internal_add(_out, _name, strlen(_name));
    (void)d_internal_add(_out, " = ", (_text[0] != '\0') ? 3u : 2u);
    (void)d_internal_add(_out, _text, strlen(_text));

    return !_out->failed;
}


/*
d_ninja_doc_set_binding
  Long as it is a table of the two changes, a binding set or one added,
each laid out before the change is tried.
*/
bool
d_ninja_doc_set_binding(
    struct d_ninja_doc* _doc,
    size_t              _statement,
    const char*         _name,
    const char*         _text
)
{
    struct d_internal_buffer splice = { NULL, 0u, 0u, false };
    struct d_internal_expect expect;

    // parameter validation first
    if ( (!_doc) ||
         (!_name) ||
         (!_text) )
    {
        return false;
    }

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(_doc, _statement);

    // a statement that takes bindings, and a name
    if ( (!statement) ||
         ( (statement->kind != D_NINJA_RULE) &&
           (statement->kind != D_NINJA_BUILD) &&
           (statement->kind != D_NINJA_POOL) ) ||
         (!d_ninja_is_name(_name, strlen(_name))) )
    {
        return (!statement) ? false
             : d_internal_fail(_doc, (d_ninja_is_name(_name, strlen(_name)))
                                         ? "the statement takes no bindings"
                                         : "not a binding's name");
    }

    const size_t last  = d_internal_last_binding(_doc, statement, _name);
    size_t       start = d_internal_line_ending(_doc, statement);
    size_t       end   = start;
    bool         made  = false;

    d_internal_expect_none(&expect, _text);
    expect.binding       = (last == D_NINJA_NONE) ? 0u : last;
    expect.binding_added = (last == D_NINJA_NONE);
    expect.binding_name  = _name;

    // a table of the two: the last of the name set, or one added
    if (last != D_NINJA_NONE)
    {
        const struct d_ninja_binding* const binding =
            &_doc->records.bindings[statement->first_binding + last];
        const struct d_ninja_value* const   value   =
            &_doc->records.values[binding->value];

        start = value->start;
        end   = value->end;
        made  = d_internal_after_equals(_doc, start, _text, &splice);
    }
    else
    {
        made = d_internal_new_binding(_doc, statement, _name, _text, &splice);
    }

    const bool kept = (made) &&
                      (d_internal_statement_change(_doc, _statement, &expect,
                                                   start, end, splice.text));

    d_internal_buffer_free(&splice);

    return kept;
}


/*
d_ninja_doc_remove_binding
  Long as each binding of the name is found, and the text between them
kept, in one splice.
*/
bool
d_ninja_doc_remove_binding(
    struct d_ninja_doc* _doc,
    size_t              _statement,
    const char*         _name
)
{
    struct d_internal_buffer splice  = { NULL, 0u, 0u, false };
    struct d_internal_expect expect;
    size_t                   gone[64];
    size_t                   count   = 0;

    // parameter validation first
    if ( (!_doc) ||
         (!_name) )
    {
        return false;
    }

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(_doc, _statement);

    // a statement the file has
    if (!statement)
    {
        return false;
    }

    size_t start = 0;
    size_t end   = 0;
    size_t from  = 0;

    // each binding of the name, its line left out of the splice's text
    for (size_t at = 0; at < statement->binding_count; ++at)
    {
        const struct d_ninja_binding* const binding =
            &_doc->records.bindings[statement->first_binding + at];

        // one of another name, or one past what can be counted
        if ( (!d_internal_name_is(&_doc->records, binding->name_start,
                                  binding->name_end, _name)) ||
             (count >= (sizeof(gone) / sizeof(gone[0]))) )
        {
            continue;
        }

        // the text between the last removed and this one stays
        if (count > 0u)
        {
            (void)d_internal_add(&splice, &_doc->text[from],
                                 binding->start - from);
        }

        start         = (count == 0u) ? binding->start : start;
        end           = binding->end;
        from          = binding->end;
        gone[count++] = at;
    }

    // a binding of the name to remove
    if (count == 0u)
    {
        d_internal_buffer_free(&splice);
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the statement has no binding %s", _name);

        return false;
    }

    d_internal_expect_none(&expect, "");
    expect.removed_bindings.items = gone;
    expect.removed_bindings.count = count;

    const bool kept = (!splice.failed) &&
                      (d_internal_statement_change(_doc, _statement, &expect,
                                                   start, end,
                                                   (splice.text) ? splice.text
                                                                 : ""));

    d_internal_buffer_free(&splice);

    return kept;
}


//==============================================================================
// 8.  PATHS
//==============================================================================


/*
d_internal_path_value
  The value of a statement's path `_k`.
*/
static const struct d_ninja_value*
d_internal_path_value(
    const struct d_ninja_doc*       _doc,
    const struct d_ninja_statement* _statement,
    size_t                          _k
)
{
    const size_t value = _doc->records.paths[_statement->first_path + _k].value;

    return &_doc->records.values[value];
}


bool
d_ninja_doc_set_path(
    struct d_ninja_doc* _doc,
    size_t              _statement,
    size_t              _path,
    const char*         _text
)
{
    struct d_internal_expect expect;

    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(_doc, _statement);

    // a path the statement has
    if ( (!statement) ||
         (_path >= statement->path_count) )
    {
        return (statement) ? d_internal_fail(_doc, "the statement has no "
                                                   "such path")
                           : false;
    }

    const struct d_ninja_value* const value =
        d_internal_path_value(_doc, statement, _path);

    d_internal_expect_none(&expect, _text);
    expect.path    = _path;
    expect.section =
        _doc->records.paths[statement->first_path + _path].section;

    return d_internal_statement_change(_doc, _statement, &expect,
                                       value->start, value->end, _text);
}


/*
d_internal_section_place
  Where a path inserted as the `_place`th of its section goes, and what
sets it apart: before the path there, after the section's last, or, for a
section with none, after what ninja reads before it, with its pipe.  The
index the path takes among the statement's is given.
  Long as it is a table of the sections and what each follows.
*/
static bool
d_internal_section_place(
    const struct d_ninja_doc*       _doc,
    const struct d_ninja_statement* _statement,
    enum d_ninja_section            _section,
    size_t                          _place,
    size_t*                         _out_at,
    const char**                    _out_lead,
    size_t*                         _out_index
)
{
    static const char* const pipes[] =
    {
        " ", " | ", " ", " | ", " || ", " |@ ", " ", " "
    };
    size_t count = 0;
    size_t last  = D_NINJA_NONE;
    size_t prior = D_NINJA_NONE;

    // the section's paths, and the last path of a section before it
    for (size_t k = 0; k < _statement->path_count; ++k)
    {
        const enum d_ninja_section section =
            _doc->records.paths[_statement->first_path + k].section;

        // a table of where the path stands: before the place, in the
        // section, or before the section
        if ( (section == _section) &&
             (count == _place) )
        {
            *_out_at    = d_internal_path_value(_doc, _statement, k)->start;
            *_out_lead  = NULL;
            *_out_index = k;

            return true;
        }
        else if (section == _section)
        {
            count++;
            last = k;
        }
        else if (section < _section)
        {
            prior = k;
        }
    }

    // the place one past the section's last, or a place there is not
    if (_place != count)
    {
        return false;
    }

    const size_t follow = (last != D_NINJA_NONE) ? last : prior;
    const bool   inputs = (_section >= D_NINJA_INPUT) &&
                          (_section <= D_NINJA_VALIDATION);

    *_out_lead  = (last != D_NINJA_NONE) ? " " : pipes[_section];
    *_out_index = (follow == D_NINJA_NONE) ? 0u : (follow + 1u);

    // a table of what the path follows: a path, the rule's name for an
    // input section none before it fills, or the build's keyword
    if ( (follow != D_NINJA_NONE) &&
         ( (!inputs) ||
           (_doc->records.paths[_statement->first_path + follow].section >=
            D_NINJA_INPUT) ) )
    {
        *_out_at = d_internal_path_value(_doc, _statement, follow)->end;
    }
    else if (inputs)
    {
        *_out_at    = _statement->name_end;
        *_out_index = (follow == D_NINJA_NONE) ? 0u : (follow + 1u);
    }
    else
    {
        *_out_at = _statement->start + 5u;
    }

    return true;
}


bool
d_ninja_doc_insert_path(
    struct d_ninja_doc*  _doc,
    size_t               _statement,
    enum d_ninja_section _section,
    size_t               _place,
    const char*          _text
)
{
    struct d_internal_buffer splice = { NULL, 0u, 0u, false };
    struct d_internal_expect expect;
    size_t                   at     = 0;
    const char*              lead   = NULL;
    size_t                   index  = 0;

    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(_doc, _statement);
    const bool                            fits      =
        (statement) &&
        ( ( (statement->kind == D_NINJA_BUILD) &&
            (_section <= D_NINJA_VALIDATION) ) ||
          ( (statement->kind == D_NINJA_DEFAULT) &&
            (_section == D_NINJA_TARGET) ) );

    // a section the statement may have, and a place in it
    if ( (!fits) ||
         (!d_internal_section_place(_doc, statement, _section, _place, &at,
                                    &lead, &index)) )
    {
        return (statement) ? d_internal_fail(_doc, "the statement has no "
                                                   "such place for a path")
                           : false;
    }

    // after what it follows, set apart; or before a path, a space after
    if (lead)
    {
        (void)d_internal_add(&splice, lead, strlen(lead));
        (void)d_internal_add(&splice, _text, strlen(_text));
    }
    else
    {
        (void)d_internal_add(&splice, _text, strlen(_text));
        (void)d_internal_add_char(&splice, ' ');
    }

    d_internal_expect_none(&expect, _text);
    expect.path       = index;
    expect.path_added = true;
    expect.section    = _section;

    const bool kept = (!splice.failed) &&
                      (d_internal_statement_change(_doc, _statement, &expect,
                                                   at, at, splice.text));

    d_internal_buffer_free(&splice);

    return kept;
}


/*
d_internal_back_over_blanks
  Back from a place over the blanks before it, and over each line
continued with `$` that the blanks stand on, and the blanks before that;
never before the statement's first byte.
*/
static size_t
d_internal_back_over_blanks(
    const struct d_ninja_doc*       _doc,
    const struct d_ninja_statement* _statement,
    size_t                          _at
)
{
    size_t at   = _at;
    bool   more = true;

    // blanks, then a continuation and the blanks before it, as long as any
    while (more)
    {
        // the blanks
        while ( (at > _statement->start) &&
                (_doc->text[at - 1u] == ' ') )
        {
            at--;
        }

        size_t ending  = at;
        size_t dollars = 0;

        // a line ending, its CR with it
        if ( (ending > _statement->start) &&
             (_doc->text[ending - 1u] == '\n') )
        {
            ending--;
            ending -= ( (ending > _statement->start) &&
                        (_doc->text[ending - 1u] == '\r') ) ? 1u : 0u;
        }

        // the `$`s before it: an odd run continues the line
        while ( (ending < at) &&
                ((ending - dollars) > _statement->start) &&
                (_doc->text[ending - dollars - 1u] == '$') )
        {
            dollars++;
        }

        more = ( (ending < at) &&
                 ((dollars % 2u) == 1u) );
        at   = (more) ? (ending - 1u) : at;
    }

    return at;
}


/*
d_internal_path_span
  The span a path takes with it: the blanks before it, and its section's
pipe and the blanks before that when it was the section's only path.
*/
static void
d_internal_path_span(
    const struct d_ninja_doc*       _doc,
    const struct d_ninja_statement* _statement,
    size_t                          _k,
    size_t*                         _out_start,
    size_t*                         _out_end
)
{
    const enum d_ninja_section section =
        _doc->records.paths[_statement->first_path + _k].section;
    const struct d_ninja_value* const value =
        d_internal_path_value(_doc, _statement, _k);
    size_t                            alone = 0;
    size_t                            start =
        d_internal_back_over_blanks(_doc, _statement, value->start);

    // how many paths its section has
    for (size_t k = 0; k < _statement->path_count; ++k)
    {
        alone += (_doc->records.paths[_statement->first_path + k].section ==
                  section) ? 1u : 0u;
    }

    // a pipe section's only path takes its pipe with it
    if ( (alone == 1u) &&
         ( (section == D_NINJA_IMPLICIT_OUTPUT) ||
           (section == D_NINJA_IMPLICIT) ||
           (section == D_NINJA_ORDER_ONLY) ||
           (section == D_NINJA_VALIDATION) ) )
    {
        // the pipe, `|`, `||` or `|@`
        while ( (start > _statement->start) &&
                ( (_doc->text[start - 1u] == '|') ||
                  (_doc->text[start - 1u] == '@') ) )
        {
            start--;
        }

        start = d_internal_back_over_blanks(_doc, _statement, start);
    }

    *_out_start = start;
    *_out_end   = value->end;

    return;
}


bool
d_ninja_doc_remove_path(
    struct d_ninja_doc* _doc,
    size_t              _statement,
    size_t              _path
)
{
    struct d_internal_expect expect;
    size_t                   start = 0;
    size_t                   end   = 0;

    // parameter validation first
    if (!_doc)
    {
        return false;
    }

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(_doc, _statement);

    // a path the statement has, and not an include's file
    if ( (!statement) ||
         (_path >= statement->path_count) ||
         (statement->kind == D_NINJA_INCLUDE) ||
         (statement->kind == D_NINJA_SUBNINJA) )
    {
        return (statement) ? d_internal_fail(_doc, "the statement has no "
                                                   "such path to remove")
                           : false;
    }

    d_internal_path_span(_doc, statement, _path, &start, &end);
    d_internal_expect_none(&expect, "");
    expect.path         = _path;
    expect.path_removed = true;

    return d_internal_statement_change(_doc, _statement, &expect, start, end,
                                       "");
}


//==============================================================================
// 9.  STATEMENTS
//==============================================================================


/*
d_internal_read_alone
  Reads a text of statements alone: false, the document saying why, when it
does not read, or holds none.
*/
static bool
d_internal_read_alone(
    struct d_ninja_doc* _doc,
    const char*         _text,
    struct d_ninja*     _out
)
{
    char why[256];

    // the text read as a file of its own
    if (!d_ninja_read(_out, _text, strlen(_text), why, sizeof(why)))
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "not statements: %s", why);
        d_ninja_release(_out);

        return false;
    }

    // with a statement in it
    if (_out->statement_count == 0u)
    {
        d_ninja_release(_out);

        return d_internal_fail(_doc, "not statements: the text has none");
    }

    return true;
}


/*
d_internal_with_ending
  A text, a line ending added when its last line has none.
*/
static bool
d_internal_with_ending(
    const char*               _text,
    bool                      _first,
    struct d_internal_buffer* _out
)
{
    const size_t length = strlen(_text);

    // a line ending for a file whose last line has none
    if (_first)
    {
        (void)d_internal_add_char(_out, '\n');
    }

    (void)d_internal_add(_out, _text, length);

    // the text's own last line ended
    if ( (length > 0u) &&
         (_text[length - 1u] != '\n') )
    {
        (void)d_internal_add_char(_out, '\n');
    }

    return !_out->failed;
}


/*
d_internal_statements_change
  Tries a splice that replaces statements `_first` to `_first + _cut`, and
keeps it if the statements there read as `_written`'s do.
*/
static bool
d_internal_statements_change(
    struct d_ninja_doc*   _doc,
    size_t                _first,
    size_t                _cut,
    const struct d_ninja* _written,
    size_t                _start,
    size_t                _end,
    const char*           _splice,
    size_t                _length
)
{
    struct d_internal_candidate candidate;
    const size_t                made = (_written) ? _written->statement_count
                                                  : 0u;

    // the splice
    if (!d_internal_try(_doc, _start, _end, _splice, _length, &candidate))
    {
        return false;
    }

    // the statements cut, and those made in their place, as many as asked
    bool kept = ( (candidate.before == _first)             &&
                  (candidate.old_after == (_first + _cut)) &&
                  (candidate.new_after == (_first + made)) );

    // each made reads as written
    for (size_t at = 0; (kept) && (at < made); ++at)
    {
        kept = d_internal_alike(&candidate.records, _first + at, _written,
                                at);
    }

    // a change that did not read back is not kept
    if (!kept)
    {
        d_internal_discard(&candidate);

        return d_internal_fail(_doc, "the change does not read back as "
                                     "given");
    }

    return d_internal_commit(_doc, &candidate);
}


/*
d_internal_lead
  The first line of the comments that lead into a statement: each comment
line directly above it, and above that, that no statement holds; the
statement's own line when there is none.
*/
static size_t
d_internal_lead(
    const struct d_ninja_doc*       _doc,
    const struct d_ninja_statement* _statement
)
{
    const struct d_ninja* const ninja = &_doc->records;
    size_t                      top   = _statement->line;
    size_t                      start = _statement->start;

    // the comments before the statement, from the nearest up
    for (size_t at = ninja->comment_count; at > 0u; --at)
    {
        const struct d_ninja_comment* const comment = &ninja->comments[at - 1u];

        // one on the statement's line or after it leads into nothing
        if (comment->start >= start)
        {
            continue;
        }

        // the line directly above, a statement's own comment, or none
        if ( (comment->within != D_NINJA_NONE) ||
             ((comment->line + 1u) != top) )
        {
            break;
        }

        top   = comment->line;
        start = ninja->line_starts[comment->line - 1u];
    }

    return start;
}


bool
d_ninja_doc_insert_statements(
    struct d_ninja_doc* _doc,
    size_t              _before,
    const char*         _text
)
{
    struct d_ninja           written;
    struct d_internal_buffer splice = { NULL, 0u, 0u, false };

    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const size_t count = _doc->records.statement_count;

    // a place the file has: before a statement, or after the last
    if (_before > count)
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the file has no statement %zu", _before + 1u);

        return false;
    }

    const size_t at     = (_before < count)
                              ? d_internal_lead(_doc,
                                                &_doc->records
                                                     .statements[_before])
                              : _doc->length;
    const bool   ending = ( (at > 0u) &&
                            (_doc->text[at - 1u] != '\n') );

    // the text, its line ended, read alone
    if ( (!d_internal_with_ending(_text, ending, &splice)) ||
         (!d_internal_read_alone(_doc, &splice.text[(ending) ? 1u : 0u],
                                 &written)) )
    {
        d_internal_buffer_free(&splice);

        return false;
    }

    const bool kept = d_internal_statements_change(_doc, _before, 0u,
                                                   &written, at, at,
                                                   splice.text,
                                                   splice.length);

    d_ninja_release(&written);
    d_internal_buffer_free(&splice);

    return kept;
}


bool
d_ninja_doc_set_statement(
    struct d_ninja_doc* _doc,
    size_t              _statement,
    const char*         _text
)
{
    struct d_ninja           written;
    struct d_internal_buffer splice = { NULL, 0u, 0u, false };

    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const struct d_ninja_statement* const statement =
        d_internal_statement_at(_doc, _statement);

    // a statement the file has, and the text, its line ended, read alone
    if ( (!statement) ||
         (!d_internal_with_ending(_text, false, &splice)) ||
         (!d_internal_read_alone(_doc, splice.text, &written)) )
    {
        d_internal_buffer_free(&splice);

        return false;
    }

    // one statement, of the kind it replaces
    if ( (written.statement_count != 1u) ||
         (written.statements[0].kind != statement->kind) )
    {
        d_ninja_release(&written);
        d_internal_buffer_free(&splice);
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "not one %s: %s", d_ninja_kind_name(statement->kind),
                       _text);

        return false;
    }

    const bool kept = d_internal_statements_change(_doc, _statement, 1u,
                                                   &written,
                                                   statement->start,
                                                   statement->end,
                                                   splice.text,
                                                   splice.length);

    d_ninja_release(&written);
    d_internal_buffer_free(&splice);

    return kept;
}


bool
d_ninja_doc_remove_statements(
    struct d_ninja_doc* _doc,
    size_t              _first,
    size_t              _last
)
{
    // parameter validation first
    if (!_doc)
    {
        return false;
    }

    // a run of statements the file has
    if ( (_first > _last) ||
         (_last >= _doc->records.statement_count) )
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the file has no statements %zu to %zu", _first + 1u,
                       _last + 1u);

        return false;
    }

    return d_internal_statements_change(
               _doc, _first, (_last - _first) + 1u, NULL,
               _doc->records.statements[_first].start,
               _doc->records.statements[_last].end, "", 0u);
}


//==============================================================================
// 10. SAVING
//==============================================================================


enum d_ninja_saved
d_ninja_doc_save(
    struct d_ninja_doc* _doc,
    const char*         _path,
    char*               _problem,
    size_t              _capacity
)
{
    size_t length = 0;

    // parameter validation first
    if (!_doc)
    {
        (void)snprintf(_problem, _capacity, "no file to save");

        return D_NINJA_SAVE_FAILED;
    }

    const char* const path = ( (_path) && (_path[0] != '\0') ) ? _path
                                                               : _doc->path;

    // somewhere to write it
    if (path[0] == '\0')
    {
        (void)snprintf(_problem, _capacity, "the file has no path to save "
                                            "to");

        return D_NINJA_SAVE_FAILED;
    }

    char* const there = d_text_read(path, &length);
    const bool  same  = (there) &&
                        (d_internal_same_text(there, length, _doc->text,
                                              _doc->length));

    free(there);

    // a file that holds the text already is left as it is
    if (same)
    {
        return D_NINJA_SAVE_UNCHANGED;
    }

    // the directories it goes in
    if (!d_fs_make_parent(path))
    {
        (void)snprintf(_problem, _capacity,
                       "cannot make the directories of %s", path);

        return D_NINJA_SAVE_FAILED;
    }

    return (d_text_write(path, _doc->text, _doc->length, _problem, _capacity))
               ? D_NINJA_SAVE_WRITTEN : D_NINJA_SAVE_FAILED;
}
