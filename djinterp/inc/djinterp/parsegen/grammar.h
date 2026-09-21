/******************************************************************************
* djinterp [parsegen]                                               grammar.h
*
* The notation-neutral grammar: what every frontend produces and every family
* consumes.
*   This is the interchange format, so the cost of getting it wrong is paid by
* everything on both sides of it. Four decisions carry that weight.
*
*   IT IS A TREE, NOT TWO LISTS. A rule whose body is a list of alternatives
* each of which is a list of terms cannot express `a (b | c)* d`, and both EBNF
* and ABNF have grouping. So a rule body is an expression, and an expression
* has children.
*
*   IT IS AN ARENA, NOT A POINTER GRAPH. Nodes live in one flat array and refer
* to each other by index, exactly as instructions refer to pool entries. That
* is what keeps a grammar copyable, hashable as a cache key, serialisable, and
* handed to C without a fix-up -- the same properties the instruction stream
* has, for the same reasons.
*
*   ORDERED AND UNORDERED CHOICE ARE DIFFERENT NODE KINDS. Not one kind with a
* flag, and emphatically not one kind with a default. PEG's `/` commits to the
* first alternative that matches and BNF's `|` does not, so collapsing them
* would silently change which language a grammar denotes -- and the damage
* would surface as a wrong parse rather than as an error. Having no neutral
* `choice` node means a frontend cannot build one without saying which it meant.
*
*   REPETITION CARRIES BOUNDS. `{min, max}`, not a star flag. `?` is {0,1}, `*`
* is {0, unbounded}, `+` is {1, unbounded}, and ABNF's `3*5x` is {3,5} -- one
* representation for all of them, so adding ABNF costs a frontend and no change
* here or below.
*
*   FEATURES ACCUMULATE AS YOU BUILD. Every construction updates the grammar's
* capability set from the kind descriptor table, so a frontend cannot forget to
* declare what it emitted. A grammar that contains an unordered choice reports
* UNORDERED_CHOICE whether or not its frontend meant to.
*   The exception, deliberately: LEFT_RECURSION is a derived property, not a
* syntactic one, and is set by analysis rather than by building.
*
*   Requires: parsegen/feature.h, parse/charset.h, parse/pool.h,
*             parse/diagnostic.h.
*
* path:      /inc/djinterp/parsegen/grammar.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE EXPRESSION
    --------------
    1.  Constants
         1.  D_PARSEGEN_NO_NODE
         2.  D_PARSEGEN_UNBOUNDED
    2.  Node kinds
         1.  d_parsegen_node_kind
    3.  The node
         1.  d_parsegen_node
    4.  Kind descriptors
         1.  d_parsegen_arity
         2.  d_parsegen_node_info
2.  THE GRAMMAR
    -----------
    1.  The rule
         1.  d_parsegen_rule
         2.  Rule flags
    2.  The container
         1.  d_parsegen_grammar
         2.  Grammar flags
3.  OPERATIONS
    ----------
    1.  Lifetime
    2.  Expression construction
    3.  Rule construction
    4.  Access
    5.  Resolution and verification
    6.  Identity
    7.  Rendering
*/

#ifndef DJINTERP_PARSEGEN_GRAMMAR_
#define DJINTERP_PARSEGEN_GRAMMAR_ 1

// std
#include <stddef.h>                     // size_t, NULL
#include <stdint.h>                     // int32_t, uint16_t, uint32_t
// djinterp
#include "../parse/charset.h"           // d_parse_charset, the class terminal
#include "../parse/diagnostic.h"        // the channel resolution and
                                        // verification report through
#include "../parse/pool.h"              // d_parse_pool, names and literals
#include "./feature.h"                  // d_parsegen_features, and the
                                        // subsystem umbrella


//==============================================================================
// 1.  THE EXPRESSION
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_PARSEGEN_NO_NODE
//   constant: the node index meaning "none" -- an absent child, an absent
// sibling, an unresolved reference, or a construction that failed.
#define D_PARSEGEN_NO_NODE          (-1)

// 1.1.2
// D_PARSEGEN_UNBOUNDED
//   constant: the repetition maximum meaning "no limit". `*` is {0, this} and
// `+` is {1, this}, so the three classic forms and an explicit range are one
// representation rather than four.
#define D_PARSEGEN_UNBOUNDED        0xFFFFFFFFu


