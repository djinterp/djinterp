/*******************************************************************************
* djinterp [c]                                                    table_layout.c
*
*   The out-of-line half of table_layout.h.  Every definition here is D_INLINE while its
* declaration in the header is not, so C11 6.7.4p7 makes each an EXTERNAL
* definition: one symbol, linkable from both languages, and free to be inlined
* within this translation unit.
*
*
* path:      /src/djinterp/c/container/table/table_layout.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.29
*******************************************************************************/

#include "../../../../../inc/djinterp/c/container/table/table_layout.h"


/*
d_table_index_fibre
  Enumerates pi^-1(_coarse) -- the refined positions lying over one coarse
position under the subdivision _factors. This is the set an ATOMIC SPLIT's
subcells are supported on, and its size is the product of the factors.

Parameter(s):
  _coarse:       the position being split.
  _factors:      the per-coordinate subdivision factors.
  _out:          receives the fibre; may be null to measure only.
  _out_capacity: how many indices _out can hold.
Return:
  The size of the fibre, whether or not it was written. A caller passing a null
_out, or too small a capacity, receives the count and writes nothing.
*/
size_t
d_table_index_fibre
(
    struct d_table_index  _coarse,
    const size_t*         _factors,
    struct d_table_index* _out,
    size_t                _out_capacity
)
{
    size_t   total;
    size_t   n;
    uint32_t r;

    total = 1;

    for (r = 0; r < _coarse.rank; ++r)
    {
        total = total * _factors[r];
    }

    // measure-only, or too small to fill: report the size and write nothing
    if ( (!_out) ||
         (_out_capacity < total) )
    {
        return total;
    }

    // the fibre in its own lexicographic order: the odometer over the factors
    for (n = 0; n < total; ++n)
    {
        size_t rest = n;

        _out[n] = d_table_index_zero(_coarse.rank);

        r = _coarse.rank;

        while (r != 0)
        {
            --r;

            _out[n].coord[r] = (D_INTERNAL_TABLE_EXTENT)
                (((size_t)_coarse.coord[r] * _factors[r]) +
                 (rest % _factors[r]));
            rest = rest / _factors[r];
        }
    }

    return total;
}
/*
d_table_layout_merge
  Adds _region to the cover as a merged cell -- the move a split inverts. The
region must lie in I_T and must not meet any cell already declared, since the
cells of a cover partition the atomic domain.

Parameter(s):
  _merges:   the caller's merge array, appended to.
  _count:    the live merge count, updated on success.
  _capacity: the array's capacity in regions.
  _region:   the region to merge.
  _domain:   the atomic domain the cover lies over.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the cell was added, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if a required argument was null, or
  - D_TABLE_STATUS_SHAPE, if the region is a singleton (nothing to merge), or
  - D_TABLE_STATUS_DOMAIN, if it leaves I_T, or
  - D_TABLE_STATUS_COVER, if it meets a cell already declared, or
  - D_TABLE_STATUS_CAPACITY, if the array is full.
*/
enum d_table_status
d_table_layout_merge
(
    struct d_table_region* _merges,
    size_t*                _count,
    size_t                 _capacity,
    struct d_table_region  _region,
    struct d_table_domain  _domain
)
{
    size_t i;

    if ( (!_merges) ||
         (!_count) )
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // formal: a singleton is not a merge
    if (!d_table_region_is_merge(_region))
    {
        return D_TABLE_STATUS_SHAPE;
    }

    // formal: a layout cell is a region of ATOMIC positions
    if (!d_table_region_within(_region, _domain))
    {
        return D_TABLE_STATUS_DOMAIN;
    }

    // formal: every atomic position lies in exactly one cell
    for (i = 0; i < *_count; ++i)
    {
        if (d_table_regions_overlap(_merges[i], _region))
        {
            return D_TABLE_STATUS_COVER;
        }
    }

    if (*_count >= _capacity)
    {
        return D_TABLE_STATUS_CAPACITY;
    }

    _merges[*_count] = _region;
    *_count          = *_count + 1;

    return D_TABLE_STATUS_OK;
}

