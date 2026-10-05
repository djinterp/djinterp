/*******************************************************************************
* djinterp [djinterp]                                                   dmatch.c
*
* Selector matching:
*   Right-to-left evaluation of a complex selector.  The descendant and
* general-sibling combinators backtrack, because more than one ancestor or
* earlier sibling may satisfy the compound to their left; child and adjacent
* do not, because exactly one candidate exists.
*   Names are compared by resolving the sheet's interned identifier to text
* and finding it in the tree's table.  Sharing one table between sheet and
* tree would make this an integer compare; that is the obvious next
* optimisation and is not done here, because a shared table outlives both and
* the lifetime question has not been ruled on.
*
*
* path:      /src/djinterp/tools/dawk/dmatch.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dmatch.h"  // corresponding header
// std
#include <stdlib.h>  // malloc, calloc, free
#include <string.h>  // strcmp, strlen, strstr, memcpy


/*
d_internal_same_name
  Compares a sheet's name with a tree's: as symbols when the sheet is bound
to the tree's table, as text otherwise.
*/
static bool
d_internal_same_name(
    const struct d_dss_sheet*  _sheet,
    struct d_node_tree*        _tree,
    bool                       _bound,
    const struct d_dss_simple* _simple,
    uint32_t                   _tree_id
)
{
    if ( (_simple->name == D_DSS_NO_INDEX) || (_tree_id == D_DSS_NO_INDEX) )
    {
        return false;
    }

    // one table: a name is its symbol, and equal names are equal integers
    if (_bound)
    {
        return (_simple->symbol == _tree_id);
    }

    return (strcmp(d_dss_text(_sheet, _simple->name),
                   d_node_name(_tree, _tree_id)) == 0);
}


/*
d_internal_attribute_matches
  Evaluates one attribute predicate against a node.  A numeric operator on a
non-numeric value never matches, rather than comparing as text: the two
readings disagree and the silent one is the wrong one to pick.
*/
static bool
d_internal_attribute_matches(
    const struct d_dss_sheet*   _sheet,
    struct d_node_tree*         _tree,
    bool                        _bound,
    const struct d_node*        _node,
    const struct d_dss_simple*  _simple
)
{
    for (uint32_t at = 0; at < _node->attribute_count; ++at)
    {
        const struct d_node_attribute* const attribute =
            d_node_attribute_at(_tree, _node->first_attribute + at);

        if (!attribute)
        {
            continue;
        }

        if (!d_internal_same_name(_sheet, _tree, _bound, _simple,
                                  attribute->name))
        {
            continue;
        }

        if (_simple->op == D_DSS_ATTR_PRESENCE)
        {
            return true;
        }

        if (_simple->numeric)
        {
            if (!attribute->numeric)
            {
                return false;
            }

            switch (_simple->op)
            {
                case D_DSS_ATTR_EQUAL:
                    return (attribute->number == _simple->number);
                case D_DSS_ATTR_NOT_EQUAL:
                    return (attribute->number != _simple->number);
                case D_DSS_ATTR_LESS:
                    return (attribute->number <  _simple->number);
                case D_DSS_ATTR_LESS_EQUAL:
                    return (attribute->number <= _simple->number);
                case D_DSS_ATTR_GREATER:
                    return (attribute->number >  _simple->number);
                case D_DSS_ATTR_GREATER_EQUAL:
                    return (attribute->number >= _simple->number);
                default:
                    return false;
            }
        }

        const char* const want = d_dss_text(_sheet, _simple->value);
        const char* const have = d_node_text(_tree, attribute->value);

        const size_t want_length = strlen(want);
        const size_t have_length = strlen(have);

        switch (_simple->op)
        {
            case D_DSS_ATTR_EQUAL:
                return (strcmp(have, want) == 0);
            case D_DSS_ATTR_NOT_EQUAL:
                return (strcmp(have, want) != 0);
            case D_DSS_ATTR_PREFIX:
                return ( (want_length <= have_length) &&
                         (strncmp(have, want, want_length) == 0) );
            case D_DSS_ATTR_SUFFIX:
                return ( (want_length <= have_length) &&
                         (strcmp(have + (have_length - want_length),
                                 want) == 0) );
            case D_DSS_ATTR_SUBSTRING:
                return (strstr(have, want) != NULL);
            default:
                return false;
        }
    }

    return false;
}



