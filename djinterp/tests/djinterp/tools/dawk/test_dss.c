#include "../../../../inc/djinterp/tools/dawk/dss.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* kind_name(unsigned k) {
    static const char* n[] = {"*","type","class","attr","pseudo","pseudo-el"};
    return (k < 6) ? n[k] : "?";
}
static const char* comb_name(unsigned c) {
    static const char* n[] = {"desc",">","+","..."};
    return (c < 4) ? n[c] : "?";
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: test_dss <sheet.dss>\n"); return 2; }
    FILE* f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 2; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char* buf = malloc((size_t)n + 1); size_t got = fread(buf, 1, (size_t)n, f);
    buf[got] = 0; fclose(f);

    struct d_dss_error err;
    struct d_dss_sheet* sheet = d_dss_parse(buf, got, &err);
    if (!sheet) {
        fprintf(stderr, "%s:%u:%u: %s\n", argv[1], err.line, err.column,
                err.message ? err.message : "parse failed");
        free(buf); return 1;
    }

    printf("at-rules: %zu   style rules: %zu\n\n",
           d_dss_at_rule_count(sheet), d_dss_rule_count(sheet));

    for (size_t i = 0; i < d_dss_at_rule_count(sheet); i++) {
        const struct d_dss_at_rule* a = d_dss_at_rule_at(sheet, i);
        printf("  @%s %s  (%u rows, line %u)\n", d_dss_text(sheet, a->name),
               a->prelude == D_DSS_NO_INDEX ? "" : d_dss_text(sheet, a->prelude),
               a->declaration_count, a->line);
    }

    printf("\n");
    for (size_t i = 0; i < d_dss_rule_count(sheet); i++) {
        const struct d_dss_rule* r = d_dss_rule_at(sheet, i);
        printf("  line %-4u ", r->line);
        for (uint32_t s = 0; s < r->selector_count; s++) {
            const struct d_dss_selector* sel =
                d_dss_selector_at(sheet, r->first_selector + s);
            if (s) printf(", ");
            const struct d_dss_compound* c = d_dss_compound_at(sheet, sel->head);
            for (uint32_t k = 0; k < c->simple_count; k++) {
                const struct d_dss_simple* sm =
                    d_dss_simple_at(sheet, c->first_simple + k);
                printf("%s(%s)", kind_name(sm->kind),
                       sm->name == D_DSS_NO_INDEX ? "" : d_dss_text(sheet, sm->name));
            }
            for (uint32_t t = 0; t < sel->step_count; t++) {
                const struct d_dss_step* st =
                    d_dss_step_at(sheet, sel->first_step + t);
                printf(" %s ", comb_name(st->combinator));
                const struct d_dss_compound* cc =
                    d_dss_compound_at(sheet, st->compound);
                for (uint32_t k = 0; k < cc->simple_count; k++) {
                    const struct d_dss_simple* sm =
                        d_dss_simple_at(sheet, cc->first_simple + k);
                    printf("%s(%s)", kind_name(sm->kind),
                           sm->name == D_DSS_NO_INDEX ? "" : d_dss_text(sheet, sm->name));
                }
            }
        }
        printf("  {");
        for (uint32_t d = 0; d < r->declaration_count; d++) {
            const struct d_dss_declaration* dd =
                d_dss_declaration_at(sheet, r->first_declaration + d);
            printf(" %s:%u", d_dss_text(sheet, dd->property), dd->value_count);
        }
        printf(" }\n");
    }
    d_dss_free(sheet); free(buf);
    return 0;
}
