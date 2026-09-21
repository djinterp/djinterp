/******************************************************************************
* djinterp [parsegen]                                              analysis.h
*
* What a grammar means, as opposed to what it says.
*   A frontend records syntax: this rule has these alternatives, that one
* repeats. Analysis derives the properties nobody wrote down -- whether a rule
* can match nothing, which symbols can begin it, whether it can ever succeed,
* whether anything reaches it, and whether it recurses on its own left edge.
* Everything downstream reads facts, not the grammar: a family decides whether
* it can run this grammar, a pass decides whether a transform is legal, a code
* generator decides how to compile a choice.
*
*   LEFT RECURSION IS A FACT, NOT AN ERROR. This is the piece that makes the
* whole arrangement work. Analysis discovers left recursion and sets
* LEFT_RECURSION on the grammar; it does not complain about it. Whether that
* matters is the FAMILY's business, answered by a registry query -- the PEG
* family rejects the capability and an LR family wants it. A grammar that would
* have been a hard error under one parser type is simply routed to another,
* and neither analysis nor the frontend had to know that either existed.
*
*   FIRST SETS ARE SOUND OVER-APPROXIMATIONS. Every set here is a superset of
* what can truly begin a match, never a subset. That direction is what makes
* them safe to optimise against: skipping an alternative whose first set
* excludes the next symbol can never skip an alternative that would have
* matched. A caller must still check nullability before skipping, since a
* nullable alternative matches regardless of what comes next -- which is why
* both facts travel together.
*
*   ALPHABET. First sets are 256-bit, because this grammar's terminals are
* classes and literals over bytes. A token-stream family will need a wider
* domain and a second facts representation; that boundary is named here rather
* than papered over.
*
*   Requires: parsegen/grammar.h, parse/charset.h, parse/diagnostic.h.
*
* path:      /inc/djinterp/parsegen/analysis.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FACTS
    -----
    1.  Properties
         1.  Fact flags
              a. D_PARSEGEN_FACT_NULLABLE
              b. D_PARSEGEN_FACT_PRODUCTIVE
              c. D_PARSEGEN_FACT_REACHABLE
              d. D_PARSEGEN_FACT_LEFT_RECURSIVE
              e. D_PARSEGEN_FACT_DIRECT_LEFT
    2.  Per-node facts
         1.  d_parsegen_node_facts
    3.  Per-rule facts
         1.  d_parsegen_rule_facts
    4.  The collection
         1.  d_parsegen_facts
2.  OPERATIONS
    ----------
    1.  Lifetime
    2.  Analysis
    3.  Access
    4.  Derived questions
*/

#ifndef DJINTERP_PARSEGEN_ANALYSIS_
#define DJINTERP_PARSEGEN_ANALYSIS_ 1

// std
#include <stdint.h>                     // int32_t, uint8_t, uint16_t, uint32_t
// djinterp
#include "../parse/charset.h"           // d_parse_charset, the first-set domain
#include "../parse/diagnostic.h"        // the channel analysis reports through
#include "./grammar.h"                  // the grammar analysis reads


//==============================================================================
// 1.  FACTS
//==============================================================================


// 1.1    Properties
//------------------------------------------------------------------------------
// 1.1.1
// D_PARSEGEN_FACT_NULLABLE
//   constant: this expression or rule can match the empty input. A caller
// optimising a choice must check this before skipping an alternative on its
// first set, since a nullable alternative matches whatever comes next.
#define D_PARSEGEN_FACT_NULLABLE        0x0001u
// D_PARSEGEN_FACT_PRODUCTIVE
//   constant: this expression or rule can match SOMETHING -- that is, some
// finite input exists that it accepts. A rule that is not productive can never
// succeed, which makes the grammar unusable rather than merely odd.
#define D_PARSEGEN_FACT_PRODUCTIVE      0x0002u
// D_PARSEGEN_FACT_REACHABLE
//   constant: rules only. Some chain of references leads here from the start
// symbol. An unreachable rule is dead weight a pass may drop.
#define D_PARSEGEN_FACT_REACHABLE       0x0004u
// D_PARSEGEN_FACT_LEFT_RECURSIVE
//   constant: rules only. This rule can reach itself without consuming
// anything first, so a top-down parser would recurse forever. Reported, never
// complained about -- see the banner.
#define D_PARSEGEN_FACT_LEFT_RECURSIVE  0x0008u
// D_PARSEGEN_FACT_DIRECT_LEFT
//   constant: rules only. The left recursion is immediate rather than through
// other rules, which is the case a simple transform can remove.
#define D_PARSEGEN_FACT_DIRECT_LEFT     0x0010u


