/******************************************************************************
* djinterp [parsegen]                                               grammar.c
*
*   Definitions for the non-inline declarations in grammar.h.
*
*
* path:      /src/djinterp/parsegen/grammar.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../inc/djinterp/parsegen/grammar.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memset, strcmp
// djinterp
#include "../../../inc/djinterp/parse/storage.h"  // d_parse_grow, the shared
                                                  // growth policy
#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
#include <stdlib.h>  // malloc, free
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP


// D_INTERNAL_GRAMMAR_MESSAGE
//   macro: the size of the buffer a diagnostic is composed in before it is
// emitted, so every message here is built the same way whether or not the sink
// offers a printf entry point.
#define D_INTERNAL_GRAMMAR_MESSAGE  192u

// D_INTERNAL_GRAMMAR_PRIMARY
//   macro: the binding strength of a terminal or a parenthesised group -- the
// tightest, so it never needs parentheses of its own.
#define D_INTERNAL_GRAMMAR_PRIMARY  4u


/*
d_parsegen_kind_info
  Everything this level knows about a node kind.
NOTE:
  One table, three readers: the builders take `implies` from it, verification
takes `arity`, and rendering takes `name` and `precedence`. Adding a node kind
is adding a row here and a builder beside it -- nothing switches on kind.

Parameter(s):
  _kind: a d_parsegen_node_kind value.
Return:
  A pointer to the descriptor, or NULL for a kind this level does not name --
including the range reserved for out-of-tree kinds.
*/
const struct d_parsegen_node_info*
d_parsegen_kind_info(
    int _kind
)
{
    static const struct d_parsegen_node_info table[D_PARSEGEN_NODE_KIND_COUNT] =
    {
        { "empty",     D_PARSEGEN_ARITY_NONE, 4u, { 0u, 0u },
          D_PARSEGEN_EMPTY_PRODUCTION },
        { "any",       D_PARSEGEN_ARITY_NONE, 4u, { 0u, 0u },
          D_PARSEGEN_FEATURE_NONE },
        { "class",     D_PARSEGEN_ARITY_NONE, 4u, { 0u, 0u },
          D_PARSEGEN_CHARACTER_CLASS },
        { "literal",   D_PARSEGEN_ARITY_NONE, 4u, { 0u, 0u },
          D_PARSEGEN_FEATURE_NONE },
        { "ref",       D_PARSEGEN_ARITY_NONE, 4u, { 0u, 0u },
          D_PARSEGEN_FEATURE_NONE },
        { "sequence",  D_PARSEGEN_ARITY_MANY, 2u, { 0u, 0u },
          D_PARSEGEN_FEATURE_NONE },
        { "ordered",   D_PARSEGEN_ARITY_MANY, 1u, { 0u, 0u },
          D_PARSEGEN_ORDERED_CHOICE },
        { "unordered", D_PARSEGEN_ARITY_MANY, 1u, { 0u, 0u },
          D_PARSEGEN_UNORDERED_CHOICE },
        { "repeat",    D_PARSEGEN_ARITY_ONE,  3u, { 0u, 0u },
          D_PARSEGEN_FEATURE_NONE },
        { "capture",   D_PARSEGEN_ARITY_ONE,  4u, { 0u, 0u },
          D_PARSEGEN_CAPTURE },
        { "and",       D_PARSEGEN_ARITY_ONE,  3u, { 0u, 0u },
          D_PARSEGEN_SYNTACTIC_PREDICATE },
        { "not",       D_PARSEGEN_ARITY_ONE,  3u, { 0u, 0u },
          D_PARSEGEN_SYNTACTIC_PREDICATE },
        { "action",    D_PARSEGEN_ARITY_ONE,  4u, { 0u, 0u },
          D_PARSEGEN_HOST_ACTION }
    };

    if ( (_kind < 0) ||
         (_kind >= (int)D_PARSEGEN_NODE_KIND_COUNT) )
    {
        return NULL;
    }

    return &table[_kind];
}


/*
d_parsegen_grammar_init
  Initialises an empty grammar owning nothing.

Parameter(s):
  _grammar: the grammar to initialise; ignored if NULL.
Return:
  none.
*/
void
d_parsegen_grammar_init(
    struct d_parsegen_grammar* _grammar
)
{
    if (!_grammar)
    {
        return;
    }

    memset(_grammar, 0, sizeof(*_grammar));

    _grammar->notation = D_PARSE_POOL_NONE;

    d_parse_pool_init(&_grammar->pool, NULL, 0u, NULL, 0u);

    return;
}


#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)

/*
d_parsegen_grammar_init_heap
  Initialises a grammar over storage it allocates and owns -- nodes, rules, and
the pool their names and terminals live in -- all of which grow on demand.

Parameter(s):
  _grammar: the grammar to initialise; ignored if NULL.
  _nodes:   expression nodes to reserve; 0 selects a default.
  _rules:   rules to reserve; 0 selects a default.
Return:
  0 on success; -1 if _grammar is NULL or an allocation was refused.
*/
int
d_parsegen_grammar_init_heap(
    struct d_parsegen_grammar* _grammar,
    uint32_t                   _nodes,
    uint32_t                   _rules
)
{
    if (!_grammar)
    {
        return -1;
    }

    d_parsegen_grammar_init(_grammar);

    const uint32_t nodes = (_nodes > 0u) ? _nodes : 64u;
    const uint32_t rules = (_rules > 0u) ? _rules : 16u;

    struct d_parsegen_node* node_store =
        (struct d_parsegen_node*)malloc((size_t)nodes * sizeof(*node_store));

    // check if the node allocation was successful
    if (!node_store)
    {
        return -1;
    }

    struct d_parsegen_rule* rule_store =
        (struct d_parsegen_rule*)malloc((size_t)rules * sizeof(*rule_store));

    // check if the rule allocation was successful
    if (!rule_store)
    {
        free(node_store);

        return -1;
    }

    // check if the pool's storage was obtained
    if (d_parse_pool_init_heap(&_grammar->pool, 0u, 0u) != 0)
    {
        free(rule_store);
        free(node_store);

        return -1;
    }

    memset(node_store, 0, (size_t)nodes * sizeof(*node_store));
    memset(rule_store, 0, (size_t)rules * sizeof(*rule_store));

    _grammar->nodes         = node_store;
    _grammar->node_capacity = nodes;
    _grammar->rules         = rule_store;
    _grammar->rule_capacity = rules;
    _grammar->flags         = (uint8_t)(D_PARSEGEN_GRAMMAR_OWNS_NODES |
                                        D_PARSEGEN_GRAMMAR_OWNS_RULES);

    return 0;
}

