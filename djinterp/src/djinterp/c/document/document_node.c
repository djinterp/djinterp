/******************************************************************************
* djinterp [utility]                                           document_node.c
*
*   The tree operations and THE FOLD.  Section V is the only traversal in the
* document subframework; every dialect is an algebra passed to it.  If a second
* walk over d_doc_node ever appears in this subframework, it is a defect and
* not an optimisation.
*
*   The fold visits a constructor's arguments in declaration order.  That is
* the whole implementation of the emission-order guarantee: no promise is kept
* by care here, only by the shape of the data.
*
*
* path:      /src/djinterp/c/util/document/document_node.c
* link(s):   TBA
* author(s): Agent B (structure)                           created: 2026.07.31
******************************************************************************/

#include "djinterp/c/util/document/document_node.h"


// doc_text_or_empty
//   helper: a borrowed string, with NULL reading as empty.  Every verb is
// promised a non-NULL string, and this is where that promise is kept.
static const char*
doc_text_or_empty(
    const char* _text
)
{
    return _text ? _text : "";
}

// doc_str_equal
//   helper: byte equality of two borrowed strings, NULL equal to NULL and to
// the empty string.  NULL and "" are the same absent value everywhere in this
// subframework, so they must compare equal.
static bool
doc_str_equal(
    const char* _left,
    const char* _right
)
{
    return (d_doc_attr_key_compare(doc_text_or_empty(_left),
                                   doc_text_or_empty(_right)) == 0);
}


// ===========================================================================
// I.     construction
// ===========================================================================
//   Each fills a zero node and sets only the fields its kind uses, so an
// unused field is definitely zero rather than incidentally so -- which is what
// makes d_doc_node_equal a structural comparison rather than a lucky one.

// doc_blank
//   helper: a zero node.  Not a valid document on its own; every constructor
// overwrites `kind`.
static struct d_doc_node
doc_blank(void)
{
    struct d_doc_node node;

    node.attrs.items    = NULL;
    node.attrs.count    = 0u;
    node.attrs.reserved = 0u;
    node.children       = NULL;
    node.text           = NULL;
    node.secondary      = NULL;
    node.millipoints    = 0;
    node.child_count    = 0u;
    node.level          = 0u;
    node.kind           = (uint32_t)D_DOC_KIND_PARAGRAPH;
    node.flags          = 0u;
    node.reserved       = 0u;

    return node;
}

struct d_doc_node
d_doc_heading(
    uint32_t                _level,
    const char*             _text,
    struct d_doc_attributes _attrs
)
{
    struct d_doc_node node;

    node       = doc_blank();
    node.kind  = (uint32_t)D_DOC_KIND_HEADING;

    // level 1 is outermost and there is no level 0; a caller asking for 0 gets
    // the outermost heading rather than an invalid tree
    node.level = (_level == 0u) ? 1u : _level;
    node.text  = _text;
    node.attrs = _attrs;

    return node;
}

struct d_doc_node
d_doc_paragraph(
    const char*             _text,
    struct d_doc_attributes _attrs
)
{
    struct d_doc_node node;

    node       = doc_blank();
    node.kind  = (uint32_t)D_DOC_KIND_PARAGRAPH;
    node.text  = _text;
    node.attrs = _attrs;

    return node;
}

struct d_doc_node
d_doc_key_value(
    const char*             _key,
    const char*             _value,
    struct d_doc_attributes _attrs
)
{
    struct d_doc_node node;

    node           = doc_blank();
    node.kind      = (uint32_t)D_DOC_KIND_KEY_VALUE;
    node.text      = _key;
    node.secondary = _value;
    node.attrs     = _attrs;

    return node;
}

struct d_doc_node
d_doc_rule(
    struct d_doc_attributes _attrs
)
{
    struct d_doc_node node;

    node       = doc_blank();
    node.kind  = (uint32_t)D_DOC_KIND_RULE;
    node.attrs = _attrs;

    return node;
}

struct d_doc_node
d_doc_space(
    int32_t                 _millipoints,
    struct d_doc_attributes _attrs
)
{
    struct d_doc_node node;

    node             = doc_blank();
    node.kind        = (uint32_t)D_DOC_KIND_SPACE;

    // a negative request is not a negative space; it is no request
    node.millipoints = (_millipoints < 0) ? 0 : _millipoints;
    node.attrs       = _attrs;

    return node;
}

struct d_doc_node
d_doc_page_break(
    struct d_doc_attributes _attrs
)
{
    struct d_doc_node node;

    node       = doc_blank();
    node.kind  = (uint32_t)D_DOC_KIND_PAGE_BREAK;

    // page_break took NO attributes until 2026-08-02, and it was the only
    // constructor that did not.  The node always had the field -- so the bag
    // was readable and permanently empty, and no dialect reads a page break's
    // hints, so nothing ever noticed.
    //
    //   It matters because it is the difference between this carrier being a
    // cofree and merely resembling one.  A cofree's annotation is a FACTOR of
    // every layer, which means supplyable, not just readable; one constructor
    // that cannot carry one breaks the correspondence with layout_doc and
    // makes the totality fixture assert less than it appears to.
    node.attrs = _attrs;

    return node;
}

struct d_doc_node
d_doc_column(
    const char*             _header,
    struct d_doc_attributes _attrs
)
{
    struct d_doc_node node;

    node       = doc_blank();
    node.kind  = (uint32_t)D_DOC_KIND_COLUMN;
    node.text  = _header;
    node.attrs = _attrs;

    return node;
}

struct d_doc_node
d_doc_cell(
    const char*             _text,
    struct d_doc_attributes _attrs
)
{
    struct d_doc_node node;

    node       = doc_blank();
    node.kind  = (uint32_t)D_DOC_KIND_CELL;
    node.text  = _text;
    node.attrs = _attrs;

    return node;
}

struct d_doc_node
d_doc_slot(
    const char* _name
)
{
    struct d_doc_node node;

    node      = doc_blank();
    node.kind = (uint32_t)D_DOC_KIND_SLOT;
    node.text = _name;

    return node;
}

// doc_container
//   helper: the shared body of every container constructor -- attach a
// borrowed child array, and normalise the count/pointer disagreement that a
// caller can make in one direction only (a NULL array with a non-zero count is
// caught by d_doc_node_is_well_sorted; a non-NULL array with a zero count is
// simply an empty container).
static struct d_doc_node
doc_container(
    enum d_doc_node_kind            _kind,
    struct d_doc_attributes         _attrs,
    const struct d_doc_node* const* _children,
    uint32_t                        _count
)
{
    struct d_doc_node node;

    node             = doc_blank();
    node.kind        = (uint32_t)_kind;
    node.attrs       = _attrs;
    node.children    = _count ? _children : NULL;
    node.child_count = _children ? _count : 0u;

    return node;
}

struct d_doc_node
d_doc_document(
    struct d_doc_attributes         _attrs,
    const struct d_doc_node* const* _blocks,
    uint32_t                        _count
)
{
    return doc_container(D_DOC_KIND_DOCUMENT, _attrs, _blocks, _count);
}

struct d_doc_node
d_doc_element(
    const char*                     _name,
    const char*                     _text,
    struct d_doc_attributes         _attrs,
    const struct d_doc_node* const* _blocks,
    uint32_t                        _count
)
{
    struct d_doc_node node;

    node           = doc_container(D_DOC_KIND_ELEMENT, _attrs,
                                   _blocks, _count);
    node.text      = _name;
    node.secondary = _text;

    return node;
}

struct d_doc_node
d_doc_list(
    bool                            _ordered,
    struct d_doc_attributes         _attrs,
    const struct d_doc_node* const* _items,
    uint32_t                        _count
)
{
    struct d_doc_node node;

    node       = doc_container(D_DOC_KIND_LIST, _attrs, _items, _count);
    node.flags = _ordered ? D_DOC_FLAG_ORDERED : 0u;

    return node;
}

struct d_doc_node
d_doc_item(
    const char*                     _text,
    struct d_doc_attributes         _attrs,
    const struct d_doc_node* const* _blocks,
    uint32_t                        _count
)
{
    struct d_doc_node node;

    node      = doc_container(D_DOC_KIND_ITEM, _attrs, _blocks, _count);
    node.text = _text;

    return node;
}

struct d_doc_node
d_doc_column_group(
    const char*                     _label,
    struct d_doc_attributes         _attrs,
    const struct d_doc_node* const* _children,
    uint32_t                        _count
)
{
    struct d_doc_node node;

    node      = doc_container(D_DOC_KIND_COLUMN_GROUP, _attrs,
                              _children, _count);
    node.text = _label;

    return node;
}

