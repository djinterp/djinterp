/*******************************************************************************
* djinterp [tools]                                                   dmake_doc.c
*
* A makefile held for change: see dmake_doc.h.
*   Every change is a splice tried as a candidate: the text before it, the
* new text with the file's line endings, the text after, read again as the
* file's make reads it. The candidate is kept only when the statements wholly
* before the splice are the file's as they were, those wholly after it the
* file's moved by the splice -- each part, line, depth, branch and orphan
* mark -- the comments likewise, and the change's own check holds: the part
* asked for holds the text asked for, the words the words, a command the
* command, statements inserted the statements their text reads as alone.
* Otherwise the candidate is discarded, the document as it was.
*
* path:      /src/djinterp/tools/dawk/ext/make/dmake_doc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/make/dmake_doc.h"  // corresponding header
// std
#include <stddef.h>  // ptrdiff_t
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, calloc, free
#include <string.h>  // memcmp, memcpy, memset, strlen
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"      // d_fs_make_parent
#include "../../../../../../inc/djinterp/tools/dawk/dsplice.h"  // d_text_read,
                                                                // d_text_write
#include "./dmake_internal.h"                                   // scanning


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Documents
//------------------------------------------------------------------------------
// 1.1.1
// d_make_doc
//   struct: the file's path and make, its text and records, how many
//   changes it has taken and where the last one was, and why the last
//   change failed.
struct d_make_doc
{
    char*               path;
    enum d_make_dialect dialect;
    char*               text;
    size_t              length;
    struct d_make       records;
    size_t              changes;
    bool                changed;
    size_t              start;
    size_t              old_end;
    size_t              new_end;
    char                problem[320];
};

// 1.1.2
// d_internal_candidate
//   struct: a change tried: its text and records, where its splice is in
//   the old text and the new, how far it moves bytes and lines after it, and
//   the statements about it: those wholly before (`before` of them), and the
//   first wholly after in the old and in the new.
struct d_internal_candidate
{
    char*         text;
    size_t        length;
    struct d_make records;
    size_t        start;
    size_t        old_end;
    size_t        new_end;
    ptrdiff_t     shift;
    ptrdiff_t     lines;
    size_t        before;
    size_t        old_after;
    size_t        new_after;
};

// 1.1.3
// d_internal_words
//   struct: a change to a rule's words: the part, the word's index, how --
//   1 inserted, -1 removed, 0 set -- the word written, and the splice that
//   makes it, its text kept here.
struct d_internal_words
{
    enum d_make_part   part;
    size_t             word;
    int                how;
    const char*        written;
    struct d_make_span splice;
    char               text[1024];
};

// 1.1.4
// D_INTERNAL_SPANS
//   constant: how many parts a statement has.
#define D_INTERNAL_SPANS 11u

// 1.1.5
// D_INTERNAL_WORDS
//   constant: how many words of a part a change can see.
#define D_INTERNAL_WORDS 256u


//==============================================================================
// 2.  DOCUMENTS
//==============================================================================


/**
 * @brief Copies a text, terminated.
 *
 * @param[in] _text    the text.
 * @param[in] _length  its length.
 * @post   The caller frees the copy.
 * @return the copy, or `NULL` when memory runs out.
 */
static char*
d_internal_copy(
    const char* _text,
    size_t      _length
)
{
    char* const copy = malloc(_length + 1u);

    // memory that ran out
    if (!copy)
    {
        return NULL;
    }

    // the bytes, if any
    if (_length > 0u)
    {
        memcpy(copy, _text, _length);
    }

    copy[_length] = '\0';

    return copy;
}


/**
 * @brief Writes a problem, when one is asked for.
 *
 * @param[out] _problem  where; may be `NULL`.
 * @param[in]  _why      what to write.
 */
static void
d_internal_tell(
    struct d_make_problem* _problem,
    const char*            _why
)
{
    // a problem asked for
    if (_problem)
    {
        (void)snprintf(_problem->text, sizeof(_problem->text), "%s", _why);
    }

    return;
}


/**
 * @brief Makes a document of a text it takes.
 *
 * @param[in]  _path     the document's path.
 * @param[in]  _text     the text, which the document takes, or frees.
 * @param[in]  _length   its length.
 * @param[in]  _dialect  the make it is read as.
 * @param[out] _problem  receives why it does not read; may be `NULL`.
 * @return the document, or `NULL`.
 */
static struct d_make_doc*
d_internal_new(
    const char*            _path,
    char*                  _text,
    size_t                 _length,
    enum d_make_dialect    _dialect,
    struct d_make_problem* _problem
)
{
    struct d_make_doc* const doc = calloc(1u, sizeof(*doc));
    char* const              path = d_internal_copy(_path ? _path : "",
                                                    _path ? strlen(_path) : 0u);

    // memory that ran out
    if ( (!doc) ||
         (!path) )
    {
        free(doc);
        free(path);
        free(_text);
        d_internal_tell(_problem, "out of memory");

        return NULL;
    }

    doc->path    = path;
    doc->dialect = _dialect;
    doc->text    = _text;
    doc->length  = _length;

    // the records, or why there are none
    if (!d_make_read(&doc->records, doc->text, doc->length, _dialect,
                     _problem))
    {
        d_make_doc_free(doc);

        return NULL;
    }

    return doc;
}


/*
d_make_doc_read
  The file is read whole, then made a document.
*/
struct d_make_doc*
d_make_doc_read(
    const char*            _path,
    enum d_make_dialect    _dialect,
    struct d_make_problem* _problem
)
{
    size_t      length = 0;
    char* const text   = (_path) ? d_text_read(_path, &length) : NULL;

    // a file that cannot be read
    if (!text)
    {
        // a problem asked for
        if (_problem)
        {
            (void)snprintf(_problem->text,
                           sizeof(_problem->text),
                           "cannot read: %s",
                           (_path) ? _path : "");
        }

        return NULL;
    }

    return d_internal_new(_path, text, length, _dialect, _problem);
}


/*
d_make_doc_make
  The text is copied, so the caller's stays its own.
*/
struct d_make_doc*
d_make_doc_make(
    const char*            _path,
    const char*            _text,
    size_t                 _length,
    enum d_make_dialect    _dialect,
    struct d_make_problem* _problem
)
{
    char* const text = d_internal_copy(_text ? _text : "",
                                       _text ? _length : 0u);

    // memory that ran out
    if (!text)
    {
        d_internal_tell(_problem, "out of memory");

        return NULL;
    }

    return d_internal_new(_path, text, _text ? _length : 0u, _dialect,
                          _problem);
}


/*
d_make_doc_free
  The records first, as they borrow the text.
*/
void
d_make_doc_free(
    struct d_make_doc* _doc
)
{
    // nothing to free
    if (!_doc)
    {
        return;
    }

    d_make_release(&_doc->records);
    free(_doc->text);
    free(_doc->path);
    free(_doc);

    return;
}


/*
d_make_doc_records
  The records as of the last change kept.
*/
const struct d_make*
d_make_doc_records(
    const struct d_make_doc* _doc
)
{
    return (_doc) ? &_doc->records : NULL;
}


/*
d_make_doc_path
  The path it was read from or made for.
*/
const char*
d_make_doc_path(
    const struct d_make_doc* _doc
)
{
    return (_doc) ? _doc->path : "";
}


/*
d_make_doc_changes
  A count of the changes kept.
*/
size_t
d_make_doc_changes(
    const struct d_make_doc* _doc
)
{
    return (_doc) ? _doc->changes : 0u;
}


