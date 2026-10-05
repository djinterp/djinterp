/*******************************************************************************
* djinterp [c]                                                      option_set.c
*
* The option set's pristine mark and the by-value status reader.
*   Defines the three functions option_set.h declares; the rest of the set
* lives in option_set_common.c.
*
*
* path:      /src/djinterp/c/option/option_set.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/option/option_set.h"  // corresponding header
// std
#include <stdbool.h>  // bool
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, uint32_t


/*
d_option_set_mark_pristine
  Lowers the flag in place on every cell in use; cells beyond `count` are not
part of the set and are left alone.
*/
void
d_option_set_mark_pristine(
    struct d_option_set* _set
)
{
    // nothing to mark
    if ( (!_set) ||
         (!_set->options) )
    {
        return;
    }

    // lower D_OPTION_FLAG_ASSIGNED on every cell in use
    for (uint32_t i = 0u; i < _set->count; ++i)
    {
        _set->options[i].flags &= ~D_OPTION_FLAG_ASSIGNED;
    }

    return;
}

/*
d_option_set_is_pristine
  A missing or empty set has no assigned cell, so it is pristine.
*/
bool
d_option_set_is_pristine(
    const struct d_option_set* _set
)
{
    // no cells, nothing assigned
    if ( (!_set) ||
         (!_set->options) )
    {
        return true;
    }

    // any assigned cell ends it
    for (uint32_t i = 0u; i < _set->count; ++i)
    {
        if ((_set->options[i].flags & D_OPTION_FLAG_ASSIGNED) != 0u)
        {
            return false;
        }
    }

    return true;
}

/*
d_option_result_status_of
  The by-value spelling of d_option_result_status, whose reading of the two
arms it reuses rather than repeats.
*/
int32_t
d_option_result_status_of(
    struct d_option_result _result
)
{
    return d_option_result_status(&_result);
}
