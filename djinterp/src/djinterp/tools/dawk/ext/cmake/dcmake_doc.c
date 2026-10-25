/*******************************************************************************
* djinterp [tools]                                                  dcmake_doc.c
*
* Definitions for the declarations in dcmake_doc.h.
*   Every change is a candidate first: the new text, read into records of
* its own. The commands are in the text's order and never overlap, so those
* the splice leaves whole before it are a prefix of the old and of the new,
* those it leaves whole after it a suffix of each, and the few it cuts lie
* between: those before must be the file's own, those after the file's own
* moved by what the splice added, and a command that pairs with one between
* must pair with one between in the new. A change of a kind then checks the
* commands between for what it was to do; only then are the file's text and
* records the candidate's. Comments are compared as commands are.
*   Files are read and written with dsplice's helpers, the directories a
* file goes in made with dfs's.
*
* path:      /src/djinterp/tools/dawk/ext/cmake/dcmake_doc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/cmake/dcmake_doc.h"  // corresponding header
// std
#include <stddef.h>  // ptrdiff_t
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memchr, memcmp, memcpy, memset, strlen
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
// d_cmake_doc
//   struct: the file: its path, text and records, the changes it has taken,
//   where the last was, and why the last failed.
struct d_cmake_doc
{
    char*          path;
    char*          text;
    size_t         length;
    struct d_cmake records;
    size_t         changes;
    size_t         start;
    size_t         old_end;
    size_t         new_end;
    char           problem[512];
};

// 1.1.2
// d_internal_candidate
//   struct: a change not yet kept: the new text and its records, where the
//   splice stands in the old text and the new, and how the commands fall
//   about it -- `before` whole before it in both, the old's from
//   `old_after` and the new's from `new_after` whole after it.
struct d_internal_candidate
{
    char*          text;
    size_t         length;
    struct d_cmake records;
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
// D_INTERNAL_BETWEEN
//   constant: a command's index mapped into the new records when it is one
//   of those the splice cut, and the new has another number of them.
#define D_INTERNAL_BETWEEN (D_CMAKE_NONE - 1u)


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


//==============================================================================
// 3.  DOCUMENTS
//==============================================================================


/*
d_internal_new
  A document of a text it takes, read; NULL, the text freed, when it does
not read as CMake.
*/
static struct d_cmake_doc*
d_internal_new(
    const char* _path,
    char*       _text,
    size_t      _length,
    char*       _problem,
    size_t      _capacity
)
{
    struct d_cmake_doc* const doc  = calloc(1u, sizeof(*doc));
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

    // a text CMake would refuse is no document
    if (!d_cmake_read(&doc->records, doc->text, doc->length, why,
                      sizeof(why)))
    {
        (void)snprintf(_problem, _capacity, "%s: %s", path, why);
        d_cmake_doc_free(doc);

        return NULL;
    }

    return doc;
}