struct d_doc_node
d_doc_row(
    struct d_doc_attributes         _attrs,
    const struct d_doc_node* const* _cells,
    uint32_t                        _count
)
{
    return doc_container(D_DOC_KIND_ROW, _attrs, _cells, _count);
}

struct d_doc_node
d_doc_table(
    struct d_doc_attributes         _attrs,
    const struct d_doc_node* const* _columns_rows,
    uint32_t                        _column_count,
    uint32_t                        _row_count
)
{
    // the split is recoverable from the children's kinds, so it is not stored:
    // the counts are taken here only to make the caller state the ordering it
    // is committing to
    return doc_container(D_DOC_KIND_TABLE, _attrs, _columns_rows,
                         _column_count + _row_count);
}

struct d_doc_node
d_doc_repeat(
    const char*                     _sequence,
    const char*                     _scope,
    struct d_doc_attributes         _attrs,
    const struct d_doc_node* const* _blocks,
    uint32_t                        _count
)
{
    struct d_doc_node node;

    node           = doc_container(D_DOC_KIND_REPEAT, _attrs,
                                   _blocks, _count);
    node.text      = _sequence;

    // an empty scope defaults to the sequence name, so repeat("modules", ...)
    // exposes {modules.name} without the caller saying so twice
    node.secondary = ( _scope && (_scope[0] != '\0') ) ? _scope : _sequence;

    return node;
}


// ===========================================================================
// II.    inspection
// ===========================================================================

/*
d_doc_column_span
  How many leaf columns a column or a group covers.

Parameter(s):
  _column_or_group: the node.
Return:
  1 for a leaf column, the sum of its children's spans for a group, 0 for
anything else.

Note:
  DERIVED, NOT STORED -- for the same reason a table's column count is: a
stored span can disagree with the subtree beneath it, and a derived one
cannot.  This is `table_metadata.hpp`'s own rule, where a header level's
extent is the sum of its cells' spans.
*/
uint32_t
d_doc_column_span(
    const struct d_doc_node* _column_or_group
)
{
    uint32_t index;
    uint32_t total;

    if (!_column_or_group)
    {
        return 0u;
    }

    if (_column_or_group->kind == (uint32_t)D_DOC_KIND_COLUMN)
    {
        return 1u;
    }

    if (_column_or_group->kind != (uint32_t)D_DOC_KIND_COLUMN_GROUP)
    {
        return 0u;
    }

    total = 0u;

    for (index = 0u; index < _column_or_group->child_count; ++index)
    {
        total += d_doc_column_span(_column_or_group->children[index]);
    }

    return total;
}

/*
d_doc_node_anchor_count
  How many nodes carry a `locator` hint -- how many anchors a recording pass
should therefore see.

Parameter(s):
  _node: the tree.
Return:
  The count, 0 for a tree with no anchors.

Note:
  This exists to make an UNWIRED RECORDER detectable.  A recorder that is never
called records nothing, an agreement check over two empty records passes, and
the whole two-pass protocol reports success while protecting nothing.  Both
sides' own tests can pass in that state -- one calls the anchor hook directly,
the other uses a stand-in -- so the only place the gap is visible is where the
tree and the record meet, which is here.
*/
/*
d_doc_extract
  The annotation carried by a node -- the counit of the cofree comonad.

Parameter(s):
  _node: the node; may be NULL.
Return:
  A borrowed pointer to the node's hint bag, never NULL: a NULL node yields a
static empty bag, so extract is TOTAL in the C sense as well as the formal
one.  A caller never has to ask whether a node is annotated.
*/
const struct d_doc_attributes*
d_doc_extract(
    const struct d_doc_node* _node
)
{
    static const struct d_doc_attributes empty = D_DOC_ATTRIBUTES_EMPTY;

    return _node ? &_node->attrs : &empty;
}

uint32_t
d_doc_node_anchor_count(
    const struct d_doc_node* _node
)
{
    uint32_t index;
    uint32_t total;

    if (!_node)
    {
        return 0u;
    }

    total = d_doc_attr_has(&_node->attrs, D_DOC_ATTR_LOCATOR) ? 1u : 0u;

    for (index = 0u; index < _node->child_count; ++index)
    {
        if (!_node->children)
        {
            break;
        }

        total += d_doc_node_anchor_count(_node->children[index]);
    }

    return total;
}

uint32_t
d_doc_table_column_count(
    const struct d_doc_node* _table
)
{
    uint32_t index;
    uint32_t total;

    if ( (!_table) ||
         (_table->kind != (uint32_t)D_DOC_KIND_TABLE) ||
         (!_table->children) )
    {
        return 0u;
    }

    total = 0u;

    for (index = 0u; index < _table->child_count; ++index)
    {
        const struct d_doc_node* child = _table->children[index];

        if ( (!child) ||
             ( (child->kind != (uint32_t)D_DOC_KIND_COLUMN) &&
               (child->kind != (uint32_t)D_DOC_KIND_COLUMN_GROUP) ) )
        {
            break;
        }

        total += d_doc_column_span(child);
    }

    return total;
}

uint32_t
d_doc_table_row_count(
    const struct d_doc_node* _table
)
{
    uint32_t columns;
    uint32_t index;

    if ( (!_table) ||
         (_table->kind != (uint32_t)D_DOC_KIND_TABLE) )
    {
        return 0u;
    }

    columns = 0u;

    for (index = 0u; index < _table->child_count; ++index)
    {
        if ( (_table->children[index]) &&
             (_table->children[index]->kind ==
                  (uint32_t)D_DOC_KIND_ROW) )
        {
            ++columns;
        }
    }

    return columns;
}

bool
d_doc_node_is_closed(
    const struct d_doc_node* _node
)
{
    uint32_t index;

    if (!_node)
    {
        return true;
    }

    // a hole or a binder makes the whole tree open, however deep it sits
    if (d_doc_kind_is_open((enum d_doc_node_kind)_node->kind))
    {
        return false;
    }

    for (index = 0u; index < _node->child_count; ++index)
    {
        if ( (!_node->children) ||
             (!d_doc_node_is_closed(_node->children[index])) )
        {
            return false;
        }
    }

    return true;
}

// doc_well_sorted
//   helper: the depth-bounded body of the well-sortedness check.  Bounded for
// the same reason the fold is: a cyclic tree must fail, not hang.
static bool
doc_well_sorted(
    const struct d_doc_node* _node,
    uint32_t                 _depth
)
{
    enum d_doc_node_kind kind;
    enum d_doc_sort      expected;
    uint32_t             index;
    bool                 seen_row;

    if (!_node)
    {
        return false;
    }

    if (_depth > D_INTERNAL_DOC_MAX_DEPTH)
    {
        return false;
    }

    kind = (enum d_doc_node_kind)_node->kind;

    if (_node->kind >= (uint32_t)D_DOC_KIND_COUNT)
    {
        return false;
    }

    // a count without storage is the one malformation a borrowed tree can
    // express and a caller can plausibly make
    if ( (_node->child_count > 0u) &&
         (!_node->children) )
    {
        return false;
    }

    if ( (_node->child_count > 0u) &&
         (!d_doc_kind_takes_children(kind)) )
    {
        return false;
    }

    if (!d_doc_attributes_is_canonical(&_node->attrs))
    {
        return false;
    }

    // the table is the one two-sorted constructor: all columns, then all rows,
    // and no column after a row.  This check is where the emission-order
    // guarantee is enforced at construction rather than trusted at render.
    if (kind == D_DOC_KIND_TABLE)
    {
        seen_row = false;

        for (index = 0u; index < _node->child_count; ++index)
        {
            const struct d_doc_node* child = _node->children[index];

            if (!child)
            {
                return false;
            }

            if ( (child->kind == (uint32_t)D_DOC_KIND_COLUMN) ||
                 (child->kind == (uint32_t)D_DOC_KIND_COLUMN_GROUP) )
            {
                if (seen_row)
                {
                    return false;
                }
            }
            else if (child->kind == (uint32_t)D_DOC_KIND_ROW)
            {
                seen_row = true;
            }
            else
            {
                return false;
            }

            if (!doc_well_sorted(child, _depth + 1u))
            {
                return false;
            }
        }

        return true;
    }

    expected = d_doc_child_sort(kind);

    for (index = 0u; index < _node->child_count; ++index)
    {
        const struct d_doc_node* child = _node->children[index];

        if (!child)
        {
            return false;
        }

        if (d_doc_kind_sort((enum d_doc_node_kind)child->kind) != expected)
        {
            return false;
        }

        if (!doc_well_sorted(child, _depth + 1u))
        {
            return false;
        }
    }

    return true;
}

bool
d_doc_node_is_well_sorted(
    const struct d_doc_node* _node
)
{
    return doc_well_sorted(_node, 0u);
}

