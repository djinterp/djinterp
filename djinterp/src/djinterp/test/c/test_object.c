/*******************************************************************************
* djinterp [test]                                                  test_object.c
*
*   Implementation of the shared node kernel.  Compiled by both languages from
* this one source.
*
*
* path:      /src/djinterp/test/c/test_object.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.31
*                                                            revised: 2026.09.30
*******************************************************************************/

#include "../../../../inc/djinterp/test/c/test_object.h"

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

    _object->status      = (d_test_status)D_TEST_STATUS_PENDING;
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

    /*   The verdict is not stored.  It went in beside the status until the
       two proved never to be independent, and it is now read back out of the
       status by d_test_object_result_of. */
    _object->status = (_result != 0)
                      ? (d_test_status)D_TEST_STATUS_PASSED
                      : (d_test_status)D_TEST_STATUS_FAILED;

    return D_TEST_OBJECT_OK;
}


enum d_test_object_result
d_test_object_set_status(
    struct d_test_object* _object,
    d_test_status         _status
)
{
    if (_object == (struct d_test_object*)0)
    {
        return D_TEST_OBJECT_INVALID;
    }

#if D_INTERNAL_TEST_OBJECT_VALIDATE_STATUS
    _object->status = d_test_status_is_valid(_status)
                      ? _status : (d_test_status)D_TEST_STATUS_ERROR;
#else
    _object->status = _status;
#endif

    /*   THIS COMMENT USED TO DEFEND A SECOND MEMBER.  It read that setting the
       status must not rewrite the boolean result, because "a runner that
       recomputed the result from a status would invent a verdict for a test it
       never ran".  The concern was real and the conclusion was backwards: not
       having a verdict and inventing one are different things, and the member
       it protected held a STALE verdict rather than an independent one.  The
       status now carries the outcome alone, and a test that never ran reports
       pending, skipped or error -- none of which read as passed. */
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
    /*   DERIVED, NOT STORED.  `passed` is the only status that asserts a true
       verdict: failed is false by definition, and pending, skipped and error
       are all states in which no verdict was reached -- which this returns as
       false because the function's contract is "did this test pass", not "is
       this test's outcome known".  A caller that needs to tell "ran and
       disagreed" from "never ran" reads the status, which is why the status is
       what the node keeps.  Null yields false for the same reason. */
    return (_object != (const struct d_test_object*)0) &&
           (_object->status == (d_test_status)D_TEST_STATUS_PASSED);
}


d_test_status
d_test_object_status(
    const struct d_test_object* _object
)
{
    return (_object != (const struct d_test_object*)0)
           ? _object->status : (d_test_status)D_TEST_STATUS_ERROR;
}


int
d_test_object_passed(
    const struct d_test_object* _object
)
{
    return (_object != (const struct d_test_object*)0) &&
           (_object->status == (d_test_status)D_TEST_STATUS_PASSED);
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