#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP


/*
d_parsegen_grammar_reset
  Empties a grammar for reuse, keeping its storage.

Parameter(s):
  _grammar: the grammar to empty; ignored if NULL.
Return:
  none.
*/
void
d_parsegen_grammar_reset(
    struct d_parsegen_grammar* _grammar
)
{
    if (!_grammar)
    {
        return;
    }

    _grammar->node_count    = 0u;
    _grammar->rule_count    = 0u;
    _grammar->features      = D_PARSEGEN_FEATURE_NONE;
    _grammar->start         = 0u;
    _grammar->notation      = D_PARSE_POOL_NONE;
    _grammar->origin_offset = 0u;
    _grammar->origin_length = 0u;
    _grammar->flags         = (uint8_t)(_grammar->flags &
                                        ~(D_PARSEGEN_GRAMMAR_RESOLVED |
                                          D_PARSEGEN_GRAMMAR_VERIFIED));

    d_parse_pool_reset(&_grammar->pool);

    return;
}


/*
d_parsegen_grammar_release
  Releases any storage the grammar owns, its pool included, and leaves it
empty.

Parameter(s):
  _grammar: the grammar to release; ignored if NULL.
Return:
  none.
*/
void
d_parsegen_grammar_release(
    struct d_parsegen_grammar* _grammar
)
{
    if (!_grammar)
    {
        return;
    }

#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
    // free only what this grammar allocated; caller storage is never touched
    if ( (_grammar->nodes != NULL) &&
         ((_grammar->flags & D_PARSEGEN_GRAMMAR_OWNS_NODES) != 0u) )
    {
        free(_grammar->nodes);
    }

    if ( (_grammar->rules != NULL) &&
         ((_grammar->flags & D_PARSEGEN_GRAMMAR_OWNS_RULES) != 0u) )
    {
        free(_grammar->rules);
    }
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP

    d_parse_pool_release(&_grammar->pool);

    memset(_grammar, 0, sizeof(*_grammar));

    return;
}


/*
d_parsegen_grammar_at
  Sets the source range every node built afterwards is attributed to.
NOTE:
  A cursor rather than a parameter on every builder, because a frontend already
holds a current token position and threading a span through twelve construction
calls would be noise at every one of them.

Parameter(s):
  _grammar: the grammar to configure; ignored if NULL.
  _offset:  byte offset in the grammar's source text.
  _length:  byte length of the range.
Return:
  none.
*/
void
d_parsegen_grammar_at(
    struct d_parsegen_grammar* _grammar,
    uint32_t                   _offset,
    uint32_t                   _length
)
{
    if (!_grammar)
    {
        return;
    }

    _grammar->origin_offset = _offset;
    _grammar->origin_length = _length;

    return;
}


/*
d_parsegen_grammar_notation
  Records which notation this grammar was read from.
NOTE:
  Informational: it names the frontend for a diagnostic, and does not change
how anything reads the grammar. What a family needs to know is in `features`.

Parameter(s):
  _grammar: the grammar to label; ignored if NULL.
  _name:    the notation's name; may be NULL to clear.
Return:
  none.
*/
void
d_parsegen_grammar_notation(
    struct d_parsegen_grammar* _grammar,
    const char*                _name
)
{
    if (!_grammar)
    {
        return;
    }

    _grammar->notation = (_name != NULL)
                         ? d_parse_pool_intern_string(&_grammar->pool, _name)
                         : D_PARSE_POOL_NONE;

    return;
}


/*
d_parsegen_grammar_internal_node
  Appends a node, attributes it to the current origin, and folds the
capabilities its kind implies into the grammar's set.

Parameter(s):
  _grammar: the grammar to append to.
  _kind:    the node kind.
  _child:   the first child index, or D_PARSEGEN_NO_NODE.
  _a:       the first operand, as the kind's descriptor defines it.
  _b:       the second operand.
Return:
  The node's index, or D_PARSEGEN_NO_NODE when the arena could not grow.
*/
static int32_t
d_parsegen_grammar_internal_node(
    struct d_parsegen_grammar* _grammar,
    int                        _kind,
    int32_t                    _child,
    int32_t                    _a,
    int32_t                    _b
)
{
    if (!_grammar)
    {
        return D_PARSEGEN_NO_NODE;
    }

    // make room, or refuse
    if (_grammar->node_count >= _grammar->node_capacity)
    {
#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
        void* const grown =
            d_parse_grow(_grammar->nodes,
                         &_grammar->node_capacity,
                         _grammar->node_count + 1u,
                         (uint32_t)sizeof(*_grammar->nodes),
                         (_grammar->flags &
                          D_PARSEGEN_GRAMMAR_OWNS_NODES) != 0u);

        // check if memory allocation was successful
        if (!grown)
        {
            return D_PARSEGEN_NO_NODE;
        }

        _grammar->nodes = (struct d_parsegen_node*)grown;
#else
        return D_PARSEGEN_NO_NODE;
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP
    }

    const uint32_t index = _grammar->node_count;

    _grammar->nodes[index].kind   = (uint16_t)_kind;
    _grammar->nodes[index].flags  = 0u;
    _grammar->nodes[index].child  = _child;
    _grammar->nodes[index].next   = D_PARSEGEN_NO_NODE;
    _grammar->nodes[index].a      = _a;
    _grammar->nodes[index].b      = _b;
    _grammar->nodes[index].offset = _grammar->origin_offset;
    _grammar->nodes[index].length = _grammar->origin_length;

    _grammar->node_count++;

    // the capability set follows what was built, so a frontend cannot emit a
    // construct and forget to declare it
    const struct d_parsegen_node_info* const info =
        d_parsegen_kind_info(_kind);

    if (info)
    {
        _grammar->features |= info->implies;
    }

    // any construction invalidates the last resolution and verification
    _grammar->flags = (uint8_t)(_grammar->flags &
                                ~(D_PARSEGEN_GRAMMAR_RESOLVED |
                                  D_PARSEGEN_GRAMMAR_VERIFIED));

    return (int32_t)index;
}