bool
d_doc_node_equal(
    const struct d_doc_node* _left,
    const struct d_doc_node* _right
)
{
    uint32_t index;

    if (_left == _right)
    {
        return true;
    }

    if ( (!_left) ||
         (!_right) )
    {
        return false;
    }

    if ( (_left->kind        != _right->kind)        ||
         (_left->level       != _right->level)       ||
         (_left->flags       != _right->flags)       ||
         (_left->child_count != _right->child_count) ||
         (_left->millipoints != _right->millipoints) )
    {
        return false;
    }

    if ( (!doc_str_equal(_left->text, _right->text)) ||
         (!doc_str_equal(_left->secondary, _right->secondary)) )
    {
        return false;
    }

    if (_left->attrs.count != _right->attrs.count)
    {
        return false;
    }

    // both bags are canonically ordered, so equality is positional: this is
    // exactly why ruling R2 exists, and why it is not merely a rendering
    // convenience
    for (index = 0u; index < _left->attrs.count; ++index)
    {
        if ( (!doc_str_equal(_left->attrs.items[index].key,
                             _right->attrs.items[index].key)) ||
             (!doc_str_equal(_left->attrs.items[index].value,
                             _right->attrs.items[index].value)) )
        {
            return false;
        }
    }

    for (index = 0u; index < _left->child_count; ++index)
    {
        if (!d_doc_node_equal(_left->children[index],
                              _right->children[index]))
        {
            return false;
        }
    }

    return true;
}

uint32_t
d_doc_node_depth(
    const struct d_doc_node* _node
)
{
    uint32_t index;
    uint32_t deepest;
    uint32_t child_depth;

    if (!_node)
    {
        return 0u;
    }

    deepest = 0u;

    for (index = 0u; index < _node->child_count; ++index)
    {
        if (!_node->children)
        {
            break;
        }

        child_depth = d_doc_node_depth(_node->children[index]);

        if (child_depth > deepest)
        {
            deepest = child_depth;
        }
    }

    return deepest + 1u;
}

uint32_t
d_doc_node_count(
    const struct d_doc_node* _node
)
{
    uint32_t index;
    uint32_t total;

    if (!_node)
    {
        return 0u;
    }

    total = 1u;

    for (index = 0u; index < _node->child_count; ++index)
    {
        if (!_node->children)
        {
            break;
        }

        total += d_doc_node_count(_node->children[index]);
    }

    return total;
}

uint32_t
d_doc_node_slot_count(
    const struct d_doc_node* _node
)
{
    uint32_t index;
    uint32_t total;

    if (!_node)
    {
        return 0u;
    }

    total = (_node->kind == (uint32_t)D_DOC_KIND_SLOT) ? 1u : 0u;

    for (index = 0u; index < _node->child_count; ++index)
    {
        if (!_node->children)
        {
            break;
        }

        total += d_doc_node_slot_count(_node->children[index]);
    }

    return total;
}


// ===========================================================================
// III.   substitution
// ===========================================================================
//   The free monad's bind.  Rebuilds the spine into the caller's arena,
// replacing each slot by the valuation's answer.  A slot the valuation
// declines to fill survives unchanged, which is what makes substitution TOTAL
// and therefore what makes the monad laws hold without a partiality side
// condition.

void
d_doc_arena_reset(
    struct d_doc_arena* _arena
)
{
    if (_arena)
    {
        _arena->node_count = 0u;
        _arena->link_count = 0u;
        _arena->byte_count = 0u;
    }

    return;
}

/*
d_doc_arena_init
  Binds an arena to caller-owned storage.  Every region may be NULL; a
substitution that needs a region it does not have reports SINK_FULL, which is
a MECHANICAL failure and stays distinct from a malformed tree.

Parameter(s):
  _arena:         the arena.
  _nodes:         storage for rebuilt nodes.
  _node_capacity: how many.
  _links:         storage for child arrays.
  _link_capacity: how many.
  _bytes:         scratch for interpolated strings; only expansion uses it.
  _byte_capacity: how many.
Return:
  none.
*/
void
d_doc_arena_init(
    struct d_doc_arena*       _arena,
    struct d_doc_node*        _nodes,
    uint32_t                  _node_capacity,
    const struct d_doc_node** _links,
    uint32_t                  _link_capacity,
    char*                     _bytes,
    uint32_t                  _byte_capacity
)
{
    if (!_arena)
    {
        return;
    }

    _arena->nodes         = _nodes;
    _arena->links         = _links;
    _arena->bytes         = _bytes;
    _arena->node_capacity = _node_capacity;
    _arena->link_capacity = _link_capacity;
    _arena->byte_capacity = _byte_capacity;
    _arena->node_count    = 0u;
    _arena->link_count    = 0u;
    _arena->byte_count    = 0u;

    return;
}

// doc_arena_node
//   helper: reserve one node in the arena.  Returns NULL when exhausted; the
// caller turns that into D_DOC_RENDER_SINK_FULL, which is a MECHANICAL
// failure and is deliberately not the same status as a malformed tree.
static struct d_doc_node*
doc_arena_node(
    struct d_doc_arena* _arena
)
{
    struct d_doc_node* slot;

    if ( (!_arena)         ||
         (!_arena->nodes)  ||
         (_arena->node_count >= _arena->node_capacity) )
    {
        return NULL;
    }

    slot = &_arena->nodes[_arena->node_count];
    ++_arena->node_count;

    return slot;
}

// doc_arena_links
//   helper: reserve _count contiguous child links.  Returns NULL when
// exhausted, and for _count == 0 returns NULL without consuming anything,
// since an empty child array is spelled NULL everywhere.
static const struct d_doc_node**
doc_arena_links(
    struct d_doc_arena* _arena,
    uint32_t            _count
)
{
    const struct d_doc_node** block;

    if (_count == 0u)
    {
        return NULL;
    }

    if ( (!_arena)         ||
         (!_arena->links)  ||
         (_count > (_arena->link_capacity - _arena->link_count)) )
    {
        return NULL;
    }

    block = &_arena->links[_arena->link_count];
    _arena->link_count += _count;

    return block;
}

enum d_doc_render_status
d_doc_subst(
    const struct d_doc_node*  _tree,
    fn_doc_valuation          _valuation,
    void*                     _context,
    struct d_doc_arena*       _arena,
    const struct d_doc_node** _out
)
{
    const struct d_doc_node*  replacement;
    const struct d_doc_node** links;
    struct d_doc_node*        rebuilt;
    enum d_doc_render_status  status;
    uint32_t                  index;

    if ( (!_tree) ||
         (!_out) )
    {
        return D_DOC_RENDER_MALFORMED;
    }

    // eta: a hole is replaced wholesale by whatever the valuation names, and
    // an unfilled hole is returned as itself -- not an error (non-property 4)
    if (_tree->kind == (uint32_t)D_DOC_KIND_SLOT)
    {
        replacement = _valuation ? _valuation(doc_text_or_empty(_tree->text),
                                              _context)
                                 : NULL;

        (*_out) = replacement ? replacement : _tree;

        return D_DOC_RENDER_OK;
    }

    // a leaf is shared rather than copied: nothing below it can change, so
    // rebuilding it would cost arena for no difference
    if (_tree->child_count == 0u)
    {
        (*_out) = _tree;

        return D_DOC_RENDER_OK;
    }

    if (!_tree->children)
    {
        return D_DOC_RENDER_MALFORMED;
    }

    links = doc_arena_links(_arena, _tree->child_count);

    if (!links)
    {
        return D_DOC_RENDER_SINK_FULL;
    }

    for (index = 0u; index < _tree->child_count; ++index)
    {
        status = d_doc_subst(_tree->children[index], _valuation, _context,
                             _arena,
                             (const struct d_doc_node**)&links[index]);

        if (status != D_DOC_RENDER_OK)
        {
            return status;
        }
    }

    rebuilt = doc_arena_node(_arena);

    if (!rebuilt)
    {
        return D_DOC_RENDER_SINK_FULL;
    }

    (*rebuilt)          = (*_tree);
    rebuilt->children   = links;

    (*_out) = rebuilt;

    return D_DOC_RENDER_OK;
}


// ===========================================================================
// IV.    expansion
// ===========================================================================
//   `expand`: unroll every repeat, interpolate every token, leave every slot
// alone.  The result has no binders and no braces, and is therefore a
// DOCUMENT in the sense of ch-documents.tex section 5 as soon as its slots
// are filled.
//
//   WHY THIS NEEDS NO OWNED STRING.  Interpolation appends literal runs and
// looked-up values in order.  That is accumulation, and accumulation runs
// into caller-supplied bytes exactly as well as into a growable string.  A
// string with no `{` is not copied at all -- the expanded node borrows the
// original pointer -- so a template with no tokens costs zero bytes here.

