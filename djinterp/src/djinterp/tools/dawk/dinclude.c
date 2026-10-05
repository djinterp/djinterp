/*******************************************************************************
* djinterp [djinterp]                                                 dinclude.c
*
* Include extension:
*   A line is an include row only if it is `#include`, optional blanks, a
* bracketed or quoted target, then optionally blanks and a `//` comment, and
* nothing else.  Anything more is emitted as a row with no cells: the table
* will not lay it out, and the lossless-rebuild proof in dlayout would refuse
* it anyway.  Declining to guess is the whole of this file's error handling.
*
*
* path:      /src/djinterp/tools/dawk/dinclude.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dinclude.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fgets, fclose
#include <string.h>  // strncmp, strlen
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dbanner.h"  // D_BANNER_LINE_MAX
#include "../../../../inc/djinterp/tools/dawk/dedit.h"    // d_edit_trim_end


/*
d_internal_cell
  Adds a cell node measured at `[_from, _from + _width)` on the line.
*/
static void
d_internal_cell(
    struct d_node_tree* _tree,
    uint32_t            _row,
    const char*         _type,
    const char*         _line,
    size_t              _from,
    size_t              _width,
    uint32_t            _line_number
)
{
    const uint32_t cell = d_node_add(_tree, _row, _type);

    if (cell == D_DSS_NO_INDEX)
    {
        return;
    }

    struct d_node* const node = d_node_at(_tree, cell);

    node->line         = _line_number;
    node->start_column = (uint32_t)(_from + 1u);
    node->width        = (uint32_t)_width;
    node->end_column   = (uint32_t)(_from + _width);

    (void)d_node_set_text(_tree, cell, _line + _from, _width);

    return;
}


/*
d_internal_row
  Parses one include line into its cells.  Returns false when the line does
not have the shape of an include row, in which case no cells are added.
*/
static bool
d_internal_row(
    struct d_node_tree* _tree,
    uint32_t            _row,
    const char*         _line,
    size_t              _length,
    uint32_t            _line_number
)
{
    size_t at = strlen("#include");

    while ((at < _length) && ((_line[at] == ' ') || (_line[at] == '\t')))
    {
        ++at;
    }

    if ((at >= _length) || ((_line[at] != '<') && (_line[at] != '"')))
    {
        return false;
    }

    const char close = (_line[at] == '<') ? '>' : '"';

    size_t end = at + 1u;

    while ((end < _length) && (_line[end] != close))
    {
        ++end;
    }

    if (end >= _length)
    {
        return false;
    }

    const size_t directive_width = end + 1u;

    size_t rest = directive_width;

    while ((rest < _length) && ((_line[rest] == ' ') || (_line[rest] == '\t')))
    {
        ++rest;
    }

    bool has_summary = false;

    if (rest < _length)
    {
        // anything after the directive other than a // comment is not a row
        if ((rest + 1u >= _length) || (_line[rest] != '/') ||
            (_line[rest + 1u] != '/'))
        {
            return false;
        }

        has_summary = true;
    }

    d_internal_cell(_tree, _row, "directive", _line, 0u, directive_width,
                    _line_number);

    if (has_summary)
    {
        size_t summary_end = _length;

        while ((summary_end > rest) && (_line[summary_end - 1u] == ' '))
        {
            --summary_end;
        }

        d_internal_cell(_tree, _row, "summary", _line, rest,
                        summary_end - rest, _line_number);
    }
    else
    {
        const uint32_t absent = d_node_add(_tree, _row, "summary");

        if (absent != D_DSS_NO_INDEX)
        {
            d_node_at(_tree, absent)->present = false;
        }
    }

    return true;
}


uint32_t
d_include_build(
    struct d_node_tree* _tree,
    uint32_t            _file,
    const char*         _path
)
{
    // parameter validation first
    if ((!_tree) || (!_path) || (_file == D_DSS_NO_INDEX))
    {
        return D_DSS_NO_INDEX;
    }

    FILE* const handle = fopen(_path, "rb");

    if (!handle)
    {
        return D_DSS_NO_INDEX;
    }

    char     buffer[D_BANNER_LINE_MAX];
    uint32_t line   = 0;
    uint32_t block  = D_DSS_NO_INDEX;
    uint32_t group  = D_DSS_NO_INDEX;
    uint32_t groups = 0;

    while (fgets(buffer, (int)sizeof(buffer), handle))
    {
        ++line;

        const size_t length = d_edit_trim_end(buffer, strlen(buffer));

        if (strncmp(buffer, "#include", 8u) != 0)
        {
            // a comment line inside a run of includes -- a category such as
            // `// std` -- ends the group but continues the block
            group = D_DSS_NO_INDEX;

            if ((block != D_DSS_NO_INDEX) &&
                (strncmp(buffer, "//", 2u) == 0))
            {
                continue;
            }

            block = D_DSS_NO_INDEX;
            continue;
        }

        if (block == D_DSS_NO_INDEX)
        {
            block = d_node_add(_tree, _file, "include-block");

            if (block == D_DSS_NO_INDEX)
            {
                break;
            }
        }

        // the first include after anything else opens a new group
        if (group == D_DSS_NO_INDEX)
        {
            group = d_node_add(_tree, block, "include-group");

            if (group == D_DSS_NO_INDEX)
            {
                break;
            }

            ++groups;
        }

        const uint32_t row = d_node_add(_tree, group, "include");

        if (row == D_DSS_NO_INDEX)
        {
            break;
        }

        d_node_at(_tree, row)->line = line;

        (void)d_node_set_text(_tree, row, buffer, length);

        if (!d_internal_row(_tree, row, buffer, length, line))
        {
            (void)d_node_set_attribute(_tree, row, "irregular", "true");
        }
    }

    (void)fclose(handle);

    return groups;
}