/*
d_parsegen_grammar_internal_link
  Chains a list of children onto a node.

Parameter(s):
  _grammar:  the grammar holding the nodes.
  _parent:   the node to attach to.
  _children: the child indices, in order.
  _count:    how many.
Return:
  A boolean value corresponding to either:
  - 1, if every child was valid and the chain was built, or
  - 0, otherwise.
*/
static int
d_parsegen_grammar_internal_link(
    struct d_parsegen_grammar* _grammar,
    int32_t                    _parent,
    const int32_t*             _children,
    uint32_t                   _count
)
{
    int32_t previous = D_PARSEGEN_NO_NODE;

    for (uint32_t index = 0u; index < _count; index++)
    {
        const int32_t child = _children[index];

        // a child that is not a node would leave a dangling chain, so refuse
        // the whole construction rather than build half of it
        if (!d_parsegen_node_at(_grammar, child))
        {
            return 0;
        }

        if (previous == D_PARSEGEN_NO_NODE)
        {
            _grammar->nodes[_parent].child = child;
        }
        else
        {
            _grammar->nodes[previous].next = child;
        }

        previous = child;
    }

    return 1;
}


/*
d_parsegen_empty
  The expression matching nothing at all.

Parameter(s):
  _grammar: the grammar to build into; may be NULL.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_empty(
    struct d_parsegen_grammar* _grammar
)
{
    return d_parsegen_grammar_internal_node(_grammar,
                                            (int)D_PARSEGEN_NODE_EMPTY,
                                            D_PARSEGEN_NO_NODE,
                                            D_PARSEGEN_NO_NODE,
                                            D_PARSEGEN_NO_NODE);
}


/*
d_parsegen_any
  The expression matching any one symbol.

Parameter(s):
  _grammar: the grammar to build into; may be NULL.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_any(
    struct d_parsegen_grammar* _grammar
)
{
    return d_parsegen_grammar_internal_node(_grammar,
                                            (int)D_PARSEGEN_NODE_ANY,
                                            D_PARSEGEN_NO_NODE,
                                            D_PARSEGEN_NO_NODE,
                                            D_PARSEGEN_NO_NODE);
}


/*
d_parsegen_class
  The expression matching one symbol from a set.
NOTE:
  The class is interned, so two rules writing the same set share one entry and
a later pass comparing two first sets compares two integers.

Parameter(s):
  _grammar: the grammar to build into; may be NULL.
  _set:     the set of acceptable symbols; may be NULL.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_class(
    struct d_parsegen_grammar*    _grammar,
    const struct d_parse_charset* _set
)
{
    if ( (!_grammar) ||
         (!_set)     )
    {
        return D_PARSEGEN_NO_NODE;
    }

    const uint32_t interned = d_parse_pool_intern(&_grammar->pool,
                                                  _set->bits,
                                                  D_PARSE_CHARSET_BYTES);

    if (interned == D_PARSE_POOL_NONE)
    {
        return D_PARSEGEN_NO_NODE;
    }

    return d_parsegen_grammar_internal_node(_grammar,
                                            (int)D_PARSEGEN_NODE_CLASS,
                                            D_PARSEGEN_NO_NODE,
                                            (int32_t)interned,
                                            D_PARSEGEN_NO_NODE);
}


/*
d_parsegen_literal
  The expression matching an exact sequence of symbols.
NOTE:
  A literal of more than one symbol sets LITERAL_STRING, since a family that
only handles single symbols can express one and not the other -- which is a
distinction a capability query has to be able to make.

Parameter(s):
  _grammar: the grammar to build into; may be NULL.
  _text:    the text to match; may be NULL.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_literal(
    struct d_parsegen_grammar* _grammar,
    const char*                _text
)
{
    if ( (!_grammar) ||
         (!_text)    )
    {
        return D_PARSEGEN_NO_NODE;
    }

    const uint32_t interned = d_parse_pool_intern_string(&_grammar->pool,
                                                         _text);

    if (interned == D_PARSE_POOL_NONE)
    {
        return D_PARSEGEN_NO_NODE;
    }

    const int32_t node =
        d_parsegen_grammar_internal_node(_grammar,
                                         (int)D_PARSEGEN_NODE_LITERAL,
                                         D_PARSEGEN_NO_NODE,
                                         (int32_t)interned,
                                         D_PARSEGEN_NO_NODE);

    // a multi-symbol literal is a capability of its own, so declare it
    if ( (node != D_PARSEGEN_NO_NODE) &&
         (_text[0] != '\0')           &&
         (_text[1] != '\0')           )
    {
        _grammar->features |= D_PARSEGEN_LITERAL_STRING;
    }

    return node;
}


/*
d_parsegen_ref
  A reference to a rule by name.
NOTE:
  The second operand holds the rule this resolves to, or D_PARSEGEN_NO_NODE
until d_parsegen_grammar_resolve fills it. Keeping the name and the binding in
one node means a grammar can be built in any order and bound afterwards.

Parameter(s):
  _grammar: the grammar to build into; may be NULL.
  _name:    the rule's name; may be NULL.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_ref(
    struct d_parsegen_grammar* _grammar,
    const char*                _name
)
{
    if ( (!_grammar) ||
         (!_name)    )
    {
        return D_PARSEGEN_NO_NODE;
    }

    const uint32_t interned = d_parse_pool_intern_string(&_grammar->pool,
                                                         _name);

    if (interned == D_PARSE_POOL_NONE)
    {
        return D_PARSEGEN_NO_NODE;
    }

    return d_parsegen_grammar_internal_node(_grammar,
                                            (int)D_PARSEGEN_NODE_REF,
                                            D_PARSEGEN_NO_NODE,
                                            (int32_t)interned,
                                            D_PARSEGEN_NO_NODE);
}


/*
d_parsegen_grammar_internal_list
  Builds a node of a many-child kind over a list of children.

Parameter(s):
  _grammar:  the grammar to build into.
  _kind:     the node kind.
  _children: the child indices, in order.
  _count:    how many; must be at least 1.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
static int32_t
d_parsegen_grammar_internal_list(
    struct d_parsegen_grammar* _grammar,
    int                        _kind,
    const int32_t*             _children,
    uint32_t                   _count
)
{
    if ( (!_grammar)     ||
         (!_children)    ||
         (_count == 0u)  )
    {
        return D_PARSEGEN_NO_NODE;
    }

    // a list of one is that one; wrapping it would make the tree deeper and
    // the rendering noisier for no gain
    if (_count == 1u)
    {
        return d_parsegen_node_at(_grammar, _children[0])
               ? _children[0]
               : D_PARSEGEN_NO_NODE;
    }

    const int32_t node = d_parsegen_grammar_internal_node(_grammar,
                                                          _kind,
                                                          D_PARSEGEN_NO_NODE,
                                                          D_PARSEGEN_NO_NODE,
                                                          D_PARSEGEN_NO_NODE);

    if (node == D_PARSEGEN_NO_NODE)
    {
        return D_PARSEGEN_NO_NODE;
    }

    if (!d_parsegen_grammar_internal_link(_grammar, node, _children, _count))
    {
        return D_PARSEGEN_NO_NODE;
    }

    return node;
}


/*
d_parsegen_sequence
  The expression matching each of its children in order.

Parameter(s):
  _grammar:  the grammar to build into; may be NULL.
  _children: the child indices, in order.
  _count:    how many; 1 returns that child unwrapped.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_sequence(
    struct d_parsegen_grammar* _grammar,
    const int32_t*             _children,
    uint32_t                   _count
)
{
    return d_parsegen_grammar_internal_list(_grammar,
                                            (int)D_PARSEGEN_NODE_SEQUENCE,
                                            _children,
                                            _count);
}


/*
d_parsegen_ordered
  The expression matching the FIRST of its children that matches, committing to
it.
NOTE:
  PEG's `/`. Once an alternative matches, the ones after it are never tried,
even if the overall parse later fails -- which is what makes a PEG
unambiguous, and what makes this a different operator from the one below.

Parameter(s):
  _grammar:  the grammar to build into; may be NULL.
  _children: the alternatives, in priority order.
  _count:    how many; 1 returns that child unwrapped.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_ordered(
    struct d_parsegen_grammar* _grammar,
    const int32_t*             _children,
    uint32_t                   _count
)
{
    return d_parsegen_grammar_internal_list(
               _grammar,
               (int)D_PARSEGEN_NODE_ORDERED_CHOICE,
               _children,
               _count);
}


/*
d_parsegen_unordered
  The expression matching ANY of its children, with no priority between them.
NOTE:
  BNF's `|`. The alternatives are a set, so a grammar using this may be
ambiguous and the order it was written in carries no meaning. A pass may lower
this to an ordered choice only where the alternatives' first sets are disjoint,
which is a proof obligation and not a default.

Parameter(s):
  _grammar:  the grammar to build into; may be NULL.
  _children: the alternatives, in no meaningful order.
  _count:    how many; 1 returns that child unwrapped.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_unordered(
    struct d_parsegen_grammar* _grammar,
    const int32_t*             _children,
    uint32_t                   _count
)
{
    return d_parsegen_grammar_internal_list(
               _grammar,
               (int)D_PARSEGEN_NODE_UNORDERED_CHOICE,
               _children,
               _count);
}


/*
d_parsegen_repeat
  The expression matching its child between a minimum and a maximum number of
times.
NOTE:
  The three classic forms are bounds rather than kinds: `?` is {0,1}, `*` is
{0, unbounded}, `+` is {1, unbounded}. Anything else sets BOUNDED_REPEAT, since
a family that can only loop needs to know it has been handed a count.

Parameter(s):
  _grammar: the grammar to build into; may be NULL.
  _child:   the expression to repeat.
  _minimum: the fewest acceptable repetitions.
  _maximum: the most, or D_PARSEGEN_UNBOUNDED.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure, including when the
bounds are reversed.
*/
int32_t
d_parsegen_repeat(
    struct d_parsegen_grammar* _grammar,
    int32_t                    _child,
    uint32_t                   _minimum,
    uint32_t                   _maximum
)
{
    // reject a missing child and a range that can never be satisfied
    if ( (!d_parsegen_node_at(_grammar, _child)) ||
         (_minimum > _maximum)                   )
    {
        return D_PARSEGEN_NO_NODE;
    }

    const int32_t node =
        d_parsegen_grammar_internal_node(_grammar,
                                         (int)D_PARSEGEN_NODE_REPEAT,
                                         _child,
                                         (int32_t)_minimum,
                                         (int32_t)_maximum);

    // anything that is not one of the three classic forms is a counted
    // repetition, and a family has to be able to ask about that
    if ( (node != D_PARSEGEN_NO_NODE) &&
         (_minimum > 1u)              )
    {
        _grammar->features |= D_PARSEGEN_BOUNDED_REPEAT;
    }
    else if ( (node != D_PARSEGEN_NO_NODE)          &&
              (_maximum != D_PARSEGEN_UNBOUNDED)    &&
              (_maximum != 1u)                      )
    {
        _grammar->features |= D_PARSEGEN_BOUNDED_REPEAT;
    }

    return node;
}


