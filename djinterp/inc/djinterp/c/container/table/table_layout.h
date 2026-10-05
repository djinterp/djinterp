/*******************************************************************************
* djinterp [c]                                                    table_layout.h
*
*   Gamma, the cover over the atomic positions: regions, the owner function,
* anchors, layout-aware access, and the merge and split moves.
*
*   PORTABILITY:
*   C99 / C++11, the framework floors.
*
*
* path:      /inc/djinterp/c/container/table/table_layout.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.02
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_TABLE_TABLE_LAYOUT_H
#define DJINTERP_C_CONTAINER_TABLE_TABLE_LAYOUT_H 1

// djinterp
#include "./table_carrier.h"

D_EXTERN_C_BEGIN


// d_table_region
//   struct: the region R_C of a layout cell -- a non-empty k-box of atomic
// positions,
//     R_C = ( prod_{r=0..k-1} [origin_r, origin_r + span_r) ) intersect I_T.
// Its extent |R_C| is the product of the spans; it is a MERGE when that exceeds
// one, and merged ALONG coordinate r when span_r exceeds one.  shape(C) is the
// span tuple.
struct d_table_region
{
    D_INTERNAL_TABLE_EXTENT origin[D_TABLE_MAX_RANK];
    D_INTERNAL_TABLE_EXTENT span[D_TABLE_MAX_RANK];
    uint32_t                rank;
};

// d_table_layout
//   struct: the cover Gamma over a table's atomic positions, given by its
// MERGES alone. Every position no declared merge covers is its own singleton
// cell -- the trivial cover Gamma_0 on the rest -- so a partition of I_T is
// recovered from the merge list exactly when the merges are in bounds and
// pairwise disjoint, and a layout with no merges IS Gamma_0. The struct owns
// nothing: `merges` is caller-owned, exactly as d_table's cells are, so a
// compile-time cover in C++ and a parser-accumulated one in C present the same
// object.
struct d_table_layout
{
    const struct d_table_region* merges;    // the declared merged cells
    D_INTERNAL_TABLE_EXTENT      count;     // how many
};

// V.    The layout cover Gamma

// d_table_region_make
//   function: the region spanning _spans from _origins, over _rank
// coordinates.
struct d_table_region d_table_region_make(const size_t* _origins,
                                          const size_t* _spans,
                                          uint32_t      _rank);

// d_table_region_make2
//   function: the rank-2 region spanning _rows x _cols from (_row0, _col0).
struct d_table_region d_table_region_make2(size_t _row0,
                                           size_t _col0,
                                           size_t _rows,
                                           size_t _cols);

// d_table_region_singleton
//   function: the singleton region at _index -- the layout cell the trivial
// cover Gamma_0 gives every atomic position.
struct d_table_region d_table_region_singleton(struct d_table_index _index);

// d_table_region_origin
//   function: the region's origin along coordinate _r.
D_NODISCARD D_INLINE size_t
d_table_region_origin(
    struct d_table_region _region,
    uint32_t              _r
)
{
    return (size_t)_region.origin[_r];
}

// d_table_region_span
//   function: shape(C) along coordinate _r -- the region's span there.
D_NODISCARD D_INLINE size_t
d_table_region_span(
    struct d_table_region _region,
    uint32_t              _r
)
{
    return (size_t)_region.span[_r];
}

// d_table_region_extent
//   function: |R_C|, the number of atomic positions the region holds -- the
// product of its spans.
D_NODISCARD D_INLINE size_t
d_table_region_extent(
    struct d_table_region _region
)
{
    size_t   total;
    uint32_t r;

    total = 1;

    for (r = 0; r < _region.rank; ++r)
    {
        total = total * (size_t)_region.span[r];
    }

    return total;
}

// d_table_region_is_merge
//   function: whether the region spans more than one atomic position.
int d_table_region_is_merge(struct d_table_region _region);

// d_table_region_is_merged_along
//   function: whether the region is merged along coordinate _r.
int d_table_region_is_merged_along(struct d_table_region _region,
                                   uint32_t              _r);

// d_table_region_contains
//   function: whether the region covers the atomic position _index.
D_NODISCARD D_INLINE int
d_table_region_contains(
    struct d_table_region _region,
    struct d_table_index  _index
)
{
    uint32_t r;

    if (_index.rank != _region.rank)
    {
        return 0;
    }

    for (r = 0; r < _region.rank; ++r)
    {
        size_t c = (size_t)_index.coord[r];

        if ( (c <  (size_t)_region.origin[r]) ||
             (c >= (size_t)_region.origin[r] + (size_t)_region.span[r]) )
        {
            return 0;
        }
    }

    return 1;
}

// d_table_region_anchor
//   function: anchor(C) = min_lex R_C -- the canonical representative that
// names a merged cell once. For a box that is its origin.
D_NODISCARD D_INLINE struct d_table_index
d_table_region_anchor(
    struct d_table_region _region
)
{
    struct d_table_index result;
    uint32_t             r;

    result.rank = _region.rank;

    for (r = 0; r < D_TABLE_MAX_RANK; ++r)
    {
        result.coord[r] = _region.origin[r];
    }

    return result;
}

// d_table_region_within
//   function: whether the region lies wholly inside a domain. A cover's
// regions must, since a layout cell is a region of ATOMIC positions.
int d_table_region_within(struct d_table_region _region,
                          struct d_table_domain _domain);

// d_table_regions_overlap
//   function: whether two regions share any atomic position. A cover's regions
// must not: every atomic position lies in exactly one cell. Two boxes overlap
// exactly when they overlap on EVERY coordinate.
int d_table_regions_overlap(struct d_table_region _a,
                            struct d_table_region _b);

// d_table_layout_none
//   function: the trivial cover Gamma_0 -- no merges, so every atomic position
// is its own layout cell. This is the ordinary table.
D_NODISCARD D_INLINE struct d_table_layout
d_table_layout_none(void)
{
    struct d_table_layout result;

    result.merges = NULL;
    result.count  = 0;

    return result;
}

// d_table_layout_over
//   function: the cover given by _count declared merges at _merges.
struct d_table_layout d_table_layout_over(const struct d_table_region* _merges,
                                          size_t                       _count);

// d_table_layout_wears_no_merges
//   function: whether every region is a singleton -- the Gamma_0 case.
int d_table_layout_wears_no_merges(const struct d_table_layout* _layout);

// d_table_layout_owner
//   function: cell_T(i) -- the layout cell owning the atomic position _index.
// A declared merge that covers it, or the singleton cell there. Total on I_T
// by construction, which is what makes Gamma a cover.
D_NODISCARD D_INLINE struct d_table_region
d_table_layout_owner(
    const struct d_table_layout* _layout,
    struct d_table_index         _index
)
{
    size_t i;

    // the first declared merge covering the position owns it; a valid cover
    // has at most one, so "first" and "the" coincide
    for (i = 0; i < (size_t)_layout->count; ++i)
    {
        if (d_table_region_contains(_layout->merges[i], _index))
        {
            return _layout->merges[i];
        }
    }

    return d_table_region_singleton(_index);
}

// d_table_layout_span
//   function: span_T(i) = R_{cell_T(i)} -- the region of the position's owner.
D_NODISCARD D_INLINE struct d_table_region
d_table_layout_span(
    const struct d_table_layout* _layout,
    struct d_table_index         _index
)
{
    return d_table_layout_owner(_layout, _index);
}

// d_table_layout_anchor_of
//   function: anchor(cell_T(i)) -- the position every index in a merged region
// reads as. Off the merges it is the index itself.
D_NODISCARD D_INLINE struct d_table_index
d_table_layout_anchor_of(
    const struct d_table_layout* _layout,
    struct d_table_index         _index
)
{
    return d_table_region_anchor(d_table_layout_owner(_layout, _index));
}

// d_table_layout_is_anchor
//   function: whether the position names its own cell -- i in A_T. A covered
// non-anchor position remains a valid index and defers to the cell anchored
// elsewhere.
int d_table_layout_is_anchor(const struct d_table_layout* _layout,
                             struct d_table_index         _index);

// d_table_resolve_index
//   function: LAYOUT-AWARE access at a rank-k index -- a pointer to the value
// the position reads as, which is its anchor's cell,
//     T[i] = T[anchor(cell_T(i))]. The two accesses agree off the merges, so
// passing Gamma_0 gives exactly d_table_cell_at_const. NULL when the index is
// outside I_T.
D_NODISCARD D_INLINE const void*
d_table_resolve_index(
    const struct d_table*        _table,
    const struct d_table_layout* _layout,
    struct d_table_index         _index
)
{
    // an out-of-domain index is undefined under either access
    if (!d_table_contains(_table, _index))
    {
        return NULL;
    }

    // A cover with no merges IS Gamma_0, under which every position anchors
    // itself, so there is nothing to resolve. The ordinary table takes this
    // branch, and the branch is worth its cost by a wide margin: an owner
    // search that can only ever return the singleton still builds a region and
    // an index by value, and a swept read measured 66x the hand-coded baseline
    // without this test and 2.1x with it.
    if (_layout->count == 0)
    {
        return d_table_cell_at_const(_table, _index);
    }

    return d_table_cell_at_const(
        _table, d_table_layout_anchor_of(_layout, _index));
}

// d_table_resolve
//   function: the rank-2 d_table_resolve_index.
D_NODISCARD D_INLINE const void*
d_table_resolve(
    const struct d_table*        _table,
    const struct d_table_layout* _layout,
    size_t                       _row,
    size_t                       _col
)
{
    // the same trivial-cover shortcut as d_table_resolve_index, taken before
    // an index is built rather than after: constructing one is most of what is
    // left to pay once the owner search is skipped
    if (_layout->count == 0)
    {
        return d_table_at_const(_table, _row, _col);
    }

    return d_table_resolve_index(_table, _layout,
                                 d_table_index_make2(_row, _col));
}


// d_table_region_contains2
//   function: the rank-2 d_table_region_contains, taking loose coordinates.
D_NODISCARD D_INLINE int
d_table_region_contains2(
    struct d_table_region _region,
    size_t                _row,
    size_t                _col
)
{
    return d_table_region_contains(_region, d_table_index_make2(_row, _col));
}

// d_table_layout_owner2
//   function: the rank-2 d_table_layout_owner.
D_NODISCARD D_INLINE struct d_table_region
d_table_layout_owner2(
    const struct d_table_layout* _layout,
    size_t                       _row,
    size_t                       _col
)
{
    return d_table_layout_owner(_layout, d_table_index_make2(_row, _col));
}

// d_table_layout_anchor_of2
//   function: the rank-2 d_table_layout_anchor_of.
D_NODISCARD D_INLINE struct d_table_index
d_table_layout_anchor_of2(
    const struct d_table_layout* _layout,
    size_t                       _row,
    size_t                       _col
)
{
    return d_table_layout_anchor_of(_layout, d_table_index_make2(_row, _col));
}

// d_table_layout_is_anchor2
//   function: the rank-2 d_table_layout_is_anchor.
D_NODISCARD D_INLINE int
d_table_layout_is_anchor2(
    const struct d_table_layout* _layout,
    size_t                       _row,
    size_t                       _col
)
{
    return d_table_layout_is_anchor(_layout, d_table_index_make2(_row, _col));
}

// V.b   Splits
//   containers.tex defines a SPLIT as a refinement of the cover: it replaces a
// cell C by a partition of its OWN region into s >= 2 non-empty pieces,
//     union_a R_a = R_C,   R_a intersect R_b = empty,
// each piece becoming a layout cell C_j = (R_j, v_j, tau_j).  So a split is the
// inverse move to a merge, and it is an OVERLAY move: I_T does not change.
//
//   SPLITTING AN ATOMIC CELL IS A DIFFERENT MOVE.  A singleton region cannot be
// partitioned, so the interface gesture that splits an atomic cell is modelled
// beneath the cover instead: the atomic domain itself is refined to a finer
// I_T' with a projection pi : I_T' -> I_T, and the position i is split exactly
// when |pi^-1(i)| > 1.  d_table_domain_refine and d_table_domain_project are
// that pair; the new subcells are supported on the fibre.
//
//   The two are not interchangeable, and the tex says so plainly: splitting an
// existing cell is an overlay move, splitting an atomic cell is a refinement of
// the index space beneath it.

// II.   mutation and reordering
enum d_table_status d_table_region_partitions(
    const struct d_table_region* _pieces,
    size_t                       _count,
    struct d_table_region        _whole);
enum d_table_status d_table_layout_split(
    struct d_table_region*       _merges,
    size_t*                      _count,
    size_t                       _capacity,
    size_t                       _which,
    const struct d_table_region* _pieces,
    size_t                       _piece_count);
enum d_table_status d_table_layout_merge(struct d_table_region* _merges,
                                         size_t*                _count,
                                         size_t                 _capacity,
                                         struct d_table_region  _region,
                                         struct d_table_domain  _domain);
size_t              d_table_index_fibre(struct d_table_index  _coarse,
                                        const size_t*         _factors,
                                        struct d_table_index* _out,
                                        size_t                _out_capacity);


// III.  operations (cont.)
enum d_table_status d_table_layout_validate(
    const struct d_table_layout* _layout,
    struct d_table_domain        _domain);


// layout assertions -- drift becomes a compile error rather than a wire-format
// bug, and both dialects compile them
D_STATIC_ASSERT(offsetof(struct d_table_region, origin) == 0,
                "d_table_region field drift: the origin must lead");

D_STATIC_ASSERT(offsetof(struct d_table_region, span) ==
                    (D_TABLE_MAX_RANK * sizeof(D_INTERNAL_TABLE_EXTENT)),
                "d_table_region field drift: the spans must follow the origin");

D_EXTERN_C_END


#endif  // DJINTERP_C_CONTAINER_TABLE_TABLE_LAYOUT_H
