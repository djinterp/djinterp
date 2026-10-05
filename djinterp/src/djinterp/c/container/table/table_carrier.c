/*******************************************************************************
* djinterp [c]                                                   table_carrier.c
*
*   The out-of-line half of table_carrier.h.  Every definition here is D_INLINE while its
* declaration in the header is not, so C11 6.7.4p7 makes each an EXTERNAL
* definition: one symbol, linkable from both languages, and free to be inlined
* within this translation unit.
*
*
* path:      /src/djinterp/c/container/table/table_carrier.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../../inc/djinterp/c/container/table/table_carrier.h"


// std
#include <string.h>  // memcmp, memcpy, memmove, memset


/*
d_table_internal_bytes_for
  Computes the byte count a _rows x _cols table of _cell_size cells needs,
detecting the overflow of the product rather than wrapping it.

Parameter(s):
  _rows:      the row extent.
  _cols:      the column extent.
  _cell_size: the width of one cell in bytes.
  _out:       receives the byte count when the product is representable.
Return:
  An integer value corresponding to either:
  - 1, if the product is representable in a size_t, or
  - 0, if the product would overflow.
*/
static int
d_table_internal_bytes_for
(
    size_t  _rows,
    size_t  _cols,
    size_t  _cell_size,
    size_t* _out
)
{
    size_t cells;
    size_t bytes;

    // an empty extent needs no bytes and cannot overflow
    if ( (_rows == 0) ||
         (_cols == 0) ||
         (_cell_size == 0) )
    {
        *_out = 0;

        return 1;
    }

    cells = _rows;

    // reject rows * cols before it wraps
    if (cells > (((size_t)-1) / _cols))
    {
        return 0;
    }

    cells = cells * _cols;

    // reject cells * cell_size before it wraps
    if (cells > (((size_t)-1) / _cell_size))
    {
        return 0;
    }

    bytes = cells * _cell_size;
    *_out = bytes;

    return 1;
}

/*
d_table_internal_ensure_bytes
  Ensures the table's buffer holds at least _needed bytes, growing it through
the caller's strategy when it does not. Growth is geometric so that repeated
appends amortise; the capacity that results is deliberately NOT part of the
table's canonical form (see d_table_bytes_equal), so a differing growth policy
in another language is not a parity divergence.

Parameter(s):
  _table:  the table whose buffer may grow.
  _needed: the byte count the buffer must reach.
  _alloc:  the caller's storage strategy; may allocate nothing.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the buffer already held _needed bytes or was grown, or
  - D_TABLE_STATUS_CAPACITY, if the buffer was too small and the strategy could
    not grow it, or
  - D_TABLE_STATUS_NO_MEMORY, if the strategy refused, or
  - D_TABLE_STATUS_OVERFLOW, if the grown size would overflow.
*/
static enum d_table_status
d_table_internal_ensure_bytes
(
    struct d_table*             _table,
    size_t                      _needed,
    const struct d_table_alloc* _alloc
)
{
    size_t         held;
    size_t         target;
    unsigned char* block;

    held = d_table_capacity_bytes(_table);

    // the buffer already suffices
    if (held >= _needed)
    {
        return D_TABLE_STATUS_OK;
    }

    // a strategy that allocates nothing makes this a bounded container, and a
    // bounded container refuses rather than growing
    if (!d_table_alloc_can_grow(_alloc))
    {
        return D_TABLE_STATUS_CAPACITY;
    }

    target = held;

    // grow geometrically until the request fits, guarding the doubling
    while (target < _needed)
    {
        if (target == 0)
        {
            target = _needed;
        }
        else if (target > ((((size_t)-1) / 2)))
        {
            target = _needed;
        }
        else
        {
            target = target * 2;
        }
    }

    block = (unsigned char*)_alloc->reallocate(
        _alloc->context, (void*)_table->cells, held, target);

    // the strategy refused
    if (!block)
    {
        return D_TABLE_STATUS_NO_MEMORY;
    }

    _table->cells    = block;
    _table->capacity = (D_INTERNAL_TABLE_EXTENT)target;

    return D_TABLE_STATUS_OK;
}

