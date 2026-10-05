/*******************************************************************************
* djinterp [djinterp]                                                   dcheck.c
*
* Check driver:
*   Walks a tree, asks an extension to build nodes for each file, matches a
* stylesheet against them, and reports what the registry says is wrong.  The
* driver knows nothing about banners, properties or file formats; it knows
* only how to run the loop.
*   Under --fix it applies at most one repair per file per pass.  A node's
* columns are measured once, from the file as it was read, so a second repair
* in the same pass would splice at offsets the first has already moved.  The
* next pass measures what is actually on disk, and the run repeats until no
* repair is produced.
*
*
* path:      /src/djinterp/tools/dawk/dcheck.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dmatch.h"  // corresponding header
// std
#include <limits.h>  // PATH_MAX
#include <stdint.h>  // SIZE_MAX, UINT32_MAX
#include <stdio.h>   // FILE, fopen, printf
#include <stdlib.h>  // malloc, calloc, free
#include <string.h>  // strcmp, strrchr
// djinterp
#include "../../../../inc/djinterp/tools/dawk/daudit.h"     // d_audit_verify
#include "../../../../inc/djinterp/tools/dawk/dbanner.h"    // d_banner_build
#include "../../../../inc/djinterp/tools/dawk/dedit.h"      // d_edit_rewrite_line
#include "../../../../inc/djinterp/tools/dawk/ext/dext_source.h"  // d_ext_source_build
#include "../../../../inc/djinterp/tools/dawk/dgit.h"       // d_git_read
#include "../../../../inc/djinterp/tools/dawk/dguard.h"     // d_guard_build
#include "../../../../inc/djinterp/tools/dawk/dinclude.h"   // d_include_build
#include "../../../../inc/djinterp/tools/dawk/dlayout.h"    // d_layout_table_repair
#include "../../../../inc/djinterp/tools/dawk/dline.h"      // d_line_build
#include "../../../../inc/djinterp/tools/dawk/dcascade.h"   // d_cascade_wins
#include "../../../../inc/djinterp/tools/dawk/dschema.h"    // d_schema_validate
#include "../../../../inc/djinterp/tools/dawk/dsymbol.h"    // d_symbol_table
#include "../../../../inc/djinterp/tools/dawk/dsection.h"   // d_section_build
#include "../../../../inc/djinterp/tools/dawk/dsettings.h"  // d_settings
#include "../../../../inc/djinterp/tools/dawk/dnode.h"      // d_node_tree
#include "../../../../inc/djinterp/tools/dawk/dproperty.h"  // d_property_evaluate
#include "../../../../inc/djinterp/tools/dawk/dsource.h"    // d_awk_source
#include "../../../../inc/djinterp/tools/dawk/dss.h"        // d_dss_sheet
#include "../../../../inc/djinterp/tools/dawk/dtree.h"      // d_awk_source_tree_init


// d_check_tally
//   struct: one row of the report.  Counting per property per rule is what
// distinguishes a rule that matched nothing from one that matched and held.
struct d_check_tally
{
    const char*  property;
    uint32_t     line;
    size_t       matched;
    size_t       failed;
};



/*
d_internal_meaning_holds
  Reports whether every content and format declaration matching a node holds.
A geometry repair is applied only when this is true.  Aligning a malformed
value is worse than leaving it visibly wrong: it removes the one signal a
reader would have used to notice, and it makes a defect look finished.
*/

/*
d_internal_redundant
  Reports whether removing a field loses nothing: an earlier sibling carries
the same `name` and a value with the same text.  `display: none` on a node
whose content exists nowhere else is refused, because deleting the only copy
of something is not a repair.
*/

/*
d_internal_swap_holder
  Finds a later sibling of the same type, on the same line, whose text is
already `_derived`.  When one exists, a content repair is a swap, not a
replacement: replacing would write the value a second time and destroy the
one it overwrote, where exchanging the two keeps both.
*/
static uint32_t
d_internal_swap_holder(
    struct d_node_tree* _tree,
    uint32_t            _node,
    const char*         _derived
)
{
    const struct d_node* const node = d_node_at(_tree, _node);

    if ((!node) || (node->parent == D_DSS_NO_INDEX))
    {
        return D_DSS_NO_INDEX;
    }

    for (uint32_t at = node->next_sibling;
         at != D_DSS_NO_INDEX;
         at = d_node_at(_tree, at)->next_sibling)
    {
        const struct d_node* const other = d_node_at(_tree, at);

        if ( (other->present) && (other->type == node->type) &&
             (other->line == node->line) &&
             (strcmp(d_node_text(_tree, other->text), _derived) == 0) )
        {
            return at;
        }
    }

    return D_DSS_NO_INDEX;
}


/*
d_internal_swap_runs
  Rebuilds a line with two runs exchanged.  Both runs are checked against the
line first, so a node whose recorded geometry has drifted from the file is
refused rather than spliced.
*/
static bool
d_internal_swap_runs(
    struct d_node_tree* _tree,
    uint32_t            _left,
    uint32_t            _right,
    const char*         _line,
    char*               _out,
    size_t              _out_size
)
{
    const struct d_node* const a = d_node_at(_tree, _left);
    const struct d_node* const b = d_node_at(_tree, _right);

    const size_t length = strlen(_line);
    const size_t a0     = a->start_column - 1u;
    const size_t a1     = a0 + a->width;
    const size_t b0     = b->start_column - 1u;
    const size_t b1     = b0 + b->width;

    if ((a1 > b0) || (b1 > length) || ((length + 1u) > _out_size) ||
        (strncmp(_line + a0, d_node_text(_tree, a->text), a->width) != 0) ||
        (strncmp(_line + b0, d_node_text(_tree, b->text), b->width) != 0))
    {
        return false;
    }

    size_t used = 0;

    (void)memcpy(_out + used, _line, a0);            used += a0;
    (void)memcpy(_out + used, _line + b0, b->width); used += b->width;
    (void)memcpy(_out + used, _line + a1, b0 - a1);  used += b0 - a1;
    (void)memcpy(_out + used, _line + a0, a->width); used += a->width;
    (void)memcpy(_out + used, _line + b1, length - b1);
    used += length - b1;

    _out[used] = '\0';

    return true;
}


static bool
d_internal_redundant(
    struct d_node_tree* _tree,
    uint32_t            _node
)
{
    const struct d_node* const node = d_node_at(_tree, _node);

    if ((!node) || (node->parent == D_DSS_NO_INDEX) ||
        (node->first_child == D_DSS_NO_INDEX))
    {
        return false;
    }

    const uint32_t name_id  = d_node_find_id(_tree, "name");
    uint32_t       my_name  = D_DSS_NO_INDEX;

    for (uint32_t at = 0; at < node->attribute_count; ++at)
    {
        const struct d_node_attribute* const attribute =
            d_node_attribute_at(_tree, node->first_attribute + at);

        if ((attribute) && (attribute->name == name_id))
        {
            my_name = attribute->value;
        }
    }

    const char* const my_value =
        d_node_text(_tree, d_node_at(_tree, node->first_child)->text);

    for (uint32_t sibling = d_node_at(_tree, node->parent)->first_child;
         (sibling != D_DSS_NO_INDEX) && (sibling != _node);
         sibling = d_node_at(_tree, sibling)->next_sibling)
    {
        const struct d_node* const other = d_node_at(_tree, sibling);

        if ((!other->present) || (other->type != node->type) ||
            (other->first_child == D_DSS_NO_INDEX))
        {
            continue;
        }

        bool same_name = false;

        for (uint32_t at = 0; at < other->attribute_count; ++at)
        {
            const struct d_node_attribute* const attribute =
                d_node_attribute_at(_tree, other->first_attribute + at);

            if ((attribute) && (attribute->name == name_id) &&
                (attribute->value == my_name))
            {
                same_name = true;
            }
        }

        if ( (same_name) &&
             (strcmp(d_node_text(_tree,
                         d_node_at(_tree, other->first_child)->text),
                     my_value) == 0) )
        {
            return true;
        }
    }

    return false;
}


