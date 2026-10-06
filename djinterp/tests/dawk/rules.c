/*******************************************************************************
* djinterp [test]                                                        rules.c
*
* Rule harness: a sheet's findings over fixture files, one per line.
*   Builds each file's source tree (dext_c or dext_cpp) and its text layer
* (dline) beneath one file node, evaluates every rule of the sheet through
* the engine's own d_property_evaluate -- so a finding here is a finding
* under dcheck -- and prints `path:line:column: message`, sorted by line.
* rules.sh diffs that against each fixture's `.expected` file.
*
*
* path:      /tests/dawk/rules.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../inc/djinterp/tools/dawk/ext/dext_c.h"    // d_ext_c_build
#include "../../inc/djinterp/tools/dawk/ext/dext_cpp.h"  // d_ext_cpp_build
// std
#include "../../inc/djinterp/tools/dawk/dsymbol.h"
#include <stdio.h>   // printf, fopen
#include <stdlib.h>  // malloc, free, qsort
#include <string.h>  // strcmp
// djinterp
#include "../../inc/djinterp/tools/dawk/dline.h"      // d_line_build_text
#include "../../inc/djinterp/tools/dawk/dmatch.h"     // d_match_rule
#include "../../inc/djinterp/tools/dawk/dproperty.h"  // d_property_evaluate
#include "../../inc/djinterp/tools/dawk/dss.h"        // d_dss_parse


// d_internal_finding
//   struct: one line of output, kept so the whole file's can be sorted.
struct d_internal_finding
{
    uint32_t  line;
    uint32_t  column;
    char      message[160];
};

static int
d_internal_by_position(
    const void* _a,
    const void* _b
)
{
    const struct d_internal_finding* a = _a;
    const struct d_internal_finding* b = _b;

    if (a->line != b->line)
    {
        return (a->line < b->line) ? -1 : 1;
    }

    if (a->column != b->column)
    {
        return (a->column < b->column) ? -1 : 1;
    }

    return strcmp(a->message, b->message);
}

static char*
d_internal_slurp(
    const char* _path,
    size_t*     _length
)
{
    FILE* const handle = fopen(_path, "rb");

    if (!handle)
    {
        return NULL;
    }

    (void)fseek(handle, 0, SEEK_END);

    const long size = ftell(handle);

    (void)fseek(handle, 0, SEEK_SET);

    char* const text = malloc((size_t)((size > 0) ? size : 0) + 1u);

    if (text)
    {
        *_length       = fread(text, 1u, (size_t)((size > 0) ? size : 0),
                               handle);
        text[*_length] = '\0';
    }

    (void)fclose(handle);

    return text;
}

int
main(
    int    _argc,
    char** _argv
)
{
    if (_argc < 3)
    {
        (void)fprintf(stderr, "usage: rules <sheet> <file>...\n");

        return 2;
    }

    size_t      sheet_length = 0u;
    char* const sheet_text   = d_internal_slurp(_argv[1], &sheet_length);

    struct d_dss_error   error;
    struct d_dss_sheet*  sheet = sheet_text
                               ? d_dss_parse(sheet_text, sheet_length, &error)
                               : NULL;

    if (!sheet)
    {
        (void)fprintf(stderr, "rules: %s did not parse\n", _argv[1]);

        return 2;
    }

    struct d_symbol_table* const symbols = d_symbol_table_new();
    uint32_t                     bad_line = 0u;
    const char*                  bad_name = NULL;
    const char*                  bad_why  = NULL;

    // one table for sheet and trees; a property nobody registered is refused
    if ( (!symbols) || (!d_dss_bind(sheet, symbols)) ||
         (!d_schema_validate(d_property_schema(), sheet, &bad_line,
                             &bad_name, &bad_why)) )
    {
        (void)fprintf(stderr, "rules: %s:%u: property '%s' refused: %s\n",
                      _argv[1], bad_line, bad_name ? bad_name : "",
                      bad_why ? bad_why : "no symbol table");

        return 2;
    }

    const size_t rules = d_dss_rule_count(sheet);

    for (int arg = 2; arg < _argc; ++arg)
    {
        size_t      length = 0u;
        char* const text   = d_internal_slurp(_argv[arg], &length);

        if (!text)
        {
            continue;
        }

        struct d_source source;

        d_source_init(&source, text, length, _argv[arg]);
        (void)d_source_skip_bom(&source);

        struct d_token_node_input input = { &source, NULL };
        struct d_node_tree* const tree = d_node_tree_new_shared(symbols);

        const uint32_t file = d_ext_cpp_accepts(_argv[arg])
            ? d_ext_cpp_build(tree, D_DSS_NO_INDEX, 0, &input, NULL)
            : d_ext_c_build(tree, D_DSS_NO_INDEX, 0, &input, NULL);

        // the text layer hangs from the same file node as the source tree
        if (file != D_DSS_NO_INDEX)
        {
            (void)d_line_build_text(tree, file, text, length);
        }

        const size_t               count    = d_node_count(tree);
        size_t                     found    = 0u;
        size_t                     capacity = 64u;
        struct d_internal_finding* findings =
            malloc(capacity * sizeof(*findings));

        for (uint32_t node = 0u; (findings) && (node < (uint32_t)count); ++node)
        {
            for (size_t at = 0u; at < rules; ++at)
            {
                const struct d_dss_rule* const rule =
                    d_dss_rule_at(sheet, (uint32_t)at);

                if (!d_match_rule(sheet, tree, node, rule))
                {
                    continue;
                }

                for (uint32_t which = 0u; which < rule->declaration_count;
                     ++which)
                {
                    const struct d_dss_declaration* const declaration =
                        d_dss_declaration_at(sheet,
                                             rule->first_declaration + which);
                    bool       evaluated = false;
                    const bool holds     = d_property_evaluate(sheet, tree,
                                                               node,
                                                               declaration,
                                                               &evaluated);

                    // a finding: a declaration that evaluated and failed
                    if ( (!evaluated) || (holds) )
                    {
                        continue;
                    }

                    if (found == capacity)
                    {
                        capacity *= 2u;

                        struct d_internal_finding* const grown =
                            realloc(findings, capacity * sizeof(*findings));

                        if (!grown)
                        {
                            break;
                        }

                        findings = grown;
                    }

                    const struct d_node* const  at_node = d_node_at(tree,
                                                                    node);
                    const char* const           property =
                        d_dss_text(sheet, declaration->property);
                    const struct d_dss_value* const value =
                        d_dss_value_at(sheet, declaration->first_value);

                    findings[found].line   = at_node->line;
                    findings[found].column = at_node->start_column;
                    (void)snprintf(findings[found].message,
                                   sizeof(findings[found].message), "%s",
                                   ( (strcmp(property, "violation") == 0) &&
                                     (value) &&
                                     (value->text != D_DSS_NO_INDEX) )
                                   ? d_dss_text(sheet, value->text)
                                   : property);
                    ++found;
                }
            }
        }

        if (findings)
        {
            qsort(findings, found, sizeof(*findings), d_internal_by_position);

            for (size_t at = 0u; at < found; ++at)
            {
                (void)printf("%s:%u:%u: %s\n", _argv[arg], findings[at].line,
                             findings[at].column, findings[at].message);
            }
        }

        free(findings);
        d_node_tree_free(tree);
        free(text);
    }

    d_dss_free(sheet);
    free(sheet_text);

    return 0;
}