/*
d_make_doc_changed
  Where the last change kept fell, as d_internal_commit recorded it.
*/
bool
d_make_doc_changed(
    const struct d_make_doc* _doc,
    size_t*                  _out_start,
    size_t*                  _out_old_end,
    size_t*                  _out_new_end
)
{
    // no change yet
    if ( (!_doc) ||
         (!_doc->changed) )
    {
        return false;
    }

    *_out_start   = _doc->start;
    *_out_old_end = _doc->old_end;
    *_out_new_end = _doc->new_end;

    return true;
}


/*
d_make_doc_problem
  Why the last change failed, "" when it did not.
*/
const char*
d_make_doc_problem(
    const struct d_make_doc* _doc
)
{
    return (_doc) ? _doc->problem : "";
}


/*
d_make_doc_text
  The text, as of the last change kept.
*/
const char*
d_make_doc_text(
    const struct d_make_doc* _doc,
    size_t*                  _out_length
)
{
    // its length, when asked
    if (_out_length)
    {
        *_out_length = (_doc) ? _doc->length : 0u;
    }

    return (_doc) ? _doc->text : "";
}


//==============================================================================
// 3.  COMPARING RECORDS
//==============================================================================


/**
 * @brief Says why a change failed.
 *
 * @param[in,out] _doc  the document.
 * @param[in]     _why  why.
 * @return `false`.
 */
static bool
d_internal_fail(
    struct d_make_doc* _doc,
    const char*        _why
)
{
    (void)snprintf(_doc->problem, sizeof(_doc->problem), "%s", _why);

    return false;
}


/**
 * @brief Gives a place of the old text as it is in the new.
 *
 * @param[in] _at     the place.
 * @param[in] _shift  how far the splice moves what is after it.
 * @return the place moved.
 */
static size_t
d_internal_moved(
    size_t    _at,
    ptrdiff_t _shift
)
{
    return (size_t)((ptrdiff_t)_at + _shift);
}


/**
 * @brief Lists a statement's parts in one order, so two statements are
 *        compared part by part.
 *
 * @param[in]  _statement  the statement.
 * @param[out] _out        receives its D_INTERNAL_SPANS parts.
 */
static void
d_internal_spans(
    const struct d_make_statement* _statement,
    struct d_make_span*            _out
)
{
    _out[0]  = _statement->modifiers;
    _out[1]  = _statement->word;
    _out[2]  = _statement->targets;
    _out[3]  = _statement->colon;
    _out[4]  = _statement->pattern;
    _out[5]  = _statement->name;
    _out[6]  = _statement->separator;
    _out[7]  = _statement->text;
    _out[8]  = _statement->order_only;
    _out[9]  = _statement->recipe;
    _out[10] = _statement->body;

    return;
}


/**
 * @brief Gives the index among a statement's parts, as d_internal_spans
 *        lists them, of the part a change names.
 *
 * @param[in] _part  the part.
 * @return its index.
 */
static size_t
d_internal_part_index(
    enum d_make_part _part
)
{
    static const size_t indices[] = { 7u, 5u, 6u, 10u, 2u, 8u };

    return indices[_part];
}


/**
 * @brief Maps a statement's index in the old records to where it must be in
 *        the new: the same before the splice, moved after it, anywhere the
 *        change made within it.
 *
 * @param[in] _candidate  the change.
 * @param[in] _old        the index in the old records, or D_MAKE_NONE.
 * @param[in] _new        the index in the new.
 * @return `true` if the new index is where the old one must be.
 */