static bool
d_internal_meaning_holds_at(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node
)
{
    for (size_t at = 0; at < d_dss_rule_count(_sheet); ++at)
    {
        const struct d_dss_rule* const rule = d_dss_rule_at(_sheet, at);

        if (!d_match_rule(_sheet, _tree, _node, rule))
        {
            continue;
        }

        for (uint32_t which = 0; which < rule->declaration_count; ++which)
        {
            const struct d_dss_declaration* const declaration =
                d_dss_declaration_at(_sheet, rule->first_declaration + which);

            const char* const property = d_dss_text(_sheet,
                                                    declaration->property);

            if ( (strcmp(property, "content") != 0) &&
                 (strcmp(property, "format")  != 0) )
            {
                continue;
            }

            bool evaluated = false;

            const bool held = d_property_evaluate(_sheet, _tree, _node,
                                                  declaration, &evaluated);

            if ((evaluated) && (!held))
            {
                return false;
            }
        }
    }

    return true;
}


/*
d_internal_meaning_holds
  A node's meaning holds when its own content and format declarations hold
and so do those of every node beneath it.  Anchoring a unit moves its parts,
so the unit is only as sound as the least sound of them.
*/
static bool
d_internal_meaning_holds(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node
)
{
    if (!d_internal_meaning_holds_at(_sheet, _tree, _node))
    {
        return false;
    }

    for (uint32_t child = d_node_at(_tree, _node)->first_child;
         child != D_DSS_NO_INDEX;
         child = d_node_at(_tree, child)->next_sibling)
    {
        if (!d_internal_meaning_holds(_sheet, _tree, child))
        {
            return false;
        }
    }

    return true;
}



// d_check_severity
//   enum: what a failed declaration costs.  `off` removes it from the run
// entirely; `advise` reports and never repairs; `warn` and `error` report and
// repair, and only an `error` left standing fails the run.
enum d_check_severity
{
    D_CHECK_SEVERITY_OFF = 0,
    D_CHECK_SEVERITY_ADVISE,
    D_CHECK_SEVERITY_WARN,
    D_CHECK_SEVERITY_ERROR
};


/*
d_internal_level
  Maps a severity keyword to its level, or returns false for a word that is
not one.
*/
static bool
d_internal_level(
    const char*            _word,
    enum d_check_severity* _out
)
{
    static const struct
    {
        const char*           word;
        enum d_check_severity level;
    }
    levels[] =
    {
        { "off",    D_CHECK_SEVERITY_OFF    },
        { "advise", D_CHECK_SEVERITY_ADVISE },
        { "warn",   D_CHECK_SEVERITY_WARN   },
        { "error",  D_CHECK_SEVERITY_ERROR  }
    };

    for (size_t at = 0; at < 4u; ++at)
    {
        if (strcmp(_word, levels[at].word) == 0)
        {
            *_out = levels[at].level;
            return true;
        }
    }

    return false;
}


/*
d_internal_severity
  Resolves the severity of one property on one node: the first matching rule
whose `severity` declaration covers the property wins.  `severity: advise;`
covers every property; `severity: advise required;` covers only `required`.
Scoping matters because severity belongs to a claim, not to a node -- a
missing `revised:` line is a backfill question, while the geometry of a
`revised:` line that exists is not.
*/
static enum d_check_severity
d_internal_severity(
    const struct d_dss_sheet* _sheet,
    const struct d_cascade*   _cascade,
    uint32_t                  _node,
    const char*               _property,
    bool                      _honour,
    enum d_check_severity     _fallback
)
{
    // severity.mode=ignore: declarations say nothing; everything is the
    // fallback.  That includes carve-outs -- turning severity off turns off
    // the exemptions built from it, which is what "off" has to mean.
    if (!_honour)
    {
        return _fallback;
    }

    size_t                best       = SIZE_MAX;
    enum d_check_severity best_level = _fallback;
    const uint32_t*       ranked     = NULL;
    const size_t          matched    = d_cascade_matched(_cascade, _node,
                                                         &ranked);

    // the node's matches arrive best first, so scanning stops at the first
    // covering declaration; `best` keeps the old outranks test for ties
    for (size_t which_match = 0; which_match < matched; ++which_match)
    {
        const size_t at = ranked[which_match];

        if (best != SIZE_MAX)
        {
            break;
        }

        const struct d_dss_rule* const rule = d_dss_rule_at(_sheet, at);

        for (uint32_t which = 0; which < rule->declaration_count; ++which)
        {
            const struct d_dss_declaration* const declaration =
                d_dss_declaration_at(_sheet, rule->first_declaration + which);

            if (strcmp(d_dss_text(_sheet, declaration->property),
                       "severity") != 0)
            {
                continue;
            }

            const struct d_dss_value* const level =
                d_dss_value_at(_sheet, declaration->first_value);

            enum d_check_severity result = D_CHECK_SEVERITY_ERROR;

            if ((!level) ||
                (!d_internal_level(d_dss_text(_sheet, level->text), &result)))
            {
                continue;
            }

            // no list: every property; a list: only the ones it names
            bool covers = (declaration->value_count == 1u);

            for (uint32_t term = 1; term < declaration->value_count; ++term)
            {
                const struct d_dss_value* const named =
                    d_dss_value_at(_sheet, declaration->first_value + term);

                if ( (named) && (named->kind == D_DSS_VALUE_IDENT) &&
                     (strcmp(d_dss_text(_sheet, named->text),
                             _property) == 0) )
                {
                    covers = true;
                }
            }

            // the best-ranked covering declaration wins, as for any property
            if ( (covers) &&
                 ((best == SIZE_MAX) || (d_dss_outranks(_sheet, at, best))) )
            {
                best       = at;
                best_level = result;
            }
        }
    }

    return best_level;
}


/*
d_internal_insertion
  Builds the line that creates an absent node, when the node says where it
goes and the sheet can derive what it holds.  `required` is a human's job
only when the missing thing's content cannot be derived; an #endif comment
is `// <guard name>`, and nothing about that needs asking.
*/
static bool
d_internal_insertion(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    const bool*               _hit,
    size_t                    _node_count,
    uint32_t                  _node,
    const char*               _line,
    char*                     _out,
    size_t                    _out_size,
    bool*                     _out_whole_line
)
{
    const struct d_node* const node = d_node_at(_tree, _node);

    *_out_whole_line = false;

    if ((!node) || (node->present))
    {
        return false;
    }

    // a node marked `insert: line` is a line the file does not have yet
    const uint32_t insert_id = d_node_find_id(_tree, "insert");

    for (uint32_t at = 0; at < node->attribute_count; ++at)
    {
        const struct d_node_attribute* const attribute =
            d_node_attribute_at(_tree, node->first_attribute + at);

        if ((attribute) && (attribute->name == insert_id))
        {
            *_out_whole_line = true;
        }
    }

    if ((!*_out_whole_line) && (node->start_column == 0))
    {
        return false;
    }

    const char* syntax = NULL;

    const uint32_t syntax_id = d_node_find_id(_tree, "syntax");

    for (uint32_t at = 0; at < node->attribute_count; ++at)
    {
        const struct d_node_attribute* const attribute =
            d_node_attribute_at(_tree, node->first_attribute + at);

        if ((attribute) && (attribute->name == syntax_id))
        {
            syntax = d_node_text(_tree, attribute->value);
        }
    }

    if (!syntax)
    {
        return false;
    }

    // The content of a whole line is stated about the value it will hold, so
    // the lookup runs on the node's value when it has one.
    const struct d_node* const holder = d_node_at(_tree, _node);

    const uint32_t subject = ((*_out_whole_line) &&
                              (holder->first_child != D_DSS_NO_INDEX))
                           ? holder->first_child : _node;

    // the node's content, by the best-ranked rule that declares one
    const struct d_dss_declaration* content = NULL;
    size_t                          owner   = 0;

    for (size_t at = 0; at < d_dss_rule_count(_sheet); ++at)
    {
        if ( (!_hit[(at * _node_count) + subject]) ||
             ((content) && (!d_dss_outranks(_sheet, at, owner))) )
        {
            continue;
        }

        const struct d_dss_rule* const rule = d_dss_rule_at(_sheet, at);

        for (uint32_t which = 0; which < rule->declaration_count; ++which)
        {
            const struct d_dss_declaration* const declaration =
                d_dss_declaration_at(_sheet, rule->first_declaration + which);

            if (strcmp(d_dss_text(_sheet, declaration->property),
                       "content") == 0)
            {
                content = declaration;
                owner   = at;
                break;
            }
        }
    }

    char derived[D_BANNER_LINE_MAX];

    if ((!content) ||
        (!d_property_derive(_sheet, _tree, subject, content, derived,
                            sizeof(derived))))
    {
        return false;
    }

    // a whole line is built from nothing; a run is spliced into its line
    if (*_out_whole_line)
    {
        if ((strlen(syntax) + strlen(derived) + 1u) > _out_size)
        {
            return false;
        }

        (void)snprintf(_out, _out_size, "%s%s", syntax, derived);

        return true;
    }

    const size_t head = node->start_column - 1u;

    if ( (head > strlen(_line)) ||
         ((head + strlen(syntax) + strlen(derived) + 1u) > _out_size) )
    {
        return false;
    }

    // Everything after the insertion point must be blank -- unless the
    // extension marked it as a replaceable block comment, which it verified
    // holds one identifier and nothing else.  Anything else is not ours.
    bool replaces = false;

    const uint32_t replaces_id = d_node_find_id(_tree, "replaces");

    for (uint32_t at = 0; at < node->attribute_count; ++at)
    {
        const struct d_node_attribute* const attribute =
            d_node_attribute_at(_tree, node->first_attribute + at);

        if ((attribute) && (attribute->name == replaces_id))
        {
            replaces = true;
        }
    }

    for (size_t at = head; (!replaces) && (_line[at] != '\0'); ++at)
    {
        if ((_line[at] != ' ') && (_line[at] != '\t'))
        {
            return false;
        }
    }

    (void)memcpy(_out, _line, head);
    (void)memcpy(_out + head, syntax, strlen(syntax));
    (void)memcpy(_out + head + strlen(syntax), derived, strlen(derived) + 1u);

    return true;
}





