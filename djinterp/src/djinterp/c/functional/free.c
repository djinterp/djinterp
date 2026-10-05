/*******************************************************************************
* djinterp [c]                                                            free.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/free.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/free.h"


/*
d_value_arena_init
  Initialises a bump arena for the values a map or traverse produces.

Parameter(s):
  _arena:      the arena to initialise.
  _bytes:      caller-owned storage of `_capacity * _value_size` bytes.
  _value_size: the width of one value; must be non-zero.
  _capacity:   how many values fit.
Return:
  A boolean value corresponding to whether the arena was initialised.
*/
bool
d_value_arena_init
(
    struct d_value_arena* _arena,
    void*                 _bytes,
    size_t                _value_size,
    size_t                _capacity
)
{
    // an unusable request leaves the arena untouched
    if ( (!_arena)         ||
         (!_bytes)         ||
         (_value_size == 0) )
    {
        return false;
    }

    _arena->bytes      = (unsigned char*)_bytes;
    _arena->value_size = _value_size;
    _arena->capacity   = _capacity;
    _arena->used       = 0;

    return true;
}

/*
d_value_arena_take
  Bumps one value slot off the arena.

Parameter(s):
  _arena: the arena to take from.
Return:
  A pointer to the slot, or NULL when the arena is exhausted.
*/
void*
d_value_arena_take
(
    struct d_value_arena* _arena
)
{
    unsigned char* slot;

    // an exhausted or unusable arena yields nothing
    if ( (!_arena)                        ||
         (!_arena->bytes)                 ||
         (_arena->used >= _arena->capacity) )
    {
        return NULL;
    }

    slot = _arena->bytes + (_arena->used * _arena->value_size);

    _arena->used = _arena->used + 1;

    return (void*)slot;
}

/*
d_free_arena_init
  Initialises a node arena over caller-owned storage.

Parameter(s):
  _arena:    the arena to initialise.
  _nodes:    caller-owned array of `_capacity` nodes.
  _capacity: how many nodes fit.
Return:
  A boolean value corresponding to whether the arena was initialised.
*/
bool
d_free_arena_init
(
    struct d_free_arena* _arena,
    struct d_free*       _nodes,
    size_t               _capacity
)
{
    // an unusable request leaves the arena untouched
    if ( (!_arena) ||
         (!_nodes) )
    {
        return false;
    }

    _arena->nodes    = _nodes;
    _arena->capacity = _capacity;
    _arena->used     = 0;

    return true;
}

/*
d_free_arena_rewind
  Releases every node at once. Individual nodes are never freed, so this is the
only deallocation the arena has.

Parameter(s):
  _arena: the arena to rewind; ignored if NULL.
Return:
  none.
*/
void
d_free_arena_rewind
(
    struct d_free_arena* _arena
)
{
    // a NULL arena has nothing to rewind
    if (!_arena)
    {
        return;
    }

    _arena->used = 0;

    return;
}

/*
d_internal_free_take
  Bumps one node off the arena.

Parameter(s):
  _arena: the arena to take from.
Return:
  A pointer to the node, or NULL when the arena is exhausted.
*/
static struct d_free*
d_internal_free_take
(
    struct d_free_arena* _arena
)
{
    struct d_free* node;

    // an exhausted or unusable arena yields nothing
    if ( (!_arena)                          ||
         (!_arena->nodes)                   ||
         (_arena->used >= _arena->capacity) )
    {
        return NULL;
    }

    node = &_arena->nodes[_arena->used];

    _arena->used = _arena->used + 1;

    return node;
}

/*
d_free_hole
  Builds a hole carrying a borrowed value. This is `pure` for the free monad.

Parameter(s):
  _arena: the arena to allocate from.
  _value: the borrowed value; must outlive the tree.
Return:
  The new node, or NULL when the arena is exhausted.
*/
struct d_free*
d_free_hole
(
    struct d_free_arena* _arena,
    const void*          _value
)
{
    struct d_free* node;

    node = d_internal_free_take(_arena);

    // an exhausted arena cannot hold another hole
    if (!node)
    {
        return NULL;
    }

    node->is_hole     = true;
    node->value       = _value;
    node->label       = NULL;
    node->children    = NULL;
    node->child_count = 0;

    return node;
}