static bool
d_internal_mapped(
    const struct d_internal_candidate* _candidate,
    size_t                             _old,
    size_t                             _new
)
{
    // a table of where the statement stood: nowhere, before, after, within
    if (_old == D_MAKE_NONE)
    {
        return (_new == D_MAKE_NONE);
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

    return ( (_new != D_MAKE_NONE) &&
             (_new >= _candidate->before) &&
             (_new < _candidate->new_after) );
}


/**
 * @brief Reports whether a part of an old statement is the new one's: the
 *        same when it ends before the splice, moved when it starts after it.
 *
 * @param[in] _a          the old part.
 * @param[in] _b          the new part.
 * @param[in] _candidate  the change.
 * @param[in] _whole      whether the statement lies wholly after the
 *                        splice, every part then moved.
 * @return `true` if they agree, or the part meets the splice and so is the
 *         change's to judge.
 */
static bool
d_internal_same_span(
    struct d_make_span                 _a,
    struct d_make_span                 _b,
    const struct d_internal_candidate* _candidate,
    bool                               _whole
)
{
    // a table of where the part stands: after, before, within
    if ( (_whole) ||
         (_a.start >= _candidate->old_end) )
    {
        return ( (d_internal_moved(_a.start, _candidate->shift) ==
                  _b.start) &&
                 (d_internal_moved(_a.end, _candidate->shift) == _b.end) );
    }
    else if (_a.end <= _candidate->start)
    {
        return ( (_a.start == _b.start) &&
                 (_a.end == _b.end) );
    }

    return true;
}


/**
 * @brief Reports whether a statement of the old records is one of the new:
 *        its kind, place, lines, parts, depth, branch and orphan mark.
 *
 * @param[in] _doc        the document, its records the old.
 * @param[in] _candidate  the change, its records the new.
 * @param[in] _left       the old statement's index.
 * @param[in] _right      the new statement's index.
 * @param[in] _skip       the index of a part the change may alter, or
 *                        D_INTERNAL_SPANS for none.
 * @return `true` if they agree.
 *
 * A statement the splice meets is compared part by part, each by where it
 * stands, the part the change names left out.
 */
static bool
d_internal_same_statement(
    const struct d_make_doc*           _doc,
    const struct d_internal_candidate* _candidate,
    size_t                             _left,
    size_t                             _right,
    size_t                             _skip
)
{
    const struct d_make_statement* const a     =
        &_doc->records.statements[_left];
    const struct d_make_statement* const b     =
        &_candidate->records.statements[_right];
    const bool                           whole = (_left >=
                                                  _candidate->old_after);
    const ptrdiff_t                      shift = (a->start >=
                                                  _candidate->old_end)
                                                     ? _candidate->lines
                                                     : 0;
    struct d_make_span                   x[D_INTERNAL_SPANS];
    struct d_make_span                   y[D_INTERNAL_SPANS];

    const bool                           apart = ( (whole) ||
                                                   (a->end <=
                                                    _candidate->start) );

    // the statement itself, and its last line when the splice is not in it
    if ( (a->kind != b->kind) ||
         (a->depth != b->depth) ||
         (a->orphan != b->orphan) ||
         (d_internal_moved(a->line, shift) != b->line) ||
         ( (apart) &&
           (d_internal_moved(a->end_line, shift) != b->end_line) ) ||
         (!d_internal_mapped(_candidate, a->next_branch, b->next_branch)) )
    {
        return false;
    }

    const struct d_make_span outer_a = { a->start, a->end };
    const struct d_make_span outer_b = { b->start, b->end };

    // its place
    if (!d_internal_same_span(outer_a, outer_b, _candidate, whole))
    {
        return false;
    }

    d_internal_spans(a, x);
    d_internal_spans(b, y);

    // each part but the change's
    for (size_t part = 0; part < D_INTERNAL_SPANS; part++)
    {
        // a part out of place
        if ( (part != _skip) &&
             (!d_internal_same_span(x[part], y[part], _candidate, whole)) )
        {
            return false;
        }
    }

    return true;
}


/**
 * @brief Finds how the statements of both texts fall about the splice.
 *
 * @param[in]     _doc        the document, its records the old.
 * @param[in,out] _candidate  the change; receives `before`, `old_after` and
 *                            `new_after`.
 * @return `true` if the new holds as many statements wholly before and
 *         after the splice as the old.
 *
 * Wholly before is ending at or before the splice's start, and wholly
 * after starting at or after its old end; in the new, the last statements,
 * as many as wholly after in the old, stand for those, and the comparison
 * proves them where they must be. So a statement the change cut at its
 * start, which the new holds where the splice is, is the change's.
 */
static bool
d_internal_place(
    const struct d_make_doc*     _doc,
    struct d_internal_candidate* _candidate
)
{
    const struct d_make* const old   = &_doc->records;
    const struct d_make* const fresh = &_candidate->records;
    size_t                     left  = 0;

    // those wholly before the splice, in the old
    while ( (left < old->statement_count) &&
            (old->statements[left].end <= _candidate->start) )
    {
        left++;
    }

    _candidate->before = left;

    // the first wholly after it, in the old
    while ( (left < old->statement_count) &&
            (old->statements[left].start < _candidate->old_end) )
    {
        left++;
    }

    const size_t after = old->statement_count - left;

    _candidate->old_after = left;
    _candidate->new_after = (fresh->statement_count >= after)
                                ? (fresh->statement_count - after)
                                : 0u;

    return (fresh->statement_count >= _candidate->before + after);
}


/**
 * @brief Reports whether every statement outside the splice is the file's
 *        own, and gives the line of the first that is not.
 *
 * @param[in]  _doc        the document.
 * @param[in]  _candidate  the change, placed.
 * @param[out] _out_line   receives the line of a statement changed.
 * @return `true` if all are as they were.
 */
static bool
d_internal_outside_statements(
    const struct d_make_doc*           _doc,
    const struct d_internal_candidate* _candidate,
    size_t*                            _out_line
)
{
    const size_t after = _doc->records.statement_count -
                         _candidate->old_after;

    // those before, then those after
    for (size_t at = 0; at < _candidate->before + after; at++)
    {
        const size_t left  = (at < _candidate->before)
                                 ? at
                                 : (_candidate->old_after + at -
                                    _candidate->before);
        const size_t right = (at < _candidate->before)
                                 ? at
                                 : (_candidate->new_after + at -
                                    _candidate->before);

        // the same, where it was or moved
        if (!d_internal_same_statement(_doc, _candidate, left, right,
                                       D_INTERNAL_SPANS))
        {
            *_out_line = _doc->records.statements[left].line;

            return false;
        }
    }

    return true;
}


/**
 * @brief Reports whether every comment outside the splice is the file's
 *        own: those before it the same, those after it moved, as many of
 *        each, each with its statement.
 *
 * @param[in]  _doc        the document.
 * @param[in]  _candidate  the change, placed.
 * @param[out] _out_line   receives the line of a comment changed.
 * @return `true` if all are as they were.
 *
 * Long as the two lists are walked side by side through the three places,
 * each a short loop.
 */
static bool
d_internal_outside_comments(
    const struct d_make_doc*           _doc,
    const struct d_internal_candidate* _candidate,
    size_t*                            _out_line
)
{
    const struct d_make* const old       = &_doc->records;
    const struct d_make* const fresh     = &_candidate->records;
    size_t                     left      = 0;
    size_t                     old_after = 0;
    size_t                     new_after = 0;

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

    // ...and in the new, the last as many as after it in the old
    new_after = ( (fresh->comment_count >= old->comment_count - old_after) )
                    ? (fresh->comment_count - (old->comment_count -
                                               old_after))
                    : 0u;

    // as many before, and as many after
    if ( (left + (old->comment_count - old_after)) > fresh->comment_count)
    {
        *_out_line = (left < old->comment_count) ? old->comments[left].line
                                                 : 1u;

        return false;
    }

    // each before, then each after
    for (size_t at = 0; at < old->comment_count; at++)
    {
        // one the splice cut is the change's
        if ( (at >= left) &&
             (at < old_after) )
        {
            continue;
        }

        const bool                         after = (at >= old_after);
        const struct d_make_comment* const a     = &old->comments[at];
        const struct d_make_comment* const b     =
            &fresh->comments[(after) ? (at - old_after + new_after) : at];
        const ptrdiff_t                    shift = (after) ? _candidate->shift
                                                           : 0;
        const ptrdiff_t                    lines = (after) ? _candidate->lines
                                                           : 0;

        // the same place and line, and its statement
        if ( (d_internal_moved(a->start, shift) != b->start) ||
             (d_internal_moved(a->end, shift) != b->end) ||
             (d_internal_moved(a->line, lines) != b->line) ||
             (!d_internal_mapped(_candidate, a->within, b->within)) )
        {
            *_out_line = a->line;

            return false;
        }
    }

    return true;
}


//==============================================================================
// 4.  CANDIDATES
//==============================================================================


/**
 * @brief Counts the line feeds of a text.
 *
 * @param[in] _text    the text.
 * @param[in] _length  its length.
 * @return how many.
 */
static ptrdiff_t
d_internal_count_lines(
    const char* _text,
    size_t      _length
)
{
    ptrdiff_t count = 0;

    // each line feed
    for (size_t at = 0; at < _length; at++)
    {
        count += (_text[at] == '\n') ? 1 : 0;
    }

    return count;
}


/**
 * @brief Releases a candidate's text and records.
 *
 * @param[in,out] _candidate  the candidate, left empty.
 */
static void
d_internal_discard(
    struct d_internal_candidate* _candidate
)
{
    d_make_release(&_candidate->records);
    free(_candidate->text);
    memset(_candidate, 0, sizeof(*_candidate));

    return;
}


/**
 * @brief Builds a candidate's text: the file before the splice, the text
 *        given with the file's line endings, the file after.
 *
 * @param[in,out] _doc        the document.
 * @param[in]     _start      the splice's first byte.
 * @param[in]     _end        past its last.
 * @param[in]     _text       what replaces them.
 * @param[out]    _candidate  receives the text, its splice and its shift.
 * @return `true`; `false` when memory runs out.
 *
 * In a file whose first line ends in CR LF, each line feed of the text not
 * already after a CR gets one.
 */
static bool
d_internal_build(
    struct d_make_doc*           _doc,
    size_t                       _start,
    size_t                       _end,
    const char*                  _text,
    struct d_internal_candidate* _candidate
)
{
    const size_t length = strlen(_text);
    const bool   crlf   = _doc->records.lines.crlf;
    size_t       added  = 0;

    // the CRs a CR LF file adds to the text
    for (size_t at = 0; (crlf) && (at < length); at++)
    {
        added += ( (_text[at] == '\n') &&
                   ( (at == 0u) ||
                     (_text[at - 1u] != '\r') ) ) ? 1u : 0u;
    }

    const size_t written = length + added;
    char* const  out     = malloc(_start + written + (_doc->length - _end) +
                                  1u);

    // memory that ran out
    if (!out)
    {
        return d_internal_fail(_doc, "out of memory");
    }

    size_t at = _start;

    memcpy(out, _doc->text, _start);

    // the text, its line endings the file's
    for (size_t index = 0; index < length; index++)
    {
        // a CR before a lone line feed
        if ( (crlf) &&
             (_text[index] == '\n') &&
             ( (index == 0u) ||
               (_text[index - 1u] != '\r') ) )
        {
            out[at++] = '\r';
        }

        out[at++] = _text[index];
    }

    memcpy(out + at, _doc->text + _end, _doc->length - _end);
    at += _doc->length - _end;
    out[at] = '\0';

    _candidate->text    = out;
    _candidate->length  = at;
    _candidate->start   = _start;
    _candidate->old_end = _end;
    _candidate->new_end = _start + written;
    _candidate->shift   = (ptrdiff_t)written - (ptrdiff_t)(_end - _start);
    _candidate->lines   = d_internal_count_lines(out + _start, written) -
                          d_internal_count_lines(_doc->text + _start,
                                                 _end - _start);

    return true;
}


/**
 * @brief Tries a splice: builds the candidate, reads it, and checks that
 *        everything outside the splice reads as it did.
 *
 * @param[in,out] _doc        the document.
 * @param[in]     _start      the splice's first byte.
 * @param[in]     _end        past its last.
 * @param[in]     _text       what replaces them.
 * @param[out]    _candidate  receives the candidate, to be committed or
 *                            discarded.
 * @return `true` if the candidate holds; `false` otherwise, the document's
 *         problem saying why and the candidate discarded.
 *
 * Long as the trial's four steps, each a few lines: the splice's bounds,
 * the text built, the text read, and the records outside compared.
 */
static bool
d_internal_try(
    struct d_make_doc*           _doc,
    size_t                       _start,
    size_t                       _end,
    const char*                  _text,
    struct d_internal_candidate* _candidate
)
{
    struct d_make_problem why;
    size_t                line = 0;

    memset(_candidate, 0, sizeof(*_candidate));
    _doc->problem[0] = '\0';

    // a splice within the file
    if ( (_start > _end) ||
         (_end > _doc->length) )
    {
        return d_internal_fail(_doc, "the change is not within the file");
    }

    // the candidate's text
    if (!d_internal_build(_doc, _start, _end, _text, _candidate))
    {
        return false;
    }

    // the new text read, or the change refused
    if (!d_make_read(&_candidate->records,
                     _candidate->text,
                     _candidate->length,
                     _doc->dialect,
                     &why))
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the change would leave the file unreadable: %s",
                       why.text);
        d_internal_discard(_candidate);

        return false;
    }

    // the statements outside the splice as they were
    if ( (!d_internal_place(_doc, _candidate)) ||
         (!d_internal_outside_statements(_doc, _candidate, &line)) )
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the change would change the statement at line %lu, "
                       "outside it",
                       (unsigned long)((line > 0u) ? line : 1u));
        d_internal_discard(_candidate);

        return false;
    }

    // and the comments
    if (!d_internal_outside_comments(_doc, _candidate, &line))
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the change would change the comment at line %lu, "
                       "outside it",
                       (unsigned long)line);
        d_internal_discard(_candidate);

        return false;
    }

    return true;
}


