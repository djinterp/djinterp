/*******************************************************************************
* djinterp [c]                                                       path_span.c
*
* A generic path, read from its rendering (path_span.h).
*   Every operation walks the text with a cursor, so nothing is copied or
* allocated. Rendering decides each label as it reaches it -- a ".." after
* it, read as a closing bracket, can fold it away -- which is what lets it
* write as snprintf does, into whatever room it has.
*
* path:      /src/djinterp/c/text/path_span.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/path_span.h"  // corresponding header
// std
#include <string.h>  // strchr, strlen


// D_INTERNAL_PATH_LABEL, D_INTERNAL_PATH_SELF, D_INTERNAL_PATH_UP
//   constant: what a label is under D_PATH_GRAMMAR_DOT_LABELS: a label, ".",
// or "..".
#define D_INTERNAL_PATH_LABEL 0
#define D_INTERNAL_PATH_SELF  1
#define D_INTERNAL_PATH_UP    2

// d_internal_path_chain
//   struct: one or two paths read as one, the second's labels after the
// first's -- what a join renders.
struct d_internal_path_chain
{
    const struct d_path_span* part[2];
    size_t                    count;
};

// d_internal_path_at
//   struct: a place in a chain: the part, and the cursor within it.
struct d_internal_path_at
{
    size_t part;
    size_t cursor;
};

/**
 * @brief Whether a character is one of a set.
 *
 * @param[in] _c    the character.
 * @param[in] _set  NUL-terminated characters, or NULL for none.
 * @return true when `_c` is in `_set`, never for the NUL that ends it.
 */
static bool
d_internal_path_in(
    char        _c,
    const char* _set
)
{
    return ( (_set != NULL)  &&
             (_c != '\0')    &&
             (strchr(_set, _c) != NULL) );
}

/**
 * @brief Folds an ASCII letter to lower case, whatever the C locale says.
 *
 * @param[in] _c  the character.
 * @return its lower case, or `_c` for anything but an ASCII capital.
 */
static char
d_internal_path_fold(
    char _c
)
{
    return ( (_c >= 'A') &&
             (_c <= 'Z') ) ? (char)(_c - 'A' + 'a')
                           : _c;
}

/**
 * @brief Whether two runs of text read alike by a grammar: any separator
 *        matches any other, and ASCII case is folded where it says so.
 *
 * @param[in] _a        a run.
 * @param[in] _b        another, as long.
 * @param[in] _length   their length, in bytes.
 * @param[in] _grammar  the grammar; may be NULL.
 * @return true when they read alike.
 */
