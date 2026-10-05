/*******************************************************************************
* djinterp [c]                                                       min_stack.h
*
*   A (minimal stack) is a LIFO (last-in, first-out) data structure optimized
* to consume minimal space.
*   This operation only supports peek (check), pop (add), and push (remove)
* operations, and is in essence implemented as a singly linked-link without
* compare, search or traversal operations.
*
*
* path:      /inc/djinterp/c/container/stack/min_stack.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.04.26
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_STACK_MIN_STACK_H
#define DJINTERP_C_CONTAINER_STACK_MIN_STACK_H 1

// std
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../container.h"
#include "../node/linked_node.h"


// d_min_stack
//   struct: a bare-bones stack LIFO (last-in, first-out) data structure
// optimized to consume minimal space in memory.
struct d_min_stack
{
    struct d_linked_node* top;
};

// creation function
// I.    creation and initialization
// I.    creation and initialization
struct d_min_stack* d_min_stack_new(void);

// II.   element access
void* d_min_stack_peek(struct d_min_stack* _min_stack);

// III.  insertion and removal
void* d_min_stack_pop(struct d_min_stack* _min_stack);
void* d_min_stack_push(struct d_min_stack* _min_stack,
                       void*               _value);

// IV.   mutation and reordering
void d_min_stack_clear(struct d_min_stack* _min_stack);

// V.    destruction
void d_min_stack_free(struct d_min_stack* _min_stack);

#endif  // DJINTERP_C_CONTAINER_STACK_MIN_STACK_H