/*
d_table_layout_split
  Replaces the merge at _which by _pieces, which must partition it. The cover is
edited in place in the caller's array: the split cell is removed, and every
piece that is itself a merge is appended. Pieces that come out singleton are NOT
stored, because Gamma_0 already gives every uncovered position its own cell --
storing them would be a second spelling of the same cover.

  This is the OVERLAY move. Splitting a cell whose region is already a singleton
cannot partition it and is refused; that gesture needs d_table_domain_refine.

Parameter(s):
  _merges:      the caller's merge array, edited in place.
  _count:       the live merge count, updated on success.
  _capacity:    the array's capacity in regions.
  _which:       the index of the merge to split.
  _pieces:      the partition of that merge's region.
  _piece_count: how many pieces.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the cover was refined, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if a required argument was null, or
  - D_TABLE_STATUS_DOMAIN, if _which names no merge, or
  - D_TABLE_STATUS_CAPACITY, if the array cannot hold the result, or
  - whatever d_table_region_partitions refuses the partition with.
*/
enum d_table_status
d_table_layout_split
(
    struct d_table_region*       _merges,
    size_t*                      _count,
    size_t                       _capacity,
    size_t                       _which,
    const struct d_table_region* _pieces,
    size_t                       _piece_count
)
{
    enum d_table_status   status;
    struct d_table_region target;
    size_t                kept;
    size_t                i;

    if ( (!_merges) ||
         (!_count)  ||
         (!_pieces) )
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // formal: there is no such cell to split
    if (_which >= *_count)
    {
        return D_TABLE_STATUS_DOMAIN;
    }

    target = _merges[_which];

    // formal: a singleton cannot be partitioned -- that gesture refines I_T
    if (!d_table_region_is_merge(target))
    {
        return D_TABLE_STATUS_SHAPE;
    }

    status = d_table_region_partitions(_pieces, _piece_count, target);

    if (status != D_TABLE_STATUS_OK)
    {
        return status;
    }

    // only the pieces that are themselves merges need storing; Gamma_0 covers
    // the singletons already
    kept = 0;

    for (i = 0; i < _piece_count; ++i)
    {
        if (d_table_region_is_merge(_pieces[i]))
        {
            ++kept;
        }
    }

    if (((*_count - 1) + kept) > _capacity)
    {
        return D_TABLE_STATUS_CAPACITY;
    }

    // drop the split cell by closing over it
    for (i = _which; (i + 1) < *_count; ++i)
    {
        _merges[i] = _merges[i + 1];
    }

    *_count = *_count - 1;

    // then append the pieces worth keeping
    for (i = 0; i < _piece_count; ++i)
    {
        if (d_table_region_is_merge(_pieces[i]))
        {
            _merges[*_count] = _pieces[i];
            *_count          = *_count + 1;
        }
    }

    return D_TABLE_STATUS_OK;
}

/*
d_table_layout_validate
  Checks that the declared merges of a cover really do give a partition of the
atomic domain. Every position no merge covers is its own singleton cell, so the
partition holds exactly when each merge lies inside I_T and no two merges share
a position.

Parameter(s):
  _layout: the cover to check.
  _domain: the atomic domain the cover lies over.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the merges give a partition, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _layout was null, or
  - D_TABLE_STATUS_DOMAIN, if a merge left I_T or was empty, or
  - D_TABLE_STATUS_SHAPE, if two merges shared an atomic position.
*/
enum d_table_status
d_table_layout_validate
(
    const struct d_table_layout* _layout,
    struct d_table_domain        _domain
)
{
    size_t                  i;
    size_t                  j;

    if (!_layout)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // the trivial cover is a partition by construction
    if (_layout->count == 0)
    {
        return D_TABLE_STATUS_OK;
    }

    if (!_layout->merges)
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < (size_t)_layout->count; ++i)
    {
        // formal: a layout cell is a region of ATOMIC positions
        if (!d_table_region_within(_layout->merges[i], _domain))
        {
            return D_TABLE_STATUS_DOMAIN;
        }

        // formal: every atomic position lies in exactly one cell
        for (j = i + 1; j < (size_t)_layout->count; ++j)
        {
            if (d_table_regions_overlap(_layout->merges[i],
                                        _layout->merges[j]))
            {
                return D_TABLE_STATUS_SHAPE;
            }
        }
    }

    return D_TABLE_STATUS_OK;
}

