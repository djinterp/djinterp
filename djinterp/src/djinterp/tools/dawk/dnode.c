/*******************************************************************************
* djinterp [dawk]                                                        dnode.c
*
* Node tree:
*   Growable arrays of nodes and attributes plus an intern table, all indexed
* by uint32_t.  Children are appended in document order and linked through
* next_sibling, so a build is append-only and never rewrites an earlier node.
*   The intern table is a linear probe over FNV-1a.  Trees here hold a few
* dozen nodes per file, so the table is rebuilt per file rather than shared;
* sharing it across files is the obvious optimisation and is deliberately not
* done yet, because a shared table outlives a tree and that lifetime question
* deserves its own decision.
*
* path:      /src/djinterp/tools/dawk/dnode.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dnode.h"  // corresponding header
// std
#include <stdlib.h>  // malloc, realloc, free, calloc
#include <string.h>  // memcpy, memcmp, strlen


// d_node_tree
//   struct: the arrays and the intern table behind them.
struct d_node_tree
{
    struct d_node*            nodes;
    size_t                    node_count;
    size_t                    node_capacity;

    struct d_node_attribute*  attributes;
    size_t                    attribute_count;
    size_t                    attribute_capacity;

    char*                     text;
    size_t                    text_used;
    size_t                    text_capacity;

    uint32_t*                 offsets;
    size_t                    id_count;
    size_t                    id_capacity;

    uint32_t*                 buckets;
    size_t                    bucket_mask;

    uint32_t                  next_origin;
};


/*
d_internal_node_hash
  FNV-1a over a counted string.
*/
static uint32_t
d_internal_node_hash(
    const char* _text,
    size_t      _length
)
{
    uint32_t hash = 2166136261u;

    for (size_t at = 0; at < _length; ++at)
    {
        hash ^= (unsigned char)_text[at];
        hash *= 16777619u;
    }

    return hash;
}


/*
d_internal_node_rehash
  Doubles the bucket array and refiles every identifier.
*/
static bool
d_internal_node_rehash(
    struct d_node_tree* _tree
)
{
    const size_t grown = (_tree->bucket_mask == 0)
                       ? 128u
                       : ((_tree->bucket_mask + 1u) * 2u);

    uint32_t* const buckets = malloc(grown * sizeof(uint32_t));

    if (!buckets)
    {
        return false;
    }

    for (size_t at = 0; at < grown; ++at)
    {
        buckets[at] = D_DSS_NO_INDEX;
    }

    free(_tree->buckets);

    _tree->buckets     = buckets;
    _tree->bucket_mask = grown - 1u;

    for (uint32_t id = 0; id < (uint32_t)_tree->id_count; ++id)
    {
        const char* const text = _tree->text + _tree->offsets[id];

        size_t slot = d_internal_node_hash(text, strlen(text))
                    & _tree->bucket_mask;

        while (_tree->buckets[slot] != D_DSS_NO_INDEX)
        {
            slot = (slot + 1u) & _tree->bucket_mask;
        }

        _tree->buckets[slot] = id;
    }

    return true;
}


