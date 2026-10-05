/*******************************************************************************
* djinterp [tools]                                                    dcascade.c
*
* Definitions for the non-inline declarations in dcascade.h.
*   Matches are appended as (node, rule, specificity) triples, then counted
* into per-node runs -- a counting sort, stable, so a run keeps the order the
* host recorded -- and each run is insertion-sorted by the policy.  Runs are
* a handful long, where an insertion sort is the fast one.  A host that
* records rules in ascending order under FIRST with no layers therefore
* pays one pass and no swaps.
*
*
* path:      /src/djinterp/tools/dawk/dcascade.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dcascade.h"  // corresponding header
// std
#include <stdlib.h>  // malloc, realloc, calloc, free


// d_cascade
//   struct: recorded matches, then the per-node ranked runs.
struct d_cascade
{
    const struct d_dss_sheet*  sheet;
    uint8_t                    policy;       // d_cascade_policy

    uint32_t*                  match_node;   // the recorded triples
    uint32_t*                  match_rule;
    uint32_t*                  match_spec;
    size_t                     match_count;
    size_t                     match_capacity;

    uint32_t*                  offsets;      // node_count + 1, after ranking
    uint32_t*                  ranked;       // rules, per node best first
    uint32_t*                  ranked_spec;  // their specificities
    size_t                     node_count;
    size_t                     node_capacity;
};


/**
 * @brief Reports whether match `_a` outranks match `_b` under the policy.
 *
 * @param[in] _cascade  the cascade.
 * @param[in] _rule_a   the first rule.
 * @param[in] _spec_a   its specificity.
 * @param[in] _rule_b   the second rule.
 * @param[in] _spec_b   its specificity.
 * @return `true` if the first is the better-ranked.
 */
static bool
d_internal_outranks(
    const struct d_cascade* _cascade,
    uint32_t                _rule_a,
    uint32_t                _spec_a,
    uint32_t                _rule_b,
    uint32_t                _spec_b
)
{
    if (_cascade->policy != D_CASCADE_CSS)
    {
        return d_dss_outranks(_cascade->sheet, _rule_a, _rule_b);
    }

    const uint32_t layer_a = d_dss_rule_at(_cascade->sheet, _rule_a)->layer;
    const uint32_t layer_b = d_dss_rule_at(_cascade->sheet, _rule_b)->layer;

    // a stronger layer, then more specific, then later in the sheet
    if (layer_a != layer_b)
    {
        return (layer_a > layer_b);
    }

    if (_spec_a != _spec_b)
    {
        return (_spec_a > _spec_b);
    }

    return (_rule_a > _rule_b);
}


/**
 * @brief Finds a property's deciding declaration within one rule.
 *
 * @param[in] _cascade   the cascade.
 * @param[in] _rule      the rule.
 * @param[in] _property  the property's symbol.
 * @return the declaration's index -- the first under FIRST and ALL, the last
 *         under CSS -- or `D_DSS_NO_INDEX` when the rule declares none.
 */
static uint32_t
d_internal_declaration_in(
    const struct d_cascade* _cascade,
    uint32_t                _rule,
    uint32_t                _property
)
{
    const struct d_dss_rule* const rule = d_dss_rule_at(_cascade->sheet,
                                                        _rule);
    uint32_t found = D_DSS_NO_INDEX;

    for (uint32_t at = 0u; (rule) && (at < rule->declaration_count); ++at)
    {
        const uint32_t which = rule->first_declaration + at;

        // CSS: a later declaration in the same block overrides an earlier
        if (d_dss_declaration_at(_cascade->sheet, which)->symbol == _property)
        {
            found = which;

            if (_cascade->policy != D_CASCADE_CSS)
            {
                break;
            }
        }
    }

    return found;
}