/*
d_parsegen_capture
  Tags a subexpression so what it matched can be recovered.

Parameter(s):
  _grammar: the grammar to build into; may be NULL.
  _child:   the expression to tag.
  _tag:     a numeric tag the family will carry through to a capture record.
  _name:    an optional readable name; may be NULL.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_capture(
    struct d_parsegen_grammar* _grammar,
    int32_t                    _child,
    int32_t                    _tag,
    const char*                _name
)
{
    if (!d_parsegen_node_at(_grammar, _child))
    {
        return D_PARSEGEN_NO_NODE;
    }

    int32_t named = D_PARSEGEN_NO_NODE;

    // an optional name, so a diagnostic and a rendering can say which capture
    if (_name)
    {
        const uint32_t interned =
            d_parse_pool_intern_string(&_grammar->pool, _name);

        if (interned == D_PARSE_POOL_NONE)
        {
            return D_PARSEGEN_NO_NODE;
        }

        named = (int32_t)interned;
    }

    return d_parsegen_grammar_internal_node(_grammar,
                                            (int)D_PARSEGEN_NODE_CAPTURE,
                                            _child,
                                            _tag,
                                            named);
}


/*
d_parsegen_predicate
  Asserts that its child does or does not match here, consuming nothing either
way.

Parameter(s):
  _grammar:  the grammar to build into; may be NULL.
  _child:    the expression to test.
  _negated:  non-zero for `!`, zero for `&`.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_predicate(
    struct d_parsegen_grammar* _grammar,
    int32_t                    _child,
    int                        _negated
)
{
    if (!d_parsegen_node_at(_grammar, _child))
    {
        return D_PARSEGEN_NO_NODE;
    }

    const int kind = (_negated != 0)
                     ? (int)D_PARSEGEN_NODE_NOT_PREDICATE
                     : (int)D_PARSEGEN_NODE_AND_PREDICATE;

    return d_parsegen_grammar_internal_node(_grammar,
                                            kind,
                                            _child,
                                            D_PARSEGEN_NO_NODE,
                                            D_PARSEGEN_NO_NODE);
}


/*
d_parsegen_action
  Attaches host-language code to a subexpression.
NOTE:
  The code is stored verbatim and never interpreted here. What language it is
in, and whether any backend can emit it, is the backend's business -- this
level only records that the grammar has one, so a capability query can answer.

Parameter(s):
  _grammar: the grammar to build into; may be NULL.
  _child:   the expression the action belongs to.
  _code:    the host code; may be NULL.
Return:
  The node's index, or D_PARSEGEN_NO_NODE on failure.
*/
int32_t
d_parsegen_action(
    struct d_parsegen_grammar* _grammar,
    int32_t                    _child,
    const char*                _code
)
{
    if ( (!d_parsegen_node_at(_grammar, _child)) ||
         (!_code)                                )
    {
        return D_PARSEGEN_NO_NODE;
    }

    const uint32_t interned = d_parse_pool_intern_string(&_grammar->pool,
                                                         _code);

    if (interned == D_PARSE_POOL_NONE)
    {
        return D_PARSEGEN_NO_NODE;
    }

    return d_parsegen_grammar_internal_node(_grammar,
                                            (int)D_PARSEGEN_NODE_ACTION,
                                            _child,
                                            (int32_t)interned,
                                            D_PARSEGEN_NO_NODE);
}


