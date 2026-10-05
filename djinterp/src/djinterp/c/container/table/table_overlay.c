/*******************************************************************************
* djinterp [c]                                                   table_overlay.c
*
*   The out-of-line half of table_overlay.h.  Every definition here is D_INLINE while its
* declaration in the header is not, so C11 6.7.4p7 makes each an EXTERNAL
* definition: one symbol, linkable from both languages, and free to be inlined
* within this translation unit.
*
*
* path:      /src/djinterp/c/container/table/table_overlay.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/

#include "../../../../../inc/djinterp/c/container/table/table_overlay.h"


/*
d_table_overlay_admits_row
  Asks whether appending _row would leave the table still wearing _overlay --
the PRESERVATION question, asked before the mutation rather than after it. This
is what distinguishes an overlay from an assertion: the restriction is kept, not
merely tested.

  Only the restrictions an append can break are consulted. Capacity can be
broken by a row; the domain can be broken by a cell; sortedness can be broken at
the seam between the last row and the new one; multiplicity can be broken by a
repeated key. A restriction whose comparator is absent is not checked.

Parameter(s):
  _table:   the table the row would join.
  _overlay: the bundle it must continue to wear.
  _row:     the cells of the candidate row.
  _width:   the number of cells in _row.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the row may be admitted, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if a required argument was null, or
  - D_TABLE_STATUS_SHAPE, if the row's width disagrees with the table's, or
  - D_TABLE_STATUS_CAPACITY, if the row would pass kappa, or
  - D_TABLE_STATUS_DOMAIN, if a cell lies outside <I>, or the row would break
    sortedness or the multiplicity bound.
*/
enum d_table_status
d_table_overlay_admits_row
(
    const struct d_table*         _table,
    const struct d_table_overlay* _overlay,
    const void*                   _row,
    size_t                        _width
)
{
    const unsigned char* cells;
    size_t               width;
    size_t               c;
    size_t               r;

    if ( (!_table) ||
         (!_overlay) ||
         (!_row) )
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // rectangularity first: a row of the wrong width is not a bundle question
    if ( (_table->extent[1] != 0) &&
         (_width != (size_t)_table->extent[1]) )
    {
        return D_TABLE_STATUS_SHAPE;
    }

    // gamma_kappa: the ceiling is over CELLS, |c| <= kappa
    if (d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_CAPACITY))
    {
        if ((d_table_size(_table) + _width) >
            d_table_overlay_capacity_of(_overlay))
        {
            return D_TABLE_STATUS_CAPACITY;
        }
    }

    cells = (const unsigned char*)_row;
    width = (size_t)_table->cell_size;

    // delta_I: every cell of the candidate row must lie in the interval
    if (d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_DOMAIN))
    {
        for (c = 0; c < _width; ++c)
        {
            if (!d_table_overlay_in_domain(_overlay, cells + (c * width)))
            {
                return D_TABLE_STATUS_DOMAIN;
            }
        }
    }

    // sigma: the seam between the last existing row and this one
    if ( (d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_SORTED)) &&
         (_overlay->compare != NULL)                                   &&
         (_table->extent[0] != 0)                                           &&
         (_width != 0) )
    {
        const void* last;

        last = d_table_cell_const(_table, (size_t)_table->extent[0] - 1, 0);

        if (_overlay->compare(last, (const void*)cells,
                              _overlay->context) > 0)
        {
            return D_TABLE_STATUS_DOMAIN;
        }
    }

    // mu_m^E: a repeated key, counted over the first column
    if ( (d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_MULTIPLICITY)) &&
         (_overlay->compare != NULL)                                         &&
         (_width != 0) )
    {
        size_t      seen;
        const void* candidate;

        seen      = 0;
        candidate = d_table_overlay_key(_overlay, (const void*)cells);

        for (r = 0; r < (size_t)_table->extent[0]; ++r)
        {
            const void* existing;

            existing = d_table_overlay_key(
                _overlay, d_table_cell_const(_table, r, 0));

            if (_overlay->compare(existing, candidate,
                                  _overlay->context) == 0)
            {
                ++seen;
            }
        }

        if ((seen + 1) > (size_t)_overlay->multiplicity)
        {
            return D_TABLE_STATUS_DOMAIN;
        }
    }

    return D_TABLE_STATUS_OK;
}

