/******************************************************************************
* djinterp [test]                                         test_object.c
*
*   Implementation of the shared node kernel.  Compiled by both languages from
* this one source.
*
* path:      /src/djinterp/c/test/test_object.c
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.31
******************************************************************************/

#include "djinterp/c/test/test_object.h"

D_EXTERN_C_BEGIN


/* =============================================================================
   THE NODE
   =============================================================================
     Section I was METADATA and is now test_metadata.c (revision.md §6),
   taking the seven d_test_metadata_* definitions and the static helper
   d_test_key_equal_ with it.  Nothing below calls into the metadata module --
   that is the cut, and it was exactly one call site: d_test_object_init used
   to end `return d_test_metadata_init(&_object->metadata);`. */

enum d_test_object_result
d_test_object_init(
    struct d_test_object* _object
)
{
    if (_object == (struct d_test_object*)0)
    {
        return D_TEST_OBJECT_INVALID;
    }

    _object->result      = false;
    _object->status      = (int32_t)D_TEST_STATUS_PENDING;
    _object->type_id     = (d_test_type_id)0;
    _object->callable_id = (d_test_callable_id)0;

    /*   THE CUT.  This line used to read
       `return d_test_metadata_init(&_object->metadata);` -- the single call
       site through which the node reached into metadata.  With a borrowed
       pointer there is nothing to initialise: a fresh node simply has no
       container, and a caller that wants one points at it afterwards. */

    return D_TEST_OBJECT_OK;
}




enum d_test_object_result
d_test_object_init_typed(
    struct d_test_object* _object,
    d_test_type_id        _type_id
)
{
    enum d_test_object_result r = d_test_object_init(_object);

    if (r == D_TEST_OBJECT_OK)
    {
        _object->type_id = _type_id;
    }

    return r;
}


enum d_test_object_result
d_test_object_evaluate(
    struct d_test_object* _object,
    int                   _result
)
{
    if (_object == (struct d_test_object*)0)
    {
        return D_TEST_OBJECT_INVALID;
    }

    /*   Assignment to a bool normalises; the ternary that used to do it by
       hand is gone rather than left as a no-op. */
    _object->result = (_result != 0);
    _object->status = (_result != 0)
                      ? (int32_t)D_TEST_STATUS_PASSED
                      : (int32_t)D_TEST_STATUS_FAILED;

    return D_TEST_OBJECT_OK;
}


enum d_test_object_result
d_test_object_set_status(
    struct d_test_object* _object,
    int32_t               _status
)
{
    if (_object == (struct d_test_object*)0)
    {
        return D_TEST_OBJECT_INVALID;
    }

#if D_INTERNAL_TEST_OBJECT_VALIDATE
    _object->status = d_test_status_is_valid(_status)
                      ? _status : (int32_t)D_TEST_STATUS_ERROR;
#else
    _object->status = _status;
#endif

    /*   Setting the status does NOT rewrite the boolean result.  The two are
       related but not derived: a node may be skipped without its verdict
       having been computed, and a runner that recomputed the result from a
       status would invent a verdict for a test it never ran. */
    return D_TEST_OBJECT_OK;
}


enum d_test_object_result
d_test_object_set_type_id(
    struct d_test_object* _object,
    d_test_type_id        _type_id
)
{
    if (_object == (struct d_test_object*)0)
    {
        return D_TEST_OBJECT_INVALID;
    }

    _object->type_id = _type_id;

    return D_TEST_OBJECT_OK;
}


enum d_test_object_result
d_test_object_set_callable_id(
    struct d_test_object* _object,
    d_test_callable_id    _id
)
{
    if (_object == (struct d_test_object*)0)
    {
        return D_TEST_OBJECT_INVALID;
    }

    _object->callable_id = _id;

    return D_TEST_OBJECT_OK;
}


bool
d_test_object_result_of(
    const struct d_test_object* _object
)
{
    return (_object != (const struct d_test_object*)0)
           ? _object->result : false;
}


int32_t
d_test_object_status(
    const struct d_test_object* _object
)
{
    return (_object != (const struct d_test_object*)0)
           ? _object->status : (int32_t)D_TEST_STATUS_ERROR;
}


int
d_test_object_passed(
    const struct d_test_object* _object
)
{
    return (_object != (const struct d_test_object*)0) &&
           (_object->status == (int32_t)D_TEST_STATUS_PASSED);
}


int
d_test_object_is_deferred(
    const struct d_test_object* _object
)
{
    return (_object != (const struct d_test_object*)0) &&
           (_object->callable_id != D_TEST_NO_CALLABLE);
}


const char*
d_test_object_result_name(
    enum d_test_object_result _result
)
{
    switch (_result)
    {
        case D_TEST_OBJECT_OK:       return "ok";
        case D_TEST_OBJECT_FULL:     return "full";
        case D_TEST_OBJECT_INVALID:  return "invalid";
        case D_TEST_OBJECT_REPLACED: return "replaced";
        default:                     break;
    }

    return "unknown";
}


/* =============================================================================
   III. CONSTRUCTION HELPERS
   ============================================================================= */

struct d_test_object
d_test_make(
    d_test_type_id _type_id,
    int            _result
)
{
    struct d_test_object o;

    d_test_object_init(&o);
    o.type_id = _type_id;
    d_test_object_evaluate(&o, _result);

    return o;
}


struct d_test_object
d_test_make_interior(
    d_test_type_id _type_id
)
{
    struct d_test_object o;

    d_test_object_init(&o);
    o.type_id = _type_id;

    /*   Interior nodes stay PENDING.  Their status is a fold over their
       children, computed by the walk, and stamping one here would be the node
       claiming a fact of position -- the thing this module does not do. */
    return o;
}


D_EXTERN_C_END
