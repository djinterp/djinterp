/*******************************************************************************
* djinterp [c]                                                       min_stack.c
*
* TBA
*
*
* path:      /src/djinterp/c/container/stack/min_stack.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../../inc/djinterp/c/container/stack/min_stack.h"


/*
d_min_stack_new
  Allocates and initializes a new (empty) `d_min_stack`.
  A min-stack (minimal stack) is a LIFO (last in, first out) stack data
structure optimized to occupy minimal space.

Parameter(s):
  none
Return:
  A `struct d_min_stack` pointer corresponding to either:
  1. an address in memory corresponding to the newly-initialized `struct
     d_min_stack`, if memory allocation was successful, or
  2. NULL, if memory allocation was unsuccessful.
*/
struct d_min_stack*
d_min_stack_new
(
    void
)
{
    struct d_min_stack* new_min_stack = malloc(sizeof(struct d_min_stack));

    // ensure that memory allocation was successful
    if (new_min_stack)
    {
        new_min_stack->top = NULL;
    }

    return new_min_stack;
}

/*
d_min_stack_clear
  Removes all elements from the stack, freeing all associated nodes.
  After this operation, the stack will be empty but still valid for use.

Parameter(s):
  _min_stack: the stack to be cleared
Return:
  none.
*/
void
d_min_stack_clear
(
    struct d_min_stack* _min_stack
)
{
    struct d_linked_node* node;
    struct d_linked_node* next;

    if (_min_stack)
    {
        node = _min_stack->top;

        while (node)
        {
            // save next node
            next = node->next;

            // free current node
            d_linked_node_free(node);

            // current node is now whichever node was next
            node = next;
        }

        // reset top to NULL after clearing all nodes
        _min_stack->top = NULL;
    }

    return;
}

/*
d_min_stack_peek
  Views the top element on a stack without removing it.
  The counterpart to the pop operation is push; push will insert an element
at the top of the stack. Push and pop operate according to the LIFO (last-in,
first-out) principle.

Parameter(s):
  _min_stack: the stack whose top element is being returned
Return:
  a pointer to the value on the top of the stack, or NULL if stack is empty or NULL
*/
void*
d_min_stack_peek
(
    struct d_min_stack* _min_stack
)
{
    return ( (_min_stack) && (_min_stack->top) )
           ? _min_stack->top->value
           : NULL;
}

/*
d_min_stack_pop
  Removes the element at top of the given `d_min_stack` and returns its value.

Parameter(s):
  _min_stack: the `d_min_stack` being modified by this operation by having the
              top element removed.
Return:
  A `void` pointer corresponding to either:
  - a memory address pointing to the value assigned to the top-most node that
    was removed by this operation (if `_min_stack` was non-NULL and non-empty),
    or
  - NULL, if any of the following is true:
    1. the given `d_min_stack` was NULL, or
    2. the given `d_min_stack` was empty, or
    3. the given `d_min_stack` was non-NULL and non-empty, but the value of
       the node removed was itself NULL.
*/
void*
d_min_stack_pop
(
    struct d_min_stack* _min_stack
)
{
    struct d_linked_node* node;
    void* value;

    // required that parameter is a non-NULL reference to a non-empty min stack
    if ( (!_min_stack) ||
         (!_min_stack->top) )
    {
        return NULL;
    }

    // store node
    node = _min_stack->top;

    // store node's value
    value = node->value;

    // remove node from stack, making top node the node below it
    _min_stack->top = node->next;

    // free memory associated with redundant node
    d_linked_node_free(node);

    return value;
}

/*
d_min_stack_push
  Adds an element to the top of the given `d_min_stack`.

Parameter(s):
  _value:     a pointer to a value being added to the top of the `d_min_stack`.
  _min_stack: the `d_min_stack` being modified by this operation.
Return:
  A `void` pointer corresponding to either:
  1. the memory address pointed to by `_value`, if the `push` operation was
     successful, or
  2. NULL, if memory allocation was unsuccessful.
*/
void*
d_min_stack_push
(
    struct d_min_stack* _min_stack,
    void*               _value
)
{
    struct d_linked_node* new_node;

    // _min_stack cannot be null
    if (!_min_stack)
    {
        return NULL;
    }

    //   attempt to allocate and initialize a new `d_linked_node` with the
    // value provided and linked to the current top
    new_node = d_linked_node_new(_value, _min_stack->top);

    // ensure allocation of new `d_linked_node` allocation was successful
    if (!new_node)
    {
        return NULL;
    }

    // make the top of `_min_stack` the new node, linked to the previous top
    _min_stack->top = new_node;

    // return the original value
    return _value;
}

/*
d_min_stack_free
  Frees the space allocated to the `d_min_stack` and all of the
`d_linked_node`(s) associated with it.

Parameter(s):
  _min_stack: the `d_min_stack` being freed.
Return:
  none.
*/
void
d_min_stack_free
(
    struct d_min_stack* _min_stack
)
{
    if (_min_stack)
    {
        // free all nodes
        d_min_stack_clear(_min_stack);

        // free struct itself
        free(_min_stack);
    }

    return;
}