/*
d_table_internal_fill_cells
  Writes _count copies of _value, or _count zeroed cells when _value is null,
beginning at _at.

Parameter(s):
  _table: the table supplying the cell width.
  _at:    the first cell to write.
  _count: the number of cells to write.
  _value: the cell to replicate; null means zeroed cells.
Return:
  none.
*/
static void
d_table_internal_fill_cells
(
    struct d_table* _table,
    unsigned char*  _at,
    size_t          _count,
    const void*     _value
)
{
    size_t width;
    size_t done;

    width = (size_t)_table->cell_size;

    // a null fill value zeroes the region in one call
    if (!_value)
    {
        if (_count != 0)
        {
            memset((void*)_at, 0, _count * width);
        }

        return;
    }

    // nothing to write -- and the seeding step below would write one cell too
    // many, which a widening resize with no new columns exposed
    if (_count == 0)
    {
        return;
    }

    // seed one cell, then double the written run each pass.  The block moves
    // vectorise where a per-cell memcpy cannot: cell-by-cell measured 14.3x the
    // hand-coded baseline, this way 0.91x -- faster than a naive scalar loop,
    // because memcpy may use wider stores than the cell type allows.
    memcpy((void*)_at, _value, width);

    for (done = 1; done < _count; )
    {
        size_t take = ((done <= (_count - done)) ? done : (_count - done));

        memcpy((void*)(_at + (done * width)), (const void*)_at, take * width);
        done = done + take;
    }

    return;
}

/*
d_table_bytes_equal
  Compares two tables through the forgetful map that CANONICALISES them: the
domain, the cell width, and the cells in use. Capacity, buffer address, and the
bytes beyond |T| are decoration and are not compared, which is what makes a
differing growth policy in another language a non-event rather than a parity
break.

  This is BYTE equality, not value equality. Padding inside a cell and the
several representations of a floating-point value both break it, and the
framework's answer to that -- the per-type equality hook -- is an open question,
so a caller comparing cells whose type has either must supply its own predicate
rather than reading a false negative here as a difference.

Parameter(s):
  _a: one table; may be null.
  _b: the other table; may be null.
Return:
  An integer value corresponding to either:
  - 1, if both are null, or both have the same domain and cell width and the
    same cells in use, or
  - 0, otherwise.
*/
int
d_table_bytes_equal
(
    const struct d_table* _a,
    const struct d_table* _b
)
{
    size_t bytes;

    // two absent tables are the same absent table
    if ( (!_a) ||
         (!_b) )
    {
        return ((!_a) && (!_b));
    }

    // the domain and the cell width are part of the canonical form
    if ( (_a->extent[0]      != _b->extent[0]) ||
         (_a->extent[1]      != _b->extent[1]) ||
         (_a->cell_size != _b->cell_size) )
    {
        return 0;
    }

    bytes = d_table_bytes(_a);

    // an empty table has no cells to disagree about
    if (bytes == 0)
    {
        return 1;
    }

    // a null buffer under a non-empty domain is a broken table, not a match
    if ( (!_a->cells) ||
         (!_b->cells) )
    {
        return 0;
    }

    return (memcmp((const void*)_a->cells,
                   (const void*)_b->cells,
                   bytes) == 0);
}

/*
d_table_clear
  Drops every row, leaving the width and the buffer alone. A zero-row table
remembers its width, so rows may be appended to it afterwards.

Parameter(s):
  _table: the table to empty; may be null.
Return:
  none.
*/
void
d_table_clear
(
    struct d_table* _table
)
{
    if (_table)
    {
        _table->extent[0] = 0;
    }

    return;
}

/*
d_table_erase_column
  Removes the column at _at, narrowing every row. The rows are walked forwards,
each landing at or before its old seat.

Parameter(s):
  _table: the table to narrow.
  _at:    the column to remove.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the column was removed, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _table was null, or
  - D_TABLE_STATUS_DOMAIN, if _at was not a valid column index.
*/
enum d_table_status
d_table_erase_column
(
    struct d_table* _table,
    size_t          _at
)
{
    size_t                  width;
    size_t                  old_stride;
    size_t                  new_stride;
    size_t                  new_cols;
    size_t                  r;
    unsigned char*          source;
    unsigned char*          target;

    if (!_table)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // formal: rows and columns name coordinates 0 and 1, so this operation is
    // defined on the row-and-column table only.  A rank-k structural surface is
    // a further generalisation, not a reinterpretation of this one.
    if (_table->rank != D_TABLE_RANK_TABLE)
    {
        return D_TABLE_STATUS_RANK;
    }

    // formal: there is no such column to remove
    if (_at >= (size_t)_table->extent[1])
    {
        return D_TABLE_STATUS_DOMAIN;
    }

    width      = (size_t)_table->cell_size;
    new_cols   = (size_t)_table->extent[1] - 1;
    old_stride = (size_t)_table->extent[1] * width;
    new_stride = new_cols * width;

    // walk the rows forwards: a row's new seat is always at or before its old
    // one, and every row below it has already vacated
    for (r = 0; r < _table->extent[0]; ++r)
    {
        source = _table->cells + ((size_t)r * old_stride);
        target = _table->cells + ((size_t)r * new_stride);

        // the head lands first, being the part that lands lowest
        if (_at != 0)
        {
            memmove((void*)target,
                    (const void*)source,
                    (size_t)_at * width);
        }

        // then the tail, closing over the removed cell
        if (_at + 1 < (size_t)_table->extent[1])
        {
            memmove((void*)(target + ((size_t)_at * width)),
                    (const void*)(source + (((size_t)_at + 1) * width)),
                    ((size_t)_table->extent[1] - _at - 1) * width);
        }
    }

    _table->extent[1] = (D_INTERNAL_TABLE_EXTENT)new_cols;

    return D_TABLE_STATUS_OK;
}

