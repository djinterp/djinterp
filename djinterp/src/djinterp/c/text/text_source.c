/*******************************************************************************
* djinterp [c]                                                     text_source.c
*
*   Text source: the text, whether the source owns it, and the offset where
* each line starts, found in one pass when the source is made. A line is
* found from an offset by binary search over those starts.
*
*
* path:      /src/djinterp/c/text/text_source.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/text_source.h"  // corresponding
                                                          // header
// std
#include <stdio.h>   // FILE, fopen, fread, fwrite, fclose, ferror, rename,
                     // remove, snprintf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, memchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add, d_ckd_mul


// dj_text_source
//   struct: the text; the copy, when the source owns one; and the offset
// where each line starts, in order.
struct dj_text_source
{
    const char* text;
    size_t      length;
    char*       owned;   // the text, when it is the source's own; else NULL
    size_t*     starts;  // where each line starts; NULL when there are none
    size_t      lines;
};


/*
d_internal_text_source_index
  Counts the lines, then records where each starts: at 0, and after each
newline that is not the text's last byte.
*/
static bool
d_internal_text_source_index(
    struct dj_text_source* _source
)
{
    const char* const text  = _source->text;
    const size_t      total = _source->length;
    size_t            lines = 0u;
    size_t            at    = 0u;
    size_t            bytes;
    size_t            line;

    // each line runs to its newline, or to the end of the text
    while (at < total)
    {
        const char* const newline =
            (const char*)memchr(text + at, '\n', total - at);

        ++lines;
        at = (newline) ? (size_t)(newline - text) + 1u : total;
    }

    _source->lines = lines;

    if (lines == 0u)
    {
        return true;
    }

    if (d_ckd_mul(&bytes, lines, sizeof(size_t)))
    {
        return false;
    }

    _source->starts = (size_t*)malloc(bytes);

    if (!_source->starts)
    {
        return false;
    }

    for (at = 0u, line = 0u; line < lines; ++line)
    {
        const char* newline;

        _source->starts[line] = at;
        newline = (const char*)memchr(text + at, '\n', total - at);
        at      = (newline) ? (size_t)(newline - text) + 1u : total;
    }

    return true;
}

/*
d_internal_text_source_make
  A source over _text, owned or not, with its lines indexed.
*/
static struct dj_text_source*
d_internal_text_source_make(
    const char* _text,
    size_t      _length,
    char*       _owned
)
{
    struct dj_text_source* const source =
        (struct dj_text_source*)malloc(sizeof(struct dj_text_source));

    if (!source)
    {
        return NULL;
    }

    source->text   = (_text) ? _text : "";
    source->length = _length;
    source->owned  = _owned;
    source->starts = NULL;
    source->lines  = 0u;

    if (!d_internal_text_source_index(source))
    {
        free(source);

        return NULL;
    }

    return source;
}


/*
d_text_source_new
  Copies the text, terminated, and indexes the copy.
*/
struct dj_text_source*
d_text_source_new(
    const char* _text,
    size_t      _length
)
{
    struct dj_text_source* source;
    char*                  copy;
    size_t                 bytes;

    // parameter validation first
    if ( ( (!_text) && (_length > 0u) )      ||
         (d_ckd_add(&bytes, _length, 1u)) )
    {
        return NULL;
    }

    copy = (char*)malloc(bytes);

    if (!copy)
    {
        return NULL;
    }

    if (_length > 0u)
    {
        memcpy(copy, _text, _length);
    }

    copy[_length] = '\0';
    source        = d_internal_text_source_make(copy, _length, copy);

    if (!source)
    {
        free(copy);
    }

    return source;
}

/*
d_text_source_borrow
  Indexes the caller's text, which must outlive the source.
*/
struct dj_text_source*
d_text_source_borrow(
    const char* _text,
    size_t      _length
)
{
    // parameter validation first
    if ( (!_text) && (_length > 0u) )
    {
        return NULL;
    }

    return d_internal_text_source_make(_text, _length, NULL);
}

/*
d_text_source_read
  Reads the file whole, and gives the source the text.
*/
struct dj_text_source*
d_text_source_read(
    const char* _path
)
{
    struct dj_text_source* source;
    size_t                 length;
    char* const            text = d_text_read(_path, &length);

    if (!text)
    {
        return NULL;
    }

    source = d_internal_text_source_make(text, length, text);

    if (!source)
    {
        free(text);
    }

    return source;
}

/*
d_text_source_free
  Accepts NULL, as free does.
*/
void
d_text_source_free(
    struct dj_text_source* _source
)
{
    if (!_source)
    {
        return;
    }

    free(_source->owned);
    free(_source->starts);
    free(_source);

    return;
}

/*
d_text_source_text
  The text and its length.
*/
const char*
d_text_source_text(
    const struct dj_text_source* _source,
    size_t*                      _out_length
)
{
    if (_out_length)
    {
        *_out_length = (_source) ? _source->length : 0u;
    }

    return (_source) ? _source->text : NULL;
}

/*
d_text_source_line_count
  The lines the index found.
*/
size_t
d_text_source_line_count(
    const struct dj_text_source* _source
)
{
    return (_source) ? _source->lines : 0u;
}

/*
d_text_source_line
  A line ends where the next starts, less its newline; the last ends at its
newline, if it has one, or at the text's end.
*/
bool
d_text_source_line(
    const struct dj_text_source* _source,
    size_t                       _line,
    size_t*                      _out_start,
    size_t*                      _out_end
)
{
    size_t start;
    size_t end;

    // parameter validation first
    if ( (!_source)                ||
         (_line == 0u)             ||
         (_line > _source->lines) )
    {
        return false;
    }

    start = _source->starts[_line - 1u];

    if (_line < _source->lines)
    {
        end = _source->starts[_line] - 1u;
    }
    else
    {
        end = _source->length;

        if ( (end > start) &&
             (_source->text[end - 1u] == '\n') )
        {
            --end;
        }
    }

    if (_out_start)
    {
        *_out_start = start;
    }

    if (_out_end)
    {
        *_out_end = end;
    }

    return true;
}