// 1.2    Per-node facts
//------------------------------------------------------------------------------
// 1.2.1
// d_parsegen_node_facts
//   struct: what was derived about one expression. Per NODE rather than only
// per rule, because the question a code generator asks is about an alternative
// inside a choice, not about the rule containing it.
struct d_parsegen_node_facts
{
    struct d_parse_charset first;
    uint16_t               flags;
    uint16_t               reserved;
};


// 1.3    Per-rule facts
//------------------------------------------------------------------------------
// 1.3.1
// d_parsegen_rule_facts
//   struct: what was derived about one rule. `left_witness` names a rule the
// left recursion passes through, so a diagnostic can say more than that a
// cycle exists -- it is the rule itself when the recursion is direct, and -1
// when there is none.
struct d_parsegen_rule_facts
{
    struct d_parse_charset first;
    uint16_t               flags;
    uint16_t               reserved;
    int32_t                left_witness;
};


// 1.4    The collection
//------------------------------------------------------------------------------
// 1.4.1
// d_parsegen_facts
//   struct: the derived properties of one grammar, indexed the same way the
// grammar is -- so a node index means the same thing in both.
//   `work`, `mark` and `stack` are analysis scratch. They are in the public
// struct rather than allocated per call because a build with no allocator must
// still be able to analyse a grammar, and the only way to do that in C is to
// let the caller supply the space.
//   `stack` is sized by NODE count rather than by nesting depth, because a
// choice with a hundred alternatives pushes a hundred entries and a fixed
// depth would silently stop short -- which would under-report left recursion,
// the one direction that is not safe to be wrong in.
struct d_parsegen_facts
{
    struct d_parsegen_node_facts* nodes;
    uint32_t                      node_count;
    struct d_parsegen_rule_facts* rules;
    uint32_t                      rule_count;
    uint32_t*                     work;
    uint8_t*                      mark;
    int32_t*                      stack;
    d_parsegen_features           implied;
    uint32_t                      rounds;
    uint8_t                       flags;
    uint8_t                       reserved[3];
};

// D_PARSEGEN_FACTS_OWNS_STORAGE
//   constant: the arrays were allocated by the facts and are freed by
// d_parsegen_facts_release.
#define D_PARSEGEN_FACTS_OWNS_STORAGE   0x01u
// D_PARSEGEN_FACTS_COMPLETE
//   constant: analysis ran to a fixed point over a grammar of this size.
#define D_PARSEGEN_FACTS_COMPLETE       0x02u


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 2.1    Lifetime
//------------------------------------------------------------------------------
void            d_parsegen_facts_init(
                    struct d_parsegen_facts*      _facts,
                    struct d_parsegen_node_facts* _nodes,
                    uint32_t                      _node_capacity,
                    struct d_parsegen_rule_facts* _rules,
                    uint32_t                      _rule_capacity,
                    uint32_t*                     _work,
                    uint8_t*                      _mark,
                    int32_t*                      _stack);
#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
D_NODISCARD int d_parsegen_facts_init_heap(
                    struct d_parsegen_facts*         _facts,
                    const struct d_parsegen_grammar* _grammar);
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP
void            d_parsegen_facts_release(struct d_parsegen_facts* _facts);

// 2.2    Analysis
//------------------------------------------------------------------------------
//   Reads the grammar, fills the facts, and folds what it DERIVED into the
// grammar's capability set -- which is why the grammar is not const. A grammar
// that has not been analysed under-reports what it uses, and a family selected
// against that under-report would be the wrong family.
D_NODISCARD int d_parsegen_analyze(struct d_parsegen_grammar* _grammar,
                                   struct d_parsegen_facts*   _facts,
                                   struct d_parse_diag_sink*  _diag);

