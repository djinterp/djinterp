/*******************************************************************************
* djinterp [djinterp]                                                  dlayout.c
*
* Layout:
*   The computed-value lookup is first match wins, in sheet order, which is
* README section 2.4's rule and the default the runtime selection between
* source order and layers will switch away from.  It lives here rather than
* in the registry because layout is the first thing that has to ask what
* value some *other* node has, rather than evaluating the node in hand.
*   Table layout computes each column's start as the largest end of the
* column before it plus that column's gap, taken over only the rows that
* have a cell in the column.  A row with no cell in a column does not push
* that column; a long directive with no summary should not move every other
* summary in the group.
*   A repair rebuilds each row's line from its cells, and first proves that
* the rebuild is lossless: the line must consist of its cells and whitespace
* and nothing else.  A row that fails the proof blocks the whole table,
* because aligning some rows of a group and not others is worse than
* leaving the group as it was.
*
*
* path:      /src/djinterp/tools/dawk/dlayout.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dlayout.h"  // corresponding header
// std
#include <stdlib.h>  // calloc, free
#include <string.h>  // strcmp, strlen, memcpy, memset
// djinterp
#include "../../../../inc/djinterp/tools/dawk/daudit.h"   // d_audit_verify
#include "../../../../inc/djinterp/tools/dawk/dbanner.h"  // D_BANNER_LINE_MAX
#include "../../../../inc/djinterp/tools/dawk/dedit.h"    // d_edit_read_line
#include "../../../../inc/djinterp/tools/dawk/dmatch.h"   // d_match_rule


// D_LAYOUT_COLUMN_MAX
//   constant: the most cells a row may carry.  A table wider than this is not
// a formatting table and is refused rather than truncated.
#define D_LAYOUT_COLUMN_MAX 16


/*
d_internal_first_value
  Returns the first value term of the first declaration of `_property` in the
first rule that matches `_node`, or NULL.
*/
static const struct d_dss_value*
d_internal_first_value(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node,
    const char*               _property
)
{
    const struct d_dss_value* found = NULL;
    size_t                    best  = 0;

    for (size_t at = 0; at < d_dss_rule_count(_sheet); ++at)
    {
        const struct d_dss_rule* const rule = d_dss_rule_at(_sheet, at);

        // a rule that cannot beat the one already found is not matched at all
        if ((found) && (!d_dss_outranks(_sheet, at, best)))
        {
            continue;
        }

        if (!d_match_rule(_sheet, _tree, _node, rule))
        {
            continue;
        }

        for (uint32_t which = 0; which < rule->declaration_count; ++which)
        {
            const struct d_dss_declaration* const declaration =
                d_dss_declaration_at(_sheet, rule->first_declaration + which);

            if (strcmp(d_dss_text(_sheet, declaration->property),
                       _property) != 0)
            {
                continue;
            }

            // the best-ranked rule to say anything is the answer; without
            // layers that is the first
            found = d_dss_value_at(_sheet, declaration->first_value);
            best  = at;
            break;
        }
    }

    return found;
}


const char*
d_layout_computed(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node,
    const char*               _property
)
{
    const struct d_dss_value* const value =
        d_internal_first_value(_sheet, _tree, _node, _property);

    if ((!value) || (value->text == D_DSS_NO_INDEX))
    {
        return NULL;
    }

    return d_dss_text(_sheet, value->text);
}


double
d_layout_computed_number(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node,
    const char*               _property,
    double                    _default
)
{
    const struct d_dss_value* const value =
        d_internal_first_value(_sheet, _tree, _node, _property);

    if ((!value) || (value->kind != D_DSS_VALUE_NUMBER))
    {
        return _default;
    }

    return value->number;
}


/*
d_internal_is
  Reports whether a node's computed `display` is `_want`.
*/
static bool
d_internal_is(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node,
    const char*               _want
)
{
    const char* const display = d_layout_computed(_sheet, _tree, _node,
                                                  "display");

    return ((display) && (strcmp(display, _want) == 0));
}


