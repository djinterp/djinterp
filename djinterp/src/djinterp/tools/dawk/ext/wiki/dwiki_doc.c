/*******************************************************************************
* djinterp [tools]                                                   dwiki_doc.c
*
* Definitions for the declarations in dwiki_doc.h.
*   Every change is a candidate first: the new text, read into records of
* its own. The records outside the splice are compared, in order, with the
* page's: those that ended at or before its start must be the page's own,
* and those that began at or after its end the page's own moved by what the
* splice added. A change of a kind is then checked for what it was to do;
* only then are the page's text and records the candidate's. A change made
* in two splices keeps the page as it was before the first, to go back to
* if the second is refused.
*   Files are read and written here, apart from the other units: a file is
* written beside its target and renamed over it.
*
* path:      /src/djinterp/tools/dawk/ext/wiki/dwiki_doc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/wiki/dwiki_doc.h"  // corresponding header
// std
#include <stddef.h>  // ptrdiff_t
#include <stdio.h>   // FILE, fopen, fread, fwrite, fclose, remove, rename,
                     // snprintf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcmp, memcpy, memset, strcmp, strlen
// platform
#if defined(_WIN32)
    #include <direct.h>    // _mkdir
    #include <sys/stat.h>  // struct _stat, _stat, _S_IFDIR
#else
    #include <sys/stat.h>  // struct stat, stat, S_ISDIR, mkdir
#endif  // _WIN32
// djinterp
#include "./dwiki_driver.h"  // d_wiki_driver_of


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Documents
//------------------------------------------------------------------------------
// 1.1.1
// d_wiki_doc
//   struct: the page: its path, dialect, text and records, the changes it
//   has taken, and why the last failed.
struct d_wiki_doc
{
    char*               path;
    enum d_wiki_dialect dialect;
    char*               text;
    size_t              length;
    struct d_wiki       records;
    size_t              changes;
    size_t              start;
    size_t              old_end;
    size_t              new_end;
    bool                composing;
    char                problem[512];
};

// 1.1.2
// d_internal_candidate
//   struct: a change not yet kept: the new text and its records, and where
//   the splice stands in the old text and in the new.
struct d_internal_candidate
{
    char*         text;
    size_t        length;
    struct d_wiki records;
    size_t        start;
    size_t        old_end;
    size_t        new_end;
};

// 1.1.3
// d_internal_kept
//   struct: the page as it was before a change, to go back to, and where
//   the change before that was.
struct d_internal_kept
{
    char*         text;
    size_t        length;
    struct d_wiki records;
    size_t        changes;
    size_t        start;
    size_t        old_end;
    size_t        new_end;
};

// 1.1.4
// d_internal_span
//   struct: bytes of the text, `start` to `end`, and what is written in
//   their place: NULL for nothing.
struct d_internal_span
{
    size_t      start;
    size_t      end;
    const char* text;
};

// 1.1.5
// d_internal_link_change
//   struct: a change to a link: the link as it was, whether it is a
//   reference, the destination and the label asked for -- NULL for one
//   left as it is -- what each is written as, and where the label goes.
struct d_internal_link_change
{
    struct d_wiki_link     link;
    bool                   refers;
    const char*            target;
    const char*            label;
    struct d_wiki_buffer   target_text;
    struct d_wiki_buffer   label_text;
    struct d_internal_span label_span;
};


//==============================================================================
// 2.  FILES
//==============================================================================


/*
d_internal_read_file
  A file's bytes, terminated; NULL when it cannot be read.
*/
static char*
d_internal_read_file(
    const char* _path,
    size_t*     _out_length
)
{
    FILE* const file     = fopen(_path, "rb");
    char*       text     = NULL;
    size_t      length   = 0;
    size_t      capacity = 0;

    // a file that cannot be opened has no text
    if (!file)
    {
        return NULL;
    }

    // the bytes, a block at a time
    for (;;)
    {
        // room for the next chunk, and its terminator
        if ((length + 4096u + 1u) > capacity)
        {
            const size_t wanted = (capacity > 0u) ? (capacity * 2u) : 8192u;
            char* const  grown  = realloc(text, wanted);

            // memory that ran out loses what was read
            if (!grown)
            {
                free(text);
                (void)fclose(file);

                return NULL;
            }

            text     = grown;
            capacity = wanted;
        }

        const size_t got = fread(&text[length], 1u, 4096u, file);

        length += got;

        // a short read is the file's end
        if (got < 4096u)
        {
            break;
        }
    }

    const bool failed = (ferror(file) != 0);

    (void)fclose(file);

    // a read that failed reads nothing
    if (failed)
    {
        free(text);

        return NULL;
    }

    text[length] = '\0';
    *_out_length = length;

    return text;
}


#if defined(_WIN32)

/*
d_internal_is_separator, d_internal_make_directory, d_internal_is_directory
  Whether a byte separates a path's parts -- `/` and `\` -- a directory
made, 0 when it is, and whether a path is a directory.
*/
static bool
d_internal_is_separator(
    char _c
)
{
    return ( (_c == '/') || (_c == '\\') );
}


static int
d_internal_make_directory(
    const char* _path
)
{
    return _mkdir(_path);
}


static bool
d_internal_is_directory(
    const char* _path
)
{
    struct _stat info;

    return ( (_stat(_path, &info) == 0) &&
             ( (info.st_mode & _S_IFDIR) != 0 ) );
}

#else

/*
d_internal_is_separator, d_internal_make_directory, d_internal_is_directory
  Whether a byte separates a path's parts -- `/` -- a directory made, 0
when it is, and whether a path is a directory.
*/
static bool
d_internal_is_separator(
    char _c
)
{
    return (_c == '/');
}


static int
d_internal_make_directory(
    const char* _path
)
{
    return mkdir(_path, 0777);
}


static bool
d_internal_is_directory(
    const char* _path
)
{
    struct stat info;

    return ( (stat(_path, &info) == 0) &&
             (S_ISDIR(info.st_mode)) );
}

#endif  // _WIN32


/*
d_internal_make_parents
  Every directory a file's path names that is missing.
*/
static bool
d_internal_make_parents(
    const char* _path
)
{
    const size_t length = strlen(_path);
    char* const  copy   = malloc(length + 1u);

    // a copy to cut at each separator
    if (!copy)
    {
        return false;
    }

    memcpy(copy, _path, length + 1u);

    // each separator after the first byte, its directory made
    for (size_t at = 1u; at < length; ++at)
    {
        // a byte that separates nothing
        if (!d_internal_is_separator(copy[at]))
        {
            continue;
        }

        copy[at] = '\0';

        // made, or there already: a directory that may not be made again
        // where it is, as a sandbox says, is there all the same
        const int made = d_internal_make_directory(copy);

        // made, or there already; past a drive's `C:` on Windows
        if ( (made != 0) &&
             (!d_internal_is_directory(copy)) &&
             ( (at < 2u) || (copy[at - 1u] != ':') ) )
        {
            free(copy);

            return false;
        }

        copy[at] = _path[at];
    }

    free(copy);

    return true;
}


/*
d_internal_write_file
  Writes a file beside its target, then renames it over the target.
*/
static bool
d_internal_write_file(
    const char* _path,
    const char* _text,
    size_t      _length,
    char*       _problem,
    size_t      _capacity
)
{
    const size_t length = strlen(_path);
    char* const  part   = malloc(length + 6u);

    // the name it is written to first
    if (!part)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    memcpy(part, _path, length);
    memcpy(&part[length], ".part", 6u);

    // the directories it goes in
    if (!d_internal_make_parents(_path))
    {
        (void)snprintf(_problem, _capacity,
                       "cannot make the directories of %s", _path);
        free(part);

        return false;
    }

    FILE* const file = fopen(part, "wb");

    // the file beside it
    if (!file)
    {
        (void)snprintf(_problem, _capacity, "cannot write %s", part);
        free(part);

        return false;
    }

    const bool written = (fwrite(_text, 1u, _length, file) == _length);
    const bool closed  = (fclose(file) == 0);

    // a rename that will not replace a file is tried again once it is gone
    bool renamed = (written) && (closed) && (rename(part, _path) == 0);

    // a rename that would not replace a file
    if ( (written) &&
         (closed) &&
         (!renamed) )
    {
        (void)remove(_path);
        renamed = (rename(part, _path) == 0);
    }

    // a file not renamed into place is taken away
    if (!renamed)
    {
        (void)remove(part);
        (void)snprintf(_problem, _capacity, "cannot write %s", _path);
    }

    free(part);

    return renamed;
}


//==============================================================================
// 3.  DOCUMENTS
//==============================================================================


struct d_wiki_doc*
d_wiki_doc_make(
    const char*         _path,
    const char*         _text,
    size_t              _length,
    enum d_wiki_dialect _dialect,
    char*               _problem,
    size_t              _capacity
)
{
    const char* const         path = (_path) ? _path : "";
    struct d_wiki_doc* const  doc  = calloc(1u, sizeof(*doc));

    // the document, its path and its text
    if (!doc)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return NULL;
    }

    doc->dialect = (_dialect != D_WIKI_OTHER) ? _dialect
                                              : d_wiki_dialect_of(path);
    doc->path    = d_wiki_copy(path, strlen(path));
    doc->text    = d_wiki_copy((_text) ? _text : "", (_text) ? _length : 0u);
    doc->length  = (_text) ? _length : 0u;

    // memory that ran out
    if ( (!doc->path) ||
         (!doc->text) )
    {
        (void)snprintf(_problem, _capacity, "out of memory");
        d_wiki_doc_free(doc);

        return NULL;
    }

    // a dialect a driver reads
    if (!d_wiki_driver_of(doc->dialect))
    {
        (void)snprintf(_problem, _capacity,
                       "%s: no wiki dialect is known by its name", path);
        d_wiki_doc_free(doc);

        return NULL;
    }

    // the text read, or the reason it was not
    if (!d_wiki_read(&doc->records, doc->text, doc->length, doc->dialect,
                     _problem, _capacity))
    {
        d_wiki_doc_free(doc);

        return NULL;
    }

    return doc;
}


