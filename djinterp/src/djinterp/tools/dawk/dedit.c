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
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dedit.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fgets, fwrite
#include <stdlib.h>  // malloc, free
#include <string.h>  // strlen

/*
d_edit_trim_end
  Returns the length with trailing newline and carriage return removed.
*/
size_t
d_edit_trim_end(
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
bool
d_edit_read_line(
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
bool
d_edit_rewrite_line(
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
bool
d_edit_delete_line(
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
  SCRATCH: dcheck.c calls this and neither archive defines it.  Inserts
`_text` and a terminator before line `_line`; one past the last line appends.
Mirrors d_edit_delete_line: read whole, splice, write whole.
*/
bool
d_edit_insert_line(
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

    const size_t length = strlen(_text);
    bool         ok     = (fwrite(source, 1u, start, out) == start);

    ok = ok && (fwrite(_text, 1u, length, out) == length);
    ok = ok && (fputc('\n', out) != EOF);
    ok = ok && (fwrite(source + start, 1u, read - start, out) == read - start);
    ok = (fclose(out) == 0) && ok;

    free(source);

    return ok;
}

/*
d_internal_read_whole
  Reads a file whole, for a primitive that writes it whole again.  NULL
when it cannot be opened or held; the caller frees what is returned.
*/
static char*
d_internal_read_whole(
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

    *_out_size = source ? fread(source, 1u, (size_t)size, handle) : 0u;
    (void)fclose(handle);

    return source;
}

/*
d_internal_line_count
  The lines a text holds: one for each line feed, and one more for a last
line that has none.
*/
static uint32_t
d_internal_line_count(
    const char* _text,
    size_t      _size
)
{
    uint32_t lines = (_size > 0u) ? 1u : 0u;

    for (size_t at = 0u; (at + 1u) < _size; ++at)
    {
        lines += (_text[at] == '\n') ? 1u : 0u;
    }

    return lines;
}

/*
d_internal_end_line
  Writes a line's terminator: a line feed, after a carriage return when the
line that is ending had one and its text has not already supplied it.
*/
static void
d_internal_end_line(
    FILE* _out,
    bool  _carriage
)
{
    if (_carriage)
    {
        (void)fputc('\r', _out);
    }

    (void)fputc('\n', _out);

    return;
}

// d_internal_run
//   struct: a run of lines, by its first and last, both 1-based.
struct d_internal_run
{
    uint32_t  first;
    uint32_t  last;
};

/*
d_internal_write_replacing
  Writes a file's text back with one run of lines replaced.  Every line
outside the run goes out as it was; the replacement stands where the run
began, the rest of the run is not written, and the run's last line gives
its terminator to the replacement's.
*/
static void
d_internal_write_replacing(
    FILE*                        _out,
    const char*                  _source,
    size_t                       _size,
    const struct d_internal_run* _run,
    const char*                  _replacement
)
{
    uint32_t at    = 1u;
    size_t   start = 0u;

    for (size_t which = 0u; which <= _size; ++which)
    {
        if ( (which != _size) && (_source[which] != '\n') )
        {
            continue;
        }

        const bool inside = ( (at >= _run->first) && (at <= _run->last) );

        if (at == _run->first)
        {
            (void)fwrite(_replacement, 1u, strlen(_replacement), _out);
        }
        else if (!inside)
        {
            (void)fwrite(_source + start, 1u, which - start, _out);
        }

        // a line's own terminator, and for the run its last line's
        if ( (which < _size) && ( (!inside) || (at == _run->last) ) )
        {
            d_internal_end_line(_out,
                                ( (inside) && (which > start) &&
                                  (_source[which - 1u] == '\r') ));
        }

        start = which + 1u;
        ++at;
    }

    return;
}

/*
d_edit_replace_lines
  Replaces lines `_first` to `_last` by `_replacement`, which may itself be
several lines joined by line feeds, in one write.  As d_edit_rewrite_line
does for one line, the file is read whole and written whole.  Reports
failure when the file has no such run, so that a stale line number writes
nothing: the run is counted before the file is opened for writing.
*/
bool
d_edit_replace_lines(
    const char* _path,
    uint32_t    _first,
    uint32_t    _last,
    const char* _replacement
)
{
    const struct d_internal_run run    = { _first, _last };
    size_t                      read   = 0u;
    char* const                 source =
        ( (_path) && (_replacement) && (_first > 0u) && (_last >= _first) )
        ? d_internal_read_whole(_path, &read)
        : NULL;
    FILE* const                 out    =
        ( (source) && (_last <= d_internal_line_count(source, read)) )
        ? fopen(_path, "wb")
        : NULL;

    if (!out)
    {
        free(source);

        return false;
    }

    d_internal_write_replacing(out, source, read, &run, _replacement);
    (void)fclose(out);
    free(source);

    return true;
}

/*
d_internal_shift_of
  How far one line of a run being shifted moves: the run's columns, or 0
for a line that keeps its place -- an empty one, and a directive, which
stands in the first column whatever is around it.  Sets `_out_ok` false for
a line the shift may not be made with: one indented by a tab, one a
backslash carries onto the next (its leading space may be inside a literal),
one with fewer spaces than it would lose, and one that is within 80
characters and would not be afterwards.
*/
static int
d_internal_shift_of(
    const char* _line,
    size_t      _length,
    int         _columns,
    bool*       _out_ok
)
{
    size_t spaces = 0u;

    while ( (spaces < _length) && (_line[spaces] == ' ') )
    {
        ++spaces;
    }

    if ( (spaces == _length) || (_line[spaces] == '\r') ||
         (_line[spaces] == '#') )
    {
        return 0;
    }

    const size_t text = ( (_length > 0u) && (_line[_length - 1u] == '\r') )
                      ? (_length - 1u)
                      : _length;

    if ( (_line[spaces] == '\t')                                    ||
         (_line[text - 1u] == '\\')                                 ||
         ( (_columns < 0) && (spaces < (size_t)(-_columns)) )       ||
         ( (_columns > 0) && (text <= 80u) &&
           ((text + (size_t)_columns) > 80u) ) )
    {
        *_out_ok = false;
    }

    return _columns;
}

/*
d_internal_write_shifted
  Writes a file's text back with a run of lines shifted, or -- with no
stream -- only reports whether every line of the run may be.  The two share
one walk so that what is checked is what is written.
*/
static bool
d_internal_write_shifted(
    FILE*                        _out,
    const char*                  _source,
    size_t                       _size,
    const struct d_internal_run* _run,
    int                          _columns
)
{
    uint32_t at    = 1u;
    size_t   start = 0u;
    bool     ok    = true;

    for (size_t which = 0u; which <= _size; ++which)
    {
        if ( (which != _size) && (_source[which] != '\n') )
        {
            continue;
        }

        const bool inside = ( (at >= _run->first) && (at <= _run->last) );
        const int  move   = inside
            ? d_internal_shift_of(_source + start, which - start, _columns,
                                  &ok)
            : 0;

        // spaces added before the line, or taken from its start
        for (int pad = 0; (_out) && (pad < move); ++pad)
        {
            (void)fputc(' ', _out);
        }

        if (_out)
        {
            const size_t skip = (move < 0) ? (size_t)(-move) : 0u;

            (void)fwrite(_source + start + skip, 1u,
                         (which - start) - skip, _out);

            if (which < _size)
            {
                (void)fputc('\n', _out);
            }
        }

        start = which + 1u;
        ++at;
    }

    return ok;
}

/*
d_edit_may_shift_lines
  Reports whether d_edit_shift_lines would move the run: the file can be
read, has the lines, and none of them is one that may not move.  Nothing is
written.  It is the same walk the write makes, so the answer is the write's.
*/
bool
d_edit_may_shift_lines(
    const char* _path,
    uint32_t    _first,
    uint32_t    _last,
    int         _columns
)
{
    const struct d_internal_run run    = { _first, _last };
    size_t                      read   = 0u;
    char* const                 source =
        ( (_path) && (_columns != 0) && (_first > 0u) && (_last >= _first) )
        ? d_internal_read_whole(_path, &read)
        : NULL;
    const bool                  may    =
        ( (source)                                                    &&
          (_last <= d_internal_line_count(source, read))              &&
          (d_internal_write_shifted(NULL, source, read, &run, _columns)) );

    free(source);

    return may;
}

/*
d_edit_shift_lines
  Moves lines `_first` to `_last` right by `_columns`, or left when it is
negative, in one write.  Only the spaces that open a line change, so the
file's tokens cannot; an empty line and a directive keep their places.  The
run is checked whole before the file is opened for writing, and a run with
any line that may not move -- see d_internal_shift_of -- is left as it is.
*/
bool
d_edit_shift_lines(
    const char* _path,
    uint32_t    _first,
    uint32_t    _last,
    int         _columns
)
{
    const struct d_internal_run run    = { _first, _last };
    size_t                      read   = 0u;
    char* const                 source =
        ( (_path) && (_columns != 0) && (_first > 0u) && (_last >= _first) )
        ? d_internal_read_whole(_path, &read)
        : NULL;
    FILE* const                 out    =
        ( (source)                                                    &&
          (_last <= d_internal_line_count(source, read))              &&
          (d_internal_write_shifted(NULL, source, read, &run, _columns)) )
        ? fopen(_path, "wb")
        : NULL;

    if (!out)
    {
        free(source);

        return false;
    }

    (void)d_internal_write_shifted(out, source, read, &run, _columns);
    (void)fclose(out);
    free(source);

    return true;
}