/*
d_internal_nth_holds
  Whether a one-based position is An+B for some n >= 0.
*/
static bool
d_internal_nth_holds(
    int32_t _a,
    int32_t _b,
    long    _position
)
{
    // A of zero is a single position
    if (_a == 0)
    {
        return (_position == (long)_b);
    }

    const long offset = _position - (long)_b;

    // n = offset / A must be a whole number, and not negative
    return ( ((offset % (long)_a) == 0) &&
             ((offset / (long)_a) >= 0) );
}


/*
d_internal_position
  Counts the present siblings before and after a node, of its type only when
asked.  Position is document position, so an absent node -- a placeholder for
something the file does not carry -- is not a sibling for this purpose.
*/
static void
d_internal_position(
    struct d_node_tree* _tree,
    uint32_t            _node_index,
    bool                _same_type,
    long*               _out_before,
    long*               _out_after
)
{
    const struct d_node* const node = d_node_at(_tree, _node_index);
    bool                       seen = false;

    *_out_before = 0;
    *_out_after  = 0;

    for (uint32_t at = d_node_at(_tree, node->parent)->first_child;
         at != D_DSS_NO_INDEX;
         at = d_node_at(_tree, at)->next_sibling)
    {
        const struct d_node* const other = d_node_at(_tree, at);

        if (at == _node_index)
        {
            seen = true;
            continue;
        }

        if ( (!other->present) ||
             ( _same_type &&
               (other->type != node->type) ) )
        {
            continue;
        }

        if (seen)
        {
            ++*_out_after;
        }
        else
        {
            ++*_out_before;
        }
    }

    return;
}


static bool d_internal_has(const struct d_dss_sheet* _sheet,
                           struct d_node_tree*       _tree,
                           uint32_t                  _anchor,
                           uint32_t                  _selector);


/*
d_internal_functional_matches
  :not holds when no argument selector matches, :is when one does, :has when
one of its relative selectors finds a match anchored at the node.
*/
static bool
d_internal_functional_matches(
    const struct d_dss_sheet*  _sheet,
    struct d_node_tree*        _tree,
    uint32_t                   _node_index,
    const struct d_dss_simple* _simple
)
{
    const bool has = (_simple->pseudo == D_DSS_PSEUDO_HAS);
    const bool is  = (_simple->pseudo == D_DSS_PSEUDO_IS);

    for (uint32_t at = 0; at < _simple->argument_count; ++at)
    {
        const uint32_t selector = _simple->first_argument + at;
        const bool     hit      = has
            ? d_internal_has(_sheet, _tree, _node_index, selector)
            : d_match_selector(_sheet, _tree, _node_index, selector);

        if (hit)
        {
            return (has || is);
        }
    }

    return !(has || is);
}


/*
d_internal_pseudo_matches
  Evaluates a pseudo-class: the functional three by their selector arguments,
the structural ones by the node's position among its present siblings.  A
root node has no siblings, so every structural pseudo-class is false there;
the functional ones still apply.
*/
static bool
d_internal_pseudo_matches(
    const struct d_dss_sheet*  _sheet,
    struct d_node_tree*        _tree,
    uint32_t                   _node_index,
    const struct d_dss_simple* _simple
)
{
    struct d_node* const node   = d_node_at(_tree, _node_index);
    long                 before = 0;
    long                 after  = 0;

    if (!node)
    {
        return false;
    }

    if (_simple->argument_count > 0u)
    {
        return d_internal_functional_matches(_sheet, _tree, _node_index,
                                             _simple);
    }

    if (node->parent == D_DSS_NO_INDEX)
    {
        return false;
    }

    d_internal_position(_tree, _node_index,
                        ( (_simple->pseudo == D_DSS_PSEUDO_FIRST_OF_TYPE) ||
                          (_simple->pseudo == D_DSS_PSEUDO_LAST_OF_TYPE) ),
                        &before, &after);

    switch (_simple->pseudo)
    {
        case D_DSS_PSEUDO_FIRST_CHILD:
        case D_DSS_PSEUDO_FIRST_OF_TYPE:
            return (before == 0);

        case D_DSS_PSEUDO_LAST_CHILD:
        case D_DSS_PSEUDO_LAST_OF_TYPE:
            return (after == 0);

        case D_DSS_PSEUDO_ONLY_CHILD:
            return ( (before == 0) &&
                     (after == 0) );

        case D_DSS_PSEUDO_NTH_CHILD:
            return d_internal_nth_holds(_simple->nth_a, _simple->nth_b,
                                        before + 1);

        case D_DSS_PSEUDO_NTH_LAST_CHILD:
            return d_internal_nth_holds(_simple->nth_a, _simple->nth_b,
                                        after + 1);

        default:
            return false;
    }
}

