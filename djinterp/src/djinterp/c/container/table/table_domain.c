/*******************************************************************************
* djinterp [c]                                                    table_domain.c
*
*   The out-of-line half of table_domain.h.  Every definition here is D_INLINE while its
* declaration in the header is not, so C11 6.7.4p7 makes each an EXTERNAL
* definition: one symbol, linkable from both languages, and free to be inlined
* within this translation unit.
*
*
* path:      /src/djinterp/c/container/table/table_domain.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.29
*******************************************************************************/

#include "../../../../../inc/djinterp/c/container/table/table_domain.h"


/*
d_table_domain_make
  the k-box domain over _rank extents read from _extents.

Parameter(s):
  _extents: array of `_rank` extents, one per coordinate.
  _rank:    the number of coordinates.
Return:
  A `d_table_domain` over the given extents; coordinates beyond `_rank` are
  zeroed.
*/
D_NODISCARD D_INLINE_DEF struct d_table_domain
d_table_domain_make
(
    const size_t* _extents,
    uint32_t      _rank
)
{
    struct d_table_domain result;
    uint32_t              r;

    result.rank = _rank;

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
d_table_domain_make2
  the rank-2 box of _rows rows and _cols columns.

Parameter(s):
  _rows: the leading extent.
  _cols: the trailing extent.
Return:
  A rank-2 `d_table_domain` over the two extents.
*/
D_NODISCARD D_INLINE_DEF struct d_table_domain
d_table_domain_make2
(
    size_t _rows,
    size_t _cols
)
{
    size_t e[2];

    e[0] = _rows;
    e[1] = _cols;

    return d_table_domain_make(e, D_TABLE_RANK_TABLE);
}

/*
d_table_domain_slice_offset
  the atomic position at which the subtable T[_leading] begins.

Parameter(s):
  _domain:  the domain the slice lives in.
  _leading: the index of the leading slice.
Return:
  The linear offset, in positions, at which that slice begins.
*/
D_NODISCARD size_t
d_table_domain_slice_offset
(
    struct d_table_domain _domain,
    size_t                _leading
)
{
    return (_leading * d_table_domain_slice_extent(_domain));
}

/*
d_table_domain_projection_extent
  the length of the projection along coordinate _r -- over a box domain every
  position reaches it, so it is that coordinate's extent.

Parameter(s):
  _domain: the domain to project.
  _r:      the coordinate to project along.
Return:
  The length of the projection along `_r`, which over a box domain is that
  coordinate's extent.
*/
D_NODISCARD D_INLINE_DEF size_t
d_table_domain_projection_extent
(
    struct d_table_domain _domain,
    uint32_t              _r
)
{
    return (size_t)_domain.extent[_r];
}

/*
d_table_domain_refine
  the finer domain I_T' obtained by subdividing each coordinate by its factor in
  _factors -- the index-space refinement an ATOMIC SPLIT needs. Every position
  of I_T becomes a box of prod_r factor_r positions in I_T'. The projection back
  is d_table_domain_project.

Parameter(s):
  _domain:  the domain to subdivide.
  _factors: per-coordinate subdivision factors.
Return:
  The finer domain obtained by multiplying each coordinate's extent by its
  factor.
*/
D_NODISCARD D_INLINE_DEF struct d_table_domain
d_table_domain_refine
(
    struct d_table_domain _domain,
    const size_t*         _factors
)
{
    struct d_table_domain result;
    uint32_t              r;

    result = _domain;

    for (r = 0; r < _domain.rank; ++r)
    {
        result.extent[r] = (D_INTERNAL_TABLE_EXTENT)
            ((size_t)_domain.extent[r] * _factors[r]);
    }

    return result;
}

/*
d_table_domain_project
  pi(i') -- the coarse index a refined index lies over. With the refinement
  above this is coordinatewise integer division by the factor, so pi is total
  and every fibre pi^-1(i) is the box of subpositions of i.

Parameter(s):
  _refined: an index in the refined domain.
  _factors: the per-coordinate factors the domain was refined by.
Return:
  The corresponding index in the coarse domain, each coordinate divided by its
  factor.
*/
D_NODISCARD struct d_table_index
d_table_domain_project
(
    struct d_table_index _refined,
    const size_t*        _factors
)
{
    struct d_table_index result;
    uint32_t             r;

    result = d_table_index_zero(_refined.rank);

    for (r = 0; r < _refined.rank; ++r)
    {
        result.coord[r] = (D_INTERNAL_TABLE_EXTENT)
            ((size_t)_refined.coord[r] / _factors[r]);
    }

    return result;
}

#include "../../../../../inc/djinterp/c/container/table/table_domain.h"


/*
d_table_index_of
  the multi-index over _rank coordinates read from _coords.

Parameter(s):
  _coords: array of `_rank` coordinates.
  _rank:   the number of coordinates.
Return:
  A `d_table_index` holding those coordinates; positions beyond D_TABLE_MAX_RANK
  are dropped.
*/
D_NODISCARD struct d_table_index
d_table_index_of
(
    const size_t* _coords,
    uint32_t      _rank
)
{
    struct d_table_index result;
    uint32_t             r;

    result = d_table_index_zero(_rank);

    for (r = 0; (r < _rank) && (r < D_TABLE_MAX_RANK); ++r)
    {
        result.coord[r] = (D_INTERNAL_TABLE_EXTENT)_coords[r];
    }

    return result;
}

/*
d_table_index_storable
  whether every coordinate fits a stored extent, and the rank fits this build.
  The guard before narrowing an index that has not been checked against a
  domain.

Parameter(s):
  _coords: array of `_rank` coordinates.
  _rank:   the number of coordinates.
Return:
  Nonzero when the coordinates fit the representable index space, 0 when the
  rank exceeds D_TABLE_MAX_RANK or a coordinate is out of range.
*/
D_NODISCARD D_INLINE_DEF int
d_table_index_storable
(
    const size_t* _coords,
    uint32_t      _rank
)
{
    uint32_t r;

    if (_rank > D_TABLE_MAX_RANK)
    {
        return 0;
    }

    for (r = 0; r < _rank; ++r)
    {
        if (!D_TABLE_EXTENT_FITS(_coords[r]))
        {
            return 0;
        }
    }

    return 1;
}