/*
d_internal_refuse
  Records a repair the audit would not let through, naming the first few.
*/
static void
d_internal_refuse(
    size_t*     _refused,
    size_t      _shown,
    const char* _path,
    uint32_t    _line,
    const char* _property,
    const char* _reason
)
{
    if (*_refused < _shown)
    {
        (void)fprintf(stderr, "%s:%u: %s repair refused: %s\n", _path, _line,
                      _property, _reason ? _reason : "unstated");
    }

    ++(*_refused);

    return;
}


/*
d_internal_claim_for
  States what an ordinary repair claims to have changed, from the node it
targeted and the property it served.  The claim is built from the node's
recorded geometry, which is exactly what goes stale -- so checking it against
the line on disk is what exposes a stale repair.
*/
static void
d_internal_claim_for(
    struct d_node_tree*   _tree,
    uint32_t              _node,
    const char*           _property,
    const char*           _derived,
    struct d_audit_claim* _out
)
{
    const struct d_node* const node = d_node_at(_tree, _node);

    memset(_out, 0, sizeof(*_out));

    const char* const text = d_node_text(_tree, node->text);

    _out->head   = (node->start_column > 0u) ? (node->start_column - 1u) : 0u;
    _out->run    = node->width;
    _out->expect = (text[0] != '\0') ? text : NULL;

    if ( (strcmp(_property, "content") == 0) ||
         (strcmp(_property, "format")  == 0) )
    {
        _out->kind  = D_AUDIT_SUBSTITUTE;
        _out->value = _derived;
    }
    else if (strcmp(_property, "width") == 0)
    {
        _out->kind = D_AUDIT_FILL;
    }
    else
    {
        _out->kind = D_AUDIT_GEOMETRY;
    }

    return;
}



// d_check_settings
//   the settings this driver registers.  Each is read somewhere below; a
// setting nothing reads is not registered.
static const char* const d_internal_resolve_choices[] = { "first", "all",
                                                          "layers", NULL };
static const char* const d_internal_mode_choices[]    = { "honour", "ignore",
                                                          NULL };
static const char* const d_internal_level_choices[]   = { "off", "advise",
                                                          "warn", "error",
                                                          NULL };
static const char* const d_internal_files_choices[]   = { "headers", "all",
                                                          NULL };

static const struct d_setting_def d_check_settings[] =
{
    { "fix", D_SETTING_BOOL, "false", NULL, NULL, 0, 0, NULL,
      "apply repairs, rather than only report what fails" },
    { "cascade.resolve", D_SETTING_ENUM, "first",
      d_internal_resolve_choices, NULL, 0, 0, "resolve",
      "which declaration of a property wins: the first matching rule, every "
      "one, or the highest cascade layer (@layer, @import ... layer())" },
    { "severity.mode", D_SETTING_ENUM, "honour", d_internal_mode_choices,
      NULL, 0, 0, NULL,
      "honour severity declarations, or ignore them and use severity.default" },
    { "severity.default", D_SETTING_ENUM, "error", d_internal_level_choices,
      NULL, 0, 0, NULL,
      "severity of a declaration that no severity rule covers" },
    { "report.refusals", D_SETTING_INT, "8", NULL, NULL, 0, 100000, NULL,
      "refused repairs named one by one before the report only counts them" },
    { "report.findings", D_SETTING_INT, "0", NULL, NULL, 0, 10000000, NULL,
      "failed declarations listed one per line, as path:line:column: "
      "severity: message, before the summary; 0 lists none" },
    { "source.tree", D_SETTING_BOOL, "false", NULL, NULL, 0, 0, NULL,
      "build the C or C++ token, group and item tree beneath each file "
      "(dext_c, dext_cpp), for sheets that select source structure" },
    { "source.lines", D_SETTING_BOOL, "false", NULL, NULL, 0, 0, NULL,
      "build the text layer beneath each file (dline): a `text` node and a "
      "`line` node per physical line, for sheets that select line facts" },
    { "source.files", D_SETTING_ENUM, "headers", d_internal_files_choices,
      NULL, 0, 0, NULL,
      "which files are checked: headers only (.h, .hpp), or every C and C++ "
      "file, sources included; a guard is built for headers either way" }
};





// D_CHECK_IMPORT_DEPTH_MAX
//   constant: how deeply sheets may import one another.
#define D_CHECK_IMPORT_DEPTH_MAX 16u

// d_internal_segment
//   struct: a run of consecutive combined lines copied from one place in one
// file.  The provenance table is a list of these, so any line of the
// combined sheet can be traced back to the file and line it came from.
struct d_internal_segment
{
    uint32_t     combined_first;
    uint32_t     count;
    uint32_t     source_first;
    const char*  file;
    const char*  layer;       // NULL: unlayered
};

// d_internal_expansion
//   struct: the combined sheet being built, and where every line came from.
struct d_internal_expansion
{
    char*                       text;
    size_t                      used;
    size_t                      capacity;
    uint32_t                    lines;

    struct d_internal_segment*  segments;
    size_t                      segment_count;
    size_t                      segment_capacity;

    char*                       owned[256];     // paths and layer names
    size_t                      owned_count;

    const char*                 stack[D_CHECK_IMPORT_DEPTH_MAX + 1u];
    size_t                      depth;

    char                        error[512];
};


static const char*
d_internal_own(
    struct d_internal_expansion* _x,
    const char*                  _text,
    size_t                       _length
)
{
    if (_x->owned_count == (sizeof(_x->owned) / sizeof(_x->owned[0])))
    {
        return NULL;
    }

    char* const copy = malloc(_length + 1u);

    if (!copy)
    {
        return NULL;
    }

    (void)memcpy(copy, _text, _length);

    copy[_length] = '\0';

    _x->owned[_x->owned_count++] = copy;

    return copy;
}