/*
d_table_erase_row
  Removes the row at _at, shifting the rows after it one position earlier.
Removal never needs storage, so it takes no strategy and cannot fail
mechanically.

Parameter(s):
  _table: the table to erase from.
  _at:    the row to remove.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the row was removed, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _table was null, or
  - D_TABLE_STATUS_DOMAIN, if _at was not a valid row index.
*/
enum d_table_status
d_table_erase_row
(
    struct d_table* _table,
    size_t          _at
)
{
    size_t         stride;
    unsigned char* seat;

    if (!_table)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // formal: rows and columns name coordinates 0 and 1, so this operation is
    // defined on the row-and-column table only.  A rank-k structural surface is
    // a further generalisation, not a reinterpretation of this one.
    if (_table->rank != D_TABLE_RANK_TABLE)
    {
        return D_TABLE_STATUS_RANK;
    }

    // formal: there is no such row to remove
    if (_at >= (size_t)_table->extent[0])
    {
        return D_TABLE_STATUS_DOMAIN;
    }

    stride = d_table_row_bytes(_table);
    seat   = _table->cells + ((size_t)_at * stride);

    // close the gap by shifting the tail one row earlier
    if (_at + 1 < (size_t)_table->extent[0])
    {
        memmove((void*)seat,
                (const void*)(seat + stride),
                ((size_t)_table->extent[0] - _at - 1) * stride);
    }

    _table->extent[0] = _table->extent[0] - 1;

    return D_TABLE_STATUS_OK;
}

/*
d_table_fill
  Overwrites every cell with _value. Element mutation throughout: I_T is left
exactly as it was.

Parameter(s):
  _table: the table to overwrite.
  _value: the cell to replicate; null means zeroed cells.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the cells were written, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _table was null.
*/
enum d_table_status
d_table_fill
(
    struct d_table* _table,
    const void*     _value
)
{
    if (!_table)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    d_table_internal_fill_cells(_table,
                                _table->cells,
                                d_table_size(_table),
                                _value);

    return D_TABLE_STATUS_OK;
}

