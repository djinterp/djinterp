/*******************************************************************************
* djinterp [djinterp]                                                dproperty.c
*
* Property evaluation:
*   Evaluation, derivation and repair for every property a sheet may name.  A
* property that is parsed but has no evaluator reports as unevaluated rather
* than passing: a rule that silently holds is worse than one that visibly
* cannot run yet.
*
*
* path:      /src/djinterp/tools/dawk/dproperty.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dproperty.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <string.h>  // strcmp, strlen, memcpy
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dlayout.h"  // d_layout_table_holds

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
d_internal_table_lookup
  Finds a row in a sheet `@table` by key and returns its first value.  This is
what makes @table more than decoration: a derivation reads the guide's own
data out of the sheet rather than carrying a copy compiled into C, so the
table has exactly one home and the sheet is the whole specification.
*/
static const char*
d_internal_table_lookup(
    const struct d_dss_sheet* _sheet,
    const char*               _table,
    const char*               _key
)
{
    for (size_t at = 0; at < d_dss_at_rule_count(_sheet); ++at)
    {
        const struct d_dss_at_rule* const rule = d_dss_at_rule_at(_sheet, at);

        if ((!rule) || (!rule->has_block))
        {
            continue;
        }

        if (strcmp(d_dss_text(_sheet, rule->name), "table") != 0)
        {
            continue;
        }

        if ( (rule->prelude == D_DSS_NO_INDEX) ||
             (strcmp(d_dss_text(_sheet, rule->prelude), _table) != 0) )
        {
            continue;
        }

        for (uint32_t which = 0; which < rule->declaration_count; ++which)
        {
            const struct d_dss_declaration* const row =
                d_dss_declaration_at(_sheet,
                                     rule->first_declaration + which);

            if ((!row) || (strcmp(d_dss_text(_sheet, row->property),
                                  _key) != 0))
            {
                continue;
            }

            if (row->value_count == 0)
            {
                return "";
            }

            const struct d_dss_value* const value =
                d_dss_value_at(_sheet, row->first_value);

            if (!value)
            {
                return "";
            }

            // an empty-string cell means the subsystem contributes no prefix
            return d_dss_text(_sheet, value->text);
        }
    }

    return NULL;
}


/*
d_internal_guard_name
  Derives a header's include guard from its path, exactly as the guide's
Include Guards section states it: DJINTERP_, the subsystem prefix for the
top-level directory, every remaining directory segment, the file stem, and
_H or _HPP.  Every run of characters that is not alphanumeric collapses to a
single underscore.
*/
static bool
d_internal_guard_name(
    const struct d_dss_sheet* _sheet,
    const char*               _path,
    char*                     _out,
    size_t                    _out_size
)
{
    const char* const first = strchr(_path, '/');

    if (!first)
    {
        return false;
    }

    char top[128];

    if ((size_t)(first - _path) >= sizeof(top))
    {
        return false;
    }

    (void)memcpy(top, _path, (size_t)(first - _path));

    top[first - _path] = '\0';

    const char* const prefix = d_internal_table_lookup(_sheet, "subsystem",
                                                       top);

    // a subsystem with no table row cannot have its guard derived
    if (!prefix)
    {
        return false;
    }

    const char* const dot = strrchr(_path, '.');

    const bool is_hpp = ((dot) && (strcmp(dot, ".hpp") == 0));

    size_t at = 0;

    const char* const lead = "DJINTERP_";

    (void)memcpy(_out, lead, strlen(lead));

    at = strlen(lead);

    for (size_t which = 0; prefix[which] != '\0'; ++which)
    {
        if (at + 1u >= _out_size)
        {
            return false;
        }

        _out[at] = prefix[which];
        ++at;
    }

    // everything after the top-level directory, stem included, extension not
    bool underscore = false;

    for (const char* scan = first + 1; scan < (dot ? dot : scan + 1); ++scan)
    {
        if (!dot)
        {
            break;
        }

        const char c = *scan;

        if ( ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) ||
             ((c >= '0') && (c <= '9')) )
        {
            if (at + 1u >= _out_size)
            {
                return false;
            }

            _out[at]   = (char)(((c >= 'a') && (c <= 'z')) ? (c - 32) : c);
            underscore = false;
            ++at;
        }
        else if (!underscore)
        {
            if (at + 1u >= _out_size)
            {
                return false;
            }

            _out[at]   = '_';
            underscore = true;
            ++at;
        }
    }

    const char* const tail = is_hpp ? "_HPP" : "_H";

    if ((at + strlen(tail) + 1u) >= _out_size)
    {
        return false;
    }

    (void)memcpy(_out + at, tail, strlen(tail) + 1u);

    return true;
}


