#include "../../../inc/c/test/test_tree.h"


/*
d_test_tree_node_new
  Allocates and initializes a d_test_tree_node.
Note: This function initializes the node links as provided. It does not
automatically splice the node into its parent's child list.

Parameter(s):
  _type:              node type / token / tag id.
  _context:           payload / per-type data; may be NULL.
  _parent_node:       parent node; may be NULL.
  _first_child_node:  first child node; may be NULL.
  _last_child_node:   last child node; may be NULL.
  _next_sibling_node: next sibling node; may be NULL.
Return:
  A newly allocated node on success; NULL on allocation failure.
*/
struct d_test_tree_node*
d_test_tree_node_new
(
    int32_t                  _type,
    void*                    _context,
    struct d_test_tree_node* _parent_node,
    struct d_test_tree_node* _first_child_node,
    struct d_test_tree_node* _last_child_node,
    struct d_test_tree_node* _next_sibling_node
)
{
    struct d_test_tree_node* result;
    struct d_test_tree_node* child;

    // normalize child list endpoints (allow callers to pass only one endpoint)
    if (!_first_child_node)
    {
        _last_child_node = NULL;
    }
    else if (!_last_child_node)
    {
        _last_child_node = _first_child_node;
    }

    result = NULL;
    child  = NULL;

    // allocate the node
    result = malloc(sizeof(struct d_test_tree_node));

    // check if memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    // initialize members
    result->type         = _type;
    result->context      = _context;
    result->parent       = _parent_node;
    result->first_child  = _first_child_node;
    result->last_child   = _last_child_node;
    result->next_sibling = _next_sibling_node;

    // set parent pointers for the provided child list (if any)
    if (_first_child_node)
    {
        child = _first_child_node;

        while (child)
        {
            child->parent = result;

            if (child == _last_child_node)
            {
                break;
            }

            child = child->next_sibling;
        }
    }

    return result;
}

/*
d_test_tree_new
  Allocates and initializes an empty d_test_tree.

Return:
  A newly allocated test tree on success; NULL on allocation failure.
*/
struct d_test_tree*
d_test_tree_new
(
    void
)
{
    struct d_test_tree* result;

    // allocate the tree
    result = malloc(sizeof(struct d_test_tree));

    // check if memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    // initialize members
    result->root = NULL;

    return result;
}

