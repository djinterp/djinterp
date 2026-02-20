#include "..\..\inc\functional\predicate.h"


/*
d_predicate_and_new
  Creates a `d_predicate_and` combinator from two predicates and their
associated contexts.

Parameter(s):
  _predicate1: the first predicate function.
  _context1:   context for the first predicate; may be NULL.
  _predicate2: the second predicate function.
  _context2:   context for the second predicate; may be NULL.
Return:
  A `d_predicate_and` struct initialized with the given predicates and
contexts.
*/
struct d_predicate_and*
d_predicate_and_new
(
    fn_predicate _predicate1,
    void*       _context1,
    fn_predicate _predicate2,
    void*       _context2
)
{
    struct d_predicate_and* new_predicate_and = 
        malloc(sizeof(struct d_predicate_and));

    // ensure that memory allocation was successful
    if (!new_predicate_and)
    {
        return NULL;
    }

    new_predicate_and->predicate1 = _predicate1;
    new_predicate_and->predicate2 = _predicate2;
    new_predicate_and->context1   = _context1;
    new_predicate_and->context2   = _context2;

    return new_predicate_and;
}

/*
d_predicate_or_new
  Creates a `d_predicate_or` combinator from two predicates and their
associated contexts.

Parameter(s):
  _predicate1: the first predicate function.
  _context1:   context for the first predicate; may be NULL.
  _predicate2: the second predicate function.
  _context2:   context for the second predicate; may be NULL.
Return:
  A `d_predicate_or` struct initialized with the given predicates and
contexts.
*/
struct d_predicate_or*
d_predicate_or_new
(
    fn_predicate _predicate1,
    void*       _context1,
    fn_predicate _predicate2,
    void*       _context2
)
{
    struct d_predicate_or* new_predicate_or =
        malloc(sizeof(struct d_predicate_or));

    // ensure that memory allocation was successful
    if (!new_predicate_or)
    {
        return NULL;
    }
    
    new_predicate_or->predicate1 = _predicate1;
    new_predicate_or->predicate2 = _predicate2;
    new_predicate_or->context1   = _context1;
    new_predicate_or->context2   = _context2;

    return new_predicate_or;
}

/*
d_predicate_xor_new
  Creates a `d_predicate_xor` combinator from two predicates and their
associated contexts.

Parameter(s):
  _predicate1: the first predicate function.
  _context1:   context for the first predicate; may be NULL.
  _predicate2: the second predicate function.
  _context2:   context for the second predicate; may be NULL.
Return:
  A `d_predicate_xor` struct initialized with the given predicates and
contexts.
*/
struct d_predicate_xor*
d_predicate_xor_new
(
    fn_predicate _predicate1,
    void*       _context1,
    fn_predicate _predicate2,
    void*       _context2
)
{
    struct d_predicate_xor* new_predicate_xor =
        malloc(sizeof(struct d_predicate_xor));

    // ensure that memory allocation was successful
    if (!new_predicate_xor)
    {
        return NULL;
    }

    new_predicate_xor->predicate1 = _predicate1;
    new_predicate_xor->predicate2 = _predicate2;
    new_predicate_xor->context1   = _context1;
    new_predicate_xor->context2   = _context2;

    return new_predicate_xor;
}

/*
d_predicate_not_new
  Creates a `d_predicate_not` combinator that negates the given predicate.

Parameter(s):
  _predicate1: the predicate function to negate.
  _context1:   context for the predicate; may be NULL.
Return:
  A `d_predicate_not` struct initialized with the given predicate and context.
*/
struct d_predicate_not*
d_predicate_not_new
(
    fn_predicate _predicate,
    void*       _context
)
{
    struct d_predicate_not* new_predicate_not =
        malloc(sizeof(struct d_predicate_not));

    // ensure that memory allocation was successful
    if (!new_predicate_not)
    {
        return NULL;
    }

    new_predicate_not->predicate = _predicate;
    new_predicate_not->context   = _context;

    return new_predicate_not;
}