struct d_wiki_doc*
d_wiki_doc_read(
    const char*         _path,
    enum d_wiki_dialect _dialect,
    char*               _problem,
    size_t              _capacity
)
{
    size_t      length = 0;
    char* const text   = (_path) ? d_internal_read_file(_path, &length)
                                 : NULL;

    // a file that cannot be read
    if (!text)
    {
        (void)snprintf(_problem, _capacity, "cannot read %s",
                       (_path) ? _path : "(no path)");

        return NULL;
    }

    struct d_wiki_doc* const doc = d_wiki_doc_make(_path, text, length,
                                                   _dialect, _problem,
                                                   _capacity);

    free(text);

    return doc;
}


void
d_wiki_doc_free(
    struct d_wiki_doc* _doc
)
{
    // freeing nothing is allowed
    if (!_doc)
    {
        return;
    }

    d_wiki_release(&_doc->records);
    free(_doc->text);
    free(_doc->path);
    free(_doc);

    return;
}


const struct d_wiki*
d_wiki_doc_records(
    const struct d_wiki_doc* _doc
)
{
    return &_doc->records;
}


const char*
d_wiki_doc_path(
    const struct d_wiki_doc* _doc
)
{
    return _doc->path;
}


enum d_wiki_dialect
d_wiki_doc_dialect(
    const struct d_wiki_doc* _doc
)
{
    return _doc->dialect;
}


size_t
d_wiki_doc_changes(
    const struct d_wiki_doc* _doc
)
{
    return _doc->changes;
}


bool
d_wiki_doc_changed(
    const struct d_wiki_doc* _doc,
    size_t*                  _out_start,
    size_t*                  _out_old_end,
    size_t*                  _out_new_end
)
{
    // nothing changed yet
    if ( (!_doc) ||
         (_doc->changes == 0u) )
    {
        return false;
    }

    *_out_start   = _doc->start;
    *_out_old_end = _doc->old_end;
    *_out_new_end = _doc->new_end;

    return true;
}


const char*
d_wiki_doc_problem(
    const struct d_wiki_doc* _doc
)
{
    return _doc->problem;
}


const char*
d_wiki_doc_text(
    const struct d_wiki_doc* _doc,
    size_t*                  _out_length
)
{
    // its length, when asked for
    if (_out_length)
    {
        *_out_length = _doc->length;
    }

    return _doc->text;
}