/*
d_internal_compound_matches
  Every simple in the compound must hold.
*/
static bool
d_internal_compound_matches(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node_index,
    uint32_t                  _compound
)
{
    const struct d_dss_compound* const compound =
        d_dss_compound_at(_sheet, _compound);

    struct d_node* const node = d_node_at(_tree, _node_index);

    if ((!compound) || (!node))
    {
        return false;
    }

    // one pointer compare per compound buys integer name tests below
    const bool bound = ( (d_dss_symbols(_sheet) != NULL) &&
                         (d_dss_symbols(_sheet) == d_node_tree_symbols(_tree)) );

    for (uint32_t at = 0; at < compound->simple_count; ++at)
    {
        const struct d_dss_simple* const simple =
            d_dss_simple_at(_sheet, compound->first_simple + at);

        if (!simple)
        {
            return false;
        }

        switch (simple->kind)
        {
            case D_DSS_SIMPLE_UNIVERSAL:
                break;

            case D_DSS_SIMPLE_TYPE:
                if (!d_internal_same_name(_sheet, _tree, bound, simple,
                                          node->type))
                {
                    return false;
                }
                break;

            case D_DSS_SIMPLE_ATTRIBUTE:
                if (!d_internal_attribute_matches(_sheet, _tree, bound, node,
                                                  simple))
                {
                    return false;
                }
                break;

            case D_DSS_SIMPLE_PSEUDO_CLASS:
                if (!d_internal_pseudo_matches(_sheet, _tree, _node_index,
                                               simple))
                {
                    return false;
                }
                break;

            // classes and pseudo-elements have no host facts yet
            default:
                return false;
        }
    }

    return true;
}


/*
d_internal_previous_sibling
  Walks the parent's children to find the node immediately before this one.
Nodes link forwards only, so this is a scan rather than a field.
*/
static uint32_t
d_internal_previous_sibling(
    struct d_node_tree* _tree,
    uint32_t            _node_index
)
{
    struct d_node* const node = d_node_at(_tree, _node_index);

    if ((!node) || (node->parent == D_DSS_NO_INDEX))
    {
        return D_DSS_NO_INDEX;
    }

    struct d_node* const parent = d_node_at(_tree, node->parent);

    if ((!parent) || (parent->first_child == _node_index))
    {
        return D_DSS_NO_INDEX;
    }

    uint32_t at = parent->first_child;

    while (at != D_DSS_NO_INDEX)
    {
        struct d_node* const child = d_node_at(_tree, at);

        if ((!child) || (child->next_sibling == _node_index))
        {
            return at;
        }

        at = child->next_sibling;
    }

    return D_DSS_NO_INDEX;
}


