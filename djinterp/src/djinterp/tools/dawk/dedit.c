/*******************************************************************************
* djinterp [dawk]                                                        dedit.c
*
* Line editing:
*   The whole of the write path.  A line's own terminator is reproduced, so a
* repair never normalises line endings it was not asked about -- a fixer that
* changes bytes no sheet declared has become a defect itself.
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