/*
d_parsegen_rule_index
  The index of a rule by name.

Parameter(s):
  _grammar: the grammar to search; may be NULL.
  _name:    the name to match exactly; may be NULL.
Return:
  The rule's index, or -1 when there is no such rule.
*/
int32_t
d_parsegen_rule_index(
    const struct d_parsegen_grammar* _grammar,
    const char*                      _name
)
{
    if ( (!_grammar) ||
         (!_name)    )
    {
        return -1;
    }

    for (uint32_t index = 0u; index < _grammar->rule_count; index++)
    {
        if (strcmp(d_parsegen_rule_name(_grammar, index), _name) == 0)
        {
            return (int32_t)index;
        }
    }

    return -1;
}


/*
d_parsegen_rule_add
  Declares a named production.
NOTE:
  The first rule added is the start symbol unless the grammar says otherwise,
which is the convention every notation here shares.

Parameter(s):
  _grammar: the grammar to add to; may be NULL.
  _name:    the rule's name; may be NULL, which is rejected.
  _body:    the rule's expression.
  _flags:   D_PARSEGEN_RULE_* bits.
Return:
  0 on success; -1 on an invalid argument, a duplicate name, or when the rule
array could not grow.
*/
int
d_parsegen_rule_add(
    struct d_parsegen_grammar* _grammar,
    const char*                _name,
    int32_t                    _body,
    uint16_t                   _flags
)
{
    // reject a missing grammar, a nameless rule, and a body that is not a node
    if ( (!_grammar)                            ||
         (!_name)                               ||
         (_name[0] == '\0')                     ||
         (!d_parsegen_node_at(_grammar, _body)) )
    {
        return -1;
    }

    // a duplicate name would make every reference to it ambiguous
    if (d_parsegen_rule_index(_grammar, _name) >= 0)
    {
        return -1;
    }

    const uint32_t interned = d_parse_pool_intern_string(&_grammar->pool,
                                                         _name);

    if (interned == D_PARSE_POOL_NONE)
    {
        return -1;
    }

    // make room, or refuse
    if (_grammar->rule_count >= _grammar->rule_capacity)
    {
#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
        void* const grown =
            d_parse_grow(_grammar->rules,
                         &_grammar->rule_capacity,
                         _grammar->rule_count + 1u,
                         (uint32_t)sizeof(*_grammar->rules),
                         (_grammar->flags &
                          D_PARSEGEN_GRAMMAR_OWNS_RULES) != 0u);

        // check if memory allocation was successful
        if (!grown)
        {
            return -1;
        }

        _grammar->rules = (struct d_parsegen_rule*)grown;
#else
        return -1;
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP
    }

    const uint32_t index = _grammar->rule_count;

    _grammar->rules[index].name     = interned;
    _grammar->rules[index].body     = _body;
    _grammar->rules[index].flags    = _flags;
    _grammar->rules[index].reserved = 0u;
    _grammar->rules[index].offset   = _grammar->origin_offset;
    _grammar->rules[index].length   = _grammar->origin_length;

    _grammar->rule_count++;

    _grammar->flags = (uint8_t)(_grammar->flags &
                                ~(D_PARSEGEN_GRAMMAR_RESOLVED |
                                  D_PARSEGEN_GRAMMAR_VERIFIED));

    return 0;
}