/*
d_free_node
  Builds a node carrying a borrowed label and a borrowed array of children.
Both must outlive the tree; the arena owns only the node itself.

Parameter(s):
  _arena:       the arena to allocate from.
  _label:       the F's own data at this node; may be NULL.
  _children:    the sub-terms; may be NULL only if `_child_count` is 0.
  _child_count: how many sub-terms.
Return:
  The new node, or NULL when the arena is exhausted.
*/
struct d_free*
d_free_node
(
    struct d_free_arena* _arena,
    const void*          _label,
    struct d_free*       _children,
    size_t               _child_count
)
{
    struct d_free* node;

    // a node claiming children must have them
    if ( (!_children) && (_child_count > 0) )
    {
        return NULL;
    }

    node = d_internal_free_take(_arena);

    // an exhausted arena cannot hold another node
    if (!node)
    {
        return NULL;
    }

    node->is_hole     = false;
    node->value       = NULL;
    node->label       = _label;
    node->children    = _children;
    node->child_count = _child_count;

    return node;
}

/*
d_free_is_hole
  Reports whether a node is a hole.

Parameter(s):
  _tree: the node to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the node is a hole, or
  - false, if it is a node or the pointer was NULL.
*/
bool
d_free_is_hole
(
    const struct d_free* _tree
)
{
    // a NULL tree is not a hole
    if (!_tree)
    {
        return false;
    }

    return _tree->is_hole;
}

/*
d_free_size
  Counts every node in the tree, holes included.

Parameter(s):
  _tree: the tree to measure; may be NULL.
Return:
  The number of nodes, or 0 for a NULL tree.
*/
size_t
d_free_size
(
    const struct d_free* _tree
)
{
    size_t total;
    size_t index;

    // a NULL tree has no nodes
    if (!_tree)
    {
        return 0;
    }

    total = 1;

    // a hole has no children to descend into
    if (_tree->is_hole)
    {
        return total;
    }

    for (index = 0; index < _tree->child_count; index = index + 1)
    {
        total = total + d_free_size(&_tree->children[index]);
    }

    return total;
}

/*
d_free_depth
  Measures the longest root-to-leaf path, so a caller handling untrusted input
can bound the recursion the other traversals will perform.

Parameter(s):
  _tree: the tree to measure; may be NULL.
Return:
  The depth, counting the root as 1, or 0 for a NULL tree.
*/
size_t
d_free_depth
(
    const struct d_free* _tree
)
{
    size_t deepest;
    size_t candidate;
    size_t index;

    // a NULL tree has no depth
    if (!_tree)
    {
        return 0;
    }

    // a hole is a leaf
    if (_tree->is_hole)
    {
        return 1;
    }

    deepest = 0;

    for (index = 0; index < _tree->child_count; index = index + 1)
    {
        candidate = d_free_depth(&_tree->children[index]);

        if (candidate > deepest)
        {
            deepest = candidate;
        }
    }

    return deepest + 1;
}

/*
d_free_hole_count
  Counts the holes, which is how many gaps a substitution would fill.

Parameter(s):
  _tree: the tree to measure; may be NULL.
Return:
  The number of holes.
*/
size_t
d_free_hole_count
(
    const struct d_free* _tree
)
{
    size_t total;
    size_t index;

    // a NULL tree has no holes
    if (!_tree)
    {
        return 0;
    }

    // a hole is exactly one hole
    if (_tree->is_hole)
    {
        return 1;
    }

    total = 0;

    for (index = 0; index < _tree->child_count; index = index + 1)
    {
        total = total + d_free_hole_count(&_tree->children[index]);
    }

    return total;
}

