/******************************************************************************
* djinterp [test]                                          test_standalone_min.c
*
*   The d_test_object model from test_standalone.h, plus a driver, so the
* standalone fs suite can be RUN.
*
*   RECONSTRUCTED, AND DELIBERATELY MINIMAL. test_standalone.h declares a much
* larger framework -- printing hooks, output templates, section headers,
* counters. None of that is implemented here, because the fs suite does not
* call any of it: linking the suite leaves exactly two symbols undefined,
* d_test_object_new_leaf and d_test_object_new_interior. This file supplies the
* four object functions and a main, and nothing else.
*
*   WHAT IS AND IS NOT MINE. Every verdict this prints is the suite's: the
* assertions are evaluated in the suite's own code against the real library,
* and these constructors only record the bool they are handed. What is mine is
* the PRESENTATION -- the walk, the tallies, the layout. So a failure reported
* here is a real failure; a formatting difference from your runner is not.
* Replace this file with the real test_standalone.c and the numbers should not
* move.
*
*   ONE DELIBERATE DIFFERENCE: names and messages are copied rather than
* aliased. The struct stores const char*, which reads as "the caller's strings
* outlive the tree" -- true for literals, false the moment a test builds a name
* in a local buffer. Copying is correct under both readings, and costs a few
* hundred small allocations in a run.
*
*
* TABLE OF CONTENTS
* =================
* I.    STRING COPY
* II.   OBJECT MODEL      new_leaf / new_interior / add_child / free
* III.  WALK              tally + render
* IV.   DRIVER            main
*
*
* path:      /src/test/test_standalone_min.c
* link(s):   TBA
* author(s): reconstructed
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../inc/djinterp/c/test/test_standalone.h"
#include "./fs/file_tests_sa.h"   // d_tests_file_run_all


///////////////////////////////////////////////////////////////////////////////
///             I.  STRING COPY                                             ///
///////////////////////////////////////////////////////////////////////////////

/*
d_internal_test_strdup
  Copies a string for a test object to own. NULL in, NULL out.
  An allocation failure yields NULL rather than aborting: a missing label is a
cosmetic loss, and taking the whole run down over one would turn a reporting
problem into a testing one.

Parameter(s):
  _text: the string to copy, or NULL.
Return:
  An owned copy, or NULL.
*/
static char*
d_internal_test_strdup
(
    const char* _text
)
{
    size_t len;
    char*  copy;

    if (!_text)
    {
        return NULL;
    }

    len  = strlen(_text);
    copy = (char*)malloc(len + 1);

    if (!copy)
    {
        return NULL;
    }

    memcpy(copy, _text, len + 1);

    return copy;
}


///////////////////////////////////////////////////////////////////////////////
///             II.  OBJECT MODEL                                           ///
///////////////////////////////////////////////////////////////////////////////

/*
d_test_object_new_leaf
  Creates one assertion: a name, a message, and the bool that was evaluated.
  The bool is recorded, not judged. There is no third state -- the suite's own
discipline is that a check it cannot run is OMITTED rather than passed -- so
this stores exactly what it was given.

Parameter(s):
  _name:    the assertion's short name.
  _message: what the assertion claims.
  _result:  the evaluated condition.
Return:
  A new leaf, or NULL on allocation failure.
*/
struct d_test_object*
d_test_object_new_leaf
(
    const char* _name,
    const char* _message,
    bool        _result
)
{
    struct d_test_object* obj;

    obj = (struct d_test_object*)malloc(sizeof(struct d_test_object));

    if (!obj)
    {
        return NULL;
    }

    obj->is_leaf  = true;
    obj->name     = d_internal_test_strdup(_name);
    obj->message  = d_internal_test_strdup(_message);
    obj->result   = _result;
    obj->args     = NULL;
    obj->count    = 0;
    obj->elements = NULL;

    return obj;
}

/*
d_test_object_new_interior
  Creates a group with room for _child_count children.
  The child array is zero-filled, so a slot the caller never fills stays NULL
and the walk skips it instead of following a wild pointer. That matters
because the count is declared up front here: it is the caller's promise, not a
fact, and the two can disagree.

Parameter(s):
  _name:        the group's name.
  _child_count: how many children the caller intends to add.
Return:
  A new interior node, or NULL on allocation failure. A count of 0 is legal
and yields a group with no child array.
*/
struct d_test_object*
d_test_object_new_interior
(
    const char* _name,
    size_t      _child_count
)
{
    struct d_test_object* obj;

    obj = (struct d_test_object*)malloc(sizeof(struct d_test_object));

    if (!obj)
    {
        return NULL;
    }

    obj->is_leaf  = false;
    obj->name     = d_internal_test_strdup(_name);
    obj->message  = NULL;
    obj->result   = true;
    obj->args     = NULL;
    obj->count    = _child_count;
    obj->elements = NULL;

    if (_child_count > 0)
    {
        obj->elements = (struct d_test_object**)calloc(
            _child_count, sizeof(struct d_test_object*));

        if (!obj->elements)
        {
            free((void*)obj->name);
            free(obj);

            return NULL;
        }
    }

    return obj;
}