// d_internal_row
//   struct: one row's cells, in order, as node indices.
struct d_internal_row
{
    uint32_t  node;
    uint32_t  cells[D_LAYOUT_COLUMN_MAX];
    uint32_t  cell_count;
};


/*
d_internal_collect
  Gathers the rows of a table and the present cells of each row.  Returns the
row count, or zero when the table is empty or too wide to lay out.  The
caller frees `*_out_rows`.
*/
static size_t
d_internal_collect(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _table,
    struct d_internal_row**   _out_rows
)
{
    struct d_node* const table = d_node_at(_tree, _table);

    *_out_rows = NULL;

    if (!table)
    {
        return 0;
    }

    // `display: contents` makes a node transparent: its children are laid
    // out as if they were its parent's.  That is what lets a sheet choose
    // whether alignment is scoped to a group or to the block around it.
    uint32_t stack[64];
    size_t   depth = 0;
    size_t   count = 0;

    uint32_t found[4096];

    stack[depth++] = table->first_child;

    while (depth > 0)
    {
        uint32_t row = stack[--depth];

        while (row != D_DSS_NO_INDEX)
        {
            const uint32_t next = d_node_at(_tree, row)->next_sibling;

            if (d_internal_is(_sheet, _tree, row, "contents"))
            {
                if ((next != D_DSS_NO_INDEX) && (depth < 64u))
                {
                    stack[depth++] = next;
                }

                row = d_node_at(_tree, row)->first_child;
                continue;
            }

            if ( (d_internal_is(_sheet, _tree, row, "table-row")) &&
                 (count < (sizeof(found) / sizeof(found[0]))) )
            {
                found[count] = row;
                ++count;
            }

            row = next;
        }
    }

    if (count == 0)
    {
        return 0;
    }

    struct d_internal_row* const rows = calloc(count,
                                               sizeof(struct d_internal_row));

    if (!rows)
    {
        return 0;
    }

    // document order is kept: rows are sorted by the line they sit on
    for (size_t i = 1; i < count; ++i)
    {
        const uint32_t key = found[i];
        size_t         j   = i;

        while ( (j > 0) &&
                (d_node_at(_tree, found[j - 1u])->line
                 > d_node_at(_tree, key)->line) )
        {
            found[j] = found[j - 1u];
            --j;
        }

        found[j] = key;
    }

    for (size_t at = 0; at < count; ++at)
    {
        const uint32_t row = found[at];

        rows[at].node = row;

        for (uint32_t cell = d_node_at(_tree, row)->first_child;
             cell != D_DSS_NO_INDEX;
             cell = d_node_at(_tree, cell)->next_sibling)
        {
            const struct d_node* const cell_node = d_node_at(_tree, cell);

            // an absent cell has no geometry to align
            if ((!cell_node->present) ||
                (!d_internal_is(_sheet, _tree, cell, "table-cell")))
            {
                continue;
            }

            if (rows[at].cell_count == D_LAYOUT_COLUMN_MAX)
            {
                free(rows);
                return 0;
            }

            rows[at].cells[rows[at].cell_count] = cell;
            ++rows[at].cell_count;
        }
    }

    *_out_rows = rows;

    return count;
}