/*
d_predicate_and_eval
  Evaluates an AND combinator against an element. Short-circuits: if the
first predicate returns false, the second is not evaluated.

Parameter(s):
  _combo:   pointer to the AND combinator to evaluate.
  _element: pointer to the element to test.
Return:
  A boolean value corresponding to either:
  - true, if both predicates returned true for the element, or
  - false, if _combo was NULL, _combo->pred1 was NULL, _combo->pred2 was
    NULL, or either predicate returned false.
*/
D_INLINE bool
d_predicate_and_eval
(
    const struct d_predicate_and* _combo,
    const void*                   _element
)
{
    // validate the combinator and its predicates
    return ( (_combo)             &&
             (_combo->predicate1) &&
             (_combo->predicate2) )
        ? ( _combo->predicate1(_element, _combo->context1) &&
            _combo->predicate2(_element, _combo->context2) )
        : false;
}

/*
d_predicate_or_eval
  Evaluates an OR combinator against an element. Short-circuits: if the
first predicate returns true, the second is not evaluated.

Parameter(s):
  _combo:   pointer to the OR combinator to evaluate.
  _element: pointer to the element to test.
Return:
  A boolean value corresponding to either:
  - true, if at least one predicate returned true for the element, or
  - false, if _combo was NULL, _combo->pred1 was NULL, _combo->pred2 was
    NULL, or both predicates returned false.
*/
D_INLINE bool
d_predicate_or_eval
(
    const struct d_predicate_or* _combo,
    const void*                  _element
)
{
    // validate the combinator and its predicates
    return ( (_combo)             &&
             (_combo->predicate1) &&
             (_combo->predicate2) )
        ? ( _combo->predicate1(_element, _combo->context1) ||
            _combo->predicate2(_element, _combo->context2) )
        : false;
}

/*
d_predicate_xor_eval
  Evaluates an XOR combinator against an element. Both predicates are always
evaluated.

Parameter(s):
  _combo:   pointer to the XOR combinator to evaluate.
  _element: pointer to the element to test.
Return:
  A boolean value corresponding to either:
  - true, if exactly one predicate returned true for the element, or
  - false, if _combo was NULL, _combo->pred1 was NULL, _combo->pred2 was
    NULL, or both predicates returned the same value.
*/
D_INLINE bool
d_predicate_xor_eval
(
    const struct d_predicate_xor* _combo,
    const void*                   _element
)
{
    // validate the combinator and its predicates
    return ( (_combo)             &&
             (_combo->predicate1) &&
             (_combo->predicate2) )
        ? ( _combo->predicate1(_element, _combo->context1) ^
            _combo->predicate2(_element, _combo->context2) )
        : false;
}

/*
d_predicate_not_eval
  Evaluates a NOT combinator against an element, returning the logical
negation of the underlying predicate.

Parameter(s):
  _combo:   pointer to the NOT combinator to evaluate.
  _element: pointer to the element to test.
Return:
  A boolean value corresponding to either:
  - true, if the underlying predicate returned false for the element, or
  - false, if _combo was NULL, _combo->pred was NULL, or the underlying
    predicate returned true.
*/
D_INLINE bool
d_predicate_not_eval
(
    const struct d_predicate_not* _combo,
    const void*                   _element
)
{
    // validate the combinator and its predicate
    return ( (_combo) &&
             (_combo->predicate) )
        ? (!(_combo->predicate(_element, _combo->context)))
        : false;
}

/*
d_predicate_is_null
  Tests whether an element pointer is NULL.

Parameter(s):
  _element: the element to test.
Return:
  A boolean value corresponding to either:
  - true, if the element is NULL, or
  - false, if the element is non-NULL.
*/
D_INLINE bool
d_predicate_is_null
(
    const void* _element
)
{
    return (_element == NULL);
}

/*
d_predicate_is_not_null
  Tests whether an element pointer is non-NULL.

Parameter(s):
  _element: the element to test.
Return:
  A boolean value corresponding to either:
  - true, if the element is non-NULL, or
  - false, if the element is NULL.
*/
D_INLINE bool
d_predicate_is_not_null
(
    const void* _element
)
{
    return (_element != NULL);
}