/*
d_internal_format_of
  Returns the pattern string of a `format:` declaration's function argument --
the "YYYY.MM.DD" in `format: date("YYYY.MM.DD")` -- or NULL.
*/
static const char*
d_internal_format_of(
    const struct d_dss_sheet*       _sheet,
    const struct d_dss_declaration* _declaration
)
{
    for (uint32_t at = 0; at < _declaration->value_count; ++at)
    {
        const struct d_dss_value* const value =
            d_dss_value_at(_sheet, _declaration->first_value + at);

        if ((!value) || (value->kind != D_DSS_VALUE_FUNCTION))
        {
            continue;
        }

        for (uint32_t arg = 0; arg < value->arg_count; ++arg)
        {
            const struct d_dss_value* const argument =
                d_dss_value_at(_sheet, value->first_arg + arg);

            if ((argument) && (argument->kind == D_DSS_VALUE_STRING))
            {
                return d_dss_text(_sheet, argument->text);
            }
        }
    }

    return NULL;
}


/*
d_internal_format_matches_at
  Tests whether `_text` matches `_pattern` exactly, starting at `_at`.  In a
pattern, Y, M and D each stand for one decimal digit and every other
character stands for itself.  A date is the only format the guide needs, and
this is the smallest notation that states one without becoming a regex.
*/
static bool
d_internal_format_matches_at(
    const char* _text,
    size_t      _at,
    const char* _pattern
)
{
    for (size_t which = 0; _pattern[which] != '\0'; ++which)
    {
        const char c = _text[_at + which];

        if (c == '\0')
        {
            return false;
        }

        const char want = _pattern[which];

        if ((want == 'Y') || (want == 'M') || (want == 'D'))
        {
            if ((c < '0') || (c > '9'))
            {
                return false;
            }
        }
        else if (c != want)
        {
            return false;
        }
    }

    return true;
}


/*
d_property_format_extract
  Finds the first run of a node's text that matches a `format:` pattern and
copies it out.  This is the format repair: a value that contains a valid date
alongside something else -- `date: 2026.03.25` under a `created:` label --
keeps the date and loses the rest.  It is derivable without a human, which is
the test every repair has to pass, and it fails closed: no match, no repair.
*/
bool
d_property_format_extract(
    const struct d_dss_sheet*       _sheet,
    struct d_node_tree*             _tree,
    uint32_t                        _node,
    const struct d_dss_declaration* _declaration,
    char*                           _out,
    size_t                          _out_size
)
{
    const char* const pattern = d_internal_format_of(_sheet, _declaration);

    struct d_node* const node = d_node_at(_tree, _node);

    if ((!pattern) || (!node))
    {
        return false;
    }

    const char* const text        = d_node_text(_tree, node->text);
    const size_t      text_length = strlen(text);
    const size_t      length      = strlen(pattern);

    if ((length == 0) || (length >= _out_size) || (length > text_length))
    {
        return false;
    }

    for (size_t at = 0; (at + length) <= text_length; ++at)
    {
        if (d_internal_format_matches_at(text, at, pattern))
        {
            (void)memcpy(_out, text + at, length);

            _out[length] = '\0';

            return true;
        }
    }

    return false;
}

