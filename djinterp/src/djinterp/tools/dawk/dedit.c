/*******************************************************************************
* djinterp [djinterp]                                                    dedit.c
*
* Line editing:
*   The whole of the write path.  A line's own terminator is reproduced, so a
* repair never normalises line endings it was not asked about -- a fixer that
* changes bytes no sheet declared has become a defect itself.
*
*
* path:      /src/djinterp/tools/dawk/dedit.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dedit.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fgets, fwrite
#include <stdlib.h>  // malloc, free
#include <string.h>  // strlen


// LINE_LIMIT
//   constant: the characters a shifted line may end within, the style
// guide's line length.
static const size_t LINE_LIMIT = 80u;

/*
d_edit_trim_end
  Returns the length with trailing newline and carriage return removed.
*/
size_t d_edit_trim_end(
    char*  _text,
    size_t _length
)
{
    while ( (_length > 0) &&
            ((_text[_length - 1u] == '\n') || (_text[_length - 1u] == '\r')) )
    {
        --_length;
    }

    _text[_length] = '\0';

    return _length;
}


/*
d_edit_read_line
  Returns one line of a file by number, without its terminator.  A repair
needs the line the node sits on, which is not the same thing as the node's
own text: a `name` node's text is the file name, and indexing that at the
name's column writes nonsense into the file.  Asking the file is the only
answer that cannot drift from what is on disk.
*/
bool d_edit_read_line(
    const char* _path,
    uint32_t    _line,
    char*       _out,
    size_t      _out_size
)
{
    FILE* const handle = fopen(_path, "rb");

    if (!handle)
    {
        return false;
    }

    uint32_t at = 0;

    while (fgets(_out, (int)_out_size, handle))
    {
        ++at;

        if (at == _line)
        {
            (void)d_edit_trim_end(_out, strlen(_out));
            (void)fclose(handle);
            return true;
        }
    }

    (void)fclose(handle);

    _out[0] = '\0';

    return false;
}

/*
d_edit_rewrite_line
  Replaces one line of a file in place.  The whole file is read, one line is
substituted, and the result is written back -- which is correct and slow, and
the right shape for a first fixer: a file is rewritten only when it has a
defect, and the alternative is an edit-ordering problem that does not need
solving yet.
*/
bool d_edit_rewrite_line(
    const char* _path,
    uint32_t    _line,
    const char* _replacement
)
{
    FILE* const handle = fopen(_path, "rb");

    if (!handle)
    {
        return false;
    }

    (void)fseek(handle, 0, SEEK_END);

    const long size = ftell(handle);

    (void)fseek(handle, 0, SEEK_SET);

    char* const source = malloc((size_t)size + 1u);

    if (!source)
    {
        (void)fclose(handle);
        return false;
    }

    const size_t read = fread(source, 1u, (size_t)size, handle);

    source[read] = '\0';

    (void)fclose(handle);

    FILE* const out = fopen(_path, "wb");

    if (!out)
    {
        free(source);
        return false;
    }

    uint32_t at    = 1;
    size_t   start = 0;

    for (size_t which = 0; which <= read; ++which)
    {
        const bool end_of_line = ((which == read) || (source[which] == '\n'));

        if (!end_of_line)
        {
            continue;
        }

        size_t     length = which - start;
        const bool carriage = ((length > 0)
                            && (source[start + length - 1u] == '\r'));

        // the terminator is reattached below, so it is not part of the line
        if (carriage)
        {
            --length;
        }

        if (at == _line)
        {
            (void)fwrite(_replacement, 1u, strlen(_replacement), out);
        }
        else
        {
            (void)fwrite(source + start, 1u, length, out);
        }

        // Reproduce the terminator this line actually had.  A fixer that
        // normalises line endings it was never asked about changes lines no
        // sheet declared, which is how a repair becomes a defect.
        if (which < read)
        {
            if (carriage)
            {
                (void)fputc('\r', out);
            }

            (void)fputc('\n', out);
        }

        start = which + 1u;
        ++at;
    }

    (void)fclose(out);

    free(source);

    return true;
}