/*
d_table_insert_column
  Inserts a column of _height cells at index _at, widening every row. Because
the row stride changes, every row moves; the rows are walked from the last
backwards so that a row's destination never lands on a row not yet moved.

Parameter(s):
  _table:  the table to widen.
  _at:     the index the new column takes; must not exceed the column count.
  _column: the cells of the new column, one per row; null means zeroed cells.
  _height: the number of cells in _column.
  _alloc:  the caller's storage strategy.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the column was inserted, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _table was null, or
  - D_TABLE_STATUS_DOMAIN, if _at exceeded the column count, or
  - D_TABLE_STATUS_SHAPE, if _height disagreed with the row count, or
  - D_TABLE_STATUS_CAPACITY, D_TABLE_STATUS_NO_MEMORY, or
    D_TABLE_STATUS_OVERFLOW, per
    d_table_internal_ensure_bytes.
*/
enum d_table_status
d_table_insert_column
(
    struct d_table*             _table,
    size_t                      _at,
    const void*                 _column,
    size_t                      _height,
    const struct d_table_alloc* _alloc
)
{
    enum d_table_status     status;
    size_t                  needed;
    size_t                  width;
    size_t                  old_stride;
    size_t                  new_stride;
    size_t                  new_cols;
    size_t                  r;
    unsigned char*          source;
    unsigned char*          target;
    const unsigned char*    supplied;

    if (!_table)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // formal: rows and columns name coordinates 0 and 1, so this operation is
    // defined on the row-and-column table only.  A rank-k structural surface is
    // a further generalisation, not a reinterpretation of this one.
    if (_table->rank != D_TABLE_RANK_TABLE)
    {
        return D_TABLE_STATUS_RANK;
    }

    // formal: an insertion point past the last column is outside the domain
    if (_at > (size_t)_table->extent[1])
    {
        return D_TABLE_STATUS_DOMAIN;
    }

    // formal: a column must reach every row for the domain to stay a box
    if (_height != (size_t)_table->extent[0])
    {
        return D_TABLE_STATUS_SHAPE;
    }

    new_cols = (size_t)_table->extent[1] + 1;
    needed   = 0;

    // mechanical: a width no extent can hold cannot be stored
    if (!D_TABLE_EXTENT_FITS(new_cols))
    {
        return D_TABLE_STATUS_OVERFLOW;
    }

    if (!d_table_internal_bytes_for((size_t)_table->extent[0],
                                    new_cols,
                                    (size_t)_table->cell_size,
                                    &needed))
    {
        return D_TABLE_STATUS_OVERFLOW;
    }

    status = d_table_internal_ensure_bytes(_table, needed, _alloc);

    if (status != D_TABLE_STATUS_OK)
    {
        return status;
    }

    width      = (size_t)_table->cell_size;
    old_stride = (size_t)_table->extent[1] * width;
    new_stride = new_cols * width;
    supplied   = (const unsigned char*)_column;

    // walk the rows backwards: a row's new seat is always at or beyond its old
    // one, and every row above it has already vacated
    r = _table->extent[0];

    while (r != 0)
    {
        --r;

        source = _table->cells + ((size_t)r * old_stride);
        target = _table->cells + ((size_t)r * new_stride);

        // the tail moves first, being the part that lands highest
        if (_at < (size_t)_table->extent[1])
        {
            memmove((void*)(target + (((size_t)_at + 1) * width)),
                    (const void*)(source + ((size_t)_at * width)),
                    ((size_t)_table->extent[1] - _at) * width);
        }

        // then the head, whose destination the tail has vacated
        if (_at != 0)
        {
            memmove((void*)target,
                    (const void*)source,
                    (size_t)_at * width);
        }

        // then the new cell in the gap the two left
        if (supplied)
        {
            memcpy((void*)(target + ((size_t)_at * width)),
                   (const void*)(supplied + ((size_t)r * width)),
                   width);
        }
        else
        {
            memset((void*)(target + ((size_t)_at * width)), 0, width);
        }
    }

    _table->extent[1] = (D_INTERNAL_TABLE_EXTENT)new_cols;

    // capacity is in bytes and means the same thing at the new width, so there
    // is nothing here to recompute -- which is the point of storing it in bytes
    return D_TABLE_STATUS_OK;
}

/*
d_table_insert_row
  Inserts a row of _width cells at index _at, shifting the rows at and after it
one position later. Structural mutation: it alters a bound function, hence the
domain I_T. Rectangularity is preserved or the insertion is refused -- a row of
the wrong width is a shape error, never a truncation.

  An empty table adopts _width as its column count, which is how a table built
row by row acquires a shape; a non-empty table requires the widths to agree.

Parameter(s):
  _table: the table to insert into.
  _at:    the index the new row takes; must not exceed the row count.
  _row:   the cells of the new row; null means a zeroed row.
  _width: the number of cells in _row.
  _alloc: the caller's storage strategy.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the row was inserted, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _table was null, or
  - D_TABLE_STATUS_DOMAIN, if _at exceeded the row count, or
  - D_TABLE_STATUS_SHAPE, if _width disagreed with the table's width, or
  - D_TABLE_STATUS_CAPACITY, D_TABLE_STATUS_NO_MEMORY, or
    D_TABLE_STATUS_OVERFLOW, per
    d_table_internal_ensure_bytes.
*/
enum d_table_status
d_table_insert_row
(
    struct d_table*             _table,
    size_t                      _at,
    const void*                 _row,
    size_t                      _width,
    const struct d_table_alloc* _alloc
)
{
    enum d_table_status status;
    size_t              needed;
    size_t              stride;
    unsigned char*      seat;

    if (!_table)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // formal: rows and columns name coordinates 0 and 1, so this operation is
    // defined on the row-and-column table only.  A rank-k structural surface is
    // a further generalisation, not a reinterpretation of this one.
    if (_table->rank != D_TABLE_RANK_TABLE)
    {
        return D_TABLE_STATUS_RANK;
    }

    // formal: an insertion point past the end is outside the domain
    if (_at > (size_t)_table->extent[0])
    {
        return D_TABLE_STATUS_DOMAIN;
    }

    // mechanical: a width no extent can hold cannot be stored
    if (!D_TABLE_EXTENT_FITS(_width))
    {
        return D_TABLE_STATUS_OVERFLOW;
    }

    // an empty table adopts the first row's width as its own
    if ( (_table->extent[0] == 0) &&
         (_table->extent[1] == 0) )
    {
        _table->extent[1] = (D_INTERNAL_TABLE_EXTENT)_width;
    }

    // formal: a differing width would leave the domain non-rectangular
    if (_width != (size_t)_table->extent[1])
    {
        return D_TABLE_STATUS_SHAPE;
    }

    needed = 0;

    if (!d_table_internal_bytes_for((size_t)_table->extent[0] + 1,
                                    (size_t)_table->extent[1],
                                    (size_t)_table->cell_size,
                                    &needed))
    {
        return D_TABLE_STATUS_OVERFLOW;
    }

    status = d_table_internal_ensure_bytes(_table, needed, _alloc);

    if (status != D_TABLE_STATUS_OK)
    {
        return status;
    }

    stride = d_table_row_bytes(_table);
    seat   = _table->cells + ((size_t)_at * stride);

    // open a gap by shifting the tail one row later
    if (_at < (size_t)_table->extent[0])
    {
        memmove((void*)(seat + stride),
                (const void*)seat,
                ((size_t)_table->extent[0] - _at) * stride);
    }

    // seat the new row, zeroed when no cells were supplied
    if (_row)
    {
        memcpy((void*)seat, _row, stride);
    }
    else if (stride != 0)
    {
        memset((void*)seat, 0, stride);
    }

    _table->extent[0] = _table->extent[0] + 1;

    return D_TABLE_STATUS_OK;
}

