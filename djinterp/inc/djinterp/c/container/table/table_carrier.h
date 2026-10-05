/*******************************************************************************
* djinterp [c]                                                   table_carrier.h
*
*   The CELL-HOMOGENEOUS carrier: one cell type serves every cell, so the
* cells sit contiguously in the atomic order and the type is carried as a
* single byte width.  Also the caller-chosen storage strategy, and the
* structural mutation that changes I_T.
*
*   PORTABILITY:
*   C99 / C++11, the framework floors.
*
*
* path:      /inc/djinterp/c/container/table/table_carrier.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.02
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_TABLE_TABLE_CARRIER_H
#define DJINTERP_C_CONTAINER_TABLE_TABLE_CARRIER_H 1

// djinterp
#include "./table_domain.h"

D_EXTERN_C_BEGIN


// d_table
//   struct: the cell-homogeneous rank-2 table -- the tex's T_ and I_T in their
// lowered form. One cell type serves every cell, so the cell type is carried
// as a single byte width and the cells sit contiguously in the atomic
// (row-major, lexicographic) order. The rectangularity invariant is the
// module's one structural claim: the
// cells in use are exactly rows * cols, and every operation that changes the
// domain restores it before returning. The struct owns nothing -- ownership is
// the caller's, or the allocator's -- so copying it copies a view, and
// d_table_release is the only thing that frees.
struct d_table
{
    unsigned char*          cells;      // row-major, |T| * cell_size bytes
    D_INTERNAL_TABLE_EXTENT extent[D_TABLE_MAX_RANK];   // the k bounds
    D_INTERNAL_TABLE_EXTENT cell_size;  // sizeof(tau); ONE size for all cells
    D_INTERNAL_TABLE_EXTENT capacity;   // BYTES the buffer holds
    uint32_t                rank;       // k
};

// d_table_alloc
//   struct: the caller's storage strategy. Structural mutation needs memory;
// this module refuses to choose where it comes from, so every growing
// operation takes one of these and there is no default anywhere in the core. A
// table
// over a fixed buffer passes a zeroed d_table_alloc and gets
// D_TABLE_STATUS_CAPACITY instead of a hidden malloc -- which is what BOUNDED
// means, made mechanical.
struct d_table_alloc
{
    void* (*reallocate)(void*  _context,    // a null _block means allocate
                        void*  _block,
                        size_t _old_size,
                        size_t _new_size);
    void  (*release)(void*  _context,       // may be null
                     void*  _block,
                     size_t _size);
    void*   context;                        // passed through untouched
};

// III.  The carrier   (the CELL-HOMOGENEOUS table)

// d_table_domain_of
//   function: the domain I_T of a table -- the projection that lets every
// domain query above apply to a table without restating it.
D_NODISCARD D_INLINE struct d_table_domain
d_table_domain_of(
    const struct d_table* _table
)
{
    struct d_table_domain result;
    uint32_t              r;

    result.rank = _table->rank;

    for (r = 0; r < D_TABLE_MAX_RANK; ++r)
    {
        result.extent[r] = _table->extent[r];
    }

    return result;
}

// d_table_rank
//   function: k, the number of coordinates the table is addressed by.
D_NODISCARD D_INLINE uint32_t
d_table_rank(
    const struct d_table* _table
)
{
    return _table->rank;
}

// d_table_over
//   function: a rank-k table laid over caller-owned memory. _cells must hold
// at least _capacity_bytes bytes; the table starts with the domain _extents.
// No allocation, no ownership: the constructor a static buffer, a stack array,
// or a C++ container's own storage uses.
struct d_table d_table_over(void*         _cells,
                            const size_t* _extents,
                            uint32_t      _rank,
                            size_t        _cell_size,
                            size_t        _capacity_bytes);

// d_table_over2
//   function: the rank-2 d_table_over, taking a row capacity as the trio
// (rows, cols, row_capacity) the row-and-column table is usually built with.
struct d_table d_table_over2(void*  _cells,
                             size_t _rows,
                             size_t _cols,
                             size_t _cell_size,
                             size_t _row_capacity);

// d_table_empty_of
//   function: an empty rank-2 table of known width and cell size, owning
// nothing. Rows may be appended to it once an allocator is supplied, because
// the width is remembered rather than inferred from the (absent) cells.
struct d_table d_table_empty_of(size_t _cols,
                                size_t _cell_size);

// d_table_size
//   function: |T|, the number of atomic cells.
D_NODISCARD D_INLINE size_t
d_table_size(
    const struct d_table* _table
)
{
    return d_table_domain_size(d_table_domain_of(_table));
}

// d_table_bytes
//   function: the number of cell bytes in use -- the runtime sizeof of the
// table's contents, |T| * cell_size.  (The struct's own sizeof is a
// compile-time
// constant and says nothing about the cells.)
D_NODISCARD D_INLINE size_t
d_table_bytes(
    const struct d_table* _table
)
{
    return (d_table_size(_table) * (size_t)_table->cell_size);
}

// d_table_capacity_bytes
//   function: the number of cell bytes the buffer holds, in use or not.
D_NODISCARD D_INLINE size_t
d_table_capacity_bytes(
    const struct d_table* _table
)
{
    return (size_t)_table->capacity;
}

// d_table_row_bytes
//   function: the byte length of one leading slice -- the product of every
// extent but the first, times the cell size. For a rank-2 table, one row.
D_NODISCARD D_INLINE size_t
d_table_row_bytes(
    const struct d_table* _table
)
{
    return (d_table_domain_slice_extent(d_table_domain_of(_table)) *
            (size_t)_table->cell_size);
}

// d_table_row_capacity
//   function: the leading slices the buffer can hold AT THE CURRENT SHAPE -- a
// derived quantity, recomputed on demand rather than stored, so that a shape
// change cannot leave it describing a slice of a different size.
size_t d_table_row_capacity(const struct d_table* _table);

// d_table_is_empty
//   function: whether the table holds no atomic cell.
D_NODISCARD D_INLINE int
d_table_is_empty(
    const struct d_table* _table
)
{
    return d_table_domain_empty(d_table_domain_of(_table));
}

// d_table_contains
//   function: whether _index is a valid index of this table.
D_NODISCARD D_INLINE int
d_table_contains(
    const struct d_table* _table,
    struct d_table_index  _index
)
{
    return d_table_domain_contains(d_table_domain_of(_table), _index);
}

// d_table_contains2
//   function: the rank-2 d_table_contains, taking loose coordinates.
D_NODISCARD D_INLINE int
d_table_contains2(
    const struct d_table* _table,
    size_t                _row,
    size_t                _column
)
{
    return ( (_table->rank == D_TABLE_RANK_TABLE)      &&
             (_row    < (size_t)_table->extent[0])     &&
             (_column < (size_t)_table->extent[1]) );
}

// d_table_holds_invariant
//   function: whether the rectangularity invariant holds -- the cells in use
// fit the bytes the buffer holds, the rank is within this build's cap, and a
// buffer-less table carries no capacity. The predicate the debug knob checks
// and the conformance test asserts after every structural operation.
int d_table_holds_invariant(const struct d_table* _table);

// d_table_cell_at
//   function: a pointer to the cell at _index -- val_T applied to a valid
// index, in its lowered (byte) form. Unchecked; the caller has established
// validity.
D_NODISCARD D_INLINE void*
d_table_cell_at(
    struct d_table*      _table,
    struct d_table_index _index
)
{
    return (void*)(_table->cells +
                   (d_table_domain_linear(d_table_domain_of(_table), _index) *
                    (size_t)_table->cell_size));
}

// d_table_cell
//   function: the rank-2 d_table_cell_at. Unchecked.
D_NODISCARD D_INLINE void*
d_table_cell(
    struct d_table* _table,
    size_t          _row,
    size_t          _column
)
{
    return (void*)(_table->cells +
                   ((((_row * (size_t)_table->extent[1]) + _column)) *
                    (size_t)_table->cell_size));
}

// d_table_cell_const
//   function: the read-only d_table_cell.
D_NODISCARD D_INLINE const void*
d_table_cell_const(
    const struct d_table* _table,
    size_t                _row,
    size_t                _column
)
{
    return (const void*)(_table->cells +
                         ((((_row * (size_t)_table->extent[1]) + _column)) *
                          (size_t)_table->cell_size));
}

// d_table_cell_at_const
//   function: the read-only, rank-k d_table_cell_at.
D_NODISCARD D_INLINE const void*
d_table_cell_at_const(
    const struct d_table* _table,
    struct d_table_index  _index
)
{
    return (const void*)(_table->cells +
                         (d_table_domain_linear(d_table_domain_of(_table),
                                                _index) *
                          (size_t)_table->cell_size));
}

// d_table_at_index
//   function: the checked, rank-k cell pointer -- NULL when the index is
// outside I_T. NULL here reports a FORMAL condition (the index is undefined)
// and is the C spelling of the C++ face's std::out_of_range.
D_NODISCARD D_INLINE void*
d_table_at_index(
    struct d_table*      _table,
    struct d_table_index _index
)
{
    // an out-of-domain index is undefined, not blank
    if (!d_table_contains(_table, _index))
    {
        return NULL;
    }

    return d_table_cell_at(_table, _index);
}

// d_table_at
//   function: the checked rank-2 cell pointer, or NULL off the domain.
D_NODISCARD D_INLINE void*
d_table_at(
    struct d_table* _table,
    size_t          _row,
    size_t          _column
)
{
    if (!d_table_contains2(_table, _row, _column))
    {
        return NULL;
    }

    return d_table_cell(_table, _row, _column);
}

// d_table_at_const
//   function: the read-only d_table_at.
D_NODISCARD D_INLINE const void*
d_table_at_const(
    const struct d_table* _table,
    size_t                _row,
    size_t                _column
)
{
    if (!d_table_contains2(_table, _row, _column))
    {
        return NULL;
    }

    return d_table_cell_const(_table, _row, _column);
}

// d_table_row
//   function: a pointer to the first cell of the leading slice T[_leading].
// The slice's length is d_table_domain_slice_extent and its stride is one
// cell, so a slice is contiguous. Unchecked.
D_NODISCARD D_INLINE void*
d_table_row(
    struct d_table* _table,
    size_t          _leading
)
{
    return (void*)(_table->cells + (_leading * d_table_row_bytes(_table)));
}

// d_table_row_const
//   function: the read-only d_table_row.
D_NODISCARD D_INLINE const void*
d_table_row_const(
    const struct d_table* _table,
    size_t                _leading
)
{
    return (const void*)(_table->cells +
                         (_leading * d_table_row_bytes(_table)));
}

// d_table_get
//   function: copy the cell at (_row, _column) into _out -- the byte-copy form
// of val_T, for a caller holding a cell of the right width but not its type.
enum d_table_status d_table_get(const struct d_table* _table,
                                size_t                _row,
                                size_t                _column,
                                void*                 _out);

// d_table_set
//   function: overwrite the cell at (_row, _column) from _value -- element
// mutation, which leaves I_T fixed.
enum d_table_status d_table_set(struct d_table* _table,
                                size_t          _row,
                                size_t          _column,
                                const void*     _value);

// IV.   The storage strategy

// d_table_alloc_none
//   function: the storage strategy that allocates nothing. A table carrying it
// cannot grow past its buffer and reports D_TABLE_STATUS_CAPACITY when asked
// to.
D_NODISCARD D_INLINE struct d_table_alloc
d_table_alloc_none(void)
{
    struct d_table_alloc result;

    result.reallocate = NULL;
    result.release    = NULL;
    result.context    = NULL;

    return result;
}

// d_table_alloc_can_grow
//   function: whether a strategy can supply more memory.
int d_table_alloc_can_grow(const struct d_table_alloc* _alloc);

// VII. Structural mutation Structural mutation changes I_T itself -- appending
// a row alters a bound function, hence the domain -- so it needs storage and
// spare capacity. These are the operations that can fail, and they are defined
// out of line: they loop, they may allocate, and inlining them would buy
// nothing the domain algebra above has not already bought. Every one preserves
// rectangularity or refuses; a row of the wrong width is a SHAPE error, not a
// truncation.
enum d_table_status d_table_reserve_rows(struct d_table*             _table,
                                         size_t                      _rows,
                                         const struct d_table_alloc* _alloc);
enum d_table_status d_table_push_row(struct d_table*             _table,
                                     const void*                 _row,
                                     size_t                      _width,
                                     const struct d_table_alloc* _alloc);
enum d_table_status d_table_insert_row(struct d_table*             _table,
                                       size_t                      _at,
                                       const void*                 _row,
                                       size_t                      _width,
                                       const struct d_table_alloc* _alloc);
enum d_table_status d_table_erase_row(struct d_table* _table,
                                      size_t          _at);
enum d_table_status d_table_pop_row(struct d_table* _table);
enum d_table_status d_table_insert_column(struct d_table*             _table,
                                          size_t                      _at,
                                          const void*                 _column,
                                          size_t                      _height,
                                          const struct d_table_alloc* _alloc);
enum d_table_status d_table_erase_column(struct d_table* _table,
                                         size_t          _at);
enum d_table_status d_table_resize(struct d_table*             _table,
                                   size_t                      _rows,
                                   size_t                      _cols,
                                   const void*                 _fill,
                                   const struct d_table_alloc* _alloc);
enum d_table_status d_table_fill(struct d_table* _table,
                                 const void*     _value);
enum d_table_status d_table_swap_rows(struct d_table* _table,
                                      size_t          _a,
                                      size_t          _b,
                                      void*           _scratch);
void                d_table_clear(struct d_table* _table);
void                d_table_release(struct d_table*             _table,
                                    const struct d_table_alloc* _alloc);


// Comparison
int d_table_bytes_equal(const struct d_table* _a,
                        const struct d_table* _b);


// layout assertions -- drift becomes a compile error rather than a wire-format
// bug, and both dialects compile them
D_STATIC_ASSERT(offsetof(struct d_table, cells) == 0,
                "d_table field drift: cells must lead");
D_STATIC_ASSERT(offsetof(struct d_table, extent) == sizeof(void*),
                "d_table field drift: the extents must follow cells");

D_EXTERN_C_END


#endif  // DJINTERP_C_CONTAINER_TABLE_TABLE_CARRIER_H