/*
d_node_intern
  Returns the identifier for a counted string, adding it when new.
*/
uint32_t
d_node_intern(
    struct d_node_tree* _tree,
    const char*         _text,
    size_t              _length
)
{
    if ((!_tree) || (!_text))
    {
        return D_DSS_NO_INDEX;
    }

    if ((_tree->bucket_mask == 0) && (!d_internal_node_rehash(_tree)))
    {
        return D_DSS_NO_INDEX;
    }

    // two thirds full is where linear probing starts to cluster
    if (((_tree->id_count + 1u) * 3u) > ((_tree->bucket_mask + 1u) * 2u))
    {
        if (!d_internal_node_rehash(_tree))
        {
            return D_DSS_NO_INDEX;
        }
    }

    size_t slot = d_internal_node_hash(_text, _length) & _tree->bucket_mask;

    while (_tree->buckets[slot] != D_DSS_NO_INDEX)
    {
        const uint32_t    id    = _tree->buckets[slot];
        const char* const found = _tree->text + _tree->offsets[id];

        if ((strlen(found) == _length) && (memcmp(found, _text, _length) == 0))
        {
            return id;
        }

        slot = (slot + 1u) & _tree->bucket_mask;
    }

    if ((_tree->text_used + _length + 1u) > _tree->text_capacity)
    {
        size_t grown = (_tree->text_capacity == 0) ? 2048u
                                                   : _tree->text_capacity;

        while (grown < (_tree->text_used + _length + 1u))
        {
            grown *= 2u;
        }

        char* const text = realloc(_tree->text, grown);

        if (!text)
        {
            return D_DSS_NO_INDEX;
        }

        _tree->text          = text;
        _tree->text_capacity = grown;
    }

    if (_tree->id_count == _tree->id_capacity)
    {
        const size_t grown = (_tree->id_capacity == 0) ? 64u
                                                       : (_tree->id_capacity
                                                          * 2u);

        uint32_t* const offsets = realloc(_tree->offsets,
                                          grown * sizeof(uint32_t));

        if (!offsets)
        {
            return D_DSS_NO_INDEX;
        }

        _tree->offsets     = offsets;
        _tree->id_capacity = grown;
    }

    const uint32_t id = (uint32_t)_tree->id_count;

    _tree->offsets[id] = (uint32_t)_tree->text_used;

    memcpy(_tree->text + _tree->text_used, _text, _length);

    _tree->text[_tree->text_used + _length] = '\0';
    _tree->text_used                       += _length + 1u;

    ++_tree->id_count;

    _tree->buckets[slot] = id;

    return id;
}


/*
d_node_tree_new
  Allocates an empty tree.
*/
struct d_node_tree*
d_node_tree_new(
    void
)
{
    return calloc(1u, sizeof(struct d_node_tree));
}


/*
d_node_tree_free
  Releases the tree and everything it owns.
*/
void
d_node_tree_free(
    struct d_node_tree* _tree
)
{
    if (_tree)
    {
        free(_tree->nodes);
        free(_tree->attributes);
        free(_tree->text);
        free(_tree->offsets);
        free(_tree->buckets);
        free(_tree);
    }

    return;
}


/*
d_node_tree_clear
  Empties the tree without releasing its allocations, so a walk over many
files reuses one set of buffers.  The origin counter is not reset: identity
stays unique across every file in a run.
*/
void
d_node_tree_clear(
    struct d_node_tree* _tree
)
{
    if (_tree)
    {
        _tree->node_count      = 0;
        _tree->attribute_count = 0;
        _tree->text_used       = 0;
        _tree->id_count        = 0;

        for (size_t at = 0; (_tree->buckets) && (at <= _tree->bucket_mask);
             ++at)
        {
            _tree->buckets[at] = D_DSS_NO_INDEX;
        }
    }

    return;
}


/*
d_node_add
  Appends a node beneath a parent and links it as the parent's last child.
Passing D_DSS_NO_INDEX as the parent creates a root.
*/
uint32_t
d_node_add(
    struct d_node_tree* _tree,
    uint32_t            _parent,
    const char*         _type
)
{
    if ((!_tree) || (!_type))
    {
        return D_DSS_NO_INDEX;
    }

    if (_tree->node_count == _tree->node_capacity)
    {
        const size_t grown = (_tree->node_capacity == 0)
                           ? 64u
                           : (_tree->node_capacity * 2u);

        struct d_node* const nodes = realloc(_tree->nodes,
                                             grown * sizeof(struct d_node));

        if (!nodes)
        {
            return D_DSS_NO_INDEX;
        }

        _tree->nodes         = nodes;
        _tree->node_capacity = grown;
    }

    const uint32_t at = (uint32_t)_tree->node_count;

    struct d_node* const node = &_tree->nodes[at];

    memset(node, 0, sizeof(*node));

    node->type            = d_node_intern(_tree, _type, strlen(_type));
    node->text            = D_DSS_NO_INDEX;
    node->parent          = _parent;
    node->first_child     = D_DSS_NO_INDEX;
    node->next_sibling    = D_DSS_NO_INDEX;
    node->first_attribute = D_DSS_NO_INDEX;
    node->origin          = _tree->next_origin;
    node->present         = true;

    ++_tree->next_origin;
    ++_tree->node_count;

    // link as the parent's last child so children stay in document order
    if (_parent != D_DSS_NO_INDEX)
    {
        struct d_node* const parent = &_tree->nodes[_parent];

        if (parent->first_child == D_DSS_NO_INDEX)
        {
            parent->first_child   = at;
            node->index_in_parent = 0;
        }
        else
        {
            uint32_t last  = parent->first_child;
            uint32_t count = 1;

            while (_tree->nodes[last].next_sibling != D_DSS_NO_INDEX)
            {
                last = _tree->nodes[last].next_sibling;
                ++count;
            }

            _tree->nodes[last].next_sibling = at;
            node->index_in_parent           = count;
        }
    }

    return at;
}