/*
d_table_region_partitions
  Checks that _count pieces really do partition _whole: each piece lies inside
it, no two share a position, and together they account for every one of its
atomic positions. This is the precondition a split must satisfy, and checking
extents rather than walking positions keeps it O(s^2) in the piece count rather
than O(|R_C|).

Parameter(s):
  _pieces: the candidate pieces.
  _count:  how many.
  _whole:  the region they must partition.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the pieces partition _whole, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if _pieces was null with a non-zero count,
  - D_TABLE_STATUS_SHAPE, if fewer than two pieces were offered, or
  - D_TABLE_STATUS_RANK, if a piece has another rank, or
  - D_TABLE_STATUS_COVER, if a piece escapes _whole, two pieces overlap, or the
    extents do not sum to |R_C|.
*/
enum d_table_status
d_table_region_partitions
(
    const struct d_table_region* _pieces,
    size_t                       _count,
    struct d_table_region        _whole
)
{
    size_t   covered;
    size_t   i;
    size_t   j;
    uint32_t r;

    if ( (!_pieces) &&
         (_count != 0) )
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // formal: a partition into fewer than two pieces is not a split
    if (_count < 2)
    {
        return D_TABLE_STATUS_SHAPE;
    }

    covered = 0;

    for (i = 0; i < _count; ++i)
    {
        if (_pieces[i].rank != _whole.rank)
        {
            return D_TABLE_STATUS_RANK;
        }

        // every piece must lie inside the region it partitions
        for (r = 0; r < _whole.rank; ++r)
        {
            if (_pieces[i].span[r] == 0)
            {
                return D_TABLE_STATUS_COVER;
            }

            if ( ((size_t)_pieces[i].origin[r] <
                  (size_t)_whole.origin[r])                          ||
                 (((size_t)_pieces[i].origin[r] +
                   (size_t)_pieces[i].span[r]) >
                  ((size_t)_whole.origin[r] + (size_t)_whole.span[r])) )
            {
                return D_TABLE_STATUS_COVER;
            }
        }

        // no two pieces may share an atomic position
        for (j = i + 1; j < _count; ++j)
        {
            if (d_table_regions_overlap(_pieces[i], _pieces[j]))
            {
                return D_TABLE_STATUS_COVER;
            }
        }

        covered = covered + d_table_region_extent(_pieces[i]);
    }

    // disjoint pieces inside the whole account for it exactly when their
    // extents sum to its own
    if (covered != d_table_region_extent(_whole))
    {
        return D_TABLE_STATUS_COVER;
    }

    return D_TABLE_STATUS_OK;
}


/*
d_table_region_make
  the region spanning _spans from _origins, over _rank coordinates.

Parameter(s):
  _origins: array of `_rank` origin coordinates.
  _spans:   array of `_rank` spans, one per coordinate.
  _rank:    the number of coordinates.
Return:
  A `d_table_region` at those origins with those spans; coordinates beyond
  `_rank` are zeroed.
*/
D_NODISCARD D_INLINE_DEF struct d_table_region
d_table_region_make
(
    const size_t* _origins,
    const size_t* _spans,
    uint32_t      _rank
)
{
    struct d_table_region result;
    uint32_t              r;

    result.rank = _rank;

    for (r = 0; r < D_TABLE_MAX_RANK; ++r)
    {
        result.origin[r] = 0;
        result.span[r]   = 0;
    }

    for (r = 0; (r < _rank) && (r < D_TABLE_MAX_RANK); ++r)
    {
        result.origin[r] = (D_INTERNAL_TABLE_EXTENT)_origins[r];
        result.span[r]   = (D_INTERNAL_TABLE_EXTENT)_spans[r];
    }

    return result;
}

/*
d_table_region_make2
  the rank-2 region spanning _rows x _cols from (_row0, _col0).

Parameter(s):
  _row0: the leading origin.
  _col0: the trailing origin.
  _rows: the leading span.
  _cols: the trailing span.
Return:
  A rank-2 `d_table_region`.
*/
D_NODISCARD D_INLINE_DEF struct d_table_region
d_table_region_make2
(
    size_t _row0,
    size_t _col0,
    size_t _rows,
    size_t _cols
)
{
    size_t o[2];
    size_t s[2];

    o[0] = _row0;
    o[1] = _col0;
    s[0] = _rows;
    s[1] = _cols;

    return d_table_region_make(o, s, D_TABLE_RANK_TABLE);
}

/*
d_table_region_singleton
  the singleton region at _index -- the layout cell the trivial cover Gamma_0
  gives every atomic position.

Parameter(s):
  _index: the position the region covers.
Return:
  The region covering exactly that one position -- every span 1.
*/
D_NODISCARD D_INLINE_DEF struct d_table_region
d_table_region_singleton
(
    struct d_table_index _index
)
{
    struct d_table_region result;
    uint32_t              r;

    result.rank = _index.rank;

    for (r = 0; r < D_TABLE_MAX_RANK; ++r)
    {
        result.origin[r] = _index.coord[r];
        result.span[r]   = ((r < _index.rank) ? 1 : 0);
    }

    return result;
}

