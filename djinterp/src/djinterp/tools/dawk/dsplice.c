/*******************************************************************************
* djinterp [tools]                                                     dsplice.c
*
* Changes to a text, and the files they go back to: see dsplice.h.
*
* path:      /src/djinterp/tools/dawk/dsplice.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dsplice.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fread, fwrite, fclose, remove, rename
#include <stdlib.h>  // malloc, realloc, free, qsort
#include <string.h>  // memcpy, memset, strlen


// d_internal_splice
//   struct: one change: the bytes from `start` to `end` become `text`;
//   `order` keeps changes at one place in the order they were made.
struct d_internal_splice
{
    size_t start;
    size_t end;
    char*  text;
    size_t length;
    size_t order;
};

// d_splices
//   struct: the changes, and how many were ever made, for their order.
struct d_splices
{
    struct d_internal_splice* items;
    size_t                    count;
    size_t                    capacity;
    size_t                    made;
};


/*
d_splices_new
  No changes yet.
*/
struct d_splices*
d_splices_new(void)
{
    struct d_splices* const splices = malloc(sizeof(*splices));

    // a set that cannot be made is NULL
    if (splices)
    {
        memset(splices, 0, sizeof(*splices));
    }

    return splices;
}


/*
d_splices_clear
  Forgets every change.
*/
void
d_splices_clear(
    struct d_splices* _splices
)
{
    // clearing nothing is allowed
    if (!_splices)
    {
        return;
    }

    for (size_t at = 0; at < _splices->count; ++at)
    {
        free(_splices->items[at].text);
    }

    _splices->count = 0;

    return;
}


/*
d_splices_free
  Forgets every change, and the set.
*/
void
d_splices_free(
    struct d_splices* _splices
)
{
    // freeing nothing is allowed
    if (!_splices)
    {
        return;
    }

    d_splices_clear(_splices);
    free(_splices->items);
    free(_splices);

    return;
}


/*
d_splices_count
  How many changes are waiting.
*/
size_t
d_splices_count(
    const struct d_splices* _splices
)
{
    return (_splices) ? _splices->count : 0u;
}


/*
d_splices_add
  Records a change, unless it overlaps one already made.
*/
enum d_splice_result
d_splices_add(
    struct d_splices* _splices,
    size_t            _start,
    size_t            _end,
    const char*       _text,
    size_t            _length
)
{
    // parameter validation first
    if ( (!_splices) ||
         (_end < _start) )
    {
        return D_SPLICE_OVERLAPS;
    }

    const bool replaces = (_end > _start);

    for (size_t at = 0; at < _splices->count; ++at)
    {
        const struct d_internal_splice* const made = &_splices->items[at];
        const bool made_replaces = (made->end > made->start);
        const bool overlap       =
            ( (replaces) && (made_replaces) &&
              (_start < made->end) && (made->start < _end) )      ||
            ( (!replaces) && (made_replaces) &&
              (_start > made->start) && (_start < made->end) )    ||
            ( (replaces) && (!made_replaces) &&
              (made->start > _start) && (made->start < _end) );

        // a change over another is refused, as neither could be kept whole
        if (overlap)
        {
            return D_SPLICE_OVERLAPS;
        }
    }

    // room for one more
    if (_splices->count == _splices->capacity)
    {
        const size_t wanted = (_splices->capacity == 0u)
                            ? 8u : (_splices->capacity * 2u);
        struct d_internal_splice* const grown =
            realloc(_splices->items, wanted * sizeof(*grown));

        if (!grown)
        {
            return D_SPLICE_NO_MEMORY;
        }

        _splices->items    = grown;
        _splices->capacity = wanted;
    }

    char* const copy = malloc(_length + 1u);

    if (!copy)
    {
        return D_SPLICE_NO_MEMORY;
    }

    // the text is copied, as the caller's need not outlive the call
    if (_length > 0u)
    {
        memcpy(copy, _text, _length);
    }

    copy[_length] = '\0';

    struct d_internal_splice* const splice =
        &_splices->items[_splices->count++];

    splice->start  = _start;
    splice->end    = _end;
    splice->text   = copy;
    splice->length = _length;
    splice->order  = _splices->made++;

    return D_SPLICE_ADDED;
}