/*
d_edit_delete_line
  Removes one line, terminator and all.  Every later line moves up by one, so
a caller that edits by line number must re-measure afterwards -- which the
fixpoint driver already does, since it applies one repair per file per pass
and rebuilds the tree from disk on the next.
*/
bool d_edit_delete_line(
    const char* _path,
    uint32_t    _line
)
{
    FILE* const handle = fopen(_path, "rb");

    if ((!handle) || (_line == 0))
    {
        if (handle)
        {
            (void)fclose(handle);
        }

        return false;
    }

    (void)fseek(handle, 0, SEEK_END);

    const long size = ftell(handle);

    (void)fseek(handle, 0, SEEK_SET);

    char* const source = malloc((size_t)size + 1u);

    if (!source)
    {
        (void)fclose(handle);
        return false;
    }

    const size_t read = fread(source, 1u, (size_t)size, handle);

    (void)fclose(handle);

    source[read] = '\0';

    size_t   start = 0;
    uint32_t at    = 1;

    while ((at < _line) && (start < read))
    {
        while ((start < read) && (source[start] != '\n'))
        {
            ++start;
        }

        ++start;
        ++at;
    }

    if ((at != _line) || (start >= read))
    {
        free(source);
        return false;
    }

    size_t end = start;

    while ((end < read) && (source[end] != '\n'))
    {
        ++end;
    }

    if (end < read)
    {
        ++end;      // the terminator goes with the line
    }

    FILE* const out = fopen(_path, "wb");

    if (!out)
    {
        free(source);
        return false;
    }

    (void)fwrite(source, 1u, start, out);
    (void)fwrite(source + end, 1u, read - end, out);
    (void)fclose(out);

    free(source);

    return true;
}


/*
d_edit_insert_line
  Inserts `_text` and a terminator before line `_line`; one past the last line
appends. Mirrors d_edit_delete_line: read whole, splice, write whole. The
terminator is spelled as the file spells its own: as the line above ends, or,
at the top, as the line it goes before ends, so an empty line put into a CRLF
file is CRLF too.
*/
bool d_edit_insert_line(
    const char* _path,
    uint32_t    _line,
    const char* _text
)
{
    FILE* const handle = fopen(_path, "rb");

    if ( (!handle) || (_line == 0) || (!_text) )
    {
        if (handle)
        {
            (void)fclose(handle);
        }

        return false;
    }

    (void)fseek(handle, 0, SEEK_END);

    const long size = ftell(handle);

    (void)fseek(handle, 0, SEEK_SET);

    char* const source = malloc((size_t)size + 1u);

    if (!source)
    {
        (void)fclose(handle);

        return false;
    }

    const size_t read = fread(source, 1u, (size_t)size, handle);

    (void)fclose(handle);

    size_t   start = 0;
    uint32_t at    = 1;

    // find the first byte of line `_line`, or the end for an append
    while ( (at < _line) && (start < read) )
    {
        while ( (start < read) && (source[start] != '\n') )
        {
            ++start;
        }

        ++start;
        ++at;
    }

    if ( (at != _line) || (start > read) )
    {
        free(source);

        return false;
    }

    FILE* const out = fopen(_path, "wb");

    if (!out)
    {
        free(source);

        return false;
    }

    // the line above's terminator, or the next line's when there is none
    size_t next = start;

    while ( (next < read) && (source[next] != '\n') )
    {
        ++next;
    }

    const bool crlf = (start > 0u)
                    ? ( (start >= 2u) && (source[start - 2u] == '\r') )
                    : ( (next < read) && (next > 0u) &&
                        (source[next - 1u] == '\r') );

    const char* const eol    = crlf ? "\r\n" : "\n";
    const size_t      length = strlen(_text);
    bool              ok     = (fwrite(source, 1u, start, out) == start);

    ok = ok && (fwrite(_text, 1u, length, out) == length);
    ok = ok && (fputs(eol, out) != EOF);
    ok = ok && (fwrite(source + start, 1u, read - start, out) == read - start);
    ok = (fclose(out) == 0) && ok;

    free(source);

    return ok;
}


/**
 * @brief Reads a whole file into memory.
 *
 * @param[in]  _path      the file.
 * @param[out] _out_size  receives its size in bytes.
 * @return the bytes, which the caller frees, or NULL when the file cannot be
 *         opened, measured or read.
 */
static
char* d_internal_edit_load(
    const char* _path,
    size_t*     _out_size
)
{
    FILE* const handle = fopen(_path, "rb");

    if (!handle)
    {
        return NULL;
    }

    (void)fseek(handle, 0, SEEK_END);

    const long size = ftell(handle);

    (void)fseek(handle, 0, SEEK_SET);

    char* const source = (size >= 0) ? malloc((size_t)size + 1u) : NULL;

    // a size the file cannot report, or memory that cannot hold it
    if (!source)
    {
        (void)fclose(handle);

        return NULL;
    }

    *_out_size = fread(source, 1u, (size_t)size, handle);
    (void)fclose(handle);

    return source;
}