// D_INTERNAL_DOC_MAX_TOKEN
//   constant: the longest `{token}` that will be resolved.  A longer one
// interpolates as empty rather than truncating to a different name, because
// resolving the wrong binding is worse than resolving none.
#ifndef D_INTERNAL_DOC_MAX_TOKEN
    #define D_INTERNAL_DOC_MAX_TOKEN  128u
#endif

// doc_scope
//   struct: one active repeat binding.  `scope` is the name tokens use,
// `sequence` the name the environment knows, `index` the current item.
struct doc_scope
{
    const char* scope;
    const char* sequence;
    uint32_t    index;
};

// doc_expand_ctx
//   struct: the walk's state.  The scope stack is bounded; a repeat nested
// past the bound still unrolls, its tokens simply falling through to the
// scalar lookup.  Degrade, never error.
struct doc_expand_ctx
{
    const struct d_doc_environment* env;
    struct d_doc_arena*             arena;
    struct doc_scope                scopes[D_INTERNAL_DOC_MAX_SCOPES];
    uint32_t                        scope_depth;
};

// doc_cstr_len
//   helper: byte length, NULL reading as zero.
static uint32_t
doc_cstr_len(
    const char* _text
)
{
    uint32_t length;

    length = 0u;

    if (_text)
    {
        while (_text[length])
        {
            ++length;
        }
    }

    return length;
}

// doc_has_token
//   helper: whether a string contains anything to interpolate.  The fast
// path: no brace means no copy, which is why an untokenised template costs
// nothing from the byte arena.
static bool
doc_has_token(
    const char* _text
)
{
    uint32_t index;

    if (!_text)
    {
        return false;
    }

    for (index = 0u; _text[index]; ++index)
    {
        if (_text[index] == '{')
        {
            return true;
        }
    }

    return false;
}

// doc_resolve
//   helper: one token name to its value, or NULL when unbound.  Scopes are
// searched innermost first, so an inner repeat shadows an outer one binding
// the same name; a token matching no scope falls through to the scalar
// lookup.  Unbound is NULL, which the caller writes as nothing.
static const char*
doc_resolve(
    const struct doc_expand_ctx* _ctx,
    const char*                  _token
)
{
    const struct doc_scope* scope;
    uint32_t                level;
    uint32_t                length;

    if ( (!_ctx->env) ||
         (!_token) )
    {
        return NULL;
    }

    level = _ctx->scope_depth;

    while (level > 0u)
    {
        --level;
        scope  = &_ctx->scopes[level];
        length = doc_cstr_len(scope->scope);

        if ( (length > 0u) &&
             (_token[length] == '.') )
        {
            uint32_t index;
            bool     matched;

            matched = true;

            for (index = 0u; index < length; ++index)
            {
                if (_token[index] != scope->scope[index])
                {
                    matched = false;

                    break;
                }
            }

            if (matched)
            {
                return _ctx->env->field
                     ? _ctx->env->field(scope->sequence, scope->index,
                                        _token + length + 1u,
                                        _ctx->env->context)
                     : NULL;
            }
        }
    }

    return _ctx->env->scalar ? _ctx->env->scalar(_token, _ctx->env->context)
                             : NULL;
}

// doc_bytes_put
//   helper: append one byte to the arena's scratch.  Returns false when
// exhausted, which the caller turns into SINK_FULL -- a MECHANICAL failure,
// never confused with a malformed tree.
static bool
doc_bytes_put(
    struct d_doc_arena* _arena,
    char                _byte
)
{
    if ( (!_arena)        ||
         (!_arena->bytes) ||
         (_arena->byte_count >= _arena->byte_capacity) )
    {
        return false;
    }

    _arena->bytes[_arena->byte_count] = _byte;
    ++_arena->byte_count;

    return true;
}

// doc_interpolate_scoped
//   helper: the body of interpolation, with a scope stack.  Emits literal
// runs verbatim, `{{` as one brace, and `{name}` as the resolved value or as
// nothing.  A `{` that opens nothing is emitted verbatim, so a document ABOUT
// braces survives being one.
static const char*
doc_interpolate_scoped(
    const char*                  _text,
    const struct doc_expand_ctx* _ctx
)
{
    char        token[D_INTERNAL_DOC_MAX_TOKEN];
    const char* start;
    const char* value;
    uint32_t    cursor;
    uint32_t    used;

    if (!doc_has_token(_text))
    {
        return _text;
    }

    if ( (!_ctx->arena) ||
         (!_ctx->arena->bytes) )
    {
        return NULL;
    }

    start  = _ctx->arena->bytes + _ctx->arena->byte_count;
    cursor = 0u;

    while (_text[cursor])
    {
        if (_text[cursor] != '{')
        {
            // `}}` is a literal brace too.  Symmetry matters here: a reader
            // who learns to escape one brace must not discover that the other
            // is different
            if ( (_text[cursor] == '}') &&
                 (_text[cursor + 1u] == '}') )
            {
                if (!doc_bytes_put(_ctx->arena, '}'))
                {
                    return NULL;
                }

                cursor += 2u;

                continue;
            }

            if (!doc_bytes_put(_ctx->arena, _text[cursor]))
            {
                return NULL;
            }

            ++cursor;

            continue;
        }

        // `{{` is a literal brace
        if (_text[cursor + 1u] == '{')
        {
            if (!doc_bytes_put(_ctx->arena, '{'))
            {
                return NULL;
            }

            cursor += 2u;

            continue;
        }

        used = 0u;

        while ( (_text[cursor + 1u + used])          &&
                (_text[cursor + 1u + used] != '}')   &&
                (_text[cursor + 1u + used] != '{')   &&
                (used < (D_INTERNAL_DOC_MAX_TOKEN - 1u)) )
        {
            token[used] = _text[cursor + 1u + used];
            ++used;
        }

        // an unterminated or over-long brace opens nothing: emit it verbatim
        // rather than swallowing the rest of the string
        if (_text[cursor + 1u + used] != '}')
        {
            if (!doc_bytes_put(_ctx->arena, '{'))
            {
                return NULL;
            }

            ++cursor;

            continue;
        }

        token[used] = '\0';
        value       = doc_resolve(_ctx, token);

        while ( (value) &&
                (*value) )
        {
            if (!doc_bytes_put(_ctx->arena, *value))
            {
                return NULL;
            }

            ++value;
        }

        cursor += used + 2u;
    }

    if (!doc_bytes_put(_ctx->arena, '\0'))
    {
        return NULL;
    }

    return start;
}

/*
d_doc_interpolate
  Resolves the tokens in one string against an environment, with no repeat
scopes in play.  Exposed because it is independently testable and because a
caller building a tree by hand wants it.

Parameter(s):
  _text:  the token-bearing string; may be NULL.
  _env:   the bindings.
  _arena: scratch for the result.
Return:
  _text itself when it carries no token, a pointer into the arena when it
does, or NULL when the arena is exhausted.
*/
const char*
d_doc_interpolate(
    const char*                     _text,
    const struct d_doc_environment* _env,
    struct d_doc_arena*             _arena
)
{
    struct doc_expand_ctx ctx;

    ctx.env         = _env;
    ctx.arena       = _arena;
    ctx.scope_depth = 0u;

    return doc_interpolate_scoped(_text, &ctx);
}

// doc_expand_width
//   helper: how many nodes a child contributes to its PARENT'S child list.
// Everything is one node except a repeat, which is as many as its unrolled
// blocks -- and a repeat may contain a repeat, so this recurses through
// repeats and nothing else.
//
//   This exists so the parent's link block can be reserved BEFORE the
// children are expanded.  The arena bumps, so a block reserved first stays
// contiguous while the recursion allocates grandchildren after it.  Counting
// first is what makes a single-pass bump allocator sufficient here.
//
//   The environment's `count` is called once here and once during the fill,
// so it must be stable across a single expansion.
static uint32_t
doc_expand_width(
    struct doc_expand_ctx*   _ctx,
    const struct d_doc_node* _node,
    uint32_t                 _depth
)
{
    uint32_t items;
    uint32_t index;
    uint32_t child;
    uint32_t total;

    if ( (!_node) ||
         (_depth > D_INTERNAL_DOC_MAX_DEPTH) )
    {
        return 0u;
    }

    if (_node->kind != (uint32_t)D_DOC_KIND_REPEAT)
    {
        return 1u;
    }

    items = ( _ctx->env && _ctx->env->count )
          ? _ctx->env->count(doc_text_or_empty(_node->text),
                             _ctx->env->context)
          : 0u;

    total = 0u;

    for (index = 0u; index < items; ++index)
    {
        bool pushed;

        pushed = false;

        if (_ctx->scope_depth < D_INTERNAL_DOC_MAX_SCOPES)
        {
            _ctx->scopes[_ctx->scope_depth].scope    =
                doc_text_or_empty(_node->secondary);
            _ctx->scopes[_ctx->scope_depth].sequence =
                doc_text_or_empty(_node->text);
            _ctx->scopes[_ctx->scope_depth].index    = index;
            ++_ctx->scope_depth;
            pushed = true;
        }

        for (child = 0u; child < _node->child_count; ++child)
        {
            total += doc_expand_width(_ctx, _node->children[child],
                                      _depth + 1u);
        }

        if (pushed)
        {
            --_ctx->scope_depth;
        }
    }

    return total;
}

