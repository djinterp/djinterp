/*******************************************************************************
* djinterp [c]                                                      functional.c
*
* The searches and the sortedness test functional.h declares.
*   Their predicates and comparator take no context here, so each is called
* with NULL. A search that finds nothing returns NULL or (size_t)-1, as the
* header's macros document.
*
*
* path:      /src/djinterp/c/functional/functional.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/functional.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL


static const void*
d_internal_functional_element(
    const void* _input,
    size_t      _i,
    size_t      _element_size
)
{
    return (const unsigned char*)_input + (_i * _element_size);
}

static bool
d_internal_functional_searchable(
    const void* _input,
    size_t      _count,
    size_t      _element_size,
    bool        _has_callback
)
{
    return ( (_has_callback)  &&
             (_input != NULL) &&
             (_count > 0u)    &&
             (_element_size > 0u) );
}

size_t
d_functional_index_of(
    const void*  _input,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test
)
{
    // nothing to search
    if (!d_internal_functional_searchable(_input,
                                          _count,
                                          _element_size,
                                          (_test != NULL)))
    {
        return (size_t)-1;
    }

    // the first element the test accepts
    for (size_t i = 0u; i < _count; ++i)
    {
        if (_test(d_internal_functional_element(_input, i, _element_size),
                  NULL))
        {
            return i;
        }
    }

    return (size_t)-1;
}

size_t
d_functional_last_index_of(
    const void*  _input,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test
)
{
    // nothing to search
    if (!d_internal_functional_searchable(_input,
                                          _count,
                                          _element_size,
                                          (_test != NULL)))
    {
        return (size_t)-1;
    }

    // the last element the test accepts, walking backwards
    for (size_t i = _count; i > 0u; --i)
    {
        if (_test(d_internal_functional_element(_input, i - 1u, _element_size),
                  NULL))
        {
            return i - 1u;
        }
    }

    return (size_t)-1;
}

void*
d_functional_find_last(
    const void*  _input,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test
)
{
    const size_t index = d_functional_last_index_of(_input,
                                                    _count,
                                                    _element_size,
                                                    _test);

    return (index == (size_t)-1)
        ? NULL
        : (void*)d_internal_functional_element(_input, index, _element_size);
}

/*
d_functional_is_sorted
  Non-decreasing order: equal neighbours are sorted, and the walk stops at
the first pair whose comparison is positive. Fewer than two elements are
sorted; a missing comparator, or a missing array with elements, is not.
*/
bool
d_functional_is_sorted(
    const void*            _input,
    size_t                 _count,
    size_t                 _element_size,
    fn_function_comparator _function_comparator
)
{
    // nothing to compare against
    if ( (!_function_comparator) ||
         ( (_count > 1u) &&
           ( (!_input) || (_element_size == 0u) ) ) )
    {
        return false;
    }

    // every adjacent pair in order
    for (size_t i = 1u; i < _count; ++i)
    {
        if (_function_comparator(
                d_internal_functional_element(_input, i - 1u, _element_size),
                d_internal_functional_element(_input, i, _element_size),
                NULL) > 0)
        {
            return false;
        }
    }

    return true;
}
