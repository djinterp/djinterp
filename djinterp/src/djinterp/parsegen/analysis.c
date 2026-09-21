/******************************************************************************
* djinterp [parsegen]                                              analysis.c
*
*   Definitions for the non-inline declarations in analysis.h.
*
*
* path:      /src/djinterp/parsegen/analysis.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../inc/djinterp/parsegen/analysis.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memset
#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
#include <stdlib.h>  // malloc, free
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP


// D_INTERNAL_ANALYSIS_MESSAGE
//   macro: the size of the buffer a diagnostic is composed in.
#define D_INTERNAL_ANALYSIS_MESSAGE 192u

// D_INTERNAL_ANALYSIS_SLACK
//   macro: rounds allowed beyond the longest possible dependency chain before
// the fixed point is declared not to have settled. The iteration is monotone
// over a finite lattice and therefore always terminates; this bound exists to
// turn a mistake in a transfer function into a diagnostic rather than a hang.
#define D_INTERNAL_ANALYSIS_SLACK   8u


/*
d_parsegen_facts_init
  Initialises facts over caller-supplied storage.
NOTE:
  The scratch arrays are not optional when the grammar has rules: left
recursion is found by searching the left-call relation, and that search needs
somewhere to keep its worklist.

Parameter(s):
  _facts:         the facts to initialise; ignored if NULL.
  _nodes:         storage for per-node facts; may be NULL.
  _node_capacity: how many nodes _nodes holds.
  _rules:         storage for per-rule facts; may be NULL.
  _rule_capacity: how many rules _rules holds.
  _work:          scratch, 2 entries per rule; may be NULL.
  _mark:          scratch, 1 entry per rule; may be NULL.
  _stack:         scratch, 1 entry per node; may be NULL.
Return:
  none.
*/
void
d_parsegen_facts_init(
    struct d_parsegen_facts*      _facts,
    struct d_parsegen_node_facts* _nodes,
    uint32_t                      _node_capacity,
    struct d_parsegen_rule_facts* _rules,
    uint32_t                      _rule_capacity,
    uint32_t*                     _work,
    uint8_t*                      _mark,
    int32_t*                      _stack
)
{
    if (!_facts)
    {
        return;
    }

    memset(_facts, 0, sizeof(*_facts));

    _facts->nodes      = _nodes;
    _facts->node_count = (_nodes != NULL) ? _node_capacity : 0u;
    _facts->rules      = _rules;
    _facts->rule_count = (_rules != NULL) ? _rule_capacity : 0u;
    _facts->work       = _work;
    _facts->mark       = _mark;
    _facts->stack      = _stack;

    return;
}


#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)

/*
d_parsegen_facts_init_heap
  Initialises facts sized for one grammar, over storage it allocates and owns.

Parameter(s):
  _facts:   the facts to initialise; ignored if NULL.
  _grammar: the grammar the facts will describe; ignored if NULL.
Return:
  0 on success; -1 on an invalid argument or a refused allocation.
*/
int
d_parsegen_facts_init_heap(
    struct d_parsegen_facts*         _facts,
    const struct d_parsegen_grammar* _grammar
)
{
    if ( (!_facts)   ||
         (!_grammar) )
    {
        return -1;
    }

    d_parsegen_facts_init(_facts, NULL, 0u, NULL, 0u, NULL, NULL, NULL);

    const uint32_t nodes = (_grammar->node_count > 0u)
                           ? _grammar->node_count
                           : 1u;
    const uint32_t rules = (_grammar->rule_count > 0u)
                           ? _grammar->rule_count
                           : 1u;

    struct d_parsegen_node_facts* node_store =
        (struct d_parsegen_node_facts*)malloc((size_t)nodes *
                                              sizeof(*node_store));

    // check if the per-node allocation was successful
    if (!node_store)
    {
        return -1;
    }

    struct d_parsegen_rule_facts* rule_store =
        (struct d_parsegen_rule_facts*)malloc((size_t)rules *
                                              sizeof(*rule_store));

    // check if the per-rule allocation was successful
    if (!rule_store)
    {
        free(node_store);

        return -1;
    }

    uint32_t* work = (uint32_t*)malloc((size_t)rules * 2u * sizeof(*work));

    // check if the worklist allocation was successful
    if (!work)
    {
        free(rule_store);
        free(node_store);

        return -1;
    }

    uint8_t* mark = (uint8_t*)malloc((size_t)rules * sizeof(*mark));

    // check if the visited-set allocation was successful
    if (!mark)
    {
        free(work);
        free(rule_store);
        free(node_store);

        return -1;
    }

    int32_t* stack = (int32_t*)malloc((size_t)nodes * sizeof(*stack));

    // check if the traversal-stack allocation was successful
    if (!stack)
    {
        free(mark);
        free(work);
        free(rule_store);
        free(node_store);

        return -1;
    }

    _facts->nodes      = node_store;
    _facts->node_count = nodes;
    _facts->rules      = rule_store;
    _facts->rule_count = rules;
    _facts->work       = work;
    _facts->mark       = mark;
    _facts->stack      = stack;
    _facts->flags      = (uint8_t)D_PARSEGEN_FACTS_OWNS_STORAGE;

    return 0;
}