/*
d_table_overlay_holds
  Asks whether a table already wears a bundle -- the membership question, over a
container that exists. Where admits_row looks at one seam, this walks the whole
container, so it is the predicate a test asserts and not one an append calls.

Parameter(s):
  _table:   the table to inspect.
  _overlay: the bundle to check it against.
Return:
  A d_table_status value corresponding to either:
  - D_TABLE_STATUS_OK, if the table wears the bundle, or
  - D_TABLE_STATUS_INVALID_ARGUMENT, if a required argument was null, or
  - D_TABLE_STATUS_CAPACITY, if |c| exceeds kappa, or
  - D_TABLE_STATUS_DOMAIN, if a cell leaves <I>, or the rows are out of order,
    or a key recurs more often than the bound allows.
*/
enum d_table_status
d_table_overlay_holds
(
    const struct d_table*         _table,
    const struct d_table_overlay* _overlay
)
{
    size_t r;
    size_t c;

    if ( (!_table) ||
         (!_overlay) )
    {
        return D_TABLE_STATUS_INVALID_ARGUMENT;
    }

    // gamma_kappa
    if (d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_CAPACITY))
    {
        if (d_table_size(_table) > d_table_overlay_capacity_of(_overlay))
        {
            return D_TABLE_STATUS_CAPACITY;
        }
    }

    // delta_I over every cell
    if (d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_DOMAIN))
    {
        for (r = 0; r < (size_t)_table->extent[0]; ++r)
        {
            for (c = 0; c < (size_t)_table->extent[1]; ++c)
            {
                if (!d_table_overlay_in_domain(
                        _overlay, d_table_cell_const(_table, r, c)))
                {
                    return D_TABLE_STATUS_DOMAIN;
                }
            }
        }
    }

    // sigma along the row positions
    if ( (d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_SORTED)) &&
         (_overlay->compare != NULL)                                   &&
         (_table->extent[1] != 0) )
    {
        for (r = 1; r < (size_t)_table->extent[0]; ++r)
        {
            if (_overlay->compare(d_table_cell_const(_table, r - 1, 0),
                                  d_table_cell_const(_table, r, 0),
                                  _overlay->context) > 0)
            {
                return D_TABLE_STATUS_DOMAIN;
            }
        }
    }

    // mu_m^E over the keys of the first column
    if ( (d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_MULTIPLICITY)) &&
         (_overlay->compare != NULL)                                         &&
         (_table->extent[1] != 0) )
    {
        for (r = 0; r < (size_t)_table->extent[0]; ++r)
        {
            size_t      seen;
            const void* key;

            seen = 0;
            key  = d_table_overlay_key(
                _overlay, d_table_cell_const(_table, r, 0));

            for (c = 0; c < (size_t)_table->extent[0]; ++c)
            {
                const void* other;

                other = d_table_overlay_key(
                    _overlay, d_table_cell_const(_table, c, 0));

                if (_overlay->compare(key, other, _overlay->context) == 0)
                {
                    ++seen;
                }
            }

            if (seen > (size_t)_overlay->multiplicity)
            {
                return D_TABLE_STATUS_DOMAIN;
            }
        }
    }

    return D_TABLE_STATUS_OK;
}