/*
d_test_object_add_child
  Places _child at _index in _parent's array.
  Out-of-range is ignored rather than fatal, and silently: the caller that got
its count wrong is the caller this exists to survive.

Parameter(s):
  _parent: the group to add to.
  _child:  the child to place.
  _index:  the slot.
Return:
  none.
*/
void
d_test_object_add_child
(
    struct d_test_object* _parent,
    struct d_test_object* _child,
    size_t                _index
)
{
    if ( (!_parent) || (!_parent->elements) || (_index >= _parent->count) )
    {
        return;
    }

    _parent->elements[_index] = _child;

    return;
}

/*
d_test_object_free
  Releases a tree.
  Depth-first, and NULL-tolerant at every level, because a partially filled
group is the normal shape when a module was configured out.

Parameter(s):
  _obj: the root to release. NULL is a no-op.
Return:
  none.
*/
void
d_test_object_free
(
    struct d_test_object* _obj
)
{
    size_t i;

    if (!_obj)
    {
        return;
    }

    for (i = 0; i < _obj->count; ++i)
    {
        if (_obj->elements)
        {
            d_test_object_free(_obj->elements[i]);
        }
    }

    free(_obj->elements);
    free((void*)_obj->name);
    free((void*)_obj->message);
    free(_obj);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             III.  WALK                                                  ///
///////////////////////////////////////////////////////////////////////////////

struct d_internal_tally
{
    size_t passed;
    size_t failed;
};

/*
d_internal_test_tally
  Counts the leaves under _obj.
  Only leaves are counted. An interior node's own `result` field is not
consulted, because nothing sets it -- the verdict of a group IS its leaves,
and deriving it here is the only way it cannot drift from them.

Parameter(s):
  _obj:   the subtree.
  _tally: accumulator.
Return:
  none.
*/
static void
d_internal_test_tally
(
    const struct d_test_object* _obj,
    struct d_internal_tally*    _tally
)
{
    size_t i;

    if (!_obj)
    {
        return;
    }

    if (_obj->is_leaf)
    {
        if (_obj->result)
        {
            ++_tally->passed;
        }
        else
        {
            ++_tally->failed;
        }

        return;
    }

    for (i = 0; i < _obj->count; ++i)
    {
        if (_obj->elements)
        {
            d_internal_test_tally(_obj->elements[i], _tally);
        }
    }

    return;
}

/*
d_internal_test_render
  Prints a subtree: groups indented, and every FAILING leaf named.
  Passing leaves are summarised rather than listed -- three hundred green lines
bury the one red one, which is the only line anybody is reading the output for.

Parameter(s):
  _obj:   the subtree.
  _depth: current indentation level.
Return:
  none.
*/
static void
d_internal_test_render
(
    const struct d_test_object* _obj,
    size_t                      _depth
)
{
    struct d_internal_tally tally;
    size_t                  i;

    if (!_obj)
    {
        return;
    }

    if (_obj->is_leaf)
    {
        if (!_obj->result)
        {
            printf("%*s  FAIL  %s -- %s\n",
                   (int)(_depth * 2), "",
                   _obj->name    ? _obj->name    : "(unnamed)",
                   _obj->message ? _obj->message : "");
        }

        return;
    }

    tally.passed = 0;
    tally.failed = 0;
    d_internal_test_tally(_obj, &tally);

    printf("%*s%-*s %4lu/%-4lu %s\n",
           (int)(_depth * 2), "",
           (int)(34 - (_depth * 2)),
           _obj->name ? _obj->name : "(unnamed)",
           (unsigned long)tally.passed,
           (unsigned long)(tally.passed + tally.failed),
           tally.failed ? "FAIL" : "ok");

    for (i = 0; i < _obj->count; ++i)
    {
        if (_obj->elements)
        {
            d_internal_test_render(_obj->elements[i], _depth + 1);
        }
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             IV.  DRIVER                                                 ///
///////////////////////////////////////////////////////////////////////////////

int
main
(
    int    _argc,
    char** _argv
)
{
    struct d_test_object*   root;
    struct d_internal_tally tally;

    (void)_argc;
    (void)_argv;

    root = d_tests_file_run_all();

    if (!root)
    {
        printf("suite setup failed\n");

        return 2;
    }

    tally.passed = 0;
    tally.failed = 0;
    d_internal_test_tally(root, &tally);

    printf("\n");
    d_internal_test_render(root, 0);
    printf("\n%s  %lu/%lu assertions\n",
           tally.failed ? "FAILED" : "PASS  ",
           (unsigned long)tally.passed,
           (unsigned long)(tally.passed + tally.failed));

    d_test_object_free(root);

    return tally.failed ? 1 : 0;
}