static enum d_doc_render_status
doc_expand_node(struct doc_expand_ctx*    _ctx,
                const struct d_doc_node*  _node,
                uint32_t                  _depth,
                const struct d_doc_node** _slots,
                uint32_t*                 _written);

// doc_expand_children
//   helper: expand a node's children into a freshly reserved, contiguous link
// block.  Reserve first, fill second -- see doc_expand_width.
static enum d_doc_render_status
doc_expand_children(
    struct doc_expand_ctx*     _ctx,
    const struct d_doc_node*   _node,
    uint32_t                   _depth,
    const struct d_doc_node*** _out_links,
    uint32_t*                  _out_count
)
{
    const struct d_doc_node** links;
    enum d_doc_render_status  status;
    uint32_t                  total;
    uint32_t                  written;
    uint32_t                  index;

    total = 0u;

    for (index = 0u; index < _node->child_count; ++index)
    {
        total += doc_expand_width(_ctx, _node->children[index], _depth);
    }

    (*_out_count) = total;
    (*_out_links) = NULL;

    if (total == 0u)
    {
        return D_DOC_RENDER_OK;
    }

    links = doc_arena_links(_ctx->arena, total);

    if (!links)
    {
        return D_DOC_RENDER_SINK_FULL;
    }

    (*_out_links) = links;
    written       = 0u;

    for (index = 0u; index < _node->child_count; ++index)
    {
        status = doc_expand_node(_ctx, _node->children[index], _depth + 1u,
                                 links, &written);

        if (status != D_DOC_RENDER_OK)
        {
            return status;
        }
    }

    return D_DOC_RENDER_OK;
}

// doc_expand_node
//   helper: expand one child into its parent's link block, appending one node
// -- or, for a repeat, one run of nodes -- at `*_written`.
static enum d_doc_render_status
doc_expand_node(
    struct doc_expand_ctx*    _ctx,
    const struct d_doc_node*  _node,
    uint32_t                  _depth,
    const struct d_doc_node** _slots,
    uint32_t*                 _written
)
{
    const struct d_doc_node** links;
    struct d_doc_node*        rebuilt;
    enum d_doc_render_status  status;
    uint32_t                  count;
    uint32_t                  items;
    uint32_t                  index;
    uint32_t                  child;

    if (!_node)
    {
        return D_DOC_RENDER_MALFORMED;
    }

    if (_depth > D_INTERNAL_DOC_MAX_DEPTH)
    {
        return D_DOC_RENDER_DEPTH_EXCEEDED;
    }

    // a repeat is not rebuilt: it VANISHES, replaced by its blocks once per
    // item.  That is what makes the result free of binders
    if (_node->kind == (uint32_t)D_DOC_KIND_REPEAT)
    {
        items = ( _ctx->env && _ctx->env->count )
              ? _ctx->env->count(doc_text_or_empty(_node->text),
                                 _ctx->env->context)
              : 0u;

        for (index = 0u; index < items; ++index)
        {
            bool pushed;

            pushed = false;

            if (_ctx->scope_depth < D_INTERNAL_DOC_MAX_SCOPES)
            {
                _ctx->scopes[_ctx->scope_depth].scope    =
                    doc_text_or_empty(_node->secondary);
                _ctx->scopes[_ctx->scope_depth].sequence =
                    doc_text_or_empty(_node->text);
                _ctx->scopes[_ctx->scope_depth].index    = index;
                ++_ctx->scope_depth;
                pushed = true;
            }

            for (child = 0u; child < _node->child_count; ++child)
            {
                status = doc_expand_node(_ctx, _node->children[child],
                                         _depth + 1u, _slots, _written);

                if (status != D_DOC_RENDER_OK)
                {
                    if (pushed)
                    {
                        --_ctx->scope_depth;
                    }

                    return status;
                }
            }

            if (pushed)
            {
                --_ctx->scope_depth;
            }
        }

        return D_DOC_RENDER_OK;
    }

    status = doc_expand_children(_ctx, _node, _depth, &links, &count);

    if (status != D_DOC_RENDER_OK)
    {
        return status;
    }

    rebuilt = doc_arena_node(_ctx->arena);

    if (!rebuilt)
    {
        return D_DOC_RENDER_SINK_FULL;
    }

    (*rebuilt)             = (*_node);
    rebuilt->children      = links;
    rebuilt->child_count   = count;

    // only text and secondary are token-bearing.  Hints are structure, not
    // content, so an attribute value full of braces survives untouched
    rebuilt->text      = doc_interpolate_scoped(_node->text, _ctx);
    rebuilt->secondary = doc_interpolate_scoped(_node->secondary, _ctx);

    if ( ((_node->text)      && (!rebuilt->text)) ||
         ((_node->secondary) && (!rebuilt->secondary)) )
    {
        return D_DOC_RENDER_SINK_FULL;
    }

    _slots[*_written] = rebuilt;
    ++(*_written);

    return D_DOC_RENDER_OK;
}

/*
d_doc_expand
  Unrolls every repeat and interpolates every token, against an environment,
into a caller-supplied arena.  Slots are left standing -- filling them is
d_doc_subst's job, and the two compose in either order.

Parameter(s):
  _tree:  the template.
  _env:   the bindings; may be NULL, in which case every sequence is empty and
          every token interpolates as nothing.
  _arena: caller-owned storage; needs a byte region only if the template
          carries tokens.
  _out:   receives the expanded tree.
Return:
  D_DOC_RENDER_OK; SINK_FULL when any arena region is exhausted;
DEPTH_EXCEEDED past the bound; MALFORMED for a NULL child.
*/
enum d_doc_render_status
d_doc_expand(
    const struct d_doc_node*        _tree,
    const struct d_doc_environment* _env,
    struct d_doc_arena*             _arena,
    const struct d_doc_node**       _out
)
{
    struct doc_expand_ctx     ctx;
    const struct d_doc_node*  slot;
    enum d_doc_render_status  status;
    uint32_t                  written;

    if ( (!_tree) ||
         (!_out) )
    {
        return D_DOC_RENDER_MALFORMED;
    }

    ctx.env         = _env;
    ctx.arena       = _arena;
    ctx.scope_depth = 0u;

    slot    = NULL;
    written = 0u;
    status  = doc_expand_node(&ctx, _tree, 0u, &slot, &written);

    if (status != D_DOC_RENDER_OK)
    {
        return status;
    }

    // a repeat at the ROOT would expand to a run rather than a tree, and a
    // run is not a document.  Reject it as malformed rather than silently
    // returning the first element
    if (written != 1u)
    {
        return D_DOC_RENDER_MALFORMED;
    }

    (*_out) = slot;

    return D_DOC_RENDER_OK;
}


// ===========================================================================
// VI-b.  the two-pass protocol
// ===========================================================================

// D_DOC_PAGE_PLACEHOLDER_MAX
//   constant: the widest reservation the placeholder buffer can spell.
#define D_DOC_PAGE_PLACEHOLDER_MAX  16u

static char g_doc_placeholder[D_DOC_PAGE_PLACEHOLDER_MAX + 1u];

/*
d_doc_page_map_init
  Binds a page map to caller-owned storage and fixes the reserved width.

Parameter(s):
  _map:         the map.
  _entries:     storage.
  _capacity:    how many entries.
  _page_digits: the RESERVED width, in digits.  Every page number renders at
                exactly this width in both passes, which is what makes the two
                extents equal.
Return:
  none.
*/
void
d_doc_page_map_init(
    struct d_doc_page_map*   _map,
    struct d_doc_page_entry* _entries,
    uint32_t                 _capacity,
    uint32_t                 _page_digits
)
{
    if (!_map)
    {
        return;
    }

    _map->entries  = _entries;
    _map->capacity = _capacity;
    _map->count    = 0u;

    // a zero reservation would make every number render empty in pass one and
    // non-empty in pass two, which is the exact failure the reservation
    // exists to prevent.  One digit is the floor
    _map->page_digits = (_page_digits == 0u) ? 1u
                      : ( (_page_digits > D_DOC_PAGE_PLACEHOLDER_MAX)
                          ? D_DOC_PAGE_PLACEHOLDER_MAX
                          : _page_digits );
    _map->max_page    = 0u;

    return;
}