// 1.2    Node kinds
//------------------------------------------------------------------------------
// 1.2.1
// d_parsegen_node_kind
//   enum: what an expression node is. Kinds that differ in MEANING are
// separate kinds rather than one kind with a modifier flag -- see the banner
// on ordered versus unordered choice, which is the case that matters.
enum d_parsegen_node_kind
{
    D_PARSEGEN_NODE_EMPTY            = 0,
    D_PARSEGEN_NODE_ANY              = 1,
    D_PARSEGEN_NODE_CLASS            = 2,
    D_PARSEGEN_NODE_LITERAL          = 3,
    D_PARSEGEN_NODE_REF              = 4,
    D_PARSEGEN_NODE_SEQUENCE         = 5,
    D_PARSEGEN_NODE_ORDERED_CHOICE   = 6,
    D_PARSEGEN_NODE_UNORDERED_CHOICE = 7,
    D_PARSEGEN_NODE_REPEAT           = 8,
    D_PARSEGEN_NODE_CAPTURE          = 9,
    D_PARSEGEN_NODE_AND_PREDICATE    = 10,
    D_PARSEGEN_NODE_NOT_PREDICATE    = 11,
    D_PARSEGEN_NODE_ACTION           = 12,
    D_PARSEGEN_NODE_KIND_COUNT       = 13,
    D_PARSEGEN_NODE_KIND_USER        = 64
};


// 1.3    The node
//------------------------------------------------------------------------------
// 1.3.1
// d_parsegen_node
//   struct: one expression node. Children hang off `child` and chain through
// `next`, so a sequence or a choice of any width costs one index per member and
// a builder can nest without knowing a width in advance.
//   `a` and `b` mean what the node's kind descriptor says they mean: a pool
// index for a class, literal, or reference; the bounds for a repetition; the
// tag for a capture. `offset` and `length` say where in the source text the
// expression came from, so a diagnostic can point at it -- byte positions only,
// with line and column derived when something is actually rendered.
struct d_parsegen_node
{
    uint16_t kind;
    uint16_t flags;
    int32_t  child;
    int32_t  next;
    int32_t  a;
    int32_t  b;
    uint32_t offset;
    uint32_t length;
};


// 1.4    Kind descriptors
//------------------------------------------------------------------------------
// 1.4.1
// d_parsegen_arity
//   enum: how many children a kind takes, which is what verification checks
// and what rendering needs to know before it walks.
enum d_parsegen_arity
{
    D_PARSEGEN_ARITY_NONE = 0,
    D_PARSEGEN_ARITY_ONE  = 1,
    D_PARSEGEN_ARITY_MANY = 2
};

// 1.4.2
// d_parsegen_node_info
//   struct: everything this level knows about a node kind -- its name, its
// arity, what its operands mean, how tightly it binds when rendered, and which
// capabilities using it implies.
//   One table drives feature accumulation, verification, and rendering. Adding
// a node kind is adding a row, which is the same bargain the operator registry
// makes one level down.
struct d_parsegen_node_info
{
    const char*         name;
    uint8_t             arity;
    uint8_t             precedence;
    uint8_t             reserved[2];
    d_parsegen_features implies;
};


//==============================================================================
// 2.  THE GRAMMAR
//==============================================================================


// 2.1    The rule
//------------------------------------------------------------------------------
// 2.1.1
// d_parsegen_rule
//   struct: one named production. The body is a node index rather than a list
// of alternatives, because a body is an expression and a choice is one shape an
// expression can take.
struct d_parsegen_rule
{
    uint32_t name;
    int32_t  body;
    uint16_t flags;
    uint16_t reserved;
    uint32_t offset;
    uint32_t length;
};

// 2.1.2
// D_PARSEGEN_RULE_EXPORTED
//   constant: this rule is part of the grammar's public surface and must
// survive a pass that would otherwise inline or drop it.
#define D_PARSEGEN_RULE_EXPORTED    0x0001u
// D_PARSEGEN_RULE_INLINE
//   constant: a hint that this rule exists for readability and may be expanded
// at its use sites. A hint, never an obligation.
#define D_PARSEGEN_RULE_INLINE      0x0002u