/*
d_cascade_new
  Nothing is sized until the first reset names a node count.
*/
struct d_cascade*
d_cascade_new(
    const struct d_dss_sheet* _sheet,
    enum d_cascade_policy     _policy
)
{
    if (!_sheet)
    {
        return NULL;
    }

    struct d_cascade* const cascade = calloc(1u, sizeof(*cascade));

    if (cascade)
    {
        cascade->sheet  = _sheet;
        cascade->policy = (uint8_t)_policy;
    }

    return cascade;
}


/*
d_cascade_free
  Accepts NULL, as free does.
*/
void
d_cascade_free(
    struct d_cascade* _cascade
)
{
    if (_cascade)
    {
        free(_cascade->match_node);
        free(_cascade->match_rule);
        free(_cascade->match_spec);
        free(_cascade->offsets);
        free(_cascade->ranked);
        free(_cascade->ranked_spec);
        free(_cascade);
    }

    return;
}


/*
d_cascade_reset
  Buffers are kept between trees and only ever grow, as the tree's are.
*/
bool
d_cascade_reset(
    struct d_cascade* _cascade,
    size_t            _node_count
)
{
    if (!_cascade)
    {
        return false;
    }

    if (_node_count + 1u > _cascade->node_capacity)
    {
        uint32_t* const offsets = realloc(_cascade->offsets,
                                          (_node_count + 1u) *
                                          sizeof(uint32_t));

        if (!offsets)
        {
            return false;
        }

        _cascade->offsets       = offsets;
        _cascade->node_capacity = _node_count + 1u;
    }

    _cascade->node_count  = _node_count;
    _cascade->match_count = 0u;

    for (size_t at = 0u; at <= _node_count; ++at)
    {
        _cascade->offsets[at] = 0u;
    }

    return true;
}


/*
d_cascade_add
  Three parallel arrays rather than an array of structs, so ranking can hand
the rule and specificity columns out without copying.
*/
bool
d_cascade_add(
    struct d_cascade* _cascade,
    uint32_t          _node,
    uint32_t          _rule,
    uint32_t          _specificity
)
{
    if ( (!_cascade) || (_node >= _cascade->node_count) )
    {
        return false;
    }

    if (_cascade->match_count == _cascade->match_capacity)
    {
        const size_t grown = (_cascade->match_capacity == 0u)
                             ? 256u
                             : (_cascade->match_capacity * 2u);
        uint32_t* const nodes = realloc(_cascade->match_node,
                                        grown * sizeof(uint32_t));
        uint32_t* const rules = nodes ? realloc(_cascade->match_rule,
                                                grown * sizeof(uint32_t))
                                      : NULL;
        uint32_t* const specs = rules ? realloc(_cascade->match_spec,
                                                grown * sizeof(uint32_t))
                                      : NULL;

        // a failed realloc leaves the old block valid and owned
        if (nodes)
        {
            _cascade->match_node = nodes;
        }

        if (rules)
        {
            _cascade->match_rule = rules;
        }

        if (!specs)
        {
            return false;
        }

        _cascade->match_spec     = specs;
        _cascade->match_capacity = grown;
    }

    _cascade->match_node[_cascade->match_count] = _node;
    _cascade->match_rule[_cascade->match_count] = _rule;
    _cascade->match_spec[_cascade->match_count] = _specificity;
    ++_cascade->match_count;
    ++_cascade->offsets[_node + 1u];

    return true;
}