/*
d_table_overlay_meet
  Composes two bundles: O1 /\ O2 = R1 u R2, the union of their restrictions.
Where both declare the same restriction the STRONGER parameter wins, since the
extension of a meet is the intersection and a meet may only shrink it -- the
smaller capacity, the smaller multiplicity, and the tighter interval.

  The comparator and the projection are taken from whichever bundle supplies
one; two bundles that disagree about comp_tau are comparing different things and
their meet is not meaningful, which is the caller's error to avoid.

Parameter(s):
  _a: one bundle; may be null, read as the empty overlay.
  _b: the other bundle; may be null, read as the empty overlay.
Return:
  The composed bundle.
*/
struct d_table_overlay
d_table_overlay_meet
(
    const struct d_table_overlay* _a,
    const struct d_table_overlay* _b
)
{
    struct d_table_overlay empty;
    struct d_table_overlay result;

    empty = d_table_overlay_none();

    // an absent bundle is the empty one, which restricts nothing
    if (!_a)
    {
        _a = &empty;
    }

    if (!_b)
    {
        _b = &empty;
    }

    result         = d_table_overlay_none();
    result.present = (_a->present | _b->present);

    // the comparator and the projection come from whichever supplies one
    result.compare = (_a->compare ? _a->compare : _b->compare);
    result.key_of  = (_a->key_of  ? _a->key_of  : _b->key_of);
    result.context = (_a->compare ? _a->context : _b->context);

    // capacity: the smaller ceiling binds
    if (d_table_overlay_wears(&result, D_TABLE_RESTRICTION_CAPACITY))
    {
        size_t ca = d_table_overlay_capacity_of(_a);
        size_t cb = d_table_overlay_capacity_of(_b);

        result.capacity = (D_INTERNAL_TABLE_EXTENT)((ca < cb) ? ca : cb);
    }

    // multiplicity: the smaller bound binds
    if (d_table_overlay_wears(&result, D_TABLE_RESTRICTION_MULTIPLICITY))
    {
        D_INTERNAL_TABLE_EXTENT ma;
        D_INTERNAL_TABLE_EXTENT mb;

        ma = (d_table_overlay_wears(_a, D_TABLE_RESTRICTION_MULTIPLICITY)
                  ? _a->multiplicity : D_INTERNAL_TABLE_EXTENT_MAX);
        mb = (d_table_overlay_wears(_b, D_TABLE_RESTRICTION_MULTIPLICITY)
                  ? _b->multiplicity : D_INTERNAL_TABLE_EXTENT_MAX);

        result.multiplicity = ((ma < mb) ? ma : mb);
    }

    // domain: the tighter interval binds, which needs a comparator to decide
    if (d_table_overlay_wears(&result, D_TABLE_RESTRICTION_DOMAIN))
    {
        int a_has = d_table_overlay_wears(_a, D_TABLE_RESTRICTION_DOMAIN);
        int b_has = d_table_overlay_wears(_b, D_TABLE_RESTRICTION_DOMAIN);

        if (!a_has)
        {
            result.domain_low  = _b->domain_low;
            result.domain_high = _b->domain_high;
        }
        else if (!b_has)
        {
            result.domain_low  = _a->domain_low;
            result.domain_high = _a->domain_high;
        }
        else if (result.compare)
        {
            result.domain_low =
                ((result.compare(_a->domain_low, _b->domain_low,
                                 result.context) >= 0)
                     ? _a->domain_low : _b->domain_low);
            result.domain_high =
                ((result.compare(_a->domain_high, _b->domain_high,
                                 result.context) <= 0)
                     ? _a->domain_high : _b->domain_high);
        }
        else
        {
            result.domain_low  = _a->domain_low;
            result.domain_high = _a->domain_high;
        }
    }

    return result;
}

/*
d_table_overlay_push_row
  Appends a row only when doing so preserves the bundle. The overlay is KEPT,
not merely tested: a row that would break a restriction is refused and the
container is left exactly as it was.

Parameter(s):
  _table:   the table to append to.
  _overlay: the bundle it must continue to wear.
  _row:     the cells of the new row.
  _width:   the number of cells in _row.
  _alloc:   the caller's storage strategy.
Return:
  A d_table_status value: the refusal from d_table_overlay_admits_row when the
bundle would break, otherwise the result of d_table_push_row.
*/
enum d_table_status
d_table_overlay_push_row
(
    struct d_table*               _table,
    const struct d_table_overlay* _overlay,
    const void*                   _row,
    size_t                        _width,
    const struct d_table_alloc*   _alloc
)
{
    enum d_table_status status;

    status = d_table_overlay_admits_row(_table, _overlay, _row, _width);

    // the restriction is preserved, so a refusal ends the operation here
    if (status != D_TABLE_STATUS_OK)
    {
        return status;
    }

    return d_table_push_row(_table, _row, _width, _alloc);
}