/*
d_table_pop_row
  Removes the last row.

Parameter(s):
  _table: the table to shorten.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if a row was removed, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _table was null, or
  - D_TABLE_STATUS_DOMAIN, if the table had no rows.
*/
enum d_table_status
d_table_pop_row
(
    struct d_table* _table
)
{
    if (!_table)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    if (_table->extent[0] == 0)
    {
        return D_TABLE_STATUS_DOMAIN;
    }

    return d_table_erase_row(_table, _table->extent[0] - 1);
}

/*
d_table_push_row
  Appends a row -- d_table_insert_row at the end.

Parameter(s):
  _table: the table to append to.
  _row:   the cells of the new row; null means a zeroed row.
  _width: the number of cells in _row.
  _alloc: the caller's storage strategy.
Return:
  A d_table_status value, per d_table_insert_row.
*/
enum d_table_status
d_table_push_row
(
    struct d_table*             _table,
    const void*                 _row,
    size_t                      _width,
    const struct d_table_alloc* _alloc
)
{
    if (!_table)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    return d_table_insert_row(_table, _table->extent[0], _row, _width, _alloc);
}

/*
d_table_release
  Returns the cell buffer to the strategy that supplied it and leaves the table
empty but still usable -- its width and cell size survive, its buffer does not.
A table over caller-owned memory is released by passing a strategy with no
release function, which detaches the buffer without freeing it.

Parameter(s):
  _table: the table to release; may be null.
  _alloc: the strategy that supplied the buffer; may be null.
Return:
  none.
*/
void
d_table_release
(
    struct d_table*             _table,
    const struct d_table_alloc* _alloc
)
{
    if (!_table)
    {
        return;
    }

    // hand the bytes back only to the strategy that issued them
    if ( (_alloc)         &&
         (_alloc->release) &&
         (_table->cells) )
    {
        _alloc->release(_alloc->context,
                        (void*)_table->cells,
                        d_table_capacity_bytes(_table));
    }

    _table->cells    = NULL;
    _table->extent[0]     = 0;
    _table->capacity = 0;

    return;
}

/*
d_table_reserve_rows
  Ensures the buffer can hold _rows rows at the current width without further
growth. The domain I_T is untouched: reserving is not a structural change, it
is the removal of a reason for one to fail.

Parameter(s):
  _table: the table whose buffer may grow.
  _rows:  the row count to make room for.
  _alloc: the caller's storage strategy.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the room exists or was made, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _table was null, or
  - D_TABLE_STATUS_OVERFLOW, if the byte count would overflow, or
  - D_TABLE_STATUS_CAPACITY or D_TABLE_STATUS_NO_MEMORY, per
    d_table_internal_ensure_bytes.
*/
enum d_table_status
d_table_reserve_rows
(
    struct d_table*             _table,
    size_t                      _rows,
    const struct d_table_alloc* _alloc
)
{
    size_t needed;

    if (!_table)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // formal: rows and columns name coordinates 0 and 1, so this operation is
    // defined on the row-and-column table only.  A rank-k structural surface is
    // a further generalisation, not a reinterpretation of this one.
    if (_table->rank != D_TABLE_RANK_TABLE)
    {
        return D_TABLE_STATUS_RANK;
    }

    needed = 0;

    // reject a request whose byte count is not representable
    if ( (!D_TABLE_EXTENT_FITS(_rows)) ||
         (!d_table_internal_bytes_for(_rows,
                                      (size_t)_table->extent[1],
                                      (size_t)_table->cell_size,
                                      &needed)) )
    {
        return D_TABLE_STATUS_OVERFLOW;
    }

    return d_table_internal_ensure_bytes(_table, needed, _alloc);
}