/**
 * @brief Reports whether one line can move `_delta` columns.
 *
 * @param[in] _line    the line's first byte.
 * @param[in] _length  its bytes, its terminator excluded.
 * @param[in] _delta   columns right, or left where negative.
 * @return true when the line is empty, or moves left over spaces it has, or
 *         moves right and still ends within LINE_LIMIT characters.
 *
 * Characters are code points, as the guide counts them: a UTF-8
 * continuation byte adds none.
 */
static
bool d_internal_edit_line_may_shift(
    const char* _line,
    size_t      _length,
    int         _delta
)
{
    size_t spaces = 0u;
    size_t points = 0u;

    while ( (spaces < _length) &&
            (_line[spaces] == ' ') )
    {
        ++spaces;
    }

    // a line of nothing, or of spaces alone, stays as it is
    if (spaces == _length)
    {
        return true;
    }

    // to the left, only over spaces the line has
    if (_delta < 0)
    {
        return (spaces >= (size_t)(-(long)_delta));
    }

    for (size_t at = 0u; at < _length; ++at)
    {
        // a continuation byte belongs to the character before it
        if ((((unsigned char)_line[at]) & 0xC0u) != 0x80u)
        {
            ++points;
        }
    }

    return ((points + (size_t)_delta) <= LINE_LIMIT);
}


/**
 * @brief Finds the offset at which a file's line begins.
 *
 * @param[in]  _source   the file's bytes.
 * @param[in]  _size     how many there are.
 * @param[in]  _line     the line, 1-based; one past the last finds the end.
 * @param[out] _out_at   receives the offset of the line's first byte.
 * @return true when the file has that line, or ends just before it.
 */
static
bool d_internal_edit_line_start(
    const char* _source,
    size_t      _size,
    uint32_t    _line,
    size_t*     _out_at
)
{
    size_t   at   = 0u;
    uint32_t line = 1u;

    // each newline before the line ends one line more
    while ( (line < _line) &&
            (at < _size) )
    {
        if (_source[at] == '\n')
        {
            ++line;
        }

        ++at;
    }

    *_out_at = at;

    return (line == _line);
}


/**
 * @brief Reports, or makes, the move of lines `_first` to `_last`.
 *
 * @param[in] _source  the file's bytes.
 * @param[in] _size    how many there are.
 * @param[in] _first   the first line, 1-based.
 * @param[in] _last    the last line.
 * @param[in] _delta   columns right, or left where negative.
 * @param[in] _out     the file to write the moved text to, or NULL to only
 *                     ask whether the move can be made.
 * @return true when every line in the range exists and can move, and, with
 *         `_out`, every byte was written.
 *
 * A line's terminator, LF or CRLF, is written as it was read; a line of
 * spaces alone, or of nothing, is written unchanged.
 */
static
bool d_internal_edit_shift(
    const char* _source,
    size_t      _size,
    uint32_t    _first,
    uint32_t    _last,
    int         _delta,
    FILE*       _out
)
{
    size_t begin = 0u;
    size_t at    = 0u;
    bool   ok    = ( (_first != 0u)                               &&
                     (_last >= _first)                            &&
                     (d_internal_edit_line_start(_source,
                                                 _size,
                                                 _first,
                                                 &begin)) );

    // the lines before the range, unchanged
    if ( (ok) &&
         (_out) )
    {
        ok = (fwrite(_source, 1u, begin, _out) == begin);
    }

    at = begin;

    for (uint32_t line = _first; (ok) && (line <= _last); ++line)
    {
        const size_t start = at;

        // the range must not run past the file's last line
        if (start >= _size)
        {
            ok = false;
            break;
        }

        while ( (at < _size) && (_source[at] != '\n') )
        {
            ++at;
        }

        const size_t end    = at;
        const size_t length = ( (end > start) &&
                                (_source[end - 1u] == '\r') )
                            ? (end - start - 1u)
                            : (end - start);
        size_t       spaces = 0u;

        at = (at < _size) ? (at + 1u) : at;
        ok = d_internal_edit_line_may_shift(_source + start,
                                            length,
                                            _delta);

        while ( (spaces < length) &&
                (_source[start + spaces] == ' ') )
        {
            ++spaces;
        }

        // a line that holds something moves; an empty one is copied as is
        if ( (ok)   &&
             (_out) &&
             (spaces < length) )
        {
            const size_t skip = (_delta < 0) ? (size_t)(-(long)_delta) : 0u;

            for (int pad = 0; (ok) && (pad < _delta); ++pad)
            {
                ok = (fputc(' ', _out) != EOF);
            }

            ok = ( (ok) &&
                   (fwrite(_source + start + skip,
                           1u,
                           at - start - skip,
                           _out) == (at - start - skip)) );
        }
        else if ( (ok) &&
                  (_out) )
        {
            ok = (fwrite(_source + start, 1u, at - start, _out) ==
                  (at - start));
        }
    }

    // the lines after the range, unchanged
    if ( (ok) &&
         (_out) )
    {
        ok = (fwrite(_source + at, 1u, _size - at, _out) == (_size - at));
    }

    return ok;
}