/*
d_internal_chain
  Having matched the compound at `_step_index` on `_node_index`, satisfies
everything to its left.  A step index of D_DSS_NO_INDEX means the head has
been reached and the selector is satisfied.
*/
static bool
d_internal_chain(
    const struct d_dss_sheet*    _sheet,
    struct d_node_tree*          _tree,
    uint32_t                     _node_index,
    const struct d_dss_selector* _selector,
    uint32_t                     _step_index
)
{
    if (_step_index == D_DSS_NO_INDEX)
    {
        return true;
    }

    const struct d_dss_step* const step =
        d_dss_step_at(_sheet, _selector->first_step + _step_index);

    if (!step)
    {
        return false;
    }

    // the compound to the left is the previous step's, or the head
    const uint32_t target = (_step_index == 0)
                          ? _selector->head
                          : d_dss_step_at(_sheet,
                                          _selector->first_step
                                          + _step_index - 1u)->compound;

    const uint32_t next_step = (_step_index == 0) ? D_DSS_NO_INDEX
                                                  : (_step_index - 1u);

    struct d_node* const node = d_node_at(_tree, _node_index);

    if (!node)
    {
        return false;
    }

    switch (step->combinator)
    {
        case D_DSS_COMBINATOR_CHILD:
        {
            if (node->parent == D_DSS_NO_INDEX)
            {
                return false;
            }

            if (!d_internal_compound_matches(_sheet, _tree, node->parent,
                                             target))
            {
                return false;
            }

            return d_internal_chain(_sheet, _tree, node->parent, _selector,
                                    next_step);
        }

        case D_DSS_COMBINATOR_DESCENDANT:
        {
            uint32_t ancestor = node->parent;

            // any ancestor may satisfy it, so a failure keeps climbing
            while (ancestor != D_DSS_NO_INDEX)
            {
                if ( (d_internal_compound_matches(_sheet, _tree, ancestor,
                                                  target)) &&
                     (d_internal_chain(_sheet, _tree, ancestor, _selector,
                                       next_step)) )
                {
                    return true;
                }

                struct d_node* const up = d_node_at(_tree, ancestor);

                ancestor = up ? up->parent : D_DSS_NO_INDEX;
            }

            return false;
        }

        case D_DSS_COMBINATOR_ADJACENT:
        {
            const uint32_t previous = d_internal_previous_sibling(_tree,
                                                                  _node_index);

            if (previous == D_DSS_NO_INDEX)
            {
                return false;
            }

            if (!d_internal_compound_matches(_sheet, _tree, previous, target))
            {
                return false;
            }

            return d_internal_chain(_sheet, _tree, previous, _selector,
                                    next_step);
        }

        case D_DSS_COMBINATOR_SIBLING:
        {
            uint32_t previous = d_internal_previous_sibling(_tree,
                                                            _node_index);

            while (previous != D_DSS_NO_INDEX)
            {
                if ( (d_internal_compound_matches(_sheet, _tree, previous,
                                                  target)) &&
                     (d_internal_chain(_sheet, _tree, previous, _selector,
                                       next_step)) )
                {
                    return true;
                }

                previous = d_internal_previous_sibling(_tree, previous);
            }

            return false;
        }

        default:
            return false;
    }
}


/*
d_internal_next_in_subtree
  The node after `_at` in a preorder walk of the subtree rooted at `_root`, or
D_DSS_NO_INDEX when the walk is done.  Iterative, so a deep tree costs no
stack.
*/
static uint32_t
d_internal_next_in_subtree(
    struct d_node_tree* _tree,
    uint32_t            _root,
    uint32_t            _at
)
{
    const struct d_node* node = d_node_at(_tree, _at);

    // down first
    if (node->first_child != D_DSS_NO_INDEX)
    {
        return node->first_child;
    }

    // then right, climbing until a sibling exists, never above the root
    while (_at != _root)
    {
        if (node->next_sibling != D_DSS_NO_INDEX)
        {
            return node->next_sibling;
        }

        _at  = node->parent;
        node = d_node_at(_tree, _at);
    }

    return D_DSS_NO_INDEX;
}


/*
d_internal_forward
  Satisfies a relative selector's compounds left to right from an anchor: the
compound at `_part` (0 is the head) must match a node related to `_anchor` by
the combinator that introduces it, and the rest must follow from there.
Recursion is bounded by the selector's length, not the tree's size.
*/
static bool
d_internal_forward(
    const struct d_dss_sheet*    _sheet,
    struct d_node_tree*          _tree,
    uint32_t                     _anchor,
    const struct d_dss_selector* _selector,
    uint32_t                     _part
)
{
    const struct d_dss_step* const step = (_part == 0u)
        ? NULL
        : d_dss_step_at(_sheet, _selector->first_step + _part - 1u);
    const uint32_t compound   = step ? step->compound : _selector->head;
    const uint8_t  combinator = step ? step->combinator : _selector->leading;
    const bool     last       = (_part == _selector->step_count);
    const struct d_node* const anchor = d_node_at(_tree, _anchor);

    // children and descendants start below the anchor, siblings beside it
    uint32_t at = ( (combinator == D_DSS_COMBINATOR_CHILD) ||
                    (combinator == D_DSS_COMBINATOR_DESCENDANT) )
                ? anchor->first_child
                : anchor->next_sibling;

    while (at != D_DSS_NO_INDEX)
    {
        const struct d_node* const node = d_node_at(_tree, at);

        if ( node->present &&
             d_internal_compound_matches(_sheet, _tree, at, compound) &&
             ( last ||
               d_internal_forward(_sheet, _tree, at, _selector,
                                  _part + 1u) ) )
        {
            return true;
        }

        // `+` looks at the next present sibling only
        if ( (combinator == D_DSS_COMBINATOR_ADJACENT) &&
             node->present )
        {
            return false;
        }

        at = (combinator == D_DSS_COMBINATOR_DESCENDANT)
           ? d_internal_next_in_subtree(_tree, _anchor, at)
           : node->next_sibling;
    }

    return false;
}