#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP


/*
d_parsegen_facts_release
  Releases any storage the facts own and leaves them empty.

Parameter(s):
  _facts: the facts to release; ignored if NULL.
Return:
  none.
*/
void
d_parsegen_facts_release(
    struct d_parsegen_facts* _facts
)
{
    if (!_facts)
    {
        return;
    }

#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
    // free only what these facts allocated; caller storage is never touched
    if ((_facts->flags & D_PARSEGEN_FACTS_OWNS_STORAGE) != 0u)
    {
        free(_facts->nodes);
        free(_facts->rules);
        free(_facts->work);
        free(_facts->mark);
        free(_facts->stack);
    }
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP

    memset(_facts, 0, sizeof(*_facts));

    return;
}


/*
d_parsegen_analysis_internal_node
  One round of the fixed point over a single node.
NOTE:
  The transfer functions, in one place. Three of them deserve their reasoning
recorded, since each is a place a careless reading gets it wrong:
  - A PREDICATE consumes nothing, so it is nullable, and it contributes NOTHING
    to a first set. Giving `&A` the first set of A would be sound but looser;
    giving it the empty set lets the sequence rule hand back the tighter and
    still-sound answer.
  - A SEQUENCE keeps absorbing first sets while its members are nullable, which
    is the only reason nullability has to be computed alongside rather than
    after.
  - A REPEAT is nullable when its minimum is zero, whatever its child says.

Parameter(s):
  _grammar: the grammar being analysed.
  _facts:   the facts being filled.
  _index:   the node to update.
Return:
  A boolean value corresponding to either:
  - 1, if anything about this node changed, or
  - 0, otherwise.
*/
static int
d_parsegen_analysis_internal_node(
    const struct d_parsegen_grammar* _grammar,
    struct d_parsegen_facts*         _facts,
    uint32_t                         _index
)
{
    const struct d_parsegen_node* const node = &_grammar->nodes[_index];
    struct d_parsegen_node_facts* const  out = &_facts->nodes[_index];

    const uint16_t         was_flags = out->flags;
    struct d_parse_charset was_first = out->first;

    uint16_t               flags = 0u;
    struct d_parse_charset first;

    d_parse_charset_clear(&first);

    switch ((enum d_parsegen_node_kind)node->kind)
    {
        case D_PARSEGEN_NODE_EMPTY:
            flags = (uint16_t)(D_PARSEGEN_FACT_NULLABLE |
                               D_PARSEGEN_FACT_PRODUCTIVE);
            break;

        case D_PARSEGEN_NODE_ANY:
            d_parse_charset_fill(&first);
            flags = (uint16_t)D_PARSEGEN_FACT_PRODUCTIVE;
            break;

        case D_PARSEGEN_NODE_CLASS:
        {
            const struct d_parse_charset* const set =
                (const struct d_parse_charset*)
                d_parse_pool_data(&_grammar->pool, (uint32_t)node->a);

            if (set)
            {
                first = *set;
            }

            // an empty class matches nothing ever, so it is not productive
            flags = (uint16_t)(d_parse_charset_count(&first) > 0u
                               ? D_PARSEGEN_FACT_PRODUCTIVE
                               : 0u);
            break;
        }

        case D_PARSEGEN_NODE_LITERAL:
        {
            const char* const text =
                d_parse_pool_string(&_grammar->pool, (uint32_t)node->a);

            // the empty literal matches nothing and consumes nothing
            if (text[0] == '\0')
            {
                flags = (uint16_t)(D_PARSEGEN_FACT_NULLABLE |
                                   D_PARSEGEN_FACT_PRODUCTIVE);
            }
            else
            {
                d_parse_charset_add(&first, (unsigned char)text[0]);
                flags = (uint16_t)D_PARSEGEN_FACT_PRODUCTIVE;
            }
            break;
        }

        case D_PARSEGEN_NODE_REF:
        {
            const struct d_parsegen_rule_facts* const target =
                d_parsegen_rule_facts_at(_facts, node->b);

            // an unresolved reference contributes nothing, which keeps the
            // fixed point sound on a grammar that failed to resolve
            if (target)
            {
                first = target->first;
                flags = (uint16_t)(target->flags &
                                   (D_PARSEGEN_FACT_NULLABLE |
                                    D_PARSEGEN_FACT_PRODUCTIVE));
            }
            break;
        }

        case D_PARSEGEN_NODE_SEQUENCE:
        {
            int     nullable   = 1;
            int     productive = 1;
            int32_t cursor     = node->child;

            // absorb first sets while the members so far can all match empty;
            // the first non-nullable member closes the set
            while (cursor != D_PARSEGEN_NO_NODE)
            {
                const struct d_parsegen_node_facts* const child =
                    d_parsegen_node_facts_at(_facts, cursor);

                if (!child)
                {
                    break;
                }

                if (nullable)
                {
                    d_parse_charset_unite(&first, &child->first);
                }

                if ((child->flags & D_PARSEGEN_FACT_NULLABLE) == 0u)
                {
                    nullable = 0;
                }

                if ((child->flags & D_PARSEGEN_FACT_PRODUCTIVE) == 0u)
                {
                    productive = 0;
                }

                cursor = _grammar->nodes[cursor].next;
            }

            flags = (uint16_t)((nullable ? D_PARSEGEN_FACT_NULLABLE : 0u) |
                               (productive ? D_PARSEGEN_FACT_PRODUCTIVE : 0u));
            break;
        }

        case D_PARSEGEN_NODE_ORDERED_CHOICE:
        case D_PARSEGEN_NODE_UNORDERED_CHOICE:
        {
            int     nullable   = 0;
            int     productive = 0;
            int32_t cursor     = node->child;

            // a choice can begin with anything any alternative can begin with,
            // and succeeds if any alternative does
            while (cursor != D_PARSEGEN_NO_NODE)
            {
                const struct d_parsegen_node_facts* const child =
                    d_parsegen_node_facts_at(_facts, cursor);

                if (!child)
                {
                    break;
                }

                d_parse_charset_unite(&first, &child->first);

                if ((child->flags & D_PARSEGEN_FACT_NULLABLE) != 0u)
                {
                    nullable = 1;
                }

                if ((child->flags & D_PARSEGEN_FACT_PRODUCTIVE) != 0u)
                {
                    productive = 1;
                }

                cursor = _grammar->nodes[cursor].next;
            }

            flags = (uint16_t)((nullable ? D_PARSEGEN_FACT_NULLABLE : 0u) |
                               (productive ? D_PARSEGEN_FACT_PRODUCTIVE : 0u));
            break;
        }

        case D_PARSEGEN_NODE_REPEAT:
        {
            const struct d_parsegen_node_facts* const child =
                d_parsegen_node_facts_at(_facts, node->child);

            if (child)
            {
                first = child->first;
            }

            // zero repetitions is a match, whatever the child can do
            const int nullable = ((uint32_t)node->a == 0u) ||
                                 ( (child != NULL) &&
                                   ((child->flags &
                                     D_PARSEGEN_FACT_NULLABLE) != 0u) );
            const int productive = ((uint32_t)node->a == 0u) ||
                                   ( (child != NULL) &&
                                     ((child->flags &
                                       D_PARSEGEN_FACT_PRODUCTIVE) != 0u) );

            flags = (uint16_t)((nullable ? D_PARSEGEN_FACT_NULLABLE : 0u) |
                               (productive ? D_PARSEGEN_FACT_PRODUCTIVE : 0u));
            break;
        }

        case D_PARSEGEN_NODE_CAPTURE:
        case D_PARSEGEN_NODE_ACTION:
        {
            const struct d_parsegen_node_facts* const child =
                d_parsegen_node_facts_at(_facts, node->child);

            // transparent: a tag and a host action change what happens on a
            // match, not what can match
            if (child)
            {
                first = child->first;
                flags = (uint16_t)(child->flags &
                                   (D_PARSEGEN_FACT_NULLABLE |
                                    D_PARSEGEN_FACT_PRODUCTIVE));
            }
            break;
        }

        case D_PARSEGEN_NODE_AND_PREDICATE:
        case D_PARSEGEN_NODE_NOT_PREDICATE:
            // consumes nothing, so it is nullable and contributes no symbol;
            // the sequence rule then hands back the tighter sound answer
            flags = (uint16_t)(D_PARSEGEN_FACT_NULLABLE |
                               D_PARSEGEN_FACT_PRODUCTIVE);
            break;

        default:
            break;
    }

    out->flags = flags;
    out->first = first;

    return ( (flags != was_flags) ||
             (!d_parse_charset_equal(&first, &was_first)) )
           ? 1
           : 0;
}