/*
d_parsegen_grammar_resolve
  Binds every reference to the rule it names.
NOTE:
  Name binding only. LEFT_RECURSION is not computed here: the indirect case
needs nullability to get right, so it belongs to analysis, and a grammar's
capability set is therefore complete only after that runs.

Parameter(s):
  _grammar: the grammar to resolve; may be NULL.
  _diag:    the sink to report unknown references through; may be NULL.
Post-condition(s):
  - on success every reference node's second operand names a rule, and
    D_PARSEGEN_GRAMMAR_RESOLVED is set.
Return:
  0 if every reference bound; -1 otherwise, with every unbound reference
reported rather than only the first.
*/
int
d_parsegen_grammar_resolve(
    struct d_parsegen_grammar* _grammar,
    struct d_parse_diag_sink*  _diag
)
{
    if (!_grammar)
    {
        return -1;
    }

    int ok = 1;

    for (uint32_t index = 0u; index < _grammar->node_count; index++)
    {
        struct d_parsegen_node* const node = &_grammar->nodes[index];

        if (node->kind != (uint16_t)D_PARSEGEN_NODE_REF)
        {
            continue;
        }

        const char* const name = d_parse_pool_string(&_grammar->pool,
                                                     (uint32_t)node->a);
        const int32_t     rule = d_parsegen_rule_index(_grammar, name);

        // an unresolved reference is a grammar error, and naming it is most of
        // the value of reporting it
        if (rule < 0)
        {
            char message[D_INTERNAL_GRAMMAR_MESSAGE];

            (void)snprintf(message,
                           sizeof(message),
                           "rule '%s' is referenced but never declared",
                           name);

            (void)d_parse_diag_emit(
                _diag,
                (int)D_PARSE_SEVERITY_ERROR,
                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_GRAMMAR,
                (uint16_t)0,
                d_parse_span_make(node->offset, node->length),
                message);

            ok = 0;

            continue;
        }

        node->b = rule;
    }

    if (ok)
    {
        _grammar->flags |= (uint8_t)D_PARSEGEN_GRAMMAR_RESOLVED;
    }

    return ok ? 0 : -1;
}


/*
d_parsegen_grammar_verify
  Checks that the arena is well formed.
NOTE:
  Structure only: that every kind is known, every child and sibling index is in
range, arity matches the kind's descriptor, every rule has a body, and the start
rule exists. What the grammar MEANS is analysis's business.

Parameter(s):
  _grammar: the grammar to verify; may be NULL.
  _diag:    the sink to report through; may be NULL.
Return:
  0 if the grammar is well formed; -1 otherwise, with every problem reported.
*/
int
d_parsegen_grammar_verify(
    struct d_parsegen_grammar* _grammar,
    struct d_parse_diag_sink*  _diag
)
{
    if (!_grammar)
    {
        return -1;
    }

    int ok = 1;

    // a grammar with rules must have a start symbol among them
    if ( (_grammar->rule_count > 0u) &&
         (_grammar->start >= _grammar->rule_count) )
    {
        (void)d_parse_diag_emit(_diag,
                                (int)D_PARSE_SEVERITY_ERROR,
                                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_GRAMMAR,
                                (uint16_t)0,
                                d_parse_span_unknown(),
                                "the start rule is outside the grammar");

        ok = 0;
    }

    for (uint32_t index = 0u; index < _grammar->node_count; index++)
    {
        const struct d_parsegen_node* const node = &_grammar->nodes[index];

        const struct d_parsegen_node_info* const info =
            d_parsegen_kind_info((int)node->kind);

        if (!info)
        {
            char message[D_INTERNAL_GRAMMAR_MESSAGE];

            (void)snprintf(message,
                           sizeof(message),
                           "node %u has kind %u, which is not a known "
                           "expression",
                           (unsigned)index,
                           (unsigned)node->kind);

            (void)d_parse_diag_emit(
                _diag,
                (int)D_PARSE_SEVERITY_ERROR,
                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_GRAMMAR,
                (uint16_t)0,
                d_parse_span_make(node->offset, node->length),
                message);

            ok = 0;

            continue;
        }

        // count the children, checking each index as we go
        uint32_t children = 0u;
        int32_t  cursor   = node->child;

        while (cursor != D_PARSEGEN_NO_NODE)
        {
            if (!d_parsegen_node_at(_grammar, cursor))
            {
                (void)d_parse_diag_emit(
                    _diag,
                    (int)D_PARSE_SEVERITY_ERROR,
                    (uint16_t)D_PARSEGEN_DIAG_DOMAIN_GRAMMAR,
                    (uint16_t)0,
                    d_parse_span_make(node->offset, node->length),
                    "a child index points outside the grammar");

                ok = 0;

                break;
            }

            children++;
            cursor = _grammar->nodes[cursor].next;

            // a chain longer than the arena is a cycle, which would otherwise
            // hang every walker in the system
            if (children > _grammar->node_count)
            {
                (void)d_parse_diag_emit(
                    _diag,
                    (int)D_PARSE_SEVERITY_ERROR,
                    (uint16_t)D_PARSEGEN_DIAG_DOMAIN_GRAMMAR,
                    (uint16_t)0,
                    d_parse_span_make(node->offset, node->length),
                    "a sibling chain loops back on itself");

                ok = 0;

                break;
            }
        }

        // arity: a terminal takes none, a modifier takes one, a list takes at
        // least two, since a list of one is returned unwrapped
        const int arity_ok =
            (info->arity == (uint8_t)D_PARSEGEN_ARITY_NONE)
            ? (children == 0u)
            : ((info->arity == (uint8_t)D_PARSEGEN_ARITY_ONE)
               ? (children == 1u)
               : (children >= 2u));

        if (!arity_ok)
        {
            char message[D_INTERNAL_GRAMMAR_MESSAGE];

            (void)snprintf(message,
                           sizeof(message),
                           "a '%s' node has %u children, which its kind does "
                           "not allow",
                           info->name,
                           (unsigned)children);

            (void)d_parse_diag_emit(
                _diag,
                (int)D_PARSE_SEVERITY_ERROR,
                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_GRAMMAR,
                (uint16_t)0,
                d_parse_span_make(node->offset, node->length),
                message);

            ok = 0;
        }
    }

    // every rule must have a body that is a node
    for (uint32_t index = 0u; index < _grammar->rule_count; index++)
    {
        if (!d_parsegen_node_at(_grammar, _grammar->rules[index].body))
        {
            char message[D_INTERNAL_GRAMMAR_MESSAGE];

            (void)snprintf(message,
                           sizeof(message),
                           "rule '%s' has no body",
                           d_parsegen_rule_name(_grammar, index));

            (void)d_parse_diag_emit(
                _diag,
                (int)D_PARSE_SEVERITY_ERROR,
                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_GRAMMAR,
                (uint16_t)0,
                d_parse_span_make(_grammar->rules[index].offset,
                                  _grammar->rules[index].length),
                message);

            ok = 0;
        }
    }

    if (ok)
    {
        _grammar->flags |= (uint8_t)D_PARSEGEN_GRAMMAR_VERIFIED;
    }

    return ok ? 0 : -1;
}


