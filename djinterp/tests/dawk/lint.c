/*******************************************************************************
* djinterp [test]                                                         lint.c
*
* Lint driver: build a tree per file through the extension that claims.
*
*
* path:      /tests/dawk/lint.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// lint driver: build a tree per file through the extension that claims
// it, run a sheet, and report every finding.  A rule carrying `severity`
// reports each match; a rule carrying `start-column` reports each match
// whose column differs.  Anything else is counted as unevaluated.
#include "../../inc/djinterp/tools/dawk/ext/dext_c.h"
#include "../../inc/djinterp/tools/dawk/ext/dext_cpp.h"
#include "../../inc/djinterp/tools/dawk/dmatch.h"
#include "../../inc/djinterp/tools/dawk/dss.h"
#include "../../inc/djinterp/tools/dawk/dproperty.h"
#include "../../inc/djinterp/tools/dawk/dsymbol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// the extensions take a source; this makes one from text, as a host would
static const struct d_source* source_of(const char* _text, size_t _length, const char* _path)
{
    static struct d_source s;
    d_source_init(&s, _text, _length, _path);
    (void)d_source_skip_bom(&s);
    return &s;
}

// and the input a build reads: that source, and no sink
static const struct d_token_node_input* input_of(const char* _text, size_t _length, const char* _path)
{
    static struct d_token_node_input in;
    in.source = source_of(_text, _length, _path);
    in.sink   = NULL;
    return &in;
}

static char* slurp(const char* path, size_t* n)
{
    FILE* f = fopen(path, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
    char* b = malloc((size_t)len + 1); *n = fread(b, 1, (size_t)len, f);
    fclose(f); b[*n] = 0; return b;
}

static void print_line(const char* text, size_t len, uint32_t line)
{
    uint32_t at = 1; size_t i = 0;
    while (i < len && at < line) { if (text[i++] == '\n') ++at; }
    size_t j = i; while (j < len && text[j] != '\n' && text[j] != '\r') ++j;
    while (i < j && (text[i] == ' ' || text[i] == '\t')) ++i;
    printf("      | %.*s\n", (int)((j - i) > 70 ? 70 : (j - i)), text + i);
}

int main(int argc, char** argv)
{
    if (argc < 3) { fprintf(stderr, "usage: lint <sheet> <files...>\n"); return 2; }
    size_t sn = 0; char* st = slurp(argv[1], &sn);
    struct d_dss_error err;
    struct d_dss_sheet* sheet = d_dss_parse(st, sn, &err);
    struct d_symbol_table* symbols = d_symbol_table_new();
    if (sheet) (void)d_dss_bind(sheet, symbols);
    if (!sheet) { fprintf(stderr, "sheet did not parse\n"); return 2; }

    const size_t rules = d_dss_rule_count(sheet);
    size_t* hits = calloc(rules, sizeof(size_t));
    size_t files = 0, nodes = 0, shown = 0, limit = 24, lex_errors = 0;
    const char* env = getenv("LINT_SHOW"); if (env) limit = (size_t)atoi(env);

    size_t generated = 0;
    for (int a = 2; a < argc; ++a) {
        size_t n = 0; char* text = slurp(argv[a], &n); if (!text) continue;
        // a file that declares itself generated is judged by its generator,
        // not by a style sheet: look for the marker near its top
        { size_t head = n < 4096 ? n : 4096; char c = text[head]; text[head] = 0;
          int gen = strstr(text, "Auto-generated. Do not edit by hand.") != NULL;
          text[head] = c;
          if (gen) { ++generated; free(text); continue; } }
        struct d_node_tree* tree = d_node_tree_new_shared(symbols);
        struct d_token_node_stats stats;
        uint32_t root = d_ext_cpp_accepts(argv[a])
            ? d_ext_cpp_build(tree, D_DSS_NO_INDEX, 0, input_of(text, n, argv[a]), &stats)
            : d_ext_c_build(tree, D_DSS_NO_INDEX, 0, input_of(text, n, argv[a]), &stats);
        if (root == D_DSS_NO_INDEX) { d_node_tree_free(tree); free(text); continue; }
        ++files; nodes += d_node_count(tree); lex_errors += stats.lex_errors;

        for (uint32_t node = 0; node < (uint32_t)d_node_count(tree); ++node) {
            for (size_t r = 0; r < rules; ++r) {
                const struct d_dss_rule* rule = d_dss_rule_at(sheet, (uint32_t)r);
                if (!d_match_rule(sheet, tree, node, rule)) continue;
                for (uint32_t d = 0; d < rule->declaration_count; ++d) {
                    const struct d_dss_declaration* decl =
                        d_dss_declaration_at(sheet, rule->first_declaration + d);
                    const char* prop = d_dss_text(sheet, decl->property);
                    const struct d_dss_value* v = d_dss_value_at(sheet, decl->first_value);
                    struct d_node* nd = d_node_at(tree, node);
                    int finding = 0; char why[96] = "";
                    // the engine's evaluator decides, as it does for dcheck:
                    // a finding is a declaration that evaluated and failed
                    bool evaluated = false;
                    const bool holds = d_property_evaluate(sheet, tree, node, decl, &evaluated);
                    if (evaluated && !holds) {
                        finding = 1;
                        if (!strcmp(prop, "violation") && v && v->text != D_DSS_NO_INDEX)
                            snprintf(why, sizeof why, "%s", d_dss_text(sheet, v->text));
                        else if (!strcmp(prop, "start-column"))
                            snprintf(why, sizeof why, "column %u, want %.0f",
                                     nd->start_column, v ? v->number : 0.0);
                        else
                            snprintf(why, sizeof why, "%s fails", prop);
                    }
                    if (!finding) continue;
                    ++hits[r];
                    if (shown++ < limit) {
                        printf("  %s:%u:%u  rule %zu (line %u of sheet): %s\n",
                               argv[a], nd->line, nd->start_column, r,
                               rule->line, why);
                        print_line(text, n, nd->line);
                    }
                }
            }
        }
        d_node_tree_free(tree); free(text);
    }

    printf("\n%zu files, %zu nodes (%zu generated files skipped), %zu lexical errors\n",
           files, nodes, generated, lex_errors);
    for (size_t r = 0; r < rules; ++r)
        printf("  rule %zu (sheet line %u): %zu finding(s)\n",
               r, d_dss_rule_at(sheet, (uint32_t)r)->line, hits[r]);
    free(hits); d_dss_free(sheet); free(st);
    return 0;
}