/*
d_property_derive
  Computes the value a `content:` declaration says a node should hold.  A
derivation that needs a source this build does not have -- commit history,
for instance -- returns false, and the property is reported as unevaluated
rather than silently passing.
*/
bool
d_property_derive(
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

    // the guide's own spelling of a field's label, from what the field means
    if (strcmp(name, "canonical-label") == 0)
    {
        const char* const meaning = d_internal_ancestor_attribute(_tree,
                                                                  _node_index,
                                                                  "name");

        static const struct
        {
            const char* meaning;
            const char* spelling;
        }
        labels[] =
        {
            { "path",    "path"      },
            { "link",    "link(s)"   },
            { "author",  "author(s)" },
            { "created", "created"   },
            { "revised", "revised"   }
        };

        for (size_t at = 0; (meaning) && (at < 5u); ++at)
        {
            if ( (strcmp(meaning, labels[at].meaning) == 0) &&
                 (strlen(labels[at].spelling) < _out_size) )
            {
                (void)memcpy(_out, labels[at].spelling,
                             strlen(labels[at].spelling) + 1u);
                return true;
            }
        }

        return false;
    }

    if (strcmp(name, "guard-name") == 0)
    {
        const char* const path = d_internal_ancestor_attribute(_tree,
                                                               _node_index,
                                                               "path");

        if (!path)
        {
            return false;
        }

        return d_internal_guard_name(_sheet, path, _out, _out_size);
    }

    // the commit-history extension supplies these as file-node attributes;
    // a tree with no history has none, and the rule reports as unevaluated
    static const struct
    {
        const char* function;
        const char* attribute;
    }
    history[] =
    {
        { "first-commit-date", "first-commit" },
        { "last-commit-date",  "last-commit"  }
    };

    for (size_t at = 0; at < 2u; ++at)
    {
        if (strcmp(name, history[at].function) != 0)
        {
            continue;
        }

        const char* const date =
            d_internal_ancestor_attribute(_tree, _node_index,
                                          history[at].attribute);

        if ((!date) || (date[0] == '\0') || (strlen(date) >= _out_size))
        {
            return false;
        }

        (void)memcpy(_out, date, strlen(date) + 1u);

        return true;
    }

    return false;
}

/*
d_property_number
  Returns the first numeric value term of a declaration, or -1 when the
declaration carries none.  A property whose value is not a number is simply
not a geometry property and is skipped by the geometry evaluator.
*/
double
d_property_number(
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
d_property_evaluate
  Applies one declaration to one matched node and reports whether it held.
This is the engine's one evaluator, and every driver -- dcheck, the lint
driver, the extension tests -- goes through it, so a sheet means the same
thing wherever it runs (docs/dss-evaluation.md).  `violation` fails on every
present node it is applied to.  Modifiers -- severity, initial, immutable --
are not claims about a node and report as unevaluated, as does a property
whose derivation source is missing, rather than silently passing.
*/
bool
d_property_evaluate(
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

    const double want = d_property_number(_sheet, _declaration);

    // `violation` is an assertion that the match should not exist: any node
    // the rule selects fails it.  It names no expected value, so it is never
    // repaired -- only reported, with the declaration's text as the message.
    if (strcmp(property, "violation") == 0)
    {
        *_out_evaluated = node->present;

        return !node->present;
    }

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
        char derived[D_PROPERTY_TEXT_MAX];

        if (!d_property_derive(_sheet, _tree, _node_index, _declaration,
                               derived, sizeof(derived)))
        {
            *_out_evaluated = false;
            return true;
        }

        return (strcmp(d_node_text(_tree, node->text), derived) == 0);
    }

    // `display: table` is a claim about a node's rows, not about the node:
    // its columns must line up.  Row and cell displays are structure that
    // the table reads; on their own they assert nothing and do not evaluate.
    if (strcmp(property, "display") == 0)
    {
        const struct d_dss_value* const value =
            d_dss_value_at(_sheet, _declaration->first_value);

        if ( (value) && (value->text != D_DSS_NO_INDEX) &&
             (strcmp(d_dss_text(_sheet, value->text), "table") == 0) )
        {
            return d_layout_table_holds(_sheet, _tree, _node_index,
                                        _out_evaluated);
        }

        // `display: none` holds exactly when the node is not there
        if ( (value) && (value->text != D_DSS_NO_INDEX) &&
             (strcmp(d_dss_text(_sheet, value->text), "none") == 0) )
        {
            return (!node->present);
        }

        *_out_evaluated = false;
        return true;
    }

    // the whole value must be the format -- a value that merely contains a
    // date somewhere is exactly the defect this property exists to catch
    if (strcmp(property, "format") == 0)
    {
        const char* const pattern = d_internal_format_of(_sheet, _declaration);

        if (!pattern)
        {
            *_out_evaluated = false;
            return true;
        }

        const char* const text = d_node_text(_tree, node->text);

        return ( (strlen(text) == strlen(pattern)) &&
                 (d_internal_format_matches_at(text, 0u, pattern)) );
    }

    // initial, immutable and severity still need a source that does not
    // exist yet; they are not silently passed
    *_out_evaluated = false;

    return true;
}




