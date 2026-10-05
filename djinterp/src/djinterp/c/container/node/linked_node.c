/*******************************************************************************
* djinterp [c]                                                     linked_node.c
*
* TBA
*
*
* path:      /src/djinterp/c/container/node/linked_node.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../../inc/djinterp/c/container/node/linked_node.h"


/*
d_linked_node_new
  Allocates and initializes a new `d_linked_node`, a node with a value and a
pointer to the next node.

Parameter(s):
  _value:     the value held by this node
  _next_node: the next node in this sequence of `d_linked_node`s.
Return:
  Either: a pointer to the newly-created `d_linked_node` (if successful);
otherwise NULL if not.
*/
struct d_linked_node*
d_linked_node_new
(
    void*                 _value,
    struct d_linked_node* _next_node
)
{
    struct d_linked_node* new_node = malloc(sizeof(struct d_linked_node));

    if (new_node)
    {
        new_node->value = _value;
        new_node->next  = _next_node;
    }

    return new_node;
}

/*
d_linked_node_free
  Frees the memory allocated this d_linked_node.

Parameter(s):
  _node: the d_linked_node being freed.
Return:
  none.
*/
void
d_linked_node_free
(
    struct d_linked_node* _node
)
{
    if (_node)
    {
        free(_node);
    }

    return;
}