/*
d_doc_page_map_record
  Notes that _anchor landed on _page.  Called by a recording algebra, once per
anchor, in document order.

Parameter(s):
  _map:    the map.
  _anchor: the anchor name; borrowed.
  _page:   the page it landed on.
Return:
  true when recorded; false when the map is full, which the caller reports as
SINK_FULL -- a mechanical failure, not a formal one.
*/
bool
d_doc_page_map_record(
    struct d_doc_page_map* _map,
    const char*            _anchor,
    uint32_t               _page
)
{
    if ( (!_map)          ||
         (!_map->entries) ||
         (_map->count >= _map->capacity) )
    {
        return false;
    }

    _map->entries[_map->count].anchor   = _anchor;
    _map->entries[_map->count].page     = _page;
    _map->entries[_map->count].reserved = 0u;
    ++_map->count;

    if (_page > _map->max_page)
    {
        _map->max_page = _page;
    }

    return true;
}

uint32_t
d_doc_page_map_find(
    const struct d_doc_page_map* _map,
    const char*                  _anchor
)
{
    uint32_t index;

    if ( (!_map) ||
         (!_map->entries) )
    {
        return 0u;
    }

    for (index = 0u; index < _map->count; ++index)
    {
        if (d_doc_attr_key_compare(_map->entries[index].anchor,
                                   _anchor) == 0)
        {
            return _map->entries[index].page;
        }
    }

    return 0u;
}

/*
d_doc_page_map_fits
  Whether the discovered page count fits the reservation.

Parameter(s):
  _map: the map.
Return:
  true when max_page has no more digits than were reserved.
Note:
  This is the whole of the two-pass protocol's soundness condition.  If it
fails, pass two's numbers are wider than pass one's placeholders, every
subsequent line shifts, and the contents page describes a document that was
never emitted.  The caller returns D_DOC_RENDER_RESERVATION rather than
emitting it.
*/
bool
d_doc_page_map_fits(
    const struct d_doc_page_map* _map
)
{
    uint32_t digits;
    uint32_t value;

    if (!_map)
    {
        return false;
    }

    digits = 1u;
    value  = _map->max_page;

    while (value >= 10u)
    {
        value /= 10u;
        ++digits;
    }

    return (digits <= _map->page_digits);
}

/*
d_doc_page_map_agree
  Whether two recording passes produced the same assignments.

Parameter(s):
  _left / _right: the two maps.
Return:
  true when both record the same anchors, in the same order, on the same
pages.

Note:
  THIS IS THE CHECK THAT MAKES THE PROTOCOL A PROTOCOL.  The obligation says
the two passes must agree on layout; without this it is a hope, and a
disagreement shows up as a contents page that is wrong in a way no test would
catch.  Order is compared as well as content, because two passes that visited
the same anchors in a different order did not lay out the same document.
*/
bool
d_doc_page_map_agree(
    const struct d_doc_page_map* _left,
    const struct d_doc_page_map* _right
)
{
    uint32_t index;

    if ( (!_left) ||
         (!_right) )
    {
        return false;
    }

    if ( (_left->count       != _right->count) ||
         (_left->max_page    != _right->max_page) ||
         (_left->page_digits != _right->page_digits) )
    {
        return false;
    }

    for (index = 0u; index < _left->count; ++index)
    {
        if (_left->entries[index].page != _right->entries[index].page)
        {
            return false;
        }

        if (d_doc_attr_key_compare(_left->entries[index].anchor,
                                   _right->entries[index].anchor) != 0)
        {
            return false;
        }
    }

    return true;
}

/*
d_doc_page_placeholder
  The string every page number renders as during the discovery pass: exactly
`page_digits` zeroes.

Parameter(s):
  _map: the map, for its reserved width.
Return:
  A static string of the reserved width.  Never NULL.
Note:
  Zeroes rather than spaces, deliberately: a placeholder that is visibly wrong
in a draft render is better than one that looks like correct blank space.
*/
const char*
d_doc_page_placeholder(
    const struct d_doc_page_map* _map
)
{
    uint32_t width;
    uint32_t index;

    width = (_map && _map->page_digits) ? _map->page_digits : 1u;

    if (width > D_DOC_PAGE_PLACEHOLDER_MAX)
    {
        width = D_DOC_PAGE_PLACEHOLDER_MAX;
    }

    for (index = 0u; index < width; ++index)
    {
        g_doc_placeholder[index] = '0';
    }

    g_doc_placeholder[width] = '\0';

    return g_doc_placeholder;
}

/*
d_doc_page_map_digits
  How many decimal digits _value needs.  Exposed because a caller sizing a
reservation wants it and should not re-derive it.

Parameter(s):
  _value: the number.
Return:
  1 for 0..9, 2 for 10..99, and so on.
*/
uint32_t
d_doc_page_map_digits(
    uint32_t _value
)
{
    uint32_t digits;

    digits = 1u;

    while (_value >= 10u)
    {
        _value /= 10u;
        ++digits;
    }

    return digits;
}

// doc_page_scalar
//   helper: the wrapped scalar lookup.  `page.<anchor>` comes from the map --
// as the placeholder during discovery, as the real number during emission --
// and everything else falls through untouched.
static const char*
doc_page_scalar(
    const char* _name,
    void*       _context
)
{
    static char              rendered[16];
    struct d_doc_page_scope* scope;
    const char*              anchor;
    uint32_t                 page;
    uint32_t                 digits;
    uint32_t                 index;

    scope = (struct d_doc_page_scope*)_context;

    if ( (!scope) ||
         (!_name) )
    {
        return NULL;
    }

    if ( (_name[0] != 'p') || (_name[1] != 'a') || (_name[2] != 'g') ||
         (_name[3] != 'e') || (_name[4] != '.') )
    {
        return ( scope->base && scope->base->scalar )
             ? scope->base->scalar(_name, scope->base->context)
             : NULL;
    }

    anchor = _name + 5;

    // discovery: every page renders at the RESERVED width, whatever it will
    // turn out to be.  That is the entire mechanism -- the extent cannot then
    // depend on the value
    if (!scope->resolved)
    {
        return d_doc_page_placeholder(scope->map);
    }

    page   = d_doc_page_map_find(scope->map, anchor);
    digits = scope->map ? scope->map->page_digits : 1u;

    if (digits > 15u)
    {
        digits = 15u;
    }

    // right-aligned into the reserved width, so a two-digit number in a
    // three-digit reservation occupies three columns and the layout agrees
    // with the pass that measured it
    for (index = 0u; index < digits; ++index)
    {
        rendered[index] = ' ';
    }

    rendered[digits] = '\0';
    index            = digits;

    do
    {
        --index;
        rendered[index] = (char)('0' + (int)(page % 10u));
        page           /= 10u;
    }
    while ( (page > 0u) &&
            (index > 0u) );

    return rendered;
}

/*
d_doc_page_environment
  The caller's environment with page numbers layered over it.

Parameter(s):
  _scope: the wrapper state.
Return:
  An environment whose scalar resolves `page.<anchor>` and delegates the rest.
*/
struct d_doc_environment
d_doc_page_environment(
    struct d_doc_page_scope* _scope
)
{
    struct d_doc_environment env;

    env.scalar  = doc_page_scalar;
    env.count   = ( _scope && _scope->base ) ? _scope->base->count : NULL;
    env.field   = ( _scope && _scope->base ) ? _scope->base->field : NULL;
    env.context = _scope;

    return env;
}

