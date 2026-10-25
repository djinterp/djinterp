/*******************************************************************************
* djinterp [tools]                                                    dgit_doc.c
*
* Definitions for the declarations in dgit_doc.h.
*   Every change is a splice, or several, each made on a copy of the text and
* read again. What lies outside a splice is compared record by record with
* what was there: the lines and records wholly before it must be as they
* were, those wholly after it as they were but moved by the splice. An entry
* is compared by its name, its value and its section -- the section the
* same, or, for an entry of a section the splice changed, one the splice
* changed too -- so renaming a section changes its entries' keys and nothing
* else does. A change of several splices keeps the text it began with, to
* go back to when one of them is refused.
*
* path:      /src/djinterp/tools/dawk/ext/git/dgit_doc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_doc.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, calloc, free
#include <string.h>  // memchr, memcmp, memcpy, memset, strchr, strcmp,
                     // strlen, strrchr
// djinterp
#include "./dgit_internal.h"  // d_git_file_read, d_git_file_write


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// d_git_doc
//   struct: a file's text, owned, and its records; the text it was made
//   from, to tell whether it changed; the changes kept; and why the last
//   was refused.
struct d_git_doc
{
    char*                path;
    enum d_git_text_kind kind;
    char*                text;
    size_t               length;
    char*                original;
    size_t               original_length;
    struct d_git_text    records;
    size_t               changes;
    char                 problem[256];
};

// d_internal_candidate
//   struct: a text a splice would make, and its records.
struct d_internal_candidate
{
    char*             text;
    size_t            length;
    struct d_git_text records;
};

// d_internal_splice
//   struct: one splice: the text from `start` to `end` replaced by
//   `length` bytes.
struct d_internal_splice
{
    size_t start;
    size_t end;
    size_t length;
};

// d_internal_span
//   struct: text a change takes away, from `start` to `end`.
struct d_internal_span
{
    size_t start;
    size_t end;
};


//==============================================================================
// 2.  DOCUMENTS
//==============================================================================


/*
d_internal_copy
  `_length` bytes, allocated and terminated; NULL when memory runs out.
*/
static char*
d_internal_copy(
    const char* _bytes,
    size_t      _length
)
{
    char* const copy = malloc(_length + 1u);

    // memory that ran out copies nothing
    if (!copy)
    {
        return NULL;
    }

    // nothing to copy leaves only the terminator
    if (_length > 0u)
    {
        memcpy(copy, _bytes, _length);
    }

    copy[_length] = '\0';

    return copy;
}


/*
d_internal_fail
  Says why a change was refused, and refuses it.
*/
static bool
d_internal_fail(
    struct d_git_doc* _doc,
    const char*       _why
)
{
    (void)snprintf(_doc->problem, sizeof(_doc->problem), "%s", _why);

    return false;
}


/*
d_internal_new_doc
  A document of a text it takes, read; NULL, the text freed, when reading
fails.
*/
static struct d_git_doc*
d_internal_new_doc(
    const char*          _path,
    enum d_git_text_kind _kind,
    char*                _text,
    size_t               _length,
    char*                _problem,
    size_t               _capacity
)
{
    const enum d_git_text_kind kind     = (_kind != D_GIT_TEXT_OTHER)
                                          ? _kind
                                          : d_git_text_kind_of(_path);
    struct d_git_doc* const    doc      = calloc(1u, sizeof(*doc));
    char* const                path     = d_internal_copy(_path,
                                                          strlen(_path));
    char* const                original = d_internal_copy(_text, _length);

    // a document, and its path and first text kept
    if ( (!doc)  ||
         (!path) ||
         (!original) )
    {
        (void)snprintf(_problem, _capacity, "out of memory");
        free(doc);
        free(path);
        free(original);
        free(_text);

        return NULL;
    }

    doc->path     = path;
    doc->original = original;

    doc->kind            = kind;
    doc->text            = _text;
    doc->length          = _length;
    doc->original_length = _length;

    // the records, or why there are none
    if (!d_git_text_read(&doc->records,
                         doc->text,
                         doc->length,
                         kind,
                         _problem,
                         _capacity))
    {
        d_git_doc_free(doc);

        return NULL;
    }

    return doc;
}


struct d_git_doc*
d_git_doc_read(
    const char*          _path,
    enum d_git_text_kind _kind,
    char*                _problem,
    size_t               _capacity
)
{
    size_t      length = 0u;
    char* const text   = (_path) ? d_git_file_read(_path, &length) : NULL;

    // a file that cannot be read
    if (!text)
    {
        (void)snprintf(_problem,
                       _capacity,
                       "cannot read %s",
                       (_path) ? _path : "(no path)");

        return NULL;
    }

    return d_internal_new_doc(_path, _kind, text, length, _problem, _capacity);
}


struct d_git_doc*
d_git_doc_make(
    const char*          _path,
    enum d_git_text_kind _kind,
    const char*          _text,
    size_t               _length,
    char*                _problem,
    size_t               _capacity
)
{
    char* const text = d_internal_copy((_text) ? _text : "",
                                       (_text) ? _length : 0u);

    // the text, copied
    if (!text)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return NULL;
    }

    return d_internal_new_doc((_path) ? _path : "",
                              _kind,
                              text,
                              (_text) ? _length : 0u,
                              _problem,
                              _capacity);
}


void
d_git_doc_free(
    struct d_git_doc* _doc
)
{
    // nothing to free
    if (!_doc)
    {
        return;
    }

    d_git_text_release(&_doc->records);
    free(_doc->path);
    free(_doc->text);
    free(_doc->original);
    free(_doc);

    return;
}


const struct d_git_text*
d_git_doc_records(
    const struct d_git_doc* _doc
)
{
    return (_doc) ? &_doc->records : NULL;
}


const char*
d_git_doc_path(
    const struct d_git_doc* _doc
)
{
    return (_doc) ? _doc->path : NULL;
}


enum d_git_text_kind
d_git_doc_kind(
    const struct d_git_doc* _doc
)
{
    return (_doc) ? _doc->kind : D_GIT_TEXT_OTHER;
}


size_t
d_git_doc_changes(
    const struct d_git_doc* _doc
)
{
    return (_doc) ? _doc->changes : 0u;
}


bool
d_git_doc_changed(
    const struct d_git_doc* _doc
)
{
    return ( (_doc) &&
             ( (_doc->length != _doc->original_length) ||
               ( (_doc->length > 0u) &&
                 (memcmp(_doc->text, _doc->original, _doc->length) != 0) ) ) );
}


const char*
d_git_doc_problem(
    const struct d_git_doc* _doc
)
{
    return (_doc) ? _doc->problem : "";
}


const char*
d_git_doc_text(
    const struct d_git_doc* _doc,
    size_t*                 _out_length
)
{
    // the length, if it is asked for
    if (_out_length)
    {
        *_out_length = (_doc) ? _doc->length : 0u;
    }

    return (_doc) ? _doc->text : NULL;
}


//==============================================================================
// 3.  COMPARING
//==============================================================================


// d_internal_compare
//   struct: what the records outside a splice are compared with: the
//   records before and after, the splice, and the sections it changed --
//   from `old_first` up to `old_end` of the old, `new_first` up to
//   `new_end` of the new.
struct d_internal_compare
{
    const struct d_git_text* old;
    const struct d_git_text* now;
    struct d_internal_splice splice;
    size_t                   old_first;
    size_t                   old_end;
    size_t                   new_first;
    size_t                   new_end;
};


/*
d_internal_moved
  Whether an offset after the splice is where the splice moved it.
*/
static bool
d_internal_moved(
    const struct d_internal_compare* _compare,
    size_t                           _old,
    size_t                           _new
)
{
    const struct d_internal_splice* const splice = &_compare->splice;

    return ((_new + (splice->end - splice->start)) == (_old + splice->length));
}


/*
d_internal_same_string
  Whether two offsets into two files' strings hold the same string, or
both none.
*/
static bool
d_internal_same_string(
    const struct d_git_text* _left,
    size_t                   _left_offset,
    const struct d_git_text* _right,
    size_t                   _right_offset
)
{
    const char* const left  = d_git_text_string(_left, _left_offset);
    const char* const right = d_git_text_string(_right, _right_offset);

    // both none, or both the same
    if ( (!left) ||
         (!right) )
    {
        return (left == right);
    }

    return (strcmp(left, right) == 0);
}


/*
d_internal_before, d_internal_after
  Whether lines are wholly before the splice -- their last ending at or
before its start -- or wholly after it, their first beginning at or past
its end.
*/
static bool
d_internal_before(
    const struct d_internal_compare* _compare,
    size_t                           _last_line
)
{
    return (_compare->old->lines[_last_line].next <= _compare->splice.start);
}


static bool
d_internal_after(
    const struct d_internal_compare* _compare,
    size_t                           _first_line
)
{
    return ( (_compare->old->lines[_first_line].start
                  >= _compare->splice.end) &&
             (!d_internal_before(_compare, _first_line)) );
}


/*
d_internal_same_line
  Whether a line before or after the splice reads as it did: its kind,
and its place, moved by the splice when it is after.
*/
static bool
d_internal_same_line(
    const struct d_internal_compare* _compare,
    size_t                           _old,
    size_t                           _new,
    bool                             _after
)
{
    const struct d_git_text_line* const old = &_compare->old->lines[_old];
    const struct d_git_text_line* const now = &_compare->now->lines[_new];

    // the same kind, and the same place
    if (old->kind != now->kind)
    {
        return false;
    }

    // after the splice, moved by it
    if (_after)
    {
        return ( (d_internal_moved(_compare, old->start, now->start)) &&
                 (d_internal_moved(_compare, old->end, now->end)) );
    }

    return ( (old->start == now->start) &&
             (old->end == now->end) );
}