// 2.2    The container
//------------------------------------------------------------------------------
// 2.2.1
// d_parsegen_grammar
//   struct: the nodes, the rules, the pool their names and terminals live in,
// and the capability set the whole thing uses.
//   `origin` is the byte range the CURRENT construction is attributed to. A
// frontend sets it once per token and every node built afterwards inherits it,
// which is the shape a frontend already has -- rather than threading a span
// through every construction call.
struct d_parsegen_grammar
{
    struct d_parsegen_node* nodes;
    uint32_t                node_count;
    uint32_t                node_capacity;
    struct d_parsegen_rule* rules;
    uint32_t                rule_count;
    uint32_t                rule_capacity;
    struct d_parse_pool     pool;
    d_parsegen_features     features;
    uint32_t                start;
    uint32_t                notation;
    uint32_t                origin_offset;
    uint32_t                origin_length;
    uint8_t                 flags;
    uint8_t                 reserved[3];
};

// 2.2.2
// D_PARSEGEN_GRAMMAR_OWNS_NODES
//   constant: the node array was allocated by the grammar.
#define D_PARSEGEN_GRAMMAR_OWNS_NODES   0x01u
// D_PARSEGEN_GRAMMAR_OWNS_RULES
//   constant: the rule array was allocated by the grammar.
#define D_PARSEGEN_GRAMMAR_OWNS_RULES   0x02u
// D_PARSEGEN_GRAMMAR_RESOLVED
//   constant: every reference has been bound to a rule since the last edit.
// Cleared by any construction.
#define D_PARSEGEN_GRAMMAR_RESOLVED     0x04u
// D_PARSEGEN_GRAMMAR_VERIFIED
//   constant: structure has been checked since the last edit.
#define D_PARSEGEN_GRAMMAR_VERIFIED     0x08u


//==============================================================================
// 3.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 3.1    Lifetime
//------------------------------------------------------------------------------
void            d_parsegen_grammar_init(struct d_parsegen_grammar* _grammar);
#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
D_NODISCARD int d_parsegen_grammar_init_heap(
                    struct d_parsegen_grammar* _grammar,
                    uint32_t                   _nodes,
                    uint32_t                   _rules);
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP
void            d_parsegen_grammar_reset(struct d_parsegen_grammar* _grammar);
void            d_parsegen_grammar_release(struct d_parsegen_grammar* _grammar);
void            d_parsegen_grammar_at(struct d_parsegen_grammar* _grammar,
                                      uint32_t                   _offset,
                                      uint32_t                   _length);
void            d_parsegen_grammar_notation(
                    struct d_parsegen_grammar* _grammar,
                    const char*                _name);

// 3.2    Expression construction
//------------------------------------------------------------------------------
//   Every builder returns a node index, or D_PARSEGEN_NO_NODE on failure, and
// accumulates the capabilities its kind implies. Note that there is no neutral
// `choice`: a frontend must say which it means, which is the one place this
// level refuses to be convenient.
int32_t         d_parsegen_empty(struct d_parsegen_grammar* _grammar);
int32_t         d_parsegen_any(struct d_parsegen_grammar* _grammar);
int32_t         d_parsegen_class(struct d_parsegen_grammar*    _grammar,
                                 const struct d_parse_charset* _set);
int32_t         d_parsegen_literal(struct d_parsegen_grammar* _grammar,
                                   const char*                _text);
int32_t         d_parsegen_ref(struct d_parsegen_grammar* _grammar,
                               const char*                _name);
int32_t         d_parsegen_sequence(struct d_parsegen_grammar* _grammar,
                                    const int32_t*             _children,
                                    uint32_t                   _count);
int32_t         d_parsegen_ordered(struct d_parsegen_grammar* _grammar,
                                   const int32_t*             _children,
                                   uint32_t                   _count);
int32_t         d_parsegen_unordered(struct d_parsegen_grammar* _grammar,
                                     const int32_t*             _children,
                                     uint32_t                   _count);
int32_t         d_parsegen_repeat(struct d_parsegen_grammar* _grammar,
                                  int32_t                    _child,
                                  uint32_t                   _minimum,
                                  uint32_t                   _maximum);
int32_t         d_parsegen_capture(struct d_parsegen_grammar* _grammar,
                                   int32_t                    _child,
                                   int32_t                    _tag,
                                   const char*                _name);
int32_t         d_parsegen_predicate(struct d_parsegen_grammar* _grammar,
                                     int32_t                    _child,
                                     int                        _negated);