static bool
d_internal_path_same(
    const char*                  _a,
    const char*                  _b,
    size_t                       _length,
    const struct d_path_grammar* _grammar
)
{
    const char* const separators = (_grammar) ? _grammar->separators : NULL;
    const bool        fold       = ( (_grammar != NULL) &&
                                     ((_grammar->flags &
                                       D_PATH_GRAMMAR_FOLD_CASE) != 0u) );

    // byte for byte, a separator for a separator
    for (size_t i = 0; i < _length; ++i)
    {
        const char a = (fold) ? d_internal_path_fold(_a[i]) : _a[i];
        const char b = (fold) ? d_internal_path_fold(_b[i]) : _b[i];

        if ( (a != b) &&
             ( (!d_internal_path_in(a, separators)) ||
               (!d_internal_path_in(b, separators)) ) )
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Whether two paths stand in one anchor.
 *
 * @param[in] _a  a path.
 * @param[in] _b  another.
 * @return true when their anchors read alike by `_a`'s grammar.
 */
static bool
d_internal_path_same_anchor(
    const struct d_path_span* _a,
    const struct d_path_span* _b
)
{
    return ( (_a->anchor == _b->anchor) &&
             (d_internal_path_same(_a->text,
                                   _b->text,
                                   _a->anchor,
                                   _a->grammar)) );
}

struct d_path_span
d_path_span_of(
    const char*                  _text,
    size_t                       _length,
    const struct d_path_grammar* _grammar
)
{
    struct d_path_span span = { NULL, 0, 0, 0, NULL };

    // no text, or no way to read it: the empty span
    if ( (!_text) ||
         (!_grammar) )
    {
        return span;
    }

    const size_t extent = (_length == D_PATH_SPAN_TERMINATED) ? strlen(_text)
                                                              : _length;
    size_t       length = 0;

    // the path proper runs to the first terminator
    while ( (length < extent) &&
            (!d_internal_path_in(_text[length], _grammar->terminators)) )
    {
        ++length;
    }

    size_t anchor = 0;

    // the grammar's anchor, or the separators the text begins with
    if (_grammar->anchor)
    {
        anchor = _grammar->anchor(_text,
                                  length,
                                  _grammar);
    }
    else
    {
        while ( (anchor < length) &&
                (d_internal_path_in(_text[anchor], _grammar->separators)) )
        {
            ++anchor;
        }
    }

    span.text    = _text;
    span.length  = length;
    span.extent  = extent;
    span.anchor  = (anchor < length) ? anchor : length;
    span.grammar = _grammar;

    return span;
}

/*
d_path_span_next
  The cursor is a byte offset into the text: anything short of the anchor's
end starts from there, and the separators before a label are skipped, so a
run of them frames one boundary and no empty label is ever handed out.
*/
bool
d_path_span_next(
    const struct d_path_span* _span,
    size_t*                   _cursor,
    struct d_path_label*      _label
)
{
    // nothing to step through
    if ( (!_span)       ||
         (!_span->text) ||
         (!_cursor) )
    {
        return false;
    }

    const char* const separators = _span->grammar->separators;
    size_t            at         = (*_cursor < _span->anchor) ? _span->anchor
                                                              : *_cursor;

    // the separators before the label
    while ( (at < _span->length) &&
            (d_internal_path_in(_span->text[at], separators)) )
    {
        ++at;
    }

    // past the last label
    if (at >= _span->length)
    {
        *_cursor = _span->length;

        return false;
    }

    const size_t start = at;

    // the label runs to the next separator, or the end of the path proper
    while ( (at < _span->length) &&
            (!d_internal_path_in(_span->text[at], separators)) )
    {
        ++at;
    }

    // where the caller wants it
    if (_label)
    {
        _label->text   = _span->text + start;
        _label->length = at - start;
    }

    *_cursor = at;

    return true;
}

size_t
d_path_span_level(
    const struct d_path_span* _span
)
{
    size_t cursor = 0;
    size_t level  = 0;

    // one for each label
    while (d_path_span_next(_span,
                            &cursor,
                            NULL))
    {
        ++level;
    }

    return level;
}

bool
d_path_span_is_anchored(
    const struct d_path_span* _span
)
{
    return ( (_span != NULL) &&
             (_span->anchor > 0) );
}

struct d_path_label
d_path_span_anchor(
    const struct d_path_span* _span
)
{
    struct d_path_label anchor = { NULL, 0 };

    // the run the anchor takes, where there is one
    if ( (_span) &&
         (_span->text) )
    {
        anchor.text   = _span->text;
        anchor.length = _span->anchor;
    }

    return anchor;
}

struct d_path_label
d_path_span_last(
    const struct d_path_span* _span
)
{
    struct d_path_label last   = { NULL, 0 };
    struct d_path_label label  = { NULL, 0 };
    size_t              cursor = 0;

    // the last label the path hands out
    while (d_path_span_next(_span,
                            &cursor,
                            &label))
    {
        last = label;
    }

    return last;
}

struct d_path_label
d_path_span_rest(
    const struct d_path_span* _span
)
{
    struct d_path_label rest = { NULL, 0 };

    // what follows the path proper, where anything does
    if ( (_span) &&
         (_span->text) )
    {
        rest.text   = _span->text + _span->length;
        rest.length = _span->extent - _span->length;
    }

    return rest;
}

/*
d_path_span_parent
  The parent is the text up to where the last label starts: the separators
before it stay, and are read as the boundary they are.
*/
struct d_path_span
d_path_span_parent(
    const struct d_path_span* _span
)
{
    struct d_path_span parent = { NULL, 0, 0, 0, NULL };

    // nothing has no parent
    if (!_span)
    {
        return parent;
    }

    const struct d_path_label last = d_path_span_last(_span);

    parent        = *_span;
    parent.length = (last.text) ? (size_t)(last.text - _span->text)
                                : _span->length;
    parent.extent = parent.length;

    return parent;
}

bool
d_path_label_equal(
    struct d_path_label          _a,
    struct d_path_label          _b,
    const struct d_path_grammar* _grammar
)
{
    return ( (_a.length == _b.length) &&
             ( (_a.length == 0) ||
               (d_internal_path_same(_a.text,
                                     _b.text,
                                     _a.length,
                                     _grammar)) ) );
}

size_t
d_path_span_meet(
    const struct d_path_span* _a,
    const struct d_path_span* _b
)
{
    // paths in different anchors are in different spaces
    if ( (!_a)       ||
         (!_b)       ||
         (!_a->text) ||
         (!_b->text) ||
         (!d_internal_path_same_anchor(_a, _b)) )
    {
        return D_PATH_SPAN_NO_MEET;
    }

    struct d_path_label a        = { NULL, 0 };
    struct d_path_label b        = { NULL, 0 };
    size_t              cursor_a = 0;
    size_t              cursor_b = 0;
    size_t              level    = 0;

    // the labels both have, in order
    while ( (d_path_span_next(_a, &cursor_a, &a)) &&
            (d_path_span_next(_b, &cursor_b, &b)) &&
            (d_path_label_equal(a, b, _a->grammar)) )
    {
        ++level;
    }

    return level;
}

bool
d_path_span_starts_with(
    const struct d_path_span* _span,
    const struct d_path_span* _prefix
)
{
    const size_t meet = d_path_span_meet(_span,
                                         _prefix);

    return ( (meet != D_PATH_SPAN_NO_MEET) &&
             (meet == d_path_span_level(_prefix)) );
}

/**
 * @brief The next label of a chain, in its part or a later one.
 *
 * @param[in]     _chain  the chain.
 * @param[in,out] _at     where to read from; moved past the label.
 * @param[out]    _label  receives the label.
 * @return true for a label, false past the last.
 */
static bool
d_internal_path_chain_next(
    const struct d_internal_path_chain* _chain,
    struct d_internal_path_at*          _at,
    struct d_path_label*                _label
)
{
    // this part's next label, or the next part's first
    while (_at->part < _chain->count)
    {
        if (d_path_span_next(_chain->part[_at->part],
                             &_at->cursor,
                             _label))
        {
            return true;
        }

        _at->part++;
        _at->cursor = 0;
    }

    return false;
}

/**
 * @brief What a label is: a label, or, under D_PATH_GRAMMAR_DOT_LABELS, "."
 *        or "..".
 *
 * @param[in] _label  the label.
 * @param[in] _dots   whether the grammar reads dot labels.
 * @return D_INTERNAL_PATH_LABEL, D_INTERNAL_PATH_SELF or D_INTERNAL_PATH_UP.
 */
static int
d_internal_path_kind(
    struct d_path_label _label,
    bool                _dots
)
{
    // only a grammar that reads them has dot labels
    if ( (!_dots)             ||
         (_label.length == 0) ||
         (_label.length > 2)  ||
         (_label.text[0] != '.') )
    {
        return D_INTERNAL_PATH_LABEL;
    }

    // "." or ".."
    if (_label.length == 1)
    {
        return D_INTERNAL_PATH_SELF;
    }

    return (_label.text[1] == '.') ? D_INTERNAL_PATH_UP
                                   : D_INTERNAL_PATH_LABEL;
}

/**
 * @brief Whether a ".." after a label folds it away.
 *
 * @param[in] _chain  the chain.
 * @param[in] _after  the place just past the label.
 * @param[in] _dots   whether the grammar reads dot labels.
 * @return true when the labels after it, read as brackets -- a label opens,
 *         a ".." closes -- close the one it opens.
 */
static bool
d_internal_path_folded(
    const struct d_internal_path_chain* _chain,
    struct d_internal_path_at           _after,
    bool                                _dots
)
{
    struct d_path_label label = { NULL, 0 };
    size_t              depth = 0;

    // a ".." with no label of its own to close closes this one
    while (d_internal_path_chain_next(_chain,
                                      &_after,
                                      &label))
    {
        const int kind = d_internal_path_kind(label,
                                              _dots);

        if (kind == D_INTERNAL_PATH_LABEL)
        {
            ++depth;
        }
        else if (kind == D_INTERNAL_PATH_UP)
        {
            if (depth == 0)
            {
                return true;
            }

            --depth;
        }
    }

    return false;
}

/**
 * @brief Writes a run as snprintf would: what fits, and counts the rest.
 *
 * @param[out]    _buf     the buffer; NULL where `_size` is 0.
 * @param[in]     _size    its capacity, in bytes.
 * @param[in,out] _out     the bytes the rendering has taken so far.
 * @param[in]     _text    the run.
 * @param[in]     _length  its length.
 */
static void
d_internal_path_put(
    char*       _buf,
    size_t      _size,
    size_t*     _out,
    const char* _text,
    size_t      _length
)
{
    // room is kept for the terminator
    for (size_t i = 0; i < _length; ++i)
    {
        if ((*_out + 1) < _size)
        {
            _buf[*_out] = _text[i];
        }

        ++(*_out);
    }

    return;
}

/*
d_internal_path_render
  One pass over the chain's labels. A label stays unless a ".." after it
folds it (d_internal_path_folded), which costs a scan of what follows: a path
has few labels, and the alternative is a stack the caller would pay for. The
".." that folds a label counts the labels still open before it, and one with
none stays in a relative path and goes from an anchored one.
*/
static size_t
d_internal_path_render(
    const struct d_internal_path_chain* _chain,
    const struct d_path_span*           _anchor,
    char*                               _buf,
    size_t                              _size
)
{
    const struct d_path_grammar* const grammar  = _anchor->grammar;
    const bool                         dots     = ( (grammar->flags &
                                                     D_PATH_GRAMMAR_DOT_LABELS)
                                                    != 0u );
    const bool                         anchored = (_anchor->anchor > 0);
    const char                         edge     = (anchored)
                                                      ? _anchor->text[
                                                            _anchor->anchor - 1]
                                                      : '\0';
    const bool                         frame    = ( (anchored) &&
                                                    (!d_internal_path_in(
                                                        edge,
                                                        grammar->separators)) );
    struct d_internal_path_at          at       = { 0, 0 };
    struct d_path_label                label    = { NULL, 0 };
    size_t                             out      = 0;
    size_t                             written  = 0;
    size_t                             open     = 0;

    d_internal_path_put(_buf,
                        _size,
                        &out,
                        _anchor->text,
                        _anchor->anchor);

    // each label, kept or not as it is reached
    while (d_internal_path_chain_next(_chain,
                                      &at,
                                      &label))
    {
        const int kind = d_internal_path_kind(label,
                                              dots);
        bool      keep = false;

        if (kind == D_INTERNAL_PATH_LABEL)
        {
            keep = !d_internal_path_folded(_chain,
                                           at,
                                           dots);
            ++open;
        }
        else if (kind == D_INTERNAL_PATH_UP)
        {
            keep = ( (open == 0) &&
                     (!anchored) );
            open = (open > 0) ? (open - 1) : 0;
        }

        // framed by the separator, after the anchor's own where it has none
        if (keep)
        {
            if ( (written > 0) ||
                 (frame) )
            {
                d_internal_path_put(_buf,
                                    _size,
                                    &out,
                                    &grammar->separator,
                                    1);
            }

            d_internal_path_put(_buf,
                                _size,
                                &out,
                                label.text,
                                label.length);
            ++written;
        }
    }

    // the terminator, where there is room for one
    if (_size > 0)
    {
        _buf[(out < _size) ? out : (_size - 1)] = '\0';
    }

    return out;
}

size_t
d_path_span_render(
    const struct d_path_span* _span,
    char*                     _buf,
    size_t                    _size
)
{
    // an empty span renders as nothing
    if ( (!_span)       ||
         (!_span->text) ||
         (!_span->grammar) )
    {
        if ( (_buf) &&
             (_size > 0) )
        {
            _buf[0] = '\0';
        }

        return 0;
    }

    const struct d_internal_path_chain chain = { { _span, NULL }, 1 };

    return d_internal_path_render(&chain,
                                  _span,
                                  _buf,
                                  _size);
}

size_t
d_path_span_render_join(
    const struct d_path_span* _base,
    const struct d_path_span* _more,
    char*                     _buf,
    size_t                    _size
)
{
    // an anchored path is taken from nothing but its anchor
    if ( (!_base)       ||
         (!_base->text) ||
         (!_more)       ||
         (!_more->text) ||
         (d_path_span_is_anchored(_more)) )
    {
        return d_path_span_render(((_more) && (_more->text)) ? _more : _base,
                                  _buf,
                                  _size);
    }

    const struct d_internal_path_chain chain = { { _base, _more }, 2 };

    return d_internal_path_render(&chain,
                                  _base,
                                  _buf,
                                  _size);
}