/*
d_parsegen_analysis_internal_left
  Whether a rule can reach itself without consuming anything first.
NOTE:
  A breadth-first walk of the left-call relation, seeded from the rule's own
leftmost positions. Iterative rather than recursive on purpose: the grammar is
runtime input, and a deeply nested one must not be able to exhaust the stack.

Parameter(s):
  _grammar: the grammar being analysed.
  _facts:   the facts, whose scratch arrays this uses.
  _rule:    the rule to test.
  _witness: receives a rule the recursion passes through, or -1; optional.
Return:
  A boolean value corresponding to either:
  - 1, if the rule is left-recursive, or
  - 0, otherwise.
*/
static int
d_parsegen_analysis_internal_left(
    const struct d_parsegen_grammar* _grammar,
    struct d_parsegen_facts*         _facts,
    uint32_t                         _rule,
    int32_t*                         _witness
)
{
    const uint32_t rules = _grammar->rule_count;

    uint32_t* const queue = _facts->work;
    uint8_t*  const seen  = _facts->mark;
    int32_t*  const stack = _facts->stack;

    memset(seen, 0, (size_t)rules);

    uint32_t head = 0u;
    uint32_t tail = 0u;

    // the node stack walks leftmost positions within one rule body; the rule
    // queue walks between rules
    uint32_t depth = 0u;

    if (_witness)
    {
        *_witness = -1;
    }

    // seed from this rule's own body, then from each rule reached
    uint32_t current = _rule;
    int      first_round = 1;

    for (;;)
    {
        depth = 0u;
        stack[depth++] = _grammar->rules[current].body;

        while (depth > 0u)
        {
            const int32_t index = stack[--depth];

            const struct d_parsegen_node* const node =
                d_parsegen_node_at(_grammar, index);

            if (!node)
            {
                continue;
            }

            switch ((enum d_parsegen_node_kind)node->kind)
            {
                case D_PARSEGEN_NODE_REF:
                {
                    const int32_t target = node->b;

                    if ( (target < 0) ||
                         ((uint32_t)target >= rules) )
                    {
                        break;
                    }

                    // reaching the rule we started from closes a cycle
                    if ((uint32_t)target == _rule)
                    {
                        if (_witness)
                        {
                            *_witness = first_round
                                        ? (int32_t)_rule
                                        : (int32_t)current;
                        }

                        return 1;
                    }

                    if (!seen[target])
                    {
                        seen[target]  = 1u;
                        queue[tail++] = (uint32_t)target;
                    }
                    break;
                }

                case D_PARSEGEN_NODE_SEQUENCE:
                {
                    // leftmost positions: the first member, and each following
                    // member only while everything before it can match empty
                    int32_t cursor = node->child;

                    while ( (cursor != D_PARSEGEN_NO_NODE) &&
                            (depth < _grammar->node_count)  )
                    {
                        stack[depth++] = cursor;

                        if (!d_parsegen_nullable(_facts, cursor))
                        {
                            break;
                        }

                        cursor = _grammar->nodes[cursor].next;
                    }
                    break;
                }

                case D_PARSEGEN_NODE_ORDERED_CHOICE:
                case D_PARSEGEN_NODE_UNORDERED_CHOICE:
                {
                    // every alternative begins at this position
                    int32_t cursor = node->child;

                    while ( (cursor != D_PARSEGEN_NO_NODE) &&
                            (depth < _grammar->node_count)  )
                    {
                        stack[depth++] = cursor;

                        cursor = _grammar->nodes[cursor].next;
                    }
                    break;
                }

                case D_PARSEGEN_NODE_REPEAT:
                case D_PARSEGEN_NODE_CAPTURE:
                case D_PARSEGEN_NODE_ACTION:
                case D_PARSEGEN_NODE_AND_PREDICATE:
                case D_PARSEGEN_NODE_NOT_PREDICATE:
                    // a predicate examines its child at this very position, so
                    // it propagates left-calls even though it consumes nothing
                    if (depth < _grammar->node_count)
                    {
                        stack[depth++] = node->child;
                    }
                    break;

                default:
                    break;
            }
        }

        if (head >= tail)
        {
            break;
        }

        current     = queue[head++];
        first_round = 0;
    }

    return 0;
}