/**
 * @brief Keeps a candidate: its text and records become the document's.
 *
 * @param[in,out] _doc        the document.
 * @param[in,out] _candidate  the candidate, emptied.
 * @return `true`.
 */
static bool
d_internal_commit(
    struct d_make_doc*           _doc,
    struct d_internal_candidate* _candidate
)
{
    d_make_release(&_doc->records);
    free(_doc->text);

    _doc->text       = _candidate->text;
    _doc->length     = _candidate->length;
    _doc->records    = _candidate->records;
    _doc->changed    = true;
    _doc->start      = _candidate->start;
    _doc->old_end    = _candidate->old_end;
    _doc->new_end    = _candidate->new_end;
    _doc->problem[0] = '\0';
    _doc->changes++;
    memset(_candidate, 0, sizeof(*_candidate));

    return true;
}


/**
 * @brief Refuses a candidate that does not read back as asked, and
 *        discards it.
 *
 * @param[in,out] _doc        the document.
 * @param[in,out] _candidate  the candidate, discarded.
 * @param[in]     _why        why.
 * @return `false`.
 */
static bool
d_internal_refuse(
    struct d_make_doc*           _doc,
    struct d_internal_candidate* _candidate,
    const char*                  _why
)
{
    d_internal_discard(_candidate);

    return d_internal_fail(_doc, _why);
}


/*
d_make_doc_splice
  A splice is tried as any change is; nothing more is asked of it.
*/
bool
d_make_doc_splice(
    struct d_make_doc* _doc,
    size_t             _start,
    size_t             _end,
    const char*        _text,
    size_t             _length
)
{
    struct d_internal_candidate candidate;

    // parameter validation first
    if (!_doc)
    {
        return false;
    }

    char* const text = d_internal_copy(_text ? _text : "",
                                       _text ? _length : 0u);

    // memory that ran out, or a text with a NUL, which no make reads
    if ( (!text) ||
         (strlen(text) != ((_text) ? _length : 0u)) )
    {
        free(text);

        return d_internal_fail(_doc, (text) ? "the text holds a NUL byte"
                                            : "out of memory");
    }

    const bool kept = d_internal_try(_doc, _start, _end, text, &candidate);

    free(text);

    return (kept) ? d_internal_commit(_doc, &candidate) : false;
}


//==============================================================================
// 5.  PARTS
//==============================================================================


/**
 * @brief Finds a statement the file has.
 *
 * @param[in,out] _doc        the document.
 * @param[in]     _statement  its index.
 * @return the statement, or `NULL`, the document's problem saying why.
 */
static const struct d_make_statement*
d_internal_statement(
    struct d_make_doc* _doc,
    size_t             _statement
)
{
    // one the file has
    if (_statement >= _doc->records.statement_count)
    {
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the file has no statement %lu",
                       (unsigned long)(_statement + 1u));

        return NULL;
    }

    return &_doc->records.statements[_statement];
}


/**
 * @brief Reports whether a statement has a part a change may set.
 *
 * @param[in] _statement  the statement.
 * @param[in] _part       the part.
 * @return `true` if it has the part.
 */
static bool
d_internal_has_part(
    const struct d_make_statement* _statement,
    enum d_make_part               _part
)
{
    const enum d_make_kind kind = _statement->kind;

    // a table of the parts, by the kinds that have them
    if (_part == D_MAKE_PART_TEXT)
    {
        return (kind != D_MAKE_DEFINE);
    }
    else if (_part == D_MAKE_PART_NAME)
    {
        return ( (kind == D_MAKE_ASSIGNMENT) ||
                 (kind == D_MAKE_DEFINE) ||
                 (_statement->name.end > _statement->name.start) );
    }
    else if (_part == D_MAKE_PART_SEPARATOR)
    {
        return ( (kind == D_MAKE_ASSIGNMENT) ||
                 ( (kind == D_MAKE_DEFINE) &&
                   (_statement->separator.end >
                    _statement->separator.start) ) );
    }
    else if (_part == D_MAKE_PART_BODY)
    {
        return ( (kind == D_MAKE_DEFINE) ||
                 (kind == D_MAKE_LOOP) );
    }
    else if (_part == D_MAKE_PART_TARGETS)
    {
        return (_statement->colon.end > _statement->colon.start);
    }

    return (kind == D_MAKE_RULE);
}


/**
 * @brief Reports whether the change made one statement of one: a single
 *        statement cut, a single one in its place, of the old one's kind.
 *
 * @param[in] _doc        the document.
 * @param[in] _candidate  the change, placed.
 * @param[in] _statement  the old statement's index.
 * @return `true` if so.
 */
static bool
d_internal_one_for_one(
    const struct d_make_doc*           _doc,
    const struct d_internal_candidate* _candidate,
    size_t                             _statement
)
{
    return ( (_candidate->before == _statement) &&
             (_candidate->old_after == _statement + 1u) &&
             (_candidate->new_after == _statement + 1u) &&
             (_candidate->records.statements[_statement].kind ==
              _doc->records.statements[_statement].kind) );
}


/**
 * @brief Reports whether a part of the new statement holds just the text
 *        the change wrote.
 *
 * @param[in] _candidate  the change, placed.
 * @param[in] _statement  the statement's index.
 * @param[in] _index      the part's index, as d_internal_spans lists it.
 * @return `true` if the part is the splice's new text, exactly.
 */
static bool
d_internal_holds_change(
    const struct d_internal_candidate* _candidate,
    size_t                             _statement,
    size_t                             _index
)
{
    struct d_make_span spans[D_INTERNAL_SPANS];

    d_internal_spans(&_candidate->records.statements[_statement], spans);

    return ( (spans[_index].start == _candidate->start) &&
             (spans[_index].end == _candidate->new_end) );
}