struct d_cmake_doc*
d_cmake_doc_read(
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


struct d_cmake_doc*
d_cmake_doc_make(
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
d_cmake_doc_free(
    struct d_cmake_doc* _doc
)
{
    // freeing nothing is allowed
    if (!_doc)
    {
        return;
    }

    d_cmake_release(&_doc->records);
    free(_doc->text);
    free(_doc->path);
    free(_doc);

    return;
}


const struct d_cmake*
d_cmake_doc_records(
    const struct d_cmake_doc* _doc
)
{
    return (_doc) ? &_doc->records : NULL;
}


const char*
d_cmake_doc_path(
    const struct d_cmake_doc* _doc
)
{
    return (_doc) ? _doc->path : "";
}


size_t
d_cmake_doc_changes(
    const struct d_cmake_doc* _doc
)
{
    return (_doc) ? _doc->changes : 0u;
}


bool
d_cmake_doc_changed(
    const struct d_cmake_doc* _doc,
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

    // each place asked for
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
d_cmake_doc_problem(
    const struct d_cmake_doc* _doc
)
{
    return (_doc) ? _doc->problem : "";
}


const char*
d_cmake_doc_text(
    const struct d_cmake_doc* _doc,
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
    struct d_cmake_doc* _doc,
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
d_internal_moved, d_internal_moved_line
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
d_internal_mapped
  A command's index in the old records as an index in the new: the same
before the splice, moved after it, and among those it cut the same only
when the new has as many of them; D_INTERNAL_BETWEEN otherwise.
*/
static size_t
d_internal_mapped(
    const struct d_internal_candidate* _candidate,
    size_t                             _index
)
{
    // a table of where the command stood: nowhere, before, after, between
    if (_index == D_CMAKE_NONE)
    {
        return D_CMAKE_NONE;
    }
    else if (_index < _candidate->before)
    {
        return _index;
    }
    else if (_index >= _candidate->old_after)
    {
        return _index - _candidate->old_after + _candidate->new_after;
    }

    return ( (_candidate->old_after - _candidate->before) ==
             (_candidate->new_after - _candidate->before) )
               ? _index : D_INTERNAL_BETWEEN;
}


/*
d_internal_same_index
  Whether an index the old records hold -- a partner, a comment's command
-- is the one the new hold.
*/
static bool
d_internal_same_index(
    const struct d_internal_candidate* _candidate,
    size_t                             _old,
    size_t                             _new
)
{
    const size_t mapped = d_internal_mapped(_candidate, _old);

    // one among those cut, when they are not as many, is any of them
    if (mapped == D_INTERNAL_BETWEEN)
    {
        return ( (_new != D_CMAKE_NONE)      &&
                 (_new >= _candidate->before) &&
                 (_new < _candidate->new_after) );
    }

    return (mapped == _new);
}


/*
d_internal_same_argument
  Whether an argument of the old records is one of the new, moved.
*/
static bool
d_internal_same_argument(
    const struct d_cmake_argument* _a,
    const struct d_cmake_argument* _b,
    ptrdiff_t                      _shift,
    ptrdiff_t                      _lines
)
{
    return ( (_a->kind == _b->kind)                             &&
             (d_internal_moved(_a->start, _shift) == _b->start) &&
             (d_internal_moved(_a->end, _shift) == _b->end)     &&
             (d_internal_moved(_a->line, _lines) == _b->line)   &&
             (d_internal_moved(_a->end_line, _lines) ==
                  _b->end_line)                                 &&
             (_a->paren == _b->paren)                           &&
             (_a->legacy == _b->legacy)                         &&
             (_a->equals == _b->equals) );
}


/*
d_internal_same_command
  Whether a command of the old records is one of the new, moved: where it
is, its arguments, and the part it takes in its block.
*/
static bool
d_internal_same_command(
    const struct d_cmake_doc*          _doc,
    const struct d_internal_candidate* _candidate,
    size_t                             _left,
    size_t                             _right,
    bool                               _after
)
{
    const struct d_cmake_command* const a = &_doc->records.commands[_left];
    const struct d_cmake_command* const b =
        &_candidate->records.commands[_right];
    const ptrdiff_t shift = (_after) ? _candidate->shift : 0;
    const ptrdiff_t lines = (_after) ? _candidate->lines : 0;

    // the command itself
    if ( (d_internal_moved(a->start, shift) != b->start)              ||
         (a->name_length != b->name_length)                           ||
         (d_internal_moved(a->open, shift) != b->open)                ||
         (d_internal_moved(a->end, shift) != b->end)                  ||
         (d_internal_moved(a->line, lines) != b->line)                ||
         (d_internal_moved(a->end_line, lines) != b->end_line)        ||
         (a->argument_count != b->argument_count)                     ||
         (a->depth != b->depth)                                       ||
         (a->block != b->block)                                       ||
         (a->role != b->role)                                         ||
         (a->unpaired != b->unpaired)                                 ||
         (!d_internal_same_index(_candidate, a->partner, b->partner)) )
    {
        return false;
    }

    // each of its arguments
    for (size_t at = 0; at < a->argument_count; ++at)
    {
        // moved as the command is
        if (!d_internal_same_argument(
                 &_doc->records.arguments[a->first_argument + at],
                 &_candidate->records.arguments[b->first_argument + at],
                 shift, lines))
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_same_comment
  Whether a comment of the old records is one of the new, moved.
*/
static bool
d_internal_same_comment(
    const struct d_cmake_doc*          _doc,
    const struct d_internal_candidate* _candidate,
    size_t                             _left,
    size_t                             _right,
    bool                               _after
)
{
    const struct d_cmake_comment* const a = &_doc->records.comments[_left];
    const struct d_cmake_comment* const b =
        &_candidate->records.comments[_right];
    const ptrdiff_t shift = (_after) ? _candidate->shift : 0;
    const ptrdiff_t lines = (_after) ? _candidate->lines : 0;

    return ( (d_internal_moved(a->start, shift) == b->start)           &&
             (d_internal_moved(a->end, shift) == b->end)               &&
             (d_internal_moved(a->line, lines) == b->line)             &&
             (d_internal_moved(a->end_line, lines) == b->end_line)     &&
             (a->bracket == b->bracket)                                &&
             (a->trailing == b->trailing)                              &&
             (d_internal_same_index(_candidate, a->within, b->within)) );
}


/*
d_internal_place_commands
  How the commands of both texts fall about the splice; false when as many
do not lie whole before it, or after it, in both.
*/
static bool
d_internal_place_commands(
    const struct d_cmake_doc*    _doc,
    struct d_internal_candidate* _candidate
)
{
    const struct d_cmake* const old   = &_doc->records;
    const struct d_cmake* const fresh = &_candidate->records;
    size_t                      left  = 0;
    size_t                      right = 0;

    // those wholly before the splice, in each
    while ( (left < old->command_count) &&
            (old->commands[left].end <= _candidate->start) )
    {
        left++;
    }

    // ...and in the new
    while ( (right < fresh->command_count) &&
            (fresh->commands[right].end <= _candidate->start) )
    {
        right++;
    }

    const bool as_many = (left == right);

    _candidate->before = left;

    // those wholly after it, in each
    while ( (left < old->command_count) &&
            (old->commands[left].start < _candidate->old_end) )
    {
        left++;
    }

    // ...and in the new
    while ( (right < fresh->command_count) &&
            (fresh->commands[right].start < _candidate->new_end) )
    {
        right++;
    }

    _candidate->old_after = left;
    _candidate->new_after = right;

    return ( (as_many) &&
             ( (old->command_count - left) ==
               (fresh->command_count - right) ) );
}


/*
d_internal_outside_commands
  Whether every command outside the splice is the file's own; gives the
line of the first that is not.
*/
static bool
d_internal_outside_commands(
    const struct d_cmake_doc*          _doc,
    const struct d_internal_candidate* _candidate,
    size_t*                            _out_line
)
{
    const size_t after = _doc->records.command_count - _candidate->old_after;

    // those before
    for (size_t at = 0; at < _candidate->before; ++at)
    {
        // the same, where it was
        if ( (at >= _candidate->records.command_count) ||
             (!d_internal_same_command(_doc, _candidate, at, at, false)) )
        {
            *_out_line = _doc->records.commands[at].line;

            return false;
        }
    }

    // those after
    for (size_t at = 0; at < after; ++at)
    {
        // the same, moved
        if (!d_internal_same_command(_doc, _candidate,
                                     _candidate->old_after + at,
                                     _candidate->new_after + at, true))
        {
            *_out_line =
                _doc->records.commands[_candidate->old_after + at].line;

            return false;
        }
    }

    return true;
}


/*
d_internal_comment_place
  Where a comment lies: -1 wholly before the splice, 1 wholly after it, 0
cut by it.
*/
static int
d_internal_comment_place(
    const struct d_cmake_comment* _comment,
    size_t                        _start,
    size_t                        _end
)
{
    // a table of the three places
    if (_comment->end <= _start)
    {
        return -1;
    }
    else if (_comment->start >= _end)
    {
        return 1;
    }

    return 0;
}


/*
d_internal_outside_comments
  Whether every comment outside the splice is the file's own; gives the
line of the first that is not.  Those before are a prefix of each list and
those after a suffix, as the comments are in the text's order.
  Long as the two lists are walked side by side through the three places,
each a loop.
*/
static bool
d_internal_outside_comments(
    const struct d_cmake_doc*          _doc,
    const struct d_internal_candidate* _candidate,
    size_t*                            _out_line
)
{
    const struct d_cmake* const old   = &_doc->records;
    const struct d_cmake* const fresh = &_candidate->records;
    size_t                      left  = 0;
    size_t                      right = 0;

    // those before, pair by pair
    while ( (left < old->comment_count) &&
            (d_internal_comment_place(&old->comments[left],
                                      _candidate->start,
                                      _candidate->old_end) < 0) )
    {
        // one the new does not hold where it was
        if ( (right >= fresh->comment_count) ||
             (!d_internal_same_comment(_doc, _candidate, left, right,
                                       false)) )
        {
            *_out_line = old->comments[left].line;

            return false;
        }

        left++;
        right++;
    }

    size_t old_first = left;
    size_t new_first = right;

    // past those cut, in each
    while ( (old_first < old->comment_count) &&
            (d_internal_comment_place(&old->comments[old_first],
                                      _candidate->start,
                                      _candidate->old_end) == 0) )
    {
        old_first++;
    }

    // in the new, one before the splice the old does not hold refuses it
    while ( (new_first < fresh->comment_count) &&
            (d_internal_comment_place(&fresh->comments[new_first],
                                      _candidate->start,
                                      _candidate->new_end) < 0) )
    {
        *_out_line = fresh->comments[new_first].line;

        return false;
    }

    // ...and past those cut in the new
    while ( (new_first < fresh->comment_count) &&
            (d_internal_comment_place(&fresh->comments[new_first],
                                      _candidate->start,
                                      _candidate->new_end) == 0) )
    {
        new_first++;
    }

    // as many after, in each
    if ( (old->comment_count - old_first) !=
         (fresh->comment_count - new_first) )
    {
        *_out_line = (old_first < old->comment_count)
                         ? old->comments[old_first].line
                         : old->line_count;

        return false;
    }

    // those after, pair by pair
    for (; old_first < old->comment_count; ++old_first, ++new_first)
    {
        // the same, moved
        if (!d_internal_same_comment(_doc, _candidate, old_first, new_first,
                                     true))
        {
            *_out_line = old->comments[old_first].line;

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
    const struct d_cmake_doc* _doc,
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
    d_cmake_release(&_candidate->records);
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
    struct d_cmake_doc*          _doc,
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
d_internal_unpaired
  How many commands of a reading are unpaired.
*/
static size_t
d_internal_unpaired(
    const struct d_cmake* _cmake
)
{
    size_t count = 0;

    // each command, counted when it pairs with none
    for (size_t at = 0; at < _cmake->command_count; ++at)
    {
        count += (_cmake->commands[at].unpaired) ? 1u : 0u;
    }

    return count;
}


/*
d_internal_try
  Makes a candidate of a splice, and keeps it only if the file outside it
reads as it did, and it leaves no block unpaired that was not.
  Long as each of the checks a candidate passes is a step.
*/
static bool
d_internal_try(
    struct d_cmake_doc*          _doc,
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
    if (!d_cmake_read(&_out->records, _out->text, _out->length, why,
                      sizeof(why)))
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the change would leave the file unreadable: %s",
                       why);
        d_internal_discard(_out);

        return false;
    }

    // the commands outside the splice as they were
    if ( (!d_internal_place_commands(_doc, _out)) ||
         (!d_internal_outside_commands(_doc, _out, &line)) )
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the change would change the command at line %zu, "
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

    // no more blocks left unpaired than there were
    if (d_internal_unpaired(&_out->records) >
        d_internal_unpaired(&_doc->records))
    {
        d_internal_discard(_out);

        return d_internal_fail(_doc, "the change would leave a block "
                                     "unpaired");
    }

    return true;
}


/*
d_internal_commit
  Keeps a candidate: the file is its text and records now.
*/
static bool
d_internal_commit(
    struct d_cmake_doc*          _doc,
    struct d_internal_candidate* _candidate
)
{
    d_cmake_release(&_doc->records);
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
d_cmake_doc_splice(
    struct d_cmake_doc* _doc,
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
// 6.  ARGUMENTS
//==============================================================================


// 6.1    Places
//------------------------------------------------------------------------------
/*
d_internal_command, d_internal_argument
  Command `_n`, and argument `_k` of it, both from 0; NULL for none.
*/
static const struct d_cmake_command*
d_internal_command(
    const struct d_cmake* _cmake,
    size_t                _n
)
{
    return (_n < _cmake->command_count) ? &_cmake->commands[_n] : NULL;
}


static const struct d_cmake_argument*
d_internal_argument(
    const struct d_cmake* _cmake,
    size_t                _n,
    size_t                _k
)
{
    const struct d_cmake_command* const command =
        d_internal_command(_cmake, _n);

    // an argument the command does not have is none
    if ( (!command) ||
         (_k >= command->argument_count) )
    {
        return NULL;
    }

    return &_cmake->arguments[command->first_argument + _k];
}


/*
d_internal_line_start
  The offset of the first byte of the line holding `_at`.
*/
static size_t
d_internal_line_start(
    const struct d_cmake_doc* _doc,
    size_t                    _at
)
{
    size_t at = _at;

    // back to the line feed before it, or the file's start
    while ( (at > 0u) &&
            (_doc->text[at - 1u] != '\n') )
    {
        at--;
    }

    return at;
}


/*
d_internal_is_blank
  Whether a byte is a blank: a space or a tab.
*/
static bool
d_internal_is_blank(
    char _c
)
{
    return ( (_c == ' ') ||
             (_c == '\t') );
}


/*
d_internal_blank_between
  Whether only blanks stand from `_from` to `_to`.
*/
static bool
d_internal_blank_between(
    const struct d_cmake_doc* _doc,
    size_t                    _from,
    size_t                    _to
)
{
    // each byte, a blank
    for (size_t at = _from; at < _to; ++at)
    {
        // anything else is something on the line
        if (!d_internal_is_blank(_doc->text[at]))
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_starts_line
  Whether nothing but blanks stands before a place on its line.
*/
static bool
d_internal_starts_line(
    const struct d_cmake_doc* _doc,
    size_t                    _at
)
{
    return d_internal_blank_between(_doc, d_internal_line_start(_doc, _at),
                                    _at);
}


/*
d_internal_skip_blanks
  The first place from `_at` that holds no blank.
*/
static size_t
d_internal_skip_blanks(
    const struct d_cmake_doc* _doc,
    size_t                    _at
)
{
    size_t at = _at;

    // past each blank
    while ( (at < _doc->length) &&
            (d_internal_is_blank(_doc->text[at])) )
    {
        at++;
    }

    return at;
}


/*
d_internal_at_line_end
  Whether a place is the end of its line: a line ending, or the file's end.
*/
static bool
d_internal_at_line_end(
    const struct d_cmake_doc* _doc,
    size_t                    _at
)
{
    return ( (_at >= _doc->length)       ||
             (_doc->text[_at] == '\n')   ||
             ( (_doc->text[_at] == '\r')    &&
               ((_at + 1u) < _doc->length) &&
               (_doc->text[_at + 1u] == '\n') ) );
}


/*
d_internal_past_line_end
  Past the line ending at a place, when one is there.
*/
static size_t
d_internal_past_line_end(
    const struct d_cmake_doc* _doc,
    size_t                    _at
)
{
    size_t at = _at;

    // a CR of a CR LF
    if ( (at < _doc->length) &&
         (_doc->text[at] == '\r') )
    {
        at++;
    }

    // the line feed
    if ( (at < _doc->length) &&
         (_doc->text[at] == '\n') )
    {
        at++;
    }

    return at;
}


/*
d_internal_comment_after
  The end of the last comment within command `_n` -- D_CMAKE_NONE for none
-- that starts on line `_line` at or after `_from`; `_from` when there is
none.
*/
static size_t
d_internal_comment_after(
    const struct d_cmake_doc* _doc,
    size_t                    _n,
    size_t                    _line,
    size_t                    _from
)
{
    size_t end = _from;

    // each comment, the last on that line after the place
    for (size_t at = 0; at < _doc->records.comment_count; ++at)
    {
        const struct d_cmake_comment* const comment =
            &_doc->records.comments[at];

        // within the command, on that line, after the place
        if ( (comment->within == _n)    &&
             (comment->line == _line)   &&
             (comment->start >= _from) )
        {
            end = comment->end;
        }
    }

    return end;
}


/*
d_internal_separates
  Whether a byte keeps an argument from running into its neighbour: space,
a line ending, or a parenthesis.
*/
static bool
d_internal_separates(
    char _byte
)
{
    return ( (_byte == ' ')  ||
             (_byte == '\t') ||
             (_byte == '\r') ||
             (_byte == '\n') ||
             (_byte == '(')  ||
             (_byte == ')') );
}


/*
d_internal_space_before, d_internal_space_after
  Whether text spliced in at `_at` needs a space before it, the byte before
running into it otherwise, or after it, running into the byte after; a
comment's `#` after it ends an unquoted argument on its own.
*/
static bool
d_internal_space_before(
    const struct d_cmake_doc* _doc,
    size_t                    _at
)
{
    return ( (_at > 0u) &&
             (!d_internal_separates(_doc->text[_at - 1u])) );
}


static bool
d_internal_space_after(
    const struct d_cmake_doc* _doc,
    size_t                    _at
)
{
    return ( (_at < _doc->length)                      &&
             (!d_internal_separates(_doc->text[_at])) &&
             (_doc->text[_at] != '#') );
}


/*
d_internal_one_argument
  Whether a text reads as exactly one argument, and nothing else: it is
read as the sole argument of a command.
*/
static bool
d_internal_one_argument(
    struct d_cmake_doc* _doc,
    const char*         _text
)
{
    const size_t span    = strlen(_text);
    char* const  wrapped = malloc(span + 4u);
    bool         one     = false;

    // the text, wrapped as `x(...)`
    if (wrapped)
    {
        struct d_cmake cmake;

        // released whether or not it is read: an empty text is not
        memset(&cmake, 0, sizeof(cmake));
        wrapped[0] = 'x';
        wrapped[1] = '(';
        memcpy(&wrapped[2], _text, span);
        wrapped[span + 2u] = ')';
        wrapped[span + 3u] = '\0';

        one = ( (span > 0u)                                          &&
                (d_cmake_read(&cmake, wrapped, span + 3u, NULL, 0u)) &&
                (cmake.command_count == 1u)                          &&
                (cmake.argument_count == 1u)                         &&
                (cmake.comment_count == 0u)                          &&
                (cmake.arguments[0].start == 2u)                     &&
                (cmake.arguments[0].end == (span + 2u))              &&
                (cmake.arguments[0].paren == '\0') );
        d_cmake_release(&cmake);
        free(wrapped);
    }

    // the refusal names the text, so the program's author can find it
    if (!one)
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "not one argument: %s", _text);
    }

    return one;
}


/*
d_internal_changeable
  The argument `_k` of command `_n`, when it may be changed alone: it is
there and is no nested parenthesis, which pairs with another.  NULL, the
document saying why, otherwise.
*/
static const struct d_cmake_argument*
d_internal_changeable(
    struct d_cmake_doc* _doc,
    size_t              _n,
    size_t              _k
)
{
    const struct d_cmake_argument* const argument =
        d_internal_argument(&_doc->records, _n, _k);

    // an argument the command does not have
    if (!argument)
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "command %zu has no argument %zu", _n + 1u, _k + 1u);

        return NULL;
    }

    // a parenthesis, changed or taken alone, would leave its pair unpaired
    if (argument->paren != '\0')
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "argument %zu of command %zu is a parenthesis, which "
                       "cannot be changed alone", _k + 1u, _n + 1u);

        return NULL;
    }

    return argument;
}


// 6.2    Reading back
//------------------------------------------------------------------------------
// d_internal_reread
//   enum: what an argument's change did to its command's arguments.
enum d_internal_reread
{
    D_INTERNAL_SET = 0,
    D_INTERNAL_INSERTED,
    D_INTERNAL_REMOVED
};


/*
d_internal_same_text
  Whether two spans of two texts hold the same bytes.
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


/*
d_internal_arguments_read_back
  Whether command `_n` reads back as the change made it: the one command the
splice cut, its name, its part in its block, every argument it kept as it
was, and the one at `_slot` -- set or inserted -- the text asked for, with
the file's endings.
  Long as each argument is matched with the one it was, by what was done.
*/
static bool
d_internal_arguments_read_back(
    struct d_cmake_doc*                _doc,
    const struct d_internal_candidate* _candidate,
    size_t                             _n,
    size_t                             _slot,
    enum d_internal_reread             _what,
    const struct d_internal_buffer*    _text
)
{
    const struct d_cmake* const         old   = &_doc->records;
    const struct d_cmake* const         fresh = &_candidate->records;
    const struct d_cmake_command* const a     = &old->commands[_n];
    const size_t                        count =
        (_what == D_INTERNAL_INSERTED) ? (a->argument_count + 1u)
      : (_what == D_INTERNAL_REMOVED)  ? (a->argument_count - 1u)
      :                                  a->argument_count;

    // the one command cut, and no other
    if ( (_candidate->before != _n)                ||
         (_candidate->old_after != (_n + 1u))      ||
         (_candidate->new_after != (_n + 1u)) )
    {
        return d_internal_fail(_doc, "the change would cut another command");
    }

    const struct d_cmake_command* const b = &fresh->commands[_n];

    // the command as it was, but for its arguments
    if ( (!d_internal_same_text(&old->text[a->start], a->name_length,
                                &fresh->text[b->start], b->name_length)) ||
         (a->block != b->block)                                          ||
         (a->role != b->role)                                            ||
         (b->argument_count != count) )
    {
        return d_internal_fail(_doc, "the change does not read back as one "
                                     "argument changed");
    }

    // each argument now, matched with what it was
    for (size_t k = 0; k < count; ++k)
    {
        const struct d_cmake_argument* const y =
            &fresh->arguments[b->first_argument + k];

        // the one written holds what was asked
        if ( (_what != D_INTERNAL_REMOVED) &&
             (k == _slot) )
        {
            // its text, and nothing more
            if (!d_internal_same_text(&fresh->text[y->start],
                                      y->end - y->start, _text->text,
                                      _text->length))
            {
                return d_internal_fail(_doc, "the argument does not read "
                                             "back as given");
            }

            continue;
        }

        const size_t from =
            ( (_what == D_INTERNAL_INSERTED) && (k > _slot) ) ? (k - 1u)
          : ( (_what == D_INTERNAL_REMOVED) && (k >= _slot) ) ? (k + 1u)
          :                                                     k;
        const struct d_cmake_argument* const x =
            &old->arguments[a->first_argument + from];

        // the rest, as they were
        if ( (x->kind != y->kind)   ||
             (x->paren != y->paren) ||
             (!d_internal_same_text(&old->text[x->start], x->end - x->start,
                                    &fresh->text[y->start],
                                    y->end - y->start)) )
        {
            return d_internal_fail(_doc, "the change would change another "
                                         "argument");
        }
    }

    return true;
}


/*
d_internal_argument_change
  Tries a splice that changes command `_n`'s arguments, and keeps it if it
reads back as it was to.
*/
static bool
d_internal_argument_change(
    struct d_cmake_doc*    _doc,
    size_t                 _n,
    size_t                 _slot,
    enum d_internal_reread _what,
    const char*            _argument,
    size_t                 _start,
    size_t                 _end,
    const char*            _splice
)
{
    struct d_internal_buffer    text      = { NULL, 0u, 0u, false };
    struct d_internal_candidate candidate;

    // the argument as the file will hold it, its endings the file's
    if (!d_internal_with_endings(_doc, _argument, strlen(_argument), &text))
    {
        d_internal_buffer_free(&text);

        return d_internal_fail(_doc, "out of memory");
    }

    // the splice, and the command read back
    if (!d_internal_try(_doc, _start, _end, _splice, strlen(_splice),
                        &candidate))
    {
        d_internal_buffer_free(&text);

        return false;
    }

    // kept only if the command reads back as asked
    if (!d_internal_arguments_read_back(_doc, &candidate, _n, _slot, _what,
                                        &text))
    {
        d_internal_discard(&candidate);
        d_internal_buffer_free(&text);

        return false;
    }

    d_internal_buffer_free(&text);

    return d_internal_commit(_doc, &candidate);
}


// 6.3    Changes
//------------------------------------------------------------------------------
bool
d_cmake_doc_set_argument(
    struct d_cmake_doc* _doc,
    size_t              _command,
    size_t              _argument,
    const char*         _text
)
{
    struct d_internal_buffer splice = { NULL, 0u, 0u, false };

    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const struct d_cmake_argument* const argument =
        d_internal_changeable(_doc, _command, _argument);

    // the argument, changeable, and the text one argument
    if ( (!argument) ||
         (!d_internal_one_argument(_doc, _text)) )
    {
        return false;
    }

    // set apart from a neighbour written against it, so the new text cannot
    // run into it
    if (d_internal_space_before(_doc, argument->start))
    {
        (void)d_internal_add_char(&splice, ' ');
    }

    (void)d_internal_add(&splice, _text, strlen(_text));

    // and the one after
    if (d_internal_space_after(_doc, argument->end))
    {
        (void)d_internal_add_char(&splice, ' ');
    }

    const bool kept = (!splice.failed)                                     &&
                      (d_internal_argument_change(_doc, _command, _argument,
                                                  D_INTERNAL_SET, _text,
                                                  argument->start,
                                                  argument->end,
                                                  splice.text));

    // memory that ran out says so
    if (splice.failed)
    {
        (void)d_internal_fail(_doc, "out of memory");
    }

    d_internal_buffer_free(&splice);

    return kept;
}


/*
d_internal_separator
  What sets an argument written beside another apart from it: a line
ending and the neighbour's indentation, when the neighbour stands on a line
of its own, or a space.
*/
static void
d_internal_separator(
    const struct d_cmake_doc*      _doc,
    const struct d_cmake_argument* _near,
    bool                           _own_line,
    struct d_internal_buffer*      _out
)
{
    // a line of its own, indented as its neighbour
    if (_own_line)
    {
        const size_t line = d_internal_line_start(_doc, _near->start);

        (void)d_internal_add_char(_out, '\n');
        (void)d_internal_add(_out, &_doc->text[line], _near->start - line);

        return;
    }

    (void)d_internal_add_char(_out, ' ');

    return;
}


/*
d_internal_insert_place
  Where an argument inserted beside `_near` goes, and whether on a line of
its own: before an argument that begins its line, or after one that does
when nothing more of the command follows it there, past the comment ending
that line.
*/
static size_t
d_internal_insert_place(
    const struct d_cmake_doc*      _doc,
    size_t                         _n,
    size_t                         _k,
    bool                           _after,
    bool*                          _out_own_line
)
{
    const struct d_cmake_argument* const near =
        d_internal_argument(&_doc->records, _n, _k);
    const struct d_cmake_argument* const next =
        d_internal_argument(&_doc->records, _n, _k + 1u);
    const bool                           own  =
        (d_internal_starts_line(_doc, near->start)) &&
        ( (!_after) ||
          (!next)   ||
          (next->line != near->end_line) );

    *_out_own_line = own;

    // a table of the three places: before it, after it on a line of its
    // own past any comment, after it on its line
    if (!_after)
    {
        return near->start;
    }
    else if (own)
    {
        return d_internal_comment_after(_doc, _n, near->end_line, near->end);
    }

    return near->end;
}


/*
d_cmake_doc_insert_argument
  The new argument is written beside its neighbour as the neighbour
stands: on a line of its own, indented as it, when it stands on one, and a
space apart otherwise; into an empty list as it stands.
  Long as the text and its separator are put in either order, by which side
of the neighbour it goes.
*/
bool
d_cmake_doc_insert_argument(
    struct d_cmake_doc* _doc,
    size_t              _command,
    size_t              _argument,
    const char*         _text,
    bool                _after
)
{
    struct d_internal_buffer splice = { NULL, 0u, 0u, false };

    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const struct d_cmake_command* const command =
        d_internal_command(&_doc->records, _command);
    const size_t count = (command) ? command->argument_count : 0u;

    // a place the command has: beside an argument, or past the last
    if ( (!command) ||
         (_argument > count) ||
         ( (_argument == count) && (_after) ) )
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "command %zu has no argument %zu", _command + 1u,
                       _argument + 1u);

        return false;
    }

    // the text one argument
    if (!d_internal_one_argument(_doc, _text))
    {
        return false;
    }

    const bool   append = ( (_argument == count) && (count > 0u) );
    const size_t near   = (append) ? (count - 1u) : _argument;
    const bool   after  = (_after) || (append);
    const size_t slot   = (after) ? (near + 1u) : near;
    bool         own    = false;
    size_t       at     = command->open + 1u;

    // beside an argument, in its layout; into an empty list as it stands
    if (count > 0u)
    {
        const struct d_cmake_argument* const beside =
            d_internal_argument(&_doc->records, _command, near);

        at = d_internal_insert_place(_doc, _command, near, after, &own);

        // after it: what separates them, then the text
        if (after)
        {
            d_internal_separator(_doc, beside, own, &splice);
        }
        else if ( (!own) &&
                  (d_internal_space_before(_doc, at)) )
        {
            (void)d_internal_add_char(&splice, ' ');
        }

        (void)d_internal_add(&splice, _text, strlen(_text));

        // before it: the text, then what separates them
        if (!after)
        {
            d_internal_separator(_doc, beside, own, &splice);
        }
        else if ( (!own) &&
                  (d_internal_space_after(_doc, at)) )
        {
            (void)d_internal_add_char(&splice, ' ');
        }
    }
    else
    {
        (void)d_internal_add(&splice, _text, strlen(_text));
    }

    const bool kept = (!splice.failed) &&
                      (d_internal_argument_change(_doc, _command, slot,
                                                  D_INTERNAL_INSERTED, _text,
                                                  at, at, splice.text));

    // memory that ran out says so
    if (splice.failed)
    {
        (void)d_internal_fail(_doc, "out of memory");
    }

    d_internal_buffer_free(&splice);

    return kept;
}


/*
d_internal_closes_up
  Where the removal of an argument that begins its line and is followed
there only by its command's `)` starts, so that the `)` closes up to what
came before it: the end of the argument before, or the `(`; `_from` when a
comment stands between, which the `)` must not follow.
*/
static size_t
d_internal_closes_up(
    const struct d_cmake_doc*      _doc,
    size_t                         _n,
    const struct d_cmake_argument* _argument,
    size_t                         _after
)
{
    const struct d_cmake_command* const command = &_doc->records.commands[_n];
    const struct d_cmake_argument* const first  =
        &_doc->records.arguments[command->first_argument];
    const size_t from = (_argument > first) ? _argument[-1].end
                                            : (command->open + 1u);

    // only the command's own `)` after it
    if ( (_after != (command->end - 1u)) ||
         (_doc->text[_after] != ')') )
    {
        return _argument->start;
    }

    // and no comment between
    for (size_t at = 0; at < _doc->records.comment_count; ++at)
    {
        const struct d_cmake_comment* const comment =
            &_doc->records.comments[at];

        // one the `)` would close up to, or follow into
        if ( (comment->start >= from) &&
             (comment->end <= _argument->start) )
        {
            return _argument->start;
        }
    }

    return from;
}


/*
d_internal_removal
  The span an argument takes with it: its whole line, the comment ending
it included, when it stood alone there; up to the argument before, or the
`(`, when only the `)` follows it on its line; otherwise the blanks that set
it apart from the argument before it on its line, or after it when nothing
comes before, but never the last blank between it and a `(`.
*/
static void
d_internal_removal(
    const struct d_cmake_doc*      _doc,
    size_t                         _n,
    const struct d_cmake_argument* _argument,
    size_t*                        _out_start,
    size_t*                        _out_end
)
{
    const size_t line_start = d_internal_line_start(_doc, _argument->start);
    const bool   first      = d_internal_starts_line(_doc, _argument->start);
    const size_t after      = d_internal_skip_blanks(
                                  _doc,
                                  d_internal_comment_after(_doc, _n,
                                                           _argument->end_line,
                                                           _argument->end));
    const size_t closing    = (first) ? d_internal_closes_up(_doc, _n,
                                                             _argument, after)
                                      : _argument->start;
    size_t       start      = _argument->start;
    size_t       end        = _argument->end;

    // a table of the four layouts: alone on its line, before the `)` on
    // it, after another on it, first on a line it shares
    if ( (first) &&
         (d_internal_at_line_end(_doc, after)) )
    {
        start = line_start;
        end   = d_internal_past_line_end(_doc, after);
    }
    else if (closing != _argument->start)
    {
        start = closing;
        end   = after;
    }
    else if (!first)
    {
        // the blanks before it go with it
        while ( (start > line_start) &&
                (d_internal_is_blank(_doc->text[start - 1u])) )
        {
            start--;
        }

        // unless they are all that keeps it from the `(`
        if (_doc->text[start - 1u] == '(')
        {
            start = _argument->start;
            end   = d_internal_skip_blanks(_doc, end);
        }
    }
    else
    {
        end = d_internal_skip_blanks(_doc, end);
    }

    *_out_start = start;
    *_out_end   = end;

    return;
}


bool
d_cmake_doc_remove_argument(
    struct d_cmake_doc* _doc,
    size_t              _command,
    size_t              _argument
)
{
    size_t start = 0;
    size_t end   = 0;

    // parameter validation first
    if (!_doc)
    {
        return false;
    }

    const struct d_cmake_argument* const argument =
        d_internal_changeable(_doc, _command, _argument);

    // the argument, changeable
    if (!argument)
    {
        return false;
    }

    d_internal_removal(_doc, _command, argument, &start, &end);

    // what stood either side must not run together where it goes
    const bool apart = (d_internal_space_before(_doc, start)) &&
                       (d_internal_space_after(_doc, end));

    return d_internal_argument_change(_doc, _command, _argument,
                                      D_INTERNAL_REMOVED, "", start, end,
                                      (apart) ? " " : "");
}


//==============================================================================
// 7.  COMMANDS
//==============================================================================


// 7.1    Layout
//------------------------------------------------------------------------------
/*
d_internal_indentation
  The blanks a command's line begins with: where it starts and how long.
*/
static size_t
d_internal_indentation(
    const struct d_cmake_doc*     _doc,
    const struct d_cmake_command* _command,
    size_t*                       _out_start
)
{
    const size_t line = d_internal_line_start(_doc, _command->start);

    *_out_start = line;

    return d_internal_skip_blanks(_doc, line) - line;
}


/*
d_internal_opens
  Whether a command opens a block, or a branch of one, that holds the
commands after it.
*/
static bool
d_internal_opens(
    const struct d_cmake_command* _command
)
{
    return ( (_command->role == D_CMAKE_ROLE_OPEN) ||
             ( (_command->role == D_CMAKE_ROLE_BRANCH) &&
               (!_command->unpaired) ) );
}


/*
d_internal_unit
  The file's own step of indentation: the first that a command inside a
block stands past its opener by; four spaces when the file has none.
*/
static void
d_internal_unit(
    const struct d_cmake_doc* _doc,
    const char**              _out_text,
    size_t*                   _out_length
)
{
    const struct d_cmake* const cmake = &_doc->records;

    *_out_text   = "    ";
    *_out_length = 4u;

    // each command that follows an opener, inside it
    for (size_t at = 1u; at < cmake->command_count; ++at)
    {
        const struct d_cmake_command* const outer = &cmake->commands[at - 1u];
        const struct d_cmake_command* const inner = &cmake->commands[at];
        size_t                              from  = 0;
        size_t                              to    = 0;

        // one a block deeper than the opener before it
        if ( (!d_internal_opens(outer)) ||
             (inner->depth != (outer->depth + 1u)) )
        {
            continue;
        }

        const size_t a = d_internal_indentation(_doc, outer, &from);
        const size_t b = d_internal_indentation(_doc, inner, &to);

        // indented past it, beginning as it does
        if ( (b > a) &&
             (memcmp(&_doc->text[from], &_doc->text[to], a) == 0) )
        {
            *_out_text   = &_doc->text[to + a];
            *_out_length = b - a;

            return;
        }
    }

    return;
}


/*
d_cmake_doc_indent
  The slot's depth is the depth of the block between command `_before - 1`
and command `_before`; a neighbour at that depth gives its indentation, and
an empty block its opener's and a step more.
  Long as each of the neighbours a slot may take its indentation from is
looked at in turn.
*/
size_t
d_cmake_doc_indent(
    const struct d_cmake_doc* _doc,
    size_t                    _before,
    char*                     _out,
    size_t                    _capacity
)
{
    const char* text   = "";
    size_t      length = 0;
    size_t      from   = 0;

    // parameter validation first
    if ( (!_doc) ||
         (!_out) ||
         (_capacity == 0u) )
    {
        return 0u;
    }

    const struct d_cmake* const         cmake = &_doc->records;
    const size_t                        at    =
        (_before < cmake->command_count) ? _before : cmake->command_count;
    const struct d_cmake_command* const prior =
        (at > 0u) ? &cmake->commands[at - 1u] : NULL;
    const struct d_cmake_command* const next  =
        (at < cmake->command_count) ? &cmake->commands[at] : NULL;
    const size_t                        depth =
        (prior) ? (prior->depth + (d_internal_opens(prior) ? 1u : 0u)) : 0u;

    // a table of the neighbours: one before at the slot's depth, one after
    // at it, or an opener whose block is empty
    if ( (prior) &&
         (prior->depth == depth) )
    {
        length = d_internal_indentation(_doc, prior, &from);
        text   = &_doc->text[from];
    }
    else if ( (next) &&
              (next->depth == depth) &&
              (next->role != D_CMAKE_ROLE_CLOSE) &&
              (next->role != D_CMAKE_ROLE_BRANCH) )
    {
        length = d_internal_indentation(_doc, next, &from);
        text   = &_doc->text[from];
    }
    else if (prior)
    {
        const char* unit   = NULL;
        size_t      step   = 0;
        const size_t outer = d_internal_indentation(_doc, prior, &from);

        d_internal_unit(_doc, &unit, &step);

        // the opener's, then a step, as far as there is room
        if ((outer + step) < _capacity)
        {
            memcpy(_out, &_doc->text[from], outer);
            memcpy(&_out[outer], unit, step);
            _out[outer + step] = '\0';

            return outer + step;
        }
    }

    length = (length < _capacity) ? length : (_capacity - 1u);
    memcpy(_out, text, length);
    _out[length] = '\0';

    return length;
}


/*
d_internal_lead
  The first line of the comments that lead into a command: each comment on
a line of its own directly above it, and above that, with no blank line
between; the command's own line when there is none.
*/
static size_t
d_internal_lead(
    const struct d_cmake_doc*     _doc,
    const struct d_cmake_command* _command
)
{
    const struct d_cmake* const cmake = &_doc->records;
    size_t                      top   = _command->line;
    size_t                      start = d_internal_line_start(_doc,
                                                              _command->start);

    // the comments before the command, from the nearest up
    for (size_t at = cmake->comment_count; at > 0u; --at)
    {
        const struct d_cmake_comment* const comment = &cmake->comments[at - 1u];

        // one after the command's line, or on it, leads into nothing
        if (comment->start >= start)
        {
            continue;
        }

        // the next line up, wholly a comment's, or the lead is done
        if ( (comment->within != D_CMAKE_NONE)                           ||
             (comment->trailing)                                         ||
             ((comment->end_line + 1u) != top)                           ||
             (!d_internal_starts_line(_doc, comment->start))             ||
             (!d_internal_at_line_end(_doc,
                                      d_internal_skip_blanks(_doc,
                                                             comment->end))) )
        {
            break;
        }

        top   = comment->line;
        start = d_internal_line_start(_doc, comment->start);
    }

    return start;
}


// 7.2    Reading back
//------------------------------------------------------------------------------
/*
d_internal_same_values
  Whether a command of one reading has the name and the arguments, by kind
and value, of a command of another.
*/
static bool
d_internal_same_values(
    const struct d_cmake*         _left,
    const struct d_cmake_command* _a,
    const struct d_cmake*         _right,
    const struct d_cmake_command* _b
)
{
    // the name and the count
    if ( (!d_internal_same_text(&_left->text[_a->start], _a->name_length,
                                &_right->text[_b->start], _b->name_length)) ||
         (_a->argument_count != _b->argument_count) )
    {
        return false;
    }

    // each argument, by its kind and its value
    for (size_t at = 0; at < _a->argument_count; ++at)
    {
        const struct d_cmake_argument* const x =
            &_left->arguments[_a->first_argument + at];
        const struct d_cmake_argument* const y =
            &_right->arguments[_b->first_argument + at];
        char* const one   = malloc((x->end - x->start) + 1u);
        char* const other = malloc((y->end - y->start) + 1u);
        bool        same  = ( (one) && (other) && (x->kind == y->kind) );

        // the values, compared
        if (same)
        {
            const size_t a = d_cmake_value(_left, x, one);
            const size_t b = d_cmake_value(_right, y, other);

            same = d_internal_same_text(one, a, other, b);
        }

        free(one);
        free(other);

        // one apart is apart
        if (!same)
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_read_commands
  Reads a text of commands alone: false, the document saying why, when it
does not read, or holds no command.
*/
static bool
d_internal_read_commands(
    struct d_cmake_doc* _doc,
    const char*         _text,
    struct d_cmake*     _out
)
{
    char why[256];

    // the text read as a file of its own
    if (!d_cmake_read(_out, _text, strlen(_text), why, sizeof(why)))
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "not commands: %s", why);
        d_cmake_release(_out);

        return false;
    }

    // with a command in it
    if (_out->command_count == 0u)
    {
        d_cmake_release(_out);

        return d_internal_fail(_doc, "not commands: the text has none");
    }

    return true;
}


/*
d_internal_misfit
  Why the commands a change made at `_first` do not fit where they stand:
one does not read as written, or one replaced takes another part in its
block than it took; NULL when they fit.
*/
static const char*
d_internal_misfit(
    const struct d_cmake_doc*          _doc,
    const struct d_internal_candidate* _candidate,
    size_t                             _first,
    const struct d_cmake*              _written,
    bool                               _same_role
)
{
    const size_t made = (_written) ? _written->command_count : 0u;

    // each made, against what was written and what it replaced
    for (size_t at = 0; at < made; ++at)
    {
        const struct d_cmake_command* const a =
            &_doc->records.commands[_first + at];
        const struct d_cmake_command* const b =
            &_candidate->records.commands[_first + at];

        // a table of the misfits: read otherwise, or another part
        if (!d_internal_same_values(&_candidate->records, b, _written,
                                    &_written->commands[at]))
        {
            return "the change does not read back as given";
        }
        else if ( (_same_role) &&
                  ( (a->block != b->block) || (a->role != b->role) ) )
        {
            return "the change would change the part the command takes in "
                   "its block";
        }
    }

    return NULL;
}


/*
d_internal_command_change
  Tries a splice that replaces the commands `_first` to `_first + _cut`,
and keeps it if the new commands there are `_written`'s and fit; a command
replaced, `_same_role`, keeps the part it took in its block.
*/
static bool
d_internal_command_change(
    struct d_cmake_doc*   _doc,
    size_t                _first,
    size_t                _cut,
    const struct d_cmake* _written,
    bool                  _same_role,
    size_t                _start,
    size_t                _end,
    const char*           _splice,
    size_t                _length
)
{
    struct d_internal_candidate candidate;
    const size_t                made = (_written) ? _written->command_count
                                                  : 0u;

    // the splice
    if (!d_internal_try(_doc, _start, _end, _splice, _length, &candidate))
    {
        return false;
    }

    // the commands cut, and those made in their place, as many as asked
    const char* why = ( (candidate.before == _first)             &&
                        (candidate.old_after == (_first + _cut)) &&
                        (candidate.new_after == (_first + made)) )
                          ? NULL
                          : "the change does not read back as given";

    // and the commands made fit where they stand
    if (!why)
    {
        why = d_internal_misfit(_doc, &candidate, _first, _written,
                                _same_role);
    }

    // a change that does not read back is not kept
    if (why)
    {
        d_internal_discard(&candidate);

        return d_internal_fail(_doc, why);
    }

    return d_internal_commit(_doc, &candidate);
}


// 7.3    Changes
//------------------------------------------------------------------------------
bool
d_cmake_doc_set_name(
    struct d_cmake_doc* _doc,
    size_t              _command,
    const char*         _name
)
{
    // parameter validation first
    if ( (!_doc) ||
         (!_name) )
    {
        return false;
    }

    const struct d_cmake_command* const command =
        d_internal_command(&_doc->records, _command);

    // a command the file has
    if (!command)
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the file has no command %zu", _command + 1u);

        return false;
    }

    // a name
    if (!d_cmake_is_name(_name, strlen(_name)))
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "not a command's name: %s", _name);

        return false;
    }

    // the command as it will read, to compare with
    const size_t             tail    = command->end - command->open;
    const size_t             span    = strlen(_name);
    struct d_internal_buffer written = { NULL, 0u, 0u, false };
    struct d_cmake           cmake;

    (void)d_internal_add(&written, _name, span);
    (void)d_internal_add(&written, &_doc->text[command->open], tail);

    // read alone, as the command written
    if ( (written.failed) ||
         (!d_internal_read_commands(_doc, written.text, &cmake)) )
    {
        d_internal_buffer_free(&written);

        return (written.failed) ? d_internal_fail(_doc, "out of memory")
                                : false;
    }

    const bool kept = d_internal_command_change(_doc, _command, 1u, &cmake,
                                                true, command->start,
                                                command->start +
                                                    command->name_length,
                                                _name, span);

    d_cmake_release(&cmake);
    d_internal_buffer_free(&written);

    return kept;
}