/*
d_parsegen_analysis_internal_reach
  Marks every rule some chain of references leads to from the start symbol.

Parameter(s):
  _grammar: the grammar being analysed.
  _facts:   the facts to mark.
Return:
  none.
*/
static void
d_parsegen_analysis_internal_reach(
    const struct d_parsegen_grammar* _grammar,
    struct d_parsegen_facts*         _facts
)
{
    if (_grammar->rule_count == 0u)
    {
        return;
    }

    uint32_t* const queue = _facts->work;

    uint32_t head = 0u;
    uint32_t tail = 0u;

    const uint32_t start = (_grammar->start < _grammar->rule_count)
                           ? _grammar->start
                           : 0u;

    _facts->rules[start].flags |= (uint16_t)D_PARSEGEN_FACT_REACHABLE;
    queue[tail++] = start;

    // breadth-first between rules, walking each reached rule's body for the
    // references it contains
    while (head < tail)
    {
        const uint32_t rule = queue[head++];

        int32_t* const stack = _facts->stack;
        uint32_t       depth = 0u;

        stack[depth++] = _grammar->rules[rule].body;

        while (depth > 0u)
        {
            const int32_t index = stack[--depth];

            const struct d_parsegen_node* const node =
                d_parsegen_node_at(_grammar, index);

            if (!node)
            {
                continue;
            }

            if (node->kind == (uint16_t)D_PARSEGEN_NODE_REF)
            {
                const int32_t target = node->b;

                if ( (target >= 0)                                  &&
                     ((uint32_t)target < _grammar->rule_count)      &&
                     ((_facts->rules[target].flags &
                       D_PARSEGEN_FACT_REACHABLE) == 0u)            )
                {
                    _facts->rules[target].flags |=
                        (uint16_t)D_PARSEGEN_FACT_REACHABLE;

                    queue[tail++] = (uint32_t)target;
                }

                continue;
            }

            int32_t cursor = node->child;

            while ( (cursor != D_PARSEGEN_NO_NODE) &&
                    (depth < _grammar->node_count)  )
            {
                stack[depth++] = cursor;

                cursor = _grammar->nodes[cursor].next;
            }
        }
    }

    return;
}