/*
d_cascade_rank
  Prefix-sums the per-node counts into offsets, places each match in its
node's run in the order recorded, then insertion-sorts each run.
*/
bool
d_cascade_rank(
    struct d_cascade* _cascade
)
{
    if (!_cascade)
    {
        return false;
    }

    const size_t count = _cascade->match_count;

    free(_cascade->ranked);
    free(_cascade->ranked_spec);
    _cascade->ranked      = malloc((count + 1u) * sizeof(uint32_t));
    _cascade->ranked_spec = malloc((count + 1u) * sizeof(uint32_t));

    uint32_t* const fill = malloc((_cascade->node_count + 1u) *
                                  sizeof(uint32_t));

    if ( (!_cascade->ranked) || (!_cascade->ranked_spec) || (!fill) )
    {
        free(fill);

        return false;
    }

    for (size_t node = 0u; node < _cascade->node_count; ++node)
    {
        _cascade->offsets[node + 1u] += _cascade->offsets[node];
        fill[node]                    = _cascade->offsets[node];
    }

    for (size_t at = 0u; at < count; ++at)
    {
        const uint32_t slot = fill[_cascade->match_node[at]]++;

        _cascade->ranked[slot]      = _cascade->match_rule[at];
        _cascade->ranked_spec[slot] = _cascade->match_spec[at];
    }

    free(fill);

    // each node's run, best first
    for (size_t node = 0u; node < _cascade->node_count; ++node)
    {
        const uint32_t first = _cascade->offsets[node];
        const uint32_t last  = _cascade->offsets[node + 1u];

        for (uint32_t at = first + 1u; at < last; ++at)
        {
            const uint32_t rule = _cascade->ranked[at];
            const uint32_t spec = _cascade->ranked_spec[at];
            uint32_t       hole = at;

            while ( (hole > first) &&
                    (d_internal_outranks(_cascade, rule, spec,
                                         _cascade->ranked[hole - 1u],
                                         _cascade->ranked_spec[hole - 1u])) )
            {
                _cascade->ranked[hole]      = _cascade->ranked[hole - 1u];
                _cascade->ranked_spec[hole] = _cascade->ranked_spec[hole - 1u];
                --hole;
            }

            _cascade->ranked[hole]      = rule;
            _cascade->ranked_spec[hole] = spec;
        }
    }

    return true;
}


size_t
d_cascade_matched(
    const struct d_cascade* _cascade,
    uint32_t                _node,
    const uint32_t**        _out_rules
)
{
    if ( (!_cascade) || (!_cascade->ranked) ||
         (_node >= _cascade->node_count) )
    {
        return 0u;
    }

    if (_out_rules)
    {
        *_out_rules = _cascade->ranked + _cascade->offsets[_node];
    }

    return (size_t)(_cascade->offsets[_node + 1u] - _cascade->offsets[_node]);
}


/*
d_cascade_winning_rule
  The runs are best first, so the first rule declaring the property is the
one every other must yield to.
*/
uint32_t
d_cascade_winning_rule(
    const struct d_cascade* _cascade,
    uint32_t                _node,
    uint32_t                _property
)
{
    const uint32_t* rules = NULL;
    const size_t    count = d_cascade_matched(_cascade, _node, &rules);

    for (size_t at = 0u; at < count; ++at)
    {
        if (d_internal_declaration_in(_cascade, rules[at], _property)
            != D_DSS_NO_INDEX)
        {
            return rules[at];
        }
    }

    return D_DSS_NO_INDEX;
}


bool
d_cascade_wins(
    const struct d_cascade* _cascade,
    uint32_t                _node,
    uint32_t                _rule,
    uint32_t                _property
)
{
    if ( (_cascade) && (_cascade->policy == D_CASCADE_ALL) )
    {
        return true;
    }

    return (d_cascade_winning_rule(_cascade, _node, _property) == _rule);
}


/*
d_cascade_computed
  CSS inheritance: an inherited property no rule declares at a node takes
its parent's value, up to the root, where it takes its initial value.
*/
uint32_t
d_cascade_computed(
    const struct d_cascade* _cascade,
    struct d_node_tree*     _tree,
    uint32_t                _node,
    uint32_t                _property,
    bool                    _inherited
)
{
    uint32_t node = _node;

    while ( (_cascade) && (_tree) && (node != D_DSS_NO_INDEX) )
    {
        const uint32_t rule = d_cascade_winning_rule(_cascade, node,
                                                     _property);

        if (rule != D_DSS_NO_INDEX)
        {
            return d_internal_declaration_in(_cascade, rule, _property);
        }

        if (!_inherited)
        {
            break;
        }

        node = d_node_at(_tree, node)->parent;
    }

    return D_DSS_NO_INDEX;
}