// 2.3    Access
//------------------------------------------------------------------------------
/*
d_parsegen_node_facts_at
  What was derived about one expression.

Parameter(s):
  _facts: the facts to read; may be NULL.
  _node:  the node index.
Return:
  A pointer to the node's facts, or NULL when the index names none.
*/
D_INLINE const struct d_parsegen_node_facts*
d_parsegen_node_facts_at(
    const struct d_parsegen_facts* _facts,
    int32_t                        _node
)
{
    if ( (!_facts)                                  ||
         (!_facts->nodes)                           ||
         (_node < 0)                                ||
         ((uint32_t)_node >= _facts->node_count)    )
    {
        return NULL;
    }

    return &_facts->nodes[_node];
}

/*
d_parsegen_rule_facts_at
  What was derived about one rule.

Parameter(s):
  _facts: the facts to read; may be NULL.
  _rule:  the rule index.
Return:
  A pointer to the rule's facts, or NULL when the index names none.
*/
D_INLINE const struct d_parsegen_rule_facts*
d_parsegen_rule_facts_at(
    const struct d_parsegen_facts* _facts,
    int32_t                        _rule
)
{
    if ( (!_facts)                                  ||
         (!_facts->rules)                           ||
         (_rule < 0)                                ||
         ((uint32_t)_rule >= _facts->rule_count)    )
    {
        return NULL;
    }

    return &_facts->rules[_rule];
}

/*
d_parsegen_nullable
  Whether an expression can match the empty input.

Parameter(s):
  _facts: the facts to read; may be NULL.
  _node:  the node index.
Return:
  A boolean value corresponding to either:
  - 1, if the expression can match nothing at all, or
  - 0, if it cannot, or the index names no node.
*/
D_INLINE int
d_parsegen_nullable(
    const struct d_parsegen_facts* _facts,
    int32_t                        _node
)
{
    const struct d_parsegen_node_facts* const node =
        d_parsegen_node_facts_at(_facts, _node);

    if (!node)
    {
        return 0;
    }

    return ((node->flags & D_PARSEGEN_FACT_NULLABLE) != 0u) ? 1 : 0;
}

/*
d_parsegen_first
  The symbols that can begin a match of an expression.
NOTE:
  A sound over-approximation: never smaller than the truth, so a test against
it can rule an alternative OUT but must not be read as ruling one in.

Parameter(s):
  _facts: the facts to read; may be NULL.
  _node:  the node index.
Return:
  A pointer to the set, or NULL when the index names no node.
*/
D_INLINE const struct d_parse_charset*
d_parsegen_first(
    const struct d_parsegen_facts* _facts,
    int32_t                        _node
)
{
    const struct d_parsegen_node_facts* const node =
        d_parsegen_node_facts_at(_facts, _node);

    if (!node)
    {
        return NULL;
    }

    return &node->first;
}

/*
d_parsegen_left_recursive
  Whether a rule can reach itself without consuming anything first.

Parameter(s):
  _facts: the facts to read; may be NULL.
  _rule:  the rule index.
Return:
  A boolean value corresponding to either:
  - 1, if the rule is left-recursive, or
  - 0, otherwise.
*/
D_INLINE int
d_parsegen_left_recursive(
    const struct d_parsegen_facts* _facts,
    int32_t                        _rule
)
{
    const struct d_parsegen_rule_facts* const rule =
        d_parsegen_rule_facts_at(_facts, _rule);

    if (!rule)
    {
        return 0;
    }

    return ((rule->flags & D_PARSEGEN_FACT_LEFT_RECURSIVE) != 0u) ? 1 : 0;
}

// 2.4    Derived questions
//------------------------------------------------------------------------------
//   The questions a pass or a code generator actually asks. Naming them here
// rather than leaving each caller to re-derive them from first sets is what
// keeps one subtle rule -- that a nullable alternative can never be dispatched
// past -- in one place instead of in every optimiser that forgets it.
D_NODISCARD int d_parsegen_choice_disjoint(
                    const struct d_parsegen_grammar* _grammar,
                    const struct d_parsegen_facts*   _facts,
                    int32_t                          _choice,
                    int32_t*                         _conflict);
D_NODISCARD int d_parsegen_choice_head_set(
                    const struct d_parsegen_grammar* _grammar,
                    const struct d_parsegen_facts*   _facts,
                    int32_t                          _choice,
                    struct d_parse_charset*          _out);

D_EXTERN_C_END


#endif  // DJINTERP_PARSEGEN_ANALYSIS_