/*
d_internal_free_fold
  The catamorphism proper, recursing under a bump-allocated scratch region.
Child results are laid down contiguously so the node arm receives them as an
array, and the region unwinds as the recursion returns.

Parameter(s):
  _tree:        the subtree being folded.
  _on_hole:     the hole arm.
  _on_node:     the node arm.
  _context:     context forwarded to both arms.
  _out:         destination for this subtree's result.
  _result_size: the width of a result.
  _scratch:     the bump region.
  _used:        how much of the region is taken; updated in place.
  _capacity:    the region's size in results.
Return:
  A boolean value corresponding to whether the subtree folded.
*/
static bool
d_internal_free_fold
(
    const struct d_free* _tree,
    fn_transformer       _on_hole,
    fn_free_on_node      _on_node,
    void*                _context,
    void*                _out,
    size_t               _result_size,
    unsigned char*       _scratch,
    size_t*              _used,
    size_t               _capacity
)
{
    size_t base;
    size_t index;

    // a NULL subtree cannot be folded
    if (!_tree)
    {
        return false;
    }

    // a hole is answered directly by its arm
    if (_tree->is_hole)
    {
        return _on_hole(_tree->value, _out, _context);
    }

    // the children's results are laid down from here, and released on return
    base = *_used;

    // the scratch must hold one result per child of this node
    if ((base + _tree->child_count) > _capacity)
    {
        return false;
    }

    *_used = base + _tree->child_count;

    for (index = 0; index < _tree->child_count; index = index + 1)
    {
        if (!d_internal_free_fold(&_tree->children[index],
                                  _on_hole,
                                  _on_node,
                                  _context,
                                  (void*)(_scratch +
                                          ((base + index) * _result_size)),
                                  _result_size,
                                  _scratch,
                                  _used,
                                  _capacity))
        {
            *_used = base;

            return false;
        }
    }

    if (!_on_node(_tree->label,
                  (const void*)(_scratch + (base * _result_size)),
                  _tree->child_count,
                  _result_size,
                  _out,
                  _context))
    {
        *_used = base;

        return false;
    }

    // the children's results are no longer needed
    *_used = base;

    return true;
}

/*
d_free_fold
  Folds a free tree to a single value: the catamorphism, and the operation the
carrier exists for. Every other summary here is this with different arms.

Parameter(s):
  _tree:         the tree to fold.
  _on_hole:      the hole arm.
  _on_node:      the node arm.
  _context:      context forwarded to both arms; may be NULL.
  _out:          destination for the result.
  _result_size:  the width of a result; must be non-zero.
  _scratch:      caller-owned region for intermediate child results.
  _scratch_size: the region's size in bytes.
Return:
  A boolean value corresponding to either:
  - true, if the tree folded, or
  - false, if an arm declined or the scratch was too small.
*/
bool
d_free_fold
(
    const struct d_free* _tree,
    fn_transformer       _on_hole,
    fn_free_on_node      _on_node,
    void*                _context,
    void*                _out,
    size_t               _result_size,
    void*                _scratch,
    size_t               _scratch_size
)
{
    size_t used;

    // an unusable request folds nothing
    if ( (!_tree)          ||
         (!_on_hole)       ||
         (!_on_node)       ||
         (!_out)           ||
         (!_scratch)       ||
         (_result_size == 0) )
    {
        return false;
    }

    used = 0;

    return d_internal_free_fold(_tree,
                                _on_hole,
                                _on_node,
                                _context,
                                _out,
                                _result_size,
                                (unsigned char*)_scratch,
                                &used,
                                _scratch_size / _result_size);
}

/*
d_free_drive_holes
  The Foldable instance: pushes every hole's value at a reducer, left to right,
stopping when the reducing state latches.

Parameter(s):
  _tree:       the tree to drain.
  _reducer:    the reducer to fold into.
  _state:      the reducing state.
  _value_size: the width of a hole's value.
Return:
  The number of hole values delivered.
*/
size_t
d_free_drive_holes
(
    const struct d_free*     _tree,
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state,
    size_t                   _value_size
)
{
    size_t delivered;
    size_t index;

    // an unusable request or a latched state delivers nothing
    if ( (!_tree)                        ||
         (!d_reducer_is_valid(_reducer)) ||
         (!_state)                       ||
         (_state->done)                  )
    {
        return 0;
    }

    // a hole is one element
    if (_tree->is_hole)
    {
        return d_reducer_drive_array(_reducer,
                                     _state,
                                     _tree->value,
                                     1,
                                     _value_size);
    }

    delivered = 0;

    for (index = 0; index < _tree->child_count; index = index + 1)
    {
        delivered = delivered + d_free_drive_holes(&_tree->children[index],
                                                   _reducer,
                                                   _state,
                                                   _value_size);
    }

    return delivered;
}

