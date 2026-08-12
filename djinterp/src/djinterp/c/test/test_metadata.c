/******************************************************************************
* djinterp [test]                                        test_metadata.c
*
*   Implementation of the metadata container.  Compiled by both languages from
* this one source.  Extracted from test_object.c (revision.md §6).
*
* path:      /src/djinterp/c/test/test_metadata.c
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.10
******************************************************************************/

#include "djinterp/c/test/test_metadata.h"

//   Nothing to compile when the module is off. The translation unit stays in
// every KERNELS line rather than being dropped per-knob: a build system that
// adds or removes a file depending on a config is a second place that config
// has to be right, and an empty object file is cheaper than that.
#if D_INTERNAL_TEST_METADATA

D_EXTERN_C_BEGIN


/*   KEY EQUALITY IS ONE COMPARISON, AND IT STAYS A NAMED FUNCTION.
   The key was a const char* compared character by character; it is now an
   interned d_test_key_id compared with ==. The body is a single expression
   and inlining it at the three call sites would be shorter.

     It stays because it is the ONE PLACE key equality is defined. A future
   change -- a tombstone value, a case-folded namespace, a generation counter
   in the high bits -- lands here rather than in three loops that have to be
   found. The differential harness also anchors an injection on this function;
   an equality rule spread across three call sites cannot be injected once. */
static int
d_test_key_equal_(
    d_test_key_id _a,
    d_test_key_id _b
)
{
    return (_a == _b) ? 1 : 0;
}


enum d_test_object_result
d_test_metadata_init(
    struct d_test_metadata* _metadata
)
{
    if (_metadata == (struct d_test_metadata*)0)
    {
        return D_TEST_OBJECT_INVALID;
    }

    _metadata->rows     = (struct d_test_kv*)0;
    _metadata->count    = (size_t)0;
    _metadata->capacity = (size_t)0;

    return D_TEST_OBJECT_OK;
}


enum d_test_object_result
d_test_metadata_bind(
    struct d_test_metadata* _metadata,
    struct d_test_kv*       _storage,
    size_t                  _capacity
)
{
    if ((_metadata == (struct d_test_metadata*)0) ||
        ((_storage == (struct d_test_kv*)0) && (_capacity != (size_t)0)))
    {
        return D_TEST_OBJECT_INVALID;
    }

    _metadata->rows     = _storage;
    _metadata->capacity = _capacity;
    _metadata->count    = (size_t)0;

    return D_TEST_OBJECT_OK;
}


enum d_test_object_result
d_test_metadata_set(
    struct d_test_metadata* _metadata,
    d_test_key_id         _key,
    void*                   _value
)
{
    size_t i;

    if ((_metadata == (struct d_test_metadata*)0) ||
        (_key == D_TEST_NO_KEY))
    {
        return D_TEST_OBJECT_INVALID;
    }

#if D_INTERNAL_TEST_METADATA_REPLACE
    for (i = (size_t)0; i < _metadata->count; ++i)
    {
        if (d_test_key_equal_(_metadata->rows[i].key, _key))
        {
            _metadata->rows[i].value = _value;

            return D_TEST_OBJECT_REPLACED;
        }
    }
#else
    (void)i;
#endif

    if (_metadata->count >= _metadata->capacity)
    {
        return D_TEST_OBJECT_FULL;
    }

    _metadata->rows[_metadata->count].key   = _key;
    _metadata->rows[_metadata->count].value = _value;
    _metadata->count += (size_t)1;

    return D_TEST_OBJECT_OK;
}


void*
d_test_metadata_get(
    const struct d_test_metadata* _metadata,
    d_test_key_id               _key
)
{
    size_t i;

    /*   A NULL CONTAINER READS AS EMPTY, so get() returns the miss string --
       the same answer an empty-but-bound container gives twenty lines below,
       and the same answer the delivered kernel gave.  revision.md 6's table
       briefly said a null return here; that entry was wrong on three counts
       (it contradicted its own summary sentence, it broke MISS's never-null
       contract, and it silently changed delivered behaviour) and has been
       reverted at the source.  D_TEST_METADATA_MISS is never null, so no
       caller of get() needs a null test. */
    if (_metadata == (const struct d_test_metadata*)0)
    {
        return D_TEST_METADATA_MISS;
    }

    if (_key == D_TEST_NO_KEY)
    {
        return D_TEST_METADATA_MISS;
    }

    for (i = (size_t)0; i < _metadata->count; ++i)
    {
        if (d_test_key_equal_(_metadata->rows[i].key, _key))
        {
            /*   THE STORED VALUE IS RETURNED AS-IS, INCLUDING NULL.  This
               used to map a null value onto the miss string, so get() never
               returned null and a row present with no value was
               indistinguishable from a miss only by intent. With a void*
               value there is nothing to map onto: MISS IS null. A row whose
               value is null still counts as present, and contains() is now the
               ONLY way to tell it from an absent key -- not merely the
               documented way. */
            return _metadata->rows[i].value;
        }
    }

    return D_TEST_METADATA_MISS;
}


int
d_test_metadata_contains(
    const struct d_test_metadata* _metadata,
    d_test_key_id               _key
)
{
    size_t i;

    if ((_metadata == (const struct d_test_metadata*)0) ||
        (_key == D_TEST_NO_KEY))
    {
        return 0;
    }

    for (i = (size_t)0; i < _metadata->count; ++i)
    {
        if (d_test_key_equal_(_metadata->rows[i].key, _key))
        {
            return 1;
        }
    }

    return 0;
}


size_t
d_test_metadata_count(
    const struct d_test_metadata* _metadata
)
{
    return (_metadata != (const struct d_test_metadata*)0)
           ? _metadata->count : (size_t)0;
}


const struct d_test_kv*
d_test_metadata_row(
    const struct d_test_metadata* _metadata,
    size_t                        _index
)
{
    if ((_metadata == (const struct d_test_metadata*)0) ||
        (_index >= _metadata->count))
    {
        return (const struct d_test_kv*)0;
    }

    return &_metadata->rows[_index];
}


D_EXTERN_C_END

#endif  // D_INTERNAL_TEST_METADATA
