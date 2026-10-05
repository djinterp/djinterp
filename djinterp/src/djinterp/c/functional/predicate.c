/*******************************************************************************
* djinterp [c]                                                       predicate.c
*
* The predicate combinators predicate.h declares.
*   Each _new allocates its combinator, which the caller releases with free();
* each _eval short-circuits as C's own operators do. A missing combinator
* evaluates to false, and _new refuses a missing predicate by returning NULL.
*
*
* path:      /src/djinterp/c/functional/predicate.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/predicate.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdlib.h>   // malloc


struct d_predicate_and*
d_predicate_and_new(
    fn_predicate _predicate1,
    void*        _context1,
    fn_predicate _predicate2,
    void*        _context2
)
{
    // both operands are required
    if ( (!_predicate1) ||
         (!_predicate2) )
    {
        return NULL;
    }

    struct d_predicate_and* const combo = malloc(sizeof(*combo));

    if (combo)
    {
        combo->predicate1 = _predicate1;
        combo->predicate2 = _predicate2;
        combo->context1   = _context1;
        combo->context2   = _context2;
    }

    return combo;
}

bool
d_predicate_and_eval(
    const struct d_predicate_and* _combo,
    const void*                   _element
)
{
    return ( (_combo != NULL)                                      &&
             (_combo->predicate1(_element, _combo->context1))      &&
             (_combo->predicate2(_element, _combo->context2)) );
}

struct d_predicate_or*
d_predicate_or_new(
    fn_predicate _predicate1,
    void*        _context1,
    fn_predicate _predicate2,
    void*        _context2
)
{
    // both operands are required
    if ( (!_predicate1) ||
         (!_predicate2) )
    {
        return NULL;
    }

    struct d_predicate_or* const combo = malloc(sizeof(*combo));

    if (combo)
    {
        combo->predicate1 = _predicate1;
        combo->predicate2 = _predicate2;
        combo->context1   = _context1;
        combo->context2   = _context2;
    }

    return combo;
}

bool
d_predicate_or_eval(
    const struct d_predicate_or* _combo,
    const void*                  _element
)
{
    return ( (_combo != NULL) &&
             ( (_combo->predicate1(_element, _combo->context1)) ||
               (_combo->predicate2(_element, _combo->context2)) ) );
}

struct d_predicate_xor*
d_predicate_xor_new(
    fn_predicate _predicate1,
    void*        _context1,
    fn_predicate _predicate2,
    void*        _context2
)
{
    // both operands are required
    if ( (!_predicate1) ||
         (!_predicate2) )
    {
        return NULL;
    }

    struct d_predicate_xor* const combo = malloc(sizeof(*combo));

    if (combo)
    {
        combo->predicate1 = _predicate1;
        combo->predicate2 = _predicate2;
        combo->context1   = _context1;
        combo->context2   = _context2;
    }

    return combo;
}

/*
d_predicate_xor_eval
  Exclusive or has nothing to short-circuit: both operands always run.
*/
bool
d_predicate_xor_eval(
    const struct d_predicate_xor* _combo,
    const void*                   _element
)
{
    // no combinator: false
    if (!_combo)
    {
        return false;
    }

    const bool first  = _combo->predicate1(_element, _combo->context1);
    const bool second = _combo->predicate2(_element, _combo->context2);

    return (first != second);
}

struct d_predicate_not*
d_predicate_not_new(
    fn_predicate _predicate,
    void*        _context
)
{
    // the operand is required
    if (!_predicate)
    {
        return NULL;
    }

    struct d_predicate_not* const combo = malloc(sizeof(*combo));

    if (combo)
    {
        combo->predicate = _predicate;
        combo->context   = _context;
    }

    return combo;
}

bool
d_predicate_not_eval(
    const struct d_predicate_not* _combo,
    const void*                   _element
)
{
    return ( (_combo != NULL) &&
             (!_combo->predicate(_element, _combo->context)) );
}