/*
d_internal_free_map
  Rebuilds a tree with every hole's value transformed. Nodes are re-allocated
from the arena and their children laid down contiguously, so the result is an
independent tree sharing only the labels.

Parameter(s):
  _tree:      the subtree to map.
  _arena:     the node arena for the new tree.
  _transform: the mapping applied to hole values.
  _context:   context forwarded to `_transform`.
  _values:    the arena the new values are written into.
  _out:       the node to fill in.
Return:
  A boolean value corresponding to whether the subtree was rebuilt.
*/
static bool
d_internal_free_map
(
    const struct d_free*  _tree,
    struct d_free_arena*  _arena,
    fn_transformer        _transform,
    void*                 _context,
    struct d_value_arena* _values,
    struct d_free*        _out
)
{
    struct d_free* children;
    void*          slot;
    size_t         index;

    // a NULL subtree cannot be rebuilt
    if ( (!_tree) ||
         (!_out)  )
    {
        return false;
    }

    // a hole's value is transformed into a fresh slot
    if (_tree->is_hole)
    {
        slot = d_value_arena_take(_values);

        if (!slot)
        {
            return false;
        }

        if (!_transform(_tree->value, slot, _context))
        {
            return false;
        }

        _out->is_hole     = true;
        _out->value       = slot;
        _out->label       = NULL;
        _out->children    = NULL;
        _out->child_count = 0;

        return true;
    }

    children = NULL;

    // the children are taken as one contiguous block, so the node can point at
    // them the way the original did
    if (_tree->child_count > 0)
    {
        if ((_arena->used + _tree->child_count) > _arena->capacity)
        {
            return false;
        }

        children     = &_arena->nodes[_arena->used];
        _arena->used = _arena->used + _tree->child_count;

        for (index = 0; index < _tree->child_count; index = index + 1)
        {
            if (!d_internal_free_map(&_tree->children[index],
                                     _arena,
                                     _transform,
                                     _context,
                                     _values,
                                     &children[index]))
            {
                return false;
            }
        }
    }

    _out->is_hole     = false;
    _out->value       = NULL;
    _out->label       = _tree->label;
    _out->children    = children;
    _out->child_count = _tree->child_count;

    return true;
}

/*
d_free_map
  The Functor instance over the hole type: rebuilds the tree with every hole's
value transformed, leaving the shape and the labels alone.

Parameter(s):
  _tree:      the tree to map.
  _arena:     the node arena for the new tree.
  _transform: the mapping applied to hole values.
  _context:   context forwarded to `_transform`; may be NULL.
  _values:    the arena the new values are written into.
Return:
  The root of the new tree, or NULL if an arena ran out or the mapping declined.
*/
struct d_free*
d_free_map
(
    const struct d_free*  _tree,
    struct d_free_arena*  _arena,
    fn_transformer        _transform,
    void*                 _context,
    struct d_value_arena* _values
)
{
    struct d_free* root;

    // an unusable request rebuilds nothing
    if ( (!_tree)      ||
         (!_arena)     ||
         (!_transform) ||
         (!_values)    )
    {
        return NULL;
    }

    root = d_internal_free_take(_arena);

    // an exhausted arena cannot hold the new root
    if (!root)
    {
        return NULL;
    }

    if (!d_internal_free_map(_tree, _arena, _transform, _context,
                             _values, root))
    {
        return NULL;
    }

    return root;
}

/*
d_internal_free_bind
  Substitutes a subtree at every hole, which is the free monad's whole point:
a tree with gaps, and an arrow saying what goes in each gap.

Parameter(s):
  _tree:    the subtree to substitute into.
  _arena:   the node arena for the new tree.
  _graft:   the arrow producing a replacement subtree.
  _context: context forwarded to `_graft`.
  _out:     the node to fill in.
Return:
  A boolean value corresponding to whether the subtree was rebuilt.
*/
static bool
d_internal_free_bind
(
    const struct d_free* _tree,
    struct d_free_arena* _arena,
    fn_free_graft        _graft,
    void*                _context,
    struct d_free*       _out
)
{
    struct d_free* replacement;
    struct d_free* children;
    size_t         index;

    // a NULL subtree cannot be rebuilt
    if ( (!_tree) ||
         (!_out)  )
    {
        return false;
    }

    // a hole is where substitution happens
    if (_tree->is_hole)
    {
        replacement = _graft(_tree->value, _arena, _context);

        // an arrow declining to graft leaves the hole standing
        if (!replacement)
        {
            _out->is_hole     = true;
            _out->value       = _tree->value;
            _out->label       = NULL;
            _out->children    = NULL;
            _out->child_count = 0;

            return true;
        }

        *_out = *replacement;

        return true;
    }

    children = NULL;

    // the children are taken as one contiguous block
    if (_tree->child_count > 0)
    {
        if ((_arena->used + _tree->child_count) > _arena->capacity)
        {
            return false;
        }

        children     = &_arena->nodes[_arena->used];
        _arena->used = _arena->used + _tree->child_count;

        for (index = 0; index < _tree->child_count; index = index + 1)
        {
            if (!d_internal_free_bind(&_tree->children[index],
                                      _arena,
                                      _graft,
                                      _context,
                                      &children[index]))
            {
                return false;
            }
        }
    }

    _out->is_hole     = false;
    _out->value       = NULL;
    _out->label       = _tree->label;
    _out->children    = children;
    _out->child_count = _tree->child_count;

    return true;
}