/*
d_table_resize
  Reshapes the table to _rows x _cols, preserving the cells the two domains
share and filling the rest from _fill. The direction of the walk follows the
direction of the width change, so no cell is overwritten before it has moved.

Parameter(s):
  _table: the table to reshape.
  _rows:  the new row extent.
  _cols:  the new column extent.
  _fill:  the cell new positions take; null means zeroed cells.
  _alloc: the caller's storage strategy.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the table was reshaped, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _table was null, or
  - D_TABLE_STATUS_CAPACITY, D_TABLE_STATUS_NO_MEMORY, or
    D_TABLE_STATUS_OVERFLOW, per
    d_table_internal_ensure_bytes.
*/
enum d_table_status
d_table_resize
(
    struct d_table*             _table,
    size_t                      _rows,
    size_t                      _cols,
    const void*                 _fill,
    const struct d_table_alloc* _alloc
)
{
    enum d_table_status     status;
    size_t                  needed;
    size_t                  width;
    size_t                  old_stride;
    size_t                  new_stride;
    size_t                  kept;
    size_t                  common_rows;
    size_t                  common_cols;
    size_t                  r;
    unsigned char*          source;
    unsigned char*          target;

    if (!_table)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // formal: rows and columns name coordinates 0 and 1, so this operation is
    // defined on the row-and-column table only.  A rank-k structural surface is
    // a further generalisation, not a reinterpretation of this one.
    if (_table->rank != D_TABLE_RANK_TABLE)
    {
        return D_TABLE_STATUS_RANK;
    }

    needed = 0;

    // mechanical: a domain no extent pair can hold cannot be stored
    if ( (!D_TABLE_EXTENT_FITS(_rows)) ||
         (!D_TABLE_EXTENT_FITS(_cols)) )
    {
        return D_TABLE_STATUS_OVERFLOW;
    }

    if (!d_table_internal_bytes_for(_rows, _cols,
                                    (size_t)_table->cell_size, &needed))
    {
        return D_TABLE_STATUS_OVERFLOW;
    }

    status = d_table_internal_ensure_bytes(_table, needed, _alloc);

    if (status != D_TABLE_STATUS_OK)
    {
        return status;
    }

    width       = (size_t)_table->cell_size;
    old_stride  = (size_t)_table->extent[1] * width;
    new_stride  = _cols * width;
    common_rows = ((_rows < (size_t)_table->extent[0]) ? _rows
                                                 : (size_t)_table->extent[0]);
    common_cols = ((_cols < (size_t)_table->extent[1]) ? _cols
                                                 : (size_t)_table->extent[1]);
    kept        = common_cols * width;

    // a widening walk must run backwards, a narrowing one forwards
    if (_cols >= (size_t)_table->extent[1])
    {
        r = common_rows;

        while (r != 0)
        {
            --r;

            source = _table->cells + ((size_t)r * old_stride);
            target = _table->cells + ((size_t)r * new_stride);

            memmove((void*)target, (const void*)source, kept);

            d_table_internal_fill_cells(_table,
                                        target + kept,
                                        (_cols - common_cols),
                                        _fill);
        }
    }
    else
    {
        for (r = 0; r < common_rows; ++r)
        {
            source = _table->cells + ((size_t)r * old_stride);
            target = _table->cells + ((size_t)r * new_stride);

            memmove((void*)target, (const void*)source, kept);
        }
    }

    // rows the old domain did not reach are new throughout
    for (r = common_rows; r < _rows; ++r)
    {
        target = _table->cells + ((size_t)r * new_stride);

        d_table_internal_fill_cells(_table, target, _cols, _fill);
    }

    _table->extent[0] = (D_INTERNAL_TABLE_EXTENT)_rows;
    _table->extent[1] = (D_INTERNAL_TABLE_EXTENT)_cols;

    return D_TABLE_STATUS_OK;
}