/*
d_internal_append_line
  Appends one line to the combined sheet and records where it came from,
extending the last segment when the line continues it.
*/
static bool
d_internal_append_line(
    struct d_internal_expansion* _x,
    const char*                  _line,
    size_t                       _length,
    const char*                  _file,
    uint32_t                     _source_line,
    const char*                  _layer
)
{
    if ((_x->used + _length + 2u) > _x->capacity)
    {
        size_t grown = (_x->capacity == 0) ? 8192u : _x->capacity;

        while (grown < (_x->used + _length + 2u))
        {
            grown *= 2u;
        }

        char* const text = realloc(_x->text, grown);

        if (!text)
        {
            return false;
        }

        _x->text     = text;
        _x->capacity = grown;
    }

    (void)memcpy(_x->text + _x->used, _line, _length);

    _x->used              += _length;
    _x->text[_x->used++]   = '\n';
    _x->text[_x->used]     = '\0';

    ++_x->lines;

    struct d_internal_segment* const last =
        (_x->segment_count > 0) ? &_x->segments[_x->segment_count - 1u]
                                : NULL;

    if ( (last) && (last->file == _file) && (last->layer == _layer) &&
         ((last->source_first + last->count) == _source_line) &&
         ((last->combined_first + last->count) == _x->lines) )
    {
        ++last->count;
        return true;
    }

    if (_x->segment_count == _x->segment_capacity)
    {
        const size_t grown = (_x->segment_capacity == 0)
                           ? 32u : (_x->segment_capacity * 2u);

        struct d_internal_segment* const segments =
            realloc(_x->segments, grown * sizeof(*segments));

        if (!segments)
        {
            return false;
        }

        _x->segments         = segments;
        _x->segment_capacity = grown;
    }

    struct d_internal_segment* const next = &_x->segments[_x->segment_count++];

    next->combined_first = _x->lines;
    next->count          = 1u;
    next->source_first   = _source_line;
    next->file           = _file;
    next->layer          = _layer;

    return true;
}


/*
d_internal_origin
  Maps a line of the combined sheet back to the file and line it came from.
*/
static void
d_internal_origin(
    const struct d_internal_expansion* _x,
    uint32_t                           _line,
    const char**                       _out_file,
    uint32_t*                          _out_line,
    const char**                       _out_layer
)
{
    *_out_file  = "?";
    *_out_line  = _line;
    *_out_layer = NULL;

    for (size_t at = 0; at < _x->segment_count; ++at)
    {
        const struct d_internal_segment* const segment = &_x->segments[at];

        if ((_line >= segment->combined_first) &&
            (_line < (segment->combined_first + segment->count)))
        {
            *_out_file  = segment->file;
            *_out_line  = segment->source_first
                        + (_line - segment->combined_first);
            *_out_layer = segment->layer;
            return;
        }
    }

    return;
}


/*
d_internal_expand
  Appends a sheet to the combined text, replacing each `@import "path"
[layer(name)];` line with the imported sheet's own expansion.  A path is
relative to the sheet that imports it.  An import without layer() stays in
the importing sheet's layer; the root sheet is unlayered, and so outranks
every layer, as in CSS.  Cycles are refused by real path, so two spellings of
one file are still one file.
*/
static bool
d_internal_expand(
    struct d_internal_expansion* _x,
    const char*                  _path,
    const char*                  _layer
)
{
    char resolved[PATH_MAX];

    if (!realpath(_path, resolved))
    {
        (void)snprintf(_x->error, sizeof(_x->error), "%s: cannot open",
                       _path);
        return false;
    }

    for (size_t at = 0; at < _x->depth; ++at)
    {
        if (strcmp(_x->stack[at], resolved) == 0)
        {
            (void)snprintf(_x->error, sizeof(_x->error), "%s: imports itself "
                           "by way of %s", _path, _x->stack[_x->depth - 1u]);
            return false;
        }
    }

    if (_x->depth == D_CHECK_IMPORT_DEPTH_MAX)
    {
        (void)snprintf(_x->error, sizeof(_x->error), "%s: imports nest more "
                       "than %u deep", _path, D_CHECK_IMPORT_DEPTH_MAX);
        return false;
    }

    const char* const file = d_internal_own(_x, _path, strlen(_path));
    const char* const real = d_internal_own(_x, resolved, strlen(resolved));

    FILE* const handle = fopen(resolved, "rb");

    if ((!file) || (!real) || (!handle))
    {
        if (handle)
        {
            (void)fclose(handle);
        }

        (void)snprintf(_x->error, sizeof(_x->error), "%s: cannot open",
                       _path);
        return false;
    }

    (void)fseek(handle, 0, SEEK_END);

    const long size = ftell(handle);

    (void)fseek(handle, 0, SEEK_SET);

    char* const source = malloc((size_t)size + 1u);

    if (!source)
    {
        (void)fclose(handle);
        return false;
    }

    const size_t read = fread(source, 1u, (size_t)size, handle);

    source[read] = '\0';

    (void)fclose(handle);

    // parse this sheet alone, to find its imports and to fail early and
    // precisely if it does not parse
    struct d_dss_error        error;
    struct d_dss_sheet* const sheet = d_dss_parse(source, read, &error);

    if (!sheet)
    {
        (void)snprintf(_x->error, sizeof(_x->error), "%s:%u:%u: %s", _path,
                       error.line, error.column,
                       error.message ? error.message : "parse failed");
        free(source);
        return false;
    }

    _x->stack[_x->depth++] = real;

    // the directory an import's path is relative to
    const char* const slash = strrchr(_path, '/');
    const size_t      base  = slash ? (size_t)(slash - _path) + 1u : 0u;

    bool         ok   = true;
    uint32_t     line = 1;
    const char*  at   = source;

    while ((ok) && (*at))
    {
        const char* end = at;

        while ((*end) && (*end != '\n'))
        {
            ++end;
        }

        size_t length = (size_t)(end - at);

        if ((length > 0) && (at[length - 1u] == '\r'))
        {
            --length;
        }

        // is this line an @import?
        const struct d_dss_at_rule* import = NULL;

        for (size_t which = 0; which < d_dss_at_rule_count(sheet); ++which)
        {
            const struct d_dss_at_rule* const candidate =
                d_dss_at_rule_at(sheet, which);

            if ( (candidate->line == line) &&
                 (strcmp(d_dss_text(sheet, candidate->name), "import") == 0) )
            {
                import = candidate;
            }
        }

        if (!import)
        {
            ok = d_internal_append_line(_x, at, length, file, line, _layer);
        }
        else
        {
            const char* const prelude = (import->prelude != D_DSS_NO_INDEX)
                                      ? d_dss_text(sheet, import->prelude)
                                      : "";
            const char* const open    = strchr(prelude, '"');
            const char* const close   = open ? strchr(open + 1, '"') : NULL;

            if (!close)
            {
                (void)snprintf(_x->error, sizeof(_x->error), "%s:%u: @import "
                               "needs a quoted path", _path, line);
                ok = false;
            }
            else
            {
                char target[PATH_MAX];

                (void)snprintf(target, sizeof(target), "%.*s%.*s", (int)base,
                               _path, (int)(close - open - 1), open + 1);

                const char* layer = _layer;
                const char* named = strstr(close, "layer(");

                if (named)
                {
                    const char* const stop = strchr(named, ')');

                    layer = stop ? d_internal_own(_x, named + 6,
                                                  (size_t)(stop - named - 6))
                                 : NULL;

                    if ((!stop) || (!layer) || (layer[0] == '\0'))
                    {
                        (void)snprintf(_x->error, sizeof(_x->error),
                                       "%s:%u: layer() needs a name", _path,
                                       line);
                        ok = false;
                    }
                }

                if (ok)
                {
                    ok = d_internal_expand(_x, target, layer);

                    // name the import site once, at the innermost failure
                    if ((!ok) && (!strstr(_x->error, " (imported at ")))
                    {
                        const size_t used = strlen(_x->error);

                        (void)snprintf(_x->error + used,
                                       sizeof(_x->error) - used,
                                       " (imported at %s:%u)", _path, line);
                    }
                }
            }
        }

        at = (*end) ? (end + 1) : end;
        ++line;
    }

    --_x->depth;

    d_dss_free(sheet);
    free(source);

    return ok;
}


/*
d_internal_expansion_free
*/
static void
d_internal_expansion_free(
    struct d_internal_expansion* _x
)
{
    free(_x->text);
    free(_x->segments);

    for (size_t at = 0; at < _x->owned_count; ++at)
    {
        free(_x->owned[at]);
    }

    return;
}