/*
d_doc_two_pass
  The two-pass driver: discover, check the reservation, verify agreement, then
and only then emit.

Parameter(s):
  _template:       the tree, with `{page.<anchor>}` tokens.
  _env:            the caller's environment; may be NULL.
  _arena:          storage for both expansions.
  _record:         the recording pass; supplied by the emission side.
  _record_context: forwarded to _record.
  _discover:       the map pass one fills; page_digits must be set.
  _verify:         a second map, for the agreement check.
  _emit:           the dialect that actually produces bytes.
Return:
  D_DOC_RENDER_OK, or D_DOC_RENDER_RESERVATION when the discovered page count
outgrew the reservation or the two passes disagreed, or whatever a pass
returned.

Note:
  NOTHING IS EMITTED UNLESS BOTH CHECKS PASS.  A driver that rendered first and
checked afterwards would produce a document whose contents page describes a
different document -- which is the failure the protocol exists to prevent, and
which is invisible in the output.
*/
enum d_doc_render_status
d_doc_two_pass(
    const struct d_doc_node*           _template,
    const struct d_doc_environment*    _env,
    struct d_doc_arena*                _arena,
    fn_doc_record_pass                 _record,
    void*                              _record_context,
    struct d_doc_page_map*             _discover,
    struct d_doc_page_map*             _verify,
    const struct d_doc_render_algebra* _emit
)
{
    struct d_doc_page_scope  scope;
    struct d_doc_environment wrapped;
    const struct d_doc_node* expanded;
    enum d_doc_render_status status;

    if ( (!_template) || (!_record) || (!_discover) ||
         (!_verify)   || (!_emit) )
    {
        return D_DOC_RENDER_MALFORMED;
    }

    scope.base     = _env;
    scope.map      = _discover;
    scope.reserved = 0;

    // -- pass one: placeholders of the reserved width, then discover --------

    scope.resolved = 0;
    wrapped        = d_doc_page_environment(&scope);

    d_doc_arena_reset(_arena);
    expanded = NULL;
    status   = d_doc_expand(_template, &wrapped, _arena, &expanded);

    if (status != D_DOC_RENDER_OK)
    {
        return status;
    }

    _discover->count    = 0u;
    _discover->max_page = 0u;
    status              = _record(expanded, _discover, _record_context);

    if (status != D_DOC_RENDER_OK)
    {
        return status;
    }

    // IS THE RECORDER ACTUALLY WIRED?  A tree with anchors whose discovery
    // pass found none means the recording hook is never being called -- and
    // everything downstream then succeeds vacuously: agreement over two empty
    // records passes, the reservation has nothing to exceed, and a document
    // ships with a contents page nothing checked.  Refuse instead.
    //
    //   The check is deliberately weak -- at least one, not exactly N.  A
    // recorder may legitimately not anchor every hinted node (one inside a
    // sub-tree its dialect drops, say), and a strict count would fail those
    // for the wrong reason.  Zero-from-nonzero has no legitimate reading.
    if ( (d_doc_node_anchor_count(expanded) > 0u) &&
         (_discover->count == 0u) )
    {
        return D_DOC_RENDER_RECORDER;
    }

    // the reservation is the protocol's soundness condition; a violation here
    // means every number after the boundary would shift
    if (!d_doc_page_map_fits(_discover))
    {
        return D_DOC_RENDER_RESERVATION;
    }

    // -- pass two: the real numbers, at the same width ----------------------

    scope.resolved = 1;
    wrapped        = d_doc_page_environment(&scope);

    d_doc_arena_reset(_arena);
    expanded = NULL;
    status   = d_doc_expand(_template, &wrapped, _arena, &expanded);

    if (status != D_DOC_RENDER_OK)
    {
        return status;
    }

    // -- verify before committing ------------------------------------------
    //   Without this the obligation "the two passes agree on layout" is a
    // hope.  With it, a disagreement costs one extra fold and fails loudly
    // instead of shipping a wrong contents page.

    _verify->page_digits = _discover->page_digits;
    _verify->count       = 0u;
    _verify->max_page    = 0u;
    status               = _record(expanded, _verify, _record_context);

    if (status != D_DOC_RENDER_OK)
    {
        return status;
    }

    if (!d_doc_page_map_agree(_discover, _verify))
    {
        return D_DOC_RENDER_RESERVATION;
    }

    return d_doc_render(expanded, _emit);
}


// ===========================================================================
// V.     defaults
// ===========================================================================
//   What a NULL verb does.  These are observable in the bytes, so they are
// interface (document_render_algebra.h section V) and not implementation.

static enum d_doc_render_status
doc_write_line(
    const struct d_doc_render_algebra* _algebra,
    const char*                        _text,
    const struct d_doc_attributes*     _attrs
)
{
    // the one primitive: its absence is a programmer error, and is reported
    // as a FORMAL failure because the algebra, not the tree, is wrong
    if (!_algebra->write_line)
    {
        return D_DOC_RENDER_NO_PRIMITIVE;
    }

    return (enum d_doc_render_status)_algebra->write_line(
               doc_text_or_empty(_text), _attrs, _algebra->context);
}

// doc_default_key_value
//   helper: the "<key>: <value>" funnel.  Joins into stack scratch and
// truncates rather than allocating: the default exists to make a
// one-function dialect possible, not to be a good renderer.
static enum d_doc_render_status
doc_default_key_value(
    const struct d_doc_render_algebra* _algebra,
    const char*                        _key,
    const char*                        _value,
    const struct d_doc_attributes*     _attrs
)
{
    char        line[D_INTERNAL_DOC_KV_BUFFER];
    const char* source;
    size_t      used;

    used   = 0u;
    source = doc_text_or_empty(_key);

    while ( (*source) &&
            (used < (sizeof(line) - 1u)) )
    {
        line[used] = (*source);
        ++used;
        ++source;
    }

    source = ": ";

    while ( (*source) &&
            (used < (sizeof(line) - 1u)) )
    {
        line[used] = (*source);
        ++used;
        ++source;
    }

    source = doc_text_or_empty(_value);

    while ( (*source) &&
            (used < (sizeof(line) - 1u)) )
    {
        line[used] = (*source);
        ++used;
        ++source;
    }

    line[used] = '\0';

    return doc_write_line(_algebra, line, _attrs);
}


// ===========================================================================
// VI.    the fold
// ===========================================================================