/*
d_table_swap_rows
  Exchanges the cells of rows _a and _b. Allocation-free: with _scratch it is
three row moves, and without it a chunked exchange through a small automatic
buffer, so a bounded table can still permute its rows.

Parameter(s):
  _table:   the table whose rows exchange.
  _a:       one row index.
  _b:       the other row index.
  _scratch: a buffer of at least one row's bytes, or null.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the rows were exchanged, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _table was null, or
  - D_TABLE_STATUS_DOMAIN, if either index was not a valid row index.
*/
enum d_table_status
d_table_swap_rows
(
    struct d_table* _table,
    size_t          _a,
    size_t          _b,
    void*           _scratch
)
{
    unsigned char  chunk[64];
    size_t         stride;
    size_t         moved;
    size_t         step;
    unsigned char* left;
    unsigned char* right;

    if (!_table)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // formal: rows and columns name coordinates 0 and 1, so this operation is
    // defined on the row-and-column table only.  A rank-k structural surface is
    // a further generalisation, not a reinterpretation of this one.
    if (_table->rank != D_TABLE_RANK_TABLE)
    {
        return D_TABLE_STATUS_RANK;
    }

    // formal: a row that is not in the domain cannot take part
    if ( (_a >= (size_t)_table->extent[0]) ||
         (_b >= (size_t)_table->extent[0]) )
    {
        return D_TABLE_STATUS_DOMAIN;
    }

    // a row exchanged with itself is already done
    if (_a == _b)
    {
        return D_TABLE_STATUS_OK;
    }

    stride = d_table_row_bytes(_table);
    left   = (unsigned char*)d_table_row(_table, _a);
    right  = (unsigned char*)d_table_row(_table, _b);

    // the fast path: one whole-row three-way move through the caller's buffer
    if (_scratch)
    {
        memcpy(_scratch, (const void*)left, stride);
        memcpy((void*)left, (const void*)right, stride);
        memcpy((void*)right, (const void*)_scratch, stride);

        return D_TABLE_STATUS_OK;
    }

    // the allocation-free path: the same exchange, one chunk at a time
    for (moved = 0; moved < stride; moved += step)
    {
        step = (stride - moved);

        if (step > sizeof(chunk))
        {
            step = sizeof(chunk);
        }

        memcpy((void*)chunk, (const void*)(left + moved), step);
        memcpy((void*)(left + moved), (const void*)(right + moved), step);
        memcpy((void*)(right + moved), (const void*)chunk, step);
    }

    return D_TABLE_STATUS_OK;
}


/*
d_table_over
  a rank-k table laid over caller-owned memory. _cells must hold at least
  _capacity_bytes bytes; the table starts with the domain _extents. No
  allocation, no ownership: the constructor a static buffer, a stack array, or a
  C++ container's own storage uses.

Parameter(s):
  _cells:          the caller's cell buffer; may be NULL for an empty table.
  _extents:        array of `_rank` extents, one per coordinate.
  _rank:           the number of coordinates; must not exceed D_TABLE_MAX_RANK.
  _cell_size:      the size, in bytes, of a single cell.
  _capacity_bytes: the size, in bytes, of the buffer at `_cells`.
Return:
  A `d_table` viewing the caller's buffer. The table borrows the storage and
  never owns it.
*/
D_NODISCARD D_INLINE_DEF struct d_table
d_table_over
(
    void*         _cells,
    const size_t* _extents,
    uint32_t      _rank,
    size_t        _cell_size,
    size_t        _capacity_bytes
)
{
    struct d_table result;
    uint32_t       r;

    result.cells     = (unsigned char*)_cells;
    result.rank      = _rank;
    result.cell_size = (D_INTERNAL_TABLE_EXTENT)_cell_size;
    result.capacity  = (D_INTERNAL_TABLE_EXTENT)_capacity_bytes;

    for (r = 0; r < D_TABLE_MAX_RANK; ++r)
    {
        result.extent[r] = 0;
    }

    for (r = 0; (r < _rank) && (r < D_TABLE_MAX_RANK); ++r)
    {
        result.extent[r] = (D_INTERNAL_TABLE_EXTENT)_extents[r];
    }

    return result;
}

/*
d_table_over2
  the rank-2 d_table_over, taking a row capacity as the trio (rows, cols,
  row_capacity) the row-and-column table is usually built with.

Parameter(s):
  _cells:        the caller's cell buffer; may be NULL for an empty table.
  _rows:         the leading extent.
  _cols:         the trailing extent.
  _cell_size:    the size, in bytes, of a single cell.
  _row_capacity: the number of rows the buffer can hold.
Return:
  A rank-2 `d_table` viewing the caller's buffer.
*/
D_NODISCARD D_INLINE_DEF struct d_table
d_table_over2
(
    void*  _cells,
    size_t _rows,
    size_t _cols,
    size_t _cell_size,
    size_t _row_capacity
)
{
    size_t e[2];

    e[0] = _rows;
    e[1] = _cols;

    return d_table_over(_cells, e, D_TABLE_RANK_TABLE, _cell_size,
                        (_row_capacity * _cols * _cell_size));
}