/*
d_edit_replace_lines
  Finds the run's first byte, the end of its last line and that line's
terminator in one walk, then writes the file once: what came before, the
new text with each `\n` spelled as the run's first line ended, the run's own
last terminator, and what came after.
*/
bool d_edit_replace_lines(
    const char* _path,
    uint32_t    _first,
    uint32_t    _last,
    const char* _text
)
{
    size_t      size   = 0u;
    char* const source = ( (_path)            &&
                           (_text)            &&
                           (_first != 0u)     &&
                           (_last >= _first) )
                       ? d_internal_edit_load(_path, &size)
                       : NULL;
    size_t      begin  = 0u;
    size_t      end    = 0u;

    // the run must lie wholly within the file
    if ( (!source)                                    ||
         (!d_internal_edit_line_start(source,
                                      size,
                                      _first,
                                      &begin))        ||
         (begin >= size)                              ||
         (!d_internal_edit_line_start(source,
                                      size,
                                      _last,
                                      &end))          ||
         (end >= size) )
    {
        free(source);

        return false;
    }

    // how the run's first line ends spells every new line break
    const char* first_end = memchr(source + begin, '\n', size - begin);
    const bool  crlf      = ( (first_end)                &&
                              (first_end > source + begin) &&
                              (first_end[-1] == '\r') );

    // the last line's text ends at its terminator, which is kept
    while ( (end < size) && (source[end] != '\n') )
    {
        ++end;
    }

    if ( (end > begin) &&
         (source[end - 1u] == '\r') &&
         (end < size) )
    {
        --end;
    }

    FILE* const  out    = fopen(_path, "wb");
    const size_t length = strlen(_text);
    bool         ok     = (out != NULL);

    ok = ok && (fwrite(source, 1u, begin, out) == begin);

    for (size_t at = 0u; (ok) && (at < length); ++at)
    {
        if ( (_text[at] == '\n') &&
             (crlf) )
        {
            ok = (fputc('\r', out) != EOF);
        }

        ok = ok && (fputc(_text[at], out) != EOF);
    }

    ok = ok && (fwrite(source + end, 1u, size - end, out) == (size - end));

    if (out)
    {
        ok = (fclose(out) == 0) && ok;
    }

    free(source);

    return ok;
}


/*
d_edit_may_shift_lines
  The same walk as d_edit_shift_lines, with nothing written, so the answer
cannot differ from what the move would do.
*/
bool d_edit_may_shift_lines(
    const char* _path,
    uint32_t    _first,
    uint32_t    _last,
    int         _delta
)
{
    size_t      size   = 0u;
    char* const source = (_path) ? d_internal_edit_load(_path, &size) : NULL;

    if (!source)
    {
        return false;
    }

    const bool may = d_internal_edit_shift(source,
                                           size,
                                           _first,
                                           _last,
                                           _delta,
                                           NULL);

    free(source);

    return may;
}


/*
d_edit_shift_lines
  Asks first, in memory, so that a refused move leaves the file as it was,
then writes the whole file once.
*/
bool d_edit_shift_lines(
    const char* _path,
    uint32_t    _first,
    uint32_t    _last,
    int         _delta
)
{
    size_t      size   = 0u;
    char* const source = (_path) ? d_internal_edit_load(_path, &size) : NULL;

    if (!source)
    {
        return false;
    }

    // nothing is written unless every line can move
    if (!d_internal_edit_shift(source,
                               size,
                               _first,
                               _last,
                               _delta,
                               NULL))
    {
        free(source);

        return false;
    }

    FILE* const out = fopen(_path, "wb");
    bool        ok  = (out != NULL);

    if (ok)
    {
        ok = d_internal_edit_shift(source,
                                   size,
                                   _first,
                                   _last,
                                   _delta,
                                   out);
        ok = (fclose(out) == 0) && ok;
    }

    free(source);

    return ok;
}