// doc_fold
//   the catamorphism.  One switch, one recursion, every dialect.  Arguments
// are visited in declaration order, which is the entirety of the emission-
// order guarantee: it is a property of the walk over the data, not a rule the
// walk remembers to obey.
static enum d_doc_render_status
doc_fold(
    const struct d_doc_node*           _node,
    const struct d_doc_render_algebra* _algebra,
    uint32_t                           _depth
)
{
    enum d_doc_render_status status;
    uint32_t                 index;
    bool                     seen_row;

    if (!_node)
    {
        return D_DOC_RENDER_MALFORMED;
    }

    // bounded, so a cyclic or pathological tree fails loudly instead of
    // exhausting the stack.  This is a MECHANICAL limit, not a formal one
    if (_depth > D_INTERNAL_DOC_MAX_DEPTH)
    {
        return D_DOC_RENDER_DEPTH_EXCEEDED;
    }

    if ( (_node->child_count > 0u) &&
         (!_node->children) )
    {
        return D_DOC_RENDER_MALFORMED;
    }

    // an unordered hint bag would emit attributes in a different order than a
    // canonically ordered one, and nothing downstream would notice.  Off in a
    // release build; on where it can still be fixed
#if D_INTERNAL_DOC_CHECK_CANONICAL
    if (!d_doc_attributes_is_canonical(&_node->attrs))
    {
        return D_DOC_RENDER_MALFORMED;
    }
#endif

    status = D_DOC_RENDER_OK;

    switch ((enum d_doc_node_kind)_node->kind)
    {
        case D_DOC_KIND_DOCUMENT:
        {
            if (_algebra->begin_document)
            {
                status = (enum d_doc_render_status)_algebra->begin_document(&_node->attrs,
                                                  _algebra->context);
            }

            for (index = 0u;
                 (index < _node->child_count) &&
                     (status == D_DOC_RENDER_OK);
                 ++index)
            {
                status = doc_fold(_node->children[index], _algebra,
                                  _depth + 1u);
            }

            if ( (status == D_DOC_RENDER_OK) &&
                 (_algebra->end_document) )
            {
                status = (enum d_doc_render_status)_algebra->end_document(_algebra->context);
            }

            break;
        }

        case D_DOC_KIND_ELEMENT:
        {
            if (_algebra->begin_element)
            {
                status = (enum d_doc_render_status)_algebra->begin_element(
                             doc_text_or_empty(_node->text),
                             doc_text_or_empty(_node->secondary),
                             &_node->attrs,
                             _algebra->context);
            }
            else if ( _node->secondary &&
                      (_node->secondary[0] != '\0') )
            {
                status = doc_write_line(_algebra, _node->secondary,
                                        &_node->attrs);
            }

            for (index = 0u;
                 (index < _node->child_count) &&
                     (status == D_DOC_RENDER_OK);
                 ++index)
            {
                status = doc_fold(_node->children[index], _algebra,
                                  _depth + 1u);
            }

            if ( (status == D_DOC_RENDER_OK) &&
                 (_algebra->end_element) )
            {
                status = (enum d_doc_render_status)_algebra->end_element(doc_text_or_empty(_node->text),
                                               _algebra->context);
            }

            break;
        }

        case D_DOC_KIND_HEADING:
        {
            status = _algebra->heading
                   ? (enum d_doc_render_status)_algebra->heading(_node->level,
                                       doc_text_or_empty(_node->text),
                                       &_node->attrs, _algebra->context)
                   : doc_write_line(_algebra, _node->text, &_node->attrs);

            break;
        }

        case D_DOC_KIND_PARAGRAPH:
        {
            status = _algebra->paragraph
                   ? (enum d_doc_render_status)_algebra->paragraph(
                         doc_text_or_empty(_node->text),
                                         &_node->attrs, _algebra->context)
                   : doc_write_line(_algebra, _node->text, &_node->attrs);

            break;
        }

        case D_DOC_KIND_KEY_VALUE:
        {
            status = _algebra->key_value
                   ? (enum d_doc_render_status)_algebra->key_value(
                         doc_text_or_empty(_node->text),
                                         doc_text_or_empty(_node->secondary),
                                         &_node->attrs, _algebra->context)
                   : doc_default_key_value(_algebra, _node->text,
                                           _node->secondary, &_node->attrs);

            break;
        }

        case D_DOC_KIND_RULE:
        {
            if (_algebra->rule)
            {
                status = (enum d_doc_render_status)_algebra->rule(&_node->attrs, _algebra->context);
            }

            break;
        }

        case D_DOC_KIND_SPACE:
        {
            if (_algebra->space)
            {
                status = (enum d_doc_render_status)_algebra->space(
                             _node->millipoints, &_node->attrs,
                             _algebra->context);
            }

            break;
        }

        case D_DOC_KIND_PAGE_BREAK:
        {
            if (_algebra->page_break)
            {
                status = (enum d_doc_render_status)_algebra->page_break(_algebra->context);
            }

            break;
        }

        case D_DOC_KIND_LIST:
        {
            if (_algebra->begin_list)
            {
                status = (enum d_doc_render_status)_algebra->begin_list(
                             (_node->flags & D_DOC_FLAG_ORDERED) ? 1 : 0,
                             &_node->attrs, _algebra->context);
            }

            for (index = 0u;
                 (index < _node->child_count) &&
                     (status == D_DOC_RENDER_OK);
                 ++index)
            {
                status = doc_fold(_node->children[index], _algebra,
                                  _depth + 1u);
            }

            if ( (status == D_DOC_RENDER_OK) &&
                 (_algebra->end_list) )
            {
                status = (enum d_doc_render_status)_algebra->end_list(_algebra->context);
            }

            break;
        }

        case D_DOC_KIND_ITEM:
        {
            status = _algebra->begin_item
                   ? (enum d_doc_render_status)_algebra->begin_item(
                         doc_text_or_empty(_node->text),
                                          &_node->attrs, _algebra->context)
                   : doc_write_line(_algebra, _node->text, &_node->attrs);

            for (index = 0u;
                 (index < _node->child_count) &&
                     (status == D_DOC_RENDER_OK);
                 ++index)
            {
                status = doc_fold(_node->children[index], _algebra,
                                  _depth + 1u);
            }

            if ( (status == D_DOC_RENDER_OK) &&
                 (_algebra->end_item) )
            {
                status = (enum d_doc_render_status)_algebra->end_item(_algebra->context);
            }

            break;
        }

        case D_DOC_KIND_TABLE:
        {
            if (_algebra->begin_table)
            {
                status = (enum d_doc_render_status)_algebra->begin_table(&_node->attrs,
                                               _algebra->context);
            }

            seen_row = false;

            // columns THEN rows, and a column after a row is malformed.  The
            // ordering is the data's; this loop only refuses to paper over a
            // tree that does not have it
            for (index = 0u;
                 (index < _node->child_count) &&
                     (status == D_DOC_RENDER_OK);
                 ++index)
            {
                const struct d_doc_node* child = _node->children[index];

                if (!child)
                {
                    status = D_DOC_RENDER_MALFORMED;

                    break;
                }

                if ( (child->kind == (uint32_t)D_DOC_KIND_COLUMN) ||
                     (child->kind ==
                          (uint32_t)D_DOC_KIND_COLUMN_GROUP) )
                {
                    if (seen_row)
                    {
                        status = D_DOC_RENDER_MALFORMED;

                        break;
                    }

                    status = doc_fold(child, _algebra, _depth + 1u);
                }
                else if (child->kind == (uint32_t)D_DOC_KIND_ROW)
                {
                    seen_row = true;
                    status   = doc_fold(child, _algebra, _depth + 1u);
                }
                else
                {
                    status = D_DOC_RENDER_MALFORMED;
                }
            }

            if ( (status == D_DOC_RENDER_OK) &&
                 (_algebra->end_table) )
            {
                status = (enum d_doc_render_status)_algebra->end_table(_algebra->context);
            }

            break;
        }

        case D_DOC_KIND_ROW:
        {
            if (_algebra->begin_row)
            {
                status = (enum d_doc_render_status)_algebra->begin_row(&_node->attrs,
                                             _algebra->context);
            }

            for (index = 0u;
                 (index < _node->child_count) &&
                     (status == D_DOC_RENDER_OK);
                 ++index)
            {
                const struct d_doc_node* child = _node->children[index];

                if ( (!child) ||
                     (child->kind != (uint32_t)D_DOC_KIND_CELL) )
                {
                    status = D_DOC_RENDER_MALFORMED;

                    break;
                }

                if (_algebra->cell)
                {
                    status = (enum d_doc_render_status)_algebra->cell(doc_text_or_empty(child->text),
                                            &child->attrs,
                                            _algebra->context);
                }
            }

            if ( (status == D_DOC_RENDER_OK) &&
                 (_algebra->end_row) )
            {
                status = (enum d_doc_render_status)_algebra->end_row(_algebra->context);
            }

            break;
        }

        case D_DOC_KIND_COLUMN_GROUP:
        {
            if (_algebra->begin_column_group)
            {
                status = (enum d_doc_render_status)
                             _algebra->begin_column_group(
                                 doc_text_or_empty(_node->text),
                                 d_doc_column_span(_node),
                                 &_node->attrs, _algebra->context);
            }

            // the columns beneath are visited whether or not the dialect
            // expressed the group, so an ungrouping dialect sees exactly the
            // flat run it saw before groups existed
            for (index = 0u;
                 (index < _node->child_count) &&
                     (status == D_DOC_RENDER_OK);
                 ++index)
            {
                status = doc_fold(_node->children[index], _algebra,
                                  _depth + 1u);
            }

            if ( (status == D_DOC_RENDER_OK) &&
                 (_algebra->end_column_group) )
            {
                status = (enum d_doc_render_status)
                             _algebra->end_column_group(_algebra->context);
            }

            break;
        }

        case D_DOC_KIND_COLUMN:
        {
            if (_algebra->column)
            {
                status = (enum d_doc_render_status)_algebra->column(
                             doc_text_or_empty(_node->text),
                             &_node->attrs, _algebra->context);
            }

            break;
        }

        case D_DOC_KIND_CELL:
        {
            // reached only outside a table, where a column or a cell has no
            // meaning: a sort error, and therefore FORMAL
            status = D_DOC_RENDER_MALFORMED;

            break;
        }

        case D_DOC_KIND_SLOT:
        case D_DOC_KIND_REPEAT:
        {
            // a residual hole and an unexpanded binder emit NOTHING and do not
            // stop the fold (ch-documents.tex, non-property 4).  This is a
            // formal decision, not an oversight: a partially bound template
            // renders rather than failing
            break;
        }

        default:
        {
            status = D_DOC_RENDER_MALFORMED;

            break;
        }
    }

    return status;
}

/*
d_doc_render
  Folds a whole document through a dialect, opening and closing the frame
around it so that the result stands alone.

Parameter(s):
  _root:    the document; must be a D_DOC_KIND_DOCUMENT node.
  _algebra: the dialect; must have a non-NULL write_line.
Return:
  D_DOC_RENDER_OK, or the first non-OK status any verb returned.
*/
enum d_doc_render_status
d_doc_render(
    const struct d_doc_node*           _root,
    const struct d_doc_render_algebra* _algebra
)
{
    if ( (!_root) ||
         (!_algebra) )
    {
        return D_DOC_RENDER_MALFORMED;
    }

    if (!_algebra->write_line)
    {
        return D_DOC_RENDER_NO_PRIMITIVE;
    }

    if (_root->kind != (uint32_t)D_DOC_KIND_DOCUMENT)
    {
        return D_DOC_RENDER_MALFORMED;
    }

    return doc_fold(_root, _algebra, 0u);
}

/*
d_doc_render_block
  Folds one block WITHOUT the document frame -- what a slot filler emits into
an in-progress render, and what a fixture uses to check one constructor in
isolation.

Parameter(s):
  _block:   any node of block sort.
  _algebra: the dialect.
Return:
  As d_doc_render.
*/
enum d_doc_render_status
d_doc_render_block(
    const struct d_doc_node*           _block,
    const struct d_doc_render_algebra* _algebra
)
{
    if ( (!_block) ||
         (!_algebra) )
    {
        return D_DOC_RENDER_MALFORMED;
    }

    if (!_algebra->write_line)
    {
        return D_DOC_RENDER_NO_PRIMITIVE;
    }

    return doc_fold(_block, _algebra, 0u);
}