/*
d_parsegen_analyze
  Derives what a grammar means and folds it into what the grammar reports.
NOTE:
  Left recursion is reported as a NOTE, never an error. Whether it is a problem
depends on which family will run the grammar, and that is a registry query --
so analysis sets LEFT_RECURSION and lets selection decide. A rule that can
never match, by contrast, makes the grammar unusable under any family, and is
an error.

Parameter(s):
  _grammar: the grammar to analyse; its capability set is updated. May be NULL.
  _facts:   the facts to fill; must be sized for the grammar. May be NULL.
  _diag:    the sink to report through; may be NULL.
Pre-condition(s):
  - the grammar has been resolved, or references contribute nothing and the
    first sets come back smaller than the truth.
Post-condition(s):
  - on success D_PARSEGEN_FACTS_COMPLETE is set and the grammar's feature set
    includes everything analysis derived.
Return:
  0 if the grammar is analysable and usable; -1 on a sizing failure, an
unanalysed fixed point, or a rule that can never match.
*/
int
d_parsegen_analyze(
    struct d_parsegen_grammar* _grammar,
    struct d_parsegen_facts*   _facts,
    struct d_parse_diag_sink*  _diag
)
{
    if ( (!_grammar) ||
         (!_facts)   )
    {
        return -1;
    }

    // facts must be at least as large as the grammar, or an index would mean
    // different things on the two sides
    if ( (_facts->node_count < _grammar->node_count) ||
         (_facts->rule_count < _grammar->rule_count) ||
         ( (_grammar->rule_count > 0u)                   &&
           ((!_facts->work) || (!_facts->mark)) )        ||
         ( (_grammar->node_count > 0u) && (!_facts->stack) ) )
    {
        (void)d_parse_diag_emit(_diag,
                                (int)D_PARSE_SEVERITY_ERROR,
                                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_ANALYSIS,
                                (uint16_t)0,
                                d_parse_span_unknown(),
                                "the facts are too small for this grammar");

        return -1;
    }

    // a grammar that was never resolved yields first sets smaller than the
    // truth, which is the unsound direction, so say so rather than proceed
    // quietly
    if ((_grammar->flags & D_PARSEGEN_GRAMMAR_RESOLVED) == 0u)
    {
        (void)d_parse_diag_emit(_diag,
                                (int)D_PARSE_SEVERITY_WARNING,
                                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_ANALYSIS,
                                (uint16_t)0,
                                d_parse_span_unknown(),
                                "analysing a grammar whose references are "
                                "not bound; first sets will be incomplete");
    }

    memset(_facts->nodes,
           0,
           (size_t)_grammar->node_count * sizeof(*_facts->nodes));
    memset(_facts->rules,
           0,
           (size_t)_grammar->rule_count * sizeof(*_facts->rules));

    for (uint32_t index = 0u; index < _grammar->rule_count; index++)
    {
        _facts->rules[index].left_witness = -1;
    }

    _facts->implied = D_PARSEGEN_FEATURE_NONE;
    _facts->rounds  = 0u;
    _facts->flags   = (uint8_t)(_facts->flags & ~D_PARSEGEN_FACTS_COMPLETE);

    const uint32_t limit = _grammar->node_count +
                           _grammar->rule_count +
                           (uint32_t)D_INTERNAL_ANALYSIS_SLACK;

    int settled = 0;

    // the fixed point: nodes and rules feed each other through references, so
    // both are relaxed in the same loop until nothing moves
    for (uint32_t round = 0u; round < limit; round++)
    {
        int changed = 0;

        for (uint32_t index = 0u; index < _grammar->node_count; index++)
        {
            changed |= d_parsegen_analysis_internal_node(_grammar,
                                                         _facts,
                                                         index);
        }

        for (uint32_t index = 0u; index < _grammar->rule_count; index++)
        {
            const struct d_parsegen_node_facts* const body =
                d_parsegen_node_facts_at(_facts,
                                         _grammar->rules[index].body);

            if (!body)
            {
                continue;
            }

            struct d_parsegen_rule_facts* const rule = &_facts->rules[index];

            const uint16_t carried = (uint16_t)(body->flags &
                                                (D_PARSEGEN_FACT_NULLABLE |
                                                 D_PARSEGEN_FACT_PRODUCTIVE));

            if ( ((rule->flags & (D_PARSEGEN_FACT_NULLABLE |
                                  D_PARSEGEN_FACT_PRODUCTIVE)) != carried) ||
                 (!d_parse_charset_equal(&rule->first, &body->first))       )
            {
                rule->flags = (uint16_t)((rule->flags &
                                          ~(D_PARSEGEN_FACT_NULLABLE |
                                            D_PARSEGEN_FACT_PRODUCTIVE)) |
                                         carried);
                rule->first = body->first;
                changed     = 1;
            }
        }

        _facts->rounds = round + 1u;

        if (!changed)
        {
            settled = 1;

            break;
        }
    }

    // the iteration is monotone and must settle; not settling means a transfer
    // function is wrong, which is worth saying loudly
    if (!settled)
    {
        (void)d_parse_diag_emit(_diag,
                                (int)D_PARSE_SEVERITY_ERROR,
                                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_ANALYSIS,
                                (uint16_t)0,
                                d_parse_span_unknown(),
                                "analysis did not reach a fixed point");

        return -1;
    }

    d_parsegen_analysis_internal_reach(_grammar, _facts);

    int ok = 1;

    for (uint32_t index = 0u; index < _grammar->rule_count; index++)
    {
        struct d_parsegen_rule_facts* const rule = &_facts->rules[index];

        int32_t witness = -1;

        // left recursion: a FACT, reported so a reader knows, and folded into
        // the grammar's capabilities so selection can route on it
        if (d_parsegen_analysis_internal_left(_grammar,
                                              _facts,
                                              index,
                                              &witness))
        {
            rule->flags        |= (uint16_t)D_PARSEGEN_FACT_LEFT_RECURSIVE;
            rule->left_witness  = witness;

            if (witness == (int32_t)index)
            {
                rule->flags |= (uint16_t)D_PARSEGEN_FACT_DIRECT_LEFT;
            }

            _facts->implied |= D_PARSEGEN_LEFT_RECURSION;

            char message[D_INTERNAL_ANALYSIS_MESSAGE];

            // two spellings rather than one with conditional fragments, so
            // neither can end up missing a quote
            if (witness == (int32_t)index)
            {
                (void)snprintf(message,
                               sizeof(message),
                               "rule '%s' is directly left-recursive",
                               d_parsegen_rule_name(_grammar, index));
            }
            else
            {
                (void)snprintf(message,
                               sizeof(message),
                               "rule '%s' is left-recursive through '%s'",
                               d_parsegen_rule_name(_grammar, index),
                               d_parsegen_rule_name(_grammar,
                                                    (uint32_t)witness));
            }

            (void)d_parse_diag_emit(
                _diag,
                (int)D_PARSE_SEVERITY_NOTE,
                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_ANALYSIS,
                (uint16_t)0,
                d_parse_span_make(_grammar->rules[index].offset,
                                  _grammar->rules[index].length),
                message);
        }

        // a rule that can never match makes the grammar unusable under any
        // family, which is a different thing entirely
        if ((rule->flags & D_PARSEGEN_FACT_PRODUCTIVE) == 0u)
        {
            char message[D_INTERNAL_ANALYSIS_MESSAGE];

            (void)snprintf(message,
                           sizeof(message),
                           "rule '%s' can never match anything",
                           d_parsegen_rule_name(_grammar, index));

            (void)d_parse_diag_emit(
                _diag,
                (int)D_PARSE_SEVERITY_ERROR,
                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_ANALYSIS,
                (uint16_t)0,
                d_parse_span_make(_grammar->rules[index].offset,
                                  _grammar->rules[index].length),
                message);

            ok = 0;
        }

        // an unreachable rule is dead weight, not a failure
        if ((rule->flags & D_PARSEGEN_FACT_REACHABLE) == 0u)
        {
            char message[D_INTERNAL_ANALYSIS_MESSAGE];

            (void)snprintf(message,
                           sizeof(message),
                           "rule '%s' is never reached from the start symbol",
                           d_parsegen_rule_name(_grammar, index));

            (void)d_parse_diag_emit(
                _diag,
                (int)D_PARSE_SEVERITY_WARNING,
                (uint16_t)D_PARSEGEN_DIAG_DOMAIN_ANALYSIS,
                (uint16_t)0,
                d_parse_span_make(_grammar->rules[index].offset,
                                  _grammar->rules[index].length),
                message);
        }
    }

    // what analysis derived becomes part of what the grammar reports, so a
    // family is selected against the whole truth rather than the syntax alone
    _grammar->features |= _facts->implied;

    if (ok)
    {
        _facts->flags |= (uint8_t)D_PARSEGEN_FACTS_COMPLETE;
    }

    return ok ? 0 : -1;
}