/*
d_table_empty_of
  an empty rank-2 table of known width and cell size, owning nothing. Rows may
  be appended to it once an allocator is supplied, because the width is
  remembered rather than inferred from the (absent) cells.

Parameter(s):
  _cols:      the trailing extent the empty table is shaped to.
  _cell_size: the size, in bytes, of a single cell.
Return:
  A `d_table` with no cells but a settled column count, so a later fill need not
  restate the shape.
*/
D_NODISCARD D_INLINE_DEF struct d_table
d_table_empty_of
(
    size_t _cols,
    size_t _cell_size
)
{
    return d_table_over2(NULL, 0, _cols, _cell_size, 0);
}

/*
d_table_row_capacity
  the leading slices the buffer can hold AT THE CURRENT SHAPE -- a derived
  quantity, recomputed on demand rather than stored, so that a shape change
  cannot leave it describing a slice of a different size.

Parameter(s):
  _table: the table to measure.
Return:
  The number of leading slices the buffer can hold at the current shape, or 0
  when the table is zero-width.
*/
D_NODISCARD size_t
d_table_row_capacity
(
    const struct d_table* _table
)
{
    size_t stride;

    stride = d_table_row_bytes(_table);

    // a zero-width table admits no slice however many bytes it holds
    if (stride == 0)
    {
        return 0;
    }

    return ((size_t)_table->capacity / stride);
}

/*
d_table_holds_invariant
  whether the rectangularity invariant holds -- the cells in use fit the bytes
  the buffer holds, the rank is within this build's cap, and a buffer-less table
  carries no capacity. The predicate the debug knob checks and the conformance
  test asserts after every structural operation.

Parameter(s):
  _table: the table to check.
Return:
  Nonzero when the table's rank, extents and capacity are mutually consistent, 0
  when any of them contradicts the others.
*/
D_NODISCARD int
d_table_holds_invariant
(
    const struct d_table* _table
)
{
    // a rank beyond the cap cannot be addressed at all
    if (_table->rank > D_TABLE_MAX_RANK)
    {
        return 0;
    }

    // a buffer-less table may hold no byte
    if ( (_table->cells == NULL) &&
         (_table->capacity != 0) )
    {
        return 0;
    }

    // the cells in use must fit the bytes the buffer holds
    return (d_table_bytes(_table) <= (size_t)_table->capacity);
}

/*
d_table_get
  copy the cell at (_row, _column) into _out -- the byte-copy form of val_T, for
  a caller holding a cell of the right width but not its type.

Parameter(s):
  _table:  the table to read; must not be NULL.
  _row:    the leading coordinate.
  _column: the trailing coordinate.
  _out:    destination for the cell's bytes; must not be NULL.
Return:
  A `d_table_status`: success when the cell was copied out, or the status naming
  why it could not be.
*/
D_NODISCARD enum d_table_status
d_table_get
(
    const struct d_table* _table,
    size_t                _row,
    size_t                _column,
    void*                 _out
)
{
    const void* cell;

    // mechanical: a null argument is a fact about a machine, not about I_T
    if ( (!_table) ||
         (!_out) )
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    cell = d_table_at_const(_table, _row, _column);

    // formal: the index is not in I_T
    if (!cell)
    {
        return D_TABLE_STATUS_DOMAIN;
    }

    memcpy(_out, cell, (size_t)_table->cell_size);

    return D_TABLE_STATUS_OK;
}

/*
d_table_set
  overwrite the cell at (_row, _column) from _value -- element mutation, which
  leaves I_T fixed.

Parameter(s):
  _table:  the table to write; must not be NULL.
  _row:    the leading coordinate.
  _column: the trailing coordinate.
  _value:  source bytes for the cell; must not be NULL.
Return:
  A `d_table_status`: success when the cell was written, or the status naming
  why it could not be.
*/
D_NODISCARD enum d_table_status
d_table_set
(
    struct d_table* _table,
    size_t          _row,
    size_t          _column,
    const void*     _value
)
{
    void* cell;

    // mechanical: a null argument is a fact about a machine, not about I_T
    if ( (!_table) ||
         (!_value) )
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    cell = d_table_at(_table, _row, _column);

    // formal: the index is not in I_T
    if (!cell)
    {
        return D_TABLE_STATUS_DOMAIN;
    }

    memcpy(cell, _value, (size_t)_table->cell_size);

    return D_TABLE_STATUS_OK;
}

/*
d_table_alloc_can_grow
  whether a strategy can supply more memory.

Parameter(s):
  _alloc: the allocation strategy to interrogate; may be NULL.
Return:
  Nonzero when the strategy can reallocate, 0 when it is absent or fixed-size.
*/
D_NODISCARD D_INLINE_DEF int
d_table_alloc_can_grow
(
    const struct d_table_alloc* _alloc
)
{
    return ( (_alloc != NULL) &&
             (_alloc->reallocate != NULL) );
}