/*
d_free_bind
  The Monad instance: replaces every hole with a subtree. This is substitution,
and it is why templates and open documents reduce to this carrier -- a template
is a tree with holes, and filling it is exactly `bind`.

Parameter(s):
  _tree:    the tree to substitute into.
  _arena:   the node arena for the new tree.
  _graft:   the arrow producing a replacement subtree; returning NULL leaves the
            hole standing.
  _context: context forwarded to `_graft`; may be NULL.
Return:
  The root of the new tree, or NULL if the arena ran out.
*/
struct d_free*
d_free_bind
(
    const struct d_free* _tree,
    struct d_free_arena* _arena,
    fn_free_graft        _graft,
    void*                _context
)
{
    struct d_free* root;

    // an unusable request rebuilds nothing
    if ( (!_tree)  ||
         (!_arena) ||
         (!_graft) )
    {
        return NULL;
    }

    root = d_internal_free_take(_arena);

    // an exhausted arena cannot hold the new root
    if (!root)
    {
        return NULL;
    }

    if (!d_internal_free_bind(_tree, _arena, _graft, _context, root))
    {
        return NULL;
    }

    return root;
}

/*
d_free_traverse
  The narrow slice of Traversable the parity assessment scopes: `T` is `free`,
and the applicative is a failure-shaped one -- `maybe` or `result`. Visits every
hole in order, rebuilding the tree, and reports failure if any visit declines.
  This is deliberately NOT the general `traverse`. A general applicative would
have to hold the rebuilt tree inside the carrier, which is the (T, F) matrix the
assessment says C should avoid; the two applicatives the corpus needs collapse
to "all visits must succeed", which is what this implements.

Parameter(s):
  _tree:      the tree to traverse.
  _visit:     the effectful visit applied to each hole value; a false result is
              the failure effect.
  _context:   context forwarded to `_visit`; may be NULL.
  _arena:     the node arena for the rebuilt tree.
  _values:    the arena the visited values are written into.
  _out_tree:  receives the rebuilt tree on success; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if every hole was visited successfully, or
  - false, if any visit declined or an arena ran out.
*/
bool
d_free_traverse
(
    const struct d_free*  _tree,
    fn_transformer        _visit,
    void*                 _context,
    struct d_free_arena*  _arena,
    struct d_value_arena* _values,
    struct d_free**       _out_tree
)
{
    struct d_free* root;

    // an unusable request traverses nothing
    if ( (!_tree)  ||
         (!_visit) ||
         (!_arena) ||
         (!_values) )
    {
        return false;
    }

    root = d_free_map(_tree, _arena, _visit, _context, _values);

    // a declined visit is the failure effect, and it collapses the whole tree
    if (!root)
    {
        return false;
    }

    // handing back the tree is optional; the effect is the answer
    if (_out_tree)
    {
        *_out_tree = root;
    }

    return true;
}

/*
d_cofree_arena_init
  Initialises a cofree node arena over caller-owned storage.

Parameter(s):
  _arena:    the arena to initialise.
  _nodes:    caller-owned array of `_capacity` nodes.
  _capacity: how many nodes fit.
Return:
  A boolean value corresponding to whether the arena was initialised.
*/
bool
d_cofree_arena_init
(
    struct d_cofree_arena* _arena,
    struct d_cofree*       _nodes,
    size_t                 _capacity
)
{
    // an unusable request leaves the arena untouched
    if ( (!_arena) ||
         (!_nodes) )
    {
        return false;
    }

    _arena->nodes    = _nodes;
    _arena->capacity = _capacity;
    _arena->used     = 0;

    return true;
}

