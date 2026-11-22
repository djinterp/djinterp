/*******************************************************************************
* djinterp [c][text]                                                 text_edit.c
*
*   Text edit: the changes in an array, each with its own copy of its text and
* the order it was made in. Applying sorts them by where they start, measures
* the new text, then copies the old text between them.
*
*
* path:      /src/djinterp/c/text/text_edit.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/text_edit.h"  // corresponding
                                                        // header
// std
#include <stdlib.h>  // malloc, realloc, free, qsort
#include <string.h>  // memcpy, memset
// djinterp
#include "../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add, d_ckd_mul


// dj_internal_text_edit_change
//   struct: one change: the bytes from `start` to `end` become `text`; `order`
// keeps changes at one place in the order they were made.
struct dj_internal_text_edit_change
{
    size_t start;
    size_t end;
    char*  text;
    size_t length;
    size_t order;
};

// dj_text_edit
//   struct: the changes, and how many were ever made, for their order.
struct dj_text_edit
{
    struct dj_internal_text_edit_change* items;
    size_t                               count;
    size_t                               capacity;
    size_t                               made;
};


/*
d_internal_text_edit_compare
  The order changes are applied in: by where they start; at one place, an
insertion before a replacement; and otherwise in the order they were made.
*/
static int
d_internal_text_edit_compare(
    const void* _left,
    const void* _right
)
{
    const struct dj_internal_text_edit_change* const left =
        (const struct dj_internal_text_edit_change*)_left;
    const struct dj_internal_text_edit_change* const right =
        (const struct dj_internal_text_edit_change*)_right;
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
d_text_edit_new
  No changes yet.
*/
struct dj_text_edit*
d_text_edit_new(void)
{
    struct dj_text_edit* const edit =
        (struct dj_text_edit*)malloc(sizeof(struct dj_text_edit));

    // a set that cannot be made is NULL
    if (edit)
    {
        memset(edit, 0, sizeof(*edit));
    }

    return edit;
}

/*
d_text_edit_clear
  Forgets every change; the order starts again.
*/
void
d_text_edit_clear(
    struct dj_text_edit* _edit
)
{
    size_t at;

    // clearing nothing is allowed
    if (!_edit)
    {
        return;
    }

    for (at = 0u; at < _edit->count; ++at)
    {
        free(_edit->items[at].text);
    }

    _edit->count = 0u;
    _edit->made  = 0u;

    return;
}

/*
d_text_edit_free
  Forgets every change, and the set.
*/
void
d_text_edit_free(
    struct dj_text_edit* _edit
)
{
    // freeing nothing is allowed
    if (!_edit)
    {
        return;
    }

    d_text_edit_clear(_edit);
    free(_edit->items);
    free(_edit);

    return;
}

/*
d_text_edit_count
  How many changes are waiting.
*/
size_t
d_text_edit_count(
    const struct dj_text_edit* _edit
)
{
    return (_edit) ? _edit->count : 0u;
}

/*
d_text_edit_add
  Records a change, unless it overlaps one already made.  A change no caller
should make is refused as such before any overlap is looked for, so a bad
offset never reads as a conflict.
*/
enum dj_text_edit_result
d_text_edit_add(
    struct dj_text_edit* _edit,
    size_t               _start,
    size_t               _end,
    const char*          _text,
    size_t               _length
)
{
    const bool                           replaces = (_end > _start);
    struct dj_internal_text_edit_change* change;
    char*                                copy;
    size_t                               bytes;
    size_t                               at;

    // parameter validation first: an insertion of no bytes changes nothing
    if ( (!_edit)                               ||
         (_end < _start)                        ||
         ( (!_text) && (_length > 0u) )         ||
         ( (_end == _start) && (_length == 0u) ) )
    {
        return D_TEXT_EDIT_INVALID;
    }

    for (at = 0u; at < _edit->count; ++at)
    {
        const struct dj_internal_text_edit_change* const made =
            &_edit->items[at];
        const bool made_replaces = (made->end > made->start);
        const bool overlap       =
            ( (replaces) && (made_replaces) &&
              (_start < made->end) && (made->start < _end) )   ||
            ( (!replaces) && (made_replaces) &&
              (_start > made->start) && (_start < made->end) ) ||
            ( (replaces) && (!made_replaces) &&
              (made->start > _start) && (made->start < _end) );

        // a change over another is refused, as neither could be kept whole
        if (overlap)
        {
            return D_TEXT_EDIT_OVERLAPS;
        }
    }

    // room for one more
    if (_edit->count == _edit->capacity)
    {
        size_t                               wanted = 8u;
        struct dj_internal_text_edit_change* grown;

        if ( ( (_edit->capacity != 0u) &&
               (d_ckd_mul(&wanted, _edit->capacity, 2u)) ) ||
             (d_ckd_mul(&bytes, wanted, sizeof(*grown))) )
        {
            return D_TEXT_EDIT_NO_MEMORY;
        }

        grown = (struct dj_internal_text_edit_change*)realloc(_edit->items,
                                                              bytes);

        if (!grown)
        {
            return D_TEXT_EDIT_NO_MEMORY;
        }

        _edit->items    = grown;
        _edit->capacity = wanted;
    }

    if (d_ckd_add(&bytes, _length, 1u))
    {
        return D_TEXT_EDIT_NO_MEMORY;
    }

    copy = (char*)malloc(bytes);

    if (!copy)
    {
        return D_TEXT_EDIT_NO_MEMORY;
    }

    // the text is copied, as the caller's need not outlive the call
    if (_length > 0u)
    {
        memcpy(copy, _text, _length);
    }

    copy[_length] = '\0';

    change         = &_edit->items[_edit->count++];
    change->start  = _start;
    change->end    = _end;
    change->text   = copy;
    change->length = _length;
    change->order  = _edit->made++;

    return D_TEXT_EDIT_ADDED;
}

/*
d_internal_text_edit_measure
  The new text's size, before a byte is written: the old length, plus what
the changes put in, plus room for a byte between at each meeting, and the
terminator. False when a change lies past the text's end, or the size will
not fit.
*/
static bool
d_internal_text_edit_measure(
    const struct dj_text_edit* _edit,
    size_t                     _count,
    size_t                     _length,
    size_t*                    _out_total
)
{
    size_t total = _length;
    size_t at;

    for (at = 0u; at < _count; ++at)
    {
        const struct dj_internal_text_edit_change* const change =
            &_edit->items[at];

        // a change past the text's end has nothing to replace there
        if (change->end > _length)
        {
            return false;
        }

        if ( (d_ckd_add(&total, total, change->length)) ||
             (d_ckd_add(&total, total, 2u)) )
        {
            return false;
        }
    }

    if (d_ckd_add(&total, total, 1u))
    {
        return false;
    }

    *_out_total = total;

    return true;
}

/*
d_internal_text_edit_between
  Where a change's text meets the text beside it, asks the hook for a byte
to put between, and puts it.
*/
static void
d_internal_text_edit_between(
    dj_text_edit_fn_between _between,
    void*                   _user,
    char                    _before,
    char                    _after,
    char*                   _out,
    size_t*                 _used
)
{
    const char between = _between(_user, _before, _after);

    if (between != '\0')
    {
        _out[(*_used)++] = between;
    }

    return;
}

/*
d_text_edit_apply
  Measures, then sorts the changes into the order they apply in, then copies
the text, each change's text in place of the span it replaces; where a
change's text meets the text beside it, the hook may put a byte between.
*/
char*
d_text_edit_apply(
    struct dj_text_edit*    _edit,
    const char*             _text,
    size_t                  _length,
    dj_text_edit_fn_between _between,
    void*                   _user,
    size_t*                 _out_length
)
{
    const size_t count = (_edit) ? _edit->count : 0u;
    size_t       total;
    size_t       from = 0u;
    size_t       used = 0u;
    size_t       at;
    char*        out;

    if (_out_length)
    {
        *_out_length = 0u;
    }

    // parameter validation first
    if ( ( (!_text) && (_length > 0u) ) ||
         (!d_internal_text_edit_measure(_edit, count, _length, &total)) )
    {
        return NULL;
    }

    out = (char*)malloc(total);

    // the new text needs room of its own
    if (!out)
    {
        return NULL;
    }

    // with no changes there is nothing to order, and no array to order
    if (count > 0u)
    {
        qsort(_edit->items,
              count,
              sizeof(*_edit->items),
              d_internal_text_edit_compare);
    }

    for (at = 0u; at < count; ++at)
    {
        const struct dj_internal_text_edit_change* const change =
            &_edit->items[at];

        if (change->start > from)
        {
            memcpy(&out[used], &_text[from], change->start - from);
            used += change->start - from;
        }

        // where the change's text meets what comes before it
        if ( (_between) && (change->length > 0u) && (used > 0u) )
        {
            d_internal_text_edit_between(_between, _user, out[used - 1u],
                                         change->text[0], out, &used);
        }

        if (change->length > 0u)
        {
            memcpy(&out[used], change->text, change->length);
            used += change->length;
        }

        from = (change->end > from) ? change->end : from;

        // and where it meets what comes after it
        if ( (_between) && (change->length > 0u) && (from < _length) )
        {
            d_internal_text_edit_between(_between, _user,
                                         change->text[change->length - 1u],
                                         _text[from], out, &used);
        }
    }

    if (_length > from)
    {
        memcpy(&out[used], &_text[from], _length - from);
        used += _length - from;
    }

    out[used] = '\0';

    if (_out_length)
    {
        *_out_length = used;
    }

    return out;
}