/*
d_test_tree_new_arr
  Allocates a new d_test_tree by deep-copying an array of d_test_tree_node
  templates into individually heap-allocated nodes, remapping internal pointers.
Note: All pointer fields in `_test_tree_nodes` must either be NULL or point to an
element within the `_test_tree_nodes[0.._count-1]` array.

Parameter(s):
  _test_tree_nodes: array of node templates describing a single connected tree.
  _count:           number of nodes in the array; must be > 0.
Return:
  A newly allocated tree on success; NULL on failure.
*/
struct d_test_tree*
    d_test_tree_new_arr
    (
        struct d_test_tree_node* _test_tree_nodes,
        size_t                   _count
    )
{
    struct d_test_tree* result;
    struct d_test_tree_node** new_nodes;
    size_t                     i;
    size_t                     j;
    size_t                     root_index;
    int                        found;

    result = NULL;
    new_nodes = NULL;
    i = 0;
    j = 0;
    root_index = 0;
    found = 0;

    // validate parameters
    if ((!_test_tree_nodes) ||
        (_count == 0))
    {
        return NULL;
    }

    // allocate the tree
    result = (struct d_test_tree*)malloc(sizeof(struct d_test_tree));

    // check if memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    // initialize members
    result->root = NULL;

    // allocate pointer table for newly allocated nodes
    new_nodes = (struct d_test_tree_node**)malloc(_count * sizeof(struct d_test_tree_node*));

    // check if memory allocation was successful
    if (!new_nodes)
    {
        free(result);

        return NULL;
    }

    // initialize the pointer table
    for (i = 0; i < _count; i++)
    {
        new_nodes[i] = NULL;
    }

    // allocate each node
    for (i = 0; i < _count; i++)
    {
        new_nodes[i] = (struct d_test_tree_node*)malloc(sizeof(struct d_test_tree_node));

        // check if memory allocation was successful
        if (!new_nodes[i])
        {
            for (j = 0; j < _count; j++)
            {
                if (new_nodes[j])
                {
                    free(new_nodes[j]);
                }
            }

            free(new_nodes);
            free(result);

            return NULL;
        }
    }

    // copy scalar fields first
    for (i = 0; i < _count; i++)
    {
        new_nodes[i]->type = _test_tree_nodes[i].type;
        new_nodes[i]->context = _test_tree_nodes[i].context;
        new_nodes[i]->parent = NULL;
        new_nodes[i]->first_child = NULL;
        new_nodes[i]->last_child = NULL;
        new_nodes[i]->next_sibling = NULL;
    }

    // helper: remap a pointer by searching for it in the source array
    // remap links (and reject any pointer that is not within the source array)
    for (i = 0; i < _count; i++)
    {
        // parent
        if (_test_tree_nodes[i].parent)
        {
            found = 0;

            for (j = 0; j < _count; j++)
            {
                if (_test_tree_nodes[i].parent == &_test_tree_nodes[j])
                {
                    new_nodes[i]->parent = new_nodes[j];
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                for (j = 0; j < _count; j++)
                {
                    free(new_nodes[j]);
                }

                free(new_nodes);
                free(result);

                return NULL;
            }
        }

        // first_child
        if (_test_tree_nodes[i].first_child)
        {
            found = 0;

            for (j = 0; j < _count; j++)
            {
                if (_test_tree_nodes[i].first_child == &_test_tree_nodes[j])
                {
                    new_nodes[i]->first_child = new_nodes[j];
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                for (j = 0; j < _count; j++)
                {
                    free(new_nodes[j]);
                }

                free(new_nodes);
                free(result);

                return NULL;
            }
        }

        // last_child
        if (_test_tree_nodes[i].last_child)
        {
            found = 0;

            for (j = 0; j < _count; j++)
            {
                if (_test_tree_nodes[i].last_child == &_test_tree_nodes[j])
                {
                    new_nodes[i]->last_child = new_nodes[j];
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                for (j = 0; j < _count; j++)
                {
                    free(new_nodes[j]);
                }

                free(new_nodes);
                free(result);

                return NULL;
            }
        }

        // next_sibling
        if (_test_tree_nodes[i].next_sibling)
        {
            found = 0;

            for (j = 0; j < _count; j++)
            {
                if (_test_tree_nodes[i].next_sibling == &_test_tree_nodes[j])
                {
                    new_nodes[i]->next_sibling = new_nodes[j];
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                for (j = 0; j < _count; j++)
                {
                    free(new_nodes[j]);
                }

                free(new_nodes);
                free(result);

                return NULL;
            }
        }
    }

    // choose a root: first node with NULL parent, else index 0
    root_index = 0;

    for (i = 0; i < _count; i++)
    {
        if (!_test_tree_nodes[i].parent)
        {
            root_index = i;
            break;
        }
    }

    result->root = new_nodes[root_index];

    free(new_nodes);

    return result;
}

/*
d_test_tree_node_clear_subtree
  Frees all descendants of the given node, but does not free the node itself.
Note: Does not free any per-node `context` payloads.

Parameter(s):
  _test_tree_node: the node whose subtree (children) will be freed; may be NULL.
Return:
  none.
*/
void
d_test_tree_node_clear_subtree
(
    struct d_test_tree_node* _test_tree_node
)
{
    struct d_test_tree_node* child;
    struct d_test_tree_node* next_sibling;

    // validate parameters
    if (!_test_tree_node)
    {
        return;
    }

    child = NULL;
    next_sibling = NULL;

    // free all children (and their descendants)
    child = _test_tree_node->first_child;

    while (child)
    {
        next_sibling = child->next_sibling;

        d_test_tree_node_free(child);

        child = next_sibling;
    }

    // clear child list
    _test_tree_node->first_child = NULL;
    _test_tree_node->last_child = NULL;

    return;
}


/*
d_test_tree_clear
  Frees all nodes in the tree, leaving the tree object allocated but empty.
Note: Does not free any per-node `context` payloads.

Parameter(s):
  _test_tree: the tree to clear; may be NULL.
Return:
  none.
*/
void
d_test_tree_clear
(
    struct d_test_tree* _test_tree
)
{
    // validate parameters
    if (!_test_tree)
    {
        return;
    }

    if (_test_tree->root)
    {
        d_test_tree_node_free(_test_tree->root);

        _test_tree->root = NULL;
    }

    return;
}


/*
d_test_tree_node_free
  Frees a d_test_tree_node and all of its descendants.
Note: Does not free the node's `context` payload.

Parameter(s):
  _test_tree_node: the node to free; may be NULL.
Return:
  none.
*/
void
d_test_tree_node_free
(
    struct d_test_tree_node* _test_tree_node
)
{
    struct d_test_tree_node* child;
    struct d_test_tree_node* next_sibling;

    if (!_test_tree_node)
    {
        return;
    }

    child = _test_tree_node->first_child;

    while (child)
    {
        next_sibling = child->next_sibling;

        d_test_tree_node_free(child);

        child = next_sibling;
    }

    free(_test_tree_node);

    return;
}

/*
d_test_tree_free
  Frees a d_test_tree and all of its nodes.
Note: Does not free any per-node `context` payloads.

Parameter(s):
  _test_tree: the tree to free; may be NULL.
Return:
  none.
*/
void
d_test_tree_free
(
    struct d_test_tree* _test_tree
)
{
    if (_test_tree)
    {
        if (_test_tree->root)
        {
            d_test_tree_node_free(_test_tree->root);
        }

        free(_test_tree);
    }

    return;
}