/*
d_make_doc_set
  The part's place is spliced, and the candidate kept when one statement of
the kind stands for one, its part the text written, its other parts where
they were or moved. A body's text gets the line feed that ends its last
line, and an operator must be one the make reads, which the reading proves.
  Long as the part's checks before the splice and the change's after it,
each a few lines.
*/
bool
d_make_doc_set(
    struct d_make_doc* _doc,
    size_t             _statement,
    enum d_make_part   _part,
    const char*        _text
)
{
    // parameter validation first
    if ( (!_doc) ||
         (!_text) ||
         ((size_t)_part > (size_t)D_MAKE_PART_ORDER_ONLY) )
    {
        return (_doc) ? d_internal_fail(_doc, "no such part") : false;
    }

    const struct d_make_statement* const statement =
        d_internal_statement(_doc, _statement);

    // a statement the file has, with the part
    if ( (!statement) ||
         (!d_internal_has_part(statement, _part)) )
    {
        return (statement) ? d_internal_fail(_doc, "the statement has no "
                                                   "such part")
                           : false;
    }

    struct d_make_span spans[D_INTERNAL_SPANS];
    const size_t       index  = d_internal_part_index(_part);
    const size_t       length = strlen(_text);
    const bool         ended  = ( (_part != D_MAKE_PART_BODY) ||
                                  (length == 0u) ||
                                  (_text[length - 1u] == '\n') );
    char* const        text   = malloc(length + 2u);

    // memory that ran out
    if (!text)
    {
        return d_internal_fail(_doc, "out of memory");
    }

    memcpy(text, _text, length);
    text[length]                     = '\n';
    text[length + (ended ? 0u : 1u)] = '\0';
    d_internal_spans(statement, spans);

    struct d_internal_candidate candidate;
    const bool                  tried = d_internal_try(_doc,
                                                       spans[index].start,
                                                       spans[index].end,
                                                       text,
                                                       &candidate);

    free(text);

    // refused outside the change
    if (!tried)
    {
        return false;
    }

    // one statement for one, the part the text, the rest as it was
    if ( (!d_internal_one_for_one(_doc, &candidate, _statement)) ||
         (!d_internal_holds_change(&candidate, _statement, index)) ||
         (!d_internal_same_statement(_doc, &candidate, _statement,
                                     _statement, index)) )
    {
        return d_internal_refuse(_doc, &candidate, "the change does not read "
                                                   "back as asked");
    }

    return d_internal_commit(_doc, &candidate);
}


//==============================================================================
// 6.  WORDS
//==============================================================================


/**
 * @brief Finds a rule's part that is a list of words.
 *
 * @param[in,out] _doc        the document.
 * @param[in]     _statement  the rule's index.
 * @param[in]     _part       the part: targets, prerequisites or
 *                            order-only prerequisites.
 * @param[out]    _out        receives the part's place.
 * @return `true` if the statement is a rule with the part; `false`
 *         otherwise, the document's problem saying why.
 */
static bool
d_internal_word_part(
    struct d_make_doc*  _doc,
    size_t              _statement,
    enum d_make_part    _part,
    struct d_make_span* _out
)
{
    const struct d_make_statement* const statement =
        d_internal_statement(_doc, _statement);

    // a statement the file has
    if (!statement)
    {
        return false;
    }

    // a rule
    if (statement->kind != D_MAKE_RULE)
    {
        return d_internal_fail(_doc, "the statement is no rule");
    }

    // a table of the parts that are words
    if (_part == D_MAKE_PART_TARGETS)
    {
        *_out = statement->targets;
    }
    else if (_part == D_MAKE_PART_TEXT)
    {
        *_out = statement->text;
    }
    else if (_part == D_MAKE_PART_ORDER_ONLY)
    {
        *_out = statement->order_only;
    }
    else
    {
        return d_internal_fail(_doc, "the part is no list of words");
    }

    return true;
}


/**
 * @brief Gives a part's place in a statement, as d_internal_spans lists it.
 *
 * @param[in] _statement  the statement.
 * @param[in] _part       the part.
 * @return its place.
 */
static struct d_make_span
d_internal_part_of(
    const struct d_make_statement* _statement,
    enum d_make_part               _part
)
{
    struct d_make_span spans[D_INTERNAL_SPANS];

    d_internal_spans(_statement, spans);

    return spans[d_internal_part_index(_part)];
}


/**
 * @brief Reports whether the new rule's words of a part are the old words
 *        changed as asked.
 *
 * @param[in] _doc        the document.
 * @param[in] _candidate  the change, placed.
 * @param[in] _statement  the rule's index.
 * @param[in] _change     what was asked.
 * @return `true` if the words agree: each new word the one written, or the
 *         old word it stands for.
 *
 * Long as the two lists of words are read, then walked side by side.
 */
static bool
d_internal_words_back(
    const struct d_make_doc*           _doc,
    const struct d_internal_candidate* _candidate,
    size_t                             _statement,
    const struct d_internal_words*     _change
)
{
    struct d_make_span old_words[D_INTERNAL_WORDS];
    struct d_make_span new_words[D_INTERNAL_WORDS];
    const size_t       old_count = d_make_words(
                                       &_doc->records,
                                       d_internal_part_of(
                                           &_doc->records
                                                .statements[_statement],
                                           _change->part),
                                       old_words,
                                       D_INTERNAL_WORDS);
    const size_t       new_count = d_make_words(
                                       &_candidate->records,
                                       d_internal_part_of(
                                           &_candidate->records
                                                .statements[_statement],
                                           _change->part),
                                       new_words,
                                       D_INTERNAL_WORDS);
    const size_t       length    = strlen(_change->written);

    // as many words as asked, all of them seen
    if ( ((long)new_count != (long)old_count + _change->how) ||
         (old_count > D_INTERNAL_WORDS) ||
         (new_count > D_INTERNAL_WORDS) )
    {
        return false;
    }

    // each new word: the one written, or the old one it stands for
    for (size_t at = 0; at < new_count; at++)
    {
        const struct d_make_span word = new_words[at];
        const size_t             size = word.end - word.start;
        const bool               own  = ( (_change->how >= 0) &&
                                          (at == _change->word) );
        const size_t             from = ( (_change->how > 0) &&
                                          (at > _change->word) )
                                            ? (at - 1u)
                                            : ( ( (_change->how < 0) &&
                                                  (at >= _change->word) )
                                                    ? (at + 1u)
                                                    : at );

        // a table of the two: the word written, or the old word
        if ( (own) &&
             ( (size != length) ||
               (memcmp(_candidate->text + word.start, _change->written,
                       size) != 0) ) )
        {
            return false;
        }
        else if ( (!own) &&
                  ( (old_words[from].end - old_words[from].start != size) ||
                    (memcmp(_doc->text + old_words[from].start,
                            _candidate->text + word.start,
                            size) != 0) ) )
        {
            return false;
        }
    }

    return true;
}


/**
 * @brief Tries a change to a rule's words and keeps it when the words read
 *        back as asked and the rule's other parts are as they were.
 *
 * @param[in,out] _doc        the document.
 * @param[in]     _statement  the rule's index.
 * @param[in]     _change     what was asked, and its splice.
 * @return `true` if the change was kept.
 */
static bool
d_internal_word_change(
    struct d_make_doc*             _doc,
    size_t                         _statement,
    const struct d_internal_words* _change
)
{
    struct d_internal_candidate candidate;

    // refused outside the change
    if (!d_internal_try(_doc, _change->splice.start, _change->splice.end,
                        _change->text, &candidate))
    {
        return false;
    }

    // one rule for one, its words as asked, the rest as it was
    if ( (!d_internal_one_for_one(_doc, &candidate, _statement)) ||
         (!d_internal_words_back(_doc, &candidate, _statement, _change)) ||
         (!d_internal_same_statement(_doc, &candidate, _statement,
                                     _statement,
                                     d_internal_part_index(_change->part))) )
    {
        return d_internal_refuse(_doc, &candidate, "the change does not read "
                                                   "back as asked");
    }

    return d_internal_commit(_doc, &candidate);
}