/*
d_table_overlay_stronger_than
  Compares two bundles under the strength order: O1 <= O2 exactly when O1 admits
no more containers than O2. A bundle is at least as strong as another when it
declares every restriction the other does, with a parameter no weaker.

  Only the parameters the framework can compare without a value comparator are
read; a domain interval is treated as no-weaker when the restriction is present,
since deciding otherwise would need comp_tau and a caller that cares can call
d_table_overlay_meet and compare the result.

Parameter(s):
  _a: the candidate stronger bundle.
  _b: the candidate weaker bundle.
Return:
  An integer value corresponding to either:
  - 1, if _a is at least as strong as _b, or
  - 0, otherwise.
*/
int
d_table_overlay_stronger_than
(
    const struct d_table_overlay* _a,
    const struct d_table_overlay* _b
)
{
    if ( (!_a) ||
         (!_b) )
    {
        return 0;
    }

    // every restriction the weaker declares must also be declared here
    if ((_a->present & _b->present) != _b->present)
    {
        return 0;
    }

    // a ceiling is no weaker when it is no larger
    if (d_table_overlay_wears(_b, D_TABLE_RESTRICTION_CAPACITY))
    {
        if (d_table_overlay_capacity_of(_a) >
            d_table_overlay_capacity_of(_b))
        {
            return 0;
        }
    }

    // a multiplicity bound is no weaker when it is no larger
    if (d_table_overlay_wears(_b, D_TABLE_RESTRICTION_MULTIPLICITY))
    {
        if (_a->multiplicity > _b->multiplicity)
        {
            return 0;
        }
    }

    return 1;
}


/*
d_table_overlay_wears
  whether the bundle declares a given restriction.

Parameter(s):
  _overlay:     the restriction bundle to interrogate.
  _restriction: one D_TABLE_RESTRICTION_* bit.
Return:
  Nonzero when the bundle declares that restriction, 0 when it does not.
*/
D_NODISCARD D_INLINE_DEF int
d_table_overlay_wears
(
    const struct d_table_overlay* _overlay,
    uint32_t                      _restriction
)
{
    return ((_overlay->present & _restriction) != 0);
}

/*
d_table_overlay_capacity_of
  kappa, or the largest representable extent when the bundle declares no
  capacity -- an undeclared ceiling does not bind.

Parameter(s):
  _overlay: the restriction bundle to interrogate.
Return:
  The declared capacity, or the largest representable extent when the bundle
  declares none -- an undeclared ceiling does not bind.
*/
D_NODISCARD D_INLINE_DEF size_t
d_table_overlay_capacity_of
(
    const struct d_table_overlay* _overlay
)
{
    if (!d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_CAPACITY))
    {
        return (size_t)D_INTERNAL_TABLE_EXTENT_MAX;
    }

    return (size_t)_overlay->capacity;
}

/*
d_table_overlay_in_domain
  whether a cell lies in <I> -- delta_I applied to one value. Vacuously true
  when the restriction is absent, and also when the bundle carries no
  comparator, since an unreadable restriction is not a failed one.

Parameter(s):
  _overlay: the restriction bundle to consult.
  _cell:    the candidate cell value.
Return:
  Nonzero when the value is admissible. An undeclared or uncheckable restriction
  admits every value.
*/
D_NODISCARD D_INLINE_DEF int
d_table_overlay_in_domain
(
    const struct d_table_overlay* _overlay,
    const void*                   _cell
)
{
    // an undeclared or uncheckable restriction admits every value
    if ( (!d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_DOMAIN)) ||
         (_overlay->compare == NULL) )
    {
        return 1;
    }

    // the closed interval: lo <= value <= hi
    return ( (_overlay->compare(_cell, _overlay->domain_low,
                                _overlay->context) >= 0) &&
             (_overlay->compare(_cell, _overlay->domain_high,
                                _overlay->context) <= 0) );
}

/*
d_table_overlay_key
  eta's projection of a cell, or the cell itself when the bundle is not keyed --
  an unkeyed duplicate-equivalence is over whole values.

Parameter(s):
  _overlay: the restriction bundle to consult.
  _cell:    the cell to take the key of.
Return:
  The cell's key under the bundle's key function, or the cell itself when the
  bundle declares no keying.
*/
D_NODISCARD D_INLINE_DEF const void*
d_table_overlay_key
(
    const struct d_table_overlay* _overlay,
    const void*                   _cell
)
{
    if ( (!d_table_overlay_wears(_overlay, D_TABLE_RESTRICTION_KEYED)) ||
         (_overlay->key_of == NULL) )
    {
        return _cell;
    }

    return _overlay->key_of(_cell, _overlay->context);
}
