/*******************************************************************************
* djinterp [dawk]                                                    dregistry.c
*
* Property registry:
*   Evaluation, derivation and repair for every property a sheet may name.  A
* property that is parsed but has no evaluator reports as unevaluated rather
* than passing: a rule that silently holds is worse than one that visibly
* cannot run yet.
*
* path:      /src/djinterp/tools/dawk/dregistry.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dregistry.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <string.h>  // strcmp, strlen, memcpy
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dbanner.h"  // D_BANNER_LINE_MAX

/*
d_internal_ancestor_attribute
  Returns the value of an attribute on this node or the nearest ancestor that
carries it.  A derivation names a fact, not a place: top-level-dir() should
not have to know that `top` lives on the file node three levels up.
*/
static const char*
d_internal_ancestor_attribute(
    struct d_node_tree* _tree,
    uint32_t            _node_index,
    const char*         _name
)
{
    const uint32_t wanted = d_node_find_id(_tree, _name);

    if (wanted == D_DSS_NO_INDEX)
    {
        return NULL;
    }

    uint32_t at = _node_index;

    while (at != D_DSS_NO_INDEX)
    {
        const struct d_node* const node = d_node_at(_tree, at);

        if (!node)
        {
            return NULL;
        }

        for (uint32_t which = 0; which < node->attribute_count; ++which)
        {
            const struct d_node_attribute* const attribute =
                d_node_attribute_at(_tree, node->first_attribute + which);

            if ((attribute) && (attribute->name == wanted))
            {
                return d_node_text(_tree, attribute->value);
            }
        }

        at = node->parent;
    }

    return NULL;
}


/*
d_registry_derive
  Computes the value a `content:` declaration says a node should hold.  A
derivation that needs a source this build does not have -- commit history,
for instance -- returns false, and the property is reported as unevaluated
rather than silently passing.
*/
bool
d_registry_derive(
    const struct d_dss_sheet*       _sheet,
    struct d_node_tree*             _tree,
    uint32_t                        _node_index,
    const struct d_dss_declaration* _declaration,
    char*                           _out,
    size_t                          _out_size
)
{
    const struct d_dss_value* function = NULL;

    for (uint32_t at = 0; at < _declaration->value_count; ++at)
    {
        const struct d_dss_value* const value =
            d_dss_value_at(_sheet, _declaration->first_value + at);

        if ((value) && (value->kind == D_DSS_VALUE_FUNCTION))
        {
            function = value;
            break;
        }
    }

    if (!function)
    {
        return false;
    }

    const char* const name = d_dss_text(_sheet, function->text);

    if (strcmp(name, "top-level-dir") == 0)
    {
        const char* const top = d_internal_ancestor_attribute(_tree,
                                                              _node_index,
                                                              "top");

        if ((!top) || (top[0] == '\0') || (strlen(top) >= _out_size))
        {
            return false;
        }

        (void)memcpy(_out, top, strlen(top) + 1u);

        return true;
    }

    if (strcmp(name, "file-name") == 0)
    {
        const char* const file_name =
            d_internal_ancestor_attribute(_tree, _node_index, "name");

        if ((!file_name) || (strlen(file_name) >= _out_size))
        {
            return false;
        }

        (void)memcpy(_out, file_name, strlen(file_name) + 1u);

        return true;
    }

    if (strcmp(name, "repo-path") == 0)
    {
        const char* const path = d_internal_ancestor_attribute(_tree,
                                                               _node_index,
                                                               "path");

        if ((!path) || ((strlen(path) + 15u) >= _out_size))
        {
            return false;
        }

        (void)snprintf(_out, _out_size, "/inc/djinterp/%s", path);

        return true;
    }

    // first-commit-date() and last-commit-date() need history, which no
    // extension supplies yet
    return false;
}

/*
d_registry_number
  Returns the first numeric value term of a declaration, or -1 when the
declaration carries none.  A property whose value is not a number is simply
not a geometry property and is skipped by the geometry evaluator.
*/
double
d_registry_number(
    const struct d_dss_sheet*       _sheet,
    const struct d_dss_declaration* _declaration
)
{
    for (uint32_t at = 0; at < _declaration->value_count; ++at)
    {
        const struct d_dss_value* const value =
            d_dss_value_at(_sheet, _declaration->first_value + at);

        if ((value) && (value->kind == D_DSS_VALUE_NUMBER))
        {
            return value->number;
        }
    }

    return -1.0;
}