/*
d_make_doc_set_word
  The word's place is spliced; nothing about it moves.
*/
bool
d_make_doc_set_word(
    struct d_make_doc* _doc,
    size_t             _statement,
    enum d_make_part   _part,
    size_t             _word,
    const char*        _text
)
{
    struct d_make_span      part;
    struct d_make_span      words[D_INTERNAL_WORDS];
    struct d_internal_words change;

    // parameter validation first
    if ( (!_doc) ||
         (!_text) ||
         (!d_internal_word_part(_doc, _statement, _part, &part)) )
    {
        return false;
    }

    const size_t count = d_make_words(&_doc->records, part, words,
                                      D_INTERNAL_WORDS);

    // a word the part has, and a text that fits
    if ( (_word >= count) ||
         (_word >= D_INTERNAL_WORDS) ||
         (strlen(_text) >= sizeof(change.text)) )
    {
        return d_internal_fail(_doc, "the part has no such word");
    }

    change.part    = _part;
    change.word    = _word;
    change.how     = 0;
    change.written = _text;
    change.splice  = words[_word];
    (void)snprintf(change.text, sizeof(change.text), "%s", _text);

    return d_internal_word_change(_doc, _statement, &change);
}


/**
 * @brief Reports whether a byte of a file is a blank of a line.
 *
 * @param[in] _byte  the byte.
 * @return `true` for a space or a tab.
 */
static bool
d_internal_is_space(
    char _byte
)
{
    return ( (_byte == ' ') ||
             (_byte == '\t') );
}


/**
 * @brief Finds where an inserted word goes, and writes the text that
 *        inserts it.
 *
 * @param[in,out] _doc     the document.
 * @param[in]     _rule    the rule.
 * @param[in]     _words   the part's words.
 * @param[in]     _count   how many.
 * @param[in,out] _change  the word and its place; receives the splice and
 *                         its text.
 * @return `true`; `false` when the word cannot be written there, the
 *         document's problem saying why.
 *
 * Long as each of the four places a word can go is written its own way:
 * among words, after the last, into an empty part, and as the first
 * order-only word, its `|` written before it.
 */
static bool
d_internal_word_place(
    struct d_make_doc*             _doc,
    const struct d_make_statement* _rule,
    const struct d_make_span*      _words,
    size_t                         _count,
    struct d_internal_words*       _change
)
{
    const char* const file  = _doc->text;
    const char* const word  = _change->written;
    const size_t      room  = sizeof(_change->text);
    size_t            where = 0;

    // a table of the places: among the words, after the last, the first
    // order-only word, an empty part
    if (_change->word < _count)
    {
        where = _words[_change->word].start;
        (void)snprintf(_change->text, room, "%s ", word);
    }
    else if (_count > 0u)
    {
        where = _words[_count - 1u].end;
        (void)snprintf(_change->text, room, " %s", word);
    }
    else if ( (_change->part == D_MAKE_PART_ORDER_ONLY) &&
              (_doc->dialect != D_MAKE_GNU) )
    {
        return d_internal_fail(_doc, "only GNU make reads order-only "
                                     "prerequisites");
    }
    else
    {
        const bool   order = (_change->part == D_MAKE_PART_ORDER_ONLY);
        const size_t at    = (order)
                                 ? _rule->text.end
                                 : ( (_change->part == D_MAKE_PART_TEXT)
                                         ? _rule->text.start
                                         : _rule->targets.start );
        const bool   blank = ( (at > 0u) &&
                               (d_internal_is_space(file[at - 1u])) );
        const bool   more  = ( (at < _rule->end) &&
                               (file[at] != '\n') &&
                               (file[at] != '\r') &&
                               (!d_internal_is_space(file[at])) );

        where = at;
        (void)snprintf(_change->text, room, "%s%s%s%s", blank ? "" : " ",
                       order ? "| " : "", word, more ? " " : "");
    }

    _change->splice.start = where;
    _change->splice.end   = where;

    return true;
}


/*
d_make_doc_insert_word
  The word and the space that parts it from its neighbour are written at
the place d_internal_word_place finds.
*/
bool
d_make_doc_insert_word(
    struct d_make_doc* _doc,
    size_t             _statement,
    enum d_make_part   _part,
    size_t             _place,
    const char*        _text
)
{
    struct d_make_span      part;
    struct d_make_span      words[D_INTERNAL_WORDS];
    struct d_internal_words change;

    // parameter validation first
    if ( (!_doc) ||
         (!_text) ||
         (!d_internal_word_part(_doc, _statement, _part, &part)) )
    {
        return false;
    }

    const size_t count = d_make_words(&_doc->records, part, words,
                                      D_INTERNAL_WORDS);

    // a place the part has, and a word that fits
    if ( (_place > count) ||
         (count >= D_INTERNAL_WORDS) ||
         (strlen(_text) + 4u >= sizeof(change.text)) )
    {
        return d_internal_fail(_doc, "the part has no such place");
    }

    change.part    = _part;
    change.word    = _place;
    change.how     = 1;
    change.written = _text;

    // where it goes, and how it is written
    if (!d_internal_word_place(_doc,
                               &_doc->records.statements[_statement],
                               words,
                               count,
                               &change))
    {
        return false;
    }

    return d_internal_word_change(_doc, _statement, &change);
}


/*
d_make_doc_remove_word
  A word goes with the blanks before it, or, the first of several, with the
blanks after it; the last order-only word takes its `|` with it, and the
only prerequisite the blanks before it.
*/
bool
d_make_doc_remove_word(
    struct d_make_doc* _doc,
    size_t             _statement,
    enum d_make_part   _part,
    size_t             _word
)
{
    struct d_make_span      part;
    struct d_make_span      words[D_INTERNAL_WORDS];
    struct d_internal_words change;

    // parameter validation first
    if ( (!_doc) ||
         (!d_internal_word_part(_doc, _statement, _part, &part)) )
    {
        return false;
    }

    const struct d_make_statement* const rule  =
        &_doc->records.statements[_statement];
    const size_t                         count = d_make_words(
                                                     &_doc->records,
                                                     part,
                                                     words,
                                                     D_INTERNAL_WORDS);

    // a table of the refusals: a word the part has not, the last target
    if ( (_word >= count) ||
         (_word >= D_INTERNAL_WORDS) )
    {
        return d_internal_fail(_doc, "the part has no such word");
    }
    else if ( (_part == D_MAKE_PART_TARGETS) &&
              (count == 1u) )
    {
        return d_internal_fail(_doc, "a rule keeps one target at least");
    }

    change.part     = _part;
    change.word     = _word;
    change.how      = -1;
    change.written  = "";
    change.text[0]  = '\0';
    change.splice   = words[_word];

    // a table of the cuts: after the first, the first of several, the last
    // order-only word with its `|`, the only one with the blanks before it
    if (_word > 0u)
    {
        change.splice.start = words[_word - 1u].end;
    }
    else if (count > 1u)
    {
        change.splice.end = words[1].start;
    }
    else if (_part == D_MAKE_PART_ORDER_ONLY)
    {
        change.splice.start = rule->text.end;
    }
    else
    {
        change.splice.start = d_make_trim_end(_doc->text, rule->start,
                                              change.splice.start);
    }

    return d_internal_word_change(_doc, _statement, &change);
}


//==============================================================================
// 7.  COMMANDS
//==============================================================================


