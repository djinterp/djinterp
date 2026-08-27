/******************************************************************************
* djinterp [c/util/document]                                 document_common.c
*
*   Implementation of the hint bag.  Compiled by both languages from this one
* source.
*
*   THE ONE INTERESTING FUNCTION IS CANONICALISE, and what makes it
* interesting is that it is not a sort.  See the header: canonical form is
* `container_metadata::set` applied left to right, which keeps a repeated
* key's FIRST position and its LAST value.  A stable sort by key would pass
* every test that used distinct keys and fail the moment a document repeated
* one -- which is the failure mode this file exists to not have.
*
* path:      /src/djinterp/c/util/document/document_common.c
* author(s): TBA                                            created: 2026.08.23
******************************************************************************/

#include "djinterp/c/util/document/document_common.h"

D_EXTERN_C_BEGIN


/*   BORROWED STRINGS COMPARE BY CONTENT, NOT BY POINTER.  Two arena entries
   holding the same key are two different pointers -- the parser copies each
   token in as it reads it -- so pointer equality would report every duplicate
   as distinct and canonicalise would become a no-op that still passed its own
   tests.
     NULL IS A KEY.  A malformed attribute can leave one, and treating NULL as
   equal to NULL rather than as never-equal means two of them collapse instead
   of both surviving into a bag that then has a duplicate in it. */
static int
d_internal_doc_key_equal_
(
    const char* _a,
    const char* _b
)
{
    size_t _i = 0;

    if (_a == _b)
    {
        return 1;
    }

    if (!_a || !_b)
    {
        return 0;
    }

    while (_a[_i] != '\0' && _a[_i] == _b[_i])
    {
        ++_i;
    }

    return _a[_i] == _b[_i];
}


struct d_doc_attributes
d_doc_attributes_canonicalise
(
    struct d_doc_attr* _run,
    uint32_t           _count
)
{
    struct d_doc_attributes _bag = D_DOC_ATTRIBUTES_EMPTY;

    uint32_t _read = 0;
    uint32_t _kept = 0;

    if (!_run || _count == 0u)
    {
        return _bag;
    }

    for (_read = 0u; _read < _count; ++_read)
    {
        uint32_t _at    = 0u;
        int      _found = 0;

        /*   FIRST POSITION, LAST VALUE.  A repeat overwrites the value where
           the key already sits; it does NOT move the entry to the end and it
           does NOT keep the earlier value.  Both of those are what a reader
           expects from "canonical" and both are wrong here -- the C++
           container appends only when the key is absent. */
        for (_at = 0u; _at < _kept; ++_at)
        {
            if (d_internal_doc_key_equal_(_run[_at].key, _run[_read].key))
            {
                _run[_at].value = _run[_read].value;
                _found          = 1;

                break;
            }
        }

        if (_found)
        {
            continue;
        }

        /*   COMPACTING IN PLACE.  `_kept` never runs ahead of `_read`, so
           this only ever moves an entry down or leaves it where it is.  The
           self-assignment when nothing has been dropped yet is deliberate --
           branching around it would be a second path to get wrong. */
        _run[_kept] = _run[_read];
        ++_kept;
    }

    _bag.items    = _run;
    _bag.count    = _kept;
    _bag.reserved = 0u;

    return _bag;
}


int32_t
d_doc_attributes_is_canonical
(
    const struct d_doc_attributes* _bag
)
{
    uint32_t _i = 0;
    uint32_t _j = 0;

    /*   AN ABSENT BAG IS CANONICAL, and so is an empty one.  This is an
       invariant check: answering 0 for "there is nothing here" would make
       every caller special-case the empty document. */
    if (!_bag || _bag->count == 0u)
    {
        return 1;
    }

    if (!_bag->items)
    {
        return 0;   /* a non-zero count over a NULL run is not a bag at all */
    }

    for (_i = 0u; _i + 1u < _bag->count; ++_i)
    {
        for (_j = _i + 1u; _j < _bag->count; ++_j)
        {
            if (d_internal_doc_key_equal_(_bag->items[_i].key,
                                          _bag->items[_j].key))
            {
                return 0;
            }
        }
    }

    return 1;
}


const char*
d_doc_attributes_find
(
    const struct d_doc_attributes* _bag,
    const char*                    _key
)
{
    uint32_t _i = 0;

    if (!_bag || !_bag->items)
    {
        return (const char*)0;
    }

    for (_i = 0u; _i < _bag->count; ++_i)
    {
        if (d_internal_doc_key_equal_(_bag->items[_i].key, _key))
        {
            return _bag->items[_i].value;
        }
    }

    return (const char*)0;
}


D_EXTERN_C_END