/*
d_internal_columns
  Computes each column's start.  Column zero starts where the rightmost first
cell does; column k starts at the largest (start of k-1 plus width of the
cell in k-1 plus gap of k), over the rows that have a cell in column k.
Returns the column count.
*/
static uint32_t
d_internal_columns(
    const struct d_dss_sheet*    _sheet,
    struct d_node_tree*          _tree,
    const struct d_internal_row* _rows,
    size_t                       _row_count,
    uint32_t                     _out_start[D_LAYOUT_COLUMN_MAX]
)
{
    uint32_t columns = 0;

    for (size_t row = 0; row < _row_count; ++row)
    {
        if (_rows[row].cell_count > columns)
        {
            columns = _rows[row].cell_count;
        }
    }

    memset(_out_start, 0, sizeof(uint32_t) * D_LAYOUT_COLUMN_MAX);

    for (size_t row = 0; row < _row_count; ++row)
    {
        if (_rows[row].cell_count == 0)
        {
            continue;
        }

        const struct d_node* const first = d_node_at(_tree,
                                                     _rows[row].cells[0]);

        if (first->start_column > _out_start[0])
        {
            _out_start[0] = first->start_column;
        }
    }

    for (uint32_t column = 1; column < columns; ++column)
    {
        uint32_t minimum = 0;
        bool     at_least = false;

        for (size_t row = 0; row < _row_count; ++row)
        {
            if (_rows[row].cell_count <= column)
            {
                continue;
            }

            const uint32_t cell = _rows[row].cells[column];

            const struct d_node* const previous =
                d_node_at(_tree, _rows[row].cells[column - 1u]);

            // `min-gap` is the guide's "at least"; `gap` is CSS's exact gap
            double gap = d_layout_computed_number(_sheet, _tree, cell,
                                                  "min-gap", -1.0);

            if (gap >= 0.0)
            {
                at_least = true;
            }
            else
            {
                gap = d_layout_computed_number(_sheet, _tree, cell, "gap",
                                               1.0);
            }

            const uint32_t need = _out_start[column - 1u] + previous->width
                                + (uint32_t)gap;

            if (need > minimum)
            {
                minimum = need;
            }
        }

        _out_start[column] = minimum;

        if (!at_least)
        {
            continue;
        }

        // Under an at-least gap the group already chose its column; keep the
        // choice the most rows made, so a repair moves the outliers rather
        // than the group.  Ties go to the smaller column, so excess spacing
        // is removed rather than propagated.
        uint32_t best       = 0;
        size_t   best_count = 0;

        for (size_t row = 0; row < _row_count; ++row)
        {
            if (_rows[row].cell_count <= column)
            {
                continue;
            }

            const uint32_t candidate =
                d_node_at(_tree, _rows[row].cells[column])->start_column;

            if (candidate < minimum)
            {
                continue;
            }

            size_t count = 0;

            for (size_t other = 0; other < _row_count; ++other)
            {
                if ( (_rows[other].cell_count > column) &&
                     (d_node_at(_tree, _rows[other].cells[column])
                          ->start_column == candidate) )
                {
                    ++count;
                }
            }

            if ( (count > best_count) ||
                 ((count == best_count) && (candidate < best)) )
            {
                best       = candidate;
                best_count = count;
            }
        }

        if (best_count > 0)
        {
            _out_start[column] = best;
        }
    }

    return columns;
}


bool
d_layout_table_holds(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _table,
    bool*                     _out_evaluated
)
{
    struct d_internal_row* rows  = NULL;
    const size_t           count = d_internal_collect(_sheet, _tree, _table,
                                                      &rows);

    *_out_evaluated = (count > 0);

    if (count == 0)
    {
        return true;
    }

    uint32_t start[D_LAYOUT_COLUMN_MAX];

    (void)d_internal_columns(_sheet, _tree, rows, count, start);

    bool holds = true;

    for (size_t row = 0; (holds) && (row < count); ++row)
    {
        for (uint32_t cell = 0; cell < rows[row].cell_count; ++cell)
        {
            if (d_node_at(_tree, rows[row].cells[cell])->start_column
                != start[cell])
            {
                holds = false;
                break;
            }
        }
    }

    free(rows);

    return holds;
}