/*
d_node_set_attribute
  Attaches an interned name/value pair.  A value that parses wholly as a
number is kept as one too, so a numeric attribute comparison needs no second
conversion at match time.
*/
bool
d_node_set_attribute(
    struct d_node_tree* _tree,
    uint32_t            _node,
    const char*         _name,
    const char*         _value
)
{
    if ((!_tree) || (_node >= _tree->node_count) || (!_name))
    {
        return false;
    }

    if (_tree->attribute_count == _tree->attribute_capacity)
    {
        const size_t grown = (_tree->attribute_capacity == 0)
                           ? 64u
                           : (_tree->attribute_capacity * 2u);

        struct d_node_attribute* const grown_data =
            realloc(_tree->attributes,
                    grown * sizeof(struct d_node_attribute));

        if (!grown_data)
        {
            return false;
        }

        _tree->attributes         = grown_data;
        _tree->attribute_capacity = grown;
    }

    const uint32_t at = (uint32_t)_tree->attribute_count;

    struct d_node_attribute* const attribute = &_tree->attributes[at];

    attribute->name    = d_node_intern(_tree, _name, strlen(_name));
    attribute->value   = _value ? d_node_intern(_tree, _value, strlen(_value))
                                : D_DSS_NO_INDEX;
    attribute->number  = 0.0;
    attribute->numeric = false;

    if (_value)
    {
        char* end = NULL;

        const double parsed = strtod(_value, &end);

        if ((end != _value) && (*end == '\0'))
        {
            attribute->number  = parsed;
            attribute->numeric = true;
        }
    }

    struct d_node* const node = &_tree->nodes[_node];

    // attributes of one node are contiguous, so the first is recorded once
    if (node->first_attribute == D_DSS_NO_INDEX)
    {
        node->first_attribute = at;
    }

    ++node->attribute_count;
    ++_tree->attribute_count;

    return true;
}


/*
d_node_set_text
  Interns a node's own source text.
*/
bool
d_node_set_text(
    struct d_node_tree* _tree,
    uint32_t            _node,
    const char*         _text,
    size_t              _length
)
{
    if ((!_tree) || (_node >= _tree->node_count) || (!_text))
    {
        return false;
    }

    _tree->nodes[_node].text = d_node_intern(_tree, _text, _length);

    return (_tree->nodes[_node].text != D_DSS_NO_INDEX);
}


size_t
d_node_count(const struct d_node_tree* _tree)
{
    return _tree ? _tree->node_count : 0u;
}

struct d_node*
d_node_at(struct d_node_tree* _tree, uint32_t _at)
{
    if ((!_tree) || (_at >= _tree->node_count))
    {
        return NULL;
    }

    return &_tree->nodes[_at];
}

const struct d_node_attribute*
d_node_attribute_at(const struct d_node_tree* _tree, uint32_t _at)
{
    if ((!_tree) || (_at >= _tree->attribute_count))
    {
        return NULL;
    }

    return &_tree->attributes[_at];
}

const char*
d_node_text(const struct d_node_tree* _tree, uint32_t _id)
{
    if ((!_tree) || (!_tree->offsets) || (_id >= _tree->id_count))
    {
        return "";
    }

    return _tree->text + _tree->offsets[_id];
}

uint32_t
d_node_find_id(const struct d_node_tree* _tree, const char* _text)
{
    if ((!_tree) || (!_text) || (_tree->bucket_mask == 0))
    {
        return D_DSS_NO_INDEX;
    }

    const size_t length = strlen(_text);

    size_t slot = d_internal_node_hash(_text, length) & _tree->bucket_mask;

    while (_tree->buckets[slot] != D_DSS_NO_INDEX)
    {
        const uint32_t    id    = _tree->buckets[slot];
        const char* const found = _tree->text + _tree->offsets[id];

        if ((strlen(found) == length) && (memcmp(found, _text, length) == 0))
        {
            return id;
        }

        slot = (slot + 1u) & _tree->bucket_mask;
    }

    return D_DSS_NO_INDEX;
}