/**
 * @brief Counts a rule's commands: the recipe's lines straight after it.
 *
 * @param[in] _doc   the document.
 * @param[in] _rule  the rule's index.
 * @return how many.
 */
static size_t
d_internal_commands(
    const struct d_make_doc* _doc,
    size_t                   _rule
)
{
    size_t count = 0;

    // each recipe line after the rule
    while ( (_rule + 1u + count < _doc->records.statement_count) &&
            (_doc->records.statements[_rule + 1u + count].kind ==
             D_MAKE_RECIPE) &&
            (!_doc->records.statements[_rule + 1u + count].orphan) )
    {
        count++;
    }

    return count;
}


/*
d_make_doc_add_command
  The line is written with the prefix the rule's first command has -- its
first byte, or for BSD and NMake the blanks that lead it -- or a tab; a
file whose last line no line feed ends gets one first. The candidate is
kept when one recipe line more stands under the rule, at the place asked,
its command the text.
  Long as the place, the prefix, the text and the check, each a few lines.
*/
bool
d_make_doc_add_command(
    struct d_make_doc* _doc,
    size_t             _rule,
    size_t             _place,
    const char*        _text
)
{
    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const struct d_make_statement* const rule =
        d_internal_statement(_doc, _rule);

    // a rule the file has
    if ( (!rule) ||
         (rule->kind != D_MAKE_RULE) )
    {
        return (rule) ? d_internal_fail(_doc, "the statement is no rule")
                      : false;
    }

    const size_t count = d_internal_commands(_doc, _rule);

    // a place the rule has
    if (_place > count)
    {
        return d_internal_fail(_doc, "the rule has no such place");
    }

    const struct d_make_statement* const first =
        (count > 0u) ? &_doc->records.statements[_rule + 1u] : NULL;
    const size_t                         where =
        (_place < count) ? _doc->records.statements[_rule + 1u + _place].start
        : (count > 0u)   ? _doc->records.statements[_rule + count].end
                         : rule->end;
    const bool                           blanks =
        ( (_doc->dialect == D_MAKE_NMAKE) ||
          (_doc->dialect == D_MAKE_BSD) );
    const size_t                         lead  =
        (!first)   ? 0u
        : (blanks) ? (d_make_skip_blanks(_doc->text, first->start,
                                         first->end) - first->start)
                   : 1u;
    const bool                           needs = ( (where == _doc->length) &&
                                                   (where > 0u) &&
                                                   (_doc->text[where - 1u] !=
                                                    '\n') );
    const size_t                         length = strlen(_text);
    char* const                          text  = malloc(length + lead + 4u);

    // memory that ran out
    if (!text)
    {
        return d_internal_fail(_doc, "out of memory");
    }

    size_t at = 0;

    text[at] = '\n';
    at      += needs ? 1u : 0u;

    // the prefix, the first command's or a tab
    if (first)
    {
        memcpy(text + at, _doc->text + first->start, lead);
        at += lead;
    }
    else
    {
        text[at++] = '\t';
    }

    memcpy(text + at, _text, length);
    at += length;
    text[at++] = '\n';
    text[at]   = '\0';

    struct d_internal_candidate candidate;
    const size_t                index = _rule + 1u + _place;
    const bool                  tried = d_internal_try(_doc, where, where,
                                                       text, &candidate);

    free(text);

    // refused outside the change
    if (!tried)
    {
        return false;
    }

    const struct d_make_statement* const added =
        (index < candidate.records.statement_count)
            ? &candidate.records.statements[index]
            : NULL;

    // one recipe line more, under the rule, its command the text
    if ( (candidate.old_after != candidate.before) ||
         (candidate.new_after != candidate.before + 1u) ||
         (candidate.before != index) ||
         (!added) ||
         (added->kind != D_MAKE_RECIPE) ||
         (added->orphan) ||
         (added->text.end - added->text.start != length) ||
         (memcmp(candidate.text + added->text.start, _text, length) != 0) )
    {
        return d_internal_refuse(_doc, &candidate, "the change does not read "
                                                   "back as asked");
    }

    return d_internal_commit(_doc, &candidate);
}


/*
d_make_doc_remove_command
  The recipe line goes with its line; the candidate is kept when it alone
went.
*/
bool
d_make_doc_remove_command(
    struct d_make_doc* _doc,
    size_t             _statement
)
{
    // parameter validation first
    if (!_doc)
    {
        return false;
    }

    const struct d_make_statement* const command =
        d_internal_statement(_doc, _statement);

    // a recipe line the file has
    if ( (!command) ||
         (command->kind != D_MAKE_RECIPE) )
    {
        return (command) ? d_internal_fail(_doc, "the statement is no "
                                                 "command")
                         : false;
    }

    struct d_internal_candidate candidate;

    // refused outside the change
    if (!d_internal_try(_doc, command->start, command->end, "", &candidate))
    {
        return false;
    }

    // it alone went
    if ( (candidate.before != _statement) ||
         (candidate.old_after != _statement + 1u) ||
         (candidate.new_after != _statement) )
    {
        return d_internal_refuse(_doc, &candidate, "the change does not read "
                                                   "back as asked");
    }

    return d_internal_commit(_doc, &candidate);
}


//==============================================================================
// 8.  STATEMENTS
//==============================================================================


/**
 * @brief Writes statements' text with a line feed ending it, and one before
 *        it where it goes past a last line no line feed ends.
 *
 * @param[in] _doc    the document.
 * @param[in] _where  where it goes.
 * @param[in] _text   the text.
 * @post   The caller frees the text.
 * @return the text, or `NULL` when memory runs out.
 */
static char*
d_internal_lines_text(
    const struct d_make_doc* _doc,
    size_t                   _where,
    const char*              _text
)
{
    const size_t length = strlen(_text);
    const bool   before = ( (_where == _doc->length) &&
                            (_where > 0u) &&
                            (_doc->text[_where - 1u] != '\n') );
    const bool   after  = ( (length > 0u) &&
                            (_text[length - 1u] != '\n') );
    char* const  text   = malloc(length + 3u);

    // memory that ran out
    if (!text)
    {
        return NULL;
    }

    size_t at = 0;

    text[at] = '\n';
    at      += before ? 1u : 0u;
    memcpy(text + at, _text, length);
    at      += length;
    text[at] = '\n';
    at      += after ? 1u : 0u;
    text[at] = '\0';

    return text;
}


/**
 * @brief Reports whether the statements the change wrote read as the text
 *        does alone: as many, each over the same bytes, each of its kind.
 *
 * @param[in] _doc        the document.
 * @param[in] _candidate  the change, placed.
 * @param[in] _text       the text written, as spliced in.
 * @return `true` if they agree.
 *
 * A line led by the recipe prefix reads as a recipe's only under a rule, so
 * the orphan mark is left out, and a recipe's line read alone may be read in
 * place as any statement that line makes in its context.
 */
static bool
d_internal_as_alone(
    const struct d_make_doc*           _doc,
    const struct d_internal_candidate* _candidate,
    const char*                        _text
)
{
    struct d_make      alone;
    const size_t       length = strlen(_text);
    const size_t       offset = _candidate->start;
    bool               same   = d_make_read(&alone, _text, length,
                                            _doc->dialect, NULL);

    same = ( (same) &&
             (alone.statement_count ==
              _candidate->new_after - _candidate->before) );

    // each statement written, against its reading alone
    for (size_t at = 0; (same) && (at < alone.statement_count); at++)
    {
        const struct d_make_statement* const a = &alone.statements[at];
        const struct d_make_statement* const b =
            &_candidate->records.statements[_candidate->before + at];

        same = ( (a->start + offset == b->start) &&
                 (a->end + offset == b->end) &&
                 ( (a->kind == b->kind) ||
                   (a->orphan) ) );
    }

    d_make_release(&alone);

    return same;
}