/*
d_internal_rebuild
  Builds a row's line with each cell at its column, after proving that the
original line is exactly its cells separated by whitespace.  Returns false
when the proof fails or the result would not fit.
*/
static bool
d_internal_rebuild(
    struct d_node_tree*          _tree,
    const struct d_internal_row* _row,
    const uint32_t               _start[D_LAYOUT_COLUMN_MAX],
    const char*                  _line,
    char*                        _out,
    size_t                       _out_size
)
{
    const size_t length = strlen(_line);

    // the proof: outside the cells there is only whitespace
    size_t covered = 0;

    for (uint32_t cell = 0; cell < _row->cell_count; ++cell)
    {
        const struct d_node* const node = d_node_at(_tree, _row->cells[cell]);

        const size_t from = node->start_column - 1u;

        if ((from < covered) || ((from + node->width) > length))
        {
            return false;
        }

        for (size_t at = covered; at < from; ++at)
        {
            if ((_line[at] != ' ') && (_line[at] != '\t'))
            {
                return false;
            }
        }

        covered = from + node->width;
    }

    for (size_t at = covered; at < length; ++at)
    {
        if ((_line[at] != ' ') && (_line[at] != '\t'))
        {
            return false;
        }
    }

    size_t used = 0;

    for (uint32_t cell = 0; cell < _row->cell_count; ++cell)
    {
        const struct d_node* const node = d_node_at(_tree, _row->cells[cell]);

        const size_t to = _start[cell] - 1u;

        if ((to + node->width + 1u) > _out_size)
        {
            return false;
        }

        while (used < to)
        {
            _out[used] = ' ';
            ++used;
        }

        (void)memcpy(_out + used, _line + (node->start_column - 1u),
                     node->width);

        used += node->width;
    }

    _out[used] = '\0';

    return true;
}


bool
d_layout_table_repair(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _table,
    const char*               _path,
    const char**              _out_refusal
)
{
    if (_out_refusal)
    {
        *_out_refusal = NULL;
    }

    struct d_internal_row* rows  = NULL;
    const size_t           count = d_internal_collect(_sheet, _tree, _table,
                                                      &rows);

    if (count == 0)
    {
        return false;
    }

    uint32_t start[D_LAYOUT_COLUMN_MAX];

    (void)d_internal_columns(_sheet, _tree, rows, count, start);

    // Every row is rebuilt before any is written.  If one row cannot prove
    // its rebuild lossless, nothing in the table is touched.
    char (*built)[D_BANNER_LINE_MAX] = calloc(count, D_BANNER_LINE_MAX);

    if (!built)
    {
        free(rows);
        return false;
    }

    bool ok = true;

    for (size_t row = 0; (ok) && (row < count); ++row)
    {
        char line[D_BANNER_LINE_MAX];

        const uint32_t line_number = d_node_at(_tree, rows[row].node)->line;

        ok = ( (line_number != 0) &&
               (d_edit_read_line(_path, line_number, line, sizeof(line))) &&
               (d_internal_rebuild(_tree, &rows[row], start, line,
                                   built[row], sizeof(built[row]))) );

#ifndef D_AUDIT_DISABLED
        // the rebuild proved the input lossless; this checks the output
        // independently -- a table may move text and may not change it
        struct d_audit_claim audit;
        const char*          why = NULL;

        memset(&audit, 0, sizeof(audit));
        audit.kind = D_AUDIT_GEOMETRY;

        if ((ok) && (!d_audit_verify(line, built[row], &audit, &why)))
        {
            ok = false;

            if (_out_refusal)
            {
                *_out_refusal = why;
            }
        }
#endif
    }

    bool wrote = false;

    for (size_t row = 0; (ok) && (row < count); ++row)
    {
        char line[D_BANNER_LINE_MAX];

        const uint32_t line_number = d_node_at(_tree, rows[row].node)->line;

        (void)d_edit_read_line(_path, line_number, line, sizeof(line));

        // lines already in place are left alone, byte for byte
        if (strcmp(line, built[row]) == 0)
        {
            continue;
        }

        if (d_edit_rewrite_line(_path, line_number, built[row]))
        {
            wrote = true;
        }
    }

    free(built);
    free(rows);

    return wrote;
}