/*
d_internal_same_place
  Whether a record's span is where it was, or where the splice moved it.
*/
static bool
d_internal_same_place(
    const struct d_internal_compare* _compare,
    size_t                           _old_start,
    size_t                           _old_end,
    size_t                           _new_start,
    size_t                           _new_end,
    bool                             _after
)
{
    // after the splice, moved by it
    if (_after)
    {
        return ( (d_internal_moved(_compare, _old_start, _new_start)) &&
                 (d_internal_moved(_compare, _old_end, _new_end)) );
    }

    return ( (_old_start == _new_start) &&
             (_old_end == _new_end) );
}


/*
d_internal_same_pattern
  Whether a pattern before or after the splice reads as it did: its place,
its text and marks, and its states, each name, state and value.
*/
static bool
d_internal_same_pattern(
    const struct d_internal_compare* _compare,
    size_t                           _old,
    size_t                           _new,
    bool                             _after
)
{
    const struct d_git_text* const         ot = _compare->old;
    const struct d_git_text* const         nt = _compare->now;
    const struct d_git_text_pattern* const o  = &ot->patterns[_old];
    const struct d_git_text_pattern* const n  = &nt->patterns[_new];

    // the place, the marks, the text, and as many states
    if ( (!d_internal_same_place(_compare,
                                 o->start,
                                 o->end,
                                 n->start,
                                 n->end,
                                 _after))                               ||
         (o->negated != n->negated)                                    ||
         (o->directory != n->directory)                                ||
         (o->macro != n->macro)                                        ||
         (!d_internal_same_string(ot, o->text, nt, n->text))           ||
         (o->attribute_count != n->attribute_count) )
    {
        return false;
    }

    // each state, alike
    for (size_t at = 0u; at < o->attribute_count; ++at)
    {
        const struct d_git_text_attribute* const os =
            &ot->attributes[o->first_attribute + at];
        const struct d_git_text_attribute* const ns =
            &nt->attributes[n->first_attribute + at];

        // the same state of the same name, the same value, in its place
        if ( (os->state != ns->state)                                   ||
             ((os->end - os->start) != (ns->end - ns->start))          ||
             (memcmp(&ot->text[os->start],
                     &nt->text[ns->start],
                     os->end - os->start) != 0)                         ||
             (!d_internal_same_place(_compare,
                                     os->start,
                                     os->end,
                                     ns->start,
                                     ns->end,
                                     _after)) )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_same_section
  Whether a section before or after the splice reads as it did.
*/
static bool
d_internal_same_section(
    const struct d_internal_compare* _compare,
    size_t                           _old,
    size_t                           _new,
    bool                             _after
)
{
    const struct d_git_text_section* const o = &_compare->old->sections[_old];
    const struct d_git_text_section* const n = &_compare->now->sections[_new];

    return ( (d_internal_same_place(_compare,
                                    o->start,
                                    o->end,
                                    n->start,
                                    n->end,
                                    _after))              &&
             (o->legacy == n->legacy)                    &&
             (d_internal_same_string(_compare->old,
                                     o->prefix,
                                     _compare->now,
                                     n->prefix)) );
}


/*
d_internal_name_of
  An entry's name, in lower case: its key past the last dot.
*/
static const char*
d_internal_name_of(
    const struct d_git_text* _text,
    size_t                   _entry
)
{
    const char* const key = d_git_text_string(_text,
                                              _text->entries[_entry].key);
    const char* const dot = strrchr(key, '.');

    return (dot) ? (dot + 1) : key;
}


/*
d_internal_same_membership
  Whether an entry after the splice is in the section it was: none, if it
was in none; the same section moved, if that section is outside the
splice; one the splice changed, if its was changed.
*/
static bool
d_internal_same_membership(
    const struct d_internal_compare* _compare,
    size_t                           _old,
    size_t                           _new
)
{
    const size_t old = _old;
    const size_t now = _new;

    // in none, before or after
    if ( (old == D_GIT_TEXT_NONE) ||
         (now == D_GIT_TEXT_NONE) )
    {
        return (old == now);
    }

    // a section the splice changed: one it changed still
    if ( (old >= _compare->old_first) &&
         (old < _compare->old_end) )
    {
        return ( (now >= _compare->new_first) &&
                 (now < _compare->new_end) );
    }

    // a section before the splice is where it was; one after, moved along
    // as the count of sections moved
    if (old < _compare->old_first)
    {
        return (now == old);
    }

    return ((now + _compare->old->section_count)
                == (old + _compare->now->section_count));
}


/*
d_internal_same_entry
  Whether an entry before or after the splice reads as it did: its place,
its name and value, and its section.
*/
static bool
d_internal_same_entry(
    const struct d_internal_compare* _compare,
    size_t                           _old,
    size_t                           _new,
    bool                             _after
)
{
    const struct d_git_text_entry* const o = &_compare->old->entries[_old];
    const struct d_git_text_entry* const n = &_compare->now->entries[_new];

    // before the splice, exactly as it was
    if (!_after)
    {
        return ( (d_internal_same_place(_compare,
                                        o->start,
                                        o->end,
                                        n->start,
                                        n->end,
                                        false))                         &&
                 (o->section == n->section)                            &&
                 (d_internal_same_string(_compare->old,
                                         o->key,
                                         _compare->now,
                                         n->key))                       &&
                 (d_internal_same_string(_compare->old,
                                         o->value,
                                         _compare->now,
                                         n->value)) );
    }

    return ( (d_internal_same_place(_compare,
                                    o->start,
                                    o->end,
                                    n->start,
                                    n->end,
                                    true))                              &&
             (strcmp(d_internal_name_of(_compare->old, _old),
                     d_internal_name_of(_compare->now, _new)) == 0)     &&
             (d_internal_same_string(_compare->old,
                                     o->value,
                                     _compare->now,
                                     n->value))                         &&
             (d_internal_same_membership(_compare, o->section, n->section)) );
}


// d_internal_records
//   struct: one kind of record, as the comparison walks it: how many the
//   old and new have, the old one's first and last lines, and whether two
//   are alike.
struct d_internal_records
{
    size_t old_count;
    size_t new_count;
    void   (*lines)(const struct d_git_text* _text,
                    size_t                   _record,
                    size_t*                  _first,
                    size_t*                  _last);
    bool   (*same)(const struct d_internal_compare* _compare,
                   size_t                           _old,
                   size_t                           _new,
                   bool                             _after);
};


/*
d_internal_pattern_lines, d_internal_section_lines, d_internal_entry_lines,
d_internal_line_lines
  The lines a record spans.
*/
static void
d_internal_pattern_lines(
    const struct d_git_text* _text,
    size_t                   _record,
    size_t*                  _first,
    size_t*                  _last
)
{
    *_first = _text->patterns[_record].line;
    *_last  = *_first;

    return;
}


static void
d_internal_section_lines(
    const struct d_git_text* _text,
    size_t                   _record,
    size_t*                  _first,
    size_t*                  _last
)
{
    *_first = _text->sections[_record].line;
    *_last  = *_first;

    return;
}


static void
d_internal_entry_lines(
    const struct d_git_text* _text,
    size_t                   _record,
    size_t*                  _first,
    size_t*                  _last
)
{
    *_first = _text->entries[_record].line;
    *_last  = _text->entries[_record].last_line;

    return;
}


static void
d_internal_line_lines(
    const struct d_git_text* _text,
    size_t                   _record,
    size_t*                  _first,
    size_t*                  _last
)
{
    (void)_text;
    *_first = _record;
    *_last  = _record;

    return;
}


/*
d_internal_records_same
  Compares one kind of record outside the splice: the old ones wholly
before it are the new ones' first, alike; those wholly after it the new
ones' last, alike but moved. The old records are in the text's order, so
those before are a prefix and those after a suffix.
  Long as the four kinds of record are compared in turn, each a few lines.
*/
static bool
d_internal_records_same(
    const struct d_internal_compare* _compare,
    const struct d_internal_records* _records
)
{
    size_t before = 0u;
    size_t after  = 0u;

    // the records wholly before the splice
    for (; before < _records->old_count; ++before)
    {
        size_t first = 0u;
        size_t last  = 0u;

        _records->lines(_compare->old, before, &first, &last);

        // the first not wholly before
        if (!d_internal_before(_compare, last))
        {
            break;
        }
    }

    // the records wholly after it, counted from the last
    for (; (before + after) < _records->old_count; ++after)
    {
        size_t first = 0u;
        size_t last  = 0u;

        _records->lines(_compare->old,
                        _records->old_count - after - 1u,
                        &first,
                        &last);

        // the last not wholly after
        if (!d_internal_after(_compare, first))
        {
            break;
        }
    }

    // the new must hold them all
    if (_records->new_count < (before + after))
    {
        return false;
    }

    // those before, alike
    for (size_t at = 0u; at < before; ++at)
    {
        // one that reads otherwise
        if (!_records->same(_compare, at, at, false))
        {
            return false;
        }
    }

    // those after, alike and moved
    for (size_t at = 0u; at < after; ++at)
    {
        // one that reads otherwise
        if (!_records->same(_compare,
                            _records->old_count - after + at,
                            _records->new_count - after + at,
                            true))
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_changed_sections
  The sections a splice changed, of the old and of the new: those neither
wholly before it nor wholly after it.
*/
static void
d_internal_changed_sections(
    struct d_internal_compare* _compare
)
{
    const size_t old_count = _compare->old->section_count;
    const size_t new_count = _compare->now->section_count;
    size_t       before    = 0u;
    size_t       after     = 0u;

    // the sections wholly before
    while ( (before < old_count) &&
            (d_internal_before(_compare,
                               _compare->old->sections[before].line)) )
    {
        ++before;
    }

    // the sections wholly after
    while ( ((before + after) < old_count) &&
            (d_internal_after(_compare,
                              _compare->old->sections[old_count - after - 1u]
                                  .line)) )
    {
        ++after;
    }

    _compare->old_first = before;
    _compare->old_end   = old_count - after;
    _compare->new_first = before;
    _compare->new_end   = (new_count >= (before + after))
                          ? (new_count - after)
                          : before;

    return;
}


/*
d_internal_outside_same
  Whether everything outside a splice reads as it did: its lines, and the
records of its kind. A configuration git read whole must still be read
whole.
*/
static bool
d_internal_outside_same(
    const struct d_git_text*        _old,
    const struct d_git_text*        _now,
    const struct d_internal_splice* _splice
)
{
    struct d_internal_compare compare;

    memset(&compare, 0, sizeof(compare));
    compare.old    = _old;
    compare.now    = _now;
    compare.splice = *_splice;

    struct d_internal_records lines = { _old->line_count,
                                        _now->line_count,
                                        d_internal_line_lines,
                                        d_internal_same_line };

    // a configuration git read whole must stay whole
    if ( (_old->broken == D_GIT_TEXT_NONE) &&
         (_now->broken != D_GIT_TEXT_NONE) )
    {
        return false;
    }

    // the lines first
    if (!d_internal_records_same(&compare, &lines))
    {
        return false;
    }

    // a configuration's sections and entries; another file's patterns
    if (_old->kind == D_GIT_TEXT_CONFIG)
    {
        struct d_internal_records sections = { _old->section_count,
                                               _now->section_count,
                                               d_internal_section_lines,
                                               d_internal_same_section };
        struct d_internal_records entries  = { _old->entry_count,
                                               _now->entry_count,
                                               d_internal_entry_lines,
                                               d_internal_same_entry };

        d_internal_changed_sections(&compare);

        return ( (d_internal_records_same(&compare, &sections)) &&
                 (d_internal_records_same(&compare, &entries)) );
    }

    struct d_internal_records patterns = { _old->pattern_count,
                                           _now->pattern_count,
                                           d_internal_pattern_lines,
                                           d_internal_same_pattern };

    return d_internal_records_same(&compare, &patterns);
}


//==============================================================================
// 4.  SPLICING
//==============================================================================


/*
d_internal_make_candidate
  The text a splice makes, and its records.
*/
static bool
d_internal_make_candidate(
    struct d_git_doc*               _doc,
    const struct d_internal_splice* _splice,
    const char*                     _text,
    struct d_internal_candidate*    _out
)
{
    const size_t length = _doc->length - (_splice->end - _splice->start)
                          + _splice->length;
    char         problem[64];

    memset(_out, 0, sizeof(*_out));
    _out->text = malloc(length + 1u);

    // room for the text
    if (!_out->text)
    {
        return d_internal_fail(_doc, "out of memory");
    }

    memcpy(_out->text, _doc->text, _splice->start);

    // the replacing text, if there is any
    if (_splice->length > 0u)
    {
        memcpy(&_out->text[_splice->start], _text, _splice->length);
    }

    memcpy(&_out->text[_splice->start + _splice->length],
           &_doc->text[_splice->end],
           _doc->length - _splice->end);
    _out->text[length] = '\0';
    _out->length       = length;

    // its records
    if (!d_git_text_read(&_out->records,
                         _out->text,
                         _out->length,
                         _doc->kind,
                         problem,
                         sizeof(problem)))
    {
        d_git_text_release(&_out->records);
        free(_out->text);
        _out->text = NULL;

        return d_internal_fail(_doc, problem);
    }

    return true;
}


/*
d_internal_keep
  Keeps a candidate as the document's text, and counts the change.
*/
static void
d_internal_keep(
    struct d_git_doc*            _doc,
    struct d_internal_candidate* _candidate
)
{
    d_git_text_release(&_doc->records);
    free(_doc->text);
    _doc->text    = _candidate->text;
    _doc->length  = _candidate->length;
    _doc->records = _candidate->records;
    ++_doc->changes;
    memset(_candidate, 0, sizeof(*_candidate));

    return;
}


/*
d_internal_splice_text
  One splice, kept if everything outside it reads as it did.
*/
static bool
d_internal_splice_text(
    struct d_git_doc* _doc,
    size_t            _start,
    size_t            _end,
    const char*       _text,
    size_t            _length
)
{
    const struct d_internal_splice splice = { _start, _end, _length };
    struct d_internal_candidate    candidate;

    // a splice within the text
    if ( (_start > _end) ||
         (_end > _doc->length) )
    {
        return d_internal_fail(_doc, "the change is not within the file");
    }

    // the text it makes, read
    if (!d_internal_make_candidate(_doc, &splice, _text, &candidate))
    {
        return false;
    }

    // what lies outside must read as it did
    if (!d_internal_outside_same(&_doc->records, &candidate.records, &splice))
    {
        d_git_text_release(&candidate.records);
        free(candidate.text);

        return d_internal_fail(_doc,
                               "the change would change what is outside it");
    }

    d_internal_keep(_doc, &candidate);
    _doc->problem[0] = '\0';

    return true;
}


bool
d_git_doc_splice(
    struct d_git_doc* _doc,
    size_t            _start,
    size_t            _end,
    const char*       _text,
    size_t            _length
)
{
    // a document to change
    if (!_doc)
    {
        return false;
    }

    return d_internal_splice_text(_doc,
                                  _start,
                                  _end,
                                  (_text) ? _text : "",
                                  (_text) ? _length : 0u);
}


// d_internal_saved
//   struct: a document's text as a change of several splices began, to go
//   back to.
struct d_internal_saved
{
    char*  text;
    size_t length;
    size_t changes;
};


/*
d_internal_begin
  Keeps the text a change begins from.
*/
static bool
d_internal_begin(
    struct d_git_doc*        _doc,
    struct d_internal_saved* _saved
)
{
    _saved->text    = d_internal_copy(_doc->text, _doc->length);
    _saved->length  = _doc->length;
    _saved->changes = _doc->changes;

    return (_saved->text) ? true : d_internal_fail(_doc, "out of memory");
}


/*
d_internal_finish
  Ends a change: kept, it counts as one; refused, the document goes back
to the text it began from, and the reason stays.
*/
static bool
d_internal_finish(
    struct d_git_doc*        _doc,
    struct d_internal_saved* _saved,
    bool                     _kept
)
{
    // kept: one change, however many splices
    if (_kept)
    {
        _doc->changes = _saved->changes + ((_doc->changes > _saved->changes)
                                           ? 1u
                                           : 0u);
        free(_saved->text);

        return true;
    }

    struct d_git_text records;
    char              problem[64];

    // a change that never began has nothing to go back to
    if (!_saved->text)
    {
        return false;
    }

    // the text it began from, read again
    if (d_git_text_read(&records,
                        _saved->text,
                        _saved->length,
                        _doc->kind,
                        problem,
                        sizeof(problem)))
    {
        d_git_text_release(&_doc->records);
        free(_doc->text);
        _doc->text    = _saved->text;
        _doc->length  = _saved->length;
        _doc->records = records;
        _doc->changes = _saved->changes;

        return false;
    }

    d_git_text_release(&records);
    free(_saved->text);

    return false;
}


//==============================================================================
// 5.  TEXT
//==============================================================================


/*
d_internal_eol
  The line ending a change writes: the file's first.
*/
static const char*
d_internal_eol(
    const struct d_git_doc* _doc
)
{
    return (_doc->records.crlf) ? "\r\n" : "\n";
}


/*
d_internal_ends_open
  Whether the text's last line has no ending, so text after it needs one.
*/
static bool
d_internal_ends_open(
    const struct d_git_doc* _doc
)
{
    return ( (_doc->length > 0u) &&
             (_doc->text[_doc->length - 1u] != '\n') );
}


/*
d_internal_line_text_start
  Where a line's text begins, past a byte-order mark on the first.
*/
static size_t
d_internal_line_text_start(
    const struct d_git_doc* _doc,
    size_t                  _line
)
{
    const size_t start = _doc->records.lines[_line].start;

    return ( (_line == 0u) && (_doc->records.bom) ) ? (start + 3u) : start;
}


/*
d_internal_one_line
  Whether text is one line: no `\n` in it.
*/
static bool
d_internal_one_line(
    const char* _text
)
{
    return ( (_text) &&
             (!strchr(_text, '\n')) );
}


/*
d_internal_line_reads
  Whether a line's text is exactly `_text`.
*/
static bool
d_internal_line_reads(
    const struct d_git_doc* _doc,
    size_t                  _line,
    const char*             _text
)
{
    // a line there
    if (_line >= _doc->records.line_count)
    {
        return false;
    }

    const size_t start  = d_internal_line_text_start(_doc, _line);
    const size_t end    = _doc->records.lines[_line].end;
    const size_t length = strlen(_text);

    return ( ((end - start) == length) &&
             (memcmp(&_doc->text[start], _text, length) == 0) );
}


/*
d_internal_valid_name
  Whether a name is one git allows an attribute: letters, digits, `-`, `.`
and `_`, not beginning with `-`.
*/
static bool
d_internal_valid_name(
    const char* _name
)
{
    // a name has a byte, and does not begin with `-`
    if ( (!_name)              ||
         (_name[0] == '\0')    ||
         (_name[0] == '-') )
    {
        return false;
    }

    // each byte one a name may hold
    for (const char* at = _name; *at; ++at)
    {
        // letters, digits, and three marks
        if ( (!( (*at >= 'a') && (*at <= 'z') )) &&
             (!( (*at >= 'A') && (*at <= 'Z') )) &&
             (!( (*at >= '0') && (*at <= '9') )) &&
             (*at != '-')                        &&
             (*at != '.')                        &&
             (*at != '_') )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_has_blank
  Whether text holds a byte attribute lines are apart by.
*/
static bool
d_internal_has_blank(
    const char* _text
)
{
    return (strpbrk(_text, " \t\r\n") != NULL);
}


//==============================================================================
// 6.  LINES
//==============================================================================


/*
d_git_doc_insert_line
  Long as the line's place, its ending, and the check that it reads back as
given are each a step.
*/
size_t
d_git_doc_insert_line(
    struct d_git_doc* _doc,
    size_t            _before,
    const char*       _text
)
{
    // a document, a line of text, and a place among its lines
    if (!_doc)
    {
        return D_GIT_TEXT_NONE;
    }
    else if (!d_internal_one_line(_text))
    {
        (void)d_internal_fail(_doc, "a line holds no line's end");

        return D_GIT_TEXT_NONE;
    }
    else if (_before > _doc->records.line_count)
    {
        (void)d_internal_fail(_doc, "no such line");

        return D_GIT_TEXT_NONE;
    }

    const size_t        count = _doc->records.line_count;
    const bool          last  = (_before == count);
    const size_t        at    = (last)
                                ? _doc->length
                                : d_internal_line_text_start(_doc, _before);
    struct d_git_buffer text;
    struct d_internal_saved saved;

    memset(&text, 0, sizeof(text));

    // a line after one with no ending gives that one its ending first
    if ( (last) &&
         (d_internal_ends_open(_doc)) )
    {
        (void)d_git_buffer_add_text(&text, d_internal_eol(_doc));
    }

    (void)d_git_buffer_add_text(&text, _text);
    (void)d_git_buffer_add_text(&text, d_internal_eol(_doc));

    // the text made, and the change begun
    if ( (text.failed) ||
         (!d_internal_begin(_doc, &saved)) )
    {
        d_git_buffer_free(&text);
        (void)d_internal_fail(_doc, "out of memory");

        return D_GIT_TEXT_NONE;
    }

    const bool kept = ( (d_internal_splice_text(_doc,
                                                at,
                                                at,
                                                text.text,
                                                text.length)) &&
                        ( (_doc->records.line_count == (count + 1u)) ||
                          (!d_internal_fail(_doc,
                                            "the text would not be one line "
                                            "more")) ) &&
                        ( (d_internal_line_reads(_doc, _before, _text)) ||
                          (!d_internal_fail(_doc,
                                            "the line would not read as "
                                            "it was given")) ) );

    d_git_buffer_free(&text);

    return (d_internal_finish(_doc, &saved, kept)) ? _before
                                                   : D_GIT_TEXT_NONE;
}


size_t
d_git_doc_set_line(
    struct d_git_doc* _doc,
    size_t            _line,
    const char*       _text
)
{
    // a document, a line of text, and a line to replace
    if (!_doc)
    {
        return D_GIT_TEXT_NONE;
    }
    else if (!d_internal_one_line(_text))
    {
        (void)d_internal_fail(_doc, "a line holds no line's end");

        return D_GIT_TEXT_NONE;
    }
    else if (_line >= _doc->records.line_count)
    {
        (void)d_internal_fail(_doc, "no such line");

        return D_GIT_TEXT_NONE;
    }

    const size_t            count = _doc->records.line_count;
    struct d_internal_saved saved;

    // the change begun
    if (!d_internal_begin(_doc, &saved))
    {
        return D_GIT_TEXT_NONE;
    }

    const bool kept = ( (d_internal_splice_text(
                             _doc,
                             d_internal_line_text_start(_doc, _line),
                             _doc->records.lines[_line].end,
                             _text,
                             strlen(_text)))                       &&
                        ( (_doc->records.line_count == count) ||
                          (!d_internal_fail(_doc,
                                            "the line would not stay a "
                                            "line")) )                 &&
                        ( (d_internal_line_reads(_doc, _line, _text)) ||
                          (!d_internal_fail(_doc,
                                            "the line would not read as "
                                            "it was given")) ) );

    return (d_internal_finish(_doc, &saved, kept)) ? _line : D_GIT_TEXT_NONE;
}


bool
d_git_doc_remove_line(
    struct d_git_doc* _doc,
    size_t            _line
)
{
    // a document, and a line to remove
    if (!_doc)
    {
        return false;
    }
    else if (_line >= _doc->records.line_count)
    {
        return d_internal_fail(_doc, "no such line");
    }

    const size_t            count = _doc->records.line_count;
    struct d_internal_saved saved;

    // the change begun
    if (!d_internal_begin(_doc, &saved))
    {
        return false;
    }

    const bool kept = ( (d_internal_splice_text(
                             _doc,
                             d_internal_line_text_start(_doc, _line),
                             _doc->records.lines[_line].next,
                             "",
                             0u))                                      &&
                        ( (_doc->records.line_count == (count - 1u)) ||
                          (!d_internal_fail(_doc,
                                            "the line would not go "
                                            "alone")) ) );

    return d_internal_finish(_doc, &saved, kept);
}


//==============================================================================
// 7.  ATTRIBUTES
//==============================================================================


/*
d_internal_last_state
  The last state of a name on a pattern's line, D_GIT_TEXT_NONE for none.
*/
static size_t
d_internal_last_state(
    const struct d_git_text* _text,
    size_t                   _pattern,
    const char*              _name
)
{
    const struct d_git_text_pattern* const pattern =
        &_text->patterns[_pattern];
    const size_t length = strlen(_name);

    // its states, the last first
    for (size_t at = pattern->attribute_count; at-- > 0u;)
    {
        const struct d_git_text_attribute* const state =
            &_text->attributes[pattern->first_attribute + at];

        // the name, whole
        if ( ((state->name_end - state->name_start) == length) &&
             (memcmp(&_text->text[state->name_start], _name, length) == 0) )
        {
            return pattern->first_attribute + at;
        }
    }

    return D_GIT_TEXT_NONE;
}


/*
d_internal_state_token
  Writes a state as an attribute line writes it.
*/
static bool
d_internal_state_token(
    struct d_git_buffer*  _out,
    const char*           _name,
    enum d_git_text_state _state,
    const char*           _value
)
{
    // the mark before the name
    if (_state == D_GIT_TEXT_UNSET)
    {
        (void)d_git_buffer_add_char(_out, '-');
    }
    else if (_state == D_GIT_TEXT_UNSPECIFIED)
    {
        (void)d_git_buffer_add_char(_out, '!');
    }

    (void)d_git_buffer_add_text(_out, _name);

    // a value after it
    if (_state == D_GIT_TEXT_VALUE)
    {
        (void)d_git_buffer_add_char(_out, '=');
        (void)d_git_buffer_add_text(_out, _value);
    }

    return !_out->failed;
}


/*
d_internal_state_reads
  Whether a pattern's line says of a name, last, what it was given.
*/
static bool
d_internal_state_reads(
    const struct d_git_doc* _doc,
    size_t                  _pattern,
    const char*             _name,
    enum d_git_text_state   _state,
    const char*             _value
)
{
    const struct d_git_text* const text = &_doc->records;

    // the pattern is still there
    if (_pattern >= text->pattern_count)
    {
        return false;
    }

    const size_t at = d_internal_last_state(text, _pattern, _name);

    // the name's last state, and its value
    if ( (at == D_GIT_TEXT_NONE) ||
         (text->attributes[at].state != _state) )
    {
        return false;
    }
    else if (_state != D_GIT_TEXT_VALUE)
    {
        return true;
    }

    const struct d_git_text_attribute* const state = &text->attributes[at];
    const size_t length = strlen(_value);

    return ( ((state->value_end - state->value_start) == length) &&
             (memcmp(&text->text[state->value_start], _value, length) == 0) );
}


/*
d_git_doc_set_state
  Long as the state's place on its line -- in place of one of its name, or
after the last -- and its text are each worked out here.
*/
bool
d_git_doc_set_state(
    struct d_git_doc*     _doc,
    size_t                _pattern,
    const char*           _name,
    enum d_git_text_state _state,
    const char*           _value
)
{
    // a document of attributes, a pattern of it, and a state it can hold
    if (!_doc)
    {
        return false;
    }
    else if ( (_doc->kind != D_GIT_TEXT_ATTRIBUTES) ||
              (_pattern >= _doc->records.pattern_count) )
    {
        return d_internal_fail(_doc, "no such pattern");
    }
    else if (!d_internal_valid_name(_name))
    {
        return d_internal_fail(_doc, "not a name git allows an attribute");
    }
    else if ( (_state == D_GIT_TEXT_VALUE) &&
              ( (!_value) || (d_internal_has_blank(_value)) ) )
    {
        return d_internal_fail(_doc, "a value cannot hold a blank");
    }

    const struct d_git_text* const         text    = &_doc->records;
    const struct d_git_text_pattern* const pattern = &text->patterns[_pattern];
    const size_t                           state   =
        d_internal_last_state(text, _pattern, _name);
    const size_t at  = (state != D_GIT_TEXT_NONE)
                       ? text->attributes[state].start
                       : ( (pattern->attribute_count > 0u)
                           ? text->attributes[pattern->first_attribute
                                              + pattern->attribute_count
                                              - 1u].end
                           : pattern->end );
    const size_t end = (state != D_GIT_TEXT_NONE)
                       ? text->attributes[state].end
                       : at;
    struct d_git_buffer     token;
    struct d_internal_saved saved;

    memset(&token, 0, sizeof(token));

    // a new state goes after a blank
    if (state == D_GIT_TEXT_NONE)
    {
        (void)d_git_buffer_add_char(&token, ' ');
    }

    // the token, and the change begun
    if ( (!d_internal_state_token(&token, _name, _state, _value)) ||
         (!d_internal_begin(_doc, &saved)) )
    {
        d_git_buffer_free(&token);

        return d_internal_fail(_doc, "out of memory");
    }

    const bool kept = ( (d_internal_splice_text(_doc,
                                                at,
                                                end,
                                                token.text,
                                                token.length)) &&
                        ( (d_internal_state_reads(_doc,
                                                  _pattern,
                                                  _name,
                                                  _state,
                                                  _value)) ||
                          (!d_internal_fail(_doc,
                                            "the line would not say what "
                                            "it was given")) ) );

    d_git_buffer_free(&token);

    return d_internal_finish(_doc, &saved, kept);
}


bool
d_git_doc_remove_state(
    struct d_git_doc* _doc,
    size_t            _pattern,
    const char*       _name
)
{
    // a document of attributes, a pattern of it, and a name
    if (!_doc)
    {
        return false;
    }
    else if ( (_doc->kind != D_GIT_TEXT_ATTRIBUTES) ||
              (_pattern >= _doc->records.pattern_count) )
    {
        return d_internal_fail(_doc, "no such pattern");
    }
    else if ( (!_name) ||
              (d_internal_last_state(&_doc->records, _pattern, _name)
                   == D_GIT_TEXT_NONE) )
    {
        return d_internal_fail(_doc, "the line has no state of that name");
    }

    struct d_internal_saved saved;
    bool                    kept = d_internal_begin(_doc, &saved);

    // each state of the name, the last first, with the blank before it
    for (size_t at = d_internal_last_state(&_doc->records, _pattern, _name);
         ( (kept) && (at != D_GIT_TEXT_NONE) );
         at = d_internal_last_state(&_doc->records, _pattern, _name))
    {
        const struct d_git_text* const         text    = &_doc->records;
        const struct d_git_text_pattern* const pattern =
            &text->patterns[_pattern];
        const size_t previous = (at > pattern->first_attribute)
                                ? text->attributes[at - 1u].end
                                : pattern->end;

        kept = d_internal_splice_text(_doc,
                                      previous,
                                      text->attributes[at].end,
                                      "",
                                      0u);
    }

    // a pattern of the line is still there
    if ( (kept) &&
         (_pattern >= _doc->records.pattern_count) )
    {
        kept = d_internal_fail(_doc, "the pattern would not stay");
    }

    return d_internal_finish(_doc, &saved, kept);
}


//==============================================================================
// 8.  CONFIGURATION
//==============================================================================


// d_internal_key
//   struct: a key as given and as git compares it: the canonical key, the
//   section's prefix it begins with, and the parts as given -- the
//   section's name, its subsection, D_GIT_TEXT_NONE long without one, and
//   the entry's name.
struct d_internal_key
{
    struct d_git_buffer canonical;
    struct d_git_buffer prefix;
    const char*         section;
    size_t              section_length;
    const char*         subsection;
    size_t              subsection_length;
    const char*         name;
};


/*
d_internal_key_read
  Reads a key git config would take; false, said, for one it refuses.
*/
static bool
d_internal_key_read(
    struct d_git_doc*      _doc,
    const char*            _key,
    struct d_internal_key* _out
)
{
    memset(_out, 0, sizeof(*_out));

    // a key git takes
    if ( (!_key) ||
         (!d_git_text_key(&_out->canonical, _key)) )
    {
        d_git_buffer_free(&_out->canonical);

        return d_internal_fail(_doc, "not a key git takes");
    }

    const char* const last = strrchr(_out->canonical.text, '.');

    (void)d_git_buffer_add(&_out->prefix,
                           _out->canonical.text,
                           (size_t)(last - _out->canonical.text));
    d_git_text_key_parts(_key,
                         &_out->section_length,
                         &_out->subsection_length);
    _out->section    = _key;
    _out->subsection = (_out->subsection_length != D_GIT_TEXT_NONE)
                       ? &_key[_out->section_length + 1u]
                       : NULL;
    _out->name       = strrchr(_key, '.') + 1;

    // the prefix too
    if (_out->prefix.failed)
    {
        d_git_buffer_free(&_out->canonical);
        d_git_buffer_free(&_out->prefix);

        return d_internal_fail(_doc, "out of memory");
    }

    return true;
}


/*
d_internal_key_free
  Lets go of a key's buffers.
*/
static void
d_internal_key_free(
    struct d_internal_key* _key
)
{
    d_git_buffer_free(&_key->canonical);
    d_git_buffer_free(&_key->prefix);

    return;
}


/*
d_internal_count_key
  How many entries have a key, and the last of them.
*/
static size_t
d_internal_count_key(
    const struct d_git_text* _text,
    const char*              _key,
    size_t*                  _out_last
)
{
    size_t count = 0u;

    *_out_last = D_GIT_TEXT_NONE;

    // each entry of the key
    for (size_t at = d_git_text_find_entry(_text, _key, D_GIT_TEXT_NONE);
         at != D_GIT_TEXT_NONE;
         at = d_git_text_find_entry(_text, _key, at))
    {
        *_out_last = at;
        ++count;
    }

    return count;
}


/*
d_internal_value_reads
  Whether an entry's value is what it was given: the same text, or none
for none.
*/
static bool
d_internal_value_reads(
    const struct d_git_text* _text,
    size_t                   _entry,
    const char*              _value
)
{
    const char* const value = d_git_text_string(_text,
                                                _text->entries[_entry].value);

    // none, or the same text
    if ( (!value) ||
         (!_value) )
    {
        return (value == _value);
    }

    return (strcmp(value, _value) == 0);
}


/*
d_internal_entry_text
  An entry as git config writes one: a tab, the name, " = " and the value
quoted as git writes it -- or the name alone for no value -- and the line's
ending.
*/
static bool
d_internal_entry_text(
    const struct d_git_doc* _doc,
    const char*             _name,
    const char*             _value,
    struct d_git_buffer*    _out
)
{
    (void)d_git_buffer_add_char(_out, '\t');
    (void)d_git_buffer_add_text(_out, _name);

    // the value, after its `=`
    if (_value)
    {
        (void)d_git_buffer_add_text(_out, " = ");
        (void)d_git_text_quote_value(_out, _value);
    }

    (void)d_git_buffer_add_text(_out, d_internal_eol(_doc));

    return !_out->failed;
}


/*
d_internal_header_text
  A section's header as git config writes one: the name, and a subsection
in quotes, as given.
*/
static bool
d_internal_header_text(
    const char*          _section,
    size_t               _section_length,
    const char*          _subsection,
    size_t               _subsection_length,
    struct d_git_buffer* _out
)
{
    (void)d_git_buffer_add_char(_out, '[');
    (void)d_git_buffer_add(_out, _section, _section_length);

    // a subsection, quoted
    if (_subsection)
    {
        struct d_git_buffer subsection;

        memset(&subsection, 0, sizeof(subsection));
        (void)d_git_buffer_add(&subsection, _subsection, _subsection_length);
        (void)d_git_buffer_add_text(_out, " \"");
        (void)d_git_text_quote_subsection(_out, subsection.text);
        (void)d_git_buffer_add_char(_out, '"');
        _out->failed = ( (_out->failed) || (subsection.failed) );
        d_git_buffer_free(&subsection);
    }

    (void)d_git_buffer_add_char(_out, ']');

    return !_out->failed;
}


/*
d_internal_last_section
  The last section of a prefix, D_GIT_TEXT_NONE for none.
*/
static size_t
d_internal_last_section(
    const struct d_git_text* _text,
    const char*              _prefix
)
{
    size_t last = D_GIT_TEXT_NONE;

    // each section of the prefix
    for (size_t at = d_git_text_find_section(_text, _prefix, D_GIT_TEXT_NONE);
         at != D_GIT_TEXT_NONE;
         at = d_git_text_find_section(_text, _prefix, at))
    {
        last = at;
    }

    return last;
}


/*
d_internal_add_place
  Where git config adds an entry of a key, and what it writes there: after
the last entry of the last section the key belongs in -- after its header,
for one with none -- or, when there is no such section, a new one at the
file's end. A line with no ending is given one first.
*/
static bool
d_internal_add_place(
    const struct d_git_doc*      _doc,
    const struct d_internal_key* _key,
    const char*                  _value,
    size_t*                      _out_at,
    struct d_git_buffer*         _out
)
{
    const struct d_git_text* const text    = &_doc->records;
    const size_t                   section = d_internal_last_section(
                                                 text,
                                                 _key->prefix.text);

    // the end of the file for a new section; else after the section's last
    if (section == D_GIT_TEXT_NONE)
    {
        *_out_at = _doc->length;
    }
    else
    {
        const struct d_git_text_section* const record =
            &text->sections[section];
        const size_t last = (record->entry_count > 0u)
                            ? text->entries[record->first_entry
                                            + record->entry_count - 1u]
                                  .last_line
                            : record->line;

        *_out_at = text->lines[last].next;
    }

    // a line with no ending gets one
    if ( (*_out_at == _doc->length) &&
         (d_internal_ends_open(_doc)) )
    {
        (void)d_git_buffer_add_text(_out, d_internal_eol(_doc));
    }

    // a new section's header
    if (section == D_GIT_TEXT_NONE)
    {
        (void)d_internal_header_text(_key->section,
                                     _key->section_length,
                                     _key->subsection,
                                     (_key->subsection)
                                     ? _key->subsection_length
                                     : 0u,
                                     _out);
        (void)d_git_buffer_add_text(_out, d_internal_eol(_doc));
    }

    return d_internal_entry_text(_doc, _key->name, _value, _out);
}


/*
d_internal_add
  Adds an entry of a key, read: one more of the key, the last its value.
*/
static bool
d_internal_add(
    struct d_git_doc*            _doc,
    const struct d_internal_key* _key,
    const char*                  _value
)
{
    size_t              last   = D_GIT_TEXT_NONE;
    const size_t        before = d_internal_count_key(&_doc->records,
                                                      _key->canonical.text,
                                                      &last);
    size_t              at     = 0u;
    struct d_git_buffer text;

    memset(&text, 0, sizeof(text));

    // what to write, and where
    if (!d_internal_add_place(_doc, _key, _value, &at, &text))
    {
        d_git_buffer_free(&text);

        return d_internal_fail(_doc, "out of memory");
    }

    const bool spliced = d_internal_splice_text(_doc,
                                                at,
                                                at,
                                                text.text,
                                                text.length);

    d_git_buffer_free(&text);

    // one more of the key, its last the value
    if ( (!spliced) ||
         (d_internal_count_key(&_doc->records,
                               _key->canonical.text,
                               &last) != (before + 1u)) ||
         (!d_internal_value_reads(&_doc->records, last, _value)) )
    {
        return (spliced) ? d_internal_fail(_doc,
                                           "the entry would not read as "
                                           "it was given")
                         : false;
    }

    return true;
}


bool
d_git_doc_add(
    struct d_git_doc* _doc,
    const char*       _key,
    const char*       _value
)
{
    struct d_internal_key   key;
    struct d_internal_saved saved;

    // a configuration, and a key git takes
    if (!_doc)
    {
        return false;
    }
    else if (_doc->kind != D_GIT_TEXT_CONFIG)
    {
        return d_internal_fail(_doc, "not a configuration");
    }
    else if (!d_internal_key_read(_doc, _key, &key))
    {
        return false;
    }

    // the change begun, made, and kept or let go
    const bool kept = ( (d_internal_begin(_doc, &saved)) &&
                        (d_internal_add(_doc, &key, _value)) );

    d_internal_key_free(&key);

    return d_internal_finish(_doc, &saved, kept);
}


/*
d_internal_after_value
  Whether anything but blanks follows an entry on its last line: a
comment, which a name alone cannot have after it.
*/
static bool
d_internal_after_value(
    const struct d_git_text* _text,
    size_t                   _entry
)
{
    const struct d_git_text_entry* const entry = &_text->entries[_entry];
    const size_t end = _text->lines[entry->last_line].end;

    // blanks only, to the line's end
    for (size_t at = entry->end; at < end; ++at)
    {
        // anything else
        if ( (_text->text[at] != ' ')  &&
             (_text->text[at] != '\t') &&
             (_text->text[at] != '\r') )
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_continued
  Whether an entry's value runs past its first line's end, a `\` joining
the next line to it.
*/
static bool
d_internal_continued(
    const struct d_git_text* _text,
    size_t                   _entry
)
{
    const struct d_git_text_entry* const entry = &_text->entries[_entry];

    return ( (entry->last_line > entry->line) ||
             (entry->end > _text->lines[entry->line].end) );
}


/*
d_internal_value_text
  What replaces an entry's value, and where: " = value" after a name alone;
a value of nothing written after its `=` with a space between; and for no
value, nothing in place of the `=` and all after it. A value continued
past its line is replaced to its last line's end, all it joined with it,
as git config replaces one.
*/
static bool
d_internal_value_text(
    const struct d_git_text* _text,
    size_t                   _entry,
    const char*              _value,
    struct d_internal_span*  _out_span,
    struct d_git_buffer*     _out
)
{
    const struct d_git_text_entry* const entry     = &_text->entries[_entry];
    const bool                           continued =
        d_internal_continued(_text, _entry);
    const size_t                         last      =
        _text->lines[entry->last_line].end;

    _out_span->start = entry->value_start;
    _out_span->end   = (continued) ? last : entry->value_end;

    // no value: the name alone
    if (!_value)
    {
        _out_span->start = entry->name_end;
        _out_span->end   = (entry->equals == D_GIT_TEXT_NONE)
                           ? entry->name_end
                           : (continued) ? last : entry->end;

        return true;
    }

    // a name alone takes its `=`; a value of nothing a space after it
    if (entry->equals == D_GIT_TEXT_NONE)
    {
        _out_span->start = entry->name_end;
        _out_span->end   = entry->name_end;
        (void)d_git_buffer_add_text(_out, " = ");
    }
    else if ( (entry->value_start == entry->value_end) &&
              (_text->text[entry->value_start - 1u] == '=') )
    {
        (void)d_git_buffer_add_char(_out, ' ');
    }

    return d_git_text_quote_value(_out, _value);
}


/*
d_internal_set_entry
  Gives an entry a value, read: the same entry, of the same key, with the
value.
*/
static bool
d_internal_set_entry(
    struct d_git_doc* _doc,
    size_t            _entry,
    const char*       _value
)
{
    const struct d_git_text* const text  = &_doc->records;
    const size_t                   count = text->entry_count;
    const char* const              was   =
        d_git_text_string(text, text->entries[_entry].key);
    char* const                    key   = d_internal_copy(was, strlen(was));
    struct d_internal_span         span  = { 0u, 0u };
    struct d_git_buffer            value;

    memset(&value, 0, sizeof(value));

    // a comment after the value would follow a name alone
    if ( (!_value) &&
         (!d_internal_continued(text, _entry)) &&
         (d_internal_after_value(text, _entry)) )
    {
        free(key);

        return d_internal_fail(_doc,
                               "a name alone cannot have a comment after it");
    }

    // the key kept, and the text made
    if ( (!key) ||
         (!d_internal_value_text(text, _entry, _value, &span, &value)) )
    {
        free(key);
        d_git_buffer_free(&value);

        return d_internal_fail(_doc, "out of memory");
    }

    const bool spliced = d_internal_splice_text(_doc,
                                                span.start,
                                                span.end,
                                                (value.text) ? value.text
                                                             : "",
                                                value.length);
    const bool reads   = ( (spliced)                                    &&
                           (_doc->records.entry_count == count)         &&
                           (strcmp(d_git_text_string(&_doc->records,
                                       _doc->records.entries[_entry].key),
                                   key) == 0)                           &&
                           (d_internal_value_reads(&_doc->records,
                                                   _entry,
                                                   _value)) );

    free(key);
    d_git_buffer_free(&value);

    // refused, kept, or not as asked
    if (!spliced)
    {
        return false;
    }

    return (reads) ? true
                   : d_internal_fail(_doc,
                                     "the entry would not read as it was "
                                     "given");
}


bool
d_git_doc_set_entry(
    struct d_git_doc* _doc,
    size_t            _entry,
    const char*       _value
)
{
    struct d_internal_saved saved;

    // a configuration, and an entry of it
    if (!_doc)
    {
        return false;
    }
    else if ( (_doc->kind != D_GIT_TEXT_CONFIG) ||
              (_entry >= _doc->records.entry_count) )
    {
        return d_internal_fail(_doc, "no such entry");
    }

    _doc->problem[0] = '\0';

    // the change begun, made, and kept or let go
    const bool kept = ( (d_internal_begin(_doc, &saved)) &&
                        (d_internal_set_entry(_doc, _entry, _value)) );

    return d_internal_finish(_doc, &saved, kept);
}


bool
d_git_doc_set(
    struct d_git_doc* _doc,
    const char*       _key,
    const char*       _value
)
{
    struct d_internal_key   key;
    struct d_internal_saved saved;
    size_t                  last = D_GIT_TEXT_NONE;

    // a configuration, and a key git takes
    if (!_doc)
    {
        return false;
    }
    else if (_doc->kind != D_GIT_TEXT_CONFIG)
    {
        return d_internal_fail(_doc, "not a configuration");
    }
    else if (!d_internal_key_read(_doc, _key, &key))
    {
        return false;
    }

    const size_t count = d_internal_count_key(&_doc->records,
                                              key.canonical.text,
                                              &last);

    // a key of several values is not one to set
    if (count > 1u)
    {
        d_internal_key_free(&key);

        return d_internal_fail(_doc, "the key has several values");
    }

    _doc->problem[0] = '\0';

    // its one entry's value, or a new entry
    const bool kept = ( (d_internal_begin(_doc, &saved)) &&
                        ( (count == 1u)
                          ? d_internal_set_entry(_doc, last, _value)
                          : d_internal_add(_doc, &key, _value) ) );

    d_internal_key_free(&key);

    return d_internal_finish(_doc, &saved, kept);
}


/*
d_internal_first_on_line
  Whether nothing but blanks is before an offset on its line.
*/
static bool
d_internal_first_on_line(
    const struct d_git_doc* _doc,
    size_t                  _offset
)
{
    const size_t line = d_git_text_line_of(&_doc->records, _offset);

    // each byte before it on the line
    for (size_t at = d_internal_line_text_start(_doc, line);
         at < _offset;
         ++at)
    {
        // anything but a blank
        if ( (_doc->text[at] != ' ')  &&
             (_doc->text[at] != '\t') &&
             (_doc->text[at] != '\r') )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_entry_span
  The text taking an entry away takes: its lines whole when it begins its
line -- a comment after it with them -- and otherwise the entry and the
blanks before it, after the header it shares its line with.
*/
static struct d_internal_span
d_internal_entry_span(
    const struct d_git_doc* _doc,
    size_t                  _entry
)
{
    const struct d_git_text* const       text  = &_doc->records;
    const struct d_git_text_entry* const entry = &text->entries[_entry];
    struct d_internal_span               span  = { entry->start, entry->end };

    // its lines, whole
    if (d_internal_first_on_line(_doc, entry->start))
    {
        span.start = d_internal_line_text_start(_doc, entry->line);
        span.end   = text->lines[entry->last_line].next;

        return span;
    }

    const size_t first = d_internal_line_text_start(_doc, entry->line);

    // the blanks before it
    while ( (span.start > first) &&
            ( (_doc->text[span.start - 1u] == ' ') ||
              (_doc->text[span.start - 1u] == '\t') ) )
    {
        --span.start;
    }

    return span;
}


/*
d_internal_same_prefix
  Whether a section's prefix is the one given.
*/
static bool
d_internal_same_prefix(
    const struct d_git_text* _text,
    size_t                   _section,
    const char*              _prefix
)
{
    return (strcmp(d_git_text_string(_text, _text->sections[_section].prefix),
                   _prefix) == 0);
}


/*
d_internal_emptied_begin
  Walks back from an entry being taken away, as git's maybe_remove_section
does: past blanks and headers of the entry's own section, to an entry or
another section's header, after which the section's text begins. A comment
on the way, or an entry before any header of its own, keeps the section.
*/
static bool
d_internal_emptied_begin(
    const struct d_git_doc* _doc,
    size_t                  _entry,
    const char*             _prefix,
    size_t*                 _out_begin
)
{
    const struct d_git_text* const       text  = &_doc->records;
    const struct d_git_text_entry* const entry = &text->entries[_entry];
    bool seen = (text->lines[entry->line].kind == D_GIT_TEXT_SECTION);

    *_out_begin = d_internal_line_text_start(_doc, 0u);

    // each line before the entry's, back to what stops the walk
    for (size_t back = entry->line; back-- > 0u;)
    {
        const struct d_git_text_line* const line = &text->lines[back];

        // a comment keeps the section
        if (line->kind == D_GIT_TEXT_COMMENT)
        {
            return false;
        }

        // an entry: the section's text begins after it, if its header is
        // between them
        if (line->entry != D_GIT_TEXT_NONE)
        {
            *_out_begin = line->next;

            return seen;
        }

        // a header of the section is passed; another's stops the walk
        if (line->kind == D_GIT_TEXT_SECTION)
        {
            // another section's
            if (!d_internal_same_prefix(text, line->section, _prefix))
            {
                *_out_begin = line->next;

                return seen;
            }

            seen = true;
        }
    }

    return seen;
}


/*
d_internal_emptied_end
  Walks on from an entry being taken away: past blanks, headers of its own
section and entries also being taken away, to another section's header,
where the section's text ends, or the file's end. A comment on the way, or
an entry that stays, keeps the section.
*/
static bool
d_internal_emptied_end(
    const struct d_git_doc* _doc,
    size_t                  _entry,
    const char*             _prefix,
    const bool*             _removing,
    size_t*                 _out_end
)
{
    const struct d_git_text* const text = &_doc->records;

    *_out_end = _doc->length;

    // each line after the entry's last
    for (size_t on = text->entries[_entry].last_line + 1u;
         on < text->line_count;
         ++on)
    {
        const struct d_git_text_line* const line = &text->lines[on];

        // a comment keeps the section
        if (line->kind == D_GIT_TEXT_COMMENT)
        {
            return false;
        }

        // another section's header ends the walk
        if ( (line->kind == D_GIT_TEXT_SECTION) &&
             (!d_internal_same_prefix(text, line->section, _prefix)) )
        {
            *_out_end = line->start;

            return true;
        }

        // an entry that stays keeps the section; one that goes is passed
        if (line->entry != D_GIT_TEXT_NONE)
        {
            // one that stays
            if (!_removing[line->entry])
            {
                return false;
            }

            on = text->entries[line->entry].last_line;
        }
    }

    return true;
}


/*
d_internal_removal_span
  The text taking an entry away takes: its section's, when it is the last
of the section's to go and no comment is within or about the section, as
git config removes a section it empties; otherwise its own.
*/
static struct d_internal_span
d_internal_removal_span(
    const struct d_git_doc* _doc,
    size_t                  _entry,
    const bool*             _removing
)
{
    const struct d_git_text* const text    = &_doc->records;
    const size_t                   section = text->entries[_entry].section;
    struct d_internal_span         span    = { 0u, 0u };

    // an entry in a section that goes with it
    if ( (section != D_GIT_TEXT_NONE) &&
         (d_internal_emptied_begin(_doc,
                                   _entry,
                                   d_git_text_string(text,
                                       text->sections[section].prefix),
                                   &span.start)) &&
         (d_internal_emptied_end(_doc,
                                 _entry,
                                 d_git_text_string(text,
                                     text->sections[section].prefix),
                                 _removing,
                                 &span.end)) )
    {
        return span;
    }

    return d_internal_entry_span(_doc, _entry);
}


/*
d_internal_compare_spans
  Orders spans by where they begin.
*/
static int
d_internal_compare_spans(
    const void* _left,
    const void* _right
)
{
    const struct d_internal_span* const left  = _left;
    const struct d_internal_span* const right = _right;

    return (left->start < right->start) ? -1
         : ( (left->start > right->start) ? 1 : 0 );
}


/*
d_internal_take_spans
  Takes spans of text away, the last first, so each one's place holds
until it goes; spans that overlap are taken as one.
*/
static bool
d_internal_take_spans(
    struct d_git_doc*       _doc,
    struct d_internal_span* _spans,
    size_t                  _count
)
{
    size_t kept = 0u;

    qsort(_spans, _count, sizeof(*_spans), d_internal_compare_spans);

    // overlapping spans joined
    for (size_t at = 0u; at < _count; ++at)
    {
        // one that overlaps the last kept joins it
        if ( (kept > 0u) &&
             (_spans[at].start < _spans[kept - 1u].end) )
        {
            _spans[kept - 1u].end = (_spans[at].end > _spans[kept - 1u].end)
                                    ? _spans[at].end
                                    : _spans[kept - 1u].end;

            continue;
        }

        _spans[kept] = _spans[at];
        ++kept;
    }

    // the last first
    for (size_t at = kept; at-- > 0u;)
    {
        // each must be kept
        if (!d_internal_splice_text(_doc,
                                    _spans[at].start,
                                    _spans[at].end,
                                    "",
                                    0u))
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_remove_entries
  Takes away the entries marked, each with its section when it empties it.
*/
static bool
d_internal_remove_entries(
    struct d_git_doc* _doc,
    const bool*       _removing
)
{
    const size_t                  count = _doc->records.entry_count;
    struct d_internal_span* const spans = malloc((count + 1u)
                                                 * sizeof(*spans));
    size_t                        used  = 0u;

    // room for a span for each
    if (!spans)
    {
        return d_internal_fail(_doc, "out of memory");
    }

    // a span for each entry that goes
    for (size_t at = 0u; at < count; ++at)
    {
        // one that goes
        if (_removing[at])
        {
            spans[used] = d_internal_removal_span(_doc, at, _removing);
            ++used;
        }
    }

    const bool taken = d_internal_take_spans(_doc, spans, used);

    free(spans);

    return taken;
}


/*
d_internal_unset
  Takes away the entries of a key, or the one given; one of several only
when told to take them all.
*/
static bool
d_internal_unset(
    struct d_git_doc* _doc,
    const char*       _key,
    size_t            _entry,
    bool              _all
)
{
    const struct d_git_text* const text     = &_doc->records;
    size_t                         last     = D_GIT_TEXT_NONE;
    const size_t                   count    = d_internal_count_key(text,
                                                                   _key,
                                                                   &last);
    bool* const                    removing = calloc(text->entry_count + 1u,
                                                     sizeof(*removing));

    // room for the marks
    if (!removing)
    {
        return d_internal_fail(_doc, "out of memory");
    }

    // the one given, or every one of the key
    for (size_t at = d_git_text_find_entry(text, _key, D_GIT_TEXT_NONE);
         at != D_GIT_TEXT_NONE;
         at = d_git_text_find_entry(text, _key, at))
    {
        removing[at] = ( (_all) ||
                         (_entry == D_GIT_TEXT_NONE) ||
                         (at == _entry) );
    }

    const bool removed = d_internal_remove_entries(_doc, removing);

    free(removing);

    // as many fewer as went
    if ( (removed) &&
         (d_internal_count_key(&_doc->records, _key, &last)
              != ( ( (_all) || (count == 1u) ) ? 0u : (count - 1u) )) )
    {
        return d_internal_fail(_doc, "the key would not go as asked");
    }

    return removed;
}


bool
d_git_doc_unset(
    struct d_git_doc* _doc,
    const char*       _key,
    bool              _all
)
{
    struct d_internal_key   key;
    struct d_internal_saved saved;
    size_t                  last = D_GIT_TEXT_NONE;

    // a configuration, and a key git takes
    if (!_doc)
    {
        return false;
    }
    else if (_doc->kind != D_GIT_TEXT_CONFIG)
    {
        return d_internal_fail(_doc, "not a configuration");
    }
    else if (!d_internal_key_read(_doc, _key, &key))
    {
        return false;
    }

    const size_t count = d_internal_count_key(&_doc->records,
                                              key.canonical.text,
                                              &last);

    // a key with an entry, and only one unless all go
    if ( (count == 0u) ||
         ( (count > 1u) && (!_all) ) )
    {
        d_internal_key_free(&key);

        return d_internal_fail(_doc,
                               (count == 0u) ? "no such key"
                                             : "the key has several values");
    }

    const bool kept = ( (d_internal_begin(_doc, &saved)) &&
                        (d_internal_unset(_doc,
                                          key.canonical.text,
                                          D_GIT_TEXT_NONE,
                                          _all)) );

    d_internal_key_free(&key);

    return d_internal_finish(_doc, &saved, kept);
}


bool
d_git_doc_remove_entry(
    struct d_git_doc* _doc,
    size_t            _entry
)
{
    struct d_internal_saved saved;

    // a configuration, and an entry of it
    if (!_doc)
    {
        return false;
    }
    else if ( (_doc->kind != D_GIT_TEXT_CONFIG) ||
              (_entry >= _doc->records.entry_count) )
    {
        return d_internal_fail(_doc, "no such entry");
    }

    const char* const key  = d_git_text_string(&_doc->records,
                                               _doc->records.entries[_entry]
                                                   .key);
    char* const       copy = d_internal_copy(key, strlen(key));

    // the key kept, for after the records change
    if (!copy)
    {
        return d_internal_fail(_doc, "out of memory");
    }

    // the change begun, made, and kept or let go
    const bool kept = ( (d_internal_begin(_doc, &saved)) &&
                        (d_internal_unset(_doc, copy, _entry, false)) );

    free(copy);

    return d_internal_finish(_doc, &saved, kept);
}


/*
d_internal_section_name
  Reads a section's name as git config takes one, "section" or
"section.subsection": the prefix its keys begin with, and the parts as
given. False, said, for a name git refuses.
*/
static bool
d_internal_section_name(
    struct d_git_doc*      _doc,
    const char*            _name,
    struct d_internal_key* _out,
    struct d_git_buffer*   _key
)
{
    memset(_key, 0, sizeof(*_key));

    // a name to read
    if (!_name)
    {
        return d_internal_fail(_doc, "not a section git takes");
    }

    (void)d_git_buffer_add_text(_key, _name);
    (void)d_git_buffer_add_text(_key, ".x");

    // the name with a name after it, as a key
    if (_key->failed)
    {
        d_git_buffer_free(_key);

        return d_internal_fail(_doc, "out of memory");
    }
    else if (!d_internal_key_read(_doc, _key->text, _out))
    {
        d_git_buffer_free(_key);

        return d_internal_fail(_doc, "not a section git takes");
    }

    return true;
}


/*
d_internal_rename
  Gives every header of a prefix the header of another, the last first.
*/
static bool
d_internal_rename(
    struct d_git_doc*            _doc,
    const struct d_internal_key* _from,
    const struct d_internal_key* _to
)
{
    struct d_git_buffer header;
    const size_t        entries = _doc->records.entry_count;

    memset(&header, 0, sizeof(header));

    // the header written
    if (!d_internal_header_text(_to->section,
                                _to->section_length,
                                _to->subsection,
                                (_to->subsection) ? _to->subsection_length
                                                  : 0u,
                                &header))
    {
        d_git_buffer_free(&header);

        return d_internal_fail(_doc, "out of memory");
    }

    bool kept = true;

    // each header of the prefix, the last first
    for (size_t at = d_internal_last_section(&_doc->records,
                                             _from->prefix.text);
         ( (kept) && (at != D_GIT_TEXT_NONE) );
         at = d_internal_last_section(&_doc->records, _from->prefix.text))
    {
        const struct d_git_text_section* const section =
            &_doc->records.sections[at];

        kept = d_internal_splice_text(_doc,
                                      section->start,
                                      section->end,
                                      header.text,
                                      header.length);

        // a name that does not change is renamed once
        if (strcmp(_from->prefix.text, _to->prefix.text) == 0)
        {
            break;
        }
    }

    d_git_buffer_free(&header);

    return ( (kept) &&
             ( (_doc->records.entry_count == entries) ||
               (d_internal_fail(_doc, "the section's entries would not "
                                      "stay")) ) );
}


bool
d_git_doc_rename_section(
    struct d_git_doc* _doc,
    const char*       _name,
    const char*       _to
)
{
    struct d_internal_key   from;
    struct d_internal_key   to;
    struct d_git_buffer     from_key;
    struct d_git_buffer     to_key;
    struct d_internal_saved saved;

    // a configuration, and two names git takes
    if (!_doc)
    {
        return false;
    }
    else if (_doc->kind != D_GIT_TEXT_CONFIG)
    {
        return d_internal_fail(_doc, "not a configuration");
    }
    else if (!d_internal_section_name(_doc, _name, &from, &from_key))
    {
        return false;
    }
    else if (!d_internal_section_name(_doc, _to, &to, &to_key))
    {
        d_internal_key_free(&from);
        d_git_buffer_free(&from_key);

        return false;
    }

    const bool found = (d_internal_last_section(&_doc->records,
                                                from.prefix.text)
                            != D_GIT_TEXT_NONE);
    const bool kept  = ( (found) &&
                         (d_internal_begin(_doc, &saved)) &&
                         (d_internal_rename(_doc, &from, &to)) );

    d_internal_key_free(&from);
    d_internal_key_free(&to);
    d_git_buffer_free(&from_key);
    d_git_buffer_free(&to_key);

    // a section there to rename
    if (!found)
    {
        return d_internal_fail(_doc, "no such section");
    }

    return d_internal_finish(_doc, &saved, kept);
}


/*
d_internal_section_span
  The text a section's removal takes: from its header -- its line, when
the header begins it -- to the next section's header, or the file's end.
*/
static struct d_internal_span
d_internal_section_span(
    const struct d_git_doc* _doc,
    size_t                  _section
)
{
    const struct d_git_text* const text = &_doc->records;
    const size_t                   start = text->sections[_section].start;
    struct d_internal_span         span  = { start, _doc->length };

    // its line, when the header begins it
    if (d_internal_first_on_line(_doc, start))
    {
        span.start = d_internal_line_text_start(_doc,
                                                text->sections[_section].line);
    }

    // to the next header: its line, when it begins it
    if ((_section + 1u) < text->section_count)
    {
        const size_t next = text->sections[_section + 1u].start;

        span.end = (d_internal_first_on_line(_doc, next))
                   ? text->lines[text->sections[_section + 1u].line].start
                   : next;
    }

    return span;
}


bool
d_git_doc_remove_section(
    struct d_git_doc* _doc,
    const char*       _name
)
{
    struct d_internal_key   name;
    struct d_git_buffer     key;
    struct d_internal_saved saved;

    // a configuration, and a name git takes
    if (!_doc)
    {
        return false;
    }
    else if (_doc->kind != D_GIT_TEXT_CONFIG)
    {
        return d_internal_fail(_doc, "not a configuration");
    }
    else if (!d_internal_section_name(_doc, _name, &name, &key))
    {
        return false;
    }

    bool kept = (d_internal_last_section(&_doc->records, name.prefix.text)
                     != D_GIT_TEXT_NONE);

    // a section there to remove
    if (!kept)
    {
        d_internal_key_free(&name);
        d_git_buffer_free(&key);

        return d_internal_fail(_doc, "no such section");
    }

    kept = d_internal_begin(_doc, &saved);

    // each section of the name, the last first
    for (size_t at = d_internal_last_section(&_doc->records,
                                             name.prefix.text);
         ( (kept) && (at != D_GIT_TEXT_NONE) );
         at = d_internal_last_section(&_doc->records, name.prefix.text))
    {
        const struct d_internal_span span = d_internal_section_span(_doc, at);

        kept = d_internal_splice_text(_doc, span.start, span.end, "", 0u);
    }

    d_internal_key_free(&name);
    d_git_buffer_free(&key);

    return d_internal_finish(_doc, &saved, kept);
}


//==============================================================================
// 9.  SAVING
//==============================================================================


enum d_git_saved
d_git_doc_save(
    const struct d_git_doc* _doc,
    const char*             _path,
    char*                   _problem,
    size_t                  _capacity
)
{
    const char* const path = ( (_path) && (_path[0] != '\0') )
                             ? _path
                             : ( (_doc) ? _doc->path : NULL );

    // a document, and somewhere to write it
    if ( (!_doc) ||
         (!path) ||
         (path[0] == '\0') )
    {
        (void)snprintf(_problem, _capacity, "the file has no path to save to");

        return D_GIT_SAVE_FAILED;
    }

    size_t      length = 0u;
    char* const there  = d_git_file_read(path, &length);

    // a file that holds the text already is left as it is
    if ( (there) &&
         (length == _doc->length) &&
         ( (length == 0u) ||
           (memcmp(there, _doc->text, length) == 0) ) )
    {
        free(there);

        return D_GIT_SAVE_UNCHANGED;
    }

    free(there);

    return (d_git_file_write(path,
                             _doc->text,
                             _doc->length,
                             _problem,
                             _capacity))
           ? D_GIT_SAVE_WRITTEN
           : D_GIT_SAVE_FAILED;
}
