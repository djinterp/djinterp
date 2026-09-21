/*******************************************************************************
* djinterp [dawk]                                                       dmatch.c
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
* path:      /src/djinterp/tools/dawk/dmatch.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dmatch.h"  // corresponding header
// std
#include <string.h>  // strcmp, strlen, strstr


/*
d_internal_same_name
  Compares a sheet-interned identifier with a tree-interned one by text.
*/
static bool
d_internal_same_name(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _sheet_id,
    uint32_t                  _tree_id
)
{
    if ((_sheet_id == D_DSS_NO_INDEX) || (_tree_id == D_DSS_NO_INDEX))
    {
        return false;
    }

    return (strcmp(d_dss_text(_sheet, _sheet_id),
                   d_node_text(_tree, _tree_id)) == 0);
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

        if (!d_internal_same_name(_sheet, _tree, _simple->name,
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
                if (!d_internal_same_name(_sheet, _tree, simple->name,
                                          node->type))
                {
                    return false;
                }
                break;

            case D_DSS_SIMPLE_ATTRIBUTE:
                if (!d_internal_attribute_matches(_sheet, _tree, node, simple))
                {
                    return false;
                }
                break;

            // classes and pseudos have no host facts yet; they never match
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