/*
d_internal_has
  Whether a relative selector, anchored at a node, finds a match.
*/
static bool
d_internal_has(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _anchor,
    uint32_t                  _selector
)
{
    const struct d_dss_selector* const selector =
        d_dss_selector_at(_sheet, _selector);

    return ( selector &&
             d_internal_forward(_sheet, _tree, _anchor, selector, 0u) );
}


/*
d_match_selector
  Tests one complex selector against one node.
*/
bool
d_match_selector(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node,
    uint32_t                  _selector
)
{
    // parameter validation first
    if ((!_sheet) || (!_tree))
    {
        return false;
    }

    const struct d_dss_selector* const selector =
        d_dss_selector_at(_sheet, _selector);

    if (!selector)
    {
        return false;
    }

    // a selector with no steps is its head alone
    if (selector->step_count == 0)
    {
        return d_internal_compound_matches(_sheet, _tree, _node,
                                           selector->head);
    }

    const uint32_t last = selector->step_count - 1u;

    const struct d_dss_step* const step =
        d_dss_step_at(_sheet, selector->first_step + last);

    if (!step)
    {
        return false;
    }

    // rightmost first: a failure here costs nothing further
    if (!d_internal_compound_matches(_sheet, _tree, _node, step->compound))
    {
        return false;
    }

    return d_internal_chain(_sheet, _tree, _node, selector, last);
}