/*
d_internal_fail
  Says why a change failed, and gives false.
*/
static bool
d_internal_fail(
    struct d_wiki_doc* _doc,
    const char*        _why
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
  A place in the old text as it is in the new: moved by `_shift` when it is
after the splice; D_WIKI_NONE stays itself.
*/
static size_t
d_internal_moved(
    size_t    _at,
    ptrdiff_t _shift
)
{
    return (_at == D_WIKI_NONE) ? D_WIKI_NONE
                                : (size_t)((ptrdiff_t)_at + _shift);
}


static bool
d_internal_same_text(
    const char* _left,
    const char* _right
)
{
    return (strcmp((_left) ? _left : "", (_right) ? _right : "") == 0);
}


/*
d_internal_same_block, d_internal_same_heading, d_internal_same_link,
d_internal_same_template, d_internal_same_region
  Whether a record of the old text is one of the new, moved by `_shift`.
*/
static bool
d_internal_same_block(
    const struct d_wiki* _old,
    size_t               _left,
    const struct d_wiki* _new,
    size_t               _right,
    ptrdiff_t            _shift
)
{
    const struct d_wiki_block* const a = &_old->blocks[_left];
    const struct d_wiki_block* const b = &_new->blocks[_right];

    return ( (a->kind == b->kind) &&
             (d_internal_moved(a->start, _shift) == b->start) &&
             (d_internal_moved(a->end, _shift) == b->end) &&
             (a->depth == b->depth) &&
             (a->level == b->level) &&
             (a->marker == b->marker) &&
             (a->segment_count == b->segment_count) &&
             (d_internal_moved(a->info_start, _shift) == b->info_start) &&
             (d_internal_moved(a->info_end, _shift) == b->info_end) );
}


static bool
d_internal_same_heading(
    const struct d_wiki* _old,
    size_t               _left,
    const struct d_wiki* _new,
    size_t               _right,
    ptrdiff_t            _shift
)
{
    const struct d_wiki_heading* const a = &_old->headings[_left];
    const struct d_wiki_heading* const b = &_new->headings[_right];

    return ( (a->level == b->level) &&
             (a->top == b->top) &&
             (a->setext == b->setext) &&
             (d_internal_moved(a->start, _shift) == b->start) &&
             (d_internal_moved(a->end, _shift) == b->end) &&
             (d_internal_moved(a->title_start, _shift) == b->title_start) &&
             (d_internal_moved(a->title_end, _shift) == b->title_end) &&
             (d_internal_same_text(a->title, b->title)) );
}


static bool
d_internal_same_link(
    const struct d_wiki* _old,
    size_t               _left,
    const struct d_wiki* _new,
    size_t               _right,
    ptrdiff_t            _shift
)
{
    const struct d_wiki_link* const a = &_old->links[_left];
    const struct d_wiki_link* const b = &_new->links[_right];

    // a reference's destination is its definition's, which may have changed
    return ( (a->form == b->form) &&
             (a->embedded == b->embedded) &&
             (d_internal_moved(a->start, _shift) == b->start) &&
             (d_internal_moved(a->end, _shift) == b->end) &&
             (d_internal_moved(a->close, _shift) == b->close) &&
             (d_internal_moved(a->target_start, _shift) == b->target_start) &&
             (d_internal_moved(a->target_end, _shift) == b->target_end) &&
             (d_internal_moved(a->label_start, _shift) == b->label_start) &&
             (d_internal_moved(a->label_end, _shift) == b->label_end) &&
             (d_internal_moved(a->title_start, _shift) == b->title_start) &&
             (d_internal_moved(a->title_end, _shift) == b->title_end) &&
             (d_internal_same_text(a->label, b->label)) &&
             ( (a->form == D_WIKI_FORM_REFERENCE) ||
               ( (a->to == b->to) &&
                 (d_internal_same_text(a->target, b->target)) &&
                 (d_internal_same_text(a->page, b->page)) &&
                 (d_internal_same_text(a->anchor, b->anchor)) ) ) );
}


static bool
d_internal_same_template(
    const struct d_wiki* _old,
    size_t               _left,
    const struct d_wiki* _new,
    size_t               _right,
    ptrdiff_t            _shift
)
{
    const struct d_wiki_template* const a = &_old->templates[_left];
    const struct d_wiki_template* const b = &_new->templates[_right];

    // the template, then each of its arguments
    if ( (a->kind != b->kind) ||
         (d_internal_moved(a->start, _shift) != b->start) ||
         (d_internal_moved(a->end, _shift) != b->end) ||
         (d_internal_moved(a->name_start, _shift) != b->name_start) ||
         (d_internal_moved(a->name_end, _shift) != b->name_end) ||
         (a->argument_count != b->argument_count) ||
         (!d_internal_same_text(a->name, b->name)) )
    {
        return false;
    }

    // each argument, moved as the template is
    for (size_t index = 0; index < a->argument_count; ++index)
    {
        const struct d_wiki_argument* const x =
            &_old->arguments[a->first_argument + index];
        const struct d_wiki_argument* const y =
            &_new->arguments[b->first_argument + index];

        // where it is, and its key
        if ( (d_internal_moved(x->start, _shift) != y->start) ||
             (d_internal_moved(x->end, _shift) != y->end) ||
             (d_internal_moved(x->name_start, _shift) != y->name_start) ||
             (d_internal_moved(x->value_start, _shift) != y->value_start) ||
             (!d_internal_same_text(x->key, y->key)) )
        {
            return false;
        }
    }

    return true;
}


static bool
d_internal_same_region(
    const struct d_wiki* _old,
    size_t               _left,
    const struct d_wiki* _new,
    size_t               _right,
    ptrdiff_t            _shift
)
{
    const struct d_wiki_region* const a = &_old->regions[_left];
    const struct d_wiki_region* const b = &_new->regions[_right];

    return ( (d_internal_same_text(a->name, b->name)) &&
             (d_internal_moved(a->begin, _shift) == b->begin) &&
             (d_internal_moved(a->content, _shift) == b->content) &&
             (d_internal_moved(a->close, _shift) == b->close) &&
             (d_internal_moved(a->end, _shift) == b->end) );
}


// 4.1    Kinds of record
//------------------------------------------------------------------------------
// 4.1.1
// d_internal_fn_same
//   function: whether a record of the old text is one of the new.
typedef bool (*d_internal_fn_same)(const struct d_wiki* _old,
                                   size_t               _left,
                                   const struct d_wiki* _new,
                                   size_t               _right,
                                   ptrdiff_t            _shift);

// 4.1.2
// d_internal_fn_span
//   function: where a record of a kind begins and ends, by its index.
typedef void (*d_internal_fn_span)(const struct d_wiki* _wiki,
                                   size_t               _index,
                                   size_t*              _out_start,
                                   size_t*              _out_end);

// 4.1.3
// d_internal_kind
//   struct: a kind of record: its name, how many a page has, its span,
//   and its comparison.
struct d_internal_kind
{
    const char*        name;
    size_t             (*count)(const struct d_wiki* _wiki);
    d_internal_fn_span span;
    d_internal_fn_same same;
};


static size_t
d_internal_block_count(
    const struct d_wiki* _wiki
)
{
    return _wiki->block_count;
}


static void
d_internal_block_span(
    const struct d_wiki* _wiki,
    size_t               _index,
    size_t*              _out_start,
    size_t*              _out_end
)
{
    *_out_start = _wiki->blocks[_index].start;
    *_out_end   = _wiki->blocks[_index].end;

    return;
}


static size_t
d_internal_heading_count(
    const struct d_wiki* _wiki
)
{
    return _wiki->heading_count;
}


static void
d_internal_heading_span(
    const struct d_wiki* _wiki,
    size_t               _index,
    size_t*              _out_start,
    size_t*              _out_end
)
{
    *_out_start = _wiki->headings[_index].start;
    *_out_end   = _wiki->headings[_index].end;

    return;
}


static size_t
d_internal_link_count(
    const struct d_wiki* _wiki
)
{
    return _wiki->link_count;
}


static void
d_internal_link_span(
    const struct d_wiki* _wiki,
    size_t               _index,
    size_t*              _out_start,
    size_t*              _out_end
)
{
    *_out_start = _wiki->links[_index].start;
    *_out_end   = _wiki->links[_index].end;

    return;
}


static size_t
d_internal_template_count(
    const struct d_wiki* _wiki
)
{
    return _wiki->template_count;
}


static void
d_internal_template_span(
    const struct d_wiki* _wiki,
    size_t               _index,
    size_t*              _out_start,
    size_t*              _out_end
)
{
    *_out_start = _wiki->templates[_index].start;
    *_out_end   = _wiki->templates[_index].end;

    return;
}


static size_t
d_internal_region_count(
    const struct d_wiki* _wiki
)
{
    return _wiki->region_count;
}


static void
d_internal_region_span(
    const struct d_wiki* _wiki,
    size_t               _index,
    size_t*              _out_start,
    size_t*              _out_end
)
{
    *_out_start = _wiki->regions[_index].begin;
    *_out_end   = _wiki->regions[_index].end;

    return;
}


/*
d_internal_next_outside
  The next record of a kind, from `*_at`, that lies wholly before `_start`
or, `_after`, wholly from `_end` on: D_WIKI_NONE when there is no more.
*/
static size_t
d_internal_next_outside(
    const struct d_internal_kind* _kind,
    const struct d_wiki*          _wiki,
    size_t*                       _at,
    bool                          _after,
    size_t                        _start,
    size_t                        _end
)
{
    const size_t count = _kind->count(_wiki);

    // each record from the place, the first that lies where wanted
    while (*_at < count)
    {
        size_t       first = 0;
        size_t       last  = 0;
        const size_t index = (*_at)++;

        _kind->span(_wiki, index, &first, &last);

        // before, or after; an empty one where nothing is cut is before
        if ( ( (!_after) && (last <= _start) ) ||
             ( (_after) && (first >= _end) && (last > _start) ) )
        {
            return index;
        }
    }

    return D_WIKI_NONE;
}


/*
d_internal_outside_same
  Whether every record of a kind outside the splice is the page's own: the
same before it, and the same moved after it.  Gives the line of the first
that is not.
*/
static bool
d_internal_outside_same(
    const struct d_internal_kind*      _kind,
    const struct d_wiki*               _old,
    const struct d_internal_candidate* _candidate,
    size_t*                            _out_line
)
{
    const ptrdiff_t shift = (ptrdiff_t)_candidate->new_end -
                            (ptrdiff_t)_candidate->old_end;

    // before the splice, then after it
    for (int pass = 0; pass < 2; ++pass)
    {
        const bool after = (pass == 1);
        size_t     left  = 0;
        size_t     right = 0;

        // the next of each that lies outside, until neither has one
        for (;;)
        {
            const size_t a = d_internal_next_outside(_kind, _old, &left, after,
                                                     _candidate->start,
                                                     _candidate->old_end);
            const size_t b = d_internal_next_outside(_kind,
                                                     &_candidate->records,
                                                     &right, after,
                                                     _candidate->start,
                                                     _candidate->new_end);

            // both done: every one outside is as it was
            if ( (a == D_WIKI_NONE) &&
                 (b == D_WIKI_NONE) )
            {
                break;
            }

            // one more on one side, or two that differ
            if ( (a == D_WIKI_NONE) ||
                 (b == D_WIKI_NONE) ||
                 (!_kind->same(_old, a, &_candidate->records, b,
                               (after) ? shift : 0)) )
            {
                size_t first = 0;
                size_t last  = 0;

                // the line of the one the page had, or of the new one, moved
                // back
                if (a != D_WIKI_NONE)
                {
                    _kind->span(_old, a, &first, &last);
                }
                else
                {
                    _kind->span(&_candidate->records, b, &first, &last);
                    first = (after) ? (size_t)((ptrdiff_t)first - shift)
                                    : first;
                }

                *_out_line = d_wiki_line_of(_old, (first < _old->length)
                                                      ? first : _old->length,
                                            NULL);

                return false;
            }
        }
    }

    return true;
}


//==============================================================================
// 5.  CHANGES
//==============================================================================


/*
d_internal_with_endings
  Text with each line feed written as the page ends its lines.
*/
static bool
d_internal_with_endings(
    const struct d_wiki_doc* _doc,
    const char*              _text,
    size_t                   _length,
    struct d_wiki_buffer*    _out
)
{
    // each byte, a lone line feed after a carriage return on a CRLF page
    for (size_t at = 0; at < _length; ++at)
    {
        // a lone line feed, on a CR LF page, gets its CR
        if ( (_doc->records.crlf) &&
             (_text[at] == '\n') &&
             ( (at == 0u) || (_text[at - 1u] != '\r') ) )
        {
            (void)d_wiki_buffer_add_char(_out, '\r');
        }

        (void)d_wiki_buffer_add_char(_out, _text[at]);
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


static void
d_internal_discard(
    struct d_internal_candidate* _candidate
)
{
    d_wiki_release(&_candidate->records);
    free(_candidate->text);
    memset(_candidate, 0, sizeof(*_candidate));

    return;
}


/*
d_internal_try
  Makes a candidate of a splice, and keeps it only if the page outside it
reads as it did.
*/
static bool
d_internal_try(
    struct d_wiki_doc*           _doc,
    size_t                       _start,
    size_t                       _end,
    const char*                  _text,
    size_t                       _length,
    struct d_internal_candidate* _out
)
{
    static const struct d_internal_kind kinds[] =
    {
        { "block",    d_internal_block_count,    d_internal_block_span,
          d_internal_same_block    },
        { "heading",  d_internal_heading_count,  d_internal_heading_span,
          d_internal_same_heading  },
        { "link",     d_internal_link_count,     d_internal_link_span,
          d_internal_same_link     },
        { "template", d_internal_template_count, d_internal_template_span,
          d_internal_same_template },
        { "region",   d_internal_region_count,   d_internal_region_span,
          d_internal_same_region   }
    };
    struct d_wiki_buffer insert = { NULL, 0u, 0u, false };
    char                 why[256];

    memset(_out, 0, sizeof(*_out));

    // the splice within the text
    if ( (_start > _end) ||
         (_end > _doc->length) )
    {
        return d_internal_fail(_doc, "the change is not within the page");
    }

    // the text, its line endings the page's
    if (!d_internal_with_endings(_doc, _text, _length, &insert))
    {
        d_wiki_buffer_free(&insert);

        return d_internal_fail(_doc, "out of memory");
    }

    _out->length  = _start + insert.length + (_doc->length - _end);
    _out->text    = malloc(_out->length + 1u);
    _out->start   = _start;
    _out->old_end = _end;
    _out->new_end = _start + insert.length;

    // memory that ran out
    if (!_out->text)
    {
        d_wiki_buffer_free(&insert);

        return d_internal_fail(_doc, "out of memory");
    }

    // the text before, the new text, the text after
    memcpy(_out->text, _doc->text, _start);

    // the new text, when there is any
    if (insert.length > 0u)
    {
        memcpy(&_out->text[_start], insert.text, insert.length);
    }

    memcpy(&_out->text[_out->new_end], &_doc->text[_end],
           _doc->length - _end);
    _out->text[_out->length] = '\0';
    d_wiki_buffer_free(&insert);

    // the new text read, or the change refused
    if (!d_wiki_read(&_out->records, _out->text, _out->length, _doc->dialect,
                     why, sizeof(why)))
    {
        d_internal_discard(_out);

        return d_internal_fail(_doc, why);
    }

    // every kind of record outside the splice as it was
    for (size_t index = 0; index < (sizeof(kinds) / sizeof(kinds[0]));
         ++index)
    {
        size_t line = 0;

        // each kind outside the splice as it was, or the change refused
        if (!d_internal_outside_same(&kinds[index], &_doc->records, _out,
                                     &line))
        {
            (void)snprintf(_doc->problem, sizeof(_doc->problem),
                           "the change would change the %s at line %zu, "
                           "outside it", kinds[index].name, line);
            d_internal_discard(_out);

            return false;
        }
    }

    return true;
}


/*
d_internal_compose
  Where a change was, made one with the splice after it: the first byte
either replaced, and, in the text after both, past the last either wrote.
*/
static void
d_internal_compose(
    struct d_wiki_doc*                 _doc,
    const struct d_internal_candidate* _candidate
)
{
    const ptrdiff_t first  = (ptrdiff_t)_doc->new_end -
                             (ptrdiff_t)_doc->old_end;
    const ptrdiff_t second = (ptrdiff_t)_candidate->new_end -
                             (ptrdiff_t)_candidate->old_end;
    size_t          end    = _doc->new_end;

    // the first's end, where the second leaves it
    if (end >= _candidate->old_end)
    {
        end = (size_t)((ptrdiff_t)end + second);
    }
    else if (end > _candidate->start)
    {
        end = _candidate->new_end;
    }

    _doc->start   = (_candidate->start < _doc->start) ? _candidate->start
                                                      : _doc->start;
    _doc->new_end = (_candidate->new_end > end) ? _candidate->new_end : end;
    _doc->old_end = (size_t)((ptrdiff_t)_doc->new_end - first - second);

    return;
}


/*
d_internal_commit
  Keeps a candidate: the page is its text and records now, and what the
page was goes to `_kept`, or is freed.  A change kept to `_kept` is the
first splice of two, counted with the second, and where it was made one
with where the second is.
*/
static void
d_internal_commit(
    struct d_wiki_doc*           _doc,
    struct d_internal_candidate* _candidate,
    struct d_internal_kept*      _kept
)
{
    // what the page was
    if (_kept)
    {
        _kept->text    = _doc->text;
        _kept->length  = _doc->length;
        _kept->records = _doc->records;
        _kept->changes = _doc->changes;
        _kept->start   = _doc->start;
        _kept->old_end = _doc->old_end;
        _kept->new_end = _doc->new_end;
    }
    else
    {
        d_wiki_release(&_doc->records);
        free(_doc->text);
    }

    // where the change was, with the first splice of two
    if (_doc->composing)
    {
        d_internal_compose(_doc, _candidate);
    }
    else
    {
        _doc->start   = _candidate->start;
        _doc->old_end = _candidate->old_end;
        _doc->new_end = _candidate->new_end;
    }

    _doc->composing = (_kept != NULL);
    _doc->changes  += (_kept) ? 0u : 1u;
    _doc->text      = _candidate->text;
    _doc->length    = _candidate->length;
    _doc->records   = _candidate->records;
    _doc->problem[0] = '\0';
    memset(_candidate, 0, sizeof(*_candidate));

    return;
}


/*
d_internal_restore, d_internal_release_kept
  Goes back to what the page was, or lets it go.
*/
static void
d_internal_restore(
    struct d_wiki_doc*      _doc,
    struct d_internal_kept* _kept
)
{
    d_wiki_release(&_doc->records);
    free(_doc->text);
    _doc->text      = _kept->text;
    _doc->length    = _kept->length;
    _doc->records   = _kept->records;
    _doc->changes   = _kept->changes;
    _doc->start     = _kept->start;
    _doc->old_end   = _kept->old_end;
    _doc->new_end   = _kept->new_end;
    _doc->composing = false;
    memset(_kept, 0, sizeof(*_kept));

    return;
}


static void
d_internal_release_kept(
    struct d_internal_kept* _kept
)
{
    // nothing kept is nothing to let go
    if (!_kept->text)
    {
        return;
    }

    d_wiki_release(&_kept->records);
    free(_kept->text);
    memset(_kept, 0, sizeof(*_kept));

    return;
}


bool
d_wiki_doc_splice(
    struct d_wiki_doc* _doc,
    size_t             _start,
    size_t             _end,
    const char*        _text,
    size_t             _length
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

    d_internal_commit(_doc, &candidate, NULL);

    return true;
}


/*
d_internal_raw_is
  Whether a span of a text is the bytes given, line endings aside.
*/
static bool
d_internal_raw_is(
    const struct d_wiki_doc* _doc,
    const char*              _text,
    size_t                   _start,
    size_t                   _end,
    const char*              _wanted
)
{
    struct d_wiki_buffer wanted = { NULL, 0u, 0u, false };
    bool                 same   = false;

    // spans not written are not the text
    if ( (_start == D_WIKI_NONE) ||
         (_end < _start) )
    {
        return false;
    }

    // the text wanted, its line endings the page's
    if (d_internal_with_endings(_doc, _wanted, strlen(_wanted), &wanted))
    {
        same = (wanted.length == (_end - _start)) &&
               ( (wanted.length == 0u) ||
                 (memcmp(wanted.text, &_text[_start], wanted.length) == 0) );
    }

    d_wiki_buffer_free(&wanted);

    return same;
}


//==============================================================================
// 6.  LINKS
//==============================================================================


/*
d_internal_find_link_at
  The link of a form that starts at an offset; D_WIKI_NONE for none.
*/
static size_t
d_internal_find_link_at(
    const struct d_wiki* _wiki,
    size_t               _start,
    enum d_wiki_form     _form
)
{
    // each link; the one that starts there in the form
    for (size_t index = 0; index < _wiki->link_count; ++index)
    {
        // a link that starts there, of that form
        if ( (_wiki->links[index].start == _start) &&
             (_wiki->links[index].form == _form) )
        {
            return index;
        }
    }

    return D_WIKI_NONE;
}


/*
d_internal_two_spans
  One splice that replaces two spans of the text, the first before the
second, keeping what is between them; a span with no text written is not
replaced.
*/
static bool
d_internal_two_spans(
    struct d_wiki_doc*            _doc,
    const struct d_internal_span* _a,
    const struct d_internal_span* _b,
    struct d_internal_candidate*  _out
)
{
    struct d_wiki_buffer middle = { NULL, 0u, 0u, false };

    // one span, or both, in order
    if (!_a->text)
    {
        return d_internal_try(_doc, _b->start, _b->end, _b->text,
                              strlen(_b->text), _out);
    }

    // only the first
    if (!_b->text)
    {
        return d_internal_try(_doc, _a->start, _a->end, _a->text,
                              strlen(_a->text), _out);
    }

    // the first before the second
    if (_a->start > _b->start)
    {
        return d_internal_two_spans(_doc, _b, _a, _out);
    }

    // two that overlap are one change, not two
    if (_a->end > _b->start)
    {
        return d_internal_fail(_doc, "the link's parts overlap");
    }

    // the first's text, what is between, the second's: one splice
    (void)d_wiki_buffer_add_text(&middle, _a->text);
    (void)d_wiki_buffer_add(&middle, &_doc->text[_a->end],
                            _b->start - _a->end);
    (void)d_wiki_buffer_add_text(&middle, _b->text);

    const bool ok = (!middle.failed) &&
                    (d_internal_try(_doc, _a->start, _b->end, middle.text,
                                    middle.length, _out));

    d_wiki_buffer_free(&middle);

    return ok;
}


/*
d_internal_label_change
  Where a label goes, and what is written there: a label written replaced;
a label added to [[...]] before its page in Gollum's order, or after it
in MediaWiki's; a label "" taking the label and its `|` away.
*/
static bool
d_internal_label_change(
    struct d_wiki_doc*        _doc,
    const struct d_wiki_link* _link,
    const char*               _label,
    struct d_internal_span*   _out_span,
    struct d_wiki_buffer*     _out
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_doc->dialect);
    const bool                        wiki   = (_link->form ==
                                                D_WIKI_FORM_WIKI);
    const bool                        gollum = (_doc->dialect ==
                                                D_WIKI_MARKDOWN);

    // a link that shows its destination has no label
    if ( ( (_link->label_start == D_WIKI_NONE) &&
           ( (!wiki) || (_link->to == D_WIKI_TO_IMAGE) ) ) ||
         (_link->form == D_WIKI_FORM_DEFINITION) )
    {
        return d_internal_fail(_doc, "this link has no label to set");
    }

    // a label "" in [[...]]: the label and its `|` go
    if ( (wiki) &&
         (_label[0] == '\0') )
    {
        // no label to take away: nothing changes
        if (_link->label_start == D_WIKI_NONE)
        {
            _out_span->start = _link->target_start;
            _out_span->end   = _link->target_start;

            return true;
        }

        _out_span->start = (gollum) ? _link->label_start : _link->target_end;
        _out_span->end   = (gollum) ? _link->target_start : _link->label_end;

        return true;
    }

    // a label added to [[...]], with its `|`
    if (_link->label_start == D_WIKI_NONE)
    {
        // Gollum's label before its page, then `|`
        if ( (!driver->label(_out, _link, _label)) ||
             ( (gollum) && (!d_wiki_buffer_add_char(_out, '|')) ) )
        {
            return d_internal_fail(_doc, "the label cannot be written in "
                                         "this link's form");
        }

        // MediaWiki's `|` before its label
        if (!gollum)
        {
            char* const label = d_wiki_buffer_take(_out);

            (void)d_wiki_buffer_add_char(_out, '|');
            (void)d_wiki_buffer_add_text(_out, label);
            free(label);
        }

        _out_span->start = (gollum) ? _link->target_start : _link->target_end;
        _out_span->end   = _out_span->start;

        return !_out->failed;
    }

    // a label written, replaced
    if (!driver->label(_out, _link, _label))
    {
        return d_internal_fail(_doc, "the label cannot be written in this "
                                     "link's form");
    }

    _out_span->start = _link->label_start;
    _out_span->end   = _link->label_end;

    return true;
}


/*
d_internal_link_texts
  What a change to a link writes: its destination, in its place or in its
definition's, and its label, with where the label goes.  Gives whether
both can be written in the link's form, the document saying why not.
*/
static bool
d_internal_link_texts(
    struct d_wiki_doc*             _doc,
    struct d_internal_link_change* _change
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_doc->dialect);
    const struct d_wiki_link* const   link   = &_change->link;

    // what the destination is written as, in its place or its definition's
    if (_change->target)
    {
        const struct d_wiki_link* const holder =
            (_change->refers)
                ? ( (link->definition != D_WIKI_NONE)
                        ? &_doc->records.links[link->definition] : NULL )
                : link;

        // a reference without a definition, or a link whose destination is not
        // written
        if ( (!holder) ||
             (holder->target_start == D_WIKI_NONE) )
        {
            return d_internal_fail(_doc, "this link's destination is not "
                                         "written where it can be set");
        }

        // the destination in the link's form
        if (!driver->target(&_change->target_text, holder, _change->target))
        {
            return d_internal_fail(_doc, "the destination cannot be written "
                                         "in this link's form");
        }
    }

    // what the label is written as, and where
    if ( (_change->label) &&
         (!d_internal_label_change(_doc, link, _change->label,
                                   &_change->label_span,
                                   &_change->label_text)) )
    {
        return false;
    }

    (void)d_wiki_buffer_add(&_change->label_text, "", 0u);
    (void)d_wiki_buffer_add(&_change->target_text, "", 0u);

    return (!_change->label_text.failed) && (!_change->target_text.failed);
}


/*
d_internal_link_splices
  The splices a change to a link makes: one over its destination and its
label; or, for a reference given both, its label here first, kept to
`_kept` to go back to, then its destination in its definition.
*/
static bool
d_internal_link_splices(
    struct d_wiki_doc*             _doc,
    struct d_internal_link_change* _change,
    struct d_internal_kept*        _kept,
    struct d_internal_candidate*   _out
)
{
    const bool             both   = (_change->refers) &&
                                    (_change->target) &&
                                    (_change->label);
    struct d_internal_span target = { 0u, 0u, NULL };
    struct d_internal_span label  = _change->label_span;

    // a reference's label here and its destination in its definition: two
    if (both)
    {
        // the label here first, kept to go back to
        if (!d_internal_try(_doc, label.start, label.end,
                            _change->label_text.text,
                            _change->label_text.length, _out))
        {
            return false;
        }

        d_internal_commit(_doc, _out, _kept);
    }

    // where the destination is written: here, or in the definition now
    if (_change->target)
    {
        const struct d_wiki_link* const holder =
            (_change->refers)
                ? &_doc->records.links[_change->link.definition]
                : &_change->link;

        target.start = holder->target_start;
        target.end   = holder->target_end;
        target.text  = _change->target_text.text;
    }

    label.text = ( (_change->label) && (!both) ) ? _change->label_text.text
                                                 : NULL;

    return d_internal_two_spans(_doc, &target, &label, _out);
}


/*
d_internal_link_reads_back
  Whether the changed link reads back as asked: found where it was, moved
by the splice, with as many links on the page, leading where it was told
and showing the label it was given.
*/
static bool
d_internal_link_reads_back(
    const struct d_wiki_doc*             _doc,
    const struct d_internal_link_change* _change,
    const struct d_internal_candidate*   _candidate
)
{
    const struct d_wiki_link* const link  = &_change->link;
    const struct d_wiki* const      now   = &_candidate->records;
    const size_t                    moved = (link->start >= _candidate->old_end)
                                                ? (link->start +
                                                   _candidate->new_end -
                                                   _candidate->old_end)
                                                : link->start;
    const size_t                    found = d_internal_find_link_at(
                                                now, moved, link->form);

    // the link, and as many as there were
    if ( (found == D_WIKI_NONE) ||
         (now->link_count != _doc->records.link_count) )
    {
        return false;
    }

    const size_t where = (_change->refers) ? now->links[found].definition
                                           : found;

    // where it leads, in its place or its definition's
    if ( (_change->target) &&
         ( (where == D_WIKI_NONE) ||
           (!d_internal_raw_is(_doc, _candidate->text,
                               now->links[where].target_start,
                               now->links[where].target_end,
                               _change->target_text.text)) ) )
    {
        return false;
    }

    // what it shows: Gollum's and MediaWiki's [[Page]] given a label shows
    // it as given
    return (!_change->label) ||
           (_change->label[0] == '\0') ||
           (d_internal_raw_is(_doc, _candidate->text,
                              now->links[found].label_start,
                              now->links[found].label_end,
                              ( (link->form == D_WIKI_FORM_WIKI) &&
                                (link->label_start == D_WIKI_NONE) )
                                  ? _change->label
                                  : _change->label_text.text));
}


bool
d_wiki_doc_set_link(
    struct d_wiki_doc* _doc,
    size_t             _link,
    const char*        _target,
    const char*        _label
)
{
    struct d_internal_candidate   candidate;
    struct d_internal_kept        kept;
    struct d_internal_link_change change;
    bool                          ok = false;

    // parameter validation first
    if ( (!_doc) ||
         (_link >= _doc->records.link_count) )
    {
        return (_doc) && (d_internal_fail(_doc, "no link of that number"));
    }

    memset(&kept, 0, sizeof(kept));
    memset(&change, 0, sizeof(change));
    change.link   = _doc->records.links[_link];
    change.refers = (change.link.form == D_WIKI_FORM_REFERENCE);
    change.target = _target;
    change.label  = _label;

    // written, spliced, and read back as asked, or not kept
    if ( (d_internal_link_texts(_doc, &change)) &&
         (d_internal_link_splices(_doc, &change, &kept, &candidate)) )
    {
        ok = d_internal_link_reads_back(_doc, &change, &candidate);

        // kept, or let go with the reason
        if (ok)
        {
            d_internal_commit(_doc, &candidate, NULL);
        }
        else
        {
            d_internal_discard(&candidate);
            (void)d_internal_fail(_doc, "the link would not read back as "
                                        "asked");
        }
    }

    // a first splice whose second was refused is undone
    if ( (!ok) &&
         (kept.text) )
    {
        char problem[512];

        memcpy(problem, _doc->problem, sizeof(problem));
        d_internal_restore(_doc, &kept);
        memcpy(_doc->problem, problem, sizeof(problem));
    }

    d_internal_release_kept(&kept);
    d_wiki_buffer_free(&change.target_text);
    d_wiki_buffer_free(&change.label_text);

    return ok;
}


bool
d_wiki_doc_unlink(
    struct d_wiki_doc* _doc,
    size_t             _link
)
{
    struct d_wiki_buffer text = { NULL, 0u, 0u, false };

    // parameter validation first
    if ( (!_doc) ||
         (_link >= _doc->records.link_count) )
    {
        return (_doc) && (d_internal_fail(_doc, "no link of that number"));
    }

    const struct d_wiki_driver* const driver = d_wiki_driver_of(_doc->dialect);
    const struct d_wiki_link          link   = _doc->records.links[_link];
    const char* const                 page   = _doc->text;

    // a definition is used by its references; HTML's link closes elsewhere
    if (link.form == D_WIKI_FORM_DEFINITION)
    {
        return d_internal_fail(_doc, "a definition is not a link to undo");
    }

    // HTML's link closes elsewhere; an image's link= is undone in its image
    if (link.form == D_WIKI_FORM_HTML)
    {
        return d_internal_fail(_doc, "an HTML link is undone in its HTML");
    }
    else if (link.form == D_WIKI_FORM_OPTION)
    {
        return d_internal_fail(_doc, "an image's link= is undone in its "
                                     "image");
    }

    // what it shows, as written: its label, or its page, and its trail
    if ( (link.to == D_WIKI_TO_IMAGE) ||
         (link.to == D_WIKI_TO_CATEGORY) ||
         (link.to == D_WIKI_TO_TAG) )
    {
        (void)d_wiki_buffer_add(&text, "", 0u);
    }
    else if (link.label_start != D_WIKI_NONE)
    {
        (void)d_wiki_buffer_add(&text, &page[link.label_start],
                                link.label_end - link.label_start);
    }
    else if (link.form == D_WIKI_FORM_WIKI)
    {
        size_t start = link.target_start;

        // MediaWiki's `:` and spaces before a page's name are not what it shows
        while ( (start < link.target_end) &&
                ( (page[start] == ':') || (page[start] == ' ') ) )
        {
            start++;
        }

        (void)d_wiki_buffer_add(&text, &page[start], link.target_end - start);
    }
    else
    {
        (void)driver->escape(&text, ( (link.label) && (link.label[0] != '\0') )
                                        ? link.label : link.target);
    }

    // MediaWiki's trail after `]]` is what it shows too
    if (link.end > link.close)
    {
        (void)d_wiki_buffer_add(&text, &page[link.close],
                                link.end - link.close);
    }

    (void)d_wiki_buffer_add(&text, "", 0u);

    struct d_internal_candidate candidate;
    const size_t                before = _doc->records.link_count;
    bool                        ok     = (!text.failed) &&
                                         (d_internal_try(_doc, link.start,
                                                         link.end, text.text,
                                                         text.length,
                                                         &candidate));

    d_wiki_buffer_free(&text);

    // one link fewer, or the page is left as it was
    if ( (ok) &&
         (candidate.records.link_count >= before) )
    {
        d_internal_discard(&candidate);
        ok = d_internal_fail(_doc, "the link's text would still be a link");
    }

    // kept, or let go with the reason
    if (ok)
    {
        d_internal_commit(_doc, &candidate, NULL);
    }

    return ok;
}


//==============================================================================
// 7.  SECTIONS
//==============================================================================


/*
d_internal_trimmed_length
  The length of text without the line endings at its end.
*/
static size_t
d_internal_trimmed_length(
    const char* _text
)
{
    size_t length = strlen(_text);

    // line endings at the end
    while ( (length > 0u) &&
            ( (_text[length - 1u] == '\n') || (_text[length - 1u] == '\r') ) )
    {
        length--;
    }

    return length;
}


/*
d_internal_ends_line
  Whether the text before an offset ends a line, or there is none.
*/
static bool
d_internal_ends_line(
    const struct d_wiki_doc* _doc,
    size_t                   _at
)
{
    const size_t begin = (_doc->records.bom) ? 3u : 0u;

    return ( (_at <= begin) ||
             (_doc->text[_at - 1u] == '\n') ||
             (_doc->text[_at - 1u] == '\r') );
}


/*
d_internal_blank_before
  Whether the line before an offset is blank, or there is none.
*/
static bool
d_internal_blank_before(
    const struct d_wiki_doc* _doc,
    size_t                   _at
)
{
    const size_t begin = (_doc->records.bom) ? 3u : 0u;
    size_t       at    = _at;

    // back past the line ending before the offset
    if ( (at > begin) &&
         (_doc->text[at - 1u] == '\n') )
    {
        at--;
    }

    // and past its CR
    if ( (at > begin) &&
         (_doc->text[at - 1u] == '\r') )
    {
        at--;
    }

    // the line before: only space, to its start
    while ( (at > begin) &&
            (_doc->text[at - 1u] != '\n') &&
            (_doc->text[at - 1u] != '\r') )
    {
        // a byte other than space: the line is not blank
        if ( (_doc->text[at - 1u] != ' ') &&
             (_doc->text[at - 1u] != '\t') )
        {
            return false;
        }

        at--;
    }

    return true;
}


/*
d_internal_body_text
  A section's body as it is written: a blank line after its heading when
it had one, the text, a line ending, and a blank line before what heading
follows.
*/
static bool
d_internal_body_text(
    const struct d_wiki_doc* _doc,
    size_t                   _section,
    const char*              _text,
    struct d_wiki_buffer*    _out
)
{
    const struct d_wiki_section* const section = &_doc->records.sections[
                                                     _section];
    const size_t                       length  = d_internal_trimmed_length(
                                                     _text);
    const bool                         follows = (section->own_end <
                                                  _doc->length);
    const bool                         empty   = (section->own_end ==
                                                  section->body);
    bool                               gap     = false;

    // a heading that ends the page without a line ending gets one
    if (!d_internal_ends_line(_doc, section->body))
    {
        (void)d_wiki_buffer_add_char(_out, '\n');
    }

    // the blank line after the heading, as the body had it
    if (_section > 0u)
    {
        size_t at = section->body;

        // past the space after the heading
        while ( (at < section->own_end) &&
                ( (_doc->text[at] == ' ') || (_doc->text[at] == '\t') ) )
        {
            at++;
        }

        gap = (empty) ||
              ( (at < section->own_end) &&
                ( (_doc->text[at] == '\n') || (_doc->text[at] == '\r') ) );
    }

    // no body: a line ending, if a section follows
    if (length == 0u)
    {
        (void)d_wiki_buffer_add_text(_out, (follows) ? "\n" : "");

        return !_out->failed;
    }

    // the blank line the body had after its heading
    if (gap)
    {
        (void)d_wiki_buffer_add_char(_out, '\n');
    }

    (void)d_wiki_buffer_add(_out, _text, length);
    (void)d_wiki_buffer_add_text(_out, (follows) ? "\n\n" : "\n");

    return !_out->failed;
}


bool
d_wiki_doc_set_body(
    struct d_wiki_doc* _doc,
    size_t             _section,
    const char*        _text
)
{
    struct d_internal_candidate candidate;
    struct d_wiki_buffer        body = { NULL, 0u, 0u, false };

    // parameter validation first
    if ( (!_doc) ||
         (_section >= _doc->records.section_count) )
    {
        return (_doc) && (d_internal_fail(_doc, "no section of that number"));
    }

    const struct d_wiki_section section = _doc->records.sections[_section];
    const size_t                count   = _doc->records.section_count;

    // the body as written, its blank lines kept
    if (!d_internal_body_text(_doc, _section, (_text) ? _text : "", &body))
    {
        d_wiki_buffer_free(&body);

        return d_internal_fail(_doc, "out of memory");
    }

    bool ok = d_internal_try(_doc, section.body, section.own_end, body.text,
                             body.length, &candidate);

    d_wiki_buffer_free(&body);

    // a body holds no heading: the sections are as they were
    if ( (ok) &&
         (candidate.records.section_count != count) )
    {
        d_internal_discard(&candidate);
        ok = d_internal_fail(_doc, "the text holds a heading: insert a "
                                   "section for it");
    }

    // kept, or let go with the reason
    if (ok)
    {
        d_internal_commit(_doc, &candidate, NULL);
    }

    return ok;
}


/*
d_internal_headings_spaced
  Whether the page leaves a blank line after a heading: as its first
heading at the top that has a line after it does, or, on a page with none,
as Markdown is written and MediaWiki's is not.
*/
static bool
d_internal_headings_spaced(
    const struct d_wiki_doc* _doc
)
{
    // each heading at the top, until one has a line after it
    for (size_t index = 0; index < _doc->records.heading_count; ++index)
    {
        const struct d_wiki_heading* const heading =
            &_doc->records.headings[index];
        size_t                             at      = heading->end;

        // a heading within a list or a template is no section's
        if (!heading->top)
        {
            continue;
        }

        // past the rest of its line, then the next line's space
        while ( (at < _doc->length) &&
                (_doc->text[at - 1u] != '\n') )
        {
            at++;
        }

        // past the next line's space
        while ( (at < _doc->length) &&
                ( (_doc->text[at] == ' ') || (_doc->text[at] == '\t') ||
                  (_doc->text[at] == '\r') ) )
        {
            at++;
        }

        // a line with something on it, or none
        if (at < _doc->length)
        {
            return (_doc->text[at] == '\n');
        }
    }

    return (_doc->dialect != D_WIKI_MEDIAWIKI);
}


/*
d_internal_section_change
  Replaces a span with text that is a run of sections, a line ending after
it and a blank line before what heading follows; `_heading` says the text
must begin with a heading at the span's start, and `_added` how many
sections more the page must have after.
*/
static bool
d_internal_section_change(
    struct d_wiki_doc* _doc,
    size_t             _start,
    size_t             _end,
    const char*        _text,
    bool               _heading,
    long               _added
)
{
    struct d_internal_candidate candidate;
    struct d_wiki_buffer        text    = { NULL, 0u, 0u, false };
    struct d_wiki_buffer        prefix  = { NULL, 0u, 0u, false };
    const size_t                length  = d_internal_trimmed_length(_text);
    const bool                  follows = (_end < _doc->length);

    // a line ending before it, and a blank line, when the page needs them
    if (length > 0u)
    {
        // a line ending before it, unless at a line's start
        if (!d_internal_ends_line(_doc, _start))
        {
            (void)d_wiki_buffer_add_char(&text, '\n');
        }

        // a blank line before it, unless there is one
        if (!d_internal_blank_before(_doc, _start))
        {
            (void)d_wiki_buffer_add_char(&text, '\n');
        }

        (void)d_internal_with_endings(_doc, (text.text) ? text.text : "",
                                      text.length, &prefix);
        (void)d_wiki_buffer_add(&text, _text, length);
        (void)d_wiki_buffer_add_text(&text, (follows) ? "\n\n" : "\n");
    }

    (void)d_wiki_buffer_add(&text, "", 0u);

    const size_t at    = _start + prefix.length;
    const long   count = (long)_doc->records.section_count;

    d_wiki_buffer_free(&prefix);
    bool         ok    = (!text.failed) &&
                         (d_internal_try(_doc, _start, _end, text.text,
                                         text.length, &candidate));

    d_wiki_buffer_free(&text);

    // as many sections as asked, and a heading where it begins
    if (ok)
    {
        bool headed = !_heading;

        // a section that begins where the text begins
        for (size_t index = 1u; ( (!headed) &&
                                  (index < candidate.records.section_count) );
             ++index)
        {
            headed = (candidate.records.sections[index].start == at);
        }

        // a heading where asked, and as many sections as asked
        if ( (!headed) ||
             ((long)candidate.records.section_count != (count + _added)) )
        {
            d_internal_discard(&candidate);
            ok = d_internal_fail(_doc, (headed)
                                           ? "the text holds other headings "
                                             "than asked"
                                           : "the text does not begin with a "
                                             "heading at the page's top");
        }
    }

    // kept, or let go with the reason
    if (ok)
    {
        d_internal_commit(_doc, &candidate, NULL);
    }

    return ok;
}


/*
d_internal_subsections
  How many sections a section holds, below it.
*/
static size_t
d_internal_subsections(
    const struct d_wiki* _wiki,
    size_t               _section
)
{
    size_t count = 0;

    // those after it that start before it ends
    for (size_t index = _section + 1u; index < _wiki->section_count; ++index)
    {
        // the first section past its end
        if (_wiki->sections[index].start >= _wiki->sections[_section].end)
        {
            break;
        }

        count++;
    }

    return count;
}


bool
d_wiki_doc_set_section(
    struct d_wiki_doc* _doc,
    size_t             _section,
    const char*        _text
)
{
    // parameter validation first
    if ( (!_doc) ||
         (_section >= _doc->records.section_count) )
    {
        return (_doc) && (d_internal_fail(_doc, "no section of that number"));
    }

    // the lead has no heading: its body is all of it
    if (_section == 0u)
    {
        return d_wiki_doc_set_body(_doc, 0u, _text);
    }

    const struct d_wiki_section section = _doc->records.sections[_section];
    const size_t                held    = d_internal_subsections(
                                              &_doc->records, _section);
    const char* const           text    = (_text) ? _text : "";

    // nothing is the section removed
    if (d_internal_trimmed_length(text) == 0u)
    {
        return d_wiki_doc_remove_section(_doc, _section);
    }

    // the sections the text makes are counted when it is read
    struct d_wiki        sections;
    char                 why[128];
    struct d_wiki_buffer probe = { NULL, 0u, 0u, false };
    long                 made  = 0;

    (void)d_wiki_buffer_add_text(&probe, text);

    // the text read alone: the sections it holds
    if ( (probe.failed) ||
         (!d_wiki_read(&sections, probe.text, probe.length, _doc->dialect,
                       why, sizeof(why))) )
    {
        d_wiki_buffer_free(&probe);

        return d_internal_fail(_doc, "out of memory");
    }

    made = (long)sections.section_count - 1;
    d_wiki_release(&sections);
    d_wiki_buffer_free(&probe);

    return d_internal_section_change(_doc, section.start, section.end, text,
                                     true, made - (long)held - 1);
}


bool
d_wiki_doc_rename_section(
    struct d_wiki_doc* _doc,
    size_t             _section,
    const char*        _title
)
{
    struct d_wiki_buffer title = { NULL, 0u, 0u, false };

    // parameter validation first; the lead has no heading to retitle
    if ( (!_doc) ||
         (_section == 0u) ||
         (_section >= _doc->records.section_count) )
    {
        return (_doc) && (d_internal_fail(_doc, "no heading of that number"));
    }

    const struct d_wiki_heading heading = _doc->records.headings[
                                              _doc->records.sections[_section]
                                                  .heading];
    const size_t                count   = _doc->records.section_count;

    // the title on one line, with a closing # where Markdown would eat one
    for (const char* at = (_title) ? _title : ""; *at != '\0'; ++at)
    {
        (void)d_wiki_buffer_add_char(&title, ( (*at == '\n') || (*at == '\r') )
                                                 ? ' ' : *at);
    }

    // Markdown's title that ends in `#` keeps it after a closing sequence
    if ( (_doc->dialect == D_WIKI_MARKDOWN) &&
         (!heading.setext) &&
         (title.length > 0u) &&
         (title.text[title.length - 1u] == '#') )
    {
        (void)d_wiki_buffer_add_text(&title, " #");
    }

    (void)d_wiki_buffer_add(&title, "", 0u);

    struct d_internal_candidate candidate;
    bool                        ok = (!title.failed) &&
                                     (d_internal_try(_doc, heading.title_start,
                                                     heading.title_end,
                                                     title.text, title.length,
                                                     &candidate));

    d_wiki_buffer_free(&title);

    // the heading at its level still, and the sections as they were
    if (ok)
    {
        const struct d_wiki_section* const now =
            &candidate.records.sections[_section];

        // the same heading, at the same place and level
        if ( (candidate.records.section_count != count) ||
             (now->start != heading.start) ||
             (now->level != heading.level) )
        {
            d_internal_discard(&candidate);
            ok = d_internal_fail(_doc, "the title would change the heading's "
                                       "level or the sections");
        }
    }

    // kept, or let go with the reason
    if (ok)
    {
        d_internal_commit(_doc, &candidate, NULL);
    }

    return ok;
}


bool
d_wiki_doc_insert_section(
    struct d_wiki_doc* _doc,
    size_t             _before,
    size_t             _level,
    const char*        _title,
    const char*        _body
)
{
    struct d_wiki_buffer text = { NULL, 0u, 0u, false };

    // parameter validation first: before a section, or after the last
    if ( (!_doc) ||
         (_before == 0u) ||
         (_before > _doc->records.section_count) )
    {
        return (_doc) && (d_internal_fail(_doc, "no section of that number"));
    }

    const size_t at = (_before < _doc->records.section_count)
                          ? _doc->records.sections[_before].start
                          : _doc->length;

    // the heading, and its body
    if (!d_wiki_write_heading(&text, _doc->dialect, _level,
                              (_title) ? _title : ""))
    {
        d_wiki_buffer_free(&text);

        return d_internal_fail(_doc, "the heading cannot be written at that "
                                     "level");
    }

    // the body, when there is one
    if ( (_body) &&
         (d_internal_trimmed_length(_body) > 0u) )
    {
        // a blank line after the heading, as the page's others have
        if (d_internal_headings_spaced(_doc))
        {
            (void)d_wiki_buffer_add_char(&text, '\n');
        }

        (void)d_wiki_buffer_add(&text, _body, d_internal_trimmed_length(_body));
    }

    (void)d_wiki_buffer_add(&text, "", 0u);

    const bool ok = (!text.failed) &&
                    (d_internal_section_change(_doc, at, at, text.text, true,
                                               1));

    d_wiki_buffer_free(&text);

    return ok;
}


bool
d_wiki_doc_remove_section(
    struct d_wiki_doc* _doc,
    size_t             _section
)
{
    struct d_internal_candidate candidate;

    // parameter validation first
    if ( (!_doc) ||
         (_section >= _doc->records.section_count) )
    {
        return (_doc) && (d_internal_fail(_doc, "no section of that number"));
    }

    // the lead's body only
    if (_section == 0u)
    {
        return d_wiki_doc_set_body(_doc, 0u, "");
    }

    const struct d_wiki_section section = _doc->records.sections[_section];
    const size_t                held    = d_internal_subsections(
                                              &_doc->records, _section);
    const size_t                count   = _doc->records.section_count;
    bool                        ok      = d_internal_try(_doc, section.start,
                                                         section.end, "", 0u,
                                                         &candidate);

    // it, and what it held, gone
    if ( (ok) &&
         (candidate.records.section_count != (count - held - 1u)) )
    {
        d_internal_discard(&candidate);
        ok = d_internal_fail(_doc, "removing it would change other "
                                   "sections");
    }

    // kept, or let go with the reason
    if (ok)
    {
        d_internal_commit(_doc, &candidate, NULL);
    }

    return ok;
}


//==============================================================================
// 8.  REGIONS
//==============================================================================


/*
d_internal_region_holds
  Whether a region of the candidate holds the text asked.
*/
static bool
d_internal_region_holds(
    const struct d_wiki_doc*           _doc,
    const struct d_internal_candidate* _candidate,
    const char*                        _name,
    const char*                        _text
)
{
    const size_t index = d_wiki_find_region(&_candidate->records, _name);

    // the region, with the text between its markers
    return ( (index != D_WIKI_NONE) &&
             (d_internal_raw_is(_doc, _candidate->text,
                                _candidate->records.regions[index].content,
                                _candidate->records.regions[index].close,
                                _text)) );
}


/*
d_internal_region_text
  A region's text as it is written: a line ending after it.
*/
static bool
d_internal_region_text(
    const char*           _text,
    struct d_wiki_buffer* _out
)
{
    const size_t length = strlen(_text);

    (void)d_wiki_buffer_add(_out, _text, length);

    // the end marker on a line of its own
    if ( (length > 0u) &&
         (_text[length - 1u] != '\n') )
    {
        (void)d_wiki_buffer_add_char(_out, '\n');
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


bool
d_wiki_doc_set_region(
    struct d_wiki_doc* _doc,
    const char*        _name,
    const char*        _text
)
{
    struct d_internal_candidate candidate;
    struct d_wiki_buffer        text = { NULL, 0u, 0u, false };

    // parameter validation first
    if ( (!_doc) ||
         (!_name) )
    {
        return false;
    }

    const size_t index = d_wiki_find_region(&_doc->records, _name);

    // no region of that name
    if (index == D_WIKI_NONE)
    {
        return d_internal_fail(_doc, "no region of that name");
    }

    const struct d_wiki_region region = _doc->records.regions[index];
    const size_t               count  = _doc->records.region_count;
    bool                       ok     = (d_internal_region_text(
                                             (_text) ? _text : "", &text)) &&
                                        (d_internal_try(_doc, region.content,
                                                        region.close,
                                                        text.text, text.length,
                                                        &candidate));

    // the region holds the text, and the regions are as many
    if ( (ok) &&
         ( (candidate.records.region_count != count) ||
           (!d_internal_region_holds(_doc, &candidate, _name, text.text)) ) )
    {
        d_internal_discard(&candidate);
        ok = d_internal_fail(_doc, "the region would not hold the text as "
                                   "given");
    }

    // kept, or let go with the reason
    if (ok)
    {
        d_internal_commit(_doc, &candidate, NULL);
    }

    d_wiki_buffer_free(&text);

    return ok;
}


bool
d_wiki_doc_add_region(
    struct d_wiki_doc* _doc,
    const char*        _name,
    const char*        _text,
    size_t             _section
)
{
    struct d_internal_candidate candidate;
    struct d_wiki_buffer        text    = { NULL, 0u, 0u, false };
    struct d_wiki_buffer        content = { NULL, 0u, 0u, false };

    // parameter validation first: a name no region has, a section there is
    if ( (!_doc) ||
         (!_name) )
    {
        return false;
    }

    // a name a region has already
    if (d_wiki_find_region(&_doc->records, _name) != D_WIKI_NONE)
    {
        return d_internal_fail(_doc, "a region of that name is there");
    }

    // a section the page does not have
    if ( (_section != D_WIKI_NONE) &&
         (_section >= _doc->records.section_count) )
    {
        return d_internal_fail(_doc, "no section of that number");
    }

    const size_t floor = (_section == D_WIKI_NONE)
                             ? ((_doc->records.bom) ? 3u : 0u)
                             : _doc->records.sections[_section].body;
    size_t       at    = (_section == D_WIKI_NONE)
                             ? _doc->length
                             : _doc->records.sections[_section].own_end;

    // back past the blank lines that end the text, which stay after it
    while ( (at > floor) &&
            (d_internal_ends_line(_doc, at)) &&
            (d_internal_blank_before(_doc, at)) )
    {
        size_t line = at - 1u;

        line -= ( (line > floor) &&
                  (_doc->text[line] == '\n') &&
                  (_doc->text[line - 1u] == '\r') ) ? 1u : 0u;

        // back to the start of the line
        while ( (line > floor) &&
                (_doc->text[line - 1u] != '\n') &&
                (_doc->text[line - 1u] != '\r') )
        {
            line--;
        }

        at = line;
    }

    size_t next = at;

    // what follows it, if not a blank line already, after a blank line
    while ( (next < _doc->length) &&
            ( (_doc->text[next] == ' ') || (_doc->text[next] == '\t') ) )
    {
        next++;
    }

    const bool follows = (next < _doc->length) &&
                         (_doc->text[next] != '\n') &&
                         (_doc->text[next] != '\r');

    // a line ending, and a blank line, before it when the page needs them
    if (!d_internal_ends_line(_doc, at))
    {
        (void)d_wiki_buffer_add_char(&text, '\n');
    }

    // a blank line before it, unless there is one
    if (!d_internal_blank_before(_doc, at))
    {
        (void)d_wiki_buffer_add_char(&text, '\n');
    }

    // the region's text, with its markers
    if ( (!d_internal_region_text((_text) ? _text : "", &content)) ||
         (!d_wiki_write_region(&text, _doc->dialect, _name, content.text)) )
    {
        d_wiki_buffer_free(&text);
        d_wiki_buffer_free(&content);

        return d_internal_fail(_doc, "a region cannot have that name");
    }

    // a blank line after it, when something follows
    if (follows)
    {
        (void)d_wiki_buffer_add_char(&text, '\n');
    }

    const size_t count = _doc->records.region_count;
    bool         ok    = (!text.failed) &&
                         (d_internal_try(_doc, at, at, text.text, text.length,
                                         &candidate));

    // one region more, holding the text
    if ( (ok) &&
         ( (candidate.records.region_count != (count + 1u)) ||
           (!d_internal_region_holds(_doc, &candidate, _name,
                                     content.text)) ) )
    {
        d_internal_discard(&candidate);
        ok = d_internal_fail(_doc, "the region would not hold the text as "
                                   "given");
    }

    // kept, or let go with the reason
    if (ok)
    {
        d_internal_commit(_doc, &candidate, NULL);
    }

    d_wiki_buffer_free(&text);
    d_wiki_buffer_free(&content);

    return ok;
}


bool
d_wiki_doc_remove_region(
    struct d_wiki_doc* _doc,
    const char*        _name,
    bool               _keep
)
{
    struct d_internal_candidate candidate;
    struct d_wiki_buffer        text = { NULL, 0u, 0u, false };

    // parameter validation first
    if ( (!_doc) ||
         (!_name) )
    {
        return false;
    }

    const size_t index = d_wiki_find_region(&_doc->records, _name);

    // no region of that name
    if (index == D_WIKI_NONE)
    {
        return d_internal_fail(_doc, "no region of that name");
    }

    const struct d_wiki_region region = _doc->records.regions[index];
    const size_t               count  = _doc->records.region_count;

    // its text, kept, or nothing
    if (_keep)
    {
        (void)d_wiki_buffer_add(&text, &_doc->text[region.content],
                                region.close - region.content);
    }

    (void)d_wiki_buffer_add(&text, "", 0u);

    bool ok = (!text.failed) &&
              (d_internal_try(_doc, region.begin, region.end, text.text,
                              text.length, &candidate));

    // one region fewer, and the others as they were
    if ( (ok) &&
         (candidate.records.region_count != (count - 1u)) )
    {
        d_internal_discard(&candidate);
        ok = d_internal_fail(_doc, "removing it would change other regions");
    }

    // kept, or let go with the reason
    if (ok)
    {
        d_internal_commit(_doc, &candidate, NULL);
    }

    d_wiki_buffer_free(&text);

    return ok;
}


//==============================================================================
// 9.  TEMPLATES
//==============================================================================


/*
d_internal_find_argument
  The last argument of a template of a key; D_WIKI_NONE for none.
*/
static size_t
d_internal_find_argument(
    const struct d_wiki* _wiki,
    size_t               _template,
    const char*          _key
)
{
    const struct d_wiki_template* const tmpl  = &_wiki->templates[_template];
    size_t                              found = D_WIKI_NONE;

    // the last of the key wins, as MediaWiki reads it
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
d_internal_argument_reads
  Whether a candidate's template at a start has an argument of a key whose
value, trimmed, is the value asked, trimmed.
*/
static bool
d_internal_argument_reads(
    const struct d_internal_candidate* _candidate,
    size_t                             _start,
    const char*                        _key,
    const char*                        _value,
    bool                               _present
)
{
    const struct d_wiki* const records = &_candidate->records;

    // the template that starts there
    for (size_t index = 0; index < records->template_count; ++index)
    {
        // the template that begins where the changed one did
        if (records->templates[index].start != _start)
        {
            continue;
        }

        const size_t argument = d_internal_find_argument(records, index, _key);

        // an argument removed is not there
        if (!_present)
        {
            return (argument == D_WIKI_NONE);
        }

        // an argument set is there
        if (argument == D_WIKI_NONE)
        {
            return false;
        }

        size_t       from = records->arguments[argument].value_start;
        size_t       to   = records->arguments[argument].value_end;
        size_t       at   = 0;
        size_t       end  = strlen(_value);
        const char*  text = _candidate->text;

        // both trimmed of space and line endings
        while ( (from < to) &&
                (strchr(" \t\r\n", text[from]) != NULL) )
        {
            from++;
        }

        // the value read, trimmed as a named argument's is
        while ( (to > from) &&
                (strchr(" \t\r\n", text[to - 1u]) != NULL) )
        {
            to--;
        }

        // the value asked for, trimmed the same way
        while ( (at < end) &&
                (strchr(" \t\r\n", _value[at]) != NULL) )
        {
            at++;
        }

        // and before the space after it
        while ( (end > at) &&
                (strchr(" \t\r\n", _value[end - 1u]) != NULL) )
        {
            end--;
        }

        return ( ((to - from) == (end - at)) &&
                 ( (to == from) ||
                   (memcmp(&text[from], &_value[at], end - at) == 0) ) );
    }

    return false;
}


bool
d_wiki_doc_set_argument(
    struct d_wiki_doc* _doc,
    size_t             _template,
    const char*        _key,
    const char*        _value
)
{
    struct d_internal_candidate candidate;
    struct d_wiki_buffer        text = { NULL, 0u, 0u, false };
    size_t                      start = 0;
    size_t                      end   = 0;

    // parameter validation first
    if ( (!_doc) ||
         (!_key) ||
         (_key[0] == '\0') ||
         (_template >= _doc->records.template_count) )
    {
        return (_doc) && (d_internal_fail(_doc, "no template of that number"));
    }

    const struct d_wiki_template tmpl     = _doc->records.templates[_template];
    const char* const            value    = (_value) ? _value : "";
    const size_t                 argument = d_internal_find_argument(
                                                &_doc->records, _template,
                                                _key);

    // an argument there: its value, the space about a named one kept
    if (argument != D_WIKI_NONE)
    {
        const struct d_wiki_argument* const given =
            &_doc->records.arguments[argument];

        start = given->value_start;
        end   = given->value_end;

        // a named argument's value, past its spaces
        if (given->name_start != D_WIKI_NONE)
        {
            // past the space before it
            while ( (start < end) &&
                    (strchr(" \t", _doc->text[start]) != NULL) )
            {
                start++;
            }

            // and before the space after it
            while ( (end > start) &&
                    (strchr(" \t\r\n", _doc->text[end - 1u]) != NULL) )
            {
                end--;
            }
        }

        (void)d_wiki_buffer_add_text(&text, value);
    }
    else
    {
        size_t numbered = 0;
        bool   lines    = false;
        char   next[32];

        // the next number, and whether the arguments stand a line each
        for (size_t index = 0; index < tmpl.argument_count; ++index)
        {
            const struct d_wiki_argument* const given =
                &_doc->records.arguments[tmpl.first_argument + index];

            numbered += (given->name_start == D_WIKI_NONE) ? 1u : 0u;
            lines     = (lines) ||
                        (memchr(&_doc->text[given->start], '\n',
                                given->end - given->start) != NULL);
        }

        (void)snprintf(next, sizeof(next), "%zu", numbered + 1u);
        start = tmpl.end - ((tmpl.kind == D_WIKI_PARAMETER) ? 3u : 2u);
        end   = start;

        (void)d_wiki_buffer_add_text(&text, (lines) ? "| " : "|");

        // a named argument's key, or a positional one's, unless its number is
        // next
        if (strcmp(next, _key) != 0)
        {
            (void)d_wiki_buffer_add_text(&text, _key);
            (void)d_wiki_buffer_add_text(&text, (lines) ? " = " : "=");
        }

        (void)d_wiki_buffer_add_text(&text, value);

        // an argument on a line of its own ends its line
        if ( (lines) &&
             (start > 0u) &&
             (_doc->text[start - 1u] == '\n') )
        {
            (void)d_wiki_buffer_add_char(&text, '\n');
        }
    }

    bool ok = (!text.failed) &&
              (d_internal_try(_doc, start, end, text.text, text.length,
                              &candidate));

    // the template reads with the argument as asked
    if ( (ok) &&
         (!d_internal_argument_reads(&candidate, tmpl.start, _key, value,
                                     true)) )
    {
        d_internal_discard(&candidate);
        ok = d_internal_fail(_doc, "the argument would not read back as "
                                   "given");
    }

    // kept, or let go with the reason
    if (ok)
    {
        d_internal_commit(_doc, &candidate, NULL);
    }

    d_wiki_buffer_free(&text);

    return ok;
}


bool
d_wiki_doc_remove_argument(
    struct d_wiki_doc* _doc,
    size_t             _template,
    const char*        _key
)
{
    struct d_internal_candidate candidate;

    // parameter validation first
    if ( (!_doc) ||
         (!_key) ||
         (_template >= _doc->records.template_count) )
    {
        return (_doc) && (d_internal_fail(_doc, "no template of that number"));
    }

    const struct d_wiki_template tmpl     = _doc->records.templates[_template];
    const size_t                 argument = d_internal_find_argument(
                                                &_doc->records, _template,
                                                _key);

    // no argument of the key
    if (argument == D_WIKI_NONE)
    {
        return d_internal_fail(_doc, "the template has no argument of that "
                                     "key");
    }

    const struct d_wiki_argument given = _doc->records.arguments[argument];
    const bool                   first = (argument == tmpl.first_argument) &&
                                         (tmpl.kind == D_WIKI_FUNCTION);

    // the argument and the `|` before it; a function's first keeps its `:`
    bool ok = d_internal_try(_doc, (first) ? given.start : (given.start - 1u),
                             given.end, "", 0u, &candidate);

    bool numbered = true;

    // a numbered argument: the ones after it are renumbered
    for (const char* at = _key; *at != '\0'; ++at)
    {
        numbered = (numbered) && (*at >= '0') && (*at <= '9');
    }

    // a named argument is gone; a number may go to the next numbered one
    if ( (ok) &&
         (!numbered) &&
         (!d_internal_argument_reads(&candidate, tmpl.start, _key, "",
                                     false)) )
    {
        d_internal_discard(&candidate);
        ok = d_internal_fail(_doc, "the template would still have the "
                                   "argument");
    }

    // kept, or let go with the reason
    if (ok)
    {
        d_internal_commit(_doc, &candidate, NULL);
    }

    return ok;
}


//==============================================================================
// 10.  SAVING
//==============================================================================


enum d_wiki_saved
d_wiki_doc_save(
    struct d_wiki_doc* _doc,
    const char*        _path,
    char*              _problem,
    size_t             _capacity
)
{
    const char* const path = ( (_path) && (_path[0] != '\0') )
                                 ? _path
                                 : (_doc) ? _doc->path : NULL;
    size_t            length = 0;

    // a document, and somewhere to write it
    if ( (!path) ||
         (path[0] == '\0') )
    {
        (void)snprintf(_problem, _capacity, "the page has no path to save "
                                            "to");

        return D_WIKI_SAVE_FAILED;
    }

    char* const there = d_internal_read_file(path, &length);

    // a file that holds the text already is left as it is
    if ( (there) &&
         (length == _doc->length) &&
         ( (length == 0u) ||
           (memcmp(there, _doc->text, length) == 0) ) )
    {
        free(there);

        return D_WIKI_SAVE_UNCHANGED;
    }

    free(there);

    return (d_internal_write_file(path, _doc->text, _doc->length, _problem,
                                  _capacity))
               ? D_WIKI_SAVE_WRITTEN : D_WIKI_SAVE_FAILED;
}