/*
d_property_repair
  Computes the corrected text for a node that failed a geometry property, or
returns false when no repair is derivable.  A repair exists only where the
expected value is computable without asking a human; everything else is
reported and left alone.
*/
bool
d_property_repair(
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

    const char* const type = d_node_name(_tree, node->type);

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

    // a delimiter's width is its fill: keep the prefix, regenerate the run
    if ((strcmp(type, "delimiter") == 0) && (strcmp(_property, "width") == 0))
    {
        const bool   spaced = (_line_text[2] == ' ');
        const size_t prefix = spaced ? 3u : 2u;
        const char   fill   = _line_text[prefix];

        if ((want <= prefix) || ((fill != '=') && (fill != '-')))
        {
            return false;
        }

        (void)memcpy(_out, _line_text, prefix);

        for (size_t at = prefix; at < want; ++at)
        {
            _out[at] = fill;
        }

        _out[want] = '\0';

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

        // at least one space must survive between what precedes the run and
        // the run itself, or anchoring glues a value to its own label
        if ((prefix + 1u + run) > want)
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

#ifdef D_AUDIT_FAULT_CORRUPT
        // test only: a deliberately wrong repair that damages the run it
        // moves.  It must never reach a file while the audit is compiled in.
        _out[want - 1u] = '#';
#endif

        return true;
    }

    // a run is placed at a start column by changing the spaces before it;
    // everything after the run is kept, and one space must survive before it
    if (strcmp(_property, "start-column") == 0)
    {
        const size_t line_length = strlen(_line_text);
        const size_t run         = node->width;
        const size_t head        = (node->start_column > 0u)
                                 ? (node->start_column - 1u)
                                 : 0u;

        if ((run == 0) || ((head + run) > line_length) || (want == 0))
        {
            return false;
        }

        size_t prefix = head;

        while ((prefix > 0) && (_line_text[prefix - 1u] == ' '))
        {
            --prefix;
        }

        const size_t target = want - 1u;

        if ( ((prefix + 1u) > target) ||
             ((target + (line_length - head) + 1u) > _out_size) )
        {
            return false;
        }

        (void)memcpy(_out, _line_text, prefix);

        for (size_t at = prefix; at < target; ++at)
        {
            _out[at] = ' ';
        }

        (void)memcpy(_out + target, _line_text + head, line_length - head);

        _out[target + (line_length - head)] = '\0';

        return true;
    }

    return false;
}



// d_internal_lint_properties
//   constant: every property a lint sheet may declare, sorted by name.  The
// evaluable ones share d_property_evaluate, which dispatches within; the
// modifiers -- severity, initial, immutable -- and min-gap, which a table
// layout reads beside its display, go through it too and come back
// unevaluated, exactly as before the registry.
static const struct d_schema_property d_internal_lint_properties[] =
{
    { "content",      false, NULL, NULL, d_property_evaluate, NULL },
    { "display",      false, NULL, NULL, d_property_evaluate, NULL },
    { "end-column",   false, NULL, NULL, d_property_evaluate, NULL },
    { "format",       false, NULL, NULL, d_property_evaluate, NULL },
    { "immutable",    false, NULL, NULL, d_property_evaluate, NULL },
    { "initial",      false, NULL, NULL, d_property_evaluate, NULL },
    { "min-gap",      false, NULL, NULL, d_property_evaluate, NULL },
    { "required",     false, NULL, NULL, d_property_evaluate, NULL },
    { "severity",     false, NULL, NULL, d_property_evaluate, NULL },
    { "start-column", false, NULL, NULL, d_property_evaluate, NULL },
    { "violation",    false, NULL, NULL, d_property_evaluate, NULL },
    { "width",        false, NULL, NULL, d_property_evaluate, NULL }
};


const struct d_schema*
d_property_schema(
    void
)
{
    static const struct d_schema schema =
    {
        d_internal_lint_properties,
        sizeof(d_internal_lint_properties) /
        sizeof(d_internal_lint_properties[0])
    };

    return &schema;
}