/**
 * @brief Finds where statements inserted before a statement go: above the
 *        comment lines that lead into it.
 *
 * @param[in] _doc     the document.
 * @param[in] _before  the statement's index, or the count to append.
 * @return the place.
 */
static size_t
d_internal_lead(
    const struct d_make_doc* _doc,
    size_t                   _before
)
{
    const struct d_make* const records = &_doc->records;

    // after the last
    if (_before >= records->statement_count)
    {
        return _doc->length;
    }

    const size_t floor = (_before > 0u)
                             ? records->statements[_before - 1u].end
                             : 0u;
    size_t       at    = records->statements[_before].start;

    // each comment line just above, from the nearest
    for (size_t index = records->comment_count; index > 0u; index--)
    {
        const struct d_make_comment* const comment =
            &records->comments[index - 1u];
        const size_t                       gap     = at - comment->end;
        size_t                             line    = comment->start;

        // a comment past the place
        if (comment->end > at)
        {
            continue;
        }

        // the start of its line
        while ( (line > floor) &&
                (_doc->text[line - 1u] != '\n') )
        {
            line--;
        }

        // a comment alone on the line just above, or the lead ends
        if ( (comment->within != D_MAKE_NONE) ||
             (line < floor) ||
             (d_make_skip_blanks(_doc->text, line, comment->start) !=
              comment->start) ||
             (!( ( (gap == 1u) &&
                   (_doc->text[comment->end] == '\n') ) ||
                 ( (gap == 2u) &&
                   (_doc->text[comment->end] == '\r') &&
                   (_doc->text[comment->end + 1u] == '\n') ) )) )
        {
            break;
        }

        at = line;
    }

    return at;
}


/**
 * @brief Tries statements written in place of others, or of none, and
 *        keeps them when they read as their text does alone.
 *
 * @param[in,out] _doc     the document.
 * @param[in]     _splice  what they replace.
 * @param[in]     _text    the statements' text.
 * @param[in]     _first   the index of the first statement they replace.
 * @param[in]     _count   how many they replace.
 * @return `true` if the change was kept.
 */
static bool
d_internal_statements_change(
    struct d_make_doc* _doc,
    struct d_make_span _splice,
    const char*        _text,
    size_t             _first,
    size_t             _count
)
{
    char* const text = d_internal_lines_text(_doc, _splice.start, _text);

    // memory that ran out
    if (!text)
    {
        return d_internal_fail(_doc, "out of memory");
    }

    struct d_internal_candidate candidate;
    const bool                  tried = d_internal_try(_doc,
                                                       _splice.start,
                                                       _splice.end,
                                                       text,
                                                       &candidate);
    const bool                  read  = ( (tried) &&
                                          (candidate.before == _first) &&
                                          (candidate.old_after ==
                                           _first + _count) &&
                                          (d_internal_as_alone(_doc,
                                                               &candidate,
                                                               text)) );

    free(text);

    // refused outside the change, or not reading as written
    if (!read)
    {
        return (tried) ? d_internal_refuse(_doc, &candidate, "the change does "
                                           "not read back as asked")
                       : false;
    }

    return d_internal_commit(_doc, &candidate);
}


/*
d_make_doc_insert_statements
  The text goes above the comment lines that lead into the statement, so a
statement and its comment stay together.
*/
bool
d_make_doc_insert_statements(
    struct d_make_doc* _doc,
    size_t             _before,
    const char*        _text
)
{
    // parameter validation first
    if ( (!_doc) ||
         (!_text) ||
         (_before > _doc->records.statement_count) )
    {
        return (_doc) ? d_internal_fail(_doc, "the file has no such place")
                      : false;
    }

    const size_t             where  = d_internal_lead(_doc, _before);
    const struct d_make_span splice = { where, where };

    return d_internal_statements_change(_doc, splice, _text, _before, 0u);
}


/*
d_make_doc_set_statement
  The statement's whole lines are spliced; the text must read as one
statement of the old one's kind.
*/
bool
d_make_doc_set_statement(
    struct d_make_doc* _doc,
    size_t             _statement,
    const char*        _text
)
{
    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const struct d_make_statement* const statement =
        d_internal_statement(_doc, _statement);

    // a statement the file has
    if (!statement)
    {
        return false;
    }

    const enum d_make_kind   kind   = statement->kind;
    const struct d_make_span splice = { statement->start, statement->end };
    struct d_make            alone;
    const bool               one    = ( (d_make_read(&alone, _text,
                                                     strlen(_text),
                                                     _doc->dialect, NULL)) &&
                                        (alone.statement_count == 1u) &&
                                        ( (alone.statements[0].kind ==
                                           kind) ||
                                          (alone.statements[0].orphan) ) );

    d_make_release(&alone);

    // one statement of its kind
    if (!one)
    {
        return d_internal_fail(_doc, "the text is not one statement of the "
                                     "kind");
    }

    return d_internal_statements_change(_doc, splice, _text, _statement, 1u);
}


/*
d_make_doc_remove_statements
  The statements' whole lines go, and the comments among them; the
candidate is kept when they alone went.
*/
bool
d_make_doc_remove_statements(
    struct d_make_doc* _doc,
    size_t             _first,
    size_t             _last
)
{
    // parameter validation first
    if ( (!_doc) ||
         (_first > _last) ||
         (_last >= _doc->records.statement_count) )
    {
        return (_doc) ? d_internal_fail(_doc, "the file has no such "
                                              "statements")
                      : false;
    }

    struct d_internal_candidate candidate;

    // refused outside the change
    if (!d_internal_try(_doc,
                        _doc->records.statements[_first].start,
                        _doc->records.statements[_last].end,
                        "",
                        &candidate))
    {
        return false;
    }

    // they alone went
    if ( (candidate.before != _first) ||
         (candidate.old_after != _last + 1u) ||
         (candidate.new_after != _first) )
    {
        return d_internal_refuse(_doc, &candidate, "the change does not read "
                                                   "back as asked");
    }

    return d_internal_commit(_doc, &candidate);
}


//==============================================================================
// 9.  SAVING
//==============================================================================


/*
d_make_doc_save
  The file on disk is read first: one that holds the text already is left
as it is, its time untouched.
*/
enum d_make_saved
d_make_doc_save(
    struct d_make_doc*     _doc,
    const char*            _path,
    struct d_make_problem* _problem
)
{
    char why[256];

    // parameter validation first
    if (!_doc)
    {
        d_internal_tell(_problem, "no file to save");

        return D_MAKE_SAVE_FAILED;
    }

    const char* const path = ( (_path) &&
                               (_path[0] != '\0') ) ? _path : _doc->path;

    // somewhere to write it
    if (path[0] == '\0')
    {
        d_internal_tell(_problem, "the file has no path to save to");

        return D_MAKE_SAVE_FAILED;
    }

    size_t      length = 0;
    char* const there  = d_text_read(path, &length);
    const bool  same   = ( (there) &&
                           (length == _doc->length) &&
                           (memcmp(there, _doc->text, length) == 0) );

    free(there);

    // a file that holds the text already
    if (same)
    {
        return D_MAKE_SAVE_UNCHANGED;
    }

    // the directories it goes in
    if (!d_fs_make_parent(path))
    {
        (void)snprintf(why, sizeof(why), "cannot make the directories of %s",
                       path);
        d_internal_tell(_problem, why);

        return D_MAKE_SAVE_FAILED;
    }

    // the file, beside its target and renamed over it
    if (!d_text_write(path, _doc->text, _doc->length, why, sizeof(why)))
    {
        d_internal_tell(_problem, why);

        return D_MAKE_SAVE_FAILED;
    }

    return D_MAKE_SAVE_WRITTEN;
}