int32_t         d_parsegen_action(struct d_parsegen_grammar* _grammar,
                                  int32_t                    _child,
                                  const char*                _code);

// 3.3    Rule construction
//------------------------------------------------------------------------------
D_NODISCARD int d_parsegen_rule_add(struct d_parsegen_grammar* _grammar,
                                    const char*                _name,
                                    int32_t                    _body,
                                    uint16_t                   _flags);
int32_t         d_parsegen_rule_index(
                    const struct d_parsegen_grammar* _grammar,
                    const char*                      _name);

// 3.4    Access
//------------------------------------------------------------------------------
const struct d_parsegen_node_info* d_parsegen_kind_info(int _kind);

/*
d_parsegen_node_at
  The node at an index.

Parameter(s):
  _grammar: the grammar to read; may be NULL.
  _index:   the node index a builder returned.
Return:
  A pointer to the node, or NULL when the index names none.
*/
D_INLINE const struct d_parsegen_node*
d_parsegen_node_at(
    const struct d_parsegen_grammar* _grammar,
    int32_t                          _index
)
{
    // reject a missing grammar, absent storage, and an index outside the arena
    if ( (!_grammar)                                    ||
         (!_grammar->nodes)                             ||
         (_index < 0)                                   ||
         ((uint32_t)_index >= _grammar->node_count)     )
    {
        return NULL;
    }

    return &_grammar->nodes[_index];
}

/*
d_parsegen_rule_at
  The rule at an index.

Parameter(s):
  _grammar: the grammar to read; may be NULL.
  _index:   the rule index, in declaration order.
Return:
  A pointer to the rule, or NULL when the index names none.
*/
D_INLINE const struct d_parsegen_rule*
d_parsegen_rule_at(
    const struct d_parsegen_grammar* _grammar,
    uint32_t                         _index
)
{
    if ( (!_grammar)                            ||
         (!_grammar->rules)                     ||
         (_index >= _grammar->rule_count)       )
    {
        return NULL;
    }

    return &_grammar->rules[_index];
}

/*
d_parsegen_rule_name
  The name of a rule.

Parameter(s):
  _grammar: the grammar to read; may be NULL.
  _index:   the rule index.
Return:
  The name, or "" when the index names no rule. Never NULL.
*/
D_INLINE const char*
d_parsegen_rule_name(
    const struct d_parsegen_grammar* _grammar,
    uint32_t                         _index
)
{
    const struct d_parsegen_rule* const rule =
        d_parsegen_rule_at(_grammar, _index);

    if (!rule)
    {
        return "";
    }

    return d_parse_pool_string(&_grammar->pool, rule->name);
}

// 3.5    Resolution and verification
//------------------------------------------------------------------------------
//   resolve binds every reference to the rule it names; verify checks that the
// arena is well formed. Both report every problem they find rather than the
// first, because a frontend bug usually shows up more than once.
//   Neither computes LEFT_RECURSION: that is structural rather than syntactic,
// needs nullability to get right for the indirect case, and belongs to
// analysis.
D_NODISCARD int d_parsegen_grammar_resolve(
                    struct d_parsegen_grammar* _grammar,
                    struct d_parse_diag_sink*  _diag);
D_NODISCARD int d_parsegen_grammar_verify(
                    struct d_parsegen_grammar* _grammar,
                    struct d_parse_diag_sink*  _diag);

// 3.6    Identity
//------------------------------------------------------------------------------
uint64_t        d_parsegen_grammar_hash(
                    const struct d_parsegen_grammar* _grammar);

// 3.7    Rendering
//------------------------------------------------------------------------------
//   Canonical notation: `/` for ordered choice and `|` for unordered, so the
// distinction the model refuses to blur is visible in the text too. Renders
// what the grammar MEANS, not the spelling it arrived in, so a round trip
// through any frontend produces the same output.
size_t          d_parsegen_expr_render(
                    const struct d_parsegen_grammar* _grammar,
                    int32_t                          _node,
                    char*                            _out,
                    size_t                           _size);
size_t          d_parsegen_grammar_render(
                    const struct d_parsegen_grammar* _grammar,
                    char*                            _out,
                    size_t                           _size);

D_EXTERN_C_END


#endif  // DJINTERP_PARSEGEN_GRAMMAR_