/*
d_internal_compare
  The order changes are applied in: by where they start; at one place, an
insertion before a replacement; and otherwise in the order they were made.
*/
static int
d_internal_compare(
    const void* _left,
    const void* _right
)
{
    const struct d_internal_splice* const left  = _left;
    const struct d_internal_splice* const right = _right;
    const bool left_inserts  = (left->end == left->start);
    const bool right_inserts = (right->end == right->start);

    // the text's order first
    if (left->start != right->start)
    {
        return (left->start < right->start) ? -1 : 1;
    }

    // an insertion at a replacement's start goes before what it replaces
    if (left_inserts != right_inserts)
    {
        return (left_inserts) ? -1 : 1;
    }

    return (left->order < right->order) ? -1
         : (left->order > right->order) ?  1
         :                                 0;
}


/*
d_splices_apply
  Copies the text, a change's text in place of each span it replaces; where
a change's text meets the text beside it, the hook may put a byte between.
*/
char*
d_splices_apply(
    struct d_splices*   _splices,
    const char*         _text,
    size_t              _length,
    d_splice_fn_between _between,
    void*               _user,
    size_t*             _out_length
)
{
    const size_t count  = (_splices) ? _splices->count : 0u;
    size_t       length = _length + (2u * count) + 1u;

    for (size_t at = 0; at < count; ++at)
    {
        length += _splices->items[at].length;
    }

    char* const out = malloc(length);

    // the new text needs room of its own
    if (!out)
    {
        return NULL;
    }

    // with no changes there is nothing to order, and no array to order
    if (count > 0u)
    {
        qsort(_splices->items, count, sizeof(*_splices->items),
              d_internal_compare);
    }

    size_t from = 0;
    size_t used = 0;

    for (size_t at = 0; at < count; ++at)
    {
        const struct d_internal_splice* const splice = &_splices->items[at];

        memcpy(&out[used], &_text[from], splice->start - from);
        used += splice->start - from;

        // where the change's text meets what comes before it
        if ( (_between)            &&
             (splice->length > 0u) &&
             (used > 0u) )
        {
            const char between = _between(_user, out[used - 1u],
                                          splice->text[0]);

            if (between != '\0')
            {
                out[used++] = between;
            }
        }

        memcpy(&out[used], splice->text, splice->length);
        used += splice->length;
        from  = splice->end;

        // and where it meets what comes after it
        if ( (_between)            &&
             (splice->length > 0u) &&
             (from < _length) )
        {
            const char between = _between(_user,
                                          splice->text[splice->length - 1u],
                                          _text[from]);

            if (between != '\0')
            {
                out[used++] = between;
            }
        }
    }

    memcpy(&out[used], &_text[from], _length - from);
    used        += _length - from;
    out[used]    = '\0';
    *_out_length = used;

    return out;
}


/*
d_text_read
  Reads a whole file, terminated.
*/
char*
d_text_read(
    const char* _path,
    size_t*     _out_length
)
{
    FILE* const handle = (_path) ? fopen(_path, "rb") : NULL;

    // a file that cannot be opened has no text
    if (!handle)
    {
        return NULL;
    }

    size_t capacity = 4096u;
    size_t used     = 0;
    char*  text     = malloc(capacity + 1u);

    while (text)
    {
        used += fread(&text[used], 1u, capacity - used, handle);

        // a short read is the end of the file
        if (used < capacity)
        {
            break;
        }

        char* const grown = realloc(text, (capacity * 2u) + 1u);

        if (!grown)
        {
            free(text);
            text = NULL;
            break;
        }

        text      = grown;
        capacity *= 2u;
    }

    (void)fclose(handle);

    // the text is terminated, so a part of it can be given as a string
    if (text)
    {
        text[used] = '\0';
    }

    *_out_length = used;

    return text;
}


/*
d_text_write
  Writes a text beside its target and renames it into place.
*/
bool
d_text_write(
    const char* _path,
    const char* _text,
    size_t      _length,
    char*       _problem,
    size_t      _capacity
)
{
    const size_t span = strlen(_path) + sizeof(".part");
    char* const  part = malloc(span);

    // the temporary's name needs room of its own
    if (!part)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    (void)snprintf(part, span, "%s.part", _path);

    FILE* const out = fopen(part, "wb");
    bool        ok  = (out != NULL);

    ok = ok && (fwrite(_text, 1u, _length, out) == _length);

    // a stream that fails to close has not written what it held
    if ( (out) &&
         (fclose(out) != 0) )
    {
        ok = false;
    }

    // the new file replaces the old only once it is whole
    if ( (ok) &&
         (rename(part, _path) != 0) )
    {
        ok = ( (remove(_path) == 0) &&
               (rename(part, _path) == 0) );
    }

    if (!ok)
    {
        (void)remove(part);
        (void)snprintf(_problem, _capacity, "cannot write: %s", _path);
    }

    free(part);

    return ok;
}