/*
d_text_source_locate
  The line is the last whose start is at or before the offset. The text's
end past a final newline, or of an empty text, opens the line after.
*/
bool
d_text_source_locate(
    const struct dj_text_source* _source,
    size_t                       _offset,
    size_t*                      _out_line,
    size_t*                      _out_column
)
{
    size_t line;
    size_t column;

    // parameter validation first
    if ( (!_source)                   ||
         (_offset > _source->length) )
    {
        return false;
    }

    if ( (_source->lines == 0u) ||
         ( (_offset == _source->length) &&
           (_source->text[_offset - 1u] == '\n') ) )
    {
        line   = _source->lines + 1u;
        column = 1u;
    }
    else
    {
        size_t low  = 0u;
        size_t high = _source->lines;

        // the last start at or before the offset: starts[low] <= _offset
        while ((high - low) > 1u)
        {
            const size_t middle = low + ((high - low) / 2u);

            if (_source->starts[middle] <= _offset)
            {
                low = middle;
            }
            else
            {
                high = middle;
            }
        }

        line   = low + 1u;
        column = (_offset - _source->starts[low]) + 1u;
    }

    if (_out_line)
    {
        *_out_line = line;
    }

    if (_out_column)
    {
        *_out_column = column;
    }

    return true;
}


/*
d_text_read
  Reads in doubling chunks rather than asking the file's size, so that a
file whose size cannot be told -- a pipe -- still reads. A read error is a
failure, not an end.
*/
char*
d_text_read(
    const char* _path,
    size_t*     _out_length
)
{
    FILE*  handle;
    size_t capacity = 4096u;
    size_t used     = 0u;
    char*  text;

    if (_out_length)
    {
        *_out_length = 0u;
    }

    handle = (_path) ? fopen(_path, "rb") : NULL;

    // a file that cannot be opened has no text
    if (!handle)
    {
        return NULL;
    }

    text = (char*)malloc(capacity + 1u);

    while (text)
    {
        size_t grown;
        size_t bytes;
        char*  larger;

        used += fread(&text[used], 1u, capacity - used, handle);

        // a short read is the end of the file, or an error
        if (used < capacity)
        {
            if (ferror(handle))
            {
                free(text);
                text = NULL;
            }

            break;
        }

        if ( (d_ckd_mul(&grown, capacity, 2u)) ||
             (d_ckd_add(&bytes, grown, 1u)) )
        {
            free(text);
            text = NULL;

            break;
        }

        larger = (char*)realloc(text, bytes);

        if (!larger)
        {
            free(text);
            text = NULL;

            break;
        }

        text     = larger;
        capacity = grown;
    }

    (void)fclose(handle);

    // the text is terminated, so a part of it can be given as a string
    if (text)
    {
        text[used] = '\0';

        if (_out_length)
        {
            *_out_length = used;
        }
    }

    return text;
}

/*
d_internal_text_problem
  Writes why a write failed, when the caller gave room for it.
*/
static void
d_internal_text_problem(
    char*       _problem,
    size_t      _capacity,
    const char* _what,
    const char* _path
)
{
    if ( (_problem) && (_capacity > 0u) )
    {
        (void)snprintf(_problem, _capacity, "%s: %s", _what, _path);
    }

    return;
}

/*
d_text_write
  Writes the text beside its target, then renames it into place. The part
file is removed when the target still holds its old text; where the old text
had been removed to let the rename through, the part file is the only copy
of either, and is left where it is.
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
    size_t span;
    char*  part;
    FILE*  out;
    bool   ok;

    // parameter validation first
    if ( (!_path) ||
         ( (!_text) && (_length > 0u) ) )
    {
        d_internal_text_problem(_problem,
                                _capacity,
                                "bad argument",
                                (_path) ? _path : "(null)");

        return false;
    }

    if (d_ckd_add(&span, strlen(_path), sizeof(".part")))
    {
        d_internal_text_problem(_problem, _capacity, "path too long", _path);

        return false;
    }

    part = (char*)malloc(span);

    // the temporary's name needs room of its own
    if (!part)
    {
        d_internal_text_problem(_problem, _capacity, "out of memory", _path);

        return false;
    }

    (void)snprintf(part, span, "%s.part", _path);
    out = fopen(part, "wb");
    ok  = (out != NULL);

    if ( (ok) && (_length > 0u) )
    {
        ok = (fwrite(_text, 1u, _length, out) == _length);
    }

    // a stream that fails to close has not written what it held
    if ( (out) &&
         (fclose(out) != 0) )
    {
        ok = false;
    }

    if (!ok)
    {
        (void)remove(part);
        d_internal_text_problem(_problem, _capacity, "cannot write", part);
        free(part);

        return false;
    }

    // the new file replaces the old only once it is whole
    if (rename(part, _path) != 0)
    {
        if (remove(_path) != 0)
        {
            (void)remove(part);
            d_internal_text_problem(_problem,
                                    _capacity,
                                    "cannot replace",
                                    _path);
            free(part);

            return false;
        }

        if (rename(part, _path) != 0)
        {
            d_internal_text_problem(_problem,
                                    _capacity,
                                    "old text removed; new text left in",
                                    part);
            free(part);

            return false;
        }
    }

    free(part);

    return true;
}