/*
d_internal_assign_layers
  Gives every rule its layer rank.  Layer order is the order names first
appear -- in an `@layer a, b;` statement or an import's layer() -- and later
layers outrank earlier ones.  Unlayered rules outrank every layer, as in CSS.
Only called when layers are in force: otherwise every rank stays zero and
ranking reduces to first match.
*/
static void
d_internal_assign_layers(
    struct d_dss_sheet*                _sheet,
    const struct d_internal_expansion* _x
)
{
    // layer names in precedence order, each copied out whole: a pointer into
    // the middle of a prelude does not know where its name ends
    char   order[128][64];
    size_t named = 0;

    #define D_INTERNAL_LAYER_INDEX(name, length, out)                         \
        do                                                                    \
        {                                                                     \
            (out) = named;                                                    \
            for (size_t k_ = 0; k_ < named; ++k_)                             \
            {                                                                 \
                if ((strlen(order[k_]) == (length)) &&                        \
                    (strncmp(order[k_], (name), (length)) == 0))              \
                {                                                             \
                    (out) = k_;                                               \
                    break;                                                    \
                }                                                             \
            }                                                                 \
            if (((out) == named) && (named < 128u) && ((length) < 64u))       \
            {                                                                 \
                (void)memcpy(order[named], (name), (length));                 \
                order[named][(length)] = '\0';                                \
                ++named;                                                      \
            }                                                                 \
        } while (0)

    // declared order first: every @layer statement, in sheet order
    for (size_t at = 0; at < d_dss_at_rule_count(_sheet); ++at)
    {
        const struct d_dss_at_rule* const rule = d_dss_at_rule_at(_sheet, at);

        if ( (strcmp(d_dss_text(_sheet, rule->name), "layer") != 0) ||
             (rule->prelude == D_DSS_NO_INDEX) )
        {
            continue;
        }

        const char* scan = d_dss_text(_sheet, rule->prelude);

        while (*scan)
        {
            while ((*scan == ' ') || (*scan == ','))
            {
                ++scan;
            }

            const char* const start = scan;

            while ((*scan) && (*scan != ',') && (*scan != ' '))
            {
                ++scan;
            }

            if (scan == start)
            {
                break;
            }

            size_t unused = 0;

            D_INTERNAL_LAYER_INDEX(start, (size_t)(scan - start), unused);
            (void)unused;
        }
    }

    for (size_t at = 0; at < d_dss_rule_count(_sheet); ++at)
    {
        const char* file  = NULL;
        uint32_t    line  = 0;
        const char* layer = NULL;

        d_internal_origin(_x, d_dss_rule_at(_sheet, at)->line, &file, &line,
                          &layer);

        // unlayered outranks every layer, as in CSS; a layer named only by
        // an import joins the order where it first appears
        uint32_t rank = UINT32_MAX;

        if (layer)
        {
            size_t index = 0;

            D_INTERNAL_LAYER_INDEX(layer, strlen(layer), index);

            rank = (uint32_t)index + 1u;
        }

        (void)d_dss_set_rule_layer(_sheet, at, rank);
    }

    #undef D_INTERNAL_LAYER_INDEX

    return;
}