/*
d_parsegen_choice_disjoint
  Whether the alternatives of a choice can be told apart by one symbol.
NOTE:
  This is the precondition for two separate transforms: lowering an unordered
choice to an ordered one without changing the language, and compiling either
into a jump table instead of a cascade of attempts. Both need the same two
things, and the second is the one that gets forgotten -- a NULLABLE alternative
matches whatever comes next, so no amount of first-set inspection can dispatch
past it.

Parameter(s):
  _grammar:  the grammar holding the node; may be NULL.
  _facts:    its analysed facts; may be NULL.
  _choice:   the choice node to test.
  _conflict: receives an alternative that overlaps an earlier one, or is
             nullable; optional.
Return:
  A boolean value corresponding to either:
  - 1, if every alternative begins with a distinct symbol set and none is
    nullable, or
  - 0, otherwise, including when the node is not a choice.
*/
int
d_parsegen_choice_disjoint(
    const struct d_parsegen_grammar* _grammar,
    const struct d_parsegen_facts*   _facts,
    int32_t                          _choice,
    int32_t*                         _conflict
)
{
    if (_conflict)
    {
        *_conflict = D_PARSEGEN_NO_NODE;
    }

    const struct d_parsegen_node* const node =
        d_parsegen_node_at(_grammar, _choice);

    if ( (!node)     ||
         (!_facts)   ||
         ( (node->kind !=
            (uint16_t)D_PARSEGEN_NODE_ORDERED_CHOICE) &&
           (node->kind !=
            (uint16_t)D_PARSEGEN_NODE_UNORDERED_CHOICE) ) )
    {
        return 0;
    }

    struct d_parse_charset seen;

    d_parse_charset_clear(&seen);

    int32_t cursor = node->child;

    while (cursor != D_PARSEGEN_NO_NODE)
    {
        const struct d_parsegen_node_facts* const child =
            d_parsegen_node_facts_at(_facts, cursor);

        if (!child)
        {
            return 0;
        }

        // a nullable alternative always matches, so nothing can be dispatched
        // past it whatever its first set says
        if ((child->flags & D_PARSEGEN_FACT_NULLABLE) != 0u)
        {
            if (_conflict)
            {
                *_conflict = cursor;
            }

            return 0;
        }

        if (!d_parse_charset_disjoint(&seen, &child->first))
        {
            if (_conflict)
            {
                *_conflict = cursor;
            }

            return 0;
        }

        d_parse_charset_unite(&seen, &child->first);

        cursor = _grammar->nodes[cursor].next;
    }

    return 1;
}


/*
d_parsegen_choice_head_set
  The symbols any alternative of a choice can begin with.
NOTE:
  What a head-fail test compares against: if the next symbol is outside this
set and the choice is not nullable, the whole choice can be skipped without
attempting any alternative.

Parameter(s):
  _grammar: the grammar holding the node; may be NULL.
  _facts:   its analysed facts; may be NULL.
  _choice:  the node to summarise.
  _out:     receives the union of the alternatives' first sets.
Return:
  0 on success; -1 on an invalid argument.
*/
int
d_parsegen_choice_head_set(
    const struct d_parsegen_grammar* _grammar,
    const struct d_parsegen_facts*   _facts,
    int32_t                          _choice,
    struct d_parse_charset*          _out
)
{
    if (!_out)
    {
        return -1;
    }

    d_parse_charset_clear(_out);

    const struct d_parsegen_node_facts* const node =
        d_parsegen_node_facts_at(_facts, _choice);

    if ( (!_grammar) ||
         (!node)     )
    {
        return -1;
    }

    *_out = node->first;

    return 0;
}