bool
d_cmake_doc_set_command(
    struct d_cmake_doc* _doc,
    size_t              _command,
    const char*         _text
)
{
    struct d_cmake cmake;

    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const struct d_cmake_command* const command =
        d_internal_command(&_doc->records, _command);

    // a command the file has
    if (!command)
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the file has no command %zu", _command + 1u);

        return false;
    }

    // the text, read alone
    if (!d_internal_read_commands(_doc, _text, &cmake))
    {
        return false;
    }

    const size_t span = strlen(_text);
    bool         one  = ( (cmake.command_count == 1u)  &&
                          (cmake.commands[0].start == 0u) &&
                          (cmake.commands[0].end == span) );

    // no comment outside the command's own parentheses
    for (size_t at = 0; (one) && (at < cmake.comment_count); ++at)
    {
        one = (cmake.comments[at].within == 0u);
    }

    // one command, and nothing else
    if (!one)
    {
        d_cmake_release(&cmake);
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "not one command: %s", _text);

        return false;
    }

    const bool kept = d_internal_command_change(_doc, _command, 1u, &cmake,
                                                true, command->start,
                                                command->end, _text, span);

    d_cmake_release(&cmake);

    return kept;
}


/*
d_internal_inside
  Whether a place of a text read is within one of its arguments or bracket
comments that spans lines, where an indentation would change it.
*/
static bool
d_internal_inside(
    const struct d_cmake* _cmake,
    size_t                _at
)
{
    // each argument that holds the place within it
    for (size_t index = 0; index < _cmake->argument_count; ++index)
    {
        // within it, not at its start
        if ( (_cmake->arguments[index].start < _at) &&
             (_cmake->arguments[index].end > _at) )
        {
            return true;
        }
    }

    // and each comment
    for (size_t index = 0; index < _cmake->comment_count; ++index)
    {
        // within it, not at its start
        if ( (_cmake->comments[index].start < _at) &&
             (_cmake->comments[index].end > _at) )
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_margin
  The fewest blanks a line of the text begins with, of the lines that may
be indented and hold something: the indentation the text has of its own,
which the file's takes the place of.
*/
static size_t
d_internal_margin(
    const struct d_cmake* _cmake,
    const char*           _text,
    size_t                _length
)
{
    size_t margin = (size_t)-1;
    size_t line   = 0;

    // each line
    while (line < _length)
    {
        size_t blanks = 0;

        // its blanks
        while ( ((line + blanks) < _length) &&
                ( (_text[line + blanks] == ' ') ||
                  (_text[line + blanks] == '\t') ) )
        {
            blanks++;
        }

        const size_t end  = line + blanks;
        const bool   held = ( (end < _length)        &&
                              (_text[end] != '\n')   &&
                              (_text[end] != '\r') );

        // a line that holds something, outside an argument
        if ( (held) &&
             (!d_internal_inside(_cmake, line)) &&
             (blanks < margin) )
        {
            margin = blanks;
        }

        const char* const feed = memchr(&_text[line], '\n', _length - line);

        line = (feed) ? (size_t)(feed - _text) + 1u : _length;
    }

    return (margin == (size_t)-1) ? 0u : margin;
}


/*
d_internal_lay_out
  The text of commands as it is inserted: each line that may be indented
taken off its own margin and given the file's indentation, a line ending
after the last, and one before the first when the file's last line has
none.
*/
static bool
d_internal_lay_out(
    const struct d_cmake* _cmake,
    const char*           _text,
    const char*           _indent,
    bool                  _ending_first,
    struct d_internal_buffer* _out
)
{
    const size_t length = strlen(_text);
    const size_t margin = d_internal_margin(_cmake, _text, length);
    size_t       line   = 0;

    // a line ending for a last line that has none
    if (_ending_first)
    {
        (void)d_internal_add_char(_out, '\n');
    }

    // each line, to the last that holds anything
    while (line < length)
    {
        const char* const feed = memchr(&_text[line], '\n', length - line);
        const size_t      end  = (feed) ? (size_t)(feed - _text) : length;
        const bool        outside = !d_internal_inside(_cmake, line);
        size_t            from    = line;

        // a line outside an argument: its margin off, the file's on, but
        // for a line that holds nothing
        if (outside)
        {
            from = ( (line + margin) <= end ) ? (line + margin) : end;

            // only blanks, or nothing, gets no indentation
            if ( (from < end) &&
                 (_text[from] != '\r') )
            {
                (void)d_internal_add(_out, _indent, strlen(_indent));
            }
        }

        (void)d_internal_add(_out, &_text[from], end - from);
        (void)d_internal_add_char(_out, '\n');
        line = end + 1u;
    }

    return !_out->failed;
}


/*
d_cmake_doc_insert_commands
  The text is read alone first, so that its commands are known and its
lines inside an argument are left as they are; the place, the indentation
and the line endings are then the file's.
  Long as the place is one of three, each found here.
*/
bool
d_cmake_doc_insert_commands(
    struct d_cmake_doc* _doc,
    size_t              _before,
    const char*         _text
)
{
    struct d_cmake           cmake;
    struct d_internal_buffer splice = { NULL, 0u, 0u, false };
    char                     indent[256];

    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const size_t count = _doc->records.command_count;

    // a place the file has: before a command, or after the last
    if (_before > count)
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the file has no command %zu", _before + 1u);

        return false;
    }

    // the text, read alone
    if (!d_internal_read_commands(_doc, _text, &cmake))
    {
        return false;
    }

    size_t at = _doc->length;

    // a table of the places: above a command and its lead, past the last
    // command's line, or at the end of a file with none
    if (_before < count)
    {
        at = d_internal_lead(_doc, &_doc->records.commands[_before]);
    }
    else if (count > 0u)
    {
        const struct d_cmake_command* const last =
            &_doc->records.commands[count - 1u];
        const size_t                        after =
            d_internal_skip_blanks(_doc,
                                   d_internal_comment_after(_doc,
                                                            D_CMAKE_NONE,
                                                            last->end_line,
                                                            last->end));

        // past the line's ending; after the command, when more stands there
        at = (d_internal_at_line_end(_doc, after))
                 ? d_internal_past_line_end(_doc, after)
                 : last->end;
    }

    (void)d_cmake_doc_indent(_doc, _before, indent, sizeof(indent));

    const bool ending = ( (at > 0u) && (_doc->text[at - 1u] != '\n') );
    const bool laid   = d_internal_lay_out(&cmake, _text, indent, ending,
                                           &splice);
    const bool kept   = (laid) &&
                        (d_internal_command_change(_doc, _before, 0u, &cmake,
                                                   false, at, at, splice.text,
                                                   splice.length));

    // memory that ran out says so
    if (!laid)
    {
        (void)d_internal_fail(_doc, "out of memory");
    }

    d_cmake_release(&cmake);
    d_internal_buffer_free(&splice);

    return kept;
}


bool
d_cmake_doc_remove_commands(
    struct d_cmake_doc* _doc,
    size_t              _first,
    size_t              _last
)
{
    // parameter validation first
    if (!_doc)
    {
        return false;
    }

    // a run of commands the file has
    if ( (_first > _last) ||
         (_last >= _doc->records.command_count) )
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the file has no commands %zu to %zu", _first + 1u,
                       _last + 1u);

        return false;
    }

    const struct d_cmake_command* const first =
        &_doc->records.commands[_first];
    const struct d_cmake_command* const last  =
        &_doc->records.commands[_last];
    const size_t                        after =
        d_internal_skip_blanks(_doc,
                               d_internal_comment_after(_doc, D_CMAKE_NONE,
                                                        last->end_line,
                                                        last->end));
    const bool                          alone =
        (d_internal_starts_line(_doc, first->start)) &&
        (d_internal_at_line_end(_doc, after));
    const size_t                        start =
        (alone) ? d_internal_line_start(_doc, first->start) : first->start;
    const size_t                        end   =
        (alone) ? d_internal_past_line_end(_doc, after) : last->end;

    return d_internal_command_change(_doc, _first, (_last - _first) + 1u,
                                     NULL, false, start, end, "", 0u);
}


//==============================================================================
// 8.  SAVING
//==============================================================================


enum d_cmake_saved
d_cmake_doc_save(
    struct d_cmake_doc* _doc,
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

        return D_CMAKE_SAVE_FAILED;
    }

    const char* const path = ( (_path) && (_path[0] != '\0') ) ? _path
                                                               : _doc->path;

    // somewhere to write it
    if (path[0] == '\0')
    {
        (void)snprintf(_problem, _capacity, "the file has no path to save "
                                            "to");

        return D_CMAKE_SAVE_FAILED;
    }

    char* const there = d_text_read(path, &length);
    const bool  same  = (there) &&
                        (d_internal_same_text(there, length, _doc->text,
                                              _doc->length));

    free(there);

    // a file that holds the text already is left as it is
    if (same)
    {
        return D_CMAKE_SAVE_UNCHANGED;
    }

    // the directories it goes in
    if (!d_fs_make_parent(path))
    {
        (void)snprintf(_problem, _capacity,
                       "cannot make the directories of %s", path);

        return D_CMAKE_SAVE_FAILED;
    }

    return (d_text_write(path, _doc->text, _doc->length, _problem, _capacity))
               ? D_CMAKE_SAVE_WRITTEN : D_CMAKE_SAVE_FAILED;
}