/*
d_parsegen_grammar_hash
  A 64-bit digest of a whole grammar.
NOTE:
  Over the structure, the rules, and the pool -- not over the memory image, and
not over the source offsets. Two grammars that mean the same thing digest the
same even when they were read from differently formatted text, which is what
makes this usable as the key of a compiled-parser cache.

Parameter(s):
  _grammar: the grammar to digest; may be NULL.
Return:
  The digest.
*/
uint64_t
d_parsegen_grammar_hash(
    const struct d_parsegen_grammar* _grammar
)
{
    uint64_t digest = 1469598103934665603ULL;

    if (!_grammar)
    {
        return digest;
    }

    #define D_INTERNAL_GRAMMAR_FOLD(value)                                    \
        do                                                                    \
        {                                                                     \
            digest ^= (uint64_t)(value);                                      \
            digest *= 1099511628211ULL;                                       \
        } while (0)

    D_INTERNAL_GRAMMAR_FOLD(_grammar->features);
    D_INTERNAL_GRAMMAR_FOLD(_grammar->start);
    D_INTERNAL_GRAMMAR_FOLD(_grammar->node_count);
    D_INTERNAL_GRAMMAR_FOLD(_grammar->rule_count);

    // the shape of every node, but not where it came from: two spellings of
    // one grammar must agree
    for (uint32_t index = 0u; index < _grammar->node_count; index++)
    {
        D_INTERNAL_GRAMMAR_FOLD(_grammar->nodes[index].kind);
        D_INTERNAL_GRAMMAR_FOLD((uint32_t)_grammar->nodes[index].child);
        D_INTERNAL_GRAMMAR_FOLD((uint32_t)_grammar->nodes[index].next);
        D_INTERNAL_GRAMMAR_FOLD((uint32_t)_grammar->nodes[index].a);
        D_INTERNAL_GRAMMAR_FOLD((uint32_t)_grammar->nodes[index].b);
    }

    for (uint32_t index = 0u; index < _grammar->rule_count; index++)
    {
        D_INTERNAL_GRAMMAR_FOLD(_grammar->rules[index].name);
        D_INTERNAL_GRAMMAR_FOLD((uint32_t)_grammar->rules[index].body);
        D_INTERNAL_GRAMMAR_FOLD(_grammar->rules[index].flags);
    }

    #undef D_INTERNAL_GRAMMAR_FOLD

    digest ^= d_parse_pool_hash(&_grammar->pool);
    digest *= 1099511628211ULL;

    return digest;
}


// d_parsegen_internal_writer
//   struct: a bounded append cursor, so a renderer can keep reporting what it
// would have needed after the caller's buffer fills.
struct d_parsegen_internal_writer
{
    char*  cursor;
    size_t remaining;
    size_t needed;
};


/*
d_parsegen_internal_put
  Appends text to a bounded writer.

Parameter(s):
  _writer: the writer to append to.
  _text:   the text to append.
Return:
  none.
*/
static void
d_parsegen_internal_put(
    struct d_parsegen_internal_writer* _writer,
    const char*                        _text
)
{
    const int written = snprintf(_writer->cursor,
                                 _writer->remaining,
                                 "%s",
                                 _text);
    const size_t grew = (written < 0) ? 0u : (size_t)written;

    _writer->needed += grew;

    // keep counting once the buffer is full, so the return stays correct
    if (grew < _writer->remaining)
    {
        _writer->cursor    += grew;
        _writer->remaining -= grew;
    }
    else
    {
        _writer->cursor    = NULL;
        _writer->remaining = 0u;
    }

    return;
}