/*
d_cofree_arena_rewind
  Releases every cofree node at once.

Parameter(s):
  _arena: the arena to rewind; ignored if NULL.
Return:
  none.
*/
void
d_cofree_arena_rewind
(
    struct d_cofree_arena* _arena
)
{
    // a NULL arena has nothing to rewind
    if (!_arena)
    {
        return;
    }

    _arena->used = 0;

    return;
}

/*
d_cofree_node
  Builds a cofree node carrying a borrowed annotation, label and children.

Parameter(s):
  _arena:       the arena to allocate from.
  _annotation:  the value attached at this node; must outlive the tree.
  _label:       the F's own data; may be NULL.
  _children:    the sub-terms; may be NULL only if `_child_count` is 0.
  _child_count: how many sub-terms.
Return:
  The new node, or NULL when the arena is exhausted.
*/
struct d_cofree*
d_cofree_node
(
    struct d_cofree_arena* _arena,
    const void*            _annotation,
    const void*            _label,
    struct d_cofree*       _children,
    size_t                 _child_count
)
{
    struct d_cofree* node;

    // a node claiming children must have them
    if ( (!_arena)                          ||
         (!_arena->nodes)                   ||
         (_arena->used >= _arena->capacity) ||
         ( (!_children) && (_child_count > 0) ) )
    {
        return NULL;
    }

    node = &_arena->nodes[_arena->used];

    _arena->used = _arena->used + 1;

    node->annotation  = _annotation;
    node->label       = _label;
    node->children    = _children;
    node->child_count = _child_count;

    return node;
}

/*
d_cofree_extract
  The comonad counit: the annotation at the root. Total, because a cofree tree
always has at least one node -- which is the structural difference from `maybe`,
where extraction can fail.

Parameter(s):
  _tree: the tree to extract from; may be NULL.
Return:
  The root's annotation, or NULL for a NULL tree.
*/
const void*
d_cofree_extract
(
    const struct d_cofree* _tree
)
{
    // a NULL tree has no annotation
    if (!_tree)
    {
        return NULL;
    }

    return _tree->annotation;
}

/*
d_cofree_size
  Counts every node in a cofree tree.

Parameter(s):
  _tree: the tree to measure; may be NULL.
Return:
  The number of nodes.
*/
size_t
d_cofree_size
(
    const struct d_cofree* _tree
)
{
    size_t total;
    size_t index;

    // a NULL tree has no nodes
    if (!_tree)
    {
        return 0;
    }

    total = 1;

    for (index = 0; index < _tree->child_count; index = index + 1)
    {
        total = total + d_cofree_size(&_tree->children[index]);
    }

    return total;
}

/*
d_cofree_depth
  Measures a cofree tree's longest root-to-leaf path.

Parameter(s):
  _tree: the tree to measure; may be NULL.
Return:
  The depth, counting the root as 1.
*/
size_t
d_cofree_depth
(
    const struct d_cofree* _tree
)
{
    size_t deepest;
    size_t candidate;
    size_t index;

    // a NULL tree has no depth
    if (!_tree)
    {
        return 0;
    }

    deepest = 0;

    for (index = 0; index < _tree->child_count; index = index + 1)
    {
        candidate = d_cofree_depth(&_tree->children[index]);

        if (candidate > deepest)
        {
            deepest = candidate;
        }
    }

    return deepest + 1;
}

/*
d_cofree_drive
  The Foldable instance: pushes every node's annotation at a reducer in
pre-order.

Parameter(s):
  _tree:            the tree to drain.
  _reducer:         the reducer to fold into.
  _state:           the reducing state.
  _annotation_size: the width of an annotation.
Return:
  The number of annotations delivered.
*/
size_t
d_cofree_drive
(
    const struct d_cofree*   _tree,
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state,
    size_t                   _annotation_size
)
{
    size_t delivered;
    size_t index;

    // an unusable request or a latched state delivers nothing
    if ( (!_tree)                        ||
         (!d_reducer_is_valid(_reducer)) ||
         (!_state)                       ||
         (_state->done)                  )
    {
        return 0;
    }

    delivered = d_reducer_drive_array(_reducer,
                                      _state,
                                      _tree->annotation,
                                      1,
                                      _annotation_size);

    for (index = 0; index < _tree->child_count; index = index + 1)
    {
        delivered = delivered + d_cofree_drive(&_tree->children[index],
                                               _reducer,
                                               _state,
                                               _annotation_size);
    }

    return delivered;
}