/*
main
  Walks a tree, builds a banner per header, runs the sheet, and prints one
row per declaration.  First match wins is not yet exercised because no two
rules in the sheet carry the same property for the same node; when they do,
this is the loop that has to stop at the first.
*/
int
main(
    int    argc,
    char** argv
)
{
    struct d_settings* const settings = d_settings_new();

    if ( (!settings) ||
         (!d_settings_register(settings, d_check_settings,
                               sizeof(d_check_settings)
                               / sizeof(d_check_settings[0]))) )
    {
        (void)fprintf(stderr, "dcheck: cannot build the settings registry\n");
        return 2;
    }

    char problem[256];

    // the environment, then the command line; standing decides, not order
    if (!d_settings_apply(settings, getenv("DAWK_SETTINGS"),
                          D_SETTING_ENVIRONMENT, problem, sizeof(problem)))
    {
        (void)fprintf(stderr, "dcheck: DAWK_SETTINGS: %s\n", problem);
        return 2;
    }

    const char* positional[2]    = { NULL, NULL };
    int         positional_count = 0;
    bool        listing          = false;

    for (int at = 1; at < argc; ++at)
    {
        const char* const argument = argv[at];

        if (strcmp(argument, "--settings") == 0)
        {
            listing = true;
            continue;
        }

        if (strncmp(argument, "--", 2u) != 0)
        {
            if (positional_count == 2)
            {
                (void)fprintf(stderr, "dcheck: unexpected '%s'\n", argument);
                return 2;
            }

            positional[positional_count++] = argument;
            continue;
        }

        // the same parser dawk uses, so the two commands cannot drift apart
        const int taken = d_settings_take_option(settings, argc, argv, &at,
                                                 problem, sizeof(problem));

        if (taken < 0)
        {
            (void)fprintf(stderr, "dcheck: %s\n", problem);
            return 2;
        }

        if (taken == 0)
        {
            (void)fprintf(stderr, "dcheck: unexpected '%s'\n", argument);
            return 2;
        }
    }

    if ((listing) && (positional_count == 0))
    {
        d_settings_describe(settings, stdout);
        d_settings_free(settings);
        return 0;
    }

    if (positional_count != 2)
    {
        (void)fprintf(stderr, "usage: dcheck [--settings] [--name=value ...] "
                      "<sheet.dss> <root>\n");
        return 2;
    }

    const char* const sheet_path = positional[0];
    const char* const root_path  = positional[1];

    size_t repairs = 0;
    size_t refused = 0;


    // The root sheet and everything it imports, as one combined sheet.
    // Each line of it can be traced back to the file it came from.
    struct d_internal_expansion expansion;

    memset(&expansion, 0, sizeof(expansion));

    if (!d_internal_expand(&expansion, sheet_path, NULL))
    {
        (void)fprintf(stderr, "dcheck: %s\n", expansion.error);
        d_internal_expansion_free(&expansion);
        return 2;
    }

    char* const  source = expansion.text;
    const size_t read   = expansion.used;

    // "file:line" for a line of the combined sheet
    char where[PATH_MAX + 32];

    #define D_INTERNAL_WHERE(line_)                                           \
        do                                                                    \
        {                                                                     \
            const char* file_  = NULL;                                        \
            uint32_t    at_    = 0;                                           \
            const char* layer_ = NULL;                                        \
            d_internal_origin(&expansion, (line_), &file_, &at_, &layer_);    \
            (void)layer_;                                                     \
            (void)snprintf(where, sizeof(where), "%s:%u", file_, at_);       \
        } while (0)

    struct d_dss_error  error;
    struct d_dss_sheet* const sheet = d_dss_parse(source, read, &error);

    if (!sheet)
    {
        D_INTERNAL_WHERE(error.line);
        (void)fprintf(stderr, "%s:%u: %s\n", where, error.column,
                      error.message ? error.message : "parse failed");
        d_internal_expansion_free(&expansion);
        return 1;
    }

    // A sheet states the settings it was written for with `@set name value;`
    // (and `@resolve mode;`, the older spelling).  They stand above defaults
    // and below the environment and the command line, and whatever is in
    // force is printed -- a sheet read under the wrong settings means
    // something different and would otherwise say nothing.
    for (size_t at = 0; at < d_dss_at_rule_count(sheet); ++at)
    {
        const struct d_dss_at_rule* const at_rule =
            d_dss_at_rule_at(sheet, at);

        const char* const kind = d_dss_text(sheet, at_rule->name);

        if ( ((strcmp(kind, "set") != 0) && (strcmp(kind, "resolve") != 0)) ||
             (at_rule->prelude == D_DSS_NO_INDEX) )
        {
            continue;
        }

        char        line[256];
        const char* name  = "cascade.resolve";
        const char* value = d_dss_text(sheet, at_rule->prelude);

        if (strcmp(kind, "set") == 0)
        {
            (void)snprintf(line, sizeof(line), "%s", value);

            char* const space = strchr(line, ' ');

            if (!space)
            {
                D_INTERNAL_WHERE(at_rule->line);
                (void)fprintf(stderr, "%s: @set needs a name and a value\n",
                              where);
                return 2;
            }

            *space = '\0';
            name   = line;
            value  = space + 1;

            while (*value == ' ')
            {
                ++value;
            }
        }

        if (!d_settings_set(settings, name, value, D_SETTING_SHEET, problem,
                            sizeof(problem)))
        {
            D_INTERNAL_WHERE(at_rule->line);
            (void)fprintf(stderr, "%s: %s\n", where, problem);
            return 2;
        }
    }

    if (listing)
    {
        d_settings_describe(settings, stdout);
        d_settings_free(settings);
        return 0;
    }

    const bool        fixing      = d_settings_bool(settings, "fix");
    const bool        sourcing    = d_settings_bool(settings, "source.tree");
    const bool        lining      = d_settings_bool(settings, "source.lines");
    const size_t      list_limit  =
        (size_t)d_settings_int(settings, "report.findings");
    size_t            listed      = 0u;
    const bool        every_file  = d_settings_is(settings,
                                                  "source.files", "all");
    const char* const resolve     = d_settings_text(settings,
                                                    "cascade.resolve");
    // `first` and `layers` both pick one winning declaration per property;
    // they differ only in how rules are ranked, which the layers carry
    const bool        first_match = !d_settings_is(settings,
                                                   "cascade.resolve", "all");
    const bool        honour      = d_settings_is(settings, "severity.mode",
                                                  "honour");
    const size_t      shown       = (size_t)d_settings_int(settings,
                                                           "report.refusals");

    // ranks are given only when layers are in force; otherwise every rule
    // stays at zero and ranking is plain first match
    if (d_settings_is(settings, "cascade.resolve", "layers"))
    {
        d_internal_assign_layers(sheet, &expansion);
    }

    enum d_check_severity fallback = D_CHECK_SEVERITY_ERROR;

    (void)d_internal_level(d_settings_text(settings, "severity.default"),
                           &fallback);

    uint32_t    bad_line = 0;
    const char* bad_name = NULL;

    if (d_match_unsupported(sheet, &bad_line, &bad_name))
    {
        D_INTERNAL_WHERE(bad_line);
        (void)fprintf(stderr, "%s: selector '%s' is not supported and "
                      "would match nothing\n", where, bad_name);
        d_dss_free(sheet);
        d_internal_expansion_free(&expansion);
        return 2;
    }

    // every property must be one the engine registered, before any file
    {
        uint32_t    unknown_line = 0u;
        const char* unknown_name = NULL;
        const char* unknown_why  = NULL;

        if (!d_schema_validate(d_property_schema(), sheet, &unknown_line,
                               &unknown_name, &unknown_why))
        {
            D_INTERNAL_WHERE(unknown_line);
            (void)fprintf(stderr, "%s: property '%s' is refused: %s\n",
                          where, unknown_name, unknown_why);
            d_dss_free(sheet);
            d_internal_expansion_free(&expansion);

            return 2;
        }
    }

    // every severity level must be a word the engine knows
    for (size_t at = 0; at < d_dss_rule_count(sheet); ++at)
    {
        const struct d_dss_rule* const rule = d_dss_rule_at(sheet, at);

        for (uint32_t which = 0; which < rule->declaration_count; ++which)
        {
            const struct d_dss_declaration* const declaration =
                d_dss_declaration_at(sheet, rule->first_declaration + which);

            enum d_check_severity unused;

            if ( (strcmp(d_dss_text(sheet, declaration->property),
                         "severity") == 0) &&
                 (!d_internal_level(d_dss_text(sheet,
                      d_dss_value_at(sheet, declaration->first_value)->text),
                                    &unused)) )
            {
                D_INTERNAL_WHERE(declaration->line);
                (void)fprintf(stderr, "%s: severity must be off, advise, "
                              "warn or error\n", where);
                d_dss_free(sheet);
                d_internal_expansion_free(&expansion);
                return 2;
            }
        }
    }

    struct d_awk_source walk;

    if (!d_awk_source_tree_init(&walk, root_path))
    {
        (void)fprintf(stderr, "dcheck: cannot walk %s\n", root_path);
        d_dss_free(sheet);
        d_internal_expansion_free(&expansion);
        return 2;
    }

    const size_t rule_count = d_dss_rule_count(sheet);

    struct d_check_tally* const tally =
        calloc(d_dss_rule_count(sheet) * 8u + 8u,
               sizeof(struct d_check_tally));

    if (!tally)
    {
        d_dss_free(sheet);
        d_internal_expansion_free(&expansion);
        return 2;
    }

    size_t tally_count = 0;

    for (size_t at = 0; at < rule_count; ++at)
    {
        const struct d_dss_rule* const rule = d_dss_rule_at(sheet, at);

        for (uint32_t which = 0; which < rule->declaration_count; ++which)
        {
            const struct d_dss_declaration* const declaration =
                d_dss_declaration_at(sheet,
                                     rule->first_declaration + which);

            tally[tally_count].property = d_dss_text(sheet,
                                                     declaration->property);
            tally[tally_count].line     = declaration->line;

            ++tally_count;
        }
    }

    // history for the whole tree, read once
    struct d_git_history* const history = d_git_read(root_path);

    // one symbol table for the sheet and the tree: names compare as integers
    struct d_symbol_table* const symbols = d_symbol_table_new();
    struct d_node_tree* const    tree    = symbols
                                           ? d_node_tree_new_shared(symbols)
                                           : NULL;
    const bool bound = ( (tree) && (d_dss_bind(sheet, symbols)) );
    struct d_match_index* const     index      = bound
                                                 ? d_match_index_new(sheet)
                                                 : NULL;
    struct d_cascade* const         cascade    =
        d_cascade_new(sheet, first_match ? D_CASCADE_FIRST : D_CASCADE_ALL);
    struct d_schema_binding* const  schema     =
        d_schema_bind(d_property_schema(), symbols);
    uint32_t* const                 candidates =
        malloc((d_dss_rule_count(sheet) + 1u) * sizeof(uint32_t));

    if ( (!index) || (!cascade) || (!schema) || (!candidates) )
    {
        (void)fprintf(stderr, "dcheck: out of memory\n");

        return 2;
    }

    size_t      files      = 0;
    size_t      errors     = 0;
    size_t      warnings   = 0;
    size_t      advisories = 0;
    size_t      banners  = 0;
    size_t      lexical  = 0;
    const char* record   = NULL;
    size_t      length   = 0;

    while (walk.next_record(walk.user, &record, &length))
    {
        const char* const dot = strrchr(record, '.');

        const bool header = ( (dot) &&
                              ( (strcmp(dot, ".h") == 0) ||
                                (strcmp(dot, ".hpp") == 0) ) );

        // headers, or under source.files=all every C and C++ file
        if ( (!header) &&
             ( (!every_file) || (!d_ext_source_accepts(record)) ) )
        {
            continue;
        }

        ++files;

        d_node_tree_clear(tree);

        const uint32_t banner = d_banner_build(tree, record,
                                               root_path);

        uint32_t file_node = D_DSS_NO_INDEX;

        // A file with no banner is still a file: the count below reports the
        // missing banner, and every other rule still reads the rest of it.
        // Skipping it here once hid every other finding in such a file.
        if (banner == D_DSS_NO_INDEX)
        {
            file_node = d_banner_file(tree, record, root_path);

            if (file_node == D_DSS_NO_INDEX)
            {
                continue;
            }
        }
        else
        {
            ++banners;

            // the second extension contributes into the same tree, beneath
            // the same file node; neither knows the other is there
            struct d_node* const banner_node = d_node_at(tree, banner);

            file_node = banner_node ? banner_node->parent : D_DSS_NO_INDEX;
        }

        // A node's attributes must be contiguous, so every fact about the
        // file is attached before any other extension adds attributes of its
        // own -- otherwise the file's range runs into theirs.
        // the file's path as the tree knows it, which is what git prints
        const char* relative = record;

        if (strncmp(record, root_path, strlen(root_path)) == 0)
        {
            relative = record + strlen(root_path);

            while (*relative == '/')
            {
                ++relative;
            }
        }

        (void)d_git_build(history, tree, file_node, relative);

        // an include guard is a header's; a source has none to model
        if (header)
        {
            (void)d_guard_build(tree, file_node, record);
        }


        (void)d_include_build(tree, file_node, record);
        (void)d_section_build(tree, file_node, record);

        // the C or C++ source tree, when a sheet selects source structure
        if (sourcing)
        {
            struct d_token_node_stats built;

            (void)d_ext_source_build(tree, file_node, record, &built);
            lexical += built.lex_errors;
        }

        // the text layer, when a sheet selects line facts
        if (lining)
        {
            (void)d_line_build(tree, file_node, record);
        }

        size_t slot = 0;

        // every rule against every node, once; resolution and severity both
        // consult it, so nothing is matched twice
        const size_t node_count = d_node_count(tree);

        bool* const hit = calloc((rule_count * node_count) + 1u, sizeof(bool));

        if (!hit)
        {
            break;
        }

        // a node meets only the rules whose subject could name its type,
        // in ascending order; the cascade ranks each node's matches once
        (void)d_cascade_reset(cascade, node_count);

        for (uint32_t node = 0; node < (uint32_t)node_count; ++node)
        {
            const size_t candidate_count = d_match_index_candidates(
                index, d_node_at(tree, node)->type, candidates, rule_count);

            for (size_t which_rule = 0; which_rule < candidate_count;
                 ++which_rule)
            {
                const uint32_t at = candidates[which_rule];

                if (d_match_rule(sheet, tree, node, d_dss_rule_at(sheet, at)))
                {
                    hit[((size_t)at * node_count) + node] = true;
                    (void)d_cascade_add(cascade, node, at, 0u);
                }
            }
        }

        (void)d_cascade_rank(cascade);

        // A node's columns are measured once per pass, from the file as it
        // was read, so one line is never spliced twice in a pass.  Lines do
        // not share geometry, so repairs to different lines may proceed in
        // the same pass.  `repaired_this_file` marks a *structural* edit --
        // a deletion, which moves every later line, or a table, which
        // rewrites several -- and either ends the file's pass.
        bool     repaired_this_file = false;
        uint32_t touched[512];
        size_t   touched_count      = 0;

        for (size_t at = 0; at < rule_count; ++at)
        {
            const struct d_dss_rule* const rule = d_dss_rule_at(sheet, at);

            for (uint32_t node = 0;
                 node < (uint32_t)d_node_count(tree);
                 ++node)
            {
                if (!hit[(at * node_count) + node])
                {
                    continue;
                }

                for (uint32_t which = 0;
                     which < rule->declaration_count;
                     ++which)
                {
                    const struct d_dss_declaration* const declaration =
                        d_dss_declaration_at(sheet,
                                             rule->first_declaration + which);

                    const char* const claim =
                        d_dss_text(sheet, declaration->property);

                    // first match: a later rule's claim yields to an earlier
                    if ((first_match) &&
                        (!d_cascade_wins(cascade, node, (uint32_t)at,
                                        declaration->symbol)))
                    {
                        continue;
                    }

                    const enum d_check_severity severity =
                        d_internal_severity(sheet, cascade, node,
                                            claim, honour, fallback);

                    if (severity == D_CHECK_SEVERITY_OFF)
                    {
                        continue;
                    }

                    bool evaluated = false;

                    const bool held = d_schema_evaluate(schema, sheet, tree, node,
                                          declaration, &evaluated);

                    if (!evaluated)
                    {
                        continue;
                    }

                    ++tally[slot + which].matched;

                    if (!held)
                    {
                        ++tally[slot + which].failed;

                        if (severity == D_CHECK_SEVERITY_ERROR)
                        {
                            ++errors;
                        }
                        else if (severity == D_CHECK_SEVERITY_WARN)
                        {
                            ++warnings;
                        }
                        else
                        {
                            ++advisories;
                        }

                        // the finding itself, where it is and what it says
                        if (listed < list_limit)
                        {
                            const struct d_node* const failed_node =
                                d_node_at(tree, node);
                            const struct d_dss_value* const said =
                                d_dss_value_at(sheet,
                                               declaration->first_value);
                            const bool worded =
                                ( (strcmp(claim, "violation") == 0) &&
                                  (said)                             &&
                                  (said->text != D_DSS_NO_INDEX) );

                            ++listed;
                            (void)printf("%s:%u:%u: %s: %s\n",
                                         record,
                                         failed_node->line,
                                         failed_node->start_column,
                                         (severity == D_CHECK_SEVERITY_ERROR)
                                         ? "error"
                                         : (severity == D_CHECK_SEVERITY_WARN)
                                         ? "warn"
                                         : "advise",
                                         worded ? d_dss_text(sheet, said->text)
                                                : claim);
                        }

                        // an advisory is reported and never repaired
                        if ((!fixing) || (repaired_this_file) ||
                            (severity == D_CHECK_SEVERITY_ADVISE))
                        {
                            continue;
                        }

                        const bool structural =
                            (strcmp(d_dss_text(sheet, declaration->property),
                                    "display") == 0);

                        // structural edits run only in a pass that has made
                        // no line edits to this file, so neither can see the
                        // other's half-applied state
                        if ((structural) && (touched_count > 0))
                        {
                            continue;
                        }

                        // A table repair rewrites its rows together, as one
                        // repair.  It reads each row's own line, so it comes
                        // before the check below: a table has no line of its
                        // own, and must not be skipped for lacking one.
                        if (strcmp(d_dss_text(sheet, declaration->property),
                                   "display") == 0)
                        {
                            const struct d_dss_value* const display_value =
                                d_dss_value_at(sheet,
                                               declaration->first_value);

                            const bool none =
                                ( (display_value) &&
                                  (display_value->text != D_DSS_NO_INDEX) &&
                                  (strcmp(d_dss_text(sheet,
                                                     display_value->text),
                                          "none") == 0) );

                            // removal is for whole-line nodes whose content
                            // survives elsewhere, and for nothing else
                            if (none)
                            {
                                struct d_node* const gone =
                                    d_node_at(tree, node);

                                if ( (gone) && (gone->line != 0) &&
                                     (gone->start_column == 1u) &&
                                     (d_internal_redundant(tree, node)) &&
                                     (d_edit_delete_line(record,
                                                         gone->line)) )
                                {
                                    ++repairs;
                                    repaired_this_file = true;
                                    break;
                                }

                                continue;
                            }

                            const char* table_refusal = NULL;

                            if (d_layout_table_repair(sheet, tree, node,
                                                      record,
                                                      &table_refusal))
                            {
                                ++repairs;
                                repaired_this_file = true;
                                break;
                            }

                            if (table_refusal)
                            {
                                d_internal_refuse(&refused, shown, record, 0u,
                                                  "display: table",
                                                  table_refusal);
                            }

                            continue;
                        }

                        struct d_node* const target = d_node_at(tree, node);

                        if ((!target) || (target->line == 0))
                        {
                            continue;
                        }

                        // a line already rewritten this pass is measured
                        // again next pass, never spliced twice in one
                        bool line_touched = (touched_count
                                             >= (sizeof(touched)
                                                 / sizeof(touched[0])));

                        for (size_t which_line = 0;
                             (!line_touched) && (which_line < touched_count);
                             ++which_line)
                        {
                            line_touched = (touched[which_line]
                                            == target->line);
                        }

                        if (line_touched)
                        {
                            continue;
                        }

                        char line_text[D_BANNER_LINE_MAX];

                        // the line on disk, not the node's own text
                        if (!d_edit_read_line(record, target->line,
                                                  line_text,
                                                  sizeof(line_text)))
                        {
                            continue;
                        }

                        char repaired[D_BANNER_LINE_MAX];

                        // an absent node the sheet can derive is created
                        if (strcmp(claim, "required") == 0)
                        {
                            bool whole_line = false;

                            if (!d_internal_insertion(sheet, tree, hit,
                                                      node_count, node,
                                                      line_text, repaired,
                                                      sizeof(repaired),
                                                      &whole_line))
                            {
                                continue;
                            }

                            // a new line shifts every line below it, so it is
                            // structural and takes the file's pass to itself
                            if (whole_line)
                            {
                                struct d_audit_claim added;
                                const char*          why = NULL;

                                memset(&added, 0, sizeof(added));

                                added.kind  = D_AUDIT_INSERT;
                                added.value = repaired;

                                if (!d_audit_verify("", repaired, &added,
                                                    &why))
                                {
                                    d_internal_refuse(&refused, shown, record,
                                                      target->line,
                                                      "required", why);
                                    continue;
                                }

                                if (d_edit_insert_line(record, target->line,
                                                       repaired))
                                {
                                    ++repairs;
                                    repaired_this_file = true;
                                    break;
                                }

                                continue;
                            }

                            // The inserted value is the derivation's, so it
                            // is not what this checks; what it checks is that
                            // the text before the point survives and that
                            // anything discarded after it is only blanks or a
                            // verified one-word block comment.
                            struct d_audit_claim audit;
                            const char*          why = NULL;
                            const struct d_node* const gone =
                                d_node_at(tree, node);

                            memset(&audit, 0, sizeof(audit));

                            audit.kind  = D_AUDIT_INSERT;
                            audit.head  = gone->start_column - 1u;
                            audit.value = ((audit.head <= strlen(repaired))
                                           ? (repaired + audit.head) : "");
                            // the flag is this node's own attribute, never
                            // a fact about the tree at large
                            const uint32_t replaces_id =
                                d_node_find_id(tree, "replaces");

                            for (uint32_t which_attribute = 0;
                                 which_attribute < gone->attribute_count;
                                 ++which_attribute)
                            {
                                const struct d_node_attribute* const attr =
                                    d_node_attribute_at(tree,
                                        gone->first_attribute
                                        + which_attribute);

                                if ((attr) && (attr->name == replaces_id))
                                {
                                    audit.replaces = true;
                                }
                            }

#ifndef D_AUDIT_DISABLED
                            if (!d_audit_verify(line_text, repaired, &audit,
                                                &why))
                            {
                                d_internal_refuse(&refused, shown, record,
                                                  target->line, "required",
                                                  why);
                                continue;
                            }
#endif

                            if (d_edit_rewrite_line(record, target->line,
                                                    repaired))
                            {
                                ++repairs;
                                touched[touched_count++] = target->line;
                            }

                            continue;
                        }

                        char        derived[D_BANNER_LINE_MAX];
                        const char* derived_or_null = NULL;

                        const char* const property =
                            d_dss_text(sheet, declaration->property);



                        const bool geometric =
                            ( (strcmp(property, "width")        == 0) ||
                              (strcmp(property, "end-column")   == 0) ||
                              (strcmp(property, "start-column") == 0) );

                        // meaning before layout: never align a bad value
                        if ((geometric) &&
                            (!d_internal_meaning_holds(sheet, tree, node)))
                        {
                            continue;
                        }

                        if (strcmp(property, "format") == 0)
                        {
                            if (!d_property_format_extract(sheet, tree, node,
                                                           declaration,
                                                           derived,
                                                           sizeof(derived)))
                            {
                                continue;
                            }

                            derived_or_null = derived;
                        }
                        else if (strcmp(property, "content") == 0)
                        {
                            if (!d_property_derive(sheet, tree, node,
                                                   declaration, derived,
                                                   sizeof(derived)))
                            {
                                continue;
                            }

                            derived_or_null = derived;

                            const uint32_t holder =
                                d_internal_swap_holder(tree, node, derived);

                            if (holder != D_DSS_NO_INDEX)
                            {
                                struct d_audit_claim audit;
                                const char*          why = NULL;
                                const struct d_node* const a =
                                    d_node_at(tree, node);
                                const struct d_node* const b =
                                    d_node_at(tree, holder);

                                memset(&audit, 0, sizeof(audit));

                                audit.kind    = D_AUDIT_SWAP;
                                audit.head    = a->start_column - 1u;
                                audit.run     = a->width;
                                audit.expect  = d_node_text(tree, a->text);
                                audit.head2   = b->start_column - 1u;
                                audit.run2    = b->width;
                                audit.expect2 = d_node_text(tree, b->text);

                                if (!d_internal_swap_runs(tree, node, holder,
                                                          line_text, repaired,
                                                          sizeof(repaired)))
                                {
                                    continue;
                                }

#ifndef D_AUDIT_DISABLED
                                if (!d_audit_verify(line_text, repaired,
                                                    &audit, &why))
                                {
                                    d_internal_refuse(&refused, shown, record,
                                                      target->line,
                                                      "content (swap)", why);
                                    continue;
                                }
#endif

                                if (d_edit_rewrite_line(record, target->line,
                                                        repaired))
                                {
                                    ++repairs;
                                    touched[touched_count++] = target->line;
                                }

                                continue;
                            }
                        }

                        if (!d_property_repair(tree, node,
                                               d_dss_text(sheet,
                                                   declaration->property),
                                               d_property_number(sheet,
                                                   declaration),
                                               line_text,
                                               derived_or_null,
                                               repaired,
                                               sizeof(repaired)))
                        {
                            continue;
                        }

                        struct d_audit_claim audit;
                        const char*          why = NULL;

                        d_internal_claim_for(tree, node, property,
                                             derived_or_null, &audit);

#ifndef D_AUDIT_DISABLED
                        if (!d_audit_verify(line_text, repaired, &audit, &why))
                        {
                            d_internal_refuse(&refused, shown, record, target->line,
                                              property, why);
                            continue;
                        }
#endif

                        if (d_edit_rewrite_line(record, target->line,
                                                repaired))
                        {
                            ++repairs;
                            touched[touched_count++] = target->line;
                        }
                    }
                }

                if (repaired_this_file)
                {
                    break;
                }
            }

            slot += rule->declaration_count;

            if (repaired_this_file)
            {
                break;
            }
        }

        free(hit);
    }

    (void)printf("\n%zu %s, %zu with a banner\n\n", files,
                 every_file ? "files" : "headers", banners);

    // the source tree's own findings, when one was built
    if (sourcing)
    {
        (void)printf("%zu lexical errors in the source trees\n\n", lexical);
    }
    (void)printf("  %-16s %-8s %8s %8s  %s\n", "PROPERTY", "SHEET",
                 "MATCHED", "FAILED", "RATE");

    for (size_t at = 0; at < tally_count; ++at)
    {
        if (tally[at].matched == 0)
        {
            continue;
        }

        // a declaration from the root sheet prints its line, as it always
        // has; one from an import names the file it lives in
        char        origin[PATH_MAX + 16];
        const char* origin_file  = NULL;
        uint32_t    origin_line  = 0;
        const char* origin_layer = NULL;

        d_internal_origin(&expansion, tally[at].line, &origin_file,
                          &origin_line, &origin_layer);

        if (strcmp(origin_file, sheet_path) == 0)
        {
            (void)snprintf(origin, sizeof(origin), "%u", origin_line);
        }
        else
        {
            (void)snprintf(origin, sizeof(origin), "%s:%u", origin_file,
                           origin_line);
        }

        (void)printf("  %-16s %-8s %8zu %8zu  %5.1f%%\n",
                     tally[at].property,
                     origin,
                     tally[at].matched,
                     tally[at].failed,
                     100.0 * (double)tally[at].failed
                           / (double)tally[at].matched);
    }

    (void)printf("\n  resolution: %s    %zu error, %zu warning, "
                 "%zu advisory\n", resolve, errors, warnings, advisories);

    for (size_t at = 0; at < d_settings_count(settings); ++at)
    {
        if (d_settings_source_at(settings, at) != D_SETTING_DEFAULT)
        {
            (void)printf("  set: %s = %s (%s)\n",
                         d_settings_def_at(settings, at)->name,
                         d_settings_value_at(settings, at),
                         d_settings_source_name(
                             d_settings_source_at(settings, at)));
        }
    }

    if (fixing)
    {
        (void)printf("\n  %zu lines repaired\n", repairs);
        (void)printf("  %zu repairs refused by audit\n", refused);
    }

    if (walk.release)
    {
        walk.release(walk.user);
    }

    d_node_tree_free(tree);
    d_cascade_free(cascade);
    d_match_index_free(index);
    d_schema_binding_free(schema);
    free(candidates);
    d_symbol_table_free(symbols);
    d_git_free(history);
    free(tally);
    d_dss_free(sheet);
    d_internal_expansion_free(&expansion);
    d_settings_free(settings);

    // a check fails the run only on an error left standing; a fix pass
    // reports its work through the repair count instead
    // a check fails on an error left standing; a fix fails on any repair
    // the audit refused, since each is a defect the tool tried and could not
    // safely close
    if (fixing)
    {
        return (refused > 0) ? 1 : 0;
    }

    return (errors > 0) ? 1 : 0;
}