/*
d_parsegen_internal_render
  Writes one expression, parenthesising a child that binds more loosely than
its parent.

Parameter(s):
  _grammar: the grammar holding the node.
  _node:    the node to render.
  _outer:   the binding strength of the context.
  _writer:  the writer to append to.
Return:
  none.
*/
static void
d_parsegen_internal_render(
    const struct d_parsegen_grammar*   _grammar,
    int32_t                            _node,
    uint8_t                            _outer,
    struct d_parsegen_internal_writer* _writer
)
{
    const struct d_parsegen_node* const node =
        d_parsegen_node_at(_grammar, _node);

    if (!node)
    {
        d_parsegen_internal_put(_writer, "<?>");

        return;
    }

    const struct d_parsegen_node_info* const info =
        d_parsegen_kind_info((int)node->kind);

    if (!info)
    {
        d_parsegen_internal_put(_writer, "<?>");

        return;
    }

    const int wrap = (info->precedence < _outer) ? 1 : 0;

    if (wrap)
    {
        d_parsegen_internal_put(_writer, "( ");
    }

    switch ((enum d_parsegen_node_kind)node->kind)
    {
        case D_PARSEGEN_NODE_EMPTY:
            d_parsegen_internal_put(_writer, "()");
            break;

        case D_PARSEGEN_NODE_ANY:
            d_parsegen_internal_put(_writer, ".");
            break;

        case D_PARSEGEN_NODE_CLASS:
        {
            const struct d_parse_charset* const set =
                (const struct d_parse_charset*)
                d_parse_pool_data(&_grammar->pool, (uint32_t)node->a);

            char rendered[96];

            rendered[0] = '\0';

            if (set)
            {
                (void)d_parse_charset_render(set, rendered, sizeof(rendered));
            }

            d_parsegen_internal_put(_writer, rendered);
            break;
        }

        case D_PARSEGEN_NODE_LITERAL:
        {
            char quoted[128];

            (void)snprintf(quoted,
                           sizeof(quoted),
                           "'%s'",
                           d_parse_pool_string(&_grammar->pool,
                                               (uint32_t)node->a));

            d_parsegen_internal_put(_writer, quoted);
            break;
        }

        case D_PARSEGEN_NODE_REF:
            d_parsegen_internal_put(_writer,
                                    d_parse_pool_string(&_grammar->pool,
                                                        (uint32_t)node->a));
            break;

        case D_PARSEGEN_NODE_SEQUENCE:
        case D_PARSEGEN_NODE_ORDERED_CHOICE:
        case D_PARSEGEN_NODE_UNORDERED_CHOICE:
        {
            // `/` for ordered and `|` for unordered, so the distinction the
            // model refuses to blur is visible in the text as well
            const char* const separator =
                (node->kind == (uint16_t)D_PARSEGEN_NODE_SEQUENCE)
                ? " "
                : ((node->kind == (uint16_t)D_PARSEGEN_NODE_ORDERED_CHOICE)
                   ? " / "
                   : " | ");

            int32_t cursor = node->child;
            int     first  = 1;

            while (cursor != D_PARSEGEN_NO_NODE)
            {
                if (!first)
                {
                    d_parsegen_internal_put(_writer, separator);
                }

                first = 0;

                const struct d_parsegen_node* const child =
                    d_parsegen_node_at(_grammar, cursor);

                d_parsegen_internal_render(_grammar,
                                           cursor,
                                           info->precedence,
                                           _writer);

                cursor = child ? child->next : D_PARSEGEN_NO_NODE;
            }
            break;
        }

        case D_PARSEGEN_NODE_REPEAT:
        {
            d_parsegen_internal_render(_grammar,
                                       node->child,
                                       info->precedence,
                                       _writer);

            const uint32_t minimum = (uint32_t)node->a;
            const uint32_t maximum = (uint32_t)node->b;

            if ( (minimum == 0u) &&
                 (maximum == D_PARSEGEN_UNBOUNDED) )
            {
                d_parsegen_internal_put(_writer, "*");
            }
            else if ( (minimum == 1u) &&
                      (maximum == D_PARSEGEN_UNBOUNDED) )
            {
                d_parsegen_internal_put(_writer, "+");
            }
            else if ( (minimum == 0u) &&
                      (maximum == 1u) )
            {
                d_parsegen_internal_put(_writer, "?");
            }
            else
            {
                char bounds[32];

                if (maximum == D_PARSEGEN_UNBOUNDED)
                {
                    (void)snprintf(bounds,
                                   sizeof(bounds),
                                   "{%u,}",
                                   (unsigned)minimum);
                }
                else
                {
                    (void)snprintf(bounds,
                                   sizeof(bounds),
                                   "{%u,%u}",
                                   (unsigned)minimum,
                                   (unsigned)maximum);
                }

                d_parsegen_internal_put(_writer, bounds);
            }
            break;
        }

        case D_PARSEGEN_NODE_CAPTURE:
        {
            d_parsegen_internal_put(_writer, "<");

            if (node->b != D_PARSEGEN_NO_NODE)
            {
                d_parsegen_internal_put(_writer,
                                        d_parse_pool_string(&_grammar->pool,
                                                            (uint32_t)node->b));
                d_parsegen_internal_put(_writer, ": ");
            }

            d_parsegen_internal_render(_grammar, node->child, 0u, _writer);
            d_parsegen_internal_put(_writer, ">");
            break;
        }

        case D_PARSEGEN_NODE_AND_PREDICATE:
            d_parsegen_internal_put(_writer, "&");
            d_parsegen_internal_render(_grammar,
                                       node->child,
                                       info->precedence,
                                       _writer);
            break;

        case D_PARSEGEN_NODE_NOT_PREDICATE:
            d_parsegen_internal_put(_writer, "!");
            d_parsegen_internal_render(_grammar,
                                       node->child,
                                       info->precedence,
                                       _writer);
            break;

        case D_PARSEGEN_NODE_ACTION:
            d_parsegen_internal_render(_grammar,
                                       node->child,
                                       info->precedence,
                                       _writer);
            d_parsegen_internal_put(_writer, " %{");
            d_parsegen_internal_put(_writer,
                                    d_parse_pool_string(&_grammar->pool,
                                                        (uint32_t)node->a));
            d_parsegen_internal_put(_writer, "}%");
            break;

        default:
            d_parsegen_internal_put(_writer, "<?>");
            break;
    }

    if (wrap)
    {
        d_parsegen_internal_put(_writer, " )");
    }

    return;
}


/*
d_parsegen_expr_render
  Writes one expression in the canonical notation.

Parameter(s):
  _grammar: the grammar holding the node; may be NULL.
  _node:    the node to render.
  _out:     the buffer to write into; may be NULL when _size is 0.
  _size:    the size of _out in bytes, including the terminator.
Return:
  The number of characters the full rendering would occupy, excluding the
terminator.
*/
size_t
d_parsegen_expr_render(
    const struct d_parsegen_grammar* _grammar,
    int32_t                          _node,
    char*                            _out,
    size_t                           _size
)
{
    struct d_parsegen_internal_writer writer;

    writer.cursor    = _out;
    writer.remaining = _size;
    writer.needed    = 0u;

    // write a terminator even for an empty rendering, so the buffer is always
    // a valid string afterwards
    if ( (_out != NULL) &&
         (_size > 0u)   )
    {
        _out[0] = '\0';
    }

    d_parsegen_internal_render(_grammar, _node, 0u, &writer);

    return writer.needed;
}


/*
d_parsegen_grammar_render
  Writes a whole grammar in the canonical notation, one rule per line.
NOTE:
  What the grammar MEANS rather than the spelling it arrived in, so a round
trip through any frontend produces the same text -- which makes this the
natural oracle for a frontend test as well as the readable form of a grammar
that was built in code.

Parameter(s):
  _grammar: the grammar to render; may be NULL.
  _out:     the buffer to write into; may be NULL when _size is 0.
  _size:    the size of _out in bytes, including the terminator.
Return:
  The number of characters the full rendering would occupy, excluding the
terminator.
*/
size_t
d_parsegen_grammar_render(
    const struct d_parsegen_grammar* _grammar,
    char*                            _out,
    size_t                           _size
)
{
    struct d_parsegen_internal_writer writer;

    writer.cursor    = _out;
    writer.remaining = _size;
    writer.needed    = 0u;

    if ( (_out != NULL) &&
         (_size > 0u)   )
    {
        _out[0] = '\0';
    }

    if (!_grammar)
    {
        return 0u;
    }

    for (uint32_t index = 0u; index < _grammar->rule_count; index++)
    {
        d_parsegen_internal_put(&writer,
                                d_parsegen_rule_name(_grammar, index));
        d_parsegen_internal_put(&writer, " = ");

        d_parsegen_internal_render(_grammar,
                                   _grammar->rules[index].body,
                                   0u,
                                   &writer);

        d_parsegen_internal_put(&writer, " ;\n");
    }

    return writer.needed;
}