/*
d_registry_evaluate
  Applies one declaration to one matched node and reports whether it held.
Only the geometry and presence properties are evaluated here; content and
format need a derivation source and history respectively, and are counted as
matched-but-unevaluated rather than silently passing.
*/
bool
d_registry_evaluate(
    const struct d_dss_sheet*       _sheet,
    struct d_node_tree*             _tree,
    uint32_t                        _node_index,
    const struct d_dss_declaration* _declaration,
    bool*                           _out_evaluated
)
{
    const char* const property = d_dss_text(_sheet, _declaration->property);

    struct d_node* const node = d_node_at(_tree, _node_index);

    *_out_evaluated = true;

    if (!node)
    {
        return false;
    }

    const double want = d_registry_number(_sheet, _declaration);

    // an absent node has no geometry; only `required` has anything to say
    if ((!node->present) && (strcmp(property, "required") != 0))
    {
        *_out_evaluated = false;
        return true;
    }

    if (strcmp(property, "width") == 0)
    {
        // `width: auto` states that the node is prose and is not measured
        if (want < 0.0)
        {
            *_out_evaluated = false;
            return true;
        }

        return ((double)node->width == want);
    }

    if (strcmp(property, "end-column") == 0)
    {
        return ((double)node->end_column == want);
    }

    if (strcmp(property, "start-column") == 0)
    {
        return ((double)node->start_column == want);
    }

    if (strcmp(property, "required") == 0)
    {
        return node->present;
    }

    if (strcmp(property, "content") == 0)
    {
        char derived[D_BANNER_LINE_MAX];

        if (!d_registry_derive(_sheet, _tree, _node_index, _declaration,
                               derived, sizeof(derived)))
        {
            *_out_evaluated = false;
            return true;
        }

        return (strcmp(d_node_text(_tree, node->text), derived) == 0);
    }

    // format, initial, immutable and severity still need a source that does
    // not exist yet; they are not silently passed
    *_out_evaluated = false;

    return true;
}




/*
d_registry_repair
  Computes the corrected text for a node that failed a geometry property, or
returns false when no repair is derivable.  A repair exists only where the
expected value is computable without asking a human; everything else is
reported and left alone.
*/
bool
d_registry_repair(
    struct d_node_tree* _tree,
    uint32_t            _node_index,
    const char*         _property,
    double              _want,
    const char*         _line_text,
    const char*         _derived,
    char*               _out,
    size_t              _out_size
)
{
    struct d_node* const node = d_node_at(_tree, _node_index);

    if (!node)
    {
        return false;
    }

    const char* const type = d_node_text(_tree, node->type);

    // A content repair substitutes the node's run in place.  The line's
    // length changes, which is why this cannot be the last word: an anchored
    // neighbour on the same line is now wrong and a further pass fixes it.
    if (_derived)
    {
        const size_t head = (node->start_column > 0u)
                          ? (node->start_column - 1u)
                          : 0u;
        const size_t run  = node->width;

        if ((run == 0) || ((head + run) > strlen(_line_text)))
        {
            return false;
        }

        const size_t tail = strlen(_line_text) - (head + run);

        if ((head + strlen(_derived) + tail + 1u) > _out_size)
        {
            return false;
        }

        (void)memcpy(_out, _line_text, head);
        (void)memcpy(_out + head, _derived, strlen(_derived));
        (void)memcpy(_out + head + strlen(_derived),
                     _line_text + head + run, tail);

        _out[head + strlen(_derived) + tail] = '\0';

        return true;
    }

    if ((_want <= 0.0) || ((size_t)_want >= _out_size))
    {
        return false;
    }

    const size_t want = (size_t)_want;

    // a rule is a run of stars with a slash at one end; its width is the
    // star count, so a repair is a regeneration rather than a pad
    if ((strcmp(type, "rule") == 0) && (strcmp(_property, "width") == 0))
    {
        const bool opening = (_line_text[0] == '/');

        size_t at = 0;

        if (opening)
        {
            _out[at] = '/';
            ++at;
        }

        while (at < (want - (opening ? 0u : 1u)))
        {
            _out[at] = '*';
            ++at;
        }

        if (!opening)
        {
            _out[at] = '/';
            ++at;
        }

        _out[at] = '\0';

        return true;
    }

    // an anchored run is repaired by changing the spaces before it, which
    // is only possible when the run itself is short enough to fit
    if (strcmp(_property, "end-column") == 0)
    {
        const size_t run = node->width;
        const size_t head = (node->start_column > 0u)
                          ? (node->start_column - 1u)
                          : 0u;

        if ((run == 0) || (run > want) || (head == 0))
        {
            return false;
        }

        // the text before the run must itself end before the new start
        size_t prefix = head;

        while ((prefix > 0) && (_line_text[prefix - 1u] == ' '))
        {
            --prefix;
        }

        if ((prefix + run) > want)
        {
            return false;
        }

        memcpy(_out, _line_text, prefix);

        for (size_t at = prefix; at < (want - run); ++at)
        {
            _out[at] = ' ';
        }

        memcpy(_out + (want - run), _line_text + head, run);

        _out[want] = '\0';

        return true;
    }

    return false;
}

