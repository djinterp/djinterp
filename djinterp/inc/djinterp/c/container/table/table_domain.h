/*******************************************************************************
* djinterp [c]                                                    table_domain.h
*
*   THE INDEX SPACE.  The multi-index i in N^k, the k-box I_T it ranges over,
* and the order both carry.  Index and domain are one concept and one file
* because the module's central theorem spans them: the lexicographic order on
* multi-indices and the row-major linear order on the box are the SAME order,
* and d_table_index_lex_compare must agree with d_table_domain_linear on every
* pair of valid indices.  Splitting them puts the two halves of that agreement
* in different files.
*
*   Also here: the refinement pair (I_T' and pi) an ATOMIC SPLIT needs, since
* a
* refinement is a statement about the index space and nothing else.
*
*   No storage.  Nothing in this header knows a table has cells.
*
*   PORTABILITY:
*   C99 / C++11, the framework floors.
*
*
* path:      /inc/djinterp/c/container/table/table_domain.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.02
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_TABLE_TABLE_DOMAIN_H
#define DJINTERP_C_CONTAINER_TABLE_TABLE_DOMAIN_H 1

// djinterp
#include "./table_common.h"


D_EXTERN_C_BEGIN

// d_table_index
//   struct: a multi-index i in N^k -- the coordinate tuple that addresses one
// atomic position. A table is addressed by coordinate: the address of a cell
// IS its multi-index, which is why a table needs no path apparatus. The
// coordinates sit INLINE, so an index is a value: returned, compared and
// serialized without an allocator. `rank` is how many of them are live.
struct d_table_index
{
    D_INTERNAL_TABLE_EXTENT coord[D_TABLE_MAX_RANK];
    uint32_t                rank;
};

// I.    The multi-index

// d_table_index_zero
//   function: the origin of a rank-_rank index space, every coordinate 0.
D_NODISCARD D_INLINE struct d_table_index
d_table_index_zero(
    uint32_t _rank
)
{
    struct d_table_index result;
    uint32_t             r;

    result.rank = _rank;

    for (r = 0; r < D_TABLE_MAX_RANK; ++r)
    {
        result.coord[r] = 0;
    }

    return result;
}

// d_table_index_make2
//   function: the rank-2 multi-index (_row, _column) -- the row-and-column
// case, which is common enough to name. Narrowing is lossless where the caller
// has established that both coordinates lie in a domain.
D_NODISCARD D_INLINE struct d_table_index
d_table_index_make2(
    size_t _row,
    size_t _column
)
{
    struct d_table_index result;

    result = d_table_index_zero(D_TABLE_RANK_TABLE);

    result.coord[0] = (D_INTERNAL_TABLE_EXTENT)_row;
    result.coord[1] = (D_INTERNAL_TABLE_EXTENT)_column;

    return result;
}

// d_table_index_of
//   function: the multi-index over _rank coordinates read from _coords.
struct d_table_index d_table_index_of(const size_t* _coords,
                                      uint32_t      _rank);

// d_table_index_at
//   function: coordinate _r of a multi-index, widened to the addressing width.
D_NODISCARD D_INLINE size_t
d_table_index_at(
    struct d_table_index _index,
    uint32_t             _r
)
{
    return (size_t)_index.coord[_r];
}

// d_table_index_storable
//   function: whether every coordinate fits a stored extent, and the rank fits
// this build. The guard before narrowing an index that has not been checked
// against a domain.
int d_table_index_storable(const size_t* _coords,
                           uint32_t      _rank);

// d_table_index_equal
//   function: whether two multi-indices denote the same position. Coordinate
// separation is automatic -- distinct coordinates are distinct labels -- so
// index identity is componentwise and needs no comparator. Indices of
// different rank are never equal.
D_NODISCARD D_INLINE int
d_table_index_equal(
    struct d_table_index _a,
    struct d_table_index _b
)
{
    uint32_t r;

    if (_a.rank != _b.rank)
    {
        return 0;
    }

    for (r = 0; r < _a.rank; ++r)
    {
        if (_a.coord[r] != _b.coord[r])
        {
            return 0;
        }
    }

    return 1;
}

// d_table_index_lex_compare
//   function: the lexicographic order on multi-indices -- the order a table
// carries, each coordinate bearing the order of N. The first coordinate
// dominates and later ones break its ties. Returns a negative value, zero, or
// a positive value as _a precedes, equals, or follows _b.
D_NODISCARD D_INLINE int
d_table_index_lex_compare(
    struct d_table_index _a,
    struct d_table_index _b
)
{
    uint32_t r;
    uint32_t common;

    common = ((_a.rank < _b.rank) ? _a.rank : _b.rank);

    for (r = 0; r < common; ++r)
    {
        if (_a.coord[r] != _b.coord[r])
        {
            return ((_a.coord[r] < _b.coord[r]) ? -1 : 1);
        }
    }

    // a proper prefix precedes its extensions
    if (_a.rank != _b.rank)
    {
        return ((_a.rank < _b.rank) ? -1 : 1);
    }

    return 0;
}

// d_table_domain
//   struct: the valid indices I_T of a rectangular rank-k table -- the k-box
//     I_T = {0..e_0-1} x ... x {0..e_{k-1}-1}, each coordinate's range fixed
// independently. It carries the k extents and nothing else: a rectangular
// domain IS its bounds, and an index is valid exactly when every coordinate
// passes its own range check. An empty domain is a legitimate value. A 0 x 3
// domain is a zero-row table that REMEMBERS its width, so rows may be appended
// to it; that is why the extents are not derived from the cell count.
struct d_table_domain
{
    D_INTERNAL_TABLE_EXTENT extent[D_TABLE_MAX_RANK];
    uint32_t                rank;
};

// II.   The domain I_T

// d_table_domain_make
//   function: the k-box domain over _rank extents read from _extents.
struct d_table_domain d_table_domain_make(const size_t* _extents,
                                          uint32_t      _rank);

// d_table_domain_make2
//   function: the rank-2 box of _rows rows and _cols columns.
struct d_table_domain d_table_domain_make2(size_t _rows,
                                           size_t _cols);

// d_table_domain_rank
//   function: k, the number of coordinates.
D_NODISCARD D_INLINE uint32_t
d_table_domain_rank(
    struct d_table_domain _domain
)
{
    return _domain.rank;
}

// d_table_domain_extent
//   function: the bound on coordinate _r, widened to the addressing width.
D_NODISCARD D_INLINE size_t
d_table_domain_extent(
    struct d_table_domain _domain,
    uint32_t              _r
)
{
    return (size_t)_domain.extent[_r];
}

// d_table_domain_rows
//   function: the first extent -- the row count of a rank-2 table.
D_NODISCARD D_INLINE size_t
d_table_domain_rows(
    struct d_table_domain _domain
)
{
    return (size_t)_domain.extent[0];
}

// d_table_domain_cols
//   function: the second extent -- the column count of a rank-2 table.
D_NODISCARD D_INLINE size_t
d_table_domain_cols(
    struct d_table_domain _domain
)
{
    return ((_domain.rank > 1) ? (size_t)_domain.extent[1] : 0);
}

// d_table_domain_size
//   function: |T| = |I_T|, the product of the extents.  A rank-0 domain holds
// exactly one position (the empty product), which is the scalar case.
D_NODISCARD D_INLINE size_t
d_table_domain_size(
    struct d_table_domain _domain
)
{
    size_t   total;
    uint32_t r;

    total = 1;

    for (r = 0; r < _domain.rank; ++r)
    {
        total = total * (size_t)_domain.extent[r];
    }

    return total;
}

// d_table_domain_empty
//   function: whether the domain holds no atomic position. A zero extent on
// ANY coordinate empties the box, so a 0 x 3 domain is empty and still has a
// width of 3.
D_NODISCARD D_INLINE int
d_table_domain_empty(
    struct d_table_domain _domain
)
{
    uint32_t r;

    for (r = 0; r < _domain.rank; ++r)
    {
        if (_domain.extent[r] == 0)
        {
            return 1;
        }
    }

    return 0;
}

// d_table_domain_stride
//   function: the atomic step between consecutive positions along coordinate
// _r -- the product of the extents BELOW it. The last coordinate has stride 1,
// which is what makes the linear order row-major.
D_NODISCARD D_INLINE size_t
d_table_domain_stride(
    struct d_table_domain _domain,
    uint32_t              _r
)
{
    size_t   step;
    uint32_t q;

    step = 1;

    for (q = _r + 1; q < _domain.rank; ++q)
    {
        step = step * (size_t)_domain.extent[q];
    }

    return step;
}

// d_table_domain_contains
//   function: whether _index is a valid index of I_T. For a box domain this is
// the conjunction of the per-coordinate range checks; an index that fails it
// is UNDEFINED, not blank. The bounds widen to the index width, never the
// reverse, so a coordinate beyond any extent answers false rather than
// wrapping.
D_NODISCARD D_INLINE int
d_table_domain_contains(
    struct d_table_domain _domain,
    struct d_table_index  _index
)
{
    uint32_t r;

    // an index of another rank addresses a different space
    if (_index.rank != _domain.rank)
    {
        return 0;
    }

    for (r = 0; r < _domain.rank; ++r)
    {
        if ((size_t)_index.coord[r] >= (size_t)_domain.extent[r])
        {
            return 0;
        }
    }

    return 1;
}

// d_table_domain_contains2
//   function: the rank-2 d_table_domain_contains, taking loose coordinates.
D_NODISCARD D_INLINE int
d_table_domain_contains2(
    struct d_table_domain _domain,
    size_t                _row,
    size_t                _column
)
{
    return ( (_domain.rank == D_TABLE_RANK_TABLE)          &&
             (_row    < (size_t)_domain.extent[0])         &&
             (_column < (size_t)_domain.extent[1]) );
}

// d_table_domain_linear
//   function: the position of _index in the atomic order -- the generalised
// row-major offset sum_r i_r * stride_r.  This is the order-isomorphism from
// the lexicographic order on I_T onto {0..|T|-1}: the SAME order the table
// carries, written as an integer, which is why cells may be stored contiguously
// without choosing an order the tex has not already fixed.
//   Unchecked: the caller has established validity with
// d_table_domain_contains.
D_NODISCARD D_INLINE size_t
d_table_domain_linear(
    struct d_table_domain _domain,
    struct d_table_index  _index
)
{
    size_t   offset;
    size_t   step;
    uint32_t r;

    offset = 0;
    step   = 1;

    // accumulate from the fastest-varying coordinate outwards, so the strides
    // are built once rather than recomputed per coordinate
    r = _domain.rank;

    while (r != 0)
    {
        --r;

        offset = offset + ((size_t)_index.coord[r] * step);
        step   = step * (size_t)_domain.extent[r];
    }

    return offset;
}

// d_table_domain_linear2
//   function: the rank-2 d_table_domain_linear -- row * cols + column.
D_NODISCARD D_INLINE size_t
d_table_domain_linear2(
    struct d_table_domain _domain,
    size_t                _row,
    size_t                _column
)
{
    return ((_row * (size_t)_domain.extent[1]) + _column);
}

// d_table_domain_index_of
//   function: the inverse of d_table_domain_linear -- the multi-index at atomic
// position _linear.  Unchecked; _linear must be below |T|.
D_NODISCARD D_INLINE struct d_table_index
d_table_domain_index_of(
    struct d_table_domain _domain,
    size_t                _linear
)
{
    struct d_table_index result;
    uint32_t             r;

    result = d_table_index_zero(_domain.rank);

    // peel the fastest-varying coordinate first, which is the last
    r = _domain.rank;

    while (r != 0)
    {
        --r;

        result.coord[r] =
            (D_INTERNAL_TABLE_EXTENT)(_linear % (size_t)_domain.extent[r]);
        _linear = _linear / (size_t)_domain.extent[r];
    }

    return result;
}

// d_table_domain_slice_extent
//   function: the length of the rank-(k-1) subtable T[p] obtained by fixing
// the leading coordinate -- bracketing a prefix leaves the suffix set, whose
// size is the product of the remaining extents.
D_NODISCARD D_INLINE size_t
d_table_domain_slice_extent(
    struct d_table_domain _domain
)
{
    size_t   total;
    uint32_t r;

    total = 1;

    for (r = 1; r < _domain.rank; ++r)
    {
        total = total * (size_t)_domain.extent[r];
    }

    return total;
}

// d_table_domain_slice_offset
//   function: the atomic position at which the subtable T[_leading] begins.
size_t d_table_domain_slice_offset(struct d_table_domain _domain,
                                   size_t                _leading);

// d_table_domain_projection_extent
//   function: the length of the projection along coordinate _r -- over a box
// domain every position reaches it, so it is that coordinate's extent.
size_t d_table_domain_projection_extent(struct d_table_domain _domain,
                                        uint32_t              _r);

// d_table_domain_equal
//   function: whether two domains admit exactly the same indices.
D_NODISCARD D_INLINE int
d_table_domain_equal(
    struct d_table_domain _a,
    struct d_table_domain _b
)
{
    uint32_t r;

    if (_a.rank != _b.rank)
    {
        return 0;
    }

    for (r = 0; r < _a.rank; ++r)
    {
        if (_a.extent[r] != _b.extent[r])
        {
            return 0;
        }
    }

    return 1;
}

// d_table_domain_refine
//   function: the finer domain I_T' obtained by subdividing each coordinate by
// its factor in _factors -- the index-space refinement an ATOMIC SPLIT needs.
// Every position of I_T becomes a box of prod_r factor_r positions in I_T'.
// The projection back is d_table_domain_project.
struct d_table_domain d_table_domain_refine(struct d_table_domain _domain,
                                            const size_t*         _factors);

// d_table_domain_project
//   function: pi(i') -- the coarse index a refined index lies over. With the
// refinement above this is coordinatewise integer division by the factor, so
// pi is total and every fibre pi^-1(i) is the box of subpositions of i.
struct d_table_index d_table_domain_project(struct d_table_index _refined,
                                            const size_t*        _factors);


// layout assertions -- drift becomes a compile error rather than a wire-format
// bug, and both dialects compile them
D_STATIC_ASSERT(offsetof(struct d_table_index, coord) == 0,
                "d_table_index field drift: the coordinates must lead");
D_STATIC_ASSERT(offsetof(struct d_table_index, rank) ==
                    (D_TABLE_MAX_RANK * sizeof(D_INTERNAL_TABLE_EXTENT)),
                "d_table_index field drift: rank must follow the coordinates");
D_STATIC_ASSERT(offsetof(struct d_table_domain, extent) == 0,
                "d_table_domain field drift: the extents must lead");
D_STATIC_ASSERT(offsetof(struct d_table_domain, rank) ==
                    (D_TABLE_MAX_RANK * sizeof(D_INTERNAL_TABLE_EXTENT)),
                "d_table_domain field drift: rank must follow the extents");
D_STATIC_ASSERT(sizeof(D_INTERNAL_TABLE_EXTENT) <= sizeof(size_t),
                "an extent must be storable in the addressing width");
D_STATIC_ASSERT(D_TABLE_MAX_RANK >= D_TABLE_RANK_TABLE,
                "the rank cap must admit the row-and-column table");

D_EXTERN_C_END


#endif  // DJINTERP_C_CONTAINER_TABLE_TABLE_DOMAIN_H