/*
d_match_rule
  Tests a node against a rule's selector list.  A node matches the rule when
it matches any one selector in the list.
*/
bool
d_match_rule(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node,
    const struct d_dss_rule*  _rule
)
{
    if ((!_sheet) || (!_tree) || (!_rule))
    {
        return false;
    }

    for (uint32_t at = 0; at < _rule->selector_count; ++at)
    {
        if (d_match_selector(_sheet, _tree, _node,
                             _rule->first_selector + at))
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_unsupported_in
  Finds a simple the matcher cannot evaluate in one selector, looking inside
the arguments of :not, :is and :has as well; nesting is bounded by the sheet.
*/
static const struct d_dss_simple*
d_internal_unsupported_in(
    const struct d_dss_sheet* _sheet,
    uint32_t                  _selector
)
{
    const struct d_dss_selector* const selector =
        d_dss_selector_at(_sheet, _selector);

    for (uint32_t part = 0; selector && (part <= selector->step_count); ++part)
    {
        const uint32_t compound_index = (part == 0)
            ? selector->head
            : d_dss_step_at(_sheet, selector->first_step + part - 1u)
                  ->compound;
        const struct d_dss_compound* const compound =
            d_dss_compound_at(_sheet, compound_index);

        for (uint32_t k = 0; k < compound->simple_count; ++k)
        {
            const struct d_dss_simple* const simple =
                d_dss_simple_at(_sheet, compound->first_simple + k);
            bool refused = ( (simple->kind == D_DSS_SIMPLE_CLASS) ||
                             (simple->kind == D_DSS_SIMPLE_PSEUDO_ELEMENT) );

            if (simple->kind == D_DSS_SIMPLE_PSEUDO_CLASS)
            {
                refused = (simple->pseudo == D_DSS_PSEUDO_UNKNOWN);
            }

            if (refused)
            {
                return simple;
            }

            for (uint32_t arg = 0; arg < simple->argument_count; ++arg)
            {
                const struct d_dss_simple* const inner =
                    d_internal_unsupported_in(_sheet,
                                              simple->first_argument + arg);

                if (inner)
                {
                    return inner;
                }
            }
        }
    }

    return NULL;
}


/*
d_match_unsupported
  Finds the first simple selector in a sheet that the matcher cannot
evaluate, and reports the line of the rule that carries it.  A pseudo-class
the matcher does not know would otherwise match nothing, silently -- a typo
that disables a rule and says nothing is the worst failure a linter can have.
*/
bool
d_match_unsupported(
    const struct d_dss_sheet* _sheet,
    uint32_t*                 _out_line,
    const char**              _out_name
)
{
    for (size_t at = 0; at < d_dss_rule_count(_sheet); ++at)
    {
        const struct d_dss_rule* const rule = d_dss_rule_at(_sheet, at);

        for (uint32_t which = 0; which < rule->selector_count; ++which)
        {
            const struct d_dss_simple* const simple =
                d_internal_unsupported_in(_sheet,
                                          rule->first_selector + which);

            if (simple)
            {
                *_out_line = rule->line;
                *_out_name = d_dss_text(_sheet, simple->name);

                return true;
            }
        }
    }

    return false;
}


// d_match_index
//   struct: rules bucketed by the type their subject compound names, in CSR
// form, plus the rules whose subject names no type.
struct d_match_index
{
    uint32_t*  offsets;          // limit + 1 entries
    uint32_t*  rules;            // bucketed rule indices, ascending per bucket
    uint32_t*  universal;        // rules any node may match, ascending
    size_t     universal_count;
    size_t     limit;            // symbols known when the index was built
    size_t     rule_count;
};


/**
 * @brief Reports the type symbol a selector's subject compound requires.
 *
 * @param[in] _sheet     a bound sheet.
 * @param[in] _selector  the selector.
 * @return the symbol, or `D_DSS_NO_INDEX` when any type may match.
 */
static uint32_t
d_internal_subject_type(
    const struct d_dss_sheet* _sheet,
    uint32_t                  _selector
)
{
    const struct d_dss_selector* const selector =
        d_dss_selector_at(_sheet, _selector);

    if (!selector)
    {
        return D_DSS_NO_INDEX;
    }

    const uint32_t subject = (selector->step_count == 0u)
        ? selector->head
        : d_dss_step_at(_sheet, selector->first_step +
                                selector->step_count - 1u)->compound;
    const struct d_dss_compound* const compound =
        d_dss_compound_at(_sheet, subject);

    for (uint32_t at = 0u; (compound) && (at < compound->simple_count); ++at)
    {
        const struct d_dss_simple* const simple =
            d_dss_simple_at(_sheet, compound->first_simple + at);

        if ( (simple->kind == D_DSS_SIMPLE_TYPE) &&
             (simple->symbol != D_DSS_NO_INDEX) )
        {
            return simple->symbol;
        }
    }

    return D_DSS_NO_INDEX;
}


/*
d_match_index_new
  Two passes over the rules: count each bucket, then fill it.  A rule whose
every subject names a type lands in each distinct bucket; a rule with any
subject naming none goes to the universal list instead, which reaches every
node already.
*/
struct d_match_index*
d_match_index_new(
    const struct d_dss_sheet* _sheet
)
{
    if (!_sheet)
    {
        return NULL;
    }

    struct d_symbol_table* const symbols = d_dss_symbols(_sheet);
    struct d_match_index* const  index   = calloc(1u, sizeof(*index));

    if (!index)
    {
        return NULL;
    }

    index->rule_count = d_dss_rule_count(_sheet);
    index->limit      = symbols ? d_symbol_count(symbols) : 0u;
    index->offsets    = calloc(index->limit + 1u, sizeof(uint32_t));
    index->universal  = malloc((index->rule_count + 1u) * sizeof(uint32_t));

    if ( (!index->offsets) || (!index->universal) )
    {
        d_match_index_free(index);

        return NULL;
    }

    uint32_t* fill = NULL;

    // pass 0 counts each bucket, pass 1 places each rule
    for (int pass = 0; pass < 2; ++pass)
    {
        for (size_t at = 0u; at < index->rule_count; ++at)
        {
            const struct d_dss_rule* const rule = d_dss_rule_at(_sheet, at);
            bool universal = (!symbols);

            for (uint32_t which = 0u;
                 (!universal) && (which < rule->selector_count);
                 ++which)
            {
                universal = (d_internal_subject_type(_sheet,
                                 rule->first_selector + which)
                             == D_DSS_NO_INDEX);
            }

            if (universal)
            {
                if (pass == 1)
                {
                    index->universal[index->universal_count++] =
                        (uint32_t)at;
                }

                continue;
            }

            for (uint32_t which = 0u; which < rule->selector_count; ++which)
            {
                const uint32_t type = d_internal_subject_type(_sheet,
                                          rule->first_selector + which);
                bool           seen = false;

                // a type two selectors share is one bucket entry
                for (uint32_t earlier = 0u; earlier < which; ++earlier)
                {
                    seen = ( (seen) ||
                             (d_internal_subject_type(_sheet,
                                  rule->first_selector + earlier) == type) );
                }

                if ( (seen) || (type >= index->limit) )
                {
                    continue;
                }

                if (pass == 0)
                {
                    ++index->offsets[type + 1u];
                }
                else
                {
                    index->rules[fill[type]++] = (uint32_t)at;
                }
            }
        }

        if (pass == 0)
        {
            for (size_t type = 0u; type < index->limit; ++type)
            {
                index->offsets[type + 1u] += index->offsets[type];
            }

            index->rules = malloc((index->offsets[index->limit] + 1u) *
                                  sizeof(uint32_t));
            fill         = malloc((index->limit + 1u) * sizeof(uint32_t));

            if ( (!index->rules) || (!fill) )
            {
                free(fill);
                d_match_index_free(index);

                return NULL;
            }

            memcpy(fill, index->offsets,
                   (index->limit + 1u) * sizeof(uint32_t));
        }
    }

    free(fill);

    return index;
}


/*
d_match_index_free
  Accepts NULL, as free does.
*/
void
d_match_index_free(
    struct d_match_index* _index
)
{
    if (_index)
    {
        free(_index->offsets);
        free(_index->rules);
        free(_index->universal);
        free(_index);
    }

    return;
}


/*
d_match_index_candidates
  Merges the type's bucket with the universal list, both ascending, so the
caller sees candidates in rule order -- the order first-match resolution and
every report depend on.  A type interned after the index was built has no
bucket and meets only the universal rules, which is exact: no rule names it.
*/
size_t
d_match_index_candidates(
    const struct d_match_index* _index,
    uint32_t                    _type,
    uint32_t*                   _out,
    size_t                      _capacity
)
{
    if ( (!_index) || (!_out) )
    {
        return 0u;
    }

    const bool            bucketed = (_type < _index->limit);
    const uint32_t* const bucket   = bucketed
                                     ? (_index->rules + _index->offsets[_type])
                                     : NULL;
    const size_t          size     = bucketed
                                     ? (size_t)(_index->offsets[_type + 1u] -
                                                _index->offsets[_type])
                                     : 0u;
    size_t                a        = 0u;
    size_t                b        = 0u;
    size_t                count    = 0u;

    // a two-way merge of ascending lists
    while ( (count < _capacity) &&
            ( (a < size) || (b < _index->universal_count) ) )
    {
        const bool from_bucket = ( (b >= _index->universal_count) ||
                                   ( (a < size) &&
                                     (bucket[a] < _index->universal[b]) ) );

        _out[count++] = from_bucket ? bucket[a++] : _index->universal[b++];
    }

    return count;
}


/*
d_match_rule_specificity
  Unlike d_match_rule it cannot stop at the first matching selector: the CSS
cascade ranks a rule by the most specific selector of its list that matched.
*/
bool
d_match_rule_specificity(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node,
    const struct d_dss_rule*  _rule,
    uint32_t*                 _out_specificity
)
{
    bool     matched = false;
    uint32_t best    = 0u;

    for (uint32_t at = 0u; (_rule) && (at < _rule->selector_count); ++at)
    {
        // every selector: the best of the matching ones is the rule's rank
        if (d_match_selector(_sheet, _tree, _node, _rule->first_selector + at))
        {
            const uint32_t specificity =
                d_dss_selector_at(_sheet, _rule->first_selector + at)
                    ->specificity;

            matched = true;
            best    = (specificity > best) ? specificity : best;
        }
    }

    if (_out_specificity)
    {
        *_out_specificity = best;
    }

    return matched;
}