/*
d_table_region_is_merge
  whether the region spans more than one atomic position.

Parameter(s):
  _region: the region to test.
Return:
  Nonzero when the region covers more than one position, 0 when it is a
  singleton.
*/
D_NODISCARD int
d_table_region_is_merge
(
    struct d_table_region _region
)
{
    return (d_table_region_extent(_region) > 1);
}

/*
d_table_region_is_merged_along
  whether the region is merged along coordinate _r.

Parameter(s):
  _region: the region to test.
  _r:      the coordinate to test along.
Return:
  Nonzero when the region spans more than one position along `_r`.
*/
D_NODISCARD D_INLINE_DEF int
d_table_region_is_merged_along
(
    struct d_table_region _region,
    uint32_t              _r
)
{
    return (_region.span[_r] > 1);
}

/*
d_table_region_within
  whether the region lies wholly inside a domain. A cover's regions must, since
  a layout cell is a region of ATOMIC positions.

Parameter(s):
  _region: the region to test.
  _domain: the domain it must lie inside.
Return:
  Nonzero when the region lies wholly inside the domain, 0 when the ranks
  disagree or any coordinate runs past its extent.
*/
D_NODISCARD D_INLINE_DEF int
d_table_region_within
(
    struct d_table_region _region,
    struct d_table_domain _domain
)
{
    uint32_t r;

    if (_region.rank != _domain.rank)
    {
        return 0;
    }

    for (r = 0; r < _region.rank; ++r)
    {
        // a region is non-empty by construction; a zero span is not a region
        if (_region.span[r] == 0)
        {
            return 0;
        }

        if (((size_t)_region.origin[r] + (size_t)_region.span[r]) >
            (size_t)_domain.extent[r])
        {
            return 0;
        }
    }

    return 1;
}

/*
d_table_regions_overlap
  whether two regions share any atomic position. A cover's regions must not:
  every atomic position lies in exactly one cell. Two boxes overlap exactly when
  they overlap on EVERY coordinate.

Parameter(s):
  _a: the first region.
  _b: the second region.
Return:
  Nonzero when the two regions share at least one position, 0 when the ranks
  disagree or they are disjoint along any coordinate.
*/
D_NODISCARD D_INLINE_DEF int
d_table_regions_overlap
(
    struct d_table_region _a,
    struct d_table_region _b
)
{
    uint32_t r;

    if (_a.rank != _b.rank)
    {
        return 0;
    }

    for (r = 0; r < _a.rank; ++r)
    {
        if ( ((size_t)_a.origin[r] >=
              (size_t)_b.origin[r] + (size_t)_b.span[r]) ||
             ((size_t)_b.origin[r] >=
              (size_t)_a.origin[r] + (size_t)_a.span[r]) )
        {
            return 0;
        }
    }

    return 1;
}

/*
d_table_layout_over
  the cover given by _count declared merges at _merges.

Parameter(s):
  _merges: the caller's array of merged regions.
  _count:  the number of regions at `_merges`.
Return:
  A `d_table_layout` viewing that array. The layout borrows the storage and
  never owns it.
*/
D_NODISCARD D_INLINE_DEF struct d_table_layout
d_table_layout_over
(
    const struct d_table_region* _merges,
    size_t                       _count
)
{
    struct d_table_layout result;

    result.merges = _merges;
    result.count  = (D_INTERNAL_TABLE_EXTENT)_count;

    return result;
}

/*
d_table_layout_wears_no_merges
  whether every region is a singleton -- the Gamma_0 case.

Parameter(s):
  _layout: the layout to interrogate.
Return:
  Nonzero when the layout declares no merges, 0 when it declares any.
*/
D_NODISCARD D_INLINE_DEF int
d_table_layout_wears_no_merges
(
    const struct d_table_layout* _layout
)
{
    return (_layout->count == 0);
}

/*
d_table_layout_is_anchor
  whether the position names its own cell -- i in A_T. A covered non-anchor
  position remains a valid index and defers to the cell anchored elsewhere.

Parameter(s):
  _layout: the layout to consult.
  _index:  the position to test.
Return:
  Nonzero when the position names its own cell, 0 when it is covered by a merge
  anchored elsewhere.
*/
D_NODISCARD int
d_table_layout_is_anchor
(
    const struct d_table_layout* _layout,
    struct d_table_index         _index
)
{
    return d_table_index_equal(d_table_layout_anchor_of(_layout, _index),
                               _index);
}
